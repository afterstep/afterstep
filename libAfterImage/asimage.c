/*
 * Copyright (c) 2000,2001 Sasha Vasko <sashav@sprintmail.com>
 *
 * This module is free software; you can redistribute it and/or modify
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

#include "../configure.h"

/*#define LOCAL_DEBUG*/
/*#define DO_CLOCKING*/

#define HAVE_MMX
#define USE_64BIT_FPU

#include <malloc.h>
#include "../include/aftersteplib.h"
#include <X11/Intrinsic.h>

#include "../include/asvisual.h"
#include "../include/asimage.h"


void encode_image_scanline_xim( ASImageOutput *imout, ASScanline *to_store );
void encode_image_scanline_asim( ASImageOutput *imout, ASScanline *to_store );

void output_image_line_top( ASImageOutput *, ASScanline *, int );
void output_image_line_fine( ASImageOutput *, ASScanline *, int );
void output_image_line_fast( ASImageOutput *, ASScanline *, int );
void output_image_line_direct( ASImageOutput *, ASScanline *, int );


/**********************************************************************
 * quality control: we support several levels of quality to allow for
 * smooth work on older computers.
 **********************************************************************/
static int asimage_quality_level = ASIMAGE_QUALITY_GOOD;
#ifdef HAVE_MMX
static Bool asimage_use_mmx = True;
#else
static Bool asimage_use_mmx = False;
#endif
/**********************************************************************/
/* initialization routines 											  */
/**********************************************************************/
/*************************** MMX **************************************/
/*inline extern*/
int mmx_init(void)
{
int mmx_available;
#ifdef HAVE_MMX
	asm volatile (
                      /* Get CPU version information */
                      "pushl %%eax\n\t"
                      "pushl %%ebx\n\t"
                      "pushl %%ecx\n\t"
                      "pushl %%edx\n\t"
                      "movl $1, %%eax\n\t"
                      "cpuid\n\t"
                      "andl $0x800000, %%edx \n\t"
					  "movl %%edx, %0\n\t"
                      "popl %%edx\n\t"
                      "popl %%ecx\n\t"
                      "popl %%ebx\n\t"
                      "popl %%eax\n\t"
                      : "=m" (mmx_available)
                      : /* no input */
					  : "ebx", "ecx", "edx"
			  );
#endif
	return mmx_available;
}

int mmx_off(void)
{
#ifdef HAVE_MMX
	__asm__ __volatile__ (
                      /* exit mmx state : */
                      "emms \n\t"
                      : /* no output */
                      : /* no input  */
			  );
#endif
	return 0;
}

/**********************   ASImage  ************************************/
void
asimage_init (ASImage * im, Bool free_resources)
{
	if (im != NULL)
	{
		if (free_resources)
		{
			register int i ;
			for( i = im->height*4-1 ; i>= 0 ; --i )
				if( im->red[i] )
					free( im->red[i] );
			free(im->red);
			if (im->buffer)
				free (im->buffer);
			if( im->ximage )
				XDestroyImage( im->ximage );
		}
		memset (im, 0x00, sizeof (ASImage));
	}
}

void
asimage_start (ASImage * im, unsigned int width, unsigned int height, unsigned int compression)
{
	if (im)
	{
		register int i;
		CARD32 *tmp ;

		asimage_init (im, True);
		im->width = width;
		im->buf_len = width + width;
		/* we want result to be 32bit aligned and padded */
		tmp = safemalloc (sizeof(CARD32)*((im->buf_len>>2)+1));
		im->buffer = (CARD8*)tmp;

		im->height = height;

		im->red = safemalloc (sizeof (CARD8*) * height * 4);
		for( i = 0 ; i < height<<2 ; i++ )
			im->red[i] = 0 ;
		im->green = im->red+height;
		im->blue = 	im->red+(height*2);
		im->alpha = im->red+(height*3);
		im->channels[IC_RED] = im->red ;
		im->channels[IC_GREEN] = im->green ;
		im->channels[IC_BLUE] = im->blue ;
		im->channels[IC_ALPHA] = im->alpha ;

		im->max_compressed_width = width*compression/100;
	}
}

/********************** ASScanline ************************************/
ASScanline*
prepare_scanline( unsigned int width, unsigned int shift, ASScanline *reusable_memory, Bool BGR_mode  )
{
	register ASScanline *sl = reusable_memory ;
	size_t aligned_width;
	void *ptr;

	if( sl == NULL )
		sl = safecalloc( 1, sizeof( ASScanline ) );
	else
		memset( sl, 0x00, sizeof(ASScanline));

	sl->width 	= width ;
	sl->shift   = shift ;
	/* we want to align data by 8 byte boundary (double)
	 * to allow for code with less ifs and easier MMX/3Dnow utilization :*/
	aligned_width = width + (width&0x00000001);
	sl->buffer = ptr = safemalloc (((aligned_width*4)+4)*sizeof(CARD32));

	sl->xc1 = sl->red 	= (CARD32*)(((long)ptr>>3)*8);
	sl->xc2 = sl->green = sl->red   + aligned_width;
	sl->xc3 = sl->blue 	= sl->green + aligned_width;
	sl->alpha 	= sl->blue  + aligned_width;

	sl->channels[IC_RED] = sl->red ;
	sl->channels[IC_GREEN] = sl->green ;
	sl->channels[IC_BLUE] = sl->blue ;
	sl->channels[IC_ALPHA] = sl->alpha ;

	if( BGR_mode )
	{
		sl->xc1 = sl->blue ;
		sl->xc3 = sl->red ;
	}
	/* this way we can be sure that our buffers have size of multiplies of 8s
	 * and thus we can skip unneeded checks in code */
	/* initializing padding into 0 to avoid any garbadge carry-over
	 * bugs with diffusion: */
	sl->red[aligned_width-1]   = 0;
	sl->green[aligned_width-1] = 0;
	sl->blue[aligned_width-1]  = 0;
	sl->alpha[aligned_width-1] = 0;

	sl->back_color = ARGB32_DEFAULT_BACK_COLOR;

	return sl;
}

void
free_scanline( ASScanline *sl, Bool reusable )
{
	if( sl )
	{
		if( sl->buffer )
			free( sl->buffer );
		if( !reusable )
			free( sl );
	}
}

/********************* ASImageDecoder ****************************/
ASImageDecoder *
start_image_decoding( ASVisual *asv,ASImage *im, ASFlagType filter,
					  int offset_x, int offset_y, unsigned int out_width )
{
	ASImageDecoder *imdec = NULL;
	if( im == NULL || filter == 0 || asv == NULL )
		return NULL;

	if( offset_x < 0 )
		offset_x = im->width - (offset_x%im->width);
	else
		offset_x %= im->width ;
	if( offset_y < 0 )
		offset_y = im->height - (offset_y%im->height);
	else
		offset_y %= im->height ;
	if( out_width == 0 )
		out_width = im->width ;

	imdec = safecalloc( 1, sizeof(ASImageDecoder));
	imdec->im = im ;
	imdec->filter = filter ;
	imdec->offset_x = offset_x ;
	imdec->out_width = out_width;
	imdec->offset_y = offset_y ;
	imdec->next_line = offset_y ;
	imdec->back_color = ARGB32_DEFAULT_BACK_COLOR ;
	prepare_scanline(out_width, 0, &(imdec->buffer), asv->BGR_mode );

	return imdec;
}

void
stop_image_decoding( ASImageDecoder **pimdec )
{
	if( pimdec )
		if( *pimdec )
		{
			free_scanline( &((*pimdec)->buffer), True );
			free( *pimdec );
			*pimdec = NULL;
		}
}

/********************* ASImageOutput ****************************/
ASImageOutput *
start_image_output( ASVisual *asv, ASImage *im, XImage *xim, Bool to_xim, int shift, int quality )
{
	register ASImageOutput *imout= NULL;

	if( im == NULL || asv == NULL)
		return imout;
	if( xim == NULL )
		xim = im->ximage ;
	if( to_xim && xim == NULL )
		return imout;
	imout = safecalloc( 1, sizeof(ASImageOutput));
	imout->asv = asv;
	imout->im = im ;
	imout->to_xim = to_xim ;
	imout->encode_image_scanline = (to_xim)?encode_image_scanline_xim:
										   encode_image_scanline_asim;

	imout->xim = xim ;
	if( xim )
	{
		imout->height = xim->height;
		imout->bpl 	  = xim->bytes_per_line;
		imout->xim_line = xim->data;
	}else
		imout->height = im->height;

	prepare_scanline( im->width, 0, &(imout->buffer[0]), asv->BGR_mode);
	prepare_scanline( im->width, 0, &(imout->buffer[1]), asv->BGR_mode);

	imout->chan_fill[IC_RED]   = ARGB32_RED8(ARGB32_DEFAULT_BACK_COLOR);
	imout->chan_fill[IC_GREEN] = ARGB32_GREEN8(ARGB32_DEFAULT_BACK_COLOR);
	imout->chan_fill[IC_BLUE]  = ARGB32_BLUE8(ARGB32_DEFAULT_BACK_COLOR);
	imout->chan_fill[IC_ALPHA] = ARGB32_ALPHA8(ARGB32_DEFAULT_BACK_COLOR);

	imout->available = &(imout->buffer[0]);
	imout->used 	 = NULL;
	imout->buffer_shift = shift;
	imout->next_line = 0 ;
	if( quality > ASIMAGE_QUALITY_TOP || quality < ASIMAGE_QUALITY_POOR )
		quality = asimage_quality_level;

	imout->quality = quality ;
	if( shift > 0 )
	{/* choose what kind of error diffusion we'll use : */
		switch( quality )
		{
			case ASIMAGE_QUALITY_POOR :
			case ASIMAGE_QUALITY_FAST :
				imout->output_image_scanline = output_image_line_fast ;
				break;
			case ASIMAGE_QUALITY_GOOD :
				imout->output_image_scanline = output_image_line_fine ;
				break;
			case ASIMAGE_QUALITY_TOP  :
				imout->output_image_scanline = output_image_line_top ;
				break;
		}
	}else /* no quanitzation - no error diffusion */
		imout->output_image_scanline = output_image_line_direct ;

	return imout;
}

void set_image_output_back_color( ASImageOutput *imout, ARGB32 back_color )
{
	if( imout )
	{
		imout->chan_fill[IC_RED]   = ARGB32_RED8  (back_color);
		imout->chan_fill[IC_GREEN] = ARGB32_GREEN8(back_color);
		imout->chan_fill[IC_BLUE]  = ARGB32_BLUE8 (back_color);
		imout->chan_fill[IC_ALPHA] = ARGB32_ALPHA8(back_color);
	}
}

void toggle_image_output_direction( ASImageOutput *imout )
{
	if( imout )
	{
		if( imout->bottom_to_top )
		{
			if( imout->next_line >= imout->height-1 )
			{
				imout->next_line = 0 ;
				if( imout->to_xim )
					imout->xim_line = imout->xim->data;
			}
		}else if( imout->next_line <= 0 )
		{
		 	imout->next_line = imout->height-1 ;
			if( imout->to_xim )
				imout->xim_line = imout->xim->data+(imout->next_line*imout->bpl);
		}
		imout->bottom_to_top = !imout->bottom_to_top ;
	}
}


void
stop_image_output( ASImageOutput **pimout )
{
	if( pimout )
	{
		register ASImageOutput *imout = *pimout;
		if( imout )
		{
			if( imout->used )
				imout->output_image_scanline( imout, NULL, 1);
			free_scanline(&(imout->buffer[0]), True);
			free_scanline(&(imout->buffer[1]), True);
			free( imout );
			*pimout = NULL;
		}
	}
}

/***********************************************************************/
/*  Compression/decompression 										   */
/***********************************************************************/
static void
asimage_apply_buffer (ASImage * im, ColorPart color, unsigned int y)
{
	size_t   len = (im->buf_used>>2)+1 ;
	CARD8  **part = im->channels[color];
	register CARD32  *dwdst = safemalloc( sizeof(CARD32)*len);
	register CARD32  *dwsrc = (CARD32*)(im->buffer);
	register int i ;
	for( i = 0 ; i < len ; ++i )
		dwdst[i] = dwsrc[i];

	if (part[y] != NULL)
		free (part[y]);
	part[y] = (CARD8*)dwdst;
}

