/*
 * Copyright (c) 2001 Sasha Vasko <sasha at aftercode.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#undef LOCAL_DEBUG
/*#define DO_CLOCKING*/

#ifdef _WIN32
#include "win32/config.h"
#else
#include "config.h"
#endif


#define DO_X11_ANTIALIASING
#define DO_2STEP_X11_ANTIALIASING
#define DO_3STEP_X11_ANTIALIASING
#define X11_AA_HEIGHT_THRESHOLD 10
#define X11_2STEP_AA_HEIGHT_THRESHOLD 15
#define X11_3STEP_AA_HEIGHT_THRESHOLD 15


#ifndef _WIN32
#include <unistd.h>
#endif
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef DO_CLOCKING
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#endif

#ifdef HAVE_FREETYPE
#ifndef HAVE_FT2BUILD_H
#ifdef HAVE_FREETYPE_FREETYPE
#include <freetype/freetype.h>
#else
#include <freetype.h>
#endif
#else
#include <ft2build.h>
#include FT_FREETYPE_H
#endif
#endif

#define INCLUDE_ASFONT_PRIVATE

#ifdef _WIN32
# include "win32/afterbase.h"
#else
# include "afterbase.h"
#endif
#include "asfont.h"
#include "asimage.h"
#include "asvisual.h"
#include "char2uni.h"

#undef MAX_GLYPHS_PER_FONT


/*********************************************************************************/
/* TrueType and X11 font management functions :   								 */
/*********************************************************************************/

/*********************************************************************************/
/* construction destruction miscelanea:			   								 */
/*********************************************************************************/

void asfont_destroy (ASHashableValue value, void *data);

ASFontManager *
create_font_manager( Display *dpy, const char * font_path, ASFontManager *reusable_memory )
{
	ASFontManager *fontman = reusable_memory;
	if( fontman == NULL )
		fontman = safecalloc( 1, sizeof(ASFontManager));
	else
		memset( fontman, 0x00, sizeof(ASFontManager));

	fontman->dpy = dpy ;
	if( font_path )
		fontman->font_path = mystrdup( font_path );

#ifdef HAVE_FREETYPE
	if( !FT_Init_FreeType( &(fontman->ft_library)) )
		fontman->ft_ok = True ;
	else
		show_error( "Failed to initialize FreeType library - TrueType Fonts support will be disabled!");
LOCAL_DEBUG_OUT( "Freetype library is %p", fontman->ft_library );
#endif

	fontman->fonts_hash = create_ashash( 7, string_hash_value, string_compare, asfont_destroy );

	return fontman;
}

void
destroy_font_manager( ASFontManager *fontman, Bool reusable )
{
	if( fontman )
	{

        destroy_ashash( &(fontman->fonts_hash) );

#ifdef HAVE_FREETYPE
		FT_Done_FreeType( fontman->ft_library);
		fontman->ft_ok = False ;
#endif
		if( fontman->font_path )
			free( fontman->font_path );

		if( !reusable )
			free( fontman );
		else
			memset( fontman, 0x00, sizeof(ASFontManager));
	}
}

#ifdef HAVE_FREETYPE
static int load_freetype_glyphs( ASFont *font );
#endif
#ifndef X_DISPLAY_MISSING
static int load_X11_glyphs( Display *dpy, ASFont *font, XFontStruct *xfs );
#endif

ASFont*
open_freetype_font( ASFontManager *fontman, const char *font_string, int face_no, int size, Bool verbose)
{
	ASFont *font = NULL ;
#ifdef HAVE_FREETYPE
	if( fontman && fontman->ft_ok )
	{
		char *realfilename;
		FT_Face face ;
		LOCAL_DEBUG_OUT( "looking for \"%s\" in \"%s\"", font_string, fontman->font_path );
		if( (realfilename = find_file( font_string, fontman->font_path, R_OK )) == NULL )
		{/* we might have face index specifier at the end of the filename */
			char *tmp = mystrdup( font_string );
			register int i = 0;
			while(tmp[i] != '\0' ) ++i ;
			while( --i >= 0 )
				if( !isdigit( tmp[i] ) )
				{
					if( tmp[i] == '.' )
					{
						face_no = atoi( &tmp[i+1] );
						tmp[i] = '\0' ;
					}
					break;
				}
			if( i >= 0 && font_string[i] != '\0' )
				realfilename = find_file( tmp, fontman->font_path, R_OK );
			free( tmp );
		}

		if( realfilename )
		{
			face = NULL ;
LOCAL_DEBUG_OUT( "font file found : \"%s\", trying to load face #%d, using library %p", realfilename, face_no, fontman->ft_library );
			if( FT_New_Face( fontman->ft_library, realfilename, face_no, &face ) )
			{
LOCAL_DEBUG_OUT( "face load failed.%s", "" );

				if( face_no  > 0  )
				{
					show_warning( "face %d is not available in font \"%s\" - falling back to first available.", face_no, realfilename );
					FT_New_Face( fontman->ft_library, realfilename, 0, &face );
				}
			}
LOCAL_DEBUG_OUT( "face found : %p", face );
			if( face != NULL )
			{
#ifdef MAX_GLYPHS_PER_FONT
				if( face->num_glyphs >  MAX_GLYPHS_PER_FONT )
					show_error( "Font \"%s\" contains too many glyphs - %d. Max allowed is %d", realfilename, face->num_glyphs, MAX_GLYPHS_PER_FONT );
				else
#endif
				{
					font = safecalloc( 1, sizeof(ASFont));
					font->magic = MAGIC_ASFONT ;
					font->fontman = fontman;
					font->type = ASF_Freetype ;
					font->ft_face = face ;
					FT_Set_Pixel_Sizes( font->ft_face, size, size );
					font->space_size = size ;
	   				load_freetype_glyphs( font );
				}
			}else if( verbose )
				show_error( "FreeType library failed to load font \"%s\"", realfilename );

			if( realfilename != font_string )
				free( realfilename );
		}
	}
#endif
	return font;
}

ASFont*
open_X11_font( ASFontManager *fontman, const char *font_string)
{
	ASFont *font = NULL ;
#ifndef X_DISPLAY_MISSING
/* 
    #ifdef I18N
     TODO: we have to use FontSet and loop through fonts instead filling
           up 2 bytes per character table with glyphs 
    #else 
*/
    /* for now assume ISO Latin 1 encoding */
	XFontStruct *xfs ;
	if( fontman->dpy == NULL ) 
		return NULL;
	if( (xfs = XLoadQueryFont( fontman->dpy, font_string )) == NULL )
	{
		show_warning( "failed to load X11 font \"%s\". Sorry about that.", font_string );
		return NULL;
	}
	font = safecalloc( 1, sizeof(ASFont));
	font->magic = MAGIC_ASFONT ;
	font->fontman = fontman;
	font->type = ASF_X11 ;
	load_X11_glyphs( fontman->dpy, font, xfs );
	XFreeFont( fontman->dpy, xfs );
#endif /* #ifndef X_DISPLAY_MISSING */
	return font;
}

ASFont*
get_asfont( ASFontManager *fontman, const char *font_string, int face_no, int size, ASFontType type )
{
	ASFont *font = NULL ;
	Bool freetype = False ;
	if( face_no >= 100 )
		face_no = 0 ;
	if( size >= 1000 )
		size = 999 ;

	if( fontman && font_string )
	{
		ASHashData hdata = { 0 };
		if( get_hash_item( fontman->fonts_hash, AS_HASHABLE((char*)font_string), &hdata.vptr) != ASH_Success )
		{
			char *ff_name ;
			int len = strlen( font_string)+1 ;
			len += ((size>=100)?3:2)+1 ;
			len += ((face_no>=10)?2:1)+1 ;
			ff_name = safemalloc( len );
			sprintf( ff_name, "%s$%d$%d", font_string, size, face_no );
			if( get_hash_item( fontman->fonts_hash, AS_HASHABLE((char*)ff_name), &hdata.vptr) != ASH_Success )
			{	/* not loaded just yet - lets do it :*/
				if( type == ASF_Freetype || type == ASF_GuessWho )
					font = open_freetype_font( fontman, font_string, face_no, size, (type == ASF_Freetype));
				if( font == NULL && type != ASF_Freetype )
				{/* don't want to try and load font as X11 unless requested to do so */
					font = open_X11_font( fontman, font_string );
				}else
					freetype = True ;
				if( font != NULL )
				{
					if( freetype )
					{
						font->name = ff_name ;
						ff_name = NULL ;
					}else
						font->name = mystrdup( font_string );
					add_hash_item( fontman->fonts_hash, AS_HASHABLE((char*)font->name), font);
				}
			}
			if( ff_name != NULL )
				free( ff_name );
		}

		if( font == NULL )
			font = hdata.vptr ;

		if( font )
			font->ref_count++ ;
	}
	return font;
}

