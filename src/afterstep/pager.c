/*
 * Copyright (c) 2003 Sasha Vasko <sasha@ aftercode.net>
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

#define LOCAL_DEBUG
#include "../../configure.h"

#include "asinternals.h"

#include "../../libAfterStep/wmprops.h"
#include "../../libAfterStep/session.h"
#include "../../libAfterStep/moveresize.h"


/***************************************************************************
 *
 * Check to see if the pointer is on the edge of the screen, and scroll/page
 * if needed
 ***************************************************************************/
void
HandlePaging (int HorWarpSize, int VertWarpSize, int *xl, int *yt,
              int *delta_x, int *delta_y, Bool Grab, ASEvent *event)
{
#ifndef NO_VIRTUAL
	int           x, y, total;
#endif
    Window wdumm;
    int    dumm;
    unsigned int udumm;

	*delta_x = 0;
	*delta_y = 0;

#ifndef NO_VIRTUAL
	if (DoHandlePageing)
	{
        if ((Scr.Feel.EdgeResistanceScroll >= 10000) || ((HorWarpSize == 0) && (VertWarpSize == 0)))
			return;

		/* need to move the viewport */
		if ((*xl >= SCROLL_REGION) && (*xl < Scr.MyDisplayWidth - SCROLL_REGION) &&
			(*yt >= SCROLL_REGION) && (*yt < Scr.MyDisplayHeight - SCROLL_REGION))
			return;

		total = 0;
        while (total < (int)Scr.Feel.EdgeResistanceScroll)
		{
            register int i ;
			sleep_a_millisec(10);
			total += 10;
            for( i = 0 ; i < PAN_FRAME_SIDES ; i++ )
                if( Scr.PanFrame[i].isMapped )
                    if (ASCheckWindowEvent (Scr.PanFrame[i].win, LeaveWindowMask, &(event->x)))
                        return;
        }

        XQueryPointer (dpy, Scr.Root, &wdumm, &wdumm, &x, &y, &dumm, &dumm, &udumm);

		/* fprintf (stderr, "-------- MoveOutline () called from pager.c\ntmp_win == 0xlX\n", (long int) tmp_win); */
		/* Turn off the rubberband if its on */
		/*        MoveOutline ( Scr.Root,  tmp_win, 0, 0, 0, 0); */

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
            if (!get_flags(Scr.Feel.flags, EdgeWrapX))
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
            if (!get_flags(Scr.Feel.flags, EdgeWrapX))
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
            if (!get_flags(Scr.Feel.flags, EdgeWrapY))
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
            if (!get_flags(Scr.Feel.flags, EdgeWrapY))
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
				grab_server();
			XWarpPointer (dpy, None, Scr.Root, 0, 0, 0, 0, *xl, *yt);
			MoveViewport (Scr.Vx + *delta_x, Scr.Vy + *delta_y, False);
            XQueryPointer (dpy, Scr.Root, &wdumm, &wdumm, xl, yt, &dumm, &dumm, &udumm);
			if (Grab)
				ungrab_server();
		}
	}
#endif
}

/***************************************************************************
 *
 *  Moves the viewport within the virtual desktop
 *
 ***************************************************************************/

Bool
viewport_aswindow_iter_func( void *data, void *aux_data )
{
    ASWindow *asw = (ASWindow*)data ;
    if( asw )
    {
        asw->status->viewport_x = Scr.Vx ;
        asw->status->viewport_y = Scr.Vy ;
        if( !ASWIN_GET_FLAGS(asw,AS_Sticky) )
            on_window_status_changed( asw, True, True );

        if( ASWIN_GET_FLAGS(asw,AS_Iconic) && get_flags(Scr.Feel.flags, StickyIcons))
        {   /* we must update let all the modules know that icon viewport has changed
             * we dont have to do that for Sticky non-iconified windows, since its assumed
             * that those follow viewport, while Icons only do that when StckiIcons is set. */
            broadcast_config (M_CONFIGURE_WINDOW, asw);
        }
    }
    return True;
}


void
MoveViewport (int newx, int newy, Bool grab)
{

	ChangeDeskAndViewport ( Scr.CurrentDesk, newx, newy, grab);

#if 0
#ifndef NO_VIRTUAL
	int           deltax, deltay;

LOCAL_DEBUG_CALLER_OUT( "new(%+d%+d), old(%+d%+d), max(%+d,%+d)", newx, newy, Scr.Vx, Scr.Vy, Scr.VxMax, Scr.VyMax );

    if (newx > Scr.VxMax)
		newx = Scr.VxMax;
	if (newy > Scr.VyMax)
		newy = Scr.VyMax;
	if (newx < 0)
		newx = 0;
	if (newy < 0)
		newy = 0;

	deltay = Scr.Vy - newy;
	deltax = Scr.Vx - newx;

	Scr.Vx = newx;
	Scr.Vy = newy;
    SendPacket( -1, M_NEW_PAGE, 3, Scr.Vx, Scr.Vy, Scr.CurrentDesk);
    set_current_viewport_prop (Scr.wmprops, Scr.Vx, Scr.Vy, get_flags( AfterStepState, ASS_NormalOperation));

	if (deltax || deltay)
	{
		if (grab)
			grab_server();
        /* traverse window list and redo the titlebar/buttons if necessary */
        iterate_asbidirlist( Scr.Windows->clients, viewport_aswindow_iter_func, NULL, NULL, False );
        /* TODO: autoplace sticky icons so they don't wind up over a stationary icon */
	    check_screen_panframes(&Scr);
		if (grab)
			ungrab_server();
    }
#endif
#endif
}

/**************************************************************************
 * Move to a new desktop
 *************************************************************************/
