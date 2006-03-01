/*
 * Copyright (C) 2003 Sasha Vasko
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

#include "asinternals.h"

#include <limits.h>
#include <sys/types.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <unistd.h>
#include <signal.h>

#include "../../libAfterStep/wmprops.h"
#include "../../libAfterStep/moveresize.h"

#include <X11/keysym.h>
#ifdef XSHMIMAGE
# include <sys/ipc.h>
# include <sys/shm.h>
# include <X11/extensions/XShm.h>
#endif

/***********************************************************************
 *  _______________________EVENT HANDLING ______________________________
 *
 *  HandleEvents  - event loop
 *  DigestEvent   - preprocesses event - finds ASWindow, context etc.
 *  DispatchEvent - calls appropriate handler for the event
 ************************************************************************/
void DigestEvent    ( ASEvent *event );
void afterstep_wait_pipes_input (int timeout_sec);

void
HandleEvents ()
{
    /* this is the only loop that allowed to run ExecutePendingFunctions(); */
    ASEvent event;
    while (True)
    {
        while(XPending (dpy))
        {
            if( ASNextEvent (&(event.x), True) )
            {
                DigestEvent( &event );
                DispatchEvent( &event, False );
            }
            ASSync( False );
            /* before we exec any function - we ought to process any Unmap and Destroy
             * events to handle all the pending window destroys : */
            while( ASCheckTypedEvent( DestroyNotify, &(event.x)) ||
                   ASCheckTypedEvent( UnmapNotify, &(event.x)) ||
                   ASCheckMaskEvent( FocusChangeMask, &(event.x)))
            {
                DigestEvent( &event );
                DispatchEvent( &event, False );
            }
            ExecutePendingFunctions();
        }
        afterstep_wait_pipes_input (0);
        ExecutePendingFunctions();
    }
}

/***************************************************************************
 * Wait for all mouse buttons to be released
 * This can ease some confusion on the part of the user sometimes
 *
 * Discard superflous button events during this wait period.
 ***************************************************************************/
#define MOVERESIZE_LOOP_MASK 	(KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask| \
                             	EnterWindowMask|LeaveWindowMask|PointerMotionMask|PointerMotionHintMask| \
							 	Button1MotionMask|Button2MotionMask|Button3MotionMask|Button4MotionMask| \
							 	Button5MotionMask|ButtonMotionMask|KeymapStateMask| \
								StructureNotifyMask|SubstructureNotifyMask)


