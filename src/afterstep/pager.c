/*
 * Copyright (c) 1999 Ethan Fischer <allanon@crystaltokyo.com>
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
 * afterstep pager handling code
 *
 ***********************************************************************/

#include "../../configure.h"

#include "../../include/asapp.h"
#include "../../libAfterImage/afterimage.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/event.h"

#include "asinternals.h"

/***************************************************************************
 *
 * Check to see if the pointer is on the edge of the screen, and scroll/page
 * if needed
 ***************************************************************************/
void
HandlePaging (int HorWarpSize, int VertWarpSize, int *xl, int *yt,
              int *delta_x, int *delta_y, Bool Grab, ASEvent *event)
{
#ifndef NO_VIRTUAL
	int           x, y, total;
#endif
    Window wdumm;
    int    dumm;
    unsigned int udumm;

	*delta_x = 0;
	*delta_y = 0;

#ifndef NO_VIRTUAL
	if (DoHandlePageing)
	{
        if ((Scr.Feel.EdgeResistanceScroll >= 10000) || ((HorWarpSize == 0) && (VertWarpSize == 0)))
			return;

		/* need to move the viewport */
		if ((*xl >= SCROLL_REGION) && (*xl < Scr.MyDisplayWidth - SCROLL_REGION) &&
			(*yt >= SCROLL_REGION) && (*yt < Scr.MyDisplayHeight - SCROLL_REGION))
			return;

		total = 0;
        while (total < Scr.Feel.EdgeResistanceScroll)
		{
			sleep_a_little (10000);
			total += 10;
            if (ASCheckWindowEvent (Scr.PanFrameTop.win, LeaveWindowMask, &(event->x)))
				return;
            if (ASCheckWindowEvent (Scr.PanFrameBottom.win, LeaveWindowMask, &(event->x)))
                return;
            if (ASCheckWindowEvent (Scr.PanFrameLeft.win, LeaveWindowMask, &(event->x)))
				return;
            if (ASCheckWindowEvent (Scr.PanFrameRight.win, LeaveWindowMask, &(event->x)))
				return;
		}

        XQueryPointer (dpy, Scr.Root, &wdumm, &wdumm, &x, &y, &dumm, &dumm, &udumm);

		/* fprintf (stderr, "-------- MoveOutline () called from pager.c\ntmp_win == 0xlX\n", (long int) tmp_win); */
		/* Turn off the rubberband if its on */
//        MoveOutline ( /*Scr.Root, */ tmp_win, 0, 0, 0, 0);

		/* Move the viewport */
		/* and/or move the cursor back to the approximate correct location */
		/* that is, the same place on the virtual desktop that it */
		/* started at */
		if (x < SCROLL_REGION)
			*delta_x = -HorWarpSize;
		else if (x >= Scr.MyDisplayWidth - SCROLL_REGION)
			*delta_x = HorWarpSize;
		else
			*delta_x = 0;
		if (y < SCROLL_REGION)
			*delta_y = -VertWarpSize;
		else if (y >= Scr.MyDisplayHeight - SCROLL_REGION)
			*delta_y = VertWarpSize;
		else
			*delta_y = 0;

		/* Ouch! lots of bounds checking */
		if (Scr.Vx + *delta_x < 0)
		{
            if (!get_flags(Scr.Feel.flags, EdgeWrapX))
			{
				*delta_x = -Scr.Vx;
				*xl = x - *delta_x;
			} else
			{
				*delta_x += Scr.VxMax + Scr.MyDisplayWidth;
				*xl = x + *delta_x % Scr.MyDisplayWidth + HorWarpSize;
			}
		} else if (Scr.Vx + *delta_x > Scr.VxMax)
		{
            if (!get_flags(Scr.Feel.flags, EdgeWrapX))
			{
				*delta_x = Scr.VxMax - Scr.Vx;
				*xl = x - *delta_x;
			} else
			{
				*delta_x -= Scr.VxMax + Scr.MyDisplayWidth;
				*xl = x + *delta_x % Scr.MyDisplayWidth - HorWarpSize;
			}
		} else
			*xl = x - *delta_x;

		if (Scr.Vy + *delta_y < 0)
		{
            if (!get_flags(Scr.Feel.flags, EdgeWrapY))
			{
				*delta_y = -Scr.Vy;
				*yt = y - *delta_y;
			} else
			{
				*delta_y += Scr.VyMax + Scr.MyDisplayHeight;
				*yt = y + *delta_y % Scr.MyDisplayHeight + VertWarpSize;
			}
		} else if (Scr.Vy + *delta_y > Scr.VyMax)
		{
            if (!get_flags(Scr.Feel.flags, EdgeWrapY))
			{
				*delta_y = Scr.VyMax - Scr.Vy;
				*yt = y - *delta_y;
			} else
			{
				*delta_y -= Scr.VyMax + Scr.MyDisplayHeight;
				*yt = y + *delta_y % Scr.MyDisplayHeight - VertWarpSize;
			}
		} else
			*yt = y - *delta_y;

		if (*xl <= SCROLL_REGION)
			*xl = SCROLL_REGION + 1;
		if (*yt <= SCROLL_REGION)
			*yt = SCROLL_REGION + 1;
		if (*xl >= Scr.MyDisplayWidth - SCROLL_REGION)
			*xl = Scr.MyDisplayWidth - SCROLL_REGION - 1;
		if (*yt >= Scr.MyDisplayHeight - SCROLL_REGION)
			*yt = Scr.MyDisplayHeight - SCROLL_REGION - 1;

		if ((*delta_x != 0) || (*delta_y != 0))
		{
			if (Grab)
				XGrabServer (dpy);
			XWarpPointer (dpy, None, Scr.Root, 0, 0, 0, 0, *xl, *yt);
			MoveViewport (Scr.Vx + *delta_x, Scr.Vy + *delta_y, False);
            XQueryPointer (dpy, Scr.Root, &wdumm, &wdumm, xl, yt, &dumm, &dumm, &udumm);
			if (Grab)
				XUngrabServer (dpy);
		}
	}
#endif
}

