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
typedef struct ASWinTab
{
	char 			*name ;
	Window 			 client ;

	ASTBarData 		*bar ;
	ASCanvas 		*client_canvas ;

	Bool closed ;
	XRectangle 	swallow_location ;

}ASWinTab;

typedef struct {

#define ASWT_StateMapped	(0x01<<0)
	ASFlagType flags ;

    Window main_window, tabs_window ;
	ASCanvas *main_canvas ;
	ASCanvas *tabs_canvas ;

	wild_reg_exp *pattern_wrexp ;

	ASVector *tabs ;

#define BANNER_LABEL_IDX  1	
	ASWinTab  banner ;

	int rows ;
	int row_height ;

    int selected_tab ;
    int pressed_tab ;

	int win_width, win_height ;

	ASTBarProps *tbar_props ;

	MyButton close_button ; 
	MyButton menu_button ;
	MyButton unswallow_button ;

}ASWinTabsState ;

ASWinTabsState WinTabsState = { 0 };

#define C_CloseButton 		C_TButton0
#define C_UnswallowButton 	C_TButton1
#define C_MenuButton 		C_TButton2


#define WINTABS_TAB_EVENT_MASK    (ButtonReleaseMask | ButtonPressMask | \
	                               LeaveWindowMask   | EnterWindowMask | \
                                   StructureNotifyMask|PointerMotionMask )

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
void do_swallow_window( ASWindowData *wd );
void check_swallow_window( ASWindowData *wd );
void rearrange_tabs( Bool dont_resize_window );
void render_tabs( Bool canvas_resized );
void on_destroy_notify(Window w);
void select_tab( int tab );
void press_tab( int tab );
void set_tab_look( ASWinTab *aswt, Bool no_bevel );

int find_tab_by_position( int root_x, int root_y );
void send_swallow_command();
void close_current_tab();
void unswallow_current_tab();

/* above function may also return : */
#define BANNER_TAB_INDEX -1		   
#define INVALID_TAB_INDEX -2


void DeadPipe(int);

