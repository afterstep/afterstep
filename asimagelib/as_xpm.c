/* This file contains code for unified image loading from XPM file  */
/********************************************************************/
/* Copyright (c) 1998 Sasha Vasko <sashav@sprintmail.com>            */
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#define LIBASIMAGE

/*#define DEBUG_AS_XPM */
/*#define GETPIXEL_PUTPIXEL     */
/*#define DO_CLOCKING     */

#include "../configure.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#ifdef XPM
#ifdef HAVE_X11_XPM
#include <X11/xpm.h>
#else
#include <xpm.h>
#endif
#endif

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/style.h"
#include "../include/screen.h"
#include "../include/ascolor.h"

/* our input-output data structures definitions */
#include "../include/loadimg.h"
#include "../include/XImage_utils.h"

#ifdef DEBUG_AS_XPM
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

int
ShowPixmapError (int Err)
{
#ifdef XPM
  if (Err == XpmSuccess)
    return 0;
  fprintf (stderr, "\nlibXpm error: ");
  if (Err == XpmOpenFailed)
    fprintf (stderr, "error reading XPM file: %s\n", XpmGetErrorString (Err));
  else if (Err == XpmColorFailed)
    fprintf (stderr, "Couldn't allocate required colors\n");
  /* else if (Err == XpmFileInvalid)
     fprintf (stderr, "Invalid Xpm File\n"); */
  else if (Err == XpmColorError)
    fprintf (stderr, "Invalid Color specified in Xpm file\n");
  else if (Err == XpmNoMemory)
    fprintf (stderr, "Insufficient Memory\n");
#endif /* XPM */

  return 1;

}

/* loads pixmap from the XPM file .
   Returns :
   0 - if failed to load pixmap ;
   1 - if pixmap loaded successfully.
 */
int
LoadXPMFile (LImageParams * pParams)
{
#ifdef XPM
  XpmImage xpmImage;
#ifdef DO_CLOCKING
  clock_t started = clock ();
#endif

  LOG1 ("\nLoadXPMFile(): reading XpmImage ...");
  if (XpmReadFileToXpmImage ((char *) (pParams->m_realfilename), &xpmImage, NULL) != XpmSuccess)
    return 0;
  LOG1 ("done.\n               checking size... ");
  CheckImageSize (pParams, xpmImage.width, xpmImage.height);
  LOG1 ("done.\n               checking visual type /bpp... ");
  {
    XpmAttributes xpm_attributes;
    XImage *image, *mask;
    int Err = 0;

    xpm_attributes.colormap = pParams->m_colormap;
    xpm_attributes.width = pParams->m_max_x;
    xpm_attributes.height = pParams->m_max_y;
    xpm_attributes.closeness = 40000;
    xpm_attributes.valuemask = XpmSize | XpmReturnPixels | XpmColormap | XpmCloseness;
    /* read XpmImage into XImage */

    if ((Err = XpmCreateImageFromXpmImage (pParams->m_dpy, &xpmImage,
			     &image, &mask, &xpm_attributes)) == XpmSuccess)
      {
	XpmFreeAttributes (&xpm_attributes);

	if (pParams->m_x_net || pParams->m_y_net)
	  {			/* scale XImage */
	    XImage *scaled_image;
	    if ((scaled_image = ScaleXImageToSize (image, pParams->m_width, pParams->m_height)))
	      {
		XImage *scaled_mask = ScaleXImageToSize (mask, pParams->m_width, pParams->m_height);
		if (scaled_mask)
		  {
		    XDestroyImage (image);
		    image = scaled_image;
		    XDestroyImage (mask);
		    mask = scaled_mask;
		  }
		else
		  XDestroyImage (scaled_image);
	      }
	  }

	/* put XImage on TargetPixmap */
	pParams->m_pImage = image;
	XImageToPixmap (pParams, image, &(pParams->m_Target));
	pParams->m_Mask = None;
	pParams->m_pMaskImage = mask;
	XImageToPixmap (pParams, mask, &(pParams->m_Mask));
      }
    else
      ShowPixmapError (Err);
  }
  XpmFreeXpmImage (&xpmImage);
  XSync (pParams->m_dpy, 0);
#ifdef DO_CLOCKING
  printf ("\n image loading time (clocks): %lu\n", clock () - started);
#endif

  LOG2 ("\nLoading XPM at:%s. ", ((pParams->m_Target == 0) ? "Failed" : "Success"));
#endif /* XPM */

  return (pParams->m_Target != 0) ? 1 : 0;
}
