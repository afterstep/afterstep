/*
 * Copyright (c) 2003 Sasha Vasko <sasha at aftercode.net>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include "../configure.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/mystyle.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/balloon.h"

#include "afterconf.h"

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/

extern SyntaxDef AlignSyntax;

TermDef WinTabsTerms[] = {
	{0, "Geometry", 8, TT_GEOMETRY, WINTABS_Geometry_ID, NULL},
	{0, "MaxRows", 7, TT_INTEGER, WINLIST_MaxRows_ID, NULL},
	{0, "MaxColumns", 10, TT_INTEGER, WINTABS_MaxColumns_ID, NULL},
	{0, "MaxTabWidth", 11, TT_INTEGER, WINTABS_MaxTabWidth_ID, NULL},
	{0, "MinTabWidth", 11, TT_INTEGER, WINTABS_MinTabWidth_ID, NULL},
	{TF_INDEXED, "Pattern", 7, TT_QUOTED_TEXT, WINTABS_Pattern_ID, NULL},
	{TF_INDEXED, "ExcludePattern", 14, TT_QUOTED_TEXT,
	 WINTABS_ExcludePattern_ID, NULL},
	{0, "Align", 5, TT_FLAG, WINTABS_Align_ID, &AlignSyntax},
	{0, "Bevel", 5, TT_FLAG, WINTABS_Bevel_ID, &BevelSyntax},
	{0, "FBevel", 6, TT_FLAG, WINTABS_FBevel_ID, &BevelSyntax},
	{0, "UBevel", 6, TT_FLAG, WINTABS_UBevel_ID, &BevelSyntax},
	{0, "SBevel", 6, TT_FLAG, WINTABS_SBevel_ID, &BevelSyntax},
	{0, "FocusedBevel", 12, TT_FLAG, WINTABS_FBevel_ID, &BevelSyntax},
	{0, "StickyBevel", 10, TT_FLAG, WINTABS_SBevel_ID, &BevelSyntax},
	{0, "UnfocusedBevel", 14, TT_FLAG, WINTABS_UBevel_ID, &BevelSyntax},
	{0, "CompositionMethod", 17, TT_UINTEGER, WINTABS_CM_ID, NULL},
	{0, "FCompositionMethod", 18, TT_UINTEGER, WINTABS_FCM_ID, NULL},
	{0, "UCompositionMethod", 18, TT_UINTEGER, WINTABS_UCM_ID, NULL},
	{0, "SCompositionMethod", 18, TT_UINTEGER, WINTABS_SCM_ID, NULL},
	{0, "Spacing", 7, TT_UINTEGER, WINTABS_Spacing_ID, NULL},
	{0, "HSpacing", 8, TT_UINTEGER, WINTABS_HSpacing_ID, NULL},
	{0, "VSpacing", 8, TT_UINTEGER, WINTABS_VSpacing_ID, NULL},

	{0, "UnfocusedStyle", 14, TT_TEXT, WINTABS_UnfocusedStyle_ID, NULL},
	{0, "FocusedStyle", 12, TT_TEXT, WINTABS_FocusedStyle_ID, NULL},
	{0, "StickyStyle", 11, TT_TEXT, WINTABS_StickyStyle_ID, NULL},
	{0, "UseSkipList", 11, TT_FLAG, WINTABS_UseSkipList_ID, NULL},
	{0, "AllDesks", 8, TT_FLAG, WINTABS_AllDesks_ID, NULL},
	{0, "Title", 5, TT_TEXT, WINTABS_Title_ID, NULL},
	{0, "IconTitle", 9, TT_TEXT, WINTABS_IconTitle_ID, NULL},
	{0, "SkipTransients", 14, TT_FLAG, WINTABS_SkipTransients_ID, NULL},
	{0, "GroupNameSeparator", 18, TT_QUOTED_TEXT,
	 WINTABS_GroupNameSeparator_ID, NULL},
	{0, "GroupTabs", 9, TT_FLAG, WINTABS_GroupTabs_ID, NULL},

/* including MyStyles definitions processing */
	INCLUDE_MYSTYLE,