int
main( int argc, char **argv )
{
    /* Save our program name - for error messages */
	set_DeadPipe_handler(DeadPipe);
    InitMyApp (CLASS_GADGET, argc, argv, NULL, NULL, 0 );
	LinkAfterStepConfig();

    set_signal_handler( SIGSEGV );


    ConnectX( ASDefaultScr, EnterWindowMask );
    ConnectAfterStep ( M_END_WINDOWLIST |M_DESTROY_WINDOW |M_SWALLOW_WINDOW|WINDOW_CONFIG_MASK|WINDOW_NAME_MASK, 0 );
    signal (SIGTERM, DeadPipe);
    signal (SIGKILL, DeadPipe);
    
    balloon_init (False);
    Config = CreateWinTabsConfig ();

    LoadBaseConfig ( GetBaseOptions);
	LoadColorScheme();
	LoadConfig ("wintabs", GetOptions);
    CheckConfigSanity();

	SendInfo ("Send_WindowList", 0);

    WinTabsState.selected_tab = WinTabsState.pressed_tab = -1 ;
	WinTabsState.tabs = create_asvector( sizeof(ASWinTab) );

    WinTabsState.main_window = make_wintabs_window();
    WinTabsState.main_canvas = create_ascanvas_container( WinTabsState.main_window );
    WinTabsState.tabs_window = make_tabs_window( WinTabsState.main_window );
    WinTabsState.tabs_canvas = create_ascanvas( WinTabsState.tabs_window );
    map_canvas_window( WinTabsState.tabs_canvas, True );
    set_root_clip_area(WinTabsState.main_canvas );

	/* delay mapping main canvas untill we actually swallowed something ! */
	if( WinTabsState.pattern_wrexp == NULL || !get_flags(Config->flags, ASWT_HideWhenEmpty )) 
	{	
		map_canvas_window( WinTabsState.main_canvas, True );
		set_flags( WinTabsState.flags, ASWT_StateMapped );
		rearrange_tabs( False );
	}

    /* map_canvas_window( WinTabsState.main_canvas, True ); */
    /* final cleanup */
	XFlush (dpy);
	sleep (1);								   /* we have to give AS a chance to spot us */

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
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
    int i = PVECTOR_USED(WinTabsState.tabs) ;
	
	{
		static int already_dead = False ; 
		if( already_dead ) 	return;/* non-reentrant function ! */
		already_dead = True ;
	}
    
	LOCAL_DEBUG_OUT( "reparenting %d clients back to the Root", i );
    while( --i >= 0  )
    {
        XReparentWindow( dpy, tabs[i].client, Scr.Root, i*10, i*10 );
    }
    ASSync(False );
    fflush(stderr);
    
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

	if( Config->pattern != NULL )
	{
		WinTabsState.pattern_wrexp = compile_wild_reg_exp( Config->pattern ) ;
	}else
	{
		show_warning( "Empty Pattern requested for windows to be captured and tabbed - will wait for swallow command");
	    if( !get_flags(Config->geometry.flags, WidthValue) )
			Config->geometry.width = 640 ;
	    if( !get_flags(Config->geometry.flags, HeightValue) )
			Config->geometry.height = 480 ;
	}

	WinTabsState.win_width = Config->geometry.width ; 
	WinTabsState.win_height = Config->geometry.height ; 


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

	WinTabsState.tbar_props = get_astbar_props(Scr.wmprops );
	if( WinTabsState.tbar_props != NULL ) 
	{
		for( i = 0 ; i < WinTabsState.tbar_props->buttons_num ; ++i ) 
		{
			MyIcon *icon = NULL ;
			if( WinTabsState.tbar_props->buttons[i].kind == _AS_BUTTON_CLOSE ) 
				icon = &(WinTabsState.close_button.unpressed);
			else if( WinTabsState.tbar_props->buttons[i].kind == _AS_BUTTON_CLOSE_PRESSED ) 	
				icon = &(WinTabsState.close_button.pressed);	 
			else if( WinTabsState.tbar_props->buttons[i].kind == _AS_BUTTON_MAXIMIZE ) 
				icon = &(WinTabsState.unswallow_button.unpressed);
			else if( WinTabsState.tbar_props->buttons[i].kind == _AS_BUTTON_MAXIMIZE_PRESSED ) 	
				icon = &(WinTabsState.unswallow_button.pressed);	 
			else if( WinTabsState.tbar_props->buttons[i].kind == _AS_BUTTON_MENU ) 
				icon = &(WinTabsState.menu_button.unpressed);
			else if( WinTabsState.tbar_props->buttons[i].kind == _AS_BUTTON_MENU_PRESSED ) 	
				icon = &(WinTabsState.menu_button.pressed);	 
			if( icon != NULL ) 
				icon_from_pixmaps( icon, WinTabsState.tbar_props->buttons[i].pmap, WinTabsState.tbar_props->buttons[i].mask, WinTabsState.tbar_props->buttons[i].alpha );
		}	 
		WinTabsState.close_button.width = max( WinTabsState.close_button.unpressed.width, WinTabsState.close_button.pressed.width );
		WinTabsState.close_button.height = max( WinTabsState.close_button.unpressed.height, WinTabsState.close_button.pressed.height );
		WinTabsState.close_button.context = C_CloseButton ; 
		WinTabsState.unswallow_button.width = max( WinTabsState.unswallow_button.unpressed.width, WinTabsState.unswallow_button.pressed.width );
		WinTabsState.unswallow_button.height = max( WinTabsState.unswallow_button.unpressed.height, WinTabsState.unswallow_button.pressed.height );
		WinTabsState.unswallow_button.context = C_UnswallowButton ; 
		WinTabsState.menu_button.width = max( WinTabsState.menu_button.unpressed.width, WinTabsState.menu_button.pressed.width );
		WinTabsState.menu_button.height = max( WinTabsState.menu_button.unpressed.height, WinTabsState.menu_button.pressed.height );
		WinTabsState.menu_button.context = C_MenuButton ; 
		
	}	 

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
	if( Config->balloon_conf == NULL ) 
	{
/*		
		Scr.Look.balloon_look->show = True ;
        Scr.Look.balloon_look->xoffset = 5 ;
        Scr.Look.balloon_look->yoffset = 5 ;
		Scr.Look.balloon_look->style = mystyle_list_find_or_default (NULL, NULL);
		Scr.Look.balloon_look->close_delay = 2000 ;
 */
	}	 
		
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

    if( config->pattern )
    {    
        set_string_value( &(Config->pattern), mystrdup(config->pattern), NULL, 0 );
        Config->pattern_type = config->pattern_type ; 
    }
    if( get_flags(config->set_flags, WINTABS_MaxRows) )
        Config->max_rows = config->max_rows;
    if( get_flags(config->set_flags, WINTABS_MaxColumns) )
        Config->max_columns = config->max_columns;
    if( get_flags(config->set_flags, WINTABS_MinTabWidth) )
        Config->min_tab_width = config->min_tab_width;
    if( get_flags(config->set_flags, WINTABS_MaxTabWidth) )
        Config->max_tab_width = config->max_tab_width;

    if( config->unfocused_style )
        set_string_value( &(Config->unfocused_style), mystrdup(config->unfocused_style), NULL, 0 );
    if( config->focused_style )
        set_string_value( &(Config->focused_style), mystrdup(config->focused_style), NULL, 0 );
    if( config->sticky_style )
        set_string_value( &(Config->sticky_style), mystrdup(config->sticky_style), NULL, 0 );

    if( get_flags(config->set_flags, WINTABS_Align) )
        Config->name_aligment = config->name_aligment;
    if( get_flags(config->set_flags, WINTABS_FBevel) )
        Config->fbevel = config->fbevel;
    if( get_flags(config->set_flags, WINTABS_UBevel) )
        Config->ubevel = config->ubevel;
    if( get_flags(config->set_flags, WINTABS_SBevel) )
        Config->sbevel = config->sbevel;

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
send_swallowed_configure_notify(ASWinTab *aswt)
{
    if( aswt->client_canvas )
    {
		send_canvas_configure_notify(WinTabsState.main_canvas, aswt->client_canvas );
    }
}




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
    }else if( type == M_SWALLOW_WINDOW ) 
	{
        struct ASWindowData *wd = fetch_window_by_id( body[0] );
		LOCAL_DEBUG_OUT( "SwallowWindow requested for window %lX/frame %lX, wd = %p ", body[0], body[1], wd );
		if( wd )
			do_swallow_window( wd );
	}	 
		
}

