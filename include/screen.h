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

#define SIZE_HINDENT 5
#define SIZE_VINDENT 3
#define MAX_WINDOW_WIDTH 32767
#define MAX_WINDOW_HEIGHT 32767

/* Cursor types */
#ifndef POSITION
#define POSITION 0		/* upper Left corner cursor */
#endif
#define TITLE_CURSOR 1		/* title-bar cursor */
#define DEFAULT 2		/* cursor for apps to inherit */
#define SYS 3			/* sys-menu and iconify boxes cursor */
#define MOVE 4			/* resize cursor */
#if defined(__alpha)		/* Do you honnestly think people will even */
#ifdef WAIT			/* have to wait on alphas :-) */
#undef WAIT
#endif
#endif
#define WAIT 5			/* wait a while cursor */
#define MENU 6			/* menu cursor */
#define SELECT 7		/* dot cursor for f.move, etc. from menus */
#define DESTROY 8		/* skull and cross bones, f.destroy */
#define TOP 9
#define RIGHT 10
#define BOTTOM 11
#define LEFT 12
#define TOP_LEFT 13
#define TOP_RIGHT 14
#define BOTTOM_LEFT 15
#define BOTTOM_RIGHT 16
#define MAX_CURSORS 18

/* Maximum number of icon boxes that are allowed */
#define MAX_BOXES 4

/* button styles */
#define NO_BUTTON_STYLE		-1	/* button is undefined */
#define XPM_BUTTON_STYLE	2	/* button is a pixmap */

/* title bar text alignment  */
#define JUSTIFY_CENTER 0
#define JUSTIFY_LEFT   1
#define JUSTIFY_RIGHT  2

#ifndef NO_VIRTUAL
typedef struct
  {
    Window win;
    int isMapped;
  }
PanFrame;

#endif

/* MENU SORT MODS : */
#define SORTBYALPHA 1
#define SORTBYDATE  2

/* for the flags value - these used to be separate Bool's */
typedef enum                /* feel file flags */
{
    ClickToFocus            = (1 << 0), /* Focus follows or click to focus */
    DecorateTransients      = (1 << 1), /* decorate transient windows? */
    DontMoveOff             = (1 << 2), /* make sure windows stay on desk */
    RandomPlacement         = (1 << 3), /* random windows placement */
    SuppressIcons           = (1 << 4), /* prevent generating icon win */
    StickyIcons             = (1 << 5), /* Icons always sticky? */
    EdgeWrapX               = (1 << 6), /* Should EdgeScroll wrap around? */
    EdgeWrapY               = (1 << 7),
    CenterOnCirculate       = (1 << 8), /* center window on circulate ? */
    KeepIconWindows         = (1 << 9),
    ClickToRaise            = (1 << 10),
    EatFocusClick           = (1 << 11),
    MenusHigh               = (1 << 12),
    NoPPosition             = (1 << 13),
    SMART_PLACEMENT         = (1 << 14),
    CirculateSkipIcons      = (1 << 15),
    StubbornIcons           = (1 << 16),
    StubbornPlacement       = (1 << 17),
    StubbornIconPlacement   = (1 << 18),
    BackingStore            = (1 << 19),
    AppsBackingStore        = (1 << 20),
    SaveUnders              = (1 << 21),
    SloppyFocus             = (1 << 22),
    IconTitle               = (1 << 23),
    MWMFunctionHints        = (1 << 24),
    MWMDecorHints           = (1 << 25),
    MWMHintOverride         = (1 << 26),
    FollowTitleChanges      = (1 << 27),
    AutoTabThroughDesks     = (1 << 28),
    DoHandlePageing         = (1 << 29)
}FeelFlags;

/* look file flags */
typedef enum
{
    TxtrMenuItmInd      = (0x01 << 0),
    MenuMiniPixmaps     = (0x01 << 1),
    GradientText        = (0x01 << 2),
    TexturedHandle      = (0x01 << 3),
    TitlebarNoPush      = (0x01 << 4),
    IconNoBorder        = (0x01 << 5),
    SeparateButtonTitle = (0x01 << 6),    /* icon title is a separate window */
    DecorateFrames      = (0x01 << 7)
}LookFlags;


typedef enum
{
    AS_StateShutdown   = (0x01<<0),
    AS_StateRestarting = (0x01<<1)
}ASScreenStateFlags ;

typedef struct fr_sz
  {
    int w;
    int h;
  }
fr_sz;

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

typedef enum {
    AST_OneDirection = 0,
    AST_ClosedLoop = 1,
    AST_OpenLoop = 2
}ASTabbingReverse;

