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

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef I18N
#include <X11/Xlocale.h>
#endif

#define LOCAL_DEBUG

#define IN_MODULE
#define MODULE_X_INTERFACE

#include "../../include/afterbase.h"
#include "../../libAfterImage/afterimage.h"
#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/mystyle.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"
#include "../../include/clientprops.h"
#include "../../include/wmprops.h"
#include "../../include/decor.h"

#include "WinData.h"

#define AS_MODULE_CLASS	"WinList"

/**********************************************************************/
/*  AfterStep specific global variables :                             */
/**********************************************************************/
char *MyName;           /* our executable name                        */
ScreenInfo Scr;			/* AS compatible screen information structure */
/**********************************************************************/
/**********************************************************************/
/*  WinList local variables :                                         */
/**********************************************************************/
ASBiDirList *WinList = NULL ;
Window 		 WinListWindow = None ;
ASCanvas 	*WinListCanvas = NULL ;
typedef struct {
	unsigned int  width, height ;
	ASTBarData *widest, *tallest ;
	unsigned int max_width, max_height ;
	ASWindowData *focused;
}ASWinListState ;
ASWinListState WinListState = { 100, 100, NULL, NULL, 0, 0, NULL };

/**********************************************************************/
/**********************************************************************/
/* Our configuration options :                                        */
/**********************************************************************/
BaseConfig *Base = NULL;
typedef enum { ASN_Name = 0, ASN_IconName, ASN_ResClass, ASN_ResName, ASN_NameTypes }ASNameTypes ;
typedef enum { ASA_Left = 0, ASA_Center, ASA_Right, ASA_AligmentTypes } ASAligmentTypes;

typedef struct WinListConfig
{
#define 	ASWL_RowsFirst 		(0x01<<16)
	ASFlagType	flags ;
	int anchor_x, anchor_y ;
	int gravity ;
	unsigned int min_width, min_height ;
	unsigned int max_width, max_height ;
	unsigned int min_rows, min_columns ;
	unsigned int max_rows, max_columns ;
	unsigned int max_col_width ;
	
	char *unfocused_style ;
	char *focused_style ;
	char *sticky_style ;
	
	ASNameTypes     show_name_type ; /* 0, 1, 2, 3 */
	ASAligmentTypes name_aligment ;  

}WinListConfig;

WinListConfig Config = { 0, 0, 0, 0, 
                    	  0, 0, 0, 0, 
						  1, 0, 1, 0,
						  0,
						  "unfocused_window_style",
						  "focused_window_style",
						  "sticky_window_style",
						  ASN_Name,
						  ASA_Left };
/**********************************************************************/

void
usage (void)
{
  printf ("Usage:\n"
  	      "%s [-d|--display display_name] [-v|--version] [-h|--help]\n", MyName);
  exit (0);
}

void GetBaseOptions (const char *filename);
void GetOptions (const char *filename);
void process_message (unsigned long type, unsigned long *body);
void DispatchEvent (XEvent * Event);
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
	int i ;
	char *global_config_file = NULL;
	int x_fd ;
	int as_fd[2] ;
	Window w ;
	
	set_application_name(argv[0]);
	SetMyName (argv[0]);
	
	set_signal_handler( SIGSEGV );
	set_output_threshold( OUTPUT_LEVEL_PROGRESS );

	i = ProcessModuleArgs (argc, argv, &global_config_file, NULL, NULL, usage);
	
#ifdef I18N
	if (setlocale (LC_ALL, AFTER_LOCALE) == NULL)
  		show_error("cannot set locale.");
#endif

	x_fd = ConnectX (&Scr, display_name, PropertyChangeMask);
	if( !x_fd ) 
		return 1 ;

	as_fd[0] = as_fd[1] = 
	        ConnectAfterStep (M_ADD_WINDOW |
		  					  M_CONFIGURE_WINDOW |
							  M_DESTROY_WINDOW |
							  M_FOCUS_CHANGE |
							  M_ICONIFY |
							  M_ICON_LOCATION |
							  M_DEICONIFY |
							  M_ICON_NAME |
							  M_WINDOW_NAME |
							  M_END_WINDOWLIST);
	if( !as_fd[0] ) 
		return 1 ;

	intern_hint_atoms ();
	intern_wmprop_atoms ();

	/* Request a list of all windows, while we load our config */
    SendInfo (as_fd, "Send_WindowList", 0);
	
	LoadBaseConfig (global_config_file, GetBaseOptions);
    LoadConfig (global_config_file, "winlist", GetOptions);
	
	WinList = create_asbidirlist(destroy_winlist_button);
	WinListWindow = w = make_winlist_window();
	WinListCanvas = create_ascanvas( WinListWindow );

	/* And at long last our main loop : */
	while (1)
    {
        XEvent event;
		if (!My_XNextEvent (dpy, x_fd, as_fd[0], process_message, &event))
  			timer_handle ();	/* handle timeout events */
		else
  		{
    		balloon_handle_event (&event);
    		DispatchEvent (&event);
  		}
    }

	return 0 ;
}

