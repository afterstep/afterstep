/*
 * Copyright (c) 1999 Ethan Fischer <allanon@crystaltokyo.com>
 * Copyright (c) 1999 Sasha Vasko <sasha at aftercode.net>
 * Copyright (c) 1996 Alfredo K. Kojima (kojima@inf.ufrgs.br)
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

#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/parse.h"
#include "../include/style.h"
#include "../include/misc.h"
#include "../include/screen.h"
#include "../include/hashtable.h"
#include "../include/ascolor.h"
#include "../include/stepgfx.h"
/*#include "../include/XImage_utils.h"*/

static int table_initialized = 0;
static HashTable ColorTable;
static int Bits;

static int
get_shifts (unsigned long mask)
{
  register int i = 1;

  while (mask >> i)
    i++;

  return i - 1;			/* can't be negative */
}

static int
get_bits (unsigned long mask)
{
  register int i;

  for (i = 0; mask; mask >>= 1)
    if (mask & 1)
      i++;

  return i;			/* can't be negative */
}

static Colormap ascolor_colormap = None;
static Visual *ascolor_visual = NULL;
static int ascolor_rshift, ascolor_gshift, ascolor_bshift;
static int ascolor_rbits, ascolor_gbits, ascolor_bbits;
int ascolor_true_depth = 0;	/* depth for true color - 0 otherwise */
int BGR_mode = 0;

unsigned long
ascolor_to_pixel_default (ASCOLOR ascol)
{
  XColor color;
  unsigned long key, r, g, b;

  color.red = ASCOLOR_RED16 (ascol);
  color.green = ASCOLOR_GREEN16 (ascol);
  color.blue = ASCOLOR_BLUE16 (ascol);
  r = color.red >> Bits;
  g = color.green >> Bits;
  b = color.blue >> Bits;
  key = r * r + g * g + b * b;

  if (!table_iget (&ColorTable, key, &(color.pixel)))
    {
      /* color not in cache, have to allocate this new color */
      if (XAllocColor (dpy, ascolor_colormap, &color) == 0)
	return 0;
      /* put color in cache */
      table_iput (&ColorTable, key, color.pixel);
    }
  return color.pixel;
}

unsigned long
ascolor_to_pixel_low (ASCOLOR ascol)
{
  return (((ASCOLOR_RED16 (ascol) >> ascolor_rshift) & ascolor_visual->red_mask) |
	  ((ASCOLOR_GREEN16 (ascol) >> ascolor_gshift) & ascolor_visual->green_mask) |
  ((ASCOLOR_BLUE16 (ascol) >> ascolor_bshift) & ascolor_visual->blue_mask));
}

unsigned long
ascolor_to_pixel15 (ASCOLOR ascol)
{
  return ASCOLOR2PIXEL15 (ascol);
}
unsigned long
ascolor_to_pixel16 (ASCOLOR ascol)
{
  return ASCOLOR2PIXEL16 (ascol);
}
unsigned long
ascolor_to_pixel24 (ASCOLOR ascol)
{
  if (BGR_mode)
    return ASCOLOR2PIXEL24BGR (ascol);
  return ASCOLOR2PIXEL24RGB (ascol);
}

