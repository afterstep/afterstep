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
    {0, 0, 0, 0, 0}
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
    {0, 0, 0, 0, 0},
    {6, 5, 4, 3, 2, 1, 0}
};

inline ASOrientation*
get_orientation_data( ASWindow *asw )
{
    if( asw && asw->magic == MAGIC_ASWINDOW && asw->hints )
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
        }else
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
            w = create_visual_window (Scr.asv, (ASWIN_DESK(asw)==Scr.CurrentDesk)?Scr.Root:Scr.ServiceWin, -10, -10, 5, 5,
                                      asw->status?asw->status->border_width:0, InputOutput, valuemask, &attributes);
            asw->frame = w ;
            register_aswindow( w, asw );
            canvas = create_ascanvas_container( w );
LOCAL_DEBUG_OUT( "++CREAT Client(%lx(%s))->FRAME->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
        }else
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
            if( asw->internal == NULL )
                XAddToSaveSet (dpy, w);

            register_aswindow( w, asw );
            canvas = create_ascanvas_container( w );
LOCAL_DEBUG_OUT( "++CREAT Client(%lx(%s))->CLIENT->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
        }else
	    invalidate_canvas_config( canvas );
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
            w = create_visual_window ( Scr.asv, (ASWIN_DESK(asw)==Scr.CurrentDesk)?Scr.Root:Scr.ServiceWin, -10, -10, 1, 1, 0,
                                       InputOutput, valuemask, &attributes );

            register_aswindow( w, asw );
            canvas = create_ascanvas( w );
