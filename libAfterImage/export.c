/* This file contains code for unified image writing into many file formats */
/********************************************************************/
/* Copyright (c) 2001 Sasha Vasko <sashav@sprintmail.com>           */
/********************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

/*#define LOCAL_DEBUG*/
#define DO_CLOCKING

#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
/* <setjmp.h> is used for the optional error recovery mechanism */

#ifdef HAVE_PNG
/* Include file for users of png library. */
#include <png.h>
#else
#include <setjmp.h>
#endif

#include "afterbase.h"
#ifdef HAVE_JPEG
/* Include file for users of png library. */
#include <jpeglib.h>
#endif
#ifdef HAVE_GIF
#include <gif_lib.h>
#endif
#ifdef HAVE_TIFF
#include <tiff.h>
#include <tiffio.h>
#endif
#ifdef HAVE_LIBXPM
#ifdef HAVE_LIBXPM_X11
#include <X11/xpm.h>
#else
#include <xpm.h>
#endif
#endif

#include "asimage.h"
#include "xcf.h"
#include "xpm.h"
#include "import.h"
#include "export.h"

inline int asimage_decode_line (ASImage * im, ColorPart color, CARD32 * to_buf, unsigned int y, unsigned int skip, unsigned int out_width);

/***********************************************************************************/
/* High level interface : 														   */
as_image_writer_func as_image_file_writers[ASIT_Unknown] =
{
	ASImage2xpm ,
	ASImage2xpm ,
	ASImage2xpm ,
	ASImage2png ,
	ASImage2jpeg,
	ASImage2xcf ,
	ASImage2ppm ,
	ASImage2ppm ,
	ASImage2bmp ,
	ASImage2ico ,
	ASImage2ico ,
	ASImage2gif ,
	ASImage2tiff,
	NULL,
	NULL,
	NULL
};

Bool
ASImage2file( ASImage *im, const char *dir, const char *file,
			  ASImageFileTypes type, int subimage,
			  unsigned int compression, unsigned int quality,
			  int max_colors, int depth )
{
	int   filename_len, dirname_len = 0 ;
	char *realfilename = NULL ;
	Bool  res = False ;

	if( im == NULL && file == NULL ) return False;

	filename_len = strlen(file);
	if( dir != NULL )
		dirname_len = strlen(dir)+1;
	realfilename = safemalloc( dirname_len+filename_len+1 );
	if( dir != NULL )
	{
		strcpy( realfilename, dir );
		realfilename[dirname_len-1] = '/' ;
	}
	strcpy( realfilename+dirname_len, file );

	if( type >= ASIT_Unknown || type < 0 )
		show_error( "Hmm, I don't seem to know anything about format you trying to write file \"%s\" in\n.\tPlease check the manual", realfilename );
   	else if( as_image_file_writers[type] )
   		res = as_image_file_writers[type](im, realfilename, type, subimage, compression, quality, max_colors, depth);
   	else
   		show_error( "Support for the format of image file \"%s\" has not been implemented yet.", realfilename );

	free( realfilename );
	return res;
}

/* hmm do we need pixmap2file ???? */

/***********************************************************************************/
/* Some helper functions :                                                         */

static FILE*
open_writeable_image_file( const char *path )
{
	FILE *fp = NULL;
	if ( path )
		if ((fp = fopen (path, "wb")) == NULL)
			show_error("cannot open image file \"%s\" for writing. Please check permissions.", path);
	return fp ;
}

void
scanline2raw( register CARD8 *row, ASScanline *buf, CARD8 *gamma_table, unsigned int width, Bool grayscale, Bool do_alpha )
{
	register int x = width;

	if( grayscale )
		row += do_alpha? width<<1 : width ;
	else
		row += width*(do_alpha?4:3) ;

	if( gamma_table )
	{
		if( !grayscale )
		{
			while ( --x >= 0 )
			{
				row -= 3 ;
				if( do_alpha )
				{
					--row;
					buf->alpha[x] = row[3];
				}
				buf->xc1[x]  = gamma_table[row[0]];
				buf->xc2[x]= gamma_table[row[1]];
				buf->xc3[x] = gamma_table[row[2]];
			}
		}else /* greyscale */
			while ( --x >= 0 )
			{
				if( do_alpha )
					buf->alpha[x] = *(--row);
				buf->xc1 [x] = buf->xc2[x] = buf->xc3[x]  = gamma_table[*(--row)];
			}
	}else
	{
		if( !grayscale )
		{
			while ( --x >= 0 )
			{
				row -= 3 ;
				if( do_alpha )
				{
					--row;
					buf->alpha[x] = row[3];
				}
				buf->xc1[x]  = row[0];
				buf->xc2[x]= row[1];
				buf->xc3[x] = row[2];
			}
		}else /* greyscale */
			while ( --x >= 0 )
			{
				if( do_alpha )
					buf->alpha[x] = *(--row);
				buf->xc1 [x] = buf->xc2[x] = buf->xc3[x]  = *(--row);
			}
	}
}
/***********************************************************************************/
/* reduced colormap building code :                                                */
/***********************************************************************************/
typedef struct ASMappedColor
{
	CARD8  alpha, red, green, blue;
	CARD32 indexed;
	unsigned int count ;
	int cmap_idx ;
	struct ASMappedColor *next ;
}ASMappedColor;

