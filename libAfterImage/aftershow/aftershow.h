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


#define MAGIC_AFTERSHOW_X_LAYER		0xAF501A73
#define MAGIC_AFTERSHOW_X_WINDOW	0xAF50814D
#define MAGIC_AFTERSHOW_X_SCREEN	0xAF503C4E
#define MAGIC_AFTERSHOW_X_GUI		0xAF507201
#define MAGIC_AFTERSHOW_WIN32_GUI	0xAF507232

typedef struct AfterShowXLayer
{

	unsigned long magic;
	int x, y, width, height;
#ifndef X_DISPLAY_MISSING
	Pixmap pmap;
	int pmap_width, pmap_height;
#endif
}AfterShowXLayer;

struct AfterShowXScreen;

typedef struct AfterShowXWindow
{
	unsigned long magic;
	int layers_num, default_layer;
	AfterShowXLayer *layers;
	struct AfterShowXScreen *screen;
	
#ifndef X_DISPLAY_MISSING
	Window 	w;
	int width, height;
	Pixmap  back;
	GC 		gc ;	
	int 	depth;
	int 	back_width, back_height;
#endif
}AfterShowXWindow;

typedef struct AfterShowXScreen
{
	unsigned long magic;
	Bool 		do_service;
	int 		screen;

	AfterShowXWindow root; /* not part of the above list */

	ASHashTable *windows;

#ifndef X_DISPLAY_MISSING
	ASVisual   *asv;
	Window 		selection_window;
	Atom   		_XA_AFTERSHOW_S;
	Time 		selection_time;
	
#endif
}AfterShowXScreen;

typedef struct AfterShowXGUI
{
	unsigned long magic;
	Bool 		valid;
	int 		screens_num;
	int 		serviced_screens_num;
	int 		first_screen;

#ifndef X_DISPLAY_MISSING
	Display    *dpy;
	int 		fd;
	AfterShowXScreen  *screens;
#endif	
}AfterShowXGUI;

typedef struct AfterShowWin32GUI
{
	unsigned long magic;
	Bool 		valid;
	int 		screens_num;
	int 		serviced_screens_num;
	int 		first_screen;

#ifdef WIN32
	int 		root_width, root_height;
#endif	
}AfterShowWin32GUI;

struct ASXmlBuffer;

typedef union
{
	ASMagic 				*magic;
	ASImage 				*asim;
	AfterShowXLayer			*xlayer;
	AfterShowXWindow		*xwindow;
	AfterShowXScreen		*xscreen;
	AfterShowXGUI			*xgui;
	AfterShowWin32GUI		*win32gui;
}AfterShowMagicPtr;

typedef struct AfterShowClient
{
	int 				 fd;
	struct ASXmlBuffer 	*xml_input_buf; 
	struct xml_elem_t	*xml_input_head, *xml_input_tail;
	struct xml_elem_t	*xml_output_head, *xml_output_tail;
	struct ASXmlBuffer 	*xml_output_buf; 
	
	AfterShowMagicPtr 	 default_gui;		/* could be either x or win32 */
	int 				 default_screen;
	AfterShowMagicPtr 	 default_window;	/* could be either x or win32 */
	ASImageManager 		 imman;				/* if NULL - use the shared one */
	ASFontManager 		 fontman;			/* if NULL - use the shared one */
}AfterShowClient;

typedef struct AfterShowContext
{
#define AfterShow_DoFork		(0x01<<0)
#define AfterShow_SingleScreen	(0x01<<1)
	ASFlagType 	 flags;
	
	char   		*display;
	
	struct {
		AfterShowXGUI  		x;
		AfterShowWin32GUI	win32;
	} gui;

	char   		*socket_name;
	int 		 socket_fd;
	int 		 min_fd; /* max (socket_fd, x.fd) */
	int 		 fd_width;	
	
	AfterShowClient	*clients; /* array of fd_width elements */

	ASImageManager 	*imman;		/* shared image manager */
	ASFontManager 	*fontman;	/* shared font manager */

}AfterShowContext;

/***** from xutil.c */
#ifndef X_DISPLAY_MISSING

#define XA_AFTERSHOW_SOCKET_NAME "_AFTERSHOW_SOCKET"

AfterShowXWindow* aftershow_XWindowID2XWindow (AfterShowContext *ctx, Window w);
AfterShowXScreen* aftershow_XWindowID2XScreen (AfterShowContext *ctx, Window w);

Bool aftershow_connect_x_gui(AfterShowContext *ctx);

Bool aftershow_get_drawable_size_and_depth ( AfterShowContext *ctx,
											 Drawable d, 
											 int *width_return, 
											 int *height_return, 
											 int *depth_return);
Bool aftershow_validate_drawable (AfterShowContext *ctx, Drawable d);
void aftershow_set_string_property (AfterShowContext *ctx, Window w, Atom property, char *data);
char *aftershow_read_string_property (AfterShowContext *ctx, Window w, Atom property);
AfterShowXWindow *aftershow_create_x_window (AfterShowContext *ctx, AfterShowXWindow *parent, int width, int height);
AfterShowXLayer *aftershow_create_x_layer (AfterShowContext *ctx, AfterShowXWindow *window);
Bool aftershow_ASImage2XLayer ( AfterShowContext *ctx, AfterShowXWindow *window, 
								AfterShowXLayer *layer, ASImage *im,  int dst_x, int dst_y);
void aftershow_ExposeXWindowArea (AfterShowContext *ctx, AfterShowXWindow *window, int left, int top, int right, int bottom);

#endif

/***** from xmlutil.c */
void aftershow_add_tags_to_queue( xml_elem_t* tags, xml_elem_t **phead, xml_elem_t **ptail);


#endif /* AFTERSHOW_H_INCLUDED */
