/****************************************************************************
 * Copyright (c) 1999 Sasha Vasko <sashav@sprintmail.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
/* need to get prototype for XrmUniqueQuark for XUniqueContext call */
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

#ifdef I18N
#include <X11/Xlocale.h>
#endif

#if defined (__sun__) && defined (SVR4)
/* Solaris has sysinfo instead of gethostname.  */
#include <sys/systeminfo.h>
#endif

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/loadimg.h"

#include "globals.h"
#include "menus.h"

#define MAXHOSTNAME 255

TextureInfo Textures;		/* texture information */

char *MyName;			/* name are we known by */

ScreenInfo Scr;			/* structures for the screen */
Display *dpy;			/* which display are we talking to */
int screen;			/* and which screen */
extern char *config_file_to_override;
extern Bool shall_override_config_file;

XErrorHandler CatchRedirectError (Display *, XErrorEvent *);
XErrorHandler ASErrorHandler (Display *, XErrorEvent *);
void newhandler (int sig);
void InitModifiers (void);
void CreateCursors (void);
void NoisyExit (int);
void ChildDied (int nonsense);
void SaveDesktopState (void);

XContext ASContext;		/* context for afterstep windows */
XContext MenuContext;		/* context for afterstep menus */

XClassHint NoClass;		/* for applications with no class */

int JunkX = 0, JunkY = 0;
Window JunkRoot, JunkChild;	/* junk window */
unsigned int JunkWidth, JunkHeight, JunkBW, JunkDepth, JunkMask;

/* assorted gray bitmaps for decorative borders */
#define g_width 2
#define g_height 2
static char g_bits[] =
{0x02, 0x01};

#define l_g_width 4
#define l_g_height 2
static char l_g_bits[] =
{0x08, 0x02};
Bool debugging = False, PPosOverride;

#define s_g_width 4
#define s_g_height 4
static char s_g_bits[] =
{0x01, 0x02, 0x04, 0x08};

#ifdef SHAPE
int ShapeEventBase, ShapeErrorBase;
#endif
#ifdef HAVE_XINERAMA
int XineEventBase, XineErrorBase;
#endif

long isIconicState = 0;
extern XEvent Event;
extern Window XmuClientWindow ();
Bool Restarting = False;
int fd_width, x_fd;
char *display_name = NULL;
FILE *savewindow_fd;
int SmartCircCounter = 0;	/* F_WARP_F/B 0=CirculateUp ; 1=CirculateDown */
int LastFunction;
Bool single = False;

static char *display_string;
static char *rdisplay_string;

ASDirs as_dirs =
{NULL, NULL, NULL};

void
InitASDirNames (Bool freemem)
{

  if (freemem)
    {
      if (as_dirs.afters_noncfdir)
	free (as_dirs.afters_noncfdir);
      if (as_dirs.after_dir)
	free (as_dirs.after_dir);
      if (as_dirs.after_sharedir)
	free (as_dirs.after_sharedir);

    }
  else
    {
      as_dirs.afters_noncfdir = PutHome (AFTER_NONCF);
      as_dirs.after_dir = PutHome (AFTER_DIR);
      as_dirs.after_sharedir = PutHome (AFTER_SHAREDIR);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	main - start of afterstep
 *
 ************************************************************************/

int
main (int argc, char **argv)
{
  XSetWindowAttributes attributes;	/* attributes for create windows */
  void InternUsefulAtoms (void);
  void InitVariables (int);
  int i;
  extern int x_fd;
  int len;
  char message[255];
  char num[10];

  MyName = argv[0];

#ifdef I18N
  if (setlocale (LC_CTYPE, AFTER_LOCALE) == NULL)
    afterstep_err ("can't set locale", NULL, NULL, NULL);
#endif
  memset( &Scr, 0x00, sizeof(Scr));
  init_old_look_variables (False);
  InitBase (False);
  InitLook (False);
  InitFeel (False);
  InitDatabase (False);
#ifndef NO_TEXTURE
  frame_init (False);
#endif /* !NO_TEXTURE */
  module_init (0);

#if defined(LOG_FONT_CALLS)
  fprintf (stderr, "logging font calls now\n");
#endif

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "--debug"))
	debugging = True;
      else if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "--version"))
	{
	  printf ("AfterStep version %s\n", VERSION);
	  exit (0);
	}
      else if ((!strcmp (argv[i], "-c")) || (!strcmp (argv[i], "--config")))
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
	}
      else if (!strcmp (argv[i], "-s") || !strcmp (argv[i], "--single"))
	single = True;
      else if ((!strcmp (argv[i], "-d") || !strcmp (argv[i], "--display")) && i + 1 < argc)
	display_name = argv[++i];
      else if (!strcmp (argv[i], "-f") && i + 1 < argc)
	{
	  shall_override_config_file = True;
	  config_file_to_override = argv[++i];
	}
      else
	usage ();
    }

  newhandler (SIGINT);
  newhandler (SIGHUP);
  newhandler (SIGQUIT);
  newhandler (SIGTERM);
  signal (SIGUSR1, Restart);

  signal (SIGPIPE, DeadPipe);

