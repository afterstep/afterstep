/*
 * Copyright (c) 1998 Rafal Wierzbicki <rafal@mcss.mcmaster.ca>
 * Copyright (c) 1998 Sasha Vasko <sasha at aftercode.net>
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

#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>						   /* for Bool, so Xutil.h will be happy */
#include <X11/Xutil.h>						   /* for XSizeHints, so afterstep.h will be happy */

#ifdef I18N
#include <X11/Xlocale.h>
#endif

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/loadimg.h"
#include "../../libAfterImage/afterimage.h"
#include "../../include/mystyle_property.h"

#include "dirtree.h"
#include "menus.h"
#include "globals.h"

char         *PixmapPath;
char         *CursorPath;

char         *global_base_file = NULL;
char         *IconPath;
char         *ModulePath = AFTER_BIN_DIR;

Bool          shall_override_config_file;
char         *config_file_to_override;
char         *white = "white";
char         *black = "black";
char         *grey = "SlateGrey";

/*
 * the old-style look variables
 */
char         *Stdfont;
char         *Windowfont;
char         *Iconfont;
char         *Menustipple;
char         *Stdback;
char         *Stdfore;
char         *Stickyback;
char         *Stickyfore;
char         *Hiback;
char         *Hifore;
char         *Mtitleback;
char         *Mtitlefore;
char         *Menuback;
char         *Menufore;
char         *Menuhiback;
char         *Menuhifore;

/*#ifndef NO_TEXTURE */
char         *TexTypes = NULL;
char         *TexMaxcols = NULL;
char         *TColor = NULL, *UColor = NULL, *SColor = NULL;
char         *IColor = NULL, *MHColor = NULL, *MColor = NULL;
char         *TPixmap = NULL, *UPixmap = NULL, *SPixmap = NULL;
char         *MTPixmap = NULL, *MPixmap = NULL, *MHPixmap = NULL;

/*#endif */
/*
 * endif
 * end of old-style look variables
 */

int           MeltStartMenu (char *buf);
void          GetColors (void);
MenuRoot     *NewMenuRoot (char *name);

#ifndef NO_TEXTURE
char         *TitleStyle = NULL;
char         *TextTextureStyle = NULL;
char         *MArrowPixmap = NULL;
char          hincolor[15];
char          hircolor[15];
char          hiscolor[15];
char          loncolor[15];
char          lorcolor[15];
char          loscolor[15];

extern void   InitTextureData (TextureInfo * info, char *title, char *utitle,
							   char *mtitle, char *item, char *mhilite, char *sticky, char *text);
int           IconTexType = TEXTURE_BUILTIN;
char         *IconBgColor;
char         *IconTexColor;
int           IconMaxColors = 16;
char         *IconPixmapFile;
int           IconTexFlags = 0;
#endif

int           dummy;

extern Bool   DoHandlePageing;

/* value for the rubberband XORing */
unsigned long XORvalue;
int           RubberBand = 0;
int           have_the_colors = 0;
char         *RMGeom = NULL;
int           Xzap = 12, Yzap = 12;
int           DrawMenuBorders = 1;
int           TextureMenuItemsIndividually = 1;
int           AutoReverse = 0;
int           AutoTabThroughDesks = 0;
int           MenuMiniPixmaps = 0;
int           StartMenuSortMode = DEFAULTSTARTMENUSORT;
int           ShadeAnimationSteps = 12;
void          SetCustomCursor (char *text, FILE * fd, char **arg, int *junk);

void          assign_string (char *text, FILE * fd, char **arg, int *);
void          assign_path (char *text, FILE * fd, char **arg, int *);
void          assign_themable_path (char *text, FILE * fd, char **arg, int *);
void          assign_pixmap (char *text, FILE * fd, char **arg, int *);
void          text_gradient_obsolete (char *text, FILE * fd, char **arg, int *);

/*
 * Order is important here! if one keyword is the same as the first part of
 * another keyword, the shorter one must come first!
 */
struct config main_config[] = {
	/* base options */
	{"IconPath", assign_path, &IconPath, (int *)0},
	{"ModulePath", assign_path, &ModulePath, (int *)0},
	{"PixmapPath", assign_themable_path, &PixmapPath, (int *)0},
	{"CursorPath", assign_path, &CursorPath, (int *)0},

	/* database options */
	{"DeskTopScale", SetInts, (char **)&Scr.VScale, &dummy},
	{"DeskTopSize", SetInts, (char **)&Scr.VxMax, &Scr.VyMax},
	{"Style", style_parse, (char **)0, (int *)0},

	/* feel options */
	{"StubbornIcons", SetFlag, (char **)StubbornIcons, (int *)0},
	{"StubbornPlacement", SetFlag, (char **)StubbornPlacement, (int *)0},
	{"StubbornIconPlacement", SetFlag, (char **)StubbornIconPlacement, (int *)0},
	{"StickyIcons", SetFlag, (char **)StickyIcons, (int *)0},
	{"IconTitle", SetFlag, (char **)IconTitle, (int *)0},
	{"KeepIconWindows", SetFlag, (char **)KeepIconWindows, (int *)0},
	{"NoPPosition", SetFlag, (char **)NoPPosition, (int *)0},
	{"CirculateSkipIcons", SetFlag, (char **)CirculateSkipIcons, (int *)0},
	{"EdgeScroll", SetInts, (char **)&Scr.EdgeScrollX, &Scr.EdgeScrollY},
	{"RandomPlacement", SetFlag, (char **)RandomPlacement, (int *)0},
	{"SmartPlacement", SetFlag, (char **)SMART_PLACEMENT, (int *)0},
	{"DontMoveOff", SetFlag, (char **)DontMoveOff, (int *)0},
	{"DecorateTransients", SetFlag, (char **)DecorateTransients, (int *)0},
	{"CenterOnCirculate", SetFlag, (char **)CenterOnCirculate, (int *)0},
	{"AutoRaise", SetInts, (char **)&Scr.AutoRaiseDelay, &dummy},
	{"ClickTime", SetInts, (char **)&Scr.ClickTime, &dummy},
	{"OpaqueMove", SetInts, (char **)&Scr.OpaqueSize, &dummy},
	{"OpaqueResize", SetInts, (char **)&Scr.OpaqueResize, &dummy},
	{"XorValue", SetInts, (char **)&XORvalue, &dummy},
	{"Mouse", ParseMouseEntry, (char **)1, (int *)0},
	{"Popup", ParsePopupEntry, (char **)1, (int *)0},
	{"Function", ParsePopupEntry, (char **)1, (int *)0},
	{"Key", ParseKeyEntry, (char **)1, (int *)0},
	{"ClickToFocus", SetFlag, (char **)ClickToFocus, (int *)EatFocusClick},
	{"ClickToRaise", SetButtonList, (char **)&Scr.RaiseButtons, (int *)0},
	{"MenusHigh", SetFlag, (char **)MenusHigh, (int *)0},
	{"SloppyFocus", SetFlag, (char **)SloppyFocus, (int *)0},
	{"Cursor", SetCursor, (char **)0, (int *)0},
	{"CustomCursor", SetCustomCursor, (char **)0, (int *)0},
	{"PagingDefault", SetInts, (char **)&DoHandlePageing, &dummy},
	{"EdgeResistance", SetInts, (char **)&Scr.ScrollResistance, &Scr.MoveResistance},
	{"BackingStore", SetFlag, (char **)BackingStore, (int *)0},
	{"AppsBackingStore", SetFlag, (char **)AppsBackingStore, (int *)0},
	{"SaveUnders", SetFlag, (char **)SaveUnders, (int *)0},
	{"Xzap", SetInts, (char **)&Xzap, (int *)&dummy},
	{"Yzap", SetInts, (char **)&Yzap, (int *)&dummy},
	{"AutoReverse", SetInts, (char **)&AutoReverse, (int *)&dummy},
	{"AutoTabThroughDesks", SetInts, (char **)&AutoTabThroughDesks, (int *)&dummy},
	{"MWMFunctionHints", SetFlag, (char **)MWMFunctionHints, NULL},
	{"MWMDecorHints", SetFlag, (char **)MWMDecorHints, NULL},
	{"MWMHintOverride", SetFlag, (char **)MWMHintOverride, NULL},
	{"FollowTitleChanges", SetFlag, (char **)FollowTitleChanges, (int *)0},
	/* look options */
	{"Font", assign_string, &Stdfont, (int *)0},
	{"WindowFont", assign_string, &Windowfont, (int *)0},
	{"MTitleForeColor", assign_string, &Mtitlefore, (int *)0},
	{"MTitleBackColor", assign_string, &Mtitleback, (int *)0},
	{"MenuForeColor", assign_string, &Menufore, (int *)0},
	{"MenuBackColor", assign_string, &Menuback, (int *)0},
	{"MenuHiForeColor", assign_string, &Menuhifore, (int *)0},
	{"MenuHiBackColor", assign_string, &Menuhiback, (int *)0},
	{"MenuStippleColor", assign_string, &Menustipple, (int *)0},
	{"StdForeColor", assign_string, &Stdfore, (int *)0},
	{"StdBackColor", assign_string, &Stdback, (int *)0},
	{"StickyForeColor", assign_string, &Stickyfore, (int *)0},
	{"StickyBackColor", assign_string, &Stickyback, (int *)0},
	{"HiForeColor", assign_string, &Hifore, (int *)0},
	{"HiBackColor", assign_string, &Hiback, (int *)0},
	{"IconBox", SetBox, (char **)0, (int *)0},
	{"IconFont", assign_string, &Iconfont, (int *)0},
	{"MyStyle", mystyle_parse, &PixmapPath, NULL},

#ifndef NO_TEXTURE
	{"TitleBarStyle", assign_string, &TitleStyle, (int *)0},
	{"TextureTypes", assign_string, &TexTypes, (int *)0},
	{"TextureMaxColors", assign_string, &TexMaxcols, (int *)0},
	{"TitleTextureColor", assign_string, &TColor, (int *)0},	/* title */
	{"UTitleTextureColor", assign_string, &UColor, (int *)0},	/* unfoc tit */
	{"STitleTextureColor", assign_string, &SColor, (int *)0},	/* stic tit */
	{"MTitleTextureColor", assign_string, &MColor, (int *)0},	/* menu title */
	{"MenuTextureColor", assign_string, &IColor, (int *)0},	/* menu items */
	{"MenuHiTextureColor", assign_string, &MHColor, (int *)0},	/* sel items */
	{"MenuPixmap", assign_string, &MPixmap, (int *)0},	/* menu entry */
	{"MenuHiPixmap", assign_string, &MHPixmap, (int *)0},	/* hil m entr */
	{"MTitlePixmap", assign_string, &MTPixmap, (int *)0},	/* menu title */
	{"MArrowPixmap", assign_string, &MArrowPixmap, (int *)0},	/* menu arrow */
	{"MenuPinOn", assign_pixmap, (char **)&Scr.MenuPinOn, NULL},	/* menu pin */
	{"MenuPinOff", assign_pixmap, (char **)&Scr.MenuPinOff, NULL},
	{"TitlePixmap", assign_string, &TPixmap, (int *)0},	/* foc tit */
	{"UTitlePixmap", assign_string, &UPixmap, (int *)0},	/* unfoc tit */
	{"STitlePixmap", assign_string, &SPixmap, (int *)0},	/* stick tit */
	{"TexturedHandle", SetFlag2, (char **)TexturedHandle, (int *)&Textures.flags},
	{"TitlebarNoPush", SetFlag2, (char **)TitlebarNoPush, (int *)&Textures.flags},

