/*
 * Copyright (c) 2000 Andrew Ferguson <andrew@owsla.cjb.net>
 * Copyright (c) 1998 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#define LOCAL_DEBUG

#include "../configure.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/hints.h"
#include "../libAfterStep/mystyle.h"
#include "../libAfterStep/screen.h"
#include "../libAfterStep/mylook.h"

#include "afterconf.h"


TermDef       AlignTerms[] = {
    {TF_NO_MYNAME_PREPENDING, "Left", 4,        TT_FLAG, ALIGN_Left_ID      , NULL},
    {TF_NO_MYNAME_PREPENDING, "Top", 3,         TT_FLAG, ALIGN_Top_ID       , NULL},
    {TF_NO_MYNAME_PREPENDING, "Right", 5,       TT_FLAG, ALIGN_Right_ID     , NULL},
    {TF_NO_MYNAME_PREPENDING, "Bottom", 6,      TT_FLAG, ALIGN_Bottom_ID    , NULL},
    {TF_NO_MYNAME_PREPENDING, "HTiled", 6,      TT_FLAG, ALIGN_HTiled_ID    , NULL},
    {TF_NO_MYNAME_PREPENDING, "VTiled", 6,      TT_FLAG, ALIGN_VTiled_ID    , NULL},
    {TF_NO_MYNAME_PREPENDING, "HScaled", 7,     TT_FLAG, ALIGN_HScaled_ID   , NULL},
    {TF_NO_MYNAME_PREPENDING, "VScaled", 7,     TT_FLAG, ALIGN_VScaled_ID   , NULL},
    {TF_NO_MYNAME_PREPENDING, "LabelSize", 9,   TT_FLAG, ALIGN_LabelSize_ID , NULL},
    {TF_NO_MYNAME_PREPENDING, "LabelWidth", 10,    TT_FLAG, ALIGN_LabelWidth_ID,  NULL},
    {TF_NO_MYNAME_PREPENDING, "LabelHeight", 11,   TT_FLAG, ALIGN_LabelHeight_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "Center", 6,      TT_FLAG, ALIGN_Center_ID    , NULL},
    {TF_NO_MYNAME_PREPENDING, "HCenter", 7,     TT_FLAG, ALIGN_HCenter_ID   , NULL},
    {TF_NO_MYNAME_PREPENDING, "VCenter", 7,     TT_FLAG, ALIGN_VCenter_ID   , NULL},
    {0, NULL, 0, 0, 0}
};

SyntaxDef     AlignSyntax = {
	',',
	'\n',
    AlignTerms,
	0,										   /* use default hash size */
    ' ',
	" ",
	"\t",
    "Alignment flags",
	"Align",
	NULL,
	0
};

flag_options_xref AlignFlagsXref[] = {
    {ALIGN_LEFT, ALIGN_Left_ID     , 0},
    {ALIGN_TOP, ALIGN_Top_ID      , 0},
    {ALIGN_RIGHT, ALIGN_Right_ID    , 0},
    {ALIGN_BOTTOM, ALIGN_Bottom_ID   , 0},
    {RESIZE_H, ALIGN_HTiled_ID   , 0},
    {RESIZE_V, ALIGN_VTiled_ID   , 0},
    {RESIZE_H, ALIGN_HScaled_ID  , 0},
    {RESIZE_V, ALIGN_VScaled_ID  , 0},
    {RESIZE_H_SCALE, ALIGN_HScaled_ID, ALIGN_HTiled_ID},
    {RESIZE_V_SCALE, ALIGN_VScaled_ID, ALIGN_VTiled_ID},
    {FIT_LABEL_SIZE, ALIGN_LabelSize_ID, 0},
    {FIT_LABEL_WIDTH, ALIGN_LabelWidth_ID, 0},
    {FIT_LABEL_HEIGHT, ALIGN_LabelHeight_ID, 0},
    {0, 0, 0}
};


TermDef       BevelTerms[] = {
    {TF_NO_MYNAME_PREPENDING, "None", 4,        TT_FLAG, BEVEL_None_ID     , NULL},
    {TF_NO_MYNAME_PREPENDING, "Left", 4,        TT_FLAG, BEVEL_Left_ID     , NULL},
    {TF_NO_MYNAME_PREPENDING, "Top", 3,         TT_FLAG, BEVEL_Top_ID      , NULL},
    {TF_NO_MYNAME_PREPENDING, "Right", 5,       TT_FLAG, BEVEL_Right_ID    , NULL},
    {TF_NO_MYNAME_PREPENDING, "Bottom", 6,      TT_FLAG, BEVEL_Bottom_ID   , NULL},
    {TF_NO_MYNAME_PREPENDING, "Extra", 5,       TT_FLAG, BEVEL_Extra_ID    , NULL},
    {TF_NO_MYNAME_PREPENDING, "NoOutline", 9,   TT_FLAG, BEVEL_NoOutline_ID, NULL},
    {0, NULL, 0, 0, 0}
};

SyntaxDef     BevelSyntax = {
	',',
	'\n',
    BevelTerms,
	0,										   /* use default hash size */
    ' ',
	" ",
	"\t",
    "Bevel flags",
	"Bevel",
	NULL,
	0
};
struct SyntaxDef     *BevelSyntaxPtr = &BevelSyntax;

flag_options_xref BevelFlagsXref[] = {
    {LEFT_HILITE, BEVEL_Left_ID, BEVEL_None_ID},
    {TOP_HILITE, BEVEL_Top_ID, BEVEL_None_ID},
    {RIGHT_HILITE, BEVEL_Right_ID, BEVEL_None_ID},
    {BOTTOM_HILITE, BEVEL_Bottom_ID, BEVEL_None_ID},
    {EXTRA_HILITE, BEVEL_Extra_ID, BEVEL_None_ID},
    {NO_HILITE_OUTLINE, BEVEL_NoOutline_ID, BEVEL_None_ID},
    {0, 0, 0}
};



TermDef       TbarLayoutTerms[] = {
    {TF_NO_MYNAME_PREPENDING, "Buttons", 7,			TT_FLAG, TBAR_LAYOUT_Buttons_ID     , NULL},
    {TF_NO_MYNAME_PREPENDING, "Spacer", 6,			TT_FLAG, TBAR_LAYOUT_Spacer_ID      , NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleSpacer", 11,	TT_FLAG, TBAR_LAYOUT_TitleSpacer_ID , NULL},
    {0, NULL, 0, 0, 0}
};

SyntaxDef     TbarLayoutSyntax = {
	',',
	'\n',
    TbarLayoutTerms,
	0,										   /* use default hash size */
    ' ',
	" ",
	"\t",
    "Titlebar Layout Flags",
	"TbarLayout",
	NULL,
	0
};
struct SyntaxDef     *TbarLayoutSyntaxPtr = &TbarLayoutSyntax;


TermDef       MyFrameTerms[] = {
    {TF_NO_MYNAME_PREPENDING, "MyFrame", 7,     TT_QUOTED_TEXT, MYFRAME_START_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "North", 5,       TT_OPTIONAL_PATHNAME,    MYFRAME_North_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "South", 5,       TT_OPTIONAL_PATHNAME,    MYFRAME_South_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "East", 4,        TT_OPTIONAL_PATHNAME,    MYFRAME_East_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "West", 4,        TT_OPTIONAL_PATHNAME,    MYFRAME_West_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "NorthWest", 9,   TT_OPTIONAL_PATHNAME,    MYFRAME_NorthWest_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "NorthEast", 9,   TT_OPTIONAL_PATHNAME,    MYFRAME_NorthEast_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "SouthWest", 9,   TT_OPTIONAL_PATHNAME,    MYFRAME_SouthWest_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "SouthEast", 9,   TT_OPTIONAL_PATHNAME,    MYFRAME_SouthEast_ID, NULL},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED, "Side", 4,            TT_OPTIONAL_PATHNAME,    MYFRAME_Side_ID, NULL},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED, "NoSide", 6,          TT_FLAG,    MYFRAME_NoSide_ID, NULL},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED, "Corner", 6,          TT_OPTIONAL_PATHNAME,    MYFRAME_Corner_ID, NULL},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED, "NoCorner", 8,        TT_FLAG,    MYFRAME_NoCorner_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleUnfocusedStyle", 19,TT_QUOTED_TEXT,  MYFRAME_TitleUnfocusedStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleUStyle", 11,        TT_QUOTED_TEXT,  MYFRAME_TitleUnfocusedStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleFocusedStyle", 17,  TT_QUOTED_TEXT,  MYFRAME_TitleFocusedStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleFStyle", 11,        TT_QUOTED_TEXT,  MYFRAME_TitleFocusedStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleStickyStyle", 16,   TT_QUOTED_TEXT,  MYFRAME_TitleStickyStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleSStyle", 11,        TT_QUOTED_TEXT,  MYFRAME_TitleStickyStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "FrameUnfocusedStyle", 19,TT_QUOTED_TEXT,  MYFRAME_FrameUnfocusedStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "FrameUStyle", 11,        TT_QUOTED_TEXT,  MYFRAME_FrameUnfocusedStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "FrameFocusedStyle", 17,  TT_QUOTED_TEXT,  MYFRAME_FrameFocusedStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "FrameFStyle", 11,        TT_QUOTED_TEXT,  MYFRAME_FrameFocusedStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "FrameStickyStyle", 16,   TT_QUOTED_TEXT,  MYFRAME_FrameStickyStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "FrameSStyle", 11,        TT_QUOTED_TEXT,  MYFRAME_FrameStickyStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED, "SideSize", 8,        TT_GEOMETRY,MYFRAME_SideSize_ID, NULL},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "SideAlign", 9,       TT_FLAG,    MYFRAME_SideAlign_ID, &AlignSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "SideBevel", 9,       TT_FLAG,    MYFRAME_SideBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "SideFBevel", 10,       TT_FLAG,    MYFRAME_SideFBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "SideUBevel", 10,       TT_FLAG,    MYFRAME_SideUBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "SideSBevel", 10,       TT_FLAG,    MYFRAME_SideSBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "SideFocusedBevel", 16,      TT_FLAG,    MYFRAME_SideFBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "SideUnfocusedBevel", 18,    TT_FLAG,    MYFRAME_SideUBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "SideStickyBevel", 15,       TT_FLAG,    MYFRAME_SideSBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED, "CornerSize", 10,     TT_GEOMETRY,MYFRAME_CornerSize_ID, NULL},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "CornerAlign", 11,    TT_FLAG,    MYFRAME_CornerAlign_ID, &AlignSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "CornerBevel", 11,    TT_FLAG,    MYFRAME_CornerBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "CornerFBevel", 12,    TT_FLAG,    MYFRAME_CornerFBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "CornerUBevel", 12,    TT_FLAG,    MYFRAME_CornerUBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "CornerSBevel", 12,    TT_FLAG,    MYFRAME_CornerSBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "CornerFocusedBevel", 18,   TT_FLAG,    MYFRAME_CornerFBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "CornerUnfocusedBevel", 20, TT_FLAG,    MYFRAME_CornerUBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING|TF_DIRECTION_INDEXED|TF_NAMED_SUBCONFIG, "CornerStickyBevel", 17,    TT_FLAG,    MYFRAME_CornerSBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING, "TitleBevel", 10,         TT_FLAG,         MYFRAME_TitleBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING, "TitleFBevel", 11,         TT_FLAG,         MYFRAME_TitleFBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING, "TitleUBevel", 11,         TT_FLAG,         MYFRAME_TitleUBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING, "TitleSBevel", 11,         TT_FLAG,         MYFRAME_TitleSBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING, "TitleFocusedBevel", 17,   TT_FLAG,         MYFRAME_TitleFBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING, "TitleUnfocusedBevel", 19, TT_FLAG,         MYFRAME_TitleUBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING, "TitleStickyBevel", 16,    TT_FLAG,         MYFRAME_TitleSBevel_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING, "TitleAlign", 10,          TT_FLAG,         MYFRAME_TitleAlign_ID, &AlignSyntax},
    {TF_NO_MYNAME_PREPENDING, "TitleCompositionMethod", 22, TT_UINTEGER,    MYFRAME_TitleCM_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleFCompositionMethod", 22, TT_UINTEGER,    MYFRAME_TitleFCM_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleUCompositionMethod", 22, TT_UINTEGER,    MYFRAME_TitleUCM_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleSCompositionMethod", 22, TT_UINTEGER,    MYFRAME_TitleSCM_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleFocusedCompositionMethod", 28, TT_UINTEGER,   MYFRAME_TitleFCM_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleUnfocusedCompositionMethod", 30, TT_UINTEGER,  MYFRAME_TitleUCM_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleStickyCompositionMethod", 27, TT_UINTEGER,    MYFRAME_TitleSCM_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "Inherit", 7, TT_QUOTED_TEXT,              MYFRAME_Inherit_ID, NULL},
    {TF_NO_MYNAME_PREPENDING | TF_SYNTAX_TERMINATOR, "~MyFrame", 8, TT_FLAG, MYFRAME_DONE_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "LeftBtnBackground", 17,  TT_FILENAME,     MYFRAME_LeftBtnBackground_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "LeftSpacerBackground", 20,  TT_FILENAME,  MYFRAME_LeftSpacerBackground_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "LeftTitleSpacerBackground", 25,  TT_FILENAME,  MYFRAME_LTitleSpacerBackground_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleBackground", 15,    TT_FILENAME,     MYFRAME_TitleBackground_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "RightTitleSpacerBackground", 26,  TT_FILENAME, MYFRAME_RTitleSpacerBackground_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "RightSpacerBackground", 21,  TT_FILENAME, MYFRAME_RightSpacerBackground_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "RightBtnBackground", 18,  TT_FILENAME,    MYFRAME_RightBtnBackground_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "LeftBtnBackAlign", 16,       TT_FLAG,     MYFRAME_LeftBtnBackAlign_ID, &AlignSyntax},
    {TF_NO_MYNAME_PREPENDING, "LeftSpacerBackAlign", 19,    TT_FLAG,     MYFRAME_LeftSpacerBackAlign_ID, &AlignSyntax},
    {TF_NO_MYNAME_PREPENDING, "LeftTitleSpacerBackAlign", 24,    TT_FLAG,     MYFRAME_LTitleSpacerBackAlign_ID, &AlignSyntax},
	{TF_NO_MYNAME_PREPENDING, "TitleBackgroundAlign", 20,   TT_FLAG,         MYFRAME_TitleBackgroundAlign_ID, &AlignSyntax},
    {TF_NO_MYNAME_PREPENDING, "RightTitleSpacerBackAlign", 25,   TT_FLAG,     MYFRAME_RTitleSpacerBackAlign_ID, &AlignSyntax},
    {TF_NO_MYNAME_PREPENDING, "RightSpacerBackAlign", 20,   TT_FLAG,   MYFRAME_RightSpacerBackAlign_ID, &AlignSyntax},
    {TF_NO_MYNAME_PREPENDING, "RightBtnBackAlign", 17,      TT_FLAG,   MYFRAME_RightBtnBackAlign_ID, &AlignSyntax},
    {TF_NO_MYNAME_PREPENDING, "CondenseTitlebar", 16,       TT_FLAG,   MYFRAME_CondenseTitlebar_ID, &AlignSyntax},
    {TF_NO_MYNAME_PREPENDING, "LeftTitlebarLayout", 18,     TT_FLAG,   MYFRAME_LeftTitlebarLayout_ID, &TbarLayoutSyntax},
    {TF_NO_MYNAME_PREPENDING, "RightTitlebarLayout", 19,    TT_FLAG,   MYFRAME_RightTitlebarLayout_ID, &TbarLayoutSyntax},

	{0, NULL, 0, 0, 0}
};

