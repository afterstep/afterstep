/****************************************************************************
 * Copyright (c) 1999 Sasha Vasko <sashav@sprintmail.com>
 * This module is based on Twm, but has been SIGNIFICANTLY modified 
 * by Rob Nation
 * by Bo Yang
 * by Frank Fejes
 ****************************************************************************/

/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/
/***********************************************************************
 *
 * afterstep menu code
 *
 ***********************************************************************/

#include "../../configure.h"

#include <limits.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "globals.h"
#include "menus.h"

extern int ShadeAnimationSteps;
extern XEvent Event;
extern ASWindow *Tmp_win;
extern int menuFromFrameOrWindowOrTitlebar;
extern int Xzap, Yzap;
extern int AutoReverse;
extern int AutoTabThroughDesks;
extern int DoHandlePageing;

extern void InitVariables (int);

/* the following variables are used for AutoReverse mode 2 */
long LastWarpIndex = -1;
long LastWarpedWindow = 0;

extern ASDirs as_dirs;

/***********************************************************************
 *
 *  Procedure:
 *	ExecuteFunction - execute a afterstep built in function
 *
 *  Inputs:
 *	func	- the function to execute
 *	action	- the menu action to execute 
 *	w	- the window to execute this function on
 *	tmp_win	- the afterstep window structure
 *	event	- the event that caused the function
 *	context - the context in which the button was pressed
 *      val1,val2 - the distances to move in a scroll operation 
 *
 ***********************************************************************/
void
ExecuteFunction (FunctionCode func, char *action, Window in_w, ASWindow * tmp_win,
	       XEvent * eventp, unsigned long context, long val1, long val2,
		 int val1_unit, int val2_unit, MenuRoot * menu, int Module)
{
  ASWindow *t = NULL;
  char *realfilename, tmpfile[255];
  int x, y;
  Window w;
#ifndef NO_VIRTUAL
  int delta_x, delta_y;
#endif /* NO_VIRTUAL */
  int warp_x = 0, warp_y = 0;
  extern int SmartCircCounter;
  extern int LastFunction;

  FunctionCode switch_func = func;

  /* Defer Execution may wish to alter this value */
  w = in_w;
  if (IsWindowFunc (func))
    {
      int cursor, fin_event;
      if (func != F_RESIZE && func != F_MOVE)
	{
	  fin_event = ButtonRelease;
	  if (func != F_DESTROY && func != F_DELETE && func != F_CLOSE)
	    cursor = DESTROY;
	  else
	    cursor = SELECT;
	}
      else
	{
	  cursor = MOVE;
	  fin_event = ButtonPress;
	}

      if (DeferExecution (eventp, &w, &tmp_win, &context, cursor, fin_event))
	switch_func = F_NOP;
      else if (tmp_win == NULL)
	switch_func = F_NOP;
    }

  switch (switch_func)
    {
    case F_NOP:
    case F_TITLE:
    case F_ENDPOPUP:
    case F_ENDFUNC:
      break;

    case F_BEEP:
      XBell (dpy, Scr.screen);
      break;

    case F_RESIZE:
      if (check_allowed_function2 (func, tmp_win) == 0)
	{
	  XBell (dpy, Scr.screen);
	  break;
	}
      tmp_win->flags &= ~MAXIMIZED;
      resize_window (w, tmp_win, val1, val2, val1_unit, val2_unit);
      break;

    case F_MOVE:
      move_window (eventp, w, tmp_win, context, val1, val2, val1_unit, val2_unit);
      break;

#ifndef NO_VIRTUAL
    case F_SCROLL:
      if ((val1 > -100000) && (val1 < 100000))
	x = Scr.Vx + val1 * val1_unit / 100;
      else
	x = Scr.Vx + (val1 / 1000) * val1_unit / 100;

      if ((val2 > -100000) && (val2 < 100000))
	y = Scr.Vy + val2 * val2_unit / 100;
      else
	y = Scr.Vy + (val2 / 1000) * val2_unit / 100;

      if (((val1 <= -100000) || (val1 >= 100000)) && (x > Scr.VxMax))
	{
	  x = 0;
	  y += Scr.MyDisplayHeight;
	  if (y > Scr.VyMax)
	    y = 0;
	}
      if (((val1 <= -100000) || (val1 >= 100000)) && (x < 0))
	{
	  x = Scr.VxMax;
	  y -= Scr.MyDisplayHeight;
	  if (y < 0)
	    y = Scr.VyMax;
	}
      if (((val2 <= -100000) || (val2 >= 100000)) && (y > Scr.VyMax))
	{
	  y = 0;
	  x += Scr.MyDisplayWidth;
	  if (x > Scr.VxMax)
	    x = 0;
	}
      if (((val2 <= -100000) || (val2 >= 100000)) && (y < 0))
	{
	  y = Scr.VyMax;
	  x -= Scr.MyDisplayWidth;
	  if (x < 0)
	    x = Scr.VxMax;
	}
      MoveViewport (x, y, True);
      break;
#endif
    case F_MOVECURSOR:
      XQueryPointer (dpy, Scr.Root, &JunkRoot, &JunkChild,
		     &x, &y, &JunkX, &JunkY, &JunkMask);
#ifndef NO_VIRTUAL
      delta_x = 0;
      delta_y = 0;
      warp_x = 0;
      warp_y = 0;
      if (x >= Scr.MyDisplayWidth - 2)
	{
	  delta_x = Scr.EdgeScrollX;
	  warp_x = Scr.EdgeScrollX - 4;
	}
      if (y >= Scr.MyDisplayHeight - 2)
	{
	  delta_y = Scr.EdgeScrollY;
	  warp_y = Scr.EdgeScrollY - 4;
	}
      if (x < 2)
	{
	  delta_x = -Scr.EdgeScrollX;
	  warp_x = -Scr.EdgeScrollX + 4;
	}
      if (y < 2)
	{
	  delta_y = -Scr.EdgeScrollY;
	  warp_y = -Scr.EdgeScrollY + 4;
	}
      if (Scr.Vx + delta_x < 0)
	delta_x = -Scr.Vx;
      if (Scr.Vy + delta_y < 0)
	delta_y = -Scr.Vy;
      if (Scr.Vx + delta_x > Scr.VxMax)
	delta_x = Scr.VxMax - Scr.Vx;
      if (Scr.Vy + delta_y > Scr.VyMax)
	delta_y = Scr.VyMax - Scr.Vy;
      if ((delta_x != 0) || (delta_y != 0))
	{
	  MoveViewport (Scr.Vx + delta_x, Scr.Vy + delta_y, True);
	  XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
			Scr.MyDisplayHeight,
			x - warp_x,
			y - warp_y);
	}
#endif
      XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
		    Scr.MyDisplayHeight, x + val1 * val1_unit / 100 - warp_x,
		    y + val2 * val2_unit / 100 - warp_y);

      break;
    case F_ICONIFY:
      if (tmp_win->flags & ICONIFIED)
	{
	  if (val1 <= 0)
	    DeIconify (tmp_win);
	}
      else
	{
	  if (check_allowed_function2 (func, tmp_win) == 0)
	    {
	      XBell (dpy, Scr.screen);
	      break;
	    }
	  if (val1 >= 0)
	    {
	      if (check_allowed_function2 (func, tmp_win) == 0)
		{
		  XBell (dpy, Scr.screen);
		  break;
		}
	      Iconify (tmp_win);
	    }
	}
      break;

    case F_RAISE:
      RaiseWindow (tmp_win);
      break;

    case F_PUTONTOP:
      /* put the window in its new place above ontop windows */
      tmp_win->layer = 1;
      RaiseWindow (tmp_win);
      break;

    case F_PUTONBACK:
      /* put the window in its new place below onback windows */
      tmp_win->layer = -1;
      RaiseWindow (tmp_win);
      break;

    case F_SETLAYER:
      /* put the window in its new place */
      tmp_win->layer = val1;
      RaiseWindow (tmp_win);
      break;

    case F_TOGGLELAYER:
      /* swap window layer */
      tmp_win->layer = (tmp_win->layer == val1) ? val2 : val1;
      RaiseWindow (tmp_win);
      break;

    case F_LOWER:
      LowerWindow (tmp_win);
      break;

    case F_STICK:
      Stick (tmp_win);
      break;

    case F_FOCUS:
      FocusOn (tmp_win, 0, False);
      break;

    case F_CHANGE_WINDOWS_DESK:
      /* don't move sticky windows/icons */
      if (!((tmp_win->flags & ICONIFIED) && (Scr.flags & StickyIcons)) &&
	  !(tmp_win->flags & STICKY) && !(tmp_win->flags & ICON_UNMAPPED))
	changeWindowsDesk (tmp_win, val1);
      break;

    case F_RAISELOWER:
      if (tmp_win->flags & VISIBLE)
	LowerWindow (tmp_win);
      else
	RaiseWindow (tmp_win);
      break;

    case F_MAXIMIZE:
      if (check_allowed_function2 (func, tmp_win))
	Maximize (tmp_win, val1, val2, val1_unit, val2_unit);
      else
	XBell (dpy, Scr.screen);
      break;

    case F_SHADE:
      if (check_allowed_function2 (func, tmp_win))
	Shade (tmp_win);
      else
	XBell (dpy, Scr.screen);
      break;

    case F_PASTE_SELECTION:
      PasteSelection ();
      break;

    case F_CHANGEWINDOW_UP:
    case F_CHANGEWINDOW_DOWN:
/* If + deiconify, AfterStep crashes X ! */
      t = Circulate (tmp_win, action, (func == F_CHANGEWINDOW_UP) ? UP : DOWN);
      if (t)
	FocusOn (t, 0, True);
      break;

    case F_DESTROY:
    case F_DELETE:
    case F_CLOSE:
      if (check_allowed_function2 (func, tmp_win) == 0)
	XBell (dpy, Scr.screen);
      else if ((tmp_win->flags & DoesWmDeleteWindow) && func != F_DESTROY)
	send_clientmessage (tmp_win->w, _XA_WM_DELETE_WINDOW, CurrentTime);
      else
	{
	  if (func == F_DELETE)
	    XBell (dpy, Scr.screen);
	  else if (XGetGeometry (dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
			 &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
	    Destroy (tmp_win, True);
	  else
	    XKillClient (dpy, tmp_win->w);
	  XSync (dpy, 0);
	}
      break;

    case F_RESTART:
      ClosePipes ();
      Done (1, action);
      break;

    case F_EXEC:
      XGrabPointer (dpy, Scr.Root, True,
		    ButtonPressMask | ButtonReleaseMask,
		    GrabModeAsync, GrabModeAsync,
		    Scr.Root, Scr.ASCursors[WAIT], CurrentTime);
      XSync (dpy, 0);

      if (!(fork ()))		/* child process */
	if (execl ("/bin/sh", "/bin/sh", "-c", action, (char *) 0) == -1)
	  exit (100);
      XUngrabPointer (dpy, CurrentTime);
      XSync (dpy, 0);
      break;

    case F_CHANGE_BACKGROUND:
      XGrabPointer (dpy, Scr.Root, True,
		    ButtonPressMask | ButtonReleaseMask,
		    GrabModeAsync, GrabModeAsync,
		    Scr.Root, Scr.ASCursors[WAIT], CurrentTime);
      XSync (dpy, 0);

      sprintf (tmpfile, BACK_FILE, Scr.CurrentDesk);
      realfilename = make_file_name (as_dirs.after_dir, tmpfile);
      if (CopyFile (action, realfilename) == 0)
	Broadcast (M_NEW_BACKGROUND, 1, 1);

      free (realfilename);
      XUngrabPointer (dpy, CurrentTime);
      XSync (dpy, 0);
      break;

    case F_CHANGE_LOOK:
#ifndef DIFFERENTLOOKNFEELFOREACHDESKTOP
      sprintf (tmpfile, LOOK_FILE, 0, Scr.d_depth);
#else /* DIFFERENTLOOKNFEELFOREACHDESKTOP */
      sprintf (tmpfile, LOOK_FILE, Scr.CurrentDesk, Scr.d_depth);
#endif /* DIFFERENTLOOKNFEELFOREACHDESKTOP */

      realfilename = make_file_name (as_dirs.after_dir, tmpfile);
      if (CopyFile (action, realfilename) == 0)
	QuickRestart ("look");

      free (realfilename);
      break;

    case F_CHANGE_FEEL:
#ifndef DIFFERENTLOOKNFEELFOREACHDESKTOP
      sprintf (tmpfile, FEEL_FILE, 0, Scr.d_depth);
#else /* DIFFERENTLOOKNFEELFOREACHDESKTOP */
      sprintf (tmpfile, FEEL_FILE, Scr.CurrentDesk, Scr.d_depth);
#endif /* DIFFERENTLOOKNFEELFOREACHDESKTOP */
      realfilename = make_file_name (as_dirs.after_dir, tmpfile);
      if (CopyFile (action, realfilename) == 0)
	QuickRestart ("feel");

      free (realfilename);
      break;

    case F_REFRESH:
      {
	XSetWindowAttributes attributes;
	unsigned long valuemask;

	valuemask = (CWBackPixmap);
	attributes.background_pixmap = ParentRelative;
	attributes.backing_store = NotUseful;

	w = XCreateWindow (dpy, Scr.Root, 0, 0,
			   (unsigned int) Scr.MyDisplayWidth,
			   (unsigned int) Scr.MyDisplayHeight,
			   (unsigned int) 0,
			   CopyFromParent, (unsigned int) CopyFromParent,
			   (Visual *) CopyFromParent, valuemask,
			   &attributes);
	XMapWindow (dpy, w);
	XDestroyWindow (dpy, w);
	XFlush (dpy);
      }
      break;

#ifndef NO_VIRTUAL
    case F_GOTO_PAGE:
      /* back up 1 virtual desktop page */
      x = val1 * Scr.MyDisplayWidth;
      y = val2 * Scr.MyDisplayHeight;
      MoveViewport (x, y, True);
      break;
#endif
#ifndef NO_VIRTUAL
    case F_TOGGLE_PAGE:
      DoHandlePageing = (DoHandlePageing) ? 0 : 1;
      Broadcast (M_TOGGLE_PAGING, 1, DoHandlePageing);
      checkPanFrames ();
      break;
#endif

    case F_GETHELP:
      XGrabPointer (dpy, Scr.Root, True,
		    ButtonPressMask | ButtonReleaseMask,
		    GrabModeAsync, GrabModeAsync,
		    Scr.Root, Scr.ASCursors[WAIT], CurrentTime);
      XSync (dpy, 0);

      if (tmp_win != NULL)
	if (tmp_win->class.res_name != NULL)
	  {
	    realfilename = safemalloc (strlen (HELPCOMMAND) + 1 + strlen (tmp_win->class.res_name) + 1);
	    sprintf (realfilename, "%s %s", HELPCOMMAND, tmp_win->class.res_name);
	    if (!(fork ()))	/* child process */
	      if (execl ("/bin/sh", "/bin/sh", "-c", realfilename, (char *) 0) == -1)
		exit (100);
	    free (realfilename);
	  }

      XUngrabPointer (dpy, CurrentTime);
      XSync (dpy, 0);
      break;

    case F_WARP_F:
    case F_WARP_B:
      if (AutoReverse == 2)
	{
	  if ((t = GetNextWindow (tmp_win, func)) != NULL)
	    {
	      LastWarpedWindow = t->warp_index;
	      FocusOn (t, 0, False);
	    }
	}
      else
	{
	  if (AutoReverse == 1)
	    {
	      if ((LastFunction != F_WARP_F) || (LastFunction != F_WARP_B))
		SmartCircCounter = abs (SmartCircCounter - 1);
	      t = Circulate (tmp_win, action,
		   (SmartCircCounter == 0 && func == F_WARP_F) ? UP : DOWN);
	    }
	  else
	    t = Circulate (tmp_win, action, (func == F_WARP_F) ? UP : DOWN);
	  if (t)
	    FocusOn (t, 0, True);
	}
      break;

    case F_WAIT:
      {
	Bool done = False;
	if (val1 == 0)
	  val1 = 1;
	while (!done)
	  {
	    if (My_XNextEvent (dpy, &Event))
	      {
		DispatchEvent ();
		if (Event.type == MapNotify)
		  {
		    if ((Tmp_win) && (matchWildcards (action, Tmp_win->name) == True))
		      done = True;
		    if ((Tmp_win) && (Tmp_win->class.res_class) &&
			(matchWildcards (action, Tmp_win->class.res_class) == True))
		      done = True;
		    if ((Tmp_win) && (Tmp_win->class.res_name) &&
			(matchWildcards (action, Tmp_win->class.res_name) == True))
		      done = True;
		  }
	      }
	  }
      }
      XSync (dpy, 0);
      break;

    case F_RAISE_IT:
      if (val1 != 0)
	{
	  FocusOn ((ASWindow *) val1, 0, False);
	  if (((ASWindow *) (val1))->flags & ICONIFIED)
	    {
	      DeIconify ((ASWindow *) val1);
	      FocusOn ((ASWindow *) val1, 0, False);
	    }
	}
      break;

    case F_DESK:
      changeDesks (val1, val2);
      break;

    case F_MODULE:
      if (eventp->type != KeyPress)
	UngrabEm ();
      executeModule (action, (FILE *) NULL, (char **) ((tmp_win) ? tmp_win->w : 0), (int *) context);
      /* If we execute a module, don't wait for buttons to come up,
       * that way, a pop-up menu could be implemented */
      Module = 0;
      break;

    case F_KILLMODULEBYNAME:
      KillModuleByName (action);
      break;

    case F_POPUP:
      menuFromFrameOrWindowOrTitlebar = FALSE;
      Module = 0;
      do_menu (menu, NULL);
      break;

    case F_QUIT:
      Done (0, NULL);
      break;

#ifndef NO_WINDOWLIST
    case F_WINDOWLIST:
      Module = 0;
      do_windowList (val1, val2);
      break;
#endif /* ! NO_WINDOWLIST */

    case F_FUNCTION:
      ComplexFunction (w, tmp_win, eventp, context, menu);
      break;

    case F_QUICKRESTART:
      QuickRestart (action);
      break;

    case F_SEND_WINDOW_LIST:
      if (Module >= 0)
	{
	  SendPacket (Module, M_TOGGLE_PAGING, 1, DoHandlePageing);
	  SendPacket (Module, M_NEW_DESK, 1, Scr.CurrentDesk);
	  SendPacket (Module, M_NEW_PAGE, 3, Scr.Vx, Scr.Vy, Scr.CurrentDesk);
	  for (t = Scr.ASRoot.next; t != NULL; t = t->next)
	    {
	      SendConfig (Module, M_CONFIGURE_WINDOW, t);
	      SendName (Module, M_WINDOW_NAME, t->w, t->frame,
			(unsigned long) t, t->name);
	      SendName (Module, M_ICON_NAME, t->w, t->frame,
			(unsigned long) t, t->icon_name);
	      SendName (Module, M_RES_CLASS, t->w, t->frame,
			(unsigned long) t, t->class.res_class);
	      SendName (Module, M_RES_NAME, t->w, t->frame,
			(unsigned long) t, t->class.res_name);

	      if ((t->flags & ICONIFIED) && (!(t->flags & ICON_UNMAPPED)))
		SendPacket (Module, M_ICONIFY, 7, t->w, t->frame,
			    (unsigned long) t,
			    t->icon_p_x, t->icon_p_y,
			    t->icon_p_width,
			    t->icon_p_height);
	      if ((t->flags & ICONIFIED) && (t->flags & ICON_UNMAPPED))
		SendPacket (Module, M_ICONIFY, 7, t->w, t->frame,
			    (unsigned long) t, 0, 0, 0, 0);
	    }
	  SendPacket (Module, M_END_WINDOWLIST, 0);
	}
    default:
      break;			/* to avoid warnings */
    }

/* Save function for SmartCirc use in F_WARP_F/B (AutoReverse) */

  LastFunction = func;

  /* Only wait for an all-buttons-up condition after calls from
   * regular built-ins, not from complex-functions or modules. */
  if (Module == -1)
    WaitForButtonsUp ();

  return;
}


