/*
 * Copyright (c) 1997 Frank Scheelen <scheelen@worldonline.nl>
 * Copyright (c) 1996 Alfredo Kengi Kojima (kojima@inf.ufrgs.br)
 * Copyright (c) 1996 Kaj Groner <kajg@mindspring.com>
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

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <limits.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MODULE_X_INTERFACE

#include "../../configure.h"
#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "Animate.h"

#define AS_PI 3.14159265358979323846

Display *dpy;
int screen;
char *MyName = NULL;

ScreenInfo Scr;			/* root window */
GC DrawGC;
char *MyName;

/* File type information */
static int fd_width;
static int fd[2];
static int x_fd;

/* masks for AS pipe */
#define mask_reg (	M_CONFIGURE_WINDOW |	\
			M_ICONIFY |		\
			M_DEICONIFY |		\
			M_LOCKONSEND	)
#define mask_off 0

void Loop ();
void ParseOptions (const char *);

struct ASAnimate Animate =
{NULL, ANIM_ITERATIONS, ANIM_DELAYMS,
 ANIM_TWIST, ANIM_WIDTH, AnimateResizeTwist};

/*
 * This makes a twisty iconify/deiconify animation for a window, similar to
 * MacOS.  Parameters specify the position and the size of the initial
 * window and the final window
 */
void
AnimateResizeTwist (int x, int y, int w, int h, int fx, int fy, int fw, int fh)
{
  float cx, cy, cw, ch;
  float xstep, ystep, wstep, hstep;
  XPoint points[5];
  float angle, angle_finite, a, d;

  x += w / 2;
  y += h / 2;
  fx += fw / 2;
  fy += fh / 2;

  xstep = (float) (fx - x) / Animate.iterations;
  ystep = (float) (fy - y) / Animate.iterations;
  wstep = (float) (fw - w) / Animate.iterations;
  hstep = (float) (fh - h) / Animate.iterations;

  cx = (float) x;
  cy = (float) y;
  cw = (float) w;
  ch = (float) h;
  a = atan (ch / cw);
  d = sqrt ((cw / 2) * (cw / 2) + (ch / 2) * (ch / 2));

  angle_finite = 2 * AS_PI * Animate.twist;
  for (angle = 0;; angle += (float) (2 * AS_PI * Animate.twist / Animate.iterations))
    {
      if (angle > angle_finite)
	angle = angle_finite;
      points[0].x = cx + cos (angle - a) * d;
      points[0].y = cy + sin (angle - a) * d;
      points[1].x = cx + cos (angle + a) * d;
      points[1].y = cy + sin (angle + a) * d;
      points[2].x = cx + cos (angle - a + AS_PI) * d;
      points[2].y = cy + sin (angle - a + AS_PI) * d;
      points[3].x = cx + cos (angle + a + AS_PI) * d;
      points[3].y = cy + sin (angle + a + AS_PI) * d;
      points[4].x = cx + cos (angle - a) * d;
      points[4].y = cy + sin (angle - a) * d;
      XGrabServer (dpy);
      XDrawLines (dpy, Scr.Root, DrawGC, points, 5, CoordModeOrigin);
      XFlush (dpy);
      sleep_a_little (Animate.delay * 1000);
      XDrawLines (dpy, Scr.Root, DrawGC, points, 5, CoordModeOrigin);
      XUngrabServer (dpy);
      cx += xstep;
      cy += ystep;
      cw += wstep;
      ch += hstep;
      a = atan (ch / cw);
      d = sqrt ((cw / 2) * (cw / 2) + (ch / 2) * (ch / 2));
      if (angle >= angle_finite)
	break;
    }
  XFlush (dpy);
}

 /*
  * Add even more 3D feel to AfterStep by doing a flipping iconify.
  * Parameters specify the position and the size of the initial and the
  * final window.
  *
  * Idea: how about texture mapped, user definable free 3D movement
  * during a resize? That should get X on its knees all right! :)
  */
