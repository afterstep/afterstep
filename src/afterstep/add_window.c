/*
 * Copyright (c) 2002 Sasha Vasko <sasha@aftercode.net>
 * Copyright (c) 1998 Makoto Kato <m_kato@ga2.so-net.ne.jp>
 * Copyright (C) 1997 Dong-hwa Oh <siage@nownuri.net>
 * Copyright (C) 1997 Tomonori <manome@itlb.te.noda.sut.ac.jp>
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

/****************************************************************************
 * This module is based on Twm, but has been SIGNIFICANTLY modified
 * by Rob Nation
 * by Bo Yang
 * by Dong-hwa Oh <siage@nownuri.net>          ) korean support patch/
 * by Tomonori <manome@itlb.te.noda.sut.ac.jp> )
 ****************************************************************************/

/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/


/**********************************************************************
 *
 * Add a new window, put the titlbar and other stuff around
 * the window
 *
 **********************************************************************/
#include "../../configure.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/decor.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/parser.h"
#include "../../include/clientprops.h"
#include "../../include/hints.h"

#include "menus.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#include <X11/Xresource.h>
#endif /* SHAPE */

extern int    LastWarpIndex;
char          NoName[] = "Untitled";		   /* name if no name is specified */


ASWindow *window2ASWindow( Window w )
{
    ASWindow *asw = NULL ;
    if( Scr.aswindow_xref )
        if( get_hash_item( Scr.aswindow_xref, AS_HASHABLE(w), (void**)&asw ) == ASH_Success )
            return asw;
    return asw;
}

Bool register_aswindow( Window w, ASWindow *asw )
{
    if( w && asw )
    {
        if( Scr.aswindow_xref == NULL )
            Scr.aswindow_xref = create_ashash( 0, NULL, NULL, NULL );

        if( add_hash_item( Scr.aswindow_xref, AS_HASHABLE(w), asw ) == ASH_Success )
            return True;
    }
    return False;
}

Bool unregister_aswindow( Window w )
{
    if( w )
    {
        if( Scr.aswindow_xref != NULL )
		{
	        if( remove_hash_item( Scr.aswindow_xref, AS_HASHABLE(w), NULL ) == ASH_Success )
  		        return True;
		}
    }
    return False;
}

Bool destroy_registered_window( Window w )
{
	Bool res = False ;
    if( w )
    {
        if( Scr.aswindow_xref != NULL )
	        res = ( remove_hash_item( Scr.aswindow_xref, AS_HASHABLE(w), NULL ) == ASH_Success )
		XDestroyWindow( dpy, w );
    }
    return res;
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

#if 0
/************************************************************************/
/* artifacts of ASWindow icon handling - assimilate in other functions: */
/************************************************************************/
void
Create_icon_windows (ASWindow *asw)
{
	unsigned long valuemask;				   /* mask for create windows */
	XSetWindowAttributes attributes;		   /* attributes for create windows */

	asw->flags &= ~(ICON_OURS | XPM_FLAG | PIXMAP_OURS | SHAPED_ICON);

	if ( !ASWIN_HFLAGS( asw, AS_Icon ) )
		return;
	/* First, see if it was specified in config. files  */
	if ( !ASWIN_HFLAGS( asw, AS_ClientIcon ) )
    GetIcon (tmp_win);
	ResizeIconWindow (tmp_win);

	if (tmp_win->icon_title_w != None)
	{
        register_aswindow( tmp_win->icon_title_w, tmp_win );
		XDefineCursor (dpy, tmp_win->icon_title_w, Scr.ASCursors[DEFAULT]);
		GrabIconButtons (tmp_win, tmp_win->icon_title_w);
		GrabIconKeys (tmp_win, tmp_win->icon_title_w);
	}

	if (tmp_win->icon_pixmap_w != None)
	{
        register_aswindow( tmp_win->icon_pixmap_w, tmp_win );
		XDefineCursor (dpy, tmp_win->icon_pixmap_w, Scr.ASCursors[DEFAULT]);
		GrabIconButtons (tmp_win, tmp_win->icon_pixmap_w);
		GrabIconKeys (tmp_win, tmp_win->icon_pixmap_w);
	}

#ifdef SHAPE
	UpdateIconShape (tmp_win);
#endif /* SHAPE */

}
#endif

/********************************************************************/
/* ASWindow frame decorations :                                     */
/********************************************************************/
/* window frame decoration consists of :
  Top level window
	  4 canvases - one for each side :
	  	  Top or left canvas contains titlebar+ adjusen frame side+corners if any
		  Bottom or right canvas contains sidebar which is the same as south frame side with corners
		  Remaining two canvasses contain east and west frame sides only ( if any );
	  Canvasses surround main window and its sizes are actually the frame size.
 */

/* this gets called when Look changes or hints changes : */
static ASCanvas *
check_side_canvas( ASWindow *asw, FrameSide side, Bool required )
{
    ASCanvas *canvas = asw->frame_sides[side];
	Window w;

    if( required )
    {
        if( canvas == NULL )
        {                                      /* create canvas here */
			unsigned long valuemask;
			XSetWindowAttributes attributes;

			valuemask = CWBorderPixel | CWEventMask;
			attributes.border_pixel = Scr.asv->black_pixel;
			if (Scr.flags & BackingStore)
			{
				valuemask |= CWBackingStore;
				attributes.backing_store = WhenMapped;
			}
			attributes.event_mask = AS_CANVAS_EVENT_MASK;

			w = create_visual_window (Scr.asv, asw->frame,
									  0, 0, 1, 1, 0, InputOutput,
									  valuemask, &attributes);
            register_aswindow( w, asw );
			canvas = create_ascanvas( w );
        }
    }else if( canvas != NULL )
    {                                          /* destroy canvas here */
		w = canvas->w ;
        destroy_ascanvas( &canvas );
        destroy_registered_window( w );
    }

    return (asw->frame_sides[side] = canvas);
}

/* creating/destroying our main frame window : */
static ASCanvas *
check_frame_canvas( ASWindow *asw, Bool required )
{
    ASCanvas *canvas = asw->frame_canvas;
	Window w;

    if( required )
    {
        if( canvas == NULL )
        {   /* create canvas here */
			unsigned long valuemask;
			XSetWindowAttributes attributes;

            /* create windows */
            valuemask = CWBorderPixel | CWCursor | CWEventMask ;
            if( Scr.asv->visual_info->visual == DefaultVisual( dpy, Scr.screen ) )
            {/* only if root has same depth and visual as us! */
                attributes.background_pixmap = ParentRelative;
                valuemask |= CWBackPixmap;
            }
            attributes.border_pixel = Scr.asv->black_pixel;
            attributes.cursor = Scr.ASCursors[DEFAULT];
            attributes.event_mask = AS_FRAME_EVENT_MASK;

            if (Scr.flags & SaveUnders)
            {
                valuemask |= CWSaveUnder;
                attributes.save_under = TRUE;
            }
            w = create_visual_window (Scr.asv, Scr.Root, -10, -10, 5, 5,
                                      asw->bw, InputOutput, valuemask, &attributes);
            asw->frame = w ;
            register_aswindow( w, asw );
            canvas = create_ascanvas_container( w );

        }
    }else if( canvas != NULL )
    {                                          /* destroy canvas here */
        w = canvas->w ;
        destroy_ascanvas( &canvas );
        destroy_registered_window( w );
    }

    return (asw->frame_canvas = canvas);
}

/* creating/destroying container canvas for our client's window : */
static ASCanvas *
check_client_canvas( ASWindow *asw, Bool required )
{
    ASCanvas *canvas = asw->client_canvas;
	Window w;

    if( required )
    {
        if( canvas == NULL )
        {                                      /* create canvas here */
			unsigned long valuemask;
			XSetWindowAttributes attributes;

            if( asw->frame == None || (w = asw->w) == None )
                return NULL ;

            XReparentWindow (dpy, w, asw->frame, 0, 0);

            valuemask = (CWEventMask | CWDontPropagate);
            attributes.event_mask = AS_CLIENT_EVENT_MASK;
            attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;
            if (Scr.flags & AppsBackingStore)
            {
                valuemask |= CWBackingStore;
                attributes.backing_store = WhenMapped;
            }
            XChangeWindowAttributes (dpy, w, valuemask, &attributes);
            XAddToSaveSet (dpy, w);

            register_aswindow( w, asw );
            canvas = create_ascanvas_container( w );
        }
    }else if( canvas != NULL )
    {                                          /* destroy canvas here */
        XWindowChanges xwc;                    /* our withdrawn geometry */
        ASStatusHints withdrawn_status = {0} ;
        register int i = 0 ;

        w = canvas->w ;

        /* calculating the withdrawn location */
        if( asw->status )
            withdrawn_status = *(asw->status) ;

        xwc.border_width = withdrawn_status.border_width ;

        for( i = 0 ; i < FRAME_SIDES ; ++i )
            withdrawn_state.frame_size[i] = 0 ;
        clear_flags( withdrawn_state.flags, AS_Shaded|AS_Iconic );
        anchor2status( &withdrawn_status, asw->hints, &(asw->anchor));
        xwc.x = withdrawn_status.x ;
        xwc.y = withdrawn_status.y ;
        xwc.width = withdrawn_status.width ;
        xwc.height = withdrawn_status.height ;

        destroy_ascanvas( &canvas );
        unregister_window( w );

        /*
         * Prevent the receipt of an UnmapNotify in case we are simply restarting,
         * since that would cause a transition to the Withdrawn state.
		 */
        if( get_flags( Scr.state, AS_StateShutdown ) )
            quietly_reparent_window( w, Scr.Root, xwc.x, xwc.y, AS_CLIENT_EVENT_MASK );
        else
            XReparentWindow (dpy, w, Scr.Root, xwc.x, xwc.y);

        /* WE have to restore window's withdrawn location now. */
        XConfigureWindow (dpy, w, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &xwc);
		XSync (dpy, 0);
    }

    return (asw->client_canvas = canvas);
}

/* creating/destroying our icon window : */
static ASCanvas *
check_icon_canvas( ASWindow *asw, Bool required )
{
    ASCanvas *canvas = asw->icon_canvas;
	Window w;

    if( required )
    {
        if( canvas == NULL )
        {                                      /* create canvas here */
			unsigned long valuemask;
			XSetWindowAttributes attributes;

            valuemask = CWBorderPixel | CWCursor | CWEventMask ;
            attributes.border_pixel = Scr.asv->black_pixel;
            attributes.cursor = Scr.ASCursors[DEFAULT];

            if ((ASWIN_HFLAGS(asw, AS_ClientIcon|AS_ClientIconPixmap) != AS_ClientIcon) ||
                 asw->hints == NULL || asw->hints->icon.window == None ||
				 !get_flags(Scr.flags, KeepIconWindows))
            { /* create windows */
                attributes.event_mask = AS_ICON_TITLE_EVENT_MASK;
                w = create_visual_window ( Scr.asv, Scr.Root, -10, -10, 1, 1, 0,
                                           InputOutput, valuemask, &attributes );
                canvas = create_container( w );
            }else
            { /* reuse client's provided window */
                attributes.event_mask = AS_ICON_EVENT_MASK;
                w = asw->hints->icon.window ;
                XChangeWindowAttributes (dpy, w, valuemask, &attributes);
                canvas = create_ascanvas_container( w );
            }
            register_aswindow( w, asw );
        }
    }else if( canvas != NULL )
    {                                          /* destroy canvas here */
        w = canvas->w ;
        destroy_ascanvas( &canvas );
        if( asw->hints && asw->hints->icon.window == w )
            unregister_window( w );
        else
            destroy_registered_window( w );
    }

    return (asw->icon_canvas = canvas);
}

/* creating/destroying our icon title window : */
static ASCanvas *
check_icon_title_canvas( ASWindow *asw, Bool required, Bool reuse_icon_canvas )
{
    ASCanvas *canvas = asw->icon_title_canvas;
	Window w;

    if( (canvas && reuse_icon_canvas && canvas != asw->icon_canvas) ||
        !required )
    {
        w = canvas->w ;
        destroy_ascanvas( &canvas );
        destroy_registered_window( w );
    }

    if( required )
    {
        if( reuse_icon_canvas )
            canvas = asw->icon_canvas )
        else if( canvas == NULL )
        {                                      /* create canvas here */
			unsigned long valuemask;
			XSetWindowAttributes attributes;

            valuemask = CWBorderPixel | CWCursor | CWEventMask ;
            attributes.border_pixel = Scr.asv->black_pixel;
            attributes.cursor = Scr.ASCursors[DEFAULT];
            attributes.event_mask = AS_ICON_TITLE_EVENT_MASK;
            /* create windows */
            w = create_visual_window ( Scr.asv, Scr.Root, -10, -10, 1, 1, 0,
                                       InputOutput, valuemask, &attributes );

            register_aswindow( w, asw );
            canvas = create_ascanvas( w );
        }
    }

    return (asw->icon_title_canvas = canvas);
}

