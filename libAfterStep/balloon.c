/*
 * Copyright (C) 1998 Ethan Fischer <allanon@crystaltokyo.com>
 * Based on code by Guylhem Aznar <guylhem@oeil.qc.ca>
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "../configure.h"
#include "../include/aftersteplib.h"

#ifdef I18N
#ifdef __STDC__
#define XTextWidth(x,y,z)	XmbTextEscapement(x ## set,y,z)
#else
#define XTextWidth(x,y,z)	XmbTextEscapement(x/**/set,y,z)
#endif
#define XDrawString(t,u,v,w,x,y,z)	XmbDrawString(t,u,FONTSET,v,w,x,y,z)
#endif

Bool balloon_show = 0;

static void balloon_draw (Balloon * balloon);
static void balloon_map (Balloon * balloon);
static void balloon_unmap (Balloon * balloon);
static void balloon_timer_handler (void *data);

static Balloon *balloon_first = NULL;
static Balloon *balloon_current = NULL;
static int balloon_yoffset = 0;
static int balloon_border_width = 0;
static Pixel balloon_border_color = 0;
static Window balloon_window = None;
static int balloon_delay = 0;
static int balloon_close_delay = 10000;
static MyStyle *balloon_style = NULL;

Bool
balloon_parse (char *tline, FILE * fd)
{
  Bool handled = False;
  while (isspace (*tline))
    tline++;

  /* check - is this one of our keywords? */
  if (mystrncasecmp (tline, "Balloon", 7))
    return False;

  if (mystrncasecmp (tline, "Balloons", 8) == 0)
    {
      handled = True;
      balloon_show = True;
    }
  else if (mystrncasecmp (tline, "BalloonBorderColor", 18) == 0)
    {
      int len;
      handled = True;
      for (tline += 18; isspace (*tline); tline++);
      for (len = 0; tline[len] != '\0' && !isspace (tline[len]); len++);
      tline[len] = '\0';
      balloon_border_color = GetColor (tline);
    }
  else if (mystrncasecmp (tline, "BalloonBorderWidth", 18) == 0)
    {
      handled = True;
      balloon_border_width = strtol (&tline[18], NULL, 10);
      if (balloon_border_width < 0)
	balloon_border_width = 0;
    }
  else if (mystrncasecmp (tline, "BalloonYOffset", 14) == 0)
    {
      handled = True;
      balloon_yoffset = strtol (&tline[14], NULL, 10);
    }
  else if (mystrncasecmp (tline, "BalloonDelay", 12) == 0)
    {
      handled = True;
      balloon_delay = strtol (&tline[12], NULL, 10);
      if (balloon_delay < 0)
	balloon_delay = 0;
    }
  else if (mystrncasecmp (tline, "BalloonCloseDelay", 12) == 0)
    {
      handled = True;
      balloon_close_delay = strtol (&tline[12], NULL, 10);
      if (balloon_close_delay < 100)
	balloon_close_delay = 100;
    }

  return handled;
}

/* call this after parsing config files to finish initialization */
void
balloon_setup (Display * dpy)
{
  XSizeHints hints;
  XSetWindowAttributes attributes;

  attributes.override_redirect = True;
  attributes.border_pixel = balloon_border_color;
  balloon_window = XCreateWindow (dpy, DefaultRootWindow (dpy), 0, 0, 64, 64,
				  balloon_border_width, CopyFromParent,
				  InputOutput, CopyFromParent,
				  CWOverrideRedirect | CWBorderPixel,
				  &attributes);
  XSelectInput (dpy, balloon_window, ExposureMask);
  hints.flags = USPosition;
  XSetWMNormalHints (dpy, balloon_window, &hints);
}

/* free all resources and reinitialize */
void
balloon_init (int free_resources)
{
  if (free_resources)
    {
      while (balloon_first != NULL)
	balloon_delete (balloon_first);
      if (balloon_window != None);
      XDestroyWindow (dpy, balloon_window);
    }

  balloon_first = NULL;
  balloon_current = NULL;
  balloon_yoffset = 0;
  balloon_border_width = 0;
  balloon_border_color = 0;
  balloon_window = None;
  balloon_delay = 0;
  balloon_style = NULL;
  balloon_close_delay = 10000;
}

/*
 * parent must be selecting EnterNotify and LeaveNotify events
 * if balloon_set_rectangle() will be used, parent must be selecting 
 * MotionNotify
 */
