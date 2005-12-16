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

/* #include <X11/keysym.h> */
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
	INT32 			 name_encoding ;
	Window 			 client ;

	ASTBarData 		*bar ;
	ASCanvas 		*client_canvas ;
	
	ASCanvas 		*frame_canvas ;

	Bool closed ;
	XRectangle 	swallow_location ;
	ASFlagType wm_protocols ;

	XSizeHints	hints ;
	
	time_t      last_selected ;

}ASWinTab;

typedef struct {

#define ASWT_StateMapped	(0x01<<0)
#define ASWT_StateFocused	(0x01<<1)
#define ASWT_AllDesks		(0x01<<2)
#define ASWT_Transparent	(0x01<<3)
#define ASWT_ShutDownInProgress	(0x01<<4)
#define ASWT_SkipTransients		(0x01<<5)
#define ASWT_StateShaded		(0x01<<6)
#define ASWT_StateSticky		(0x01<<7)

	ASFlagType flags ;

    Window main_window, tabs_window ;
	ASCanvas *main_canvas ;
	ASCanvas *tabs_canvas ;

	wild_reg_exp *pattern_wrexp ;
	wild_reg_exp *exclude_pattern_wrexp ;

	ASVector *tabs ;

#define BANNER_BUTTONS_IDX  	0	
#define BANNER_LABEL_IDX  		1	
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

	CARD32      my_desktop ;
	
	ASHashTable *unswallowed_apps ; 

}ASWinTabsState ;

ASWinTabsState WinTabsState = { 0 };

#define C_CloseButton 		C_TButton0
#define C_UnswallowButton 	C_TButton1
#define C_MenuButton 		C_TButton2

#define WINTABS_SWITCH_KEYCODE	  49	/* XKeysymToKeycode (dpy, XK_grave) */
#define WINTABS_SWITCH_MOD        Mod1Mask
#define WINTABS_TAB_EVENT_MASK    (ButtonReleaseMask | ButtonPressMask | \
	                               LeaveWindowMask   | EnterWindowMask | \
                                   StructureNotifyMask|PointerMotionMask| \
								   	KeyPressMask )

#define WINTABS_MESSAGE_MASK      (M_END_WINDOWLIST |M_DESTROY_WINDOW |M_SWALLOW_WINDOW| \
					   			   WINDOW_CONFIG_MASK|WINDOW_NAME_MASK|M_SHUTDOWN)
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

char *pattern_override = NULL ;
char *exclude_pattern_override = NULL ;
char *title_override = NULL, *icon_title_override = NULL ;

void CheckConfigSanity(const char *pattern_override, const char *exclude_pattern_override, 
					   const char *title_override, const char *icon_title_override);
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
void on_unmap_notify(Window w);
void select_tab( int tab );
void press_tab( int tab );
void set_tab_look( ASWinTab *aswt, Bool no_bevel );
void set_tab_title( ASWinTab *aswt );

void show_hint( Bool redraw );
void show_banner_buttons();

int find_tab_by_position( int root_x, int root_y );
void send_swallow_command();
void close_current_tab();
Bool unswallow_current_tab();
Bool unswallow_tab(int t);
void  update_focus();
Bool handle_tab_name_change( Window client );
void update_tabs_desktop();
void update_tabs_state();
void register_unswallowed_client( Window client );
Bool check_unswallowed_client( Window client ); 

/* above function may also return : */
#define BANNER_TAB_INDEX -1		   
#define INVALID_TAB_INDEX -2


void DeadPipe(int);

CommandLineOpts WinTabs_cmdl_options[8] =
{
	{NULL, "pattern","Overrides module's inclusion pattern", NULL,
	 handler_set_string, &pattern_override, 0, CMO_HasArgs },
	
	{NULL, "exclude-pattern","Overrides module's exclusion pattern", NULL,
	 handler_set_string, &exclude_pattern_override, 0, CMO_HasArgs },
	
	{NULL, "all-desks","Swallow windows from any desktop", NULL, handler_set_flag, &(WinTabsState.flags),
	 ASWT_AllDesks, 0 },
	
	{NULL, "title","Overrides module's main window title", NULL, handler_set_string,
	 &title_override, 0, CMO_HasArgs },
	
	{NULL, "icon-title","Overrides module's title in iconic state", NULL, handler_set_string,
	 &icon_title_override, 0, CMO_HasArgs },
	
	{NULL, "skip-transients","Disregard any transient window(dialog)", NULL, handler_set_flag,
	 &(WinTabsState.flags), ASWT_SkipTransients, 0 },
    
	{"tr", "transparent","keep window-background transparent", NULL, handler_set_flag,
	 &(WinTabsState.flags), ASWT_Transparent, 0 },

	{NULL, NULL, NULL, NULL, NULL, NULL, 0, 0 }
};


void
WinTabs_usage (void)
{
	printf (OPTION_USAGE_FORMAT " [--pattern <pattern>] \n\t[--exclude-pattern <pattern>]\n\t[--all-desks] [--title <title>] [--icon-title <icon_title>]\n", MyName );
	print_command_line_opt("standard_options are ", as_standard_cmdl_options, ASS_Restarting);
	print_command_line_opt("additional options are ", WinTabs_cmdl_options, 0);
	exit (0);
}

void OnDisconnect( int nonsense ) 
{
   	int i = PVECTOR_USED(WinTabsState.tabs);
LOCAL_DEBUG_OUT( "remaining clients %d", i );
    if( i > 0 )            
	{	
		clear_flags( WinTabsState.flags, ASWT_ShutDownInProgress);
		SetAfterStepDisconnected();
		window_data_cleanup();
		XSelectInput( dpy, Scr.Root, EnterWindowMask|PropertyChangeMask );
		/* Scr.wmprops->selection_window = None ; */
		/* don't really want to terminate as that will kill our clients.
		 * running without AfterStep is not really bad, except that we won't be 
		 * able to swallow any new windows */
	}else
		DeadPipe(0);
}	 


