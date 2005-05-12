/*
 * Copyright (C) 2003 Sasha Vasko
 * Copyright (C) 1996 Frank Fejes
 * Copyright (C) 1996 Alfredo Kojima
 * Copyright (C) 1995 Bo Yang
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

#define LOCAL_DEBUG

#include "../../configure.h"

#include "asinternals.h"

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
    TBTN_ORDER_L2R,TBTN_ORDER_L2R,
    &NormalX, &NormalY, &NormalWidth, &NormalHeight,
    &NormalX, &NormalY, &NormalWidth, &NormalHeight,
    0,
    {0, 1, 2, 3, 4, 5, 6},
    {0, 0, 0, 0, 0, 0, 0}
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
    TBTN_ORDER_B2T,TBTN_ORDER_B2T,
    &NormalX, &NormalY, &NormalWidth, &NormalHeight,
    &NormalY, &NormalX, &NormalHeight, &NormalWidth,
    FLIP_VERTICAL,
    {0, 0, 0, 0, 0, 0, 0},
    {7, 6, 5, 4, 3, 2, 1}
};

ASOrientation*
get_orientation_data( ASWindow *asw )
{
    if( asw && asw->magic == MAGIC_ASWINDOW && asw->hints )
        return ASWIN_HFLAGS(asw, AS_VerticalTitle)?&VertOrientation:&HorzOrientation;
    return &HorzOrientation;
}

void 
compile_tbar_layout( unsigned int *tbar_layout, unsigned int *default_tbar_layout, unsigned long left_layout, unsigned long right_layout )
{
	int l, r; 	

	Bool used[MYFRAME_TITLE_BACKS] = {False, False, False, False, False, False, False };

	LOCAL_DEBUG_OUT( "left_layout = %lX, right_layout = %lX", left_layout, right_layout );
	for( l = 0 ; l < MYFRAME_TITLE_BACKS ; ++l )
		tbar_layout[l] = -1 ;
	for( l = 0 ; l < MYFRAME_TITLE_SIDE_ELEMS ; ++l ) 
	{
		int elem =  MYFRAME_GetTbarLayoutElem(left_layout,l);
		
		if( elem != MYFRAME_TITLE_BACK_INVALID && !used[l] )
		{	
			tbar_layout[elem] = default_tbar_layout[l] ; 
			used[l] = True ;
		}
	}
	for( r = MYFRAME_TITLE_BACKS ; r > MYFRAME_TITLE_SIDE_ELEMS+1 ; ) 
	{
		int elem =  MYFRAME_GetTbarLayoutElem(right_layout,(MYFRAME_TITLE_BACKS-r));
		--r ;
		LOCAL_DEBUG_OUT( "r = %d, elem = %d, used = %d", r, elem, used[r] );
		if( elem != MYFRAME_TITLE_BACK_INVALID && !used[r] )
		{	
			/* right layout is in reverse order ! */
			elem = MYFRAME_TITLE_BACKS - MYFRAME_TITLE_SIDE_ELEMS + elem;
			tbar_layout[elem] = default_tbar_layout[r] ; 
			LOCAL_DEBUG_OUT( "right_elem %d has slot = %d", elem, tbar_layout[elem] );
			used[r] = True ;
		}
	}
	
	tbar_layout[MYFRAME_TITLE_BACK_LBL] = default_tbar_layout[MYFRAME_TITLE_BACK_LBL];
	used[MYFRAME_TITLE_BACK_LBL] = True ;
	for( l = 0 ; l < MYFRAME_TITLE_BACKS ; ++l )
	{	
		if( tbar_layout[l] == -1 )
		{
	 		int i ;
			int from = 0, to = MYFRAME_TITLE_SIDE_ELEMS;
			if( l >= MYFRAME_TITLE_SIDE_ELEMS ) 
			{
				from = 	MYFRAME_TITLE_BACK_LEFT2RIGHT(MYFRAME_TITLE_SIDE_ELEMS);
				to   = 	MYFRAME_TITLE_BACKS ;
			}	 
			LOCAL_DEBUG_OUT( "missing layout for elem %d, from = %d, to = %d", l, from, to );
			for( i = from ; i < to ; ++i ) 
			{
				if( !used[i] ) 
				{	
					LOCAL_DEBUG_OUT( "found vacant slot %d", default_tbar_layout[i] );
					tbar_layout[l] = default_tbar_layout[i];
					used[i] = True;
					break;
				}		 
			}		  
		}	 
	}
}

/* this gets called when Look changes or hints changes : */
static ASCanvas *
check_side_canvas( ASWindow *asw, FrameSide side, Bool required )
{
    ASCanvas *canvas = asw->frame_sides[side];
	Window w;
	LOCAL_DEBUG_CALLER_OUT( "asw = %p, side = %d, required = %d", asw, side, required );
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
        } else
			invalidate_canvas_config( canvas );
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
			int bw = 0;

            /* create windows */
            valuemask = CWBorderPixel | CWCursor | CWEventMask ;
            if( Scr.asv->visual_info.visual == DefaultVisual( dpy, Scr.screen ) )
            {
			/* only if root has same depth and visual as us! */
                attributes.background_pixmap = ParentRelative;
                valuemask |= CWBackPixmap;
            }
            attributes.border_pixel = Scr.asv->black_pixel;
            attributes.cursor = Scr.Feel.cursors[ASCUR_Default];
            attributes.event_mask = AS_FRAME_EVENT_MASK;

            if (get_flags(Scr.Feel.flags, SaveUnders))
            {
                valuemask |= CWSaveUnder;
                attributes.save_under = True;
            }
			if( asw->hints && get_flags(asw->hints->flags, AS_Border)) 
				bw = asw->hints->border_width ;
			asw->status->frame_border_width = bw ;
            w = create_visual_window (Scr.asv, (ASWIN_DESK(asw)==Scr.CurrentDesk)?Scr.Root:Scr.ServiceWin, 
									  0, 0, 
									  asw->status->width, asw->status->height,
                                      bw, InputOutput, valuemask, &attributes);
			XLowerWindow( dpy, w );
            asw->frame = w ;
            register_aswindow( w, asw );
            canvas = create_ascanvas_container( w );
