/*--------------------------------*-C-*---------------------------------*
 * File:	pixmap.c
 *----------------------------------------------------------------------*
 * Copyright (c) 1999 Ethan Fischer <allanon@crystaltokyo.com>
 * Copyright (c) 1999 Sasha Vasko   <sasha at aftercode.net>
 *
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
 *---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*
 * Originally written:
 *    1999	Sasha Vasko <sasha at aftercode.net>
 *----------------------------------------------------------------------*/

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "../configure.h"
#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/style.h"
#include "../include/screen.h"
#include "../include/XImage_utils.h"

#define NO_PIXMAP_PROTOS
#include "../include/pixmap.h"

/*#define DO_CLOCKING       */
/*#define DEBUG_IMAGING     */

#ifdef DO_CLOCKING
#include <time.h>
#endif


/* Xdisplay, Xroot needs to be defined if code is moved to some other prog */
#define Xdisplay dpy
#ifndef Xscreen
#define Xscreen  DefaultScreen(dpy)
#endif
#define Xroot    RootWindow(dpy, Xscreen)

#define CREATE_TRG_PIXMAP(w,h) XCreatePixmap(Xdisplay, Xroot, w, h, DefaultDepth(dpy, Xscreen))

void
CopyAndShadeArea (Drawable src, Pixmap trg,
		  int x, int y, int w, int h,
		  int trg_x, int trg_y,
		  GC gc, ShadingInfo * shading)
{
  if (shading)
    {
      XImage *img;
      if (x < 0 || y < 0)
	return;
      if ((img = XGetImage (Xdisplay, src, x, y, w, h, AllPlanes, ZPixmap)) != NULL)
	{
	  ShadeXImage (img, shading, gc);
	  XPutImage (Xdisplay, trg, gc, img, 0, 0, trg_x, trg_y, w, h);
	  XDestroyImage (img);
	  return;
	}
    }
  XCopyArea (Xdisplay, src, trg, gc, x, y, w, h, trg_x, trg_y);
}

void
ShadeTiledPixmap (Pixmap src, Pixmap trg, int src_w, int src_h, int x, int y, int w, int h, GC gc, ShadingInfo * shading)
{
  int tile_x, tile_y, left_w, bott_h;

  tile_x = x % src_w;
  tile_y = y % src_h;
  left_w = min (src_w - tile_x, w);
  bott_h = min (src_h - tile_y, h);
/*fprintf( stderr, "\nShadeTiledPixmap(): tile_x = %d, tile_y = %d, left_w = %d, bott_h = %d, SRC = %dx%d TRG=%dx%d", tile_x, tile_y, left_w, bott_h, src_w, src_h, w, h); */
  CopyAndShadeArea (src, trg, tile_x, tile_y, left_w, bott_h, 0, 0, gc, shading);
  if (bott_h < h)
    {				/* right-top parts */
      CopyAndShadeArea (src, trg, tile_x, 0, left_w, h - bott_h, 0, bott_h, gc, shading);
    }
  if (left_w < w)
    {				/* left-bott parts */
      CopyAndShadeArea (src, trg, 0, tile_y, w - left_w, bott_h, left_w, 0, gc, shading);
      if (bott_h < h)		/* left-top parts */
	CopyAndShadeArea (src, trg, 0, 0, w - left_w, h - bott_h, left_w, bott_h, gc, shading);
    }
}

/****************************************************************************
 *
 * fill part of a pixmap with the root pixmap, offset properly to look
 * "transparent"
 *
 ***************************************************************************/
int
FillPixmapWithTile (Pixmap pixmap, Pixmap tile, int x, int y, int width, int height, int tile_x, int tile_y)
{
  if (tile != None && pixmap != None)
    {
      GC gc;
      XGCValues gcv;

      gcv.tile = tile;
      gcv.fill_style = FillTiled;
      gcv.ts_x_origin = -tile_x;
      gcv.ts_y_origin = -tile_y;
      gc = XCreateGC (dpy, tile, GCFillStyle | GCTile | GCTileStipXOrigin | GCTileStipYOrigin, &gcv);
      XFillRectangle (dpy, pixmap, gc, x, y, width, height);
      XFreeGC (dpy, gc);
      return 1;
    }
  return 0;
}


