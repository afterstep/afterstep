/*
 * This is the complete from scratch rewrite of the original WinList
 * module.
 *
 * Copyright (C) 2003 Sasha Vasko <sasha at aftercode.net>
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
 */

/*#define DO_CLOCKING      */
#define LOCAL_DEBUG
#define EVENT_TRACE

#include "../../configure.h"

#include "../../libAfterStep/asapp.h"

#include <signal.h>
#include <unistd.h>

#include "../../libAfterImage/afterimage.h"

#include "../../libAfterStep/afterstep.h"
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/mystyle.h"
#include "../../libAfterStep/mystyle_property.h"
#include "../../libAfterStep/parser.h"
#include "../../libAfterStep/clientprops.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterStep/decor.h"
#include "../../libAfterStep/aswindata.h"
#include "../../libAfterStep/balloon.h"
#include "../../libAfterStep/event.h"

#include "../../libAfterConf/afterconf.h"

/**********************************************************************/
/*  AfterStep specific global variables :                             */
/**********************************************************************/
/**********************************************************************/
/*  Gadget local variables :                                         */
/**********************************************************************/
typedef struct ASWinTab{
	unsigned short	 row, col ;                /* needed for layout purposes */
	ASTBarData 		*bar ;
	ASCanvas 		*client ;
}ASWinTab;

typedef struct {
	Window main_window ;
	ASCanvas *main_canvas ;
	ASCanvas *tabs_canvas ;

	wild_reg_exp *pattern_wrexp ;

	ASWinTab *tabs ;
	int tabs_num ;

}ASWinTabsState ;

ASWinTabsState WinTabsState = { 0 };


#define WINTABS_TAB_EVENT_MASK    (ButtonReleaseMask | ButtonPressMask | \
	                               LeaveWindowMask   | EnterWindowMask | \
                                   StructureNotifyMask )

/**********************************************************************/
/**********************************************************************/
/* Our configuration options :                                        */
/**********************************************************************/
/*char *default_unfocused_style = "unfocused_window_style";
 char *default_focused_style = "focused_window_style";
 char *default_sticky_style = "sticky_window_style";
 */

WinTabsConfig *Config = NULL ;
/**********************************************************************/

void CheckConfigSanity();
void GetBaseOptions (const char *filename);
void GetOptions (const char *filename);
void HandleEvents();
void process_message (unsigned long type, unsigned long *body);
void DispatchEvent (ASEvent * Event);
Window make_wintabs_window();
Window make_tabs_window( Window parent );
void check_swallow_window( ASWindowData *wd );



int
main( int argc, char **argv )
{
    /* Save our program name - for error messages */
    InitMyApp (CLASS_GADGET, argc, argv, NULL, NULL, 0 );

    set_signal_handler( SIGSEGV );

    ConnectX( &Scr, PropertyChangeMask|EnterWindowMask );
    ConnectAfterStep (  M_END_WINDOWLIST |
                    	WINDOW_CONFIG_MASK |
                    	WINDOW_NAME_MASK );
    balloon_init (False);
    Config = CreateWinTabsConfig ();

    LoadBaseConfig ( GetBaseOptions);
	LoadColorScheme();
	LoadConfig ("wintabs", GetOptions);
    CheckConfigSanity();

	if( Config->pattern != NULL )
	{
		WinTabsState.pattern_wrexp = compile_wild_reg_exp( Config->pattern ) ;
	}else
	{
		show_error( "Empty Pattern requested for windows to be captured and tabbed - Abborting!");
		DeadPipe(0);
	}

	SendInfo ("Send_WindowList", 0);


    WinTabsState.main_window = make_wintabs_window();
    WinTabsState.main_canvas = create_ascanvas_container( WinTabsState.main_window );
	WinTabsState.tabs_canvas = create_ascanvas( make_tabs_window( WinTabsState.main_window ) );
    set_root_clip_area(WinTabsState.main_canvas );

	/* And at long last our main loop : */
    HandleEvents();
	return 0 ;
}

void HandleEvents()
{
    ASEvent event;
    Bool has_x_events = False ;
    while (True)
    {
        while((has_x_events = XPending (dpy)))
        {
            if( ASNextEvent (&(event.x), True) )
            {
                event.client = NULL ;
                setup_asevent_from_xevent( &event );
                DispatchEvent( &event );
            }
        }
        module_wait_pipes_input ( process_message );
    }
}

