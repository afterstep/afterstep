#ifndef CONF_DEFS_H_FILE_INCLUDED
#define CONF_DEFS_H_FILE_INCLUDED

#include "../libAfterImage/asvisual.h"
struct ASFontManager;
struct SyntaxDef;
struct MyLook;
extern struct SyntaxDef      BevelSyntax;
extern struct SyntaxDef      AlignSyntax;
extern struct SyntaxDef     *BevelSyntaxPtr;


/***************************************************************************/
/*                        Base file pasring definitions                    */
/***************************************************************************/
#define BASE_ID_START        	0
#define BASE_MODULE_PATH_ID     BASE_ID_START
#define BASE_AUDIO_PATH_ID      BASE_ID_START+1
#define BASE_ICON_PATH_ID     	BASE_ID_START+2
#define BASE_PIXMAP_PATH_ID     BASE_ID_START+3
#define BASE_FONT_PATH_ID       BASE_ID_START+4
#define BASE_CURSOR_PATH_ID     BASE_ID_START+5
#define BASE_MYNAME_PATH_ID     BASE_ID_START+6
#define BASE_DESKTOP_SIZE_ID	BASE_ID_START+7
#define BASE_DESKTOP_SCALE_ID	BASE_ID_START+8
#define BASE_ID_END             BASE_ID_START+10

typedef struct
{
    char *module_path;
    char *audio_path;
    char *icon_path;
    char *pixmap_path;
    char *font_path;
    char *cursor_path;
    char *myname_path;
    ASGeometry desktop_size;
    int desktop_scale;

    FreeStorageElem *more_stuff;
}BaseConfig;

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
#define INCLUDE_MYSTYLE {TF_NO_MYNAME_PREPENDING,"MyStyle", 7, TT_QUOTED_TEXT, MYSTYLE_START_ID, &MyStyleSyntax}

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
#define SET_SET_FLAG(d,f)  do{(d).set_flags |= (f);}while(0)
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

void MergeMyStyleText (MyStyleDefinition ** list, const char *name,
                  const char *new_font, const char *new_fcolor, const char *new_bcolor, int new_style);
void MergeMyStyleTextureOld (MyStyleDefinition ** list, const char *name,
                        int type, char *color_from, char *color_to, char *pixmap);
FreeStorageElem **MyStyleDefs2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, MyStyleDefinition * defs);


/**************************************************************************/
/*                        MyFrame parsing definitions                     */
/**************************************************************************/
#define MYFRAME_ID_START	(MYSTYLE_ID_END+1)

#define MYFRAME_START_ID 	(MYFRAME_ID_START)
#define MYFRAME_North_ID        (MYFRAME_ID_START+1)
#define MYFRAME_East_ID         (MYFRAME_ID_START+2)
#define MYFRAME_South_ID        (MYFRAME_ID_START+3)
#define MYFRAME_West_ID         (MYFRAME_ID_START+4)
#define MYFRAME_NorthWest_ID    (MYFRAME_ID_START+5)
#define MYFRAME_NorthEast_ID    (MYFRAME_ID_START+6)
#define MYFRAME_SouthWest_ID    (MYFRAME_ID_START+7)
#define MYFRAME_SouthEast_ID    (MYFRAME_ID_START+8)
#define MYFRAME_Side_ID                 (MYFRAME_ID_START+9)
#define MYFRAME_NoSide_ID               (MYFRAME_ID_START+10)
#define MYFRAME_Corner_ID               (MYFRAME_ID_START+11)
#define MYFRAME_NoCorner_ID             (MYFRAME_ID_START+12)
#define MYFRAME_TitleUnfocusedStyle_ID  (MYFRAME_ID_START+13)
#define MYFRAME_TitleFocusedStyle_ID    (MYFRAME_ID_START+14)
#define MYFRAME_TitleStickyStyle_ID     (MYFRAME_ID_START+15)
#define MYFRAME_FrameUnfocusedStyle_ID  (MYFRAME_ID_START+16)
#define MYFRAME_FrameFocusedStyle_ID    (MYFRAME_ID_START+17)
#define MYFRAME_FrameStickyStyle_ID     (MYFRAME_ID_START+18)
#define MYFRAME_TitleBackground_ID      (MYFRAME_ID_START+19)
#define MYFRAME_SideSize_ID             (MYFRAME_ID_START+20)
#define MYFRAME_CornerSize_ID           (MYFRAME_ID_START+21)
#define MYFRAME_SideAlign_ID            (MYFRAME_ID_START+22)
#define MYFRAME_CornerAlign_ID          (MYFRAME_ID_START+23)
#define MYFRAME_SideBevel_ID            (MYFRAME_ID_START+24)
#define MYFRAME_CornerBevel_ID          (MYFRAME_ID_START+25)
#define MYFRAME_TitleBevel_ID           (MYFRAME_ID_START+26)
#define MYFRAME_TitleAlign_ID           (MYFRAME_ID_START+27)
#define MYFRAME_TitleBackgroundAlign_ID (MYFRAME_ID_START+28)
#define MYFRAME_Inherit_ID              (MYFRAME_ID_START+29)
#define MYFRAME_DONE_ID         (MYFRAME_ID_START+30)

