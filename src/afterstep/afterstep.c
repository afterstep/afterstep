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


#include "asinternals.h"
#include "../../libAfterConf/afterconf.h"

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#include <X11/cursorfont.h>

#include "../../libAfterStep/session.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterStep/moveresize.h"


/**************************************************************************/
/* 		Global variables that defines our behaviour 		  */
/**************************************************************************/
/* our status */
ASFlagType AfterStepState = 0;	/* default status */

#define ASSF_BypassAutoexec	(0x01<<0)
ASFlagType AfterStepStartupFlags = 0;

/* DBUS stuff is separated into dbus.c */
int ASDBus_fd = -1;
char *GnomeSessionClientID = NULL;

/* Config : */
ASVector *Modules = NULL;
int Module_fd = 0;
int Module_npipes = 8;

ASBalloonState *MenuBalloons = NULL;
ASBalloonState *TitlebarBalloons = NULL;

char *original_DISPLAY_string = NULL;

char *SMClientID_string = NULL;

/**************************************************************************/
void SetupScreen ();
void CleanupScreen ();

void IgnoreSignal (int sig);
SIGNAL_T Restart (int nonsense);
SIGNAL_T SigDone (int nonsense);

void CaptureAllWindows (ScreenInfo * scr);
void DoAutoexec (Bool restarting);
void DeadPipe (int);

Bool afterstep_parent_hints_func (Window parent, ASParentHints * dst);

#ifdef __CYGWIN__
void ASCloseOnExec ()
{
	LOCAL_DEBUG_OUT ("pid(%d),shutting down modukes...", getpid ());
	ShutdownModules (True);
	LOCAL_DEBUG_OUT ("pid(%d),closing x_fd...", getpid ());
	if (x_fd)
		close (x_fd);
	LOCAL_DEBUG_OUT ("pid(%d),complete...", getpid ());
}
#endif
#ifdef DEBUG_TRACE_X
const char *window_id2name (Display * dpy, Window w, int *how_to_free)
{
	ASWindow *asw = window2ASWindow (w);
	if (asw) {
		*how_to_free = 1;
		return mystrdup (ASWIN_NAME (asw));
	}
	return NULL;
}
#endif

CommandLineOpts AfterStep_cmdl_options[3] = {
	{NULL, "bypass-autoexec", "Bypass running autoexec on startup", NULL,
	 handler_set_flag, &AfterStepStartupFlags, ASSF_BypassAutoexec, 0}
	,
	{NULL, "sm-client-id", "Session manager client's ID", NULL,
	 handler_set_string, &SMClientID_string, 0, CMO_HasArgs}
	,
	{NULL, NULL, NULL, NULL, NULL, NULL, 0}
};


void AfterStep_usage (void)
{
	printf (OPTION_USAGE_FORMAT " [additional options]\n", MyName);
	print_command_line_opt ("standard_options are ",
													as_standard_cmdl_options, 0);
	print_command_line_opt ("additional options are ",
													AfterStep_cmdl_options, 0);
	exit (0);
}


/**************************************************************************/
/**************************************************************************/
/***********************************************************************
 *  Procedure:
 *	main - start of afterstep
 ************************************************************************/
