/***********************************************************************
 *
 * afterstep per-screen data include file
 *
 ***********************************************************************/

#ifndef MY_LOOK_HEADER_INCLUDED
#define MY_LOOK_HEADER_INCLUDED

#include "../libAfterImage/asvisual.h"
#include "../libAfterImage/asimage.h"

#ifdef __cplusplus
extern "C" {
#endif

struct MyStyle;
struct ASSupportedHints;
struct MyFrame;
struct ASHashTable;
struct ScreenInfo;

#define ICON_STYLE_FOCUS   "ButtonTitleFocus"
#define ICON_STYLE_STICKY  "ButtonTitleSticky"
#define ICON_STYLE_UNFOCUS "ButtonTitleUnfocus"

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
    TitlebarNoPush      = (0x01 << 3),
    IconNoBorder        = (0x01 << 4),
    SeparateButtonTitle = (0x01 << 5),    /* icon title is a separate window */
    DecorateFrames      = (0x01 << 6),
    DontDrawBackground	= (0x01 << 7),
  /* this flags add up to the above set for no-flag options, so we can track
     what option was specifyed in config */
    LOOK_TitleButtonSpacing = (0x01 << 16),
    LOOK_TitleButtonStyle   = (0x01 << 17),
    LOOK_TitleButtonXOffset = (0x01 << 18),
    LOOK_TitleButtonYOffset = (0x01 << 19),
    LOOK_ResizeMoveGeometry = (0x01 << 20),
    LOOK_StartMenuSortMode  = (0x01 << 21),
    LOOK_DrawMenuBorders    = (0x01 << 22),
    LOOK_ButtonSize         = (0x01 << 23),
    LOOK_RubberBand         = (0x01 << 24),
    LOOK_ShadeAnimationSteps= (0x01 << 25),
    LOOK_TitleTextAlign     = (0x01 << 26)

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
	
	char *loaded_im_name ;
	Pixmap loaded_pixmap ;
}MyBackground;

typedef struct MyDesktopConfig
{
	unsigned long magic ;
	int   desk ;
	char *back_name ;
	char *layout_name ;
}MyDesktopConfig;

#define FR_N_Mask           (0x01<<FR_N)
#define FR_E_Mask           (0x01<<FR_E)
#define FR_S_Mask           (0x01<<FR_S)
#define FR_W_Mask           (0x01<<FR_W)
#define FR_NE_Mask          (0x01<<FR_NE)
#define FR_NW_Mask          (0x01<<FR_NW)
#define FR_SE_Mask          (0x01<<FR_SE)
#define FR_SW_Mask          (0x01<<FR_SW)
#define FRAME_PARTS_MASK    (FR_N_Mask|FR_E_Mask|FR_S_Mask|FR_W_Mask| \
                             FR_NE_Mask|FR_NW_Mask|FR_SE_Mask|FR_SW_Mask)

#define MYFRAME_TITLE_BACK_BTN				0
#define MYFRAME_TITLE_BACK_SPACER			1
#define MYFRAME_TITLE_BACK_TITLE_SPACER		2
#define MYFRAME_TITLE_BACK_INVALID			3
#define MYFRAME_TITLE_SIDE_MASK				0x03  /* ORed values of everything above */  
#define MYFRAME_TITLE_SIDE_BITS				2     /* number of set bits in above mask */		
#define MYFRAME_GetTbarLayoutElem(layout,i)    (((layout)>>((i)*MYFRAME_TITLE_SIDE_BITS))&MYFRAME_TITLE_SIDE_MASK)
#define MYFRAME_SetTbarLayoutElem(layout,i,elem)    (layout = (((layout)&(~(MYFRAME_TITLE_SIDE_MASK<<((i)*MYFRAME_TITLE_SIDE_BITS))))|(((elem)&MYFRAME_TITLE_SIDE_MASK)<<((i)*MYFRAME_TITLE_SIDE_BITS))))

#define MYFRAME_TITLE_SIDE_ELEMS			3     /* number of set bits in above mask */		   
			   
#define MYFRAME_TITLE_BACK_TITLE_LABEL		MYFRAME_TITLE_SIDE_ELEMS


#define MYFRAME_TITLE_BACKS					(MYFRAME_TITLE_SIDE_ELEMS+1+MYFRAME_TITLE_SIDE_ELEMS)

