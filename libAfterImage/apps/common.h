#ifndef AFTERIMAGE_COMMON_H_HEADER_INCLUDED
#define AFTERIMAGE_COMMON_H_HEADER_INCLUDED

extern Atom _XA_WM_DELETE_WINDOW;
Window create_top_level_window( ASVisual *asv, Window root, int x, int y,
	                            unsigned int width, unsigned int height,
								unsigned int border_width,
								unsigned long attr_mask,
								XSetWindowAttributes *attr,
								char *app_class );
Pixmap set_window_background_and_free( Window w, Pixmap p );

#endif

