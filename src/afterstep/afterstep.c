/****************************************************************************
 * Copyright (c) 1999,2002 Sasha Vasko <sasha at aftercode.net>
 * This module is based on Twm, but has been SIGNIFICANTLY modified
 * by Rob Nation
 * by Bo Yang
 * by Frank Fejes
 * by Alfredo Kojima
 ****************************************************************************/

/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/


#include "../../configure.h"

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#include <X11/cursorfont.h>

#include "../../include/asapp.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/screen.h"
#include "../../include/event.h"
#include "../../include/module.h"
#include "../../include/clientprops.h"
#include "../../include/hints.h"
#include "../../libAfterImage/afterimage.h"

#include "asinternals.h"

/**************************************************************************/
/* 		Global variables that defines our behaviour 		  */
/**************************************************************************/
/* our status */
ASFlagType AfterStepState = 0; /* default status */

/* Config : */
struct SessionConfig *Session = NULL;          /* filenames of look, feel and background */
struct ASDatabase    *Database = NULL;

/* Base config : */
char         *PixmapPath;
char         *CursorPath;
char         *IconPath;
char         *ModulePath = AFTER_BIN_DIR;

#ifdef SHAPE
int           ShapeEventBase, ShapeErrorBase;
#endif


int           fd_width, x_fd;

ASVector     *Modules       = NULL;
int           Module_fd     = 0;
int           Module_npipes = 8;

/**************************************************************************/
void          SetupSignalHandler (int sig);
void          make_screen_envvars( ScreenInfo *scr );

void          SetupEnvironment( ScreenInfo *scr );
void          SetupInputFocus( ScreenInfo *scr );
SIGNAL_T      Restart (int nonsense);
SIGNAL_T      SigDone (int nonsense);

void          CaptureAllWindows (ScreenInfo *scr);
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
	Bool good_screen_count = 0 ;
    XSetWindowAttributes attr;           /* attributes for create windows */

#ifdef DEBUG_TRACE_X
	trace_window_id2name_hook = &window_id2name;
#endif
    InitMyApp( CLASS_AFTERSTEP, argc, argv, NULL, 0 );
    AfterStepState = MyArgs.flags ;

    init_old_look_variables (False);
	InitBase (False);
	InitLook (False);
	InitFeel (False);
	InitDatabase (False);

#if defined(LOG_FONT_CALLS)
	fprintf (stderr, "logging font calls now\n");
#endif

    set_parent_hints_func( afterstep_parent_hints_func ); /* callback for collect_hints() */

    /* These signals are mandatory : */
    signal (SIGUSR1, Restart);
    signal (SIGALRM, AlarmHandler); /* see SetTimer() */
    /* These signals we would like to handle only if those are not handled already (by debugger): */
    SetupSignalHandler(SIGINT);
    SetupSignalHandler(SIGHUP);
    SetupSignalHandler(SIGQUIT);
    SetupSignalHandler(SIGTERM);

    if( (x_fd = ConnectX( AS_ROOT_EVENT_MASK, True )) < 0 )
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
    Session = GetNCASSession(Scr.true_depth, MyArgs.override_home, MyArgs.override_share);
    if( MyArgs.override_config )
        SetSessionOverride( Session, MyArgs.override_config );

    InternUsefulAtoms ();

#ifdef SHAPE
	XShapeQueryExtension (dpy, &ShapeEventBase, &ShapeErrorBase);