Pixmap
ShadePixmap (Pixmap src, int x, int y, int width, int height, GC gc, ShadingInfo * shading)
{
  Pixmap trg = CREATE_TRG_PIXMAP (width, height);
  if (trg != None)
    {
      CopyAndShadeArea (src, trg, x, y, width, height, 0, 0, gc, shading);
    }
  return trg;
}
int MyXDestroyImage (XImage * ximage);

Pixmap
ScalePixmap (Pixmap src, int src_w, int src_h, int width, int height, GC gc, ShadingInfo * shading)
{
  XImage *srcImage, *trgImage;
  Pixmap trg = None;

  if (src != None)
    {
      if ((srcImage = XGetImage (Xdisplay, src, 0, 0, src_w, src_h, AllPlanes, ZPixmap)) != NULL)
	{
	  if ((trgImage = ScaleXImageToSize (srcImage, (Dimension) width, (Dimension) height)) != NULL)
	    {
	      if ((trg = CREATE_TRG_PIXMAP (width, height)) != 0)
		{
		  if (shading)
		    ShadeXImage (trgImage, shading, gc);
		  XPutImage (Xdisplay, trg, gc, trgImage, 0, 0, 0, 0, width, height);
		}
	      if (trgImage)
		XDestroyImage (trgImage);
	    }
	  XDestroyImage (srcImage);
	}
    }
  return trg;
}

Pixmap
CenterPixmap (Pixmap src, int src_w, int src_h, int width, int height, GC gc, ShadingInfo * shading)
{
  int x, y, w, h, src_x = 0, src_y = 0;
  Pixmap trg;
  /* create target pixmap of the size of the window */
  trg = CREATE_TRG_PIXMAP (width, height);
  if (trg != None)
    {
      /* fill it with background color */
      XFillRectangle (Xdisplay, trg, gc, 0, 0, width, height);
      /* place image at the center of it */
      x = (width - src_w) >> 1;
      y = (height - src_h) >> 1;
      if (x < 0)
	{
	  src_x -= x;
	  w = min (width, src_w + x);
	  x = 0;
	}
      else
	w = min (width, src_w);
      if (y < 0)
	{
	  src_y -= y;
	  h = min (height, src_h + y);
	  y = 0;
	}
      else
	h = min (height, src_h);

      CopyAndShadeArea (src, trg, src_x, src_y, w, h, x, y, gc, shading);
    }

  return trg;
}

Pixmap
GrowPixmap (Pixmap src, int src_w, int src_h, int width, int height, GC gc, ShadingInfo * shading)
{
  int w, h;
  Pixmap trg;
  /* create target pixmap of the size of the window */
  trg = CREATE_TRG_PIXMAP (width, height);
  if (trg != None)
    {
      /* fill it with background color */
      XFillRectangle (Xdisplay, trg, gc, 0, 0, width, height);
      /* place image at the center of it */
      w = min (width, src_w);
      h = min (height, src_h);

      CopyAndShadeArea (src, trg, 0, 0, w, h, 0, 0, gc, shading);
    }

  return trg;
}
/* PROTO */
int
GetRootDimensions (int *width, int *height)
{
  Window root;
  int w_x, w_y;
  unsigned int junk;
  if (!XGetGeometry (Xdisplay, Xroot, &root,
		     &w_x, &w_y, width, height, &junk, &junk))
    {
      *width = 0;
      *height = 0;
    }
  return (*width > 0 && *height > 0) ? 1 : 0;
}

