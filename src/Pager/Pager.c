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

typedef struct ASPagerDesk {
    int desk ;
    ASCanvas   *desk_canvas;
    ASTBarData *title;
    ASTBarData *background;
    Window     *separator_bars;                /* (rows-1)*(columns-1) */
    unsigned int title_size;
}ASPagerDesk;

typedef struct ASPagerState
{
    ASFlagType flags ;

    ASCanvas    *main_canvas;
    ASCanvas    *icon_canvas;

    ASPagerDesk *desks ;
    int start_desk, desks_num ;

    int desk_rows,  desk_columns;
    int page_rows,  page_columns ;
    int desk_width, desk_height ; /* x and y size of desktop */
    int vscreen_width, vscreen_height ; /* x and y size of desktop */
    int aspect_x,   aspect_y;

    int wait_as_response ;

}ASPagerState;

ASPagerState PagerState ={ 0, NULL, NULL, NULL, 0, 0 };
#define DEFAULT_BORDER_COLOR 0xFF808080

PagerConfig *Config = NULL;

void
pager_usage (void)
{
	printf ("Usage:\n"
			"%s [--version] [--help] n m\n"
			"%*s where desktops n through m are displayed\n", MyName, (int)strlen (MyName), MyName);
	exit (0);
}

void HandleEvents(int x_fd, int *as_fd);
void process_message (unsigned long type, unsigned long *body);
void DispatchEvent (ASEvent * Event);
void rearrange_pager_window();
Window make_pager_window();
void GetOptions (const char *filename);
void GetBaseOptions (const char *filename);
void CheckConfigSanity();
void rearrange_pager_window();
void on_pager_window_moveresize( void *client, Window w, int x, int y, unsigned int width, unsigned int height );

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
    int x_fd ;
	int as_fd[2] ;

    /* Save our program name - for error messages */
    InitMyApp (CLASS_PAGER, argc, argv, pager_usage, NULL, 0 );

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

    if( (x_fd = ConnectX( &Scr, MyArgs.display_name, PropertyChangeMask )) < 0 )
    {
        show_error("failed to initialize window manager session. Aborting!", Scr.screen);
        exit(1);
    }
    if (fcntl (x_fd, F_SETFD, 1) == -1)
	{
        show_error ("close-on-exec failed");
        exit (3);
	}

    if (get_flags( MyArgs.flags, ASS_Debugging))
        set_synchronous_mode(True);
    XSync (dpy, 0);


    as_fd[0] = as_fd[1] = ConnectAfterStep (M_ADD_WINDOW |
                                            M_CONFIGURE_WINDOW |
                                            M_DESTROY_WINDOW |
                                            M_FOCUS_CHANGE |
                                            M_NEW_PAGE |
                                            M_NEW_DESK |
                                            M_NEW_BACKGROUND |
                                            M_RAISE_WINDOW |
                                            M_LOWER_WINDOW |
                                            M_ICONIFY |
                                            M_ICON_LOCATION |
                                            M_DEICONIFY |
                                            M_ICON_NAME |
                                            M_END_WINDOWLIST);

    Config = CreatePagerConfig (PagerState.desks_num);

    LOCAL_DEBUG_OUT("parsing Options ...%s","");
    LoadBaseConfig (GetBaseOptions);
    LoadConfig ("pager", GetOptions);

    CheckConfigSanity();

    /* Create a list of all windows */
    /* Request a list of all windows,
     * wait for ConfigureWindow packets */
    SendInfo (as_fd, "Send_WindowList", 0);

    PagerState.main_canvas = create_ascanvas( make_pager_window() );
    rearrange_pager_window();

    LOCAL_DEBUG_OUT("starting The Loop ...%s","");
    HandleEvents(x_fd, as_fd);

    return 0;
}

void HandleEvents(int x_fd, int *as_fd)
{
    ASEvent event;
    Bool has_x_events = False ;
    while (True)
    {
        if (PagerState.wait_as_response > 0)
        {
            ASMessage *msg = CheckASMessage (as_fd[1], WAIT_AS_RESPONSE_TIMEOUT);
            if (msg)
            {
                process_message (msg->header[1], msg->body);
                DestroyASMessage (msg);
            }
        }else
        {
            while((has_x_events = XPending (dpy)))
            {
                if( ASNextEvent (&(event.x), True) )
                {
                    event.client = NULL ;
                    DispatchEvent( &event );
                }
            }
            module_wait_pipes_input ( x_fd, as_fd[1], process_message );
        }
    }
}
/****************************************************************************/
/* PROCESSING OF AFTERSTEP MESSAGES :                                       */
/****************************************************************************/
void
process_message (unsigned long type, unsigned long *body)
{
	if( (type&WINDOW_PACKET_MASK) != 0 )
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );
//        ASTBarData *tbar = wd?wd->tbar:NULL;
		WindowPacketResult res ;


		show_progress( "message %X window %X data %p", type, body[0], wd );
		res = handle_window_packet( type, body, &wd );