typedef struct ASSortedColorStack
{
	unsigned int count, count_used ;
	ASMappedColor *stack ;

	int good_offset ;                       /* skip to closest stack that
											 * has mapped colors */
	ASMappedColor *first_good, *last_good ; /* pointers to first and last
											 * mapped colors in the stack */
}ASSortedColorStack;

#define MAKE_INDEXED_COLOR24(red,green,blue) \
                   ((((green&0x200)|(blue&0x100)|(red&0x80))<<14)| \
		            (((green&0x100)|(blue&0x80) |(red&0x40))<<12)| \
		            (((green&0x80) |(blue&0x40) |(red&0x20))<<10)| \
					(((green&0x40) |(blue&0x20) |(red&0x10))<<8)| \
					(((green&0x20) |(blue&0x10) |(red&0x08))<<6)| \
					(((green&0x10) |(blue&0x08) |(red&0x04))<<4)| \
					(((green&0x08) |(blue&0x04) |(red&0x02))<<2)| \
					 ((green&0x04) |(blue&0x02) |(red&0x01)))
#define MAKE_INDEXED_COLOR21(red,green,blue) \
                   ((((green&0x200)|(blue&0x100)|(red&0x80))<<12)| \
		            (((green&0x100)|(blue&0x80) |(red&0x40))<<10)| \
		            (((green&0x80) |(blue&0x40) |(red&0x20))<<8)| \
					(((green&0x40) |(blue&0x20) |(red&0x10))<<6)| \
					(((green&0x20) |(blue&0x10) |(red&0x08))<<4)| \
					(((green&0x10) |(blue&0x08) |(red&0x04))<<2)| \
					(((green&0x08) |(blue&0x04) |(red&0x02))>>1))

#define SLOTS_OFFSET24 15
#define SLOTS_MASK24   0x1FF
#define SLOTS_OFFSET21 12
#define SLOTS_MASK21   0x1FF

#define MAKE_INDEXED_COLOR MAKE_INDEXED_COLOR21
#define SLOTS_OFFSET	9
#define SLOTS_MASK		0xFFF
#define COLOR_STACK_SLOTS		  4096


typedef struct ASSortedColorIndex
{
	unsigned int count ;
	ASSortedColorStack stacks[COLOR_STACK_SLOTS] ;
	CARD32  last_found ;
	int     last_idx ;
}ASSortedColorIndex;

typedef struct ASColormapEntry
{
	CARD8 red, green, blue;
}ASColormapEntry;

typedef struct ASColormap
{
	ASColormapEntry *entries ;
	unsigned int count ;
	ASHashTable *xref ;
}ASColormap;


static inline ASMappedColor *new_mapped_color( CARD8 red, CARD8 green, CARD8 blue, CARD32 indexed )
{
	register ASMappedColor *pnew = malloc( sizeof( ASMappedColor ));
	if( pnew != NULL )
	{
		pnew->red = red ;
		pnew->green = green ;
		pnew->blue = blue ;
		pnew->indexed = indexed ;
		pnew->count = 1 ;
		pnew->cmap_idx = -1 ;
		pnew->next = NULL ;
/*LOCAL_DEBUG_OUT( "indexed color added: 0x%X(%d): #%2.2X%2.2X%2.2X", indexed, indexed, red, green, blue );*/
	}
	return pnew;
}

int
add_index_color( ASSortedColorIndex *index, CARD32 red, CARD32 green, CARD32 blue )
{
	ASSortedColorStack *stack ;
	ASMappedColor **pnext ;
	CARD32 indexed ;

	green = green << 2 ;
	blue = blue <<1 ;
	indexed = MAKE_INDEXED_COLOR(red,green,blue);
	green = green >> 2 ;
	blue = blue >>1 ;
	stack = &(index->stacks[(indexed>>SLOTS_OFFSET)&SLOTS_MASK]);
	pnext = &(stack->stack);
	while( *pnext )
	{
		register ASMappedColor *pelem = *pnext ;/* to avoid double redirection */
		if( pelem->indexed == indexed )
		{
			++(stack->count_used);
			return ++(pelem->count);
		}
		else if( pelem->indexed > indexed )
		{
			register ASMappedColor *pnew = new_mapped_color( red, green, blue, indexed );
			if( pnew )
			{
				++(index->count);
				++(stack->count);
				++(stack->count_used);
				pnew->next = pelem ;
				*pnext = pnew ;
				return 1;
			}
		}
		pnext = &(pelem->next);
	}
	/* we want to avoid memory overflow : */
	if( (*pnext = new_mapped_color( red, green, blue, indexed )) != NULL )
	{
		++(index->count);
		++(stack->count);
		++(stack->count_used);
		return 1;
	}
	return -1;
}

