/*
 * Copyright (C) 2000 Sasha Vasko <sasha at aftercode.net>
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
 */

#include "../configure.h"
#include "asapp.h"
#include "../libAfterImage/afterimage.h"
#include "afterstep.h"
#include "screen.h"
#include "event.h"

#ifdef XSHMIMAGE
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

static Bool   as_xserver_is_local = False;	   /* True if we are running on the same host as X Server */

/* This is used for error handling : */
int           last_event_type = 0;
Window        last_event_window = 0;


#define FullStructureNotifyMask (SubstructureNotifyMask|StructureNotifyMask)

static ASEventDescription _as_event_types[LASTEvent] = {
/*  */ {"nothing", 0},
/*  */ {"nothing", 0},
/* KeyPress			2  */ {"KeyPress", KeyPressMask, ASE_KeyboardEvent},
/* KeyRelease		3  */ {"KeyRelease", KeyReleaseMask, ASE_KeyboardEvent},
/* ButtonPress		4  */ {"ButtonPress", ButtonPressMask, ASE_MousePressEvent},
/* ButtonRelease	5  */ {"ButtonRelease", ButtonReleaseMask, ASE_MousePressEvent},
/* MotionNotify		6  */ {"MotionNotify", PointerMotionMask, ASE_MouseMotionEvent},
/* EnterNotify		7  */ {"EnterNotify", EnterWindowMask, ASE_MouseMotionEvent},
/* LeaveNotify		8  */ {"LeaveNotify", LeaveWindowMask, ASE_MouseMotionEvent},
/* FocusIn			9  */ {"FocusIn", FocusChangeMask, 0},
/* FocusOut			10 */ {"FocusOut", FocusChangeMask, 0},
/* KeymapNotify		11 */ {"KeymapNotify", KeymapStateMask, 0},
/* Expose			12 */ {"Expose", ExposureMask, ASE_Expose},
/* GraphicsExpose	13 */ {"GraphicsExpose", ExposureMask, ASE_Expose},
/* NoExpose			14 */ {"NoExpose", ExposureMask, ASE_Expose},
/* VisibilityNotify	15 */ {"VisibilityNotify", VisibilityChangeMask, 0},
/* CreateNotify		16 */ {"CreateNotify", SubstructureNotifyMask, 0},
/* DestroyNotify	17 */ {"DestroyNotify", FullStructureNotifyMask, 0},
/* UnmapNotify		18 */ {"UnmapNotify", FullStructureNotifyMask, 0},
/* MapNotify		19 */ {"MapNotify", FullStructureNotifyMask, 0},
/* MapRequest		20 */ {"MapRequest", SubstructureRedirectMask, 0},
/* ReparentNotify	21 */ {"ReparentNotify", FullStructureNotifyMask, 0},
/* ConfigureNotify	22 */ {"ConfigureNotify", FullStructureNotifyMask, ASE_Config},
/* ConfigureRequest	23 */ {"ConfigureRequest", SubstructureRedirectMask, 0},
/* GravityNotify	24 */ {"GravityNotify", FullStructureNotifyMask, 0},
/* ResizeRequest	25 */ {"ResizeRequest", ResizeRedirectMask, 0},
/* CirculateNotify	26 */ {"CirculateNotify", FullStructureNotifyMask, 0},
/* CirculateRequest	27 */ {"CirculateRequest", SubstructureRedirectMask, 0},
/* PropertyNotify	28 */ {"PropertyNotify", PropertyChangeMask, 0},
/* SelectionClear	29 */ {"SelectionClear", SelectionMask, 0},
/* SelectionRequest	30 */ {"SelectionRequest", SelectionMask, 0},
/* SelectionNotify	31 */ {"SelectionNotify", SelectionMask, 0},
/* ColormapNotify	32 */ {"ColormapNotify", ColormapChangeMask, 0},
/* ClientMessage	33 */ {"ClientMessage", ClientMask, 0},
/* MappingNotify	34 */ {"MappingNotify", MappingMask, 0}
};