#define MYFRAME_ID_END      (MYFRAME_ID_START+40)

#define ALIGN_ID_START      (MYFRAME_ID_END+1)
#define ALIGN_Left_ID       (ALIGN_ID_START+1)
#define ALIGN_Top_ID        (ALIGN_ID_START+2)
#define ALIGN_Right_ID      (ALIGN_ID_START+3)
#define ALIGN_Bottom_ID     (ALIGN_ID_START+4)
#define ALIGN_HTiled_ID     (ALIGN_ID_START+5)
#define ALIGN_VTiled_ID     (ALIGN_ID_START+6)
#define ALIGN_HScaled_ID    (ALIGN_ID_START+7)
#define ALIGN_VScaled_ID    (ALIGN_ID_START+8)
#define ALIGN_LabelSize_ID  (ALIGN_ID_START+9)
#define ALIGN_ID_END        (ALIGN_ID_START+10)

#define BEVEL_ID_START      (ALIGN_ID_END+1)
#define BEVEL_None_ID       (BEVEL_ID_START+1)
#define BEVEL_Left_ID       (BEVEL_ID_START+2)
#define BEVEL_Top_ID        (BEVEL_ID_START+3)
#define BEVEL_Right_ID      (BEVEL_ID_START+4)
#define BEVEL_Bottom_ID     (BEVEL_ID_START+5)
#define BEVEL_Extra_ID      (BEVEL_ID_START+6)
#define BEVEL_NoOutline_ID  (BEVEL_ID_START+7)
#define BEVEL_ID_END        (BEVEL_ID_START+10)

/*********************************************************************
 * Window decorations Frame can be defined as such :
 *
 * MyFrame "name"
 *     [Inherit     "name"]
 * #traditional form :
 *     [North       <pixmap>]
 *     [East        <pixmap>]
 *     [South       <pixmap>]
 *     [West        <pixmap>]
 *     [NorthEast   <pixmap>]
 *     [NorthWest   <pixmap>]
 *     [SouthEast   <pixmap>]
 *     [SouthWest   <pixmap>]
 * #alternative form :
 *     [Side        North|South|East|West|Any [<pixmap>]] - if pixmap is ommited -
 *                                                          empty bevel will be drawn
 *     [NoSide      North|South|East|West|Any]
 *     [Corner      NorthEast|SouthEast|NorthWest|SouthWest|Any <pixmap>] - if pixmap is ommited -
 *                                                                          empty bevel will be drawn
 *     [NoCorner    NorthEast|SouthEast|NorthWest|SouthWest|Any]
 * #new settings :
 *     [TitleUnfocusedStyle   <style>
 *     [TitleFocusedStyle     <style>
 *     [TitleStickyStyle      <style>
 *     [FrameUnfocusedStyle   <style>
 *     [FrameFocusedStyle     <style>
 *     [FrameStickyStyle      <style>
 *     [TitleBackground       <pixmap>] - gets overlayed over background and under the text
 * #additional attributes :
 *     [SideSize        North|South|East|West|Any <WIDTHxLENGTH>] - pixmap will be scaled to this size
 *     [SideAlign       North|South|East|West|Any Left,Top,Right,Bottom,HTiled,VTiled,HScaled,VScaled]
 *     [SideBevel       North|South|East|West|Any None|[Left,Top,Right,Bottom,Extra,NoOutline]]
 *     [CornerSize      NorthEast|SouthEast|NorthWest|SouthWest|Any <WIDTHxHEIGHT>]
 *     [CornerAlign     NorthEast|SouthEast|NorthWest|SouthWest|Any Left,Top,Right,Bottom,HTiled,VTiled,HScaled,VScaled]
 *     [CornerBevel     NorthEast|SouthEast|NorthWest|SouthWest|Any None|[Left,Top,Right,Bottom,Extra,NoOutline]]
 *     [TitleBevel      None|[Left,Top,Right,Bottom,Extra,NoOutline]
 *     [TitleAlign      None|[Left,Top,Right,Bottom]
 *     [TitleBackgroundAlign  None|[Left,Top,Right,Bottom,HTiled,VTiled,HScaled,VScaled,LabelSize]
 * ~MyFrame
 */

extern SyntaxDef MyFrameSyntax;
/* use this in module term definition to add MyStyle parsing functionality */
#define INCLUDE_MYFRAME	{TF_NO_MYNAME_PREPENDING,"MyFrame", 7, TT_QUOTED_TEXT, MYFRAME_START_ID, &MyFrameSyntax}

