#ifndef CONF_DEFS_H_FILE_INCLUDED
#define CONF_DEFS_H_FILE_INCLUDED

#include "../libAfterImage/asvisual.h"
/***************************************************************************/
/*                        Base file pasring definitions                    */
/***************************************************************************/
#define BASE_ID_START        	0
#define BASE_MODULE_PATH_ID     BASE_ID_START
#define BASE_ICON_PATH_ID     	BASE_ID_START+1
#define BASE_PIXMAP_PATH_ID     BASE_ID_START+2
#define BASE_MYNAME_PATH_ID     BASE_ID_START+3
#define BASE_DESKTOP_SIZE_ID	BASE_ID_START+4
#define BASE_DESKTOP_SCALE_ID	BASE_ID_START+5
#define BASE_ID_END           	BASE_ID_START+10

typedef struct
  {
    char *module_path;
    char *icon_path;
    char *pixmap_path;
    char *myname_path;
    ASGeometry desktop_size;
    int desktop_scale;

    FreeStorageElem *more_stuff;
  }
BaseConfig;

BaseConfig *ParseBaseOptions (const char *filename, char *myname);
/*
 * all data members that has been used from BaseConfig structure, returned
 * by this call must be set to NULL, or memory allocated for them will be
 * deallocated by the following DestroyBaseConfig function !
 */

void DestroyBaseConfig (BaseConfig * config);

/***************************************************************************/
/*                           MyStyles                                      */
/***************************************************************************/
#define MYSTYLE_ID_START	BASE_ID_END+1
#define MYSTYLE_START_ID	MYSTYLE_ID_START
#define MYSTYLE_INHERIT_ID	MYSTYLE_ID_START+1
#define MYSTYLE_FONT_ID		MYSTYLE_ID_START+2
#define MYSTYLE_FORECOLOR_ID	MYSTYLE_ID_START+3
#define MYSTYLE_BACKCOLOR_ID	MYSTYLE_ID_START+4
#define MYSTYLE_TEXTSTYLE_ID	MYSTYLE_ID_START+5
#define MYSTYLE_MAXCOLORS_ID	MYSTYLE_ID_START+6
#define MYSTYLE_BACKGRADIENT_ID	MYSTYLE_ID_START+7
#define MYSTYLE_BACKPIXMAP_ID	MYSTYLE_ID_START+8
#define MYSTYLE_DRAWTEXTBACKGROUND_ID MYSTYLE_ID_START+9
#define MYSTYLE_DONE_ID		MYSTYLE_ID_START+10

#define MYSTYLE_ID_END		MYSTYLE_ID_START+20

#ifndef NO_TEXTURE

#define MYSTYLE_TERMS \
{TF_NO_MYNAME_PREPENDING,"MyStyle", 	7, TT_QUOTED_TEXT, MYSTYLE_START_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"Inherit", 	7, TT_QUOTED_TEXT, MYSTYLE_INHERIT_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"Font",    	4, TT_FONT, MYSTYLE_FONT_ID		, NULL},\
{TF_NO_MYNAME_PREPENDING,"ForeColor",	9, TT_COLOR, MYSTYLE_FORECOLOR_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"BackColor",	9, TT_COLOR, MYSTYLE_BACKCOLOR_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"TextStyle",	9, TT_INTEGER, MYSTYLE_TEXTSTYLE_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"MaxColors",	9, TT_INTEGER, MYSTYLE_MAXCOLORS_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"BackGradient",12,TT_INTEGER, MYSTYLE_BACKGRADIENT_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"BackPixmap",	10,TT_INTEGER, MYSTYLE_BACKPIXMAP_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"DrawTextBackground",18,TT_COLOR, MYSTYLE_DRAWTEXTBACKGROUND_ID, NULL},\
{TF_NO_MYNAME_PREPENDING|TF_SYNTAX_TERMINATOR,"~MyStyle", 	8, TT_FLAG, MYSTYLE_DONE_ID		, NULL}

#else