int main (int argc, char **argv, char **envp)
{
	register int i;

	int start_viewport_x = 0;
	int start_viewport_y = 0;
	int start_desk = 0;

#ifdef LOCAL_DEBUG
#if 0
	LOCAL_DEBUG_OUT ("calibrating sleep_a_millisec : %s", "");
	for (i = 0; i < 500; ++i)
		sleep_a_millisec (10);
	LOCAL_DEBUG_OUT ("500 sliip_a_millisec(10) completed%s", "");
	for (i = 0; i < 50; ++i)
		sleep_a_millisec (100);
	LOCAL_DEBUG_OUT ("50 sliip_a_millisec(100) completed%s", "");
	for (i = 0; i < 10; ++i)
		sleep_a_millisec (300);
	LOCAL_DEBUG_OUT ("10 sliip_a_millisec(300) completed%s", "");
#endif
#endif

	_as_grab_screen_func = GrabEm;
	_as_ungrab_screen_func = UngrabEm;

	original_DISPLAY_string = getenv ("DISPLAY");
	if (original_DISPLAY_string)
		original_DISPLAY_string = mystrdup (original_DISPLAY_string);

#ifdef DEBUG_TRACE_X
	trace_window_id2name_hook = &window_id2name;
#endif
	set_DeadPipe_handler (DeadPipe);

#if !HAVE_DECL_ENVIRON
	override_environ (envp);
#endif
	InitMyApp (CLASS_AFTERSTEP, argc, argv, NULL, AfterStep_usage, 0);

	LinkAfterStepConfig ();

	AfterStepState = MyArgs.flags;
	clear_flags (AfterStepState, ASS_NormalOperation);
	set_flags (AfterStepState, ASS_SuppressDeskBack);

#ifdef __CYGWIN__
	CloseOnExec = ASCloseOnExec;
#endif

#if defined(LOG_FONT_CALLS)
	fprintf (stderr, "logging font calls now\n");
#endif

	/* These signals are mandatory : */
	signal (SIGUSR1, Restart);
	/* These signals we would like to handle only if those are not handled already (by debugger): */
	IgnoreSignal (SIGINT);
	IgnoreSignal (SIGHUP);
	IgnoreSignal (SIGQUIT);
	IgnoreSignal (SIGTERM);

	if (ConnectX (ASDefaultScr, AS_ROOT_EVENT_MASK) < 0) {
		show_error ("Hostile X server encountered - unable to proceed :-(");
		return 1;										/* failed to accure window management selection - other wm is running */
	}


	ASDBus_fd = asdbus_init ();

	XSetWindowBackground (dpy, Scr.Root, Scr.asv->black_pixel);
	Scr.Look.desktop_animation_tint = get_random_tint_color ();

	cover_desktop ();
	if (get_flags (AfterStepState, ASS_Restarting)) {
		show_progress ("AfterStep v.%s is restarting ...", VERSION);
		display_progress (True, "AfterStep v.%s is restarting ...", VERSION);
	} else {
		show_progress ("AfterStep v.%s is starting up ...", VERSION);
		display_progress (True, "AfterStep v.%s is starting up ...", VERSION);
	}

	if (ASDBus_fd >= 0) {
		show_progress ("Successfuly accured Session DBus connection.");
		GnomeSessionClientID = asdbus_RegisterSMClient (SMClientID_string);
		if (GnomeSessionClientID != NULL) {
			show_progress
					("Successfuly registered with GNOME Session Manager with Client Path \"%s\".",
					 GnomeSessionClientID);
			change_func_code ("Restart", F_NOP);
			change_func_code ("QuitWM", F_NOP);
			if (!CanShutdown())
				change_func_code ("SystemShutdown", F_NOP);
		} else {
			change_func_code ("SystemShutdown", F_NOP);
			change_func_code ("Logout", F_NOP);
		}
	}
	/* these use UPower which is sitting on system bus, independent from ASDBus_fd */
	if (!CanSuspend())
		change_func_code ("Suspend", F_NOP);
	if (!CanHibernate())
		change_func_code ("Hibernate", F_NOP);

	SHOW_CHECKPOINT;
	InitSession ();
	SHOW_CHECKPOINT;
	XSync (dpy, 0);
	SHOW_CHECKPOINT;
	set_parent_hints_func (afterstep_parent_hints_func);	/* callback for collect_hints() */
	SHOW_CHECKPOINT;
	SetupModules ();
	SHOW_CHECKPOINT;
	SetupScreen ();
	SHOW_CHECKPOINT;
	event_setup (True /*Bool local */ );
	SHOW_CHECKPOINT;
	/*
	 *  Lets init each and every screen separately :
	 */

	for (i = 0; i < Scr.NumberOfScreens; i++) {
		show_progress ("Initializing screen %d ...", i);
		display_progress (True, "Initializing screen %d ...", i);

		if (i != Scr.screen) {
			if (!get_flags (MyArgs.flags, ASS_SingleScreen)) {
				int pid =
						spawn_child (MyName, (i < MAX_USER_SINGLETONS_NUM) ? i : -1, i,
												 NULL, None, C_NO_CONTEXT, True, True, NULL);
				if (pid >= 0)
					show_progress ("\t instance of afterstep spawned with pid %d.",
												 pid);
				else
					show_error
							("failed to launch instance of afterstep to handle screen #%d",
							 i);
			}
		} else {
			make_screen_envvars (ASDefaultScr);
			putenv (Scr.rdisplay_string);
			putenv (Scr.display_string);
			if (is_output_level_under_threshold (OUTPUT_LEVEL_PROGRESS)) {
				show_progress ("\t screen[%d].size = %ux%u", Scr.screen,
											 Scr.MyDisplayWidth, Scr.MyDisplayHeight);
				display_progress (True, "    screen[%d].size = %ux%u", Scr.screen,
													Scr.MyDisplayWidth, Scr.MyDisplayHeight);
				show_progress ("\t screen[%d].root = %lX", Scr.screen, Scr.Root);
				show_progress ("\t screen[%d].color_depth = %d", Scr.screen,
											 Scr.asv->true_depth);
				display_progress (True, "    screen[%d].color_depth = %d",
													Scr.screen, Scr.asv->true_depth);
				show_progress ("\t screen[%d].colormap    = 0x%lX", Scr.screen,
											 Scr.asv->colormap);
				show_progress ("\t screen[%d].visual.id         = %X", Scr.screen,
											 Scr.asv->visual_info.visualid);
				display_progress (True, "    screen[%d].visual.id         = %X",
													Scr.screen, Scr.asv->visual_info.visualid);
				show_progress ("\t screen[%d].visual.class      = %d", Scr.screen,
											 Scr.asv->visual_info.class);
				display_progress (True, "    screen[%d].visual.class      = %d",
													Scr.screen, Scr.asv->visual_info.class);
				show_progress ("\t screen[%d].visual.red_mask   = 0x%8.8lX",
											 Scr.screen, Scr.asv->visual_info.red_mask);
				show_progress ("\t screen[%d].visual.green_mask = 0x%8.8lX",
											 Scr.screen, Scr.asv->visual_info.green_mask);
				show_progress ("\t screen[%d].visual.blue_mask  = 0x%8.8lX",
											 Scr.screen, Scr.asv->visual_info.blue_mask);
				show_progress ("\t screen[%d].rdisplay_string = \"%s\"",
											 Scr.screen, Scr.rdisplay_string);
				show_progress ("\t screen[%d].display_string = \"%s\"", Scr.screen,
											 Scr.display_string);
				display_progress (True, "    screen[%d].display_string = \"%s\"",
													Scr.screen, Scr.display_string);
			}
		}
	}

	/* make sure we're on the right desk, and the _WIN_DESK property is set */
	Scr.CurrentDesk = INVALID_DESK;
	if (get_flags (Scr.wmprops->set_props, WMC_ASDesks)) {
		start_desk = Scr.wmprops->as_current_desk;
	} else if (get_flags (Scr.wmprops->set_props, WMC_DesktopCurrent)) {
		int curr = Scr.wmprops->desktop_current;
		start_desk = curr;
		if (get_flags (Scr.wmprops->set_props, WMC_DesktopViewport) &&
				curr < Scr.wmprops->desktop_viewports_num) {
			/* we have to do that prior to capturing any window so that they'll get in
			 * correct position and will not end up outside of the screen */
			start_viewport_x = Scr.wmprops->desktop_viewport[curr << 1];
			start_viewport_y = Scr.wmprops->desktop_viewport[(curr << 1) + 1];
		}
	}
	if (get_flags (Scr.wmprops->set_props, WMC_ASViewport)) {
		start_viewport_x = Scr.wmprops->as_current_vx;
		start_viewport_y = Scr.wmprops->as_current_vy;
	}
	/* temporarily setting up desktop 0 */
	ChangeDesks (0);

	/* Load config ... */
	/* read config file, set up menus, colors, fonts */
	LoadASConfig (0, PARSE_EVERYTHING);

	/* Reparent all the windows and setup pan frames : */
	XSync (dpy, 0);
	 /***********************************************************/
#ifndef DONT_GRAB_SERVER				/* grabbed   !!!!! */
	grab_server ();								/* grabbed   !!!!! */
#endif													/* grabbed   !!!!! */
	init_screen_panframes (ASDefaultScr);	/* grabbed   !!!!! */
	display_progress (True, "Capturing all windows ...");
	CaptureAllWindows (ASDefaultScr);	/* grabbed   !!!!! */
	display_progress (False, "Done.");
	check_screen_panframes (ASDefaultScr);	/* grabbed   !!!!! */
	ASSync (False);
#ifndef DONT_GRAB_SERVER				/* grabbed   !!!!! */
	ungrab_server ();							/* UnGrabbed !!!!! */
#endif													/* UnGrabbed !!!!! */
	/**********************************************************/
	XDefineCursor (dpy, Scr.Root, Scr.Feel.cursors[ASCUR_Default]);

	display_progress (True, "Seting initial viewport to %+d%+d ...",
										Scr.wmprops->as_current_vx,
										Scr.wmprops->as_current_vy);

	SetupFunctionHandlers ();
	display_progress (True, "Processing all pending events ...");
	ConfigureNotifyLoop ();
	display_progress (True, "All done.");
	remove_desktop_cover ();

	if (!get_flags (AfterStepStartupFlags, ASSF_BypassAutoexec))
		DoAutoexec (get_flags (AfterStepState, ASS_Restarting));

	/* once all the windows are swallowed and placed in its proper desks - we cas restore proper
	   desktop/viewport : */
	clear_flags (AfterStepState, ASS_SuppressDeskBack);
	/* all system Go! we are completely Operational! */
	set_flags (AfterStepState, ASS_NormalOperation);
	ChangeDeskAndViewport (start_desk, start_viewport_x, start_viewport_y, False);

#if (defined(LOCAL_DEBUG)||defined(DEBUG)) && defined(DEBUG_ALLOCS)
	LOCAL_DEBUG_OUT ("printing memory%s", "");
	spool_unfreed_mem ("afterstep.allocs.startup", NULL);
#endif
	LOCAL_DEBUG_OUT ("entering main loop%s", "");

	HandleEvents ();
	return (0);
}

