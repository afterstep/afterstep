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

#include "../../include/asapp.h"
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#define IN_MODULE
#define MODULE_X_INTERFACE

#include "../../include/afterstep.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/parse.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"
#include "../../include/mystyle.h"
#include "../../include/mystyle_property.h"
#include "../../include/balloon.h"
#include "../../include/aswindata.h"
#include "../../include/decor.h"
#include "../../include/event.h"
#include "../../include/wmprops.h"
#include "../../include/moveresize.h"

typedef struct ASPagerDesk {

#define ASP_DeskShaded          (0x01<<0)
#define ASP_UseRootBackground   (0x01<<1)

    ASFlagType flags ;
    int desk ;
    ASCanvas   *desk_canvas;
    ASTBarData *title;
    ASTBarData *background;
    Window     *separator_bars;                /* (rows-1)*(columns-1) */
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
    int start_desk, desks_num ;

    int page_rows,  page_columns ;
    int desk_width, desk_height ; /* x and y size of desktop */
    int vscreen_width, vscreen_height ; /* x and y size of desktop */
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
}ASPagerState;

ASPagerState PagerState;
#define DEFAULT_BORDER_COLOR 0xFF808080

#define PAGE_MOVE_THRESHOLD     15   /* precent */

#define CLIENT_EVENT_MASK   StructureNotifyMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask

/* Storing window list as hash table hashed by client window ID :     */
ASHashTable *PagerClients = NULL;

PagerConfig *Config = NULL;

void
pager_usage (void)
{
	printf ("Usage:\n"
			"%s [--version] [--help] n m\n"
			"%*s where desktops n through m are displayed\n", MyName, (int)strlen (MyName), MyName);
	exit (0);
}

void HandleEvents();
void process_message (unsigned long type, unsigned long *body);
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


/***********************************************************************
 *   main - start of module
 ***********************************************************************/
int
main (int argc, char **argv)
{
    int i;
    char *cptr = NULL ;
//    char *global_config_file = NULL;
    int desk1 = 0, desk2 = 0;

    /* Save our program name - for error messages */
    InitMyApp (CLASS_PAGER, argc, argv, pager_usage, NULL, 0 );

    memset( &PagerState, 0x00, sizeof(PagerState));

    for( i = 1 ; i< argc && argv[i] == NULL ; ++i);
    if( i < argc )
    {
        cptr = argv[i];
        while (isspace (*cptr))
            cptr++;
        desk1 = atoi (cptr);
        while (!(isspace (*cptr)) && (*cptr))
            cptr++;
        while (isspace (*cptr))
            cptr++;

        if (!(*cptr) && i+1 < argc )
            cptr = argv[i + 1];
    }
	if (cptr)
        desk2 = atoi (cptr);
	else
        desk2 = desk1;

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

    ConnectX( &Scr, PropertyChangeMask );

    if (get_flags( MyArgs.flags, ASS_Debugging))
        set_synchronous_mode(True);
    XSync (dpy, 0);

    ConnectAfterStep (  M_ADD_WINDOW |
                        M_CONFIGURE_WINDOW |
                        M_DESTROY_WINDOW |
                        M_FOCUS_CHANGE |
                        M_NEW_PAGE |
                        M_NEW_DESK |
                        M_NEW_BACKGROUND |
                        M_WINDOW_NAME |
                        M_ICON_NAME |
                        M_END_WINDOWLIST|
                        M_STACKING_ORDER);

    Config = CreatePagerConfig (PagerState.desks_num);

    LOCAL_DEBUG_OUT("parsing Options ...%s","");
    LoadBaseConfig (GetBaseOptions);
    LoadConfig ("pager", GetOptions);

    CheckConfigSanity();

    /* Create a list of all windows */
    /* Request a list of all windows,
     * wait for ConfigureWindow packets */
    SendInfo ("Send_WindowList", 0);

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
        if (PagerState.wait_as_response > 0)
        {
            ASMessage *msg = CheckASMessage (WAIT_AS_RESPONSE_TIMEOUT);
            if (msg)
            {
                process_message (msg->header[1], msg->body);
                DestroyASMessage (msg);
                --PagerState.wait_as_response;
            }
        }else
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
}
void
DeadPipe (int nonsense)
{
    if( PagerState.main_canvas )
        destroy_ascanvas( &PagerState.main_canvas );

    if( Config )
        DestroyPagerConfig (Config);

    FreeMyAppResources();
#ifdef DEBUG_ALLOCS
    print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

    XFlush (dpy);			/* need this for SetErootPixmap to take effect */
	XCloseDisplay (dpy);		/* need this for SetErootPixmap to take effect */
    exit (0);
}


