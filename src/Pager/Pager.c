/*
 * Copyright (C) 1998 Eric Tremblay <deltax@pragma.net>
 * Copyright (c) 1998 Michal Vitecek <fuf@fuf.sh.cvut.cz>
 * Copyright (c) 1998 Doug Alcorn <alcornd@earthlink.net>
 * Copyright (c) 2002,1998 Sasha Vasko <sasha at aftercode.net>
 * Copyright (c) 1997 ric@giccs.georgetown.edu
 * Copyright (C) 1998 Makoto Kato <m_kato@ga2.so-net.ne.jp>
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (C) 1996 Rainer M. Canavan (canavan@Zeus.cs.bonn.edu)
 * Copyright (C) 1996 Dan Weeks
 * Copyright (C) 1994 Rob Nation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*#define DO_CLOCKING      */
#define LOCAL_DEBUG
#define EVENT_TRACE

#include "../../configure.h"
#include "../../libAfterStep/asapp.h"
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#define IN_MODULE
#define MODULE_X_INTERFACE

#include "../../libAfterStep/afterstep.h"
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/parser.h"
#include "../../libAfterStep/mystyle.h"
#include "../../libAfterStep/mystyle_property.h"
#include "../../libAfterStep/balloon.h"
#include "../../libAfterStep/aswindata.h"
#include "../../libAfterStep/decor.h"
#include "../../libAfterStep/event.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterStep/moveresize.h"
#include "../../libAfterStep/shape.h"

#include "../../libAfterConf/afterconf.h"

/* pager flags  - shared between PagerDEsk and PagerState */
#define ASP_DeskShaded          (0x01<<0)
#define ASP_UseRootBackground   (0x01<<1)
#define ASP_Shaped              (0x01<<2)
#define ASP_ShapeDirty          (0x01<<3)
#define ASP_ReceivingWindowList (0x01<<4)


typedef struct ASPagerDesk {

    ASFlagType flags ;
    INT32 desk ;								/* absolute value - no need to add start_desk */
    ASCanvas   *desk_canvas;
    ASTBarData *title;
    ASTBarData *background;
    Window     *separator_bars;                /* (rows-1)*(columns-1) */
    XRectangle *separator_bar_rects ;
    unsigned int separator_bars_num ;
    unsigned int title_width, title_height;

    ASWindowData **clients ;
    unsigned int   clients_num ;

    ASImage *back ;
}ASPagerDesk;

typedef struct ASPagerState
{
    ASFlagType flags ;

    ASCanvas    *main_canvas;
    ASCanvas    *icon_canvas;

    ASPagerDesk *desks ;
    INT32 start_desk, desks_num ;

    int page_rows,  page_columns ;
	/* x and y size of desktop */
	int vscaled_desk_width, vscaled_desk_height ;  /* calculated in accordance to Scale param in database */
	int desk_width, desk_height ;                  /* adjusted for the size of title */
	/* area of the main window used up by labels, borders and other garbadge :*/
	int wasted_width, wasted_height ;
	/* x and y size of virtual screen size inside desktop mini-window */
	int vscreen_width, vscreen_height ;
    int aspect_x,   aspect_y;

    int wait_as_response ;

    ASCanvas   *pressed_canvas ;
    ASTBarData *pressed_bar;
    int         pressed_context;
    ASPagerDesk *pressed_desk;
    int         pressed_button;

    ASPagerDesk  *focused_desk;
    ASPagerDesk  *resize_desk;                 /* desk on which we are currently resizing the window */

    Window      selection_bars[4];
    XRectangle  selection_bar_rects[4] ;

	ASTBarProps *tbar_props ;

#define C_ShadeButton 		C_TButton0	
	   
	MyButton shade_button ;
}ASPagerState;

ASPagerState PagerState;
#define DEFAULT_BORDER_COLOR 0xFF808080

#define PAGE_MOVE_THRESHOLD     15   /* precent */

#define CLIENT_EVENT_MASK   StructureNotifyMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask|EnterWindowMask|LeaveWindowMask

/* Storing window list as hash table hashed by client window ID :     */
ASHashTable *PagerClients = NULL;

PagerConfig *Config = NULL;
int Rows_override = 0 ;
int Columns_override = 0 ;

