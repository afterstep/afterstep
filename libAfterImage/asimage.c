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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "../configure.h"

/*#define LOCAL_DEBUG*/

#include "../include/aftersteplib.h"
#include <X11/Intrinsic.h>

#include "../include/afterstep.h"
#include "../include/screen.h"
#include "../include/ascolor.h"
#include "../include/mytexture.h"
#include "../include/XImage_utils.h"
#include "../include/asimage.h"

void
asimage_free_color (ASImage * im, CARD8 ** color)
{
	if (im != NULL && color != NULL)
	{
		register int  i;

		for (i = 0; i < im->height; i++)
			if (color[i])
				free (color[i]);
		free (color);
	}
}

void
asimage_init (ASImage * im, Bool free_resources)
{
	if (im != NULL)
	{
		if (free_resources)
		{
			asimage_free_color (im, im->red);
			asimage_free_color (im, im->green);
			asimage_free_color (im, im->blue);
			asimage_free_color (im, im->alpha);
			if (im->buffer)
				free (im->buffer);
			if( im->ximage )
				XDestroyImage( im->ximage );
		}
		memset (im, 0x00, sizeof (ASImage));
	}
}

void
asimage_start (ASImage * im, unsigned int width, unsigned int height)
{
	if (im)
	{
		asimage_init (im, True);
		im->width = width;
		im->buf_len = width + width;
		im->buffer = malloc (im->buf_len);

		im->height = height;

		im->red = safemalloc (sizeof (CARD8 *) * height);
		memset (im->red, 0x00, sizeof (CARD8 *) * height);
		im->green = safemalloc (sizeof (CARD8 *) * height);
		memset (im->green, 0x00, sizeof (CARD8 *) * height);
		im->blue = safemalloc (sizeof (CARD8 *) * height);
		memset (im->blue, 0x00, sizeof (CARD8 *) * height);
		im->alpha = safemalloc (sizeof (CARD8 *) * height);
		memset (im->alpha, 0x00, sizeof (CARD8 *) * height);
	}
}

static inline CARD8 **
asimage_get_color_ptr (ASImage * im, ColorPart color)
{
	switch (color)
	{
	 case IC_RED:
		 return im->red;
	 case IC_GREEN:
		 return im->green;
	 case IC_BLUE:
		 return im->blue;
	 case IC_ALPHA:
		 return im->alpha;
	}
	return NULL;
}

void
asimage_apply_buffer (ASImage * im, ColorPart color, unsigned int y)
{
	CARD8       **part;

	if (im == NULL)
		return;
	if (im->buffer == NULL || y >= im->height)
		return;

	if ((part = asimage_get_color_ptr (im, color)) != NULL)
	{
		if (part[y] != NULL)
			free (part[y]);
		part[y] = safemalloc (im->buf_used);
		memcpy (part[y], im->buffer, im->buf_used);
		im->buf_used = 0;
	}
}

void
asimage_add_line (ASImage * im, ColorPart color, register CARD32 * data, unsigned int y)
{
	int             i = 1, bstart = 0, ccolor = 0;
	unsigned int    width;
	register CARD8 *dst;
	register int 	tail = 0;
	int best_size, best_bstart = 0, best_tail = 0;

	if (im == NULL || data == NULL)
		return;
	if (im->buffer == NULL || y >= im->height)
		return;

	best_size = 0 ;
	width = im->width;
	dst = im->buffer;
/*	fprintf( stderr, "%d:%d:%d<%2.2X ", y, color, 0, data[0] ); */

	if( width == 1 )
	{
		dst[0] = RLE_DIRECT_TAIL ;
		dst[1] = data[0] ;
		im->buf_used = 2;
	}else
	{
		do
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
		}while( i < width );
		if( best_size+width < tail )
		{
			LOCAL_DEBUG_OUT( " %d:%d >resetting bytes starting with offset %d(%d) (0x%2.2X) to DIRECT_TAIL( %d bytes total )", y, color, best_tail, best_bstart, dst[best_tail], width-best_bstart );
			dst[best_tail] = RLE_DIRECT_TAIL;
			dst += best_tail+1;
			data += best_bstart;
			for( i = width-best_bstart-1 ; i >=0 ; --i )
				dst[i] = data[i] ;
			im->buf_used = best_tail+1+width-best_bstart ;
		}else
		{
			dst[tail] = RLE_EOL;
			im->buf_used = tail+1;
		}
	}
	asimage_apply_buffer (im, color, y);
}

