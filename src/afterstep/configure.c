/*
 * Copyright (c) 1998 Rafal Wierzbicki <rafal@mcss.mcmaster.ca>
 * Copyright (c) 1998,2002 Sasha Vasko <sasha at aftercode.net>
 * Copyright (c) 1998 Michal Vitecek <fuf@fuf.sh.cvut.cz>
 * Copyright (c) 1998 Nat Makarevitch <nat@linux-france.com>
 * Copyright (c) 1998 Mike Venaccio <venaccio@aero.und.edu>
 * Copyright (c) 1998 Ethan Fischer <allanon@crystaltokyo.com>
 * Copyright (c) 1998 Mike Venaccio <venaccio@aero.und.edu>
 * Copyright (c) 1998 Chris Ridd <c.ridd@isode.com>
 * Copyright (c) 1997 Raphael Goulais <velephys@hol.fr>
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (C) 1996 Frank Fejes
 * Copyright (C) 1995 Bo Yang
 * Copyright (C) 1993 Robert Nation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/****************************************************************************
 *
 * Configure.c: reads the configuration files, interprets them,
 * and sets up menus, bindings, colors, and fonts as specified
 *
 ***************************************************************************/

#define LOCAL_DEBUG
#include "../../configure.h"

#include "asinternals.h"

#include <unistd.h>

#include "dirtree.h"

#include "../../libAfterStep/session.h"
#include "../../libAfterStep/mystyle_property.h"

#include "../../libAfterConf/afterconf.h"


/* old look auxilary variables : */
static MyFont StdFont    = {NULL};         /* font structure */
static MyFont WindowFont = {NULL};      /* font structure for window titles */
static MyFont IconFont   = {NULL};        /* for icon labels */
/*
 * the old-style look variables
 */
static char         *Stdfont     = NULL;
static char         *Windowfont  = NULL;
static char         *Iconfont    = NULL;

static char         *WindowForeColor[BACK_STYLES]    = { NULL };
static char         *WindowBackColor[BACK_STYLES]    = { NULL };
static char         *WindowGradient[BACK_STYLES]     = { NULL };
static char         *WindowPixmap[BACK_STYLES]       = { NULL };
static char         *MenuForeColor[MENU_BACK_STYLES] = { NULL };
static char         *MenuBackColor[MENU_BACK_STYLES] = { NULL };
static char         *MenuGradient[MENU_BACK_STYLES]  = { NULL };
static char         *MenuPixmap[MENU_BACK_STYLES]    = { NULL };
static char         *IconBgColor = NULL;
static char         *IconTexColor = NULL;
static char         *IconPixmapFile = NULL;

static char         *TexTypes = NULL;
static int           TitleTextType = 0;
static int           TitleTextY = 0;
static int           IconTexType = TEXTURE_BUILTIN;

static char         *MenuPinOn = NULL ;
static int           MenuPinOnButton = -1 ;

static MyFrameDefinition *MyFrameList = NULL ;
static char         *DefaultFrameName = NULL ;

static balloonConfig BalloonConfig = {0, 0, 0, 0, 0, 0, NULL };


/* parsing handling functions for different data types : */

void          SetInts               (char *text, FILE * fd, char **arg1, int *arg2);
void          SetFlag               (char *text, FILE * fd, char **arg, int *another);
void          SetFlag2              (char *text, FILE * fd, char **arg, int *var);
void          SetBox                (char *text, FILE * fd, char **arg, int *junk);
void          SetCursor             (char *text, FILE * fd, char **arg, int *junk);
void          SetCustomCursor       (char *text, FILE * fd, char **arg, int *junk);
void          SetButtonList         (char *text, FILE * fd, char **arg1, int *arg2);
void          SetTitleText          (char *tline, FILE * fd, char **junk, int *junk2);
void          SetTitleButton        (char *tline, FILE * fd, char **junk, int *junk2);
void          SetFramePart          (char *text, FILE * fd, char **frame, int *id);
void          SetModifier           (char *text, FILE * fd, char **mod, int *junk2);
void          SetTButtonOrder       (char *text, FILE * fd, char **mod, int *junk2);

void          assign_string         (char *text, FILE * fd, char **arg, int *idx);
void          assign_quoted_string  (char *text, FILE * fd, char **arg, int *junk);
void          assign_path           (char *text, FILE * fd, char **arg, int *idx);
void          assign_themable_path  (char *text, FILE * fd, char **arg, int *idx);
void          assign_pixmap         (char *text, FILE * fd, char **arg, int *idx);
void          assign_geometry       (char *text, FILE * fd, char **geom, int *junk);
void          obsolete              (char *text, FILE * fd, char **arg, int *);

void          deskback_parse        (char *text, FILE * fd, char **junk, int *junk2);

/* main parsing function  : */
void          match_string (struct config *table, char *text, char *error_msg, FILE * fd);

/* menu loading code : */
int           MeltStartMenu (char *buf);

/* scratch variable : */
static int dummy;

/*
 * Order is important here! if one keyword is the same as the first part of
 * another keyword, the shorter one must come first!
 */
struct config main_config[] = {
	/* feel options */
	{"StubbornIcons", SetFlag, (char **)StubbornIcons, (int *)0},
	{"StubbornPlacement", SetFlag, (char **)StubbornPlacement, (int *)0},
	{"StubbornIconPlacement", SetFlag, (char **)StubbornIconPlacement, (int *)0},
	{"StickyIcons", SetFlag, (char **)StickyIcons, (int *)0},
	{"IconTitle", SetFlag, (char **)IconTitle, (int *)0},
	{"KeepIconWindows", SetFlag, (char **)KeepIconWindows, (int *)0},
	{"NoPPosition", SetFlag, (char **)NoPPosition, (int *)0},
	{"CirculateSkipIcons", SetFlag, (char **)CirculateSkipIcons, (int *)0},
    {"EdgeScroll", SetInts, (char **)&Scr.Feel.EdgeScrollX, &Scr.Feel.EdgeScrollY},
	{"RandomPlacement", SetFlag, (char **)RandomPlacement, (int *)0},
	{"SmartPlacement", SetFlag, (char **)SMART_PLACEMENT, (int *)0},
	{"DontMoveOff", SetFlag, (char **)DontMoveOff, (int *)0},
	{"DecorateTransients", SetFlag, (char **)DecorateTransients, (int *)0},
	{"CenterOnCirculate", SetFlag, (char **)CenterOnCirculate, (int *)0},
    {"AutoRaise", SetInts, (char **)&Scr.Feel.AutoRaiseDelay, &dummy},
    {"ClickTime", SetInts, (char **)&Scr.Feel.ClickTime, &dummy},
    {"OpaqueMove", SetInts, (char **)&Scr.Feel.OpaqueMove, &dummy},
    {"OpaqueResize", SetInts, (char **)&Scr.Feel.OpaqueResize, &dummy},
    {"XorValue", SetInts, (char **)&Scr.Feel.XorValue, &dummy},
	{"Mouse", ParseMouseEntry, (char **)1, (int *)0},
    {"Popup", ParsePopupEntry, (char **)1, (int *)0},
    {"Function", ParseFunctionEntry, (char **)1, (int *)0},
	{"Key", ParseKeyEntry, (char **)1, (int *)0},
	{"ClickToFocus", SetFlag, (char **)ClickToFocus, (int *)EatFocusClick},
    {"EatFocusClick", SetFlag, (char **)EatFocusClick, (int *)0},
    {"ClickToRaise", SetButtonList, (char **)&Scr.Feel.RaiseButtons, (int *)0},
	{"MenusHigh", SetFlag, (char **)MenusHigh, (int *)0},
	{"SloppyFocus", SetFlag, (char **)SloppyFocus, (int *)0},
	{"Cursor", SetCursor, (char **)0, (int *)0},
	{"CustomCursor", SetCustomCursor, (char **)0, (int *)0},
    {"PagingDefault", SetFlag, (char **)DoHandlePageing, NULL},
    {"EdgeResistance", SetInts, (char **)&Scr.Feel.EdgeResistanceScroll, &Scr.Feel.EdgeResistanceMove},
	{"BackingStore", SetFlag, (char **)BackingStore, (int *)0},
	{"AppsBackingStore", SetFlag, (char **)AppsBackingStore, (int *)0},
	{"SaveUnders", SetFlag, (char **)SaveUnders, (int *)0},
    {"Xzap", SetInts, (char **)&Scr.Feel.Xzap, (int *)&dummy},
    {"Yzap", SetInts, (char **)&Scr.Feel.Yzap, (int *)&dummy},
    {"AutoReverse", SetInts, (char **)&Scr.Feel.AutoReverse, (int *)&dummy},
    {"AutoTabThroughDesks", SetFlag, (char **)AutoTabThroughDesks, NULL},
    {"MWMFunctionHints", obsolete, (char **)0, NULL},
    {"MWMDecorHints", obsolete, (char **)0, NULL},
    {"MWMHintOverride", obsolete, (char **)0, NULL},
    {"FollowTitleChanges", SetFlag, (char **)FollowTitleChanges, (int *)0},
    {"PersistentMenus", SetFlag, (char **)PersistentMenus, (int *)0},
    {"NoSnapKey", SetModifier, (char **)&(Scr.Feel.no_snaping_mod), (int *)0},
    {"ScreenEdgeAttraction", SetInts, (char **)&Scr.Feel.EdgeAttractionScreen, &dummy},
    {"WindowEdgeAttraction", SetInts, (char **)&Scr.Feel.EdgeAttractionWindow, &dummy},
    {"DontRestoreFocus", SetFlag, (char **)DontRestoreFocus, &dummy},
    {"WindowBox", windowbox_parse, (char**)&(Scr.Feel.window_boxes), (int*)&(Scr.Feel.window_boxes_num)},
    {"DefaultWindowBox", assign_quoted_string, (char**)&(Scr.Feel.default_window_box_name), (int*)0},
    {"RecentSubmenuItems", SetInts, (char**)&Scr.Feel.recent_submenu_items, (int*)&dummy},

