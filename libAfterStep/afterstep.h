/***********************************************************************
 * afterstep include file
 ***********************************************************************/

#ifndef AFTERSTEP_H_HEADER_INCLUDED
#define AFTERSTEP_H_HEADER_INCLUDED

#ifndef WithdrawnState
#define WithdrawnState 0
#endif

#ifndef set_flags
#define set_flags(v,f)  	((v)|=(f)) 
#define clear_flags(v,f)	((v)&=~(f))
#define get_flags(v,f)  	((v)&(f))
#endif

/************************** Magic Numbers : ********************************/
#define MAGIC_ASDBRECORD        0x7A3DBECD
#define MAGIC_ASWINDOW          0x7A3819D0
#define MAGIC_ASDESKTOP         0x7A3DE347
#define MAGIC_ASFEEL            0x7A3FEE1D
#define MAGIC_ASICON            0x7A301C07


/* use PanFrames! this replaces the 3 pixel margin with PanFrame windows
 * it should not be an option, once it works right. HEDU 2/2/94
 */
#define PAN_FRAME_THICKNESS 2	/* or just 1 ? */


/* The maximum number of titlebar buttons */
#define MAX_BUTTONS 10

/* the maximum number of mouse buttons afterstep knows about :
 * This is the better approach : */
#define MAX_MOUSE_BUTTONS 	(Button5-Button1+1)


#ifdef SIGNALRETURNSINT
#define SIGNAL_T int
#define SIGNAL_RETURN return 0
#else
#define SIGNAL_T void
#define SIGNAL_RETURN return
#endif

#define BW 1			/* border width */
#define BOUNDARY_WIDTH 7	/* border width */
#define CORNER_WIDTH 16		/* border width */

#define HEIGHT_EXTRA 4		/* Extra height for texts in popus */
#define HEIGHT_EXTRA_TITLE 4	/* Extra height for underlining title */
#define HEIGHT_SEPARATOR 4	/* Height of separator lines */

#define SCROLL_REGION 2		/* region around screen edge that */
				/* triggers scrolling */

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#define NULLSTR ((char *) NULL)

/* possible style index values */
#define BACK_FOCUSED		0
#define BACK_UNFOCUSED		1
#define BACK_STICKY         2
#define BACK_DEFAULT		3
#define BACK_STYLES         4

#define MENU_BACK_ITEM 		0
#define MENU_BACK_TITLE 	1
#define MENU_BACK_HILITE 	2
#define MENU_BACK_STIPPLE 	3
#define MENU_BACK_STYLES 	4


/* contexts for button presses */
#define C_NO_CONTEXT		0
#define C_WINDOW		(1 <<  0)
#define C_TITLE			(1 <<  1)
#define C_ICON			(1 <<  2)
#define C_ROOT			(1 <<  3)
#define C_FRAME			(1 <<  4)
#define C_SIDEBAR		(1 <<  5)
#define C_L1    		(1 <<  6)
#define C_L2     		(1 <<  7)
#define C_L3     		(1 <<  8)
#define C_L4     		(1 <<  9)
#define C_L5     		(1 << 10)
#define C_R1     		(1 << 11)
#define C_R2     		(1 << 12)
#define C_R3     		(1 << 13)
#define C_R4     		(1 << 14)
#define C_R5     		(1 << 15)
#define C_RALL			(C_R1|C_R2|C_R3|C_R4|C_R5)
#define C_LALL			(C_L1|C_L2|C_L3|C_L4|C_L5)
#define C_ALL			(C_WINDOW|C_TITLE|C_ICON|C_ROOT|C_FRAME|\
				 C_SIDEBAR|C_L1|C_L2|C_L3|C_L4|C_L5|\
				 C_R1|C_R2|C_R3|C_R4|C_R5)


#define INVALID_POSITION	(-30000)
#define INVALID_DESK		(10000)
#define IsValidDesk(d)		(d!=INVALID_DESK)

#include "myicon.h"

typedef struct button_t
  {
    MyIcon unpressed;		/* icon to draw when button is not pressed */
    MyIcon pressed;		    /* icon to draw when button is pressed */
    Bool is_pressed;		/* is the button pressed? */
	unsigned int width, height ;
  }
button_t;

typedef button_t MyButton ;