void
ChangeWarpIndex (const long current_win_warp_index, FunctionCode func)
{
  ASWindow *t;

  /* if the cursor was not placed on any window and F_WARP_B was called,
     no change to the warp list will be done */
  if ((func == F_WARP_B) && (!current_win_warp_index))
    return;

  /* go through the list of all windows and change their warp index */
  for (t = Scr.ASRoot.next; t != NULL; t = t->next)
    {
      /* we're warping forwards */
      if (func == F_WARP_F)
	{
	  /* if the window's warp_index is lower than of the current window
	     it's increased (it's moved one position down in the list) */
	  if (t->warp_index < current_win_warp_index)
	    t->warp_index++;
	  /* if we found the window with the proper warp_index we move the
	     window to the top of the warp list */
	  else if (t->warp_index == current_win_warp_index)
	    t->warp_index = 0;
	}
      /* we're warping backwards */
      else
	/* func == F_WARP_B */
	{
	  /* if the window's warp index is higher than of the current window,
	     it's decreased (it's moved one position up in the list) */
	  if (t->warp_index > current_win_warp_index)
	    t->warp_index--;
	  /* if we found the window with the proper warp_index we move the
	     window to the top of the warp list */
	  else if (t->warp_index == current_win_warp_index)
	    t->warp_index = 0;
	}
    }
}


ASWindow *
GetNextWindow (const ASWindow * current_win, const int func)
{
  ASWindow *t;
  ASWindow *win_selected, *win_reverse_selected;
  long selected_warp, reverse_selected_warp;
  long look_for_warp;
  int found;

  /* initial settings */
  win_selected = NULL;
  win_reverse_selected = NULL;
  if ((func == F_WARP_F) || (current_win == NULL))
    {
      selected_warp = LastWarpIndex;
      reverse_selected_warp = LastWarpIndex;
    }
  else
    /* (func == F_WARP_B) || (current_win == NULL) */
    {
      selected_warp = 0;
      reverse_selected_warp = 0;
    }

  /* if the mouse cursor wasn't placed on any window, we'll try to find the
     most recent one */
  if (current_win == NULL)
    look_for_warp = 0;
  else
    look_for_warp = current_win->warp_index;

  /* traverse through all the windows in the list and try to find the most
     appropriate one */
  found = 0;
  for (t = Scr.ASRoot.next; t != NULL; t = t->next)
    {
      if (AutoTabThroughDesks)
	{
	  if (((Scr.flags & CirculateSkipIcons) && (t->flags & ICONIFIED)) ||
	      (t->flags & CIRCULATESKIP) ||
	      (t->flags & WINDOWLISTSKIP))
	    continue;
	}
      else
	/* AutoTabThroughDesks == 0 */
	{
	  if (((Scr.flags & CirculateSkipIcons) && (t->flags & ICONIFIED)) ||
	      (t->Desk != Scr.CurrentDesk) ||
	      (t->flags & CIRCULATESKIP) ||
	      (t->flags & WINDOWLISTSKIP))
	    continue;
	}
      if (((func == F_WARP_F) &&
	   (((current_win != NULL) && (t->warp_index > look_for_warp) && (t->warp_index <= selected_warp)) ||
	    ((current_win == NULL) && (t->warp_index >= look_for_warp) && (t->warp_index < selected_warp)))) ||
	  ((func == F_WARP_B) &&
	   (((current_win != NULL) && (t->warp_index < look_for_warp) && (t->warp_index >= selected_warp)) ||
	    ((current_win == NULL) && (t->warp_index >= look_for_warp) && (t->warp_index < selected_warp)))))
	{
	  selected_warp = t->warp_index;
	  win_selected = t;
	  found = 1;
	}
      else if (((func == F_WARP_F) && (t->warp_index < reverse_selected_warp)) ||
	    ((func == F_WARP_B) && (t->warp_index > reverse_selected_warp)))
	{
	  reverse_selected_warp = t->warp_index;
	  win_reverse_selected = t;
	}
    }
  if (!found)
    win_selected = win_reverse_selected;

  return (win_selected);
}