static void
asimage_dup_line (ASImage * im, ColorPart color, unsigned int y1, unsigned int y2, unsigned int length)
{
	CARD8       **part = im->channels[color];
	if (part[y2] != NULL)
		free (part[y2]);
	if( part[y1] )
	{
		register int i ;
		register CARD32 *dwsrc = (CARD32*)part[y1];
		register CARD32 *dwdst ;
		length = (length>>2)+1;
		dwdst = safemalloc (sizeof(CARD32)*length);
		/* 32bit copy gives us about 15% performance gain */
		for( i = 0 ; i < length ; ++i )
			dwdst[i] = dwsrc[i];
		part[y2] = (CARD8*)dwdst;
	}else
		part[y2] = NULL;
}

void
asimage_erase_line( ASImage * im, ColorPart color, unsigned int y )
{
	if( im )
	{
		if( color < IC_NUM_CHANNELS )
		{
			CARD8       **part = im->channels[color];
			if( part[y] )
			{
				free( part[y] );
				part[y] = NULL;
			}
		}else
		{
			int c ;
			for( c = 0 ; c < IC_NUM_CHANNELS ; c++ )
			{
				CARD8       **part = im->channels[color];
				if( part[y] )
				{
					free( part[y] );
					part[y] = NULL;
				}
			}
		}
	}
}


size_t
asimage_add_line_mono (ASImage * im, ColorPart color, register CARD8 value, unsigned int y)
{
	register CARD8 *dst;
	int rep_count;

	if (im == NULL || color <0 || color >= IC_NUM_CHANNELS )
		return 0;
	if (im->buffer == NULL || y >= im->height)
		return 0;

	dst = im->buffer;
	rep_count = im->width - RLE_THRESHOLD;
	if (rep_count <= RLE_MAX_SIMPLE_LEN)
	{
		dst[0] = (CARD8) rep_count;
		dst[1] = (CARD8) value;
		dst[2] = 0 ;
		im->buf_used = 3;
	} else
	{
		dst[0] = (CARD8) ((rep_count >> 8) & RLE_LONG_D)|RLE_LONG_B;
		dst[1] = (CARD8) ((rep_count) & 0x00FF);
		dst[2] = (CARD8) value;
		dst[3] = 0 ;
		im->buf_used = 4;
	}
	asimage_apply_buffer (im, color, y);
	return im->buf_used;
}

size_t
asimage_add_line (ASImage * im, ColorPart color, register CARD32 * data, unsigned int y)
{
	int             i = 0, bstart = 0, ccolor = 0;
	unsigned int    width;
	register CARD8 *dst;
	register int 	tail = 0;
	int best_size, best_bstart = 0, best_tail = 0;

	if (im == NULL || data == NULL || color <0 || color >= IC_NUM_CHANNELS )
		return 0;
	if (im->buffer == NULL || y >= im->height)
		return 0;

	best_size = 0 ;
	dst = im->buffer;
/*	fprintf( stderr, "%d:%d:%d<%2.2X ", y, color, 0, data[0] ); */

	if( im->width == 1 )
	{
		dst[0] = RLE_DIRECT_TAIL ;
		dst[1] = data[0] ;
		im->buf_used = 2;
	}else
	{
/*		width = im->width; */
		width = im->max_compressed_width ;
		while( i < width )
		{
			while( i < width && data[i] == data[ccolor])
			{
/*				fprintf( stderr, "%d<%2.2X ", i, data[i] ); */
				++i ;
			}
			if( i > ccolor + RLE_THRESHOLD )
			{ /* we have to write repetition count and length */
				register unsigned int rep_count = i - ccolor - RLE_THRESHOLD;

				if (rep_count <= RLE_MAX_SIMPLE_LEN)
				{
					dst[tail] = (CARD8) rep_count;
					dst[++tail] = (CARD8) data[ccolor];
/*					fprintf( stderr, "\n%d:%d: >%d: %2.2X %d: %2.2X ", y, color, tail-1, dst[tail-1], tail, dst[tail] ); */
				} else
				{
					dst[tail] = (CARD8) ((rep_count >> 8) & RLE_LONG_D)|RLE_LONG_B;
					dst[++tail] = (CARD8) ((rep_count) & 0x00FF);
					dst[++tail] = (CARD8) data[ccolor];
/*					fprintf( stderr, "\n%d:%d: >%d: %2.2X %d: %2.2X %d: %2.2X ", y, color, tail-2, dst[tail-2], tail-1, dst[tail-1], tail, dst[tail] ); */
				}
				++tail ;
				bstart = ccolor = i;
			}
/*			fprintf( stderr, "\n"); */
			while( i < width )
			{
/*				fprintf( stderr, "%d<%2.2X ", i, data[i] ); */
				if( data[i] != data[ccolor] )
					ccolor = i ;
				else if( i-ccolor > RLE_THRESHOLD )
					break;
				++i ;
			}
			if( i == width )
				ccolor = i ;
			while( ccolor > bstart )
			{/* we have to write direct block */
				int dist = ccolor-bstart ;

				if( tail-bstart < best_size )
				{
					best_tail = tail ;
					best_bstart = bstart ;
					best_size = tail-bstart ;
				}
				if( dist > RLE_MAX_DIRECT_LEN )
					dist = RLE_MAX_DIRECT_LEN ;
				dst[tail] = RLE_DIRECT_B | ((CARD8)(dist-1));
/*				fprintf( stderr, "\n%d:%d: >%d: %2.2X ", y, color, tail, dst[tail] ); */
				dist += bstart ;
				++tail ;
				while ( bstart < dist )
				{
					dst[tail] = (CARD8) data[bstart];
/*					fprintf( stderr, "%d: %2.2X ", tail, dst[tail]); */
					++tail ;
					++bstart;
				}
			}
/*			fprintf( stderr, "\n"); */
		}
		if( best_size+im->width < tail )
		{
			width = im->width;
			LOCAL_DEBUG_OUT( " %d:%d >resetting bytes starting with offset %d(%d) (0x%2.2X) to DIRECT_TAIL( %d bytes total )", y, color, best_tail, best_bstart, dst[best_tail], width-best_bstart );
			dst[best_tail] = RLE_DIRECT_TAIL;
			dst += best_tail+1;
			data += best_bstart;
			for( i = width-best_bstart-1 ; i >=0 ; --i )
				dst[i] = data[i] ;
			im->buf_used = best_tail+1+width-best_bstart ;
		}else
		{
			if( i < im->width )
			{
				dst[tail] = RLE_DIRECT_TAIL;
				dst += tail+1 ;
				data += i;
				im->buf_used = tail+1+im->width-i ;
				for( i = im->width-(i+1) ; i >=0 ; --i )
					dst[i] = data[i] ;
			}else
			{
				dst[tail] = RLE_EOL;
				im->buf_used = tail+1;
			}
		}
	}
	asimage_apply_buffer (im, color, y);
	return im->buf_used;
}

ASFlagType
get_asimage_chanmask( ASImage *im)
{
    ASFlagType mask = 0 ;
	int color ;

	if( im )
		for( color = 0; color < IC_NUM_CHANNELS ; color++ )
		{
			register CARD8 **chan = im->channels[color];
			register int y, height = im->height ;
			for( y = 0 ; y < height ; y++ )
				if( chan[y] )
				{
					set_flags( mask, 0x01<<color );
					break;
				}
		}
    return mask ;
}


unsigned int
asimage_print_line (ASImage * im, ColorPart color, unsigned int y, unsigned long verbosity)
{
	CARD8       **color_ptr;
	register CARD8 *ptr;
	int           to_skip = 0;
	int 		  uncopressed_size = 0 ;

	if (im == NULL || color < 0 || color >= IC_NUM_CHANNELS )
		return 0;
	if (y >= im->height)
		return 0;

	if((color_ptr = im->channels[color]) == NULL )
		return 0;
	ptr = color_ptr[y];
	if( ptr == NULL )
	{
		if(  verbosity != 0 )
			show_error( "no data available for line %d", y );
		return 0;
	}
	while (*ptr != RLE_EOL)
	{
		while (to_skip-- > 0)
		{
			if (get_flags (verbosity, VRB_LINE_CONTENT))
				fprintf (stderr, " %2.2X", *ptr);
			ptr++;
		}

		if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
			fprintf (stderr, "\nControl byte encountered : %2.2X", *ptr);

		if (((*ptr) & RLE_DIRECT_B) != 0)
		{
			if( *ptr == RLE_DIRECT_TAIL )
			{
				if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
					fprintf (stderr, " is RLE_DIRECT_TAIL ( %d bytes ) !", im->width-uncopressed_size);
				ptr += im->width-uncopressed_size;
				break;
			}
			to_skip = 1 + ((*ptr) & (RLE_DIRECT_D));
			uncopressed_size += to_skip;
			if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
				fprintf (stderr, " is RLE_DIRECT !");
		} else if (((*ptr) & RLE_SIMPLE_B_INV) == 0)
		{
			if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
				fprintf (stderr, " is RLE_SIMPLE !");
			uncopressed_size += ((int)ptr[0])+ RLE_THRESHOLD;
			to_skip = 1;
		} else if (((*ptr) & RLE_LONG_B) != 0)
		{
			if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
				fprintf (stderr, " is RLE_LONG !");
			uncopressed_size += ((int)ptr[1])+((((int)ptr[0])&RLE_LONG_D ) << 8)+RLE_THRESHOLD;
			to_skip = 1 + 1;
		}
		to_skip++;
		if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
			fprintf (stderr, " to_skip = %d, uncompressed size = %d\n", to_skip, uncopressed_size);
	}
	if (get_flags (verbosity, VRB_LINE_CONTENT))
		fprintf (stderr, " %2.2X\n", *ptr);

	ptr++;

	if (get_flags (verbosity, VRB_LINE_SUMMARY))
		fprintf (stderr, "Row %d, Component %d, Memory Used %d\n", y, color, ptr - color_ptr[y]);
	return ptr - color_ptr[y];
}

inline static CARD32*
asimage_decode_block32 (register CARD8 *src, CARD32 *to_buf, unsigned int width )
{
	register CARD32 *dst = to_buf;
	while ( *src != RLE_EOL)
	{
		if( src[0] == RLE_DIRECT_TAIL )
		{
			register int i = width - (dst-to_buf) ;
			dst += i ;
			src += i+1 ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[i] ;
				++i ;
			}
			break;
		}else if( ((*src)&RLE_DIRECT_B) != 0 )
		{
			register int i = (((int)src[0])&RLE_DIRECT_D) + 1;
			dst += i ;
			src += i+1 ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[i] ;
				++i ;
			}
		}else if( ((*src)&RLE_LONG_B) != 0 )
		{
			register int i = ((((int)src[0])&RLE_LONG_D)<<8|src[1]) + RLE_THRESHOLD;
			dst += i ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[2] ;
				++i ;
			}
			src += 3;
		}else
		{
			register int i = (int)src[0] + RLE_THRESHOLD ;
			dst += i ;
			i = -i;
			while( i <= 0 )
			{
				dst[i] = src[1] ;
				++i ;
			}
			src += 2;
		}
	}
	return dst;
}

inline static CARD8*
asimage_decode_block8 (register CARD8 *src, CARD8 *to_buf, unsigned int width )
{
	register CARD8 *dst = to_buf;
	while ( *src != RLE_EOL)
	{
		if( src[0] == RLE_DIRECT_TAIL )
		{
			register int i = width - (dst-to_buf) ;
			dst += i ;
			src += i+1 ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[i] ;
				++i ;
			}
			break;
		}else if( ((*src)&RLE_DIRECT_B) != 0 )
		{
			register int i = (((int)src[0])&RLE_DIRECT_D) + 1;
			dst += i ;
			src += i+1 ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[i] ;
				++i ;
			}
		}else if( ((*src)&RLE_LONG_B) != 0 )
		{
			register int i = ((((int)src[0])&RLE_LONG_D)<<8|src[1]) + RLE_THRESHOLD;
			dst += i ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[2] ;
				++i ;
			}
			src += 3;
		}else
		{
			register int i = (int)src[0] + RLE_THRESHOLD ;
			dst += i ;
			i = -i;
			while( i <= 0 )
			{
				dst[i] = src[1] ;
				++i ;
			}
			src += 2;
		}
	}
	return dst;
}

void print_component( register CARD32 *data, int nonsense, int len );

