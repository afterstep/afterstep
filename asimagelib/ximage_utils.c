/*
 *  Copyright (c) 1999 Sasha Vasko <sashav@sprintmail.com>
 *  most of the code were taken from XFree86 sources
 *  below is the copyright notice for XFree86
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* $TOG: ImUtil.c /main/48 1998/02/06 17:37:38 kaleb $ */
/*

   Copyright 1986, 1998  The Open Group

   All Rights Reserved.

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
   OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
   AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of The Open Group shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from The Open Group.

 */
/*#define DO_CLOCKING      */
/*#define GETPIXEL_PUTPIXEL */

/*#define ATERM */

#ifndef ATERM
#include "../configure.h"
#else
#include "../config.h"
#endif

#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT) || !defined(ATERM)

#include <X11/Intrinsic.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>

#ifndef RUINT32T
#include <X11/Xmd.h>
#define RUINT32T CARD32
#endif

#include <stdio.h>
#ifdef DO_CLOCKING
#include <time.h>
#endif

#ifdef __STDC__
#define Const const
#else
#define Const
/**/
#endif

#ifndef ATERM
#include "../include/aftersteplib.h"
#define NO_XIMAGE_PROTO 
#include "../include/XImage_utils.h"
#else
#define safemalloc(x) malloc(x)
#include "rxvt.h"
#endif

#ifndef LIBAFTERSTEP_HAS_XIMAGE_UTILS
/*
 * Macros
 * 
 * The ROUNDUP macro rounds up a quantity to the specified boundary,
 * then truncates to bytes.
 *
 */

#define ROUNDUP(nbytes, pad) ((((nbytes) + ((pad)-1)) / (pad)) * ((pad)>>3))

/* 
 * GetXImageDataSize calculates the size of memory needed to store XImage
 * pixel's data
 * 
 */

#define XIMAGE_PLANE_SIZE(img)  ((img)->bytes_per_line*(img)->height)

unsigned long
GetXImageDataSize (XImage * ximage)
{
  unsigned long dsize;
  dsize = XIMAGE_PLANE_SIZE (ximage);
  if (ximage->format == XYPixmap)
    dsize = dsize * ximage->depth;

  return dsize;
}

/*
 * CreateXImageBySample
 * 
 * Creates a new image that has same characteristics as an existing one.
 * Allocates the memory necessary for the new XImage data structure. 
 * Pointer to new image is returned. Do not copy pixels from one image 
 * to another
 *
 */
 /*
    * _DestroyImage
    *   
    * Deallocates the memory associated with the ximage data structure. 
    * this version handles the case of the image data being malloc'd
    * entirely by the library.
  */

int
MyXDestroyImage (ximage)
     XImage *ximage;

{
  if (ximage->data != NULL)
    free ((char *) ximage->data);
  if (ximage->obdata != NULL)
    free ((char *) ximage->obdata);
  free ((char *) ximage);
  return 1;
}

void _XInitImageFuncPtrs (XImage *);

static XImage *
CreateXImageBySample (ximage, width, height)
     XImage *ximage;
     unsigned int width;	/* width in pixels of new subimage */
     unsigned int height;	/* height in pixels of new subimage */
{
  register XImage *subimage;
  unsigned long dsize;
  char *data;

  if ((subimage = (XImage *) calloc (1, sizeof (XImage))) == NULL)
    return (XImage *) NULL;
  subimage->width = width;
  subimage->height = height;
  subimage->xoffset = 0;
  subimage->format = ximage->format;
  subimage->byte_order = ximage->byte_order;
  subimage->bitmap_unit = ximage->bitmap_unit;
  subimage->bitmap_bit_order = ximage->bitmap_bit_order;
  subimage->bitmap_pad = ximage->bitmap_pad;
  subimage->bits_per_pixel = ximage->bits_per_pixel;
  subimage->depth = ximage->depth;
  /*
   * compute per line accelerator.
   */
  if (subimage->format == ZPixmap)
    subimage->bytes_per_line =
      ROUNDUP (subimage->bits_per_pixel * width,
	       subimage->bitmap_pad);
  else
    subimage->bytes_per_line =
      ROUNDUP (width, subimage->bitmap_pad);
  subimage->obdata = NULL;
  _XInitImageFuncPtrs (subimage);
  subimage->f.destroy_image = MyXDestroyImage;

  dsize = GetXImageDataSize (subimage);
  if (((data = (char *) calloc (1, dsize)) == NULL) && (dsize > 0))
    {
      Xfree ((char *) subimage);
      return (XImage *) NULL;
    }
  subimage->data = data;

  return subimage;
}