ASFont*
dup_asfont( ASFont *font )
{
	if( font && font->fontman )
		font->ref_count++ ;
	else
		font = NULL;
	return font;
}

int
release_font( ASFont *font )
{
	int res = -1 ;
	if( font )
	{
		if( font->magic == MAGIC_ASFONT )
		{
			if( --(font->ref_count) < 0 )
			{
				ASFontManager *fontman = font->fontman ;
				if( fontman )
					remove_hash_item(fontman->fonts_hash, (ASHashableValue)(char*)font->name, NULL, True);
			}else
				res = font->ref_count ;
		}
	}
	return res ;
}

static inline void
free_glyph_data( register ASGlyph *asg )
{
    if( asg->pixmap )
        free( asg->pixmap );
/*fprintf( stderr, "\t\t%p\n", asg->pixmap );*/
    asg->pixmap = NULL ;
}

static void
destroy_glyph_range( ASGlyphRange **pgr )
{
	ASGlyphRange *gr = *pgr;
	if( gr )
	{
		*pgr = gr->above ;
        if( gr->below )
			gr->below->above = gr->above ;
        if( gr->above )
			gr->above->below = gr->below ;
        if( gr->glyphs )
		{
            int max_i = ((int)gr->max_char-(int)gr->min_char)+1 ;
            register int i = -1 ;
/*fprintf( stderr, " max_char = %d, min_char = %d, i = %d", gr->max_char, gr->min_char, max_i);*/
            while( ++i < max_i )
            {
/*fprintf( stderr, "%d >", i );*/
				free_glyph_data( &(gr->glyphs[i]) );
            }
            free( gr->glyphs );
			gr->glyphs = NULL ;
		}
		free( gr );
	}
}

static void
destroy_font( ASFont *font )
{
	if( font )
	{
#ifdef HAVE_FREETYPE
        if( font->type == ASF_Freetype && font->ft_face )
			FT_Done_Face(font->ft_face);
#endif
        if( font->name )
			free( font->name );
        while( font->codemap )
			destroy_glyph_range( &(font->codemap) );
        free_glyph_data( &(font->default_glyph) );
        if( font->locale_glyphs )
			destroy_ashash( &(font->locale_glyphs) );
        font->magic = 0 ;
		free( font );
	}
}

void
asglyph_destroy (ASHashableValue value, void *data)
{
	if( data )
	{
		free_glyph_data( (ASGlyph*)data );
		free( data );
	}
}

void
asfont_destroy (ASHashableValue value, void *data)
{
	if( data )
	{
	    char* cval = (char*)value ;
        if( ((ASMagic*)data)->magic == MAGIC_ASFONT )
        {
            if( cval == ((ASFont*)data)->name )
                cval = NULL ;          /* name is freed as part of destroy_font */
/*              fprintf( stderr,"freeing font \"%s\"...", (char*) value ); */
              destroy_font( (ASFont*)data );
/*              fprintf( stderr,"   done.\n"); */
        }
        if( cval )
            free( cval );
    }
}

/*************************************************************************/
/* Low level bitmap handling - compression and antialiasing              */
/*************************************************************************/

static unsigned char *
compress_glyph_pixmap( unsigned char *src, unsigned char *buffer,
                       unsigned int width, unsigned int height,
					   int src_step )
{
	unsigned char *pixmap ;
	register unsigned char *dst = buffer ;
	register int k = 0, i = 0 ;
	int count = -1;
	unsigned char last = src[0];
/* new way: if its FF we encode it as 01rrrrrr where rrrrrr is repitition count
 * if its 0 we encode it as 00rrrrrr. Any other symbol we bitshift right by 1
 * and then store it as 1sssssss where sssssss are topmost sugnificant bits.
 * Note  - single FFs and 00s are encoded as any other character. Its been noted
 * that in 99% of cases 0 or FF repeats, and very seldom anything else repeats
 */
	while ( height)
	{
		if( src[k] != last || (last != 0 && last != 0xFF) || count >= 63 )
		{
			if( count == 0 )
				dst[i++] = (last>>1)|0x80;
			else if( count > 0 )
			{
				if( last == 0xFF )
					count |= 0x40 ;
				dst[i++] = count;
				count = 0 ;
			}else
				count = 0 ;
			last = src[k] ;
		}else
		 	count++ ;
/*fprintf( stderr, "%2.2X ", src[k] ); */
		if( ++k >= (int)width )
		{
/*			fputc( '\n', stderr ); */
			--height ;
			k = 0 ;
			src += src_step ;
		}
	}
	if( count == 0 )
		dst[i++] = (last>>1)|0x80;
	else
	{
		if( last == 0xFF )
			count |= 0x40 ;
		dst[i++] = count;
	}
    pixmap  = safemalloc( i/*+(32-(i&0x01F) )*/);
/*fprintf( stderr, "pixmap alloced %p size %d(%d)", pixmap, i, i+(32-(i&0x01F) )); */
	memcpy( pixmap, buffer, i );

#if 0
/*  Old way : very simple RLE compression - should work well on glyphs */
 /* we reducing value range to 7bit - quite sufficient even for advanced
  * antialiasing. If topmost bit is 1 - then data is direct, otherwise
  * second byte is repitition count. Can't go with plain run length -
  * antialiasing introduces lots of randomization into pixel values,
  * so single count bytes are quite common, as the result we get memory
  * usage increase - not decrease. Thus we'll be employing more
  * complicated scheme described above.
  */
	if( hpad == 0 )
	{
		dst[0] = src[0]>>1 ;
		dst[1] = 0 ;
		k = 1 ;
	}else
	{
		dst[0] = 0 ;
		dst[1] = hpad-1 ;
	}
	for( i = 0 ; i < height ; i++ )
	{
		for( ; k < width  ; ++k )
		{
			unsigned char c = src[k]>>1;
			if( c == dst[0] && dst[1] < 127 )
				++dst[1];
			else
			{
				if( dst[1] == 0 )
					dst[0] |= 0x80;
				else
					++dst ;
				++dst ;
				dst[0] = c;
				dst[1] = 0 ;
			}
		}
		src += src_step ;
		if( hpad > 0 )
		{
			if( dst[0] == 0 && dst[1]+hpad < 127)
				dst[1] += hpad ;
			else
			{
				++dst ; ++dst ;
				dst[0] = 0 ;
				dst[1] = hpad-1 ;
			}
		}
		k = 0 ;
	}
	if( dst[1] > 0 )
	{
		++dst ; ++dst ;
	}
	pixmap  = safemalloc( dst - buffer);
/*	memcpy( pixmap, buffer, dst-buffer ); */
#endif
	return pixmap;
}

