/* Copyright (C) 1999 Rafal Wierzbicki <rafal@mcss.mcmaster.ca>

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
 *
 * TODO:
 * State Toggles:
 *                      Fixed Position
 *                      Ignore on Arrange
 *
 * no idea what this is:
 * EXPANDED_SIZE
 */

#undef DEBUG
static char *cvsident = "$Id: Gnome.c,v 1.4 2003/03/25 17:32:49 sasha Exp $";

#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "../../configure.h"
#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/module.h"

#include "Gnome.h"

#ifdef DEBUG
#define LOG(s) fprintf (stderr, "%s\n", s)
#else
#define LOG(s)
#endif

static s_list *window_list;	/* single linked list of window id's etc */
static int current_desk = 0;	/* current workspace */
static char **desk_names;	/* holds workspace names */
static int desk_count = 0;	/* number of workspaces */
static Bool PAGES = False;	/* virtual paging or no */
static int area_x = 0;		/* area coordinates for virtual paging */
static int area_y = 0;		/*                ""                   */


ScreenInfo Scr;

/* inserts a window id into the list
 * args: list, id
 */
list_item *
s_list_insert (s_list * list, long id)
{
  list_item *item = NULL;

  item = (list_item *) safemalloc (sizeof (list_item));

  if (!item)
    return NULL;

  if (list->first == NULL)
    {
      list->first = item;
      list->last = item;
    }
  else
    {
      list->last->next = item;
    }
  item->next = NULL;
  list->last = item;
  item->id = id;
  return item;
}

/* removes an item from list by window id
 * args: list, id
 */
int
s_list_remove_by_data (s_list * list, long id)
{
  list_item *tmp1;

  tmp1 = s_list_find_by_data (list, id);

  if (tmp1 == NULL)
    return 0;

  /* this is our only list item */
  if (tmp1 == list->first && tmp1 == list->last)
    {
      free (tmp1);
      list->first = NULL;
      list->last = NULL;
      return 1;
    }
  /* the last item */
  else if (tmp1 == list->last)
    {
      list_item *tmp2;

      tmp2 = list->first;
      while (tmp2)
	{
	  if (tmp2->next == list->last)
	    break;
	  tmp2 = tmp2->next;
	}
      free (tmp1);
      tmp2->next = NULL;
      list->last = tmp2;
      return 1;
    }
  /* the first item */
  else if (tmp1 == list->first)
    {
      list->first = tmp1->next;
      free (tmp1);
      return 1;
    }
  /*somewhere in between */
  else
    {
      list_item *tmp2;

      tmp2 = list->first;
      while (tmp2)
	{
	  if (tmp2->next == tmp1)
	    break;
	  tmp2 = tmp2->next;
	}
      tmp2->next = tmp1->next;
      free (tmp1);
      return 1;
    }
  return 0;
}

/* searches for a window id in a list
 * args: list, id
 */
list_item *
s_list_find_by_data (s_list * list, long id)
{
  list_item *tmp;

  tmp = list->first;

  if (list->first == NULL && list->last == NULL)
    return NULL;

  while (tmp)
    {
      if (tmp->id == id)
	return tmp;
      tmp = tmp->next;
    }
  return NULL;
}

/* creates a new single linked list */
s_list *
s_list_new ()
{
  s_list *list;

  list = (s_list *) safemalloc (sizeof (s_list));

  if (!list)
    return NULL;

  list->first = list->last = NULL;
  return list;
}

/* destroys a single linked list
 * args: list
 */
int
s_list_free (s_list * list)
{
  list_item *tmp1, *tmp2;

  tmp1 = list->first;
  while (tmp1)
    {
      tmp2 = tmp1->next;
      free (tmp1);
      tmp1 = tmp2;
    }
  free (list);
  return 0;
}

/* returns the count of items in a list, skips transient and windowlistskip
 * windows
 * args: list
 */
int
s_list_count (s_list * list)
{
  list_item *tmp;
  int count = 0;

  tmp = list->first;

  while (tmp)
    {
      if (!(tmp->flags & WINDOWLISTSKIP))
	if (!(tmp->flags & TRANSIENT))
	  count++;
      tmp = tmp->next;
    }
  return count;
}

/* sets up the properties and the supported protocols list */
static void
gnome_compliance_init ()
{
  Atom supported_list[12];
  int count;

  root_win = RootWindow (dpy, DefaultScreen(dpy));

  /* supporting WM check */
  _XA_WIN_SUPPORTING_WM_CHECK = XInternAtom (dpy, "_WIN_SUPPORTING_WM_CHECK",
					     False);
  _XA_WIN_PROTOCOLS = XInternAtom (dpy, "_WIN_PROTOCOLS", False);
  _XA_WIN_LAYER = XInternAtom (dpy, "_WIN_LAYER", False);
  _XA_WIN_STATE = XInternAtom (dpy, "_WIN_STATE", False);
  _XA_WIN_HINTS = XInternAtom (dpy, "_WIN_HINTS", False);
  _XA_WIN_APP_STATE = XInternAtom (dpy, "_WIN_APP_STATE", False);
/*_XA_WIN_EXPANDED_SIZE = XInternAtom(dpy, "_WIN_EXPANDED_SIZE", False);*/
  _XA_WIN_ICONS = XInternAtom (dpy, "_WIN_ICONS", False);
  _XA_WIN_WORKSPACE = XInternAtom (dpy, "_WIN_WORKSPACE", False);
  _XA_WIN_WORKSPACE_COUNT = XInternAtom (dpy, "_WIN_WORKSPACE_COUNT", False);
  _XA_WIN_WORKSPACE_NAMES = XInternAtom (dpy, "_WIN_WORKSPACE_NAMES", False);
  _XA_WIN_CLIENT_LIST = XInternAtom (dpy, "_WIN_CLIENT_LIST", False);
  _XA_WIN_DESKTOP_BUTTON_PROXY = XInternAtom (dpy, "_WIN_DESKTOP_BUTTON_PROXY",
					      False);

  if (PAGES)
    {
      _XA_WIN_AREA_COUNT = XInternAtom (dpy, "_WIN_AREA_COUNT", False);
      _XA_WIN_AREA = XInternAtom (dpy, "_WIN_AREA", False);
    }

  /* create the GNOME window */
  gnome_win = XCreateSimpleWindow (dpy, root_win, 0, 0, 5, 5, 0, 0, 0);

  /* supported WM check */
  XChangeProperty (dpy, root_win, _XA_WIN_SUPPORTING_WM_CHECK,
	 XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &gnome_win, 1);
  XChangeProperty (dpy, gnome_win, _XA_WIN_SUPPORTING_WM_CHECK,
	 XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &gnome_win, 1);

  /* supported protocols */
  count = 0;

  supported_list[count++] = _XA_WIN_LAYER;	/* done */
  supported_list[count++] = _XA_WIN_STATE;	/* done */
  supported_list[count++] = _XA_WIN_HINTS;	/* done */
  supported_list[count++] = _XA_WIN_APP_STATE;	/* ???? */
  /*supported_list[count++] = _XA_WIN_EXPANDED_SIZE; */
  supported_list[count++] = _XA_WIN_ICONS;	/* ???? */
  supported_list[count++] = _XA_WIN_WORKSPACE;	/* done */
  supported_list[count++] = _XA_WIN_WORKSPACE_COUNT;	/* done */
  supported_list[count++] = _XA_WIN_WORKSPACE_NAMES;	/* done */
  supported_list[count++] = _XA_WIN_CLIENT_LIST;	/* done */
  if (PAGES)
    {
      supported_list[count++] = _XA_WIN_AREA_COUNT;
      supported_list[count++] = _XA_WIN_AREA;
    }

  XChangeProperty (dpy, root_win, _XA_WIN_PROTOCOLS, XA_ATOM, 32, PropModeReplace,
		   (unsigned char *) supported_list, count);

}

/* sets the number of virtual pages per desk */
static void
gnome_set_area_count ()
{
  unsigned long val[2];

  val[0] = 2;
  val[1] = 2;
  XChangeProperty (dpy, root_win, _XA_WIN_AREA_COUNT, XA_CARDINAL, 32,
		   PropModeReplace, (unsigned char *) &val, 2);
}

/* sets the current visible page */
static void
gnome_set_current_area (int x, int y)
{
  unsigned long val[2];

  val[0] = x;
  val[1] = y;
  XChangeProperty (dpy, root_win, _XA_WIN_AREA, XA_CARDINAL, 32,
		   PropModeReplace, (unsigned char *) &val, 2);
}

/* sets the current workspace */
static void
gnome_set_current_workspace (int current_desk)
{

  XChangeProperty (dpy, root_win, _XA_WIN_WORKSPACE, XA_CARDINAL, 32,
		   PropModeReplace, (unsigned char *) &current_desk, 1);
}

/* sets the number of workspaces */
static void
gnome_set_workspace_count (int workspaces)
{

  XChangeProperty (dpy, root_win, _XA_WIN_WORKSPACE_COUNT, XA_CARDINAL, 32,
		   PropModeReplace, (unsigned char *) &workspaces, 1);

}

/* sets up the 'workspace names' property */
static void
gnome_set_workspace_names (int count, char **names)
{
  XTextProperty text;

  if (XStringListToTextProperty (names, count, &text))
    {
      XSetTextProperty (dpy, root_win, &text, _XA_WIN_WORKSPACE_NAMES);
      XFree (text.value);
    }
}

/* sets the window list property, skips transients and winlistskip windows */
static void
gnome_set_client_list (s_list * list)
{
  Window *windows = NULL;
  int windows_count = 0, count;
  list_item *tmp;

  windows_count = s_list_count (list);

  if (windows_count != 0)
    {
      windows = safemalloc (sizeof (Window) * windows_count);

      if (!windows)
	{
	  fprintf (stderr, "gnome_set_client_list: malloc failed\n");
	  return;
	}
      tmp = list->first;
      count = 0;
      while (tmp)
	{
	  if (!(tmp->flags & WINDOWLISTSKIP))
	    {
	      if (!(tmp->flags & TRANSIENT))
		windows[count++] = tmp->id;
	    }

	  tmp = tmp->next;
	}
      XChangeProperty (dpy, root_win, _XA_WIN_CLIENT_LIST, XA_CARDINAL, 32,
		 PropModeReplace, (unsigned char *) windows, windows_count);
      free (windows);
    }
}

/* translates AS window flags to GNOME state properties */
static void
gnome_set_win_hints (list_item * aswin)
{
  long flags = 0;

  if (aswin->flags & STICKY)
    flags |= WIN_STATE_STICKY;

  if (aswin->flags & ICONIFIED)
    flags |= WIN_STATE_MINIMIZED;

  if (aswin->flags & SHADED)
    flags |= WIN_STATE_SHADED;

  if (!(aswin->flags & STICKY))
    if (aswin->workspace != current_desk)
      flags |= WIN_STATE_HID_WORKSPACE;

  if (aswin->flags & TRANSIENT)
    flags |= WIN_STATE_HID_TRANSIENT;

  XChangeProperty (dpy, (Window) aswin->id, _XA_WIN_WORKSPACE, XA_CARDINAL,
	       32, PropModeReplace, (unsigned char *) &aswin->workspace, 1);

  XChangeProperty (dpy, (Window) aswin->id, _XA_WIN_STATE, XA_CARDINAL,
		   32, PropModeReplace, (unsigned char *) &flags, 1);
}

/* layer change requests are handled here, GNOME -> AS */
static void
gnome_handle_win_layer (XClientMessageEvent * event, Window id, int layer)
{
  int new_layer;
  list_item *aswin;
  char as_mesg[50];


  if (event)
    {
      new_layer = event->data.l[0];
      aswin = s_list_find_by_data (window_list, (long) event->window);
      if (!aswin)
	return;
      id = aswin->id;
    }
  else
    new_layer = layer;

  /*desktop layers */
  switch (new_layer)
    {
    case WIN_LAYER_DESKTOP:
      LOG ("    WIN_LAYER_DESKTOP");
      sprintf (as_mesg, "SetLayer -2\n");
      SendInfo (fd, as_mesg, id);
      break;
    case WIN_LAYER_BELOW:
      LOG ("    WIN_LAYER_BELOW");
      sprintf (as_mesg, "SetLayer -1\n");
      SendInfo (fd, as_mesg, id);
      break;
    case WIN_LAYER_NORMAL:
      LOG ("    WIN_LAYER_NORMAL");
      sprintf (as_mesg, "SetLayer 0\n");
      SendInfo (fd, as_mesg, id);
      break;
    case WIN_LAYER_ONTOP:
      LOG ("    WIN_LAYER_ONTOP");
      sprintf (as_mesg, "SetLayer 1\n");
      SendInfo (fd, as_mesg, id);
      break;
    case WIN_LAYER_DOCK:
      LOG ("    WIN_LAYER_DOCK");
      sprintf (as_mesg, "SetLayer 1\n");
      SendInfo (fd, as_mesg, id);
      break;
    case WIN_LAYER_ABOVE_DOCK:
      LOG ("    WIN_LAYER_ABOVE_DOCK");
      sprintf (as_mesg, "SetLayer 2\n");
      SendInfo (fd, as_mesg, id);
      break;
    default:
      fprintf (stderr, "%s: I don't know anything about layer: %d\n", MyName, new_layer);
      break;
    }
}

/* hints change requests are handled here, GNOME -> AS */
static void
gnome_handle_win_hints (XClientMessageEvent * event, Window id, long flags)
{
  list_item *aswin;
  int hints;
  Bool update = False;
  unsigned long tmpflags = 0;

  if (event)
    {
      hints = event->data.l[1];
      aswin = s_list_find_by_data (window_list, (long) event->window);
    }
  else
    {
      hints = flags;
      aswin = s_list_find_by_data (window_list, id);
    }


  if (!aswin)
    {
      LOG ("gnome_handle_win_hints: got an event for an unknown window");
      return;
    }

  if (hints & WIN_HINTS_SKIP_FOCUS)
    {
      tmpflags |= NOFOCUS;
      update = True;
      LOG ("    SKIPFOCUS");
    }

  if (hints & WIN_HINTS_SKIP_WINLIST)
    {
      tmpflags |= WINDOWLISTSKIP;
      update = True;
      LOG ("    SKIPWINLIST");
    }
  if (hints & WIN_HINTS_DO_NOT_COVER)
    {
      tmpflags |= AVOID_COVER;
      update = True;
      LOG ("    AVOIDCOVER");
    }
  if (hints & WIN_HINTS_SKIP_TASKBAR)
    {
      tmpflags |= CIRCULATESKIP;
      update = True;
      LOG ("    SKIPTASKBAR");
    }
  if (hints & WIN_HINTS_GROUP_TRANSIENT)
    {
      tmpflags |= TRANSIENT;
      update = True;
      LOG ("    TRANSIENT");
    }
  if (hints & WIN_HINTS_FOCUS_ON_CLICK)
    LOG ("    FOCUSONCLICK ignored");

  update = True;
  if (update)
    {
      char as_msg[50];
      sprintf (as_msg, "SET_FLAGS %lu\n", tmpflags);
      SendInfo (fd, as_msg, aswin->id);
      gnome_set_client_list (window_list);
    }
}

/* state requests, GNOME -> AS */
static void
gnome_handle_win_state (XClientMessageEvent * event, Window id, long mask)
{
  long new_members, change_mask;
  list_item *aswin;

  if (event)
    {
      aswin = s_list_find_by_data (window_list, (long) event->window);
      change_mask = event->data.l[0];
      new_members = event->data.l[1];
    }
  else
    {
      aswin = s_list_find_by_data (window_list, id);
      new_members = mask;
      change_mask = 0;
    }
  if (!aswin)
    return;

  /* stick or unstick */
  if (new_members & WIN_STATE_STICKY)
    {
      if (!(aswin->flags & STICKY))
	{
	  LOG ("    STICKY req");
	  SendInfo (fd, "Stick\n", aswin->id);
	}
    }
  else if (change_mask & WIN_STATE_STICKY)
    {
      if (aswin->flags & STICKY)
	{
	  LOG ("    UNSTICKY req");
	  SendInfo (fd, "Stick\n", aswin->id);
	}
    }

  if ((new_members & WIN_STATE_MAXIMIZED_VERT) &&
      (new_members & WIN_STATE_MAXIMIZED_HORIZ))
    {
      LOG ("    MAX req");
      SendInfo (fd, "Maximize 100% 100%\n", aswin->id);
    }
  else
    {
      /* maximize verticaly */
      if (new_members & WIN_STATE_MAXIMIZED_VERT)
	{
	  LOG ("    MAXVERT req");
	  SendInfo (fd, "Maximize 0% 100%\n", aswin->id);
	}

      /* maximize horizontaly */
      if (new_members & WIN_STATE_MAXIMIZED_HORIZ)
	{
	  LOG ("    MAXHOR req");
	  SendInfo (fd, "Maximize 100% 0%\n", aswin->id);
	}
    }
  /* shade or unshade */
  if (new_members & WIN_STATE_SHADED)
    {
      if (!(aswin->flags & SHADED))
	{
	  LOG ("    SHADE req");
	  SendInfo (fd, "Shade 1\n", aswin->id);
	}
    }
  else if (change_mask & WIN_STATE_SHADED)
    {
      if (aswin->flags & SHADED)
	SendInfo (fd, "Shade 0\n", aswin->id);
    }

  if (new_members & WIN_STATE_HIDDEN)
    {
      LOG ("    HIDDEN req");
      SendInfo (fd, "Iconify 1\n", aswin->id);
    }
  if (new_members & WIN_STATE_MINIMIZED)
    {
      LOG ("    MINI req");
      SendInfo (fd, "Iconify 1\n", aswin->id);
    }
  /* check if workspace changed, layers, hints and state are handled
   * elsewhere
   */
  if (event)
    gnome_get_prop_workspace (aswin->id);
}

/* initial app state, get GNOME state from windows that set them */
static void
gnome_get_prop_state (Window id)
{
  Atom ret_type;
  int fmt;
  unsigned long nitems, bytes_after;
  long flags, *data = 0;

  if (XGetWindowProperty (dpy, id, _XA_WIN_STATE, 0, 1, False,
			XA_CARDINAL, &ret_type, &fmt, &nitems, &bytes_after,
			  (unsigned char **) &data) == Success && data)
    {
      flags = *data;
      XFree (data);
      gnome_handle_win_state (NULL, id, flags);
    }

}

/* initial app hints, get GNOME hints from apps that use them */
static void
gnome_get_prop_hints (Window id)
{
  Atom ret_type;
  int fmt;
  unsigned long nitems;
  unsigned long bytes_after;
  long flags, *data = 0;
  if (XGetWindowProperty (dpy, id, _XA_WIN_HINTS, 0, 1, False,
			  XA_CARDINAL, &ret_type, &fmt, &nitems,
			  &bytes_after,
			  (unsigned char **) &data) == Success && data)
    {
      flags = *data;
      XFree (data);
      gnome_handle_win_hints (NULL, id, flags);
    }
}

/* initial app layer, get GNOME layers from apps that use them */
static void
gnome_get_prop_layer (Window id)
{
  Atom ret_type;
  int fmt;
  unsigned long nitems;
  unsigned long bytes_after;
  long val, *data = 0;
  if (XGetWindowProperty (dpy, id, _XA_WIN_LAYER, 0, 1, False,
			  XA_CARDINAL, &ret_type, &fmt, &nitems,
			  &bytes_after,
			  (unsigned char **) &data) == Success && data)
    {
      val = *data;
      XFree (data);
      gnome_handle_win_layer (NULL, id, (int) val);
    }

}

/* initial app workspace, get it from the window if set */
static void
gnome_get_prop_workspace (Window id)
{
  Atom ret_type;
  int fmt;
  unsigned long nitems;
  unsigned long bytes_after;
  long val, *data = 0;

  LOG ("gnome_get_prop_workspace");

  if (XGetWindowProperty (dpy, id, _XA_WIN_WORKSPACE, 0, 1, False,
			  XA_CARDINAL, &ret_type, &fmt, &nitems,
			  &bytes_after,
			  (unsigned char **) &data) == Success && data)
    {
      val = *data;
      XFree (data);
      if (val != current_desk)
	{
	  char msg[50];
	  LOG ("ChangeDesk req");
	  sprintf (msg, "WindowsDesk %d\n", (int) val);
	  SendInfo (fd, msg, id);
	}

    }
}

/* initial hints, check for GNOME hints on a window */
static void
gnome_check_client_hints (Window id)
{
  /* state */
  gnome_get_prop_state (id);

  /* hints */
  gnome_get_prop_hints (id);

  /* layer */
  gnome_get_prop_layer (id);

  /* workspace */
  gnome_get_prop_workspace (id);
}

/* signal handler, also called by AS when it wants to kill the module */
void
DeadPipe (int sig)
{
#ifdef DEBUG_ALLOCS
  int i;
#endif
  switch (sig)
    {
    case SIGSEGV:
      fprintf (stderr, "Segmentation fault in %s, please send a bug report\n", MyName);
      exit (-1);
    case SIGINT:
      fprintf (stderr, "User abort, exiting\n");
      exit (-1);
    case SIGPIPE:
      fprintf (stderr, "pipe\n");
    default:
      break;
    }
#ifdef DEBUG_ALLOCS
  s_list_free (window_list);
  for (i = 0; i < desk_count; i++)
    free (desk_names[i]);
  free (desk_names);
  print_unfreed_mem ();
#endif
  exit (0);
}

/* set_as_mask:
 * set the mask on the pipe
 */

static void
set_as_mask (long unsigned mask)
{
  char set_mask_mesg[255];

  sprintf (set_mask_mesg, "SET_MASK %lu\n", mask);
  SendInfo (fd, set_mask_mesg, 0);
}

/* process AS events */
static void
process_message (unsigned long type, int elements, unsigned long *body)
{
  int status = 0;

  switch (type)
    {
    case M_CONFIGURE_WINDOW:
      status = list_configure (body);
      break;
    case M_ADD_WINDOW:
      status = list_add_window (body);
      break;
    case M_DESTROY_WINDOW:
      status = list_destroy_window (body);
      break;
    case M_ICONIFY:
      status = list_iconify (body);
      break;
    case M_DEICONIFY:
      status = list_deiconify (body);
      break;
    case M_WINDOW_NAME:
      status = list_window_name (body);
      break;
    case M_ICON_NAME:
      status = list_icon_name (body);
      break;
    case M_END_WINDOWLIST:
      status = list_end ();
      break;
    case M_NEW_PAGE:
      if (PAGES)
	status = list_page_change (body);
      break;
    case M_NEW_DESK:
      LOG ("M_NEW_DESK");
      status = list_desk_change (body);
      break;
    case M_TOGGLE_PAGING:
      break;
    default:
      break;
    }
}

/* list_configure:
 * pipe configure events
 */

int
list_configure (unsigned long *body)
{
  list_item *item;

  if (body[0] == gnome_win)
    return 0;

  if (body[0] == None)
    return 0;

  if ((item = s_list_find_by_data (window_list, body[0])) != NULL)
    {
      Bool update = False;

      if (item->flags != body[8])
	{
	  item->flags = body[8];
	  update = True;
	}
      if (item->workspace != body[7])
	{
	  if (!(item->flags & ICONIFIED))
	    {
	      item->workspace = body[7];
	      update = True;
	    }
	}

      if (update)
	{
	  gnome_set_win_hints (item);
	  gnome_set_client_list (window_list);
	  return 1;
	}
      return 0;
    }
  return (list_add_window (body));
}
/* list_add_window:
 * pipe add window events
 */

int
list_add_window (unsigned long *body)
{
  list_item *item;

  LOG ("list_add");

  if (body[0] == None)
    return 0;

  if (s_list_find_by_data (window_list, body[0]))
    {
      LOG ("already exists!");
      return 0;
    }
  if (body[0] == gnome_win)
    return 0;

  item = s_list_insert (window_list, body[0]);
  item->flags = body[8];
  item->workspace = body[7];
  item->area_x = area_x;
  item->area_y = area_y;
  gnome_check_client_hints (item->id);
  gnome_set_win_hints (item);
  gnome_set_client_list (window_list);
  return 1;
}

/* list_window_name:
 * pipe chage of window name events
 */

int
list_window_name (unsigned long *body)
{
  return 0;
}

/* list_icon_name:
 * pipe change of icon name events
 */

int
list_icon_name (unsigned long *body)
{
  return 0;
}

/* list_destroy_window:
 * pipe destroyed windows event
 */

int
list_destroy_window (unsigned long *body)
{
  if (s_list_find_by_data (window_list, body[0]))
    {
      s_list_remove_by_data (window_list, body[0]);
      gnome_set_client_list (window_list);
      return 1;
    }
  return 0;
}

/* list_end
 * end of window list from pipe
 */

static int
list_end (void)
{
  gnome_set_current_workspace (current_desk);
  gnome_set_workspace_count (desk_count);
  gnome_set_workspace_names (desk_count, desk_names);
  if (PAGES)
    {
      gnome_set_area_count ();
      gnome_set_current_area (0, 0);
    }
  gnome_set_client_list (window_list);
  return 0;
}

/* list_deiconify:
 * window deiconified from the pipe
 */

int
list_deiconify (unsigned long *body)
{
  return 0;
}

/* list_iconify:
 * window iconified from the pipe
 */

int
list_iconify (unsigned long *body)
{
  return 0;
}

/*rafa */
int
list_page_change (unsigned long *body)
{
  current_desk = (int) body[2];

  if (body[2] != 10000)
    {

      if (body[0] > 0)
	area_x = 1;
      else
	area_x = 0;

      if (body[1] > 0)
	area_y = 1;
      else
	area_y = 0;
      gnome_set_current_area (area_x, area_y);
      XFlush (dpy);
    }
  return 0;
}

int
list_desk_change (unsigned long *body)
{
  if (body[0] != 10000)
    {
      current_desk = (unsigned int) body[0];
      gnome_set_current_workspace (current_desk);
    }
  return 0;
}
static int
error_handler (Display * disp, XErrorEvent * event)
{
/*    fprintf (stderr, "%s: internal error, error code %d, request code %d, minor code %d\n  . Please send a bug report\n", MyName, event->error_code, event->request_code, event->minor_code); */
  return 0;
}

static void
event_handler ()
{
  XEvent Event;
  char tmp[255];

  while (XPending (dpy))
    {
      XNextEvent (dpy, &Event);
      switch (Event.type)
	{
	case PropertyNotify:
	  if (Event.xproperty.window == gnome_win)
	    {
	      gnome_check_client_hints (Event.xclient.window);
	      LOG ("PropNot");
	    }
	  break;
	case ClientMessage:
	  /* change workspace request */
	  if (Event.xclient.message_type == _XA_WIN_WORKSPACE)
	    {
	      int desk;
	      desk = Event.xclient.data.l[0];
	      sprintf (tmp, "Desk 10000 %d\n", desk);
	      SendInfo (fd, tmp, None);
	    }
	  /* change layer request */
	  else if (Event.xclient.message_type == _XA_WIN_LAYER)
	    {
	      LOG ("_XA_WIN_LAYER request");
	      gnome_handle_win_layer (&Event.xclient, None, 0);
	    }
	  /* state requests */
	  else if (Event.xclient.message_type == _XA_WIN_STATE)
	    {
	      LOG ("_XA_WIN_STATE request");
	      gnome_handle_win_state (&Event.xclient, None, 0);
	    }
	  /* win hints requests */
	  else if (Event.xclient.message_type == _XA_WIN_HINTS)
	    {
	      LOG ("_XA_WIN_HINTS request");
	      gnome_handle_win_hints (&Event.xclient, None, 0);
	    }
	  break;
	default:
	  break;
	}
    }
}
/******************************************************************************
  EndLessLoop -  Read and redraw until we get killed, blocking when can't read
******************************************************************************/
static void
EndLessLoop ()
{
  fd_set readset;
  struct timeval tv;

  while (1)
    {
      FD_ZERO (&readset);
      FD_SET (fd[1], &readset);
      FD_SET (x_fd, &readset);
      tv.tv_sec = 0;
      tv.tv_usec = 0;
#ifdef __hpux
      if (!select (fd_width, (int *) &readset, NULL, NULL, &tv))
	{
	  XPending (dpy);
	  FD_ZERO (&readset);
	  FD_SET (fd[1], &readset);
	  FD_SET (x_fd, &readset);
	  select (fd_width, (int *) &readset, NULL, NULL, NULL);
	}
#else
      if (!select (fd_width, &readset, NULL, NULL, &tv))
	{
	  XPending (dpy);
	  FD_ZERO (&readset);
	  FD_SET (fd[1], &readset);
	  FD_SET (x_fd, &readset);
	  select (fd_width, &readset, NULL, NULL, NULL);
	}
#endif
      if (FD_ISSET (x_fd, &readset))
	event_handler ();
      if (!FD_ISSET (fd[1], &readset))
	continue;
      read_as_socket ();
    }
}

static void
read_as_socket ()
{
  unsigned long header[3], *body;
  if (ReadASPacket (fd[1], header, &body) > 0)
    {
      process_message (header[1], header[2], body);
      free (body);
    }
}

static void
version (void)
{
  printf ("%s version %s\n%s\n", MyName, VERSION, cvsident);
  exit (0);
}

static void
usage (void)
{
  printf ("Usage:\n"
	  "%s [-f [config file]] [-v|--version] [-h|--help]\n", MyName);
  exit (0);
}

static void
default_config ()
{
  int i;

  desk_count = 4;

  desk_names = safemalloc (sizeof (char *));

  for (i = 0; i < 4; i++)
    {
      if (i != 0)
	desk_names = realloc (desk_names, sizeof (char *) + i * sizeof (char *));
      desk_names[i] = safemalloc (10);
      sprintf (desk_names[i], "Desktop %i", i);
    }
}


static void
parse_config (char *cfgfile)
{
  char *line, *tline, *tmp;
  FILE *ptr;
  int len;

  if ((ptr = fopen (cfgfile, "r")) == NULL)
    {
      fprintf (stderr, "%s, can\'t open config file %s, using defaults\n", MyName, "file");
      default_config ();
      return;
    }

  line = (char *) safemalloc (MAXLINELENGTH);
  len = strlen (MyName);


  while ((tline = fgets (line, MAXLINELENGTH, ptr)) != NULL)
    {
      while (isspace (*tline))
	tline++;
      if ((*tline == '*') && (!mystrncasecmp (tline + 1, MyName, len)))
	{
	  tline += len + 1;
	  if (!mystrncasecmp (tline, "DeskName", 8))
	    {
	      if (desk_count == 0)
		desk_names = malloc (sizeof (char *));
	      else
		desk_names = realloc (desk_names,
			    sizeof (char *) + desk_count * sizeof (char *));
	      tmp = tline + 8 + 1;
	      desk_names[desk_count] = (char *) malloc (strlen (tmp) + 1);
	      strcpy (desk_names[desk_count], tmp);
	      desk_count++;
	    }
	  if (!mystrncasecmp (tline, "Pages", 5))
	    {
	      if (atoi (tline + 5) > 0)
		{
		  LOG ("PAGING IS ON");
		  PAGES = True;
		}
	    }
	}
    }

  if (desk_count == 0)
    default_config ();

  free (line);
  fclose (ptr);
}

int
main (int argc, char **argv)
{
  char *temp;
  int i;
  char *global_config_file = NULL;
  char tmp[128];
  FILE *fp = NULL;

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
  signal (SIGSEGV, DeadPipe);
  signal (SIGINT, DeadPipe);

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

  x_fd = XConnectionNumber (dpy);
  fd_width = GetFdWidth ();

  XSetErrorHandler (error_handler);

  if (global_config_file != NULL)
    temp = PutHome (global_config_file);
  else
    {
      memset (tmp, 128, '\0');
      sprintf (tmp, "%s/Gnome", AFTER_DIR);
      temp = PutHome (tmp);
      if ((fp = fopen (temp, "r")) == NULL)
	{
	  sprintf (tmp, "%s/Gnome", AFTER_SHAREDIR);
	  free (temp);
	  temp = PutHome (tmp);
	}
    }

  if (fp)
    fclose (fp);

  parse_config (temp);
  free (temp);

  gnome_compliance_init ();

  window_list = s_list_new ();


  set_as_mask ((long unsigned) mask_reg);
  SendInfo (fd, "Send_WindowList\n", 0);

  XSelectInput (dpy, root_win, PropertyChangeMask | SubstructureNotifyMask);
  XSelectInput (dpy, gnome_win, PropertyChangeMask);

  EndLessLoop ();
  return 0;
}