void
set_ascolor_depth (Window root, unsigned int depth)
{
  XWindowAttributes attr;
  XGetWindowAttributes (dpy, root, &attr);
  if (depth == 0)
    depth = attr.depth;
  else if (depth == 16 && attr.depth == 15)
    depth = 15;

  if (ascolor_visual != attr.visual)
    {
      ascolor_visual = attr.visual;
      BGR_mode = attr.visual->red_mask & 0x01;
      if (ascolor_visual->class == StaticColor || ascolor_visual->class == TrueColor)
	{
	  ascolor_rshift = get_shifts (ascolor_visual->red_mask);
	  ascolor_gshift = get_shifts (ascolor_visual->green_mask);
	  ascolor_bshift = get_shifts (ascolor_visual->blue_mask);
	  ascolor_rbits = get_bits (ascolor_visual->red_mask);
	  ascolor_gbits = get_bits (ascolor_visual->green_mask);
	  ascolor_bbits = get_bits (ascolor_visual->blue_mask);
	}

      if (ascolor_visual->class != TrueColor)
	{
	  Colormap new_map;
	  if (attr.map_installed == True)
	    new_map = attr.colormap;
	  else
	    new_map = DefaultColormap (dpy, DefaultScreen (dpy));

	  if (!table_initialized)
	    {
	      table_initialized = 1;
	      table_init (&ColorTable);
	    }
	  else if (new_map != ascolor_colormap)
	    {
	      table_idestroy (&ColorTable);
	      table_init (&ColorTable);
	    }
	  ascolor_colormap = new_map;
	  Bits = 16 - ascolor_visual->bits_per_rgb;
	}
      else if (depth < 15)
	{
	  ascolor_rshift = 15 - get_shifts (ascolor_visual->red_mask);
	  ascolor_gshift = 15 - get_shifts (ascolor_visual->green_mask);
	  ascolor_bshift = 15 - get_shifts (ascolor_visual->blue_mask);
	}
    }

  if (ascolor_visual->class == TrueColor)
    {
      switch (depth)
	{
	case 15:
	  ascolor_to_pixel_func = ascolor_to_pixel15;
	  break;
	case 16:
	  ascolor_to_pixel_func = ascolor_to_pixel16;
	  break;
	case 32:
	case 24:
	  ascolor_to_pixel_func = ascolor_to_pixel24;
	  break;
	default:
	  ascolor_to_pixel_func = ascolor_to_pixel_low;
	}
      ascolor_true_depth = depth;
    }
  else
    {
      ascolor_to_pixel_func = ascolor_to_pixel_default;
      ascolor_true_depth = 0;
    }

}

unsigned long
ascolor_to_pixel_init (ASCOLOR ascol)
{
  set_ascolor_depth (Scr.Root, Scr.d_depth);
  return ascolor_to_pixel_func (ascol);
}

unsigned long (*ascolor_to_pixel_func) (ASCOLOR ascol) = ascolor_to_pixel_init;

/*
 * Allocates a color or get's it from the color cache.
 * If the display is TrueColor, make the color by ourselves.
 */
int
MyAllocColor (XColor * color)
{
  color->pixel = ASCOLOR2PIXEL (MAKE_ASCOLOR_RGB16 (color->red, color->green, color->blue));
  return 1;
}

/**************************************************************************/
/* will scatter supplied pair of colors over supplied array making
   illusion of smooth gradient
 */
static RND32 stepgfx_rnd32_seed = GRADIENT_SEED;

ASCOLOR
MakeLight (ASCOLOR base)
{
  ASCOLOR average = ASCOLOR_AVG_COMP (base);
  ASCOLOR light = ((base >> 1) & ASCOLOR_MASK);
  int red, green, blue;

  red = ASCOLOR_RED8 (light) + average;
  green = ASCOLOR_GREEN8 (light) + average;
  blue = ASCOLOR_BLUE8 (light) + average;
  red = (red > ASCOLOR_MAXCOMP) ? ASCOLOR_MAXCOMP : red;
  green = (green > ASCOLOR_MAXCOMP) ? ASCOLOR_MAXCOMP : green;
  blue = (blue > ASCOLOR_MAXCOMP) ? ASCOLOR_MAXCOMP : blue;
  light = MAKE_ASCOLOR_RGB8 (red, green, blue);
  return light;
}

ASCOLOR
MakeDark (ASCOLOR base)
{
  /* this is 1/4 of base (can shift only one bit at a time) */
  ASCOLOR substr = ((((base >> 1) & ASCOLOR_MASK) >> 1) & ASCOLOR_MASK);
  /* getting 1/4 - 1/16 to approximate to original 20% substract defined by Kojima */
  substr -= ((((substr >> 1) & ASCOLOR_MASK) >> 1) & ASCOLOR_MASK);

  return (base - substr) & ASCOLOR_MASK;
}

void
draw_gradient_line_dither (ASCOLOR * line, int width, int npoints, XColor * color, double *offset)
{
  int i, offset_prev = offset[0] * width, x = MAX (0, offset_prev);
  for (i = 1; i < npoints; i++)
    {
      int offset_next = offset[i] * width;
      if (offset_next > offset_prev)
	{
	  int xf = MIN (offset_next, width);
	  int ratio_add = MAX_MY_RND32 / (offset_next - offset_prev);
	  int ratio = (x - offset_prev - 1) * ratio_add;
	  for (; x < xf; x++)
	    line[x] = MY_RND32 () > (ratio += ratio_add) ? color[i - 1].pixel : color[i].pixel;
	}
      offset_prev = offset_next;
    }
}

