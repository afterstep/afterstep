/*
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (c) 1994 Nobutaka Suzuki <nobuta-s@is.aist-nara.ac.jp>
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

#define YES "Yes"
#define NO  "No"

#include "../../configure.h"

#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xmu/WinUtil.h>

#define IN_MODULE
#define MODULE_X_INTERFACE
#include "../../include/afterbase.h"
#include "../../include/aftersteplib.h"
#include "../../include/module.h"
#include "Scroll.h"

int fd[2];

ScreenInfo Scr;

char *BackColor = "black";
char *ForeColor = "grey";

Window app_win;

#define MW_EVENTS   (ExposureMask | ButtonReleaseMask | KeyReleaseMask)

void ParseOptions (const char *);

void
version (void)
{
  printf ("%s version %s\n", MyName, VERSION);
  exit (0);
}

void
usage (void)
{
  printf ("Usage:\n"
	  "%s [-v|--version] [-h|--help] [--window window-id] [x y]\n", MyName);
  exit (0);
}

/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
int
main (int argc, char **argv)
{
  char configfile[255];
  char *realconfigfile;
  char *temp;
  int i;
  char *global_config_file = NULL;

  /* Save our program name - for error messages */
  temp = strrchr (argv[0], '/');
  MyName = temp ? temp + 1 : argv[0];
  set_application_name( argv[0] );

  for (i = 1; i < argc && *argv[i] == '-'; i++)
    {
      if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
	usage ();
      else if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "--version"))
	version ();
      else if (!strcmp (argv[i], "-w") || !strcmp (argv[i], "--window"))
	app_win = strtol (argv[++i], NULL, 16);
      else if (!strcmp (argv[i], "-c") || !strcmp (argv[i], "--context"))
	i++;
      else if (!strcmp (argv[i], "-f") && i + 1 < argc)
	global_config_file = argv[++i];
    }

  /* Dead pipe == dead AfterStep */
  signal (SIGPIPE, DeadPipe);
  set_signal_handler (SIGSEGV);

  x_fd = ConnectX( &Scr, display_name, 0 );
  /* connect to AfterStep */
  fd[0] = fd[1] = ConnectAfterStep (0);

  if (i < argc)
    {
      char *cptr = argv[i++];
      extern int Reduction_H, Reduction_V;
      Reduction_H = strtol (cptr, &cptr, 10);
      Reduction_V = strtol (cptr, &cptr, 10);
      if (Reduction_H < 2)
	Reduction_H = 2;
      if (Reduction_V < 2)
	Reduction_V = 2;
    }

  /* scan config file for set-up parameters */
  /* Colors and fonts */
  if (global_config_file != NULL)
    ParseOptions (global_config_file);
  else
    {
      sprintf (configfile, "%s/scroll", AFTER_DIR);

      realconfigfile = PutHome (configfile);

      if ((CheckFile (realconfigfile)) == -1)
	{
	  free (realconfigfile);
	  sprintf (configfile, "%s/scroll", AFTER_SHAREDIR);
	  realconfigfile = PutHome (configfile);
	}
      ParseOptions (realconfigfile);
      free (realconfigfile);
    }

  if (app_win == 0)
    GetTargetWindow (&app_win);

  if (app_win == 0)
    return 1;

  fd_width = GetFdWidth ();

  GrabWindow (app_win);
  Loop (app_win);

  return 0;
}


void
ParseOptions (const char *filename)
{
  FILE *file;
  char *line, *tline;
  int len;

  if ((file = fopen (filename, "r")) != (FILE *) NULL)
    {
      line = (char *) safemalloc (MAXLINELENGTH);
      len = strlen (MyName);
      while ((tline = fgets (line, MAXLINELENGTH, file)) == NULL)
	{
	  while (isspace (*tline))
	    tline++;
	  if ((*tline == '*') && (!mystrncasecmp (tline + 1, MyName, len)))
	    {
	      tline += len + 1;
	      if (!mystrncasecmp (tline, "Fore", 4))
		CopyString (&ForeColor, tline + 4);
	      else if (!mystrncasecmp (tline, "Back", 4))
		CopyString (&BackColor, tline + 4);
	    }
	}
      free (line);
      fclose (file);
    }
}

/***********************************************************************
 *
 * Detected a broken pipe - time to exit 
 *
 **********************************************************************/
void
DeadPipe (int nonsense)
{
  extern Atom wm_del_win;

  XReparentWindow (dpy, app_win, Scr.Root, 0, 0);
  send_clientmessage (app_win, wm_del_win, CurrentTime);
  XSync (dpy, 0);
  exit (0);
}


/**********************************************************************
 *
 * If no application window was indicated on the command line, prompt
 * the user to select one 
 *
 *********************************************************************/
void
GetTargetWindow (Window * app_win)
{
  XEvent eventp;
  int val = -10, trials;
  Window target_win;

  trials = 0;
  while ((trials < 100) && (val != GrabSuccess))
    {
      val = XGrabPointer (dpy, Scr.Root, True,
			  ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, Root,
			  XCreateFontCursor (dpy, XC_crosshair),
			  CurrentTime);
      if (val != GrabSuccess)
	{
	  sleep_a_little (1000);
	}
      trials++;
    }
  if (val != GrabSuccess)
    {
      fprintf (stderr, "%s: Couldn't grab the cursor!\n", MyName);
      exit (1);
    }
  XMaskEvent (dpy, ButtonReleaseMask, &eventp);
  XUngrabPointer (dpy, CurrentTime);
  XSync (dpy, 0);
  *app_win = eventp.xany.window;
  if (eventp.xbutton.subwindow != None)
    *app_win = eventp.xbutton.subwindow;

  target_win = ClientWindow (*app_win);
  if (target_win != None)
    *app_win = target_win;
}


/****************************************************************************
 *
 * Find the actual application 
 * 
 ***************************************************************************/
Window
ClientWindow (Window input)
{
  Atom _XA_WM_STATE;
  unsigned int nchildren;
  Window root, parent, *children, target;
  unsigned long nitems, bytesafter;
  unsigned char *prop;
  Atom atype;
  int aformat;
  int i;

  _XA_WM_STATE = XInternAtom (dpy, "WM_STATE", False);

  if (XGetWindowProperty (dpy, input, _XA_WM_STATE, 0L,
			  3L, False, _XA_WM_STATE, &atype,
			  &aformat, &nitems, &bytesafter,
			  &prop) == Success)
    {
      if (prop != NULL)
	{
	  XFree (prop);
	  return input;
	}
    }

  if (!XQueryTree (dpy, input, &root, &parent, &children, &nchildren))
    return None;

  for (i = 0; i < nchildren; i++)
    {
      target = ClientWindow (children[i]);
      if (target != None)
	{
	  XFree ((char *) children);
	  return target;
	}
    }
  XFree ((char *) children);
  return None;
}