void
AnimateResizeFlip (int x, int y, int w, int h, int fx, int fy, int fw, int fh)
{
  float cx, cy, cw, ch;
  float xstep, ystep, wstep, hstep;
  XPoint points[5];

  float distortx;
  float distortch;
  float midy;

  float angle, angle_finite;

  xstep = (float) (fx - x) / Animate.iterations;
  ystep = (float) (fy - y) / Animate.iterations;
  wstep = (float) (fw - w) / Animate.iterations;
  hstep = (float) (fh - h) / Animate.iterations;

  cx = (float) x;
  cy = (float) y;
  cw = (float) w;
  ch = (float) h;

  angle_finite = 2 * AS_PI * Animate.twist;
  for (angle = 0;; angle += (float) (2 * AS_PI * Animate.twist / Animate.iterations))
    {
      if (angle > angle_finite)
	angle = angle_finite;

      distortx = (cw / 10) - ((cw / 5) * sin (angle));
      distortch = (ch / 2) * cos (angle);
      midy = cy + (ch / 2);

      points[0].x = cx + distortx;
      points[0].y = midy - distortch;
      points[1].x = cx + cw - distortx;
      points[1].y = points[0].y;
      points[2].x = cx + cw + distortx;
      points[2].y = midy + distortch;
      points[3].x = cx - distortx;
      points[3].y = points[2].y;
      points[4].x = points[0].x;
      points[4].y = points[0].y;

      XGrabServer (dpy);
      XDrawLines (dpy, Scr.Root, DrawGC, points, 5, CoordModeOrigin);
      XFlush (dpy);
      sleep_a_little (Animate.delay * 1000);
      XDrawLines (dpy, Scr.Root, DrawGC, points, 5, CoordModeOrigin);
      XUngrabServer (dpy);
      cx += xstep;
      cy += ystep;
      cw += wstep;
      ch += hstep;
      if (angle >= angle_finite)
	break;
    }
  XFlush (dpy);
}


 /*
  * And another one, this time around the Y-axis.
  */
void
AnimateResizeTurn (int x, int y, int w, int h, int fx, int fy, int fw, int fh)
{
  float cx, cy, cw, ch;
  float xstep, ystep, wstep, hstep;
  XPoint points[5];

  float distorty;
  float distortcw;
  float midx;

  float angle, angle_finite;

  xstep = (float) (fx - x) / Animate.iterations;
  ystep = (float) (fy - y) / Animate.iterations;
  wstep = (float) (fw - w) / Animate.iterations;
  hstep = (float) (fh - h) / Animate.iterations;

  cx = (float) x;
  cy = (float) y;
  cw = (float) w;
  ch = (float) h;

  angle_finite = 2 * AS_PI * Animate.twist;
  for (angle = 0;; angle += (float) (2 * AS_PI * Animate.twist / Animate.iterations))
    {
      if (angle > angle_finite)
	angle = angle_finite;

      distorty = (ch / 10) - ((ch / 5) * sin (angle));
      distortcw = (cw / 2) * cos (angle);
      midx = cx + (cw / 2);

      points[0].x = midx - distortcw;
      points[0].y = cy + distorty;
      points[1].x = midx + distortcw;
      points[1].y = cy - distorty;
      points[2].x = points[1].x;
      points[2].y = cy + ch + distorty;
      points[3].x = points[0].x;
      points[3].y = cy + ch - distorty;
      points[4].x = points[0].x;
      points[4].y = points[0].y;

      XGrabServer (dpy);
      XDrawLines (dpy, Scr.Root, DrawGC, points, 5, CoordModeOrigin);
      XFlush (dpy);
      sleep_a_little (Animate.delay * 1000);
      XDrawLines (dpy, Scr.Root, DrawGC, points, 5, CoordModeOrigin);
      XUngrabServer (dpy);
      cx += xstep;
      cy += ystep;
      cw += wstep;
      ch += hstep;
      if (angle >= angle_finite)
	break;
    }
  XFlush (dpy);
}