int
main( int argc, char **argv )
{
    int i ;
    register int opt;
	/* Save our program name - for error messages */
	set_DeadPipe_handler(DeadPipe);
    InitMyApp (CLASS_GADGET, argc, argv, WinTabs_usage, NULL, ASS_Restarting );
	LinkAfterStepConfig();

    set_signal_handler( SIGSEGV );
	set_flags( WinTabsState.flags, ASWT_Transparent );  /* default */
    

	
for( i = 1 ; i< argc ; ++i)
 {
	 LOCAL_DEBUG_OUT( "argv[%d] = \"%s\", original argv[%d] = \"%s\"", i, argv[i], i, MyArgs.saved_argv[i]);	  
	 if( argv[i] != NULL )
	 { 	
		 if( ( opt = match_command_line_opt( &(argv[i][0]), WinTabs_cmdl_options ) ) < 0)
			 continue;
		 
		 /* command-line-option 'opt' has been matched */
		 
		 if( get_flags ( WinTabs_cmdl_options[opt].flags, CMO_HasArgs) )
			 if( ++i >= argc)
				 continue;
		 
		 WinTabs_cmdl_options[opt].handler( argv[i], WinTabs_cmdl_options[opt].trg,
						    WinTabs_cmdl_options[opt].param);
		 
	 }
 }

    ConnectX( ASDefaultScr, EnterWindowMask );
    ConnectAfterStep ( WINTABS_MESSAGE_MASK, 0 );               /* no AfterStep */
	
	signal (SIGPIPE, OnDisconnect);
    
	signal (SIGTERM, DeadPipe);
    signal (SIGKILL, DeadPipe);
    
    balloon_init (False);
    Config = CreateWinTabsConfig ();

    LoadBaseConfig ( GetBaseOptions);
	LoadColorScheme();
	LoadConfig ("wintabs", GetOptions);
    CheckConfigSanity(pattern_override, exclude_pattern_override, title_override, icon_title_override);

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
	{
		static int already_dead = False ; 
		if( already_dead ) 	return;/* non-reentrant function ! */
		already_dead = True ;
	}

LOCAL_DEBUG_OUT( "DeadPipe%s", "" );    
	if( !get_flags( WinTabsState.flags, ASWT_ShutDownInProgress) )
	{	
		LOCAL_DEBUG_OUT( "reparenting clients back to the Root%s","" );

		while( unswallow_current_tab() );
    
		ASSync(False );
    }	  
	if( WinTabsState.main_canvas )
    	destroy_ascanvas( &WinTabsState.main_canvas );
    if( WinTabsState.main_window )
        XDestroyWindow( dpy, WinTabsState.main_window );
	ASSync(False );
	fflush(stderr);
    
	window_data_cleanup();
    FreeMyAppResources();

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
retrieve_wintabs_astbar_props()
{
	destroy_astbar_props( &(WinTabsState.tbar_props) );
	
	WinTabsState.tbar_props = get_astbar_props(Scr.wmprops );
	button_from_astbar_props( WinTabsState.tbar_props, &WinTabsState.close_button, 		C_CloseButton, 		_AS_BUTTON_CLOSE, _AS_BUTTON_CLOSE_PRESSED );
	button_from_astbar_props( WinTabsState.tbar_props, &WinTabsState.unswallow_button, 	C_UnswallowButton, 	_AS_BUTTON_MAXIMIZE, _AS_BUTTON_MAXIMIZE_PRESSED );
	button_from_astbar_props( WinTabsState.tbar_props, &WinTabsState.menu_button, 		C_MenuButton, 		_AS_BUTTON_MENU, _AS_BUTTON_MENU_PRESSED );
}	 

void
CheckConfigSanity(const char *pattern_override, const char *exclude_pattern_override, 
				  const char *title_override, const char *icon_title_override)
{
    
	int i ;
    char *default_style = safemalloc( 1+strlen(MyName)+1);
	
	default_style[0] = '*' ;
	strcpy( &(default_style[1]), MyName );

    if( Config == NULL )
        Config = CreateWinTabsConfig ();

	if( MyArgs.geometry.flags != 0 ) 
		Config->geometry = MyArgs.geometry ;
	if( pattern_override ) 
		set_string(&(Config->pattern), mystrdup(pattern_override));
	if( exclude_pattern_override ) 
		set_string(&(Config->exclude_pattern), mystrdup(exclude_pattern_override) );
   	if( title_override ) 
		set_string(&(Config->title), mystrdup(title_override) );
   	if( icon_title_override ) 
		set_string(&(Config->icon_title), mystrdup(icon_title_override) );

	if( Config->icon_title == NULL && Config->title != NULL ) 
	{
		Config->icon_title = safemalloc( strlen(Config->title)+32 );
		sprintf( Config->icon_title, "%s - iconic", Config->title );
	}	 

	if( Config->pattern != NULL )
	{
		WinTabsState.pattern_wrexp = compile_wild_reg_exp( Config->pattern ) ;
		if( Config->exclude_pattern )
		{
LOCAL_DEBUG_OUT( "exclude_pattern = \"%s\"", Config->exclude_pattern );
			WinTabsState.exclude_pattern_wrexp = compile_wild_reg_exp( Config->exclude_pattern ) ;
		}
	}else
	{
		show_warning( "Empty Pattern requested for windows to be captured and tabbed - will wait for swallow command");
	}

	if( get_flags(Config->set_flags, WINTABS_AllDesks ) )
	{	
		if( get_flags(Config->flags, WINTABS_AllDesks ) )
			set_flags( WinTabsState.flags, ASWT_AllDesks );		
	}
	if( get_flags(Config->set_flags, WINTABS_SkipTransients ) )
	{	
		if( get_flags(Config->flags, WINTABS_SkipTransients ) )
			set_flags( WinTabsState.flags, ASWT_SkipTransients );		
	}


    if( !get_flags(Config->geometry.flags, WidthValue) )
		Config->geometry.width = 640 ;
    if( !get_flags(Config->geometry.flags, HeightValue) )
		Config->geometry.height = 480 ;
	
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

    retrieve_wintabs_astbar_props();
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
	
	balloon_config2look( &(Scr.Look), Config->balloon_conf, "*WinTabsBalloon" );
    set_balloon_look( Scr.Look.balloon_look );

}


void
GetBaseOptions (const char *filename)
{
	ReloadASEnvironment( NULL, NULL, NULL, False, True );
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
        set_string( &(Config->pattern), mystrdup(config->pattern) );
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
        set_string( &(Config->unfocused_style), mystrdup(config->unfocused_style) );
    if( config->focused_style )
        set_string( &(Config->focused_style), mystrdup(config->focused_style) );
    if( config->sticky_style )
        set_string( &(Config->sticky_style), mystrdup(config->sticky_style) );

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
        if( (res == WP_DataCreated || res == WP_DataChanged) && WinTabsState.pattern_wrexp != NULL )
        {
			if( wd->window_name != NULL )  /* must wait for all the names transferred */ 
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
	}else if( type == M_SHUTDOWN )
	{
		set_flags( WinTabsState.flags, ASWT_ShutDownInProgress);
		if( get_module_out_fd() >= 0 ) 
			close( get_module_out_fd() );
		OnDisconnect(0);
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
        int i = tabs_num ;
		Bool rerender_tabs = False ;

		safe_asimage_destroy( Scr.RootImage );	  
        set_root_clip_area(WinTabsState.tabs_canvas );

		rerender_tabs = update_astbar_transparency(WinTabsState.banner.bar, WinTabsState.tabs_canvas, True);
        while( --i >= 0 ) 
            if( update_astbar_transparency(tabs[i].bar, WinTabsState.tabs_canvas, True) )
                rerender_tabs = True ;
		if( !rerender_tabs ) 
			clear_flags(tabs_changes, CANVAS_MOVED); 
    }    
	return tabs_changes;    
}	 

Bool
recheck_swallow_windows(void *data, void *aux_data)
{
	ASWindowData *wd = (ASWindowData *)data;
	check_swallow_window( wd );			
	return True;
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
	
		if( (event->eclass & ASE_POINTER_EVENTS) != 0 && is_balloon_click( &(event->x) ) )
		{
			withdraw_balloon(NULL);
			return;
		}

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
		case ReparentNotify :
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
        case KeyPress:
			{
				XKeyEvent *xk = &(event->x.xkey);	
				if( xk->keycode == WINTABS_SWITCH_KEYCODE && xk->state == WINTABS_SWITCH_MOD ) 
				{	
					if( WinTabsState.selected_tab+1 < PVECTOR_USED( WinTabsState.tabs ) )
				 		select_tab( WinTabsState.selected_tab+1 );    		
					else
						select_tab( 0 );    		   
					/* XBell (dpy, event->scr->screen); */
				}else if( xk->window != WinTabsState.tabs_window ) 
				{	
					if( WinTabsState.selected_tab >= 0 && WinTabsState.selected_tab < PVECTOR_USED( WinTabsState.tabs ) )
    				{
        				ASWinTab *old_tab =  PVECTOR_HEAD(ASWinTab,WinTabsState.tabs)+WinTabsState.selected_tab ;
	
						xk->window = old_tab->client;
        	    		XSendEvent (dpy, xk->window, False, KeyPressMask, &(event->x));
					}
				}

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
		case FocusIn :
			set_flags(WinTabsState.flags, ASWT_StateFocused );
			update_focus();
		    break ;
		case FocusOut : clear_flags(WinTabsState.flags, ASWT_StateFocused );
		    break ;
        case MotionNotify :
            if( pointer_tab >= 0 && (event->x.xmotion.state&AllButtonMask) != 0) 
            {
                press_tab( pointer_tab );        
            }    
            break ;
        case UnmapNotify:
            on_unmap_notify(event->w);
            break;
        case DestroyNotify:
            on_destroy_notify(event->w);
            break;
		case Expose :
#if 1
			{	  
                int i = PVECTOR_USED(WinTabsState.tabs);
                ASWinTab *tabs = PVECTOR_HEAD( ASWinTab, WinTabsState.tabs );
				while( ASCheckTypedWindowEvent( event->w, Expose, &(event->x) ) );
				while( --i >= 0 ) 
				{	
					if( event->w == tabs[i].frame_canvas->w ) 
					{
						XSetBackground(dpy, Scr.DrawGC, Scr.asv->black_pixel);
						XFillRectangle(dpy, tabs[i].frame_canvas->w, Scr.DrawGC, 0, 0, tabs[i].frame_canvas->width, tabs[i].frame_canvas->height);
						break;	
					}	 
				}
			}
#endif
		    break ;
        case ClientMessage:
            if ( event->x.xclient.format == 32 )
			{	
				if( event->x.xclient.data.l[0] == _XA_WM_DELETE_WINDOW )
			    	DeadPipe(0);
			}
	        break;
	    case PropertyNotify:
			LOCAL_DEBUG_OUT( "property %s(%lX), _XROOTPMAP_ID = %lX, event->w = %lX, root = %lX", XGetAtomName(dpy, event->x.xproperty.atom), event->x.xproperty.atom, _XROOTPMAP_ID, event->w, Scr.Root );
			if( event->w == Scr.Root || event->w == Scr.wmprops->selection_window ) 
			{
				if( event->w == Scr.Root && event->x.xproperty.atom == _XA_WIN_SUPPORTING_WM_CHECK )
				{
			    	destroy_wmprops( Scr.wmprops, False );
					Scr.wmprops = setup_wmprops( &Scr, 0, 0xFFFFFFFF, NULL );
					retrieve_wintabs_astbar_props();
					show_banner_buttons();
				}else 
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
					CheckConfigSanity(NULL, NULL, NULL, NULL);
					/* now we need to update everything */
                	while( --i >= 0 ) 
                    	set_tab_look( &(tabs[i]), False);
					set_tab_look( &(WinTabsState.banner), True);
                	rearrange_tabs(False );
            	}else if( event->x.xproperty.atom == _AS_TBAR_PROPS )
				{
                	int i = PVECTOR_USED(WinTabsState.tabs);
                	ASWinTab *tabs = PVECTOR_HEAD( ASWinTab, WinTabsState.tabs );
		 			retrieve_wintabs_astbar_props();		
                
					show_banner_buttons();
				
					set_tab_look( &(WinTabsState.banner), False);
					if( i == 0 ) 
						show_hint(True);
				
					while( --i >= 0 ) 
					{	
                    	set_tab_look( &(tabs[i]), False);
						set_tab_title( &(tabs[i]) );
					}
				
                	rearrange_tabs(False );
            	}else if( event->x.xproperty.atom == _AS_MODULE_SOCKET && get_module_out_fd() < 0 )
				{
					if( ConnectAfterStep(WINTABS_MESSAGE_MASK, 0) >= 0)
					{
							  
						SendInfo ("Send_WindowList", 0);
						XSelectInput( dpy, Scr.Root, EnterWindowMask );
					}
				}	 
			}else if(   event->w == WinTabsState.main_window )
			{
			 	if( event->x.xproperty.atom == _XA_NET_WM_DESKTOP ) 
				{
					CARD32 new_desk = read_extwm_desktop_val(WinTabsState.main_window);

					if( !get_flags( WinTabsState.flags, ASWT_AllDesks )) 
					{
						if( WinTabsState.my_desktop != new_desk )
						{	
							WinTabsState.my_desktop = new_desk ;
							if( WinTabsState.pattern_wrexp != NULL ) 
								iterate_window_data( recheck_swallow_windows, NULL);
						}
					}else
						WinTabsState.my_desktop = new_desk ;

					update_tabs_desktop();
				}else if( event->x.xproperty.atom == _XA_NET_WM_STATE ) 
				{
					ASFlagType extwm_flags = 0 ; 
					if( get_extwm_state_flags (WinTabsState.main_window, &extwm_flags) )
					{
						ASFlagType new_state = get_flags(extwm_flags, EXTWM_StateShaded)?ASWT_StateShaded:0;

						new_state |= get_flags(extwm_flags, EXTWM_StateSticky)?ASWT_StateSticky:0;
						LOCAL_DEBUG_OUT( "old_state = %lX, new_state = %lX", (WinTabsState.flags&(ASWT_StateShaded|ASWT_StateSticky)), new_state );
						if( (WinTabsState.flags&(ASWT_StateShaded|ASWT_StateSticky)) != new_state )
						{
							clear_flags( WinTabsState.flags, ASWT_StateShaded|ASWT_StateSticky );
							set_flags( WinTabsState.flags, new_state );
							update_tabs_state();
						}		  
					}		
					
				}	 
			}else if( IsNameProp(event->x.xproperty.atom) )
			{                  /* Maybe name change on the client !!! */
				handle_tab_name_change( event->w );				 		
			}	 
			break;
		default:
#ifdef XSHMIMAGE
			LOCAL_DEBUG_OUT( "XSHMIMAGE> EVENT : completion_type = %d, event->type = %d ", Scr.ShmCompletionEventType, event->x.type );
			if( event->x.type == Scr.ShmCompletionEventType )
				handle_ShmCompletion( event );
#endif /* SHAPE */
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
	int align = Config->name_aligment ;
	int h_spacing = Config->h_spacing ;
	int v_spacing = Config->v_spacing ;

	if( WinTabsState.tbar_props )
	{
		if( !get_flags( Config->set_flags, WINTABS_Align ) )
			align = WinTabsState.tbar_props->align ;
		if( !get_flags( Config->set_flags, WINTABS_H_SPACING ) )
			h_spacing = WinTabsState.tbar_props->title_h_spacing ;
		if( !get_flags( Config->set_flags, WINTABS_V_SPACING ) )
			v_spacing = WinTabsState.tbar_props->title_v_spacing ;
	}	 
	if( Config->pattern ) 
	{	
		banner_text = safemalloc( 16 + strlen(Config->pattern) + 1 );
		sprintf( banner_text, "Waiting for %s", Config->pattern );
	}else
	{
		banner_text = safemalloc( 64 );
		sprintf( banner_text, "Waiting for SwallowWindow command" );
	}	 
	delete_astbar_tile( WinTabsState.banner.bar, BANNER_LABEL_IDX );
	add_astbar_label( WinTabsState.banner.bar, 0, 0, 0, align, h_spacing, v_spacing, banner_text, 0);
	free( banner_text );	

	if( redraw ) 
		rearrange_tabs( False );
}

void
show_banner_buttons()
{
	MyButton *buttons[3] ;
	int buttons_num = 0;
	int h_border = 1 ; 
	int v_border = 1 ;
	int spacing = 1 ; 
	
	if( WinTabsState.menu_button.width > 0 )
		buttons[buttons_num++] = &WinTabsState.menu_button ;
	if( WinTabsState.unswallow_button.width > 0 )
		buttons[buttons_num++] = &WinTabsState.unswallow_button ;
	if( WinTabsState.close_button.width > 0 )
		buttons[buttons_num++] = &WinTabsState.close_button ;
	if( WinTabsState.tbar_props )
	{	
      	h_border = WinTabsState.tbar_props->buttons_h_border ;
		v_border = WinTabsState.tbar_props->buttons_v_border ;
		spacing = WinTabsState.tbar_props->buttons_spacing ;
	}
	    
	delete_astbar_tile( WinTabsState.banner.bar, -1 );
	if( buttons_num > 0 ) 
	{	
		add_astbar_btnblock(WinTabsState.banner.bar,
  			            	1, 0, 0, ALIGN_CENTER, &buttons[0], 0xFFFFFFFF, buttons_num,
            	        	h_border, v_border, spacing, TBTN_ORDER_L2R );
    	set_astbar_balloon( WinTabsState.banner.bar, C_CloseButton, "Close window in current tab", AS_Text_ASCII );
		set_astbar_balloon( WinTabsState.banner.bar, C_MenuButton, "Select new window to be swallowed", AS_Text_ASCII );
		set_astbar_balloon( WinTabsState.banner.bar, C_UnswallowButton, "Unswallow (release) window in current tab", AS_Text_ASCII );
	}
   	if( PVECTOR_USED(WinTabsState.tabs) == 0 )
		show_hint(False);

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
    w = create_visual_window( Scr.asv, Scr.Root, x, y, 1, 1, 0, InputOutput, CWBackPixmap, &attributes);
    set_client_names( w, Config->title?Config->title:"WinTabs", 
						 Config->icon_title?Config->icon_title:"WinTabs - iconic", 
					  	 CLASS_WINTABS, MyName );

    shints.flags = USPosition|USSize|PWinGravity;
    shints.win_gravity = Config->gravity ;

	extwm_hints.pid = getpid();
    extwm_hints.flags = EXTWM_PID|EXTWM_TypeASModule|EXTWM_TypeNormal ;

	set_client_hints( w, NULL, &shints, AS_DoesWmDeleteWindow, &extwm_hints );
	set_client_cmd (w);

    /* we will need to wait for PropertyNotify event indicating transition
	   into Withdrawn state, so selecting event mask: */
    XSelectInput (dpy, w, PropertyChangeMask|StructureNotifyMask|FocusChangeMask|KeyPressMask 
                          /*|ButtonReleaseMask | ButtonPressMask */
                  );

	WinTabsState.banner.bar = create_astbar();
	
	set_tab_look( &WinTabsState.banner, True );
	show_banner_buttons();
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
	XGrabKey( dpy, WINTABS_SWITCH_KEYCODE, WINTABS_SWITCH_MOD, w, True, GrabModeAsync, GrabModeAsync);
    return w;
}

Window
make_frame_window( Window parent )
{
	static XSetWindowAttributes attr ;
	ASFlagType attr_mask ;
    Window w ;
	attr.event_mask = SubstructureRedirectMask|FocusChangeMask|KeyPressMask ;
	attr_mask = CWEventMask ;
	if( get_flags( WinTabsState.flags, ASWT_Transparent ) )
	{
		attr.background_pixmap = ParentRelative;
		attr_mask |= CWBackPixmap ;
		attr.event_mask |= ExposureMask ;
		LOCAL_DEBUG_OUT( "Is transparent %s", "" );
	}else
	{		 
		attr.background_pixel = Scr.asv->white_pixel;
		attr_mask |= CWBackPixel ;
	}
    w = create_visual_window( Scr.asv, parent, 0, 0, WinTabsState.win_width, WinTabsState.win_height, 0, InputOutput, attr_mask, &attr );
	XGrabKey( dpy, WINTABS_SWITCH_KEYCODE, WINTABS_SWITCH_MOD, w, True, GrabModeAsync, GrabModeAsync);
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
		if( get_flags( Config->set_flags, WINTABS_FBevel ) || WinTabsState.tbar_props == NULL )
			set_astbar_hilite( aswt->bar, BAR_STATE_FOCUSED,   Config->fbevel );
		else
			set_astbar_hilite( aswt->bar, BAR_STATE_FOCUSED,   WinTabsState.tbar_props->bevel );
		if( get_flags( Config->set_flags, WINTABS_UBevel ) || WinTabsState.tbar_props == NULL)
			set_astbar_hilite( aswt->bar, BAR_STATE_UNFOCUSED, Config->ubevel );
		else
			set_astbar_hilite( aswt->bar, BAR_STATE_UNFOCUSED, WinTabsState.tbar_props->bevel );
	}
	set_astbar_composition_method( aswt->bar, BAR_STATE_UNFOCUSED, Config->ucm );
	set_astbar_composition_method( aswt->bar, BAR_STATE_FOCUSED,   Config->fcm );
	set_astbar_style_ptr (aswt->bar, BAR_STATE_UNFOCUSED, Scr.Look.MSWindow[BACK_UNFOCUSED]);
	set_astbar_style_ptr (aswt->bar, BAR_STATE_FOCUSED,   Scr.Look.MSWindow[BACK_FOCUSED]);
}