void
DeadPipe (int nonsense)
{
#ifdef DEBUG_ALLOCS
	int i;
/* normally, we let the system clean up, but when auditing time comes
 * around, it's best to have the books in order... */
    balloon_init (1);
	if( WinList ) 
	{
		destroy_asbidirlist( &WinList );
	}
	if( WinListCanvas )
		destroy_ascanvas( &WinListCanvas );
		
	if( Base ) 
		DestroyBaseConfig(&Base);

    {
    	GC foreGC, backGC, reliefGC, shadowGC;

    	mystyle_get_global_gcs (mystyle_first, &foreGC, &backGC, &reliefGC, &shadowGC);
    	while (mystyle_first)
			mystyle_delete (mystyle_first);
    	XFreeGC (dpy, foreGC);
    	XFreeGC (dpy, backGC);
    	XFreeGC (dpy, reliefGC);
    	XFreeGC (dpy, shadowGC);
    }
	

    print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

    XFlush (dpy);			/* need this for SetErootPixmap to take effect */
	XCloseDisplay (dpy);		/* need this for SetErootPixmap to take effect */
    exit (0);
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
	
	Scr.image_manager = create_image_manager( NULL, 2.2, Base->pixmap_path, Base->icon_path, NULL );
//	Scr.font_manager = create_font_manager( NULL, NULL );
}

void
GetOptions (const char *filename)
{
	mystyle_get_property (dpy, Scr.Root, _AS_STYLE, XA_INTEGER);
	/* TODO: implement some options here : */

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
		ASTBarData *tbar = wd?wd->data:NULL;
		WindowPacketResult res ;
		
		
		show_progress( "message %X window %X data %p", type, body[0], wd );
		res = handle_window_packet( type, body, &wd );
		if( res == WP_DataCreated ) 
			add_winlist_button( tbar, wd );
		else if( res == WP_DataChanged )
			refresh_winlist_button( tbar, wd );
		else if( res == WP_DataDeleted )
			delete_winlist_button( tbar, wd );
	}
}

void
DispatchEvent (XEvent * Event)
{
    switch (Event->type)
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

/********************************************************************/
/* showing our main window :                                        */
/********************************************************************/
Window 
make_winlist_window()
{
	Window        w;
	XSizeHints    shints;
	ExtendedWMHints extwm_hints ;

	w = create_visual_window( Scr.asv, Scr.Root, Config.anchor_x, Config.anchor_y, WinListState.width, WinListState.height, 0, InputOutput, 0, NULL);
	set_client_names( w, MyName, MyName, AS_MODULE_CLASS, MyName );

	shints.flags = USPosition|USSize|PMinSize|PMaxSize|PBaseSize;
	shints.min_width = shints.min_height = 4;
	shints.max_width = (Config.max_width>0)?Config.max_width:Scr.MyDisplayWidth;
	shints.max_height = (Config.max_height>0)?Config.max_height:Scr.MyDisplayHeight;
	shints.base_width = shints.base_height = 4;

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
	switch( Config.show_name_type )
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
	unsigned int allowed_max_width = (Config.max_width==0)?Scr.MyDisplayWidth:Config.max_width ;
	unsigned int allowed_max_height = (Config.max_height==0)?Scr.MyDisplayHeight:Config.max_height ;
	unsigned int max_col_width = (Config.max_col_width==0)?Scr.MyDisplayWidth:Config.max_col_width ;
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
LOCAL_DEBUG_OUT( "Tbar name \"%s\" width is %d, height is %d", tbar->label_text, width, height );			
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

	if( get_flags(Config.flags, ASWL_RowsFirst) )
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
	unsigned int allowed_max_width = (Config.max_width==0)?Scr.MyDisplayWidth:Config.max_width ;
	unsigned int allowed_max_height = (Config.max_height==0)?Scr.MyDisplayHeight:Config.max_height ;
	
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
			if( get_flags(Config.flags, ASWL_RowsFirst) )
			{
				if( next_x+max_width > allowed_max_width )
					if( curr_row  < Config.max_rows ) 
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
					if( curr_col  < Config.max_columns )
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
	set_astbar_style( tbar, BAR_STATE_FOCUSED, Config.focused_style );

	if( get_flags(wd->flags, STICKY) )
		set_astbar_style( tbar, BAR_STATE_UNFOCUSED, Config.sticky_style );
	else
		set_astbar_style( tbar, BAR_STATE_UNFOCUSED, Config.unfocused_style );
	set_astbar_label( tbar, name);
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