#define MYSTYLE_TERMS \
{TF_NO_MYNAME_PREPENDING,"MyStyle", 	7, TT_QUOTED_TEXT, MYSTYLE_START_ID	, NULL, NULL},\
{TF_NO_MYNAME_PREPENDING,"Inherit", 	7, TT_QUOTED_TEXT, MYSTYLE_INHERIT_ID	, NULL, NULL},\
{TF_NO_MYNAME_PREPENDING,"Font",    	4, TT_FONT, MYSTYLE_FONT_ID		, NULL, NULL},\
{TF_NO_MYNAME_PREPENDING,"ForeColor",	9, TT_COLOR, MYSTYLE_FORECOLOR_ID	, NULL, NULL},\
{TF_NO_MYNAME_PREPENDING,"BackColor",	9, TT_COLOR, MYSTYLE_BACKCOLOR_ID	, NULL, NULL},\
{TF_NO_MYNAME_PREPENDING,"TextStyle",	9, TT_INTEGER, MYSTYLE_TEXTSTYLE_ID	, NULL, NULL},\
{TF_NO_MYNAME_PREPENDING,"DrawTextBackground",18,TT_COLOR, MYSTYLE_DRAWTEXTBACKGROUND_ID, NULL, NULL},\
{TF_NO_MYNAME_PREPENDING|TF_SYNTAX_TERMINATOR,"~MyStyle", 	8, TT_FLAG, MYSTYLE_DONE_ID		, NULL, NULL}

#endif

extern SyntaxDef MyStyleSyntax;
/* use this in module term definition to add MyStyle parsing functionality */
#define INCLUDE_MYSTYLE	{TF_NO_MYNAME_PREPENDING,"MyStyle", 7, TT_QUOTED_TEXT, MYSTYLE_START_ID, &MyStyleSyntax, NULL}

typedef struct mystyle_definition
  {
    char *name;
    char **inherit;
    int inherit_num;
    char *font;
    char *fore_color, *back_color;
    int text_style;
    int max_colors;
    int backgradient_type;
    char *backgradient_from, *backgradient_to;
    int back_pixmap_type;
    char *back_pixmap;
    int draw_text_background;
#define SET_SET_FLAG(d,f)  d.set_flags |= f
    unsigned long set_flags;
    int finished;		/* if this one is set to 0 - MyStyle was not terminated with ~MyStyle
				 * error should be displayed as the result and this definition
				 * should be ignored
				 */
    FreeStorageElem *more_stuff;

    struct mystyle_definition *next;	/* as long as there could be several MyStyle definitions
					 * per config file, we arrange them all into the linked list
					 */
  }
MyStyleDefinition;
/* this function will process consequent MyStyle options from FreeStorage,
 * create (if needed ) and initialize MyStyleDefinition structure
 * new structures will be added at the tail of linked list.
 * it will return the new tail.
 * [options] will be changed to point to the next non-MyStyle FreeStorageElem
 */
void DestroyMyStyleDefinitions (MyStyleDefinition ** list);
MyStyleDefinition **ProcessMyStyleOptions (FreeStorageElem * options, MyStyleDefinition ** tail);

void PrintMyStyleDefinitions (MyStyleDefinition * list);
/*
 * this function process a linked list of MyStyle definitions
 * and create MyStyle for each valid definition
 * this operation is destructive in the way that all
 * data members of definitions that are used in MyStyle will be
 * set to NULL, so to prevent them being deallocated by destroy function,
 * and/or being used in other places
 * ATTENTION !!!!!
 * MyStyleDefinitions become unusable as the result, and get's destroyed
 * pointer to a list becomes NULL !
 */
void
  ProcessMyStyleDefinitions (MyStyleDefinition ** list);

/***************************************************************************/
extern char *pixmapPath;
/****************************************************************************/
/*                             Pager                                        */
/****************************************************************************/
/* flags used in configuration */
#define USE_LABEL   			(1<<0)
#define START_ICONIC			(1<<1)
#define REDRAW_BG				(1<<2)
#define STICKY_ICONS			(1<<3)
#define LABEL_BELOW_DESK 		(1<<4)
#define HIDE_INACTIVE_LABEL		(1<<5)
#define PAGE_SEPARATOR			(1<<6)
#define DIFFERENT_GRID_COLOR	(1<<7)
#define DIFFERENT_BORDER_COLOR	(1<<8)
#define SHOW_SELECTION          (1<<9)
#define FAST_STARTUP			(1<<10)
#define SET_ROOT_ON_STARTUP		(1<<11)
#define VERTICAL_LABEL          (1<<12)
#define PAGER_FLAGS_MAX_SHIFT   12
#define PAGER_FLAGS_DEFAULT	(USE_LABEL|REDRAW_BG|PAGE_SEPARATOR|SHOW_SELECTION)
/* set/unset flags : */
#define PAGER_SET_GEOMETRY  		(1<<16)
#define PAGER_SET_ICON_GEOMETRY  	(1<<17)
#define PAGER_SET_ALIGN 			(1<<18)
#define PAGER_SET_SMALL_FONT 		(1<<19)
#define PAGER_SET_ROWS 				(1<<20)
#define PAGER_SET_COLUMNS 			(1<<21)
#define PAGER_SET_SELECTION_COLOR	(1<<22)
#define PAGER_SET_GRID_COLOR		(1<<23)
#define PAGER_SET_BORDER_COLOR		(1<<24)
#define PAGER_SET_BORDER_WIDTH		(1<<25)


