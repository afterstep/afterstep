/*
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

/***********************************************************************
 *
 * code for moving windows
 *
 ***********************************************************************/

#include "../../configure.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"

extern XEvent Event;
extern int    menuFromFrameOrWindowOrTitlebar;
Bool          NeedToResizeToo;

/****************************************************************************
 *
 * Start a window move operation
 *
 ****************************************************************************/
void
move_window (XEvent * eventp, Window w, ASWindow * tmp_win, int context,
			 int val1, int val2, int val1_unit, int val2_unit)
{
	int           FinalX, FinalY;

	/* gotta have a window */
	if (tmp_win == NULL)
		return;

	w = tmp_win->frame;
	if (tmp_win->flags & ICONIFIED)
	{
		if (tmp_win->icon_pixmap_w != None)
		{
			w = tmp_win->icon_pixmap_w;
			if (tmp_win->icon_title_w != None)
				XUnmapWindow (dpy, tmp_win->icon_title_w);
		} else
			w = tmp_win->icon_title_w;
	}

	if (w == None)
		return;

	if ((val1 != 0) || (val2 != 0))
	{
		FinalX = val1 * val1_unit / 100;
		FinalY = val2 * val2_unit / 100;
	} else
		InteractiveMove (&w, tmp_win, &FinalX, &FinalY, eventp);

	if (w == tmp_win->frame)
	{
		SetupFrame (tmp_win, FinalX, FinalY, tmp_win->frame_width, tmp_win->frame_height, FALSE);
	} else
	{										   /* icon window */
		tmp_win->flags |= ICON_MOVED;
		tmp_win->icon_p_x = FinalX;
		tmp_win->icon_p_y = FinalY;
		Broadcast (M_ICON_LOCATION, 7, tmp_win->w, tmp_win->frame,
				   (unsigned long)tmp_win,
				   tmp_win->icon_p_x, tmp_win->icon_p_y,
				   tmp_win->icon_p_width, tmp_win->icon_p_height);
		if (tmp_win->icon_pixmap_w != None)
			XMoveWindow (dpy, tmp_win->icon_pixmap_w, tmp_win->icon_p_x, tmp_win->icon_p_y);
		if (tmp_win->icon_title_w != None)
		{
			XMoveWindow (dpy, tmp_win->icon_title_w, tmp_win->icon_p_x,
						 tmp_win->icon_p_y + tmp_win->icon_p_height);
			XMapWindow (dpy, tmp_win->icon_title_w);
		}
	}

	UpdateVisibility ();
}

void
resist_move (ASWindow * win, int cx, int cy, int *fx, int *fy)
{
	ASWindow     *t;
	int           x = *fx, y = *fy;
	int           w, h;

	get_window_geometry (win, win->flags, NULL, NULL, &w, &h);
	w += 2 * win->bw;
	h += 2 * win->bw;

	/* resist moving windows over AvoidCover windows, but don't snap offscreen */
	for (t = Scr.ASRoot.next; t != NULL; t = t->next)
		if (t != win && ASWIN_HFLAGS(t, AS_AvoidCover))
		{
			/* check for overlap */
			int           width, height, ml, mr, mt, mb;

			get_window_geometry (t, t->flags, NULL, NULL, &width, &height);
			ml = x + w - t->frame_x;
			mr = t->frame_x + width + 2 * t->bw - x;
			mt = y + h - t->frame_y;
			mb = t->frame_y + height + 2 * t->bw - y;
			if (ml > 0 && mr > 0 && mt > 0 && mb > 0)
			{
				/* we have overlap - now find what direction we're overlapping from */
				int           kl = cx - t->frame_x;
				int           kr = t->frame_x + width + 2 * t->bw - cx;
				int           kt = cy - t->frame_y;
				int           kb = t->frame_y + height + 2 * t->bw - cy;

				if (kl <= kr && kl <= kt && kl <= kb && x - ml > 0)
					x -= ml;
				else if (kr <= kt && kr <= kb && mr + x + w < Scr.MyDisplayWidth)
					x += mr;
				else if (kt <= kb && y - mt > 0)
					y -= mt;
				else if (mb + y + h < Scr.MyDisplayHeight)
					y += mb;
			}
		}

	/* resist moving windows over the edge of the screen */
	if (x + w >= Scr.MyDisplayWidth && x + w < Scr.MyDisplayWidth + Scr.MoveResistance)
		x = Scr.MyDisplayWidth - w;
	if (x < 0 && x > -Scr.MoveResistance)
		x = 0;
	if (y + h >= Scr.MyDisplayHeight && y + h < Scr.MyDisplayHeight + Scr.MoveResistance)
		y = Scr.MyDisplayHeight - h;
	if (y < 0 && y > -Scr.MoveResistance)
		y = 0;

	*fx = x;
	*fy = y;
}

