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

#include <stdlib.h>
#include <time.h>

#include "../include/afterbase.h"
#include "../include/afterstep.h"
#include "../include/screen.h"
#include "../include/event.h"

static Bool   as_xserver_is_local = False;     /* True if we are running on the same host as X Server */

/* This is used for error handling : */
int           last_event_type = 0;
Window        last_event_window = 0;


#define FullStructureNotifyMask (SubstructureNotifyMask|StructureNotifyMask)

static ASEventDescription _as_event_types[LASTEvent] = {
/*  */	{"nothing", 0},
/*  */	{"nothing", 0},
/* KeyPress			2  */ {"KeyPress", 			KeyPressMask			,ASE_KeyboardEvent},
/* KeyRelease		3  */ {"KeyRelease", 		KeyReleaseMask			,ASE_KeyboardEvent},
/* ButtonPress		4  */ {"ButtonPress", 		ButtonPressMask			,ASE_MousePressEvent},
/* ButtonRelease	5  */ {"ButtonRelease", 	ButtonReleaseMask		,ASE_MousePressEvent},
/* MotionNotify		6  */ {"MotionNotify", 		PointerMotionMask		,ASE_MouseMotionEvent},
/* EnterNotify		7  */ {"EnterNotify", 		EnterWindowMask			,ASE_MouseMotionEvent},
/* LeaveNotify		8  */ {"LeaveNotify", 		LeaveWindowMask			,ASE_MouseMotionEvent},
/* FocusIn			9  */ {"FocusIn", 			FocusChangeMask			,0},
/* FocusOut			10 */ {"FocusOut", 			FocusChangeMask			,0},
/* KeymapNotify		11 */ {"KeymapNotify", 		KeymapStateMask			,0},
/* Expose			12 */ {"Expose", 			ExposureMask			,ASE_Expose},
/* GraphicsExpose	13 */ {"GraphicsExpose",	ExposureMask			,ASE_Expose},
/* NoExpose			14 */ {"NoExpose", 			ExposureMask			,ASE_Expose},
/* VisibilityNotify	15 */ {"VisibilityNotify",	VisibilityChangeMask	,0},
/* CreateNotify		16 */ {"CreateNotify", 		SubstructureNotifyMask	,0},
/* DestroyNotify	17 */ {"DestroyNotify", 	FullStructureNotifyMask	,0},
/* UnmapNotify		18 */ {"UnmapNotify", 		FullStructureNotifyMask	,0},
/* MapNotify		19 */ {"MapNotify", 		FullStructureNotifyMask	,0},
/* MapRequest		20 */ {"MapRequest", 		SubstructureRedirectMask,0},
/* ReparentNotify	21 */ {"ReparentNotify",	FullStructureNotifyMask	,0},
/* ConfigureNotify	22 */ {"ConfigureNotify",	FullStructureNotifyMask	,ASE_Config},
/* ConfigureRequest	23 */ {"ConfigureRequest",	SubstructureRedirectMask,0},
/* GravityNotify	24 */ {"GravityNotify", 	FullStructureNotifyMask	,0},
/* ResizeRequest	25 */ {"ResizeRequest", 	ResizeRedirectMask		,0},
/* CirculateNotify	26 */ {"CirculateNotify", 	FullStructureNotifyMask	,0},
/* CirculateRequest	27 */ {"CirculateRequest", 	SubstructureRedirectMask,0},
/* PropertyNotify	28 */ {"PropertyNotify", 	PropertyChangeMask		,0},
/* SelectionClear	29 */ {"SelectionClear", 	SelectionMask			,0},
/* SelectionRequest	30 */ {"SelectionRequest", 	SelectionMask			,0},
/* SelectionNotify	31 */ {"SelectionNotify", 	SelectionMask			,0},
/* ColormapNotify	32 */ {"ColormapNotify", 	ColormapChangeMask		,0},
/* ClientMessage	33 */ {"ClientMessage", 	ClientMask				,0},
/* MappingNotify	34 */ {"MappingNotify", 	MappingMask				,0}
};


