/* This file contains code for unified image loading from file

 *  Copyright (c) 1998 Sasha Vasko <sasha at aftercode.net>
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

#define LIBASIMAGE
/*#define DEBUG_LOADIMAGE */


#include "../configure.h"

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/xpm.h>

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"

/* our input-output data structures definitions */

#include "../include/ascolor.h"
#include "../include/loadimg.h"
#include "../include/XImage_utils.h"

#ifdef DEBUG_LOADIMAGE
#define LOG1(a)       fprintf( stderr, a );
#define LOG2(a,b)    fprintf( stderr, a, b );
#define LOG3(a,b,c)    fprintf( stderr, a, b, c );
#define LOG4(a,b,c,d)    fprintf( stderr, a, b, c, d );
#else
#define LOG1(a)
#define LOG2(a,b)
#define LOG3(a,b,c)
#define LOG4(a,b,c,d)
#endif

/* Here we'll detect image type */
int bReportErrorIfTypeUnknown = 1;
/* we might have a compressed XPM file */
int
CheckIfCompressed (const char *realfilename)
{
  int len = strlen (realfilename);

  if (len > 3 && strcasecmp (realfilename + len - 3, ".gz") == 0)
    return 1;
  if (len > 2 && strcasecmp (realfilename + len - 2, ".z") == 0)
    return 1;
  return 0;
}

#define FORMAT_TYPE_MAX_SIZE 128
#define FORMAT_TYPE_MIN_SIZE 10

/* return 0 if not detected      */
int
GetImageType (const char *realfilename)
{
  int fd;
  unsigned char head[FORMAT_TYPE_MAX_SIZE];
  int bytes_in = FORMAT_TYPE_MAX_SIZE;

  if ((fd = open (realfilename, O_RDONLY)) == -1)
    return -1;
  bytes_in = read (fd, head, bytes_in);
  close (fd);

  if (bytes_in > FORMAT_TYPE_MIN_SIZE)
    {
      if (bytes_in < FORMAT_TYPE_MAX_SIZE)
	head[bytes_in] = '\0';
      else
	head[bytes_in - 1] = '\0';

      if (head[0] == 0xff && head[1] == 0xd8 && head[2] == 0xff)
	return F_JPEG;
      else if (head[0] == 'G' && head[1] == 'I' && head[2] == 'F')
	return F_GIF;
      else if (head[0] == head[1] && (head[0] == 'I' || head[0] == 'M'))
	return F_TIFF;
      else if (head[0] == 0xa && head[1] <= 5 && head[2] == 1)
	return F_PCX;
      else if (head[0] == 'B' && head[1] == 'M')
	return F_BMP;
      else if (head[0] == 0 && head[1] == 0 &&
	       head[2] == 2 && head[3] == 0 &&
	       head[4] == 0 && head[5] == 0 &&
	       head[6] == 0 && head[7] == 0)
	return F_TARGA;
      else if (strncmp ((char *) head + 1, "PNG", (size_t) 3) == 0)
	return F_PNG;
      else if (strncmp ((char *) head, "#define", (size_t) 7) == 0)
	return F_XBM;
      else if (strstr ((char *) head, "XPM") != NULL)
	return F_XPM;
      else if (CheckIfCompressed (realfilename) != 0)
	return F_XPM;
      /* nothing yet for PCD */

    }
  return F_UNKNOWN;
}



int
CreateTarget (LImageParams * pParams)
{
  pParams->m_Target = XCreatePixmap (pParams->m_dpy, pParams->m_w,
				     pParams->m_width, pParams->m_height,
				     pParams->m_depth);
  XSync (pParams->m_dpy, False);
  LOG2 ("\n loadimage.c: Target Pixmap created: %lu .", pParams->m_Target);
  if (pParams->m_Target)
    if ((pParams->m_pImage = CreateXImageAndData (pParams->m_dpy,
						  pParams->m_visual,
						  pParams->m_depth,
						  ZPixmap, 0,
						  pParams->m_width,
						pParams->m_height)) == NULL)
      {
	XFreePixmap (pParams->m_dpy, pParams->m_Target);
	pParams->m_Target = 0;
      }
  LOG2 ("\n loadimage.c: Target XImage %s.", (pParams->m_pImage) ? "created" : "failed");
  return ((pParams->m_pImage == NULL) ? 0 : 1);
}

