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

#include "../../configure.h"


#define LOCAL_DEBUG
#include "../../include/asapp.h"
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#define IN_MODULE
#define MODULE_X_INTERFACE

#include "../../include/afterstep.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/mystyle.h"
#include "../../include/mystyle_property.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"
#include "../../include/clientprops.h"
#include "../../include/wmprops.h"
#include "../../include/decor.h"
#include "../../libAfterImage/afterimage.h"
#include "../../include/aswindata.h"
#include "../../include/balloon.h"
#include "../../include/event.h"

/**********************************************************************/
/*  AfterStep specific global variables :                             */
/**********************************************************************/
/**********************************************************************/
/*  WinList local variables :                                         */
/**********************************************************************/
typedef struct {
	unsigned int  width, height ;
	ASTBarData *widest, *tallest ;
	unsigned int max_width, max_height ;
	ASWindowData *focused;

    Window       main_window;
    ASCanvas    *main_canvas;

}ASWinListState ;

ASWinListState WinListState = { 100, 100, NULL, NULL, 0, 0, NULL, None, NULL };

/**********************************************************************/
/**********************************************************************/
/* Our configuration options :                                        */
/**********************************************************************/
BaseConfig *Base = NULL;

/*char *default_unfocused_style = "unfocused_window_style";
 char *default_focused_style = "focused_window_style";
 char *default_sticky_style = "sticky_window_style";
 */

WinListConfig *Config = NULL ;


/**********************************************************************/

void CheckConfigSanity();
void GetBaseOptions (const char *filename);
void GetOptions (const char *filename);
void HandleEvents();
void process_message (unsigned long type, unsigned long *body);
void DispatchEvent (ASEvent * Event);
Window make_winlist_window();
void destroy_winlist_button( void *data );
void add_winlist_button( ASTBarData *tbar, ASWindowData *wd );
void refresh_winlist_button( ASTBarData *tbar, ASWindowData *wd );
void delete_winlist_button( ASTBarData *tbar, ASWindowData *wd );
static void rearrange_winlist_buttons();
static Bool rearrange_winlist_window();


int
main( int argc, char **argv )
{
    Window w ;

    /* Save our program name - for error messages */
    InitMyApp (CLASS_WINLIST, argc, argv, NULL, NULL, 0 );

    set_signal_handler( SIGSEGV );
	set_output_threshold( OUTPUT_LEVEL_PROGRESS );

    ConnectX( &Scr, PropertyChangeMask );
    ConnectAfterStep (M_FOCUS_CHANGE |
                    M_DESTROY_WINDOW |
                    WINDOW_CONFIG_MASK |
                    WINDOW_NAME_MASK |
                    M_END_WINDOWLIST);
    balloon_init (False);
    Config = CreateWinListConfig (PagerState.desks_num);

    /* Request a list of all windows, while we load our config */
    SendInfo ("Send_WindowList", 0);

    LoadBaseConfig ( GetBaseOptions);
    LoadConfig ("winlist", GetOptions);
    CheckConfigSanity();

	WinList = create_asbidirlist(destroy_winlist_button);
	WinListWindow = w = make_winlist_window();
	WinListCanvas = create_ascanvas( WinListWindow );

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
    window_data_cleanup();
    FreeMyAppResources();

    if( WinList )
        destroy_asbidirlist( &WinList );
    if( WinListCanvas )
		destroy_ascanvas( &WinListCanvas );
	if( Base )
        DestroyBaseConfig(Base);

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
    char *default_winlist_style = safemalloc( 1+strlen(MyName)+1);
	default_winlist_style[0] = '*' ;
	strcpy( &(default_winlist_style[1]), MyName );

    if( Config == NULL )
        Config = CreateWinListConfig ();

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

    if( Config->focused_style == NULL )
        Config->focused_style = mystrdup( default_winlist_style );
    if( Config->unfocused_style == NULL )
        Config->unfocused_style = mystrdup( default_winlist_style );
    if( Config->sticky_style == NULL )
        Config->sticky_style = mystrdup( default_winlist_style );
    free( default_winlist_style );

    mystyle_get_property (Scr.wmprops);

    Scr.Look.MSWindow[BACK_UNFOCUSED] = mystyle_find( Config->unfocused_style );
    Scr.Look.MSWindow[BACK_FOCUSED] = mystyle_find( Config->focused_style );
    Scr.Look.MSWindow[BACK_STICKY] = mystyle_find( Config->sticky_style );

    for( i = 0 ; i < BACK_STYLES ; ++i )
    {
        static char *default_window_style_name[BACK_STYLES] ={"focused_window_style","sticky_window_style","unfocused_window_style", NULL};
        if( Scr.Look.MSWindow[i] == NULL )
            Scr.Look.MSWindow[i] = mystyle_find_or_default( default_window_style_name[i] );
    }

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
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
    BaseConfig *config = ParseBaseOptions (filename, MyName);

    if (!config)
	    exit (0);			/* something terrible has happend */

    if( Base!= NULL )
		DestroyBaseConfig (Base);
	Base = config ;

    if( Scr.image_manager )
        destroy_image_manager( Scr.image_manager, False );
    if( Scr.font_manager )
        destroy_font_manager( Scr.font_manager, False );

    Scr.image_manager = create_image_manager( NULL, 2.2, Base->pixmap_path, Base->icon_path, NULL );
    Scr.font_manager = create_font_manager (dpy, Base->font_path, NULL);
}

