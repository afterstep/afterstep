/***********************************************************************
 *
 * afterstep per-screen data include file
 *
 ***********************************************************************/

#ifndef SCREEN_H_HEADER_INCLUDED
#define SCREEN_H_HEADER_INCLUDED

#include "afterstep.h"
#include "clientprops.h"
#include "hints.h"
#include "asdatabase.h"
#include "font.h"
#include "mylook.h"
#include "asfeel.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SIZE_HINDENT 5
#define SIZE_VINDENT 3
#define MAX_WINDOW_WIDTH 32767
#define MAX_WINDOW_HEIGHT 32767

/* Maximum number of icon boxes that are allowed */
#define MAX_BOXES 4

/* button styles */
#define NO_BUTTON_STYLE		-1	/* button is undefined */
#define XPM_BUTTON_STYLE	2	/* button is a pixmap */

#ifndef NO_VIRTUAL
typedef struct
{
    Window win;
    int isMapped;
}ASPanFrame;

#endif

/* look file flags */
struct MenuRoot;
struct MouseButton;
struct FuncKey;
struct MyStyle;
struct ASSupportedHints;
struct MyFrame;
struct ASHashTable;
struct ComplexFunction;
struct ASFontManager;
struct ASImageManager;
struct ASCanvas;
struct ASMoveResizeData;

typedef struct ASDesktop
{
	int 	Desk ;
	unsigned short vx, vy ;
}ASDesktop;

typedef struct ASIconBox
{
	int desktop ;
    ASGeometry *areas ;
	unsigned short areas_num ;
	ASBiDirList *icons ;
}ASIconBox;

typedef struct ASBackgroundHandler
{
    Pixmap  pmap;
    unsigned int pmap_width, pmap_height ;
    int     cmd_pid;
    ASImage *im;
}ASBackgroundHandler;

typedef struct ScreenInfo
  {
    unsigned long screen;
    int d_depth;            /* copy of DefaultDepth(dpy, screen) */
    int NumberOfScreens;	/* number of screens on display */
    int MyDisplayWidth;		/* my copy of DisplayWidth(dpy, screen) */
    int MyDisplayHeight;	/* my copy of DisplayHeight(dpy, screen) */

    Bool localhost ;
    char *rdisplay_string, *display_string;

    struct ASWMProps    *wmprops;              /* window management properties */

	struct ASVisual *asv ;  /* ASVisual for libAfterImage */
    Window Root;        /* the root window */
    struct ASImage  *RootImage;
    struct ASCanvas *RootCanvas;
    /* this is used to limit area of the root window from which to get root image : */
    XRectangle RootClipArea;                /* used only by modules */
    ASBackgroundHandler *RootBackground;    /* used only by those who change root background */

    Window SizeWindow;      /* the resize dimensions window */
    Window ServiceWin;      /* Auxilary window that we use for :
                             *    1) hiding focus - it will own focus when no other windows have it
                             *    2) desktop switching - off-desktop windows will be reparented there
                             */

    struct ASWindowList *Windows ;
/*    ASWindow ASRoot;        the head of the afterstep window list */
/*    struct ASHashTable *aswindow_xref;        xreference of window/resource IDs to ASWindow structures */

    struct ASIconBox   *default_icon_box ; /* if we have icons following desktops - then we only need one icon box */
	struct ASHashTable *icon_boxes ; /* hashed by desk no - one icon box per desktop ! */

#define PAN_FRAME_SIDES 4

#define AS_PANFRAME_EVENT_MASK (EnterWindowMask|LeaveWindowMask|VisibilityChangeMask)

#ifndef NO_VIRTUAL
    ASPanFrame PanFrame[PAN_FRAME_SIDES];
    int usePanFrames;		/* toggle to disable them */
#endif

    /* interactive move resize data : */
    struct ASMoveResizeData *moveresize_in_progress;

    int randomx;        /* values used for randomPlacement */
    int randomy;
    unsigned VScale;		/* Panner scale factor */
    int VxMax;			/* Max location for top left of virt desk */
    int VyMax;
    int Vx;			/* Current loc for top left of virt desk */
    int Vy;

    int CurrentDesk;        /* The current desktop number */
    int LastValidDesk;      /* Last nonspecial desktop's number  (<> 10000) */

    Time   last_Timestamp;                      /* last event timestamp */
    Time   menu_grab_Timestamp;                 /* pointer grab time used in menus */

    ASFeel  Feel;
    MyLook  Look;

    Cursor  standard_cursors[MAX_CURSORS];

    GC DrawGC;          /* GC to draw lines for move and resize */

    int xinerama_screens_num ;
	XRectangle *xinerama_screens;

    struct ASFontManager  *font_manager ;
    struct ASImageManager *image_manager ;

	Bool (*on_dead_window)( Window w );
	
	/* supported X extentions : */
	int XineEventBase, XineErrorBase;
	int	ShmCompletionEventType ;
	int	ShapeEventBase ;
	int	ShapeErrorBase ;

}ScreenInfo;

void init_screen_gcs(ScreenInfo *scr);
void destroy_screen_gcs(ScreenInfo *scr);

void make_screen_envvars( ScreenInfo *scr );

void init_screen_panframes(ScreenInfo *scr);
void check_screen_panframes(ScreenInfo *scr);
void raise_scren_panframes (ScreenInfo *scr);
Window get_lowest_panframe(ScreenInfo *scr);


#ifdef HAVE_XINERAMA
void get_Xinerama_rectangles (ScreenInfo * scr);
#endif
Bool set_synchronous_mode (Bool enable);
int ConnectX (ScreenInfo * scr, unsigned long event_mask);
void setup_modifiers ();

#define  create_screen_window(scr,p,x,y,w,h,bw,c,mask,attr) \
    create_visual_window((scr)->asv,((p)==None)?((scr)->Root):(p),x,y,w,h,bw,c,mask,attr)

void merge_geometry( ASGeometry *from, ASGeometry *to );
void check_desksize_sanity( ScreenInfo *scr );

#ifdef __cplusplus
}
#endif



#endif /* _SCREEN_ */
