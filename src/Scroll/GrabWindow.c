/*
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
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

#define TRUE 1
#define FALSE 0
#define MAX_ICON_NAME_LEN 255

#include "../../configure.h"

#ifdef ISC
#include <sys/bsdtypes.h>	/* Saul */
#endif

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "../../include/module.h"
#include "Scroll.h"
char *MyName;

Display *dpy;			/* which display are we talking to */
int x_fd, fd_width;
int Width = 300, Height = 300;
int target_width, target_height;
int target_x_offset = 0, target_y_offset = 0;
int exposed;
int Reduction_H = 2;
int Reduction_V = 2;

#define BAR_WIDTH 21
#define SCROLL_BAR_WIDTH 9

#define PAD_WIDTH2 3
#define PAD_WIDTH3 5

Window Root;
int screen;
int d_depth;

Window main_win, holder_win;
Pixel back_pix, fore_pix, hilite_pix, shadow_pix;
GC ReliefGC, ShadowGC;
extern char *ForeColor, *BackColor;

#define MW_EVENTS   (ExposureMask | StructureNotifyMask| ButtonReleaseMask |\
		     ButtonPressMask | ButtonMotionMask | FocusChangeMask)

Atom wm_del_win;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_COLORMAP_WINDOWS;

/****************************************************************************
 *
 *  Draws the relief pattern around a window
 *
 ****************************************************************************/
void
RelieveWindow (Window win, int x, int y, int w, int h,
	       GC rgc, GC sgc)
{
  XSegment seg[4];
  int i;

  i = 0;
  seg[i].x1 = x;
  seg[i].y1 = y;
  seg[i].x2 = w + x - 1;
  seg[i++].y2 = y;

  seg[i].x1 = x;
  seg[i].y1 = y;
  seg[i].x2 = x;
  seg[i++].y2 = h + y - 1;

  seg[i].x1 = x + 1;
  seg[i].y1 = y + 1;
  seg[i].x2 = x + w - 2;
  seg[i++].y2 = y + 1;

  seg[i].x1 = x + 1;
  seg[i].y1 = y + 1;
  seg[i].x2 = x + 1;
  seg[i++].y2 = y + h - 2;
  XDrawSegments (dpy, win, rgc, seg, i);

  i = 0;
  seg[i].x1 = x;
  seg[i].y1 = y + h - 1;
  seg[i].x2 = w + x - 1;
  seg[i++].y2 = y + h - 1;

  seg[i].x1 = x + w - 1;
  seg[i].y1 = y;
  seg[i].x2 = x + w - 1;
  seg[i++].y2 = y + h - 1;
  if (d_depth < 2)
    XDrawSegments (dpy, win, ShadowGC, seg, i);
  else
    XDrawSegments (dpy, win, sgc, seg, i);

  i = 0;
  seg[i].x1 = x + 1;
  seg[i].y1 = y + h - 2;
  seg[i].x2 = x + w - 2;
  seg[i++].y2 = y + h - 2;

  seg[i].x1 = x + w - 2;
  seg[i].y1 = y + 1;
  seg[i].x2 = x + w - 2;
  seg[i++].y2 = y + h - 2;

  XDrawSegments (dpy, win, sgc, seg, i);
}

/************************************************************************
 *
 * Sizes and creates the window 
 *
 ***********************************************************************/
