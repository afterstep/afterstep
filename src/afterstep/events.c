/*
 * Copyright (C) 1995 Bo Yang
 * Copyright (C) 1993 Robert Nation
 * Copyright (C) 1993 Frank Fejes
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

#define LOCAL_DEBUG
#define EVENT_TRACE

#include "../../configure.h"

#include "../../include/asapp.h"
#include <limits.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include "../../include/afterstep.h"
#include "../../include/module.h"
#include "../../include/parse.h"
#include "../../include/decor.h"
#include "../../include/screen.h"
#include "../../include/event.h"
#include "../../include/balloon.h"
#include "../../include/loadimg.h"
#include "../../libAfterImage/afterimage.h"
#include "../../include/mystyle.h"
#include "../../include/wmprops.h"
#include "../../include/moveresize.h"
#include "asinternals.h"

#include <X11/keysym.h>

/***********************************************************************
 *  _______________________EVENT HANDLING ______________________________
 *
 *  HandleEvents  - event loop
 *  DigestEvent   - preprocesses event - finds ASWindow, context etc.
 *  DispatchEvent - calls appropriate handler for the event
 ************************************************************************/
void DigestEvent    ( ASEvent *event );
void DispatchEvent  ( ASEvent *event );
void afterstep_wait_pipes_input ();
void SetTimer (int delay);

void
HandleEvents ()
{
    ASEvent event;
    Bool has_x_events = False ;
    while (True)
    {
        while((has_x_events = XPending (dpy)))
        {
            if( ASNextEvent (&(event.x), True) )
            {
                DigestEvent( &event );
                DispatchEvent( &event );
            }
        }
        afterstep_wait_pipes_input ();
    }
}

/***************************************************************************
 * Wait for all mouse buttons to be released
 * This can ease some confusion on the part of the user sometimes
 *
 * Discard superflous button events during this wait period.
 ***************************************************************************/
void
WaitForButtonsUpLoop ()
{
	XEvent        JunkEvent;
    unsigned int  mask;

    if( !get_flags( AfterStepState, ASS_PointerOutOfScreen) )
	{
		do
		{
			XAllowEvents (dpy, ReplayPointer, CurrentTime);
            ASQueryPointerMask(&mask);
        	ASFlushAndSync();
		}while( (mask & (Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask)) != 0 );

    	while (ASCheckMaskEvent ( ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, &JunkEvent))
			XAllowEvents (dpy, ReplayPointer, CurrentTime);
	}
}

Bool
WaitEventLoop( ASEvent *event, int finish_event_type, long timeout )
{
	unsigned long mask = ButtonPressMask | ButtonReleaseMask |
						 ExposureMask | KeyPressMask | ButtonMotionMask |
						 PointerMotionMask	/* | EnterWindowMask | LeaveWindowMask */;

    while (event->x.type != finish_event_type)
	{
		/* block until there is an event */
        ASFlushIfEmpty();
        ASMaskEvent ( mask, &(event->x));

        if( event->x.type == KeyPress )
            KeyboardShortcuts (&(event->x), finish_event_type, 20);
            /* above line might have changed event code !!!*/
        else if (event->x.type == ButtonPress )
			XAllowEvents (dpy, ReplayPointer, CurrentTime);

        DigestEvent( event );
        DispatchEvent( event );
    }

    return True ;
}

/*****************************************************************************
 * Waits click_time, or until it is evident that the user is not
 * clicking, but is moving the cursor
 ****************************************************************************/
Bool
IsClickLoop( ASEvent *event, unsigned int end_mask, unsigned int click_time )
{
	int dx = 0, dy = 0, total = 0;
	int x_orig = event->x.xbutton.x_root ;
	int y_orig = event->x.xbutton.y_root ;
    /* we are in the middle of running Complex function - we must only do mandatory
     * processing on received events, but do not actually handle them !!
     * Client window affected, as well as the context must not change -
     * only the X event could */
    ASEvent tmp_event ;
    register XEvent *xevt = &(tmp_event.x) ;

    do
	{
        ASFlush();
		sleep_a_little (1000);
        if (ASCheckMaskEvent (end_mask, xevt))
        {
            DigestEvent( &tmp_event );
            event->x = *xevt ;                 /* everything else must remain the same !!! */
            return True;
        }
        if( total++ > click_time )
			break;

        if (ASCheckMaskEvent ( ButtonMotionMask | PointerMotionMask, xevt))
		{
			dx = x_orig - xevt->xmotion.x_root;
			dy = y_orig - xevt->xmotion.y_root;
            DigestEvent( &tmp_event );
		}
    }while( dx > -5 && dx < 5 && dy > -5 && dy < 5 );
    event->x = *xevt ;                 /* everything else must remain the same !!! */

	return False;
}

Bool
WaitWindowLoop( char *pattern, long timeout )
{
	wild_reg_exp *wrexp = compile_wild_reg_exp( pattern );
	Bool done = False ;
	ASEvent event ;
	Bool has_x_events ;

	if( wrexp == NULL )
		return False;
	while (!done)
	{
		do
		{
            ASFlush();
			while((has_x_events = XPending (dpy)))
			{
                if( ASNextEvent (&(event.x), True) )
                {

                    DigestEvent( &event );
                    DispatchEvent( &event );
                    if( event.x.type == MapNotify && event.client )
                        if( match_string_list (event.client->hints->names, MAX_WINDOW_NAMES, wrexp) == 0 )
                        {
                            destroy_wild_reg_exp( wrexp );
                            return True;
                        }
                }
			}
		}while( has_x_events );

		afterstep_wait_pipes_input ();
	}
	destroy_wild_reg_exp( wrexp );
    return False;
}