void
draw_gradient_line_true (ASCOLOR * line, int width, int npoints, XColor * color, double *offset)
{
  int i, x = 0, offset_prev = offset[0] * width;
  for (i = 1; i < npoints; i++)
    {
      int offset_next = offset[i] * width;
      if (offset_next > offset_prev)
	{
	  int xf = MIN (offset_next, width);
	  int red = ((int) color[i - 1].red) << 13;
	  int red_add = ((((int) color[i].red) << 13) - red) / (offset_next - offset_prev);
	  int green = ((int) color[i - 1].green) << 13;
	  int green_add = ((((int) color[i].green) << 13) - green) / (offset_next - offset_prev);
	  int blue = ((int) color[i - 1].blue) << 13;
	  int blue_add = ((((int) color[i].blue) << 13) - blue) / (offset_next - offset_prev);
	  red += (x - offset_prev - 1) * red_add;
	  green += (x - offset_prev - 1) * green_add;
	  blue += (x - offset_prev - 1) * blue_add;
	  for (; x < xf; x++)
	    {
	      line[x] = MAKE_ASCOLOR_RGB16 ((red += red_add) >> 13, (green += green_add) >> 13, (blue += blue_add) >> 13);
	    }
	}
      offset_prev = offset_next;
    }
}

/* returns the maximum number of true colors between a and b */
int
xcolor_manhattan_distance (XColor * a, XColor * b)
{
  return (ABS ((int) a->red - b->red) >> (16 - ascolor_rbits)) +
    (ABS ((int) a->green - b->green) >> (16 - ascolor_gbits)) +
    (ABS ((int) a->blue - b->blue) >> (16 - ascolor_bbits));
}

/* draw dithered gradient to XImage, then put XImage to drawable; this
 * algorithm cares about maxcolors and finesse */