#endif /* SHAPE */

	fd_width = get_fd_width ();
    SetupModules();

    /*
     *  Lets init each and every screen separately :
     */
    for (i = 0; i < NumberOfScreens; i++)
	{
        show_progress( "Initializing screen %d ...", i );
        if (i != Scr.screen)
        {
            if( !get_flags(MyArgs.flags, ASS_SingleScreen) )
            {
                int pid = spawn_child( MyName, (i<MAX_USER_SINGLETONS_NUM)?i:-1, i, None, C_NO_Context, True, True, NULL );
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
                show_progress( "\t screen[%d].root = %d", Scr.screen, Scr.Root );
                show_progress( "\t screen[%d].color_depth = %d", Scr.screen, Scr.asv->true_depth );
                show_progress( "\t screen[%d].colormap    = 0x%lX", Scr.screen, Scr.asv->colormap );
                show_progress( "\t screen[%d].visual.id         = %d",  Scr.screen, Scr.asv->visual_info.visualid );
                show_progress( "\t screen[%d].visual.class      = %d",  Scr.screen, Scr.asv->visual_info.class );
                show_progress( "\t screen[%d].visual.red_mask   = 0x%8.8lX", Scr.screen, Scr.asv->visual_info.red_mask   );
                show_progress( "\t screen[%d].visual.green_mask = 0x%8.8lX", Scr.screen, Scr.asv->visual_info.green_mask );
                show_progress( "\t screen[%d].visual.blue_mask  = 0x%8.8lX", Scr.screen, Scr.asv->visual_info.blue_mask  );
                make_screen_envvars(*Scr);
                putenv (Scr.rdisplay_string);
                putenv (Scr.display_string);
                show_progress( "\t screen[%d].rdisplay_string = \"%s\"", Scr.screen, Scr.rdisplay_string );
                show_progress( "\t screen[%d].display_string = \"%s\"", Scr.screen, Scr.display_string );

            }
        }
    }

    init_screen_gcs(&Scr);

    Scr.supported_hints = create_hints_list();
    enable_hints_support( Scr.supported_hints, HINTS_ICCCM );
    enable_hints_support( Scr.supported_hints, HINTS_Motif );
    enable_hints_support( Scr.supported_hints, HINTS_Gnome );
    enable_hints_support( Scr.supported_hints, HINTS_ExtendedWM );
    enable_hints_support( Scr.supported_hints, HINTS_ASDatabase );
    enable_hints_support( Scr.supported_hints, HINTS_GroupLead );
    enable_hints_support( Scr.supported_hints, HINTS_Transient );

    event_setup( True /*Bool local*/ );

    /* the SizeWindow will be moved into place in LoadASConfig() */
    attr.override_redirect = True;
    attr.bit_gravity = NorthWestGravity;
	Scr.SizeWindow = create_visual_window (Scr.asv, Scr.Root, -999, -999, 10, 10, 0,
										   InputOutput, CWBitGravity | CWOverrideRedirect,
                                           &attr);

    /* create a window which will accept the keyboard focus when no other
	   windows have it */
	attributes.event_mask = KeyPressMask | FocusChangeMask;
	attributes.override_redirect = True;
	Scr.NoFocusWin = create_visual_window (Scr.asv, Scr.Root, -10, -10, 10, 10, 0,
										   InputOnly, CWEventMask | CWOverrideRedirect,
										   &attributes);
	XMapWindow (dpy, Scr.NoFocusWin);
	XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, CurrentTime);


    /* Load config ... */

    /* Reparent all the windows .... */

    /* install pan frames */



    {
        ASEvent event = {0};
        FunctionData restart_func ;
        init_func_data( &restart_func );
        restart_func.func = F_FUNCTION ;
        restart_func.popup = Restarting?Scr.RestartFunction:Scr.InitFunction ;
        if (restart_func.popup)
            ExecuteFunction (&restart_func, &event, -1);
    }
    XDefineCursor (dpy, Scr.Root, Scr.ASCursors[DEFAULT]);

   /* make sure we're on the right desk, and the _WIN_DESK property is set */
    ChangeDesks (Scr.CurrentDesk);

    LOCAL_DEBUG_OUT( "TOTAL SCREENS INITIALIZED : %d", good_screen_count );
#if (defined(LOCAL_DEBUG)||defined(DEBUG)) && defined(DEBUG_ALLOCS)
    LOCAL_DEBUG_OUT( "printing memory%s","");
    spool_unfreed_mem( "afterstep.allocs.startup", NULL );
#endif
    LOCAL_DEBUG_OUT( "entering main loop%s","");

    HandleEvents ();
	return (0);
}