int
CreateMask (LImageParams * pParams)
{

  pParams->m_Mask = XCreatePixmap (pParams->m_dpy, pParams->m_w,
				   pParams->m_width, pParams->m_height,
				   MASK_DEPTH);
  if (pParams->m_Mask)
    if ((pParams->m_pMaskImage = XGetImage (pParams->m_dpy, pParams->m_Mask, 0, 0,
					pParams->m_width, pParams->m_height,
					    AllPlanes, ZPixmap)) == NULL)
      {
	XFreePixmap (pParams->m_dpy, pParams->m_Mask);
	pParams->m_Mask = 0;
      }
  LOG1 ("\nLoadImage: CreateMask done.");
  return ((pParams->m_pMaskImage == NULL) ? 0 : 1);
}

void
XImageToPixmap (LImageParams * pParams, XImage * pImage, Pixmap * pTarget)
{
  if (pImage == NULL)
    {
      if (*pTarget)
	{
	  LOG1 ("\nXImageToPixmap: XImageToPixmap pImage is NULL.");
	  XFreePixmap (pParams->m_dpy, *pTarget);
	  *pTarget = None;
	}
      return;
    }
  LOG1 ("\nXImageToPixmap: XImageToPixmap pImage is not NULL.");
  LOG4 ("\nXImageToPixmap: XImage depth=%u, format=%s, width=%d", pImage->depth, (pImage->format == ZPixmap) ? "ZPixmap" : ((pImage->format == XYBitmap) ? "XYBitmap" : "XYPixmap"), pImage->width)
    LOG3 (", height=%d, bpp=%u ", pImage->height, pImage->bits_per_pixel)
    if (*pTarget == None)
    if ((*pTarget = XCreatePixmap (pParams->m_dpy, pParams->m_w,
				   pParams->m_width, pParams->m_height,
				   pImage->depth)))
      {
	if (pParams->m_gc)
	  XFreeGC (pParams->m_dpy, pParams->m_gc);
	pParams->m_gc = XCreateGC (pParams->m_dpy, *pTarget, 0, NULL);
      }

  if (*pTarget)
    {
      GC mgc = XCreateGC (pParams->m_dpy, *pTarget, 0, NULL);
      XPutImage (pParams->m_dpy, *pTarget, mgc,
		 pImage, 0, 0, 0, 0, pParams->m_width, pParams->m_height);
      XFreeGC (pParams->m_dpy, mgc);
    }
}

void
CheckImageSize (LImageParams * pParams, unsigned int real_width, unsigned int real_height)
{
  if (pParams->m_width > 0 && pParams->m_x_net)
    {
      LOG3 ("\nCheckImageSize(): to_width = %d, from_width=%d", pParams->m_width, real_width)
	if (pParams->m_width == real_width)
	{
	  free (pParams->m_x_net);
	  pParams->m_x_net = NULL;
	}
      else
	Scale (pParams->m_x_net, pParams->m_width, 0, real_width - 1);

    }
  else if (pParams->m_max_x > 0 && pParams->m_max_x < real_width)
    pParams->m_width = pParams->m_max_x;
  else
    pParams->m_width = real_width;
  LOG2 ("\nCheckImageSize(): width = %d", pParams->m_width)
    if (pParams->m_height > 0 && pParams->m_y_net)
    {
      LOG3 ("\nCheckImageSize(): to_height = %d, from_height=%d", pParams->m_height, real_height)
	if (pParams->m_height == real_height)
	{
	  free (pParams->m_y_net);
	  pParams->m_y_net = NULL;
	}
      else
	Scale (pParams->m_y_net, pParams->m_height, 0, real_height - 1);
    }
  else if (pParams->m_max_y > 0 && pParams->m_max_y < real_height)
    pParams->m_height = pParams->m_max_y;
  else
    pParams->m_height = real_height;
  LOG2 ("\nCheckImageSize(): height = %d", pParams->m_height)
}

/* main image loading code here */

extern int LoadJPEGFile (LImageParams * pParams);
extern int LoadXPMFile (LImageParams * pParams);
extern void LoadPNGFile (LImageParams * pParams);

pixmap_ref_t *pixmap_ref_first = NULL;
int use_pixmap_ref = 1;

int
set_use_pixmap_ref (int on)
{
  int old = use_pixmap_ref;
  use_pixmap_ref = on;
  return old;
}

