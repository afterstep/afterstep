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
#include "../../configure.h"
#define LOCAL_DEBUG

#include "../../include/asapp.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/event.h"
#include "../../include/decor.h"
#include "../../include/screen.h"
#include "../../include/parser.h"
#include "../../include/module.h"
#include "../../include/mystyle.h"
#include "../../include/clientprops.h"
#include "../../include/hints.h"
#include "../../include/balloon.h"
#include "../../libAfterImage/afterimage.h"
#include "asinternals.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#include <X11/Xresource.h>
#endif /* SHAPE */

#warning "implement titlebar config language in redecorate_window"

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
 *    For each frame part and title we have separate TBarData structure. ( 9 total )
 *    In windows with vertical title we keep the orientation of canvases but turn
 *    tbars counterclockwise 90 degrees. As The result Title tbar will be shown on FR_W canvas,
 *    instead of FR_N canvas. Frame parts will be shown on the following canvases :
 *      Frame part  |   Canvas
 *    --------------+-----------
 *      FR_N            FR_W
 *      FR_NE           FR_W
 *      FR_E            FR_N
 *      FR_SE           FR_E
 *      FR_S            FR_E
 *      FR_SW           FR_E
 *      FR_W            FR_S
 *      FR_NW           FR_W
 *
 *      That has to be taken on account whenever we have to associate tbars and canvases,
 *      like in moving/resizing, redrawing, hiliting, pressing and context lookup
 *
 * Icon is drawn on one or two canvases. If separate button titles are selected and Icon title
 * is allowed for this style, then separate canvas will be created for the titlebar.
 * Respectively icon_title_canvas will be different then icon_canvas. In all other cases
 * Icon_title_canvas will be the same as icon_canvas.
 */

static int NormalX, NormalY;
static unsigned int NormalWidth, NormalHeight ;
typedef struct ASOrientation
{
    unsigned int frame_contexts[FRAME_PARTS];
    unsigned int frame_rotation[FRAME_PARTS];
    unsigned int tbar2canvas_xref[FRAME_PARTS+1];
    unsigned int tbar_side;
    unsigned int tbar_corners[2];
    unsigned int tbar_mirror_corners[2];               /* see note below */
    unsigned int sbar_side;
    unsigned int sbar_corners[2];
    unsigned int sbar_mirror_corners[2];               /* see note below */
    unsigned int left_side, right_side;
    unsigned int left_mirror_side, right_mirror_side;  /* see note below */
    unsigned long horz_side_mask;
    int left_btn_order, right_btn_order;
    int *in_x, *in_y;
    unsigned int *in_width, *in_height;
    int *out_x, *out_y;
    unsigned int *out_width, *out_height;
    int flip;
    unsigned int default_tbar_elem_col[3];
    unsigned int default_tbar_elem_row[3];
}ASOrientation;


/* Mirror Note :
 *
 * For the purpose of sizing/placing left and right sides and corners - we employ somewhat
 * twisted logic - we mirror sides over lt2rb diagonal in case of
 * vertical title orientation. That allows us to apply simple x/y switching instead of complex
 * calculations. Note that we only do that for placement purposes. Contexts and images are
 * still taken from MyFrame parts as if it was rotated counterclockwise instead of mirrored.
 */

ASOrientation HorzOrientation =
{
    { C_FrameN, C_FrameE, C_FrameS, C_FrameW, C_FrameNW,C_FrameNE,C_FrameSW,C_FrameSE },
    {FR_N, FR_E, FR_S, FR_W, FR_NW, FR_NE, FR_SW, FR_SE },
/*      N     E     S     W     NW    NE    SW    SE    TITLE */
    {FR_N, FR_E, FR_S, FR_W, FR_N, FR_N, FR_S, FR_S, FR_N },
    FR_N,
    {FR_NW, FR_NE},
    {FR_NW, FR_NE},
    FR_S,
    {FR_SW, FR_SE},
    {FR_SW, FR_SE},
    FR_W, FR_E,
    FR_W, FR_E,
    MYFRAME_HOR_MASK,
    TBTN_ORDER_L2R,TBTN_ORDER_R2L,
    &NormalX, &NormalY, &NormalWidth, &NormalHeight,
    &NormalX, &NormalY, &NormalWidth, &NormalHeight,
    0,
    {0, 1, 2},
    {0, 0, 0}
};

ASOrientation VertOrientation =
{
    { C_FrameW, C_FrameN, C_FrameE, C_FrameS, C_FrameSW,C_FrameNW,C_FrameSE,C_FrameNE },
    {FR_W, FR_N, FR_E, FR_S, FR_SW, FR_NW, FR_SE, FR_NE },
/*      N     E     S     W     NW    NE    SW    SE    TITLE */
    {FR_N, FR_E, FR_S, FR_W, FR_W, FR_E, FR_W, FR_E, FR_W },
    FR_W,
    {FR_SW, FR_NW},
    {FR_NW, FR_SW},
    FR_E,
    {FR_SE, FR_NE},
    {FR_NE, FR_SE},
    FR_S, FR_N,
    FR_N, FR_S,
    MYFRAME_VERT_MASK,
    TBTN_ORDER_B2T,TBTN_ORDER_T2B,
    &NormalX, &NormalY, &NormalWidth, &NormalHeight,
    &NormalY, &NormalX, &NormalHeight, &NormalWidth,
    FLIP_VERTICAL,
    {0, 0, 0},
    {2, 1, 0}
};