Bool
deskviewport_aswindow_iter_func(void *data, void *aux_data)
{
    ASWindow *asw = (ASWindow *)data ;
    int new_desk = (int)Scr.CurrentDesk ;
	int dvx = asw->status->viewport_x - Scr.Vx ;
	int dvy = asw->status->viewport_y - Scr.Vy ;
	union {
		void *ptr ;
		int id ;
	}old_desk ;
	old_desk.ptr = aux_data;

    asw->status->viewport_x = Scr.Vx ;
    asw->status->viewport_y = Scr.Vy ;

	if( ASWIN_GET_FLAGS(asw, AS_Sticky) ||
        (ASWIN_GET_FLAGS(asw, AS_Iconic) && get_flags( Scr.Feel.flags, StickyIcons)) )
    {  /* Window is sticky */
		if( ASWIN_DESK(asw) != new_desk && IsValidDesk(new_desk))
        {
            ASWIN_DESK(asw) = new_desk ;
            if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
                set_client_desktop( asw->w, new_desk );
            broadcast_config (M_CONFIGURE_WINDOW, asw);
        }else if( ASWIN_GET_FLAGS(asw,AS_Iconic) && get_flags(Scr.Feel.flags, StickyIcons))
        {   /* we must update let all the modules know that icon viewport has changed
             * we dont have to do that for Sticky non-iconified windows, since its assumed
             * that those follow viewport, while Icons only do that when StckiIcons is set. */
            broadcast_config (M_CONFIGURE_WINDOW, asw);
        }
    }else
    {
		if( old_desk.id != new_desk )
        	quietly_reparent_aswindow( asw, (ASWIN_DESK(asw)==new_desk)?Scr.Root:Scr.ServiceWin, True );
		if( dvx != 0 || dvy != 0 )
			on_window_status_changed( asw, True, True );
    }
    return True;
}

Bool
count_desk_client_iter_func(void *data, void *aux_data)
{
    int *pcount = (int*)aux_data ;
    ASWindow *asw = (ASWindow *)data ;

    if( !ASWIN_GET_FLAGS(asw, AS_Sticky) &&
        (!ASWIN_GET_FLAGS(asw, AS_Iconic) || !get_flags( Scr.Feel.flags, StickyIcons)) )
    {
        if(ASWIN_DESK(asw)==Scr.CurrentDesk)
            ++(*pcount);
    }
    return True;
}

void change_desktop_background( int desk, int old_desk );

void
ChangeDeskAndViewport ( int new_desk, int new_vx, int new_vy, Bool force_grab)
{
	int dvx, dvy;
    int old_desk = Scr.CurrentDesk ;

LOCAL_DEBUG_CALLER_OUT( "new(%d%+d%+d), old(%d%+d%+d), max(%+d,%+d)", new_desk, new_vx, new_vy, Scr.CurrentDesk, Scr.Vx, Scr.Vy, Scr.VxMax, Scr.VyMax );

    if (new_vx > Scr.VxMax)
		new_vx = Scr.VxMax;
	else if (new_vx < 0)
		new_vx = 0;
	if (new_vy > Scr.VyMax)
		new_vy = Scr.VyMax;
	else if (new_vy < 0)
		new_vy = 0;

	dvx = Scr.Vx - new_vx;
	dvy = Scr.Vy - new_vy;

    if( IsValidDesk( old_desk ) )
		Scr.LastValidDesk = old_desk ;

    /* we have to handle all the pending ConfigureNotifys here : */
    ConfigureNotifyLoop();

    if( old_desk != new_desk && IsValidDesk( old_desk ) )
    {
        int client_count = 0 ;
        iterate_asbidirlist( Scr.Windows->clients, count_desk_client_iter_func, (void*)&client_count, NULL, False );
        if( client_count != 0 )
            old_desk = INVALID_DESK ;
		force_grab = True ;
	}

	if( Scr.CurrentDesk != new_desk )
	{
		Scr.CurrentDesk = new_desk;

   		if( as_desk2ext_desk (Scr.wmprops, new_desk) == INVALID_DESKTOP_PROP )
       		set_desktop_num_prop( Scr.wmprops, new_desk, Scr.Root, True );
    	set_current_desk_prop ( Scr.wmprops, new_desk);
	}

	if( dvx != 0 || dvy != 0 )
	{
		Scr.Vx = new_vx;
		Scr.Vy = new_vy;
    	set_current_viewport_prop (Scr.wmprops, Scr.Vx, Scr.Vy, get_flags( AfterStepState, ASS_NormalOperation));
	}
   	SendPacket( -1, M_NEW_DESKVIEWPORT, 3, Scr.Vx, Scr.Vy, Scr.CurrentDesk);

	if (dvx != 0 || dvy != 0 || old_desk != new_desk )
	{
		union
		{
			void *ptr ;
			int id ;
		}desk_id;

		if ( force_grab )
			grab_server();
		if( Scr.moveresize_in_progress && !Scr.moveresize_in_progress->move_only )
		{
			ASWindow *asw = window2ASWindow( AS_WIDGET_WINDOW(Scr.moveresize_in_progress->mr));
			if( !get_flags (asw->status->flags, AS_Sticky) )
			{
				Scr.moveresize_in_progress->curr.x += dvx ;
				Scr.moveresize_in_progress->curr.y += dvy ;
				Scr.moveresize_in_progress->curr.width -= dvx ;
				Scr.moveresize_in_progress->curr.height -= dvy ;

				Scr.moveresize_in_progress->start.x += dvx ;
				Scr.moveresize_in_progress->start.y += dvy ;
				Scr.moveresize_in_progress->start.width  -= dvx ;
				Scr.moveresize_in_progress->start.height -= dvy ;
			}
		}
		desk_id.id = old_desk ;
        /* traverse window list and redo the titlebar/buttons if necessary */
        iterate_asbidirlist( Scr.Windows->clients, deskviewport_aswindow_iter_func, desk_id.ptr, NULL, False );
        /* TODO: autoplace sticky icons so they don't wind up over a stationary icon */
	    check_screen_panframes(&Scr);
		if ( force_grab)
			ungrab_server();
    }
	/* yield to let modules handle desktop/viewport change */
	FlushAllQueues();

	if( old_desk != new_desk )
	{
    	if (get_flags(Scr.Feel.flags, ClickToFocus))
		{
        	int i ;
        	int circ_count = VECTOR_USED(*(Scr.Windows->circulate_list));
        	if( !get_flags( Scr.Feel.flags, DontRestoreFocus ) )
        	{
            	ASWindow **circ_list = VECTOR_HEAD(ASWindow*,*(Scr.Windows->circulate_list));
            	for( i =0 ; i < circ_count ; ++i )
                	if( circ_list[i] != NULL && circ_list[i]->magic == MAGIC_ASWINDOW )
                    	if( ASWIN_DESK(circ_list[i]) == new_desk )
                    	{
                        	focus_aswindow( circ_list[i] );
                        	break;
                    	}
        	}else
            	i = circ_count ;

        	if( i >= circ_count )
            	hide_focus();
    	}

    	if( IsValidDesk(new_desk) )
		{
        	restack_window_list( new_desk, False );
			FlushAllQueues();              /* yield to modules */
		}
    	/* Change the look to this desktop's one if it really changed */
#ifdef DIFFERENTLOOKNFEELFOREACHDESKTOP
		if( get_flags( AfterStepState, ASS_NormalOperation) )
			QuickRestart ("look&feel");
#endif

    	/* we need to set the desktop background : */
		if( (IsValidDesk( new_desk ) && IsValidDesk( old_desk )) ||
			Scr.LastValidDesk != new_desk )
		{
	    	change_desktop_background(new_desk, Scr.LastValidDesk);
		}
	}

}


