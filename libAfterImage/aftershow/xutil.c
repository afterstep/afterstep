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


AfterShowXWindow* 
aftershow_XWindowID2XWindow (AfterShowContext *ctx, Window w)
{
	ASHashData hdata = {0} ;
	int i;
	for (i = 0 ; i < ctx->gui.x.screens_num; ++i)
	{
        if (get_hash_item (ctx->gui.x.screens[i]->windows, AS_HASHABLE(w), &hdata.vptr) == ASH_Success)
		    return hdata.vptr;
	}
}



AfterShowXScreen* 
aftershow_XWindowID2XScreen (AfterShowContext *ctx, Window w)
{
	int i;
	AfterShowXWindow* window = NULL;
	for (i = 0 ; i < ctx->gui.x.screens_num; ++i)
		if (w == ctx->gui.x.screens[i].selection_window)
			return &(ctx->gui.x.screens[i]);

	if ((window = XWindowID2XWindow (ctx, w)) == NULL)
		return NULL;
		
	return window->screen;
}

static void
aftershow_setup_root_x_window (AfterShowContext *ctx, AfterShowXWindow *window)
{
	window->w = RootWindow(ctx->gui.x.dpy, window->screen->screen);
	window->width = DisplayWidth (ctx->gui.x.dpy, window->screen->screen);
	window->height = DisplayHeight (ctx->gui.x.dpy, window->screen->screen);
	window->depth = DefaultDepth(ctx->gui.x.dpy, window->screen->screen);
	
	window->gc = DefaultGC(ctx->gui.x.dpy, window->screen->screen);
	window->back = 

	if (window->back)
		aftershow_get_drawable_size_and_depth (ctx, window->back, &(window->back_width), &(window->back_height), NULL);
}

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
				scr->root.magic = MAGIC_AFTERSHOW_X_WINDOW;
				scr->root.screen = scr;
				aftershow_create_root_x_window (ctx, &(scr->root));
				scr->windows = create_ashash( 0, NULL, NULL, NULL ); 
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


AfterShowXWindow *
aftershow_create_x_window (AfterShowContext *ctx, AfterShowXWindow *parent, int width, int height)
{
	AfterShowXWindow *window = safecalloc( 1, sizeof(AfterShowXWindow));
	AfterShowXScreen *scr = parent->screen;
	unsigned long attr_mask = CWEventMask ;
	XSetWindowAttributes attr;

	window->magic = MAGIC_AFTERSHOW_X_WINDOW;
	window->screen = scr;
	
	attr.event_mask = ExposureMask;
	
	window->w = create_visual_window(scr->asv, parent->w, 0, 0, width, height, 0, 
									 InputOutput, attr_mask, &attr);

	add_hash_item (scr->windows, AS_HASHABLE(window->w), window);

	window->width = width;
	window->height = height;

	window->back = create_visual_pixmap(scr->asv, window->w, width, height, 0);
	window->gc = create_visual_gc(scr->asv, window->w, 0, NULL);	
	window->depth = scr->asv->true_depth;
	window->back_width = width;
	window->back_height = height;
	
	return window;
}

AfterShowXLayer *
aftershow_create_x_layer (AfterShowContext *ctx, AfterShowXWindow *window)
{
	AfterShowXLayer *layer = safecalloc( 1, sizeof(AfterShowXLayer));
	layer->magic = MAGIC_AFTERSHOW_X_LAYER;
	layer->width = window->width;
	layer->height = window->height;
	
	return layer;
}

Bool aftershow_ASImage2XLayer ( AfterShowContext *ctx, AfterShowXWindow *window, 
								AfterShowXLayer *layer, ASImage *im,  int dst_x, int dst_y)
{ 
	Bool success = False;
	AfterShowXScreen *screen = window->screen;
	XImage *xim = create_visual_scratch_ximage( screen->asv, im->width, im->height, window->depth);

	if( subimage2ximage (screen->asv, im, 0, 0, xim)	)
	{
		int dst_width = im->width ;
		int dst_height = im->height ;
		if (layer->pmap == None)
		{
			layer->pmap = create_visual_pixmap(screen->asv, window->w, window->width, window->height, 0);
			layer->pmap_width = window->width ;
			layer->pmap_height = window->height;
		}
		if (dst_x < layer->pmap_width && dst_y < layer->pmap_height)
		{
			if (dst_width+dst_x > layer->pmap_width)
				dst_width = layer->pmap_width - dst_x;
			if (dst_height+dst_y > layer->pmap_height)
				dst_height = layer->pmap_height - dst_y;
			put_ximage( screen->asv, xim, layer->pmap, window->gc, 0, 0, dst_x, dst_y, dst_width, dst_height );	
			success = True;
		}
	}
	XDestroyImage( xim );				  
	return success;
}

void 
aftershow_ExposeXWindowArea (AfterShowContext *ctx, AfterShowXWindow *window, 
						  	 int left, int top, int right, int bottom)
{
	int i;
	
	if (right > window->width)
		right = window->width;
	if (bottom > window->height)
		bottom = window->height;
	if (left >= right || top >= bottom)
		return;
	
	for (i = 0 ; i < window->layers_num ; ++i)
	{
		AfterShowXLayer *layer = &(window->layers[i]);
		if (layer->pmap)
		{
			int lx = (layer->x < left)?left - layer->x:0;
			int ly = (layer->y < top)?top - layer->y:0;
			int lw = layer->x + layer->width - lx - left;
			int lh = layer->y + layer->height - ly - top;
			if (lw > 0 && lh > 0)
			{
				if (left + lw > right)
					lw = right - left;
				if (top + lh > bottom)
					lh = bottom - top;
				
				XCopyArea( ctx->gui.x.dpy, layer->pmap, window->w, window->gc, lx, ly, lw, lh, left, top);
			}
		}
	}

}


#endif 
/*********************************************************************************
 * The end !!!! 																 
 ********************************************************************************/

