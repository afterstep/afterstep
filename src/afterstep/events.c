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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/module.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/loadimg.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

#include "menus.h"

extern int    menuFromFrameOrWindowOrTitlebar;

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
int           warp_in_process = 0;
int           warping_direction = 0;

#ifdef SHAPE
extern int    ShapeEventBase;
void          HandleShapeNotify (void);
#endif /* SHAPE */


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
	}
	if (finished)
	{
		/* the window becomes the first one in the warp list now */
		ChangeWarpIndex (t->warp_index, warping_direction);
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

void
DigestEvent( ASEvent *event )
{
    setup_asevent_from_xevent( event );
    event->client = window2ASWindow( event->w );
    event->context = C_ROOT ;
    event->widget = NULL ;
    if( (event->event_class & ASE_POINTER_EVENTS) != 0 && event->client )
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
                register int i = FRAME_SIDES ;
                while( --i >= 0 )
                    if( asw->frame_sides[i]->w == w )
                    {
                        int tbar_context ;
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
                    XTransalteCoordinates(dpy,Scr.Root,w,xk->x_root, xk->y_root, &(xk->x), &(xk->y), &dumm );
                }
            }
            if( event->context == C_FRAME )
            {
                if( (tbar_context = check_astbar_point( asw->tbar, xk->x_root, xk->y_root )) != C_NO_CONTEXT )
                    event->context = tbar_context ;
                else
                {
                    for( i = 0 ; i < FRAME_PARTS ; ++i )
                        if( asw->frame_bars[i] != NULL &&
                            (tbar_context = check_astbar_point( asw->frame_bars[i], xk->x_root, xk->y_root )) != C_NO_CONTEXT )
                        {
                            if( tbar_context == C_TITLE )
                                event->context = C_FrameN<<i ;
                            else
                                event->context = tbar_context ;
                            break;
                        }
                }
            }
            event->w = w ;
        }else
        {
            if( asw->icon_canvas && w == asw->icon_canvas->w )
                canvas = asw->icon_canvas ;
            else( asw->icon_title_canvas && w == asw->icon_title_canvas->w )
                canvas = asw->icon_title_canvas ;

            event->context = C_ICON ;
        }
        event->widget  = canvas ;
    }
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
	 case Expose:
         HandleExpose (event);
		 break;
	 case DestroyNotify:
         HandleDestroyNotify (event);
		 break;
	 case MapRequest:
         HandleMapRequest (event);
		 break;
	 case MapNotify:
         HandleMapNotify (event);
		 break;
	 case UnmapNotify:
         HandleUnmapNotify (event);
		 break;
	 case ButtonPress:
		 /* if warping, a button press, non-warp keypress, or pointer motion
		  * indicates that the warp is done */
         if ((event->client != NULL) && (warp_in_process))
             warp_ungrab (event->client, True);
         HandleButtonPress (event);
		 break;
	 case EnterNotify:
         HandleEnterNotify (event);
		 break;
	 case LeaveNotify:
         HandleLeaveNotify (event);
#if 0
		 /* if warping, leaving a window means that we need to ungrab, but
		  * the ungrab should be taken care of by the FocusOut */
         if ((warp_in_process) && (event->client != NULL))
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
         if (event->client != NULL && warp_in_process)
             warp_ungrab (event->client, False);
		 break;
	 case MotionNotify:
		 /* if warping, a button press, non-warp keypress, or pointer motion
		  * indicates that the warp is done */
         if ((warp_in_process) && (event->client != NULL))
             warp_ungrab (event->client, True);
		 break;
	 case ConfigureRequest:
         HandleConfigureRequest (event);
		 break;
	 case ClientMessage:
         HandleClientMessage (event);
		 break;
	 case PropertyNotify:
         HandlePropertyNotify (event);
		 break;
	 case KeyPress:
		 /* if a key has been pressed and it's not one of those that cause
		    warping, we know the warping is finished */
         HandleKeyPress (event);
		 break;
	 case ColormapNotify:
         HandleColormapNotify (event);
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
	XEvent        d;
	Window        w;

    while (ASCheckTypedEvent (FocusIn, &event.x));
    DigestEvent( &event );

    if (!event->client)
	{
        SetBorder (Scr.Hilite, False,  True, True, None);
		Broadcast (M_FOCUS_CHANGE, 3, 0L, 0L, 0L);
    } else if (event->client != Scr.Hilite)
	{
		SetBorder (Tmp_win, True, True, True, None);
		Broadcast (M_FOCUS_CHANGE, 3, Tmp_win->w, Tmp_win->frame, (unsigned long)Tmp_win);
	}
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleKeyPress - key press event handler
 *
 ************************************************************************/