    /* look options */
	{"Font", assign_string, &Stdfont, (int *)0},
	{"WindowFont", assign_string, &Windowfont, (int *)0},
    {"MTitleForeColor", assign_string, &MenuForeColor[MENU_BACK_TITLE], (int *)0},
    {"MTitleBackColor", assign_string, &MenuBackColor[MENU_BACK_TITLE], (int *)0},
    {"MenuForeColor", assign_string, &MenuForeColor[MENU_BACK_ITEM], (int *)0},
    {"MenuBackColor", assign_string, &MenuBackColor[MENU_BACK_ITEM], (int *)0},
    {"MenuHiForeColor", assign_string, &MenuForeColor[MENU_BACK_HILITE], (int *)0},
    {"MenuHiBackColor", assign_string, &MenuBackColor[MENU_BACK_HILITE], (int *)0},
    {"MenuStippleColor", assign_string, &MenuForeColor[MENU_BACK_STIPPLE], (int *)0},
    {"StdForeColor", assign_string, &WindowForeColor[BACK_UNFOCUSED], (int *)0},
    {"StdBackColor", assign_string, &WindowBackColor[BACK_UNFOCUSED], (int *)0},
    {"StickyForeColor", assign_string, &WindowForeColor[BACK_STICKY], (int *)0},
    {"StickyBackColor", assign_string, &WindowBackColor[BACK_STICKY], (int *)0},
    {"HiForeColor", assign_string, &WindowForeColor[BACK_FOCUSED], (int *)0},
    {"HiBackColor", assign_string, &WindowBackColor[BACK_FOCUSED], (int *)0},
	{"IconBox", SetBox, (char **)0, (int *)0},
	{"IconFont", assign_string, &Iconfont, (int *)0},
	{"MyStyle", mystyle_parse, NULL, NULL},
    /* new stuff : */
    {"MyBackground", myback_parse, (char**)"asetroot", NULL},  /* pretending to be asteroot here */
    {"DeskBack", deskback_parse, NULL, NULL },
    {"*asetrootDeskBack", deskback_parse, NULL, NULL },        /* pretending to be asteroot here */
    {"MyFrame", myframe_parse, (char**)"afterstep", (int*)&MyFrameList},
    {"DefaultFrame", assign_quoted_string, (char**)&DefaultFrameName, (int*)0},
    {"DontDrawBackground", SetFlag2, (char **)DontDrawBackground, (int *)&Scr.Look.flags},

#ifndef NO_TEXTURE
	{"TextureTypes", assign_string, &TexTypes, (int *)0},
    {"TextureMaxColors", obsolete, NULL, (int *)0},
    {"TitleTextureColor", assign_string, &WindowGradient[BACK_FOCUSED], (int *)0},    /* title */
    {"UTitleTextureColor", assign_string, &WindowGradient[BACK_UNFOCUSED], (int *)0},   /* unfoc tit */
    {"STitleTextureColor", assign_string, &WindowGradient[BACK_STICKY], (int *)0},   /* stic tit */
    {"MTitleTextureColor", assign_string, &MenuGradient[MENU_BACK_TITLE], (int *)0},   /* menu title */
    {"MenuTextureColor", assign_string, &MenuGradient[MENU_BACK_ITEM], (int *)0}, /* menu items */
    {"MenuHiTextureColor", assign_string, &MenuGradient[MENU_BACK_HILITE], (int *)0},  /* sel items */
    {"MenuPixmap", assign_string, &MenuPixmap[MENU_BACK_ITEM], (int *)0},  /* menu entry */
    {"MenuHiPixmap", assign_string, &MenuPixmap[MENU_BACK_HILITE], (int *)0},   /* hil m entr */
    {"MTitlePixmap", assign_string, &MenuPixmap[MENU_BACK_TITLE], (int *)0},   /* menu title */
    {"TitlePixmap", assign_string,  &WindowPixmap[BACK_FOCUSED], (int *)0}, /* foc tit */
    {"UTitlePixmap", assign_string, &WindowPixmap[BACK_UNFOCUSED], (int *)0},    /* unfoc tit */
    {"STitlePixmap", assign_string, &WindowPixmap[BACK_STICKY], (int *)0},    /* stick tit */

    {"MenuPinOn", assign_string, &MenuPinOn, (int *)0},    /* menu pin */
    {"MenuPinOff", obsolete, (char **)NULL, (int *)0},
    {"MArrowPixmap", assign_pixmap, (char **)&Scr.Look.MenuArrow, (int *)0},   /* menu arrow */

    {"TexturedHandle", SetFlag2, (char **)TexturedHandle, (int *)&Scr.Look.flags},
    {"TitlebarNoPush", SetFlag2, (char **)TitlebarNoPush, (int *)&Scr.Look.flags},

	/* these are obsolete : */
    {"TextGradientColor", obsolete, (char **)NULL, (int *)0}, /* title text */
    {"GradientText", obsolete, (char **)NULL, (int *)0},

    {"ButtonTextureType", SetInts, (char **)&IconTexType, &dummy},
	{"ButtonBgColor", assign_string, &IconBgColor, (int *)0},
	{"ButtonTextureColor", assign_string, &IconTexColor, (int *)0},
    {"ButtonMaxColors", obsolete, (char **)NULL, NULL},
	{"ButtonPixmap", assign_string, &IconPixmapFile, (int *)0},
    {"ButtonNoBorder", SetFlag2, (char **)IconNoBorder, (int *)&Scr.Look.flags},
    {"TextureMenuItemsIndividually", SetFlag2, (char **)TxtrMenuItmInd,(int *)&Scr.Look.flags},
    {"MenuMiniPixmaps", SetFlag2, (char **)MenuMiniPixmaps, (int *)&Scr.Look.flags},
    {"FrameNorth", SetFramePart, NULL, (int *)FR_N},
    {"FrameSouth", SetFramePart, NULL, (int *)FR_S},
    {"FrameEast",  SetFramePart, NULL, (int *)FR_E},
    {"FrameWest",  SetFramePart, NULL, (int *)FR_W},
    {"FrameNW", SetFramePart, NULL, (int *)FR_NW},
    {"FrameNE", SetFramePart, NULL, (int *)FR_NE},
    {"FrameSW", SetFramePart, NULL, (int *)FR_SW},
    {"FrameSE", SetFramePart, NULL, (int *)FR_SE},
    {"DecorateFrames", SetFlag2, (char **)DecorateFrames, (int *)&Scr.Look.flags},
#endif /* NO_TEXTURE */
    {"TitleTextAlign", SetInts, (char **)&Scr.Look.TitleTextAlign, &dummy},
    {"TitleButtonSpacing", SetInts, (char **)&Scr.Look.TitleButtonSpacing, (int *)&dummy},
    {"TitleButtonXOffset", SetInts, (char **)&Scr.Look.TitleButtonXOffset, (int *)&dummy},
    {"TitleButtonYOffset", SetInts, (char **)&Scr.Look.TitleButtonYOffset, (int *)&dummy},
    {"TitleButtonStyle", SetInts, (char **)&Scr.Look.TitleButtonStyle, (int *)&dummy},
    {"TitleButtonOrder", SetTButtonOrder, (char **)&(Scr.Look.button_xref[0]), (int*)&(Scr.Look.button_first_right)},
    {"TitleTextMode", SetTitleText, (char **)1, (int *)0},
    {"ResizeMoveGeometry", assign_geometry, (char**)&Scr.Look.resize_move_geometry, (int *)0},
    {"StartMenuSortMode", SetInts, (char **)&Scr.Look.StartMenuSortMode, (int *)&dummy},
    {"DrawMenuBorders", SetInts, (char **)&Scr.Look.DrawMenuBorders, (int *)&dummy},
    {"ButtonSize", SetInts, (char **)&Scr.Look.ButtonWidth, (int *)&Scr.Look.ButtonHeight},
    {"SeparateButtonTitle", SetFlag2, (char **)SeparateButtonTitle, (int *)&Scr.Look.flags},
    {"RubberBand", SetInts, (char **)&Scr.Look.RubberBand, &dummy},
    {"DefaultStyle", mystyle_parse_set_style, (char **)&Scr.Look.MSWindow[BACK_DEFAULT], (int *)0},
    {"FWindowStyle", mystyle_parse_set_style, (char **)&Scr.Look.MSWindow[BACK_FOCUSED], (int *)0},
    {"UWindowStyle", mystyle_parse_set_style, (char **)&Scr.Look.MSWindow[BACK_UNFOCUSED], (int *)0},
    {"SWindowStyle", mystyle_parse_set_style, (char **)&Scr.Look.MSWindow[BACK_STICKY], (int *)0},
    {"MenuItemStyle", mystyle_parse_set_style, (char **)&Scr.Look.MSMenu[MENU_BACK_ITEM], (int *)0},
    {"MenuTitleStyle", mystyle_parse_set_style, (char **)&Scr.Look.MSMenu[MENU_BACK_TITLE], (int *)0},
    {"MenuHiliteStyle", mystyle_parse_set_style, (char **)&Scr.Look.MSMenu[MENU_BACK_HILITE], (int *)0},
    {"MenuStippleStyle", mystyle_parse_set_style, (char **)&Scr.Look.MSMenu[MENU_BACK_STIPPLE], (int *)0},
    {"MenuSubItemStyle", mystyle_parse_set_style, (char **)&Scr.Look.MSMenu[MENU_BACK_SUBITEM], (int *)0},
    {"MenuHiTitleStyle", mystyle_parse_set_style, (char **)&Scr.Look.MSMenu[MENU_BACK_HITITLE], (int *)0},
    {"MenuItemCompositionMethod", SetInts, (char **)&Scr.Look.menu_icm, &dummy},
    {"MenuHiliteCompositionMethod", SetInts, (char **)&Scr.Look.menu_hcm, &dummy},
    {"MenuStippleCompositionMethod", SetInts, (char **)&Scr.Look.menu_scm, &dummy},
    {"ShadeAnimationSteps", SetInts, (char **)&Scr.Feel.ShadeAnimationSteps, (int *)&dummy},
    {"TitleButtonBalloonBorderHilite", bevel_parse, (char**)"afterstep", (int*)&(BalloonConfig.border_hilite)},
    {"TitleButtonBalloonXOffset", SetInts, (char**)&(BalloonConfig.x_offset), NULL},
    {"TitleButtonBalloonYOffset", SetInts, (char**)&(BalloonConfig.y_offset), NULL},
    {"TitleButtonBalloonDelay", SetInts, (char**)&(BalloonConfig.delay), NULL},
    {"TitleButtonBalloonCloseDelay", SetInts, (char**)&(BalloonConfig.close_delay), NULL},
    {"TitleButtonBalloonStyle", assign_quoted_string, &(BalloonConfig.style), NULL},
    {"TitleButtonBalloons", SetFlag2, (char**)BALLOON_USED, (int*)&(BalloonConfig.set_flags)},
    {"TitleButton", SetTitleButton, (char **)1, (int *)0},
    {"KillBackgroundThreshold", SetInts, (char**)&(Scr.Look.KillBackgroundThreshold), NULL },
	{"", 0, (char **)0, (int *)0}
};