void
set_tab_title( ASWinTab *aswt )
{
	int align = Config->name_aligment ;
	int h_spacing = Config->h_spacing ;
	int v_spacing = Config->v_spacing ;

	if( WinTabsState.tbar_props )
	{
		if( !get_flags( Config->set_flags, WINTABS_Align ) )
			align = WinTabsState.tbar_props->align ;
		if( !get_flags( Config->set_flags, WINTABS_H_SPACING ) )
			h_spacing = WinTabsState.tbar_props->title_h_spacing ;
		if( !get_flags( Config->set_flags, WINTABS_V_SPACING ) )
			v_spacing = WinTabsState.tbar_props->title_v_spacing ;
	}	 
	
	delete_astbar_tile( aswt->bar, 0 );
	add_astbar_label( aswt->bar, 0, 0, 0, align, h_spacing, v_spacing, aswt->name, aswt->name_encoding);
}

ASWinTab *
add_tab( Window client, const char *name, INT32 encoding )
{
	ASWinTab aswt ;

	if( AS_ASSERT(client) )
		return NULL ;
	aswt.client = client ;
	aswt.name = mystrdup(name);
	aswt.name_encoding = encoding;
	
	aswt.bar = create_astbar();
	set_tab_look( &aswt, False );
	set_tab_title( &aswt );
	append_vector( WinTabsState.tabs, &aswt, 1 );

	delete_astbar_tile( WinTabsState.banner.bar, BANNER_LABEL_IDX );

    return PVECTOR_TAIL(ASWinTab,WinTabsState.tabs)-1;
}