#if 1				/* see SetTimer() */
  {
    void enterAlarm (int);
    signal (SIGALRM, enterAlarm);
  }
#endif /* 1 */

  ReapChildren ();

  if (!(dpy = XOpenDisplay (display_name)))
    {
      afterstep_err ("can't open display %s", XDisplayName (display_name),
		     NULL, NULL);
      exit (1);
    }
  x_fd = XConnectionNumber (dpy);

  if (fcntl (x_fd, F_SETFD, 1) == -1)
    {
      afterstep_err ("close-on-exec failed", NULL, NULL, NULL);
      exit (1);
    }
  	
  Scr.screen = DefaultScreen (dpy);
  screen = Scr.screen ;
  Scr.NumberOfScreens = ScreenCount (dpy);

  if (!single)
    {
      for (i = 0; i < Scr.NumberOfScreens; i++)
	{
	  if (i != Scr.screen)
	    {
	      sprintf (message, "%s -d %s", MyName, XDisplayString (dpy));
	      len = strlen (message);
	      message[len - 1] = 0;
	      sprintf (num, "%d", i);
	      strcat (message, num);
	      strcat (message, " -s ");
	      if (debugging)
		strcat (message, "--debug");
	      if (shall_override_config_file)
		{
		  strcat (message, " -f ");
		  strcat (message, config_file_to_override);
		}
	      strcat (message, " &\n");

	      if (!fork ())
		execl ("/bin/sh", "sh", "-c", message, (char *) 0);
	    }
	}
    }

  /* initializing our dirs names */
  InitASDirNames (False);

  /*  Add a DISPLAY entry to the environment, incase we were started
   * with afterstep -display term:0.0
   */
  len = strlen (XDisplayString (dpy));
  display_string = safemalloc (len + 10);
  sprintf (display_string, "DISPLAY=%s", XDisplayString (dpy));
  putenv (display_string);
  /* Add a HOSTDISPLAY environment variable, which is the same as
   * DISPLAY, unless display = :0.0 or unix:0.0, in which case the full
   * host name will be used for ease in networking . */
  if (strncmp (display_string, "DISPLAY=:", 9) == 0)
    {
      char client[MAXHOSTNAME];
      mygethostname (client, MAXHOSTNAME);
      rdisplay_string = safemalloc (len + 14 + strlen (client));
      sprintf (rdisplay_string, "HOSTDISPLAY=%s:%s", client, &display_string[9]);
    }
  else if (strncmp (display_string, "DISPLAY=unix:", 13) == 0)
    {
      char client[MAXHOSTNAME];
      mygethostname (client, MAXHOSTNAME);
      rdisplay_string = safemalloc (len + 14 + strlen (client));
      sprintf (rdisplay_string, "HOSTDISPLAY=%s:%s", client,
	       &display_string[13]);
    }
  else
    {
      rdisplay_string = safemalloc (len + 14);
      sprintf (rdisplay_string, "HOSTDISPLAY=%s", XDisplayString (dpy));
    }
  putenv (rdisplay_string);

  Scr.Root = RootWindow (dpy, Scr.screen);
  if (Scr.Root == None)
    {
      afterstep_err ("Screen %d is not a valid screen", (char *) Scr.screen,
		     NULL, NULL);
      exit (1);
    }
