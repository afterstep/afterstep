#ifndef CONF_DEFS_H_FILE_INCLUDED
#define CONF_DEFS_H_FILE_INCLUDED

/***************************************************************************/
/*                        Base file pasring definitions                    */
/***************************************************************************/
#define BASE_ID_START        	0
#define BASE_MODULE_PATH_ID     BASE_ID_START
#define BASE_ICON_PATH_ID     	BASE_ID_START+1
#define BASE_PIXMAP_PATH_ID	BASE_ID_START+2
#define BASE_MYNAME_PATH_ID	BASE_ID_START+3
#define BASE_DESKTOP_SIZE_ID	BASE_ID_START+4
#define BASE_DESKTOP_SCALE_ID	BASE_ID_START+5
#define BASE_ID_END           	BASE_ID_START+10

typedef struct
  {
    char *module_path;
    char *icon_path;
    char *pixmap_path;
    char *myname_path;
    MyGeometry desktop_size;
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
  ProcessMyStyleDefinitions (MyStyleDefinition ** list, const char *PixmapPath);

/***************************************************************************/
extern char *pixmapPath;
/****************************************************************************/
/*                             Pager                                        */
/****************************************************************************/
/* flags used in configuration */
#define USE_LABEL   		(1<<0)
#define START_ICONIC		(1<<1)
#define REDRAW_BG		(1<<2)
#define STICKY_ICONS		(1<<3)
#define LABEL_BELOW_DESK 	(1<<4)
#define HIDE_INACTIVE_LABEL	(1<<5)
#define PAGE_SEPARATOR		(1<<6)
#define DIFFERENT_GRID_COLOR	(1<<7)
#define DIFFERENT_BORDER_COLOR	(1<<8)
#define SHOW_SELECTION          (1<<9)
#define FAST_STARTUP		(1<<10)
#define SET_ROOT_ON_STARTUP	(1<<11)
#define PAGER_FLAGS_DEFAULT	(USE_LABEL|REDRAW_BG|PAGE_SEPARATOR|SHOW_SELECTION)
/* set/unset flags : */
#define PAGER_SET_GEOMETRY  (1<<16)
#define PAGER_SET_ICON_GEOMETRY  	(1<<17)
#define PAGER_SET_ALIGN 			(1<<18)
#define PAGER_SET_SMALL_FONT 		(1<<19)
#define PAGER_SET_ROWS 				(1<<20)
#define PAGER_SET_COLUMNS 			(1<<21)


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
#define PAGER_STYLE_ID		(PAGER_ID_START+12)
#define PAGER_DESKTOP_IMAGE_ID	(PAGER_ID_START+13)
#define PAGER_LOADER_ARGS_ID	(PAGER_ID_START+14)

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
#define PAGER_ID_END		(PAGER_ID_START+50)
/* config data structure */

typedef struct
  {
    MyGeometry geometry, icon_geometry;
    char **labels;
    char **styles;
    int align;
    unsigned long flags, set_flags;
    char *small_font_name;
    int rows, columns;
    char *selection_color;
    char *grid_color;
    int border_width;
    char *border_color;

    MyStyleDefinition *style_defs;

    FreeStorageElem *more_stuff;

  }
PagerConfig;

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
    MyGeometry cut;
    char *tint;
    MyGeometry scale;
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


#endif /* CONF_DEFS_H_FILE_INCLUDED */