/****************************************************************************
 *
 * Move the rubberband around, return with the new window location
 *
 ****************************************************************************/
void
moveLoop (ASWindow * tmp_win, int XOffset, int YOffset, int Width,
		  int Height, int *FinalX, int *FinalY, Bool opaque_move, Bool AddWindow)
{
	Bool          finished = False;
	Bool          done;
	int           xl, yt, delta_x, delta_y;

	XQueryPointer (dpy, Scr.Root, &JunkRoot, &JunkChild, &xl, &yt, &JunkX, &JunkY, &JunkMask);
	xl += XOffset;
	yt += YOffset;

	/* fprintf (stderr, "From moveLoop () top -- tmp_win == 0x%lX \n", (long int) tmp_win); */

	if ((!opaque_move) || (AddWindow))
		MoveOutline ( /*Scr.Root, */ tmp_win, xl, yt, Width, Height);

	DisplayPosition (tmp_win, xl + Scr.Vx, yt + Scr.Vy, True);

	while (!finished)
	{
		/* block until there is an interesting event */
		XMaskEvent (dpy, ButtonPressMask | ButtonReleaseMask | KeyPressMask |
					PointerMotionMask | ButtonMotionMask | ExposureMask, &Event);
		StashEventTime (&Event);

		/* discard any extra motion events before a logical release */
		if (Event.type == MotionNotify)
		{
			while (XCheckMaskEvent (dpy, PointerMotionMask | ButtonMotionMask |
									ButtonPressMask | ButtonRelease, &Event))
			{
				StashEventTime (&Event);
				if (Event.type == ButtonRelease)
					break;
			}
		}
		done = FALSE;
		/* Handle a limited number of key press events to allow mouseless
		 * operation */
		if (Event.type == KeyPress)
			Keyboard_shortcuts (&Event, ButtonRelease, 20);
		switch (Event.type)
		{
		 case KeyPress:
			 done = TRUE;
			 break;
		 case ButtonPress:
			 XAllowEvents (dpy, ReplayPointer, CurrentTime);
			 if (Event.xbutton.button == 2)
			 {
				 NeedToResizeToo = True;
				 /* Fallthrough to button-release */
			 } else
			 {
				 done = 1;
				 break;
			 }
		 case ButtonRelease:

			 /* fprintf (stderr, "From moveLoop () case ButtonRelease: -- tmp_win == 0x%lX \n", (long int) tmp_win); */

			 if (!opaque_move)
				 MoveOutline ( /*Scr.Root, */ tmp_win, 0, 0, 0, 0);
			 xl = Event.xmotion.x_root + XOffset;
			 yt = Event.xmotion.y_root + YOffset;

			 resist_move (tmp_win, Event.xmotion.x_root, Event.xmotion.y_root, &xl, &yt);

			 *FinalX = xl;
			 *FinalY = yt;

			 done = TRUE;
			 finished = TRUE;
			 break;

		 case MotionNotify:
			 /* update location of the pager_view window */
			 xl = Event.xmotion.x_root;
			 yt = Event.xmotion.y_root;
			 HandlePaging (tmp_win, Scr.MyDisplayWidth, Scr.MyDisplayHeight, &xl, &yt,
						   &delta_x, &delta_y, False);
			 /* redraw the rubberband */
			 xl += XOffset;
			 yt += YOffset;

			 resist_move (tmp_win, Event.xmotion.x_root, Event.xmotion.y_root, &xl, &yt);


			 /* fprintf (stderr, "From moveLoop () case MotionNotify: -- tmp_win == 0x%lX \n", (long int) tmp_win); */

			 if (!opaque_move)
				 MoveOutline ( /*Scr.Root, */ tmp_win, xl, yt, Width, Height);
			 else
			 {
				 if (tmp_win->flags & ICONIFIED)
				 {
					 tmp_win->icon_p_x = xl;
					 tmp_win->icon_p_y = yt;
					 if (tmp_win->icon_pixmap_w != None)
						 XMoveWindow (dpy, tmp_win->icon_pixmap_w, xl, yt);
					 else if (tmp_win->icon_title_w != None)
						 XMoveWindow (dpy, tmp_win->icon_title_w, xl, yt + tmp_win->icon_p_height);
				 } else
					 XMoveWindow (dpy, tmp_win->frame, xl, yt);
			 }
			 DisplayPosition (tmp_win, xl + Scr.Vx, yt + Scr.Vy, False);
			 done = TRUE;
			 break;

		 default:
			 break;
		}
		if (!done)
		{

			/* fprintf (stderr, "From moveLoop () bottom -- tmp_win == 0x%lX \n", (long int) tmp_win); */

			if (!opaque_move)
				MoveOutline ( /*Scr.Root, */ tmp_win, 0, 0, 0, 0);
			DispatchEvent ();
			if (!opaque_move)
				MoveOutline ( /*Scr.Root, */ tmp_win, xl, yt, Width, Height);
		}
	}
}

