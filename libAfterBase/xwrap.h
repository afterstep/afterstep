#ifndef XWRAP_H_HEADER_INCLUDED
#define XWRAP_H_HEADER_INCLUDED

extern Display *dpy;

Bool     get_drawable_size (Drawable d, unsigned int *ret_w, unsigned int *ret_h);
Drawable validate_drawable (Drawable d, unsigned int *pwidth, unsigned int *pheight);
void	 backtrace_window ( Window w );

Window get_parent_window( Window w );
Window get_topmost_parent( Window w, Window *desktop_w );

#endif