#if 0

    void          InternUsefulAtoms (void);
	void          InitVariables (int);
	int           i;
	extern int    x_fd;
	int           len;
	char          message[255];
	char          num[10];


    set_output_threshold(OUTPUT_LEVEL_PROGRESS);
	for (i = 1; i < argc; i++)
	{
		show_progress("argv[%d] = \"%s\"", i, argv[i]);
		if (!strcmp (argv[i], "--debug"))
		{
			debugging = True;
			set_output_level(OUTPUT_LEVEL_DEBUG);
			set_output_threshold(OUTPUT_LEVEL_DEBUG);
			show_progress("running in debug mode.");
		}else if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "--version"))
		{
			printf ("AfterStep version %s\n", VERSION);
			exit (0);
		} else if ((!strcmp (argv[i], "-c")) || (!strcmp (argv[i], "--config")))
		{
			printf ("AfterStep version %s\n", VERSION);
			printf ("BinDir            %s\n", AFTER_BIN_DIR);
			printf ("ManDir            %s\n", AFTER_MAN_DIR);
			printf ("DocDir            %s\n", AFTER_DOC_DIR);
			printf ("ShareDir          %s\n", AFTER_SHAREDIR);
			printf ("GNUstep           %s\n", GNUSTEP);
			printf ("GNUstepLib        %s\n", GNUSTEPLIB);
			printf ("AfterDir          %s\n", AFTER_DIR);
			printf ("NonConfigDir      %s\n", AFTER_NONCF);
			exit (0);
		} else if (!strcmp (argv[i], "-s") || !strcmp (argv[i], "--single"))
			single = True;
		else if ((!strcmp (argv[i], "-d") || !strcmp (argv[i], "--display")) && i + 1 < argc)
			display_name = argv[++i];
		else if (!strcmp (argv[i], "-f") && i + 1 < argc)
		{
			shall_override_config_file = True;
			config_file_to_override = argv[++i];
		} else
			usage ();
	}

	newhandler (SIGINT);
	newhandler (SIGHUP);
	newhandler (SIGQUIT);
	set_signal_handler (SIGSEGV);
	set_signal_handler (SIGTERM);
	signal (SIGUSR1, Restart);
	set_signal_handler (SIGUSR2);

	signal (SIGPIPE, DeadPipe);

#if 1										   /* see SetTimer() */
	{
		void          enterAlarm (int);

		signal (SIGALRM, enterAlarm);
	}
#endif /* 1 */

	ReapChildren ();

	if (debugging)
		set_synchronous_mode (True);

#ifdef SHAPE
	XShapeQueryExtension (dpy, &ShapeEventBase, &ShapeErrorBase);
#endif /* SHAPE */
	CreateCursors ();
	InitVariables (1);

	/* read config file, set up menus, colors, fonts */
	LoadASConfig (display_name, 0, 1, 1, 1);

/*print_unfreed_mem();
 */

	XSync (dpy, 0);

   /***********************************************************/
#ifndef DONT_GRAB_SERVER                    /* grabbed   !!!!!*/
	XGrabServer (dpy);                		/* grabbed   !!!!!*/
#endif										/* grabbed   !!!!!*/
#ifndef NO_VIRTUAL							/* grabbed   !!!!!*/
    InitPanFrames ();                       /* grabbed   !!!!!*/
#endif /* NO_VIRTUAL */						/* grabbed   !!!!!*/
	CaptureAllWindows ();					/* grabbed   !!!!!*/
#ifndef NO_VIRTUAL							/* grabbed   !!!!!*/
    CheckPanFrames ();                      /* grabbed   !!!!!*/
#endif /* NO_VIRTUAL */						/* grabbed   !!!!!*/
#ifndef DONT_GRAB_SERVER					/* grabbed   !!!!!*/
	XUngrabServer (dpy);					/* UnGrabbed !!!!!*/
#endif										/* UnGrabbed !!!!!*/
	/**********************************************************/
}
#endif

/*** NEW STUFF : *******************************************************/









/*** OLD STUFF : *******************************************************/
/***********************************************************************
 *
 *  Procedure:
 *	MappedNotOverride - checks to see if we should really
 *		put a afterstep frame on the window
 *
 *  Returned Value:
 *	TRUE	- go ahead and frame the window
 *	FALSE	- don't frame the window
 *
 *  Inputs:
 *	w	- the window to check
 *
 ***********************************************************************/

int
MappedNotOverride (Window w)
{
	XWindowAttributes wa;
	Atom          atype;
	int           aformat;
	unsigned long nitems, bytes_remain;
	unsigned char *prop;

	isIconicState = DontCareState;

	if (!XGetWindowAttributes (dpy, w, &wa))
		return False;

	if (XGetWindowProperty (dpy, w, _XA_WM_STATE, 0L, 3L, False, _XA_WM_STATE,
							&atype, &aformat, &nitems, &bytes_remain, &prop) == Success)
	{
		if (prop != NULL)
		{
			isIconicState = *(long *)prop;
			XFree (prop);
		}
	}
	return (((isIconicState == IconicState) || (wa.map_state != IsUnmapped)) &&
			(wa.override_redirect != True));
}