void destroy_colorindex( ASSortedColorIndex *index, Bool reusable )
{
	if( index )
	{
		int i ;
		for( i = 0 ; i < COLOR_STACK_SLOTS ; i++ )
			while( index->stacks[i].stack )
			{
				ASMappedColor *to_free = index->stacks[i].stack;
				index->stacks[i].stack = to_free->next ;
				free( to_free );
			}
		if( !reusable )
			free( index );
	}
}

static inline void
add_colormap_item( register ASColormapEntry *pentry, ASMappedColor *pelem, int cmap_idx )
{
	pentry->red = pelem->red ;
	pentry->green = pelem->green ;
	pentry->blue = pelem->blue ;
	pelem->cmap_idx = cmap_idx ;
LOCAL_DEBUG_OUT( "colormap entry added: %d: #%2.2X%2.2X%2.2X",cmap_idx, pelem->red, pelem->green, pelem->blue );
}

unsigned int
add_colormap_items( ASSortedColorIndex *index, unsigned int start, unsigned int stop, unsigned int quota, unsigned int base, ASColormapEntry *entries )
{
	int cmap_idx = 0 ;
	int i ;
	if( quota >= index->count )
	{
		for( i = start ; i < stop ; i++ )
		{
			register ASMappedColor *pelem = index->stacks[i].stack ;
			while ( pelem != NULL )
			{
				add_colormap_item( &(entries[cmap_idx]), pelem, base++ );
				++cmap_idx ;
				pelem = pelem->next ;
			}
		}
	}else
	{
		int total = 0 ;
		int subcount = 0 ;
		ASMappedColor *best = NULL ;
		for( i = start ; i <= stop ; i++ )
			total += index->stacks[i].count_used ;

		for( i = start ; i <= stop ; i++ )
		{
			register ASMappedColor *pelem = index->stacks[i].stack ;
			while ( pelem != NULL /*&& cmap_idx < quota*/ )
			{
				if( pelem->cmap_idx < 0 )
				{
					if( best == NULL )
						best = pelem ;
					else if( best->count < pelem->count )
						best = pelem ;
					else if( best->count == pelem->count &&
						     subcount >= (total>>2) && subcount <= (total>>1)*3 )
						best = pelem ;
					subcount += pelem->count*quota ;
/*LOCAL_DEBUG_OUT( "count = %d subtotal = %d, quota = %d, idx = %d, i = %d, total = %d", pelem->count, subcount, quota, cmap_idx, i, total );*/
					if( subcount >= total )
					{
						add_colormap_item( &(entries[cmap_idx]), best, base++ );
						++cmap_idx ;
						subcount -= total ;
						best = NULL ;
					}
				}
				pelem = pelem->next ;
			}
		}
	}
	return cmap_idx ;
}

void
fix_colorindex_shortcuts( ASSortedColorIndex *index )
{
	int i ;
	int last_good = -1, next_good = -1;

	index->last_found = -1 ;

	for( i = 0 ; i < COLOR_STACK_SLOTS ; i++ )
	{
		register ASMappedColor *pelem = index->stacks[i].stack ;
		ASMappedColor *first = NULL, *last = NULL ;
		while( pelem != NULL )
		{
			if( pelem->cmap_idx >= 0 )
			{
				if( first == NULL ) first = pelem ;
				last = pelem ;
			}
			pelem = pelem->next ;
		}
		index->stacks[i].first_good = first ;
		index->stacks[i].last_good = last ;
	}
	for( i = 0 ; i < COLOR_STACK_SLOTS ; i++ )
	{
		if( next_good < 0 )
		{
			for( next_good = i ; next_good < COLOR_STACK_SLOTS ; next_good++ )
				if( index->stacks[next_good].first_good )
					break;
			if( next_good >= COLOR_STACK_SLOTS )
				next_good = last_good ;
		}
		if( index->stacks[i].first_good )
		{
			last_good = i;
			next_good = -1;
		}else
		{
			if( last_good < 0 || ( i-last_good >= next_good-i && i < next_good ) )
				index->stacks[i].good_offset = next_good-i ;
			else
				index->stacks[i].good_offset = last_good-i ;
		}
	}
}