static ASImage*
get_window_icon_image( ASWindow *asw )
{
	ASImage *im = NULL ;
	if( ASWIN_HFLAGS( asw, AS_ClientIcon|AS_ClientIconPixmap ) == AS_ClientIcon|AS_ClientIconPixmap &&
	    get_flags( Scr.flags, KeepIconWindows )&&
		asw->hints->icon.pixmap != None ) 
	{/* convert client's icon into ASImage */
		unsigned int width, height ;
		get_drawable_size( asw->hints->icon.pixmap, &width, &height );
		im = picture2asimage( Scr.asv, asw->hints->icon.pixmap, 
		                               asw->hints->icon_mask, 
									   0, 0, width, height, 
									   0xFFFFFFFF, False, 100 );
	}
	/* TODO: we also need to check for newfashioned ARGB icon from 
	 * extended WM specs here 
	 */
	if( im == NULL && asw->hints->icon_file )
		im = get_asimage( Scr.image_manager, asw->hints->icon_file, 0xFFFFFFFF, 100 );
	
	return im;
}

static ASTBarData*
check_tbar( ASTBarData **tbar, Bool required, const char *mystyle_name, ASImage *img, unsigned short back_w, unsigned short back_h )
{
    if( required )
    {
        if( *tbar == NULL )
        {
            *tbar = create_astbar();
        }
        set_astbar_style( *tbar, BAR_STATE_FOCUSED, mystyle_name );
        set_astbar_image( *tbar, img );
        set_astbar_back_size( *tbar, back_w, back_h );
        set_astbar_size( *tbar, (back_w == 0)?1:back_w, (back_h == 0)?1:back_h );
    }else if( *tbar )
    {
        destroy_astbar( tbar );
    }
    return *tbar;;
}

/******************************************************************************/
/* no wexternally available interfaces to the above functions :               */
/******************************************************************************/
void
invalidate_window_icon( ASWindow *asw )
{
    if( asw )
        check_icon_canvas( asw, False );
}

/* this should set up proper feel settings - grab keyboard and mouse buttons : 
 * Note: must be called after redecorate_window, since some of windows may have 
 * been created at that time.
 */
void
grab_window_input( ASWindow *asw, Bool release_grab )
{
	/**** 1) frame window grabs : 			****/
	/**** 2) client window grabs : 			****/
	/**** 3) icon window grabs : 			****/
	/**** 4) icon title window grabs : 		****/
	/**** 5) frame decor windows grabs :	****/
}

