#ifndef CLIENTPROPS_H_HEADER_INCLUDED
#define CLIENTPROPS_H_HEADER_INCLUDED

#include <X11/Xresource.h>

#ifdef __cplusplus
extern "C" {
#endif


struct ASDatabase;
struct ASDatabaseRecord;
struct ASStatusHints;
struct ScreenInfo;
struct AtomXref;


typedef enum
{
    HINTS_ICCCM = 0,
    HINTS_GroupLead,
	HINTS_Transient,
	HINTS_Motif,
    HINTS_Gnome,
	HINTS_KDE,
    HINTS_ExtendedWM,
    HINTS_XResources,
    HINTS_ASDatabase,
    HINTS_Supported
}HintsTypes;

#if 0
/***********************************************************/
/* AS Startup flags :                                      */
#define AS_StartPosition        (1<<0)
#define AS_StartPositionUser    (1<<1)
#define AS_Position 		    (1<<1)
#define AS_StartSize			(1<<2)
#define AS_Size					(1<<2)
#define AS_StartSizeUser		(1<<3)
#define AS_StartBorderWidth     (1<<4)
#define AS_BorderWidth     		(1<<4)
/* Viewport cannot be changed after window is mapped : */
#define AS_StartViewportX       (1<<5)
#define AS_StartViewportY       (1<<6)
#define AS_StartDesktop         (1<<7)
#define AS_Desktop 		        (1<<7)
#define AS_StartLayer			(1<<8)
#define AS_Layer				(1<<8)
/* the following are flags identifying client's status : */
#define AS_StartsIconic         (1<<9)
#define AS_Iconic               (1<<9)
#define AS_StartsMaximizedX     (1<<10)
#define AS_MaximizedX           (1<<10)
#define AS_StartsMaximizedY     (1<<11)
#define AS_MaximizedY           (1<<11)
#define AS_StartsSticky         (1<<12)
#define AS_Sticky               (1<<12)
#define AS_StartsShaded         (1<<13)
#define AS_Shaded               (1<<13)
/* special state - client withdrawn itself */
#define AS_Withdrawn            (1<<14)
#define AS_Dead                 (1<<15) /* dead client - has been destroyd or about to be destroyed */
/* special state - client is mapped - there is a small gap between MapRequest+XMapWindow and MapNotify event */
#define AS_Mapped               (1<<16)
#define AS_IconMapped           (1<<17)

/***********************************************************/
/* General flags                                           */
#define AS_MinSize				(1<<0)
#define AS_MaxSize				(1<<1)
#define AS_SizeInc				(1<<2)
#define AS_Aspect				(1<<3)
#define AS_BaseSize				(1<<4)
#define AS_Gravity				(1<<5)
#define AS_PID					(1<<6)
#define AS_Transient 			(1<<7)
#define AS_AcceptsFocus			(1<<8)
#define AS_ClickToFocus			(1<<9)
#define AS_Titlebar				(1<<10)
#define AS_VerticalTitle		(1<<11)
#define AS_Border				(1<<12)
#define AS_Handles				(1<<13)
#define AS_Frame				(1<<14)
#define AS_SkipWinList          (1<<15)
#define AS_DontCirculate        (1<<16)
#define AS_AvoidCover           (1<<17)
#define AS_IconTitle            (1<<18)
#define AS_Icon                 (1<<19)
#define AS_ClientIcon           (1<<20)
#define AS_ClientIconPixmap     (1<<21)
#define AS_ClientIconPosition   (1<<22)

/***********************************************************/
/* AS supported protocols :                                */
#define AS_DoesWmTakeFocus 		(1<<0)
#define AS_DoesWmDeleteWindow 	(1<<1)
#define AS_DoesWmPing           (1<<2)
#define AS_NeedsVisibleName		(1<<3)  /* only if window has _NET_WM_NAME hint */

/***********************************************************/
/* AS function masks :                                     */
#define AS_FuncPopup            (1<<0)
#define AS_FuncMinimize         (1<<1)
#define AS_FuncMaximize         (1<<2)
#define AS_FuncResize           (1<<3)
#define AS_FuncMove             (1<<4)
#define AS_FuncClose            (1<<7)
#define AS_FuncKill             (1<<8)

/***********************************************************/
/* AS layers :                                             */
#define AS_LayerDesktop         -10000  /* our desktop - just for the heck of it */
#define AS_LayerOtherDesktop    -2      /* for all those other file managers, KDE, GNOME, etc. */
#define AS_LayerBack            -1      /* normal windows below */
#define AS_LayerNormal           0      /* normal windows */
#define AS_LayerTop              1      /* normal windows above */
#define AS_LayerService          2      /* primarily for Wharfs, etc. */
#define AS_LayerUrgent           3      /* for modal dialogs that needs urgent answer (System Modal)*/
#define AS_LayerOtherMenu        4      /* for all those other menus - KDE, GNOME, etc. */
#define AS_LayerMenu             10000  /* our menu  - can't go wrong with that */

#define ASHINTS_STATIC_DATA 	 28     /* number of elements below that are not */
                                        /* dynamic arrays */

#endif
/************************************************************************/
/*		globals (atom IDs)					*/
/************************************************************************/
extern Atom _XA_WM_PROTOCOLS;
extern Atom _XA_WM_TAKE_FOCUS;
extern Atom _XA_WM_DELETE_WINDOW;
extern Atom _XA_WM_COLORMAP_WINDOWS;
extern Atom _XA_WM_STATE;

/* Motif hints */
extern Atom _XA_MwmAtom;

/* Gnome hints */
extern Atom _XA_WIN_LAYER;
extern Atom _XA_WIN_STATE;
extern Atom _XA_WIN_WORKSPACE;
extern Atom _XA_WIN_HINTS;

/* wm-spec _NET hints : */
extern Atom _XA_NET_WM_NAME;
extern Atom _XA_NET_WM_ICON_NAME;

extern Atom _XA_NET_WM_VISIBLE_NAME;
extern Atom _XA_NET_WM_VISIBLE_ICON_NAME;

extern Atom _XA_NET_WM_DESKTOP;
extern Atom _XA_NET_WM_WINDOW_TYPE;
extern Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP;
extern Atom _XA_NET_WM_WINDOW_TYPE_DOCK;
extern Atom _XA_NET_WM_WINDOW_TYPE_TOOLBAR;
extern Atom _XA_NET_WM_WINDOW_TYPE_MENU;
extern Atom _XA_NET_WM_WINDOW_TYPE_DIALOG;
extern Atom _XA_NET_WM_WINDOW_TYPE_NORMAL;
extern Atom _XA_NET_WM_WINDOW_TYPE_UTILITY;
extern Atom _XA_NET_WM_WINDOW_TYPE_SPLASH;
extern Atom _XA_AS_WM_WINDOW_TYPE_MODULE;

extern Atom _XA_NET_WM_STATE;
extern Atom _XA_NET_WM_STATE_MODAL;
extern Atom _XA_NET_WM_STATE_STICKY;
extern Atom _XA_NET_WM_STATE_MAXIMIZED_VERT;
extern Atom _XA_NET_WM_STATE_MAXIMIZED_HORZ;
extern Atom _XA_NET_WM_STATE_SHADED;
extern Atom _XA_NET_WM_STATE_SKIP_TASKBAR;
extern Atom _XA_NET_WM_STATE_SKIP_PAGER;
extern Atom _XA_NET_WM_STATE_HIDDEN;
extern Atom _XA_NET_WM_STATE_FULLSCREEN;
extern Atom _XA_NET_WM_STATE_ABOVE;
extern Atom _XA_NET_WM_STATE_BELOW;
extern Atom _XA_NET_WM_STATE_DEMANDS_ATTENTION;

#define MAX_NET_WM_STATES   12

extern Atom _XA_NET_WM_PID;
extern Atom _XA_NET_WM_PING;
extern Atom _XA_NET_WM_ICON;

extern Atom _XA_NET_WM_WINDOW_OPACITY;
#define NET_WM_WINDOW_OPACITY_OPAQUE		0xffffffff

extern Atom _XA_AS_MOVERESIZE;

extern struct AtomXref      *EXTWM_State;

#define IsNameProp(a) \
        ((a) == XA_WM_NAME ||  \
         (a) == XA_WM_ICON_NAME || \
         (a) == XA_WM_CLASS ||  \
         (a) == _XA_NET_WM_NAME || \
         (a) == _XA_NET_WM_ICON_NAME || \
         (a) == _XA_NET_WM_VISIBLE_NAME || \
         (a) == _XA_NET_WM_VISIBLE_ICON_NAME )

/* must not track state properties, since we manage it ourselves !! */
#define NeedToTrackPropChanges(a) \
   (((a)== XA_WM_HINTS  )|| \
    ((a)== XA_WM_NORMAL_HINTS  )|| \
    ((a)== XA_WM_TRANSIENT_FOR  )|| \
    ((a)== XA_WM_COMMAND  )|| \
    ((a)== XA_WM_CLIENT_MACHINE  )|| \
    ((a)== _XA_WM_PROTOCOLS  )|| \
    ((a)== _XA_WM_COLORMAP_WINDOWS  )|| \
    ((a)== _XA_MwmAtom  )|| \
    ((a)== _XA_WIN_HINTS  )|| \
    ((a)== _XA_NET_WM_WINDOW_TYPE  ))

/* Crossreferences of atoms into flag value for
   different atom list type of properties :*/


/************************************************************************/
/* 		ICCCM window hints		 			*/
/************************************************************************/

/* our own enchancement to XWMHints flags : */
#define IconWindowIsChildHint	(1L << 14)


/*
All of this stuff is part of standard Xlib so we put it into comments -
no need to redefine it.

This are placed By Client :
 WM_NAME                 TEXT
 WM_CLASS   		 XClassHint
    res_class
    res_name
 WM_ICON_NAME            TEXT
 WM_CLIENT_MACHINE       TEXT

 WM_HINTS                XWMHints   	32
    flags 	         CARD32
	USPosition
	USSize
	PPosition
	PSize
	PMinSize
	PMaxSize
	PResizeInc
	PAspect
	PBaseSize
	PWinGravity
    pad 	         4*CARD32
    max_width            INT32
    max_height           INT32
    width_inc 	         INT32
    height_inc           INT32
    max_aspect           (INT32,INT32)
    base_width           INT32
    base_height          INT32
    win_gravity
	NorthWest
	North
	NorthEast
	West
	Center
	East
	SouthWest
	South
	SouthEast
	Static

 WM_NORMAL_HINTS         XSizeHints 	32
    flags 	         CARD32
	InputHint
	StateHint
	IconPixmapHint
	IconWindowHint
	IconPositionHint
	IconMaskHint
	WindowGroupHint
	MessageHint       (this bit is obsolete)
	UrgencyHint
    input 	         CARD32
	False - if Globally Active and No Input
	True  - Passive and Locally Active
    initial_state        CARD32
	WithdrawnState   0
	NormalState      1
	IconicState      3
    icon_pixmap          PIXMAP
    icon_window          WINDOW
    icon_mask            PIXMAP
  Ignored :
    icon_x 		 INT32
    icon_y 	         INT32

 WM_PROTOCOLS            ATOM 		32 (list of atoms)
    WM_TAKE_FOCUS
    WM_DELETE_WINDOW

 WM_TRANSIENT_FOR        WINDOW         32
 WM_COLORMAP_WINDOWS     WINDOW     	32 (List of windows)

This are placed By Window Manager :
 WM_STATE                WM_STATE       32
    state		 CARD32
	WithdrawnState   0
	NormalState      1
	IconicState      3
    icon 		 WINDOW

 WM_ICON_SIZE            XIconSize
    max_width            CARD32
    max_height           CARD32
    width_inc            CARD32
    height_inc           CARD32
*/

#ifndef 	UrgencyHint
# ifndef 	XUrgencyHint
#  define 	UrgencyHint		256
# else
#  define 	UrgencyHint		XUrgencyHint
# endif
#endif


/************************************************************************/
/* 		Motif WM window hints					*/
/************************************************************************/
/* This are placed By Client : */

typedef struct MwmHints
{
  CARD32 flags;         /* window hints */
  CARD32 functions;     /* requested functions */
  CARD32 decorations;   /* requested decorations */
  INT32  inputMode;     /* input mode */
  CARD32 status;        /* status (ignored) */
}MwmHints;

/* Motif WM window hints */
#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)

/* Motif WM function flags */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)
#define MWM_FUNC_EVERYTHING     (MWM_FUNC_RESIZE|MWM_FUNC_MOVE|MWM_FUNC_MINIMIZE|MWM_FUNC_MAXIMIZE|MWM_FUNC_CLOSE)