pixmap_ref_t *
pixmap_ref_new (const char *filename, Pixmap pixmap, Pixmap mask)
{
  pixmap_ref_t *ref = safemalloc (sizeof (pixmap_ref_t));
  ref->next = pixmap_ref_first;
  pixmap_ref_first = ref;
  ref->refcount = 1;
  ref->name = (filename == NULL) ? NULL : mystrdup (filename);
  ref->pixmap = pixmap;
  ref->mask = mask;
  return ref;
}

void
pixmap_ref_delete (pixmap_ref_t * ref)
{
  if (ref == NULL)
    return;

  if (pixmap_ref_first == ref)
    pixmap_ref_first = (*ref).next;
  else if (pixmap_ref_first != NULL)
    {
      pixmap_ref_t *ptr;
      for (ptr = pixmap_ref_first; (*ptr).next != NULL; ptr = (*ptr).next)
	if ((*ptr).next == ref)
	  break;
      if ((*ptr).next == ref)
	(*ptr).next = (*ref).next;
    }

  if (ref->name != NULL)
    free (ref->name);
  if (ref->pixmap != None)
    XFreePixmap (dpy, ref->pixmap);
  if (ref->mask != None)
    XFreePixmap (dpy, ref->mask);

  free (ref);
}

pixmap_ref_t *
pixmap_ref_find_by_name (const char *filename)
{
  pixmap_ref_t *ref;
  for (ref = pixmap_ref_first; ref != NULL; ref = ref->next)
    if (!strcmp (ref->name, filename))
      break;
  return ref;
}

pixmap_ref_t *
pixmap_ref_find_by_pixmap (Pixmap pixmap)
{
  pixmap_ref_t *ref;
  for (ref = pixmap_ref_first; ref != NULL; ref = ref->next)
    if (ref->pixmap == pixmap)
      break;
  return ref;
}

pixmap_ref_t *
pixmap_ref_find_by_mask (Pixmap mask)
{
  pixmap_ref_t *ref;
  for (ref = pixmap_ref_first; ref != NULL; ref = ref->next)
    if (ref->mask == mask)
      break;
  return ref;
}

int
pixmap_ref_increment (pixmap_ref_t * ref)
{
  return ++ref->refcount;
}

int
pixmap_ref_decrement (pixmap_ref_t * ref)
{
  int c = --ref->refcount;
/* don't delete the reference, the pixmap might be immediately reloaded;
   ** let the app tell us when to purge the pixmap via pixmap_ref_purge() */
/*
   if (c <= 0)
   pixmap_ref_delete(ref);
 */
  return c;
}

int
pixmap_ref_purge (void)
{
  int done = 0, count;
  for (count = -1; !done; count++)
    {
      pixmap_ref_t *ref;
      for (ref = pixmap_ref_first; ref != NULL; ref = ref->next)
	if (ref->refcount <= 0)
	  {
	    pixmap_ref_delete (ref);
	    break;
	  }
      done = (ref == NULL);
    }
  return count;
}

/* UnloadImage()
 * if pixmap is found in reference list, decrements reference count; else
 * XFreePixmap()'s the pixmap
 * returns number of remaining references to pixmap */
