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


TermDef       TbarLayoutTerms[] = {
	{TF_NO_MYNAME_PREPENDING, "Buttons", 7, TT_FLAG, TBAR_LAYOUT_Buttons_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "Spacer", 6, TT_FLAG, TBAR_LAYOUT_Spacer_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "TitleSpacer", 11, TT_FLAG, TBAR_LAYOUT_TitleSpacer_ID, NULL}
	,
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
	"Look Titlebar Layout Flags",
	"TbarLayout",
	"",
	NULL,
	0
};

struct SyntaxDef *TbarLayoutSyntaxPtr = &TbarLayoutSyntax;


TermDef       MyFrameTerms[] = {
	{TF_NO_MYNAME_PREPENDING | TF_SYNTAX_START, "MyFrame", 7, TT_QUOTED_TEXT, MYFRAME_START_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "North", 5, TT_OPTIONAL_PATHNAME, MYFRAME_North_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "East", 4, TT_OPTIONAL_PATHNAME, MYFRAME_East_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "South", 5, TT_OPTIONAL_PATHNAME, MYFRAME_South_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "West", 4, TT_OPTIONAL_PATHNAME, MYFRAME_West_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "NorthWest", 9, TT_OPTIONAL_PATHNAME, MYFRAME_NorthWest_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "NorthEast", 9, TT_OPTIONAL_PATHNAME, MYFRAME_NorthEast_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "SouthWest", 9, TT_OPTIONAL_PATHNAME, MYFRAME_SouthWest_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "SouthEast", 9, TT_OPTIONAL_PATHNAME, MYFRAME_SouthEast_ID, NULL},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED, "Side", 4, TT_OPTIONAL_PATHNAME, MYFRAME_Side_ID, NULL},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED, "NoSide", 6, TT_FLAG, MYFRAME_NoSide_ID, NULL},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED, "Corner", 6, TT_OPTIONAL_PATHNAME, MYFRAME_Corner_ID, NULL},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED, "NoCorner", 8, TT_FLAG, MYFRAME_NoCorner_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleUnfocusedStyle", 19, TT_QUOTED_TEXT, MYFRAME_TitleUnfocusedStyle_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleUStyle", 11, TT_QUOTED_TEXT, MYFRAME_TitleUnfocusedStyle_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleFocusedStyle", 17, TT_QUOTED_TEXT, MYFRAME_TitleFocusedStyle_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleFStyle", 11, TT_QUOTED_TEXT, MYFRAME_TitleFocusedStyle_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleStickyStyle", 16, TT_QUOTED_TEXT, MYFRAME_TitleStickyStyle_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleSStyle", 11, TT_QUOTED_TEXT, MYFRAME_TitleStickyStyle_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "FrameUnfocusedStyle", 19, TT_QUOTED_TEXT, MYFRAME_FrameUnfocusedStyle_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "FrameUStyle", 11, TT_QUOTED_TEXT, MYFRAME_FrameUnfocusedStyle_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "FrameFocusedStyle", 17, TT_QUOTED_TEXT, MYFRAME_FrameFocusedStyle_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "FrameFStyle", 11, TT_QUOTED_TEXT, MYFRAME_FrameFocusedStyle_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "FrameStickyStyle", 16, TT_QUOTED_TEXT, MYFRAME_FrameStickyStyle_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "FrameSStyle", 11, TT_QUOTED_TEXT, MYFRAME_FrameStickyStyle_ID, NULL},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED, "SideSize", 8, TT_GEOMETRY, MYFRAME_SideSize_ID, NULL},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "SideAlign", 9, TT_FLAG, MYFRAME_SideAlign_ID,
	 &AlignSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "SideBevel", 9, TT_FLAG, MYFRAME_SideBevel_ID,
	 &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "SideFBevel", 10, TT_FLAG, MYFRAME_SideFBevel_ID,
	 &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "SideUBevel", 10, TT_FLAG, MYFRAME_SideUBevel_ID,
	 &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "SideSBevel", 10, TT_FLAG, MYFRAME_SideSBevel_ID,
	 &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "SideFocusedBevel", 16, TT_FLAG, MYFRAME_SideFBevel_ID,
	 &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "SideUnfocusedBevel", 18, TT_FLAG,
	 MYFRAME_SideUBevel_ID, &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "SideStickyBevel", 15, TT_FLAG, MYFRAME_SideSBevel_ID,
	 &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED, "CornerSize", 10, TT_GEOMETRY, MYFRAME_CornerSize_ID, NULL},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "CornerAlign", 11, TT_FLAG, MYFRAME_CornerAlign_ID,
	 &AlignSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "CornerBevel", 11, TT_FLAG, MYFRAME_CornerBevel_ID,
	 &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "CornerFBevel", 12, TT_FLAG, MYFRAME_CornerFBevel_ID,
	 &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "CornerUBevel", 12, TT_FLAG, MYFRAME_CornerUBevel_ID,
	 &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "CornerSBevel", 12, TT_FLAG, MYFRAME_CornerSBevel_ID,
	 &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "CornerFocusedBevel", 18, TT_FLAG,
	 MYFRAME_CornerFBevel_ID, &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "CornerUnfocusedBevel", 20, TT_FLAG,
	 MYFRAME_CornerUBevel_ID, &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "CornerStickyBevel", 17, TT_FLAG,
	 MYFRAME_CornerSBevel_ID, &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING, "TitleBevel", 10, TT_FLAG, MYFRAME_TitleBevel_ID, &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING, "TitleFBevel", 11, TT_FLAG, MYFRAME_TitleFBevel_ID, &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING, "TitleUBevel", 11, TT_FLAG, MYFRAME_TitleUBevel_ID, &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING, "TitleSBevel", 11, TT_FLAG, MYFRAME_TitleSBevel_ID, &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING, "TitleFocusedBevel", 17, TT_FLAG, MYFRAME_TitleFBevel_ID, &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING, "TitleUnfocusedBevel", 19, TT_FLAG, MYFRAME_TitleUBevel_ID, &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING, "TitleStickyBevel", 16, TT_FLAG, MYFRAME_TitleSBevel_ID, &BevelSyntax},
	{TF_NO_MYNAME_PREPENDING, "TitleAlign", 10, TT_FLAG, MYFRAME_TitleAlign_ID, &AlignSyntax},
	{TF_NO_MYNAME_PREPENDING, "TitleCompositionMethod", 22, TT_UINTEGER, MYFRAME_TitleCM_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleFCompositionMethod", 22, TT_UINTEGER, MYFRAME_TitleFCM_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleUCompositionMethod", 22, TT_UINTEGER, MYFRAME_TitleUCM_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleSCompositionMethod", 22, TT_UINTEGER, MYFRAME_TitleSCM_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleFocusedCompositionMethod", 28, TT_UINTEGER, MYFRAME_TitleFCM_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleUnfocusedCompositionMethod", 30, TT_UINTEGER, MYFRAME_TitleUCM_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleStickyCompositionMethod", 27, TT_UINTEGER, MYFRAME_TitleSCM_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleFHue", 9, TT_COLOR, MYFRAME_TitleFHue_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleUHue", 9, TT_COLOR, MYFRAME_TitleUHue_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleSHue", 9, TT_COLOR, MYFRAME_TitleSHue_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleFocusedHue", 15, TT_COLOR, MYFRAME_TitleFHue_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleUnfocusedHue", 17, TT_COLOR, MYFRAME_TitleUHue_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleStickyHue", 14, TT_COLOR, MYFRAME_TitleSHue_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleFSaturation", 16, TT_UINTEGER, MYFRAME_TitleFSat_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleUSaturation", 16, TT_UINTEGER, MYFRAME_TitleUSat_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleSSaturation", 16, TT_UINTEGER, MYFRAME_TitleSSat_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleFocusedSaturation", 22, TT_UINTEGER, MYFRAME_TitleFSat_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleUnfocusedSaturation", 24, TT_UINTEGER, MYFRAME_TitleUSat_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleStickySaturation", 21, TT_UINTEGER, MYFRAME_TitleSSat_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleHSpacing", 13, TT_INTEGER, MYFRAME_TitleHSpacing_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleVSpacing", 13, TT_INTEGER, MYFRAME_TitleVSpacing_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "InheritDefaults", 15, TT_FLAG, MYFRAME_InheritDefaults_ID, NULL},
	{TF_NO_MYNAME_PREPENDING | TF_NONUNIQUE, "Inherit", 7, TT_QUOTED_TEXT, MYFRAME_Inherit_ID, NULL},
	{TF_NO_MYNAME_PREPENDING | TF_SYNTAX_TERMINATOR, "~MyFrame", 8, TT_FLAG, MYFRAME_DONE_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "LeftBtnBackground", 17, TT_FILENAME, MYFRAME_LeftBtnBackground_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "LeftSpacerBackground", 20, TT_FILENAME, MYFRAME_LeftSpacerBackground_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "LeftTitleSpacerBackground", 25, TT_FILENAME, MYFRAME_LTitleSpacerBackground_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleBackground", 15, TT_FILENAME, MYFRAME_TitleBackground_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "RightTitleSpacerBackground", 26, TT_FILENAME, MYFRAME_RTitleSpacerBackground_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "RightSpacerBackground", 21, TT_FILENAME, MYFRAME_RightSpacerBackground_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "RightBtnBackground", 18, TT_FILENAME, MYFRAME_RightBtnBackground_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "LeftBtnBackAlign", 16, TT_FLAG, MYFRAME_LeftBtnBackAlign_ID, &AlignSyntax},
	{TF_NO_MYNAME_PREPENDING, "LeftSpacerBackAlign", 19, TT_FLAG, MYFRAME_LeftSpacerBackAlign_ID, &AlignSyntax},
	{TF_NO_MYNAME_PREPENDING, "LeftTitleSpacerBackAlign", 24, TT_FLAG, MYFRAME_LTitleSpacerBackAlign_ID, &AlignSyntax},
	{TF_NO_MYNAME_PREPENDING, "TitleBackgroundAlign", 20, TT_FLAG, MYFRAME_TitleBackgroundAlign_ID, &AlignSyntax},
	{TF_NO_MYNAME_PREPENDING, "RightTitleSpacerBackAlign", 25, TT_FLAG, MYFRAME_RTitleSpacerBackAlign_ID, &AlignSyntax},
	{TF_NO_MYNAME_PREPENDING, "RightSpacerBackAlign", 20, TT_FLAG, MYFRAME_RightSpacerBackAlign_ID, &AlignSyntax},
	{TF_NO_MYNAME_PREPENDING, "RightBtnBackAlign", 17, TT_FLAG, MYFRAME_RightBtnBackAlign_ID, &AlignSyntax},
	{TF_NO_MYNAME_PREPENDING, "CondenseTitlebar", 16, TT_FLAG, MYFRAME_CondenseTitlebar_ID, &AlignSyntax},
	{TF_NO_MYNAME_PREPENDING, "LeftTitlebarLayout", 18, TT_FLAG, MYFRAME_LeftTitlebarLayout_ID, &TbarLayoutSyntax},
	{TF_NO_MYNAME_PREPENDING, "RightTitlebarLayout", 19, TT_FLAG, MYFRAME_RightTitlebarLayout_ID, &TbarLayoutSyntax},
	{TF_NO_MYNAME_PREPENDING, "NoBorder", 8, TT_FLAG, MYFRAME_NoBorder_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "AllowBorder", 11, TT_FLAG, MYFRAME_AllowBorder_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "LeftBtnAlign", 12, TT_FLAG, MYFRAME_LeftBtnAlign_ID, &AlignSyntax},
	{TF_NO_MYNAME_PREPENDING, "RightBtnAlign", 13, TT_FLAG, MYFRAME_RightBtnAlign_ID, &AlignSyntax},
	{TF_NO_MYNAME_PREPENDING | TF_DIRECTION_INDEXED | TF_NAMED, "SideSlicing", 11, TT_GEOMETRY, MYFRAME_SideSlicing_ID,
	 NULL},
	{TF_NO_MYNAME_PREPENDING, "LeftBtnBackSlicing", 18, TT_GEOMETRY, MYFRAME_LeftBtnBackSlicing_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "LeftSpacerBackSlicing", 21, TT_GEOMETRY, MYFRAME_LeftSpacerBackSlicing_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "LeftTitleSpacerBackSlicing", 26, TT_GEOMETRY, MYFRAME_LTitleSpacerBackSlicing_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "TitleBackgroundSlicing", 22, TT_GEOMETRY, MYFRAME_TitleBackSlicing_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "RightTitleSpacerBackSlicing", 27, TT_GEOMETRY, MYFRAME_RTitleSpacerBackSlicing_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "RightSpacerBackSlicing", 22, TT_GEOMETRY, MYFRAME_RightSpacerBackSlicing_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "RightBtnBackSlicing", 19, TT_GEOMETRY, MYFRAME_RightBtnBackSlicing_ID, NULL},

	{0, NULL, 0, 0, 0}
};