/*************************************************************************/
/* Our Screen initial state setup and initialization of management data :*/
/*************************************************************************/
void CreateCursors (void)
{
	/* define cursors */
	Scr.standard_cursors[ASCUR_Position] =
			XCreateFontCursor (dpy, XC_left_ptr);
/*  Scr.ASCursors[DEFAULT] = XCreateFontCursor(dpy, XC_top_left_arrow); */
	Scr.standard_cursors[ASCUR_Default] =
			XCreateFontCursor (dpy, XC_left_ptr);
	Scr.standard_cursors[ASCUR_Sys] = XCreateFontCursor (dpy, XC_left_ptr);
	Scr.standard_cursors[ASCUR_Title] = XCreateFontCursor (dpy, XC_left_ptr);
	Scr.standard_cursors[ASCUR_Move] = XCreateFontCursor (dpy, XC_fleur);
	Scr.standard_cursors[ASCUR_Menu] = XCreateFontCursor (dpy, XC_left_ptr);
	Scr.standard_cursors[ASCUR_Wait] = XCreateFontCursor (dpy, XC_watch);
	Scr.standard_cursors[ASCUR_Select] = XCreateFontCursor (dpy, XC_dot);
	Scr.standard_cursors[ASCUR_Destroy] = XCreateFontCursor (dpy, XC_pirate);
	Scr.standard_cursors[ASCUR_Left] = XCreateFontCursor (dpy, XC_left_side);
	Scr.standard_cursors[ASCUR_Right] =
			XCreateFontCursor (dpy, XC_right_side);
	Scr.standard_cursors[ASCUR_Top] = XCreateFontCursor (dpy, XC_top_side);
	Scr.standard_cursors[ASCUR_Bottom] =
			XCreateFontCursor (dpy, XC_bottom_side);
	Scr.standard_cursors[ASCUR_TopLeft] =
			XCreateFontCursor (dpy, XC_top_left_corner);
	Scr.standard_cursors[ASCUR_TopRight] =
			XCreateFontCursor (dpy, XC_top_right_corner);
	Scr.standard_cursors[ASCUR_BottomLeft] =
			XCreateFontCursor (dpy, XC_bottom_left_corner);
	Scr.standard_cursors[ASCUR_BottomRight] =
			XCreateFontCursor (dpy, XC_bottom_right_corner);
}