CommandLineOpts Pager_cmdl_options[3] =
{
	{NULL, "rows","Overrides module's layout rows", NULL, handler_set_int, &Rows_override, 0, CMO_HasArgs },
	{NULL, "cols","Overrides module's layout cols", NULL, handler_set_int, &Columns_override, 0, CMO_HasArgs },
    {NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};


void
pager_usage (void)
{
	printf (OPTION_USAGE_FORMAT " [--rows rows] [--cols cols] n m\n", MyName );
	print_command_line_opt("standard_options are ", as_standard_cmdl_options, ASS_Restarting);
	print_command_line_opt("additional options are ", Pager_cmdl_options, 0);
	printf("The last two numbers n and m define a range of desktops to be displayed.\n"); 
	exit (0);
}

void HandleEvents();
void process_message (send_data_type type, send_data_type *body);
void DispatchEvent (ASEvent * Event);
void rearrange_pager_window();
Window make_pager_window();
void GetOptions (const char *filename);
void GetBaseOptions (const char *filename);
void CheckConfigSanity();
void redecorate_pager_desks();
void rearrange_pager_desks(Bool dont_resize_main );
void on_pager_window_moveresize( void *client, Window w, int x, int y, unsigned int width, unsigned int height );
void on_pager_pressure_changed( ASEvent *event );
void release_pressure();
void on_desk_moveresize( ASPagerDesk *d );
void on_scroll_viewport( ASEvent *event );
void place_separation_bars( ASPagerDesk *d );
void DeadPipe(int);
void request_background_image( ASPagerDesk *d );


/***********************************************************************
 *   main - start of module
 ***********************************************************************/
int
main (int argc, char **argv)
{
    int i;
    INT32 desk1 = 0, desk2 = 0;
	int desk_cnt = 0 ;

    /* Save our program name - for error messages */
	set_DeadPipe_handler(DeadPipe);
    InitMyApp (CLASS_PAGER, argc, argv, pager_usage, NULL, ASS_Restarting );
	LinkAfterStepConfig();

    memset( &PagerState, 0x00, sizeof(PagerState));
	PagerState.page_rows = PagerState.page_columns = 1 ;

    for( i = 1 ; i< argc ; ++i)
	{
		LOCAL_DEBUG_OUT( "argv[%d] = \"%s\", original argv[%d] = \"%s\"", i, argv[i], i, MyArgs.saved_argv[i]);	  
		if( argv[i] != NULL )
		{ 	
			if( isdigit (argv[i][0]) )
			{
				++desk_cnt ;
				if( desk_cnt == 1 )
					desk1 = atoi (argv[i]);
				else if( desk_cnt == 2 )
					desk2 = atoi (argv[i]);
				else
					break;
	    	}else if( strcmp( argv[i] , "--rows" ) == 0 && i+1 < argc &&  argv[i+1] != NULL )
			{
				Rows_override = atoi( argv[++i] );
	    	}else if( strcmp( argv[i] , "--cols" ) == 0 && i+1 < argc &&  argv[i+1] != NULL )
			{
				Columns_override = atoi( argv[++i] );
			}
		}
	}
	LOCAL_DEBUG_OUT( "desk1 = %ld, desk2 = %ld, desks = %ld, start_desk = %ld", desk1, desk2, PagerState.desks_num, PagerState.start_desk );
    if (desk2 < desk1)
	{
        PagerState.desks_num = (desk1-desk2)+1 ;
        PagerState.start_desk = desk2 ;
    }else
    {
        PagerState.desks_num = (desk2-desk1)+1 ;
        PagerState.start_desk = desk1 ;
    }

    PagerState.desks = safecalloc (PagerState.desks_num, sizeof (ASPagerDesk));
    for( i = 0; i< PagerState.desks_num; ++i )
        PagerState.desks[i].desk = PagerState.start_desk+i ;

    ConnectX( ASDefaultScr, EnterWindowMask );
    if( ConnectAfterStep (  M_ADD_WINDOW |
                        M_CONFIGURE_WINDOW |
						M_STATUS_CHANGE |
                        M_DESTROY_WINDOW |
                        M_FOCUS_CHANGE |
                        M_NEW_DESKVIEWPORT |
                        M_NEW_BACKGROUND |
                        M_WINDOW_NAME |
                        M_ICON_NAME |
                        M_END_WINDOWLIST|
                        M_STACKING_ORDER, 0) < 0 ) 
		exit(1);               /* no AfterStep */

    Config = CreatePagerConfig (PagerState.desks_num);

    LOCAL_DEBUG_OUT("parsing Options for \"%s\"",MyName);
    LoadBaseConfig (GetBaseOptions);
	LoadColorScheme();
    LoadConfig ("pager", GetOptions);

    CheckConfigSanity();

    /* Create a list of all windows */
    /* Request a list of all windows,
     * wait for ConfigureWindow packets */
    SendInfo ("Send_WindowList", 0);
    set_flags( PagerState.flags, ASP_ReceivingWindowList );

    PagerState.main_canvas = create_ascanvas( make_pager_window() );
    redecorate_pager_desks();
    rearrange_pager_desks(False);

    LOCAL_DEBUG_OUT("starting The Loop ...%s","");
    HandleEvents();

    return 0;
}

void HandleEvents()
{
    ASEvent event;
    Bool has_x_events = False ;
    while (True)
    {
    LOCAL_DEBUG_OUT( "wait_as_resp = %d", PagerState.wait_as_response );

        while(PagerState.wait_as_response > 0)
        {
            ASMessage *msg = CheckASMessage (WAIT_AS_RESPONSE_TIMEOUT);
            if (msg)
            {
                process_message (msg->header[1], msg->body);
                DestroyASMessage (msg);
            }
			--PagerState.wait_as_response ;
        }
        while((has_x_events = XPending (dpy)) )
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
	int i ;

	if( already_dead ) 
		return;
	already_dead = True ;

	window_data_cleanup();
	destroy_ashash( &PagerClients );
	for( i = 0 ; i < PagerState.desks_num ; ++i ) 
	{
		ASPagerDesk *d = &PagerState.desks[i] ;	
		destroy_astbar( &(d->title) );
		destroy_astbar( &(d->background) );
		destroy_ascanvas( &(d->desk_canvas) );
		if( d->separator_bars ) 
		{
			int p ;
            for( p = 0 ; p < d->separator_bars_num ; ++p )
                if( d->separator_bars[p] )
                    XDestroyWindow( dpy, d->separator_bars[p] );
            free( d->separator_bars );
            free( d->separator_bar_rects );
		}	 
		if( d->clients ) 
			free( d->clients );
		if( d->back ) 
			safe_asimage_destroy( d->back );
	}	    
    destroy_ascanvas( &PagerState.main_canvas );
    destroy_ascanvas( &PagerState.icon_canvas );
    
	for( i = 0 ; i < 4 ; ++i ) 
	{	
		if( PagerState.selection_bars[i] ) 
			XDestroyWindow( dpy, PagerState.selection_bars[i] );
	}

    if( Config )
        DestroyPagerConfig (Config);
	destroy_astbar_props( &(PagerState.tbar_props) );
    free_button_resources( &PagerState.shade_button );

    if( Config->MSDeskBack  )
        free( Config->MSDeskBack );
	free( PagerState.desks );

    FreeMyAppResources();
#ifdef DEBUG_ALLOCS
	print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

    XFlush (dpy);
    XCloseDisplay (dpy);
    exit (0);
}


void
retrieve_pager_astbar_props()
{
	destroy_astbar_props( &(PagerState.tbar_props) );
	
	PagerState.tbar_props = get_astbar_props(Scr.wmprops );

    free_button_resources( &PagerState.shade_button );
	PagerState.shade_button.context = C_NO_CONTEXT ;
    if( get_flags( Config->set_flags, PAGER_SET_SHADE_BUTTON ) )
    {
		if( Config->shade_button[0] ) 
	        if( load_button( &PagerState.shade_button, Config->shade_button, Scr.image_manager ) )
				PagerState.shade_button.context = C_ShadeButton ;
    }else
		button_from_astbar_props( PagerState.tbar_props, &PagerState.shade_button, C_ShadeButton, _AS_BUTTON_SHADE, _AS_BUTTON_SHADE_PRESSED );
}	 


/*****************************************************************************
 * This routine is responsible for reading and parsing the config file
 ****************************************************************************/
void
CheckConfigSanity()
{
    int i;
    char buf[256];

    if( Config == NULL )
        Config = CreatePagerConfig (PagerState.desks_num);

	if( Rows_override > 0 ) 
		Config->rows = Rows_override ;
	if( Columns_override > 0 ) 
		Config->columns = Columns_override ;
    LOCAL_DEBUG_OUT( "columns = %d, rows = %d, desks = %ld, start_desk = %ld", Config->columns, Config->rows, PagerState.desks_num, PagerState.start_desk );
	if( Config->rows > PagerState.desks_num )
		Config->rows = PagerState.desks_num ;
	if( Config->rows == 0 )
        Config->rows = 1;

    if( Config->columns == 0 ||
        Config->rows*Config->columns != PagerState.desks_num )
        Config->columns = PagerState.desks_num/Config->rows;

    if( Config->rows*Config->columns < PagerState.desks_num )
        ++(Config->columns);
    
	LOCAL_DEBUG_OUT( "columns = %d, rows = %d, desks = %ld, start_desk = %ld", Config->columns, Config->rows, PagerState.desks_num, PagerState.start_desk );

	if( MyArgs.geometry.flags != 0 ) 
		Config->geometry = MyArgs.geometry ;

	LOCAL_DEBUG_OUT( "geometry = %dx%d%+d%+d", Config->geometry.width, Config->geometry.height, Config->geometry.x, Config->geometry.y );

    if( get_flags(Config->geometry.flags, XNegative) )
        Config->gravity = get_flags(Config->geometry.flags, YNegative)? SouthEastGravity:NorthEastGravity;
    else if( get_flags(Config->geometry.flags, YNegative) )
        Config->gravity = SouthWestGravity;

	if(MyArgs.gravity != ForgetGravity)
    	Config->gravity = MyArgs.gravity ;

    if (Config->geometry.width <= Config->columns )
		clear_flags(Config->geometry.flags, WidthValue);
    if (!get_flags(Config->geometry.flags, WidthValue) )
		Config->geometry.width = PagerState.vscaled_desk_width*Config->columns ;

	PagerState.desk_width = Config->geometry.width/Config->columns ;
	Config->geometry.width = PagerState.desk_width*Config->columns ;

    if (Config->geometry.height <= Config->rows )
		clear_flags(Config->geometry.flags, HeightValue);
    if (!get_flags(Config->geometry.flags, HeightValue) || Config->geometry.height <= Config->rows )
		Config->geometry.height = PagerState.vscaled_desk_height*Config->rows ;

    PagerState.desk_height = Config->geometry.height/Config->rows ;
    Config->geometry.height = PagerState.desk_height*Config->rows ;

	PagerState.wasted_width = PagerState.wasted_height = 0 ;

    if( !get_flags(Config->geometry.flags, XValue))
    {
        Config->geometry.x = 0;
    }else
    {
        int real_x = Config->geometry.x ;
        if( get_flags(Config->geometry.flags, XNegative) )
        {
            Config->gravity = NorthEastGravity ;
            real_x += (int)Scr.MyDisplayWidth ;
        }
        if( real_x + (int)Config->geometry.width  < 0 )
            Config->geometry.x = get_flags(Config->geometry.flags, XNegative)?
                                    (int)Config->geometry.width-(int)Scr.MyDisplayWidth : 0 ;
        else if( real_x > (int)Scr.MyDisplayWidth )
            Config->geometry.x = get_flags(Config->geometry.flags, XNegative)?
                                    0 : (int)Scr.MyDisplayWidth-(int)Config->geometry.width ;
    }
    if( !get_flags(Config->geometry.flags, YValue) )
    {
        Config->geometry.y = 0;
    }else
    {
        int real_y = Config->geometry.y ;
        if( get_flags(Config->geometry.flags, YNegative) )
        {
            Config->gravity = (Config->gravity==NorthEastGravity)?SouthEastGravity:SouthWestGravity ;
            real_y += (int)Scr.MyDisplayHeight ;
        }
        if( real_y + (int)Config->geometry.height  < 0 )
            Config->geometry.y = get_flags(Config->geometry.flags, YNegative)?
                                    (int)Config->geometry.height-(int)Scr.MyDisplayHeight : 0 ;
        else if( real_y > (int)Scr.MyDisplayHeight )
            Config->geometry.y = get_flags(Config->geometry.flags, YNegative)?
                                    0 : (int)Scr.MyDisplayHeight-(int)Config->geometry.height ;
    }


    if( get_flags( Config->set_flags, PAGER_SET_ICON_GEOMETRY ) )
    {
        if ( !get_flags(Config->icon_geometry.flags, WidthValue) || Config->icon_geometry.width <= 0 )
            Config->icon_geometry.width = 54;
        if ( !get_flags(Config->icon_geometry.flags, HeightValue)|| Config->icon_geometry.height <= 0)
            Config->icon_geometry.height = 54;
    }else
    {
        Config->icon_geometry.width = 54;
        Config->icon_geometry.height = 54;
    }

	parse_argb_color( Config->border_color, &(Config->border_color_argb) );
	parse_argb_color( Config->selection_color, &(Config->selection_color_argb) );
    parse_argb_color( Config->grid_color, &(Config->grid_color_argb) );


    if( !get_flags( Config->set_flags, PAGER_SET_ACTIVE_BEVEL ) )
        Config->active_desk_bevel = DEFAULT_TBAR_HILITE ;
    if( !get_flags( Config->set_flags, PAGER_SET_INACTIVE_BEVEL ) )
        Config->inactive_desk_bevel = DEFAULT_TBAR_HILITE;

	if ( PagerState.tbar_props == NULL ) 
		retrieve_pager_astbar_props();

    LOCAL_DEBUG_OUT("active_bevel = %lX, inactive_bevel = %lX", Config->active_desk_bevel, Config->inactive_desk_bevel );
	mystyle_get_property (Scr.wmprops);

    for( i = 0 ; i < BACK_STYLES ; ++i )
    {
        static char *window_style_names[BACK_STYLES] ={"*%sFWindowStyle", "*%sUWindowStyle", "*%sSWindowStyle", NULL };
        static char *default_window_style_name[BACK_STYLES] ={"focused_window_style","unfocused_window_style","sticky_window_style", NULL};
		if( window_style_names[i] )
		{
      		sprintf( &(buf[0]), window_style_names[i], MyName );
	        if( (Scr.Look.MSWindow[i] = mystyle_find( buf )) == NULL  && i != BACK_URGENT)
  		        Scr.Look.MSWindow[i] = mystyle_find_or_default( default_window_style_name[i] );
		}
    }

    if( Config->small_font_name )
    {
        MyFont small_font = { NULL, NULL};
        if( load_font (Config->small_font_name, &small_font) )
        {
            mystyle_merge_font( Scr.Look.MSWindow[0], &small_font, True );
            for( i = 1 ; i < BACK_STYLES ; ++i )
                if( Scr.Look.MSWindow[i] )
                    mystyle_merge_font( Scr.Look.MSWindow[i], &small_font, True );
        }
		unload_font (&small_font);
    }

    for( i = 0 ; i < DESK_STYLES ; ++i )
    {
        static char *desk_style_names[DESK_STYLES] ={"*%sActiveDesk", "*%sInActiveDesk" };

        sprintf( buf, desk_style_names[i], MyName );
        Config->MSDeskTitle[i] = mystyle_find_or_default( buf );
LOCAL_DEBUG_OUT( "desk_style %d: \"%s\" ->%p(\"%s\")->colors(%lX,%lX)", i, buf, Config->MSDeskTitle[i], Config->MSDeskTitle[i]->name, Config->MSDeskTitle[i]->colors.fore, Config->MSDeskTitle[i]->colors.back );
    }

    if( Config->MSDeskBack == NULL )
        Config->MSDeskBack = safecalloc( PagerState.desks_num, sizeof(MyStyle*));
    for( i = 0 ; i < PagerState.desks_num ; ++i )
    {
        Config->MSDeskBack[i] = NULL ;
        if( Config->styles && Config->styles[i] != NULL )
            Config->MSDeskBack[i] = mystyle_find( Config->styles[i] );

        if( Config->MSDeskBack[i] == NULL )
        {
            sprintf( buf, "*%sDesk%ld", MyName, i + PagerState.start_desk);
            Config->MSDeskBack[i] = mystyle_find_or_default( buf );
        }
    }
    /* shade button : */
    Scr.Feel.EdgeResistanceMove = 5;
    Scr.Feel.EdgeAttractionScreen = 5;
    Scr.Feel.EdgeAttractionWindow  = 10;
    Scr.Feel.OpaqueMove = 100;
    Scr.Feel.OpaqueResize = 100;
    Scr.Feel.no_snaping_mod = ShiftMask ;
	
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
    Print_balloonConfig ( Config->balloon_conf );
#endif
    balloon_config2look( &(Scr.Look), NULL, Config->balloon_conf, "*PagerBalloon" );
    set_balloon_look( Scr.Look.balloon_look );

	LOCAL_DEBUG_OUT( "geometry = %dx%d%+d%+d", Config->geometry.width, Config->geometry.height, Config->geometry.x, Config->geometry.y );

}

void
GetOptions (const char *filename)
{
    PagerConfig *config = ParsePagerOptions (filename, MyName, PagerState.start_desk, PagerState.start_desk+PagerState.desks_num);
    int i;
    START_TIME(option_time);

/*   WritePagerOptions( filename, MyName, Pager.desk1, Pager.desk2, config, WF_DISCARD_UNKNOWN|WF_DISCARD_COMMENTS );
 */

    /* Need to merge new config with what we have already :*/
    /* now lets check the config sanity : */
    /* mixing set and default flags : */
    Config->flags = (config->flags&config->set_flags)|(Config->flags & (~config->set_flags));
    Config->set_flags |= config->set_flags;

    if( get_flags(config->set_flags, PAGER_SET_ROWS) )
        Config->rows = config->rows;

    if( get_flags(config->set_flags, PAGER_SET_COLUMNS) )
        Config->columns = config->columns;

    Config->gravity = NorthWestGravity ;
    if( get_flags( config->set_flags, PAGER_SET_GEOMETRY ) )
        merge_geometry(&(config->geometry), &(Config->geometry) );

    if( get_flags( config->set_flags, PAGER_SET_ICON_GEOMETRY ) )
        merge_geometry(&(config->icon_geometry), &(Config->icon_geometry) );

    if( config->labels )
    {
        if( Config->labels == NULL )
            Config->labels = safecalloc( PagerState.desks_num, sizeof(char*));
        for( i = 0 ; i < PagerState.desks_num ; ++i )
            if( config->labels[i] )
                set_string( &(Config->labels[i]), mystrdup(config->labels[i]) );
    }
    if( config->styles )
    {
        if( Config->styles == NULL )
            Config->styles = safecalloc( PagerState.desks_num, sizeof(char*));
        for( i = 0 ; i < PagerState.desks_num ; ++i )
            if( config->styles[i] )
                set_string( &(Config->styles[i]), mystrdup(config->styles[i]) );
    }
    if( get_flags( config->set_flags, PAGER_SET_ALIGN ) )
        Config->align = config->align ;

    if( get_flags( config->set_flags, PAGER_SET_SMALL_FONT ) )
        set_string( &(Config->small_font_name), mystrdup(config->small_font_name) );

    if( get_flags( config->set_flags, PAGER_SET_BORDER_WIDTH ) )
        Config->border_width = config->border_width;

    if( get_flags( config->set_flags, PAGER_SET_SELECTION_COLOR ) )
		set_string( &(Config->selection_color), mystrdup(config->selection_color) );

    if( get_flags( config->set_flags, PAGER_SET_GRID_COLOR ) )
		set_string( &(Config->grid_color), mystrdup(config->grid_color) );


    if( get_flags( config->set_flags, PAGER_SET_BORDER_COLOR ) )
		set_string( &(Config->border_color), mystrdup(config->border_color) );

    if( config->shade_button[0] )
        set_string( &(Config->shade_button[0]), mystrdup(config->shade_button[0]) );

    if( config->shade_button[1] )
        set_string( &(Config->shade_button[1]), mystrdup(config->shade_button[1]) );

    if( get_flags( config->set_flags, PAGER_SET_ACTIVE_BEVEL ) )
    {    
        Config->active_desk_bevel = config->active_desk_bevel ;
        set_flags( Config->set_flags, PAGER_SET_ACTIVE_BEVEL );
    }        
    if( get_flags( config->set_flags, PAGER_SET_INACTIVE_BEVEL ) )
    {        
        Config->inactive_desk_bevel = config->inactive_desk_bevel ;
        set_flags( Config->set_flags, PAGER_SET_INACTIVE_BEVEL );
    }

    if( Config->balloon_conf )
        Destroy_balloonConfig( Config->balloon_conf );
    Config->balloon_conf = config->balloon_conf ;
    config->balloon_conf = NULL ;

    if (config->style_defs)
        ProcessMyStyleDefinitions (&(config->style_defs));

    DestroyPagerConfig (config);
    SHOW_TIME("Config parsing",option_time);
}
/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the base file
 *
 ****************************************************************************/
void
GetBaseOptions (const char *filename)
{

    START_TIME(started);

	ReloadASEnvironment( NULL, NULL, NULL, False, True );

	if (Environment->desk_pages_h > 0 )
        PagerState.page_columns = Environment->desk_pages_h;
    if (Environment->desk_pages_v > 0)
        PagerState.page_rows = Environment->desk_pages_v;

    Scr.Vx = 0;
    Scr.Vy = 0;
    PagerState.vscreen_width = Scr.VxMax + Scr.MyDisplayWidth;
    PagerState.vscreen_height = Scr.VyMax + Scr.MyDisplayHeight;
    PagerState.vscaled_desk_width = PagerState.vscreen_width/Scr.VScale;
    PagerState.vscaled_desk_height = PagerState.vscreen_height/Scr.VScale;

    SHOW_TIME("BaseConfigParsingTime",started);
    LOCAL_DEBUG_OUT("desk_size(%dx%d),vscreen_size(%dx%d),vscale(%d)", PagerState.desk_width, PagerState.desk_height, PagerState.vscreen_width, PagerState.vscreen_height, Scr.VScale );
}

/********************************************************************/
/* showing our main window :                                        */
/********************************************************************/
Window
make_pager_window()
{
	Window        w;
	XSizeHints    shints;
	ExtendedWMHints extwm_hints ;
	int x, y ;
    int width = Config->geometry.width;
    int height = Config->geometry.height;
    XSetWindowAttributes attr;
    LOCAL_DEBUG_OUT("configured geometry is %dx%d%+d%+d", width, height, Config->geometry.x, Config->geometry.y );
	switch( Config->gravity )
	{
		case NorthEastGravity :
            x = Scr.MyDisplayWidth - width + Config->geometry.x ;
            y = Config->geometry.y ;
			break;
		case SouthEastGravity :
            x = Scr.MyDisplayWidth - width + Config->geometry.x ;
            y = Scr.MyDisplayHeight - height + Config->geometry.y ;
			break;
		case SouthWestGravity :
            x = Config->geometry.x ;
            y = Scr.MyDisplayHeight - height + Config->geometry.y ;
			break;
		case NorthWestGravity :
		default :
            x = Config->geometry.x ;
            y = Config->geometry.y ;
			break;
	}
    attr.event_mask = StructureNotifyMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask ;
    w = create_visual_window( Scr.asv, Scr.Root, x, y, width, height, 0, InputOutput, CWEventMask, &attr);
    set_client_names( w, MyName, MyName, AS_MODULE_CLASS, CLASS_PAGER );

    Scr.RootClipArea.x = x;
    Scr.RootClipArea.y = y;
    Scr.RootClipArea.width = width;
    Scr.RootClipArea.height = height;

    shints.flags = USSize|PMinSize|PResizeInc|PWinGravity;
    if( get_flags( Config->set_flags, PAGER_SET_GEOMETRY ) || get_flags(MyArgs.geometry.flags, XValue|YValue) )
        shints.flags |= USPosition ;
    else
        shints.flags |= PPosition ;

    shints.min_width = Config->columns+Config->border_width;
    shints.min_height = Config->rows+Config->border_width;
    shints.width_inc = 1;
    shints.height_inc = 1;
	shints.win_gravity = Config->gravity ;

	extwm_hints.pid = getpid();
    extwm_hints.flags = EXTWM_PID|EXTWM_StateSet|EXTWM_TypeSet ;
	extwm_hints.type_flags = EXTWM_TypeMenu|EXTWM_TypeASModule ;
	extwm_hints.state_flags = EXTWM_StateSkipTaskbar|EXTWM_StateSkipPager ;

	set_client_hints( w, NULL, &shints, AS_DoesWmDeleteWindow, &extwm_hints );
	set_client_cmd (w);

	/* showing window to let user see that we are doing something */
	XMapRaised (dpy, w);
    LOCAL_DEBUG_OUT( "mapping main window at %ux%u%+d%+d", width, height,  x, y );
    /* final cleanup */
	XFlush (dpy);
	sleep (1);								   /* we have to give AS a chance to spot us */
	/* we will need to wait for PropertyNotify event indicating transition
	   into Withdrawn state, so selecting event mask: */
	XSelectInput (dpy, w, PropertyChangeMask|StructureNotifyMask);
	return w ;
}

int
update_main_canvas_config()
{
    int changes = handle_canvas_config( PagerState.main_canvas );
    if( changes != 0 )
        set_root_clip_area( PagerState.main_canvas );
    return changes;
}

static void
place_desk_title( ASPagerDesk *d )
{
    if( d->title )
    {
        int x = 0, y = 0;
        int width = d->title_width, height = d->title_height;
        if( get_flags(Config->flags, VERTICAL_LABEL) )
        {
            if( get_flags(Config->flags, LABEL_BELOW_DESK) && !get_flags(d->flags, ASP_DeskShaded ) )
                x = PagerState.desk_width - width - Config->border_width;
            height = PagerState.desk_height-Config->border_width ;
        }else
        {
            if( get_flags(Config->flags, LABEL_BELOW_DESK) && !get_flags(d->flags, ASP_DeskShaded ))
                y = PagerState.desk_height - height - Config->border_width;
            width = PagerState.desk_width-Config->border_width ;
        }
        move_astbar( d->title, d->desk_canvas, x, y);
        set_astbar_size( d->title, width, height );
    }
}

static void
place_desk_background( ASPagerDesk *d )
{
    int x = 0, y = 0;
	int width = PagerState.desk_width ; 
	int height = PagerState.desk_height ; 	
    if( get_flags(Config->flags, USE_LABEL) )
	{
		width -= Config->border_width ; 
		height -= Config->border_width ; 	

       	if( get_flags(Config->flags, VERTICAL_LABEL) )
			width -= d->title_width ; 
		else
			height -= d->title_height ; 
		
		
	    if( !get_flags(Config->flags, LABEL_BELOW_DESK) || get_flags(d->flags, ASP_DeskShaded ))
    	{
        	if( get_flags(Config->flags, VERTICAL_LABEL) )
	            x = d->title_width ;
    	    else
        	    y = d->title_height ;
	    }
	}
    move_astbar( d->background, d->desk_canvas, x, y);
    set_astbar_size( d->background, width, height );
}

static Bool
render_desk( ASPagerDesk *d, Bool force )
{
    if( is_canvas_needs_redraw( d->desk_canvas) )
        force = True;

    if( d->title )
        if( force || DoesBarNeedsRendering(d->title) )
            render_astbar( d->title, d->desk_canvas );
    if( force || DoesBarNeedsRendering(d->background) )
        render_astbar( d->background, d->desk_canvas );

    if( d->desk_canvas->shape == NULL && get_flags(d->flags, ASP_Shaped) )
    {
        clear_flags(d->flags, ASP_Shaped);
        set_flags(d->flags, ASP_ShapeDirty);
    }else if( d->desk_canvas->shape != NULL )
    {
        set_flags(d->flags, ASP_Shaped);
        set_flags(d->flags, ASP_ShapeDirty);
    }

    if( is_canvas_dirty( d->desk_canvas) )
    {
        update_canvas_display( d->desk_canvas );
//		update_pager_shape();
        return True;
    }
    return False;
}

#if 0
static void
update_desk_shape( ASPagerDesk *d )
{
    int i ;

    if( d == NULL )
        return;

    update_canvas_display_mask (d->desk_canvas, True);

    LOCAL_DEBUG_CALLER_OUT( "desk %p flags = 0x%lX", d, d->flags );
    if( get_flags(Config->flags, SHOW_SELECTION) && d->desk == Scr.CurrentDesk )
    {
        XShapeCombineRectangles ( dpy, d->desk_canvas->w, ShapeBounding,
                                  0, 0, &(PagerState.selection_bar_rects[0]), 4, ShapeUnion, Unsorted);
        LOCAL_DEBUG_OUT( "added selection_bar_rects to shape%s","" );
    }
    if( get_flags(Config->flags, PAGE_SEPARATOR) )
    {
        XShapeCombineRectangles ( dpy, d->desk_canvas->w, ShapeBounding,
                                  0, 0, &(d->separator_bar_rects[0]), d->separator_bars_num, ShapeUnion, Unsorted);
        LOCAL_DEBUG_OUT( "added %d separator_bar_rects to shape", d->separator_bars_num);
#if 0
		i = d->separator_bars_num ;
	    while( --i >= 0 )
    	{
        	LOCAL_DEBUG_OUT( "\t %dx%d%+d%+d", d->separator_bar_rects[i].width,
            	                                d->separator_bar_rects[i].height,
                	                            d->separator_bar_rects[i].x,
                    	                        d->separator_bar_rects[i].y );
	    }
#endif
    }

    if( d->clients_num > 0 )
    {
        register ASWindowData **clients = d->clients ;
        i = d->clients_num ;
        LOCAL_DEBUG_OUT( "clients_num %d", d->clients_num );
        while( --i >= 0 )
        {
            LOCAL_DEBUG_OUT( "client %d data %p", i, clients[i] );
            if( clients[i] )
            {
                LOCAL_DEBUG_OUT( "combining client \"%s\"", clients[i]->icon_name );
                combine_canvas_shape( d->desk_canvas, clients[i]->canvas, False, True );
            }
        }
    }
    clear_flags( d->flags, ASP_ShapeDirty );
}
#endif

void
update_pager_shape()
{
#ifdef SHAPE
    int i ;
    Bool shape_cleared = False ;
	XRectangle border[4] ;

    if( get_flags( PagerState.flags, ASP_ReceivingWindowList ) )
        return ;

    LOCAL_DEBUG_CALLER_OUT( "pager flags = 0x%lX", PagerState.flags );

    if( get_flags( PagerState.flags, ASP_ShapeDirty ) )
    {
        shape_cleared = True ;
        clear_flags( PagerState.flags, ASP_ShapeDirty );
    }

	if( PagerState.main_canvas->shape )
		flush_vector( PagerState.main_canvas->shape );
	else
		PagerState.main_canvas->shape = create_shape();

    for( i = 0 ; i < PagerState.desks_num; ++i )
    {
        ASPagerDesk *d = &(PagerState.desks[i]) ;
        int x, y ;
		unsigned int d_width, d_height, bw ;

#ifdef STRICT_GEOMETRY        
		get_current_canvas_geometry( d->desk_canvas, &x, &y, &d_width, &d_height, &bw );
#else
		x = d->desk_canvas->root_x - PagerState.main_canvas->root_x ;
		y = d->desk_canvas->root_y - PagerState.main_canvas->root_y ;
		d_width = d->desk_canvas->width ;
		d_height = d->desk_canvas->height ;
		bw = Config->border_width ;
#endif

		LOCAL_DEBUG_OUT( "desk geometry = %dx%d%+d%+d, bw = %d", d_width, d_height, x, y, bw );
		combine_canvas_shape_at_geom( PagerState.main_canvas, d->desk_canvas, x, y, d_width, d_height, bw );

		if( Config->MSDeskBack[i]->texture_type == TEXTURE_SHAPED_PIXMAP ||
            Config->MSDeskBack[i]->texture_type == TEXTURE_SHAPED_SCALED_PIXMAP )
		{
	    	if( get_flags(Config->flags, SHOW_SELECTION) && d->desk == Scr.CurrentDesk )
				add_shape_rectangles( PagerState.main_canvas->shape, &(PagerState.selection_bar_rects[0]), 4, x, y, PagerState.main_canvas->width, PagerState.main_canvas->height );

			if( get_flags(Config->flags, PAGE_SEPARATOR) )
				add_shape_rectangles( PagerState.main_canvas->shape, &(d->separator_bar_rects[0]), d->separator_bars_num, x, y, PagerState.main_canvas->width, PagerState.main_canvas->height );

	    	if( d->clients_num > 0 )
    		{
        		register ASWindowData **clients = d->clients ;
        		int k = d->clients_num ;
        		LOCAL_DEBUG_OUT( "desk %d clients_num %d", i, d->clients_num );
        		while( --k >= 0 )
        		{
            		LOCAL_DEBUG_OUT( "client %d data %p", i, clients[k] );
            		if( clients[k] && clients[k]->canvas )
            		{
			        	int client_x, client_y ;
						unsigned int client_width, client_height, client_bw ;
						get_current_canvas_geometry( clients[k]->canvas, &client_x, &client_y, &client_width, &client_height, &client_bw );

                		LOCAL_DEBUG_OUT( "combining client \"%s\"", clients[k]->icon_name );
                		combine_canvas_shape_at_geom(PagerState.main_canvas, clients[k]->canvas, client_x+x, client_y+y, client_width, client_height, client_bw );
            		}
        		}
			}
		}
	    clear_flags( d->flags, ASP_ShapeDirty );
    }
	
	border[0].x = 0 ;
	border[0].y = PagerState.main_canvas->height - Config->border_width ;
	border[0].width = PagerState.main_canvas->width ;
	border[0].height = Config->border_width ;
	border[1].x = PagerState.main_canvas->width - Config->border_width ;
	border[1].y = 0 ;
	border[1].width = Config->border_width ;
	border[1].height = PagerState.main_canvas->height ;
#if 1
	border[2].x = 0 ;
	border[2].y = 0 ;
	border[2].width = PagerState.main_canvas->width ;
	border[2].height = Config->border_width ;
	border[3].x = 0 ;
	border[3].y = 0 ;
	border[3].width = Config->border_width ;
	border[3].height = PagerState.main_canvas->height ;
#endif	

	add_shape_rectangles( PagerState.main_canvas->shape, &(border[0]), 4, 0, 0, PagerState.main_canvas->width, PagerState.main_canvas->height );

	update_canvas_display_mask (PagerState.main_canvas, True);

#endif
}


unsigned int
calculate_desk_width( ASPagerDesk *d )
{
    unsigned int width = PagerState.desk_width;

	if( get_flags( Config->flags, VERTICAL_LABEL ) && d->title_width > 0 )
    {
        if( get_flags( d->flags, ASP_DeskShaded ) )
            width = d->title_width+Config->border_width;
    }
    return width;
}

unsigned int
calculate_desk_height( ASPagerDesk *d )
{
    unsigned int height = PagerState.desk_height;

    if( !get_flags( Config->flags, VERTICAL_LABEL ) && d->title_height > 0 )
    {
        if( get_flags( d->flags, ASP_DeskShaded ) )
            height = d->title_height+Config->border_width;
    }
    return height;
}

void place_desk( ASPagerDesk *d, int x, int y, unsigned int width, unsigned int height )
{
    int           win_x = 0, win_y = 0;
	unsigned int bw ;
    get_canvas_position( d->desk_canvas, NULL, &win_x, &win_y, &bw );
    LOCAL_DEBUG_OUT( "desk window %lX, curr_geom = %dx%d%+d%+d, bw = %d, new_geom = %dx%d%+d%+d", d->desk_canvas->w, d->desk_canvas->width, d->desk_canvas->height, win_x, win_y, bw, width, height, x, y );
    if( d->desk_canvas->width == width && d->desk_canvas->height == height && win_x == x && win_y == y )
    {
        on_desk_moveresize( d );
    }else
        moveresize_canvas( d->desk_canvas, x, y, width, height );
}

inline ASPagerDesk *
get_pager_desk( INT32 desk )
{
	if( IsValidDesk(desk) )
	{
        register INT32 pager_desk = desk - PagerState.start_desk ;
	    if(  pager_desk >= 0 && pager_desk < PagerState.desks_num )
  		    return &(PagerState.desks[pager_desk]);
	}
    return NULL ;
}

void
restack_desk_windows( ASPagerDesk *d )
{
    Window *list, *curr ;
    int win_count = 0 ;
    int i, k ;
    if( d == NULL )
        return;

    win_count = d->clients_num ;
    if( get_flags(Config->flags, SHOW_SELECTION) && d->desk == Scr.CurrentDesk )
        win_count += 4 ;
    if( get_flags(Config->flags, PAGE_SEPARATOR) )
        win_count += d->separator_bars_num ;

    if( win_count <= 1 )
        return ;

    curr = list = safecalloc( win_count, sizeof(Window));
	k = 0 ;

    if( get_flags(Config->flags, SHOW_SELECTION) && d->desk == Scr.CurrentDesk )
        for( i = 0 ; i < 4 ; ++i )
		{
			if( PagerState.selection_bars[i] )
			{
	            curr[k] = PagerState.selection_bars[i] ;
				++k ;
			}
		}

    if( get_flags(Config->flags, PAGE_SEPARATOR) )
    {
        register Window *sbars = d->separator_bars;
        i = d->separator_bars_num ;
        while( --i >= 0 )
		{
			if( sbars[i] )
			{
          		curr[k] = sbars[i] ;
				++k ;
			}
	    }
	}

    if( d->clients_num > 0 )
    {
        register ASWindowData **clients = d->clients ;
        i = -1 ;
        while( ++i < d->clients_num )
        {
            if( clients[i] && clients[i]->desk == d->desk &&
			    clients[i]->canvas  &&
				clients[i]->canvas->w )
			{
				LOCAL_DEBUG_OUT("k = %d, w = %lX, client = %lX, wd = %p, name = \"%s\"", 
								 k, clients[i]->canvas->w,clients[i]->client, clients[i], 
								 clients[i]->window_name?clients[i]->window_name:"(null)" );
                curr[k++] = clients[i]->canvas->w ;
			}
        }
    }

    XRaiseWindow( dpy, list[0] );
    XRestackWindows( dpy, list, k );
	free( list );
}

void
place_separation_bars( ASPagerDesk *d )
{
	register Window *wa = d?d->separator_bars:NULL;
    register XRectangle *wrecta = d?d->separator_bar_rects:NULL;
    if( wa )
    {
        register int p = PagerState.page_columns-1;
        int pos_inc = d->background->width/PagerState.page_columns ;
        int pos = d->background->win_x+p*pos_inc;
        int size = d->background->height ;
        int pos2 = d->background->win_y ;
        /* vertical bars : */
        while( --p >= 0 )
        {
            wrecta[p].x = pos ;
            wrecta[p].y = pos2 ;
            wrecta[p].width = 1 ;
            wrecta[p].height = size ;
            XMoveResizeWindow( dpy, wa[p], pos, pos2, 1, size );
            pos -= pos_inc ;
        }
        /* horizontal bars */
        wa += PagerState.page_columns-1;
        wrecta += PagerState.page_columns-1;
        p = PagerState.page_rows-1;
        pos_inc = d->background->height/PagerState.page_rows ;
        pos = d->background->win_y + p*pos_inc;
        pos2 = d->background->win_x ;
        size = d->background->width ;
        while( --p >= 0 )
        {
            wrecta[p].x = pos2 ;
            wrecta[p].y = pos ;
            wrecta[p].width = size ;
            wrecta[p].height = 1 ;
	        XMoveResizeWindow( dpy, wa[p], pos2, pos, size, 1 );
            pos -= pos_inc ;
        }
    }
}

void
place_selection()
{
LOCAL_DEBUG_CALLER_OUT( "Scr.CurrentDesk(%d)->start_desk(%ld)", Scr.CurrentDesk, PagerState.start_desk );
    if( get_flags(Config->flags, SHOW_SELECTION) )
	{
        ASPagerDesk *sel_desk = get_pager_desk( Scr.CurrentDesk );

  	    if( sel_desk!= NULL && sel_desk->background && sel_desk->desk_canvas )
  		{
      		int sel_x = sel_desk->background->win_x ;
	        int sel_y = sel_desk->background->win_y ;
  		    int page_width = sel_desk->background->width/PagerState.page_columns ;
      		int page_height = sel_desk->background->height/PagerState.page_rows ;
	        int i = 4;

  		    sel_x += (Scr.Vx*page_width)/Scr.MyDisplayWidth ;
      		sel_y += (Scr.Vy*page_height)/Scr.MyDisplayHeight ;
LOCAL_DEBUG_OUT( "sel_pos(%+d%+d)->page_size(%dx%d)->desk(%ld)", sel_x, sel_y, page_width, page_height, sel_desk->desk );
	        while ( --i >= 0 )
  		        XReparentWindow( dpy, PagerState.selection_bars[i], sel_desk->desk_canvas->w, -10, -10 );

      		PagerState.selection_bar_rects[0].x = sel_x-1 ;
	        PagerState.selection_bar_rects[0].y = sel_y-1 ;
  		    PagerState.selection_bar_rects[0].width = page_width+2 ;
      		PagerState.selection_bar_rects[0].height = 1 ;

	        PagerState.selection_bar_rects[1].x = sel_x-1 ;
  		    PagerState.selection_bar_rects[1].y = sel_y-1 ;
	        PagerState.selection_bar_rects[1].width = 1 ;
  		    PagerState.selection_bar_rects[1].height = page_height+2;

	        PagerState.selection_bar_rects[2].x = sel_x-1 ;
  		    PagerState.selection_bar_rects[2].y = sel_y+page_height+1 ;
	        PagerState.selection_bar_rects[2].width = page_width+2 ;
  		    PagerState.selection_bar_rects[2].height = 1 ;

      		PagerState.selection_bar_rects[3].x = sel_x+page_width+1 ;
	        PagerState.selection_bar_rects[3].y = sel_y-1 ;
  		    PagerState.selection_bar_rects[3].width = 1 ;
      		PagerState.selection_bar_rects[3].height = page_height+2 ;

	        if( !get_flags( sel_desk->flags, ASP_DeskShaded ) )
  		    {
      		    i = 4 ;
          		while ( --i >= 0 )
	                XMoveResizeWindow( dpy, PagerState.selection_bars[i],
  	                                      PagerState.selection_bar_rects[i].x,
    	                                  PagerState.selection_bar_rects[i].y,
      	                                  PagerState.selection_bar_rects[i].width,
        	                              PagerState.selection_bar_rects[i].height );
      		}
  		    XMapSubwindows( dpy, sel_desk->desk_canvas->w );
			ASSync(False);
	        restack_desk_windows( sel_desk );
      		set_flags(sel_desk->flags, ASP_ShapeDirty);
		}
    }
}

void
redecorate_pager_desks()
{
    /* need to create enough desktop canvases */
    int i ;
    char buf[256];
    XSetWindowAttributes attr;
    int wasted_x = Config->border_width * (Config->columns) ;
    int wasted_y = Config->border_width * (Config->rows);
	int max_title_width = 0 ;
	int max_title_height = 0 ;

    for( i = 0 ; i < PagerState.desks_num ; ++i )
    {
        ASPagerDesk *d = &(PagerState.desks[i]);
        int p ;
		Bool just_created_background = False ;

        ARGB2PIXEL(Scr.asv,Config->border_color_argb,&(attr.border_pixel));
        if( d->desk_canvas == NULL )
        {
            Window w;
            attr.event_mask = StructureNotifyMask|ButtonReleaseMask|ButtonPressMask|ButtonMotionMask ;

            w = create_visual_window(Scr.asv, PagerState.main_canvas->w, 0, 0, PagerState.desk_width, PagerState.desk_height,
                                     Config->border_width, InputOutput, CWEventMask|CWBorderPixel, &attr );
            d->desk_canvas = create_ascanvas( w );
            LOCAL_DEBUG_OUT("+CREAT canvas(%p)->desk(%ld)->geom(%dx%d%+d%+d)->parent(%lx)", d->desk_canvas, PagerState.start_desk+i, PagerState.desk_width, PagerState.desk_height, 0, 0, PagerState.main_canvas->w );
            handle_canvas_config( d->desk_canvas );
        }else
		{
			XSetWindowBorder( dpy, d->desk_canvas->w, attr.border_pixel );
		}
        /* create & moveresize label bar : */
        if( get_flags( Config->flags, USE_LABEL ) )
        {
            int align = Config->align ;
			ASFlagType ibevel = Config->inactive_desk_bevel;
			ASFlagType abevel = Config->active_desk_bevel;
			int h_spacing = Config->h_spacing ;
			int v_spacing = Config->v_spacing ;
            int flip = get_flags(Config->flags, VERTICAL_LABEL)?FLIP_VERTICAL:0;
			Bool just_created = False ;

			if( !get_flags( Config->set_flags, PAGER_SET_ALIGN ) )
				align = PagerState.tbar_props->align ;
			if( !get_flags( Config->set_flags, PAGER_SET_ACTIVE_BEVEL ) )
				abevel = PagerState.tbar_props->bevel ;
			if( !get_flags( Config->set_flags, PAGER_SET_INACTIVE_BEVEL ) )
				ibevel = PagerState.tbar_props->bevel ;
			h_spacing = PagerState.tbar_props->title_h_spacing ;
			v_spacing = PagerState.tbar_props->title_v_spacing ;

            if( d->title == NULL )
            {
                d->title = create_astbar();
                d->title->context = C_TITLE ;
				just_created = True ;
            }else /* delete label if it was previously created : */
	            delete_astbar_tile( d->title, -1 );
				
            set_astbar_hilite( d->title, BAR_STATE_UNFOCUSED, ibevel);
            set_astbar_hilite( d->title, BAR_STATE_FOCUSED,   abevel);

            set_astbar_style_ptr( d->title, BAR_STATE_FOCUSED, Config->MSDeskTitle[DESK_ACTIVE] );
            set_astbar_style_ptr( d->title, BAR_STATE_UNFOCUSED, Config->MSDeskTitle[DESK_INACTIVE] );
            
            if( Config->labels && Config->labels[i] )
                add_astbar_label( d->title, 0, flip?1:0, flip, align, h_spacing, v_spacing, Config->labels[i], AS_Text_ASCII );
            else
            {
                sprintf( buf, "Desk %ld", PagerState.start_desk+i );
                add_astbar_label( d->title, 0, flip?1:0, flip, align, h_spacing, v_spacing, buf, AS_Text_ASCII );
            }
            if( PagerState.shade_button.context != C_NO_CONTEXT )
			{	
				MyButton *list[1] ;
				list[0] = &(PagerState.shade_button);
                add_astbar_btnblock(d->title, flip?0:1, 0, flip, NO_ALIGN, &list[0], 0xFFFFFFFF, 1, 
									PagerState.tbar_props->buttons_h_border, 
									PagerState.tbar_props->buttons_v_border, 
									PagerState.tbar_props->buttons_spacing, 0);
			}
            if( get_flags( Config->flags, VERTICAL_LABEL ) )
            {
				int size = calculate_astbar_width( d->title );
				if( size  > max_title_width )
					max_title_width = size ;
                d->title_height = PagerState.desk_height - Config->border_width*2;
				/*we do it below using max_title_width : wasted_x += d->title_width ; */
            }else
            {
                int size = calculate_astbar_height( d->title );
				LOCAL_DEBUG_OUT( "size = %d, max_title_height = %d", size, max_title_height );
                d->title_width = PagerState.desk_width-Config->border_width*2;
				if( size  > max_title_height )
					max_title_height = size ;
				/*we do it below using max_title_height : wasted_y += d->title_height ;*/
            }
        }else
        {
            if( d->title )
                destroy_astbar( &(d->title) );
            d->title_width = d->title_height = 0 ;
        }

        /* create & moveresize desktop background bar : */
        if( d->background == NULL )
		{
            d->background = create_astbar();
			just_created_background = True ;
		}

        set_astbar_style_ptr( d->background, BAR_STATE_FOCUSED, Config->MSDeskBack[i] );
        set_astbar_style_ptr( d->background, BAR_STATE_UNFOCUSED, Config->MSDeskBack[i] );
        if( Config->styles[i] == NULL )
            set_flags( d->flags, ASP_UseRootBackground );

		if( just_created_background )
			place_desk_background( d );
		if( get_flags( d->flags, ASP_UseRootBackground ) )
			request_background_image( d );

        if( d->separator_bars )
        {
            for( p = 0 ; p < d->separator_bars_num ; ++p )
                if( d->separator_bars[p] )
                    XDestroyWindow( dpy, d->separator_bars[p] );
            free( d->separator_bars );
            free( d->separator_bar_rects );
            d->separator_bars_num = 0 ;
            d->separator_bars = NULL ;
			d->separator_bar_rects = NULL ;
        }
        if( get_flags( Config->flags, PAGE_SEPARATOR ) )
        {
            d->separator_bars_num = PagerState.page_columns-1+PagerState.page_rows-1 ;
            d->separator_bars = safecalloc( d->separator_bars_num, sizeof(Window));
            d->separator_bar_rects = safecalloc( d->separator_bars_num, sizeof(XRectangle));
            ARGB2PIXEL(Scr.asv,Config->grid_color_argb,&(attr.background_pixel));
            for( p = 0 ; p < d->separator_bars_num ; ++p )
            {
                d->separator_bars[p] = create_visual_window(Scr.asv, d->desk_canvas->w, 0, 0, 1, 1,
                                                             0, InputOutput, CWBackPixel, &attr );
                d->separator_bar_rects[p].width = d->separator_bar_rects[p].height = 1 ;
            }
            XMapSubwindows( dpy, d->desk_canvas->w );
			place_separation_bars( d );
        }
    }

    if( get_flags( Config->flags, USE_LABEL ) )
    {
		for( i = 0 ; i < PagerState.desks_num ; ++i )
    	{	/* create & moveresize label bar : */
        	ASPagerDesk *d = &(PagerState.desks[i]);

            if( get_flags( Config->flags, VERTICAL_LABEL ) )
                d->title_width = max_title_width;
            else
                d->title_height = max_title_height;
			place_desk_title( d );
		}
        
        if( get_flags( Config->flags, VERTICAL_LABEL ) )
        {    
            wasted_x += max_title_width*Config->columns ;
            LOCAL_DEBUG_OUT( "title_width = %d", max_title_width );
        }else 
        {    
            wasted_y += max_title_height*Config->rows ;
            LOCAL_DEBUG_OUT( "title_height = %d", max_title_height );
        }
	}

	/* if wasted space changed and configured geometry does not specify size -
	 * adjust desk_width/height accordingly :
	 */
	if( !get_flags( Config->geometry.flags, WidthValue ) )
	{
        int delta = (wasted_x - PagerState.wasted_width)/Config->columns ;
        LOCAL_DEBUG_OUT( "wasted_x = %d, (old was = %d) - adjusting desk_width by %d", wasted_x, PagerState.wasted_width, delta );
        if( delta != 0 )
		{
			PagerState.desk_width += delta ;
			PagerState.wasted_width += delta * Config->columns  ;
		}
	}
	if( !get_flags( Config->geometry.flags, HeightValue ) )
	{
        int delta = (wasted_y - PagerState.wasted_height)/Config->rows ;
		LOCAL_DEBUG_OUT( " :RESIZING: wasted_y = %d, (old was = %d) - adjusting desk_height(%d) by %d", wasted_y, PagerState.wasted_height, PagerState.desk_height, delta );
		if( delta != 0 )
		{
			PagerState.desk_height += delta ;
			PagerState.wasted_height += delta * Config->rows ;
		}
	}

    /* selection bars : */
    ARGB2PIXEL(Scr.asv,Config->selection_color_argb,&(attr.background_pixel));
    if(get_flags( Config->flags, SHOW_SELECTION ) )
    {
        for( i = 0 ; i < 4 ; i++ )
            if( PagerState.selection_bars[i] == None )
			{
                PagerState.selection_bars[i] = create_visual_window( Scr.asv, PagerState.main_canvas->w, 0, 0, 1, 1,
                                                                     0, InputOutput, CWBackPixel, &attr );
			}else
			{
				XSetWindowBackground( dpy, PagerState.selection_bars[i], attr.background_pixel ) ;
				XClearWindow( dpy, PagerState.selection_bars[i] );
			}
    }else
        for( i = 0 ; i < 4 ; i++ )
            if( PagerState.selection_bars[i] != None )
            {
                XDestroyWindow( dpy, PagerState.selection_bars[i] );
                PagerState.selection_bars[i] = None ;
            }
    XMapSubwindows( dpy, PagerState.main_canvas->w );
}

void
rearrange_pager_desks(Bool dont_resize_main )
{
    /* need to create enough desktop canvases */
    int i ;
    int col = 0;
    int x = 0, y = 0, row_height = 0;
    /* Pass 1: first we must resize or main window !!! */
    update_main_canvas_config();
    if( !dont_resize_main )
    {
        int all_width = 0, all_height = 0;
        for( i = 0 ; i < PagerState.desks_num ; ++i )
        {
            ASPagerDesk *d = &(PagerState.desks[i]);
            int width, height;

            width = calculate_desk_width( d );
            height = calculate_desk_height( d );
            
            if( height > row_height )
                row_height = height;
            
            LOCAL_DEBUG_OUT( " :RESIZING: desk = %d, size = %dx%d, all_size = %+d%+d", i, width, height, all_width, all_height );

            if( ++col >= Config->columns )
            {
                if( all_width < x+width )
                    all_width = x+width ;
                y += row_height;

                row_height = 0 ;
                x = 0 ;
                col = 0;
            }else
                x += width;
        }
        if( all_width < x )
            all_width = x ;
        if( all_height < y+row_height )
            all_height = y+row_height;
        all_width += Config->border_width ;
        all_height += Config->border_width ;
        LOCAL_DEBUG_OUT( " :RESIZING: resizing_main : all_size = %dx%d current size = %dx%d", 
                         all_width, all_height, PagerState.main_canvas->width, PagerState.main_canvas->height );
        if( PagerState.main_canvas->width != all_width || PagerState.main_canvas->height != all_height )
        {
            resize_canvas( PagerState.main_canvas, all_width, all_height );
            return;
        }
    }
    /* Pass 2: now we can resize the rest of the windows : */
    col = 0;
    x = y = 0;
    row_height = 0 ;
    for( i = 0 ; i < PagerState.desks_num ; ++i )
    {
        ASPagerDesk *d = &(PagerState.desks[i]);
        int width, height;

        width = calculate_desk_width( d );
        height = calculate_desk_height( d );

        place_desk( d, x, y, width-Config->border_width, height-Config->border_width );

        if( height > row_height )
            row_height = height;

        if( ++col >= Config->columns )
        {
            y += row_height;
            row_height = 0 ;
            x = 0 ;
            col = 0;
        }else
            x += width;
    }
    ASSync(False);
}

unsigned int
calculate_pager_desk_width()
{
    int main_width = PagerState.main_canvas->width - Config->border_width;
    if( !get_flags( Config->flags, VERTICAL_LABEL ) )
        return main_width/Config->columns;
    else
    {
        unsigned int unshaded_col_count = 0 ;
        unsigned int shaded_width = 0 ;
        int col = 0 ;
        /* we have to calculate the number of not-shaded columns,
        * and then devide size of the main canvas by this number : */
        for( col = 0 ; col < Config->columns ; ++col )
        {
            Bool unshaded = False ;
            unsigned int col_shaded_width = 0 ;
            int i ;

            for( i = col ; i < PagerState.desks_num ; i+= Config->columns )
            {
                ASPagerDesk *d = &(PagerState.desks[i]);
                if( !get_flags(d->flags, ASP_DeskShaded )  )
                    unshaded = True ;
				else if( col_shaded_width < d->title_width )
                    col_shaded_width = d->title_width ;
            }
            if( unshaded )
                ++unshaded_col_count ;
            shaded_width += col_shaded_width ;
        }

LOCAL_DEBUG_OUT( "unshaded_col_count = %d", unshaded_col_count );
        if( unshaded_col_count == 0 )
            return PagerState.desk_width;
        return (main_width-shaded_width)/unshaded_col_count;
    }
}

unsigned int
calculate_pager_desk_height()
{
    int main_height = PagerState.main_canvas->height - Config->border_width ;
    if( get_flags( Config->flags, VERTICAL_LABEL ) )
        return main_height/Config->rows;
    else
    {
        unsigned int unshaded_row_count = 0 ;
        unsigned int shaded_height = 0 ;
        int row ;
        /* we have to calculate the number of not-shaded columns,
        * and then devide size of the main canvas by this number : */
        for( row = 0 ; row < Config->rows ; ++row )
        {
            Bool unshaded = False ;
            unsigned int row_shaded_height = 0 ;
            int i, max_i = (row+1)*Config->columns ;

			if( max_i > PagerState.desks_num )
				max_i = PagerState.desks_num ;

            for( i = row*Config->columns ; i < max_i ; ++i  )
            {
                ASPagerDesk *d = &(PagerState.desks[i]);
                if( !get_flags(d->flags, ASP_DeskShaded )  )
                    unshaded = True ;
				else if( row_shaded_height < d->title_height )
                    row_shaded_height = d->title_height ;
            }
            if( unshaded )
                ++unshaded_row_count ;
            shaded_height += row_shaded_height ;
        }
LOCAL_DEBUG_OUT( "unshaded_row_count = %d", unshaded_row_count );
        if( unshaded_row_count == 0 )
            return PagerState.desk_height;
        return (main_height - shaded_height)/unshaded_row_count;
    }
}


/*************************************************************************
 * shading/unshading
 *************************************************************************/
void
shade_desk_column( ASPagerDesk *d, Bool shade )
{
    int i ;
    for( i = (d->desk-PagerState.start_desk)%Config->columns ; i < PagerState.desks_num ; i += Config->columns )
    {
        if( shade )
            set_flags( PagerState.desks[i].flags, ASP_DeskShaded );
        else
            clear_flags( PagerState.desks[i].flags, ASP_DeskShaded );
    }
    rearrange_pager_desks( False );
}

void
shade_desk_row( ASPagerDesk *d, Bool shade )
{
    int col_start = ((d->desk-PagerState.start_desk)/Config->columns)*Config->columns ;
    int i ;
    for( i = col_start ; i < col_start+Config->columns ; ++i )
    {
        if( shade )
            set_flags( PagerState.desks[i].flags, ASP_DeskShaded );
        else
            clear_flags( PagerState.desks[i].flags, ASP_DeskShaded );
    }
	LOCAL_DEBUG_OUT( "Shading - rcol_start = %d", col_start );
    rearrange_pager_desks( False );
}
/*************************************************************************
 *
 *************************************************************************/
void
set_client_name( ASWindowData *wd, Bool redraw )
{
    if( wd->bar )
    {
		LOCAL_DEBUG_OUT( "name_enc = %ld, name = \"%s\"", wd->window_name_encoding, 
															wd->window_name?wd->window_name:"(null)");
        change_astbar_first_label( wd->bar, wd->window_name, wd->window_name_encoding );
        set_astbar_balloon( wd->bar, 0, wd->window_name, wd->window_name_encoding );
    }
    if( redraw && wd->canvas )
        render_astbar( wd->bar, wd->canvas );
}

void
place_client( ASPagerDesk *d, ASWindowData *wd, Bool force_redraw, Bool dont_update_shape )
{
    int x = 0, y = 0, width = 1, height = 1;
    int curr_x, curr_y ;
    int desk_width = d->background->width ;
    int desk_height = d->background->height ;

	if( desk_width == 0 || desk_height == 0 )
		return ;
    if( wd )
    {
        int client_x = wd->frame_rect.x ;
        int client_y = wd->frame_rect.y ;
        int client_width  = wd->frame_rect.width ;
        int client_height = wd->frame_rect.height ;
        if( get_flags( wd->state_flags, AS_Iconic )  )
        {
            client_x = wd->icon_rect.x+Scr.Vx ;
            client_y = wd->icon_rect.y+Scr.Vy ;
            client_width  = wd->icon_rect.width ;
            client_height = wd->icon_rect.height ;
        }else
		{
			if( get_flags( wd->state_flags, AS_Sticky )  )
        	{
            	client_x += Scr.Vx ;
            	client_y += Scr.Vy ;
        	}
			if( get_flags( wd->state_flags, AS_Shaded )  )
        	{
            	if( get_flags( wd->flags, AS_VerticalTitle )  )
                	client_width  = (PagerState.vscreen_width *2)/desk_width ;
            	else
                	client_height = (PagerState.vscreen_height*2)/desk_height;
			}
		}
        LOCAL_DEBUG_OUT( "+PLACE->client(%lX)->frame_geom(%dx%d%+d%+d)", wd->client, client_width, client_height, client_x, client_y );
        x = ( client_x * desk_width )/PagerState.vscreen_width ;
        y = ( client_y * desk_height)/PagerState.vscreen_height;
        width  = ( client_width  * desk_width )/PagerState.vscreen_width ;
        height = ( client_height * desk_height)/PagerState.vscreen_height;
        if( x < 0 )
        {
            width += x ;
            x = 0 ;
        }
        if( y < 0 )
        {
            height += y ;
            y = 0 ;
        }
        if( width <= 0 )
            width = 1;
        if( height <= 0 )
            height = 1 ;

		LOCAL_DEBUG_OUT( "@@@@     ###   $$$   Client \"%s\" background position is %+d%+d, client's = %+d%+d", wd->window_name?wd->window_name:"(null)", d->background->win_x, d->background->win_y, x, y );
        x += (int)d->background->win_x ;
        y += (int)d->background->win_y ;
        if( wd->canvas )
        {
            ASCanvas *canvas = wd->canvas ;
			unsigned int bw = 0 ;
            get_canvas_position( canvas, NULL, &curr_x, &curr_y, &bw );
			LOCAL_DEBUG_OUT( "current canvas at = %+d%+d", curr_x, curr_y );
            if( curr_x == x && curr_y == y && width == canvas->width && height == canvas->height  )
            {
                if( force_redraw )
                {
                    render_astbar( wd->bar, canvas );
                    update_canvas_display( canvas );
                }
            }else
                moveresize_canvas( canvas, x - bw, y - bw, width, height );

            LOCAL_DEBUG_OUT( "+PLACE->canvas(%p)->geom(%dx%d%+d%+d)", wd->canvas, width, height, x, y );
        }
    }
}

void
set_client_look( ASWindowData *wd, Bool redraw )
{
    LOCAL_DEBUG_CALLER_OUT( "%p, %p", wd, wd->bar );

    if( wd->bar )
    {
		int state = get_flags( wd->state_flags, AS_Sticky )?BACK_STICKY:BACK_UNFOCUSED;
       	set_astbar_style_ptr( wd->bar, BAR_STATE_UNFOCUSED, Scr.Look.MSWindow[state] );
        set_astbar_style_ptr( wd->bar, BAR_STATE_FOCUSED, Scr.Look.MSWindow[BACK_FOCUSED] );
    }else
		show_warning( "NULL tbar for window data found. client = %lX", wd->client );

    if( redraw && wd->canvas )
        render_astbar( wd->bar, wd->canvas );
}

void
on_client_moveresize(ASWindowData *wd )
{
    if( handle_canvas_config( wd->canvas ) != 0 )
    {
        ASPagerDesk *d = get_pager_desk( wd->desk );
        set_astbar_size( wd->bar, wd->canvas->width, wd->canvas->height );
        render_astbar( wd->bar, wd->canvas );
        update_canvas_display( wd->canvas );
        if( d )
            set_flags(d->flags, ASP_ShapeDirty);
    }
}

Bool
register_client( ASWindowData *wd )
{
    if( PagerClients == NULL )
        PagerClients = create_ashash(0, NULL, NULL, NULL);

    return (add_hash_item(PagerClients, AS_HASHABLE(wd->canvas->w), wd) == ASH_Success);
}

ASWindowData *
fetch_client( Window w )
{
	ASHashData hdata  = {0};
    if( PagerClients )
        if( get_hash_item( PagerClients, AS_HASHABLE(w), &hdata.vptr ) != ASH_Success )
			hdata.vptr = NULL ;
    return hdata.vptr ;
}

void
unregister_client( Window w )
{
    if( PagerClients )
        remove_hash_item( PagerClients, AS_HASHABLE(w), NULL, False );
}

void forget_desk_client( int desk, ASWindowData *wd )
{
    ASPagerDesk *d = get_pager_desk( desk );
    LOCAL_DEBUG_CALLER_OUT( "%d(%p),%p", desk, d, wd );
    if( d && wd && d->clients )
    {
        register int i = d->clients_num ;
        while( --i >= 0 )
            if( d->clients[i] == wd )
            {
                register int k = i, last_k = d->clients_num ;
		        LOCAL_DEBUG_OUT( "client found at %d", i );
                while( ++k < last_k )
                    d->clients[k-1] = d->clients[k] ;
                d->clients[k-1] = NULL;
                --(d->clients_num);
            }
        if( i >= 0 )
            set_flags( d->flags, ASP_ShapeDirty);
    }
}

void add_desk_client( ASPagerDesk *d, ASWindowData *wd )
{
    LOCAL_DEBUG_OUT( "%p, %p, index %d", d, wd, d?d->clients_num:-1 );
    if( d && wd )
    {
		int i = d->clients_num;
		while( --i >= 0 )
			if( d->clients[i] == wd )
				return ; /* already belongs to that desk */
        d->clients = realloc( d->clients, (d->clients_num+1)*sizeof(ASWindowData*));
        d->clients[d->clients_num] = wd ;
        ++(d->clients_num);
        set_flags( d->flags, ASP_ShapeDirty);
    }
}

void add_client( ASWindowData *wd )
{
    ASPagerDesk *d = get_pager_desk( wd->desk );
    Window w;
    XSetWindowAttributes attr ;

    if( d == NULL )
        return;

    attr.event_mask = CLIENT_EVENT_MASK ;
    /* create window, canvas and tbar : */
    w = create_visual_window( Scr.asv, d->desk_canvas->w, -1, -1, 1, 1, 0, InputOutput, CWEventMask, &attr );
    if( w == None )
        return;
 
    wd->canvas = create_ascanvas( w );
    wd->bar = create_astbar();

    add_desk_client( d, wd );
    register_client( wd );

    set_astbar_hilite( wd->bar, BAR_STATE_UNFOCUSED, NORMAL_HILITE|NO_HILITE_OUTLINE );
    set_astbar_hilite( wd->bar, BAR_STATE_FOCUSED, NORMAL_HILITE|NO_HILITE_OUTLINE );
    add_astbar_label( wd->bar, 0, 0, 0, NO_ALIGN, 0, 0, NULL, AS_Text_ASCII );
    move_astbar( wd->bar, wd->canvas, 0, 0 );
    if( wd->focused )
        set_astbar_focused( wd->bar, NULL, True );

    set_client_name( wd, False);
    set_client_look( wd, False );
    place_client( d, wd, True, False );
    map_canvas_window( wd->canvas, True );
    LOCAL_DEBUG_OUT( "+CREAT->canvas(%p)->bar(%p)->client_win(%lX)", wd->canvas, wd->bar, wd->client );
}

void refresh_client( INT32 old_desk, ASWindowData *wd )
{
    ASPagerDesk *d = get_pager_desk( wd->desk );
    LOCAL_DEBUG_OUT( "client(%lX)->name(%s)->icon_name(%s)->desk(%ld)->old_desk(%ld)", wd->client, wd->window_name?wd->window_name:"(null)", wd->icon_name, wd->desk, old_desk );
    if( old_desk != wd->desk )
    {
        forget_desk_client( old_desk, wd );
		if( d != NULL )
		{
	        add_desk_client( d, wd );
  		    LOCAL_DEBUG_OUT( "reparenting client to desk %ld", d->desk );
      		quietly_reparent_canvas( wd->canvas, d->desk_canvas->w, CLIENT_EVENT_MASK, False, None );
		}
    }
    set_client_name( wd, False );
	LOCAL_DEBUG_OUT( "client \"%s\" focused = %d", wd->window_name?wd->window_name:"(null)", wd->focused );
    set_astbar_focused( wd->bar, NULL, wd->focused );
	set_client_look( wd, True );
	LOCAL_DEBUG_OUT( "placing client%s", "" );
	if( d != NULL )
	    place_client( d, wd, False, False );
    LOCAL_DEBUG_OUT( "all done%s", "" );
}


void
change_desk_stacking( int desk, unsigned int clients_num, Window *clients )
{
    ASPagerDesk *d = get_pager_desk( desk );
    int i, real_clients_count = 0 ;
    if( d == NULL )
        return;

    if( d->clients_num < clients_num )
    {
        d->clients = realloc( d->clients, clients_num*sizeof(ASWindowData*));
		memset( d->clients, 0x00, clients_num*sizeof(ASWindowData*) );
        d->clients_num = clients_num ;
    }
    for( i = 0 ; i < clients_num ; ++i )
    {
		ASWindowData *wd = fetch_window_by_id( clients[i] );
		if( wd != NULL ) 
		{/* window is in stacking order, but wew were not notifyed about it yet */
			int k = real_clients_count;
			while( --k >= 0 )
				if( d->clients[k] == wd )
					break ; /* already belongs to that desk */
			if( k < 0  )
			{
      			d->clients[i] = wd ;
				++real_clients_count ;
  		    	LOCAL_DEBUG_OUT( "id(%lX)->wd(%p)", clients[i], d->clients[i] );
			}
		}
    }
	d->clients_num = real_clients_count ;
    set_flags( d->flags, ASP_ShapeDirty);
    restack_desk_windows( d );
}

void
set_desktop_pixmap( int desk, Pixmap pmap )
{
    ASPagerDesk *d = get_pager_desk( desk );
    unsigned int width, height;

    if( !get_drawable_size( pmap, &width, &height ) )
        pmap = None ;
    LOCAL_DEBUG_OUT( "desk(%d)->d(%p)->pmap(%lX)->size(%dx%d)", desk, d, pmap, width, height );
    if( pmap == None  )
        return ;
    if( d == NULL )
    {
        XFreePixmap( dpy, pmap );
        return;
    }

    if( get_flags( d->flags, ASP_UseRootBackground ) )
    {
        ASImage *im = pixmap2asimage (Scr.asv, pmap, 0, 0, width, height, 0xFFFFFFFF, False, 100);

        XFreePixmap( dpy, pmap );
        if( d->back )
            destroy_asimage(&(d->back));
        d->back = im ;
        delete_astbar_tile( d->background, 0 );
        add_astbar_icon( d->background, 0, 0, 0, NO_ALIGN, d->back );
        render_astbar( d->background, d->desk_canvas );
        update_canvas_display( d->desk_canvas );
    }
}

void
move_sticky_clients()
{
    int desk = PagerState.desks_num ;
	ASPagerDesk *current_desk = get_pager_desk( Scr.CurrentDesk ) ;

    while( --desk >= 0 )
    {
        ASPagerDesk *d = &(PagerState.desks[desk]) ;
        if( d->clients && d->clients_num > 0 )
        {
            register int i = d->clients_num ;
            register ASWindowData **clients = d->clients ;
            while( --i >= 0 )
                if( clients[i] && get_flags( clients[i]->state_flags, AS_Sticky))
				{
				    if( clients[i]->desk != Scr.CurrentDesk && current_desk )
					{/* in order to make an illusion of smooth desktop
					  * switching - we'll reparent window ahead of time */
				        LOCAL_DEBUG_OUT( "reparenting client to desk %ld", d->desk );
        				quietly_reparent_canvas( clients[i]->canvas, current_desk->desk_canvas->w, CLIENT_EVENT_MASK, False, None );
    				}
                    place_client( d, clients[i], True, True );
				}
            set_flags(d->flags, ASP_ShapeDirty);
        }
    }
}


void
switch_deskviewport( int new_desk, int new_vx, int new_vy )
{
	Bool view_changed = (new_vx != Scr.Vx || new_vy != Scr.Vy) ;
	Bool desk_changed = (new_desk != Scr.CurrentDesk) ;

	Scr.Vx = new_vx;
    Scr.Vy = new_vy;
	Scr.CurrentDesk = new_desk;

    if( desk_changed && IsValidDesk( new_desk ) )
    {
		ASPagerDesk *new_d = get_pager_desk( new_desk ) ;

        if( PagerState.focused_desk != new_d )
        {
            if( PagerState.focused_desk )
            {
                set_astbar_focused( PagerState.focused_desk->title,      PagerState.focused_desk->desk_canvas, False );
                set_astbar_focused( PagerState.focused_desk->background, PagerState.focused_desk->desk_canvas, False );
                if( is_canvas_dirty(PagerState.focused_desk->desk_canvas) )
                    update_canvas_display(PagerState.focused_desk->desk_canvas);
                PagerState.focused_desk = NULL ;
            }
            if ( new_d )
            {
                set_astbar_focused( new_d->title, new_d->desk_canvas, True );
                set_astbar_focused( new_d->background, new_d->desk_canvas, True );
                if( is_canvas_dirty(new_d->desk_canvas) )
                    update_canvas_display(new_d->desk_canvas);
                PagerState.focused_desk = new_d ;
            }
        }
    }

	if( view_changed || desk_changed )
	{
		move_sticky_clients();
    	place_selection();
		if( new_desk < PagerState.start_desk || new_desk >= PagerState.start_desk+PagerState.desks_num ) 
		{
			int i = 4;
	        while ( --i >= 0 )
  		        XUnmapWindow( dpy, PagerState.selection_bars[i] );

		}
	}
}

static char as_comm_buf[256];

void
move_client_to_desk( ASWindowData* wd, int desk )
{
    sprintf( as_comm_buf, "WindowsDesk %d", desk );
    SendInfo( as_comm_buf, wd->client );
}

ASPagerDesk *
translate_client_pos_main( int x, int y, unsigned int width, unsigned int height, int desk, int *ret_x, int *ret_y )
{
    ASPagerDesk *d = NULL ;
    if( x+width >= PagerState.main_canvas->root_x && y+height >= PagerState.main_canvas->root_y &&
        x < PagerState.main_canvas->width+PagerState.main_canvas->root_x &&
        y < PagerState.main_canvas->height+PagerState.main_canvas->root_y )
    {
        int i = PagerState.desks_num ;
        if( (d=get_pager_desk( desk )) != NULL )
        {
            if( d->desk_canvas->root_x+d->desk_canvas->bw > x+width  ||
				d->desk_canvas->root_x+d->desk_canvas->bw+d->desk_canvas->width  <= x ||
                d->desk_canvas->root_y+d->desk_canvas->bw > y+height ||
				d->desk_canvas->root_y+d->desk_canvas->bw+d->desk_canvas->height <= y )
            {
                d = NULL;
            }
        }

        while( --i >= 0 && d == NULL )
        {
            d = &(PagerState.desks[i]);
            LOCAL_DEBUG_OUT( "checking desk %d: pos(%+d%+d)->desk_geom(%dx%d%+d%+d)", i, x, y, d->desk_canvas->width, d->desk_canvas->height, d->desk_canvas->root_x, d->desk_canvas->root_y );
            if( d->desk_canvas->root_x+d->desk_canvas->bw > x+width  ||
				d->desk_canvas->root_x+d->desk_canvas->bw+d->desk_canvas->width  <= x ||
                d->desk_canvas->root_y+d->desk_canvas->bw > y+height ||
				d->desk_canvas->root_y+d->desk_canvas->bw+d->desk_canvas->height <= y )
            {
                d = NULL;
            }
        }
    }

    if( d )
    {
        x -= d->background->root_x ;
        y -= d->background->root_y ;
        LOCAL_DEBUG_OUT("desk(%ld) pager_vpos(%+d%+d) vscreen_size(%dx%d) desk_size(%dx%d) ", d->desk, x, y, PagerState.vscreen_width, PagerState.vscreen_height, d->background->width, d->background->height );
        if( d->background->width > 0 )
            x = (x*PagerState.vscreen_width)/d->background->width ;
        if( d->background->height > 0 )
            y = (y*PagerState.vscreen_height)/d->background->height ;
        *ret_x = x ;
        *ret_y = y ;
    }else
    {
        *ret_x = x ;
        *ret_y = y ;
    }
    return d;
}

void
translate_client_size( unsigned int width,  unsigned int height, unsigned int *ret_width,  unsigned int *ret_height )
{
    if( PagerState.resize_desk )
    {
        ASPagerDesk *d = PagerState.resize_desk ;
        if( d->background->width > 0 )
        {
            width = (width*PagerState.vscreen_width)/d->background->width ;
        }
        if( d->background->height > 0 )
        {
            height = (height*PagerState.vscreen_height)/d->background->height ;
        }
        *ret_width = width ;
        *ret_height = height ;
    }
}

typedef struct ASPagerMoveResizeReq
{
	FunctionCode func;
	send_signed_data_type func_val[2] ;	
	Window client ;
	Bool pending ;
}ASPagerMoveResizeReq;

ASPagerMoveResizeReq PagerMoveResizeReq = { 0, {0, 0}, None, False};

void exec_moveresize_req( void *data )
{
	ASPagerMoveResizeReq *req = (ASPagerMoveResizeReq *)data ;
	if( req && req->pending ) 
	{
		send_signed_data_type unit_val[2] = {1, 1};
	    SendNumCommand ( req->func, NULL, &(req->func_val[0]), &(unit_val[0]), req->client);
		req->pending = False ;	
	}		 
}	 

void 
schedule_moveresize_req( FunctionCode func, send_signed_data_type val1, send_signed_data_type val2, Window client, Bool immidiate )
{
	if( PagerMoveResizeReq.pending ) 
	{
		if( PagerMoveResizeReq.client != client || PagerMoveResizeReq.func != func )
		{	
			timer_remove_by_data( &PagerMoveResizeReq );
			exec_moveresize_req( &PagerMoveResizeReq );
		}else if( immidiate ) 
		{
	  		timer_remove_by_data( &PagerMoveResizeReq );
			PagerMoveResizeReq.pending	= False;
		}
	}	
	PagerMoveResizeReq.func 	= func ; 
	PagerMoveResizeReq.func_val[0]	= val1 ; 
	PagerMoveResizeReq.func_val[1]	= val2 ; 
	PagerMoveResizeReq.client 	= client ; 
	if( immidiate ) 
		exec_moveresize_req( &PagerMoveResizeReq );
	else if( !PagerMoveResizeReq.pending	) 
	{
		PagerMoveResizeReq.pending	= True;
		timer_new( 40, exec_moveresize_req, &PagerMoveResizeReq ); 
	}	 
}

void
apply_client_move(struct ASMoveResizeData *data)
{
    ASWindowData *wd = fetch_client(AS_WIDGET_WINDOW(data->mr));
    int real_x = 0, real_y = 0;
    ASPagerDesk  *d = translate_client_pos_main( PagerState.main_canvas->root_x+data->curr.x,
                                                 PagerState.main_canvas->root_y+data->curr.y,
                                                 data->curr.width, data->curr.height,
                                                 wd->desk, &real_x, &real_y );
    if( d && d->desk != wd->desk )
    {
        move_client_to_desk( wd, d->desk );
        set_moveresize_aspect( data, PagerState.vscreen_width, d->background->width,
                                     PagerState.vscreen_height, d->background->height,
                                     d->background->root_x, d->background->root_y );
    }
    LOCAL_DEBUG_OUT( "d(%p)->curr(%+d%+d)->real(%+d%+d)", d, data->curr.x, data->curr.y, real_x, real_y);
    schedule_moveresize_req( F_MOVE, real_x, real_y, wd->client, False );
}

void complete_client_move(struct ASMoveResizeData *data, Bool cancelled)
{
    ASWindowData *wd = fetch_client( AS_WIDGET_WINDOW(data->mr));
    int real_x = 0, real_y = 0;
    ASPagerDesk  *d = NULL ;
    MRRectangle *rect = &(data->curr);

    if( cancelled )
        rect = &(data->start);

    d = translate_client_pos_main( rect->x+PagerState.main_canvas->root_x,
                                   rect->y+PagerState.main_canvas->root_y,
                                   rect->width,
                                   rect->height,
                                   wd->desk, &real_x, &real_y );

    if( d && d->desk != wd->desk )
    {
        move_client_to_desk( wd, d->desk );
        set_moveresize_aspect( data, PagerState.vscreen_width, d->background->width,
                                     PagerState.vscreen_height, d->background->height,
                                     d->background->root_x, d->background->root_y );
    }
    LOCAL_DEBUG_OUT( "d(%p)->start(%+d%+d)->curr(%+d%+d)->real(%+d%+d)", d, data->start.x, data->start.y, data->curr.x, data->curr.y, real_x, real_y);
    schedule_moveresize_req( F_MOVE, real_x, real_y, wd->client, True );
    Scr.moveresize_in_progress = NULL ;
}

void apply_client_resize(struct ASMoveResizeData *data)
{
    ASWindowData *wd = fetch_client(AS_WIDGET_WINDOW(data->mr));
    unsigned int real_width=1, real_height = 1;
LOCAL_DEBUG_OUT( "desk(%p)->size(%dx%d)", PagerState.resize_desk, data->curr.width, data->curr.height);
    translate_client_size( data->curr.width, data->curr.height, &real_width, &real_height );
	schedule_moveresize_req( F_RESIZE, real_width, real_height, wd->client, False );
}

void complete_client_resize(struct ASMoveResizeData *data, Bool cancelled)
{
    ASWindowData *wd = fetch_client( AS_WIDGET_WINDOW(data->mr));
    unsigned int real_width=1, real_height = 1;

	if( cancelled )
	{
        LOCAL_DEBUG_OUT( "%dx%d%+d%+d", data->start.x, data->start.y, data->start.width, data->start.height);
        translate_client_size( data->start.width, data->start.height, &real_width, &real_height );
    }else
	{
        LOCAL_DEBUG_OUT( "%dx%d%+d%+d", data->curr.x, data->curr.y, data->curr.width, data->curr.height);
        translate_client_size( data->curr.width, data->curr.height, &real_width, &real_height );
    }
	schedule_moveresize_req( F_RESIZE, real_width, real_height, wd->client, True );
    PagerState.resize_desk = NULL ;
    Scr.moveresize_in_progress = NULL ;
}

ASGrid *make_pager_grid()
{
    ASGrid  *grid ;
    int resist = Scr.Feel.EdgeResistanceMove ;
    int attract = Scr.Feel.EdgeAttractionScreen ;
    int i ;

    grid = safecalloc( 1, sizeof(ASGrid));

    for( i = 0  ; i < PagerState.desks_num ; ++i )
    {
        ASPagerDesk *d = &(PagerState.desks[i]);
        int k = d->clients_num ;
        ASWindowData *wd ;
        ASTBarData *bb = d->background;

        add_gridline( &(grid->h_lines), bb->root_y, bb->root_x, bb->root_x+bb->width, resist, attract );
        add_gridline( &(grid->h_lines), bb->root_y+bb->height, bb->root_x, bb->root_x+bb->width, resist, attract );
        add_gridline( &(grid->v_lines), bb->root_x, bb->root_y, bb->root_y+bb->height, resist, attract );
        add_gridline( &(grid->v_lines), bb->root_x+bb->width, bb->root_y, bb->root_y+bb->height, resist, attract );

        if( !get_flags(d->flags, ASP_DeskShaded ) )
        {   /* add all the grid separation windows : */
            register int p = PagerState.page_columns-1;
            int pos_inc = bb->width/PagerState.page_columns ;
            int pos = bb->root_x+p*pos_inc;
            int size = bb->height ;
            int pos2 = bb->root_y ;
            /* vertical bars : */
            while( --p >= 0 )
            {
                add_gridline( &(grid->v_lines), pos, pos2, pos2+size, resist, attract );
                pos -= pos_inc ;
            }
            /* horizontal bars */
            p = PagerState.page_rows-1;
            pos_inc = bb->height/PagerState.page_rows ;
            pos = bb->root_y + p*pos_inc;
            pos2 = bb->root_x ;
            size = bb->width ;
            while( --p >= 0 )
            {
                add_gridline( &(grid->h_lines), pos, pos2, pos2+size, resist, attract );
                pos -= pos_inc ;
            }
        }
        while( --k >= 0 )
            if( (wd = PagerState.desks[i].clients[k]) != NULL )
            {
                int outer_gravity = Scr.Feel.EdgeAttractionWindow ;
                int inner_gravity = Scr.Feel.EdgeAttractionWindow ;
                if( get_flags(wd->flags, AS_AvoidCover) )
                    inner_gravity = -1 ;

                if( inner_gravity != 0 )
                    add_canvas_grid( grid, wd->canvas, outer_gravity, inner_gravity, 0, 0 );
            }
    }
    /* add all the window edges for this desktop : */
    /* iterate_asbidirlist( Scr.Windows->clients, get_aswindow_grid_iter_func, (void*)&grid_data, NULL, False ); */

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
    print_asgrid( grid );
#endif

    return grid;
}


void
start_moveresize_client( ASWindowData *wd, Bool move, ASEvent *event )
{
    ASMoveResizeData *mvrdata = NULL ;
    ASPagerDesk *d = get_pager_desk( wd->desk );
    MyStyle *pager_focused_style = Scr.Look.MSWindow[BACK_FOCUSED];

    release_pressure();
    if( Scr.moveresize_in_progress )
        return ;


    if( move )
    {
        Scr.Look.MSWindow[BACK_FOCUSED] = mystyle_find_or_default("focused_window_style");
        mvrdata = move_widget_interactively(PagerState.main_canvas,
                                            wd->canvas,
                                            event,
                                            apply_client_move,
                                            complete_client_move );
    }else if( d != NULL )
    {
        if( get_flags( wd->state_flags, AS_Shaded ) )
        {
            XBell (dpy, Scr.screen);
            return;
        }

        Scr.Look.MSWindow[BACK_FOCUSED] = mystyle_find_or_default("focused_window_style");
        PagerState.resize_desk = d ;
        mvrdata = resize_widget_interactively(  d->desk_canvas,
                                                wd->canvas,
                                                event,
                                                apply_client_resize,
                                                complete_client_resize,
                                                FR_SE );
    }
    Scr.Look.MSWindow[BACK_FOCUSED] = pager_focused_style;

    LOCAL_DEBUG_OUT( "mvrdata(%p)", mvrdata );
    if( mvrdata )
    {
        if( d )
            set_moveresize_aspect( mvrdata, PagerState.vscreen_width, d->background->width,
                                            PagerState.vscreen_height, d->background->height,
                                            d->background->root_x, d->background->root_y );

        mvrdata->grid = make_pager_grid();
        Scr.moveresize_in_progress = mvrdata ;
    }
}

/*************************************************************************
 * individuaL Desk manipulation
 *************************************************************************/
/****************************************************************************/
/* PROCESSING OF AFTERSTEP MESSAGES :                                       */
/****************************************************************************/
void
process_message (send_data_type type, send_data_type *body)
{
    LOCAL_DEBUG_OUT( "received message %lX, wait_as_resp = %d", type, PagerState.wait_as_response );
	--PagerState.wait_as_response;
	if( (type&WINDOW_PACKET_MASK) != 0 )
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );
		WindowPacketResult res ;
        /* saving relevant client info since handle_window_packet could destroy the actuall structure */
        Window               saved_w = None ;
        INT32                saved_desk = wd?wd->desk:INVALID_DESK;
		INT32 				 new_desk = saved_desk ; 
        struct ASWindowData *saved_wd = wd ;

        if( wd && wd->canvas )
            saved_w = wd->canvas->w;

/*         show_activity( "message %lX window %X data %p", type, body[0], wd ); */
		res = handle_window_packet( type, body, &wd );
        if( res == WP_DataCreated )
            add_client( wd );
		else if( res == WP_DataChanged )
		{	
            refresh_client( saved_desk, wd );
			new_desk = wd->desk ;
		}else if( res == WP_DataDeleted )
        {
			int i = PagerState.desks_num ;
            LOCAL_DEBUG_OUT( "client deleted (%p)->window(%lX)->desk(%ld)", saved_wd, saved_w, (long)saved_desk );
			/* we really want to make sure that no desk is referencing this client : */
			while( --i  >= 0 )
	            forget_desk_client( i, saved_wd );
            unregister_client( saved_w );
        }
		if( !get_flags( PagerState.flags, ASP_ReceivingWindowList ) )
		{
			Bool need_shape_update = False ;	  
			if( IsValidDesk(saved_desk) )
			{
        		register INT32 pager_desk = saved_desk - PagerState.start_desk ;
			    if(  pager_desk >= 0 && pager_desk < PagerState.desks_num )
				if( Config->MSDeskBack[pager_desk]->texture_type == TEXTURE_SHAPED_PIXMAP ||
    	        	Config->MSDeskBack[pager_desk]->texture_type == TEXTURE_SHAPED_SCALED_PIXMAP )
					need_shape_update = True ;
			}
			if( need_shape_update && saved_desk != new_desk && IsValidDesk(new_desk))
			{	
				register INT32 pager_desk = new_desk - PagerState.start_desk ;
			    if(  pager_desk >= 0 && pager_desk < PagerState.desks_num )
				if( Config->MSDeskBack[pager_desk]->texture_type == TEXTURE_SHAPED_PIXMAP ||
            		Config->MSDeskBack[pager_desk]->texture_type == TEXTURE_SHAPED_SCALED_PIXMAP )
					need_shape_update = True ;
			}
			if( need_shape_update )
				update_pager_shape();
		}
    }else
    {
        switch( type )
        {
            case M_TOGGLE_PAGING :
                break;
            case M_NEW_DESKVIEWPORT :
                {
                    LOCAL_DEBUG_OUT("M_NEW_DESKVIEWPORT(desk = %ld,Vx=%ld,Vy=%ld)", body[2], body[0], body[1]);
                    switch_deskviewport( body[2], body[0], body[1] );
					update_pager_shape();
                }
                break ;
            case M_STACKING_ORDER :
                {
                    LOCAL_DEBUG_OUT("M_STACKING_ORDER(desk=%ld, clients_num=%ld)", body[0], body[1]);
                    change_desk_stacking( body[0], body[1], &(body[2]) );
                }
                break ;
            case M_END_WINDOWLIST :
                clear_flags( PagerState.flags, ASP_ReceivingWindowList );
				update_pager_shape();
                break ;
           default:
                return;
        }
    }

}
/*************************************************************************
 * Event handling :
 *************************************************************************/