SyntaxDef     MyFrameSyntax = {
	'\n',
	'\0',
	MyFrameTerms,
    7,                                         /* hash size */
	'\t',
	"",
	"\t",
	"MyFrame definition",
	"MyFrame",
	NULL,
	0
};

TermDef       SupportedHintsTerms[] =
{
    {TF_NO_MYNAME_PREPENDING, "ICCCM", 5, TT_FLAG, HINTS_ICCCM_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "GroupLead", 9, TT_FLAG, HINTS_GroupLead_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "Transient", 9, TT_FLAG, HINTS_Transient_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "Motif", 5, TT_FLAG, HINTS_Motif_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "Gnome", 5, TT_FLAG, HINTS_Gnome_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "ExtendedWM", 10, TT_FLAG, HINTS_ExtendedWM_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "Xresources", 10, TT_FLAG, HINTS_XResources_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "AfterStepDB",11, TT_FLAG, HINTS_ASDatabase_ID, NULL},
    {0, NULL, 0, 0, 0}
};

SyntaxDef     SupportedHintsSyntax = {
    ',',
    '\n',
    SupportedHintsTerms,
    7,                                         /* hash size */
	' ',
	" ",
	"\t",
    "Supported hints list",
	"SupportedHints",
	NULL,
	0
};

extern SyntaxDef     MyBackgroundSyntax ;      /* see ASetRoot.c */

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the base.<bpp> config
 * file
 *
 ****************************************************************************/