/***********************************************************************
 *
 *  Procedure:
 *      CaptureAllWindows
 *
 *   Decorates all windows at start-up
 *
 ***********************************************************************/

void
CaptureAllWindows (void)
{
	int           i, j;
	unsigned int  nchildren;
	Window        root, parent, *children;
	XPointer      dummy;

	PPosOverride = TRUE;

	if (!XQueryTree (dpy, Scr.Root, &root, &parent, &children, &nchildren))
		return;


	/*
	 * weed out icon windows
	 */

	for (i = 0; i < nchildren; i++)
	{
		if (children[i])
		{
			XWMHints     *wmhintsp = XGetWMHints (dpy, children[i]);

			if (wmhintsp)
			{
				if (wmhintsp->flags & IconWindowHint)
				{
					for (j = 0; j < nchildren; j++)
					{
						if (children[j] == wmhintsp->icon_window)
						{
							children[j] = None;
							break;
						}
					}
				}
				XFree ((char *)wmhintsp);
			}
		}
	}


	/*
	 * map all of the non-override, non-menu windows (menus are handled
	 * elsewhere)
	 */

	for (i = 0; i < nchildren; i++)
	{
		if (children[i] && MappedNotOverride (children[i]) &&
			XFindContext (dpy, children[i], MenuContext, &dummy) == XCNOENT)
		{
			XUnmapWindow (dpy, children[i]);
            AddWindow (children[i]);
		}
	}

	isIconicState = DontCareState;

	if (nchildren > 0)
		XFree ((char *)children);

	/* after the windows already on the screen are in place,
	 * don't use PPosition */
	PPosOverride = FALSE;
}

/***********************************************************************
 *
 *  Procedure:
 *	newhandler: Installs new signal handler
 *
 ************************************************************************/
void
newhandler (int sig)
{
	if (signal (sig, SIG_IGN) != SIG_IGN)
		signal (sig, SigDone);
}

/*************************************************************************
 * Restart on a signal
 ************************************************************************/
void
Restart (int nonsense)
{
	Done (1, MyName);
	SIGNAL_RETURN;
}

/***********************************************************************
 *
 * put_command : write for SaveWindowsOpened
 *
 ************************************************************************/

/*
 * Copyright (c) 1997 Alfredo K. Kojima
 * (taken from savews, GPL, included with WMker)
 */

void
addsavewindow_win (char *client, Window fwin)
{
	int           x, y;
	unsigned int  w, h, b;
	XTextProperty tp;
	int           i;
	static int    first = 1;
	XSizeHints    hints;
	long          rets;
	Window        win, r;
	int           client_argc;
	char        **client_argv = NULL;

	win = XmuClientWindow (dpy, fwin);
	if (win == None)
		return;

	if (!XGetGeometry (dpy, win, &r, &x, &y, &w, &h, &b, &b))
		return;
	if (!XGetGeometry (dpy, fwin, &r, &x, &y, &b, &b, &b, &b))
		return;
	XGetWMClientMachine (dpy, win, &tp);
	if (tp.value == NULL || tp.encoding != XA_STRING || tp.format != 8)
		return;
	if (strcmp ((char *)tp.value, client) != 0)
		return;

	if (!XGetCommand (dpy, win, &client_argv, &client_argc))
		return;
	XGetWMNormalHints (dpy, win, &hints, &rets);
	if (rets & PResizeInc)
	{
		if (rets & PBaseSize)
		{
			w -= hints.base_width;
			h -= hints.base_height;
		}
		if (hints.width_inc <= 0)
			hints.width_inc = 1;
		if (hints.height_inc <= 0)
			hints.height_inc = 1;
		w = w / hints.width_inc;
		h = h / hints.height_inc;
	}
	if (!first)
	{
		fprintf (savewindow_fd, "&\n");
	}
	first = 0;
	for (i = 0; i < client_argc; i++)
	{
		if (strcmp (client_argv[i], "-geometry") == 0)
		{
			if (i <= client_argc - 1)
			{
				i++;
			}
		} else
		{
			fprintf (savewindow_fd, "%s ", client_argv[i]);
		}
	}
	fprintf (savewindow_fd, "-geometry %dx%d+%d+%d ", w, h, x, y);
	XFreeStringList (client_argv);
}