#ifdef SHAPE
  XShapeQueryExtension (dpy, &ShapeEventBase, &ShapeErrorBase);
#endif /* SHAPE */
#ifdef HAVE_XINERAMA
  if( XPanoramiXQueryExtension(dpy, &XineEventBase, &XineErrorBase))
  {
	  Scr.xinerama_screens = XineramaQueryScreens( dpy, &(Scr.xinerama_screens_num) );
  }
#endif /* XINERAMA */

  InternUsefulAtoms ();

  /* Make sure property priority colors is empty */
  XChangeProperty (dpy, Scr.Root, _XA_MIT_PRIORITY_COLORS,
		   XA_CARDINAL, 32, PropModeReplace, NULL, 0);

  set_gnome_proxy ();

  XSetErrorHandler ((XErrorHandler) CatchRedirectError);
  XSelectInput (dpy, Scr.Root,
		LeaveWindowMask | EnterWindowMask | PropertyChangeMask |
		SubstructureRedirectMask |	/* SubstructureNotifyMask | */
		KeyPressMask | ButtonPressMask | ButtonReleaseMask);
  XSync (dpy, 0);

  XSetErrorHandler ((XErrorHandler) ASErrorHandler);

  CreateCursors ();
  InitVariables (1);
  module_setup ();
#ifndef DONT_GRAB_SERVER
  XGrabServer (dpy);
#endif

  Scr.gray_bitmap =
    XCreateBitmapFromData (dpy, Scr.Root, g_bits, g_width, g_height);

  /* the SizeWindow will be moved into place in LoadASConfig() */
  attributes.override_redirect = True;
  attributes.bit_gravity = NorthWestGravity;
  Scr.SizeWindow = XCreateWindow (dpy, Scr.Root, -999, -999, 10, 10, 0, 0,
				  CopyFromParent, CopyFromParent,
			    CWBitGravity | CWOverrideRedirect, &attributes);

  /* read config file, set up menus, colors, fonts */
  LoadASConfig (display_name, 0, 1, 1, 1);

/*print_unfreed_mem();
 */
  if (Scr.d_depth < 2)
    {
      Scr.gray_pixmap =
	XCreatePixmapFromBitmapData (dpy, Scr.Root, g_bits, g_width, g_height,
		 WhitePixel (dpy, Scr.screen), BlackPixel (dpy, Scr.screen),
				     Scr.d_depth);
      Scr.light_gray_pixmap =
	XCreatePixmapFromBitmapData (dpy, Scr.Root, l_g_bits, l_g_width, l_g_height,
		 WhitePixel (dpy, Scr.screen), BlackPixel (dpy, Scr.screen),
				     Scr.d_depth);
      Scr.sticky_gray_pixmap =
	XCreatePixmapFromBitmapData (dpy, Scr.Root, s_g_bits, s_g_width, s_g_height,
		 WhitePixel (dpy, Scr.screen), BlackPixel (dpy, Scr.screen),
				     Scr.d_depth);
    }
  /* create a window which will accept the keyboard focus when no other 
     windows have it */
  attributes.event_mask = KeyPressMask | FocusChangeMask;
  attributes.override_redirect = True;
  Scr.NoFocusWin = XCreateWindow (dpy, Scr.Root, -10, -10, 10, 10, 0, 0,
				  InputOnly, CopyFromParent,
				  CWEventMask | CWOverrideRedirect,
				  &attributes);
  XMapWindow (dpy, Scr.NoFocusWin);

  XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, CurrentTime);

  XSync (dpy, 0);
  if (debugging)
    XSynchronize (dpy, 1);

#ifndef NO_VIRTUAL
  initPanFrames ();
#endif /* NO_VIRTUAL */
  CaptureAllWindows ();
#ifndef NO_VIRTUAL
  checkPanFrames ();
#endif /* NO_VIRTUAL */
#ifndef DONT_GRAB_SERVER
  XUngrabServer (dpy);
