
#include "../configure.h"

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>

#ifdef ISC									   /* Saul */
#include <sys/bsdtypes.h>					   /* Saul */
#endif /* Saul */

#include <stdlib.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#ifdef I18N
#include <X11/Xlocale.h>
#endif

#define IN_MODULE
#define MODULE_X_INTERFACE

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/style.h"
#include "../include/screen.h"
#include "../include/ascolor.h"
#include "../include/mytexture.h"
#include "../include/module.h"
#include "../include/parser.h"
#include "../include/confdefs.h"
#include "../include/mystyle.h"
#include "../include/background.h"
#include "../include/asimage.h"
#include "../include/XImage_utils.h"

/*
 * We Store images using RLE encoding - see asimage.h for more
 */

#ifndef NO_INLINE
inline unsigned int
_asimage_get_length (CARD8 * cblock)
{
	if (((*cblock) & RLE_DIRECT_B) != 0)
		return (*cblock) & (RLE_DIRECT_D);
	if (((*cblock) & RLE_LONG_B) != 0)
		return ((((*cblock) & RLE_LONG_D) << 8) | *(cblock + 1));
	return (*cblock) & (RLE_SIMPLE_D);
}
#else

#define _asimage_get_length(cblock) \
    ((((*cblock)&RLE_DIRECT_B)!= 0 )?   (*cblock) & (RLE_DIRECT_D): \
     ((((*cblock)&RLE_LONG_B) != 0 )?((((*cblock) & RLE_LONG_D)<<8)| *(cblock+1)): \
                                        (*cblock) & (RLE_SIMPLE_D)))
#endif


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
		im->buf_len = 1 + width + width / RLE_MAX_DIRECT_LEN + 1;
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

static CARD8 **
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
asimage_add_line (ASImage * im, ColorPart color, CARD32 * data, unsigned int y)
{
	int           i;
	register int  rep_count, block_count;
	register CARD32 *ptr = data;
	register CARD32 *bstart, *ccolor;
	unsigned int  width;
	CARD8        *tail;
	Bool          direct = True;

	if (im == NULL || data == NULL)
		return;
	if (im->buffer == NULL || y >= im->height)
		return;

	bstart = ccolor = ptr;
	width = im->width;
	tail = im->buffer;
	rep_count = block_count = 0;

	for (i = 0; i < width; i++)
	{
		if (*ptr == *ccolor)
		{
			rep_count++;
			if (direct && rep_count >= RLE_THRESHOLD)
				direct = False;
		} else
		{
			if (!direct)
			{
				if (rep_count <= RLE_MAX_SIMPLE_LEN)
				{
					tail[0] = (CARD8)  rep_count;
					tail[1] = (CARD8) *ccolor;
					tail += 2 ;
				} else
				{
					tail[0] = ((CARD8) (rep_count >> 8)) & RLE_LONG_D;
					tail[1] = (CARD8) (rep_count) & 0xFF;
					tail[2] = (CARD8) *ccolor;
					tail += 3 ;
				}
				block_count = 0;
				bstart = ptr;
				direct = True;
			}
			ccolor = ptr;
			rep_count = 0;
		}
		block_count++;
		if (!direct || block_count >= RLE_MAX_DIRECT_LEN)
			if (bstart < ccolor)
			{
				register int i = ccolor - bstart ;
				block_count -= rep_count + 1 + 1;
				*(tail++) = RLE_DIRECT_B | (CARD8) block_count;
				while ( --i >= 0 )
					tail[i] = (CARD8)bstart[i];
				tail += ccolor - bstart ;
				bstart = ccolor;
				block_count = rep_count;
			}
		ptr++;
	}
	if (block_count > 0)
	{
		if (bstart < ccolor || rep_count == 0)
		{									   /* count of 1 is represented by 0 */
			register int i = ptr - bstart;
			*(tail++) = RLE_DIRECT_B | (CARD8) (block_count - 1);
			while (--i >= 0 )
				tail[i] = (CARD8)bstart[i];
			tail  += ptr - bstart;
			bstart = ptr;
		} else
		{
			if (rep_count <= RLE_MAX_SIMPLE_LEN)
			{								   /* count of 1 is represented by 0 as well but it should not be less the 2 :) */
				tail[0] = (CARD8) rep_count;
				tail[1] = (CARD8) *ccolor;
				tail += 2 ;
			} else
			{
				tail[0] = ((CARD8) (rep_count >> 8)) & RLE_LONG_D;
				tail[1] = (CARD8) (rep_count) & 0xFF;
				tail[2] = (CARD8) *ccolor;
				tail += 3 ;
			}
		}
	}

	*(tail++) = RLE_EOL;

	im->buf_used = tail - im->buffer;
	asimage_apply_buffer (im, color, y);
}