void
DeadPipe (int nonsense)
{
    FreeMyAppResources();

    if( WinTabsState.main_canvas )
        destroy_ascanvas( &WinTabsState.main_canvas );
    if( WinTabsState.main_window )
        XDestroyWindow( dpy, WinTabsState.main_window );
    if( Config )
        DestroyWinTabsConfig(Config);

#ifdef DEBUG_ALLOCS
    print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

    XFlush (dpy);			/* need this for SetErootPixmap to take effect */
	XCloseDisplay (dpy);		/* need this for SetErootPixmap to take effect */
    exit (0);
}

void
CheckConfigSanity()
{
    int i ;
    char *default_style = safemalloc( 1+strlen(MyName)+1);
	default_style[0] = '*' ;
	strcpy( &(default_style[1]), MyName );

    if( Config == NULL )
        Config = CreateWinTabsConfig ();

    Config->gravity = NorthWestGravity ;
    if( get_flags(Config->geometry.flags, XNegative) )
        Config->gravity = get_flags(Config->geometry.flags, YNegative)? SouthEastGravity:NorthEastGravity;
    else if( get_flags(Config->geometry.flags, YNegative) )
        Config->gravity = SouthWestGravity;

    Config->anchor_x = get_flags( Config->geometry.flags, XValue )?Config->geometry.x:0;
    if( get_flags(Config->geometry.flags, XNegative) )
        Config->anchor_x += Scr.MyDisplayWidth ;

    Config->anchor_y = get_flags( Config->geometry.flags, YValue )?Config->geometry.y:0;
    if( get_flags(Config->geometry.flags, YNegative) )
        Config->anchor_y += Scr.MyDisplayHeight ;

    mystyle_get_property (Scr.wmprops);

    Scr.Look.MSWindow[BACK_UNFOCUSED] = mystyle_find( Config->unfocused_style );
    Scr.Look.MSWindow[BACK_FOCUSED] = mystyle_find( Config->focused_style );
    Scr.Look.MSWindow[BACK_STICKY] = mystyle_find( Config->sticky_style );

    for( i = 0 ; i < BACK_STYLES ; ++i )
    {
        static char *default_window_style_name[BACK_STYLES] ={"focused_window_style","unfocused_window_style","sticky_window_style", NULL};
        if( Scr.Look.MSWindow[i] == NULL )
            Scr.Look.MSWindow[i] = mystyle_find( default_window_style_name[i] );
        if( Scr.Look.MSWindow[i] == NULL )
            Scr.Look.MSWindow[i] = mystyle_find_or_default( default_style );
    }
    free( default_style );

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
    PrintWinTabsConfig (Config);
    Print_balloonConfig ( Config->balloon_conf );
#endif
    balloon_config2look( &(Scr.Look), Config->balloon_conf );
    LOCAL_DEBUG_OUT( "balloon mystyle = %p (\"%s\")", Scr.Look.balloon_look->style,
                    Scr.Look.balloon_look->style?Scr.Look.balloon_look->style->name:"none" );
    set_balloon_look( Scr.Look.balloon_look );

}


void
GetBaseOptions (const char *filename)
{
	ReloadASEnvironment( NULL, NULL, NULL, False );
}

