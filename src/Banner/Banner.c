/*
 * Copyright (C) 1998 Sasha Vasko <sasha at aftercode.net>
 * Copyright (C) 1996 Robert Depenbrock (robert@eclipse.asta.uni-essen.de)
 * Copyright (C) 1994 Robert Nation
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

#include "../../configure.h"
#ifndef XPM
int
main (int argc, char **argv)
{
  return 1;
}
#else /* XPM */
#ifdef ISC
#include <sys/bsdtypes.h>	/* Saul */
#endif

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif
#include <X11/xpm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* #ifdef BROKEN_SUN_HEADERS #include "../../include/sun_headers.h" #endif */
/* #ifdef NEEDS_ALPHA_HEADER #include "../../include/alpha_header.h" #endif */

#define MODULE_X_INTERFACE

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.xpm"
#include "../../include/afterstep.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"

typedef struct _XpmIcon
  {
    Pixmap pixmap;
    Pixmap mask;
    XpmAttributes attributes;
  }
XpmIcon;

/**************************************************************************
 * A few function prototypes
 **************************************************************************/
void RedrawWindow (void);
void GetXPMData (char **);
void GetXPMFile (char *, char *);
void change_window_name (char *str);
int flush_expose (Window w);
/* static void parseOptions (int fd[2]); */
XpmIcon view;
Window win;

char *pixmapPath = NULL;
char *pixmapName = NULL;
char *myName = NULL;

int timeout = 3000000;		/* default time of 3 seconds */

char *MyName = NULL;
Display *dpy;			/* which display are we talking to */
int screen;
ScreenInfo Scr;
int x_fd;

XSizeHints mysizehints;
Pixel back_pix, fore_pix;
GC NormalGC, FGC;
static Atom wm_del_win;

#define MW_EVENTS   (ExposureMask | ButtonReleaseMask)

/****************************************************************************
 *
 * Gets a module configuration line from afterstep. Returns NULL if there are
 * no more lines to be had. "line" is a pointer to a char *.
 * (comes from fvwm2lib)
 ***************************************************************************/

#define M_END_CONFIG_INFO    (1<<19)
#define M_CONFIG_INFO        (1<<18)

void *
GetConfigLine (int *fd, char **tline)
{
  static int first_pass = 1;
  int count, done = 0;
  static char *line = NULL;
  unsigned long header[HEADER_SIZE];

  if (line != NULL)
    free (line);

  if (first_pass)
    {
      SendInfo (fd, "Send_ConfigInfo", 0);
      first_pass = 0;
    }

  while (!done)
    {
      count = ReadASPacket (fd[1], header, (unsigned long **) &line);
      if (count > 0)
	*tline = &line[3 * sizeof (long)];
      else
	*tline = NULL;
      if (*tline != NULL)
	{
	  while (isspace (**tline))
	    (*tline)++;
	}
/*      fprintf(stderr,"%x %x\n",header[1],M_END_CONFIG_INFO); */
      if (header[1] == M_CONFIG_INFO)
	done = 1;
      else if (header[1] == M_END_CONFIG_INFO)
	{
	  done = 1;
	  if (line != NULL)
	    free (line);
	  line = NULL;
	  *tline = NULL;
	}
    }
  return 0;
}

void
do_exit (void)
{
  XDestroyWindow (dpy, win);
  XSync (dpy, 0);
  exit (0);
}

void
usage (void)
{
  printf ("Usage:\n"
	  "%s [--version] [--help] [image]\n", MyName);
  exit (0);
}

/*
 * error_handler
 * catch X errors, display the message and continue running.
 */
int
error_handler (Display * disp, XErrorEvent * event)
{
  fprintf (stderr, "%s: internal error, error code %d, request code %d, minor code %d.\n",
	 MyName, event->error_code, event->request_code, event->minor_code);
  return 0;
}

/****************************************************************************
 *
 * Creates an icon window as needed
 *
 ****************************************************************************/