unsigned int
asimage_print_line (ASImage * im, ColorPart color, unsigned int y, unsigned long verbosity)
{
	CARD8       **color_ptr;
	register CARD8 *ptr;
	int           to_skip = 0;

	if (im == NULL)
		return 0;
	if (y >= im->height)
		return 0;

	if ((color_ptr = asimage_get_color_ptr (im, color)) == NULL)
		return 0;
	ptr = color_ptr[y];
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
			to_skip = 1 + ((*ptr) & (RLE_DIRECT_D));
			if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
				fprintf (stderr, " is RLE_DIRECT !");
		} else if (((*ptr) & RLE_SIMPLE_B_INV) == 0)
		{
			if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
				fprintf (stderr, " is RLE_SIMPLE !");
			to_skip = 1;
		} else if (((*ptr) & RLE_LONG_B) != 0)
		{
			if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
				fprintf (stderr, " is RLE_LONG !");
			to_skip = 1 + 1;

		}
		to_skip++;
		if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
			fprintf (stderr, " to_skip = %d\n", to_skip);
	}
	if (get_flags (verbosity, VRB_LINE_CONTENT))
		fprintf (stderr, " %2.2X\n", *ptr);

	ptr++;

	if (get_flags (verbosity, VRB_LINE_SUMMARY))
		fprintf (stderr, "Row %d, Component %d, Memory Used %d\n", y, color, ptr - color_ptr[y]);
	return ptr - color_ptr[y];
}

unsigned int
asimage_decode_line (ASImage * im, ColorPart color, CARD32 * to_buf, unsigned int y)
{
	CARD8       **color_ptr;
	register CARD8  *src ;
	register CARD32 *dst;
	register int  to_write;

	if (im == NULL || to_buf == NULL)
		return 0;
	if (y > im->height)
		return 0;

	if ((color_ptr = asimage_get_color_ptr (im, color)) == NULL)
		return 0;
	src = color_ptr[y];
	dst = to_buf;

	while (*src != RLE_EOL)
	{
		to_write = _asimage_get_length (src);

		if (((*src) & RLE_DIRECT_B) != 0)
		{
			register int i = -1 ;
			src++;
			while (to_write >= ++i)			   /* we start counting from 0 - 0 is actually count of 1 */
				dst[i] = src[i];
			src += i ;
			dst += i ;
		} else
		{
			register int i = -1 ;
			if (((*src) & RLE_LONG_B) != 0)
				src++;
			src++;
			while (to_write >= ++i )
				dst[i] = *src;
    		dst += i ;
			src++;
		}
	}
	return (dst - to_buf);
}

unsigned int
asimage_copy_line (CARD8 * from, CARD8 * to)
{
	register CARD8 *src = from, *dst = to;
	register int  to_write;

	/* merely copying the data */
	if (src == NULL || dst == NULL)
		return 0;
	while (*src != RLE_EOL)
	{
		if (((*src) & RLE_DIRECT_B) != 0)
		{
			to_write = ((*src) & (RLE_DIRECT_D)) + 1;
			while (to_write-- >= 0)			   /* we start counting from 0 - 0 is actually count of 1 */
				*(dst++) = *(src++);
		} else if (((*src) & RLE_SIMPLE_B_INV) == 0)
		{
			dst[0] = src[0];
			dst[1] = src[1];
			dst += 2 ;
			src += 2 ;
		} else if (((*src) & RLE_LONG_B) != 0)
		{
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst += 3 ;
			src += 3 ;
		}
	}
	return (dst - to);
}

unsigned int  asimage_scale_line_down (CARD8 * from, CARD8 * to, unsigned int from_width, unsigned int to_width);

unsigned int
asimage_scale_line_up (CARD8 * from, CARD8 * to, unsigned int from_width, unsigned int to_width)
{
	if (from == NULL || to == NULL)
		return 0;
	if (from_width > to_width)
		return asimage_scale_line_down (from, to, from_width, to_width);
	else if (from_width == to_width)
		return asimage_copy_line (from, to);
	/* TODO : implement scaling up :) */
	return 0;
}

#if 0
unsigned int
asimage_scale_line_down (CARD8 * from, CARD8 * to, unsigned int from_width, unsigned int to_width)
{
	register CARD8 *src = from, *dst = to;

	/* initializing Brsesengham algoritm parameters : */
	int           bre_curr = from_width >> 1, bre_limit = from_width, bre_inc = to_width;

	/* these are used for color averaging : */
	CARD32        total;
	unsigned int  count;

	if (src == NULL || dst == NULL)
		return 0;
	if (from_width <= to_width)
		return asimage_scale_line_up (src, dst, from_width, to_width);

	while (*src != RLE_EOL)
	{
		int           n1, n2;

		n1 = _asimage_get_length (src);
	}
	return (dst - to);
}
#endif


