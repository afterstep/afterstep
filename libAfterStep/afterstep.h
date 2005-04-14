/***********************************************************************
 * afterstep include file
 ***********************************************************************/

#ifndef AFTERSTEP_H_HEADER_INCLUDED
#define AFTERSTEP_H_HEADER_INCLUDED

#define NO_ASRENDER

#include "myicon.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#define MAGIC_MENU_DATA         0x7A3EDDAF
#define MAGIC_MENU_DATA_ITEM    0x7A3ED1FE
#define MAGIC_COMPLEX_FUNC      0x7A3CCF46
#define MAGIC_ASMENU            0x7A3E78C1

#define ASMENU_RES_CLASS        "ASMenu"

#define MAGIC_MYLOOK_RES_BASE   0x7A311000
#define MAGIC_MYLOOK            (MAGIC_MYLOOK_RES_BASE+0)
#define MAGIC_MYSTYLE           (MAGIC_MYLOOK_RES_BASE+1)
#define MAGIC_MYBACKGROUND      (MAGIC_MYLOOK_RES_BASE+2)
#define MAGIC_MYDESKTOP         (MAGIC_MYLOOK_RES_BASE+3)
#define MAGIC_MYFRAME           (MAGIC_MYLOOK_RES_BASE+4)
#define MAGIC_MYDESKTOPCONFIG   (MAGIC_MYLOOK_RES_BASE+5)





/* List of flags that allows us to put AfterSTep in different states : */
#define ASS_NormalOperation     (0x01<<0)      /* otherwise we are in restart or shutdown or initialization  */
#define ASS_Restarting          (0x01<<1)      /* otherwise we either initializing for the first time or shutting down */
#define ASS_Shutdown            (0x01<<2)      /* otherwise we either initializing for the first time or shutting down */
#define ASS_SingleScreen        (0x01<<3)      /* if we've been requested not to spawn our copies on other screens */
#define ASS_RunningLocal        (0x01<<4)      /* if AfterSTep runs on the same host as X Server */
#define ASS_Debugging           (0x01<<5)      /* if we are debugging AfterSTep and hence should run synchronously */
#define ASS_HousekeepingMode    (0x01<<6)      /* after GrabEm and before UngrabEm - noone has focus, we can do what we want. */
#define ASS_WarpingMode         (0x01<<7)      /* after first F_WARP* and before mouse/keyboard actions. */
#define ASS_PointerOutOfScreen  (0x01<<8)      /*  */
#define ASS_ScreenGrabbed		(0x01<<9)      /* XGrabScreen had been issued - we should try and
												  avoid deadlocks ! */
#define ASS_SuppressDeskBack	(0x01<<10)      

#define AS_Text_ASCII			0
#define AS_Text_UTF8			1
#define AS_Text_UNICODE			2


/* use PanFrames! this replaces the 3 pixel margin with PanFrame windows
 * it should not be an option, once it works right. HEDU 2/2/94
 */
#define PAN_FRAME_THICKNESS 2	/* or just 1 ? */


/* The maximum number of titlebar buttons */
#define MAX_BUTTONS 10

#define MAX_CURSORS	32                         /* just for the heck of it! */


/* the maximum number of mouse buttons afterstep knows about :
 * This is the better approach : */
#define MAX_MOUSE_BUTTONS 	(Button5-Button1+1)

#define BW 1			/* border width */
#define BOUNDARY_WIDTH 7	/* border width */
#define CORNER_WIDTH 16		/* border width */

#define HEIGHT_EXTRA 4		/* Extra height for texts in popus */
#define HEIGHT_EXTRA_TITLE 4	/* Extra height for underlining title */
#define HEIGHT_SEPARATOR 4	/* Height of separator lines */

#define SCROLL_REGION 2		/* region around screen edge that */
				/* triggers scrolling */

#define NO_HILITE     0
#define TOP_HILITE    (0x01<<0)
#define RIGHT_HILITE  (0x01<<1)
#define BOTTOM_HILITE (0x01<<2)
#define LEFT_HILITE   (0x01<<3)
#define NORMAL_HILITE (TOP_HILITE|RIGHT_HILITE|BOTTOM_HILITE|LEFT_HILITE)
#define EXTRA_HILITE  (0x01<<4)
#define FULL_HILITE   (EXTRA_HILITE|NORMAL_HILITE)
#define NO_HILITE_OUTLINE  (0x01<<5)
#define HILITE_MASK   (NO_HILITE_OUTLINE|FULL_HILITE)

#define DEFAULT_TBAR_HILITE   (BOTTOM_HILITE|RIGHT_HILITE)
#define DEFAULT_MENU_HILITE   (BOTTOM_HILITE|RIGHT_HILITE)
#define DEFAULT_MENU_OHILITE  (BOTTOM_HILITE|RIGHT_HILITE)


#define PAD_H_OFFSET    0
#define PAD_LEFT        (0x01<<0)
#define PAD_RIGHT       (0x01<<1)
#define PAD_H_MASK      (PAD_RIGHT|PAD_LEFT)