void
draw_gradient_dither (Display * dpy, Drawable d, int tx, int ty, int tw, int th,
		      int npoints, XColor * color, double *offset, int type,
		      int maxcolors, int finesse, int depth, GC gc)
{
  int length = (type == TEXTURE_GRADIENT_L2R) ? tw : ((type == TEXTURE_GRADIENT_T2B) ? th : 2 * tw);
  int iw, ih;
  int i, free_color = 0;
  XImage *image;
  Pixmap pixmap;

  /* calculate image width and height */
  switch (type)
    {
    case TEXTURE_GRADIENT_TL2BR:	/* top-left to bottom-right */
    case TEXTURE_GRADIENT_BL2TR:	/* bottom-left to top-right */
      iw = tw * 2;
      ih = MIN (th, finesse);
      break;
    case TEXTURE_GRADIENT_L2R:	/* left to right */
      iw = tw;
      ih = MIN (th, finesse);
      break;
    default:
    case TEXTURE_GRADIENT_T2B:	/* top to bottom */
      iw = MIN (tw, finesse);
      ih = th;
      break;
    }

  /* create an XImage to draw on */
  image = CreateXImageAndData (dpy, DefaultVisual (dpy, Scr.screen), depth,
			       ZPixmap, 0, iw, ih);
  if (image)
    set_ascolor_depth (Scr.Root, image->bits_per_pixel);
  else
    return;

  /* split up gradient if we have enough colors */
  if (maxcolors > 0 && maxcolors < 255 && maxcolors > npoints)
    {
      XColor *color2 = NEW_ARRAY (XColor, maxcolors);
      double *offset2 = NEW_ARRAY (double, maxcolors);
      free_color = 1;
      for (i = 0; i < npoints; i++)
	{
	  color2[i] = color[i];
	  offset2[i] = offset[i];
	}
      color = color2;
      offset = offset2;
      /* find the largest span and split it */
      for (; npoints < maxcolors; npoints++)
	{
	  int max = 0, n = -1;
	  for (i = 1; i < npoints; i++)
	    {
	      int colors_used = xcolor_manhattan_distance (color + i - 1, color + i);
	      int dist = (offset[i] - offset[i - 1]) * length;
	      int val = colors_used;
	      if (max < val && (dist >= 8 || (dist < val && dist < npoints - maxcolors)))
		{
		  max = val;
		  n = i;
		}
	    }
	  /* don't bother splitting really small spans */
	  if (n == -1)
	    break;
	  /* calculate the intermediate color and offset */
	  memmove (color + n + 1, color + n, sizeof (XColor) * (npoints - n));
	  color[n].red = (color[n - 1].red + color[n + 1].red) / 2;
	  color[n].green = (color[n - 1].green + color[n + 1].green) / 2;
	  color[n].blue = (color[n - 1].blue + color[n + 1].blue) / 2;
	  color[n].pixel = MAKE_ASCOLOR_RGB16 (color[n].red, color[n].green, color[n].blue);
	  memmove (offset + n + 1, offset + n, sizeof (double) * (npoints - n));
	  offset[n] = (offset[n - 1] + offset[n + 1]) / 2;
	}
    }

  /* draw the gradient */
  switch (type)
    {
    case TEXTURE_GRADIENT_TL2BR:
      {				/* top-left to bottom-right */
	ASCOLOR *buffer = NEW_ARRAY (ASCOLOR, iw);
	DECLARE_XIMAGE_CURSOR (image, curr, 0);
	int y;
	double *offsetp = NEW_ARRAY (double, npoints);
	double ratio = 0.5 / th;
	for (i = 0; i < npoints; i++)
	  offsetp[i] = offset[i];
	for (y = 0; y < ih; y++)
	  {
	    int x = MAX (offsetp[0] * iw, 0);
	    int xf = MIN (offsetp[npoints - 1] * iw, iw);
	    draw_gradient_line_dither (buffer, iw, npoints, color, offsetp);
	    SET_XIMAGE_CURSOR (image, curr, x, y);
	    for (; x < xf; x++)
	      PUT_ASCOLOR (image, buffer[x], curr, x, y);
	    for (i = 0; i < npoints; i++)
	      offsetp[i] -= ratio;
	  }
	free (offsetp);
	free (buffer);
	break;
      }
    case TEXTURE_GRADIENT_BL2TR:
      {				/* bottom-left to top-right */
	ASCOLOR *buffer = NEW_ARRAY (ASCOLOR, iw);
	DECLARE_XIMAGE_CURSOR (image, curr, 0);
	int y;
	double *offsetp = NEW_ARRAY (double, npoints);
	double ratio = 0.5 / th;
	for (i = 0; i < npoints; i++)
	  offsetp[i] = offset[i];
	for (y = 0; y < ih; y++)
	  {
	    int x = MAX (offsetp[0] * iw, 0);
	    int xf = MIN (offsetp[npoints - 1] * iw, iw);
	    draw_gradient_line_dither (buffer, iw, npoints, color, offsetp);
	    SET_XIMAGE_CURSOR (image, curr, x, y);
	    for (; x < xf; x++)
	      PUT_ASCOLOR (image, buffer[x], curr, x, y);
	    for (i = 0; i < npoints; i++)
	      offsetp[i] += ratio;
	  }
	free (offsetp);
	free (buffer);
	break;
      }
    case TEXTURE_GRADIENT_L2R:
      {				/* left to right */
	ASCOLOR *buffer = NEW_ARRAY (ASCOLOR, iw);
	DECLARE_XIMAGE_CURSOR (image, curr, 0);
	int xi = MAX (offset[0] * iw, 0);
	int xf = MIN (offset[npoints - 1] * iw, iw);
	int x, y;
	for (y = 0; y < ih; y++)
	  {
	    draw_gradient_line_dither (buffer, iw, npoints, color, offset);
	    SET_XIMAGE_CURSOR (image, curr, xi, y);
	    for (x = xi; x < xf; x++)
	      PUT_ASCOLOR (image, buffer[x], curr, x, y);
	  }
	free (buffer);
	break;
      }
    case TEXTURE_GRADIENT_T2B:
      {				/* top to bottom */
	ASCOLOR *buffer = NEW_ARRAY (ASCOLOR, ih);
	DECLARE_XIMAGE_CURSOR (image, curr, 0);
	int yi = MAX (offset[0] * ih, 0);
	int yf = MIN (offset[npoints - 1] * ih, ih);
	int x, y, next_offset = image->bytes_per_line - BYTES_PER_PIXEL;
	for (x = 0; x < iw; x++)
	  {
	    draw_gradient_line_dither (buffer, ih, npoints, color, offset);
	    SET_XIMAGE_CURSOR (image, curr, x, yi);
	    for (y = yi; y < yf; y++)
	      {
		PUT_ASCOLOR (image, buffer[y], curr, x, y);
		INCREMENT_XIMAGE_CURSOR (curr, next_offset);
	      }
	  }
	free (buffer);
	break;
      }
    }

  /* create a Pixmap to allow server-side copies via XCopyArea() */
  pixmap = XCreatePixmap (dpy, d, iw, ih, depth);

  /* copy the XImage to the Pixmap */
  XPutImage (dpy, pixmap, gc, image, 0, 0, 0, 0, iw, ih);

  /* draw the Pixmap to the Drawable */
  switch (type)
    {
    case TEXTURE_GRADIENT_TL2BR:
      {				/* top-left to bottom-right */
	int y;
	double x = 0, x_add = (double) tw * ih / th;
	for (y = 0; y < th; y += ih)
	  {
	    XCopyArea (dpy, pixmap, d, gc, x, 0, tw, MIN (ih, th - y), tx, ty + y);
	    x += x_add;
	  }
	break;
      }
    case TEXTURE_GRADIENT_BL2TR:
      {				/* bottom-left to top-right */
	int y;
	double x = tw, x_add = (double) tw * ih / th;
	for (y = 0; y < th; y += ih)
	  {
	    XCopyArea (dpy, pixmap, d, gc, x, 0, tw, MIN (ih, th - y), tx, ty + y);
	    x -= x_add;
	  }
	break;
      }
    case TEXTURE_GRADIENT_L2R:
      {				/* left to right */
	int y;
	for (y = 0; y < th; y += ih)
	  XCopyArea (dpy, pixmap, d, gc, 0, 0, tw, MIN (ih, th - y), tx, ty + y);
	break;
      }
    case TEXTURE_GRADIENT_T2B:
      {				/* top to bottom */
	int x;
	for (x = 0; x < tw; x += iw)
	  XCopyArea (dpy, pixmap, d, gc, 0, 0, MIN (iw, tw - x), th, tx + x, ty);
	break;
      }
    }

  /* free up memory */
  XFreePixmap (dpy, pixmap);
  XDestroyImage (image);
  if (free_color)
    {
      free (color);
      free (offset);
    }
}