LOCAL_DEBUG_OUT( "++CREAT Client(%lx(%s))->FRAME->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
        } else
			invalidate_canvas_config( canvas );
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

            attributes.event_mask = AS_CLIENT_EVENT_MASK;
            if( asw->internal )
			{
	            XWindowAttributes internal_attr ;
                XGetWindowAttributes( dpy, w, &internal_attr );
                attributes.event_mask |= internal_attr.your_event_mask;
			}
		    quietly_reparent_window( w, asw->frame, 0, 0, attributes.event_mask );

            valuemask = (CWEventMask | CWDontPropagate );
            attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;

            if (get_flags(Scr.Feel.flags, AppsBackingStore))
            {
                valuemask |= CWBackingStore;
                attributes.backing_store = WhenMapped;
            }
            XChangeWindowAttributes (dpy, w, valuemask, &attributes);
            if( asw->internal == NULL )
                XAddToSaveSet (dpy, w);

            register_aswindow( w, asw );
            canvas = create_ascanvas_container( w );
			if( ASWIN_GET_FLAGS( asw, AS_Shaped ) )
				refresh_container_shape( canvas );
LOCAL_DEBUG_OUT( "++CREAT Client(%lx(%s))->CLIENT->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
        }
		/*else    invalidate_canvas_config( canvas ); */
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

        /*
         * Prevent the receipt of an UnmapNotify in case we are simply restarting,
         * since that would cause a transition to the Withdrawn state.
		 */
#if 1
        if( !ASWIN_GET_FLAGS(asw, AS_Dead) && get_parent_window( w ) == asw->frame )
        {
            if( get_flags( AfterStepState, ASS_Shutdown ) )
                quietly_reparent_window( w, Scr.Root, xwc.x, xwc.y, AS_CLIENT_EVENT_MASK );
            else
                XReparentWindow (dpy, w, Scr.Root, xwc.x, xwc.y);
            /* WE have to restore window's withdrawn location now. */
            XConfigureWindow (dpy, w, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &xwc);
        }
        XSync (dpy, 0);
#endif
    }

	asw->client_canvas = canvas;
    return canvas;
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
            attributes.cursor = Scr.Feel.cursors[ASCUR_Default];

            if ((ASWIN_HFLAGS(asw, AS_ClientIcon|AS_ClientIconPixmap) != AS_ClientIcon) ||
                 asw->hints == NULL || asw->hints->icon.window == None ||
                 !get_flags(Scr.Feel.flags, KeepIconWindows))
            { /* create windows */
                attributes.event_mask = AS_ICON_TITLE_EVENT_MASK;
                w = create_visual_window ( Scr.asv, (ASWIN_DESK(asw)==Scr.CurrentDesk)?Scr.Root:Scr.ServiceWin, -10, -10, 1, 1, 0,
                                           InputOutput, valuemask, &attributes );
                canvas = create_ascanvas( w );
            }else
            { /* reuse client's provided window */
                attributes.event_mask = AS_ICON_EVENT_MASK;
                w = asw->hints->icon.window ;
                XChangeWindowAttributes (dpy, w, valuemask, &attributes);
                canvas = create_ascanvas_container( w );
            }
LOCAL_DEBUG_OUT( "++CREAT Client(%lx(%s))->ICON->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
            register_aswindow( w, asw );
        }else
			invalidate_canvas_config( canvas );
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
	asw->icon_canvas = canvas;

	return canvas;
}

