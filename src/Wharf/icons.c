/*
 * Copyright (C) 1998 Sasha Vasko <sashav@sprintmail.com>
 * Copyright (C) 1995 Bo Yang
 * Copyright (C) 1993 Robert Nation
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
 *
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

#ifdef NeXT
#include <fcntl.h>
#endif

#include "Wharf.h"
#include "../../include/loadimg.h"
#include "../../include/XImage_utils.h"
#include "../../include/pixmap.h"

/****************************************************************************
 *
 * Loads an icon file into a pixmap
 *
 ****************************************************************************/
void
LoadIconFile (icon_info * icon)
{
#ifndef NO_ICONS
  /* First, check for a monochrome bitmap */
  if ((*icon).file != NULL)
    GetBitmapFile (icon);
  /* Next, check for a color pixmap */
  if (((*icon).file != NULL) &&
      ((*icon).w == 0) && ((*icon).h == 0))
    GetImageFile (icon);
#endif
}

/****************************************************************************
 *
 * Creates an Icon Window
 *
 ****************************************************************************/
void
CreateButtonIconWindow (button_info * button, Window * win)
{
#ifndef NO_ICONS
  unsigned long valuemask;	/* mask for create windows */
  XSetWindowAttributes attributes;	/* attributes for create windows */

  /* This used to make buttons without icons explode when pushed
     if(((*button).icon_w == 0)&&((*button).icon_h == 0))
     return;
   */
  attributes.background_pixel = Style->colors.back;
  attributes.event_mask = ExposureMask;
  valuemask = CWEventMask | CWBackPixel;

  /* make sure the window geometry does not match the button, so 
   * place_buttons() is forced to configure it */
  (*button).IconWin =
    XCreateWindow (dpy, *win, 0, 0, 1, 1, 0, CopyFromParent,
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
ConfigureIconWindow (button_info * button)
{
#ifndef NO_ICONS
  int i;
  GC ShapeGC;
  int w, h;
  int xoff, yoff;
  icon_info icon = back_pixmap;
  int bMyIcon = 0;

  if (button->width <= 0 || button->height <= 0)
    return;

  /* handle transparency */
  if (Style->texture_type >= TEXTURE_TRANSPARENT && Style->texture_type < TEXTURE_BUILTIN)
    {
      int x = 0, y = 0;
      Pixmap pixmap;

      if (button->parent != NULL)
	{
	  x = button->parent->x;
	  y = button->parent->y;
	}
      pixmap = mystyle_make_pixmap_overlay (Style, x + button->x, y + button->y, button->width, button->height, None);
      if (pixmap != None)
	{
	  icon.icon = pixmap;
	  icon.w = button->width;
	  icon.h = button->height;
	  bMyIcon = 1;
	}
    }

  if (button->completeIcon.icon != None)
    XFreePixmap (dpy, button->completeIcon.icon);
  button->completeIcon.icon = XCreatePixmap (dpy, Scr.Root, button->width,
					     button->height, Scr.d_depth);
  XSetForeground (dpy, NormalGC, BlackPixel (dpy, Scr.screen));
  XFillRectangle (dpy, button->completeIcon.icon, NormalGC,
		  0, 0, button->width, button->height);

  /* if the background does not fill the whole button, shape */
  clear_flags (button->flags, WB_Shaped);
  if (icon.icon == None || icon.mask != None)
    set_flags (button->flags, WB_Shaped);

  /* if an icon fills the whole button, don't shape */
  for (i = 0; i < button->num_icons; i++)
    if ((button->icons[i].mask == None) &&
	(button->icons[i].w >= button->width) &&
	(button->icons[i].h >= button->height))
      clear_flags (button->flags, WB_Shaped);

  /* create and clear the mask */
  if (button->completeIcon.mask != None)
    XFreePixmap (dpy, button->completeIcon.mask);
  button->completeIcon.mask = XCreatePixmap (dpy, Scr.Root, button->width,
					     button->height, 1);
  ShapeGC = XCreateGC (dpy, button->completeIcon.mask, 0, NULL);
  if (get_flags (button->flags, WB_Shaped))
    XSetForeground (dpy, ShapeGC, 0);
  else
    XSetForeground (dpy, ShapeGC, 1);
  XFillRectangle (dpy, button->completeIcon.mask, ShapeGC,
		  0, 0, button->width, button->height);

  /* tile with the background pixmap */
  if (icon.icon != None)
    {
      XSetFillStyle (dpy, NormalGC, FillTiled);
      XSetTile (dpy, NormalGC, icon.icon);
      XFillRectangle (dpy, button->completeIcon.icon, NormalGC,
		      0, 0, button->width, button->height);
      XSetFillStyle (dpy, NormalGC, FillSolid);

      if (icon.mask != None)
	{
	  XSetFillStyle (dpy, ShapeGC, FillTiled);
	  XSetTile (dpy, ShapeGC, icon.mask);
	  XFillRectangle (dpy, button->completeIcon.mask, ShapeGC,
			  0, 0, button->width, button->height);
	  XSetFillStyle (dpy, ShapeGC, FillSolid);
	}
    }

  /* overlay the icons */
  XSetFunction (dpy, ShapeGC, GXor);
  for (i = 0; i < button->num_icons; i++)
    {
      w = button->icons[i].w;
      h = button->icons[i].h;
      if (w < 1 || h < 1)
	continue;
      if (w > button->width)
	w = button->width;
      if (h > button->height)
	h = button->height;
      if (w < 1)
	w = 1;
      if (h < 1)
	h = 1;
      xoff = (button->width - w) / 2;
      yoff = (button->height - h) / 2;
      if (xoff < 0)
	xoff = 0;
      if (yoff < 0)
	yoff = 0;
      if (button->icons[i].mask != None)
	{
	  XSetClipOrigin (dpy, NormalGC, xoff, yoff);
	  XSetClipMask (dpy, NormalGC, button->icons[i].mask);
	}
      else
	{
	  XRectangle rect[1];
	  rect[0].x = 0;
	  rect[0].y = 0;
	  rect[0].width = w;
	  rect[0].height = h;

	  XSetClipRectangles (dpy, NormalGC, xoff, yoff, rect, 1, YSorted);
	}
      XCopyArea (dpy, button->icons[i].icon,
		 button->completeIcon.icon, NormalGC, 0, 0,
		 w, h, xoff, yoff);
      if (button->icons[i].mask != None)
	{
	  XCopyArea (dpy, button->icons[i].mask,
		     button->completeIcon.mask, ShapeGC, 0, 0,
		     w, h, xoff, yoff);
	}
      else
	{
	  XSetForeground (dpy, ShapeGC, 1);
	  XFillRectangle (dpy, button->completeIcon.mask, ShapeGC,
			  xoff, yoff, w, h);
	}

      if (button->icons[i].depth == -1)
	{
	  XCopyPlane (dpy, button->icons[i].icon,
		      button->completeIcon.icon, NormalGC,
		      0, 0, w, h, xoff, yoff, 1);
	  UnloadImage (button->icons[i].icon);
	  button->icons[i].icon = None;
	}
    }
  XFreeGC (dpy, ShapeGC);
  XSetClipMask (dpy, NormalGC, None);
  XSetWindowBackgroundPixmap (dpy, button->IconWin,
			      button->completeIcon.icon);
  XClearWindow (dpy, button->IconWin);

  if (bMyIcon && icon.icon != None)
    UnloadImage (icon.icon);
#endif
}

/***************************************************************************
 *
 * Looks for a monochrome icon bitmap file
 *
 **************************************************************************/
void
GetBitmapFile (icon_info * icon)
{
#ifndef NO_ICONS
  char *path = NULL;
  int HotX, HotY;

  path = findIconFile ((*icon).file, iconPath, R_OK);
  if (path == NULL)
    return;

  if (XReadBitmapFile (dpy, Scr.Root, path, (unsigned int *) &(*icon).w,
		       (unsigned int *) &(*icon).h,
		       &(*icon).icon,
		       (int *) &HotX,
		       (int *) &HotY) != BitmapSuccess)
    {
      (*icon).w = 0;
      (*icon).h = 0;
    }
  else
    {
      (*icon).depth = -1;
    }
  (*icon).mask = None;
  free (path);
#endif
}


/****************************************************************************
 *
 * Looks for a color icon file
 *
 ****************************************************************************/
int
GetImageFile (icon_info * icon)
{
#ifndef NO_ICONS
  char *path = NULL;
  unsigned int dum;
  int dummy;
  int width, height;
  Window root;

  if ((path = findIconFile ((*icon).file, pixmapPath, R_OK)) == NULL)
    return 0;

  icon->icon = LoadImageWithMask (dpy, Scr.Root, 256, path, &(icon->mask));
  free (path);

  if (icon->icon == 0)
    return 0;

  XGetGeometry (dpy, icon->icon, &root, &dummy, &dummy,
		&width, &height, &dum, &dum);
  icon->w = width;
  icon->h = height;
  icon->depth = Scr.d_depth;
  return 1;
#endif
}

/****************************************************************************
 *
 * read background icons from data
 *
 ****************************************************************************/
int
GetXPMData (icon_info * icon, char **data)
{
#ifndef NO_ICONS
#ifdef XPM
  XWindowAttributes root_attr;
  XpmAttributes xpm_attributes;

  if (icon != &back_pixmap)
    return 0;
  XGetWindowAttributes (dpy, Scr.Root, &root_attr);
  xpm_attributes.colormap = root_attr.colormap;
  xpm_attributes.valuemask = XpmSize | XpmReturnPixels | XpmColormap;
  if (XpmCreatePixmapFromData (dpy, Scr.Root, data,
			       &(*icon).icon,
			       &(*icon).mask,
			       &xpm_attributes) == XpmSuccess)
    {
      (*icon).w = xpm_attributes.width;
      (*icon).h = xpm_attributes.height;
      (*icon).depth = Scr.d_depth;
    }
  else
    {
      return 0;
    }
  DrawOutline ((*icon).icon, (*icon).w, (*icon).h);

  return 1;
#else /* !XPM */
  (*icon).icon = None;
  return 0;
#endif /* !XPM */
#endif
}
