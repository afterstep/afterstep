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

#define LOCAL_DEBUG
#include "../../configure.h"

#include "../../include/asapp.h"
#include "../../libAfterImage/afterimage.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/event.h"
#include "../../include/decor.h"
#include "../../include/mystyle.h"
#include "../../include/wmprops.h"
#include "../../include/session.h"

#include "asinternals.h"

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
        while (total < Scr.Feel.EdgeResistanceScroll)
		{
            register int i ;
			sleep_a_little (10000);
			total += 10;
            for( i = 0 ; i < PAN_FRAME_SIDES ; i++ )
                if( Scr.PanFrame[i].isMapped )
                    if (ASCheckWindowEvent (Scr.PanFrame[i].win, LeaveWindowMask, &(event->x)))
                        return;
        }

        XQueryPointer (dpy, Scr.Root, &wdumm, &wdumm, &x, &y, &dumm, &dumm, &udumm);

		/* fprintf (stderr, "-------- MoveOutline () called from pager.c\ntmp_win == 0xlX\n", (long int) tmp_win); */
		/* Turn off the rubberband if its on */
//        MoveOutline ( /*Scr.Root, */ tmp_win, 0, 0, 0, 0);

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
				XGrabServer (dpy);
			XWarpPointer (dpy, None, Scr.Root, 0, 0, 0, 0, *xl, *yt);
			MoveViewport (Scr.Vx + *delta_x, Scr.Vy + *delta_y, False);
            XQueryPointer (dpy, Scr.Root, &wdumm, &wdumm, xl, yt, &dumm, &dumm, &udumm);
			if (Grab)
				XUngrabServer (dpy);
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
        on_window_status_changed( asw, True, True );
    }
    return True;
}


void
MoveViewport (int newx, int newy, Bool grab)
{
#ifndef NO_VIRTUAL
	int           deltax, deltay;

LOCAL_DEBUG_CALLER_OUT( "new(%+d%+d), old(%+d%+d), max(%+d,%+d)", newx, newy, Scr.Vx, Scr.Vy, Scr.VxMax, Scr.VyMax );

	if (grab)
		XGrabServer (dpy);

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

	if (deltax || deltay)
	{

        /* traverse window list and redo the titlebar/buttons if necessary */
        iterate_asbidirlist( Scr.Windows->clients, viewport_aswindow_iter_func, NULL, NULL, False );

#if 0                                          /* old cruft : */
        /* Here's an attempt at optimization by reducing (hopefully) the expose
		 * events sent to moved windows.  Move the windows which will be on the
		 * new desk, from the front window to the back one.  Move the other
		 * windows from the back one to the front.  Thus if a window is totally
		 * (or partially) obscured, it will not be uncovered if possible. */

		/* do the windows which will be on the new desk first */
		for (t = Scr.ASRoot.next; t != NULL; t = t->next)
		{
			t->flags &= ~PASS_1;

			/* don't move sticky windows */
			if (!(t->flags & STICKY))
			{
				int           x, y, w, h;

				get_window_geometry (t, t->flags, &x, &y, &w, &h);
				w += 2 * t->bw;
				h += 2 * t->bw;
				/* do the window now if it would be onscreen after moving */
				if (x + deltax < Scr.MyDisplayWidth && x + deltax + w > 0 &&
					y + deltax < Scr.MyDisplayHeight && y + deltay + h > 0)
				{
					t->flags |= PASS_1;
					SetupFrame (t, t->frame_x + deltax, t->frame_y + deltay,
								t->frame_width, t->frame_height, FALSE);
				}
				/* if StickyIcons is set, treat the icon as sticky */
				if (!(Scr.flags & StickyIcons) &&
					x + deltax < Scr.MyDisplayWidth && x + deltax + w > 0 &&
					y + deltax < Scr.MyDisplayHeight && y + deltay + h > 0)
				{
					t->flags |= PASS_1;
					t->icon_p_x += deltax;
					t->icon_p_y += deltay;
					if (t->flags & ICONIFIED)
					{
						if (t->icon_pixmap_w != None)
							XMoveWindow (dpy, t->icon_pixmap_w, t->icon_p_x, t->icon_p_y);
						if (t->icon_title_w != None)
							XMoveWindow (dpy, t->icon_title_w, t->icon_p_x,
										 t->icon_p_y + t->icon_p_height);
						if (!(t->flags & ICON_UNMAPPED))
							Broadcast (M_ICON_LOCATION, 7, t->w, t->frame,
									   (unsigned long)t,
									   t->icon_p_x, t->icon_p_y, t->icon_p_width, t->icon_p_height);
					}
				}
			}
		}
		/* now do the other windows, back to front */
		for (t = &Scr.ASRoot; t->next != NULL; t = t->next);
		for (; t != &Scr.ASRoot; t = t->prev)
			if (!(t->flags & PASS_1))
			{
				/* don't move sticky windows */
				if (!(t->flags & STICKY))
				{
					SetupFrame (t, t->frame_x + deltax, t->frame_y + deltay,
								t->frame_width, t->frame_height, FALSE);
					/* if StickyIcons is set, treat the icon as sticky */
					if (!(Scr.flags & StickyIcons))
					{
						t->icon_p_x += deltax;
						t->icon_p_y += deltay;
						if (t->flags & ICONIFIED)
						{
							if (t->icon_pixmap_w != None)
								XMoveWindow (dpy, t->icon_pixmap_w, t->icon_p_x, t->icon_p_y);
							if (t->icon_title_w != None)
								XMoveWindow (dpy, t->icon_title_w, t->icon_p_x,
											 t->icon_p_y + t->icon_p_height);
							if (!(t->flags & ICON_UNMAPPED))
								Broadcast (M_ICON_LOCATION, 7, t->w, t->frame,
										   (unsigned long)t,
										   t->icon_p_x, t->icon_p_y,
										   t->icon_p_width, t->icon_p_height);
						}
					}
				}
            }
#endif
        /* TODO: autoplace sticky icons so they don't wind up over a stationary icon */
    }
    check_screen_panframes(&Scr);
	if (grab)
		XUngrabServer (dpy);
#endif
}