void
redecorate_window( ASWindow *asw, Bool free_resources )
{
    MyFrame *frame = NULL ;
    ASCanvas *tbar_canvas = NULL, *sidebar_canvas = NULL ;
    ASCanvas *left_canvas = NULL, *right_canvas = NULL ;
    Bool has_tbar = False ;
	int i ;
    char *mystyle_name = Scr.MSFWindow?Scr.MSFWindow->name:NULL;
	ASImage *icon_image = NULL ;

    if( AS_ASSERT(asw) )
        return ;

    if( !free_resources && asw->hints )
    {
        if( ASWIN_HFLAGS(asw, AS_Handles) )
            frame = myframe_find( ASWIN_HFLAGS(asw, AS_Frame)?asw->hints->frame_name:NULL );
        has_tbar = (ASWIN_HFLAGS(asw, AS_Titlebar)!= 0);
    }
    if(  free_resources || asw->hints == NULL || asw->status == NULL )
    {/* destroy window decorations here : */
     /* destruction goes in reverese order ! */
	    check_tbar( &(asw->icon_title), False, NULL, NULL, 0, 0 );
	    check_tbar( &(asw->icon_button), False, NULL, NULL, 0, 0 );
        check_tbar( &(asw->tbar), False, NULL, NULL, 0, 0 );
		i = FRAME_PARTS ; 
		while( --i >= 0 )
            check_tbar( &(asw->frame_bars[i]), False, NULL, NULL, 0, 0 );
	
        check_side_canvas( asw, FR_W, False );
        check_side_canvas( asw, FR_E, False );
        check_side_canvas( asw, FR_S, False );
        check_side_canvas( asw, FR_N, False );

        check_icon_title_canvas( asw, False );
        check_icon_canvas( asw, False );
        check_client_canvas( asw, False );
        check_frame_canvas( asw, False );

        return ;
    }

    if( asw->hints->mystyle_names[BACK_FOCUSED] )
        mystyle_name = asw->hints->mystyle_names[BACK_FOCUSED];
    /* 1) we need to create our frame window : */
    if( check_frame_canvas( asw, True ) == NULL )
        return;
    /* 2) we need to reparent our title window : */
    if( check_client_canvas( asw, True ) == NULL )
        return;
    /* 3) we need to prepare icon window : */
    check_icon_canvas( asw, (ASWIN_HFLAGS( asw, AS_Icon) && !get_flags(Scr.flags, SuppressIcons)) );
    /* 4) we need to prepare icon title window : */
    check_icon_title_canvas( asw, (ASWIN_HFLAGS( asw, AS_IconTitle) &&
                                  get_flags(Scr.flags, IconTitle) &&
                                  !get_flags(Scr.flags, SuppressIcons)),
                                  get_flags(Scr.look_flags, SeparateButtonTitle) );
    /* 5) we need to prepare windows for 4 frame decoration sides : */
    if( ASWIN_HFLAGS(asw, AS_VerticalTitle) )
    {
        tbar_canvas = check_side_canvas( asw, FR_W, has_tbar||myframe_has_parts(frame, FRAME_TOP_MASK) );
        sidebar_canvas = check_side_canvas( asw, FR_E, myframe_has_parts(frame, FRAME_BTM_MASK) );
        left_canvas = check_side_canvas( asw, FR_S, myframe_has_parts(frame, FRAME_LEFT_MASK) );
        right_canvas = check_side_canvas( asw, FR_N, myframe_has_parts(frame, FRAME_RIGHT_MASK) );
    }else
    {
        tbar_canvas = check_side_canvas( asw, FR_N, has_tbar||myframe_has_parts(frame, FRAME_TOP_MASK) );
        sidebar_canvas = check_side_canvas( asw, FR_S, myframe_has_parts(frame, FRAME_BTM_MASK) );
        left_canvas = check_side_canvas( asw, FR_W, myframe_has_parts(frame, FRAME_LEFT_MASK) );
        right_canvas = check_side_canvas( asw, FR_E, myframe_has_parts(frame, FRAME_RIGHT_MASK) );
    }
    /* 6) now we have to create bar for icon - if it is not client's animated icon */
	if( asw->icon_canvas )
		icon_image = get_window_icon_image( asw );
    check_tbar( &(asw->icon_button), (asw->icon_canvas != NULL), AS_ICON_MYSTYLE,
                icon_image, 0, 0 );/* scaling icon image */
	if( icon_image ) 
        safe_asimage_destroy( icon_image );
    /* 7) now we have to create bar for icon title (optional) */
    check_tbar( &(asw->icon_title), (asw->icon_title_canvas != NULL), AS_ICON_TITLE_MYSTYLE,
                NULL, 0, 0 );
    /* 8) now we have to create actuall bars - for each frame element plus one for the titlebar */
	for( i = 0 ; i < FRAME_PARTS ; ++i )
    {
        unsigned short back_w = 0, back_h = 0 ;
        ASImage *img = NULL ;

        if( frame )
        {
            img = frame->parts[i]?frame->parts[i]->image:NULL ;

            back_w = frame->part_width[i] ;
            back_h = frame->part_length[i] ;
            if( ASWIN_HFLAGS(asw, AS_VerticalTitle) )
            {
                if( !((0x01<<i)&MYFRAME_VERT_MASK) )
                {
                    back_h = frame->part_width[i] ;
                    back_w = frame->part_length[i] ;
                }
            }else if( (0x01<<i)&MYFRAME_HOR_MASK )
            {
                back_h = frame->part_width[i] ;
                back_w = frame->part_length[i] ;
            }
        }
        check_tbar( &(asw->frame_bars[i]), IsFramePart(frame,i), mystyle_name,
                    img, back_w, back_h );
    }
    check_tbar( &(asw->tbar), has_tbar, mystyle_name, NULL, 0, 0 );

    /* 9) now we have to setup titlebar buttons */
    if( asw->tbar )
	{ /* need to add some titlebuttons */
		ASTBtnBlock* btns ;
		/* left buttons : */
		btns = build_tbtn_block( &(Scr.buttons[0]),
		                         ~(asw->hints->disabled_buttons),
		                         TITLE_BUTTONS_PERSIDE, 1,
                                 ASWIN_HFLAGS(asw, AS_VerticalTitle)?
									TBTN_ORDER_B2T:TBTN_ORDER_L2R );
        set_astbar_btns( asw->tbar, &btns, True );
        /* right buttons : */
		btns = build_tbtn_block( &(Scr.buttons[TITLE_BUTTONS_PERSIDE]),
		                         (~(asw->hints->disabled_buttons))>>TITLE_BUTTONS_PERSIDE,
		                         TITLE_BUTTONS_PERSIDE, 1,
                                 ASWIN_HFLAGS(asw, AS_VerticalTitle)?
									TBTN_ORDER_T2B:TBTN_ORDER_R2L );
        set_astbar_btns( asw->tbar, &btns, False );
	}
    /* we also need to setup label, unfocused/sticky style and tbar sizes -
     * it all is done when we change windows state, or move/resize it */
}


/* this gets called when Root background changes : */
void
update_window_transparency( ASWindow *asw )
{
/* TODO: */

}


static void
resize_canvases( ASWindow *asw, ASCanvas *tbar, ASCanvas *sbar, ASCanvas *left, ASCanvas *right, unsigned int width, unsigned int height )
{
    unsigned short tbar_size = 0 ;
    unsigned short sbar_size = 0 ;
    if( ASWIN_HFLAGS(asw, AS_VerticalTitle) )
    { /* west and east canvases are the big ones - resize them first */
        if( tbar )
        {
            tbar_size = tbar->width ;
            moveresize_canvas( tbar, 0, 0, tbar_size, height );
        }
        if( sbar )
        {
            sbar_size = sbar->width ;
            moveresize_canvas( sbar, width-(tbar_size+sbar_size), 0, sbar_size, height );
        }
        if( left )
            moveresize_canvas( left, tbar_size, height-left->height, width-(tbar_size+sbar_size), left->height );
        if( right )
            moveresize_canvas( right, tbar_size, 0, width-(tbar_size+sbar_size), right->height );
    }else
    {
        if( tbar )
        {
            tbar_size = tbar->height ;
            moveresize_canvas( tbar, 0, 0, width, tbar_size );
        }
        if( sbar )
        {
            sbar_size = sbar->width ;
            moveresize_canvas( sbar, 0, height-(tbar_size+sbar_size), width, sbar_size );
        }
        if( left )
            moveresize_canvas( left, 0, tbar_size, left->width, height-(tbar_size+sbar_size));
        if( right )
            moveresize_canvas( right, width-right->width, tbar_size, right->width, height-(tbar_size+sbar_size));
    }
}

static unsigned short
frame_side_height(ASCanvas *c1, ASCanvas *c2 )
{
    unsigned short h = 0 ;
    if( c1 )
        h += c1->height ;
    if( c2 )
        h += c2->height ;
    return h;
}

static unsigned short
frame_side_width(ASCanvas *c1, ASCanvas *c2 )
{
    unsigned short w = 0 ;
    if( c1 )
        w += c1->width ;
    if( c2 )
        w += c2->width ;
    return w;
}

static unsigned short
frame_corner_height(ASTBarData *c1, ASTBarData *c2 )
{
    unsigned short h = 0 ;
    if( c1 )
        h += c1->height ;
    if( c2 )
        h += c2->height ;
    return h;
}

static unsigned short
frame_corner_width(ASTBarData *c1, ASTBarData *c2 )
{
    unsigned short w = 0 ;
    if( c1 )
        w += c1->width ;
    if( c2 )
        w += c2->width ;
    return w;
}