void
DispatchEvent (ASEvent * event)
{
	static Bool root_pointer_moved = True ;
    SHOW_EVENT_TRACE(event);

    LOCAL_DEBUG_OUT( "mvrdata(%p)->main_canvas(%p)->widget(%p)", Scr.moveresize_in_progress, PagerState.main_canvas, event->widget );
    if( Scr.moveresize_in_progress )
    {
        event->widget = PagerState.resize_desk?PagerState.resize_desk->desk_canvas:PagerState.main_canvas ;
        if( check_moveresize_event( event ) )
            return;
    }

	if( (event->eclass & ASE_POINTER_EVENTS) != 0 && is_balloon_click( &(event->x) ) )
	{
		withdraw_balloon(NULL);
		return;
	}

    event->client = NULL ;
    event->widget = PagerState.main_canvas ;

    if( event->w != PagerState.main_canvas->w )
    {
        ASWindowData *wd = fetch_client( event->w );

        if( wd )
        {
            if( event->x.type == ButtonPress && event->x.xbutton.button != Button2 )
            {
                event->w = get_pager_desk( wd->desk )->desk_canvas->w ;
            }else
            {
                event->client = (void*)wd;
                event->widget = ((ASWindowData*)(event->client))->canvas ;
                if( (event->eclass & ASE_POINTER_EVENTS) != 0 )
				{	
                    on_astbar_pointer_action( ((ASWindowData*)(event->client))->bar, 0, (event->x.type == LeaveNotify), root_pointer_moved );
					root_pointer_moved = False ;
				}
            }
        }
    }

    switch (event->x.type)
    {
	    case ConfigureNotify:
            if( event->client != NULL )
                on_client_moveresize( (ASWindowData*)event->client );
            else
            {
                on_pager_window_moveresize( event->client, event->w,
                                    event->x.xconfigure.x,
                                    event->x.xconfigure.y,
                                    event->x.xconfigure.width,
                                    event->x.xconfigure.height );
            }
            break;
        case KeyPress :
            if( event->client != NULL )
            {
                ASWindowData *wd = (ASWindowData*)(event->client);
                event->x.xkey.window = wd->client;
                XSendEvent (dpy, wd->client, False, KeyPressMask, &(event->x));
            }
            return ;
        case KeyRelease :
            if( event->client != NULL )
            {
                ASWindowData *wd = (ASWindowData*)(event->client);
                event->x.xkey.window = wd->client;
                XSendEvent (dpy, wd->client, False, KeyReleaseMask, &(event->x));
            }
			return ;
        case ButtonPress:
            on_pager_pressure_changed( event );
            return ;
        case ButtonRelease:
LOCAL_DEBUG_OUT( "state(0x%X)->state&ButtonAnyMask(0x%X)", event->x.xbutton.state, event->x.xbutton.state&ButtonAnyMask );
            if( (event->x.xbutton.state&ButtonAnyMask) == (Button1Mask<<(event->x.xbutton.button-Button1)) )
                release_pressure();
			return ;
        case EnterNotify :
			if( event->x.xcrossing.window == Scr.Root )
				withdraw_active_balloon();
			return ;
        case MotionNotify :
			root_pointer_moved = True ;
            if( (event->x.xbutton.state&Button3Mask) )
			{	
				XEvent        d;
				sleep_a_millisec(10);
				ASSync(False);
				while (ASCheckTypedEvent ( MotionNotify, &d))
				{
					event->x = d ;
					setup_asevent_from_xevent( event );	  
				}
                on_scroll_viewport( event );
				sleep_a_millisec(100);
			}
			return ;
	    case ClientMessage:
            LOCAL_DEBUG_OUT("ClientMessage(\"%s\",format = %d, data=(%8.8lX,%8.8lX,%8.8lX,%8.8lX,%8.8lX)", XGetAtomName( dpy, event->x.xclient.message_type ), event->x.xclient.format, event->x.xclient.data.l[0], event->x.xclient.data.l[1], event->x.xclient.data.l[2], event->x.xclient.data.l[3], event->x.xclient.data.l[4]);
            if ( event->x.xclient.format == 32 &&
                 event->x.xclient.data.l[0] == _XA_WM_DELETE_WINDOW )
			{
                DeadPipe(0);
            }else if( event->x.xclient.format == 32 &&
                      event->x.xclient.message_type == _AS_BACKGROUND && event->x.xclient.data.l[1] != None )
            {
                set_desktop_pixmap( event->x.xclient.data.l[0]-PagerState.start_desk, event->x.xclient.data.l[1] );
            }

			return ;
	    case PropertyNotify:
            if( event->x.xproperty.atom == _XA_NET_WM_STATE )
            {
				LOCAL_DEBUG_OUT( "_XA_NET_WM_STATE updated!%s","");
				return;
			}
			handle_wmprop_event (Scr.wmprops, &(event->x));
            if( event->x.xproperty.atom == _AS_BACKGROUND )
            {
                register int i = PagerState.desks_num ;
                LOCAL_DEBUG_OUT( "root background updated!%s","");
                safe_asimage_destroy( Scr.RootImage );
                Scr.RootImage = NULL ;
                while( --i >= 0 )
                {
                    update_astbar_transparency(PagerState.desks[i].title, PagerState.desks[i].desk_canvas, True);
                    update_astbar_transparency(PagerState.desks[i].background, PagerState.desks[i].desk_canvas, True);
                    render_desk( &(PagerState.desks[i]), False );
                }
            }else if( event->x.xproperty.atom == _AS_STYLE )
			{
                int i = PagerState.desks_num ;
				LOCAL_DEBUG_OUT( "AS Styles updated!%s","");
				mystyle_list_destroy_all(&(Scr.Look.styles_list));
				LoadColorScheme();
				CheckConfigSanity();
				/* now we need to update everything */
				while( --i >= 0 )
                {
					register int k = PagerState.desks[i].clients_num ;
					register ASWindowData **clients = PagerState.desks[i].clients ;
					LOCAL_DEBUG_OUT( "i = %d, clients_num = %d ", i, k );
					while( --k >= 0 )
					{
						LOCAL_DEBUG_OUT( "k = %d", k );
						if( clients[k] )
							set_client_look( clients[k], False );
						else
							show_warning( "client %d of the desk %d is NULL", k, i );
					}
                }
				redecorate_pager_desks();
				rearrange_pager_desks( False );
			}else if( event->x.xproperty.atom == _AS_TBAR_PROPS )
			{
		 		retrieve_pager_astbar_props();		
				redecorate_pager_desks();
				rearrange_pager_desks( False );
            }
			return ;
        default:
#ifdef XSHMIMAGE
			LOCAL_DEBUG_OUT( "XSHMIMAGE> EVENT : completion_type = %d, event->type = %d ", Scr.ShmCompletionEventType, event->x.type );
			if( event->x.type == Scr.ShmCompletionEventType )
				handle_ShmCompletion( event );
#endif /* SHAPE */
            return;
    }
    update_pager_shape();
}