XSizeHints mysizehints;
void
CreateWindow (int x, int y, int w, int h)
{
  XGCValues gcv;
  unsigned long gcm;


  wm_del_win = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
  _XA_WM_PROTOCOLS = XInternAtom (dpy, "WM_PROTOCOLS", False);

  mysizehints.flags = PWinGravity | PResizeInc | PBaseSize |
    PMaxSize | PMinSize | USSize | USPosition;
  /* subtract one for the right/bottom border */
  mysizehints.width_inc = 1;
  mysizehints.height_inc = 1;
  mysizehints.base_height = BAR_WIDTH + PAD_WIDTH3;
  mysizehints.base_width = BAR_WIDTH + PAD_WIDTH3;

  Width = w / Reduction_H + BAR_WIDTH + PAD_WIDTH3;
  Height = h / Reduction_V + BAR_WIDTH + PAD_WIDTH3;
  target_width = w;
  target_height = h;
  mysizehints.width = Width;
  mysizehints.height = Height;
  mysizehints.x = x;
  mysizehints.y = y;
  mysizehints.max_width = w + BAR_WIDTH + PAD_WIDTH3;
  mysizehints.max_height = h + BAR_WIDTH + PAD_WIDTH3;

  mysizehints.win_gravity = NorthWestGravity;

  if (d_depth < 2)
    {
      back_pix = GetColor ("black");
      fore_pix = GetColor ("white");
      hilite_pix = back_pix;
      shadow_pix = fore_pix;
    }
  else
    {
      back_pix = GetColor (BackColor);
      fore_pix = GetColor (ForeColor);
      hilite_pix = GetHilite (fore_pix);
      shadow_pix = GetShadow (fore_pix);

    }

  main_win = XCreateSimpleWindow (dpy, Root, mysizehints.x, mysizehints.y,
				  mysizehints.width, mysizehints.height,
				  0, fore_pix, back_pix);
  XSetWMProtocols (dpy, main_win, &wm_del_win, 1);

  XSetWMNormalHints (dpy, main_win, &mysizehints);
  XSelectInput (dpy, main_win, MW_EVENTS);
  change_window_name (MyName);

  holder_win = XCreateSimpleWindow (dpy, main_win, PAD_WIDTH3, PAD_WIDTH3,
				 mysizehints.width - BAR_WIDTH - PAD_WIDTH3,
				mysizehints.height - BAR_WIDTH - PAD_WIDTH3,
				    0, fore_pix, back_pix);
  XMapWindow (dpy, holder_win);
  gcm = GCForeground | GCBackground;
  gcv.foreground = hilite_pix;
  gcv.background = fore_pix;
  ReliefGC = XCreateGC (dpy, Root, gcm, &gcv);

  gcm = GCForeground | GCBackground;
  gcv.foreground = shadow_pix;
  gcv.background = fore_pix;
  ShadowGC = XCreateGC (dpy, Root, gcm, &gcv);

  _XA_WM_COLORMAP_WINDOWS = XInternAtom (dpy, "WM_COLORMAP_WINDOWS", False);
}



/***********************************************************************
 *
 *  Procedure:
 *	Loop - wait for data to process
 *
 ***********************************************************************/
#define RIGHT 7
#define LEFT 6
#define BOTTOM 5
#define TOP 4
#define QUIT 3
#define VERTICAL 2
#define HORIZONTAL 1
#define NONE 0
int motion = NONE;