/***********************************************************************
 *
 *  Procedure:
 *      DisplayPosition - display the position in the dimensions window
 *
 *  Inputs:
 *      tmp_win - the current afterstep window
 *      x, y    - position of the window
 *
 ************************************************************************/

void
DisplayPosition (ASWindow * tmp_win, int x, int y, int Init)
{
	char          str[100];
	int           offset;

	(void)sprintf (str, " %+-4d %+-4d ", x, y);

	if (Init)
	{
		GC            reliefGC, shadowGC;

		mystyle_get_global_gcs (Scr.MSFWindow, NULL, NULL, &reliefGC, &shadowGC);
		mystyle_set_window_background (Scr.SizeWindow, Scr.MSFWindow);
		XClearWindow (dpy, Scr.SizeWindow);
		if (Scr.d_depth >= 2)
			RelieveWindow (tmp_win, Scr.SizeWindow, 0, 0,
						   Scr.SizeStringWidth + SIZE_HINDENT * 2,
						   (*Scr.MSFWindow).font.height + SIZE_VINDENT * 2,
						   reliefGC, shadowGC, FULL_HILITE);
	} else
	{
		XClearArea (dpy, Scr.SizeWindow, SIZE_HINDENT, SIZE_VINDENT,
					Scr.SizeStringWidth, (*Scr.MSFWindow).font.height, False);
	}

	offset = (Scr.SizeStringWidth + SIZE_HINDENT * 2
			  - XTextWidth ((*Scr.MSFWindow).font.font, str, strlen (str))) / 2;
	mystyle_draw_text (Scr.SizeWindow, Scr.MSFWindow, str, offset,
					   (*Scr.MSFWindow).font.font->ascent + SIZE_VINDENT);
}


/****************************************************************************
 *
 * For menus, move, and resize operations, we can effect keyboard 
 * shortcuts by warping the pointer.
 *
 ****************************************************************************/