	/* these are obsolete : */
	{"TextGradientColor", text_gradient_obsolete, (char **)NULL, (int *)0},	/* title text */
	{"GradientText", text_gradient_obsolete, (char **)NULL, (int *)0},
	/* use these instead : */
	{"TextTextureStyle", assign_string, &TextTextureStyle, (int *)0},

	{"ButtonTextureType", SetInts, (char **)&IconTexType, &dummy},
	{"ButtonBgColor", assign_string, &IconBgColor, (int *)0},
	{"ButtonTextureColor", assign_string, &IconTexColor, (int *)0},
	{"ButtonMaxColors", SetInts, (char **)&IconMaxColors, &dummy},
	{"ButtonPixmap", assign_string, &IconPixmapFile, (int *)0},
	{"ButtonNoBorder", SetFlag2, (char **)IconNoBorder, (int *)&Textures.flags},
	{"TextureMenuItemsIndividually", SetInts, (char **)&TextureMenuItemsIndividually,
	 (int *)&dummy},
	{"MenuMiniPixmaps", SetInts, (char **)&MenuMiniPixmaps, &dummy},
	{"FrameNorth", assign_string, &FrameN, (int *)0},
	{"FrameSouth", assign_string, &FrameS, (int *)0},
	{"FrameEast", assign_string, &FrameE, (int *)0},
	{"FrameWest", assign_string, &FrameW, (int *)0},
	{"FrameNW", assign_string, &FrameNW, (int *)0},
	{"FrameNE", assign_string, &FrameNE, (int *)0},
	{"FrameSW", assign_string, &FrameSW, (int *)0},
	{"FrameSE", assign_string, &FrameSE, (int *)0},
	{"DecorateFrames", SetInts, (char **)&DecorateFrames, (int *)&dummy},
#endif /* NO_TEXTURE */
	{"TitleTextAlign", SetInts, (char **)&Scr.TitleTextAlign, &dummy},
	{"TitleButtonSpacing", SetInts, (char **)&Scr.TitleButtonSpacing, (int *)&dummy},
	{"TitleButtonStyle", SetInts, (char **)&Scr.TitleButtonStyle, (int *)&dummy},
	{"TitleButton", SetTitleButton, (char **)1, (int *)0},
	{"TitleTextMode", SetTitleText, (char **)1, (int *)0},
	{"ResizeMoveGeometry", assign_string, &RMGeom, (int *)0},
	{"StartMenuSortMode", SetInts, (char **)&StartMenuSortMode, (int *)&dummy},
	{"DrawMenuBorders", SetInts, (char **)&DrawMenuBorders, (int *)&dummy},
	{"ButtonSize", SetInts, (char **)&Scr.ButtonWidth, (int *)&Scr.ButtonHeight},
	{"SeparateButtonTitle", SetFlag2, (char **)SeparateButtonTitle, (int *)&Textures.flags},
	{"RubberBand", SetInts, (char **)&RubberBand, &dummy},
	{"DefaultStyle", mystyle_parse_set_style, (char **)&Scr.MSDefault, NULL},
	{"FWindowStyle", mystyle_parse_set_style, (char **)&Scr.MSFWindow, NULL},
	{"UWindowStyle", mystyle_parse_set_style, (char **)&Scr.MSUWindow, NULL},
	{"SWindowStyle", mystyle_parse_set_style, (char **)&Scr.MSSWindow, NULL},
	{"MenuItemStyle", mystyle_parse_set_style, (char **)&Scr.MSMenuItem, NULL},
	{"MenuTitleStyle", mystyle_parse_set_style, (char **)&Scr.MSMenuTitle, NULL},
	{"MenuHiliteStyle", mystyle_parse_set_style, (char **)&Scr.MSMenuHilite, NULL},
	{"MenuStippleStyle", mystyle_parse_set_style, (char **)&Scr.MSMenuStipple, NULL},
	{"ShadeAnimationSteps", SetInts, (char **)&ShadeAnimationSteps, (int *)&dummy},
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

extern ASDirs as_dirs;

int
MakeASDir (const char *name, mode_t perms)
{
	fprintf (stderr, "Creating %s ... ", name);
	if (mkdir (name, perms))
	{
		fprintf (stderr,
				 "ERROR !\n AfterStep depends on %s directory !\nPlease check permissions or contact your sysadmin !\n",
				 name);
		return (-1);
	}
	fprintf (stderr, "done\n");
	return 0;
}

int
MakeASFile (const char *name)
{
	FILE         *touch;

	fprintf (stderr, "Creating %s ... ", name);
	if ((touch = fopen (name, "w")) == NULL)
	{
		fprintf (stderr, "ERROR !\n Cannot open file %s for writing!\n"
				 " Please check permissions or contact your sysadmin !\n", name);
		return (-1);
	}
	fclose (touch);
	fprintf (stderr, "done\n");
	return 0;
}

int
CheckOrCreate (const char *what)
{
	mode_t        perms = 0755;
	int           res = 0;

	if (*what == '~' && *(what + 1) == '/')
	{
		char         *checkdir = PutHome (what);

		if (CheckDir (checkdir) != 0)
			res = MakeASDir (checkdir, perms);
		free (checkdir);
	} else if (CheckDir (what) != 0)
		res = MakeASDir (what, perms);

	return res;
}

int
CheckOrCreateFile (const char *what)
{
	int           res = 0;

	if (*what == '~' && *(what + 1) == '/')
	{
		char         *checkfile = PutHome (what);

		if (CheckFile (checkfile) != 0)
			res = MakeASFile (checkfile);
		free (checkfile);
	} else if (CheckFile (what) != 0)
		res = MakeASFile (what);

	return res;
}

/* copying file from shared dir to home dir */
int
HomeCreateIfNeeded (const char *filename)
{
	char         *from, *to;
	int           res = 0;

	to = (char *)make_file_name (as_dirs.after_dir, filename);
	if (CheckFile (to) != 0)
	{
		from = (char *)make_file_name (as_dirs.after_sharedir, filename);
		res = CopyFile (from, to);
		free (from);
	}
	free (to);
	return res;
}

/***************************************************************
 **************************************************************/
void
text_gradient_obsolete (char *text, FILE * fd, char **arg, int *i)
{
	tline_error ("This option is opbsolete, please use TextTextureStyle instead ");
}

/***************************************************************
 *
 * get an icon
 *
 **************************************************************/
Bool
GetIconFromFile (char *file, MyIcon * icon, int max_colors)
{
	char         *path = NULL;
	Bool          success = False;
	unsigned int  dum;
	int           dummy;
	Window        root;

	(*icon).pix = None;
	(*icon).mask = None;
	(*icon).width = 0;
	(*icon).height = 0;

	if ((path = findIconFile (file, PixmapPath, R_OK)) == NULL)
		return False;

	/* -1 is default for screen depth */
	if (max_colors == -1 && Scr.d_depth <= 8)
		max_colors = 10;

	if ((icon->pix = LoadImageWithMask (dpy, Scr.Root, max_colors, path, &(icon->mask))) != None)
	{
		success = True;
		XGetGeometry (dpy, icon->pix, &root, &dummy, &dummy,
					  &(icon->width), &(icon->height), &dum, &dum);
	}

	free (path);
	return success;
}

/***************************************************************
 *
 * Read a XPM file
 *
 **************************************************************/
Pixmap
GetXPMTile (char *file, int max_colors)
{
	MyIcon        icon;

	GetIconFromFile (file, &icon, max_colors);
	if (icon.mask != None)
		UnloadMask (icon.mask);
	return icon.pix;
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
	if (free_resources)
	{
		/* the fonts */
		if (Stdfont != NULL)
			free (Stdfont);
		if (Windowfont != NULL)
			free (Windowfont);
		if (Iconfont != NULL)
			free (Iconfont);

		/* the colors */
		if (Stdback != NULL)
			free (Stdback);
		if (Stdfore != NULL)
			free (Stdfore);
		if (Hiback != NULL)
			free (Hiback);
		if (Hifore != NULL)
			free (Hifore);
		if (Stickyback != NULL)
			free (Stickyback);
		if (Stickyfore != NULL)
			free (Stickyfore);
		if (Mtitlefore != NULL)
			free (Mtitlefore);
		if (Mtitleback != NULL)
			free (Mtitleback);
		if (Menuback != NULL)
			free (Menuback);
		if (Menufore != NULL)
			free (Menufore);
		if (Menuhifore != NULL)
			free (Menuhifore);
		if (Menuhiback != NULL)
			free (Menuhiback);
		if (Menustipple != NULL)
			free (Menustipple);

#ifndef NO_TEXTURE
		/* the gradients */
		if (TextTextureStyle != NULL)
			free (TextTextureStyle);
		if (UColor != NULL)
			free (UColor);
		if (TColor != NULL)
			free (TColor);
		if (SColor != NULL)
			free (SColor);
		if (MColor != NULL)
			free (MColor);
		if (IColor != NULL)
			free (IColor);
		if (MHColor != NULL)
			free (MHColor);

		/* the pixmaps */
		if (UPixmap != NULL)
			free (UPixmap);
		if (TPixmap != NULL)
			free (TPixmap);
		if (SPixmap != NULL)
			free (SPixmap);
		if (MTPixmap != NULL)
			free (MTPixmap);
		if (MPixmap != NULL)
			free (MPixmap);
		if (MHPixmap != NULL)
			free (MHPixmap);

		/* miscellaneous stuff */
		if (TexTypes != NULL)
			free (TexTypes);
		if (TexMaxcols != NULL)
			free (TexMaxcols);

		/* icons */
		if (MArrowPixmap != NULL)
			free (MArrowPixmap);
#endif
	}

	/* the fonts */
	Stdfont = NULL;
	Windowfont = NULL;
	Iconfont = NULL;

	/* the text type */
	Scr.TitleTextType = 0;

	/* the colors */
	Stdback = NULL;
	Stdfore = NULL;
	Hiback = NULL;
	Hifore = NULL;
	Stickyback = NULL;
	Stickyfore = NULL;
	Mtitlefore = NULL;
	Mtitleback = NULL;
	Menuback = NULL;
	Menufore = NULL;
	Menuhifore = NULL;
	Menuhiback = NULL;
	Menustipple = NULL;

#ifndef NO_TEXTURE
	/* the gradients */
	TextTextureStyle = NULL;
	UColor = NULL;
	TColor = NULL;
	SColor = NULL;
	MColor = NULL;
	IColor = NULL;
	MHColor = NULL;

	/* the pixmaps */
	UPixmap = NULL;
	TPixmap = NULL;
	SPixmap = NULL;
	MTPixmap = NULL;
	MPixmap = NULL;
	MHPixmap = NULL;

	/* miscellaneous stuff */
	TexTypes = NULL;
	TexMaxcols = NULL;

	/* icons */
	MArrowPixmap = NULL;
#endif
}


void
merge_old_look_colors (MyStyle * style, int type, int maxcols, char *fore, char *back,
					   char *gradient, char *pixmap)
{
	if ((fore != NULL) && !((*style).user_flags & F_FORECOLOR))
	{
		if (parse_argb_color (fore, &((*style).colors.fore)) != fore)
			(*style).user_flags |= F_FORECOLOR;
	}
	if ((back != NULL) && !((*style).user_flags & F_BACKCOLOR))
	{
		if (parse_argb_color (back, &((*style).colors.back)) != back)
		{
			(*style).relief.fore = GetHilite ((*style).colors.back);
			(*style).relief.back = GetShadow ((*style).colors.back);
			(*style).user_flags |= F_BACKCOLOR;
		}
	}
#ifndef NO_TEXTURE
	if ((maxcols != -1) && !((*style).user_flags & F_MAXCOLORS))
	{
		(*style).max_colors = maxcols;
		(*style).user_flags |= F_MAXCOLORS;
	}
	if (type >= 0)
	{
		switch (type)
		{
		 case TEXTURE_GRADIENT:
			 style->texture_type = TEXTURE_GRADIENT_TL2BR;
			 break;
		 case TEXTURE_HGRADIENT:
			 style->texture_type = TEXTURE_GRADIENT_L2R;
			 break;
		 case TEXTURE_HCGRADIENT:
			 style->texture_type = TEXTURE_GRADIENT_L2R;
			 break;
		 case TEXTURE_VGRADIENT:
			 style->texture_type = TEXTURE_GRADIENT_T2B;
			 break;
		 case TEXTURE_VCGRADIENT:
			 style->texture_type = TEXTURE_GRADIENT_T2B;
			 break;
		 default:
			 style->texture_type = type;
			 break;
		}
	}
	if ((type > 0) && (type < TEXTURE_PIXMAP) && !((*style).user_flags & F_BACKGRADIENT))
	{
		if (gradient != NULL)
		{
			ARGB32        c1, c2 = 0;
			ASGradient    grad;
			char         *ptr;

			ptr = (char *)parse_argb_color (gradient, &c1);
			parse_argb_color (ptr, &c2);
			if (ptr != gradient && (type = mystyle_parse_old_gradient (type, c1, c2, &grad)) >= 0)
			{
				if (style->user_flags & F_BACKGRADIENT)
				{
					free (style->gradient.color);
					free (style->gradient.offset);
				}
				style->gradient = grad;
				grad.type = mystyle_translate_grad_type (type);
				style->texture_type = type;
				style->user_flags |= F_BACKGRADIENT;
			} else
				fprintf (stderr, "%s: bad gradient: %s\n", MyName, gradient);
		}
	} else if ((type == TEXTURE_PIXMAP) && !((*style).user_flags & F_BACKPIXMAP))
	{
		if (pixmap != NULL)
		{
			int           colors = -1;

			if ((*style).set_flags & F_MAXCOLORS)
				colors = (*style).max_colors;
			if (((*style).back_icon.pix = GetXPMTile (pixmap, colors)) != None)
				(*style).user_flags |= F_BACKPIXMAP;
			else
				afterstep_err ("unable to load pixmap: '%s'", pixmap, NULL, NULL);
		}
	}
#endif
	(*style).set_flags = (*style).user_flags | (*style).inherit_flags;
}

static void
merge_old_look_font (MyStyle * style, MyFont * font)
{
	/* NOTE: these should have inherit_flags set, so the font is only
	 *       unloaded once */
	if (style != NULL && !(style->set_flags & F_FONT))
	{
		style->font = *font;
		style->inherit_flags |= F_FONT;
		style->user_flags &= ~F_FONT;		   /* to prevent confusion */
		style->set_flags = style->user_flags | style->inherit_flags;
	}
}

/*
 * merge the old variables into the new styles
 * the new styles have precedence
 */
void
merge_old_look_variables (void)
{
	MyStyle      *button_title_focus = mystyle_find ("ButtonTitleFocus");
	MyStyle      *button_title_sticky = mystyle_find ("ButtonTitleSticky");
	MyStyle      *button_title_unfocus = mystyle_find ("ButtonTitleUnfocus");

	/* the fonts */
	if (Stdfont != NULL)
	{
		if (load_font (Stdfont, &Scr.StdFont) == False)
		{
			fprintf (stderr, "%s: unable to load font %s\n", MyName, Stdfont);
			exit (1);
		}
		merge_old_look_font (Scr.MSMenuItem, &Scr.StdFont);
		merge_old_look_font (Scr.MSMenuHilite, &Scr.StdFont);
		merge_old_look_font (Scr.MSMenuStipple, &Scr.StdFont);
	}
	if (Windowfont != NULL)
	{
		if (load_font (Windowfont, &Scr.WindowFont) == False)
		{
			fprintf (stderr, "%s: unable to load font %s\n", MyName, Windowfont);
			exit (1);
		}
		merge_old_look_font (Scr.MSUWindow, &Scr.WindowFont);
		merge_old_look_font (Scr.MSFWindow, &Scr.WindowFont);
		merge_old_look_font (Scr.MSSWindow, &Scr.WindowFont);
		merge_old_look_font (Scr.MSMenuTitle, &Scr.WindowFont);
	}
	if (Iconfont != NULL)
	{
		if (load_font (Iconfont, &Scr.IconFont) == False)
		{
			fprintf (stderr, "%s: unable to load font %s\n", MyName, Iconfont);
			exit (1);
		}
		merge_old_look_font (button_title_focus, &Scr.IconFont);
		merge_old_look_font (button_title_sticky, &Scr.IconFont);
		merge_old_look_font (button_title_unfocus, &Scr.IconFont);
	}
	/* the text type */
	if (Scr.TitleTextType != 0)
	{
		if (((*Scr.MSUWindow).set_flags & F_TEXTSTYLE) == 0)
		{
			(*Scr.MSUWindow).text_style = Scr.TitleTextType;
			(*Scr.MSUWindow).user_flags |= F_TEXTSTYLE;
			(*Scr.MSUWindow).set_flags |= F_TEXTSTYLE;
		}
		if (((*Scr.MSFWindow).set_flags & F_TEXTSTYLE) == 0)
		{
			(*Scr.MSFWindow).text_style = Scr.TitleTextType;
			(*Scr.MSFWindow).user_flags |= F_TEXTSTYLE;
			(*Scr.MSFWindow).set_flags |= F_TEXTSTYLE;
		}
		if (((*Scr.MSSWindow).set_flags & F_TEXTSTYLE) == 0)
		{
			(*Scr.MSSWindow).text_style = Scr.TitleTextType;
			(*Scr.MSSWindow).user_flags |= F_TEXTSTYLE;
			(*Scr.MSSWindow).set_flags |= F_TEXTSTYLE;
		}
	}
	/* the colors */
	/* for black and white - ignore user choices */
	/* for color - accept user choices */
	if (Scr.d_depth > 1)
	{
		int           utype, ftype, stype, mttype, mhtype, mitype;	/* texture types */
		int           umax, fmax, smax, mtmax, mhmax, mimax;	/* max texture colors */

		utype = ftype = stype = mttype = mhtype = mitype = -1;
		umax = fmax = smax = mtmax = mhmax = mimax = -1;
#ifndef NO_TEXTURE
		if (TexTypes != NULL)
			sscanf (TexTypes, "%i %i %i %i %i %i", &ftype, &utype, &stype, &mttype, &mitype,
					&mhtype);
		if (TexMaxcols != NULL)
			sscanf (TexMaxcols, "%i %i %i %i %i %i", &fmax, &umax, &smax, &mtmax, &mimax, &mhmax);

		if (IconTexType == TEXTURE_BUILTIN)
			IconTexType = -1;
#endif /* !NO_TEXTURE */

		/* check for missing 1.4.5.x keywords */
		if (Mtitlefore == NULL && Hifore != NULL)
			Mtitlefore = mystrdup (Hifore);
		if (Mtitleback == NULL && Hiback != NULL)
			Mtitleback = mystrdup (Hiback);
		if (Menuhifore == NULL && Hifore != NULL)
			Menuhifore = mystrdup (Hifore);
		if (Menuhiback == NULL && Menuback != NULL)
		{
			mhtype = mitype;
			mhmax = mimax;
			Menuhiback = mystrdup (Menuback);
			if ((MHColor == NULL) && (IColor != NULL))
				MHColor = mystrdup (IColor);
			if ((MHPixmap == NULL) && (MPixmap != NULL))
				MHPixmap = mystrdup (MPixmap);
		}
		merge_old_look_colors (Scr.MSUWindow, utype, umax, Stdfore, Stdback, UColor, UPixmap);
		merge_old_look_colors (Scr.MSFWindow, ftype, fmax, Hifore, Hiback, TColor, TPixmap);
		merge_old_look_colors (Scr.MSSWindow, stype, smax, Stickyfore, Stickyback, SColor, SPixmap);
		merge_old_look_colors (Scr.MSMenuTitle, mttype, mtmax, Mtitlefore, Mtitleback, MColor,
							   MTPixmap);
		merge_old_look_colors (Scr.MSMenuItem, mitype, mimax, Menufore, Menuback, IColor, MPixmap);
		merge_old_look_colors (Scr.MSMenuHilite, mhtype, mhmax, Menuhifore, Menuhiback, MHColor,
							   MHPixmap);
		merge_old_look_colors (Scr.MSMenuStipple, mitype, mimax, Menustipple, Menuback, IColor,
							   MPixmap);

#ifndef NO_TEXTURE
		{
			MyStyle      *button_pixmap = mystyle_find ("ButtonPixmap");

			/* icon styles automagically inherit from window title styles */
			if (button_pixmap != NULL)
			{
				mystyle_merge_styles (Scr.MSFWindow, button_pixmap, 0, 0);
				merge_old_look_colors (button_pixmap, IconTexType, IconMaxColors, NULL, IconBgColor,
									   IconTexColor, IconPixmapFile);
			}
		}
#endif /* !NO_TEXTURE */
		if (button_title_focus != NULL)
			mystyle_merge_styles (Scr.MSFWindow, button_title_focus, 0, 0);
		if (button_title_sticky != NULL)
			mystyle_merge_styles (Scr.MSSWindow, button_title_sticky, 0, 0);
		if (button_title_unfocus != NULL)
			mystyle_merge_styles (Scr.MSUWindow, button_title_unfocus, 0, 0);
	}
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
		if (PixmapPath != NULL)
			free (PixmapPath);
	}