/* Motif WM decor flags */
#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)
#define MWM_DECOR_EVERYTHING    (MWM_DECOR_BORDER|MWM_DECOR_RESIZEH|MWM_DECOR_TITLE|MWM_DECOR_MENU|MWM_DECOR_MINIMIZE|MWM_DECOR_MAXIMIZE)

/* Motif WM input modes */
#define MWM_INPUT_MODELESS 			0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 	1
#define MWM_INPUT_SYSTEM_MODAL 			2
#define MWM_INPUT_FULL_APPLICATION_MODAL 	3
#define MWM_INPUT_APPLICATION_MODAL 		MWM_INPUT_PRIMARY_APPLICATION_MODAL

#define PROP_MOTIF_WM_HINTS_ELEMENTS  5
#define PROP_MWM_HINTS_ELEMENTS       PROP_MOTIF_WM_HINTS_ELEMENTS

/************************************************************************/
/* 		OldGnome WM window hints 				*/
/************************************************************************/

/*
    Client hints include :
    _WIN_LAYER
    _WIN_STATE
    _WIN_WORKSPACE
    _WIN_HINTS
    Unsupported :
    _WIN_EXPANDED_SIZE
*/

/*_WIN_LAYER is also a CARDINAL that is the stacking layer the application wishes to exist in.
  The values for this property are:    */