void
GetOptions (const char *filename)
{
    int i ;
    START_TIME(option_time);
    WinListConfig *config = ParseWinListOptions( filename, MyName );

#ifdef LOCAL_DEBUG
	PrintWinListConfig (config);
#endif

    /* Need to merge new config with what we have already :*/
    /* now lets check the config sanity : */
    /* mixing set and default flags : */
    Config->flags = (config->flags&config->set_flags)|(Config->flags & (~config->set_flags));
    Config->set_flags |= config->set_flags;

    Config->gravity = NorthWestGravity ;
    if( get_flags(config->set_flags, WINLIST_MinSize) )
        merge_geometry(&(config->geometry), &(Config->geometry) );

    if( get_flags(config->set_flags, WINLIST_MinSize) )
    {
        Config->min_width = config->min_width;
        Config->min_height = config->min_height;
    }

    if( get_flags(config->set_flags, WINLIST_MaxSize) )
    {
        Config->max_width = config->max_width;
        Config->max_height = config->max_height;
    }
    if( get_flags(config->set_flags, WINLIST_MaxRows) )
        Config->max_rows = config->max_rows;
    if( get_flags(config->set_flags, WINLIST_MaxColumns) )
        Config->max_columns = config->max_columns;
    if( get_flags(config->set_flags, WINLIST_MaxColWidth) )
        Config->max_col_width = config->max_col_width;
    if( get_flags(config->set_flags, WINLIST_MinColWidth) )
        Config->min_col_width = config->min_col_width;

    if( config->unfocused_style )
        set_string_value( &(Config->unfocused_style), config->unfocused_style, NULL, 0 );
    if( config->focused_style )
        set_string_value( &(Config->focused_style), config->focused_style, NULL, 0 );
    if( config->sticky_style )
        set_string_value( &(Config->sticky_style), config->sticky_style, NULL, 0 );

    if( get_flags(config->set_flags, WINLIST_UseName) )
        Config->show_name_type = config->show_name_type;
    if( get_flags(config->set_flags, WINLIST_Justify) )
        Config->name_aligment = config->name_aligment;
    if( get_flags(config->set_flags, WINLIST_Bevel) )
        Config->bevel = config->bevel;

    for( i = 0 ; i < MAX_MOUSE_BUTTONS ; ++i )
        if( config->mouse_actions[i] )
            set_string_value(&(Config->mouse_actions[i]), config->mouse_actions[i], NULL, 0 );

    if( Config->balloon_conf )
        Destroy_balloonConfig( Config->balloon_conf );
    Config->balloon_conf = config->balloon_conf ;
    config->balloon_conf = NULL ;

    if (config->style_defs)
        ProcessMyStyleDefinitions (&(config->style_defs));
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
        struct ASWindowData *saved_wd = wd ;
        ASTBarData *tbar = wd?wd->bar:NULL;
        WindowPacketResult res ;

		show_progress( "message %X window %X data %p", type, body[0], wd );
		res = handle_window_packet( type, body, &wd );
		if( res == WP_DataCreated )
			add_winlist_button( tbar, wd );
		else if( res == WP_DataChanged )
			refresh_winlist_button( tbar, wd );
		else if( res == WP_DataDeleted )
            delete_winlist_button( tbar, saved_wd );
	}
}