	IconPath = NULL;
	ModulePath = NULL;
	PixmapPath = NULL;
}

/*
 * Initialize look variables
 */
void
InitLook (Bool free_resources)
{
	int           i;

	balloon_init (free_resources);

	if (free_resources)
	{
		/* styles/textures */
		while (mystyle_first != NULL)
			mystyle_delete (mystyle_first);

		/* GCs */
		if (Scr.LineGC != None)
			XFreeGC (dpy, Scr.LineGC);
		if (Scr.DrawGC != None)
			XFreeGC (dpy, Scr.DrawGC);
		if (Scr.NormalGC != None)
			XFreeGC (dpy, Scr.NormalGC);
		if (Scr.StippleGC != None)
			XFreeGC (dpy, Scr.StippleGC);
		if (Scr.ScratchGC1 != None)
			XFreeGC (dpy, Scr.ScratchGC1);
		if (Scr.ScratchGC2 != None)
			XFreeGC (dpy, Scr.ScratchGC2);

		/* fonts */
		unload_font (&Scr.StdFont);
		unload_font (&Scr.WindowFont);
		unload_font (&Scr.IconFont);

#ifndef NO_TEXTURE
		/* icons */
		if (Scr.MenuArrow.pix != None)
			UnloadImage (Scr.MenuArrow.pix);
		if (Scr.MenuPinOn.pix != None)
			UnloadImage (Scr.MenuPinOn.pix);
		if (Scr.MenuPinOff.pix != None)
			UnloadImage (Scr.MenuPinOff.pix);

		/* cached gradients */
		if (Scr.TitleGradient != None)
			XFreePixmap (dpy, Scr.TitleGradient);
#endif /* !NO_TEXTURE */

		/* titlebar buttons */
		for (i = 0; i < 10; i++)
		{
			if (Scr.button_pixmap[i] != None)
				UnloadImage (Scr.button_pixmap[i]);
			if (Scr.dbutton_pixmap[i] != None)
				UnloadImage (Scr.dbutton_pixmap[i]);
		}

#ifndef NO_TEXTURE
		/* iconized window background */
		if (IconBgColor != NULL)
			free (IconBgColor);
		if (IconTexColor != NULL)
			free (IconTexColor);
		if (IconPixmapFile != NULL)
			free (IconPixmapFile);
#endif /* !NO_TEXTURE */

		/* resize/move window geometry */
		if (RMGeom != NULL)
			free (RMGeom);
	}
	/* styles/textures */
	mystyle_first = NULL;
	Scr.MSDefault = NULL;
	Scr.MSFWindow = NULL;
	Scr.MSUWindow = NULL;
	Scr.MSSWindow = NULL;
	Scr.MSMenuTitle = NULL;
	Scr.MSMenuItem = NULL;
	Scr.MSMenuHilite = NULL;
	Scr.MSMenuStipple = NULL;

	/* GCs */
	Scr.LineGC = None;
	Scr.DrawGC = None;
	Scr.NormalGC = None;
	Scr.StippleGC = None;
	Scr.ScratchGC1 = None;
	Scr.ScratchGC2 = None;

	/* fonts */
	Scr.StdFont.name = NULL;
	Scr.WindowFont.name = NULL;
	Scr.IconFont.name = NULL;
	Scr.StdFont.font = NULL;
	Scr.WindowFont.font = NULL;
	Scr.IconFont.font = NULL;
#ifdef I18N
	Scr.StdFont.fontset = NULL;
	Scr.WindowFont.fontset = NULL;
	Scr.IconFont.fontset = NULL;
#endif

#ifndef NO_TEXTURE
	/* icons */
	Scr.MenuArrow.pix = Scr.MenuArrow.mask = None;
	Scr.MenuPinOn.pix = Scr.MenuPinOn.mask = None;
	Scr.MenuPinOff.pix = Scr.MenuPinOff.mask = None;

	/* cached gradients */
	Scr.TitleGradient = None;

	/* titlebar text */
	Scr.TitleStyle = TITLE_OLD;
#endif /* !NO_TEXTURE */
	Scr.TitleTextAlign = 0;

	/* titlebar buttons */
	Scr.nr_right_buttons = 0;
	Scr.nr_left_buttons = 0;
	Scr.ButtonType = 0;
	Scr.TitleButtonSpacing = 2;
	Scr.TitleButtonStyle = 0;
	for (i = 0; i < 10; i++)
	{
		Scr.button_style[i] = NO_BUTTON_STYLE;
		Scr.button_width[i] = 0;
		Scr.button_height[i] = 0;
		Scr.button_pixmap[i] = None;
		Scr.button_pixmap_mask[i] = None;
		Scr.dbutton_pixmap[i] = None;
		Scr.dbutton_pixmap_mask[i] = None;
	}

#ifndef NO_TEXTURE
	/* iconized window background */
	IconTexType = TEXTURE_BUILTIN;
	IconBgColor = NULL;
	IconTexColor = NULL;
	IconPixmapFile = NULL;
#endif /* !NO_TEXTURE */
	Scr.ButtonWidth = 0;
	Scr.ButtonHeight = 0;

	/* resize/move window geometry */
	RMGeom = NULL;

#ifndef NO_TEXTURE
	/* frames */
	frame_init (True);
#endif /* !NO_TEXTURE */

	/* miscellaneous stuff */
	RubberBand = 0;
	DrawMenuBorders = 1;
	TextureMenuItemsIndividually = 1;
	Textures.flags = SeparateButtonTitle;
	MenuMiniPixmaps = 0;
	Scr.NumBoxes = 0;
}