#endif
  fd_width = GetFdWidth ();

  /* watch for incoming module connections */
  module_setup_socket (Scr.Root, display_string);

  if (Restarting)
    {
      if (Scr.RestartFunction != NULL)
	ExecuteFunction (F_FUNCTION, NULL, None, NULL, &Event, C_ROOT, 0, 0,
			 0, 0, Scr.RestartFunction, -1);
    }
  else
    {
      if (Scr.InitFunction != NULL)
	ExecuteFunction (F_FUNCTION, NULL, None, NULL, &Event, C_ROOT, 0, 0,
			 0, 0, Scr.InitFunction, -1);
    }
  XDefineCursor (dpy, Scr.Root, Scr.ASCursors[DEFAULT]);

  /* make sure we're on the right desk, and the _WIN_DESK property is set */
  changeDesks (0, Scr.CurrentDesk);

  HandleEvents ();
  return (0);
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
  int i, j;
  unsigned int nchildren;
  Window root, parent, *children;
  XPointer dummy;

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
	  XWMHints *wmhintsp = XGetWMHints (dpy, children[i]);
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
	      XFree ((char *) wmhintsp);
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
	  Event.xmaprequest.window = children[i];
	  HandleMapRequest ();
	}
    }

  isIconicState = DontCareState;

  if (nchildren > 0)
    XFree ((char *) children);

  /* after the windows already on the screen are in place,
   * don't use PPosition */
  PPosOverride = FALSE;
}

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
  Atom atype;
  int aformat;
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
	  isIconicState = *(long *) prop;
	  XFree (prop);
	}
    }
  return (((isIconicState == IconicState) || (wa.map_state != IsUnmapped)) &&
	  (wa.override_redirect != True));
}


/***********************************************************************
 *
 *  Procedure:
 *	InternUsefulAtoms:
 *            Dont really know what it does
 *
 ***********************************************************************
 */
Atom _XA_MIT_PRIORITY_COLORS;
Atom _XA_WM_CHANGE_STATE;
Atom _XA_WM_STATE;
Atom _XA_WM_COLORMAP_WINDOWS;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_TAKE_FOCUS;
Atom _XA_WM_DELETE_WINDOW;
Atom _XA_MwmAtom;
Atom _XA_WIN_STATE;
Atom _XROOTPMAP_ID;
Atom _AS_STYLE;
Atom _AS_MODULE_SOCKET;
Atom _XA_WIN_DESK;

void
InternUsefulAtoms (void)
{
  /* 
   * Create priority colors if necessary.
   */
  _XA_MIT_PRIORITY_COLORS = XInternAtom (dpy, "_MIT_PRIORITY_COLORS", False);
  _XA_WM_CHANGE_STATE = XInternAtom (dpy, "WM_CHANGE_STATE", False);
  _XA_WM_STATE = XInternAtom (dpy, "WM_STATE", False);
  _XA_WM_COLORMAP_WINDOWS = XInternAtom (dpy, "WM_COLORMAP_WINDOWS", False);
  _XA_WM_PROTOCOLS = XInternAtom (dpy, "WM_PROTOCOLS", False);
  _XA_WM_TAKE_FOCUS = XInternAtom (dpy, "WM_TAKE_FOCUS", False);
  _XA_WM_DELETE_WINDOW = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
  _XA_MwmAtom = XInternAtom (dpy, "_MOTIF_WM_HINTS", False);
  _XA_WIN_STATE = XInternAtom (dpy, "_WIN_STATE", False);
  _XROOTPMAP_ID = XInternAtom (dpy, "_XROOTPMAP_ID", False);
  _AS_STYLE = XInternAtom (dpy, "_AS_STYLE", False);
  _AS_MODULE_SOCKET = XInternAtom (dpy, "_AS_MODULE_SOCKET", False);
  _XA_WIN_DESK = XInternAtom (dpy, "_WIN_DESK", False);
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
  int x, y;
  unsigned int w, h, b;
  XTextProperty tp;
  int i;
  static int first = 1;
  XSizeHints hints;
  long rets;
  Window win, r;
  int client_argc;
  char **client_argv = NULL;

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
  if (strcmp ((char *) tp.value, client) != 0)
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
	}
      else
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
  Window *wins;
  int i;
  unsigned int nwins;
  Window foo;
  char client[MAXHOSTNAME];
  char *realsavewindows_file;

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