typedef struct MyFrameDefinition
{
    struct MyFrameDefinition *next;

    char        *name;
    ASFlagType   set_parts;
    ASFlagType   parts_mask; /* first 8 bits represent one enabled side/corner each */
    char        *parts[FRAME_PARTS];
    char        *title_styles[BACK_STYLES];
    char        *frame_styles[BACK_STYLES];
    char        *title_back;
    ASFlagType   set_part_size ;
    unsigned int part_width[FRAME_PARTS];
    unsigned int part_length[FRAME_PARTS];
    ASFlagType   set_part_bevel ;
    ASFlagType   part_bevel[FRAME_PARTS];
    ASFlagType   set_part_align ;
    ASFlagType   part_align[FRAME_PARTS];
    ASFlagType   set_title_attr ;
    ASFlagType   title_bevel;
    ASFlagType   title_align, title_back_align;

    char       **inheritance_list ;
    int          inheritance_num ;

}MyFrameDefinition;

/* this functions work exactly like MyStyle stuff ( see above ) */
void PrintMyFrameDefinitions (MyFrameDefinition * list, int index);
void DestroyMyFrameDefinitions (MyFrameDefinition ** list);
MyFrameDefinition **ProcessMyFrameOptions (FreeStorageElem * options,
					   MyFrameDefinition ** tail);

/* converts MYFRAME defs back into FreeStorage */
FreeStorageElem **MyFrameDefs2FreeStorage (SyntaxDef * syntax,
					   FreeStorageElem ** tail,
					   MyFrameDefinition * defs);

void myframe_parse (char *tline, FILE * fd, char **myname, int *myframe_list);
/**************************************************************************/
/**************************************************************************/
/*                        balloon pasring definitions                       */
/**************************************************************************/

#define BALLOON_ID_START            (BEVEL_ID_END+1)

#define BALLOON_USED_ID	 			 BALLOON_ID_START
#define BALLOON_BorderHilite_ID     (BALLOON_ID_START+1)
#define BALLOON_XOffset_ID          (BALLOON_ID_START+2)
#define BALLOON_YOffset_ID          (BALLOON_ID_START+3)
#define BALLOON_Delay_ID            (BALLOON_ID_START+4)
#define BALLOON_CloseDelay_ID       (BALLOON_ID_START+5)
#define BALLOON_Style_ID            (BALLOON_ID_START+6)

#define	BALLOON_ID_END				(BALLOON_ID_START+10)

#define BALLOON_TERMS \
 {0, "Balloons", 8, TT_FLAG , BALLOON_USED_ID   , NULL}, \
 {0, "BalloonBorderHilite",19, TT_FLAG, BALLOON_BorderHilite_ID, &BevelSyntax}, \
 {0, "BalloonXOffset", 14, TT_INTEGER, BALLOON_XOffset_ID        , NULL}, \
 {0, "BalloonYOffset", 14, TT_INTEGER, BALLOON_YOffset_ID        , NULL}, \
 {0, "BalloonDelay", 12, TT_UINTEGER, BALLOON_Delay_ID      , NULL}, \
 {0, "BalloonCloseDelay", 17, TT_UINTEGER, BALLOON_CloseDelay_ID , NULL}, \
 {0, "BalloonStyle", 12, TT_QUOTED_TEXT, BALLOON_Style_ID , NULL}


typedef struct balloonConfig
{
  unsigned long set_flags;	/* identifyes what option is set */
#define BALLOON_USED				(0x01<<0)
#define BALLOON_HILITE              (0x01<<1)
#define BALLOON_XOFFSET             (0x01<<2)
#define BALLOON_YOFFSET             (0x01<<3)
#define BALLOON_DELAY               (0x01<<4)
#define BALLOON_CLOSE_DELAY			(0x01<<5)
#define BALLOON_STYLE               (0x01<<6)
  ASFlagType border_hilite;
  int x_offset, y_offset;
  unsigned int delay, close_delay;
  char *style ;
}balloonConfig;

struct BalloonLook;

balloonConfig *Create_balloonConfig ();
void Destroy_balloonConfig (balloonConfig * config);
balloonConfig *Process_balloonOptions (FreeStorageElem * options,
				       balloonConfig * config);
void Print_balloonConfig (balloonConfig * config);
FreeStorageElem **balloon2FreeStorage (SyntaxDef * syntax,
				       FreeStorageElem ** tail,
				       balloonConfig * config);

//void BalloonConfig2BalloonLook(struct BalloonLook *blook, struct balloonConfig *config);
void balloon_config2look( struct MyLook *look, balloonConfig *config );

/***************************************************************************/

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
#define PAGER_ID_START      (BALLOON_ID_END+1)
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
    balloonConfig *balloon_conf;
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

MyBackgroundConfig *ParseMyBackgroundOptions (FreeStorageElem * Storage, char *myname);
ASetRootConfig *ParseASetRootOptions (const char *filename, char *myname);
/*
 * all data members that has been used from ASetRootConfig structure, returned
 * by this call must be set to NULL, or memory allocated for them will be
 * deallocated by the following DestroyBaseConfig function !
 */