void
Loop (Window target)
{
  Window root;
  int x, y, border_width, depth;
  XEvent Event;
  int tw, th;
  char *temp;
  char *prop = NULL;
  Atom actual = None;
  int actual_format;
  unsigned long nitems, bytesafter;

  while (1)
    {
      XNextEvent (dpy, &Event);
      switch (Event.type)
	{
	case Expose:
	  exposed = 1;
	  RedrawWindow (target);
	  break;

	case ConfigureNotify:
	  XGetGeometry (dpy, main_win, &root, &x, &y,
			(unsigned int *) &tw, (unsigned int *) &th,
			(unsigned int *) &border_width,
			(unsigned int *) &depth);
	  if ((tw != Width) || (th != Height))
	    {
	      XResizeWindow (dpy, holder_win, tw - BAR_WIDTH, th - BAR_WIDTH);
	      Width = tw;
	      Height = th;
	      if (target_y_offset + Height - BAR_WIDTH > target_height)
		target_y_offset = target_height - Height + BAR_WIDTH;
	      if (target_y_offset < 0)
		target_y_offset = 0;
	      if (target_x_offset < 0)
		target_x_offset = 0;
	      if (target_x_offset + Width - BAR_WIDTH > target_width)
		target_x_offset = target_width - Width + BAR_WIDTH;

	      XMoveWindow (dpy, target, -target_x_offset, -target_y_offset);
	      exposed = 1;
	      RedrawWindow (target);
	    }
	  break;

	case ButtonPress:
	  if ((Event.xbutton.y > Height - BAR_WIDTH) &&
	      (Event.xbutton.x < SCROLL_BAR_WIDTH + PAD_WIDTH3))
	    {
	      motion = LEFT;
	      exposed = 2;
	      RedrawWindow (target);
	    }
	  else if ((Event.xbutton.y > Height - BAR_WIDTH) &&
	     (Event.xbutton.x > Width - BAR_WIDTH - SCROLL_BAR_WIDTH - 2) &&
		   (Event.xbutton.x < Width - BAR_WIDTH))
	    {
	      motion = RIGHT;
	      exposed = 2;
	      RedrawWindow (target);
	    }
	  else if ((Event.xbutton.y < SCROLL_BAR_WIDTH + PAD_WIDTH3) &&
		   (Event.xbutton.x > Width - BAR_WIDTH))
	    {
	      motion = TOP;
	      exposed = 2;
	      RedrawWindow (target);
	    }
	  else if ((Event.xbutton.y > Height - BAR_WIDTH - SCROLL_BAR_WIDTH - 2) &&
		   (Event.xbutton.y < Height - BAR_WIDTH) &&
		   (Event.xbutton.x > Width - BAR_WIDTH))
	    {
	      motion = BOTTOM;
	      exposed = 2;
	      RedrawWindow (target);
	    }
	  else if ((Event.xbutton.x > Width - BAR_WIDTH) &&
		   (Event.xbutton.y < Height - BAR_WIDTH))
	    {

	      motion = VERTICAL;
	      target_y_offset = (Event.xbutton.y - PAD_WIDTH3 - SCROLL_BAR_WIDTH) *
		target_height /
		(Height - BAR_WIDTH - PAD_WIDTH3 - 2 * SCROLL_BAR_WIDTH);
	      if (target_y_offset + Height - BAR_WIDTH - PAD_WIDTH3 > target_height)
		target_y_offset = target_height - Height + BAR_WIDTH + PAD_WIDTH3;
	      if (target_y_offset < 0)
		target_y_offset = 0;
	      XMoveWindow (dpy, target, -target_x_offset, -target_y_offset);
	      RedrawWindow (target);
	    }
	  else if ((Event.xbutton.y > Height - BAR_WIDTH) &&
		   (Event.xbutton.x < Width - BAR_WIDTH))
	    {
	      motion = HORIZONTAL;
	      target_x_offset = (Event.xbutton.x - PAD_WIDTH3 - SCROLL_BAR_WIDTH) *
		target_width /
		(Width - BAR_WIDTH - PAD_WIDTH3 - 2 * SCROLL_BAR_WIDTH);
	      if (target_x_offset < 0)
		target_x_offset = 0;

	      if (target_x_offset + Width - BAR_WIDTH - PAD_WIDTH3 > target_width)
		target_x_offset = target_width - Width + BAR_WIDTH + PAD_WIDTH3;
	      XMoveWindow (dpy, target, -target_x_offset, -target_y_offset);
	      RedrawWindow (target);
	    }
	  else if ((Event.xbutton.y > Height - BAR_WIDTH) &&
		   (Event.xbutton.x > Width - BAR_WIDTH))
	    {
	      exposed = 2;
	      motion = QUIT;
	    }
	  RedrawWindow (target);
	  break;

	case ButtonRelease:
	  if ((Event.xbutton.y > Height - BAR_WIDTH) &&
	      (Event.xbutton.x > Width - BAR_WIDTH) &&
	      (motion == QUIT))
	    {
	      XUnmapWindow (dpy, main_win);
	      XReparentWindow (dpy, target, Root, x, y);
	      XSync (dpy, 0);
	      exit (0);
	    }
	  if ((motion == LEFT) && (Event.xbutton.y > Height - BAR_WIDTH) &&
	      (Event.xbutton.x < SCROLL_BAR_WIDTH + PAD_WIDTH3))
	    {
	      target_x_offset -= (Width - BAR_WIDTH - PAD_WIDTH2);
	      if (target_x_offset < 0)
		target_x_offset = 0;
	      XMoveWindow (dpy, target, -target_x_offset, -target_y_offset);
	      motion = NONE;
	      exposed = 2;
	    }
	  else if ((motion == RIGHT) && (Event.xbutton.y > Height - BAR_WIDTH) &&
	     (Event.xbutton.x > Width - BAR_WIDTH - SCROLL_BAR_WIDTH - 2) &&
		   (Event.xbutton.x < Width - BAR_WIDTH))
	    {
	      target_x_offset += (Width - BAR_WIDTH - PAD_WIDTH2);
	      if (target_x_offset + Width - BAR_WIDTH - PAD_WIDTH3 > target_width)
		target_x_offset = target_width - Width + BAR_WIDTH + PAD_WIDTH3;
	      XMoveWindow (dpy, target, -target_x_offset, -target_y_offset);
	      motion = NONE;
	      exposed = 2;
	    }
	  else if ((motion == TOP) &&
		   (Event.xbutton.y < SCROLL_BAR_WIDTH + PAD_WIDTH3) &&
		   (Event.xbutton.x > Width - BAR_WIDTH))
	    {
	      target_y_offset -= (Height - BAR_WIDTH - PAD_WIDTH2);
	      if (target_y_offset < 0)
		target_y_offset = 0;
	      XMoveWindow (dpy, target, -target_x_offset, -target_y_offset);
	      motion = NONE;
	      exposed = 2;
	    }
	  else if ((motion == BOTTOM) &&
	    (Event.xbutton.y > Height - BAR_WIDTH - SCROLL_BAR_WIDTH - 2) &&
		   (Event.xbutton.y < Height - BAR_WIDTH) &&
		   (Event.xbutton.x > Width - BAR_WIDTH))
	    {
	      target_y_offset += (Height - BAR_WIDTH - PAD_WIDTH2);
	      if (target_y_offset + Height - BAR_WIDTH - PAD_WIDTH3 > target_height)
		target_y_offset = target_height - Height + BAR_WIDTH + PAD_WIDTH3;
	      XMoveWindow (dpy, target, -target_x_offset, -target_y_offset);
	      motion = NONE;
	      exposed = 2;
	    }
	  if (motion == VERTICAL)
	    {
	      target_y_offset = (Event.xbutton.y - PAD_WIDTH3 - SCROLL_BAR_WIDTH) * target_height /
		(Height - BAR_WIDTH - PAD_WIDTH3 - 2 * SCROLL_BAR_WIDTH);
	      if (target_y_offset + Height - BAR_WIDTH - PAD_WIDTH3 > target_height)
		target_y_offset = target_height - Height + BAR_WIDTH + PAD_WIDTH3;
	      if (target_y_offset < 0)
		target_y_offset = 0;
	      XMoveWindow (dpy, target, -target_x_offset, -target_y_offset);
	    }
	  if (motion == HORIZONTAL)
	    {
	      target_x_offset = (Event.xbutton.x - PAD_WIDTH3 - SCROLL_BAR_WIDTH) * target_width /
		(Width - BAR_WIDTH - PAD_WIDTH3 - 2 * SCROLL_BAR_WIDTH);
	      if (target_x_offset < 0)
		target_x_offset = 0;

	      if (target_x_offset + Width - BAR_WIDTH - PAD_WIDTH3 > target_width)
		target_x_offset = target_width - Width + BAR_WIDTH + PAD_WIDTH3;
	      XMoveWindow (dpy, target, -target_x_offset, -target_y_offset);
	    }
	  RedrawWindow (target);
	  motion = NONE;
	  break;

	case MotionNotify:
	  if ((motion == LEFT) && ((Event.xmotion.y < Height - BAR_WIDTH) ||
			 (Event.xmotion.x > SCROLL_BAR_WIDTH + PAD_WIDTH3)))
	    {
	      motion = NONE;
	      exposed = 2;
	    }
	  else if ((motion == RIGHT) && ((Event.xmotion.y < Height - BAR_WIDTH) ||
	     (Event.xmotion.x < Width - BAR_WIDTH - SCROLL_BAR_WIDTH - 2) ||
				     (Event.xmotion.x > Width - BAR_WIDTH)))
	    {
	      motion = NONE;
	      exposed = 2;
	    }
	  else if ((motion == TOP) &&
		   ((Event.xmotion.y > SCROLL_BAR_WIDTH + PAD_WIDTH3) ||
		    (Event.xmotion.x < Width - BAR_WIDTH)))
	    {
	      motion = NONE;
	      exposed = 2;
	    }
	  else if ((motion == BOTTOM) &&
	    ((Event.xmotion.y < Height - BAR_WIDTH - SCROLL_BAR_WIDTH - 2) ||
	     (Event.xmotion.y > Height - BAR_WIDTH) ||
	     (Event.xmotion.x < Width - BAR_WIDTH)))
	    {
	      motion = NONE;
	      exposed = 2;
	    }
	  if (motion == VERTICAL)
	    {
	      target_y_offset = (Event.xmotion.y - PAD_WIDTH3 - SCROLL_BAR_WIDTH) *
		target_height /
		(Height - BAR_WIDTH - PAD_WIDTH3 - 2 * SCROLL_BAR_WIDTH);
	      if (target_y_offset + Height - BAR_WIDTH - PAD_WIDTH3 > target_height)
		target_y_offset = target_height - Height + BAR_WIDTH + PAD_WIDTH3;
	      if (target_y_offset < 0)
		target_y_offset = 0;
	      XMoveWindow (dpy, target, -target_x_offset, -target_y_offset);
	    }
	  if (motion == HORIZONTAL)
	    {
	      target_x_offset = (Event.xmotion.x - PAD_WIDTH3 - SCROLL_BAR_WIDTH) *
		target_width /
		(Width - BAR_WIDTH - PAD_WIDTH3 - 2 * SCROLL_BAR_WIDTH);
	      if (target_x_offset < 0)
		target_x_offset = 0;

	      if (target_x_offset + Width - BAR_WIDTH - PAD_WIDTH3 > target_width)
		target_x_offset = target_width - Width + BAR_WIDTH + PAD_WIDTH3;
	      XMoveWindow (dpy, target, -target_x_offset, -target_y_offset);
	    }
	  if ((motion == QUIT) &&
	      ((Event.xbutton.y < Height - BAR_WIDTH) ||
	       (Event.xbutton.x < Width - BAR_WIDTH)))
	    {
	      motion = NONE;
	      exposed = 2;
	    }
	  RedrawWindow (target);
	  break;
	case ClientMessage:
	  if ((Event.xclient.format == 32) &&
	      (Event.xclient.data.l[0] == wm_del_win))
	    {
	      DeadPipe (1);
	    }
	  break;
	case PropertyNotify:
	  if (Event.xproperty.atom == XA_WM_NAME)
	    {
	      if (XFetchName (dpy, target, &temp) == 0)
		temp = NULL;
	      change_window_name (temp);
	    }
	  else if (Event.xproperty.atom == XA_WM_ICON_NAME)
	    {
	      if (XGetWindowProperty (dpy,
				      target, Event.xproperty.atom, 0,
				      MAX_ICON_NAME_LEN, False, XA_STRING,
				      &actual, &actual_format, &nitems,
				      &bytesafter, (unsigned char **) &prop)
		  == Success && (prop != NULL))
		change_icon_name (prop);
	    }
	  else if (Event.xproperty.atom == XA_WM_HINTS)
	    {
	      XWMHints *wmhints;

	      wmhints = XGetWMHints (dpy, target);
	      XSetWMHints (dpy, main_win, wmhints);
	      XFree (wmhints);
	    }
	  else if (Event.xproperty.atom == XA_WM_NORMAL_HINTS)
	    {
	      /* don't do Normal Hints. They alter the size of the window */
	    }
	  else if (Event.xproperty.atom == _XA_WM_COLORMAP_WINDOWS)
	    {
	    }
	  break;

	case DestroyNotify:
	  DeadPipe (1);
	  break;

	case UnmapNotify:
	  break;

	case MapNotify:
	  XMapWindow (dpy, main_win);
	  break;
	case FocusIn:
	  XSetInputFocus (dpy, target, RevertToParent, CurrentTime);
	  break;
	case ColormapNotify:
	  {
	    XWindowAttributes xwa;
	    if (XGetWindowAttributes (dpy, target, &xwa) != 0)
	      {
		XSetWindowColormap (dpy, main_win, xwa.colormap);
	      }
	  }
	  break;
	default:
	  break;
	}
    }
  return;
}



