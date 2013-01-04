/***********************************************************************
 * afterstep FEEL include file
 ***********************************************************************/
#ifndef ASFEEL_HEADER_FILE_INCLUDED
#define ASFEEL_HEADER_FILE_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* Cursor types */
enum ASCursorTypes
{	
	ASCUR_Position = 0,		/* upper Left corner cursor */
	ASCUR_Title,			/* title-bar cursor */
	ASCUR_Default,			/* cursor for apps to inherit */
	ASCUR_Sys,				/* sys-menu and iconify boxes cursor */
	ASCUR_Move,				/* resize cursor */
	ASCUR_Wait,				/* wait a while cursor */
	ASCUR_Menu,				/* menu cursor */
	ASCUR_Select,			/* dot cursor for f.move, etc. from menus */
	ASCUR_Destroy,			/* skull and cross bones, f.destroy */
	ASCUR_Top,
	ASCUR_Right,
	ASCUR_Bottom,
	ASCUR_Left,
	ASCUR_TopLeft,
	ASCUR_TopRight,
	ASCUR_BottomLeft,
	ASCUR_BottomRight,
	ASCUR_UsefulCursors
};

/* MENU SORT MODS : */
#define SORTBYALPHA 1
#define SORTBYDATE  2

typedef enum ASWinListSortOrderVals {
	ASO_Circulation = 0,
	ASO_Alpha,
	/* TODO: */
	ASO_Stacking,
	ASO_Creation,
	ASO_Default = ASO_Circulation
}ASWinListSortOrderVals;

struct name2const {
	const char*	name;
	const int	value;
};

/* for the flags value - these used to be separate Bool's */
typedef enum                /* feel file flags */
{
	ClickToFocus            = (1 << 0), /* Focus follows or click to focus */
	DecorateTransients      = (1 << 1), /* decorate transient windows? */
	DontMoveOff             = (1 << 2), /* make sure windows stay on desk */
	SuppressIcons           = (1 << 3), /* prevent generating icon win */
	StickyIcons             = (1 << 4), /* Icons always sticky? */
	EdgeWrapX               = (1 << 5), /* Should EdgeScroll wrap around? */
	EdgeWrapY               = (1 << 6),
	CenterOnCirculate       = (1 << 7), /* center window on circulate ? */
	KeepIconWindows         = (1 << 8),
	ClickToRaise            = (1 << 9),
	EatFocusClick           = (1 << 10),
	NoPPosition             = (1 << 11),
	CirculateSkipIcons      = (1 << 12),
	StubbornIcons           = (1 << 13),
	StubbornPlacement       = (1 << 14),
	StubbornIconPlacement   = (1 << 15),
	BackingStore            = (1 << 16),
	AppsBackingStore        = (1 << 17),
	SaveUnders              = (1 << 18),
	SloppyFocus             = (1 << 19),
	IconTitle               = (1 << 20),
	FollowTitleChanges      = (1 << 21),
	AutoTabThroughDesks     = (1 << 22),
	DoHandlePageing         = (1 << 23),
	DontRestoreFocus        = (1 << 24),
	PersistentMenus			= (1 << 25),
	DontAnimateBackground   = (1 << 26),
	WinListHideIcons	    = (1 << 27),
	AnimateDeskChange		= (1 << 28),
	DontCoverDesktop		= (1 << 29),
	WarpPointer		= (1 << 30)	
}FeelFlags;

/* since we have too many feel flags  - we want another enum for
  feel set flags, that will indicate which option is set ny user */
typedef enum			/* feel file set flags */
{
	FEEL_ClickTime              = (0x01<<0),
	FEEL_OpaqueMove             = (0x01<<1),
	FEEL_OpaqueResize           = (0x01<<2),
	FEEL_AutoRaise              = (0x01<<3),
	FEEL_AutoReverse            = (0x01<<4),
	FEEL_DeskAnimationType      = (0x01<<5),
	FEEL_DeskAnimationSteps     = (0x01<<6),

	FEEL_XorValue               = (0x01<<7),
	FEEL_Xzap                   = (0x01<<8),
	FEEL_Yzap                   = (0x01<<9),

	FEEL_ClickToRaise           = (0x01<<10),

	FEEL_EdgeScroll             = (0x01<<11),
	FEEL_EdgeResistance         = (0x01<<12),
	FEEL_ShadeAnimationSteps    = (0x01<<13),
	FEEL_ExternalMenus			= (0x01<<14),
	FEEL_EdgeResistanceToDragging = (0x01<<15)

}FeelSetValFlags;



typedef enum{
	ASP_SmartPlacement = 0,
	ASP_RandomPlacement,
	ASP_Tile,
	ASP_Cascade,
	ASP_UnderPointer,  
	ASP_Manual
}ASPlacementStrategy ;

typedef enum
{
	ASP_UseMainStrategy = 0,
	ASP_UseBackupStrategy
}ASUsePlacementStrategy ;