Bool
on_tabs_canvas_config()
{
	int tabs_num  = PVECTOR_USED(WinTabsState.tabs);
    ASWinTab *tabs = PVECTOR_HEAD( ASWinTab, WinTabsState.tabs );
	int tabs_changes = handle_canvas_config( WinTabsState.tabs_canvas );

    if( tabs_changes != 0 )
	{
		safe_asimage_destroy( Scr.RootImage );	  
        set_root_clip_area(WinTabsState.tabs_canvas );
	}
                    
    if( get_flags( tabs_changes, CANVAS_MOVED ) )
    {
        int i = tabs_num ;
		Bool rerender_tabs = False ;

		rerender_tabs = update_astbar_transparency(WinTabsState.banner.bar, WinTabsState.tabs_canvas, True);
        while( --i >= 0 ) 
            if( update_astbar_transparency(tabs[i].bar, WinTabsState.tabs_canvas, True) )
                rerender_tabs = True ;
		if( !rerender_tabs ) 
			tabs_changes = 0 ; 
    }    
	return tabs_changes;    
}	 

void
DispatchEvent (ASEvent * event)
{
/*    ASWindowData *pointer_wd = NULL ; */
    ASCanvas *mc = WinTabsState.main_canvas ;
    int pointer_tab = -1 ;

    SHOW_EVENT_TRACE(event);

    if( (event->eclass & ASE_POINTER_EVENTS) != 0 )
    {
		int pointer_root_x = event->x.xkey.x_root;
		int pointer_root_y = event->x.xkey.y_root;
		static int last_pointer_root_x = -1, last_pointer_root_y = -1; 
        
		pointer_tab  = find_tab_by_position( pointer_root_x, pointer_root_y );
        LOCAL_DEBUG_OUT( "pointer at %d,%d - pointer_tab = %d", event->x.xmotion.x_root, event->x.xmotion.y_root, pointer_tab );
		if( pointer_tab == BANNER_TAB_INDEX ) 
		{
            int tbar_context ;
			if( (tbar_context = check_astbar_point( WinTabsState.banner.bar, pointer_root_x, pointer_root_y )) != C_NO_CONTEXT )
			{	
	            event->context = tbar_context ;
	        	on_astbar_pointer_action( WinTabsState.banner.bar, tbar_context, 
								  		 (event->x.type == LeaveNotify),
								  		 (last_pointer_root_x != pointer_root_x || last_pointer_root_y != pointer_root_y) );
			}
		}	 
		last_pointer_root_x = pointer_root_x ;
		last_pointer_root_y = pointer_root_y ;
    
	}
    LOCAL_DEBUG_OUT( "mc.geom = %dx%d%+d%+d", mc->width, mc->height, mc->root_x, mc->root_y );
    switch (event->x.type)
    {
	    case ConfigureNotify:
            {
                int tabs_num  = PVECTOR_USED(WinTabsState.tabs);
                Bool rerender_tabs = False ;
                
                if( event->w == WinTabsState.main_window ) 
                {                        
                    ASFlagType changes = handle_canvas_config( mc );
                    if( get_flags( changes, CANVAS_RESIZED ) )
					{	
						if( tabs_num > 0 ) 
						{	
							WinTabsState.win_width = mc->width ; 
							WinTabsState.win_height = mc->height ; 
						}
                        rearrange_tabs( True );
                    }else if( get_flags( changes, CANVAS_MOVED ) )
                    {
                        int i  = tabs_num;
                        ASWinTab *tabs = PVECTOR_HEAD( ASWinTab, WinTabsState.tabs );

						rerender_tabs = on_tabs_canvas_config();
                            
                        while( --i >= 0 ) 
                        {
                            handle_canvas_config( tabs[i].client_canvas );
                            send_swallowed_configure_notify(&(tabs[i]));
                        }    
                    }    
                }else if( event->w == WinTabsState.tabs_window ) 
                {
					rerender_tabs = on_tabs_canvas_config();
                }
                if( rerender_tabs ) 
                    render_tabs( get_flags( rerender_tabs, CANVAS_RESIZED ));
            }
	        break;
        case ButtonPress:
            if( pointer_tab >= 0 ) 
                press_tab( pointer_tab );
			else if( pointer_tab == BANNER_TAB_INDEX ) 
			{
				switch( event->context ) 
				{
					case C_MenuButton :		send_swallow_command();		break ;	
					case C_CloseButton : 	close_current_tab();    	break ;	
					case C_UnswallowButton: unswallow_current_tab();   	break ;	
				}		  
			}	 
            break;
        case ButtonRelease:
            press_tab( -1 );    
            if( pointer_tab >= 0 ) 
            {
                select_tab( pointer_tab );    
            }
			break;
        case EnterNotify :
			if( event->x.xcrossing.window == Scr.Root )
			{
				withdraw_active_balloon();
            }
            break;
        case LeaveNotify :
            break;
        case MotionNotify :
            if( pointer_tab >= 0 && (event->x.xmotion.state&AllButtonMask) != 0) 
            {
                press_tab( pointer_tab );        
            }    
            break ;
        case DestroyNotify:
            on_destroy_notify(event->w);
            break;

        case ClientMessage:
            if ((event->x.xclient.format == 32) &&
                (event->x.xclient.data.l[0] == _XA_WM_DELETE_WINDOW))
			{
                DeadPipe(0);
			}
	        break;
	    case PropertyNotify:
			LOCAL_DEBUG_OUT( "property %s(%lX), _XROOTPMAP_ID = %lX, event->w = %lX, root = %lX", XGetAtomName(dpy, event->x.xproperty.atom), event->x.xproperty.atom, _XROOTPMAP_ID, event->w, Scr.Root );
			handle_wmprop_event (Scr.wmprops, &(event->x));
            if( event->x.xproperty.atom == _AS_BACKGROUND )
            {
                LOCAL_DEBUG_OUT( "root background updated!%s","");
                safe_asimage_destroy( Scr.RootImage );
                Scr.RootImage = NULL ;
            }else if( event->x.xproperty.atom == _AS_STYLE )
			{
                int i  = PVECTOR_USED(WinTabsState.tabs);
                ASWinTab *tabs = PVECTOR_HEAD( ASWinTab, WinTabsState.tabs );
                
                LOCAL_DEBUG_OUT( "AS Styles updated!%s","");
				mystyle_list_destroy_all(&(Scr.Look.styles_list));
				LoadColorScheme();
				CheckConfigSanity();
				/* now we need to update everything */
                            
                while( --i >= 0 ) 
                    set_tab_look( &(tabs[i]), False);
				set_tab_look( &(WinTabsState.banner), True);
                rearrange_tabs(False );
             }
			break;
    }
}