#ifdef DO_X11_ANTIALIASING
void
antialias_glyph( unsigned char *buffer, unsigned int width, unsigned int height )
{
	unsigned char *row1, *row2 ;
	register unsigned char *row ;
	register int x;
	int y;

	row1 = &(buffer[0]);
	row = &(buffer[width]);
	row2 = &(buffer[width+width]);
	for( x = 1 ; x < (int)width-1 ; x++ )
		if( row1[x] == 0 )
		{/* antialiasing here : */
			unsigned int c = (unsigned int)row[x]+
							(unsigned int)row1[x-1]+
							(unsigned int)row1[x+1];
			if( c >= 0x01FE )  /* we cut off secondary aliases */
				row1[x] = c>>2;
		}
	for( y = 1 ; y < (int)height-1 ; y++ )
	{
		if( row[0] == 0 )
		{/* antialiasing here : */
			unsigned int c = (unsigned int)row1[0]+
							(unsigned int)row[1]+
							(unsigned int)row2[0];
			if( c >= 0x01FE )  /* we cut off secondary aliases */
				row[0] = c>>2;
		}
		for( x = 1 ; x < (int)width-1 ; x++ )
		{
			if( row[x] == 0 )
			{/* antialiasing here : */
				unsigned int c = (unsigned int)row1[x]+
								(unsigned int)row[x-1]+
								(unsigned int)row[x+1]+
								(unsigned int)row2[x];
				if( row1[x] != 0 && row[x-1] != 0 && row[x+1] != 0 && row2[x] != 0 &&
					c >= 0x01FE )
					row[x] = c>>3;
				else if( c >= 0x01FE )  /* we cut off secondary aliases */
					row[x] = c>>2;
			}
		}
		if( row[x] == 0 )
		{/* antialiasing here : */
			unsigned int c = (unsigned int)row1[x]+
							(unsigned int)row[x-1]+
							(unsigned int)row2[x];
			if( c >= 0x01FE )  /* we cut off secondary aliases */
				row[x] = c>>2;
		}
		row  += width ;
		row1 += width ;
		row2 += width ;
	}
	for( x = 1 ; x < (int)width-1 ; x++ )
		if( row[x] == 0 )
		{/* antialiasing here : */
			unsigned int c = (unsigned int)row1[x]+
							(unsigned int)row[x-1]+
							(unsigned int)row[x+1];
			if( c >= 0x01FE )  /* we cut off secondary aliases */
				row[x] = c>>2;
		}
#ifdef DO_2STEP_X11_ANTIALIASING
	if( height  > X11_2STEP_AA_HEIGHT_THRESHOLD )
	{
		row1 = &(buffer[0]);
		row = &(buffer[width]);
		row2 = &(buffer[width+width]);
		for( y = 1 ; y < (int)height-1 ; y++ )
		{
			for( x = 1 ; x < (int)width-1 ; x++ )
			{
				if( row[x] == 0 )
				{/* antialiasing here : */
					unsigned int c = (unsigned int)row1[x]+
									(unsigned int)row[x-1]+
									(unsigned int)row[x+1]+
									(unsigned int)row2[x];
					if( row1[x] != 0 && row[x-1] != 0 && row[x+1] != 0 && row2[x] != 0
						&& c >= 0x00FF+0x007F)
						row[x] = c>>3;
					else if( (c >= 0x00FF+0x007F)|| c == 0x00FE )  /* we cut off secondary aliases */
						row[x] = c>>2;
				}
			}
			row  += width ;
			row1 += width ;
			row2 += width ;
		}
	}
#endif
#ifdef DO_3STEP_X11_ANTIALIASING
	if( height  > X11_3STEP_AA_HEIGHT_THRESHOLD )
	{
		row1 = &(buffer[0]);
		row = &(buffer[width]);
		row2 = &(buffer[width+width]);
		for( y = 1 ; y < (int)height-1 ; y++ )
		{
			for( x = 1 ; x < (int)width-1 ; x++ )
			{
				if( row[x] == 0xFF )
				{/* antialiasing here : */
					if( row1[x] < 0xFE || row2[x] < 0xFE )
						if( row[x+1] < 0xFE || row[x-1] < 0xFE )
							row[x] = 0xFE;
				}
			}
			row  += width ;
			row1 += width ;
			row2 += width ;
		}
		row = &(buffer[width]);
		for( y = 1 ; y < (int)height-1 ; y++ )
		{
			for( x = 1 ; x < (int)width-1 ; x++ )
				if( row[x] == 0xFE )
					row[x] = 0xBF ;
			row  += width ;
		}
	}

#endif
}
#endif

/*********************************************************************************/
/* encoding/locale handling						   								 */
/*********************************************************************************/

/* Now, this is the mess, I know :
 * Internally we store everything in current locale;
 * WE then need to convert it into Unicode 4 byte codes
 *
 * TODO: think about incoming data - does it has to be made local friendly ???
 * Definately
 */

#ifndef X_DISPLAY_MISSING
static ASGlyphRange*
split_X11_glyph_range( unsigned int min_char, unsigned int max_char, XCharStruct *chars )
{
	ASGlyphRange *first = NULL, **r = &first;
    int c = 0, delta = (max_char-min_char)+1;
LOCAL_DEBUG_CALLER_OUT( "min_char = %u, max_char = %u, chars = %p", min_char, max_char, chars );
	while( c < delta )
	{
		while( c < delta && chars[c].width == 0 ) ++c;

		if( c < delta )
		{
			*r = safecalloc( 1, sizeof(ASGlyphRange));
			(*r)->min_char = c+min_char ;
			while( c < delta && chars[c].width  != 0 ) ++c ;
			(*r)->max_char = (c-1)+min_char;
LOCAL_DEBUG_OUT( "created glyph range from %lu to %lu", (*r)->min_char, (*r)->max_char );
			r = &((*r)->above);
		}
	}
	return first;
}