void
InteractiveMoveLoop ()
{
    ASEvent event;
    Bool has_x_events = False ;
    while (Scr.moveresize_in_progress != NULL )
    {
LOCAL_DEBUG_OUT( "checking masked events ...%s", "" );
        while((has_x_events = ASCheckMaskEvent (MOVERESIZE_LOOP_MASK, &(event.x))))
        {
			DigestEvent( &event );
            DispatchEvent( &event, False );
            if( Scr.moveresize_in_progress == NULL )
                return;
        }
        afterstep_wait_pipes_input (0);
    }
}



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
WaitEventLoop( ASEvent *event, int finish_event_type, long timeout, ASHintWindow *hint )
{
	unsigned long mask = ButtonPressMask | ButtonReleaseMask |
						 ExposureMask | KeyPressMask | ButtonMotionMask |
						 PointerMotionMask	/* | EnterWindowMask | LeaveWindowMask */;
	Bool done = False ;

    while (!done)
	{
		/* block until there is an event */
        ASFlushIfEmpty();
        ASMaskEvent ( mask, &(event->x));

        if( event->x.type == KeyPress )
            KeyboardShortcuts (&(event->x), finish_event_type, 20);
            /* above line might have changed event code !!!*/
        else if (event->x.type == ButtonPress )
			XAllowEvents (dpy, ReplayPointer, CurrentTime);

        if( event->x.type == finish_event_type )
		{
			done = True ;
			if( event->x.xbutton.window == Scr.Root )
				event->x.xbutton.window = event->x.xbutton.subwindow ;
			/* otherwise event will be reported as if it occured relative to
			   root window */
		}
        DigestEvent( event );
        DispatchEvent( event, done );
		if( event->x.type == MotionNotify && hint ) 
			update_ashint_geometry( hint, False );
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
	int dx = 0, dy = 0;
	int x_orig = event->x.xbutton.x_root ;
	int y_orig = event->x.xbutton.y_root ;
    /* we are in the middle of running Complex function - we must only do mandatory
     * processing on received events, but do not actually handle them !!
     * Client window affected, as well as the context must not change -
     * only the X event could */
    ASEvent tmp_event ;
    register XEvent *xevt = &(tmp_event.x) ;

    ASSync(False);
	start_ticker (click_time);
    do
	{
        sleep_a_millisec (10);
        if (ASCheckMaskEvent (end_mask, xevt))
        {
            DigestEvent( &tmp_event );
            event->x = *xevt ;                 /* everything else must remain the same !!! */
	        DispatchEvent( event, True );
            return True;
        }
        if( is_tick() )
			break;

        if (ASCheckMaskEvent ( ButtonMotionMask | PointerMotionMask, xevt))
		{
			dx = x_orig - xevt->xmotion.x_root;
			dy = y_orig - xevt->xmotion.y_root;
            DigestEvent( &tmp_event );
		    event->x = *xevt ;                 /* everything else must remain the same !!! */
		}
    }while( dx > -5 && dx < 5 && dy > -5 && dy < 5 );

	return False;
}

ASWindow *
WaitWindowLoop( char *pattern, long timeout )
{
	Bool done = False ;
	ASEvent event ;
	Bool has_x_events ;
	time_t end_time = (timeout<=0?DEFAULT_WINDOW_WAIT_TIMEOUT:timeout)/100 ;
	time_t click_end_time = end_time / 4;
	time_t start_time = time(NULL);
	ASWindow *asw = NULL ;
	ASHintWindow *hint ;
	char *text ;

	end_time += start_time ;
	click_end_time += start_time ;

	if( pattern == NULL || pattern[0] == '\0' )
		return NULL ;

	LOCAL_DEBUG_OUT( "waiting for \"%s\"", pattern );
    if( (asw = complex_pattern2ASWindow( pattern )) != NULL )
  		return asw;

	text = safemalloc(64+strlen(pattern)+1);
	sprintf( text, "Waiting for window matching \"%s\" ... Press button to cancel.", pattern );
	hint = create_ashint_window( ASDefaultScr, &(Scr.Look), text);
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
					/* we do not want user to do anything interactive at that time - hence
					   deffered == True */
                    DispatchEvent( &event, True );
					if( event.x.type == ButtonPress || event.x.type == KeyPress )
					{
						end_time = click_end_time ;
						break;
					}else if( event.x.type == MotionNotify && hint ) 
						update_ashint_geometry( hint, False );
                    
					if( (event.x.type == MapNotify || event.x.type == PropertyNotify )&& event.client )
                        if( (asw = complex_pattern2ASWindow( pattern )) != NULL )
						{
							destroy_ashint_window( &hint );
                            return asw;
						}
                }
			}
			if( time(NULL) > end_time )
			{
			    done = True ;
				break;
			}

		}while( has_x_events );

		if( time(NULL) > end_time )
			break;
		afterstep_wait_pipes_input (1);
	}
	destroy_ashint_window( &hint );
    return NULL;
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
        DispatchEvent( &event, False );
        ASSync(False);
    }
}

void
MapConfigureNotifyLoop()
{
    ASEvent event;

	do
	{
		if( !ASCheckTypedEvent(MapNotify,&(event.x)) )
			if( !ASCheckTypedEvent(ConfigureNotify,&(event.x)) )
				return ;
        DigestEvent( &event );
        DispatchEvent( &event, False );
        ASSync(False);
    }while(1);
}