LOCAL_DEBUG_OUT( "++CREAT Client(%lx(%s))->ICONT->canvas(%p)->window(%lx)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", canvas, canvas->w );
        }else
			invalidate_canvas_config( canvas );
    }

    return (asw->icon_title_canvas = canvas);
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
            ASFlagType align,
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

        set_astbar_style( *tbar, BAR_STATE_FOCUSED, mystyle_name );
        set_astbar_style( *tbar, BAR_STATE_UNFOCUSED, "default" );
        delete_astbar_tile( *tbar, -1 );
        if( img )
        {
            LOCAL_DEBUG_OUT("adding bar icon %p %ux%u", img, img->width, img->height );
            add_astbar_icon( *tbar, 0, 0, 0, align, img );
            if( back_w == 0 )
                back_w = img->width ;
            if( back_h == 0 )
                back_h = img->height ;
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
        Bool do_focus_grab = (!focused && get_flags( Scr.Feel.flags, ClickToFocus ));
        if( do_focus_grab )
            grab_focus_click( asw->frame );
        else
            ungrab_window_buttons( asw->frame );

        if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
        {
            ungrab_window_buttons( asw->w );
            grab_window_buttons ( asw->w, C_WINDOW);
        }

        if( asw->icon_canvas && !ASWIN_GET_FLAGS(asw, AS_Dead) && validate_drawable(asw->icon_canvas->w, NULL, NULL) != None )
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

int
estimate_titlebar_size( ASHints *hints )
{
    int width = 0 ;
    if( hints )
    {
        ASTBarData *tbar = create_astbar();
        ASFlagType btn_mask = compile_titlebuttons_mask (hints);
        tbar->h_spacing = DEFAULT_TBAR_SPACING ;
        tbar->v_spacing = DEFAULT_TBAR_SPACING ;
        /* left buttons : */
        add_astbar_btnblock(tbar, 0, 0, 0, NO_ALIGN,
                            &(Scr.Look.ordered_buttons[0]), btn_mask,
                            Scr.Look.button_first_right,
                            Scr.Look.TitleButtonXOffset, Scr.Look.TitleButtonYOffset, Scr.Look.TitleButtonSpacing,
                            TBTN_ORDER_L2R );
        /* label */
        add_astbar_label(   tbar, 1, 0, 0, ALIGN_LEFT, DEFAULT_TBAR_SPACING, DEFAULT_TBAR_SPACING, hints->names[0], hints->names_encoding[0]);
        /* right buttons : */
        add_astbar_btnblock(tbar, 2, 0, 0, NO_ALIGN,
                            &(Scr.Look.ordered_buttons[Scr.Look.button_first_right]), btn_mask,
                            TITLE_BUTTONS-Scr.Look.button_first_right,
                            Scr.Look.TitleButtonXOffset, Scr.Look.TitleButtonYOffset, Scr.Look.TitleButtonSpacing,
                            TBTN_ORDER_R2L );

        set_astbar_style_ptr( tbar, BAR_STATE_UNFOCUSED, Scr.Look.MSMenu[MENU_BACK_TITLE] );
        set_astbar_style_ptr( tbar, BAR_STATE_FOCUSED, Scr.Look.MSMenu[MENU_BACK_HILITE] );
        width = calculate_astbar_width( tbar );
        destroy_astbar( &tbar );
    }
    return width;
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
    clear_flags(new_align , (RESIZE_H|RESIZE_V));
    if( get_flags( new_align, RESIZE_H_SCALE ) )
		set_flags( new_align, FIT_LABEL_WIDTH );
	if( get_flags( new_align, RESIZE_V_SCALE) )
    	set_flags( new_align, FIT_LABEL_HEIGHT );
	LOCAL_DEBUG_OUT( "Fixed align from 0x%x to 0x%x", align, new_align );
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
	Bool add_icon_label = (asw->icon_title==NULL) ;
	Bool tbar_created = False;
	Bool status_changed = False ;

	LOCAL_DEBUG_CALLER_OUT( "asw(%p)->client(%lX)", asw, asw?asw->w:0 );

	if( old_hints == NULL || get_flags(old_hints->flags, AS_Icon) != ASWIN_HFLAGS( asw, AS_Icon) )
	{	/* 3) we need to prepare icon window : */
    	check_icon_canvas( asw, (ASWIN_HFLAGS( asw, AS_Icon) && !get_flags(Scr.Feel.flags, SuppressIcons)) );
	}

	if( old_hints == NULL || get_flags(old_hints->flags, AS_IconTitle|AS_Icon) != ASWIN_HFLAGS( asw, AS_IconTitle|AS_Icon) )
	{/* 4) we need to prepare icon title window : */
	    check_icon_title_canvas( asw, (ASWIN_HFLAGS( asw, AS_IconTitle) &&
    	                               get_flags(Scr.Feel.flags, IconTitle) &&
        	                          !get_flags(Scr.Feel.flags, SuppressIcons)),
            	                      !get_flags(Scr.Look.flags, SeparateButtonTitle)&&
                	                  (asw->icon_canvas!=NULL) );
	}

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
	   	check_side_canvas( asw, od->tbar_side, has_tbar||myframe_has_parts(frame, FRAME_TOP_MASK) );
	if( frame != old_frame )
	{
	    check_side_canvas( asw, od->sbar_side, myframe_has_parts(frame, FRAME_BTM_MASK) );
  		check_side_canvas( asw, od->left_side, myframe_has_parts(frame, FRAME_LEFT_MASK) );
	    check_side_canvas( asw, od->right_side, myframe_has_parts(frame, FRAME_RIGHT_MASK) );
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
            		    icon_image, icon_image?icon_image->width:0, icon_image?icon_image->height:0,/* scaling icon image */
                		ALIGN_CENTER,
                		DEFAULT_TBAR_HILITE, DEFAULT_TBAR_HILITE,
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
    }

    /* 7) now we have to create bar for icon title (optional) */
	if( asw->icon_title == NULL &&
		(asw->icon_canvas != NULL||asw->icon_title_canvas != NULL) )
	{
		check_tbar( &(asw->icon_title), (asw->icon_canvas != NULL||asw->icon_title_canvas != NULL), AS_ICON_TITLE_MYSTYLE,
  	            	NULL, 0, 0, ALIGN_CENTER, DEFAULT_TBAR_HILITE, DEFAULT_TBAR_HILITE,
              		TEXTURE_TRANSPIXMAP_ALPHA, TEXTURE_TRANSPIXMAP_ALPHA,
              		C_IconTitle );
	}
    if( asw->icon_title )
    {
		if( add_icon_label )
		{
	        LOCAL_DEBUG_OUT( "setting icon label to %s", ASWIN_ICON_NAME(asw) );
	        add_astbar_label( asw->icon_title, 0, 0, 0, frame->title_align,
							  DEFAULT_TBAR_SPACING, DEFAULT_TBAR_SPACING,
						      ASWIN_ICON_NAME(asw),
							  (asw->hints->icon_name_idx < 0)?AS_Text_ASCII :
														  asw->hints->names_encoding[asw->hints->icon_name_idx]);
		}else if( old_hints == NULL || mystrcmp(ASWIN_ICON_NAME(asw), old_hints->icon_name) != 0 )
		{
	        set_astbar_style( asw->icon_title, BAR_STATE_FOCUSED, AS_ICON_TITLE_MYSTYLE );
  	    	set_astbar_style( asw->icon_title, BAR_STATE_UNFOCUSED, "default" );
			change_astbar_first_label( asw->icon_title,
									   ASWIN_ICON_NAME(asw),
		  							  (asw->hints->icon_name_idx < 0)?AS_Text_ASCII :
																	  asw->hints->names_encoding[asw->hints->icon_name_idx]);
		}
    }

    if( asw->hints->mystyle_names[BACK_FOCUSED] )
        mystyle_name = asw->hints->mystyle_names[BACK_FOCUSED];
    if( frame->title_style_names[BACK_FOCUSED] )
        mystyle_name = frame->title_style_names[BACK_FOCUSED];
    frame_mystyle_name = frame->frame_style_names[BACK_FOCUSED];

    /* 8) now we have to create actuall bars - for each frame element plus one for the titlebar */
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
    			LOCAL_DEBUG_OUT( "part(%d)->real_part(%d)->from_size(%ux%u)->in_size(%ux%u)->out_size(%ux%u)", i, real_part, frame->part_width[i], frame->part_length[i], *(od->in_width), *(od->in_height), *(od->out_width), *(od->out_height) );
            	check_tbar( &(asw->frame_bars[real_part]), IsFramePart(frame,i), frame_mystyle_name?frame_mystyle_name:mystyle_name,
                        	img, *(od->out_width), *(od->out_height),
                        	frame->part_align[i],
                        	frame->part_fbevel[i], frame->part_ubevel[i],
                        	TEXTURE_TRANSPIXMAP_ALPHA, TEXTURE_TRANSPIXMAP_ALPHA,
                        	frame_contexts[i] );
	      	}
    	}else
        	for( i = 0 ; i < FRAME_PARTS ; ++i )
            	check_tbar( &(asw->frame_bars[i]), False, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, C_NO_CONTEXT );

    	check_tbar( &(asw->tbar), has_tbar, mystyle_name, NULL, 0, 0,
                	frame->title_align,
                	frame->title_fbevel, frame->title_ubevel,
                	frame->title_fcm, frame->title_ucm,
                	C_TITLE );
		tbar_created = (asw->tbar != NULL);
	}

	if( asw->tbar )
	{  /* 9) now we have to setup titlebar buttons */
        ASFlagType title_align = frame->title_align ;
        ASFlagType btn_mask = compile_titlebuttons_mask (asw->hints);

		if( old_hints == NULL ||
 		    get_flags(old_hints->flags, AS_VerticalTitle) != ASWIN_HFLAGS(asw, AS_VerticalTitle) )
		{
			if( ASWIN_HFLAGS( asw, AS_VerticalTitle ) )
				set_flags( asw->tbar->state, BAR_FLAGS_VERTICAL );
			else
				clear_flags( asw->tbar->state, BAR_FLAGS_VERTICAL );
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

		if( tbar_created )
		{
#if 1
      		if( frame->title_backs[MYFRAME_TITLE_BACK_LBL] &&
				frame->title_backs[MYFRAME_TITLE_BACK_LBL]->image )
      		{
          	    int title_back_align = frame->title_backs_align[MYFRAME_TITLE_BACK_LBL] ;
          		LOCAL_DEBUG_OUT( "title_back_align = 0x%X", title_back_align );
          		if( get_flags( title_back_align, FIT_LABEL_SIZE ) )
          		{
              		/* left spacer  - if we have an icon to go under the label if align is right or center */
              		if( get_flags( frame->title_align, ALIGN_RIGHT ) )
                	    add_astbar_spacer( asw->tbar,
                                     od->default_tbar_elem_col[ASO_TBAR_ELEM_LTITLE_SPACER],
                                     od->default_tbar_elem_row[ASO_TBAR_ELEM_LTITLE_SPACER],
                                     od->flip, PAD_LEFT, 1, 1);

              		/* right spacer - if we have an icon to go under the label and align is left or center */
              		if( get_flags( frame->title_align, ALIGN_LEFT ) )
                	    add_astbar_spacer( asw->tbar,
                                     od->default_tbar_elem_col[ASO_TBAR_ELEM_RTITLE_SPACER],
                                     od->default_tbar_elem_row[ASO_TBAR_ELEM_RTITLE_SPACER],
                                     od->flip, PAD_RIGHT, 1, 1);
            		title_align = 0 ;
          		}
				title_back_align = fix_background_align( title_back_align );

        		add_astbar_icon( asw->tbar,
                             od->default_tbar_elem_col[ASO_TBAR_ELEM_LBL],
                             od->default_tbar_elem_row[ASO_TBAR_ELEM_LBL],
                             od->flip, title_back_align,
                             frame->title_backs[MYFRAME_TITLE_BACK_LBL]->image);
      		}
#endif

	        /* label ( goes on top of above pixmap ) */
  		    add_astbar_label( asw->tbar,
                          od->default_tbar_elem_col[ASO_TBAR_ELEM_LBL],
                          od->default_tbar_elem_row[ASO_TBAR_ELEM_LBL],
                          od->flip,
                          title_align, DEFAULT_TBAR_SPACING, max( asw->tbar->top_bevel, asw->tbar->bottom_bevel),
                          asw->hints->names[0], asw->hints->names_encoding[0]);
		}else if( old_hints == NULL ||
				  mystrcmp( asw->hints->names[0], old_hints->names[0] ) != 0 )
		{
			ASCanvas *canvas = ASWIN_HFLAGS(asw, AS_VerticalTitle)?asw->frame_sides[FR_W]:asw->frame_sides[FR_N];
	        /* label ( goes on top of above pixmap ) */
	        if( change_astbar_first_label( asw->tbar, asw->hints->names[0], asw->hints->names_encoding[0] ) )
  		        if( canvas )
      		    {
	                render_astbar( asw->tbar, canvas );
  		            update_canvas_display( canvas );
				}
		}
		/* all the buttons go after the label to be rendered over it */
		if( tbar_created )
		{
			/* really is only 2 iterations - but still  - toghter code this way */
			for( i = MYFRAME_TITLE_BACK_LBTN ; i <= MYFRAME_TITLE_BACK_LSPACER ; ++i )
				if( frame->title_backs[i] && frame->title_backs[i]->image )
				{
					add_astbar_icon( asw->tbar,
                             	od->default_tbar_elem_col[ASO_TBAR_ELEM_LBTN+i],
                             	od->default_tbar_elem_row[ASO_TBAR_ELEM_LBTN+i],
                             	od->flip, fix_background_align(frame->title_backs_align[i]),
                             	frame->title_backs[i]->image);
      			}
			/* really is only 2 iterations - but still  - toghter code this way */
			for( i = MYFRAME_TITLE_BACK_RSPACER ; i <= MYFRAME_TITLE_BACK_RBTN ; ++i )
				if( frame->title_backs[i] && frame->title_backs[i]->image )
				{
					int idx = ASO_TBAR_ELEM_RSPACER-MYFRAME_TITLE_BACK_RSPACER+i ;
					add_astbar_icon( asw->tbar,
                             	od->default_tbar_elem_col[idx],
                             	od->default_tbar_elem_row[idx],
                             	od->flip, fix_background_align(frame->title_backs_align[i]),
                             	frame->title_backs[i]->image);
      			}

			 /* left buttons : */
	        add_astbar_btnblock(asw->tbar,
  		                        od->default_tbar_elem_col[ASO_TBAR_ELEM_LBTN],
      		                    od->default_tbar_elem_row[ASO_TBAR_ELEM_LBTN],
          		                0, ALIGN_VCENTER,
              		            &(Scr.Look.ordered_buttons[0]), btn_mask,
                  		        Scr.Look.button_first_right,
                      		    Scr.Look.TitleButtonXOffset, Scr.Look.TitleButtonYOffset, Scr.Look.TitleButtonSpacing,
                          		od->left_btn_order );

			/* right buttons : */
  		    add_astbar_btnblock(asw->tbar,
      		                    od->default_tbar_elem_col[ASO_TBAR_ELEM_RBTN],
          		                od->default_tbar_elem_row[ASO_TBAR_ELEM_RBTN],
              		            0, ALIGN_VCENTER,
                  		        &(Scr.Look.ordered_buttons[Scr.Look.button_first_right]), btn_mask,
                      		    TITLE_BUTTONS - Scr.Look.button_first_right,
                          		Scr.Look.TitleButtonXOffset, Scr.Look.TitleButtonYOffset, Scr.Look.TitleButtonSpacing,
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
	/* we need to map all the possibly created subwindows if window is not iconic */
	if( !ASWIN_GET_FLAGS(asw, AS_Iconic ) )
		XMapSubwindows(dpy, asw->frame);
	else
	{
		map_canvas_window( asw->icon_canvas, False );
		map_canvas_window( asw->icon_title_canvas, False );
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

LOCAL_DEBUG_OUT( "asw(%p)->free_res(%d)", asw, free_resources );
    if( AS_ASSERT(asw) )
        return ;

    if(  free_resources || asw->hints == NULL || asw->status == NULL )
    {/* destroy window decorations here : */
     /* destruction goes in reverese order ! */
        check_tbar( &(asw->icon_title),  False, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, C_NO_CONTEXT );
        check_tbar( &(asw->icon_button), False, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, C_NO_CONTEXT );
        check_tbar( &(asw->tbar),        False, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, C_NO_CONTEXT );
		i = FRAME_PARTS ;
		while( --i >= 0 )
            check_tbar( &(asw->frame_bars[i]), False, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, C_NO_CONTEXT );

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
            clear_canvas_shape( asw->frame_canvas );
            return;
        }

        if( ASWIN_GET_FLAGS( asw, AS_Iconic ) )
        {                                      /* todo: update icon's shape */

        }else
        {
            int i ;

            combine_canvas_shape (asw->frame_canvas, asw->client_canvas, True, True );
#if 0
            Window        wdumm;
			int client_x = 0, client_y = 0 ;
			unsigned int width, height, bw, unused_depth  ;
            XGetGeometry( dpy, asw->w, &wdumm, &client_x, &client_y, &width, &height, &bw, &unused_depth );

            if( ASWIN_GET_FLAGS( asw, AS_Shaped ) )
            {
				/* we must use Translate coordinates since some of the canvases may not have updated
				 * their config at the time */
    			LOCAL_DEBUG_OUT( "combining client shape at %+d%+d", client_x, client_y );
                XShapeCombineShape (dpy, asw->frame, ShapeBounding,
                                    client_x+bw,
                                    client_y+bw,
                                    asw->w, ShapeBounding, ShapeSet);
            }else
            {
                XRectangle    rect;
				/* we must use Translate coordinates since some of the canvases may not have updated
				 * their config at the time */
				rect.x = client_x ;
				rect.y = client_y ;
				LOCAL_DEBUG_OUT( "setting client shape to rectangle at %+d%+d", rect.x, rect.y );
                rect.width  = width+bw*2;
                rect.height = height+bw*2;

                XShapeCombineRectangles (dpy, asw->frame, ShapeBounding,
                                         0, 0, &rect, 1, ShapeSet, Unsorted);
            }
#endif
            for( i = 0 ; i < FRAME_SIDES ; ++i )
                if( asw->frame_sides[i] )
                    combine_canvas_shape( asw->frame_canvas, asw->frame_sides[i], False, False );
        }
    }
#if 0 /*old code : */
        int bw = asw->status->border_width ;
        if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
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
#endif /* old_code */
#endif /* SHAPE */
}

void
ClearShape (ASWindow *asw)
{
#ifdef SHAPE
    if( asw && asw->frame_canvas )
    {
	unsigned int width, height ;
        XRectangle    rect;

	get_drawable_size( asw->frame_canvas->w, &width, &height );
        rect.x = 0;
        rect.y = 0;
        rect.width  = width;
        rect.height = height;
        LOCAL_DEBUG_OUT( "setting shape to rectangle %dx%d", rect.width, rect.height );
        XShapeCombineRectangles ( dpy, asw->frame, ShapeBounding,
                                  0, 0, &rect, 1, ShapeSet, Unsorted);
    }
#endif /* SHAPE */
}