/************************************************************************
 *
 * Draw the window 
 *
 ***********************************************************************/
void
RedrawWindow (Window target)
{
  static int xv = 0, yv = 0, hv = 0, wv = 0;
  static int xh = 0, yh = 0, hh = 0, wh = 0;
  int x, y, w, h;
  XEvent dummy;

  while (XCheckTypedWindowEvent (dpy, main_win, Expose, &dummy))
    exposed |= 1;

  XSetWindowBorderWidth (dpy, target, 0);

  RelieveWindow (main_win, PAD_WIDTH3 - 2, PAD_WIDTH3 - 2,
		 Width - BAR_WIDTH - PAD_WIDTH3 + 4,
		 Height - BAR_WIDTH - PAD_WIDTH3 + 4, ShadowGC, ReliefGC);

  y = (Height - BAR_WIDTH - PAD_WIDTH3 - 2 * SCROLL_BAR_WIDTH) *
    target_y_offset / target_height
    + PAD_WIDTH2 + 2 + SCROLL_BAR_WIDTH;
  x = Width - SCROLL_BAR_WIDTH - PAD_WIDTH2 - 2;
  w = SCROLL_BAR_WIDTH;
  h = (Height - BAR_WIDTH - PAD_WIDTH3 - 2 * SCROLL_BAR_WIDTH) *
    (Height - BAR_WIDTH - PAD_WIDTH3) /
    target_height;
  if ((y != yv) || (x != xv) || (w != wv) || (h != hv) || (exposed & 1))
    {
      yv = y;
      xv = x;
      wv = w;
      hv = h;
      XClearArea (dpy, main_win, x, PAD_WIDTH3 + SCROLL_BAR_WIDTH,
	  w, Height - BAR_WIDTH - PAD_WIDTH3 - 2 * SCROLL_BAR_WIDTH, False);

      RelieveWindow (main_win, x, y, w, h, ReliefGC, ShadowGC);
    }
  if (exposed & 1)
    RelieveWindow (main_win, x - 2, PAD_WIDTH2,
	    w + 4, Height - BAR_WIDTH - PAD_WIDTH2 + 2, ShadowGC, ReliefGC);
  if (exposed)
    {
      if (motion == TOP)
	RedrawTopButton (ShadowGC, ReliefGC, x, PAD_WIDTH3);
      else
	RedrawTopButton (ReliefGC, ShadowGC, x, PAD_WIDTH3);
      if (motion == BOTTOM)
	RedrawBottomButton (ShadowGC, ReliefGC, x,
			    Height - BAR_WIDTH - SCROLL_BAR_WIDTH);
      else
	RedrawBottomButton (ReliefGC, ShadowGC, x,
			    Height - BAR_WIDTH - SCROLL_BAR_WIDTH);
    }

  x = (Width - BAR_WIDTH - PAD_WIDTH3 - 2 * SCROLL_BAR_WIDTH) * target_x_offset /
    target_width + PAD_WIDTH2 + 2 + SCROLL_BAR_WIDTH;
  y = Height - SCROLL_BAR_WIDTH - PAD_WIDTH2 - 2;
  w = (Width - BAR_WIDTH - PAD_WIDTH3 - 2 * SCROLL_BAR_WIDTH) *
    (Width - BAR_WIDTH - PAD_WIDTH3) / target_width;
  h = SCROLL_BAR_WIDTH;
  if ((y != yh) || (x != xh) || (w != wh) || (h != hh) || (exposed & 1))
    {
      yh = y;
      xh = x;
      wh = w;
      hh = h;
      XClearArea (dpy, main_win, PAD_WIDTH3 + SCROLL_BAR_WIDTH, y,
	   Width - BAR_WIDTH - PAD_WIDTH3 - 2 * SCROLL_BAR_WIDTH, h, False);
      RelieveWindow (main_win, x, y, w, h, ReliefGC, ShadowGC);
    }
  if (exposed & 1)
    {
      RelieveWindow (main_win, PAD_WIDTH2, y - 2, Width - BAR_WIDTH - PAD_WIDTH2 + 2, h + 4,
		     ShadowGC, ReliefGC);
    }
  if (exposed)
    {
      if (motion == LEFT)
	RedrawLeftButton (ShadowGC, ReliefGC, PAD_WIDTH3, y);
      else
	RedrawLeftButton (ReliefGC, ShadowGC, PAD_WIDTH3, y);
      if (motion == RIGHT)
	RedrawRightButton (ShadowGC, ReliefGC,
			   Width - BAR_WIDTH - SCROLL_BAR_WIDTH, y);
      else
	RedrawRightButton (ReliefGC, ShadowGC,
			   Width - BAR_WIDTH - SCROLL_BAR_WIDTH, y);
    }

  if (exposed)
    {
      XClearArea (dpy, main_win, Width - BAR_WIDTH + 2,
	       Height - BAR_WIDTH + 2, BAR_WIDTH - 3, BAR_WIDTH - 3, False);
      if (motion == QUIT)
	RelieveWindow (main_win, Width - SCROLL_BAR_WIDTH - PAD_WIDTH2 - 4,
		       Height - SCROLL_BAR_WIDTH - PAD_WIDTH2 - 4,
		       SCROLL_BAR_WIDTH + 4, SCROLL_BAR_WIDTH + 4,
		       ShadowGC, ReliefGC);
      else
	RelieveWindow (main_win, Width - SCROLL_BAR_WIDTH - PAD_WIDTH2 - 4,
		       Height - SCROLL_BAR_WIDTH - PAD_WIDTH2 - 4,
		       SCROLL_BAR_WIDTH + 4, SCROLL_BAR_WIDTH + 4,
		       ReliefGC, ShadowGC);
    }
  exposed = 0;
}