void
ChangeDesks (int new_desk)
{
	ChangeDeskAndViewport ( new_desk, Scr.Vx, Scr.Vy, False);
#if 0
    int old_desk = Scr.CurrentDesk ;
LOCAL_DEBUG_CALLER_OUT( "new_desk(%d)->old_desk(%d)", new_desk, old_desk );
    if( Scr.CurrentDesk == new_desk )
        return;

    if( IsValidDesk( old_desk ) )
		Scr.LastValidDesk = old_desk ;

    /* we have to handle all the pending ConfigureNotifys here : */
    ConfigureNotifyLoop();

    if( IsValidDesk( Scr.CurrentDesk ) )
    {
        int client_count = 0 ;
        iterate_asbidirlist( Scr.Windows->clients, count_desk_client_iter_func, (void*)&client_count, NULL, False );
        if( client_count != 0 )
            old_desk = INVALID_DESK ;
    }

    Scr.CurrentDesk = new_desk;
    if( as_desk2ext_desk (Scr.wmprops, new_desk) == INVALID_DESKTOP_PROP )
        set_desktop_num_prop( Scr.wmprops, new_desk, Scr.Root, True );

    set_current_desk_prop ( Scr.wmprops, new_desk);

    /* SendPacket( -1, M_NEW_DESK, 1, new_desk); */
	/* yeld to let modules handle desktop change */
	sleep_a_millisec(500);
    /* Scan the window list, mapping windows on the new Desk, unmapping
	 * windows on the old Desk; do this in reverse order to reduce client
	 * expose events */
	grab_server();
    iterate_asbidirlist( Scr.Windows->clients, change_aswindow_desk_iter_func, (void*)new_desk, NULL, False );
    ungrab_server();

    if (get_flags(Scr.Feel.flags, ClickToFocus))
	{
        int i ;
        int circ_count = VECTOR_USED(*(Scr.Windows->circulate_list));
        if( !get_flags( Scr.Feel.flags, DontRestoreFocus ) )
        {
            ASWindow **circ_list = VECTOR_HEAD(ASWindow*,*(Scr.Windows->circulate_list));
            for( i =0 ; i < circ_count ; ++i )
                if( circ_list[i] != NULL && circ_list[i]->magic == MAGIC_ASWINDOW )
                    if( ASWIN_DESK(circ_list[i]) == new_desk )
                    {
                        focus_aswindow( circ_list[i] );
                        break;
                    }
        }else
            i = circ_count ;

        if( i >= circ_count )
            hide_focus();
    }

    if( IsValidDesk(new_desk) )
        restack_window_list( new_desk, False );

    /* Change the look to this desktop's one if it really changed */
#ifdef DIFFERENTLOOKNFEELFOREACHDESKTOP
	QuickRestart ("look&feel");
#endif

    /* we need to set the desktop background : */
	if( (IsValidDesk( new_desk ) && IsValidDesk( old_desk )) ||
		Scr.LastValidDesk != new_desk )
	{
	    change_desktop_background(new_desk, Scr.LastValidDesk);
	}/* otherwise we were switching to service desk number in order to switch
		viewport and desktop change is not needed */

#if 0
    /* autoplace sticky icons so they don't wind up over a stationary icon */
	AutoPlaceStickyIcons ();
#endif
#endif

}

/*************************************************************************
 *  Root Background handling :
 * - Each root background is described by MyBackground object.
 * - MyBackground could produce ASImage or it could run an external command
 * - If ASImage is produced - it will be stored either under its filename,
 *   if there were no transformation performed, or under MyBackground's name
 * - When there are no windows on the desk and it gets switched and its
 *   background size is over the threshold - it will be destroyed/released
 *   from hash.
 * - if destroyed ASImage is same as Scr.RootImage - Scr.RootImage will be
 *   set to NULL.
 * - ASImage also produces pixmap that goes into Root window background
 * - pixmap gets published using _X_ROOTPMAP property
 * - when PropertyNotify event comes and property is set to current pixmap -
 *   its ASImage will be assigned to Scr.RootImage
 * - If pixmap is unknown - then Scr.RootImage will be set to NULL and it
 *   will be generated as required by mystyle functions.
 * - If Scr.RootImage has no name - it will be destroyed when PropertyNotify
 *   event arrives
 **************************************************************************/

