/*
 * Copyright (C) 1998 Sasha Vasko <sasha at aftercode.net>
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

#include "../../configure.h"
#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
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
int dwidth, dheight;
char *MyName;
int fd[2], fd_width;
window_list wins = NULL, wins_tail = NULL;
int wins_count = 0;
FILE *console;

/* switches */
int ofsx = 0, ofsy = 0;
int maxx, maxy;
int untitled = 0, transients = 0;
int maximized = 0;
int all = 0;
int desk = 0;
int reversed = 0, raise_window = 1;
int resize = 1, nostretch = 0;
int sticky = 0, horizontal = 0;
int maxnum = 0;

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
      if (!((x < dwidth) && (y < dheight)
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
  unsigned long header[HEADER_SIZE], *body;
  int count, last = 0;
  fd_set infds;
  FD_ZERO (&infds);
  FD_SET (fd[1], &infds);
  select (fd_width, &infds, 0, 0, NULL);
  if ((count = ReadASPacket (fd[1], header, &body)) > 0)
    {
      switch (header[1])
	{
	case M_CONFIGURE_WINDOW:
	  if (is_suitable_window (body))
	    {
	      window_item *wi =
	      (window_item *) safemalloc (sizeof (window_item));
	      wi->frame = (Window) body[1];
	      wi->th = (int) body[9];
	      wi->bw = (int) body[10];
	      wi->width = body[5];
	      wi->height = body[6];
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
	  fprintf (console,
		   "%s: internal inconsistency: unknown message\n",
		   MyName);
	  break;
	}
      free (body);
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
tile_windows (void)
{
  char msg[128];
  int cur_x = ofsx, cur_y = ofsy;
  int wdiv, hdiv, i, j, count = 1;
  window_item *w = reversed ? wins_tail : wins;

  if (horizontal)
    {
      if ((maxnum > 0) && (maxnum < wins_count))
	{
	  count = wins_count / maxnum;
	  if (wins_count % maxnum)
	    ++count;
	  hdiv = (maxy - ofsy + 1) / maxnum;
	}
      else
	{
	  maxnum = wins_count;
	  hdiv = (maxy - ofsy + 1) / wins_count;
	}
      wdiv = (maxx - ofsx + 1) / count;

      for (i = 0; w && (i < count); ++i)
	{
	  for (j = 0; w && (j < maxnum); ++j)
	    {
	      int nw = wdiv - w->bw * 2;
	      int nh = hdiv - w->bw * 2 - w->th;

	      if (resize)
		{
		  if (nostretch)
		    {
		      if (nw > w->width)
			nw = w->width;
		      if (nh > w->height)
			nh = w->height;
		    }
		  sprintf (msg, "Resize %lup %lup",
			   (nw > 0) ? nw : w->width,
			   (nh > 0) ? nh : w->height);
		  SendInfo (fd, msg, w->frame);
		}
	      sprintf (msg, "Move %up %up", cur_x, cur_y);
	      SendInfo (fd, msg, w->frame);
	      if (raise_window)
		SendInfo (fd, "Raise", w->frame);
	      cur_y += hdiv;
	      wait_configure (w);
	      w = reversed ? w->prev : w->next;
	    }
	  cur_x += wdiv;
	  cur_y = ofsy;
	}
    }
  else
    {
      if ((maxnum > 0) && (maxnum < wins_count))
	{
	  count = wins_count / maxnum;
	  if (wins_count % maxnum)
	    ++count;
	  wdiv = (maxx - ofsx + 1) / maxnum;
	}
      else
	{
	  maxnum = wins_count;
	  wdiv = (maxx - ofsx + 1) / wins_count;
	}
      hdiv = (maxy - ofsy + 1) / count;

      for (i = 0; w && (i < count); ++i)
	{
	  for (j = 0; w && (j < maxnum); ++j)
	    {
	      int nw = wdiv - w->bw * 2;
	      int nh = hdiv - w->bw * 2 - w->th;

	      if (resize)
		{
		  if (nostretch)
		    {
		      if (nw > w->width)
			nw = w->width;
		      if (nh > w->height)
			nh = w->height;
		    }
		  sprintf (msg, "Resize %lup %lup",
			   (nw > 0) ? nw : w->width,
			   (nh > 0) ? nh : w->height);
		  SendInfo (fd, msg, w->frame);
		}
	      sprintf (msg, "Move %up %up", cur_x, cur_y);
	      SendInfo (fd, msg, w->frame);
	      if (raise_window)
		SendInfo (fd, "Raise", w->frame);
	      cur_x += wdiv;
	      wait_configure (w);
	      w = reversed ? w->prev : w->next;
	    }
	  cur_x = ofsx;
	  cur_y += hdiv;
	}
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
	  all = untitled = transients = maximized = 1;
	}
      else if (!strcmp (argv[argi], "-noraise"))
	{
	  raise_window = 0;
	}
      else if (!strcmp (argv[argi], "-noresize"))
	{
	  resize = 0;
	}
      else if (!strcmp (argv[argi], "-nostretch"))
	{
	  nostretch = 1;
	}
      else if (!strcmp (argv[argi], "-desk"))
	{
	  desk = 1;
	}
      else if (!strcmp (argv[argi], "-r"))
	{
	  reversed = 1;
	}
      else if (!strcmp (argv[argi], "-h"))
	{
	  horizontal = 1;
	}
      else if (!strcmp (argv[argi], "-m"))
	{
	  maximized = 1;
	}
      else if (!strcmp (argv[argi], "-mn") && ((argi + 1) < argc))
	{
	  maxnum = atoi (argv[++argi]);
	}
      else if (!strcmp (argv[argi], "-s"))
	{
	  sticky = 1;
	}
      else
	{
	  if (++nsargc > 4)
	    {
	      fprintf (console,
		       "%s: ignoring unknown arg %s\n",
		       MyName, argv[argi]);
	      continue;
	    }
	  if (nsargc == 1)
	    {
	      ofsx = atopixel (argv[argi], dwidth);
	    }
	  else if (nsargc == 2)
	    {
	      ofsy = atopixel (argv[argi], dheight);
	    }
	  else if (nsargc == 3)
	    {
	      maxx = atopixel (argv[argi], dwidth);
	    }
	  else if (nsargc == 4)
	    {
	      maxy = atopixel (argv[argi], dheight);
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
  char *line;
  char *s;
  int len, found;

  line = (char *) safemalloc (MAXLINELENGTH);
  if ((f = fopen (filename, "r")) != NULL)
    {
      len = strlen (MyName);
      found = 0;
      while (((s = fgets (line, MAXLINELENGTH, f)) != NULL) && !found)
	{
	  if ((*line) && (!strncmp (line + 1, MyName, len)))
	    {
	      found = 1;
	      break;
	    }
	}
      fclose (f);
      if (found)
	{
	  char *ret;

	  ret = (char *) safemalloc (strlen (line));
	  strcpy (ret, line);
	  if ((s = strchr (ret, '\n')) != NULL)
	    *s = '\0';
	  return ret;
	}
      else
	return NULL;
    }
  else
    return NULL;
}

void
DeadPipe (int sig)
{
  exit (0);
}

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
	"%s [--version] [--help] [-u] [-t] [-a] [-h] [-r] [-m] [-s] [-noraise]\n"
      "%*s [-desk] [-flatx] [-flaty] [-resize] [-nostretch] [-incx value]\n"
	  "%*s [-incy value] [xoffset yoffset maxwidth maxheight]\n"
	  ,MyName, (int) strlen (MyName), "", (int) strlen (MyName), "");
  exit (0);
}

int
main (int argc, char *argv[])
{
  char msg[MAXLINELENGTH];
  int config_line_count;
  char *config_line;
  char *temp;
  int i;
  char *global_config_file = NULL;

  /* Save our program name - for error messages */
  temp = strrchr (argv[0], '/');
  MyName = temp ? temp + 1 : argv[0];

  for (i = 1; i < argc && *argv[i] == '-'; i++)
    {
      if ( strcmp (argv[i], "--help") == 0 )
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

  console = fopen ("/dev/console", "w");
  if (!console)
    console = stderr;

  dwidth = DisplayWidth (dpy, screen);
  dheight = DisplayHeight (dpy, screen);

  fd_width = GetFdWidth ();

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

  sprintf (msg, "SET_MASK %lu\n", (unsigned long) (
						    M_CONFIGURE_WINDOW |
						    M_END_WINDOWLIST
	   ));

  SendInfo (fd, msg, 0);

/* avoid interactive placement in fvwm1
 *    if (!ofsx) ++ofsx;
 *      if (!ofsy) ++ofsy;
 */

  if (!maxx)
    maxx = dwidth;
  if (!maxy)
    maxy = dheight;

  SendInfo (fd, "Send_WindowList", 0);
  while (get_window ());
  if (wins_count)
    tile_windows ();
  free_window_list (&wins);
  if (console != stderr)
    fclose (console);
  return 0;
}
