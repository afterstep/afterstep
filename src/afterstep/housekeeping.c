/****************************************************************************
 * Copyright (c) 2000,2001 Sasha Vasko <sasha at aftercode.net>
 * Copyright (c) 1999 Ethan Fisher <allanon@crystaltokyo.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/
/***********************************************************************
 * assorted houskeeping code
 ***********************************************************************/

#include "../../configure.h"

#include "../../include/asapp.h"
#include <limits.h>
#include <signal.h>
#include <unistd.h>

#include "../../include/afterstep.h"
#include "../../include/screen.h"
#include "asinternals.h"

/*****************************************************************************
 * Grab the pointer and keyboard
 ****************************************************************************/

static ScreenInfo *grabbed_screen = NULL;
static ASWindow   *grabbed_screen_focus = NULL;
static int         grabbed_depth = 0 ;

Bool
GrabEm (ScreenInfo *scr, Cursor cursor)
{
	int           i = 0;
	unsigned int  mask;
    int res ;

	XSync (dpy, 0);
    if( get_flags(AfterStepState, ASS_HousekeepingMode) )  /* check if we already grabbed everything */
    {
        ++grabbed_depth ;
        return True ;
    }
    set_flags(AfterStepState, ASS_HousekeepingMode);
    grabbed_depth = 1 ;
	/* move the keyboard focus prior to grabbing the pointer to
	 * eliminate the enterNotify and exitNotify events that go
	 * to the windows */
	grabbed_screen = scr ;
    grabbed_screen_focus = scr->Windows->focused ;
    hide_focus();

    mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
           PointerMotionMask | EnterWindowMask | LeaveWindowMask ;
    while ( (res = XGrabPointer (dpy, scr->Root, False, mask, GrabModeAsync,
                                 GrabModeAsync, scr->Root, cursor,
                                 CurrentTime)) != GrabSuccess )
	{
        if( i++ >= 1000 )
        {
#define MAX_GRAB_ERROR 4
            static char *_as_grab_error_code[MAX_GRAB_ERROR+1+1] =
            {
                "Grab Success",
                "pointer is actively grabbed by some other client",
                "the specified time is earlier than the last-pointer-grab time or later than the current X server time",
                "window is not viewable or lies completely outside the boundaries of the root window",
                "pointer is frozen by an active grab of another client",
                "I'm totally messed up - restart me please"
            };
            char *error_text = _as_grab_error_code[MAX_GRAB_ERROR+1];
            if( res <= MAX_GRAB_ERROR )
                error_text = _as_grab_error_code[res];

            show_warning( "Failed to grab pointer for requested interactive operation.(X server says:\"%s\")", error_text );
            clear_flags(AfterStepState, ASS_HousekeepingMode);
            return False;
        }
		/* If you go too fast, other windows may not get a change to release
		 * any grab that they have. */
        sleep_a_little (1000);
        XSync (dpy, 0);
    }
	return True;
}

/*****************************************************************************
 * UnGrab the pointer and keyboard
 ****************************************************************************/
void
UngrabEm ()
{
    if( get_flags(AfterStepState, ASS_HousekeepingMode) && grabbed_screen )  /* check if we grabbed everything */
    {
        if( --grabbed_depth > 0 )
            return;
        XSync (dpy, 0);
        XUngrabPointer (dpy, CurrentTime);

	    clear_flags(AfterStepState, ASS_HousekeepingMode);
        grabbed_depth = 0 ;

        if (grabbed_screen_focus != NULL)
        {
            focus_aswindow(grabbed_screen_focus, False);
            grabbed_screen_focus = NULL ;
        }
        XSync (dpy, 0);
		grabbed_screen = NULL;
    }
}

static ScreenInfo *warping_screen = NULL;

Bool
StartWarping(ScreenInfo *scr, Cursor cursor)
{
    if( get_flags(AfterStepState, ASS_WarpingMode) )
        return True ;
    set_flags(AfterStepState, ASS_WarpingMode);
	warping_screen = scr;
    return GrabEm(scr, cursor);
}

void
EndWarping()
{
    if( get_flags(AfterStepState, ASS_WarpingMode) && warping_screen )
    {
        clear_flags(AfterStepState, ASS_WarpingMode);
        if( warping_screen->Windows->hilited != warping_screen->Windows->active )
            activate_aswindow( warping_screen->Windows->hilited, False, False );
        UngrabEm();
		warping_screen = NULL;
    }
}

/****************************************************************************
 * paste the cut buffer into the window with the input focus
 ***************************************************************************/
void
PasteSelection (ScreenInfo *scr)
{
    if (scr->Windows->focused != NULL)
	{
		int           length;
		int           buffer = 0;
		char         *buf;

		buf = XFetchBuffer (dpy, &length, buffer);

/* everything from here on is a kludge; if you know a better way to do it,
 * please fix it!
 */
		if (buf != NULL && length > 0)
		{
			int           i;
			XEvent        event;
            Window        w = scr->Windows->focused->w ;

			event.xkey.display = dpy;
			event.xkey.window = w;
			event.xkey.time = CurrentTime;
			event.xkey.same_screen = True;
			XQueryPointer (dpy, w, &event.xkey.root,
						   &event.xkey.subwindow, &event.xkey.x_root,
						   &event.xkey.y_root, &event.xkey.x, &event.xkey.y, &event.xkey.state);
			for (i = 0; i < length; i++)
			{
				event.xkey.state &= ~(ShiftMask | ControlMask);
				if (*buf < ' ')
				{
					char          ch = *buf;

					/* this code assumes an ASCII character mapping */
					/* need to translate newline to carriage-return */
					if (ch == '\n')
						ch = '\r';
					event.xkey.keycode = XKeysymToKeycode (dpy, ch + '@');
					event.xkey.state |= ControlMask;
				} else if (isupper (*buf))
				{
					event.xkey.keycode = XKeysymToKeycode (dpy, *buf);
					event.xkey.state |= ShiftMask;
				} else
					event.xkey.keycode = XKeysymToKeycode (dpy, *buf);
				if (event.xkey.keycode != NoSymbol)
				{
					event.type = KeyPress;
					XSendEvent (dpy, w, False, KeyPressMask, &event);
					event.type = KeyRelease;
					XSendEvent (dpy, w, False, KeyReleaseMask, &event);
				}
				buf++;
			}
		}
	}
}

