/*
 * Copyright (C) 1998 Eric Tremblay <deltax@pragma.net>
 * Copyright (c) 1998 Doug Alcorn <alcornd@earthlink.net>
 * Copyright (C) 1998 Sasha Vasko <sasha at aftercode.net>
 * Copyright (C) 1998 Makoto Kato <m_kato@ga2.so-net.ne.jp>
 * Copyright (C) 1998 Ric Lister <ric@giccs.georgetown.edu>
 * Copyright (C) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (C) 1996 Dan Weeks
 * Copyright (C) 1995 Rob Nation
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
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#define IN_MODULE
#include "../../include/afterbase.h"
#include "../../libAfterImage/afterimage.h"
#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"
#include "../../include/mystyle.h"
#include "../../include/pixmap.h"
#include "../../include/background.h"
#include "Pager.h"

#ifdef DEBUG_X_PAGER
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


int CheckForDrawChild (int);
extern Display *dpy;

extern int window_w, window_h, window_x, window_y, window_x_negative, window_y_negative,
  usposition;
extern int icon_w, icon_h, icon_x, icon_y;

extern char *MyName;

int desk_w = 0;
int desk_h = 0;
int scr_w = 0;
int scr_h = 0;
int label_h = 0;
int label_y = 0;

int WaitASResponse = 0;
XErrorHandler
MyXErrorHandler (Display * d, XErrorEvent * error)
{
  LOG4 ("\n%s: XError # %u, in resource %lu", MyName,
	error->error_code, error->resourceid);
  LOG3 (", Request: %d.%d", error->request_code, error->minor_code)
    return 0;
};


Window icon_win;		/* icon window */

void
InitDesk (long Desk)
{
  char line[100];
  LOG2 ("\nEnter InitDesk %ld", Desk);
  sprintf (line, "Desk %lu", Desk + Pager.desk1);
  CopyString (&Pager.Desks[Desk].label, line);
  Pager.Desks[Desk].exposed = False;

  Pager.Desks[Desk].StyleName = NULL;
  Look.DeskStyles[Desk] = NULL;
  LOG2 ("\nExit InitDesk %ld", Desk);
}

#ifdef PAGER_BACKGROUND
void
RetrieveBackgrounds ()
{
  int i;
  ASDeskBack *back;
  MyStyle *Style;
  char *style_name;

  GetBackgroundsProperty (&(Pager.Backgrounds), Atoms[ATOM_BACKGROUNDS].atom);

  /* some postprocessing for desks that gets filled with MyStyle */
  for (i = 0; i < Pager.ndesks; i++)
    if (Pager.Desks[i].StyleName == NULL)
      if ((back = FindDeskBack (&(Pager.Backgrounds), i + Pager.desk1)) != NULL)
	if (back->MyStyle != None)
	  if ((style_name = XGetAtomName (dpy, back->MyStyle)) != NULL)
	    {
	      if ((Style = mystyle_find (style_name)) != NULL)
		Look.DeskStyles[i] = Style;
	      else
		Look.DeskStyles[i] = NULL;
	      XFree (style_name);
	    }
}
#endif

/***********************************************************************
 *
 *  Procedure:
 *   Initialize_pager - creates the pager window, if needed
 *
 *  Inputs:
 *   x,y location of the window
 *
 ***********************************************************************/
char *pager_name = "AfterStep Pager";
XSizeHints sizehints =
{
  (PMinSize | PResizeInc | PBaseSize | PWinGravity),
  0, 0, 100, 100,		/* x, y, width and height */
  1, 1,				/* Min width and height */
  0, 0,				/* Max width and height */
  1, 1,				/* Width and height increments */
  {0, 0},
  {0, 0},			/* Aspect ratio - not used */
  1, 1,				/* base size */
  (NorthWestGravity)		/* gravity */
};

void
CalcDeskSize ()
{
  desk_w = window_w / Pager.Columns - (Look.DeskBorderWidth * 2);
  desk_h = window_h / Pager.Rows - label_h - Look.DeskBorderWidth * 3;
  label_y = ((Pager.Flags & LABEL_BELOW_DESK) ? desk_h + Look.DeskBorderWidth * 2 : 0);
}

#define SCREEN2PAGER_X(sx)	(((sx)*scr_w)/Scr.MyDisplayWidth)
#define SCREEN2PAGER_Y(sy)	(((sy)*scr_h)/Scr.MyDisplayHeight)
#define PAGER2SCREEN_X(sx)	(((sx)*Scr.MyDisplayWidth)/scr_w)
#define PAGER2SCREEN_Y(sy)	(((sy)*Scr.MyDisplayHeight)/scr_h)

void
CalcViewPortXY (int *x, int *y)
{
  *x = SCREEN2PAGER_X (Scr.Vx);
  *y = SCREEN2PAGER_Y (Scr.Vy) + 1;
}