SyntaxDef     MyFrameSyntax = {
	'\n',
	'\0',
	MyFrameTerms,
	7,										   /* hash size */
	'\t',
	"",
	"\t",
	"Look MyFrame",
	"MyFrame",
	"defines how AfterStep should construct window's frame",
	NULL,
	0
};

const char   *
frame_side_idx2name (int idx)
{
	if (idx >= 0 && idx < FRAME_PARTS)
		return MyFrameTerms[1 + idx].keyword;
	return NULL;
}

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

		for (i = 0; i < BACK_STYLES; ++i)
		{
			if (fd->title_styles[i])
				free (fd->title_styles[i]);
			if (fd->frame_styles[i])
				free (fd->frame_styles[i]);
		}
		for (i = 0; i < MYFRAME_TITLE_BACKS; ++i)
		{
			if (fd->title_backs[i])
				free (fd->title_backs[i]);
		}
		if (fd->inheritance_list)
		{
			i = fd->inheritance_num - 1;
			while (--i >= 0)
				if (fd->inheritance_list[i])
					free (fd->inheritance_list[i]);
			free (fd->inheritance_list);
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
		for (i = 0; i < BACK_STYLES; ++i)
			if (list->title_styles[i])
				fprintf (stderr, "MyFrame[%d].TitleStyle[%d] = \"%s\";\n", index, i, list->title_styles[i]);
		for (i = 0; i < BACK_STYLES; ++i)
			if (list->frame_styles[i])
				fprintf (stderr, "MyFrame[%d].FrameStyle[%d] = \"%s\";\n", index, i, list->frame_styles[i]);
		for (i = 0; i < MYFRAME_TITLE_BACKS; ++i)
		{
			if (list->title_backs[i])
				fprintf (stderr, "MyFrame[%d].TitleBack[%d] = \"%s\";\n", index, i, list->title_backs[i]);
		}
		fprintf (stderr, "MyFrame[%d].set_part_size = 0x%lX;\n", index, list->set_part_size);
		fprintf (stderr, "MyFrame[%d].set_part_bevel = 0x%lX;\n", index, list->set_part_bevel);
		fprintf (stderr, "MyFrame[%d].set_part_align = 0x%lX;\n", index, list->set_part_align);
		for (i = 0; i < FRAME_PARTS; ++i)
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
		for (i = 0; i < MYFRAME_TITLE_BACKS; ++i)
		{
			fprintf (stderr, "MyFrame[%d].title_backs_align[%d] = 0x%lX;\n", index, i, list->title_backs_align[i]);
		}
		for (i = 0; i < MYFRAME_TITLE_BACKS; ++i)
		{
			if (list->title_backs_slicing[i].flags != 0)
				fprintf (stderr, "MyFrame[%d].title_backs_slicing[%d] = 0x%X, %dx%d%+d%+d;\n",
						 index, i, list->title_backs_slicing[i].flags,
						 list->title_backs_slicing[i].width, list->title_backs_slicing[i].height,
						 list->title_backs_slicing[i].x, list->title_backs_slicing[i].y);
		}
		fprintf (stderr, "MyFrame[%d].title_fcm = %d;\n", index, list->title_fcm);
		fprintf (stderr, "MyFrame[%d].title_ucm = %d;\n", index, list->title_ucm);
		fprintf (stderr, "MyFrame[%d].title_scm = %d;\n", index, list->title_scm);
		for (i = 0; i < FRAME_SIDES; ++i)
			if (list->side_slicing[i].flags != 0)
				fprintf (stderr, "MyFrame[%d].side_slicing[%d] = 0x%X, %dx%d%+d%+d;\n",
						 index, i, list->side_slicing[i].flags,
						 list->side_slicing[i].width, list->side_slicing[i].height,
						 list->side_slicing[i].x, list->side_slicing[i].y);

		if (list->inheritance_list)
			for (i = 0; i < list->inheritance_num; ++i)
				fprintf (stderr, "MyFrame[%d].Inherit[%d] = \"%s\";\n", index, i, list->inheritance_list[i]);
		PrintMyFrameDefinitions (list->next, index + 1);
	}
}


