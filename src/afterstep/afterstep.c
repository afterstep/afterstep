/****************************************************************************
 * Copyright (c) 1999,2002 Sasha Vasko <sasha at aftercode.net>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "../../configure.h"
#define LOCAL_DEBUG

#include "../../include/asapp.h"
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#include <X11/cursorfont.h>

#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/screen.h"
#include "../../include/event.h"
#include "../../include/decor.h"
#include "../../include/module.h"
#include "../../include/clientprops.h"
#include "../../include/hints.h"
#include "../../include/session.h"
#include "../../include/wmprops.h"
#include "../../include/balloon.h"
#include "../../include/loadimg.h"
#include "../../libAfterImage/afterimage.h"

#include "asinternals.h"

/**************************************************************************/
/* 		Global variables that defines our behaviour 		  */
/**************************************************************************/
/* our status */
ASFlagType AfterStepState = 0; /* default status */

/* Config : */
struct ASSession *Session = NULL;          /* filenames of look, feel and background */
struct ASDatabase    *Database = NULL;

/* Base config : */
char         *PixmapPath = NULL;
char         *CursorPath = NULL;
char         *IconPath   = NULL;
char         *ModulePath = AFTER_BIN_DIR;
char         *FontPath   = NULL;

#ifdef SHAPE
int           ShapeEventBase, ShapeErrorBase;
#endif


int           fd_width, x_fd;

ASVector     *Modules       = NULL;
int           Module_fd     = 0;
int           Module_npipes = 8;

/**************************************************************************/
void          SetupScreen();
void          CleanupScreen();

void          IgnoreSignal (int sig);
SIGNAL_T      Restart (int nonsense);
SIGNAL_T      SigDone (int nonsense);

void          CaptureAllWindows (ScreenInfo *scr);
void          DoAutoexec( Bool restarting );

Bool afterstep_parent_hints_func(Window parent, ASParentHints *dst );

/**************************************************************************/
/**************************************************************************/
/***********************************************************************
 *  Procedure:
 *	main - start of afterstep
 ************************************************************************/