static struct ContextDescr
{
	int           context;
	char         *name;
} context_description[] =
{
#define CONTEXT_DESCR(ctx)  {ctx, #ctx}
	CONTEXT_DESCR (C_NO_CONTEXT),
		CONTEXT_DESCR (C_FrameN),
		CONTEXT_DESCR (C_FrameE),
		CONTEXT_DESCR (C_FrameS),
		CONTEXT_DESCR (C_FrameW),
		CONTEXT_DESCR (C_FrameNW),
		CONTEXT_DESCR (C_FrameNE),
		CONTEXT_DESCR (C_FrameSW),
		CONTEXT_DESCR (C_FrameSE),
		CONTEXT_DESCR (C_SIDEBAR),
		CONTEXT_DESCR (C_VERTICAL_SIDEBAR),
		CONTEXT_DESCR (C_FRAME),
		CONTEXT_DESCR (C_FrameStart),
		CONTEXT_DESCR (C_FrameEnd),
		CONTEXT_DESCR (C_WINDOW),
		CONTEXT_DESCR (C_CLIENT),
		CONTEXT_DESCR (C_TITLE),
		CONTEXT_DESCR (C_IconTitle),
		CONTEXT_DESCR (C_IconButton),
		CONTEXT_DESCR (C_ICON),
		CONTEXT_DESCR (C_ROOT),
		CONTEXT_DESCR (C_TButton0),
		CONTEXT_DESCR (C_TButton1),
		CONTEXT_DESCR (C_TButton2),
		CONTEXT_DESCR (C_TButton3),
		CONTEXT_DESCR (C_TButton4),
		CONTEXT_DESCR (C_TButton5),
		CONTEXT_DESCR (C_TButton6),
		CONTEXT_DESCR (C_TButton7),
		CONTEXT_DESCR (C_TButton8), CONTEXT_DESCR (C_TButton9), CONTEXT_DESCR (C_TButtonAll), CONTEXT_DESCR (C_ALL),
	{
	-1, NULL}
};

/***************************************************************************
 * we need to prepare message handlers :
 ***************************************************************************/
void
event_setup (Bool local)
{
	register int  i;
	XEvent        event;

	as_xserver_is_local = local;
	/* adding out handlers here : */
	for (i = 0; i < LASTEvent; ++i)
	{
		_as_event_types[i].time_offset = _as_event_types[i].window_offset = _as_event_types[i].last_time = 0;
		if (i >= KeyPress && i <= LeaveNotify)
		{
			_as_event_types[i].time_offset = (char *)&(event.xkey.time) - (char *)&(event);
/*			WE want the actuall window we selected mask on here,
			not some lame subwindow !

			_as_event_types[i].window_offset =
				 (char*)&(event.xkey.subwindow) - (char*)&(event); */

		} else if ((i >= CreateNotify && i <= GravityNotify) || (i >= CirculateNotify && i <= CirculateRequest))
			/*CreateNotify, DestroyNotify, UnmapNotify, MapNotify, MapRequest,
			 *ReparentNotify, ConfigureNotify, ConfigureRequest, GravityNotify */
		{
			_as_event_types[i].window_offset = (char *)&(event.xcreatewindow.window) - (char *)&(event);
		}
	}
	_as_event_types[PropertyNotify].time_offset = (char *)&(event.xproperty.time) - (char *)&(event);
	_as_event_types[SelectionClear].time_offset = (char *)&(event.xselectionclear.time) - (char *)&(event);
	_as_event_types[SelectionRequest].time_offset = (char *)&(event.xselectionrequest.time) - (char *)&(event);
	_as_event_types[SelectionNotify].time_offset = (char *)&(event.xselection.time) - (char *)&(event);
}

const char   *
event_type2name (int type)
{
	if (type > 0 && type < LASTEvent)
		return _as_event_types[type].name;
	return "unknown";
}

const char   *
context2text (int ctx)
{
	register int  i = -1;

	while (context_description[++i].name)
		if (context_description[i].context == ctx)
			return context_description[i].name;
	return "unknown";
}

/***********************************************************************
 * Event Queue management : 										   *
 ***********************************************************************/
void
flush_event_queue (Bool check_pending)
{
	if (check_pending)
		if (XPending (dpy))
			return;
	XFlush (dpy);
}

void
sync_event_queue (Bool forget)
{
	XSync (dpy, forget);
}

/****************************************************************************
 * Records the time of the last processed event. Used in XSetInputFocus
 ****************************************************************************/