unsigned long
ParseTbarLayoutOptions (FreeStorageElem * options)
{
	unsigned long layout = 0;
	int           count = 0;

	while (options)
	{
		LOCAL_DEBUG_OUT ("options(%p)->keyword(\"%s\")", options, options->term->keyword);
		if (options->term != NULL)
		{
			int           elem = options->term->id - TBAR_LAYOUT_ID_START;
			int           i;

			if (elem >= 0 && elem <= MYFRAME_TITLE_SIDE_ELEMS)
			{
				for (i = 0; i < count; ++i)
					if (MYFRAME_GetTbarLayoutElem (layout, i) == elem)
						break;
				if (i >= count)
				{
					MYFRAME_SetTbarLayoutElem (layout, count, elem);
					++count;
				}
			}
		}
		options = options->next;
	}
	while (count < MYFRAME_TITLE_SIDE_ELEMS)
	{
		MYFRAME_SetTbarLayoutElem (layout, count, MYFRAME_TITLE_BACK_INVALID);
		++count;
	}
	return layout;
}

void
tbar_layout_parse (char *tline, FILE * fd, char **myname, int *playout)
{
	FreeStorageElem *Storage = tline_subsyntax_parse (NULL, tline, fd, (char *)myname, &BevelSyntax, NULL, NULL);

	if (Storage)
	{
		*playout = ParseTbarLayoutOptions (Storage);
		DestroyFreeStorage (&Storage);
	}
}