/***********************************************************************
 *
 * SaveWindowsOpened : write their position into a file
 *
 ************************************************************************/

/*
 * Copyright (c) 1997 Alfredo K. Kojima
 * (taken from savews, GPL, included with WMker)
 */

void
SaveWindowsOpened ()
{
	Window       *wins;
	int           i;
	unsigned int  nwins;
	Window        foo;
	char          client[MAXHOSTNAME];
	char         *realsavewindows_file;

	mygethostname (client, MAXHOSTNAME);
	if (!client)
	{
		printf ("Could not get HOST environment variable\nSet it by hand !\n");
		return;
	}
	if (!XQueryTree (dpy, DefaultRootWindow (dpy), &foo, &foo, &wins, &nwins))
	{
		printf ("SaveWindowsOpened : XQueryTree() failed\n");
		return;
	}
	realsavewindows_file = PutHome (AFTER_SAVE);

	if ((savewindow_fd = fopen (realsavewindows_file, "w+")) == NULL)
	{
		free (realsavewindows_file);
		printf ("Can't save file in %s !\n", AFTER_SAVE);;
		return;
	}
	free (realsavewindows_file);
	for (i = 0; i < nwins; i++)
		addsavewindow_win (client, wins[i]);
	XFree (wins);
	fprintf (savewindow_fd, "\n");
	fclose (savewindow_fd);
}

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
        gcv.foreground = XORvalue;
        gcv.subwindow_mode = IncludeInferiors;
        Scr.DrawGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
    }
}

/***********************************************************************
 *  Procedure:
 *	CreateCursors - Loads afterstep cursors
 ***********************************************************************
 */
void
CreateCursors (void)
{
	/* define cursors */
	Scr.ASCursors[POSITION] = XCreateFontCursor (dpy, XC_left_ptr);
/*  Scr.ASCursors[DEFAULT] = XCreateFontCursor(dpy, XC_top_left_arrow); */
	Scr.ASCursors[DEFAULT] = XCreateFontCursor (dpy, XC_left_ptr);
	Scr.ASCursors[SYS] = XCreateFontCursor (dpy, XC_left_ptr);
	Scr.ASCursors[TITLE_CURSOR] = XCreateFontCursor (dpy, XC_left_ptr);
	Scr.ASCursors[MOVE] = XCreateFontCursor (dpy, XC_fleur);
	Scr.ASCursors[MENU] = XCreateFontCursor (dpy, XC_left_ptr);
	Scr.ASCursors[WAIT] = XCreateFontCursor (dpy, XC_watch);
	Scr.ASCursors[SELECT] = XCreateFontCursor (dpy, XC_dot);
	Scr.ASCursors[DESTROY] = XCreateFontCursor (dpy, XC_pirate);
	Scr.ASCursors[LEFT] = XCreateFontCursor (dpy, XC_left_side);
	Scr.ASCursors[RIGHT] = XCreateFontCursor (dpy, XC_right_side);
	Scr.ASCursors[TOP] = XCreateFontCursor (dpy, XC_top_side);
	Scr.ASCursors[BOTTOM] = XCreateFontCursor (dpy, XC_bottom_side);
	Scr.ASCursors[TOP_LEFT] = XCreateFontCursor (dpy, XC_top_left_corner);
	Scr.ASCursors[TOP_RIGHT] = XCreateFontCursor (dpy, XC_top_right_corner);
	Scr.ASCursors[BOTTOM_LEFT] = XCreateFontCursor (dpy, XC_bottom_left_corner);
	Scr.ASCursors[BOTTOM_RIGHT] = XCreateFontCursor (dpy, XC_bottom_right_corner);
}

/***********************************************************************
 *
 *  Procedure:
 *	InitVariables - initialize afterstep variables
 *
 ************************************************************************/