int
GetWinPosition (Window w, int *x, int *y)
{
  Window root, parent, *children;
  unsigned int nchildren;
  static int rootWidth = 0, rootHeight = 0;
  XWindowAttributes attr;
  int my_x, my_y;

  XGetWindowAttributes (Xdisplay, w, &attr);
  if (attr.map_state != IsViewable)
    return 0;

  if (!x)
    x = &my_x;
  if (!y)
    y = &my_y;

  *x = 0;
  *y = 0;

  if (!rootWidth || !rootHeight)
    if (!GetRootDimensions (&rootWidth, &rootHeight))
      return 0;

  while (XQueryTree (Xdisplay, w, &root, &parent, &children, &nchildren))
    {
      int w_x, w_y;
      unsigned int w_w, w_h, border_w, w_depth;
      if (children)
	XFree (children);
      if (!XGetGeometry (Xdisplay, w, &root,
			 &w_x, &w_y, &w_w, &w_h, &border_w, &w_depth))
	break;
      (*x) += w_x + border_w;
      (*y) += w_y + border_w;

      if (parent == root)
	{			/* taking in to consideration virtual desktopping */
	  int bRes = 1;
	  if (*x < 0 || *x >= rootWidth || *y < 0 || *y >= rootHeight)
	    bRes = 0;
	  /* don't want to return position outside the screen even if we fail */
	  while (*x < 0)
	    *x += rootWidth;
	  while (*y < 0)
	    *y += rootHeight;
	  if (*x > rootWidth)
	    *x %= rootWidth;
	  if (*y > rootHeight)
	    *y %= rootHeight;
	  return bRes;
	}
      w = parent;
    }
  *x = 0;
  *y = 0;
  return 0;
}

static Pixmap
CutPixmap ( Pixmap src, Pixmap trg,
            int x, int y,
	    unsigned int src_w, unsigned int src_h,
	    unsigned int width, unsigned int height,
	    GC gc, ShadingInfo * shading)
{
  Bool my_pixmap = (trg == None )?True:False ;
  int screen_w, screen_h ;
  int w = width, h = height;
  int offset_x = 0, offset_y = 0;

  if (width < 2 || height < 2 )
    return trg;

  screen_w = DisplayWidth( Xdisplay, Scr.screen );
  screen_h = DisplayHeight( Xdisplay, Scr.screen );

  while( x+(int)width < 0 )  x+= screen_w ;
  while( x >= screen_w )  x-= screen_w ;
  while( y+(int)height < 0 )  y+= screen_h ;
  while( y >= screen_h )  y-= screen_h ;

  if( x < 0 )
  {
    offset_x = (-x);
    w -= offset_x ;
    x = 0 ;
  }
  if( y < 0 )
  {
    offset_y = (-y) ;
    h -= offset_y;
    y = 0 ;
  }
  if( x+w >= screen_w )
    w = screen_w - x ;

  if( y+height >= screen_h )
    h = screen_h - y ;

  if (src == None) /* we don't have root pixmap ID */
    { /* we want to create Overrideredirect window overlapping out window
         with background type of Parent Relative and then grab it */
     XSetWindowAttributes attr ;
     XEvent event ;
     int tick_count = 0 ;
     Bool grabbed = False ;
        attr.background_pixmap = ParentRelative ;
	attr.backing_store = Always ;
	attr.event_mask = ExposureMask ;
	attr.override_redirect = True ;
	src = XCreateWindow(Xdisplay, Xroot, x, y, w, h,
	                    0,
			    CopyFromParent, CopyFromParent, CopyFromParent,
			    CWBackPixmap|CWBackingStore|CWOverrideRedirect|CWEventMask,
			    &attr);

	if( src == None ) return trg ;
	XGrabServer( Xdisplay );
	grabbed = True ;
	XMapRaised( Xdisplay, src );
	XSync(Xdisplay, False );
	start_ticker(1);
	/* now we have to wait for our window to become mapped - waiting for Expose */
	for( tick_count = 0 ; !XCheckWindowEvent( Xdisplay, src, ExposureMask, &event ) && tick_count < 100 ; tick_count++)
	    wait_tick();
	if( tick_count < 100 )
	{
	    if( trg == None )    trg = CREATE_TRG_PIXMAP (width, height);
	    if (trg != None)
	    {	/* custom code to cut area, so to ungrab server ASAP */
	        if (shading)
	        {
	          XImage *img;
		  img = XGetImage (Xdisplay, src, 0, 0, w, h, AllPlanes, ZPixmap);
		  XDestroyWindow( Xdisplay, src );
		  src = None ;
		  XUngrabServer( Xdisplay );
		  grabbed = False ;
		  if (img != NULL)
		  {
    		    ShadeXImage (img, shading, gc);
		    XPutImage (Xdisplay, trg, gc, img, 0, 0, offset_x, offset_y, w, h);
		    XDestroyImage (img);
		  }else if( my_pixmap )
		  {
		    XFreePixmap( Xdisplay, trg );
		    trg = None ;
		  }
		}else
		    XCopyArea (Xdisplay, src, trg, gc, 0, 0, w, h, offset_x, offset_y);
	    }
        }
	if( src )
	    XDestroyWindow( Xdisplay, src );
	if( grabbed )
	    XUngrabServer( Xdisplay );
	return trg ;
    }
  /* we have root pixmap ID */
  /* find out our coordinates relative to the root window */
  if (x + w > src_w || y + h > src_h)
    {			/* tiled pixmap processing here */
      Pixmap tmp ;
      w = min (w, src_w);
      h = min (h, src_h);

      tmp = CREATE_TRG_PIXMAP (w, h);
      if (tmp != None)
      {
        ShadeTiledPixmap (src, tmp, src_w, src_h, x, y, w, h, gc, shading);
        if( trg == None )
	{
           if( (trg = CREATE_TRG_PIXMAP (w+offset_x, h+offset_y)) != None )
		XCopyArea (Xdisplay, tmp, trg, gc, 0, 0, w, h, offset_x, offset_y);
	}else
	   FillPixmapWithTile( trg, tmp, offset_x, offset_y, width, height, 0, 0 );

	XFreePixmap( Xdisplay, tmp );
        return trg;
      }
    }

  /* create target pixmap of the size of the window */
  if( trg == None )    trg = CREATE_TRG_PIXMAP (width, height);
  if (trg != None)
    {
      /* cut area */
      CopyAndShadeArea (src, trg, x, y, w, h, offset_x, offset_y, gc, shading);

    }

  return trg;
}

