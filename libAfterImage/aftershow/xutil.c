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

#include "../afterbase.h"
#include "../afterimage.h"
#include "aftershow.h"

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

#endif 
/*********************************************************************************
 * The end !!!! 																 
 ********************************************************************************/