unsigned int
asimage_print_line (ASImage * im, ColorPart color, unsigned int y, unsigned long verbosity)
{
	CARD8       **color_ptr;
	register CARD8 *ptr;
	int           to_skip = 0;
	int 		  uncopressed_size = 0 ;

	if (im == NULL)
		return 0;
	if (y >= im->height)
		return 0;

	if ((color_ptr = asimage_get_color_ptr (im, color)) == NULL)
		return 0;
	ptr = color_ptr[y];
	if( ptr == NULL )
	{
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

inline int
asimage_decode_line (ASImage * im, ColorPart color, CARD32 * to_buf, unsigned int y)
{
	CARD8       **color_ptr = asimage_get_color_ptr (im, color);
	register CARD8  *src = color_ptr[y];
	register CARD32 *dst = to_buf;
	/* that thing below is supposedly highly optimized : */
	if( src == NULL )
		return 0;
	while ( *src != RLE_EOL)
	{
		if( src[0] == RLE_DIRECT_TAIL )
		{
			register int i = im->width - (dst-to_buf) ;
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
	return (dst - to_buf);
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
	asimage_decode_line( im, color, tmp, y );
	for( i = 0 ; i < im->width ; i++ )
		if( tmp[i] != to_buf[i] )
		{
			if( verbose )
				show_error( "line %d, component %d differ at offset %d ( 0x%lX(compresed) != 0x%lX(orig) )\n", y, color, i, tmp[i], to_buf[i] );
			return False ;
		}
	return True;
}

typedef struct ASScanline
{
#define SCL_DO_RED          (0x01<<0)
#define SCL_DO_GREEN        (0x01<<1)
#define SCL_DO_BLUE         (0x01<<2)
#define SCL_DO_ALPHA		(0x01<<3)
#define SCL_DO_ALL			(SCL_DO_RED|SCL_DO_GREEN|SCL_DO_BLUE|SCL_DO_ALPHA)
	ASFlagType 	   flags;
	CARD32        *buffer ;
	CARD32        *red, *green, *blue, *alpha;
	unsigned int   width, shift;
/*    CARD32 r_mask, g_mask, b_mask ; */
}ASScanline;

static ASScanline*
prepare_scanline( unsigned int width, unsigned int shift, ASScanline *reusable_memory  )
{
	register ASScanline *sl = reusable_memory ;

	if( sl == NULL )
		sl = safecalloc( 1, sizeof( ASScanline ) );

	sl->width 	= width ;
	sl->shift   = shift ;
	sl->red 	= sl->buffer = safemalloc ((width*4)*sizeof(CARD32));
	sl->green 	= sl->red   + width;
	sl->blue 	= sl->green + width;
	sl->alpha 	= sl->blue  + width;
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

static inline void
fill_ximage_buffer_pseudo (XImage *xim, ASScanline * xim_buf, unsigned int line)
{
 	XColor        color;
	register int i ;
	for( i = 0 ; i < xim_buf->width ; i++ )
	{
		color.pixel = XGetPixel( xim, i, line );
		XQueryColor (dpy, ascolor_colormap, &color);
		xim_buf->red[i] = color.red ;
		xim_buf->green[i] = color.green ;
		xim_buf->blue[i] = color.blue ;
	}
}

static inline void
put_ximage_buffer_pseudo (XImage *xim, ASScanline * xim_buf, unsigned int line)
{
 	XColor        color;
	register int i ;
	for( i = 0 ; i < xim_buf->width ; i++ )
	{
		color.red   = xim_buf->red[i] ;
		color.green = xim_buf->green[i] ;
		color.blue  = xim_buf->blue[i] ;
		XAllocColor (dpy, ascolor_colormap, &color);
		XPutPixel( xim, i, line, color.pixel );
	}
}


static inline void
fill_ximage_buffer (unsigned char *xim_line, ASScanline * xim_buf, int BGR_mode, int byte_order, int bpp)
{
	int           i, width = xim_buf->width;
	register CARD32 *r = xim_buf->red, *g = xim_buf->green, *b = xim_buf->blue;
	register CARD8  *src = (CARD8 *) xim_line;

	if (BGR_mode != byte_order)
	{
		r = xim_buf->blue;
		b = xim_buf->red ;
	}
	if ( bpp == 24)
	{
		for (i = 0 ; i < width; i++)
		{   						   			/* must add LSB/MSB checking */
			r[i] = src[0];
			g[i] = src[1];
			b[i] = src[2];
			src += 3;
		}
	} else if (bpp == 32)
	{
		if (byte_order == MSBFirst)
			for (i = 0 ; i < width; i++)
			{
				r[i] = src[1];
				g[i] = src[2];
				b[i] = src[3];
				src+= 4;
			}
		else
			for (i = 0 ; i < width; i++)
			{
				r[i] = src[0];
				g[i] = src[1];
				b[i] = src[2];
				src+=4;
			}
	} else if (bpp == 16)
	{										   /* must add LSB/MSB checking */
		if (byte_order == MSBFirst)
			for (i = 0 ; i < width; i++)
			{
				r[i] =  (src[0]&0xF8);
				g[i] = ((src[0]&0x07)<<5)|((src[1]&0xE0)>>3);
				b[i] =  (src[1]&0x1F)<<3;
				src += 2;
			}
		else
			for (i = 0 ; i < width; i++)
			{
				r[i] =  (src[1]&0xF8);
				g[i] = ((src[1]&0x07)<<5)|((src[0]&0xE0)>>3);;
				b[i] =  (src[0]&0x1F)<<3;
				src += 2;
			}
	} else if (bpp == 15)
	{										   /* must add LSB/MSB checking */
		if (byte_order == MSBFirst)
			for (i = 0 ; i < width; i++)
			{
				r[i] =  (src[0]&0x7C)<<1;
				g[i] = ((src[0]&0x03)<<5)|((src[1]&0xE0)>>3);
				b[i] =  (src[1]&0x1F)<<3;
				src += 2;
			}
		else
			for (i = 0 ; i < width; i++)
			{
				r[i] =  (src[1]&0x7C)<<1;
				g[i] = ((src[1]&0x03)<<5)|((src[0]&0xE0)>>3);
				b[i] =  (src[0]&0x1F)<<3;
				src += 2;
			}
	} else
	{										   /* below 8 bpp handling */
		for (i = 0 ; i < width; i++)
			pixel_to_color_low( src[i], r+i, g+i, b+i );
	}
}

static inline void
put_ximage_buffer (unsigned char *xim_line, ASScanline * xim_buf, int BGR_mode, int byte_order, int bpp)
{
	int           i, width = xim_buf->width;
	register CARD32 *r = xim_buf->red, *g = xim_buf->green, *b = xim_buf->blue;
	register CARD8  *src = (CARD8 *) xim_line;

	if (BGR_mode != byte_order)
	{
		r = xim_buf->blue;
		b = xim_buf->red ;
	}
	if ( bpp == 24)
	{
		for (i = 0 ; i < width; i++)
		{   						   			/* must add LSB/MSB checking */
			src[0] = r[i];
			src[1] = g[i];
			src[2] = b[i];
			src += 3;
		}
	} else if (bpp == 32)
	{
		if (byte_order == MSBFirst)
			for (i = 0 ; i < width; i++)
			{
				src[1] = r[i];
				src[2] = g[i];
				src[3] = b[i];
				src+= 4;
			}
		else
			for (i = 0 ; i < width; i++)
			{
				src[0] = r[i];
				src[1] = g[i];
				src[2] = b[i];
				src+=4;
			}
	} else if (bpp == 16)
	{										   /* must add LSB/MSB checking */
		CARD32 err_red = 0, err_green = 0, err_blue = 0;
		CARD32 red, green, blue;
		if (byte_order == MSBFirst)
			for (i = 0 ; i < width; i++)
			{ /* diffusion to compensate for quantization error :*/
				red = r[i]+err_red ; err_red = (red&0x07)>>1 ;
				green = g[i]+err_green ; err_green = (green&0x03)>>1 ;
				blue = g[i]+err_blue ; err_blue = (blue&0x07)>>1 ;
				src[0] = (CARD8)((red << 3) | (( green >> 3) & 0x07 ));
				src[1] = (CARD8)((green<<5) |    blue);
				src += 2;
			}
		else
			for (i = 0 ; i < width; i++)
			{/* diffusion to compensate for quantization error :*/
				red = r[i]+err_red ; err_red = (red&0x07)>>1 ;
				green = g[i]+err_green ; err_green = (green&0x03)>>1 ;
				blue = g[i]+err_blue ; err_blue = (blue&0x07)>>1 ;
				src[1] = (CARD8)((red << 3) | (( green >> 3) & 0x07 ));
				src[0] = (CARD8)((green << 5) |  blue);
				src += 2;
			}
	} else if (bpp == 15)
	{										   /* must add LSB/MSB checking */
		CARD32 err_red = 0, err_green = 0, err_blue = 0;
		CARD32 red, green, blue;
		if (byte_order == MSBFirst)
			for (i = 0 ; i < width; i++)
			{/* diffusion to compensate for quantization error :*/
				red = r[i]+err_red ; err_red = (red&0x07)>>1 ;
				green = g[i]+err_green ; err_green = (green&0x07)>>1 ;
				blue = g[i]+err_blue ; err_blue = (blue&0x07)>>1 ;
				src[0] = (CARD8)((red << 2) | (( green >> 3) & 0x03 ))&0x7F;
				src[1] = (CARD8)((green << 5) |  blue);
				src += 2;
			}
		else
			for (i = 0 ; i < width; i++)
			{/* diffusion to compensate for quantization error :*/
				red = r[i]+err_red ; err_red = (red&0x07)>>1 ;
				green = g[i]+err_green ; err_green = (green&0x07)>>1 ;
				blue = g[i]+err_blue ; err_blue = (blue&0x07)>>1 ;
				src[1] = (CARD8)((red << 2) | (( green >> 3) & 0x03 ))&0x7F;
				src[0] = (CARD8)((green << 5) |  blue);
				src += 2;
			}
	} else
	{										   /* below 8 bpp handling */
		for (i = 0 ; i < width; i++)
			src[i] = color_to_pixel_low( r[i], g[i], b[i] );
	}
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
/* for scale factor of 3 we use these formulas :  */
/* Ca = (-2C1+8*C2+5*C3-2C4)/9 		  			  */
/* Cb = (-2C1+5*C2+8*C3-2C4)/9 		  			  */
/* or better : 									  */
/* Ca = (-C1+5*C2+3*C3-C4)/6 		  			  */
/* Cb = (-C1+3*C2+5*C3-C4)/6 		  			  */
#define INTERPOLATE_A_COLOR3(c1,c2,c3,c4)  	(((((c2)<<2)+(c2)+((c3)<<1)+(c3)-(c1)-(c4))<<QUANT_ERR_BITS)/6)
#define INTERPOLATE_B_COLOR3(c1,c2,c3,c4)  	(((((c2)<<1)+(c2)+((c3)<<2)+(c3)-(c1)-(c4))<<QUANT_ERR_BITS)/6)
/* just a hypotesus, but it looks good for scale factors S > 3: */
/* Cn = (-C1+(2*(S-n)+1)*C2+(2*n+1)*C3-C4)/2S  	  			   */
/* or :
 * Cn = (-C1+(2*S+1)*C2+C3-C4-n*2*C2+n*2*C3)/2S  			   */
/*       [ T                   [C2s]  [C3s]]   			       */
#define INTERPOLATION_C2s(c2)	 		 	((c2)<<2)
#define INTERPOLATION_C3s(c3)	 		 	((c3)<<2)
#define INTERPOLATION_TOTAL_START(c1,C2s,C3s,c4,S) 	((((S)<<2)+1)*(C2s)+((C3s)<<1)+(C3s)-c1-c4)
#define INTERPOLATION_TOTAL_STEP(C2s,C3s)  	((C3s)-(C2s))
#define INTERPOLATE_N_COLOR(T,S)		  	(((T)<<(QUANT_ERR_BITS-1))/(S))

#define AVERAGE_COLOR1(c) 					((c)<<QUANT_ERR_BITS)
#define AVERAGE_COLOR2(c1,c2)				(((c1)+(c2))<<(QUANT_ERR_BITS-1))
#define AVERAGE_COLORN(T,N)					(((T)<<QUANT_ERR_BITS)/N)

static inline void
enlarge_component12( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* expected len >= 2  */
	register int i = 0, k = 0;
	register int c1 = src[0], c4 = src[1];
	--len; --len ;
	while( i < len )
	{
		c4 = src[i+2];
		if( scales[i] == 0 )
		{
			c1 = src[i];     /* that's right we can do that PRIOR as we calculate nothing */
			dst[k] = INTERPOLATE_COLOR1(c1) ;
			++k;
		}else
		{
			register int c2 = src[i], c3 = src[i+1] ;
			dst[k] = INTERPOLATE_COLOR1(c2) ;
			dst[++k] = INTERPOLATE_COLOR2(c1,c2,c3,c4);
			++k;
			c1 = c2;
		}
		++i;
	}

	/* to avoid one more if() in loop we moved tail part out of the loop : */
	if( scales[i] == 0 )
		dst[k] = INTERPOLATE_COLOR1(src[i]);
	else
	{
		register int c2 = src[i], c3 = src[i+1] ;
		dst[k] = INTERPOLATE_COLOR1(c2) ;
		dst[++k] = INTERPOLATE_COLOR2(c1,c2,c3,c3);
	}
	dst[k+1] = INTERPOLATE_COLOR1(src[i+1]);
}

static inline void
enlarge_component23( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* expected len >= 2  */
	register int i = 0, k = 0;
	register int c1 = src[0], c4 = src[1];
	--len; --len;
	while( i < len )
	{
		register int c2 = src[i], c3 = src[i+1] ;
		c4 = src[i+2];
		dst[k] = INTERPOLATE_COLOR1(c2) ;
		if( scales[0] == 1 )
		{
			dst[++k] = INTERPOLATE_COLOR2(c1,c2,c3,c4);
		}else
		{
			dst[++k] = INTERPOLATE_A_COLOR3(c1,c2,c3,c4);
			dst[++k] = INTERPOLATE_B_COLOR3(c1,c2,c3,c4);
		}
		c1 = c2 ;
		++k;
		++i;
	}
	/* to avoid one more if() in loop we moved tail part out of the loop : */
	{
		register int c2 = src[i], c3 = src[i+1] ;
		dst[k] = INTERPOLATE_COLOR1(c2) ;
		if( scales[0] == 1 )
		{
			dst[k+1] = INTERPOLATE_COLOR2(c1,c2,c3,c4);
		}else
		{
			dst[++k] = INTERPOLATE_A_COLOR3(c1,c2,c3,c4);
			dst[k+1] = INTERPOLATE_B_COLOR3(c1,c2,c3,c4);
		}
	}
	dst[k+2] = INTERPOLATE_COLOR1(c4) ;
}

/* this case is more complex since we cannot really hardcode coefficients
 * visible artifacts on smooth gradient-like images
 */
static inline void
enlarge_component( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* we skip all checks as it is static function and we want to optimize it
  * as much as possible */
	int i = 0;
	int c1 = src[0], c4 = src[1];
	register int k = 0;
	--len; --len;
	while( i <= len )
	{
		register int C2s = INTERPOLATION_C2s(src[i]), C3s = INTERPOLATION_C3s(src[i+1]) ;
		register int n, T ;
		int S = scales[i]+1 ;

		if( i < len )
			c4 = src[i+2];
		T = INTERPOLATION_TOTAL_START(c1,C2s,C3s,c4,S);
		for( n = 1 ; n < S ; n++ )
		{
			dst[k+n] = INTERPOLATE_N_COLOR(T,S);
			T += INTERPOLATION_TOTAL_STEP(C2s,C3s);
		}
		c1 = src[i];
		dst[k] = INTERPOLATE_COLOR1(c1) ;
		k += n ;
		++i;
	}
	dst[k] = INTERPOLATE_COLOR1(c4) ;
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
			dst[k] = AVERAGE_COLORN(c1,scales[k]);
		}
	}
}
/* for consistency sake : */
static inline void
copy_component( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		dst[i] = src[i];
}

static inline void
add_component( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		src[i] += dst[i];
}

static inline int
set_component( register CARD32 *src, register CARD32 value, int offset, int len )
{
	register int i ;
	for( i = offset ; i < len ; ++i )
		src[i] += value;
	return len-offset;
}

static inline void
divide_component( register CARD32 *data, int ratio, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		data[i] /= ratio;
}

static inline void
rbitshift_component( register CARD32 *data, int bits, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		data[i] = data[i]>>bits;
}

static inline void
simple_diffuse_shift_component( register CARD32 *data, int bits, int len )
{/* we carry half of the quantization error onto the following pixel : */
	register int i ;
	register CARD32 err = 0;
	for( i = 0 ; i < len ; ++i )
	{
		data[i] += err ;
		err = (data[i]&QUANT_ERR_MASK)>>1 ;
		data[i] = data[i]>>bits;
	}
}

/* the following 5 macros will in fact unfold into huge but fast piece of code : */
/* we make poor compiler work overtime unfolding all this macroses but I bet it  */
/* is still better then C++ templates :)									     */

#define DECODE_SCANLINE(im,dst,y) \
do{												\
	clear_flags( dst.flags,SCL_DO_ALL);			\
	if( set_component(dst.red  ,0,asimage_decode_line(im,IC_RED  ,dst.red,  y),dst.width)<dst.width) \
		set_flags( dst.flags,SCL_DO_RED);												 \
	if( set_component(dst.green,0,asimage_decode_line(im,IC_GREEN,dst.green,y),dst.width)<dst.width) \
		set_flags( dst.flags,SCL_DO_GREEN);												 \
	if( set_component(dst.blue ,0,asimage_decode_line(im,IC_BLUE ,dst.blue, y),dst.width)<dst.width) \
		set_flags( dst.flags,SCL_DO_BLUE);												 \
	if( set_component(dst.alpha,0,asimage_decode_line(im,IC_ALPHA,dst.alpha,y),dst.width)<dst.width) \
		set_flags( dst.flags,SCL_DO_ALPHA);												 \
  }while(0)

#define ENCODE_SCANLINE(im,src,y) \
do{	asimage_add_line(im, IC_RED,   src.red,   y); \
   	asimage_add_line(im, IC_GREEN, src.green, y); \
   	asimage_add_line(im, IC_BLUE,  src.blue,  y); \
	if( get_flags(src.flags,SCL_DO_ALPHA))asimage_add_line(im, IC_ALPHA, src.alpha, y); \
  }while(0)

#define SCANLINE_FUNC(f,src,dst,scales,len) \
do{	f((src).red,  (dst).red,  (scales),(len));		\
	f((src).green,(dst).green,(scales),(len)); 		\
	f((src).blue, (dst).blue, (scales),(len));   	\
	if(get_flags(src.flags,SCL_DO_ALPHA)) f((src).alpha,(dst).alpha,(scales),(len)); \
  }while(0)

#define CHOOSE_SCANLINE_FUNC(r,src,dst,scales,len) \
 switch( r )                                              							\
 {  case 0:	SCANLINE_FUNC(shrink_component,(src),(dst),(scales),(len));break;   	\
	case 1: SCANLINE_FUNC(copy_component,  (src),(dst),(scales),(len));	break;  	\
	case 2:	SCANLINE_FUNC(enlarge_component12,(src),(dst),(scales),(len));break ; 	\
	case 3:	SCANLINE_FUNC(enlarge_component23,(src),(dst),(scales),(len));break;  	\
	default:SCANLINE_FUNC(enlarge_component,  (src),(dst),(scales),(len));        	\
 }

#define SCANLINE_MOD(f,src,p,len) \
do{	f((src).red,p,(len));		\
	f((src).green,p,(len));		\
	f((src).blue,p,(len));		\
	if(get_flags(src.flags,SCL_DO_ALPHA)) f((src).alpha,p,(len));\
  }while(0)

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
	int eps = 0;

	if( smaller == 0 )
		smaller = 1;
	if( bigger == 0 )
		bigger = 1;
	scales = safecalloc( smaller, sizeof(int));
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

void
scale_image_down( ASImage *src, ASImage *dst, int h_ratio, int *scales_h, int* scales_v, Bool to_xim)
{
	ASScanline src_line, dst_line, total ;
	int i = 0, k = 0;
	unsigned char *xim_line = NULL;
	int            height = 0, bpl = 0;

	if( to_xim  )
	{
		if( dst->ximage == NULL )
			return;
		height =   dst->ximage->height;
		bpl = 	   dst->ximage->bytes_per_line;
		xim_line = dst->ximage->data;
	}
	prepare_scanline( src->width, 0, &src_line );
	prepare_scanline( dst->width, QUANT_ERR_BITS, &dst_line );
	prepare_scanline( dst->width, QUANT_ERR_BITS, &total );
	while( k < dst->height )
	{
		int reps = scales_v[k] ;
		DECODE_SCANLINE(src,src_line,i);
		total.flags = src_line.flags ;
		CHOOSE_SCANLINE_FUNC(h_ratio,src_line,dst_line,scales_h,total.width);
		reps += i-1;
		while ( reps > i )
		{
			++i ;
			DECODE_SCANLINE(src,src_line,i);
			total.flags |= src_line.flags ;
			CHOOSE_SCANLINE_FUNC(h_ratio,src_line,dst_line,scales_h,dst_line.width);
			SCANLINE_FUNC(add_component,total,dst_line,NULL,total.width);
		}
		if( (reps = scales_v[k])> 1 )
		{
			if( reps == 2 )
				SCANLINE_MOD(rbitshift_component,total,1,total.width);
			else
				SCANLINE_MOD(divide_component,total,reps,total.width);
			SCANLINE_MOD(simple_diffuse_shift_component,total,QUANT_ERR_BITS,total.width);
		}else
			SCANLINE_MOD(rbitshift_component,total,QUANT_ERR_BITS,total.width);
		if( to_xim )
		{
			if( ascolor_true_depth == 0 )
				put_ximage_buffer_pseudo( dst->ximage, &total, k );
			else
			{
				put_ximage_buffer (xim_line, &total, 0, dst->ximage->byte_order, dst->ximage->bits_per_pixel );
				xim_line += bpl;
			}
		}else
			ENCODE_SCANLINE(dst,total,k);
		++k;
	}
	free_scanline(&src_line, True);
	free_scanline(&dst_line, True);
	free_scanline(&total, True);
}

void
scale_image_up12( ASImage *src, ASImage *dst, int h_ratio )
{
}

void
scale_image_up23( ASImage *src, ASImage *dst, int h_ratio )
{
}

void
scale_image_up( ASImage *src, ASImage *dst, int h_ratio )
{
}

ASImage *
scale_asimage( ASImage *src, int to_width, int to_height, Bool to_xim, int depth )
{
	ASImage *dst = NULL ;
	int h_ratio, v_ratio ;
	int *scales_h = NULL, *scales_v = NULL;

	if( !check_scale_parameters(src,&to_width,&to_height) )
		return NULL;

	dst = safecalloc(1, sizeof(ASImage));
	asimage_start (dst, to_width, to_height);
	if( to_xim )
		dst->ximage = CreateXImageAndData( dpy, ascolor_visual, depth, ZPixmap, 0, to_width, to_height );

	h_ratio = to_width/src->width;
	if( to_width%src->width > 0 )
		h_ratio++ ;
	v_ratio = to_height/src->height;
	if( to_height%src->height > 0 )
		v_ratio++ ;

	scales_h = make_scales( src->width, to_width );
	scales_v = make_scales( src->height, to_height );

	if( v_ratio <= 1 ) 					   /* scaling down */
		scale_image_down( src, dst, h_ratio, scales_h, scales_v, to_xim );
	else if( v_ratio > 1 && v_ratio <= 2) /* scaling up */
		scale_image_up12( src, dst, h_ratio );
	else if( v_ratio > 2 && v_ratio <= 3)
		scale_image_up23( src, dst, h_ratio );
	else
		scale_image_up( src, dst, h_ratio );

	return dst;
}

/****************************************************************************/
/* ASImage->XImage->pixmap->XImage->ASImage conversion						*/
/****************************************************************************/

ASImage      *
asimage_from_ximage (XImage * xim)
{
	ASImage      *im = NULL;
	unsigned char *xim_line;
	int           i, height, bpl;
	ASScanline    xim_buf;
#ifdef LOCAL_DEBUG
	CARD32       *tmp ;
#endif
	if (xim == NULL)
		return im;

	im = (ASImage *) safecalloc (1, sizeof (ASImage));
	asimage_start (im, xim->width, xim->height);
#ifdef LOCAL_DEBUG
	tmp = safemalloc( xim->width * sizeof(CARD32));
#endif
	prepare_scanline( xim->width, 0, &xim_buf );

	height = xim->height;
	bpl = xim->bytes_per_line;
	xim_line = xim->data;

	if( ascolor_true_depth == 0 )
		for (i = 0; i < height; i++)
		{
			fill_ximage_buffer_pseudo( xim, &xim_buf, i );
			asimage_add_line (im, IC_RED,   xim_buf.red, i);
			asimage_add_line (im, IC_GREEN, xim_buf.green, i);
			asimage_add_line (im, IC_BLUE,  xim_buf.blue, i);
		}
	else                                       /* TRue color visual */
		for (i = 0; i < height; i++)
		{
			fill_ximage_buffer (xim_line, &xim_buf, 0, xim->byte_order, xim->bits_per_pixel );
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
ximage_from_asimage (ASImage *im, int depth)
{
	XImage        *xim = NULL;
	unsigned char *xim_line;
	int           i, height, bpl;
	ASScanline    xim_buf;

	if (im == NULL)
		return xim;

	xim = CreateXImageAndData( dpy, ascolor_visual, depth, ZPixmap, 0, im->width, im->height );

	prepare_scanline( xim->width, 0, &xim_buf );

	height = im->height;
	bpl = 	   xim->bytes_per_line;
	xim_line = xim->data;

	if( ascolor_true_depth == 0 )
		for (i = 0; i < height; i++)
		{
			asimage_decode_line (im, IC_RED,   xim_buf.red, i);
			asimage_decode_line (im, IC_GREEN, xim_buf.green, i);
			asimage_decode_line (im, IC_BLUE,  xim_buf.blue, i);
			put_ximage_buffer_pseudo( xim, &xim_buf, i );
		}
	else                                       /* TRue color visual */
		for (i = 0; i < height; i++)
		{
			asimage_decode_line (im, IC_RED,   xim_buf.red, i);
			asimage_decode_line (im, IC_GREEN, xim_buf.green, i);
			asimage_decode_line (im, IC_BLUE,  xim_buf.blue, i);
			put_ximage_buffer (xim_line, &xim_buf, 0, xim->byte_order, xim->bits_per_pixel );
			xim_line += bpl;
		}
	free_scanline(&xim_buf, True);

	return xim;
}

ASImage      *
asimage_from_pixmap (Pixmap p, int x, int y, unsigned int width, unsigned int height, unsigned long plane_mask, Bool keep_cache)
{
	XImage       *xim = XGetImage (dpy, p, x, y, width, height, plane_mask, ZPixmap);
	ASImage      *im = NULL;

	if (xim)
	{
		im = asimage_from_ximage (xim);
		if( keep_cache )
			im->ximage = xim ;
		else
			XDestroyImage (xim);
	}
	return im;
}

Pixmap
pixmap_from_asimage(ASImage *im, Window w, GC gc, Bool use_cached)
{
	XImage       *xim ;
	Pixmap        p = None;
	XWindowAttributes attr;

	if( XGetWindowAttributes (dpy, w, &attr) )
	{
		set_ascolor_depth( w, attr.depth );
		if ( !use_cached || im->ximage == NULL )
			xim = ximage_from_asimage( im, attr.depth );
		else
			xim = im->ximage ;
		if (xim != NULL )
		{
			p = XCreatePixmap( dpy, w, xim->width, xim->height, attr.depth );
			XPutImage( dpy, p, gc, xim, 0, 0, 0, 0, xim->width, xim->height );
			if( xim != im->ximage )
				XDestroyImage (xim);
		}
	}
	return p;
}