void
DispatchEvent (ASEvent * event)
{
    switch (event->x.type)
    {
	    case ConfigureNotify:
			if( handle_canvas_config( WinListCanvas ) )
			{
				ASBiDirElem *elem ;
				rearrange_winlist_buttons();
				for( elem = WinList->head ; elem ; elem = elem->next )
					update_astbar_transparency( elem->data, WinListCanvas );
			}
	        break;
	    case Expose:
	        break;
	    case ButtonPress:
	    case ButtonRelease:
			break;
        case EnterNotify :
        case LeaveNotify :
        case MotionNotify :
            {
                ASBiDirElem *elem ;
                for( elem = WinList->head ; elem ; elem = elem->next )
                {
                    ASTBarData *bar = elem->data ;
                    if( check_astbar_point( bar, event->x.xmotion.x_root, event->x.xmotion.y_root ) != C_NO_CONTEXT )
                    {
                        on_astbar_pointer_action( bar, 0, (event->x.type == LeaveNotify) );
                        break;
                    }
                }
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
	unsigned int width = (WinListState.width <= 0)?1:WinListState.width;
	unsigned int height = (WinListState.height <= 0)?1:WinListState.height;

	switch( Config->gravity )
	{
		case NorthEastGravity :
			x = Scr.MyDisplayWidth - width + Config->anchor_x ;
			y = Config->anchor_y ;
			break;
		case SouthEastGravity :
			x = Scr.MyDisplayWidth - width + Config->anchor_x ;
			y = Scr.MyDisplayHeight - height + Config->anchor_y ;
			break;
		case SouthWestGravity :
			x = Config->anchor_x ;
			y = Scr.MyDisplayHeight - height + Config->anchor_y ;
			break;
		case NorthWestGravity :
		default :
			x = Config->anchor_x ;
			y = Config->anchor_y ;
			break;
	}

	w = create_visual_window( Scr.asv, Scr.Root, x, y, width, height, 0, InputOutput, 0, NULL);
    set_client_names( w, MyName, MyName, CLASS_WINLIST, MyName );

	shints.flags = USPosition|USSize|PMinSize|PMaxSize|PBaseSize|PWinGravity;
	shints.min_width = shints.min_height = 4;
	shints.max_width = (Config->max_width>0)?Config->max_width:Scr.MyDisplayWidth;
	shints.max_height = (Config->max_height>0)?Config->max_height:Scr.MyDisplayHeight;
	shints.base_width = shints.base_height = 4;
	shints.win_gravity = Config->gravity ;

	extwm_hints.pid = getpid();
	extwm_hints.flags = EXTWM_PID|EXTWM_StateSkipTaskbar|EXTWM_StateSkipPager|EXTWM_TypeMenu ;

	set_client_hints( w, NULL, &shints, AS_DoesWmDeleteWindow, &extwm_hints );

	/* showing window to let user see that we are doing something */
	XMapRaised (dpy, w);
	rearrange_winlist_window();
	/* final cleanup */
	XFlush (dpy);
	sleep (1);								   /* we have to give AS a chance to spot us */
	/* we will need to wait for PropertyNotify event indicating transition
	   into Withdrawn state, so selecting event mask: */
	XSelectInput (dpy, w, PropertyChangeMask|StructureNotifyMask);

	return w ;
}
/********************************************************************/
/* WinList buttons handling :                                       */
/********************************************************************/
/* Private stuff : **************************************************/
static char *
get_visible_window_name( ASWindowData *wd )
{
	char *vname = NULL ;
	switch( Config->show_name_type )
	{
		case ASN_Name :     vname = wd->window_name ; break ;
		case ASN_IconName : vname = wd->icon_name ; break ;
		case ASN_ResClass : vname = wd->res_class ; break ;
		case ASN_ResName :  vname = wd->res_name ; break ;
	 default :
	}
	if( vname == NULL )
	{
		if( wd->window_name )
			return wd->window_name ;
		if( wd->icon_name )
			return wd->icon_name ;
		if( wd->res_class )
			return wd->res_class ;
		if( wd->res_name )
			return wd->res_name ;
	}
	return vname ;
}

static Bool
render_winlist_button( ASTBarData *tbar )
{
	LOCAL_DEBUG_CALLER_OUT("tbar %p", tbar );
	if( render_astbar( tbar, WinListCanvas ) )
	{
		update_canvas_display( WinListCanvas );
		return True ;
	}
	return False ;
}

/* WE do redrawing in two steps :
  1) we need to determine the desired window size and resize it accordingly
  2) when we get StructureNotify event - we need to reposition and redraw
     everything accordingly
 */

static Bool
rearrange_winlist_window()
{
	ASBiDirElem *elem ;
	int rows = 1, cols = 1 ;
	unsigned int max_height = 1, max_width = 1 ;
	unsigned int allowed_max_width = (Config->max_width==0)?Scr.MyDisplayWidth:Config->max_width ;
	unsigned int allowed_max_height = (Config->max_height==0)?Scr.MyDisplayHeight:Config->max_height ;
	unsigned int max_col_width = (Config->max_col_width==0)?Scr.MyDisplayWidth:Config->max_col_width ;
	unsigned int count = 0 ;

	if( allowed_max_width > Scr.MyDisplayWidth )
		allowed_max_width = Scr.MyDisplayWidth ;
	if( allowed_max_height > Scr.MyDisplayHeight )
		allowed_max_height = Scr.MyDisplayHeight ;
	for( elem = WinList->head ; elem ; elem = elem->next )
	{
		ASTBarData *tbar = elem->data ;
		if( tbar )
		{
			unsigned int width, height ;

			++count ;

			width = calculate_astbar_width( tbar );
			height = calculate_astbar_height( tbar );
			if( width > max_col_width )
				width = max_col_width ;
			if( width > allowed_max_width )
				width = allowed_max_width ;
			if( height > allowed_max_height )
				height = allowed_max_height ;
			if( height  > max_height )
			{
				max_height = height ;
				WinListState.tallest = tbar ;
			}
			if( width  > max_width )
			{
				max_width = width ;
				WinListState.widest = tbar ;
			}
		}
	}

	if( get_flags(Config->flags, ASWL_RowsFirst) )
	{
		cols = (allowed_max_width+1)/max_width ;
		if( cols > count )
		{
			cols = count;
			rows = 1 ;
		}else
		{
			rows = count / cols ;
			if( rows * cols < count )
				++rows ;
		}
	}else
	{
		rows = (allowed_max_height+1)/max_height ;
		if( rows > count )
		{
			rows = count ;
			cols = 1 ;
		}else
		{
			cols = count / rows ;
			if( cols * rows < count )
				++cols ;
		}
	}

	if( WinListState.width  == cols * max_width &&
	    WinListState.height == rows * max_height &&
		WinListCanvas->width == WinListState.width &&
		WinListCanvas->height == WinListState.height )
		return False ;
	WinListState.width  = cols * max_width ;
	WinListState.height = rows * max_height;
	if( WinListState.width <= 0 )
		WinListState.width = 1 ;
	if( WinListState.height <= 0 )
		WinListState.height = 1 ;

	LOCAL_DEBUG_OUT("Resizing Winlist window to (%dx%d) %dx%d max size is (%dx%d)", cols, rows, WinListState.width, WinListState.height, max_width, max_height );
	if( WinListCanvas )
		resize_canvas( WinListCanvas, WinListState.width, WinListState.height );

	WinListState.max_width = max_width ;
	WinListState.max_height = max_height ;

	return True ;
}

static void
rearrange_winlist_buttons()
{
	ASBiDirElem *elem ;
	int curr_row = 1, curr_col = 1 ;
	unsigned int next_x = 0 ;
	unsigned int next_y = 0 ;
	unsigned int max_height = WinListState.max_height, max_width = WinListState.max_width ;
	unsigned int allowed_max_width = (Config->max_width==0)?Scr.MyDisplayWidth:Config->max_width ;
	unsigned int allowed_max_height = (Config->max_height==0)?Scr.MyDisplayHeight:Config->max_height ;

	if( allowed_max_width > Scr.MyDisplayWidth )
		allowed_max_width = Scr.MyDisplayWidth ;
	if( allowed_max_height > Scr.MyDisplayHeight )
		allowed_max_height = Scr.MyDisplayHeight ;
	for( elem = WinList->head ; elem ; elem = elem->next )
	{
		ASTBarData *tbar = elem->data ;
		Bool redraw = False ;
		int x, y ;
		if( tbar )
		{
			redraw = set_astbar_size( tbar, max_width, max_height );
			if( get_flags(Config->flags, ASWL_RowsFirst) )
			{
				if( next_x+max_width > allowed_max_width )
					if( curr_row  < Config->max_rows )
					{
						++curr_row ;
						next_x = 0 ;
						next_y += max_height ;
					}
				x = next_x ;
				y = next_y ;
				next_x += max_width ;
			}else
			{
				if( next_y+max_height > allowed_max_height )
					if( curr_col  < Config->max_columns )
					{
						++curr_col ;
						next_y = 0 ;
						next_x += max_width ;
					}
				x = next_x ;
				y = next_y ;
				next_y += max_height ;
			}
			if( move_astbar( tbar, WinListCanvas, x, y ) )
				redraw = True ;
			render_astbar( tbar, WinListCanvas );
		}
	}
	update_canvas_display( WinListCanvas );
}

/* Public stuff : ***************************************************/
void
destroy_winlist_button( void *data )
{
	if( data )
	{
		destroy_astbar( (ASTBarData**)&data );
	}
}

static void
configure_tbar_props( ASTBarData *tbar, ASWindowData *wd )
{
	char *name = get_visible_window_name(wd);
	set_astbar_style( tbar, BAR_STATE_FOCUSED, Config->focused_style?Config->focused_style:default_winlist_style );

    if( get_flags(wd->state_flags, AS_Sticky) )
		set_astbar_style( tbar, BAR_STATE_UNFOCUSED, Config->sticky_style?Config->sticky_style:default_winlist_style );
	else
		set_astbar_style( tbar, BAR_STATE_UNFOCUSED, Config->unfocused_style?Config->unfocused_style:default_winlist_style );
    add_astbar_label( tbar, 0, 0, 0, ALIGN_LEFT, name);
    set_astbar_balloon( tbar, 0, name );
	if( wd->focused )
	{
		if( WinListState.focused && WinListState.focused != wd )
		{
			WinListState.focused->focused = False ;
			refresh_winlist_button( WinListState.focused->data, WinListState.focused ) ;
		}
		set_flags( tbar->state, BAR_STATE_FOCUSED );
		WinListState.focused = wd ;
	}else
		clear_flags( tbar->state, BAR_STATE_FOCUSED );
}

void
add_winlist_button( ASTBarData *tbar, ASWindowData *wd )
{
	tbar = create_astbar();
	tbar = append_bidirelem( WinList, tbar );
	if( tbar )
	{
		configure_tbar_props( tbar, wd );

//		LOCAL_DEBUG_OUT( "Added tbar %p with name [%s]", tbar, name);
		wd->data = tbar ;
		rearrange_winlist_window();
	}
}

void
refresh_winlist_button( ASTBarData *tbar, ASWindowData *wd )
{
	if( tbar )
	{
		configure_tbar_props( tbar, wd );
		if( tbar == WinListState.widest || tbar == WinListState.tallest )
		{
			if( !rearrange_winlist_window() )
				render_winlist_button( tbar );
		}else
		{
			render_winlist_button( tbar );
		}
	}
}

void
delete_winlist_button( ASTBarData *tbar, ASWindowData *wd )
{
	if( WinListState.focused == wd )
		WinListState.focused = NULL ;
LOCAL_DEBUG_OUT("tbar = %p, wd = %p", tbar, wd );
	if( tbar )
	{
		discard_bidirelem( WinList, tbar );
		rearrange_winlist_window();
	}
}