/***********************************************************************
 *
 *  Procedure:
 *	CreateCursors - Loads afterstep cursors
 *
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
  ASContext = XUniqueContext ();
  MenuContext = XUniqueContext ();
  NoClass.res_name = NoName;
  NoClass.res_class = NoName;

  Scr.d_depth = DefaultDepth (dpy, Scr.screen);
  Scr.ASRoot.w = Scr.Root;
  Scr.ASRoot.next = 0;
  XGetWindowAttributes (dpy, Scr.Root, &(Scr.ASRoot.attr));
  Scr.root_pushes = 0;
  Scr.pushed_window = &Scr.ASRoot;
  Scr.ASRoot.number_cmap_windows = 0;

  Scr.MyDisplayWidth = DisplayWidth (dpy, Scr.screen);
  Scr.MyDisplayHeight = DisplayHeight (dpy, Scr.screen);

  Scr.NoBoundaryWidth = 0;
  Scr.BoundaryWidth = BOUNDARY_WIDTH;
  Scr.CornerWidth = CORNER_WIDTH;
  Scr.Hilite = NULL;
  Scr.Focus = NULL;
  Scr.Ungrabbed = NULL;

  Scr.VScale = 32;

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
      Atom atype;
      int aformat;
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
	      Scr.CurrentDesk = *(unsigned long *) prop;
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
  Scr.NumBoxes = 0;

  Scr.randomx = Scr.randomy = 0;
  Scr.buttons2grab = 7;

  Scr.next_focus_sequence = 0;

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
  int m, i, knl;
  char *kn;
  KeySym ks;
  KeyCode kc, *kp;
  unsigned lockmask, *mp;
  XModifierKeymap *mm = XGetModifierMapping (dpy);
  lockmask = LockMask;
  if (mm)
    {
      kp = mm->modifiermap;
      for (m = 0; m < 8; m++)
	{
	  for (i = 0; i < mm->max_keypermod; i++)
	    {
	      if ((kc = *kp++) &&
		  ((ks = XKeycodeToKeysym (dpy, kc, 0)) != NoSymbol))
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
    Scr.lock_mods = (unsigned *) safemalloc (256 * sizeof (unsigned));
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
  ASWindow *t;

  /* remove afterstep frame from all windows */
  XGrabServer (dpy);

  InstallWindowColormaps (&Scr.ASRoot);		/* force reinstall */

  for (t = &Scr.ASRoot; t->next != NULL; t = t->next);
  while (t != &Scr.ASRoot)
    {
      ASWindow *tmp = t->prev;
      Destroy (t, False);
      t = tmp;
    }
#ifndef NO_TEXTURE
  if (DecorateFrames)
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
  XErrorEvent event;

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
#ifndef NO_VIRTUAL
  MoveViewport (0, 0, False);
#endif

  /* remove window frames */
  Reborder ();

  /* Close all my pipes */
  ClosePipes ();

  /* freeing up memory */
  InitASDirNames (True);
  free_func_hash ();