/* draw gradient using XDrawLine(); this algorithm assumes truecolor, and
 * ignores maxcolors and finesse */
void
draw_gradient_true (Display * dpy, Drawable d, int tx, int ty, int tw, int th,
		    int npoints, XColor * color, double *offset, int type,
		    int maxcolors, int finesse, int depth, GC gc)
{
  XRectangle clip_region;

  /* set a clip region to clip diagonal gradients */
  clip_region.x = tx;
  clip_region.y = ty;
  clip_region.width = tw;
  clip_region.height = th;
  XSetClipRectangles (dpy, gc, 0, 0, &clip_region, 1, YXSorted);

  /* draw the gradient */
  switch (type)
    {
    case TEXTURE_GRADIENT_TL2BR:
      {				/* top-left to bottom-right */
	int x = 2 * MAX (offset[0] * tw, 0);
	int xf = 2 * CLAMP (offset[npoints - 1] * tw, 0, tw);
	ASCOLOR *buffer = NEW_ARRAY (ASCOLOR, 2 * tw);
	draw_gradient_line_true (buffer, 2 * tw, npoints, color, offset);
	for (; x < xf; x++)
	  {
	    XSetForeground (dpy, gc, ASCOLOR2PIXEL (buffer[x]));
	    XDrawLine (dpy, d, gc, tx + x, ty, tx + x - tw, ty + th - 1);
	  }
	free (buffer);
	break;
      }
    case TEXTURE_GRADIENT_BL2TR:
      {				/* bottom-left to top-right */
	int x = 2 * CLAMP (offset[0] * tw, 0, tw);
	int xf = 2 * CLAMP (offset[npoints - 1] * tw, 0, tw);
	ASCOLOR *buffer = NEW_ARRAY (ASCOLOR, 2 * tw);
	draw_gradient_line_true (buffer, 2 * tw, npoints, color, offset);
	for (; x < xf; x++)
	  {
	    XSetForeground (dpy, gc, ASCOLOR2PIXEL (buffer[x]));
	    XDrawLine (dpy, d, gc, tx + x - tw, ty, tx + x, ty + th - 1);
	  }
	free (buffer);
	break;
      }
    case TEXTURE_GRADIENT_L2R:
      {				/* left to right */
	int x = CLAMP (offset[0] * tw, 0, tw);
	int xf = CLAMP (offset[npoints - 1] * tw, 0, tw);
	ASCOLOR *buffer = NEW_ARRAY (ASCOLOR, tw);
	draw_gradient_line_true (buffer, tw, npoints, color, offset);
	for (; x < xf; x++)
	  {
	    XSetForeground (dpy, gc, ASCOLOR2PIXEL (buffer[x]));
	    XDrawLine (dpy, d, gc, tx + x, ty, tx + x, ty + th - 1);
	  }
	free (buffer);
	break;
      }
    case TEXTURE_GRADIENT_T2B:
      {				/* top to bottom */
	int y = CLAMP (offset[0] * th, 0, th);
	int yf = CLAMP (offset[npoints - 1] * th, 0, th);
	ASCOLOR *buffer = NEW_ARRAY (ASCOLOR, th);
	draw_gradient_line_true (buffer, th, npoints, color, offset);
	for (; y < yf; y++)
	  {
	    XSetForeground (dpy, gc, ASCOLOR2PIXEL (buffer[y]));
	    XDrawLine (dpy, d, gc, tx, ty + y, tx + tw - 1, ty + y);
	  }
	free (buffer);
	break;
      }
    }
}