/* different frame decoration part ids : */
typedef enum
{				/* don't change order !!!!
				 * If you do, the following must be met :
				 *     - sides must go first and start with 0
				 *     - order must be the same as order of cursors in screen.h
				 */
  FR_N = 0,
  FR_E,
  FR_S,
  FR_W,
  FRAME_SIDES,
  FR_NW = FRAME_SIDES,
  FR_NE,
  FR_SW,
  FR_SE,
  FRAME_CORNERS,
  FRAME_PARTS = FRAME_CORNERS
}
FrameSide;

typedef struct
{
  int flags, x, y;
  unsigned int width, height;
}
ASGeometry;


typedef struct fr_pos
  {
    int x;
    int y;
    int w;
    int h;
  }
fr_pos;

struct ASHints;
struct ASStatusHints;

/* for each window that is on the display, one of these structures
 * is allocated and linked into a list 
 */
typedef struct ASWindow
  {
    struct ASWindow *next;	/* next afterstep window */
    struct ASWindow *prev;	/* prev afterstep window */
    Window w;			/* the child window */
	
	struct ASHints       *hints;
	struct ASStatusHints *status;
	struct ASStatusHints *saved_status; /* status prior to maximization */
	XPoint 				  anchor ;
	
#define ASWIN_NAME(t)       ((t)->hints->names[0])
#define ASWIN_CLASS(t)      ((t)->hints->res_class)
#define ASWIN_RES_NAME(t)   ((t)->hints->res_name)
#define ASWIN_ICON_NAME(t)  ((t)->hints->icon_name)
#define ASWIN_DESK(t)       ((t)->status->desktop)
#define ASWIN_LAYER(t)      ((t)->status->layer)
#define ASWIN_GET_FLAGS(t,f) 	get_flags((t)->status->flags,(f))
#define ASWIN_SET_FLAGS(t,f)    set_flags((t)->status->flags,(f))
#define ASWIN_CLEAR_FLAGS(t,f) 	clear_flags((t)->status->flags,(f))
#define ASWIN_FUNC_MASK(t)  ((t)->hints->function_mask)

    int old_bw;			/* border width before reparenting */
    Window frame;		/* the frame window */
    Window Parent;		/* Ugly Ugly Ugly - it looks like you
				 * HAVE to reparent the app window into
				 * a window whose size = app window,
				 * or else you can't keep xv and matlab
				 * happy at the same time! */
    Window title_w;		/* the title bar window */
    Window side;
    Window corners[2];		/* Corner pieces */
    Window fw[8];		/* frame windows */
    int nr_left_buttons;
    int nr_right_buttons;
    Window left_w[5];
    Window right_w[5];
    int space_taken_left_buttons;
    int space_taken_right_buttons;
#ifndef NO_SHAPE
    int wShaped;		/* is this a shaped window */
#endif
    int warp_index;		/* index of the window in the window list,
				   changes when window is warped */
    int frame_x;		/* x position of frame */
    int frame_y;		/* y position of frame */
    int frame_width;		/* width of frame */
    int frame_height;		/* height of frame */
    int boundary_width;
    int boundary_height;
    int corner_width;
    int bw;
    int title_x;
    int title_y;
    int title_height;		/* height of the title bar */
    int title_width;		/* width of the title bar */
    int button_height;		/* height of the buttons on title bar */

    Window icon_pixmap_w;	/* the icon window */
    int icon_p_x;		/* icon pixmap window x */
    int icon_p_y;		/* icon pixmap window y */
    int icon_p_width;		/* icon pixmap window width */
    int icon_p_height;		/* icon pixmap window height */
    Window icon_title_w;	/* the icon title window */
    int icon_t_x;		/* icon title x */
    int icon_t_y;		/* icon title y */
    int icon_t_width;		/* icon title width */
    int icon_t_height;		/* icon title height */
    char *icon_pm_file;		/* icon pixmap filename */
    Pixmap icon_pm_pixmap;	/* icon pixmap */
    Pixmap icon_pm_mask;	/* icon pixmap mask */
    int icon_pm_x;		/* icon pixmap x */
    int icon_pm_y;		/* icon pixmap y */
    int icon_pm_width;		/* icon pixmap width */
    int icon_pm_height;		/* icon pixmap height */
    int icon_pm_depth;		/* icon pixmap drawable depth */

    XWindowAttributes attr;	/* the child window attributes */
    XSizeHints normal_hints;		/* normal hints */
    XWMHints *wmhints;		/* WM hints */
    int Desk;			/* Tells which desktop this window is on */
    int FocusDesk;		/* Where (if at all) was it focussed */
    int DeIconifyDesk;		/* Desk to deiconify to, for StubbornIcons */
    int layer;			/* layer this window is in */
    Window transientfor;

    unsigned long flags;

    int orig_x;			/* unmaximized x coordinate */
    int orig_y;			/* unmaximized y coordinate */
    int orig_wd;		/* unmaximized window width */
    int orig_ht;		/* unmaximized window height */

    int *mwm_hints;
    int functions;
    Window *cmap_windows;	/* Colormap windows property */
    int number_cmap_windows;	/* Should generally be 0 */
    long focus_sequence;
    long circulate_sequence;
    unsigned long buttons;
    Bool focus_var;		/* to see if focus will work this way */
#ifndef NO_TEXTURE
    int bp_width, bp_height;	/* size of background pixmap */
    Pixmap backPixmap;		/* for titlebar background */
    Pixmap backPixmap2;		/* unfocused window titlebar */
    Pixmap backPixmap3;		/* unfocused sticky titlebar */
#endif
    MyStyle *style_focus;
    MyStyle *style_unfocus;
    MyStyle *style_sticky;
    fr_pos fp[8];
  }