#define WIN_LAYER_DESKTOP       0
#define WIN_LAYER_BELOW         2
#define WIN_LAYER_NORMAL        4
#define WIN_LAYER_ONTOP         6
#define WIN_LAYER_DOCK          8
#define WIN_LAYER_ABOVE_DOCK   10
#define WIN_LAYER_MENU         12

/* The bitmask for _WIN_STATE is as follows: */
#define WIN_STATE_STICKY          (1<<0)	/*everyone knows sticky */
#define WIN_STATE_MINIMIZED       (1<<1)	/*Reserved - definition is unclear */
#define WIN_STATE_MAXIMIZED_VERT  (1<<2)	/*window in maximized V state */
#define WIN_STATE_MAXIMIZED_HORIZ (1<<3)	/*window in maximized H state */
#define WIN_STATE_HIDDEN          (1<<4)	/*not on taskbar but window visible */
#define WIN_STATE_SHADED          (1<<5)	/*shaded (MacOS / Afterstep style) */
#define WIN_STATE_HID_WORKSPACE   (1<<6)	/*not on current desktop */
#define WIN_STATE_HID_TRANSIENT   (1<<7)	/*owner of transient is hidden */
#define WIN_STATE_FIXED_POSITION  (1<<8)	/*window is fixed in position even */
#define WIN_STATE_ARRANGE_IGNORE  (1<<9)	/*ignore for auto arranging */