/* creating/destroying our icon title window : */
static ASCanvas *
check_icon_title_canvas( ASWindow *asw, Bool required, Bool reuse_icon_canvas )
{
    ASCanvas *canvas = asw->icon_title_canvas;
	Window w;

	if( !required ) 
	{	
    	if( canvas && canvas != asw->icon_canvas )
    	{
        	w = canvas->w ;
			LOCAL_DEBUG_OUT( "--DESTR Client(%lx(%s))->ICONT->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
        	destroy_ascanvas( &canvas );
        	destroy_registered_window( w );
		}
    }else  /* if( required ) */
    {
        if( reuse_icon_canvas )
            canvas = asw->icon_canvas ;
        else if( canvas == NULL )
        {                                      /* create canvas here */
			unsigned long valuemask;
			XSetWindowAttributes attributes;

            valuemask = CWBorderPixel | CWCursor | CWEventMask ;
            attributes.border_pixel = Scr.asv->black_pixel;
            attributes.cursor = Scr.Feel.cursors[ASCUR_Default];
            attributes.event_mask = AS_ICON_TITLE_EVENT_MASK;
            /* create windows */
            w = create_visual_window ( Scr.asv, (ASWIN_DESK(asw)==Scr.CurrentDesk)?Scr.Root:Scr.ServiceWin, -10, -10, 1, 1, 0,
                                       InputOutput, valuemask, &attributes );

            register_aswindow( w, asw );
            canvas = create_ascanvas( w );
LOCAL_DEBUG_OUT( "++CREAT Client(%lx(%s))->ICONT->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
        }else
			invalidate_canvas_config( canvas );
    }
	
	asw->icon_title_canvas = canvas;

    return canvas;
}

ASImage*
get_window_icon_image( ASWindow *asw )
{
	ASImage *im = NULL ;
    if( asw )
    {
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
            LOCAL_DEBUG_OUT( "converted client's pixmap into an icon %dx%d %p", width, height, im );
        }
        /* TODO: we also need to check for newfashioned ARGB icon from
        * extended WM specs here
        */
        if( im == NULL )
        {
            if( asw->hints->icon_file )
            {
                im = get_asimage( Scr.image_manager, asw->hints->icon_file, 0xFFFFFFFF, 100 );
                LOCAL_DEBUG_OUT( "loaded icon from \"%s\" into %dx%d %p", asw->hints->icon_file, im?im->width:0, im?im->height:0, im );
            }else
            {
                LOCAL_DEBUG_OUT( "no icon to use %s", "" );
            }
        }
    }
	return im;
}

static ASTBarData*
check_tbar( ASTBarData **tbar, Bool required, const char *mystyle_name,
            ASImage *img, unsigned short back_w, unsigned short back_h,
            int flip, ASFlagType align,
            ASFlagType fbevel, ASFlagType ubevel,
            unsigned char fcm, unsigned char ucm,
            int context )
{
    if( required )
    {
        if( *tbar == NULL )
        {
            *tbar = create_astbar();
LOCAL_DEBUG_OUT( "++CREAT tbar(%p)->context(%s)", *tbar, context2text(context) );
        }else
            delete_astbar_tile( *tbar, -1 );

		set_astbar_flip( *tbar, flip );   

        set_astbar_style( *tbar, BAR_STATE_FOCUSED, mystyle_name );
        set_astbar_style( *tbar, BAR_STATE_UNFOCUSED, "default" );
        if( img )
        {
            LOCAL_DEBUG_OUT("adding bar icon %p %ux%u", img, img->width, img->height );
            add_astbar_icon( *tbar, 0, 0, flip, align, img );
            if( back_w == 0 )
                back_w = get_flags(flip,FLIP_VERTICAL)?img->height:img->width ;
            if( back_h == 0 )
                back_h = get_flags(flip,FLIP_VERTICAL)?img->width:img->height ;
        }

        set_astbar_hilite( *tbar, BAR_STATE_FOCUSED, fbevel );
        set_astbar_hilite( *tbar, BAR_STATE_UNFOCUSED, ubevel );
        set_astbar_composition_method( *tbar, BAR_STATE_FOCUSED, fcm );
        set_astbar_composition_method( *tbar, BAR_STATE_UNFOCUSED, ucm );
        set_astbar_size( *tbar, (back_w == 0)?1:back_w, (back_h == 0)?1:back_h );
        (*tbar)->context = context ;
    }else if( *tbar )
    {
        destroy_astbar( tbar );
    }
    return *tbar;
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
        Bool do_focus_grab = (get_flags( Scr.Feel.flags, ClickToFocus ));
        if( do_focus_grab )
		{	
            if( focused )
				ungrab_focus_click( asw->frame );
			else				
				grab_focus_click( asw->frame );
        }else
            ungrab_window_buttons( asw->frame );

        if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
        {
            ungrab_window_buttons( asw->w );
            grab_window_buttons ( asw->w, C_WINDOW);
        }

        if( asw->icon_canvas && !ASWIN_GET_FLAGS(asw, AS_Dead) && validate_drawable(asw->icon_canvas->w, NULL, NULL) != None )
        {
            ungrab_window_buttons( asw->icon_canvas->w );
            if( do_focus_grab )
			{
	            if( focused )
					ungrab_focus_click( asw->icon_canvas->w );
				else				
			        grab_focus_click( asw->icon_canvas->w );
			}
            grab_window_buttons( asw->icon_canvas->w, C_ICON );
        }

        if( asw->icon_title_canvas && asw->icon_title_canvas != asw->icon_canvas )
        {
            ungrab_window_buttons( asw->icon_title_canvas->w );
            if( do_focus_grab )
			{
	            if( focused )
					ungrab_focus_click( asw->icon_title_canvas->w );
				else				
	                grab_focus_click( asw->icon_title_canvas->w );
			}
            grab_window_buttons( asw->icon_title_canvas->w, C_ICON );
    
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
    grab_window_keys (asw->frame, (C_WINDOW | C_TITLE | C_TButtonAll | C_FRAME));

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
			ungrab_window_buttons( asw->frame );
            if( asw->icon_canvas )
			{	
                ungrab_window_keys ( asw->icon_canvas->w );
				ungrab_window_buttons( asw->icon_canvas->w );
            }
			if( asw->icon_title_canvas && asw->icon_title_canvas != asw->icon_canvas )
			{	
                ungrab_window_keys ( asw->icon_title_canvas->w );
				ungrab_window_buttons( asw->icon_title_canvas->w );
			}
            XSync(dpy, False );
        }else
        {
            grab_aswindow_buttons(asw, (Scr.Windows->focused==asw));
            grab_aswindow_keys(asw);
        }
    }
}

/*
 ** returns a newline delimited list of the Mouse functions bound to a
 ** given context, in human readable form
 */