void
load_X11_glyph_range( Display *dpy, ASFont *font, XFontStruct *xfs, size_t char_offset,
													  unsigned char byte1,
                                                      unsigned char min_byte2,
													  unsigned char max_byte2, GC *gc )
{
	ASGlyphRange  *all, *r ;
	unsigned long  min_char = (byte1<<8)|min_byte2;
	unsigned char *buffer, *compressed_buf ;
	unsigned int   height = xfs->ascent+xfs->descent ;
	static XGCValues gcv;

	buffer = safemalloc( xfs->max_bounds.width*height*2);
	compressed_buf = safemalloc( xfs->max_bounds.width*height*4);
	all = split_X11_glyph_range( min_char, (byte1<<8)|max_byte2, &(xfs->per_char[char_offset]));
	for( r = all ; r != NULL ; r = r->above )
	{
		XCharStruct *chars = &(xfs->per_char[char_offset+r->min_char-min_char]);
        int len = ((int)r->max_char-(int)r->min_char)+1;
		unsigned char char_base = r->min_char&0x00FF;
		register int i ;
		Pixmap p;
		XImage *xim;
		unsigned int total_width = 0 ;
		int pen_x = 0;
LOCAL_DEBUG_OUT( "loading glyph range of %lu-%lu", r->min_char, r->max_char );
		r->glyphs = safecalloc( len, sizeof(ASGlyph) );
		for( i = 0 ; i < len ; i++ )
		{
			int w = chars[i].rbearing - chars[i].lbearing ;
			r->glyphs[i].lead = chars[i].lbearing ;
			r->glyphs[i].width = MAX(w,(int)chars[i].width) ;
			r->glyphs[i].step = chars[i].width;
			total_width += r->glyphs[i].width ;
			if( chars[i].lbearing > 0 )
				total_width += chars[i].lbearing ;
		}
		p = XCreatePixmap( dpy, DefaultRootWindow(dpy), total_width, height, 1 );
		if( *gc == NULL )
		{
			gcv.font = xfs->fid;
			gcv.foreground = 1;
			*gc = XCreateGC( dpy, p, GCFont|GCForeground, &gcv);
		}else
			XSetForeground( dpy, *gc, 1 );
		XFillRectangle( dpy, p, *gc, 0, 0, total_width, height );
		XSetForeground( dpy, *gc, 0 );

		for( i = 0 ; i < len ; i++ )
		{
			XChar2b test_char ;
			int offset = MIN(0,(int)chars[i].lbearing);

			test_char.byte1 = byte1 ;
			test_char.byte2 = char_base+i ;
			/* we cannot draw string at once since in some fonts charcters may
			 * overlap each other : */
			XDrawImageString16( dpy, p, *gc, pen_x-offset, xfs->ascent, &test_char, 1 );
			pen_x += r->glyphs[i].width ;
			if( chars[i].lbearing > 0 )
				pen_x += chars[i].lbearing ;
		}
		/*XDrawImageString( dpy, p, *gc, 0, xfs->ascent, test_str_char, len );*/
		xim = XGetImage( dpy, p, 0, 0, total_width, height, 0xFFFFFFFF, ZPixmap );
		XFreePixmap( dpy, p );
		pen_x = 0 ;
		for( i = 0 ; i < len ; i++ )
		{
			register int x, y ;
			int width = r->glyphs[i].width;
			unsigned char *row = &(buffer[0]);

			if( chars[i].lbearing > 0 )
				pen_x += chars[i].lbearing ;
			for( y = 0 ; y < height ; y++ )
			{
				for( x = 0 ; x < width ; x++ )
				{
/*					fprintf( stderr, "glyph %d (%c): (%d,%d) 0x%X\n", i, (char)(i+r->min_char), x, y, XGetPixel( xim, pen_x+x, y ));*/
					/* remember default GC colors are black on white - 0 on 1 - and we need
					* quite the opposite - 0xFF on 0x00 */
					row[x] = ( XGetPixel( xim, pen_x+x, y ) != 0 )? 0x00:0xFF;
				}
				row += width;
			}

#ifdef DO_X11_ANTIALIASING
			if( height > X11_AA_HEIGHT_THRESHOLD )
				antialias_glyph( buffer, width, height );
#endif
			r->glyphs[i].pixmap = compress_glyph_pixmap( buffer, compressed_buf, width, height, width );
			r->glyphs[i].height = height ;
			r->glyphs[i].ascend = xfs->ascent ;
			r->glyphs[i].descend = xfs->descent ;
LOCAL_DEBUG_OUT( "glyph %u(range %lu-%lu) (%c) is %dx%d ascend = %d, lead = %d",  i, r->min_char, r->max_char, (char)(i+r->min_char), r->glyphs[i].width, r->glyphs[i].height, r->glyphs[i].ascend, r->glyphs[i].lead );
			pen_x += width ;
		}
		if( xim )
			XDestroyImage( xim );
	}
LOCAL_DEBUG_OUT( "done loading glyphs. Attaching set of glyph ranges to the codemap...%s", "" );
	if( all != NULL )
	{
		if( font->codemap == NULL )
			font->codemap = all ;
		else
		{
			for( r = font->codemap ; r != NULL ; r = r->above )
			{
				if( r->min_char > all->min_char )
				{
					if( r->below )
						r->below->above = all ;
					r->below = all ;
					while ( all->above != NULL )
						all = all->above ;
					all->above = r ;
					r->below = all ;
					break;
				}
				all->below = r ;
			}
			if( r == NULL && all->below->above == NULL )
				all->below->above = all ;
		}
	}
	free( buffer ) ;
	free( compressed_buf ) ;
LOCAL_DEBUG_OUT( "all don%s", "" );
}


void
make_X11_default_glyph( ASFont *font, XFontStruct *xfs )
{
	unsigned char *buf, *compressed_buf ;
	int width, height ;
	int x, y;
	unsigned char *row ;


	height = xfs->ascent+xfs->descent ;
	width = xfs->max_bounds.width ;

	if( height <= 0 ) height = 4;
	if( width <= 0 ) width = 4;
	buf = safecalloc( height*width, sizeof(unsigned char) );
	compressed_buf = safemalloc( height*width*2 );
	row = buf;
	for( x = 0 ; x < width ; ++x )
		row[x] = 0xFF;
	for( y = 1 ; y < height-1 ; ++y )
	{
		row += width ;
		row[0] = 0xFF ; row[width-1] = 0xFF ;
	}
	for( x = 0 ; x < width ; ++x )
		row[x] = 0xFF;
	font->default_glyph.pixmap = compress_glyph_pixmap( buf, compressed_buf, width, height, width );
	font->default_glyph.width = width ;
	font->default_glyph.step = width ;
	font->default_glyph.height = height ;
	font->default_glyph.lead = 0 ;
	font->default_glyph.ascend = xfs->ascent ;
	font->default_glyph.descend = xfs->descent ;

	free( buf ) ;
	free( compressed_buf ) ;
}

static int
load_X11_glyphs( Display *dpy, ASFont *font, XFontStruct *xfs )
{
	GC gc = NULL;
#if 0 /*I18N*/
	if( xfs->max_byte1 > 0 && xfs->min_byte1 > 0 )
	{

		char_num *= rows ;
	}else
	{
		int i;
		int min_byte1 = (xfs->min_char_or_byte2>>8)&0x00FF;
		int max_byte1 = (xfs->max_char_or_byte2>>8)&0x00FF;
        size_t offset = MAX(0x00FF,(int)xfs->max_char_or_byte2-(int)(min_byte1<<8)) ;

		load_X11_glyph_range( dpy, font, xfs, 0, min_byte1,
											xfs->min_char_or_byte2-(min_byte1<<8),
			                                offset, &gc );
		offset -= xfs->min_char_or_byte2-(min_byte1<<8);
		if( max_byte1 > min_byte1 )
		{
			for( i = min_byte1+1; i < max_byte1 ; i++ )
			{
				load_X11_glyph_range( dpy, font, xfs, offset, i, 0x00, 0xFF, &gc );
				offset += 256 ;
			}
			load_X11_glyph_range( dpy, font, xfs, offset, max_byte1,
				                                     0,
                                                     (int)xfs->max_char_or_byte2-(int)(max_byte1<<8), &gc );
		}
	}
#else
	{
		/* we blame X consortium for the following mess : */
		int min_char, max_char, our_min_char = 0x0021, our_max_char = 0x00FF ;
		int byte1 = xfs->min_byte1;
		if( xfs->min_byte1 > 0 )
		{
			min_char = xfs->min_char_or_byte2 ;
			max_char = xfs->max_char_or_byte2 ;
			if( min_char > 0x00FF )
			{
				byte1 = (min_char>>8)&0x00FF;
				min_char &=  0x00FF;
				if( ((max_char>>8)&0x00FF) > byte1 )
					max_char =  0x00FF;
				else
					max_char &= 0x00FF;
			}
		}else
		{
			min_char = ((xfs->min_byte1<<8)&0x00FF00)|(xfs->min_char_or_byte2&0x00FF);
			max_char = ((xfs->min_byte1<<8)&0x00FF00)|(xfs->max_char_or_byte2&0x00FF);
			our_min_char |= ((xfs->min_byte1<<8)&0x00FF00) ;
			our_max_char |= ((xfs->min_byte1<<8)&0x00FF00) ;
		}
		our_min_char = MAX(our_min_char,min_char);
		our_max_char = MIN(our_max_char,max_char);

        load_X11_glyph_range( dpy, font, xfs, (int)our_min_char-(int)min_char, byte1, our_min_char&0x00FF, our_max_char&0x00FF, &gc );
	}
#endif
	if( font->default_glyph.pixmap == NULL )
		make_X11_default_glyph( font, xfs );
	font->max_height = xfs->ascent+xfs->descent;
	font->max_ascend = xfs->ascent;
	font->max_descend = xfs->descent;
	font->space_size = xfs->max_bounds.width*2/3 ;
	font->pen_move_dir = LEFT_TO_RIGHT;
	if( gc )
		XFreeGC( dpy, gc );
	return xfs->ascent+xfs->descent;
}
#endif /* #ifndef X_DISPLAY_MISSING */

