/*
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (c) 1996 Andrew Veliath
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#ifdef ISC
#include <sys/bsdtypes.h>
#endif

#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MODULE_X_INTERFACE

#include "../../configure.h"
#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"

typedef struct window_item
  {
    Window frame;
    int th, bw;
    unsigned long width, height;
    struct window_item *prev, *next;
  }
window_item, *window_list;

/* vars */
Display *dpy;
int screen;
ScreenInfo Scr;
char *MyName = NULL;

static int fd_width;
static int fd[2];
static int x_fd;

window_list wins = NULL, wins_tail = NULL;
int wins_count = 0;

/* switches */
int ofsx = 0, ofsy = 0;
int maxw = 0, maxh = 0;
int untitled = 0, transients = 0;
int maximized = 0;
int all = 0;
int desk = 0;
int reversed = 0, raise_window = 1;
int resize = 0, nostretch = 0, sticky = 0;
int flatx = 0, flaty = 0;
int incx = 0, incy = 0;

/* masks for AS pipe */
#define mask_reg (	M_CONFIGURE_WINDOW |	\
			M_END_WINDOWLIST	)

void
insert_window_list (window_list * wl, window_item * i)
{
  if (*wl)
    {
      if ((i->prev = (*wl)->prev))
	i->prev->next = i;
      i->next = *wl;
      (*wl)->prev = i;
    }
  else
    i->next = i->prev = NULL;
  *wl = i;
}

void
free_window_list (window_list * wl)
{
  window_item *q;
  while (*wl)
    {
      q = (*wl)->next;
      free (*wl);
      *wl = q;
    }
}

int
is_suitable_window (unsigned long *body)
{
  XWindowAttributes xwa;
  unsigned long flags = body[8];

  if ((flags & WINDOWLISTSKIP) && !all)
    return 0;

  if ((flags & MAXIMIZED) && !maximized)
    return 0;

  if ((flags & STICKY) && !sticky)
    return 0;

  if (!XGetWindowAttributes (dpy, (Window) body[1], &xwa))
    return 0;

  if (xwa.map_state != IsViewable)
    return 0;

  if (!(flags & MAPPED))
    return 0;

  if (flags & ICONIFIED)
    return 0;

  if (!desk)
    {
      int x = (int) body[3], y = (int) body[4];
      int w = (int) body[5], h = (int) body[6];
      if (!((x < Scr.MyDisplayWidth) && (y < Scr.MyDisplayHeight)
	    && (x + w > 0) && (y + h > 0)))
	return 0;
    }
  if (!(flags & TITLE) && !untitled)
    return 0;

  if ((flags & TRANSIENT) && !transients)
    return 0;

  return 1;
}

int
get_window (void)
{
  ASMessage *asmsg = NULL;
  int last = 0;
  asmsg = CheckASMessage (fd[1], -1);
  if (asmsg)
    {
      switch (asmsg->header[1])
	{
	case M_CONFIGURE_WINDOW:
	  if (is_suitable_window (asmsg->body))
	    {
	      window_item *wi =
	      (window_item *) safemalloc (sizeof (window_item));
	      wi->frame = (Window) asmsg->body[1];
	      wi->th = (int) asmsg->body[9];
	      wi->bw = (int) asmsg->body[10];
	      wi->width = asmsg->body[5];
	      wi->height = asmsg->body[6];
	      if (!wins_tail)
		wins_tail = wi;
	      insert_window_list (&wins, wi);
	      ++wins_count;
	    }
	  last = 1;
	  break;

	case M_END_WINDOWLIST:
	  break;

	default:
	  fprintf (stderr,
		   "%s: internal inconsistency: unexpected message\n",
		   MyName);
	  break;
	}
      DestroyASMessage (asmsg);
    }
  return last;
}

void
wait_configure (window_item * wi)
{
  int found = 0;
  unsigned long header[HEADER_SIZE], *body;
  int count;
  fd_set infds;
  FD_ZERO (&infds);
  FD_SET (fd[1], &infds);
  select (fd_width, &infds, 0, 0, NULL);
  while (!found)
    if ((count = ReadASPacket (fd[1], header, &body)) > 0)
      {
	if ((header[1] == M_CONFIGURE_WINDOW)
	    && (Window) body[1] == wi->frame)
	  found = 1;
	free (body);
      }
}