#ifdef HAVE_XINERAMA
  if( Scr.xinerama_screens )
  {
	  XFree(Scr.xinerama_screens);
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
	char *my_argv[10];
	int i = 0;

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
    }
  else
    {
      extern Atom _XA_WIN_DESK;
      XDeleteProperty (dpy, Scr.Root, _XA_WIN_DESK);

#ifndef NO_SAVEWINDOWS
      SaveWindowsOpened ();
#endif

#ifdef DEBUG_ALLOCS
      {				/* free up memory */
	extern char *global_base_file;

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
	while (Scr.first_menu != NULL)
	  DeleteMenuRoot (Scr.first_menu);
	/* global drawing GCs */
	if (Scr.NormalGC != NULL)
	  XFreeGC (dpy, Scr.NormalGC);
	if (Scr.StippleGC != NULL)
	  XFreeGC (dpy, Scr.StippleGC);
	if (Scr.DrawGC != NULL)
	  XFreeGC (dpy, Scr.DrawGC);
	if (Scr.LineGC != NULL)
	  XFreeGC (dpy, Scr.LineGC);
	if (Scr.ScratchGC1 != NULL)
	  XFreeGC (dpy, Scr.ScratchGC1);
	if (Scr.ScratchGC2 != NULL)
	  XFreeGC (dpy, Scr.ScratchGC2);
	if (Scr.gray_bitmap != None)
	  XFreePixmap (dpy, Scr.gray_bitmap);
	/* balloons */
	balloon_init (1);
	/* pixmap references */
	pixmap_ref_purge ();
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


/***********************************************************************
 *
 *  Procedure:
 *	ASErrorHandler - displays info on internal errors
 *
 ************************************************************************/
XErrorHandler
ASErrorHandler (Display * dpy, XErrorEvent * event)
{
  extern int last_event_type;

  /* some errors are acceptable, mostly they're caused by 
   * trying to update a lost  window */
  if ((event->error_code == BadWindow) || (event->request_code == X_GetGeometry) ||
      (event->error_code == BadDrawable) || (event->request_code == X_SetInputFocus) ||
      (event->request_code == X_GrabButton) ||
      (event->request_code == X_ChangeWindowAttributes) ||
      (event->request_code == X_InstallColormap))
    return 0;


  afterstep_err ("internal error", NULL, NULL, NULL);
  fprintf (stderr, "      Request %d, Error %d\n", event->request_code,
	   event->error_code);
  fprintf (stderr, "      EventType: %d", last_event_type);
  fprintf (stderr, "\n");
  return 0;
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
  fprintf (stderr, "AfterStep v. %s\n\nusage: afterstep [-v|--version] [-c|--config] [-d dpy] [--debug] [-f old_config_file] [-s]\n", VERSION);
  exit (-1);
}

#ifndef NO_VIRTUAL
/* the root window is surrounded by four window slices, which are InputOnly.
 * So you can see 'through' them, but they eat the input. An EnterEvent in
 * one of these windows causes a Paging. The windows have the according cursor
 * pointing in the pan direction or are hidden if there is no more panning
 * in that direction. This is mostly intended to get a panning even atop
 * of Motif applictions, which does not work yet. It seems Motif windows
 * eat all mouse events.
 *
 * Hermann Dunkel, HEDU, dunkel@cul-ipn.uni-kiel.de 1/94
 */

/***************************************************************************
 * checkPanFrames hides PanFrames if they are on the very border of the
 * VIRTUELL screen and EdgeWrap for that direction is off. 
 * (A special cursor for the EdgeWrap border could be nice) HEDU
 ****************************************************************************/
void
checkPanFrames (void)
{
  extern Bool DoHandlePageing;
  int wrapX = (Scr.flags & EdgeWrapX);
  int wrapY = (Scr.flags & EdgeWrapY);

  /* Remove Pan frames if paging by edge-scroll is permanently or
   * temporarily disabled */
  if ((Scr.EdgeScrollY == 0) || (!DoHandlePageing))
    {
      XUnmapWindow (dpy, Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped = False;
      XUnmapWindow (dpy, Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped = False;
    }
  if ((Scr.EdgeScrollX == 0) || (!DoHandlePageing))
    {
      XUnmapWindow (dpy, Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped = False;
      XUnmapWindow (dpy, Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped = False;
    }
  if (((Scr.EdgeScrollX == 0) && (Scr.EdgeScrollY == 0)) || (!DoHandlePageing))
    return;

  /* LEFT, hide only if EdgeWrap is off */
  if (Scr.Vx == 0 && Scr.PanFrameLeft.isMapped && (!wrapX))
    {
      XUnmapWindow (dpy, Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped = False;
    }
  else if (Scr.Vx > 0 && Scr.PanFrameLeft.isMapped == False)
    {
      XMapRaised (dpy, Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped = True;
    }
  /* RIGHT, hide only if EdgeWrap is off */
  if (Scr.Vx == Scr.VxMax && Scr.PanFrameRight.isMapped && (!wrapX))
    {
      XUnmapWindow (dpy, Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped = False;
    }
  else if (Scr.Vx < Scr.VxMax && Scr.PanFrameRight.isMapped == False)
    {
      XMapRaised (dpy, Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped = True;
    }
  /* TOP, hide only if EdgeWrap is off */
  if (Scr.Vy == 0 && Scr.PanFrameTop.isMapped && (!wrapY))
    {
      XUnmapWindow (dpy, Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped = False;
    }
  else if (Scr.Vy > 0 && Scr.PanFrameTop.isMapped == False)
    {
      XMapRaised (dpy, Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped = True;
    }
  /* BOTTOM, hide only if EdgeWrap is off */
  if (Scr.Vy == Scr.VyMax && Scr.PanFrameBottom.isMapped && (!wrapY))
    {
      XUnmapWindow (dpy, Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped = False;
    }
  else if (Scr.Vy < Scr.VyMax && Scr.PanFrameBottom.isMapped == False)
    {
      XMapRaised (dpy, Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped = True;
    }
}

/****************************************************************************
 *
 * Gotta make sure these things are on top of everything else, or they
 * don't work!
 *
 ***************************************************************************/
void
raisePanFrames (void)
{
  if (Scr.PanFrameTop.isMapped)
    XRaiseWindow (dpy, Scr.PanFrameTop.win);
  if (Scr.PanFrameLeft.isMapped)
    XRaiseWindow (dpy, Scr.PanFrameLeft.win);
  if (Scr.PanFrameRight.isMapped)
    XRaiseWindow (dpy, Scr.PanFrameRight.win);
  if (Scr.PanFrameBottom.isMapped)
    XRaiseWindow (dpy, Scr.PanFrameBottom.win);
}

/****************************************************************************
 *
 * Creates the windows for edge-scrolling 
 *
 ****************************************************************************/
void
initPanFrames ()
{
  XSetWindowAttributes attributes;	/* attributes for create */
  unsigned long valuemask;

  attributes.event_mask = (EnterWindowMask | LeaveWindowMask |
			   VisibilityChangeMask);
  valuemask = (CWEventMask | CWCursor);

  attributes.cursor = Scr.ASCursors[TOP];
  Scr.PanFrameTop.win =
    XCreateWindow (dpy, Scr.Root,
		   0, 0,
		   Scr.MyDisplayWidth, PAN_FRAME_THICKNESS,
		   0,		/* no border */
		   CopyFromParent, InputOnly,
		   CopyFromParent,
		   valuemask, &attributes);
  attributes.cursor = Scr.ASCursors[LEFT];
  Scr.PanFrameLeft.win =
    XCreateWindow (dpy, Scr.Root,
		   0, PAN_FRAME_THICKNESS,
		   PAN_FRAME_THICKNESS,
		   Scr.MyDisplayHeight - 2 * PAN_FRAME_THICKNESS,
		   0,		/* no border */
		   CopyFromParent, InputOnly, CopyFromParent,
		   valuemask, &attributes);
  attributes.cursor = Scr.ASCursors[RIGHT];
  Scr.PanFrameRight.win =
    XCreateWindow (dpy, Scr.Root,
	      Scr.MyDisplayWidth - PAN_FRAME_THICKNESS, PAN_FRAME_THICKNESS,
		   PAN_FRAME_THICKNESS,
		   Scr.MyDisplayHeight - 2 * PAN_FRAME_THICKNESS,
		   0,		/* no border */
		   CopyFromParent, InputOnly, CopyFromParent,
		   valuemask, &attributes);
  attributes.cursor = Scr.ASCursors[BOTTOM];
  Scr.PanFrameBottom.win =
    XCreateWindow (dpy, Scr.Root,
		   0, Scr.MyDisplayHeight - PAN_FRAME_THICKNESS,
		   Scr.MyDisplayWidth, PAN_FRAME_THICKNESS,
		   0,		/* no border */
		   CopyFromParent, InputOnly, CopyFromParent,
		   valuemask, &attributes);
  Scr.PanFrameTop.isMapped = Scr.PanFrameLeft.isMapped =
    Scr.PanFrameRight.isMapped = Scr.PanFrameBottom.isMapped = False;

  Scr.usePanFrames = True;

}

#endif /* NO_VIRTUAL */