void
GetOptions (const char *filename)
{
    START_TIME(option_time);
    WinTabsConfig *config = ParseWinTabsOptions( filename, MyName );

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
    PrintWinTabsConfig (config);
#endif
    /* Need to merge new config with what we have already :*/
    /* now lets check the config sanity : */
    /* mixing set and default flags : */
    Config->flags = (config->flags&config->set_flags)|(Config->flags & (~config->set_flags));
    Config->set_flags |= config->set_flags;

    Config->gravity = NorthWestGravity ;
    if( get_flags(config->set_flags, WINTABS_Geometry) )
        merge_geometry(&(config->geometry), &(Config->geometry) );


    if( config->unfocused_style )
        set_string_value( &(Config->unfocused_style), mystrdup(config->unfocused_style), NULL, 0 );
    if( config->focused_style )
        set_string_value( &(Config->focused_style), mystrdup(config->focused_style), NULL, 0 );
    if( config->sticky_style )
        set_string_value( &(Config->sticky_style), mystrdup(config->sticky_style), NULL, 0 );

    if( get_flags(config->set_flags, WINTABS_FCM) )
        Config->fcm = config->fcm;
    if( get_flags(config->set_flags, WINTABS_UCM) )
        Config->ucm = config->ucm;
    if( get_flags(config->set_flags, WINTABS_SCM) )
        Config->scm = config->scm;

    if( get_flags(config->set_flags, WINTABS_H_SPACING) )
        Config->h_spacing = config->h_spacing;
    if( get_flags(config->set_flags, WINTABS_V_SPACING) )
        Config->v_spacing = config->v_spacing;

    if( Config->balloon_conf )
        Destroy_balloonConfig( Config->balloon_conf );
    Config->balloon_conf = config->balloon_conf ;
    config->balloon_conf = NULL ;

    if (config->style_defs)
        ProcessMyStyleDefinitions (&(config->style_defs));
    SHOW_TIME("Config parsing",option_time);
}

/****************************************************************************/
/* PROCESSING OF AFTERSTEP MESSAGES :                                       */
/****************************************************************************/
void
process_message (unsigned long type, unsigned long *body)
{
    LOCAL_DEBUG_OUT( "received message %lX", type );
	if( (type&WINDOW_PACKET_MASK) != 0 )
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );
        WindowPacketResult res ;
        /* saving relevant client info since handle_window_packet could destroy the actuall structure */
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
        Window               saved_w = (wd && wd->canvas)?wd->canvas->w:None;
        int                  saved_desk = wd?wd->desk:INVALID_DESK;
        struct ASWindowData *saved_wd = wd ;
#endif
        LOCAL_DEBUG_OUT( "message %lX window %lX data %p", type, body[0], wd );
        res = handle_window_packet( type, body, &wd );
        LOCAL_DEBUG_OUT( "\t res = %d, data %p", res, wd );
        if( res == WP_DataCreated || res == WP_DataChanged )
        {
            check_swallow_window( wd );
        }else if( res == WP_DataDeleted )
        {
            LOCAL_DEBUG_OUT( "client deleted (%p)->window(%lX)->desk(%d)", saved_wd, saved_w, saved_desk );
        }
    }
}

void
DispatchEvent (ASEvent * event)
{
    ASWindowData *pointer_wd = NULL ;

    SHOW_EVENT_TRACE(event);

    if( (event->eclass & ASE_POINTER_EVENTS) != 0 )
    {
    }

    switch (event->x.type)
    {
	    case ConfigureNotify:
            {
                ASFlagType changes = handle_canvas_config( WinTabsState.main_canvas );
                if( changes != 0 )
                    set_root_clip_area( WinTabsState.main_canvas );

                if( get_flags( changes, CANVAS_RESIZED ) )
				{
				}else if( changes != 0 )        /* moved - update transparency ! */
				{
				}
            }
	        break;
        case ButtonPress:
            break;
        case ButtonRelease:
			break;
        case EnterNotify :
			if( event->x.xcrossing.window == Scr.Root )
			{
				withdraw_active_balloon();
				break;
			}
        case LeaveNotify :
        case MotionNotify :
            break ;
	    case ClientMessage:
            if ((event->x.xclient.format == 32) &&
                (event->x.xclient.data.l[0] == _XA_WM_DELETE_WINDOW))
			{
                DeadPipe(0);
			}
	        break;
	    case PropertyNotify:
			LOCAL_DEBUG_OUT( "property %s(%lX), _XROOTPMAP_ID = %lX, event->w = %lX, root = %lX", XGetAtomName(dpy, event->x.xproperty.atom), event->x.xproperty.atom, _XROOTPMAP_ID, event->w, Scr.Root );
            if( event->x.xproperty.atom == _XROOTPMAP_ID && event->w == Scr.Root )
            {
                LOCAL_DEBUG_OUT( "root background updated!%s","");
                safe_asimage_destroy( Scr.RootImage );
                Scr.RootImage = NULL ;
            }else if( event->x.xproperty.atom == _AS_STYLE )
			{
				LOCAL_DEBUG_OUT( "AS Styles updated!%s","");
				handle_wmprop_event (Scr.wmprops, &(event->x));
				mystyle_list_destroy_all(&(Scr.Look.styles_list));
				LoadColorScheme();
				CheckConfigSanity();
				/* now we need to update everything */
			}
			break;
    }
}

