/*
 * This is the complete from scratch rewrite of the original WinList
 * module.
 *
 * Copyright (C) 2002 Sasha Vasko <sasha at aftercode.net>
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
/*  WinList local variables :                                         */
/**********************************************************************/
typedef struct ASWinListColumn
{
    ASWindowData **items ;
    unsigned int items_num ;
    unsigned int width, height ;
}ASWinListColumn;

typedef struct {
    int windows_num ;

    ASWindowData       *focused;

    Window       main_window;
    ASCanvas    *main_canvas;

    ASTBarData  *idle_bar;

    ASWindowData *window_order[MAX_WINLIST_WINDOW_COUNT];
    unsigned short bar_col[MAX_WINLIST_WINDOW_COUNT];
    unsigned short bar_row[MAX_WINLIST_WINDOW_COUNT];
    unsigned short bar_width[MAX_WINLIST_WINDOW_COUNT];
    unsigned short col_width[MAX_WINLIST_WINDOW_COUNT];
    unsigned short col_x[MAX_WINLIST_WINDOW_COUNT];

    unsigned int max_item_height ;
    unsigned int columns_num, rows_num;

    ASTBarData  *pressed_bar;

	Bool postpone_display ;
	time_t last_message_time ;

	ASTBarProps *tbar_props ;

}ASWinListState ;

ASWinListState WinListState = { 0, NULL, None, NULL, NULL };


#define WINLIST_BACK_URGENT		BACK_DEFAULT

/**********************************************************************/
/**********************************************************************/
/* Our configuration options :                                        */
/**********************************************************************/
/*char *default_unfocused_style = "unfocused_window_style";
 char *default_focused_style = "focused_window_style";
 char *default_sticky_style = "sticky_window_style";
 */

WinListConfig *Config = NULL ;


/**********************************************************************/

void retrieve_winlist_astbar_props();
void SetWinListLook();
void GetBaseOptions (const char *filename);
void GetOptions (const char *filename);
void HandleEvents();
void process_message (send_data_type type, send_data_type *body);
void DispatchEvent (ASEvent * Event);
Window make_winlist_window();
void add_winlist_button( ASTBarData *tbar, ASWindowData *wd );
void refresh_winlist_button( ASTBarData *tbar, ASWindowData *wd, Bool focus_only );
void delete_winlist_button( ASTBarData *tbar, ASWindowData *wd );
Bool rearrange_winlist_window( Bool dont_resize_main_canvas );
unsigned int find_button_by_position( int x, int y );
void press_winlist_button( ASWindowData *wd );
void release_winlist_button( ASWindowData *wd, int button );
void update_winlist_styles();
void DeadPipe(int);

int
main( int argc, char **argv )
{
    /* Save our program name - for error messages */
	set_DeadPipe_handler(DeadPipe);
    InitMyApp (CLASS_WINLIST, argc, argv, NULL, NULL, 0 );
	LinkAfterStepConfig();

    set_signal_handler( SIGSEGV );

    ConnectX( ASDefaultScr, EnterWindowMask );
    if( ConnectAfterStep (M_FOCUS_CHANGE |
                      M_DESTROY_WINDOW |
                      WINDOW_CONFIG_MASK |
                      WINDOW_NAME_MASK |
                      M_END_WINDOWLIST, 0) < 0 ) 
		exit(1);               /* no AfterStep */
    balloon_init (False);

    /* Request a list of all windows, while we load our config */
    SendInfo ("Send_WindowList", 0);

/*  LoadBaseConfig ( GetBaseOptions); */
	ReloadASEnvironment( NULL, NULL, NULL, False, True );

	LoadColorScheme();
//	ASModuleConfig *asm_config = parse_asmodule_config_all( WinListConfigClass );
	Config = AS_WINLIST_CONFIG(parse_asmodule_config_all( WinListConfigClass )/*asm_config*/);
	
/* 	LoadConfig ("winlist", GetOptions); */
	CheckWinListConfigSanity(Config, &(MyArgs.geometry), MyArgs.gravity);
	ReloadASDatabase();
	ReloadCategories(True);
    
	retrieve_winlist_astbar_props();
		
	SetWinListLook();

    WinListState.main_window = make_winlist_window();
    WinListState.main_canvas = create_ascanvas( WinListState.main_window );
    set_root_clip_area( WinListState.main_canvas );
    rearrange_winlist_window( False );

	WinListState.postpone_display = True ;

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
	static int already_dead = False ; 
	if( already_dead ) 
		return;/* non-reentrant function ! */
	already_dead = True ;
    
	window_data_cleanup();

    if( WinListState.idle_bar )
        destroy_astbar( &WinListState.idle_bar );
    if( WinListState.main_canvas )
        destroy_ascanvas( &WinListState.main_canvas );
    if( WinListState.main_window )
        XDestroyWindow( dpy, WinListState.main_window );

	destroy_astbar_props( &(WinListState.tbar_props) );
	    
	DestroyCategories();
    
	if( Config )
        destroy_ASModule_config( AS_MODULE_CONFIG(Config));

	FreeMyAppResources();

#ifdef DEBUG_ALLOCS
    print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

    XFlush (dpy);			/* need this for SetErootPixmap to take effect */
	XCloseDisplay (dpy);		/* need this for SetErootPixmap to take effect */
    exit (0);
}

void
retrieve_winlist_astbar_props()
{
	destroy_astbar_props( &(WinListState.tbar_props) );
	
	WinListState.tbar_props = get_astbar_props(Scr.wmprops );
}	 