void
request_background_image( ASPagerDesk *d )
{
	if( d->back == NULL || 
		(d->back->width != d->background->width || 
		 d->back->height != d->background->height )  )
    {
        XEvent e;
        e.xclient.type = ClientMessage ;
        e.xclient.message_type = _AS_BACKGROUND ;
        e.xclient.format = 16;
        e.xclient.window = PagerState.main_canvas->w ;
        e.xclient.data.s[0] = d->desk ;
        e.xclient.data.s[1] = 0 ;
        e.xclient.data.s[2] = 0 ;
        e.xclient.data.s[3] = d->background->width ;
        e.xclient.data.s[4] = d->background->height ;
        e.xclient.data.s[5] = 0x01 ;
        e.xclient.data.s[6] = 0 ;
        e.xclient.data.s[7] = 0 ;
        e.xclient.data.s[8] = 0 ;
        LOCAL_DEBUG_OUT( "size(%dx%d)", e.xclient.data.s[3], e.xclient.data.s[4] );
        XSendEvent( dpy, Scr.Root, False, PropertyChangeMask, &e );
        ASSync(False);
    }
}

void
on_desk_moveresize( ASPagerDesk *d )
{
    ASFlagType changes = handle_canvas_config( d->desk_canvas );

    if( get_flags(changes, CANVAS_RESIZED ) )
    {
        place_desk_title( d );
        place_desk_background( d );
        /* place all the grid separation windows : */
		place_separation_bars( d );

        /* rearrange all the client windows : */
        if( d->clients && d->clients_num > 0 )
        {
            register int i ;
            i = d->clients_num ;
            while( --i >= 0 )
                place_client( d, d->clients[i], False, True );
        }
        if( d->desk == Scr.CurrentDesk )
            place_selection();
        if( get_flags( d->flags, ASP_UseRootBackground )  )
			request_background_image( d );
    }
    update_astbar_transparency(d->title, d->desk_canvas, False);
    update_astbar_transparency(d->background, d->desk_canvas, False);

    render_desk( d, (changes!=0) );
}