void
InitVariables (int shallresetdesktop)
{
	MenuContext = XUniqueContext ();

	Scr.d_depth = Scr.asv->visual_info.depth;

    Scr.MyDisplayWidth = DisplayWidth (dpy, Scr.screen);
	Scr.MyDisplayHeight = DisplayHeight (dpy, Scr.screen);

    Scr.Windows = init_aswindow_list();

	Scr.VScale = 32;

    ColormapSetup();

	if (shallresetdesktop == 1)
	{
#ifndef NO_VIRTUAL
		Scr.VxMax = 3;
		Scr.VyMax = 3;
#else
		Scr.VxMax = 1;
		Scr.VyMax = 1;
#endif
		Scr.Vx = Scr.Vy = 0;
	}
	/* Sets the current desktop number to zero : multiple desks are available
	 * even in non-virtual compilations
	 */

	if (shallresetdesktop == 1)
	{
		Atom          atype;
		int           aformat;
		unsigned long nitems, bytes_remain;
		unsigned char *prop;

		Scr.CurrentDesk = 0;

		if ((XGetWindowProperty (dpy, Scr.Root, _XA_WIN_DESK, 0L, 1L, True,
								 AnyPropertyType, &atype, &aformat, &nitems,
								 &bytes_remain, &prop)) == Success)
		{
			if (prop != NULL)
			{
				Restarting = True;
				Scr.CurrentDesk = *(unsigned long *)prop;
			}
		}
	}
	Scr.EdgeScrollX = Scr.EdgeScrollY = -100000;
	Scr.ScrollResistance = Scr.MoveResistance = 0;
	Scr.OpaqueSize = 5;
	Scr.ClickTime = 150;
	Scr.AutoRaiseDelay = 0;
	Scr.RaiseButtons = 0;

	Scr.flags = 0;
	Scr.randomx = Scr.randomy = 0;
	Scr.buttons2grab = 7;

/* ßß
   Scr.InitFunction = NULL;
   Scr.RestartFunction = NULL;
 */

	InitModifiers ();

	return;
}

/* Read the server modifier mapping */

void
InitModifiers (void)
{
	int           m, i, knl;
	char         *kn;
	KeySym        ks;
	KeyCode       kc, *kp;
	unsigned      lockmask, *mp;
	XModifierKeymap *mm = XGetModifierMapping (dpy);

	lockmask = LockMask;
	if (mm)
	{
		kp = mm->modifiermap;
		for (m = 0; m < 8; m++)
		{
			for (i = 0; i < mm->max_keypermod; i++)
			{
				if ((kc = *kp++) && ((ks = XKeycodeToKeysym (dpy, kc, 0)) != NoSymbol))
				{
					kn = XKeysymToString (ks);
					knl = strlen (kn);
					if ((knl > 6) && (mystrcasecmp (kn + knl - 4, "lock") == 0))
						lockmask |= (1 << m);
				}
			}
		}
		XFreeModifiermap (mm);
	}
/* forget shift & control locks */
	lockmask &= ~(ShiftMask | ControlMask);

	Scr.nonlock_mods = ((ShiftMask | ControlMask | Mod1Mask | Mod2Mask
						 | Mod3Mask | Mod4Mask | Mod5Mask) & ~lockmask);

	if (Scr.lock_mods == NULL)
		Scr.lock_mods = (unsigned *)safemalloc (256 * sizeof (unsigned));
	mp = Scr.lock_mods;
	for (m = 0, i = 1; i < 256; i++)
	{
		if ((i & lockmask) > m)
			m = *mp++ = (i & lockmask);
	}
	*mp = 0;
}


/***********************************************************************
 *
 *  Procedure:
 *	Reborder - Removes afterstep border windows
 *
 ************************************************************************/

