/***********************************************************************
 *
 * afterstep per-screen data include file
 *
 ***********************************************************************/

#ifndef MY_LOOK_HEADER_INCLUDED
#define MY_LOOK_HEADER_INCLUDED

#include "../libAfterImage/asvisual.h"
#include "../libAfterImage/asimage.h"

struct MyStyle;
struct ASSupportedHints;
struct MyFrame;
struct ASHashTable;
struct ScreenInfo;

#define DEFAULT_OBJ_NAME        "default"
#define ICON_OBJ_PREFIX 	    "icon_"
#define VERTICAL_OBJ_PREFIX 	"vert_"
#define DEFAULT_ICON_OBJ_NAME   ICON_OBJ_PREFIX DEFAULT_OBJ_NAME
#define DEFAULT_VERTICAL 	    VERTICAL_OBJ_PREFIX DEFAULT_OBJ_NAME
#define DEFAULT_ICON_VERTICAL   VERTICAL_OBJ_PREFIX DEFAULT_ICON_OBJ_NAME

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

/* this flags can be used to selectively load some parts of Look */
typedef enum
{
  LL_MyStyles 		= (0x01<<0),
  LL_MSMenu 		= (0x01<<1),	/* menu MyStyle pointers */
  LL_MSWindow 		= (0x01<<2),	/* focussed, unfocussed, sticky window styles */

  LL_MenuIcons 		= (0x01<<3),	/* Menu icons: pin-on, arrow, close */

  /* TODO: Titlebuttons - we 'll move them into  MyStyles */
  /* we will translate button numbers from the user input */
  LL_Buttons 		= (0x01<<4),
  /* assorted numerical look parameters */
  LL_SizeWindow 	= (0x01<<5),
  LL_Boundary 		= (0x01<<6),
  LL_Layouts     	= (0x01<<7),
  LL_MenuParams 	= (0x01<<8),
  LL_Icons 			= (0x01<<9),
  LL_Misc 			= (0x01<<10),
  LL_Flags 			= (0x01<<11),

  LL_Balloons 		= (0x01<<12),
  LL_SupportedHints = (0x01<<13),
  LL_DeskBacks  	= (0x01<<14),
  LL_DeskConfigs	= (0x01<<15),
  LL_IconBoxConfig  = (0x01<<16),

  LL_ImageServer    = (0x01<<30),
  /* EVERYTHING !!!!! */
  LL_Everything 	= (0xFFFFFFFF&(~LL_ImageServer))
}
LookLoadFlags;

/**********************************************************************/
/* Desktop configuration : */
/**********************************************************************/
typedef struct MyBackground
{
	unsigned long magic ;
	char 	  *name ;
    enum {
        MB_BackImage = 0,
        MB_BackMyStyle,
        MB_BackCmd
    } type ;
    char      *data ;

    ASGeometry cut;
    ASGeometry scale;
    ARGB32     tint;
    ARGB32     pad_color;
    ASFlagType align_flags;

    int        ref_count ;
}MyBackground;

typedef struct MyDesktopConfig
{
	unsigned long magic ;
	int   desk ;
	char *back_name ;
	char *layout_name ;
}MyDesktopConfig;

/*********************************************************************
 * Window decorations Frame can be defined as such :
 *
 * MyFrame "name"
 *     [Inherit     "name"]
 * #traditional form :
 *     [FrameN   <pixmap>]
 *     [FrameE   <pixmap>]
 *     [FrameS   <pixmap>]
 *     [FrameW   <pixmap>]
 *     [FrameNE  <pixmap>]
 *     [FrameNW  <pixmap>]
 *     [FrameSE  <pixmap>]
 *     [FrameSW  <pixmap>]
 * #alternative form :
 *     [Side        North|South|East|West|Any [<pixmap>]] - if pixmap is ommited -
 *                                                          empty bevel will be drawn
 *     [NoSide      North|South|East|West|Any]
 *     [Corner      NorthEast|SouthEast|NorthWest|SouthWest|Any <pixmap>] - if pixmap is ommited -
 *                                                                          empty bevel will be drawn
 *     [NoCorner    NorthEast|SouthEast|NorthWest|SouthWest|Any]
 *     [SideSize    North|South|East|West|Any <WIDTHxLENGTH>] - pixmap will be scaled to this size
 *     [CornerSize  NorthEast|SouthEast|NorthWest|SouthWest|Any <WIDTHxHEIGHT>]
 * ~MyFrame
 */