inline int
asimage_decode_line (ASImage * im, ColorPart color, CARD32 * to_buf, unsigned int y, unsigned int skip, unsigned int out_width)
{
	register CARD8  *src = im->channels[color][y];
	/* that thing below is supposedly highly optimized : */
LOCAL_DEBUG_CALLER_OUT( "im->width = %d, color = %d, y = %d, skip = %d, out_width = %d, src = %p", im->width, color, y, skip, out_width, src );
	if( src )
	{
		register int i = 0;
#if 1
  		if( skip > 0 || out_width+skip < im->width)
		{
			int max_i ;

			asimage_decode_block8( src, im->buffer, im->width );
			skip = skip%im->width ;
			max_i = MIN(out_width,im->width-skip);
			src = im->buffer+skip ;
			while( i < out_width )
			{
				while( i < max_i )
				{
					to_buf[i] = src[i] ;
					++i ;
				}
				src = im->buffer-i ;
				max_i = MIN(out_width,im->width+i) ;
			}
		}else
		{
#endif
			i = asimage_decode_block32( src, to_buf, im->width ) - to_buf;
#if 1
	  		while( i < out_width )
			{   /* tiling code : */
				register CARD32 *src = to_buf-i ;
				int max_i = MIN(out_width,im->width+i);
				while( i < max_i )
				{
					to_buf[i] = src[i] ;
					++i ;
				}
			}
#endif
		}
		return i;
	}
	return 0;
}


unsigned int
asimage_copy_line (CARD8 * from, CARD8 * to, int width)
{
	register CARD8 *src = from, *dst = to;
	int uncompressed_size = 0;

	/* merely copying the data */
	if (src == NULL || dst == NULL)
		return 0;
	while (*src != RLE_EOL)
	{
		if ( *src == RLE_DIRECT_TAIL)
		{
			register int  to_write = width - uncompressed_size;
			while (to_write-- >= 0)			   /* we start counting from 0 - 0 is actually count of 1 */
				dst[to_write] = src[to_write];
			dst += to_write ;
			src += to_write ;
			break;
		} else if (((*src) & RLE_DIRECT_B) != 0)
		{
			register int  to_write = 1+ ((*src) & (RLE_DIRECT_D)) + 1;
			uncompressed_size += to_write-1 ;
			while (to_write-- >= 0)			   /* we start counting from 0 - 0 is actually count of 1 */
				dst[to_write] = src[to_write];
			dst += to_write ;
			src += to_write ;
		} else if (((*src) & RLE_SIMPLE_B_INV) == 0)
		{
			uncompressed_size += src[0]+RLE_THRESHOLD ;
			dst[0] = src[0];
			dst[1] = src[1];
			dst += 2 ;
			src += 2 ;
		} else if (((*src) & RLE_LONG_B) != 0)
		{
			uncompressed_size += src[1] + ((src[0]&RLE_LONG_D)<<8) + RLE_THRESHOLD ;
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst += 3 ;
			src += 3 ;
		}
	}
	return (dst - to);
}

Bool
asimage_compare_line (ASImage *im, ColorPart color, CARD32 *to_buf, CARD32 *tmp, unsigned int y, Bool verbose)
{
	register int i;
	asimage_decode_line( im, color, tmp, y, 0, im->width );
	for( i = 0 ; i < im->width ; i++ )
		if( tmp[i] != to_buf[i] )
		{
			if( verbose )
				show_error( "line %d, component %d differ at offset %d ( 0x%lX(compresed) != 0x%lX(orig) )\n", y, color, i, tmp[i], to_buf[i] );
			return False ;
		}
	return True;
}

/*******************************************************************************/
/* below goes all kinds of funky stuff we can do with scanlines : 			   */
/*******************************************************************************/
/* this will enlarge array based on count of items in dst per PAIR of src item with smoothing/scatter/dither */
/* the following formulas use linear approximation to calculate   */
/* color values for new pixels : 				  				  */
/* note that we shift values by 8 to keep quanitzation error in   */
/* lower 1 byte for subsequent dithering 	:					  */
#define QUANT_ERR_BITS  	8
#define QUANT_ERR_MASK  	0x000000FF
/* for scale factor of 2 we use this formula :    */
/* C = (-C1+3*C2+3*C3-C4)/4 					  */
/* or better :				 					  */
/* C = (-C1+5*C2+5*C3-C4)/8 					  */
#define INTERPOLATE_COLOR1(c) 			   	((c)<<QUANT_ERR_BITS)  /* nothing really to interpolate here */
#define INTERPOLATE_COLOR2(c1,c2,c3,c4)    	((((c2)<<2)+(c2)+((c3)<<2)+(c3)-(c1)-(c4))<<(QUANT_ERR_BITS-3))
#define INTERPOLATE_COLOR2_V(c1,c2,c3,c4)    	((((c2)<<2)+(c2)+((c3)<<2)+(c3)-(c1)-(c4))>>3)
/* for scale factor of 3 we use these formulas :  */
/* Ca = (-2C1+8*C2+5*C3-2C4)/9 		  			  */
/* Cb = (-2C1+5*C2+8*C3-2C4)/9 		  			  */
/* or better : 									  */
/* Ca = (-C1+5*C2+3*C3-C4)/6 		  			  */
/* Cb = (-C1+3*C2+5*C3-C4)/6 		  			  */
#define INTERPOLATE_A_COLOR3(c1,c2,c3,c4)  	(((((c2)<<2)+(c2)+((c3)<<1)+(c3)-(c1)-(c4))<<QUANT_ERR_BITS)/6)
#define INTERPOLATE_B_COLOR3(c1,c2,c3,c4)  	(((((c2)<<1)+(c2)+((c3)<<2)+(c3)-(c1)-(c4))<<QUANT_ERR_BITS)/6)
#define INTERPOLATE_A_COLOR3_V(c1,c2,c3,c4)  	((((c2)<<2)+(c2)+((c3)<<1)+(c3)-(c1)-(c4))/6)
#define INTERPOLATE_B_COLOR3_V(c1,c2,c3,c4)  	((((c2)<<1)+(c2)+((c3)<<2)+(c3)-(c1)-(c4))/6)
/* just a hypotesus, but it looks good for scale factors S > 3: */
/* Cn = (-C1+(2*(S-n)+1)*C2+(2*n+1)*C3-C4)/2S  	  			   */
/* or :
 * Cn = (-C1+(2*S+1)*C2+C3-C4+n*(2*C3-2*C2)/2S  			   */
/*       [ T                   [C2s]  [C3s]]   			       */
#define INTERPOLATION_Cs(c)	 		 	    ((c)<<1)
/*#define INTERPOLATION_TOTAL_START(c1,c2,c3,c4,S) 	(((S)<<1)*(c2)+((c3)<<1)+(c3)-c2-c1-c4)*/
#define INTERPOLATION_TOTAL_START(c1,c2,c3,c4,S) 	((((S)<<1)+1)*(c2)+(c3)-(c1)-(c4))
#define INTERPOLATION_TOTAL_STEP(c2,c3)  	((c3<<1)-(c2<<1))
#define INTERPOLATE_N_COLOR(T,S)		  	(((T)<<(QUANT_ERR_BITS-1))/(S))

#define AVERAGE_COLOR1(c) 					((c)<<QUANT_ERR_BITS)
#define AVERAGE_COLOR2(c1,c2)				(((c1)+(c2))<<(QUANT_ERR_BITS-1))
#define AVERAGE_COLORN(T,N)					(((T)<<QUANT_ERR_BITS)/N)

static inline void
enlarge_component12( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* expected len >= 2  */
	register int i = 0, k = 0;
	register int c1 = src[0], c4;
	--len; --len ;
	while( i < len )
	{
		c4 = src[i+2];
		/* that's right we can do that PRIOR as we calculate nothing */
		dst[k] = INTERPOLATE_COLOR1(src[i]) ;
		if( scales[i] == 2 )
		{
			register int c2 = src[i], c3 = src[i+1] ;
			c3 = INTERPOLATE_COLOR2(c1,c2,c3,c4);
			dst[++k] = (c3&0xFF000000 )?0:c3;
		}
		c1 = src[i];
		++k;
		++i;
	}

	/* to avoid one more if() in loop we moved tail part out of the loop : */
	if( scales[i] == 1 )
		dst[k] = INTERPOLATE_COLOR1(src[i]);
	else
	{
		register int c2 = src[i], c3 = src[i+1] ;
		c2 = INTERPOLATE_COLOR2(c1,c2,c3,c3);
		dst[k] = (c2&0xFF000000 )?0:c2;
	}
	dst[k+1] = INTERPOLATE_COLOR1(src[i+1]);
}

static inline void
enlarge_component23( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* expected len >= 2  */
	register int i = 0, k = 0;
	register int c1 = src[0], c4 = src[1];
	if( scales[0] == 1 )
	{/* special processing for first element - it can be 1 - others can only be 2 or 3 */
		dst[k] = INTERPOLATE_COLOR1(src[0]) ;
		++k;
		++i;
	}
	--len; --len ;
	while( i < len )
	{
		register int c2 = src[i], c3 = src[i+1] ;
		c4 = src[i+2];
		dst[k] = INTERPOLATE_COLOR1(c2) ;
		if( scales[i] == 2 )
		{
			c3 = INTERPOLATE_COLOR2(c1,c2,c3,c3);
			dst[++k] = (c3&0x7F000000 )?0:c3;
		}else
		{
			dst[++k] = INTERPOLATE_A_COLOR3(c1,c2,c3,c4);
			if( dst[k]&0x7F000000 )
				dst[k] = 0 ;
			c3 = INTERPOLATE_B_COLOR3(c1,c2,c3,c3);
			dst[++k] = (c3&0x7F000000 )?0:c3;
		}
		c1 = c2 ;
		++k;
		++i;
	}
	/* to avoid one more if() in loop we moved tail part out of the loop : */
	{
		register int c2 = src[i], c3 = src[i+1] ;
		dst[k] = INTERPOLATE_COLOR1(c2) ;
		if( scales[i] == 2 )
		{
			c2 = INTERPOLATE_COLOR2(c1,c2,c3,c3);
			dst[k+1] = (c2&0x7F000000 )?0:c2;
		}else
		{
			if( scales[i] == 1 )
				--k;
			else
			{
				dst[++k] = INTERPOLATE_A_COLOR3(c1,c2,c3,c3);
				if( dst[k]&0x7F000000 )
					dst[k] = 0 ;
				c2 = INTERPOLATE_B_COLOR3(c1,c2,c3,c3);
				dst[k+1] = (c2&0x7F000000 )?0:c2;
			}
		}
	}
 	dst[k+2] = INTERPOLATE_COLOR1(src[i+1]) ;
}

/* this case is more complex since we cannot really hardcode coefficients
 * visible artifacts on smooth gradient-like images
 */
static inline void
enlarge_component( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* we skip all checks as it is static function and we want to optimize it
  * as much as possible */
	int i = 0;
	int c1 = src[0];
	--len ;
	do
	{
		register int step = INTERPOLATION_TOTAL_STEP(src[i],src[i+1]);
		register int T ;
		register short S = scales[i];

/*		LOCAL_DEBUG_OUT( "pixel %d, S = %d, step = %d", i, S, step );*/
		T = INTERPOLATION_TOTAL_START(c1,src[i],src[i+1],src[i+2],S);
		if( step )
		{
			register int n = 0 ;
			do
			{
				dst[n] = (T&0x7F000000)?0:INTERPOLATE_N_COLOR(T,S);
				if( ++n >= S ) break;
				(int)T += (int)step;
			}while(1);
			dst += n ;
		}else
		{
			register CARD32 c = (T&0x7F000000)?0:INTERPOLATE_N_COLOR(T,S);
			while(--S >= 0){	dst[S] = c;	}
			dst += scales[i] ;
		}
		c1 = src[i];
		if( ++i >= len )
			break;
	}while(1);
	*dst = INTERPOLATE_COLOR1(src[i]) ;
/*LOCAL_DEBUG_OUT( "%d pixels written", k );*/
}