typedef struct ASScanline
{
	CARD32        *buffer ;
	CARD32        *red, *green, *blue;
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
	sl->red 	= sl->buffer = safemalloc (width*3);
	sl->green 	= sl->buffer + width;
	sl->blue 	= sl->buffer + (width<<1);
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
				r[i] =  (src[0]&0xF8)>>3;
				g[i] = ((src[0]&0x07)<<3)|((src[1]&0xE0)>>5);
				b[i] =  (src[1]&0x1F);
				src += 2;
			}
		else
			for (i = 0 ; i < width; i++)
			{
				r[i] =  (src[1]&0xF8) >> 3;
				g[i] = ((src[1]&0x07)<<3)|((src[0]&0xE0)>>5);;
				b[i] =  (src[0]&0x1F);
				src += 2;
			}
	} else if (bpp == 15)
	{										   /* must add LSB/MSB checking */
		if (byte_order == MSBFirst)
			for (i = 0 ; i < width; i++)
			{
				r[i] =  (src[0]&0x7C)>>2;
				g[i] = ((src[0]&0x03)<<3)|((src[1]&0xE0)>>5);
				b[i] =  (src[1]&0x1F);
				src += 2;
			}
		else
			for (i = 0 ; i < width; i++)
			{
				r[i] =  (src[1]&0x7C)>>2;
				g[i] = ((src[1]&0x03)<<3)|((src[0]&0xE0)>>5);;
				b[i] =  (src[0]&0x1F);
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
		if (byte_order == MSBFirst)
			for (i = 0 ; i < width; i++)
			{
				src[0] = (CARD8)((r[i] << 3) | (( g[i] >> 3) & 0x07 ));
				src[1] = (CARD8)((g[i] << 5) |    b[i]);
				src += 2;
			}
		else
			for (i = 0 ; i < width; i++)
			{
				src[1] = (CARD8)((r[i] << 3) | (( g[i] >> 3) & 0x07 ));
				src[0] = (CARD8)((g[i] << 5) |    b[i]);
				src += 2;
			}
	} else if (bpp == 15)
	{										   /* must add LSB/MSB checking */
		if (byte_order == MSBFirst)
			for (i = 0 ; i < width; i++)
			{
				src[0] = (CARD8)((r[i] << 2) | (( g[i] >> 3) & 0x03 ))&0x7F;
				src[1] = (CARD8)((g[i] << 5) |    b[i]);
				src += 2;
			}
		else
			for (i = 0 ; i < width; i++)
			{
				src[1] = (CARD8)((r[i] << 2) | (( g[i] >> 3) & 0x03 ))&0x7F;
				src[0] = (CARD8)((g[i] << 5) |    b[i]);
				src += 2;
			}
	} else
	{										   /* below 8 bpp handling */
		for (i = 0 ; i < width; i++)
			src[i] = color_to_pixel_low( r[i], g[i], b[i] );
	}
}


ASImage      *
asimage_from_ximage (XImage * xim)
{
	ASImage      *im = NULL;
	unsigned char *xim_line;
	int           i, height, bpl;
	ASScanline    xim_buf;

	if (xim == NULL)
		return im;

	im = (ASImage *) safemalloc (sizeof (ASImage));
	asimage_init (im, False);
	asimage_start (im, xim->width, xim->height);

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
asimage_from_pixmap (Pixmap p, int x, int y, unsigned int width, unsigned int height, unsigned long plane_mask)
{
	XImage       *xim = XGetImage (dpy, p, x, y, width, height, plane_mask, ZPixmap);
	ASImage      *im = NULL;

	if (xim)
	{
		im = asimage_from_ximage (xim);
		XDestroyImage (xim);
	}
	return im;
}

Pixmap
pixmap_from_asimage(ASImage *im, Drawable d, GC gc)
{
	XImage       *xim ;
	Pixmap        p = None;
	XWindowAttributes attr;

	if( XGetWindowAttributes (dpy, d, &attr) )
		if ((xim = ximage_from_asimage( im, attr.depth )) != NULL )
		{
			p = XCreatePixmap( dpy, d, im->width, im->height, attr.depth );
			XPutImage( dpy, p, gc, xim, 0, 0, 0, 0, im->width, im->height );
			XDestroyImage (xim);
		}
	return p;
}