void
delete_tab( int index ) 
{
	int tabs_num = PVECTOR_USED(WinTabsState.tabs) ;
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
	int tab_to_select = -1 ; 
	if( index >= tabs_num ) 
		return ;
    if( WinTabsState.selected_tab == index ) 
    {    
		int i;
        WinTabsState.selected_tab = -1 ;
		for( i = 0 ; i < tabs_num ; ++i ) 
			if( i != index )
			{
				if( tab_to_select >= 0 ) 
					if( tabs[tab_to_select].last_selected >= tabs[i].last_selected )
						continue;
				tab_to_select = i ;
			}
    }
    if( WinTabsState.pressed_tab == index ) 
    {    
        WinTabsState.pressed_tab = -1 ;
    }
    destroy_astbar( &(tabs[index].bar) );
    XSelectInput (dpy, tabs[index].client, NoEventMask);
    destroy_ascanvas( &(tabs[index].client_canvas) );
	XDestroyWindow(dpy, tabs[index].frame_canvas->w );
	destroy_ascanvas( &(tabs[index].frame_canvas) );
    XRemoveFromSaveSet (dpy, tabs[index].client);
    if( tabs[index].name ) 
        free( tabs[index].name );
    vector_remove_index( WinTabsState.tabs, index );

	if( PVECTOR_USED(WinTabsState.tabs) == 0 )
	{
		if( get_module_out_fd() < 0 ) 
			DeadPipe(0);
		show_hint( False );
	}else if( tab_to_select >= 0 )
		select_tab( tab_to_select );
}    