/**************************************************************************
 *  Change the window name displayed in the title bar.
 **************************************************************************/
void
change_window_name (char *str)
{
  XTextProperty name;

  if (str == NULL)
    return;

  if (XStringListToTextProperty (&str, 1, &name) == 0)
    {
      fprintf (stderr, "%s: cannot allocate window name", MyName);
      return;
    }
  XSetWMName (dpy, main_win, &name);
  XFree (name.value);
}


/**************************************************************************
 *  Change the window name displayed in the icon.
 **************************************************************************/
void
change_icon_name (char *str)
{
  XTextProperty name;

  if (str == NULL)
    return;
  if (XStringListToTextProperty (&str, 1, &name) == 0)
    {
      fprintf (stderr, "%s: cannot allocate window name", MyName);
      return;
    }
  XSetWMIconName (dpy, main_win, &name);
  XFree (name.value);
}


/***************************************************************************
 *
 * ICCCM Client Messages - Section 4.2.8 of the ICCCM dictates that all
 * client messages will have the following form:
 *
 *     event type	ClientMessage
 *     message type	_XA_WM_PROTOCOLS
 *     window		tmp->w
 *     format		32
 *     data[0]		message atom
 *     data[1]		time stamp
 *
 ****************************************************************************/
void
send_clientmessage (Window w, Atom a, Time timestamp)
{
  XClientMessageEvent ev;

  ev.type = ClientMessage;
  ev.window = w;
  ev.message_type = _XA_WM_PROTOCOLS;
  ev.format = 32;
  ev.data.l[0] = a;
  ev.data.l[1] = timestamp;
  XSendEvent (dpy, w, False, 0L, (XEvent *) & ev);
}