Balloon *
balloon_new (Display * dpy, Window parent)
{
  Balloon *balloon;

  balloon = NEW (Balloon);

  (*balloon).next = balloon_first;
  balloon_first = balloon;

  (*balloon).dpy = dpy;
  (*balloon).parent = parent;
  (*balloon).text = NULL;
  /* a width of -1 means that the whole window triggers the balloon */
  (*balloon).px = (*balloon).py = (*balloon).pwidth = (*balloon).pheight = -1;
  /* an x of -1 means that the balloon should be placed according to YOffset */
  (*balloon).x = (*balloon).y = -1;

  return balloon;
}

Balloon *
balloon_new_with_text (Display * dpy, Window parent, char *text)
{
  Balloon *balloon;

  balloon = balloon_new (dpy, parent);
  if (text != NULL)
    (*balloon).text = mystrdup (text);

  return balloon;
}

Balloon *
balloon_find (Window parent)
{
  Balloon *balloon;

  for (balloon = balloon_first; balloon != NULL; balloon = (*balloon).next)
    if ((*balloon).parent == parent)
      break;
  return balloon;
}

/* balloon_delete() checks for NULL so that:

 *   balloon_delete(balloon_find(win));
 *
 * will always work
 */
void
balloon_delete (Balloon * balloon)
{
  if (balloon == NULL)
    return;

  if (balloon_current == balloon)
    balloon_unmap (balloon);

  while (timer_remove_by_data (balloon));

  if (balloon_first == balloon)
    balloon_first = (*balloon).next;
  else if (balloon_first != NULL)
    {
      Balloon *ptr;
      for (ptr = balloon_first; (*ptr).next != NULL; ptr = (*ptr).next)
	if ((*ptr).next == balloon)
	  break;
      if ((*ptr).next == balloon)
	(*ptr).next = (*balloon).next;
    }

  if ((*balloon).text != NULL)
    free ((*balloon).text);

  free (balloon);
}

void
balloon_set_style (Display * dpy, MyStyle * style)
{
  Balloon *tmp = balloon_current;

  if (style == NULL)
    return;

  if (tmp != NULL)
    balloon_unmap (tmp);
  balloon_style = style;
  if (style != NULL)
    XSetWindowBackground (dpy, balloon_window, style->colors.back);
  if (tmp != NULL)
    balloon_map (tmp);
}

void
balloon_set_text (Balloon * balloon, const char *text)
{
  if ((*balloon).text != NULL)
    free ((*balloon).text);
  (*balloon).text = (text != NULL) ? mystrdup (text) : NULL;
  if (balloon_current == balloon)
    balloon_draw (balloon);
}

static void
balloon_draw (Balloon * balloon)
{
  XClearWindow (balloon->dpy, balloon_window);
  if (balloon->text != NULL)
    mystyle_draw_text (balloon_window, balloon_style, balloon->text, 2, 1 + balloon_style->font.y);
}

static void
balloon_map (Balloon * balloon)
{
  if (balloon_current == balloon)
    return;
  if ((*balloon).text != NULL)
    {
      int w = 4;
      int h = 4;
      mystyle_get_text_geometry (balloon_style, balloon->text, strlen (balloon->text), &w, &h);
      w += 4;
      h += 4;
      /* there can be only one! */
      if (balloon_current != NULL)
	balloon_unmap (balloon_current);
      if ((*balloon).x == -1)
	{
	  XWindowChanges wc;
	  Window root;
	  int screen = DefaultScreen ((*balloon).dpy);
	  int x, y, width, height, border, junk;
	  XGetGeometry ((*balloon).dpy, (*balloon).parent, &root, &x, &y, &width, &height, &border, &junk);
	  XTranslateCoordinates ((*balloon).dpy, (*balloon).parent, root, 0, 0, &wc.x, &wc.y, &root);
	  wc.width = w;
	  wc.height = h;
	  if ((*balloon).pwidth > 0)
	    {
	      wc.x += (*balloon).px;
	      wc.y += (*balloon).py;
	      width = (*balloon).pwidth;
	      height = (*balloon).pheight;
	    }
	  wc.x += (width - wc.width) / 2 - balloon_border_width;
	  if (balloon_yoffset >= 0)
	    wc.y += balloon_yoffset + height + 2 * border;
	  else
	    wc.y += balloon_yoffset - wc.height - 2 * balloon_border_width;
	  /* clip to screen */
	  if (wc.x < 2)
	    wc.x = 2;
	  if (wc.x > DisplayWidth ((*balloon).dpy, screen) - wc.width - 2)
	    wc.x = DisplayWidth ((*balloon).dpy, screen) - wc.width - 2;
	  XConfigureWindow ((*balloon).dpy, balloon_window, CWX | CWY | CWWidth | CWHeight, &wc);
	}
      else
	XMoveResizeWindow ((*balloon).dpy, balloon_window, (*balloon).x, (*balloon).y, w, h);
      XMapRaised ((*balloon).dpy, balloon_window);
      balloon_current = balloon;
      balloon->timer_action = BALLOON_TIMER_CLOSE;
      timer_new (balloon_close_delay, &balloon_timer_handler, (void *) balloon);
    }
}

static void
balloon_unmap (Balloon * balloon)
{
  while (timer_remove_by_data (balloon));
  if (balloon_current == balloon)
    {
      XUnmapWindow ((*balloon).dpy, balloon_window);
      balloon_current = NULL;
    }
}

static void
balloon_timer_handler (void *data)
{
  Balloon *balloon = (Balloon *) data;
  switch (balloon->timer_action)
    {
    case BALLOON_TIMER_OPEN:
      balloon_map (balloon);
      break;
    case BALLOON_TIMER_CLOSE:
      balloon_unmap (balloon);
      break;
    }
}

void
balloon_set_active_rectangle (Balloon * balloon, int x, int y, int width, int height)
{
  (*balloon).px = x;
  (*balloon).py = y;
  (*balloon).pwidth = width;
  (*balloon).pheight = height;
}

void
balloon_set_position (Balloon * balloon, int x, int y)
{
  (*balloon).x = x;
  (*balloon).y = y;
  if (balloon == balloon_current)
    XMoveWindow ((*balloon).dpy, balloon_window, x, y);
}

Bool
balloon_handle_event (XEvent * event)
{
  Balloon *balloon;

  if (!balloon_show)
    return False;

  /* is this our event? */
  if ((*event).type != EnterNotify && (*event).type != LeaveNotify && (*event).type != MotionNotify && (*event).type != Expose)
    return False;

  /* handle expose events specially */
  if ((*event).type == Expose)
    {
      if (event->xexpose.window != balloon_window || balloon_current == NULL)
	return False;
      balloon_draw (balloon_current);
      return True;
    }

  for (balloon = balloon_first; balloon != NULL; balloon = balloon->next)
    {
      if ((*balloon).parent != (*event).xany.window)
	continue;

      /* handle the event */
      switch ((*event).type)
	{
	case EnterNotify:
	  if (event->xcrossing.detail != NotifyInferior && (*balloon).pwidth <= 0)
	    {
	      if (balloon_delay == 0)
		balloon_map (balloon);
	      else
		{
		  while (timer_remove_by_data (balloon));
		  balloon->timer_action = BALLOON_TIMER_OPEN;
		  timer_new (balloon_delay, &balloon_timer_handler, (void *) balloon);
		}
	    }
	  break;
	case LeaveNotify:
	  if (event->xcrossing.detail != NotifyInferior)
	    {
	      while (timer_remove_by_data (balloon));
	      balloon_unmap (balloon);
	    }
	  break;
	case MotionNotify:
	  if ((*balloon).pwidth <= 0)
	    break;
	  if ((balloon == balloon_current || timer_find_by_data (balloon)) &&
	      (event->xmotion.x < (*balloon).px || event->xmotion.x >= (*balloon).px + (*balloon).pwidth ||
	       event->xmotion.y < (*balloon).py || event->xmotion.y >= (*balloon).py + (*balloon).pheight))
	    {
	      balloon_unmap (balloon);
	    }
	  else if (balloon != balloon_current && !timer_find_by_data (balloon) &&
		   event->xmotion.x >= (*balloon).px && event->xmotion.x < (*balloon).px + (*balloon).pwidth &&
		   event->xmotion.y >= (*balloon).py && event->xmotion.y < (*balloon).py + (*balloon).pheight)
	    {
	      if (balloon_delay == 0)
		balloon_map (balloon);
	      else
		{
		  while (timer_remove_by_data (balloon));
		  balloon->timer_action = BALLOON_TIMER_OPEN;
		  timer_new (balloon_delay, &balloon_timer_handler, (void *) balloon);
		}
	    }
	  break;
	}
    }

  return True;
}
