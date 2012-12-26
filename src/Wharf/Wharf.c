/*
 * Copyright (c) 1998 Michal Vitecek <M.Vitecek@sh.cvut.cz>
 * Copyright (c) 1998,2002 Sasha Vasko <sasha at aftercode.net>
 * Copyright (C) 1998 Ethan Fischer
 * Copyright (C) 1998 Guylhem Aznar
 * Copyright (C) 1996 Alfredo K. Kojima
 * Copyright (C) 1996 Beat Christen
 * Copyright (C) 1996 Kaj Groner
 * Copyright (C) 1996 Frank Fejes
 * Copyright (C) 1996 mj@dfv.rwth-aachen.de
 * Copyright (C) 1995 Bo Yang
 * Copyright (C) 1993 Robert Nation
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
 *
 */

#undef DO_CLOCKING
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

#include "../../libAfterStep/afterstep.h"
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/session.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/parser.h"
#include "../../libAfterStep/mystyle.h"
#include "../../libAfterStep/mystyle_property.h"
#include "../../libAfterStep/balloon.h"
#include "../../libAfterStep/aswindata.h"
#include "../../libAfterStep/decor.h"
#include "../../libAfterStep/event.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterStep/functions.h"
#include "../../libAfterStep/shape.h"
#include "../../libAfterStep/desktop_category.h"

#include "../../libAfterConf/afterconf.h"

#define UNFOCUSED_TILE_STYLE 		0
#define FOCUSED_TILE_STYLE 			1
#define UNFOCUSED_ODD_TILE_STYLE 	2
#define FOCUSED_ODD_TILE_STYLE 		3
#define WHARF_TILE_STYLES			4

#if (WHARF_TILE_STYLES>BACK_STYLES)
# warning "WHARF_TILE_STYLES exceed the size of MSWindow pointers array"
#endif  



#ifdef ENABLE_SOUND
#define WHEV_PUSH		0
#define WHEV_CLOSE_FOLDER	1
#define WHEV_OPEN_FOLDER	2
#define WHEV_CLOSE_MAIN		3
#define WHEV_OPEN_MAIN		4
#define WHEV_DROP		5
#define MAX_EVENTS		6
#endif

#define MAGIC_WHARF_BUTTON    0xA38AB110
#define MAGIC_WHARF_FOLDER    0xA38AF01D

struct ASWharfFolder;

typedef struct ASSwallowed
{
    ASCanvas *normal, *iconic;
    ASCanvas *current;                         /* one of the above */
	int pid ; 
}ASSwallowed;

typedef struct ASWharfButton
{
    unsigned long magic;
#define ASW_SwallowTarget   (0x01<<0)
#define ASW_MaxSwallow      (0x01<<1)
#define ASW_FixedWidth		(0x01<<2)
#define ASW_FixedHeight		(0x01<<3)
#define ASW_Transient		(0x01<<4)
#define ASW_Focusable		(0x01<<5)
#define ASW_NameIsUTF8		(0x01<<6)
#define ASW_CommentIsUTF8	(0x01<<7)
    ASFlagType flags;
    char        *name ;
    char        *comment ;
    ASCanvas    *canvas;
    ASSwallowed *swallowed;
    ASTBarData  *bar;

    unsigned int    desired_width, desired_height;
    /* this is where it it will actually be placed and what size it should have at the end : */
    int             folder_x, folder_y;
    unsigned int    folder_width, folder_height;

    FunctionData   *fdata[Button5];

    struct ASWharfFolder   *folder;
    struct ASWharfFolder   *parent;
}ASWharfButton;

typedef struct ASWharfFolder
{
    unsigned long magic;
#define ASW_Mapped          (0x01<<0)
#define ASW_Vertical        (0x01<<1)
#define ASW_Withdrawn       (0x01<<2)
#define ASW_NeedsShaping    (0x01<<3)
#define ASW_Shaped          (0x01<<4)
#define ASW_ReverseOrder    (0x01<<5)
#define ASW_UseBoundary     (0x01<<6)
#define ASW_AnimationPending  (0x01<<7)
#define ASW_ReverseAnimation  (0x01<<8)
    ASFlagType  flags;

    ASCanvas    *canvas;
    ASWharfButton *buttons ;
    int buttons_num;
    ASWharfButton *parent ;
	
	int gravity ;
    unsigned int total_width, total_height;    /* size calculated based on size of participating buttons */

    int animation_steps;                       /* how many steps left */
    int animation_dir;                         /* +1 or -1 */
	/* this will cache RootImage so we don't have to pull it every time we 
	 * have to be animated */
	ASImage   *root_image ; 
	XRectangle root_clip_area ;

	XRectangle boundary ;
	unsigned int animate_from_w, animate_from_h ;
	unsigned int animate_to_w, animate_to_h ;

	ASWharfButton *withdrawn_button ;

}ASWharfFolder;

typedef struct ASWharfState
{
    ASHashTable   *win2obj_xref;               /* xref of window IDs to wharf buttons and folders */
    ASWharfFolder *root_folder ;

    ASHashTable   *swallow_targets;            /* hash of buttons that needs to swallow  */

    ASWharfButton *pressed_button;
    int pressed_state;

    Bool shaped_style;

	ASImage   *withdrawn_root_image ; 
	XRectangle withdrawn_root_clip_area ;

	ASWharfFolder *root_image_folder ; /* points to a folder that owns current Scr.RootImage */

	int buttons_render_pending ;

	ASWharfButton *focused_button ;

	FunctionData   *default_action[Button5];

}ASWharfState;

ASWharfState WharfState;
WharfConfig *Config = NULL;
int Rows_override = -1 ;
int Columns_override = -1 ;

Atom _AS_WHARF_CLOSE = None ;

#define WHARF_BUTTON_EVENT_MASK   (ButtonReleaseMask |\
                                   ButtonPressMask | LeaveWindowMask | EnterWindowMask |\
                                   StructureNotifyMask | SubstructureRedirectMask )
#define WHARF_FOLDER_EVENT_MASK   (StructureNotifyMask)


void HandleEvents();
void process_message (send_data_type type, send_data_type *body);
void DispatchEvent (ASEvent * Event);
Window make_wharf_window();
void GetOptions (const char *filename);
void GetBaseOptions (const char *filename);
void CheckConfigSanity();
void SetWharfLook();

ASWharfFolder *build_wharf_folder( WharfButton *list, ASWharfButton *parent, Bool vertical );
Bool display_wharf_folder( ASWharfFolder *aswf, int left, int top, int right, int bottom );
Bool display_main_folder();
void withdraw_wharf_folder( ASWharfFolder *aswf );
static inline void withdraw_wharf_subfolders( ASWharfFolder *aswf );
void on_wharf_moveresize( ASEvent *event );
void destroy_wharf_folder( ASWharfFolder **paswf );
void on_wharf_pressed( ASEvent *event );
void release_pressure( int button );
Bool check_pending_swallow( ASWharfFolder *aswf );
void exec_pending_swallow( ASWharfFolder *aswf );
void check_swallow_window( ASWindowData *wd );
void update_wharf_folder_transprency( ASWharfFolder *aswf, Bool force );
Bool update_wharf_button_styles( ASWharfButton *aswb, Bool odd );
void update_wharf_folder_styles( ASWharfFolder *aswf, Bool force );
void on_wharf_button_confreq( ASWharfButton *aswb, ASEvent *event );
void do_wharf_animate_iter( void *vdata );
void clear_root_image_cache( ASWharfFolder *aswf );
Bool render_wharf_button( ASWharfButton *aswb );
void set_wharf_clip_area( ASWharfFolder *aswf, int x, int y );
void set_withdrawn_clip_area( ASWharfFolder *aswf, int x, int y, unsigned int w, unsigned int h );
void change_button_focus(ASWharfButton *aswb, Bool focused ); 
static Bool check_app_click	( ASWharfButton *aswb, XButtonEvent *xbtn );
void DeadPipe(int);

/***********************************************************************
 *   main - start of module
 ***********************************************************************/
int
main (int argc, char **argv)
{
    int i;
	/* Save our program name - for error messages */
	set_DeadPipe_handler(DeadPipe);
    InitMyApp (CLASS_WHARF, argc, argv, NULL, NULL, 0 );
	LinkAfterStepConfig();
    for( i = 1 ; i< argc ; ++i)
	{
		LOCAL_DEBUG_OUT( "argv[%d] = \"%s\", original argv[%d] = \"%s\"", i, argv[i], i, MyArgs.saved_argv[i]);	  
		if( argv[i] != NULL )
		{ 	
	    	if( strcmp( argv[i] , "--rows" ) == 0 && i+1 < argc &&  argv[i+1] != NULL )
				Rows_override = atoi( argv[++i] );
	    	else if( strcmp( argv[i] , "--cols" ) == 0 && i+1 < argc &&  argv[i+1] != NULL )
				Columns_override = atoi( argv[++i] );
		}
	}

    memset( &WharfState, 0x00, sizeof(WharfState));

    ConnectX( ASDefaultScr, EnterWindowMask );
	_AS_WHARF_CLOSE = XInternAtom( dpy, "_AS_WHARF_CLOSE", False );
    ConnectAfterStep (M_TOGGLE_PAGING |
                    M_NEW_DESKVIEWPORT |
                    M_END_WINDOWLIST |
                    WINDOW_CONFIG_MASK |
                    WINDOW_NAME_MASK, 0);
    Config = CreateWharfConfig ();

    LOCAL_DEBUG_OUT("parsing Options ...%s","");
    LoadBaseConfig (GetBaseOptions);
	LoadColorScheme();
    LoadConfig ("wharf", GetOptions);

    CheckConfigSanity();
	SetWharfLook();
	ReloadCategories(True);

    WharfState.root_folder = build_wharf_folder( Config->root_folder, NULL, (Config->columns > 0 ) );
	/* no longer need that stuff : */
	DestroyCategories(); 
	while (Config->root_folder)
        DestroyWharfButton (&(Config->root_folder));
		
	/* let's proceed now : */
	if( !display_main_folder() )
    {
        show_error( "main folder does not have any entries or has zero size. Aborting!");
        return 1;
    }

    /* Create a list of all windows */
    /* Request a list of all windows,
     * wait for ConfigureWindow packets */
    if( check_pending_swallow(WharfState.root_folder) )
        SendInfo ("Send_WindowList", 0);

    /* create main folder here : */

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
        while((has_x_events = XPending (dpy)))
        {
            if( ASNextEvent (&(event.x), True) )
            {
                event.client = NULL ;
                setup_asevent_from_xevent( &event );
                DispatchEvent( &event );
				timer_handle ();
            }
        }
        module_wait_pipes_input (process_message );
    }
}

void
MapConfigureNotifyLoop()
{
    ASEvent event;

	do
	{
		if( !ASCheckTypedEvent(MapNotify,&(event.x)) )
			if( !ASCheckTypedEvent(ConfigureNotify,&(event.x)) )
				return ;
        
        event.client = NULL ;
        setup_asevent_from_xevent( &event );
        DispatchEvent( &event );
        ASSync(False);
    }while(1);
}