/*
 * Initialize feel variables
 */
void
InitFeel (Bool free_resources)
{
	if (free_resources)
	{
		while (Scr.MouseButtonRoot != NULL)
		{
			MouseButton  *mb = Scr.MouseButtonRoot;

			Scr.MouseButtonRoot = mb->NextButton;
			if (mb->action)
				free (mb->action);
			free (mb);
		}
		while (Scr.FuncKeyRoot != NULL)
		{
			FuncKey      *fk = Scr.FuncKeyRoot;

			Scr.FuncKeyRoot = fk->next;
			if (fk->name != NULL)
				free (fk->name);
			if (fk->action != NULL)
				free (fk->action);
			free (fk);
		}
	}
	StartMenuSortMode = 0;
	AutoReverse = 0;
	AutoTabThroughDesks = 0;
	DoHandlePageing = True;
	Xzap = 12;
	Yzap = 12;
	StartMenuSortMode = DEFAULTSTARTMENUSORT;
	Scr.VScale = 1;
	Scr.EdgeScrollX = Scr.EdgeScrollY = -100000;
	Scr.ScrollResistance = Scr.MoveResistance = 0;
	Scr.OpaqueSize = 5;
	Scr.OpaqueResize = 0;
	Scr.ClickTime = 150;
	Scr.AutoRaiseDelay = 0;
	Scr.RaiseButtons = 0;
	Scr.flags = 0;

	Scr.MouseButtonRoot = NULL;
	Scr.FuncKeyRoot = NULL;
}