#define PARSE_BUFFER_SIZE 	MAXLINELENGTH
char         *orig_tline = NULL;

/* the following values must not be reset other then by main
   config reading routine
 */
int           curr_conf_line = -1;
char         *curr_conf_file = NULL;

void
error_point ()
{
	fprintf (stderr, "AfterStep");
	if (curr_conf_file)
		fprintf (stderr, "(%s:%d)", curr_conf_file, curr_conf_line);
	fprintf (stderr, ":");
}

void
tline_error (const char *err_text)
{
	error_point ();
	fprintf (stderr, "%s in [%s]\n", err_text, orig_tline);
}

/***************************************************************
 **************************************************************/
void
obsolete (char *text, FILE * fd, char **arg, int *i)
{
    tline_error ("This option is obsolete. ");
}

/***************************************************************
 * get an icon
 **************************************************************/
void CheckImageManager()
{
    if( Scr.image_manager == NULL )
	{
		char *ppath = Environment->pixmap_path ;
		if( ppath == NULL )
			ppath = getenv( "IMAGE_PATH" );
		if( ppath == NULL )
			ppath = getenv( "PATH" );
		Scr.image_manager = create_image_manager( NULL, 2.2, ppath, getenv( "IMAGE_PATH" ), getenv( "PATH" ), NULL );
	}
}

Bool
GetIconFromFile (char *file, MyIcon * icon, int max_colors)
{
    CheckImageManager();
    memset( icon, 0x00, sizeof(icon_t));
    return load_icon(icon, file, Scr.image_manager );
}

ASImage *
GetASImageFromFile (char *file)
{
    ASImage *im;
    CheckImageManager();
    im = get_asimage( Scr.image_manager, file, ASFLAGS_EVERYTHING, 100 );
    if( im == NULL )
        show_error( "failed to locate icon file \"%s\" in the IconPath and PixmapPath", file );
    return im;
}

/*
 * Copies a string into a new, malloc'ed string
 * Strips all data before the second quote. and strips trailing spaces and
 * new lines
 */

char         *
stripcpy3 (const char *source, const Bool Warn)
{
	while ((*source != '"') && (*source != 0))
		source++;
	if (*source != 0)
		source++;
	while ((*source != '"') && (*source != 0))
		source++;
	if (*source == 0)
	{
		if (Warn)
			tline_error ("bad binding");
		return 0;
	}
	source++;
	return stripcpy (source);
}


/*
 * initialize the old-style look variables
 */
void
init_old_look_variables (Bool free_resources)
{
    int i ;
	if (free_resources)
	{
        /* the fonts */
		if (Stdfont != NULL)
			free (Stdfont);
		if (Windowfont != NULL)
			free (Windowfont);
		if (Iconfont != NULL)
			free (Iconfont);
        for( i = 0 ; i < BACK_STYLES ; ++i )
        {
            if( WindowForeColor[i] )
                free( WindowForeColor[i] );
            if( WindowBackColor[i] )
                free( WindowBackColor[i] );
            if( WindowGradient[i] )
                free( WindowGradient[i] );
            if( WindowPixmap[i] )
                free( WindowPixmap[i] );
        }
        for( i = 0 ; i < MENU_BACK_STYLES ; ++i )
        {
            if( MenuForeColor[i] )
                free(MenuForeColor[i]);
            if( MenuBackColor[i] )
                free(MenuBackColor[i]);
            if( MenuGradient[i] )
                free(MenuGradient[i]);
            if( MenuPixmap[i] )
                free(MenuPixmap[i]);
        }

        if( IconBgColor )
            free(IconBgColor);
        if( IconTexColor )
            free(IconTexColor);
        if( IconPixmapFile )
            free(IconPixmapFile);
        if( TexTypes )
            free(TexTypes);
    }

	/* the fonts */
	Stdfont = NULL;
	Windowfont = NULL;
	Iconfont = NULL;

    for( i = 0 ; i < BACK_STYLES ; ++i )
    {
        WindowForeColor[i] = NULL ;
        WindowBackColor[i] = NULL ;
        WindowGradient[i] = NULL ;
        WindowPixmap[i] = NULL ;
    }
    for( i = 0 ; i < MENU_BACK_STYLES ; ++i )
    {
        MenuForeColor[i] = NULL ;
        MenuBackColor[i] = NULL ;
        MenuGradient[i] = NULL ;
        MenuPixmap[i] = NULL ;
    }
    IconBgColor = NULL ;
    IconTexColor = NULL ;
    IconPixmapFile = NULL ;
    /* miscellaneous stuff */
	TexTypes = NULL;
    TitleTextType = 0;
    TitleTextY = 0;
    IconTexType = TEXTURE_BUILTIN;

}


/*
 * merge the old variables into the new styles
 * the new styles have precedence
 */
void
merge_old_look_variables (MyLook *look)
{
    char         *button_style_names[BACK_STYLES] = {"ButtonTitleFocus", "ButtonTitleUnfocus", "ButtonTitleSticky", "ButtonTitleDefault" };
    MyStyle      *button_styles[BACK_STYLES];
    int i ;

    for( i = 0 ; i < BACK_STYLES ; ++i )
        button_styles[i] = mystyle_list_find(look->styles_list, button_style_names[i]);

	/* the fonts */
	if (Stdfont != NULL)
	{
        if (load_font (Stdfont, &StdFont) == False)
            exit(1);
        else
            for( i = MENU_BACK_ITEM ; i < MENU_BACK_STYLES ; ++i )
                mystyle_inherit_font (look->MSMenu[i], &StdFont);
    }
	if (Windowfont != NULL)
	{
        if (load_font (Windowfont, &WindowFont) == False)
            exit(1);
        for( i = 0 ; i < BACK_STYLES ; ++i )
            mystyle_inherit_font (look->MSWindow[i], &WindowFont);
        mystyle_inherit_font (look->MSMenu[MENU_BACK_TITLE], &WindowFont);
	}
	if (Iconfont != NULL)
	{
        if (load_font (Iconfont, &IconFont) == False)
            exit (1);
        for( i = 0 ; i < BACK_STYLES ; ++i )
            mystyle_inherit_font (button_styles[i], &IconFont);
    }
	/* the text type */
    if (TitleTextType != 0)
	{
        for( i = 0 ; i < BACK_STYLES ; ++i )
            if (!get_flags(look->MSWindow[i]->set_flags, F_TEXTSTYLE))
            {
                set_flags(look->MSWindow[i]->text_style, TitleTextType);
                set_flags(look->MSWindow[i]->user_flags, F_TEXTSTYLE);
                set_flags(look->MSWindow[i]->set_flags, F_TEXTSTYLE);
            }
    }
	/* the colors */
	/* for black and white - ignore user choices */
	/* for color - accept user choices */
	if (Scr.d_depth > 1)
	{
        int wtype[BACK_STYLES];
        int mtype[MENU_BACK_STYLES];

        for( i = 0 ; i < BACK_STYLES ; ++i )
            wtype[i] = -1 ;
        for( i = 0 ; i < MENU_BACK_STYLES ; ++i )
            mtype[i] = -1 ;

#ifndef NO_TEXTURE
		if (TexTypes != NULL)
            sscanf (TexTypes, "%i %i %i %i %i %i", &wtype[BACK_FOCUSED], &wtype[BACK_UNFOCUSED], &wtype[BACK_STICKY],
                                                   &mtype[MENU_BACK_TITLE], &mtype[MENU_BACK_ITEM], &mtype[MENU_BACK_HILITE]);

		if (IconTexType == TEXTURE_BUILTIN)
			IconTexType = -1;
#endif /* !NO_TEXTURE */

		/* check for missing 1.4.5.x keywords */
        if (MenuForeColor[MENU_BACK_TITLE] == NULL && WindowForeColor[BACK_FOCUSED] != NULL)
            MenuForeColor[MENU_BACK_TITLE] = mystrdup(WindowForeColor[BACK_FOCUSED]);
        if (MenuBackColor[MENU_BACK_TITLE] == NULL && WindowBackColor[BACK_FOCUSED] != NULL)
            MenuBackColor[MENU_BACK_TITLE] = mystrdup(WindowBackColor[BACK_FOCUSED]);
        if (MenuForeColor[MENU_BACK_HILITE] == NULL && WindowForeColor[BACK_FOCUSED] != NULL)
            MenuForeColor[MENU_BACK_HILITE] = mystrdup(WindowForeColor[BACK_FOCUSED]);
        if (MenuBackColor[MENU_BACK_HILITE] == NULL && MenuBackColor[MENU_BACK_ITEM] != NULL)
		{
            mtype[MENU_BACK_HILITE] = mtype[MENU_BACK_ITEM];
            MenuBackColor[MENU_BACK_HILITE] = mystrdup (MenuBackColor[MENU_BACK_ITEM]);
            if (MenuGradient[MENU_BACK_HILITE] == NULL && MenuGradient[MENU_BACK_ITEM] != NULL)
                MenuGradient[MENU_BACK_HILITE] = mystrdup(MenuGradient[MENU_BACK_ITEM]);
            if (MenuPixmap[MENU_BACK_HILITE] == NULL && MenuPixmap[MENU_BACK_ITEM] != NULL)
                MenuPixmap[MENU_BACK_HILITE] = mystrdup(MenuPixmap[MENU_BACK_ITEM]);
        }
        for( i = 0 ; i < BACK_STYLES ; ++i )
            mystyle_merge_colors (look->MSWindow[i], wtype[i], WindowForeColor[i], WindowBackColor[i], WindowGradient[i], WindowPixmap[i]);
        for( i = 0 ; i < MENU_BACK_STYLES ; ++i )
            mystyle_merge_colors (look->MSMenu[i], mtype[i], MenuForeColor[i], MenuBackColor[i], MenuGradient[i], MenuPixmap[i]);

#ifndef NO_TEXTURE
		{
            MyStyle      *button_pixmap = mystyle_list_find (look->styles_list, AS_ICON_MYSTYLE);

			/* icon styles automagically inherit from window title styles */
			if (button_pixmap != NULL)
			{
                mystyle_merge_styles (look->MSWindow[BACK_FOCUSED], button_pixmap, 0, 0);
                mystyle_merge_colors (button_pixmap, IconTexType, NULL, IconBgColor, IconTexColor, IconPixmapFile);
			}
		}
#endif /* !NO_TEXTURE */
        for( i = 0 ; i < BACK_STYLES ; ++i )
            if (button_styles[i] != NULL)
                mystyle_merge_styles (look->MSWindow[i], button_styles[i], 0, 0);
    }
    init_old_look_variables (True);            /* no longer need those strings !!!! */
}


