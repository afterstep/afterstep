/*
 * Copyright (c) 2002 Sasha Vasko <sasha@aftercode.net>
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

/**********************************************************************
 *
 * Add a new window, put the titlbar and other stuff around
 * the window
 *
 **********************************************************************/
#define LOCAL_DEBUG

#include "../../configure.h"

#include "asinternals.h"
#include "../../libAfterStep/moveresize.h"

/*************************************************************************/

/*
 * The following function was written for
 * new hints management code in libafterstep :
 */
Bool
afterstep_parent_hints_func(Window parent, ASParentHints *dst )
{
	if( dst == NULL || parent == None ) return False ;

	memset( dst, 0x00, sizeof(ASParentHints));
	dst->parent = parent ;
	if( parent != Scr.Root )
	{
		ASWindow     *p;

        if ((p = window2ASWindow( parent )) == NULL) return False ;

        dst->desktop = p->status->desktop ;
		set_flags( dst->flags, AS_StartDesktop );

		/* we may need to move our viewport so that the parent becomes visible : */
        if ( !ASWIN_GET_FLAGS(p, AS_Iconic) )
		{
#if 0
            int x, y ;
            unsigned int width, height ;
            x = p->status->x - p->decor->west ;
            y = p->status->y - p->decor->north ;
            width = p->status->width + p->decor->west + p->decor->east ;
            height = p->status->height + p->decor->north + p->decor->south ;

            if( (dst->viewport_x = calculate_viewport( &x, width, p->scr->Vx, p->scr->MyDisplayWidth, p->scr->VxMax)) != p->scr->Vx )
				set_flags( dst->flags, AS_StartViewportX );

            if( (dst->viewport_y = calculate_viewport( &y, height, p->scr->Vy, p->scr->MyDisplayHeight, p->scr->VyMax)) != p->scr->Vy )
				set_flags( dst->flags, AS_StartViewportY );
#endif
		}
	}
	return True ;
}


void
init_aswindow(ASWindow * t, Bool free_resources )
{
	if (!t)
		return;
    if( free_resources && t->magic == MAGIC_ASWINDOW )
    {
        if( t->transients )
    	    destroy_asvector( &(t->transients) );
    	if( t->group_members )
        	destroy_asvector( &(t->group_members) );
        if( t->status )
        	free( t->status );
		if( t->hints )
		    destroy_hints( t->hints, False );
	}
    memset (t, 0x00, sizeof (ASWindow));
    t->magic = MAGIC_ASWINDOW ;
}

void
check_aswindow_shaped( ASWindow *asw )
{
    int           boundingShaped= False;
#ifdef SHAPE
    int           dumm, clipShaped= False;
    unsigned      udumm;
    XShapeQueryExtents (dpy, asw->w,
                        &boundingShaped, &dumm, &dumm, &udumm, &udumm,
                        &clipShaped, &dumm, &dumm, &udumm, &udumm);
#endif /* SHAPE */
    if( boundingShaped )
        ASWIN_SET_FLAGS( asw, AS_Shaped );
    else
        ASWIN_CLEAR_FLAGS( asw, AS_Shaped );
}

void quietly_reparent_aswindow( ASWindow *asw, Window dst, Bool user_root_pos )
{
    if( asw )
    {
		Window cover = get_desktop_cover_window();
		LOCAL_DEBUG_OUT( "cover = %lX", cover );
        quietly_reparent_canvas( asw->frame_canvas, dst, AS_FRAME_EVENT_MASK, user_root_pos, cover );
        quietly_reparent_canvas( asw->icon_canvas, dst, AS_ICON_EVENT_MASK, user_root_pos, cover );
        if( asw->icon_title_canvas != asw->icon_canvas )
            quietly_reparent_canvas( asw->icon_title_canvas, dst, AS_ICON_TITLE_EVENT_MASK, user_root_pos, cover );
    }
}

/****************************************************************************
 * Interprets the property MOTIF_WM_HINTS, sets decoration and functions
 * accordingly
 *****************************************************************************/
void
SelectDecor (ASWindow * t)
{
	ASHints *hints = t->hints ;
	ASFlagType tflags = hints->flags ;

    if (!get_flags(hints->function_mask,AS_FuncResize) || !get_flags( Scr.Look.flags, DecorateFrames))
	{
		/* a wide border, with corner tiles */
		if (get_flags(tflags,AS_Frame))
			clear_flags( t->hints->flags, AS_Frame );
	}
}


/***********************************************************************
 *
 *  Procedure:
 *	AddWindow - add a new window to the afterstep list
 *
 *  Returned Value:
 *	(ASWindow *) - pointer to the ASWindow structure
 *
 *  Inputs:
 *	w	- the window id of the window to add
 *	iconm	- flag to tell if this is an icon manager window
 *
 ***********************************************************************/