void
SetWinListLook()
{
    int i ;
    char *default_winlist_style = safemalloc( 1+strlen(MyName)+1);
	default_winlist_style[0] = '*' ;
	strcpy( &(default_winlist_style[1]), MyName );

    mystyle_get_property (Scr.wmprops);
    for( i = 0 ; i < BACK_STYLES ; ++i )
		Scr.Look.MSWindow[i] = NULL ;

    Scr.Look.MSWindow[BACK_UNFOCUSED] = mystyle_find( Config->UnfocusedStyle );
	LOCAL_DEBUG_OUT( "Configured MyStyle %d \"%s\" is %p", BACK_UNFOCUSED, Config->UnfocusedStyle?Config->UnfocusedStyle:"(null)", Scr.Look.MSWindow[BACK_UNFOCUSED] );
    Scr.Look.MSWindow[BACK_FOCUSED] = mystyle_find( Config->FocusedStyle );
	LOCAL_DEBUG_OUT( "Configured MyStyle %d \"%s\" is %p", BACK_FOCUSED, Config->FocusedStyle?Config->FocusedStyle:"(null)", Scr.Look.MSWindow[BACK_FOCUSED] );
    Scr.Look.MSWindow[BACK_STICKY] = mystyle_find( Config->StickyStyle );
	LOCAL_DEBUG_OUT( "Configured MyStyle %d \"%s\" is %p", BACK_STICKY, Config->StickyStyle?Config->StickyStyle:"(null)", Scr.Look.MSWindow[BACK_STICKY] );
	Scr.Look.MSWindow[BACK_URGENT] = mystyle_find( Config->UrgentStyle );
	LOCAL_DEBUG_OUT( "Configured MyStyle %d \"%s\" is %p", BACK_URGENT, Config->UrgentStyle?Config->UrgentStyle:"(null)", Scr.Look.MSWindow[BACK_URGENT] );

    for( i = 0 ; i < BACK_STYLES ; ++i )
    {
        static char *default_window_style_name[BACK_STYLES] ={"focused_window_style","unfocused_window_style","sticky_window_style", NULL};
        if( Scr.Look.MSWindow[i] == NULL )
            Scr.Look.MSWindow[i] = mystyle_find( default_window_style_name[i] );
        if( Scr.Look.MSWindow[i] == NULL && i != BACK_URGENT )
            Scr.Look.MSWindow[i] = mystyle_find_or_default( default_winlist_style );
		LOCAL_DEBUG_OUT( "MyStyle %d is \"%s\"", i, Scr.Look.MSWindow[i]?Scr.Look.MSWindow[i]->name:"none" );
    }
    free( default_winlist_style );
	
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
    PrintWinListConfig (Config);
    Print_balloonConfig ( Config->asmodule_config.balloon_conf );
#endif
    balloon_config2look( &(Scr.Look), Config->asmodule_config.balloon_conf, "*WinListBalloon" );
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
#if 0
    START_TIME(option_time);
    WinListConfig *config = ParseWinListOptions( filename, MyName );

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
    PrintWinListConfig (config);
#endif
    /* Need to merge new config with what we have already :*/
	MergeWinListOptions( Config, config );

    if (config->asmodule_config.style_defs)
        ProcessMyStyleDefinitions (&(config->asmodule_config.style_defs));
	DestroyWinListConfig( config );
    SHOW_TIME("Config parsing",option_time);
#endif	
}


/****************************************************************************/
/* PROCESSING OF AFTERSTEP MESSAGES :                                       */
/****************************************************************************/
void
process_message (send_data_type type, send_data_type *body)
{
    LOCAL_DEBUG_OUT( "received message %lX", type );
	WinListState.last_message_time = time(NULL) ;

	if( type == M_END_WINDOWLIST )
	{
		WinListState.postpone_display = False ;
		rearrange_winlist_window( False );
	}else if( (type&WINDOW_PACKET_MASK) != 0 )
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );
        struct ASWindowData *saved_wd = wd ;
        ASTBarData *tbar = wd?wd->bar:NULL;
        WindowPacketResult res ;

		show_progress( "message %X window %X data %p", type, body[0], wd );
		res = handle_window_packet( type, body, &wd );
		if( res == WP_DataCreated )
        {
            if( WinListState.windows_num < MAX_WINLIST_WINDOW_COUNT &&
                WinListState.windows_num < Config->MaxRows*Config->MaxColumns )
            {
                if( !get_flags( Config->flags, ASWL_UseSkipList ) ||
                    !get_flags( wd->flags, AS_SkipWinList ) )
                    add_winlist_button( tbar, wd );
            }
        }else if( res == WP_DataChanged )
			refresh_winlist_button( tbar, wd, (type == M_FOCUS_CHANGE) );
		else if( res == WP_DataDeleted )
            delete_winlist_button( tbar, saved_wd );

	}
}

