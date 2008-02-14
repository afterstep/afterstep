#ifndef AFTERSHOW_H_INCLUDED
#define AFTERSHOW_H_INCLUDED

#ifndef STDIN_FILENO
# define STDIN_FILENO   0
# define STDOUT_FILENO  1
# define STDERR_FILENO  2
#endif

#if !defined (EACCESS) && defined(EAGAIN)
# define EACCESS EAGAIN
#endif

#ifndef EXIT_SUCCESS            /* missing from <stdlib.h> */
# define EXIT_SUCCESS           0       /* exit function success */
# define EXIT_FAILURE           1       /* exit function failure */
#endif


typedef struct AfterShowXWindowLayer
{
	int x, y, width, height;
#ifndef X_DISPLAY_MISSING
	Pixmap pmap;
	int pmap_width, pmap_height;
#endif
}AfterShowXWindowLayer;

typedef struct AfterShowXWindow
{
	int layers_num;
	AfterShowXWindowLayer *layers;

#ifndef X_DISPLAY_MISSING
	Window 	w;
	Pixmap  back;
	int back_width, back_height;
	
#endif
}AfterShowXWindow;

typedef struct AfterShowXScreen
{
	Bool 		do_service;
	int 		screen;
	int 		windows_num;
	AfterShowXWindow *windows;
	
#ifndef X_DISPLAY_MISSING
	ASVisual   *asv;
	Window  	root;
	int 		root_width, root_height;
	Window 		selection_window;
	Atom   		_XA_AFTERSHOW_S;
	Time 		selection_time;
	
#endif
}AfterShowXScreen;

typedef struct AfterShowXGUI
{
	Bool 		valid;
	int 		screens_num;
	int 		serviced_screens_num;

#ifndef X_DISPLAY_MISSING
	Display    *dpy;
	int 		fd;
	AfterShowXScreen  *screens;
#endif	
}AfterShowXGUI;

typedef struct AfterShowWin32GUI
{
	Bool 		valid;
	int 		screens_num;
	int 		serviced_screens_num;

#ifdef WIN32
	int 		root_width, root_height;
#endif	
}AfterShowWin32GUI;


typedef struct AfterShowContext
{
#define AfterShow_DoFork		(0x01<<0)
#define AfterShow_SingleScreen	(0x01<<1)
	ASFlagType flags;
	
	char *display;
	
	struct {
		AfterShowXGUI  		x;
		AfterShowWin32GUI	win32;
	} gui;

	int fd_width;	
}AfterShowContext;

/***** from xutils.h */
#ifndef X_DISPLAY_MISSING
Bool aftershow_get_drawable_size_and_depth ( AfterShowContext *ctx,
											 Drawable d, 
											 int *width_return, 
											 int *height_return, 
											 int *depth_return);
Bool aftershow_validate_drawable (AfterShowContext *ctx, Drawable d);
#endif

#endif /* AFTERSHOW_H_INCLUDED */