MyBackground *get_desk_back_or_default( int desk, Bool old_desk )
{
	MyBackground *myback = mylook_get_desk_back( &(Scr.Look), desk );
	if( myback == NULL && !old_desk && !get_flags( Scr.Look.flags, DontDrawBackground ))
	{
		const char *const_configfile = get_session_file (Session, desk, F_CHANGE_BACKGROUND, True);
		if( const_configfile != NULL )
		{  /* there is a preconfigured background file - so creating MyBack,
			* even thou look does not have one. */
			MyDesktopConfig *dc ;
			char * buf = safemalloc( strlen(DEFAULT_BACK_NAME)+15+1 );

			sprintf( buf, DEFAULT_BACK_NAME, desk );
			dc = mylook_get_desk_config( &(Scr.Look), desk );
			if( dc == NULL )
			{
		    	dc = create_mydeskconfig( desk, buf );
				free( buf );
				dc = add_deskconfig( &(Scr.Look), dc );
			}
			if( dc != NULL )
			{
       			myback = mylook_get_back( &(Scr.Look), dc->back_name);
        		if( myback == NULL  )
        		{
            		myback = create_myback( dc->back_name );
            		myback->type = MB_BackImage ;
            		add_myback( &(Scr.Look), myback );
				}
			}
		}
	}
	return myback;
}