/* ID's used in our config */
#define PAGER_ID_START 		MYSTYLE_ID_END+1
#define PAGER_GEOMETRY_ID 	(PAGER_ID_START)
#define PAGER_ICON_GEOMETRY_ID  (PAGER_ID_START+1)
#define PAGER_ALIGN_ID		(PAGER_ID_START+2)
#define PAGER_DRAW_BG_ID	(PAGER_ID_START+3)
#define PAGER_FAST_STARTUP_ID	(PAGER_ID_START+4)
#define PAGER_SET_ROOT_ID	(PAGER_ID_START+5)
#define PAGER_SMALL_FONT_ID	(PAGER_ID_START+6)
#define PAGER_START_ICONIC_ID	(PAGER_ID_START+7)
#define PAGER_ROWS_ID		(PAGER_ID_START+8)
#define PAGER_COLUMNS_ID	(PAGER_ID_START+9)
#define PAGER_STICKY_ICONS_ID	(PAGER_ID_START+10)
#define PAGER_LABEL_ID 		(PAGER_ID_START+11)
#define PAGER_STYLE_ID      (PAGER_ID_START+12)
#define PAGER_SHADE_BUTTON_ID   (PAGER_ID_START+13)

#define PAGER_DECORATION_ID	(PAGER_ID_START+20)
#define PAGER_MYSTYLE_ID	(PAGER_ID_START+21)
#define PAGER_BALLOONS_ID	(PAGER_ID_START+22)

#define PAGER_DECOR_NOLABEL_ID 		(PAGER_ID_START+30)
#define PAGER_DECOR_NOSEPARATOR_ID 	(PAGER_ID_START+31)
#define PAGER_DECOR_NOSELECTION_ID 	(PAGER_ID_START+32)
#define PAGER_DECOR_SEL_COLOR_ID 	(PAGER_ID_START+33)
#define PAGER_DECOR_GRID_COLOR_ID 	(PAGER_ID_START+34)
#define PAGER_DECOR_BORDER_WIDTH_ID 	(PAGER_ID_START+35)
#define PAGER_DECOR_BORDER_COLOR_ID	(PAGER_ID_START+36)
#define PAGER_DECOR_LABEL_BELOW_ID	(PAGER_ID_START+37)
#define PAGER_DECOR_HIDE_INACTIVE_ID 	(PAGER_ID_START+38)
#define PAGER_DECOR_VERTICAL_LABEL_ID   (PAGER_ID_START+39)
#define PAGER_ID_END		(PAGER_ID_START+50)
/* config data structure */

typedef struct
  {
    int    rows, columns;
    ASGeometry geometry, icon_geometry;
    char **labels;
    char **styles;
    int    align;
    unsigned long flags, set_flags;
    char  *small_font_name;
    int    border_width;
    char  *shade_button[2];

    char *selection_color;
    char *grid_color;
    char *border_color;
    MyStyleDefinition *style_defs;
    FreeStorageElem *more_stuff;

    /* these are generated after reading the config : */
    int gravity ;
    ARGB32  selection_color_argb;
    ARGB32  grid_color_argb;
    ARGB32  border_color_argb;
#define DESK_ACTIVE     0
#define DESK_INACTIVE   1
#define DESK_STYLES     2
    struct MyStyle *MSDeskTitle[DESK_STYLES];
    struct MyStyle **MSDeskBack;
    struct button_t *shade_btn ;

}PagerConfig;

PagerConfig *CreatePagerConfig (int ndesks);
PagerConfig *ParsePagerOptions (const char *filename, char *myname, int desk1, int desk2);
int WritePagerOptions (const char *filename, char *myname, int desk1, int desk2, PagerConfig * config, unsigned long flags);
void DestroyPagerConfig (PagerConfig * config);