/*************************************************************************
 * This loop handles all the pending Configure Notifys so that canvases get
 * all nicely synchronized. This is generaly needed when we need to do
 * reparenting.
 *************************************************************************/
void
ConfigureNotifyLoop()
{
    ASEvent event;
    while( ASCheckTypedEvent(ConfigureNotify,&(event.x)) )
    {
        DigestEvent( &event );
        DispatchEvent( &event );
        ASSync(False);
    }
}

void
DigestEvent( ASEvent *event )
{
    register int i ;
    setup_asevent_from_xevent( event );
    event->context = C_ROOT ;
    event->widget = Scr.RootCanvas ;
    /* in housekeeping mode we handle pointer events only as applied to root window ! */
    if( Scr.moveresize_in_progress && (event->eclass & ASE_POINTER_EVENTS) != 0)
        event->client = NULL;
    else
        event->client = window2ASWindow( event->w );

    if( (event->eclass & ASE_POINTER_EVENTS) != 0 && event->client )
    {
        /* now lets determine the context of the event : (former GetContext)*/
        Window   w = event->w ;
        ASWindow *asw = event->client ;
        XKeyEvent *xk = &(event->x.xkey);
        ASCanvas  *canvas = asw->frame_canvas ;
        ASTBarData *pointer_bar = NULL ;
        /* Since key presses and button presses are grabbed in the frame
         * when we have re-parented windows, we need to find out the real
         * window where the event occured */
        if (!ASWIN_GET_FLAGS(asw, AS_Iconic))
        {
            if (xk->subwindow != None)
                w = xk->subwindow;
            if( w == asw->client_canvas->w )
            {
                canvas = asw->client_canvas ;
                event->context = C_CLIENT ;
            }else if( w != asw->frame )
            {
                i = FRAME_SIDES ;
                while( --i >= 0 )
                    if( asw->frame_sides[i] != NULL &&
                        asw->frame_sides[i]->w == w )
                    {
                        canvas = asw->frame_sides[i];
                        /* determine what part of the frame : */
                        event->context = C_FRAME ;
                        break;
                    }
            }

            if( w != asw->frame )
            {
                if( event->w == asw->frame )
                {
                    xk->x = xk->x_root - canvas->root_x ;
                    xk->y = xk->y_root - canvas->root_y ;
                }else
                {
                    Window dumm;
                    XTranslateCoordinates(dpy,Scr.Root,w,xk->x_root, xk->y_root, &(xk->x), &(xk->y), &dumm );
                }
            }
            if( event->context == C_FRAME )
            {
                int tbar_context ;
                if( asw->tbar != NULL &&
                    (tbar_context = check_astbar_point( asw->tbar, xk->x_root, xk->y_root )) != C_NO_CONTEXT )
                {
                    event->context = tbar_context ;
                    pointer_bar = asw->tbar ;
                }else
                {
                    for( i = 0 ; i < FRAME_PARTS ; ++i )
                        if( asw->frame_bars[i] != NULL &&
                            (tbar_context = check_astbar_point( asw->frame_bars[i], xk->x_root, xk->y_root )) != C_NO_CONTEXT )
                        {
                            event->context = tbar_context ;
                            pointer_bar = asw->frame_bars[i] ;
                            break;
                        }
                }
            }
            event->w = w ;
        }else
        {
            if( asw->icon_canvas && w == asw->icon_canvas->w )
            {
                event->context = C_IconButton ;
                canvas = asw->icon_canvas ;
                pointer_bar = asw->icon_button ;
                if( canvas == asw->icon_title_canvas )
                {
                    int c = check_astbar_point( asw->icon_title, xk->x_root, xk->y_root );
                    if( c != C_NO_CONTEXT )
                    {
                        event->context = c ;
                        pointer_bar = asw->icon_title ;
                    }
                }
            }else if( asw->icon_title_canvas && w == asw->icon_title_canvas->w )
            {
                canvas = asw->icon_title_canvas ;
                event->context = C_IconTitle ;
                pointer_bar = asw->icon_title ;
            }
        }

        on_astbar_pointer_action( pointer_bar, event->context, (event->x.type == LeaveNotify) );

        event->widget  = canvas ;
        /* we have to do this at all times !!!! */
        if( event->x.type == ButtonRelease && Scr.Windows->pressed )
            release_pressure();
    }
#if !defined(EVENT_TRACE) || defined(NO_DEBUG_OUTPUT)
    if( get_output_threshold() >= OUTPUT_LEVEL_DEBUG )
#endif
    {
        show_progress("****************************************************************");
        show_progress("%s:%s:%d><<EVENT type(%d(%s))->x.window(%lx)->event.w(%lx)->client(%p)->context(%s)->send_event(%d)", __FILE__, __FUNCTION__, __LINE__, event->x.type, event_type2name(event->x.type), event->x.xany.window, event->w, event->client, context2text(event->context), event->x.xany.send_event);
    }
}

/****************************************************************************
 * For menus, move, and resize operations, we can effect keyboard
 * shortcuts by warping the pointer.
 ****************************************************************************/