void
place_tabs_line( ASWinTab *tabs, int x, int y, int first, int last, int spare, int max_width, int tab_height )
{
    int i ;
    int delta = 0;
	
	if( last+1 > first  ) 
		delta = spare / (last+1-first) ;

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
moveresize_client( ASWinTab *aswt, int x, int y, int width, int height )
{
	int frame_width = width, frame_height = height ;
	if( get_flags( aswt->hints.flags, AS_MaxSize ) )
	{
		if( aswt->hints.max_width < width ) 
			width = aswt->hints.max_width ;
		if( aswt->hints.max_height < height ) 
			height = aswt->hints.max_height ;
	}	 
	
	if( get_flags( aswt->hints.flags, AS_SizeInc ) )
	{
		int min_w = 0, min_h = 0 ; 
		if( aswt->hints.width_inc == 0 ) 
			aswt->hints.width_inc = 1;
		if( aswt->hints.height_inc == 0 ) 
			aswt->hints.height_inc = 1;
		if( get_flags( aswt->hints.flags, AS_MinSize ) )
		{
			min_w = aswt->hints.min_width ;
			min_h = aswt->hints.min_height ;
		}	 
		if( width > min_w && aswt->hints.width_inc < width  ) 
		{
			width = min_w + ((width - min_w)/aswt->hints.width_inc)*aswt->hints.width_inc ;
		}

		if( height > min_h && aswt->hints.height_inc < height  ) 
			height = min_h + ((height - min_h)/aswt->hints.height_inc)*aswt->hints.height_inc ;
	}	 

	moveresize_canvas( aswt->frame_canvas, 0, y, frame_width, frame_height );    
    moveresize_canvas( aswt->client_canvas, (frame_width - width)/2, (frame_height - height)/2, width, height );
	if( !get_flags( WinTabsState.flags, ASWT_Transparent ) )
	{	
		XSetWindowBackground( dpy, aswt->frame_canvas->w, Scr.asv->black_pixel );
		XClearWindow( dpy, aswt->frame_canvas->w );
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
        }
		
		if( i == tabs_num - 1 )
        {    
            place_tabs_line( tabs, start_x, y, start, i, max_x - (x+width), max_width, tab_height );
            x = 0 ;
			start = i+1 ;
			start_x = 0 ;
        }
        
		x += width ;
    }
    if( i >= tabs_num )    
        y += tab_height ; 
    
    moveresize_canvas( WinTabsState.tabs_canvas, 0, 0, max_x, y );
	render_tabs(True);
    
    max_y -= y ;
	if( max_y <= 0 ) 
		max_y = 1 ;
    i = tabs_num ; 
    LOCAL_DEBUG_OUT( "moveresaizing %d client canvases to %dx%d%+d%+d", i, max_x, max_y, 0, y );
    while( --i >= 0 ) 
    {    
		moveresize_client( &(tabs[i]), 0, y, max_x, max_y );
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
	ASWinTab *new_tab ;
	ASStatusHints status = {0};
	ASFlagType default_status_flags = get_flags(WinTabsState.flags,ASWT_StateShaded)?AS_Shaded:0;

	default_status_flags |= get_flags(WinTabsState.flags,ASWT_StateSticky)?AS_Sticky:0;

    if( tab == WinTabsState.selected_tab ) 
        return ; 
    if( WinTabsState.selected_tab >= 0 && WinTabsState.selected_tab < PVECTOR_USED( WinTabsState.tabs ) )
    {
        ASWinTab *old_tab =  PVECTOR_HEAD(ASWinTab,WinTabsState.tabs)+WinTabsState.selected_tab ;
		status.flags = AS_Hidden|default_status_flags ; 
		set_client_state (old_tab->client, &status);
        set_astbar_focused(old_tab->bar, WinTabsState.tabs_canvas, False);
        WinTabsState.selected_tab = -1 ;
    }    
    if( tab < 0 || tab >= PVECTOR_USED( WinTabsState.tabs ) )
		tab = 0 ;
		
    new_tab = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs)+tab ;
	status.flags = default_status_flags ; 
	set_client_state (new_tab->client, &status);
    set_astbar_focused(new_tab->bar, WinTabsState.tabs_canvas, True);
    XRaiseWindow( dpy, new_tab->frame_canvas->w );
    WinTabsState.selected_tab = tab ;
	new_tab->last_selected = time(NULL);
	ASSync(False);
	update_focus();
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
check_wm_protocols( ASWinTab *aswt )
{
	ASRawHints hints ;
	memset( &hints, 0x00, sizeof(ASRawHints));
 	read_wm_protocols ( &hints, aswt->client);
	aswt->wm_protocols = hints.wm_protocols ;
	
}

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
	aswt->frame_canvas = create_ascanvas_container( make_frame_window(WinTabsState.main_window) );
	aswt->swallow_location.x = nc->root_x ; 
	aswt->swallow_location.y = nc->root_y ; 
	aswt->swallow_location.width = nc->width ; 
	aswt->swallow_location.height = nc->height ; 
	aswt->hints = wd->hints ;
	aswt->hints.flags = wd->flags ;
	check_wm_protocols( aswt );
    XReparentWindow( dpy, wd->client, aswt->frame_canvas->w, WinTabsState.win_width - nc->width, WinTabsState.win_height - nc->height );
    XSelectInput (dpy, wd->client, StructureNotifyMask|PropertyChangeMask|FocusChangeMask);
    XAddToSaveSet (dpy, wd->client);
	register_unswallowed_client( wd->client );    

	XGrabKey( dpy, WINTABS_SWITCH_KEYCODE, WINTABS_SWITCH_MOD, wd->client, True, GrabModeAsync, GrabModeAsync);	  
	set_client_desktop (wd->client, WinTabsState.my_desktop );	 

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

	map_canvas_window( aswt->frame_canvas, True );
	map_canvas_window( nc, True );
    send_swallowed_configure_notify(aswt);
    
    select_tab( PVECTOR_USED(WinTabsState.tabs)-1 );

	if( !handle_tab_name_change( wd->client ) )
	    rearrange_tabs( False );

    ASSync(False);
    ungrab_server();
}