#define PAD_V_OFFSET    (PAD_H_OFFSET+2)
#define PAD_BOTTOM      (0x01<<(PAD_V_OFFSET))    /* rotated with Top on purpose */
#define PAD_TOP         (0x01<<(PAD_V_OFFSET+1))
#define PAD_V_MASK      (PAD_TOP|PAD_BOTTOM)
#define PAD_MASK        (PAD_V_MASK|PAD_H_MASK)

#define RESIZE_H_OFFSET (PAD_V_OFFSET+2)
#define RESIZE_H        (0x01<<(RESIZE_H_OFFSET))
#define RESIZE_H_SCALE  (0x01<<(RESIZE_H_OFFSET+1))
#define RESIZE_H_MASK   (RESIZE_H|RESIZE_H_SCALE)

#define RESIZE_V_OFFSET (RESIZE_H_OFFSET+2)
#define RESIZE_V        (0x01<<(RESIZE_V_OFFSET))
#define RESIZE_V_SCALE  (0x01<<(RESIZE_V_OFFSET+1))
#define RESIZE_V_MASK   (RESIZE_V|RESIZE_V_SCALE)
#define RESIZE_MASK     (RESIZE_V_MASK|RESIZE_H_MASK)

#define FIT_LABEL_WIDTH  (0x01<<(RESIZE_V_OFFSET+2))
#define FIT_LABEL_HEIGHT (0x01<<(RESIZE_V_OFFSET+3))
#define FIT_LABEL_SIZE   (FIT_LABEL_WIDTH|FIT_LABEL_HEIGHT)

#define NO_ALIGN      0
#define ALIGN_LEFT    PAD_RIGHT
#define ALIGN_RIGHT   PAD_LEFT
#define ALIGN_TOP     PAD_BOTTOM
#define ALIGN_BOTTOM  PAD_TOP
#define ALIGN_HCENTER (PAD_LEFT|PAD_RIGHT)
#define ALIGN_VCENTER (PAD_TOP|PAD_BOTTOM)
#define ALIGN_CENTER  (ALIGN_VCENTER|ALIGN_HCENTER)

#define DEFAULT_TBAR_HSPACING        5
#define DEFAULT_TBAR_VSPACING        1
#define DEFAULT_MENU_SPACING        5
#define DEFAULT_MENU_ITEM_HBORDER   1
#define DEFAULT_MENU_ITEM_VBORDER   1


/* possible style index values */
#define BACK_FOCUSED		0
#define BACK_UNFOCUSED		1
#define BACK_STICKY         2
#define BACK_DEFAULT		3
#define BACK_STYLES         4

#define MENU_BACK_TITLE     0
#define MENU_BACK_ITEM      1
#define MENU_BACK_HILITE    2
#define MENU_BACK_STIPPLE 	3
#define MENU_BACK_SUBITEM   4
#define MENU_BACK_HITITLE   5
#define MENU_BACK_STYLES    6

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
  FRAME_PARTS = FRAME_CORNERS,
  FRAME_TITLE = FRAME_PARTS,
  FRAME_ALL
}
FrameSide;

#define FRAME_TOP_MASK      ((0x01<<FR_N)|(0x01<<FR_NW)|(0x01<<FR_NE))
#define FRAME_BTM_MASK      ((0x01<<FR_S)|(0x01<<FR_SW)|(0x01<<FR_SE))
#define FRAME_LEFT_MASK      (0x01<<FR_W)
#define FRAME_RIGHT_MASK     (0x01<<FR_E)




/* contexts for button presses */
#define C_NO_CONTEXT		0
#define C_FrameN        (0x01<<0)
#define C_FrameE        (0x01<<1)
#define C_FrameS        (0x01<<2)
#define C_FrameW        (0x01<<3)
#define C_FrameNW       (0x01<<4)
#define C_FrameNE       (0x01<<5)
#define C_FrameSW       (0x01<<6)
#define C_FrameSE       (0x01<<7)
#define C_SIDEBAR           (C_FrameS|C_FrameSW|C_FrameSE)
#define C_VERTICAL_SIDEBAR  (C_FrameW|C_FrameNW|C_FrameSW)
#define C_FRAME             (C_FrameN|C_FrameE|C_FrameS|C_FrameW| \
                             C_FrameNW|C_FrameNE|C_FrameSW|C_FrameSE)
#define C_FrameStart    (C_FrameN)
#define C_FrameEnd      (C_FrameSE)

#define C_WINDOW        (0x01<<8)
#define C_CLIENT        C_WINDOW
#define C_TITLE         (0x01<<9)
#define C_IconTitle     (0x01<<10)
#define C_IconButton    (0x01<<11)
#define C_ICON          (C_IconButton|C_IconTitle)
#define C_ROOT          (0x01<<12)
#define C_TButton0      (0x01<<13)
#define C_TButton1      (0x01<<14)
#define C_TButton2      (0x01<<15)
#define C_TButton3      (0x01<<16)
#define C_TButton4      (0x01<<17)
#define C_TButton5      (0x01<<18)
#define C_TButton6      (0x01<<19)
#define C_TButton7      (0x01<<20)
#define C_TButton8      (0x01<<21)
#define C_TButton9      (0x01<<22)
#define C_TButtonAll    (C_TButton0|C_TButton1|C_TButton2|C_TButton3|C_TButton4| \
                         C_TButton5|C_TButton6|C_TButton7|C_TButton8|C_TButton9)