int
main (int argc, char **argv)
{
    register int i ;

#ifdef DEBUG_TRACE_X
	trace_window_id2name_hook = &window_id2name;
#endif
    InitMyApp( CLASS_AFTERSTEP, argc, argv, NULL, NULL, 0);
    AfterStepState = MyArgs.flags ;

#if defined(LOG_FONT_CALLS)
	fprintf (stderr, "logging font calls now\n");
#endif

    set_parent_hints_func( afterstep_parent_hints_func ); /* callback for collect_hints() */

    /* These signals are mandatory : */
    signal (SIGUSR1, Restart);
    signal (SIGALRM, AlarmHandler); /* see SetTimer() */
    /* These signals we would like to handle only if those are not handled already (by debugger): */
    IgnoreSignal(SIGINT);
    IgnoreSignal(SIGHUP);
    IgnoreSignal(SIGQUIT);
    IgnoreSignal(SIGTERM);


    if( (x_fd = ConnectX( &Scr, MyArgs.display_name, AS_ROOT_EVENT_MASK )) < 0 )
    {
        show_error("failed to initialize window manager session. Aborting!", Scr.screen);
        exit(1);
    }
    if (fcntl (x_fd, F_SETFD, 1) == -1)
	{
        show_error ("close-on-exec failed");
        exit (3);
	}

    if (get_flags( AfterStepState, ASS_Debugging))
        set_synchronous_mode(True);
    XSync (dpy, 0);

    /* initializing our dirs names */
    Session = GetNCASSession(&Scr, MyArgs.override_home, MyArgs.override_share);
    if( MyArgs.override_config )
        set_session_override( Session, MyArgs.override_config, 0 );
    if( MyArgs.override_look )
        set_session_override( Session, MyArgs.override_look, F_CHANGE_LOOK );
    if( MyArgs.override_feel )
        set_session_override( Session, MyArgs.override_feel, F_CHANGE_FEEL );

#ifdef SHAPE
	XShapeQueryExtension (dpy, &ShapeEventBase, &ShapeErrorBase);
#endif /* SHAPE */

	fd_width = get_fd_width ();
    SetupModules();
    SetupScreen();
    event_setup( True /*Bool local*/ );
    /*
     *  Lets init each and every screen separately :
     */
    for (i = 0; i < Scr.NumberOfScreens; i++)
	{
        show_progress( "Initializing screen %d ...", i );
        if (i != Scr.screen)
        {
            if( !get_flags(MyArgs.flags, ASS_SingleScreen) )
            {
                int pid = spawn_child( MyName, (i<MAX_USER_SINGLETONS_NUM)?i:-1, i, None, C_NO_CONTEXT, True, True, NULL );
                if( pid >= 0 )
                    show_progress( "\t instance of afterstep spawned with pid %d.", pid );
                else
                    show_error( "failed to launch instance of afterstep to handle screen #%d", i );
            }
        }else
        {
            if( is_output_level_under_threshold( OUTPUT_LEVEL_PROGRESS ) )
            {
                show_progress( "\t screen[%d].size = %ux%u", Scr.screen, Scr.MyDisplayWidth, Scr.MyDisplayHeight );
                show_progress( "\t screen[%d].root = %lX", Scr.screen, Scr.Root );
                show_progress( "\t screen[%d].color_depth = %d", Scr.screen, Scr.asv->true_depth );
                show_progress( "\t screen[%d].colormap    = 0x%lX", Scr.screen, Scr.asv->colormap );
                show_progress( "\t screen[%d].visual.id         = %X",  Scr.screen, Scr.asv->visual_info.visualid );
                show_progress( "\t screen[%d].visual.class      = %d",  Scr.screen, Scr.asv->visual_info.class );
                show_progress( "\t screen[%d].visual.red_mask   = 0x%8.8lX", Scr.screen, Scr.asv->visual_info.red_mask   );
                show_progress( "\t screen[%d].visual.green_mask = 0x%8.8lX", Scr.screen, Scr.asv->visual_info.green_mask );
                show_progress( "\t screen[%d].visual.blue_mask  = 0x%8.8lX", Scr.screen, Scr.asv->visual_info.blue_mask  );
                make_screen_envvars(&Scr);
                putenv (Scr.rdisplay_string);
                putenv (Scr.display_string);
                show_progress( "\t screen[%d].rdisplay_string = \"%s\"", Scr.screen, Scr.rdisplay_string );
                show_progress( "\t screen[%d].display_string = \"%s\"", Scr.screen, Scr.display_string );
            }
        }
    }

#ifdef SHAPE
	XShapeQueryExtension (dpy, &ShapeEventBase, &ShapeErrorBase);
#endif /* SHAPE */

    /* Load config ... */
    /* read config file, set up menus, colors, fonts */
    InitBase (False);
    InitDatabase (False);
    LoadASConfig (0, PARSE_EVERYTHING);

    /* Reparent all the windows and setup pan frames : */
    XSync (dpy, 0);
   /***********************************************************/
#ifndef DONT_GRAB_SERVER                    /* grabbed   !!!!!*/
	XGrabServer (dpy);                		/* grabbed   !!!!!*/
#endif										/* grabbed   !!!!!*/
#ifndef NO_VIRTUAL							/* grabbed   !!!!!*/
    InitPanFrames ();                       /* grabbed   !!!!!*/
#endif /* NO_VIRTUAL */                     /* grabbed   !!!!!*/
    CaptureAllWindows (&Scr);               /* grabbed   !!!!!*/
#ifndef NO_VIRTUAL							/* grabbed   !!!!!*/
    CheckPanFrames ();                      /* grabbed   !!!!!*/
#endif /* NO_VIRTUAL */						/* grabbed   !!!!!*/
#ifndef DONT_GRAB_SERVER					/* grabbed   !!!!!*/
	XUngrabServer (dpy);					/* UnGrabbed !!!!!*/
#endif										/* UnGrabbed !!!!!*/
	/**********************************************************/
    XDefineCursor (dpy, Scr.Root, Scr.Feel.cursors[DEFAULT]);

   /* make sure we're on the right desk, and the _WIN_DESK property is set */
    ChangeDesks (Scr.wmprops->desktop_current);

    SetupFunctionHandlers();
    DoAutoexec(get_flags( AfterStepState, ASS_Restarting));

    /* all system Go! we are completely Operational! */
    set_flags( AfterStepState, ASS_NormalOperation);

#if (defined(LOCAL_DEBUG)||defined(DEBUG)) && defined(DEBUG_ALLOCS)
    LOCAL_DEBUG_OUT( "printing memory%s","");
    spool_unfreed_mem( "afterstep.allocs.startup", NULL );
#endif
    LOCAL_DEBUG_OUT( "entering main loop%s","");

    HandleEvents ();
	return (0);
}

