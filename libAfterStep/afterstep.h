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

#define MAGIC_MENU_DATA         0x7A3EDDAF
#define MAGIC_MENU_DATA_ITEM    0x7A3ED1FE
#define MAGIC_COMPLEX_FUNC      0x7A3CCF46


/* List of flags that allows us to put AfterSTep in different states : */
#define ASS_NormalOperation     (0x01<<0)      /* otherwise we are in restart or shutdown or initialization  */
#define ASS_Restarting          (0x01<<1)      /* otherwise we either initializing for the first time or shutting down */
#define ASS_SingleScreen        (0x01<<2)      /* if we've been requested not to spawn our copies on other screens */
#define ASS_RunningLocal        (0x01<<3)      /* if AfterSTep runs on the same host as X Server */
#define ASS_Debugging           (0x01<<4)      /* if we are debugging AfterSTep and hence should run synchronously */
#define ASS_HousekeepingMode    (0x01<<5)      /* after GrabEm and before UngrabEm - noone has focus, we can do what we want. */
#define ASS_WarpingMode         (0x01<<6)      /* after first F_WARP* and before mouse/keyboard actions. */
#define ASS_PointerOutOfScreen  (0x01<<7)      /*  */

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

#define NO_HILITE     0x0000
#define TOP_HILITE    0x0001
#define RIGHT_HILITE  0x0002
#define BOTTOM_HILITE 0x0004
#define LEFT_HILITE   0x0008
#define EXTRA_HILITE  0x0010
#define FULL_HILITE   0x001F


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

#define MENU_BACK_TITLE     0
#define MENU_BACK_ITEM      1
#define MENU_BACK_HILITE    2
#define MENU_BACK_STIPPLE 	3
#define MENU_BACK_STYLES 	4

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
#define C_L1            (0x01<<13)
#define C_L2            (0x01<<14)
#define C_L3            (0x01<<15)
#define C_L4            (0x01<<16)
#define C_L5            (0x01<<17)
#define C_R1            (0x01<<18)
#define C_R2            (0x01<<19)
#define C_R3            (0x01<<20)
#define C_R4            (0x01<<21)
#define C_R5            (0x01<<22)
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
/*    Bool is_pressed;         is the button pressed? */
	unsigned int width, height ;
  }
button_t;

typedef button_t MyButton ;

typedef struct
{
  int flags, x, y;
  unsigned int width, height;
}
ASGeometry;

typedef struct ASRectangle
{
  int x, y;
  unsigned int width, height;
}ASRectangle;

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
									 StructureNotifyMask)


#define AS_FRAME_EVENT_MASK  		(ButtonPressMask 	| \
									 ButtonReleaseMask 	| \
									 EnterWindowMask 	| \
                                     FocusChangeMask 	| \
									 ExposureMask 		| \
									 LeaveWindowMask 	| \
									 SubstructureRedirectMask)

#define AS_CANVAS_EVENT_MASK 		(ButtonPressMask 	| \
									 ButtonReleaseMask 	| \
									 EnterWindowMask 	| \
									 LeaveWindowMask 	| \
									 StructureNotifyMask)

#define AS_ICON_TITLE_EVENT_MASK 	(ButtonPressMask 	| \
                                     ButtonReleaseMask 	| \
									 EnterWindowMask 	| \
									 FocusChangeMask 	| \
									 KeyPressMask )

#define AS_ICON_EVENT_MASK 	 		(ButtonPressMask 	| \
									 ButtonReleaseMask 	| \
									 EnterWindowMask 	| \
									 FocusChangeMask 	| \
								     KeyPressMask 		| \
									 LeaveWindowMask)

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


/* for each window that is on the display, one of these structures
 * is allocated and linked into a list
 */