/***************************************************************************
 *
 *  Moves the viewport within the virtual desktop
 *
 ***************************************************************************/

Bool
viewport_aswindow_iter_func( void *data, void *aux_data )
{
    ASWindow *asw = (ASWindow*)data ;
    if( asw )
    {
        asw->status->viewport_x = Scr.Vx ;
        asw->status->viewport_y = Scr.Vy ;
        on_window_status_changed( asw, True, True );
    }
    return True;
}


void
MoveViewport (int newx, int newy, Bool grab)
{
#ifndef NO_VIRTUAL
	int           deltax, deltay;

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
    SendPacket( -1, M_NEW_PAGE, 3, Scr.Vx, Scr.Vy, Scr.CurrentDesk);

	if (deltax || deltay)
	{

        /* traverse window list and redo the titlebar/buttons if necessary */
        iterate_asbidirlist( Scr.Windows->clients, viewport_aswindow_iter_func, NULL, NULL, False );

#if 0                                          /* old cruft : */
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
				int           x, y, w, h;

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
							XMoveWindow (dpy, t->icon_pixmap_w, t->icon_p_x, t->icon_p_y);
						if (t->icon_title_w != None)
							XMoveWindow (dpy, t->icon_title_w, t->icon_p_x,
										 t->icon_p_y + t->icon_p_height);
						if (!(t->flags & ICON_UNMAPPED))
							Broadcast (M_ICON_LOCATION, 7, t->w, t->frame,
									   (unsigned long)t,
									   t->icon_p_x, t->icon_p_y, t->icon_p_width, t->icon_p_height);
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
								XMoveWindow (dpy, t->icon_pixmap_w, t->icon_p_x, t->icon_p_y);
							if (t->icon_title_w != None)
								XMoveWindow (dpy, t->icon_title_w, t->icon_p_x,
											 t->icon_p_y + t->icon_p_height);
							if (!(t->flags & ICON_UNMAPPED))
								Broadcast (M_ICON_LOCATION, 7, t->w, t->frame,
										   (unsigned long)t,
										   t->icon_p_x, t->icon_p_y,
										   t->icon_p_width, t->icon_p_height);
						}
					}
				}
            }
#endif
        /* TODO: autoplace sticky icons so they don't wind up over a stationary icon */
    }
    CheckPanFrames ();
	if (grab)
		XUngrabServer (dpy);
#endif
}

/**************************************************************************
 * Move to a new desktop
 *************************************************************************/
