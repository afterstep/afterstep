/*
 * Copyright (C) 1999 Sasha Vasko <sashav@sprintmail.com>
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

#if !defined(DEBUG_ALLOCS) && defined(DEBUG_TRACE)

#include <string.h>							   /* for memset */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "trace.h"				   

#if defined(TRACE_XNextEvent) || defined(TRACE_DispatchEvent)
typedef struct EventDesc
{
	char         *name;
	unsigned long mask;
}
EventDesc;

EventDesc     event_types[] = {
	{"nothing", 0},
	{"nothing", 0},
	{"KeyPress", KeyPressMask},
	{"KeyRelease", KeyReleaseMask},
	{"ButtonPress", ButtonPressMask},
	{"ButtonRelease", ButtonReleaseMask},
	{"MotionNotify", PointerMotionMask},
	{"EnterNotify", EnterWindowMask},
	{"LeaveNotify", LeaveWindowMask},
	{"FocusIn", FocusChangeMask},
	{"FocusOut", FocusChangeMask},
	{"KeymapNotify", KeymapStateMask},
	{"Expose", ExposureMask},
	{"GraphicsExpose", ExposureMask},
	{"NoExpose", ExposureMask},
	{"VisibilityNotify", VisibilityChangeMask},
	{"CreateNotify", SubstructureNotifyMask},
	{"DestroyNotify", SubstructureNotifyMask},
	{"UnmapNotify", SubstructureNotifyMask},
	{"MapNotify", SubstructureNotifyMask},
	{"MapRequest", SubstructureRedirectMask},
	{"ReparentNotify", SubstructureNotifyMask},
	{"ConfigureNotify", SubstructureNotifyMask},
	{"ConfigureRequest", SubstructureRedirectMask},
	{"GravityNotify", SubstructureNotifyMask},
	{"ResizeRequest", ResizeRedirectMask},
	{"CirculateNotify", SubstructureNotifyMask},
	{"CirculateRequest", SubstructureRedirectMask},
	{"PropertyNotify", PropertyChangeMask},
	{"SelectionClear", SelectionMask},
	{"SelectionRequest", SelectionMask},
	{"SelectionNotify", SelectionMask},
	{"ColormapNotify", ColormapChangeMask},
	{"ClientMessage", ClientMask},
	{"MappingNotify", MappingMask}
};

int
event_traceble (XEvent * e)
{
	if (e)
		if (e->type >= KeyPress && e->type < LASTEvent)
			if (event_types[e->type].mask & (EVENT_TRACE_MASK))
				return 1;
	return 0;
}

void
printf_xevent (XEvent * e)
{
	if (e)
	{
		if (e->type >= KeyPress && e->type < LASTEvent)
		{
			fprintf (stderr, "<%s>,%s,0x%lX,...", event_types[e->type].name,
					 (e->xany.send_event) ? "SendEvent" : "NotSendEvent", e->xany.window);
		} else
		{
			fprintf (stderr, "unknown event type %d", e->type);
		}
	}
}

#endif /*TRACE_XNextEvent */

#define BOOL2STRING(b)   ((b)?"True":"False")

/* Xlib calls */
#ifdef TRACE_XDestroyWindow
#undef XDestroyWindow
Status
trace_XDestroyWindow (Display * dpy, Window w, const char *filename, int line)
{
	Status        ret;

	fprintf (stderr, "X>%s(%d):XDestroyWindow(0x%lX)\n", filename, line, w);

	ret = XDestroyWindow (dpy, w);

	fprintf (stderr, "X<%s(%d):XDestroyWindow(0x%lX) returned %d\n", filename, line, w, ret);

	return ret;
}
#endif

#ifdef TRACE_XGetGeometry
#undef XGetGeometry
Status
trace_XGetGeometry (Display * dpy, Drawable d,
					Window * root_return,
					int *x_return, int *y_return,
					unsigned int *width_return, unsigned int *height_return,
					unsigned int *border_width_return, unsigned int *depth_return, const char *filename, int line)
{
	Status        ret;

	fprintf (stderr, "X>%s(%d):XGetGeometry(0x%lX)\n", filename, line, d);

	ret = XGetGeometry (dpy, d,
						root_return,
						x_return, y_return, width_return, height_return, border_width_return, depth_return);

	fprintf (stderr,
			 "X<%s(%d):XGetGeometry(0x%lX) returned(0x%lX,%d,%d,%u,%u,%u,%u)  Status=%d\n",
			 filename, line, d, (root_return) ? *root_return : 0,
			 (x_return) ? *x_return : 0, (y_return) ? *y_return : 0,
			 (width_return) ? *width_return : 0,
			 (height_return) ? *height_return : 0,
			 (border_width_return) ? *border_width_return : 0, (depth_return) ? *depth_return : 0, ret);
	return ret;
}
#endif

