/***********************************************************************
 * afterstep FEEL include file
 ***********************************************************************/
#ifndef ASFEEL_HEADER_FILE_INCLUDED
#define ASFEEL_HEADER_FILE_INCLUDED

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


/* MENU SORT MODS : */
#define SORTBYALPHA 1
#define SORTBYDATE  2

enum {
    ASO_Circulation = 0,
    /* TODO: */
    ASO_Stacking,
    ASO_Creation
}ASWinListSortOrder;

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
    FollowTitleChanges      = (1 << 24),
    AutoTabThroughDesks     = (1 << 25),
    DoHandlePageing         = (1 << 26),
    DontRestoreFocus        = (1 << 27),
	PersistentMenus			= (1 << 28),
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
	FEEL_ExternalMenus			= (0x01<<14)
}FeelSetValFlags;



typedef enum{
    ASP_SmartPlacement = 0,
	ASP_RandomPlacement,
	ASP_Tile,
	ASP_Cascade,
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
#define ASA_ReverseOrder		(0x01<<1)
#define ASA_VerticalPriority	(0x01<<2)
#define ASA_AreaSet				(0x01<<3)
#define ASA_MainStrategySet		(0x01<<4)
#define ASA_BackupStrategySet  	(0x01<<5)
#define ASA_MinWidthSet			(0x01<<6)
#define ASA_MinHeightSet		(0x01<<7)
#define ASA_MaxWidthSet			(0x01<<8)
#define ASA_MaxHeightSet		(0x01<<9)
#define ASA_DesktopSet          (0x01<<10)
#define ASA_MinLayerSet         (0x01<<11)
#define ASA_MaxLayerSet         (0x01<<12)

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
}
ASFeel;



/***************************************************************************/
/*                        Function Prototypes                              */
/***************************************************************************/

void init_asfeel( ASFeel *feel );
ASFeel *create_asfeel ();
void destroy_asfeel( ASFeel *feel, Bool reusable );
void check_feel_sanity(ASFeel *feel);


ASWindowBox *create_aswindow_box( const char *name );
void destroy_aswindow_box( ASWindowBox *aswbox, Bool reusable );
void print_window_box (ASWindowBox *aswbox, int index);
ASWindowBox* find_window_box( ASFeel *feel, const char *name );


void init_list_of_menus(ASHashTable **list, Bool force);

#endif /* SFEEL_HEADER_FILE_INCLUDED */