/* Circulate()
   ** find the next window in circulation order; if direction == DOWN, the next 
   ** window is the window with the next highest circulate_sequence after 
   ** tmp_win, or the lowest circulate_sequence if tmp_win is the highest 
   ** overall; if direction == UP, the next window is the window with the next 
   ** lowest circulate_sequence after tmp_win, or the highest if tmp_win is 
   ** the lowest overall
   ** returns the next window, or if no next window is found, returns NULL
 */
ASWindow *
Circulate (ASWindow * tmp_win, char *action, int direction)
{
  ASWindow *t, *target = NULL, *first = NULL, *last = NULL;

  if (action != NULL && *action == '\0')
    action = NULL;

  for (t = Scr.ASRoot.next; t != NULL; t = t->next)
    {
      /* look for interesting windows */
      if (!(t->flags & CIRCULATESKIP) &&
	  !(t->flags & NOFOCUS) &&
	  !(t->flags & WINDOWLISTSKIP) &&
	  (AutoTabThroughDesks || t->Desk == Scr.CurrentDesk) &&
	  (!(Scr.flags & CirculateSkipIcons) || !(t->flags & ICONIFIED)) &&
	  (action == NULL ||
	   (t->name != NULL && matchWildcards (action, t->name)) ||
	   (t->icon_name != NULL && matchWildcards (action, t->icon_name)) ||
	   (t->class.res_name != NULL && matchWildcards (action, t->class.res_name))))
	{
	  /* update first, last, and target */
	  if (first == NULL || t->circulate_sequence < first->circulate_sequence)
	    first = t;
	  if (last == NULL || t->circulate_sequence > last->circulate_sequence)
	    last = t;
	  if (tmp_win)
	    {
	      if (t != tmp_win &&
		  ((direction == DOWN &&
		    t->circulate_sequence < tmp_win->circulate_sequence &&
		    (target == NULL ||
		     t->circulate_sequence > target->circulate_sequence)) ||
		   (direction == UP &&
		    t->circulate_sequence > tmp_win->circulate_sequence &&
		    (target == NULL ||
		     t->circulate_sequence < target->circulate_sequence))))
		target = t;
	    }
	  else if (t != target &&
		   ((direction == DOWN &&
		     (target == NULL ||
		      t->circulate_sequence > target->circulate_sequence)) ||
		    (direction == UP &&
		     (target == NULL ||
		      t->circulate_sequence < target->circulate_sequence))))
	    target = t;
	}
    }
  if (target == NULL)
    {
      if (direction == DOWN && first == tmp_win && last != tmp_win)
	target = last;
      else			/* if (direction == UP && first != tmp_win && last == tmp_win) */
	target = first;
    }
  return target;
}

/***********************************************************************
 *
 *  Procedure:
 *	DeferExecution - defer the execution of a function to the
 *	    next button press if the context is C_ROOT
 *
 *  Inputs:
 *      eventp  - pointer to XEvent to patch up
 *      w       - pointer to Window to patch up
 *      tmp_win - pointer to ASWindow Structure to patch up
 *	context	- the context in which the mouse button was pressed
 *	func	- the function to defer
 *	cursor	- the cursor to display while waiting
 *      finishEvent - ButtonRelease or ButtonPress; tells what kind of event to
 *                    terminate on.
 *
 ***********************************************************************/
int
DeferExecution (XEvent * eventp, Window * w, ASWindow ** tmp_win,
		unsigned long *context, int cursor, int FinishEvent)
{
  int done;
  int finished = 0;
  Window dummy;
  Window original_w;

  original_w = *w;

  if ((*context != C_ROOT) && (*context != C_NO_CONTEXT))
    {
      if ((FinishEvent == ButtonPress) || ((FinishEvent == ButtonRelease) &&
					   (eventp->type != ButtonPress)))
	{
	  return FALSE;
	}
    }
  if (!GrabEm (cursor))
    {
      XBell (dpy, Scr.screen);
      return True;
    }
  while (!finished)
    {
      done = 0;
      /* block until there is an event */
      XMaskEvent (dpy, ButtonPressMask | ButtonReleaseMask |
		  ExposureMask | KeyPressMask |
		  ButtonMotionMask | PointerMotionMask	/* | EnterWindowMask | 
							   LeaveWindowMask */ , eventp);
      StashEventTime (eventp);

      if (eventp->type == KeyPress)
	Keyboard_shortcuts (eventp, FinishEvent, 20);
      if (eventp->type == FinishEvent)
	finished = 1;
      if (eventp->type == ButtonPress)
	{
	  XAllowEvents (dpy, ReplayPointer, CurrentTime);
	  done = 1;
	}
      if (eventp->type == ButtonRelease)
	done = 1;
      if (eventp->type == KeyPress)
	done = 1;

      if (!done)
	{
	  DispatchEvent ();
	}
    }

  *w = eventp->xany.window;
  if (((*w == Scr.Root) || (*w == Scr.NoFocusWin))
      && (eventp->xbutton.subwindow != (Window) 0))
    {
      *w = eventp->xbutton.subwindow;
      eventp->xany.window = *w;
    }
  if (*w == Scr.Root)
    {
      *context = C_ROOT;
      XBell (dpy, Scr.screen);
      UngrabEm ();
      return TRUE;
    }
  if (XFindContext (dpy, *w, ASContext, (caddr_t *) tmp_win) == XCNOENT)
    {
      *tmp_win = NULL;
      XBell (dpy, Scr.screen);
      UngrabEm ();
      return (TRUE);
    }
/*  if (*w == (*tmp_win)->Parent)
   *w = (*tmp_win)->w;

   if (original_w == (*tmp_win)->Parent)
   original_w = (*tmp_win)->w;
 */
  /* this ugly mess attempts to ensure that the release and press
   * are in the same window. */
  /* the windows are considered the same if any one of these is found true:
     - the window IDs are identical
     - the original window is Scr.Root, None, or NoFocusWin
     - the original window is the client window and the current window
     is the frame window
   */
/*  if ((*w != original_w) && (original_w != Scr.Root) &&
   (original_w != None) && (original_w != Scr.NoFocusWin) &&
   (*w != (*tmp_win)->frame) && (original_w != (*tmp_win)->w))
   {
   *context = C_ROOT;
   XBell(dpy,Scr.screen);
   UngrabEm();
   return TRUE;
   }
 */
  *context = GetContext (*tmp_win, eventp, &dummy);

  UngrabEm ();
  return FALSE;
}


/****************************************************************************
 *
 * This is used to tell applications which windows on the screen are
 * top level appication windows, and which windows are the icon windows
 * that go with them.
 *
 ****************************************************************************/
void
SetMapStateProp (ASWindow * tmp_win, int state)
{
  unsigned long data[2];	/* "suggested" by ICCCM version 1 */

  data[0] = (unsigned long) state;
  data[1] = (unsigned long) tmp_win->icon_pixmap_w;

  XChangeProperty (dpy, tmp_win->w, _XA_WM_STATE, _XA_WM_STATE, 32,
		   PropModeReplace, (unsigned char *) data, 2);
  return;
}
/***************************************************************************
 *
 *  Moves the viewport within the virtual desktop
 *
 ***************************************************************************/