typedef struct ASWindow
  {
    unsigned long magic ;
    Window           w;     /* the child window */
    Window           frame; /* the frame window */
#define get_window_frame(asw)   (asw->frame)

	struct ASHints       *hints;

    /* Window geometry:
     *  3 different settings: anchor, status and canvas
     * 1) anchor reflects anchor point of the client window according
     *    to its size and gravity and requested position. It is calculated in virtual coordinates
     *    for non-sticky windows. For example in case of SouthEastGravity anchor.x will be at the
     *    right edge of the frame, and anchor.y will be at the bottom edge of the frame.
     * 2) status reflects current size and position of the frame as calculated based upon anchor
     *    point and frame decorations size. It is always in real screen coordinates.
     *    status->viewport_x and status->viewport_y reflect viewport position at the time of such
     *    calculation. Whenever viewport changes - this two must be changed and status recalculated.
     * 3) canvases reflect window position as reported in last received ConfigureNotify event and
     *    reflect exact position of window on screen as viewed by user.
     *
     * Anchor is needed so we could handle changing size of the frame decorations
     * status represents desired geometry/state of the window
     * canvases represent factual geometry/state of the window
     */
	struct ASStatusHints *status;
	struct ASStatusHints *saved_status; /* status prior to maximization */
    XRectangle            anchor ;

    struct ASWindow      *transient_owner,  *group_lead ;
    struct ASVector      *transients,       *group_members ;


#define ASWIN_NAME(t)       ((t)->hints->names[0])
#define ASWIN_CLASS(t)      ((t)->hints->res_class)
#define ASWIN_RES_NAME(t)   ((t)->hints->res_name)
#define ASWIN_ICON_NAME(t)  ((t)->hints->icon_name)
#define ASWIN_DESK(t)       ((t)->status->desktop)
#define ASWIN_LAYER(t)      ((t)->status->layer)
#define ASWIN_HFLAGS(t,f) 	get_flags((t)->hints->flags,(f))
#define ASWIN_PROTOS(t,f) 	get_flags((t)->hints->protocols,(f))
#define ASWIN_GET_FLAGS(t,f) 	get_flags((t)->status->flags,(f))
#define ASWIN_SET_FLAGS(t,f)    set_flags((t)->status->flags,(f))
#define ASWIN_CLEAR_FLAGS(t,f) 	clear_flags((t)->status->flags,(f))
#define ASWIN_FUNC_MASK(t)  ((t)->hints->function_mask)

#define TITLE_BUTTONS		10
#define TITLE_BUTTONS_PERSIDE 5
#define FIRST_RIGHT_TBTN    TITLE_BUTTONS_PERSIDE
#define IsLeftButton(b) 	((b)<FIRST_RIGHT_TBTN)
#define IsRightButton(b) 	((b)>=FIRST_RIGHT_TBTN)
#define RightButtonIdx(b) 	((b)-FIRST_RIGHT_TBTN)

#define IsBtnDisabled(t,b)  get_flags((t)->hints->disabled_buttons,(0x01<<(b)))
#define IsBtnEnabled(t,b)   (!get_flags((t)->hints->disabled_buttons,(0x01<<(b))))
#define DisableBtn(t,b)  	clear_flags((t)->hints->disabled_buttons,(0x01<<(b)))

	/********************************************************************/
	/* ASWindow frame decorations :                                     */
	/********************************************************************/
	/* window frame decoration consists of :
	  Top level window
		  4 canvases - one for each side :
		  	  Top or left canvas contains titlebar+ adjusen frame side+corners if any
			  Bottom or right canvas contains sidebar which is the same as south frame side with corners
			  Remaining two canvasses contain east and west frame sides only ( if any );
		  Canvasses surround main window and its sizes are actually the frame size.
	 */

    struct ASCanvas   *frame_canvas ;
    struct ASCanvas   *client_canvas ;
    struct ASCanvas   *frame_sides[FRAME_SIDES] ;
    struct ASCanvas   *icon_canvas ;
	struct ASCanvas   *icon_title_canvas ; /* same as icon_canvas if !SeparateButtonTitle */

    struct MyFrame    *frame_data;  /* currently selected frame decorations for this window */

    struct ASTBarData *frame_bars[FRAME_PARTS] ; /* regular sidebar is the same as frame with S, SE and SW parts */
    struct ASTBarData *tbar ;                    /* same as frame_bars[FRAME_PARTS] for convinience */
    struct ASTBarData *icon_button ;
	struct ASTBarData *icon_title ;

	/********************************************************************/
	/* END of NEW ASWindow frame decorations                            */
	/********************************************************************/
    int old_bw;			/* border width before reparenting */
#ifndef NO_SHAPE
    int wShaped;		/* is this a shaped window */
#endif
    int warp_index;		/* index of the window in the window list,
                           changes when window is warped */

    int FocusDesk;      /* Where (if at all) was it focussed */
    int DeIconifyDesk;		/* Desk to deiconify to, for StubbornIcons */

    unsigned long flags;

    int orig_x;			/* unmaximized x coordinate */
    int orig_y;			/* unmaximized y coordinate */
    int orig_wd;		/* unmaximized window width */
    int orig_ht;		/* unmaximized window height */

    long focus_sequence;
    long circulate_sequence;
  }ASWindow;

typedef struct ASLayer
{
    int          layer ;
    Window       w ;
    struct ASVector    *members ;          /* vector of ASDesktopElems */
}ASLayer;