/*
 * This makes a zooming iconify/deiconify animation for a window, like most
 * any other icon animation out there.  Parameters specify the position and
 * the size of the initial window and the final window
 */
void
AnimateResizeZoom (int x, int y, int w, int h, int fx, int fy, int fw, int fh)
{
  float cx, cy, cw, ch;
  float xstep, ystep, wstep, hstep;
  int i;

  xstep = (float) (fx - x) / Animate.iterations;
  ystep = (float) (fy - y) / Animate.iterations;
  wstep = (float) (fw - w) / Animate.iterations;
  hstep = (float) (fh - h) / Animate.iterations;

  cx = (float) x;
  cy = (float) y;
  cw = (float) w;
  ch = (float) h;
  for (i = 0; i < Animate.iterations; i++)
    {
      XGrabServer (dpy);
      XDrawRectangle (dpy, Scr.Root, DrawGC, (int) cx, (int) cy, (int) cw, (int) ch);
      XFlush (dpy);
      sleep_a_little (Animate.delay);
      XDrawRectangle (dpy, Scr.Root, DrawGC, (int) cx, (int) cy, (int) cw, (int) ch);
      XUngrabServer (dpy);
      cx += xstep;
      cy += ystep;
      cw += wstep;
      ch += hstep;
    }
  XFlush (dpy);
}

/*
 * The effect of this is similar to AnimateResizeZoom but this time we
 * add lines to create a 3D effect.  The gotcha is that we have to do
 * something different depending on the direction we are zooming in.
 *
 * Andy Parker <parker_andy@hotmail.com>
 */
void
AnimateResizeZoom3D (int x, int y, int w, int h, int fx, int fy, int fw, int fh)
{
  float cx, cy, cw, ch;
  float xstep, ystep, wstep, hstep, srca, dsta;
  int i;

  xstep = (float) (fx - x) / Animate.iterations;
  ystep = (float) (fy - y) / Animate.iterations;
  wstep = (float) (fw - w) / Animate.iterations;
  hstep = (float) (fh - h) / Animate.iterations;
  dsta = (float) (fw + fh);
  srca = (float) (w + h);

  cx = (float) x;

  cy = (float) y;
  cw = (float) w;
  ch = (float) h;

  if (dsta <= srca)
    /* We are going from a Window to an Icon */
    {
      for (i = 0; i < Animate.iterations; i++)
	{
	  XGrabServer (dpy);
	  XDrawRectangle (dpy, Scr.Root, DrawGC, (int) cx, (int) cy, (int) cw,
			  (int) ch);
	  XDrawRectangle (dpy, Scr.Root, DrawGC, (int) fx, (int) fy, (int) fw,
			  (int) fh);
	  XDrawLine (dpy, Scr.Root, DrawGC, (int) cx, (int) cy, fx, fy);
	  XDrawLine (dpy, Scr.Root, DrawGC, ((int) cx + (int) cw), (int) cy,
		     (fx + fw), fy);
	  XDrawLine (dpy, Scr.Root, DrawGC, ((int) cx + (int) cw),
		     ((int) cy + (int) ch), (fx + fw), (fy + fh));
	  XDrawLine (dpy, Scr.Root, DrawGC, (int) cx, ((int) cy + (int) ch), fx,
		     (fy + fh));
	  XFlush (dpy);
	  sleep_a_little (Animate.delay);
	  XDrawRectangle (dpy, Scr.Root, DrawGC, (int) cx, (int) cy, (int) cw,
			  (int) ch);
	  XDrawRectangle (dpy, Scr.Root, DrawGC, (int) fx, (int) fy, (int) fw,
			  (int) fh);
	  XDrawLine (dpy, Scr.Root, DrawGC, (int) cx, (int) cy, fx, fy);
	  XDrawLine (dpy, Scr.Root, DrawGC, ((int) cx + (int) cw), (int) cy,
		     (fx + fw), fy);
	  XDrawLine (dpy, Scr.Root, DrawGC, ((int) cx + (int) cw),
		     ((int) cy + (int) ch), (fx + fw), (fy + fh));
	  XDrawLine (dpy, Scr.Root, DrawGC, (int) cx, ((int) cy + (int) ch), fx,
		     (fy + fh));
	  XUngrabServer (dpy);
	  cx += xstep;
	  cy += ystep;
	  cw += wstep;
	  ch += hstep;
	}
    }
  if (dsta > srca)
    {
/* We are going from an Icon to a Window */
      for (i = 0; i < Animate.iterations; i++)
	{
	  XGrabServer (dpy);
	  XDrawRectangle (dpy, Scr.Root, DrawGC, (int) cx, (int) cy, (int) cw,
			  (int) ch);
	  XDrawRectangle (dpy, Scr.Root, DrawGC, x, y, w, h);
	  XDrawLine (dpy, Scr.Root, DrawGC, (int) cx, (int) cy, x, y);
	  XDrawLine (dpy, Scr.Root, DrawGC, ((int) cx + (int) cw), (int) cy,
		     (x + w), y);
	  XDrawLine (dpy, Scr.Root, DrawGC, ((int) cx + (int) cw), ((int) cy +
					       (int) ch), (x + w), (y + h));
	  XDrawLine (dpy, Scr.Root, DrawGC, (int) cx, ((int) cy + (int) ch), x,
		     (y + h));
	  XFlush (dpy);
	  sleep_a_little (Animate.delay);
	  XDrawRectangle (dpy, Scr.Root, DrawGC, (int) cx, (int) cy, (int) cw,
			  (int) ch);
	  XDrawRectangle (dpy, Scr.Root, DrawGC, x, y, w, h);
	  XDrawLine (dpy, Scr.Root, DrawGC, (int) cx, (int) cy, x, y);
	  XDrawLine (dpy, Scr.Root, DrawGC, ((int) cx + (int) cw), (int) cy,
		     (x + w), y);
	  XDrawLine (dpy, Scr.Root, DrawGC, ((int) cx + (int) cw),
		     ((int) cy + (int) ch), (x + w), (y + h));
	  XDrawLine (dpy, Scr.Root, DrawGC, (int) cx, ((int) cy + (int) ch), x,
		     (y + h));
	  XUngrabServer (dpy);
	  cx += xstep;
	  cy += ystep;
	  cw += wstep;
	  ch += hstep;
	}
    }
  XFlush (dpy);
}

 /*
  * This picks one of the above animations.
  */