void
HandleKeyPress ()
{
	FuncKey      *key;
	unsigned int  modifier;
	Window        dummy;

	Context = GetContext (Tmp_win, &Event, &dummy);

	modifier = (Event.xkey.state & Scr.nonlock_mods);
	for (key = Scr.FuncKeyRoot; key != NULL; key = key->next)
	{
		ButtonWindow = Tmp_win;
		/* Here's a real hack - some systems have two keys with the
		 * same keysym and different keycodes. This converts all
		 * the cases to one keycode. */
		Event.xkey.keycode = XKeysymToKeycode (dpy, XKeycodeToKeysym (dpy, Event.xkey.keycode, 0));
		if ((key->keycode == Event.xkey.keycode) &&
			((key->mods == (modifier & (~LockMask))) ||
			 (key->mods == AnyModifier)) && (key->cont & Context))
		{
			extern int    AutoReverse;

			/* check if the warp key was pressed */
			warp_in_process = ((key->func == F_WARP_B || key->func == F_WARP_F) &&
							   AutoReverse == 2);
			if (warp_in_process)
				warping_direction = key->func;

			ExecuteFunction (key->func, key->action, Event.xany.window, Tmp_win,
							 &Event, Context, key->val1, key->val2,
							 key->val1_unit, key->val2_unit, key->menu, -1);
			return;
		}
	}

	/* if a key has been pressed and it's not one of those that cause
	   warping, we know the warping is finished */
	if (warp_in_process)
	{
		if (Tmp_win != NULL)
			warp_ungrab (Tmp_win, True);
		warp_in_process = 0;
		Tmp_win = NULL;
	}

	/* if we get here, no function key was bound to the key.  Send it
	 * to the client if it was in a window we know about.
	 */

	if (Tmp_win)
	{
		if (Event.xkey.window != Tmp_win->w && !warp_in_process)
		{
			Event.xkey.window = Tmp_win->w;
			XSendEvent (dpy, Tmp_win->w, False, KeyPressMask, &Event);
		}
	}
	ButtonWindow = NULL;
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
HandlePropertyNotify ()
{
#ifdef I18N
	char        **list;
	int           num;
#endif

	/* force updates for "transparent" windows */
	if (Event.xproperty.atom == _XROOTPMAP_ID && Event.xproperty.window == Scr.Root)
	{
		ASWindow     *win;

		if (Scr.RootImage)
			destroy_asimage (&(Scr.RootImage));
		for (win = Scr.ASRoot.next; win != NULL; win = win->next)
			if (SetTransparency (win))
				SetBorder (win, Scr.Hilite == win, True, True, None);
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

	if ((!Tmp_win) || (XGetGeometry (dpy, Tmp_win->w, &JunkRoot, &JunkX, &JunkY,
									 &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0))
		return;

	if( Event.xproperty.atom == XA_WM_NAME ||
		Event.xproperty.atom == XA_WM_ICON_NAME ||
		Event.xproperty.atom == _XA_NET_WM_NAME ||
		Event.xproperty.atom == _XA_NET_WM_ICON_NAME ||
		Event.xproperty.atom == _XA_NET_WM_VISIBLE_NAME ||
		Event.xproperty.atom == _XA_NET_WM_VISIBLE_ICON_NAME )
	{
		show_debug( __FILE__, __FUNCTION__, __LINE__, "name prop changed..." );
		if( update_property_hints_manager( Tmp_win->w, Event.xproperty.atom,
		                                   Scr.supported_hints,
                                           Tmp_win->hints, Tmp_win->status ) )
		{
			BroadcastName( M_WINDOW_NAME, Tmp_win->w, Tmp_win->frame,
						   (unsigned long)Tmp_win, ASWIN_NAME(Tmp_win));
			BroadcastName( M_ICON_NAME, Tmp_win->w, Tmp_win->frame,
						   (unsigned long)Tmp_win, ASWIN_ICON_NAME(Tmp_win));

			show_debug( __FILE__, __FUNCTION__, __LINE__, "New name is \"%s\", icon_name \"%s\"", ASWIN_NAME(Tmp_win), ASWIN_ICON_NAME(Tmp_win) );

			if (Scr.flags & FollowTitleChanges)
				ChangeIcon (Tmp_win);

			/* fix the name in the title bar */
			if (!(Tmp_win->flags & ICONIFIED))
				SetTitleBar (Tmp_win, (Scr.Hilite == Tmp_win), True);
			else
			{
				DrawIconWindow (Tmp_win);
                if (Scr.look_flags & SeparateButtonTitle)
					RedoIconName (Tmp_win);
			}
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
HandleClientMessage ()
{
	XEvent        button;

	if ((Event.xclient.message_type == _XA_WM_CHANGE_STATE) &&
		(Tmp_win) && (Event.xclient.data.l[0] == IconicState) && !(Tmp_win->flags & ICONIFIED))
	{
		XQueryPointer (dpy, Scr.Root, &JunkRoot, &JunkChild,
					   &(button.xmotion.x_root),
					   &(button.xmotion.y_root), &JunkX, &JunkY, &JunkMask);
		button.type = 0;
		ExecuteFunction (F_ICONIFY, NULLSTR, Event.xany.window,
						 Tmp_win, &button, C_FRAME, 0, 0, 0, 0, (MenuRoot *) 0, -1);
#ifdef ENABLE_DND
		/* Pass the event to the client window */
		if (Event.xclient.window != Tmp_win->w)
		{
			Event.xclient.window = Tmp_win->w;
			XSendEvent (dpy, Tmp_win->w, True, NoEventMask, &Event);
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
HandleExpose ()
{
	if (Event.xexpose.count != 0)
		return;

	if (Tmp_win)
	{
		Window        w = Event.xany.window;
		int           i, title_only = 1;

		flush_expose (Tmp_win->frame);
		flush_expose (Tmp_win->Parent);

		/* discard exposures of the frame or Parent windows */
		if (Event.xexpose.window == Tmp_win->Parent)
			return;
		if (Event.xexpose.window == Tmp_win->frame)
		{
			/*  if (Tmp_win->flags & FRAME && (!(Tmp_win->flags & SHADED)))
			   frame_draw_frame (Tmp_win); */
			return;
		}
		flush_expose (Tmp_win->title_w);
		for (i = 0; i < Scr.nr_left_buttons; i++)
			if (Tmp_win->left_w[i] != None)
				flush_expose (Tmp_win->left_w[i]);
		for (i = 0; i < Scr.nr_right_buttons; i++)
			if (Tmp_win->right_w[i] != None)
				flush_expose (Tmp_win->right_w[i]);

		if (flush_expose (Tmp_win->side) || w == Tmp_win->side)
			title_only = 0;
		if (flush_expose (Tmp_win->corners[0]) || w == Tmp_win->corners[0])
			title_only = 0;
		if (flush_expose (Tmp_win->corners[1]) || w == Tmp_win->corners[1])
			title_only = 0;
		if (flush_expose (Tmp_win->icon_pixmap_w) || w == Tmp_win->icon_pixmap_w)
			title_only = 0;
		if (flush_expose (Tmp_win->icon_title_w) || w == Tmp_win->icon_title_w)
			title_only = 0;

		if (title_only)
			SetTitleBar (Tmp_win, (Scr.Hilite == Tmp_win), False);
		else
			SetBorder (Tmp_win, (Scr.Hilite == Tmp_win), True, True, Event.xany.window);
	}
}



/***********************************************************************
 *
 *  Procedure:
 *	HandleDestroyNotify - DestroyNotify event handler
 *
 ***********************************************************************/
void
HandleDestroyNotify ()
{
	if (Tmp_win)
	{
		Destroy (Tmp_win, True);
		UpdateVisibility ();
	}
}




/***********************************************************************
 *
 *  Procedure:
 *	HandleMapRequest - MapRequest event handler
 *
 ************************************************************************/
void
HandleMapRequest ()
{
	Event.xany.window = Event.xmaprequest.window;

    Tmp_win = window2ASWindow( Event.xany.window );
	XFlush (dpy);

	/* If the window has never been mapped before ... */
	if (!Tmp_win)
	{
		/* Add decorations. */
		Tmp_win = AddWindow (Event.xany.window);
		if (Tmp_win == NULL)
			return;
	}
	/* If it's not merely iconified, and we have hints, use them. */
	if (!(Tmp_win->flags & ICONIFIED))
	{
		int           state = NormalState;

		if( get_flags( Tmp_win->status->flags, AS_Iconic ) )
			state = IconicState;
		else if( get_flags( Tmp_win->status->flags, AS_Shaded ) )
			state = ZoomState;


		XGrabServer (dpy);
		switch (state)
		{
		 case DontCareState:
		 case NormalState:
		 case InactiveState:
		 default:
			 if (Tmp_win->status->desktop == Scr.CurrentDesk)
			 {
				 XMapWindow (dpy, Tmp_win->w);
				 XMapWindow (dpy, Tmp_win->frame);
				 Tmp_win->flags |= MAP_PENDING;
				 SetMapStateProp (Tmp_win, NormalState);
				 if (Scr.flags & ClickToFocus)
				 {
					 SetFocus (Tmp_win->w, Tmp_win, False);
				 }
			 } else
			 {
				 XMapWindow (dpy, Tmp_win->w);
				 SetMapStateProp (Tmp_win, NormalState);
			 }
			 break;

		 case IconicState:
			 Iconify (Tmp_win);
			 break;

		 case ZoomState:
			 Shade (Tmp_win);
			 break;
		}
		XSync (dpy, 0);
		XUngrabServer (dpy);
	}
	/* If no hints, or currently an icon, just "deiconify" */
	else
	{
		DeIconify (Tmp_win);
	}
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleMapNotify - MapNotify event handler
 *
 ***********************************************************************/
void
HandleMapNotify ()
{
	if (!Tmp_win || (Event.xmap.event == Scr.ASRoot.w))
	{
		return;
	}
	/*
	 * Need to do the grab to avoid race condition of having server send
	 * MapNotify to client before the frame gets mapped; this is bad because
	 * the client would think that the window has a chance of being viewable
	 * when it really isn't.
	 */
	XGrabServer (dpy);
	if (Tmp_win->icon_pixmap_w != None)
		XUnmapWindow (dpy, Tmp_win->icon_pixmap_w);
	XMapSubwindows (dpy, Tmp_win->frame);

	if (Tmp_win->status->desktop == Scr.CurrentDesk)
	{
		XMapWindow (dpy, Tmp_win->frame);
	}
	if (Tmp_win->flags & ICONIFIED)
		Broadcast (M_DEICONIFY, 7, Tmp_win->w, Tmp_win->frame, (unsigned long)Tmp_win,
				   Tmp_win->icon_p_x, Tmp_win->icon_p_y, Tmp_win->icon_p_width,
				   Tmp_win->icon_p_height);
	else
		Broadcast (M_MAP, 3, Tmp_win->w, Tmp_win->frame, (unsigned long)Tmp_win);

	if (Scr.flags & ClickToFocus)
		SetFocus (Tmp_win->w, Tmp_win, False);

	if (!ASWIN_HFLAGS(Tmp_win, (AS_Titlebar|AS_Handles)) && (Tmp_win->boundary_width < 2))
		SetBorder (Tmp_win, False, True, True, Tmp_win->frame);

	XSync (dpy, 0);
	XUngrabServer (dpy);
	XFlush (dpy);
	Tmp_win->flags |= MAPPED;
	Tmp_win->flags &= ~MAP_PENDING;
	Tmp_win->flags &= ~ICONIFIED;
	Tmp_win->flags &= ~ICON_UNMAPPED;
	UpdateVisibility ();
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleUnmapNotify - UnmapNotify event handler
 *
 ************************************************************************/
void
HandleUnmapNotify ()
{
	int           dstx, dsty;
	Window        dumwin;
	XEvent        dummy;
	extern ASWindow *colormap_win;

	/*
	 * The July 27, 1988 ICCCM spec states that a client wishing to switch
	 * to WithdrawnState should send a synthetic UnmapNotify with the
	 * event field set to (pseudo-)root, in case the window is already
	 * unmapped (which is the case for afterstep for IconicState).  Unfortunately,
	 * we looked for the ASContext using that field, so try the window
	 * field also.
	 */
	if (!Tmp_win)
	{
		Event.xany.window = Event.xunmap.window;
		Tmp_win = window2ASWindow( Event.xany.window );
	}
	if (Event.xunmap.event == Scr.ASRoot.w)
		return;

	if (!Tmp_win)
		return;

	if (Tmp_win == Scr.Hilite)
		Scr.Hilite = NULL;

	if (Scr.PreviousFocus == Tmp_win)
		Scr.PreviousFocus = NULL;

	if ((Tmp_win == Scr.Focus) && (Scr.flags & ClickToFocus))
	{
		ASWindow     *t, *tn = NULL;
		long          best = LONG_MIN;

		for (t = Scr.ASRoot.next; t != NULL; t = t->next)
		{
			if ((t->focus_sequence > best) && (t != Tmp_win))
			{
				best = t->focus_sequence;
				tn = t;
			}
		}
		if (tn)
			SetFocus (tn->w, tn, False);
		else
			SetFocus (Scr.NoFocusWin, NULL, False);
	}
	if (Scr.Focus == Tmp_win)
		SetFocus (Scr.NoFocusWin, NULL, False);

	if (Tmp_win == Scr.pushed_window)
		Scr.pushed_window = NULL;

	if (Tmp_win == colormap_win)
		colormap_win = NULL;

	if ((!(Tmp_win->flags & MAPPED) && !(Tmp_win->flags & ICONIFIED)))
	{
		return;
	}
	XGrabServer (dpy);

    if (ASCheckTypedWindowEvent ( Event.xunmap.window, DestroyNotify, &dummy))
	{
		Destroy (Tmp_win, True);
		XUngrabServer (dpy);
		UpdateVisibility ();
		return;
	}
	/*
	 * The program may have unmapped the client window, from either
	 * NormalState or IconicState.  Handle the transition to WithdrawnState.
	 *
	 * We need to reparent the window back to the root (so that afterstep exiting
	 * won't cause it to get mapped) and then throw away all state (pretend
	 * that we've received a DestroyNotify).
	 */
	if (XTranslateCoordinates (dpy, Event.xunmap.window, Scr.Root, 0, 0, &dstx, &dsty, &dumwin))
	{
		XEvent        ev;
		Bool          reparented;

        reparented = ASCheckTypedWindowEvent ( Event.xunmap.window, ReparentNotify, &ev);
		SetMapStateProp (Tmp_win, WithdrawnState);
		if (reparented)
		{
			if (Tmp_win->old_bw)
				XSetWindowBorderWidth (dpy, Event.xunmap.window, Tmp_win->old_bw);
			if (get_flags(Tmp_win->hints->flags, AS_Icon) &&
				((Tmp_win->hints->flags)&(AS_ClientIcon|AS_ClientIconPixmap)) == AS_ClientIcon )
				XUnmapWindow (dpy, Tmp_win->hints->icon.window);
		} else
		{
			RestoreWithdrawnLocation (Tmp_win, False);
		}
		XRemoveFromSaveSet (dpy, Event.xunmap.window);
		XSelectInput (dpy, Event.xunmap.window, NoEventMask);
		Destroy (Tmp_win, True);			   /* do not need to mash event before */
		/*
		 * Flush any pending events for the window.
		 */
		/* Bzzt! it could be about to re-map */
/*      while(XCheckWindowEvent(dpy, Event.xunmap.window,
   StructureNotifyMask | PropertyChangeMask |
   ColormapChangeMask |
   EnterWindowMask | LeaveWindowMask, &dummy));
 */
	}										   /* else window no longer exists and we'll get a destroy notify */
	XUngrabServer (dpy);
	UpdateVisibility ();
	XFlush (dpy);
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleButtonPress - ButtonPress event handler
 *
 ***********************************************************************/
void
HandleButtonPress ()
{
	unsigned int  modifier;
	MouseButton  *MouseEntry;
	Window        xPressedW;
	int           LocalContext;
	Bool          AShandled = False;

	Context = GetContext (Tmp_win, &Event, &PressedW);
	LocalContext = Context;
	xPressedW = PressedW;

	/* click to focus stuff goes here */
	if (Scr.flags & ClickToFocus)
	{
		if (Tmp_win && (Tmp_win != Scr.Ungrabbed)
			&& ((Event.xbutton.state & Scr.nonlock_mods) == 0))
		{
			Bool          focusAccepted = SetFocus (Tmp_win->w, Tmp_win, False);

			if (!(Tmp_win->flags & VISIBLE))
			{
				if ((Scr.flags & ClickToRaise) && (Context == C_WINDOW)
					&& (Scr.RaiseButtons & (1 << Event.xbutton.button)))
					RaiseWindow (Tmp_win);
				else
				{
					if (Scr.AutoRaiseDelay > 0)
					{
						SetTimer (Scr.AutoRaiseDelay);
					} else
					{
#ifdef CLICKY_MODE_1
						if ((Event.xany.window != Tmp_win->w) &&
							(Event.xbutton.subwindow != Tmp_win->w) &&
							(Event.xany.window != Tmp_win->Parent) &&
							(Event.xbutton.subwindow != Tmp_win->Parent))
#endif
						{
							if (Scr.AutoRaiseDelay == 0)
								RaiseWindow (Tmp_win);
						}
					}
				}
			}
			if (!(Tmp_win->flags & ICONIFIED) && focusAccepted)
			{
				XSync (dpy, 0);
				if ((Scr.flags & EatFocusClick) && (Context == C_WINDOW))
					XAllowEvents (dpy, AsyncPointer, CurrentTime);
				else
					XAllowEvents (dpy, ReplayPointer, CurrentTime);

				XSync (dpy, 0);
				return;
			}
		}
	} else if ((Scr.flags & ClickToRaise) && Tmp_win && (Context == C_WINDOW)
			   && (Scr.RaiseButtons & (1 << Event.xbutton.button))
			   && ((Event.xbutton.state & Scr.nonlock_mods) == 0)
			   && ((Tmp_win->flags & VISIBLE) == 0))
	{
		RaiseWindow (Tmp_win);
		if (!(Tmp_win->flags & ICONIFIED))
		{
			XSync (dpy, 0);
			XAllowEvents (dpy, AsyncPointer, CurrentTime);
			XSync (dpy, 0);
			return;
		}
	}
	XSync (dpy, 0);
	XAllowEvents (dpy, (Context == C_WINDOW) ? ReplayPointer : AsyncPointer, CurrentTime);
	XSync (dpy, 0);

	if (Context == C_TITLE)
		SetTitleBar (Tmp_win, (Scr.Hilite == Tmp_win), False);
	else
		SetBorder (Tmp_win, (Scr.Hilite == Tmp_win), True, True, PressedW);

	ButtonWindow = Tmp_win;

	/* we have to execute a function or pop up a menu
	 */

	modifier = (Event.xbutton.state & Scr.nonlock_mods);
	/* need to search for an appropriate mouse binding */
	MouseEntry = Scr.MouseButtonRoot;
	while (MouseEntry != (MouseButton *) 0)
	{
		if (((MouseEntry->Button == Event.xbutton.button) || (MouseEntry->Button == 0)) &&
			(MouseEntry->Context & Context) &&
			((MouseEntry->Modifier == AnyModifier) || (MouseEntry->Modifier == modifier)))
		{
			/* got a match, now process it */
			if (MouseEntry->func != (int)NULL)
			{
				ExecuteFunction (MouseEntry->func, MouseEntry->action,
								 Event.xany.window,
								 Tmp_win, &Event, Context, MouseEntry->val1,
								 MouseEntry->val2,
								 MouseEntry->val1_unit, MouseEntry->val2_unit,
								 MouseEntry->menu, -1);
				AShandled = True;
				break;
			}
		}
		MouseEntry = MouseEntry->NextButton;
	}

	/* GNOME this click hasn't been taken by AfterStep */
	if (!AShandled && Event.xany.window == Scr.Root)
	{
		extern Window GnomeProxyWin;

		if (Event.type == ButtonPress)
			XUngrabPointer (dpy, CurrentTime);
		XSendEvent (dpy, GnomeProxyWin, False, SubstructureNotifyMask, &Event);
	}

	PressedW = None;

	if (LocalContext != C_TITLE)
		SetBorder (ButtonWindow, (Scr.Hilite == ButtonWindow), True, True, xPressedW);
	else
		SetTitleBar (ButtonWindow, (Scr.Hilite == ButtonWindow), False);

	ButtonWindow = NULL;
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleEnterNotify - EnterNotify event handler
 *
 ************************************************************************/
void
HandleEnterNotify ()
{
	XEnterWindowEvent *ewp = &Event.xcrossing;
	XEvent        d;

	/* look for a matching leaveNotify which would nullify this enterNotify */
    if (ASCheckTypedWindowEvent ( ewp->window, LeaveNotify, &d))
	{
		balloon_handle_event (&d);
		if ((d.xcrossing.mode == NotifyNormal) && (d.xcrossing.detail != NotifyInferior))
			return;
	}
/* an EnterEvent in one of the PanFrameWindows activates the Paging */
#ifndef NO_VIRTUAL
	if (ewp->window == Scr.PanFrameTop.win
		|| ewp->window == Scr.PanFrameLeft.win
		|| ewp->window == Scr.PanFrameRight.win || ewp->window == Scr.PanFrameBottom.win)
	{
		int           delta_x = 0, delta_y = 0;

		/* this was in the HandleMotionNotify before, HEDU */
		HandlePaging (NULL, Scr.EdgeScrollX, Scr.EdgeScrollY,
					  &Event.xcrossing.x_root, &Event.xcrossing.y_root, &delta_x, &delta_y, True);
		return;
	}
#endif /* NO_VIRTUAL */

	if (Event.xany.window == Scr.Root)
	{
		if ((!(Scr.flags & ClickToFocus)) && (!(Scr.flags & SloppyFocus)))
		{
			SetFocus (Scr.NoFocusWin, NULL, False);
		}
		InstallWindowColormaps (NULL);
		return;
	}
	/* make sure its for one of our windows */
	if (!Tmp_win)
		return;

	if (ASWIN_HFLAGS(Tmp_win,AS_AcceptsFocus))
	{
		if (!(Scr.flags & ClickToFocus))
		{
			if (Scr.Focus != Tmp_win)
			{
				if ((Scr.AutoRaiseDelay > 0) && (!(Tmp_win->flags & VISIBLE)))
					SetTimer (Scr.AutoRaiseDelay);
				SetFocus (Tmp_win->w, Tmp_win, False);
			} else
				SetFocus (Tmp_win->w, Tmp_win, True);	/* don't affect the circ.seq. */
		}
		if ((!(Tmp_win->flags & ICONIFIED)) && (Event.xany.window == Tmp_win->w))
			InstallWindowColormaps (Tmp_win);
		else
			InstallWindowColormaps (NULL);
	}
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleLeaveNotify - LeaveNotify event handler
 *
 ************************************************************************/
void
HandleLeaveNotify ()
{
	/* If we leave the root window, then we're really moving
	 * another screen on a multiple screen display, and we
	 * need to de-focus and unhighlight to make sure that we
	 * don't end up with more than one highlighted window at a time */
	if (Event.xcrossing.window == Scr.Root)
	{
		if (Event.xcrossing.mode == NotifyNormal)
		{
			if (Event.xcrossing.detail != NotifyInferior)
			{
				if (Scr.Focus != NULL)
				{
					SetFocus (Scr.NoFocusWin, NULL, False);
				}
				if (Scr.Hilite != NULL)
					SetBorder (Scr.Hilite, False, True, True, None);
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
HandleConfigureRequest ()
{
	XWindowChanges xwc;
	unsigned long xwcm;
	int           x, y, width, height;
	XConfigureRequestEvent *cre = &Event.xconfigurerequest;

	/*
	 * Event.xany.window is Event.xconfigurerequest.parent, so Tmp_win will
	 * be wrong
	 */
	Event.xany.window = cre->window;		   /* mash parent field */
	Tmp_win = window2ASWindow( cre->window );

	/*
	 * According to the July 27, 1988 ICCCM draft, we should ignore size and
	 * position fields in the WM_NORMAL_HINTS property when we map a window.
	 * Instead, we'll read the current geometry.  Therefore, we should respond
	 * to configuration requests for windows which have never been mapped.
	 */

	if (Tmp_win == NULL)
	{
		xwcm = cre->value_mask & (CWX | CWY | CWWidth | CWHeight | CWBorderWidth);
		xwc.x = cre->x;
		xwc.y = cre->y;

		xwc.width = cre->width;
		xwc.height = cre->height;
		xwc.border_width = cre->border_width;
		XConfigureWindow (dpy, Event.xany.window, xwcm, &xwc);
		return;
	}
	if (cre->value_mask & CWStackMode)
	{
        ASWindow     *otherwin = window2ASWindow( cre->above);

		xwc.sibling = (((cre->value_mask & CWSibling) &&
                        ( otherwin != NULL))?otherwin->frame : cre->above);
		xwc.stack_mode = cre->detail;
		XConfigureWindow (dpy, Tmp_win->frame, cre->value_mask & (CWSibling | CWStackMode), &xwc);
		XSync (dpy, False);
		CorrectStackOrder ();
	}
#ifdef SHAPE
	{
		int           xws, yws, xbs, ybs;
		unsigned      wws, hws, wbs, hbs;
		int           boundingShaped, clipShaped;

		XShapeQueryExtents (dpy, Tmp_win->w, &boundingShaped, &xws, &yws, &wws,
							&hws, &clipShaped, &xbs, &ybs, &wbs, &hbs);
		Tmp_win->wShaped = boundingShaped;
	}
#endif /* SHAPE */

	/* for restoring */
	if (cre->value_mask & CWBorderWidth)
	{
		Tmp_win->old_bw = cre->border_width;
	}

	x = Tmp_win->frame_x;
	y = Tmp_win->frame_y;
	width = Tmp_win->frame_width;
	height = Tmp_win->frame_height;
	get_resize_geometry (Tmp_win, cre->x, cre->y, cre->width, cre->height,
						 (cre->value_mask & CWX) ? &x : NULL,
						 (cre->value_mask & CWY) ? &y : NULL,
						 (cre->value_mask & CWWidth) ? &width : NULL,
						 (cre->value_mask & CWHeight) ? &height : NULL);

	/*
	 * SetupWindow (x,y) are the location of the upper-left outer corner and
	 * are passed directly to XMoveResizeWindow (frame).  The (width,height)
	 * are the inner size of the frame.  The inner width is the same as the
	 * requested client window width; the inner height is the same as the
	 * requested client window height plus any title bar slop.
	 */
    on_window_status_changed( Tmp_win, True );
	UpdateVisibility ();

}

/***********************************************************************
 *
 *  Procedure:
 *      HandleShapeNotify - shape notification event handler
 *
 ***********************************************************************/
#ifdef SHAPE
void
HandleShapeNotify (void)
{
	XShapeEvent  *sev = (XShapeEvent *) & Event;

	if (!Tmp_win)
		return;
	if (sev->kind != ShapeBounding)
		return;
	Tmp_win->wShaped = sev->shaped;
    SetShape (Tmp_win, 0/*Tmp_win->frame_width*/);
}
#endif /* SHAPE */

#if 1										   /* see SetTimer() */
/**************************************************************************
 *
 * For auto-raising windows, this routine is called
 *
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