#ifndef NO_VIRTUAL
void
MoveViewport (int newx, int newy, Bool grab)
{
  ASWindow *t;
  int deltax, deltay;

  if (grab)
    XGrabServer (dpy);


  if (newx > Scr.VxMax)
    newx = Scr.VxMax;
  if (newy > Scr.VyMax)
    newy = Scr.VyMax;
  if (newx < 0)
    newx = 0;
  if (newy < 0)
    newy = 0;

  deltay = Scr.Vy - newy;
  deltax = Scr.Vx - newx;

  Scr.Vx = newx;
  Scr.Vy = newy;
  Broadcast (M_NEW_PAGE, 3, Scr.Vx, Scr.Vy, Scr.CurrentDesk);

  if (deltax || deltay)
    {
      /* Here's an attempt at optimization by reducing (hopefully) the expose 
       * events sent to moved windows.  Move the windows which will be on the 
       * new desk, from the front window to the back one.  Move the other 
       * windows from the back one to the front.  Thus if a window is totally 
       * (or partially) obscured, it will not be uncovered if possible. */

      /* do the windows which will be on the new desk first */
      for (t = Scr.ASRoot.next; t != NULL; t = t->next)
	{
	  t->flags &= ~PASS_1;

	  /* don't move sticky windows */
	  if (!(t->flags & STICKY))
	    {
	      int x, y, w, h;
	      get_window_geometry (t, t->flags, &x, &y, &w, &h);
	      w += 2 * t->bw;
	      h += 2 * t->bw;
	      /* do the window now if it would be onscreen after moving */
	      if (x + deltax < Scr.MyDisplayWidth && x + deltax + w > 0 &&
		  y + deltax < Scr.MyDisplayHeight && y + deltay + h > 0)
		{
		  t->flags |= PASS_1;
		  SetupFrame (t, t->frame_x + deltax, t->frame_y + deltay,
			      t->frame_width, t->frame_height, FALSE);
		}
	      /* if StickyIcons is set, treat the icon as sticky */
	      if (!(Scr.flags & StickyIcons) &&
		  x + deltax < Scr.MyDisplayWidth && x + deltax + w > 0 &&
		  y + deltax < Scr.MyDisplayHeight && y + deltay + h > 0)
		{
		  t->flags |= PASS_1;
		  t->icon_p_x += deltax;
		  t->icon_p_y += deltay;
		  if (t->flags & ICONIFIED)
		    {
		      if (t->icon_pixmap_w != None)
			XMoveWindow (dpy, t->icon_pixmap_w, t->icon_p_x,
				     t->icon_p_y);
		      if (t->icon_title_w != None)
			XMoveWindow (dpy, t->icon_title_w, t->icon_p_x,
				     t->icon_p_y + t->icon_p_height);
		      if (!(t->flags & ICON_UNMAPPED))
			Broadcast (M_ICON_LOCATION, 7, t->w, t->frame,
				   (unsigned long) t,
				   t->icon_p_x, t->icon_p_y,
				   t->icon_p_width, t->icon_p_height);
		    }
		}
	    }
	}
      /* now do the other windows, back to front */
      for (t = &Scr.ASRoot; t->next != NULL; t = t->next);
      for (; t != &Scr.ASRoot; t = t->prev)
	if (!(t->flags & PASS_1))
	  {
	    /* don't move sticky windows */
	    if (!(t->flags & STICKY))
	      {
		SetupFrame (t, t->frame_x + deltax, t->frame_y + deltay,
			    t->frame_width, t->frame_height, FALSE);
		/* if StickyIcons is set, treat the icon as sticky */
		if (!(Scr.flags & StickyIcons))
		  {
		    t->icon_p_x += deltax;
		    t->icon_p_y += deltay;
		    if (t->flags & ICONIFIED)
		      {
			if (t->icon_pixmap_w != None)
			  XMoveWindow (dpy, t->icon_pixmap_w, t->icon_p_x,
				       t->icon_p_y);
			if (t->icon_title_w != None)
			  XMoveWindow (dpy, t->icon_title_w, t->icon_p_x,
				       t->icon_p_y + t->icon_p_height);
			if (!(t->flags & ICON_UNMAPPED))
			  Broadcast (M_ICON_LOCATION, 7, t->w, t->frame,
				     (unsigned long) t,
				     t->icon_p_x, t->icon_p_y,
				     t->icon_p_width, t->icon_p_height);
		      }
		  }
	      }
	  }
      /* autoplace sticky icons so they don't wind up over a stationary icon */
      AutoPlaceStickyIcons ();
    }
#ifndef NO_VIRTUAL
  checkPanFrames ();

  /* do this with PanFrames too ??? HEDU */
  while (XCheckTypedEvent (dpy, MotionNotify, &Event))
    StashEventTime (&Event);
#endif
  UpdateVisibility ();
  if (grab)
    XUngrabServer (dpy);
}
#endif


/**************************************************************************
 *
 * Moves focus to specified window 
 *
 *************************************************************************/
void
FocusOn (ASWindow * t, int DeIconifyOnly, Bool circulating)
{
#ifndef NO_VIRTUAL
  int dx, dy;
  int cx, cy;
#endif
  int x, y;

  if (t == NULL)
    return;

  if (t->Desk != Scr.CurrentDesk)
    changeDesks (0, t->Desk);

#ifndef NO_VIRTUAL

  if (t->flags & ICONIFIED)
    {
      cx = t->icon_p_x + t->icon_p_width / 2;
      cy = t->icon_p_y + t->icon_p_height / 2;
    }
  else
    {
      cx = t->frame_x + t->frame_width / 2;
      cy = t->frame_y + t->frame_height / 2;
    }

  /* Put center of window on the visible screen */
  if ((!DeIconifyOnly) && (Scr.flags & CenterOnCirculate))
    {
      dx = cx - Scr.MyDisplayWidth / 2 + Scr.Vx;
      dy = cy - Scr.MyDisplayHeight / 2 + Scr.Vy;
    }
  else
    {
      dx = (cx + Scr.Vx) / Scr.MyDisplayWidth * Scr.MyDisplayWidth;
      dy = (cy + Scr.Vy) / Scr.MyDisplayHeight * Scr.MyDisplayHeight;
    }
  MoveViewport (dx, dy, True);
#endif

  RaiseWindow (t);

#ifdef NO_VIRTUAL
  /* If the window is still not visible, make it visible! */
  if (((t->frame_x + t->frame_width) < 0) || (t->frame_y + t->frame_height < 0) ||
    (t->frame_x > Scr.MyDisplayWidth) || (t->frame_y > Scr.MyDisplayHeight))
    {
      SetupFrame (t, 0, 0, t->frame_width, t->frame_height, False);
      RaiseWindow (t);
    }
#endif

  if (t->flags & ICONIFIED)
    {
      x = t->icon_p_x;
      y = t->icon_p_y;
    }
  else
    {
      x = t->frame_x + Xzap;
      y = t->frame_y + Yzap;
    }

  if (!(Scr.flags & ClickToFocus))
    XWarpPointer (dpy, None, Scr.Root, 0, 0, 0, 0, x + Xzap, y + Yzap);

  UngrabEm ();
  SetFocus (t->w, t, circulating);
}


/***********************************************************************
 *
 *  Procedure:
 *	(Un)Stick a window.
 *
 ***********************************************************************/
void
Stick (ASWindow * t)
{
  ASWindow *temp;

  /* flip sticky state */
  if (t->flags & STICKY)
    t->flags &= ~STICKY;
  else
    t->flags |= STICKY;

  if (Scr.Hilite != t)
    {
      /* Need to make SetBorder change the window back color */
      temp = Scr.Hilite;
      SetBorder (t, True, True, True, None);
      SetBorder (t, False, True, True, None);
      SetBorder (temp, True, True, True, None);
    }
  BroadcastConfig (M_CONFIGURE_WINDOW, t);
  /*ßß MoveResizePagerView */
}


/***********************************************************************
 *
 *  Procedure:
 *	(Un)Maximize a window.
 *
 ***********************************************************************/