/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/
void
CheckConfigSanity()
{
    int i;
    char buf[256];

    if( Config == NULL )
        Config = CreatePagerConfig (PagerState.desks_num);
    if( Config->rows == 0 )
        Config->rows = 1;

    if( Config->columns == 0 ||
        Config->rows*Config->columns != PagerState.desks_num )
        Config->columns = PagerState.desks_num/Config->rows;

    if( Config->rows*Config->columns < PagerState.desks_num )
        ++(Config->columns);

    Config->gravity = NorthWestGravity ;
    if( get_flags(Config->geometry.flags, XNegative) )
        Config->gravity = get_flags(Config->geometry.flags, YNegative)? SouthEastGravity:NorthEastGravity;
    else if( get_flags(Config->geometry.flags, YNegative) )
        Config->gravity = SouthWestGravity;


    if (get_flags(Config->geometry.flags, WidthValue) && Config->geometry.width > Config->columns )
       PagerState.desk_width = Config->geometry.width/Config->columns ;

    Config->geometry.width = PagerState.desk_width*Config->columns ;

    if (get_flags(Config->geometry.flags, HeightValue) && Config->geometry.height > Config->rows )
        PagerState.desk_height = Config->geometry.height/Config->rows ;

    Config->geometry.height = PagerState.desk_height*Config->rows ;

    if( !get_flags(Config->geometry.flags, XValue))
    {
        Config->geometry.x = 0;
    }else
    {
        int real_x = Config->geometry.x ;
        if( get_flags(Config->geometry.flags, XNegative) )
        {
            Config->gravity = NorthEastGravity ;
            real_x += Scr.MyDisplayWidth ;
        }
        if( real_x + Config->geometry.width  < 0 )
            Config->geometry.x = get_flags(Config->geometry.flags, XNegative)?
                                    Config->geometry.width-Scr.MyDisplayWidth : 0 ;
        else if( real_x > Scr.MyDisplayWidth )
            Config->geometry.x = get_flags(Config->geometry.flags, XNegative)?
                                    0 : Scr.MyDisplayWidth-Config->geometry.width ;
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
            real_y += Scr.MyDisplayHeight ;
        }
        if( real_y + Config->geometry.height  < 0 )
            Config->geometry.y = get_flags(Config->geometry.flags, YNegative)?
                                    Config->geometry.height-Scr.MyDisplayHeight : 0 ;
        else if( real_y > Scr.MyDisplayHeight )
            Config->geometry.y = get_flags(Config->geometry.flags, YNegative)?
                                    0 : Scr.MyDisplayHeight-Config->geometry.height ;
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

    mystyle_get_property (Scr.wmprops);

    for( i = 0 ; i < BACK_STYLES ; ++i )
    {
        static char *window_style_names[BACK_STYLES] ={"*%sFWindowStyle", "*%sSWindowStyle", "*%sUWindowStyle" };
        static char *default_window_style_name[BACK_STYLES] ={"focused_window_style","sticky_window_style","unfocused_window_style"};

        sprintf( buf, window_style_names[i], MyName );
        if( (Scr.Look.MSWindow[i] = mystyle_find( buf )) == NULL )
            Scr.Look.MSWindow[i] = mystyle_find_or_default( default_window_style_name[i] );
    }

    if( Config->small_font_name )
    {
        MyFont small_font ;
        if( load_font (Config->small_font_name, &small_font) )
        {
            mystyle_merge_font( Scr.Look.MSWindow[0], &small_font, True, False );
            for( i = 1 ; i < BACK_STYLES ; ++i )
                if( Scr.Look.MSWindow[i] )
                    mystyle_merge_font( Scr.Look.MSWindow[i], &small_font, True, True );
        }
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
            sprintf( buf, "*%sDesk%d", MyName, i + PagerState.start_desk);
            Config->MSDeskBack[i] = mystyle_find_or_default( buf );
        }
    }
    /* shade button : */
    if( Config->shade_btn )
    {
        free_button_resources( Config->shade_btn );
        if( Config->shade_button[0] == NULL )
        {
            free( Config->shade_btn );
            Config->shade_btn = NULL ;
        }
    }
    if( Config->shade_button[0] )
    {
        if( Config->shade_btn == NULL )
            Config->shade_btn = safecalloc( 1, sizeof(button_t));
        if( !load_button(Config->shade_btn, Config->shade_button, Scr.image_manager ) )
        {
            free( Config->shade_btn );
            Config->shade_btn = NULL;
        }
    }

    Scr.Feel.EdgeResistanceMove = 5;
    Scr.Feel.EdgeAttractionScreen = 5;
    Scr.Feel.EdgeAttractionWindow  = 10;
    Scr.Feel.no_snaping_mod = ShiftMask ;
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

    config->gravity = NorthWestGravity ;
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
                set_string_value( &(Config->labels[i]), config->labels[i], NULL, 0 );
    }
    if( config->styles )
    {
        if( Config->styles == NULL )
            Config->styles = safecalloc( PagerState.desks_num, sizeof(char*));
        for( i = 0 ; i < PagerState.desks_num ; ++i )
            if( config->styles[i] )
                set_string_value( &(Config->styles[i]), config->styles[i], NULL, 0 );
    }
    if( get_flags( config->set_flags, PAGER_SET_ALIGN ) )
        Config->align = config->align ;

    if( get_flags( config->set_flags, PAGER_SET_SMALL_FONT ) )
        set_string_value( &(Config->small_font_name), config->small_font_name, NULL, 0 );

    if( get_flags( config->set_flags, PAGER_SET_BORDER_WIDTH ) )
        Config->border_width = config->border_width;

    if( get_flags( config->set_flags, PAGER_SET_SELECTION_COLOR ) )
        parse_argb_color( config->selection_color, &(Config->selection_color_argb) );

    if( get_flags( config->set_flags, PAGER_SET_GRID_COLOR ) )
        parse_argb_color( config->grid_color, &(Config->grid_color_argb) );

    if( get_flags( config->set_flags, PAGER_SET_BORDER_COLOR ) )
        parse_argb_color( config->border_color, &(Config->border_color_argb) );

    if( config->shade_button[0] )
        set_string_value( &(Config->shade_button[0]), config->shade_button[0], NULL, 0 );

    if( config->shade_button[1] )
        set_string_value( &(Config->shade_button[1]), config->shade_button[1], NULL, 0 );
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
    BaseConfig *config = ParseBaseOptions (filename, MyName);

    if (!config)
        exit (0);           /* something terrible has happend */

    if( Scr.image_manager )
        destroy_image_manager( Scr.image_manager, False );
    Scr.image_manager = create_image_manager( NULL, 2.2, config->pixmap_path, config->icon_path, NULL );

    if (config->desktop_size.flags & WidthValue)
        PagerState.page_columns = config->desktop_size.width;
    if (config->desktop_size.flags & HeightValue)
        PagerState.page_rows = config->desktop_size.height;

    Scr.VScale = config->desktop_scale;

    DestroyBaseConfig (config);

    Scr.Vx = 0;
    Scr.Vy = 0;

    Scr.VxMax = (PagerState.page_columns - 1) * Scr.MyDisplayWidth;
    Scr.VyMax = (PagerState.page_rows - 1) * Scr.MyDisplayHeight;
    PagerState.vscreen_width = Scr.VxMax + Scr.MyDisplayWidth;
    PagerState.vscreen_height = Scr.VyMax + Scr.MyDisplayHeight;
    PagerState.desk_width = PagerState.vscreen_width/Scr.VScale;
    PagerState.desk_height = PagerState.vscreen_height/Scr.VScale;

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
    w = create_visual_window( Scr.asv, Scr.Root, x, y, width, height, 4, InputOutput, CWEventMask, &attr);
    set_client_names( w, MyName, MyName, CLASS_PAGER, MyName );

    Scr.RootClipArea.x = x;
    Scr.RootClipArea.y = y;
    Scr.RootClipArea.width = width;
    Scr.RootClipArea.height = height;

    shints.flags = USSize|PMinSize|PResizeInc|PWinGravity;
    if( get_flags( Config->set_flags, PAGER_SET_GEOMETRY ) )
        shints.flags |= USPosition ;
    else
        shints.flags |= PPosition ;

    shints.min_width = Config->columns;
    shints.min_height = Config->rows;
    shints.width_inc = Config->columns;
    shints.height_inc = Config->rows;
	shints.win_gravity = Config->gravity ;

	extwm_hints.pid = getpid();
	extwm_hints.flags = EXTWM_PID|EXTWM_StateSkipTaskbar|EXTWM_StateSkipPager|EXTWM_TypeMenu ;

	set_client_hints( w, NULL, &shints, AS_DoesWmDeleteWindow, &extwm_hints );

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
    {
        Scr.RootClipArea.x = PagerState.main_canvas->root_x;
        Scr.RootClipArea.y = PagerState.main_canvas->root_y;
        Scr.RootClipArea.width  = PagerState.main_canvas->width;
        Scr.RootClipArea.height = PagerState.main_canvas->height;
        if( Scr.RootImage )
        {
            safe_asimage_destroy( Scr.RootImage );
            Scr.RootImage = NULL ;
        }
    }
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
            if( get_flags(Config->flags, LABEL_BELOW_DESK) )
                x = PagerState.desk_width ;
            height = PagerState.desk_height ;
        }else
        {
            if( get_flags(Config->flags, LABEL_BELOW_DESK) )
                y = PagerState.desk_height ;
            width = PagerState.desk_width ;
        }
        move_astbar( d->title, d->desk_canvas, x, y);
        set_astbar_size( d->title, width, height );
    }
}