ASWindow     *
AddWindow (Window w, Bool from_map_request)
{
	ASWindow     *tmp_win;					   /* new afterstep window structure */
	ASRawHints    raw_hints ;
    ASHints      *hints  = NULL;
    ASStatusHints status;
    Bool pending_placement ;


	/* allocate space for the afterstep window */
	tmp_win = safecalloc (1, sizeof (ASWindow));
    init_aswindow( tmp_win, False );

    tmp_win->w = w;
    if (validate_drawable(w, NULL, NULL) == None)
	{
		free ((char *)tmp_win);
		return (NULL);
	}

	/* we want to set event mask as soon as possible so not to miss eny configure
	 * request that happen after we get client's geometry in collect_hints,
	 * and the moment when we afctually setup client's decorations. 
	 */
	XSelectInput( dpy, w, AS_CLIENT_EVENT_MASK );
		
    if( collect_hints( ASDefaultScr, w, HINT_ANY, &raw_hints ) )
    {
        if( is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
            print_hints( NULL, NULL, &raw_hints );
        hints = merge_hints( &raw_hints, Database, &status, Scr.Look.supported_hints, HINT_ANY, NULL, w );
        destroy_raw_hints( &raw_hints, True );
        if( hints )
        {
			show_debug( __FILE__, __FUNCTION__, __LINE__,  "Window management hints collected and merged for window %X", w );
            if( is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
			{
                print_clean_hints( NULL, NULL, hints );
				print_status_hints( NULL, NULL, &status );
			}
        }else
		{
			show_warning( "Failed to merge window management hints for window %X", w );
			free ((char *)tmp_win);
			XSelectInput( dpy, w, 0 );
			return (NULL);
		}
		tmp_win->hints = hints ;
    }else
	{
		show_warning( "Unable to obtain window management hints for window %X", w );
		free ((char *)tmp_win);
		XSelectInput( dpy, w, 0 );
		return (NULL);
	}
    SelectDecor (tmp_win);

    tmp_win->wm_state_transition = get_flags(status.flags, AS_Iconic)?ASWT_Withdrawn2Iconic:ASWT_Withdrawn2Normal ;

    pending_placement = init_aswindow_status( tmp_win, &status );
#ifdef SHAPE
    XShapeSelectInput (dpy, tmp_win->w, ShapeNotifyMask);
#endif
    check_aswindow_shaped( tmp_win );
    /*
	 * Make sure the client window still exists.  We don't want to leave an
	 * orphan frame window if it doesn't.  Since we now have the server
	 * grabbed, the window can't disappear later without having been
	 * reparented, so we'll get a DestroyNotify for it.  We won't have
	 * gotten one for anything up to here, however.
	 */
    ASSync( False );
    grab_server();
/*vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Start Grab vvvvvvvvvvvvvvvvvvvvv */
    if (validate_drawable(tmp_win->w, NULL, NULL) == None)
	{
		destroy_hints(tmp_win->hints, False);
		free ((char *)tmp_win);
		ungrab_server();
		return (NULL);
	}
    XSetWindowBorderWidth (dpy, tmp_win->w, 0);

	/* add the window into the afterstep list */
    enlist_aswindow( tmp_win );
    redecorate_window  ( tmp_win, False );
    /* saving window management properties : */
    set_client_desktop( tmp_win->w, ASWIN_DESK(tmp_win) );

	/* we have to set shape on frame window. If window has title - 
	 * on_window_title_changed will take care of it - otherwise we force it
	 * by calling SetShape directly.
	 */

	if( tmp_win->tbar )
	    on_window_title_changed ( tmp_win, False );
	else 
		SetShape( tmp_win, 0 );
	/* Must do it now or else Java will freak out !!! */
    XMapRaised (dpy, tmp_win->w);
	RaiseWindow(tmp_win);

/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ End Grab ^^^^^^^^^^^^^^^^^^^^ */
	ungrab_server();

	/* doing that for any window seems to resolve java sizing bugs */
    on_window_status_changed( tmp_win, False, True );
    if( pending_placement )
    {
        if( !place_aswindow( tmp_win ) )
        {
            LOCAL_DEBUG_OUT( "window status initialization failed for %lX - will put it where it is", w );
            /* 	Destroy (tmp_win, False);
            	return NULL;
			 */
        }
    }


    set_window_wm_state( tmp_win, get_flags(status.flags, AS_Iconic), from_map_request );
 
	if( ASWIN_HFLAGS( tmp_win, AS_AvoidCover )  )
		enforce_avoid_cover( tmp_win );

    /*
	 * Reparenting generates an UnmapNotify event, followed by a MapNotify.
	 * Set the map state to FALSE to prevent a transition back to
	 * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
	 * again in HandleMapNotify.
     */
    broadcast_config (M_ADD_WINDOW, tmp_win);
    broadcast_res_names( tmp_win );
    broadcast_icon_name( tmp_win );
    broadcast_window_name( tmp_win );

    InstallWindowColormaps (tmp_win);
	set_flags( tmp_win->internal_flags, ASWF_WindowComplete) ;

	return (tmp_win);
}

/* hints gets swallowed, but status does not : */
/* w must be unmapped !!!! */
ASWindow*
AddInternalWindow (Window w, ASInternalWindow **pinternal, ASHints **phints, ASStatusHints *status)
{
	ASWindow     *tmp_win;					   /* new afterstep window structure */
    ASHints      *hints = *phints ;

	/* allocate space for the afterstep window */
	tmp_win = safecalloc (1, sizeof (ASWindow));
    init_aswindow( tmp_win, False );

    tmp_win->w = w;
    if (validate_drawable(w, NULL, NULL) == None)
	{
		free ((char *)tmp_win);
		return (NULL);
	}

    tmp_win->internal = *pinternal ;
    *pinternal = NULL ;

    if( hints )
    {
        show_debug( __FILE__, __FUNCTION__, __LINE__,  "Window management hints supplied for window %X", w );
        if( is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
        {
            print_clean_hints( NULL, NULL, hints );
            print_status_hints( NULL, NULL, status );
        }
    }
    tmp_win->hints = hints ;
    *phints = NULL ;

    SelectDecor (tmp_win);

    tmp_win->wm_state_transition = get_flags(status->flags, AS_Iconic)?ASWT_Withdrawn2Iconic:ASWT_Withdrawn2Normal ;

    init_aswindow_status( tmp_win, status );

#ifdef SHAPE
    XShapeSelectInput (dpy, tmp_win->w, ShapeNotifyMask);
#endif

    XSetWindowBorderWidth (dpy, tmp_win->w, 0);

	/* add the window into the afterstep list */
    enlist_aswindow( tmp_win );

    redecorate_window       ( tmp_win, False );
    on_window_title_changed ( tmp_win, False );
    set_window_wm_state( tmp_win, get_flags(status->flags, AS_Iconic), False );
	RaiseWindow( tmp_win );
/*    activate_aswindow( tmp_win, False, False ); */

    /*
	 * Reparenting generates an UnmapNotify event, followed by a MapNotify.
	 * Set the map state to FALSE to prevent a transition back to
	 * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
	 * again in HandleMapNotify.
     */
    broadcast_config (M_ADD_WINDOW, tmp_win);
    broadcast_res_names( tmp_win );
    broadcast_icon_name( tmp_win );
    broadcast_window_name( tmp_win );

#if 0
/* TODO : */
	if (NeedToResizeToo)
	{
		XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
					  Scr.MyDisplayHeight,
					  tmp_win->frame_x + (tmp_win->frame_width >> 1),
					  tmp_win->frame_y + (tmp_win->frame_height >> 1));
		resize_window (tmp_win->w, tmp_win, 0, 0, 0, 0);
	}
#endif

	set_flags( tmp_win->internal_flags, ASWF_WindowComplete);
	return (tmp_win);
}

/***************************************************************************
 * Handles destruction of a window
 ****************************************************************************/
void
Destroy (ASWindow *asw, Bool kill_client)
{
    static int nested_level = 0 ;
    /*
	 * Warning, this is also called by HandleUnmapNotify; if it ever needs to
	 * look at the event, HandleUnmapNotify will have to mash the UnmapNotify
	 * into a DestroyNotify.
	 */
    if (AS_ASSERT(asw))
		return;
LOCAL_DEBUG_CALLER_OUT( "asw(%p)->internal(%p)->data(%p)", asw, asw->internal, asw->internal?asw->internal->data:NULL );

    /* we could be recursively called from delist_aswindow call - trying to prevent that : */
    if( nested_level > 0 )
        return;
    ++nested_level ;

    grab_server();
	if( !ASWIN_GET_FLAGS( asw, AS_Dead ) )
	{
		if( validate_drawable( asw->w, NULL, NULL ) == None )
        	ASWIN_SET_FLAGS( asw, AS_Dead );
	}

	if( !ASWIN_GET_FLAGS( asw, AS_Dead ) )
	{
        if( asw->internal == NULL )
            XRemoveFromSaveSet (dpy, asw->w);
        XSelectInput (dpy, asw->w, NoEventMask);
	}
    XUnmapWindow (dpy, asw->frame);
	grab_window_input( asw, True );

	if( Scr.moveresize_in_progress != NULL &&
		Scr.moveresize_in_progress->mr == asw->frame_canvas )
	{
		complete_interactive_action( Scr.moveresize_in_progress, True );
	    Scr.moveresize_in_progress = NULL ;
		if( !get_flags( asw->internal_flags, ASWF_WindowComplete) )
		{
			/* will be deleted from AddWindow  - can't destroy here, since we are in recursive event loop */
			ungrab_server();
		    --nested_level ;
			return;
		}
		/* otherwise we can delete window normally - there are no recursion */
	}

    XSync (dpy, 0);
	SendPacket (-1, M_DESTROY_WINDOW, 3, asw->w, asw->frame, (unsigned long)asw);

    UninstallWindowColormaps( asw );
    CheckWarpingFocusDestroyed(asw);
    CheckGrabbedFocusDestroyed(asw);

    if ( asw == Scr.Windows->focused )
        focus_prev_aswindow( asw );

    if ( asw == Scr.Windows->ungrabbed )
        Scr.Windows->ungrabbed = NULL;

    if ( asw == Scr.Windows->active )
        Scr.Windows->active = NULL;
    if ( asw == Scr.Windows->hilited )
        Scr.Windows->hilited = NULL;
    if ( asw == Scr.Windows->pressed )
        Scr.Windows->pressed = NULL;

    if (!kill_client && asw->internal == NULL )
        RestoreWithdrawnLocation (asw, True);

    if( asw->internal )
    {
        if( asw->internal->destroy )
            asw->internal->destroy( asw->internal );
        if( asw->internal->data ) /* in case destroy() above did not work properly */
            free( asw->internal->data );
        free( asw->internal );
        asw->internal = NULL ;
    }

    redecorate_window( asw, True );
    unregister_aswindow( asw->w );
	remove_iconbox_icon( asw );
    delist_aswindow( asw );

    init_aswindow( asw, True );

    XSync (dpy, 0);
    ungrab_server();

    memset( asw, 0x00, sizeof(ASWindow));
    free (asw);
    --nested_level ;

	return;
}

/***********************************************************************
 *  Procedure:
 *	RestoreWithdrawnLocation
 *  Puts windows back where they were before afterstep took over
 ************************************************************************/
void
RestoreWithdrawnLocation (ASWindow * asw, Bool restart)
{
    int x = 0, y = 0;
    unsigned int width = 0, height = 0, bw = 0 ;
    Bool map_too = False ;

LOCAL_DEBUG_CALLER_OUT("%p, %d", asw, restart );

    if (AS_ASSERT(asw))
		return;

    if( asw->status )
    {
        if( asw->status->width > 0 )
            width = asw->status->width ;
        if( asw->status->height > 0 )
            width = asw->status->height ;
        if( get_flags( asw->status->flags, AS_StartBorderWidth) )
            bw = asw->status->border_width ;
        /* We'll get withdrawn
            * location in virtual coordinates, so that when window is mapped again
            * we'll get it in correct position on the virtual screen.
            * Besides when we map window we check its postion so it would not be
            * completely off the screen ( if PPosition only, since User can place
            * window anywhere he wants ).
            *                             ( Sasha )
            */

        make_detach_pos( asw->hints, asw->status, &(asw->anchor), &x, &y );

        if ( get_flags(asw->status->flags, AS_Iconic ))
        {
            /* if we don't do that while exiting AfterStep -
               we will loose the client while starting AS again, as window will be
               unmapped, AS will be waiting for it to be mapped by client, and
               client will not be aware that it should do so, as WM exits and
               startups are transparent for it */
            map_too = True ;
        }
    	/*
     	* Prevent the receipt of an UnmapNotify, since that would
     	* cause a transition to the Withdrawn state.
     	*/
    	if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
    	{
        	XSelectInput (dpy, asw->w, NoEventMask);
        	if( get_parent_window( asw->w ) == asw->frame )
        	{
            	ASStatusHints withdrawn_state ;
            	/* !!! Most of it has been done in datach_basic_widget : */
            	XReparentWindow (dpy, asw->w, Scr.Root, x, y);
            	withdrawn_state.flags = AS_Withdrawn ;
            	withdrawn_state.icon_window = None ;
            	set_client_state( asw->w, &withdrawn_state );

            	if( width > 0 && height > 0 )
                	XResizeWindow( dpy, asw->w, width, height );
            	XSetWindowBorderWidth (dpy, asw->w, bw);

            	if( map_too )
                	XMapWindow (dpy, asw->w);
            	XSync( dpy, False );
        	}
        	/* signaling client that we no longer handle it : */
        	set_multi32bit_property (asw->w, _XA_WM_STATE, _XA_WM_STATE, 2, WithdrawnState, None);
    	}
	}
}

/**********************************************************************/
/* The End                                                            */
/**********************************************************************/