typedef struct MyFrame
{
    unsigned long magic ;
    char      *name;
    ASFlagType flags; /* first 8 bits represent one enabled side/corner each */
    struct icon_t    *parts[FRAME_PARTS];
    char             *part_filenames[FRAME_PARTS];
    unsigned int part_width[FRAME_PARTS];
    unsigned int part_length[FRAME_PARTS];
#define MYFRAME_HOR_MASK    ((0x01<<FR_N)|(0x01<<FR_S))
#define MYFRAME_VERT_MASK   ((0x01<<FR_W)|(0x01<<FR_E))
#define IsSideVertical(side)  ((side) == FR_W || (side)== FR_E)
#define IsFrameCorner(p)   ((p)>=FRAME_SIDES)
#define IsFramePart(f,p)   ((f)->parts[(p)] || ((f)->part_width[(p)] && (f)->part_length[(p)]))

    unsigned int spacing ;
}MyFrame;

extern int    _as_frame_corner_xref[FRAME_SIDES+1];
#define LeftCorner(side)    _as_frame_corner_xref[(side)]
#define RightCorner(side)   _as_frame_corner_xref[(side)+1]



/**********************************************************************/
/* MyLook itself : */
/**********************************************************************/
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
    struct ASHashTable *desk_configs; /* hash of MyDesktopConfig by desk number */

    unsigned int    KillBackgroundThreshold ; /* if WIDTHxHEIGHT of the background image is greater then this value, then
                                               * image will be destroyed when there are no windows on the desktop and desktop
                                               * is switched */

    struct MyStyle *MSMenu[MENU_BACK_STYLES]; /* menu MyStyles */

    /* focussed, unfocussed, sticky window styles */
    struct MyStyle *MSWindow[BACK_STYLES];

    ASGeometry resize_move_geometry;  /* position of the size window */

    char *DefaultIcon;      /* Icon to use when no other icons are found */

    struct icon_t *MenuArrow;

    MyButton    buttons[TITLE_BUTTONS];

    /* Menu parameters */
#define DRAW_MENU_BORDERS_NONE      0
#define DRAW_MENU_BORDERS_ITEM      1
#define DRAW_MENU_BORDERS_OVERALL   2
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

/* title bar text alignment  */
#define JUSTIFY_CENTER 0
#define JUSTIFY_LEFT   1
#define JUSTIFY_RIGHT  2
    int TitleTextAlign;       /* alignment of title bar text */

    struct ASSupportedHints *supported_hints;

    struct MyFrame *DefaultFrame;
    struct ASHashTable *FramesList ;/* hash table of named MyFrames */

}MyLook;

MyFrame *create_myframe();
MyFrame *create_default_myframe();
MyFrame *myframe_find( const char *name );
void myframe_load ( MyFrame * frame, struct ASImageManager *imman );
Bool filename2myframe_part (MyFrame *frame, int part, char *filename);
Bool myframe_has_parts(const MyFrame *frame, ASFlagType mask);
void destroy_myframe( MyFrame **pframe );

char *make_myback_image_name( MyLook *look, char *name );
void check_mybacks_list( MyLook *look );
MyBackground *create_myback( char *name );
MyBackground *add_myback( MyLook *look, MyBackground *back );
void myback_delete( MyBackground **myback, ASImageManager *imman );

MyDesktopConfig *create_mydeskconfig( int desk, char *data );
void init_deskconfigs_list( MyLook *look );
void mydeskconfig_delete( MyDesktopConfig **dc );
inline MyDesktopConfig *add_deskconfig_to_list( ASHashTable *list, MyDesktopConfig *dc );
MyDesktopConfig *add_deskconfig( MyLook *look, MyDesktopConfig *dc );


void mylook_init (MyLook * look, Bool free_resources, unsigned long what_flags /*see LookLoadFlags */ );
MyLook *mylook_create ();
void mylook_destroy (MyLook ** look);


struct MyStyle *mylook_get_style(MyLook *look, const char *name);
inline MyBackground  *mylook_get_desk_back(MyLook *look, long desk);
inline MyBackground  *mylook_get_back(MyLook *look, char *name);



/* MyLook : */
void myobj_destroy (ASHashableValue value, void *data);


#endif /* MY_LOOK_HEADER_INCLUDED */