/* now special cases that should be processed by it's own handlers */
	BALLOON_TERMS,
	{0, NULL, 0, 0, 0}
};

SyntaxDef WinTabsSyntax = {
	'\n',
	'\0',
	WinTabsTerms,
	0,														/* use default hash size */
	' ',
	"",
	"\t",
	"Module:WinTabs",
	"WinTabs",
	"AfterStep module for arranging windows using tabs",
	NULL,
	0
};


WinTabsConfig *CreateWinTabsConfig ()
{
	WinTabsConfig *config =
			(WinTabsConfig *) safecalloc (1, sizeof (WinTabsConfig));

	init_asgeometry (&(config->geometry));
	config->gravity = NorthWestGravity;
	config->max_rows = 1;
	config->pattern_type = ASN_Name;
	config->name_aligment = ALIGN_CENTER;
	config->balloon_conf = NULL;
	config->h_spacing = DEFAULT_TBAR_HSPACING;
	config->v_spacing = DEFAULT_TBAR_VSPACING;
	config->fbevel = config->ubevel = config->sbevel = DEFAULT_TBAR_HILITE;

	return config;
}

void DestroyWinTabsConfig (WinTabsConfig * config)
{
	if (config->unfocused_style)
		free (config->unfocused_style);
	if (config->focused_style)
		free (config->focused_style);
	if (config->sticky_style)
		free (config->sticky_style);

	destroy_string (&(config->GroupNameSeparator));

	Destroy_balloonConfig (config->balloon_conf);
	DestroyFreeStorage (&(config->more_stuff));
	DestroyMyStyleDefinitions (&(config->style_defs));

	free (config);
}

void PrintWinTabsConfig (WinTabsConfig * config)
{
	fprintf (stderr, "WinTabsConfig = %p;\n", config);
	if (config == NULL)
		return;

	fprintf (stderr, "WinTabsConfig.UseSkipList = %s;\n",
					 get_flags (config->flags,
											WINTABS_UseSkipList) ? "True" : "False");
	fprintf (stderr, "WINTABSConfig.set_flags = 0x%lX;\n",
					 config->set_flags);
	fprintf (stderr, "WINTABSConfig.geometry.flags = 0x%X;\n",
					 config->geometry.flags);
	fprintf (stderr, "WINTABSConfig.geometry = %dx%d%+d%+d;\n",
					 config->geometry.width, config->geometry.height,
					 config->geometry.x, config->geometry.y);
	fprintf (stderr, "WINTABSConfig.gravity = %d;\n", config->gravity);
	fprintf (stderr, "WINTABSConfig.MaxRows = %d;\n", config->max_rows);
	fprintf (stderr, "WINTABSConfig.MaxColumns = %d;\n",
					 config->max_columns);
	fprintf (stderr, "WINTABSConfig.min_tab_width = %d;\n",
					 config->min_tab_width);
	fprintf (stderr, "WINTABSConfig.max_tab_width = %d;\n",
					 config->max_tab_width);

	fprintf (stderr, "WINTABSConfig.pattern_type = %d;\n", config->pattern_type);	/* 0, 1, 2, 3 */
	fprintf (stderr, "WINTABSConfig.pattern = \"%s\";\n", config->pattern);	/* 0, 1, 2, 3 */
	fprintf (stderr, "WINTABSConfig.name_aligment = %lX;\n",
					 config->name_aligment);
	fprintf (stderr, "WINTABSConfig.fbevel = %lX;\n", config->fbevel);
	fprintf (stderr, "WINTABSConfig.ubevel = %lX;\n", config->ubevel);
	fprintf (stderr, "WINTABSConfig.sbevel = %lX;\n", config->sbevel);
	fprintf (stderr, "WINTABSConfig.fcm = %d;\n", config->fcm);
	fprintf (stderr, "WINTABSConfig.ucm = %d;\n", config->ucm);
	fprintf (stderr, "WINTABSConfig.scm = %d;\n", config->scm);
	fprintf (stderr, "WINTABSConfig.h_spacing = %d;\n", config->h_spacing);
	fprintf (stderr, "WINTABSConfig.v_spacing = %d;\n", config->v_spacing);

	fprintf (stderr, "WINTABSConfig.unfocused_style = %p;\n",
					 config->unfocused_style);
	if (config->unfocused_style)
		fprintf (stderr, "WINTABSConfig.unfocused_style = \"%s\";\n",
						 config->unfocused_style);
	fprintf (stderr, "WINTABSConfig.focused_style = %p;\n",
					 config->focused_style);
	if (config->focused_style)
		fprintf (stderr, "WINTABSConfig.focused_style = \"%s\";\n",
						 config->focused_style);
	fprintf (stderr, "WINTABSConfig.sticky_style = %p;\n",
					 config->sticky_style);
	if (config->sticky_style)
		fprintf (stderr, "WINTABSConfig.sticky_style = \"%s\";\n",
						 config->sticky_style);
	if (config->title)
		fprintf (stderr, "WINTABSConfig.title = \"%s\";\n", config->title);
	if (config->icon_title)
		fprintf (stderr, "WINTABSConfig.icon_title = \"%s\";\n",
						 config->icon_title);
	fprintf (stderr, "WinTabsConfig.SkipTransients = %s;\n",
					 get_flags (config->flags,
											WINTABS_SkipTransients) ? "True" : "False");

	fprintf (stderr, "WinTabsConfig.GroupNameSeparator = \"%s\";\n",
					 config->GroupNameSeparator);

}

