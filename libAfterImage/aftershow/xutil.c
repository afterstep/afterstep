/*
 * Copyright (c) 2008 Sasha Vasko <sasha at aftercode.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License.
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

/*********************************************************************************
 * Utility functions to handle X stuff
 *********************************************************************************/

#ifdef _WIN32
#include "../win32/config.h"
#else
#include "../config.h"
#endif

#ifndef X_DISPLAY_MISSING

#undef LOCAL_DEBUG

#include <string.h>

#include "../afterbase.h"
#include "../afterimage.h"
#include "aftershow.h"

Bool
aftershow_connect_x_gui(AfterShowContext *ctx)
{
	if (!ctx->gui.x.valid)
	{
		int i;
		AfterShowXScreen *scr;
		ctx->gui.x.dpy = XOpenDisplay(ctx->display);
		if (ctx->gui.x.dpy != NULL)
		{
			ctx->gui.x.fd = XConnectionNumber (ctx->gui.x.dpy);
			ctx->gui.x.screens_num = get_flags(ctx->flags, AfterShow_SingleScreen)?1:ScreenCount (ctx->gui.x.dpy);	
			ctx->gui.x.screens = scr = safecalloc(ctx->gui.x.screens_num, sizeof(AfterShowXScreen));

			for (i = 0; i < ctx->gui.x.screens_num; ++i)
			{
				scr->screen = i;
				scr->root = RootWindow(ctx->gui.x.dpy, scr->screen);
				scr->root_width = DisplayWidth (ctx->gui.x.dpy, scr->screen);
				scr->root_height = DisplayHeight (ctx->gui.x.dpy, scr->screen);
				++scr;
			}
			ctx->gui.x.first_screen = 0;
			ctx->gui.x.valid = True;
			show_progress( "X display \"%s\" connected. Servicing %d screens.", ctx->display?ctx->display:"", ctx->gui.x.screens_num);
		}else
		{
			if (ctx->display)
	        	show_error("failed to initialize X Window session for display \"%s\"", ctx->display);
			else
				show_error("failed to initialize X Window session for default display");
		}
	}

	return ctx->gui.x.valid;
}

Bool
aftershow_get_drawable_size_and_depth (AfterShowContext *ctx, Drawable d, int *width_return, int *height_return, int *depth_return)
{
	Window root;
    int junk;
	unsigned int ujunk, width, height, depth;

    if (d != None)
  		if (!XGetGeometry (ctx->gui.x.dpy, d, &root, &junk, &junk, &width, &height, &ujunk, &depth))
			return False;	

	if (width_return)
		*width_return = width;
	if (height_return)
		*height_return = height;
	if (depth_return)
		*depth_return = depth;
	return True;
}

static int
nop_error_handler (Display * dpy, XErrorEvent * error)
{
	return 0;
}


Bool
aftershow_validate_drawable (AfterShowContext *ctx, Drawable d)
{
	Window root;
    int junk;
	unsigned int ujunk ;
	int (*oldXErrorHandler) (Display *, XErrorEvent *) = XSetErrorHandler (nop_error_handler);

    if (d != None)
  		if (!XGetGeometry (ctx->gui.x.dpy, d, &root, &junk, &junk, &ujunk, &ujunk, &ujunk, &ujunk))
			d = None;

	XSetErrorHandler (oldXErrorHandler);
	return (d != None);
}


void
aftershow_set_string_property (AfterShowContext *ctx, Window w, Atom property, char *data)
{
    if (w != None && property != None && data)
	{
		LOCAL_DEBUG_OUT( "setting property %lX on %lX to \"%s\"", property, w, data );
        XChangeProperty (ctx->gui.x.dpy, w, property, XA_STRING, 8,
                         PropModeReplace, (unsigned char *)data, strlen (data));
    }
}

char * 
aftershow_read_string_property (AfterShowContext *ctx, Window w, Atom property)
{
	char *res = NULL;
    if (w != None && property != None)
	{
        int           actual_format;
        Atom          actual_type;
        unsigned long junk;
		unsigned char *tmp = NULL;

        if (XGetWindowProperty(ctx->gui.x.dpy, w, property, 0, ~0, False, AnyPropertyType, &actual_type,
             &actual_format, &junk, &junk, &tmp) == Success)
        {
            if (actual_type == XA_STRING && actual_format == 8)
				res = strdup((char*)tmp);
            XFree (tmp);
        }
	}
	return res;
}

#endif 
/*********************************************************************************
 * The end !!!! 																 
 ********************************************************************************/