/*************************************************************************/
/* Our Screen initial state setup and initialization of management data :*/
/*************************************************************************/
void
CreateGCs (void)
{
    if( Scr.DrawGC == None )
    {
        XGCValues     gcv;
        unsigned long gcm;
        gcm = GCFunction | GCLineWidth | GCForeground | GCSubwindowMode;
        gcv.function = GXxor;
        gcv.line_width = 0;
        gcv.foreground = Scr.Feel.XorValue;
        gcv.subwindow_mode = IncludeInferiors;
        Scr.DrawGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
    }
}
void
CreateCursors (void)
{
	/* define cursors */
    Scr.standard_cursors[POSITION]     = XCreateFontCursor (dpy, XC_left_ptr);
/*  Scr.ASCursors[DEFAULT] = XCreateFontCursor(dpy, XC_top_left_arrow); */
    Scr.standard_cursors[DEFAULT]      = XCreateFontCursor (dpy, XC_left_ptr);
    Scr.standard_cursors[SYS]          = XCreateFontCursor (dpy, XC_left_ptr);
    Scr.standard_cursors[TITLE_CURSOR] = XCreateFontCursor (dpy, XC_left_ptr);
    Scr.standard_cursors[MOVE]         = XCreateFontCursor (dpy, XC_fleur);
    Scr.standard_cursors[MENU]         = XCreateFontCursor (dpy, XC_left_ptr);
    Scr.standard_cursors[WAIT]         = XCreateFontCursor (dpy, XC_watch);
    Scr.standard_cursors[SELECT]       = XCreateFontCursor (dpy, XC_dot);
    Scr.standard_cursors[DESTROY]      = XCreateFontCursor (dpy, XC_pirate);
    Scr.standard_cursors[LEFT]         = XCreateFontCursor (dpy, XC_left_side);
    Scr.standard_cursors[RIGHT]        = XCreateFontCursor (dpy, XC_right_side);
    Scr.standard_cursors[TOP]          = XCreateFontCursor (dpy, XC_top_side);
    Scr.standard_cursors[BOTTOM]       = XCreateFontCursor (dpy, XC_bottom_side);
    Scr.standard_cursors[TOP_LEFT]     = XCreateFontCursor (dpy, XC_top_left_corner);
    Scr.standard_cursors[TOP_RIGHT]    = XCreateFontCursor (dpy, XC_top_right_corner);
    Scr.standard_cursors[BOTTOM_LEFT]  = XCreateFontCursor (dpy, XC_bottom_left_corner);
    Scr.standard_cursors[BOTTOM_RIGHT] = XCreateFontCursor (dpy, XC_bottom_right_corner);
}

void
CreateManagementWindows()
{
    XSetWindowAttributes attr;           /* attributes for create windows */
    /* the SizeWindow will be moved into place in LoadASConfig() */
    attr.override_redirect = True;
    attr.bit_gravity = NorthWestGravity;
	Scr.SizeWindow = create_visual_window (Scr.asv, Scr.Root, -999, -999, 10, 10, 0,
										   InputOutput, CWBitGravity | CWOverrideRedirect,
                                           &attr);

    /* create a window which will accept the keyboard focus when no other
	   windows have it */
    attr.event_mask = KeyPressMask | FocusChangeMask;
    attr.override_redirect = True;
	Scr.NoFocusWin = create_visual_window (Scr.asv, Scr.Root, -10, -10, 10, 10, 0,
										   InputOnly, CWEventMask | CWOverrideRedirect,
                                           &attr);
	XMapWindow (dpy, Scr.NoFocusWin);
	XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, CurrentTime);
}