void
initialize_pager (void)
{
  XTextProperty name;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  XWMHints wmhints;
  int i, l;
  XClassHint class1;

  XSetErrorHandler ((XErrorHandler) MyXErrorHandler);

#ifdef SHAPE
  /* We need SHAPE extensions to hide inactive lables. */
  if (Pager.Flags & HIDE_INACTIVE_LABEL)
    {
      int s1, s2;
      if (!XShapeQueryExtension (dpy, &s1, &s2))
	Pager.Flags &= ~HIDE_INACTIVE_LABEL;
    }
#endif

  /* Size the window */
  if (Pager.Rows <= 0)
    {
      if (Pager.Columns <= 0)
	{
	  Pager.Columns = Pager.ndesks;
	  Pager.Rows = 1;
	}
      else
	{
	  Pager.Rows = Pager.ndesks / Pager.Columns;
	  if (Pager.Rows * Pager.Columns < Pager.ndesks)
	    Pager.Rows++;
	}
    }
  else if (Pager.Columns <= 0)
    {
      Pager.Columns = Pager.ndesks / Pager.Rows;
      if (Pager.Rows * Pager.Columns < Pager.ndesks)
	Pager.Columns++;
    }
  else if (Pager.Rows * Pager.Columns != Pager.ndesks)
    {
      Pager.Rows = Pager.ndesks / Pager.Columns;
      if (Pager.Rows * Pager.Columns < Pager.ndesks)
	Pager.Rows++;
    }

  LOG3 ("\nPager: Rows=%d, Columns=%d", Pager.Rows, Pager.Columns)
    CalcDeskSize ();
  if (window_w > 0)
    {
      window_w = (desk_w + Look.DeskBorderWidth * 2) * Pager.Columns;
      if (window_w > 0)
	Scr.VScale = Pager.xSize / window_w;
    }
  if (window_h > 0)
    {
      window_h = (desk_h + label_h + Look.DeskBorderWidth * 3) * Pager.Rows;
      if (window_h > label_h)
	Scr.VScale = Pager.ySize / (window_h - label_h - Look.DeskBorderWidth * 3);
      else
	window_h = 0;
    }

  if (window_w <= 0)
    window_w = Pager.Columns * (Pager.xSize / Scr.VScale + (Pager.PageColumns - 1) + Look.DeskBorderWidth * 2);
  if (window_h <= 0)
    window_h = Pager.Rows * (Pager.ySize / Scr.VScale + (Pager.PageRows - 1) + label_h + Look.DeskBorderWidth * 3);

  if (window_x < 0 || (window_x == 0 && window_x_negative == 1))
    {
      sizehints.win_gravity = NorthEastGravity;
      window_x = Scr.MyDisplayWidth - window_w + window_x;
    }
  if (window_y < 0 || (window_y == 0 && window_y_negative == 1))
    {
      window_y = Scr.MyDisplayHeight - window_h + window_y;
      if (sizehints.win_gravity == NorthEastGravity)
	sizehints.win_gravity = SouthEastGravity;
      else
	sizehints.win_gravity = SouthWestGravity;
    }
  if (usposition)
    sizehints.flags |= USPosition;

  valuemask = (CWEventMask);
  attributes.event_mask = (StructureNotifyMask);
  sizehints.width = window_w;
  sizehints.height = window_h;
  sizehints.x = window_x;
  sizehints.y = window_y;
  sizehints.width_inc = Pager.Columns * Pager.PageColumns;
  sizehints.height_inc = Pager.Rows * Pager.PageRows;
  sizehints.base_width = Pager.Columns * (Pager.PageColumns * 2 - 1) + Pager.Columns * 2 - 1;
  sizehints.base_height = Pager.Rows * (Pager.PageRows * 2 + label_h) + Pager.PageRows * 2 - 1;
  sizehints.min_width = sizehints.base_width;
  sizehints.min_height = sizehints.base_height;

  Pager.Pager_w = XCreateWindow (dpy, Scr.Root, window_x, window_y, window_w,
				 window_h, (unsigned int) 1,
				 CopyFromParent, InputOutput,
				 (Visual *) CopyFromParent,
				 valuemask, &attributes);
  XSetWMProtocols (dpy, Pager.Pager_w, &Atoms[ATOM_WM_DEL_WIN].atom, 1);
  XSetWMNormalHints (dpy, Pager.Pager_w, &sizehints);
  XSetWindowBackgroundPixmap (dpy, Pager.Pager_w, ParentRelative);

  if ((Pager.desk1 == Pager.desk2) && (Pager.Desks[0].label != NULL))
    XStringListToTextProperty (&Pager.Desks[0].label, 1, &name);
  else
    XStringListToTextProperty (&pager_name, 1, &name);

  attributes.event_mask = (StructureNotifyMask | ExposureMask);
  if (icon_w < 1)
    icon_w = (window_w - Pager.Columns + 1) / Pager.Columns;
  if (icon_h < 1)
    icon_h = (window_h - Pager.Rows * label_h - Pager.Rows + 1) / Pager.Rows;

  icon_w = ((icon_w / Pager.PageColumns) + 1) * Pager.PageColumns - 1;
  icon_h = ((icon_h / Pager.PageRows) + 1) * (Pager.PageRows) - 1;
  icon_win = XCreateWindow (dpy, Scr.Root, window_x, window_y,
			    icon_w, icon_h,
			    (unsigned int) 1,
			    CopyFromParent, InputOutput,
			    (Visual *) CopyFromParent,
			    valuemask, &attributes);
  XGrabButton (dpy, 1, AnyModifier, icon_win,
	       True, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
	       GrabModeAsync, GrabModeAsync, None,
	       None);
  XGrabButton (dpy, 2, AnyModifier, icon_win,
	       True, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
	       GrabModeAsync, GrabModeAsync, None,
	       None);
  XGrabButton (dpy, 3, AnyModifier, icon_win,
	       True, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
	       GrabModeAsync, GrabModeAsync, None,
	       None);
  if (!(Pager.Flags & START_ICONIC))
    wmhints.initial_state = NormalState;
  else
    wmhints.initial_state = IconicState;
  wmhints.flags = 0;
  if (icon_x > -10000)
    {
      if (icon_x < 0)
	icon_x = Scr.MyDisplayWidth + icon_x - icon_w;
      if (icon_y > -10000)
	{
	  if (icon_y < 0)
	    icon_y = Scr.MyDisplayHeight + icon_y - icon_h;
	}
      else
	icon_y = 0;

      wmhints.icon_x = icon_x;
      wmhints.icon_y = icon_y;
      wmhints.flags = IconPositionHint;
    }
  wmhints.icon_window = icon_win;
  wmhints.input = False;

  wmhints.flags |= InputHint | StateHint | IconWindowHint;

/* here indicating that we are still loading, so AS would not think that we are done already */
#ifdef PAGER_BACKGROUND
  if (Pager.Flags & SET_ROOT_ON_STARTUP)
    BackgroundSetForDesk (&(Pager.Backgrounds), Pager.desk1);
#endif

  class1.res_name = MyName;	/* for future use */
  class1.res_class = "ASModule";

  XSetWMProperties (dpy, Pager.Pager_w, &name, &name, NULL, 0,
		    &sizehints, &wmhints, &class1);
  /* showing window to let user see that we are doing something */
  XMapRaised (dpy, Pager.Pager_w);

  CalcDeskSize ();

  valuemask = (CWEventMask);

  attributes.event_mask = (ExposureMask | ButtonReleaseMask | ButtonPressMask | ButtonMotionMask);


  for (i = 0; i < Pager.ndesks; i++)
    {
      Pager.Desks[i].title_w = XCreateWindow (dpy, Pager.Pager_w,
				      0, 0, desk_w, desk_h + label_h + 2, 1,
					      CopyFromParent,
					      InputOutput, CopyFromParent,
					      valuemask,
					      &attributes);
      XSetWindowBorderWidth (dpy, Pager.Desks[i].title_w, Look.DeskBorderWidth);

      Pager.Desks[i].w = XCreateWindow (dpy, Pager.Desks[i].title_w, -1, label_h - 1, desk_w, desk_h, 1,
					CopyFromParent,
					InputOutput, CopyFromParent,
					valuemask, &attributes);
      XSelectInput (dpy, Pager.Desks[i].w, attributes.event_mask);
      XSetWindowBorderWidth (dpy, Pager.Desks[i].w, Look.DeskBorderWidth);

      XMapRaised (dpy, Pager.Desks[i].w);
      XMapRaised (dpy, Pager.Desks[i].title_w);
    }

  attributes.event_mask = 0;
  for (l = 0; l < 4 && (Pager.Flags & SHOW_SELECTION); l++)
    {

      Pager.SelectionWin[l] = XCreateWindow (dpy, Pager.Desks[0].w, -1000, -1000, 1, 1, 0,
				CopyFromParent, InputOutput, CopyFromParent,
					     valuemask, &attributes);
      XMapRaised (dpy, Pager.SelectionWin[l]);
    }

  /* final cleanup */
  XFree ((char *) name.value);
  /* need to do this prior to resizing window to properly resie it ;) */
  XSync (dpy, False);
  Pager.bStarted = 1;
  /* we need this to get right size of the Pager after all this mess above */
  XResizeWindow (dpy, Pager.Pager_w, window_w, window_h);
  XSync (dpy, False);

  window_w--;
  ReConfigureEx (window_w, window_h);
}

/*
 * Pointer handling functions
 */
typedef struct
{
  unsigned int my_width, my_height;
  int icon_action, title_origin;
  PagerWindow *start_window;
  PagerWindow *curr_window;
  unsigned int start_desk;
  int start_x, start_y;
  unsigned int curr_desk;
  int curr_x, curr_y;
  int curr_row, curr_col;
}
PointerActionData;

int
InitPointerAction (PointerActionData * data, XEvent * event)
{
  int i = 0;
  PagerWindow *t;
  data->start_x = data->curr_x = event->xbutton.x;
  data->start_y = data->curr_y = event->xbutton.y;
  data->title_origin = 0;

  if (event->xany.window == icon_win)
    {
      data->icon_action = TRUE;
      data->my_width = icon_w;
      data->my_height = icon_h;
      for (t = Pager.Start; t != NULL; t = t->next)
	if (t->IconView == event->xbutton.subwindow)
	  break;
    }
  else
    {
      data->icon_action = FALSE;
      for (; i < Pager.ndesks; i++)
	{
	  if (event->xany.window == Pager.Desks[i].w)
	    {
	      CalcViewPortXY (&(data->start_x), &(data->start_y));
	      break;
	    }
	  if (event->xany.window == Pager.Desks[i].title_w)
	    {
	      data->start_x = data->start_y = 0;
	      data->title_origin = 1;
	      break;
	    }
	}
      data->start_desk = data->curr_desk = i;
      data->my_width = desk_w;
      data->my_height = desk_h;
      /* let's findout what window we have a click over */
      for (t = Pager.Start; t != NULL; t = t->next)
	if (t->PagerView == event->xbutton.subwindow)
	  break;
    }

  data->start_window = data->curr_window = t;

  data->curr_row = i / Pager.Columns;
  data->curr_col = i - (data->curr_row * Pager.Columns);

  return (i < Pager.ndesks);
}