/* this will shrink array based on count of items in src per one dst item with averaging */
static inline void
shrink_component( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* we skip all checks as it is static function and we want to optimize it
  * as much as possible */
	register int i = -1, k = -1;
	while( ++k < len )
	{
		register int reps = scales[k] ;
		register int c1 = src[++i];
/*LOCAL_DEBUG_OUT( "pixel = %d, scale[k] = %d", k, reps );*/
		if( reps == 1 )
			dst[k] = AVERAGE_COLOR1(c1);
		else if( reps == 2 )
		{
			++i;
			dst[k] = AVERAGE_COLOR2(c1,src[i]);
		}else
		{
			reps += i-1;
			while( reps > i )
			{
				++i ;
				c1 += src[i];
			}
			{
				register short S = scales[k];
				dst[k] = AVERAGE_COLORN(c1,S);
			}
		}
	}
}
static inline void
shrink_component11( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		dst[i] = AVERAGE_COLOR1(src[i]);
}


/* for consistency sake : */
static inline void
copy_component( register CARD32 *src, register CARD32 *dst, int *unused, int len )
{
#if 1
#ifdef CARD64
	CARD64 *dsrc = (CARD64*)src;
	CARD64 *ddst = (CARD64*)dst;
#else
	double *dsrc = (double*)src;
	double *ddst = (double*)dst;
#endif
	register int i = 0;

	len += len&0x01;
	len = len>>1 ;
	do
	{
		ddst[i] = dsrc[i];
	}while(++i < len );
#else
	register int i = 0;

	len += len&0x01;
	do
	{
		dst[i] = src[i];
	}while(++i < len );

#endif

}

static inline void
reverse_component( register CARD32 *src, register CARD32 *dst, int *unused, int len )
{
	register int i = 0;
	src += len-1 ;
	do
	{
		dst[i] = src[-i];
	}while(++i < len );
}

static inline void
add_component( CARD32 *src, CARD32 *incr, int *scales, int len )
{
	int i = 0;

	len += len&0x01;
#if 1
#ifdef HAVE_MMX
	if( asimage_use_mmx )
	{
		double *ddst = (double*)&(src[0]);
		double *dinc = (double*)&(incr[0]);
		len = len>>1;
		do{
			asm volatile
       		(
            	"movq %0, %%mm0  \n\t" /* load 8 bytes from src[i] into MM0 */
            	"paddd %1, %%mm0 \n\t" /* MM0=src[i]>>1              */
            	"movq %%mm0, %0  \n\t" /* store the result in dest */
				: "=m" (ddst[i])       /* %0 */
				:  "m"  (dinc[i])       /* %2 */
	        );
		}while( ++i < len );
	}else
#endif
#endif
	{
		register int c1, c2;
		do{
			c1 = (int)src[i] + (int)incr[i] ;
			c2 = (int)src[i+1] + (int)incr[i+1] ;
			src[i] = c1;
			src[i+1] = c2;
			i += 2 ;
		}while( i < len );
	}
}

static inline int
set_component( register CARD32 *src, register CARD32 value, int offset, int len )
{
	register int i ;
	for( i = offset ; i < len ; ++i )
		src[i] = value;
	return len-offset;
}

static inline void
divide_component( register CARD32 *src, register CARD32 *dst, CARD16 ratio, int len )
{
	register int i = 0;
	len += len&0x00000001;                     /* we are 8byte aligned/padded anyways */
	if( ratio == 2 )
	{
#ifdef HAVE_MMX
		if( asimage_use_mmx )
		{
			double *ddst = (double*)&(dst[0]);
			double *dsrc = (double*)&(src[0]);
			len = len>>1;
			do{
				asm volatile
       		    (
            		"movq %1, %%mm0  \n\t" // load 8 bytes from src[i] into MM0
            		"psrld $1, %%mm0 \n\t" // MM0=src[i]>>1
            		"movq %%mm0, %0  \n\t" // store the result in dest
					: "=m" (ddst[i]) // %0
					: "m"  (dsrc[i]) // %1
	            );
			}while( ++i < len );
		}else
#endif
			do{
				dst[i] = src[i] >> 1;
				dst[i+1] = src[i+1]>> 1;
				i += 2 ;
			}while( i < len );
	}else
	{
		do{
			register int c1 = src[i];
			register int c2 = src[i+1];
			dst[i] = c1/ratio;
			dst[i+1] = c2/ratio;
			i+=2;
		}while( i < len );
	}
}

static inline void
rbitshift_component( register CARD32 *src, register CARD32 *dst, int shift, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		dst[i] = src[i]>>shift;
}

/* diffusingly combine src onto self and dst, and rightbitshift src by quantization shift */
static inline void
best_output_filter( register CARD32 *line1, register CARD32 *line2, int unused, int len )
{/* we carry half of the quantization error onto the surrounding pixels : */
 /*        X    7/16 */
 /* 3/16  5/16  1/16 */
	register int i ;
	register CARD32 errp = 0, err = 0, c;
	c = line1[0];
	if( (c&0xFFFF0000)!= 0 )
		c = ( c&0x7F000000 )?0:0x0000FFFF;
	errp = c&QUANT_ERR_MASK;
	line1[0] = c>>QUANT_ERR_BITS ;
	line2[0] += (errp*5)>>4 ;

	for( i = 1 ; i < len ; ++i )
	{
		c = line1[i];
		if( (c&0xFFFF0000)!= 0 )
			c = ( c&0x7F000000 )?0:0x0000FFFF;
		c += ((errp*7)>>4) ;
		err = c&QUANT_ERR_MASK;
		line1[i] = (c&0x00FF0000)?0x000000FF:(c>>QUANT_ERR_BITS) ;
		line2[i-1] += (err*3)>>4 ;
		line2[i] += ((err*5)>>4)+(errp>>4);
		errp = err ;
	}
}

static inline void
fine_output_filter( register CARD32 *src, register CARD32 *dst, short ratio, int len )
{/* we carry half of the quantization error onto the following pixel and store it in dst: */
	register int i = 0;
	if( ratio <= 1 )
	{
		register int c = src[0];
  	    do
		{
			if( (c&0xFFFF0000)!= 0 )
				c = ( c&0x7F000000 )?0:0x0000FFFF;
			dst[i] = c>>(QUANT_ERR_BITS) ;
			if( ++i >= len )
				break;
			c = ((c&QUANT_ERR_MASK)>>1)+src[i];
		}while(1);
	}else if( ratio == 2 )
	{
		register CARD32 c = src[0];
  	    do
		{
			c = c>>1 ;
			if( (c&0xFFFF0000) != 0 )
				c = ( c&0x7F000000 )?0:0x0000FFFF;
			dst[i] = c>>(QUANT_ERR_BITS) ;
			if( ++i >= len )
				break;
			c = ((c&QUANT_ERR_MASK)>>1)+src[i];
		}while( 1 );
	}else
	{
		register CARD32 c = src[0];
  	    do
		{
			c = c/ratio ;
			if( c&0xFFFF0000 )
				c = ( c&0x7F000000 )?0:0x0000FFFF;
			dst[i] = c>>(QUANT_ERR_BITS) ;
			if( ++i >= len )
				break;
			c = ((c&QUANT_ERR_MASK)>>1)+src[i];
		}while(1);
	}
}

static inline void
fast_output_filter( register CARD32 *src, register CARD32 *dst, short ratio, int len )
{/*  no error diffusion whatsoever: */
	register int i = 0;
	if( ratio <= 1 )
	{
		for( ; i < len ; ++i )
		{
			register CARD32 c = src[i];
			if( (c&0xFFFF0000) != 0 )
				dst[i] = ( c&0x7F000000 )?0:0x000000FF;
			else
				dst[i] = c>>(QUANT_ERR_BITS) ;
		}
	}else if( ratio == 2 )
	{
		for( ; i < len ; ++i )
		{
			register CARD32 c = src[i]>>1;
			if( (c&0xFFFF0000) != 0 )
				dst[i] = ( c&0x7F000000 )?0:0x000000FF;
			else
				dst[i] = c>>(QUANT_ERR_BITS) ;
		}
	}else
	{
		for( ; i < len ; ++i )
		{
			register CARD32 c = src[i]/ratio;
			if( (c&0xFFFF0000) != 0 )
				dst[i] = ( c&0x7F000000 )?0:0x000000FF;
			else
				dst[i] = c>>(QUANT_ERR_BITS) ;
		}
	}
}

static inline void
start_component_interpolation( CARD32 *c1, CARD32 *c2, CARD32 *c3, CARD32 *c4, register CARD32 *T, register CARD32 *step, CARD16 S, int len)
{
	register int i;
	for( i = 0 ; i < len ; i++ )
	{
		register int rc2 = c2[i], rc3 = c3[i] ;
		T[i] = INTERPOLATION_TOTAL_START(c1[i],rc2,rc3,c4[i],S)/(S<<1);
		step[i] = INTERPOLATION_TOTAL_STEP(rc2,rc3)/(S<<1);
	}
}

static inline void
component_interpolation_hardcoded( CARD32 *c1, CARD32 *c2, CARD32 *c3, CARD32 *c4, register CARD32 *T, CARD32 *unused, CARD16 kind, int len)
{
	register int i;
	if( kind == 1 )
	{
		for( i = 0 ; i < len ; i++ )
		{
#if 1
			/* its seems that this simple formula is completely sufficient
			   and even better then more complicated one : */
			T[i] = (c2[i]+c3[i])>>1 ;
#else
    		register int minus = c1[i]+c4[i] ;
			register int plus  = (c2[i]<<1)+c2[i]+(c3[i]<<1)+c3[i];

			T[i] = ( (plus>>1) < minus )?(c2[i]+c3[i])>>1 :
								   		 (plus-minus)>>2;
#endif
		}
	}else if( kind == 2 )
	{
		for( i = 0 ; i < len ; i++ )
		{
    		register int rc1 = c1[i], rc2 = c2[i], rc3 = c3[i] ;
			T[i] = INTERPOLATE_A_COLOR3_V(rc1,rc2,rc3,c4[i]);
		}
	}else
		for( i = 0 ; i < len ; i++ )
		{
    		register int rc1 = c1[i], rc2 = c2[i], rc3 = c3[i] ;
			T[i] = INTERPOLATE_B_COLOR3_V(rc1,rc2,rc3,c4[i]);
		}
}

static inline void
divide_component_mod( register CARD32 *data, CARD16 ratio, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		data[i] /= ratio;
}

static inline void
rbitshift_component_mod( register CARD32 *data, int bits, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		data[i] = data[i]>>bits;
}

static inline void
fine_output_filter_mod( register CARD32 *data, int unused, int len )
{/* we carry half of the quantization error onto the following pixel : */
	register int i ;
	register CARD32 err = 0, c;
	for( i = 0 ; i < len ; ++i )
	{
		c = data[i];
		if( (c&0xFFFF0000) != 0 )
			c = ( c&0x7E000000 )?0:0x000000FF;
		c += err;
		err = (c&QUANT_ERR_MASK)>>1 ;
		data[i] = (c&0x00FF0000)?0x000000FF:c>>QUANT_ERR_BITS ;
	}
}

void
print_component( register CARD32 *data, int nonsense, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		fprintf( stderr, " %8.8lX", data[i] );
	fprintf( stderr, "\n");
}

static inline void
tint_component_mod( register CARD32 *data, CARD16 ratio, int len )
{
	register int i ;
	if( ratio == 255 )
		for( i = 0 ; i < len ; ++i )
			data[i] = data[i]<<8;
	else if( ratio == 128 )
		for( i = 0 ; i < len ; ++i )
			data[i] = data[i]<<7;
	else if( ratio == 0 )
		for( i = 0 ; i < len ; ++i )
			data[i] = 0;
	else
		for( i = 0 ; i < len ; ++i )
			data[i] = data[i]*ratio;
}

static inline void
make_component_gradient16( register CARD32 *data, CARD16 from, CARD16 to, CARD8 seed, int len )
{
	register int i ;
	long incr = (((long)to<<8)-((long)from<<8))/len ;

	if( incr == 0 )
		for( i = 0 ; i < len ; ++i )
			data[i] = from;
	else
	{
		long curr = from<<8;
		curr += ((((CARD32)seed)<<8) > incr)?incr:((CARD32)seed)<<8 ;
		for( i = 0 ; i < len ; ++i )
		{/* we make calculations in 24bit per chan, then convert it back to 16 and
		  * carry over half of the quantization error onto the next pixel */
			data[i] = curr>>8;
			curr += ((curr&0x00FF)>>1)+incr ;
		}
	}
}

/* the following 5 macros will in fact unfold into huge but fast piece of code : */
/* we make poor compiler work overtime unfolding all this macroses but I bet it  */
/* is still better then C++ templates :)									     */