ASImage *
load_myback_image( int desk, MyBackground *back )
{
    ASImage *im = NULL ;
    if( back->data && back->data[0] )
	{
		LOCAL_DEBUG_OUT( "Attempting to load background image from \"%s\"", back->data );
        im = get_asimage( Scr.image_manager, back->data, 0xFFFFFFFF, 100 );
	}

    if( im == NULL )
    {
        const char *const_configfile = get_session_file (Session, desk, F_CHANGE_BACKGROUND, False);
        if( const_configfile != NULL )
        {
            im = get_asimage( Scr.image_manager, const_configfile, 0xFFFFFFFF, 100 );
            show_progress("BACKGROUND for desktop %d loaded from \"%s\" ...", desk, const_configfile);
        }else
            show_progress("BACKGROUND file cannot be found for desktop %d", desk );
    }
    if( im != NULL )
    {
        ASImage *scaled_im = NULL ;
        ASImage *tiled_im = NULL ;
        /* crop and or tint */
        if( back->cut.flags != 0 || (back->tint != TINT_NONE && back->tint != TINT_LEAVE_SAME))
        {

        }
        /* scale */
		LOCAL_DEBUG_OUT( "scale.flags = 0x%X", back->scale.flags );
		LOCAL_DEBUG_OUT( "scale.width = %d", back->scale.width );
		LOCAL_DEBUG_OUT( "scale.height = %d", back->scale.height );
		if( get_flags(back->scale.flags, (WidthValue|HeightValue)) )
        {
            int width = im->width ;
            int height = im->height ;
            if( get_flags(back->scale.flags, WidthValue) )
                width = back->scale.width;
            if( get_flags(back->scale.flags, HeightValue) )
                height = back->scale.height;
            if( width != im->width || height != im->height )
            {
                scaled_im = scale_asimage( Scr.asv, im, width, height, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
                LOCAL_DEBUG_OUT( "image scaled to %dx%d. tmp_im = %p", width, height, scaled_im );
                if( scaled_im != im )
                {
                    safe_asimage_destroy( im );
                    im = scaled_im ;
                }
            }
        }
        /* pad */
        if( back->align_flags != NO_ALIGN && (im->width != Scr.MyDisplayWidth || im->height != Scr.MyDisplayHeight))
        {
            int x = 0, y = 0;
            x = Scr.MyDisplayWidth - im->width ;
            if( get_flags( back->align_flags, ALIGN_LEFT ) )
                x = 0 ;
            else if( get_flags( back->align_flags, ALIGN_HCENTER ) )
                x /= 2;

            y = Scr.MyDisplayHeight - im->height;
            if( get_flags( back->align_flags, ALIGN_TOP ) )
                y = 0 ;
            else if( get_flags( back->align_flags, ALIGN_VCENTER ) )
                y /= 2;

            tiled_im = pad_asimage( Scr.asv, im, x, y, Scr.MyDisplayWidth, Scr.MyDisplayHeight, back->pad_color,
                                           ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
            if( tiled_im != im )
            {
                safe_asimage_destroy( im );
                im = tiled_im ;
            }
        }
        if( im->width > Scr.MyDisplayWidth || im->height > Scr.MyDisplayHeight )
        {
            int width = ( im->width > Scr.MyDisplayWidth )?Scr.MyDisplayWidth:im->width ;
            int height = ( im->height > Scr.MyDisplayHeight )?Scr.MyDisplayHeight:im->height ;

            tiled_im = tile_asimage( Scr.asv, im, 0, 0, width, height, TINT_LEAVE_SAME, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
            LOCAL_DEBUG_OUT( "image croped to %dx%d. tmp_im = %p", width, height, tiled_im );
            if( tiled_im != im )
            {
                safe_asimage_destroy( im );
                im = tiled_im ;
            }
        }
    }
#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing %s","");
    ASSync(False);
#endif
    return im;
}

void
release_old_background( int desk, Bool forget )
{
    MyBackground *back = get_desk_back_or_default( desk, True );
    ASImage *im = NULL ;
    char *imname ;

	LOCAL_DEBUG_CALLER_OUT("%d,%d", desk, forget);
    if( back == NULL || back->type == MB_BackCmd )
        return ;
	
	imname = make_myback_image_name( &(Scr.Look), back->name );

    if( back->loaded_im_name )
    {
        im = query_asimage( Scr.image_manager, back->data );
        LOCAL_DEBUG_OUT( "query_asimage \"%s\" - returned %p", back->data, im );
    }
	
	if( im == NULL ) 
	{
  		im = query_asimage( Scr.image_manager, imname );
	    LOCAL_DEBUG_OUT( "query_asimage \"%s\" - returned %p", imname, im );
	}

    if( im == NULL && back->type == MB_BackImage )
    {
        if( im == NULL )
        {
            const char *const_configfile = get_session_file (Session, desk, F_CHANGE_BACKGROUND, False);
            if( const_configfile != NULL )
            {
                im = query_asimage( Scr.image_manager, const_configfile );
                LOCAL_DEBUG_OUT( "query_asimage \"%s\" - returned %p", const_configfile, im );
            }
        }
    }
#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing %s","");
    ASSync(False);
#endif
    if( im != NULL )
    {
        if( Scr.RootImage == im )
        {                                      /* always do that ! */
            safe_asimage_destroy( im );
            Scr.RootImage = NULL ;
        }
        
		if( forget || im->width*im->height >= Scr.Look.KillBackgroundThreshold )
        {
            LOCAL_DEBUG_OUT( "im = %p, ref_count = %d", im, im->ref_count );
            safe_asimage_destroy( im );
        }else if( strcmp( im->name, imname) != 0 )	 /* we need to store it for future use ! */
		{	                                     	 /* for future references we store image under imname */
			if( im->ref_count > 1 )
			{ 		   /* then something else is using this image, and we need to duplicate it  */		   
				release_asimage( im );
				im = clone_asimage( im, 0xFFFFFFFF );
			}else
				forget_asimage( im );
			store_asimage( Scr.image_manager, im, imname );
		}	 
		if( back->loaded_im_name )
		{	
			free( back->loaded_im_name );
			back->loaded_im_name = mystrdup(im->name); 
		}
    }
	free( imname );

#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing, loaded_pixmap = %lX", back->loaded_pixmap );
    ASSync(False);
#endif
	if( back->loaded_pixmap && (forget || Scr.Feel.conserve_memory > 0) ) 
	{
		LOCAL_DEBUG_OUT( "ROOT_PIXMAP = %lX at %d", Scr.RootBackground->pmap, __LINE__ );
		if( Scr.RootBackground->pmap == back->loaded_pixmap ) 
			Scr.RootBackground->pmap = None ;
		LOCAL_DEBUG_OUT( "destroying pixmap %lX", back->loaded_pixmap );
		destroy_visual_pixmap( Scr.asv, &(back->loaded_pixmap));			
	}	 
}

ASImage*
make_desktop_image( int desk, MyBackground *new_back )
{
    ASImage *new_im = NULL;
    char *new_imname = NULL ; 

    if( new_back->loaded_im_name )
		new_im = fetch_asimage( Scr.image_manager, new_back->loaded_im_name );
	
	if( new_im == NULL ) 
	{	
		new_imname = make_myback_image_name( &(Scr.Look), new_back->name );
    	if( new_back->type == MB_BackImage )
		{	
			if( (new_im = fetch_asimage( Scr.image_manager, new_imname )) == NULL )
			{	
				LOCAL_DEBUG_OUT( "fetch_asimage could not find the back \"%s\" - loading ", new_imname );
			}
		}
		if( new_im == NULL && new_back->loaded_pixmap != None )
		{
			unsigned int width, height ;
			get_drawable_size( new_back->loaded_pixmap, &width, &height );
			new_im = pixmap2asimage(Scr.asv, new_back->loaded_pixmap, 0, 0, width, height, 0xFFFFFFFF, False, 100);
			store_asimage( Scr.image_manager, new_im, new_imname );
		}
	}			
	if( new_im != NULL ) 
		return new_im ;
	
	if( new_back->type == MB_BackImage )
    {
        if( (new_im = load_myback_image( desk, new_back )) != NULL )
		{
  		    if( new_im->name == NULL )
      		    store_asimage( Scr.image_manager, new_im, new_imname );
		}
    }else if( new_back->type == MB_BackMyStyle )
    {
        MyStyle *style = mystyle_find_or_default( new_back->data );
        int root_width = Scr.MyDisplayWidth;
        int root_height = Scr.MyDisplayHeight ;
        ASImage *old_root = Scr.RootImage;

        if( style->texture_type == TEXTURE_SOLID || style->texture_type == TEXTURE_TRANSPARENT ||
            style->texture_type == TEXTURE_TRANSPARENT_TWOWAY )
            root_width = root_height = 1 ;
        else if( style->texture_type == TEXTURE_PIXMAP || style->texture_type == TEXTURE_SHAPED_PIXMAP ||
                 (style->texture_type >= TEXTURE_TRANSPIXMAP && style->texture_type < TEXTURE_SCALED_TRANSPIXMAP ))
        {
            if( root_width > style->back_icon.width )
                root_width = style->back_icon.width ;
            if( root_height > style->back_icon.height )
                root_height = style->back_icon.height ;
        }
        if( style->texture_type >= TEXTURE_TRANSPARENT )
        {
            Scr.RootImage = create_asimage( root_width, 1, 100 );
            Scr.RootImage->back_color = style->colors.back ;
        }
        LOCAL_DEBUG_OUT( "drawing root background using mystyle \"%s\" size %dx%d", style->name, root_width, root_height );
        new_im = mystyle_make_image( style, 0, 0, root_width, root_height, 0 );

        if( style->texture_type >= TEXTURE_TRANSPARENT )
        {
            destroy_asimage( &(Scr.RootImage) );
            Scr.RootImage = old_root;
        }
		if( new_im )
	        store_asimage( Scr.image_manager, new_im, new_imname );
    }

	if( new_im != NULL ) 
	{
		set_string_value( &(new_back->loaded_im_name), mystrdup(new_im->name), NULL, 0 );
		LOCAL_DEBUG_OUT( "produced image %p", new_im );
	}
	
#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing %s","");
    ASSync(False);
    if( new_im )
        LOCAL_DEBUG_OUT( "im(%p)->ref_count(%d)->name(\"%s\")", new_im, new_im->ref_count, new_im->name );
#endif
    if( new_imname )
        free( new_imname );
    
    return new_im;
}

typedef struct ASBackgroundXferData
{
	char 		*im_name ;
	ASImage 	*im_ptr ;
	Pixmap       target_pmap ;
	int 		 lines_per_iteration, lines_done, total_lines ;
	XImage		*shm_ximage ;
	int step ;
}ASBackgroundXferData;

static unsigned long background_xfer_shmseg = 0 ;

Bool
is_background_xfer_ximage( unsigned long id ) 
{return (id==background_xfer_shmseg);}	  

void
do_background_xfer_iter( void *vdata )
{
	ASBackgroundXferData *data = (ASBackgroundXferData *)vdata ;
	ASImage *im ;
	Bool success = False ; 
LOCAL_DEBUG_CALLER_OUT( "%p:\"%s\", pmap %lX ", vdata, data->im_name, data->target_pmap );	
	if( data == NULL )
		return ; 
	if( Scr.RootBackground != NULL ) 
	{	
		LOCAL_DEBUG_OUT( "target_pmap = %lX, curr pmap = %lX", data->target_pmap, Scr.RootBackground->pmap );
		if( data->target_pmap == Scr.RootBackground->pmap )
		{	
			if( (im = query_asimage( Scr.image_manager, data->im_name )) != NULL )
			{
				LOCAL_DEBUG_OUT( "found image %p, named as it should. Original was %p", im, data->im_ptr );
				if( im == data->im_ptr ) 
				{
					XImage *xim = data->shm_ximage ;
					int lines = data->lines_per_iteration ;
					int y = data->lines_done/2;

					if( lines > data->total_lines - data->lines_done )
						lines = data->total_lines - data->lines_done ;
					if( data->step & 0x000001 )
						y = (data->total_lines - (data->lines_done-data->lines_per_iteration)/2) - lines ;
												   
					if( xim == NULL ) 
						xim = create_visual_scratch_ximage( Scr.asv, im->width, lines, 0 );
					LOCAL_DEBUG_OUT( "making ximage %p, starting at %d, and including %d lines", xim, data->lines_done, lines );
					if( subimage2ximage (Scr.asv, im, 0, y, xim)	)
					{	
						Bool res ;
						LOCAL_DEBUG_OUT( "done, copying to pixmap at %d, (bytes_per_line = %d)", data->lines_done, xim->bytes_per_line );
						res = put_ximage( Scr.asv, xim, Scr.RootBackground->pmap, 
					            	Scr.DrawGC,  0, 0, 0, y, im->width, lines );	
						LOCAL_DEBUG_OUT( "%s", res?"Success":"Failure" );
						data->lines_done += lines ;
						++(data->step);

						if( im->width >= Scr.MyDisplayWidth && im->height >= Scr.MyDisplayHeight )
							XClearWindow( dpy, Scr.Root );/* only if not tiled ! */
						ASSync(False);
					}
					if( xim != data->shm_ximage )
						XDestroyImage( xim );				   
					success = True ;
				}	 
			}
		}	
	}
	if( success && data->lines_done >= data->total_lines )
	{
        XSetWindowBackgroundPixmap( dpy, Scr.Root, Scr.RootBackground->pmap );
        XClearWindow( dpy, Scr.Root );
        set_xrootpmap_id (Scr.wmprops, Scr.RootBackground->pmap );
		ASSync(False);
	}	 

	if( !success || data->lines_done >= data->total_lines )
	{
		if( data->shm_ximage ) 
			XDestroyImage( data->shm_ximage );

		background_xfer_shmseg = 0 ;
		free( data->im_name );
		free( data );
	}else
	{
		timer_new (10, do_background_xfer_iter, vdata);	
	}		 
	
}	 

static ASBackgroundXferData *
start_background_xfer( ASImage *new_im )
{
 	ASBackgroundXferData *data = safecalloc( 1, sizeof(ASBackgroundXferData));

	data->im_name = mystrdup(new_im->name); 
	data->im_ptr = new_im ; 
	data->target_pmap = Scr.RootBackground->pmap ; 
	if( new_im->width < Scr.MyDisplayWidth || new_im->height < Scr.MyDisplayHeight )
		data->lines_per_iteration = ASSHM_SAVED_MAX / (new_im->width * 4) ;
	else
	{	
#ifdef __CYGWIN__		
		/* under Windows XImage must not exceed 32K , or performance drops sagnificantly */
		data->lines_per_iteration = 7800/new_im->width ;
#else		
		data->lines_per_iteration = Scr.MyDisplayHeight/90;
#endif
	}
	if( data->lines_per_iteration == 0 ) 
		data->lines_per_iteration = 1 ;
	data->lines_done = 0 ;
	data->total_lines = new_im->height ;
	data->shm_ximage = NULL ; 
#ifdef XSHMIMAGE		
	if(check_shmem_images_enabled())
	{	
		data->shm_ximage = create_visual_ximage( Scr.asv, new_im->width, data->lines_per_iteration, 0 );
		background_xfer_shmseg = ximage2shmseg(data->shm_ximage) ;
	}
#endif	   

	do_background_xfer_iter( data );
	return data;
}



void
change_desktop_background( int desk, int old_desk )
{
    MyBackground *new_back = get_desk_back_or_default( desk, False );
    MyBackground *old_back = get_desk_back_or_default( old_desk, True );
    ASImage *new_im = NULL ;
	static ASBackgroundXferData *last_back_xfer = NULL ; 

LOCAL_DEBUG_CALLER_OUT( "desk(%d)->old_desk(%d)->new_back(%p)->old_back(%p)", desk, old_desk, new_back, old_back );
    if( new_back == NULL )
        return ;

    if( new_back->loaded_im_name != NULL  && Scr.RootBackground )
	{
		if( Scr.RootBackground->im != NULL && Scr.RootBackground->im->name != NULL )
			if( strcmp( new_back->loaded_im_name, Scr.RootBackground->im->name ) == 0 ) 
				return ; /* same background as what we already have - do nothing */
	}
	
    if( new_back == old_back &&
        desk != old_desk ) /* if desks are the same then we are reloading current background !!! */
        return;
    
    if( Scr.RootBackground != NULL )
	{	
		LOCAL_DEBUG_OUT( "ROOT_PIXMAP = %lX at %d", Scr.RootBackground->pmap, __LINE__ );
	}

#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing %s","");
    ASSync(False);
#endif
	if( old_back ) 
	{LOCAL_DEBUG_OUT( "old_back>>> desk = %d, ptr = %p, loaded_im_name = \"%s\", pixmap = %lX", old_desk, old_back, old_back->loaded_im_name, old_back->loaded_pixmap );}
	if( new_back ) 
	{LOCAL_DEBUG_OUT( "new_back>>> desk = %d, ptr = %p, loaded_im_name = \"%s\", pixmap = %lX", desk, new_back, new_back->loaded_im_name, new_back->loaded_pixmap );}
    if( Scr.RootBackground == NULL )
        Scr.RootBackground = safecalloc( 1, sizeof(ASBackgroundHandler));
    else
    {                                          /* do cleanup - kill command maybe */
        if( Scr.RootBackground->cmd_pid )
            check_singleton_child (BACKGROUND_SINGLETON_ID, True);
        Scr.RootBackground->cmd_pid = 0;
        Scr.RootBackground->im = NULL ;
    }
	 
    release_old_background( old_desk, (desk==old_desk) );
	if( new_back->loaded_pixmap ) 
	{
		ASBackgroundHandler *bh = Scr.RootBackground ;			
 		unsigned int width, height ;       
		if( !get_drawable_size(new_back->loaded_pixmap, &width, &height ) )
			new_back->loaded_pixmap = None ;
		else
		{
			bh->pmap_width = width ;
        	bh->pmap_height = height ;
        	bh->im = NULL;
			bh->pmap = new_back->loaded_pixmap ;
			XSetWindowBackgroundPixmap( dpy, Scr.Root, new_back->loaded_pixmap );
			XClearWindow( dpy, Scr.Root );
			set_xrootpmap_id (Scr.wmprops, new_back->loaded_pixmap );
			return ;   
		}
	}		  
    cover_desktop();
   	display_progress( True, "Changing background for desktop #%d ...", desk);
	

    if( new_back->type == MB_BackCmd )
    {                                          /* run command */
        Scr.RootBackground->cmd_pid = spawn_child( XIMAGELOADER, BACKGROUND_SINGLETON_ID, Scr.screen, None, C_NO_CONTEXT, True, False, new_back->data, NULL );
    }else
        new_im = make_desktop_image( desk, new_back );

    LOCAL_DEBUG_OUT( "im(%p)", new_im );
    if( new_im )
    {
        ASBackgroundHandler *bh = Scr.RootBackground ;
		Pixmap old_pmap ;
        /* Scr.RootImage = new_im ; */         /* will be done in event handler !!! */
        if( new_im->name == NULL )
        {
            char *new_imname = make_myback_image_name( &(Scr.Look), new_back->name );
            store_asimage( Scr.image_manager, new_im, new_imname );
        }
		LOCAL_DEBUG_OUT( "ROOT_PIXMAP = %lX at %d", Scr.RootBackground->pmap, __LINE__ );
		if( old_back && bh->pmap == old_back->loaded_pixmap)
		{
			if( Scr.Feel.conserve_memory == 0 )
				bh->pmap = None ;				
			else
				old_back->loaded_pixmap = None ;
		}
		LOCAL_DEBUG_OUT( "ROOT_PIXMAP = %lX at %d", Scr.RootBackground->pmap, __LINE__ );
		old_pmap = bh->pmap ;
        if( bh->pmap && (new_im->width != bh->pmap_width ||
            new_im->height != bh->pmap_height) )
        {
			if( Scr.wmprops->root_pixmap == bh->pmap )
				set_xrootpmap_id (Scr.wmprops, None );
            XFreePixmap( dpy, bh->pmap );
            ASSync(False);
            LOCAL_DEBUG_OUT( "root pixmap with id %lX destroyed", bh->pmap );
            bh->pmap = None ;
        }
        LOCAL_DEBUG_OUT( "ROOT_PIXMAP = %lX at %d", Scr.RootBackground->pmap, __LINE__ );
		if( !validate_drawable( bh->pmap, NULL, NULL ) )
			bh->pmap = None ;
		if( bh->pmap == None )
		{
            bh->pmap = create_visual_pixmap( Scr.asv, Scr.Root, new_im->width, new_im->height, 0 );
            LOCAL_DEBUG_OUT( "new root pixmap created with id %lX and size %dx%d", bh->pmap, new_im->width, new_im->height );
		}else
            LOCAL_DEBUG_OUT( "using root pixmap created with id %lX", bh->pmap );

        bh->pmap_width = new_im->width ;
        bh->pmap_height = new_im->height ;
        bh->im = new_im;

		new_back->loaded_pixmap = bh->pmap ;
		/*print_asimage( new_im, 0xFFFFFFFF, __FUNCTION__, __LINE__ );*/
        ASSync(False);
        LOCAL_DEBUG_OUT( "width(%d)->height(%d)->pixmap(%lX/%lu)", new_im->width, new_im->height, bh->pmap, bh->pmap );
		if( old_back ) 
		{LOCAL_DEBUG_OUT( "old_back>>>#2 desk = %d, ptr = %p, loaded_im_name = \"%s\", pixmap = %lX", old_desk, old_back, old_back->loaded_im_name, old_back->loaded_pixmap );}
		if( new_back ) 
		{LOCAL_DEBUG_OUT( "new_back>>>#2 desk = %d, ptr = %p, loaded_im_name = \"%s\", pixmap = %lX", desk, new_back, new_back->loaded_im_name, new_back->loaded_pixmap );}
		
		/* cancel last background xfer is there was any  */
		if( last_back_xfer )
		{	
			timer_remove_by_data( last_back_xfer );
			last_back_xfer = NULL ;
		}
		
		if( new_im->width * new_im->height * 4 >= ASSHM_SAVED_MAX/2 &&
			/* can't animate if pixmap is tiled - X is slow then */
			!get_flags( Scr.Feel.flags, DontAnimateBackground) && 
			/* when we do this for the first time  - we better do it all at once */
			Scr.wmprops->root_pixmap != None )
		{	
			Bool tiled = ( new_im->width < Scr.MyDisplayWidth || new_im->height < Scr.MyDisplayHeight );
			if( old_pmap != bh->pmap )
			{	
				if( !tiled ) 
					XSetWindowBackgroundPixmap( dpy, Scr.Root, bh->pmap );
			}else if( tiled ) 
			{
				/* animation of tiled backgrounds causes X to be very slow */
				XSetWindowBackgroundPixmap( dpy, Scr.Root, None );
			}

		 	last_back_xfer = start_background_xfer( new_im );  /* we need to do it in small steps! */
		}else
		{	
        	if( !asimage2drawable( Scr.asv, bh->pmap, new_im, Scr.DrawGC, 0, 0, 0, 0, new_im->width, new_im->height, True) )
				show_warning( "failed to draw root background onto pixmap");
        	flush_asimage_cache(new_im);
        	XSetWindowBackgroundPixmap( dpy, Scr.Root, bh->pmap );
        	XClearWindow( dpy, Scr.Root );
        	set_xrootpmap_id (Scr.wmprops, bh->pmap );
		}  
		if( Scr.RootImage == NULL )
			Scr.RootImage = dup_asimage(Scr.RootBackground->im) ;
    }else
        set_xrootpmap_id (Scr.wmprops, None );
    ASSync(False);
    print_asimage_registry();
    LOCAL_DEBUG_OUT("done%s","" );
    remove_desktop_cover();
}

/*************************************************************************
 * Client access to the root background for different desktops :
 *
 * Client should send the ClientMessage to the Root window
 *      window        = client's window
 *      message_type  = _AS_BACKGROUND
 *      format        = 16
 *          data.s[0]   = desktop number
 *          data.s[1]   = clip_x
 *          data.s[2]   = clip_y
 *          data.s[3]   = clip_width
 *          data.s[4]   = clip_height
 *          data.s[5]   = flags
 *          data.s[6]   = red tint
 *          data.s[7]   = green tint
 *          data.s[8]   = blue tint
 * When target Pixmap has been populated with required background
 * AfterStep will send the ClientMessage event back to client's window.
 * Contents of the message should be as follows :
 *      window        = root window
 *      message_type  = _AS_BACKGROUND
 *      format        = 32
 *          data.l[0]   = desktop number
 *          data.l[1]   = result pixmap
 *
 * In case AfterStep fails to generate required image it will still send the
 * ClientMessage with all the data set to 0
 *
 * It is the responcibility of the client to create and destroy the
 * target Pixmap.
 *************************************************************************/
void
SendBackgroundReplay( ASEvent *src, Pixmap pmap )
{
    XClientMessageEvent *xcli = &(src->x.xclient) ;
    Window w = xcli->window;
    int desk = xcli->data.s[0];

    xcli->data.l[0] = desk ;
    xcli->data.l[1] = pmap ;
    xcli->data.l[2] = 0 ;
    xcli->data.l[3] = 0 ;
    xcli->data.l[4] = 0 ;
    xcli->window = Scr.Root ;
    xcli->format = 32 ;
    XSendEvent( dpy, w, False, 0, &(src->x) );
}

void
HandleBackgroundRequest( ASEvent *event )
{
    XClientMessageEvent *xcli = &(event->x.xclient) ;
    Pixmap  p           = None ;
    int     desk        = xcli->data.s[0] ;
    int     clip_x      = xcli->data.s[1] ;
    int     clip_y      = xcli->data.s[2] ;
    int     clip_width  = xcli->data.s[3] ;
    int     clip_height = xcli->data.s[4] ;
    Bool    flags       = xcli->data.s[5] ;
    ARGB32  tint        = MAKE_ARGB32(0,xcli->data.s[6],xcli->data.s[7],xcli->data.s[8]) ;
    ASImage *im ;
    MyBackground *back = get_desk_back_or_default( desk, False );
    Bool res = False;
    Bool do_tint = True ;

    if( xcli->data.s[6] == xcli->data.s[7] == xcli->data.s[8] )
        do_tint = !(xcli->data.s[6] == 0 || xcli->data.s[6] == 0x7F );

    LOCAL_DEBUG_OUT("pmap(%lX/%lu)->clip_pos(%+d%+d)->clip_size(%dx%d)->scale(%d)->tint(%lX)->window(%lX)", p, p, clip_x, clip_y, clip_width, clip_height, flags, tint, event->x.xclient.window );

    if( clip_width > 0 && clip_height > 0 && back != NULL && back->type != MB_BackCmd )
    {
        if( (im = make_desktop_image( desk, back )) != NULL )
        {
            p = create_visual_pixmap( Scr.asv, Scr.Root, clip_width, clip_height, 0 );
            if( p )
            {
                ASImage *tmp_im ;
                int tmp_w = clip_width ;
                int tmp_h = clip_height ;
                if( get_flags(flags, 0x01) )
                {
                    if( clip_width < im->width )
                        tmp_w = im->width ;
                    if( clip_height < im->height )
                        tmp_h = im->height ;
                }
                if( get_flags(flags, 0x02) )
                {
                    if( clip_width > im->width )
                        tmp_w = im->width ;
                    if( clip_height > im->height )
                        tmp_h = im->height ;
                }

                if( clip_x != 0 || clip_y != 0 || tmp_w != im->width || tmp_h != im->height || do_tint )
                {
                    tmp_im = tile_asimage( Scr.asv, im, clip_x, clip_y, tmp_w, tmp_h, tint, (tmp_w != im->width || tmp_h != im->height)?ASA_ASImage:ASA_XImage, ASIMAGE_QUALITY_DEFAULT, 0 );
                    if( tmp_im != NULL )
                    {
                        safe_asimage_destroy( im );
                        im = tmp_im ;
                    }
                }
                if( im->width != clip_width || im->height != clip_height )
                {
                    tmp_im = scale_asimage( Scr.asv, im, clip_width, clip_height, ASA_XImage, ASIMAGE_QUALITY_DEFAULT, 0 );
                    if( tmp_im != NULL )
                    {
                        safe_asimage_destroy( im );
                        im = tmp_im ;
                    }
                }
                if( asimage2drawable( Scr.asv, p, im, Scr.DrawGC, 0, 0, 0, 0, clip_width, clip_height, True) )
                    res = True ;
                else
                {
                    XFreePixmap( dpy, p );
                    p = None ;
                }
            }
            safe_asimage_destroy( im );
        }
    }
    SendBackgroundReplay( event, p );
}