#define MYFRAME_TITLE_BACK_LBTN				MYFRAME_TITLE_BACK_BTN
#define MYFRAME_TITLE_BACK_LSPACER			MYFRAME_TITLE_BACK_SPACER
#define MYFRAME_TITLE_BACK_LTITLE_SPACER	MYFRAME_TITLE_BACK_TITLE_SPACER
#define MYFRAME_TITLE_BACK_LBL				MYFRAME_TITLE_BACK_TITLE_LABEL
#define MYFRAME_TITLE_BACK_LEFT2RIGHT(l) 	(MYFRAME_TITLE_BACKS-1-(l))
#define MYFRAME_TITLE_BACK_RTITLE_SPACER	MYFRAME_TITLE_BACK_LEFT2RIGHT(MYFRAME_TITLE_BACK_TITLE_SPACER)
#define MYFRAME_TITLE_BACK_RSPACER			MYFRAME_TITLE_BACK_LEFT2RIGHT(MYFRAME_TITLE_BACK_SPACER)
#define MYFRAME_TITLE_BACK_RBTN				MYFRAME_TITLE_BACK_LEFT2RIGHT(MYFRAME_TITLE_BACK_BTN)

#define MYFRAME_DEFAULT_TITLE_LAYOUT		0xFFFFFFFF