Bool check_swallow_name( char *name ) 
{	
	if( name ) 
	{		   	
		if( match_wild_reg_exp( name, WinTabsState.pattern_wrexp) == 0 )
			return True;
	}
	return False ;
}

Bool check_no_swallow_name( char *name ) 
{	
	LOCAL_DEBUG_OUT( "name = \"%s\", excl_wrexp = %p", name, WinTabsState.exclude_pattern_wrexp );
	if( name && WinTabsState.exclude_pattern_wrexp) 
	{		   	
		if( match_wild_reg_exp( name, WinTabsState.exclude_pattern_wrexp) == 0 )
			return True;
	}
	return False ;
}

void
check_swallow_window( ASWindowData *wd )
{
	INT32 encoding ;
	ASWinTab *aswt = NULL ;
	int i = 0;

    if( wd == NULL || !get_flags( wd->state_flags, AS_Mapped))
        return;

	if( wd->client == WinTabsState.main_window )
		return;
	if( get_flags( WinTabsState.flags, ASWT_SkipTransients ) )
	{
		if( get_flags( wd->flags, AS_Transient ) )
			return;
	}	 
	/* first lets check if we have already swallowed this one : */
	i = PVECTOR_USED(WinTabsState.tabs);
	aswt = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
	while( --i >= 0 )
		if( aswt[i].client == wd->client )
			return ;

	if( !get_flags( WinTabsState.flags, ASWT_AllDesks ) )
		if( read_extwm_desktop_val( wd->client ) != WinTabsState.my_desktop ) 
			return ;

	/* now lets try and match its name : */
	LOCAL_DEBUG_OUT( "name(\"%s\")->icon_name(\"%s\")->res_class(\"%s\")->res_name(\"%s\")",
                     wd->window_name, wd->icon_name, wd->res_class, wd->res_name );
	
	if( check_unswallowed_client( wd->client ) )
		return;
	if( get_flags( Config->set_flags, WINTABS_PatternType ) )
	{	
		if( !check_swallow_name(get_window_name( wd, Config->pattern_type, &encoding )) )
			return ;
	}else
	{
		if( !check_swallow_name( wd->window_name ) && 
			!check_swallow_name( wd->icon_name ) && 
			!check_swallow_name( wd->res_class ) && 
			!check_swallow_name( wd->res_name ) )
		{
			return;
		}		   
	}	 
	if( get_flags( Config->set_flags, WINTABS_ExcludePatternType ) )
	{	
		if( check_no_swallow_name(get_window_name( wd, Config->exclude_pattern_type, &encoding )) )
			return ;
	}else
	{
		if( check_no_swallow_name( wd->window_name ) ||
			check_no_swallow_name( wd->icon_name ) || 
			check_no_swallow_name( wd->res_class ) || 
			check_no_swallow_name( wd->res_name ) )
		{
			return;
		}		   
	}	 
	
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

void 
on_unmap_notify(Window w)
{
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
    int i = PVECTOR_USED(WinTabsState.tabs) ;
    while( --i >= 0 ) 
        if( tabs[i].client == w ) 
        {
			unswallow_tab( i );
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
			XKillClient( dpy, tabs[curr].client); 	
		else
		{	
			send_wm_protocol_request(tabs[curr].client, _XA_WM_DELETE_WINDOW, CurrentTime);
			tabs[curr].closed = True ;
		}
	}		   
}	 

Bool unswallow_tab(int t)
{
    int tabs_num  =  PVECTOR_USED(WinTabsState.tabs);
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
	LOCAL_DEBUG_OUT( "tab = %d, tabs_num = %d", t, tabs_num );
	if( tabs_num > 0 && t >= 0 && t < tabs_num ) 
	{
		/* XGrabKey( dpy, WINTABS_SWITCH_KEYCODE, Mod1Mask|Mod2Mask, w, True, GrabModeAsync, GrabModeAsync); */
		XDeleteProperty( dpy, tabs[t].client, _XA_NET_WM_STATE );
		XDeleteProperty( dpy, tabs[t].client, _XA_WIN_STATE );
		XDeleteProperty( dpy, tabs[t].client, _XA_NET_WM_DESKTOP );
		XDeleteProperty( dpy, tabs[t].client, _XA_WIN_WORKSPACE );
		XResizeWindow( dpy, tabs[t].client, tabs[t].swallow_location.width, tabs[t].swallow_location.height );
		XReparentWindow( dpy, tabs[t].client, Scr.Root, tabs[t].swallow_location.x, tabs[t].swallow_location.y );
		delete_tab( t ); 		
		rearrange_tabs( False );
		return True;
	}	
	return False;
}	 

Bool unswallow_current_tab()
{
	return unswallow_tab(WinTabsState.selected_tab);
}	 

void 
update_focus()
{
	int curr = WinTabsState.selected_tab ;
    int tabs_num  =  PVECTOR_USED(WinTabsState.tabs);
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);

	LOCAL_DEBUG_OUT( "curr = %d, tabs_num = %d, focused = %ld", curr, tabs_num, get_flags(WinTabsState.flags, ASWT_StateFocused ) );	   
	if( tabs_num > 0 && curr >= 0 && curr < tabs_num && get_flags(WinTabsState.flags, ASWT_StateFocused )) 
	{
		if( get_flags( tabs[curr].wm_protocols, AS_DoesWmTakeFocus ) )
			send_wm_protocol_request ( tabs[curr].client, _XA_WM_TAKE_FOCUS, Scr.last_Timestamp);
		else
			XSetInputFocus ( dpy, tabs[curr].client, RevertToParent, Scr.last_Timestamp );
	}	
}	 