void
DispatchEvent (ASEvent * event)
{
    ASWindowData *pointer_wd = NULL ;
	static Bool root_pointer_moved = True ;
    SHOW_EVENT_TRACE(event);
	
    if( (event->eclass & ASE_POINTER_EVENTS) != 0 )
    {
        int i  = find_button_by_position( event->x.xmotion.x_root - (WinListState.main_canvas->root_x+(int)WinListState.main_canvas->bw),
                                          event->x.xmotion.y_root - (WinListState.main_canvas->root_y+(int)WinListState.main_canvas->bw) );
		if( is_balloon_click( &(event->x) ) )
		{
			withdraw_balloon(NULL);
			return;
		}
        if( i < WinListState.windows_num )
            pointer_wd = WinListState.window_order[i] ;
    }

    switch (event->x.type)
    {
	    case ConfigureNotify:
            {
                ASFlagType changes = handle_canvas_config( WinListState.main_canvas );
                if( changes != 0 )
                    set_root_clip_area( WinListState.main_canvas );

                if( get_flags( changes, CANVAS_RESIZED ) )
                    rearrange_winlist_window(True);
                else if( changes != 0 )        /* moved - update transparency ! */
                {
                    int i ;
                    for( i = 0 ; i < WinListState.windows_num ; ++i )
                        update_astbar_transparency( WinListState.window_order[i]->bar, WinListState.main_canvas, False );
                }
            }
	        break;
        case ButtonPress:
            if( pointer_wd )
                press_winlist_button( pointer_wd );
            break;
        case ButtonRelease:
            if( pointer_wd )
                release_winlist_button( pointer_wd, event->x.xbutton.button );
			break;
        case EnterNotify :
			if( event->x.xcrossing.window == Scr.Root )
			{
				withdraw_active_balloon();
				break;
			}
        case LeaveNotify :
        case MotionNotify :
			if( event->x.type == MotionNotify ) 
				root_pointer_moved = True ; 
            if( pointer_wd )
			{	
                on_astbar_pointer_action( pointer_wd->bar, 0, (event->x.type == LeaveNotify), root_pointer_moved );
				root_pointer_moved = False ; 
			}
            break ;
	    case ClientMessage:
            if ((event->x.xclient.format == 32) &&
                (event->x.xclient.data.l[0] == _XA_WM_DELETE_WINDOW))
			{
                DeadPipe(0);
			}
	        break;
	    case PropertyNotify:
			if( event->w == Scr.Root || event->w == Scr.wmprops->selection_window ) 
			{
				handle_wmprop_event (Scr.wmprops, &(event->x));
            	if( event->x.xproperty.atom == _AS_BACKGROUND )
            	{
                	int i ;
                	LOCAL_DEBUG_OUT( "root background updated!%s","");
                	safe_asimage_destroy( Scr.RootImage );
                	Scr.RootImage = NULL ;
                	for( i = 0 ; i < WinListState.windows_num ; ++i )
                    	if( update_astbar_transparency( WinListState.window_order[i]->bar, WinListState.main_canvas, True ) )
							render_astbar( WinListState.window_order[i]->bar, WinListState.main_canvas );
		        	if( is_canvas_dirty( WinListState.main_canvas ) )
					{
						LOCAL_DEBUG_OUT( "update main canvas%s","");
            			update_canvas_display( WinListState.main_canvas );
					}
            	}else if( event->x.xproperty.atom == _AS_STYLE )
				{
                	int i ;
					LOCAL_DEBUG_OUT( "AS Styles updated!%s","");
					mystyle_list_destroy_all(&(Scr.Look.styles_list));
					LoadColorScheme();
					SetWinListLook();
					/* now we need to update everything */
					update_winlist_styles();
					rearrange_winlist_window( False );
                	for( i = 0 ; i < WinListState.windows_num ; ++i )
						refresh_winlist_button( WinListState.window_order[i]->bar, WinListState.window_order[i], False );
				}else if( event->x.xproperty.atom == _AS_TBAR_PROPS )
				{
					int i ;
		 			retrieve_winlist_astbar_props();		
					update_winlist_styles();
					rearrange_winlist_window( False );
                	for( i = 0 ; i < WinListState.windows_num ; ++i )
						refresh_winlist_button( WinListState.window_order[i]->bar, WinListState.window_order[i], False );
            	}
			}else if( event->x.xproperty.atom == _XA_NET_WM_ICON )
			{                  /* Maybe name change on the client !!! */
				/*handle_tab_name_change( event->w ); */
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
Window
make_winlist_window()
{
	Window        w;
	XSizeHints    shints;
	ExtendedWMHints extwm_hints ;
	int x, y ;
    unsigned int width = max(Config->MinSize.width,1);
    unsigned int height = max(Config->MinSize.height,1);

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
    set_client_names( w, "Window List", "Window List", CLASS_WINLIST, MyName );

	shints.flags = USPosition|USSize|PMinSize|PMaxSize|PBaseSize|PWinGravity;
	shints.min_width = shints.min_height = 4;
	shints.max_width = (Config->MaxSize.width>0)?Config->MaxSize.width:Scr.MyDisplayWidth;
	shints.max_height = (Config->MaxSize.height>0)?Config->MaxSize.height:Scr.MyDisplayHeight;
	shints.base_width = shints.base_height = 4;
	shints.win_gravity = Config->gravity ;

	extwm_hints.pid = getpid();
	extwm_hints.flags = EXTWM_PID|EXTWM_TypeSet|EXTWM_StateSet ;
	extwm_hints.state_flags = EXTWM_StateSkipTaskbar|EXTWM_StateSkipPager ;
	extwm_hints.type_flags = EXTWM_TypeDock|EXTWM_TypeASModule ;

	set_client_hints( w, NULL, &shints, AS_DoesWmDeleteWindow, &extwm_hints );
	set_client_cmd (w);

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
                          EnterWindowMask|LeaveWindowMask);

	return w ;
}
/********************************************************************/
/* WinList buttons handling :                                       */
/********************************************************************/
/* Private stuff : **************************************************/

static Bool
render_winlist_button( ASTBarData *tbar )
{
	LOCAL_DEBUG_CALLER_OUT("tbar %p", tbar );
    if( render_astbar( tbar, WinListState.main_canvas ) )
	{
		LOCAL_DEBUG_OUT( "update main canvas%s","");
        update_canvas_display( WinListState.main_canvas );
		return True ;
	}
	return False ;
}

/* WE do redrawing in two steps :
  1) we need to determine the desired window size and resize it accordingly
  2) when we get StructureNotify event - we need to reposition and redraw
     everything accordingly
 */
void
postponed_rearrange_winlist( void *vdata )
{
	Bool dont_resize_main_canvas  = (Bool)vdata ;	
	rearrange_winlist_window( dont_resize_main_canvas );	
}

Bool
rearrange_winlist_window( Bool dont_resize_main_canvas )
{
    int i ;
    unsigned int allowed_max_width = (Config->MaxSize.width==0)?Scr.MyDisplayWidth:Config->MaxSize.width ;
	unsigned int allowed_max_height = (Config->MaxSize.height==0)?Scr.MyDisplayHeight:Config->MaxSize.height ;
    unsigned int allowed_min_width = Config->MinSize.width ;
    unsigned int allowed_min_height = Config->MinSize.height;
    unsigned int max_col_width = (Config->MaxColWidth==0)?Scr.MyDisplayWidth:Config->MaxColWidth ;
    unsigned int max_item_height = 0;
    unsigned int max_rows = 0 ;
    int row = 0, col = 0;
    unsigned int total_width = 0, total_height = 0 ;
    int y ;

    LOCAL_DEBUG_CALLER_OUT( "%sresize canvas. windows_num = %d",
                            dont_resize_main_canvas?"Don't ":"Do ", WinListState.windows_num );
    
	if( dont_resize_main_canvas )
    {
        LOCAL_DEBUG_OUT( "Main_canvas geometry = %dx%d", WinListState.main_canvas->width, WinListState.main_canvas->height );
        allowed_min_width  = allowed_max_width  = WinListState.main_canvas->width ;
        allowed_min_height = allowed_max_height = WinListState.main_canvas->height ;
    }else
    {
        if( allowed_max_width > Scr.MyDisplayWidth )
            allowed_max_width = Scr.MyDisplayWidth ;
        if( allowed_max_height > Scr.MyDisplayHeight )
            allowed_max_height = Scr.MyDisplayHeight ;
        if( allowed_min_width > allowed_max_width )
            allowed_min_width = allowed_max_width ;
        if( allowed_min_height > allowed_max_height )
            allowed_min_height = allowed_max_height ;
    }

    LOCAL_DEBUG_OUT( "allowed_min_size=%dx%d; allowed_max_size==%dx%d",
                     allowed_min_width, allowed_min_height, allowed_max_width, allowed_max_height );

    if( WinListState.windows_num == 0 || WinListState.window_order == NULL )
    {
        if( !dont_resize_main_canvas )
        {
            int width = max(1,allowed_min_width);
            int height = max(1,allowed_min_height);
            Bool invalid = ( height != WinListState.main_canvas->height || width != WinListState.main_canvas->width );
            resize_canvas( WinListState.main_canvas, width, height );
            if( invalid )
                return False;
        }
        if( allowed_min_width > 1 && allowed_min_height > 1 )
        {
            if( WinListState.idle_bar == NULL )
            {
                int flip = (allowed_min_height > allowed_min_width)?FLIP_VERTICAL:0;
                char *banner = safemalloc( 9+1+2+1+strlen(VERSION)+1);
                WinListState.idle_bar = create_astbar();
                sprintf( banner, "AfterStep v. %s", VERSION );
                add_astbar_label( WinListState.idle_bar, 0, 0, flip, ALIGN_CENTER, 0, 0, banner, AS_Text_ASCII );
                free( banner );
                set_astbar_style_ptr( WinListState.idle_bar, BAR_STATE_UNFOCUSED, Scr.Look.MSWindow[BACK_UNFOCUSED] );
                set_astbar_style_ptr( WinListState.idle_bar, BAR_STATE_FOCUSED, Scr.Look.MSWindow[BACK_FOCUSED] );
            }
            set_astbar_size( WinListState.idle_bar, allowed_min_width, allowed_min_height );
            move_astbar( WinListState.idle_bar, NULL, 0, 0 );
            render_astbar( WinListState.idle_bar, WinListState.main_canvas );
			LOCAL_DEBUG_OUT( "update main canvas%s","");
            update_canvas_display( WinListState.main_canvas );
        }
        return False;
    }

    if( max_col_width > allowed_max_width )
        max_col_width = allowed_max_width ;

    /* Pass 1: determining size of each individual bar, as well as max height of any bar */
    for( i = 0 ; i < WinListState.windows_num ; ++i )
	{
        ASTBarData   *tbar = WinListState.window_order[i]->bar ;
        int width, height ;
        if( tbar == NULL )
        {
            WinListState.bar_width[i] = 0 ;
            continue;
        }

        width = calculate_astbar_width( tbar );
        height = calculate_astbar_height( tbar );
        if( width == 0 )
            width = 1 ;
        if( height == 0 )
            height = 1 ;

        if( width > max_col_width )
            width = max_col_width ;
        WinListState.bar_width[i] = width ;
        if( height > max_item_height )
            max_item_height = height ;
    }
	LOCAL_DEBUG_OUT( "calculated max_item_height = %d", max_item_height );
	if( max_item_height <= 0 ) 
		max_item_height = 1 ;
    max_rows = (allowed_max_height + max_item_height - 1 ) / max_item_height ;
    if( max_rows > Config->MaxRows )
        max_rows = Config->MaxRows ;

    /* Pass 2: we have to decide on number of rows/columns : */
    WinListState.columns_num = WinListState.rows_num = 0 ;
    if( get_flags( Config->flags, ASWL_RowsFirst ) )
    { /* tricky layout policy, since column width may change when new element added/removed to it
       * To resolve that race condition - lets try and find maximum number of columns that can fit into
       * allowed_max_width  - we start with min(windows_num,max_columns) columns and then decrease this number untill
       * we get a fit*/
        int columns_num = min(WinListState.windows_num,Config->MaxColumns);
        int max_row_width = 0;
        int min_columns = (WinListState.windows_num + max_rows - 1)/ max_rows ;
        if( min_columns <= 0 )
            min_columns = 1 ;
        while( columns_num > min_columns )
        {
            int row_width = 0 ;
            for( i = 0 ; i < WinListState.windows_num ; ++i )
            {
                row_width += WinListState.bar_width[i] ;
                if( ++col >= columns_num )
                {
                    col = 0 ;
                    if( max_row_width < row_width )
                        max_row_width = row_width ;
                }
            }
            if( max_row_width <= allowed_max_width )
                break;
            --columns_num;
        }
        WinListState.columns_num = columns_num ;
        WinListState.rows_num = (WinListState.windows_num+columns_num-1)/columns_num ;
    }else
    {
        /* this layout strategy is simplier - since all the bars will have the same height -
         * we simply use min(max_rows, windows_num) as the number of rows :*/
        int rows_num = min(max_rows, WinListState.windows_num);
        WinListState.columns_num = (WinListState.windows_num+rows_num-1)/rows_num ;
        WinListState.rows_num = rows_num ;
    }
    LOCAL_DEBUG_OUT("max_rows = %d, rows = %d", max_rows, WinListState.rows_num );

    /* Pass 3: we assign rows/columns to bars, and calculate column width */
    row = 0 ;
    col = 0 ;
    /* just in case initializing entire col_width array : */
    for( i = 0 ; i < WinListState.windows_num ; ++i )
        WinListState.col_width[i] = 0 ;

    for( i = 0 ; i < WinListState.windows_num ; ++i )
    {
        WinListState.bar_col[i] = col ;
        WinListState.bar_row[i] = row ;
        if( WinListState.bar_width[i] > WinListState.col_width[col] )
            WinListState.col_width[col] = min(WinListState.bar_width[i],max_col_width);
        if( get_flags( Config->flags, ASWL_RowsFirst ) )
        {
            if( ++col >= WinListState.columns_num )
            {
                ++row ;
                col = 0 ;
            }
        }else
        {
            if( ++row >= WinListState.rows_num )
            {
                ++col;
                row = 0 ;
            }
        }
    }

    /* Pass 4: we have to calculate overall used window size : */
    for( i = 0 ; i < WinListState.columns_num ; ++i )
        total_width += WinListState.col_width[i] ;
    total_height = WinListState.rows_num * max_item_height ;

    /* Pass 5: redistribute everything that remains to fit the min_size : */
    if( total_width > allowed_max_width )
    {
		int total_delta = (total_width - allowed_max_width) ;
		int applied_delta = 0 ;
        /* in fact that will always be column 0 in the single column layout : */
	    for( i = 0 ; i < WinListState.columns_num-1 ; ++i )
		{
			int delta = (total_delta*WinListState.col_width[i])/total_width ;
			if( delta == 0 )
				delta = 1 ;
			if( total_delta - (applied_delta+delta) < 0 )
				break;
   	    	WinListState.col_width[i] -= delta ;
			applied_delta += delta ;
		}
        WinListState.col_width[WinListState.columns_num-1] -= (total_delta-applied_delta) ;
		total_width = allowed_max_width ;
    }else if( total_width < allowed_min_width )
    {
        int dw = (allowed_min_width - total_width) / WinListState.columns_num ;
        if( dw > 0 )
        {
            for( i = 0 ; i < WinListState.columns_num ; ++i )
            {
                WinListState.col_width[i] += dw ;
                total_width += dw ;
            }
        }
        if( total_width < allowed_min_width )
		{
            WinListState.col_width[WinListState.columns_num-1] += allowed_min_width-total_width;
			total_width = allowed_min_width ;
		}
    }

    if( total_height > allowed_max_height )
    {
        max_item_height = (allowed_max_height+WinListState.rows_num-1)/WinListState.rows_num ;
        total_height = allowed_max_height ;
    }else if( total_height < allowed_min_height )
    { /* same as above, only we do not want to exceed our limits : */
        max_item_height = (allowed_min_height)/WinListState.rows_num ;
        total_height = max_item_height*WinListState.rows_num ;
    }
    WinListState.max_item_height = max_item_height ;


    /* Pass 5: resize main canvas if we were allowed to do so : */
    if( !dont_resize_main_canvas )
    {
        int height = max(total_height,allowed_min_height);
        Bool invalid = ( height != WinListState.main_canvas->height || total_width != WinListState.main_canvas->width );
        resize_canvas( WinListState.main_canvas, total_width, height );
        ASSync( False );
        if( invalid )
            return True;
    }

    /* Pass 6: calculate position of each column (for entire set - just in case)*/
    WinListState.col_x[0] = 0 ;
    for( i = 1 ; i < WinListState.windows_num ; ++i )
        WinListState.col_x[i] = WinListState.col_x[i-1]+WinListState.col_width[i-1] ;

    /* Pass 7: moveresize all the bars to calculated size/position */
    col = 0;
    row = 0;
    y = 0 ;
    for( i = 0 ; i < WinListState.windows_num ; ++i )
    {
        ASTBarData   *tbar = WinListState.window_order[i]->bar ;
        int height = max_item_height ;
        int width = WinListState.col_width[col] ;
        if( tbar == NULL )
            continue;

        if( row == WinListState.rows_num-1 && total_height < allowed_min_height )
            height += allowed_min_height-total_height ;

        /* to fill the gap where there are no bars : */
        if( get_flags( Config->flags, ASWL_RowsFirst ) )
        {
            if( i == WinListState.windows_num - 1 )  /* last row */
                width = total_width - WinListState.col_x[col] ;
        }else  if( col == WinListState.columns_num - 2 )  /* one before last column */
            if( WinListState.windows_num < i + WinListState.rows_num + 1 )
                width += WinListState.col_width[col+1] ;

        set_astbar_size( tbar, width, height );
        move_astbar( tbar, WinListState.main_canvas, WinListState.col_x[col], y );
        if( get_flags( Config->flags, ASWL_RowsFirst ) )
        {
            if( ++col >= WinListState.columns_num )
            {
                ++row ;
                col = 0 ;
                y += height ;
            }
        }else
        {
            if( ++row >= WinListState.rows_num )
            {
                ++col;
                row = 0 ;
                y = 0 ;
            }else
                y += height ;
        }
    }

    for( i = 0 ; i < WinListState.windows_num ; ++i )
    {
        ASTBarData   *tbar = WinListState.window_order[i]->bar ;
        if( tbar != NULL )
            if( DoesBarNeedsRendering(tbar) || dont_resize_main_canvas )
                render_astbar( tbar, WinListState.main_canvas );
    }
    if( is_canvas_dirty( WinListState.main_canvas ) )
	{
		LOCAL_DEBUG_OUT( "update main canvas%s","");
        update_canvas_display( WinListState.main_canvas );
	}
    return True ;
}

unsigned int
find_window_index( ASWindowData *wd )
{
    int i = 0;
    while( i < WinListState.windows_num )
        if( WinListState.window_order[i] == wd )
            break;
        else
            ++i ;
    return i;
}

unsigned int
find_button_by_position( int x, int y )
{
/*    int col = WinListState.columns_num ; */
    int i  = WinListState.windows_num;
/*
    while( --col >= 0 )
        if( WinListState.col_x[col] > x )
        {
            ++col ;
            break;
        }
*/
    while( --i >= 0 )
/*        if( WinListState.bar_col[i] == col ) */
        {
            register ASTBarData *bar = WinListState.window_order[i]->bar ;
            if( bar )
                if( bar->win_x <= x && bar->win_x+bar->width > x &&
                    bar->win_y <= y && bar->win_y+bar->height > y )
                    return i;
        }

    return WinListState.windows_num;
}

/* Public stuff : ***************************************************/
void 
configure_tbar_icon( ASTBarData *tbar, ASWindowData *wd )
{
	ASRawHints    raw;
	ASHints       clean;
	ASSupportedHints *list = create_hints_list ();
	ASImage *icon_im = NULL ;
		
	enable_hints_support (list, HINTS_ICCCM);
	enable_hints_support (list, HINTS_KDE);
	enable_hints_support (list, HINTS_ExtendedWM);
	enable_hints_support (list, HINTS_ASDatabase);
		
	memset( &raw, 0x00, sizeof(ASRawHints));
	memset( &clean, 0x00, sizeof(ASHints));
		
	if( collect_hints (ASDefaultScr, wd->client, HINT_NAME|HINT_GENERAL, &raw) )
	{
		if( merge_hints (&raw, Database, NULL, list, HINT_NAME|HINT_GENERAL, &clean, wd->client) )
		{
			icon_im = get_client_icon_image( ASDefaultScr, &clean );
			destroy_hints( &clean, True );
		}
		destroy_raw_hints ( &raw, True);
	}
	destroy_hints_list( &list );		

	if( icon_im ) 
	{
		int pos[10][2] = {{0,1},
						  {0,2},{1,2},{2,2},
						  {0,1},{1,1},{2,1},
						  {0,0},{1,0},{2,0}};
		int width = icon_im->width ; 
	    int height = icon_im->height ; 

		if( get_flags( Config->flags, WINLIST_ScaleIconToTextHeight ) )
		{
			 height = width = calculate_astbar_height( tbar );
		}	 
		if( get_flags(Config->set_flags, WINLIST_IconSize) )
		{
			if( Config->IconSize.width > 0 ) 
				width = Config->IconSize.width;
			if( Config->IconSize.height > 0 ) 
				height = Config->IconSize.height;
		}
		if( width != icon_im->width || height != icon_im->height ) 
		{
			ASImage *scaled_im = scale_asimage( Scr.asv, icon_im, width, height, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
			if( scaled_im != NULL ) 
			{
			   	safe_asimage_destroy( icon_im );
				icon_im = scaled_im ;
			}					   
		}	 
		add_astbar_icon( tbar, 
						 pos[Config->IconLocation%10][0], 
						 pos[Config->IconLocation%10][1], 
						 0, Config->IconAlign, icon_im );
   	   	safe_asimage_destroy( icon_im );
	}	 
}

void
do_blink_urgent_bar( void *vdata )
{
	ASTBarData *tbar = (ASTBarData *)vdata ;
	set_astbar_focused( tbar, WinListState.main_canvas, !IsASTBarFocused(tbar) );
	timer_new (500, do_blink_urgent_bar, tbar);	
}

static void 
focus_winlist_button( ASWindowData *wd )
{
	timer_remove_by_data( wd->bar );  /* just in case */ 

	if( wd->focused ) 
	{
		set_astbar_style_ptr( wd->bar, BAR_STATE_FOCUSED, Scr.Look.MSWindow[BACK_FOCUSED] );
		set_astbar_focused( wd->bar, NULL, True );
        if( WinListState.focused && WinListState.focused != wd )
		{
			WinListState.focused->focused = False ; 
			focus_winlist_button( WinListState.focused );
		}
		WinListState.focused = wd ;
	}else
	{
		if( get_flags(wd->state_flags, AS_Urgent) && Scr.Look.MSWindow[BACK_URGENT] != NULL )
		{
	    	set_astbar_style_ptr( wd->bar, BAR_STATE_FOCUSED, Scr.Look.MSWindow[BACK_URGENT] );
			set_astbar_focused( wd->bar, NULL, True );
			timer_new (500, do_blink_urgent_bar, wd->bar);	
		}else
			set_astbar_focused( wd->bar, NULL, False );
	}
	if( DoesBarNeedsRendering(wd->bar) )
		render_astbar( wd->bar, WinListState.main_canvas );		
}

static void
configure_tbar_props( ASTBarData *tbar, ASWindowData *wd, Bool focus_only )
{
	INT32 encoding = AS_Text_ASCII ;
	char *name = get_window_name(wd, Config->UseName, &encoding );

	if( !focus_only ) 
	{
	    ASFlagType align = ALIGN_TOP|ALIGN_BOTTOM ;
		int h_spacing = Config->HSpacing ;
		int v_spacing = Config->VSpacing ;
		int fbevel = Config->FBevel ;
		int sbevel = Config->SBevel ;
		int ubevel = Config->UBevel ;
		int fake_label_id = -1 ;
				   
    	delete_astbar_tile( tbar, -1 );
		LOCAL_DEBUG_OUT( "setting bar border to %+d, %+d", tbar->h_border, tbar->v_border );

    	set_astbar_style_ptr( tbar, BAR_STATE_FOCUSED, Scr.Look.MSWindow[BACK_FOCUSED] );
		if( WinListState.tbar_props )
		{
			if( !get_flags( Config->set_flags, WINLIST_Align ) )
				align = WinListState.tbar_props->align ;
			else
				align = Config->Align ;
		    if( !get_flags( Config->set_flags, WINLIST_HSpacing) )
				h_spacing = WinListState.tbar_props->title_h_spacing ;
    		if( !get_flags( Config->set_flags, WINLIST_VSpacing) )
    			v_spacing = WinListState.tbar_props->title_v_spacing ;
			if( !get_flags( Config->set_flags, WINLIST_FBevel ) )
			{	
				fbevel = WinListState.tbar_props->bevel ;
				if( fbevel == 0  ) 
					fbevel = DEFAULT_TBAR_HILITE ;
			}
			if( !get_flags( Config->set_flags, WINLIST_SBevel ) )
			{	
				sbevel = WinListState.tbar_props->bevel ;
				if( sbevel == 0  ) 
					sbevel = DEFAULT_TBAR_HILITE ;
			}

			if( !get_flags( Config->set_flags, WINLIST_UBevel ) )
			{	
				ubevel = WinListState.tbar_props->bevel ;
				if( ubevel == 0  ) 
					ubevel = DEFAULT_TBAR_HILITE ;
			}

		}	 

    	set_astbar_hilite( tbar, BACK_FOCUSED, fbevel );
    	set_astbar_composition_method( tbar, BACK_FOCUSED, Config->FCompositionMethod );
    	if( get_flags(wd->state_flags, AS_Sticky) )
    	{
        	set_astbar_style_ptr( tbar, BAR_STATE_UNFOCUSED, Scr.Look.MSWindow[BACK_STICKY] );
        	set_astbar_hilite( tbar, BACK_UNFOCUSED, sbevel );
        	set_astbar_composition_method( tbar, BACK_UNFOCUSED, Config->SCompositionMethod );
    	}else
    	{
        	set_astbar_style_ptr( tbar, BAR_STATE_UNFOCUSED, Scr.Look.MSWindow[BACK_UNFOCUSED] );
        	set_astbar_hilite( tbar, BACK_UNFOCUSED, ubevel );
        	set_astbar_composition_method( tbar, BACK_UNFOCUSED, Config->UCompositionMethod );
    	}
		
		if( get_flags( Config->flags, ASWL_ShowIcon ) && (Config->IconLocation%10 == 5 ))
		{	
        	fake_label_id = add_astbar_label( tbar, 3, 3, 0, align, h_spacing, v_spacing, "Aq#,`", 0);
			configure_tbar_icon( tbar, wd );
		}
    	if( get_flags( wd->state_flags, AS_Iconic ) && name != NULL && name[0] != '\0')
    	{
        	char *iconic_name = safemalloc(1+strlen(name)+1+1);
        	sprintf(iconic_name, "(%s)", name );
        	add_astbar_label( tbar, 1, 1, 0, align, h_spacing, v_spacing, iconic_name, encoding);
        	free( iconic_name );
    	}else
        	add_astbar_label( tbar, 1, 1, 0, align, h_spacing, v_spacing, name, encoding);
    	
		if( fake_label_id > -1 ) 
			delete_astbar_tile( tbar, fake_label_id );
		else if( get_flags( Config->flags, ASWL_ShowIcon ) )
			configure_tbar_icon( tbar, wd );

		set_astbar_balloon( tbar, 0, name, encoding );
	}
	focus_winlist_button( wd );
}

void
add_winlist_button( ASTBarData *tbar, ASWindowData *wd )
{
    tbar = create_astbar();
LOCAL_DEBUG_OUT("tbar = %p, wd = %p", tbar, wd );
    if( tbar )
	{
        wd->bar = tbar ;
        WinListState.window_order[WinListState.windows_num] = wd ;
        ++(WinListState.windows_num);
        configure_tbar_props( tbar, wd, False );
		if( !WinListState.postpone_display )
		    rearrange_winlist_window( False );
	}
}

void
refresh_winlist_button( ASTBarData *tbar, ASWindowData *wd, Bool focus_only )
{
LOCAL_DEBUG_OUT("tbar = %p, wd = %p", tbar, wd );
    if( tbar )
	{
        int i = find_window_index( wd ) ;
        if( i < WinListState.windows_num )
        {
            configure_tbar_props( tbar, wd, focus_only );
			if( !WinListState.postpone_display )
			{
				Bool rearranged = False ;
				if( !focus_only )
				{	
          			if( calculate_astbar_width( tbar ) > WinListState.col_width[WinListState.bar_col[i]] ||
              			calculate_astbar_height( tbar ) > WinListState.max_item_height )
					{
	                	rearrange_winlist_window( False );
						rearranged = True ;
					}
  		        }
				if( !rearranged )
      		    	render_winlist_button( tbar );
			}
        }
	}
}

void
delete_winlist_button( ASTBarData *tbar, ASWindowData *wd )
{
    int i = find_window_index( wd ) ;

LOCAL_DEBUG_OUT("tbar = %p, wd = %p", tbar, wd );

    if( WinListState.focused == wd )
		WinListState.focused = NULL ;

	if( tbar )
		timer_remove_by_data( tbar );  /* just in case */ 

    if( i < WinListState.windows_num  )
    {
        while( ++i < WinListState.windows_num )
            WinListState.window_order[i-1] = WinListState.window_order[i] ;
        WinListState.window_order[i-1] = NULL ;
        --(WinListState.windows_num);
		if( !WinListState.postpone_display )
      		rearrange_winlist_window(False);
    }
}

/*************************************************************************/
/* pressing and depressing actions :                                     */
/*************************************************************************/
void
press_winlist_button( ASWindowData *wd )
{
    if( wd != NULL && wd->bar != NULL )
    {
        if( wd->bar != WinListState.pressed_bar )
        {
            set_astbar_pressed( WinListState.pressed_bar, WinListState.main_canvas, False );
            WinListState.pressed_bar = wd->bar ;
            set_astbar_pressed( WinListState.pressed_bar, WinListState.main_canvas, True );
            if( is_canvas_dirty( WinListState.main_canvas ) )
			{
				LOCAL_DEBUG_OUT( "update main canvas%s","");	  
                update_canvas_display( WinListState.main_canvas );
			}
        }
    }
}

void
release_winlist_button( ASWindowData *wd, int button )
{
    if( wd != NULL && wd->bar != NULL )
    {
        char **action_list ;

        if( wd->bar != WinListState.pressed_bar )
            set_astbar_pressed( WinListState.pressed_bar, WinListState.main_canvas, False );
        set_astbar_pressed( WinListState.pressed_bar, WinListState.main_canvas, False );
        WinListState.pressed_bar = NULL ;

        if( is_canvas_dirty( WinListState.main_canvas ) )
		{
			LOCAL_DEBUG_OUT( "update main canvas%s","");	  
            update_canvas_display( WinListState.main_canvas );
		}

        action_list = Config->Action[button - Button1] ;
        if( action_list )
        {
            int i = -1;
            while( action_list[++i] )
            {
                LOCAL_DEBUG_OUT( "%d: \"%s\"", i, action_list[i] );
                SendInfo ( action_list[i], wd->client);
            }
        }
    }
}


void
update_winlist_styles()
{
    int i  = WinListState.windows_num;

    while( --i >= 0 )
    {
        register ASTBarData *bar = WinListState.window_order[i]->bar ;
        if( bar )
			configure_tbar_props( bar, WinListState.window_order[i], False );
    }
	if( WinListState.idle_bar )
	{
	    set_astbar_style_ptr( WinListState.idle_bar, BAR_STATE_UNFOCUSED, Scr.Look.MSWindow[BACK_UNFOCUSED] );
        set_astbar_style_ptr( WinListState.idle_bar, BAR_STATE_FOCUSED, Scr.Look.MSWindow[BACK_FOCUSED] );
	}
}

