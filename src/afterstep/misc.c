/*
 * Copyright (C) 1993 Rob Nation
 * Copyright (C) 1995 Bo Yang
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

/**************************************************************************
 *
 * Assorted odds and ends
 *
 **************************************************************************/


#include "../../configure.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

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
#include "../../include/loadimg.h"
#include "../../include/clientprops.h"
#include "../../include/hints.h"

#include "menus.h"

XGCValues     Globalgcv;
unsigned long Globalgcm;


/**********************************************************************/
/* window management specifics - mapping/unmapping with no events :   */
/**********************************************************************/
void
quietly_unmap_window( Window w, long event_mask )
{
    /* blocking UnmapNotify events since that may bring us into Withdrawn state */
    XSelectInput (dpy, w, event_mask & ~StructureNotifyMask);
    XUnmapWindow( dpy, w );
    XSelectInput (dpy, w, event_mask );
}

void
quietly_reparent_window( Window w, Window new_parent, int x, int y, long event_mask )
{
    /* blocking UnmapNotify events since that may bring us into Withdrawn state */
    XSelectInput (dpy, w, event_mask & ~StructureNotifyMask);
    XReparentWindow( dpy, w, (new_parent!=None)?new_parent:Scr.Root, x, y );
    XSelectInput (dpy, w, event_mask );
}

/****************************************************************************/
/* window management specifics - button ungrabbing convinience functions:   */
/****************************************************************************/
inline void
ungrab_window_buttons( Window w )
{
    XUngrabButton (dpy, AnyButton, AnyModifier, w);
}

inline void
ungrab_window_keys (Window w )
{
    XUngrabKey (dpy, AnyKey, AnyModifier, w);
}

/******************************************************************************
 * Versions of grab primitives that circumvent modifier problems
 *****************************************************************************/
void
MyXGrabButton ( unsigned button, unsigned modifiers,
                Window grab_window, Bool owner_events, unsigned event_mask,
                int pointer_mode, int keyboard_mode, Window confine_to, Cursor cursor)
{
    if( modifiers == AnyModifier )
        XGrabButton (dpy, button, AnyModifier, grab_window,
                     owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor);
    else
    {
        register int i = 0 ;
        do
        {
            XGrabButton (dpy, button, modifiers | Scr.lock_mods[i], grab_window,
                         owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor);
            if( Scr.lock_mods[i] == 0 )
                break;
            ++i ;
        }while(1);
    }
}

void
MyXUngrabButton ( unsigned button, unsigned modifiers, Window grab_window)
{
    if( modifiers == AnyModifier )
        XUngrabButton (dpy, button, AnyModifier, grab_window);
    else
    {
        register int i = 0 ;
        do
        {
            XUngrabButton (dpy, button, modifiers | Scr.lock_mods[i], grab_window);
            if( Scr.lock_mods[i] == 0 )
                break;
            ++i ;
        }while(1);
    }
}

void
grab_window_buttons (Window w, ASFlagType context_mask)
{
    register MouseButton  *MouseEntry;

    for( MouseEntry = Scr.MouseButtonRoot ; MouseEntry ; MouseEntry = MouseEntry->NextButton)
        if ( MouseEntry->func  && get_flags(MouseEntry->Context, context_mask))
		{
			if (MouseEntry->Button > 0)
                MyXGrabButton (MouseEntry->Button, MouseEntry->Modifier, w,
							   True, ButtonPressMask | ButtonReleaseMask,
							   GrabModeAsync, GrabModeAsync, None, Scr.ASCursors[DEFAULT]);
            else
			{
                register int  i = MAX_MOUSE_BUTTONS+1;
                while( --i > 0 )
                    MyXGrabButton (i, MouseEntry->Modifier, w,
								   True, ButtonPressMask | ButtonReleaseMask,
								   GrabModeAsync, GrabModeAsync, None, Scr.ASCursors[DEFAULT]);
            }
        }
	return;
}


void
grab_focus_click( Window w )
{
    int i ;
    if( w )
    { /* need to grab all buttons for window that we are about to unfocus */
        for (i = 0; i < MAX_MOUSE_BUTTONS; i++)
            if (Scr.buttons2grab & (0x01 << i))
            {
                MyXGrabButton ( i + 1, 0, w,
                                True, ButtonPressMask, GrabModeSync,
                                GrabModeAsync, None, Scr.ASCursors[SYS]);
            }
    }
}

