/*
 * Copyright (C) 1999 Sasha Vasko <sasha at aftercode.net>
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

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>

#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>

#include "../configure.h"

#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* SHAPE */

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/style.h"
#include "../include/mystyle.h"
#include "../include/screen.h"
#include "../include/module.h"
#include "../libAfterBase/selfdiag.h"
#include "../libAfterImage/afterimage.h"

#ifdef HAVE_XINERAMA
static int XineEventBase, XineErrorBase;
#endif
static Bool as_X_synchronous_mode = False ;


void get_Xinerama_rectangles(ScreenInfo *scr)
{
#ifdef HAVE_XINERAMA
  register int i ;
  XineramaScreenInfo *s ;
  if( (s = XineramaQueryScreens( dpy, &(scr->xinerama_screens_num) )) != NULL )
  {
	  scr->xinerama_screens = safemalloc( sizeof(XRectangle)*scr->xinerama_screens_num );
	  for( i = 0 ; i < scr->xinerama_screens_num ; ++i )
	  {
		  scr->xinerama_screens[i].x = s[i].x_org ;
		  scr->xinerama_screens[i].y = s[i].y_org ;
		  scr->xinerama_screens[i].width = s[i].width ;
		  scr->xinerama_screens[i].height = s[i].height ;
	  }
	  XFree( s );
  }
#else
  scr->xinerama_screens = NULL ;
  scr->xinerama_screens_num = 0 ;
#endif
}

Bool
set_synchronous_mode(Bool enable)
{
    Bool old = as_X_synchronous_mode ;
    XSynchronize( dpy, enable );
    as_X_synchronous_mode = enable ;

    return old ;
}

Bool
is_synchronous_request( int request_code )
{
    if( as_X_synchronous_mode )
        return True;
    switch( request_code )
    {
        case X_CreateWindow              :
        case X_GetWindowAttributes       :
        case X_GetGeometry               :
        case X_QueryTree                 :
        case X_InternAtom                :
        case X_GetAtomName               :
        case X_GetProperty               :
        case X_ListProperties            :
        case X_GetSelectionOwner         :
        case X_ConvertSelection          :
        case X_QueryPointer              :
        case X_GetMotionEvents           :
        case X_TranslateCoords           :
        case X_GetInputFocus             :
        case X_QueryKeymap               :
        case X_OpenFont                  :
        case X_QueryFont                 :
        case X_QueryTextExtents          :
        case X_ListFonts                 :
        case X_ListFontsWithInfo         :
        case X_GetFontPath               :
        case X_CreatePixmap              :
        case X_CreateGC                  :
        case X_GetImage                  :
        case X_CreateColormap            :
        case X_CopyColormapAndFree       :
        case X_ListInstalledColormaps    :
        case X_AllocColor                :
        case X_AllocNamedColor           :
        case X_AllocColorCells           :
        case X_AllocColorPlanes          :
        case X_QueryColors               :
        case X_LookupColor               :
        case X_CreateCursor              :
        case X_CreateGlyphCursor         :
        case X_QueryBestSize             :
        case X_QueryExtension            :
        case X_ListExtensions            :
        case X_GetKeyboardMapping        :
        case X_GetKeyboardControl        :
        case X_GetPointerControl         :
        case X_GetScreenSaver            :
        case X_ListHosts                 :
        case X_GetPointerMapping         :
        case X_GetModifierMapping        :
            return True;
            break;
        default:
            break;
    }
    return False;
}

int
Empty_XErrorHandler (Display * dpy, XErrorEvent * event)
{
    return 0;
}

int
ASErrorHandler (Display * dpy, XErrorEvent * event)
{
	char         *err_text;
	fprintf(stderr, "%s has encountered the following problem interacting with X Windows :\n", MyName);
    if( event && dpy )
    {
        err_text = safemalloc (128);
        strcpy (err_text, "unknown error");
        XGetErrorText (dpy, event->error_code, err_text, 120);
        fprintf (stderr, "      Request: %d,    Error: %d(%s)\n", event->request_code, event->error_code, err_text);
        free (err_text);
        fprintf (stderr, "      in resource: 0x%lX\n", event->resourceid);
#ifndef __CYGWIN__
        if( is_synchronous_request( event->request_code ) )
            print_simple_backtrace ();
#endif
    }
    return 0;
}


int
ConnectX (ScreenInfo * scr, char *display_name, unsigned long message_mask)
{
  int x_fd;
  /* Initialize X connection */
  if (!(dpy = XOpenDisplay (display_name)))
    {
      fprintf (stderr, "%s: can't open display %s", MyName,
	       XDisplayName (display_name));
      exit (1);
    }
  x_fd = XConnectionNumber (dpy);
  XSetErrorHandler (ASErrorHandler);
  
  memset( scr, 0x00, sizeof(ScreenInfo));

  scr->screen = DefaultScreen (dpy);
  scr->Root = RootWindow (dpy, scr->screen);
  if (scr->Root == None)
    {
      fprintf (stderr, "%s: Screen %d is not valid ", MyName, (int) scr->screen);
      exit (1);
    }
  scr->MyDisplayWidth = DisplayWidth (dpy, scr->screen);
  scr->MyDisplayHeight = DisplayHeight (dpy, scr->screen);
  scr->CurrentDesk = -1;
  
  scr->asv = create_asvisual( dpy, scr->screen, scr->d_depth, NULL );
  scr->d_depth = scr->asv->visual_info.depth;

#ifdef HAVE_XINERAMA
  if( XineramaQueryExtension(dpy, &XineEventBase, &XineErrorBase))
	  get_Xinerama_rectangles(scr);
#endif

  XSelectInput (dpy, scr->Root, message_mask);
  return x_fd;
}

void QuietlyDestroyWindow( Window w )
{
	int	 (*old_handler) (Display * dpy, XErrorEvent * event);

	old_handler = XSetErrorHandler (Empty_XErrorHandler);
	XDestroyWindow( dpy, w );
	XSync(dpy, False);
	XSetErrorHandler( old_handler );
}

void
InitAtoms (Display * dpy, ASAtom * atoms)
{
  int i;
  for (i = 0; atoms[i].name != NULL; i++)
    atoms[i].atom = XInternAtom (dpy, atoms[i].name, False);
}

void DeadPipe (int nonsense);

int
My_XNextEvent (Display * dpy, int x_fd, int as_fd, void (*as_msg_handler) (unsigned long type, unsigned long *body), XEvent * event)
{
  fd_set in_fdset;
  ASMessage msg;
  static int miss_counter = 0;
  struct timeval tv;
  struct timeval *t = NULL;

  if (as_msg_handler == NULL || as_fd < 0 || x_fd < 0)
    return 0;

  if (XPending (dpy))
    {
      XNextEvent (dpy, event);
      miss_counter = 0;
      return 1;
    }
  else
    {
      FD_ZERO (&in_fdset);
      FD_SET (x_fd, &in_fdset);
      FD_SET (as_fd, &in_fdset);
      if (timer_delay_till_next_alarm ((time_t *) & tv.tv_sec, (time_t *) & tv.tv_usec))
	t = &tv;
#ifdef __hpux
      while (select (MAX(x_fd,as_fd)+1, (int *) &in_fdset, 0, 0, t) == -1)
	if (errno != EINTR)
	  break;
#else
      while (select (MAX(x_fd,as_fd)+1, &in_fdset, 0, 0, t) == -1)
	if (errno != EINTR)
	  break;
#endif
      if (FD_ISSET (x_fd, &in_fdset))
	{
	  if (XPending (dpy))
	    {
	      XNextEvent (dpy, event);
	      miss_counter = 0;
	      return 1;
	    }
	  if (++miss_counter > 100)
	    DeadPipe (0);
	}
      if (FD_ISSET (as_fd, &in_fdset))
	if (ReadASPacket (as_fd, msg.header, &(msg.body)) > 0)
	  {
	    as_msg_handler (msg.header[1], msg.body);
	    free (msg.body);
	  }
    }
  return 0;
}