/*

 * relief :
 *  <0 - sunken
 *  =0 - flat
 *  >0 - rised
 * type:
 *   TEXTURE_GRADIENT_TL2BR => top-left to bottom-right
 *   TEXTURE_GRADIENT_BL2TR => bottom-left to top-right
 *   TEXTURE_GRADIENT_L2R   => left to right
 *   TEXTURE_GRADIENT_T2B   => top to bottom
 * when the gradient is dithered, only finesse lines will actually be
 * drawn, and all other lines will be copies of them
 */

int
draw_gradient (Display * dpy, Drawable d, int tx, int ty, int tw, int th,
	       int npoints, XColor * color, double *offset, int relief,
	       int type, int maxcolors, int finesse)
{
  GC gc;
  Window root;
  int i, junk, dither = 0;
  unsigned int width, height, bw, depth;
  int length = (type == TEXTURE_GRADIENT_L2R) ? tw : ((type == TEXTURE_GRADIENT_T2B) ? th : 2 * tw);

  /* check for invalid type, or less than 2 points */
  if (npoints < 2 || type < TEXTURE_GRADIENT_TL2BR || type > TEXTURE_GRADIENT_L2R)
    return -1;

  /* limit finesse to no longer than the gradient length */
  finesse = MIN (length, finesse);

  /* drawable may have different colordepth then default */
  XGetGeometry (dpy, d, &root, &junk, &junk, &width, &height, &bw, &depth);
  set_ascolor_depth (root, depth);

  /* create a temporary drawing GC */
  gc = XCreateGC (dpy, d, 0, NULL);

  /* convert original colors */
  for (i = 0; i < npoints; i++)
    color[i].pixel = MAKE_ASCOLOR_RGB16 (color[i].red, color[i].green, color[i].blue);

  /* draw the bevel */
  if (relief)
    {
      int r = ABS (relief);
      if (tw > 2 * r && th > 2 * r)
	{
	  ASCOLOR hilite, shadow;
	  if (maxcolors < npoints + 2)
	    {
	      hilite = color[0].pixel;
	      shadow = color[npoints - 1].pixel;
	    }
	  else
	    {
	      hilite = MakeLight (color[i].pixel);
	      shadow = MakeDark (color[npoints - 1].pixel);
	      maxcolors -= 2;
	    }
	  if (relief < 0)
	    SWAP (hilite, shadow, ASCOLOR);
	  XSetForeground (dpy, gc, ASCOLOR2PIXEL (hilite));
	  for (i = 0; i < r; i++)
	    {
	      XDrawLine (dpy, d, gc, tx + i, ty + i, tx + i, ty + th - i - 1);
	      XDrawLine (dpy, d, gc, tx + i, ty + i, tx + tw - i - 1, ty + i);
	    }
	  XSetForeground (dpy, gc, ASCOLOR2PIXEL (shadow));
	  for (i = 0; i < r; i++)
	    {
	      XDrawLine (dpy, d, gc, tx + tw - i - 1, ty + th - i - 1, tx + tw - i - 1, ty + i);
	      XDrawLine (dpy, d, gc, tx + tw - i - 1, ty + th - i - 1, tx + i, ty + th - i - 1);
	    }
	  tx += r;
	  ty += r;
	  tw -= 2 * r;
	  th -= 2 * r;
	}
    }

  /* choose whether or not to dither; dither if our visual uses dynamic
   * colors (ie, class is odd), or if bands of a single color which are
   * wider than some minimum (we use 3) are found */
  if ((ascolor_visual->class & 1) && ascolor_visual->class != DirectColor)
    {
      dither = 1;
    }
  else
    {
      int max = 3, n = -1;
      for (i = 1; i < npoints; i++)
	{
	  int colors_used = xcolor_manhattan_distance (color + i - 1, color + i);
	  if (colors_used > 0)
	    {
	      int val = (offset[i] - offset[i - 1]) * length / colors_used;
	      if (max < val)
		{
		  max = val;
		  n = i;
		}
	    }
	}
      if (n != -1)
	{
	  dither = 1;
	  maxcolors = 254;
	}
    }

  if (dither)
    draw_gradient_dither (dpy, d, tx, ty, tw, th, npoints, color, offset, type, maxcolors, finesse, depth, gc);
  else
    draw_gradient_true (dpy, d, tx, ty, tw, th, npoints, color, offset, type, maxcolors, finesse, depth, gc);

  /* free up memory */
  XFreeGC (dpy, gc);

  return 0;
}