ASColormap *
color_index2colormap( ASSortedColorIndex *index, unsigned int max_colors, ASColormap *reusable_memory )
{
	ASColormap *cmap = reusable_memory;
	int cmap_idx = 0 ;
	int i ;

	if( cmap == NULL )
		cmap = safecalloc( 1, sizeof(ASColormap));
	cmap->count = MIN(max_colors,index->count);
	cmap->entries = safemalloc( cmap->count*sizeof( ASColormapEntry) );
	/* now lets go ahead and populate colormap : */
	if( index->count <= max_colors )
	{
		add_colormap_items( index, 0, COLOR_STACK_SLOTS, index->count, 0, cmap->entries);
	}else
	{
		long total = 0 ;
		long subcount = 0 ;
		int start_slot = 0 ;
		for( i = 0 ; i <= COLOR_STACK_SLOTS ; i++ )
			total += index->stacks[i].count_used ;

		for( i = 0 ; i < COLOR_STACK_SLOTS ; i++ )
		{
			subcount += index->stacks[i].count_used*max_colors ;
/*LOCAL_DEBUG_OUT( "count = %d, subtotal = %d, to_add = %d, idx = %d, i = %d, total = %d", index->stacks[i].count, subcount, subcount/total, cmap_idx, i, total );*/
			if( subcount >= total )
			{	/* we need to add subcount/index->count items */
				int to_add = subcount/total ;
				if( i == COLOR_STACK_SLOTS-1 && to_add < max_colors-cmap_idx )
					to_add = max_colors-cmap_idx;
				cmap_idx += add_colormap_items( index, start_slot, i, to_add, cmap_idx, &(cmap->entries[cmap_idx]));
				subcount %= total;
				start_slot = i+1;
			}
		}
	}
	fix_colorindex_shortcuts( index );
	return cmap;
}

void destroy_colormap( ASColormap *cmap, Bool reusable )
{
	if( cmap )
	{
		if( cmap->entries )
			free( cmap->entries );
		if( !reusable )
			free( cmap );
	}
}

int
get_color_index( ASSortedColorIndex *index, CARD32 red, CARD32 green, CARD32 blue )
{
	ASSortedColorStack *stack ;
	ASMappedColor *pnext, *lesser ;
	CARD32 indexed ;
	int slot, offset ;

	green = green << 2 ;
	blue = blue <<1 ;
	indexed = MAKE_INDEXED_COLOR(red,green,blue);
	if( index->last_found == indexed )
		return index->last_idx;
	index->last_found = indexed ;

	slot = (indexed>>SLOTS_OFFSET)&SLOTS_MASK ;
/*LOCAL_DEBUG_OUT( "index = %X(%d), slot = %d, offset = %d", indexed, indexed, slot, index->stacks[slot].good_offset );*/
	if( (offset = index->stacks[slot].good_offset) != 0 )
		slot += offset ;
	stack = &(index->stacks[slot]);
/*LOCAL_DEBUG_OUT( "first_good = %X(%d), last_good = %X(%d)", stack->first_good->indexed, stack->first_good->indexed, stack->last_good->indexed, stack->last_good->indexed );*/
	if( offset < 0 || stack->last_good->indexed <= indexed )
		return (index->last_idx=stack->last_good->cmap_idx);
	if( offset > 0 || stack->first_good->indexed >= indexed )
		return (index->last_idx=stack->first_good->cmap_idx);

	lesser = stack->first_good ;
	for( pnext = lesser; pnext != stack->last_good ; pnext = pnext->next )
		if( pnext->cmap_idx >= 0 )
		{
/*LOCAL_DEBUG_OUT( "lesser = %X(%d), pnext = %X(%d)", lesser->indexed, lesser->indexed, pnext->indexed, pnext->indexed );*/
			if( pnext->indexed >= indexed )
			{
				index->last_idx = ( pnext->indexed-indexed > indexed-lesser->indexed )?
						lesser->cmap_idx : pnext->cmap_idx ;
				return index->last_idx;
			}
			lesser = pnext ;
		}
	return stack->last_good->cmap_idx;
}