TermDef       LookTerms[] = {
/* depreciated options : */
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "Font", 4, TT_FONT, LOOK_Font_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "WindowFont", 10, TT_FONT, LOOK_WindowFont_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "MTitleForeColor", 15, TT_COLOR, LOOK_MTitleForeColor_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "MTitleBackColor", 15, TT_COLOR, LOOK_MTitleBackColor_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "MenuForeColor", 13, TT_COLOR, LOOK_MenuForeColor_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "MenuBackColor", 13, TT_COLOR, LOOK_MenuBackColor_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "MenuHiForeColor", 15, TT_COLOR, LOOK_MenuHiForeColor_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "MenuHiBackColor", 15, TT_COLOR, LOOK_MenuHiBackColor_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "MenuStippleColor", 16, TT_COLOR, LOOK_MenuStippleColor_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "StdForeColor", 12, TT_COLOR, LOOK_StdForeColor_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "StdBackColor", 12, TT_COLOR, LOOK_StdBackColor_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "StickyForeColor", 15, TT_COLOR, LOOK_StickyForeColor_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "StickyBackColor", 15, TT_COLOR, LOOK_StickyBackColor_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "HiForeColor", 11, TT_COLOR, LOOK_HiForeColor_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "HiBackColor", 11, TT_COLOR, LOOK_HiBackColor_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "IconFont", 8, TT_FONT, LOOK_IconFont_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "TextureTypes", 12, TT_INTARRAY, LOOK_TextureTypes_ID, NULL},
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "TextureMaxColors", 16, TT_INTARRAY, LOOK_TextureMaxColors_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "TitleTextureColor", 17, TT_COLOR, LOOK_TitleTextureColor_ID, NULL},                                          /* title */
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "UTitleTextureColor", 18, TT_COLOR, LOOK_UTitleTextureColor_ID, NULL},                                          /* unfoc tit */
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "STitleTextureColor", 18, TT_COLOR, LOOK_STitleTextureColor_ID, NULL},                                          /* stic tit */
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "MTitleTextureColor", 18, TT_COLOR, LOOK_MTitleTextureColor_ID, NULL},                                          /* menu title */
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "MenuTextureColor", 16, TT_COLOR, LOOK_MenuTextureColor_ID, NULL},                                          /* menu items */
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "MenuHiTextureColor", 18, TT_COLOR, LOOK_MenuHiTextureColor_ID, NULL},                                          /* sel items */
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "MenuPixmap", 10, TT_FILENAME, LOOK_MenuPixmap_ID, NULL},                                          /* menu entry */
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "MenuHiPixmap", 12, TT_FILENAME, LOOK_MenuHiPixmap_ID, NULL},                                          /* hil m entr */
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "MTitlePixmap", 12, TT_FILENAME, LOOK_MTitlePixmap_ID, NULL},                                          /* menu title */
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "TitlePixmap", 11, TT_FILENAME, LOOK_TitlePixmap_ID, NULL},                                          /* foc tit */
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "UTitlePixmap", 12, TT_FILENAME, LOOK_UTitlePixmap_ID, NULL},                                          /* unfoc tit */
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "STitlePixmap", 12, TT_FILENAME, LOOK_STitlePixmap_ID, NULL},                                          /* stick tit */

    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "ButtonTextureType", 17, TT_UINTEGER, LOOK_ButtonTextureType_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "ButtonBgColor", 13, TT_COLOR, LOOK_ButtonBgColor_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "ButtonTextureColor", 18, TT_COLOR, LOOK_ButtonTextureColor_ID, NULL},
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "ButtonMaxColors", 15, TT_UINTEGER, LOOK_ButtonMaxColors_ID, NULL},
    {TF_DEPRECIATED|TF_NO_MYNAME_PREPENDING, "ButtonPixmap", 12, TT_FILENAME, LOOK_ButtonPixmap_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleTextMode", 13, TT_UINTEGER, LOOK_TitleTextMode_ID, NULL},

/* non depreciated options : */
    {TF_NO_MYNAME_PREPENDING, "IconBox", 7, TT_BOX, LOOK_IconBox_ID, NULL},
	/* including MyStyles definitions processing */
	INCLUDE_MYSTYLE,

    {TF_NO_MYNAME_PREPENDING, "MArrowPixmap", 12, TT_FILENAME, LOOK_MArrowPixmap_ID, NULL},                                          /* menu arrow */
    {TF_NO_MYNAME_PREPENDING, "MenuPinOn", 9, TT_FILENAME, LOOK_MenuPinOn_ID, NULL},                                          /* menu pin */
    {TF_NO_MYNAME_PREPENDING, "MenuPinOff", 10, TT_FILENAME, LOOK_MenuPinOff_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TextureMenuItemsIndividually", 28, TT_FLAG, LOOK_TxtrMenuItmInd_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "MenuMiniPixmaps", 15, TT_FLAG, LOOK_MenuMiniPixmaps_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TextGradientColor", 17, TT_CUSTOM, LOOK_TextGradientColor_ID, NULL},                                          /* title text */
    {TF_NO_MYNAME_PREPENDING, "GradientText", 12, TT_FLAG, LOOK_GradientText_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitlebarNoPush", 14, TT_FLAG, LOOK_TitlebarNoPush_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "ButtonNoBorder", 14, TT_FLAG, LOOK_ButtonNoBorder_ID, NULL},
/* TitleButtonBalloons */
    {TF_NO_MYNAME_PREPENDING, "TitleButtonBalloons", 19, TT_FLAG, BALLOON_USED_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleButtonBalloonBorderHilite", 29, TT_FLAG, BALLOON_BorderHilite_ID, &BevelSyntax},
    {TF_NO_MYNAME_PREPENDING, "TitleButtonBalloonXOffset", 25, TT_INTEGER, BALLOON_XOffset_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleButtonBalloonYOffset", 25, TT_INTEGER, BALLOON_YOffset_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleButtonBalloonDelay", 23, TT_UINTEGER, BALLOON_Delay_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleButtonBalloonCloseDelay", 28, TT_UINTEGER, BALLOON_CloseDelay_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleButtonBalloonStyle", 23, TT_QUOTED_TEXT, BALLOON_Style_ID, NULL},

    {TF_NO_MYNAME_PREPENDING, "TitleTextAlign", 14, TT_UINTEGER, LOOK_TitleTextAlign_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleButtonSpacing", 18, TT_INTEGER, LOOK_TitleButtonSpacing_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleButtonStyle", 16, TT_UINTEGER, LOOK_TitleButtonStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleButtonXOffset", 18, TT_INTEGER, LOOK_TitleButtonXOffset_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TitleButtonYOffset", 18, TT_INTEGER, LOOK_TitleButtonYOffset_ID, NULL},
    {TF_INDEXED|TF_NO_MYNAME_PREPENDING, "TitleButton", 11, TT_BUTTON, LOOK_TitleButton_ID, NULL},
    {TF_INDEXED|TF_NO_MYNAME_PREPENDING, "IconTitleButton", 15, TT_BUTTON, LOOK_IconTitleButton_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "ResizeMoveGeometry", 18, TT_GEOMETRY, LOOK_ResizeMoveGeometry_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "StartMenuSortMode", 17, TT_UINTEGER, LOOK_StartMenuSortMode_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "DrawMenuBorders", 15, TT_UINTEGER, LOOK_DrawMenuBorders_ID, NULL},

    {TF_NO_MYNAME_PREPENDING, "ButtonSize", 10, TT_BOX, LOOK_ButtonSize_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "SeparateButtonTitle", 19, TT_FLAG, LOOK_SeparateButtonTitle_ID, NULL},

    {TF_NO_MYNAME_PREPENDING, "RubberBand", 10, TT_UINTEGER, LOOK_RubberBand_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "ShadeAnimationSteps", 19, TT_UINTEGER, LOOK_ShadeAnimationSteps_ID, NULL},

    {TF_NO_MYNAME_PREPENDING, "DefaultStyle", 12, TT_QUOTED_TEXT, LOOK_DefaultStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "FWindowStyle", 12, TT_QUOTED_TEXT, LOOK_FWindowStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "UWindowStyle", 12, TT_QUOTED_TEXT, LOOK_UWindowStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "SWindowStyle", 12, TT_QUOTED_TEXT, LOOK_SWindowStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "MenuItemStyle", 13, TT_QUOTED_TEXT, LOOK_MenuItemStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "MenuTitleStyle", 14, TT_QUOTED_TEXT, LOOK_MenuTitleStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "MenuHiliteStyle", 15, TT_QUOTED_TEXT, LOOK_MenuHiliteStyle_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "MenuStippleStyle", 16, TT_QUOTED_TEXT, LOOK_MenuStippleStyle_ID, NULL},

	INCLUDE_MYFRAME,

    {TF_NO_MYNAME_PREPENDING, "MenuFrame", 16, TT_QUOTED_TEXT, LOOK_MenuFrame_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "SupportedHints", 14, TT_FLAG, LOOK_SupportedHints_ID, &SupportedHintsSyntax},

	/* see ASetRoot.c : */
    {TF_NO_MYNAME_PREPENDING, "MyBackground", 12, TT_QUOTED_TEXT,BGR_MYBACKGROUND, &MyBackgroundSyntax},
	{TF_NO_MYNAME_PREPENDING|TF_INDEXED|TF_DONT_SPLIT, "DeskBack", 8, TT_QUOTED_TEXT, LOOK_DeskBack_ID, NULL},
	{TF_NO_MYNAME_PREPENDING|TF_INDEXED|TF_DONT_SPLIT, "DeskLayout", 8, TT_QUOTED_TEXT, LOOK_DeskLayout_ID, NULL},

	{0, NULL, 0, 0, 0}
};

SyntaxDef     LookSyntax = {
	'\n',
	'\0',
	LookTerms,
	0,										   /* use default hash size */
    '\t',
	"",
	"\t",
	"Look configuration",
	"Look",
	NULL,
	0
};

flag_options_xref LookFlagsXref[] = {
    {TxtrMenuItmInd, LOOK_TxtrMenuItmInd_ID, 0},
    {MenuMiniPixmaps, LOOK_MenuMiniPixmaps_ID, 0},
    {GradientText, LOOK_GradientText_ID, 0},
    {IconNoBorder, LOOK_ButtonNoBorder_ID, 0},
    {SeparateButtonTitle, LOOK_SeparateButtonTitle_ID, 0},
	{0, 0, 0}

};

/********************************************************************/
/* First MyFrame processing code				    */
/********************************************************************/
MyFrameDefinition *
AddMyFrameDefinition (MyFrameDefinition ** tail)
{
    MyFrameDefinition *new_def = safecalloc (1, sizeof (MyFrameDefinition));
	*tail = new_def;
	return new_def;
}

void
DestroyMyFrameDefinitions (MyFrameDefinition ** list)
{
	if (*list)
	{
		register int  i;
        MyFrameDefinition *fd = *list;

        DestroyMyFrameDefinitions (&(fd->next));
        if (fd->name)
            free (fd->name);
		for (i = 0; i < FRAME_PARTS; i++)
            if (fd->parts[i])
                free (fd->parts[i]);

        for( i = 0 ; i < BACK_STYLES ; ++i )
        {
            if( fd->title_styles[i] )
                free(fd->title_styles[i]);
            if( fd->frame_styles[i] )
                free(fd->frame_styles[i]);
        }
		for( i = 0 ; i < MYFRAME_TITLE_BACKS ; ++i )
		{
        	if( fd->title_backs[i] )
            	free( fd->title_backs[i] );
		}
        if( fd->inheritance_list )
        {
            i = fd->inheritance_num-1 ;
            while( --i >= 0 )
                if( fd->inheritance_list[i] )
                    free( fd->inheritance_list[i] );
            free( fd->inheritance_list );
        }
        free (fd);
        *list = NULL;
	}
}

void
PrintMyFrameDefinitions (MyFrameDefinition * list, int index)
{
	if (list)
	{
		register int  i;

        if (list->name)
            fprintf (stderr, "MyFrame[%d].name = \"%s\";\n", index, list->name);
        fprintf (stderr, "MyFrame[%d].set_parts = 0x%lX;\n", index, list->set_parts);
        fprintf (stderr, "MyFrame[%d].parts_mask = 0x%lX;\n", index, list->parts_mask);
        for (i = 0; i < FRAME_PARTS; i++)
			if (list->parts[i])
                fprintf (stderr, "MyFrame[%d].Side[%d] = \"%s\";\n", index, i, list->parts[i]);
        for( i = 0 ; i < BACK_STYLES ; ++i )
            if( list->title_styles[i])
                fprintf (stderr, "MyFrame[%d].TitleStyle[%d] = \"%s\";\n", index, i, list->title_styles[i]);
        for( i = 0 ; i < BACK_STYLES ; ++i )
            if( list->frame_styles[i])
                fprintf (stderr, "MyFrame[%d].FrameStyle[%d] = \"%s\";\n", index, i, list->frame_styles[i]);
		for( i = 0 ; i < MYFRAME_TITLE_BACKS ; ++i )
		{
	        if( list->title_backs[i] )
    	        fprintf (stderr, "MyFrame[%d].TitleBack[%d] = \"%s\";\n", index, i, list->title_backs[i]);
		}
        fprintf (stderr, "MyFrame[%d].set_part_size = 0x%lX;\n", index, list->set_part_size);
        fprintf (stderr, "MyFrame[%d].set_part_bevel = 0x%lX;\n", index, list->set_part_bevel);
        fprintf (stderr, "MyFrame[%d].set_part_align = 0x%lX;\n", index, list->set_part_align);
        for( i = 0 ; i < FRAME_PARTS ; ++i )
        {
            fprintf (stderr, "MyFrame[%d].Part[%d].width = %d;\n", index, i, list->part_width[i]);
            fprintf (stderr, "MyFrame[%d].Part[%d].length = %d;\n", index, i, list->part_length[i]);
            fprintf (stderr, "MyFrame[%d].Part[%d].fbevel = 0x%lX;\n", index, i, list->part_fbevel[i]);
            fprintf (stderr, "MyFrame[%d].Part[%d].ubevel = 0x%lX;\n", index, i, list->part_ubevel[i]);
            fprintf (stderr, "MyFrame[%d].Part[%d].sbevel = 0x%lX;\n", index, i, list->part_sbevel[i]);
            fprintf (stderr, "MyFrame[%d].Part[%d].align = 0x%lX;\n", index, i, list->part_align[i]);
        }
        fprintf (stderr, "MyFrame[%d].set_title_attr = 0x%lX;\n", index, list->set_title_attr);
        fprintf (stderr, "MyFrame[%d].title_fbevel = 0x%lX;\n", index, list->title_fbevel);
        fprintf (stderr, "MyFrame[%d].title_ubevel = 0x%lX;\n", index, list->title_ubevel);
        fprintf (stderr, "MyFrame[%d].title_sbevel = 0x%lX;\n", index, list->title_sbevel);
        fprintf (stderr, "MyFrame[%d].title_align = 0x%lX;\n", index, list->title_align);
    	for( i = 0 ; i < MYFRAME_TITLE_BACKS ; ++i )
		{
	       fprintf (stderr, "MyFrame[%d].title_backs_align[%d] = 0x%lX;\n", index, i, list->title_backs_align[i]);
		}
        fprintf (stderr, "MyFrame[%d].title_fcm = %d;\n", index, list->title_fcm);
        fprintf (stderr, "MyFrame[%d].title_ucm = %d;\n", index, list->title_ucm);
        fprintf (stderr, "MyFrame[%d].title_scm = %d;\n", index, list->title_scm);
        if( list->inheritance_list )
            for( i = 0 ; i < list->inheritance_num ; ++i )
                fprintf (stderr, "MyFrame[%d].Inherit[%d] = \"%s\";\n", index, i, list->inheritance_list[i]);
        PrintMyFrameDefinitions (list->next, index+1);
    }
}

ASFlagType
ParseBevelOptions( FreeStorageElem * options )
{
    ASFlagType bevel = 0 ;
    while( options )
	{
        LOCAL_DEBUG_OUT( "options(%p)->keyword(\"%s\")", options, options->term->keyword );
        if (options->term != NULL)
            ReadFlagItem (NULL, &bevel, options, BevelFlagsXref);
        options = options->next;
    }
    return bevel;
}

void
bevel_parse(char *tline, FILE * fd, char **myname, int *pbevel)
{
    FilePtrAndData fpd ;
    ConfigDef    *ConfigReader ;
    FreeStorageElem *Storage = NULL, *more_stuff = NULL;
	ConfigData cd ;
    if( pbevel == NULL )
        return;

    fpd.fp = fd ;
    fpd.data = safemalloc( strlen(tline)+1+1 ) ;
    sprintf( fpd.data, "%s\n", tline );
LOCAL_DEBUG_OUT( "fd(%p)->tline(\"%s\")->fpd.data(\"%s\")", fd, tline, fpd.data );
	cd.fileptranddata = &fpd ;
    ConfigReader = InitConfigReader ((char*)myname, &BevelSyntax, CDT_FilePtrAndData, cd, NULL);
    free( fpd.data );

    if (!ConfigReader)
        return ;

	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &Storage);
	PrintFreeStorage (Storage);

	/* getting rid of all the crap first */
    StorageCleanUp (&Storage, &more_stuff, CF_DISABLED_OPTION);
    DestroyFreeStorage (&more_stuff);

    *pbevel = ParseBevelOptions(Storage);

	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);
}


ASFlagType
ParseAlignOptions( FreeStorageElem * options )
{
    ASFlagType align = 0 ;
    while( options )
	{
        LOCAL_DEBUG_OUT( "options(%p)->keyword(\"%s\")", options, options->term->keyword );
        if (options->term != NULL)
		{
			switch( options->term->id )
			{
				case ALIGN_Center_ID :
					set_flags( align, ALIGN_CENTER );
				    break ;
				case ALIGN_HCenter_ID :
					set_flags( align, ALIGN_HCENTER );
				    break ;
				case ALIGN_VCenter_ID :
					set_flags( align, ALIGN_VCENTER );
				    break ;
				default:
            		ReadFlagItem (NULL, &align, options, AlignFlagsXref);
			}
		}
        options = options->next;
    }
    return align;
}