XImage *
CreateXImageAndData (Display * dpy, Visual * visual,
		     unsigned int depth, int format, int offset,
		     unsigned int width, unsigned int height)
{
  register XImage *ximage;
  unsigned long dsize;
  char *data;

  ximage = XCreateImage (dpy, visual, depth, format, offset, NULL, width, height,
			 dpy->bitmap_unit, 0);
  if (ximage != NULL)
    {
      dsize = GetXImageDataSize (ximage);
      if (((data = calloc (1, dsize)) == NULL) && (dsize > 0))
	{
	  Xfree ((char *) ximage);
	  ximage = (XImage *) NULL;
	}
      else
	ximage->data = data;
    }
  return ximage;
}

/* 
 * This function does not perform any error checking becouse of the
 * performance considerations - do all the checks yourself !!!
 *
 * This function will copy entire line from source image into 
 * target image
 * Can be used for fast XImage transformations
 * XImages must be of the same type, and horizontal size
 *
 * This function does not perform any error checking becouse of the
 * performance considerations - do all the checks yourself !!!
 *
 */
void
CopyXImageLine (XImage * src, XImage * trg, int src_y, int trg_y)
{
  if (src->format == ZPixmap)
    memcpy (trg->data + trg->bytes_per_line * trg_y,
	    src->data + src->bytes_per_line * src_y,
	    src->bytes_per_line);
  else
    {
      long plane_size = XIMAGE_PLANE_SIZE (src);
      long src_offset = src->bytes_per_line * src_y;
      long trg_offset = trg->bytes_per_line * trg_y;
      register int i = 0;

      for (; i < src->depth; i++)
	{
	  memcpy (trg->data + trg_offset,
		  src->data + src_offset,
		  src->bytes_per_line);
	  src_offset += plane_size;
	  trg_offset += plane_size;
	}
    }
}

#endif /* #ifndef LIBAFTERSTEP_HAS_XIMAGE_UTILS */

#ifndef LIBAFTERSTEP_HAS_SCALEXIMAGE

/*
 * This function will create array of size of the target image length/width
 * filled with x/y coordinates for pixel in source image
 */

void
Scale (int *target, int target_size,
       int src_start, int src_end)
{
  int trg_first_half_end, src_first_half_end;

  if (target_size > 1)
    {
      if (src_start == src_end)
	{
	  register int i = 0;
	  for (; i < target_size; i++)
	    *(target + i) = src_start;
	}
      else
	{
	  src_first_half_end = (src_start + src_end) >> 1;
	  trg_first_half_end = target_size >> 1;
	  Scale (target, trg_first_half_end, src_start, src_first_half_end);
	  Scale (target + trg_first_half_end, target_size - trg_first_half_end,
		 src_first_half_end + 1, src_end);
	}
    }
  else if (target_size == 1)
    *target = (src_start + src_end) >> 1;
}

/*
 * This function will actually create scaled image from source image
 */

#if 0
void
XQueryC (Display * display, Colormap cmap, XColor * color)
{
  color->red = (color->pixel & 0xf800) << 0;
  color->green = (color->pixel & 0x07e0) << 5;
  color->blue = (color->pixel & 0x001f) << 11;
}

void
XAllocC (Display * display, Colormap cmap, XColor * color)
{
  color->pixel = (color->red & 0xf800) | ((color->green >> 5) & 0x7e0) | (color->blue >> 11);
}

#define modf(a,b) my_modf(a,b)
double
my_modf (double x, double *nptr)
{
  double r = x - (int) x;
  if (nptr != NULL)
    *nptr = (int) x;
  return r;
}