/**************************************************************************
 * Move to a new desktop
 *************************************************************************/
Bool
change_aswindow_desk_iter_func(void *data, void *aux_data)
{
    int new_desk = (int)aux_data ;
    ASWindow *asw = (ASWindow *)data ;

    if( ASWIN_GET_FLAGS(asw, AS_Sticky) ||
        (ASWIN_GET_FLAGS(asw, AS_Iconic) && get_flags( Scr.Feel.flags, StickyIcons)) )
    {  /* Window is sticky */
        ASWIN_DESK(asw) = new_desk ;
        set_client_desktop( asw->w, new_desk );
    }else
    {
        Window dst = Scr.ServiceWin;
//        Bool use_root_pos = get_flags( AfterStepState, ASS_NormalOperation);
        if(ASWIN_DESK(asw)==new_desk)
            dst = Scr.Root;
        quietly_reparent_canvas( asw->frame_canvas, dst, AS_FRAME_EVENT_MASK, True );
        quietly_reparent_canvas( asw->icon_canvas, dst, AS_ICON_EVENT_MASK, True );
        if( asw->icon_title_canvas != asw->icon_canvas )
            quietly_reparent_canvas( asw->icon_title_canvas, dst, AS_ICON_TITLE_EVENT_MASK, True );
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
ChangeDesks (int new_desk)
{
    int old_desk = Scr.CurrentDesk ;
LOCAL_DEBUG_CALLER_OUT( "new_desk(%d)->old_desk(%d)", new_desk, old_desk );
    if( Scr.CurrentDesk == new_desk )
        return;

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

    SendPacket( -1, M_NEW_DESK, 1, new_desk);
    /* Scan the window list, mapping windows on the new Desk, unmapping
	 * windows on the old Desk; do this in reverse order to reduce client
	 * expose events */
	XGrabServer (dpy);
    iterate_asbidirlist( Scr.Windows->clients, change_aswindow_desk_iter_func, (void*)new_desk, NULL, False );
    XUngrabServer (dpy);

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
                        focus_aswindow( circ_list[i], False );
                        break;
                    }
        }else
            i = circ_count ;

        if( i >= circ_count )
            hide_focus();
    }

    restack_window_list( new_desk, False );

    /* Change the look to this desktop's one if it really changed */
#ifdef DIFFERENTLOOKNFEELFOREACHDESKTOP
	QuickRestart ("look&feel");
#endif

    /* we need to set the desktop background : */
    change_desktop_background(new_desk, old_desk);

    /*TODO: implement virtual desktops switching : */
#if 0
    /* autoplace sticky icons so they don't wind up over a stationary icon */
	AutoPlaceStickyIcons ();


	CorrectStackOrder ();
	update_windowList ();
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