/********************************************************************/
/* showing our main window :                                        */
/********************************************************************/
void
show_hint( Bool redraw )
{		   
	char *banner_text ;
	if( Config->pattern ) 
	{	
		banner_text = safemalloc( 16 + strlen(Config->pattern) + 1 );
		sprintf( banner_text, "Waiting for %s", Config->pattern );
	}else
	{
		banner_text = safemalloc( 64 );
		sprintf( banner_text, "Waiting for SwallowWindow command" );
	}	 
	add_astbar_label( WinTabsState.banner.bar, 0, 0, 0, Config->name_aligment, Config->h_spacing, Config->v_spacing, banner_text, 0);
	free( banner_text );	

	if( redraw ) 
		rearrange_tabs( False );
}

Window
make_wintabs_window()
{
	Window        w;
	XSizeHints    shints;
	ExtendedWMHints extwm_hints ;
	int x, y ;
    unsigned int width = max(Config->geometry.width,1);
    unsigned int height = max(Config->geometry.height,1);
    XSetWindowAttributes attributes;
	MyButton *buttons[3] ;
	int buttons_num ;

    attributes.background_pixmap = ParentRelative;
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
    LOCAL_DEBUG_OUT( "creating main window with geometry %dx%d%+d%+d", width, height, x, y );
    w = create_visual_window( Scr.asv, Scr.Root, x, y, width, height, 0, InputOutput, CWBackPixmap, &attributes);
    set_client_names( w, "WinTabs", "WINTABS", CLASS_GADGET, MyName );

    shints.flags = USPosition|USSize|PWinGravity;
    shints.win_gravity = Config->gravity ;

	extwm_hints.pid = getpid();
    extwm_hints.flags = EXTWM_PID|EXTWM_TypeASModule ;

	set_client_hints( w, NULL, &shints, AS_DoesWmDeleteWindow, &extwm_hints );

    /* we will need to wait for PropertyNotify event indicating transition
	   into Withdrawn state, so selecting event mask: */
    XSelectInput (dpy, w, PropertyChangeMask|StructureNotifyMask|SubstructureRedirectMask
                          /*|ButtonReleaseMask | ButtonPressMask */
                  );

	WinTabsState.banner.bar = create_astbar();
	
	set_tab_look( &WinTabsState.banner, True );

	buttons_num = 0 ;
	if( WinTabsState.menu_button.width > 0 )
		buttons[buttons_num++] = &WinTabsState.menu_button ;
	if( WinTabsState.unswallow_button.width > 0 )
		buttons[buttons_num++] = &WinTabsState.unswallow_button ;
	if( WinTabsState.close_button.width > 0 )
		buttons[buttons_num++] = &WinTabsState.close_button ;
    add_astbar_btnblock(WinTabsState.banner.bar,
  		                1, 0, 0, ALIGN_CENTER, &buttons[0], 0xFFFFFFFF, buttons_num,
                      	2, 2, 
						2,
                        TBTN_ORDER_L2R );
    set_astbar_balloon( WinTabsState.banner.bar, C_CloseButton, "Close window in current tab", AS_Text_ASCII );
	set_astbar_balloon( WinTabsState.banner.bar, C_MenuButton, "Select new window to be swallowed", AS_Text_ASCII );
	set_astbar_balloon( WinTabsState.banner.bar, C_UnswallowButton, "Unswallow (release) window in current tab", AS_Text_ASCII );

	/* this must be added after buttons, so that it will have index of 1 */
	show_hint( False );


	return w ;
}