int
UpdatePointerAction (PointerActionData * data, XEvent * event)
{
  /* check if we actually changed our position */
  if (data->curr_x == event->xbutton.x && data->curr_y == event->xbutton.y)
    return 0;

  data->curr_x = event->xbutton.x;
  data->curr_y = event->xbutton.y;
  if (data->icon_action)
    {
      if (data->curr_x < 0 || data->curr_x > data->my_width ||
	  data->curr_y < 0 || data->curr_y > data->my_height)
	data->curr_desk = Pager.ndesks;		/* invalidating desk */
    }
  else
    {
      /* now let's find out on what desk we are, and translate coordinates
         into this desk coordinates */
      data->curr_row = data->start_desk / Pager.Columns;
      data->curr_col = data->start_desk - data->curr_row * Pager.Columns;

      while (data->curr_x < 0)
	{
	  data->curr_col--;
	  data->curr_x += desk_w + (Look.DeskBorderWidth * 2);
	}
      while (data->curr_x > desk_w)
	{
	  data->curr_col++;
	  data->curr_x -= desk_w + (Look.DeskBorderWidth * 2);
	}
      while (data->curr_y < ((data->title_origin) ? 0 : -label_h))
	{
	  data->curr_row--;
	  data->curr_y += desk_h + (Look.DeskBorderWidth * 2) + label_h;
	}
      while (data->curr_y > desk_h + ((data->title_origin) ? label_h : 0))
	{
	  data->curr_row++;
	  data->curr_y -= desk_h + (Look.DeskBorderWidth * 2) + label_h;
	}
      /* let's calculate what desk we are on at the moment */
      if (data->curr_row >= 0 && data->curr_row < Pager.Rows &&
	  data->curr_col >= 0 && data->curr_col < Pager.Columns)
	data->curr_desk = data->curr_row * Pager.Columns + data->curr_col;
      else
	data->curr_desk = Pager.ndesks;		/* invalidating desk */
    }
  return 1;
}

void DoViewPortMoving (PointerActionData * data, XEvent * event);
void
RestoreDeskAndPage (PointerActionData * data, XEvent * event)
{
  data->curr_desk = data->start_desk;
  data->curr_x = data->start_x;
  data->curr_y = data->start_y;
  DoViewPortMoving (data, event);
}

void
SaveVirtualPos (PointerActionData * data)
{
  data->start_x = (Scr.Vx * data->my_width) / Pager.xSize;
  data->start_y = (Scr.Vy * data->my_height) / Pager.ySize;
}

void
DoDeskPageSwitching (PointerActionData * data, XEvent * event)
{
#ifndef NON_VIRTUAL

  if (event->type == ButtonPress)
    SaveVirtualPos (data);

  if (data->curr_desk < Pager.ndesks)
    {
      char command[64];
      int page_column, page_row;

      /* calculating virtual page row and column */
      page_column = data->curr_x / (data->my_width / Pager.PageColumns);
      page_row = data->curr_y / (data->my_height / Pager.PageRows);

      /* range checking */
      if (page_column > Pager.PageColumns)
	page_column = Pager.PageColumns;
      if (page_row > Pager.PageRows)
	page_row = Pager.PageRows;
      /* no need to bother AfterStep if we do not change anything */
      if (page_column * Scr.MyDisplayWidth != Scr.Vx ||
	  page_row * Scr.MyDisplayHeight != Scr.Vy)
	{
	  if (!(data->icon_action) &&
	      data->curr_desk != Scr.CurrentDesk - Pager.desk1)
	    {			/* that will prevent AS from switching to page on the current desk */
	      /* and only then changing desk - avoiding blinking redraws         */
	      SendInfo (Pipes.fd, "Desk 0 10000", 0);
	      WaitASResponse++;
	    }
	  sprintf (command, "GotoPage %d %d\n", page_column, page_row);
	  SendInfo (Pipes.fd, command, 0);
	  WaitASResponse++;
	}

      if (!(data->icon_action) &&
	  data->curr_desk != Scr.CurrentDesk - Pager.desk1)
	{
	  sprintf (command, "Desk 0 %d\n", data->curr_desk + Pager.desk1);
	  SendInfo (Pipes.fd, command, 0);
	  WaitASResponse++;
	}
    }
  else if (event->type == ButtonRelease)
    RestoreDeskAndPage (data, event);
#endif
}

void
DoViewPortMoving (PointerActionData * data, XEvent * event)
{
#ifndef NON_VIRTUAL
  char command[64];
  long sx, sy;

  if (event->type == ButtonPress)
    SaveVirtualPos (data);

  if (data->curr_desk < Pager.ndesks)
    {
      if (!(data->icon_action) &&
	  data->curr_desk != Scr.CurrentDesk - Pager.desk1)
	{
	  sprintf (command, "Desk 0 %d\n", data->curr_desk + Pager.desk1);
	  SendInfo (Pipes.fd, command, 0);
	  WaitASResponse++;
	}

      /* first translating coordinates in our window into coordinates
         on the actual virtual desktop */
      sx = data->curr_x * Pager.AspectX;
      sy = data->curr_y * Pager.AspectY;
      /* now let's check for validness */
      if (sx > Scr.VxMax)
	sx = Scr.VxMax;
      else if (sx < 0)
	sx = 0;
      if (sy > Scr.VyMax)
	sy = Scr.VyMax;
      else if (sy < 0)
	sy = 0;
      /* now calculating delta */
      sx -= Scr.Vx;
      sy -= Scr.Vy;
      /* now translating delta into persentage of the screen width */
      sx = (100 * sx) / Scr.MyDisplayWidth;
      sy = (100 * sy) / Scr.MyDisplayHeight;
      /* we don't want to move in very small increments */
      if (event->type == MotionNotify)
	if (sx < PAGE_MOVE_THRESHOLD && sy < PAGE_MOVE_THRESHOLD &&
	    sx > -(PAGE_MOVE_THRESHOLD) && sy > -(PAGE_MOVE_THRESHOLD))
	  return;

      sprintf (command, "Scroll %ld %ld\n", sx, sy);
      SendInfo (Pipes.fd, command, 0);
      WaitASResponse++;
    }
  else if (event->type == ButtonRelease)
    RestoreDeskAndPage (data, event);
#endif
}

void
ChangeWinDesk (PagerWindow * t, int to_desk)
{
  if (!IS_STICKY (t))
    {
      char command[32];
      sprintf (command, "WindowsDesk %d", to_desk);
      SendInfo (Pipes.fd, command, t->w);
    }
}