XImage *
ScaleXImageToSize (XImage * src, int width, int height)
{
#ifdef DO_CLOCKING
  clock_t started = clock ();
#endif
  XImage *dst;
  Colormap cmap = DefaultColormap (dpy, DefaultScreen (dpy));
  if ((dst = CreateXImageBySample (src, width, height)) != NULL)
    {
      int dx, dy;
/* scattering only looks good for small images; only do it if 
   ** (width >= src->width * 3 && height >= src->height * 3) */
#if defined SCALE_SCATTER
      for (dy = 0; dy < height; dy++)
	{
	  int ty = dy * src->height / height;
	  for (dx = 0; dx < width; dx++)
	    {
	      int sx = dx * src->width / width;
	      int sy = ty;
	      if (sx > 0 && sx < src->width - 1)
		sx += rand () % 3 - 1;
	      if (sy > 0 && sy < src->height - 1)
		sy += rand () % 3 - 1;
	      XPutPixel (dst, dx, dy, XGetPixel (src, sx, sy));
	    }
	}
/* simple scaling algorithm that's useful for non-truecolor images */
#elif defined SCALE_SIMPLE
      for (dy = 0; dy < height; dy++)
	for (dx = 0; dx < width; dx++)
	  XPutPixel (dst, dx, dy, XGetPixel (src, dx * src->width / width, dy * src->height / height));
/* this one is slow, but very accurate; destination pixels are the sum 
   ** of weighted source pixels; the weight is the percentage of the source 
   ** pixel that is contained in the destination pixel */
#else /* SCALE_SLOW */
      double wx = (double) src->width / width;
      double wy = (double) src->height / height;
      double w = 1.0 / (wx * wy);
      double sy = 0.0;
      for (dy = 0; dy < height; dy++)
	{
	  double sy1 = (int) sy, py1 = 1.0 - (sy - sy1), sy2 = (int) (sy + wy),
	    py2 = sy + wy - sy2;
	  double sx = 0.0;
	  if (sy1 == sy2)
	    py1 = wy;
	  for (dx = 0; dx < width; dx++)
	    {
	      double sx1 = (int) sx, px1 = 1.0 - (sx - sx1), sx2 = (int) (sx + wx),
	        px2 = sx + wx - sx2;
	      double red = 0.5, green = 0.5, blue = 0.5;
	      int x, y;
	      XColor c;
	      if (sx1 == sx2)
		px1 = wx;
	      for (y = sy1; y < sy2 + 1; y++)
		{
		  double w1 = w;
		  if (y == sy1)
		    w1 *= py1;
		  else if (y == sy2)
		    w1 *= py2;
		  for (x = sx1; x < sx2 + 1; x++)
		    {
		      double w2 = w1;
		      if (x == sx1)
			w2 *= px1;
		      else if (x == sx2)
			w2 *= px2;
		      c.pixel = XGetPixel (src, x, y);
		      XQueryC (dpy, cmap, &c);
		      red += (double) c.red * w2;
		      green += (double) c.green * w2;
		      blue += (double) c.blue * w2;
		    }
		}
	      c.red = red;
	      c.green = green;
	      c.blue = blue;
	      XAllocC (dpy, cmap, &c);
	      XPutPixel (dst, dx, dy, c.pixel);
	      sx += wx;
	    }
	  sy += wy;
	}
#endif
    }
#ifdef DO_CLOCKING
  printf ("Scaling time (seconds): %.3f\n", (double) (clock () - started) / CLOCKS_PER_SEC);
#endif
  return dst;
}
#else
XImage *
ScaleXImageToSize (src, width, height)
     XImage *src;
     int width, height;
{
  XImage *dst;
  int y;
  register int x;
  int *x_net = NULL, *y_net = NULL;
#ifdef DO_CLOCKING
  clock_t started = clock ();
#endif
#ifdef ATERM
  Visual *visual = DefaultVisual (Xdisplay, Xscreen);
#else
  Visual *visual = DefaultVisual (dpy, DefaultScreen (dpy));
#endif
#define VISUAL visual


  if (width == 0 || height == 0)
    return NULL;

  if (width == src->width && height == src->height)
    {
      dst = XSubImage (src, 0, 0, width, height);
    }
  else
    {
      /* It would be nice average the skipped pixels. */
      if ((dst = CreateXImageBySample (src, width, height)) != NULL)
	{
#ifndef GETPIXEL_PUTPIXEL
	  unsigned char bytes_per_pixel = src->bits_per_pixel >> 3;
	  unsigned char *dst_start = (unsigned char *) dst->data;
	  if (src->bits_per_pixel == 15)
	    bytes_per_pixel = 2;
#endif

	  if (width != src->width)
	    {
	      x_net = (int *) safemalloc (sizeof (int) * width);
	      Scale (x_net, width, 0, src->width - 1);
#ifndef GETPIXEL_PUTPIXEL
	      if (src->bits_per_pixel >= 8 && src->format == ZPixmap && visual->class == TrueColor)
		for (x = 0; x < width; x++)
		  x_net[x] = x_net[x] * bytes_per_pixel;
#endif
	    }
	  y_net = (int *) safemalloc (sizeof (int) * height);
	  Scale (y_net, height, 0, src->height - 1);

	  for (y = 0; y < height; y++)
	    {

	      if (y > 0)
		if (y_net[y] == y_net[y - 1])
		  {
		    CopyXImageLine (dst, dst, y - 1, y);
#ifndef GETPIXEL_PUTPIXEL
		    dst_start += dst->bytes_per_line;
#endif
		    continue;
		  }
	      if (width == src->width)
		{
		  CopyXImageLine (src, dst, y_net[y], y);
		}
	      else
#ifndef GETPIXEL_PUTPIXEL
	      if (src->bits_per_pixel >= 8 && src->format == ZPixmap && visual->class == TrueColor)
		{		/* this should be a faster variant */
		  unsigned char *src_start = (unsigned char *) (src->data + y_net[y] * src->bytes_per_line);
		  unsigned char *dst_pos = dst_start;
		  unsigned char *src_pos;
		  register int i;
		  for (x = 0; x < width; x++)
		    {
		      src_pos = src_start + x_net[x];
		      for (i = 0; i < bytes_per_pixel; i++)
			*dst_pos++ = *src_pos++;
		    }
		  dst_start += dst->bytes_per_line;
		}
	      else
#endif
	      if (width > src->width)
		{

		  Pixel pix = XGetPixel (src, x_net[0], y_net[y]);
		  XPutPixel (dst, 0, y, pix);
		  for (x = 1; x < width; x++)
		    {
		      if (x_net[x] != x_net[x - 1])
			pix = XGetPixel (src, x_net[x], y_net[y]);
		      XPutPixel (dst, x, y, pix);
		    }
		}
	      else
		{
		  for (x = 0; x < width; x++)
		    XPutPixel (dst, x, y, XGetPixel (src, x_net[x], y_net[y]));
		}
	    }
	  if (x_net)
	    free (x_net);
	  if (y_net)
	    free (y_net);
	}
    }
  return (dst);
}
#endif