#if defined(LOG_LOADIMG_CALLS) && defined(DEBUG_ALLOCS)
int
l_UnloadImage (const char *file, int line, Pixmap pixmap)
{
  log_call (file, line, "UnloadImage", NULL);
#else
int
UnloadImage (Pixmap pixmap)
{
#endif
  if (use_pixmap_ref)
    {
      pixmap_ref_t *ref = pixmap_ref_find_by_pixmap (pixmap);
      if (ref != NULL)
	return pixmap_ref_decrement (ref);
    }
  XFreePixmap (dpy, pixmap);

  return 0;
}

/* UnloadMask()
 * if mask is found in the reference list and there is no corresponding
 * pixmap, decrements reference count; else if mask is found, does
 * nothing; else XFreePixmap()'s the mask
 * returns number of remaining references to mask */
#if defined(LOG_LOADIMG_CALLS) && defined(DEBUG_ALLOCS)
int
l_UnloadMask (const char *file, int line, Pixmap mask)
{
  log_call (file, line, "UnloadImage", NULL);
#else
int
UnloadMask (Pixmap mask)
{
#endif
  if (mask == None)
    return 0;

  if (use_pixmap_ref)
    {
      pixmap_ref_t *ref = pixmap_ref_find_by_mask (mask);
      if (ref == NULL)
	XFreePixmap (dpy, mask);
      else if (ref->pixmap == None)
	return pixmap_ref_decrement (ref);
      return (ref != NULL) ? ref->refcount : 0;
    }
  XFreePixmap (dpy, mask);
  return 0;
}

/* this one will load image from file */
Pixmap
LoadImageEx (LImageParams * pParams)
{

  pixmap_ref_t *ref = NULL;
  if (use_pixmap_ref)
    {
      ref = pixmap_ref_find_by_name (pParams->m_realfilename);
      if (ref != NULL)
	{
	  pixmap_ref_increment (ref);
	  pParams->m_Target = ref->pixmap;
	  pParams->m_Mask = ref->mask;
	  return ref->pixmap;
	}
    }

  pParams->m_Target = None;
  pParams->m_Mask = None;
  pParams->m_pImage = NULL;
  pParams->m_pMaskImage = NULL;
  pParams->m_gc = None;
  pParams->m_img_colormap = NULL;
  {				/* adjusting gamma correction */
    char *gamma_str;
    if ((gamma_str = getenv ("SCREEN_GAMMA")) != NULL)
      pParams->m_gamma = atof (gamma_str);
    else
      pParams->m_gamma = SCREEN_GAMMA;
    if (pParams->m_gamma == 0.0)
      pParams->m_gamma = SCREEN_GAMMA;
    if (pParams->m_gamma != 1.0)
      {
	register int i;
	double gamma_i = 1.0 / pParams->m_gamma;
	pParams->m_gamma_table = safemalloc (256);
	for (i = 0; i < 256; i++)
	  pParams->m_gamma_table[i] = ADJUST_GAMMA8_INV ((CARD8) i, gamma_i);
      }
    else
      pParams->m_gamma_table = NULL;
  }

  {				/* queriing window attributes */
    XWindowAttributes win_attr;
    XGetWindowAttributes (pParams->m_dpy, pParams->m_w, &win_attr);
    LOG3 ("\nLoadImageWithMask: window size %dx%d", win_attr.width, win_attr.height);
    if (pParams->m_max_x < 0)
      pParams->m_max_x = win_attr.width;
    if (pParams->m_max_y < 0)
      pParams->m_max_y = win_attr.height;
    pParams->m_colormap = win_attr.colormap;
    pParams->m_visual = win_attr.visual;
    pParams->m_depth = win_attr.depth;
    if (pParams->m_width <= 0)
      pParams->m_x_net = NULL;
    if (pParams->m_height <= 0)
      pParams->m_y_net = NULL;
  }

  LOG1 ("\nLoadImage: Checking file format...");
  switch (GetImageType (pParams->m_realfilename))
    {
    case -1:
      fprintf (stderr, "\n LoadImage: cannot read file[%s].", pParams->m_realfilename);
      break;
    case F_XPM:
#ifdef XPM
      LOG1 ("\nLoadImage: XPM format");
      LoadXPMFile (pParams);
#else
      if (bReportErrorIfTypeUnknown)
	{
	  fprintf (stderr, "\n LoadImage: cannot load XPM file [%s]", pParams->m_realfilename);
	  fprintf (stderr, "\n LoadImage: you need to install libXPM v 4.0 or higher, in order to read images of this format.");
	}
#endif /* XPM */
      break;

    case F_JPEG:
#ifdef JPEG
      LoadJPEGFile (pParams);
      LOG1 ("\nLoadImage: JPEG format");
#else
      if (bReportErrorIfTypeUnknown)
	{
	  fprintf (stderr, "\n LoadImage: cannot load JPEG file [%s]", pParams->m_realfilename);
	  fprintf (stderr, "\n LoadImage: you need to install libJPEG v 6.0 or higher, in order to read images of this format.");
	}
#endif /* JPEG */
      break;

    case F_PNG:
#ifdef PNG
      LoadPNGFile (pParams);
      LOG1 ("\nLoadImage: PNG format");
#else
      if (bReportErrorIfTypeUnknown)
	{
	  fprintf (stderr, "\n LoadImage: cannot load PNG file [%s]", pParams->m_realfilename);
	  fprintf (stderr, "\n LoadImage: you need to install libPNG, in order to read images of this format.");
	}
#endif /* PNG */
      break;

    default:
      if (bReportErrorIfTypeUnknown)
	fprintf (stderr, "\n LoadImage: Unknown or unsupported image format.\n");
      break;
    }

  /* here deallocating resources if needed */
  if (pParams->m_pImage)
    {
      LOG1 ("\nLoadImage: Destroying m_pImage..");
      XDestroyImage (pParams->m_pImage);
      pParams->m_pImage = NULL;
      LOG1 (" Done.");
    }

  if (pParams->m_pMaskImage)
    {
      LOG1 ("\nLoadImage: Destroying m_pMaskImage..");
      XDestroyImage (pParams->m_pMaskImage);
      pParams->m_pMaskImage = NULL;
      LOG1 (" Done.");
    }

  if (pParams->m_gc != 0)
    {
      LOG1 ("\nLoadImage: Freeing GC...");
      XFreeGC (pParams->m_dpy, pParams->m_gc);
      pParams->m_gc = 0;
      LOG1 (" Done.");
    }

  if (pParams->m_gamma_table)
    {
      free (pParams->m_gamma_table);
      pParams->m_gamma_table = NULL;
    }

  if (pParams->m_img_colormap)
    {
      free (pParams->m_img_colormap);
      pParams->m_img_colormap = NULL;
    }

  if (use_pixmap_ref && pParams->m_Target)
    {
      int using_tmp_head = set_use_tmp_heap (0);
      pixmap_ref_new (pParams->m_realfilename, pParams->m_Target, pParams->m_Mask);
      set_use_tmp_heap (using_tmp_head);
    }
  return pParams->m_Target;
}

void
SetMask (LImageParams * pParams, Pixmap * pMask)
{
  if (pMask)
    {
      if (pParams->m_Target != None)
	*pMask = pParams->m_Mask;
      else
	*pMask = None;
    }
  else if (pParams->m_Target != None && pParams->m_Mask)
    {
      XFreePixmap (pParams->m_dpy, pParams->m_Mask);
      pParams->m_Mask = None;
    }

}

Pixmap
LoadImageWithMask (Display * dpy, Window w, unsigned long max_colors, const char *realfilename, Pixmap * pMask)
{
  LImageParams Params;

  if (dpy == NULL || w == 0 || realfilename == NULL)
    return 0;
  if (strlen (realfilename) <= 0 || max_colors == 0)
    return 0;

  Params.m_dpy = dpy;
  Params.m_w = w;
  Params.m_max_colors = max_colors;
  Params.m_realfilename = realfilename;

  Params.m_width = 0;
  Params.m_height = 0;

  Params.m_max_x = 0;
  Params.m_max_y = 0;

  LOG2 ("\nLoadImageWithMask: filename: [%s].", realfilename);
  LoadImageEx (&Params);
  LOG1 ("\nLoadImageWithMask: image loaded, saving mask...");
  SetMask (&Params, pMask);
  LOG1 (" Done.");

  return Params.m_Target;
}

Pixmap
LoadImageWithMaskAndScale (Display * dpy, Window w, unsigned long max_colors, const char *realfilename, unsigned int to_width, unsigned int to_height, Pixmap * pMask)
{
  LImageParams Params;

  if (dpy == NULL || w == 0 || realfilename == NULL)
    return 0;
  if (strlen (realfilename) <= 0 || max_colors == 0)
    return 0;

  Params.m_dpy = dpy;
  Params.m_w = w;
  Params.m_max_colors = max_colors;
  Params.m_realfilename = realfilename;
  Params.m_width = to_width;

  Params.m_max_x = -1;
  Params.m_max_y = -1;

  if (to_width > 0)
    Params.m_x_net = (int *) safemalloc (to_width * sizeof (int));
  Params.m_height = to_height;
  if (to_height > 0)
    Params.m_y_net = (int *) safemalloc (to_height * sizeof (int));

  LOG2 ("\nLoadImageWithMask: filename: [%s].", realfilename);
  LoadImageEx (&Params);
  LOG1 ("\nLoadImageWithMask: image loaded, saving mask...");
  SetMask (&Params, pMask);

  if (Params.m_x_net)
    free (Params.m_x_net);
  if (Params.m_y_net)
    free (Params.m_y_net);

  LOG1 (" Done.");

  return Params.m_Target;
}

/*Pixmap LoadImage( Display *dpy, Window w, unsigned long max_colors, char* realfilename ){} */