/*
 * Initialize feel variables
 */
void
InitFeel (ASFeel *feel, Bool free_resources)
{
    if (free_resources && feel)
		destroy_asfeel( feel, True );
    feel->magic = MAGIC_ASFEEL ;
	init_asfeel( feel );

}


void
ApplyFeel( ASFeel *feel )
{
    check_screen_panframes(&Scr);
}

/*
 * Initialize look variables
 */
void
InitLook (MyLook *look, Bool free_resources)
{
    /* actuall MyLook cleanup : */
    mylook_init (look, free_resources, LL_Everything );

    /* other related things : */
    if (free_resources)
	{
#ifndef NO_TEXTURE
		/* icons */
        if( MenuPinOn != NULL )
            free( MenuPinOn );
#endif /* !NO_TEXTURE */
        if( Scr.default_icon_box )
            destroy_asiconbox( &(Scr.default_icon_box));
        if( Scr.icon_boxes )
            destroy_ashash( &(Scr.icon_boxes));

        /* temporary old-style fonts : */
        unload_font (&StdFont);
        unload_font (&WindowFont);
        unload_font (&IconFont);
        DestroyMyFrameDefinitions (&MyFrameList);
        if( DefaultFrameName )
            free( DefaultFrameName );
        if( BalloonConfig.style )
            free( BalloonConfig.style );
    }
    MenuPinOn = NULL;
    MenuPinOnButton = -1;

    Scr.default_icon_box = NULL ;
    Scr.icon_boxes = NULL ;

    /* temporary old-style fonts : */
    memset(&StdFont, 0x00, sizeof(MyFont));
    memset(&WindowFont, 0x00, sizeof(MyFont));
    memset(&IconFont, 0x00, sizeof(MyFont));
    MyFrameList = NULL ;
    DefaultFrameName = NULL ;
    memset( &BalloonConfig, 0x00, sizeof(BalloonConfig));
}

void
make_styles (MyLook *look)
{
/* make sure the globals are defined */
    char *style_names[BACK_STYLES] = { "default", "FWindow", "UWindow", "SWindow" };
    char *menu_style_names[MENU_BACK_STYLES] = { "MenuTitle", "MenuItem", "MenuHilite", "MenuStipple", "MenuSubItem", "MenuHiTitle" };
    int i ;

    if (look->MSWindow[BACK_DEFAULT] == NULL)
        look->MSWindow[BACK_DEFAULT] = mystyle_list_find_or_default (look->styles_list, "default");
    for( i = 0 ; i < BACK_STYLES ; ++i )
        if (look->MSWindow[i] == NULL)
            look->MSWindow[i] = mystyle_list_new (look->styles_list, style_names[i]);

    for( i = 0 ; i < MENU_BACK_STYLES ; ++i )
        if (look->MSMenu[i] == NULL)
        {
            if( i == MENU_BACK_SUBITEM )
                look->MSMenu[i] = look->MSMenu[MENU_BACK_ITEM];
            else if( i == MENU_BACK_HITITLE )
                look->MSMenu[i] = look->MSWindow[BACK_FOCUSED];
            else
                look->MSMenu[i] = mystyle_list_new (look->styles_list, menu_style_names[i]);
        }
    if (mystyle_list_find (look->styles_list, "ButtonPixmap") == NULL)
        mystyle_list_new (look->styles_list, "ButtonPixmap");
    if (mystyle_list_find (look->styles_list, "ButtonTitleFocus") == NULL)
        mystyle_list_new (look->styles_list, "ButtonTitleFocus");
    if (mystyle_list_find (look->styles_list, "ButtonTitleSticky") == NULL)
        mystyle_list_new (look->styles_list, "ButtonTitleSticky");
    if (mystyle_list_find (look->styles_list, "ButtonTitleUnfocus") == NULL)
        mystyle_list_new (look->styles_list, "ButtonTitleUnfocus");
}

MyFrame *add_myframe_from_def( ASHashTable *list, MyFrameDefinition *fd, ASFlagType default_title_align )
{
    MyFrame *frame = create_myframe();
    int i ;
    frame->name = mystrdup( fd->name );
    for( i = 0 ; i < fd->inheritance_num ; ++i )
    {
        MyFrame *ancestor = NULL ;
        if( get_hash_item( list, AS_HASHABLE(fd->inheritance_list[i]), (void**)&ancestor ) == ASH_Success )
            inherit_myframe( frame, ancestor );
    }
    frame->parts_mask = (frame->parts_mask&(~fd->set_parts))|fd->parts_mask;
    frame->set_parts |= fd->set_parts ;
    for( i = 0 ; i < FRAME_PARTS ; ++i )
    {
        if( fd->parts[i] )
            set_string_value(&(frame->part_filenames[i]), mystrdup(fd->parts[i]), NULL, 0);
        if( get_flags( fd->set_part_size, 0x01<<i ) )
        {
            frame->part_width[i] = fd->part_width[i];
            frame->part_length[i] = fd->part_length[i];
        }
        if( IsPartFBevelSet(frame,i) )
            frame->part_fbevel[i] = fd->part_fbevel[i];
        if( IsPartUBevelSet(frame,i) )
            frame->part_ubevel[i] = fd->part_ubevel[i];
        if( IsPartSBevelSet(frame,i) )
            frame->part_sbevel[i] = fd->part_sbevel[i];
        if( get_flags( fd->set_part_align, 0x01<<i ) )
            frame->part_align[i] = fd->part_align[i];
    }
    frame->set_part_size  |= fd->set_part_size ;
    frame->set_part_bevel |= fd->set_part_bevel ;
    frame->set_part_align |= fd->set_part_align ;

    for( i = 0 ; i < FRAME_PARTS ; ++i )
        if( frame->part_filenames[i] )
            if( !get_flags( frame->set_part_align, 0x01<<i ) )
            {
                frame->part_align[i] = RESIZE_V|RESIZE_H ;
                set_flags( frame->set_part_align, 0x01<<i );
            }

    for( i = 0 ; i < BACK_STYLES ; ++i )
    {
        if( fd->title_styles[i] )
            set_string_value(&(frame->title_style_names[i]), mystrdup(fd->title_styles[i]), NULL, 0 );
        if( fd->frame_styles[i] )
            set_string_value(&(frame->frame_style_names[i]), mystrdup(fd->frame_styles[i]), NULL, 0 );
    }
    if( get_flags( fd->set_title_attr, MYFRAME_TitleFBevelSet ) )
        frame->title_fbevel = fd->title_fbevel;
    if( get_flags( fd->set_title_attr, MYFRAME_TitleUBevelSet ) )
        frame->title_ubevel = fd->title_ubevel;
    if( get_flags( fd->set_title_attr, MYFRAME_TitleSBevelSet ) )
        frame->title_sbevel = fd->title_sbevel;
    if( get_flags( fd->set_title_attr, MYFRAME_TitleAlignSet ) )
        frame->title_align = fd->title_align;
    if( get_flags( fd->set_title_attr, MYFRAME_TitleBackAlignSet ) )
        frame->title_back_align = fd->title_back_align;
    if( get_flags( fd->set_title_attr, MYFRAME_TitleFCMSet ) )
        frame->title_fcm = fd->title_fcm;
    if( get_flags( fd->set_title_attr, MYFRAME_TitleUCMSet ) )
        frame->title_ucm = fd->title_ucm;
    if( get_flags( fd->set_title_attr, MYFRAME_TitleSCMSet ) )
        frame->title_scm = fd->title_scm;

    frame->set_title_attr |= fd->set_title_attr ;

    if( fd->title_back )
    {
        set_string_value(&(frame->title_back_filename), mystrdup(fd->title_back), NULL, 0 );
        if( !get_flags( fd->set_title_attr, MYFRAME_TitleBackAlignSet ) )
        {
            frame->title_back_align = FIT_LABEL_SIZE ;
            set_flags( fd->set_title_attr, MYFRAME_TitleBackAlignSet );
        }
    }

    /* wee need to make sure that frame has such a
     * neccessary attributes as title align and title bevel : */
    if( !get_flags(frame->set_title_attr, MYFRAME_TitleFBevelSet ) )
        frame->title_fbevel = DEFAULT_TBAR_HILITE;
    if( !get_flags(frame->set_title_attr, MYFRAME_TitleUBevelSet ) )
        frame->title_ubevel = DEFAULT_TBAR_HILITE;
    if( !get_flags(frame->set_title_attr, MYFRAME_TitleSBevelSet ) )
        frame->title_sbevel = DEFAULT_TBAR_HILITE;
    if( !get_flags(frame->set_title_attr, MYFRAME_TitleAlignSet ) )
        frame->title_align = default_title_align;
    set_flags( frame->set_title_attr, MYFRAME_TitleBevelSet|MYFRAME_TitleAlignSet );

    if( add_hash_item( list, AS_HASHABLE(frame->name), frame ) != ASH_Success )
        destroy_myframe( &frame );
    else
    {
        LOCAL_DEBUG_OUT( "added frame with the name \"%s\"", frame->name );
    }
    return frame;
}