static void
place_desk_background( ASPagerDesk *d )
{
    int x = 0, y = 0;
    if( !get_flags(Config->flags, LABEL_BELOW_DESK) )
    {
        if( get_flags(Config->flags, VERTICAL_LABEL) )
            x = d->title_width ;
        else
            y = d->title_height ;
    }

    move_astbar( d->background, d->desk_canvas, x, y);
    set_astbar_size( d->background, PagerState.desk_width, PagerState.desk_height );
}

static void
render_desk( ASPagerDesk *d, Bool force )
{
    if( is_canvas_needs_redraw( d->desk_canvas) )
        force = True;

    if( d->title )
        if( force || DoesBarNeedsRendering(d->title) )
            render_astbar( d->title, d->desk_canvas );
    if( force || DoesBarNeedsRendering(d->background) )
        render_astbar( d->background, d->desk_canvas );

    if( is_canvas_dirty( d->desk_canvas) )
        update_canvas_display( d->desk_canvas );
}

unsigned int
calculate_desk_width( ASPagerDesk *d )
{
    unsigned int width = PagerState.desk_width;

    if( get_flags( Config->flags, VERTICAL_LABEL ) )
    {
        if( get_flags( d->flags, ASP_DeskShaded ) )
            width = d->title_width;
        else
            width += d->title_width;
    }
    return width;
}

unsigned int
calculate_desk_height( ASPagerDesk *d )
{
    unsigned int height = PagerState.desk_height;

    if( !get_flags( Config->flags, VERTICAL_LABEL ) )
    {
        if( get_flags( d->flags, ASP_DeskShaded ) )
            height = d->title_height;
        else
            height += d->title_height;
    }
    return height;
}

void place_desk( ASPagerDesk *d, int x, int y, unsigned int width, unsigned int height )
{
    int           win_x = 0, win_y = 0;
    get_canvas_position( d->desk_canvas, NULL, &win_x, &win_y );

    if( d->desk_canvas->width == width && d->desk_canvas->height == height && win_x == x && win_y == y )
    {
        on_desk_moveresize( d );
    }else
        moveresize_canvas( d->desk_canvas, x, y, width, height );
}

inline ASPagerDesk *
get_pager_desk( int desk )
{
    register int pager_desk = desk - PagerState.start_desk ;
    if(  pager_desk >= 0 && pager_desk < PagerState.desks_num )
        return &(PagerState.desks[pager_desk]);
    return NULL ;
}

void
restack_desk_windows( ASPagerDesk *d )
{
    Window *list, *curr ;
    int win_count = 0 ;
    int i ;
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

    if( get_flags(Config->flags, SHOW_SELECTION) && d->desk == Scr.CurrentDesk )
    {
        for( i = 0 ; i < 4 ; ++i )
            curr[i] = PagerState.selection_bars[i] ;
        curr += 4 ;
    }
    if( get_flags(Config->flags, PAGE_SEPARATOR) )
    {
        register Window *sbars = d->separator_bars;
        i = d->separator_bars_num ;
        while( --i >= 0 )
            curr[i] = sbars[i] ;
        curr += d->separator_bars_num ;
    }

    if( d->clients_num > 0 )
    {
        register ASWindowData **clients = d->clients ;
        i = d->clients_num ;
        while( --i >= 0 )
        {
            if( clients[i] )
                curr[i] = clients[i]->canvas->w ;
            else
            {
                --curr ;
                --win_count ;
            }
        }
    }

    XRaiseWindow( dpy, list[0] );
    XRestackWindows( dpy, list, win_count );
}


void
place_selection()
{
     ASPagerDesk *sel_desk ;

LOCAL_DEBUG_CALLER_OUT( "Scr.CurrentDesk(%d)->start_desk(%d)", Scr.CurrentDesk, PagerState.start_desk );
    if( get_flags(Config->flags, SHOW_SELECTION) && (sel_desk = get_pager_desk( Scr.CurrentDesk ))!= NULL )
    {
        int sel_x = sel_desk->background->win_x ;
        int sel_y = sel_desk->background->win_y ;
        int page_width = sel_desk->background->width/PagerState.page_columns ;
        int page_height = sel_desk->background->height/PagerState.page_rows ;

        sel_x += (Scr.Vx*page_width)/Scr.MyDisplayWidth ;
        sel_y += (Scr.Vy*page_height)/Scr.MyDisplayHeight ;
LOCAL_DEBUG_OUT( "sel_pos(%+d%+d)->page_size(%dx%d)->desk(%d)", sel_x, sel_y, page_width, page_height, sel_desk->desk );
        XReparentWindow( dpy, PagerState.selection_bars[0], sel_desk->desk_canvas->w, -10, -10 );
        XReparentWindow( dpy, PagerState.selection_bars[1], sel_desk->desk_canvas->w, -10, -10 );
        XReparentWindow( dpy, PagerState.selection_bars[2], sel_desk->desk_canvas->w, -10, -10 );
        XReparentWindow( dpy, PagerState.selection_bars[3], sel_desk->desk_canvas->w, -10, -10 );

        if( !get_flags( sel_desk->flags, ASP_DeskShaded ) )
        {
            XMoveResizeWindow( dpy, PagerState.selection_bars[0], sel_x-1, sel_y-1, page_width+2, 1 );
            XMoveResizeWindow( dpy, PagerState.selection_bars[1], sel_x-1, sel_y-1, 1, page_height+2 );
            XMoveResizeWindow( dpy, PagerState.selection_bars[2], sel_x-1, sel_y+page_height+1, page_width+2, 1 );
            XMoveResizeWindow( dpy, PagerState.selection_bars[3], sel_x+page_width+1, sel_y-1, 1, page_height+2 );
        }

        restack_desk_windows( sel_desk );
        XMapSubwindows( dpy, sel_desk->desk_canvas->w );
    }
}