#ifdef TRACE_XNextEvent
#undef XCheckMaskEvent
Bool
trace_XCheckMaskEvent (Display * dpy, long mask, XEvent * event, const char *file, int line)
{
	int           res;

	res = XCheckMaskEvent (dpy, mask, event);
	if (res && event_traceble (event))
	{
		fprintf (stderr, "X<%s(%d):XCheckMaskEvent(0x%lX,0x%lX) returned {", file, line, mask, (long)event);
		printf_xevent (event);
		fprintf (stderr, "} Status=%d\n", res);
	}
	return res;
}

#undef XCheckTypedEvent
Bool
trace_XCheckTypedEvent (Display * dpy, int event_type, XEvent * event_return, const char *file, int line)
{
	int           res;

	res = XCheckTypedEvent (dpy, event_type, event_return);
	if (res && event_traceble (event_return))
	{
		fprintf (stderr, "X<%s(%d):XCheckTypedEvent(%d,0x%lX) returned {", file, line, event_type, (long)event_return);
		printf_xevent (event_return);
		fprintf (stderr, "} Status=%d\n", res);
	}
	return res;
}

#undef XCheckTypedWindowEvent
Bool
trace_XCheckTypedWindowEvent (Display * dpy, Window w, int event_type,
							  XEvent * event_return, const char *file, int line)
{
	int           res;

	res = XCheckTypedWindowEvent (dpy, w, event_type, event_return);
	if (res && event_traceble (event_return))
	{
		fprintf (stderr,
				 "X<%s(%d):XCheckTypedWindowEvent(0x%lX,%d,0x%lX) returned {",
				 file, line, w, event_type, (long)event_return);
		printf_xevent (event_return);
		fprintf (stderr, "} Status=%d\n", res);
	}
	return res;

}

#undef XCheckWindowEvent
Bool
trace_XCheckWindowEvent (Display * dpy, Window w, long event_mask, XEvent * event_return, const char *file, int line)
{
	int           res;

	res = XCheckWindowEvent (dpy, w, event_mask, event_return);
	if (res && event_traceble (event_return))
	{
		fprintf (stderr,
				 "X<%s(%d):XCheckWindowEvent(0x%lX,0x%lX,0x%lX) returned {",
				 file, line, w, event_mask, (long)event_return);
		printf_xevent (event_return);
		fprintf (stderr, "} Status=%d\n", res);
	}
	return res;
}

#undef XNextEvent
int
trace_XNextEvent (Display * dpy, XEvent * event, const char *file, int line)
{
	int           res;

	res = XNextEvent (dpy, event);
	if (event_traceble (event))
	{
		fprintf (stderr, "X<%s(%d):XNextEvent(0x%lX) returned {", file, line, (long)event);
		printf_xevent (event);
		fprintf (stderr, "} Status=%d\n", res);
	}
	return res;
}

#undef XPeekEvent
int
trace_XPeekEvent (Display * dpy, XEvent * event_return, const char *file, int line)
{
	int           res;

	res = XPeekEvent (dpy, event_return);
	if (event_traceble (event_return))
	{
		fprintf (stderr, "X<%s(%d):XPeekEvent(0x%lX) returned {", file, line, (long)event_return);
		printf_xevent (event_return);
		fprintf (stderr, "} Status=%d\n", res);
	}
	return res;
}

#undef XMaskEvent
int
trace_XMaskEvent (Display * dpy, long event_mask, XEvent * event_return, const char *file, int line)
{
	int           res;

	res = XMaskEvent (dpy, event_mask, event_return);
	if (event_traceble (event_return))
	{
		fprintf (stderr, "X<%s(%d):XMaskEvent(0x%lX,0x%lX) returned {", file, line, event_mask, (long)event_return);
		printf_xevent (event_return);
		fprintf (stderr, "} Status=%d\n", res);
	}
	return res;
}

#undef XPutBackEvent
int
trace_XPutBackEvent (Display * dpy, XEvent * event, const char *file, int line)
{
	int           res;

	res = XPutBackEvent (dpy, event);
	if (event_traceble (event))
	{
		fprintf (stderr, "X<%s(%d):XPutBackEvent(0x%lX) returned {", file, line, (long)event);
		printf_xevent (event);
		fprintf (stderr, "} Status=%d\n", res);
	}
	return res;
}

#undef XWindowEvent
int
trace_XWindowEvent (Display * dpy, Window w, long event_mask, XEvent * event_return, const char *file, int line)
{
	int           res;

	res = XWindowEvent (dpy, w, event_mask, event_return);
	if (event_traceble (event_return))
	{
		fprintf (stderr,
				 "X<%s(%d):XWindowEvent(0x%lX,0x%lX,0x%lX) returned {", file, line, w, event_mask, (long)event_return);
		printf_xevent (event_return);
		fprintf (stderr, "} Status=%d\n", res);
	}
	return res;
}