void
Maximize (ASWindow * tmp_win, int val1, int val2,
	  int val1_unit, int val2_unit)
{
  if (tmp_win->flags & MAXIMIZED)
    {
      tmp_win->flags &= ~MAXIMIZED;
      SetupFrame (tmp_win, tmp_win->orig_x, tmp_win->orig_y, tmp_win->orig_wd,
		  tmp_win->orig_ht, TRUE);
      SetBorder (tmp_win, True, True, True, None);
    }
  else
    {
      int nx = tmp_win->frame_x;
      int ny = tmp_win->frame_y;
      int nw = tmp_win->frame_width + 2 * tmp_win->bw;
      int nh = tmp_win->frame_height + 2 * tmp_win->bw;
      int x, y, w, h;
      ASWindow *t;

      x = 0;
      y = 0;
      if (!val1)		/* width */
	{
	  w = nw;
	  x = nx;
	}
      else
	w = Scr.MyDisplayWidth;
      if (!val2)		/* height */
	{
	  h = nh;
	  y = ny;
	}
      else
	h = Scr.MyDisplayHeight;

      /* try not to maximize over AvoidCover windows */
      for (t = Scr.ASRoot.next; t != NULL; t = t->next)
	if (t != tmp_win && (t->flags & AVOID_COVER))
	  {
	    int fx = t->frame_x;
	    int fy = t->frame_y;
	    int fw = t->frame_width + 2 * t->bw;
	    int fh = t->frame_height + 2 * t->bw;
	    /* if we already overlap the AvoidCover window, ignore it */
	    if (nx + nw > fx && fx + fw > nx && ny + nh > fy && fy + fh > ny)
	      continue;
	    if (x + w > fx && fx + fw > x && y + h > fy && fy + fh > y)
	      {
		int px, py, pw, ph;
		if (nx > fx)
		  {
		    px = fx + fw;
		    pw = x + w - px;
		  }
		else
		  {
		    px = x;
		    pw = fx - x;
		  }
		if (ny > fy)
		  {
		    py = fy + fh;
		    ph = y + h - py;
		  }
		else
		  {
		    py = y;
		    ph = fy - y;
		  }
		if (pw * h > w * ph)
		  {
		    x = px;
		    w = pw;
		  }
		else
		  {
		    y = py;
		    h = ph;
		  }
	      }
	  }

      if (val1 != 0)
	{
	  if (val1_unit == Scr.MyDisplayWidth)
	    val1_unit = w;
	  if (val1 < 0)
	    x += w - val1 * val1_unit / 100;
	  w = val1 * val1_unit / 100;
	}

      if (val2 != 0)
	{
	  if (val2_unit == Scr.MyDisplayHeight)
	    val2_unit = h;
	  if (val2 < 0)
	    y += h - val2 * val2_unit / 100;
	  h = val2 * val2_unit / 100;
	}

      tmp_win->flags |= MAXIMIZED;
      ConstrainSize (tmp_win, &w, &h);
      SetupFrame (tmp_win, x, y, w, h, TRUE);
      SetBorder (tmp_win, True, True, True, tmp_win->right_w[0]);
    }
  UpdateVisibility ();
}

/***********************************************************************
 *    
 *  Procedure:
 *      (Un)Shade a window.
 *        
 ***********************************************************************/
void
Shade (ASWindow * tmp_win)
{
  XWindowAttributes winattrs;
  unsigned long eventMask;
#ifndef NO_SHADE
  int y, s, w, h;
#endif

  if (ShadeAnimationSteps <= 0)
    ShadeAnimationSteps = 1;
  if (tmp_win == NULL)
    return;

  if (tmp_win->flags & SHADED)
    {
      /* unshade window */
/*      tmp_win->flags |= MAPPED; */
      XMapWindow (dpy, tmp_win->w);
#ifndef NO_SHADE
      /* do the unshading animation */
      if (tmp_win->flags & VERTICAL_TITLE)
	{
	  int x;
	  h = tmp_win->title_height;
	  w = tmp_win->title_width;
	  s = tmp_win->frame_width / ShadeAnimationSteps;
	  if (s < 1)
	    s = 1;
	  x = -(tmp_win->frame_width - (w + tmp_win->bw));
	  while (x < 0)
	    {
	      XResizeWindow (dpy, tmp_win->frame, w, h);
	      XMoveWindow (dpy, tmp_win->w, x, 0);
	      XSync (dpy, 0);
	      w += s;
	      x += s;
	    }
	  XMoveWindow (dpy, tmp_win->w, 0, 0);
	}
      else
	{
	  h = tmp_win->title_height;
	  w = tmp_win->title_width;
	  s = tmp_win->frame_height / ShadeAnimationSteps;
	  if (s < 1)
	    s = 1;
	  y = -(tmp_win->frame_height - (h + tmp_win->bw));
	  while (y < 0)
	    {
	      XResizeWindow (dpy, tmp_win->frame, w, h);
	      XMoveWindow (dpy, tmp_win->w, 0, y);
	      XSync (dpy, 0);
	      h += s;
	      y += s;
	    }
	  XMoveWindow (dpy, tmp_win->w, 0, 0);
	}
#endif /* ! NO_SHADE */
      XResizeWindow (dpy, tmp_win->frame, tmp_win->frame_width,
		     tmp_win->frame_height);
/*
   if ((Scr.flags & StubbornIcons) || (Scr.flags & ClickToFocus))
   FocusOn(tmp_win, 1, True, 2);
 */
      tmp_win->flags &= ~SHADED;
      Broadcast (M_UNSHADE, 3, tmp_win->w, tmp_win->frame, (unsigned long) tmp_win);
      BroadcastConfig (M_CONFIGURE_WINDOW, tmp_win);
    }
  else
    {
      tmp_win->flags |= SHADED;

      /* can't shade a window with no titlebar */
      if (!tmp_win->flags & TITLE)
	return;

#ifndef NO_SHADE
      XLowerWindow (dpy, tmp_win->w);
      /* do the shading animation */
      if (tmp_win->flags & VERTICAL_TITLE)
	{
	  int x;
	  h = tmp_win->frame_height;
	  w = tmp_win->frame_width;
	  s = w / ShadeAnimationSteps;
	  x = 0;
	  while (w > tmp_win->title_width)
	    {
	      XMoveWindow (dpy, tmp_win->w, x, 0);
	      XResizeWindow (dpy, tmp_win->frame, w, h);
	      XSync (dpy, 0);
	      w -= s;
	      x -= s;
	    }
	  XMoveWindow (dpy, tmp_win->w, 0, 0);
	}
      else
	{
	  h = tmp_win->frame_height;
	  w = tmp_win->frame_width;
	  s = h / ShadeAnimationSteps;
	  y = 0;

	  while (h > tmp_win->title_height)
	    {
	      XMoveWindow (dpy, tmp_win->w, 0, y);
	      XResizeWindow (dpy, tmp_win->frame, w, h);
	      XSync (dpy, 0);
	      h -= s;
	      y -= s;
	    }
	}
      XMoveWindow (dpy, tmp_win->w, 0, 0);
#endif /* ! NO_SHADE */

      XResizeWindow (dpy, tmp_win->frame, tmp_win->title_width,
		     tmp_win->title_height);

      XGetWindowAttributes (dpy, tmp_win->w, &winattrs);
      eventMask = winattrs.your_event_mask;

      /*
       * Prevent the receipt of an UnmapNotify, since that would
       * cause a transition to the Withdrawn state.
       */
      /* tmp_win->flags &= ~MAPPED; */
      XSelectInput (dpy, tmp_win->w, eventMask & ~StructureNotifyMask);
      XUnmapWindow (dpy, tmp_win->w);
      XSelectInput (dpy, tmp_win->w, eventMask);

      Broadcast (M_SHADE, 3, tmp_win->w, tmp_win->frame, (unsigned long) tmp_win);
      BroadcastConfig (M_CONFIGURE_WINDOW, tmp_win);
    }
  UpdateVisibility ();
}

/****************************************************************************
 *
 * paste the cut buffer into the window with the input focus
 * 
 ***************************************************************************/

void
PasteSelection (void)
{
  if (Scr.Focus != NULL)
    {
      int length;
      int buffer = 0;
      char *buf;

      buf = XFetchBuffer (dpy, &length, buffer);

/* everything from here on is a kludge; if you know a better way to do it, 
 * please fix it!
 */
      if ((buf != NULL) && (length > 0))
	{
	  int i;
	  XEvent event;
	  event.xkey.display = dpy;
	  event.xkey.window = Scr.Focus->w;
	  event.xkey.time = CurrentTime;
	  event.xkey.same_screen = True;
	  XQueryPointer (dpy, Scr.Focus->w, &event.xkey.root, &event.xkey.subwindow, &event.xkey.x_root, &event.xkey.y_root, &event.xkey.x, &event.xkey.y, &event.xkey.state);
	  for (i = 0; i < length; i++)
	    {
	      event.xkey.state &= ~(ShiftMask | ControlMask);
	      if (*buf < ' ')
		{
		  char ch = *buf;
		  /* this code assumes an ASCII character mapping */
		  /* need to translate newline to carriage-return */
		  if (ch == '\n')
		    ch = '\r';
		  event.xkey.keycode = XKeysymToKeycode (dpy, ch + '@');
		  event.xkey.state |= ControlMask;
		}
	      else if (isupper (*buf))
		{
		  event.xkey.keycode = XKeysymToKeycode (dpy, *buf);
		  event.xkey.state |= ShiftMask;
		}
	      else
		event.xkey.keycode = XKeysymToKeycode (dpy, *buf);
	      if (event.xkey.keycode != NoSymbol)
		{
		  event.type = KeyPress;
		  XSendEvent (dpy, Scr.Focus->w, False, KeyPressMask, &event);
		  event.type = KeyRelease;
		  XSendEvent (dpy, Scr.Focus->w, False, KeyReleaseMask, &event);
		}
	      buf++;
	    }
	}
    }
}