#define C_ALL           (C_WINDOW|C_TITLE|C_ICON|C_ROOT|C_FRAME|C_SIDEBAR|C_TButtonAll)

#define TITLE_BUTTONS       10

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


#define MAX_POSITION        30000
#define INVALID_POSITION    (-MAX_POSITION)
#define INVALID_DESK        10000
#define IsValidDesk(d)		(d!=INVALID_DESK)


#define MAX_RUBBER_BAND         7

struct icon_t;
struct button_t;

typedef struct
{
  int flags, x, y;
  unsigned int width, height;
}
ASGeometry;

typedef struct ASRectangle
{/* 64 bit safe : */
  long x, y;
  unsigned long width, height;
}ASRectangle;

typedef struct
{
#define LeftValue	(0x01<<0)
#define TopValue	(0x01<<1)
#define RightValue	(0x01<<2)
#define BottomValue	(0x01<<3)
#define LeftNegative	(0x01<<4)
#define TopNegative	(0x01<<5)
#define RightNegative	(0x01<<6)
#define BottomNegative	(0x01<<7)
  int flags;
  int left, top;
  int right, bottom;
}
ASBox;

typedef struct ASCursor
{
	char              *image_file;   /* image of the cursor */
	char              *mask_file;    /* mask of the cursor  */
	Cursor             cursor;       /* loaded X Cursor */
}ASCursor;


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
struct ASCanvas;
struct ASTBarData;


#define AS_CLIENT_EVENT_MASK 		(ColormapChangeMask | \
									 EnterWindowMask 	| \
									 FocusChangeMask 	| \
									 LeaveWindowMask 	| \
                                     PropertyChangeMask | \
									 StructureNotifyMask )


#define AS_FRAME_EVENT_MASK  		(ButtonPressMask 	| \
									 ButtonReleaseMask 	| \
									 EnterWindowMask 	| \
                                     FocusChangeMask 	| \
									 LeaveWindowMask 	| \
                                     SubstructureRedirectMask| \
                                     StructureNotifyMask| \
                                     PointerMotionMask)  /* needed for proper Balloons operation */

#define AS_CANVAS_EVENT_MASK 		(ButtonPressMask 	| \
									 ButtonReleaseMask 	| \
									 EnterWindowMask 	| \
									 LeaveWindowMask 	| \
									 StructureNotifyMask| \
									 PointerMotionMask)  /* needed to be able to switch mouse pointers */

#define AS_ICON_TITLE_EVENT_MASK 	(ButtonPressMask 	| \
                                     ButtonReleaseMask 	| \
									 EnterWindowMask 	| \
									 FocusChangeMask 	| \
                                     KeyPressMask       | \
                                     StructureNotifyMask)

#define AS_ICON_EVENT_MASK 	 		(ButtonPressMask 	| \
									 ButtonReleaseMask 	| \
									 EnterWindowMask 	| \
									 FocusChangeMask 	| \
								     KeyPressMask 		| \
                                     LeaveWindowMask    | \
                                     StructureNotifyMask)

#define AS_MENU_EVENT_MASK         ( ButtonPressMask    | \
									 ButtonReleaseMask 	| \
                                     PointerMotionMask  | \
									 EnterWindowMask 	| \
								     KeyPressMask 		| \
                                     LeaveWindowMask    | \
                                     StructureNotifyMask)


#define AS_ROOT_EVENT_MASK          (LeaveWindowMask | \
                                     EnterWindowMask | \
                                     PropertyChangeMask | \
                                     SubstructureRedirectMask |  \
                                      /* SubstructureNotifyMask | */\
                                     KeyPressMask | \
                                     ButtonPressMask | \
                                     ButtonReleaseMask )

#define AS_ICON_MYSTYLE					"ButtonPixmap"
#define AS_ICON_TITLE_MYSTYLE			"ButtonTitleFocus"
#define AS_ICON_TITLE_STICKY_MYSTYLE	"ButtonTitleSticky"
#define AS_ICON_TITLE_UNFOCUS_MYSTYLE	"ButtonTitleUnfocus"





#define TITLE_OLD		0	/* old (NEXTSTEP 3) style titlebar */
#define TITLE_NEXT4		1	/* NEXTSTEP 4 style titlebar */

#define ASIMMAN_PATH_PIXMAPPATH  	0
#define ASIMMAN_PATH_IMAGEPATH   	1
#define ASIMMAN_PATH_PATH   		2
#define ASIMMAN_PATH_PRIVICONS		3
#define ASIMMAN_PATH_PRIVBACKS		4

#ifdef __cplusplus
}
#endif



#endif /* #ifndef AFTERSTEP_H_HEADER_INCLUDED */