inline Time
stash_event_time (XEvent * xevent)
{
	if (xevent->type < LASTEvent)
	{
		register Time *ptime = (Time *) ((char *)xevent + _as_event_types[xevent->type].time_offset);

		last_event_type = xevent->type;
		last_event_window = xevent->xany.window;

		if (ptime != (Time *) xevent)
		{
			register Time NewTimestamp = *ptime;

			if (NewTimestamp < ASDefaultScr->last_Timestamp)
			{
				if (as_xserver_is_local)
				{							   /* hack to detect local time change and try to work around it */
					static time_t last_system_time = 0;
					time_t        curr_time;

					if (time (&curr_time) < last_system_time)
					{						   /* local time has been changed !!!!!!!! */
						ASDefaultScr->last_Timestamp = NewTimestamp;
						ASDefaultScr->menu_grab_Timestamp = NewTimestamp;
					}
					last_system_time = curr_time;
				}
				if (ASDefaultScr->last_Timestamp - NewTimestamp > 0x7FFFFFFF)	/* detecting time lapse */
				{
					ASDefaultScr->last_Timestamp = NewTimestamp;
					if (ASDefaultScr->menu_grab_Timestamp - NewTimestamp > 0x7FFFFFFF)
						ASDefaultScr->menu_grab_Timestamp = 0;
				}
			} else
				ASDefaultScr->last_Timestamp = NewTimestamp;
			return (_as_event_types[xevent->type].last_time = *ptime);
		}
	}
	return 0;
}

/* here we will determine what screen event occured on : */
inline ScreenInfo *
query_event_screen (register XEvent * event)
{											   /* stub since stable AS does not support multiscreen handling in one process */
	return ASDefaultScr;
}

Window
get_xevent_window (XEvent * xevt)
{
	int           type = xevt->type;

	if (type < LASTEvent)
	{
		register Window *pwin = (Window *) ((char *)xevt + _as_event_types[type].window_offset);

		return (pwin == (Window *) xevt || *pwin == None) ? xevt->xany.window : *pwin;
	} else
		return xevt->xany.window;
}

void
setup_asevent_from_xevent (ASEvent * event)
{
	XEvent       *xevt = &(event->x);
	int           type = xevt->type;

	if (type < LASTEvent)
	{
		register Time *ptime = (Time *) ((char *)xevt + _as_event_types[type].time_offset);
		register Window *pwin = (Window *) ((char *)xevt + _as_event_types[type].window_offset);

		event->w = (pwin == (Window *) xevt || *pwin == None) ? xevt->xany.window : *pwin;
		event->event_time = (ptime == (Time *) xevt) ? 0 : *ptime;

		event->scr = ASEventScreen (xevt);
		event->mask = _as_event_types[type].mask;
		event->eclass = _as_event_types[type].event_class;
		event->last_time = _as_event_types[type].last_time;
	} else
	{
		event->w = xevt->xany.window;
		event->event_time = 0;

		event->scr = ASDefaultScr;
		event->mask = 0;
		event->eclass = 0;
		event->last_time = 0;
	}
}

/**********************************************************************/
/* window management specifics - mapping/unmapping with no events :   */
/**********************************************************************/

void
add_window_event_mask (Window w, long event_mask)
{
	XWindowAttributes attr;

	if (XGetWindowAttributes (dpy, w, &attr))
		XSelectInput (dpy, w, attr.your_event_mask | event_mask);
}


void
quietly_unmap_window (Window w, long event_mask)
{
	/* blocking UnmapNotify events since that may bring us into Withdrawn state */
	XSelectInput (dpy, w, event_mask & ~StructureNotifyMask);
	XUnmapWindow (dpy, w);
	XSelectInput (dpy, w, event_mask);
}

void
quietly_reparent_window (Window w, Window new_parent, int x, int y, long event_mask)
{
	/* blocking UnmapNotify events since that may bring us into Withdrawn state */
	XSelectInput (dpy, w, event_mask & ~StructureNotifyMask);
	XReparentWindow (dpy, w, (new_parent != None) ? new_parent : ASDefaultRoot, x, y);
	XSelectInput (dpy, w, event_mask);
}

int
Empty_XErrorHandler (Display * dpy, XErrorEvent * event)
{
	return 0;
}

void
safely_destroy_window (Window w)
{
	int           (*old_handler) (Display * dpy, XErrorEvent * event);

	old_handler = XSetErrorHandler (Empty_XErrorHandler);
	XDestroyWindow (dpy, w);
	XSync (dpy, False);
	XSetErrorHandler (old_handler);
}

