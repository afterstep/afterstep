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

#define DO_CLOCKING
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
#include "../../include/functions.h"

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

typedef struct ASWharfButton
{
    unsigned long magic;
#define ASW_SwallowTarget   (0x01<<0)
    ASFlagType flags;
    char *name ;
    ASCanvas *canvas;
    ASCanvas   *swallowed;
    ASTBarData *bar;

    unsigned int     desired_width, desired_height;
    FunctionData    *fdata;

    struct ASWharfFolder   *folder;
    struct ASWharfFolder   *parent;
}ASWharfButton;

typedef struct ASWharfFolder
{
    unsigned long magic;
#define ASW_Mapped          (0x01<<0)
#define ASW_Vertical        (0x01<<1)
    ASFlagType  flags;

    ASCanvas    *canvas;
    ASWharfButton *buttons ;
    int buttons_num;
    ASWharfButton *parent ;

    unsigned int total_width, total_height;    /* size calculated based on size of participating buttons */

    int animation_steps;                       /* how many steps left */
    int animation_dir;                         /* +1 or -1 */
}ASWharfFolder;

typedef struct ASWharfState
{
    ASHashTable   *win2obj_xref;               /* xref of window IDs to wharf buttons and folders */
    ASWharfFolder *root_folder ;

}ASWharfState;

ASWharfState WharfState;
WharfConfig *Config = NULL;

int x_fd;
int as_fd[2];

#define WHARF_BUTTON_EVENT_MASK   (ButtonReleaseMask |\
                                   ButtonPressMask | LeaveWindowMask | EnterWindowMask |\
                                   StructureNotifyMask)
#define WHARF_FOLDER_EVENT_MASK   (StructureNotifyMask)


void HandleEvents(int x_fd, int *as_fd);
void process_message (unsigned long type, unsigned long *body);
void DispatchEvent (ASEvent * Event);
Window make_wharf_window();
void GetOptions (const char *filename);
void GetBaseOptions (const char *filename);
void CheckConfigSanity();

ASWharfFolder *build_wharf_folder( WharfButton *list, ASWharfButton *parent, Bool vertical );
void display_wharf_folder( ASWharfFolder *aswf );


/***********************************************************************
 *   main - start of module
 ***********************************************************************/
int
main (int argc, char **argv)
{
    int x_fd ;

    /* Save our program name - for error messages */
    InitMyApp (CLASS_WHARF, argc, argv, NULL, NULL, 0 );

    memset( &WharfState, 0x00, sizeof(WharfState));

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

    as_fd[0] = as_fd[1] = ConnectAfterStep (M_TOGGLE_PAGING |
                                            M_NEW_DESK |
                                            M_END_WINDOWLIST |
                                            M_MAP |
                                            M_RES_NAME |
                                            M_RES_CLASS |
                                            M_WINDOW_NAME);

    Config = CreateWharfConfig ();

    LOCAL_DEBUG_OUT("parsing Options ...%s","");
    LoadBaseConfig (GetBaseOptions);
    LoadConfig ("wharf", GetOptions);

    CheckConfigSanity();

    WharfState.root_folder = build_wharf_folder( Config->root_folder, NULL, (Config->rows != 1) );
    display_wharf_folder( WharfState.root_folder );

    /* Create a list of all windows */
    /* Request a list of all windows,
     * wait for ConfigureWindow packets */
    SendInfo (as_fd, "Send_WindowList", 0);

    /* create main folder here : */

    LOCAL_DEBUG_OUT("starting The Loop ...%s","");
//    HandleEvents(x_fd, as_fd);

    return 0;
}

void HandleEvents(int x_fd, int *as_fd)
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
        module_wait_pipes_input ( x_fd, as_fd[1], process_message );
    }
}