/* this gets called when StructureNotify/SubstractureNotify arrives : */
void
on_window_moveresize( ASWindow *asw, Window w, int x, int y, unsigned int width, unsigned int height )
{
    int i ;

    if( AS_ASSERT(asw) || w == asw->w )
        return ;

    if( w == asw->frame )
    {/* resize canvases here :*/
        if( ASWIN_HFLAGS(asw, AS_VerticalTitle) )
        { /* west and east canvases are the big ones - resize them first */
            resize_canvases( asw, asw->frame_sides[FR_W],
                                  asw->frame_sides[FR_E],
                                  asw->frame_sides[FR_N],
                                  asw->frame_sides[FR_S], width, height );
        }else
            resize_canvases( asw, asw->frame_sides[FR_N],
                                  asw->frame_sides[FR_S],
                                  asw->frame_sides[FR_W],
                                  asw->frame_sides[FR_E], width, height );
    }else
    {
        for( i = 0 ; i < FRAME_SIDES ; ++i )
            if( asw->frame_sides[i] && asw->frame_sides[i]->w == w )
            {   /* canvas has beer resized - resize tbars!!! */
                unsigned short corner_size = 0;
                Bool canvas_moved = handle_canvas_config (asw->frame_sides[i]);

                if( ASWIN_HFLAGS(asw, AS_VerticalTitle) )
                {
                    if( i == FR_W )
                    {
                        if( set_astbar_size( asw->tbar, asw->tbar->width, height ) ||
                            canvas_moved)
                            render_astbar( asw->tbar, asw->frame_sides[i] );
                        corner_size = frame_corner_height(asw->frame_bars[FR_NW], asw->frame_bars[FR_SW]);
                    }else if( i == FR_E )
                        corner_size = frame_corner_height(asw->frame_bars[FR_NE], asw->frame_bars[FR_SE]);
                    if( asw->frame_bars[i] )
                        if( set_astbar_size( asw->frame_bars[i], asw->frame_bars[i]->width, height - corner_size) ||
                            canvas_moved )
                            render_astbar( asw->frame_bars[i], asw->frame_sides[i] );
                }else
                {
                    if( i == FR_N )
                    {
                        if( set_astbar_size( asw->tbar, width, asw->tbar->height )||
                            canvas_moved )
                            render_astbar( asw->tbar, asw->frame_sides[i] );
                        corner_size = frame_corner_width(asw->frame_bars[FR_NE], asw->frame_bars[FR_NW]);
                    }else if( i == FR_S )
                        corner_size = frame_corner_width(asw->frame_bars[FR_SE], asw->frame_bars[FR_SW]);
                    if( asw->frame_bars[i] )
                        if( set_astbar_size( asw->frame_bars[i], width - corner_size, asw->frame_bars[i]->height )||
                            canvas_moved )
                            render_astbar( asw->frame_bars[i], asw->frame_sides[i] );
                }
                /* now corner's turn ( if any ) : */
                if( corner_size > 0 && canvas_moved )
                {
                    render_astbar( asw->frame_bars[LeftCorner(i)], asw->frame_sides[i] );
                    render_astbar( asw->frame_bars[RightCorner(i)], asw->frame_sides[i] );
                }

                /* now we need to show them on screen !!!! */
                update_canvas_display( asw->frame_sides[i] );
                break;
            }
    }
}

void
on_window_title_changed( ASWindow *asw, Bool update_display )
{
    if( AS_ASSERT(asw) )
        return ;
    if( asw->tbar )
    {
        ASCanvas *canvas = ASWIN_HFLAGS(asw, AS_VerticalTitle)?asw->frame_sides[FR_W]:asw->frame_sides[FR_N];
        set_astbar_label( asw->tbar, ASWIN_NAME(asw) );
        if( canvas && update_display )
        {
            render_astbar( asw->tbar, canvas );
            update_canvas_display( canvas );
        }
    }
}

void
on_window_status_changed( ASWindow *asw, Bool update_display )
{
    char *unfocus_mystyle = NULL ;
    int i ;
    Bool changed = False;
    unsigned short tbar_size = 0;

    if( AS_ASSERT(asw) )
        return ;

    if( ASWIN_GET_FLAGS(asw, AS_Sticky ) )
    {
        unfocus_mystyle = asw->hints->mystyle_names[BACK_STICKY];
        if( unfocus_mystyle == NULL )
            unfocus_mystyle = Scr.MSSWindow?Scr.MSSWindow->name:NULL ;
    }else
        unfocus_mystyle = asw->hints->mystyle_names[BACK_UNFOCUSED];

    if( unfocus_mystyle == NULL )
        unfocus_mystyle = Scr.MSUWindow?Scr.MSUWindow->name:NULL ;

    for( i = 0 ; i < FRAME_PARTS ; ++i )
        if( asw->frame_bars[i] )
            if( set_astbar_style( asw->frame_bars[i], BAR_STATE_UNFOCUSED, unfocus_mystyle ) )
                changed = True ;

    if( asw->tbar )
        if( set_astbar_style( asw->tbar, BAR_STATE_UNFOCUSED, unfocus_mystyle ) )
            changed = True ;

    if( changed )
    {/* now we need to update frame sizes in status */
        int tbar_side = FR_N ;
        unsigned int *frame_size = &(asw->status->frame_size[0]) ;
	    if( ASWIN_HFLAGS(asw, AS_VerticalTitle) )
		{
			tbar_size = calculate_astbar_width( asw->tbar );
			tbar_side = FR_W ;
			set_astbar_size( asw->tbar, tbar_size, asw->tbar?asw->tbar->height:0 );
		}else
		{
			tbar_size = calculate_astbar_height( asw->tbar );
			set_astbar_size( asw->tbar, asw->tbar?asw->tbar->width:0, tbar_size );
		}
		for( i = 0 ; i < FRAME_SIDES ; ++i )
			if( asw->frame_bars[i] )
				frame_size[i] = IsSideVertical(i)?asw->frame_bars[i]->width:
				                                  asw->frame_bars[i]->height ;
			else
				frame_size[i] = 0;

		frame_size[tbar_side] += tbar_size ;
        anchor2status ( asw->status, asw->hints, &(asw->anchor));
    }else if( asw->tbar )
        tbar_size = ASWIN_HFLAGS(asw, AS_VerticalTitle )?asw->tbar->width:asw->tbar->height;

    /* now we need to move/resize our frame window */
    if( ASWIN_GET_FLAGS( asw, AS_Iconic ) )
	{

    }else if( ASWIN_GET_FLAGS( asw, AS_Shaded ) && tbar_size > 0 )
	{
	    if( ASWIN_HFLAGS(asw, AS_VerticalTitle) )
			XMoveResizeWindow( dpy, asw->frame,
			                   asw->status->x, asw->status->y,
							   tbar_size, asw->status->height );
		else
			XMoveResizeWindow( dpy, asw->frame,
			                   asw->status->x, asw->status->y,
							   asw->status->width, tbar_size );
    }else
		XMoveResizeWindow( dpy, asw->frame,
		                   asw->status->x, asw->status->y,
						   asw->status->width, asw->status->height );

    BroadcastConfig (M_CONFIGURE_WINDOW, asw);
}
/********************************************************************/
/* end of ASWindow frame decorations management                     */
/********************************************************************/
/*
   ** returns a newline delimited list of the Mouse functions bound to a
   ** given context, in human readable form
 */
char         *
list_functions_by_context (int context)
{
	MouseButton  *btn;
	char         *str = NULL;
	int           allocated_bytes = 0;

	for (btn = Scr.MouseButtonRoot; btn != NULL; btn = btn->NextButton)
		if (btn->Context & context)
		{
			extern SyntaxDef FuncSyntax;
			TermDef      *fterm;

			fterm = FindTerm (&FuncSyntax, TT_ANY, btn->func);
			if (fterm != NULL)
			{
				char         *ptr;

				if (str)
				{
					str = realloc (str, allocated_bytes + 1 + fterm->keyword_len + 32);
					ptr = str + allocated_bytes;
					*(ptr++) = '\n';
				} else
					ptr = str = safemalloc (fterm->keyword_len + 32);

				sprintf (ptr, "%s: ", fterm->keyword);
				ptr += fterm->keyword_len + 2;
				if (btn->Modifier & ShiftMask)
				{
					strcat (ptr, "Shift+");
					ptr += 8;
				}
				if (btn->Modifier & ControlMask)
				{
					strcat (ptr, "Ctrl+");
					ptr += 7;
				}
				if (btn->Modifier & Mod1Mask)
				{
					strcat (ptr, "Meta+");
					ptr += 7;
				}
				sprintf (ptr, "Button %d", btn->Button);
				allocated_bytes = strlen (str);
			}
		}
	return str;
}

/*
   ** button argument - is translated button number !!!!
   ** 0 1 2 3 4  text 9 8 7 6 5
   ** none of that old crap !!!!
 */
Bool
create_titlebutton_balloon (ASWindow * tmp_win, int b)
{
/*	int           n = button / 2 + 5 * (button & 1); */
	char         *str = NULL ;
	Window 		  w = None;

	if (!ASWIN_HFLAGS(tmp_win, AS_Titlebar) ||
	    Scr.buttons[b].width <= 0 || IsBtnDisabled(tmp_win, b ))
		return False;

#if 0                                          /* TODO: fix balloons */
	if (IsLeftButton(b))
	{
		str = list_functions_by_context (C_L1 << b);
		w = tmp_win->left_w[b];
	}else
	{
		int rb = RightButtonIdx(b);
		str = list_functions_by_context (C_R1 << rb);
		w = tmp_win->right_w[rb];
	}
#endif
	if( str )
	{
		if( w )
			balloon_new_with_text (dpy, w, str);
		free (str);
	}
	return True;
}