Window
make_tabs_window( Window parent )
{
	static XSetWindowAttributes attr ;
    Window w ;
	attr.event_mask = WINTABS_TAB_EVENT_MASK ;
    w = create_visual_window( Scr.asv, parent, 0, 0, 1, 1, 0, InputOutput, CWEventMask, &attr );
    return w;
}
/**************************************************************************
 * add/remove a tab code
 **************************************************************************/
void
set_tab_look( ASWinTab *aswt, Bool no_bevel )
{
	if( no_bevel ) 
	{
		set_astbar_hilite( aswt->bar, BAR_STATE_UNFOCUSED, NO_HILITE|NO_HILITE_OUTLINE );
		set_astbar_hilite( aswt->bar, BAR_STATE_FOCUSED,   NO_HILITE|NO_HILITE_OUTLINE );
	}else
	{	
		set_astbar_hilite( aswt->bar, BAR_STATE_UNFOCUSED, Config->ubevel );
		set_astbar_hilite( aswt->bar, BAR_STATE_FOCUSED,   Config->fbevel );
	}
	set_astbar_composition_method( aswt->bar, BAR_STATE_UNFOCUSED, Config->ucm );
	set_astbar_composition_method( aswt->bar, BAR_STATE_FOCUSED,   Config->fcm );
	set_astbar_style_ptr (aswt->bar, BAR_STATE_UNFOCUSED, Scr.Look.MSWindow[BACK_UNFOCUSED]);
	set_astbar_style_ptr (aswt->bar, BAR_STATE_FOCUSED,   Scr.Look.MSWindow[BACK_FOCUSED]);
}

ASWinTab *
add_tab( Window client, const char *name, INT32 encoding )
{
	ASWinTab aswt ;

	if( AS_ASSERT(client) )
		return NULL ;
	aswt.client = client ;
	aswt.name = mystrdup(name);

	aswt.bar = create_astbar();
	set_tab_look( &aswt, False );
	add_astbar_label( aswt.bar, 0, 0, 0, Config->name_aligment, Config->h_spacing, Config->v_spacing, name, encoding);

	append_vector( WinTabsState.tabs, &aswt, 1 );

	delete_astbar_tile( WinTabsState.banner.bar, BANNER_LABEL_IDX );

    return PVECTOR_TAIL(ASWinTab,WinTabsState.tabs)-1;
}

void
delete_tab( int index ) 
{
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
    if( WinTabsState.selected_tab == index ) 
    {    
        WinTabsState.selected_tab = -1 ;
        select_tab( (index > 0) ? index-1:index+1 );
    }
    if( WinTabsState.pressed_tab == index ) 
    {    
        WinTabsState.pressed_tab = -1 ;
    }
    destroy_astbar( &(tabs[index].bar) );
    XRemoveFromSaveSet (dpy, tabs[index].client);
    XSelectInput (dpy, tabs[index].client, NoEventMask);
    destroy_ascanvas( &(tabs[index].client_canvas) );
    if( tabs[index].name ) 
        free( tabs[index].name );
    vector_remove_index( WinTabsState.tabs, index );

	if( PVECTOR_USED(WinTabsState.tabs) == 0 )
		show_hint( False );
}    

void
place_tabs_line( ASWinTab *tabs, int x, int y, int first, int last, int spare, int max_width, int tab_height )
{
    int i ;
    int delta = spare / (last+1-first) ;

    for( i = first ; i <= last ; ++i ) 
    {
        int width  = calculate_astbar_width( tabs[i].bar );
        if( width < Config->min_tab_width ) 
            width = Config->min_tab_width ;
        if( width > max_width )
            width = max_width ;
        width += delta ;
        spare -= delta ; 
        if( i < last ) 
            delta = spare / (last - i) ;

        set_astbar_size( tabs[i].bar, width, tab_height );
        move_astbar( tabs[i].bar, WinTabsState.tabs_canvas, x, y );
        x += width ;
    }    

    
}