typedef struct MyFrame
{
    unsigned long magic ;
    char      *name;
    ASFlagType   set_parts;
    ASFlagType   parts_mask; /* first 8 bits represent one enabled side/corner each */
    struct icon_t    *parts[FRAME_PARTS];
    char             *part_filenames[FRAME_PARTS];
    char             *title_style_names[BACK_STYLES];
    char             *frame_style_names[BACK_STYLES];
    char             *title_back_filenames[MYFRAME_TITLE_BACKS];
    struct icon_t    *title_backs[MYFRAME_TITLE_BACKS];
    ASFlagType   set_part_size ;
    unsigned int part_width[FRAME_PARTS];
    unsigned int part_length[FRAME_PARTS];
    ASFlagType   set_part_bevel ;
/* this must not be inline functions !!!! */
#define SetPartFBevelMask(frame,mask)  ((frame)->set_part_bevel |= (mask))
#define SetPartUBevelMask(frame,mask)  ((frame)->set_part_bevel |= ((mask)<<FRAME_PARTS))
#define SetPartSBevelMask(frame,mask)  ((frame)->set_part_bevel |= ((mask)<<(FRAME_PARTS+FRAME_PARTS)))

#define SetPartFBevelSet(frame,part)  ((frame)->set_part_bevel |= (0x01<<(part)))
#define SetPartUBevelSet(frame,part)  ((frame)->set_part_bevel |= ((0x01<<FRAME_PARTS)<<(part)))
#define SetPartSBevelSet(frame,part)  ((frame)->set_part_bevel |= ((0x01<<(FRAME_PARTS+FRAME_PARTS))<<(part)))

#define IsPartFBevelSet(frame,part)  ((frame)->set_part_bevel&(0x01<<(part)))
#define IsPartUBevelSet(frame,part)  ((frame)->set_part_bevel&((0x01<<FRAME_PARTS)<<(part)))
#define IsPartSBevelSet(frame,part)  ((frame)->set_part_bevel&((0x01<<(FRAME_PARTS+FRAME_PARTS))<<(part)))
    ASFlagType   part_fbevel[FRAME_PARTS];
    ASFlagType   part_ubevel[FRAME_PARTS];
    ASFlagType   part_sbevel[FRAME_PARTS];
    ASFlagType   set_part_align ;
    ASFlagType   part_align[FRAME_PARTS];
    ASFlagType   set_title_attr ;
#define MYFRAME_TitleFBevelSet      (0x01<<1)
#define MYFRAME_TitleUBevelSet      (0x01<<2)
#define MYFRAME_TitleSBevelSet      (0x01<<3)
#define MYFRAME_TitleBevelSet       (MYFRAME_TitleFBevelSet|MYFRAME_TitleUBevelSet|MYFRAME_TitleSBevelSet)
#define MYFRAME_TitleAlignSet       (0x01<<4)
#define MYFRAME_TitleFCMSet         (0x01<<5)
#define MYFRAME_TitleUCMSet         (0x01<<6)
#define MYFRAME_TitleSCMSet         (0x01<<7)
#define MYFRAME_TitleCMSet          (MYFRAME_TitleFCMSet|MYFRAME_TitleUCMSet|MYFRAME_TitleSCMSet)

#define MYFRAME_TitleBackAlignSet_Start     		(0x01<<8)
#define MYFRAME_TitleBackAlignSet_LBtn  			(MYFRAME_TitleBackAlignSet_Start<<MYFRAME_TITLE_BACK_LBTN)
#define MYFRAME_TitleBackAlignSet_LSpacer 		 	(MYFRAME_TitleBackAlignSet_Start<<MYFRAME_TITLE_BACK_LSPACER)
#define MYFRAME_TitleBackAlignSet_LTitleSpacer  	(MYFRAME_TitleBackAlignSet_Start<<MYFRAME_TITLE_BACK_LTITLE_SPACER)
#define MYFRAME_TitleBackAlignSet_Lbl  				(MYFRAME_TitleBackAlignSet_Start<<MYFRAME_TITLE_BACK_LBL)
#define MYFRAME_TitleBackAlignSet_RTitleSpacer     	(MYFRAME_TitleBackAlignSet_Start<<MYFRAME_TITLE_BACK_RTITLE_SPACER)
#define MYFRAME_TitleBackAlignSet_RSpacer  			(MYFRAME_TitleBackAlignSet_Start<<MYFRAME_TITLE_BACK_RSPACER)
#define MYFRAME_TitleBackAlignSet_RBtn  			(MYFRAME_TitleBackAlignSet_Start<<MYFRAME_TITLE_BACK_RBTN)
#define MYFRAME_TitleBackAlignSet_End       		(MYFRAME_TitleBackAlignSet_Start<<MYFRAME_TITLE_BACKS)

#define MYFRAME_CondenseTitlebarSet   	(0x01<<16)
#define MYFRAME_LeftTitlebarLayoutSet   (0x01<<17)
#define MYFRAME_RightTitlebarLayoutSet  (0x01<<18)

    ASFlagType   title_fbevel, title_ubevel, title_sbevel;
    unsigned int title_fcm, title_ucm, title_scm ;
    ASFlagType   title_align, title_backs_align[MYFRAME_TITLE_BACKS];

    ASFlagType   condense_titlebar ;

#define MYFRAME_HOR_MASK    ((0x01<<FR_N)|(0x01<<FR_S))
#define MYFRAME_VERT_MASK   ((0x01<<FR_W)|(0x01<<FR_E))
#define IsSideVertical(side)  ((side) == FR_W || (side)== FR_E)
#define IsFrameCorner(p)   ((p)>=FRAME_SIDES)
#define IsFramePart(f,p)   (get_flags((f)->parts_mask,0x01<<(p))&&((f)->parts[(p)] || (((f)->part_width[(p)] || p >= FRAME_SIDES) && (f)->part_length[(p)])))

    unsigned int spacing ;

	unsigned long left_layout, right_layout ;

#define MYFRAME_NOBORDER	(0x01<<0)
	ASFlagType flags ;
	ASFlagType set_flags ;

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

	char *CursorFore, *CursorBack ;

    /* here buttons are mentioned in the same order they are in look and feel file */
    MyButton     buttons[TITLE_BUTTONS];
    /* thes is compiled set of pointers to above buttons : */
    unsigned int button_xref[TITLE_BUTTONS];   /* translates button numbering in look and feel into indexes in array */
    MyButton    *ordered_buttons[TITLE_BUTTONS];
#define DEFAULT_FIRST_RIGHT_BUTTON  5
    unsigned int button_first_right ;          /* index of pointer to the first right button in the above array */


    /* Menu parameters */
#define DRAW_MENU_BORDERS_NONE          0
#define DRAW_MENU_BORDERS_ITEM          1
#define DRAW_MENU_BORDERS_OVERALL       2
#define DRAW_MENU_BORDERS_FOCUSED_ITEM  3
#define DRAW_MENU_BORDERS_O_AND_F       4

    int DrawMenuBorders;
    int StartMenuSortMode;
    int menu_hcm, menu_icm, menu_scm;                    /* composition methods */

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
	/* array of two elements cos we allow different values for buttons
	 * on the right and on the left :*/
    int TitleButtonSpacing[2];
    int TitleButtonXOffset[2];
    int TitleButtonYOffset[2];

/* title bar text alignment  */
#define JUSTIFY_CENTER 0
#define JUSTIFY_LEFT   1
#define JUSTIFY_RIGHT  2
    int TitleTextAlign;       /* alignment of title bar text */

    struct ASSupportedHints *supported_hints;

    struct MyFrame *DefaultFrame;
    struct ASHashTable *FramesList ;/* hash table of named MyFrames */

    struct ASBalloonLook *balloon_look ;

}MyLook;

MyFrame *create_myframe();
MyFrame *create_default_myframe(ASFlagType default_title_align);
void inherit_myframe( MyFrame *frame, MyFrame *ancestor );
MyFrame *myframe_find( const char *name );
void myframe_load ( MyFrame * frame, struct ASImageManager *imman );
Bool filename2myframe_part (MyFrame *frame, int part, char *filename);
Bool myframe_has_parts(const MyFrame *frame, ASFlagType mask);
void destroy_myframe( MyFrame **pframe );
void check_myframes_list( MyLook *look );


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
inline MyDesktopConfig *mylook_get_desk_config(MyLook *look, long desk);



/* MyLook : */
void myobj_destroy (ASHashableValue value, void *data);

#ifdef __cplusplus
}
#endif


#endif /* MY_LOOK_HEADER_INCLUDED */