void
DestroyManagementWindows()
{
    if( Scr.NoFocusWin )
        XDestroyWindow( dpy, Scr.NoFocusWin );
    Scr.NoFocusWin = None;
    if( Scr.SizeWindow )
        XDestroyWindow( dpy, Scr.SizeWindow );
    Scr.SizeWindow = None;
}

/***********************************************************************
 *  Procedure:
 *  Setup Screen - main function
 ************************************************************************/
void
SetupScreen()
{
    InitLook(&Scr.Look, False);
    InitFeel(&Scr.Feel, False);

    Scr.Vx = Scr.Vy = 0;
    Scr.CurrentDesk = 0;

    Scr.randomx = Scr.randomy = 0;
    Scr.RootCanvas = create_ascanvas_container( Scr.Root );

    SetupColormaps();
    CreateCursors ();
    CreateManagementWindows();
    Scr.Windows = init_aswindow_list();

}

void
CleanupScreen()
{
    int i ;

    if( Scr.Windows )
    {
        XGrabServer (dpy);
        destroy_aswindow_list( &(Scr.Windows), True );
        XUngrabServer (dpy);
    }
    DestroyManagementWindows();
    CleanupColormaps();

    if( Scr.RootCanvas )
        destroy_ascanvas( &(Scr.RootCanvas) );

    XSetInputFocus (dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XSync (dpy, 0);

#ifdef HAVE_XINERAMA
	if (Scr.xinerama_screens)
	{
		free (Scr.xinerama_screens);
		Scr.xinerama_screens_num = 0;
		Scr.xinerama_screens = NULL;
	}
#endif /* XINERAMA */


    for( i = 0 ; i < MAX_CURSORS; ++i )
        if( Scr.standard_cursors[i] )
        {
            XFreeCursor( dpy, Scr.standard_cursors[i] );
            Scr.standard_cursors[i] = None ;
        }

    InitLook(&Scr.Look, True);
    InitFeel(&Scr.Feel, True);


    /* free display strings; can't do this in main(), because some OS's
     * don't copy the environment variables properly */
    if( Scr.display_string )
    {
        free (Scr.display_string);
        Scr.display_string = NULL ;
    }
    if( Scr.rdisplay_string )
    {
        free (Scr.rdisplay_string);
        Scr.rdisplay_string = NULL ;
    }

    destroy_wmprops( Scr.wmprops, False);
    destroy_image_manager( Scr.image_manager, False );
LOCAL_DEBUG_OUT("destroying font manager : %s","");
    destroy_font_manager( Scr.font_manager, False );
LOCAL_DEBUG_OUT("screen cleanup complete.%s","");
}

/*************************************************************************/
/* populating windowlist with presently available windows :
 * (backported from as-devel)
 */
/*************************************************************************/
void
CaptureAllWindows (ScreenInfo *scr)
{
    int           i;
	unsigned int  nchildren;
	Window        root, parent, *children;
    Window        focused = None ;
    int           revert_to = RevertToNone ;
    XWindowAttributes attr;

	if( scr == NULL )
		return ;
	focused = XGetInputFocus( dpy, &focused, &revert_to );

	if (!XQueryTree (dpy, scr->Root, &root, &parent, &children, &nchildren))
		return;

    /* weed out icon windows : */
	for (i = 0; i < nchildren; i++)
        if (children[i] )
		{
			XWMHints     *wmhintsp = NULL;
            if( (wmhintsp = XGetWMHints (dpy, children[i])) != NULL )
            {
                if( get_flags(wmhintsp->flags, IconWindowHint) )
                {
                    register int j ;
                    for (j = 0; j < nchildren; j++)
						if (children[j] == wmhintsp->icon_window)
						{
							children[j] = None;
							break;
						}
				}
				XFree ((char *)wmhintsp);
			}
		}

    /* map all the rest of the windows : */
    for (i = 0; i < nchildren; i++)
        if (children[i] && window2ASWindow (children[i]) == NULL )
        {
            /* weed out override redirect windows and unmapped windows : */
            unsigned long nitems = 0;
            unsigned long *state_prop = NULL ;
            int wm_state = DontCareState ;

            if ( !XGetWindowAttributes (dpy, children[i], &attr) )
                continue;
            if( attr.override_redirect )
                continue;
            if( read_32bit_proplist (children[i], _XA_WM_STATE, 2, &state_prop, &nitems) )
			{
                wm_state = state_prop[0] ;
				XFree( state_prop );
			}
            if( (wm_state == IconicState) || (attr.map_state != IsUnmapped))
                AddWindow( children[i] );
		}

    if (children)
		XFree ((char *)children);

    if( focused != None && focused != PointerRoot )
    {
        ASWindow *t = window2ASWindow( focused );
        if( t )
            activate_aswindow( t, False, False );
    }
}

/***********************************************************************
 * running Autoexec code ( if any ) :
 ************************************************************************/
void DoAutoexec( Bool restarting )
{
    FunctionData func ;
    ASEvent event = {0};
    char   screen_func_name[128];

    init_func_data( &func );
    func.func = F_FUNCTION ;
    func.name = restarting?"RestartFunction":"InitFunction";
    if( Scr.screen > 0 )
    {
        sprintf (screen_func_name, restarting?"RestartScreen%ldFunction":"InitScreen%ldFunction", Scr.screen);
        if( find_complex_func( Scr.Feel.ComplexFunctions, &(screen_func_name[0])) == NULL )
            func.name = &(screen_func_name[0]);
    }
    ExecuteFunction (&func, &event, -1);
}

/***********************************************************************
 * our signal handlers :
 ************************************************************************/
void
IgnoreSignal (int sig)
{
	if (signal (sig, SIG_IGN) != SIG_IGN)
		signal (sig, SigDone);
}
void
Restart (int nonsense)
{
    Done (True, NULL);
	SIGNAL_RETURN;
}
void
SigDone (int nonsense)
{
    Done (False, NULL);
	SIGNAL_RETURN;
}

/*************************************************************************/
/* our shutdown function :                                               */
/*************************************************************************/
void
Done (Bool restart, char *command )
{
    int restart_screen = get_flags( AfterStepState, ASS_SingleScreen)?Scr.screen:-1;
LOCAL_DEBUG_CALLER_OUT( "%s restart, cmd=\"%s\"", restart?"Do":"Don't", command?command:"");
    set_flags( Scr.state, AS_StateShutdown );
    if( restart )
        set_flags( Scr.state, AS_StateRestarting );
#ifndef NO_VIRTUAL
//    MoveViewport (0, 0, False);
#endif
#ifndef NO_SAVEWINDOWS
	if (!restart)
    {
        char *fname = make_session_file( Session, AFTER_SAVE, False );
        save_aswindow_list( Scr.Windows, fname );
        free( fname );
    }
#endif
    /* remove window frames */
    CleanupScreen();

	/* Close all my pipes */
    ShutdownModules();

	/* freeing up memory */
    destroy_assession( Session );
    InitDatabase(True);
    InitBase(True);
    free_func_hash ();
    if( lock_mods );
        free (lock_mods);
    /* balloons */
    balloon_init (1);
    /* pixmap references */
    pixmap_ref_purge ();
    build_xpm_colormap (NULL);

    /* Really make sure that the connection is closed and cleared! */
    XSelectInput (dpy, Scr.Root, 0);
    XSync (dpy, 0);
    XCloseDisplay (dpy);

	if (restart)
	{
        spawn_child( command?command:MyName, -1, restart_screen,
                     None, C_NO_CONTEXT, False, False, NULL );
    } else
	{
#ifdef DEBUG_ALLOCS
        restack_window_list(INVALID_DESK);
        clientprops_cleanup ();
        wmprops_cleanup ();
        flush_ashash_memory_pool();
        destroy_asvisual( Scr.asv, False );
        flush_asbidirlist_memory_pool();
        free( MyArgs.saved_argv );
        print_unfreed_mem ();
#endif /*DEBUG_ALLOCS */
    }
    exit(0);
}