int
atopixel (char *s, unsigned long f)
{
  int l = strlen (s);
  if (l < 1)
    return 0;
  if (isalpha (s[l - 1]))
    {
      char s2[24];
      strcpy (s2, s);
      s2[strlen (s2) - 1] = 0;
      return atoi (s2);
    }
  return (atoi (s) * f) / 100;
}

void
cascade_windows (void)
{
  char msg[128];
  int cur_x = ofsx, cur_y = ofsy;
  window_item *w = reversed ? wins_tail : wins;
  while (w)
    {
      unsigned long nw = 0, nh = 0;
      if (raise_window)
	SendInfo (fd, "Raise", w->frame);
      sprintf (msg, "Move %up %up", cur_x, cur_y);
      SendInfo (fd, msg, w->frame);
      if (resize)
	{
	  if (nostretch)
	    {
	      if (maxw
		  && (w->width > maxw))
		nw = maxw;
	      if (maxh
		  && (w->height > maxh))
		nh = maxh;
	    }
	  else
	    {
	      nw = maxw;
	      nh = maxh;
	    }
	  if (nw || nh)
	    {
	      sprintf (msg, "Resize %lup %lup",
		       nw ? nw : w->width,
		       nh ? nh : w->height);
	      SendInfo (fd, msg, w->frame);
	    }
	}
      wait_configure (w);
      if (!flatx)
	cur_x += w->bw;
      cur_x += incx;
      if (!flaty)
	cur_y += w->bw + w->th;
      cur_y += incy;
      w = reversed ? w->prev : w->next;
    }
}

void
parse_args (char *s, int argc, char *argv[], int argi)
{
  int nsargc = 0;
  /* parse args */
  for (; argi < argc; ++argi)
    {
      if (!strcmp (argv[argi], "-u"))
	{
	  untitled = 1;
	}
      else if (!strcmp (argv[argi], "-t"))
	{
	  transients = 1;
	}
      else if (!strcmp (argv[argi], "-a"))
	{
	  all = untitled = sticky = transients = maximized = 1;
	}
      else if (!strcmp (argv[argi], "-r"))
	{
	  reversed = 1;
	}
      else if (!strcmp (argv[argi], "-noraise"))
	{
	  raise_window = 0;
	}
      else if (!strcmp (argv[argi], "-desk"))
	{
	  desk = 1;
	}
      else if (!strcmp (argv[argi], "-flatx"))
	{
	  flatx = 1;
	}
      else if (!strcmp (argv[argi], "-flaty"))
	{
	  flaty = 1;
	}
      else if (!strcmp (argv[argi], "-m"))
	{
	  maximized = 1;
	}
      else if (!strcmp (argv[argi], "-s"))
	{
	  sticky = 1;
	}
      else if (!strcmp (argv[argi], "-resize"))
	{
	  resize = 1;
	}
      else if (!strcmp (argv[argi], "-nostretch"))
	{
	  nostretch = 1;
	}
      else if (!strcmp (argv[argi], "-incx") && ((argi + 1) < argc))
	{
	  incx = atopixel (argv[++argi], Scr.MyDisplayWidth);
	}
      else if (!strcmp (argv[argi], "-incy") && ((argi + 1) < argc))
	{
	  incy = atopixel (argv[++argi], Scr.MyDisplayHeight);
	}
      else
	{
	  if (++nsargc > 4)
	    {
	      fprintf (stderr,
		       "%s: %s: ignoring unknown arg %s\n",
		       MyName, s, argv[argi]);
	      continue;
	    }
	  if (nsargc == 1)
	    {
	      ofsx = atopixel (argv[argi], Scr.MyDisplayWidth);
	    }
	  else if (nsargc == 2)
	    {
	      ofsy = atopixel (argv[argi], Scr.MyDisplayHeight);
	    }
	  else if (nsargc == 3)
	    {
	      maxw = atopixel (argv[argi], Scr.MyDisplayWidth);
	    }
	  else if (nsargc == 4)
	    {
	      maxh = atopixel (argv[argi], Scr.MyDisplayHeight);
	    }
	}
    }
}