/* The bitmask for _WIN_HINTS is as follows: */
#define WIN_HINTS_SKIP_FOCUS      (1<<0)	/*"alt-tab" skips this win */
#define WIN_HINTS_SKIP_WINLIST    (1<<1)	/*do not show in window list */
#define WIN_HINTS_SKIP_TASKBAR    (1<<2)	/*do not show on taskbar */
#define WIN_HINTS_GROUP_TRANSIENT (1<<3)	/*Reserved - definition is unclear */
#define WIN_HINTS_FOCUS_ON_CLICK  (1<<4)	/*app only accepts focus if clicked */

typedef struct GnomeHints
{
#define GNOME_LAYER		(1<<0)
#define GNOME_STATE		(1<<1)
#define GNOME_WORKSPACE		(1<<2)
#define GNOME_HINTS		(1<<3)
#define GNOME_EXP_SIZE		(1<<4)
  ASFlagType flags;

  CARD32 layer;
  CARD32 state;
  CARD32 workspace;
  CARD32 hints;
  /* Unsupported : */
  CARD32 expanded_size[4];

}
GnomeHints;

/************************************************************************/
/*	New Gnome-KDE WM compatibility window hints			*/
/*      see: http://www.freedesktop.org/standards/wm-spec/      	*/
/************************************************************************/
/*
  Client window properties :
    _NET_WM_NAME		UTF-8
    _NET_WM_VISIBLE_NAME_STRING UTF-8
    _NET_WM_DESKTOP		CARDINAL	32
    _NET_WM_WINDOW_TYPE		list of Atoms :
	_NET_WM_WINDOW_TYPE_DESKTOP
	_NET_WM_WINDOW_TYPE_DOCK
	_NET_WM_WINDOW_TYPE_TOOLBAR
	_NET_WM_WINDOW_TYPE_MENU
	_NET_WM_WINDOW_TYPE_DIALOG
	_NET_WM_WINDOW_TYPE_NORMAL
    _NET_WM_STATE		list of Atoms :
	_NET_WM_STATE_MODAL
	_NET_WM_STATE_STICKY
	_NET_WM_STATE_MAXIMIZED_VERT
	_NET_WM_STATE_MAXIMIZED_HORZ
	_NET_WM_STATE_SHADED
	_NET_WM_STATE_SKIP_TASKBAR

    _NET_WM_PID

 Unsupported :
    _NET_WM_STRUT
    _NET_WM_HANDLED_ICONS
    _NET_WM_ICON_GEOMETRY
    _NET_WM_ICON

 Protocols :
    _NET_WM_PING
 	_NET_WM_ICON

*/

