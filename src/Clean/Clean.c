/*
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
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

#define TRUE 1
#define FALSE
#define IN_MODULE
#define MODULE_X_INTERFACE

#include "../../configure.h"
#ifdef ISC
#include <sys/bsdtypes.h>	/* Saul */
#endif

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"

#include "Clean.h"

char *MyName;
Display *dpy;			/* which display are we talking to */
int screen;
ScreenInfo Scr;

int fd_width;
int fd[2];
struct list *list_root = NULL;
unsigned long current_focus = 0;

long period[3] =
{-1, -1, -1};
long maxperiod = -1;
char command[3][256];
int num_commands = 0;

unsigned long next_event_time = 0;
unsigned long t0;

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
          "%s [-f [config file]] [-v|--version] [-h|--help]\n", MyName);
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
  int i;
  char *global_config_file = NULL;

  /* Save our program name - for error messages */
  SetMyName (argv[0]);
  i = ProcessModuleArgs (argc, argv, &(global_config_file), NULL, NULL, usage);
  if (i == argc)
    usage ();
  
  /* Dead pipes mean AfterStep died */
  signal (SIGPIPE, DeadPipe);

  ConnectX (&Scr, display_name, 0);
  screen = Scr.screen ;

  /* connect to AfterStep */
  fd[0] = fd[1] = ConnectAfterStep (M_ADD_WINDOW |
						M_CONFIGURE_WINDOW |
						M_DESTROY_WINDOW |
						M_FOCUS_CHANGE |
						M_END_WINDOWLIST);

  /* scan config file for set-up parameters */
  LoadConfig (global_config_file, "clean", ParseOptions);

  if (num_commands == 0)
    {
      fprintf (stderr, "%s: Nothing to do!\n", MyName);
      exit (0);
    }

  fd_width = GetFdWidth ();

  /* Create a list of all windows */
  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendInfo (fd, "Send_WindowList", 0);

  Loop (fd);

  return 0;
}