int
main (int argc, char **argv)
{
  int retval = 0;
  XEvent Event;
  fd_set in_fdset;
  int fd_width;
  struct timeval value;
  int i;
  char *global_config_file = NULL;

  /* Save our program name - for error messages */
  SetMyName (argv[0]);

  i = ProcessModuleArgs (argc, argv, &(global_config_file), NULL, NULL, usage);
  if (i < argc)
    {
      pixmapName = safemalloc (strlen (argv[i]) + 1);
      strcpy (pixmapName, argv[i++]);
    }

  x_fd = ConnectX (&Scr, display_name, PropertyChangeMask);
  XSetErrorHandler (error_handler);

  fd_width = GetFdWidth ();

  /* no connection with afterstep is necessary */

  /* Get the xpm banner */
  if (pixmapName)
    GetXPMFile (pixmapName, pixmapPath);
  else
    GetXPMData (afterstep);

  /* Create a window to hold the banner */
  mysizehints.flags =
    USSize | USPosition | PWinGravity | PResizeInc | PBaseSize | PMinSize | PMaxSize;
  /* subtract one for the right/bottom border */
  mysizehints.width = view.attributes.width;
  mysizehints.height = view.attributes.height;
  mysizehints.width_inc = 1;
  mysizehints.height_inc = 1;
  mysizehints.base_height = mysizehints.height;
  mysizehints.base_width = mysizehints.width;
  mysizehints.min_height = mysizehints.height;
  mysizehints.min_width = mysizehints.width;
  mysizehints.max_height = mysizehints.height;
  mysizehints.max_width = mysizehints.width;
  mysizehints.win_gravity = NorthWestGravity;

  mysizehints.x = (Scr.MyDisplayWidth - view.attributes.width) / 2;
  mysizehints.y = (Scr.MyDisplayHeight - view.attributes.height) / 2;

  win = XCreateSimpleWindow (dpy, Scr.Root, mysizehints.x, mysizehints.y,
			     mysizehints.width, mysizehints.height,
			     1, fore_pix, None);


  /* Set assorted info for the window */
  XSetTransientForHint (dpy, win, Scr.Root);
  wm_del_win = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
  XSetWMProtocols (dpy, win, &wm_del_win, 1);

  XSetWMNormalHints (dpy, win, &mysizehints);
  change_window_name ("Banner");

  XSetWindowBackgroundPixmap (dpy, win, view.pixmap);
#ifdef SHAPE
  if (view.mask != None)
    XShapeCombineMask (dpy, win, ShapeBounding, 0, 0, view.mask, ShapeSet);
#endif
  XMapWindow (dpy, win);
  XSync (dpy, 0);
  XSelectInput (dpy, win, ButtonReleaseMask);
  /* Display the window */
  value.tv_usec = timeout % 1000000;
  value.tv_sec = timeout / 1000000;
  while (1)
    {
      FD_ZERO (&in_fdset);
      FD_SET (x_fd, &in_fdset);

      if (!XPending (dpy))
#ifdef __hpux
	retval = select (fd_width, (int *) &in_fdset, 0, 0, &value);
#else
	retval = select (fd_width, &in_fdset, 0, 0, &value);
#endif
      if (retval == 0)
	do_exit ();

      if (FD_ISSET (x_fd, &in_fdset))
	{
	  /* read a packet */
	  XNextEvent (dpy, &Event);
	  switch (Event.type)
	    {
	    case ButtonRelease:
	      do_exit ();
	    case ClientMessage:
	      if (Event.xclient.format == 32 && Event.xclient.data.l[0] == wm_del_win)
		do_exit ();
	    default:
	      break;
	    }
	}
    }
  return 0;
}

/****************************************************************************
 *
 * Looks for a color XPM icon file
 *
 ****************************************************************************/
void
GetXPMData (char **data)
{
  view.attributes.valuemask = XpmReturnPixels | XpmCloseness | XpmExtensions;
  view.attributes.closeness = 40000 /* Allow for "similar" colors */ ;
  if (XpmCreatePixmapFromData (dpy, Scr.Root, data,
			       &view.pixmap, &view.mask,
			       &view.attributes) != XpmSuccess)
    {
      fprintf (stderr, "Banner: ERROR couldn't convert data to pixmap\n");
      exit (1);
    }
}

void
GetXPMFile (char *file, char *path)
{
  char *full_file = NULL;

  view.attributes.valuemask = XpmReturnPixels | XpmCloseness | XpmExtensions;
  view.attributes.closeness = 40000 /* Allow for "similar" colors */ ;

  if (file)
    full_file = findIconFile (file, path, R_OK);

  if (full_file)
    {
      if (XpmReadFileToPixmap (dpy,
			       Scr.Root,
			       full_file,
			       &view.pixmap,
			       &view.mask,
			       &view.attributes) == XpmSuccess)
	{
	  return;
	}
      fprintf (stderr, "Banner: ERROR reading pixmap file\n");
    }
  else
    fprintf (stderr, "Banner: ERROR finding pixmap file in PixmapPath\n");
  GetXPMData (afterstep);
}

void
nocolor (char *a, char *b)
{
  fprintf (stderr, "Banner: can't %s %s\n", a, b);
}

#if 0
static void
parseOptions (int fd[2])
{
  char *tline = NULL;
  int clength;

  clength = strlen (MyName);

  while (GetConfigLine (fd, &tline), tline != NULL)
    {
      if (strlen (tline) > 1)
	{
	  if (mystrncasecmp (tline,
			     CatString3 ("*", MyName, "Pixmap"),
			     clength + 7) == 0)
	    {
	      if (pixmapName == (char *) 0)
		{
		  CopyString (&pixmapName, &tline[clength + 7]);
		  if (pixmapName[0] == 0)
		    {
		      free (pixmapName);
		      pixmapName = (char *) 0;
		    }
		}
	      continue;
	    }
	  if (mystrncasecmp (tline,
			     CatString3 ("*", MyName, "Timeout"),
			     clength + 8) == 0)
	    {
	      timeout = atoi (&tline[clength + 8]) * 1000000;
	      continue;
	    }
	  if (mystrncasecmp (tline, "PixmapPath", 10) == 0)
	    {
	      CopyString (&pixmapPath, &tline[10]);
	      if (pixmapPath[0] == 0)
		{
		  free (pixmapPath);
		  pixmapPath = (char *) 0;
		}
	      else
		replaceEnvVar (&pixmapPath);
	      continue;
	    }
	}
    }
  return;
}
#endif

/**************************************************************************
 *  Change the window name displayed in the title bar.
 **************************************************************************/
void
change_window_name (char *str)
{
  XTextProperty name;

  if (XStringListToTextProperty (&str, 1, &name) == 0)
    {
      fprintf (stderr, "Banner: cannot allocate window name");
      return;
    }
  XSetWMName (dpy, win, &name);
  XSetWMIconName (dpy, win, &name);
  XFree (name.value);
}


/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means is dying
 *
 ***********************************************************************/

/*ARGSUSED */
void
DeadPipe (int nonsense)
{
  exit (0);
}
#endif /* XPM */