Bool
KeyboardShortcuts (XEvent * xevent, int return_event, int move_size)
{
	int           x, y, x_root, y_root;
	int           x_move, y_move;
	KeySym        keysym;

	/* Pick the size of the cursor movement */
    if (xevent->xkey.state & ControlMask)
		move_size = 1;
    if (xevent->xkey.state & ShiftMask)
		move_size = 100;

    keysym = XLookupKeysym (&(xevent->xkey), 0);

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
         xevent->type = return_event;
		 break;
	 default:
         return False;
	}
    ASQueryPointerXY(&xevent->xany.window,&x_root, &y_root, &x, &y);

	if ((x_move != 0) || (y_move != 0))
	{
		/* beat up the event */
        XWarpPointer (dpy, None, Scr.Root, 0, 0, 0, 0, x_root + x_move, y_root + y_move);

		/* beat up the event */
        xevent->type = MotionNotify;
        xevent->xkey.x += x_move;
        xevent->xkey.y += y_move;
        xevent->xkey.x_root += x_move;
        xevent->xkey.y_root += y_move;
	}
    return True;
}



void
DispatchEvent ( ASEvent *event )
{
    if( Scr.moveresize_in_progress )
        if( check_moveresize_event( event ) )
            return;

	/* handle menu events specially */
    /* if (HandleMenuEvent (NULL, event) == True)
     *  return;
     */

    switch (event->x.type)
	{
        case KeyPress:
            /* if a key has been pressed and it's not one of those that cause
                warping, we know the warping is finished */
            HandleKeyPress (event);
            break;
        case ButtonPress:
            /* if warping, a button press, non-warp keypress, or pointer motion
            * indicates that the warp is done */
            if( get_flags( AfterStepState, ASS_WarpingMode ) )
                EndWarping();
            HandleButtonPress (event);
            break;
        case ButtonRelease:
            /* if warping, a button press, non-warp keypress, or pointer motion
            * indicates that the warp is done */
            if( get_flags( AfterStepState, ASS_WarpingMode ) )
                EndWarping();
            if( Scr.Windows->pressed )
                HandleButtonRelease (event);
            break;
        case MotionNotify:
            /* if warping, a button press, non-warp keypress, or pointer motion
            * indicates that the warp is done */
            if( get_flags( AfterStepState, ASS_WarpingMode ) )
                EndWarping();
            if( event->client && event->client->internal )
                event->client->internal->on_pointer_event( event->client->internal, event );
            break;
        case EnterNotify:
            HandleEnterNotify (event);
            break;
        case LeaveNotify:
            HandleLeaveNotify (event);
            break;
        case FocusIn:
            HandleFocusIn (event);
            break;
        case Expose:
            HandleExpose (event);
            break;
        case DestroyNotify:
            HandleDestroyNotify (event);
            break;
        case UnmapNotify:
            HandleUnmapNotify (event);
            break;
        case MapNotify:
            HandleMapNotify (event);
            break;
        case MapRequest:
            HandleMapRequest (event);
            break;
        case ConfigureNotify:
            if( event->client )
            {
                LOCAL_DEBUG_CALLER_OUT( "ConfigureNotify:(%p,%lx,asw->w=%lx,(%dx%d%+d%+d)", event->client, event->w, event->client->w,
                                                               event->x.xconfigure.width,
                                                               event->x.xconfigure.height,
                                                               event->x.xconfigure.x,
                                                               event->x.xconfigure.y  );
                on_window_moveresize( event->client, event->w );
            }
            break;
        case ConfigureRequest:
            HandleConfigureRequest (event);
            break;
        case PropertyNotify:
            HandlePropertyNotify (event);
            break;
        case ColormapNotify:
            HandleColormapNotify (event);
            break;
        case ClientMessage:
            HandleClientMessage (event);
            break;
        default:
#ifdef SHAPE
            if (event->x.type == (ShapeEventBase + ShapeNotify))
                HandleShapeNotify (event);
#endif /* SHAPE */

            break;
	}
	return;
}

/***********************************************************************
 * ___________________________ EVENT HANDLERS __________________________
 * Now its time for event handlers :
 ***********************************************************************/

/***********************************************************************
 *  Procedure:
 *  HandleFocusIn - client received focus
 ************************************************************************/