/**************************************************************************/
/***************************************************************************/
/*                        MyBackground pasring definitions                 */
/***************************************************************************/
#define BGR_ID_START        	PAGER_ID_END
#define BGR_MYBACKGROUND        BGR_ID_START
#define BGR_USE		        BGR_ID_START+1
#define BGR_CUT       		BGR_ID_START+2
#define BGR_TINT	        BGR_ID_START+3
#define BGR_SCALE	        BGR_ID_START+4
#define BGR_ALIGN		BGR_ID_START+5
#define BGR_PAD		        BGR_ID_START+6
#define BGR_MYBACKGROUND_END    BGR_ID_START+7

#define BGR_DESK_BACK     	BGR_ID_START+8
#define BGR_ID_END        	BGR_ID_START+20

typedef struct my_background_config
  {

    char *name;
    unsigned long flags;
    char *data;
    ASGeometry cut;
    char *tint;
    ASGeometry scale;
    char *pad;
    struct my_background_config *next;
  }
MyBackgroundConfig;

typedef struct desk_back_config
  {
    int desk;
    char *back_name;
    MyBackgroundConfig *back;

    struct desk_back_config *next;
  }
DeskBackConfig;

typedef struct
  {
    MyBackgroundConfig *my_backs;
    DeskBackConfig *my_desks;

    MyStyleDefinition *style_defs;

    FreeStorageElem *more_stuff;
  }
ASetRootConfig;

ASetRootConfig *ParseASetRootOptions (const char *filename, char *myname);
/*
 * all data members that has been used from ASetRootConfig structure, returned
 * by this call must be set to NULL, or memory allocated for them will be
 * deallocated by the following DestroyBaseConfig function !
 */

void DestroyASetRootConfig (ASetRootConfig * config);
void myback_parse (char *tline, FILE * fd, char **myname, int *mylook);

/***************************************************************************/
/***************************************************************************/
/*                        WinList config parsing definitions               */
/***************************************************************************/
/* New winlist config :
 *
 *	*WinListGeometry		+x+y
 *  *WinListMinSize			WxH
 *  *WinListMaxSize			WxH
 *  *WinListMaxRows			count
 *  *WinListMaxColumns		count
 *  *WinListMinColWidth		width
 *  *WinListMaxColWidth		width
 *  *WinListFillRowsFirst
 *  *WinListUseSkipList
 *  *WinListUnfocusedStyle 	"style"
 *  *WinListFocusedStyle 	"style"
 *  *WinListStickyStyle 	"style"
 *	*WinListUseName			0|1|2|3   # 0 - Name, 1 - icon, 2 - res_name, 3 - res_class
 *	*WinListJustify			l|c|r     # l - Left, c - Center, r - Right
 *  *WinListAction			[Click]1|2|3|4|5
 *
 * Obsolete functions :
 *
 *  *WinListHideGeometry	WxH+x+y
 *  *WinListNoAnchor
 *  *WinListUseIconNames
 *  *WinListMaxWidth		width
 *  *WinListOrientation		across|vertical
 *  *WinListAutoHide
 */
#define WINLIST_ID_START        		(BGR_ID_END+1)
#define WINLIST_FillRowsFirst_ID		(WINLIST_ID_START)
#define WINLIST_UseSkipList_ID			(WINLIST_ID_START+1)
#define WINLIST_Geometry_ID				(WINLIST_ID_START+2)
#define WINLIST_MinSize_ID				(WINLIST_ID_START+3)
#define WINLIST_MaxSize_ID				(WINLIST_ID_START+4)
#define WINLIST_MaxRows_ID				(WINLIST_ID_START+5)
#define WINLIST_MaxColumns_ID			(WINLIST_ID_START+6)
#define WINLIST_MaxColWidth_ID			(WINLIST_ID_START+7)
#define WINLIST_MinColWidth_ID			(WINLIST_ID_START+8)
#define WINLIST_UseName_ID				(WINLIST_ID_START+9)
#define WINLIST_Justify_ID				(WINLIST_ID_START+10)
#define WINLIST_Action_ID				(WINLIST_ID_START+12)
#define WINLIST_UnfocusedStyle_ID		(WINLIST_ID_START+13)
#define WINLIST_FocusedStyle_ID			(WINLIST_ID_START+14)
#define WINLIST_StickyStyle_ID			(WINLIST_ID_START+15)

#define WINLIST_BALLOONS_ID				(WINLIST_ID_START+16)