Pixmap
CutWinPixmap (Window win, Drawable src, int src_w, int src_h, int width,
	      int height, GC gc, ShadingInfo * shading)
{
  unsigned int x = 0, y = 0;

  if (!GetWinPosition (win, &x, &y))
	return None;

  return CutPixmap( src, None, x, y, src_w, src_h, width, height, gc, shading );
}

#if 0
Pixmap
CutWinPixmap (Window win, Drawable src, int src_w, int src_h, int width, int height, GC gc, ShadingInfo * shading)
{
  unsigned int x = 0, y = 0, w, h;
  Pixmap trg;
  int bNeedToRemap = 0;

  if (src == None)
    {
      Window root, parent, *children;
      unsigned int border, depth, nchildren;

      if (!XGetGeometry (Xdisplay, win, &root, &x, &y, &w, &h, &border, &depth))
	return None;
      if (!GetWinPosition (win, &x, &y))
	return None;
      w -= border * 2 + 1;
      h -= border * 2 + 1;
      src_w = w + x;
      src_h = h + y;
      /* of course we can do withou it, but to make code more generic */
      while (XQueryTree (Xdisplay, win, &src, &parent, &children, &nchildren))
	{
	  if (children)
	    XFree (children);
	  if (parent == src)
	    break;
	  win = parent;
	}

      if (src == None || win == None)
	return None;		/* it aint gonna happen, coz we always
				   have root window but just in case */
      XUnmapWindow (Xdisplay, win);
      bNeedToRemap = 1;

    }
  else
    {
      /* find out our coordinates relative to the root window */
      if (!GetWinPosition (win, &x, &y))
	return None;
      w = width;
      h = height;

      if (x + w > src_w || y + h > src_h)
	{			/* tiled pixmap processing here */

	  if ((trg = CREATE_TRG_PIXMAP (min (w, src_w), min (h, src_h))))
	    ShadeTiledPixmap (src, trg, src_w, src_h, x, y, min (w, src_w), min (h, src_h), gc, shading);
	  return trg;
	}
    }

  if (x < 0 || y < 0 || w < 2 || h < 2)
    return None;

  /* create target pixmap of the size of the window */
  trg = CREATE_TRG_PIXMAP (width, height);
  if (trg != None)
    {
      /* cut area */
      CopyAndShadeArea (src, trg, x, y, w, h, 0, 0, gc, shading);

    }
  if (bNeedToRemap)
    XMapWindow (Xdisplay, win);

  return trg;
}
#endif