void
DoWindowMoving (PointerActionData * data, XEvent * event)
{
  int sx, sy;
  PagerWindow *t = data->start_window;

  if (data->curr_desk >= Pager.ndesks)
    {
      if (event->type != ButtonPress && data->start_window != NULL)
	{
	  Window swin;
	  XTranslateCoordinates (dpy, event->xany.window, Scr.Root,
		       event->xbutton.x, event->xbutton.y, &sx, &sy, &swin);
	  swin = (data->start_window->flags & ICONIFIED) ? data->start_window->icon_w :
	    data->start_window->w;
	  /* positioning window on the screen first */
	  XMoveWindow (dpy, swin, sx, sy);
	  /* now let's let it go ... */
	  XUngrabPointer (dpy, CurrentTime);
	  XSync (dpy, 0);
	  if (!data->icon_action && t->desk != Scr.CurrentDesk)
	    ChangeWinDesk (t, Scr.CurrentDesk);
	  /* ... AS will move it from here. */
	  SendInfo (Pipes.fd, "Move", swin);
	  XFlush (dpy);
	}
      return;
    }

  if (data->start_window != NULL && data->curr_y > 0)
    {
      Window view;
      static int handle_x = 0, handle_y = 0;
      int vlines, hlines;
      view = (data->icon_action) ? t->IconView : t->PagerView;
      if (event->type == ButtonPress)
	{			/* raising our window so that it does not get lost underneath
				   other windows while we are moving it */
	  XTranslateCoordinates (dpy, event->xany.window, view,
				 event->xbutton.x, event->xbutton.y,
				 &handle_x, &handle_y, &view);
	  /*correction for border width */
	  handle_x--;
	  handle_y--;
	  SendInfo (Pipes.fd, "Raise", t->w);
	  WaitASResponse++;
	  return;
	}
      if (event->type != ButtonRelease)
	{
	  data->curr_x -= handle_x;
/*          if( data->curr_x < 0 ) data->curr_x = 0 ; */
	  data->curr_y -= handle_y;
/*          if( data->curr_y < 0 ) data->curr_y = 0 ; */
	}
      /* size of the separator lines */
      hlines = ((data->curr_x * Pager.AspectX) / Scr.MyDisplayWidth) - 1;
      vlines = ((data->curr_y * Pager.AspectY) / Scr.MyDisplayHeight) - 1;

      /* Screen coordinates relative to current virtual desktop position */
      sx = PAGER2SCREEN_X (data->curr_x - hlines) - Scr.Vx;
      sy = PAGER2SCREEN_Y (data->curr_y - vlines) - Scr.Vy;
      /* for sticky windows - they must not leave active page/desk */
      if (IS_STICKY (t))
	{			/* sticky windows can only be moved inside current viewport */
	  if (sx < 0)
	    sx = 0;
	  else if (sx > Scr.MyDisplayWidth)
	    sx = Scr.MyDisplayWidth;
	  if (sy < 0)
	    sy = 0;
	  else if (sy > Scr.MyDisplayHeight)
	    sy = Scr.MyDisplayHeight;
	  data->curr_desk = Scr.CurrentDesk - Pager.desk1;
	}
      /* converting screen coordinates back into pager coordinates to
         be safe */
      data->curr_x = SCREEN2PAGER_X (sx + Scr.Vx) + hlines;
      data->curr_y = SCREEN2PAGER_Y (sy + Scr.Vy) + vlines;
      /* now lets move our window inside the pager */
      if (event->type != ButtonRelease)
	{
	  if (data->curr_desk + Pager.desk1 != t->desk && !data->icon_action)
	    {
	      t->desk = data->curr_desk + Pager.desk1;
	      XReparentWindow (dpy, view, Pager.Desks[data->curr_desk].w,
			       data->curr_x, data->curr_y);
	    }
	  else
	    XMoveWindow (dpy, view, data->curr_x, data->curr_y);
	}
      else
	{			/* end of moving - making changes permanent */
	  if (!(data->icon_action) && data->start_desk + Pager.desk1 != t->desk)
	    ChangeWinDesk (t, data->curr_desk + Pager.desk1);
	  XMoveWindow (dpy, ((t->flags & ICONIFIED) ? t->icon_w : t->w), sx, sy);
	}
    }
}
/****************************************************************************
 *
 * Decide what to do about received X events
 *
 ****************************************************************************/
void
DispatchEvent (XEvent * Event)
{
  int i;
  static PointerActionData PointerData;
  static void (*PointerActions[3][5]) (PointerActionData *, XEvent *) =
  {
    {DoDeskPageSwitching, DoWindowMoving, DoViewPortMoving, NULL, NULL},
    {DoDeskPageSwitching, DoWindowMoving, DoViewPortMoving, NULL, NULL},
    {DoDeskPageSwitching, DoWindowMoving, DoViewPortMoving, NULL, NULL}
  };

  switch (Event->type)
    {
    case ConfigureNotify:
      ReConfigureEx (Event->xconfigure.width, Event->xconfigure.height);
      break;
    case Expose:
      HandleExpose (Event);
      break;
      /* mouse buttons :
         1 (right)        - switch page(&desk)
         2 (middle)       - move window
         3 (left)         - move viewport
       */
    case ButtonPress:

      if (!InitPointerAction (&PointerData, Event))
	break;
      if (PointerActions[0][Event->xbutton.button - 1])
	(PointerActions[0][Event->xbutton.button - 1]) (&PointerData, Event);
      break;
    case MotionNotify:
      while (XCheckMaskEvent (dpy, PointerMotionMask | ButtonMotionMask, Event));
      if (UpdatePointerAction (&PointerData, Event))
	{
	  if (Event->xmotion.state & Button1MotionMask)
	    {
	      if (PointerActions[1][0])
		PointerActions[1][0] (&PointerData, Event);
	    }
	  else if (Event->xmotion.state & Button2MotionMask)
	    {
	      if (PointerActions[1][1])
		(PointerActions[1][1]) (&PointerData, Event);
	    }
	  else if (Event->xmotion.state & Button3MotionMask)
	    {
	      if (PointerActions[1][2])
		(PointerActions[1][2]) (&PointerData, Event);
	    }
	}
      break;
    case ButtonRelease:
      if (PointerActions[2][Event->xbutton.button - 1])
	(PointerActions[2][Event->xbutton.button - 1]) (&PointerData, Event);

    case ClientMessage:
      if ((Event->xclient.format == 32) &&
	  (Event->xclient.data.l[0] == Atoms[ATOM_WM_DEL_WIN].atom))
	{
	  exit (0);
	}
      break;
    case PropertyNotify:
      {
	/* if user used some Esetroot compatible prog to set the root
	 * bg, use the property to determine that. We don't use it's
	 * value, though */

	if (Event->xproperty.atom == Atoms[ATOM_STYLES].atom)
	  OnLookUpdated ();
#ifdef PAGER_BACKGROUND
	else if (Event->xproperty.atom == Atoms[ATOM_ROOT_PIXMAP].atom)
	  {
	    static Pixmap lastRootPixmap = None;
		if( Scr.RootImage )
		    destroy_asimage( &(Scr.RootImage));

	    Pager.CurentRootBack = GetRootPixmap (Atoms[ATOM_ROOT_PIXMAP].atom);
	    if (lastRootPixmap != Pager.CurentRootBack)
	      {			/* if it was changed not by us */
			UpdateTransparency ();
			lastRootPixmap = Pager.CurentRootBack;
	      }
	  }
	else if (Event->xproperty.atom == Atoms[ATOM_BACKGROUNDS].atom)
	  {
	    RetrieveBackgrounds ();
	    if (Scr.CurrentDesk >= Pager.desk1 && Scr.CurrentDesk <= Pager.desk2
		&& Pager.Flags & REDRAW_BG)
	      {
		int waiting;
		/* drawing may not have finished yet - waiting up to 5 minutes */
		for (waiting = 0; CheckForDrawChild (0) && waiting < 300; waiting++)
		  sleep (1);
		BackgroundSetForDesk (&(Pager.Backgrounds), Scr.CurrentDesk);
	      }
	    /* we should update all our window backgrounds here if
	       they are not defined in *PagerDeskStyle */
	    for (i = 0; i < Pager.ndesks; i++)
	      if (Pager.Desks[i].StyleName == NULL &&
	       FindDeskBack (&(Pager.Backgrounds), i + Pager.desk1) != NULL)
		{
		  DecorateDesk (i);
		  DrawGrid (i);
		}
	  }
#endif
      }				/* case */
      break;
    }
}

void
HandleExpose (XEvent * Event)
{
  int i;
  PagerWindow *t;
  static Bool b_first_expose = True;

  for (i = 0; i < Pager.ndesks; i++)
    {
      if (Event->xany.window == Pager.Desks[i].w)
	{
	  if (!Pager.Desks[i].exposed)
	    {
	      DecorateDesk (i);
	      Pager.Desks[i].exposed = True;
	    }
	  DrawGrid (i);
	}
      else if (Event->xany.window == Pager.Desks[i].title_w)
	LabelDesk (i);
    }
  if (Event->xany.window == icon_win)
    DrawIconGrid (0);

  for (t = Pager.Start; t != NULL; t = t->next)
    if (t->PagerView == Event->xany.window)
      LabelWindow (t);
    else if (t->IconView == Event->xany.window)
      LabelIconWindow (t);

  b_first_expose = False;
}