#ifdef HAVE_FREETYPE
static void
load_glyph_freetype( ASFont *font, ASGlyph *asg, int glyph )
{
	register FT_Face face = font->ft_face;
	if( !FT_Load_Glyph( face, glyph, FT_LOAD_DEFAULT ) )
		if( !FT_Render_Glyph( face->glyph, ft_render_mode_normal ) )
			if( face->glyph->bitmap.buffer )
			{
				FT_Bitmap 	*bmap = &(face->glyph->bitmap) ;
				register CARD8 *buf, *src = bmap->buffer ;
/* 				int hpad = (face->glyph->bitmap_left<0)? -face->glyph->bitmap_left: face->glyph->bitmap_left ;
*/
				asg->width   = bmap->width ;
				asg->height  = bmap->rows ;
				asg->step = bmap->width+face->glyph->bitmap_left ;

				if( bmap->pitch < 0 )
					src += -bmap->pitch*bmap->rows ;
				buf = safemalloc( asg->width*asg->height*3);
				/* we better do some RLE encoding in attempt to preserv memory */
				asg->pixmap  = compress_glyph_pixmap( src, buf, bmap->width, bmap->rows, bmap->pitch );
				free( buf );
				asg->ascend  = face->glyph->bitmap_top;
				asg->descend = bmap->rows - asg->ascend;
				/* we only want to keep lead if it was negative */
				asg->lead    = face->glyph->bitmap_left;
	LOCAL_DEBUG_OUT( "glyph %p with FT index %u is %dx%d ascend = %d, lead = %d, bmap_top = %d", asg, glyph, asg->width, asg->height, asg->ascend, asg->lead, face->glyph->bitmap_top );
			}
}

static ASGlyphRange*
split_freetype_glyph_range( unsigned long min_char, unsigned long max_char, FT_Face face )
{
	ASGlyphRange *first = NULL, **r = &first;
LOCAL_DEBUG_CALLER_OUT( "min_char = %lu, max_char = %lu, face = %p", min_char, max_char, face );
	while( min_char <= max_char )
	{
		register long i = min_char;
		while( i <= max_char && FT_Get_Char_Index( face, CHAR2UNICODE(i)) == 0 ) i++ ;
		if( i <= max_char )
		{
			*r = safecalloc( 1, sizeof(ASGlyphRange));
			(*r)->min_char = i ;
			while( i <= max_char && FT_Get_Char_Index( face, CHAR2UNICODE(i)) != 0 ) i++ ;
			(*r)->max_char = i ;
LOCAL_DEBUG_OUT( "created glyph range from %lu to %lu", (*r)->min_char, (*r)->max_char );
			r = &((*r)->above);
		}
		min_char = i ;
	}
	return first;
}

static ASGlyph*
load_freetype_locale_glyph( ASFont *font, UNICODE_CHAR uc )
{
	ASGlyph *asg = NULL ;
	if( FT_Get_Char_Index( font->ft_face, uc) != 0 )
	{
		asg = safecalloc( 1, sizeof(ASGlyph));
		load_glyph_freetype( font, asg, FT_Get_Char_Index( font->ft_face, uc));
		if( add_hash_item( font->locale_glyphs, AS_HASHABLE(uc), asg ) != ASH_Success )
		{
			LOCAL_DEBUG_OUT( "Failed to add glyph %p for char %d to hash", asg, uc );
			asglyph_destroy( 0, asg);
			asg = NULL ;
		}else
		{
			LOCAL_DEBUG_OUT( "added glyph %p for char %d to hash font attr(%d,%d,%d) glyph attr (%d,%d)", asg, uc, font->max_ascend, font->max_descend, font->max_height, asg->ascend, asg->descend );

			if( asg->ascend > font->max_ascend )
				font->max_ascend = asg->ascend ;
			if( asg->descend > font->max_descend )
				font->max_descend = asg->descend ;
			font->max_height = font->max_ascend+font->max_descend ;
			LOCAL_DEBUG_OUT( "font attr(%d,%d,%d) glyph attr (%d,%d)", font->max_ascend, font->max_descend, font->max_height, asg->ascend, asg->descend );
		}
	}else
		add_hash_item( font->locale_glyphs, AS_HASHABLE(uc), NULL );
	return asg;
}

static void
load_freetype_locale_glyphs( unsigned long min_char, unsigned long max_char, ASFont *font )
{
	register int i = min_char ;
LOCAL_DEBUG_CALLER_OUT( "min_char = %lu, max_char = %lu, font = %p", min_char, max_char, font );
	if( font->locale_glyphs == NULL )
		font->locale_glyphs = create_ashash( 0, NULL, NULL, asglyph_destroy );
	while( i <= max_char )
	{
		load_freetype_locale_glyph( font, CHAR2UNICODE(i));
		++i;
	}
	LOCAL_DEBUG_OUT( "font attr(%d,%d,%d)", font->max_ascend, font->max_descend, font->max_height );
}


static int
load_freetype_glyphs( ASFont *font )
{
	int max_ascend = 0, max_descend = 0;
	unsigned long i ;
	ASGlyphRange *r ;

    /* we preload only codes in range 0x21-0xFF in current charset */
	/* if draw_unicode_text is used and we need some other glyphs
	 * we'll just need to add them on demand */
	font->codemap = split_freetype_glyph_range( 0x0021, 0x007F, font->ft_face );
	font->pen_move_dir = LEFT_TO_RIGHT ;

	load_glyph_freetype( font, &(font->default_glyph), 0);/* special no-symbol glyph */
    load_freetype_locale_glyphs( 0x0080, 0x00FF, font );
	if( font->codemap == NULL )
	{
		font->max_height = font->default_glyph.ascend+font->default_glyph.descend;
		if( font->max_height <= 0 )
			font->max_height = 1 ;
		font->max_ascend = MAX((int)font->default_glyph.ascend,1);
		font->max_descend = MAX((int)font->default_glyph.descend,1);
	}else
	{
		for( r = font->codemap ; r != NULL ; r = r->above )
		{
			int min_char = r->min_char ;
			int max_char = r->max_char ;

            r->glyphs = safecalloc( ((int)max_char - (int)min_char) + 1, sizeof(ASGlyph));
			for( i = min_char ; i < max_char ; ++i )
			{
				if( i != ' ' && i != '\t' && i!= '\n' )
				{
					ASGlyph *asg = &(r->glyphs[i-min_char]);
					load_glyph_freetype( font, asg, FT_Get_Char_Index( font->ft_face, CHAR2UNICODE(i)));
					if( asg->lead >= 0 || asg->lead+asg->width > 3 )
						font->pen_move_dir = LEFT_TO_RIGHT ;
					if( asg->ascend > max_ascend )
						max_ascend = asg->ascend ;
					if( asg->descend > max_descend )
						max_descend = asg->descend ;
				}
			}
		}
		if( (int)font->max_ascend <= max_ascend )
			font->max_ascend = MAX(max_ascend,1);
		if( (int)font->max_descend <= max_descend )
			font->max_descend = MAX(max_descend,1);
	 	font->max_height = font->max_ascend+font->max_descend;
	}
	return max_ascend+max_descend;
}
#endif

inline ASGlyph *get_unicode_glyph( const UNICODE_CHAR uc, ASFont *font )
{
	register ASGlyphRange *r;
	ASGlyph *asg = NULL ;
	ASHashData hdata = {0} ;
	for( r = font->codemap ; r != NULL ; r = r->above )
	{
LOCAL_DEBUG_OUT( "looking for glyph for char %lu (%p) if range (%d,%d)", uc, asg, r->min_char, r->max_char);

		if( r->max_char >= uc )
			if( r->min_char <= uc )
			{
				asg = &(r->glyphs[uc - r->min_char]);
LOCAL_DEBUG_OUT( "Found glyph for char %lu (%p)", uc, asg );
				if( asg->width > 0 && asg->pixmap != NULL )
					return asg;
				break;
			}
	}
	if( get_hash_item( font->locale_glyphs, AS_HASHABLE(uc), &hdata.vptr ) != ASH_Success )
	{
#ifdef HAVE_FREETYPE
		asg = load_freetype_locale_glyph( font, uc );
#endif
	}else
		asg = hdata.vptr ;
LOCAL_DEBUG_OUT( "%sFound glyph for char %lu ( %p )", asg?"":"Did not ", uc, asg );
	return asg?asg:&(font->default_glyph) ;
}


inline ASGlyph *get_character_glyph( const unsigned char c, ASFont *font )
{
	return get_unicode_glyph( CHAR2UNICODE(c), font );
}