void
rearrange_tabs( Bool dont_resize_window )
{
	int tab_height = 0 ;
	ASCanvas *mc = WinTabsState.main_canvas ;
	int i ;
	ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
    int tabs_num = PVECTOR_USED(WinTabsState.tabs) ;
    int x = 0, y = 0 ; 
    int max_x = mc->width ; 
    int max_y = mc->height ; 
    int max_width = Config->max_tab_width ; 
    int start = 0, start_x ;

	if( tabs_num == 0 ) 
	{
		if(	WinTabsState.pattern_wrexp != NULL || !get_flags(Config->flags, ASWT_HideWhenEmpty ) )
		{                      /* displaying banner with pattern or something else */
		}else if( get_flags( WinTabsState.flags, ASWT_StateMapped ) )  /* hiding ourselves: */ 
		{
			XEvent xev ;
		
			unmap_canvas_window( WinTabsState.main_canvas );
			/* we must follow with syntetic UnmapNotify per ICCCM :*/
			xev.xunmap.type = UnmapNotify ;
			xev.xunmap.event = Scr.Root ;
			xev.xunmap.window = WinTabsState.main_window;
			xev.xunmap.from_configure = False;
			XSendEvent( dpy, Scr.Root, False, 
			            SubstructureRedirectMask|SubstructureNotifyMask, 
						&xev );
			
			clear_flags( WinTabsState.flags, ASWT_StateMapped );
			return ;
		}
	}

	tab_height = calculate_astbar_height( WinTabsState.banner.bar );
	x = calculate_astbar_width( WinTabsState.banner.bar );
	
	if( tabs_num == 0 ) 
	{	
		max_y = tab_height ;
		if( !dont_resize_window )
			if( resize_canvas( WinTabsState.main_canvas, x, max_y ) != 0 ) 
				return;
		max_x = x = mc->width ; 
		tab_height = mc->height ;
	}else
	{	
		if( !dont_resize_window )
			if( resize_canvas( WinTabsState.main_canvas, WinTabsState.win_width, WinTabsState.win_height ) != 0 ) 
				return;
		max_x = WinTabsState.win_width ; 
		max_y = WinTabsState.win_height ;

    	i = tabs_num ;
		while( --i >= 0 )
		{
        	int height = calculate_astbar_height( tabs[i].bar );
        	if( height > tab_height )
            	tab_height = height ;
    	}
	}

    if( tab_height == 0 || max_x <= 0 || max_y <= 0 )
        return ;

	if( max_width <= 0 || max_width > max_x )
        max_width = max_x ;

    LOCAL_DEBUG_OUT( "max_x = %d, max_y = %d, max_width = %d", max_x, max_y, max_width );
		   
    set_astbar_size( WinTabsState.banner.bar, x, tab_height );
    move_astbar( WinTabsState.banner.bar, WinTabsState.tabs_canvas, 0, 0 );

	start = 0 ;
	start_x = x ;
    for( i = 0 ; i < tabs_num ; ++i ) 
    {    
        int width  = calculate_astbar_width( tabs[i].bar );
        if( width < Config->min_tab_width ) 
            width = Config->min_tab_width ;
        if( width > max_width )
            width = max_width ;
        if( x + width > max_x )
        {   
			if( i  > 0 )  
            	place_tabs_line( tabs, start_x, y, start, i - 1, max_x - x, max_width, tab_height );
            if( y + tab_height > max_y ) 
                break;
            y += tab_height ; 
            x = 0 ;
			start = i ;
			start_x = 0 ;
        }else if( i == tabs_num - 1 )
        {    
            place_tabs_line( tabs, start_x, y, start, i, max_x - (x+width), max_width, tab_height );
            x = 0 ;
			start = i+1 ;
			start_x = 0 ;
        }else
            x += width ;
    }
    if( i >= tabs_num )    
        y += tab_height ; 
    
    if( (moveresize_canvas( WinTabsState.tabs_canvas, 0, 0, max_x, y )&CANVAS_RESIZED)!= 0 );
		render_tabs(True);
    
    max_y -= y ;
	if( max_y <= 0 ) 
		max_y = 1 ;
    i = tabs_num ; 
    LOCAL_DEBUG_OUT( "moveresaizing %d client canvases to %dx%d%+d%+d", i, max_x, max_y, 0, y );
    while( --i >= 0 ) 
    {    
        moveresize_canvas( tabs[i].client_canvas, 0, y, max_x, max_y );    
    }
	
	if( !get_flags( WinTabsState.flags, ASWT_StateMapped ) )
	{
		  map_canvas_window( WinTabsState.main_canvas, True );
		  set_flags( WinTabsState.flags, ASWT_StateMapped );
	}	
}