ASImage *
load_myback_image( int desk, MyBackground *back )
{
    ASImage *im = NULL ;
    if( back->data && back->data[0] )
        im = get_asimage( Scr.image_manager, back->data, 0xFFFFFFFF, 100 );

    if( im == NULL )
    {
        const char *const_configfile = get_session_file (Session, desk, F_CHANGE_BACKGROUND);
        if( const_configfile != NULL )
        {
            im = get_asimage( Scr.image_manager, const_configfile, 0xFFFFFFFF, 100 );
            show_progress("BACKGROUND for desktop %d loaded from \"%s\" ...", desk, const_configfile);
        }else
            show_progress("BACKGROUND file cannot be found for desktop %d", desk );
    }
    if( im != NULL )
    {
        /* crop and or tint */
        if( back->cut.flags != 0 || (back->tint != TINT_NONE && back->tint != TINT_LEAVE_SAME))
        {

        }
        /* scale */
        if( get_flags(back->scale.flags, (WidthValue|HeightValue)) )
        {
            ASImage *tmp_im ;
            int width = im->width ;
            int height = im->height ;
            if( get_flags(back->scale.flags, WidthValue) )
                width = back->scale.width;
            if( get_flags(back->scale.flags, HeightValue) )
                height = back->scale.height;
            if( width != im->width || height != im->height )
            {
                tmp_im = scale_asimage( Scr.asv, im, width, height, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
                if( tmp_im )
                {
                    safe_asimage_destroy( im );
                    im = tmp_im ;
                }
            }
        }
        /* pad */
        if( back->align_flags != NO_ALIGN && (im->width != Scr.MyDisplayWidth || im->height != Scr.MyDisplayHeight))
        {
            int x = 0, y = 0;
            ASImage *tmp_im ;
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

            tmp_im = pad_asimage( Scr.asv, im, x, y, Scr.MyDisplayWidth, Scr.MyDisplayHeight, back->pad_color,
                                           ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
            if( tmp_im )
            {
                safe_asimage_destroy( im );
                im = tmp_im ;
            }
        }
    }
    return im;
}

void release_old_background( int desk )
{
    MyBackground *back = mylook_get_desk_back( &(Scr.Look), desk );
    ASImage *im = NULL ;
    char *imname ;

    if( back == NULL || back->type == MB_BackCmd )
        return ;

    imname = make_myback_image_name( &(Scr.Look), back->name );
    im = query_asimage( Scr.image_manager, imname );
    free( imname );

    if( im == NULL && back->type == MB_BackImage )
    {
        if( back->data && back->data[0] )
            im = query_asimage( Scr.image_manager, back->data );
        if( im == NULL )
        {
            const char *const_configfile = get_session_file (Session, desk, F_CHANGE_BACKGROUND);
            if( const_configfile != NULL )
                im = query_asimage( Scr.image_manager, const_configfile );
        }
    }

    if( im != NULL && im->width*im->height >= Scr.Look.KillBackgroundThreshold )
        safe_asimage_destroy( im );
}

void
change_desktop_background( int desk, int old_desk )
{
    MyBackground *new_back = mylook_get_desk_back( &(Scr.Look), desk );
    MyBackground *old_back = mylook_get_desk_back( &(Scr.Look), old_desk );
    char *new_imname ;
    ASImage *new_im = NULL ;
LOCAL_DEBUG_CALLER_OUT( "desk(%d)->old_desk(%d)->new_back(%p)->old_back(%p)", desk, old_desk, new_back, old_back );
    if( new_back == NULL )
        return ;

    if( new_back == old_back &&
        desk != old_desk ) /* if desks are the same then we are reloading current background !!! */
        return;

    release_old_background( old_desk );

    if( Scr.RootBackground == NULL )
        Scr.RootBackground = safecalloc( 1, sizeof(ASBackgroundHandler));
    else
    {                                          /* do cleanup - kill command maybe */
        if( Scr.RootBackground->cmd_pid )
            check_singleton_child (BACKGROUND_SINGLETON_ID, True);
        Scr.RootBackground->cmd_pid = 0;
        Scr.RootBackground->im = NULL ;
    }

    new_imname = make_myback_image_name( &(Scr.Look), new_back->name );
    if( new_back->type == MB_BackImage )
    {
        if( (new_im = fetch_asimage( Scr.image_manager, new_imname )) == NULL )
        {
            if( (new_im = load_myback_image( desk, new_back )) != NULL )
                if( new_im->name == NULL )
                    store_asimage( Scr.image_manager, new_im, new_imname );
        }
    }if( new_back->type == MB_BackMyStyle )
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
        new_im = mystyle_make_image( style, 0, 0, root_width, root_height );

        if( style->texture_type >= TEXTURE_TRANSPARENT )
        {
            destroy_asimage( &(Scr.RootImage) );
            Scr.RootImage = old_root;
        }
    }else
    {                                          /* run command */
        Scr.RootBackground->cmd_pid = spawn_child( XIMAGELOADER, BACKGROUND_SINGLETON_ID, Scr.screen, None, C_NO_CONTEXT, True, False, new_back->data, NULL );
    }
    LOCAL_DEBUG_OUT( "im(%p)", new_im );
    if( new_im )
    {
        ASBackgroundHandler *bh = Scr.RootBackground ;
        /* Scr.RootImage = new_im ; */         /* will be done in event handler !!! */
        if( new_im->name == NULL )
            store_asimage( Scr.image_manager, new_im, new_imname );
        if( bh->pmap && (new_im->width != bh->pmap_width ||
            new_im->height != bh->pmap_height) )
        {
            XFreePixmap( dpy, bh->pmap );
            bh->pmap = None ;
        }
        if( bh->pmap == None )
            bh->pmap = create_visual_pixmap( Scr.asv, Scr.Root, new_im->width, new_im->height, 0 );

        bh->pmap_width = new_im->width ;
        bh->pmap_height = new_im->height ;
        bh->im = new_im;

        LOCAL_DEBUG_OUT( "width(%d)->height(%d)", new_im->width, new_im->height );

        asimage2drawable( Scr.asv, bh->pmap, new_im, NULL, 0, 0, 0, 0, new_im->width, new_im->height, True);
        XSetWindowBackgroundPixmap( dpy, Scr.Root, bh->pmap );
        XClearWindow( dpy, Scr.Root );
        set_xrootpmap_id (Scr.wmprops, bh->pmap );
    }else
        set_xrootpmap_id (Scr.wmprops, None );
}
