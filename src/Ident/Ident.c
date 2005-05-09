/*
 * Copyright (c) 2004 Sasha Vasko <sasha@aftercode.net>
 * Copyright (c) 1999 Rafal Wierzbicki <rafal@mcss.mcmaster.ca>
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (c) 1994 Nobutaka Suzuki
 * Copyright (c) 1994 Robert Nation
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

#include <X11/cursorfont.h>

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
#include "../../libAfterStep/session.h"

#include "../../libAfterConf/afterconf.h"

/**********************************************************************/
/*  AfterStep specific global variables :                             */
/**********************************************************************/
/**********************************************************************/
/*  WinList local variables :                                         */
/**********************************************************************/
typedef struct {
    char  	*property_name ;
	char 	*value ;
	ASTBarData *label_bar, *value_bar ;
	int height, label_width, value_width ;
	Bool span_cols ;
}ASProperty ;



typedef struct {
    Window       main_window;
    ASCanvas    *main_canvas;

#define MAX_PROPERTIES 256
	ASProperty	props[MAX_PROPERTIES];
	int used_props ;
}ASIdentState ;

ASIdentState IdentState = {};

IdentConfig *Config = NULL ;
struct ASDatabase    *Database = NULL;
/**********************************************************************/
Window get_target_window();
void GetBaseOptions (const char *filename);
void GetOptions (const char *filename);
void CheckConfigSanity();
void ParseDatabase ();

void HandleEvents();
void DispatchEvent (ASEvent * event);
void process_message (send_data_type type, send_data_type *body);

Window make_ident_window();
void fill_window_data();
void display_window_data();
void add_property( const char *name, const char *value, unsigned long value_encoding, Bool span_cols );
void DeadPipe(int);

int
main( int argc, char **argv )
{
    /* Save our program name - for error messages */
	set_DeadPipe_handler(DeadPipe);
    InitMyApp (CLASS_IDENT, argc, argv, NULL, NULL, 0 );
	LinkAfterStepConfig();

    set_signal_handler( SIGSEGV );

    ConnectX( ASDefaultScr, 0 );
    ConnectAfterStep (WINDOW_CONFIG_MASK |
                      WINDOW_NAME_MASK |
                      M_END_WINDOWLIST, 0);
    Config = CreateIdentConfig ();

    /* Request a list of all windows, while we load our config */
    SendInfo ("Send_WindowList", 0);

    LoadBaseConfig ( GetBaseOptions);
	LoadColorScheme();
	LoadConfig ("ident", GetOptions);
    CheckConfigSanity();
	ParseDatabase ();

	if (MyArgs.src_window == 0)
		MyArgs.src_window = get_target_window();

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

    if( IdentState.main_canvas )
        destroy_ascanvas( &IdentState.main_canvas );
    if( IdentState.main_window )
        XDestroyWindow( dpy, IdentState.main_window );
    
	FreeMyAppResources();
    
	if( Config )
        DestroyIdentConfig(Config);

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
    char buf[256];

    if( Config == NULL )
        Config = CreateIdentConfig ();

    if( Config->geometry.width <= 10 ) 
		Config->geometry.width = 512;
    if( Config->geometry.height <= 10 ) 
		Config->geometry.height = 512;

    mystyle_get_property (Scr.wmprops);

    sprintf( buf, "*%sTile", get_application_name() );
    LOCAL_DEBUG_OUT("Attempting to use style \"%s\"", buf);
    Scr.Look.MSWindow[BACK_UNFOCUSED] = mystyle_find_or_default( buf );
    LOCAL_DEBUG_OUT("Will use style \"%s\"", Scr.Look.MSWindow[BACK_UNFOCUSED]->name);
}


void
GetBaseOptions (const char *filename)
{
	ReloadASEnvironment( NULL, NULL, NULL, False, False );
}

void
GetOptions (const char *filename)
{
    START_TIME(option_time);
    IdentConfig *config = ParseIdentOptions( filename, MyName );

    if (config->style_defs)
        ProcessMyStyleDefinitions (&(config->style_defs));
    SHOW_TIME("Config parsing",option_time);
}