void
DeadPipe (int nonsense)
{
#ifdef DEBUG_ALLOCS

/* normally, we let the system clean up, but when auditing time comes
 * around, it's best to have the books in order... */
    balloon_init (1);

    if( Config )
        DestroyWharfConfig (Config);

    mystyle_free_global_gcs ();
    mystyle_destroy_all();
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
    char buf[256];

    if( Config == NULL )
        Config = CreateWharfConfig ();

    if( Config->rows == 0 )
        Config->rows = 1;

    if( Config->columns == 0 )
        Config->columns = 1;

    Config->gravity = NorthWestGravity ;
    if( get_flags(Config->geometry.flags, XNegative) )
        Config->gravity = get_flags(Config->geometry.flags, YNegative)? SouthEastGravity:NorthEastGravity;
    else if( get_flags(Config->geometry.flags, YNegative) )
        Config->gravity = SouthWestGravity;


    mystyle_get_property (Scr.wmprops);

    sprintf( buf, "*%sStyle", get_application_name() );
    Scr.Look.MSWindow[BACK_UNFOCUSED] = mystyle_find_or_default( buf );

    show_progress( "printing wharf config : ");
    PrintWharfConfig(Config);
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
    START_TIME(option_time);
SHOW_CHECKPOINT;
    show_progress( "loading wharf config from \"%s\": ", filename);
    config = ParseWharfOptions (filename, MyName);
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

    config->gravity = NorthWestGravity ;
    if( get_flags( config->set_flags, WHARF_GEOMETRY ) )
        merge_geometry(&(config->geometry), &(Config->geometry) );

    if( get_flags( config->set_flags, WHARF_WITHDRAW_STYLE ) )
        Config->withdraw_style = config->withdraw_style;

    if( get_flags( config->set_flags, WHARF_FORCE_SIZE ) )
        merge_geometry( &(Config->force_size), &(config->force_size));

    if( get_flags( config->set_flags, WHARF_ANIMATE_STEPS ) )
        Config->animate_steps = config->animate_steps;
    if( get_flags( config->set_flags, WHARF_ANIMATE_STEPS_MAIN ) )
        Config->animate_steps_main = config->animate_steps_main ;
    if( get_flags( config->set_flags, WHARF_ANIMATE_DELAY ) )
        Config->animate_delay = config->animate_delay;
    if( get_flags( config->set_flags, WHARF_LABEL_LOCATION ) )
        Config->label_location = config->label_location;

    if( get_flags( config->set_flags, WHARF_SOUND ) )
    {
        int i ;
        for( i = 0 ; i < WHEV_MAX_EVENTS ; ++i )
        {
            set_string_value(&(Config->sounds[i]), config->sounds[i], NULL, 0 );
            config->sounds[i] = NULL ;
        }
    }
    /* merging folders : */

    if( config->root_folder )
        merge_wharf_folders( &(Config->root_folder), &(config->root_folder) );

    if (config->style_defs)
        ProcessMyStyleDefinitions (&(config->style_defs));

    DestroyWharfConfig (config);
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

    Scr.VScale = config->desktop_scale;

    DestroyBaseConfig (config);

    Scr.Vx = 0;
    Scr.Vy = 0;

    Scr.VxMax = Scr.MyDisplayWidth;
    Scr.VyMax = Scr.MyDisplayHeight;

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
    ASMagic *obj = NULL ;
    if( WharfState.win2obj_xref )
        get_hash_item( WharfState.win2obj_xref, AS_HASHABLE(w), (void**)&obj );
    return obj ;
}