void
quantize_scanline( ASScanline *scl, ASSortedColorIndex *index, ASColormap *cmap, int *dst, int transp_threshold, int transp_idx )
{
	int i ;
	register CARD32 *a = scl->alpha ;
#if 0
	register int red = 0, green = 0, blue = 0;
	for( i = 0 ; i < scl->width ; i++ )
		if( a[i] >= transp_threshold )
		{
			ASColormapEntry mc ;
			red += scl->red[i] ;
			if( red < 0 ) red = 0 ;
			else if( red > 255 ) red = 255 ;

			green += scl->green[i] ;
			if( green < 0 ) green = 0 ;
			else if( green > 255 ) green = 255 ;

			blue += scl->blue[i] ;
			if( blue < 0 ) blue = 0 ;
			else if( blue > 255 ) blue = 255 ;

			dst[i] = get_color_index( index, red, green, blue );
			mc = cmap->entries[dst[i]] ;
			if( red != mc.red || green != mc.green || blue != mc.blue )

			red   = (mc.red  -red  )>>1 ;
			green = (mc.green-green)>>1 ;
			blue  = (mc.blue -blue )>>1 ;
#endif
	register CARD32 *red = scl->red, *green = scl->green, *blue = scl->blue;

	if( transp_threshold > 0 )
	{
		for( i = 0 ; i < scl->width ; i++ )
			if( a[i] >= transp_threshold )
				dst[i] = get_color_index( index, red[i], green[i], blue[i] );
			else
				dst[i] = transp_idx ;
	}else
		for( i = 0 ; i < scl->width ; i++ )
			dst[i] = get_color_index( index, red[i], green[i], blue[i] );

}

/***********************************************************************************/
#define SHOW_PENDING_IMPLEMENTATION_NOTE(f) \
	show_error( "I'm sorry, but " f " image writing is pending implementation. Appreciate your patience" )
#define SHOW_UNSUPPORTED_NOTE(f,path) \
	show_error( "unable to write file \"%s\" - " f " image format is not supported.\n", (path) )


#ifdef HAVE_XPM      /* XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM */

#ifdef LOCAL_DEBUG
Bool print_component( CARD32*, int, unsigned int );
#endif

typedef struct ASXpmCharmap
{
	unsigned int count ;
	unsigned int cpp ;
	char *char_code ;
}ASXpmCharmap;

#define MAXPRINTABLE 92			/* number of printable ascii chars
								 * minus \ and " for string compat
								 * and ? to avoid ANSI trigraphs. */

static char *printable =
" .XoO+@#$%&*=-;:>,<1234567890qwertyuipasdfghjklzxcvbnmMNBVCZASDFGHJKLPIUYTREWQ!~^/()_`'][{}|";

ASXpmCharmap*
build_xpm_charmap( ASColormap *cmap, Bool has_alpha, ASXpmCharmap *reusable_memory )
{
	ASXpmCharmap *xpm_cmap = reusable_memory ;
	char *ptr ;
	int i ;
	int rem ;

	xpm_cmap->count = cmap->count+((has_alpha)?1:0) ;

	xpm_cmap->cpp = 0 ;
	for( rem = xpm_cmap->count ; rem > 0 ; rem = rem/MAXPRINTABLE )
		++(xpm_cmap->cpp) ;
	ptr = xpm_cmap->char_code = safemalloc(xpm_cmap->count*(xpm_cmap->cpp+1)) ;
	for( i = 0 ; i < xpm_cmap->count ; i++ )
	{
		register int k = xpm_cmap->cpp ;
		rem = i ;
		ptr[k] = '\0' ;
		while( --k >= 0 )
		{
			ptr[k] = printable[rem%MAXPRINTABLE] ;
			rem /= MAXPRINTABLE ;
		}
		ptr += xpm_cmap->cpp+1 ;
	}

	return xpm_cmap;
}

void destroy_xpm_charmap( ASXpmCharmap *xpm_cmap, Bool reusable )
{
	if( xpm_cmap )
	{
		if( xpm_cmap->char_code )
			free( xpm_cmap->char_code );
		if( !reusable )
			free( xpm_cmap );
	}
}

Bool
ASImage2xpm ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	FILE *outfile;
	ASScanline imbuf ;
	Bool skip_alpha = False;
	Bool has_alpha = False;
	int alpha_thresh = 0 ;
	int y, x ;
	int *row_pointer ;
	ASSortedColorIndex *index;                 /* better not allocate such a large structure on stack!!! */
	ASColormap         cmap;
	START_TIME(started);
	ASXpmCharmap       xpm_cmap ;
	int xpm_idx = 0 ;

	LOCAL_DEBUG_CALLER_OUT ("(\"%s\")", path);

	if ((outfile = open_writeable_image_file( path )) == NULL)
		return False;

	index = safecalloc( 1, sizeof(ASSortedColorIndex));
	prepare_scanline( im->width, 0, &imbuf, False );
	row_pointer = safemalloc( im->width*sizeof(int));

	for( y = 0 ; y < im->height ; y++ )
	{
		asimage_decode_line (im, IC_RED,   imbuf.red,   y, 0, imbuf.width);
		asimage_decode_line (im, IC_GREEN, imbuf.green, y, 0, imbuf.width);
		asimage_decode_line (im, IC_BLUE,  imbuf.blue,  y, 0, imbuf.width);
		if( !skip_alpha )
			if( asimage_decode_line (im, IC_ALPHA, imbuf.alpha, y, 0, imbuf.width) <= imbuf.width )
				skip_alpha = True ;
		for( x = 0; x < imbuf.width ; x++ )
		{
#if 0
			if( !skip_alpha )
				if( imbuf.alpha[x] < 127 )
				{
					has_alpha = True ;
					continue;
				}
#endif
			add_index_color( index, imbuf.red[x],imbuf.green[x],imbuf.blue[x]);
		}
	}
	if( skip_alpha )
		has_alpha = False ;
	if( has_alpha )
		alpha_thresh = 127 ;
	SHOW_TIME("color indexing",started);

LOCAL_DEBUG_OUT("building colormap%s","");
	color_index2colormap( index, max_colors, &cmap );
	SHOW_TIME("colormap calculation",started);

LOCAL_DEBUG_OUT("building charmap%s","");
	build_xpm_charmap( &cmap, has_alpha, &xpm_cmap );
	SHOW_TIME("charmap calculation",started);

LOCAL_DEBUG_OUT("writing file%s","");
	fprintf( outfile, "/* XPM */\nstatic char *asxpm[] = {\n/* columns rows colors chars-per-pixel */\n"
					  "\"%d %d %d %d\",\n", im->width, im->height, xpm_cmap.count,  xpm_cmap.cpp );
	for( y = 0 ; y < cmap.count ; y++ )
	{
		fprintf( outfile, "\"%s c #%2.2X%2.2X%2.2X\",\n", &(xpm_cmap.char_code[xpm_idx]), cmap.entries[y].red, cmap.entries[y].green, cmap.entries[y].blue );
		xpm_idx += xpm_cmap.cpp+1 ;
	}
	if( has_alpha && y < xpm_cmap.count )
		fprintf( outfile, "\"%s c None\",\n", &(xpm_cmap.char_code[xpm_idx]) );
	SHOW_TIME("image header writing",started);

	for( y = 0 ; y < im->height ; y++ )
	{
		asimage_decode_line (im, IC_ALPHA, imbuf.alpha, y, 0, imbuf.width);
		asimage_decode_line (im, IC_RED,   imbuf.red, y, 0, imbuf.width);
		asimage_decode_line (im, IC_GREEN, imbuf.green, y, 0, imbuf.width);
		asimage_decode_line (im, IC_BLUE,  imbuf.blue, y, 0, imbuf.width);
		quantize_scanline( &imbuf, index, &cmap, row_pointer,  alpha_thresh, xpm_cmap.count-1 );
		fputc( '"', outfile );
		for( x = 0; x < imbuf.width ; x++ )
			fprintf( outfile, "%s", &(xpm_cmap.char_code[row_pointer[x]*(xpm_cmap.cpp+1)]) );
		if( y < im->height-1 )
			fprintf( outfile, "\",\n" );
		else
			fprintf( outfile, "\"\n" );
	}
	fprintf( outfile, "};\n" );
	fclose( outfile );

	SHOW_TIME("image writing",started);
	destroy_xpm_charmap( &xpm_cmap, True );
	destroy_colormap( &cmap, True );
	destroy_colorindex( index, False );

	SHOW_TIME("total",started);
	return False;
}