void CreateManagementWindows ()
{
	XSetWindowAttributes attr;		/* attributes for create windows */
	/* the SizeWindow will be moved into place in LoadASConfig() */
	attr.override_redirect = True;
	attr.bit_gravity = NorthWestGravity;
	Scr.SizeWindow =
			create_visual_window (Scr.asv, Scr.Root, -999, -999, 10, 10, 0,
														InputOutput, CWBitGravity | CWOverrideRedirect,
														&attr);
	LOCAL_DEBUG_OUT ("Scr.SizeWindow = %lX;", Scr.SizeWindow);
	/* create a window which will accept the keyboard focus when no other
	   windows have it */
	attr.event_mask = KeyPressMask | FocusChangeMask | AS_ROOT_EVENT_MASK;
	attr.override_redirect = True;
	Scr.ServiceWin = create_visual_window (Scr.asv, Scr.Root, 0, 0, 1, 1, 0,
																				 InputOutput,
																				 CWEventMask | CWOverrideRedirect,
																				 &attr);
	set_service_window_prop (Scr.wmprops, Scr.ServiceWin);
	LOCAL_DEBUG_OUT ("Scr.ServiceWin = %lX;", Scr.ServiceWin);
	XMapRaised (dpy, Scr.ServiceWin);
	XSetInputFocus (dpy, Scr.ServiceWin, RevertToParent, CurrentTime);
/*    show_progress( "Service window created with ID %lX", Scr.ServiceWin ); */
}