#define WINLIST_HideGeometry_ID			(WINLIST_ID_START+17)
#define WINLIST_MaxWidth_ID				(WINLIST_ID_START+18)
#define WINLIST_Orientation_ID			(WINLIST_ID_START+19)
#define WINLIST_NoAnchor_ID				(WINLIST_ID_START+20)
#define WINLIST_UseIconNames_ID			(WINLIST_ID_START+21)
#define WINLIST_AutoHide_ID				(WINLIST_ID_START+22)

#define WINLIST_ID_END	        		(WINLIST_ID_START+32)

/* config data structure */

typedef enum
{ ASN_Name = 0, ASN_IconName, ASN_ResClass, ASN_ResName, ASN_NameTypes }ASNameTypes ;
typedef enum
{ ASA_Left = 0, ASA_Center, ASA_Right, ASA_AligmentTypes } ASAligmentTypes;

typedef struct WinListConfig
{
#define WINLIST_FillRowsFirst	(0x01<<0)
#define WINLIST_UseSkipList		(0x01<<1)
#define WINLIST_Geometry		(0x01<<2)
#define WINLIST_MinSize			(0x01<<3)
#define WINLIST_MaxSize			(0x01<<4)
#define WINLIST_MaxRows			(0x01<<5)
#define WINLIST_MaxColumns		(0x01<<6)
#define WINLIST_MaxColWidth		(0x01<<7)
#define WINLIST_MinColWidth		(0x01<<8)
#define WINLIST_UseName			(0x01<<9)
#define WINLIST_Justify			(0x01<<10)

#define 	ASWL_RowsFirst 		WINLIST_FillRowsFirst
#define 	ASWL_UseSkipList	WINLIST_UseSkipList
	ASFlagType	flags ;
	ASFlagType	set_flags ;
	int anchor_x, anchor_y ;
	int gravity ;
	unsigned int min_width, min_height ;
	unsigned int max_width, max_height ;
	unsigned int max_rows, max_columns ;
	unsigned int min_col_width, max_col_width ;

	char *unfocused_style ;
	char *focused_style ;
	char *sticky_style ;

	ASNameTypes     show_name_type ; /* 0, 1, 2, 3 */
	ASAligmentTypes name_aligment ;

	char *mouse_actions[MAX_MOUSE_BUTTONS];

    MyStyleDefinition *style_defs;

    FreeStorageElem *more_stuff;

}WinListConfig;

WinListConfig *CreateWinListConfig ();
void DestroyWinListConfig (WinListConfig * config);
void PrintWinListConfig (WinListConfig * config);
int WriteWinListOptions (const char *filename, char *myname, WinListConfig * config, unsigned long flags);
WinListConfig *ParseWinListOptions (const char *filename, char *myname);

/**************************************************************************/
/*                        database pasring definitions                    */
/**************************************************************************/


#define GRAVITY_ID_START            (WINLIST_ID_END+1)

#define GRAVITY_NorthWest_ID        (GRAVITY_ID_START+NorthWestGravity)
#define GRAVITY_North_ID            (GRAVITY_ID_START+NorthGravity)
#define GRAVITY_NorthEast_ID        (GRAVITY_ID_START+NorthEastGravity)
#define GRAVITY_West_ID             (GRAVITY_ID_START+WestGravity)
#define GRAVITY_Center_ID           (GRAVITY_ID_START+CenterGravity)
#define GRAVITY_East_ID             (GRAVITY_ID_START+EastGravity)
#define GRAVITY_SouthWest_ID        (GRAVITY_ID_START+SouthWestGravity)
#define GRAVITY_South_ID            (GRAVITY_ID_START+SouthGravity)
#define GRAVITY_SouthEast_ID        (GRAVITY_ID_START+SouthEastGravity)
#define GRAVITY_Static_ID           (GRAVITY_ID_START+StaticGravity)
#define GRAVITY_ID_END              (GRAVITY_ID_START+11)