XImage *
ScaleXImage (src, scale_x, scale_y)
     XImage *src;
     double scale_x, scale_y;
{
  return ScaleXImageToSize (src,
			    (Dimension) max (scale_x * src->width, 1),
			    (Dimension) max (scale_y * src->height, 1));
}

#endif /*#ifndef LIBAFTERSTEP_HAS_SCALEXIMAGE */

#ifndef LIBAFTERSTEP_HAS_SHADEXIMAGE

void
ShadeXImage (XImage * srcImage, ShadingInfo * shading, GC gc)
{
#ifdef DO_CLOCKING
  clock_t started = clock ();
#endif
  unsigned int int_rm, int_gm, int_bm;
  int shade = 0;

#ifdef ATERM
  Visual *visual = DefaultVisual (Xdisplay, Xscreen);
#else
  Visual *visual = DefaultVisual (dpy, DefaultScreen (dpy));
#endif

#define VISUAL visual

  if (visual->class != TrueColor || srcImage->format != ZPixmap)
    return;
  if (srcImage->bits_per_pixel <= 8)
    return;
  if (shading->shading < 0)
    shade = 100 - shading->shading;	/*we want to lighten 
					   instead of darken */
  else
    shade = shading->shading;
  if (shade > 200)
    shade = 200;
  int_rm = (shading->tintColor.red >> 8) * shade / 100;
  int_gm = (shading->tintColor.green >> 8) * shade / 100;
  int_bm = (shading->tintColor.blue >> 8) * shade / 100;

#ifdef GETPIXEL_PUTPIXEL
  {
    register unsigned long p, x;
    unsigned long y;
    unsigned int rb_mask, g_mask;
    unsigned char r_shift, g_shift, b_shift;

    g_mask = rb_mask = 0xf8;
    b_shift = 3;
    switch (srcImage->bits_per_pixel)
      {
      case 15:
	r_shift = 7;
	g_shift = 2;
	break;
      case 16:
	g_mask = 0xfc;
	r_shift = 8;
	g_shift = 3;
	break;
      case 32:
      case 24:
	g_mask = rb_mask = 0xff;
	r_shift = 16;
	g_shift = 8;
	break;
      }

    if (srcImage->bits_per_pixel <= 16)		/* for better speed we don't want to check for it 
						   inside the loop */
      {
	for (y = 0; y < srcImage->height; y++)
	  for (x = 0; x < srcImage->width; x++)
	    {
	      p = XGetPixel (srcImage, x, y);

	      XPutPixel (srcImage, x, y,
			 ((((((p >> r_shift) & rb_mask) * int_rm) / 255) & rb_mask) << r_shift) |
			 ((((((p >> g_shift) & g_mask) * int_gm) / 255) & g_mask) << g_shift) |
			 ((((((p << b_shift) & rb_mask) * int_bm) / 255) & rb_mask) >> b_shift));
	    }
      }
    else
      {
	for (y = 0; y < srcImage->height; y++)
	  for (x = 0; x < srcImage->width; x++)
	    {
	      p = XGetPixel (srcImage, x, y);
	      XPutPixel (srcImage, x, y,
			 ((((((p >> r_shift) & rb_mask) * int_rm) / 255) & rb_mask) << r_shift) |
			 ((((((p >> g_shift) & g_mask) * int_gm) / 255) & g_mask) << g_shift) |
			 ((((p & rb_mask) * int_bm) / 255) & rb_mask));
	    }

      }
  }
#else
  {
    RUINT32T rk = VISUAL->red_mask;
    RUINT32T gk = VISUAL->green_mask;
    RUINT32T bk = VISUAL->blue_mask;

/*      fprintf( stderr, "\n ATERM: RedMask = %x, GreenMask= %x, BlueMask = %x", rk, bk, bk ); */
    /* The following code has been written by 
     * Ethan Fischer <allanon@crystaltokyo.com>
     * for AfterStep window manager. It's much faster then previous */
    /* this code only works for TrueColor (and maybe DirectColor) modes; 
     * it would do very odd things in PseudoColor */
    switch (srcImage->bits_per_pixel)
      {
      case 15:
      case 16:
	{
	  unsigned short *p1 = (unsigned short *) srcImage->data;
	  unsigned short *pf = (unsigned short *) (srcImage->data + srcImage->height * srcImage->bytes_per_line);
	  while (p1 < pf)
	    {
	      unsigned short *p = p1;
	      unsigned short *pl = p1 + srcImage->width;
	      for (; p < pl; p++)
		{
		  RUINT32T v = *p;
		  v = (((v & rk) * int_rm >> 8) & rk) |
		    (((v & gk) * int_gm >> 8) & gk) |
		    (((v & bk) * int_bm >> 8) & bk);
		  *p = v;
		}
	      p1 = (unsigned short *) ((char *) p1 + srcImage->bytes_per_line);
	    }
	  break;
	}
      case 24:
	{
	  unsigned char *p1 = (unsigned char *) srcImage->data;
	  unsigned char *pf = (unsigned char *) (srcImage->data + srcImage->height * srcImage->bytes_per_line);
	  if (rk >= 0xFF0000)	/* we need it for some wierd XServers like XFree86 3.3.3.1 */
	    {
	      unsigned int int_tmp = int_rm;
	      int_rm = int_bm;
	      int_bm = int_tmp;
	    }
	  while (p1 < pf)
	    {
	      unsigned char *p = p1;
	      unsigned char *pl = p1 + srcImage->width * 3;
	      for (; p < pl; p += 3)
		{
		  p[0] = (unsigned long) p[0] * int_rm >> 8;
		  p[1] = (unsigned long) p[1] * int_gm >> 8;
		  p[2] = (unsigned long) p[2] * int_bm >> 8;
		}
	      p1 = (unsigned char *) ((char *) p1 + srcImage->bytes_per_line);
	    }
	  break;
	}
      case 32:
	{
	  RUINT32T *p1 = (RUINT32T *) srcImage->data;
	  RUINT32T *pf = (RUINT32T *) (srcImage->data + srcImage->height * srcImage->bytes_per_line);

	  while (p1 < pf)
	    {
	      RUINT32T *p = p1;
	      RUINT32T *pl = p1 + srcImage->width;
	      for (; p < pl; p++)
		{
		  RUINT32T v = *p;
		  v = (v & ~rk) | (((v & rk) * int_rm / 256) & rk);
		  v = (v & ~gk) | (((v & gk) * int_gm / 256) & gk);
		  v = (v & ~bk) | (((v & bk) * int_bm / 256) & bk);
		  *p = v;
		}
	      p1 = (RUINT32T *) ((char *) p1 + srcImage->bytes_per_line);
	    }
	  break;
	}
      }
  }
#endif

#ifdef DO_CLOCKING
  printf ("\n Shading time (clocks): %lu\n", clock () - started);
#endif

}