/* PROTO */
Pixmap
GetRootPixmap (Atom id)
{
  Pixmap currentRootPixmap = None;
  if (id == None)
    id = XInternAtom (Xdisplay, "_XROOTPMAP_ID", True);

  if (id != None)
    {
      Atom act_type;
      int act_format;
      unsigned long nitems, bytes_after;
      unsigned char *prop = NULL;

/*fprintf(stderr, "\n aterm GetRootPixmap(): root pixmap is set");                  */
      if (XGetWindowProperty (Xdisplay, Xroot, id, 0, 1, False, XA_PIXMAP,
			      &act_type, &act_format, &nitems, &bytes_after,
			      &prop) == Success)
	{
	  if (prop)
	    {
	      currentRootPixmap = *((Pixmap *) prop);
	      XFree (prop);
/*fprintf(stderr, "\n aterm GetRootPixmap(): root pixmap is [%lu]", currentRootPixmap); */
	    }
	}
    }
  return currentRootPixmap;
}

static int
pixmap_error_handler (Display * dpy, XErrorEvent * error)
{
#ifdef DEBUG_IMAGING
  fprintf (stderr, "\n aterm caused XError # %u, in resource %lu, Request: %d.%d",
	   error->error_code, error->resourceid, error->request_code, error->minor_code);
#endif
  return 0;
}


Pixmap
ValidatePixmap (Pixmap p, int bSetHandler, int bTransparent, unsigned int *pWidth, unsigned int *pHeight)
{
  int (*oldXErrorHandler) (Display *, XErrorEvent *) = NULL;
  /* we need to check if pixmap is still valid */
  Window root;
  int junk;
  if (bSetHandler)
    oldXErrorHandler = XSetErrorHandler (pixmap_error_handler);

  if (bTransparent)
    p = GetRootPixmap (None);
  if (!pWidth)
    pWidth = &junk;
  if (!pHeight)
    pHeight = &junk;

  if (p != None)
    {
      if (!XGetGeometry (Xdisplay, p, &root, &junk, &junk, pWidth, pHeight, &junk, &junk))
	p = None;
    }
  if (bSetHandler)
    XSetErrorHandler (oldXErrorHandler);

  return p;
}

/****************************************************************************
 *
 * grab a section of the screen and darken it
 *
 ***************************************************************************/
int
fill_with_darkened_background (Pixmap * pixmap, XColor color, int x, int y, int width, int height, int root_x, int root_y, int bDiscardOriginal)
{
  unsigned int root_w, root_h;
  Pixmap root_pixmap;
  ShadingInfo shading;

  /* added by Sasha on 02/24/1999 to use transparency&shading provided by
     libasimage 1.1 */
  root_pixmap = ValidatePixmap (None, 1, 1, &root_w, &root_h);

  if (root_pixmap != None)
    {

      INIT_SHADING2 (shading, color)
	if (*pixmap == None)
	{
	  *pixmap = XCreatePixmap (dpy, RootWindow (dpy, Xscreen), width, height, DefaultDepth (dpy, Xscreen));
	  bDiscardOriginal = 1;
	}
      FillPixmapWithTile (*pixmap, root_pixmap, x, y, width, height, root_x, root_y);
      if (!NO_NEED_TO_SHADE (shading))
	{
	  Pixmap shaded_pixmap = ShadePixmap (*pixmap, x, y, width, height, DefaultGC (dpy, Xscreen), &shading);
	  if (shaded_pixmap)
	    {
	      if (bDiscardOriginal)
		XFreePixmap (dpy, *pixmap);
	      *pixmap = shaded_pixmap;
	    }
	}

      return 1;
    }
  return 0;
}

/****************************************************************************
 *
 * grab a section of the screen and combine it with an XImage
 *
 ***************************************************************************/
int
fill_with_pixmapped_background (Pixmap * pixmap, XImage * image, int x, int y, int width, int height, int root_x, int root_y, int bDiscardOriginal)
{
  unsigned int root_w, root_h;
  Pixmap root_pixmap;

  root_pixmap = ValidatePixmap (None, 1, 1, &root_w, &root_h);
  if (root_pixmap != None)
    {
      if (*pixmap == None)
	*pixmap = XCreatePixmap (dpy, RootWindow (dpy, Xscreen), width, height, DefaultDepth (dpy, Xscreen));
      FillPixmapWithTile (*pixmap, root_pixmap, x, y, width, height, root_x, root_y);
      CombinePixmapWithXImage (*pixmap, image, x, y, width, height);
      return 1;
    }
  return 0;
}
