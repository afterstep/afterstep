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



typedef struct ASPagerDesk {
    int desk ;
    ASCanvas   *main_canvas;
    ASTBarData *title;
    ASTBarData *background;
    Window     *separator_bars;                /* (rows-1)*(columns-1) */
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

void process_message (unsigned long type, unsigned long *body);
void DispatchEvent (XEvent * Event);
void rearrange_pager_window();
Window make_pager_window();
void GetOptions (const char *filename);
void GetBaseOptions (const char *filename);


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

    /* Create a list of all windows */
    /* Request a list of all windows,
     * wait for ConfigureWindow packets */
    SendInfo (as_fd, "Send_WindowList", 0);

    PagerState.main_canvas = create_ascanvas( make_pager_window() );

    LOCAL_DEBUG_OUT("starting The Loop ...%s","");
    while (1)
    {
        XEvent event;
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
            if (!My_XNextEvent (dpy, x_fd, as_fd[1], process_message, &event))
                timer_handle ();    /* handle timeout events */
            else
            {
                balloon_handle_event (&event);
                DispatchEvent (&event);
            }
        }
    }

    return 0;
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
DispatchEvent (XEvent * Event)
{
    switch (Event->type)
    {
	    case ConfigureNotify:
#if 0
            if( handle_canvas_config( WinListCanvas ) )
			{
				ASBiDirElem *elem ;
				rearrange_winlist_buttons();
				for( elem = WinList->head ; elem ; elem = elem->next )
					update_astbar_transparency( elem->data, WinListCanvas );
			}
#endif
	        break;
	    case Expose:
	        break;
	    case ButtonPress:
	    case ButtonRelease:
			break;
	    case ClientMessage:
    	    if ((Event->xclient.format == 32) &&
		  	    (Event->xclient.data.l[0] == _XA_WM_DELETE_WINDOW))
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
    if( Config == NULL )
        Config = CreatePagerConfig (PagerState.desks_num);
    if( Config->rows == 0 )
        Config->rows = 1;

    if( Config->columns == 0 ||
        Config->rows*Config->columns != PagerState.desks_num )
        Config->columns = PagerState.desks_num/Config->rows;

    if( Config->rows*Config->columns < PagerState.desks_num )
        if( ++(config->columns) ;


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
        for( i = 0 ; i < PagerState.desks_num ; ++i )
            if( config->labels[i] )
                set_string_value( &(Config->labels[i]), config->labels[i], NULL, 0 );
    if( config->styles )
        for( i = 0 ; i < PagerState.desks_num ; ++i )
            if( config->styles[i] )
                set_string_value( &(Config->styles[i]), config->styles[i], NULL, 0 );
#if 0
    int align;
    unsigned long flags, set_flags;
    char *small_font_name;
    int border_width;

    char *selection_color;
    char *grid_color;
    char *border_color;

    /* these are generated after reading the config : */
    int gravity ;
    ARGB32  selection_color_argb;
    ARGB32  grid_color_argb;
    ARGB32  border_color_argb;
#endif

    DestroyPagerConfig (config);
    SHOW_TIME("Config parsing",option_time);
}











#if 0


        if (get_flags(config->geometry.flags, WidthValue) && config->geometry.width > config->columns )
            PagerState.desk_width = config->geometry.width/config->columns ;

        config->geometry.width = PagerState.desk_width*config->columns ;

        if (get_flags(config->geometry.flags, HeightValue) && config->geometry.height > config->rows )
            PagerState.desk_height = config->geometry.height/config->rows ;

        config->geometry.height = PagerState.desk_width*config->rows ;

        if( !get_flags(config->geometry.flags, XValue))
        {
            if(get_flags(config->geometry.flags, YValue))
                config->geometry.x = 0;
        }else
        {
            int real_x = config->geometry.x ;
            if( get_flags(config->geometry.flags, XNegative) )
            {
                config->gravity = NorthEastGravity ;
                real_x += Scr.MyDisplayWidth ;
            }
            if( real_x + config->geometry.width  < 0 )
                config->geometry.x = get_flags(config->geometry.flags, XNegative)?
                                        config->geometry.width-Scr.MyDisplayWidth : 0 ;
            else if( real_x > Scr.MyDisplayWidth )
                config->geometry.x = get_flags(config->geometry.flags, XNegative)?
                                        0 : Scr.MyDisplayWidth-config->geometry.width ;
        }
        if( !get_flags(config->geometry.flags, YValue) )
        {
            if( get_flags(config->geometry.flags, XValue) )
                config->geometry.y = 0;
        }else
        {
            int real_y = config->geometry.y ;
            if( get_flags(config->geometry.flags, YNegative) )
            {
                config->gravity = (config->gravity==NorthEastGravity)?SouthEastGravity:SouthWestGravity ;
                real_y += Scr.MyDisplayHeight ;
            }
            if( real_y + config->geometry.height  < 0 )
                config->geometry.y = get_flags(config->geometry.flags, YNegative)?
                                        config->geometry.height-Scr.MyDisplayHeight : 0 ;
            else if( real_y > Scr.MyDisplayHeight )
                config->geometry.y = get_flags(config->geometry.flags, YNegative)?
                                        0 : Scr.MyDisplayHeight-config->geometry.height ;
        }
    }else
    {
        config->geometry.x = 0 ;
        config->geometry.y = 0 ;
        config->geometry.width = PagerState.desk_width*PagerState.desk_columns ;
        config->geometry.height = PagerState.desk_height*PagerState.desk_rows ;
    }

    if( get_flags( config->set_flags, PAGER_SET_ICON_GEOMETRY ) )
    {
        if ( !get_flags(config->icon_geometry.flags, WidthValue) || config->icon_geometry.width <= 0 )
            config->icon_geometry.width = 54;
        if ( !get_flags(config->icon_geometry.flags, HeightValue)|| config->icon_geometry.height <= 0)
            config->icon_geometry.height = 54;
    }else
    {
        config->icon_geometry.width = 54;
        config->icon_geometry.height = 54;
    }

    if (config->style_defs)
        ProcessMyStyleDefinitions (&(config->style_defs), PixmapPath);
    mystyle_get_property (Scr.wmprops);
#endif

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
    PagerState.desk_width = Scr.VxMax + Scr.MyDisplayWidth;
    PagerState.desk_height = Scr.VyMax + Scr.MyDisplayHeight;

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
    unsigned int width = Config->geometry.width;
    unsigned int height = Config->geometry.height;

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

	w = create_visual_window( Scr.asv, Scr.Root, x, y, width, height, 0, InputOutput, 0, NULL);
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
    /* final cleanup */
	XFlush (dpy);
	sleep (1);								   /* we have to give AS a chance to spot us */
	/* we will need to wait for PropertyNotify event indicating transition
	   into Withdrawn state, so selecting event mask: */
	XSelectInput (dpy, w, PropertyChangeMask|StructureNotifyMask);
	return w ;
}

void rearrange_pager_window()
{


}