void
redecorate_pager_desks()
{
    /* need to create enough desktop canvases */
    int i ;
    char buf[256];
    XSetWindowAttributes attr;
    for( i = 0 ; i < PagerState.desks_num ; ++i )
    {
        ASPagerDesk *d = &(PagerState.desks[i]);
        int p ;

        if( d->desk_canvas == NULL )
        {
            Window w;
            attr.event_mask = StructureNotifyMask|ButtonReleaseMask|ButtonPressMask|ButtonMotionMask ;
            ARGB2PIXEL(Scr.asv,Config->border_color_argb,&(attr.border_pixel));

            w = create_visual_window(Scr.asv, PagerState.main_canvas->w, 0, 0, PagerState.desk_width, PagerState.desk_height,
                                     Config->border_width, InputOutput, CWEventMask|CWBorderPixel, &attr );
            d->desk_canvas = create_ascanvas( w );
            LOCAL_DEBUG_OUT("+CREAT canvas(%p)->desk(%d)->geom(%dx%d%+d%+d)->parent(%lx)", d->desk_canvas, PagerState.start_desk+i, PagerState.desk_width, PagerState.desk_height, 0, 0, PagerState.main_canvas->w );
            //enable_rendering = False ;
            handle_canvas_config( d->desk_canvas );
        }
        /* create & moveresize label bar : */
        if( get_flags( Config->flags, USE_LABEL ) )
        {
            int align = (Config->align>0)?ALIGN_LEFT:((Config->align<0)?ALIGN_RIGHT:ALIGN_HCENTER) ;
            int flip = get_flags(Config->flags, VERTICAL_LABEL)?FLIP_VERTICAL:0;
            if( d->title == NULL )
            {
                d->title = create_astbar();
                d->title->context = C_TITLE ;
            }
            set_astbar_hilite( d->title, NORMAL_HILITE|NO_HILITE_OUTLINE);
            set_astbar_style_ptr( d->title, BAR_STATE_FOCUSED, Config->MSDeskTitle[DESK_ACTIVE] );
            set_astbar_style_ptr( d->title, BAR_STATE_UNFOCUSED, Config->MSDeskTitle[DESK_INACTIVE] );
            /* delete label if it was previously created : */
            delete_astbar_tile( d->title, -1 );
            if( Config->labels && Config->labels[i] )
                add_astbar_label( d->title, 0, flip?1:0, flip, align, Config->labels[i] );
            else
            {
                sprintf( buf, "Desk %d", PagerState.start_desk+i );
                add_astbar_label( d->title, 0, flip?1:0, flip, align, buf );
            }
            if( Config->shade_btn )
                add_astbar_btnblock( d->title, flip?0:1, 0, flip, NO_ALIGN, Config->shade_btn, 0xFFFFFFFF, 1, 1, 1, 0, 0,C_R1);
            if( get_flags( Config->flags, VERTICAL_LABEL ) )
            {
                d->title_width = calculate_astbar_width( d->title );
                d->title_height = PagerState.desk_height;
            }else
            {
                d->title_width = PagerState.desk_width;
                d->title_height = calculate_astbar_height( d->title );
            }
        }else
        {
            if( d->title )
                destroy_astbar( &(d->title) );
            d->title_width = d->title_height = 0 ;
        }

        /* create & moveresize desktop background bar : */
        if( d->background == NULL )
            d->background = create_astbar();
        set_astbar_style_ptr( d->background, BAR_STATE_FOCUSED, Config->MSDeskBack[i] );
        set_astbar_style_ptr( d->background, BAR_STATE_UNFOCUSED, Config->MSDeskBack[i] );
        if( Config->styles[i] == NULL )
            set_flags( d->flags, ASP_UseRootBackground );

        if( d->separator_bars )
        {
            for( p = 0 ; p < d->separator_bars_num ; ++p )
                if( d->separator_bars[p] )
                    XDestroyWindow( dpy, d->separator_bars[p] );
            free( d->separator_bars );
            d->separator_bars_num = 0 ;
            d->separator_bars = NULL ;
        }
        if( get_flags( Config->flags, PAGE_SEPARATOR ) )
        {
            d->separator_bars_num = PagerState.page_columns-1+PagerState.page_rows-1 ;
            d->separator_bars = safecalloc( d->separator_bars_num, sizeof(Window));
            ARGB2PIXEL(Scr.asv,Config->grid_color_argb,&(attr.background_pixel));
            for( p = 0 ; p < d->separator_bars_num ; ++p )
                d->separator_bars[p] = create_visual_window(Scr.asv, d->desk_canvas->w, 0, 0, 1, 1,
                                                             0, InputOutput, CWBackPixel, &attr );
            XMapSubwindows( dpy, d->desk_canvas->w );
        }
    }
    /* selection bars : */
    ARGB2PIXEL(Scr.asv,Config->selection_color_argb,&(attr.background_pixel));
    if(get_flags( Config->flags, SHOW_SELECTION ) )
    {
        for( i = 0 ; i < 4 ; i++ )
            if( PagerState.selection_bars[i] == None )
                PagerState.selection_bars[i] = create_visual_window( Scr.asv, PagerState.main_canvas->w, 0, 0, 1, 1,
                                                                     0, InputOutput, CWBackPixel, &attr );
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
                row_height = height+Config->border_width;

            if( ++col >= Config->columns )
            {
                if( all_width < x+Config->border_width+width+Config->border_width )
                    all_width = x+Config->border_width+width+Config->border_width ;
                y += row_height;

                row_height = 0 ;
                x = 0 ;
                col = 0;
            }else
                x += width+Config->border_width;
        }
        if( all_height < y+row_height+Config->border_width )
            all_height = y+row_height+Config->border_width;
        if( PagerState.main_canvas->width != all_width || PagerState.main_canvas->height != all_height )
        {
            resize_canvas( PagerState.main_canvas, all_width, all_height );
            return;
        }
    }
    /* Pass 2: now we can resize the rest of the windows : */
    col = 0;
    x = y = row_height = 0;
    for( i = 0 ; i < PagerState.desks_num ; ++i )
    {
        ASPagerDesk *d = &(PagerState.desks[i]);
        int width, height;

        width = calculate_desk_width( d );
        height = calculate_desk_height( d );

        place_desk( d, x, y, width, height );

        if( height > row_height )
            row_height = height+Config->border_width;

        if( ++col >= Config->columns )
        {
            y += row_height;
            row_height = 0 ;
            x = 0 ;
            col = 0;
        }else
            x += width+Config->border_width;
    }
    ASSync(False);
}