void
ungrab_focus_click( Window w )
{
    if( w )
    {   /* if we do click to focus, remove the grab on mouse events that
         * was made to detect the focus change */
        register int i = 0;
        register ASFlagType grab_btn_mask = Scr.buttons2grab<<1 ;
        while ( ++i <= MAX_MOUSE_BUTTONS )
            if ( grab_btn_mask&(1<<i) )
                MyXUngrabButton (i, 0, w);
    }
}


/***********************************************************************
 * Key grabbing :
 ***********************************************************************/
void
grab_window_keys (Window w, ASFlagType context_mask)
{
	FuncKey      *tmp;
	for (tmp = Scr.FuncKeyRoot; tmp != NULL; tmp = tmp->next)
        if (get_flags( tmp->cont, context_mask ))
        {
            if( tmp->mods == AnyModifier )
                XGrabKey( dpy, tmp->keycode, AnyModifier, w, True, GrabModeAsync, GrabModeAsync);
            else
            {
                register int i = 0;
                do
                {/* combining modifiers with <Lock> keys,
                  * so to enable things like ScrollLock+Alt+A to work the same as Alt+A */
                    XGrabKey( dpy, tmp->keycode, tmp->mods|Scr.lock_mods[i], w, True, GrabModeAsync, GrabModeAsync);
                    if( Scr.lock_mods[i] == 0 )
                        break;
                    ++i;
                }while(1);
            }
        }
}



#if 0										   /* (see SetTimer) */
/**************************************************************************
 *
 * For auto-raising windows, this routine is called
 *
 *************************************************************************/

static void
autoraise_timer_handler (void *data)
{
	Window        child;

	XQueryPointer (dpy, Scr.Root, &JunkRoot, &child, &JunkX, &JunkY, &JunkX, &JunkY, &JunkMask);
	if (Scr.Focus != NULL && child == Scr.Focus->frame && !(Scr.Focus->flags & VISIBLE))
		RaiseWindow (Scr.Focus);
}
#endif /* 0 */

/***************************************************************************
 *
 * Start/Stops the auto-raise timer
 *
 ****************************************************************************/

void
SetTimer (int delay)
{
#if 1
	/* unfortunately, a bug in glibc-2.0.7 causes the timer version of this
	 * code to segfault; until glibc-2.0.7 is no longer in popular use, we
	 * use the old code */
#ifdef TIME_WITH_SYS_TIME
	struct itimerval value;

	value.it_value.tv_usec = 1000 * (delay % 1000);
	value.it_value.tv_sec = delay / 1000;
	value.it_interval.tv_usec = 0;
	value.it_interval.tv_sec = 0;
	setitimer (ITIMER_REAL, &value, NULL);
#endif
#else /* 1 */
	while (timer_remove_by_data (&autoraise_timer_handler));
	if (delay > 0)
		timer_new (delay, &autoraise_timer_handler, &autoraise_timer_handler);
#endif /* 1 */
}

/******************************************************************************
 *
 * Move a window to the top (dir 1) or bottom (dir -1) of the circulate seq.
 *
 *****************************************************************************/

void
SetCirculateSequence (ASWindow * tw, int dir)
{
	ASWindow     *t;
	long          best = (dir == -1) ? LONG_MAX : LONG_MIN;

	t = Scr.ASRoot.next;
	if (t)
	{
		do
		{
			if ((dir == -1) ? (t->circulate_sequence < best) : (t->circulate_sequence > best))
				best = t->circulate_sequence;
		}
		while ((t = t->next));
	} else
		best = 0;

	tw->circulate_sequence = best + dir;
}

/******************************************************************************
 *
 * Grab ClickToRaise button press events for a window
 *
 *****************************************************************************/
void
GrabRaiseClick (ASWindow * t)
{
	int           b;

	for (b = 1; b <= MAX_MOUSE_BUTTONS; b++)
	{
		if (Scr.RaiseButtons & (1 << b))
			MyXGrabButton (dpy, b, 0, t->w, True, ButtonPressMask, GrabModeSync,
						   GrabModeAsync, None, Scr.ASCursors[TITLE_CURSOR]);
	}
}

/******************************************************************************
 *
 * Ungrab ClickToRaise button press events to allow their use in applications
 *
 *****************************************************************************/
void
UngrabRaiseClick (ASWindow * t)
{
	int           b;

	for (b = 1; b <= MAX_MOUSE_BUTTONS; b++)
	{
		if (Scr.RaiseButtons & (1 << b))
			MyXUngrabButton (dpy, b, 0, t->w);
	}
}