/****************************************************************************
 * Interprets the property MOTIF_WM_HINTS, sets decoration and functions
 * accordingly
 *****************************************************************************/
void
SelectDecor (ASWindow * t)
{
	int border_width =0, resize_width = 0;
	ASHints *hints = t->hints ;
	ASFlagType tflags = hints->flags ;

	border_width = get_flags(tflags, AS_Border)?hints->border_width:Scr.NoBoundaryWidth;
	resize_width = get_flags(tflags, AS_Handles)?hints->handle_width:Scr.BoundaryWidth;

#ifdef SHAPE
	if (t->wShaped)
	{
		tflags &= ~(AS_Border | AS_Frame);
		clear_flags(hints->function_mask,AS_FuncResize);
	}
#endif
	/* Assume no decorations, and build up */
	t->boundary_width = 0;
	t->boundary_height = 0;

#ifndef NO_TEXTURE
	if (!get_flags(hints->function_mask,AS_FuncResize) || !DecorateFrames)
#endif /* !NO_TEXTURE */
	{
		/* a wide border, with corner tiles */
		if (get_flags(tflags,AS_Frame))
			clear_flags( t->hints->flags, AS_Frame );
	}

	if (!get_flags(hints->function_mask,AS_FuncPopup))
		disable_titlebuttons_with_function (t, F_POPUP);
	if (!get_flags(hints->function_mask,AS_FuncMinimize))
		disable_titlebuttons_with_function (t, F_ICONIFY);
	if (!get_flags(hints->function_mask,AS_FuncMaximize))
		disable_titlebuttons_with_function (t, F_MAXIMIZE);

	t->boundary_width = 0;
	t->boundary_height = ASWIN_HFLAGS(t, AS_Handles) ? Scr.BoundaryWidth : 0;
	t->corner_width = 16 + t->boundary_height;

	if( get_flags(tflags,AS_Frame) )
		t->bw = 0 ;
	else if( get_flags(tflags, AS_Handles))
		t->bw = border_width;
	else
		t->bw = 1;
}

/*
 * The following function was written for
 * new hints management code in libafterstep :
 */
Bool afterstep_parent_hints_func(Window parent, ASParentHints *dst )
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

static void
maximize_window_status( ASStatusHints *status, ASStatusHints *saved_status, ASStatusHints *adjusted_status, ASFlagType flags )
{
    ASRectangle rect ;
	rect.x = 0 ;
	rect.y = 0 ;
	rect.width = Scr.MyDisplayWidth ;
	rect.height = Scr.MyDisplayHeight ;

    if( get_flags( status->flags, AS_StartPositionUser|AS_StartPosition ) )
    {
        saved_status->x = status->x ;
        saved_status->y = status->y ;
        set_flags( saved_status->flags, AS_Position );
    }
    if( get_flags( status->flags, AS_StartSize ) )
    {
        saved_status->width = status->width ;
        saved_status->height = status->height ;
        set_flags( saved_status->flags, AS_Size );
    }
    if( get_flags( flags, AS_MaximizedX ) )
    {
        adjusted_status->x = rect.x ;
        adjusted_status->width = rect.width ;
    }
    if( get_flags( flags, AS_MaximizedY ) )
    {
        adjusted_status->y = rect.y ;
        adjusted_status->height = rect.height ;
    }
    set_flags( adjusted_status->flags, AS_Position|AS_Size );
}


