/*
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (c) 1994 Robert Nation
 * Copyright (c) 1994 Per Persson <pp@solace.mh.se>
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

#include "../../configure.h"

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "../../include/aftersteplib.h"
#include "../../include/module.h"

#include "Save.h"

char *MyName;
int fd_width;
int fd[2];

struct list *list_root = NULL;

Display *dpy;			/* which display are we talking to */
int ScreenWidth, ScreenHeight;
int screen;

long Vx, Vy;

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
  char *temp;
  int i;
  char *global_config_file = NULL;

  /* Save our program name - for error messages */
  temp = strrchr (argv[0], '/');
  MyName = temp ? temp + 1 : argv[0];

  for (i = 1; i < argc && *argv[i] == '-'; i++)
    {
      if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
	usage ();
      else if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "--version"))
	version ();
      else if (!strcmp (argv[i], "-w") || !strcmp (argv[i], "--window"))
	i++;
      else if (!strcmp (argv[i], "-c") || !strcmp (argv[i], "--context"))
	i++;
      else if (!strcmp (argv[i], "-f") && i + 1 < argc)
	global_config_file = argv[++i];
    }

  /* Dead pipe == dead AfterStep */
  signal (SIGPIPE, DeadPipe);

  if ((dpy = XOpenDisplay ("")) == NULL)
    {
      fprintf (stderr, "%s: couldn't open display %s\n",
	       MyName, XDisplayName (""));
      exit (1);
    }
  screen = DefaultScreen (dpy);

  /* connect to AfterStep */
  temp = module_get_socket_property (RootWindow (dpy, screen));
  fd[0] = fd[1] = module_connect (temp);
  XFree (temp);
  if (fd[0] < 0)
    {
      fprintf (stderr, "%s: unable to establish connection to AfterStep\n", MyName);
      exit (1);
    }
  temp = safemalloc (9 + strlen (MyName) + 1);
  sprintf (temp, "SET_NAME %s", MyName);
  SendInfo (fd, temp, None);
  free (temp);

  ScreenHeight = DisplayHeight (dpy, screen);
  ScreenWidth = DisplayWidth (dpy, screen);

  fd_width = GetFdWidth ();

  /* Create a list of all windows */
  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendInfo (fd, "Send_WindowList", 0);

  Loop (fd);

  return 0;
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
  int count = 0;

  while (1)
    {
      /* read a packet */
      if ((count = ReadASPacket (fd[1], header, &body)) > 0)
	{
	  /* dispense with the new packet */
	  process_message (header[1], body);
	  free (body);
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
  switch (type)
    {
    case M_CONFIGURE_WINDOW:
      if (!find_window (body[0]))
	add_window (body[0], body);
      break;
    case M_NEW_PAGE:
      list_new_page (body);
      break;
    case M_END_WINDOWLIST:
      do_save ();
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
 *	add_window - add a new window in the current window list 
 *
 ***********************************************************************/
void
add_window (unsigned long new_win, unsigned long *body)
{
  struct list *t;

  if (new_win == 0)
    return;

  t = (struct list *) safemalloc (sizeof (struct list));
  t->id = new_win;
  t->next = list_root;
  t->frame_height = (int) body[6];
  t->frame_width = (int) body[5];
  t->base_width = (int) body[11];
  t->base_height = (int) body[12];
  t->width_inc = (int) body[13];
  t->height_inc = (int) body[14];
  t->frame_x = (int) body[3];
  t->frame_y = (int) body[4];;
  t->title_height = (int) body[9];;
  t->boundary_width = (int) body[10];
  t->flags = (unsigned long) body[8];
  t->gravity = body[21];
  list_root = t;
}



/***********************************************************************
 *
 *  Procedure:
 *	list_new_page - capture new-page info
 *
 ***********************************************************************/
void
list_new_page (unsigned long *body)
{
  Vx = (long) body[0];
  Vy = (long) body[1];
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
 *	writes a command line argument to file "out"
 *      checks for qoutes and stuff
 *
 ***********************************************************************/
void
write_string (FILE * out, char *line)
{
  int len, space = 0, qoute = 0, i;

  len = strlen (line);

  for (i = 0; i < len; i++)
    {
      if (isspace (line[i]))
	space = 1;
      if (line[i] == '\"')
	qoute = 1;
    }
  if (space == 1)
    fprintf (out, "\"");
  if (qoute == 0)
    fprintf (out, "%s", line);
  else
    {
      for (i = 0; i < len; i++)
	{
	  if (line[i] == '\"')
	    fprintf (out, "\\\"");
	  else
	    fprintf (out, "%c", line[i]);
	}
    }
  if (space == 1)
    fprintf (out, "\"");
  fprintf (out, " ");

}



/***********************************************************************
 *
 *  Procedure:
 *	checks to see if we are supposed to take some action now,
 *      finds time for next action to be performed.
 *
 ***********************************************************************/
void
do_save (void)
{
  struct list *t;
  char tname[200], loc[30];
  FILE *out;
  char **command_list;
  int dwidth, dheight, xtermline = 0;
  int x1, x2, y1, y2, i, command_count;
  long tVx, tVy;

  sprintf (tname, "%s/.xinitrc", getenv ("HOME"));
  out = fopen (tname, "w+");
  for (t = list_root; t != NULL; t = t->next)
    {
      tname[0] = 0;

      x1 = t->frame_x;
      x2 = ScreenWidth - x1 - t->frame_width - 2;
      if (x2 < 0)
	x2 = 0;
      y1 = t->frame_y;
      y2 = ScreenHeight - y1 - t->frame_height - 2;
      if (y2 < 0)
	y2 = 0;
      dheight = t->frame_height - t->title_height - 2 * t->boundary_width;
      dwidth = t->frame_width - 2 * t->boundary_width;
      dwidth -= t->base_width;
      dheight -= t->base_height;
      dwidth /= t->width_inc;
      dheight /= t->height_inc;

      if (t->flags & STICKY)
	{
	  tVx = 0;
	  tVy = 0;
	}
      else
	{
	  tVx = Vx;
	  tVy = Vy;
	}
      sprintf (tname, "%dx%d", dwidth, dheight);
      if ((t->gravity == EastGravity) ||
	  (t->gravity == NorthEastGravity) ||
	  (t->gravity == SouthEastGravity))
	sprintf (loc, "-%d", x2);
      else
	sprintf (loc, "+%d", x1 + (int) tVx);
      strcat (tname, loc);

      if ((t->gravity == SouthGravity) ||
	  (t->gravity == SouthEastGravity) ||
	  (t->gravity == SouthWestGravity))
	sprintf (loc, "-%d", y2);
      else
	sprintf (loc, "+%d", y1 + (int) tVy);
      strcat (tname, loc);

      if (XGetCommand (dpy, t->id, &command_list, &command_count))
	{
	  for (i = 0; i < command_count; i++)
	    {
	      if (strncmp ("-geo", command_list[i], 4) == 0)
		{
		  i++;
		  continue;
		}
	      if (strncmp ("-ic", command_list[i], 3) == 0)
		continue;
	      if (strncmp ("-display", command_list[i], 8) == 0)
		{
		  i++;
		  continue;
		}
	      write_string (out, command_list[i]);
	      if (strstr (command_list[i], "xterm"))
		{
		  fprintf (out, "-geometry %s ", tname);
		  if (t->flags & ICONIFIED)
		    fprintf (out, "-ic ");
		  xtermline = 1;
		}
	    }
	  if (command_count > 0)
	    {
	      if (xtermline == 0)
		{
		  if (t->flags & ICONIFIED)
		    fprintf (out, "-ic ");
		  fprintf (out, "-geometry %s &\n", tname);
		}
	      else
		{
		  fprintf (out, "&\n");
		}
	    }
	  XFreeStringList (command_list);
	  xtermline = 0;
	}
    }
  fprintf (out, "\nexec afterstep\n");
  fclose (out);
  exit (0);
}