static UNICODE_CHAR
utf8_to_unicode ( const unsigned char *s )
{
	unsigned char c = s[0];

	if (c < 0x80)
	{
  		return (UNICODE_CHAR)c;
	} else if (c < 0xc2)
	{
  		return 0;
    } else if (c < 0xe0)
	{
	    if (!((s[1] ^ 0x80) < 0x40))
    		return 0;
	    return ((UNICODE_CHAR) (c & 0x1f) << 6)
  		       |(UNICODE_CHAR) (s[1] ^ 0x80);
    } else if (c < 0xf0)
	{
	    if (!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
  		      && (c >= 0xe1 || s[1] >= 0xa0)))
	        return 0;
		return ((UNICODE_CHAR) (c & 0x0f) << 12)
        	 | ((UNICODE_CHAR) (s[1] ^ 0x80) << 6)
          	 |  (UNICODE_CHAR) (s[2] ^ 0x80);
	} else if (c < 0xf8 && sizeof(UNICODE_CHAR)*8 >= 32)
	{
	    if (!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
  	        && (s[3] ^ 0x80) < 0x40
    	    && (c >= 0xf1 || s[1] >= 0x90)))
    		return 0;
	    return ((UNICODE_CHAR) (c & 0x07) << 18)
             | ((UNICODE_CHAR) (s[1] ^ 0x80) << 12)
	         | ((UNICODE_CHAR) (s[2] ^ 0x80) << 6)
  	         |  (UNICODE_CHAR) (s[3] ^ 0x80);
	} else if (c < 0xfc && sizeof(UNICODE_CHAR)*8 >= 32)
	{
	    if (!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
  	        && (s[3] ^ 0x80) < 0x40 && (s[4] ^ 0x80) < 0x40
    	    && (c >= 0xf9 || s[1] >= 0x88)))
	        return 0;
		return ((UNICODE_CHAR) (c & 0x03) << 24)
             | ((UNICODE_CHAR) (s[1] ^ 0x80) << 18)
	         | ((UNICODE_CHAR) (s[2] ^ 0x80) << 12)
  	         | ((UNICODE_CHAR) (s[3] ^ 0x80) << 6)
    	     |  (UNICODE_CHAR) (s[4] ^ 0x80);
	} else if (c < 0xfe && sizeof(UNICODE_CHAR)*8 >= 32)
	{
	    if (!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
  		    && (s[3] ^ 0x80) < 0x40 && (s[4] ^ 0x80) < 0x40
      	    && (s[5] ^ 0x80) < 0x40
        	&& (c >= 0xfd || s[1] >= 0x84)))
	  		return 0;
		return ((UNICODE_CHAR) (c & 0x01) << 30)
      	     | ((UNICODE_CHAR) (s[1] ^ 0x80) << 24)
        	 | ((UNICODE_CHAR) (s[2] ^ 0x80) << 18)
             | ((UNICODE_CHAR) (s[3] ^ 0x80) << 12)
	         | ((UNICODE_CHAR) (s[4] ^ 0x80) << 6)
  	    	 |  (UNICODE_CHAR) (s[5] ^ 0x80);
    }
    return 0;
}

inline ASGlyph *get_utf8_glyph( const char *utf8, ASFont *font )
{
	UNICODE_CHAR uc = utf8_to_unicode ( utf8 );
	return get_unicode_glyph( uc, font );
}

/*********************************************************************************/
/* actuall rendering code :						   								 */
/*********************************************************************************/

typedef struct ASGlyphMap
{
	unsigned int  height, width ;
#define GLYPH_TAB	((ASGlyph*)0x00000003)
#define GLYPH_SPACE	((ASGlyph*)0x00000002)
#define GLYPH_EOL	((ASGlyph*)0x00000001)
#define GLYPH_EOT	((ASGlyph*)0x00000000)
	ASGlyph 	**glyphs;
	int 		  glyphs_num;
}ASGlyphMap;


static void
apply_text_3D_type( ASText3DType type,
                    unsigned int *width, unsigned int *height )
{
	switch( type )
	{
		case AST_Embossed   :
		case AST_Sunken     :
				(*width) += 2; (*height) += 2 ;
				break ;
		case AST_ShadeAbove :
		case AST_ShadeBelow :
				(*width)+=3; (*height)+=3 ;
				break ;
		case AST_SunkenThick :
		case AST_EmbossedThick :
				(*width)+=3; (*height)+=3 ;
				break ;
		default  :
				break ;
	}
}

#define FILL_TEXT_GLYPH_MAP(name,type,getglyph,incr) \
static unsigned int \
name( const type *text, ASFont *font, ASGlyphMap *map, int space_size, unsigned int offset_3d_x ) \
{ \
	unsigned int w = 0, line_count = 0, line_width = 0; \
	int i = -1, g = 0 ; \
	ASGlyph *last_asg = NULL ; \
	do \
	{ \
		++i ; \
		LOCAL_DEBUG_OUT("begin_i=%d, char_code = 0x%2.2X",i,text[i]); \
		if( text[i] == '\n' || text[i] == '\0' ) \
		{ \
			if( last_asg && last_asg->width+last_asg->lead > last_asg->step ) \
				line_width += last_asg->width+last_asg->lead - last_asg->step ; \
			last_asg = NULL; \
			if( line_width > w ) \
				w = line_width ; \
			line_width = 0 ; \
			++line_count ; \
			map->glyphs[g++] = (text[i] == '\0')?GLYPH_EOT:GLYPH_EOL; \
		}else \
		{ \
			last_asg = NULL ; \
			if( text[i] == ' ' ) \
			{ \
				line_width += space_size ; \
				map->glyphs[g++] = GLYPH_SPACE; \
			}else if( text[i] == '\t' ) \
			{ \
				line_width += space_size*8 ; \
				map->glyphs[g++] = GLYPH_TAB; \
			}else \
			{ \
				last_asg = getglyph; \
				map->glyphs[g++] = last_asg; \
				line_width += last_asg->step+offset_3d_x ; \
				LOCAL_DEBUG_OUT("pre_i=%d",i); \
				incr; /* i+=CHAR_SIZE(text[i])-1; */ \
				LOCAL_DEBUG_OUT("post_i=%d",i); \
			} \
		} \
	}while( text[i] != '\0' );  \
	map->width = MAX( w, (unsigned int)1 ); \
	return line_count ; \
}

FILL_TEXT_GLYPH_MAP(fill_text_glyph_map_Char,char,get_character_glyph(text[i],font),1)
FILL_TEXT_GLYPH_MAP(fill_text_glyph_map_UTF8,char,get_utf8_glyph(&text[i],font),i+=(UTF8_CHAR_SIZE(text[i])-1))
FILL_TEXT_GLYPH_MAP(fill_text_glyph_map_Unicode,UNICODE_CHAR,get_unicode_glyph(text[i],font),1)

typedef enum {
  ASCT_UTF8 = 0,
  ASCT_Char	= 1,
  ASCT_Unicode=sizeof(UNICODE_CHAR)
}ASCharType;

void
free_glyph_map( ASGlyphMap *map, Bool reusable )
{
    if( map )
    {
        free( map->glyphs );
        if( !reusable )
            free( map );
    }
}