/****************************************************************************
 *
 * These function will display and hide frame around desk in Pager window
 * frame consist of 4 windows forming sides of the Box
 *
 ****************************************************************************/
void
DisplayFrame (long Desk)
{
  int x, y;
  static Window old_w = None;
  Window new_w = Pager.Desks[Desk].w;
  int i;

  if (Pager.Flags & SHOW_SELECTION)
    {
      for (i = 0; new_w != old_w && i < 4; i++)
	XReparentWindow (dpy, Pager.SelectionWin[i], new_w, 0, 0);

      old_w = new_w;

      CalcViewPortXY (&x, &y);

      XMoveResizeWindow (dpy, Pager.SelectionWin[0], x - 2, y - 2, scr_w + 4, 3);
      XMoveResizeWindow (dpy, Pager.SelectionWin[1], x - 2, y + scr_h - 1, scr_w + 4, 3);
      XMoveResizeWindow (dpy, Pager.SelectionWin[2], x - 2, y + 1, 3, scr_h - 2);
      XMoveResizeWindow (dpy, Pager.SelectionWin[3], x + scr_w - 1, y + 1, 3, scr_h - 2);

      LowerFrame (Desk);
    }
}

void
LowerFrame ()
{
  int i;
  if (Pager.Flags & SHOW_SELECTION)
    for (i = 0; i < 4; i++)
      XLowerWindow (dpy, Pager.SelectionWin[i]);
}

void
HideFrame ()
{
  int i;
  if (Pager.Flags & SHOW_SELECTION)
    for (i = 0; i < 4; i++)
      XMoveWindow (dpy, Pager.SelectionWin[i], -1000, -1000);

}
#ifdef SHAPE
/* Update window shaping when using HideInactiveLabels */
/* the following code needs some consideration */
void
UpdateWindowShape ()
{
  int i, j, desk_cnt;
  XRectangle *shape = 0;
  int desk_pos_x = -1, desk_pos_y = -1, vis_desk_w, vis_desk_h;
  int squares_num = Pager.ndesks + 1;

  if (!(Pager.Flags & HIDE_INACTIVE_LABEL) || !(Pager.Flags & USE_LABEL) || label_h <= 0)
    return;
  shape = safemalloc (squares_num * sizeof (XRectangle));

  if (Scr.CurrentDesk > Pager.desk2 || Scr.CurrentDesk < Pager.desk1)
    squares_num = Pager.ndesks;

  desk_cnt = 0;
  vis_desk_w = desk_w + Look.DeskBorderWidth * 2;
  vis_desk_h = desk_h + Look.DeskBorderWidth * 3;
  desk_pos_y = ((Pager.Flags & LABEL_BELOW_DESK) ? 0 : label_h);
  for (i = 0; i < Pager.Rows; i++)
    {
      desk_pos_x = 0;
      for (j = 0; j < Pager.Columns; j++)
	{
	  if (desk_cnt < Pager.ndesks)
	    {
	      shape[desk_cnt].x = desk_pos_x;
	      shape[desk_cnt].y = desk_pos_y;
	      shape[desk_cnt].width = vis_desk_w;
	      shape[desk_cnt].height = vis_desk_h;

	      if (desk_cnt == Scr.CurrentDesk - Pager.desk1)
		{
		  shape[Pager.ndesks].x = desk_pos_x;
		  shape[Pager.ndesks].y = desk_pos_y + label_y - ((Pager.Flags & LABEL_BELOW_DESK) ? 0 : label_h + Look.DeskBorderWidth * 2);
		  shape[Pager.ndesks].width = vis_desk_w;
		  shape[Pager.ndesks].height = label_h + Look.DeskBorderWidth * 2;
		}
	    }
	  desk_pos_x += vis_desk_w;
	  desk_cnt++;
	}
      desk_pos_y += vis_desk_h + label_h;
    }
  XShapeCombineRectangles (dpy, Pager.Pager_w, ShapeBounding, 0, 0,
			   shape, squares_num, ShapeSet, 0);
  free (shape);
}
#endif

/****************************************************************************
 *
 * Respond to a change in window geometry.
 *
 ****************************************************************************/
void
ReConfigure (void)
{
  Window root;
  unsigned width, height, bord_width, depth;
  int dum;

  XGetGeometry (dpy, Pager.Pager_w, &root, &dum, &dum,
		(unsigned *) &width, (unsigned *) &height,
		&bord_width, &depth);

  ReConfigureEx (width, height);
}

void
ReConfigureEx (unsigned width, unsigned height)
{
  int i = 0, j, k, desk_pos_x = -1, desk_pos_y = 0;
  PagerWindow *t;

  if (!Pager.bStarted)
    return;

  if (width == window_w && height == window_h)
    {
#ifdef PAGER_BACKGROUND
      UpdateTransparency ();
#endif
      return;			/* nothing to be done */
    }

  LOG3 ("width = %d, height = %d", window_w, window_h)

    window_w = width;
  window_h = height;

  CalcDeskSize ();

  scr_w = (desk_w - Pager.PageColumns + 1) / Pager.PageColumns;
  scr_h = (desk_h - Pager.PageRows + 1) / Pager.PageRows;

  Pager.AspectX = Pager.xSize / (desk_w - Pager.PageColumns + 1);
  Pager.AspectY = Pager.ySize / (desk_h - Pager.PageRows + 1);

  for (k = 0; k < Pager.Rows; k++)
    {
      desk_pos_x = 0;
      for (j = 0; j < Pager.Columns; j++)
	{
	  if (i < Pager.ndesks)
	    {
	      XMoveResizeWindow (dpy, Pager.Desks[i].title_w,
				 desk_pos_x, desk_pos_y, desk_w, desk_h + label_h + Look.DeskBorderWidth);
	      XMoveResizeWindow (dpy, Pager.Desks[i].w,
				 0 - Look.DeskBorderWidth, ((Pager.Flags & LABEL_BELOW_DESK) ? (0 - Look.DeskBorderWidth) : (label_h - Look.DeskBorderWidth)), desk_w, desk_h + Look.DeskBorderWidth);
	      /* do we really need to do this here ??? */
	      if (i == Scr.CurrentDesk - Pager.desk1)
		DisplayFrame (i);
	    }
	  desk_pos_x += desk_w + Look.DeskBorderWidth * 2;
	  i++;
	}
      desk_pos_y += desk_h + label_h + Look.DeskBorderWidth * 3;
    }
  if (Scr.CurrentDesk < Pager.desk1 && Scr.CurrentDesk > Pager.desk2)
    HideFrame (i);

  DecoratePager ();

#ifdef SHAPE
  /* Update window shapes. */
  UpdateWindowShape ();
#endif

  /* reconfigure all the subordinate windows */
  for (t = Pager.Start; t != NULL; t = t->next)
    MoveResizePagerView (t);
}