void
ChangeDesks (int new_desk)
{
    /*TODO: implement virtual desktops switching : */
#if 0
    int           oldDesk;
	ASWindow     *t;
	ASWindow     *FocusWin = 0;
	static ASWindow *StickyWin = 0;
	unsigned long data;
	extern Atom   _XA_WIN_DESK;

	oldDesk = Scr.CurrentDesk;

	if ((val1 != 0) && (val1 != 10000))
		Scr.CurrentDesk = Scr.CurrentDesk + val1;
	else
		Scr.CurrentDesk = val2;

	/* update property to tell us what desk we were on when we restart;
	 * always do this so that when we get called from main(), the property
	 * will be set; this property is what we use to determine if we're
	 * starting up for the first time, or restarting */
	data = (unsigned long)Scr.CurrentDesk;
	XChangeProperty (dpy, Scr.Root, _XA_WIN_DESK, XA_CARDINAL, 32,
					 PropModeReplace, (unsigned char *)&data, 1);

	if (Scr.CurrentDesk == oldDesk)
		return;

    SendPacket( -1, M_NEW_DESK, 1, Scr.CurrentDesk);

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
			if (ASWIN_DESK(t) == oldDesk)
			{
				if (Scr.Focus == t)
					t->FocusDesk = oldDesk;
				else
					t->FocusDesk = -1;
				UnmapIt (t);
			} else if (ASWIN_DESK(t) == Scr.CurrentDesk)
			{
				MapIt (t);
				if (t->FocusDesk == Scr.CurrentDesk)
				{
					FocusWin = t;
				}
			}
		} else
		{
			/* Window is sticky */
			ASWIN_DESK(t) = Scr.CurrentDesk ;
			set_client_desktop( t->w, Scr.CurrentDesk );
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
#endif
}



#ifndef NO_VIRTUAL
/* the root window is surrounded by four window slices, which are InputOnly.
 * So you can see 'through' them, but they eat the input. An EnterEvent in
 * one of these windows causes a Paging. The windows have the according cursor
 * pointing in the pan direction or are hidden if there is no more panning
 * in that direction. This is mostly intended to get a panning even atop
 * of Motif applictions, which does not work yet. It seems Motif windows
 * eat all mouse events.
 *
 * Hermann Dunkel, HEDU, dunkel@cul-ipn.uni-kiel.de 1/94
 */

/***************************************************************************
 * checkPanFrames hides PanFrames if they are on the very border of the
 * VIRTUELL screen and EdgeWrap for that direction is off.
 * (A special cursor for the EdgeWrap border could be nice) HEDU
 ****************************************************************************/
void
CheckPanFrames (void)
{
    int           wrapX = get_flags(Scr.Feel.flags, EdgeWrapX);
    int           wrapY = get_flags(Scr.Feel.flags, EdgeWrapY);

	/* Remove Pan frames if paging by edge-scroll is permanently or
	 * temporarily disabled */
    if ((Scr.Feel.EdgeScrollY == 0) || !get_flags(Scr.Feel.flags, DoHandlePageing))
	{
		XUnmapWindow (dpy, Scr.PanFrameTop.win);
		Scr.PanFrameTop.isMapped = False;
		XUnmapWindow (dpy, Scr.PanFrameBottom.win);
		Scr.PanFrameBottom.isMapped = False;
	}
    if ((Scr.Feel.EdgeScrollX == 0) || !get_flags(Scr.Feel.flags, DoHandlePageing))
	{
		XUnmapWindow (dpy, Scr.PanFrameLeft.win);
		Scr.PanFrameLeft.isMapped = False;
		XUnmapWindow (dpy, Scr.PanFrameRight.win);
		Scr.PanFrameRight.isMapped = False;
	}
    if (((Scr.Feel.EdgeScrollX == 0) && (Scr.Feel.EdgeScrollY == 0)) ||
        !get_flags(Scr.Feel.flags, DoHandlePageing))
		return;

	/* LEFT, hide only if EdgeWrap is off */
	if (Scr.Vx == 0 && Scr.PanFrameLeft.isMapped && (!wrapX))
	{
		XUnmapWindow (dpy, Scr.PanFrameLeft.win);
		Scr.PanFrameLeft.isMapped = False;
	} else if (Scr.Vx > 0 && Scr.PanFrameLeft.isMapped == False)
	{
		XMapRaised (dpy, Scr.PanFrameLeft.win);
		Scr.PanFrameLeft.isMapped = True;
	}
	/* RIGHT, hide only if EdgeWrap is off */
	if (Scr.Vx == Scr.VxMax && Scr.PanFrameRight.isMapped && (!wrapX))
	{
		XUnmapWindow (dpy, Scr.PanFrameRight.win);
		Scr.PanFrameRight.isMapped = False;
	} else if (Scr.Vx < Scr.VxMax && Scr.PanFrameRight.isMapped == False)
	{
		XMapRaised (dpy, Scr.PanFrameRight.win);
		Scr.PanFrameRight.isMapped = True;
	}
	/* TOP, hide only if EdgeWrap is off */
	if (Scr.Vy == 0 && Scr.PanFrameTop.isMapped && (!wrapY))
	{
		XUnmapWindow (dpy, Scr.PanFrameTop.win);
		Scr.PanFrameTop.isMapped = False;
	} else if (Scr.Vy > 0 && Scr.PanFrameTop.isMapped == False)
	{
		XMapRaised (dpy, Scr.PanFrameTop.win);
		Scr.PanFrameTop.isMapped = True;
	}
	/* BOTTOM, hide only if EdgeWrap is off */
	if (Scr.Vy == Scr.VyMax && Scr.PanFrameBottom.isMapped && (!wrapY))
	{
		XUnmapWindow (dpy, Scr.PanFrameBottom.win);
		Scr.PanFrameBottom.isMapped = False;
	} else if (Scr.Vy < Scr.VyMax && Scr.PanFrameBottom.isMapped == False)
	{
		XMapRaised (dpy, Scr.PanFrameBottom.win);
		Scr.PanFrameBottom.isMapped = True;
	}
}

