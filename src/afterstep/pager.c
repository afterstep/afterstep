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

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"

#include "menus.h"

extern XEvent Event;

XGCValues     Globalgcv;
unsigned long Globalgcm;

void          DrawPartitionLines (void);
ASWindow     *FindCounterpart (Window target);
Bool          pagerOn = True;
Bool          EnablePagerRedraw = True;
Bool          DoHandlePageing = True;

int
highest_layer (ASWindow * list)
{
	int           highest = -10000;

	for (; list != NULL; list = list->next)
		if (highest < list->status->layer)
			highest = list->status->layer;
	return highest;
}

int
highest_layer_below_window (ASWindow * list, ASWindow * w)
{
	int           highest = -10000;

	for (; list != NULL; list = list->next)
		if (highest < list->status->layer && list->status->layer < w->status->layer)
			highest = list->status->layer;
	return highest == -10000 ? w->status->layer : highest;
}

ASWindow     *
list_prepend (ASWindow * list1, ASWindow * list2)
{
	ASWindow     *tmp = list2->prev;

	if (list1 == NULL)
		return list2;
	if (list2 == NULL)
		return list1;
	list2->prev->next = list1;
	list1->prev->next = list2;
	list2->prev = list1->prev;
	list1->prev = tmp;
	return list2;
}

ASWindow     *
list_append (ASWindow * list1, ASWindow * list2)
{
	if (list1 == NULL)
		return list2;
	/* check for circular list */
	if (list1->prev != NULL)
		list_prepend (list1, list2);
	else
	{
		ASWindow     *ptr;

		for (ptr = list1; ptr->next != NULL; ptr = ptr->next);
		ptr->next = list2;
		list2->prev->next = NULL;
		list2->prev = ptr;
	}
	return list1;
}

ASWindow     *
list_extract (ASWindow * w)
{
	if (w->prev != NULL)
		w->prev->next = w->next;
	if (w->next != NULL)
		w->next->prev = w->prev;
	w->next = w->prev = w;
	return w;
}

/* count the windows in the list */
int
list_count_windows (ASWindow * list)
{
	int           count = 0;
	ASWindow     *w;

	for (w = list; w != NULL; w = w->next)
	{
		if ((w->flags & ICONIFIED) && !(w->flags & SUPPRESSICON))
		{
			if (w->icon_pixmap_w != None)
				count++;
			if (w->icon_title_w != None)
				count++;
		}
		count++;
		if (w->next == list)
			break;
	}
	return count;
}

#if 0
void
list_print (ASWindow * list)
{
	ASWindow     *ptr;

	for (ptr = list; ptr != NULL; ptr = ptr->next)
	{
		fprintf (stderr, "%d : '%s'", ptr->layer, ptr->name);
		fprintf (stderr, " (%sfully visible)", (ptr->flags & VISIBLE) ? "" : "not ");
		fprintf (stderr, "\n");
		if (ptr->prev->next != ptr)
			fprintf (stderr, "INCONSISTENCY 1\n");
		if (ptr->next != NULL && ptr->next->prev != ptr)
			fprintf (stderr, "INCONSISTENCY 2\n");
		if (ptr->next == list)
			break;
	}
}
#endif