void DestroyASetRootConfig (ASetRootConfig * config);
void myback_parse (char *tline, FILE * fd, char **myname, int *mylook);
void DestroyMyBackgroundConfig (MyBackgroundConfig ** head);


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
 *  *WinListAlign           Left,Right,Top,Bottom
 *  *WinListBevel           None,Left,Right,Top, Bottom, NoOutline
 *  *WinListAction          [Click]1|2|3|4|5
 *  *WinListShapeToContents
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
#define WINLIST_Align_ID                (WINLIST_ID_START+10)
#define WINLIST_Bevel_ID                (WINLIST_ID_START+11)
#define WINLIST_Action_ID               (WINLIST_ID_START+12)
#define WINLIST_UnfocusedStyle_ID		(WINLIST_ID_START+13)
#define WINLIST_FocusedStyle_ID			(WINLIST_ID_START+14)
#define WINLIST_StickyStyle_ID			(WINLIST_ID_START+15)
#define WINLIST_ShapeToContents_ID      (WINLIST_ID_START+16)

#define WINLIST_BALLOONS_ID             (WINLIST_ID_START+17)

#define WINLIST_HideGeometry_ID         (WINLIST_ID_START+20)
#define WINLIST_MaxWidth_ID             (WINLIST_ID_START+21)
#define WINLIST_Orientation_ID          (WINLIST_ID_START+22)
#define WINLIST_NoAnchor_ID             (WINLIST_ID_START+23)
#define WINLIST_UseIconNames_ID         (WINLIST_ID_START+24)
#define WINLIST_AutoHide_ID             (WINLIST_ID_START+25)

#define WINLIST_ID_END	        		(WINLIST_ID_START+32)

/* config data structure */

typedef enum
{ ASN_Name = 0, ASN_IconName, ASN_ResClass, ASN_ResName, ASN_NameTypes }ASNameTypes ;

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
#define WINLIST_Align           (0x01<<10)
#define WINLIST_Bevel           (0x01<<11)
#define WINLIST_ShapeToContents (0x01<<12)

#define 	ASWL_RowsFirst 		WINLIST_FillRowsFirst
#define 	ASWL_UseSkipList	WINLIST_UseSkipList
	ASFlagType	flags ;
	ASFlagType	set_flags ;
    ASGeometry geometry ;
    unsigned int min_width, min_height ;
	unsigned int max_width, max_height ;
#define MAX_WINLIST_WINDOW_COUNT    512        /* 512 x 4 == 2048 == 1 page in memory */
	unsigned int max_rows, max_columns ;
	unsigned int min_col_width, max_col_width ;

	char *unfocused_style ;
	char *focused_style ;
	char *sticky_style ;

	ASNameTypes     show_name_type ; /* 0, 1, 2, 3 */
    ASFlagType      name_aligment ;
    ASFlagType      bevel ;

    char **mouse_actions[MAX_MOUSE_BUTTONS];

    balloonConfig *balloon_conf;
    MyStyleDefinition *style_defs;

    FreeStorageElem *more_stuff;

    /* calculated based on geometry : */
    int anchor_x, anchor_y ;
	int gravity ;


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

/**************************************************************************/
/*                        Wharf pasring definitions                       */
/**************************************************************************/

#define WHARF_ID_START          (DATABASE_ID_END+1)
#define WHARF_Wharf_ID			(WHARF_ID_START)
#define WHARF_FolderEnd_ID		(WHARF_ID_START+1)
#define WHARF_Geometry_ID		(WHARF_ID_START+2)
#define WHARF_Rows_ID			(WHARF_ID_START+3)
#define WHARF_Columns_ID		(WHARF_ID_START+4)
#define WHARF_NoPush_ID			(WHARF_ID_START+5)
#define WHARF_FullPush_ID		(WHARF_ID_START+6)
#define WHARF_NoBorder_ID		(WHARF_ID_START+7)
#define WHARF_WithdrawStyle_ID		(WHARF_ID_START+8)
/* the NoWithdraw option is undocumented, deprecated, and
 ** may be removed at Wharf's maintainer's discretion */
#define WHARF_NoWithdraw_ID  		(WHARF_ID_START+9)
#define WHARF_ForceSize_ID 		(WHARF_ID_START+10)
/* TextureType, MaxColors, BgColor, TextureColor, and Pixmap are obsolete */
#define WHARF_TextureType_ID		(WHARF_ID_START+11)
#define WHARF_MaxColors_ID 		(WHARF_ID_START+12)
#define WHARF_BgColor_ID 		(WHARF_ID_START+13)
#define WHARF_TextureColor_ID		(WHARF_ID_START+14)
#define WHARF_Pixmap_ID 		(WHARF_ID_START+15)
#define WHARF_AnimateStepsMain_ID	(WHARF_ID_START+16)
#define WHARF_AnimateSteps_ID		(WHARF_ID_START+17)
#define WHARF_AnimateDelay_ID		(WHARF_ID_START+18)
#define WHARF_AnimateMain_ID 		(WHARF_ID_START+19)
#define WHARF_Animate_ID		(WHARF_ID_START+20)