/********************************************************************/
/* showing our main window :                                        */
/********************************************************************/
Window
make_wintabs_window()
{
	Window        w;
	XSizeHints    shints;
	ExtendedWMHints extwm_hints ;
	int x, y ;
    unsigned int width = max(Config->geometry.width,1);
    unsigned int height = max(Config->geometry.height,1);

	switch( Config->gravity )
	{
		case NorthEastGravity :
            x = Config->anchor_x - width ;
			y = Config->anchor_y ;
			break;
		case SouthEastGravity :
            x = Config->anchor_x - width;
            y = Config->anchor_y - height;
			break;
		case SouthWestGravity :
			x = Config->anchor_x ;
            y = Config->anchor_y - height;
			break;
		case NorthWestGravity :
		default :
			x = Config->anchor_x ;
			y = Config->anchor_y ;
			break;
	}

	w = create_visual_window( Scr.asv, Scr.Root, x, y, width, height, 0, InputOutput, 0, NULL);
    set_client_names( w, "WinTabs", "WINTABS", CLASS_GADGET, MyName );

	shints.flags = USPosition|USSize|PMinSize|PMaxSize|PBaseSize|PWinGravity;
	shints.min_width = shints.min_height = 4;
	shints.base_width = shints.base_height = 4;
	shints.win_gravity = Config->gravity ;

	extwm_hints.pid = getpid();
	extwm_hints.flags = EXTWM_PID|EXTWM_StateSkipTaskbar|EXTWM_StateSkipPager|EXTWM_TypeDock ;

	set_client_hints( w, NULL, &shints, AS_DoesWmDeleteWindow, &extwm_hints );

	/* showing window to let user see that we are doing something */
	XMapRaised (dpy, w);
	/* final cleanup */
	XFlush (dpy);
	sleep (1);								   /* we have to give AS a chance to spot us */
	/* we will need to wait for PropertyNotify event indicating transition
	   into Withdrawn state, so selecting event mask: */
    XSelectInput (dpy, w, PropertyChangeMask|StructureNotifyMask|
                          ButtonPressMask|ButtonReleaseMask|PointerMotionMask|
                          KeyPressMask|KeyReleaseMask|
                          EnterWindowMask|LeaveWindowMask|
						  SubstructureRedirectMask);

	return w ;
}

Window
make_tabs_window( Window parent )
{
	static XSetWindowAttributes attr ;

	attr.event_mask = WINTABS_TAB_EVENT_MASK ;
    return create_visual_window( Scr.asv, parent, 0, 0, 1, 1, 0, InputOutput, CWEventMask, &attr );
}

/**************************************************************************
 * Swallowing code
 **************************************************************************/

