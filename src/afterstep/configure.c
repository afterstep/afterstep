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

#include "../../configure.h"

#include "../../include/asapp.h"
#include <unistd.h>

#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/mystyle.h"
#include "../../include/decor.h"
#include "../../include/screen.h"
#include "../../include/loadimg.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"
#include "../../include/balloon.h"
#include "../../include/session.h"
#include "../../libAfterImage/afterimage.h"
#include "../../include/mystyle_property.h"

#include "dirtree.h"
#include "asinternals.h"

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

void          assign_string         (char *text, FILE * fd, char **arg, int *idx);
void          assign_path           (char *text, FILE * fd, char **arg, int *idx);
void          assign_themable_path  (char *text, FILE * fd, char **arg, int *idx);
void          assign_pixmap         (char *text, FILE * fd, char **arg, int *idx);
void          assign_geometry       (char *text, FILE * fd, char **geom, int *junk);
void          obsolete              (char *text, FILE * fd, char **arg, int *);

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
	/* base options */
    {"IconPath",   assign_path, &IconPath, (int *)0},
	{"ModulePath", assign_path, &ModulePath, (int *)0},
	{"PixmapPath", assign_themable_path, &PixmapPath, (int *)0},
	{"CursorPath", assign_path, &CursorPath, (int *)0},
    {"FontPath",   assign_path, &FontPath, (int *)0},

	/* database options */
	{"DeskTopScale", SetInts, (char **)&Scr.VScale, &dummy},
    {"DeskTopSize",  SetInts, (char **)&Scr.VxMax, &Scr.VyMax},

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
	{"MWMFunctionHints", SetFlag, (char **)MWMFunctionHints, NULL},
	{"MWMDecorHints", SetFlag, (char **)MWMDecorHints, NULL},
	{"MWMHintOverride", SetFlag, (char **)MWMHintOverride, NULL},
	{"FollowTitleChanges", SetFlag, (char **)FollowTitleChanges, (int *)0},
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
	{"MyStyle", mystyle_parse, &PixmapPath, NULL},

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

    {"MenuPinOn", assign_pixmap, (char **)&Scr.Look.MenuPinOn, (int *)0},    /* menu pin */
    {"MenuPinOff", assign_pixmap, (char **)&Scr.Look.MenuPinOff, (int *)0},
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
    {"TitleButtonStyle", SetInts, (char **)&Scr.Look.TitleButtonStyle, (int *)&dummy},
	{"TitleButton", SetTitleButton, (char **)1, (int *)0},
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
    {"ShadeAnimationSteps", SetInts, (char **)&Scr.Look.ShadeAnimationSteps, (int *)&dummy},
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

