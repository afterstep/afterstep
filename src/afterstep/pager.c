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

#define LOCAL_DEBUG
#include "../../configure.h"

#include "../../include/asapp.h"
#include "../../libAfterImage/afterimage.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/event.h"
#include "../../include/decor.h"
#include "../../include/wmprops.h"

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
            register int i ;
			sleep_a_little (10000);
			total += 10;
            for( i = 0 ; i < PAN_FRAME_SIDES ; i++ )
                if( Scr.PanFrame[i].isMapped )
                    if (ASCheckWindowEvent (Scr.PanFrame[i].win, LeaveWindowMask, &(event->x)))
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

LOCAL_DEBUG_CALLER_OUT( "new(%+d%+d), old(%+d%+d), max(%+d,%+d)", newx, newy, Scr.Vx, Scr.Vy, Scr.VxMax, Scr.VyMax );

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
    check_screen_panframes(&Scr);
	if (grab)
		XUngrabServer (dpy);
#endif
}

/**************************************************************************
 * Move to a new desktop
 *************************************************************************/
Bool
change_aswindow_desk_iter_func(void *data, void *aux_data)
{
    int new_desk = (int)aux_data ;
    ASWindow *asw = (ASWindow *)data ;

    if( ASWIN_GET_FLAGS(asw, AS_Sticky) ||
        (ASWIN_GET_FLAGS(asw, AS_Iconic) && get_flags( Scr.Feel.flags, StickyIcons)) )
    {  /* Window is sticky */
        ASWIN_DESK(asw) = new_desk ;
        set_client_desktop( asw->w, new_desk );
    }else
    {
        Window dst = Scr.ServiceWin;
        if(ASWIN_DESK(asw)==new_desk)
            dst = Scr.Root;

        quietly_reparent_canvas( asw->frame_canvas, dst, AS_FRAME_EVENT_MASK, True );
        quietly_reparent_canvas( asw->icon_canvas, dst, AS_ICON_EVENT_MASK, True );
        if( asw->icon_title_canvas != asw->icon_canvas )
            quietly_reparent_canvas( asw->icon_title_canvas, dst, AS_ICON_TITLE_EVENT_MASK, True );
    }
    return True;
}


void
ChangeDesks (int new_desk)
{
    if( Scr.CurrentDesk == new_desk )
        return;

    Scr.CurrentDesk = new_desk;
    if( as_desk2ext_desk (Scr.wmprops, new_desk) == INVALID_DESKTOP_PROP )
        set_desktop_num_prop( Scr.wmprops, new_desk, Scr.Root, True );

    set_current_desk_prop ( Scr.wmprops, new_desk);

    SendPacket( -1, M_NEW_DESK, 1, new_desk);
    /* Scan the window list, mapping windows on the new Desk, unmapping
	 * windows on the old Desk; do this in reverse order to reduce client
	 * expose events */
	XGrabServer (dpy);
    iterate_asbidirlist( Scr.Windows->clients, change_aswindow_desk_iter_func, (void*)new_desk, NULL, False );
    XUngrabServer (dpy);

    if (get_flags(Scr.Feel.flags, ClickToFocus))
	{
        int i ;
        int circ_count = VECTOR_USED(*(Scr.Windows->circulate_list));
        if( !get_flags( Scr.Feel.flags, DontRestoreFocus ) )
        {
            ASWindow **circ_list = VECTOR_HEAD(ASWindow*,*(Scr.Windows->circulate_list));
            for( i =0 ; i < circ_count ; ++i )
                if( circ_list[i] != NULL && circ_list[i]->magic == MAGIC_ASWINDOW )
                    if( ASWIN_DESK(circ_list[i]) == new_desk )
                    {
                        focus_aswindow( circ_list[i], False );
                        break;
                    }
        }else
            i = circ_count ;

        if( i >= circ_count )
            hide_focus();
    }

    restack_window_list( new_desk );

    /* Change the look to this desktop's one if it really changed */
#ifdef DIFFERENTLOOKNFEELFOREACHDESKTOP
	QuickRestart ("look&feel");
#endif

    /*TODO: implement virtual desktops switching : */
#if 0
    /* autoplace sticky icons so they don't wind up over a stationary icon */
	AutoPlaceStickyIcons ();


	CorrectStackOrder ();
	update_windowList ();
#endif

}