void
ParseOptions (const char *filename)
{
  FILE *file;
  char *line, *tline;
  int len;

  file = fopen (filename, "r");
  if (file != NULL)
    {
      line = (char *) safemalloc (MAXLINELENGTH);
      len = strlen (MyName);
      while (((tline = fgets (line, MAXLINELENGTH, file)) != NULL) && (num_commands < 3))
	{
	  while (isspace (*tline))
	    tline++;
	  if ((*tline == '*') && (!mystrncasecmp (tline + 1, MyName, len)))
	    {
	      tline += len + 1;
	      sscanf (tline, "%ld", &period[num_commands]);
	      if (period[num_commands] > maxperiod)
		maxperiod = period[num_commands];
	      while (isspace (*tline))
		tline++;
	      while ((!isspace (*tline)) && (*tline))
		tline++;
	      while (isspace (*tline))
		tline++;	/* points to "command" field */
	      strcpy (command[num_commands], tline);
	      num_commands++;
	    }
	}
      fclose (file);
      free (line);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	Loop - wait for data to process
 *
 ***********************************************************************/
void
Loop (int *fd)
{
  unsigned long header[3], *body;
  fd_set in_fdset;
  struct itimerval value;
  int retval;

  while (1)
    {
      FD_ZERO (&in_fdset);
      FD_SET (fd[1], &in_fdset);

      /* Find out when we have to iconify or whatever a window */
      find_next_event_time ();

      /* set up a time-out, in case no packets arrive before we have to
       * iconify something */
      value.it_value.tv_usec = 0;
      value.it_value.tv_sec = next_event_time - t0;

#ifdef __hpux
      if (value.it_value.tv_sec > 0)
	retval = select (fd_width, (int *) &in_fdset, 0, 0, &value.it_value);
      else
	retval = select (fd_width, (int *) &in_fdset, 0, 0, NULL);
#else
      if (value.it_value.tv_sec > 0)
	retval = select (fd_width, &in_fdset, 0, 0, &value.it_value);
      else
	retval = select (fd_width, &in_fdset, 0, 0, NULL);
#endif
      if (FD_ISSET (fd[1], &in_fdset))
	{
	  /* read a packet */
	  if (ReadASPacket (fd[1], header, &body) > 0)
	    {
	      /* dispense with the new packet */
	      process_message (header[1], body);
	      free (body);
	    }
	}
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	Process message - examines packet types, and takes appropriate action
 *
 ***********************************************************************/
void
process_message (unsigned long type, unsigned long *body)
{
  struct list *l;

  switch (type)
    {
    case M_ADD_WINDOW:
      if (!find_window (body[0]))
	add_window (body[0]);
      break;
    case M_CONFIGURE_WINDOW:
      if (!find_window (body[0]))
	add_window (body[0]);
      break;
    case M_DESTROY_WINDOW:
      remove_window (body[0]);
      break;
    case M_FOCUS_CHANGE:
      l = find_window (body[0]);
      if (l == 0)
	{
	  add_window (body[0]);
	  l = find_window (body[0]);
	  update_focus (l, body[0]);
	}
      else
	update_focus (l, body[0]);
      break;
    default:
      break;
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	find_window - find a window in the current window list 
 *
 ***********************************************************************/
struct list *
find_window (unsigned long id)
{
  struct list *l;

  if (list_root == NULL)
    return NULL;

  for (l = list_root; l != NULL; l = l->next)
    {
      if (l->id == id)
	return l;
    }
  return NULL;
}


/***********************************************************************
 *
 *  Procedure:
 *	remove_window - remove a window in the current window list 
 *
 ***********************************************************************/
void
remove_window (unsigned long id)
{
  struct list *l, *prev;

  if (list_root == NULL)
    return;

  prev = NULL;
  for (l = list_root; l != NULL; l = l->next)
    {
      if (l->id == id)
	{
	  if (prev != NULL)
	    prev->next = l->next;
	  else
	    list_root = l->next;
	  free (l);
	  return;
	}
      prev = l;
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	add_window - add a new window in the current window list 
 *
 ***********************************************************************/
void
add_window (unsigned long new_win)
{
  struct list *t;

  if (new_win == 0)
    return;

  t = (struct list *) safemalloc (sizeof (struct list));
  t->id = new_win;
  t->last_focus_time = time (0);
  t->actions = 0;
  t->next = list_root;
  list_root = t;
}


/***********************************************************************
 *
 *  Procedure:
 *	mark the window which currently has focus, so we don't iconify it
 *
 ***********************************************************************/
void
update_focus (struct list *l, unsigned long win)
{
  struct list *t;

  t = find_window (current_focus);
  if (t != NULL)
    {
      t->last_focus_time = time (0);
      t->actions = 0;
    }

  if (l != NULL)
    {
      l->last_focus_time = time (0);
      l->actions = 0;
    }
  current_focus = win;
}

/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means AfterStep is dying
 *
 ***********************************************************************/
void
DeadPipe (int nonsense)
{
  exit (0);
}

/***********************************************************************
 *
 *  Procedure:
 *	checks to see if we are supposed to take some action now,
 *      finds time for next action to be performed.
 *
 ***********************************************************************/
void
find_next_event_time (void)
{
  struct list *t;

  t0 = time (0);
  next_event_time = t0;

  if (list_root == NULL)
    return;

  next_event_time = t0 + maxperiod;

  for (t = list_root; t != NULL; t = t->next)
    {
      if (t->id != current_focus)
	{
	  if ((period[2] > 0) && (t->last_focus_time + period[2] <= t0))
	    {
	      if (!(t->actions & ACTION1))
		{
		  SendInfo (fd, command[2], t->id);
		  t->actions |= ACTION1;
		}
	    }
	  else if ((period[1] > 0) && (t->last_focus_time + period[1] <= t0))
	    {
	      if (!(t->actions & ACTION2))
		{
		  SendInfo (fd, command[1], t->id);
		  t->actions |= ACTION2;
		}
	    }
	  else if ((period[0] > 0) && (t->last_focus_time + period[0] <= t0))
	    {
	      if (!(t->actions & ACTION3))
		{
		  SendInfo (fd, command[0], t->id);
		  t->actions |= ACTION3;
		}
	    }

	}
      else
	t->last_focus_time = t0;

      if ((period[0] > 0) && (t->last_focus_time + period[0] > t0) &&
	  (t->last_focus_time + period[0] < next_event_time))
	next_event_time = t->last_focus_time + period[0];
      else if ((period[1] > 0) && (t->last_focus_time + period[1] > t0) &
	       (t->last_focus_time + period[1] < next_event_time))
	next_event_time = t->last_focus_time + period[1];
      else if ((period[2] > 0) && (t->last_focus_time + period[2] > t0) &&
	       (t->last_focus_time + period[2] < next_event_time))
	next_event_time = t->last_focus_time + period[2];
    }
}