void
check_swallow_window( ASWindowData *wd )
{
    Window w;
    int try_num = 0 ;
	Bool withdraw_btn ;
    ASCanvas *nc ;
    int swidth, sheight ;
	char *name = NULL ;
	INT32 encoding ;

    if( wd == NULL && !get_flags( wd->state_flags, AS_Mapped))
        return;

	name = get_window_name( wd, Config->pattern_type, &encoding );
    LOCAL_DEBUG_OUT( "name(\"%s\")->icon_name(\"%s\")->res_class(\"%s\")->res_name(\"%s\")",
                     wd->window_name, wd->icon_name, wd->res_class, wd->res_name );

	if( match_wild_reg_exp( name, WinTabsState.pattern_wrexp) != 0 )
		return ;

#if 0
    LOCAL_DEBUG_OUT( "swallow target is %p, swallowed = %p", aswb, aswb->swallowed );
	aswf = aswb->parent ;
    if( aswb->swallowed != NULL )
        return;
    /* do the actuall swallowing here : */
    XGrabServer( dpy );
    /* first lets check if window is still not swallowed : it should have no more then 2 parents before root */
    w = get_parent_window( wd->client );
    LOCAL_DEBUG_OUT( "first parent %lX, root %lX", w, Scr.Root );
	while( w == Scr.Root && ++try_num  < 10 )
	{/* we should wait for AfterSTep to complete AddWindow protocol */
	    /* do the actuall swallowing here : */
    	XUngrabServer( dpy );
		sleep_a_millisec(200*try_num);
		XGrabServer( dpy );
		w = get_parent_window( wd->client );
		LOCAL_DEBUG_OUT( "attempt %d:first parent %lX, root %lX", try_num, w, Scr.Root );
	}
	if( w == Scr.Root )
	{
		XUngrabServer( dpy );
		return ;
	}
    if( w != None )
        w = get_parent_window( w );
    LOCAL_DEBUG_OUT( "second parent %lX, root %lX", w, Scr.Root );
    if( w != Scr.Root )
	{
		XUngrabServer( dpy );
		return ;
	}
    withdraw_btn = (WITHDRAW_ON_EDGE(Config) &&
					(&(aswf->buttons[0]) == aswb || &(aswf->buttons[aswf->buttons_num-1]) == aswb)) ||
                    WITHDRAW_ON_ANY(Config) ;
    /* its ok - we can swallow it now : */
    /* create swallow object : */
    aswb->swallowed = safecalloc( 1, sizeof(ASSwallowed ));
    /* first thing - we reparent window and its icon if there is any */
    nc = aswb->swallowed->normal = create_ascanvas_container( wd->client );
    XReparentWindow( dpy, wd->client, aswb->canvas->w, (aswb->canvas->width - nc->width)/2, (aswb->canvas->height - nc->height)/2 );
    register_object( wd->client, (ASMagic*)aswb );
    XSelectInput (dpy, wd->client, StructureNotifyMask);
    grab_swallowed_canvas_btns( nc, (aswb->folder!=NULL), withdraw_btn && aswb->parent == WharfState.root_folder);

    if( get_flags( wd->flags, AS_ClientIcon ) && !get_flags( wd->flags, AS_ClientIconPixmap) &&
		wd->icon != None )
    {
        ASCanvas *ic = create_ascanvas_container( wd->icon );
        aswb->swallowed->iconic = ic;
        XReparentWindow( dpy, wd->icon, aswb->canvas->w, (aswb->canvas->width-ic->width)/2, (aswb->canvas->height-ic->height)/2 );
        register_object( wd->icon, (ASMagic*)aswb );
        XSelectInput (dpy, wd->icon, StructureNotifyMask);
        grab_swallowed_canvas_btns(  ic, (aswb->folder!=NULL), withdraw_btn && aswb->parent == WharfState.root_folder);
    }
    aswb->swallowed->current = ( get_flags( wd->state_flags, AS_Iconic ) &&
                                    aswb->swallowed->iconic != NULL )?
                                aswb->swallowed->iconic:aswb->swallowed->normal;
    handle_canvas_config( aswb->swallowed->current );
    LOCAL_DEBUG_OUT( "client(%lX)->icon(%lX)->current(%lX)", wd->client, wd->icon, aswb->swallowed->current->w );

    if( get_flags( aswb->flags, ASW_MaxSwallow ) ||
		(Config->force_size.width == 0 && !get_flags(aswb->flags, ASW_FixedWidth)))
        aswb->desired_width = aswb->swallowed->current->width;
    if( get_flags( aswb->flags, ASW_MaxSwallow ) ||
		(Config->force_size.height == 0 && !get_flags(aswb->flags, ASW_FixedHeight)) )
        aswb->desired_height = aswb->swallowed->current->height;
    swidth = min( aswb->desired_width, aswb->swallowed->current->width );
    sheight = min( aswb->desired_height, aswb->swallowed->current->height );
    moveresize_canvas( aswb->swallowed->current,
                       make_tile_pad( get_flags(Config->align_contents,PAD_LEFT),
                                      get_flags(Config->align_contents,PAD_RIGHT),
                                      aswb->canvas->width, swidth      ),
                       make_tile_pad( get_flags(Config->align_contents,PAD_TOP),
                                      get_flags(Config->align_contents,PAD_BOTTOM),
                                      aswb->canvas->height, sheight    ),
                       swidth, sheight );
    map_canvas_window( aswb->swallowed->current, True );
    send_swallowed_configure_notify(aswb);

    update_wharf_folder_size( aswf );

    ASSync(False);
    XUngrabServer( dpy );
#endif
}