void on_pager_window_moveresize( void *client, Window w, int x, int y, unsigned int width, unsigned int height )
{
    if( client == NULL )
    {   /* then its one of our canvases !!! */
        int i ;

        if( w == PagerState.main_canvas->w )
        { /* need to rescale everything? maybe: */
            int new_desk_width;
            int new_desk_height = 0;
            int changes;

            changes = update_main_canvas_config();
            if( changes&CANVAS_RESIZED )
            {
                set_flags(PagerState.flags, ASP_ShapeDirty);

                new_desk_width  = calculate_pager_desk_width();
                new_desk_height = calculate_pager_desk_height();

                if( new_desk_width <= 0 )
                    new_desk_width = 1;
                if( new_desk_height <= 0 )
                    new_desk_height = 1;
                LOCAL_DEBUG_OUT( " :RESIZING:  old_desk_size(%dx%d) new_desk_size(%dx%d)", PagerState.desk_width, PagerState.desk_height, new_desk_width, new_desk_height );
                if( new_desk_width != PagerState.desk_width ||
                    new_desk_height != PagerState.desk_height )
                {
                    PagerState.desk_width = new_desk_width ;
                    PagerState.desk_height = new_desk_height ;
                }
                rearrange_pager_desks( True );
            }else if( changes != 0 )
            {
                for( i = 0 ; i < PagerState.desks_num; ++i )
                    on_desk_moveresize( &(PagerState.desks[i]) );
            }
        }else                                  /* then its one of our desk subwindows : */
        {
            for( i = 0 ; i < PagerState.desks_num; ++i )
            {
                if( PagerState.desks[i].desk_canvas->w == w )
                {
                    on_desk_moveresize( &(PagerState.desks[i]) );
                    break;
                }
            }
        }
    }
}

