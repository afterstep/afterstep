
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
asimage_add_line (ASImage * im, ColorPart color, CARD8 * data, unsigned int y)
{
	int           i;
	register int  rep_count, block_count;
	register CARD8 *ptr = data;
	register CARD8 *bstart, *ccolor;
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
					*(tail++) = (CARD8) rep_count;
					*(tail++) = *ccolor;
				} else
				{
					*(tail++) = ((CARD8) (rep_count >> 8)) & RLE_LONG_D;
					*(tail++) = (CARD8) (rep_count) & 0xFF;
					*(tail++) = *ccolor;
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
				block_count -= rep_count + 1 + 1;
				*(tail++) = RLE_DIRECT_B | (CARD8) block_count;
				while (bstart < ccolor)
					*(tail++) = *(bstart++);
				block_count = rep_count;
			}
		ptr++;
	}
	if (block_count > 0)
	{
		if (bstart < ccolor || rep_count == 0)
		{									   /* count of 1 is represented by 0 */
			*(tail++) = RLE_DIRECT_B | (CARD8) (block_count - 1);
			while (bstart < ptr)
				*(tail++) = *(bstart++);
		} else
		{
			if (rep_count <= RLE_MAX_SIMPLE_LEN)
			{								   /* count of 1 is represented by 0 as well but it should not be less the 2 :) */
				*(tail++) = (CARD8) rep_count;
				*(tail++) = *ccolor;
			} else
			{
				*(tail++) = ((CARD8) (rep_count >> 8)) & RLE_LONG_D;
				*(tail++) = (CARD8) (rep_count) & 0xFF;
				*(tail++) = *ccolor;
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
asimage_decode_line (ASImage * im, ColorPart color, unsigned int y, CARD8 * to_buf)
{
	CARD8       **color_ptr;
	register CARD8 *src, *dst;
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
			src++;
			while (to_write-- >= 0)			   /* we start counting from 0 - 0 is actually count of 1 */
				*(dst++) = *(src++);
		} else
		{
			if (((*src) & RLE_LONG_B) != 0)
				src++;
			src++;
			while (to_write-- >= 0)
				*(dst++) = *src;
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
			*(dst++) = *(src++);
			*(dst++) = *(src++);
		} else if (((*src) & RLE_LONG_B) != 0)
		{
			*(dst++) = *(src++);
			*(dst++) = *(src++);
			*(dst++) = *(src++);
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

typedef struct XImageBuffer
{

	CARD8        *red, *green, *blue;

	int           bpp, byte_order, BGR_mode;
/*    CARD32 r_mask, g_mask, b_mask ; */

}
XImageBuffer;

static void
asimage_line_from_ximage (unsigned char *xim_line, unsigned int y, ASImage * im, XImageBuffer * xim_buf)
{
	int           i;
	ColorPart     color1 = IC_RED, color2 = IC_BLUE;
	register CARD8 *r = xim_buf->red, *g = xim_buf->green, *b = xim_buf->blue;

	if (xim_buf->bpp == 24)
	{
		register CARD8 *src = xim_line;

		if (xim_buf->BGR_mode != xim_buf->byte_order)
		{
			color1 = IC_BLUE;
			color2 = IC_RED;
		}
		for (i = im->width; i > 0; i--)
		{
			*(r++) = *(src++);
			*(g++) = *(src++);
			*(b++) = *(src++);
		}
	} else if (xim_buf->bpp == 32)
	{
		register CARD8 *src = xim_line;

		if (xim_buf->BGR_mode != xim_buf->byte_order)
		{
			color1 = IC_BLUE;
			color2 = IC_RED;
		}
		if (xim_buf->byte_order == MSBFirst)
			for (i = im->width; i > 0; i--)
			{
				src++;
				*(r++) = *(src++);
				*(g++) = *(src++);
				*(b++) = *(src++);
		} else
			for (i = im->width; i > 0; i--)
			{
				*(r++) = *(src++);
				*(g++) = *(src++);
				*(b++) = *(src++);
				src++;
			}

	} else if (xim_buf->bpp == 16)
	{										   /* must add LSB/MSB checking + BGR checking */
		register CARD16 *src = (CARD16 *) xim_line;

		for (i = im->width; i > 0; i--)
		{
			*(r++) = ((*src) & 0xF800) >> 11;
			*(g++) = (((*src) & 0x07E0) >> 5) & 0x003F;
			*(b++) = ((*src) & 0x001F);
			src++;
		}
	} else if (xim_buf->bpp == 15)
	{										   /* must add LSB/MSB checking + BGR checking */
		register CARD16 *src = (CARD16 *) xim_line;

		for (i = im->width; i > 0; i--)
		{
			*(r++) = ((*src) & 0x7C00) >> 10;
			*(g++) = (((*src) & 0x03E0) >> 5) & 0x001F;
			*(b++) = ((*src) & 0x001F);
			src++;
		}
	} else
	{										   /* add 8bpp and below handling */

	}

	asimage_add_line (im, color1, xim_buf->red, y);
	asimage_add_line (im, IC_GREEN, xim_buf->green, y);
	asimage_add_line (im, color2, xim_buf->blue, y);
}

ASImage      *
asimage_from_ximage (XImage * xim)
{
	ASImage      *im = NULL;
	unsigned char *xim_line;
	int           i, height, bpl;
	XImageBuffer  xim_buf;

	if (xim == NULL)
		return im;

	im = (ASImage *) safemalloc (sizeof (ASImage));
	asimage_init (im, False);

	xim_buf.red = safemalloc (xim->width);
	xim_buf.green = safemalloc (xim->width);
	xim_buf.blue = safemalloc (xim->width);
	xim_buf.bpp = xim->depth;
	xim_buf.byte_order = xim->byte_order;
	xim_buf.BGR_mode = 0;					   /* will change later */

	asimage_start (im, xim->width, xim->height);

	height = xim->height;
	bpl = xim->bytes_per_line;
	xim_line = xim->data;

	for (i = 0; i < height; i++)
	{
		asimage_line_from_ximage (xim_line, i, im, &xim_buf);
		xim_line += bpl;
	}

	free (xim_buf.red);
	free (xim_buf.green);
	free (xim_buf.blue);

	return im;
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