void
DeadPipe (int nonsense)
{
	static int already_dead = False ; 
	if( already_dead ) 
		return;/* non-reentrant function ! */
	already_dead = True ;
    
	destroy_wharf_folder( &(WharfState.root_folder) );
    DestroyWharfConfig( Config );
    destroy_ashash( &(WharfState.win2obj_xref) );
    destroy_ashash( &(WharfState.swallow_targets) );
    window_data_cleanup();
    FreeMyAppResources();
#ifdef DEBUG_ALLOCS
/* normally, we let the system clean up, but when auditing time comes
 * around, it's best to have the books in order... */
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
Bool check_style_shaped(MyStyle *style)
{
	if( style == NULL ) 
		return False;
    if( style->texture_type >= TEXTURE_TEXTURED_START &&
        style->texture_type <= TEXTURE_SHAPED_PIXMAP  )
    {
        LOCAL_DEBUG_OUT( "shaped pixmap detected%s","");
        return True;
    }else if( 	style->texture_type >= TEXTURE_SCALED_PIXMAP &&
        		style->texture_type <= TEXTURE_PIXMAP  )
    {
        if( style->back_icon.image && check_asimage_alpha( Scr.asv, style->back_icon.image ) )
        {
            LOCAL_DEBUG_OUT( "transparent pixmap detected%s","");
            return True ;
        }
    }
	return False;
}	 

void
CheckConfigSanity()
{
    if( Config == NULL )
        Config = CreateWharfConfig ();

	if( MyArgs.geometry.flags != 0 ) 
		Config->geometry = MyArgs.geometry ;

	if( Rows_override >= 0 ) 
		Config->rows = Rows_override ;
	if( Columns_override >= 0 ) 
		Config->columns = Columns_override ;

    if( Config->rows <= 0 && Config->columns <= 0 )
        Config->rows = 1;

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
    show_progress( "printing wharf config : ");
    PrintWharfConfig(Config);
#endif
    
}

void 
SetWharfLook()
{
    char *buf;
	int i ;
	MyStyle *menu_folder_pixmap = NULL ;

	mystyle_get_property (Scr.wmprops);

	buf = safemalloc(strlen(get_application_name())+256 );
    sprintf( buf, "*%sTile", get_application_name() );
    Scr.Look.MSWindow[UNFOCUSED_TILE_STYLE] = mystyle_find_or_default( buf );
    LOCAL_DEBUG_OUT("Will use style \"%s\"", Scr.Look.MSWindow[UNFOCUSED_TILE_STYLE]->name);
    
	sprintf( buf, "*%sFocusedTile", get_application_name() );
    Scr.Look.MSWindow[FOCUSED_TILE_STYLE] = mystyle_find( buf );
	
	/* its actually a hack to use MSMenu, but should be fine */
	sprintf( buf, "*%sOddTile", get_application_name() );
    Scr.Look.MSWindow[UNFOCUSED_ODD_TILE_STYLE] = mystyle_find( buf );
	
	sprintf( buf, "*%sFocusedOddTile", get_application_name() );
    Scr.Look.MSWindow[FOCUSED_ODD_TILE_STYLE] = mystyle_find( buf );
	free( buf );

	menu_folder_pixmap = mystyle_find( "menu_folder_pixmap" );
	if( menu_folder_pixmap && menu_folder_pixmap->back_icon.image != NULL ) 
	{
		if( menu_folder_pixmap->back_icon.image->name == NULL ) 
		{
			release_asimage_by_name(Scr.image_manager, ASXMLVAR_MenuFolderPixmap );
			store_asimage( Scr.image_manager, menu_folder_pixmap->back_icon.image, ASXMLVAR_MenuFolderPixmap ); 
			/* and increment refcount : */
			menu_folder_pixmap->back_icon.image = dup_asimage( menu_folder_pixmap->back_icon.image );
		}
	}

	if( get_flags( Config->set_flags, WHARF_FORCE_SIZE ) )
    {
        if( Config->force_size.width == 0 )
            Config->force_size.width = 64 ;
        if( Config->force_size.height == 0 )
            Config->force_size.height = 64 ;
    }else if( !get_flags(Config->flags, WHARF_FitContents) )
    {
        if( Scr.Look.MSWindow[UNFOCUSED_TILE_STYLE]->back_icon.image != NULL )
        {
            Config->force_size.width  = Scr.Look.MSWindow[UNFOCUSED_TILE_STYLE]->back_icon.width ;
            Config->force_size.height = Scr.Look.MSWindow[UNFOCUSED_TILE_STYLE]->back_icon.height ;
        }else
        {
            Config->force_size.width  = 64;
            Config->force_size.height = 64;
        }
    }else
	{
        Config->force_size.width  = 0 ;
        Config->force_size.height = 0 ;
	}

    if( Config->CompositionMethod == 0 )
        Config->CompositionMethod = WHARF_DEFAULT_CompositionMethod;

    WharfState.shaped_style = False ;
	for( i = 0 ; i < WHARF_TILE_STYLES ; ++i ) 
		if( check_style_shaped(Scr.Look.MSWindow[i]) )
		{
			WharfState.shaped_style = True ;
			break;
		}

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
    show_progress( "printing wharf look: ");
    Print_balloonConfig ( Config->balloon_conf );
#endif

    balloon_config2look( &(Scr.Look), NULL, Config->balloon_conf, "*WharfBalloon" );
    set_balloon_look( Scr.Look.balloon_look );

	
}

void
merge_wharf_folders( WharfButton **pf1, WharfButton **pf2 )
{
    while( *pf1 )
        pf1 = &((*pf1)->next);
    *pf1 = *pf2 ;
    *pf2 = NULL ;
}

void
GetOptions (const char *filename)
{
    WharfConfig *config;
    WharfConfig *to = Config, *from;
    START_TIME(option_time);
SHOW_CHECKPOINT;
    LOCAL_DEBUG_OUT( "loading wharf config from \"%s\": ", filename);
    from = config = ParseWharfOptions (filename, MyName);
    SHOW_TIME("Config parsing",option_time);

    /* Need to merge new config with what we have already :*/
    /* now lets check the config sanity : */
    /* mixing set and default flags : */
    Config->flags = (config->flags&config->set_flags)|(Config->flags & (~config->set_flags));
    Config->set_flags |= config->set_flags;

    if( get_flags(config->set_flags, WHARF_ROWS) )
        Config->rows = config->rows;

    if( get_flags(config->set_flags, WHARF_COLUMNS) )
        Config->columns = config->columns;

    if( get_flags( config->set_flags, WHARF_GEOMETRY ) )
        merge_geometry(&(config->geometry), &(Config->geometry) );

    if( get_flags( config->set_flags, WHARF_WITHDRAW_STYLE ) )
        Config->withdraw_style = config->withdraw_style;

    if( get_flags( config->set_flags, WHARF_FORCE_SIZE ) )
        merge_geometry( &(config->force_size), &(Config->force_size));

    if( get_flags( config->set_flags, WHARF_ANIMATE_STEPS ) )
        Config->animate_steps = config->animate_steps;
    if( get_flags( config->set_flags, WHARF_ANIMATE_STEPS_MAIN ) )
        Config->animate_steps_main = config->animate_steps_main ;
    if( get_flags( config->set_flags, WHARF_ANIMATE_DELAY ) )
        Config->animate_delay = config->animate_delay;
	ASCF_MERGE_SCALAR_KEYWORD(WHARF, to, from, LabelLocation);
	ASCF_MERGE_SCALAR_KEYWORD(WHARF, to, from, AlignContents);
	ASCF_MERGE_SCALAR_KEYWORD(WHARF, to, from, Bevel);
	ASCF_MERGE_SCALAR_KEYWORD(WHARF, to, from, ShowHints );
	ASCF_MERGE_SCALAR_KEYWORD(WHARF, to, from, CompositionMethod);
	ASCF_MERGE_SCALAR_KEYWORD(WHARF, to, from, FolderOffset);
	ASCF_MERGE_SCALAR_KEYWORD(WHARF, to, from, OrthogonalFolderOffset);

/*LOCAL_DEBUG_OUT( "align_contents = %d", Config->align_contents ); */
    if( get_flags( config->set_flags, WHARF_SOUND ) )
    {
        int i ;
        for( i = 0 ; i < WHEV_MAX_EVENTS ; ++i )
        {
            set_string(&(Config->sounds[i]), mystrdup(config->sounds[i]) );
            config->sounds[i] = NULL ;
        }
    }
    /* merging folders : */

    if( config->root_folder )
        merge_wharf_folders( &(Config->root_folder), &(config->root_folder) );

    if( Config->balloon_conf )
        Destroy_balloonConfig( Config->balloon_conf );
    Config->balloon_conf = config->balloon_conf ;
    config->balloon_conf = NULL ;

    if (config->style_defs)
        ProcessMyStyleDefinitions (&(config->style_defs));

    DestroyWharfConfig (config);
	FreeSyntaxHash( &WharfFolderSyntax );
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

    SHOW_TIME("BaseConfigParsingTime",started);
}

/****************************************************************************/
/* Window ID xref :                                                         */
/****************************************************************************/
Bool
register_object( Window w, ASMagic *obj)
{
    if( WharfState.win2obj_xref == NULL )
        WharfState.win2obj_xref = create_ashash(0, NULL, NULL, NULL);

    return (add_hash_item(WharfState.win2obj_xref, AS_HASHABLE(w), obj) == ASH_Success);
}

ASMagic *
fetch_object( Window w )
{
	ASHashData hdata = {0} ;
    if( WharfState.win2obj_xref )
        if( get_hash_item( WharfState.win2obj_xref, AS_HASHABLE(w), &hdata.vptr ) != ASH_Success )
			hdata.vptr = NULL ;
    return hdata.vptr ;
}

void
unregister_object( Window w )
{
    if( WharfState.win2obj_xref )
        remove_hash_item( WharfState.win2obj_xref, AS_HASHABLE(w), NULL, False );
}

Bool
register_swallow_target( char *name, ASWharfButton *aswb)
{
    if( name && aswb )
    {
        if( WharfState.swallow_targets == NULL )
            WharfState.swallow_targets = create_ashash(0, casestring_hash_value, casestring_compare, NULL);

        return (add_hash_item(WharfState.swallow_targets, AS_HASHABLE(name), aswb) == ASH_Success);
    }
    return False;
}

ASWharfButton *
fetch_swallow_target( char *name )
{
    ASHashData hdata = {0};
    if( WharfState.swallow_targets && name )
        if( get_hash_item( WharfState.swallow_targets, AS_HASHABLE(name), &hdata.vptr ) != ASH_Success )
			hdata.vptr = NULL ;
    return hdata.vptr ;
}

void
unregister_swallow_target( char *name )
{
    if( WharfState.swallow_targets && name )
        remove_hash_item( WharfState.swallow_targets, AS_HASHABLE(name), NULL, False );
}


/****************************************************************************/
/* PROCESSING OF AFTERSTEP MESSAGES :                                       */
/****************************************************************************/
void
process_message (send_data_type type, send_data_type *body)
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
    }else if( type == M_END_WINDOWLIST )
        exec_pending_swallow( WharfState.root_folder );
}
/*************************************************************************
 * Event handling :
 *************************************************************************/