void
str_error (const char *err_format, const char *string)
{
	error_point ();
	fprintf (stderr, err_format, string);
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
Bool
GetIconFromFile (char *file, MyIcon * icon, int max_colors)
{
    if( Scr.image_manager == NULL )
	{
		char *ppath = PixmapPath ;
		if( ppath == NULL )
			ppath = getenv( "IMAGE_PATH" );
		if( ppath == NULL )
			ppath = getenv( "PATH" );
		Scr.image_manager = create_image_manager( NULL, 2.2, ppath, getenv( "IMAGE_PATH" ), getenv( "PATH" ), NULL );
	}

    memset( icon, 0x00, sizeof(icon_t));
    return load_icon(icon, file, Scr.image_manager );
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
                mystyle_merge_font (look->MSMenu[i], &StdFont);
    }
	if (Windowfont != NULL)
	{
        if (load_font (Windowfont, &WindowFont) == False)
            exit(1);
        for( i = 0 ; i < BACK_STYLES ; ++i )
            mystyle_merge_font (look->MSWindow[i], &WindowFont);
        mystyle_merge_font (look->MSMenu[MENU_BACK_TITLE], &WindowFont);
	}
	if (Iconfont != NULL)
	{
        if (load_font (Iconfont, &IconFont) == False)
            exit (1);
        for( i = 0 ; i < BACK_STYLES ; ++i )
            mystyle_merge_font (button_styles[i], &IconFont);
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
            MyStyle      *button_pixmap = mystyle_list_find (look->styles_list, "ButtonPixmap");

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
 * Initialize base.#bpp variables
 */
void
InitBase (Bool free_resources)
{
	if (free_resources)
	{
		if (IconPath != NULL)
			free (IconPath);
		if (ModulePath != NULL)
			free (ModulePath);
        if (CursorPath != NULL)
            free (CursorPath);
        if (PixmapPath != NULL)
			free (PixmapPath);
        if (FontPath != NULL)
            free (FontPath);
    }

	IconPath = NULL;
	ModulePath = NULL;
	PixmapPath = NULL;
    CursorPath = NULL;
    FontPath = NULL;

    Scr.VxMax = 1;
    Scr.VyMax = 1;
    Scr.VScale = 1;
}

void
CheckBaseSanity()
{
    if( Scr.VxMax <= 0 )
        Scr.VxMax = 0 ;
    else if( Scr.VxMax < 32000/Scr.MyDisplayWidth )
        Scr.VxMax = (Scr.VxMax * Scr.MyDisplayWidth) - Scr.MyDisplayWidth ;

    if( Scr.VyMax <= 0 )
        Scr.VyMax = 0 ;
    else if( Scr.VyMax < 32000/Scr.MyDisplayHeight )
        Scr.VyMax = (Scr.VyMax * Scr.MyDisplayHeight) - Scr.MyDisplayHeight ;
}


/*
 * Initialize feel variables
 */
void
InitFeel (ASFeel *feel, Bool free_resources)
{
    int i ;
    if (free_resources && feel)
	{
        while (feel->MouseButtonRoot != NULL)
		{
            MouseButton  *mb = feel->MouseButtonRoot;

            feel->MouseButtonRoot = mb->NextButton;
            if (mb->fdata)
            {
                free_func_data( mb->fdata);
                free (mb->fdata);
            }
			free (mb);
		}
        while (feel->FuncKeyRoot != NULL)
		{
            FuncKey      *fk = feel->FuncKeyRoot;

            feel->FuncKeyRoot = fk->next;
			if (fk->name != NULL)
				free (fk->name);
            if (fk->fdata != NULL)
            {
                free_func_data(fk->fdata);
                free (fk->fdata);
            }
			free (fk);
		}
        for( i = 0 ; i < MAX_CURSORS; ++i )
            if( feel->cursors[i] && feel->cursors[i] != Scr.standard_cursors[i] )
            {
                XFreeCursor( dpy, feel->cursors[i] );
                feel->cursors[i] = None ;
            }
        if( feel->Popups )
            destroy_ashash( &feel->Popups );
        if( feel->ComplexFunctions )
            destroy_ashash( &feel->ComplexFunctions );
	}

    feel->buttons2grab = 7;
    feel->AutoReverse = 0;
    feel->Xzap = 12;
    feel->Yzap = 12;
    feel->EdgeScrollX = feel->EdgeScrollY = -100000;
    feel->EdgeResistanceScroll = feel->EdgeResistanceMove = 0;
    feel->OpaqueMove = 5;
    feel->OpaqueResize = 0;
    feel->ClickTime = 150;
    feel->AutoRaiseDelay = 0;
    feel->RaiseButtons = 0;
    feel->flags = DoHandlePageing;
    feel->XorValue = (((unsigned long)1) << Scr.d_depth) - 1;

    feel->MouseButtonRoot = NULL;
    feel->FuncKeyRoot = NULL;
    feel->Popups = NULL;
    feel->ComplexFunctions = NULL;

    for( i = 0 ; i < MAX_CURSORS; ++i )
        if( feel->cursors[i] )
            feel->cursors[i] = Scr.standard_cursors[i] ;

}

void
CheckFeelSanity( ASFeel *feel )
{
    /* If no edge scroll line is provided in the setup file, use a default */
    if (feel->EdgeScrollX == -100000)
        feel->EdgeScrollX = 25;
    if (feel->EdgeScrollY == -100000)
        feel->EdgeScrollY = feel->EdgeScrollX;

    if (get_flags(feel->flags, ClickToRaise) && feel->AutoRaiseDelay == 0)
        feel->AutoRaiseDelay = -1;

    /* if edgescroll >1000 and < 100000m
        * wrap at edges of desktop (a "spherical" desktop) */
    if (feel->EdgeScrollX >= 1000)
    {
        feel->EdgeScrollX /= 1000;
        set_flags(feel->flags, EdgeWrapX);
    }
    if (feel->EdgeScrollY >= 1000)
    {
        feel->EdgeScrollY /= 1000;
        set_flags(feel->flags, EdgeWrapY);
    }
    feel->EdgeScrollX = feel->EdgeScrollX * Scr.MyDisplayWidth / 100;
    feel->EdgeScrollY = feel->EdgeScrollY * Scr.MyDisplayHeight / 100;

    if (Scr.VxMax == 0)
        clear_flags(feel->flags, EdgeWrapX);
    if (Scr.VyMax == 0)
        clear_flags(feel->flags, EdgeWrapY);
}

/*
 * Initialize look variables
 */
void
InitLook (MyLook *look, Bool free_resources)
{
	int           i;

    balloon_init (free_resources);
    if (free_resources)
	{
		/* styles/textures */
        mystyle_list_destroy_all(&(look->styles_list));

        if( look->DefaultFrame )
            destroy_myframe( &(look->DefaultFrame) );
        if( look->FramesList )
            destroy_ashash( &(look->FramesList));

#ifndef NO_TEXTURE
		/* icons */
        if (look->MenuArrow != NULL)
            destroy_icon( &(look->MenuArrow) );
        if (look->MenuPinOn != NULL)
            destroy_icon( &(look->MenuPinOn) );
        if (look->MenuPinOff != NULL)
            destroy_icon( &(look->MenuPinOff) );

#endif /* !NO_TEXTURE */
		/* titlebar buttons */
		for (i = 0; i < 10; i++)
		{
            free_icon_resources( look->buttons[i].unpressed );
            free_icon_resources( look->buttons[i].pressed );
		}
        if( look->configured_icon_areas )
            free( look->configured_icon_areas );

        if( Scr.default_icon_box )
            destroy_asiconbox( &(Scr.default_icon_box));
        if( Scr.icon_boxes )
            destroy_ashash( &(Scr.icon_boxes));

        /* temporary old-style fonts : */
        unload_font (&StdFont);
        unload_font (&WindowFont);
        unload_font (&IconFont);

        destroy_hints_list(&(look->supported_hints));
    }
    /* styles/textures */
    look->styles_list = NULL;
    for( i = 0 ; i < BACK_STYLES ; ++i )
        look->MSWindow[i] = NULL;
    for( i = 0 ; i < MENU_BACK_STYLES ; ++i )
        look->MSMenu[i] = NULL;

    look->DefaultFrame = create_default_myframe();

    look->TitleTextAlign = 0;

	/* titlebar buttons */
    look->TitleButtonSpacing = 2;
    look->TitleButtonStyle = 0;
	for (i = 0; i < 10; i++)
        memset(&(look->buttons[i]), 0x00, sizeof(MyButton));

    look->ButtonWidth = 0;
    look->ButtonHeight = 0;

	/* resize/move window geometry */
    memset( &(look->resize_move_geometry), 0x00, sizeof(ASGeometry));

    /* miscellaneous stuff */
    look->RubberBand = 0;
    look->flags = TxtrMenuItmInd|SeparateButtonTitle ;
    look->DrawMenuBorders = 1;
    look->configured_icon_areas_num = 0;
    look->configured_icon_areas = NULL ;

    Scr.default_icon_box = NULL ;
    Scr.icon_boxes = NULL ;

    /* initialize some lists */
    look->DefaultIcon = NULL;
    /* free pixmaps that are no longer in use */
	pixmap_ref_purge ();

    look->StartMenuSortMode = DEFAULTSTARTMENUSORT;
    look->supported_hints = NULL ;

    /* temporary old-style fonts : */
    memset(&StdFont, 0x00, sizeof(MyFont));
    memset(&WindowFont, 0x00, sizeof(MyFont));
    memset(&IconFont, 0x00, sizeof(MyFont));
}

void
make_styles (MyLook *look)
{
/* make sure the globals are defined */
    char *style_names[BACK_STYLES] = { "default", "FWindow", "UWindow", "SWindow" };
    char *menu_style_names[MENU_BACK_STYLES] = { "MenuTitle", "MenuItem", "MenuHilite", "MenuStipple" };
    int i ;

    if (look->MSWindow[BACK_DEFAULT] == NULL)
        look->MSWindow[BACK_DEFAULT] = mystyle_list_find_or_default (look->styles_list, "default");
    for( i = 0 ; i < BACK_STYLES ; ++i )
        if (look->MSWindow[i] == NULL)
            look->MSWindow[i] = mystyle_list_new (look->styles_list, style_names[i]);

    for( i = 0 ; i < MENU_BACK_STYLES ; ++i )
        if (look->MSMenu[i] == NULL)
            look->MSMenu[i] = mystyle_list_new (look->styles_list, menu_style_names[i]);

    if (mystyle_list_find (look->styles_list, "ButtonPixmap") == NULL)
        mystyle_list_new (look->styles_list, "ButtonPixmap");
    if (mystyle_list_find (look->styles_list, "ButtonTitleFocus") == NULL)
        mystyle_list_new (look->styles_list, "ButtonTitleFocus");
    if (mystyle_list_find (look->styles_list, "ButtonTitleSticky") == NULL)
        mystyle_list_new (look->styles_list, "ButtonTitleSticky");
    if (mystyle_list_find (look->styles_list, "ButtonTitleUnfocus") == NULL)
        mystyle_list_new (look->styles_list, "ButtonTitleUnfocus");
}

void
FixLook( MyLook *look )
{
    /* make sure all needed styles are created */
    make_styles (look);
    /* merge pre-1.5 compatibility keywords */
    merge_old_look_variables (look);

    /* fill in remaining members with the default style */
    mystyle_list_fix_styles (look->styles_list);

    mystyle_list_set_property (Scr.wmprops, look->styles_list);

#ifndef NO_TEXTURE
    /* update frame geometries */
    if (get_flags( look->flags, DecorateFrames))
    {
        if( look->DefaultFrame )
            myframe_load ( look->DefaultFrame, Scr.image_manager );
        else
            look->DefaultFrame = create_default_myframe();
        /* TODO: need to load the list as well (if we have any )*/
    }
#endif /* ! NO_TEXTURE */

    /* updating balloons look */
    balloon_setup (dpy);
    balloon_set_style (dpy, mystyle_find_or_default ("TitleButtonBalloon"));

    /* checking sanity of the move-resize window geometry :*/
    if( !get_flags(look->resize_move_geometry.flags, WidthValue ) )
        look->resize_move_geometry.width = XTextWidth (look->MSWindow[BACK_FOCUSED]->font.font, " +88888 x +88888 ", 17);

    if( !get_flags(look->resize_move_geometry.flags, HeightValue ) )
        look->resize_move_geometry.height = look->MSWindow[BACK_FOCUSED]->font.height + SIZE_VINDENT * 2;

    set_flags( look->resize_move_geometry.flags, HeightValue|WidthValue );

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
            print_asdb( NULL, NULL, Database );
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
		if (*ptr != '\0' && *ptr != '#' && *ptr != '*')
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
LoadASConfig (int thisdesktop, Bool parse_menu, Bool parse_look, Bool parse_feel)
{
    int           parse_base = True, parse_database = True;
	char         *tline = NULL;

	ASImageManager *old_image_manager = Scr.image_manager ;

#ifndef DIFFERENTLOOKNFEELFOREACHDESKTOP
	/* only one look & feel should be used */
	thisdesktop = 0;
#endif /* !DIFFERENTLOOKNFEELFOREACHDESKTOP */

	/* kludge: make sure functions get updated */
    parse_feel = parse_feel||parse_menu;

    show_progress("Detected colordepth : %d. Loading configuration ...", Scr.d_depth);
    if (Session->overriding_file == NULL )
	{
        char *configfile;
        const char *const_configfile;
        if (parse_base)
		{
            if( (configfile = make_session_file(Session, BASE_FILE ".%dbpp", True )) != NULL )
            {
                char * old_pixmap_path = PixmapPath ;

                InitBase (True);
                ParseConfigFile (configfile, &tline);
                /* Save base filename to pass to modules */
                if( (old_pixmap_path == NULL && PixmapPath != NULL )||
                    (old_pixmap_path != NULL && PixmapPath == NULL )||
                    (old_pixmap_path != NULL && PixmapPath != NULL && strcmp(old_pixmap_path, PixmapPath) == 0 ) )
                {
                    Scr.image_manager = create_image_manager( NULL, 2.2, PixmapPath, getenv( "IMAGE_PATH" ), getenv( "PATH" ), NULL );
                    InitLook (&Scr.Look, True);
                    parse_look = True ;
                }
                free( old_pixmap_path );
                show_progress("BASE configuration loaded from \"%s\" ...", configfile);
                free( configfile );
            }else
            {
                show_warning("BASE configuration file cannot be found");
                parse_base = False ;
            }
        }
        if (parse_look)
		{
            if( (const_configfile = get_session_file (Session, thisdesktop, F_CHANGE_LOOK) ) != NULL )
            {
                InitLook (&Scr.Look, True);
                ParseConfigFile (const_configfile, &tline);
                show_progress("LOOK configuration loaded from \"%s\" ...", const_configfile);
            }else
            {
                show_warning("LOOK configuration file cannot be found");
                parse_look = False ;
            }
        }
        if (parse_menu)
		{
			if (tline == NULL)
				tline = safemalloc (MAXLINELENGTH + 1);
			MeltStartMenu (tline);
        }
        if (parse_feel)
		{
            if( (const_configfile = get_session_file (Session, thisdesktop, F_CHANGE_FEEL) ) != NULL )
            {
                InitFeel (&Scr.Feel, True);
                ParseConfigFile (const_configfile, &tline);
                show_progress("FEEL configuration loaded from \"%s\" ...", const_configfile);
            }else
            {
                show_warning("FEEL configuration file cannot be found");
                parse_feel = False ;
            }
        }
		if (parse_feel || parse_look)
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
        if( (configfile = make_session_file(Session, AUTOEXEC_FILE, False )) != NULL )
        {
            ParseConfigFile (configfile, &tline);
            show_progress("AUTOEXEC configuration loaded from \"%s\" ...", configfile);
            free( configfile );
        }else
            show_warning("AUTOEXEC configuration file cannot be found");

        if( (configfile = make_session_file(Session, DATABASE_FILE, False )) != NULL )
        {
            InitDatabase (True);
            ParseDatabase (configfile);
            show_progress("DATABASE configuration loaded from \"%s\" ...", configfile);
            free( configfile );
        }else
        {
            show_warning("DATABASE configuration file cannot be found");
            parse_database = False ;
        }

	} else
	{
		Scr.image_manager = NULL ;
		/* Yes, override config file */
        InitBase (True);
        InitLook (&Scr.Look, True);
        InitFeel (&Scr.Feel, True);
        InitDatabase (True);
        ParseConfigFile (Session->overriding_file, &tline);
        show_progress("AfterStep configuration loaded from \"%s\" ...", Session->overriding_file);
        parse_base = parse_feel = parse_look = parse_database = True ;
    }

	/* let's free the memory used for parsing */
	if (tline)
		free (tline);
    show_progress("Done loading configuration.");


    CheckBaseSanity();
    CheckFeelSanity( &Scr.Feel );

    if (parse_look)
        FixLook( &Scr.Look );

    /* TODO: update the menus */
    if (parse_look || parse_feel|| parse_base || parse_menu)
	{
    }

    /* force update of window frames */
    if (parse_look || parse_feel || parse_base || parse_database)
        iterate_asbidirlist( Scr.Windows->clients, redecorate_aswindow_iter_func, NULL, NULL, False );

    if( old_image_manager && old_image_manager != Scr.image_manager )
		destroy_image_manager( old_image_manager, False );
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

/*****************************************************************************
 *
 * Copies a PATH string from the config file to a specified location
 *
 ****************************************************************************/

void
assign_themable_path (char *text, FILE * fd, char **arg, int *junk)
{
    char         *as_theme_data = make_session_dir(Session, THEME_DATA_DIR, False);
	char         *tmp = stripcpy (text);
	int           tmp_len;

	replaceEnvVar (&tmp);
	tmp_len = strlen (tmp);
	*arg = safemalloc (tmp_len + 1 + strlen (as_theme_data) + 1);
	strcpy (*arg, tmp);
	(*arg)[tmp_len] = ':';
	strcpy ((*arg) + tmp_len + 1, as_theme_data);
	free (tmp);
	free (as_theme_data);
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
        GetIconFromFile (fname, (MyIcon *) arg, -1);
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
	char          file[256], file2[256];
	int           fnamelen = 0, offset = 0, linelen;
	int           n;

	if (balloon_parse (tline, fd))
		return;

	linelen = strlen (tline);
	if ((n = sscanf (tline, "%d", &num)) <= 0)
	{
		fprintf (stderr, "wrong number of parameters given with TitleButton\n");
		return;
	}
	if (num < 0 || num > 9)
	{
		fprintf (stderr, "invalid Titlebar button number: %d\n", num);
		return;
	}

	num = translate_title_button(num);

	/* going the hard way to prevent buffer overruns */
	while (isspace (*(tline + offset)) && offset < linelen)
		offset++;
	while (isdigit (*(tline + offset)) && offset < linelen)
		offset++;
	while (isspace (*(tline + offset)) && offset < linelen)
		offset++;
	for (; !isspace (*(tline + offset)) && offset < linelen; offset++)
		if (fnamelen < 254)
			file[fnamelen++] = *(tline + offset);

	file[fnamelen] = '\0';
	if (fnamelen)
	{
		while (isspace (*(tline + offset)) && offset < linelen)
			offset++;
		for (fnamelen = 0; !isspace (*(tline + offset)) && offset < linelen; offset++)
			if (fnamelen < 254)
				file2[fnamelen++] = *(tline + offset);
		file2[fnamelen] = '\0';
	}
	if (fnamelen == 0)
	{
		fprintf (stderr, "wrong number of parameters given with TitleButton\n");
		return;
	}

    GetIconFromFile (file, &(Scr.Look.buttons[num].unpressed), 0);
    GetIconFromFile (file2, &(Scr.Look.buttons[num].pressed), 0);

    Scr.Look.buttons[num].width = 0 ;
    Scr.Look.buttons[num].height = 0 ;

    if( Scr.Look.buttons[num].unpressed.image )
	{
        Scr.Look.buttons[num].width = Scr.Look.buttons[num].unpressed.image->width ;
        Scr.Look.buttons[num].height = Scr.Look.buttons[num].unpressed.image->height ;
	}
    if( Scr.Look.buttons[num].pressed.image )
	{
        if( Scr.Look.buttons[num].pressed.image->width > Scr.Look.buttons[num].width )
            Scr.Look.buttons[num].width = Scr.Look.buttons[num].pressed.image->width ;
        if( Scr.Look.buttons[num].pressed.image->height > Scr.Look.buttons[num].height )
            Scr.Look.buttons[num].height = Scr.Look.buttons[num].pressed.image->height ;
	}
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

	path = findIconFile (f_mask, CursorPath, R_OK);
	if (path)
	{
		XReadBitmapFile (dpy, Scr.Root, path, &width, &height, &mask, &x, &y);
		free (path);
	} else
	{
		tline_error ("Cursor mask not found");
		return;
	}

	path = findIconFile (f_cursor, CursorPath, R_OK);
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

	dirtree_make_menu2 (tree, buf);
	/* to keep backward compatibility, make a copy of the root menu with
	 * the name "start" */
	{
		if ((*tree).name != NULL)
			free ((*tree).name);
		(*tree).name = mystrdup ("start");
		(*tree).flags |= DIRTREE_KEEPNAME;
		dirtree_make_menu2 (tree, buf);
	}
	/* cleaning up cache of the searcher */
	is_executable_in_path (NULL);

	dirtree_delete (tree);
	return 0;
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