static Bool
get_text_glyph_map( const char *text, ASFont *font, ASText3DType type, ASGlyphMap *map, ASCharType char_type )
{
	unsigned int line_count = 0;
	unsigned int offset_3d_x = 0, offset_3d_y = 0 ;
	int space_size  = (font->space_size>>1)+1+font->spacing_x;

	apply_text_3D_type( type, &offset_3d_x, &offset_3d_y );

	if( text == NULL || font == NULL || map == NULL)
		return False;

	offset_3d_x += font->spacing_x ;
	offset_3d_y += font->spacing_y ;

	map->glyphs_num = 1;
	if( char_type == ASCT_Char )
	{
		register int count = 0 ;
		register char *ptr = (char*)text ;
		while( ptr[count] != 0 )++count;
		map->glyphs_num += count ;
	}else if( char_type == ASCT_UTF8 )
	{
		register int count = 0 ;
		register char *ptr = (char*)text ;
		while( *ptr != 0 ){	++count; ptr += UTF8_CHAR_SIZE(*ptr); }
		map->glyphs_num += count ;
	}else if( char_type == ASCT_Unicode )
	{
		register int count = 0 ;
		register UNICODE_CHAR *uc_ptr = (UNICODE_CHAR*)text ;
		while( uc_ptr[count] != 0 )	++count;
		map->glyphs_num += count ;
	}

	if( map->glyphs_num == 0 )
		return False;
	map->glyphs = safecalloc( map->glyphs_num, sizeof(ASGlyph*));

	if( char_type == ASCT_Char )
		line_count = fill_text_glyph_map_Char( text, font, map, space_size, offset_3d_x );
	else if( char_type == ASCT_UTF8 )
		line_count = fill_text_glyph_map_UTF8( text, font, map, space_size, offset_3d_x );
	else if( char_type == ASCT_Unicode )
		line_count = fill_text_glyph_map_Unicode( (UNICODE_CHAR*)text, font, map, space_size, offset_3d_x );

    map->height = line_count * (font->max_height+offset_3d_y) - font->spacing_y;

	if( map->height <= 0 )
		map->height = 1 ;

	return True;
}


#define GET_TEXT_SIZE_LOOP(getglyph,incr) \
	do{ ++i ;\
		if( text[i] == '\n' || text[i] == '\0' ) { \
			if( last_asg && last_asg->width+last_asg->lead > last_asg->step ) \
				line_width += last_asg->width+last_asg->lead - last_asg->step ; \
			last_asg = NULL; \
			if( line_width > w ) \
				w = line_width ; \
			line_width = 0 ; \
            ++line_count ; \
		}else { \
			last_asg = NULL ; \
			if( text[i] == ' ' ){ \
				line_width += space_size ; \
			}else if( text[i] == '\t' ){ \
				line_width += space_size*8 ; \
			}else{ \
				last_asg = getglyph; \
				line_width += last_asg->step+offset_3d_x ;  \
				incr ; \
			} \
		} \
	}while( text[i] != '\0' )

static Bool
get_text_size_localized( const char *src_text, ASFont *font, ASText3DType type, ASCharType char_type, unsigned int *width, unsigned int *height )
{
    unsigned int w = 0, h = 0, line_count = 0;
	unsigned int line_width = 0;
    int i = -1;
	ASGlyph *last_asg = NULL ;
	int space_size  = (font->space_size>>1)+1+font->spacing_x;
	unsigned int offset_3d_x = 0, offset_3d_y = 0 ;

	apply_text_3D_type( type, &offset_3d_x, &offset_3d_y );
	if( src_text == NULL || font == NULL )
		return False;

	offset_3d_x += font->spacing_x ;
	offset_3d_y += font->spacing_y ;

	if( char_type == ASCT_Char )
	{
		char *text = (char*)&src_text[0] ;
		GET_TEXT_SIZE_LOOP(get_character_glyph(text[i],font),1);
	}else if( char_type == ASCT_UTF8 )
	{
		char *text = (char*)&src_text[0] ;
		GET_TEXT_SIZE_LOOP(get_utf8_glyph(&text[i],font),i+=UTF8_CHAR_SIZE(text[i])-1);
	}else if( char_type == ASCT_Unicode )
	{
		UNICODE_CHAR *text = (UNICODE_CHAR*)&src_text[0] ;
		GET_TEXT_SIZE_LOOP(get_unicode_glyph(text[i],font),1);
	}

    h = line_count * (font->max_height+offset_3d_y) - font->spacing_y;

    if( w < 1 )
		w = 1 ;
	if( h < 1 )
		h = 1 ;
	if( width )
		*width = w;
	if( height )
		*height = h;
	return True ;
}

Bool
get_text_size( const char *src_text, ASFont *font, ASText3DType type, unsigned int *width, unsigned int *height )
{
	return get_text_size_localized( src_text, font, type, ASCT_Char, width, height );
}

Bool
get_unicode_text_size( const UNICODE_CHAR *src_text, ASFont *font, ASText3DType type, unsigned int *width, unsigned int *height )
{
	return get_text_size_localized( (char*)src_text, font, type, ASCT_Unicode, width, height );
}

Bool
get_utf8_text_size( const char *src_text, ASFont *font, ASText3DType type, unsigned int *width, unsigned int *height )
{
	return get_text_size_localized( src_text, font, type, ASCT_UTF8, width, height );
}


inline static void
render_asglyph( CARD32 **scanlines, CARD8 *row,
                int start_x, int y, int width, int height,
				CARD32 ratio )
{
	int count = -1 ;
	int max_y = y + height ;
	register CARD32 data = 0;
	while( y < max_y )
	{
		register CARD32 *dst = scanlines[y]+start_x;
		register int x = -1;
		while( ++x < width )
		{
/*fprintf( stderr, "data = %X, count = %d, x = %d, y = %d\n", data, count, x, y );*/
			if( count < 0 )
			{
				data = *(row++);
				if( (data&0x80) != 0)
				{
					data = ((data&0x7F)<<1);
					if( data != 0 )
						++data;
				}else
				{
					count = data&0x3F ;
					data = ((data&0x40) != 0 )? 0xFF: 0x00;
				}
				if( ratio != 0xFF && data != 0 )
					data = ((data*ratio)>>8)+1 ;
			}
			if( data >= ratio || data > dst[x] )
				dst[x] = data ;
			--count;
		}
		++y;
	}
}

