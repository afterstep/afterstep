/*
 * Copyright (c) 2001 Sasha Vasko <sashav@sprintmail.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "../configure.h"

#define LOCAL_DEBUG
/*#define DO_CLOCKING*/

#include <unistd.h>

#include "../include/aftersteplib.h"
#include <X11/Intrinsic.h>

#ifdef HAVE_FREETYPE
#ifndef HAVE_FT2BUILD_H
#include <freetype/freetype.h>
#else
#include <ft2build.h>
#include FT_FREETYPE_H
#endif
#endif

#define INCLUDE_ASFONT_PRIVATE

#include "../include/afterstep.h"
#include "../include/screen.h"
#include "../include/ashash.h"
#include "../include/asimage.h"
#include "../include/asfont.h"

/*********************************************************************************/
/* TrueType and X11 font management functions :   								 */
/*********************************************************************************/

/*********************************************************************************/
/* construction destruction miscelanea:			   								 */
/*********************************************************************************/

void asfont_destroy (ASHashableValue value, void *data);

ASFontManager *
create_font_manager( const char * font_path, ASFontManager *reusable_memory )
{
	ASFontManager *fontman = reusable_memory;
	if( fontman == NULL )
		fontman = safecalloc( 1, sizeof(ASFontManager));
	else
		memset( fontman, 0x00, sizeof(ASFontManager));

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

static int load_freetype_glyphs( ASFont *font );
static int load_X11_glyphs( ASFont *font, XFontStruct *xfs );


ASFont*
open_freetype_font( ASFontManager *fontman, const char *font_string, int face_no, int size, Bool verbose)
{
	ASFont *font = NULL ;
#ifdef HAVE_FREETYPE
	if( fontman && fontman->ft_ok )
	{
		char *realfilename;
		FT_Face face ;
LOCAL_DEBUG_OUT( "looking for \"%s\"", font_string );
		if( (realfilename = findIconFile( font_string, fontman->font_path, R_OK )) == NULL )
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
				realfilename = findIconFile( tmp, fontman->font_path, R_OK );
			free( tmp );
		}

		if( realfilename )
		{
			face = NULL ;
LOCAL_DEBUG_OUT( "font file found : \"%s\", trying to load face #%d, using library %p", realfilename, face_no, fontman->ft_library );
			if( FT_New_Face( fontman->ft_library, "test.ttf"/*realfilename*/, face_no, &face ) )
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
				if( face->num_glyphs >  MAX_GLYPHS_PER_FONT )
					show_error( "Font \"%s\" contains too many glyphs - %d. Max allowed is %d", realfilename, face->num_glyphs, MAX_GLYPHS_PER_FONT );
				else
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
	XFontStruct *xfs ;
#ifdef I18N
	/* TODO: we have to use FontSet and loop through fonts instead filling
	 * up 2 bytes per character table with glyphs */



#else                                          /* assume ISO Latin 1 encoding */

	if( (xfs = XLoadQueryFont( dpy, font_string )) == NULL )
		return NULL;
	font = safecalloc( 1, sizeof(ASFont));
	font->magic = MAGIC_ASFONT ;
	font->fontman = fontman;
	font->type = ASF_X11 ;
	load_X11_glyphs( font, xfs );
	XFreeFont( dpy, xfs );
#endif
	return font;
}

ASFont*
get_asfont( ASFontManager *fontman, const char *font_string, int face_no, int size, ASFontType type )
{
	ASFont *font = NULL ;
	if( fontman && font_string )
	{
		if( get_hash_item( fontman->fonts_hash, (ASHashableValue)((char*)font_string), (void**)&font) != ASH_Success )
		{	/* not loaded just yet - lets do it :*/
			if( type == ASF_Freetype || type == ASF_GuessWho )
				font = open_freetype_font( fontman, font_string, face_no, size, (type == ASF_Freetype));
			if( font == NULL )
				font = open_X11_font( fontman, font_string );
			if( font != NULL )
			{
				font->name = mystrdup( font_string );
				add_hash_item( fontman->fonts_hash, (ASHashableValue)(char*)font->name, font);
			}
		}
	}
	return font;
}

void
destroy_font( ASFont *font )
{
	if( font )
	{
#ifdef HAVE_FREETYPE
		if( font->type == ASF_Freetype && font->ft_face )
			FT_Done_Face(font->ft_face);
#endif
		font->magic = 0 ;
		free( font );
	}
}

void
asfont_destroy (ASHashableValue value, void *data)
{
	if( data )
	{
		free( value.string_val );
		if( ((ASMagic*)data)->magic == MAGIC_ASFONT )
			destroy_font( (ASFont*)data );
	}
}

static unsigned char *
compress_glyph_pixmap( unsigned char *src, unsigned char *buffer,
                       unsigned int width, unsigned int height,
					   unsigned int hpad,
					   int src_step )
{/* very simple RLE compression - should work well on glyphs */
	unsigned char *pixmap ;
	register unsigned char *dst = buffer ;
	register int i, k ;
	dst[0] = 0 ;
	dst[1] = hpad-1 ;
	for( i = 0 ; i < height ; i++ )
	{
		for( k = 0 ; k < width  ; ++k )
		{
			if( src[k] == dst[0] && dst[1] < 255 )
				++dst[1];
			else
			{
				++dst ; ++dst ;
				dst[0] = src[k];
				dst[1] = 0 ;
			}
		}
		src += src_step ;
		if( hpad > 0 )
		{
			if( dst[0] == 0 && dst[1]+hpad < 255)
				dst[1] += hpad ;
			else
			{
				++dst ; ++dst ;
				dst[0] = 0 ;
				dst[1] = hpad-1 ;
			}
		}
	}
	if( dst[1] > 0 )
	{
		++dst ; ++dst ;
	}
	pixmap  = safemalloc( dst - buffer );
	memcpy( pixmap, buffer, dst-buffer );
	return pixmap;
}

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

static ASGlyphRange*
split_X11_glyph_range( unsigned long min_char, unsigned int max_char, XCharStruct *chars )
{
	ASGlyphRange *first = NULL, **r = &first;
LOCAL_DEBUG_CALLER_OUT( "min_char = %lu, max_char = %lu, chars = %p", min_char, max_char, chars );
	while( min_char <= max_char )
	{
		register int i = min_char;
		while( i <= max_char && chars[i-min_char].width == 0 ) i++ ;
		chars = &(chars[i]);
		if( min_char <= max_char )
		{
			*r = safecalloc( 1, sizeof(ASGlyphRange));
			(*r)->min_char = i ;
			while( i <= max_char && chars[i-min_char].width  != 0 ) i++ ;
			(*r)->max_char = i ;
LOCAL_DEBUG_OUT( "created glyph range from %lu to %lu", (*r)->min_char, (*r)->max_char );
			r = &((*r)->above);
		}
		min_char = i ;
	}
	return first;
}

void
load_X11_glyph_range( ASFont *font, XFontStruct *xfs, size_t char_offset,
													  unsigned char byte1,
                                                      unsigned char min_byte2,
													  unsigned char max_byte2, GC *gc )
{
	ASGlyphRange  *all, *r ;
	unsigned long  min_char = (byte1<<8)|min_byte2;
	unsigned char *buffer, *compressed_buf ;
	unsigned int   height = xfs->ascent+xfs->descent ;
	static XGCValues gcv;

	buffer = safemalloc( xfs->max_bounds.width*height);
	compressed_buf = safemalloc( xfs->max_bounds.width*height*2);
	all = split_X11_glyph_range( min_char, (byte1<<8)|max_byte2, &(xfs->per_char[char_offset]));
	for( r = all ; r != NULL ; r = r->above )
	{
		XChar2b *test_str ;
		XCharStruct *chars = &(xfs->per_char[char_offset+r->min_char-min_char]);
	    int len = (r->max_char-r->min_char+1);
		unsigned char c = r->min_char&0x00FF;
		register int i ;
		Pixmap p;
		XImage *xim;
		unsigned int total_width = 0 ;
		int pen_x = 0;

		test_str = safemalloc( len * sizeof(XChar2b));
		for( i = 0 ; i < len ; i++ )
		{
			test_str[i].byte1 = byte1 ;
			test_str[i].byte2 = i+c ;
			total_width = chars[i].width ;
		}
		p = XCreatePixmap( dpy, Scr.Root, total_width, height, 1 );

		if( *gc == NULL )
		{
			gcv.font = xfs->fid;
			*gc = XCreateGC( dpy, p, GCFont, &gcv);
		}
		XDrawImageString16( dpy, p, *gc, 0, xfs->ascent, test_str, len );
		xim = XGetImage( dpy, p, 0, 0, total_width, height, 0xFFFFFFFF, ZPixmap );
		XFreePixmap( dpy, p );
		free( test_str );

		r->glyphs = safecalloc( len, sizeof(ASGlyph) );
		for( i = 0 ; i < len ; i++ )
		{
			register int x, y ;
			int width = chars[i].width;
			unsigned char *row = &(buffer[0]), *row1, *row2;

			for( y = 0 ; y < height ; y++ )
			{
				for( x = 0 ; x < width ; x++ )
					row[x] = ( XGetPixel( xim, pen_x+x, y ) != 0 )? 0xFF:0x00;
				row += width;
			}
			row1 = &(buffer[0]);
			row = &(buffer[width]);
			row2 = &(buffer[width+width]);

			for( y = 1 ; y < height-1 ; y++ )
			{
				for( x = 1 ; x < width-1 ; x++ )
					if( row[x] == 0 )
					{/* antialiasing here : */
						int c = (int)row1[x]+
								(int)row[x-1]+
								(int)row[x+1]+
								(int)row2[x];
						row[x] = c>>3;
					}
				row  += width ;
				row1 += width ;
				row2 += width ;
			}
			r->glyphs[i].pixmap = compress_glyph_pixmap( buffer, compressed_buf, width, height, 0, width );
			r->glyphs[i].width = width ;
			r->glyphs[i].height = height ;
			r->glyphs[i].lead = 0 ;
			r->glyphs[i].ascend = xfs->ascent ;
			r->glyphs[i].descend = xfs->descent ;
LOCAL_DEBUG_OUT( "glyph %lu is %dx%d ascend = %d, lead = %d",  i, r->glyphs[i].width, r->glyphs[i].height, r->glyphs[i].ascend, r->glyphs[i].lead );
			pen_x += width ;
		}
		if( xim )
			XDestroyImage( xim );
	}
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
}

static int
load_X11_glyphs( ASFont *font, XFontStruct *xfs )
{
	GC gc = NULL;
#ifdef I18N
	if( xfs->max_byte1 > 0 && xfs->min_byte1 > 0 )
	{

		char_num *= rows ;
	}else
	{
		int i;
		int min_byte1 = (xfs->min_char_or_byte2>>8)&0x00FF;
		int max_byte1 = (xfs->max_char_or_byte2>>8)&0x00FF;
		size_t offset = MAX(0x00FF,xfs->max_char_or_byte2-(min_byte1<<8)) ;

		load_X11_glyph_range( font, xfs, 0, min_byte1,
											xfs->min_char_or_byte2-(min_byte1<<8),
			                                offset, &gc );
		offset -= xfs->min_char_or_byte2-(min_byte1<<8);
		if( max_byte1 > min_byte1 )
		{
			for( i = min_byte1+1; i < max_byte1 ; i++ )
			{
				load_X11_glyph_range( font, xfs, offset, i, 0x00, 0xFF, &gc );
				offset += 256 ;
			}
			load_X11_glyph_range( font, xfs, offset, max_byte1,
				                                     0,
													 xfs->max_char_or_byte2-(max_byte1<<8), &gc );
		}
	}
#else
	load_X11_glyph_range( font, xfs, 0, 0x00, 0x21, 0xFF, &gc );
#endif
	font->max_height = xfs->ascent+xfs->descent;
	font->max_ascend = xfs->ascent;
	font->space_size = xfs->max_bounds.width*2/3 ;
	font->pen_move_dir = LEFT_TO_RIGHT;
	if( gc )
		XFreeGC( dpy, gc );
	return xfs->ascent+xfs->descent;
}

#ifdef HAVE_FREETYPE
static void
load_glyph_freetype( ASFont *font, ASGlyph *asg, int glyph )
{
	register FT_Face face = font->ft_face;
	if( !FT_Load_Glyph( face, glyph, FT_LOAD_DEFAULT ) )
		if( !FT_Render_Glyph( face->glyph, ft_render_mode_normal ) )
		{
			FT_Bitmap 	*bmap = &(face->glyph->bitmap) ;
			register CARD8 *buf, *src = bmap->buffer ;
			int hpad = (face->glyph->bitmap_left<0)? -face->glyph->bitmap_left: face->glyph->bitmap_left ;

			if( bmap->pitch < 0 )
				src += -bmap->pitch*bmap->rows ;
			buf = safemalloc( bmap->rows*(bmap->width+hpad)*2);
			/* we better do some RLE encoding in attempt to preserv memory */
			asg->pixmap  = compress_glyph_pixmap( src, buf, bmap->width, bmap->rows, hpad, bmap->pitch );
			free( buf );
			asg->width   = bmap->width+hpad ;
			asg->height  = bmap->rows ;
			asg->ascend  = face->glyph->bitmap_top;
			asg->descend = bmap->rows - asg->ascend;
			/* we only want to keep lead if it was negative */
			asg->lead    = face->glyph->bitmap_left ;
LOCAL_DEBUG_OUT( "glyph %p is %dx%d ascend = %d, lead = %d, bmap_top = %d",  asg, asg->width, asg->height, asg->ascend, asg->lead, face->glyph->bitmap_top );
		}
}

static ASGlyphRange*
split_freetype_glyph_range( unsigned long min_char, unsigned long max_char, FT_Face face )
{
	ASGlyphRange *first = NULL, **r = &first;
#ifdef  I18N
#else
#define TO_UNICODE(i)   i
#endif
LOCAL_DEBUG_CALLER_OUT( "min_char = %lu, max_char = %lu, face = %p", min_char, max_char, face );
	while( min_char <= max_char )
	{
		register long i = min_char;
		while( i <= max_char && FT_Get_Char_Index( face, TO_UNICODE(i)) == 0 ) i++ ;
		if( i <= max_char )
		{
			*r = safecalloc( 1, sizeof(ASGlyphRange));
			(*r)->min_char = i ;
			while( i <= max_char && FT_Get_Char_Index( face, TO_UNICODE(i)) != 0 ) i++ ;
			(*r)->max_char = i ;
LOCAL_DEBUG_OUT( "created glyph range from %lu to %lu", (*r)->min_char, (*r)->max_char );
			r = &((*r)->above);
		}
		min_char = i ;
	}
	return first;
}

static int
load_freetype_glyphs( ASFont *font )
{
	int max_ascend = 0, max_descend = 0;
	unsigned long i ;
	ASGlyphRange *r ;
#ifdef I18N
	/* TODO: add font drawing internationalization : */
	font->codemap = split_glyph_freetype_range( 0x21, 0xFFFFFF, font->ft_face );
	font->pen_move_dir = RIGHT_TO_LEFT ;
#else                                          /* assume ISO Latin 1 encoding */
	font->codemap = split_freetype_glyph_range( 0x21, 0xFF, font->ft_face );
	font->pen_move_dir = LEFT_TO_RIGHT ;
#endif
	for( r = font->codemap ; r != NULL ; r = r->above )
	{
		int min_char = r->min_char ;
		int max_char = r->max_char ;

		r->glyphs = safecalloc( max_char - min_char + 1, sizeof(ASGlyph));
		for( i = min_char ; i < max_char ; ++i )
		{
			if( i != ' ' && i != '\t' && i!= '\n' )
			{
				ASGlyph *asg = &(r->glyphs[i-min_char]);
				load_glyph_freetype( font, asg, FT_Get_Char_Index( font->ft_face, TO_UNICODE(i)));
				if( asg->lead >= 0 || asg->lead+asg->width > 3 )
					font->pen_move_dir = LEFT_TO_RIGHT ;
				if( asg->ascend > max_ascend )
					max_ascend = asg->ascend ;
				if( asg->descend > max_descend )
					max_descend = asg->descend ;
			}
		}
	}

	load_glyph_freetype( font, &(font->default_glyph), 0);/* special no-symbol glyph */

	font->max_height = max_ascend+max_descend;
	font->max_ascend = max_ascend;
	return max_ascend+max_descend;
}
#endif

inline ASGlyph *get_character_glyph( const char *c, ASFont *font )
{
	unsigned long uc;
	register ASGlyphRange *r;
#ifdef I18N
#else
	uc = (unsigned long)*c;
#endif

	for( r = font->codemap ; r != NULL ; r = r->above )
		if( r->max_char >= uc )
			if( r->min_char <= uc )
				return &(r->glyphs[uc - r->min_char]);
	return &(font->default_glyph);
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

static Bool
get_text_glyph_map( const char *text, ASFont *font, ASGlyphMap *map )
{
	unsigned int w = 0;
	unsigned int line_count = 0, line_width = 0;
	int i = -1;
	const char *ptr = text;

	if( text == NULL || font == NULL || map == NULL)
		return False;

	map->glyphs_num = 0;
	while( *ptr != 0 )
	{
		++(map->glyphs_num);
		ptr += CHAR_SIZE(*ptr);
	}

	if( map->glyphs_num == 0 )
		return False;
	map->glyphs = safecalloc( map->glyphs_num, sizeof(ASGlyph*));
	do
	{
		++i ;
		if( text[i] == '\n' || text[i] == '\0' )
		{
			if( line_width > w )
				w = line_width ;
			line_width = 0 ;
			++line_count ;
			map->glyphs[i] = (text[i] == '\0')?GLYPH_EOT:GLYPH_EOL;
		}else
		{
			if( text[i] == ' ' )
			{
				line_width += font->space_size ;
				map->glyphs[i] = GLYPH_SPACE;
			}else if( text[i] == '\t' )
			{
				line_width += font->space_size*8 ;
				map->glyphs[i] = GLYPH_TAB;
			}else
			{
				ASGlyph *asg = get_character_glyph( &text[i], font );
				map->glyphs[i] = asg;
				line_width += asg->width ;
				i+=CHAR_SIZE(text[i])-1;
			}
		}
	}while( text[i] != '\0' );

	map->width = w;
	map->height = line_count * font->max_height;
	return True;
}

Bool
get_text_size( const char *text, ASFont *font, unsigned int *width, unsigned int *height )
{
	unsigned int w = 0;
	unsigned int line_count = 0, line_width = 0;
	int i = -1;
	if( text == NULL || font == NULL)
		return False;

	do
	{
		++i ;
		if( text[i] == '\n' || text[i] == '\0' )
		{
			if( line_width > w )
				w = line_width ;
			line_width = 0 ;
			++line_count ;
		}else
		{
			if( text[i] == ' ' )
				line_width += font->space_size ;
			else if( text[i] == '\t' )
				line_width += font->space_size*8 ;
			else
			{
				register ASGlyph *asg = get_character_glyph( &text[i], font );
				line_width += asg->width ;
				i+=CHAR_SIZE(text[i])-1;
			}
		}
	}while( text[i] != '\0' );

	if( width )
		*width = w ;
	if( height )
		*height = line_count * font->max_height ;
	return True;
}


ASImage *
draw_text( const char *text, ASFont *font, int compression )
{
	ASGlyphMap map ;
	CARD32 *memory;
	CARD32 **scanlines ;
	int i = 0, offset = 0, line_height, space_size, base_line;
	ASImage *im;
	int pen_x = 0, pen_y = 0;
LOCAL_DEBUG_CALLER_OUT( "text = \"%s\", font = %p, compression = %d", text, font, compression );
	if( !get_text_glyph_map( text, font, &map) )
		return NULL;
LOCAL_DEBUG_OUT( "text size = %dx%d pixels", map.width, map.height );
	im = safecalloc( 1, sizeof(ASImage));
	asimage_start( im, map.width, map.height, compression );

	line_height = font->max_height ;
	space_size  = (font->space_size>>1)+1;
	base_line = font->max_ascend;
	scanlines = safemalloc( line_height*sizeof(CARD32*));
	memory = safemalloc( map.width*line_height*sizeof(CARD32));
	do
	{
		scanlines[i] = memory + offset;
		offset += map.width;
	}while ( ++i < line_height );

	i = -1 ;
	if(font->pen_move_dir == RIGHT_TO_LEFT)
		pen_x = map.width;
	do
	{
		++i;
/*fprintf( stderr, "drawing character %d '%c'\n", i, text[i] );*/
		if( map.glyphs[i] == GLYPH_EOL || map.glyphs[i] == GLYPH_EOT )
		{
			int y;
			for( y = 0 ; y < line_height ; ++y )
			{
				register int x = pen_x;
				register CARD32 *line = scanlines[y];
				if( font->pen_move_dir == RIGHT_TO_LEFT )
					while( x >= 0 )
						line[x--] = 0 ;
				else
					while( x < map.width )
						line[x++] = 0 ;

/*				x = 0;
				while( x < map.width )
					fprintf( stderr, "%2.2X ", scanlines[y][x++] );
				fprintf( stderr, "\n" );
*/
 				asimage_add_line (im, IC_ALPHA, line, pen_y+y);
			}
			pen_x = (font->pen_move_dir == RIGHT_TO_LEFT)? map.width : 0;
			pen_y += line_height;
			base_line += line_height;
		}else
		{
			if( map.glyphs[i] == GLYPH_SPACE || map.glyphs[i] == GLYPH_TAB )
			{
				int x = pen_x;
				int d_pen = (map.glyphs[i] == GLYPH_TAB)?space_size*8 : space_size;
				if( font->pen_move_dir == RIGHT_TO_LEFT )
					x 	  -= d_pen ;
				else
					pen_x += d_pen ;

				while( x < pen_x )
				{
					register int y ;
					for( y = 0 ; y < line_height ; ++y )
						scanlines[y][x] = 0 ;
					++x;
				}
				if( font->pen_move_dir == RIGHT_TO_LEFT )
					pen_x  -= d_pen ;
			}else
			{
				/* now comes the fun part : */
				ASGlyph *asg = map.glyphs[i] ;
				int start_y = base_line - asg->ascend, y = 0;
				int max_y = start_y + asg->height;
				register CARD8 *row = asg->pixmap;
				int width = asg->width ;
				register int x = 0;
				int count = row[1];

				if( font->pen_move_dir == RIGHT_TO_LEFT )
					pen_x  -= width ;
				while( y < start_y )
				{
					register CARD32 *dst = scanlines[y]+pen_x;
					while( x < width ) dst[x++] = 0;
					++y;
				}
				while( y < max_y )
				{
					register CARD32 *dst = scanlines[y]+pen_x;
					register CARD32 data = row[0];
					for( x = 0 ; x < width ; ++x )
					{
/*fprintf( stderr, "data = %X, count = %d, x = %d, y = %d\n", data, count, x, y );*/
						if( count < 0 )
						{
							++row, ++row ;
							data = row[0];
						 	count = row[1];
						}
						dst[x] = data ;
						--count;
					}
					++y;
				}
				while( y < line_height )
				{
					register CARD32 *dst = scanlines[y]+pen_x;
					for( x = 0 ; x < width ; ++x )
						dst[x] = 0;
					++y;
				}
				if( font->pen_move_dir == LEFT_TO_RIGHT )
					pen_x  += width ;
			}
		}
	}while( map.glyphs[i] != GLYPH_EOT );

	free( memory );
	free( scanlines );
	return im;
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
		fprintf( stream, "font.max_ascend = %d\n", font->max_ascend );
		fprintf( stream, "font.pen_move_dir = %d\n", font->pen_move_dir );
	}
}

void print_asglyph( FILE* stream, ASFont* font, unsigned long c)
{
	if( font )
	{
		int i, k ;
		ASGlyph *asg = get_character_glyph( (char*)&c, font );
		if( asg == NULL )
			return;

		fprintf( stream, "glyph[%lu].ASCII = %c\n", c, (char)c );
		fprintf( stream, "glyph[%lu].width = %d\n", c, asg->width  );
		fprintf( stream, "glyph[%lu].height = %d\n", c, asg->height  );
		fprintf( stream, "glyph[%lu].lead = %d\n", c, asg->lead  );
		fprintf( stream, "glyph[%lu].ascend = %d\n", c, asg->ascend);
		fprintf( stream, "glyph[%lu].descend = %d\n", c, asg->descend );
		k = 0 ;
		for( i = 0 ; i < asg->height*asg->width ; i++ )
		{
			fprintf( stream, "%d(%2.2X) ", asg->pixmap[k+1], asg->pixmap[k]);
			i += asg->pixmap[k+1] ;
			k++;
		}
	}
}



/*********************************************************************************/
/* The end !!!! 																 */
/*********************************************************************************/