void
on_desk_pressure_changed( ASPagerDesk *d, ASEvent *event )
{
    int root_x = event->x.xbutton.x_root;
    int root_y = event->x.xbutton.y_root;
/*    int state = event->x.xbutton.state ; */
    int context = check_astbar_point( d->title, root_x, root_y );
LOCAL_DEBUG_OUT( "root_pos(%+d%+d)->title_root_pos(%+d%+d)->context(%s)", root_x, root_y, d->title?d->title->root_x:0, d->title?d->title->root_y:0, context2text(context) );
    if( context != C_NO_CONTEXT && d->title )
    {
        set_astbar_btn_pressed (d->title, context);  /* must go before next call to properly redraw :  */
        set_astbar_pressed( d->title, d->desk_canvas, context&C_TITLE );
        PagerState.pressed_bar = d->title ;
        PagerState.pressed_context = context ;
    }else
    {
        set_astbar_pressed( d->background, d->desk_canvas, context&C_TITLE );
        PagerState.pressed_bar = d->background ;
        PagerState.pressed_context = C_ROOT ;
    }
    PagerState.pressed_canvas = d->desk_canvas ;
    PagerState.pressed_desk = d ;
    PagerState.pressed_button = event->x.xbutton.button ;

LOCAL_DEBUG_OUT( "canvas(%p)->bar(%p)->context(%X)", PagerState.pressed_canvas, PagerState.pressed_bar, context );

    if( is_canvas_dirty(d->desk_canvas) )
        update_canvas_display(d->desk_canvas);
}