#define ENCODE_SCANLINE(im,src,y) \
do{	asimage_add_line((im), IC_RED,   (src).red,   (y)); \
   	asimage_add_line((im), IC_GREEN, (src).green, (y)); \
   	asimage_add_line((im), IC_BLUE,  (src).blue,  (y)); \
	if( get_flags((src).flags,SCL_DO_ALPHA))asimage_add_line((im), IC_ALPHA, (src).alpha, (y)); \
  }while(0)

#define SCANLINE_FUNC(f,src,dst,scales,len) \
do{	if( (src).offset_x > 0 || (dst).offset_x > 0 ) \
		LOCAL_DEBUG_OUT( "(src).offset_x = %d. (dst).offset_x = %d", (src).offset_x, (dst).offset_x ); \
	f((src).red+(src).offset_x,  (dst).red+(dst).offset_x,  (scales),(len));		\
	f((src).green+(src).offset_x,(dst).green+(dst).offset_x,(scales),(len)); 		\
	f((src).blue+(src).offset_x, (dst).blue+(dst).offset_x, (scales),(len));   	\
	if(get_flags((src).flags,SCL_DO_ALPHA)) f((src).alpha+(src).offset_x,(dst).alpha+(dst).offset_x,(scales),(len)); \
  }while(0)

#define CHOOSE_SCANLINE_FUNC(r,src,dst,scales,len) \
 switch(r)                                              							\
 {  case 0:	SCANLINE_FUNC(shrink_component11,(src),(dst),(scales),(len));break;   	\
	case 1: SCANLINE_FUNC(shrink_component, (src),(dst),(scales),(len));	break;  \
	case 2:	SCANLINE_FUNC(enlarge_component12,(src),(dst),(scales),(len));break ; 	\
	case 3:	SCANLINE_FUNC(enlarge_component23,(src),(dst),(scales),(len));break;  	\
	default:SCANLINE_FUNC(enlarge_component,  (src),(dst),(scales),(len));        	\
 }

#define SCANLINE_MOD(f,src,p,len) \
do{	f((src).red+(src).offset_x,(p),(len));		\
	f((src).green+(src).offset_x,(p),(len));		\
	f((src).blue+(src).offset_x,(p),(len));		\
	if(get_flags((src).flags,SCL_DO_ALPHA)) f((src).alpha+(src).offset_x,(p),(len));\
  }while(0)

#define SCANLINE_COMBINE_slow(f,c1,c2,c3,c4,o1,o2,p,len)						   \
do{	f((c1).red,(c2).red,(c3).red,(c4).red,(o1).red,(o2).red,(p),(len));		\
	f((c1).green,(c2).green,(c3).green,(c4).green,(o1).green,(o2).green,(p),(len));	\
	f((c1).blue,(c2).blue,(c3).blue,(c4).blue,(o1).blue,(o2).blue,(p),(len));		\
	if(get_flags((c1).flags,SCL_DO_ALPHA)) f((c1).alpha,(c2).alpha,(c3).alpha,(c4).alpha,(o1).alpha,(o2).alpha,(p),(len));	\
  }while(0)

#define SCANLINE_COMBINE(f,c1,c2,c3,c4,o1,o2,p,len)						   \
do{	f((c1).red,(c2).red,(c3).red,(c4).red,(o1).red,(o2).red,(p),(len+(len&0x01))*3);		\
	if(get_flags((c1).flags,SCL_DO_ALPHA)) f((c1).alpha,(c2).alpha,(c3).alpha,(c4).alpha,(o1).alpha,(o2).alpha,(p),(len));	\
  }while(0)

static inline void
copytintpad_scanline( ASScanline *src, ASScanline *dst, int offset, ARGB32 tint )
{
	register int i ;
	CARD32 chan_tint[4], chan_fill[4] ;
	int color ;
	int copy_width = src->width, dst_offset = 0, src_offset = 0;

	if( offset+src->width < 0 || offset > dst->width )
		return;
	chan_tint[IC_RED]   = ARGB32_RED8  (tint)<<1;
	chan_tint[IC_GREEN] = ARGB32_GREEN8(tint)<<1;
	chan_tint[IC_BLUE]  = ARGB32_BLUE8 (tint)<<1;
	chan_tint[IC_ALPHA] = ARGB32_ALPHA8(tint)<<1;
	chan_fill[IC_RED]   = ARGB32_RED8  (dst->back_color)<<dst->shift;
	chan_fill[IC_GREEN] = ARGB32_GREEN8(dst->back_color)<<dst->shift;
	chan_fill[IC_BLUE]  = ARGB32_BLUE8 (dst->back_color)<<dst->shift;
	chan_fill[IC_ALPHA] = ARGB32_ALPHA8(dst->back_color)<<dst->shift;
	if( offset < 0 )
		src_offset = -offset ;
	else
		dst_offset = offset ;
	copy_width = MIN( src->width-src_offset, dst->width-dst_offset );

	dst->flags = src->flags ;
	for( color = 0 ; color < IC_NUM_CHANNELS ; ++color )
	{
		register CARD32 *psrc = src->channels[color]+src_offset;
		register CARD32 *pdst = dst->channels[color];
		int ratio = chan_tint[color];
/*	fprintf( stderr, "channel %d, tint is %d(%X), src_offset = %d, dst_offset = %d psrc = %p, pdst = %p\n", color, ratio, ratio, src_offset, dst_offset, psrc, pdst );*/
		{
/*			register CARD32 fill = chan_fill[color]; */
			for( i = 0 ; i < dst_offset ; ++i )
				pdst[i] = 0;
			pdst += dst_offset ;
		}

		if( get_flags(src->flags, 0x01<<color) )
		{
			if( ratio == 255 )
				for( i = 0 ; i < copy_width ; ++i )
					pdst[i] = psrc[i]<<8;
			else if( ratio == 128 )
				for( i = 0 ; i < copy_width ; ++i )
					pdst[i] = psrc[i]<<7;
			else if( ratio == 0 )
				for( i = 0 ; i < copy_width ; ++i )
					pdst[i] = 0;
			else
				for( i = 0 ; i < copy_width ; ++i )
					pdst[i] = psrc[i]*ratio;
		}else
		{
			for( i = 0 ; i < copy_width ; ++i )
				pdst[i] = ratio<<8;
			set_flags( dst->flags, (0x01<<color));
		}
		{
/*			register CARD32 fill = chan_fill[color]; */
			for( ; i < dst->width-dst_offset ; ++i )
				pdst[i] = 0;
/*				print_component(pdst, 0, dst->width ); */
		}
	}
}

void
decode_image_scanline( ASImageDecoder *imdec )
{
	int 	 			 y = imdec->next_line%imdec->im->height ;
	size_t   			 count ;
	register ASScanline *scl = &(imdec->buffer);

	clear_flags( scl->flags,SCL_DO_ALL);
	if( get_flags(imdec->filter, SCL_DO_RED) )
		if( (count = asimage_decode_line(imdec->im,IC_RED, scl->red,y,imdec->offset_x,scl->width)) < scl->width)
			set_component( scl->red, ARGB32_RED8(imdec->back_color)<<scl->shift, count, scl->width );
	if( get_flags(imdec->filter, SCL_DO_GREEN) )
		if( (count = asimage_decode_line(imdec->im,IC_GREEN, scl->green,y,imdec->offset_x,scl->width)) < scl->width)
			set_component( scl->green, ARGB32_GREEN8(imdec->back_color)<<scl->shift, count, scl->width );
	if( get_flags(imdec->filter, SCL_DO_BLUE) )
		if( (count = asimage_decode_line(imdec->im,IC_BLUE, scl->blue,y,imdec->offset_x,scl->width)) < scl->width)
			set_component( scl->blue, ARGB32_BLUE8(imdec->back_color)<<scl->shift, count, scl->width );

	set_flags( scl->flags,get_flags(imdec->filter,SCL_DO_COLOR));

	if( get_flags(imdec->filter, SCL_DO_ALPHA) )
		if( asimage_decode_line(imdec->im,IC_ALPHA, scl->alpha,y,imdec->offset_x,scl->width) >= scl->width)
			set_flags( scl->flags,SCL_DO_ALPHA);

	++(imdec->next_line);
}

void
encode_image_scanline_xim( ASImageOutput *imout, ASScanline *to_store )
{
	if( imout->next_line < imout->xim->height && imout->next_line >= 0 )
	{
		if( !get_flags(to_store->flags, SCL_DO_RED) )
			set_component( to_store->red  , ARGB32_RED8(to_store->back_color), 0, to_store->width );
		if( !get_flags(to_store->flags, SCL_DO_GREEN) )
			set_component( to_store->green, ARGB32_GREEN8(to_store->back_color), 0, to_store->width );
		if( !get_flags(to_store->flags, SCL_DO_BLUE) )
			set_component( to_store->blue , ARGB32_BLUE8(to_store->back_color), 0, to_store->width );

		PUT_SCANLINE(imout->asv, imout->xim,to_store,imout->next_line,imout->xim_line);
		if( imout->tiling_step > 0 )
		{
			register int i ;
			int step = ( imout->bottom_to_top )? -imout->tiling_step : imout->tiling_step ;
			int xim_step = step*imout->bpl ;
			unsigned char *xim_line = imout->xim_line+xim_step ;
			for( i = imout->next_line+step ; i < imout->height && i >= 0 ; i+=step )
			{
				memcpy( xim_line, imout->xim_line, imout->bpl );
				xim_line += xim_step ;
			}
		}
		if( imout->bottom_to_top )
		{
			imout->xim_line -= imout->bpl;
			--(imout->next_line);
		}else
		{
			imout->xim_line += imout->bpl;
			++(imout->next_line);
		}
	}
}

void
encode_image_scanline_asim( ASImageOutput *imout, ASScanline *to_store )
{
LOCAL_DEBUG_CALLER_OUT( "imout->next_line = %d, imout->im->height = %d", imout->next_line, imout->im->height );
	if( imout->next_line < imout->im->height && imout->next_line >= 0 )
	{
		CARD32 chan_fill[4];
		chan_fill[IC_RED]   = ARGB32_RED8  (to_store->back_color);
		chan_fill[IC_GREEN] = ARGB32_GREEN8(to_store->back_color);
		chan_fill[IC_BLUE]  = ARGB32_BLUE8 (to_store->back_color);
		chan_fill[IC_ALPHA] = ARGB32_ALPHA8(to_store->back_color);
		if( imout->tiling_step )
		{
			int bytes_count ;
			register int i, color ;
			int max_i = imout->im->height ;
			int step = (imout->bottom_to_top)?-imout->tiling_step:imout->tiling_step;

			for( color = 0 ; color < IC_NUM_CHANNELS ; color++ )
			{
				if( get_flags(to_store->flags,0x01<<color))
					bytes_count = asimage_add_line(imout->im, color, to_store->channels[color]+to_store->offset_x,   imout->next_line);
				else if( chan_fill[color] != imout->chan_fill[color] )
					bytes_count = asimage_add_line_mono( imout->im, color, chan_fill[color], imout->next_line);
				else
				{
					asimage_erase_line( imout->im, color, imout->next_line );
					for( i = imout->next_line+step ; i < max_i && i >= 0 ; i+=step )
						asimage_erase_line( imout->im, color, i );
					continue;
				}
				for( i = imout->next_line+step ; i < max_i && i >= 0 ; i+=step )
				{
/*						fprintf( stderr, "copy-encoding color %d, from lline %d to %d, %d bytes\n", color, imout->next_line, i, bytes_count );*/
					asimage_dup_line( imout->im, color, imout->next_line, i, bytes_count );
				}
			}
		}else
		{
			register int color ;
			for( color = 0 ; color < IC_NUM_CHANNELS ; color++ )
				if( get_flags(to_store->flags,0x01<<color))
					asimage_add_line(imout->im, color, to_store->channels[color]+to_store->offset_x, imout->next_line);
				else if( chan_fill[color] != imout->chan_fill[color] )
					asimage_add_line_mono( imout->im, color, chan_fill[color], imout->next_line);
				else
					asimage_erase_line( imout->im, color, imout->next_line );
		}
	}
	imout->next_line += imout->bottom_to_top?-1:1;
}