void
HandleFocusIn ( ASEvent *event )
{
    while (ASCheckTypedEvent (FocusIn, &event->x));
    DigestEvent( event );

    if( get_flags( AfterStepState, ASS_WarpingMode ) )
        ChangeWarpingFocus( event->client );

    if( Scr.Windows->focused != event->client )
        Scr.Windows->focused = NULL ;

    if( event->client == NULL && get_flags(AfterStepState, ASS_HousekeepingMode))
        return;
    /* note that hilite_aswindow changes value of Scr.Hilite!!! */
    hilite_aswindow( event->client );
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleKeyPress - key press event handler
 *
 ************************************************************************/
void
HandleKeyPress ( ASEvent *event )
{
	FuncKey      *key;
    XKeyEvent *xk = &(event->x.xkey);
    unsigned int modifier = (xk->state & nonlock_mods);

    for (key = Scr.Feel.FuncKeyRoot; key != NULL; key = key->next)
	{
		/* Here's a real hack - some systems have two keys with the
		 * same keysym and different keycodes. This converts all
		 * the cases to one keycode. */
        xk->keycode = XKeysymToKeycode (dpy, XKeycodeToKeysym (dpy, xk->keycode, 0));
        if ((key->keycode == xk->keycode) &&
			((key->mods == (modifier & (~LockMask))) ||
             (key->mods == AnyModifier)) && (key->cont & event->context))
		{
			/* check if the warp key was pressed */
            ExecuteFunction (key->fdata, event, -1);
			return;
		}
	}

	/* if a key has been pressed and it's not one of those that cause
	   warping, we know the warping is finished */
    if( get_flags( AfterStepState, ASS_WarpingMode ) )
        EndWarping();

	/* if we get here, no function key was bound to the key.  Send it
     * to the client if it was in a window we know about: */
    if (event->client)
        if (xk->window != event->client->w )
		{
            xk->window = event->client->w;
            XSendEvent (dpy, event->client->w, False, KeyPressMask, &(event->x));
		}
}


/***********************************************************************
 *
 *  Procedure:
 *	HandlePropertyNotify - property notify event handler
 *
 ***********************************************************************/
#define MAX_NAME_LEN 200L					   /* truncate to this many */
#define MAX_ICON_NAME_LEN 200L				   /* ditto */


Bool
update_transp_iter_func(void *data, void *aux_data)
{
    update_window_transparency( (ASWindow*)data );
    return True;
}

void
HandlePropertyNotify (ASEvent *event)
{
#ifdef I18N
	char        **list;
	int           num;
#endif
    ASWindow       *asw;
    XPropertyEvent *xprop = &(event->x.xproperty);
    Atom atom = xprop->atom ;

	/* force updates for "transparent" windows */
    if (atom == _XROOTPMAP_ID && event->w == Scr.Root)
	{
        read_xrootpmap_id (Scr.wmprops, (xprop->state == PropertyDelete));
        if(Scr.RootImage)
        {
            safe_asimage_destroy (Scr.RootImage);
            Scr.RootImage = NULL ;
        }
        if(Scr.RootBackground && Scr.RootBackground->im != NULL )
        {
            if( Scr.RootBackground->pmap && Scr.wmprops->root_pixmap == Scr.RootBackground->pmap );
                Scr.RootImage = dup_asimage(Scr.RootBackground->im) ;
        }

        iterate_asbidirlist( Scr.Windows->clients, update_transp_iter_func, NULL, NULL, False );

        /* use move_menu() to update transparent menus; this is a kludge, but it works */
#if 0                                          /* reimplement menu redrawing : */
        if ((*Scr.MSMenuTitle).texture_type == 129 || (*Scr.MSMenuItem).texture_type == 129 ||
			(*Scr.MSMenuHilite).texture_type == 129)
		{
            MenuRoot     *menu;

			for (menu = Scr.first_menu; menu != NULL; menu = menu->next)
				if ((*menu).is_mapped)
					move_menu (menu, (*menu).x, (*menu).y);
        }
#endif
    }

//    if( (asw = event->client) == NULL )
        return ;

    if( IsNameProp(atom))
    {
        show_debug( __FILE__, __FUNCTION__, __LINE__, "name prop changed..." );
        if( update_property_hints_manager( asw->w, xprop->atom,
                                        Scr.Look.supported_hints,
                                        asw->hints, asw->status ) )
        {
            broadcast_window_name( asw );
            broadcast_icon_name( asw );

            show_debug( __FILE__, __FUNCTION__, __LINE__, "New name is \"%s\", icon_name \"%s\"", ASWIN_NAME(asw), ASWIN_ICON_NAME(asw) );
            /* fix the name in the title bar */
            if (!ASWIN_GET_FLAGS(asw, AS_Iconic))
                on_window_title_changed( asw, True );
        }
        if( get_flags( Scr.Feel.flags, FollowTitleChanges) )
            on_window_hints_changed( asw );
    /* otherwise we should check if this is the status property that we change ourselves : */
    }else if( NeedToTrackPropChanges(atom) )
        on_window_hints_changed( asw );
    /* we have to do the complete refresh of hints, since we have to override WH_HINTS with database, etc. */
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleClientMessage - client message event handler
 *
 ************************************************************************/
void
HandleClientMessage (ASEvent *event)
{
    LOCAL_DEBUG_OUT("ClientMessage(\"%s\")", XGetAtomName( dpy, event->x.xclient.message_type ));

    if ((event->x.xclient.message_type == _XA_WM_CHANGE_STATE) &&
        (event->client) &&
        (event->x.xclient.data.l[0] == IconicState) &&
        !ASWIN_GET_FLAGS(event->client, AS_Iconic))
	{
        set_window_wm_state( event->client, True );
#ifdef ENABLE_DND
		/* Pass the event to the client window */
        if (event->x.xclient.window != event->client->w)
		{
            event->x.xclient.window = event->client->w;
            XSendEvent (dpy, event->client->w, True, NoEventMask, &(event->x));
		}
#endif
    }else if( event->x.xclient.message_type == _AS_BACKGROUND )
    {
        HandleBackgroundRequest( event );
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleExpose - expose event handler
 *
 ***********************************************************************/
void
HandleExpose ( ASEvent *event )
{
    /* do nothing on expose - we use doublebuffering !!! */
}



/***********************************************************************
 *
 *  Procedure:
 *	HandleDestroyNotify - DestroyNotify event handler
 *
 ***********************************************************************/
void
HandleDestroyNotify (ASEvent *event )
{
    if (event->client)
	{
        Destroy (event->client, True);
	}
}

/***********************************************************************
 *  Procedure:
 *	HandleMapRequest - MapRequest event handler
 ************************************************************************/
void
HandleMapRequest (ASEvent *event )
{
    /* If the window has never been mapped before ... */
    if (event->client == NULL)
    {
        if( (event->client = AddWindow (event->w)) == NULL )
            return;
    }else /* If no hints, or currently an icon, just "deiconify" */
        set_window_wm_state( event->client, True );
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleMapNotify - MapNotify event handler
 *
 ***********************************************************************/
void
HandleMapNotify ( ASEvent *event )
{
    ASWindow *asw = event->client;
    if ( asw == NULL || event->w == Scr.Root )
        return;

    if( event->w != asw->w )
    {
        if( asw->wm_state_transition == ASWT_Withdrawn2Iconic && event->w == asw->status->icon_window )
        {/* we finally reached iconic state : */
            complete_wm_state_transition( asw, IconicState );
        }
        return ;
    }
    if( asw->wm_state_transition == ASWT_StableState )
    {
        if( ASWIN_GET_FLAGS( asw, AS_Iconic ) )
            set_window_wm_state( asw, False );  /* client has requested deiconification */
        return ;                                /* otherwise it is redundand event */
    }
    if( get_flags(asw->wm_state_transition, ASWT_FROM_ICONIC ) )
    {
        if (get_flags( Scr.Feel.flags, ClickToFocus) )
            activate_aswindow (asw, True, False);
    }
    ASWIN_SET_FLAGS(asw, AS_Mapped);
    ASWIN_CLEAR_FLAGS(asw, AS_IconMapped);
    ASWIN_CLEAR_FLAGS(asw, AS_Iconic);
    complete_wm_state_transition( asw, NormalState );
    broadcast_config( M_MAP, asw );
    /* finally reaches Normal state */
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleUnmapNotify - UnmapNotify event handler
 *
 ************************************************************************/
void
HandleUnmapNotify (ASEvent *event )
{
	XEvent        dummy;
    ASWindow *asw = event->client ;
    Bool destroyed = False ;

    if ( event->x.xunmap.event == Scr.Root && asw == NULL )
        asw = window2ASWindow( event->x.xunmap.window );

    if ( asw == NULL || event->x.xunmap.window != asw->w )
		return;

    ASWIN_CLEAR_FLAGS(asw, AS_Mapped);
    ASWIN_CLEAR_FLAGS(asw, AS_UnMapPending);
    /* Window remains hilited even when unmapped !!!! */
    /* if (Scr.Hilite == asw )
        Scr.Hilite = NULL; */

    if (Scr.Windows->previous_active == asw)
        Scr.Windows->previous_active = NULL;

    if (Scr.Windows->focused == asw )
        focus_next_aswindow( asw );

    if( get_flags( asw->wm_state_transition, ASWT_TO_WITHDRAWN ))
    {/* redundand UnmapNotify - ignoring */
        return ;
    }
    if( get_flags( asw->wm_state_transition, ASWT_TO_ICONIC ))
    {/* we finally reached iconic state : */
        complete_wm_state_transition( asw, IconicState );
        return ;
    }
    /*
    * The program may have unmapped the client window, from either
    * NormalState or IconicState.  Handle the transition to WithdrawnState.
    */

    XGrabServer (dpy);
    destroyed = ASCheckTypedWindowEvent ( event->w, DestroyNotify, &dummy) ;
    event->client->wm_state_transition = ASWIN_GET_FLAGS(event->client, AS_Iconic)?ASWT_Iconic2Withdrawn:ASWT_Normal2Withdrawn ;
    Destroy (event->client, destroyed);               /* do not need to mash event before */
    XUngrabServer (dpy);
    ASFlush ();
}


/***********************************************************************
 *  Procedure:
 *	HandleButtonPress - ButtonPress event handler
 ***********************************************************************/
void
HandleButtonPress ( ASEvent *event )
{
	unsigned int  modifier;
	MouseButton  *MouseEntry;
    Bool          AShandled = False;
    ASWindow     *asw = event->client ;
    XButtonEvent *xbtn = &(event->x.xbutton);



	/* click to focus stuff goes here */
    if( asw != NULL )
    {
        Bool          focus_accepted = False;

        if (get_flags( Scr.Feel.flags, ClickToFocus) )
        {
            if ( asw != Scr.Windows->ungrabbed && (xbtn->state & nonlock_mods) == 0)
                focus_accepted = focus_aswindow(asw);
        }

        if (!ASWIN_GET_FLAGS(asw, AS_Visible))
        {
            if (get_flags(Scr.Feel.flags, ClickToRaise) && event->context == C_WINDOW
                && (Scr.Feel.RaiseButtons & (1 << xbtn->button)) )
                RaiseWindow (asw);
            else
            {
                if (Scr.Feel.AutoRaiseDelay > 0)
                {
                    SetTimer (Scr.Feel.AutoRaiseDelay);
                } else
                {
#ifdef CLICKY_MODE_1
                    if (event->w != asw->w)
#endif
                    {
                        if (Scr.Feel.AutoRaiseDelay == 0)
                            RaiseWindow (asw);
                    }
                }
            }
        }
        if (!ASWIN_GET_FLAGS(asw, AS_Iconic))
        {
            XSync (dpy, 0);
            XAllowEvents (dpy, (event->context == C_WINDOW) ? ReplayPointer : AsyncPointer, CurrentTime);
            XSync (dpy, 0);
        }
        if( focus_accepted )
            return;

        press_aswindow( asw, event->context );
    }

    /* we have to execute a function or pop up a menu : */
    modifier = (xbtn->state & nonlock_mods);
	/* need to search for an appropriate mouse binding */
    MouseEntry = Scr.Feel.MouseButtonRoot;
    while (MouseEntry != NULL)
	{
        if ((MouseEntry->Button == xbtn->button || MouseEntry->Button == 0) &&
            (MouseEntry->Context & event->context) &&
            (MouseEntry->Modifier == AnyModifier || MouseEntry->Modifier == modifier))
		{
			/* got a match, now process it */
            if (MouseEntry->fdata != NULL)
			{
                ExecuteFunction (MouseEntry->fdata, event, -1);
				AShandled = True;
				break;
			}
		}
		MouseEntry = MouseEntry->NextButton;
	}

	/* GNOME this click hasn't been taken by AfterStep */
    if (!AShandled && xbtn->window == Scr.Root)
	{
        XUngrabPointer (dpy, CurrentTime);
        XSendEvent (dpy, Scr.wmprops->wm_event_proxy, False, SubstructureNotifyMask, &(event->x));
	}
}

/***********************************************************************
 *  Procedure:
 *  HandleButtonRelease - De-press currently pressed window if all buttons are up
 ***********************************************************************/
void
HandleButtonRelease ( ASEvent *event )
{   /* click to focus stuff goes here */
LOCAL_DEBUG_CALLER_OUT("pressed(%p)->state(0x%X)", Scr.Windows->pressed, (event->x.xbutton.state&(Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask)) );
    if( (event->x.xbutton.state&(Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask)) == 0 )
        release_pressure();
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleEnterNotify - EnterNotify event handler
 *
 ************************************************************************/
void
HandleEnterNotify (ASEvent *event)
{
    XEnterWindowEvent *ewp = &(event->x.xcrossing);
	XEvent        d;
    ASWindow *asw = event->client;
    int i ;

	/* look for a matching leaveNotify which would nullify this enterNotify */
    if (ASCheckTypedWindowEvent ( ewp->window, LeaveNotify, &d))
	{
        on_astbar_pointer_action( NULL, 0, True );
        if ((d.xcrossing.mode == NotifyNormal) && (d.xcrossing.detail != NotifyInferior))
			return;
	}
/* an EnterEvent in one of the PanFrameWindows activates the Paging */
#ifndef NO_VIRTUAL
    for( i = 0 ; i < PAN_FRAME_SIDES ; i++ )
        if( Scr.PanFrame[i].isMapped && ewp->window == Scr.PanFrame[0].win)
        {
            int           delta_x = 0, delta_y = 0;

            /* this was in the HandleMotionNotify before, HEDU */
            HandlePaging (Scr.Feel.EdgeScrollX, Scr.Feel.EdgeScrollY,
                        &(ewp->x_root), &(ewp->y_root), &delta_x, &delta_y, True, event);
            return;
        }
#endif /* NO_VIRTUAL */

    if (ewp->window == Scr.Root)
	{
        if (!get_flags(Scr.Feel.flags, ClickToFocus|SloppyFocus))
            hide_focus();
        InstallRootColormap();
		return;
    }else if( event->context != C_WINDOW )
        InstallAfterStepColormap();

	/* make sure its for one of our windows */
    if (asw == NULL )
		return;

    if (ASWIN_HFLAGS(asw,AS_AcceptsFocus))
	{
        if (!get_flags(Scr.Feel.flags, ClickToFocus))
		{
            if (Scr.Windows->focused != asw)
                if (Scr.Feel.AutoRaiseDelay > 0 && !ASWIN_GET_FLAGS(asw, AS_Visible))
                    SetTimer (Scr.Feel.AutoRaiseDelay);
            focus_aswindow(asw);
		}
        if (!ASWIN_GET_FLAGS(asw, AS_Iconic) && event->context == C_WINDOW )
            InstallWindowColormaps (asw);
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleLeaveNotify - LeaveNotify event handler
 *
 ************************************************************************/
void
HandleLeaveNotify ( ASEvent *event )
{
    XEnterWindowEvent *ewp = &(event->x.xcrossing);
    /* If we leave the root window, then we're really moving
	 * another screen on a multiple screen display, and we
	 * need to de-focus and unhighlight to make sure that we
	 * don't end up with more than one highlighted window at a time */
    if (ewp->window == Scr.Root)
	{
        if (ewp->mode == NotifyNormal)
		{
            if (ewp->detail != NotifyInferior)
			{
                if (Scr.Windows->focused != NULL)
                    hide_focus();
                if (Scr.Windows->hilited != NULL)
                    hide_hilite();
            }
		}
	}
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleConfigureRequest - ConfigureRequest event handler
 *
 ************************************************************************/
void
HandleConfigureRequest ( ASEvent *event )
{
    XConfigureRequestEvent *cre = &(event->x.xconfigurerequest);
    ASWindow *asw = event->client ;
    XWindowChanges xwc;

    /*
	 * According to the July 27, 1988 ICCCM draft, we should ignore size and
	 * position fields in the WM_NORMAL_HINTS property when we map a window.
	 * Instead, we'll read the current geometry.  Therefore, we should respond
	 * to configuration requests for windows which have never been mapped.
	 */

    if (asw == NULL)
	{
        unsigned long xwcm;
        xwcm = cre->value_mask & (CWX | CWY | CWWidth | CWHeight | CWBorderWidth);
		xwc.x = cre->x;
		xwc.y = cre->y;
		xwc.width = cre->width;
		xwc.height = cre->height;
		xwc.border_width = cre->border_width;
        XConfigureWindow (dpy, event->w, xwcm, &xwc);
		return;
	}

    if (cre->value_mask & CWStackMode)
        restack_window( asw, (cre->value_mask & CWSibling)?cre->above:None, cre->detail );

    check_aswindow_shaped( asw );

    /* for restoring */
	if (cre->value_mask & CWBorderWidth)
        asw->status->border_width = cre->border_width;

    /* now we need to update window's anchor : */

    if( cre->value_mask & (CWWidth|CWHeight|CWX|CWY) )
    {
        XRectangle new_anchor = asw->anchor;
        int grav_x, grav_y ;
        get_gravity_offsets (asw->hints, &grav_x, &grav_y);


        if( cre->value_mask&CWWidth )
            new_anchor.width = cre->width ;
        if( cre->value_mask&CWHeight )
            new_anchor.height = cre->height ;
        if( cre->value_mask&CWX )
            new_anchor.x = make_anchor_pos ( asw->status, cre->x, new_anchor.width, Scr.Vx, grav_x, Scr.VxMax+Scr.MyDisplayWidth );
        if( cre->value_mask&CWY )
            new_anchor.y = make_anchor_pos ( asw->status, cre->y, new_anchor.height, Scr.Vy, grav_y, Scr.VyMax+Scr.MyDisplayHeight );

LOCAL_DEBUG_OUT( "old anchor(%dx%d%+d%+d), new_anchor(%dx%d%+d%+d)", asw->anchor.width, asw->anchor.height, asw->anchor.x, asw->anchor.y, new_anchor.width, new_anchor.height, new_anchor.x, new_anchor.y );
        asw->anchor = new_anchor ;
        on_window_status_changed( asw, True, True );
    }
}

/***********************************************************************
 *
 *  Procedure:
 *      HandleShapeNotify - shape notification event handler
 *
 ***********************************************************************/
#ifdef SHAPE
void
HandleShapeNotify (ASEvent *event)
{
    XShapeEvent  *sev = (XShapeEvent *) &(event->x);

    if (!event->client)
		return;
	if (sev->kind != ShapeBounding)
		return;
    if( sev->shaped )
        ASWIN_SET_FLAGS( event->client, AS_Shaped );
    else
        ASWIN_CLEAR_FLAGS( event->client, AS_Shaped );

    SetShape (event->client, 0);
}
#endif /* SHAPE */

#if 1										   /* see SetTimer() */
/**************************************************************************
 * For auto-raising windows, this routine is called
 *************************************************************************/
volatile int  alarmed = False;
void
AlarmHandler (int nonsense)
{
	alarmed = True;
    signal (SIGALRM, AlarmHandler);
}
#endif /* 1 */

/****************************************************************************
 * Start/Stops the auto-raise timer
 ****************************************************************************/
void
SetTimer (int delay)
{
#ifdef TIME_WITH_SYS_TIME
	struct itimerval value;

	value.it_value.tv_usec = 1000 * (delay % 1000);
	value.it_value.tv_sec = delay / 1000;
	value.it_interval.tv_usec = 0;
	value.it_interval.tv_sec = 0;
	setitimer (ITIMER_REAL, &value, NULL);
#endif
}

/***************************************************************************
 *
 * Waits for next X event, or for an auto-raise timeout.
 *
 ****************************************************************************/
void
afterstep_wait_pipes_input()
{
	fd_set        in_fdset, out_fdset;
	int           retval;
	struct timeval tv;
	struct timeval *t = NULL;
    int           max_fd = 0;

	FD_ZERO (&in_fdset);
	FD_ZERO (&out_fdset);

	FD_SET (x_fd, &in_fdset);
    max_fd = x_fd ;

    if (Module_fd >= 0)
    {

        FD_SET (Module_fd, &in_fdset);
        if (max_fd < Module_fd)
            max_fd = Module_fd;

    }

    {   /* adding all the modules pipes to our wait list */
        register int i = MIN(MODULES_NUM,Module_npipes) ;
        register module_t *list = MODULES_LIST ;
        while( --i >= 0 )
        {
            if (list[i].fd >= 0)
            {
                FD_SET (list[i].fd, &in_fdset);
                if (list[i].output_queue != NULL)
                    FD_SET (list[i].fd, &out_fdset);
                if (max_fd < list[i].fd)
                    max_fd = list[i].fd;
            }else /* man, this modules is dead! get rid of it - it stinks! */
                vector_remove_index( Modules, i );
        }
    }

	/* watch for timeouts */
	if (timer_delay_till_next_alarm ((time_t *) & tv.tv_sec, (time_t *) & tv.tv_usec))
		t = &tv;

#if 1										   /* see SetTimer() */
	{
		struct itimerval value;
		Window        child;
		/* Do this prior to the select() call, in case the timer already expired,
		 * in which case the select would never return. */
		if (alarmed)
		{
            alarmed = False;
            ASQueryPointerChild(Scr.Root,&child);
            if ((Scr.Windows->focused != NULL) && (child == get_window_frame(Scr.Windows->focused)))
                RaiseObscuredWindow(Scr.Windows->focused);
			return ;
		}
#ifndef TIME_WITH_SYS_TIME
		value.it_value.tv_usec = 0;
		value.it_value.tv_sec = 0;
#else
		getitimer (ITIMER_REAL, &value);
#endif
		if (value.it_value.tv_sec > 0 || value.it_value.tv_usec > 0)
			if (t == NULL || value.it_value.tv_sec < tv.tv_sec ||
				(value.it_value.tv_sec == tv.tv_sec && value.it_value.tv_usec < tv.tv_usec))
				t = &value.it_value;
	}
#endif /* 1 */

    retval = PORTABLE_SELECT(min (max_fd + 1, fd_width),&in_fdset,&out_fdset,NULL,t);

	if (retval > 0)
	{
        register module_t *list ;
        register int i ;
		/* check for incoming module connections */
        if (Module_fd >= 0)
            if (FD_ISSET (Module_fd, &in_fdset))
                if (AcceptModuleConnection(Module_fd) != -1)
                    show_progress("accepted module connection");
        /* note that we have to do it AFTER we accepted incoming connections as those alter the list */
        list = MODULES_LIST ;
        i = MIN(MODULES_NUM,Module_npipes) ;
        /* Check for module input. */
        while( --i >= 0 )
        {
            Bool has_input = FD_ISSET (list[i].fd, &in_fdset);
            Bool has_output = FD_ISSET (list[i].fd, &out_fdset);
            if( has_input || has_output )
                HandleModuleInOut(i, has_input, has_output);
        }
	}

	/* handle timeout events */
	timer_handle ();
}


/* old stuff : */
#if 0
int
afterstep_wait_pipes_input ()
{
    extern int    fd_width, x_fd;
	fd_set        in_fdset, out_fdset;
	int           i;
	int           retval;
	struct timeval tv;
	struct timeval *t = NULL;
	extern module_t *Module;
	int           max_fd = 0;

    FD_ZERO (&in_fdset);
	FD_SET (x_fd, &in_fdset);
	max_fd = x_fd;
	FD_ZERO (&out_fdset);

	if (module_fd >= 0)
	{
		FD_SET (module_fd, &in_fdset);
		if (max_fd < module_fd)
			max_fd = module_fd;
	}
	for (i = 0; i < npipes; i++)
		if (Module[i].fd >= 0)
		{
			FD_SET (Module[i].fd, &in_fdset);
			if (max_fd < Module[i].fd)
				max_fd = Module[i].fd;
			if (Module[i].output_queue != NULL)
				FD_SET (Module[i].fd, &out_fdset);
		}
	/* watch for timeouts */
	if (timer_delay_till_next_alarm ((time_t *) & tv.tv_sec, (time_t *) & tv.tv_usec))
		t = &tv;

#if 1										   /* see SetTimer() */
	{
		struct itimerval value;
		Window        child;

		/* Do this prior to the select() call, in case the timer already expired,
		 * in which case the select would never return. */
		if (alarmed)
		{
			alarmed = False;
            ASQueryPointerChild(None,&child);
            if ((Scr.Windows->focused != NULL) && (child == Scr.Windows->focused->frame))
			{
                if (!(Scr.Windows->focused->flags & VISIBLE))
				{
                    RaiseWindow (Scr.Windows->focused);
				}
			}
			return 0;
		}
#ifndef TIME_WITH_SYS_TIME
		value.it_value.tv_usec = 0;
		value.it_value.tv_sec = 0;
#else
		getitimer (ITIMER_REAL, &value);
#endif
		if (value.it_value.tv_sec > 0 || value.it_value.tv_usec > 0)
			if (t == NULL || value.it_value.tv_sec < tv.tv_sec ||
				(value.it_value.tv_sec == tv.tv_sec && value.it_value.tv_usec < tv.tv_usec))
				t = &value.it_value;
	}
#endif /* 1 */

    ReapChildren ();
	XFlush (dpy);

#ifdef __hpux
	retval = select (min (max_fd + 1, fd_width), (int *)&in_fdset, (int *)&out_fdset, NULL, t);
#else
	retval = select (min (max_fd + 1, fd_width), &in_fdset, &out_fdset, NULL, t);
#endif

	/* check for incoming module connections */
	if (module_fd >= 0 && FD_ISSET (module_fd, &in_fdset))
	{
		if (module_accept (module_fd) != -1)
			fprintf (stderr, "accepted module connection\n");
	}
	if (retval > 0)
	{
		/* Check for module input. */
		for (i = 0; i < npipes; i++)
		{
			if (Module[i].fd >= 0 && FD_ISSET (Module[i].fd, &in_fdset))
				HandleModuleInput (i);
			if (Module[i].fd >= 0 && FD_ISSET (Module[i].fd, &out_fdset))
				FlushQueue (i);
		}
	}

	/* handle timeout events */
	timer_handle ();
	return 0;
}
#endif
