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

#define LOCAL_DEBUG
#include "../configure.h"

#include "asapp.h"

#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/un.h>

#include "afterstep.h"
#include "mystyle.h"
#include "screen.h"
#include "module.h"
#include "wmprops.h"
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
        if( event->error_code == BadWindow && Scr.Windows != NULL )
            if( Scr.on_dead_window )
                if( Scr.on_dead_window( event->resourceid ) )
                    return 0;

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
ConnectX (ScreenInfo * scr, unsigned long event_mask)
{
    /* Initialize X connection */
    if (!(dpy = XOpenDisplay (MyArgs.display_name)))
	{
        show_error("Can't open display %s. Exiting!", XDisplayName (MyArgs.display_name));
		exit (1);
	}

	x_fd = XConnectionNumber (dpy);

    if( x_fd < 0 )
    {
        show_error("failed to initialize X Windows session. Aborting!");
        exit(1);
    }
    if (fcntl (x_fd, F_SETFD, 1) == -1)
        show_warning ("close-on-exec for X Windows connection failed");

	XSetErrorHandler (ASErrorHandler);

    if( get_flags(MyArgs.flags, ASS_Debugging) )
        set_synchronous_mode (True);

    intern_hint_atoms ();
    intern_wmprop_atoms ();

	memset (scr, 0x00, sizeof (ScreenInfo));

	scr->screen = DefaultScreen (dpy);
	scr->Root = RootWindow (dpy, scr->screen);
	if (scr->Root == None)
	{
        show_error("Screen %d is not valid. Exiting.", (int)scr->screen);
		exit (1);
	}

    scr->NumberOfScreens = NumberOfScreens = ScreenCount (dpy);
	scr->MyDisplayWidth = DisplayWidth (dpy, scr->screen);
	scr->MyDisplayHeight = DisplayHeight (dpy, scr->screen);
	scr->CurrentDesk = -1;

    scr->RootClipArea.width = scr->MyDisplayWidth;
    scr->RootClipArea.height = scr->MyDisplayHeight;

    if( (scr->wmprops = setup_wmprops( scr, (event_mask&SubstructureRedirectMask), 0xFFFFFFFF, NULL )) == NULL )
        return -1;

    scr->asv = create_asvisual (dpy, scr->screen, scr->d_depth, NULL);
    scr->d_depth = scr->asv->visual_info.depth;

    scr->last_Timestamp = CurrentTime ;
    scr->menu_grab_Timestamp = CurrentTime ;

    setup_modifiers ();

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

/* Setting up global variables  nonlock_mods, and lock_mods, defined in asapp.c : */
void
setup_modifiers ()
{
	int           m, i, knl;
	char         *kn;
	KeySym        ks;
	KeyCode       kc, *kp;
	unsigned      lockmask, *mp;
	XModifierKeymap *mm = XGetModifierMapping (dpy);

	lockmask = LockMask;
	if (mm)
	{
		kp = mm->modifiermap;
		for (m = 0; m < 8; m++)
		{
			for (i = 0; i < mm->max_keypermod; i++)
			{
				if ((kc = *kp++) && ((ks = XKeycodeToKeysym (dpy, kc, 0)) != NoSymbol))
				{
					kn = XKeysymToString (ks);
					knl = strlen (kn);
					if ((knl > 6) && (mystrcasecmp (kn + knl - 4, "lock") == 0))
						lockmask |= (1 << m);
				}
			}
		}
		XFreeModifiermap (mm);
	}
/* forget shift & control locks */
	lockmask &= ~(ShiftMask | ControlMask);

	nonlock_mods = ((ShiftMask | ControlMask | Mod1Mask | Mod2Mask
	 				 | Mod3Mask | Mod4Mask | Mod5Mask) & ~lockmask);

	mp = lock_mods;
	for (m = 0, i = 1; i < MAX_LOCK_MODS; i++)
	{
		if ((i & lockmask) > m)
			m = *mp++ = (i & lockmask);
	}
	*mp = 0;
}

/****************************************************************************
 * Pan Frames : windows for edge-scrolling
 * the root window is surrounded by four window slices, which are InputOnly.
 * So you can see 'through' them, but they eat the input. An EnterEvent in
 * one of these windows causes a Paging. The windows have the according cursor
 * pointing in the pan direction or are hidden if there is no more panning
 * in that direction. This is mostly intended to get a panning even atop
 * of Motif applictions, which does not work yet. It seems Motif windows
 * eat all mouse events.
 *
 * Hermann Dunkel, HEDU, dunkel@cul-ipn.uni-kiel.de 1/94
 ****************************************************************************/
void
init_screen_panframes(ScreenInfo *scr)
{
#ifndef NO_VIRTUAL
    XRectangle  frame_rects[PAN_FRAME_SIDES] = PAN_FRAME_PLACEMENT ;

    XSetWindowAttributes attributes;           /* attributes for create */
    register int i ;

	frame_rects[2].width = frame_rects[0].width = scr->MyDisplayWidth ;
    frame_rects[1].x = scr->MyDisplayWidth - SCROLL_REGION ;
	frame_rects[3].height = frame_rects[1].height = scr->MyDisplayHeight - (SCROLL_REGION*2) ;


    attributes.event_mask = AS_PANFRAME_EVENT_MASK;
    for( i = 0 ; i < PAN_FRAME_SIDES ; i++ )
    {
        scr->PanFrame[i].win =
			create_screen_window( scr, None,
								  frame_rects[i].x, frame_rects[i].y,
              		              frame_rects[i].width, frame_rects[i].height, 0, /* no border */
	                              InputOnly, (CWEventMask), &attributes);
        LOCAL_DEBUG_OUT( "creating PanFrame %d(%lX) at %dx%d%+d%+d", i, scr->PanFrame[i].win,
                         frame_rects[i].width, frame_rects[i].height, frame_rects[i].x, frame_rects[i].y );
        scr->PanFrame[i].isMapped = False ;
    }
#endif /* NO_VIRTUAL */
}

/***************************************************************************
 * checkPanFrames hides PanFrames if they are on the very border of the
 * VIRTUELL screen and EdgeWrap for that direction is off.
 * (A special cursor for the EdgeWrap border could be nice) HEDU
 ****************************************************************************/
void
check_screen_panframes(ScreenInfo *scr)
{
#ifndef NO_VIRTUAL
    int           wrapX ;
    int           wrapY ;
    Bool          map_frame[PAN_FRAME_SIDES] = {False, False, False, False};
    register int  i;

	if( scr == NULL )
		scr = &Scr ;

    wrapX = get_flags(scr->Feel.flags, EdgeWrapX);
    wrapY = get_flags(scr->Feel.flags, EdgeWrapY);

    if( get_flags(scr->Feel.flags, DoHandlePageing) )
    {
        if (scr->Feel.EdgeScrollY > 0)
        {
            if (scr->Vy > 0 || wrapY )
                map_frame[FR_N] = True ;
            if (scr->Vy < scr->VyMax || wrapY )
                map_frame[FR_S] = True ;
        }
        if (scr->Feel.EdgeScrollX > 0 )
        {
            if (scr->Vx < scr->VxMax || wrapX )
                map_frame[FR_E] = True ;
            if (scr->Vx > 0 || wrapX )
                map_frame[FR_W] = True ;
        }
    }
    /* Remove Pan frames if paging by edge-scroll is permanently or
	 * temporarily disabled */
    for( i = 0 ; i < PAN_FRAME_SIDES ; i++ )
    {
        if( scr->PanFrame[i].win )
        {
            if( map_frame[i] != scr->PanFrame[i].isMapped )
            {
                if( map_frame[i] )
                {
                    LOCAL_DEBUG_OUT( "mapping PanFrame %d(%lX)", i, scr->PanFrame[i].win );
                    XMapRaised (dpy, scr->PanFrame[i].win);
                }else
                    XUnmapWindow (dpy, scr->PanFrame[i].win);
                scr->PanFrame[i].isMapped = map_frame[i];
            }

            if( map_frame[i] )
            {
                /* to maintain stacking order where first mapped pan frame is the lowest window :*/
                LOCAL_DEBUG_OUT( "rasing PanFrame %d(%lX)", i, scr->PanFrame[i].win );
                XRaiseWindow( dpy, scr->PanFrame[i].win );
                XDefineCursor (dpy, scr->PanFrame[i].win, scr->Feel.cursors[TOP+i]);
            }
        }
    }
#endif
}

/****************************************************************************
 * Gotta make sure these things are on top of everything else, or they
 * don't work!
 ***************************************************************************/
void
raise_scren_panframes (ScreenInfo *scr)
{
#ifndef NO_VIRTUAL
    register int i ;
	if( scr == NULL )
		scr = &Scr ;
    for( i = 0 ; i < PAN_FRAME_SIDES ; i++ )
        if( scr->PanFrame[i].isMapped )
        {
            LOCAL_DEBUG_OUT( "rasing PanFrame %d(%lX)", i, scr->PanFrame[i].win );
            XRaiseWindow (dpy, scr->PanFrame[i].win);
        }
#endif
}

Window
get_lowest_panframe(ScreenInfo *scr)
{
    register int i;
    for( i = 0 ; i < PAN_FRAME_SIDES ; i++ )
        if( scr->PanFrame[i].isMapped )
            return scr->PanFrame[i].win;
    return None;
}

/* utility function for geometry merging : */
void merge_geometry( ASGeometry *from, ASGeometry *to )
{
    if ( get_flags(from->flags, WidthValue) )
        to->width = from->width ;
    if ( get_flags(from->flags, HeightValue) )
        to->height = from->height ;
    if ( get_flags(from->flags, XValue) )
    {
        to->x = from->x ;
        if( !get_flags(from->flags, XNegative) )
            clear_flags(to->flags, XNegative);
    }
    if ( get_flags(from->flags, YValue) )
    {
        to->y = from->y ;
        if( !get_flags(from->flags, YNegative) )
            clear_flags(to->flags, YNegative);
    }
    to->flags |= from->flags ;
}