char         *
list_functions_by_context (int context, ASHints *hints )
{
	MouseButton  *btn;
	char         *str = NULL;
	int           allocated_bytes = 0;
	if( hints == NULL )
		return NULL ;
    for (btn = Scr.Feel.MouseButtonRoot; btn != NULL; btn = btn->NextButton)
		if ( (btn->Context & context) && check_allowed_function (btn->fdata, hints ) )
		{
			TermDef      *fterm;

            fterm = func2fterm(btn->fdata->func, True);
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



/****************************************************************************
 *
 * Checks the function "function", and sees if it
 * is an allowed function for window t,  according to the motif way of life.
 * This routine is used to decide if we should refuse to perform a function.
 *
 ****************************************************************************/
int
check_allowed_function2 (int func, ASHints *hints)
{
	if (func == F_NOP)
		return 0;

    if( hints )
	{
        int mask = function2mask( func );
        if( func == F_SHADE )
            return get_flags( hints->flags, AS_Titlebar ) ;
        else if( mask != 0 )
            return get_flags( hints->function_mask, mask );
    }
	return 1;
}
/****************************************************************************
 *
 * Checks the function described in menuItem mi, and sees if it
 * is an allowed function for window Tmp_Win,
 * according to the motif way of life.
 *
 * This routine is used to determine whether or not to grey out menu items.
 *
 ****************************************************************************/
int
check_allowed_function (FunctionData *fdata, ASHints *hints)
{
    int func = fdata->func ;
    int i ;
    ComplexFunction *cfunc ;

    if (func != F_FUNCTION)
        return check_allowed_function2 (func, hints);

    if( (cfunc = get_complex_function( fdata->name ) ) == NULL )
        return 0;

    for( i = 0 ; i < cfunc->items_num ; ++i )
        if( cfunc->items[i].func == F_FUNCTION )
        {
            if( check_allowed_function (&(cfunc->items[i]), hints) == 0 )
                return 0;
        }else if( check_allowed_function2 (cfunc->items[i].func, hints) == 0 )
            return 0;
    return 1;
}


ASFlagType
compile_titlebuttons_mask (ASHints *hints)
{
    ASFlagType disabled_mask = hints->disabled_buttons ;
    ASFlagType enabled_mask = 0 ;
    int        i;

    LOCAL_DEBUG_OUT( "disabled mask from hints is 0x%lX", disabled_mask );

	for (i = 0; i < TITLE_BUTTONS; i++)
	{
        if (Scr.Look.buttons[i].unpressed.image != NULL )
        {
            int context = Scr.Look.buttons[i].context;
            MouseButton  *func;
			Bool at_least_one_enabled = False ;

            enabled_mask |= context ;
            for (func = Scr.Feel.MouseButtonRoot; func != NULL; func = (*func).NextButton)
                if ( (func->Context & context) != 0 )
                    if( check_allowed_function (func->fdata,hints ) )
					{
						at_least_one_enabled = True ;
						break;
					}
			if( !at_least_one_enabled )
	            disabled_mask |= Scr.Look.buttons[i].context ;
        }
    }
    LOCAL_DEBUG_OUT( "disabled mask(0x%lX)->enabled_mask(0x%lX)->button_mask(0x%lX)", disabled_mask, enabled_mask, enabled_mask&(~disabled_mask) );


    return enabled_mask&(~disabled_mask);
}

void
estimate_titlebar_size( ASHints *hints, unsigned int *width_ret, unsigned int *height_ret )
{
    unsigned int width = 0, height = 0 ;
    if( hints )
    {
        ASTBarData *tbar = create_astbar();
        ASFlagType btn_mask ; 

		btn_mask = compile_titlebuttons_mask (hints);
        tbar->h_spacing = DEFAULT_TBAR_HSPACING ;
        tbar->v_spacing = DEFAULT_TBAR_VSPACING ;
        /* left buttons : */
        add_astbar_btnblock(tbar, 0, 0, 0, NO_ALIGN,
                            &(Scr.Look.ordered_buttons[0]), btn_mask,
                            Scr.Look.button_first_right,
                            Scr.Look.TitleButtonXOffset[0], Scr.Look.TitleButtonYOffset[0], Scr.Look.TitleButtonSpacing[0],
                            TBTN_ORDER_L2R );
      
		/* label */
        add_astbar_label(   tbar, 1, 0, 0, ALIGN_LEFT, DEFAULT_TBAR_HSPACING, DEFAULT_TBAR_VSPACING, hints->names[0], hints->names_encoding[0]);
        /* right buttons : */
        add_astbar_btnblock(tbar, 2, 0, 0, NO_ALIGN,
                            &(Scr.Look.ordered_buttons[Scr.Look.button_first_right]), btn_mask,
                            TITLE_BUTTONS-Scr.Look.button_first_right,
                            Scr.Look.TitleButtonXOffset[1], Scr.Look.TitleButtonYOffset[1], Scr.Look.TitleButtonSpacing[1],
                            TBTN_ORDER_R2L );

        set_astbar_style_ptr( tbar, BAR_STATE_UNFOCUSED, Scr.Look.MSMenu[MENU_BACK_TITLE] );
        set_astbar_style_ptr( tbar, BAR_STATE_FOCUSED, Scr.Look.MSMenu[MENU_BACK_HITITLE] );
        width = calculate_astbar_width( tbar );
        height = calculate_astbar_height( tbar );
        destroy_astbar( &tbar );
    }
    if( width_ret )
		*width_ret = width;
	if( height_ret )
		*height_ret = height ;
}

inline static ASFlagType
fix_background_align( ASFlagType align )
{
	ASFlagType new_align = align ;
	/* don't ask why - simply magic :)
	 * Well not really - we want to disregard background size in calculations of overall
	 * titlebar size. Overall titlebar size is determined only by text size and button sizes.
	 * At the same time we would like to be able to scale backgrounds to the size of the
	 * titlebar
	 */
	/* clear_flags(new_align , (RESIZE_H|RESIZE_V)); */
    if( get_flags( new_align, RESIZE_H_SCALE ) )
		set_flags( new_align, FIT_LABEL_WIDTH );
	if( get_flags( new_align, RESIZE_V_SCALE) )
    	set_flags( new_align, FIT_LABEL_HEIGHT );
	LOCAL_DEBUG_OUT( "Fixed align from 0x%lx to 0x%lx", align, new_align );
	return new_align;
}


Bool
hints2decorations( ASWindow *asw, ASHints *old_hints )
{
	int i ;
	MyFrame *frame = NULL, *old_frame = NULL ;
    Bool has_tbar = False, had_tbar = False ;
	Bool icon_image_changed = True ;
    ASOrientation *od = get_orientation_data(asw);
    char *mystyle_name = Scr.Look.MSWindow[BACK_FOCUSED]->name;
    char *frame_mystyle_name = NULL ;
    int *frame_contexts  = &(od->frame_contexts[0]);
	Bool tbar_created = False;
	Bool status_changed = False ;

	LOCAL_DEBUG_CALLER_OUT( "asw(%p)->client(%lX)", asw, asw?asw->w:0 );

	/* 3) we need to prepare icon window : */
    check_icon_canvas( asw, (ASWIN_HFLAGS( asw, AS_Icon) && !get_flags(Scr.Feel.flags, SuppressIcons)) );

	/* 4) we need to prepare icon title window : */
	check_icon_title_canvas( asw, (ASWIN_HFLAGS( asw, AS_IconTitle) &&
                                   get_flags(Scr.Feel.flags, IconTitle) &&
								  !get_flags(Scr.Feel.flags, SuppressIcons)),
           	                      !get_flags(Scr.Look.flags, SeparateButtonTitle)&&
               	                  (asw->icon_canvas!=NULL) );

    has_tbar = (ASWIN_HFLAGS(asw, AS_Titlebar)!= 0);
	frame = myframe_find( ASWIN_HFLAGS(asw, AS_Frame)?asw->hints->frame_name:NULL );
	if( old_hints == NULL )
	{
		had_tbar = !has_tbar ;
		old_frame = NULL ;
	}else
	{
		had_tbar = (get_flags(old_hints->flags, AS_Titlebar)!= 0);
		old_frame = myframe_find( get_flags(old_hints->flags, AS_Frame)?old_hints->frame_name:NULL );
	}

	asw->frame_data = frame ;

	/* 5) we need to prepare windows for 4 frame decoration sides : */
	if( has_tbar != had_tbar || frame != old_frame )
	{
		LOCAL_DEBUG_OUT( "has_tbar = %d, has_top_parts = %d, ", has_tbar, myframe_has_parts(frame, FRAME_TOP_MASK) );
	   	check_side_canvas( asw, od->tbar_side, has_tbar||myframe_has_parts(frame, FRAME_TOP_MASK) );
	}
    if( !ASWIN_HFLAGS(asw, AS_Handles) )
    {
        check_side_canvas( asw, od->sbar_side, False );
        check_side_canvas( asw, od->left_side, False );
        check_side_canvas( asw, od->right_side, False );
    }else if( frame != old_frame || (get_flags( old_hints->flags, AS_Handles ) != ASWIN_HFLAGS(asw, AS_Handles)))
	{
	    check_side_canvas( asw, od->sbar_side, myframe_has_parts(frame, FRAME_BTM_MASK) );
  		check_side_canvas( asw, od->left_side, myframe_has_parts(frame, FRAME_LEFT_MASK) );
	    check_side_canvas( asw, od->right_side, myframe_has_parts(frame, FRAME_RIGHT_MASK) );
	}

	if( ASWIN_HFLAGS(asw, AS_Handles) && get_flags(frame->flags, MYFRAME_NOBORDER) )
	{	
		XSetWindowBorderWidth( dpy, asw->frame, 0 );
		asw->status->frame_border_width = 0 ;
	}else if( asw->hints && get_flags(asw->hints->flags, AS_Border)) 
	{	
		XSetWindowBorderWidth( dpy, asw->frame, asw->hints->border_width );
		asw->status->frame_border_width = asw->hints->border_width ;
	}

    /* make sure all our decoration windows are mapped and in proper order: */
    if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
        XRaiseWindow( dpy, asw->w );

    /* 6) now we have to create bar for icon - if it is not client's animated icon */
	if( asw->icon_canvas )
	{
		if( old_hints )
		{
			icon_image_changed = (ASWIN_HFLAGS( asw, AS_ClientIcon|AS_ClientIconPixmap ) != get_flags(old_hints->flags, AS_ClientIcon|AS_ClientIconPixmap));
			if( !icon_image_changed )
        		icon_image_changed = (asw->hints->icon.pixmap != old_hints->icon.pixmap );
			if( !icon_image_changed )
				icon_image_changed = ( mystrcmp(asw->hints->icon_file, old_hints->icon_file) != 0 );
		}

		if( icon_image_changed )
		{
			ASImage *icon_image = get_window_icon_image( asw );
    		check_tbar( &(asw->icon_button), (asw->icon_canvas != NULL), AS_ICON_MYSTYLE,
            		    icon_image, Scr.Look.ButtonWidth, Scr.Look.ButtonHeight,/* scaling icon image */
                		0, Scr.Look.ButtonAlign,
                		Scr.Look.ButtonBevel, Scr.Look.ButtonBevel,
                		TEXTURE_TRANSPIXMAP_ALPHA, TEXTURE_TRANSPIXMAP_ALPHA,
                		C_IconButton );
			
			if( icon_image )
    	    	safe_asimage_destroy( icon_image );
		}
	}
    if( asw->icon_button && old_hints == NULL )
    {
        set_astbar_style( asw->icon_button, BAR_STATE_UNFOCUSED, AS_ICON_MYSTYLE );
        set_astbar_style( asw->icon_button, BAR_STATE_FOCUSED, AS_ICON_MYSTYLE );
		asw->icon_button->h_border = asw->icon_button->v_border = Scr.Look.ButtonIconSpacing ;
    }

    /* 7) now we have to create bar for icon title (optional) */
	check_tbar( &(asw->icon_title), 
			    get_flags(Scr.Feel.flags, IconTitle) && ((asw->icon_canvas != NULL && !get_flags(Scr.Look.flags, SeparateButtonTitle)) || asw->icon_title_canvas != NULL), 
				AS_ICON_TITLE_MYSTYLE,
  	            NULL, 0, 0, 0, ALIGN_CENTER, DEFAULT_TBAR_HILITE, DEFAULT_TBAR_HILITE,
              	TEXTURE_TRANSPIXMAP_ALPHA, TEXTURE_TRANSPIXMAP_ALPHA,
              	C_IconTitle );
    
	if( asw->icon_title )
    {
	    LOCAL_DEBUG_OUT( "setting icon label to %s", ASWIN_ICON_NAME(asw) );
	    add_astbar_label( asw->icon_title, 0, 0, 0, frame->title_align,
							DEFAULT_TBAR_HSPACING, DEFAULT_TBAR_VSPACING,
						    ASWIN_ICON_NAME(asw),
							(asw->hints->icon_name_idx < 0)?AS_Text_ASCII :
							asw->hints->names_encoding[asw->hints->icon_name_idx]);
	    set_astbar_style( asw->icon_title, BAR_STATE_FOCUSED, AS_ICON_TITLE_MYSTYLE );
  	    set_astbar_style( asw->icon_title, BAR_STATE_UNFOCUSED, AS_ICON_TITLE_UNFOCUS_MYSTYLE );
    }

    if( asw->hints->mystyle_names[BACK_FOCUSED] )
        mystyle_name = asw->hints->mystyle_names[BACK_FOCUSED];
    if( frame->title_style_names[BACK_FOCUSED] )
        mystyle_name = frame->title_style_names[BACK_FOCUSED];
    frame_mystyle_name = frame->frame_style_names[BACK_FOCUSED];

    /* 8) now we have to create actuall bars - for each frame element plus one for the titlebar */
	for( i = 0 ; i < FRAME_SIDES ; ++i )
		clear_flags( asw->internal_flags, ASWF_FirstCornerFollowsTbarSize<<i);
	if( old_hints == NULL ||
		(get_flags( old_hints->flags, AS_Handles ) != ASWIN_HFLAGS(asw, AS_Handles)) ||
		old_frame != frame )
	{
		status_changed = True ;
    	if( ASWIN_HFLAGS(asw, AS_Handles) )
    	{
        	for( i = 0 ; i < FRAME_PARTS ; ++i )
        	{
            	ASImage *img = NULL ;
            	unsigned int real_part = od->frame_rotation[i];
				int part_width = frame->part_width[i] ;
				int part_length = frame->part_length[i] ;

            	img = frame->parts[i]?frame->parts[i]->image:NULL ;

				if( img == NULL )
				{
					if( i < FRAME_SIDES )
					{
						if( part_width == 0 )
							part_width = BOUNDARY_WIDTH ;
						if( part_length == 0 )
							part_length = 1 ;

					}else
					{
						if( part_width == 0 )
						{
							part_width = CORNER_WIDTH ;
							set_flags( asw->internal_flags, ASWF_FirstCornerFollowsTbarSize<<(i-FRAME_SIDES)) ;
						}
						if( part_length == 0 )
							part_length = BOUNDARY_WIDTH ;
					}
				}

            	if( (0x01<<i)&MYFRAME_HOR_MASK )
            	{
                	*(od->in_width) = part_length ;
                	*(od->in_height) = part_width ;
            	}else
            	{
                	*(od->in_width) = part_width ;
                	*(od->in_height) = part_length ;
            	}
    			LOCAL_DEBUG_OUT( "part(%d)->real_part(%d)->from_size(%ux%u)->in_size(%ux%u)->out_size(%ux%u)", i, real_part, frame->part_width[i], frame->part_length[i], *(od->in_width), *(od->in_height), *(od->out_width), *(od->out_height) );
            	check_tbar( &(asw->frame_bars[real_part]), IsFramePart(frame,i), frame_mystyle_name?frame_mystyle_name:mystyle_name,
                        	img, *(od->out_width), *(od->out_height),
                        	od->flip, frame->part_align[i],
                        	frame->part_fbevel[i], frame->part_ubevel[i],
                        	TEXTURE_TRANSPIXMAP_ALPHA, TEXTURE_TRANSPIXMAP_ALPHA,
                        	frame_contexts[i] );
	      	}
    	}else
        	for( i = 0 ; i < FRAME_PARTS ; ++i )
            	check_tbar( &(asw->frame_bars[i]), False, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, C_NO_CONTEXT );

    	check_tbar( &(asw->tbar), has_tbar, mystyle_name, NULL, 0, 0,
					od->flip,
					frame->title_align,
                	frame->title_fbevel, frame->title_ubevel,
                	frame->title_fcm, frame->title_ucm,
                	C_TITLE );
		if( asw->tbar ) 
		{	
			set_astbar_huesat( asw->tbar, BAR_STATE_FOCUSED, frame->title_fhue, frame->title_fsat );
			set_astbar_huesat( asw->tbar, BAR_STATE_UNFOCUSED, frame->title_uhue, frame->title_usat );
		}

		tbar_created = (asw->tbar != NULL);
	}

	if( asw->tbar )
	{  /* 9) now we have to setup titlebar buttons */
        ASFlagType title_align = frame->title_align ;
        ASFlagType btn_mask = compile_titlebuttons_mask (asw->hints);
#ifdef SHAPE
        if( get_flags( frame->condense_titlebar, ALIGN_LEFT|ALIGN_RIGHT )  )
            ASWIN_SET_FLAGS( asw, AS_ShapedDecor );
#endif

		if( old_hints == NULL ||
 		    get_flags(old_hints->flags, AS_VerticalTitle) != ASWIN_HFLAGS(asw, AS_VerticalTitle) )
		{
			set_astbar_flip( asw->tbar, ASWIN_HFLAGS( asw, AS_VerticalTitle )?FLIP_VERTICAL:0 );   
			if( !tbar_created )
			{
				tbar_created = True ;
				delete_astbar_tile( asw->tbar, -1 );
			}
		}
	    /* need to add some titlebuttons */
		/* Don't need to do this as we already have padding set for label
		 * if( tbar_created )
		 * 	{
		 *     		asw->tbar->h_spacing = DEFAULT_TBAR_SPACING ;
		 *     		asw->tbar->v_spacing = DEFAULT_TBAR_SPACING ;
		 * 	}
		 */

		if( !tbar_created && old_hints != NULL )
		{
	        ASFlagType old_btn_mask = compile_titlebuttons_mask (old_hints);
			if( old_btn_mask != btn_mask )
				tbar_created = True ;
			else if( frame != old_frame )
				tbar_created = True ;
			if( tbar_created )
				delete_astbar_tile( asw->tbar, -1 );
		}

		if( !tbar_created )
		{
			if( old_hints == NULL ||
				  mystrcmp( asw->hints->names[0], old_hints->names[0] ) != 0 )
			{
				ASCanvas *canvas = ASWIN_HFLAGS(asw, AS_VerticalTitle)?asw->frame_sides[FR_W]:asw->frame_sides[FR_N];
	        	/* label ( goes on top of above pixmap ) */
	        	if( change_astbar_first_label( asw->tbar, asw->hints->names[0], asw->hints->names_encoding[0] ) )
  		        	if( canvas )
      		    	{
	                	render_astbar( asw->tbar, canvas );
						invalidate_canvas_save( canvas );
  		            	update_canvas_display( canvas );
					}
			}
		}else
		{	  
			Bool rtitle_spacer_added = False ;
			Bool ltitle_spacer_added = False ;
			unsigned int tbar_layout_row[MYFRAME_TITLE_BACKS] ;
			unsigned int tbar_layout_col[MYFRAME_TITLE_BACKS] ;

			compile_tbar_layout( tbar_layout_row, od->default_tbar_elem_row, frame->left_layout, frame->right_layout );
			compile_tbar_layout( tbar_layout_col, od->default_tbar_elem_col, frame->left_layout, frame->right_layout );

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPU) 
			for( i = 0 ; i < MYFRAME_TITLE_BACKS ; ++i )
			{
				LOCAL_DEBUG_OUT( "TitlebarLayout col[%d] = %d", i, tbar_layout_col[i] );
				LOCAL_DEBUG_OUT( "TitlebarLayout row[%d] = %d", i, tbar_layout_row[i] );
			}
#endif						

			for( i = MYFRAME_TITLE_BACK_LBTN ; i < MYFRAME_TITLE_BACKS ; ++i )
				if( frame->title_backs[i] && frame->title_backs[i]->image )
				{
                    LOCAL_DEBUG_OUT( "Adding Title Back #%d", i );
                    add_astbar_icon( asw->tbar,
                             	tbar_layout_col[ASO_TBAR_ELEM_LBTN+i],
                             	tbar_layout_row[ASO_TBAR_ELEM_LBTN+i],
                             	od->flip, fix_background_align(frame->title_backs_align[i]),
                             	frame->title_backs[i]->image);
					if( i == MYFRAME_TITLE_BACK_RTITLE_SPACER ) 
						rtitle_spacer_added = True ;
					else if( i == MYFRAME_TITLE_BACK_LTITLE_SPACER ) 
						ltitle_spacer_added = True ;
					   
      			}
#if 1
      		/* special handling to accomodate FIT_LABEL_SIZE alignment of the title */
			if( frame->title_backs[MYFRAME_TITLE_BACK_LBL] &&
				frame->title_backs[MYFRAME_TITLE_BACK_LBL]->image )
      		{
          		LOCAL_DEBUG_OUT( "title_back_align = 0x%lX", frame->title_backs_align[MYFRAME_TITLE_BACK_LBL] );
          		if( get_flags( frame->title_backs_align[MYFRAME_TITLE_BACK_LBL], FIT_LABEL_SIZE ) )
          		{
              		/* left spacer  - if we have an icon to go under the label if align is right or center */
              		if( get_flags( frame->title_align, ALIGN_RIGHT ) && !ltitle_spacer_added )
                	    add_astbar_spacer( asw->tbar,
                                     tbar_layout_col[ASO_TBAR_ELEM_LTITLE_SPACER],
                                     tbar_layout_row[ASO_TBAR_ELEM_LTITLE_SPACER],
                                     od->flip, PAD_LEFT, 1, 1);

              		/* right spacer - if we have an icon to go under the label and align is left or center */
              		if( get_flags( frame->title_align, ALIGN_LEFT ) && !rtitle_spacer_added )
                	    add_astbar_spacer( asw->tbar,
                                     tbar_layout_col[ASO_TBAR_ELEM_RTITLE_SPACER],
                                     tbar_layout_row[ASO_TBAR_ELEM_RTITLE_SPACER],
                                     od->flip, PAD_RIGHT, 1, 1);
					title_align = 0 ;
          		}
      		}
#endif

	        /* label ( goes on top of above pixmap ) */
  		    add_astbar_label( asw->tbar,
                          tbar_layout_col[ASO_TBAR_ELEM_LBL],
                          tbar_layout_row[ASO_TBAR_ELEM_LBL],
                          od->flip,
                          title_align, frame->title_h_spacing, frame->title_v_spacing,
                          asw->hints->names[0], asw->hints->names_encoding[0]);
		
			/* all the buttons go after the label to be rendered over it */
			/* left buttons : */
	        add_astbar_btnblock(asw->tbar,
  		                        tbar_layout_col[ASO_TBAR_ELEM_LBTN],
      		                    tbar_layout_row[ASO_TBAR_ELEM_LBTN],
                                od->flip, frame->left_btn_align,
              		            &(Scr.Look.ordered_buttons[0]), btn_mask,
                  		        Scr.Look.button_first_right,
                      		    Scr.Look.TitleButtonXOffset[0], Scr.Look.TitleButtonYOffset[0], Scr.Look.TitleButtonSpacing[0],
                          		od->left_btn_order );

			/* right buttons : */
  		    add_astbar_btnblock(asw->tbar,
      		                    tbar_layout_col[ASO_TBAR_ELEM_RBTN],
          		                tbar_layout_row[ASO_TBAR_ELEM_RBTN],
                                od->flip, frame->right_btn_align,
                  		        &(Scr.Look.ordered_buttons[Scr.Look.button_first_right]), btn_mask,
                      		    TITLE_BUTTONS - Scr.Look.button_first_right,
                          		Scr.Look.TitleButtonXOffset[1], Scr.Look.TitleButtonYOffset[1], Scr.Look.TitleButtonSpacing[1],
	                            od->right_btn_order );
  		    /* titlebar balloons */
      		for( i = 0 ; i < TITLE_BUTTONS ; ++i )
	        {
  		        if( get_flags( btn_mask, C_TButton0<<i) )
      		    {
          		    char *str = list_functions_by_context (C_TButton0<<i, asw->hints);
              		LOCAL_DEBUG_OUT( "balloon text will be \"%s\"", str?str:"none" );
	                set_astbar_balloon( asw->tbar, C_TButton0<<i, str, AS_Text_ASCII );
  		            if( str )
      		            free( str );
          		}
      		}
		}
	}

    /* we also need to setup label, unfocused/sticky style and tbar sizes -
     * it all is done when we change windows state, or move/resize it */
    /*since we might have destroyed/created some windows - we have to refresh grabs :*/
    grab_window_input( asw, False );
	if( get_flags( asw->internal_flags, ASWF_WindowComplete) )
	{
		/* we need to map all the possibly created subwindows if window is not iconic */
		if( !ASWIN_GET_FLAGS(asw, AS_Iconic ) )
		{
			LOCAL_DEBUG_OUT("mapping frame subwindows for client %lX, frame canvas = %p", asw->w, asw->frame_canvas );
			XMapSubwindows(dpy, asw->frame);
		}else
		{
			map_canvas_window( asw->icon_canvas, False );
			map_canvas_window( asw->icon_title_canvas, False );
		}
	}
	return	status_changed ;
}


/*************************************************************************/
/*   Main window decoration routine                                      */
/*************************************************************************/
void
redecorate_window( ASWindow *asw, Bool free_resources )
{
	int i ;

LOCAL_DEBUG_OUT( "as?w(%p)->free_res(%d)", asw, free_resources );
    if( AS_ASSERT(asw) )
        return ;

    if(  free_resources || asw->hints == NULL || asw->status == NULL )
    {/* destroy window decorations here : */
     /* destruction goes in reverese order ! */
        check_tbar( &(asw->icon_title),  False, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, C_NO_CONTEXT );
        check_tbar( &(asw->icon_button), False, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, C_NO_CONTEXT );
        check_tbar( &(asw->tbar),        False, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, C_NO_CONTEXT );
		i = FRAME_PARTS ;
		while( --i >= 0 )
            check_tbar( &(asw->frame_bars[i]), False, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, C_NO_CONTEXT );

        check_side_canvas( asw, FR_W, False );
        check_side_canvas( asw, FR_E, False );
        check_side_canvas( asw, FR_S, False );
        check_side_canvas( asw, FR_N, False );

		if( asw->icon_canvas == asw->icon_title_canvas )
			asw->icon_title_canvas = NULL ;
		if( asw->icon_title_canvas && asw->icon_title_canvas->w )
	        check_icon_title_canvas( asw, False, False );
		if( asw->icon_canvas && asw->icon_canvas->w )
	        check_icon_canvas( asw, False );
        check_client_canvas( asw, False );
        check_frame_canvas( asw, False );

        return ;
    }

    /* 1) we need to create our frame window : */
    if( check_frame_canvas( asw, True ) == NULL )
        return;

    /* 2) we need to reparent our title window : */
    if( check_client_canvas( asw, True ) == NULL )
        return;

	hints2decorations( asw, NULL );

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
	LOCAL_DEBUG_CALLER_OUT( "asw(%p)->client(%lX)", asw, asw?asw->w:0 );

	if( asw )
	{

        if( ASWIN_GET_FLAGS(asw, AS_Dead) )
        {
            clear_canvas_shape( asw->frame_canvas, True );
            return;
        }

        if( ASWIN_GET_FLAGS( asw, AS_Iconic ) )
        {                                      /* todo: update icon's shape */

        }else
        {
            int i, tbar_side = -1 ;
		    int child_x = 0 ;
		    int child_y = 0 ;
        	unsigned int width, height, bw;
			XRectangle    rect;

			/* starting with an empty list of rectangles */
			if( asw->frame_canvas->shape )
				flush_vector( asw->frame_canvas->shape );
			else
				asw->frame_canvas->shape = create_shape();

			get_current_canvas_geometry( asw->client_canvas, &child_x, &child_y, &width, &height, &bw );
	        rect.x = 0 ;
    	    rect.y = 0 ;
        	rect.width  = width;
            rect.height = height;

			/* adding rectangles for all the frame decorations */
			if( asw->shading_steps == 0 && ASWIN_GET_FLAGS(asw, AS_Shaded) )
			{
				ASOrientation *od = get_orientation_data( asw );
	        	tbar_side = od->tbar_side ;
			}
          	for( i = 0 ; i < FRAME_SIDES ; ++i )
              	if( asw->frame_sides[i] && (i == tbar_side || tbar_side == -1) )
				{
					int x, y ;
					unsigned int s_width, s_height, bw ;
					LOCAL_DEBUG_OUT( " Frame side %d", i );
					get_current_canvas_geometry( asw->frame_sides[i], &x, &y, &s_width, &s_height, &bw );
			
					combine_canvas_shape_at_geom( asw->frame_canvas, asw->frame_sides[i], x, y,
							  					s_width, s_height, bw );
/*                  	combine_canvas_shape( asw->frame_canvas, asw->frame_sides[i] ); */
				}

			/* subtract client rectangle, before adding clients shape */
			subtract_shape_rectangle( asw->frame_canvas->shape, &rect, 1, child_x, child_y, asw->frame_canvas->width, asw->frame_canvas->height );
			/* add clients shape */
			combine_canvas_shape_at_geom (asw->frame_canvas, asw->client_canvas, child_x, child_y, width, height, bw );

			print_shape( asw->frame_canvas->shape ) ;
			/* apply shape */
			update_canvas_display_mask (asw->frame_canvas, True);
			set_flags( asw->internal_flags, ASWF_PendingShapeRemoval );

        }
    }
#endif /* SHAPE */
}

void
ClearShape (ASWindow *asw)
{
#ifdef SHAPE
    if( asw && asw->frame_canvas )
    {
		clear_canvas_shape( asw->frame_canvas, True );
		clear_flags( asw->internal_flags, ASWF_PendingShapeRemoval );
    }
#endif /* SHAPE */
}