#define WHARF_Player_ID 		(WHARF_ID_START+21)
#define WHARF_Sound_ID 			(WHARF_ID_START+22)
#define WHARF_ShowLabel_ID      (WHARF_ID_START+23)
#define WHARF_LabelLocation_ID  (WHARF_ID_START+24)
#define WHARF_FlipLabel_ID      (WHARF_ID_START+25)
#define WHARF_FitContents_ID    (WHARF_ID_START+26)
#define WHARF_ShapeToContents_ID    (WHARF_ID_START+27)
#define WHARF_AlignContents_ID  (WHARF_ID_START+28)

#define	WHARF_ID_END			(WHARF_ID_START+30)
#define WFUNC_START_ID			(WHARF_ID_END)

#define WFUNC_Folders_ID		(WFUNC_START_ID)
#define WFUNC_Swallow_ID		(WFUNC_START_ID+1)

#define	WFUNC_ID_END			(WFUNC_START_ID+10)

#define WHEV_PUSH		0
#define WHEV_CLOSE_FOLDER	1
#define WHEV_OPEN_FOLDER	2
#define WHEV_CLOSE_MAIN		3
#define WHEV_OPEN_MAIN		4
#define WHEV_DROP		5
#define WHEV_MAX_EVENTS		6

#define WHEV_START_ID			(WFUNC_ID_END)

#define WHEV_Id2Code(id)		((id)-WHEV_START_ID)
#define WHEV_Code2Id(code)		((code)+WHEV_START_ID)

#define WHEV_PUSH_ID			WHEV_Code2Id(WHEV_PUSH)
#define WHEV_CLOSE_FOLDER_ID		WHEV_Code2Id(WHEV_CLOSE_FOLDER)
#define WHEV_OPEN_FOLDER_ID		WHEV_Code2Id(WHEV_OPEN_FOLDER)
#define WHEV_CLOSE_MAIN_ID		WHEV_Code2Id(WHEV_CLOSE_MAIN)
#define WHEV_OPEN_MAIN_ID		WHEV_Code2Id(WHEV_OPEN_MAIN)
#define WHEV_DROP_ID			WHEV_Code2Id(WHEV_DROP)

#define WHEV_END_ID			WHEV_Code2Id(WHEV_MAX_EVENTS)


struct WharfFolder;
struct FunctionData;

typedef struct WharfButton
{
  unsigned long set_flags;
  char *title;
  char **icon;
  struct FunctionData *function;
  struct WharfButton  *next;
  struct WharfButton  *folder;
}
WharfButton;

#define  WHARF_GEOMETRY         (0x01<<0)
#define  WHARF_ROWS             (0x01<<1)
#define  WHARF_COLUMNS			(0x01<<2)
#define  WHARF_NO_PUSH			(0x01<<3)
#define  WHARF_FULL_PUSH		(0x01<<4)
#define  WHARF_NO_BORDER		(0x01<<5)
#define  WHARF_WITHDRAW_STYLE		(0x01<<6)
#define  WHARF_NO_WITHDRAW		(0x01<<7)
#define  WHARF_FORCE_SIZE		(0x01<<8)
#define  WHARF_TEXTURE_TYPE		(0x01<<9)
#define  WHARF_MAX_COLORS		(0x01<<10)
#define  WHARF_BG_COLOR			(0x01<<11)
#define  WHARF_TEXTURE_COLOR		(0x01<<12)
#define  WHARF_PIXMAP			(0x01<<13)
#define  WHARF_ANIMATE_STEPS		(0x01<<14)
#define  WHARF_ANIMATE_STEPS_MAIN	(0x01<<15)
#define  WHARF_ANIMATE_DELAY		(0x01<<16)
#define  WHARF_ANIMATE_MAIN		(0x01<<17)
#define  WHARF_ANIMATE			(0x01<<18)
#define  WHARF_SOUND			(0x01<<19)
#define  WHARF_SHOW_LABEL       (0x01<<20)
#define  WHARF_LABEL_LOCATION   (0x01<<21)
#define  WHARF_FLIP_LABEL       (0x01<<22)
#define  WHARF_FIT_CONTENTS     (0x01<<23)
#define  WHARF_SHAPE_TO_CONTENTS (0x01<<24)
#define  WHARF_ALIGN_CONTENTS   (0x01<<25)