void
render_tabs( Bool canvas_resized )
{        
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
    int i = PVECTOR_USED(WinTabsState.tabs) ;

    while( --i >= 0  )
    {
        register ASTBarData   *tbar = tabs[i].bar ;
        if( tbar != NULL )
            if( DoesBarNeedsRendering(tbar) || canvas_resized )
                render_astbar( tbar, WinTabsState.tabs_canvas );
    }
    
	if( DoesBarNeedsRendering(WinTabsState.banner.bar) || canvas_resized )
		render_astbar( WinTabsState.banner.bar, WinTabsState.tabs_canvas );

    if( is_canvas_dirty( WinTabsState.tabs_canvas ) )
	{
        update_canvas_display( WinTabsState.tabs_canvas );
	}
}

void 
select_tab( int tab )
{
    if( tab == WinTabsState.selected_tab ) 
        return ; 
    if( WinTabsState.selected_tab >= 0 && WinTabsState.selected_tab < PVECTOR_USED( WinTabsState.tabs ) )
    {
        ASWinTab *old_tab =  PVECTOR_HEAD(ASWinTab,WinTabsState.tabs)+WinTabsState.selected_tab ;
        set_astbar_focused(old_tab->bar, WinTabsState.tabs_canvas, False);
        WinTabsState.selected_tab = -1 ;
    }    
    if( tab >= 0 && tab < PVECTOR_USED( WinTabsState.tabs ) )
    {
        ASWinTab *new_tab = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs)+tab ;
        set_astbar_focused(new_tab->bar, WinTabsState.tabs_canvas, True);
        XRaiseWindow( dpy, new_tab->client );
        WinTabsState.selected_tab = tab ;
    }
}    

void 
press_tab( int tab )
{
    if( tab == WinTabsState.pressed_tab ) 
        return ; 
    if( WinTabsState.pressed_tab >= 0 && WinTabsState.pressed_tab < PVECTOR_USED( WinTabsState.tabs ) )
    {
        ASWinTab *old_tab =  PVECTOR_HEAD(ASWinTab,WinTabsState.tabs)+WinTabsState.pressed_tab ;
        set_astbar_pressed(old_tab->bar, WinTabsState.tabs_canvas, False);
        WinTabsState.pressed_tab = -1 ;
    }    
    if( tab >= 0 && tab < PVECTOR_USED( WinTabsState.tabs ) )
    {
        ASWinTab *new_tab = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs)+tab ;
        set_astbar_pressed(new_tab->bar, WinTabsState.tabs_canvas, True);
        WinTabsState.pressed_tab = tab ;
    }
}    

/**************************************************************************
 * Swallowing code
 **************************************************************************/
void
do_swallow_window( ASWindowData *wd )
{
	Window w;
    int try_num = 0 ;
    ASCanvas *nc ;
    char *name = NULL ;
	INT32 encoding ;
	ASWinTab *aswt = NULL ;

	if( wd->client == WinTabsState.main_window )
		return;

	/* we have a match */
	/* now we actually swallow the window : */
    grab_server();
    /* first lets check if window is still not swallowed : it should have no more then 2 parents before root */
    w = get_parent_window( wd->client );
    LOCAL_DEBUG_OUT( "first parent %lX, root %lX", w, Scr.Root );
	while( w == Scr.Root && ++try_num  < 10 )
	{/* we should wait for AfterSTep to complete AddWindow protocol */
	    /* do the actuall swallowing here : */
    	ungrab_server();
		sleep_a_millisec(200*try_num);
		grab_server();
		w = get_parent_window( wd->client );
		LOCAL_DEBUG_OUT( "attempt %d:first parent %lX, root %lX", try_num, w, Scr.Root );
	}
	if( w == Scr.Root )
	{
		ungrab_server();
		return ;
	}
    if( w != None )
        w = get_parent_window( w );
    LOCAL_DEBUG_OUT( "second parent %lX, root %lX", w, Scr.Root );
    if( w != Scr.Root )
	{
		ungrab_server();
		return ;
	}
    /* its ok - we can swallow it now : */
    /* create swallow object : */
	name = get_window_name( wd, ASN_Name, &encoding );
	aswt = add_tab( wd->client, name, encoding );
    LOCAL_DEBUG_OUT( "crerated new #%d for window \"%s\" client = %8.8lX", PVECTOR_USED(WinTabsState.tabs), name, wd->client );

	if( aswt == NULL )
	{
		ungrab_server();
		return ;
	}

    /* first thing - we reparent window and its icon if there is any */
    nc = aswt->client_canvas = create_ascanvas_container( wd->client );
	aswt->swallow_location.x = nc->root_x ; 
	aswt->swallow_location.y = nc->root_y ; 
	aswt->swallow_location.width = nc->width ; 
	aswt->swallow_location.height = nc->height ; 
    XReparentWindow( dpy, wd->client, WinTabsState.main_window, WinTabsState.main_canvas->width - nc->width, WinTabsState.main_canvas->height - nc->height );
    XSelectInput (dpy, wd->client, StructureNotifyMask);
    XAddToSaveSet (dpy, wd->client);

#if 0   /* TODO : implement support for icons : */
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
    LOCAL_DEBUG_OUT( "client(%lX)->icon(%lX)->current(%lX)", wd->client, wd->icon, aswb->swallowed->current->w );

#endif
    handle_canvas_config( nc );

    map_canvas_window( nc, True );
    send_swallowed_configure_notify(aswt);
    
    select_tab( PVECTOR_USED(WinTabsState.tabs)-1 );

    rearrange_tabs( False );

    ASSync(False);
    ungrab_server();
}