void
AnimateResizeRandom (int x, int y, int w, int h, int fx, int fy, int fw, int fh)
{
  switch ((rand () + (x * y + w * h + fx)) % 4)
    {
    case 0:
      AnimateResizeTwist (x, y, w, h, fx, fy, fw, fh);
      break;
    case 1:
      AnimateResizeFlip (x, y, w, h, fx, fy, fw, fh);
      break;
    case 2:
      AnimateResizeTurn (x, y, w, h, fx, fy, fw, fh);
      break;
    case 3:
      AnimateResizeZoom (x, y, w, h, fx, fy, fw, fh);
      break;
    default:
      AnimateResizeZoom3D (x, y, w, h, fx, fy, fw, fh);
      break;
    }
}

#if 0
/*
 * This makes an animation that looks like that light effect
 * when you turn off an old TV.
 * Used for window destruction
 */
void
AnimateClose (int x, int y, int w, int h)
{
  int i, step;

  if (h > 4)
    {
      step = h * 4 / Animate.iterations;
      if (step == 0)
	{
	  step = 2;
	}
      for (i = h; i >= 2; i -= step)
	{
	  XDrawRectangle (dpy, Scr.Root, DrawGC, x, y, w, i);
	  XFlush (dpy);
	  sleep_a_little (ANIM_DELAY2 * 600);
	  XDrawRectangle (dpy, Scr.Root, DrawGC, x, y, w, i);
	  y += step / 2;
	}
    }
  if (w < 2)
    return;
  step = w * 4 / Animate.iterations;
  if (step == 0)
    {
      step = 2;
    }
  for (i = w; i >= 0; i -= step)
    {
      XDrawRectangle (dpy, Scr.Root, DrawGC, x, y, i, 2);
      XFlush (dpy);
      sleep_a_little (ANIM_DELAY2 * 1000);
      XDrawRectangle (dpy, Scr.Root, DrawGC, x, y, i, 2);
      x += step / 2;
    }
  sleep_a_little (100000);
  XFlush (dpy);
}
#endif