/***************************************************************************
 * we need to prepare message handlers :
 ***************************************************************************/
void event_setup( Bool local )
{
	register int i ;
	XEvent event;

	as_xserver_is_local = local ;
    /* adding out handlers here : */
	for( i = 0 ; i < LASTEvent ; ++i )
	{
		_as_event_types[i].time_offset =
		_as_event_types[i].window_offset =
		_as_event_types[i].last_time = 0;
		if (i >= KeyPress && i <= LeaveNotify)
		{
			_as_event_types[i].time_offset =
		         (void*)&(event.xkey.time) - (void*)&(event);
/*			WE want the actuall window we selected mask on here,
            not some lame subwindow !

			_as_event_types[i].window_offset =
		         (void*)&(event.xkey.subwindow) - (void*)&(event); */

    	}else if ( (i >= CreateNotify && i <= GravityNotify) ||
			       (i >= CirculateNotify && i <= CirculateRequest) )
				  /*CreateNotify, DestroyNotify, UnmapNotify, MapNotify, MapRequest,
 				  *ReparentNotify, ConfigureNotify, ConfigureRequest, GravityNotify */
		{
			_as_event_types[i].window_offset =
		         (void*)&(event.xcreatewindow.window) - (void*)&(event);
		}
	}
	_as_event_types[PropertyNotify].time_offset =
			 (void*)&(event.xproperty.time) - (void*)&(event);
	_as_event_types[SelectionClear].time_offset =
			 (void*)&(event.xselectionclear.time) - (void*)&(event);
	_as_event_types[SelectionRequest].time_offset =
			 (void*)&(event.xselectionrequest.time) - (void*)&(event);
	_as_event_types[SelectionNotify].time_offset =
	         (void*)&(event.xselection.time) - (void*)&(event);
}

const char *event_type2name( int type )
{
	if( type > 0 && type < LASTEvent )
		return _as_event_types[type].name;
	return "unknown";
}

/***********************************************************************
 * Event Queue management : 										   *
 ***********************************************************************/
void flush_event_queue(Bool check_pending)
{
    if( check_pending )
        if( XPending( dpy ) )
            return;
    XFlush( dpy );
}

void sync_event_queue(Bool forget)
{
    XSync( dpy, forget );
}

/****************************************************************************
 * Records the time of the last processed event. Used in XSetInputFocus
 ****************************************************************************/
inline Time stash_event_time (XEvent * xevent)
{
	register Time *ptime = (Time*)((void*)xevent + _as_event_types[xevent->type].time_offset);

    last_event_type   = xevent->type;
    last_event_window = xevent->xany.window;

	if( ptime != (Time*)xevent )
	{
	    register Time  NewTimestamp = *ptime;

        if (NewTimestamp < Scr.last_Timestamp)
        {
            if(as_xserver_is_local)
            {   /* hack to detect local time change and try to work around it */
                static time_t        last_system_time = 0;
                time_t               curr_time ;

                if( time(&curr_time) < last_system_time )
                {   /* local time has been changed !!!!!!!! */
                    Scr.last_Timestamp = NewTimestamp ;
                    Scr.menu_grab_Timestamp = NewTimestamp ;
                }
                last_system_time = curr_time ;
            }
            if( Scr.last_Timestamp - NewTimestamp > 0x7FFFFFFF ) /* detecting time lapse */
            {
                Scr.last_Timestamp = NewTimestamp;
                if( Scr.menu_grab_Timestamp - NewTimestamp > 0x7FFFFFFF )
                    Scr.menu_grab_Timestamp = 0 ;
            }
        }else
			Scr.last_Timestamp = NewTimestamp;
		return (_as_event_types[xevent->type].last_time = *ptime) ;
    }
	return 0;
}
/* here we will determine what screen event occured on : */
inline ScreenInfo *
query_event_screen( register XEvent *event )
{
    return &Scr;

    static ScreenInfo *pointer_screen = NULL;
	static ScreenInfo *focus_screen = NULL;

	switch( event->type )
	{
		case KeyPress		:
		case KeyRelease		:
		case ButtonPress	:
		case ButtonRelease	:
		case MotionNotify	:
		case LeaveNotify	:
		  if( pointer_screen )
		  		return pointer_screen;
		case EnterNotify	:
		  {
			int scr_num = is_root_window(event->xany.window);
			if( scr_num >= 0 )
				pointer_screen = all_screens[scr_num];
		  }
		  return pointer_screen;
		case FocusOut		:
		  if( focus_screen )
		  		return focus_screen ;
		case FocusIn		:
		  focus_screen = get_window_screen(event->xany.window);
		  return focus_screen ;
		default :
		  break;
	}
	return get_window_screen(event->xany.window);
}