/****************************************************************************
 *
 * Gotta make sure these things are on top of everything else, or they
 * don't work!
 *
 ***************************************************************************/
void
RaisePanFrames (void)
{
	if (Scr.PanFrameTop.isMapped)
		XRaiseWindow (dpy, Scr.PanFrameTop.win);
	if (Scr.PanFrameLeft.isMapped)
		XRaiseWindow (dpy, Scr.PanFrameLeft.win);
	if (Scr.PanFrameRight.isMapped)
		XRaiseWindow (dpy, Scr.PanFrameRight.win);
	if (Scr.PanFrameBottom.isMapped)
		XRaiseWindow (dpy, Scr.PanFrameBottom.win);
}

/****************************************************************************
 *
 * Creates the windows for edge-scrolling
 *
 ****************************************************************************/
void
InitPanFrames ()
{
	XSetWindowAttributes attributes;		   /* attributes for create */
	unsigned long valuemask;

	attributes.event_mask = (EnterWindowMask | LeaveWindowMask | VisibilityChangeMask);
	valuemask = (CWEventMask | CWCursor);

    attributes.cursor = Scr.Feel.cursors[TOP];
	Scr.PanFrameTop.win = create_visual_window (Scr.asv, Scr.Root, 0, 0, Scr.MyDisplayWidth, PAN_FRAME_THICKNESS, 0,	/* no border */
												InputOnly, valuemask, &attributes);
    attributes.cursor = Scr.Feel.cursors[LEFT];
	Scr.PanFrameLeft.win = create_visual_window (Scr.asv, Scr.Root, 0, PAN_FRAME_THICKNESS, PAN_FRAME_THICKNESS, Scr.MyDisplayHeight - 2 * PAN_FRAME_THICKNESS, 0,	/* no border */
												 InputOnly, valuemask, &attributes);
    attributes.cursor = Scr.Feel.cursors[RIGHT];
	Scr.PanFrameRight.win = create_visual_window (Scr.asv, Scr.Root, Scr.MyDisplayWidth - PAN_FRAME_THICKNESS, PAN_FRAME_THICKNESS, PAN_FRAME_THICKNESS, Scr.MyDisplayHeight - 2 * PAN_FRAME_THICKNESS, 0,	/* no border */
												  InputOnly, valuemask, &attributes);
    attributes.cursor = Scr.Feel.cursors[BOTTOM];
	Scr.PanFrameBottom.win = create_visual_window (Scr.asv, Scr.Root, 0, Scr.MyDisplayHeight - PAN_FRAME_THICKNESS, Scr.MyDisplayWidth, PAN_FRAME_THICKNESS, 0,	/* no border */
												   InputOnly, valuemask, &attributes);
	Scr.PanFrameTop.isMapped = Scr.PanFrameLeft.isMapped =
		Scr.PanFrameRight.isMapped = Scr.PanFrameBottom.isMapped = False;

	Scr.usePanFrames = True;

}

#endif /* NO_VIRTUAL */