void
Reborder ()
{
	/* remove afterstep frame from all windows */
	XGrabServer (dpy);

    destroy_aswindow_list( &(Scr.Windows), True );
#ifndef NO_TEXTURE
    if (get_flags( Scr.look_flags, DecorateFrames))
		frame_free_data (NULL, True);
#endif /* !NO_TEXTURE */

	XUngrabServer (dpy);
	XSetInputFocus (dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XSync (dpy, 0);
}

/***********************************************************************
 *
 *  Procedure: NoisyExit
 *	Print error messages and die. (segmentation violation)
 *
 **********************************************************************/
void
NoisyExit (int nonsense)
{
	XErrorEvent   event;

	afterstep_err ("Seg Fault", NULL, NULL, NULL);
	event.error_code = 0;
	event.request_code = 0;
	ASErrorHandler (dpy, &event);

	/* Attempt to do a re-start of afterstep */
	Done (0, NULL);
}





/***********************************************************************
 *
 *  Procedure:
 *	Done - cleanup and exit afterstep
 *
 ***********************************************************************
 */
void
SigDone (int nonsense)
{
	Done (0, NULL);
	SIGNAL_RETURN;
}

void
Done (int restart, char *command)
{
    set_flags( Scr.state, AS_StateShutdown );
    if( restart )
        set_flags( Scr.state, AS_StateRestarting );
#ifndef NO_VIRTUAL
	MoveViewport (0, 0, False);
#endif
#ifndef NO_SAVEWINDOWS
	if (!restart)
		SaveWindowsOpened ();
#endif


	/* remove window frames */
	Reborder ();

	/* Close all my pipes */
	ClosePipes ();

	/* freeing up memory */
	InitASDirNames (True);
	free_func_hash ();

#ifdef HAVE_XINERAMA
	if (Scr.xinerama_screens)
	{
		free (Scr.xinerama_screens);
		Scr.xinerama_screens_num = 0;
		Scr.xinerama_screens = NULL;
	}
#endif /* XINERAMA */

	if (restart)
	{
		/* Really make sure that the connection is closed and cleared! */
		XSelectInput (dpy, Scr.Root, 0);
		XSync (dpy, 0);
		XCloseDisplay (dpy);

		{
			char         *my_argv[10];
			int           i = 0;

			sleep (1);
			ReapChildren ();

			my_argv[i++] = command;

			if (strstr (command, "afterstep") != NULL)
			{
				my_argv[0] = MyName;

				if (single)
					my_argv[i++] = "-s";

				if (shall_override_config_file)
				{
					my_argv[i++] = "-f";
					my_argv[i++] = config_file_to_override;
				}
			}

			while (i < 10)
				my_argv[i++] = NULL;

			execvp (my_argv[0], my_argv);
			fprintf (stderr, "AfterStep: Call of '%s' failed!!!!\n", my_argv[0]);
		}
	} else
	{
		extern Atom   _XA_WIN_DESK;

		XDeleteProperty (dpy, Scr.Root, _XA_WIN_DESK);

#ifdef DEBUG_ALLOCS
		{									   /* free up memory */
			extern char  *global_base_file;

			/* free display strings; can't do this in main(), because some OS's
			 * don't copy the environment variables properly */
			free (display_string);
			free (rdisplay_string);

			free (Scr.lock_mods);
			/* module stuff */
			module_init (1);
			free (global_base_file);
			/* configure stuff */
			init_old_look_variables (True);
			InitBase (True);
			InitLook (True);
			InitFeel (True);
			InitDatabase (True);

            if( Scr.Popups )
                destroy_ashash( &Scr.Popups );
            if( Scr.ComplexFunctions )
                destroy_ashash( &Scr.ComplexFunctions );
            /* global drawing GCs */
			if (Scr.DrawGC != NULL)
				XFreeGC (dpy, Scr.DrawGC);
			/* balloons */
			balloon_init (1);
			/* pixmap references */
			pixmap_ref_purge ();
            build_xpm_colormap (NULL);
		}
		print_unfreed_mem ();
#endif /*DEBUG_ALLOCS */

		XCloseDisplay (dpy);

		exit (0);
	}
}

/***********************************************************************
 *
 *  Procedure:
 *	CatchRedirectError - Figures out if there's another WM running
 *
 ************************************************************************/
XErrorHandler
CatchRedirectError (Display * dpy, XErrorEvent * event)
{
	afterstep_err ("Another Window Manager is running", NULL, NULL, NULL);
	exit (1);
}


void
afterstep_err (const char *message, const char *arg1, const char *arg2, const char *arg3)
{
	fprintf (stderr, "AfterStep: ");
	fprintf (stderr, message, arg1, arg2, arg3);
	fprintf (stderr, "\n");
}

void
usage (void)
{
	fprintf (stderr,
			 "AfterStep v. %s\n\nusage: afterstep [-v|--version] [-c|--config] [-d dpy] [--debug] [-f old_config_file] [-s]\n",
			 VERSION);
	exit (-1);
}


