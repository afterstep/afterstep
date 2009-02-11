/*
 * Copyright (c) 2001,2000,1999 Sasha Vasko <sasha at aftercode.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>

#include "xwrap.h"
#include "audit.h"
#include "output.h"
#include "parse.h"

static Display *asbase_current_dpy = NULL;
static int DisplayGrabCount = 0 ;

void set_current_X_display (Display *dpy)
{
	asbase_current_dpy = dpy;
}

Display *
get_current_X_display ()
{
#ifndef X_DISPLAY_MISSING
	return asbase_current_dpy;
#else
	return NULL;
#endif
}

#ifndef X_DISPLAY_MISSING
static int
quiet_xerror_handler (Display * dpy, XErrorEvent * error)
{
    return 0;
}

#endif

int 
grab_server()
{
#ifndef X_DISPLAY_MISSING
	if( asbase_current_dpy )
	{
		XGrabServer( asbase_current_dpy );
		++DisplayGrabCount ;
	}
#endif
	return DisplayGrabCount;	
}

int 
ungrab_server()
{
#ifndef X_DISPLAY_MISSING
	if( asbase_current_dpy )
	{
		XUngrabServer( asbase_current_dpy );
		--DisplayGrabCount ;
	}
#endif
	return DisplayGrabCount;	
}

Bool
is_server_grabbed()
{
	return (DisplayGrabCount > 0);
}

int
get_drawable_size (Drawable d, unsigned int *ret_w, unsigned int *ret_h)
{
	int result = 0 ;
#ifndef X_DISPLAY_MISSING
	if( d != None && asbase_current_dpy) 
	{
		Window        root;
		unsigned int  ujunk;
		int           junk;
		int           (*oldXErrorHandler) (Display *, XErrorEvent *) = XSetErrorHandler (quiet_xerror_handler);
		result = XGetGeometry (asbase_current_dpy, d, &root, &junk, &junk, ret_w, ret_h, &ujunk, &ujunk);
		XSetErrorHandler (oldXErrorHandler);
	}
#endif
	if ( result == 0)
	{
		*ret_w = 0;
		*ret_h = 0;
		return 0;
	}
	return 1;
}

Drawable
validate_drawable (Drawable d, unsigned int *pwidth, unsigned int *pheight)
{
#ifndef X_DISPLAY_MISSING
	int           (*oldXErrorHandler) (Display *, XErrorEvent *) = NULL;

	/* we need to check if pixmap is still valid */
	Window        root;
	int           junk;
	unsigned int  ujunk;

	oldXErrorHandler = XSetErrorHandler (quiet_xerror_handler);

	if (!pwidth)
		pwidth = &ujunk;
	if (!pheight)
		pheight = &ujunk;

	if (d != None && asbase_current_dpy)
	{
		if (!XGetGeometry (asbase_current_dpy, d, &root, &junk, &junk, pwidth, pheight, &ujunk, &ujunk))
			d = None;
	}
	XSetErrorHandler (oldXErrorHandler);

	return d;
#else
	return None ;
#endif
}

void
backtrace_window ( const char *file, int line, Window w )
{
#ifndef X_DISPLAY_MISSING
    Window        root, parent, *children = NULL;
	unsigned int  nchildren;
    int           (*oldXErrorHandler) (Display *, XErrorEvent *) = NULL;

    oldXErrorHandler = XSetErrorHandler (quiet_xerror_handler);
    fprintf (stderr, "%s(line%d): Backtracing [%lX]", file, line, w);
	while (XQueryTree (asbase_current_dpy, w, &root, &parent, &children, &nchildren))
	{
		int x, y ;
		unsigned int width, height, border, depth ;
		XGetGeometry( asbase_current_dpy, w, &root, &x, &y, &width, &height, &border, &depth );
	    fprintf (stderr, "(%dx%d%+d%+d)", width, height, x, y );

		if (children)
			XFree (children);
        children = NULL ;
		w = parent;
        fprintf (stderr, "->[%lX] ", w);
		if( w == None )
			break;
	}
    XSetErrorHandler (oldXErrorHandler);
    fprintf (stderr, "\n");
#endif
}

Window
get_parent_window( Window w )
{
	Window parent = None ;
#ifndef X_DISPLAY_MISSING
    Window        root, *children = NULL;
    unsigned int  child_count = 0;

	XSync( asbase_current_dpy, False );
    XQueryTree (asbase_current_dpy, w, &root, &parent, &children, &child_count);
    if (children)
        XFree (children);
#endif
	return parent ;
}

Window
get_topmost_parent( Window w, Window *desktop_w )
{
	Window desktop = w ;
#ifndef X_DISPLAY_MISSING
    Window  root = None, parent = w;
	Window *children = NULL;
    unsigned int  child_count;

	XSync( asbase_current_dpy, False );
	while( w != root && w != None )
	{
		parent = desktop ;
		desktop = w ;
		XQueryTree (asbase_current_dpy, w, &root, &w, &children, &child_count);
    	if (children)
        	XFree (children);
	}
#endif
	if( desktop_w )
		*desktop_w = desktop ;
	return w ;
}

#ifdef X_DISPLAY_MISSING
int XParseGeometry (  char *string,int *x,int *y,
                      unsigned int *width,    /* RETURN */
					  unsigned int *height)    /* RETURN */
{
	int flags = 0 ;
	parse_geometry( string, x, y, width, height, &flags );
	return flags ;
}

void XDestroyImage( void* d){}
int XGetWindowAttributes( void*d, Window w, unsigned long m, void* s){  return 0;}
void *XGetImage( void* dpy,Drawable d,int x,int y,unsigned int width,unsigned int height, unsigned long m,int t)
{return NULL ;}
unsigned long XGetPixel(void* d, int x, int y){return 0;}
int XQueryColors(void* a,Colormap c,void* x,int m){return 0;}
#endif