void
fix_menu_pin_on( MyLook *look )
{
    if (MenuPinOn != NULL)
    {
        if( MenuPinOnButton < 0 || MenuPinOnButton >= TITLE_BUTTONS )
        {
            register int i = TITLE_BUTTONS ;
            while ( --i >= 0 )
            {
                if( look->buttons[i].unpressed.image == NULL && look->buttons[i].pressed.image == NULL )
                    break;
            }
            if( i >= 0 )
            {
                if( GetIconFromFile (MenuPinOn, &(Scr.Look.buttons[i].unpressed), 0) )
                {
                    int context = C_TButton0<<i ;
                    register int k ;
                    Scr.Look.buttons[i].width = Scr.Look.buttons[i].unpressed.image->width ;
                    Scr.Look.buttons[i].height = Scr.Look.buttons[i].unpressed.image->height ;
                    MenuPinOnButton = i ;

                    if( Scr.Look.buttons[i].context == C_NO_CONTEXT )
                        Scr.Look.buttons[i].context = context ;
                    context = Scr.Look.buttons[i].context ;
                    for( k = 0 ; k < TITLE_BUTTONS ; ++k )
                        if( Scr.Look.button_xref[k] == context )
                            break;
                    if( k == TITLE_BUTTONS )
                    {
                        while( --k >= 0 )
                            if( Scr.Look.button_xref[k] == C_NO_CONTEXT )
                            {
                                Scr.Look.button_xref[k] =context ;
                                break;
                            }
                    }
                    if( k >= 0 && k < TITLE_BUTTONS )
                        Scr.Look.ordered_buttons[k] = &(Scr.Look.buttons[i]) ;
                    else
                        show_warning( "there is no slot on the titlebar to place button %d into. Check yout TitleButtonOrder setting.", i );
                }else
                    MenuPinOnButton = -1 ;
            }
        }
        if( MenuPinOnButton >= 0 )
        {
            static char binding[128];
            sprintf( binding, "1 %d A PinMenu\n", MenuPinOnButton );
            /* also need to add mouse binding for this one */
            ParseMouseEntry (binding, NULL, NULL, NULL);
        }
        show_warning( "MenuPinOn setting is depreciated - instead add a Title button and bind PinMenu function to it." );
    }
#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing %s","");
    ASSync(False);
#endif

}

void
FixLook( MyLook *look )
{
    ASFlagType default_title_align = ALIGN_LEFT ;
    int i ;
#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing %s","");
    ASSync(False);
#endif
    /* make sure all needed styles are created */
    make_styles (look);
#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing %s","");
    ASSync(False);
#endif

    /* merge pre-1.5 compatibility keywords */
    merge_old_look_variables (look);
#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing %s","");
    ASSync(False);
#endif

    /* fill in remaining members with the default style */
    mystyle_list_fix_styles (look->styles_list);
#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing %s","");
    ASSync(False);
#endif

    mystyle_list_set_property (Scr.wmprops, look->styles_list);
#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing %s","");
    ASSync(False);
#endif

    if(look->TitleTextAlign == JUSTIFY_RIGHT )
        default_title_align = ALIGN_RIGHT ;
    else if(look->TitleTextAlign == JUSTIFY_CENTER )
        default_title_align = ALIGN_CENTER ;

#ifndef NO_TEXTURE
    /* update frame geometries */
    if (get_flags( look->flags, DecorateFrames))
    {
        MyFrameDefinition *fd ;
        MyFrame *frame ;
        /* TODO: need to load the list as well (if we have any )*/
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
        PrintMyFrameDefinitions (MyFrameList, 1);
#endif
        LOCAL_DEBUG_OUT( "MyFrameList %p", MyFrameList );
        if( MyFrameList )
            check_myframes_list( look );
        for( fd = MyFrameList ; fd != NULL ; fd = fd->next )
        {
            LOCAL_DEBUG_OUT( "processing MyFrameDefinition %p", fd );
            frame = add_myframe_from_def( look->FramesList, fd, default_title_align|ALIGN_VCENTER );
            myframe_load ( frame, Scr.image_manager );
        }
        DestroyMyFrameDefinitions (&MyFrameList);
        if( look->DefaultFrame != NULL )
            myframe_load ( look->DefaultFrame, Scr.image_manager );
        else if( DefaultFrameName != NULL )
        {
            get_hash_item( look->FramesList, AS_HASHABLE(DefaultFrameName), (void**)&(look->DefaultFrame));
            LOCAL_DEBUG_OUT( "DefaultFrameName is \"%s\": found frame %p with that name.", DefaultFrameName, look->DefaultFrame );
        }
    }
#endif /* ! NO_TEXTURE */
    if( look->DefaultFrame == NULL )
        look->DefaultFrame = create_default_myframe(default_title_align|ALIGN_VCENTER);

#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing %s","");
    ASSync(False);
#endif

    /* checking that all the buttons have assigned slots in the button xref : */
    for( i = 0 ; i < TITLE_BUTTONS ; ++i )
    {
        if( Scr.Look.buttons[i].unpressed.image != NULL )
        {
            int context = C_TButton0<<i ;
            register int k ;
            if( Scr.Look.buttons[i].context == C_NO_CONTEXT )
                Scr.Look.buttons[i].context = context ;
            context = Scr.Look.buttons[i].context ;
            for( k = 0 ; k < TITLE_BUTTONS ; ++k )
                if( Scr.Look.button_xref[k] == context )
                    break;
            if( k == TITLE_BUTTONS )
            {
                while( --k >= 0 )
                    if( Scr.Look.button_xref[k] == C_NO_CONTEXT )
                    {
                        Scr.Look.button_xref[k] =context ;
                        break;
                    }
            }
            if( k >= 0 && k < TITLE_BUTTONS )
                Scr.Look.ordered_buttons[k] = &(Scr.Look.buttons[i]) ;
            else
                show_warning( "there is no slot on the titlebar to place button %d into. Check yout TitleButtonOrder setting.", i );
        }
    }

#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing %s","");
    ASSync(False);
#endif
    /* updating balloons look */
    if( BalloonConfig.style == NULL )
        BalloonConfig.style = mystrdup( "TitleButtonBalloon" );

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
    Print_balloonConfig ( &BalloonConfig );
#endif
    balloon_config2look( look, &BalloonConfig );
    set_balloon_look( look->balloon_look );

    /* checking sanity of the move-resize window geometry :*/
    if( (look->resize_move_geometry.flags&(HeightValue|WidthValue)) != (HeightValue|WidthValue))
	{
		int width = 0;
		int height = 0 ;
		get_text_size( " +88888 x +88888 ", look->MSWindow[BACK_FOCUSED]->font.as_font, look->MSWindow[BACK_FOCUSED]->text_style, &width, &height );
    	if( !get_flags(look->resize_move_geometry.flags, WidthValue ) )
	        look->resize_move_geometry.width = width + SIZE_VINDENT * 2;

	    if( !get_flags(look->resize_move_geometry.flags, HeightValue ) )
    	    look->resize_move_geometry.height = height + SIZE_VINDENT * 2;

	    set_flags( look->resize_move_geometry.flags, HeightValue|WidthValue );
	}
    if( look->supported_hints == NULL )
    {
        look->supported_hints = create_hints_list();
        enable_hints_support( look->supported_hints, HINTS_ICCCM );
        enable_hints_support( look->supported_hints, HINTS_Motif );
        enable_hints_support( look->supported_hints, HINTS_Gnome );
        enable_hints_support( look->supported_hints, HINTS_ExtendedWM );
        enable_hints_support( look->supported_hints, HINTS_ASDatabase );
        enable_hints_support( look->supported_hints, HINTS_GroupLead );
        enable_hints_support( look->supported_hints, HINTS_Transient );
    }
    switch( look->TitleButtonStyle )
    {
        case 0 :
            look->TitleButtonXOffset = 3;
            look->TitleButtonYOffset = 3;
            break ;
        case 1 :
            look->TitleButtonXOffset = 1;
            look->TitleButtonYOffset = 1;
            break ;
    }

    /* now we need to go through all the deskconfigs and create generic
     * MyBackground for those that alreadyu do not have one */
    if( look->desk_configs )
    {
        ASHashIterator it ;
        if( start_hash_iteration( look->desk_configs, &it ) )
            do
            {
                MyDesktopConfig *dc = (MyDesktopConfig *)curr_hash_data( &it );
                MyBackground *myback = mylook_get_back(look, dc->back_name);
                if( myback == NULL && dc->back_name != NULL )
                {
                    myback = create_myback( dc->back_name );
                    myback->type = MB_BackImage ;
                    myback->data = mystrdup( dc->back_name );
                    add_myback( look, myback );
                }
            }while( next_hash_item( &it ) );
    }else if( !get_flags( Scr.Look.flags, DontDrawBackground ) )
	{
		MyDesktopConfig *dc ;
		MyBackground *myback ;

	    dc = create_mydeskconfig( 0, "default_background" );
		add_deskconfig( &(Scr.Look), dc );
       	myback = mylook_get_back(look, dc->back_name);
        if( myback == NULL  )
        {
            myback = create_myback( dc->back_name );
            myback->type = MB_BackImage ;
            add_myback( look, myback );
		}
	}
#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing %s","");
    ASSync(False);
#endif
}