Bool handle_tab_name_change( Window client)
{
    int tabs_num  =  PVECTOR_USED(WinTabsState.tabs);
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
	int i ;
	Bool changed = False ;

	for( i = 0 ; i < tabs_num ; ++i ) 
	{
		if( tabs[i].client == client  )
			break;	  
	}	 
	
	if( i < tabs_num ) 
	{
		ASRawHints    raw;
		ASHints       clean;
		ASSupportedHints *list = create_hints_list ();
		
		enable_hints_support (list, HINTS_ICCCM);
		enable_hints_support (list, HINTS_KDE);
		enable_hints_support (list, HINTS_ExtendedWM);
		
		memset( &raw, 0x00, sizeof(ASRawHints));
		memset( &clean, 0x00, sizeof(ASHints));
		
		if( collect_hints (ASDefaultScr, tabs[i].client, HINT_NAME, &raw) )
		{
			if( merge_hints (&raw, NULL, NULL, list, HINT_NAME, &clean, tabs[i].client) )
			{
				if( clean.names[0] ) 
				{
					if( strcmp( clean.names[0], tabs[i].name ) != 0 )
					{
						free( tabs[i].name );
						tabs[i].name = mystrdup( clean.names[0] );
						tabs[i].name_encoding = clean.names_encoding[0] ;
						set_tab_title( &(tabs[i]) );
						rearrange_tabs( False );
						changed = True ;
					}	 
				}	 
				destroy_hints( &clean, True );
			}
			destroy_raw_hints ( &raw, True);
		}
		destroy_hints_list( &list );		

	}	 
	return changed ;
}	 