typedef struct ExtendedWMHints
{
#define EXTWM_NAME              (0x01<<0)
#define EXTWM_ICON_NAME         (0x01<<1)
#define EXTWM_VISIBLE_NAME      (0x01<<2)
#define EXTWM_VISIBLE_ICON_NAME (0x01<<3)
#define EXTWM_DESKTOP           (0x01<<4)

#define EXTWM_TypeSet	        (0x01<<5)
#define EXTWM_StateSet	        (0x01<<6)
#define EXTWM_ICON              (0x01<<7)
#define EXTWM_PID               (0x01<<8)
#define EXTWM_DoesWMPing        (0x01<<9)
#define EXTWM_WINDOW_OPACITY    (0x01<<10)

/* actions to be performed on window state upon client request :*/
#define EXTWM_StateRemove   0    /* remove/unset property */
#define EXTWM_StateAdd      1    /* add/set property */
#define EXTWM_StateToggle   2    /* toggle property  */

  ASFlagType flags;
  
#define EXTWM_TypeDesktop       (0x01<<0)
#define EXTWM_TypeDock          (0x01<<1)
#define EXTWM_TypeToolbar       (0x01<<2)
#define EXTWM_TypeMenu          (0x01<<3)
#define EXTWM_TypeDialog        (0x01<<4)
#define EXTWM_TypeNormal        (0x01<<5)
#define EXTWM_TypeUtility       (0x01<<6)
#define EXTWM_TypeSplash        (0x01<<7)
#define EXTWM_TypeASModule      (0x01<<8)
#define EXTWM_TypeEverything    (EXTWM_TypeDesktop|EXTWM_TypeDock|EXTWM_TypeToolbar| \
                                 EXTWM_TypeMenu|EXTWM_TypeDialog|EXTWM_TypeNormal| \
								 EXTWM_TypeUtility|EXTWM_TypeSplash|EXTWM_TypeASModule)

  ASFlagType type_flags;

#define EXTWM_StateModal        (0x01<<16)
#define EXTWM_StateSticky       (0x01<<17)
#define EXTWM_StateMaximizedV   (0x01<<18)
#define EXTWM_StateMaximizedH   (0x01<<19)
#define EXTWM_StateShaded       (0x01<<20)
#define EXTWM_StateSkipTaskbar  (0x01<<21)
#define EXTWM_StateSkipPager    (0x01<<22)
#define EXTWM_StateHidden    	(0x01<<23)
#define EXTWM_StateFullscreen	(0x01<<24)
#define EXTWM_StateAbove	 	(0x01<<25)
#define EXTWM_StateBelow	 	(0x01<<26)
#define EXTWM_StateDemandsAttention	(0x01<<27)
#define EXTWM_StateEverything   (EXTWM_StateModal|EXTWM_StateSticky|EXTWM_StateMaximizedV| \
                                 EXTWM_StateMaximizedH|EXTWM_StateShaded| \
                                 EXTWM_StateSkipTaskbar|EXTWM_StateSkipPager | \
								 EXTWM_StateHidden|EXTWM_StateFullscreen| \
								 EXTWM_StateAbove|EXTWM_StateBelow|EXTWM_StateDemandsAttention)

  ASFlagType state_flags;
  
  XTextProperty *name;
  XTextProperty *icon_name;
  XTextProperty *visible_name;
  XTextProperty *visible_icon_name;
  CARD32 desktop;
  CARD32 pid;
  CARD32 *icon;
  long icon_length;
  CARD32 window_opacity ;
}
ExtendedWMHints;

typedef struct KDEHints
{
#define KDE_DesktopWindow              (0x01<<0)
#define KDE_SysTrayWindowFor           (0x01<<1)
	
	ASFlagType flags ;
	Window systray_window_for;
}KDEHints;

/***********************************************************************/
/*    AfterStep structure to collect all the hints set on window :     */
/***********************************************************************/

typedef struct ASParentHints
{ /* this contains all the information about window that has governing relations
   * to us. For eaxmple window which we are transient for, or leader of the group
   */
	Window  parent ;
    ASFlagType flags ;
	int 	desktop ;
	int viewport_x, viewport_y ;

}ASParentHints;