void
GetViewPosition (PagerWindow * t, PagerViewPosition * pos)
{
  int abs_x, abs_y, n, m, n1, m1;

  n = Pager.PageColumns - 1;	/* number of grid lines */
  m = Pager.PageRows - 1;

  abs_x = Scr.Vx + t->x;	/* absolute coordinate within the desk */
  abs_y = Scr.Vy + t->y;

  n1 = abs_x / Scr.MyDisplayWidth;	/* number of pages before */
  m1 = abs_y / Scr.MyDisplayHeight;

  pos->normal_x = SCREEN2PAGER_X (abs_x) + n1;
  pos->normal_y = SCREEN2PAGER_Y (abs_y) + m1;

  pos->normal_width = SCREEN2PAGER_X (t->width);
  pos->normal_height = SCREEN2PAGER_Y (t->height);

  /* cruel hack to help WavePoet :) */
  if (pos->normal_width + pos->normal_x >= desk_w &&
      abs_x + t->width < Scr.Vx + Scr.MyDisplayWidth)
    pos->normal_width--;

  if (pos->normal_width < 1)
    pos->normal_width = 1;

  if (pos->normal_height + pos->normal_y >= desk_h &&
      abs_y + t->height < Scr.Vx + Scr.MyDisplayHeight)
    pos->normal_height--;

  if (pos->normal_height < 1)
    pos->normal_height = 1;


  pos->icon_x = abs_x * (icon_w - n) / Pager.xSize;
  pos->icon_y = abs_y * (icon_h - m) / Pager.ySize;
  pos->icon_width = (abs_x + t->width + 2) * (icon_w - n) / Pager.xSize - 2 - pos->icon_x;
  pos->icon_height = (abs_y + t->height + 2) * (icon_h - m) / Pager.ySize - 2 - pos->icon_y;

  pos->icon_x += n1;
  pos->icon_y += m1;

  if (pos->icon_width < 1)
    pos->icon_width = 1;
  if (pos->icon_height < 1)
    pos->icon_height = 1;

}

/***************************************************************************/
/*****************  Desk decoration functions ******************************/
/***************************************************************************/

/****************************************************************************
 *
 * Draw grid lines for desk #i
 *
 ****************************************************************************/
void
DrawIconGrid (int erase)
{
  int y, x, w, h;
  int i;

  if (!Look.GridGC)
    return;
  if (erase)
    XClearWindow (dpy, icon_win);

  if (Pager.Flags & PAGE_SEPARATOR)
    {
      for (i = 1; i < Pager.PageColumns; i++)
	{
	  x = i * icon_w / Pager.PageColumns;
	  XDrawLine (dpy, icon_win, Look.GridGC, x, 0, x, icon_h);
	}
      for (i = 1; i < Pager.PageRows; i++)
	{
	  y = i * icon_h / Pager.PageRows;
	  XDrawLine (dpy, icon_win, Look.GridGC, 0, y, icon_w, y);
	}
    }

  w = icon_w - Pager.PageColumns - 1;	/* icon desk size - grid lines num */
  h = icon_h - Pager.PageRows - 1;

  x = w * Scr.Vx / Pager.xSize + Scr.Vx / Scr.MyDisplayWidth;
  y = h * Scr.Vy / Pager.ySize + Scr.Vy / Scr.MyDisplayHeight;

  XFillRectangle (dpy, icon_win, Look.GridGC, x, y, w / Pager.PageColumns, h / Pager.PageRows);
}


void
DrawGrid (int i /*Desk */ )
{
  int pos, d;

  if ((i < 0) || (i >= Pager.ndesks))
    return;
  if (Pager.Flags & PAGE_SEPARATOR && Look.GridGC)
    {
      for (d = 1; d < Pager.PageColumns; d++)
	{
	  pos = d * desk_w / Pager.PageColumns;
	  XDrawLine (dpy, Pager.Desks[i].w, Look.GridGC, pos, 0, pos, desk_h);
	}
      for (d = 1; d < Pager.PageRows; d++)
	{
	  pos = d * desk_h / Pager.PageRows;
	  XDrawLine (dpy, Pager.Desks[i].w, Look.GridGC, 0, pos, desk_w, pos);
	}
    }
}

Pixmap
GetMyStylePixmap (Window w, MyStyle * style, unsigned int width, unsigned int height)
{
  Pixmap p = None;
#ifndef  NO_TEXTURE
  int real_x, real_y;

  if (style)
    {
      if (style->texture_type > 0 && style->texture_type <= TEXTURE_PIXMAP)
	p = mystyle_make_pixmap (style, width, height, None);
      else if (style->texture_type > TEXTURE_PIXMAP)
	if (GetWinPosition (w, &real_x, &real_y))
	  p = mystyle_make_pixmap_overlay (style,
					   real_x, real_y,
					   width, height, None);
    }
#endif
  return p;
}

void
LabelDesk (int i /*Desk */ )
{
  if ((i < 0) || (i >= Pager.ndesks))
    return;
  if ((Pager.Flags & USE_LABEL) && label_h > 0)
    {
      MyStyle *style;
      int hor_off, w;
      char str[16], *ptr = Pager.Desks[i].label;

      style = (Pager.desk1 + i == Scr.CurrentDesk) ? Look.Styles[STYLE_ADESK] : Look.Styles[STYLE_INADESK];
      if (!(style->set_flags & F_FONT) || ptr == NULL)
	return;

      w = XTextWidth (style->font.font, ptr, strlen (ptr));
      if (w > desk_w)
	{
	  sprintf (str, "%d", Pager.desk1 + i);
	  ptr = str;
	  w = XTextWidth (style->font.font, ptr, strlen (ptr));
	}

      if (w <= desk_w)
	{
	  if (Look.TitleAlign > 0)
	    hor_off = Look.TitleAlign;
	  else if (Look.TitleAlign == 0)
	    hor_off = (desk_w - w) / 2;
	  else
	    hor_off = desk_w - w + Look.TitleAlign;

	  mystyle_draw_text (Pager.Desks[i].title_w, style, ptr,
			     hor_off, label_y + (label_h - style->font.height) / 2 + style->font.y);
	}
#ifdef SHAPE
      /* Update window shapes. */
      UpdateWindowShape ();
#endif
    }
}

void
HilightDesk (int i /*Desk */ , int if_texture)
{
  MyStyle *style;

  if ((i < 0) || (i >= Pager.ndesks))
    return;

  style = (Pager.desk1 + i == Scr.CurrentDesk) ? Look.Styles[STYLE_ADESK] : Look.Styles[STYLE_INADESK];

  if (if_texture >= 0 && style->texture_type < if_texture)
    return;

  if ((Pager.Flags & USE_LABEL) && label_h > 0)
    {
      XSetWindowAttributes attr;
      unsigned long valuemask;

      /* let's setup window border/background at this point */
      attr.background_pixmap = GetMyStylePixmap (Pager.Desks[i].title_w, style, desk_w, label_h);
      attr.background_pixel = style->colors.back;
      if (Pager.Flags & DIFFERENT_BORDER_COLOR)
	attr.border_pixel = Look.BorderColor;
      else
	attr.border_pixel = style->colors.fore;

      if (attr.background_pixmap)
	valuemask = (CWBackPixmap | CWBorderPixel);
      else
	valuemask = (CWBackPixel | CWBorderPixel);
      XChangeWindowAttributes (dpy, Pager.Desks[i].title_w, valuemask, &attr);
      XClearWindow (dpy, Pager.Desks[i].title_w);
      if (attr.background_pixmap)
	XFreePixmap (dpy, attr.background_pixmap);

      LabelDesk (i);

    }
  else				/* no label */
    XSetWindowBorder (dpy, Pager.Desks[i].title_w, style->colors.fore);

}

/* this function will set up colors, and pixmaps of all desks'frames
   and Pager's frame
 */
