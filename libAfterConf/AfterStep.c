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

SyntaxDef DummyFuncSyntax = {
    '\0',   '\n',	NULL,
	0, 		' ',	"",			"\t",
	"AfterStep Function",
	"Functions",
	"built in AfterStep functions",
	NULL,	0
};

SyntaxDef     DummyPopupFuncSyntax = {
	'\n',	'\0',	NULL,
	0, 		' ',	"\t",		"\t",
    "Popup/Complex function definition",
	"Popup",
	"",	
	NULL,	0
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

SyntaxDef SupportedHintsSyntax = {',','\n',SupportedHintsTerms,7,' '," ","\t","Look Supported hints list","SupportedHints","",NULL,0};

TermDef       PlacementStrategyTerms[] =
{
    {TF_NO_MYNAME_PREPENDING, "SmartPlacement", 14, TT_FLAG, 	FEEL_SmartPlacement_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "RandomPlacement", 15, TT_FLAG, 	FEEL_RandomPlacement_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "Tile", 5, TT_FLAG, 				FEEL_Tile_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "Cascade", 7, TT_FLAG, 			FEEL_Cascade_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "Manual", 6, TT_FLAG, 			FEEL_Manual_ID, NULL},
    {0, NULL, 0, 0, 0}
};

SyntaxDef PlacementStrategySyntax = {',','\n',PlacementStrategyTerms,7,' '," ","\t","Window Placement types","Placement",
	"AfterStep supports several different window placement policies."
	" Some of them designed to fill free space, and some allowing for windows to be placed on top of others.",NULL,0};


extern SyntaxDef     MyBackgroundSyntax ;      /* see ASetRoot.c */
/**************************************************************************
 * WindowBox terms : 
 *************************************************************************/ 
TermDef       WindowBoxTerms[] = {
    {TF_NO_MYNAME_PREPENDING|TF_SYNTAX_START, "WindowBox", 9, TT_QUOTED_TEXT, WINDOWBOX_START_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "Area", 4,              TT_GEOMETRY,WINDOWBOX_Area_ID   	  	 , NULL},
    {TF_NO_MYNAME_PREPENDING, "Virtual", 7,           TT_FLAG,    WINDOWBOX_Virtual_ID       , NULL},
    {TF_NO_MYNAME_PREPENDING, "MinWidth", 8,          TT_INTEGER, WINDOWBOX_MinWidth_ID	  	 , NULL},
    {TF_NO_MYNAME_PREPENDING, "MinHeight", 9,         TT_INTEGER, WINDOWBOX_MinHeight_ID	 , NULL},
    {TF_NO_MYNAME_PREPENDING, "MaxWidth", 8,          TT_INTEGER, WINDOWBOX_MaxWidth_ID	  	 , NULL},
    {TF_NO_MYNAME_PREPENDING, "MaxHeight", 9,         TT_INTEGER, WINDOWBOX_MaxHeight_ID	 , NULL},
    {TF_NO_MYNAME_PREPENDING, "FirstTry", 8,          TT_INTEGER, WINDOWBOX_FirstTry_ID	  	 , &PlacementStrategySyntax},
    {TF_NO_MYNAME_PREPENDING, "ThenTry", 7,           TT_INTEGER, WINDOWBOX_ThenTry_ID 	  	 , &PlacementStrategySyntax},
    {TF_NO_MYNAME_PREPENDING, "Desk", 4,              TT_INTEGER, WINDOWBOX_Desk_ID          , NULL},
    {TF_NO_MYNAME_PREPENDING, "MinLayer", 8,          TT_INTEGER, WINDOWBOX_MinLayer_ID      , NULL},
    {TF_NO_MYNAME_PREPENDING, "MaxLayer", 8,          TT_INTEGER, WINDOWBOX_MaxLayer_ID      , NULL},
    {TF_NO_MYNAME_PREPENDING, "VerticalPriority", 16, TT_FLAG,    WINDOWBOX_VerticalPriority_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "ReverseOrder", 12,     TT_FLAG,    WINDOWBOX_ReverseOrder_ID    , NULL},
    {TF_NO_MYNAME_PREPENDING |
	 TF_SYNTAX_TERMINATOR,    "~WindowBox", 10, TT_FLAG, WINDOWBOX_DONE_ID, NULL},
	{0, NULL, 0, 0, 0}
};

SyntaxDef     WindowBoxSyntax = {
	'\n',
	'\0',
	WindowBoxTerms,
    7,                                         /* hash size */
	'\t',
	"",
	"\t",
	"Feel WindowBox definition",
	"FeelWindowBox",
	"defines placement policy for certain type of windows, based on desk, window attributes, window size, etc."
	" Usefull for xinerama configurations, where windows should not be placed in between screens.",
	NULL,
	0
};

/**************************************************************************
 * .include file terms : 
 *************************************************************************/ 

TermDef includeTerms[] =
{
  /* focus : */
  {TF_NO_MYNAME_PREPENDING, "include",12            , TT_FUNCTION   , INCLUDE_include_ID          	, NULL},
  {TF_NO_MYNAME_PREPENDING, "keepname", 12          , TT_FLAG       , INCLUDE_keepname_ID          	, NULL},
  {TF_NO_MYNAME_PREPENDING, "extension", 12         , TT_TEXT       , INCLUDE_extension_ID          , NULL},
  {TF_NO_MYNAME_PREPENDING, "miniextension", 12     , TT_TEXT       , INCLUDE_miniextension_ID      , NULL},
  {TF_NO_MYNAME_PREPENDING, "minipixmap", 12        , TT_FILENAME   , INCLUDE_minipixmap_ID         , NULL},
  {TF_NO_MYNAME_PREPENDING, "command", 12           , TT_FLAG  		, INCLUDE_command_ID          	, &DummyFuncSyntax},
  {TF_NO_MYNAME_PREPENDING, "order", 12           	, TT_INTEGER    , INCLUDE_order_ID       		, NULL},
  {TF_NO_MYNAME_PREPENDING, "RecentSubmenuItems", 12, TT_INTEGER    , INCLUDE_RecentSubmenuItems_ID , NULL},
  {TF_NO_MYNAME_PREPENDING, "name", 12           	, TT_TEXT       , INCLUDE_name_ID          		, NULL},
  {0, NULL, 0, 0, 0}
};

SyntaxDef includeSyntax =
{
  '\n',
  '\0',
  includeTerms,
  0,				/* use default hash size */
  '\t',
  "",
  "\t",
  "include",
  "include",
  "include file for Popups",
  NULL,
  0
};

/**************************************************************************
 * LOOK terms : 
 **************************************************************************/

/* depreciated options : */
#define OBSOLETE_AFTERSTEP_LOOK_TERMS \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "Font", 4, TT_FONT, LOOK_Font_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "WindowFont", 10, TT_FONT, LOOK_WindowFont_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "MTitleForeColor", 15, TT_COLOR, LOOK_MTitleForeColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "MTitleBackColor", 15, TT_COLOR, LOOK_MTitleBackColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "MenuForeColor", 13, TT_COLOR, LOOK_MenuForeColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "MenuBackColor", 13, TT_COLOR, LOOK_MenuBackColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "MenuHiForeColor", 15, TT_COLOR, LOOK_MenuHiForeColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "MenuHiBackColor", 15, TT_COLOR, LOOK_MenuHiBackColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "MenuStippleColor", 16, TT_COLOR, LOOK_MenuStippleColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "StdForeColor", 12, TT_COLOR, LOOK_StdForeColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "StdBackColor", 12, TT_COLOR, LOOK_StdBackColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "StickyForeColor", 15, TT_COLOR, LOOK_StickyForeColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "StickyBackColor", 15, TT_COLOR, LOOK_StickyBackColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "HiForeColor", 11, TT_COLOR, LOOK_HiForeColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "HiBackColor", 11, TT_COLOR, LOOK_HiBackColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "IconFont", 8, TT_FONT, LOOK_IconFont_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "TextureTypes", 12, TT_INTARRAY, LOOK_TextureTypes_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "TextureMaxColors", 16, TT_INTARRAY, LOOK_TextureMaxColors_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "TitleTextureColor", 17, TT_COLOR, LOOK_TitleTextureColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "UTitleTextureColor", 18, TT_COLOR, LOOK_UTitleTextureColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "STitleTextureColor", 18, TT_COLOR, LOOK_STitleTextureColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "MTitleTextureColor", 18, TT_COLOR, LOOK_MTitleTextureColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "MenuTextureColor", 16, TT_COLOR, LOOK_MenuTextureColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "MenuHiTextureColor", 18, TT_COLOR, LOOK_MenuHiTextureColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "MenuPixmap", 10, TT_FILENAME, LOOK_MenuPixmap_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "MenuHiPixmap", 12, TT_FILENAME, LOOK_MenuHiPixmap_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "MTitlePixmap", 12, TT_FILENAME, LOOK_MTitlePixmap_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "TitlePixmap", 11, TT_FILENAME, LOOK_TitlePixmap_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "UTitlePixmap", 12, TT_FILENAME, LOOK_UTitlePixmap_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "STitlePixmap", 12, TT_FILENAME, LOOK_STitlePixmap_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "ButtonTextureType", 17, TT_UINTEGER, LOOK_ButtonTextureType_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "ButtonBgColor", 13, TT_COLOR, LOOK_ButtonBgColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "ButtonTextureColor", 18, TT_COLOR, LOOK_ButtonTextureColor_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "ButtonMaxColors", 15, TT_UINTEGER, LOOK_ButtonMaxColors_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "ButtonPixmap", 12, TT_FILENAME, LOOK_ButtonPixmap_ID, NULL}, \
    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "TitleTextMode", 13, TT_UINTEGER, LOOK_TitleTextMode_ID, NULL}
	