/*
 * Initialize database variables
 */
void
InitDatabase (Bool free_resources)
{
	if (free_resources)
	{
		while (Scr.TheList != NULL)
			style_delete (Scr.TheList);
		if (Scr.DefaultIcon != NULL)
			free (Scr.DefaultIcon);
	}
	Scr.TheList = NULL;
	Scr.DefaultIcon = NULL;
}

/*
 * Create/destroy window titlebar/buttons as necessary.
 */
void
titlebar_sanity_check (void)
{
	int           i;
	ASWindow     *t;

	for (i = 4; i >= 0; i--)
		if (Scr.button_style[i * 2 + 1] != NO_BUTTON_STYLE)
			break;
	Scr.nr_left_buttons = i + 1;
	for (i = 4; i >= 0; i--)
		if (Scr.button_style[(i * 2 + 2) % 10] != NO_BUTTON_STYLE)
			break;
	Scr.nr_right_buttons = i + 1;
	/* traverse window list and redo the titlebar/buttons if necessary */
	for (t = Scr.ASRoot.next; t != NULL; t = t->next)
	{
		if (t->flags & TITLE)
		{
			int           width = t->frame_width;
			int           height = t->frame_height;

			init_titlebutton_windows (t, True);
			create_titlebutton_windows (t);
			t->frame_width = t->frame_height = 0;	/* force reconfigure titlebar */
			SetupFrame (t, t->frame_x, t->frame_y, width, height, 0);
			XMapSubwindows (dpy, t->frame);
		}
	}
}