#endif

#ifdef TRACE_mystrdup
#undef mystrdup
extern char  *mystrdup (const char *str);
char         *
trace_mystrdup (const char *str, const char *file, int line)
{
	char         *strres;

	fprintf (stderr, "L>%s(%d):mystrdup(0x%lX{\"%s\"})\n", file, line, (unsigned long)str, str);
	strres = mystrdup (str);
	fprintf (stderr,
			 "L<%s(%d):mystrdup(0x%lX{\"%s\"}) returned: 0x%lX{\"%s\"}.\n",
			 file, line, (unsigned long)str, str, (unsigned long)strres, strres);
	return strres;
}

#undef mystrndup
extern char  *mystrndup (const char *str, size_t n);
char         *
trace_mystrndup (const char *str, size_t n, const char *file, int line)
{
	char         *strres;

	fprintf (stderr, "L>%s(%d):mystrndup(0x%lX{\"%s\"},%d)\n", file, line, (unsigned long)str, str, n);
	strres = mystrndup (str, n);
	fprintf (stderr,
			 "L<%s(%d):mystrndup(0x%lX{\"%s\"},%d) returned: 0x%lX{\"%s\"}.\n",
			 file, line, (unsigned long)str, str, n, (unsigned long)strres, strres);
	return strres;
}
#endif
#endif /* DEBUG_ALLOCS */

#ifdef DEBUG_TRACE_X

#include <string.h>							   /* for memset */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "trace.h"				   

#undef XRaiseWindow
#undef XLowerWindow
#undef XRestackWindows
#undef XResizeWindow
#undef XMoveResizeWindow
#undef XMoveWindow

#define trace_enabled_func(a) (trace_enabled_funcs & (1 << a))

/* The order of the following enum and trace_func array must be exactly the
** same, and the enum must not have defined values in it!  The functions
** below require this to be true.
*/
enum
{
	TRACE_XRAISEWINDOW,
	TRACE_XLOWERWINDOW,
	TRACE_XRESTACKWINDOWS,
	TRACE_XRESIZEWINDOW,
	TRACE_XMOVERESIZEWINDOW,
	TRACE_XMOVEWINDOW,
};
static const char *trace_func[] = {
	"XRaiseWindow",
	"XLowerWindow",
	"XRestackWindows",
	"XResizeWindow",
	"XMoveResizeWindow",
	"XMoveWindow",
};

static unsigned int trace_enabled_funcs = 0;
const char   *(*trace_window_id2name_hook) (Display * dpy, Window w, int *how_to_free) = NULL;
static const char *trace_window_id2name (Display * dpy, Window w, int *how_to_free);
static const char *trace_window_id2name_internal (Display * dpy, Window w, int *how_to_free);
void
trace_XRaiseWindow (const char *file, const char *func, int line, Display * dpy, Window w)
{
	int           fn = TRACE_XRAISEWINDOW;

	if (trace_enabled_func (fn))
	{
		const char   *str;
		int           how_to_free = 0;

		str = trace_window_id2name (dpy, w, &how_to_free);
		fprintf (stderr, "%s:%s:%d: %s(", file, func, line, trace_func[fn]);
		fprintf (stderr, "%08x%s%s)\n", (unsigned int)w, str ? " => " : "", str ? str : "");
		if (how_to_free == 1)
			free ((char *)str);
		else if (how_to_free == 2)
			XFree ((char *)str);
	}
	XRaiseWindow (dpy, w);
}

void
trace_XLowerWindow (const char *file, const char *func, int line, Display * dpy, Window w)
{
	int           fn = TRACE_XLOWERWINDOW;

	if (trace_enabled_func (fn))
	{
		const char   *str;
		int           how_to_free = 0;

		str = trace_window_id2name (dpy, w, &how_to_free);
		fprintf (stderr, "%s:%s:%d: %s(", file, func, line, trace_func[fn]);
		fprintf (stderr, "%08x%s%s)\n", (unsigned int)w, str ? " => " : "", str ? str : "");
		if (how_to_free == 1)
			free ((char *)str);
		else if (how_to_free == 2)
			XFree ((char *)str);
	}
	XLowerWindow (dpy, w);
}