/******************************************************************************
 *
 * Recalculate the visibility flags
 *
 *****************************************************************************/

void
UpdateVisibility (void)
{
	ASWindow     *t, *s, *tbase;

	tbase = Scr.ASRoot.next;
	for (t = Scr.ASRoot.next; t != NULL; t = t->next)
	{
		int           visible = 0;
		int           tx1, ty1, tx2, ty2;

		if (t->flags & MAPPED)
		{
			tx1 = t->frame_x;
			ty1 = t->frame_y;
			if (t->flags & SHADED)
			{
				tx2 = t->frame_x + t->title_width;
				ty2 = t->frame_y + t->title_height;
			} else
			{
				tx2 = t->frame_x + t->frame_width;
				ty2 = t->frame_y + t->frame_height;
			}
		} else if (t->flags & ICONIFIED)
		{
			tx1 = t->icon_p_x;
			ty1 = t->icon_p_y;
			tx2 = t->icon_p_x + t->icon_p_width;
			ty2 = t->icon_p_y + t->icon_p_height;
		} else
			continue;

		if ((tx2 > 0) && (tx1 < Scr.MyDisplayWidth) && (ty2 > 0) && (ty1 < Scr.MyDisplayHeight))
		{
			visible = VISIBLE;
			for (s = Scr.ASRoot.next; s != t; s = s->next)
			{
				if (get_flags(s->hints->flags, AS_Transient) && (s->hints->transient_for == t->w))
					continue;
				else if (s->status->layer != t->status->layer)
					continue;

				if (s->flags & MAPPED)
				{
					if ((tx2 > s->frame_x) && (tx1 < s->frame_x + s->frame_width) &&
						(ty2 > s->frame_y) && (ty1 < s->frame_y + s->frame_height))
					{
						visible = 0;
						break;
					}
				} else if (s->flags & ICONIFIED)
				{
					if ((tx2 > s->icon_p_x) && (tx1 < s->icon_p_x + s->icon_p_width) &&
						(ty2 > s->icon_p_y) && (ty1 < s->icon_p_y + s->icon_p_height))
					{
						visible = 0;
						break;
					}
				} else if (s->flags & SHADED)
				{
					if ((tx2 > s->frame_x) && (tx1 < s->frame_x + s->title_width) &&
						(ty2 > s->frame_y) && (ty1 < s->frame_y + s->title_height))
					{
						visible = 0;
						break;
					}
				}
			}
		}
		if ((t->flags & VISIBLE) != visible)
		{
			t->flags ^= VISIBLE;
			if ((Scr.flags & ClickToRaise) && !(Scr.flags & ClickToFocus) && (t->flags & MAPPED))
			{
				if (visible)
					UngrabRaiseClick (t);
				else
					GrabRaiseClick (t);
			}
		}
	}
}

char         *
fit_vertical_text (MyFont font, char *text, int len, int maxheight)
{
	int           savelen, i, h;
	char         *trunct;

	savelen = len;
	h = len * font.height;

	while (h > maxheight && len > 0)
	{
		len -= 1;
		h = len * font.height;
	}
	if (len <= 0)
		len = 0;
	trunct = (char *)safemalloc (len * sizeof (char) + 1);
	memset (trunct, '\0', len * sizeof (char) + 1);
	strncpy (trunct, text, len);

	if (savelen != len)
	{
		for (i = 1; i <= 3; i++)
		{
			len -= 1;
			if (len <= 0)
				break;
			trunct[len] = '.';
		}
	}
	return trunct;
}

char         *
fit_horizontal_text (MyFont font, char *text, int len, int maxwidth)
{
	int           w, savelen, i;
	char         *trunct = NULL;

	savelen = len;
	w = XTextWidth (font.font, text, len);
	while (w > maxwidth && len > 0)
	{
		len -= 1;
		w = XTextWidth (font.font, text, len);
	}
	if (len <= 0)
		len = 0;
	trunct = (char *)safemalloc (len * sizeof (char) + 1);
	memset (trunct, '\0', len * sizeof (char) + 1);
	strncpy (trunct, text, len);

	if (savelen != len)
	{
		for (i = 1; i <= 3; i++)
		{
			len -= 1;
			if (len <= 0)
				break;
			trunct[len] = '.';
		}
	}

	return trunct;
}

































































































































