int
CombinePixmapWithXImage (Pixmap dest, XImage * src, int x, int y, int width, int height)
{
  XImage *image;

  if (dest == None || src == NULL || (image = XGetImage (dpy, dest, x, y, width, height, AllPlanes, src->format)) == NULL)
    return 0;
  CombineXImageWithXImage (image, src);
  XPutImage (dpy, dest, DefaultGC (dpy, DefaultScreen (dpy)), image, 0, 0, x, y, width, height);
  XDestroyImage (image);
  return 1;
}

void
CombineXImageWithXImage (XImage * dest, XImage * src)
{
#ifdef DO_CLOCKING
  clock_t started = clock ();
#endif

#ifdef ATERM
  Visual *visual = DefaultVisual (Xdisplay, Xscreen);
#else
  Visual *visual = DefaultVisual (dpy, DefaultScreen (dpy));
#endif

  if (visual->class != TrueColor ||
      src->format != ZPixmap ||
      dest->format != ZPixmap ||
      src->bits_per_pixel <= 8 ||
      src->bits_per_pixel != dest->bits_per_pixel)
    return;

#ifdef GETPIXEL_PUTPIXEL
  {
    register unsigned long p, x;
    unsigned long y;
    unsigned int rb_mask, g_mask;
    unsigned char r_shift, g_shift, b_shift;

    g_mask = rb_mask = 0xf8;
    b_shift = 3;
    switch (src->bits_per_pixel)
      {
      case 15:
	r_shift = 7;
	g_shift = 2;
	break;
      case 16:
	g_mask = 0xfc;
	r_shift = 8;
	g_shift = 3;
	break;
      case 32:
      case 24:
	g_mask = rb_mask = 0xff;
	r_shift = 16;
	g_shift = 8;
	break;
      }

    if (src->bits_per_pixel <= 16)	/* for better speed we don't want to check for it 
					   inside the loop */
      {
	for (y = 0; y < src->height; y++)
	  for (x = 0; x < src->width; x++)
	    {
	      p = XGetPixel (src, x, y);

	      XPutPixel (src, x, y,
			 ((((((p >> r_shift) & rb_mask) * int_rm) / 255) & rb_mask) << r_shift) |
			 ((((((p >> g_shift) & g_mask) * int_gm) / 255) & g_mask) << g_shift) |
			 ((((((p << b_shift) & rb_mask) * int_bm) / 255) & rb_mask) >> b_shift));
	    }
      }
    else
      {
	for (y = 0; y < src->height; y++)
	  for (x = 0; x < src->width; x++)
	    {
	      p = XGetPixel (src, x, y);
	      XPutPixel (src, x, y,
			 ((((((p >> r_shift) & rb_mask) * int_rm) / 255) & rb_mask) << r_shift) |
			 ((((((p >> g_shift) & g_mask) * int_gm) / 255) & g_mask) << g_shift) |
			 ((((p & rb_mask) * int_bm) / 255) & rb_mask));
	    }

      }
  }
#else
  {
    RUINT32T rk = visual->red_mask;
    RUINT32T gk = visual->green_mask;
    RUINT32T bk = visual->blue_mask;

    /* The following code has been written by 
     * Ethan Fischer <allanon@crystaltokyo.com>
     * for AfterStep window manager. It's much faster then previous */
    /* this code only works for TrueColor (and maybe DirectColor) modes; 
     * it would do very odd things in PseudoColor */
    switch (src->bits_per_pixel)
      {
      case 15:
      case 16:
	{
	  unsigned short *p1 = (unsigned short *) dest->data;
	  unsigned short *pf = (unsigned short *) (dest->data + dest->height * dest->bytes_per_line);
	  unsigned short *p2 = (unsigned short *) src->data;
	  unsigned short *p2f = (unsigned short *) (src->data + src->height * src->bytes_per_line);
	  while (p1 < pf)
	    {
	      unsigned short *p = p1;
	      unsigned short *pl = p1 + dest->width;
	      unsigned short *q = p2;
	      unsigned short *ql = p2 + src->width;
	      for (; p < pl; p++)
		{
		  RUINT32T u = *p, v = *q;
		  u = ((((u & rk) + (v & rk)) >> 1) & rk) |
		    ((((u & gk) + (v & gk)) >> 1) & gk) |
		    ((((u & bk) + (v & bk)) >> 1) & bk);
		  *p = u;
		  if (++q > ql)
		    q = p2;
		}
	      p1 = (unsigned short *) ((char *) p1 + dest->bytes_per_line);
	      p2 = (unsigned short *) ((char *) p2 + src->bytes_per_line);
	      if (p2 >= p2f)
		p2 = (unsigned short *) src->data;
	    }
	  break;
	}
      case 24:
	{
	  unsigned char *p1 = (unsigned char *) dest->data;
	  unsigned char *pf = (unsigned char *) (dest->data + dest->height * dest->bytes_per_line);
	  unsigned char *p2 = (unsigned char *) src->data;
	  unsigned char *p2f = (unsigned char *) (src->data + src->height * src->bytes_per_line);
	  while (p1 < pf)
	    {
	      unsigned char *p = p1;
	      unsigned char *pl = p1 + dest->width * 3;
	      unsigned char *q = p2;
	      unsigned char *ql = p2 + src->width * 3;
	      for (; p < pl; p += 3)
		{
		  p[0] = ((unsigned long) p[0] + q[0]) >> 1;
		  p[1] = ((unsigned long) p[1] * q[1]) >> 1;
		  p[2] = ((unsigned long) p[2] * q[2]) >> 1;
		  if ((q += 3) > ql)
		    q = p2;
		}
	      p1 = (unsigned char *) ((char *) p1 + dest->bytes_per_line);
	      p2 = (unsigned char *) ((char *) p2 + src->bytes_per_line);
	      if (p2 >= p2f)
		p2 = (unsigned char *) src->data;
	    }
	  break;
	}
      case 32:
	{
	  RUINT32T *p1 = (RUINT32T *) dest->data;
	  RUINT32T *pf = (RUINT32T *) (dest->data + dest->height * dest->bytes_per_line);
	  RUINT32T *p2 = (RUINT32T *) src->data;
	  RUINT32T *p2f = (RUINT32T *) (src->data + src->height * src->bytes_per_line);
	  while (p1 < pf)
	    {
	      RUINT32T *p = p1;
	      RUINT32T *pl = p1 + dest->width;
	      RUINT32T *q = p2;
	      RUINT32T *ql = p2 + src->width;
	      for (; p < pl; p++)
		{
		  RUINT32T u = *p, v = *q;
		  u = ((((u & rk) + (v & rk)) >> 1) & rk) |
		    ((((u & gk) + (v & gk)) >> 1) & gk) |
		    ((((u & bk) + (v & bk)) >> 1) & bk);
		  *p = u;
		  if (++q > ql)
		    q = p2;
		}
	      p1 = (RUINT32T *) ((char *) p1 + dest->bytes_per_line);
	      p2 = (RUINT32T *) ((char *) p2 + src->bytes_per_line);
	      if (p2 >= p2f)
		p2 = (RUINT32T *) src->data;
	    }
	  break;
	}
      }
  }
#endif

#ifdef DO_CLOCKING
  printf ("\n Combining time (clocks): %lu\n", clock () - started);
#endif

}

#endif /* #ifndef LIBAFTERSTEP_HAS_SHADEXIMAGE */
#endif /* defined(BACKGROUND_IMAGE) || defined(TRANSPARENT) || !defined(ATERM) */