void
output_image_line_top( ASImageOutput *imout, ASScanline *new_line, int ratio )
{
	ASScanline *to_store = NULL ;
	/* caching and preprocessing line into our buffer : */
	if( new_line )
	{
		if( ratio > 1 )
			SCANLINE_FUNC(divide_component,*(new_line),*(imout->available),ratio,imout->available->width);
		else
			SCANLINE_FUNC(copy_component,*(new_line),*(imout->available),NULL,imout->available->width);
		imout->available->flags = new_line->flags ;
		imout->available->back_color = new_line->back_color ;
	}
	/* copying/encoding previously cahced line into destination image : */
	if( imout->used != NULL )
	{
		if( new_line != NULL )
			SCANLINE_FUNC(best_output_filter,*(imout->used),*(imout->available),0,imout->available->width);
		else
			SCANLINE_MOD(fine_output_filter_mod,*(imout->used),0,imout->used->width);
		to_store = imout->used ;
	}
	if( to_store )
		imout->encode_image_scanline( imout, to_store );

	/* rotating the buffers : */
	if( imout->buffer_shift > 0 )
	{
		if( new_line == NULL )
			imout->used = NULL ;
		else
			imout->used = imout->available ;
		imout->available = &(imout->buffer[0]);
		if( imout->available == imout->used )
			imout->available = &(imout->buffer[1]);
	}
}

void
output_image_line_fine( ASImageOutput *imout, ASScanline *new_line, int ratio )
{
	/* caching and preprocessing line into our buffer : */
	if( new_line )
	{
		SCANLINE_FUNC(fine_output_filter, *(new_line),*(imout->available),ratio,imout->available->width);
		imout->available->flags = new_line->flags ;
		imout->available->back_color = new_line->back_color ;
/*		SCANLINE_MOD(print_component,*(imout->available),0, new_line->width ); */
		/* copying/encoding previously cached line into destination image : */
		imout->encode_image_scanline( imout, imout->available );
	}
}

void
output_image_line_fast( ASImageOutput *imout, ASScanline *new_line, int ratio )
{
	/* caching and preprocessing line into our buffer : */
	if( new_line )
	{
		SCANLINE_FUNC(fast_output_filter,*(new_line),*(imout->available),ratio,imout->available->width);
		imout->available->flags = new_line->flags ;
		imout->available->back_color = new_line->back_color ;
		imout->encode_image_scanline( imout, imout->available );
	}
}

void
output_image_line_direct( ASImageOutput *imout, ASScanline *new_line, int ratio )
{
	/* caching and preprocessing line into our buffer : */
	if( new_line )
	{
		if( ratio != 1)
		{
			SCANLINE_FUNC(divide_component,*(new_line),*(imout->available),ratio,imout->available->width);
			imout->available->flags = new_line->flags ;
			imout->available->back_color = new_line->back_color ;
			imout->encode_image_scanline( imout, imout->available );
		}else
			imout->encode_image_scanline( imout, new_line );
	}
}

/***********************************************************************************************/
/* drawing gradient on scanline :  															   */
/***********************************************************************************************/
void
make_gradient_scanline( ASScanline *scl, ASGradient *grad, ASFlagType filter, ARGB32 seed )
{
	if( scl && grad && filter != 0 )
	{
		int offset = 0, step, i, max_i = grad->npoints - 1 ;
		for( i = 0  ; i < max_i ; i++ )
		{
			if( i == max_i-1 )
				step = scl->width - offset;
			else
				step = grad->offset[i+1] * scl->width - offset ;
			if( step > 0 )
			{
				int color ;
				for( color = 0 ; color < IC_NUM_CHANNELS ; ++color )
					if( get_flags( filter, 0x01<<color ) )
						make_component_gradient16( scl->channels[color]+offset,
												   ARGB32_CHAN8(grad->color[i],color)<<8,
												   ARGB32_CHAN8(grad->color[i+1],color)<<8,
												   ARGB32_CHAN8(seed,color),
												   step);
				offset += step ;
			}
		}
		scl->flags = filter ;
	}
}

/***********************************************************************************************/
/* Scaling code ; 																			   */
/***********************************************************************************************/
Bool
check_scale_parameters( ASImage *src, int *to_width, int *to_height )
{
	if( src == NULL )
		return False;

	if( *to_width < 0 )
		*to_width = src->width ;
	else if( *to_width < 2 )
		*to_width = 2 ;
	if( *to_height < 0 )
		*to_height = src->height ;
	else if( *to_height < 2 )
		*to_height = 2 ;
	return True;
}

int *
make_scales( unsigned short from_size, unsigned short to_size )
{
	int *scales ;
	unsigned short smaller = MIN(from_size,to_size);
	unsigned short bigger  = MAX(from_size,to_size);
	register int i = 0, k = 0;
	int eps;

	if( from_size < to_size )
	{	smaller--; bigger-- ; }
	if( smaller == 0 )
		smaller = 1;
	if( bigger == 0 )
		bigger = 1;
	scales = safecalloc( smaller, sizeof(int));
	eps = -(bigger>>1);
	/* now using Bresengham algoritm to fiill the scales :
	 * since scaling is merely transformation
	 * from 0:bigger space (x) to 0:smaller space(y)*/
	for ( i = 0 ; i < bigger ; i++ )
	{
		++scales[k];
		eps += smaller;
		if( (eps << 1) >= bigger )
		{
			++k ;
			eps -= bigger ;
		}
	}
	return scales;
}

/********************************************************************/
void
scale_image_down( ASImageDecoder *imdec, ASImageOutput *imout, int h_ratio, int *scales_h, int* scales_v)
{
	ASScanline dst_line, total ;
	int k = -1;
	int max_k 	 = imout->im->height,
		line_len = MIN(imout->im->width,imdec->im->width);

	prepare_scanline( imout->im->width, QUANT_ERR_BITS, &dst_line, imout->asv->BGR_mode );
	prepare_scanline( imout->im->width, QUANT_ERR_BITS, &total, imout->asv->BGR_mode );
	while( ++k < max_k )
	{
		int reps = scales_v[k] ;
		decode_image_scanline( imdec );
		total.flags = imdec->buffer.flags ;
		CHOOSE_SCANLINE_FUNC(h_ratio,imdec->buffer,total,scales_h,line_len);

		while( --reps > 0 )
		{
			decode_image_scanline( imdec );
			total.flags = imdec->buffer.flags ;
			CHOOSE_SCANLINE_FUNC(h_ratio,imdec->buffer,dst_line,scales_h,line_len);
			SCANLINE_FUNC(add_component,total,dst_line,NULL,total.width);
		}

		imout->output_image_scanline( imout, &total, scales_v[k] );
	}
	free_scanline(&dst_line, True);
	free_scanline(&total, True);
}

void
scale_image_up( ASImageDecoder *imdec, ASImageOutput *imout, int h_ratio, int *scales_h, int* scales_v)
{
	ASScanline step, src_lines[4], *c1, *c2, *c3, *c4 = NULL;
	int i = 0, max_i,
		line_len = MIN(imout->im->width,imdec->im->width),
		out_width = imout->im->width;

	for( i = 0 ; i < 4 ; i++ )
		prepare_scanline( out_width, 0, &(src_lines[i]), imout->asv->BGR_mode);
	prepare_scanline( out_width, QUANT_ERR_BITS, &step, imout->asv->BGR_mode );


/*	set_component(src_lines[0].red,0x00000000,0,out_width*3); */
	decode_image_scanline( imdec );
	step.flags = src_lines[0].flags = src_lines[1].flags = imdec->buffer.flags ;
	CHOOSE_SCANLINE_FUNC(h_ratio,imdec->buffer,src_lines[1],scales_h,line_len);

	SCANLINE_FUNC(copy_component,src_lines[1],src_lines[0],0,out_width);

	decode_image_scanline( imdec );
	src_lines[2].flags = imdec->buffer.flags ;
	CHOOSE_SCANLINE_FUNC(h_ratio,imdec->buffer,src_lines[2],scales_h,line_len);

	i = 0 ;
	max_i = imdec->im->height-1 ;
	do
	{
		int S = scales_v[i] ;

		c1 = &(src_lines[i&0x03]);
		c2 = &(src_lines[(i+1)&0x03]);
		c3 = &(src_lines[(i+2)&0x03]);
		c4 = &(src_lines[(i+3)&0x03]);

		if( i+1 < max_i )
		{
			decode_image_scanline( imdec );
			c4->flags = imdec->buffer.flags ;
			CHOOSE_SCANLINE_FUNC(h_ratio,imdec->buffer,*c4,scales_h,line_len);
		}
		/* now we'll prepare total and step : */
		imout->output_image_scanline( imout, c2, 1);
		if( S > 1 )
		{
			if( S == 2 )
			{
				SCANLINE_COMBINE(component_interpolation_hardcoded,*c1,*c2,*c3,*c4,*c1,*c1,1,out_width);
				imout->output_image_scanline( imout, c1, 1);
			}else if( S == 3 )
			{
				SCANLINE_COMBINE(component_interpolation_hardcoded,*c1,*c2,*c3,*c4,*c1,*c1,2,out_width);
				imout->output_image_scanline( imout, c1, 1);
				SCANLINE_COMBINE(component_interpolation_hardcoded,*c1,*c2,*c3,*c4,*c1,*c1,3,out_width);
				imout->output_image_scanline( imout, c1, 1);
			}else
			{
				SCANLINE_COMBINE(start_component_interpolation,*c1,*c2,*c3,*c4,*c1,step,S,out_width);
				do
				{
					imout->output_image_scanline( imout, c1, 1);
					if((--S)<=1)
						break;
					SCANLINE_FUNC(add_component,*c1,step,NULL,out_width );
 				}while(1);
			}
		}
	}while( ++i < max_i );
	imout->output_image_scanline( imout, c4, 1);

	for( i = 0 ; i < 4 ; i++ )
		free_scanline(&(src_lines[i]), True);
	free_scanline(&step, True);
}

/******************************************************************************/
/* ASImage transformations : 												  */
/******************************************************************************/
ASImage *
scale_asimage( ASVisual *asv, ASImage *src, unsigned int to_width, unsigned int to_height,
			   Bool to_xim, unsigned int compression_out, int quality )
{
	ASImage *dst = NULL ;
	ASImageOutput  *imout ;
	ASImageDecoder *imdec;
	int h_ratio ;
	int *scales_h = NULL, *scales_v = NULL;

	if( !check_scale_parameters(src,&to_width,&to_height) )
		return NULL;

	if( (imdec = start_image_decoding(asv, src, SCL_DO_ALL, 0, 0, src->width)) == NULL )
		return NULL;

	dst = safecalloc(1, sizeof(ASImage));
	asimage_start (dst, to_width, to_height, compression_out);
	if( to_xim )
		if( (dst->ximage = create_visual_ximage( asv, to_width, to_height, 0 )) == NULL )
		{
			show_error( "Unable to create XImage for the visual %d", asv->visual_info.visualid );
			asimage_init(dst, True);
			free( dst );
			stop_image_decoding( &imdec );
			return NULL ;
		}
	if( to_width == src->width )
		h_ratio = 0;
	else if( to_width < src->width )
		h_ratio = 1;
	else if( src->width > 1 )
	{
		h_ratio = to_width/(src->width-1);
		if( h_ratio*(src->width-1) < to_width )
			h_ratio++ ;
	}else
		h_ratio = to_width ;

	scales_h = make_scales( src->width, to_width );
	scales_v = make_scales( src->height, to_height );

#ifdef LOCAL_DEBUG
	{
	  register int i ;
	  for( i = 0 ; i < MIN(src->width, to_width) ; i++ )
		fprintf( stderr, " %d", scales_h[i] );
	  fprintf( stderr, "\n" );
	  for( i = 0 ; i < MIN(src->height, to_height) ; i++ )
		fprintf( stderr, " %d", scales_v[i] );
	  fprintf( stderr, "\n" );
	}
#endif
#ifdef HAVE_MMX
	mmx_init();
#endif
	if((imout = start_image_output( asv, dst, dst->ximage, to_xim, QUANT_ERR_BITS, quality )) == NULL )
	{
		asimage_init(dst, True);
		free( dst );
		dst = NULL ;
	}else
	{
		if( to_height <= src->height ) 					   /* scaling down */
			scale_image_down( imdec, imout, h_ratio, scales_h, scales_v );
		else
			scale_image_up( imdec, imout, h_ratio, scales_h, scales_v );
		stop_image_output( &imout );
	}
#ifdef HAVE_MMX
	mmx_off();
#endif
	free( scales_h );
	free( scales_v );
	stop_image_decoding( &imdec );
	return dst;
}