void
DispatchEvent (ASEvent * event)
{
	static Bool root_pointer_moved = True ;
	
    SHOW_EVENT_TRACE(event);
	
	if( (event->eclass & ASE_POINTER_EVENTS) != 0 && is_balloon_click( &(event->x) ) )
	{
		withdraw_balloon(NULL);
		return;
	}

    event->client = NULL ;
    switch (event->x.type)
    {
	    case ConfigureNotify:
            on_wharf_moveresize( event );
            break;
        case KeyPress :
        case KeyRelease :
			{	
			    ASMagic *obj = fetch_object( event->w ) ;
				if( obj && obj->magic == MAGIC_WHARF_BUTTON )
    			{
        			ASWharfButton *aswb = (ASWharfButton*)obj;
					if( aswb->swallowed ) 
					{
            			event->x.xkey.window = aswb->swallowed->current->w;
   	        			XSendEvent (dpy, aswb->swallowed->current->w, False, KeyPressMask|KeyReleaseMask, &(event->x));
					}
				}
			}
            break ;
        case ButtonPress:
            on_wharf_pressed( event );
            break;
        case ButtonRelease:
            release_pressure(event->x.xbutton.button);
			{
			    ASMagic *obj = fetch_object( event->w ) ;
				if( obj && obj->magic == MAGIC_WHARF_BUTTON )
    			{
        			ASWharfButton *aswb = (ASWharfButton*)obj;
					if( check_app_click	( aswb, &(event->x.xbutton) ) ) 
					{
	            		event->x.xbutton.window = aswb->swallowed->current->w;
						XSendEvent (dpy, aswb->swallowed->current->w, False, ButtonReleaseMask, &(event->x));	
						return;
					}	 
				}
			}
            break;
        case MotionNotify :
			root_pointer_moved = True ;
            break ;
        case EnterNotify :
			if( event->x.xcrossing.window == Scr.Root )
			{
				if( WharfState.focused_button ) 
					change_button_focus(WharfState.focused_button, False ); 
				withdraw_active_balloon();
				break;
			}
        case LeaveNotify :
            {
                ASMagic *obj = fetch_object( event->w ) ;
				if( WharfState.focused_button ) 
					change_button_focus(WharfState.focused_button, False ); 
                if( obj != NULL && obj->magic == MAGIC_WHARF_BUTTON )
                {
                    ASWharfButton *aswb = (ASWharfButton*)obj;
                    on_astbar_pointer_action( aswb->bar, 0, (event->x.type==LeaveNotify), root_pointer_moved);
					root_pointer_moved = False ;
					if(event->x.type == EnterNotify)
						change_button_focus(aswb, True ); 
                }
            }
            break ;
        case ConfigureRequest:
			{
            	ASMagic *obj = fetch_object( event->w ) ;
                if( obj != NULL && obj->magic == MAGIC_WHARF_BUTTON )
                {
                    ASWharfButton *aswb = (ASWharfButton*)obj;
					on_wharf_button_confreq( aswb, event );
				}
			}
            break;

	    case ClientMessage:
            {
#ifdef LOCAL_DEBUG
                char *name = XGetAtomName( dpy, event->x.xclient.message_type );
                LOCAL_DEBUG_OUT("ClientMessage(\"%s\",data=(%lX,%lX,%lX,%lX,%lX)", name, event->x.xclient.data.l[0], event->x.xclient.data.l[1], event->x.xclient.data.l[2], event->x.xclient.data.l[3], event->x.xclient.data.l[4]);
                XFree( name );
#endif
                if ( event->x.xclient.format == 32 )
				{	
					if( event->x.xclient.data.l[0] == _XA_WM_DELETE_WINDOW )
                	{
                    	DeadPipe(0);
                	}else if ( event->x.xclient.data.l[0] == _AS_WHARF_CLOSE )
                	{
						withdraw_wharf_subfolders( WharfState.root_folder );
					}
				}		  
            }
            break;
        case ReparentNotify :
            if( event->x.xreparent.parent == Scr.Root )
            {
                //sleep_a_millisec( 100 );
                //XMoveResizeWindow( dpy, event->x.xreparent.window, -10000, -10000, 1, 1 );
            }
            break ;
	    case PropertyNotify:
			handle_wmprop_event (Scr.wmprops, &(event->x));
            if( event->x.xproperty.atom == _AS_BACKGROUND )
            {
                LOCAL_DEBUG_OUT( "root background updated!%s","");
				clear_root_image_cache( WharfState.root_folder );
				if( Scr.RootImage ) 
				{	
                	safe_asimage_destroy( Scr.RootImage );
                	Scr.RootImage = NULL ;
				}
                update_wharf_folder_transprency( WharfState.root_folder, True );
            }else if( event->x.xproperty.atom == _AS_STYLE )
			{
				LOCAL_DEBUG_OUT( "AS Styles updated!%s","");
				mystyle_list_destroy_all(&(Scr.Look.styles_list));
				LoadColorScheme();
				SetWharfLook();
				reload_asimage_manager( Scr.image_manager );
				/* now we need to update everything */
				update_wharf_folder_styles( WharfState.root_folder, True );
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

/*************************************************************************/
/* Wharf buttons :                                                       */

ASCanvas*
create_wharf_folder_canvas(ASWharfFolder *aswf)
{
    static XSetWindowAttributes attr ;
    Window w ;
    ASCanvas *pc = NULL;
    unsigned long mask = CWEventMask|CWBackPixel ; /*map ; */

    attr.event_mask = WHARF_FOLDER_EVENT_MASK ;
    attr.background_pixel = Scr.asv->black_pixel ;
    attr.background_pixmap = ParentRelative ;
    if( Scr.asv->visual_info.visual != DefaultVisual( dpy, DefaultScreen(dpy)) )
        mask |= CWBackPixel ;
    w = create_visual_window( Scr.asv, Scr.Root, 0, 0, 2, 2, 0, InputOutput, mask, &attr );

#ifdef SHAPE
	if( get_flags(Config->flags, WHARF_ANIMATE ) )
	{
		XRectangle rect ;
		rect.x = rect.y = 0 ;
		rect.width = rect.height = 1 ;
		XShapeCombineRectangles ( dpy, w, ShapeBounding, 0, 0, &rect, 1, ShapeSet, Unsorted);
		aswf->animate_from_w = 1;
		aswf->animate_from_h = 1;
		set_flags(aswf->flags,ASW_AnimationPending );
	}
#endif

    register_object( w, (ASMagic*)aswf );
    pc = create_ascanvas_container(w);
    LOCAL_DEBUG_OUT("folder canvas %p created for window %lX", pc, w );
    return pc;
}


ASCanvas*
create_wharf_button_canvas(ASWharfButton *aswb, ASCanvas *parent)
{
    static XSetWindowAttributes attr ;
    Window w ;
    ASCanvas *canvas;

    attr.event_mask = WHARF_BUTTON_EVENT_MASK ;
	attr.background_pixel = Scr.asv->black_pixel ;
    w = create_visual_window( Scr.asv, parent->w, -1, -1, 1, 1, 0, InputOutput, CWEventMask|CWBackPixel, &attr );
    register_object( w, (ASMagic*)aswb );
    canvas = create_ascanvas(w);

    if( WharfState.shaped_style )
        set_flags( canvas->state, CANVAS_FORCE_MASK );
    return canvas;
}


void 
build_wharf_button_comment( ASWharfButton *aswb, WharfButton *wb, ASTBarData *bar )
{
	/* if both comment and title are present - it came from Desktop Entries and thus are 
	   encoded in UTF-8. So we should not worry.
	 */
	char *hint = NULL; 
	int hint_length = 0, exec_length = 0 ;
	char encoding = AS_Text_UTF8 ; 
	int i, pos ;

	/* first we figure out what text encoding we should use : */
	if( !get_flags( wb->set_flags, WHARF_BUTTON_TITLE_IS_UTF8 ) &&
		get_flags( Config->ShowHints, WHARF_SHOW_HINT_Name ) ) 
	{
		encoding = AS_Text_ASCII ; 
	}
	/* done with encoding - now to build actuall text */
	/* at first we determine its length : */
	if( get_flags( Config->ShowHints, WHARF_SHOW_HINT_Name ) && wb->title ) 
		hint_length += strlen(wb->title) + 3 ; 

	if( get_flags( Config->ShowHints, WHARF_SHOW_HINT_Comment ) && wb->comment ) 
		hint_length += strlen(wb->comment) + 1 ; 
	
	if( get_flags( Config->ShowHints, WHARF_SHOW_HINT_Exec ) ) 
	{
		for( i = 0 ; i < Button5 ; ++i ) 
			if( aswb->fdata[i] ) 
			{
				exec_length += 128 ; 
				if( aswb->fdata[i]->name ) 
					exec_length += 1+1+strlen(aswb->fdata[i]->name)+1 ;
				if( aswb->fdata[i]->text ) 
					exec_length += 1+strlen(aswb->fdata[i]->text)+1 ;
			}
		hint_length += exec_length ; 
	}
	if( hint_length <= 1 ) 
		return;
	/* now lets compile actuall text : */
	hint = safemalloc( hint_length );
	pos = 0 ;
	
	if( get_flags( Config->ShowHints, WHARF_SHOW_HINT_Name ) && wb->title ) 
	{
		strcpy(&(hint[pos]), wb->title); 
		while( hint[pos] ) ++pos;
	}

	if( get_flags( Config->ShowHints, WHARF_SHOW_HINT_Comment ) && wb->comment ) 
	{
		if( pos > 0 ) 
			sprintf(&(hint[pos]), " - %s", wb->comment );
		else	
			strcpy(&(hint[pos]), wb->comment); 
		while( hint[pos] ) ++pos;
	}
	
	if( get_flags( Config->ShowHints, WHARF_SHOW_HINT_Exec ) && exec_length > 0 ) 
	{
		for( i = 0 ; i < Button5 ; ++i ) 
			if( aswb->fdata[i] ) 
			{
				TermDef      *fterm;

    	        fterm = func2fterm(aswb->fdata[i]->func, True);
				sprintf (&(hint[pos]), (pos>0)?"\n%s ":"%s ", fterm->keyword);
				while( hint[pos] ) ++pos;
				if( aswb->fdata[i]->name ) 
				{
					sprintf(&(hint[pos]), "\"%s\" ", aswb->fdata[i]->name );
					while( hint[pos] ) ++pos;
				}
				if( aswb->fdata[i]->text ) 
				{
					strcpy(&(hint[pos]), aswb->fdata[i]->text );
					while( hint[pos] ) ++pos;
				}
			}
	}
	
	set_astbar_balloon( bar, 0, hint, encoding );
	free( hint );
}

ASTBarData *
build_wharf_button_tbar(ASWharfButton *aswb, WharfButton *wb)
{
    ASTBarData *bar = create_astbar();
    int icon_row = 0, icon_col = 0;
    int label_row = 0, label_col = 0;
    int label_flip = get_flags( Config->flags, WHARF_FlipLabel )?FLIP_UPSIDEDOWN:0;
    int label_align = NO_ALIGN ;
#define WLL_Vertical    (0x01<<0)
#define WLL_Opposite    (0x01<<1)
#define WLL_AlignCenter (0x01<<2)
#define WLL_AlignRight  (0x01<<3)
#define WLL_OverIcon    (0x01<<4)

    if( get_flags( Config->flags, WHARF_SHOW_LABEL ) && wb->title )
    {
        if( get_flags(Config->LabelLocation, WLL_Vertical ))
            label_flip |= FLIP_VERTICAL ;
        if( !get_flags(Config->LabelLocation, WLL_OverIcon ))
        {
            if( get_flags(Config->LabelLocation, WLL_Vertical ))
            {
                if( get_flags(Config->LabelLocation, WLL_Opposite ))
                    label_col = 1;
                else
                    icon_col = 1;
            }else if( get_flags(Config->LabelLocation, WLL_Opposite ))
                label_row = 1;
            else
                icon_row = 1;
        }
        if( get_flags(Config->LabelLocation, WLL_AlignCenter ))
            label_align = ALIGN_CENTER ;
        else if( get_flags(Config->LabelLocation, WLL_AlignRight ))
            label_align = ALIGN_RIGHT|ALIGN_TOP ;
        else
            label_align = ALIGN_LEFT|ALIGN_BOTTOM ;
    }
	if( wb->contents )
	{
		char ** icon = NULL ;
		Bool no_icon = False;
		if( wb->selected_content >= 0 && wb->selected_content < wb->contents_num )
		{	
			icon = wb->contents[wb->selected_content].icon ;
			if( icon == NULL && IsSwallowFunc(wb->contents[wb->selected_content].function->func) )
				no_icon = True;
		}
		if( icon == NULL && !no_icon)
		{
			register int i ;
			for( i = 0 ; i < wb->contents_num ; ++i )
				if( wb->contents[i].icon != NULL )
				{
					icon = wb->contents[i].icon ;
					break;
				}
		}

    	if( icon )
    	{
        	register int i = -1;
        	while( icon[++i] )
        	{
            	ASImage *im = NULL ;
            	/* load image here */
            	im = load_environment_icon ("apps", icon[i], 64 );
            	if( im )
                	add_astbar_icon( bar, icon_col, icon_row, 0, Config->AlignContents, im );
        	}
    	}
	}

    set_astbar_composition_method( bar, BAR_STATE_UNFOCUSED, Config->CompositionMethod );

	if( !get_flags( wb->set_flags, WHARF_BUTTON_TRANSIENT ) )
	{	
    	if( get_flags( Config->flags, WHARF_SHOW_LABEL ) && wb->title )
		{
        	add_astbar_label( bar, label_col, label_row, label_flip, label_align, 2, 2, 
							  wb->title,
							  get_flags( wb->set_flags, WHARF_BUTTON_TITLE_IS_UTF8 )?AS_Text_UTF8:AS_Text_ASCII );

		}
		build_wharf_button_comment( aswb, wb, bar );
	}

    LOCAL_DEBUG_OUT( "wharf bevel is %s, value 0x%lX, wharf_no_border is %s",
                        get_flags( Config->set_flags, WHARF_Bevel )? "set":"unset",
                        Config->Bevel,
                        get_flags( Config->flags, WHARF_NO_BORDER )?"set":"unset" );
    if( get_flags( Config->flags, WHARF_NO_BORDER ) )
	{	
        set_astbar_hilite( bar, BAR_STATE_UNFOCUSED, 0 );
        set_astbar_hilite( bar, BAR_STATE_FOCUSED, 0 );
    }else if( get_flags( Config->set_flags, WHARF_Bevel ) )
    {
        set_astbar_hilite( bar, BAR_STATE_UNFOCUSED, Config->Bevel );
        set_astbar_hilite( bar, BAR_STATE_FOCUSED, Config->Bevel );
    }else
	{	
       	set_astbar_hilite( bar, BAR_STATE_UNFOCUSED, RIGHT_HILITE|BOTTOM_HILITE );
		set_astbar_hilite( bar, BAR_STATE_FOCUSED, RIGHT_HILITE|BOTTOM_HILITE );
	}
    return bar;
}


/*************************************************************************/
/* Wharf folders :                                                       */

ASWharfFolder *create_wharf_folder( int button_count, ASWharfButton *parent )
{
    ASWharfFolder *aswf = NULL ;
    if( button_count > 0 )
    {
        register int i = button_count;
        aswf = safecalloc( 1, sizeof(ASWharfFolder));
        aswf->magic = MAGIC_WHARF_FOLDER ;
        aswf->buttons = safecalloc( i, sizeof(ASWharfButton));
        aswf->buttons_num = i ;
        while( --i >= 0 )
        {
            aswf->buttons[i].magic = MAGIC_WHARF_BUTTON ;
            aswf->buttons[i].parent = aswf ;
        }
        aswf->parent = parent ;
    }
    return aswf;
}

void update_wharf_button_content_from_de( WharfButtonContent *content, ASDesktopEntry *de ) 
{

	if( de->fulliconname )
	{
		content->icon = safecalloc( 2, sizeof(char*) );
		content->icon[0] = mystrdup(de->fulliconname);
		/* curr->contents[0].icon[1] is NULL - terminator */
	}
	if( content->function == NULL ) 
		content->function = desktop_entry2function( de, NULL);
	
	if( de->type == ASDE_TypeDirectory && content->function ) 
		content->function->func = F_Folder ; 
}

WharfButton *desktop_category2wharf_folder( WharfButton *owner, WharfButtonContent *content, const char *category_name, int max_depth )
{
	ASCategoryTree *ct = NULL ;  
	ASDesktopCategory *dc = NULL ;
    WharfButton *list = NULL ;
    WharfButton **tail = &list ;
    WharfButton *curr = NULL ;

	ASDesktopEntryInfo *entries ; 
	int i, entries_num ; 
	
	if( --max_depth < 0 ) 
		return NULL;

	if( (dc = name2desktop_category( category_name, &ct )) != NULL ) 
	{
		ASDesktopEntry *cat_de = fetch_desktop_entry( ct, dc->name );
		if( owner != NULL && cat_de != NULL ) 
		{
			char * utf8_title = NULL ; 
			if( content != NULL && content->icon == NULL ) 
				update_wharf_button_content_from_de( content, cat_de ); 				
			if( dup_desktop_entry_Name( cat_de, &utf8_title ) )
			{
				if( owner->title ) 
					free( owner->title ) ;
				owner->title = utf8_title ; 
				set_flags( owner->set_flags, WHARF_BUTTON_TITLE_IS_UTF8 );
			}else 
				destroy_string( &utf8_title );
			if( owner->comment ) 
				free( owner->comment ) ;
			if( dup_desktop_entry_Comment( cat_de, &(owner->comment) ) )
				set_flags( owner->set_flags, WHARF_BUTTON_COMMENT_IS_UTF8 );
		}
		
		entries = desktop_category_get_entries( ct, dc, max_depth, NULL, &entries_num);
		if( entries ) 
		{
			for( i = 0 ; i < entries_num ; ++i ) 
			{	
				ASDesktopEntry *de = entries[i].de;
				if(  de->type != ASDE_TypeApplication && 
					(de->type != ASDE_TypeDirectory || max_depth <= 0 ))
					continue;
					
				if( get_flags(de->flags, ASDE_Unavailable ) )
					continue;

				if( de->type == ASDE_TypeApplication ) 
				{
					if( max_depth > 0 )
						if( desktop_entry_in_subcategory( ct, de, entries, entries_num ) )
							continue ;
				}else if( de->type == ASDE_TypeDirectory && max_depth <= 0 )
					continue;
					
				curr = CreateWharfButton ();    
				*tail = curr ; 
				tail = &(curr->next) ; 

				if( dup_desktop_entry_Name( de, &(curr->title) ) )
					set_flags( curr->set_flags, WHARF_BUTTON_TITLE_IS_UTF8 );
				if( dup_desktop_entry_Comment( de, &(curr->comment) ) )
					set_flags( curr->set_flags, WHARF_BUTTON_COMMENT_IS_UTF8 );

				curr->contents = safecalloc( 1, sizeof(WharfButtonContent));
				curr->contents_num = 1 ; 
				update_wharf_button_content_from_de( &(curr->contents[0]), de ); 
				if( de->type == ASDE_TypeDirectory ) 
				{
					curr->folder = desktop_category2wharf_folder( NULL, NULL, de->Name, max_depth );
				}
			}
			free( entries );
		}			
	}
	if( owner != NULL ) 
	{
		*tail = owner->folder ; 
		owner->folder = list ; 
	}
	return list ; 			
}	 


ASWharfFolder *
build_wharf_folder( WharfButton *list, ASWharfButton *parent, Bool vertical )
{
    ASWharfFolder *aswf = NULL ;
    int count = 0 ;
    WharfButton *wb = list ;
    while( wb )
    {
		Bool disabled = True ;
		int i = 0 ;
		if( get_flags( wb->set_flags, WHARF_BUTTON_TRANSIENT ) )
			disabled = False ;
		else			
		{	
			for( i = 0 ; i < wb->contents_num ; ++i )
			{
				FunctionData *function = wb->contents[i].function ;
LOCAL_DEBUG_OUT( "contents %d has function %p with func = %ld", i, function, function?function->func:-1 );
	        	if( function )
				{
					int func = function->func ;
					if(func == F_DesktopEntry )
					{
						char *de_name = function->text?function->text:function->name ;
						ASDesktopEntry *de = name2desktop_entry( de_name, NULL );
						if( de == NULL ) 
						{
							disabled = True ;
							function->func = F_NOP ;
						}else
						{
							if( dup_desktop_entry_Name( de, &(wb->title) ) )
								set_flags( wb->set_flags, WHARF_BUTTON_TITLE_IS_UTF8 );
							if( dup_desktop_entry_Comment( de, &(wb->comment) ) )
								set_flags( wb->set_flags, WHARF_BUTTON_COMMENT_IS_UTF8 );

							destroy_func_data( &(wb->contents[i].function) );
							update_wharf_button_content_from_de( &(wb->contents[i]), de ); 

							function = wb->contents[i].function ;
						}
					}else if(func == F_CATEGORY || func == F_CATEGORY_TREE)
					{
						char *cat_name = function->text?function->text:function->name ;
						desktop_category2wharf_folder( wb, &(wb->contents[i]), cat_name, (func==F_CATEGORY)?1:5 );
						function->func = F_Folder ;
							
					}else
					{
						char *target = function->text;
						disabled = True ;
						if( (func < F_ExecToolStart || func > F_ExecToolEnd) &&
							  (IsSwallowFunc(func) || IsExecFunc(func)) )
						{
						}else if (func == F_ExecInTerm)
				        {
    	        			target = strstr( function->text, "-e ");
							target = target? target+3 : function->text;
						}else
							disabled = False ;

						if( disabled ) 
				   			disabled = !is_executable_in_path (target);
    					if( disabled )
						{
							wb->contents[i].unavailable = True ;
							show_warning( "Application \"%s\" cannot be found in the PATH.", target );
						}
	      		}
				}
				if( !disabled )
				{
					wb->selected_content = i ;
					break;
				}
			}
			if( wb->folder != NULL )
				disabled = False ;
		}

		if( wb->contents_num == 0 && disabled )
		{
			set_flags( wb->set_flags, WHARF_BUTTON_DISABLED );
			show_warning( "Button \"%s\" has no functions nor folder assigned to it. Button will be disabled", wb->title?wb->title:"-" );
		}else if( disabled )
		{
			set_flags( wb->set_flags, WHARF_BUTTON_DISABLED );
			show_warning( "None of Applications assigned to the button \"%s\" can be found in the PATH. Button will be disabled", wb->title?wb->title:"-" );
		}
		if( !disabled )
			++count ;
        wb = wb->next ;
    }

    if( (aswf = create_wharf_folder(count, parent)) != NULL  )
    {
		int button_no = 0 ; 
        aswf->canvas = create_wharf_folder_canvas(aswf);
        if( vertical )
            set_flags( aswf->flags, ASW_Vertical );
        else
            clear_flags( aswf->flags, ASW_Vertical );

        wb = list;
        while( wb )
        {
	        ASWharfButton *aswb = &(aswf->buttons[button_no]);
			if( get_flags( wb->set_flags, WHARF_BUTTON_DISABLED ) )
			{
				wb = wb->next ;
				continue;
			}

            aswb->name = mystrdup( wb->title );
			if( wb->contents )
			{
				int i ;
				for( i = 0 ; i < wb->contents_num ; ++i )
					if( !wb->contents[i].unavailable )
					{
						FunctionData *function = wb->contents[i].function ;
			            if( function )
            			{
							int btn = Button1 ;
							if( function->name != NULL && strlen(function->name) == 1 )
							{
								switch( function->name[0] )
								{
									case 'l' : btn = Button1 ;    break ;	
									case 'm' : btn = Button2 ;    break ;	
									case 'r' : btn = Button3 ;    break ;	
									case '4' : btn = Button4 ;	  break ;
									case '5' : btn = Button5 ;	  break ;
								}		   
							}	  
							btn -= Button1 ;
							if( aswb->fdata[btn] == NULL ) 
							{	
                				aswb->fdata[btn] = safecalloc( 1, sizeof(FunctionData) );
                				dup_func_data( aswb->fdata[btn], function );
                				if( IsSwallowFunc(aswb->fdata[btn]->func) )
                				{
                    				set_flags( aswb->flags, ASW_SwallowTarget );
                    				register_swallow_target( aswb->fdata[btn]->name, aswb );
                    				if( aswb->fdata[btn]->func == F_MaxSwallow ||
                        				aswb->fdata[btn]->func == F_MaxSwallowModule )
                        				set_flags( aswb->flags, ASW_MaxSwallow );
                				}
							}
            			}
					}
			}
			/* TODO:	Transient buttons are just spacers - they should not 
			 * 			have any balloons displayed on them , nor they should 
			 * 			interpret any clicks
			 */
			if( get_flags( wb->set_flags, WHARF_BUTTON_TRANSIENT ) )
			{
				LOCAL_DEBUG_OUT( "Markip button %p as transient", aswb );	  
				set_flags( aswb->flags, ASW_Transient );
			}

			if( get_flags( wb->set_flags, WHARF_BUTTON_SIZE ) )
			{
				if( wb->width > 0 )
					set_flags( aswb->flags, ASW_FixedWidth );
				if( wb->height > 0 )
					set_flags( aswb->flags, ASW_FixedHeight );
				aswb->desired_width  = wb->width ;
    	        aswb->desired_height = wb->height ;
			}

            aswb->canvas = create_wharf_button_canvas(aswb, aswf->canvas);
            aswb->bar = build_wharf_button_tbar(aswb, wb);

			update_wharf_button_styles( aswb, ((button_no&0x01)==0));

            if( !get_flags( aswb->flags, ASW_SwallowTarget ) )
            {
				if( aswb->desired_width == 0 )
	                aswb->desired_width = (Config->force_size.width == 0)?calculate_astbar_width( aswb->bar ) :Config->force_size.width;
				if( aswb->desired_height == 0 )
	            	aswb->desired_height = (Config->force_size.height== 0)?calculate_astbar_height( aswb->bar ):Config->force_size.height;
            }else
            {
    			if( aswb->desired_width == 0 )
	            	aswb->desired_width  = Config->force_size.width ;
    			if( aswb->desired_height == 0 )
	                aswb->desired_height = Config->force_size.height ;
            }

            if( aswb->desired_width == 0 )
                aswb->desired_width = 64 ;

            if( aswb->desired_height == 0 )
                aswb->desired_height = 64 ;

            if( wb->folder )
                aswb->folder = build_wharf_folder( wb->folder, aswb, vertical?False:True );

            ++button_no;
            wb = wb->next ;
        }
        XMapSubwindows( dpy, aswf->canvas->w );
    }
    return aswf;
}

void
destroy_wharf_folder( ASWharfFolder **paswf )
{
    ASWharfFolder *aswf ;
    if( paswf == NULL )
        return;

    if( (aswf = *paswf) == NULL )
        return;
    if( aswf->magic != MAGIC_WHARF_FOLDER )
        return;

    if( aswf->canvas )
        unmap_canvas_window( aswf->canvas );

    if( aswf->buttons )
    {
        int i = aswf->buttons_num;
        while( --i >= 0 )
        {
            ASWharfButton *aswb = &(aswf->buttons[i]);
			int i ;
			if( aswb->swallowed ) 
			{	
				send_wm_protocol_request(aswb->swallowed->current->w, _XA_WM_DELETE_WINDOW, CurrentTime);
				ASSync(False);
				sleep_a_millisec(200);
				XKillClient(dpy, aswb->swallowed->current->w );
				ASSync(False);
				sleep_a_millisec(100);
				if( aswb->swallowed->pid > 0 ) 
					kill( aswb->swallowed->pid, SIGKILL );
			}
            if( aswb->name )
                free( aswb->name );
            destroy_astbar(&(aswb->bar));
            if( aswb->canvas )
                unregister_object( aswb->canvas->w );
            destroy_ascanvas(&(aswb->canvas));
			for( i = 0 ; i < 5 ; ++i ) 
				if( aswb->fdata[i] )
            		destroy_func_data(&(aswb->fdata[i]));
            destroy_wharf_folder(&(aswb->folder));
        }
        free( aswf->buttons );
    }
    if( aswf->canvas )
        unregister_object( aswf->canvas->w );
    destroy_ascanvas(&(aswf->canvas));

    memset(aswf, 0x00, sizeof(ASWharfFolder));
    free( aswf );
    *paswf = NULL ;
}
				
void set_folder_name( ASWharfFolder *aswf, Bool withdrawn )
{	
	if( withdrawn ) 
	{	
		char *withdrawn_name = safemalloc( strlen(MyName)+ 16 );
		sprintf( withdrawn_name, "%s%s", MyName, "Withdrawn" );
		set_client_names( aswf->canvas->w, withdrawn_name, withdrawn_name, AS_MODULE_CLASS, CLASS_WHARF_WITHDRAWN );
		free( withdrawn_name );
	}else if( aswf == WharfState.root_folder ) 
	{
		set_client_names( aswf->canvas->w, MyName, MyName, AS_MODULE_CLASS, CLASS_WHARF );
	}else
	{
		
		char *folder_name ;
		
		if( aswf->parent->name ) 
		{
#define WHARF_SUBFOLDER_TEXT	"Subfolder"		
			folder_name = safemalloc(strlen(MyName)+sizeof( WHARF_SUBFOLDER_TEXT " - " )+ strlen(aswf->parent->name) + 1 );
			sprintf( folder_name, "%s" WHARF_SUBFOLDER_TEXT " - %s", MyName, aswf->parent->name );
		}else
		{
			folder_name = safemalloc(strlen(MyName)+sizeof(WHARF_SUBFOLDER_TEXT)+1);
			sprintf( folder_name, "%s" WHARF_SUBFOLDER_TEXT, MyName );
		}
		set_client_names( aswf->canvas->w, folder_name, folder_name, AS_MODULE_CLASS, CLASS_WHARF );
		free( folder_name );
	}
}

Bool
update_wharf_folder_shape( ASWharfFolder *aswf )
{
#ifdef SHAPE
    int i =  aswf->buttons_num;
    int set = 0 ;

    clear_flags( aswf->flags, ASW_Shaped);

    if( (get_flags( Config->flags, WHARF_ShapeToContents ) && get_flags( aswf->flags, ASW_NeedsShaping))||
         WharfState.shaped_style || get_flags( aswf->flags, ASW_UseBoundary) )
    {
		if( aswf->canvas->shape )
			flush_vector( aswf->canvas->shape );
		else
			aswf->canvas->shape = create_shape();

		while ( --i >= 0 )
        {
            register ASWharfButton *aswb = &(aswf->buttons[i]);
			Bool do_combine = False ; 
			LOCAL_DEBUG_OUT( "Adding shape of the button %d (%p) with geometry %dx%d%+d%+d, and geometry inside folder %dx%d%+d%+d", 
							 i, aswb, aswb->canvas->width, aswb->canvas->height, aswb->canvas->root_x, aswb->canvas->root_y,
							 aswb->folder_width, aswb->folder_height, aswb->folder_x, aswb->folder_y );
            if( aswb->canvas->width == aswb->folder_width && aswb->canvas->height == aswb->folder_height )
				do_combine = True;
            if( aswb->swallowed )
            {
				refresh_container_shape( aswb->swallowed->current );
				LOCAL_DEBUG_OUT( "$$$$$$ name = \"%s\" current pos = %+d%+d, button = %+d%+d", aswb->name, aswb->swallowed->current->root_x, aswb->swallowed->current->root_y, 
								aswb->canvas->root_x, aswb->canvas->root_y );
				combine_canvas_shape_at (aswb->canvas, aswb->swallowed->current, 
										 aswb->swallowed->current->root_x - aswb->canvas->root_x, 
										 aswb->swallowed->current->root_y - aswb->canvas->root_y );
				update_canvas_display_mask (aswb->canvas, True);
/*
 * We already combined swallowed shape with button's shape - now we need to combine 
 * this was causing wierd artifacts :  if( combine_canvas_shape_at (aswf->canvas, aswb->swallowed->current, aswb->folder_x, aswb->folder_y ) )
 */
				do_combine = True;
            }
    		if( do_combine ) 
		        if( combine_canvas_shape_at (aswf->canvas, aswb->canvas, aswb->folder_x, aswb->folder_y ) )
		            ++set;

        }
		if( get_flags( aswf->flags, ASW_UseBoundary) )
		{
			XRectangle sr ;
			int do_subtract = 0 ;
			sr = aswf->boundary ;
			LOCAL_DEBUG_OUT( "boundary = %dx%d%+d%+d, canvas = %dx%d\n", 
						     sr.width, sr.height, sr.x, sr.y, aswf->canvas->width, aswf->canvas->height ); 
			
			if( sr.width < aswf->canvas->width ) 
			{	
				if( sr.x > 0 )
				{	 
					sr.width = sr.x ;
					sr.x = 0 ;
				}else 
				{
					sr.x = sr.width ;
					sr.width = aswf->canvas->width - sr.width ;
				}
				++do_subtract ;
			}
			
			if( sr.height < aswf->canvas->height ) 
			{	
				if( sr.y > 0 ) 
				{	
					sr.height = sr.y ;
					sr.y = 0 ;
				}else 
				{
					sr.y = sr.height ;
					sr.height = aswf->canvas->height - sr.height ;
				}
				++do_subtract ;
			}
#if 0
			fprintf( stderr, "%s: substr_boundary = %dx%d%+d%+d canvas = %dx%d%+d%+d\n", 
					 __FUNCTION__, sr.width, sr.height, sr.x, sr.y,  
					 aswf->canvas->width, aswf->canvas->height, aswf->canvas->root_x, aswf->canvas->root_y );
			fprintf( stderr, "shape = %p, used = %d\n", aswf->canvas->shape, PVECTOR_USED(aswf->canvas->shape) );
#endif			
			if( sr.width > 0 && sr.height > 0 && do_subtract > 0 ) 
			{	
				subtract_shape_rectangle( aswf->canvas->shape, &sr, 1, 0, 0, aswf->canvas->width, aswf->canvas->height );
			}
		}	 
		
		update_canvas_display_mask (aswf->canvas, True);

        if( set > 0 )
            set_flags( aswf->flags, ASW_Shaped);
		return True;
    }
#endif
	return False;
}

void
update_wharf_folder_transprency( ASWharfFolder *aswf, Bool force )
{
	if( aswf )
	{
		int i = aswf->buttons_num;
		while( --i >= 0 )
		{
			ASWharfButton *aswb= &(aswf->buttons[i]);
			if( update_astbar_transparency( aswb->bar, aswb->canvas, force ) )
			{
				invalidate_canvas_save( aswb->canvas );
				render_wharf_button( aswb );
				update_canvas_display( aswb->canvas );
			}
			if( aswb->folder && get_flags( aswb->folder->flags, ASW_Mapped )  )
				update_wharf_folder_transprency( aswb->folder, force );
		}
		/*update_wharf_folder_shape( aswf ); */
	}
}

void 
change_button_focus(ASWharfButton *aswb, Bool focused )
{
	
	if( aswb == NULL )
		return ;
	if( focused && !get_flags( aswb->flags, ASW_Focusable) ) 
		return ;

	if( set_astbar_focused( aswb->bar, NULL, focused ) )
	{	
		if( !swap_save_canvas( aswb->canvas ) )	   
		{		
			render_wharf_button( aswb );
		}
		update_canvas_display( aswb->canvas );
		update_wharf_folder_shape( aswb->parent );
	}
	if( focused ) 
		WharfState.focused_button = aswb ;
	else if( WharfState.focused_button == aswb )
		WharfState.focused_button = NULL ;
}	 

Bool
update_wharf_button_styles( ASWharfButton *aswb, Bool odd )
{
 	MyStyle *focused_style = Scr.Look.MSWindow[odd?FOCUSED_ODD_TILE_STYLE:FOCUSED_TILE_STYLE];
  	MyStyle *unfocused_style = Scr.Look.MSWindow[odd?UNFOCUSED_ODD_TILE_STYLE:UNFOCUSED_TILE_STYLE];
	if( unfocused_style == NULL ) 
		unfocused_style = Scr.Look.MSWindow[UNFOCUSED_TILE_STYLE];
	set_flags( aswb->flags, ASW_Focusable);

	if( get_flags( Config->flags, WHARF_StretchBackground ) )
		set_flags( aswb->bar->state, BAR_FLAGS_CROP_BACK );
	else
		clear_flags( aswb->bar->state, BAR_FLAGS_CROP_BACK );

	if( focused_style == NULL ) 
	{	
		if( odd && unfocused_style == Scr.Look.MSWindow[UNFOCUSED_TILE_STYLE] ) 
			focused_style = Scr.Look.MSWindow[FOCUSED_TILE_STYLE] ;
		if( focused_style == NULL ) 
		{	
			focused_style = unfocused_style ;
			clear_flags( aswb->flags, ASW_Focusable);
		}
	}
		
	set_astbar_style_ptr( aswb->bar, -1, focused_style );
	return set_astbar_style_ptr( aswb->bar, BAR_STATE_UNFOCUSED, unfocused_style );
}

void
update_wharf_folder_styles( ASWharfFolder *aswf, Bool force )
{
	if( aswf )
	{
		int i = aswf->buttons_num;
		while( --i >= 0 )
		{
			ASWharfButton *aswb= &(aswf->buttons[i]);

			if( update_wharf_button_styles( aswb, ((i&0x01)==0) ) )
			{
				invalidate_canvas_save( aswb->canvas );
				render_wharf_button( aswb );
				update_canvas_display( aswb->canvas );
			}
			if( aswb->folder /*&& get_flags( aswb->folder->flags, ASW_Mapped )*/  )
				update_wharf_folder_styles( aswb->folder, force );
		}
		update_wharf_folder_shape( aswf );
	}
}

void
map_wharf_folder( ASWharfFolder *aswf,
                  int gravity )
{
    XSizeHints      shints;
	ExtendedWMHints extwm_hints ;
    ASFlagType protocols = 0;

	set_folder_name( aswf, False );

    shints.flags = USSize|PWinGravity;
    if( get_flags( Config->set_flags, WHARF_GEOMETRY ) || get_flags(MyArgs.geometry.flags, XValue|YValue))
        shints.flags |= USPosition ;
    else
        shints.flags |= PPosition ;

    shints.win_gravity = gravity ;
	aswf->gravity = gravity ;

	extwm_hints.pid = getpid();
    extwm_hints.flags = EXTWM_PID|EXTWM_TypeSet|EXTWM_StateSet ;
	extwm_hints.type_flags = EXTWM_TypeDock|EXTWM_TypeASModule ;
	extwm_hints.state_flags = EXTWM_StateSkipTaskbar ;
    if( aswf != WharfState.root_folder )
	{
        XSetTransientForHint(dpy, aswf->canvas->w, WharfState.root_folder->canvas->w);
		set_flags(extwm_hints.type_flags,  EXTWM_TypeDialog ) ;
    }else
	{
        protocols = AS_DoesWmDeleteWindow ;
		set_client_cmd (aswf->canvas->w);
	}

    set_client_hints( aswf->canvas->w, NULL, &shints, AS_DoesWmDeleteWindow, &extwm_hints );

//	ASSync(False);
//	sleep_a_millisec (10);
	/* showing window to let user see that we are doing something */
    map_canvas_window(aswf->canvas, True);
#ifdef SHAPE
	if( get_flags(Config->flags, WHARF_ANIMATE ) )
	{	
		XShapeCombineRectangles ( dpy, aswf->canvas->w, ShapeBounding, 0, 0, &(aswf->boundary), 1, ShapeSet, Unsorted);
		aswf->animate_from_w = 1;
		aswf->animate_from_h = 1;
		set_flags(aswf->flags,ASW_AnimationPending );
	}
#endif
    LOCAL_DEBUG_OUT( "mapping folder window for folder %p", aswf );
    /* final cleanup */
//    ASSync( False );
//    sleep_a_millisec (10);                                 /* we have to give AS a chance to spot us */
}

void
place_wharf_buttons( ASWharfFolder *aswf, int *total_width_return, int *total_height_return )
{
    int max_width = 0, max_height = 0 ;
    int x = 0, y = 0 ;
    int i;
    Bool fit_contents = get_flags(Config->flags, WHARF_FitContents);
    Bool needs_shaping = False ;
	Bool reverse_order = get_flags( aswf->flags, ASW_ReverseOrder )?aswf->buttons_num-1:-1;
	int button_offset_x = 0 ;
	int button_offset_y = 0 ;

    *total_width_return  = 0 ;
	*total_height_return = 0 ;

    LOCAL_DEBUG_OUT( "flags 0x%lX, reverse_order = %d", aswf->flags, reverse_order );
    if( get_flags( aswf->flags, ASW_Vertical ) )
    {
        int columns = (aswf == WharfState.root_folder)?Config->columns:1;
        int buttons_per_column = (aswf->buttons_num + columns - 1) / columns ;
		int bc = 0 ;

		if( buttons_per_column > Scr.MyDisplayHeight/64 ) 
		{/* too many buttons - let's add some columns, shall we */
            int avg_height = 0;
			for( i = 0 ; i < aswf->buttons_num ; ++i )
				avg_height += (!get_flags( aswf->buttons[0].flags, ASW_MaxSwallow ))?
								aswf->buttons[0].desired_height:64 ;
            avg_height = avg_height / aswf->buttons_num ;

			if( buttons_per_column > Scr.MyDisplayHeight/avg_height ) 
			{
				buttons_per_column = Scr.MyDisplayHeight/avg_height - 1 ; 
				if( buttons_per_column <= 4 ) 
					buttons_per_column = min(4, aswf->buttons_num); 
				columns = (aswf->buttons_num + buttons_per_column - 1) / buttons_per_column ;
			}	
		}
        for( i = 0 ; i < aswf->buttons_num ; ++i )
        {
            ASWharfButton *aswb = &(aswf->buttons[reverse_order>=0?reverse_order-i:i]);
            int height ;

            if( bc == 0 )
            {
                register int k = i+buttons_per_column ;
                if( k > aswf->buttons_num )
                    k = aswf->buttons_num ;
                max_width = 0;
                max_height = 0;
                while( --k >= i )
                {
                    register ASWharfButton *aswb = &(aswf->buttons[reverse_order>=0?reverse_order-k:k]);
                    if( max_width < aswb->desired_width )
                        max_width = aswb->desired_width ;
                    if( max_height < aswb->desired_height && !get_flags( aswb->flags, ASW_MaxSwallow ) )
                        max_height = aswb->desired_height ;
                }
                LOCAL_DEBUG_OUT( "max_size(%dx%d)", max_width, max_height );
				if( max_width+button_offset_x  < 0) 
					button_offset_x = 0 ;
            }
            height = (get_flags( aswb->flags, ASW_MaxSwallow ) || fit_contents )?aswb->desired_height:max_height ;
            if( get_flags(Config->flags, WHARF_ShapeToContents) )
            {
                int dx = max_width - aswb->desired_width ;
                int dy = height - aswb->desired_height ;
                if( get_flags( Config->AlignContents, ALIGN_RIGHT ) == 0 )
                    dx = 0 ;
                else if( get_flags( Config->AlignContents, ALIGN_LEFT ))
                    dx = dx>>1 ;
                if( get_flags( Config->AlignContents, ALIGN_BOTTOM ) == 0 )
                    dy = 0 ;
                else if( get_flags( Config->AlignContents, ALIGN_TOP ))
                    dy = dy>>1 ;
                aswb->folder_x = x+dx+button_offset_x ;
                aswb->folder_y = y+dy+button_offset_y;
                aswb->folder_width = aswb->desired_width ;
                aswb->folder_height = aswb->desired_height;
                if( aswb->desired_width != max_width )
                    needs_shaping = True;
            }else
            {
                aswb->folder_x = x+button_offset_x;
                aswb->folder_y = y+button_offset_y;
                aswb->folder_width = max_width ;
                aswb->folder_height = height;
            }
            moveresize_canvas( aswb->canvas, aswb->folder_x, aswb->folder_y, aswb->folder_width, aswb->folder_height );
            y += height+button_offset_y ;
            if( ++bc >= buttons_per_column )
            {
                *total_width_return += max_width+button_offset_x ;
                if( *total_height_return < y )
                    *total_height_return = y ;
                x += max_width ;
                y = 0 ;
                bc = 0;
            }
        }
        if( columns * buttons_per_column > aswf->buttons_num )
        {
            *total_width_return += max_width+button_offset_x ;
            if( *total_height_return < y )
                *total_height_return = y ;
        }

    }else
    {
        int rows = (aswf == WharfState.root_folder)?Config->rows:1;
        int buttons_per_row = (aswf->buttons_num + rows - 1) / rows, br = 0 ;

		if( buttons_per_row > Scr.MyDisplayWidth/64 ) 
		{/* too many buttons - let's add some columns, shall we */
            int avg_width = 0;
			for( i = 0 ; i < aswf->buttons_num ; ++i )
				avg_width += aswf->buttons[0].desired_width ;
            avg_width = avg_width / aswf->buttons_num ;

			if( buttons_per_row > Scr.MyDisplayWidth/avg_width ) 
			{
				buttons_per_row = Scr.MyDisplayWidth/avg_width - 1 ; 
				if( buttons_per_row <= 4 ) 
					buttons_per_row = min( 4, aswf->buttons_num ); 
				rows = (aswf->buttons_num + buttons_per_row - 1) / buttons_per_row ;
			}	
		}

        for( i = 0 ; i < aswf->buttons_num ; ++i )
        {
            ASWharfButton *aswb = &(aswf->buttons[reverse_order>=0?reverse_order-i:i]);
            int width ;

            if( br == 0 )
            {
                register int k = i+buttons_per_row ;
                if( k > aswf->buttons_num )
                    k = aswf->buttons_num ;
                max_width = 0;
                max_height = 0;
                while( --k >= i )
                {
                    register ASWharfButton *aswb = &(aswf->buttons[reverse_order>=0?reverse_order-k:k]);
                    if( max_width < aswb->desired_width && !get_flags( aswb->flags, ASW_MaxSwallow ) )
                        max_width = aswb->desired_width ;
                    if( max_height < aswb->desired_height )
                        max_height = aswb->desired_height ;
                }
                LOCAL_DEBUG_OUT( "max_size(%dx%d)", max_width, max_height );
            }

            width = (get_flags( aswb->flags, ASW_MaxSwallow )|| fit_contents )?aswb->desired_width:max_width ;
            if( get_flags(Config->flags, WHARF_ShapeToContents) )
            {
                int dx = width - aswb->desired_width ;
                int dy = max_height - aswb->desired_height ;
                if( get_flags( Config->AlignContents, ALIGN_RIGHT ) == 0 )
                    dx = 0 ;
                else if( get_flags( Config->AlignContents, ALIGN_LEFT ))
                    dx = dx>>1 ;
                if( get_flags( Config->AlignContents, ALIGN_BOTTOM ) == 0 )
                    dy = 0 ;
                else if( get_flags( Config->AlignContents, ALIGN_TOP ))
                    dy = dy>>1 ;
                aswb->folder_x = x+dx ;
                aswb->folder_y = y+dy;
                aswb->folder_width = aswb->desired_width ;
                aswb->folder_height = aswb->desired_height;
                if( aswb->desired_height != max_height )
                    needs_shaping = True;
            }else
            {
                aswb->folder_x = x ;
                aswb->folder_y = y ;
                aswb->folder_width = width ;
                aswb->folder_height = max_height;
            }
            moveresize_canvas( aswb->canvas, aswb->folder_x, aswb->folder_y, aswb->folder_width, aswb->folder_height );
            x += width;
            if( ++br >= buttons_per_row )
            {
                if( *total_width_return < x )
                    *total_width_return = x ;
                *total_height_return += max_height ;
                x = 0 ;
                y += max_height ;
                br = 0;
            }
        }
        if( rows * buttons_per_row > aswf->buttons_num )
        {
            if( *total_width_return < x )
                *total_width_return = x ;
            *total_height_return += max_height ;
        }
    }
    LOCAL_DEBUG_OUT( "total_size_return(%dx%d)", *total_width_return, *total_height_return );

    ASSync( False );
	WharfState.buttons_render_pending = aswf->buttons_num ;
    if( needs_shaping )
        set_flags( aswf->flags, ASW_NeedsShaping );
    else
        clear_flags( aswf->flags, ASW_NeedsShaping );
}


void
clamp_animation_rect( ASWharfFolder *aswf, int from_width, int from_height, int to_width, int to_height, XRectangle *rect, Bool reverse )
{
	if( get_flags( aswf->flags, ASW_Vertical ) )
	{	
		Bool down = (aswf->gravity == SouthWestGravity || aswf->gravity == SouthEastGravity) ;
		
		if( reverse ) 
			down = !down ;
			
		if( down  )
			rect->y = max(to_height,from_height) - (short)rect->height ;
		if( rect->y < 0 ) 
		{	
			rect->y = 0 ; 
			rect->height = to_height ;
		}
	}else 
	{	
		Bool right = (aswf->gravity == NorthEastGravity || aswf->gravity == SouthEastGravity) ;
		if( reverse ) 
			right = !right ;
		if( right )
			rect->x = max(to_width,from_width) - (short)rect->width ;
		if( rect->x < 0 ) 
		{	
			rect->x = 0 ; 
			rect->width = to_width ;
		}
	}
}	 

void
animate_wharf_loop(ASWharfFolder *aswf, int from_width, int from_height, int to_width, int to_height, Bool reverse )	
{
	int i, steps ;
	XRectangle rect ;

	steps = get_flags( Config->set_flags, WHARF_ANIMATE_STEPS )?Config->animate_steps:12;
	LOCAL_DEBUG_OUT( "steps = %d", steps );
	for( i = 0 ; i < steps ; ++i )
	{
		rect.x = rect.y = 0 ;		
		rect.width = get_flags( aswf->flags, ASW_Vertical )?to_width:from_width+((to_width-from_width)/steps)*(i+1) ;
		rect.height = get_flags( aswf->flags, ASW_Vertical )?from_height+((to_height-from_height)/steps)*(i+1):to_height ;

		clamp_animation_rect( aswf, from_width, from_height, to_width, to_height, &rect, reverse );
		
		LOCAL_DEBUG_OUT("boundary = %dx%d%+d%+d, canvas = %dx%d\n", 
						 rect.width, rect.height, rect.x, rect.y, 
						 aswf->canvas->width, aswf->canvas->height );
		if( rect.x+rect.width > aswf->canvas->width ||
			rect.y+rect.height > aswf->canvas->height ) 
		{
			return;			
		} 

		   
		aswf->boundary = rect ;
		update_wharf_folder_shape( aswf );
		ASSync(False);
		
		if( get_flags( Config->set_flags, WHARF_ANIMATE_DELAY ) && Config->animate_delay > 0 )
 			sleep_a_millisec(Config->animate_delay+1);	  
		else
			sleep_a_millisec(50);
	}	 

	rect.x = rect.y = 0 ;
	rect.width = to_width ;
	rect.height = to_height ;
	clamp_animation_rect( aswf, from_width, from_height, to_width, to_height, &rect, reverse );
	
	LOCAL_DEBUG_OUT("boundary = %dx%d%+d%+d, canvas = %dx%d\n", 
					rect.width, rect.height, rect.x, rect.y, aswf->canvas->width, aswf->canvas->height );

	aswf->boundary = rect ;
	update_wharf_folder_shape( aswf );
}

void
animate_wharf( ASWharfFolder *aswf, int *new_width_return, int *new_height_return )
{
    int new_width = aswf->canvas->width ;
    int new_height = aswf->canvas->height ;

    if( aswf->animation_dir < 0 )
    {
        if( get_flags( aswf->flags, ASW_Vertical ) )
        {
            if( aswf->animation_steps <= 1 )
                new_height = 0 ;
            else
            {
                new_height = (new_height*(aswf->animation_steps-1))/aswf->animation_steps ;
                if( new_height == aswf->canvas->height )
                    --new_height ;
            }
        }else
        {
            if( aswf->animation_steps <= 1 )
                new_width =0 ;
            else
            {
                new_width = (new_width*(aswf->animation_steps-1))/aswf->animation_steps;
                if( new_width == aswf->canvas->width )
                    --new_width;
            }
        }
    }else
    {
        new_width = aswf->total_width ;
        new_height = aswf->total_height ;
        if( get_flags( aswf->flags, ASW_Vertical ) )
        {
            if( aswf->animation_steps <= 1 )
                new_height = aswf->total_height ;
            else
            {
                new_height = aswf->canvas->height+((aswf->total_height - aswf->canvas->height)/aswf->animation_steps)  ;
                if( new_height == aswf->canvas->height && new_height < aswf->total_height )
                    ++new_height;
            }
        }else
        {
            if( aswf->animation_steps <= 1 )
                new_width = aswf->total_width ;
            else
            {
                new_width = aswf->canvas->width+((aswf->total_width - aswf->canvas->width)/aswf->animation_steps)  ;
                if( new_width == aswf->canvas->width && new_width < aswf->total_width )
                    ++new_width;
            }
        }
    }
    --(aswf->animation_steps);

    *new_width_return = new_width ;
    *new_height_return = new_height ;
}

Bool
display_wharf_folder( ASWharfFolder *aswf, int left, int top, int right, int bottom )
{
    Bool east  = get_flags( Config->geometry.flags, XNegative);
    Bool south = get_flags( Config->geometry.flags, YNegative);
    int x, y, width = 0, height = 0;
    int total_width = 0, total_height = 0;
	int old_x, old_y ; 
	Bool reverse = False ; 
    if( AS_ASSERT( aswf ) ||
        (get_flags( aswf->flags, ASW_Mapped ) && !get_flags( aswf->flags, ASW_Withdrawn )) )
        return False;

	if( MyArgs.gravity == SouthWestGravity || MyArgs.gravity == SouthEastGravity )
		south = True ;
	if( MyArgs.gravity == NorthEastGravity || MyArgs.gravity == SouthEastGravity )
		east = True ;

	if( aswf != WharfState.root_folder )
	{
  		if( get_flags( aswf->flags, ASW_Vertical ) )
	    {
		  	if( south )
				set_flags( aswf->flags, ASW_ReverseOrder );
	    }else
  		{
			if( east )
				set_flags( aswf->flags, ASW_ReverseOrder );
		}
	}
    place_wharf_buttons( aswf, &total_width, &total_height );

    if( total_width == 0 || total_height == 0 )
        return False;;
    if( total_width > Scr.MyDisplayWidth )
        total_width = Scr.MyDisplayWidth;
    if( total_height > Scr.MyDisplayHeight )
        total_height = Scr.MyDisplayHeight;

    aswf->total_width = total_width ;
    aswf->total_height = total_height ;

    if( left < 0 )
        east = False;
    else if( left < total_width )
        east = !( Scr.MyDisplayWidth > right && (Scr.MyDisplayWidth-right > total_width || left < Scr.MyDisplayWidth-right));

    if( east )
    {
        if( left < total_width )
            left = total_width ;
    }else if( Scr.MyDisplayWidth-right  < total_width )
        right = Scr.MyDisplayWidth - total_width ;

    if( south )
    {
        if( top < total_height )
            top = total_height ;
    }else if( Scr.MyDisplayHeight - bottom < total_height )
        bottom = Scr.MyDisplayHeight - total_width ;

    if( south && top < total_height )
        south = !( Scr.MyDisplayHeight > bottom && (Scr.MyDisplayHeight-bottom > total_height || top < Scr.MyDisplayHeight-bottom));

#if 0    
	if( aswf->animation_steps > 0 )
    {
        aswf->animation_dir = 1;
        aswf->canvas->width = aswf->canvas->height = 1 ;
        animate_wharf( aswf, &width, &height );
    }
#endif
    if( width == 0 )
        width = total_width ;
    if( height == 0 )
        height = total_height ;
    LOCAL_DEBUG_OUT( "animation_steps(%d)->size(%dx%d)->total_size(%dx%d)", aswf->animation_steps, width, height, total_width, total_height );

    if( get_flags( aswf->flags, ASW_Vertical ) )
    {
        x = east? right - width: left ;
        y = south? top - height : bottom ;
        if( top != bottom )
            y += south?-Config->FolderOffset:Config->FolderOffset ;
        if( left != right)
            x += east?-Config->OrthogonalFolderOffset:Config->OrthogonalFolderOffset ;
    }else
    {
        x = east? left - width : right ;
        y = south? bottom - height: top ;
        if( left != right)
            x += east?-Config->FolderOffset:Config->FolderOffset ;
        if( top != bottom )
            y += south?-Config->OrthogonalFolderOffset:Config->OrthogonalFolderOffset ;
    }
	LOCAL_DEBUG_OUT("calculated pos(%+d%+d), east(%d), south(%d), total_size(%dx%d)", x, y, east, south, total_width, total_height );
	if( east )
	{
		if( x + width > Scr.MyDisplayWidth )
			x = Scr.MyDisplayWidth - width ;
	}else
	{
		if( x + total_width > Scr.MyDisplayWidth )
			x = Scr.MyDisplayWidth - total_width ;
	}
	if( south )
	{
		if( y + height > Scr.MyDisplayHeight )
			y = Scr.MyDisplayHeight - height ;
	}else
	{
		if( y + total_height > Scr.MyDisplayHeight )
			y = Scr.MyDisplayHeight - total_height ;
	}
    /* if user has configured us so that we'll have to overlap ourselves -
	   then its theirs fault - we cannot account for all situations */

	LOCAL_DEBUG_OUT("corrected  pos(%+d%+d)", x, y );
    LOCAL_DEBUG_OUT( "flags 0x%lX, reverse_order = %d", aswf->flags, get_flags( aswf->flags, ASW_ReverseOrder)?aswf->buttons_num-1:-1 );

	old_x = aswf->canvas->root_x ; 
	old_y = aswf->canvas->root_y ; 
	moveresize_canvas( aswf->canvas, x, y, width, height );
	set_wharf_clip_area( aswf, left, top );
	
	aswf->animate_to_w = width;
	aswf->animate_to_h = height;
    if( get_flags( aswf->flags, ASW_Vertical ) )
		reverse = ((old_y > y && !south) || (old_y < y && south ));
	else
		reverse = ((old_x > x && !east) || (old_x < x && east ));
    
	if( get_flags(Config->flags, WHARF_ANIMATE ) )
    {
		set_flags(aswf->flags,ASW_UseBoundary|ASW_AnimationPending );
		if( reverse ) 
			set_flags( aswf->flags, ASW_ReverseAnimation );
		aswf->animate_from_w = get_flags( aswf->flags, ASW_Vertical )?aswf->canvas->width:0; 
		aswf->animate_from_h = get_flags( aswf->flags, ASW_Vertical )?0:aswf->canvas->height;
		aswf->boundary.x = aswf->boundary.y = 0 ; 
		if( get_flags( aswf->flags, ASW_Vertical ) )
		{		
			if( south )
				aswf->boundary.y = height-1 ; 
			aswf->boundary.height = 1 ;
			aswf->boundary.width = 1 ;
		}else
		{
			if( east )
				aswf->boundary.x = width-1 ; 
			aswf->boundary.height = 1 ;
			aswf->boundary.width = 1 ;

/* 			aswf->boundary.width = (aswf->gravity == NorthWestGravity || aswf->gravity == SouthWestGravity )?aswf->canvas->width:1 ;
			aswf->boundary.height = (aswf->boundary.width == 1)?aswf->canvas->height:1 ;
 */ 
		}	 
#ifdef SHAPE		
		LOCAL_DEBUG_OUT("boundary pos(%dx%d%+d%+d) shaping window %lX", aswf->boundary.width, aswf->boundary.height, aswf->boundary.x, aswf->boundary.y, aswf->canvas->w );
		/* fprintf( stderr, "setting boundary to 1x1\n" );  */
	    XShapeCombineRectangles ( dpy, aswf->canvas->w, ShapeBounding, 0, 0, &(aswf->boundary), 1, ShapeSet, Unsorted);
#endif
    }
    
	map_wharf_folder( aswf, east?(south?SouthEastGravity:NorthEastGravity):
                                 (south?SouthWestGravity:NorthWestGravity) );

	set_flags( aswf->flags, ASW_Mapped );
    clear_flags( aswf->flags, ASW_Withdrawn );
	ASSync(False);

	if( aswf->canvas->width == width && aswf->canvas->height == height && get_flags(Config->flags, WHARF_ANIMATE ))
	{
		animate_wharf_loop( aswf, aswf->animate_from_w, aswf->animate_from_h, aswf->animate_to_w, aswf->animate_to_h, reverse );
		clear_flags(aswf->flags,ASW_UseBoundary|ASW_AnimationPending );
	}	 

    return True;
}

static inline void unmap_wharf_folder( ASWharfFolder *aswf );

static inline void
unmap_wharf_subfolders( ASWharfFolder *aswf )
{
    int i = aswf->buttons_num;
    while ( --i >= 0 )
    {
        if( aswf->buttons[i].folder &&
            get_flags( aswf->buttons[i].folder->flags, ASW_Mapped ) )
            unmap_wharf_folder( aswf->buttons[i].folder );
    }
}

static inline void
unmap_wharf_folder( ASWharfFolder *aswf )
{
LOCAL_DEBUG_OUT( "unmapping canvas %p at %dx%d%+d%+d", aswf->canvas, aswf->canvas->width, aswf->canvas->height, aswf->canvas->root_x, aswf->canvas->root_y );
    unmap_canvas_window( aswf->canvas );
    /*moveresize_canvas( aswf->canvas, -1000, -1000, 1, 1 ); to make sure we get ConfigureNotify next time we map the folder again */
    clear_flags( aswf->flags, ASW_Mapped );

	unmap_wharf_subfolders( aswf );
}

void
withdraw_wharf_folder( ASWharfFolder *aswf )
{
LOCAL_DEBUG_OUT( "withdrawing folder %p", aswf );
    if( AS_ASSERT(aswf) )
        return;
LOCAL_DEBUG_OUT( "folder->flags(%lX)", aswf->flags );
    if( !get_flags( aswf->flags, ASW_Mapped ) )
        return ;

	withdraw_wharf_subfolders( aswf );
	ASSync(False);

    if( get_flags(Config->flags, WHARF_ANIMATE ) )
    {
		set_flags(aswf->flags,ASW_UseBoundary );
		aswf->boundary.x = aswf->boundary.y = 0 ; 
		aswf->boundary.width = aswf->canvas->width ;
		aswf->boundary.height = aswf->canvas->height ;
		
		if( get_flags(aswf->flags,ASW_Vertical) )
			animate_wharf_loop(aswf, aswf->canvas->width, aswf->canvas->height, aswf->canvas->width, 1, False );
		else
			animate_wharf_loop(aswf, aswf->canvas->width, aswf->canvas->height, 1, aswf->canvas->height, False );
    }
LOCAL_DEBUG_OUT( "unmapping folder %p", aswf );
    unmap_wharf_folder( aswf );
    ASSync( False );
	sleep_a_millisec( 10 );
}

static inline void
withdraw_wharf_subfolders( ASWharfFolder *aswf )
{
    int i = aswf->buttons_num;
    while ( --i >= 0 )
    {
        if( aswf->buttons[i].swallowed ) 
		{
			ASCanvas *sc = aswf->buttons[i].swallowed->current ;
			if( sc )
			{	
				XClientMessageEvent ev;
				ev.type = ClientMessage;
				ev.window = sc->w;
				ev.message_type = _AS_WHARF_CLOSE;
				ev.format = 32;
				ev.data.l[0] = _AS_WHARF_CLOSE;
				ev.data.l[1] = CurrentTime;
    			XSendEvent (dpy, sc->w, False, 0, (XEvent *) & ev);
			}
		}	 
		if( aswf->buttons[i].folder &&
            get_flags( aswf->buttons[i].folder->flags, ASW_Mapped ) )
            withdraw_wharf_folder( aswf->buttons[i].folder );
    }
}



Bool display_main_folder()
{
    int left = Config->geometry.x;
    int top = Config->geometry.y;

    if( get_flags( Config->geometry.flags, XNegative ))
        left += Scr.MyDisplayWidth ;

    if( get_flags( Config->geometry.flags, YNegative ))
        top += Scr.MyDisplayHeight ;
    return display_wharf_folder( WharfState.root_folder, left, top, left, top );
}

Bool
check_pending_swallow( ASWharfFolder *aswf )
{
    if( aswf )
    {
        int i = aswf->buttons_num ;
        /* checking buttons : */
        while( --i >= 0 )
            if( get_flags(aswf->buttons[i].flags, ASW_SwallowTarget ) )
                return True;
        /* checking subfolders : */
        i = aswf->buttons_num ;
        while( --i >= 0 )
            if( aswf->buttons[i].folder )
                if( check_pending_swallow( aswf->buttons[i].folder ) )
                    return True;
    }
    return False;
}

void
exec_pending_swallow( ASWharfFolder *aswf )
{
    if( aswf )
    {
        int i = aswf->buttons_num ;
		int sent = 0 ;
        while( --i >= 0 )
        {
            if( get_flags(aswf->buttons[i].flags, ASW_SwallowTarget ) &&
                aswf->buttons[i].swallowed == NULL )
            {
				int k ; 
				for( k = 0 ; k < 5 ; ++k ) 
                	if( aswf->buttons[i].fdata[k] && IsSwallowFunc(aswf->buttons[i].fdata[k]->func)) 
					{
                		SendCommand( aswf->buttons[i].fdata[k], 0);
						++sent ;
						break;
					}
            }
            if( aswf->buttons[i].folder )
                exec_pending_swallow( aswf->buttons[i].folder );
        }
		if( sent ) 
			sleep_a_millisec(200);/* give AS a chance to handle requests */
    }
}

void
grab_swallowed_canvas_btn( ASCanvas *canvas, int button, int non_lock_mods )
{
	register int i = 0 ;
	register unsigned int *mods = lock_mods;

	LOCAL_DEBUG_OUT( "%p,%d,0x%x", canvas, button, non_lock_mods );

	if( AS_ASSERT(canvas) )
		return;
    do
    {
        /* grab button 1 if this button performs an action */
        XGrabButton(dpy, button, mods[i]|non_lock_mods,
	                canvas->w,
                    False, ButtonPressMask | ButtonReleaseMask,
                    GrabModeAsync, GrabModeAsync, None, None);
#if !defined(NO_DEBUG_OUTPUT)			
			ASSync(False);
			fprintf( stderr, "line = %d, mods = 0x%X\n", __LINE__, mods[i] );
#endif
		if( mods[i] == 0 )
			return;
    }while (++i < MAX_LOCK_MODS );

    XGrabButton(dpy, button, non_lock_mods,
                canvas->w,
                False, ButtonPressMask | ButtonReleaseMask,
                GrabModeAsync, GrabModeAsync, None, None);
#if !defined(NO_DEBUG_OUTPUT)			   		
		ASSync(False);
		fprintf( stderr, "line = %d, mods = 0\n", __LINE__ );
#endif
}


void
grab_swallowed_canvas_btns( ASCanvas *canvas, ASWharfButton *aswb, Bool withdraw_btn )
{	
	if( aswb->folder!=NULL )
		grab_swallowed_canvas_btn( canvas, Button1, 0);

	if( withdraw_btn && aswb->parent == WharfState.root_folder )
	{	
		grab_swallowed_canvas_btn( canvas, Button3, ControlMask);
		grab_swallowed_canvas_btn( canvas, Button3, 0);
	}else if( aswb->fdata[Button3-Button1] ) 
		grab_swallowed_canvas_btn( canvas, Button3, 0);
	
	if( aswb->fdata[Button2-Button1] ) 
		grab_swallowed_canvas_btn( canvas, Button2, 0);
	if( aswb->fdata[Button4-Button1] ) 
		grab_swallowed_canvas_btn( canvas, Button4, 0);
	if( aswb->fdata[Button5-Button1] ) 
		grab_swallowed_canvas_btn( canvas, Button5, 0);
}

void
update_wharf_folder_size( ASWharfFolder *aswf)
{
    int total_width = 1, total_height = 1;
    if( !get_flags( aswf->flags, ASW_Mapped) )
		return;		
	place_wharf_buttons( aswf, &total_width, &total_height );

    if( total_width != 0 && total_height != 0 )
    {
        if( total_width > Scr.MyDisplayWidth )
            total_width = Scr.MyDisplayWidth;
        if( total_height > Scr.MyDisplayHeight )
            total_height = Scr.MyDisplayHeight;

        aswf->total_width = total_width ;
        aswf->total_height = total_height ;
    }
    resize_canvas( aswf->canvas, total_width, total_height );
}

void
send_swallowed_configure_notify(ASWharfButton *aswb)
{
LOCAL_DEBUG_CALLER_OUT( "%p,%p", aswb, aswb->swallowed );
    if( aswb->swallowed )
    {
		send_canvas_configure_notify(aswb->canvas, aswb->swallowed->current );
    }
}


int make_swallow_pad( Bool pad_before, Bool pad_after, int cell_size, int tile_size )
{
	if( !pad_before )
        return 0;
    if( !pad_after )
        return cell_size-tile_size;
    return (cell_size-tile_size)/2;
}


void
check_swallow_window( ASWindowData *wd )
{
    ASWharfFolder *aswf = NULL ;
    ASWharfButton *aswb = NULL ;
    Window w;
    int try_num = 0 ;
	Bool withdraw_btn ;
    ASCanvas *nc ;
    int swidth, sheight ;
	Window icon_w = None ;
	int user_set_height = 0, user_set_width = 0 ; 
	XWMHints *wm_hints ;
	XSizeHints *wm_normal_hints;
	
    if( wd == NULL || !get_flags( wd->state_flags, AS_Mapped))
        return;
    LOCAL_DEBUG_OUT( "name(\"%s\")->icon_name(\"%s\")->res_class(\"%s\")->res_name(\"%s\")",
                     wd->window_name, wd->icon_name, wd->res_class, wd->res_name );
    if( (aswb = fetch_swallow_target( wd->window_name )) == NULL )
        if( (aswb = fetch_swallow_target( wd->icon_name )) == NULL )
            if( (aswb = fetch_swallow_target( wd->res_class )) == NULL )
                if( (aswb = fetch_swallow_target( wd->res_name )) == NULL )
                    return ;
    LOCAL_DEBUG_OUT( "swallow target is %p, swallowed = %p", aswb, aswb->swallowed );
	aswf = aswb->parent ;
    if( aswb->swallowed != NULL )
        return;
    /* do the actuall swallowing here : */
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
    LOCAL_DEBUG_OUT( "second parent %lX, root %lX, service_win = %lX", w, Scr.Root, Scr.ServiceWin );
    if( w != Scr.Root && w != Scr.ServiceWin)
	{
		ungrab_server();
		return ;
	}
    withdraw_btn = (WITHDRAW_ON_EDGE(Config) &&
					(&(aswf->buttons[0]) == aswb || &(aswf->buttons[aswf->buttons_num-1]) == aswb)) ||
                    WITHDRAW_ON_ANY(Config) ;
    /* its ok - we can swallow it now : */
    /* create swallow object : */
    aswb->swallowed = safecalloc( 1, sizeof(ASSwallowed ));
	aswb->swallowed->pid = wd->pid ; 
    /* first thing - we reparent window and its icon if there is any */
    nc = aswb->swallowed->normal = create_ascanvas_container( wd->client );
    XReparentWindow( dpy, wd->client, aswb->canvas->w, (aswb->canvas->width - nc->width)/2, (aswb->canvas->height - nc->height)/2 );
    register_object( wd->client, (ASMagic*)aswb );
    XSelectInput (dpy, wd->client, StructureNotifyMask|LeaveWindowMask|EnterWindowMask);
	ASSync(False);
    ungrab_server();
	ASSync(False);
    sleep_a_millisec(100);
	grab_swallowed_canvas_btns( nc, aswb, withdraw_btn );

	wm_hints = XGetWMHints (dpy, wd->client);
	if( (wm_normal_hints = XAllocSizeHints()) != NULL )
	{
		long  supplied;
		if( XGetWMNormalHints (dpy, wd->client, wm_normal_hints, &supplied) == 0 ) 
		{
			XFree( wm_normal_hints );
			wm_normal_hints = NULL;
		}
	}	

    if( get_flags( wd->client_icon_flags, AS_ClientIcon ) && !get_flags( wd->client_icon_flags, AS_ClientIconPixmap) )
		icon_w = wd->icon ;
	else if( get_flags( wd->flags, AS_WMDockApp ) ) 
	{
		if (wm_hints != NULL)
		{
			if( get_flags (wm_hints->flags, IconWindowHint) )
				icon_w = wm_hints->icon_window ;
		}	
	}

	if( icon_w != None ) 
    {
        ASCanvas *ic = create_ascanvas_container( icon_w );
        aswb->swallowed->iconic = ic;
        XReparentWindow( dpy, icon_w, aswb->canvas->w, (aswb->canvas->width-ic->width)/2, (aswb->canvas->height-ic->height)/2 );
        register_object( icon_w, (ASMagic*)aswb );
        XSelectInput (dpy, icon_w, StructureNotifyMask);
		ASSync(False);
		grab_swallowed_canvas_btns( ic, aswb, withdraw_btn );
    }
    aswb->swallowed->current = aswb->swallowed->normal;
	if( aswb->swallowed->iconic != NULL )
	{	
		if( get_flags( wd->flags, AS_WMDockApp ) ||
#if 0
			(wd->res_class && strcasecmp(wd->res_class, "DockApp") == 0) || 
#endif				
			get_flags( wd->state_flags, AS_Iconic ) )
		{
			aswb->swallowed->current = aswb->swallowed->iconic ;
		}
	}
    handle_canvas_config( aswb->swallowed->current );
    LOCAL_DEBUG_OUT( "client(%lX)->icon(%lX)->current(%lX)", wd->client, icon_w, aswb->swallowed->current->w );

    if( get_flags( aswb->flags, ASW_MaxSwallow ) ||
		(Config->force_size.width == 0 && !get_flags(aswb->flags, ASW_FixedWidth)))
        aswb->desired_width = aswb->swallowed->current->width;
    if( get_flags( aswb->flags, ASW_MaxSwallow ) ||
		(Config->force_size.height == 0 && !get_flags(aswb->flags, ASW_FixedHeight)) )
        aswb->desired_height = aswb->swallowed->current->height;

	if( wm_normal_hints ) 
	{
		LOCAL_DEBUG_OUT( "wm_normal_hints->flags = 0x%lX", wm_normal_hints->flags );
		if( get_flags(wm_normal_hints->flags, USSize) )
		{
			user_set_width = wm_normal_hints->width ; 
			user_set_height = wm_normal_hints->height ; 
			if( user_set_width == 0 )
				user_set_width = aswb->swallowed->current->width ;				
			if( user_set_height == 0 )
				user_set_height = aswb->swallowed->current->height ;				
		}
	}
    swidth = aswb->swallowed->current->width ;
	if( get_flags( wd->flags, AS_MinSize ) && swidth < wd->hints.min_width)
   		swidth = wd->hints.min_width;
	else if( user_set_width > 0 )
		swidth = user_set_width ; 
	else if( get_flags( wd->flags, AS_WMDockApp ) )
		swidth = 64 ; 
	else
   		swidth = aswb->desired_width;
	if( swidth <= 2 ) 
   		swidth = aswb->desired_width;
		
    sheight = aswb->swallowed->current->height ;
	if( get_flags( wd->flags, AS_MinSize ) && sheight < wd->hints.min_height)
   		sheight = wd->hints.min_height;
	else if( user_set_height > 0 )
		sheight = user_set_height ; 
	else if( get_flags( wd->flags, AS_WMDockApp ) )
		sheight = 64 ; 
	else
   		sheight = aswb->desired_height;
	if( sheight <= 2 ) 
   		sheight = aswb->desired_height;

	LOCAL_DEBUG_OUT( "original %dx%d, user %dx%d, final %dx%d, desired %dx%d", 
						aswb->swallowed->current->width,aswb->swallowed->current->height,
						user_set_width, user_set_height,
						swidth, sheight, 
						aswb->desired_width, aswb->desired_height );
						
    moveresize_canvas( aswb->swallowed->current,
                       make_swallow_pad( get_flags(Config->AlignContents,PAD_LEFT),
                                         get_flags(Config->AlignContents,PAD_RIGHT),
                                      aswb->canvas->width, swidth      ),
                       make_swallow_pad( get_flags(Config->AlignContents,PAD_TOP),
                                   		 get_flags(Config->AlignContents,PAD_BOTTOM),
                                      aswb->canvas->height, sheight    ),
                       swidth, sheight );
	if( !get_flags( wd->client_icon_flags, AS_ClientIcon ) ) 
	{  /* workaround for broken wmdock apps that draw into icon, that is a child of a client, 
		* and at the same time not properly sized ( Just don't ask - some ppl are wierd ) */
		LOCAL_DEBUG_OUT( "wmfire workaround step1: wm_hints = %p", wm_hints );
		if( wm_hints ) 
		{
			LOCAL_DEBUG_OUT( "wmfire workaround step2: icon_window_hint = %ld, icon_window = %lX", get_flags (wm_hints->flags, IconWindowHint), wm_hints->icon_window );
			if( get_flags (wm_hints->flags, IconWindowHint) && wm_hints->icon_window != None )				   
			{	
				LOCAL_DEBUG_OUT( "wmfire workaround step3: resizing to %dx%d", swidth, sheight );
				XResizeWindow( dpy, wm_hints->icon_window, swidth, sheight );
			}
		}
	}

	if( wm_normal_hints != NULL ) 
		XFree( wm_normal_hints );
	if( wm_hints != NULL ) 
		XFree (wm_hints);			   

    map_canvas_window( aswb->swallowed->current, True );
    send_swallowed_configure_notify(aswb);
	update_wharf_folder_size( aswf );
}


/*************************************************************************/
/* Event handling                                                        */
/*************************************************************************/
void
on_wharf_button_confreq( ASWharfButton *aswb, ASEvent *event )
{
	if( aswb && aswb->swallowed )
	{
		XConfigureRequestEvent *cre = &(event->x.xconfigurerequest);
	    XWindowChanges xwc;
        unsigned long xwcm;
		int re_width = aswb->desired_width ;
		int re_height = aswb->desired_height ;

        xwcm = CWX | CWY | CWWidth | CWHeight | (cre->value_mask&CWBorderWidth);

		if( get_flags( cre->value_mask, CWWidth )  )
		{
			if( get_flags( aswb->flags, ASW_MaxSwallow ) ||
				(Config->force_size.width == 0 && !get_flags(aswb->flags, ASW_FixedWidth)))
			{
        		aswb->desired_width = cre->width;
			}
			re_width = cre->width ;
		}
		if( get_flags( cre->value_mask, CWHeight )  )
		{
	    	if( get_flags( aswb->flags, ASW_MaxSwallow ) ||
				(Config->force_size.height == 0 && !get_flags(aswb->flags, ASW_FixedHeight)) )
			{
        		aswb->desired_height = cre->height;
			}
			re_height = cre->height ;
		}

		xwc.width = min( aswb->desired_width, re_width );
   		xwc.height = min( aswb->desired_height, re_height );

		xwc.x = make_tile_pad( get_flags(Config->AlignContents,PAD_LEFT),
                               get_flags(Config->AlignContents,PAD_RIGHT),
                               aswb->canvas->width, xwc.width );
		xwc.y = make_tile_pad( get_flags(Config->AlignContents,PAD_TOP),
                               get_flags(Config->AlignContents,PAD_BOTTOM),
                               aswb->canvas->height, xwc.height );

		xwc.border_width = cre->border_width;

		LOCAL_DEBUG_OUT( "Configuring swallowed window %lX to req(%dx%d%+d%+d bw = %d)- actual(%dx%d%+d%+d), (flags=%lX)", (unsigned long)aswb->swallowed->current->w, cre->width, cre->height, cre->x, cre->y, cre->border_width, xwc.width, xwc.height, xwc.x, xwc.y, xwcm );
        XConfigureWindow (dpy, aswb->swallowed->current->w, xwcm, &xwc);
		return;
	}
}

Bool
on_wharf_button_moveresize( ASWharfButton *aswb, ASEvent *event )
{
    ASFlagType changes = handle_canvas_config (aswb->canvas );
    ASFlagType swallowed_changes = 0 ;

    if( aswb->swallowed )
    {
        ASCanvas* sc = aswb->swallowed->current;
        swallowed_changes = handle_canvas_config ( sc );
        if( get_flags(swallowed_changes, CANVAS_RESIZED ) )
        {
            update_wharf_folder_size( aswb->parent );
        }
        if( get_flags(changes, CANVAS_RESIZED ) )
        {
		    int swidth = min( aswb->desired_width, aswb->swallowed->current->width );
    		int sheight = min( aswb->desired_height, aswb->swallowed->current->height );

			moveresize_canvas( aswb->swallowed->current,
                       make_tile_pad( get_flags(Config->AlignContents,PAD_LEFT),
                                      get_flags(Config->AlignContents,PAD_RIGHT),
                                      aswb->canvas->width, swidth      ),
                       make_tile_pad( get_flags(Config->AlignContents,PAD_TOP),
                                      get_flags(Config->AlignContents,PAD_BOTTOM),
                                      aswb->canvas->height, sheight    ),
                       swidth, sheight );
        }
        if( !get_flags(swallowed_changes, CANVAS_RESIZED ) && changes != 0 )
            send_swallowed_configure_notify(aswb);
    }

    if( aswb->folder_width != aswb->canvas->width ||
        aswb->folder_height != aswb->canvas->height )
        return False;

    if( get_flags(changes, CANVAS_RESIZED ) )
        set_astbar_size( aswb->bar, aswb->canvas->width, aswb->canvas->height );

    if( changes != 0 )                         /* have to always do that whenever canvas is changed */
        update_astbar_transparency( aswb->bar, aswb->canvas, False );

    if( changes != 0 && (DoesBarNeedsRendering(aswb->bar) || is_canvas_needs_redraw( aswb->canvas )))
	{
		invalidate_canvas_save( aswb->canvas );
        render_wharf_button( aswb );
	}
#ifndef SHAPE
    swallowed_changes = 0 ;                    /* if no shaped extentions - ignore the changes */
#endif
    if( is_canvas_dirty( aswb->canvas ) || swallowed_changes != 0 )
    {
        update_canvas_display( aswb->canvas );
#ifdef SHAPE
        if( get_flags( aswb->canvas->state, CANVAS_SHAPE_SET ) && aswb->swallowed )
        {
			update_wharf_folder_shape( aswb->parent );
        }
#endif
    }
    return True;
}

void
set_wharf_clip_area( ASWharfFolder *aswf, int x, int y )
{
	if( aswf != NULL ) 
	{
		int w = aswf->total_width ;
		int h = aswf->total_height ;
		XRectangle *target_area ;
		ASImage **target_root_image ;
		
		x += (int)aswf->canvas->bw ;
		y += (int)aswf->canvas->bw ;

		LOCAL_DEBUG_OUT( "folder: %dx%d%+d%+d", w, h, x, y );

		target_area = &(aswf->root_clip_area);
		target_root_image = &(aswf->root_image);
		LOCAL_DEBUG_OUT( "target is normal (%dx%d%+d%+d)", target_area->width, target_area->height, target_area->x, target_area->y );

		if( target_area->x     != x || target_area->width != w ||
			target_area->y     != y || target_area->height != h )
		{	
			target_area->x = x ; 
			target_area->y = y ; 
			target_area->width = w ; 
			target_area->height = h ; 
			if( *target_root_image ) 
			{
				safe_asimage_destroy( *target_root_image );
				if( Scr.RootImage == *target_root_image )
					Scr.RootImage = NULL ;
				*target_root_image = NULL ;
			}	 
		}
	}
}

void
set_withdrawn_clip_area( ASWharfFolder *aswf, int x, int y, unsigned int w, unsigned int h )
{
	if( aswf != NULL ) 
	{
		XRectangle *target_area ;
		ASImage **target_root_image ;
		
		x += (int)aswf->canvas->bw ;
		y += (int)aswf->canvas->bw ;

		LOCAL_DEBUG_OUT( "folder: %dx%d%+d%+d", w, h, x, y);

		target_area = &(WharfState.withdrawn_root_clip_area);
		target_root_image = &(WharfState.withdrawn_root_image);
		LOCAL_DEBUG_OUT( "target is withdrawn (%dx%d%+d%+d)", target_area->width, target_area->height, target_area->x, target_area->y );

		if( target_area->x     != x || target_area->width != w ||
			target_area->y     != y || target_area->height != h )
		{	
			target_area->x = x ; 
			target_area->y = y ; 
			target_area->width = w ; 
			target_area->height = h ; 
			if( *target_root_image ) 
			{
				safe_asimage_destroy( *target_root_image );
				if( Scr.RootImage == *target_root_image )
					Scr.RootImage = NULL ;
				*target_root_image = NULL ;
			}	 
		}
	}
}


void 
clear_root_image_cache( ASWharfFolder *aswf )
{
	if( aswf ) 
	{
        int i = aswf->buttons_num ;
		if( aswf->root_image ) 
		{	
			safe_asimage_destroy( aswf->root_image );
			if( Scr.RootImage == aswf->root_image )
				Scr.RootImage = NULL ;
			aswf->root_image = NULL ;
		}
		while( --i >= 0 ) 
			if( aswf->buttons[i].folder != NULL ) 
		 		clear_root_image_cache( aswf->buttons[i].folder );
	}		   
}	  

Bool render_wharf_button( ASWharfButton *aswb )
{
	Bool result ; 
	ASWharfFolder *aswf = aswb->parent ;
	Bool withdrawn_root = ( aswf == WharfState.root_folder && get_flags( aswf->flags, ASW_Withdrawn) );
	if ( withdrawn_root )
	{
	 	Scr.RootImage = WharfState.withdrawn_root_image ;
		Scr.RootClipArea = WharfState.withdrawn_root_clip_area ; 
	}else
	{			  
		Scr.RootImage = aswf->root_image ; 
		Scr.RootClipArea = aswf->root_clip_area ; 
	}
	LOCAL_DEBUG_OUT( "rendering button %p for folder %p. Root Image = %p", aswb, aswf, Scr.RootImage );
	if( get_flags( aswb->bar->state, BAR_FLAGS_CROP_BACK ) )
		result = render_astbar_cached_back( aswb->bar, aswb->canvas, NULL, aswf->canvas );
	else
		result = render_astbar( aswb->bar, aswb->canvas );
	ASSync( False );
	--WharfState.buttons_render_pending ;

	if ( withdrawn_root )
	 	WharfState.withdrawn_root_image = Scr.RootImage ;
	else
		aswf->root_image  = Scr.RootImage ;
	
	return result;
}	 
	

#if 0                          /* keep here just in case */
		void 
		add_wharf_folder_to_area( ASWharfFolder *aswf, int *from_x, int *from_y, int *to_x, int *to_y )
		{
			if( aswf != NULL && get_flags( aswf->flags, ASW_Mapped ) ) 
			{
				int x1 = aswf->canvas->root_x+(int)aswf->canvas->bw ;
				int y1 = aswf->canvas->root_y+(int)aswf->canvas->bw ;
				int x2 = aswf->total_width ;
				int y2 = aswf->total_height ;
        		int i = aswf->buttons_num ;

				if( aswf->gravity == NorthEastGravity || aswf->gravity == SouthEastGravity )
					x1 = x1+aswf->canvas->width - x2 ; 
				if( aswf->gravity == SouthWestGravity || aswf->gravity == SouthEastGravity )
					y1 = y1+aswf->canvas->height - y2 ; 
				x2 += x1 ;
				y2 += y1 ;
				LOCAL_DEBUG_OUT( "folder: from %+d%+d to %+d%+d, all: from %+d%+d to %+d%+d", 
						 		x1, y1, x2, y2, *from_x, *from_y, *to_x, *to_y );
				if ( x2 > 0 && x1 < (int)Scr.MyDisplayWidth &&
			 		y2 > 0 && y1 < (int)Scr.MyDisplayHeight )
				{
					x1 = AS_CLAMP( 0, x1, Scr.MyDisplayWidth );
					x2 = AS_CLAMP( 0, x2, Scr.MyDisplayWidth );
					y1 = AS_CLAMP( 0, y1, Scr.MyDisplayHeight );
					y2 = AS_CLAMP( 0, y2, Scr.MyDisplayHeight );
					if( x1 < *from_x ) *from_x = x1 ; 				   
					if( x2 > *to_x ) *to_x = x2 ; 				   
					if( y1 < *from_y ) *from_y = y1 ; 				   
					if( y2 > *to_y ) *to_y = y2 ; 				   

					LOCAL_DEBUG_OUT( "CLAMPED: folder: from %+d%+d to %+d%+d, all: from %+d%+d to %+d%+d", 
							 		x1, y1, x2, y2, *from_x, *from_y, *to_x, *to_y );
				}
				while( --i >= 0 ) 
					if( aswf->buttons[i].folder != NULL ) 
		 				add_wharf_folder_to_area( aswf->buttons[i].folder, 
										  		from_x, from_y, to_x, to_y );
			}
		}	 

		void
		update_root_clip_area()
		{
			/* TODO: update root clip area to the max area occupied by all mapped folders */
			int from_x = Scr.MyDisplayWidth, from_y = Scr.MyDisplayHeight ;
			int to_x = 0, to_y = 0;
			ASWharfFolder *aswf = WharfState.root_folder ; 
			add_wharf_folder_to_area( aswf, &from_x, &from_y, &to_x, &to_y );
			Scr.RootClipArea.x = from_x;
    		Scr.RootClipArea.y = from_y;
    		Scr.RootClipArea.width  = (to_x > from_x)?to_x - from_x:1;
    		Scr.RootClipArea.height = (to_y > from_y)?to_y - from_y:1;
    		if( Scr.RootImage )
    		{
        		safe_asimage_destroy( Scr.RootImage );
        		Scr.RootImage = NULL ;
    		}
		}
#endif

void
do_wharf_animate_iter( void *vdata )
{
	ASWharfFolder *aswf = (ASWharfFolder*)vdata ;
	
    if( aswf != NULL && aswf->animation_steps > 0 )
    {
        int new_width = 1, new_height = 1;
        animate_wharf( aswf, &new_width, &new_height );
        if( new_width == 0 || new_height == 0 /*||
            (new_width == aswf->canvas->width && new_height == aswf->canvas->height )*/)
            unmap_wharf_folder( aswf );
        else
        {
            LOCAL_DEBUG_OUT( "resizing folder from %dx%d to %dx%d", aswf->canvas->width, aswf->canvas->height, new_width, new_height );
            resize_canvas( aswf->canvas, new_width, new_height) ;
            ASSync( False ) ;
			if( get_flags( Config->set_flags, WHARF_ANIMATE_DELAY ) && Config->animate_delay > 0 )
			{	
				timer_new (Config->animate_delay+1, do_wharf_animate_iter, vdata);	  
            }else
				timer_new (60, do_wharf_animate_iter, vdata);	  
        }
	}		  
}


void on_wharf_moveresize( ASEvent *event )
{
    ASMagic *obj = NULL;

    obj = fetch_object( event->w ) ;
    if( obj == NULL )
        return;
    if( obj->magic == MAGIC_WHARF_BUTTON )
    {
        ASWharfButton *aswb = (ASWharfButton*)obj;
		
		/* need to check if there were any ConfigureNotify 's for our folder 
		 * and if so - go process them first */
		ASEvent parent_event;
    	while( ASCheckTypedWindowEvent(aswb->parent->canvas->w, ConfigureNotify,&(parent_event.x)) )
    	{
     		parent_event.client = NULL ;
            setup_asevent_from_xevent( &parent_event );
            DispatchEvent( &parent_event );
			timer_handle ();
        	ASSync(False);
    	}

		LOCAL_DEBUG_OUT("Handling button resizefor button %p", aswb );
        on_wharf_button_moveresize( aswb, event );
    }else if( obj->magic == MAGIC_WHARF_FOLDER )
    {
        ASWharfFolder *aswf = (ASWharfFolder*)obj;
        ASFlagType changes = handle_canvas_config (aswf->canvas );
		LOCAL_DEBUG_OUT("Handling folder resize for folder %p, mapped = %lX", aswf, get_flags( aswf->flags, ASW_Mapped ) );
        if( aswf->animation_steps == 0 && get_flags( aswf->flags, ASW_Mapped ) && aswf->animation_dir < 0 )
        {
            unmap_wharf_folder( aswf );
        }else if( changes != 0 )
        {
            int i = aswf->buttons_num ;
			Bool withdrawn = False ;
LOCAL_DEBUG_OUT("animation_steps = %d", aswf->animation_steps );

			withdrawn = (aswf->canvas->width == 1 || aswf->canvas->height == 1 ||
						 (aswf->canvas->root_x == -10000 && aswf->canvas->root_y == -10000) );

#ifdef SHAPE
            if( get_flags( changes, CANVAS_RESIZED ) && get_flags(aswf->flags,ASW_AnimationPending ) )
				XShapeCombineRectangles ( dpy, aswf->canvas->w, ShapeBounding, 0, 0, &(aswf->boundary), 1, ShapeSet, Unsorted);
#endif

			if( !withdrawn )
			{	
				if( !get_flags( aswf->flags, ASW_Withdrawn ))
				{	
					set_wharf_clip_area( aswf, aswf->canvas->root_x, aswf->canvas->root_y );
					while( --i >= 0 )
    	            	on_wharf_button_moveresize( &(aswf->buttons[i]), event );
				}else if( aswf->withdrawn_button != NULL )
					on_wharf_button_moveresize( aswf->withdrawn_button, event );
			}

#if 1			   
            if( get_flags( changes, CANVAS_RESIZED ))
			{
				LOCAL_DEBUG_OUT("AnimationPending ? = %lX", get_flags(aswf->flags,ASW_AnimationPending));
				if( get_flags(aswf->flags,ASW_AnimationPending ) )
				{	
					LOCAL_DEBUG_OUT("animate from = %dx%d, to = %dx%d", aswf->animate_from_w, aswf->animate_from_h, aswf->animate_to_w, aswf->animate_to_h);
					animate_wharf_loop( aswf, aswf->animate_from_w, aswf->animate_from_h, aswf->animate_to_w, aswf->animate_to_h, get_flags(aswf->flags,ASW_ReverseAnimation ) );
					clear_flags(aswf->flags,ASW_UseBoundary|ASW_AnimationPending|ASW_ReverseAnimation );
				}else if( !withdrawn )
				{
					/* fprintf(stderr, "clearing or applying boundary\n");	  */
					if( !update_wharf_folder_shape( aswf ) ) 
					{	
						/* fprintf(stderr, "forcing boundary\n");	  */
						update_canvas_display_mask(aswf->canvas, True);
					}
				}
			}
#endif
		
        }
    }
}

void
press_wharf_button( ASWharfButton *aswb, int state )
{
    if( WharfState.pressed_button &&
        WharfState.pressed_button != aswb )
    {
        set_astbar_pressed( WharfState.pressed_button->bar, WharfState.pressed_button->canvas, False );
        WharfState.pressed_button = NULL ;
    }
    if( aswb &&
        WharfState.pressed_button != aswb )
    {
        set_astbar_pressed( aswb->bar, aswb->canvas, True );
        WharfState.pressed_state = state ;
        WharfState.pressed_button = aswb ;
    }
}

void
release_pressure( int button )
{
    ASWharfButton *pressed = WharfState.pressed_button ;
LOCAL_DEBUG_OUT( "pressed button is %p", pressed );
	button -= Button1 ;
    if( pressed )
    {
        if( pressed->folder && !(button > 0 && pressed->fdata[button]))
        {
LOCAL_DEBUG_OUT( "pressed button has folder %p (%s)", pressed->folder, get_flags( pressed->folder->flags, ASW_Mapped )?"Mapped":"Unmapped" );
            if( get_flags( pressed->folder->flags, ASW_Mapped ) )
                withdraw_wharf_folder( pressed->folder );
            else
                display_wharf_folder( pressed->folder, pressed->canvas->root_x, pressed->canvas->root_y,
                                                       pressed->canvas->root_x+pressed->canvas->width,
                                                       pressed->canvas->root_y+pressed->canvas->height  );
        }else if( pressed->fdata[button] )
        {
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
            print_func_data(__FILE__, __FUNCTION__, __LINE__, pressed->fdata[button]);
#endif
            if( button > 0 ||
				!get_flags( pressed->flags, ASW_SwallowTarget ) || 
				pressed->swallowed == NULL )
            {  /* send command to the AS-proper : */
				ASWharfFolder *parentf = pressed->parent ;
				ASWharfButton *parentb = NULL ;
                SendCommand( pressed->fdata[button], 0);
				sleep_a_millisec(200);/* give AS a chance to handle requests */
				while( parentf != WharfState.root_folder )
				{
					parentb = parentf->parent ;
	                withdraw_wharf_folder( parentf );
					if( parentb == NULL )
						break;
					parentf = parentb->parent ;
				}
            }
        }
        set_astbar_pressed( pressed->bar, pressed->canvas, False );
        WharfState.pressed_button = NULL ;
    }
}


static Bool 
check_app_click	( ASWharfButton *aswb, XButtonEvent *xbtn )
{
	if( aswb->swallowed )
	{	
		int sx = aswb->swallowed->current->root_x ;
		int sy = aswb->swallowed->current->root_y ;
		return ( sx <= xbtn->x_root && sy <= xbtn->y_root &&
			 	aswb->swallowed->current->width + sx > xbtn->x_root &&
			 	aswb->swallowed->current->height + sy > xbtn->y_root );
	}
	return False;
}

void
on_wharf_pressed( ASEvent *event )
{
    ASMagic *obj = fetch_object( event->w ) ;
    if( obj == NULL )
        return;
    if( obj->magic == MAGIC_WHARF_BUTTON )
    {
        ASWharfButton *aswb = (ASWharfButton*)obj;
        ASWharfFolder *aswf = aswb->parent;

		if( get_flags( aswb->flags, ASW_Transient ) )
			return ;

		if( event->x.xbutton.button == Button3 && aswf == WharfState.root_folder )
		{	
            if( (WITHDRAW_ON_EDGE(Config) && (&(aswf->buttons[0]) == aswb || &(aswf->buttons[aswf->buttons_num-1]) == aswb)) ||
                 WITHDRAW_ON_ANY(Config))
        	{
				if( aswb->fdata[Button3-Button1] == NULL || 
					(event->x.xbutton.state&AllModifierMask) == ControlMask )
				{			   
            		if( get_flags( WharfState.root_folder->flags, ASW_Withdrawn) )
					{
						/* update our name to normal */	  
						set_folder_name( WharfState.root_folder, False );
						LOCAL_DEBUG_OUT( "un - withdrawing folder%s","" );
                		display_main_folder();
            		}else
            		{
                		int wx = 0, wy = 0, wwidth, wheight;
                		int i = aswf->buttons_num ;
						Bool reverse = False ; 
                
						if( Config->withdraw_style < WITHDRAW_ON_ANY_BUTTON_AND_SHOW )
                    		aswb = &(aswf->buttons[0]);

						withdraw_wharf_subfolders( aswf );
						/* update our name to withdrawn */	  					
						set_folder_name( aswf, True );

                		wwidth = aswb->desired_width ;
                		wheight = aswb->desired_height ;
						if( get_flags( aswf->flags, ASW_Vertical ) )
						{
							int dy = event->x.xbutton.y_root - aswf->canvas->root_y ; 
							if( get_flags( Config->geometry.flags, XNegative ) )
	                    		wx = Scr.MyDisplayWidth - wwidth ;
							if( dy > aswf->canvas->height/2 ) 
							{
								wy = Scr.MyDisplayHeight - wheight ;	
								reverse = (!get_flags( Config->geometry.flags, YNegative ));
							}else
								reverse = (get_flags( Config->geometry.flags, YNegative ));
						}else
						{
							int dx = event->x.xbutton.x_root - aswf->canvas->root_x ; 
                			if( get_flags( Config->geometry.flags, YNegative ) )
                    			wy = Scr.MyDisplayHeight - wheight ;
							if( dx > aswf->canvas->width/2 ) 
							{
								wx = Scr.MyDisplayWidth - wwidth ;	
								reverse = (!get_flags( Config->geometry.flags, XNegative ));
							}else
								reverse = (get_flags( Config->geometry.flags, XNegative ));
						}	
						if( get_flags(Config->flags, WHARF_ANIMATE ) )
						{	
							set_flags(aswf->flags,ASW_UseBoundary );
							animate_wharf_loop(aswf, aswf->canvas->width, aswf->canvas->height, wwidth, wheight, reverse );
							clear_flags(aswf->flags,ASW_UseBoundary );
						}

						LOCAL_DEBUG_OUT( "withdrawing folder to %dx%d%+d%+d", wwidth, wheight, wx, wy );
                		XRaiseWindow( dpy, aswb->canvas->w );
                		while ( --i >= 0 )
                		{
                    		if( &(aswf->buttons[i]) != aswb && 
								aswf->buttons[i].canvas->root_x == aswf->canvas->root_x && 
								aswf->buttons[i].canvas->root_y == aswf->canvas->root_y)
                    		{
                        		aswf->buttons[i].folder_x = wwidth ;
                        		aswf->buttons[i].folder_y = wheight ;
                        		move_canvas( aswf->buttons[i].canvas, wwidth, wheight );
                    		}
                		}
                		set_flags( aswf->flags, ASW_Withdrawn );
						set_withdrawn_clip_area( aswf, wx, wy, wwidth, wheight );
                		moveresize_canvas( aswf->canvas, wx, wy, wwidth, wheight );
	//					ASSync(False);
	//					MapConfigureNotifyLoop();

                		aswb->folder_x = 0;
                		aswb->folder_y = 0;
                		aswb->folder_width = wwidth ;
                		aswb->folder_height = wheight ;
                		moveresize_canvas( aswb->canvas, 0, 0, wwidth, wheight );

						aswf->withdrawn_button = aswb ;
            		}
	            	return;
				}
        	}
			if( event->x.xbutton.button == Button1 || aswb->fdata[event->x.xbutton.button-Button1] == NULL ) 
			{	
				if( check_app_click	( aswb, &(event->x.xbutton) ) ) 
				{
	            	event->x.xbutton.window = aswb->swallowed->current->w;
					XSendEvent (dpy, aswb->swallowed->current->w, False, ButtonPressMask, &(event->x));	
					return;
				}	 
			}
		}
        press_wharf_button( aswb, event->x.xbutton.state );
    }
}