typedef struct ASFeel
{
    unsigned long magic;

    unsigned long set_flags;  /* what options were set by user */
    unsigned long flags;      /* feel file flags */

    unsigned int ClickTime;        /* Max buttonclickdelay for Function built-in */
    unsigned int OpaqueMove;       /* Keep showing window while being moved if size<N% */
    unsigned int OpaqueResize;     /* Keep showing window while being resized if size<N% */
    unsigned int AutoRaiseDelay;       /* Delay between setting focus and raisingwin */

#define FEEL_WARP_ONEWAY       0
#define FEEL_WARP_YOYO         1
#define FEEL_WARP_ENDLESS      2
    unsigned int AutoReverse;

    int XorValue ;
    int Xzap, Yzap;

    long RaiseButtons;     /* The buttons to do click-to-raise */

    int EdgeScrollX;      /* % of the screen width to scroll on screen edge */
    int EdgeScrollY;      /* % of the screen height to scroll on screen edge */
    int EdgeResistanceScroll;      /* #pixels to scroll on screen edge */
    int EdgeResistanceMove;      /* #pixels to scroll on screen edge */
    int ShadeAnimationSteps;

    struct ASHashTable *Popups ;
    struct ASHashTable *ComplexFunctions ;

    struct MouseButton *MouseButtonRoot;
    struct FuncKey *FuncKeyRoot;

    Cursor    cursors[MAX_CURSORS];

    ASFlagType          buttons2grab ;           /* calculated after all the mouse bindings are set */
      /* TODO: add no_snaping_mod to Feel parsing code : */
    unsigned int        no_snaping_mod ;         /* modifyer mask that disables snapping to edges whle move/resizing the window */
}
ASFeel;

typedef struct MyLook
{
    unsigned long magic;
    /* The following two things are mostly used by ASRenderer : */
    char         *name;         /* filename of the look */
    int           ref_count ;   /* number of times referenced */
    /* Actuall Look data : */
    unsigned long flags;
    /* the rest of the stuff is related to windows.
        Window gets values when its first initialized, and then we have
        to go and update it whenever we move it from one desk to another.
    */
    unsigned short look_id;
    unsigned short deskback_id_base ;

    /* update mystyle_fix_styles() and InitLook() if you add a style */
    struct ASHashTable *styles_list;  /* hash of MyStyles by name */
    struct ASHashTable *backs_list;   /* hash of MyBackgrounds   by name */

    struct MyStyle *MSMenu[MENU_BACK_STYLES]; /* menu MyStyles */

    /* focussed, unfocussed, sticky window styles */
    struct MyStyle *MSWindow[BACK_STYLES];

    ASGeometry resize_move_geometry;  /* position of the size window */

    char *DefaultIcon;      /* Icon to use when no other icons are found */

    struct icon_t *MenuArrow;
    struct icon_t *MenuPinOn;
    struct icon_t *MenuPinOff;

    MyButton    buttons[TITLE_BUTTONS];

    /* Menu parameters */
    int DrawMenuBorders;
    int StartMenuSortMode;

    /* Icons parameters */
    ASGeometry *configured_icon_areas ;
	unsigned int configured_icon_areas_num ;
    unsigned int ButtonWidth, ButtonHeight;

    /* misc stuff */
    int RubberBand;

    /* these affect window placement : */
    /* None of it should be used by placement related functions from this
        structure - they should consult copies of it stored in ASWindow
        structure.
    */
    int TitleButtonStyle;     /* 0 - afterstep, 1 - WM old, 2 - free hand */
    int TitleButtonSpacing;
    int TitleButtonXOffset;
    int TitleButtonYOffset;
    int TitleTextAlign;       /* alignment of title bar text */

    struct ASSupportedHints *supported_hints;

    struct MyFrame *DefaultFrame;
    struct ASHashTable *FramesList ;/* hash table of named MyFrames */

}MyLook;



typedef struct ScreenInfo
  {
    ASFlagType    state ;   /* shutting down, restarting, etc. */
    unsigned long screen;
    int d_depth;            /* copy of DefaultDepth(dpy, screen) */
    int NumberOfScreens;	/* number of screens on display */
    int MyDisplayWidth;		/* my copy of DisplayWidth(dpy, screen) */
    int MyDisplayHeight;	/* my copy of DisplayHeight(dpy, screen) */

    Bool localhost ;
    char *rdisplay_string, *display_string;

    struct ASWMProps    *wmprops;              /* window management properties */

	struct ASVisual *asv ;  /* ASVisual for libAfterImage */
	struct ASImage  *RootImage;

    struct ASWindowList *Windows ;
/*    ASWindow ASRoot;        the head of the afterstep window list */
/*    struct ASHashTable *aswindow_xref;        xreference of window/resource IDs to ASWindow structures */

    struct ASIconBox   *default_icon_box ; /* if we have icons following desktops - then we only need one icon box */
	struct ASHashTable *icon_boxes ; /* hashed by desk no - one icon box per desktop ! */

    Window Root;		/* the root window */
    Window SizeWindow;		/* the resize dimensions window */
    Window NoFocusWin;		/* Window which will own focus when no other
				 * windows have it */
#ifndef NO_VIRTUAL
    PanFrame PanFrameTop, PanFrameLeft, PanFrameRight, PanFrameBottom;
    int usePanFrames;		/* toggle to disable them */
#endif

    int randomx;        /* values used for randomPlacement */
    int randomy;
    unsigned VScale;		/* Panner scale factor */
    int VxMax;			/* Max location for top left of virt desk */
    int VyMax;
    int Vx;			/* Current loc for top left of virt desk */
    int Vy;

    int CurrentDesk;        /* The current desktop number */

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
}ScreenInfo;

extern ScreenInfo Scr;

void init_screen_gcs(ScreenInfo *scr);
void make_screen_envvars( ScreenInfo *scr );
#ifdef HAVE_XINERAMA
void get_Xinerama_rectangles (ScreenInfo * scr);
#endif
Bool set_synchronous_mode (Bool enable);
int ConnectX (ScreenInfo * scr, char *display_name, unsigned long event_mask);
void setup_modifiers ();






#endif /* _SCREEN_ */