void 
update_tabs_desktop()
{	
	int i = PVECTOR_USED(WinTabsState.tabs);
    ASWinTab *tabs = PVECTOR_HEAD( ASWinTab, WinTabsState.tabs );
	while( --i >= 0) 
		set_client_desktop (tabs[i].client, WinTabsState.my_desktop );	
}

void 
update_tabs_state()
{	
	ASStatusHints status = {0};
	int i = PVECTOR_USED(WinTabsState.tabs);
    ASWinTab *tabs = PVECTOR_HEAD( ASWinTab, WinTabsState.tabs );
	
	status.flags = get_flags(WinTabsState.flags,ASWT_StateShaded)?AS_Shaded:0;
	status.flags |= get_flags(WinTabsState.flags,ASWT_StateSticky)?AS_Sticky:0;

	while( --i >= 0) 
	{	
		if( i == WinTabsState.selected_tab ) 
			clear_flags( status.flags, AS_Hidden );
		else
			set_flags( status.flags, AS_Hidden );
		set_client_state (tabs[i].client, &status);
	}
}

void 
register_unswallowed_client( Window client ) 
{
	if( WinTabsState.unswallowed_apps == NULL ) 
		WinTabsState.unswallowed_apps = create_ashash( 0, NULL, NULL, NULL ); 	 

	add_hash_item( WinTabsState.unswallowed_apps, AS_HASHABLE(client), NULL );
}


Bool 
check_unswallowed_client( Window client ) 
{
	ASHashData hdata = {0} ;
	if( WinTabsState.unswallowed_apps != NULL ) 
		return (get_hash_item( WinTabsState.unswallowed_apps, AS_HASHABLE(client), &hdata.vptr ) == ASH_Success );
	return False;
}