unsigned long
ParseTbarLayoutOptions( FreeStorageElem * options )
{
    unsigned long layout = 0 ;
	int count = 0 ; 
    while( options )
	{
        LOCAL_DEBUG_OUT( "options(%p)->keyword(\"%s\")", options, options->term->keyword );
        if (options->term != NULL)
		{	
			int elem = options->term->id - TBAR_LAYOUT_ID_START ;
			int i ;
			if( elem >= 0 && elem <= MYFRAME_TITLE_SIDE_ELEMS ) 
			{
				for( i = 0 ; i < count ; ++i ) 
					if( MYFRAME_GetTbarLayoutElem(layout,i) == elem ) 
						break;
				if( i >= count ) 
				{
					MYFRAME_SetTbarLayoutElem(layout,count,elem);
					++count ;
				}	 
			}	 
        }
		options = options->next;
    }
	while( count < MYFRAME_TITLE_SIDE_ELEMS ) 
	{
		MYFRAME_SetTbarLayoutElem(layout,count,MYFRAME_TITLE_BACK_INVALID);
		++count ;			
	}	 
    return layout;
}

void
tbar_layout_parse(char *tline, FILE * fd, char **myname, int *playout)
{
    FilePtrAndData fpd ;
    ConfigDef    *ConfigReader ;
    FreeStorageElem *Storage = NULL, *more_stuff = NULL;
	ConfigData cd ;
    if( playout == NULL )
        return;

    fpd.fp = fd ;
    fpd.data = safemalloc( strlen(tline)+1+1 ) ;
    sprintf( fpd.data, "%s\n", tline );
LOCAL_DEBUG_OUT( "fd(%p)->tline(\"%s\")->fpd.data(\"%s\")", fd, tline, fpd.data );
	cd.fileptranddata = &fpd ;
    ConfigReader = InitConfigReader ((char*)myname, &BevelSyntax, CDT_FilePtrAndData, cd, NULL);
    free( fpd.data );

    if (!ConfigReader)
        return ;

	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &Storage);
	PrintFreeStorage (Storage);

	/* getting rid of all the crap first */
    StorageCleanUp (&Storage, &more_stuff, CF_DISABLED_OPTION);
    DestroyFreeStorage (&more_stuff);

    *playout = ParseTbarLayoutOptions(Storage);

	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);
}


MyFrameDefinition **
ProcessMyFrameOptions (FreeStorageElem * options, MyFrameDefinition ** tail)
{
	ConfigItem    item;
	int           rel_id;
    MyFrameDefinition *fd = NULL ;

	item.memory = NULL;

	for (; options; options = options->next)
	{
		if (options->term == NULL)
			continue;
        LOCAL_DEBUG_OUT( "MyFrame(%p)->options(%p)->keyword(\"%s\")", fd, options, options->term->keyword );
		if (options->term->id < MYFRAME_ID_START || options->term->id > MYFRAME_ID_END)
			continue;

		if (options->term->type == TT_FLAG)
		{
            if( fd != NULL  )
            {
                if( options->term->id == MYFRAME_DONE_ID )
                    break;                     /* for loop */
				if( options->term->id >= MYFRAME_TitleBackgroundAlign_ID_START )
				{
					int index = options->term->id - MYFRAME_TitleBackgroundAlign_ID_START ;
					if( index < MYFRAME_TITLE_BACKS )
					{
						fd->title_backs_align[index] = ParseAlignOptions( options->sub );
                        set_flags( fd->set_title_attr, MYFRAME_TitleBackAlignSet_Start<<index );
						continue;
					}
				}
                switch( options->term->id )
                {
                    case MYFRAME_TitleBevel_ID :
                        fd->title_fbevel = ParseBevelOptions( options->sub );
                        fd->title_ubevel = fd->title_fbevel ;
                        fd->title_sbevel = fd->title_fbevel ;
                        set_flags( fd->set_title_attr, MYFRAME_TitleBevelSet );
                        break;
                    case MYFRAME_TitleFBevel_ID :
                        fd->title_fbevel = ParseBevelOptions( options->sub );
                        set_flags( fd->set_title_attr, MYFRAME_TitleBevelSet );
                        break;
                    case MYFRAME_TitleUBevel_ID :
                        fd->title_ubevel = ParseBevelOptions( options->sub );
                        set_flags( fd->set_title_attr, MYFRAME_TitleBevelSet );
                        break;
                    case MYFRAME_TitleSBevel_ID :
                        fd->title_sbevel = ParseBevelOptions( options->sub );
                        set_flags( fd->set_title_attr, MYFRAME_TitleBevelSet );
                        break;
                    case MYFRAME_TitleAlign_ID :
                        fd->title_align = ParseAlignOptions( options->sub );
                        set_flags( fd->set_title_attr, MYFRAME_TitleAlignSet );
                        break;
                    case MYFRAME_CondenseTitlebar_ID :
                        fd->condense_titlebar = ParseAlignOptions( options->sub );
                        set_flags( fd->set_title_attr, MYFRAME_CondenseTitlebarSet );
                        break;
                    case MYFRAME_LeftTitlebarLayout_ID :
                        fd->left_layout = ParseTbarLayoutOptions( options->sub );
                        set_flags( fd->set_title_attr, MYFRAME_LeftTitlebarLayoutSet );
                        break;
                    case MYFRAME_RightTitlebarLayout_ID :
                        fd->right_layout = ParseTbarLayoutOptions( options->sub );
                        set_flags( fd->set_title_attr, MYFRAME_RightTitlebarLayoutSet );
                        break;
                    default:
                        if (!ReadConfigItem (&item, options))
                            continue;
                        LOCAL_DEBUG_OUT( "item.index = %d", item.index );
                        if( item.index < 0 || item.index >= FRAME_PARTS )
                        {
                            item.ok_to_free = 1;
                            continue;
                        }
                        switch( options->term->id )
                        {
                            case MYFRAME_NoSide_ID :
                            case MYFRAME_NoCorner_ID :
                                clear_flags( fd->parts_mask, 0x01<<item.index );
                                set_flags( fd->set_parts, 0x01<<item.index );
                                break;
                            case MYFRAME_SideAlign_ID :
                            case MYFRAME_CornerAlign_ID :
                                fd->part_align[item.index] = ParseAlignOptions( options->sub );
                                set_flags( fd->set_part_align, 0x01<<item.index );
                                break;
                            case MYFRAME_SideBevel_ID :
                            case MYFRAME_CornerBevel_ID :
                                fd->part_fbevel[item.index] = ParseBevelOptions( options->sub );
                                fd->part_ubevel[item.index] = fd->part_fbevel[item.index];
                                fd->part_sbevel[item.index] = fd->part_fbevel[item.index];
                                SetPartFBevelSet(fd,item.index );
                                SetPartUBevelSet(fd,item.index );
                                SetPartSBevelSet(fd,item.index );
                                break;
                            case MYFRAME_SideFBevel_ID :
                            case MYFRAME_CornerFBevel_ID :
                                fd->part_fbevel[item.index] = ParseBevelOptions( options->sub );
                                SetPartFBevelSet(fd,item.index );
                                break;
                            case MYFRAME_SideUBevel_ID :
                            case MYFRAME_CornerUBevel_ID :
                                fd->part_ubevel[item.index] = ParseBevelOptions( options->sub );
                                SetPartUBevelSet(fd,item.index );
                                break;
                            case MYFRAME_SideSBevel_ID :
                            case MYFRAME_CornerSBevel_ID :
                                fd->part_sbevel[item.index] = ParseBevelOptions( options->sub );
                                SetPartSBevelSet(fd,item.index );
                                break;
                            default:
                                show_warning( "Unexpected MyFrame definition keyword \"%s\" . Ignoring.", options->term->keyword );
                                item.ok_to_free = 1;
                        }
                }
            }
            continue;
        }

		if (!ReadConfigItem (&item, options))
        {
            LOCAL_DEBUG_OUT( "ReadConfigItem has failed%s","");
			continue;
        }
		if (*tail == NULL)
            fd = AddMyFrameDefinition (tail);

        rel_id = options->term->id - MYFRAME_START_ID;
        if (rel_id == 0)
		{
            if (fd->name != NULL)
			{
				show_error("MyFrame \"%s\": Previous MyFrame definition [%s] was not terminated correctly, and will be ignored.",
                           item.data.string, fd->name);
				DestroyMyFrameDefinitions (tail);
                fd = AddMyFrameDefinition (tail);
			}
            set_string_value (&(fd->name), item.data.string, NULL, 0);
        } else if( rel_id <= FRAME_PARTS )
		{
            --rel_id ;
            set_string_value (&(fd->parts[rel_id]), item.data.string, &(fd->set_parts), (0x01<<rel_id));
            set_flags( fd->parts_mask, (0x01<<rel_id));
        }else
        {
            rel_id = item.index ;
            LOCAL_DEBUG_OUT( "item.index = %d", item.index );
            if( rel_id < 0 || rel_id >= FRAME_PARTS )
                rel_id = 0 ;
            switch( options->term->id )
            {
                case MYFRAME_Side_ID :
                case MYFRAME_Corner_ID :
                    set_string_value (&(fd->parts[rel_id]), item.data.string, &(fd->set_parts), (0x01<<rel_id));
                    set_flags( fd->parts_mask, (0x01<<rel_id));
                    break;
                case MYFRAME_TitleUnfocusedStyle_ID :
                    set_string_value (&(fd->title_styles[BACK_UNFOCUSED]), item.data.string, NULL, 0);
                    break;
                case MYFRAME_TitleFocusedStyle_ID :
                    set_string_value (&(fd->title_styles[BACK_FOCUSED]), item.data.string, NULL, 0);
                    break;
                case MYFRAME_TitleStickyStyle_ID :
                    set_string_value (&(fd->title_styles[BACK_STICKY]), item.data.string, NULL, 0);
                    break;
                case MYFRAME_FrameUnfocusedStyle_ID :
                    set_string_value (&(fd->frame_styles[BACK_UNFOCUSED]), item.data.string, NULL, 0);
                    break;
                case MYFRAME_FrameFocusedStyle_ID :
                    set_string_value (&(fd->frame_styles[BACK_FOCUSED]), item.data.string, NULL, 0);
                    break;
                case MYFRAME_FrameStickyStyle_ID :
                    set_string_value (&(fd->frame_styles[BACK_STICKY]), item.data.string, NULL, 0);
                    break;
                case MYFRAME_SideSize_ID :
                case MYFRAME_CornerSize_ID :
                    if( get_flags( item.data.geometry.flags, WidthValue) )
                        fd->part_width[rel_id] = item.data.geometry.width ;
                    else
                        fd->part_width[rel_id] = 0 ;
                    if( get_flags( item.data.geometry.flags, HeightValue) )
                        fd->part_length[rel_id] = item.data.geometry.height ;
                    else
                        fd->part_length[rel_id] = 0 ;
                    set_flags( fd->set_part_size, (0x01<<rel_id));
                    break;
                case MYFRAME_TitleCM_ID :
                    fd->title_fcm = item.data.integer;
                    fd->title_ucm = item.data.integer;
                    fd->title_scm = item.data.integer;
                    set_flags( fd->set_title_attr, MYFRAME_TitleCMSet );
                    break;
                case MYFRAME_TitleFCM_ID :
                    fd->title_fcm = item.data.integer;
                    set_flags( fd->set_title_attr, MYFRAME_TitleFCMSet );
                    break;
                case MYFRAME_TitleUCM_ID :
                    fd->title_ucm = item.data.integer;
                    set_flags( fd->set_title_attr, MYFRAME_TitleUCMSet );
                    break;
                case MYFRAME_TitleSCM_ID :
                    fd->title_scm = item.data.integer;
                    set_flags( fd->set_title_attr, MYFRAME_TitleSCMSet );
                    break;

                case MYFRAME_Inherit_ID :
                    {
                        fd->inheritance_list = realloc( fd->inheritance_list, (fd->inheritance_num+1)*sizeof(char*));
                        fd->inheritance_list[fd->inheritance_num] = item.data.string ;
                        ++(fd->inheritance_num);
                    }
                    break;
                default:
					{
						int index = options->term->id - MYFRAME_TitleBackground_ID_START ;
						if( index >= 0 && index < MYFRAME_TITLE_BACKS )
						{
		                    set_string_value (&(fd->title_backs[index]), item.data.string, NULL, 0);
						}else
						{
							item.ok_to_free = 1;
        		            show_warning( "Unexpected MyFrame definition keyword \"%s\" . Ignoring.", options->term->keyword );
						}
					}
            }
        }
    }
    if (fd != NULL )
    {
        if( fd->name == NULL)
        {
            show_warning( "MyFrame with no name encoutered. Discarding." );
            DestroyMyFrameDefinitions (tail);
        }else
        {
            LOCAL_DEBUG_OUT("done parsing myframe: [%s]", fd->name);
            while (*tail)
                tail = &((*tail)->next);    /* just in case */
        }
    }
    ReadConfigItem (&item, NULL);
	return tail;
}