void
Keyboard_shortcuts (XEvent * Event, int ReturnEvent, int move_size)
{
	int           x, y, x_root, y_root;
	int           x_move, y_move;
	KeySym        keysym;

	/* Pick the size of the cursor movement */
	if (Event->xkey.state & ControlMask)
		move_size = 1;
	if (Event->xkey.state & ShiftMask)
		move_size = 100;

	keysym = XLookupKeysym (&Event->xkey, 0);

	x_move = 0;
	y_move = 0;
	switch (keysym)
	{
	 case XK_Up:
	 case XK_k:
	 case XK_p:
		 y_move = -move_size;
		 break;
	 case XK_Down:
	 case XK_n:
	 case XK_j:
		 y_move = move_size;
		 break;
	 case XK_Left:
	 case XK_b:
	 case XK_h:
		 x_move = -move_size;
		 break;
	 case XK_Right:
	 case XK_f:
	 case XK_l:
		 x_move = move_size;
		 break;
	 case XK_Return:
	 case XK_space:
		 /* beat up the event */
		 Event->type = ReturnEvent;
		 break;
	 default:
		 break;
	}
	XQueryPointer (dpy, Scr.Root, &JunkRoot, &Event->xany.window,
				   &x_root, &y_root, &x, &y, &JunkMask);

	if ((x_move != 0) || (y_move != 0))
	{
		/* beat up the event */
		XWarpPointer (dpy, None, Scr.Root, 0, 0, 0, 0, x_root + x_move, y_root + y_move);

		/* beat up the event */
		Event->type = MotionNotify;
		Event->xkey.x += x_move;
		Event->xkey.y += y_move;
		Event->xkey.x_root += x_move;
		Event->xkey.y_root += y_move;
	}
}


void
InteractiveMove (Window * win, ASWindow * tmp_win, int *FinalX, int *FinalY, XEvent * eventp)
{
	extern int    Stashed_X, Stashed_Y;
	int           origDragX, origDragY, DragX, DragY, DragWidth, DragHeight;
	int           XOffset, YOffset;
	Window        w;

	Bool          opaque_move = False;


	InstallRootColormap ();
	if (menuFromFrameOrWindowOrTitlebar)
	{
		/* warp the pointer to the cursor position from before menu appeared */
		XWarpPointer (dpy, None, Scr.Root, 0, 0, 0, 0, Stashed_X, Stashed_Y);
		XFlush (dpy);
	}
	DragX = eventp->xbutton.x_root;
	DragY = eventp->xbutton.y_root;
	/* If this is left commented out, then the move starts from the button press 
	 * location instead of the current location, which seems to be an
	 * improvement */
	/*  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
	   &DragX, &DragY,    &JunkX, &JunkY, &JunkMask); */

	if (!GrabEm (MOVE))
	{
		XBell (dpy, Scr.screen);
		return;
	}
	w = tmp_win->frame;

	if (tmp_win->flags & ICONIFIED)
	{
		if (tmp_win->icon_pixmap_w != None)
		{
			w = tmp_win->icon_pixmap_w;
			if (tmp_win->icon_title_w != None)
				XUnmapWindow (dpy, tmp_win->icon_title_w);
		} else
			w = tmp_win->icon_title_w;
	}

	*win = w;

	XGetGeometry (dpy, w, &JunkRoot, &origDragX, &origDragY,
				  (unsigned int *)&DragWidth, (unsigned int *)&DragHeight, &JunkBW, &JunkDepth);

	if (DragWidth * DragHeight < (Scr.OpaqueSize * Scr.MyDisplayWidth * Scr.MyDisplayHeight) / 100)
		opaque_move = True;
#if 1										   /* if we grab, windows can't update when we change desks */
	else
		XGrabServer (dpy);
#endif

	if ((!opaque_move) && (tmp_win->flags & ICONIFIED))
		XUnmapWindow (dpy, w);

	DragWidth += JunkBW;
	DragHeight += JunkBW;
	XOffset = origDragX - DragX;
	YOffset = origDragY - DragY;
	XMapRaised (dpy, Scr.SizeWindow);
	moveLoop (tmp_win, XOffset, YOffset, DragWidth, DragHeight, FinalX, FinalY, opaque_move, False);

	XUnmapWindow (dpy, Scr.SizeWindow);
	UninstallRootColormap ();

#if 1
	if (!opaque_move)
		XUngrabServer (dpy);
#endif
	UngrabEm ();


}