void
DecorateDesk (long Desk)
{
  XSetWindowAttributes attr;
  unsigned long mask;
#ifdef PAGER_BACKGROUND
  unsigned int height = desk_h + Look.DeskBorderWidth;
#endif

  attr.border_pixel = Look.GridColor;
/*       attr.background_pixel = (Look.Styles[STYLE_INADESK]->colors).back ; */
  attr.background_pixmap = None;
#ifdef PAGER_BACKGROUND
  if (Look.DeskStyles[Desk] == NULL)
    {				/* we scale root background by default */
      unsigned int dum;
      int dummy;
      Window root;
      unsigned int src_width = 0, src_height = 0;
      ASDeskBack *back = FindDeskBack (&(Pager.Backgrounds), Desk + Pager.desk1);
      if (back)
	if (back->data.pixmap)
	  if (XGetGeometry (dpy, back->data.pixmap, &root, &dummy, &dummy,
			    &src_width, &src_height, &dum, &dum) != 0)
	    attr.background_pixmap = scale_pixmap (Scr.asv, back->data.pixmap,
						  src_width, src_height,
						  desk_w, height,
						  DefaultGC (dpy, screen), TINT_LEAVE_SAME );
    }
  else
    attr.background_pixmap = GetMyStylePixmap (Pager.Desks[Desk].w,
					       Look.DeskStyles[Desk],
					       desk_w, height);
#endif
  if (attr.background_pixmap == None)
    {
      if (Look.DeskStyles[Desk])
	attr.background_pixel = Look.DeskStyles[Desk]->colors.back;
      else
	attr.background_pixel = 0;
      mask = CWBorderPixel | CWBackPixel;
    }
  else
    mask = CWBorderPixel | CWBackPixmap;

  XChangeWindowAttributes (dpy, Pager.Desks[Desk].w, mask, &attr);
  XClearWindow (dpy, Pager.Desks[Desk].w);

  if (attr.background_pixmap != None)
    XFreePixmap (dpy, attr.background_pixmap);
}
#ifdef PAGER_BACKGROUND
void
UpdateTransparency ()
{
  long Desk = 0;
  if( Scr.RootImage )
    destroy_asimage( &(Scr.RootImage));

  for (; Desk < Pager.ndesks; Desk++)
    {
      if (Look.DeskStyles[Desk])
	if (Look.DeskStyles[Desk]->texture_type >= TEXTURE_TRANSPARENT)
	  {
	    DecorateDesk (Desk);
	    DrawGrid (Desk);
	  }
      if (label_h > 0)
	HilightDesk (Desk, TEXTURE_TRANSPARENT);
    }
	if( Scr.RootImage )
	    destroy_asimage( &(Scr.RootImage));
	
}
#endif
void
DecoratePager ()
{
  XSetWindowAttributes attr;
  int i;

  /* set Pager's border color to GridColor and back pix to inactive back color */
  attr.border_pixel = Look.GridColor;
  attr.background_pixel = (Look.Styles[STYLE_INADESK]->colors).back;
  XChangeWindowAttributes (dpy, Pager.Pager_w, CWBorderPixel | CWBackPixel, &attr);
  XClearWindow (dpy, Pager.Pager_w);

  /* let's set up Selection's color at this point */
  attr.border_pixel = attr.background_pixel = Look.SelectionColor;
  for (i = 0; i < 4; i++)
    {
      XChangeWindowAttributes (dpy, Pager.SelectionWin[i], CWBorderPixel | CWBackPixel, &attr);
      XClearWindow (dpy, Pager.SelectionWin[i]);
    }
  /* now let's set up every desk */
  for (i = 0; i < Pager.ndesks; i++)
    {
      XSetWindowBorder (dpy, Pager.Desks[i].title_w, Look.DeskBorderWidth);
      /* that should set every desk's title window's
         border color, pixmap or back color,  */
      HilightDesk (i, -1);

      /* lets set desk's background at this point - so DrawGrid will
         correctly draw lines above it */
      /* to repaint grid over changed desk's background */
      DrawGrid (i);
    }
}

/***************************************************************************/
/***************  Window management functions ******************************/
/***************************************************************************/

PagerWinType
GetWinType (PagerWindow * t)
{
  if (t)
    {
      if (t == Pager.FocusWin)
	return WIN_FOCUSED;
      if (t->flags & STICKY)
	return WIN_STICKY;
    }
  return WIN_UNFOCUSED;
}

int
GetWinAttributes (PagerWindow * t, XSetWindowAttributes * attributes, unsigned long *valuemask, PagerViewPosition * ppos)
{
  PagerWinType type = GetWinType (t);
  PagerViewPosition pos;

  if (!t)
    return 0;

  if (!ppos)
    ppos = &pos;
  GetViewPosition (t, ppos);

  if (Look.Styles[type]->texture_type > 0 && Look.Styles[type]->texture_type <= 128)
    attributes->background_pixmap = mystyle_make_pixmap (Look.Styles[type],
							 ppos->normal_width,
						 ppos->normal_height, None);
  else
    attributes->background_pixmap = None;
  LOG3 ("\nType %d, Back pixmap %ld", type, attributes->background_pixmap);

  attributes->background_pixel = Look.Styles[type]->colors.back;
  attributes->border_pixel = Look.Styles[type]->colors.fore;

  if (attributes->background_pixmap)
    *valuemask = (CWBackPixmap | CWBorderPixel);
  else
    *valuemask = (CWBackPixel | CWBorderPixel);

  return 1;
}

void
PutLabel (PagerWindow * t, Window w)
{
  if (!t)
    return;
  LOG2 ("\nPutLabel(%s)... ", t->icon_name)
    if (w != None && t->icon_name != NULL)
    {
      PagerWinType type = GetWinType (t);
      GC gc = Look.WinForeGC[type];
      int y;

      if (gc != None && Look.windowFont.font != NULL)
	y = Look.windowFont.y + 2;
      else
	y = (Look.Styles[type]->font).y + 2;

      XClearWindow (dpy, w);

      if (gc == None || Look.windowFont.font == NULL)
	mystyle_draw_text (w, Look.Styles[type], t->icon_name, 2, y);
      else
#undef FONTSET
#define FONTSET Look.windowFont.fontset
	XDrawImageString (dpy, w, gc, 2, y, t->icon_name, strlen (t->icon_name));
    }

  LOG1 (" Done.")
}

void
Hilight (PagerWindow * t)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;

  if (t == NULL)
    return;

  LOG2 ("\nHilight(%s)... ", t->icon_name)
    if (GetWinAttributes (t, &attributes, &valuemask, NULL))
    {
      if (t->PagerView != None)
	{
	  XChangeWindowAttributes (dpy, t->PagerView, valuemask, &attributes);
	  LabelWindow (t);	/* that should clear window for us */
	}
      if (t->IconView != None)
	{
	  valuemask = (valuemask & (~CWBackPixmap)) | CWBackPixel;
	  XChangeWindowAttributes (dpy, t->IconView, valuemask, &attributes);
	  LabelIconWindow (t);	/* that should clear window for us */
	}
      if (attributes.background_pixmap)
	XFreePixmap (dpy, attributes.background_pixmap);
    }
}

void
AddNewWindow (PagerWindow * t)
{
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  PagerViewPosition pos;
  int i;			/* Desk */

  if (t == NULL)
    return;
  if (t->desk < Pager.desk1 || t->desk > Pager.desk2)
    {
      t->IconView = None;
      t->PagerView = None;
      return;
    }

  i = t->desk - Pager.desk1;
  LOG2 ("\nAddNewWindow(%s)... ", t->icon_name)
    GetWinAttributes (t, &attributes, &valuemask, &pos);
  attributes.event_mask = (ExposureMask | EnterWindowMask | LeaveWindowMask);
  valuemask |= CWEventMask;

  /* Enter and Leave events to pop up balloon window */
  if ((t->PagerView = XCreateWindow (dpy, Pager.Desks[i].w,
	 pos.normal_x, pos.normal_y, pos.normal_width, pos.normal_height, 1,
				     CopyFromParent,
				     InputOutput, CopyFromParent,
				     valuemask, &attributes)) != None)
    {

      balloon_new_with_text (dpy, t->PagerView, t->icon_name);
      XMapRaised (dpy, t->PagerView);
      LabelWindow (t);

      if (Scr.CurrentDesk != t->desk)
	{
	  pos.icon_x = -1000;	/* showing only windows on active desk if iconized */
	  pos.icon_y = -1000;
	}
      /* don't want pixmaps in iconized Pager */
      valuemask = (valuemask & (~CWBackPixmap)) | CWBackPixel;
      if ((t->IconView = XCreateWindow (dpy, icon_win, pos.icon_x, pos.icon_y,
					pos.icon_width, pos.icon_height, 1,
					CopyFromParent,
					InputOutput, CopyFromParent,
					valuemask, &attributes)) != None)
	{
	  if (Scr.CurrentDesk == t->desk)
	    XGrabButton (dpy, 2, AnyModifier, t->IconView,
	       True, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
			 GrabModeAsync, GrabModeAsync, None,
			 None);
	  balloon_new_with_text (dpy, t->IconView, t->icon_name);
	  XMapRaised (dpy, t->IconView);
	  LabelIconWindow (t);

	}

      if (attributes.background_pixmap)
	XFreePixmap (dpy, attributes.background_pixmap);
    }				/* PagerView != None */
}