#else  			/* XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM */

Bool
ASImage2xpm ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	SHOW_UNSUPPORTED_NOTE("XPM",path);
	return False ;
}

#endif 			/* XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM */
/***********************************************************************************/

/***********************************************************************************/
#ifdef HAVE_PNG		/* PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG */
Bool
ASImage2png ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	FILE *outfile;
	png_structp png_ptr  = NULL;
	png_infop   info_ptr = NULL;
	Bool has_alpha ;
	Bool greyscale = (depth == 1) ;
	png_byte *row_pointer;
	ASScanline imbuf ;
	int y ;
	START_TIME(started);


	if ((outfile = open_writeable_image_file( path )) == NULL)
		return False;

	png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
    if ( png_ptr != NULL )
    	if( (info_ptr = png_create_info_struct(png_ptr)) != NULL )
			if( setjmp(png_ptr->jmpbuf) )
			{
				png_destroy_info_struct(png_ptr, (png_infopp) &info_ptr);
				info_ptr = NULL ;
    		}

	if( !info_ptr)
	{
		if( png_ptr )
    		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		fclose( outfile );
    	return False;
    }
	png_init_io(png_ptr, outfile);

	if( compression > 0 )
		png_set_compression_level(png_ptr,MIN(compression,99)/10);

	/* lets see if we have alpha channel indeed : */
	has_alpha = False ;
	for( y = 0 ; y < im->height ; y++ )
		if( im->alpha[y] != NULL )
		{
			has_alpha = True ;
			break;
		}

	png_set_IHDR(png_ptr, info_ptr, im->width, im->height, 8,
		         greyscale ? (has_alpha?PNG_COLOR_TYPE_GRAY_ALPHA:PNG_COLOR_TYPE_GRAY):
		                     (has_alpha?PNG_COLOR_TYPE_RGB_ALPHA:PNG_COLOR_TYPE_RGB),
				 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				 PNG_FILTER_TYPE_DEFAULT );
	/* PNG treats alpha s alevel of opacity,
	 * and so do we - there is no need to reverse it : */
	/*	png_set_invert_alpha(png_ptr); */

	/* starting writing the file : writing info first */
	png_write_info(png_ptr, info_ptr);

	prepare_scanline( im->width, 0, &imbuf, False );
	if( greyscale )
	{
		row_pointer = safemalloc( im->width*(has_alpha?2:1));
		for ( y = 0 ; y < im->height ; y++ )
		{
			register int i = im->width;
			CARD8   *ptr = (CARD8*)row_pointer;
			asimage_decode_line (im, IC_RED,   imbuf.red, y, 0, imbuf.width);
			asimage_decode_line (im, IC_GREEN, imbuf.green, y, 0, imbuf.width);
			asimage_decode_line (im, IC_BLUE,  imbuf.blue, y, 0, imbuf.width);
			if( has_alpha )
			{
				asimage_decode_line (im, IC_ALPHA,  imbuf.alpha, y, 0, imbuf.width);

				while( --i >= 0 ) /* normalized greylevel computing :  */
				{
					ptr[(i<<1)] = (54*imbuf.red[i]+183*imbuf.green[i]+19*imbuf.blue[i])/256 ;
					ptr[(i<<1)+1] = imbuf.alpha[i] ;
				}
			}else
				while( --i >= 0 ) /* normalized greylevel computing :  */
					ptr[i] = (54*imbuf.red[i]+183*imbuf.green[i]+19*imbuf.blue[i])/256 ;
			png_write_rows(png_ptr, &row_pointer, 1);
		}
	}else
	{
		row_pointer = safecalloc( im->width * (has_alpha?4:3), 1 );
		for (y = 0; y < im->height; y++)
		{
			register int i = im->width;
			CARD8   *ptr = (CARD8*)(row_pointer+(i-1)*(has_alpha?4:3)) ;
			asimage_decode_line (im, IC_RED,   imbuf.red, y, 0, imbuf.width);
			asimage_decode_line (im, IC_GREEN, imbuf.green, y, 0, imbuf.width);
			asimage_decode_line (im, IC_BLUE,  imbuf.blue, y, 0, imbuf.width);
			if( has_alpha )
			{
				asimage_decode_line (im, IC_ALPHA,  imbuf.alpha, y, 0, imbuf.width);
				while( --i >= 0 )
				{
					/* 0 is red, 1 is green, 2 is blue, 3 is alpha */
		            ptr[0] = imbuf.red[i] ;
					ptr[1] = imbuf.green[i] ;
					ptr[2] = imbuf.blue[i] ;
					ptr[3] = imbuf.alpha[i] ;
					ptr-=4;
				}
			}else
				while( --i >= 0 )
				{
					ptr[0] = imbuf.red[i] ;
					ptr[1] = imbuf.green[i] ;
					ptr[2] = imbuf.blue[i] ;
					ptr-=3;
				}
			png_write_rows(png_ptr, &row_pointer, 1);
		}
	}

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	free( row_pointer );
	free_scanline(&imbuf, True);
	fclose(outfile);

	SHOW_TIME("image writing", started);
	return False ;
}
#else 			/* PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG */
Bool
ASImage2png ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	SHOW_UNSUPPORTED_NOTE( "PNG", path );
	return False;
}