#define AFTERSTEP_LOOK_TERMS \
	{TF_NO_MYNAME_PREPENDING, "IconBox", 7, TT_BOX, LOOK_IconBox_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "MArrowPixmap", 12, TT_FILENAME, LOOK_MArrowPixmap_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "MenuPinOn", 9, TT_FILENAME, LOOK_MenuPinOn_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "MenuMiniPixmaps", 15, TT_FLAG, LOOK_MenuMiniPixmaps_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitlebarNoPush", 14, TT_FLAG, LOOK_TitlebarNoPush_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleTextAlign", 14, TT_UINTEGER, LOOK_TitleTextAlign_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "ResizeMoveGeometry", 18, TT_GEOMETRY, LOOK_ResizeMoveGeometry_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "StartMenuSortMode", 17, TT_UINTEGER, LOOK_StartMenuSortMode_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "DrawMenuBorders", 15, TT_UINTEGER, LOOK_DrawMenuBorders_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "ButtonSize", 10, TT_BOX, LOOK_ButtonSize_ID, NULL}, \
	{TF_NO_MYNAME_PREPENDING, "MinipixmapSize", 14, TT_BOX, LOOK_MinipixmapSize_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "SeparateButtonTitle", 19, TT_FLAG, LOOK_SeparateButtonTitle_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "RubberBand", 10, TT_UINTEGER, LOOK_RubberBand_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "DefaultStyle", 12, TT_QUOTED_TEXT, LOOK_DefaultStyle_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "FWindowStyle", 12, TT_QUOTED_TEXT, LOOK_FWindowStyle_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "UWindowStyle", 12, TT_QUOTED_TEXT, LOOK_UWindowStyle_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "SWindowStyle", 12, TT_QUOTED_TEXT, LOOK_SWindowStyle_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "MenuItemStyle", 13, TT_QUOTED_TEXT, LOOK_MenuItemStyle_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "MenuTitleStyle", 14, TT_QUOTED_TEXT, LOOK_MenuTitleStyle_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "MenuHiliteStyle", 15, TT_QUOTED_TEXT, LOOK_MenuHiliteStyle_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "MenuStippleStyle", 16, TT_QUOTED_TEXT, LOOK_MenuStippleStyle_ID, NULL}, \
	{TF_NO_MYNAME_PREPENDING, "MenuSubItemStyle", 16, TT_QUOTED_TEXT, LOOK_MenuSubItemStyle_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "MenuHiTitleStyle", 16, TT_QUOTED_TEXT, LOOK_MenuHiTitleStyle_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "MenuItemCompositionMethod", 25, TT_INTEGER, LOOK_MenuItemCompositionMethod_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "MenuHiliteCompositionMethod", 27, TT_INTEGER, LOOK_MenuHiliteCompositionMethod_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "MenuStippleCompositionMethod", 28, TT_INTEGER, LOOK_MenuStippleCompositionMethod_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "SupportedHints", 14, TT_FLAG, LOOK_SupportedHints_ID, &SupportedHintsSyntax}, \
	{TF_NO_MYNAME_PREPENDING|TF_INDEXED|TF_DONT_SPLIT, "DeskBack", 8, TT_QUOTED_TEXT, LOOK_DeskBack_ID, NULL}, \
	{TF_NO_MYNAME_PREPENDING, "DefaultFrame", 12, TT_QUOTED_TEXT, LOOK_DefaultFrame_ID, NULL}, \
	{TF_NO_MYNAME_PREPENDING, "CursorFore", 10, TT_COLOR, LOOK_CursorFore_ID, NULL}, \
	{TF_NO_MYNAME_PREPENDING, "CursorBack", 10, TT_COLOR, LOOK_CursorFore_ID, NULL}