/************************************************************************
 *
 * Draws text with a texture
 *
 * d - target drawable
 * font - font to draw text
 * x,y - position of text
 * gradient - texture pixmap. size must be at least as large as text
 * text - text to draw
 * chars - chars in text
 ************************************************************************/
#ifdef I18N
void
DrawTexturedText (Display * dpy, Drawable d, XFontStruct * font, XFontSet fontset, int x, int y, Pixmap gradient, char *text, int chars)
#else
void
DrawTexturedText (Display * dpy, Drawable d, XFontStruct * font, int x, int y, Pixmap gradient, char *text, int chars)
#endif
{
  Pixmap mask;
  int w, h;
  GC gc;
  XGCValues gcv;

  /* make the mask pixmap */
  w = XTextWidth (font, text, chars);
  h = font->ascent + font->descent;
  mask = XCreatePixmap (dpy, DefaultRootWindow (dpy), w + 1, h + 1, 1);
  gcv.foreground = 0;
  gcv.function = GXcopy;
  gcv.font = font->fid;
  gc = XCreateGC (dpy, mask, GCFunction | GCForeground | GCFont, &gcv);
  XFillRectangle (dpy, mask, gc, 0, 0, w, h);
  XSetForeground (dpy, gc, 1);
#undef FONTSET
#define FONTSET fontset
  XDrawString (dpy, mask, gc, 0, font->ascent, text, chars);
  XFreeGC (dpy, gc);
  /* draw the texture */
  gcv.function = GXcopy;
  gc = XCreateGC (dpy, d, GCFunction, &gcv);
  XSetClipOrigin (dpy, gc, x, y);
  XSetClipMask (dpy, gc, mask);
  XCopyArea (dpy, gradient, d, gc, 0, 0, w, h, x, y);
  XFreeGC (dpy, gc);
  XFreePixmap (dpy, mask);
}