#endif 			/* PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG */
/***********************************************************************************/


/***********************************************************************************/
#ifdef HAVE_JPEG     /* JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG */
Bool
ASImage2jpeg( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	ASScanline     imbuf;
	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	/* More stuff */
	FILE 		 *outfile;		/* target file */
    JSAMPROW      row_pointer[1];/* pointer to JSAMPLE row[s] */
	int 		  y;
	START_TIME(started);

	if ((outfile = open_writeable_image_file( path )) == NULL)
		return False;

	prepare_scanline( im->width, 0, &imbuf, False );
	/* Step 1: allocate and initialize JPEG compression object */
	/* We have to set up the error handler first, in case the initialization
	* step fails.  (Unlikely, but it could happen if you are out of memory.)
	* This routine fills in the contents of struct jerr, and returns jerr's
	* address which we place into the link field in cinfo.
	*/
	cinfo.err = jpeg_std_error(&jerr);
	/* Now we can initialize the JPEG compression object. */
	jpeg_create_compress(&cinfo);

	/* Step 2: specify data destination (eg, a file) */
	/* Note: steps 2 and 3 can be done in either order. */
	/* Here we use the library-supplied code to send compressed data to a
	* stdio stream.  You can also write your own code to do something else.
	* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	* requires it in order to write binary files.
	*/
	jpeg_stdio_dest(&cinfo, outfile);

	/* Step 3: set parameters for compression */
	cinfo.image_width  = im->width; 	/* image width and height, in pixels */
	cinfo.image_height = im->height;
	cinfo.input_components = (depth==1)?1:3;		    /* # of color components per pixel */
	cinfo.in_color_space   = (depth==1)?JCS_GRAYSCALE:JCS_RGB; 	/* colorspace of input image */
	/* Now use the library's routine to set default compression parameters.
	* (You must set at least cinfo.in_color_space before calling this)*/
	jpeg_set_defaults(&cinfo);
	if( quality > 0 )
		jpeg_set_quality(&cinfo, MIN(quality,100), TRUE /* limit to baseline-JPEG values */);

	/* Step 4: Start compressor */
	/* TRUE ensures that we will write a complete interchange-JPEG file.*/
	jpeg_start_compress(&cinfo, TRUE);

	/* Step 5: while (scan lines remain to be written) */
	/*           jpeg_write_scanlines(...); */

	/* Here we use the library's state variable cinfo.next_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	* To keep things simple, we pass one scanline per call; you can pass
	* more if you wish, though.
	*/
	if( depth == 1 )
	{
		row_pointer[0] = safemalloc( im->width );
		for (y = 0; y < im->height; y++)
		{
			register int i = im->width;
			CARD8   *ptr = (CARD8*)row_pointer[0];
			asimage_decode_line (im, IC_RED,   imbuf.red, y, 0, imbuf.width);
			asimage_decode_line (im, IC_GREEN, imbuf.green, y, 0, imbuf.width);
			asimage_decode_line (im, IC_BLUE,  imbuf.blue, y, 0, imbuf.width);
			while( --i >= 0 ) /* normalized greylevel computing :  */
				ptr[i] = (54*imbuf.red[i]+183*imbuf.green[i]+19*imbuf.blue[i])/256 ;
			(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}
	}else
	{
		row_pointer[0] = safemalloc( im->width * 3 );
		for (y = 0; y < im->height; y++)
		{
			register int i = im->width;
			CARD8   *ptr = (CARD8*)(row_pointer[0]+(i-1)*3) ;
LOCAL_DEBUG_OUT( "decoding  row %d", y );
			asimage_decode_line (im, IC_RED,   imbuf.red, y, 0, imbuf.width);
			asimage_decode_line (im, IC_GREEN, imbuf.green, y, 0, imbuf.width);
			asimage_decode_line (im, IC_BLUE,  imbuf.blue, y, 0, imbuf.width);
LOCAL_DEBUG_OUT( "building  row %d", y );
			while( --i >= 0 )
			{
				ptr[0] = imbuf.red[i] ;
				ptr[1] = imbuf.green[i] ;
				ptr[2] = imbuf.blue[i] ;
				ptr-=3;
			}
LOCAL_DEBUG_OUT( "writing  row %d", y );
			(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}
	}
LOCAL_DEBUG_OUT( "done writing image%s","" );
/*	free(buffer); */

	/* Step 6: Finish compression and release JPEG compression object*/
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	free_scanline(&imbuf, True);
	fclose(outfile);

	SHOW_TIME("image export",started);
	LOCAL_DEBUG_OUT("done writing JPEG image \"%s\"", path);
	return False ;
}
#else 			/* JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG */

Bool
ASImage2jpeg( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	SHOW_UNSUPPORTED_NOTE( "JPEG", path );
	return False;
}

#endif 			/* JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG */
/***********************************************************************************/

/***********************************************************************************/
/* XCF - GIMP's native file format : 											   */

Bool
ASImage2xcf ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	/* More stuff */
	XcfImage  *xcf_im = NULL;
	START_TIME(started);

	SHOW_PENDING_IMPLEMENTATION_NOTE("XCF");
	if( xcf_im == NULL )
		return False;

#ifdef LOCAL_DEBUG
	print_xcf_image( xcf_im );
#endif
	/* Make a one-row-high sample array that will go away when done with image */
	SHOW_TIME("write initialization",started);

	free_xcf_image(xcf_im);
	SHOW_TIME("image export",started);
	return True ;
}

/***********************************************************************************/
/* PPM/PNM file format : 											   				   */
Bool
ASImage2ppm ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	START_TIME(started);
	SHOW_PENDING_IMPLEMENTATION_NOTE("PPM");
	SHOW_TIME("image export",started);
	return False;
}