void
GrabWindow (Window target)
{
  char *temp;
  Window Junk, root;
  unsigned int tw, th, border_width, depth;
  int x, y;
  char *prop = NULL;
  Atom actual = None;
  int actual_format;
  unsigned long nitems, bytesafter;

  XUnmapWindow (dpy, target);
  XSync (dpy, 0);
  XGetGeometry (dpy, target, &root, &x, &y,
		(unsigned int *) &tw, (unsigned int *) &th,
		(unsigned int *) &border_width,
		(unsigned int *) &depth);
  XSync (dpy, 0);

  XTranslateCoordinates (dpy, target, Root, 0, 0, &x, &y, &Junk);

  CreateWindow (x, y, tw, th);
  XSetWindowBorderWidth (dpy, target, 0);
  XReparentWindow (dpy, target, holder_win, 0, 0);
  XMapWindow (dpy, target);
  XSelectInput (dpy, target, PropertyChangeMask | StructureNotifyMask |
		ColormapChangeMask);
  if (XFetchName (dpy, target, &temp) == 0)
    temp = NULL;
  if (XGetWindowProperty (dpy,
			  target, XA_WM_ICON_NAME, 0,
			  MAX_ICON_NAME_LEN, False, XA_STRING,
			  &actual, &actual_format, &nitems,
			  &bytesafter, (unsigned char **) &prop)
      == Success && (prop != NULL))
    {
      change_icon_name (prop);
      XFree (prop);
    }
  change_window_name (temp);
  {
    XWMHints *wmhints;

    wmhints = XGetWMHints (dpy, target);
    if (wmhints != NULL)
      {
	XSetWMHints (dpy, main_win, wmhints);
	XFree (wmhints);
      }
  }
  {
    XWindowAttributes xwa;
    if (XGetWindowAttributes (dpy, target, &xwa) != 0)
      {
	XSetWindowColormap (dpy, main_win, xwa.colormap);
      }
  }

  XMapWindow (dpy, main_win);
  RedrawWindow (target);
  XFree (temp);
}