unsigned int
calculate_pager_desk_width()
{
    int main_width = PagerState.main_canvas->width - (Config->columns+1)*Config->border_width;
    if( !get_flags( Config->flags, VERTICAL_LABEL ) )
        return main_width/Config->columns;
    else
    {
        unsigned int unshaded_col_count = 0 ;
        unsigned int label_width = 0 ;
        int col = 0 ;
        /* we have to calculate the number of not-shaded columns,
        * and then devide size of the main canvas by this number : */
        for( col = 0 ; col < Config->columns ; ++col )
        {
            Bool unshaded = False ;
            unsigned int col_label_width = 0 ;
            int i ;

            for( i = col ; i < PagerState.desks_num ; i+= Config->columns )
            {
                ASPagerDesk *d = &(PagerState.desks[i]);
                if( col_label_width < d->title_width )
                    col_label_width = d->title_width ;
                if( !get_flags(d->flags, ASP_DeskShaded )  )
                    unshaded = True ;
            }
            if( unshaded )
                ++unshaded_col_count ;
            label_width += col_label_width ;
        }

LOCAL_DEBUG_OUT( "unshaded_col_count = %d", unshaded_col_count );
        if( unshaded_col_count == 0 )
            return PagerState.desk_width;
        return (main_width-label_width)/unshaded_col_count;
    }
}

unsigned int
calculate_pager_desk_height()
{
    int main_height = PagerState.main_canvas->height - (Config->rows+1)*Config->border_width ;
    if( get_flags( Config->flags, VERTICAL_LABEL ) )
        return main_height/Config->rows;
    else
    {
        unsigned int unshaded_row_count = 0 ;
        unsigned int label_height = 0 ;
        int row ;
        /* we have to calculate the number of not-shaded columns,
        * and then devide size of the main canvas by this number : */
        for( row = 0 ; row < Config->rows ; ++row )
        {
            Bool unshaded = False ;
            unsigned int row_label_height = 0 ;
            int i ;

            for( i = row*Config->columns ; i < PagerState.desks_num ; ++i  )
            {
                ASPagerDesk *d = &(PagerState.desks[i]);
                if( row_label_height < d->title_height )
                    row_label_height = d->title_height ;
                if( !get_flags(d->flags, ASP_DeskShaded )  )
                    unshaded = True ;
            }
            if( unshaded )
                ++unshaded_row_count ;
            label_height += row_label_height ;
        }
LOCAL_DEBUG_OUT( "unshaded_row_count = %d", unshaded_row_count );
        if( unshaded_row_count == 0 )
            return PagerState.desk_height;
        return (main_height - label_height)/unshaded_row_count;
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
    int row_start = ((d->desk-PagerState.start_desk)/Config->columns)*Config->columns ;
    int i ;
    for( i = row_start ; i < row_start+Config->columns ; ++i )
    {
        if( shade )
            set_flags( PagerState.desks[i].flags, ASP_DeskShaded );
        else
            clear_flags( PagerState.desks[i].flags, ASP_DeskShaded );
    }
    rearrange_pager_desks( False );
}
/*************************************************************************
 *
 *************************************************************************/
void
set_client_name( ASWindowData *wd, Bool redraw )
{
    if( wd->bar )
        change_astbar_first_label( wd->bar, wd->window_name );
    if( redraw && wd->canvas )
        render_astbar( wd->bar, wd->canvas );
}

void
place_client( ASPagerDesk *d, ASWindowData *wd, Bool force_redraw )
{
    int x = 0, y = 0, width = 1, height = 1;
    int curr_x, curr_y ;
    int desk_width = d->background->width ;
    int desk_height = d->background->height ;

    if( wd )
    {
        int client_x = wd->frame_rect.x ;
        int client_y = wd->frame_rect.y ;
        if( get_flags( wd->state_flags, AS_Sticky )  )
        {
            client_x += Scr.Vx ;
            client_y += Scr.Vy ;
        }
        LOCAL_DEBUG_OUT( "+PLACE->client(%lX)->frame_geom(%dx%d%+d%+d)", wd->client, wd->frame_rect.width, wd->frame_rect.height, wd->frame_rect.x, wd->frame_rect.y );
        x = ( client_x * desk_width )/PagerState.vscreen_width ;
        y = ( client_y * desk_height)/PagerState.vscreen_height;
        width  = ( wd->frame_rect.width  * desk_width )/PagerState.vscreen_width ;
        height = ( wd->frame_rect.height * desk_height)/PagerState.vscreen_height;
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

        x += (int)d->background->win_x ;
        y += (int)d->background->win_y ;

        if( wd->canvas )
        {
            ASCanvas *canvas = wd->canvas ;
            get_canvas_position( canvas, NULL, &curr_x, &curr_y );
            if( curr_x == x && curr_y == y && width == canvas->width && height == canvas->height  )
            {
                if( force_redraw )
                {
                    render_astbar( wd->bar, canvas );
                    update_canvas_display( canvas );
                }
            }else
                moveresize_canvas( canvas, x, y, width, height );
            LOCAL_DEBUG_OUT( "+PLACE->canvas(%p)->geom(%dx%d%+d%+d)", wd->canvas, width, height, x, y );
        }
    }
}

void
set_client_look( ASWindowData *wd, Bool redraw )
{
    if( wd->bar )
    {
        set_astbar_style_ptr( wd->bar, BAR_STATE_UNFOCUSED, Scr.Look.MSWindow[BACK_UNFOCUSED] );
        set_astbar_style_ptr( wd->bar, BAR_STATE_FOCUSED, Scr.Look.MSWindow[BACK_FOCUSED] );
    }

    if( redraw && wd->canvas )
        render_astbar( wd->bar, wd->canvas );
}

void
on_client_moveresize(ASWindowData *wd )
{
    if( handle_canvas_config( wd->canvas ) != 0 )
    {
        set_astbar_size( wd->bar, wd->canvas->width, wd->canvas->height );
        render_astbar( wd->bar, wd->canvas );
        update_canvas_display( wd->canvas );
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
    ASWindowData *wd = NULL ;
    if( PagerClients )
        get_hash_item( PagerClients, AS_HASHABLE(w), (void**)&wd );
    return wd ;
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
                while( ++k < last_k )
                    d->clients[k-1] = d->clients[k] ;
                d->clients[k-1] = NULL;
                --(d->clients_num);
                break;
            }
        LOCAL_DEBUG_OUT( "client found at %d", i );
    }
}