typedef struct
{
    unsigned long set_flags;
    unsigned long flags;

    ASGeometry geometry;
    unsigned int rows, columns;
#define NO_WITHDRAW                         0
#define WITHDRAW_ON_ANY_BUTTON              1
#define WITHDRAW_ON_EDGE_BUTTON             2
#define WITHDRAW_ON_ANY_BUTTON_AND_SHOW     3
#define WITHDRAW_ON_EDGE_BUTTON_AND_SHOW    4
#define WITHDRAW_ON_ANY(c)   ((c)->withdraw_style == WITHDRAW_ON_ANY_BUTTON  || (c)->withdraw_style == WITHDRAW_ON_ANY_BUTTON_AND_SHOW )
#define WITHDRAW_ON_EDGE(c)  ((c)->withdraw_style == WITHDRAW_ON_EDGE_BUTTON || (c)->withdraw_style == WITHDRAW_ON_EDGE_BUTTON_AND_SHOW )
    unsigned int withdraw_style;
    ASGeometry force_size;
    unsigned int texture_type, max_colors;
    char *bg_color, *texture_color;
    char *pixmap;
    unsigned int animate_steps, animate_steps_main, animate_delay;
    char *sounds[WHEV_MAX_EVENTS];
    WharfButton *root_folder;

    unsigned int label_location;
    unsigned int align_contents;

    balloonConfig *balloon_conf;
    MyStyleDefinition *style_defs;

    FreeStorageElem *more_stuff;

}
WharfConfig;


WharfButton *CreateWharfButton ();
WharfConfig *CreateWharfConfig ();

void DestroyWharfButton (WharfButton **pbtn);
void DestroyWharfConfig (WharfConfig * config);

WharfConfig *ParseWharfOptions (const char *filename, char *myname);
void PrintWharfConfig(WharfConfig *config );

int WriteWharfOptions (const char *filename, char *myname,
		       WharfConfig * config, unsigned long flags);

/***************************************************************************/
/***************************************************************************/
/*                 Supported HINTS parsing definitions                     */
/***************************************************************************/

#define HINTS_ID_START              (WHEV_END_ID+1)
#define HINTS_ICCCM_ID              (HINTS_ID_START+HINTS_ICCCM)
#define HINTS_GroupLead_ID          (HINTS_ID_START+HINTS_GroupLead)
#define HINTS_Transient_ID          (HINTS_ID_START+HINTS_Transient)
#define HINTS_Motif_ID              (HINTS_ID_START+HINTS_Motif)
#define HINTS_Gnome_ID              (HINTS_ID_START+HINTS_Gnome)
#define HINTS_ExtendedWM_ID         (HINTS_ID_START+HINTS_ExtendedWM)
#define HINTS_XResources_ID         (HINTS_ID_START+HINTS_XResources)
#define HINTS_ASDatabase_ID         (HINTS_ID_START+HINTS_ASDatabase)
#define HINTS_ID_END                (HINTS_ID_START+HINTS_Supported)

/****************************************************************************/
/*                                LOOK OPTIONS 	   		   	    */
/****************************************************************************/
#define DESK_CONFIG_BACK	0
#define DESK_CONFIG_LAYOUT	1

#define LOOK_ID_START               (HINTS_ID_END+1)

#define LOOK_DEPRECIATED_ID_START	(LOOK_ID_START)
#define LOOK_Font_ID			(LOOK_DEPRECIATED_ID_START)
#define LOOK_WindowFont_ID		(LOOK_DEPRECIATED_ID_START+1)
#define LOOK_MTitleForeColor_ID		(LOOK_DEPRECIATED_ID_START+2)
#define LOOK_MTitleBackColor_ID		(LOOK_DEPRECIATED_ID_START+3)
#define LOOK_MenuForeColor_ID		(LOOK_DEPRECIATED_ID_START+4)
#define LOOK_MenuBackColor_ID		(LOOK_DEPRECIATED_ID_START+5)
#define LOOK_MenuHiForeColor_ID		(LOOK_DEPRECIATED_ID_START+6)
#define LOOK_MenuHiBackColor_ID		(LOOK_DEPRECIATED_ID_START+7)
#define LOOK_MenuStippleColor_ID	(LOOK_DEPRECIATED_ID_START+8)
#define LOOK_StdForeColor_ID		(LOOK_DEPRECIATED_ID_START+9)
#define LOOK_StdBackColor_ID		(LOOK_DEPRECIATED_ID_START+10)
#define LOOK_StickyForeColor_ID 	(LOOK_DEPRECIATED_ID_START+11)
#define LOOK_StickyBackColor_ID 	(LOOK_DEPRECIATED_ID_START+12)
#define LOOK_HiForeColor_ID		(LOOK_DEPRECIATED_ID_START+13)
#define LOOK_HiBackColor_ID		(LOOK_DEPRECIATED_ID_START+14)
#define LOOK_IconFont_ID		(LOOK_DEPRECIATED_ID_START+15)
#define LOOK_TitleTextMode_ID		(LOOK_DEPRECIATED_ID_START+16)