void
make_styles (void)
{
/* make sure the globals are defined */
	if (Scr.MSDefault == NULL)
	{
		if ((Scr.MSDefault = mystyle_find ("default")) == NULL)
			Scr.MSDefault = mystyle_new_with_name ("default");
	}
	/* for now, the default style must be named "default" */
	else if (mystrcasecmp ((*Scr.MSDefault).name, "default"))
	{
		free ((*Scr.MSDefault).name);
		(*Scr.MSDefault).name = mystrdup ("default");
	}
	if (Scr.MSFWindow == NULL)
		Scr.MSFWindow = mystyle_new_with_name ("FWindow");
	if (Scr.MSUWindow == NULL)
		Scr.MSUWindow = mystyle_new_with_name ("UWindow");
	if (Scr.MSSWindow == NULL)
		Scr.MSSWindow = mystyle_new_with_name ("SWindow");
	if (Scr.MSMenuTitle == NULL)
		Scr.MSMenuTitle = mystyle_new_with_name ("MenuTitle");
	if (Scr.MSMenuItem == NULL)
		Scr.MSMenuItem = mystyle_new_with_name ("MenuItem");
	if (Scr.MSMenuHilite == NULL)
		Scr.MSMenuHilite = mystyle_new_with_name ("MenuHilite");
	if (Scr.MSMenuStipple == NULL)
		Scr.MSMenuStipple = mystyle_new_with_name ("MenuStipple");
	if (mystyle_find ("ButtonPixmap") == NULL)
		mystyle_new_with_name ("ButtonPixmap");
	if (mystyle_find ("ButtonTitleFocus") == NULL)
		mystyle_new_with_name ("ButtonTitleFocus");
	if (mystyle_find ("ButtonTitleSticky") == NULL)
		mystyle_new_with_name ("ButtonTitleSticky");
	if (mystyle_find ("ButtonTitleUnfocus") == NULL)
		mystyle_new_with_name ("ButtonTitleUnfocus");
}

void
CheckASTree (int thisdesk, Bool parse_look, Bool parse_feel)
{
	char          file[64];

	/* Create missing directories & put there defaults */
	if (CheckDir (as_dirs.afters_noncfdir) != 0)
	{
		int           i;

		fprintf (stderr, "\n");
		CheckOrCreate (GNUSTEP);
		CheckOrCreate (GNUSTEPLIB);
		CheckOrCreate (as_dirs.after_dir);
		CheckOrCreate (as_dirs.afters_noncfdir);
		CheckOrCreateFile (AFTER_SAVE);

		for (i = 0; i < 4; i++)
		{
			sprintf (file, BACK_FILE, i);
			HomeCreateIfNeeded (file);
		}
	}
	if (parse_look)
	{
		sprintf (file, LOOK_FILE, thisdesk, Scr.d_depth);
		HomeCreateIfNeeded (file);
	}
	if (parse_feel)
	{
		sprintf (file, FEEL_FILE, thisdesk, Scr.d_depth);
		HomeCreateIfNeeded (file);
	}

}