void
ParseDatabase ()
{
    struct name_list *list = NULL ;
	char *file = make_session_file(Session, DATABASE_FILE, False );
	
	/* memory management for parsing buffer */
	if (file == NULL)
        return ;

    list = ParseDatabaseOptions (file, "afterstep");
    if( list )
    {
        Database = build_asdb( list );
        if( is_output_level_under_threshold( OUTPUT_LEVEL_DATABASE ) )
            print_asdb( NULL, NULL, Database );
        while (list != NULL)
            delete_name_list (&(list));
    }else
        show_progress( "no database records loaded." );
    /* XResources : */
    load_user_database();
}

/****************************************************************************/
/* PROCESSING OF AFTERSTEP MESSAGES :                                       */
/****************************************************************************/
void
process_message (send_data_type type, send_data_type *body)
{
    LOCAL_DEBUG_OUT( "received message %lX", type );
	if( type == M_END_WINDOWLIST )
	{
		fill_window_data();
		display_window_data();
	}else if( (type&WINDOW_PACKET_MASK) != 0 )
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );
        WindowPacketResult res ;

		show_progress( "message %X window %X data %p", type, body[0], wd );
		res = handle_window_packet( type, body, &wd );
		if( res == WP_DataCreated )
        {
			/* we may need to translate frame window into client window, 
			 * as Window Data is hashed by client, and get_target_window 
			 * return the frame */
			if( wd->frame == MyArgs.src_window )
				MyArgs.src_window = wd->client ;
        }else if( res == WP_DataChanged )
		{	
		}else if( res == WP_DataDeleted )
		{	
		}
	}
}

void
DispatchEvent (ASEvent * event)
{
    SHOW_EVENT_TRACE(event);
    switch (event->x.type)
    {
	    case ConfigureNotify:
            {
                ASFlagType changes = handle_canvas_config( IdentState.main_canvas );
                if( changes != 0 )
                    set_root_clip_area( IdentState.main_canvas );

                if( get_flags( changes, CANVAS_RESIZED ) )
				{	
                
				}else if( changes != 0 )        /* moved - update transparency ! */
                {
					
                   /*  int i ;
                    	for( i = 0 ; i < Ident.windows_num ; ++i )
                        	update_astbar_transparency( WinListState.window_order[i]->bar, WinListState.main_canvas, False );
					*/
                }
            }
	        break;
        case ButtonPress:
            break;
        case ButtonRelease:
			break;
        case EnterNotify :
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
			handle_wmprop_event (Scr.wmprops, &(event->x));
			if( event->x.xproperty.atom == _AS_BACKGROUND )
            {
                /*int i ;*/
                LOCAL_DEBUG_OUT( "root background updated!%s","");
                safe_asimage_destroy( Scr.RootImage );
                Scr.RootImage = NULL ;
				/*
                for( i = 0 ; i < WinListState.windows_num ; ++i )
                    if( update_astbar_transparency( WinListState.window_order[i]->bar, WinListState.main_canvas, True ) )
						render_astbar( WinListState.window_order[i]->bar, WinListState.main_canvas );
				 */
		        if( is_canvas_dirty( IdentState.main_canvas ) )
				{
            		update_canvas_display( IdentState.main_canvas );
				}
            }else if( event->x.xproperty.atom == _AS_STYLE )
			{
                /*int i ;*/
				LOCAL_DEBUG_OUT( "AS Styles updated!%s","");
				mystyle_list_destroy_all(&(Scr.Look.styles_list));
				LoadColorScheme();
				CheckConfigSanity();
				/* now we need to update everything */
			}
			break;
    }
}