#ifndef NO_TEXTURE
#define LOOK_TextureTypes_ID		(LOOK_DEPRECIATED_ID_START+17)
#define LOOK_TextureMaxColors_ID	(LOOK_DEPRECIATED_ID_START+18)
#define LOOK_TitleTextureColor_ID	(LOOK_DEPRECIATED_ID_START+19)	/* title */
#define LOOK_UTitleTextureColor_ID	(LOOK_DEPRECIATED_ID_START+20)	/* unfoc tit */
#define LOOK_STitleTextureColor_ID	(LOOK_DEPRECIATED_ID_START+21)	/* stic tit */
#define LOOK_MTitleTextureColor_ID	(LOOK_DEPRECIATED_ID_START+22)	/* menu title */
#define LOOK_MenuTextureColor_ID	(LOOK_DEPRECIATED_ID_START+23)	/* menu items */
#define LOOK_MenuHiTextureColor_ID	(LOOK_DEPRECIATED_ID_START+24)	/* sel items */
#define LOOK_MenuPixmap_ID		(LOOK_DEPRECIATED_ID_START+25)	/* menu entry */
#define LOOK_MenuHiPixmap_ID		(LOOK_DEPRECIATED_ID_START+26)	/* hil m entr */
#define LOOK_MTitlePixmap_ID		(LOOK_DEPRECIATED_ID_START+27)	/* menu title */
#define LOOK_TitlePixmap_ID		(LOOK_DEPRECIATED_ID_START+28)	/* foc tit */
#define LOOK_UTitlePixmap_ID		(LOOK_DEPRECIATED_ID_START+29)	/* unfoc tit */
#define LOOK_STitlePixmap_ID		(LOOK_DEPRECIATED_ID_START+30)	/* stick tit */

#define LOOK_ButtonTextureType_ID	(LOOK_DEPRECIATED_ID_START+31)
#define LOOK_ButtonBgColor_ID		(LOOK_DEPRECIATED_ID_START+32)
#define LOOK_ButtonTextureColor_ID	(LOOK_DEPRECIATED_ID_START+33)
#define LOOK_ButtonMaxColors_ID		(LOOK_DEPRECIATED_ID_START+34)
#define LOOK_ButtonPixmap_ID		(LOOK_DEPRECIATED_ID_START+35)
#endif

#define LOOK_DEPRECIATED_ID_END		(LOOK_DEPRECIATED_ID_START+48)

/* non depreciated options : */
#define LOOK_SUPPORTED_ID_START		(LOOK_DEPRECIATED_ID_END)

#define LOOK_IconBox_ID             (LOOK_SUPPORTED_ID_START)

#ifndef NO_TEXTURE
#define LOOK_MArrowPixmap_ID		(LOOK_SUPPORTED_ID_START+1)	/* menu arrow */
#define LOOK_MenuPinOn_ID           (LOOK_SUPPORTED_ID_START+2) /* menu pin */
#define LOOK_MenuPinOff_ID          (LOOK_SUPPORTED_ID_START+3)
#define LOOK_TxtrMenuItmInd_ID		(LOOK_SUPPORTED_ID_START+4)
#define LOOK_MenuMiniPixmaps_ID		(LOOK_SUPPORTED_ID_START+5)
#define LOOK_TextGradientColor_ID	(LOOK_SUPPORTED_ID_START+6)	/* title text */
#define LOOK_GradientText_ID		(LOOK_SUPPORTED_ID_START+7)
#define LOOK_TexturedHandle_ID		(LOOK_SUPPORTED_ID_START+8)
#define LOOK_TitlebarNoPush_ID		(LOOK_SUPPORTED_ID_START+9)
#define LOOK_ButtonNoBorder_ID		(LOOK_SUPPORTED_ID_START+10)
#endif /* NO_TEXTURE */
#define LOOK_TitleTextAlign_ID		(LOOK_SUPPORTED_ID_START+11)
#define LOOK_TitleButtonSpacing_ID	(LOOK_SUPPORTED_ID_START+12)
#define LOOK_TitleButtonStyle_ID	(LOOK_SUPPORTED_ID_START+13)
#define LOOK_TitleButtonXOffset_ID	(LOOK_SUPPORTED_ID_START+14)
#define LOOK_TitleButtonYOffset_ID	(LOOK_SUPPORTED_ID_START+15)
#define LOOK_TitleButton_ID         (LOOK_SUPPORTED_ID_START+16)
#define LOOK_IconTitleButton_ID     (LOOK_SUPPORTED_ID_START+17)
#define LOOK_ResizeMoveGeometry_ID  (LOOK_SUPPORTED_ID_START+18)
#define LOOK_StartMenuSortMode_ID   (LOOK_SUPPORTED_ID_START+19)
#define LOOK_DrawMenuBorders_ID     (LOOK_SUPPORTED_ID_START+20)

#define LOOK_ButtonSize_ID          (LOOK_SUPPORTED_ID_START+21)
#define LOOK_SeparateButtonTitle_ID (LOOK_SUPPORTED_ID_START+22)

#define LOOK_RubberBand_ID          (LOOK_SUPPORTED_ID_START+23)
#define LOOK_ShadeAnimationSteps_ID (LOOK_SUPPORTED_ID_START+24)