/*****************************************************************************
 *
 * Grab the pointer and keyboard
 *
 ****************************************************************************/

Bool
GrabEm (int cursor)
{
  int i = 0, val = 0;
  unsigned int mask;
#ifndef DONT_GRAB_SERVER
  XSync (dpy, 0);
  /* move the keyboard focus prior to grabbing the pointer to
   * eliminate the enterNotify and exitNotify events that go
   * to the windows */
  if (Scr.PreviousFocus == NULL)
    Scr.PreviousFocus = Scr.Focus;
  SetFocus (Scr.NoFocusWin, NULL, False);
  mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionMask
    | EnterWindowMask | LeaveWindowMask;
  while ((i < 1000) && (val = XGrabPointer (dpy, Scr.Root, True, mask,
				     GrabModeAsync, GrabModeAsync, Scr.Root,
				      Scr.ASCursors[cursor], CurrentTime) !=
			GrabSuccess))
    {
      i++;
      /* If you go too fast, other windows may not get a change to release
       * any grab that they have. */
      sleep_a_little (1000);
    }				/* If we fall out of the loop without grabbing the pointer, its
				   time to give up */
  XSync (dpy, 0);
  if (val != GrabSuccess)
    {
      return False;
    }
#endif
  return True;
}


/*****************************************************************************
 *
 * UnGrab the pointer and keyboard
 *
 ****************************************************************************/
void
UngrabEm ()
{
  Window w;

  XSync (dpy, 0);
  XUngrabPointer (dpy, CurrentTime);

  if (Scr.PreviousFocus != NULL)
    {
      w = Scr.PreviousFocus->w;

      /* if the window still exists, focus on it */
      if (w)
	{
	  SetFocus (w, Scr.PreviousFocus, True);
	}
      Scr.PreviousFocus = NULL;
    }
  XSync (dpy, 0);
}


/*****************************************************************************
 *
 * Waits Scr.ClickTime, or until it is evident that the user is not
 * clicking, but is moving the cursor
 *
 ****************************************************************************/
Bool
IsClick (int x, int y, unsigned EndMask, XEvent * d)
{
  int xcurrent, ycurrent, total = 0;

  xcurrent = x;
  ycurrent = y;
  while ((total < Scr.ClickTime) &&
	 (x - xcurrent < 5) && (x - xcurrent > -5) &&
	 (y - ycurrent < 5) && (y - ycurrent > -5))
    {
      sleep_a_little (10000);
      total += 10;
      if (XCheckMaskEvent (dpy, EndMask, d))
	{
	  StashEventTime (d);
	  return True;
	}
      if (XCheckMaskEvent (dpy, ButtonMotionMask | PointerMotionMask, d))
	{
	  xcurrent = d->xmotion.x_root;
	  ycurrent = d->xmotion.y_root;
	  StashEventTime (d);
	}
    }
  return False;
}

/*****************************************************************************
 *
 * Builtin which determines if the button press was a click or double click...
 *
 ****************************************************************************/
void
ComplexFunction (Window w, ASWindow * tmp_win, XEvent * eventp,
		 unsigned long context, MenuRoot * mr)
{
  char type = MOTION;
  char c;
  XEvent *ev;
  MenuItem *mi;
  XEvent d;
  Bool Persist = False;
  Bool HaveDoubleClick = False;
  Bool HaveTripleClick = False;
  Bool NeedsTarget = False;
  int x, y;

  if (mr == NULL)
    return;
  mi = mr->first;
  while (mi != NULL)
    {
      /* make lower case */
      c = (mi->item != NULL) ? *mi->item : '\0';
      if (IsWindowFunc (mi->func))
	NeedsTarget = True;
      if (isupper (c))
	c = tolower (c);
      if (c == DOUBLE_CLICK)
	HaveDoubleClick = True;
      if (c == TRIPLE_CLICK)
	HaveTripleClick = True;
      else if (c == IMMEDIATE)
	{
	  if (tmp_win)
	    w = tmp_win->frame;
	  else
	    w = None;
	  ExecuteFunction (mi->func, mi->action, w,
			   tmp_win, eventp, context, mi->val1, mi->val2,
			   mi->val1_unit, mi->val2_unit,
			   mi->menu, -1);
	}
      else
	Persist = True;
      mi = mi->next;
    }

  if (!Persist)
    return;

  /* Only defer execution if there is a possibility of needing
   * a window to operate on */
  if (NeedsTarget)
    {
      if (DeferExecution (eventp, &w, &tmp_win, &context, SELECT, ButtonPress))
	{
	  WaitForButtonsUp ();
	  return;
	}
    }
  if (!GrabEm (SELECT))
    {
      XBell (dpy, Scr.screen);
      return;
    }
  XQueryPointer (dpy, Scr.Root, &JunkRoot, &JunkChild,
		 &x, &y, &JunkX, &JunkY, &JunkMask);

  /* A window has already been selected */
  ev = eventp;

  /* Wait and see if we have a click, or a move */
  /* wait 100 msec, see if the user releases the button */
  if (IsClick (x, y, ButtonReleaseMask, &d))
    {
      type = CLICK;
      ev = &d;
    }
  /* If it was a click, wait to see if its a double click */
  if ((HaveDoubleClick) && (type == CLICK) &&
      (IsClick (x, y, ButtonPressMask, &d)))
    {
      type = ONE_AND_A_HALF_CLICKS;
      ev = &d;
    }
  if ((HaveDoubleClick) && (type == ONE_AND_A_HALF_CLICKS) &&
      (IsClick (x, y, ButtonReleaseMask, &d)))
    {
      type = DOUBLE_CLICK;
      ev = &d;
    }
  if ((HaveTripleClick) && (type == DOUBLE_CLICK) &&
      (IsClick (x, y, ButtonPressMask, &d)))
    {
      type = TWO_AND_A_HALF_CLICKS;
      ev = &d;
    }
  if ((HaveTripleClick) && (type == TWO_AND_A_HALF_CLICKS) &&
      (IsClick (x, y, ButtonReleaseMask, &d)))
    {
      type = TRIPLE_CLICK;
      ev = &d;
    }
  /* some functions operate on button release instead of 
   * presses. These gets really weird for complex functions ... */
  if (eventp->type == ButtonPress)
    eventp->type = ButtonRelease;

  mi = mr->first;
  while (mi != NULL)
    {
      /* make lower case */
      c = (mi->item != NULL) ? *mi->item : '\0';
      if (isupper (c))
	c = tolower (c);
      if (c == type)
	{
	  if (tmp_win)
	    w = tmp_win->frame;
	  else
	    w = None;
	  ExecuteFunction (mi->func, mi->action, w,
			   tmp_win, eventp, context,
			   mi->val1, mi->val2,
			   mi->val1_unit, mi->val2_unit, mi->menu, -2);
	}
      mi = mi->next;
    }
  WaitForButtonsUp ();
  UngrabEm ();
}


/* For Ultrix 4.2 */
#include <sys/types.h>
#include <sys/time.h>


/**************************************************************************
 * 
 * Move to a new desktop
 *
 *************************************************************************/