int
ParseConfigFile (const char *file, char **tline)
{
	char         *realfilename;
	FILE         *fp = NULL;
	register char *ptr;

	/* memory management for parsing buffer */
	if (file == NULL)
		return -1;

	realfilename = make_file_name (as_dirs.after_dir, file);
	if (CheckFile (realfilename) != 0)
	{
		free (realfilename);
		realfilename = make_file_name (as_dirs.after_sharedir, file);
		if (CheckFile (realfilename) != 0)
		{
			free (realfilename);
			return -1;
		}
	}
	/* this should not happen, but still checking */
	if ((fp = fopen (realfilename, "r")) == (FILE *) NULL)
	{
		afterstep_err
			("can't open %s, exiting now.\nMost likely you have incorrect permissions on the AfterStep configuration dir.",
			 file, NULL, NULL);
		exit (1);
	}
	free (realfilename);

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
 *
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/
/* MakeMenus - for those who can't remember LoadASConfig's real name */
void
LoadASConfig (const char *display_name, int thisdesktop, Bool parse_menu,
			  Bool parse_look, Bool parse_feel)
{
	ASWindow     *t;
	int           parse_base = 1, parse_database = 1;
	char         *tline = NULL;

#ifndef DIFFERENTLOOKNFEELFOREACHDESKTOP
	/* only one look & feel should be used */
	thisdesktop = 0;
#endif /* !DIFFERENTLOOKNFEELFOREACHDESKTOP */

	/* keep client window geometry, and nuke old frame geometry */
	for (t = Scr.ASRoot.next; t != NULL; t = t->next)
		get_client_geometry (t, t->frame_x, t->frame_y, t->frame_width, t->frame_height,
							 &t->frame_x, &t->frame_y, &t->frame_width, &t->frame_height);

	/* kludge: make sure functions get updated */
	if (parse_menu)
		parse_feel = True;

	/* always parse database */
	InitDatabase (True);

	/* base.* variables */
	if (parse_base || shall_override_config_file)
	{
		InitBase (True);
		Scr.VxMax = 1;
		Scr.VyMax = 1;
	}

	if (parse_look || shall_override_config_file)
	{
		InitLook (True);
		init_old_look_variables (True);
	}

	if (parse_feel || shall_override_config_file)
		InitFeel (True);

	XORvalue = (((unsigned long)1) << Scr.d_depth) - 1;

	/* initialize some lists */
	Scr.DefaultIcon = NULL;

	/* free pixmaps that are no longer in use */
	pixmap_ref_purge ();

	fprintf (stderr, "Detected colordepth : %d. Loading configuration ", Scr.d_depth);
	if (!shall_override_config_file)
	{
		char          configfile[64];

		CheckASTree (thisdesktop, parse_look, parse_feel);
		if (parse_base)
		{
			sprintf (configfile, "%s.%dbpp", BASE_FILE, Scr.d_depth);
			ParseConfigFile (configfile, &tline);
			/* Save base filename to pass to modules */
			if (global_base_file != NULL)
				free (global_base_file);
			global_base_file = mystrdup (configfile);
		}
		fprintf (stderr, ".");
		if (parse_look)
		{
			Bool          done = False;

			if (Scr.screen != 0)
			{
				sprintf (configfile, LOOK_FILE ".scr%ld", thisdesktop, Scr.d_depth, Scr.screen);
				done = (ParseConfigFile (configfile, &tline) > 0);
			}
			if (!done)
			{
				sprintf (configfile, LOOK_FILE, thisdesktop, Scr.d_depth);
/*fprintf( stderr, "screen = %ld, look :[%s]\n", Scr.screen, configfile );*/
				ParseConfigFile (configfile, &tline);
			}
		}
		fprintf (stderr, ".");
		if (parse_menu)
		{
			if (tline == NULL)
				tline = safemalloc (MAXLINELENGTH + 1);
			MeltStartMenu (tline);
		}
		fprintf (stderr, ".");
		if (parse_feel)
		{
			Bool          done = False;

			if (Scr.screen != 0)
			{
				sprintf (configfile, FEEL_FILE ".scr%ld", thisdesktop, Scr.d_depth, Scr.screen);
				done = (ParseConfigFile (configfile, &tline) > 0);
			}
			if (!done)
			{
				sprintf (configfile, FEEL_FILE, thisdesktop, Scr.d_depth);
				ParseConfigFile (configfile, &tline);
			}
		}
		if (parse_feel || parse_look)
		{
			Bool          done = False;

			if (Scr.screen != 0)
			{
				sprintf (configfile, THEME_FILE ".scr%ld", thisdesktop, Scr.d_depth, Scr.screen);
				done = (ParseConfigFile (configfile, &tline) > 0);
			}
			if (!done)
			{
				sprintf (configfile, THEME_FILE, thisdesktop, Scr.d_depth);
				ParseConfigFile (configfile, &tline);
			}
			ParseConfigFile (THEME_OVERRIDE_FILE, &tline);
		}
		fprintf (stderr, ".");
		ParseConfigFile (AUTOEXEC_FILE, &tline);
		fprintf (stderr, ".");
		ParseConfigFile (DATABASE_FILE, &tline);
		fprintf (stderr, ".");
	} else
	{
		/* Yes, override config file */
		ParseConfigFile (config_file_to_override, &tline);
		fprintf (stderr, "......");
	}
	free_func_hash ();

	/* let's free the memory used for parsing */
	if (tline)
		free (tline);
	fprintf (stderr, ". Done.\n");

	if (parse_base || shall_override_config_file)
	{
		Scr.VxMax = Scr.VxMax * Scr.MyDisplayWidth - Scr.MyDisplayWidth;
		Scr.VyMax = Scr.VyMax * Scr.MyDisplayHeight - Scr.MyDisplayHeight;
		if (Scr.VxMax < 0)
			Scr.VxMax = 0;
		if (Scr.VyMax < 0)
			Scr.VyMax = 0;

		if (Scr.VxMax == 0)
			Scr.flags &= ~EdgeWrapX;
		if (Scr.VyMax == 0)
			Scr.flags &= ~EdgeWrapY;
	}

	if (parse_look || shall_override_config_file)
	{
		/* make sure all needed styles are created */
		make_styles ();

		/* merge pre-1.5 compatibility keywords */
		merge_old_look_variables ();

		/* fill in remaining members with the default style */
		mystyle_fix_styles ();

		mystyle_set_property (dpy, Scr.Root, _AS_STYLE, XA_INTEGER);

#ifndef NO_TEXTURE
		if (MArrowPixmap != NULL && !GetIconFromFile (MArrowPixmap, &Scr.MenuArrow, -1))
			fprintf (stderr, "couldn't load menu arrow pixmap\n");

		/* update frame geometries */
		if (DecorateFrames)
			frame_create_gcs ();
#endif /* ! NO_TEXTURE */
	}

	/* update the resize/move window geometry */
	if (parse_look || shall_override_config_file)
	{
		int           invalid_RMGeom = 0;
		int           x = 0, y = 0;
		int           width, height;

		height = (*Scr.MSFWindow).font.height + SIZE_VINDENT * 2;
		Scr.SizeStringWidth = XTextWidth ((*Scr.MSFWindow).font.font, " +8888 x +8888 ", 15);
		XSetWindowBorder (dpy, Scr.SizeWindow, (*Scr.MSFWindow).colors.fore);
		XSetWindowBackground (dpy, Scr.SizeWindow, (*Scr.MSFWindow).colors.back);

		width = Scr.SizeStringWidth + SIZE_HINDENT * 2;

		if ((RMGeom != NULL) && (strlen (RMGeom) == 2))
		{
			if (RMGeom[0] == '+')
				x = 0;
			else if (RMGeom[0] == '-')
				x = DisplayWidth (dpy, Scr.screen) - width;
			else
				invalid_RMGeom = 1;

			if (RMGeom[1] == '+')
				y = 0;
			else if (RMGeom[1] == '-')
				y = DisplayHeight (dpy, Scr.screen) - height;
			else
				invalid_RMGeom = 1;
		} else
			invalid_RMGeom = 1;				   /* not necessarily invalid--maybe unspecified */

		if (invalid_RMGeom)
		{
			/* the default case is, of course, to center the R/M window */
			x = (DisplayWidth (dpy, Scr.screen) - width) / 2;
			y = (DisplayHeight (dpy, Scr.screen) - height) / 2;
		}
		XMoveResizeWindow (dpy, Scr.SizeWindow, x, y, width, height);
	}

	if (parse_look || shall_override_config_file)
		GetColors ();

	if (parse_feel || shall_override_config_file)
	{
		/* If no edge scroll line is provided in the setup file, use a default */
		if (Scr.EdgeScrollX == -100000)
			Scr.EdgeScrollX = 25;
		if (Scr.EdgeScrollY == -100000)
			Scr.EdgeScrollY = Scr.EdgeScrollX;

		if ((Scr.flags & ClickToRaise) && (Scr.AutoRaiseDelay == 0))
			Scr.AutoRaiseDelay = -1;

		/* if edgescroll >1000 and < 100000m
		 * wrap at edges of desktop (a "spherical" desktop) */
		if (Scr.EdgeScrollX >= 1000)
		{
			Scr.EdgeScrollX /= 1000;
			Scr.flags |= EdgeWrapX;
		}
		if (Scr.EdgeScrollY >= 1000)
		{
			Scr.EdgeScrollY /= 1000;
			Scr.flags |= EdgeWrapY;
		}
		Scr.EdgeScrollX = Scr.EdgeScrollX * Scr.MyDisplayWidth / 100;
		Scr.EdgeScrollY = Scr.EdgeScrollY * Scr.MyDisplayHeight / 100;
	}

	/* update the menus */
	if (parse_look || parse_feel || parse_menu || shall_override_config_file)
	{
		MenuRoot     *menu;

		for (menu = Scr.first_menu; menu != NULL; menu = (*menu).next)
			MakeMenu (menu);
	}

	/* reset the window frame geometries */
	for (t = Scr.ASRoot.next; t != NULL; t = t->next)
	{
		name_list     nl;

		style_init (&nl);
		style_fill_by_name (&nl, t->name, NULL, t->class.res_name, t->class.res_class);
		if (!(nl.off_flags & STYLE_FOCUS_FLAG) ||
			(t->style_focus = mystyle_find (nl.style_focus)) == NULL)
			t->style_focus = Scr.MSFWindow;
		if (!(nl.off_flags & STYLE_UNFOCUS_FLAG) ||
			(t->style_unfocus = mystyle_find (nl.style_unfocus)) == NULL)
			t->style_unfocus = Scr.MSUWindow;
		if (!(nl.off_flags & STYLE_STICKY_FLAG) ||
			(t->style_sticky = mystyle_find (nl.style_sticky)) == NULL)
			t->style_sticky = Scr.MSSWindow;

		set_titlebar_geometry (t);
#ifndef NO_TEXTURE
		frame_set_positions (t);
		get_frame_geometry (t, t->frame_x, t->frame_y, t->frame_width, t->frame_height, &t->frame_x,
							&t->frame_y, &t->frame_width, &t->frame_height);
#endif /* !NO_TEXTURE */
	}

	/* setup the titlebar buttons */
	if (parse_look || shall_override_config_file)
	{
		balloon_setup (dpy);
		balloon_set_style (dpy, mystyle_find_or_default ("TitleButtonBalloon"));
		titlebar_sanity_check ();
	}

	/* grab the new button/keybindings */
	if (parse_feel || shall_override_config_file)
	{
		ASWindow     *win;

		for (win = Scr.ASRoot.next; win != NULL; win = win->next)
		{
			XUngrabKey (dpy, AnyKey, AnyModifier, win->w);
			XUngrabButton (dpy, AnyButton, AnyModifier, win->w);
			GrabKeys (win);
			GrabButtons (win);
		}
	}

	/* force update of window frames */
	if (!parse_feel || shall_override_config_file)
	{
		ASWindow     *win;

		for (win = Scr.ASRoot.next; win != NULL; win = win->next)
		{
#ifndef NO_TEXTURE
			win->bp_width = -1;				   /* force recreate gradients */
#endif
			SetupFrame (win, win->frame_x, win->frame_y, win->frame_width, win->frame_height, 0);
			SetBorder (win, Scr.Hilite == win, True, True, None);
		}
	}

	/* redo icons in case IconBox, ButtonSize, SeparateButtonTitle, or one
	 * of the Icon definitions in database changed */
	if (parse_database || parse_look || shall_override_config_file)
	{
		ASWindow     *win;

		for (win = Scr.ASRoot.next; win != NULL; win = win->next)
			ChangeIcon (win);
		AutoPlaceStickyIcons ();
	}
}

/*****************************************************************************
 *
 * Copies a text string from the config file to a specified location
 *
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
	char         *as_theme_data = make_file_name (as_dirs.after_dir, THEME_DATA_DIR);
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

/*****************************************************************************
 *
 * Loads a pixmap to the assigned location
 *
 ****************************************************************************/

void
assign_pixmap (char *text, FILE * fd, char **arg, int *junk)
{
	char         *tmp = stripcpy (text);

	if (!GetIconFromFile (tmp, (MyIcon *) arg, -1))
		fprintf (stderr, "unable to load icon '%s'\n", tmp);
	free (tmp);
}

/****************************************************************************
 *
 *  Read TitleText Controls
 *
 ****************************************************************************/

void
SetTitleText (char *tline, FILE * fd, char **junk, int *junk2)
{
#ifndef NO_TEXTURE
	int           n;
	int           ttype, y;

	n = sscanf (tline, "%d %d %s %s %s %s %s %s", &ttype, &y, hircolor,
				hiscolor, hincolor, lorcolor, loscolor, loncolor);

	if (n != 8)
	{
		fprintf (stderr, "wrong number of parameters given to TitleText\n");
		fprintf (stderr, "t=%i y=%i 1=%s 2=%s 3=%s 4=%s 5=%s 6=%s\n", ttype, y, hircolor,
				 hiscolor, hincolor, lorcolor, loscolor, loncolor);
		return;
	}
	Scr.TitleTextType = ttype;
	Scr.TitleTextY = y;
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
	extern char  *PixmapPath;
	char         *path = NULL;
	int           num;
	char          file[256], file2[256];
	int           fnamelen = 0, offset = 0, linelen;
	int           n;
	unsigned int  dum;
	int           dummy;
	Window        root;
	int           width, height;

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


	path = findIconFile (file, PixmapPath, R_OK);
	if (path == NULL)
	{
		fprintf (stderr, "couldn't find Titlebar button %s\n", file);
		return;
	}
	Scr.button_pixmap[num] =
		LoadImageWithMask (dpy, Scr.Root, 256, path, &Scr.button_pixmap_mask[num]);
	free (path);
	if (Scr.button_pixmap[num] == None)
		return;

	XGetGeometry (dpy, Scr.button_pixmap[num], &root, &dummy, &dummy,
				  &(Scr.button_width[num]), &(Scr.button_height[num]), &dum, &dum);
	Scr.button_style[num] = XPM_BUTTON_STYLE;

	/* check if the button's height is bigger that the current highest one */
	if ((path = findIconFile (file2, PixmapPath, R_OK)) == NULL)
	{
		fprintf (stderr, "couldn't find Titlebar button %s\n", file2);
		return;
	}
	Scr.ButtonType = 1;

	Scr.dbutton_pixmap[num] =
		LoadImageWithMask (dpy, Scr.Root, 256, path, &Scr.dbutton_pixmap_mask[num]);
	free (path);
	if (Scr.dbutton_pixmap[num] == None)
		return;

	XGetGeometry (dpy, Scr.dbutton_pixmap[num], &root, &dummy, &dummy, &width, &height, &dum, &dum);

	Scr.button_width[num] = max (width, Scr.button_width[num]);
	Scr.button_height[num] = max (height, Scr.button_height[num]);
	Scr.button_style[num] = XPM_BUTTON_STYLE;
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
		Scr.ASCursors[cursor_num] = XCreateFontCursor (dpy, cursor_style);
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
	Scr.ASCursors[cursor_num] = XCreatePixmapCursor (dpy, cursor, mask, &fore, &back, x, y);
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
	Scr.flags |= (unsigned long)arg;
	if (another)
	{
		long          i = strtol (text, NULL, 0);

		if (i)
			Scr.flags |= (unsigned long)another;
	}
}

void
SetFlag2 (char *text, FILE * fd, char **arg, int *var)
{
	unsigned long *flags = (unsigned long *)var;
	char         *ptr;
	int           val = strtol (text, &ptr, 0);

	if (flags == NULL)
		flags = &Scr.flags;
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

	for (i = 0; i < MAX_BUTTONS; i++)
	{
		b = (int)strtol (text, &next, 0);
		if (next == text)
			break;
		text = next;
		if (*text == ',')
			text++;
		if ((b > 0) && (b <= MAX_BUTTONS))
			Scr.RaiseButtons |= 1 << b;
	}
	Scr.flags |= ClickToRaise;
}


/*****************************************************************************
 *
 * Reads Dimensions for an icon box from the config file
 *
 ****************************************************************************/

void
SetBox (char *text, FILE * fd, char **arg, int *junk)
{
	int           x1, y1, x2, y2, num;

	if (Scr.NumBoxes >= MAX_BOXES)
	{
		fprintf (stderr, "too many IconBoxes (max is %d)\n", MAX_BOXES);
		return;
	}

	/* Standard X11 geometry string */
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

	if (num < 4 || x1 >= x2 || y1 >= y2 ||
		x1 < 0 || x1 > Scr.MyDisplayWidth || x2 < 0 || x2 > Scr.MyDisplayWidth ||
		y1 < 0 || y1 > Scr.MyDisplayHeight || y2 < 0 || y2 > Scr.MyDisplayHeight)
		fprintf (stderr, "invalid IconBox '%s'\n", text);
	else
	{
		Scr.IconBoxes[Scr.NumBoxes][0] = x1;
		Scr.IconBoxes[Scr.NumBoxes][1] = y1;
		Scr.IconBoxes[Scr.NumBoxes][2] = x2;
		Scr.IconBoxes[Scr.NumBoxes][3] = y2;
		Scr.NumBoxes++;
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

	switch (StartMenuSortMode)
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

	if (CheckDir (as_dirs.after_dir) == 0)
	{
		as_start = make_file_name (as_dirs.after_dir, START_DIR);
		if (CheckDir (as_start) != 0)
		{
			free (as_start);
			as_start = NULL;
		}
	}
	if (as_start == NULL)
	{
		printf ("Using system wide defaults from '%s'", as_dirs.after_sharedir);
		as_start = make_file_name (as_dirs.after_sharedir, START_DIR);
		if (CheckDir (as_start) != 0)
		{
			free (as_start);
			perror ("unable to locate the menu directory");
			Done (0, NULL);
			return 0;
		}
	}
	tree = dirtree_new_from_dir (as_start);
	free (as_start);

#ifdef FIXED_DIR
	{
		char         *as_fixeddir = make_file_name (as_dirs.after_sharedir, FIXED_DIR);

		if (CheckDir (as_fixeddir) == 0)
		{
			dirtree_t    *fixed_tree = dirtree_new_from_dir (as_fixeddir);

			free (as_fixeddir);
			dirtree_move_children (tree, fixed_tree);
			dirtree_delete (fixed_tree);
		} else
			perror ("unable to locate the fixed menu directory");
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
 * This routine loads all needed colors, and fonts,
 * and creates the GC's
 *
 ***************************************************************************/

void
GetColors (void)
{
	if (have_the_colors)
		return;

	have_the_colors = 1;

	/* create graphics contexts */
	CreateGCs ();
	XSync (dpy, 0);
	return;
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
	table = find_config (table, text);
	if (table != NULL)
	{
		for (text += strlen (table->keyword); isspace (*text); text++);
		table->action (text, fd, table->arg, table->arg2);
	} else
		tline_error (error_msg);
}



/***********************************************************************
 *
 *  Procedure:
 *	CreateGCs - open fonts and create all the needed GC's.  I only
 *		    want to do this once, hence the first_time flag.
 *
 ***********************************************************************/

void
CreateGCs (void)
{
	XGCValues     gcv;
	unsigned long gcm;

	gcm = GCForeground | GCBackground | GCGraphicsExposures;

	gcv.foreground = (*Scr.MSFWindow).colors.fore;
	gcv.background = (*Scr.MSFWindow).colors.back;
	gcv.graphics_exposures = False;

	gcm = GCLineWidth | GCForeground | GCBackground | GCFunction;
	gcv.function = GXcopy;
	gcv.line_width = 1;

	gcv.foreground = (*Scr.MSFWindow).colors.fore;
	gcv.background = (*Scr.MSFWindow).colors.back;

	Scr.LineGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);

	gcm = GCFunction | GCLineWidth | GCForeground | GCSubwindowMode;
	gcv.function = GXxor;
	gcv.line_width = 0;
	gcv.foreground = XORvalue;
	gcv.subwindow_mode = IncludeInferiors;
	Scr.DrawGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);

	gcm = GCFunction | GCPlaneMask | GCGraphicsExposures | GCLineWidth |
		GCForeground | GCBackground | GCFont;
	gcv.line_width = 0;
	gcv.function = GXcopy;
	gcv.plane_mask = AllPlanes;

	gcv.foreground = (*Scr.MSFWindow).colors.fore;
	gcv.background = (*Scr.MSFWindow).colors.back;
	gcv.font = (*Scr.MSFWindow).font.font->fid;
	/*
	 * Prevent GraphicsExpose and NoExpose events.  We'd only get NoExpose
	 * events anyway;  they cause BadWindow errors from XGetWindowAttributes
	 * call in FindScreenInfo (events.c) (since drawable is a pixmap).
	 */
	gcv.graphics_exposures = False;

	Scr.NormalGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);

	gcv.fill_style = FillStippled;
	gcv.stipple = Scr.gray_bitmap;
	gcm = GCFunction | GCPlaneMask | GCGraphicsExposures | GCLineWidth | GCForeground |
		GCBackground | GCFont | GCStipple | GCFillStyle;

	Scr.StippleGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);

	gcm = GCFunction | GCPlaneMask | GCGraphicsExposures | GCLineWidth | GCForeground |
		GCBackground | GCFont;
	Globalgcm = gcm;
	Globalgcv = gcv;
	gcv.foreground = (*Scr.MSFWindow).relief.fore;
	gcv.background = (*Scr.MSFWindow).relief.back;
	Scr.ScratchGC1 = XCreateGC (dpy, Scr.Root, gcm, &gcv);

	gcv.foreground = (*Scr.MSFWindow).relief.back;
	gcv.background = (*Scr.MSFWindow).relief.fore;
	Scr.ScratchGC2 = XCreateGC (dpy, Scr.Root, gcm, &gcv);
}