void
RedrawLeftButton (GC rgc, GC sgc, int x1, int y1)
{
  XSegment seg[4];
  int i = 0;

  seg[i].x1 = x1 + 1;
  seg[i].y1 = y1 + SCROLL_BAR_WIDTH / 2;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 2;
  seg[i++].y2 = y1 + 1;

  seg[i].x1 = x1;
  seg[i].y1 = y1 + SCROLL_BAR_WIDTH / 2;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 1;
  seg[i++].y2 = y1;
  XDrawSegments (dpy, main_win, rgc, seg, i);

  i = 0;
  seg[i].x1 = x1 + 1;
  seg[i].y1 = y1 + SCROLL_BAR_WIDTH / 2;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 2;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1;
  seg[i].y1 = y1 + SCROLL_BAR_WIDTH / 2;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 1;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 1;

  seg[i].x1 = x1 + SCROLL_BAR_WIDTH - 2;
  seg[i].y1 = y1 + 1;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 2;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1 + SCROLL_BAR_WIDTH - 1;
  seg[i].y1 = y1;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 1;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 1;
  XDrawSegments (dpy, main_win, sgc, seg, i);
}

void
RedrawRightButton (GC rgc, GC sgc, int x1, int y1)
{
  XSegment seg[4];
  int i = 0;

  seg[i].x1 = x1 + 1;
  seg[i].y1 = y1 + 1;
  seg[i].x2 = x1 + 1;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1;
  seg[i].y1 = y1;
  seg[i].x2 = x1;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 1;

  seg[i].x1 = x1 + 1;
  seg[i].y1 = y1 + 1;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 2;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH / 2;

  seg[i].x1 = x1;
  seg[i].y1 = y1;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 1;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH / 2;

  XDrawSegments (dpy, main_win, rgc, seg, i);

  i = 0;
  seg[i].x1 = x1;
  seg[i].y1 = y1 + SCROLL_BAR_WIDTH - 2;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 2;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH / 2;

  seg[i].x1 = x1;
  seg[i].y1 = y1 + SCROLL_BAR_WIDTH - 1;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 1;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH / 2;
  XDrawSegments (dpy, main_win, sgc, seg, i);
}