/*
 * Initialize database variables
 */

void
InitDatabase (Bool free_resources)
{
	if (free_resources)
    {
        destroy_asdb( &Database );
        /* XResources : */
        destroy_user_database();
    }else
        Database = NULL ;
}

/*
 * Create/destroy window titlebar/buttons as necessary.
 */
Bool
redecorate_aswindow_iter_func(void *data, void *aux_data)
{
    ASWindow *asw = (ASWindow*)data;
    if(asw )
	{
        redecorate_window( asw, False );
        if( asw->internal && asw->internal->on_look_feel_changed )
            asw->internal->on_look_feel_changed( asw->internal, &Scr.Feel, &Scr.Look, ASFLAGS_EVERYTHING );
        on_window_status_changed( asw, True, False );
    }
    return True;
}


/*************************************************************************/
/* reading confiug files now :                                           */
/*************************************************************************/
void
ParseDatabase (const char *file)
{
    struct name_list *list = NULL ;

	/* memory management for parsing buffer */
	if (file == NULL)
        return ;

    list = ParseDatabaseOptions (file, "afterstep");
    if( list )
    {
        Database = build_asdb( list );
        if( is_output_level_under_threshold( OUTPUT_LEVEL_DATABASE ) )
            print_asdb( fprintf, stderr, Database );
        while (list != NULL)
            delete_name_list (&(list));
    }else
        show_progress( "no database records loaded." );
    /* XResources : */
    load_user_database();
}


int
ParseConfigFile (const char *file, char **tline)
{
    FILE         *fp = NULL;
	register char *ptr;

	/* memory management for parsing buffer */
	if (file == NULL)
		return -1;

    /* this should not happen, but still checking */
    if ((fp = fopen (file, "r")) == (FILE *) NULL)
	{
        show_error("can't open config file [%s] - skipping it for now.\nMost likely you have incorrect permissions on the AfterStep configuration dir.",
             file);
        return -1;
	}

	if (*tline == NULL)
		*tline = safemalloc (MAXLINELENGTH + 1);

	curr_conf_file = (char *)file;
	curr_conf_line = 0;
	while (fgets (*tline, MAXLINELENGTH, fp))
	{
		curr_conf_line++;
		/* prventing buffer overflow */
		*((*tline) + MAXLINELENGTH) = '\0';
		/* remove comments from the line */
		ptr = stripcomments (*tline);
		/* parsing the line */
		orig_tline = ptr;
        if (*ptr != '\0' && *ptr != '#')
			match_string (main_config, ptr, "error in config:", fp);
	}
	curr_conf_file = NULL;
	fclose (fp);
	return 1;
}

/*****************************************************************************
 *****************************************************************************
 * This routine is responsible for reading and parsing the config file
 ****************************************************************************
 ****************************************************************************/
/* MakeMenus - for those who can't remember LoadASConfig's real name        */
void
LoadASConfig (int thisdesktop, ASFlagType what)
{
    char            *tline = NULL;
    ASImageManager  *old_image_manager = NULL ;
    ASFontManager   *old_font_manager  = NULL ;

#ifndef DIFFERENTLOOKNFEELFOREACHDESKTOP
	/* only one look & feel should be used */
	thisdesktop = 0;
#endif /* !DIFFERENTLOOKNFEELFOREACHDESKTOP */

    show_progress("Loading configuration files ...");
    if (Session->overriding_file == NULL )
	{
        char *configfile;
        const char *const_configfile;
        if (get_flags(what, PARSE_BASE_CONFIG))
		{
			if( ReloadASEnvironment( &old_image_manager, &old_font_manager, NULL, get_flags(what, PARSE_LOOK_CONFIG) ) )
			{
				if( !get_flags(what, PARSE_LOOK_CONFIG) )
				{
					if( old_image_manager != NULL || old_font_manager != NULL )
					{
                    	InitLook (&Scr.Look, True);
                    	set_flags(what, PARSE_LOOK_CONFIG);
					}
				}else
                	clear_flags(what, PARSE_BASE_CONFIG);
            }
        }else if(get_flags(what, PARSE_LOOK_CONFIG))
		{  /* must reload Image manager so that changed images would get updated */
			ReloadASImageManager( &old_image_manager );
		}

        if (get_flags(what, PARSE_LOOK_CONFIG))
		{
			LoadColorScheme();

			/* now we can proceed to loading them look and theme */
            if( (const_configfile = get_session_file (Session, thisdesktop, F_CHANGE_LOOK) ) != NULL )
            {
                InitLook (&Scr.Look, True);
                ParseConfigFile (const_configfile, &tline);
                show_progress("LOOK configuration loaded from \"%s\" ...", const_configfile);
#ifdef ASETROOT_FILE
				if( Scr.Look.desk_configs == NULL )
				{/* looks like there is no background information in the look file and we should be
				    getting it from the asetroot file : */

		            if( (configfile = make_session_file(Session, ASETROOT_FILE, False )) != NULL )
      			    {
      			        ParseConfigFile (configfile, &tline);
              			/* Save base filename to pass to modules */
				        show_progress("ROOT BACKGROUND configuration loaded from \"%s\" ...", configfile);
              			free( configfile );
		            }
				}
#endif
            }else
            {
                show_warning("LOOK configuration file cannot be found");
                clear_flags(what, PARSE_LOOK_CONFIG);
            }
        }
        if (get_flags(what, PARSE_FEEL_CONFIG))
		{
            if( (const_configfile = get_session_file (Session, thisdesktop, F_CHANGE_FEEL) ) != NULL )
            {
                InitFeel (&Scr.Feel, True);
                if (tline == NULL)
                    tline = safemalloc (MAXLINELENGTH + 1);
                MeltStartMenu (tline);
                ParseConfigFile (const_configfile, &tline);
                if( (configfile = make_session_file(Session, AUTOEXEC_FILE, False )) != NULL )
                {
                    ParseConfigFile (configfile, &tline);
                    show_progress("AUTOEXEC configuration loaded from \"%s\" ...", configfile);
                    free( configfile );
                }else
                    show_warning("AUTOEXEC configuration file cannot be found");
                show_progress("FEEL configuration loaded from \"%s\" ...", const_configfile);
            }else
            {
                show_warning("FEEL configuration file cannot be found");
                clear_flags(what, PARSE_FEEL_CONFIG);
            }
        }
        if (get_flags(what, PARSE_LOOK_CONFIG|PARSE_FEEL_CONFIG))
		{
            if( (const_configfile = get_session_file (Session, thisdesktop, F_CHANGE_THEME) ) != NULL )
            {
                ParseConfigFile (const_configfile, &tline);
                show_progress("THEME configuration loaded from \"%s\" ...", const_configfile);
                if( (configfile = make_session_data_file  (Session, False, R_OK, THEME_OVERRIDE_FILE, NULL )) != NULL )
                {
                    ParseConfigFile (configfile, &tline);
                    show_progress("THEME OVERRIDES configuration loaded from \"%s\" ...", configfile);
                    free( configfile );
                }
            }
        }

        if (get_flags(what, PARSE_DATABASE_CONFIG))
        {
            if( (configfile = make_session_file(Session, DATABASE_FILE, False )) != NULL )
            {
                InitDatabase (True);
                ParseDatabase (configfile);
                show_progress("DATABASE configuration loaded from \"%s\" ...", configfile);
                free( configfile );
            }else
            {
                show_warning("DATABASE configuration file cannot be found");
                clear_flags(what, PARSE_DATABASE_CONFIG);
            }
        }
	} else
	{
		ReloadASEnvironment( &old_image_manager, &old_font_manager, NULL, True );
		LoadColorScheme();
		InitLook (&Scr.Look, True);
        InitFeel (&Scr.Feel, True);
        InitDatabase (True);
        ParseConfigFile (Session->overriding_file, &tline);
        show_progress("AfterStep configuration loaded from \"%s\" ...", Session->overriding_file);
        what = PARSE_EVERYTHING ;
    }

	/* let's free the memory used for parsing */
	if (tline)
		free (tline);
    show_progress("Done loading configuration.");

    check_desksize_sanity( &Scr );
    check_feel_sanity( &Scr.Feel );

    if (get_flags(what, PARSE_FEEL_CONFIG))
        ApplyFeel( &Scr.Feel );

    if (get_flags(what, PARSE_LOOK_CONFIG))
    {
        FixLook( &Scr.Look );
        if( thisdesktop == Scr.CurrentDesk )
            change_desktop_background( Scr.CurrentDesk, Scr.CurrentDesk );
    }

    if( get_flags(what, PARSE_LOOK_CONFIG|PARSE_FEEL_CONFIG))
        fix_menu_pin_on( &Scr.Look );


    /* TODO: update the menus */
    if (get_flags(what, PARSE_BASE_CONFIG|PARSE_LOOK_CONFIG|PARSE_FEEL_CONFIG))
	{
    }

    /* force update of window frames */
    if (get_flags(what, PARSE_BASE_CONFIG|PARSE_LOOK_CONFIG|PARSE_FEEL_CONFIG|PARSE_DATABASE_CONFIG))
        iterate_asbidirlist( Scr.Windows->clients, redecorate_aswindow_iter_func, NULL, NULL, False );

    if( old_image_manager && old_image_manager != Scr.image_manager )
    {
        if(Scr.RootImage && Scr.RootImage->imageman == old_image_manager )
        {
            safe_asimage_destroy(Scr.RootImage);
            Scr.RootImage = NULL ;
        }
        destroy_image_manager( old_image_manager, False );
    }
    if( old_font_manager && old_font_manager != Scr.font_manager )
        destroy_font_manager( old_font_manager, False );
}