/* error_handler:
 * catch X errors, display the message and continue running.
 */
int
error_handler (Display * disp, XErrorEvent * event)
{
  fprintf (stderr, "%s: internal error, error code %d, request code %d, minor code %d.\n",
	 MyName, event->error_code, event->request_code, event->minor_code);
  return 0;
}

void
DeadPipe (int foo)
{
  exit (0);
}

void
usage (void)
{
  printf ("Usage:\n"
	  "%s [--version] [--help]\n", MyName);
  exit (0);
}

int
main (int argc, char **argv)
{
  int i;
  char *global_config_file = NULL;
  unsigned long color;
  XGCValues gcv;

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

  LoadConfig (global_config_file, "animate", ParseOptions);

  color = WhitePixel (dpy, screen);
  if (Animate.color)
    {
      XColor xcol;
      if (XParseColor (dpy, DefaultColormap (dpy, screen), Animate.color, &xcol))
	{
	  if (XAllocColor (dpy, DefaultColormap (dpy, screen), &xcol))
	    color = xcol.pixel;
	  else
	    color = 0;

	  if (color == 0)
	    {
	      fprintf (stderr, "\n%s: could not allocate color '%s' - using WHITE\n",
		       MyName, Animate.color);
	      color = WhitePixel (dpy, screen);
	    }

	}
      else
	{
	  fprintf (stderr, "%s: could not parse color '%s'\n",
		   MyName, Animate.color);
	}
    }

  gcv.line_width = Animate.width;
  gcv.function = GXxor;
  gcv.foreground = color;
  gcv.background = color;
  gcv.subwindow_mode = IncludeInferiors;
  DrawGC = XCreateGC (dpy, Scr.Root, GCFunction |
		      GCForeground |
		      GCLineWidth |
		      GCBackground |
		      GCSubwindowMode,
		      &gcv);
  SendInfo (fd, "Nop \"\"", 0);
  Loop ();
  return 0;
}

void
GetWindowSize (Window win, int *x, int *y, int *w, int *h)
{
  Window root_r;
  unsigned int bw_r, d_r;

  XGetGeometry (dpy, win, &root_r, x, y, (unsigned int *) w, (unsigned int *) h,
		&bw_r, &d_r);
}

/*
 * Wait for some event like iconify, deiconify and stuff.
 * 
 */
void
Loop ()
{
  int x0, y0, w0, h0, xf, yf, wf, hf;
  Window win;
  ASMessage *asmsg = NULL;

  while (1)
    {
      asmsg = CheckASMessage (fd[1], -1);
      if (asmsg)
	{
	  switch (asmsg->header[1])
	    {
	    case M_DEICONIFY:
	      win = (Window) * (asmsg->body + 1);
	      x0 = (int) *(asmsg->body + 3);
	      y0 = (int) *(asmsg->body + 4);

	      GetWindowSize (win, &xf, &yf, &wf, &hf);
	      Animate.resize (x0, y0, 64, 64, xf, yf, wf, hf);
	      DestroyASMessage (asmsg);
	      break;

	    case M_ICONIFY:
	      xf = (int) *(asmsg->body + 3);
	      yf = (int) *(asmsg->body + 4);
	      if (xf <= -10000)
		break;
	      wf = 64;
	      hf = 64;
	      DestroyASMessage (asmsg);

	      SendInfo (fd, "UNLOCK 1\n", 0);
	      asmsg = CheckASMessage (fd[1], -1);
	      if (!asmsg)
		break;
	      if (asmsg->header[1] != M_CONFIGURE_WINDOW)
		{
		  DestroyASMessage (asmsg);
		  break;
		}
	      x0 = (int) *(asmsg->body + 3);
	      y0 = (int) *(asmsg->body + 4);
	      w0 = (int) *(asmsg->body + 5);
	      h0 = (int) *(asmsg->body + 6);
	      Animate.resize (x0, y0, w0, h0, xf, yf, wf, hf);
	      DestroyASMessage (asmsg);
	      break;
	    default:
	      DestroyASMessage (asmsg);
	    }
	}
      SendInfo (fd, "UNLOCK 1\n", 0);
    }
}