void add_desk_client( ASPagerDesk *d, ASWindowData *wd )
{
    LOCAL_DEBUG_OUT( "%p, %p", d, wd );
    if( d && wd )
    {
        d->clients = realloc( d->clients, (d->clients_num+1)*sizeof(ASWindowData*));
        d->clients[d->clients_num] = wd ;
        ++(d->clients_num);
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

    set_astbar_hilite( wd->bar, NORMAL_HILITE|NO_HILITE_OUTLINE );
    add_astbar_label( wd->bar, 0, 0, 0, NO_ALIGN, NULL );
    move_astbar( wd->bar, wd->canvas, 0, 0 );
    if( wd->focused )
        set_astbar_focused( wd->bar, NULL, True );

    set_client_name( wd, False);
    set_client_look( wd, False );
    place_client( d, wd, True );
    map_canvas_window( wd->canvas, True );
    LOCAL_DEBUG_OUT( "+CREAT->canvas(%p)->bar(%p)->client_win(%lX)", wd->canvas, wd->bar, wd->client );
}

void refresh_client( int old_desk, ASWindowData *wd )
{
    ASPagerDesk *d = get_pager_desk( wd->desk );
    LOCAL_DEBUG_OUT( "client(%lX)->name(%s)->icon_name(%s)->desk(%d)->old_desk(%d)", wd->client, wd->window_name, wd->icon_name, wd->desk, old_desk );
    if( old_desk != wd->desk )
    {
        forget_desk_client( old_desk, wd );
        add_desk_client( d, wd );
        LOCAL_DEBUG_OUT( "reparenting client to desk %d", d->desk );
        quietly_reparent_canvas( wd->canvas, d->desk_canvas->w, CLIENT_EVENT_MASK, False );
    }
    set_client_name( wd, True );
    set_astbar_focused( wd->bar, wd->canvas, wd->focused );
    place_client( d, wd, False );
    LOCAL_DEBUG_OUT( "all done%s", "" );
}


void
change_desk_stacking( int desk, unsigned int clients_num, Window *clients )
{
    ASPagerDesk *d = get_pager_desk( desk );
    int i ;
    if( d == NULL )
        return;

    if( d->clients_num < clients_num )
        d->clients = realloc( d->clients, clients_num*sizeof(ASWindowData*));
    for( i = 0 ; i < clients_num ; ++i )
    {
        d->clients[i] = fetch_window_by_id( clients[i] );
        LOCAL_DEBUG_OUT( "id(%lX)->wd(%p)", clients[i], d->clients[i] );
    }
    restack_desk_windows( d );
}

void
set_desktop_pixmap( int desk, Pixmap pmap )
{
    ASPagerDesk *d = get_pager_desk( desk );
    int width, height;

    if( !get_drawable_size( pmap, &width, &height ) )
        pmap = None ;
    LOCAL_DEBUG_OUT( "desk(%d)->d(%p)->pmap(%lX)->size(%dx%d)", desk, d, pmap, width, height );
    if( pmap == None  )
        return ;
    if( d == None )
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
switch_desks( int new_desk )
{
    Scr.CurrentDesk = new_desk;

    if( IsValidDesk( new_desk ) )
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
    place_selection();
}

void
move_sticky_clients()
{
    int desk = PagerState.desks_num ;

    while( --desk >= 0 )
    {
        ASPagerDesk *d = &(PagerState.desks[desk]) ;
        if( d->clients && d->clients_num > 0 )
        {
            register int i = d->clients_num ;
            register ASWindowData **clients = d->clients ;
            while( --i >= 0 )
                if( clients[i] && get_flags( clients[i]->state_flags, AS_Sticky))
                    place_client( d, clients[i], True );
        }
    }
}

static char as_comm_buf[256];
void
move_client_wm( ASWindowData* wd, int x, int y )
{
    sprintf( as_comm_buf, "Move %dp %dp", x-Scr.Vx, y-Scr.Vy );
    SendInfo( as_comm_buf, wd->client );
}

void
move_client_to_desk( ASWindowData* wd, int desk )
{
    sprintf( as_comm_buf, "WindowsDesk %d", desk );
    SendInfo( as_comm_buf, wd->client );
}

void
resize_client_wm( ASWindowData* wd, unsigned int width, unsigned int height )
{
    sprintf( as_comm_buf, "Resize %dp %dp", width, height );
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
        if( (d=get_pager_desk( desk )) )
        {
            if( d->desk_canvas->root_x > x+width  || d->desk_canvas->root_x+d->desk_canvas->width  <= x ||
                d->desk_canvas->root_y > y+height || d->desk_canvas->root_y+d->desk_canvas->height <= y )
            {
                d = NULL;
            }
        }

        while( --i >= 0 && d == NULL )
        {
            d = &(PagerState.desks[i]);
            LOCAL_DEBUG_OUT( "checking desk %d: pos(%+d%+d)->desk_geom(%dx%d%+d%+d)", i, x, y, d->desk_canvas->width, d->desk_canvas->height, d->desk_canvas->root_x, d->desk_canvas->root_y );
            if( d->desk_canvas->root_x > x+width  || d->desk_canvas->root_x+d->desk_canvas->width  <= x ||
                d->desk_canvas->root_y > y+height || d->desk_canvas->root_y+d->desk_canvas->height <= y )
            {
                d = NULL;
            }
        }
    }

    if( d )
    {
        x -= d->background->root_x ;
        y -= d->background->root_y ;
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


void
apply_client_move(struct ASMoveResizeData *data)
{
    ASWindowData *wd = fetch_client(AS_WIDGET_WINDOW(data->mr));
    int real_x = 0, real_y = 0;
    ASPagerDesk  *d = translate_client_pos_main( data->curr.x, data->curr.y, data->curr.width, data->curr.height, wd->desk, &real_x, &real_y );
    if( d && d->desk + PagerState.start_desk != wd->desk )
    {
        move_client_to_desk( wd, d->desk + PagerState.start_desk );
        set_moveresize_aspect( data, PagerState.vscreen_width, d->background->width,
                                     PagerState.vscreen_height, d->background->height,
                                     d->background->root_x, d->background->root_y );
    }
    LOCAL_DEBUG_OUT( "d(%p)->curr(%+d%+d)->real(%+d%+d)", d, data->curr.x, data->curr.y, real_x, real_y);
    move_client_wm( wd, real_x, real_y);
}

void complete_client_move(struct ASMoveResizeData *data, Bool cancelled)
{
    ASWindowData *wd = fetch_client( AS_WIDGET_WINDOW(data->mr));
    int real_x = 0, real_y = 0;
    ASPagerDesk  *d = NULL ;
    if( cancelled )
        d = translate_client_pos_main( data->start.x, data->start.y, data->start.width, data->start.height, wd->desk, &real_x, &real_y );
    else
        d = translate_client_pos_main( data->curr.x, data->curr.y, data->curr.width, data->curr.height, wd->desk, &real_x, &real_y );

    if( d && d->desk + PagerState.start_desk != wd->desk )
    {
        move_client_to_desk( wd, d->desk + PagerState.start_desk );
        set_moveresize_aspect( data, PagerState.vscreen_width, d->background->width,
                                     PagerState.vscreen_height, d->background->height,
                                     d->background->root_x, d->background->root_y );
    }
    LOCAL_DEBUG_OUT( "d(%p)->start(%+d%+d)->curr(%+d%+d)->real(%+d%+d)", d, data->start.x, data->start.y, data->curr.x, data->curr.y, real_x, real_y);
    move_client_wm( wd, real_x, real_y);
    Scr.moveresize_in_progress = NULL ;
}

void apply_client_resize(struct ASMoveResizeData *data)
{
    ASWindowData *wd = fetch_client(AS_WIDGET_WINDOW(data->mr));
    unsigned int real_width=1, real_height = 1;
LOCAL_DEBUG_OUT( "desk(%p)->size(%dx%d)", PagerState.resize_desk, data->curr.width, data->curr.height);
    translate_client_size( data->curr.width, data->curr.height, &real_width, &real_height );
    resize_client_wm( wd, real_width, real_height);
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
    resize_client_wm( wd, real_width, real_height);
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
                    add_canvas_grid( grid, wd->canvas, outer_gravity, inner_gravity );
            }
    }
    /* add all the window edges for this desktop : */
    //iterate_asbidirlist( Scr.Windows->clients, get_aswindow_grid_iter_func, (void*)&grid_data, NULL, False );

#ifdef LOCAL_DEBUG
    print_asgrid( grid );
#endif

    return grid;
}


void
start_moveresize_client( ASWindowData *wd, Bool move, ASEvent *event )
{
    ASMoveResizeData *mvrdata;
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
    }else
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

//        set_moveresize_restrains( mvrdata, asw->hints, asw->status);
//            mvrdata->subwindow_func = on_deskelem_move_subwindow ;
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
process_message (unsigned long type, unsigned long *body)
{
    LOCAL_DEBUG_OUT( "received message %lX", type );
	if( (type&WINDOW_PACKET_MASK) != 0 )
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );
//        ASTBarData *tbar = wd?wd->tbar:NULL;
		WindowPacketResult res ;
        /* saving relevant client info since handle_window_packet could destroy the actuall structure */
        Window               saved_w = wd?wd->canvas->w:None;
        int                  saved_desk = wd?wd->desk:INVALID_DESK;
        struct ASWindowData *saved_wd = wd ;

        show_progress( "message %lX window %X data %p", type, body[0], wd );
		res = handle_window_packet( type, body, &wd );
        if( res == WP_DataCreated )
            add_client( wd );
		else if( res == WP_DataChanged )
            refresh_client( saved_desk, wd );
		else if( res == WP_DataDeleted )
        {
            LOCAL_DEBUG_OUT( "client deleted (%p)->window(%lX)->desk(%d)", saved_wd, saved_w, saved_desk );
            forget_desk_client( saved_desk, saved_wd );
            unregister_client( saved_w );
        }
    }else
    {
        switch( type )
        {
            case M_TOGGLE_PAGING :
                break;
            case M_NEW_PAGE :
                {
                    Scr.Vx = (long) body[0];
                    Scr.Vy = (long) body[1];
                    LOCAL_DEBUG_OUT("M_NEW_PAGE(desk = %d,Vx=%d,Vy=%d)", Scr.CurrentDesk, Scr.Vx, Scr.Vy);
                    move_sticky_clients();
                    place_selection();
                }
                break;
            case M_NEW_DESK :
                {
                    LOCAL_DEBUG_OUT("M_NEW_DESK(New=%ld, Old=%d)", body[0], Scr.CurrentDesk);
                    switch_desks( body[0] );
                }
                break ;
            case M_STACKING_ORDER :
                {
                    LOCAL_DEBUG_OUT("M_STACKING_ORDER(desk=%ld, clients_num=%ld)", body[0], body[1]);
                    change_desk_stacking( body[0], body[1], &(body[2]) );
                }
                break ;
        }
    }
}
/*************************************************************************
 * Event handling :
 *************************************************************************/