/*****************************************************************************
 *  This series of functions do actuall parsing of config options on per-line
 *  basis :
 *****************************************************************************/
/*****************************************************************************
 * Copies a text string from the config file to a specified location
 ****************************************************************************/

void
assign_string (char *text, FILE * fd, char **arg, int *junk)
{
	*arg = stripcpy (text);
}

void
assign_quoted_string (char *text, FILE * fd, char **arg, int *junk)
{
    *arg = stripcpy2 (text, 0);
}

/*****************************************************************************
 *
 * Copies a PATH string from the config file to a specified location
 *
 ****************************************************************************/

void
assign_themable_path (char *text, FILE * fd, char **arg, int *junk)
{
    char         *tmp = stripcpy (text);
	int           tmp_len;
    char         *as_theme_data = NULL ; /*make_session_dir(Session, ICON_DIR, False); */

	replaceEnvVar (&tmp);
    if( as_theme_data )
    {
        tmp_len = strlen (tmp);
        *arg = safemalloc (tmp_len + 1 + strlen (as_theme_data) + 1);
        strcpy (*arg, tmp);
        (*arg)[tmp_len] = ':';
        strcpy ((*arg) + tmp_len + 1, as_theme_data);
        free (tmp);
        free (as_theme_data);
    }else
        *arg = tmp;
}


void
assign_path (char *text, FILE * fd, char **arg, int *junk)
{
	*arg = stripcpy (text);
	replaceEnvVar (arg);
}

void
assign_geometry (char *text, FILE * fd, char **arg, int *junk)
{
    ASGeometry *geom = (ASGeometry*)arg ;

    geom->x = geom->y = 0;
    geom->width = geom->height = 1;
    geom->flags = 0 ;
    parse_geometry (text, &(geom->x), &(geom->y), &(geom->width), &(geom->height), &(geom->flags));
}

/*****************************************************************************
 * Loads a pixmap to the assigned location
 ****************************************************************************/
void
assign_pixmap (char *text, FILE * fd, char **arg, int *junk)
{
    char *fname = NULL ;
    if( parse_filename(text, &fname) != text )
    {
        MyIcon **picon = (MyIcon**)arg ;
        *picon = safecalloc( 1, sizeof(icon_t));
        GetIconFromFile (fname, *picon, -1);
        free (fname);
    }
}

/****************************************************************************
 *  Read TitleText Controls
 ****************************************************************************/

void
SetTitleText (char *tline, FILE * fd, char **junk, int *junk2)
{
#ifndef NO_TEXTURE
	int           ttype, y;

    sscanf (tline, "%d %d", &ttype, &y);
    TitleTextType = ttype;
    TitleTextY = y;
#endif /* !NO_TEXTURE */
}

/****************************************************************************
 *
 *  Read Titlebar pixmap button
 *
 ****************************************************************************/

void
SetTitleButton (char *tline, FILE * fd, char **junk, int *junk2)
{
	int           num;
    char          *files[2] = {NULL, NULL};
    int           offset = 0;
	int           n;

    if ((n = sscanf (tline, "%d", &num)) <= 0)
	{
        show_error("wrong number of parameters given with TitleButton");
		return;
	}
    if (num < 0 || num >= TITLE_BUTTONS)
	{
        show_error("invalid Titlebar button number: %d", num);
		return;
	}

    /* going the hard way to prevent buffer overruns */
    while (isspace (tline[offset]))   offset++;
    while (isdigit (tline[offset]))   offset++;
    while (isspace (tline[offset]))   offset++;

    tline = parse_filename( &(tline[offset]), &(files[0]));
    offset = 0 ;
    while( isspace(tline[offset]) ) ++offset;
    if( tline[offset] != '\0' )
        parse_filename( &(tline[offset]), &(files[1]));

    if( !load_button( &(Scr.Look.buttons[num]), files, Scr.image_manager ) )
        show_error( "wrong number of parameters given with TitleButton");
    if( files[0] )
        free(files[0] ) ;
    if( files[1] )
        free(files[1] ) ;
}

/*****************************************************************************
 *
 * Changes a cursor def.
 *
 ****************************************************************************/

void
SetCursor (char *text, FILE * fd, char **arg, int *junk)
{
	int           num, cursor_num, cursor_style;

	num = sscanf (text, "%d %d", &cursor_num, &cursor_style);
	if ((num != 2) || (cursor_num >= MAX_CURSORS) || (cursor_num < 0))
		tline_error ("bad Cursor");
	else
    {
        Cursor new_c = XCreateFontCursor (dpy, cursor_style);
        if( new_c )
        {
            if( Scr.Feel.cursors[cursor_num] && Scr.Feel.cursors[cursor_num] != Scr.standard_cursors[cursor_num])
                XFreeCursor( dpy, Scr.Feel.cursors[cursor_num] );
            Scr.Feel.cursors[cursor_num] = new_c ;
        }
    }
}

void
SetCustomCursor (char *text, FILE * fd, char **arg, int *junk)
{
	int           num, cursor_num;
	char          f_cursor[1024], f_mask[1024];
	Pixmap        cursor = None, mask = None;
	int           width, height, x, y;
	XColor        fore, back;
	char         *path;
    Cursor new_c ;

	num = sscanf (text, "%d %s %s", &cursor_num, f_cursor, f_mask);
	if ((num != 3) || (cursor_num >= MAX_CURSORS) || (cursor_num < 0))
	{
		tline_error ("bad Cursor");
		return;
	}

	path = findIconFile (f_mask, Environment->cursor_path, R_OK);
	if (path)
	{
		XReadBitmapFile (dpy, Scr.Root, path, &width, &height, &mask, &x, &y);
		free (path);
	} else
	{
		tline_error ("Cursor mask not found");
		return;
	}

	path = findIconFile (f_cursor, Environment->cursor_path, R_OK);
	if (path)
	{
		XReadBitmapFile (dpy, Scr.Root, path, &width, &height, &cursor, &x, &y);
		free (path);
	} else
	{
		tline_error ("cursor bitmap not found");
		return;
	}

	fore.pixel = Scr.asv->black_pixel;
	back.pixel = Scr.asv->white_pixel;
	XQueryColor (dpy, Scr.asv->colormap, &fore);
	XQueryColor (dpy, Scr.asv->colormap, &back);

	if (cursor == None || mask == None)
	{
		tline_error ("unrecognized format for cursor");
		return;
	}

    new_c = XCreatePixmapCursor (dpy, cursor, mask, &fore, &back, x, y);
    if( new_c )
    {
        if( Scr.Feel.cursors[cursor_num] && Scr.Feel.cursors[cursor_num] != Scr.standard_cursors[cursor_num])
            XFreeCursor( dpy, Scr.Feel.cursors[cursor_num] );
        Scr.Feel.cursors[cursor_num] = new_c ;
    }
    XFreePixmap (dpy, mask);
    XFreePixmap (dpy, cursor);
    ASSync(False);
    LOCAL_DEBUG_OUT( "mask %lX and cursor %lX freed", mask, cursor );
}

/*****************************************************************************
 *
 * Sets a boolean flag to true
 *
 ****************************************************************************/

void
SetFlag (char *text, FILE * fd, char **arg, int *another)
{
    Scr.Feel.flags |= (unsigned long)arg;
	if (another)
	{
		long          i = strtol (text, NULL, 0);
		if (i)
            Scr.Feel.flags |= (unsigned long)another;
	}
}

void
SetFlag2 (char *text, FILE * fd, char **arg, int *var)
{
	unsigned long *flags = (unsigned long *)var;
	char         *ptr;
	int           val = strtol (text, &ptr, 0);

	if (flags == NULL)
        flags = &Scr.Feel.flags;
	if (ptr != text && val == 0)
		*flags &= ~(unsigned long)arg;
	else
		*flags |= (unsigned long)arg;
}

/*****************************************************************************
 *
 * Reads in one or two integer values
 *
 ****************************************************************************/