void
RaiseWindow (ASWindow * t)
{
	ASWindow     *list, *w, *wn;
	int           highest, count;
	Window       *wins;
	MenuRoot     *menu;

	SetTimer (0);

	/* collect all windows which go above us */
	list = NULL;
	while ((highest = highest_layer (Scr.ASRoot.next)) > t->status->layer)
		for (w = Scr.ASRoot.next; w != NULL; w = wn)
		{
			wn = w->next;
			if (w->status->layer == highest)
				list = list_append (list, list_extract (w));
		}

	/* next, any transients for our window */
#ifndef DONT_RAISE_TRANSIENTS
	for (w = Scr.ASRoot.next; w != NULL; w = wn)
	{
		wn = w->next;
		if (get_flags(w->hints->flags, AS_Transient) && w->hints->transient_for == t->w)
			list = list_append (list, list_extract (w));
	}
#endif /* !DONT_RAISE_TRANSIENTS */

	/* next, our window */
	list = list_append (list, list_extract (t));

	/* count windows to raise */
	count = 0;

	/* menus always go on top */
	for (menu = Scr.first_menu; menu != NULL; menu = (*menu).next)
		if ((*menu).is_mapped == True)
			count++;

	/* count the windows in the list */
	count += list_count_windows (list);

	wins = (Window *) safemalloc (count * sizeof (Window));
	count = 0;

	/* menus always go on top */
	for (menu = Scr.first_menu; menu != NULL; menu = (*menu).next)
		if ((*menu).is_mapped == True)
			wins[count++] = (*menu).w;

	/* next, the windows in the list */
	for (w = list; w != NULL; w = w->next)
	{
		wins[count++] = w->frame;
		if ((w->flags & ICONIFIED) && !(w->flags & SUPPRESSICON))
		{
			if (w->icon_pixmap_w != None)
				wins[count++] = w->icon_pixmap_w;
			if (w->icon_title_w != None)
				wins[count++] = w->icon_title_w;
		}
		if (w->next == list)
			break;
	}

	/* put the windows back in the window list */
	if (Scr.ASRoot.next == NULL && list != NULL)
	{
		list->prev->next = NULL;
		Scr.ASRoot.next = list;
		list->prev = &Scr.ASRoot;
	} else
		list_prepend (Scr.ASRoot.next, list);

	Broadcast (M_RAISE_WINDOW, 3, t->w, t->frame, (unsigned long)t);

	/* raise the windows! */
	XRaiseWindow (dpy, wins[0]);
	XRestackWindows (dpy, wins, count);
	free (wins);

#ifndef NO_VIRTUAL
	raisePanFrames ();
#endif
	UpdateVisibility ();
}

void
LowerWindow (ASWindow * t)
{
	ASWindow     *list = NULL, *w, *wn;
	int           highest, count;
	Window       *wins;

	SetTimer (0);

	/* first, any transients for our window */
#ifndef DONT_RAISE_TRANSIENTS
	for (w = Scr.ASRoot.next; w != NULL; w = wn)
	{
		wn = w->next;
		if (get_flags(w->hints->flags, AS_Transient) && w->hints->transient_for == t->w)
			list = list_append (list, list_extract (w));
	}
#endif /* !DONT_RAISE_TRANSIENTS */

	/* next, our window */
	list = list_append (list, list_extract (t));

	/* next, any windows which go below us */
	while ((highest = highest_layer_below_window (Scr.ASRoot.next, t)) < t->status->layer)
		for (w = Scr.ASRoot.next; w != NULL; w = wn)
		{
			wn = w->next;
			if (w->status->layer == highest)
				list = list_append (list, list_extract (w));
		}

	/* prepend the last window in Scr.ASRoot (if any) */
	if (Scr.ASRoot.next != NULL)
	{
		for (w = Scr.ASRoot.next; w->next != NULL; w = w->next);
		list = list_prepend (list, list_extract (w));
	}

	/* count windows to raise */
	count = list_count_windows (list);

	wins = (Window *) safemalloc (count * sizeof (Window));
	count = 0;

	/* add the windows in the list */
	for (w = list; w != NULL; w = w->next)
	{
		wins[count++] = w->frame;
		if ((w->flags & ICONIFIED) && !(w->flags & SUPPRESSICON))
		{
			if (w->icon_pixmap_w != None)
				wins[count++] = w->icon_pixmap_w;
			if (w->icon_title_w != None)
				wins[count++] = w->icon_title_w;
		}
		if (w->next == list)
			break;
	}

	/* put the windows back in the window list */
	list_append (&Scr.ASRoot, list);

	Broadcast (M_LOWER_WINDOW, 3, t->w, t->frame, (unsigned long)t);

	/* restack the windows! */
	XRestackWindows (dpy, wins, count);
	free (wins);

	UpdateVisibility ();
}


/******************************************************************************
 *
 * Get the correct window stacking order from the X server and
 * make sure everything that depends on the order is fine and dandy
 *
 *****************************************************************************/