Bool
init_aswindow_status( ASWindow *t, ASStatusHints *status )
{
	extern int PPosOverride ;
	ASStatusHints adjusted_status ;

	if( t->status == NULL )
		t->status = safecalloc(1, sizeof(ASStatusHints));

    if( get_flags( status->flags, AS_StartDesktop) && status->desktop != Scr.CurrentDesk )
        changeDesks( 0, status->desktop );
    t->status->desktop = Scr.CurrentDesk ;

    if( get_flags( status->flags, AS_StartViewportX))
        t->status->viewport_x = MIN(status->viewport_x,Scr.VxMax) ;
    else
        t->status->viewport_x = Scr.Vx ;

    if( get_flags( status->flags, AS_StartViewportY))
        t->status->viewport_y = MIN(status->viewport_y,Scr.VyMax) ;
    else
        t->status->viewport_y = Scr.Vy ;
    if( t->status->viewport_x != Scr.Vx || t->status->viewport_y != Scr.Vy )
        MoveViewport (t->status->viewport_x, t->status->viewport_y, 0);

    adjusted_status = *status ;

    if( PPosOverride )
        set_flags( adjusted_status.flags, AS_Position );
	else if( get_flags(Scr.flags, NoPPosition) &&
            !get_flags( status->flags, AS_StartPositionUser ) )
        clear_flags( adjusted_status.flags, AS_Position );

    if( get_flags( status->flags, AS_MaximizedX|AS_MaximizedY ))
        maximize_window_status( status, t->saved_status, &adjusted_status, status->flags );
    else if( !get_flags( adjusted_status.flags, AS_Position ))
        if( !place_aswindow( t, &adjusted_status ) )
            return False;

    if( is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
		print_status_hints( NULL, NULL, &adjusted_status );

	/* invalidating position in status so that change_placement would always work */
	t->status->x = adjusted_status.x-1000 ;
	t->status->y = adjusted_status.y-1000 ;
	t->status->width = adjusted_status.width-1000 ;
	t->status->height = adjusted_status.height-1000 ;

#if 0
    if( change_placement( t->scr, t->hints, t->status, &(t->anchor), &adjusted_status, t->scr->Vx, t->scr->Vy, adjusted_status.flags ) != 0 )
	{
        anchor_decor_client( t->decor, t->hints, t->status, &(t->anchor), t->scr->Vx, t->scr->Vy );
		configure_decor( t->decor, t->status );
	}
#endif
    /* TODO: AS_Iconic */
	if (ASWIN_GET_FLAGS(t, AS_StartsIconic ))
		t->flags |= STARTICONIC;

	if( !ASWIN_GET_FLAGS(t, AS_StartLayer ) )
		ASWIN_LAYER(t) = AS_LayerNormal ;

    if( ASWIN_GET_FLAGS(t, AS_StartsSticky ) )
		t->flags |= STICKY;

    if( ASWIN_GET_FLAGS(t, AS_StartsShaded ) )
		t->flags |= SHADED;

    status2anchor( &(t->anchor), t->hints, t->status, Scr.VxMax, Scr.VyMax);

	return True;
}

/***************************************************************************************/
/* iconify/deiconify code :                                                            */
/***************************************************************************************/
Bool
iconify_window( ASWindow *asw, Bool iconify )
{
LOCAL_DEBUG_CALLER_OUT( "client = %p, iconify = %d, batch = %d", asw, iconify, batch );
    if( AS_ASSERT(asw) )
        return False;
    if( (iconify?1:0) == (ASWIN_GET_FLAGS(asw, AS_Iconic )?1:0) )
        return False;

    if( iconify )
    {
        /*
         * Performing transition NormalState->IconicState
         * Prevent the receipt of an UnmapNotify, since we trigger any such event
         * as signal for transition to the Withdrawn state.
         */
		asw->flags &= ~MAPPED;
		asw->DeIconifyDesk = ASWIN_DESK(asw);

LOCAL_DEBUG_OUT( "unmaping client window 0x%lX", (unsigned long)asw->w );
        quietly_unmap_window( asw->w, AS_CLIENT_EVENT_MASK );
        XUnmapWindow (dpy, asw->frame);

		SetMapStateProp (asw, IconicState);

		/* this transient logic is kinda broken - we need to exepriment with it and
		   fix it eventually, less we want to end up with iconified transients
		   without icons
		 */
		if ( ASWIN_HFLAGS(asw, AS_Transient))
		{
			asw->flags |= ICONIFIED | ICON_UNMAPPED;
			Broadcast (M_ICONIFY, 7, asw->w, asw->frame,
					   (unsigned long)asw, -10000, -10000, asw->icon_p_width, asw->icon_p_height);
			BroadcastConfig (M_CONFIGURE_WINDOW, asw);
		}else
		{
			if (asw->icon_pixmap_w == None && asw->icon_title_w == None)
				CreateIconWindow (asw);
			asw->flags |= ICONIFIED;
			asw->flags &= ~ICON_UNMAPPED;
			AutoPlace (asw);
			XFlush (dpy);
			Broadcast (M_ICONIFY, 7, asw->w, asw->frame,
			  		   (unsigned long)asw,
					    asw->icon_p_x, asw->icon_p_y,
						asw->icon_p_width, asw->icon_p_height);
			BroadcastConfig (M_CONFIGURE_WINDOW, asw);

			LowerWindow (asw);
			if (ASWIN_DESK(asw) == Scr.CurrentDesk)
			{
				if (asw->icon_pixmap_w != None)
					XMapWindow (dpy, asw->icon_pixmap_w);
				if (asw->icon_title_w != None)
					XMapWindow (dpy, asw->icon_title_w);
			}

			if ((Scr.flags & ClickToFocus) || (Scr.flags & SloppyFocus))
			{
				if (asw == Scr.Focus)
				{
		  			if (Scr.PreviousFocus == Scr.Focus)
						Scr.PreviousFocus = NULL;
					if ((Scr.flags & ClickToFocus) && (asw->next))
						SetFocus (asw->next->w, asw->next, False);
					else
						SetFocus (Scr.NoFocusWin, NULL, False);
				}
			}
		}
LOCAL_DEBUG_OUT( "updating status to iconic for client %p(\"%s\")", asw, ASWIN_NAME(asw) );
        set_flags( asw->status->flags, AS_Iconic );
        asw->status->icon_window = asw->icon_pixmap_w?asw->icon_pixmap_w:asw->icon_title_w ;
    }else
    {   /* Performing transition IconicState->NormalState  */
        asw->flags |= MAPPED;

        if (Scr.flags & StubbornIcons)
            ASWIN_DESK(asw) = asw->DeIconifyDesk;
        else
            ASWIN_DESK(asw) = Scr.CurrentDesk;
        set_client_desktop( asw->w, ASWIN_DESK(asw));

        /* TODO: make sure that the window is on this screen */

        if (asw->icon_pixmap_w != None)
            XUnmapWindow (dpy, asw->icon_pixmap_w);
        if (asw->icon_title_w != None)
            XUnmapWindow (dpy, asw->icon_title_w);
        Broadcast (M_DEICONIFY, 7, asw->w, asw->frame, (unsigned long)asw,
                   asw->icon_p_x, asw->icon_p_y, asw->icon_p_width, asw->icon_p_height);
        XMapWindow (dpy, asw->w);
        if (ASWIN_DESK(asw) == Scr.CurrentDesk)
        {
            XMapWindow (dpy, asw->frame);
            asw->flags |= MAP_PENDING;
        }
        SetMapStateProp (asw, NormalState);
        asw->flags &= ~ICONIFIED;
        asw->flags &= ~ICON_UNMAPPED;

        XRaiseWindow (dpy, asw->w);

        XFlush (dpy);
        if( !ASWIN_HFLAGS(asw, AS_Transient ))
        {
            if ((Scr.flags & StubbornIcons) || (Scr.flags & ClickToFocus))
                FocusOn (asw, 1, False);
            else
                RaiseWindow (asw);
        }
        clear_flags( asw->status->flags, AS_Iconic );
        asw->status->icon_window = None ;
    }

    on_window_status_changed( asw, True );
    return True;
}

/****************************************************************************
 *
 * Sets up the shaped window borders
 *
 ****************************************************************************/
void
SetShape (ASWindow *asw, int w)
{
#ifdef SHAPE
	if( asw )
	{
		int bw = asw->status->border_width ;

		XShapeCombineShape (dpy, asw->frame, ShapeBounding,
	  	      	            asw->status->x + bw,
							asw->status->y + bw,
							asw->w, ShapeBounding, ShapeSet);

		/* windows with titles */
		if (ASWIN_HFLAGS(asw,AS_Titlebar) && asw->tbar)
		{
			XRectangle    rect;

			rect.x = asw->tbar->win_x - bw;
			rect.y = asw->tbar->win_y - bw;
			rect.width  = asw->tbar->width  + 2*bw;
			rect.height = asw->tbar->height + 2*bw;

			XShapeCombineRectangles (dpy, asw->frame, ShapeBounding,
									 0, 0, &rect, 1, ShapeUnion, Unsorted);
		}

		/* TODO: add frame decorations shape */

		/* update icon shape */
		if (asw->icon_pixmap_w != None)
			UpdateIconShape (asw);
	}
#endif /* SHAPE */
}

/********************************************************************
 *
 * Sets the input focus to the indicated window.
 *
 **********************************************************************/

Bool
SetFocus (Window w, ASWindow * Fw, Bool circulated)
{
	int           i;
	extern Time   lastTimestamp;
	Bool          focusAccepted = True;

	if (!circulated && Fw)
		SetCirculateSequence (Fw, 1);

	/* ClickToFocus focus queue manipulation */
	if (Fw && Fw != Scr.Focus && Fw != &Scr.ASRoot)
		Fw->focus_sequence = Scr.next_focus_sequence++;

	if (Scr.NumberOfScreens > 1)
	{
		XQueryPointer (dpy, Scr.Root, &JunkRoot, &JunkChild,
					   &JunkX, &JunkY, &JunkX, &JunkY, &JunkMask);
		if (JunkRoot != Scr.Root)
		{
			if ((Scr.flags & ClickToFocus) && (Scr.Ungrabbed != NULL))
			{
				/* Need to grab buttons for focus window */
				XSync (dpy, 0);
				for (i = 0; i < MAX_MOUSE_BUTTONS; i++)
					if (Scr.buttons2grab & (1 << i))
						MyXGrabButton (dpy, i + 1, 0, Scr.Ungrabbed->frame,
									   True, ButtonPressMask, GrabModeSync,
									   GrabModeAsync, None, Scr.ASCursors[SYS]);
				Scr.Focus = NULL;
				Scr.Ungrabbed = NULL;
				XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, lastTimestamp);
			}
			return False;
		}
	}

	if (Fw && (Fw->status->desktop != Scr.CurrentDesk))
	{
		Fw = NULL;
		w = Scr.NoFocusWin;
		focusAccepted = False;
	}
	if ((Scr.flags & ClickToFocus) && (Scr.Ungrabbed != Fw))
	{
		/* need to grab all buttons for window that we are about to
		 * unfocus */
		if (Scr.Ungrabbed != NULL)
		{
			XSync (dpy, 0);
			for (i = 0; i < MAX_MOUSE_BUTTONS; i++)
				if (Scr.buttons2grab & (1 << i))
				{
					MyXGrabButton (dpy, i + 1, 0, Scr.Ungrabbed->frame,
								   True, ButtonPressMask, GrabModeSync,
								   GrabModeAsync, None, Scr.ASCursors[SYS]);
				}
			Scr.Ungrabbed = NULL;
		}
		/* if we do click to focus, remove the grab on mouse events that
		 * was made to detect the focus change */
		if ((Scr.flags & ClickToFocus) && (Fw != NULL))
		{
			for (i = 0; i < MAX_MOUSE_BUTTONS; i++)
				if (Scr.buttons2grab & (1 << i))
				{
					MyXUngrabButton (dpy, i + 1, 0, Fw->frame);
				}
			Scr.Ungrabbed = Fw;
		}
	}

	/* try to give focus to shaded windows */
	if ((Fw != NULL) && (Fw->flags & SHADED))
	{
		if (Fw->frame != None)
			w = Fw->frame;
		else
			return False;
	}

	/* give focus to icons */
	if (Fw != NULL && (Fw->flags & ICONIFIED))
	{
		if (Fw->icon_title_w != None)
			w = Fw->icon_title_w;
		else if (Fw->icon_pixmap_w != None)
			w = Fw->icon_pixmap_w;
	}

	if (Fw && ASWIN_HFLAGS(Fw, AS_AcceptsFocus))
	{
		/* Window will accept input focus */
		XSetInputFocus (dpy, w, RevertToParent, lastTimestamp);
		Scr.Focus = Fw;
	} else if ((Scr.Focus) && (Scr.Focus->status->desktop == Scr.CurrentDesk))
	{
		/* Window doesn't want focus. Leave focus alone */
		/* XSetInputFocus (dpy,Scr.Hilite->w , RevertToParent, lastTimestamp); */
		focusAccepted = False;
	} else
	{
		XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, lastTimestamp);
		Scr.Focus = NULL;
		focusAccepted = False;
	}
	if ((Fw) && get_flags(Fw->hints->protocols, AS_DoesWmTakeFocus))
		send_clientmessage (w, _XA_WM_TAKE_FOCUS, lastTimestamp);

	XSync (dpy, 0);

	return focusAccepted;
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
AddWindow (Window w)
{
	ASWindow     *tmp_win;					   /* new afterstep window structure */
	unsigned long valuemask;				   /* mask for create windows */
	XSetWindowAttributes attributes;		   /* attributes for create windows */
	int           i;
	int           a, b;
	extern Bool   NeedToResizeToo;
	extern ASWindow *colormap_win;

#ifdef I18N
	char        **list;
	int           num;
#endif
	ASRawHints    raw_hints ;
    ASHints      *hints  = NULL;
    ASStatusHints status;
	extern ASDatabase *Database;


	/* allocate space for the afterstep window */
	tmp_win = safecalloc (1, sizeof (ASWindow));

	NeedToResizeToo = False;

//    init_titlebar_windows (tmp_win, False);
//    init_titlebutton_windows (tmp_win, False);

	tmp_win->flags = VISIBLE;
	tmp_win->w = w;

	if (XGetGeometry (dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
					  &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
	{
		free ((char *)tmp_win);
		return (NULL);
	}

    set_parent_hints_func( afterstep_parent_hints_func ); /* callback for collect_hints() */

	if( collect_hints( &Scr, w, HINT_ANY, &raw_hints ) )
    {
        if( is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
            print_hints( NULL, NULL, &raw_hints );
        hints = merge_hints( &raw_hints, Database, &status, Scr.supported_hints, HINT_ANY, NULL );
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
			return (NULL);
		}
		tmp_win->hints = hints ;
    }else
	{
		show_warning( "Unable to obtain window management hints for window %X", w );
		free ((char *)tmp_win);
		return (NULL);
	}

/*  fprintf( stderr, "[%s]: %dx%d%+d%+d\n", tmp_win->name, JunkWidth, JunkHeight, JunkX, JunkY );
*/
	tmp_win->focus_sequence = 1;
	SetCirculateSequence (tmp_win, -1);

	if (!XGetWindowAttributes (dpy, tmp_win->w, &tmp_win->attr))
		tmp_win->attr.colormap = Scr.ASRoot.attr.colormap;

	tmp_win->old_bw = tmp_win->attr.border_width;

#ifdef SHAPE
	{
		int           xws, yws, xbs, ybs;
		unsigned      wws, hws, wbs, hbs;
		int           boundingShaped, clipShaped;

		XShapeSelectInput (dpy, tmp_win->w, ShapeNotifyMask);
		XShapeQueryExtents (dpy, tmp_win->w,
							&boundingShaped, &xws, &yws, &wws, &hws,
							&clipShaped, &xbs, &ybs, &wbs, &hbs);
		tmp_win->wShaped = boundingShaped;
	}
#endif /* SHAPE */

	SelectDecor (tmp_win);

	if (!get_flags(tmp_win->hints->flags,AS_Icon) || get_flags(Scr.flags, SuppressIcons))
		tmp_win->flags |= SUPPRESSICON;
	else
	{	/* an icon was specified */
		tmp_win->icon_pm_file = NULL ;
		if( tmp_win->hints->icon.window != None )
		{

		}else
		{
			tmp_win->icon_pm_file = tmp_win->hints->icon_file;
			if(tmp_win->icon_pm_file == NULL )/* use default icon */
		  		tmp_win->icon_pm_file = Scr.DefaultIcon;
		}
	}
	tmp_win->icon_pm_pixmap = None;
	tmp_win->icon_pm_mask = None;

	if( !init_aswindow_status( tmp_win, &status ) )
	{
		Destroy (tmp_win, False);
		return NULL;
	}

#if 0
    /* Old stuff - needs to be updated to use tmp_win->hints :*/
    /* size and place the window */
	set_titlebar_geometry (tmp_win);
	get_frame_geometry (tmp_win, tmp_win->attr.x, tmp_win->attr.y, tmp_win->attr.width,
						tmp_win->attr.height, NULL, NULL, &tmp_win->frame_width,
						&tmp_win->frame_height);
	ConstrainSize (tmp_win, &tmp_win->frame_width, &tmp_win->frame_height);

	get_frame_geometry (tmp_win, tmp_win->attr.x, tmp_win->attr.y, tmp_win->attr.width,
						tmp_win->attr.height, &tmp_win->frame_x, &tmp_win->frame_y,
						&tmp_win->frame_width, &tmp_win->frame_height);

#endif
    /*
	 * Make sure the client window still exists.  We don't want to leave an
	 * orphan frame window if it doesn't.  Since we now have the server
	 * grabbed, the window can't disappear later without having been
	 * reparented, so we'll get a DestroyNotify for it.  We won't have
	 * gotten one for anything up to here, however.
	 */
	XGrabServer (dpy);
	if (XGetGeometry (dpy, w, &JunkRoot, &JunkX, &JunkY,
					  &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
	{
		destroy_hints(tmp_win->hints, False);
		free ((char *)tmp_win);
		XUngrabServer (dpy);
		return (NULL);
	}
    XSetWindowBorderWidth (dpy, tmp_win->w, 0);

	tmp_win->flags &= ~ICONIFIED;
	tmp_win->flags &= ~ICON_UNMAPPED;
	tmp_win->flags &= ~MAXIMIZED;

	/* the newly created window will be on the top of the warp list */
	tmp_win->warp_index = 0;
	ChangeWarpIndex (++LastWarpIndex, F_WARP_F);

	/* add the window into the afterstep list */
	tmp_win->next = Scr.ASRoot.next;
	if (Scr.ASRoot.next != NULL)
		Scr.ASRoot.next->prev = tmp_win;
	tmp_win->prev = &Scr.ASRoot;
	Scr.ASRoot.next = tmp_win;

	/* create windows */
	valuemask = CWBorderPixel | CWCursor | CWEventMask;
	if (Scr.d_depth < 2)
	{
		attributes.background_pixmap = Scr.light_gray_pixmap;
		if (tmp_win->flags & STICKY)
			attributes.background_pixmap = Scr.sticky_gray_pixmap;
		valuemask |= CWBackPixmap;
	} else
	{
		attributes.background_pixmap = ParentRelative;
		valuemask |= CWBackPixmap;
	}
	attributes.border_pixel = Scr.asv->black_pixel;

	attributes.cursor = Scr.ASCursors[DEFAULT];
	attributes.event_mask = AS_FRAME_EVENT_MASK;

	if (Scr.flags & SaveUnders)
	{
		valuemask |= CWSaveUnder;
		attributes.save_under = TRUE;
	}
	/* What the heck, we'll always reparent everything from now on! */
	tmp_win->frame =
		create_visual_window (Scr.asv, Scr.Root, tmp_win->frame_x, tmp_win->frame_y,
							  tmp_win->frame_width, tmp_win->frame_height,
							  tmp_win->bw, InputOutput, valuemask, &attributes);

    register_aswindow( tmp_win->frame, tmp_win );
    register_aswindow( tmp_win->w, tmp_win );

    attributes.save_under = FALSE;

    redecorate_window       ( tmp_win, False );
    on_window_title_changed ( tmp_win, False );
    on_window_status_changed( tmp_win, False );

	XMapSubwindows (dpy, tmp_win->frame);
    XRaiseWindow (dpy, tmp_win->w);
    XReparentWindow (dpy, tmp_win->w, tmp_win->frame, 0, 0);

	valuemask = (CWEventMask | CWDontPropagate);
	attributes.event_mask = AS_CLIENT_EVENT_MASK;
	attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;
	if (Scr.flags & AppsBackingStore)
	{
		valuemask |= CWBackingStore;
		attributes.backing_store = WhenMapped;
	}
	XChangeWindowAttributes (dpy, tmp_win->w, valuemask, &attributes);
	XAddToSaveSet (dpy, tmp_win->w);

	/*
	 * Reparenting generates an UnmapNotify event, followed by a MapNotify.
	 * Set the map state to FALSE to prevent a transition back to
	 * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
	 * again in HandleMapNotify.
	 */
	tmp_win->flags &= ~MAPPED;

#if 0
	width = tmp_win->frame_width;
	height = tmp_win->frame_height;
	tmp_win->frame_width = 0;
	tmp_win->frame_height = 0;

    SetupFrame (tmp_win, tmp_win->frame_x, tmp_win->frame_y, width, height, True);
#endif
	/* wait until the window is iconified and the icon window is mapped
	 * before creating the icon window
	 */
	tmp_win->icon_title_w = None;
	GrabButtons (tmp_win);
	GrabKeys (tmp_win);
	RaiseWindow (tmp_win);
	XUngrabServer (dpy);

	XGetGeometry (dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
				  &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
	XTranslateCoordinates (dpy, tmp_win->frame, Scr.Root, JunkX, JunkY, &a, &b, &JunkChild);

	if (Scr.flags & ClickToFocus)
	{
		/* need to grab all buttons for window that we are about to
		 * unhighlight */
		for (i = 0; i < MAX_MOUSE_BUTTONS; i++)
			if (Scr.buttons2grab & (1 << i))
			{
				MyXGrabButton (dpy, i + 1, 0, tmp_win->frame,
							   True, ButtonPressMask, GrabModeSync,
							   GrabModeAsync, None, Scr.ASCursors[SYS]);
			}
	}
	BroadcastConfig (M_ADD_WINDOW, tmp_win);

	BroadcastName (M_WINDOW_NAME, tmp_win->w, tmp_win->frame,
				   (unsigned long)tmp_win, tmp_win->hints->names[0]);
	BroadcastName (M_ICON_NAME, tmp_win->w, tmp_win->frame,
				   (unsigned long)tmp_win, tmp_win->hints->icon_name);
	BroadcastName (M_RES_CLASS, tmp_win->w, tmp_win->frame,
				   (unsigned long)tmp_win, tmp_win->hints->res_class);
	BroadcastName (M_RES_NAME, tmp_win->w, tmp_win->frame,
				   (unsigned long)tmp_win, tmp_win->hints->res_name);

	if (!(XGetWindowAttributes (dpy, tmp_win->w, &(tmp_win->attr))))
		tmp_win->attr.colormap = Scr.ASRoot.attr.colormap;
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
	InstallWindowColormaps (colormap_win);
	if (!ASWIN_HFLAGS(tmp_win, AS_SkipWinList))
		update_windowList ();

	return (tmp_win);
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabButtons - grab needed buttons for the window
 *
 *  Inputs:
 *	tmp_win - the afterstep window structure to use
 *
 ***********************************************************************/
void
GrabButtons (ASWindow * tmp_win)
{
	MouseButton  *MouseEntry;

	MouseEntry = Scr.MouseButtonRoot;
	while (MouseEntry != (MouseButton *) 0)
	{
		if ((MouseEntry->func != (int)0) && (MouseEntry->Context & C_WINDOW))
		{
			if (MouseEntry->Button > 0)
			{
				MyXGrabButton (dpy, MouseEntry->Button,
							   MouseEntry->Modifier, tmp_win->w,
							   True, ButtonPressMask | ButtonReleaseMask,
							   GrabModeAsync, GrabModeAsync, None, Scr.ASCursors[DEFAULT]);
			} else
			{
				int           i;

				for (i = 0; i < MAX_MOUSE_BUTTONS; i++)
				{
					MyXGrabButton (dpy, i + 1, MouseEntry->Modifier, tmp_win->w,
								   True, ButtonPressMask | ButtonReleaseMask,
								   GrabModeAsync, GrabModeAsync, None, Scr.ASCursors[DEFAULT]);
				}
			}
		}
		MouseEntry = MouseEntry->NextButton;
	}
	return;
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabKeys - grab needed keys for the window
 *
 *  Inputs:
 *	tmp_win - the afterstep window structure to use
 *
 ***********************************************************************/
void
GrabKeys (ASWindow * tmp_win)
{
	FuncKey      *tmp;

	for (tmp = Scr.FuncKeyRoot; tmp != NULL; tmp = tmp->next)
	{
		if (tmp->cont & (C_WINDOW | C_TITLE | C_RALL | C_LALL | C_SIDEBAR))
			MyXGrabKey (dpy, tmp->keycode, tmp->mods, tmp_win->frame, True,
						GrabModeAsync, GrabModeAsync);
	}
	return;
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Archive of old code :                                              */
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

#if 0                                          /* old useless code  */
void
init_titlebar_windows (ASWindow * tmp_win, Bool free_resources)
{
	if (free_resources)
	{
		if (tmp_win->title_w != None)
		{
			XDestroyWindow (dpy, tmp_win->title_w);
			XDeleteContext (dpy, tmp_win->title_w, ASContext);
		}
	}
	tmp_win->title_w = None;
}

/*
 * Create the titlebar.
 * We place the window anywhere, and let SetupFrame() take care of
 *  putting it where it belongs.
 */
Bool
create_titlebar_windows (ASWindow * tmp_win)
{
	unsigned long valuemask;
	XSetWindowAttributes attributes;

	if (!ASWIN_HFLAGS(tmp_win, AS_Titlebar))
		return False;

	valuemask = CWBackPixmap | CWBorderPixel | CWCursor | CWEventMask;
	attributes.background_pixmap = ParentRelative;
	attributes.border_pixel = Scr.asv->black_pixel;
	if (Scr.flags & BackingStore)
	{
		valuemask |= CWBackingStore;
		attributes.backing_store = WhenMapped;
	}
	attributes.event_mask = (ButtonPressMask | ButtonReleaseMask | ExposureMask |
							 EnterWindowMask | LeaveWindowMask);
	attributes.cursor = Scr.ASCursors[TITLE_CURSOR];
	tmp_win->title_x = -999;
	tmp_win->title_y = -999;
	tmp_win->title_w = create_visual_window (Scr.asv, tmp_win->frame,
											 tmp_win->title_x, tmp_win->title_y,
											 tmp_win->title_width,
											 (tmp_win->title_height + tmp_win->bw), 0, InputOutput,
											 valuemask, &attributes);
	XSaveContext (dpy, tmp_win->title_w, ASContext, (caddr_t) tmp_win);
	return True;
}

void
init_titlebutton_windows (ASWindow * tmp_win, Bool free_resources)
{
	int           i;

	if (free_resources)
	{
		for (i = 0; i < (TITLE_BUTTONS>>1); i++)
		{
			if (tmp_win->left_w[i] != None)
			{
				balloon_delete (balloon_find (tmp_win->left_w[i]));
				XDestroyWindow (dpy, tmp_win->left_w[i]);
				XDeleteContext (dpy, tmp_win->left_w[i], ASContext);
			}
			if (tmp_win->right_w[i] != None)
			{
				balloon_delete (balloon_find (tmp_win->left_w[i]));
				XDestroyWindow (dpy, tmp_win->right_w[i]);
				XDeleteContext (dpy, tmp_win->right_w[i], ASContext);
			}
		}
	}
	for (i = 0; i < (TITLE_BUTTONS>>1); i++)
	{
		tmp_win->left_w[i] = None;
		tmp_win->right_w[i] = None;
	}
	tmp_win->nr_right_buttons = 0;
	tmp_win->nr_left_buttons = 0;
	tmp_win->space_taken_left_buttons = 0;
	tmp_win->space_taken_right_buttons = 0;
}

/*
 * Create the titlebar buttons.
 * We place the windows anywhere, and let SetupFrame() take care of
 *  putting them where they belong.
 */
Bool
create_titlebutton_windows (ASWindow * tmp_win)
{
	int           i;
	unsigned long valuemask;
	XSetWindowAttributes attributes;

	if (!ASWIN_HFLAGS(tmp_win, AS_Titlebar))
		return False;
	valuemask = CWBackPixmap | CWBorderPixel | CWCursor | CWEventMask;
	attributes.background_pixmap = ParentRelative;
	attributes.border_pixel = Scr.asv->black_pixel;
	if (Scr.flags & BackingStore)
	{
		valuemask |= CWBackingStore;
		attributes.backing_store = WhenMapped;
	}
	attributes.event_mask = (ButtonPressMask | ButtonReleaseMask | ExposureMask |
							 EnterWindowMask | LeaveWindowMask);
	attributes.cursor = Scr.ASCursors[SYS];
	tmp_win->nr_left_buttons = 0;
	tmp_win->nr_right_buttons = 0;
	tmp_win->space_taken_left_buttons = 0;
	tmp_win->space_taken_right_buttons = 0;
	for (i = 0; i < TITLE_BUTTONS; i++)
	{
		Window w ;
		if( Scr.buttons[i].width <= 0 || IsBtnDisabled(tmp_win, i ) )
			continue;
		w = create_visual_window (Scr.asv, tmp_win->frame, -999, -999,
		  						  Scr.buttons[i].width, Scr.buttons[i].height,
								  0, InputOutput, valuemask, &attributes);
		XSaveContext (dpy, w, ASContext, (caddr_t) tmp_win);

		if (IsLeftButton(i))
		{
			tmp_win->left_w[i] = w ;
			tmp_win->nr_left_buttons++;
			tmp_win->space_taken_left_buttons +=
				Scr.buttons[i].width + Scr.TitleButtonSpacing;
		}else
		{
			int rb = RightButtonIdx(i);
			tmp_win->right_w[rb] = w ;
			tmp_win->nr_right_buttons++;
			tmp_win->space_taken_right_buttons +=
				Scr.buttons[i].width + Scr.TitleButtonSpacing;
		}
		create_titlebutton_balloon (tmp_win, i);
	}

	/* if left buttons exist, remove one extraneous space so title is shown correctly */
	if (tmp_win->space_taken_left_buttons)
		tmp_win->space_taken_left_buttons -= Scr.TitleButtonSpacing;
	/* if right buttons exist, remove one extraneous space so title is shown correctly */
	if (tmp_win->space_taken_right_buttons)
		tmp_win->space_taken_right_buttons -= Scr.TitleButtonSpacing;

	/* add borders if Scr.TitleButtonStyle == 0 */
	if (!Scr.TitleButtonStyle)
	{
		tmp_win->space_taken_left_buttons += 6;
		tmp_win->space_taken_right_buttons += 6;
	}

	return True;
}


#endif