#define AFTERSTEP_MYBACK_TERMS \
	{TF_NO_MYNAME_PREPENDING, "MyBackground", 12, TT_QUOTED_TEXT,BGR_MYBACKGROUND, &MyBackgroundSyntax}, \
	{TF_NO_MYNAME_PREPENDING, "KillBackgroundThreshold", 23, TT_INTEGER, LOOK_KillBackgroundThreshold_ID, NULL}, \
	{TF_NO_MYNAME_PREPENDING, "DontAnimateBackground", 21, TT_INTEGER, LOOK_DontAnimateBackground_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "DontDrawBackground", 18, TT_FLAG, LOOK_DontDrawBackground_ID, NULL}

#define AFTERSTEP_TITLEBUTTON_TERMS \
    {TF_NO_MYNAME_PREPENDING, "Balloons", 8, TT_FLAG, BALLOON_USED_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonBalloons", 19, TT_FLAG, BALLOON_USED_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "BalloonBorderHilite", 18, TT_FLAG, BALLOON_BorderHilite_ID, &BevelSyntax}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonBalloonBorderHilite", 29, TT_FLAG, BALLOON_BorderHilite_ID, &BevelSyntax}, \
    {TF_NO_MYNAME_PREPENDING, "BalloonXOffset", 14, TT_INTEGER, BALLOON_XOffset_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonBalloonXOffset", 25, TT_INTEGER, BALLOON_XOffset_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "BalloonYOffset", 14, TT_INTEGER, BALLOON_YOffset_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonBalloonYOffset", 25, TT_INTEGER, BALLOON_YOffset_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "BalloonDelay", 12, TT_UINTEGER, BALLOON_Delay_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonBalloonDelay", 23, TT_UINTEGER, BALLOON_Delay_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "BalloonCloseDelay", 17, TT_UINTEGER, BALLOON_CloseDelay_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonBalloonCloseDelay", 28, TT_UINTEGER, BALLOON_CloseDelay_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "BalloonStyle", 12, TT_QUOTED_TEXT, BALLOON_Style_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonBalloonStyle", 23, TT_QUOTED_TEXT, BALLOON_Style_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonSpacingLeft", 22, TT_INTEGER, LOOK_TitleButtonSpacingLeft_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonSpacingRight", 23, TT_INTEGER, LOOK_TitleButtonSpacingRight_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonSpacing", 18, TT_INTEGER, LOOK_TitleButtonSpacing_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonXOffsetLeft", 22, TT_INTEGER, LOOK_TitleButtonXOffsetLeft_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonXOffsetRight", 23, TT_INTEGER, LOOK_TitleButtonXOffsetRight_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonXOffset", 18, TT_INTEGER, LOOK_TitleButtonXOffset_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonYOffsetLeft", 22, TT_INTEGER, LOOK_TitleButtonYOffsetLeft_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonYOffsetRight", 23, TT_INTEGER, LOOK_TitleButtonYOffsetRight_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonYOffset", 18, TT_INTEGER, LOOK_TitleButtonYOffset_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonStyle", 16, TT_INTEGER, LOOK_TitleButtonStyle_ID, NULL}, \
    {TF_NO_MYNAME_PREPENDING, "TitleButtonOrder", 16, TT_TEXT, LOOK_TitleButtonOrder_ID, NULL}, \
    {TF_INDEXED|TF_NO_MYNAME_PREPENDING, "TitleButton", 11, TT_BUTTON, LOOK_TitleButton_ID, NULL}

/**************************************************************************
 * FEEL terms : 
 **************************************************************************/

#define POPUP_TERM 		{TF_NO_MYNAME_PREPENDING|TF_SYNTAX_START, "Popup",5                   , TT_QUOTED_TEXT, FEEL_Popup_ID             , &DummyPopupFuncSyntax}
#define FUNCTION_TERM 	{TF_NO_MYNAME_PREPENDING|TF_SYNTAX_START, "Function",8                , TT_QUOTED_TEXT, FEEL_Function_ID          , &DummyPopupFuncSyntax}
  
#define AFTERSTEP_FEEL_TERMS \
  {TF_NO_MYNAME_PREPENDING, "ClickToFocus",12           , TT_FLAG       , FEEL_ClickToFocus_ID          , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "SloppyFocus",11            , TT_FLAG       , FEEL_SloppyFocus_ID           , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "AutoFocus",9               , TT_FLAG       , FEEL_AutoFocus_ID             , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "DecorateTransients",18     , TT_FLAG       , FEEL_DecorateTransients_ID    , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "DontMoveOff",11            , TT_FLAG       , FEEL_DontMoveOff_ID           , NULL}, \
  {TF_NO_MYNAME_PREPENDING|TF_DEPRECIATED, "NoPPosition",11 , TT_FLAG   , FEEL_NoPPosition_ID           , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "SmartPlacement",14         , TT_FLAG       , FEEL_SmartPlacement_ID        , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "RandomPlacement",15        , TT_FLAG       , FEEL_RandomPlacement_ID       , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "StubbornPlacement",17      , TT_FLAG       , FEEL_StubbornPlacement_ID     , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "MenusHigh",9               , TT_FLAG       , FEEL_MenusHigh_ID             , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "CenterOnCirculate",17      , TT_FLAG       , FEEL_CenterOnCirculate_ID     , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "SuppressIcons",13          , TT_FLAG       , FEEL_SuppressIcons_ID         , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "IconTitle",9               , TT_FLAG       , FEEL_IconTitle_ID             , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "KeepIconWindows",15        , TT_FLAG       , FEEL_KeepIconWindows_ID       , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "StickyIcons",11            , TT_FLAG       , FEEL_StickyIcons_ID           , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "StubbornIcons",13          , TT_FLAG       , FEEL_StubbornIcons_ID         , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "StubbornIconPlacement",21  , TT_FLAG       , FEEL_StubbornIconPlacement_ID , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "CirculateSkipIcons",18     , TT_FLAG       , FEEL_CirculateSkipIcons_ID    , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "BackingStore",12           , TT_FLAG       , FEEL_BackingStore_ID          , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "AppsBackingStore",16       , TT_FLAG       , FEEL_AppsBackingStore_ID      , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "SaveUnders",10             , TT_FLAG       , FEEL_SaveUnders_ID            , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "PagingDefault",13          , TT_FLAG       , FEEL_PagingDefault_ID         , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "AutoTabThroughDesks",19    , TT_FLAG       , FEEL_AutoTabThroughDesks_ID   , NULL}, \
  {TF_NO_MYNAME_PREPENDING, "ClickTime",9               , TT_UINTEGER   , FEEL_ClickTime_ID         	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "OpaqueMove",10             , TT_UINTEGER   , FEEL_OpaqueMove_ID        	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "OpaqueResize",12           , TT_UINTEGER   , FEEL_OpaqueResize_ID      	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "AutoRaise",9               , TT_UINTEGER   , FEEL_AutoRaise_ID         	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "AutoReverse",11            , TT_UINTEGER   , FEEL_AutoReverse_ID       	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "DeskAnimationType",17      , TT_UINTEGER   , FEEL_DeskAnimationType_ID 	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "ShadeAnimationSteps",19    , TT_UINTEGER   , FEEL_ShadeAnimationSteps_ID	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "DeskAnimationSteps",18     , TT_UINTEGER   , FEEL_DeskAnimationSteps_ID	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "XorValue",8                , TT_INTEGER    , FEEL_XorValue_ID          	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "Xzap",4                    , TT_INTEGER    , FEEL_Xzap_ID              	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "Yzap",4                    , TT_INTEGER    , FEEL_Yzap_ID              	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "ClickToRaise",12           , TT_BITLIST    , FEEL_ClickToRaise_ID      	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "EdgeScroll",10             , TT_INTARRAY   , FEEL_EdgeScroll_ID        	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "EdgeResistance",14         , TT_INTARRAY   , FEEL_EdgeResistance_ID    	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "ShadeAnimationSteps", 19, TT_UINTEGER, 	FEEL_ShadeAnimationSteps_ID		, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "CoverAnimationSteps", 19, TT_UINTEGER, 	FEEL_CoverAnimationSteps_ID		, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "CoverAnimationType", 18,  TT_UINTEGER, 	FEEL_CoverAnimationType_ID		, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "FollowTitleChanges", 18, 	TT_FLAG, 		FEEL_FollowTitleChanges_ID		, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "PersistentMenus", 15, 		TT_FLAG, 		FEEL_PersistentMenus_ID			, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "NoSnapKey", 9, 			TT_BITLIST, 	FEEL_NoSnapKey_ID				, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "ScreenEdgeAttraction", 20, TT_INTEGER, 	FEEL_EdgeAttractionScreen_ID	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "WindowEdgeAttraction", 20, TT_INTEGER, 	FEEL_EdgeAttractionWindow_ID	, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "DontRestoreFocus", 16, 	TT_FLAG, 		FEEL_DontRestoreFocus_ID		, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "DefaultWindowBox", 16, 	TT_QUOTED_TEXT, FEEL_DefaultWindowBox_ID		, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "RecentSubmenuItems", 18, 	TT_INTEGER, 	FEEL_RecentSubmenuItems_ID		, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "WinListSortOrder", 16, 	TT_INTEGER, 	FEEL_WinListSortOrder_ID		, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "WinListHideIcons", 16, 	TT_FLAG, 		FEEL_WinListHideIcons_ID		, NULL}, \
  {TF_NO_MYNAME_PREPENDING, "AnimateDeskChange", 17, 	TT_FLAG, 		FEEL_AnimateDeskChange_ID		, NULL}


#define AFTERSTEP_CURSOR_TERMS \
  {TF_INDEXED|TF_NO_MYNAME_PREPENDING, "Cursor",6           , TT_INTEGER    , FEEL_Cursor_ID            , NULL}, \
  {TF_INDEXED|TF_NO_MYNAME_PREPENDING, "CustomCursor",12    , TT_CURSOR     , FEEL_CustomCursor_ID      , NULL}

#define AFTERSTEP_MOUSE_TERMS \
  {TF_SPECIAL_PROCESSING|TF_NO_MYNAME_PREPENDING, "Mouse",5, TT_BINDING , FEEL_Mouse_ID  				, &DummyFuncSyntax}

#define AFTERSTEP_KEYBOARD_TERMS \
  {TF_SPECIAL_PROCESSING|TF_NO_MYNAME_PREPENDING, "Key",3  , TT_BINDING , FEEL_Key_ID    				, &DummyFuncSyntax}

#define AFTERSTEP_WINDOWBOX_TERMS \
  {TF_NO_MYNAME_PREPENDING, "WindowBox", 9,				TT_QUOTED_TEXT, FEEL_WindowBox_ID				, &WindowBoxSyntax}
    
	/* obsolete stuff : */
#define OBSOLETE_AFTERSTEP_FEEL_TERMS \
  {TF_OBSOLETE|TF_NO_MYNAME_PREPENDING, "MWMFunctionHints",16       , TT_FLAG       , FEEL_MWMFunctionHints_ID  , NULL}, \
  {TF_OBSOLETE|TF_NO_MYNAME_PREPENDING, "MWMDecorHints",13          , TT_FLAG       , FEEL_MWMDecorHints_ID     , NULL}, \
  {TF_OBSOLETE|TF_NO_MYNAME_PREPENDING, "MWMHintOverride",15        , TT_FLAG       , FEEL_MWMHintOverride_ID   , NULL}



/**************************************************************************
 * Different compilations to be used in parsing : 
 **************************************************************************/

TermDef       LookTerms[] = {
	OBSOLETE_AFTERSTEP_LOOK_TERMS,
/*    {TF_OBSOLETE   |TF_NO_MYNAME_PREPENDING, "MenuPinOff", 10, TT_FILENAME, LOOK_MenuPinOff_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "TextureMenuItemsIndividually", 28, TT_FLAG, LOOK_TxtrMenuItmInd_ID, NULL},

    {TF_NO_MYNAME_PREPENDING, "TextGradientColor", 17, TT_CUSTOM, LOOK_TextGradientColor_ID, NULL},                                          
    {TF_NO_MYNAME_PREPENDING, "GradientText", 12, TT_FLAG, LOOK_GradientText_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "ButtonNoBorder", 14, TT_FLAG, LOOK_ButtonNoBorder_ID, NULL},
    {TF_INDEXED|TF_NO_MYNAME_PREPENDING, "IconTitleButton", 15, TT_BUTTON, LOOK_IconTitleButton_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "MenuFrame", 16, TT_QUOTED_TEXT, LOOK_MenuFrame_ID, NULL},
	{TF_NO_MYNAME_PREPENDING|TF_INDEXED|TF_DONT_SPLIT, "DeskLayout", 8, TT_QUOTED_TEXT, LOOK_DeskLayout_ID, NULL},
*/
		
/* non depreciated options : */
	AFTERSTEP_LOOK_TERMS,
	/* including MyStyles definitions processing */
	INCLUDE_MYSTYLE,
	INCLUDE_MYFRAME,
    AFTERSTEP_MYBACK_TERMS,
	AFTERSTEP_TITLEBUTTON_TERMS,
    
	{0, NULL, 0, 0, 0}
};

TermDef       AfterStepLookTerms[] = {
	AFTERSTEP_LOOK_TERMS,
	{0, NULL, 0, 0, 0}
};

TermDef       ModuleMyStyleTerms[] = {
	INCLUDE_MYSTYLE,
	{0, NULL, 0, 0, 0}
};
TermDef       AfterStepMyFrameTerms[] = {
	INCLUDE_MYFRAME,
	{0, NULL, 0, 0, 0}
};
TermDef       AfterStepMyBackgroundTerms[] = {
    AFTERSTEP_MYBACK_TERMS,
	{0, NULL, 0, 0, 0}
};
TermDef       AfterStepTitleButtonTerms[] = {
	AFTERSTEP_TITLEBUTTON_TERMS,
	{0, NULL, 0, 0, 0}
};

TermDef FeelTerms[] =
{
	POPUP_TERM,
	FUNCTION_TERM,
	AFTERSTEP_FEEL_TERMS,
	AFTERSTEP_CURSOR_TERMS, 
	AFTERSTEP_MOUSE_TERMS, 
	AFTERSTEP_KEYBOARD_TERMS, 
	AFTERSTEP_WINDOWBOX_TERMS,
	OBSOLETE_AFTERSTEP_FEEL_TERMS,
  	{0, NULL, 0, 0, 0}
};

TermDef AfterStepFeelTerms[] =
{
	AFTERSTEP_FEEL_TERMS,
  	{0, NULL, 0, 0, 0}
};
TermDef AfterStepCursorTerms[] =
{
	AFTERSTEP_CURSOR_TERMS, 
  	{0, NULL, 0, 0, 0}
};
TermDef MouseBindingTerms[] =
{
	AFTERSTEP_MOUSE_TERMS, 
  	{0, NULL, 0, 0, 0}
};
TermDef KeyboardBindingTerms[] =
{
	AFTERSTEP_KEYBOARD_TERMS, 
  	{0, NULL, 0, 0, 0}
};
TermDef AfterStepWindowBoxTerms[] =
{
	AFTERSTEP_WINDOWBOX_TERMS,
  	{0, NULL, 0, 0, 0}
};

TermDef PopupTerms[] =
{
  POPUP_TERM,
  {0, NULL, 0, 0, 0}
};

TermDef FunctionTerms[] =
{
  FUNCTION_TERM,
  {0, NULL, 0, 0, 0}
};

extern TermDef       WharfTerms[];

/**************************************************************************
 * Syntaxes : 
 *************************************************************************/
SyntaxDef LookSyntax 		={ '\n', '\0', LookTerms, 		0, '\t', "", "\t", "Look", "Look", "AfterStep look", NULL, 0};
SyntaxDef FeelSyntax 		={ '\n', '\0', FeelTerms, 		0, '\t', "", "\t", "Feel", "Feel", "AfterStep feel configuration", NULL, 0};
SyntaxDef FunctionSyntax 	={ '\n', '\0', FunctionTerms,	0, 	' ', "", "\t", "AfterStep Complex/Builtin Function", "Function", "functions that can be executed by AfterStep", NULL, 0};
SyntaxDef PopupSyntax 		={ '\n', '\0', PopupTerms,		0,	' ', "", "\t", "AfterStep Popups", "Popups", "Definitions for AfterStep Popups", NULL, 0 };
SyntaxDef AutoExecSyntax 	={ '\n', '\0', FunctionTerms,	0, 	' ', "", "\t", "AfterStep Autoexec (startup/restart sequences)", "AutoExec", "functions to be executed by AfterStep on startup/shutdown", NULL, 0};
SyntaxDef ThemeSyntax 		={ '\n', '\0', FunctionTerms,	0,	' ', "", "\t", "theme installation script", "Theme", "AfterStep theme file", NULL, 0};

SyntaxDef AfterStepLookSyntax 		={ '\n', '\0', AfterStepLookTerms, 			0, '\t', "", "\t", "Look", "Look", "AfterStep look", NULL, 0};
SyntaxDef ModuleMyStyleSyntax 		={ '\n', '\0', ModuleMyStyleTerms, 			0, '\t', "", "\t", "MyStyles", "MyStyles", "AfterStep MyStyle definitions", NULL, 0};
SyntaxDef AfterStepMyFrameSyntax   	={ '\n', '\0', AfterStepMyFrameTerms, 		0, '\t', "", "\t", "MyFrames", "MyFrames", "AfterStep MyFrame definitions", NULL, 0};
SyntaxDef AfterStepMyBackSyntax    	={ '\n', '\0', AfterStepMyBackgroundTerms,	0, '\t', "", "\t", "MyBackgrounds", "MyBackgrounds", "AfterStep MyBackground definitions", NULL, 0};
SyntaxDef AfterStepTitleButtonSyntax={ '\n', '\0', AfterStepTitleButtonTerms,	0, '\t', "", "\t", "TitleButtons", "TitleButtons", "AfterStep TitleButton definitions", NULL, 0};

SyntaxDef AfterStepFeelSyntax 		={ '\n', '\0', AfterStepFeelTerms, 			0, '\t', "", "\t", "Feel", "Feel", "AfterStep feel configuration", NULL, 0};
SyntaxDef AfterStepCursorSyntax 	={ '\n', '\0', AfterStepCursorTerms,   		0, '\t', "", "\t", "Cursors", "Cursors", "AfterStep Cursor configuration", NULL, 0};
SyntaxDef AfterStepMouseSyntax 		={ '\n', '\0', MouseBindingTerms, 			0, '\t', "", "\t", "MouseBindings", "MouseBindings", "AfterStep MouseBinding configuration", NULL, 0};
SyntaxDef AfterStepKeySyntax 		={ '\n', '\0', KeyboardBindingTerms,   		0, '\t', "", "\t", "KeyboardBindings", "KeyboardBindings", "AfterStep KeyboardBinding configuration", NULL, 0};
SyntaxDef AfterStepWindowBoxSyntax 	={ '\n', '\0', AfterStepWindowBoxTerms,		0, '\t', "", "\t", "WindowBoxes", "WindowBoxes", "AfterStep WindowBox configuration", NULL, 0};


void LinkAfterStepConfig()
{
	int i = 0;
	TermDef *termsets[] = {includeTerms,	
						FeelTerms,	
						MouseBindingTerms,	
						KeyboardBindingTerms,	
						PopupTerms,	
						FunctionTerms,
						WharfTerms,
						NULL } ;
	while( termsets[i] ) 
	{
		TermDef *terms = termsets[i] ; 
		int k = 0 ; 
		while( terms[k].keyword != NULL ) 
		{
			if( terms[k].sub_syntax == &DummyFuncSyntax ) 
				terms[k].sub_syntax = pFuncSyntax ; 
			else if( terms[k].sub_syntax == &DummyPopupFuncSyntax ) 
				terms[k].sub_syntax = pPopupFuncSyntax ; 
			++k ;	
		}	 
		++i ;	
	}	 
}	 