void DestroyManagementWindows ()
{
	if (Scr.ServiceWin)
		XDestroyWindow (dpy, Scr.ServiceWin);
	Scr.ServiceWin = None;
	if (Scr.SizeWindow)
		XDestroyWindow (dpy, Scr.SizeWindow);
	Scr.SizeWindow = None;
}

/***********************************************************************
 *  Procedure:
 *  Setup Screen - main function
 ************************************************************************/
void SetupScreen ()
{
	Scr.Look.magic = MAGIC_MYLOOK;
	InitLook (&Scr.Look, False);
	InitFeel (&Scr.Feel, False);

	Scr.Vx = Scr.Vy = 0;
	Scr.CurrentDesk = 0;

	Scr.randomx = Scr.randomy = 0;
	Scr.RootCanvas = create_ascanvas_container (Scr.Root);

	SetupColormaps ();
	CreateCursors ();
	CreateManagementWindows ();
	Scr.Windows = init_aswindow_list ();

	XSelectInput (dpy, Scr.Root, AS_ROOT_EVENT_MASK);

	MenuBalloons = create_balloon_state ();
	TitlebarBalloons = create_balloon_state ();
}

void CleanupScreen ()
{
	int i;

	if (Scr.Windows) {
		grab_server ();
		destroy_aswindow_list (&(Scr.Windows), True);
		ungrab_server ();
	}

	destroy_balloon_state (&TitlebarBalloons);
	destroy_balloon_state (&MenuBalloons);

	release_all_old_background (True);

	DestroyManagementWindows ();
	CleanupColormaps ();

	if (Scr.RootCanvas)
		destroy_ascanvas (&(Scr.RootCanvas));

	XSetInputFocus (dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XSync (dpy, 0);

#ifdef HAVE_XINERAMA
	if (Scr.xinerama_screens) {
		free (Scr.xinerama_screens);
		Scr.xinerama_screens_num = 0;
		Scr.xinerama_screens = NULL;
	}
#endif													/* XINERAMA */


	for (i = 0; i < MAX_CURSORS; ++i)
		if (Scr.standard_cursors[i]) {
			XFreeCursor (dpy, Scr.standard_cursors[i]);
			Scr.standard_cursors[i] = None;
		}

	InitLook (&Scr.Look, True);
	InitFeel (&Scr.Feel, True);


	/* free display strings; can't do this in main(), because some OS's
	 * don't copy the environment variables properly */
	if (Scr.display_string) {
		free (Scr.display_string);
		Scr.display_string = NULL;
	}
	if (Scr.rdisplay_string) {
		free (Scr.rdisplay_string);
		Scr.rdisplay_string = NULL;
	}

	if (Scr.RootBackground) {
		if (Scr.RootBackground->pmap) {
			if (Scr.wmprops->root_pixmap == Scr.RootBackground->pmap) {
				set_xrootpmap_id (Scr.wmprops, None);
				set_as_background (Scr.wmprops, None);
			}
			XFreePixmap (dpy, Scr.RootBackground->pmap);
			ASSync (False);
			LOCAL_DEBUG_OUT ("root pixmap with id %lX destroyed",
											 Scr.RootBackground->pmap);
			Scr.RootBackground->pmap = None;
		}
		free (Scr.RootBackground);
	}
	LOCAL_DEBUG_OUT ("destroying image manager : %p", Scr.image_manager);
	destroy_image_manager (Scr.image_manager, False);
	LOCAL_DEBUG_OUT ("destroying font manager : %p", Scr.font_manager);
	destroy_font_manager (Scr.font_manager, False);

	LOCAL_DEBUG_OUT ("destroying visual : %p", Scr.asv);
	destroy_screen_gcs (ASDefaultScr);
	destroy_asvisual (Scr.asv, False);

	LOCAL_DEBUG_OUT ("selecting input mask for Root window to 0 : %s", "");
	/* Must release SubstructureRedirectMask prior to releasing wm selection in
	 * destroy_wmprops() : */
	XSelectInput (dpy, Scr.Root, 0);
	XUngrabPointer (dpy, CurrentTime);
	XUngrabButton (dpy, AnyButton, AnyModifier, Scr.Root);

	LOCAL_DEBUG_OUT ("destroying wmprops : %p", Scr.wmprops);
	/* this must be done at the very end !!!! */
	destroy_wmprops (Scr.wmprops, False);
	LOCAL_DEBUG_OUT ("screen cleanup complete.%s", "");
}

/*************************************************************************/
/* populating windowlist with presently available windows :
 * (backported from as-devel)
 */
/*************************************************************************/
void CaptureAllWindows (ScreenInfo * scr)
{
	int i;
	unsigned int nchildren;
	Window root, parent, *children;
	Window focused = None;
	int revert_to = RevertToNone;
	XWindowAttributes attr;

	if (scr == NULL)
		return;
	focused = XGetInputFocus (dpy, &focused, &revert_to);

	if (!XQueryTree (dpy, scr->Root, &root, &parent, &children, &nchildren))
		return;

	/* weed out icon windows : */
	for (i = 0; i < nchildren; i++)
		if (children[i]) {
			XWMHints *wmhintsp = NULL;
			if ((wmhintsp = XGetWMHints (dpy, children[i])) != NULL) {
				if (get_flags (wmhintsp->flags, IconWindowHint)) {
					register int j;
					for (j = 0; j < nchildren; j++)
						if (children[j] == wmhintsp->icon_window) {
							children[j] = None;
							break;
						}
				}
				XFree ((char *)wmhintsp);
			}
		}

	/* map all the rest of the windows : */
	for (i = 0; i < nchildren; i++)
		if (children[i]) {
			long nitems = 0;
			CARD32 *state_prop = NULL;
			int wm_state = DontCareState;
			int k;
			for (k = 0; k < 4; ++k)
				if (children[i] == Scr.PanFrame[k].win) {
					children[i] = None;
					break;
				}

			if (children[i] == None)
				continue;

			if (children[i] == Scr.SizeWindow || children[i] == Scr.ServiceWin)
				continue;

			if (window2ASWindow (children[i]))
				continue;
			/* weed out override redirect windows and unmapped windows : */
			if (!XGetWindowAttributes (dpy, children[i], &attr))
				continue;
			if (attr.override_redirect)
				continue;
			if (read_32bit_proplist
					(children[i], _XA_WM_STATE, 2, &state_prop, &nitems)) {
				wm_state = state_prop[0];
				free (state_prop);
			}
			if ((wm_state == IconicState) || (attr.map_state != IsUnmapped)) {
				LOCAL_DEBUG_OUT ("adding window %lX", children[i]);
				AddWindow (children[i], False);
			}
		}

	if (children)
		XFree ((char *)children);

	if (focused != None && focused != PointerRoot) {
		ASWindow *t = window2ASWindow (focused);
		if (t)
			activate_aswindow (t, False, False);
	}
}

void CloseAllWindows ()
{

}


/***********************************************************************
 * running Autoexec code ( if any ) :
 ************************************************************************/
void DoAutoexec (Bool restarting)
{
	FunctionData func;
	ASEvent event = { 0 };
	char screen_func_name[128];

	init_func_data (&func);
	func.func = F_FUNCTION;
	func.name = restarting ? "RestartFunction" : "InitFunction";
	if (Scr.screen > 0) {
		sprintf (screen_func_name,
						 restarting ? "RestartScreen%ldFunction" :
						 "InitScreen%ldFunction", Scr.screen);
		if (find_complex_func
				(Scr.Feel.ComplexFunctions, &(screen_func_name[0])) != NULL)
			func.name = &(screen_func_name[0]);
	}
	ExecuteFunction (&func, &event, -1);
	HandleEventsWhileFunctionsPending ();
}

/***********************************************************************
 * our signal handlers :
 ************************************************************************/
void IgnoreSignal (int sig)
{
	if (signal (sig, SIG_IGN) != SIG_IGN)
		signal (sig, SigDone);
}

void Restart (int nonsense)
{
	Done (True, NULL);
	SIGNAL_RETURN;
}

void SigDone (int nonsense)
{
	Done (False, NULL);
	SIGNAL_RETURN;
}

/*************************************************************************/
/* our shutdown function :                                               */
/*************************************************************************/

Bool RequestLogout ()
{
	Bool requested = False;
	if (GnomeSessionClientID)
		requested = asdbus_Logout (0, 500);
	return requested;
}

Bool CanShutdown ()
{
	return (GnomeSessionClientID && asdbus_GetCanShutdown ());
}

Bool CanSuspend ()
{
	return (asdbus_GetCanSuspend ());
}
Bool CanHibernate ()
{
	return (asdbus_GetCanHibernate ());
}

Bool CanRestart ()
{
	return (GnomeSessionClientID == NULL);
}

Bool CanLogout ()
{
	return (GnomeSessionClientID != NULL);
}

Bool CanQuit ()
{
	return (GnomeSessionClientID == NULL);
}

Bool RequestShutdown (FunctionCode kind)
{
	Bool requested = False;
	switch(kind)
	{
		case F_SYSTEM_SHUTDOWN : 
			if (GnomeSessionClientID && asdbus_GetCanShutdown ())
				requested = asdbus_Shutdown (500);
			break;
		case F_SUSPEND : 
			if (asdbus_GetCanSuspend ())
				requested = asdbus_Suspend (500); 
			break;
		case F_HIBERNATE : 
			if (asdbus_GetCanHibernate ())
				requested = asdbus_Hibernate (500); 
			break;
	}
	return requested;
}

void SaveSession (Bool force)
{
#ifndef NO_SAVEWINDOWS
	static Bool saved = False;
	if (!saved || force) {
		char *fname = make_session_file (Session, AFTER_SAVE, False);
		save_aswindow_list (Scr.Windows, fname, get_gnome_autosave ());
		free (fname);
		saved = True;
	}
#endif
}

static void CloseSessionRetryHandler (void *data)
{
	CloseSessionClients (data != NULL);
}


void CloseSessionClients (Bool only_modules)
{
	int modules_killed;
	/* Its the end of the session and we better close all non-module windows 
	   that support the protocol. Otherwise they'll just crash when X connection goes down. 
	 */
	show_progress ("Closing down all modules ...");
	display_progress (True, "Closing down all modules ...");
	/* keep datastructures operational since we could still be inside event loop */
	modules_killed = KillAllModules ();

	if (!only_modules) {
		show_progress ("Session end: Closing down all remaining windows ...");
		display_progress (True,
											"Session end: Closing down all remaining windows ...");
		close_aswindow_list (Scr.Windows, True);
		ASFlushAndSync ();
		sleep_a_millisec (100);
	}
/*	if (modules_killed > 0) */
	ASHashData d;
	d.i = only_modules;
	timer_new (100, &CloseSessionRetryHandler, d.vptr);
}

void Done (Bool restart, char *command)
{
	int restart_screen =
			get_flags (AfterStepState, ASS_SingleScreen) ? Scr.screen : -1;
	Bool restart_self = False;
	char *local_command = NULL;
	{
		static int already_dead = False;
		if (already_dead)
			return;										/* non-reentrant function ! */
		already_dead = True;
	}

	/* lets duplicate the string so we don't accidentaly delete it while closing self down */
	if (restart) {
		int my_name_len = strlen (MyName);
		if (command) {
			if (strncmp (command, MyName, my_name_len) == 0)
				restart_self = (command[my_name_len] == ' '
												|| command[my_name_len] == '\0');
			local_command = mystrdup (command);
		} else {
			local_command = mystrdup (MyName);
			restart_self = True;
		}
		if (!is_executable_in_path (local_command)) {
			if (!restart_self || MyArgs.saved_argv[0] == NULL) {
				show_error
						("Cannot restart with command \"%s\" - application is not in PATH!",
						 local_command);
				return;
			}
			free (local_command);
			if (command) {
				local_command =
						safemalloc (strlen (command) + 1 +
												strlen (MyArgs.saved_argv[0]) + 1);
				sprintf (local_command, "%s %s", MyArgs.saved_argv[0],
								 command + my_name_len);
			} else
				local_command = mystrdup (MyArgs.saved_argv[0]);
		}
	}

#ifdef XSHMIMAGE
	/* may not need to do that as server may still have some of the shared 
	 * memory and work in it */
	flush_shm_cache ();
#endif

	LOCAL_DEBUG_CALLER_OUT ("%s restart, cmd=\"%s\"",
													restart ? "Do" : "Don't",
													command ? command : "");

	XSelectInput (dpy, Scr.Root, 0);
	SendPacket (-1, M_SHUTDOWN, 0);
	FlushAllQueues ();
	sleep_a_millisec (1000);

	LOCAL_DEBUG_OUT ("local_command = \"%s\", restart_self = %s",
									 local_command, restart_self ? "Yes" : "No");
	set_flags (AfterStepState, ASS_Shutdown);
	if (restart)
		set_flags (AfterStepState, ASS_Restarting);
	clear_flags (AfterStepState, ASS_NormalOperation);
#ifndef NO_VIRTUAL
	MoveViewport (0, 0, False);
#endif

	if (!restart)
		SaveSession (False);

	/* Close all my pipes */
	show_progress ("Shuting down Modules subsystem ...");
	ShutdownModules (False);

	CloseSessionClients (GnomeSessionClientID == NULL || restart);

	desktop_cover_cleanup ();

	/* remove window frames */
	CleanupScreen ();
	/* Really make sure that the connection is closed and cleared! */
	XSync (dpy, 0);

	if (ASDBus_fd >= 0) {
		if (GnomeSessionClientID != NULL)
			asdbus_UnregisterSMClient (GnomeSessionClientID);
		asdbus_shutdown ();
	}
#ifdef XSHMIMAGE
	flush_shm_cache ();
#endif
	if (restart) {
		set_flags (MyArgs.flags, ASS_Restarting);
		spawn_child (local_command, -1, restart_screen,
								 original_DISPLAY_string, None, C_NO_CONTEXT, False,
								 restart_self, NULL);
	} else {

		XCloseDisplay (dpy);
		dpy = NULL;

		/* freeing up memory */
		DestroyPendingFunctionsQueue ();
		DestroyCategories ();

		cleanup_default_balloons ();
		destroy_asdatabase ();
		destroy_assession (Session);
		destroy_asenvironment (&Environment);
		/* pixmap references */
		build_xpm_colormap (NULL);

		free_scratch_ids_vector ();
		free_scratch_layers_vector ();
		clientprops_cleanup ();
		wmprops_cleanup ();

		free_func_hash ();
		flush_keyword_ids ();
		purge_asimage_registry ();

		asxml_var_cleanup ();
		custom_color_cleanup ();

		free_as_app_args ();
		free (ASDefaultScr);

		flush_default_asstorage ();
		flush_asbidirlist_memory_pool ();
		flush_ashash_memory_pool ();
#ifdef DEBUG_ALLOCS
		print_unfreed_mem ();
#endif													/*DEBUG_ALLOCS */
#ifdef XSHMIMAGE
		flush_shm_cache ();
#endif

	}
	exit (0);
}