Bool
query_pointer (Window w,
			   Window * root_return, Window * child_return,
			   int *root_x_return, int *root_y_return, int *win_x_return, int *win_y_return, unsigned int *mask_return)
{
	Window        root, child;
	int           root_x, root_y;
	int           win_x, win_y;
	unsigned int  mask;

	if (!XQueryPointer (dpy, ((w == None) ? ASDefaultRoot : w), &root, &child, &root_x, &root_y, &win_x, &win_y, &mask))
		return False;

	if (root_return)
		*root_return = root;
	if (child_return)
		*child_return = child;
	if (root_x_return)
		*root_x_return = root_x;
	if (root_y_return)
		*root_y_return = root_y;
	if (win_x_return)
		*win_x_return = win_x;
	if (win_y_return)
		*win_y_return = win_y;
	if (mask_return)
		*mask_return = mask;

	return True;
}

/**********************************************************************/
/* Low level interfaces : 											  */
/**********************************************************************/
Bool
check_event_masked (register long mask, register XEvent * event_return)
{
	register int  res;

	res = XCheckMaskEvent (dpy, mask, event_return);
	if (res)
		stash_event_time (event_return);
	return res;
}

Bool
check_event_typed (register int event_type, register XEvent * event_return)
{
	register int  res;

	res = XCheckTypedEvent (dpy, event_type, event_return);
	if (res)
		stash_event_time (event_return);
	return res;
}

Bool
check_event_typed_windowed (Window w, int event_type, register XEvent * event_return)
{
	register int  res;

	res = XCheckTypedWindowEvent (dpy, w, event_type, event_return);
	if (res)
		stash_event_time (event_return);
	return res;
}

Bool
check_event_windowed (Window w, long event_mask, register XEvent * event_return)
{
	register int  res;

	res = XCheckWindowEvent (dpy, w, event_mask, event_return);
	if (res)
		stash_event_time (event_return);
	return res;
}

Bool
recursively_find_motion_notify (int depth)
{
	XEvent        junk_event;

	if (XCheckMaskEvent (dpy, 0xFFFFFFFF, &junk_event))
	{
		XPutBackEvent (dpy, &junk_event);
		if (junk_event.type == MotionNotify)
			return True;
		if (depth > 0)
			return recursively_find_motion_notify (depth - 1);
	}
	return False;
}

int
next_event (register XEvent * event_return, Bool compress_motion)
{
	register int  res;

	res = (XNextEvent (dpy, event_return) == 0);
	if (res)
	{
		stash_event_time (event_return);
#if 0
		if (compress_motion && event_return->type == MotionNotify)
		{
			if (recursively_find_motion_notify (5))
				return (False);
			XFlush (dpy);
			if (recursively_find_motion_notify (5))
				return (False);
			sleep_a_millisec (20);			   /* 0.3 sec delay */
			if (recursively_find_motion_notify (10))
				return False;
		}
#endif
	}
	return res;
}

int
peek_event (register XEvent * event_return)
{
	register int  res;

	res = XPeekEvent (dpy, event_return);
	if (res)
		stash_event_time (event_return);
	return res;
}

int
mask_event (long event_mask, register XEvent * event_return)
{
	register int  res;

	res = XMaskEvent (dpy, event_mask, event_return);
	if (res)
		stash_event_time (event_return);
	return res;
}

int
window_event (Window w, long event_mask, register XEvent * event_return)
{
	register int  res;

	res = XWindowEvent (dpy, w, event_mask, event_return);
	if (res)
		stash_event_time (event_return);
	return res;
}

Bool
wait_event (XEvent * event, Window w, int mask, int max_wait)
{
	int           tick_count;

	start_ticker (1);
	/* now we have to wait for our window to become mapped - waiting for PropertyNotify */
	for (tick_count = 0; !XCheckWindowEvent (dpy, w, mask, event) && tick_count < max_wait; tick_count++)
	{
		XSync (dpy, False);
		wait_tick ();
	}
	return (tick_count < max_wait);
}

void
handle_ShmCompletion (ASEvent * event)
{
#ifdef XSHMIMAGE
	XShmCompletionEvent *sev = (XShmCompletionEvent *) & (event->x);

	LOCAL_DEBUG_OUT ("XSHMIMAGE> EVENT : offset   %ld(%lx), shmseg = %lx", (long)sev->offset,
					 (unsigned long)(sev->offset), sev->shmseg);
	destroy_xshm_segment (sev->shmseg);
#endif /* SHAPE */
}