#define DATABASE_ID_START           (GRAVITY_ID_END+1)
#define DATABASE_STYLE_ID            DATABASE_ID_START
#define DATABASE_Icon_ID            (DATABASE_ID_START+1)
#define DATABASE_NoIcon_ID          (DATABASE_ID_START+2)
#define DATABASE_FocusStyle_ID		(DATABASE_ID_START+3)
#define DATABASE_UnfocusStyle_ID	(DATABASE_ID_START+4)
#define DATABASE_StickyStyle_ID		(DATABASE_ID_START+5)
#define DATABASE_NoIconTitle_ID		(DATABASE_ID_START+6)
#define DATABASE_IconTitle_ID		(DATABASE_ID_START+7)
#define DATABASE_Focus_ID           (DATABASE_ID_START+8)
#define DATABASE_NoFocus_ID         (DATABASE_ID_START+9)
#define DATABASE_NoTitle_ID         (DATABASE_ID_START+10)
#define DATABASE_Title_ID           (DATABASE_ID_START+11)
#define DATABASE_NoHandles_ID		(DATABASE_ID_START+12)
#define DATABASE_Handles_ID         (DATABASE_ID_START+13)
#define DATABASE_NoButton_ID		(DATABASE_ID_START+14)
#define DATABASE_Button_ID          (DATABASE_ID_START+15)
#define DATABASE_WindowListSkip_ID	(DATABASE_ID_START+16)
#define DATABASE_WindowListHit_ID	(DATABASE_ID_START+17)
#define DATABASE_CirculateSkip_ID	(DATABASE_ID_START+18)
#define DATABASE_CirculateHit_ID	(DATABASE_ID_START+19)
#define DATABASE_StartIconic_ID		(DATABASE_ID_START+20)
#define DATABASE_StartNormal_ID		(DATABASE_ID_START+21)
#define DATABASE_Layer_ID           (DATABASE_ID_START+22)
#define DATABASE_StaysOnTop_ID		(DATABASE_ID_START+23)
#define DATABASE_StaysPut_ID		(DATABASE_ID_START+24)
#define DATABASE_StaysOnBack_ID		(DATABASE_ID_START+25)
#define DATABASE_AvoidCover_ID		(DATABASE_ID_START+26)
#define DATABASE_AllowCover_ID		(DATABASE_ID_START+27)
#define DATABASE_VerticalTitle_ID	(DATABASE_ID_START+28)
#define DATABASE_HorizontalTitle_ID	(DATABASE_ID_START+29)
#define DATABASE_Sticky_ID          (DATABASE_ID_START+30)
#define DATABASE_Slippery_ID		(DATABASE_ID_START+31)
#define DATABASE_BorderWidth_ID	 	(DATABASE_ID_START+32)
#define DATABASE_HandleWidth_ID		(DATABASE_ID_START+33)
#define DATABASE_StartsOnDesk_ID	(DATABASE_ID_START+34)
#define DATABASE_ViewportX_ID		(DATABASE_ID_START+35)
#define DATABASE_ViewportY_ID		(DATABASE_ID_START+36)
#define DATABASE_StartsAnywhere_ID	(DATABASE_ID_START+37)
#define DATABASE_NoFrame_ID         (DATABASE_ID_START+38)
#define DATABASE_Frame_ID           (DATABASE_ID_START+39)
#define DATABASE_DefaultGeometry_ID        (DATABASE_ID_START+40)
#define DATABASE_OverrideGravity_ID        (DATABASE_ID_START+41)
#define DATABASE_HonorPPosition_ID         (DATABASE_ID_START+42)
#define DATABASE_NoPPosition_ID            (DATABASE_ID_START+43)
#define DATABASE_HonorGroupHints_ID        (DATABASE_ID_START+44)
#define DATABASE_NoGroupHints_ID           (DATABASE_ID_START+45)
#define DATABASE_HonorTransientHints_ID    (DATABASE_ID_START+46)
#define DATABASE_NoTransientHints_ID       (DATABASE_ID_START+47)
#define DATABASE_HonorMotifHints_ID        (DATABASE_ID_START+48)
#define DATABASE_NoMotifHints_ID           (DATABASE_ID_START+49)
#define DATABASE_HonorGnomeHints_ID        (DATABASE_ID_START+50)
#define DATABASE_NoGnomeHints_ID           (DATABASE_ID_START+51)
#define DATABASE_HonorExtWMHints_ID        (DATABASE_ID_START+52)
#define DATABASE_NoExtWMHints_ID           (DATABASE_ID_START+53)
#define DATABASE_HonorXResources_ID        (DATABASE_ID_START+54)
#define DATABASE_NoXResources_ID           (DATABASE_ID_START+55)

#define DATABASE_ID_END             (DATABASE_ID_START+64)

/* we use name_list structure 1 to 1 in here, as it does not requre any
   preprocessing from us
 */
struct name_list;

unsigned int translate_title_button (unsigned int user_button);
unsigned int translate_title_button_back (unsigned int title_button);
struct name_list *ParseDatabaseOptions (const char *filename, char *myname);
int WriteDatabaseOptions (const char *filename, char *myname,
			  struct name_list *config, unsigned long flags);


#endif /* CONF_DEFS_H_FILE_INCLUDED */