void
DispatchEvent (ASEvent * event)
{
#ifndef EVENT_TRACE
    if( get_output_threshold() >= OUTPUT_LEVEL_DEBUG )
#endif
    {
        show_progress("****************************************************************");
        show_progress("%s:%s:%d><<EVENT type(%d(%s))->x.window(%lx)->event.w(%lx)->client(%p)->context(%s)->send_event(%d)", __FILE__, __FUNCTION__, __LINE__, event->x.type, event_type2name(event->x.type), event->x.xany.window, event->w, event->client, context2text(event->context), event->x.xany.send_event);
    }

    LOCAL_DEBUG_OUT( "mvrdata(%p)->main_canvas(%p)->widget(%p)", Scr.moveresize_in_progress, PagerState.main_canvas, event->widget );
    if( Scr.moveresize_in_progress )
    {
        event->widget = PagerState.resize_desk?PagerState.resize_desk->desk_canvas:PagerState.main_canvas ;
        if( check_moveresize_event( event ) )
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
                event->client = (ASWindow*)wd;
                event->widget = ((ASWindowData*)(event->client))->canvas ;
            }
        }
    }

    balloon_handle_event (&(event->x));

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
            break ;
        case KeyRelease :
            if( event->client != NULL )
            {
                ASWindowData *wd = (ASWindowData*)(event->client);
                event->x.xkey.window = wd->client;
                XSendEvent (dpy, wd->client, False, KeyReleaseMask, &(event->x));
            }
            break ;
        case ButtonPress:
            on_pager_pressure_changed( event );
            break;
        case ButtonRelease:
LOCAL_DEBUG_OUT( "state(0x%X)->state&ButtonAnyMask(0x%X)", event->x.xbutton.state, event->x.xbutton.state&ButtonAnyMask );
            if( (event->x.xbutton.state&ButtonAnyMask) == (Button1Mask<<(event->x.xbutton.button-Button1)) )
                release_pressure();
            break;
        case MotionNotify :
            if( (event->x.xbutton.state&Button3Mask) )
                on_scroll_viewport( event );
            break ;
	    case ClientMessage:
            LOCAL_DEBUG_OUT("ClientMessage(\"%s\",data=(%lX,%lX,%lX,%lX,%lX)", XGetAtomName( dpy, event->x.xclient.message_type ), event->x.xclient.data.l[0], event->x.xclient.data.l[1], event->x.xclient.data.l[2], event->x.xclient.data.l[3], event->x.xclient.data.l[4]);
            if ( event->x.xclient.format == 32 &&
                 event->x.xclient.data.l[0] == _XA_WM_DELETE_WINDOW )
			{
                DeadPipe(0);
            }else if( event->x.xclient.format == 32 &&
                      event->x.xclient.message_type == _AS_BACKGROUND && event->x.xclient.data.l[1] != None )
            {
                set_desktop_pixmap( event->x.xclient.data.l[0]-PagerState.start_desk, event->x.xclient.data.l[1] );
            }

	        break;
	    case PropertyNotify:
            if( event->x.xproperty.atom == _XROOTPMAP_ID && event->w == Scr.Root )
            {
                register int i = PagerState.desks_num ;
                LOCAL_DEBUG_OUT( "root background updated!%s","");
                safe_asimage_destroy( Scr.RootImage );
                Scr.RootImage = NULL ;
                while( --i >= 0 )
                {
                    update_astbar_transparency(PagerState.desks[i].title, PagerState.desks[i].desk_canvas);
                    update_astbar_transparency(PagerState.desks[i].background, PagerState.desks[i].desk_canvas);
                    render_desk( &(PagerState.desks[i]), False );
                }
            }
            break;
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
        if( !get_flags(d->flags, ASP_DeskShaded ) )
        {   /* place all the grid separation windows : */
            register Window *wa = d->separator_bars;
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
                    XMoveResizeWindow( dpy, wa[p], pos, pos2, 1, size );
                    pos -= pos_inc ;
                }
                /* horizontal bars */
                wa += PagerState.page_columns-1;
                p = PagerState.page_rows-1;
                pos_inc = d->background->height/PagerState.page_rows ;
                pos = d->background->win_y + p*pos_inc;
                pos2 = d->background->win_x ;
                size = d->background->width ;
                while( --p >= 0 )
                {
                    XMoveResizeWindow( dpy, wa[p], pos2, pos, size, 1 );
                    pos -= pos_inc ;
                }
            }
            /* rearrange all the client windows : */
            if( d->clients && d->clients_num > 0 )
            {
                register int i ;
                i = d->clients_num ;
                while( --i >= 0 )
                    place_client( d, d->clients[i], False );
            }
            if( d->desk == Scr.CurrentDesk )
                place_selection();
            if( get_flags( d->flags, ASP_UseRootBackground )  )
            {
                if( d->back == NULL || (d->back->width != d->background->width || d->back->height != d->background->height )  )
                {
                    XEvent e;
                    e.xclient.type = ClientMessage ;
                    e.xclient.message_type = _AS_BACKGROUND ;
                    e.xclient.format = 16;
                    e.xclient.window = PagerState.main_canvas->w ;
                    e.xclient.data.s[0] = PagerState.start_desk+d->desk ;
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
        }
    }

    update_astbar_transparency(d->title, d->desk_canvas);
    update_astbar_transparency(d->background, d->desk_canvas);

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
                new_desk_width  = calculate_pager_desk_width();
                new_desk_height = calculate_pager_desk_height();

                if( new_desk_width <= 0 )
                    new_desk_width = 1;
                if( new_desk_height <= 0 )
                    new_desk_height = 1;
                LOCAL_DEBUG_OUT( "old_desk_size(%dx%d) new_desk_size(%dx%d)", PagerState.desk_width, PagerState.desk_height, new_desk_width, new_desk_height );
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
                if( PagerState.desks[i].desk_canvas->w == w )
                {
                    on_desk_moveresize( &(PagerState.desks[i]) );
                    break;
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
LOCAL_DEBUG_OUT( "root_pos(%+d%+d)->title_root_pos(%+d%+d)->context(%s)", root_x, root_y, d->title->root_x, d->title->root_y, context2text(context) );
    if( context != C_NO_CONTEXT )
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
        px -= d->desk_canvas->root_x ;
        py -= d->desk_canvas->root_y ;
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
                /* now calculating delta */
                sx -= Scr.Vx;
                sy -= Scr.Vy;
                /* now translating delta into persentage of the screen width */
                sx = (100 * sx) / Scr.MyDisplayWidth;
                sy = (100 * sy) / Scr.MyDisplayHeight;
                /* we don't want to move in very small increments */
                if (sx < PAGE_MOVE_THRESHOLD && sy < PAGE_MOVE_THRESHOLD &&
                    sx > -(PAGE_MOVE_THRESHOLD) && sy > -(PAGE_MOVE_THRESHOLD))
                    return;
                sprintf (command, "Scroll %d %d\n", sx, sy);
                SendInfo ( command, 0);
                PagerState.wait_as_response++;
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
        if( PagerState.pressed_desk )
        {
            ASPagerDesk *d = PagerState.pressed_desk ;
            if( PagerState.pressed_context == C_R1 )
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
                px -= d->desk_canvas->root_x ;
                py -= d->desk_canvas->root_y ;
                if( px > 0 && px < d->desk_canvas->width &&
                    py > 0 && py < d->desk_canvas->height )
                {
                    px -= d->background->win_x;
                    py -= d->background->win_y;
                    if( px >= 0 && py >= 0 &&
                        px < d->background->width && py < d->background->height )
                    {
                        SendInfo ("Desk 0 10000", 0);
                        PagerState.wait_as_response++;
                        if( PagerState.pressed_button == Button3 )
                        {
                            int sx = (px*PagerState.vscreen_width)/d->background->width ;
                            int sy = (py*PagerState.vscreen_height)/d->background->height ;
                            /* now calculating delta */
                            sx -= Scr.Vx;
                            sy -= Scr.Vy;
                            /* now translating delta into persentage of the screen width */
                            sx = (100 * sx) / Scr.MyDisplayWidth;
                            sy = (100 * sy) / Scr.MyDisplayHeight;
#if 0
                            /* we don't want to move in very small increments */
                            if (event->type == MotionNotify)
                                if (sx < PAGE_MOVE_THRESHOLD && sy < PAGE_MOVE_THRESHOLD &&
                                    sx > -(PAGE_MOVE_THRESHOLD) && sy > -(PAGE_MOVE_THRESHOLD))
                                return;
#endif
                            sprintf (command, "Scroll %d %d\n", sx, sy);
                        }else
                        {
                            int page_column = 0, page_row = 0;
                            /*  calculate destination page : */
                            page_column = (px*PagerState.page_columns)/d->background->width ;
                            page_row    = (py*PagerState.page_rows)/d->background->height ;
                            sprintf (command, "GotoPage %d %d\n", page_column, page_row);
                        }
                        SendInfo ( command, 0);
                        PagerState.wait_as_response++;
                    }
                    sprintf (command, "Desk 0 %d\n", d->desk + PagerState.start_desk);
                    SendInfo (command, 0);
                    PagerState.wait_as_response++;
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
        sleep_a_little (1000);
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