MyFrameDefinition **
ProcessMyFrameOptions (FreeStorageElem * options, MyFrameDefinition ** tail)
{
	ConfigItem    item;
	int           rel_id;
	MyFrameDefinition *fd = NULL;

	item.memory = NULL;

	for (; options; options = options->next)
	{
		if (options->term == NULL)
			continue;
		LOCAL_DEBUG_OUT ("MyFrame(%p)->options(%p)->keyword(\"%s\")", fd, options, options->term->keyword);
		if (options->term->id < MYFRAME_ID_START || options->term->id > MYFRAME_ID_END)
			continue;

		if (options->term->type == TT_FLAG)
		{
			if (fd != NULL)
			{
				if (options->term->id == MYFRAME_DONE_ID)
					break;					   /* for loop */
				if (options->term->id == MYFRAME_InheritDefaults_ID)
				{
					set_flags (fd->flags, MYFRAME_INHERIT_DEFAULTS);
					set_flags (fd->set_flags, MYFRAME_INHERIT_DEFAULTS);
					continue;
				}
				if (options->term->id >= MYFRAME_TitleBackgroundAlign_ID_START)
				{
					int           index = options->term->id - MYFRAME_TitleBackgroundAlign_ID_START;

					if (index < MYFRAME_TITLE_BACKS)
					{
						fd->title_backs_align[index] = ParseAlignOptions (options->sub);
						set_flags (fd->set_title_attr, MYFRAME_TitleBackAlignSet_Start << index);
						continue;
					}
				}
				if (options->term->id >= MYFRAME_TitleBackSlicing_ID_START)
				{
					int           index = options->term->id - MYFRAME_TitleBackSlicing_ID_START;

					if (index < MYFRAME_TITLE_BACKS)
					{
						if (!ReadConfigItem (&item, options))
							continue;
						fd->title_backs_slicing[index] = item.data.geometry;
					}
				}
				switch (options->term->id)
				{
				 case MYFRAME_TitleBevel_ID:
					 fd->title_fbevel = ParseBevelOptions (options->sub);
					 fd->title_ubevel = fd->title_fbevel;
					 fd->title_sbevel = fd->title_fbevel;
					 set_flags (fd->set_title_attr, MYFRAME_TitleBevelSet);
					 break;
				 case MYFRAME_TitleFBevel_ID:
					 fd->title_fbevel = ParseBevelOptions (options->sub);
					 set_flags (fd->set_title_attr, MYFRAME_TitleBevelSet);
					 break;
				 case MYFRAME_TitleUBevel_ID:
					 fd->title_ubevel = ParseBevelOptions (options->sub);
					 set_flags (fd->set_title_attr, MYFRAME_TitleBevelSet);
					 break;
				 case MYFRAME_TitleSBevel_ID:
					 fd->title_sbevel = ParseBevelOptions (options->sub);
					 set_flags (fd->set_title_attr, MYFRAME_TitleBevelSet);
					 break;
				 case MYFRAME_TitleAlign_ID:
					 fd->title_align = ParseAlignOptions (options->sub);
					 set_flags (fd->set_title_attr, MYFRAME_TitleAlignSet);
					 break;
				 case MYFRAME_LeftBtnAlign_ID:
					 fd->left_btn_align = ParseAlignOptions (options->sub);
					 set_flags (fd->set_title_attr, MYFRAME_LeftBtnAlignSet);
					 break;
				 case MYFRAME_RightBtnAlign_ID:
					 fd->right_btn_align = ParseAlignOptions (options->sub);
					 set_flags (fd->set_title_attr, MYFRAME_RightBtnAlignSet);
					 break;
				 case MYFRAME_CondenseTitlebar_ID:
					 fd->condense_titlebar = ParseAlignOptions (options->sub);
					 set_flags (fd->set_title_attr, MYFRAME_CondenseTitlebarSet);
					 break;
				 case MYFRAME_LeftTitlebarLayout_ID:
					 fd->left_layout = ParseTbarLayoutOptions (options->sub);
					 set_flags (fd->set_title_attr, MYFRAME_LeftTitlebarLayoutSet);
					 break;
				 case MYFRAME_RightTitlebarLayout_ID:
					 fd->right_layout = ParseTbarLayoutOptions (options->sub);
					 set_flags (fd->set_title_attr, MYFRAME_RightTitlebarLayoutSet);
					 break;
				 case MYFRAME_NoBorder_ID:
					 set_flags (fd->flags, MYFRAME_NOBORDER);
					 set_flags (fd->set_flags, MYFRAME_NOBORDER);
					 break;
				 case MYFRAME_AllowBorder_ID:
					 clear_flags (fd->flags, MYFRAME_NOBORDER);
					 set_flags (fd->set_flags, MYFRAME_NOBORDER);
					 break;
				 default:
					 if (!ReadConfigItem (&item, options))
						 continue;
					 LOCAL_DEBUG_OUT ("item.index = %d", item.index);
					 if (item.index < 0 || item.index >= FRAME_PARTS)
					 {
						 item.ok_to_free = 1;
						 continue;
					 }
					 switch (options->term->id)
					 {
					  case MYFRAME_NoSide_ID:
					  case MYFRAME_NoCorner_ID:
						  clear_flags (fd->parts_mask, 0x01 << item.index);
						  set_flags (fd->set_parts, 0x01 << item.index);
						  break;
					  case MYFRAME_SideAlign_ID:
					  case MYFRAME_CornerAlign_ID:
						  fd->part_align[item.index] = ParseAlignOptions (options->sub);
						  set_flags (fd->set_part_align, 0x01 << item.index);
						  break;
					  case MYFRAME_SideBevel_ID:
					  case MYFRAME_CornerBevel_ID:
						  fd->part_fbevel[item.index] = ParseBevelOptions (options->sub);
						  fd->part_ubevel[item.index] = fd->part_fbevel[item.index];
						  fd->part_sbevel[item.index] = fd->part_fbevel[item.index];
						  SetPartFBevelSet (fd, item.index);
						  SetPartUBevelSet (fd, item.index);
						  SetPartSBevelSet (fd, item.index);
						  break;
					  case MYFRAME_SideFBevel_ID:
					  case MYFRAME_CornerFBevel_ID:
						  fd->part_fbevel[item.index] = ParseBevelOptions (options->sub);
						  SetPartFBevelSet (fd, item.index);
						  break;
					  case MYFRAME_SideUBevel_ID:
					  case MYFRAME_CornerUBevel_ID:
						  fd->part_ubevel[item.index] = ParseBevelOptions (options->sub);
						  SetPartUBevelSet (fd, item.index);
						  break;
					  case MYFRAME_SideSBevel_ID:
					  case MYFRAME_CornerSBevel_ID:
						  fd->part_sbevel[item.index] = ParseBevelOptions (options->sub);
						  SetPartSBevelSet (fd, item.index);
						  break;
					  case MYFRAME_SideSlicing_ID:
						  fd->side_slicing[item.index] = item.data.geometry;
						  break;
					  default:
						  show_warning ("Unexpected MyFrame definition keyword \"%s\" . Ignoring.",
										options->term->keyword);
						  item.ok_to_free = 1;
					 }
				}
			}
			continue;
		}

		if (!ReadConfigItem (&item, options))
		{
			LOCAL_DEBUG_OUT ("ReadConfigItem has failed%s", "");
			continue;
		}
		if (*tail == NULL)
			fd = AddMyFrameDefinition (tail);

		rel_id = options->term->id - MYFRAME_START_ID;
		if (fd)
		{
			if (rel_id == 0)
			{
				if (fd->name != NULL)
				{
					show_error
						("MyFrame \"%s\": Previous MyFrame definition [%s] was not terminated correctly, and will be ignored.",
						 item.data.string, fd->name);
					DestroyMyFrameDefinitions (tail);
					fd = AddMyFrameDefinition (tail);
				}
				set_string (&(fd->name), item.data.string);
			} else if (rel_id <= FRAME_PARTS)
			{
				--rel_id;
				set_string_value (&(fd->parts[rel_id]), item.data.string, &(fd->set_parts), (0x01 << rel_id));
				set_flags (fd->parts_mask, (0x01 << rel_id));
			} else
			{
				rel_id = item.index;
				LOCAL_DEBUG_OUT ("item.index = %d", item.index);
				if (rel_id < 0 || rel_id >= FRAME_PARTS)
					rel_id = 0;
				switch (options->term->id)
				{
				 case MYFRAME_Side_ID:
				 case MYFRAME_Corner_ID:
					 set_string_value (&(fd->parts[rel_id]), item.data.string, &(fd->set_parts), (0x01 << rel_id));
					 set_flags (fd->parts_mask, (0x01 << rel_id));
					 break;
				 case MYFRAME_TitleUnfocusedStyle_ID:
					 set_string (&(fd->title_styles[BACK_UNFOCUSED]), item.data.string);
					 break;
				 case MYFRAME_TitleFocusedStyle_ID:
					 set_string (&(fd->title_styles[BACK_FOCUSED]), item.data.string);
					 break;
				 case MYFRAME_TitleStickyStyle_ID:
					 set_string (&(fd->title_styles[BACK_STICKY]), item.data.string);
					 break;
				 case MYFRAME_FrameUnfocusedStyle_ID:
					 set_string (&(fd->frame_styles[BACK_UNFOCUSED]), item.data.string);
					 break;
				 case MYFRAME_FrameFocusedStyle_ID:
					 set_string (&(fd->frame_styles[BACK_FOCUSED]), item.data.string);
					 break;
				 case MYFRAME_FrameStickyStyle_ID:
					 set_string (&(fd->frame_styles[BACK_STICKY]), item.data.string);
					 break;
				 case MYFRAME_SideSize_ID:
				 case MYFRAME_CornerSize_ID:
					 if (get_flags (item.data.geometry.flags, WidthValue))
						 fd->part_width[rel_id] = item.data.geometry.width;
					 else
						 fd->part_width[rel_id] = 0;
					 if (get_flags (item.data.geometry.flags, HeightValue))
						 fd->part_length[rel_id] = item.data.geometry.height;
					 else
						 fd->part_length[rel_id] = 0;
					 set_flags (fd->set_part_size, (0x01 << rel_id));
					 break;
				 case MYFRAME_TitleCM_ID:
					 fd->title_fcm = item.data.integer;
					 fd->title_ucm = item.data.integer;
					 fd->title_scm = item.data.integer;
					 set_flags (fd->set_title_attr, MYFRAME_TitleCMSet);
					 break;
				 case MYFRAME_TitleFCM_ID:
					 fd->title_fcm = item.data.integer;
					 set_flags (fd->set_title_attr, MYFRAME_TitleFCMSet);
					 break;
				 case MYFRAME_TitleUCM_ID:
					 fd->title_ucm = item.data.integer;
					 set_flags (fd->set_title_attr, MYFRAME_TitleUCMSet);
					 break;
				 case MYFRAME_TitleSCM_ID:
					 fd->title_scm = item.data.integer;
					 set_flags (fd->set_title_attr, MYFRAME_TitleSCMSet);
					 break;
				 case MYFRAME_TitleFHue_ID:
					 set_string_value (&(fd->title_fhue), item.data.string, &(fd->set_title_attr),
									   MYFRAME_TitleFHueSet);
					 break;
				 case MYFRAME_TitleUHue_ID:
					 set_string_value (&(fd->title_uhue), item.data.string, &(fd->set_title_attr),
									   MYFRAME_TitleUHueSet);
					 break;
				 case MYFRAME_TitleSHue_ID:
					 set_string_value (&(fd->title_shue), item.data.string, &(fd->set_title_attr),
									   MYFRAME_TitleSHueSet);
					 break;
				 case MYFRAME_TitleFSat_ID:
					 fd->title_fsat = item.data.integer;
					 set_flags (fd->set_title_attr, MYFRAME_TitleFSatSet);
					 break;
				 case MYFRAME_TitleUSat_ID:
					 fd->title_usat = item.data.integer;
					 set_flags (fd->set_title_attr, MYFRAME_TitleUSatSet);
					 break;
				 case MYFRAME_TitleSSat_ID:
					 fd->title_ssat = item.data.integer;
					 set_flags (fd->set_title_attr, MYFRAME_TitleSSatSet);
					 break;
				 case MYFRAME_TitleHSpacing_ID:
					 fd->title_h_spacing = item.data.integer;
					 set_flags (fd->set_title_attr, MYFRAME_TitleHSpacingSet);
					 break;
				 case MYFRAME_TitleVSpacing_ID:
					 fd->title_v_spacing = item.data.integer;
					 set_flags (fd->set_title_attr, MYFRAME_TitleVSpacingSet);
					 break;

				 case MYFRAME_Inherit_ID:
					 {
						 fd->inheritance_list =
							 realloc (fd->inheritance_list, (fd->inheritance_num + 1) * sizeof (char *));
						 fd->inheritance_list[fd->inheritance_num] = item.data.string;
						 ++(fd->inheritance_num);
					 }
					 break;
				 default:
					 {
						 int           index = options->term->id - MYFRAME_TitleBackground_ID_START;

						 if (index >= 0 && index < MYFRAME_TITLE_BACKS)
						 {
							 set_string (&(fd->title_backs[index]), item.data.string);
						 } else
						 {
							 item.ok_to_free = 1;
							 show_warning ("Unexpected MyFrame definition keyword \"%s\" . Ignoring.",
										   options->term->keyword);
						 }
					 }
				}
			}
		}
	}
	if (fd != NULL)
	{
		if (fd->name == NULL)
		{
			show_warning ("MyFrame with no name encoutered. Discarding.");
			DestroyMyFrameDefinitions (tail);
		} else
		{
			LOCAL_DEBUG_OUT ("done parsing myframe: [%s]", fd->name);
			while (*tail)
				tail = &((*tail)->next);	   /* just in case */
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
	FreeStorageElem *Storage = NULL;
	MyFrameDefinition **list = (MyFrameDefinition **) myframe_list, **tail;

	if (list == NULL)
		return;
	for (tail = list; *tail != NULL; tail = &((*tail)->next));

	Storage = tline_subsyntax_parse ("MyFrame", tline, fd, (char *)myname, &MyFrameSyntax, NULL, NULL);
	if (Storage)
	{
		ProcessMyFrameOptions (Storage, tail);
		DestroyFreeStorage (&Storage);
	}
}