void
DigestEvent( ASEvent *event )
{
    register int i ;
    setup_asevent_from_xevent( event );
    /* in housekeeping mode we handle pointer events only as applied to root window ! */
    if( Scr.moveresize_in_progress && (event->eclass & ASE_POINTER_EVENTS) != 0)
    {
	    event->context = C_ROOT ;
    	event->widget = Scr.RootCanvas ;
        event->client = NULL;
        /* we have to do this at all times !!!! */
        if( event->x.type == ButtonRelease && Scr.Windows->pressed )
            release_pressure();
    }else
	{
		if( event->w == Scr.Root || 
			event->w == Scr.ServiceWin ||
			event->w == Scr.SizeWindow )
		{
	    	event->context = C_ROOT ; 
		}else 
			event->context = C_NO_CONTEXT ;

		if( event->context == C_ROOT )
		{	
    		event->widget = Scr.RootCanvas ;
			event->client = NULL ;
		}else
		{
			if( (event->eclass & ASE_POINTER_EVENTS) != 0 && is_balloon_click( &(event->x) ) )
			{	
				event->client = NULL ;
				event->widget = NULL ;	 
			}else
			{	
				event->widget = NULL ;	
				event->client = window2ASWindow( event->w );
			}
		}	 
	}

    if( (event->eclass & ASE_POINTER_EVENTS) != 0 && event->client )
    {
        /* now lets determine the context of the event : (former GetContext)*/
        Window   w = event->w ;
        ASWindow *asw = event->client ;
        XKeyEvent *xk = &(event->x.xkey);
        ASCanvas  *canvas = asw->frame_canvas ;
        ASTBarData *pointer_bar = NULL ;
		int pointer_root_x = xk->x_root;
		int pointer_root_y = xk->y_root;
		static int last_pointer_root_x = -1, last_pointer_root_y = -1; 
		int tbar_side = ASWIN_HFLAGS(asw, AS_VerticalTitle)?FR_W:FR_N ; 

        /* Since key presses and button presses are grabbed in the frame
         * when we have re-parented windows, we need to find out the real
         * window where the event occured */
        if (!ASWIN_GET_FLAGS(asw, AS_Iconic))
        {
			if( w != asw->client_canvas->w )
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
            }else
			{	/* we are on the border of the frame : see what side ofthe frame we are on */
				event->context = C_FRAME ;
				if( pointer_root_x < asw->frame_canvas->root_x+(int)asw->frame_canvas->bw )
					pointer_root_x = asw->frame_canvas->root_x+(int)asw->frame_canvas->bw ;
				else if( pointer_root_x >= asw->frame_canvas->root_x+(int)asw->frame_canvas->bw+(int)asw->frame_canvas->width )
					pointer_root_x = asw->frame_canvas->root_x+(int)asw->frame_canvas->bw+(int)asw->frame_canvas->width-1;
				if( pointer_root_y < asw->frame_canvas->root_y+(int)asw->frame_canvas->bw )
					pointer_root_y = asw->frame_canvas->root_y+(int)asw->frame_canvas->bw ;
				else if( pointer_root_y >= asw->frame_canvas->root_y+(int)asw->frame_canvas->bw+(int)asw->frame_canvas->height )
					pointer_root_y = asw->frame_canvas->root_y+(int)asw->frame_canvas->bw+(int)asw->frame_canvas->height-1;
				else
					event->context = C_CLIENT ;

			}

	        if (ASWIN_GET_FLAGS(asw, AS_Shaded) && canvas != asw->frame_sides[tbar_side] )
			{
				event->context = C_NO_CONTEXT ;				
				XRaiseWindow( dpy, asw->frame_sides[tbar_side]->w );
			}else if( w != asw->frame )
            {
                if( event->w == asw->frame )
                {
                    xk->x = pointer_root_x - (canvas->root_x+(int)canvas->bw) ;
                    xk->y = pointer_root_y - (canvas->root_y+(int)canvas->bw) ;
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
                    (tbar_context = check_astbar_point( asw->tbar, pointer_root_x, pointer_root_y )) != C_NO_CONTEXT )
                {
                    event->context = tbar_context ;
                    pointer_bar = asw->tbar ;
                }else
                {
                    for( i = 0 ; i < FRAME_PARTS ; ++i )
                        if( asw->frame_bars[i] != NULL &&
                            (tbar_context = check_astbar_point( asw->frame_bars[i], pointer_root_x, pointer_root_y )) != C_NO_CONTEXT )
                        {
                            event->context = tbar_context ;
                            pointer_bar = asw->frame_bars[i] ;
                            break;
                        }
                }
            }
			if( event->context == C_NO_CONTEXT && get_flags(Scr.Feel.flags, ClickToFocus) )
			{
				w = asw->frame ; 
				event->context = C_FRAME ;
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
                    int c = check_astbar_point( asw->icon_title, pointer_root_x, pointer_root_y );
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

		if( pointer_bar != NULL ) 
		{	
        	on_astbar_pointer_action( pointer_bar, event->context, 
								  	(event->x.type == LeaveNotify),
								  	(last_pointer_root_x != pointer_root_x || last_pointer_root_y != pointer_root_y) );
		}		
		last_pointer_root_x = pointer_root_x ;
		last_pointer_root_y = pointer_root_y ;

		if( asw != NULL && w != asw->w && w != asw->frame && event->context != C_NO_CONTEXT )
			apply_context_cursor( w, &(Scr.Feel), event->context );
        event->widget  = canvas ;
        /* we have to do this at all times !!!! */
        /* if( event->x.type == ButtonRelease && Scr.Windows->pressed )
            release_pressure(); */
    }
    SHOW_EVENT_TRACE(event);
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
DispatchEvent ( ASEvent *event, Bool deffered )
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
            HandleButtonPress (event, deffered);
            break;
        case ButtonRelease:
            /* if warping, a button press, non-warp keypress, or pointer motion
            * indicates that the warp is done */
            if( get_flags( AfterStepState, ASS_WarpingMode ) )
                EndWarping();
            if( Scr.Windows->pressed )
                HandleButtonRelease (event, deffered);
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
		case SelectionClear :
			HandleSelectionClear(event);
		    break ;
        default:
#ifdef SHAPE
            if (event->x.type == (Scr.ShapeEventBase + ShapeNotify))
                HandleShapeNotify (event);
#endif /* SHAPE */
#ifdef XSHMIMAGE
			LOCAL_DEBUG_OUT( "XSHMIMAGE> EVENT : completion_type = %d, event->type = %d ", Scr.ShmCompletionEventType, event->x.type );
			if( event->x.type == Scr.ShmCompletionEventType )
				HandleShmCompletion(event);
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
    int events_count = 0 ;
	while (ASCheckTypedEvent (FocusIn, &event->x)) events_count++;
	if( events_count > 0 ) 
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
	int m ; 

	/* Here's a real hack - some systems have two keys with the
		* same keysym and different keycodes. This converts all
		* the cases to one keycode. */
	for( m = 0 ; m < 8 ; ++m ) 
	{	
		KeySym keysym = XKeycodeToKeysym (dpy, xk->keycode, m); 
		int keycode ; 
		if( keysym == NoSymbol ) 
			continue;
    	if( (keycode = XKeysymToKeycode (dpy, keysym)) == 0 )
			continue;
		xk->keycode = keycode;

    	for (key = Scr.Feel.FuncKeyRoot; key != NULL; key = key->next)
		{
        	if ((key->keycode == xk->keycode) &&
				((key->mods == (modifier & (~LockMask))) ||
             	(key->mods == AnyModifier)) && (key->cont & event->context))
			{
				/* check if the warp key was pressed */
            	ExecuteFunction (key->fdata, event, -1);
				return;
			}
		}
	}
	/* if a key has been pressed and it's not one of those that cause
	   warping, we know the warping is finished */
    if( get_flags( AfterStepState, ASS_WarpingMode ) )
        EndWarping();

	LOCAL_DEBUG_OUT( "client = %p, context = %s", event->client, context2text((event)->context));
	/* if we get here, no function key was bound to the key.  Send it
     * to the client if it was in a window we know about: */
    if (event->client)
	{
		LOCAL_DEBUG_OUT( "internal = %p", event->client->internal );
		if( event->client->internal && event->client->internal->on_keyboard_event )
            event->client->internal->on_keyboard_event( event->client->internal, event );
	  	else if (xk->window != event->client->w )
		{
            xk->window = event->client->w;
            XSendEvent (dpy, event->client->w, False, KeyPressMask, &(event->x));
		}
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
	ASWindow *asw = (ASWindow*)data;
	if( !check_window_offscreen(asw) )
	{	
    	update_window_transparency( asw, True );
		if( asw->internal && asw->internal->on_root_background_changed ) 
			asw->internal->on_root_background_changed( asw->internal );
	}
    return True;
}

void
HandlePropertyNotify (ASEvent *event)
{
    ASWindow       *asw;
    XPropertyEvent *xprop = &(event->x.xproperty);
    Atom atom = xprop->atom ;
	XEvent prop_xev ; 

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
            if( Scr.RootBackground->pmap && Scr.wmprops->root_pixmap == Scr.RootBackground->pmap )
                Scr.RootImage = dup_asimage(Scr.RootBackground->im) ;
        }
		if( Scr.wmprops->as_root_pixmap != Scr.wmprops->root_pixmap ) 
			set_as_background(Scr.wmprops, Scr.wmprops->root_pixmap );

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
		return;
    }

    if( (asw = event->client) == NULL || ASWIN_GET_FLAGS( asw, AS_Dead ))
	{
		if( event->w != Scr.Root )
			while (XCheckTypedWindowEvent (dpy, event->w, PropertyNotify, &prop_xev));
        return ;
	}
    else
    {
		char *prop_name = NULL;
        LOCAL_DEBUG_OUT( "property %s", (prop_name = XGetAtomName( dpy, atom )) );
		if( prop_name )
	    	XFree( prop_name );
    }
    if( IsNameProp(atom))
    {
		char *old_name = get_flags( asw->internal_flags, ASWF_NameChanged )?NULL:mystrdup( ASWIN_NAME(asw) );
		
		/* we want to check if there were some more events generated 
		 * as window names tend to change multiple properties : */
		while (XCheckTypedWindowEvent (dpy, asw->w, PropertyNotify, &prop_xev))
			if( !IsNameProp(prop_xev.xproperty.atom) ) 
			{	
				XPutBackEvent( dpy, &prop_xev );
				break;
			}
		
		/*ASFlagType old_hflags = asw->hints->flags ; */
		show_debug( __FILE__, __FUNCTION__, __LINE__, "name prop changed..." );
        if( get_flags( Scr.Feel.flags, FollowTitleChanges) )
    	    on_window_hints_changed( asw );
    	else if( update_property_hints_manager( asw->w, xprop->atom,
             	                           		Scr.Look.supported_hints,
												Database,
                                        		asw->hints, asw->status ) )
        {
            if( ASWIN_GET_FLAGS( asw, AS_Dead ) )
				return;
			show_debug( __FILE__, __FUNCTION__, __LINE__, "New name is \"%s\", icon_name \"%s\", following title change ? %s", 
				        ASWIN_NAME(asw), ASWIN_ICON_NAME(asw), get_flags( Scr.Feel.flags, FollowTitleChanges)?"yes":"no" );
	    	LOCAL_DEBUG_OUT( "hints flags = %lX, ShortLived ? %lX ", asw->hints->flags, ASWIN_HFLAGS( asw, AS_ShortLived ) );
			if( old_name && strcmp( old_name, ASWIN_NAME(asw) ) != 0 )
				set_flags( asw->internal_flags, ASWF_NameChanged );
            /* fix the name in the title bar */
			if (!ASWIN_GET_FLAGS(asw, AS_Iconic))
       	    	on_window_title_changed( asw, True );
			broadcast_res_names( asw );
        	broadcast_window_name( asw );
       		broadcast_icon_name( asw );
        }
		if( old_name )
			free( old_name );
    	LOCAL_DEBUG_OUT( "hints flags = %lX, ShortLived ? %lX ", asw->hints->flags, ASWIN_HFLAGS( asw, AS_ShortLived ) );
	/* otherwise we should check if this is the status property that we change ourselves : */
    }else if( atom == XA_WM_COMMAND || atom == XA_WM_CLIENT_MACHINE )
	{
		update_cmd_line_hints (asw->w, atom, asw->hints, asw->status );
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
    char *aname = NULL ;
    LOCAL_DEBUG_OUT("ClientMessage(\"%s\")", (aname = XGetAtomName( dpy, event->x.xclient.message_type )));
    if( aname != NULL )
        XFree( aname );

    if ((event->x.xclient.message_type == _XA_WM_CHANGE_STATE) &&
        (event->client) &&
        (event->x.xclient.data.l[0] == IconicState) &&
        !ASWIN_GET_FLAGS(event->client, AS_Iconic))
	{
        set_window_wm_state( event->client, True, False );
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
    }else if( event->x.xclient.message_type == _XA_NET_WM_STATE && event->client != NULL )
	{
		ASFlagType extwm_flags = 0, as_flags = 0;
		CARD32 props[2] ;
		XClientMessageEvent *xcli = &(event->x.xclient) ;
		props[0] = xcli->data.l[1] ;
		props[1] = xcli->data.l[2] ;

		translate_atom_list (&extwm_flags, EXTWM_State, &props[0], 2);
		/* now we need to translate EXTWM flags into AS flags : */
		as_flags = extwm_state2as_state_flags( extwm_flags );
		if( xcli->data.l[0] == EXTWM_StateRemove ) 
			as_flags = ASWIN_GET_FLAGS(event->client,as_flags);
		else if( xcli->data.l[0] == EXTWM_StateAdd ) 
			as_flags = as_flags&(~ASWIN_GET_FLAGS(event->client,as_flags));
		if( as_flags != 0 ) 
			toggle_aswindow_status(event->client, as_flags );
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
        if( (event->client = AddWindow (event->w, True)) == NULL )
            return;
    }else /* If no hints, or currently an icon, just "deiconify" */
        set_window_wm_state( event->client, False, True );
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
    Bool force_activation = False ;
	Bool no_focus = False ;

    if ( asw == NULL || event->w == Scr.Root )
        return;

    LOCAL_DEBUG_OUT( "asw->w = %lX, event->w = %lX", asw->w, event->w );
    if( event->w != asw->w )
    {
        if( asw->wm_state_transition == ASWT_Withdrawn2Iconic && event->w == asw->status->icon_window )
        {/* we finally reached iconic state : */
            complete_wm_state_transition( asw, IconicState );
        }
        return ;
    }

	if( asw->wm_state_transition == ASWT_Withdrawn2Normal )
	{
		if( ASWIN_HFLAGS(asw, AS_FocusOnMap) )
			force_activation = True;
		else
			no_focus = True ;
	}
    LOCAL_DEBUG_OUT( "asw->wm_state_transition = %d", asw->wm_state_transition );

	if( asw->wm_state_transition == ASWT_StableState )
    {
        if( ASWIN_GET_FLAGS( asw, AS_Iconic ) )
            set_window_wm_state( asw, False, False );  /* client has requested deiconification */
        return ;                                /* otherwise it is redundand event */
    }
    if( get_flags(asw->wm_state_transition, ASWT_FROM_ICONIC ) )
        if (get_flags( Scr.Feel.flags, ClickToFocus) )
            force_activation = True;

    ASWIN_SET_FLAGS(asw, AS_Mapped);
    ASWIN_CLEAR_FLAGS(asw, AS_IconMapped);
    ASWIN_CLEAR_FLAGS(asw, AS_Iconic);
    complete_wm_state_transition( asw, NormalState );
	LOCAL_DEBUG_OUT( "no_focus = %d, force_activation = %d, AcceptsFocus = %ld", no_focus, force_activation, ASWIN_HFLAGS(asw,AS_AcceptsFocus) );
	if( !no_focus && ASWIN_HFLAGS(asw,AS_AcceptsFocus) )
    	activate_aswindow (asw, force_activation, False);
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

    grab_server();
    destroyed = ASCheckTypedWindowEvent ( event->w, DestroyNotify, &dummy) ;
	LOCAL_DEBUG_OUT("wm_state_transition = 0x%X", asw->wm_state_transition );
	if( !get_flags( asw->wm_state_transition, ASWT_FROM_WITHDRAWN ) )
    	asw->wm_state_transition = ASWIN_GET_FLAGS(asw, AS_Iconic)?ASWT_Iconic2Withdrawn:ASWT_Normal2Withdrawn ;
	else
		asw->wm_state_transition = ASWT_Withdrawn2Withdrawn ;
    Destroy (asw, destroyed);               /* do not need to mash event before */
    ungrab_server();
    ASFlush ();
}


/***********************************************************************
 *  Procedure:
 *	HandleButtonPress - ButtonPress event handler
 ***********************************************************************/
void
HandleButtonPress ( ASEvent *event, Bool deffered )
{
	unsigned int  modifier;
	MouseButton  *MouseEntry;
    Bool          AShandled = False;
    ASWindow     *asw = event->client ;
    XButtonEvent *xbtn = &(event->x.xbutton);
	Bool 		  raise_on_click = False ; 
    Bool 		  focus_accepted = False ;
    Bool 		  eat_click = False;
	Bool 		  activate_window = False ;

	/* click to focus stuff goes here */
    if( asw != NULL )
    {
		LOCAL_DEBUG_OUT( "deferred = %d, button = %X", deffered, (event->context&(~C_TButtonAll)) );
        /* if all we do is pressing titlebar buttons - then we should not raise/focus window !!! */
        if( !deffered )
        {
			if( (event->context&(~C_TButtonAll)) != 0 )
			{	
            	if (get_flags( Scr.Feel.flags, ClickToFocus) )
      			{
					LOCAL_DEBUG_OUT( "asw = %p, ungrabbed = %p, nonlock_mods = %x", asw, Scr.Windows->ungrabbed, (xbtn->state & nonlock_mods) );
          			if ( asw != Scr.Windows->ungrabbed && (xbtn->state & nonlock_mods) == 0)
                	{
						if( get_flags( Scr.Feel.flags, EatFocusClick ) )
						{
							if( Scr.Windows->focused != asw )
								if( (focus_accepted = activate_aswindow( asw, False, False)) )
                        			eat_click = True ;
						}else if( Scr.Windows->focused != asw )
							activate_window = True;
						LOCAL_DEBUG_OUT( "eat_click = %d", eat_click );
                	}
	        	}

            	if ( get_flags(Scr.Feel.flags, ClickToRaise) )
					raise_on_click = ( Scr.Feel.RaiseButtons == 0 || (Scr.Feel.RaiseButtons & (1 << xbtn->button))) ; 
			}
			
			if (!ASWIN_GET_FLAGS(asw, AS_Iconic))
  			{
      			XSync (dpy, 0);
        		XAllowEvents (dpy, (event->context == C_WINDOW) ? ReplayPointer : AsyncPointer, CurrentTime);
	    		XSync (dpy, 0);
  			}
		} /* !deffered */

        press_aswindow( asw, event->context );
    }


	if( !deffered && !eat_click)
	{
		LOCAL_DEBUG_OUT( "checking for associated functions...%s","" );		
	    /* we have to execute a function or pop up a menu : */
  		modifier = (xbtn->state & nonlock_mods);
		LOCAL_DEBUG_OUT( "state = %X, modifier = %X", xbtn->state, modifier);		   
		/* need to search for an appropriate mouse binding */
  		MouseEntry = Scr.Feel.MouseButtonRoot;
	    while (MouseEntry != NULL)
		{
			/*LOCAL_DEBUG_OUT( "mouse fdata %p button %d + modifier %X has context %lx", MouseEntry->fdata, MouseEntry->Button, MouseEntry->Modifier, get_flags(MouseEntry->Context, event->context) );*/
      		if ((MouseEntry->Button == xbtn->button || MouseEntry->Button == 0) &&
          		(MouseEntry->Context & event->context) &&
	            (MouseEntry->Modifier == AnyModifier || MouseEntry->Modifier == modifier))
			{
				/* got a match, now process it */
	            if (MouseEntry->fdata != NULL)
				{
      		        ExecuteFunction (MouseEntry->fdata, event, -1);
					raise_on_click = False ;
					AShandled = True;
					break;
				}
			}
			MouseEntry = MouseEntry->NextButton;
		}
	}

	LOCAL_DEBUG_OUT( "ashandled = %d, context = %X", AShandled, (event->context&(C_CLIENT|C_TITLE)) );
	if( activate_window && (!AShandled || (event->context&(C_CLIENT|C_TITLE)))) 
		focus_accepted = activate_aswindow( asw, False, False);

	if( raise_on_click && (asw->internal == NULL || (event->context&C_CLIENT) == 0 ) )
    	restack_window(asw,None,focus_accepted?Above:TopIf);

	/* GNOME this click hasn't been taken by AfterStep */
    if (!deffered && !AShandled && !eat_click && xbtn->window == Scr.Root)
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
HandleButtonRelease ( ASEvent *event, Bool deffered )
{   /* click to focus stuff goes here */
LOCAL_DEBUG_CALLER_OUT("pressed(%p)->state(0x%X)", Scr.Windows->pressed, (event->x.xbutton.state&(Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask)) );
    if( (event->x.xbutton.state&AllButtonMask) == (Button1Mask<<(event->x.xbutton.button-Button1)) )
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
    if (ewp->window != Scr.Root)
    	if (ASCheckTypedWindowEvent ( ewp->window, LeaveNotify, &d))
		{
        	on_astbar_pointer_action( NULL, 0, True, False );
        	if ((d.xcrossing.mode == NotifyNormal) && (d.xcrossing.detail != NotifyInferior))
				return;
		}
/* an EnterEvent in one of the PanFrameWindows activates the Paging */
#ifndef NO_VIRTUAL
    for( i = 0 ; i < PAN_FRAME_SIDES ; i++ )
    {
        LOCAL_DEBUG_OUT("checking panframe %d, mapped %d", i, Scr.PanFrame[i].isMapped );
        if( Scr.PanFrame[i].isMapped && ewp->window == Scr.PanFrame[i].win)
        {
            int           delta_x = 0, delta_y = 0;

            /* this was in the HandleMotionNotify before, HEDU */
            HandlePaging (Scr.Feel.EdgeScrollX, Scr.Feel.EdgeScrollY,
                        &(ewp->x_root), &(ewp->y_root), &delta_x, &delta_y, True, event);
            return;
        }
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
        if (!get_flags(Scr.Feel.flags, ClickToFocus) || asw->internal != NULL )
		{
            if (Scr.Windows->focused != asw)
                activate_aswindow(asw, False, False);
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

	LOCAL_DEBUG_OUT( "cre={0x%lx, geom = %dx%d%+d%+d} ", cre->value_mask, cre->width, cre->height,
	cre->x, cre->y );
    if (asw == NULL)
	{
        unsigned long xwcm;
        xwcm = cre->value_mask & (CWX | CWY | CWWidth | CWHeight | CWBorderWidth);
		xwc.x = cre->x;
		xwc.y = cre->y;
		xwc.width = cre->width;
		xwc.height = cre->height;
		xwc.border_width = cre->border_width;
		LOCAL_DEBUG_OUT( "Configuring untracked window %lX to %dx%d%+d%+d and bw = %d, (flags=%lX)", (unsigned long)event->w, cre->width, cre->height, cre->x, cre->y, cre->border_width, xwcm );
        XConfigureWindow (dpy, event->w, xwcm, &xwc);
		return;
	}

	if( ASWIN_HFLAGS(asw, AS_IgnoreConfigRequest ) ||
		ASWIN_GET_FLAGS( asw, AS_Fullscreen ) )
	{	
		LOCAL_DEBUG_OUT( "Ignoring ConfigureRequest for client %p as required by hints", asw );
		send_canvas_configure_notify(asw->frame_canvas, asw->client_canvas);
		return;
	}

    if (cre->value_mask & CWStackMode)
        restack_window( asw, (cre->value_mask & CWSibling)?cre->above:None, cre->detail );

    /* check_aswindow_shaped( asw ); */

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
        validate_window_anchor( asw, &new_anchor );
LOCAL_DEBUG_OUT( "validated_anchor(%dx%d%+d%+d)", new_anchor.width, new_anchor.height, new_anchor.x, new_anchor.y );
        asw->anchor = new_anchor ;
        on_window_anchor_changed( asw );
		enforce_avoid_cover( asw );
    }
}

void
HandleSelectionClear( ASEvent *event )
{
	LOCAL_DEBUG_OUT( "SelectionClearEvent : window = %lx, selection = %lx, time = %ld. our( %lx,%lx,%ld )",
					 event->x.xselectionclear.window,
					 event->x.xselectionclear.selection,
					 event->x.xselectionclear.time,
					 Scr.wmprops->selection_window,
					 Scr.wmprops->_XA_WM_S,
					 Scr.wmprops->selection_time );
	if( event->x.xselectionclear.window == Scr.wmprops->selection_window &&
		event->x.xselectionclear.selection == Scr.wmprops->_XA_WM_S  )
	{
		/* must give up window manager's selection if time of the event
		 * after time of us accuring the selection */
		if( event->x.xselectionclear.time  > Scr.wmprops->selection_time )
			Done( False, NULL );
	}
}

/***********************************************************************
 *
 *  Procedure:
 *      HandleShapeNotify - shape notification event handler
 *
 ***********************************************************************/
void
HandleShapeNotify (ASEvent *event)
{
#ifdef SHAPE
    XShapeEvent  *sev = (XShapeEvent *) &(event->x);
    Window w ;
    Bool needs_update = (sev->kind == ShapeBounding) ;
    Bool shaped = sev->shaped ;

    if (!event->client)
		return;
	if( event->client->w != sev->window )
		return ;

    w = event->client->w ;
    while( ASCheckTypedWindowEvent( w, Scr.ShapeEventBase + ShapeNotify, &(event->x) ) )
    {
        if (sev->kind == ShapeBounding)
        {
            needs_update = True ;
            shaped = sev->shaped ;
        }
		ASSync(False);
		sleep_a_millisec(10);
    }

    if( needs_update )
    {
        if( shaped )
            ASWIN_SET_FLAGS( event->client, AS_Shaped );
        else
            ASWIN_CLEAR_FLAGS( event->client, AS_Shaped );
		if( refresh_container_shape( event->client->client_canvas ) )
        	SetShape (event->client, 0);
    }
#endif /* SHAPE */
}

void HandleShmCompletion(ASEvent *event)
{
#ifdef XSHMIMAGE
    XShmCompletionEvent  *sev = (XShmCompletionEvent*) &(event->x);
	LOCAL_DEBUG_OUT( "XSHMIMAGE> EVENT : offset   %ld(%lx), shmseg = %lx", (long)sev->offset, (unsigned long)(sev->offset), sev->shmseg );
	if( !is_background_xfer_ximage( sev->shmseg ) )
		destroy_xshm_segment( sev->shmseg );
#endif /* SHAPE */
}



/***************************************************************************
 *
 * Waits for next X event, or for an auto-raise timeout.
 *
 ****************************************************************************/
void
afterstep_wait_pipes_input(int timeout_sec)
{
	fd_set        in_fdset, out_fdset;
	int           retval;
	struct timeval tv;
	struct timeval *t = NULL;
    int           max_fd = 0;
	LOCAL_DEBUG_OUT( "waiting pipes%s", "");
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
	else if( timeout_sec > 0 )
	{
		t = &tv ; 
		tv.tv_sec = timeout_sec ; 
		tv.tv_usec = 0 ;
	}
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



