/*
 * Copyright (c) 1999 Sasha Vasko <sashav@sprintmail.com>
 * Copyright (c) 1998 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (c) 1993 Robert Nation
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
 */

#include "../../configure.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#ifdef XPM
#include <X11/xpm.h>
#endif /* XPM */

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

#ifdef NeXT
#include <fcntl.h>
#endif

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/loadimg.h"
#include "Zharf.h"

/****************************************************************************
 *
 * Loads an icon file into a pixmap
 *
 ****************************************************************************/
void
LoadIconFile (int button)
{
#ifndef NO_ICONS
  /* First, check for a monochrome bitmap */
  if (Buttons[button].icon_file != NULL)
    GetBitmapFile (button);

  /* Next, check for a color pixmap */
  if ((Buttons[button].icon_file != NULL) &&
      (Buttons[button].icon_w == 0) && (Buttons[button].icon_h == 0))
    GetImageFile (button);
#endif
}

/****************************************************************************
 *
 * Creates an Icon Window
 *
 ****************************************************************************/
void
CreateIconWindow (int button)
{
#ifndef NO_ICONS
  unsigned long valuemask;	/* mask for create windows */
  XSetWindowAttributes attributes;	/* attributes for create windows */

  if ((Buttons[button].icon_w == 0) && (Buttons[button].icon_h == 0))
    return;

  attributes.background_pixel = back_pix;
  attributes.event_mask = ExposureMask | EnterWindowMask | LeaveWindowMask;
  valuemask = CWEventMask | CWBackPixel;

  Buttons[button].IconWin =
    XCreateWindow (dpy, main_win, 0, 0, Buttons[button].icon_w,
		   Buttons[button].icon_w, 0, CopyFromParent,
		   CopyFromParent, CopyFromParent, valuemask, &attributes);
  return;
#endif
}

/****************************************************************************
 *
 * Combines icon shape masks after a resize
 *
 ****************************************************************************/
void
ConfigureIconWindow (int button, int row, int column)
{
#ifndef NO_ICONS
  int x, y, w, h;
  int xoff, yoff;
  Pixmap temp;

  if ((Buttons[button].icon_w == 0) && (Buttons[button].icon_h == 0))
    return;

  if (Buttons[button].swallow != 0)
    return;

  w = Buttons[button].icon_w;
  h = Buttons[button].icon_h;
  if (w > Buttons[button].BWidth * ButtonWidth - 8)
    w = Buttons[button].BWidth * ButtonWidth - 8;
  if (strcmp (Buttons[button].title, "-") == 0)
    {
      if (h > Buttons[button].BHeight * ButtonHeight - 8)
	h = Buttons[button].BHeight * ButtonHeight - 8;
    }
  else
    {
      if (h > Buttons[button].BHeight * ButtonHeight - 8 - font.height)
	h = Buttons[button].BHeight * ButtonHeight - 8 - font.height;
    }


  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;

  x = column * ButtonWidth;
  y = row * ButtonHeight;
  xoff = (Buttons[button].BWidth * ButtonWidth - w) >> 1;
  yoff = (Buttons[button].BHeight * ButtonHeight - (h + font.height)) >> 1;

  if (xoff < 2)
    xoff = 2;
  if (yoff < 2)
    yoff = 2;
  x += xoff;
  y += yoff;

  XMoveResizeWindow (dpy, Buttons[button].IconWin, x, y, w, h);

#ifdef XPM
#ifdef SHAPE
  if (Buttons[button].icon_maskPixmap != None)
    {
      xoff = (w - Buttons[button].icon_w) >> 1;
      yoff = (h - Buttons[button].icon_h) >> 1;
      XShapeCombineMask (dpy, Buttons[button].IconWin, ShapeBounding, 0, 0,
			 Buttons[button].icon_maskPixmap, ShapeSet);
    }
#endif
#endif
  if (Buttons[button].icon_depth == -1)
    {
      temp = Buttons[button].iconPixmap;
      Buttons[button].iconPixmap =
	XCreatePixmap (dpy, Scr.Root, Buttons[button].icon_w,
		       Buttons[button].icon_h, Scr.d_depth);
      XCopyPlane (dpy, temp, Buttons[button].iconPixmap, NormalGC,
		  0, 0, Buttons[button].icon_w, Buttons[button].icon_h,
		  0, 0, 1);
    }
  XSetWindowBackgroundPixmap (dpy, Buttons[button].IconWin, Buttons[button].iconPixmap);

  XClearWindow (dpy, Buttons[button].IconWin);
#endif
}

/***************************************************************************
 *
 * Looks for a monochrome icon bitmap file
 *
 **************************************************************************/
void
GetBitmapFile (int button)
{
#ifndef NO_ICONS
  char *path = NULL;
  int HotX, HotY;

  path = findIconFile (Buttons[button].icon_file, iconPath, R_OK);
  if (path == NULL)
    return;

  if (XReadBitmapFile (dpy, Scr.Root, path, (unsigned int *) &Buttons[button].icon_w,
		       (unsigned int *) &Buttons[button].icon_h,
		       &Buttons[button].iconPixmap,
		       (int *) &HotX,
		       (int *) &HotY) != BitmapSuccess)
    {
      Buttons[button].icon_w = 0;
      Buttons[button].icon_h = 0;
    }
  else
    {
      Buttons[button].icon_depth = -1;
    }
  Buttons[button].icon_maskPixmap = None;
  free (path);
#endif
}


/****************************************************************************
 *
 * Looks for a color XPM icon file
 *
 ****************************************************************************/
void
GetImageFile (int button)
{
#ifndef NO_ICONS
  unsigned int dum;
  int dummy;
  Window root;
  char *path = findIconFile (Buttons[button].icon_file, pixmapPath, R_OK);

  if (path == NULL)
    return;

  Buttons[button].iconPixmap = LoadImageWithMask (dpy, Scr.Root, 256, path, &(Buttons[button].icon_maskPixmap));
  free (path);

  if (Buttons[button].iconPixmap == 0)
    return;

  XGetGeometry (dpy, Buttons[button].iconPixmap, &root, &dummy, &dummy,
		(unsigned int *) &(Buttons[button].icon_w),
		(unsigned int *) &(Buttons[button].icon_h), &dum, &dum);
  Buttons[button].icon_depth = Scr.d_depth;

#endif
}