static inline ASOrientation*
get_orientation_data( ASWindow *asw )
{
    if( asw )
        return ASWIN_HFLAGS(asw, AS_VerticalTitle)?&VertOrientation:&HorzOrientation;
    return &HorzOrientation;
}

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
            if (Scr.Feel.flags & BackingStore)
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
LOCAL_DEBUG_OUT( "++CREAT Client(%lx(%s))->side(%d)->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname",side, canvas, canvas->w );
        }
    }else if( canvas != NULL )
    {                                          /* destroy canvas here */
		w = canvas->w ;
LOCAL_DEBUG_OUT( "--DESTR Client(%lx(%s))->side(%d)->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname",side, canvas, canvas->w );
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
            if( Scr.asv->visual_info.visual == DefaultVisual( dpy, Scr.screen ) )
            {/* only if root has same depth and visual as us! */
                attributes.background_pixmap = ParentRelative;
                valuemask |= CWBackPixmap;
            }
            attributes.border_pixel = Scr.asv->black_pixel;
            attributes.cursor = Scr.Feel.cursors[DEFAULT];
            attributes.event_mask = AS_FRAME_EVENT_MASK;

            if (get_flags(Scr.Feel.flags, SaveUnders))
            {
                valuemask |= CWSaveUnder;
                attributes.save_under = True;
            }
            w = create_visual_window (Scr.asv, Scr.Root, -10, -10, 5, 5,
                                      asw->status?asw->status->border_width:0, InputOutput, valuemask, &attributes);
            asw->frame = w ;
            register_aswindow( w, asw );
            canvas = create_ascanvas_container( w );
LOCAL_DEBUG_OUT( "++CREAT Client(%lx(%s))->FRAME->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
        }
    }else if( canvas != NULL )
    {                                          /* destroy canvas here */
        w = canvas->w ;
LOCAL_DEBUG_OUT( "--DESTR Client(%lx(%s))->FRAME->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
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
            if( asw->internal )
            {
                XWindowAttributes attr ;
                if( XGetWindowAttributes( dpy, w, &attr ) )
                    attributes.event_mask |= attr.your_event_mask;
            }

            if (get_flags(Scr.Feel.flags, AppsBackingStore))
            {
                valuemask |= CWBackingStore;
                attributes.backing_store = WhenMapped;
            }
            XChangeWindowAttributes (dpy, w, valuemask, &attributes);
            XAddToSaveSet (dpy, w);

            register_aswindow( w, asw );
            canvas = create_ascanvas_container( w );
LOCAL_DEBUG_OUT( "++CREAT Client(%lx(%s))->CLIENT->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
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
            withdrawn_status.frame_size[i] = 0 ;
        clear_flags( withdrawn_status.flags, AS_Shaded|AS_Iconic );
        anchor2status( &withdrawn_status, asw->hints, &(asw->anchor));
        xwc.x = withdrawn_status.x ;
        xwc.y = withdrawn_status.y ;
        xwc.width = withdrawn_status.width ;
        xwc.height = withdrawn_status.height ;

LOCAL_DEBUG_OUT( "--DESTR Client(%lx(%s))->CLIENT->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
        destroy_ascanvas( &canvas );
        unregister_aswindow( w );

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
            attributes.cursor = Scr.Feel.cursors[DEFAULT];

            if ((ASWIN_HFLAGS(asw, AS_ClientIcon|AS_ClientIconPixmap) != AS_ClientIcon) ||
                 asw->hints == NULL || asw->hints->icon.window == None ||
                 !get_flags(Scr.Feel.flags, KeepIconWindows))
            { /* create windows */
                attributes.event_mask = AS_ICON_TITLE_EVENT_MASK;
                w = create_visual_window ( Scr.asv, Scr.Root, -10, -10, 1, 1, 0,
                                           InputOutput, valuemask, &attributes );
                canvas = create_ascanvas_container( w );
            }else
            { /* reuse client's provided window */
                attributes.event_mask = AS_ICON_EVENT_MASK;
                w = asw->hints->icon.window ;
                XChangeWindowAttributes (dpy, w, valuemask, &attributes);
                canvas = create_ascanvas_container( w );
            }
LOCAL_DEBUG_OUT( "++CREAT Client(%lx(%s))->ICON->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
            register_aswindow( w, asw );
        }
    }else if( canvas != NULL )
    {                                          /* destroy canvas here */
        w = canvas->w ;
LOCAL_DEBUG_OUT( "--DESTR Client(%lx(%s))->ICON->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
        destroy_ascanvas( &canvas );
        if( asw->hints && asw->hints->icon.window == w )
            unregister_aswindow( w );
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

    if( canvas &&
        ((reuse_icon_canvas && canvas != asw->icon_canvas) || !required) )
    {
        w = canvas->w ;
LOCAL_DEBUG_OUT( "--DESTR Client(%lx(%s))->ICONT->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
        destroy_ascanvas( &canvas );
        destroy_registered_window( w );
    }

    if( required )
    {
        if( reuse_icon_canvas )
            canvas = asw->icon_canvas ;
        else if( canvas == NULL )
        {                                      /* create canvas here */
			unsigned long valuemask;
			XSetWindowAttributes attributes;

            valuemask = CWBorderPixel | CWCursor | CWEventMask ;
            attributes.border_pixel = Scr.asv->black_pixel;
            attributes.cursor = Scr.Feel.cursors[DEFAULT];
            attributes.event_mask = AS_ICON_TITLE_EVENT_MASK;
            /* create windows */
            w = create_visual_window ( Scr.asv, Scr.Root, -10, -10, 1, 1, 0,
                                       InputOutput, valuemask, &attributes );

            register_aswindow( w, asw );
            canvas = create_ascanvas( w );
LOCAL_DEBUG_OUT( "++CREAT Client(%lx(%s))->ICONT->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
        }
    }

    return (asw->icon_title_canvas = canvas);
}

static ASImage*
get_window_icon_image( ASWindow *asw )
{
	ASImage *im = NULL ;
    if( ASWIN_HFLAGS( asw, AS_ClientIcon|AS_ClientIconPixmap ) == (AS_ClientIcon|AS_ClientIconPixmap) &&
        get_flags( Scr.Feel.flags, KeepIconWindows )&&
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
check_tbar( ASTBarData **tbar, Bool required, const char *mystyle_name,
            ASImage *img, unsigned short back_w, unsigned short back_h,
            int context )
{
    if( required )
    {
        if( *tbar == NULL )
        {
            *tbar = create_astbar();
LOCAL_DEBUG_OUT( "++CREAT tbar(%p)->context(%x)", *tbar, context );
        }else
            delete_astbar_tile( *tbar, -1 );

        set_astbar_style( *tbar, BAR_STATE_FOCUSED, mystyle_name );
        set_astbar_hilite( *tbar, DEFAULT_TBAR_HILITE );
        set_astbar_image( *tbar, img );
        set_astbar_back_size( *tbar, back_w, back_h );
        set_astbar_size( *tbar, (back_w == 0)?1:back_w, (back_h == 0)?1:back_h );
        (*tbar)->context = context ;
    }else if( *tbar )
    {
        destroy_astbar( tbar );
    }
    return *tbar;;
}

/******************************************************************************/
/* now externally available interfaces to the above functions :               */
/******************************************************************************/
void
invalidate_window_icon( ASWindow *asw )
{
    if( asw )
        check_icon_canvas( asw, False );
}

/***********************************************************************
 *  grab_aswindow_buttons - grab needed buttons for all of the windows
 *  for specified client
 ***********************************************************************/
void
grab_aswindow_buttons( ASWindow *asw, Bool focused )
{
    if( asw )
    {
        Bool do_focus_grab = (!focused && get_flags( Scr.Feel.flags, ClickToFocus ));
        if( do_focus_grab )
            grab_focus_click( asw->frame );
        else
            ungrab_window_buttons( asw->frame );

        ungrab_window_buttons( asw->w );
        grab_window_buttons ( asw->w, C_WINDOW);

        if( asw->icon_canvas )
        {
            ungrab_window_buttons( asw->icon_canvas->w );
            grab_window_buttons( asw->icon_canvas->w, C_ICON );
            if( do_focus_grab )
                grab_focus_click( asw->icon_canvas->w );
        }

        if( asw->icon_title_canvas && asw->icon_title_canvas != asw->icon_canvas )
        {
            ungrab_window_buttons( asw->icon_title_canvas->w );
            grab_window_buttons( asw->icon_title_canvas->w, C_ICON );
            if( do_focus_grab )
                grab_focus_click( asw->icon_title_canvas->w );
        }
    }
    XSync(dpy, False );
}

/***********************************************************************
 *  grab_aswindow_keys - grab needed keys for the window
 ***********************************************************************/
void
grab_aswindow_keys( ASWindow *asw )
{
    if( AS_ASSERT(asw) )
        return;

    ungrab_window_keys ( asw->frame );
    grab_window_keys (asw->frame, (C_WINDOW | C_TITLE | C_RALL | C_LALL | C_SIDEBAR));

    if( asw->icon_canvas )
    {
        ungrab_window_keys ( asw->icon_canvas->w );
        grab_window_keys (asw->icon_canvas->w, (C_ICON));
    }
    if( asw->icon_title_canvas && asw->icon_title_canvas != asw->icon_canvas )
    {
        ungrab_window_keys ( asw->icon_title_canvas->w );
        grab_window_keys (asw->icon_title_canvas->w, (C_ICON));
    }
    XSync(dpy, False );
}


/* this should set up proper feel settings - grab keyboard and mouse buttons :
 * Note: must be called after redecorate_window, since some of windows may have
 * been created at that time.
 */
void
grab_window_input( ASWindow *asw, Bool release_grab )
{
    if( asw )
    {
        if( release_grab )
        {
            ungrab_window_keys ( asw->frame );
            if( asw->icon_canvas )
                ungrab_window_keys ( asw->icon_canvas->w );
            if( asw->icon_title_canvas && asw->icon_title_canvas != asw->icon_canvas )
                ungrab_window_keys ( asw->icon_title_canvas->w );
            XSync(dpy, False );
        }else
        {
            grab_aswindow_buttons(asw, (Scr.Windows->focused==asw));
            grab_aswindow_keys(asw);
        }
    }
}

void
redecorate_window( ASWindow *asw, Bool free_resources )
{
    MyFrame *frame = NULL ;
    ASCanvas *tbar_canvas = NULL, *sidebar_canvas = NULL ;
    ASCanvas *left_canvas = NULL, *right_canvas = NULL ;
    Bool has_tbar = False ;
	int i ;
    char *mystyle_name = Scr.Look.MSWindow[BACK_FOCUSED]->name;
	ASImage *icon_image = NULL ;

    ASOrientation *od = get_orientation_data(asw);

    int *frame_contexts  = &(od->frame_contexts[0]);
LOCAL_DEBUG_OUT( "asw(%p)->free_res(%d)", asw, free_resources );
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
        check_tbar( &(asw->icon_title), False, NULL, NULL, 0, 0, C_NO_CONTEXT );
        check_tbar( &(asw->icon_button), False, NULL, NULL, 0, 0, C_NO_CONTEXT );
        check_tbar( &(asw->tbar), False, NULL, NULL, 0, 0, C_NO_CONTEXT );
		i = FRAME_PARTS ;
		while( --i >= 0 )
            check_tbar( &(asw->frame_bars[i]), False, NULL, NULL, 0, 0, C_NO_CONTEXT );

        check_side_canvas( asw, FR_W, False );
        check_side_canvas( asw, FR_E, False );
        check_side_canvas( asw, FR_S, False );
        check_side_canvas( asw, FR_N, False );

        check_icon_title_canvas( asw, False, False );
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
    check_icon_canvas( asw, (ASWIN_HFLAGS( asw, AS_Icon) && !get_flags(Scr.Feel.flags, SuppressIcons)) );
    /* 4) we need to prepare icon title window : */
    check_icon_title_canvas( asw, (ASWIN_HFLAGS( asw, AS_IconTitle) &&
                                  get_flags(Scr.Feel.flags, IconTitle) &&
                                  !get_flags(Scr.Feel.flags, SuppressIcons)),
                                        !get_flags(Scr.Look.flags, SeparateButtonTitle)&&
                                        (asw->icon_canvas!=NULL) );

    /* 5) we need to prepare windows for 4 frame decoration sides : */
    tbar_canvas = check_side_canvas( asw, od->tbar_side, has_tbar||myframe_has_parts(frame, FRAME_TOP_MASK) );
    sidebar_canvas = check_side_canvas( asw, od->sbar_side, myframe_has_parts(frame, FRAME_BTM_MASK) );
    left_canvas = check_side_canvas( asw, od->left_side, myframe_has_parts(frame, FRAME_LEFT_MASK) );
    right_canvas = check_side_canvas( asw, od->right_side, myframe_has_parts(frame, FRAME_RIGHT_MASK) );

    /* make sure all our decoration windows are mapped and in proper order: */
    XRaiseWindow( dpy, asw->w );
    XMapSubwindows (dpy, asw->frame);
    XMapWindow (dpy, asw->frame);

    /* 6) now we have to create bar for icon - if it is not client's animated icon */
	if( asw->icon_canvas )
		icon_image = get_window_icon_image( asw );
    check_tbar( &(asw->icon_button), (asw->icon_canvas != NULL), AS_ICON_MYSTYLE,
                icon_image, icon_image?icon_image->width:0, icon_image?icon_image->height:0,/* scaling icon image */
                C_IconButton );
	if( icon_image )
        safe_asimage_destroy( icon_image );
    /* 7) now we have to create bar for icon title (optional) */
    check_tbar( &(asw->icon_title), (asw->icon_title_canvas != NULL), AS_ICON_TITLE_MYSTYLE,
                NULL, 0, 0, C_IconTitle );
    /* 8) now we have to create actuall bars - for each frame element plus one for the titlebar */
    if( frame )
    {
        for( i = 0 ; i < FRAME_PARTS ; ++i )
        {
            ASImage *img = NULL ;
            unsigned int real_part = od->frame_rotation[i];

            img = frame->parts[i]?frame->parts[i]->image:NULL ;

            if( (0x01<<i)&MYFRAME_HOR_MASK )
            {
                *(od->in_width) = frame->part_length[i] ;
                *(od->in_height) = frame->part_width[i] ;
            }else
            {
                *(od->in_width) = frame->part_width[i] ;
                *(od->in_height) = frame->part_length[i] ;
            }
/*LOCAL_DEBUG_OUT( "part(%d)->real_part(%d)->from_size(%ux%u)->in_size(%ux%u)->out_size(%ux%u)", i, real_part, frame->part_width[i], frame->part_length[i], *(od->in_width), *(od->in_height), *(od->out_width), *(od->out_height) );*/
            check_tbar( &(asw->frame_bars[real_part]), IsFramePart(frame,i), mystyle_name,
                        img, *(od->out_width), *(od->out_height), frame_contexts[i] );
        }
    }
    check_tbar( &(asw->tbar), has_tbar, mystyle_name, NULL, 0, 0, C_TITLE );

    /* 9) now we have to setup titlebar buttons */
    if( asw->tbar )
	{ /* need to add some titlebuttons */
        ASFlagType title_align = ALIGN_LEFT ;
        ASFlagType btn_mask = compile_titlebuttons_mask (asw->hints);
        asw->tbar->h_spacing = DEFAULT_TBAR_SPACING ;
        asw->tbar->v_spacing = DEFAULT_TBAR_SPACING ;
        /* left buttons : */
        add_astbar_btnblock(asw->tbar,
                            od->default_tbar_elem_col[0], od->default_tbar_elem_row[0],
                            0, NO_ALIGN,
                            &(Scr.Look.buttons[0]), ~btn_mask,
                            TITLE_BUTTONS_PERSIDE,
                            Scr.Look.TitleButtonXOffset, Scr.Look.TitleButtonYOffset, Scr.Look.TitleButtonSpacing,
                            od->left_btn_order, C_L1 );
        /* label */
        if(Scr.Look.TitleTextAlign == JUSTIFY_RIGHT )
            title_align = ALIGN_RIGHT ;
        else if(Scr.Look.TitleTextAlign == JUSTIFY_CENTER )
            title_align = ALIGN_CENTER ;

        add_astbar_label( asw->tbar,
                          od->default_tbar_elem_col[1], od->default_tbar_elem_row[1],
                          od->flip, title_align, ASWIN_NAME(asw));
        /* right buttons : */
        add_astbar_btnblock(asw->tbar,
                            od->default_tbar_elem_col[2], od->default_tbar_elem_row[2],
                            0, NO_ALIGN,
                            &(Scr.Look.buttons[TITLE_BUTTONS_PERSIDE]), (~btn_mask)>>TITLE_BUTTONS_PERSIDE,
                            TITLE_BUTTONS_PERSIDE,
                            Scr.Look.TitleButtonXOffset, Scr.Look.TitleButtonYOffset, Scr.Look.TitleButtonSpacing,
                            od->right_btn_order, C_R1 );
	}

    /* we also need to setup label, unfocused/sticky style and tbar sizes -
     * it all is done when we change windows state, or move/resize it */
    /*since we might have destroyed/created some windows - we have to refresh grabs :*/
    grab_window_input( asw, False );
}

/* icon geometry relative to the root window :                      */
Bool get_icon_root_geometry( ASWindow *asw, ASRectangle *geom )
{
    ASCanvas *canvas = NULL ;
    if( AS_ASSERT(asw) || AS_ASSERT(geom) )
        return False;

    geom->height = 0 ;
    if( asw->icon_canvas )
    {
        canvas = asw->icon_canvas ;
        if( asw->icon_title_canvas && asw->icon_title_canvas != canvas )
            geom->height =  asw->icon_title_canvas->height ;
    }else if( asw->icon_title_canvas )
        canvas = asw->icon_title_canvas ;

    if( canvas )
    {
        geom->x = canvas->root_x ;
        geom->y = canvas->root_y ;
        geom->width  = canvas->width ;
        geom->height += canvas->height ;
        return True;
    }
    return False;
}

/* this gets called when Root background changes : */
void
update_window_transparency( ASWindow *asw )
{
/* TODO: */

}


static void
resize_canvases( ASWindow *asw, ASOrientation *od, unsigned int normal_width, unsigned int normal_height, unsigned int *frame_sizes )
{
    unsigned short tbar_size = frame_sizes[od->tbar_side] ;
    unsigned short sbar_size = frame_sizes[od->sbar_side] ;

    *(od->in_width)  = normal_width ;
    *(od->in_height) = tbar_size ;
    if( asw->frame_sides[od->tbar_side] )
        moveresize_canvas( asw->frame_sides[od->tbar_side], 0, 0, *(od->out_width), *(od->out_height) );

    *(od->in_x) = 0 ;
    *(od->in_y) = normal_height - sbar_size ;
    *(od->in_height) = sbar_size ;
    if( asw->frame_sides[od->sbar_side] )
        moveresize_canvas( asw->frame_sides[od->sbar_side], *(od->out_x), *(od->out_y), *(od->out_width), *(od->out_height) );

    /* for left and right sides - somewhat twisted logic - we mirror sides over lt2rb diagonal in case of
     * vertical title orientation. That allows us to apply simple x/y switching instead of complex
     * calculations. Note that we only do that for placement purposes. Contexts and images are still taken
     * from MyFrame parts as if it was rotated counterclockwise instead of mirrored.
     */
    *(od->in_x) = 0 ;
    *(od->in_y) = tbar_size ;
    *(od->in_width)  = frame_sizes[od->left_mirror_side] ;
    *(od->in_height) = normal_height-(tbar_size+sbar_size) ;
    if( asw->frame_sides[od->left_mirror_side] )
        moveresize_canvas( asw->frame_sides[od->left_mirror_side], *(od->out_x), *(od->out_y), *(od->out_width), *(od->out_height));

    *(od->in_x) = normal_width-frame_sizes[od->right_mirror_side] ;
    *(od->in_width)  = frame_sizes[od->right_mirror_side] ;
    if( asw->frame_sides[od->right_mirror_side] )
        moveresize_canvas( asw->frame_sides[od->right_mirror_side], *(od->out_x), *(od->out_y), *(od->out_width), *(od->out_height));
}

#if 0
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
#endif

static int
make_shade_animation_step( ASWindow *asw, ASOrientation *od )
{
    int step_size = 0;
    if( asw->tbar )
    {
        int steps = asw->shading_steps;

        if( steps > 0 )
        {
            int from_size, to_size ;
            *(od->in_width) = asw->frame_canvas->width ;
            *(od->in_height) = asw->frame_canvas->height ;
            from_size = *(od->out_height);
            if( ASWIN_GET_FLAGS( asw, AS_Shaded ) )
            {
                *(od->in_width) = asw->tbar->width ;
                *(od->in_height) = asw->tbar->height ;
            }else
            {
                *(od->in_width) = asw->status->width ;
                *(od->in_height) = asw->status->height ;
            }
            to_size = *(od->out_height);

            if(from_size != to_size)
            {
                int step_delta = (from_size - to_size)/steps ;
                if( step_delta == 0 )
                    step_delta = (from_size > to_size)?1:-1;
LOCAL_DEBUG_OUT( "@@ANIM to(%d)->from(%d)->delta(%d)->step(%d)", to_size, from_size, step_delta, steps );

                step_size = from_size-step_delta;
                --(asw->shading_steps);
            }
        }else if( !ASWIN_GET_FLAGS( asw, AS_Shaded ) )
            return 0;
        else
        {
            *(od->in_width) = asw->tbar->width ;
            *(od->in_height) = asw->tbar->height ;
            return *(od->out_height);
        }
    }
    return step_size;
}

inline static Bool
move_resize_frame_bar( ASTBarData *tbar, ASCanvas *canvas, int normal_x, int normal_y, unsigned int normal_width, unsigned int normal_height, Bool force_render )
{
    if( set_astbar_size( tbar, normal_width, normal_height) )
        force_render = True;
    if( move_astbar( tbar, canvas, normal_x, normal_y ) )
        force_render = True;
    if( force_render )
        render_astbar( tbar, canvas );
    return force_render;
}

inline static Bool
move_resize_corner( ASTBarData *bar, ASCanvas *canvas, ASOrientation *od, int normal_y, unsigned int normal_width,  Bool left, Bool force_render )
{
    unsigned int w = bar->width;
    unsigned int h = bar->height;

    *(od->in_y) = normal_y;
    /* Do we really need to rotate it ? */
    *(od->in_width)  = w ;
    *(od->in_height) = h ;

    *(od->in_x) = left?0:(normal_width-(*(od->out_width)));
    return move_resize_frame_bar( bar, canvas, *(od->out_x), *(od->out_y), w, h, force_render );
}

inline static Bool
move_resize_longbar( ASTBarData *bar, ASCanvas *canvas, ASOrientation *od,
                     int normal_y, unsigned int normal_width,
                     unsigned int corner_size1, unsigned int corner_size2,
                     Bool vertical, Bool force_render )
{
    unsigned int w = bar->width;
    unsigned int h = bar->height;
    int bar_size ;

    /* swapping bar height in case of vertical orientation : */
    *(od->in_width) = w ;
    *(od->in_height) = h ;
    if( vertical )
    {
        bar_size = *(od->out_width);
        *(od->in_width)  = bar_size ;
        *(od->in_height) = normal_width ;
        *(od->in_x) = normal_y;
        *(od->in_y) = corner_size1;
    }else
    {
        bar_size = *(od->out_height);
        *(od->in_height) = bar_size ;
        if( corner_size1+corner_size2 > normal_width )
            *(od->in_width) = 1;
        else
            *(od->in_width) = normal_width-(corner_size1+corner_size2) ;
        *(od->in_x) = corner_size1;
        *(od->in_y) = normal_y;
    }

    return move_resize_frame_bar( bar, canvas, *(od->out_x), *(od->out_y), *(od->out_width), *(od->out_height), force_render );
}

static Bool
move_resize_frame_bars( ASWindow *asw, int side, ASOrientation *od, unsigned int normal_width, unsigned int normal_height, Bool force_render )
{
    int corner_size1 = 0, corner_size2 = 0;
    Bool rendered = False ;
    int tbar_size = 0;
    ASCanvas *canvas = asw->frame_sides[side];
    ASTBarData *title = NULL, *corner1 = NULL, *longbar = NULL, *corner2 = NULL;
    Bool vertical = False ;

    longbar = asw->frame_bars[side];
    if( side == od->tbar_side )
    {
        title = asw->tbar ;
        corner1 = asw->frame_bars[od->tbar_mirror_corners[0]];
        corner2 = asw->frame_bars[od->tbar_mirror_corners[1]];
    }else if( side == od->sbar_side )
    {
        corner1 = asw->frame_bars[od->sbar_mirror_corners[0]];
        corner2 = asw->frame_bars[od->sbar_mirror_corners[1]];
    }else
        vertical = True ;

    if( title )
    {
        if( move_resize_longbar( title, canvas, od, 0, normal_width, 0, 0, False, force_render ) )
            rendered = True;
        tbar_size = *(od->out_height);
    }
    /* mirror_corner 0 */
    if( corner1 )
    {
        if( move_resize_corner( corner1, canvas, od, tbar_size, normal_width, True, force_render ) )
            rendered = True ;

        corner_size1 = *(od->out_width);
    }
    /* mirror_corner 1 */
    if( corner2 )
    {
        if( move_resize_corner( corner2, canvas, od, tbar_size, normal_width, False, force_render ) )
            rendered = True ;

        corner_size2 = *(od->out_width);
    }
    /* side */
    if( longbar )
    {
        if( move_resize_longbar( longbar, canvas, od,
                                 tbar_size, vertical?normal_height:normal_width,
                                 corner_size1, corner_size2,
                                 vertical, force_render ) )
            rendered = True;
    }
    return rendered;
}

static void
apply_window_status_size(register ASWindow *asw, ASOrientation *od)
{
    /* note that icons are handled by iconbox */
    if( !ASWIN_GET_FLAGS( asw, AS_Iconic ) )
	{
        int step_size = make_shade_animation_step( asw, od );
LOCAL_DEBUG_OUT( "**CONFG Client(%lx(%s))->status(%ux%u%+d%+d,%s,%s(%d>-%d))", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", asw->status->width, asw->status->height, asw->status->x, asw->status->y, ASWIN_HFLAGS(asw, AS_VerticalTitle)?"Vert":"Horz", step_size>0?"Shaded":"Unshaded", asw->shading_steps, step_size );
        if( step_size > 0 )
        {
            if( asw->frame_sides[od->tbar_side] )
                XRaiseWindow( dpy, asw->frame_sides[od->tbar_side]->w );
            XLowerWindow( dpy, asw->w );
            if( ASWIN_HFLAGS(asw, AS_VerticalTitle) )
                XMoveResizeWindow(  dpy, asw->frame,
                                    asw->status->x, asw->status->y,
                                    step_size, asw->status->height );
            else
                XMoveResizeWindow(  dpy, asw->frame,
                                    asw->status->x, asw->status->y,
                                    asw->status->width, step_size );
        }else
        {
            XRaiseWindow( dpy, asw->w );
            XMoveResizeWindow( dpy, asw->frame,
                            asw->status->x, asw->status->y,
                            asw->status->width, asw->status->height );
        }
    }
}

static void
on_frame_bars_moved( ASWindow *asw, unsigned int side, ASOrientation *od)
{
    ASCanvas *canvas = asw->frame_sides[side];

    update_astbar_transparency(asw->frame_bars[side], canvas);
    if( side == od->tbar_side )
    {
        update_astbar_transparency(asw->tbar, canvas);
        update_astbar_transparency(asw->frame_bars[od->tbar_mirror_corners[0]], canvas);
        update_astbar_transparency(asw->frame_bars[od->tbar_mirror_corners[1]], canvas);
    }else if( side == od->sbar_side )
    {
        update_astbar_transparency(asw->frame_bars[od->sbar_mirror_corners[0]], canvas);
        update_astbar_transparency(asw->frame_bars[od->sbar_mirror_corners[1]], canvas);
    }

}

static void
update_window_frame_moved( ASWindow *asw, ASOrientation *od )
{
    int i ;
    handle_canvas_config (asw->client_canvas);
    if( asw->internal && asw->internal->on_moveresize )
        asw->internal->on_moveresize( asw->internal, None );

    for( i = 0 ; i < FRAME_SIDES ; ++i )
        if( asw->frame_sides[i] )
        {   /* canvas has beer resized - resize tbars!!! */
            handle_canvas_config (asw->frame_sides[i]);
            on_frame_bars_moved( asw, i, od);
        }
}

void
SendConfigureNotify(ASWindow *asw)
{
    XEvent client_event ;

    client_event.type = ConfigureNotify;
    client_event.xconfigure.display = dpy;
    client_event.xconfigure.event = asw->w;
    client_event.xconfigure.window = asw->w;

    client_event.xconfigure.x = asw->client_canvas->root_x;
    client_event.xconfigure.y = asw->client_canvas->root_y;
    client_event.xconfigure.width = asw->client_canvas->width;
    client_event.xconfigure.height = asw->client_canvas->height;

    if (client_event.xconfigure.height <= 0)
        client_event.xconfigure.height = asw->hints->height_inc;

    if (client_event.xconfigure.width <= 0)
        client_event.xconfigure.width = asw->hints->width_inc;


    client_event.xconfigure.border_width = asw->status->border_width;
    /* Real ConfigureNotify events say we're above title window, so ... */
    /* what if we don't have a title ????? */
    client_event.xconfigure.above = asw->frame_canvas[FR_N].w?asw->frame_canvas[FR_N].w:asw->frame;
    client_event.xconfigure.override_redirect = False;
    XSendEvent (dpy, asw->w, False, StructureNotifyMask, &client_event);
}


/* this gets called when StructureNotify/SubstractureNotify arrives : */
void
on_window_moveresize( ASWindow *asw, Window w, int x, int y, unsigned int width, unsigned int height )
{
    int i ;
    Bool canvas_moved = False;
    ASOrientation *od ;
    unsigned int normal_width, normal_height ;

LOCAL_DEBUG_CALLER_OUT( "(%p,%lx,asw->w=%lx,%ux%u%+d%+d)", asw, w, asw->w, width, height, x, y );
    if( AS_ASSERT(asw) )
        return ;

    od = get_orientation_data(asw);
    *(od->in_width)  = width ;
    *(od->in_height) = height ;
    normal_width  = *(od->out_width)  ;
    normal_height = *(od->out_height) ;

    if( w == asw->w )
    {  /* simply update client's size and position */
        handle_canvas_config (asw->client_canvas);
        if( asw->internal && asw->internal->on_moveresize )
            asw->internal->on_moveresize( asw->internal, w );

        broadcast_config (M_CONFIGURE_WINDOW, asw);
    }else if( w == asw->frame )
    {/* resize canvases here :*/
        Bool resized = (width != asw->frame_canvas->width || height != asw->frame_canvas->height);
        Bool moved = handle_canvas_config (asw->frame_canvas);
        if( resized )
        {
            register unsigned int *frame_size = &(asw->status->frame_size[0]) ;
            int step_size = make_shade_animation_step( asw, od );
            if( step_size <= 0 )  /* don't moveresize client window while shading !!!! */
            {
                resize_canvases( asw, od, normal_width, normal_height, frame_size );
                if( !ASWIN_GET_FLAGS(asw, AS_Shaded ) )  /* leave shaded client alone ! */
                    moveresize_canvas( asw->client_canvas,
                                    frame_size[FR_W],
                                    frame_size[FR_N],
                                    width-(frame_size[FR_W]+frame_size[FR_E]),
                                    height-(frame_size[FR_N]+frame_size[FR_S]));
            }else if( normal_height != step_size )
            {
                sleep_a_little(10000);
                /* we get smoother animation if we move decoration ahead of actually
                 * resizing frame window : */
                resize_canvases( asw, od, normal_width, step_size, frame_size );
                *(od->in_width)=normal_width ;
                *(od->in_height)=step_size ;
/*LOCAL_DEBUG_OUT( "**SHADE Client(%lx(%s))->(%d>-%d)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", asw->shading_steps, step_size );*/
                XMoveResizeWindow(  dpy, asw->frame,
                                    x, y, *(od->out_width), *(od->out_height));
                ASSync(False);
            }
        }else if( moved )
        {
            update_window_frame_moved( asw, od );
            broadcast_config (M_CONFIGURE_WINDOW, asw);
            SendConfigureNotify(asw);
            /* also sent synthetic ConfigureNotify : */
        }

        if( moved || resized )
            update_window_transparency( asw );
    }else if( asw->icon_canvas && w == asw->icon_canvas->w )
    {
        canvas_moved = handle_canvas_config (asw->icon_canvas);
        if( canvas_moved )
        {
            unsigned short title_size = 0 ;
            if( asw->icon_title && asw->icon_title_canvas == asw->icon_canvas )
            {
                set_astbar_size( asw->icon_title, width, asw->icon_title->height );
                title_size = asw->icon_title->height ;
                render_astbar( asw->icon_title, asw->icon_canvas );
            }
            set_astbar_size( asw->icon_button, width, height-title_size );
            render_astbar( asw->icon_button, asw->icon_canvas );
            update_canvas_display( asw->icon_canvas );
        }
        broadcast_config (M_CONFIGURE_WINDOW, asw);
    }else if( asw->icon_title_canvas && w == asw->icon_title_canvas->w )
    {
        if( handle_canvas_config(asw->icon_title_canvas) && asw->icon_title )
        {
            set_astbar_size( asw->icon_title, width, asw->icon_title->height );
            render_astbar( asw->icon_title, asw->icon_title_canvas );
            update_canvas_display( asw->icon_title_canvas );
        }
        broadcast_config (M_CONFIGURE_WINDOW, asw);
    }else  /* one of the frame canvases :*/
    {
        Bool found = False;
        for( i = 0 ; i < FRAME_SIDES ; ++i )
            if( asw->frame_sides[i] && asw->frame_sides[i]->w == w )
            {   /* canvas has beer resized - resize tbars!!! */
                Bool canvas_moved = handle_canvas_config (asw->frame_sides[i]);
                /* don't redraw window decoration while in the middle of animation : */
                if( asw->shading_steps<= 0 )
                {
                    if( move_resize_frame_bars( asw, i, od, normal_width, normal_height, canvas_moved) ||
                        canvas_moved )  /* now we need to show them on screen !!!! */
                        update_canvas_display( asw->frame_sides[i] );
                }
                found = True;
                break;
            }
        if( !found )
            if( asw->internal && asw->internal->on_moveresize )
                asw->internal->on_moveresize( asw->internal, w );

    }
    ASSync(False);
}

void
on_window_title_changed( ASWindow *asw, Bool update_display )
{
    if( AS_ASSERT(asw) )
        return ;
    if( asw->tbar )
    {
        ASCanvas *canvas = ASWIN_HFLAGS(asw, AS_VerticalTitle)?asw->frame_sides[FR_W]:asw->frame_sides[FR_N];
        if( change_astbar_first_label( asw->tbar, ASWIN_NAME(asw) ) )
            if( canvas && update_display )
            {
                render_astbar( asw->tbar, canvas );
                update_canvas_display( canvas );
            }
    }
    if( asw->icon_title )
    {
        if( change_astbar_first_label( asw->icon_title, ASWIN_ICON_NAME(asw) ) )
            on_icon_changed( asw );
    }
}

void
on_window_status_changed( ASWindow *asw, Bool update_display, Bool reconfigured )
{
    char *unfocus_mystyle = NULL ;
    int i ;
    Bool changed = False;
    ASOrientation *od = get_orientation_data( asw );

    if( AS_ASSERT(asw) )
        return ;

LOCAL_DEBUG_CALLER_OUT( "(%p,%s Update display,%s Reconfigured)", asw, update_display?"":"Don't", reconfigured?"":"Not" );
    if( ASWIN_GET_FLAGS(asw, AS_Iconic ) )
    {
        unfocus_mystyle = ASWIN_GET_FLAGS(asw, AS_Sticky )?
                            AS_ICON_TITLE_STICKY_MYSTYLE:
                            AS_ICON_TITLE_UNFOCUS_MYSTYLE ;
        if( asw->icon_title )
            changed = set_astbar_style( asw->icon_title, BAR_STATE_UNFOCUSED, unfocus_mystyle );
        if( changed ) /* now we need to update icon title size */
            on_icon_changed( asw );
    }else
    {
        if( ASWIN_GET_FLAGS(asw, AS_Sticky ) )
        {
            unfocus_mystyle = asw->hints->mystyle_names[BACK_STICKY];
            if( unfocus_mystyle == NULL )
                unfocus_mystyle = Scr.Look.MSWindow[BACK_STICKY]->name ;
        }else
            unfocus_mystyle = asw->hints->mystyle_names[BACK_UNFOCUSED];

        if( unfocus_mystyle == NULL )
            unfocus_mystyle = Scr.Look.MSWindow[BACK_UNFOCUSED]->name ;

        for( i = 0 ; i < FRAME_PARTS ; ++i )
            if( asw->frame_bars[i] )
                if( set_astbar_style( asw->frame_bars[i], BAR_STATE_UNFOCUSED, unfocus_mystyle ) )
                    changed = True ;

        if( asw->tbar )
            if( set_astbar_style( asw->tbar, BAR_STATE_UNFOCUSED, unfocus_mystyle ) )
                changed = True ;
        if( changed || reconfigured )
        {/* now we need to update frame sizes in status */
            unsigned int *frame_size = &(asw->status->frame_size[0]) ;
            unsigned short tbar_size = 0;
            if( ASWIN_HFLAGS(asw, AS_VerticalTitle) )
            {
                tbar_size = calculate_astbar_width( asw->tbar );
                /* we need that to set up tbar size : */
                set_astbar_size( asw->tbar, tbar_size, asw->tbar?asw->tbar->height:0 );
            }else
            {
                tbar_size = calculate_astbar_height( asw->tbar );
                /* we need that to set up tbar size : */
                set_astbar_size( asw->tbar, asw->tbar?asw->tbar->width:0, tbar_size );
            }
            for( i = 0 ; i < FRAME_SIDES ; ++i )
            {
                if( asw->frame_bars[i] )
                    frame_size[i] = IsSideVertical(i)?asw->frame_bars[i]->width:
                                                    asw->frame_bars[i]->height ;
                else
                    frame_size[i] = 0;
            }
            frame_size[od->tbar_side] += tbar_size ;
            anchor2status ( asw->status, asw->hints, &(asw->anchor));
        }
    }

    /* now we need to move/resize our frame window */
    apply_window_status_size(asw, od);
    set_client_state( asw->w, asw->status );
}

void
on_window_hilite_changed( ASWindow *asw, Bool focused )
{
    ASOrientation *od = get_orientation_data( asw );

LOCAL_DEBUG_CALLER_OUT( "(%p,%s focused)", asw, focused?"":"not" );
    if( AS_ASSERT(asw) )
        return;

    broadcast_focus_change( asw, focused );

    if(!ASWIN_GET_FLAGS(asw, AS_Iconic))
    {
        register int i = FRAME_PARTS;
        /* Titlebar */
        set_astbar_focused( asw->tbar, asw->frame_sides[od->tbar_side], focused );
        /* frame decor : */
        for( i = FRAME_PARTS; --i >= 0; )
            set_astbar_focused( asw->frame_bars[i], asw->frame_sides[od->tbar2canvas_xref[i]], focused );
        /* now posting all the changes on display :*/
        for( i = FRAME_SIDES; --i >= 0; )
            if( is_canvas_dirty(asw->frame_sides[i]) )
                update_canvas_display( asw->frame_sides[i] );
        if( asw->internal && asw->internal->on_hilite_changed )
            asw->internal->on_hilite_changed( asw->internal, NULL, focused );
    }else /* Iconic !!! */
    {
        set_astbar_focused( asw->icon_button, asw->icon_canvas, focused );
        set_astbar_focused( asw->icon_title, asw->icon_title_canvas, focused );
        if( is_canvas_dirty(asw->icon_canvas) )
            update_canvas_display( asw->icon_canvas );
        if( is_canvas_dirty(asw->icon_title_canvas) )
            update_canvas_display( asw->icon_title_canvas );
    }
}

void
on_window_pressure_changed( ASWindow *asw, int pressed_context )
{
    ASOrientation *od = get_orientation_data( asw );
LOCAL_DEBUG_CALLER_OUT( "(%p,%s)", asw, context2text(pressed_context));

    if( AS_ASSERT(asw))
        return;

    if(!ASWIN_GET_FLAGS(asw, AS_Iconic))
    {
        register int i = FRAME_PARTS;
        /* Titlebar */
        set_astbar_btn_pressed (asw->tbar, pressed_context);  /* must go before next call to properly redraw :  */
        set_astbar_pressed( asw->tbar, asw->frame_sides[od->tbar_side], pressed_context&C_TITLE );
        /* frame decor : */
        for( i = FRAME_PARTS; --i >= 0; )
            set_astbar_pressed( asw->frame_bars[i], asw->frame_sides[od->tbar2canvas_xref[i]], pressed_context&(C_FrameN<<i) );
        /* now posting all the changes on display :*/
        for( i = FRAME_SIDES; --i >= 0; )
            if( is_canvas_dirty(asw->frame_sides[i]) )
                update_canvas_display( asw->frame_sides[i] );
        if( asw->internal && asw->internal->on_pressure_changed )
            asw->internal->on_pressure_changed( asw->internal, pressed_context&C_CLIENT );
    }else /* Iconic !!! */
    {
        set_astbar_pressed( asw->icon_button, asw->icon_canvas, pressed_context&C_IconButton );
        set_astbar_pressed( asw->icon_title, asw->icon_title_canvas, pressed_context&C_IconTitle );
        if( is_canvas_dirty(asw->icon_canvas) )
            update_canvas_display( asw->icon_canvas );
        if( is_canvas_dirty(asw->icon_title_canvas) )
            update_canvas_display( asw->icon_title_canvas );
    }
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

    for (btn = Scr.Feel.MouseButtonRoot; btn != NULL; btn = btn->NextButton)
		if (btn->Context & context)
		{
			TermDef      *fterm;

            fterm = FindTerm (&FuncSyntax, TT_ANY, btn->fdata->func);
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
        Scr.Look.buttons[b].width <= 0 || IsBtnDisabled(tmp_win, b ))
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
	ASHints *hints = t->hints ;
	ASFlagType tflags = hints->flags ;

#ifdef SHAPE
	if (t->wShaped)
	{
		tflags &= ~(AS_Border | AS_Frame);
		clear_flags(hints->function_mask,AS_FuncResize);
	}
#endif

#ifndef NO_TEXTURE
    if (!get_flags(hints->function_mask,AS_FuncResize) || !get_flags( Scr.Look.flags, DecorateFrames))
#endif /* !NO_TEXTURE */
	{
		/* a wide border, with corner tiles */
		if (get_flags(tflags,AS_Frame))
			clear_flags( t->hints->flags, AS_Frame );
	}
}

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
place_aswindow( ASWindow *asw, ASStatusHints *status )
{

    return True;
}

void
moveresize_aswindow_wm( ASWindow *asw, int x, int y, unsigned int width, unsigned int height )
{
    if( !AS_ASSERT(asw) )
    {
        ASStatusHints scratch_status  = *(asw->status);
        scratch_status.x = x ;
        scratch_status.y = y ;
        scratch_status.width = width ;
        scratch_status.height = height ;

        if( ASWIN_GET_FLAGS( asw, AS_Shaded ) )
        {    /* tricky tricky */
            if( ASWIN_HFLAGS(asw,AS_VerticalTitle ) )
                scratch_status.width = asw->status->width ;
            else
                scratch_status.height = asw->status->height ;
        }

        /* need to apply two-way conversion in order to make sure that size restrains are applied : */
        status2anchor( &(asw->anchor), asw->hints, &scratch_status, Scr.VxMax, Scr.VyMax);
        anchor2status ( asw->status, asw->hints, &(asw->anchor));

        /* now lets actually resize the window : */
        apply_window_status_size(asw, get_orientation_data(asw));
    }
}

Bool
init_aswindow_status( ASWindow *t, ASStatusHints *status )
{
	ASStatusHints adjusted_status ;

	if( t->status == NULL )
		t->status = safecalloc(1, sizeof(ASStatusHints));

    if( get_flags( status->flags, AS_StartDesktop) && status->desktop != Scr.CurrentDesk )
        ChangeDesks( status->desktop );
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
        MoveViewport (t->status->viewport_x, t->status->viewport_y, False);

    adjusted_status = *status ;

    if( !get_flags(AfterStepState, ASS_NormalOperation) )
        set_flags( adjusted_status.flags, AS_Position );
    else if( get_flags(Scr.Feel.flags, NoPPosition) &&
            !get_flags( status->flags, AS_StartPositionUser ) )
        clear_flags( adjusted_status.flags, AS_Position );

    if( get_flags( status->flags, AS_MaximizedX|AS_MaximizedY ))
        maximize_window_status( status, t->saved_status, &adjusted_status, status->flags );
    else if( !get_flags( adjusted_status.flags, AS_Position ))
        if( !place_aswindow( t, &adjusted_status ) )
            return False;

    if( is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
		print_status_hints( NULL, NULL, &adjusted_status );

    /* TODO: AS_Iconic */
	if( !ASWIN_GET_FLAGS(t, AS_StartLayer ) )
		ASWIN_LAYER(t) = AS_LayerNormal ;

    t->status->flags = adjusted_status.flags ;
    status2anchor( &(t->anchor), t->hints, &adjusted_status, Scr.VxMax, Scr.VyMax);

	return True;
}

/***************************************************************************************/
/* iconify/deiconify code :                                                            */
/***************************************************************************************/
Bool
iconify_window( ASWindow *asw, Bool iconify )
{
LOCAL_DEBUG_CALLER_OUT( "client = %p, iconify = %d", asw, iconify );

    if( AS_ASSERT(asw) )
        return False;
    if( (iconify?1:0) == (ASWIN_GET_FLAGS(asw, AS_Iconic )?1:0) )
        return False;

    broadcast_config(M_CONFIGURE_WINDOW, asw );

    if( iconify )
    {
        /*
         * Performing transition NormalState->IconicState
         * Prevent the receipt of an UnmapNotify, since we trigger any such event
         * as signal for transition to the Withdrawn state.
         */
		asw->DeIconifyDesk = ASWIN_DESK(asw);

LOCAL_DEBUG_OUT( "unmaping client window 0x%lX", (unsigned long)asw->w );
        quietly_unmap_window( asw->w, AS_CLIENT_EVENT_MASK );
        XUnmapWindow (dpy, asw->frame);


		/* this transient logic is kinda broken - we need to exepriment with it and
		   fix it eventually, less we want to end up with iconified transients
		   without icons
		 */
        broadcast_config (M_CONFIGURE_WINDOW, asw);

        if( !ASWIN_HFLAGS(asw, AS_Transient))
		{
            set_flags( asw->status->flags, AS_Iconic );
            asw->status->icon_window = None ;
            if( asw->icon_canvas )
                asw->status->icon_window = asw->icon_canvas->w ;
            else if( asw->icon_title_canvas )
                asw->status->icon_window = asw->icon_title_canvas->w ;
            set_client_state( asw->w, asw->status);

            add_iconbox_icon( asw );
            restack_window( asw, None, Below );

            if (get_flags(Scr.Feel.flags, ClickToFocus) || get_flags(Scr.Feel.flags, SloppyFocus))
			{
                if (asw == Scr.Windows->focused)
                    focus_next_aswindow (asw);
            }
		}
LOCAL_DEBUG_OUT( "updating status to iconic for client %p(\"%s\")", asw, ASWIN_NAME(asw) );
    }else
    {   /* Performing transition IconicState->NormalState  */
        clear_flags( asw->status->flags, AS_Iconic );
        remove_iconbox_icon( asw );
        if (get_flags(Scr.Feel.flags, StubbornIcons))
            ASWIN_DESK(asw) = asw->DeIconifyDesk;
        else
            ASWIN_DESK(asw) = Scr.CurrentDesk;
        set_client_desktop( asw->w, ASWIN_DESK(asw));

        /* TODO: make sure that the window is on this screen */

        XMapWindow (dpy, asw->w);
        if (ASWIN_DESK(asw) == Scr.CurrentDesk)
            XMapWindow (dpy, asw->frame);

        XRaiseWindow (dpy, asw->w);
        if( !ASWIN_HFLAGS(asw, AS_Transient ))
        {
            if (get_flags(Scr.Feel.flags, StubbornIcons|ClickToFocus))
                activate_aswindow (asw, True, False);
            else
                RaiseWindow (asw);
        }
        asw->status->icon_window = None ;
    }

    on_window_status_changed( asw, True, False );
    return True;
}

Bool
make_aswindow_visible( ASWindow *asw, Bool deiconify )
{
    if (asw == NULL)
        return False;

    if( ASWIN_GET_FLAGS( asw, AS_Iconic ) )
    {
        if( deiconify )
        {/* TODO: deiconify here */}
        else
            return False;
    }

    if (ASWIN_DESK(asw) != Scr.CurrentDesk)
        ChangeDesks( ASWIN_DESK(asw));

    /* TODO: need to to center on window */
    return True;
}

void
change_aswindow_layer( ASWindow *asw, int layer )
{
    if( AS_ASSERT(asw) )
        return;
    if( ASWIN_LAYER(asw) != layer )
    {
        ASLayer  *dst_layer = NULL, *src_layer ;

        src_layer = get_aslayer( ASWIN_LAYER(asw), Scr.Windows );
        ASWIN_LAYER(asw) = layer ;
        dst_layer = get_aslayer( ASWIN_LAYER(asw), Scr.Windows );

        vector_remove_elem( src_layer->members, &asw );
        /* inserting window into the top of the new layer */
        vector_insert_elem( dst_layer->members, &asw, 1, NULL, False );

        restack_window_list( ASWIN_DESK(asw), False );
    }
}

void quietly_reparent_aswindow( ASWindow *asw, Window dst, Bool user_root_pos )
{
    if( asw )
    {
        quietly_reparent_canvas( asw->frame_canvas, dst, AS_FRAME_EVENT_MASK, user_root_pos );
        quietly_reparent_canvas( asw->icon_canvas, dst, AS_ICON_EVENT_MASK, user_root_pos );
        if( asw->icon_title_canvas != asw->icon_canvas )
            quietly_reparent_canvas( asw->icon_title_canvas, dst, AS_ICON_TITLE_EVENT_MASK, user_root_pos );
    }
}

void change_aswindow_desktop( ASWindow *asw, int new_desk )
{
    int old_desk ;
    if( AS_ASSERT(asw) )
        return ;
    if( ASWIN_DESK(asw) == new_desk || ASWIN_GET_FLAGS(asw, AS_Sticky))
        return ;

    old_desk = ASWIN_DESK(asw) ;
    ASWIN_DESK(asw) = new_desk ;
    /* desktop changing : */
    if( new_desk == Scr.CurrentDesk )
    {
        quietly_reparent_aswindow( asw, Scr.Root, True );
    }else if( old_desk == Scr.CurrentDesk )
        quietly_reparent_aswindow( asw, Scr.ServiceWin, True );
    broadcast_config (M_CONFIGURE_WINDOW, asw);
}

void toggle_aswindow_status( ASWindow *asw, ASFlagType flags )
{
    ASFlagType on_flags, off_flags ;

    if( AS_ASSERT(asw) )
        return ;
    if( flags == 0 )
        return ;

    on_flags = (~(asw->status->flags))&flags ;
    off_flags = (asw->status->flags)&(~flags) ;
    asw->status->flags = on_flags|off_flags ;

    if( get_flags( flags, AS_Shaded ) )
        asw->shading_steps = Scr.Feel.ShadeAnimationSteps ;

    on_window_status_changed( asw, True, False );
    /* TODO: implement maximization !!!! */
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
        /*if (asw->icon_canvas != NULL)
            UpdateIconShape (asw); */
	}
#endif /* SHAPE */
}

/********************************************************************
 * hides focus for the screen.
 **********************************************************************/
void
hide_focus()
{
    if (get_flags(Scr.Feel.flags, ClickToFocus) && Scr.Windows->ungrabbed != NULL)
        grab_aswindow_buttons( Scr.Windows->ungrabbed, False );

    Scr.Windows->focused = NULL;
    Scr.Windows->ungrabbed = NULL;
    XSetInputFocus (dpy, Scr.ServiceWin, RevertToParent, Scr.last_Timestamp);
    XSync(dpy, False );
}

/********************************************************************
 * Sets the input focus to the indicated window.
 **********************************************************************/
Bool
focus_aswindow( ASWindow *asw, Bool circulated )
{
    Bool          do_hide_focus = False ;
    Bool          do_nothing = False ;
    Window        w = None;

    if( asw )
    {
        if (!circulated )
            if( vector_remove_elem( Scr.Windows->circulate_list, &asw ) == 1 )
                vector_insert_elem( Scr.Windows->circulate_list, &asw, 1, NULL, True );

#if 0
        /* ClickToFocus focus queue manipulation */
        if ( asw != Scr.Focus )
            asw->focus_sequence = Scr.next_focus_sequence++;
#endif
        do_hide_focus = (ASWIN_DESK(asw) != Scr.CurrentDesk) ||
                        (ASWIN_GET_FLAGS( asw, AS_Iconic ) &&
                            asw->icon_canvas == NULL && asw->icon_title_canvas == NULL );

        if( !ASWIN_HFLAGS(asw, AS_AcceptsFocus) )
        {
            if( Scr.Windows->focused != NULL && ASWIN_DESK(Scr.Windows->focused) == Scr.CurrentDesk )
                do_nothing = True ;
            else
                do_hide_focus = True ;
        }
    }else
        do_hide_focus = True ;

    if (Scr.NumberOfScreens > 1 && !do_hide_focus )
	{
        Window pointer_root ;
        /* if pointer went onto another screen - we need to release focus
         * and let other screen's manager manage it from now on, untill
         * pointer comes back to our screen :*/
        ASQueryPointerRoot(&pointer_root,&w);
        if(pointer_root != Scr.Root)
        {
            do_hide_focus = True;
            do_nothing = False ;
        }
    }
    if( !do_nothing && do_hide_focus )
        hide_focus();
    if( do_nothing || do_hide_focus )
        return False;

    if (get_flags(Scr.Feel.flags, ClickToFocus) && Scr.Windows->ungrabbed != asw)
    {  /* need to grab all buttons for window that we are about to
        * unfocus */
        grab_aswindow_buttons( Scr.Windows->ungrabbed, False );
        grab_aswindow_buttons( asw, True );
        Scr.Windows->ungrabbed = asw;
    }

    if( ASWIN_GET_FLAGS(asw, AS_Iconic ) )
    { /* focus icon window or icon title of the iconic window */
        if( asw->icon_canvas )
            w = asw->icon_canvas->w;
        else if( asw->icon_title_canvas )
            w = asw->icon_title_canvas->w;
    }else if( ASWIN_GET_FLAGS(asw, AS_Shaded ) )
    { /* focus frame window of shaded clients */
        w = asw->frame ;
    }else
    { /* clients with visible top window can get focus directly:  */
        w = asw->w ;
    }

    XSetInputFocus (dpy, w, RevertToParent, Scr.last_Timestamp);
    if (get_flags(asw->hints->protocols, AS_DoesWmTakeFocus))
        send_wm_protocol_request (asw->w, _XA_WM_TAKE_FOCUS, Scr.last_Timestamp);
    Scr.Windows->focused = asw ;

    XSync(dpy, False );
    return True;
}

/*********************************************************************/
/* focus management goes here :                                      */
/*********************************************************************/
/* making window active : */
/* handing over actuall focus : */
Bool
focus_active_window()
{
    /* don't fiddle with focus if we are in housekeeping mode !!! */
LOCAL_DEBUG_OUT( "checking if we are in housekeeping mode (%ld)", get_flags(AfterStepState, ASS_HousekeepingMode) );
    if( get_flags(AfterStepState, ASS_HousekeepingMode) || Scr.Windows->active == NULL )
        return False ;

    if( Scr.Windows->focused == Scr.Windows->active )
        return True ;                          /* already has focus */

    return focus_aswindow( Scr.Windows->active, False );
}

Bool
activate_aswindow( ASWindow *asw, Bool force, Bool deiconify )
{
    if (asw == NULL)
        return False;

    if( force )
    {
        GrabEm (&Scr, Scr.Feel.cursors[SELECT]);     /* to prevent Enter Notify events to
                                                      be sent to us while shifting windows around */
        if( !make_aswindow_visible( asw, deiconify ) )
            return False;
        Scr.Windows->active = asw ;   /* must do that prior to UngrabEm, so that window gets focused */
        UngrabEm ();
    }else
    {
        if( ASWIN_GET_FLAGS( asw, AS_Iconic ) )
        {
            if( deiconify )
            {/* TODO: deiconify here */}
            else
                return False;
        }
        if (ASWIN_DESK(asw) != Scr.CurrentDesk)
            return False;

        if( asw->status->x + asw->status->width < 0  || asw->status->x >= Scr.MyDisplayWidth ||
            asw->status->y + asw->status->height < 0 || asw->status->y >= Scr.MyDisplayHeight )
        {
            return False;                      /* we are out of screen - can't focus */
        }
        Scr.Windows->active = asw ;   /* must do that prior to UngrabEm, so that window gets focused */
        focus_active_window();
    }
    return True;
}

/* second version of above : */
void
focus_next_aswindow( ASWindow *asw )
{
    ASWindow     *new_focus = NULL;

    if( get_flags(Scr.Feel.flags, ClickToFocus))
        new_focus = get_next_window (asw, NULL, 1);
    if( !activate_aswindow( new_focus, False, False) )
        hide_focus();
}

void
hilite_aswindow( ASWindow *asw )
{
    if( Scr.Windows->hilited != asw )
    {
        if( Scr.Windows->hilited )
            on_window_hilite_changed (Scr.Windows->hilited, False);
        if( asw )
            on_window_hilite_changed (asw, True);
        Scr.Windows->hilited = asw ;
    }
}

void
hide_hilite()
{
    if( Scr.Windows->hilited != NULL )
    {
        on_window_hilite_changed (Scr.Windows->hilited, False);
        Scr.Windows->hilited = NULL ;
    }
}

void
press_aswindow( ASWindow *asw, int context )
{
    if( context == C_NO_CONTEXT )
    {
        if( Scr.Windows->pressed == asw )
            Scr.Windows->pressed = NULL;
    }else if( Scr.Windows->pressed != asw )
    {
        if( Scr.Windows->pressed != NULL )
            on_window_pressure_changed( Scr.Windows->pressed, C_NO_CONTEXT );
        Scr.Windows->pressed = asw ;
    }

    if( asw )
       on_window_pressure_changed( asw, context );
}

void
release_pressure()
{
    if( Scr.Windows->pressed != NULL )
    {
        on_window_pressure_changed (Scr.Windows->pressed, C_NO_CONTEXT);
        Scr.Windows->pressed = NULL ;
    }
}

void
warp_to_aswindow( ASWindow *asw, Bool deiconify )
{
    if( asw )
        activate_aswindow( asw, True, deiconify );
}



/*************************************************************************/
/* end of the focus management                                           */
/*************************************************************************/
/*************************************************************************/

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
	    if( t->saved_status )
    	    free( t->saved_status );
    	if( t->status )
        	free( t->status );
		if( t->hints )
		    destroy_hints( t->hints, False );
	}
    memset (t, 0x00, sizeof (ASWindow));
    t->magic = MAGIC_ASWINDOW ;
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
	ASRawHints    raw_hints ;
    ASHints      *hints  = NULL;
    ASStatusHints status;


	/* allocate space for the afterstep window */
	tmp_win = safecalloc (1, sizeof (ASWindow));
    init_aswindow( tmp_win, False );

    tmp_win->w = w;
    if (validate_drawable(w, NULL, NULL) == None)
	{
		free ((char *)tmp_win);
		return (NULL);
	}

    if( collect_hints( &Scr, w, HINT_ANY, &raw_hints ) )
    {
        if( is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
            print_hints( NULL, NULL, &raw_hints );
        hints = merge_hints( &raw_hints, Database, &status, Scr.Look.supported_hints, HINT_ANY, NULL );
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

    if( !init_aswindow_status( tmp_win, &status ) )
	{
		Destroy (tmp_win, False);
		return NULL;
	}

    /*
	 * Make sure the client window still exists.  We don't want to leave an
	 * orphan frame window if it doesn't.  Since we now have the server
	 * grabbed, the window can't disappear later without having been
	 * reparented, so we'll get a DestroyNotify for it.  We won't have
	 * gotten one for anything up to here, however.
	 */
	XGrabServer (dpy);
    if (validate_drawable(tmp_win->w, NULL, NULL) == None)
	{
		destroy_hints(tmp_win->hints, False);
		free ((char *)tmp_win);
		XUngrabServer (dpy);
		return (NULL);
	}
    XSetWindowBorderWidth (dpy, tmp_win->w, 0);

	/* add the window into the afterstep list */
    enlist_aswindow( tmp_win );

    redecorate_window       ( tmp_win, False );
    on_window_title_changed ( tmp_win, False );
    on_window_status_changed( tmp_win, False, True );

    /*
	 * Reparenting generates an UnmapNotify event, followed by a MapNotify.
	 * Set the map state to FALSE to prevent a transition back to
	 * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
	 * again in HandleMapNotify.
     */
    RaiseWindow (tmp_win);
	XUngrabServer (dpy);

    broadcast_config (M_ADD_WINDOW, tmp_win);

    broadcast_window_name( tmp_win );
    broadcast_res_names( tmp_win );
    broadcast_icon_name( tmp_win );

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
    InstallWindowColormaps (tmp_win);
	if (!ASWIN_HFLAGS(tmp_win, AS_SkipWinList))
		update_windowList ();

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

    if( !init_aswindow_status( tmp_win, status ) )
	{
		Destroy (tmp_win, False);
		return NULL;
	}

    XGrabServer (dpy);
    XSetWindowBorderWidth (dpy, tmp_win->w, 0);

	/* add the window into the afterstep list */
    enlist_aswindow( tmp_win );

    redecorate_window       ( tmp_win, False );

    /* internal windows must not be mapped prior to getting here ! */
    XMapWindow( dpy, w );

    on_window_title_changed ( tmp_win, False );
    on_window_status_changed( tmp_win, False, True );

    /*
	 * Reparenting generates an UnmapNotify event, followed by a MapNotify.
	 * Set the map state to FALSE to prevent a transition back to
	 * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
	 * again in HandleMapNotify.
     */
    RaiseWindow (tmp_win);
	XUngrabServer (dpy);

    broadcast_config (M_ADD_WINDOW, tmp_win);

    broadcast_window_name( tmp_win );
    broadcast_res_names( tmp_win );
    broadcast_icon_name( tmp_win );

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

    if (!ASWIN_HFLAGS(tmp_win, AS_SkipWinList))
		update_windowList ();

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

    XUnmapWindow (dpy, asw->frame);
    XRemoveFromSaveSet (dpy, asw->w);
    XSelectInput (dpy, asw->w, NoEventMask);
    XSync (dpy, 0);

    SendPacket (-1, M_DESTROY_WINDOW, 3, asw->w, asw->frame, (unsigned long)asw);

    UninstallWindowColormaps( asw );

    if ( asw == Scr.Windows->focused )
        focus_next_aswindow( asw );

    if ( asw == Scr.Windows->hilited )
        Scr.Windows->hilited = NULL;

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
    delist_aswindow( asw );

    init_aswindow( asw, True );

    memset( asw, 0x00, sizeof(ASWindow));
    free (asw);

	XSync (dpy, 0);
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
    }
    /*
     * Prevent the receipt of an UnmapNotify, since that would
     * cause a transition to the Withdrawn state.
     */
    XSelectInput (dpy, asw->w, NoEventMask);
    XGrabServer( dpy );
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
    XUngrabServer( dpy ) ;
}

/**********************************************************************/
/* The End                                                            */
/**********************************************************************/