void
CorrectStackOrder (void)
{
	Window        root, parent, *children, *wins;
	ASWindow     *list, *w;
	unsigned int  nchildren;
	int           highest, count;

	if (XQueryTree (dpy, Scr.ASRoot.w, &root, &parent, &children, &nchildren))
	{
		Window       *cp;
		ASWindow     *t;

		for (cp = children; nchildren-- > 0; cp++)
            if (t = window2aswindow( *cp)) && t->frame == *cp)
				list_prepend (Scr.ASRoot.next, list_extract (t));
		XFree (children);
	} else
	{
		fprintf (stderr, "CorrectStackOrder(): XQueryTree failed!\n");
	}

	/* reorder the windows in layer order */
	list = NULL;
	while ((highest = highest_layer (Scr.ASRoot.next)) > -10000)
	{
		ASWindow     *wn;

		for (w = Scr.ASRoot.next; w != NULL; w = wn)
		{
			wn = w->next;
			if (w->status->layer == highest)
				list = list_append (list, list_extract (w));
		}
	}

	/* done if there are no windows to restack */
	if (list == NULL)
		return;

	/* count windows */
	count = list_count_windows (list);

	wins = (Window *) safemalloc (count * sizeof (Window));
	count = 0;

	/* add the windows in the list */
	for (w = list; w != NULL; w = w->next)
	{
		wins[count++] = w->frame;
		if ((w->flags & ICONIFIED) && !(w->flags & SUPPRESSICON))
		{
			if (w->icon_pixmap_w != None)
				wins[count++] = w->icon_pixmap_w;
			if (w->icon_title_w != None)
				wins[count++] = w->icon_title_w;
		}
		if (w->next == list)
			break;
	}

	/* put the windows back in the window list */
	list_append (&Scr.ASRoot, list);

	/* raise the windows! */
	XRestackWindows (dpy, wins, count);
	free (wins);

#ifndef NO_VIRTUAL
	raisePanFrames ();
#endif
	UpdateVisibility ();
}

/***************************************************************************
 *
 * Check to see if the pointer is on the edge of the screen, and scroll/page
 * if needed
 ***************************************************************************/
void
HandlePaging (ASWindow * tmp_win, int HorWarpSize, int VertWarpSize, int *xl, int *yt,
			  int *delta_x, int *delta_y, Bool Grab)
{
#ifndef NO_VIRTUAL
	int           x, y, total;
#endif

	*delta_x = 0;
	*delta_y = 0;

#ifndef NO_VIRTUAL
	if (DoHandlePageing)
	{
		if ((Scr.ScrollResistance >= 10000) || ((HorWarpSize == 0) && (VertWarpSize == 0)))
			return;

		/* need to move the viewport */
		if ((*xl >= SCROLL_REGION) && (*xl < Scr.MyDisplayWidth - SCROLL_REGION) &&
			(*yt >= SCROLL_REGION) && (*yt < Scr.MyDisplayHeight - SCROLL_REGION))
			return;

		total = 0;
		while (total < Scr.ScrollResistance)
		{
			sleep_a_little (10000);
			total += 10;
			if (XCheckWindowEvent (dpy, Scr.PanFrameTop.win, LeaveWindowMask, &Event))
			{
				StashEventTime (&Event);
				return;
			}
			if (XCheckWindowEvent (dpy, Scr.PanFrameBottom.win, LeaveWindowMask, &Event))
			{
				StashEventTime (&Event);
				return;
			}
			if (XCheckWindowEvent (dpy, Scr.PanFrameLeft.win, LeaveWindowMask, &Event))
			{
				StashEventTime (&Event);
				return;
			}
			if (XCheckWindowEvent (dpy, Scr.PanFrameRight.win, LeaveWindowMask, &Event))
			{
				StashEventTime (&Event);
				return;
			}
		}

		XQueryPointer (dpy, Scr.Root, &JunkRoot, &JunkChild, &x, &y, &JunkX, &JunkY, &JunkMask);

		/* fprintf (stderr, "-------- MoveOutline () called from pager.c\ntmp_win == 0xlX\n", (long int) tmp_win); */
		/* Turn off the rubberband if its on */
		MoveOutline ( /*Scr.Root, */ tmp_win, 0, 0, 0, 0);

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
			if (!(Scr.flags & EdgeWrapX))
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
			if (!(Scr.flags & EdgeWrapX))
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
			if (!(Scr.flags & EdgeWrapY))
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
			if (!(Scr.flags & EdgeWrapY))
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
			XQueryPointer (dpy, Scr.Root, &JunkRoot, &JunkChild, xl, yt, &JunkX, &JunkY, &JunkMask);
			if (Grab)
				XUngrabServer (dpy);
		}
	}
#endif
}