void
on_pager_pressure_changed( ASEvent *event )
{
    if( event->client == NULL )
    {
        if( event->w == PagerState.main_canvas->w )
        {


        }else
        {
            int i ;
            for( i = 0 ; i < PagerState.desks_num; ++i )
                if( PagerState.desks[i].desk_canvas->w == event->w )
                {
                    on_desk_pressure_changed(&(PagerState.desks[i]), event );
                    break;
                }
        }
    }else
        start_moveresize_client( (ASWindowData*)(event->client), (event->x.xbutton.state&ControlMask)==0, event );
}

void
on_scroll_viewport( ASEvent *event )
{
    ASPagerDesk *d = PagerState.pressed_desk ;
    if( d )
    {
        char command[64];
        int px = 0, py = 0;
        ASQueryPointerRootXY(&px,&py);
        px -= d->desk_canvas->root_x+d->desk_canvas->bw ;
        py -= d->desk_canvas->root_y+d->desk_canvas->bw ;
        if( px > 0 && px < d->desk_canvas->width &&
            py > 0 && py < d->desk_canvas->height )
        {
            px -= d->background->win_x;
            py -= d->background->win_y;
            if( px >= 0 && py >= 0 &&
                px < d->background->width && py < d->background->height )
            {
                int sx = (px*PagerState.vscreen_width)/d->background->width ;
                int sy = (py*PagerState.vscreen_height)/d->background->height ;
				
                sprintf (command, "GotoDeskViewport %ld%+d%+d\n", d->desk, sx, sy);
                SendInfo ( command, 0);
				++PagerState.wait_as_response ;
            }
        }
    }
}