typedef struct ASWindowBox
{

#define ASA_Virtual				(0x01<<0)
#define ASA_ReverseOrderH		(0x01<<1)
#define ASA_ReverseOrderV		(0x01<<2)
#define ASA_ReverseOrder		(ASA_ReverseOrderH|ASA_ReverseOrderV)
#define ASA_VerticalPriority	(0x01<<3)
#define ASA_AreaSet				(0x01<<4)
#define ASA_MainStrategySet		(0x01<<5)
#define ASA_BackupStrategySet  	(0x01<<6)
#define ASA_MinWidthSet			(0x01<<7)
#define ASA_MinHeightSet		(0x01<<8)
#define ASA_MaxWidthSet			(0x01<<9)
#define ASA_MaxHeightSet		(0x01<<10)
#define ASA_DesktopSet          (0x01<<11)
#define ASA_MinLayerSet         (0x01<<12)
#define ASA_MaxLayerSet         (0x01<<13)
#define ASA_XSpacingSet         (0x01<<14)
#define ASA_YSpacingSet         (0x01<<15)

	ASFlagType 	set_flags;
	ASFlagType 	flags;

	char       *name ;
	ASGeometry  area ;                         /* could be aither screen region or virtual region */

	ASPlacementStrategy main_strategy;         /* how to place window in the empty space :Smart, Random Or Tile */
	ASPlacementStrategy backup_strategy ;      /* how to place window if no empty space of
												* sufficient size exists: Random, Cascade or Manual*/
	unsigned int min_width, min_height ;
	unsigned int max_width, max_height ;
	int min_layer, max_layer ;

	int desk ;

	int x_spacing, y_spacing ;
	/* this store status : */
	unsigned int cascade_pos ;                 /* gets incremented every time window is cascaded */

}ASWindowBox;

typedef enum {
	AST_OneDirection = 0,
	AST_ClosedLoop = 1,
	AST_OpenLoop = 2
}ASTabbingReverse;

typedef struct MouseButton
{
	int Button;
	int Context;
	int Modifier;
	struct MouseButton *NextButton;
	struct FunctionData *fdata;
}MouseButton;

typedef struct FuncKey
{
	struct FuncKey *next;	/* next in the list of function keys */
	char *name;			/* key name */
	KeyCode keycode;		/* X keycode */
	int cont;			/* context */
	int mods;			/* modifiers */

	struct FunctionData *fdata ;
}FuncKey;

typedef struct ASFeel
{
	unsigned long magic;

	unsigned long set_flags;  /* what options were set by user */
	unsigned long flags;      /* feel file flags */

	/* for compatibility with old configs : */
#define FEEL_DEPRECATED_RandomPlacement (0x01 << 0)  /* random windows placement */
#define FEEL_DEPRECATED_SmartPlacement	(0x01 << 1)  /* smart placement */
	ASFlagType 	deprecated_flags; 

	ASFlagType 	set_val_flags;                 /* see  enum FeelSetValFlags */

	unsigned int ClickTime;        /* Max buttonclickdelay for Function built-in */
	unsigned int OpaqueMove;       /* Keep showing window while being moved if size<N% */
	unsigned int OpaqueResize;     /* Keep showing window while being resized if size<N% */
	int AutoRaiseDelay;            /* Delay between setting focus and raisingwin */

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
	int EdgeAttractionScreen;      /* #pixels to scroll on screen edge */
	int EdgeAttractionWindow;      /* #pixels to scroll on screen edge */
	int EdgeResistanceMove;        /* #pixels to scroll on screen edge */
	int EdgeResistanceDragScroll;      /* #pixels to scroll on screen edge while dragging the window */
	int ShadeAnimationSteps;

	struct ASHashTable *Popups ;
	struct ASHashTable *ComplexFunctions ;

	struct MouseButton *MouseButtonRoot;
	struct FuncKey *FuncKeyRoot;

	Cursor    cursors[MAX_CURSORS];

	ASFlagType          buttons2grab ;           /* calculated after all the mouse bindings are set */
	  /* TODO: add no_snaping_mod to Feel parsing code : */
	unsigned int        no_snaping_mod ;         /* modifyer mask that disables snapping to edges whle move/resizing the window */

	ASWindowBox		   *window_boxes ;
	ASWindowBox        *default_window_box ;
	char               *default_window_box_name ;
	unsigned int        window_boxes_num ;

	unsigned int        recent_submenu_items;

	unsigned int        desk_cover_animation_steps ;
	unsigned int        desk_cover_animation_type ;

	int 				conserve_memory ;
	
	ASWinListSortOrderVals winlist_sort_order;
}ASFeel;



/***************************************************************************/
/*                        Function Prototypes                              */
/***************************************************************************/

void init_asfeel( ASFeel *feel );
ASFeel *create_asfeel ();
void destroy_asfeel( ASFeel *feel, Bool reusable );
void check_feel_sanity(ASFeel *feel);

void apply_context_cursor( Window w, ASFeel *feel, unsigned long context );
void recolor_feel_cursors( ASFeel *feel, CARD32 fore, CARD32 back );


ASWindowBox *create_aswindow_box( const char *name );
void destroy_aswindow_box( ASWindowBox *aswbox, Bool reusable );
void print_window_box (ASWindowBox *aswbox, int index);
ASWindowBox* find_window_box( ASFeel *feel, const char *name );


void init_list_of_menus(ASHashTable **list, Bool force);

#ifdef __cplusplus
}
#endif



#endif /* SFEEL_HEADER_FILE_INCLUDED */