/***********************************************************************************/
/* Windows BMP file format :   									   				   */
Bool
ASImage2bmp ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	START_TIME(started);
	SHOW_PENDING_IMPLEMENTATION_NOTE("BMP");
	SHOW_TIME("image export",started);
	return False;
}

/***********************************************************************************/
/* Windows ICO/CUR file format :   									   			   */
Bool
ASImage2ico ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	START_TIME(started);
	SHOW_PENDING_IMPLEMENTATION_NOTE("ICO");
	SHOW_TIME("image export",started);
	return False;
}

/***********************************************************************************/
#ifdef HAVE_GIF		/* GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF */
Bool ASImage2gif( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	START_TIME(started);
	show_error( "I'm sorry but GIF image export is disabled due to stupid licensing issues. Blame UNISYS");
    SHOW_TIME("image export",started);
	return False ;
}
#else 			/* GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF */
Bool
ASImage2gif( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	SHOW_UNSUPPORTED_NOTE("GIF",path);
	return False ;
}
#endif			/* GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF */

#ifdef HAVE_TIFF/* TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF */
Bool
ASImage2tiff( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	START_TIME(started);
	SHOW_PENDING_IMPLEMENTATION_NOTE("ICO");
	SHOW_TIME("image export",started);
	return False;
}
#else 			/* TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF */

Bool
ASImage2tiff( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	SHOW_UNSUPPORTED_NOTE("TIFF",path);
	return False ;
}
#endif			/* TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF */
