/*
 * Copyright (C) 2002 Sasha Vasko <sasha at aftercode.net>
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

#include <signal.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>

#include "../configure.h"
#include "../include/asapp.h"

#include "../include/afterstep.h"
#include "../include/mystyle.h"
#include "../include/screen.h"
#include "../include/module.h"
#include "../libAfterImage/afterimage.h"

#ifdef HAVE_XINERAMA
static int    XineEventBase, XineErrorBase;
#endif
static Bool   as_X_synchronous_mode = False;

void
get_Xinerama_rectangles (ScreenInfo * scr)
{
#ifdef HAVE_XINERAMA
	register int  i;
	XineramaScreenInfo *s;

	if ((s = XineramaQueryScreens (dpy, &(scr->xinerama_screens_num))) != NULL)
	{
		scr->xinerama_screens = safemalloc (sizeof (XRectangle) * scr->xinerama_screens_num);
		for (i = 0; i < scr->xinerama_screens_num; ++i)
		{
			scr->xinerama_screens[i].x = s[i].x_org;
			scr->xinerama_screens[i].y = s[i].y_org;
			scr->xinerama_screens[i].width = s[i].width;
			scr->xinerama_screens[i].height = s[i].height;
		}
		XFree (s);
	}
#else
	scr->xinerama_screens = NULL;
	scr->xinerama_screens_num = 0;
#endif
}

Bool
set_synchronous_mode (Bool enable)
{
	Bool          old = as_X_synchronous_mode;

	XSynchronize (dpy, enable);
	as_X_synchronous_mode = enable;

	return old;
}

Bool
is_synchronous_request (int request_code)
{
	if (as_X_synchronous_mode)
		return True;
	switch (request_code)
	{
	 case X_CreateWindow:
	 case X_GetWindowAttributes:
	 case X_GetGeometry:
	 case X_QueryTree:
	 case X_InternAtom:
	 case X_GetAtomName:
	 case X_GetProperty:
	 case X_ListProperties:
	 case X_GetSelectionOwner:
	 case X_ConvertSelection:
	 case X_QueryPointer:
	 case X_GetMotionEvents:
	 case X_TranslateCoords:
	 case X_GetInputFocus:
	 case X_QueryKeymap:
	 case X_OpenFont:
	 case X_QueryFont:
	 case X_QueryTextExtents:
	 case X_ListFonts:
	 case X_ListFontsWithInfo:
	 case X_GetFontPath:
	 case X_CreatePixmap:
	 case X_CreateGC:
	 case X_GetImage:
	 case X_CreateColormap:
	 case X_CopyColormapAndFree:
	 case X_ListInstalledColormaps:
	 case X_AllocColor:
	 case X_AllocNamedColor:
	 case X_AllocColorCells:
	 case X_AllocColorPlanes:
	 case X_QueryColors:
	 case X_LookupColor:
	 case X_CreateCursor:
	 case X_CreateGlyphCursor:
	 case X_QueryBestSize:
	 case X_QueryExtension:
	 case X_ListExtensions:
	 case X_GetKeyboardMapping:
	 case X_GetKeyboardControl:
	 case X_GetPointerControl:
	 case X_GetScreenSaver:
	 case X_ListHosts:
	 case X_GetPointerMapping:
	 case X_GetModifierMapping:
		 return True;
		 break;
	 default:
		 break;
	}
	return False;
}

int
ASErrorHandler (Display * dpy, XErrorEvent * event)
{
	char         *err_text;

	fprintf (stderr, "%s has encountered the following problem interacting with X Windows :\n", MyName);
	if (event && dpy)
	{
		err_text = safemalloc (128);
		strcpy (err_text, "unknown error");
		XGetErrorText (dpy, event->error_code, err_text, 120);
		fprintf (stderr, "      Request: %d,    Error: %d(%s)\n", event->request_code, event->error_code, err_text);
		free (err_text);
		fprintf (stderr, "      in resource: 0x%lX\n", event->resourceid);
#ifndef __CYGWIN__
		if (is_synchronous_request (event->request_code))
			print_simple_backtrace ();
#endif
	}
	return 0;
}


int
ConnectX (ScreenInfo * scr, char *display_name, unsigned long event_mask)
{
	int           x_fd;

	/* Initialize X connection */
	if (!(dpy = XOpenDisplay (display_name)))
	{
		fprintf (stderr, "%s: can't open display %s", MyName, XDisplayName (display_name));
		exit (1);
	}
	x_fd = XConnectionNumber (dpy);
	XSetErrorHandler (ASErrorHandler);

    if( get_flags(MyArgs.flags, ASS_Debugging) )
        set_synchronous_mode (True);

    intern_hint_atoms ();

	memset (scr, 0x00, sizeof (ScreenInfo));

	scr->screen = DefaultScreen (dpy);
	scr->Root = RootWindow (dpy, scr->screen);
	if (scr->Root == None)
	{
		fprintf (stderr, "%s: Screen %d is not valid ", MyName, (int)scr->screen);
		exit (1);
	}

    scr->NumberOfScreens = ScreenCount (dpy);
	scr->MyDisplayWidth = DisplayWidth (dpy, scr->screen);
	scr->MyDisplayHeight = DisplayHeight (dpy, scr->screen);
	scr->CurrentDesk = -1;

    if( (scr->wmprops = setup_wmprops( scr, (event_mask&SubstructureRedirectMask), 0xFFFFFFFF, NULL )) == NULL )
        return -1;

    scr->asv = create_asvisual (dpy, scr->screen, scr->d_depth, NULL);
    scr->d_depth = scr->asv->visual_info.depth;

    scr->last_Timestamp = CurrentTime ;
    scr->menu_grab_Timestamp = CurrentTime ;


#ifdef HAVE_XINERAMA
	if (XineramaQueryExtension (dpy, &XineEventBase, &XineErrorBase))
		get_Xinerama_rectangles (scr);
#endif

    XSelectInput (dpy, scr->Root, event_mask);
	return x_fd;
}

/***********************************************************************
 *  Procedure:
 *  SetupEnvironment - setup environment variables DISPLAY and HOSTDISPLAY
 *  for our children to use.
 ************************************************************************/
void
make_screen_envvars( ScreenInfo *scr )
{
    int           len ;
    char         *display = ":0.0";
    char          client[MAXHOSTNAME];
/* We set environmanet vars and we keep memory used for those allocated
 * as some OS's don't copy the environment variables properly
 */
	if( scr == NULL )
		return;

	if( scr->display_string )
        free( scr->display_string );
    if( scr->rdisplay_string )
        free( scr->rdisplay_string );

    /*  Add a DISPLAY entry to the environment, incase we were started
	 * with afterstep -display term:0.0
	 */
	display = XDisplayString (dpy);
    len = strlen (display);
    scr->display_string = safemalloc (len + 10);
    sprintf (scr->display_string, "DISPLAY=%s", display);

	/* Add a HOSTDISPLAY environment variable, which is the same as
	 * DISPLAY, unless display = :0.0 or unix:0.0, in which case the full
	 * host name will be used for ease in networking . */
    if ( scr->display_string[8] == ':' ||
        strncmp (&(scr->display_string[8]), "unix:", 5) == 0)
	{
        register char * ptr = &(scr->display_string[8]);
        if( *ptr != ':' ) ptr += 5 ;
		mygethostname (client, MAXHOSTNAME);
		scr->localhost = True ;
		scr->rdisplay_string = safemalloc (len + 14 + strlen (client));
        sprintf (scr->rdisplay_string, "HOSTDISPLAY=%s:%s", client, ptr);
    }else
    {   /* X Server is likely to be runnig on other computer: */
		scr->localhost = False ;
		scr->rdisplay_string = safemalloc (len + 14);
        sprintf (scr->rdisplay_string, "HOSTDISPLAY=%s", display);
	}
}

void
init_screen_gcs(ScreenInfo *scr)
{
	if( scr )
	{
		XGCValues     gcv;
    	unsigned long gcm = GCGraphicsExposures;
		gcv.graphics_exposures = False;
        scr->DrawGC = create_visual_gc( scr->asv, scr->Root, gcm, &gcv );
    }
}