void
RedrawTopButton (GC rgc, GC sgc, int x1, int y1)
{
  XSegment seg[4];
  int i = 0;

  seg[i].x1 = x1 + SCROLL_BAR_WIDTH / 2;
  seg[i].y1 = y1 + 1;
  seg[i].x2 = x1 + 1;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1 + SCROLL_BAR_WIDTH / 2;
  seg[i].y1 = y1;
  seg[i].x2 = x1 + 0;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 1;
  XDrawSegments (dpy, main_win, rgc, seg, i);

  i = 0;
  seg[i].x1 = x1 + SCROLL_BAR_WIDTH / 2;
  seg[i].y1 = y1 + 1;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 2;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1 + SCROLL_BAR_WIDTH / 2;
  seg[i].y1 = y1;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 1;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 1;

  seg[i].x1 = x1 + 1;
  seg[i].y1 = y1 + SCROLL_BAR_WIDTH - 2;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 2;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1 + 0;
  seg[i].y1 = y1 + SCROLL_BAR_WIDTH - 1;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 1;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 1;
  XDrawSegments (dpy, main_win, sgc, seg, i);
}

void
RedrawBottomButton (GC rgc, GC sgc, int x1, int y1)
{
  XSegment seg[4];
  int i = 0;

  seg[i].x1 = x1 + 1;
  seg[i].y1 = y1 + 1;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH / 2;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1;
  seg[i].y1 = y1 + 0;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH / 2;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 1;

  seg[i].x1 = x1 + 1;
  seg[i].y1 = y1 + 1;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 2;
  seg[i++].y2 = y1 + 1;

  seg[i].x1 = x1;
  seg[i].y1 = y1 + 0;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH - 1;
  seg[i++].y2 = y1 + 0;
  XDrawSegments (dpy, main_win, rgc, seg, i);

  i = 0;
  seg[i].x1 = x1 + SCROLL_BAR_WIDTH - 2;
  seg[i].y1 = y1 + 1;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH / 2;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1 + SCROLL_BAR_WIDTH - 1;
  seg[i].y1 = y1 + 0;
  seg[i].x2 = x1 + SCROLL_BAR_WIDTH / 2;
  seg[i++].y2 = y1 + SCROLL_BAR_WIDTH - 1;
  XDrawSegments (dpy, main_win, sgc, seg, i);
}