void setup_asevent_from_xevent( ASEvent *event )
{
	XEvent *xevt = &(event->x);
	int type = xevt->type;
	register Time *ptime = (Time*)((void*)xevt + _as_event_types[type].time_offset);
	register Window *pwin = (Window*)((void*)xevt + _as_event_types[type].window_offset);

	event->w = (pwin==(Window*)xevt || *pwin == None )?xevt->xany.window:*pwin;
	event->event_time = (ptime==(Time*)xevt)?0:*ptime;

	event->scr 			= ASEventScreen( xevt );
	event->mask 		= _as_event_types[type].mask ;
	event->eclass 		= _as_event_types[type].event_class ;
	event->last_time 	= _as_event_types[type].last_time ;
}

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

/**********************************************************************/
/* Low level interfaces : 											  */
/**********************************************************************/
Bool
check_event_masked( register long mask, register XEvent * event_return)
{
    register int           res;
    res = XCheckMaskEvent (dpy, mask, event_return);
    if (res)
        stash_event_time(event_return);
	return res;
}

Bool
check_event_typed( register int event_type, register XEvent * event_return)
{
    register int           res;
    res = XCheckTypedEvent (dpy, event_type, event_return);
    if (res)
        stash_event_time(event_return);
    return res;
}

Bool
check_event_typed_windowed( Window w, int event_type, register XEvent *event_return )
{
    register int           res;
	res = XCheckTypedWindowEvent (dpy, w, event_type, event_return);
    if (res)
        stash_event_time(event_return);
    return res;
}

Bool
check_event_windowed (Window w, long event_mask, register XEvent * event_return)
{
    register int           res;
	res = XCheckWindowEvent (dpy, w, event_mask, event_return);
    if (res)
        stash_event_time(event_return);
    return res;
}

int
next_event (register XEvent * event_return)
{
    register int           res;
    res = XNextEvent (dpy, event_return);
    if (res)
        stash_event_time(event_return);
    return res;
}

int
peek_event (register XEvent * event_return)
{
    register int           res;
	res = XPeekEvent (dpy, event_return);
    if (res)
        stash_event_time(event_return);
    return res;
}

int
mask_event (long event_mask, register XEvent * event_return)
{
    register int           res;
	res = XMaskEvent (dpy, event_mask, event_return);
    if (res)
        stash_event_time(event_return);
    return res;
}

int
window_event (Window w, long event_mask, register XEvent * event_return)
{
    register int           res;
	res = XWindowEvent (dpy, w, event_mask, event_return);
    if (res)
        stash_event_time(event_return);
    return res;
}

Bool
wait_event( XEvent *event, Window w, int mask, int max_wait )
{
    int tick_count ;

    start_ticker (1);
    /* now we have to wait for our window to become mapped - waiting for PropertyNotify */
    for (tick_count = 0; !XCheckWindowEvent (dpy, w, mask, event) && tick_count < max_wait; tick_count++)
        wait_tick ();
    return (tick_count < max_wait);
}


