#include "config.h"

#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>

#include "afterimage.h"

/****h* libAfterImage/tutorials/common.h
 * SYNOPSIS
 * Generic Xlib related functionality, common for all the tutorials.
 * SEE ALSO
 * libAfterImage,
 * ASView, ASScale, ASTint, ASMerge, ASGrad, ASFlip, ASText
 **************/

/****v* libAfterImage/tutorials/_XA_WM_DELETE_WINDOW
 * SYNOPSIS
 * _XA_WM_DELETE_WINDOW - stores value of X Atom "WM_DELETE_WINDOW".
 * DESCRIPTION
 * In X all client's top level windows are managed by window manager.
 * That includes moving, resizing, decorating, focusing and closing of
 * windows. Interactions between window manager and client window are
 * governed by ICCCM specification.
 * All the parts of this specification are completely optional, but it
 * is recommended that the following minimum set of hints be supplied
 * for any client window:
 * Window's title(WM_NAME), iconified window title(WM_ICON_NAME), class
 * hint (WM_CLASS) and supported protocols (WM_PROTOCOLS). It is
 * recommended also that WM_DELETE_WINDOW protocol be supported, as
 * otherwise there are no way to safely close client window, but to
 * kill it.
 * All of the above mentioned hints are identified by atoms that have
 * standard preset values, except for WM_DELETE_WINDOW. As the result we
 * need to obtain WM_DELETE_WINDOW atoms ID explicitly. We use
 * _XA_WM_DELETE_WINDOW to store the ID of that atom, so it is accessible
 * anywhere from our application.
 * SOURCE
 */
Atom _XA_WM_DELETE_WINDOW = None;
/**************/

/****f* libAfterImage/tutorials/create_top_level_window()
 * SYNOPSIS
 * Window create_top_level_window( ASVisual *asv, Window root,
 *                                 int x, int y,
 *                                 unsigned int width,
 *                                 unsigned int height,
 *                                 unsigned int border_width,
 *                                 unsigned long attr_mask,
 *                                 XSetWindowAttributes *attr,
 *                                 char *app_class );
 * INPUTS
 * asv           - pointer to valid preinitialized ASVisual structure.
 * root          - root window of the screen on which to create window.
 * x, y          - desired position of the window
 * width, height - desired window size.
 * border_width  - desired initial border width of the window (may not
 *                 have any effect due to Window Manager intervention.
 * attr_mask     - mask of the attributes that has to be set on the
 *                 window
 * attr          - values of the attributes to be set.
 * app_class     - title of the application to be used as its window
 *                 Title and resources class.
 * RETURN VALUE
 * ID of the created window.
 * DESCRIPTION
 * create_top_level_window() is autyomating process of creating top
 * level application window. It creates window for specified ASVisual,
 * and then sets up basic ICCCM hints for that window, such as WM_NAME,
 * WM_ICON_NAME, WM_CLASS and WM_PROTOCOLS.
 * SOURCE
 */
Window
create_top_level_window( ASVisual *asv, Window root, int x, int y,
                         unsigned int width, unsigned int height,
						 unsigned int border_width, unsigned long attr_mask,
						 XSetWindowAttributes *attr, char *app_class )
{
	Window w ;
	char *tmp ;
	XTextProperty name;
	XClassHint class1;

	w = create_visual_window(asv, root, x, y, width, height, border_width, InputOutput, attr_mask, attr );

	tmp = (char*)get_application_name();
    XStringListToTextProperty (&tmp, 1, &name);

    class1.res_name = tmp;	/* for future use */
    class1.res_class = app_class;
    XSetWMProtocols (dpy, w, &_XA_WM_DELETE_WINDOW, 1);
    XSetWMProperties (dpy, w, &name, &name, NULL, 0, NULL, NULL, &class1);
    /* final cleanup */
    XFree ((char *) name.value);

	return w;
}
/**************/