static ASImage *
draw_text_localized( const char *text, ASFont *font, ASText3DType type, ASCharType char_type, int compression )
{
	ASGlyphMap map ;
	CARD32 *memory;
	CARD32 **scanlines ;
	int i = 0, offset = 0, line_height, space_size, base_line;
	ASImage *im;
	int pen_x = 0, pen_y = 0;
	unsigned int offset_3d_x = 0, offset_3d_y = 0  ;
#ifdef DO_CLOCKING
	time_t started = clock ();
#endif

LOCAL_DEBUG_CALLER_OUT( "text = \"%s\", font = %p, compression = %d", text, font, compression );
	if( !get_text_glyph_map( text, font, type, &map, char_type) )
		return NULL;

	apply_text_3D_type( type, &(offset_3d_x), &(offset_3d_y) );

	offset_3d_x += font->spacing_x ;
	offset_3d_y += font->spacing_y ;
	line_height = font->max_height+offset_3d_y ;

LOCAL_DEBUG_OUT( "text size = %dx%d pixels", map.width, map.height );
	im = create_asimage( map.width, map.height, compression );

	space_size  = (font->space_size>>1)+1+font->spacing_x;
	base_line = font->max_ascend;
LOCAL_DEBUG_OUT( "line_height is %d, space_size is %d, base_line is %d", line_height, space_size, base_line );
	scanlines = safemalloc( line_height*sizeof(CARD32*));
LOCAL_DEBUG_OUT( "scanline list memory allocated %d", line_height*sizeof(CARD32*) );
	memory = safecalloc( line_height,map.width*sizeof(CARD32));
LOCAL_DEBUG_OUT( "scanline buffer memory allocated %d", map.width*line_height*sizeof(CARD32) );
	do
	{
		scanlines[i] = memory + offset;
		offset += map.width;
	}while ( ++i < line_height );

	i = -1 ;
	if(font->pen_move_dir == RIGHT_TO_LEFT)
		pen_x = map.width ;

	do
	{
		++i;
/*fprintf( stderr, "drawing character %d '%c'\n", i, text[i] );*/
		if( map.glyphs[i] == GLYPH_EOL || map.glyphs[i] == GLYPH_EOT )
		{
			int y;
			for( y = 0 ; y < line_height ; ++y )
			{
				register int x = 0;
				register CARD32 *line = scanlines[y];
#if 1
#ifdef LOCAL_DEBUG
				x = 0;
				while( x < map.width )
					fprintf( stderr, "%2.2X ", scanlines[y][x++] );
				fprintf( stderr, "\n" );
#endif
#endif
 				asimage_add_line (im, IC_ALPHA, line, pen_y+y);
				for( x = 0; x < (int)map.width; ++x )
					line[x] = 0;
			}
			pen_x = (font->pen_move_dir == RIGHT_TO_LEFT)? map.width : 0;
			pen_y += line_height;
			if( pen_y <0 )
				pen_y = 0 ;
		}else
		{
			if( map.glyphs[i] == GLYPH_SPACE || map.glyphs[i] == GLYPH_TAB )
			{
				int d_pen = (map.glyphs[i] == GLYPH_TAB)?space_size*8 : space_size;
				if( font->pen_move_dir == RIGHT_TO_LEFT )
					pen_x -= d_pen ;
				else
					pen_x += d_pen ;
			}else
			{
				/* now comes the fun part : */
				ASGlyph *asg = map.glyphs[i] ;
				int y = base_line - asg->ascend;
				int start_x = 0;

				if( font->pen_move_dir == RIGHT_TO_LEFT )
					pen_x  -= asg->step+offset_3d_x ;
				if( asg->lead > 0 )
					start_x = pen_x + asg->lead ;
				else if( pen_x  > -asg->lead )
					start_x = pen_x + asg->lead ;
				if( start_x < 0 )
					start_x = 0 ;
				if( y < 0 )
					y = 0 ;

				switch( type )
				{
					case AST_Plain :
						render_asglyph( scanlines, asg->pixmap, start_x, y, asg->width, asg->height, 0x00FF );
					    break ;
					case AST_Embossed :
						render_asglyph( scanlines, asg->pixmap, start_x, y, asg->width, asg->height, 0x00FF );
						render_asglyph( scanlines, asg->pixmap, start_x+2, y+2, asg->width, asg->height, 0x009F );
						render_asglyph( scanlines, asg->pixmap, start_x+1, y+1, asg->width, asg->height, 0x00CF );
 					    break ;
					case AST_Sunken :
						render_asglyph( scanlines, asg->pixmap, start_x, y, asg->width, asg->height, 0x009F );
						render_asglyph( scanlines, asg->pixmap, start_x+2, y+2, asg->width, asg->height, 0x00FF );
						render_asglyph( scanlines, asg->pixmap, start_x+1, y+1, asg->width, asg->height, 0x00CF );
					    break ;
					case AST_ShadeAbove :
						render_asglyph( scanlines, asg->pixmap, start_x, y, asg->width, asg->height, 0x007F );
						render_asglyph( scanlines, asg->pixmap, start_x+3, y+3, asg->width, asg->height, 0x00FF );
					    break ;
					case AST_ShadeBelow :
						render_asglyph( scanlines, asg->pixmap, start_x+3, y+3, asg->width, asg->height, 0x007F );
						render_asglyph( scanlines, asg->pixmap, start_x, y, asg->width, asg->height, 0x00FF );
					    break ;
					case AST_EmbossedThick :
						render_asglyph( scanlines, asg->pixmap, start_x, y, asg->width, asg->height, 0x00FF );
						render_asglyph( scanlines, asg->pixmap, start_x+1, y+1, asg->width, asg->height, 0x00EF );
						render_asglyph( scanlines, asg->pixmap, start_x+3, y+3, asg->width, asg->height, 0x007F );
						render_asglyph( scanlines, asg->pixmap, start_x+2, y+2, asg->width, asg->height, 0x00CF );
 					    break ;
					case AST_SunkenThick :
						render_asglyph( scanlines, asg->pixmap, start_x, y, asg->width, asg->height, 0x007F );
						render_asglyph( scanlines, asg->pixmap, start_x+1, y+1, asg->width, asg->height, 0x00AF );
						render_asglyph( scanlines, asg->pixmap, start_x+3, y+3, asg->width, asg->height, 0x00FF );
						render_asglyph( scanlines, asg->pixmap, start_x+2, y+2, asg->width, asg->height, 0x00CF );
 					    break ;
				  default:
				        break ;
				}

				if( font->pen_move_dir == LEFT_TO_RIGHT )
  					pen_x  += asg->step+offset_3d_x;
			}
		}
	}while( map.glyphs[i] != GLYPH_EOT );
    free_glyph_map( &map, True );
	free( memory );
	free( scanlines );
#ifdef DO_CLOCKING
	fprintf (stderr, __FUNCTION__ " time (clocks): %lu mlsec\n", ((clock () - started)*100)/CLOCKS_PER_SEC);
#endif
	return im;
}

ASImage *
draw_text( const char *text, ASFont *font, ASText3DType type, int compression )
{
	return draw_text_localized( text, font, type, ASCT_Char, compression );
}

ASImage *
draw_unicode_text( const UNICODE_CHAR *text, ASFont *font, ASText3DType type, int compression )
{
	return draw_text_localized( (char*)text, font, type, ASCT_Unicode, compression );
}

ASImage *
draw_utf8_text( const char *text, ASFont *font, ASText3DType type, int compression )
{
	return draw_text_localized( text, font, type, ASCT_UTF8, compression );
}


Bool get_asfont_glyph_spacing( ASFont* font, int *x, int *y )
{
	if( font )
	{
		if( x )
			*x = font->spacing_x ;
		if( y )
			*y = font->spacing_y ;
		return True ;
	}
	return False ;
}

Bool set_asfont_glyph_spacing( ASFont* font, int x, int y )
{
	if( font )
	{
		font->spacing_x = (x < 0 )? 0: x;
		font->spacing_y = (y < 0 )? 0: y;
		return True ;
	}
	return False ;
}

/* Misc functions : */
void print_asfont( FILE* stream, ASFont* font)
{
	if( font )
	{
		fprintf( stream, "font.type = %d\n", font->type       );
#ifdef HAVE_FREETYPE
		fprintf( stream, "font.ft_face = %p\n", font->ft_face    );              /* free type font handle */
#endif
		fprintf( stream, "font.max_height = %d\n", font->max_height );
		fprintf( stream, "font.space_size = %d\n" , font->space_size );
		fprintf( stream, "font.spacing_x  = %d\n" , font->spacing_x );
		fprintf( stream, "font.spacing_y  = %d\n" , font->spacing_y );
		fprintf( stream, "font.max_ascend = %d\n", font->max_ascend );
		fprintf( stream, "font.max_descend = %d\n", font->max_descend );
		fprintf( stream, "font.pen_move_dir = %d\n", font->pen_move_dir );
	}
}

void print_asglyph( FILE* stream, ASFont* font, unsigned long c)
{
	if( font )
	{
		int i, k ;
		ASGlyph *asg = get_unicode_glyph( c, font );
		if( asg == NULL )
			return;

		fprintf( stream, "glyph[%lu].ASCII = %c\n", c, (char)c );
		fprintf( stream, "glyph[%lu].width = %d\n", c, asg->width  );
		fprintf( stream, "glyph[%lu].height = %d\n", c, asg->height  );
		fprintf( stream, "glyph[%lu].lead = %d\n", c, asg->lead  );
		fprintf( stream, "glyph[%lu].ascend = %d\n", c, asg->ascend);
		fprintf( stream, "glyph[%lu].descend = %d\n", c, asg->descend );
		k = 0 ;
		fprintf( stream, "glyph[%lu].pixmap = {", c);
#if 1
		for( i = 0 ; i < asg->height*asg->width ; i++ )
		{
			if( asg->pixmap[k]&0x80 )
			{
				fprintf( stream, "%2.2X ", ((asg->pixmap[k]&0x7F)<<1));
			}else
			{
				int count = asg->pixmap[k]&0x3F;
				if( asg->pixmap[k]&0x40 )
					fprintf( stream, "FF(%d times) ", count+1 );
				else
					fprintf( stream, "00(%d times) ", count+1 );
				i += count ;
			}
		    k++;
		}
#endif
		fprintf( stream, "}\nglyph[%lu].used_memory = %d\n", c, k );
	}
}



/*********************************************************************************/
/* The end !!!! 																 */
/*********************************************************************************/

