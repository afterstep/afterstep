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

#include "../../configure.h"

#ifdef ISC
#include <sys/bsdtypes.h>
#endif

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

/* Some people say that AIX and AIXV3 need 3 preceding underscores, other say
 * no. I'll do both */
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/module.h"
#include "../../include/parse.h"
#include "../../include/decor.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/loadimg.h"
#include "asinternals.h"

#include <X11/keysym.h>

#if 0
    /* All this is so much evel that I just have to get rid of it : */
    int           Context = C_NO_CONTEXT;          /* current button press context */
    int           Button = 0;
    ASWindow     *ButtonWindow;                    /* button press window structure */
    XEvent        Event;                           /* the current event */
    ASWindow     *Tmp_win;                         /* the current afterstep window */
    Window        PressedW;
#endif

/* those are used for AutoReverse mode 1 */
static int warp_in_process = 0;
static int warping_direction = 0;

void
warp_grab (ASWindow * t)
{
	XWindowAttributes attributes;

	/* we're watching all key presses and accept mouse cursor motion events
	   so we will be able to tell when warp mode is finished */
	XGetWindowAttributes (dpy, t->frame, &attributes);
	XSelectInput (dpy, t->frame, attributes.your_event_mask | (PointerMotionMask | KeyPressMask));
	if (t->w != None)
	{
		XGetWindowAttributes (dpy, t->w, &attributes);
		XSelectInput (dpy, t->w, attributes.your_event_mask | (PointerMotionMask | KeyPressMask));
	}
}

void
warp_ungrab (ASWindow * t, Bool finished)
{
    if( warp_in_process )
    {
        if (t != NULL)
        {
            XWindowAttributes attributes;

            /* we no longer need to watch keypresses or pointer motions */
            XGetWindowAttributes (dpy, t->frame, &attributes);
            XSelectInput (dpy, t->frame,
                        attributes.your_event_mask & ~(PointerMotionMask | KeyPressMask));
            if (t->w != None)
            {
                XGetWindowAttributes (dpy, t->w, &attributes);
                XSelectInput (dpy, t->w,
                            attributes.your_event_mask & ~(PointerMotionMask | KeyPressMask));
            }
            if (finished)       /* the window becomes the first one in the warp list now */
            ChangeWarpIndex (t->warp_index, warping_direction);
        }
        if (finished)
            warp_in_process = 0;
    }
}

/***********************************************************************
 *  _______________________EVENT HANDLING ______________________________
 *
 *  HandleEvents  - event loop
 *  DigestEvent   - preprocesses event - finds ASWindow, context etc.
 *  DispatchEvent - calls appropriate handler for the event
 ************************************************************************/
void DigestEvent    ( ASEvent *event );
void DispatchEvent  ( ASEvent *event );