ASWindow;

void bind_aswindow_styles(ASWindow *t);



#define TITLE_OLD		0	/* old (NEXTSTEP 3) style titlebar */
#define TITLE_NEXT4		1	/* NEXTSTEP 4 style titlebar */

enum				/* look file flags, used in TextureInfo */
  {
    TexturedHandle = (1 << 0),
    TitlebarNoPush = (1 << 1),
    IconNoBorder = (1 << 3),
    SeparateButtonTitle = (1 << 4)	/* icon title is a separate window */
  };

typedef struct
  {
    int Tfrom[3], Tto[3];	/* title bar rgb colors */
    int Ifrom[3], Ito[3];	/* menu item rgb colors */
    int Hfrom[3], Hto[3];	/* menu item hilite rgb colors */
    int Mfrom[3], Mto[3];	/* menu title rgb colors */
    int Ufrom[3], Uto[3];	/* unfocused item rgb colors */
    int Sfrom[3], Sto[3];	/* unfoc. sticky titlebar */
    int Tmaxcols, Imaxcols;	/* number of colors reserved */
    int Umaxcols, Mmaxcols;
    int Smaxcols, Hmaxcols;
    int Ttype, Itype, Utype;	/* texture type */
    int Htype;
    int Mtype, Stype;		/* sticky texture type */
    unsigned long flags;	/* misc. flags */
    int TGfrom[3], TGto[3];	/* gradient for the title text */
  }
TextureInfo;
extern TextureInfo Textures;

/***************************************************************************
 * window flags definitions 
 ***************************************************************************/
#define STICKY			(1<< 0)		/* Does window stick to glass? */
#define BORDER			(1<< 1)		/* Is this decorated with border */
#ifndef TITLE
#define TITLE			(1<< 2)		/* Is this decorated with title */
#endif
#define MAPPED			(1<< 3)		/* is it mapped? */
#define ICONIFIED		(1<< 4)		/* is it an icon now? */
#define TRANSIENT		(1<< 5)		/* is it a transient window? */
#define RAISED			(1<< 6)		/* if its a sticky window, does it need to be raised */
#define VISIBLE			(1<< 7)		/* is the window fully visible */
#define ICON_OURS		(1<< 8)		/* is the icon window supplied by the app? */
#define XPM_FLAG		(1<< 9)		/* is the icon window an xpm? */
#define PIXMAP_OURS		(1<<10)		/* is the icon pixmap ours to free? */
#define SHAPED_ICON		(1<<11)		/* is the icon shaped? */
#define MAXIMIZED		(1<<12)		/* is the window maximized? */
#define DoesWmTakeFocus		(1<<13)
#define DoesWmDeleteWindow	(1<<14)
/* has the icon been moved by the user? */
#define ICON_MOVED      	(1<<15)
/* was the icon unmapped, even though the window is still iconified
 * (Transients) */