ASImage *
tile_asimage( ASVisual *asv, ASImage *src,
		      int offset_x, int offset_y,
			  unsigned int to_width,
			  unsigned int to_height,
			  ARGB32 tint,
			  Bool to_xim, unsigned int compression_out, int quality )
{
	ASImage *dst = NULL ;
	ASImageDecoder *imdec ;
	ASImageOutput  *imout ;
LOCAL_DEBUG_CALLER_OUT( "offset_x = %d, offset_y = %d, to_width = %d, to_height = %d", offset_x, offset_y, to_width, to_height );
	if( (imdec = start_image_decoding(asv, src, SCL_DO_ALL, offset_x, offset_y, to_width)) == NULL )
		return NULL;

	dst = safecalloc(1, sizeof(ASImage));
	asimage_start (dst, to_width, to_height, compression_out);
	if( to_xim )
		if( (dst->ximage = create_visual_ximage( asv, to_width, to_height, 0 )) == NULL )
		{
			show_error( "Unable to create XImage for the visual %d", asv->visual_info.visualid );
			asimage_init(dst, True);
			free( dst );
			stop_image_decoding( &imdec );
			return NULL ;
		}
#ifdef HAVE_MMX
	mmx_init();
#endif
	if((imout = start_image_output( asv, dst, dst->ximage, to_xim, (tint!=0)?8:0, quality)) == NULL )
	{
		asimage_init(dst, True);
		free( dst );
		dst = NULL ;
	}else
	{
		int y, max_y = to_height;
LOCAL_DEBUG_OUT("tiling actually...%s", "");
		if( to_height > src->height )
		{
			imout->tiling_step = src->height ;
			max_y = src->height ;
		}
		if( tint != 0 )
		{
			for( y = 0 ; y < max_y ; y++  )
			{
				decode_image_scanline( imdec );
				tint_component_mod( imdec->buffer.red, ARGB32_RED8(tint)<<1, to_width );
				tint_component_mod( imdec->buffer.green, ARGB32_GREEN8(tint)<<1, to_width );
  				tint_component_mod( imdec->buffer.blue, ARGB32_BLUE8(tint)<<1, to_width );
				tint_component_mod( imdec->buffer.alpha, ARGB32_ALPHA8(tint)<<1, to_width );
				imout->output_image_scanline( imout, &(imdec->buffer), 1);
			}
		}else
			for( y = 0 ; y < max_y ; y++  )
			{
				decode_image_scanline( imdec );
				imout->output_image_scanline( imout, &(imdec->buffer), 1);
			}
		stop_image_output( &imout );
	}
#ifdef HAVE_MMX
	mmx_off();
#endif
	stop_image_decoding( &imdec );
	return dst;
}

ASImage *
merge_layers( ASVisual *asv,
				ASImageLayer *layers, int count,
			  	unsigned int dst_width,
			  	unsigned int dst_height,
			  	Bool to_xim, unsigned int compression_out, int quality )
{
	ASImage *dst = NULL ;
	ASImageDecoder **imdecs ;
	ASImageOutput  *imout ;
	ASScanline dst_line, tmp_line ;
	int i ;
LOCAL_DEBUG_CALLER_OUT( "dst_width = %d, dst_height = %d", dst_width, dst_height );
	dst = safecalloc(1, sizeof(ASImage));
	asimage_start (dst, dst_width, dst_height, compression_out);
	if( to_xim )
		if( (dst->ximage = create_visual_ximage( asv, dst_width, dst_height, 0 )) == NULL )
		{
			show_error( "Unable to create XImage for the visual %d", asv->visual_info.visualid );
			asimage_init(dst, True);
			free( dst );
			return NULL ;
		}
	prepare_scanline( dst->width, QUANT_ERR_BITS, &dst_line, asv->BGR_mode );
	prepare_scanline( dst->width, QUANT_ERR_BITS, &tmp_line, asv->BGR_mode );
	dst_line.flags = SCL_DO_ALL ;
	tmp_line.flags = SCL_DO_ALL ;

	imdecs = safecalloc( count, sizeof(ASImageDecoder*));
	for( i = 0 ; i < count ; i++ )
		if( layers[i].im )
		{
			imdecs[i] = start_image_decoding(asv, layers[i].im, SCL_DO_ALL, layers[i].clip_x, layers[i].clip_y, layers[i].clip_width);
			imdecs[i]->back_color = layers[i].back_color ;
		}
#ifdef HAVE_MMX
	mmx_init();
#endif

	if((imout = start_image_output( asv, dst, dst->ximage, to_xim, QUANT_ERR_BITS, quality)) == NULL )
	{
		asimage_init(dst, True);
		free( dst );
		dst = NULL ;
	}else
	{
		int y, max_y = 0;
		int min_y = dst_height;
LOCAL_DEBUG_OUT("blending actually...%s", "");
		for( i = 0 ; i < count ; i++ )
			if( imdecs[i] )
			{
				if( layers[i].dst_y < min_y )
					min_y = layers[i].dst_y;
				if( layers[i].dst_y+layers[i].clip_height > max_y )
					max_y = layers[i].dst_y+layers[i].clip_height;
			}
		if( min_y < 0 )
			min_y = 0 ;
		if( max_y > dst_height )
			max_y = dst_height ;
		else
			imout->tiling_step = max_y ;

/*		for( i = 0 ; i < count ; i++ )
			if( imdecs[i] )
				imdecs[i]->next_line = min_y - layers[i].dst_y ;
 */
LOCAL_DEBUG_OUT( "min_y = %d, max_y = %d", min_y, max_y );
		for( y = min_y ; y < max_y ; y++  )
		{
			for( i = 0 ; i < count ; i++ )
				if( imdecs[i] && layers[i].dst_y <= y && layers[i].dst_y+layers[i].clip_height > y )
				{
					decode_image_scanline( imdecs[i] );
					copytintpad_scanline( &(imdecs[i]->buffer), &dst_line, layers[i].dst_x, (layers[i].tint==0)?0x7F7F7F7F:layers[i].tint );
					break;
				}
			while( ++i < count )
				if( imdecs[i] && layers[i].dst_y <= y && layers[i].dst_y+layers[i].clip_height > y )
				{
					decode_image_scanline( imdecs[i] );
					copytintpad_scanline( &(imdecs[i]->buffer), &tmp_line, layers[i].dst_x, (layers[i].tint==0)?0x7F7F7F7F:layers[i].tint );
					layers[i].merge_scanlines( &dst_line, &tmp_line, layers[i].merge_mode );
				}
			imout->output_image_scanline( imout, &dst_line, 1);
		}
		stop_image_output( &imout );
	}
#ifdef HAVE_MMX
	mmx_off();
#endif
	for( i = 0 ; i < count ; i++ )
		if( imdecs[i] != NULL )
			stop_image_decoding( &(imdecs[i]) );
	free( imdecs );
	free_scanline( &tmp_line, True );
	free_scanline( &dst_line, True );
	return dst;
}

/***************************************************************************************/
/* GRADIENT drawing : 																   */
/***************************************************************************************/
static void
make_gradient_left2right( ASImageOutput *imout, ASScanline *dither_lines, int dither_lines_num, ASFlagType filter )
{
	int line ;

	imout->tiling_step = dither_lines_num;
	for( line = 0 ; line < dither_lines_num ; line++ )
		imout->output_image_scanline( imout, &(dither_lines[line]), 1);
}

static void
make_gradient_top2bottom( ASImageOutput *imout, ASScanline *dither_lines, int dither_lines_num, ASFlagType filter )
{
	int y, height = imout->height, width = imout->im->width ;
	int line ;
	ASScanline result;
	CARD32 chan_data[MAX_GRADIENT_DITHER_LINES] = {0,0,0,0};
LOCAL_DEBUG_CALLER_OUT( "width = %d, height = %d, filetr = 0x%lX, dither_count = %d", width, height, filter, dither_lines_num );
	prepare_scanline( width, QUANT_ERR_BITS, &result, imout->asv->BGR_mode );
	for( y = 0 ; y < height ; y++ )
	{
		int color ;

		result.flags = 0 ;
		result.back_color = ARGB32_DEFAULT_BACK_COLOR ;
		for( color = 0 ; color < IC_NUM_CHANNELS ; color++ )
			if( get_flags( filter, 0x01<<color ) )
			{
				Bool dithered = False ;
				for( line = 0 ; line < dither_lines_num ; line++ )
				{
					/* we want to do error diffusion here since in other places it only works
						* in horisontal direction : */
					CARD32 c = dither_lines[line].channels[color][y] + ((dither_lines[line].channels[color][y+1]&0xFF)>>1);
					if( (c&0xFFFF0000) != 0 )
						chan_data[line] = ( c&0x7F000000 )?0:0x0000FF00;
					else
						chan_data[line] = c ;

					if( chan_data[line] != chan_data[0] )
						dithered = True;
				}
				if( !dithered )
				{
					result.back_color = (result.back_color&(~MAKE_ARGB32_CHAN8(0xFF,color)))|
										MAKE_ARGB32_CHAN16(chan_data[0],color);
				}else
				{
					register CARD32  *dst = result.channels[color] ;
					for( line = 0 ; line  < dither_lines_num ; line++ )
					{
						register int x ;
						register CARD32 d = chan_data[line] ;
						for( x = line ; x < width ; x+=dither_lines_num )
							dst[x] = d ;
					}
					set_flags(result.flags, 0x01<<color);
				}
			}
		imout->output_image_scanline( imout, &result, 1);
	}
	free_scanline( &result, True );
}

static void
make_gradient_diag_width( ASImageOutput *imout, ASScanline *dither_lines, int dither_lines_num, ASFlagType filter, Bool from_bottom )
{
	int line = 0;
	/* using bresengham algorithm again to trigger horizontal shift : */
	unsigned short smaller = imout->im->height;
	unsigned short bigger  = imout->im->width;
	register int i = 0;
	int eps;

	if( from_bottom )
		toggle_image_output_direction( imout );
	eps = -(bigger>>1);
	for ( i = 0 ; i < bigger ; i++ )
	{
		eps += smaller;
		if( (eps << 1) >= bigger )
		{
			/* put scanline with the same x offset */
			dither_lines[line].offset_x = i ;
			imout->output_image_scanline( imout, &(dither_lines[line]), 1);
			if( ++line >= dither_lines_num )
				line = 0;
			eps -= bigger ;
		}
	}
}

static void
make_gradient_diag_height( ASImageOutput *imout, ASScanline *dither_lines, int dither_lines_num, ASFlagType filter, Bool from_bottom )
{
	int line = 0;
	unsigned short width = imout->im->width, height = imout->im->height ;
	/* using bresengham algorithm again to trigger horizontal shift : */
	unsigned short smaller = width;
	unsigned short bigger  = height;
	register int i = 0, k =0;
	int eps;
	ASScanline result;
	int *offsets ;

	prepare_scanline( width, QUANT_ERR_BITS, &result, imout->asv->BGR_mode );
	offsets = safemalloc( sizeof(int)*width );
	offsets[0] = 0 ;

	eps = -(bigger>>1);
	for ( i = 0 ; i < bigger ; i++ )
	{
		++offsets[k];
		eps += smaller;
		if( (eps << 1) >= bigger )
		{
			++k ;
			if( k < width )
				offsets[k] = offsets[k-1] ; /* seeding the offset */
			eps -= bigger ;
		}
	}

	if( from_bottom )
		toggle_image_output_direction( imout );

	result.flags = (filter&SCL_DO_ALL);
	if( (filter&SCL_DO_ALL) == SCL_DO_ALL )
	{
		for( i = 0 ; i < height ; i++ )
		{
			for( k = 0 ; k < width ; k++ )
			{
				int offset = i+offsets[k] ;
				CARD32 **src_chan = &(dither_lines[line].channels[0]) ;
				result.alpha[k] = src_chan[IC_ALPHA][offset] ;
				result.red  [k] = src_chan[IC_RED]  [offset] ;
				result.green[k] = src_chan[IC_GREEN][offset] ;
				result.blue [k] = src_chan[IC_BLUE] [offset] ;
				if( ++line >= dither_lines_num )
					line = 0 ;
			}
			imout->output_image_scanline( imout, &result, 1);
		}
	}else
	{
		for( i = 0 ; i < height ; i++ )
		{
			for( k = 0 ; k < width ; k++ )
			{
				int offset = i+offsets[k] ;
				CARD32 **src_chan = &(dither_lines[line].channels[0]) ;
				if( get_flags(filter, SCL_DO_ALPHA) )
					result.alpha[k] = src_chan[IC_ALPHA][offset] ;
				if( get_flags(filter, SCL_DO_RED) )
					result.red[k]   = src_chan[IC_RED]  [offset] ;
				if( get_flags(filter, SCL_DO_GREEN) )
					result.green[k] = src_chan[IC_GREEN][offset] ;
				if( get_flags(filter, SCL_DO_BLUE) )
					result.blue[k]  = src_chan[IC_BLUE] [offset] ;
				if( ++line >= dither_lines_num )
					line = 0 ;
			}
			imout->output_image_scanline( imout, &result, 1);
		}
	}

	free( offsets );
	free_scanline( &result, True );
}