void
HandleEvents ()
{
    ASEvent event;
	while (True)
	{
        if ( AS_XNextEvent (dpy, &(event.x)) )
		{
/*fprintf( stderr, "%s:%d Received event %d\n", __FUNCTION__, __LINE__, event.x.type );*/
            DigestEvent( &event );
            DispatchEvent( &event );
		}
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
	Window root ;
	int junk ;
	unsigned int  mask;

    if( !get_flags( AfterStepState, ASS_PointerOutOfScreen) )
	{
		do
		{
			XAllowEvents (dpy, ReplayPointer, CurrentTime);
            XQueryPointer (dpy, Scr.Root, &root, &root, &junk, &junk, &junk, &junk, &mask);
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


void
DigestEvent( ASEvent *event )
{
    register int i ;
    setup_asevent_from_xevent( event );
    event->client = window2ASWindow( event->w );
    event->context = C_ROOT ;
    event->widget = NULL ;
    if( (event->eclass & ASE_POINTER_EVENTS) != 0 && event->client )
    {
        /* now lets determine the context of the event : (former GetContext)*/
        Window   w = event->w ;
        ASWindow *asw = event->client ;
        XKeyEvent *xk = &(event->x.xkey);
        ASCanvas  *canvas = asw->frame_canvas ;
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
                    if( asw->frame_sides[i]->w == w )
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
                if( (tbar_context = check_astbar_point( asw->tbar, xk->x_root, xk->y_root )) != C_NO_CONTEXT )
                    event->context = tbar_context ;
                else
                {
                    for( i = 0 ; i < FRAME_PARTS ; ++i )
                        if( asw->frame_bars[i] != NULL &&
                            (tbar_context = check_astbar_point( asw->frame_bars[i], xk->x_root, xk->y_root )) != C_NO_CONTEXT )
                        {
                            event->context = tbar_context ;
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
                if( canvas == asw->icon_title_canvas )
                {
                    int c = check_astbar_point( asw->icon_title, xk->x_root, xk->y_root );
                    if( c != C_NO_CONTEXT )
                        event->context = c ;
                }
            }else if( asw->icon_title_canvas && w == asw->icon_title_canvas->w )
            {
                canvas = asw->icon_title_canvas ;
                event->context = C_IconTitle ;
            }
        }
        event->widget  = canvas ;
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
	unsigned int junk_mask;
	Window junk_root;

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
    XQueryPointer (dpy, Scr.Root, &junk_root, &xevent->xany.window,
				   &x_root, &y_root, &x, &y, &junk_mask);

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
    /* handle balloon events specially */
    balloon_handle_event (&(event->x));

	/* handle menu events specially */
    if (HandleMenuEvent (NULL, event) == True)
		return;

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
            warp_ungrab (event->client, True);
            HandleButtonPress (event);
            break;
        case MotionNotify:
            /* if warping, a button press, non-warp keypress, or pointer motion
            * indicates that the warp is done */
            warp_ungrab (event->client, True);
            break;
        case EnterNotify:
            HandleEnterNotify (event);
            break;
        case LeaveNotify:
            HandleLeaveNotify (event);
#if 0
            /* if warping, leaving a window means that we need to ungrab, but
            * the ungrab should be taken care of by the FocusOut */
            warp_ungrab (event->client, False);
#endif
            break;
        case FocusIn:
            HandleFocusIn (event);
            if (event->client != NULL)
            {
                if (warp_in_process)
                    warp_grab (event->client);
                else
                    ChangeWarpIndex (event->client->warp_index, F_WARP_F);
            }
            break;
        case FocusOut:
            /* if warping, this is the normal way to determine that we should ungrab
            * window events */
            warp_ungrab (event->client, False);
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
                on_window_moveresize( event->client, event->w, event->x.xconfigure.x,
                                                               event->x.xconfigure.y,
                                                               event->x.xconfigure.width,
                                                               event->x.xconfigure.height );
                if( event->w == event->client->frame )
                    UpdateVisibility();
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
 *
 *  Procedure:
 *	HandleFocusIn - handles focus in events
 *
 ************************************************************************/
void
HandleFocusIn ( ASEvent *event )
{
    while (ASCheckTypedEvent (FocusIn, &event->x));
    DigestEvent( event );

    if (event->client != Scr.Hilite)
        broadcast_focus_change( event->client );
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
    unsigned int modifier = (xk->state & Scr.nonlock_mods);

	for (key = Scr.FuncKeyRoot; key != NULL; key = key->next)
	{
		/* Here's a real hack - some systems have two keys with the
		 * same keysym and different keycodes. This converts all
		 * the cases to one keycode. */
        xk->keycode = XKeysymToKeycode (dpy, XKeycodeToKeysym (dpy, xk->keycode, 0));
        if ((key->keycode == xk->keycode) &&
			((key->mods == (modifier & (~LockMask))) ||
             (key->mods == AnyModifier)) && (key->cont & event->context))
		{
			extern int    AutoReverse;

			/* check if the warp key was pressed */
            warp_in_process = ((key->fdata->func == F_WARP_B || key->fdata->func == F_WARP_F) &&
							   AutoReverse == 2);
			if (warp_in_process)
                warping_direction = key->fdata->func;

            ExecuteFunction (key->fdata, event, -1);
			return;
		}
	}

	/* if a key has been pressed and it's not one of those that cause
	   warping, we know the warping is finished */
    warp_ungrab (event->client, True);

	/* if we get here, no function key was bound to the key.  Send it
     * to the client if it was in a window we know about: */
    if (event->client)
        if (xk->window != event->client->w && !warp_in_process)
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

void
HandlePropertyNotify (ASEvent *event)
{
#ifdef I18N
	char        **list;
	int           num;
#endif
    ASWindow       *asw;
    XPropertyEvent *xprop = &(event->x.xproperty);

	/* force updates for "transparent" windows */
    if (xprop->atom == _XROOTPMAP_ID && event->w == Scr.Root)
	{
        if (Scr.RootImage)
			destroy_asimage (&(Scr.RootImage));
        for (asw = Scr.ASRoot.next; asw != NULL; asw = asw->next)
            update_window_transparency( asw );
		/* use move_menu() to update transparent menus; this is a kludge, but it works */
		if ((*Scr.MSMenuTitle).texture_type == 129 || (*Scr.MSMenuItem).texture_type == 129 ||
			(*Scr.MSMenuHilite).texture_type == 129)
		{
			MenuRoot     *menu;

			for (menu = Scr.first_menu; menu != NULL; menu = menu->next)
				if ((*menu).is_mapped)
					move_menu (menu, (*menu).x, (*menu).y);
		}
	}

    if( (asw = event->client) == NULL )
        return ;

    if(  xprop->atom == XA_WM_NAME ||
         xprop->atom == XA_WM_ICON_NAME ||
         xprop->atom == _XA_NET_WM_NAME ||
         xprop->atom == _XA_NET_WM_ICON_NAME ||
         xprop->atom == _XA_NET_WM_VISIBLE_NAME ||
         xprop->atom == _XA_NET_WM_VISIBLE_ICON_NAME)
	{
		show_debug( __FILE__, __FUNCTION__, __LINE__, "name prop changed..." );
        if( update_property_hints_manager( asw->w, xprop->atom,
		                                   Scr.supported_hints,
                                           asw->hints, asw->status ) )
		{
            broadcast_window_name( asw );
            broadcast_icon_name( asw );

            show_debug( __FILE__, __FUNCTION__, __LINE__, "New name is \"%s\", icon_name \"%s\"", ASWIN_NAME(asw), ASWIN_ICON_NAME(asw) );

			if (Scr.flags & FollowTitleChanges)
                on_icon_changed(asw);

			/* fix the name in the title bar */
            if (!ASWIN_GET_FLAGS(asw, AS_Iconic))
                on_window_title_changed( asw, True );
        }
	}else
	{
#warning "fix handling of updated window management hints"
#if 0
		switch (Event.xproperty.atom)
		{
	  	 case XA_WM_HINTS:
			 if (Tmp_win->wmhints)
				 XFree ((char *)Tmp_win->wmhints);
			 Tmp_win->wmhints = XGetWMHints (dpy, Event.xany.window);

			 if (Tmp_win->wmhints == NULL)
				 return;

			 if ((Tmp_win->wmhints->flags & IconPixmapHint) ||
				 (Tmp_win->wmhints->flags & IconWindowHint) ||
				 !(Tmp_win->flags & (ICON_OURS | PIXMAP_OURS)))
				 ChangeIcon (Tmp_win);
			 break;

		 case XA_WM_NORMAL_HINTS:
			 GetWindowSizeHints (Tmp_win);
			 BroadcastConfig (M_CONFIGURE_WINDOW, Tmp_win);
			 break;

		 default:
			 if (Event.xproperty.atom == _XA_WM_PROTOCOLS)
				 FetchWmProtocols (Tmp_win);
			 else if (Event.xproperty.atom == _XA_WM_COLORMAP_WINDOWS)
			 {
				 FetchWmColormapWindows (Tmp_win); /* frees old data */
				 ReInstallActiveColormap ();
			 } else if (Event.xproperty.atom == _XA_WM_STATE)
			 {
				 if ((Scr.flags & ClickToFocus) && (Tmp_win == Scr.Focus) && (Tmp_win != NULL))
				 {
					 Scr.Focus = NULL;
					 SetFocus (Tmp_win->w, Tmp_win, False);
				 }
			 }
			 break;
		}
#endif
	}
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
    if ((event->x.xclient.message_type == _XA_WM_CHANGE_STATE) &&
        (event->client) &&
        (event->x.xclient.data.l[0] == IconicState) &&
        !ASWIN_GET_FLAGS(event->client, AS_Iconic))
	{
        FunctionData fdata ;
        init_func_data( &fdata );
        fdata.func = F_ICONIFY ;
        ExecuteFunction (&fdata, event, -1);
#ifdef ENABLE_DND
		/* Pass the event to the client window */
        if (event->x.xclient.window != event->client->w)
		{
            event->x.xclient.window = event->client->w;
            XSendEvent (dpy, event->client->w, True, NoEventMask, &(event->x));
		}
#endif
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
		UpdateVisibility ();
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
        iconify_window( event->client, False );
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
    /*
	 * Need to do the grab to avoid race condition of having server send
	 * MapNotify to client before the frame gets mapped; this is bad because
	 * the client would think that the window has a chance of being viewable
	 * when it really isn't.
	 */
	XGrabServer (dpy);
    unmap_canvas_window(asw->icon_canvas );
    XMapSubwindows (dpy, asw->frame);

#warning "recode the way windows are removed from screen when desktop changes (make it ICCCM compliant)"
    if (asw->status->desktop == Scr.CurrentDesk)
	{
        XMapWindow (dpy, asw->frame);
	}

    broadcast_status_change( ASWIN_GET_FLAGS(asw, AS_Iconic)?M_DEICONIFY:M_MAP, asw );

    if (get_flags( Scr.flags, ClickToFocus) )
        focus_aswindow (asw, False);

#warning "do we need to un-hilite window at the time of mapNotify?"
    XSync (dpy, 0);
	XUngrabServer (dpy);
	XFlush (dpy);
    ASWIN_SET_FLAGS(asw, AS_Mapped);
    ASWIN_CLEAR_FLAGS(asw, AS_IconMapped);
    ASWIN_CLEAR_FLAGS(asw, AS_Iconic);
    UpdateVisibility ();
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

    if ( event->x.xunmap.event == Scr.Root )
		return;

    if (!asw)
		return;

    /* Window remains hilited even when unmapped !!!! */
    /* if (Scr.Hilite == asw )
        Scr.Hilite = NULL; */

    if (Scr.PreviousFocus == asw)
		Scr.PreviousFocus = NULL;

    if (Scr.Focus == asw )
        focus_next_aswindow( asw );

    if (ASWIN_GET_FLAGS(asw, AS_Mapped) || ASWIN_GET_FLAGS(asw, AS_Iconic))
	{
        Bool destroyed = False ;
        XGrabServer (dpy);
        destroyed = ASCheckTypedWindowEvent ( event->w, DestroyNotify, &dummy) ;
        /*
        * The program may have unmapped the client window, from either
        * NormalState or IconicState.  Handle the transition to WithdrawnState.
        */
        Destroy (event->client, destroyed);               /* do not need to mash event before */
        XUngrabServer (dpy);
        UpdateVisibility ();
        XFlush (dpy);
    }
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

        if (get_flags( Scr.flags, ClickToFocus) )
        {
            if ( asw != Scr.Ungrabbed && (xbtn->state & Scr.nonlock_mods) == 0)
                focus_accepted = focus_aswindow(asw, False);
        }

        if (!ASWIN_GET_FLAGS(asw, AS_Visible))
        {
            if (get_flags(Scr.flags, ClickToRaise) && event->context == C_WINDOW
                && (Scr.RaiseButtons & (1 << xbtn->button)) )
                RaiseWindow (asw);
            else
            {
                if (Scr.AutoRaiseDelay > 0)
                {
                    SetTimer (Scr.AutoRaiseDelay);
                } else
                {
#ifdef CLICKY_MODE_1
                    if (event->w != asw->w)
#endif
                    {
                        if (Scr.AutoRaiseDelay == 0)
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

        on_window_pressure_changed( asw, event->context );
    }

    /* we have to execute a function or pop up a menu : */
    modifier = (xbtn->state & Scr.nonlock_mods);
	/* need to search for an appropriate mouse binding */
	MouseEntry = Scr.MouseButtonRoot;
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
		extern Window GnomeProxyWin;
        XUngrabPointer (dpy, CurrentTime);
        XSendEvent (dpy, GnomeProxyWin, False, SubstructureNotifyMask, &(event->x));
	}
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

	/* look for a matching leaveNotify which would nullify this enterNotify */
    if (ASCheckTypedWindowEvent ( ewp->window, LeaveNotify, &d))
	{
		balloon_handle_event (&d);
		if ((d.xcrossing.mode == NotifyNormal) && (d.xcrossing.detail != NotifyInferior))
			return;
	}
/* an EnterEvent in one of the PanFrameWindows activates the Paging */
#ifndef NO_VIRTUAL
    if (ewp->window == Scr.PanFrameTop.win   ||
        ewp->window == Scr.PanFrameLeft.win  ||
        ewp->window == Scr.PanFrameRight.win ||
        ewp->window == Scr.PanFrameBottom.win  )
	{
		int           delta_x = 0, delta_y = 0;

		/* this was in the HandleMotionNotify before, HEDU */
		HandlePaging (NULL, Scr.EdgeScrollX, Scr.EdgeScrollY,
                      &(ewp->x_root), &(ewp->y_root), &delta_x, &delta_y, True);
		return;
	}
#endif /* NO_VIRTUAL */

    if (ewp->window == Scr.Root)
	{
        if (!get_flags(Scr.flags, ClickToFocus) && !get_flags(Scr.flags, SloppyFocus))
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
        if (!get_flags(Scr.flags, ClickToFocus))
		{
            if (Scr.Focus != asw)
			{
                if (Scr.AutoRaiseDelay > 0 && !ASWIN_GET_FLAGS(asw, AS_Visible))
					SetTimer (Scr.AutoRaiseDelay);
                focus_aswindow(asw, False);
            }else
                focus_aswindow(asw, True);         /* don't affect the circ.seq. */
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
				if (Scr.Focus != NULL)
                    hide_focus();
                if (Scr.Hilite != NULL)
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
	{
        ASWindow     *otherwin = window2ASWindow( cre->above);

		xwc.sibling = (((cre->value_mask & CWSibling) &&
                        ( otherwin != NULL))?otherwin->frame : cre->above);
		xwc.stack_mode = cre->detail;
        XConfigureWindow (dpy, asw->frame, cre->value_mask & (CWSibling | CWStackMode), &xwc);
		XSync (dpy, False);
		CorrectStackOrder ();
	}

#ifdef SHAPE
	{
		int           xws, yws, xbs, ybs;
		unsigned      wws, hws, wbs, hbs;
		int           boundingShaped, clipShaped;

        XShapeQueryExtents (dpy, asw->w, &boundingShaped, &xws, &yws, &wws,
							&hws, &clipShaped, &xbs, &ybs, &wbs, &hbs);
        asw->wShaped = boundingShaped;
	}
#endif /* SHAPE */

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
    event->client->wShaped = sev->shaped;
    SetShape (event->client, 0/*Tmp_win->frame_width*/);
}
#endif /* SHAPE */

#if 1										   /* see SetTimer() */
/**************************************************************************
 * For auto-raising windows, this routine is called
 *************************************************************************/
volatile int  alarmed;
void
enterAlarm (int nonsense)
{
	alarmed = True;
	signal (SIGALRM, enterAlarm);
}
#endif /* 1 */

/***************************************************************************
 *
 * Waits for next X event, or for an auto-raise timeout.
 *
 ****************************************************************************/
int
AS_XNextEvent (Display * dpy, XEvent * event)
{
	extern int    fd_width, x_fd;
	fd_set        in_fdset, out_fdset;
	int           i;
	int           retval;
	struct timeval tv;
	struct timeval *t = NULL;
	extern module_t *Module;
	int           max_fd = 0;

	XFlush (dpy);
	if (XPending (dpy))
	{
        ASNextEvent (event);
		return 1;
	}

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
			XQueryPointer (dpy, Scr.Root, &JunkRoot, &child, &JunkX, &JunkY, &JunkX,
						   &JunkY, &JunkMask);
			if ((Scr.Focus != NULL) && (child == Scr.Focus->frame))
			{
				if (!(Scr.Focus->flags & VISIBLE))
				{
					RaiseWindow (Scr.Focus);
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

	/* Do this IMMEDIATELY prior to select, to prevent any nasty
	 * queued up X events from just hanging around waiting to be
	 * flushed */
	if (XPending (dpy))
	{
        ASNextEvent (event);
		return 1;
	}
	/* Zap all those zombies! */
	/* If we get to here, then there are no X events waiting to be processed.
	 * Just take a moment to check for dead children. */
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