typedef Bool (*get_parent_hints_func)(Window parent, ASParentHints *dst );

/* use this function to override default function : */
get_parent_hints_func set_parent_hints_func(get_parent_hints_func new_func);

#define HINT_NAME               (1<<0)
#define HINT_PROTOCOL           (1<<1)
#define HINT_COLORMAP           (1<<2)
#define HINT_GENERAL            (1<<3)
#define HINT_STARTUP            (1<<4)
#define HINT_ANY                ASFLAGS_EVERYTHING

typedef struct ASRawHints
{
  struct ScreenInfo 	*scr;
  /* ICCCM hints : */
  XTextProperty *wm_name;
  XTextProperty *wm_icon_name;
  XClassHint    *wm_class;
  ASRectangle    placement ;
  unsigned int   border_width;
  XWMHints      *wm_hints;
  ASParentHints *group_leader ;
  XSizeHints    *wm_normal_hints;
  ASParentHints *transient_for;

  ASFlagType     wm_protocols;
  CARD32        *wm_cmap_windows;
  long           wm_cmap_win_count;

  XTextProperty *wm_client_machine ;    /* hostname of the computer on which client was executed */
  int            wm_cmd_argc ;
  char         **wm_cmd_argv ;

  INT32          wm_state ;
  CARD32         wm_state_icon_win ;

  /* Motif Hints : */
  MwmHints      *motif_hints;

  /* Gnome Hints : */
  GnomeHints     gnome_hints;   /* see gnome_hints.flags for what's actually set */

  /* WM-specs (new Gnome) Hints : */
  ExtendedWMHints extwm_hints;
  KDEHints	kde_hints;

  ASFlagType  hints_types ;

}
ASRawHints;

/*************************************************************************/
/*                           Interface                                   */
/*************************************************************************/
/* low level */
CARD32 read_extwm_desktop_val ( Window w);


/* X Resources : */
void init_xrm();
void load_user_database();
void destroy_user_database();
Bool read_int_resource( XrmDatabase db, const char *res_name, const char*res_class, int *value );

/* High level */
void intern_hint_atoms ();
ASRawHints *collect_hints (struct ScreenInfo *scr, Window w, ASFlagType what, ASRawHints *reusable_memory);
void destroy_raw_hints( ASRawHints *hints, Bool reusable );
void clientprops_cleanup ();

void read_wm_protocols (ASRawHints * hints, Window w);

/* printing functions :
 * if func and stream are not specified - fprintf(stderr) is used ! */
void print_wm_hints         ( stream_func func, void* stream, XWMHints *hints );
void print_wm_normal_hints  ( stream_func func, void* stream, XSizeHints *hints );
void print_motif_hints      ( stream_func func, void* stream, MwmHints *hints );
void print_gnome_hints      ( stream_func func, void* stream, GnomeHints *hints );
void print_extwm_hints      ( stream_func func, void* stream, ExtendedWMHints *hints );
void print_hints            ( stream_func func, void* stream, ASRawHints *hints );


/*************************************************************************/
Bool handle_client_property_update ( Window w, Atom property, ASRawHints *raw );
Bool handle_manager_property_update( Window w, Atom property, ASRawHints *raw );

Bool get_extwm_state_flags (Window w, ASFlagType *flags);


/*************************************************************************/
/****************** Setting properties - updating hints : ****************/
/*************************************************************************/
void set_client_state( Window w, struct ASStatusHints *status );
void set_extwm_urgency_state (Window w, Bool set );
void set_client_desktop( Window w, int ext_desk );
void set_client_names( Window w, char *name, char *icon_name, char *res_class, char *res_name );
void set_client_protocols (Window w, ASFlagType protocols, ASFlagType extwm_protocols);
void set_extwm_hints( Window w, ExtendedWMHints *extwm_hints );
void set_gnome_hints( Window w, GnomeHints *gnome_hints );

void set_client_hints( Window w, XWMHints *hints, XSizeHints *size_hints, ASFlagType protocols,
				  	   ExtendedWMHints *extwm_hints );
void set_client_cmd (Window w);


void send_wm_protocol_request (Window w, Atom request, Time timestamp);

/*************************************************************************/
/********************************THE END**********************************/
/*************************************************************************/
#ifdef __cplusplus
}
#endif


#endif  /* CLIENTPROPS_H_HEADER_INCLUDED */