Window
make_ident_window( int width, int height)
{
	Window        w;
	XSizeHints    shints;
	ExtendedWMHints extwm_hints ;
	int x, y ;
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
    set_client_names( w, MyName, MyName, CLASS_IDENT, MyName );

    Scr.RootClipArea.x = x;
    Scr.RootClipArea.y = y;
    Scr.RootClipArea.width = width;
    Scr.RootClipArea.height = height;

    shints.flags = USSize|PResizeInc|PWinGravity;
    if( get_flags( Config->set_flags, IDENT_SET_GEOMETRY ) )
        shints.flags |= USPosition ;
    else
        shints.flags |= PPosition ;

    shints.width_inc = 1;
    shints.height_inc = 1;
	shints.win_gravity = Config->gravity ;

	extwm_hints.pid = getpid();
    extwm_hints.flags = EXTWM_PID|EXTWM_TypeASModule ;

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

/**********************************************************************
 * If no application window was indicated on the command line, prompt
 * the user to select one
 *********************************************************************/
Window
get_target_window ()
{
	XEvent eventp;
	int val = -10, trials;
	Window target = None;

	trials = 0;
	while ((trials < 100) && (val != GrabSuccess))
    {
		val = XGrabPointer (dpy, Scr.Root, True,
				  		   	ButtonReleaseMask,
			  				GrabModeAsync, GrabModeAsync, Scr.Root,
			  				XCreateFontCursor (dpy, XC_crosshair),
			  				CurrentTime);
      	if( val != GrabSuccess )
			sleep_a_little (100);
		trials++;
    }
  	if (val != GrabSuccess)
    {
    	show_error( "Couldn't grab the cursor!\n", MyName);
      	DeadPipe(0);
    }
  	XMaskEvent (dpy, ButtonReleaseMask, &eventp);
  	XUngrabPointer (dpy, CurrentTime);
  	ASSync(0);
  	target = eventp.xbutton.window;
	LOCAL_DEBUG_OUT( "window = %lX, root = %lX, subwindow = %lX", 
					 eventp.xbutton.window, eventp.xbutton.root, eventp.xbutton.subwindow );
  	if( eventp.xbutton.subwindow != None )
    	target = eventp.xbutton.subwindow;
	return target;
}

struct 
{
	ASFlagType flag ;
	char *name_set ;	  
	char *name_unset ;	  
} DatabaseFlagsList[] = 
{	{STYLE_STICKY, 				"Sticky", 				"Slippery"},
	{STYLE_TITLE, 				"Title", 				"NoTitle"},
	{STYLE_CIRCULATE, 			"CirculateHit", 		"CirculateSkip"},
	{STYLE_WINLIST, 			"WindowListHit", 		"WindowListSkip"},
	{STYLE_START_ICONIC, 		"StartIconic", 			"StartNormal"},
	{STYLE_ICON_TITLE, 			"IconTitle", 			"NoIconTitle"},
	{STYLE_FOCUS, 				"Focus", 				"NoFocus"},
	{STYLE_AVOID_COVER, 		"AvoidCover", 			"AllowCover"},
	{STYLE_VERTICAL_TITLE, 		"VerticalTitle", 		"HorizontalTitle"},
	{STYLE_HANDLES, 			"Handles", 				"NoHandles"},
	{STYLE_PPOSITION, 			"HonorPPosition", 		"NoPPosition"},
	{STYLE_GROUP_HINTS, 		"HonorGroupHints", 		"NoGroupHints"},
	{STYLE_TRANSIENT_HINTS, 	"HonorTransientHints", 	"NoTransientHints"},
	{STYLE_MOTIF_HINTS, 		"HonorMotifHints", 		"NoMotifHints"},
	{STYLE_GNOME_HINTS, 		"HonorGnomeHints", 		"NoGnomeHints"},
	{STYLE_EXTWM_HINTS, 		"HonorExtWMHints", 		"NoExtWMHints"},
	{STYLE_XRESOURCES_HINTS, 	"HonorXResources", 		"NoXResources"},
	{STYLE_FOCUS_ON_MAP, 		"FocusOnMap", 			"NoFocusOnMap"},
	{STYLE_LONG_LIVING,			"LongLiving",			"ShortLiving"},
	{STYLE_IGNORE_CONFIG,  		"IgnoreConfig",			"HonorConfig"},
	{0,NULL, NULL}};

const char *Gravity2text[] =
{
	"ForgetGravity",	
	"NorthWestGravity",
	"NorthGravity",
	"NorthEastGravity",
	"WestGravity",
	"CenterGravity",
	"EastGravity",
	"SouthWestGravity",
	"SouthGravity",	
	"SouthEastGravity",
	"StaticGravity"
};	 

void 
fill_window_data()
{
	ASWindowData *wd;
	INT32 encoding;
	char *name, *names[5];
	static char buf[4096], buf2[4096];
	ASRawHints    raw ;
	ASDatabaseRecord db_rec ;

	wd = fetch_window_by_id( MyArgs.src_window );
	LOCAL_DEBUG_OUT( "src_window = %lX, wd = %p", MyArgs.src_window, wd );
	encoding = AS_Text_ASCII ;
	name = get_window_name(wd, ASN_Name, &encoding );
	   
	add_property( "Window name:", name, encoding, True );
	name = get_window_name(wd, ASN_NameMatched, &encoding );
	add_property( "Window name matched:", name, encoding, True );
	name = get_window_name(wd, ASN_IconName, &encoding );
	add_property( "Icon name:", name, encoding, True );		
	name = get_window_name(wd, ASN_ResClass, &encoding );
	add_property( "Resource class:", name, encoding, False );
	name = get_window_name(wd, ASN_ResName, &encoding );
	add_property( "Resource name:", name, encoding, False );

	sprintf( buf, "%ld ( 0x%lX )", wd->client, wd->client );
	add_property("Client Window ID:", buf, AS_Text_ASCII, False);
	sprintf( buf, "%ld ( 0x%lX )", wd->frame, wd->frame );
	add_property("Frame Window ID:", buf, AS_Text_ASCII, False);
	sprintf( buf, "%ldx%ld%+ld%+ld", wd->frame_rect.width, wd->frame_rect.height, wd->frame_rect.x, wd->frame_rect.y );
	add_property("Frame Geometry:", buf, AS_Text_ASCII, False);

#define SHOW_FLAG(flags,flg)   \
	do{ if( get_flags( flags, AS_##flg)){ if( buf[0] != '\0' ) strcat( buf, ", "); strcat( buf, #flg );}}while(0)
#define SHOW_MWM_FLAG(flags,type,flg)   \
	do{ if( get_flags( flags, MWM_##type##_##flg)){ if( buf[0] != '\0' ) strcat( buf, ", "); strcat( buf, #flg );}}while(0)
#define SHOW_EXTWM_FLAG(flags,type,flg)   \
	do{ if( get_flags( flags, EXTWM_##type##flg)){ if( buf[0] != '\0' ) strcat( buf, ", "); strcat( buf, #flg );}}while(0)
#define SHOW_GNOME_FLAG(flags,type,flg)   \
	do{ if( get_flags( flags, WIN_##type##_##flg)){ if( buf[0] != '\0' ) strcat( buf, ", "); strcat( buf, #flg );}}while(0)

	buf[0] = '\0' ;
	SHOW_FLAG(wd->state_flags,Iconic);
	SHOW_FLAG(wd->state_flags,MaximizedX);
	SHOW_FLAG(wd->state_flags,MaximizedY);    
	SHOW_FLAG(wd->state_flags,Sticky);    
	SHOW_FLAG(wd->state_flags,Shaded);        
	SHOW_FLAG(wd->state_flags,Withdrawn);
	SHOW_FLAG(wd->state_flags,Dead);     
	SHOW_FLAG(wd->state_flags,Mapped);
	SHOW_FLAG(wd->state_flags,IconMapped);
	SHOW_FLAG(wd->state_flags,Visible);
	SHOW_FLAG(wd->state_flags,Shaped);
	SHOW_FLAG(wd->state_flags,ShapedDecor);
	SHOW_FLAG(wd->state_flags,ShapedIcon);
		   
	add_property("Current state flags:", buf, AS_Text_ASCII, True);
	
	buf[0] = '\0' ;
	SHOW_FLAG(wd->flags,Iconic);
	SHOW_FLAG(wd->flags,Transient);
	SHOW_FLAG(wd->flags,AcceptsFocus);
	SHOW_FLAG(wd->flags,ClickToFocus);
	SHOW_FLAG(wd->flags,Titlebar);
	SHOW_FLAG(wd->flags,VerticalTitle);
	SHOW_FLAG(wd->flags,Border);
	SHOW_FLAG(wd->flags,Handles);
	SHOW_FLAG(wd->flags,Frame);		
	SHOW_FLAG(wd->flags,SkipWinList); 
	SHOW_FLAG(wd->flags,DontCirculate); 
	SHOW_FLAG(wd->flags,AvoidCover);          
	SHOW_FLAG(wd->flags,IconTitle);
	SHOW_FLAG(wd->flags,Icon);          
	SHOW_FLAG(wd->flags,ClientIcon);
	SHOW_FLAG(wd->flags,ClientIconPixmap);    
	SHOW_FLAG(wd->flags,ClientIconPosition);
	SHOW_FLAG(wd->flags,FocusOnMap);          
	SHOW_FLAG(wd->flags,ShortLived);
	
	add_property("Current hints flags:", buf, AS_Text_ASCII, True);
	
	buf[0] = '\0' ;
	SHOW_FLAG(wd->flags,Windowbox);           
	SHOW_FLAG(wd->flags,Aspect);
	SHOW_FLAG(wd->flags,PID);
	SHOW_FLAG(wd->flags,MinSize);
	SHOW_FLAG(wd->flags,MaxSize);
	SHOW_FLAG(wd->flags,SizeInc);
	SHOW_FLAG(wd->flags,BaseSize);
	SHOW_FLAG(wd->flags,Gravity);
	SHOW_FLAG(wd->flags,Module);
	SHOW_FLAG(wd->flags,IgnoreConfigRequest);
	SHOW_FLAG(wd->flags,WMDockApp);
	
	add_property("Specified hint values:", buf, AS_Text_ASCII, True);

    if( collect_hints( ASDefaultScr, wd->client, HINT_ANY, &raw ) )
	{
		ExtendedWMHints *eh = &(raw.extwm_hints);
		GnomeHints *gh = &(raw.gnome_hints);
		
		if (raw.motif_hints)
		{
			buf[0] = '\0' ;
			if (get_flags (raw.motif_hints->flags, MWM_HINTS_INPUT_MODE))
			{
				INT32 input_mode = raw.motif_hints->inputMode;
					  
				if (input_mode == MWM_INPUT_SYSTEM_MODAL)
					add_property("Motif Input mode:", "System modal", AS_Text_ASCII, False);
				else if (input_mode == MWM_INPUT_FULL_APPLICATION_MODAL)
					add_property("Motif Input mode:", "Full application modal", AS_Text_ASCII, False);
			}

 			check_motif_hints_sanity( raw.motif_hints );

			if (get_flags (raw.motif_hints->flags, MWM_HINTS_FUNCTIONS))
			{	
				ASFlagType funcs = raw.motif_hints->functions;
				buf[0] = '\0' ;
				SHOW_MWM_FLAG(funcs,FUNC,RESIZE);
				SHOW_MWM_FLAG(funcs,FUNC,MOVE);
				SHOW_MWM_FLAG(funcs,FUNC,MINIMIZE);
				SHOW_MWM_FLAG(funcs,FUNC,MAXIMIZE);
				SHOW_MWM_FLAG(funcs,FUNC,CLOSE);
				add_property("Motif Functionality hints:", buf, AS_Text_ASCII, False);
			}
			if (get_flags (raw.motif_hints->flags, MWM_HINTS_DECORATIONS))
			{	
				ASFlagType decor = raw.motif_hints->decorations;
				buf[0] = '\0' ;
				SHOW_MWM_FLAG(decor,DECOR,BORDER);
				SHOW_MWM_FLAG(decor,DECOR,RESIZEH);
				SHOW_MWM_FLAG(decor,DECOR,TITLE);
				SHOW_MWM_FLAG(decor,DECOR,MENU);
				SHOW_MWM_FLAG(decor,DECOR,MINIMIZE);
				SHOW_MWM_FLAG(decor,DECOR,MAXIMIZE);
				add_property("Motif decor hints:", buf, AS_Text_ASCII, False);
			}
		}	 
		/* window state hints : */
		if (get_flags (eh->flags, EXTWM_StateEverything))
		{
			buf[0] = '\0' ;
			SHOW_EXTWM_FLAG(eh->flags,State,Modal );
			SHOW_EXTWM_FLAG(eh->flags,State,Sticky );
			SHOW_EXTWM_FLAG(eh->flags,State,MaximizedV );
			SHOW_EXTWM_FLAG(eh->flags,State,MaximizedH );
			SHOW_EXTWM_FLAG(eh->flags,State,Shaded );
			SHOW_EXTWM_FLAG(eh->flags,State,SkipTaskbar );
			add_property("Extended WM status flags:", buf, AS_Text_ASCII, True);
		}
		/* window type hints : */
		if (get_flags (eh->flags, EXTWM_TypeEverything))
		{
			buf[0] = '\0' ;
			SHOW_EXTWM_FLAG(eh->flags,Type,Desktop);
			SHOW_EXTWM_FLAG(eh->flags,Type,Dock);
			SHOW_EXTWM_FLAG(eh->flags,Type,Toolbar);
			SHOW_EXTWM_FLAG(eh->flags,Type,Menu);
			SHOW_EXTWM_FLAG(eh->flags,Type,Dialog);
			SHOW_EXTWM_FLAG(eh->flags,Type,Normal);
			SHOW_EXTWM_FLAG(eh->flags,Type,Utility);
			SHOW_EXTWM_FLAG(eh->flags,Type,Splash);
			add_property("Extended WM type flags:", buf, AS_Text_ASCII, True);
		}
		
		if (get_flags (eh->flags, EXTWM_PID))
		{
			sprintf( buf, "%ld", eh->pid );
			add_property("Extended WM PID:", buf, AS_Text_ASCII, False);
		}

		if (get_flags (eh->flags, EXTWM_DoesWMPing))
			add_property("Extended WM protocols:", "DoesWMPing", AS_Text_ASCII, False);
		
		if (get_flags (eh->flags, EXTWM_DESKTOP))
		{
			if (eh->desktop == 0xFFFFFFFF)
				strcpy(buf, "sticky");
			else
				sprintf( buf, "%ld", eh->desktop );
			add_property("Extended WM desktop:", buf, AS_Text_ASCII, False);
		}

		if (get_flags (gh->flags, GNOME_LAYER))
		{
			sprintf( buf, "%ld", gh->layer );
			add_property("Gnome hints layer:", buf, AS_Text_ASCII, False);
		}
		if (get_flags (gh->flags, GNOME_WORKSPACE))
		{
			sprintf( buf, "%ld", gh->workspace );
			add_property("Gnome hints desktop:", buf, AS_Text_ASCII, False);
		}
		if (get_flags (gh->flags, GNOME_STATE) && gh->state != 0)
		{
			buf[0] = '\0' ;
			SHOW_GNOME_FLAG(gh->state,STATE,STICKY);
			SHOW_GNOME_FLAG(gh->state,STATE,MINIMIZED);
			SHOW_GNOME_FLAG(gh->state,STATE,MAXIMIZED_VERT);
			SHOW_GNOME_FLAG(gh->state,STATE,MAXIMIZED_HORIZ);
			SHOW_GNOME_FLAG(gh->state,STATE,SHADED);
			add_property("Gnome state flags:", buf, AS_Text_ASCII, False);
		}

		if (get_flags (gh->flags, GNOME_HINTS) && gh->hints != 0)
		{	
			buf[0] = '\0' ;
			SHOW_GNOME_FLAG(gh->state,HINTS,SKIP_FOCUS);
			SHOW_GNOME_FLAG(gh->state,HINTS,SKIP_WINLIST);
			SHOW_GNOME_FLAG(gh->state,HINTS,SKIP_TASKBAR);
			SHOW_GNOME_FLAG(gh->state,HINTS,FOCUS_ON_CLICK);
			add_property("Gnome hints flags:", buf, AS_Text_ASCII, False);
		}
	}

	names[0] = get_window_name(wd, ASN_NameMatched, &encoding );
	names[1] = get_window_name(wd, ASN_IconName, &encoding );
	names[2] = get_window_name(wd, ASN_ResClass, &encoding );
	names[3] = get_window_name(wd, ASN_ResName, &encoding );
	names[4] = NULL ;

#define APPEND_DBSTYLE_TEXT(text) 	\
			do { if( !first ) strcat( buf, ", "); else first = False ; strcat( buf, text); } while(0)

	if ( fill_asdb_record (Database, names, &db_rec, False) != NULL )
	{
		int i ;
		add_property("Matched Styles:", "", AS_Text_ASCII, True);
		for( i = 0 ; Database->match_list[i] >= 0 ; ++i )
		{
			ASDatabaseRecord *dr = &(Database->styles_table[Database->match_list[i]]) ;
			int f ;
			Bool first = True ;
			sprintf( buf, "\"%s\" \t", dr->regexp->raw );

			for( f = 0 ; DatabaseFlagsList[f].name_set != NULL ; ++f ) 
			{	
				if( get_flags( dr->set_flags, DatabaseFlagsList[f].flag ) ) 
				{	
					name = get_flags( dr->flags, DatabaseFlagsList[f].flag )?
									DatabaseFlagsList[f].name_set : DatabaseFlagsList[f].name_unset ;
					APPEND_DBSTYLE_TEXT(name);
				}
			}	

			if( get_flags( dr->set_data_flags, STYLE_ICON ) )
			{	
				sprintf( buf2, "Icon \"%s\"", dr->icon_file );
				APPEND_DBSTYLE_TEXT(buf2);
			}
#if 1			
			if( get_flags( dr->set_data_flags, STYLE_STARTUP_DESK ) )
			{	
				sprintf( buf2, "StartsOnDesk %d", dr->desk );
				APPEND_DBSTYLE_TEXT(buf2);
			}
			if( get_flags( dr->set_data_flags, STYLE_BORDER_WIDTH ) )
			{	
				sprintf( buf2, "BorderWidth %d", dr->border_width );
				APPEND_DBSTYLE_TEXT(buf2);
			}
			if( get_flags( dr->set_data_flags, STYLE_HANDLE_WIDTH ) )
			{	
				sprintf( buf2, "HandleWidth %d", dr->resize_width );
				APPEND_DBSTYLE_TEXT(buf2);
			}
			if( get_flags( dr->set_data_flags, STYLE_DEFAULT_GEOMETRY ) )
			{	
				sprintf( buf2, "DefaultGeometry %dx%d%+d%+d", dr->default_geometry.width, dr->default_geometry.height, dr->default_geometry.x, dr->default_geometry.y );
				APPEND_DBSTYLE_TEXT(buf2);
			}
			if( get_flags( dr->set_data_flags, STYLE_VIEWPORTX ) )
			{	
				sprintf( buf2, "ViewportX %d", dr->viewport_x );
				APPEND_DBSTYLE_TEXT(buf2);
			}
			if( get_flags( dr->set_data_flags, STYLE_VIEWPORTY ) )
			{	
				sprintf( buf2, "ViewportY %d", dr->viewport_y );
				APPEND_DBSTYLE_TEXT(buf2);
			}
			if( get_flags( dr->set_data_flags, STYLE_GRAVITY ) )
			{	
				sprintf( buf2, "OverrideGravity %s", Gravity2text[dr->gravity] );
				APPEND_DBSTYLE_TEXT(buf2);
			}
			if( get_flags( dr->set_data_flags, STYLE_LAYER ) )
			{	
				sprintf( buf2, "Layer %d", dr->layer );
				APPEND_DBSTYLE_TEXT(buf2);
			}
			if( get_flags( dr->set_data_flags, STYLE_FRAME ) )
			{	
				sprintf( buf2, "Frame \"%s\"", dr->frame_name );
				APPEND_DBSTYLE_TEXT(buf2);
			}
			if( get_flags( dr->set_data_flags, STYLE_WINDOWBOX ) )
			{	
				sprintf( buf2, "WindowBox \"%s\"", dr->windowbox_name );
				APPEND_DBSTYLE_TEXT(buf2);
			}
#endif		
			add_property("Style:", buf, AS_Text_ASCII, True);
		}	 
	}	 
}

void 
display_window_data()
{
	int i, y ;
	int max_label_width = 0, max_height = 0, rows_count = 0 ;
	int total_width = 0, total_height = 0 ;
	int col = 0, line_width = 0 ;

	for( i = 0 ; i < IdentState.used_props ; ++i ) 
	{	
		int w = IdentState.props[i].label_width ;
		if( max_label_width < w ) 
			max_label_width = w ;
		if( max_height < IdentState.props[i].height )
			max_height = IdentState.props[i].height ;
		if( IdentState.props[i].span_cols )
			++col ;
		if ( ++col >= 2 )
		{
			col = 0 ;
			++rows_count ;
		}	 
	}
	total_height = rows_count * max_height ;
	col = 0 ;
	for( i = 0 ; i < IdentState.used_props ; ++i ) 
	{	
		int w = IdentState.props[i].value_width ;
		
		if( IdentState.props[i].span_cols || col == 2 )
		{
			col = IdentState.props[i].span_cols ? 1 : 0 ;

			if( total_width	< line_width ) 
				total_width	= line_width ;
			line_width = 0 ;
		}			

		line_width += w + max_label_width ;
		++col ;		
	}
	
	if( total_width	< line_width ) 
		total_width	= line_width ;

	total_width = ((total_width / 2)+5)*2 ;
		   
	IdentState.main_window = make_ident_window( total_width, total_height);
    IdentState.main_canvas = create_ascanvas( IdentState.main_window );
	handle_canvas_config( IdentState.main_canvas );
    set_root_clip_area( IdentState.main_canvas );

	y = 0 ; 
	col = 0 ;
	for( i = 0 ; i < IdentState.used_props ; ++i ) 
	{
		int w ; 
		int x  = 0;
		set_astbar_size( IdentState.props[i].label_bar, max_label_width, max_height );	 
		
		if( IdentState.props[i].span_cols )
			w = total_width ;   
		else if( i == IdentState.used_props - 1 )
			w = total_width ;   
		else if( col == 0 && IdentState.props[i+1].span_cols )
			w = total_width ;   
		else
			w = total_width/2 ;   
		
		set_astbar_size( IdentState.props[i].value_bar, w - max_label_width, max_height );	 
		
		if( w < total_width && col == 0 )
			x = w ;
		
		move_astbar( IdentState.props[i].label_bar, IdentState.main_canvas, x, y );
		move_astbar( IdentState.props[i].value_bar, IdentState.main_canvas, x+max_label_width, y );
		
		if( IdentState.props[i].span_cols )
			++col ;
		if ( ++col >= 2 )
		{	
			col = 0 ;
			y += IdentState.props[i].height ;		   
		}
		render_astbar( IdentState.props[i].label_bar, IdentState.main_canvas );
		render_astbar( IdentState.props[i].value_bar, IdentState.main_canvas );
	}
	
	if( is_canvas_dirty( IdentState.main_canvas ) )
	{
        update_canvas_display( IdentState.main_canvas );
	}
}

void 
add_property( const char *name, const char *value, unsigned long value_encoding, Bool span_cols )
{
	if( IdentState.used_props < MAX_PROPERTIES )
	{
		int tmp ;
		IdentState.props[IdentState.used_props].property_name = mystrdup(name);
		IdentState.props[IdentState.used_props].value = mystrdup(value);
		IdentState.props[IdentState.used_props].label_bar = create_astbar();
		IdentState.props[IdentState.used_props].value_bar = create_astbar();
		set_astbar_style_ptr (	IdentState.props[IdentState.used_props].label_bar, 
								BAR_STATE_UNFOCUSED, Scr.Look.MSWindow[BACK_UNFOCUSED] );
		set_astbar_style_ptr (	IdentState.props[IdentState.used_props].value_bar, 
								BAR_STATE_UNFOCUSED, Scr.Look.MSWindow[BACK_UNFOCUSED] );
		
		add_astbar_label( IdentState.props[IdentState.used_props].label_bar, 
						  0, 0, 0, ALIGN_RIGHT, 5, 2, 
						  IdentState.props[IdentState.used_props].property_name,
						  AS_Text_ASCII);		   
		add_astbar_label( IdentState.props[IdentState.used_props].value_bar, 
						  0, 0, 0, ALIGN_LEFT, 5, 2, 
						  IdentState.props[IdentState.used_props].value,
						  value_encoding);		   
		
		IdentState.props[IdentState.used_props].height = calculate_astbar_height( IdentState.props[IdentState.used_props].label_bar );
		tmp = calculate_astbar_height( IdentState.props[IdentState.used_props].value_bar );
		if( tmp > IdentState.props[IdentState.used_props].height ) 
			IdentState.props[IdentState.used_props].height = tmp ;
		
		IdentState.props[IdentState.used_props].label_width = calculate_astbar_width( IdentState.props[IdentState.used_props].label_bar) ;
		IdentState.props[IdentState.used_props].value_width = calculate_astbar_width( IdentState.props[IdentState.used_props].value_bar );

		IdentState.props[IdentState.used_props].span_cols = span_cols ;
			
		++(IdentState.used_props);
	}
}