void
check_swallow_window( ASWindowData *wd )
{
    char *name = NULL ;
	INT32 encoding ;
	ASWinTab *aswt = NULL ;
	int i = 0;

    if( wd == NULL && !get_flags( wd->state_flags, AS_Mapped))
        return;

	if( wd->client == WinTabsState.main_window )
		return;
	/* first lets check if we have already swallowed this one : */
	i = PVECTOR_USED(WinTabsState.tabs);
	aswt = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
	while( --i >= 0 )
		if( aswt[i].client == wd->client )
			return ;

	/* now lets try and match its name : */
	name = get_window_name( wd, Config->pattern_type, &encoding );
    LOCAL_DEBUG_OUT( "name(\"%s\")->icon_name(\"%s\")->res_class(\"%s\")->res_name(\"%s\")",
                     wd->window_name, wd->icon_name, wd->res_class, wd->res_name );
	if( match_wild_reg_exp( name, WinTabsState.pattern_wrexp) != 0 )
		return ;
	
	do_swallow_window( wd );
}


void 
on_destroy_notify(Window w)
{
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
    int i = PVECTOR_USED(WinTabsState.tabs) ;
    while( --i >= 0 ) 
        if( tabs[i].client == w ) 
        {
            delete_tab( i );
            rearrange_tabs( False );
            return ; 
        }    
}    

int
find_tab_by_position( int root_x, int root_y )
{
/*    int col = WinListState.columns_num ; */
    int tabs_num  =  PVECTOR_USED(WinTabsState.tabs);
    int i = tabs_num ;
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
    ASCanvas *tc = WinTabsState.tabs_canvas ; 

    root_x -= tc->root_x ;
    root_y -= tc->root_y ;
    LOCAL_DEBUG_OUT( "pointer win( %d,%d), tc.size(%dx%d)", root_x, root_y, tc->width, tc->height );
    if( root_x  >= 0 && root_y >= 0 &&
        root_x < tc->width && root_y < tc->height )
    {
		register ASTBarData *bar = WinTabsState.banner.bar ;
        if( bar->win_x <= root_x && bar->win_x+bar->width > root_x &&
            bar->win_y <= root_y && bar->win_y+bar->height > root_y )
			return BANNER_TAB_INDEX;

		for( i = 0 ; i < tabs_num ; ++i ) 
        {
            bar = tabs[i].bar ;
            LOCAL_DEBUG_OUT( "Checking tab %d at %dx%d%+d%+d", i, bar->width, bar->height, bar->win_x, bar->win_y );
            if( bar->win_x <= root_x && bar->win_x+bar->width > root_x &&
                bar->win_y <= root_y && bar->win_y+bar->height > root_y )
                break;
        }
    }

    return i>= tabs_num ? INVALID_TAB_INDEX : i;
}

void send_swallow_command()
{
	SendTextCommand ( F_SWALLOW_WINDOW, "-", MyName, 0);
}	 

void close_current_tab()
{
	int curr = WinTabsState.selected_tab ;
    int tabs_num  =  PVECTOR_USED(WinTabsState.tabs);
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
	
	if( tabs_num > 0 && curr >= 0 && curr < tabs_num ) 
	{
		if( tabs[curr].closed ) 
			XDestroyWindow( dpy, tabs[curr].client); 	
		else
		{	
			send_wm_protocol_request(tabs[curr].client, _XA_WM_DELETE_WINDOW, CurrentTime);
			tabs[curr].closed = True ;
		}
	}		   
}	 

void unswallow_current_tab()
{
	int curr = WinTabsState.selected_tab ;
    int tabs_num  =  PVECTOR_USED(WinTabsState.tabs);
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
	
	if( tabs_num > 0 && curr >= 0 && curr < tabs_num ) 
	{
		XReparentWindow( dpy, tabs[curr].client, Scr.Root, tabs[curr].swallow_location.x, tabs[curr].swallow_location.y );
		XResizeWindow( dpy, tabs[curr].client, tabs[curr].swallow_location.width, tabs[curr].swallow_location.height );
		delete_tab( curr ); 		
		rearrange_tabs( False );
	}	
}	 