void
trace_XRestackWindows (const char *file, const char *func, int line, Display * dpy, Window windows[], int nwindows)
{
	int           fn = TRACE_XRESTACKWINDOWS;

	if (trace_enabled_func (fn))
	{
		int           i;

		fprintf (stderr, "%s:%s:%d: %s(\n", file, func, line, trace_func[fn]);
		for (i = 0; i < nwindows; i++)
		{
			const char   *str;
			int           how_to_free = 0;

			str = trace_window_id2name (dpy, windows[i], &how_to_free);
			if (i)
				fprintf (stderr, ",\n");
			fprintf (stderr, "%08x%s%s", (unsigned int)windows[i], str ? " => " : "", str ? str : "");
			if (how_to_free == 1)
				free ((char *)str);
			else if (how_to_free == 2)
				XFree ((char *)str);
		}
		fprintf (stderr, ")\n");
	}
	XRestackWindows (dpy, windows, nwindows);
}

void
trace_XResizeWindow (const char *file, const char *func, int line, Display * dpy, Window w, int width, int height)
{
	int           fn = TRACE_XRESIZEWINDOW;

	if (trace_enabled_func (fn))
	{
		const char   *str;
		int           how_to_free = 0;

		str = trace_window_id2name (dpy, w, &how_to_free);
		fprintf (stderr, "%s:%s:%d: %s(", file, func, line, trace_func[fn]);
		fprintf (stderr, "%08x%s%s, %d, %d)\n", (unsigned int)w, str ? " => " : "", str ? str : "", width, height);
		if (how_to_free == 1)
			free ((char *)str);
		else if (how_to_free == 2)
			XFree ((char *)str);
	}
	XResizeWindow (dpy, w, width, height);
}

void
trace_XMoveWindow (const char *file, const char *func, int line, Display * dpy, Window w, int x, int y)
{
	int           fn = TRACE_XMOVEWINDOW;

	if (trace_enabled_func (fn))
	{
		const char   *str;
		int           how_to_free = 0;

		str = trace_window_id2name (dpy, w, &how_to_free);
		fprintf (stderr, "%s:%s:%d: %s(", file, func, line, trace_func[fn]);
		fprintf (stderr, "%08x%s%s, %d, %d)\n", (unsigned int)w, str ? " => " : "", str ? str : "", x, y);
		if (how_to_free == 1)
			free ((char *)str);
		else if (how_to_free == 2)
			XFree ((char *)str);
	}
	XMoveWindow (dpy, w, x, y);
}

void
trace_XMoveResizeWindow (const char *file, const char *func, int line,
						 Display * dpy, Window w, int x, int y, int width, int height)
{
	int           fn = TRACE_XMOVERESIZEWINDOW;

	if (trace_enabled_func (fn))
	{
		const char   *str;
		int           how_to_free = 0;

		str = trace_window_id2name (dpy, w, &how_to_free);
		fprintf (stderr, "%s:%s:%d: %s(", file, func, line, trace_func[fn]);
		fprintf (stderr, "%08x%s%s, %d, %d, %d, %d)\n", (unsigned int)w,
				 str ? " => " : "", str ? str : "", x, y, width, height);
		if (how_to_free == 1)
			free ((char *)str);
		else if (how_to_free == 2)
			XFree ((char *)str);
	}
	XMoveResizeWindow (dpy, w, x, y, width, height);
}
static const char *
trace_window_id2name (Display * dpy, Window w, int *how_to_free)
{
	const char   *str = NULL;

	if (trace_window_id2name_hook)
		str = trace_window_id2name_hook (dpy, w, how_to_free);
	else
		str = trace_window_id2name_internal (dpy, w, how_to_free);
	return str;
}
static const char *
trace_window_id2name_internal (Display * dpy, Window w, int *how_to_free)
{
	const char   *str = NULL;
	XTextProperty text_prop;

	if (XGetWMName (dpy, w, &text_prop))
	{
		str = (const char *)text_prop.value;
		*how_to_free = 2;
	}
	return str;
}

int
trace_enable_function (const char *name)
{
	int           i;
	int           val = -1;

	if (!strcmp (name, "all"))
	{
		val = trace_enabled_funcs;
		trace_enabled_funcs = ~0;
	} else
	{
		for (i = 0; i < sizeof (trace_func) / sizeof (char *); i++)

		{
			if (!strcmp (trace_func[i], name))
			{
				val = (trace_enabled_funcs & (1 << i)) ? 1 : 0;
				trace_enabled_funcs |= (1 << i);
				break;
			}
		}
	}
	return val;
}

int
trace_disable_function (const char *name)
{
	int           i;
	int           val = -1;

	if (!strcmp (name, "all"))
	{
		val = trace_enabled_funcs;
		trace_enabled_funcs = 0;
	} else
	{
		for (i = 0; i < sizeof (trace_func) / sizeof (char *); i++)

		{
			if (!strcmp (trace_func[i], name))
			{
				val = (trace_enabled_funcs & (1 << i)) ? 1 : 0;
				trace_enabled_funcs &= ~(1 << i);
				break;
			}
		}
	}
	return val;
}
#endif /* DEBUG_TRACE_X */