WinTabsConfig *ParseWinTabsOptions (const char *filename, char *myname)
{
	ConfigData cd;
	ConfigDef *ConfigReader;
	WinTabsConfig *config = CreateWinTabsConfig ();

	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem item;

	cd.filename = filename;
	ConfigReader =
			InitConfigReader (myname, &WinTabsSyntax, CDT_Filename, cd, NULL);
	if (!ConfigReader)
		return config;

	item.memory = NULL;
	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &Storage);

	/* getting rid of all the crap first */
	StorageCleanUp (&Storage, &(config->more_stuff), CF_DISABLED_OPTION);

	config->balloon_conf =
			Process_balloonOptions (Storage, NULL, BALLOON_ID_START);
	config->style_defs = free_storage2MyStyleDefinitionsList (Storage);

	for (pCurr = Storage; pCurr; pCurr = pCurr->next) {
		if (pCurr->term == NULL)
			continue;

		if (pCurr->term->type == TT_FLAG) {
			switch (pCurr->term->id) {
			case WINTABS_UseSkipList_ID:
				SET_CONFIG_FLAG (config->flags, config->set_flags,
												 WINTABS_UseSkipList);
				break;
			case WINTABS_AllDesks_ID:
				SET_CONFIG_FLAG (config->flags, config->set_flags,
												 WINTABS_AllDesks);
				break;
			case WINTABS_SkipTransients_ID:
				SET_CONFIG_FLAG (config->flags, config->set_flags,
												 WINTABS_SkipTransients);
				break;
			case WINTABS_GroupTabs_ID:
				SET_CONFIG_FLAG (config->flags, config->set_flags,
												 WINTABS_GroupTabs);
				break;
			case WINTABS_Align_ID:
				set_flags (config->set_flags, WINTABS_Align);
				config->name_aligment = ParseAlignOptions (pCurr->sub);
				break;
			case WINTABS_Bevel_ID:
				set_flags (config->set_flags, WINTABS_Bevel);
				config->ubevel = config->sbevel = config->fbevel =
						ParseBevelOptions (pCurr->sub);
				break;
			case WINTABS_FBevel_ID:
				set_flags (config->set_flags, WINTABS_FBevel);
				config->fbevel = ParseBevelOptions (pCurr->sub);
				break;
			case WINTABS_UBevel_ID:
				set_flags (config->set_flags, WINTABS_UBevel);
				config->ubevel = ParseBevelOptions (pCurr->sub);
				break;
			case WINTABS_SBevel_ID:
				set_flags (config->set_flags, WINTABS_SBevel);
				config->sbevel = ParseBevelOptions (pCurr->sub);
				break;
			}
		} else {
			if (!ReadConfigItem (&item, pCurr))
				continue;

			switch (pCurr->term->id) {
			case WINTABS_Geometry_ID:
				set_flags (config->set_flags, WINTABS_Geometry);
				config->geometry = item.data.geometry;
				break;
			case WINTABS_MaxRows_ID:
				set_flags (config->set_flags, WINTABS_MaxRows);
				config->max_rows = item.data.integer;
				break;
			case WINTABS_MaxColumns_ID:
				set_flags (config->set_flags, WINTABS_MaxColumns);
				config->max_columns = item.data.integer;
				break;
			case WINTABS_MinTabWidth_ID:
				set_flags (config->set_flags, WINTABS_MinTabWidth);
				config->min_tab_width = item.data.integer;
				break;
			case WINTABS_MaxTabWidth_ID:
				set_flags (config->set_flags, WINTABS_MaxTabWidth);
				config->max_tab_width = item.data.integer;
				break;
			case WINTABS_Pattern_ID:
				config->pattern_type = item.index % ASN_NameTypes;
				LOCAL_DEBUG_OUT ("old_pattern = %p, new pattern = %s",
												 config->pattern, item.data.string);
				set_string_value (&(config->pattern), item.data.string,
													&(config->set_flags), WINTABS_PatternType);
				break;
			case WINTABS_ExcludePattern_ID:
				config->exclude_pattern_type = item.index % ASN_NameTypes;
				LOCAL_DEBUG_OUT ("old_excl_pattern = %p, new excl_pattern = %s",
												 config->exclude_pattern, item.data.string);
				set_string_value (&(config->exclude_pattern), item.data.string,
													&(config->set_flags),
													WINTABS_ExcludePatternType);
				break;
			case WINTABS_UnfocusedStyle_ID:
				REPLACE_STRING (config->unfocused_style, item.data.string);
				break;
			case WINTABS_FocusedStyle_ID:
				REPLACE_STRING (config->focused_style, item.data.string);
				break;
			case WINTABS_StickyStyle_ID:
				REPLACE_STRING (config->sticky_style, item.data.string);
				break;
			case WINTABS_CM_ID:
				set_flags (config->set_flags, WINTABS_CM);
				config->ucm = config->scm = config->fcm = item.data.integer;
				break;
			case WINTABS_FCM_ID:
				set_flags (config->set_flags, WINTABS_FCM);
				config->fcm = item.data.integer;
				break;
			case WINTABS_UCM_ID:
				set_flags (config->set_flags, WINTABS_UCM);
				config->ucm = item.data.integer;
				break;
			case WINTABS_SCM_ID:
				set_flags (config->set_flags, WINTABS_SCM);
				config->scm = item.data.integer;
				break;
			case WINTABS_Spacing_ID:
				set_flags (config->set_flags,
									 WINTABS_H_SPACING | WINTABS_V_SPACING);
				config->h_spacing = config->v_spacing = item.data.integer;
				break;
			case WINTABS_HSpacing_ID:
				set_flags (config->set_flags, WINTABS_H_SPACING);
				config->h_spacing = item.data.integer;
				break;
			case WINTABS_VSpacing_ID:
				set_flags (config->set_flags, WINTABS_V_SPACING);
				config->v_spacing = item.data.integer;
				break;
			case WINTABS_Title_ID:
				set_string (&(config->title), item.data.string);
				break;
			case WINTABS_IconTitle_ID:
				set_string (&(config->icon_title), item.data.string);
				break;

			case WINTABS_GroupNameSeparator_ID:
				REPLACE_STRING (config->GroupNameSeparator, item.data.string);
				break;

			default:
				item.ok_to_free = 1;
			}
		}
	}
	ReadConfigItem (&item, NULL);
	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);

	return config;
}