void
SetInts (char *text, FILE * fd, char **arg1, int *arg2)
{
    sscanf (text, "%d%*c%d", (int *)arg1, (int *)arg2);
/*    LOCAL_DEBUG_OUT( "text=[%s], arg1=%p, Scr.Feel.Autoreverse = %p, res = %d", text, arg1, &(Scr.Feel.AutoReverse), *((int*)arg1) );*/
}

/*****************************************************************************
 *
 * Reads in a list of mouse button numbers
 *
 ****************************************************************************/

void
SetButtonList (char *text, FILE * fd, char **arg1, int *arg2)
{
	int           i, b;
	char         *next;

	for (i = 0; i < MAX_MOUSE_BUTTONS; i++)
	{
		b = (int)strtol (text, &next, 0);
		if (next == text)
			break;
		text = next;
		if (*text == ',')
			text++;
		if ((b > 0) && (b <= MAX_MOUSE_BUTTONS))
            Scr.Feel.RaiseButtons |= 1 << b;
	}
    set_flags(Scr.Feel.flags, ClickToRaise);
}


/*****************************************************************************
 *
 * Reads Dimensions for an icon box from the config file
 *
 ****************************************************************************/

void
SetBox (char *text, FILE * fd, char **arg, int *junk)
{
    int x1 = 0, y1 = 0, x2 = Scr.MyDisplayWidth, y2 = Scr.MyDisplayHeight ;
    int num;

    /* not a standard X11 geometry string :*/
	num = sscanf (text, "%d%d%d%d", &x1, &y1, &x2, &y2);

	/* check for negative locations */
	if (x1 < 0)
		x1 += Scr.MyDisplayWidth;
	if (y1 < 0)
		y1 += Scr.MyDisplayHeight;

	if (x2 < 0)
		x2 += Scr.MyDisplayWidth;
	if (y2 < 0)
		y2 += Scr.MyDisplayHeight;

    if (x1 >= x2 || y1 >= y2 ||
		x1 < 0 || x1 > Scr.MyDisplayWidth || x2 < 0 || x2 > Scr.MyDisplayWidth ||
		y1 < 0 || y1 > Scr.MyDisplayHeight || y2 < 0 || y2 > Scr.MyDisplayHeight)
    {
        show_error("invalid IconBox '%s'", text);
    }else
	{
        int box_no = Scr.Look.configured_icon_areas_num;
        Scr.Look.configured_icon_areas = realloc( Scr.Look.configured_icon_areas, (box_no+1)*sizeof(ASGeometry));
        Scr.Look.configured_icon_areas[box_no].x = x1 ;
        Scr.Look.configured_icon_areas[box_no].y = y1 ;
        Scr.Look.configured_icon_areas[box_no].width = x2-x1 ;
        Scr.Look.configured_icon_areas[box_no].height = y2-y1 ;
        Scr.Look.configured_icon_areas[box_no].flags = XValue|YValue|WidthValue|HeightValue ;
        if( x1 > Scr.MyDisplayWidth-x2 )
            Scr.Look.configured_icon_areas[box_no].flags |= XNegative ;
        if( y1 > Scr.MyDisplayHeight-y2 )
            Scr.Look.configured_icon_areas[box_no].flags |= YNegative ;
        ++Scr.Look.configured_icon_areas_num;
    }
}

void
SetFramePart (char *text, FILE * fd, char **frame, int *id)
{
    char *fname = NULL;
    MyFrame *pframe  = (MyFrame*)frame;
    if( parse_filename (text, &fname) != text )
    {
        if( pframe == NULL )
        {
            if( Scr.Look.DefaultFrame == NULL )
                Scr.Look.DefaultFrame = create_myframe();
            pframe = Scr.Look.DefaultFrame;
        }
        filename2myframe_part (pframe, (int)id, fname);
        free( fname );
    }
}

void
SetModifier (char *text, FILE * fd, char **mod, int *junk2)
{
    int *pmod = (int*)mod ;
    if( pmod )
        *pmod = parse_modifier( text );
}

void
SetTButtonOrder(char *text, FILE * fd, char **pxref, int *prbtn)
{
    unsigned int *xref = (unsigned int*)pxref ;
    unsigned int *rbtn = (unsigned int*)prbtn ;
    if( xref && rbtn )
    {
        register int i = 0, btn = 0;
        *rbtn = TITLE_BUTTONS ;
        while( !isspace(text[i]) && text[i] != '\0' )
        {
            int context = C_NO_CONTEXT ;
            switch(text[i])
            {
                case '0' : context = C_TButton0;  break ;
                case '1' : context = C_TButton1;  break ;
                case '2' : context = C_TButton2;  break ;
                case '3' : context = C_TButton3;  break ;
                case '4' : context = C_TButton4;  break ;
                case '5' : context = C_TButton5;  break ;
                case '6' : context = C_TButton6;  break ;
                case '7' : context = C_TButton7;  break ;
                case '8' : context = C_TButton8;  break ;
                case '9' : context = C_TButton9;  break ;
                case 'T' :
                case 't' : context = C_TITLE ; break;
                default:
                    show_warning("invalid context specifier '%c' in TitleButtonOrder setting", text[i] );
            }
            if( context == C_TITLE )
            {
                *rbtn = btn ;
            }else if( context != C_NO_CONTEXT )
            {
                xref[btn] = context ;

                if( ++btn >= TITLE_BUTTONS )
                    break;
            }
            ++i;
        }
        while( btn < TITLE_BUTTONS )
        {
            xref[btn] = C_NO_CONTEXT ;
            ++btn ;
        }
    }
}


/****************************************************************************
 *
 * These routines put together files from start directory
 *
 ***************************************************************************/

/* buf must be at least MAXLINELENGTH chars long */
int
MeltStartMenu (char *buf)
{
	char         *as_start = NULL;
	dirtree_t    *tree;

    switch (Scr.Look.StartMenuSortMode)
	{
	 case SORTBYALPHA:
		 dirtree_compar_list[0] = dirtree_compar_order;
		 dirtree_compar_list[1] = dirtree_compar_type;
		 dirtree_compar_list[2] = dirtree_compar_alpha;
		 dirtree_compar_list[3] = NULL;
		 break;

	 case SORTBYDATE:
		 dirtree_compar_list[0] = dirtree_compar_order;
		 dirtree_compar_list[1] = dirtree_compar_type;
		 dirtree_compar_list[2] = dirtree_compar_mtime;
		 dirtree_compar_list[3] = NULL;
		 break;

	 default:
		 dirtree_compar_list[0] = NULL;
		 break;
	}

	/*
	 *    Here we test the existence of various
	 *    directories used for the generation.
	 */

    as_start = make_session_dir (Session, START_DIR, False);
    tree = dirtree_new_from_dir (as_start);
    show_progress("MENU loaded from \"%s\" ...", as_start);
	free (as_start);

#ifdef FIXED_DIR
	{
        char         *as_fixeddir = make_session_dir (Session, FIXED_DIR, False);

		if (CheckDir (as_fixeddir) == 0)
		{
			dirtree_t    *fixed_tree = dirtree_new_from_dir (as_fixeddir);

			free (as_fixeddir);
			dirtree_move_children (tree, fixed_tree);
			dirtree_delete (fixed_tree);
            show_progress("FIXED MENU loaded from \"%s\" ...", as_fixeddir);
		} else
            show_error("unable to locate the fixed menu directory at \"%s\"", as_fixeddir);
		free (as_fixeddir);
	}
#endif /* FIXED_DIR */

	dirtree_parse_include (tree);
	dirtree_remove_order (tree);
	dirtree_merge (tree);
	dirtree_sort (tree);
	dirtree_set_id (tree, 0);
	/* make sure one copy of the root menu uses the name "0" */
	(*tree).flags &= ~DIRTREE_KEEPNAME;

    dirtree_make_menu2 (tree, buf, True);
	/* to keep backward compatibility, make a copy of the root menu with
	 * the name "start" */
	{
		if ((*tree).name != NULL)
			free ((*tree).name);
		(*tree).name = mystrdup ("start");
		(*tree).flags |= DIRTREE_KEEPNAME;
        dirtree_make_menu2 (tree, buf, False);
	}
	/* cleaning up cache of the searcher */
	is_executable_in_path (NULL);

	dirtree_delete (tree);
	return 0;
}

void deskback_parse(char *text, FILE * fd, char **junk, int *junk2)
{
    register int i = 0;
    int desk = atoi(text);
    char *data = NULL ;
    MyDesktopConfig *dc = NULL ;

    if( !IsValidDesk(desk) )
    {
        show_error( "invalid desktop number in: \"%s\"", text);
        return;
    }

    while( isdigit(text[i]) ) ++i ;
    if( i == 0 || !isspace(text[i]) )
    {
        show_error( "missing desktop number in: \"%s\"", text);
        return;
    }

    while( isspace(text[i]) ) ++i ;
    if( text[i] == '#' )
        return;

    data = stripcpy2 (&(text[i]), 0);
    if( data == NULL )
    {
        show_error( "DeskBack option with no name of the relevant MyBackground: \"%s\"", text);
        return ;
    }
    LOCAL_DEBUG_OUT("desk(%d)->data(\"%s\")->text(%s)", desk, data, text );
    dc = create_mydeskconfig( desk, data );
    add_deskconfig( &(Scr.Look), dc );
    free( data );
}

/****************************************************************************
 *
 * Matches text from config to a table of strings, calls routine
 * indicated in table.
 *
 ****************************************************************************/

void
match_string (struct config *table, char *text, char *error_msg, FILE * fd)
{
    register int i ;
    table = find_config (table, text);
	if (table != NULL)
	{
        i = strlen (table->keyword);
        while(isspace(text[i])) ++i;
        table->action (&(text[i]), fd, table->arg, table->arg2);
	} else
		tline_error (error_msg);
}