typedef struct ASWindowList
{
    struct ASBiDirList *clients ;
    struct ASHashTable *aswindow_xref;         /* xreference of window/resource IDs to ASWindow structures */
    struct ASHashTable *layers ;               /* list of ASLayer structures from above hashed by layer num */

    /* lists of pointers to the ASWindow structures */
    struct ASVector    *circulate_list ;
    struct ASVector    *sticky_list ;

    ASWindow *root ;         /* root window - special purpose ASWindow struct to
                              * enable root window handling same way as any other window */

    /* warping/circulation pointers : */
    int       warp_curr_index;              /* last warped to window */
    int       warp_init_dir;                /* initial warping direction */
    int       warp_curr_dir, warp_user_dir; /* current warping direction
                                             * - internal direction maybe different from
                                             *  user direction in YOYO mode */

    /* focus pointers : */
    ASWindow *active;        /* currently active client
                              *     - may not have focus during housekeeping operations
                              *     - may be different from hilited/ungrabbed if we changed active
                              *       client during house keeping operation */
    ASWindow *focused;       /* currently focused client. Will be NULL during housekeeping */
    ASWindow *ungrabbed;     /* client that has no grab on mouse events */
    ASWindow *hilited;       /* client who's frame decorations has been hilited
                              * to show that it is focused. May not be same as focused/ungrabbed
                              * right after focus is handed over and before FocusIn event */
    ASWindow *previous_active;        /* last active client */
    /* Focus management is somewhat tricky.
     * Firstly, we have to track pointer movements to see when mouse enters the window
     *          so we can switch focus to that window. ( or mouse clicked in the window
     *          where ClickToFocus is requested ). To Accomplish that we grab mouse
     *          events on all the windows but currently focused.
     * Secondly we need to hilite currently active window with frame decorations, when window
     *          gets focus. So right after focus is forced on window and before FocusIn event
     *          we'll have : (focused == ungrabbed) != hilited
     * Thirdly, we may need to steal focus for our own needs while we perform housekeeping
     *          like desk switching, etc. ( GrabEm/UngrabEm ) In such situation window
     *          remains hilited, but has no focus, and has no pointer event grabs.
     *          (focused == NULL) != (ungrabbed == hilited)
     * Fourthsly, When housekeeping is completed we want to give the focus back to it, but
     *          in some situations housekeeping operations will make us give the focus to
     *          some other window. To do that we substitute focused pointer to new window
     *          right before UngrabEm. In this case focused != ( ungrabbed == hilited )
     */
}ASWindowList;


/* from aswindow.c : */
ASWindowList *init_aswindow_list();
void destroy_aswindow_list( ASWindowList **list, Bool restore_root );
ASWindow *window2ASWindow( Window w );
Bool register_aswindow( Window w, ASWindow *asw );
Bool unregister_aswindow( Window w );
Bool destroy_registered_window( Window w );
ASWindow *pattern2ASWindow( const char *pattern );
ASLayer *get_aslayer( int layer, ASWindowList *list );
Bool enlist_aswindow( ASWindow *t );
void delist_aswindow( ASWindow *t );
void save_aswindow_list( ASWindowList *list, char *file );
void restack_window_list( int desk );
Bool is_window_obscured (ASWindow * above, ASWindow * below);
void restack_window( ASWindow *t, Window sibling_window, int stack_mode );
#define RaiseWindow(asw)    restack_window((asw),None,Above)
#define LowerWindow(asw)    restack_window((asw),None,Below)
#define RaiseObscuredWindow(asw)  restack_window((asw),None,TopIf)
#define RaiseLowerWindow(asw)     restack_window((asw),None,Opposite)


ASWindow     *get_next_window (ASWindow * curr_win, char *action, int dir);
ASWindow     *warp_aswindow_list ( ASWindowList *list, Bool backwards );


/* from add_window.c : */
void destroy_icon_windows( ASWindow *asw );
Bool get_icon_root_geometry( ASWindow *asw, ASRectangle *geom );

/* swiss army knife - does everything about grabbing : */
void grab_window_input( ASWindow *asw, Bool release_grab );

void hide_focus();
Bool focus_aswindow( ASWindow *asw, Bool circulated );
Bool focus_active_window();
void focus_next_aswindow( ASWindow *asw );     /* should be called when window is unmapped or destroyed */

void hide_hilite();                            /* unhilites currently highlited window */
void hilite_aswindow( ASWindow *asw );         /* actually hilites focused window on reception of event */
void warp_to_aswindow( ASWindow *asw, Bool deiconify );
Bool activate_aswindow( ASWindow *asw, Bool force, Bool deiconify );


void redecorate_window( ASWindow *asw, Bool free_resources );
void update_window_transparency( ASWindow *asw );
void on_window_moveresize( ASWindow *asw, Window w, int x, int y, unsigned int width, unsigned int height );
void on_icon_changed( ASWindow *asw );
void on_window_title_changed( ASWindow *asw, Bool update_display );
void on_window_status_changed( ASWindow *asw, Bool update_display, Bool reconfigured );
void on_window_hilite_changed( ASWindow *asw, Bool focused );
void on_window_pressure_changed( ASWindow *asw, int pressed_context );

Bool iconify_window( ASWindow *asw, Bool iconify );
Bool make_aswindow_visible( ASWindow *asw, Bool deiconify );
void change_aswindow_layer( ASWindow *asw, int layer );
void change_aswindow_desktop( ASWindow *asw, int new_desk );
void toggle_aswindow_status( ASWindow *asw, ASFlagType flags );

void SelectDecor (ASWindow *);
ASWindow *AddWindow (Window);
void Destroy (ASWindow *, Bool);
void RestoreWithdrawnLocation (ASWindow *, Bool);
void SetShape (ASWindow *, int);

/* from decorations.c :*/
void disable_titlebuttons_with_function (ASWindow * t, int function);


#define TITLE_OLD		0	/* old (NEXTSTEP 3) style titlebar */
#define TITLE_NEXT4		1	/* NEXTSTEP 4 style titlebar */

#endif /* #ifndef AFTERSTEP_H_HEADER_INCLUDED */