#define ICON_UNMAPPED   	(1<<16)
#define WINDOWLISTSKIP		(1<<17)
#define SUPPRESSICON		(1<<18)
#define CIRCULATESKIP		(1<<19)
#define STARTICONIC		(1<<20)
/* Sent an XMapWindow, but didn't receive a MapNotify yet. */
#define MAP_PENDING		(1<<21)
#define SHADED			(1<<22)
#define NOICON_TITLE		(1<<23)
#define AVOID_COVER		(1<<24)		/* if other windows avoid covering this one */
#define VERTICAL_TITLE		(1<<25)		/* if this window has a vertical titlebar */
#define PASS_1			(1<<26)		/* if window was handled in the first pass of a multi-pass operation */
#define NOFOCUS			(1<<27)		/* set on windows that don't want any focus */
#define FRAME			(1<<28)		/* for windows that want to have a frame around the parent window */
#define HANDLES			(1<<29)
/* flags to suppress/enable title bar buttons */
#define BUTTON1     1
#define BUTTON2     2
#define BUTTON3     4
#define BUTTON4     8
#define BUTTON5    16
#define BUTTON6    32
#define BUTTON7    64
#define BUTTON8   128
#define BUTTON9   256
#define BUTTON10  512

/* possible flags to identify which tasks needs to be done on
   frame decorations/handles */
#define TODO_EVENT		(0x01)
#define TODO_RESIZE_X		(0x01<<1)
#define TODO_RESIZE_Y		(0x01<<2)
#define TODO_RESIZE		(TODO_RESIZE_X|TODO_RESIZE_Y)
#define TODO_MOVE_X		(0x01<<3)
#define TODO_MOVE_Y		(0x01<<4)
#define TODO_MOVE		(TODO_MOVE_Y|TODO_MOVE_X)
#define TODO_TITLE		(0x01<<5)
#define TODO_SIDE		(0x01<<6)
#define TODO_DECOR		(TODO_TITLE|TODO_SIDE)
#define TODO_REDRAW_TITLE 	(0x01<<7)
#define TODO_REDRAW_SIDE  	(0x01<<8)
#define TODO_REDRAW		(TODO_REDRAW_TITLE|TODO_REDRAW_SIDE)
#define TODO_PAGE_X		(0x01<<9)
#define TODO_PAGE_Y		(0x01<<10)
#define TODO_PAGE		(TODO_PAGE_X|TODO_PAGE_Y)
/* this 2 are used in MyWidgets : */
#define TODO_REBUILD_CACHE	(0x01<<11)
#define TODO_REDRAW_WIDGET	(0x01<<12)
/* */

/*
 * FEW PRESET LEVELS OF OUTPUT :
 */
#define OUTPUT_LEVEL_PARSE_ERR      1
#define OUTPUT_LEVEL_HINTS          OUTPUT_VERBOSE_THRESHOLD
#define OUTPUT_LEVEL_DATABASE       OUTPUT_VERBOSE_THRESHOLD
#define OUTPUT_LEVEL_VROOT          7
#define OUTPUT_LEVEL_WINDOW_LIST   (OUTPUT_LEVEL_DEBUG+9) /* too much output - too slow */


extern void Reborder (void);
extern void SigDone (int);
extern void SaveWindowsOpened (void);
extern void Done (int, char *);

extern XClassHint NoClass;

extern XContext ASContext;

extern Window JunkRoot, JunkChild;
extern int JunkX, JunkY;
extern unsigned int JunkWidth, JunkHeight, JunkBW, JunkDepth, JunkMask;

#ifdef PAN_FRAMES
extern void checkPanFrames ();
extern void raisePanFrames ();
#endif

extern Atom _XA_MIT_PRIORITY_COLORS;
extern Atom _XA_WM_CHANGE_STATE;
extern Atom _XA_WM_STATE;
extern Atom _XA_WM_COLORMAP_WINDOWS;
extern Atom _XA_WM_PROTOCOLS;
extern Atom _XA_WM_TAKE_FOCUS;
extern Atom _XA_WM_SAVE_YOURSELF;
extern Atom _XA_WM_DELETE_WINDOW;
extern Atom _XA_WM_DESKTOP;
extern Atom _XA_FVWM_STICKS_TO_GLASS;
extern Atom _XA_FVWM_CLIENT;
extern Atom _XROOTPMAP_ID;
extern Atom _AS_STYLE;
extern Atom _AS_MODULE_SOCKET;

#endif /* #ifndef AFTERSTEP_H_HEADER_INCLUDED */