static FreeStorageElem **
MyFrameDef2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, MyFrameDefinition * def)
{
	register int  i;
	FreeStorageElem **new_tail;

	if (def == NULL)
		return tail;

	new_tail = QuotedString2FreeStorage (syntax, tail, NULL, def->name, MYFRAME_START_ID);
	if (new_tail == tail)
		return tail;

    tail = &((*tail)->sub);

	for (i = 0; i < FRAME_PARTS; i++)
		tail = Path2FreeStorage (&MyFrameSyntax, tail, NULL, def->parts[i], MYFRAME_START_ID + i + 1);

	tail = Flag2FreeStorage (&MyFrameSyntax, tail, MYFRAME_DONE_ID);
    /* TODO: implement the rest of the MyFrame writing : */

	return new_tail;
}

FreeStorageElem **
MyFrameDefs2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, MyFrameDefinition * defs)
{
	while (defs)
	{
		tail = MyFrameDef2FreeStorage (syntax, tail, defs);
		defs = defs->next;
	}
	return tail;
}

void
myframe_parse (char *tline, FILE * fd, char **myname, int *myframe_list)
{
    FilePtrAndData fpd ;
    ConfigData cd ;
	ConfigDef    *ConfigReader ;
    FreeStorageElem *Storage = NULL, *more_stuff = NULL;
    MyFrameDefinition **list = (MyFrameDefinition**)myframe_list, **tail ;

    if( list == NULL )
        return;
    for( tail = list ; *tail != NULL ; tail = &((*tail)->next) );

    fpd.fp = fd ;
    fpd.data = safemalloc( 12+1+strlen(tline)+1+1 ) ;
    sprintf( fpd.data, "MyFrame %s\n", tline );
LOCAL_DEBUG_OUT( "fd(%p)->tline(\"%s\")->fpd.data(\"%s\")", fd, tline, fpd.data );
    cd.fileptranddata = &fpd ;
	ConfigReader = InitConfigReader ((char*)myname, &MyFrameSyntax, CDT_FilePtrAndData, cd, NULL);
    free( fpd.data );

    if (!ConfigReader)
        return ;

	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &Storage);
	PrintFreeStorage (Storage);

	/* getting rid of all the crap first */
    StorageCleanUp (&Storage, &more_stuff, CF_DISABLED_OPTION);
    DestroyFreeStorage (&more_stuff);

    ProcessMyFrameOptions (Storage, tail);

	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);
}


/**********************************************************************/
/* The following code has been backported from as-devel and remains
 * undebugged for future use. Feel free to debug it :
 *                                                      Sasha Vasko.
 **********************************************************************/


/*****************  Create/Destroy DeskBackConfig     *****************/
DesktopConfig *
GetDesktopConfig (DesktopConfig **plist, int desk)
{

	register DesktopConfig *curr = *plist ;
	while( curr )
		if( curr->desk == desk )
			return curr ;
		else
			curr = curr->next ;

	curr = (DesktopConfig *) safecalloc (1, sizeof (DesktopConfig));

	curr->desk = desk;
	curr->next = *plist;
	*plist = curr ;

	return curr;
}

void
PrintDesktopConfig (DesktopConfig * head)
{
	DesktopConfig *cur;

	for (cur = head; cur; cur = cur->next)
		fprintf ( stderr, "Desk: %d, back[%s], layout[%s]\n",
		          cur->desk, cur->back_name, cur->layout_name );
}

void
DestroyDesktopConfig (DesktopConfig ** head)
{
	if (head)
	{
		DesktopConfig *cur = *head, *next;

		while (cur)
		{
			if (cur->back_name)
				free (cur->back_name);
			if (cur->layout_name)
				free (cur->layout_name);

			next = cur->next;
			free (cur);
			cur = next;
		}
		*head = NULL;
	}
}

DesktopConfig *
ParseDesktopOptions (DesktopConfig **plist, ConfigItem * item, int id )
{
	DesktopConfig *config = GetDesktopConfig (plist, item->index);

	item->ok_to_free = 0;
	switch( id )
	{
		case DESK_CONFIG_BACK :
			set_string_value (&(config->back_name), item->data.string, NULL, 0);
			break;
		case DESK_CONFIG_LAYOUT :
			set_string_value (&(config->layout_name), item->data.string, NULL, 0);
			break;
		default:
			item->ok_to_free = 1;
	}
	return config;
}


/********************************************************************/
/* Supported Hints processing code                                  */
/********************************************************************/
ASSupportedHints *ProcessSupportedHints(FreeStorageElem *options)
{
    ASSupportedHints *list = NULL ;
    if( options )
    {
        list = create_hints_list();
        while( options )
        {
            enable_hints_support( list, options->term->id - HINTS_ID_START );
            options = options->next ;
        }
    }
    return list;
}

static FreeStorageElem **
SupportedHints2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, ASSupportedHints *list)
{
	register int  i;
	FreeStorageElem **new_tail;
    HintsTypes *hints ;
    int hints_num ;

    if (list == NULL)
		return tail;

    new_tail = Flag2FreeStorage (syntax, tail, LOOK_SupportedHints_ID);
	if (new_tail == tail)
		return tail;

	/* we must not free hints after the following !!!!!!!!!! */
    if( (hints = supported_hints_types( list, &hints_num )) != NULL )
    {
        tail = &((*tail)->sub);
        for (i = 0; i < hints_num; i++)
            tail = Flag2FreeStorage (&SupportedHintsSyntax, tail, HINTS_ID_START + hints[i]);
    }
	return new_tail;
}

/********************************************************************/
/* Now look processing code 					    */
/********************************************************************/

LookConfig   *
CreateLookConfig ()
{
    LookConfig   *config = (LookConfig *) safecalloc (1, sizeof (LookConfig));
	return config;
}

void
DestroyLookConfig (LookConfig * config)
{
	register int  i;

	if (config->menu_arrow != NULL)
		free (config->menu_arrow);			   /* menu arrow */
	if (config->menu_pin_on != NULL)
		free (config->menu_pin_on);			   /* menu pin */
	if (config->menu_pin_off != NULL)
		free (config->menu_pin_off);

	for (i = 0; i < 2; i++)
		if (config->text_gradient[i] != NULL)
			free (config->text_gradient[i]);   /* title text */
	for (i = 0; i < MAX_BUTTONS; i++)
    {
        if (config->normal_buttons[i] != NULL)
		{
            destroy_asbutton (config->normal_buttons[i], False);
			config->normal_buttons[i] = NULL ;
        }
		if (config->icon_buttons[i] != NULL)
		{
            destroy_asbutton (config->icon_buttons[i], False);
			config->icon_buttons[i] = NULL ;
		}
    }
    for (i = 0; i < BACK_STYLES; i++)
		if (config->window_styles[i] != NULL)
			free (config->window_styles[i]);

	for (i = 0; i < MENU_BACK_STYLES; i++)
		if (config->menu_styles[i] != NULL)
			free (config->menu_styles[i]);

	Destroy_balloonConfig (config->balloon_conf);
	DestroyMyStyleDefinitions (&(config->style_defs));
	DestroyMyFrameDefinitions (&(config->frame_defs));
	DestroyFreeStorage (&(config->more_stuff));
    DestroyMyBackgroundConfig(&(config->back_defs));
	DestroyDesktopConfig(&(config->desk_configs));


    if( config->menu_frame )
        free(config->menu_frame);

    if( config->supported_hints )
        destroy_hints_list( &(config->supported_hints));

	free (config);
}


void
spread_back_param_array (int *trg, int *src, int count)
{
	register int  i = 0, k;

	if (trg == NULL || src == NULL || count == 0)
		return;
	for (k = 0; i < count && i < 3 && k < BACK_STYLES; i++)
		trg[k++] = src[i];
	for (k = BACK_STYLES; i < count && k < BACK_STYLES + MENU_BACK_STYLES; i++)
		trg[k++] = src[i];
}