void
unregister_client( Window w )
{
    if( WharfState.win2obj_xref )
        remove_hash_item( WharfState.win2obj_xref, AS_HASHABLE(w), NULL, False );
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
        WindowPacketResult res ;
        /* saving relevant client info since handle_window_packet could destroy the actuall structure */
        Window               saved_w = wd?wd->canvas->w:None;
        int                  saved_desk = wd?wd->desk:INVALID_DESK;
        struct ASWindowData *saved_wd = wd ;

        show_progress( "message %lX window %X data %p", type, body[0], wd );
		res = handle_window_packet( type, body, &wd );
        if( res == WP_DataCreated )
        {
        }else if( res == WP_DataChanged )
        {
        }else if( res == WP_DataDeleted )
        {
            LOCAL_DEBUG_OUT( "client deleted (%p)->window(%lX)->desk(%d)", saved_wd, saved_w, saved_desk );
        }
    }else
    {
        switch( type )
        {
            case M_TOGGLE_PAGING :
                break;
            case M_NEW_PAGE :
                break;
            case M_NEW_DESK :
                break ;
            case M_STACKING_ORDER :
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

    event->client = NULL ;
    balloon_handle_event (&(event->x));

    switch (event->x.type)
    {
	    case ConfigureNotify:
            break;
        case KeyPress :
            break ;
        case KeyRelease :
            break ;
        case ButtonPress:
            break;
        case ButtonRelease:
            break;
        case MotionNotify :
            break ;
	    case ClientMessage:
            break;
	    case PropertyNotify:
            if( event->x.xproperty.atom == _XROOTPMAP_ID && event->w == Scr.Root )
            {
                LOCAL_DEBUG_OUT( "root background updated!%s","");
                safe_asimage_destroy( Scr.RootImage );
                Scr.RootImage = NULL ;
            }
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

    attr.event_mask = WHARF_FOLDER_EVENT_MASK ;
    w = create_visual_window( Scr.asv, None, 0, 0, 1, 1, 0, InputOutput, CWEventMask, &attr );
    register_object( w, (ASMagic*)aswf );
    return create_ascanvas(w);
}


ASCanvas*
create_wharf_button_canvas(ASWharfButton *aswb, ASCanvas *parent)
{
    static XSetWindowAttributes attr ;
    Window w ;

    attr.event_mask = WHARF_BUTTON_EVENT_MASK ;
    w = create_visual_window( Scr.asv, parent->w, 0, 0, 64, 64, 0, InputOutput, CWEventMask, &attr );
    register_object( w, (ASMagic*)aswb );
    return create_ascanvas(w);
}

ASTBarData *
build_wharf_button_tbar(WharfButton *wb)
{
    ASTBarData *bar = create_astbar();
    int icon_row = 0, icon_col = 0;

    if( get_flags( Config->flags, WHARF_SHOW_LABEL ) && wb->title )
    {
        int label_row = 0, label_col = 0;
        int label_flip = get_flags( Config->flags, WHARF_FLIP_LABEL )?FLIP_UPSIDEDOWN:0;
        int label_align = NO_ALIGN ;
#define WLL_Vertical    (0x01<<0)
#define WLL_Opposite    (0x01<<1)
#define WLL_AlignCenter (0x01<<2)
#define WLL_AlignRight  (0x01<<3)
#define WLL_OverIcon    (0x01<<4)

        if( get_flags(Config->label_location, WLL_Vertical ))
            label_flip |= FLIP_VERTICAL ;
        if( !get_flags(Config->label_location, WLL_OverIcon ))
        {
            if( get_flags(Config->label_location, WLL_Vertical ))
            {
                if( get_flags(Config->label_location, WLL_Opposite ))
                    label_col = 1;
                else
                    icon_col = 1;
            }else if( get_flags(Config->label_location, WLL_Opposite ))
                label_row = 1;
            else
                icon_row = 1;
        }
        if( get_flags(Config->label_location, WLL_AlignCenter ))
            label_align = ALIGN_CENTER ;
        else if( get_flags(Config->label_location, WLL_AlignRight ))
            label_align = ALIGN_RIGHT|ALIGN_TOP ;
        else
            label_align = ALIGN_LEFT|ALIGN_BOTTOM ;
        add_astbar_label( bar, label_col, label_row, label_flip, label_align, wb->title );
    }
    if( wb->icon )
    {
        register int i = -1;
        while( wb->icon[++i] )
        {
            ASImage *im = NULL ;
            /* load image here */
            im = get_asimage( Scr.image_manager, wb->icon[i], ASFLAGS_EVERYTHING, 100 );
            if( im )
                add_astbar_icon( bar, icon_col, icon_row, 0, 0, im );
        }
    }
    set_astbar_style_ptr( bar, BAR_STATE_UNFOCUSED, Scr.Look.MSWindow[BACK_UNFOCUSED] );
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

ASWharfFolder *
build_wharf_folder( WharfButton *list, ASWharfButton *parent, Bool vertical )
{
    ASWharfFolder *aswf = NULL ;
    int count = 0 ;
    WharfButton *wb = list ;
    while( wb )
    {
        ++count ;
        wb = wb->next ;
    }

    if( (aswf = create_wharf_folder(count, parent)) != NULL  )
    {
        ASWharfButton *aswb = aswf->buttons;
        aswf->canvas = create_wharf_folder_canvas(aswf);
        if( vertical )
            set_flags( aswf->flags, ASW_Vertical );

        wb = list;
        while( wb )
        {
            aswb->name = mystrdup( wb->title );
            if( wb->function )
            {
                aswb->fdata = safecalloc( 1, sizeof(FunctionData) );
                dup_func_data( aswb->fdata, wb->function );
                if( IsSwallowFunc(aswb->fdata->func) )
                    set_flags( aswb->flags, ASW_SwallowTarget );
            }

            aswb->canvas = create_wharf_button_canvas(aswb, aswf->canvas);
            aswb->bar = build_wharf_button_tbar(wb);

            if( !get_flags( aswb->flags, ASW_SwallowTarget ) )
            {
                aswb->desired_width  = calculate_astbar_width( aswb->bar );
                aswb->desired_height = calculate_astbar_height( aswb->bar );
            }else
            {
                aswb->desired_width = 64 ;
                aswb->desired_height = 64 ;
            }

            if( wb->folder )
                aswb->folder = build_wharf_folder( wb->folder, aswb, !vertical );

            ++aswb;
            wb = wb->next ;
        }
    }
    return aswf;
}

void
display_wharf_folder( ASWharfFolder *aswf )
{

}