#define LOOK_WindowStyle_ID_START   (LOOK_SUPPORTED_ID_START+26)
#define LOOK_FWindowStyle_ID		(LOOK_WindowStyle_ID_START+BACK_FOCUSED)
#define LOOK_UWindowStyle_ID		(LOOK_WindowStyle_ID_START+BACK_UNFOCUSED)
#define LOOK_SWindowStyle_ID		(LOOK_WindowStyle_ID_START+BACK_STICKY)
#define LOOK_DefaultStyle_ID		(LOOK_WindowStyle_ID_START+BACK_DEFAULT)
#define LOOK_WindowStyle_ID_END		(LOOK_WindowStyle_ID_START+BACK_STYLES)

#define LOOK_MenuStyle_ID_START		(LOOK_WindowStyle_ID_END)
#define LOOK_MenuItemStyle_ID		(LOOK_MenuStyle_ID_START+MENU_BACK_ITEM)
#define LOOK_MenuTitleStyle_ID		(LOOK_MenuStyle_ID_START+MENU_BACK_TITLE)
#define LOOK_MenuHiliteStyle_ID		(LOOK_MenuStyle_ID_START+MENU_BACK_HILITE)
#define LOOK_MenuStippleStyle_ID	(LOOK_MenuStyle_ID_START+MENU_BACK_STIPPLE)
#define LOOK_MenuStyle_ID_END		(LOOK_MenuStyle_ID_START+MENU_BACK_STYLES)

#define LOOK_MenuFrame_ID           (LOOK_MenuStyle_ID_END+1)
#define LOOK_SupportedHints_ID      (LOOK_MenuStyle_ID_END+2)

#define LOOK_DeskConfig_ID_START    (LOOK_MenuStyle_ID_END+3)
#define LOOK_DeskBack_ID            (LOOK_DeskConfig_ID_START+DESK_CONFIG_BACK)
#define LOOK_DeskLayout_ID          (LOOK_DeskConfig_ID_START+DESK_CONFIG_LAYOUT)
#define LOOK_DeskConfig_ID_END      (LOOK_DeskConfig_ID_START+4)

#define LOOK_SUPPORTED_ID_END		(LOOK_SUPPORTED_ID_START+64)
#define LOOK_ID_END			(LOOK_SUPPORTED_ID_END)

typedef struct DesktopConfig
{
  int desk;
  char *back_name;
  char *layout_name;
  MyBackgroundConfig *back;

  struct DesktopConfig *next;
}DesktopConfig;

void DestroyDesktopConfig (DesktopConfig ** head);
DesktopConfig *ParseDesktopOptions (DesktopConfig **plist, ConfigItem * item, int id);

typedef struct LookConfig
{

/* this are values for different LOOK flags */
  unsigned long flags;
  unsigned long set_flags;

  ASBox *icon_boxes;
  short unsigned int icon_boxes_num;

#ifndef NO_TEXTURE
  char *menu_arrow;		/* menu arrow */
  char *menu_pin_on;		/* menu pin */
  char *menu_pin_off;

  char *text_gradient[2];	/* title text */
#endif				/* NO_TEXTURE */
  unsigned short int title_text_align;
  short int title_button_spacing;
  unsigned short int title_button_style;
  short int title_button_x_offset;
  short int title_button_y_offset;
  ASButton *normal_buttons[MAX_BUTTONS];       /* titlebar buttons for windows */
  ASButton *icon_buttons[MAX_BUTTONS];         /* titlebar buttons for iconifyed windows */
  ASGeometry resize_move_geometry;
  unsigned short int start_menu_sort_mode;
  unsigned short int draw_menu_borders;
  ASBox button_size;

  unsigned int rubber_band;
  unsigned int shade_animation_steps;

  char *window_styles[BACK_STYLES];
  char *menu_styles[MENU_BACK_STYLES];

  balloonConfig *balloon_conf;
  MyStyleDefinition *style_defs;
  MyFrameDefinition *frame_defs;
  MyBackgroundConfig *back_defs;
  DesktopConfig      *desk_configs;

  char *menu_frame ;

  struct ASSupportedHints *supported_hints ;

  FreeStorageElem *more_stuff;

}
LookConfig;

LookConfig *CreateLookConfig ();
void DestroyLookConfig (LookConfig * config);

ASFlagType ParseBevelOptions( FreeStorageElem * options );
ASFlagType ParseAlignOptions( FreeStorageElem * options );
void bevel_parse(char *text, FILE * fd, char **myname, int *pbevel);
LookConfig *ParseLookOptions (const char *filename, char *myname);
struct MyLook *LookConfig2MyLook ( struct LookConfig *config, struct MyLook *look,
			  			   	       struct ASImageManager *imman,
								   struct ASFontManager *fontman,
                                   Bool free_resources, Bool do_init,
								   unsigned long what_flags	);
/***************************************************************************/


#endif /* CONF_DEFS_H_FILE_INCLUDED */