void
DestroyIconView (PagerWindow * t)
{
  if (t != NULL)
    if (t->IconView != None)
      {
	balloon_delete (balloon_find (t->IconView));
	XDestroyWindow (dpy, t->IconView);
	t->IconView = None;
      }
}

void
DestroyView (PagerWindow * t)
{
  if (t != NULL)
    if (t->PagerView != None)
      {
	balloon_delete (balloon_find (t->PagerView));
	XDestroyWindow (dpy, t->PagerView);
	t->PagerView = None;
      }
  DestroyIconView (t);
}

/***************************************************************************/
/************* Event handlers and moving stuff *****************************/
/***************************************************************************/

void
ReConfigureIcons (void)
{
  PagerWindow *t;
  PagerViewPosition pos;

  for (t = Pager.Start; t != NULL; t = t->next)
    if (t->IconView != None)
      {
	if (Scr.CurrentDesk == t->desk)
	  {
	    GetViewPosition (t, &pos);
	    XMoveResizeWindow (dpy, t->IconView,
		   pos.icon_x, pos.icon_y, pos.icon_width, pos.icon_height);
	  }
	else
	  XMoveWindow (dpy, t->IconView, -1000, -1000);
      }
}

void
MovePage (void)
{
  XTextProperty name;
  char str[64], *sptr;
  static int icon_desk_shown = -1000;

  LOG2 ("\n%s:MovePage():\t", MyName)
    LOG1 ("\n\t\t\t\t DisplayFrame")
    if (Scr.CurrentDesk < Pager.desk1 || Scr.CurrentDesk > Pager.desk2)
    HideFrame ();
  else
    DisplayFrame (Scr.CurrentDesk - Pager.desk1);
  LOG1 ("\n\t\t\t\t DrawIconGrid (1);")
    DrawIconGrid (1);
  LOG1 ("\n\t\t\t\t ReConfigureIcons ();")
    ReConfigureIcons ();
  if (Scr.CurrentDesk != icon_desk_shown)
    {
      icon_desk_shown = Scr.CurrentDesk;

      if ((Scr.CurrentDesk >= Pager.desk1) && (Scr.CurrentDesk <= Pager.desk2))
	sptr = Pager.Desks[Scr.CurrentDesk - Pager.desk1].label;
      else
	{
	  sprintf (str, "Desk %d", Scr.CurrentDesk);
	  sptr = &str[0];
	}
      if (XStringListToTextProperty (&sptr, 1, &name) == 0)
	{
	  fprintf (stderr, "%s: cannot allocate window name", MyName);
	  return;
	}

      XSetWMIconName (dpy, Pager.Pager_w, &name);
      XFree (name.value);
    }
  LOG2 ("\n%s:MovePage():\tDone!", MyName)
}

void
ChangeDeskForWindow (PagerWindow * t, long newdesk)
{
  PagerViewPosition pos;

  if (t == NULL || newdesk == 10000)
    return;
  if (newdesk < Pager.desk1 || newdesk > Pager.desk2)
    {
      DestroyView (t);
      return;
    }

  LOG3 ("\n Changing Window [%ld] Desk to desk : %ld", t->w, (newdesk - Pager.desk1))
    if (t->PagerView == None || t->IconView == None)
    {
      t->desk = newdesk;
      DestroyView (t);
      AddNewWindow (t);
      LOG2 ("\n  adding new window to desk : %ld", (newdesk - Pager.desk1))
	return;
    }

  GetViewPosition (t, &pos);
  XReparentWindow (dpy, t->PagerView, Pager.Desks[newdesk - Pager.desk1].w,
		   pos.normal_x, pos.normal_y);
  XResizeWindow (dpy, t->PagerView, pos.normal_width, pos.normal_height);

  t->desk = newdesk;
  if (Scr.CurrentDesk == newdesk)
    XMoveResizeWindow (dpy, t->IconView, pos.icon_x, pos.icon_y,
		       pos.icon_width, pos.icon_height);
  else
    XMoveWindow (dpy, t->IconView, -1000, -1000);
}

void
MoveResizePagerView (PagerWindow * t)
{
  PagerViewPosition pos;

  if (t == NULL)
    return;
  if (t->desk < Pager.desk1 || t->desk > Pager.desk2)
    return;

  if (t->PagerView == None || t->IconView == None)
    {
      DestroyView (t);
      AddNewWindow (t);
      return;
    }

  GetViewPosition (t, &pos);

  LOG3 ("\n Resizing window [%ld:%s] to:\n", t->w, t->icon_name)
    LOG3 ("x=%d, y=%d, ", pos.normal_x, pos.normal_x)
    LOG3 ("width=%d, height=%d.\n", pos.normal_width, pos.normal_height)
    LOG3 ("Original width=%d, height=%d.", t->width, t->height)

    XMoveResizeWindow (dpy, t->PagerView, pos.normal_x, pos.normal_y,
		       pos.normal_width, pos.normal_height);

  if (Scr.CurrentDesk == t->desk)
    XMoveResizeWindow (dpy, t->IconView, pos.icon_x, pos.icon_y,
		       pos.icon_width, pos.icon_height);
  else
    XMoveWindow (dpy, t->IconView, -1000, -1000);

  LOG1 ("... Done!")
}

void
RestackWindows( int desk )
{
  register int count = 0;
  register PagerWindow *t;

    for (count = 0, t = Pager.Start; t != NULL; t = t->next)
        if (t->desk == desk) count++ ;

      /* restoring stacking order of the windows */
    if (count > 0)
    {
      Window *windows = (Window *) safemalloc (sizeof (Window) * count);
	for (count = 0, t = Pager.Start; t != NULL; t = t->next)
	    if ( t->desk == desk )
	    {
    		windows[count++] = t->PagerView;
	    }
	XRestackWindows (dpy, windows, count);
	if( desk == Scr.CurrentDesk )
	{
	    for (count = 0, t = Pager.Start; t != NULL; t = t->next)
		if ( t->desk == desk && t->IconView != None )
		{
    		    windows[count++] = t->IconView;
		}
	    XRestackWindows (dpy, windows, count);
	}
	free (windows);
    }
}

void
RestackAllWindows()
{
  register int desk = Pager.desk1;
    while( desk < Pager.desk2 )
	RestackWindows( desk++ );
}


void
MoveStickyWindows (void)
{
  PagerWindow *t;
  int count = 0;

  if (Scr.CurrentDesk != 10000)
    {
      for (t = Pager.Start; t != NULL; t = t->next, count++)
	{
	  if (t->flags & STICKY || ((t->flags & ICONIFIED) && (Pager.Flags & STICKY_ICONS)))
	    {
	      LOG3 ("\n Moving sticky window [%ld:%s]: ", t->w, t->icon_name)
		if (t->desk != Scr.CurrentDesk)
		{
		  LOG2 (" Changing Desk to [%d]", Scr.CurrentDesk)
		    ChangeDeskForWindow (t, Scr.CurrentDesk);
		}
	      else
		{
		  LOG1 (" Resizing Window to a new page")
		    MoveResizePagerView (t);
		}
	    }
	  LOG2 ("\n window %s moved.", t->icon_name);
	}
      /* restoring stacking order of the windows */
      RestackWindows( Scr.CurrentDesk );
    }
}