int
parse_line (char *s, char ***args)
{
  int count = 0, i = 0;
  char *arg_save[48];
  strtok (s, " ");
  while ((s = strtok (NULL, " ")))
    arg_save[count++] = s;
  *args = (char **) safemalloc (sizeof (char *) * count);
  for (; i < count; ++i)
    (*args)[i] = arg_save[i];
  return count;
}

char *
GetConfigLine (const char *filename)
{
  FILE *f;
  char *line, *tline;
  int found = 0, len;

  if ((f = fopen (filename, "r")) != NULL)
    {
      line = (char *) safemalloc (MAXLINELENGTH);
      len = strlen (MyName);
      while (((tline = fgets (line, MAXLINELENGTH, f)) != NULL) && (!found))
	{
	  if ((*line == '*') && (!strncmp (line + 1, MyName, len)))
	    {
	      found = 1;
	      break;
	    }
	}
      fclose (f);
      if (found)
	{
	  len = strlen (line);
	  tline = (char *) safemalloc (len);
	  strcpy (tline, line);
	  free (line);
	  if (tline[len - 1] == '\n')
	    tline[len - 1] = 0;
	  return (tline);
	}
      free (line);
    }
  return (NULL);
}

void
DeadPipe (int sig)
{
  exit (0);
}

void
usage (void)
{
  printf ("Usage:\n"
	"%s [--version] [--help] [-u] [-t] [-a] [-r] [-m] [-s] [-noraise]\n"
      "%*s [-desk] [-flatx] [-flaty] [-resize] [-nostretch] [-incx value]\n"
	  "%*s [-incy value] [xoffset yoffset maxwidth maxheight]\n"
	  ,MyName, (int) strlen (MyName), "", (int) strlen (MyName), "");
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

int
main (int argc, char *argv[])
{
  int config_line_count;
  char *config_line;
  int i;
  char *global_config_file = NULL;

  /* Save our program name - for error messages */
  SetMyName (argv[0]);

  i = ProcessModuleArgs (argc, argv, &(global_config_file), NULL, NULL, usage);

  /* Dead pipe == AS died */
  signal (SIGPIPE, DeadPipe);
  signal (SIGQUIT, DeadPipe);
  signal (SIGSEGV, DeadPipe);
  signal (SIGTERM, DeadPipe);

  x_fd = ConnectX (&Scr, display_name, PropertyChangeMask);
  XSetErrorHandler (error_handler);

  /* connect to AfterStep */
  fd_width = GetFdWidth ();
  fd[0] = fd[1] = ConnectAfterStep (mask_reg);

  if (global_config_file == NULL)
    {
      char configfile[255];
      sprintf (configfile, "%s/base.%dbpp", AFTER_DIR, DefaultDepth (dpy, screen));
      global_config_file = (char *) PutHome (configfile);
      if (CheckFile (global_config_file) == -1)
	{
	  free (global_config_file);
	  sprintf (configfile, "%s/base.%dbpp", AFTER_SHAREDIR, DefaultDepth (dpy, screen));
	  global_config_file = PutHome (configfile);
	}
    }
  if ((config_line = GetConfigLine (global_config_file)) != NULL)
    {
      char **args = NULL;
      config_line_count = parse_line (config_line, &args);
      parse_args ("config args",
		  config_line_count, args, 0);
      free (config_line);
      free (args);
    }
  parse_args ("module args", argc, argv, i);

/* avoid interactive placement in fvwm1
 *    if (!ofsx) ++ofsx;
 *      if (!ofsy) ++ofsy;
 */

  SendInfo (fd, "Send_WindowList", 0);
  while (get_window ());
  if (wins_count)
    cascade_windows ();
  free_window_list (&wins);

  return 0;
}