void
changeDesks (int val1, int val2)
{
  int oldDesk;
  ASWindow *t;
  ASWindow *FocusWin = 0;
  static ASWindow *StickyWin = 0;
  unsigned long data;
  extern Atom _XA_WIN_DESK;

  oldDesk = Scr.CurrentDesk;

  if ((val1 != 0) && (val1 != 10000))
    Scr.CurrentDesk = Scr.CurrentDesk + val1;
  else
    Scr.CurrentDesk = val2;

  /* update property to tell us what desk we were on when we restart; 
   * always do this so that when we get called from main(), the property 
   * will be set; this property is what we use to determine if we're 
   * starting up for the first time, or restarting */
  data = (unsigned long) Scr.CurrentDesk;
  XChangeProperty (dpy, Scr.Root, _XA_WIN_DESK, XA_CARDINAL, 32,
		   PropModeReplace, (unsigned char *) &data, 1);

  if (Scr.CurrentDesk == oldDesk)
    return;

  Broadcast (M_NEW_DESK, 1, Scr.CurrentDesk);

  /* Scan the window list, mapping windows on the new Desk, unmapping 
   * windows on the old Desk; do this in reverse order to reduce client 
   * expose events */
  XGrabServer (dpy);
  for (t = Scr.ASRoot.next; t != NULL; t = t->next)
    {
      /* Only change mapping for non-sticky windows */
      if (!((t->flags & ICONIFIED) && (Scr.flags & StickyIcons)) &&
	  !(t->flags & STICKY) && !(t->flags & ICON_UNMAPPED))
	{
	  if (t->Desk == oldDesk)
	    {
	      if (Scr.Focus == t)
		t->FocusDesk = oldDesk;
	      else
		t->FocusDesk = -1;
	      UnmapIt (t);
	    }
	  else if (t->Desk == Scr.CurrentDesk)
	    {
	      MapIt (t);
	      if (t->FocusDesk == Scr.CurrentDesk)
		{
		  FocusWin = t;
		}
	    }
	}
      else
	{
	  /* Window is sticky */
	  t->Desk = Scr.CurrentDesk;
	  aswindow_set_desk_property (t, t->Desk);
	  if (Scr.Focus == t)
	    {
	      t->FocusDesk = oldDesk;
	      StickyWin = t;
	    }
	}
    }
  XUngrabServer (dpy);
  /* autoplace sticky icons so they don't wind up over a stationary icon */
  AutoPlaceStickyIcons ();

  if (Scr.flags & ClickToFocus)
    {
#ifndef NO_REMEMBER_FOCUS
      if (FocusWin)
	SetFocus (FocusWin->w, FocusWin, False);
      else if (StickyWin && (StickyWin->flags && STICKY))
	SetFocus (StickyWin->w, StickyWin, False);
      else
#endif
	SetFocus (Scr.NoFocusWin, NULL, False);
    }

  CorrectStackOrder ();
  update_windowList ();

  /* Change the look to this desktop's one if it really changed */
#ifdef DIFFERENTLOOKNFEELFOREACHDESKTOP
  QuickRestart ("look&feel");
#endif
}



/**************************************************************************
 * 
 * Move to a new desktop
 *
 *************************************************************************/
void
changeWindowsDesk (ASWindow * t, int new_desk)
{
  if (new_desk == t->Desk)
    return;

  /* change mapping if necessary */
  if (t->Desk == Scr.CurrentDesk)
    {
      t->Desk = new_desk;
      UnmapIt (t);
    }
  else if (new_desk == Scr.CurrentDesk)
    {
      t->Desk = new_desk;
      /* If its an icon, auto-place it */
      if (t->flags & ICONIFIED)
	AutoPlace (t);
      MapIt (t);
    }
  else
    t->Desk = new_desk;

  /* Better re-draw the pager now */
  BroadcastConfig (M_CONFIGURE_WINDOW, t);
  CorrectStackOrder ();
  if (!(t->flags & WINDOWLISTSKIP))
    update_windowList ();

  /* update the _WIN_DESK property */
  aswindow_set_desk_property (t, t->Desk);
}

/* update the _WIN_DESK property */
void
aswindow_set_desk_property (ASWindow * t, int new_desk)
{
  extern Atom _XA_WIN_DESK;
  unsigned long desk = new_desk;

  XChangeProperty (dpy, t->w, _XA_WIN_DESK, XA_CARDINAL, 32, PropModeReplace,
		   (unsigned char *) &desk, 1);
}


/**************************************************************************
 * 
 * Unmaps a window on transition to a new desktop
 *
 *************************************************************************/
void
UnmapIt (ASWindow * t)
{
  XWindowAttributes winattrs;
  unsigned long eventMask;
  /*
   * Prevent the receipt of an UnmapNotify, since that would
   * cause a transition to the Withdrawn state.
   */
  XGetWindowAttributes (dpy, t->w, &winattrs);
  eventMask = winattrs.your_event_mask;
  XSelectInput (dpy, t->w, eventMask & ~StructureNotifyMask);
  if (t->flags & ICONIFIED)
    {
      if (t->icon_pixmap_w != None)
	XUnmapWindow (dpy, t->icon_pixmap_w);
      if (t->icon_title_w != None)
	XUnmapWindow (dpy, t->icon_title_w);
    }
  else if (t->flags & (MAPPED | MAP_PENDING))
    {
      XUnmapWindow (dpy, t->frame);
    }
  XSelectInput (dpy, t->w, eventMask);
}

/**************************************************************************
 * 
 * Maps a window on transition to a new desktop
 *
 *************************************************************************/
void
MapIt (ASWindow * t)
{
  if (t->flags & ICONIFIED)
    {
      if (t->icon_pixmap_w != None)
	XMapWindow (dpy, t->icon_pixmap_w);
      if (t->icon_title_w != None)
	XMapWindow (dpy, t->icon_title_w);
    }
  else if (t->flags & MAPPED)
    {
      XMapWindow (dpy, t->frame);
      t->flags |= MAP_PENDING;
      XMapWindow (dpy, t->Parent);
    }
}

void
QuickRestart (char *what)
{
  extern char *display_name;
  extern int have_the_colors;
  extern Bool shall_override_config_file;
  Bool parse_menu = False;
  Bool parse_look = shall_override_config_file;
  Bool parse_feel = shall_override_config_file;
  Bool update_background = False;

  if (what == NULL)
    return;

  if (strcmp (what, "all") == 0)
    parse_menu = parse_look = parse_feel = update_background = True;
  else if (strcmp (what, "look&feel") == 0)
    parse_look = parse_feel = True;
  else if (strcmp (what, "startmenu") == 0)
    parse_menu = True;
  else if (strcmp (what, "look") == 0)
    parse_look = True;
  else if (strcmp (what, "feel") == 0)
    parse_feel = True;
  else if (strcmp (what, "background") == 0)
    update_background = True;

  /* Force reinstall */
  if (parse_look || parse_feel || parse_menu || shall_override_config_file)
    {
      InstallWindowColormaps (&Scr.ASRoot);
      GrabEm (WAIT);
    }

  /* Don't kill modules */
  /* ClosePipes(); */

  /* delete menus - only if necessary */
  if (shall_override_config_file || parse_menu)
    while (Scr.first_menu != NULL)
      DeleteMenuRoot (Scr.first_menu);

  if (parse_look || parse_feel || parse_menu || shall_override_config_file)
    {
      have_the_colors = 0;

      /* Don't reset desktop position */
      /* InitVariables(0); */

      /* LoadASConfig must be called, or AS will be left in an unusable state */
      LoadASConfig (display_name, Scr.CurrentDesk, parse_menu, parse_look, parse_feel);

#ifndef NO_VIRTUAL
      XUnmapWindow (dpy, Scr.PanFrameLeft.win);
      XUnmapWindow (dpy, Scr.PanFrameRight.win);
      XUnmapWindow (dpy, Scr.PanFrameBottom.win);
      XUnmapWindow (dpy, Scr.PanFrameTop.win);
      Scr.PanFrameBottom.isMapped = Scr.PanFrameTop.isMapped =
	Scr.PanFrameLeft.isMapped = Scr.PanFrameRight.isMapped = False;

      checkPanFrames ();
#endif

      UngrabEm ();
    }

  if (update_background)
    Broadcast (M_NEW_BACKGROUND, 1, 1);
}