void
merge_depreciated_options (LookConfig * config, FreeStorageElem * Storage)
{
	FreeStorageElem *pCurr;
	MyStyleDefinition **mystyles_list = &(config->style_defs);
	ConfigItem    item;
	register int  i;
    char         *button_title_styles[BACK_STYLES] = {  ICON_STYLE_FOCUS,
                                                        ICON_STYLE_STICKY,
                                                        ICON_STYLE_UNFOCUS,
                                                        NULL
                                                     };
	char         *ButtonColors[2] = { NULL, NULL };
	char         *ButtonTextureColors[2] = { NULL, NULL };
	char         *ButtonPixmap = NULL;
	int           ButtonTextureType = -1;


#define SET_WINDOW		0
#define SET_MENU		1
#define SET_ICON		2
#define SET_NUM			3
	char         *Fonts[SET_NUM];

	char         *MenuColors[MENU_BACK_STYLES][2];
	char         *MenuTextureColors[MENU_BACK_STYLES][2];
	char         *MenuPixmaps[MENU_BACK_STYLES];

	char         *WinColors[BACK_STYLES][2];
	char         *WinTextureColors[BACK_STYLES][2];
	char         *WinPixmaps[BACK_STYLES];

	int           TitleTextStyle = -1;

	int           TextureTypes[BACK_STYLES + MENU_BACK_STYLES];
	int          *array = NULL;


#define CHANGED_WINDOW		(0x01<<0)
#define CHANGED_MENU		(0x01<<1)
#define CHANGED_ICON		(0x01<<2)
	unsigned long change_flags = 0;

	/* start initialization */
	memset (&(Fonts[0]), 0x00, sizeof (char *) * SET_NUM);

	memset (&(MenuColors[0]), 0x00, sizeof (char *) * MENU_BACK_STYLES * 2);
	memset (&(MenuTextureColors[0]), 0x00, sizeof (char *) * MENU_BACK_STYLES * 2);
	memset (&(MenuPixmaps[0]), 0x00, sizeof (char *) * MENU_BACK_STYLES);

	memset (&(WinColors[0]), 0x00, sizeof (char *) * BACK_STYLES * 2);
	memset (&(WinTextureColors[0]), 0x00, sizeof (char *) * BACK_STYLES * 2);
	memset (&(WinPixmaps[0]), 0x00, sizeof (char *) * BACK_STYLES);

	for (i = 0; i < BACK_STYLES + MENU_BACK_STYLES; i++)
		TextureTypes[i] = -1;

	item.memory = NULL;
	/* end initialization */

	for (pCurr = Storage; pCurr; pCurr = pCurr->next)
	{
		register int  id;

		if (pCurr->term == NULL)
			continue;
		id = pCurr->term->id;
		if (id < LOOK_DEPRECIATED_ID_START && id >= LOOK_DEPRECIATED_ID_END)
			continue;
		if (!ReadConfigItem (&item, pCurr))
			continue;

		switch (id)
		{									   /* fonts : */
		 case LOOK_Font_ID:
			 set_string_value (&Fonts[SET_MENU], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_WindowFont_ID:
			 set_string_value (&Fonts[SET_WINDOW], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_IconFont_ID:
			 set_string_value (&Fonts[SET_ICON], item.data.string, &change_flags, CHANGED_ICON);
			 break;
			 /* colors */
		 case LOOK_MTitleForeColor_ID:
			 set_string_value (&MenuColors[MENU_BACK_TITLE][0], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MTitleBackColor_ID:
			 set_string_value (&MenuColors[MENU_BACK_TITLE][1], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MenuForeColor_ID:
			 set_string_value (&MenuColors[MENU_BACK_ITEM][0], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MenuBackColor_ID:
			 set_string_value (&MenuColors[MENU_BACK_ITEM][1], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MenuHiForeColor_ID:
			 set_string_value (&MenuColors[MENU_BACK_HILITE][0], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MenuHiBackColor_ID:
			 set_string_value (&MenuColors[MENU_BACK_HILITE][1], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MenuStippleColor_ID:
			 set_string_value (&MenuColors[MENU_BACK_STIPPLE][0], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_StdForeColor_ID:
			 set_string_value (&WinColors[BACK_UNFOCUSED][0], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_StdBackColor_ID:
			 set_string_value (&WinColors[BACK_UNFOCUSED][1], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_StickyForeColor_ID:
			 set_string_value (&WinColors[BACK_STICKY][0], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_StickyBackColor_ID:
			 set_string_value (&WinColors[BACK_STICKY][1], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_HiForeColor_ID:
			 set_string_value (&WinColors[BACK_FOCUSED][0], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_HiBackColor_ID:
			 set_string_value (&WinColors[BACK_FOCUSED][1], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_TitleTextMode_ID:
			 TitleTextStyle = item.data.integer;
			 set_flags (change_flags, CHANGED_WINDOW);
			 break;

		 case LOOK_TextureTypes_ID:
			 array = &(TextureTypes[0]);	   /* will process it down below */
			 break;
		 case LOOK_TitleTextureColor_ID:	   /* title */
			 if (pCurr->argc >= 2)
			 {
				 set_string_value (&WinTextureColors[BACK_FOCUSED][0], item.data.string, &change_flags, CHANGED_WINDOW);
				 set_string_value (&WinTextureColors[BACK_FOCUSED][1], mystrdup (pCurr->argv[1]), &change_flags,
								   CHANGED_WINDOW);
			 }
			 break;
		 case LOOK_UTitleTextureColor_ID:	   /* unfoc tit */
			 if (pCurr->argc >= 2)
			 {
				 set_string_value (&WinTextureColors[BACK_UNFOCUSED][0], item.data.string, &change_flags,
								   CHANGED_WINDOW);
				 set_string_value (&WinTextureColors[BACK_UNFOCUSED][1], mystrdup (pCurr->argv[1]), &change_flags,
								   CHANGED_WINDOW);
			 }
			 break;
		 case LOOK_STitleTextureColor_ID:	   /* stic tit */
			 if (pCurr->argc >= 2)
			 {
				 set_string_value (&WinTextureColors[BACK_STICKY][0], item.data.string, &change_flags, CHANGED_WINDOW);
				 set_string_value (&WinTextureColors[BACK_STICKY][1], mystrdup (pCurr->argv[1]), &change_flags,
								   CHANGED_WINDOW);
			 }
			 break;
		 case LOOK_MTitleTextureColor_ID:	   /* menu title */
			 if (pCurr->argc >= 2)
			 {
				 set_string_value (&MenuTextureColors[MENU_BACK_TITLE][0], item.data.string, &change_flags,
								   CHANGED_MENU);
				 set_string_value (&MenuTextureColors[MENU_BACK_TITLE][1], mystrdup (pCurr->argv[1]), &change_flags,
								   CHANGED_MENU);
			 }
			 break;
		 case LOOK_MenuTextureColor_ID:	   /* menu items */
			 if (pCurr->argc >= 2)
			 {
				 set_string_value (&MenuTextureColors[MENU_BACK_ITEM][0], item.data.string, &change_flags,
								   CHANGED_MENU);
				 set_string_value (&MenuTextureColors[MENU_BACK_ITEM][1], mystrdup (pCurr->argv[1]), &change_flags,
								   CHANGED_MENU);
			 }
			 break;
		 case LOOK_MenuHiTextureColor_ID:	   /* sel items */
			 if (pCurr->argc >= 2)
			 {
				 set_string_value (&MenuTextureColors[MENU_BACK_HILITE][0], item.data.string, &change_flags,
								   CHANGED_MENU);
				 set_string_value (&MenuTextureColors[MENU_BACK_HILITE][1], mystrdup (pCurr->argv[1]), &change_flags,
								   CHANGED_MENU);
			 }
			 break;
		 case LOOK_MenuPixmap_ID:			   /* menu entry */
			 set_string_value (&MenuPixmaps[MENU_BACK_ITEM], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MenuHiPixmap_ID:		   /* hil m entr */
			 set_string_value (&MenuPixmaps[MENU_BACK_HILITE], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MTitlePixmap_ID:		   /* menu title */
			 set_string_value (&MenuPixmaps[MENU_BACK_TITLE], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_TitlePixmap_ID:			   /* foc tit */
			 set_string_value (&WinPixmaps[BACK_FOCUSED], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_UTitlePixmap_ID:		   /* unfoc tit */
			 set_string_value (&WinPixmaps[BACK_UNFOCUSED], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_STitlePixmap_ID:		   /* stick tit */
			 set_string_value (&WinPixmaps[BACK_STICKY], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;

		 case LOOK_ButtonTextureType_ID:
			 ButtonTextureType = item.data.integer;
			 break;
		 case LOOK_ButtonBgColor_ID:
			 set_string_value (&ButtonColors[1], item.data.string, &change_flags, CHANGED_ICON);
			 break;
		 case LOOK_ButtonTextureColor_ID:
			 if (pCurr->argc >= 2)
			 {
				 set_string_value (&ButtonTextureColors[0], item.data.string, &change_flags, CHANGED_ICON);
				 set_string_value (&ButtonTextureColors[1], mystrdup (pCurr->argv[1]), &change_flags, CHANGED_ICON);
			 }
			 break;
		 case LOOK_ButtonPixmap_ID:
			 set_string_value (&ButtonPixmap, item.data.string, &change_flags, CHANGED_ICON);
			 break;
		 default:
			 item.ok_to_free = 1;
		}
		if (array != NULL)
		{
            if( item.data.int_array.size > BACK_STYLES + MENU_BACK_STYLES )
                item.data.int_array.size = BACK_STYLES + MENU_BACK_STYLES ;
            spread_back_param_array (array, item.data.int_array.array, item.data.int_array.size);
			item.ok_to_free = 1;

			if (item.data.int_array.size > 0)
			{
				set_flags (change_flags, CHANGED_WINDOW);
				if (item.data.int_array.size > 3)
					set_flags (change_flags, CHANGED_MENU);
			}
			array = NULL;
		}
	}

	ReadConfigItem (&item, NULL);

	if (get_flags (change_flags, CHANGED_WINDOW))
		for (i = 0; i < BACK_STYLES; i++)
		{
			MergeMyStyleText (mystyles_list, config->window_styles[i], Fonts[SET_WINDOW], WinColors[i][0],
							  WinColors[i][1], TitleTextStyle);
            MergeMyStyleTextureOld (mystyles_list, config->window_styles[i], TextureTypes[i],
									WinTextureColors[i][0], WinTextureColors[i][0], WinPixmaps[i]);

			if (WinColors[i][0])
				free (WinColors[i][0]);
			if (WinColors[i][1])
				free (WinColors[i][1]);
			if (WinTextureColors[i][0])
				free (WinTextureColors[i][0]);
			if (WinTextureColors[i][1])
				free (WinTextureColors[i][1]);
			if (WinPixmaps[i])
				free (WinPixmaps[i]);
		}
	if (get_flags (change_flags, CHANGED_MENU))
		for (i = 0; i < MENU_BACK_STYLES; i++)
		{
			MergeMyStyleText (mystyles_list, config->menu_styles[i], Fonts[SET_MENU], MenuColors[i][0],
							  MenuColors[i][0], -1);
			MergeMyStyleTextureOld (mystyles_list, config->menu_styles[i], TextureTypes[i + BACK_STYLES],
									MenuTextureColors[i][0], MenuTextureColors[i][0], MenuPixmaps[i]);
			if (MenuColors[i][0])
				free (MenuColors[i][0]);
			if (MenuColors[i][1])
				free (MenuColors[i][1]);
			if (MenuTextureColors[i][0])
				free (MenuTextureColors[i][0]);
			if (MenuTextureColors[i][1])
				free (MenuTextureColors[i][1]);
			if (MenuPixmaps[i])
				free (MenuPixmaps[i]);
		}
	if (get_flags (change_flags, CHANGED_ICON))
	{
		for (i = 0; i < BACK_STYLES; i++)
		{
			MergeMyStyleText (mystyles_list, button_title_styles[i], Fonts[SET_ICON], ButtonColors[0], ButtonColors[1],
							  -1);
			MergeMyStyleTextureOld (mystyles_list, button_title_styles[i], ButtonTextureType,
									ButtonTextureColors[0], ButtonTextureColors[0], ButtonPixmap);
		}
		if (ButtonColors[0])
			free (ButtonColors[0]);
		if (ButtonColors[1])
			free (ButtonColors[1]);
		if (ButtonTextureColors[0])
			free (ButtonTextureColors[0]);
		if (ButtonTextureColors[1])
			free (ButtonTextureColors[1]);
		if (ButtonPixmap)
			free (ButtonPixmap);
	}

	for (i = 0; i < SET_NUM; i++)
		if (Fonts[i])
			free (Fonts[i]);

}


LookConfig   *
ParseLookOptions (const char *filename, char *myname)
{
	ConfigData cd ;
	ConfigDef    *ConfigReader;
	LookConfig   *config = CreateLookConfig ();
	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem    item;
	MyStyleDefinition **styles_tail = &(config->style_defs);
	MyFrameDefinition **frames_tail = &(config->frame_defs);
	MyBackgroundConfig **backs_tail = &(config->back_defs);

	cd.filename = filename ;
	ConfigReader = InitConfigReader (myname, &LookSyntax, CDT_Filename, cd, NULL);
	if (!ConfigReader)
		return config;

	item.memory = NULL;
	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &Storage);
	PrintFreeStorage (Storage);

	/* getting rid of all the crap first */
	StorageCleanUp (&Storage, &(config->more_stuff), CF_DISABLED_OPTION);
	config->balloon_conf = Process_balloonOptions (Storage, NULL);

	for (pCurr = Storage; pCurr; pCurr = pCurr->next)
	{
		register int  id;

		if (pCurr->term == NULL)
			continue;
		id = pCurr->term->id;
		if (id == BGR_MYBACKGROUND)
		{
            if ((*backs_tail = ParseMyBackgroundOptions (pCurr->sub, myname)) != NULL)
				backs_tail = &((*backs_tail)->next);
			continue;
		}

		if (id < LOOK_SUPPORTED_ID_START && id > LOOK_SUPPORTED_ID_END)
			continue;
		if (ReadFlagItem (&(config->set_flags), &(config->flags), pCurr, LookFlagsXref))
			continue;
		if (!ReadConfigItem (&item, pCurr))
			continue;
		if (id >= LOOK_WindowStyle_ID_START && id < LOOK_WindowStyle_ID_END)
		{
			id -= LOOK_WindowStyle_ID_START;
			set_string_value (&(config->window_styles[id]), item.data.string, NULL, 0);
		} else if (id >= LOOK_MenuStyle_ID_START && id < LOOK_MenuStyle_ID_END)
		{
			id -= LOOK_MenuStyle_ID_START;
			set_string_value (&(config->menu_styles[id]), item.data.string, NULL, 0);
		} else
		{
			switch (pCurr->term->id)
			{
			 case LOOK_IconBox_ID:
				 {
					 register int  bnum = config->icon_boxes_num % MAX_BOXES;

					 config->icon_boxes[bnum] = item.data.box;
					 config->icon_boxes_num++;
				 }
				 break;
#define SET_ONOFF_FLAG(i,f,sf,v) if(i)set_flags((f),(v));else clear_flags((f),(v)); set_flags((sf),(v))
			 case LOOK_MArrowPixmap_ID:	   /* menu arrow */
				 set_string_value (&(config->menu_arrow), item.data.string, NULL, 0);
				 break;
			 case LOOK_MenuPinOn_ID:		   /* menu pin */
				 set_string_value (&(config->menu_pin_on), item.data.string, NULL, 0);
				 break;
			 case LOOK_MenuPinOff_ID:
				 set_string_value (&(config->menu_pin_off), item.data.string, NULL, 0);
				 break;

			 case LOOK_TextGradientColor_ID:  /* title text */
				 {
					 register int  i;

					 for (i = 0; i < 2 && i < pCurr->argc; i++)
						 set_string_value (&(config->text_gradient[i]), mystrdup (pCurr->argv[i]), NULL, 0);
					 item.ok_to_free = 1;
				 }
				 break;
			 case LOOK_TitleTextAlign_ID:
				 config->title_text_align = item.data.integer;
				 set_flags (config->set_flags, LOOK_TitleTextAlign);
				 break;
			 case LOOK_TitleButtonSpacing_ID:
				 config->title_button_spacing = item.data.integer;
				 set_flags (config->set_flags, LOOK_TitleButtonSpacing);
				 break;
			 case LOOK_TitleButtonStyle_ID:
				 config->title_button_style = item.data.integer;
				 set_flags (config->set_flags, LOOK_TitleButtonStyle);
				 break;
			 case LOOK_TitleButtonXOffset_ID:
				 config->title_button_x_offset = item.data.integer;
				 set_flags (config->set_flags, LOOK_TitleButtonXOffset);
				 break;
			 case LOOK_TitleButtonYOffset_ID:
				 config->title_button_y_offset = item.data.integer;
				 set_flags (config->set_flags, LOOK_TitleButtonYOffset);
				 break;
			 case LOOK_TitleButton_ID:
				 if (item.data.button != NULL)
				 {
                     if (config->normal_buttons[item.index])
                         destroy_asbutton (config->normal_buttons[item.index], False);
                     config->normal_buttons[item.index] = item.data.button;
				 } else
					 item.ok_to_free = 1;
				 break;
             case LOOK_ResizeMoveGeometry_ID:
				 config->resize_move_geometry = item.data.geometry;
				 set_flags (config->set_flags, LOOK_ResizeMoveGeometry);
				 break;
			 case LOOK_StartMenuSortMode_ID:
				 config->start_menu_sort_mode = item.data.integer;
				 set_flags (config->set_flags, LOOK_StartMenuSortMode);
				 break;
			 case LOOK_DrawMenuBorders_ID:
				 config->draw_menu_borders = item.data.integer;
				 set_flags (config->set_flags, LOOK_DrawMenuBorders);
				 break;
			 case LOOK_ButtonSize_ID:
				 config->button_size = item.data.box;
				 set_flags (config->set_flags, LOOK_ButtonSize);
				 break;

			 case LOOK_RubberBand_ID:
				 config->rubber_band = item.data.integer;
				 set_flags (config->set_flags, LOOK_RubberBand);
				 break;
			 case LOOK_ShadeAnimationSteps_ID:
				 config->shade_animation_steps = item.data.integer;
				 set_flags (config->set_flags, LOOK_ShadeAnimationSteps);
				 break;

			 case MYSTYLE_START_ID:
				 styles_tail = ProcessMyStyleOptions (pCurr->sub, styles_tail);
				 item.ok_to_free = 1;
				 break;
			 case MYFRAME_START_ID:
				 frames_tail = ProcessMyFrameOptions (pCurr->sub, frames_tail);
				 item.ok_to_free = 1;
				 break;

             case LOOK_MenuFrame_ID :
                 set_string_value (&(config->menu_frame), item.data.string, NULL, 0);
                 break;

			 case LOOK_DeskBack_ID:
			 case LOOK_DeskLayout_ID:
				 ParseDesktopOptions (&(config->desk_configs), &item, pCurr->term->id-LOOK_DeskConfig_ID_START );
				 break;

			 case LOOK_SupportedHints_ID:
                 if( config->supported_hints )
                    destroy_hints_list( &(config->supported_hints) );
                 config->supported_hints = ProcessSupportedHints(pCurr->sub);
                 item.ok_to_free = 1;
                 break;

			 default:
				 item.ok_to_free = 1;
			}
		}
	}
	if (config->icon_boxes_num >= MAX_BOXES)
		config->icon_boxes_num = MAX_BOXES;

	ReadConfigItem (&item, NULL);

	merge_depreciated_options (config, Storage);

	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);
	return config;
}

/* returns:
 *            0 on success
 *              1 if data is empty
 *              2 if ConfigWriter cannot be initialized
 *
 */
int
WriteLookOptions (const char *filename, char *myname, LookConfig * config, unsigned long flags)
{
	ConfigData cd ;
	ConfigDef    *ConfigWriter = NULL;
	FreeStorageElem *Storage = NULL, **tail = &Storage;
	register int  i;

	if (config == NULL)
		return 1;
	cd.filename = filename;
	if ((ConfigWriter = InitConfigWriter (myname, &LookSyntax, CDT_Filename, cd)) == NULL)
		return 2;

	CopyFreeStorage (&Storage, config->more_stuff);

	/* building free storage here */
	/* MyStyles go first */
	if (config->style_defs)
        tail = MyStyleDefs2FreeStorage (&LookSyntax, tail, config->style_defs);
	/* then MyFrames */
	if (config->frame_defs)
		tail = MyFrameDefs2FreeStorage (&LookSyntax, tail, config->frame_defs);

	/* Window Styles */
    for (i = 0; i < BACK_STYLES; i++)
		if (config->window_styles[i])
			tail = QuotedString2FreeStorage (&LookSyntax, tail, NULL, config->window_styles[i],
											 LOOK_WindowStyle_ID_START + i);

	/* Menu Styles */
	for (i = 0; i < MENU_BACK_STYLES; i++)
		if (config->menu_styles[i])
			tail = QuotedString2FreeStorage (&LookSyntax, tail, NULL, config->menu_styles[i],
											 LOOK_MenuStyle_ID_START + i);

    tail = String2FreeStorage (&LookSyntax, tail, config->menu_frame, LOOK_MenuFrame_ID);

    /* Menu Icons */
	if (config->menu_arrow)
		tail = String2FreeStorage (&LookSyntax, tail, config->menu_arrow, LOOK_MArrowPixmap_ID);
	if (config->menu_pin_on)
		tail = String2FreeStorage (&LookSyntax, tail, config->menu_pin_on, LOOK_MenuPinOn_ID);
	if (config->menu_pin_off)
		tail = String2FreeStorage (&LookSyntax, tail, config->menu_pin_off, LOOK_MenuPinOff_ID);
	/* TextGradient */
	if (get_flags (config->set_flags, GradientText))
	{
		if (config->text_gradient[0] != NULL && config->text_gradient[1] != NULL)
			tail = Strings2FreeStorage (&LookSyntax, tail, config->text_gradient, 2, LOOK_TextGradientColor_ID);
	}
	/* Other Misc stuff */
	if (get_flags (config->set_flags, LOOK_StartMenuSortMode))
        tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->start_menu_sort_mode, LOOK_StartMenuSortMode_ID);
	if (get_flags (config->set_flags, LOOK_DrawMenuBorders))
        tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->draw_menu_borders, LOOK_DrawMenuBorders_ID);
	if (get_flags (config->set_flags, LOOK_RubberBand))
        tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->rubber_band, LOOK_RubberBand_ID);
	if (get_flags (config->set_flags, LOOK_ShadeAnimationSteps))
        tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->shade_animation_steps, LOOK_ShadeAnimationSteps_ID);
	/* flags : */
	tail = Flags2FreeStorage (&LookSyntax, tail, LookFlagsXref, config->set_flags, config->flags);

	/* ResizeMove window geometry */
	if (get_flags (config->set_flags, LOOK_ResizeMoveGeometry))
		tail = Geometry2FreeStorage (&LookSyntax, tail, &(config->resize_move_geometry), LOOK_ResizeMoveGeometry_ID);
	/* Icon boxes and size */
	for (i = 0; i < config->icon_boxes_num; i++)
		tail = Box2FreeStorage (&LookSyntax, tail, &(config->icon_boxes[i]), LOOK_IconBox_ID);
	if (get_flags (config->set_flags, LOOK_ButtonSize))
		tail = Box2FreeStorage (&LookSyntax, tail, &(config->button_size), LOOK_ButtonSize_ID);
	/* TitleButtons config: */
/*    if (config->balloon_conf)
        tail = balloon2FreeStorage (&LookSyntax, tail, config->balloon_conf); */

	if (get_flags (config->set_flags, LOOK_TitleTextAlign))
        tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->title_text_align, LOOK_TitleTextAlign_ID);
	if (get_flags (config->set_flags, LOOK_TitleButtonStyle))
        tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->title_button_style, LOOK_TitleButtonStyle_ID);
	if (get_flags (config->set_flags, LOOK_TitleButtonXOffset))
        tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->title_button_x_offset, LOOK_TitleButtonXOffset_ID);
	if (get_flags (config->set_flags, LOOK_TitleButtonYOffset))
        tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->title_button_y_offset, LOOK_TitleButtonYOffset_ID);
	if (get_flags (config->set_flags, LOOK_TitleButtonSpacing))
        tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->title_button_spacing, LOOK_TitleButtonSpacing_ID);

    /* then Supported Hints :*/
    if (config->supported_hints)
        tail = SupportedHints2FreeStorage (&LookSyntax, tail, config->supported_hints);

    /* Normal Title Buttons : */
	for (i = 0; i < MAX_BUTTONS; i++)
        if (config->normal_buttons[i] != NULL)
            tail = ASButton2FreeStorage (&LookSyntax, tail, i, config->normal_buttons[i], LOOK_TitleButton_ID);
    /* Iconifyed window's Title Buttons : */
    for (i = 0; i < MAX_BUTTONS; i++)
        if (config->icon_buttons[i] != NULL)
            tail = ASButton2FreeStorage (&LookSyntax, tail, i, config->icon_buttons[i], LOOK_IconTitleButton_ID);

	/* writing config into the file */
	cd.filename = filename ;
	WriteConfig (ConfigWriter, &Storage, CDT_Filename, &cd, flags);
    DestroyConfig (ConfigWriter);

	if (Storage)
	{
		fprintf (stderr, "\n%s:Config Writing warning: Not all Free Storage discarded! Trying again...", myname);
		DestroyFreeStorage (&Storage);
		fprintf (stderr, (Storage != NULL) ? " failed." : " success.");
	}
	return 0;
}

#if 0
MyLook       *
LookConfig2MyLook ( struct LookConfig * config, MyLook * look,
				    struct ASImageManager *imman, struct ASFontManager *fontman,
                    Bool free_resources, Bool do_init, unsigned long what_flags	)
{
	register int  i;

	if (look)
	{
        if( look->magic != MAGIC_MYLOOK )
            look = mylook_create ();
        else if (do_init)
			mylook_init (look, free_resources, what_flags);
	} else
		look = mylook_create ();

	if (get_flags (what_flags, LL_Flags))
		look->flags = (look->flags & ~config->set_flags) | config->flags;

	if (get_flags (what_flags, LL_MyStyles))
	{
		MyStyleDefinition *pCurr;

		look->styles_list = mystyle_list_init();
		for (pCurr = config->style_defs; pCurr; pCurr = pCurr->next)
			mystyle_list_create_from_definition (look->styles_list, pCurr, imman, fontman);

/*	DestroyMyStyleDefinitions (list); */
		if (config->style_defs != NULL)
			mystyle_list_fix_styles (look->styles_list, imman, fontman );

		if (get_flags (what_flags, LL_MSMenu))
		{									   /* menu MyStyles */
			for (i = 0; i < MENU_BACK_STYLES; i++)
				if (config->menu_styles[i] != NULL)
					look->MSMenu[i] = mystyle_list_new(look->styles_list, config->menu_styles[i]);
		}

		if (get_flags (what_flags, LL_MSWindow))
		{									   /* focussed, unfocussed, sticky window styles */
			for (i = 0; i < BACK_STYLES; i++)
				if (config->window_styles[i] != NULL)
					look->MSWindow[i] = mystyle_list_new (look->styles_list, config->window_styles[i]);
		}
	}

	if (get_flags (what_flags, LL_DeskBacks))
	{
		MyBackgroundConfig 	*pcurr ;
		MyBackground 		*myback;

		init_deskbacks_list( look );
		for( pcurr = config->back_defs ; pcurr ; pcurr = pcurr->next )
			if( (myback = render_myback( look, pcurr, imman ))!=NULL )
				add_myback_to_list( look->backs_list, myback, imman);
	}
	if (get_flags (what_flags, LL_DeskConfigs))
	{
		register DesktopConfig 		*pcurr ;

		init_deskconfigs_list( look );

		for( pcurr = config->desk_configs ; pcurr ; pcurr = pcurr->next )
		{
			MyDesktopConfig 	*dc = safecalloc( 1, sizeof(MyDesktopConfig));
			dc->desk = pcurr->desk ;
			dc->back_name = pcurr->back_name ;
			dc->layout_name = pcurr->layout_name ;
			pcurr->back_name = pcurr->layout_name = NULL ;

			add_deskconfig_to_list( look->desk_configs, dc);
		}
	}

	if (get_flags (what_flags, LL_MenuIcons))
	{
		if (config->menu_arrow)
			look->MenuArrow = filename2asicon (config->menu_arrow, imman );
		if (config->menu_pin_on)
			look->MenuPinOn = filename2asicon (config->menu_pin_on, imman );
		if (config->menu_pin_off)
			look->MenuPinOff = filename2asicon(config->menu_pin_off, imman );
	}

	if (get_flags (what_flags, LL_Buttons))
	{										   /* TODO: Titlebuttons - we 'll move them into  MyStyles */
        check_ascontextbuttons( &(look->normal_buttons), ASC_TitleButton_Start, ASC_TitleButton_End );
        check_ascontextbuttons( &(look->icon_buttons), ASC_TitleButton_Start, ASC_TitleButton_End );
        /* we will translate button numbers from the user input */
        for (i = 0; i <= ASC_TitleButton_End-ASC_TitleButton_Start; i++)
		{
            look->normal_buttons.buttons[i] = dup_asbutton(config->normal_buttons[i]);
            if (look->normal_buttons.buttons[i])
			{
				int s ;
				for( s = 0 ; s < ASB_StateCount ; ++s )
        			if( look->normal_buttons.buttons[i]->shapes[s] )
	                    load_asicon (look->normal_buttons.buttons[i]->shapes[s], NULL, imman);

                look->buttons[i] = look->normal_buttons.buttons[i]->shapes[ASB_State_Up];
                look->dbuttons[i] = look->normal_buttons.buttons[i]->shapes[ASB_State_Down];
			} else
			{
				look->buttons[i] = NULL;
				look->dbuttons[i] = NULL;
			}

            look->icon_buttons.buttons[i] = dup_asbutton(config->icon_buttons[i]);
            if (look->icon_buttons.buttons[i])
			{
				int s ;
				for( s = 0 ; s < ASB_StateCount ; ++s )
                	if (look->icon_buttons.buttons[i]->shapes[s])
                    	load_asicon (look->icon_buttons.buttons[i]->shapes[s], NULL, imman);
            }
        }
	}

	if (get_flags (what_flags, LL_SizeWindow))
	{
		MyStyle      *focus_style = look->MSWindow[BACK_FOCUSED];
		int           width = -1, height = -1;

		if (focus_style)
		{
			int           max_digits = 1;
			char         *test_str;
			unsigned long test_val = (Scr.VxMax + Scr.MyDisplayWidth) * 3;

			if (test_val < (Scr.VyMax + Scr.MyDisplayHeight) * 3)
				test_val = (Scr.VyMax + Scr.MyDisplayHeight) * 3;
			while ((test_val / (10 * max_digits)) > 0)
				max_digits++;

			test_str = safemalloc (max_digits * 4 + 11);
			sprintf (test_str, " %lu x %lu %+ld %+ld ", test_val, test_val, test_val, test_val);
/*			mystyle_get_text_geometry(focus_style, test_str, strlen(test_str), &width, &height); */
			free (test_str);
		}

		if( get_flags (config->set_flags, LOOK_ResizeMoveGeometry) )
			look->resize_move_geometry = config->resize_move_geometry;
		else
			look->resize_move_geometry.flags = 0 ;
		if (!get_flags (look->resize_move_geometry.flags, WidthValue) && width > 0)
		{
			look->resize_move_geometry.width = width;
			set_flags (look->resize_move_geometry.flags, WidthValue);
		}

		if (!get_flags (look->resize_move_geometry.flags, HeightValue) && height > 0)
		{
			look->resize_move_geometry.height = height;
			set_flags (look->resize_move_geometry.flags, HeightValue);
		}
	}

	if (get_flags (what_flags, LL_Layouts))
	{
		MyFrameDefinition *frame_def = config->frame_defs;

		look->TitleTextAlign = get_flags (config->set_flags, LOOK_TitleTextAlign) ? config->title_text_align : 0;	/* alignment of title bar text */
		look->TitleButtonSpacing =
			get_flags (config->set_flags, LOOK_TitleButtonSpacing) ? config->title_button_spacing : 3;
		look->TitleButtonStyle = get_flags (config->set_flags, LOOK_TitleButtonStyle) ? config->title_button_style : 0;	/* 0 - afterstep, 1 - WM old, 2 - free hand */
		look->TitleButtonXOffset =
			get_flags (config->set_flags, LOOK_TitleButtonXOffset) ? config->title_button_x_offset : 0;
		look->TitleButtonYOffset =
			get_flags (config->set_flags, LOOK_TitleButtonYOffset) ? config->title_button_y_offset : 0;
        look->TitleButtonStyle %= 3;
        switch (look->TitleButtonStyle)
        {
            case 0:                                   /* traditional AfterStep style */
	            look->TitleButtonXOffset[0] = look->TitleButtonXOffset[1] = 3;
    	        look->TitleButtonYOffset[0] = look->TitleButtonYOffset[1] = 3;
        	    break ;
        	case 1 :
            	look->TitleButtonXOffset[0] = look->TitleButtonXOffset[1] = 1;
            	look->TitleButtonYOffset[0] = look->TitleButtonYOffset[1] = 1;
                break;
            case 2:                                   /* advanced AfterStep style */
                /* nothing to do here - user must supply values for x/y offsets */
                break;
        }
		mylook_init_layouts( look );

		while (frame_def != NULL)
		{
			add_frame_to_list(look->layouts, filenames2myframe (frame_def->name, frame_def->parts, imman));
			frame_def = frame_def->next;
		}
        if( config->menu_frame )
		{
			ASHashData hdata = {0};
            if( get_hash_item( look->layouts, (ASHashableValue) config->menu_frame, &hdata.vptr) != ASH_Success )
                look->menu_frame = NULL;
			else
				look->menu_frame = hdata.vptr ;
		}
	}

	if (get_flags (what_flags, LL_Buttons) && look->layouts )
	{
		update_titlebar_buttons( look->layouts, NULL, False, &(look->normal_buttons) );
		update_titlebar_buttons( look->layouts, NULL, True, &(look->icon_buttons) );
	}

	if (get_flags (what_flags, LL_Boundary))
		look->BoundaryWidth = BOUNDARY_WIDTH;  /* not a configuration option yet */

	if (get_flags (what_flags, LL_MenuParams))
	{
		look->DrawMenuBorders = get_flags (config->set_flags, LOOK_DrawMenuBorders) ? config->draw_menu_borders : 1;
		look->StartMenuSortMode =
			get_flags (config->set_flags, LOOK_StartMenuSortMode) ? config->start_menu_sort_mode : SORTBYALPHA;
	}

	if (get_flags (what_flags, LL_Icons))
	{
		for (i = 0; i < config->icon_boxes_num; i++)
		{
			look->IconBoxes[i] = config->icon_boxes[i];
			while( look->IconBoxes[i].left < 0 ) look->IconBoxes[i].left += Scr.MyDisplayWidth ;
			if( look->IconBoxes[i].left == 0 && get_flags( look->IconBoxes[i].flags, LeftNegative ) )
				look->IconBoxes[i].left = Scr.MyDisplayWidth ;

			while( look->IconBoxes[i].top < 0 ) look->IconBoxes[i].top += Scr.MyDisplayHeight ;
			if( look->IconBoxes[i].top == 0 && get_flags( look->IconBoxes[i].flags, TopNegative ) )
				look->IconBoxes[i].top = Scr.MyDisplayHeight ;

			while( look->IconBoxes[i].right < 0 ) look->IconBoxes[i].right += Scr.MyDisplayWidth ;
			if( look->IconBoxes[i].right == 0 && get_flags( look->IconBoxes[i].flags, RightNegative ) )
				look->IconBoxes[i].right = Scr.MyDisplayWidth ;

			while( look->IconBoxes[i].bottom < 0 ) look->IconBoxes[i].bottom += Scr.MyDisplayHeight ;
			if( look->IconBoxes[i].bottom == 0 && get_flags( look->IconBoxes[i].flags, BottomNegative ) )
				look->IconBoxes[i].bottom = Scr.MyDisplayHeight ;
		}
		look->NumBoxes = config->icon_boxes_num;
		if (get_flags (config->set_flags, LOOK_ButtonSize))
		{
			look->ButtonWidth = config->button_size.left;
			look->ButtonHeight = config->button_size.top;
		}
	}

	if (get_flags (what_flags, LL_Misc))
	{
		look->RubberBand = get_flags (config->set_flags, LOOK_RubberBand) ? config->rubber_band : 0;
		look->ShadeAnimationSteps =
			get_flags (config->set_flags, LOOK_ShadeAnimationSteps) ? config->shade_animation_steps : 12;
	}

	if (get_flags (what_flags, LL_Balloons))
	{
		look->balloon_look = balloon_look_new ();
		if (config->balloon_conf)
			BalloonConfig2BalloonLook (look->balloon_look, config->balloon_conf);
	}

    if (get_flags (what_flags, LL_SupportedHints))
    {
        if( config->supported_hints )
        {
            look->supported_hints = config->supported_hints ;
            config->supported_hints = NULL ;
        }else
        {/* Otherwise we should enable all the hints possible in standard order :*/
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

	return look;
}

#endif