#if 0
        if( res == WP_DataCreated )
			add_winlist_button( tbar, wd );
		else if( res == WP_DataChanged )
			refresh_winlist_button( tbar, wd );
		else if( res == WP_DataDeleted )
			delete_winlist_button( tbar, wd );
#endif
	}
}

void
DispatchEvent (ASEvent * event)
{
    balloon_handle_event (&(event->x));

    switch (event->x.type)
    {
	    case ConfigureNotify:
            on_pager_window_moveresize( event->client, event->w,
                                  event->x.xconfigure.x,
                                  event->x.xconfigure.y,
                                  event->x.xconfigure.width,
                                  event->x.xconfigure.height );
            break;
        case ButtonPress:
	    case ButtonRelease:
			break;
	    case ClientMessage:
            if ((event->x.xclient.format == 32) &&
                (event->x.xclient.data.l[0] == _XA_WM_DELETE_WINDOW))
			{
			    exit (0);
			}
	        break;
	    case PropertyNotify:
			break;
    }
}

void
DeadPipe (int nonsense)
{
#ifdef DEBUG_ALLOCS
	int i;
    GC foreGC, backGC, reliefGC, shadowGC;

/* normally, we let the system clean up, but when auditing time comes
 * around, it's best to have the books in order... */
    balloon_init (1);
    if( PagerState.main_canvas )
        destroy_ascanvas( &PagerState.main_canvas );

    if( Config )
        DestroyPagerConfig (Config);

    mystyle_get_global_gcs (mystyle_first, &foreGC, &backGC, &reliefGC, &shadowGC);
    mystyle_destroy_all();
    XFreeGC (dpy, foreGC);
    XFreeGC (dpy, backGC);
    XFreeGC (dpy, reliefGC);
    XFreeGC (dpy, shadowGC);

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
    for( i = 0 ; i < DESK_STYLES ; ++i )
    {
        static char *desk_style_names[DESK_STYLES] ={"*%sActiveDesk", "*%sInActiveDesk" };

        sprintf( buf, desk_style_names[i], MyName );
        Config->MSDeskTitle[i] = mystyle_find_or_default( buf );
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
}


void merge_geometry( ASGeometry *from, ASGeometry *to )
{
    if ( get_flags(from->flags, WidthValue) )
        to->width = from->width ;
    if ( get_flags(from->flags, HeightValue) )
        to->height = from->height ;
    if ( get_flags(from->flags, XValue) )
    {
        to->x = from->x ;
        if( !get_flags(from->flags, XNegative) )
            clear_flags(to->flags, XNegative);
    }
    if ( get_flags(from->flags, YValue) )
    {
        to->y = from->y ;
        if( !get_flags(from->flags, YNegative) )
            clear_flags(to->flags, YNegative);
    }
    to->flags |= from->flags ;
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

    if (config->style_defs)
        ProcessMyStyleDefinitions (&(config->style_defs), PixmapPath);

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
    attr.event_mask = StructureNotifyMask ;
    w = create_visual_window( Scr.asv, Scr.Root, x, y, width, height, 0, InputOutput, CWEventMask, &attr);
    set_client_names( w, MyName, MyName, CLASS_PAGER, MyName );

    shints.flags = USSize|PMinSize|PResizeInc|PWinGravity;
    if( get_flags( Config->set_flags, PAGER_SET_GEOMETRY ) )
        shints.flags |= USPosition ;
    else
        shints.flags |= PPosition ;

    shints.min_width = PagerState.desk_columns;
    shints.min_height = PagerState.desk_rows;
    shints.width_inc = PagerState.desk_columns;
    shints.height_inc = PagerState.desk_rows;
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

static void
place_desk_title( ASPagerDesk *d )
{
    if( d->title )
    {
        move_astbar( d->title, d->desk_canvas, 0, get_flags(Config->flags,LABEL_BELOW_DESK)?d->desk_canvas->height - d->title_size:0);
        set_astbar_size( d->title, d->desk_canvas->width, d->title_size );
    }
}

static void
place_desk_background( ASPagerDesk *d )
{
    move_astbar( d->background, d->desk_canvas, 0, get_flags(Config->flags,LABEL_BELOW_DESK)?0:d->title_size);
    set_astbar_size( d->background, d->desk_canvas->width, d->desk_canvas->height - d->title_size );
}

static void
render_desk( ASPagerDesk *d, Bool force )
{
    Bool update_display = False ;

    if( d->title )
        if( force || DoesBarNeedsRendering(d->title) )
            if( render_astbar( d->title, d->desk_canvas ) )
                update_display = True;
    if( force || DoesBarNeedsRendering(d->background) )
        if( render_astbar( d->background, d->desk_canvas ) )
            update_display = True;
    if( update_display )
            update_canvas_display( d->desk_canvas );
}

void rearrange_pager_window()
{
    /* need to create enough desktop canvases */
    int i ;
    int col = 0, row = 0 ;
    char buf[256];
    XSetWindowAttributes desk_attr;
    desk_attr.event_mask = StructureNotifyMask|ButtonReleaseMask|ButtonPressMask|ButtonMotionMask ;
    ARGB2PIXEL(Scr.asv,Config->border_color_argb,&(desk_attr.border_pixel));
    for( i = 0 ; i < PagerState.desks_num ; ++i )
    {
        ASPagerDesk *d = &(PagerState.desks[i]);
        int x, y;
        Bool enable_rendering = True;
        x = col*PagerState.desk_width;
        y = row*PagerState.desk_height;

        if( d->desk_canvas == NULL )
        {
            Window w;
            w = create_visual_window(Scr.asv, PagerState.main_canvas->w, x, y, PagerState.desk_width, PagerState.desk_height,
                                     Config->border_width, InputOutput, CWEventMask|CWBorderPixel, &desk_attr );
            d->desk_canvas = create_ascanvas( w );
            enable_rendering = False ;
        }else
        {
            if( d->desk_canvas->width != PagerState.desk_width || d->desk_canvas->height != PagerState.desk_height )
                enable_rendering = False ;
            moveresize_canvas( d->desk_canvas, x, y, PagerState.desk_width, PagerState.desk_height );
        }
        /* create & moveresize label bar : */
        if( get_flags( Config->flags, USE_LABEL ) )
        {
            if( d->title == NULL )
            {
                d->title = create_astbar();
                add_astbar_label( d->title, 0, 0, 0, 0, NULL );
            }
            set_astbar_style_ptr( d->title, BAR_STATE_FOCUSED, Config->MSDeskTitle[DESK_ACTIVE] );
            set_astbar_style_ptr( d->title, BAR_STATE_UNFOCUSED, Config->MSDeskTitle[DESK_INACTIVE] );
            if( Config->labels && Config->labels[i] )
                change_astbar_first_label( d->title, Config->labels[i] );
            else
            {
                sprintf( buf, "Desk %d", PagerState.start_desk+i );
                change_astbar_first_label( d->title, buf );
            }
            d->title_size = calculate_astbar_height( d->title );
            place_desk_title(d);
        }else
        {
            if( d->title )
                destroy_astbar( &(d->title) );
            d->title_size = 0 ;
        }
        /* create & moveresize desktop background bar : */
        if( d->background == NULL )
            d->background = create_astbar();
        set_astbar_style_ptr( d->background, BAR_STATE_FOCUSED, Config->MSDeskBack[i] );
        set_astbar_style_ptr( d->background, BAR_STATE_UNFOCUSED, Config->MSDeskBack[i] );
        place_desk_background(d);

        if( enable_rendering )
            render_desk( d, False );

        if( ++col >= PagerState.desk_columns )
        {
            ++row;
            col = 0;
        }
    }
}
/*************************************************************************
 * individuaL Desk manipulation
 *************************************************************************/
void
on_desk_moveresize( ASPagerDesk *d, int x, int y, unsigned int width, unsigned int height )
{
    ASFlagType changes = handle_canvas_config( d->desk_canvas );
    if( get_flags(changes, CANVAS_RESIZED ) )
    {
        place_desk_title( d );
        place_desk_background( d );
    }

    if( changes != 0 )
        render_desk( d, False );
}

/*************************************************************************
 * Event handling :
 *************************************************************************/
void on_pager_window_moveresize( void *client, Window w, int x, int y, unsigned int width, unsigned int height )
{
    if( client == NULL )
    {   /* then its one of our canvases !!! */
        int i ;
        if( w == PagerState.main_canvas->w )
        { /* need to rescale everything? maybe: */
            int new_desk_width = width / PagerState.desk_columns ;
            int new_desk_height = height / PagerState.desk_rows ;
            int col = 0, row = 0;

            handle_canvas_config( PagerState.main_canvas );
            if( new_desk_width <= 0 )
                new_desk_width = 1;
            if( new_desk_height <= 0 )
                new_desk_height = 1;

            if( new_desk_width != PagerState.desk_width ||
                new_desk_height != PagerState.desk_height )
            {
                PagerState.desk_width = new_desk_width ;
                PagerState.desk_height = new_desk_height ;
                for( i = 0 ; i < PagerState.desks_num ; ++i )
                {
                    moveresize_canvas( PagerState.desks[i].desk_canvas, col* (PagerState.desk_width), row*PagerState.desk_height, new_desk_width, new_desk_height );
                    if( ++col >= PagerState.desk_columns )
                    {
                        ++row;
                        col = 0;
                    }
                }
            }
        }else                                  /* then its one of our desk subwindows : */
        {
            for( i = 0 ; i < PagerState.desks_num; ++i )
                if( PagerState.desks[i].desk_canvas->w == w )
                {
                    on_desk_moveresize( &(PagerState.desks[i]), x, y, width, height );
                    break;
                }
        }
    }

}