void
release_pressure()
{
    if( PagerState.pressed_canvas && PagerState.pressed_bar )
    {
LOCAL_DEBUG_OUT( "canvas(%p)->bar(%p)->context(%s)", PagerState.pressed_canvas, PagerState.pressed_bar, context2text(PagerState.pressed_context) );
LOCAL_DEBUG_OUT( "main_geometry(%dx%d%+d%+d)", PagerState.main_canvas->width, PagerState.main_canvas->height, PagerState.main_canvas->root_x, PagerState.main_canvas->root_y );
        if( PagerState.pressed_desk )
        {
            ASPagerDesk *d = PagerState.pressed_desk ;
            if( PagerState.pressed_context == C_TButton0 )
            {
                if( get_flags( Config->flags, VERTICAL_LABEL ) )
                    shade_desk_column( d, !get_flags( d->flags, ASP_DeskShaded) );
                else
                    shade_desk_row( d, !get_flags( d->flags, ASP_DeskShaded) );
            }else                              /* need to switch desktops here ! */
            {
                char command[64];
                int px = 0, py = 0;
                ASQueryPointerRootXY(&px,&py);
LOCAL_DEBUG_OUT( "pointer root pos(%+d%+d)", px, py );
                px -= d->desk_canvas->root_x+d->desk_canvas->bw ;
                py -= d->desk_canvas->root_y+d->desk_canvas->bw ;
                if( px > 0 && px < d->desk_canvas->width &&
                    py > 0 && py < d->desk_canvas->height )
                {
					int new_desk, new_vx = -1, new_vy = -1 ;
					new_desk = d->desk ;
                    px -= d->background->win_x;
                    py -= d->background->win_y;
                    if( px >= 0 && py >= 0 &&
                        px < d->background->width && py < d->background->height )
                    {
                        if( PagerState.pressed_button == Button3 )
                        {
                            new_vx = (px*PagerState.vscreen_width)/d->background->width ;
                            new_vy = (py*PagerState.vscreen_height)/d->background->height ;
                        }else
                        {   /*  calculate destination page : */
                            new_vx  = (px*PagerState.page_columns)/d->background->width ;
                            new_vy  = (py*PagerState.page_rows)   /d->background->height ;
							new_vx *= Scr.MyDisplayWidth ;
							new_vy *= Scr.MyDisplayHeight ;
                        }
                    }
					if( new_vx >= 0 && new_vy >= 0 )
						sprintf (command, "GotoDeskViewport %d%+d%+d\n", new_desk, new_vx, new_vy);
					else
                    	sprintf (command, "Desk 0 %d\n", new_desk);
                    SendInfo (command, 0);
					++PagerState.wait_as_response ;
                }
            }
        }
        set_astbar_btn_pressed (PagerState.pressed_bar, 0);  /* must go before next call to properly redraw :  */
        set_astbar_pressed( PagerState.pressed_bar, PagerState.pressed_canvas, False );
        if( is_canvas_dirty(PagerState.pressed_canvas) )
            update_canvas_display(PagerState.pressed_canvas);
    }
    PagerState.pressed_canvas = NULL ;
    PagerState.pressed_bar = NULL ;
    PagerState.pressed_desk = NULL ;
    PagerState.pressed_context = C_NO_CONTEXT ;
}

/*****************************************************************************
 * Grab the pointer and keyboard
 ****************************************************************************/
static ScreenInfo *grabbed_screen = NULL;

Bool
GrabEm (ScreenInfo *scr, Cursor cursor)
{
	int           i = 0;
	unsigned int  mask;
    int res ;

	XSync (dpy, 0);
    /* move the keyboard focus prior to grabbing the pointer to
	 * eliminate the enterNotify and exitNotify events that go
	 * to the windows */
	grabbed_screen = scr ;

    mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
           PointerMotionMask | EnterWindowMask | LeaveWindowMask ;
    while ( (res = XGrabPointer (dpy, PagerState.main_canvas->w, True, mask, GrabModeAsync,
                                 GrabModeAsync, scr->Root, cursor,
                                 CurrentTime)) != GrabSuccess )
	{
        if( i++ >= 1000 )
        {
#define MAX_GRAB_ERROR 4
            static char *_as_grab_error_code[MAX_GRAB_ERROR+1+1] =
            {
                "Grab Success",
                "pointer is actively grabbed by some other client",
                "the specified time is earlier than the last-pointer-grab time or later than the current X server time",
                "window is not viewable or lies completely outside the boundaries of the root window",
                "pointer is frozen by an active grab of another client",
                "I'm totally messed up - restart me please"
            };
            char *error_text = _as_grab_error_code[MAX_GRAB_ERROR+1];
            if( res <= MAX_GRAB_ERROR )
                error_text = _as_grab_error_code[res];

            show_warning( "Failed to grab pointer for requested interactive operation.(X server says:\"%s\")", error_text );
            return False;
        }
		/* If you go too fast, other windows may not get a change to release
		 * any grab that they have. */
        sleep_a_millisec (100);
        XSync (dpy, 0);
    }
	return True;
}

/*****************************************************************************
 * UnGrab the pointer and keyboard
 ****************************************************************************/
void
UngrabEm ()
{
    if( grabbed_screen )  /* check if we grabbed everything */
    {
        XSync (dpy, 0);
        XUngrabPointer (dpy, CurrentTime);
        XSync (dpy, 0);
		grabbed_screen = NULL;
    }
}