/*****************************************************************************
 * 
 * This routine is responsible for reading and parsing the config file
 * Ripped from the Pager.
 *
 ****************************************************************************/
char *
strtrim (char *str, char **retbuf)
{
  char *beg, *end;

  beg = str;
  while (isspace (*beg))
    beg++;
  end = beg;
  while ((*end) && (!isspace (*end)) && (*end != '\n'))
    end++;
  strncpy (*retbuf, beg, end - beg);
  *(*retbuf + (end - beg)) = '\0';
  return (*retbuf);
}

void
ParseOptions (const char *filename)
{
  FILE *fd;
  char *line, *tline;
  char *parsebuf;
  unsigned int seed;
  long curtime;
  int len;

  if ((fd = fopen (filename, "r")) == NULL)
    {
      fprintf (stderr, "%s: can't open config file %s", MyName, filename);
      exit (1);
    }
  line = (char *) safemalloc (MAXLINELENGTH);
  parsebuf = (char *) safemalloc (MAXLINELENGTH);
  len = strlen (MyName);
  while (fgets (line, MAXLINELENGTH, fd) != NULL)
    {
      if ((*line == '*') && (!mystrncasecmp (line + 1, MyName, len)))
	{
	  tline = line + len + 1;
	  if (!mystrncasecmp (tline, "Color", 5))
	    {
	      strtrim (tline + 5, &parsebuf);
	      if (Animate.color)
		free (Animate.color);
	      Animate.color = strdup (parsebuf);
	    }
	  else if (!mystrncasecmp (tline, "Delay", 5))
	    {
	      Animate.delay = atoi (strtrim (tline + 5, &parsebuf));
	    }
	  else if (!mystrncasecmp (tline, "Width", 5))
	    {
	      Animate.width = atoi (strtrim (tline + 5, &parsebuf));
	    }
	  else if (!mystrncasecmp (tline, "Twist", 5))
	    {
	      Animate.twist = atof (strtrim (tline + 5, &parsebuf));
	    }
	  else if (!mystrncasecmp (tline, "Iterations", 10))
	    {
	      Animate.iterations = atoi (strtrim (tline + 10, &parsebuf));
	    }
	  else if (!mystrncasecmp (tline, "Resize", 6))
	    {
	      strtrim (tline + 6, &parsebuf);
	      if (!mystrncasecmp (parsebuf, "Twist", 5))
		Animate.resize = AnimateResizeTwist;
	      else if (!mystrncasecmp (parsebuf, "Flip", 4))
		Animate.resize = AnimateResizeFlip;
	      else if (!mystrncasecmp (parsebuf, "Turn", 4))
		Animate.resize = AnimateResizeTurn;
	      else if (!mystrncasecmp (parsebuf, "Zoom", 4))
		Animate.resize = AnimateResizeZoom;
	      else if (!mystrncasecmp (parsebuf, "Zoom3D", 6))
		Animate.resize = AnimateResizeZoom3D;
	      else if (!mystrncasecmp (parsebuf, "Random", 6))
		{
		  Animate.resize = AnimateResizeRandom;
		  curtime = time (NULL);
		  seed = (unsigned) curtime % INT_MAX;
		  srand (seed);
		}
	      else
		{
		  Animate.resize = AnimateResizeFlip;
		  fprintf (stderr, "%s: Bad Resize method '%s'\n", MyName, parsebuf);
		}
	    }
	}
    }

  free (parsebuf);
  free (line);
  fclose (fd);
}