ASImage*
make_gradient( ASVisual *asv, ASGradient *grad,
               unsigned int width, unsigned int height, ASFlagType filter,
  			   Bool to_xim, unsigned int compression_out, int quality  )
{
	ASImage *im = NULL ;
	ASImageOutput *imout;
	int line_len = width;

	if( asv == NULL || grad == NULL )
		return NULL;
	if( width == 0 )
		width = 2;
 	if( height == 0 )
		height = 2;
	im = safecalloc( 1, sizeof(ASImage) );
	asimage_start (im, width, height, compression_out);
	if( to_xim )
		if( (im->ximage = create_visual_ximage( asv, width, height, 0 )) == NULL )
		{
			show_error( "Unable to create XImage for the visual %d", asv->visual_info.visualid );
			asimage_init(im, True);
			free( im );
			return NULL ;
		}
	if( get_flags(grad->type,GRADIENT_TYPE_ORIENTATION) )
		line_len = height ;
	if( get_flags(grad->type,GRADIENT_TYPE_DIAG) )
		line_len = MAX(width,height)<<1 ;
	if((imout = start_image_output( asv, im, im->ximage, to_xim, QUANT_ERR_BITS, quality)) == NULL )
	{
		asimage_init(im, True);
		free( im );
		im = NULL ;
	}else
	{
		int dither_lines = MIN(imout->quality+1, MAX_GRADIENT_DITHER_LINES) ;
		ASScanline *lines;
		int line;
		static ARGB32 dither_seeds[MAX_GRADIENT_DITHER_LINES] = { 0, 0xFFFFFFFF, 0x7F0F7F0F, 0x0F7F0F7F };

		if( dither_lines > im->height || dither_lines > im->width )
			dither_lines = MIN(im->height, im->width) ;

		lines = safecalloc( dither_lines, sizeof(ASScanline));
		for( line = 0 ; line < dither_lines ; line++ )
		{
			prepare_scanline( line_len, QUANT_ERR_BITS, &(lines[line]), asv->BGR_mode );
			make_gradient_scanline( &(lines[line]), grad, filter, dither_seeds[line] );
		}

		switch( get_flags(grad->type,GRADIENT_TYPE_MASK) )
		{
			case GRADIENT_Left2Right :
				make_gradient_left2right( imout, lines, dither_lines, filter );
  	    		break ;
			case GRADIENT_Top2Bottom :
				make_gradient_top2bottom( imout, lines, dither_lines, filter );
				break ;
			case GRADIENT_TopLeft2BottomRight :
			case GRADIENT_BottomLeft2TopRight :
				if( width >= height )
					make_gradient_diag_width( imout, lines, dither_lines, filter,
											 (grad->type==GRADIENT_BottomLeft2TopRight));
				else
					make_gradient_diag_height( imout, lines, dither_lines, filter,
											  (grad->type==GRADIENT_BottomLeft2TopRight));
				break ;
			default:
		}
		stop_image_output( &imout );
		for( line = 0 ; line < dither_lines ; line++ )
			free_scanline( &(lines[line]), True );
		free( lines );
	}
	return im;
}

/****************************************************************************/
/* Image flipping(rotation)													*/
/****************************************************************************/
ASImage *
flip_asimage( ASVisual *asv, ASImage *src,
		      int offset_x, int offset_y,
			  unsigned int to_width,
			  unsigned int to_height,
			  int flip,
			  Bool to_xim, unsigned int compression_out, int quality )
{
	ASImage *dst = NULL ;
	ASImageDecoder *imdec ;
	ASImageOutput  *imout ;
	ASFlagType filter = SCL_DO_ALL;
LOCAL_DEBUG_CALLER_OUT( "offset_x = %d, offset_y = %d, to_width = %d, to_height = %d", offset_x, offset_y, to_width, to_height );
	if( src )
		filter = get_asimage_chanmask(src);
	if( (imdec = start_image_decoding(asv, src, filter, offset_x, offset_y, to_width)) == NULL )
		return NULL;


	dst = safecalloc(1, sizeof(ASImage));
	asimage_start (dst, to_width, to_height, compression_out);
	if( to_xim )
		if( (dst->ximage = create_visual_ximage( asv, to_width, to_height, 0 )) == NULL )
		{
			show_error( "Unable to create XImage for the visual %d", asv->visual_info.visualid );
			asimage_init(dst, True);
			free( dst );
			stop_image_decoding( &imdec );
			return NULL ;
		}
#ifdef HAVE_MMX
	mmx_init();
#endif
	if((imout = start_image_output( asv, dst, dst->ximage, to_xim, 0, quality)) == NULL )
	{
		asimage_init(dst, True);
		free( dst );
		dst = NULL ;
	}else
	{
		int y, max_y = to_height;
LOCAL_DEBUG_OUT("flip-flopping actually...%s", "");
		if( get_flags( flip, FLIP_VERTICAL ) )
		{
		}else
		{
			ASScanline result ;
			toggle_image_output_direction( imout );
			prepare_scanline( to_width, 0, &result, asv->BGR_mode );
			for( y = 0 ; y < max_y ; y++  )
			{
				decode_image_scanline( imdec );
				result.flags = imdec->buffer.flags ;
				result.back_color = imdec->buffer.back_color ;
				SCANLINE_FUNC(reverse_component,imdec->buffer,result,0,to_width);
				imout->output_image_scanline( imout, &result, 1);
			}
		}
		stop_image_output( &imout );
	}
#ifdef HAVE_MMX
	mmx_off();
#endif
	stop_image_decoding( &imdec );
	return dst;


}

/****************************************************************************/
/* ASImage->XImage->pixmap->XImage->ASImage conversion						*/
/****************************************************************************/

ASImage      *
ximage2asimage (ASVisual *asv, XImage * xim, unsigned int compression)
{
	ASImage      *im = NULL;
	unsigned char *xim_line;
	int           i, height, bpl;
	ASScanline    xim_buf;
#ifdef LOCAL_DEBUG
	CARD32       *tmp ;
#endif

	if( xim == NULL)
		return im;

	height = xim->height;
	bpl 	  = xim->bytes_per_line;
	xim_line = xim->data;

	im = (ASImage *) safecalloc (1, sizeof (ASImage));
	asimage_start (im, xim->width, xim->height, compression);
#ifdef LOCAL_DEBUG
	tmp = safemalloc( xim->width * sizeof(CARD32));
#endif
	prepare_scanline( xim->width, 0, &xim_buf, asv->BGR_mode );
	for (i = 0; i < height; i++)
	{
		GET_SCANLINE(asv,xim,&xim_buf,i,xim_line);
		asimage_add_line (im, IC_RED,   xim_buf.red, i);
		asimage_add_line (im, IC_GREEN, xim_buf.green, i);
		asimage_add_line (im, IC_BLUE,  xim_buf.blue, i);
#ifdef LOCAL_DEBUG
		if( !asimage_compare_line( im, IC_RED,  xim_buf.red, tmp, i, True ) )
			exit(0);
		if( !asimage_compare_line( im, IC_GREEN,  xim_buf.green, tmp, i, True ) )
			exit(0);
		if( !asimage_compare_line( im, IC_BLUE,  xim_buf.blue, tmp, i, True ) )
			exit(0);
#endif
		xim_line += bpl;
	}
	free_scanline(&xim_buf, True);

	return im;
}

XImage*
asimage2ximage (ASVisual *asv, ASImage *im)
{
	XImage        *xim = NULL;
	int            i;
	ASScanline     xim_buf;
	ASImageOutput *imout ;
#ifdef DO_CLOCKING
	clock_t       started = clock ();
#endif

	if (im == NULL)
		return xim;

	xim = create_visual_ximage( asv, im->width, im->height, 0 );
	if( (imout = start_image_output( asv, im, xim, True, 0, ASIMAGE_QUALITY_DEFAULT )) == NULL )
		return xim;

	prepare_scanline( xim->width, 0, &xim_buf, asv->BGR_mode );
#ifdef DO_CLOCKING
	started = clock ();
#endif
	for (i = 0; i < im->height; i++)
	{
		asimage_decode_line (im, IC_RED,   xim_buf.red, i, 0, xim_buf.width);
		asimage_decode_line (im, IC_GREEN, xim_buf.green, i, 0, xim_buf.width);
		asimage_decode_line (im, IC_BLUE,  xim_buf.blue, i, 0, xim_buf.width);
		imout->output_image_scanline( imout, &xim_buf, 1 );
	}
#ifdef DO_CLOCKING
	fprintf (stderr, "asimage->ximage time (clocks): %lu\n", clock () - started);
#endif
	free_scanline(&xim_buf, True);

	return xim;
}

XImage*
asimage2mask_ximage (ASVisual *asv, ASImage *im)
{
	XImage        *xim = NULL;
	int            i;
	ASScanline     xim_buf;
	ASImageOutput *imout ;

	if (im == NULL)
		return xim;

	xim = create_visual_ximage( asv, im->width, im->height, 1 );
	if( (imout = start_image_output( asv, im, xim, True, 0, ASIMAGE_QUALITY_DEFAULT )) == NULL )
		return xim;

	prepare_scanline( xim->width, 0, &xim_buf, asv->BGR_mode );
	for (i = 0; i < im->height; i++)
	{
		asimage_decode_line (im, IC_RED, xim_buf.alpha, i, 0, xim_buf.width);
		imout->output_image_scanline( imout, &xim_buf, 1 );
	}
	free_scanline(&xim_buf, True);

	return xim;
}

ASImage      *
pixmap2asimage(ASVisual *asv, Pixmap p, int x, int y, unsigned int width, unsigned int height, unsigned long plane_mask, Bool keep_cache, unsigned int compression)
{
	XImage       *xim = XGetImage (asv->dpy, p, x, y, width, height, plane_mask, ZPixmap);
	ASImage      *im = NULL;

	if (xim)
	{
		im = ximage2asimage (asv, xim, compression);
		if( keep_cache )
			im->ximage = xim ;
		else
			XDestroyImage (xim);
	}
	return im;
}

Pixmap
asimage2pixmap(ASVisual *asv, Window root, ASImage *im, GC gc, Bool use_cached)
{
	XImage       *xim ;
	Pixmap        p = None;

	if ( !use_cached || im->ximage == NULL )
	{
		if( (xim = asimage2ximage( asv, im )) == NULL )
		{
			show_error("cannot export image into XImage.");
			return None ;
		}
	}else
		xim = im->ximage ;
	if (xim != NULL )
	{
		p = create_visual_pixmap( asv, root, xim->width, xim->height, 0 );
		XPutImage( asv->dpy, p, gc, xim, 0, 0, 0, 0, xim->width, xim->height );
		if( xim != im->ximage )
			XDestroyImage (xim);
	}
	return p;
}

Pixmap
asimage2mask(ASVisual *asv, Window root, ASImage *im, GC gc, Bool use_cached)
{
	XImage       *xim ;
	Pixmap        mask = None;

	if( (xim = asimage2mask_ximage( asv, im )) == NULL )
	{
		show_error("cannot export image's mask into XImage.");
		return None ;
	}
	mask = create_visual_pixmap( asv, root, xim->width, xim->height, 1 );
	XPutImage( asv->dpy, mask, gc, xim, 0, 0, 0, 0, xim->width, xim->height );
	XDestroyImage (xim);
	return mask;
}


/*********************************************************************************/
/* The end !!!! 																 */
/*********************************************************************************/

