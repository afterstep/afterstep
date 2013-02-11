/*
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

TermDef       PagerDecorationTerms[] = {
	{TF_NO_MYNAME_PREPENDING, "NoDeskLabel", 11, TT_FLAG, PAGER_DECOR_NOLABEL_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "NoPageSeparator", 15, TT_FLAG, PAGER_DECOR_NOSEPARATOR_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "NoSelection", 11, TT_FLAG, PAGER_DECOR_NOSELECTION_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "SelectionColor", 14, TT_COLOR, PAGER_DECOR_SEL_COLOR_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "GridColor", 9, TT_COLOR, PAGER_DECOR_GRID_COLOR_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "DeskBorderWidth", 15, TT_INTEGER, PAGER_DECOR_BORDER_WIDTH_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "DeskBorderColor", 15, TT_COLOR, PAGER_DECOR_BORDER_COLOR_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "LabelBelowDesk", 14, TT_FLAG, PAGER_DECOR_LABEL_BELOW_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "HideInactiveLabels", 18, TT_FLAG, PAGER_DECOR_HIDE_INACTIVE_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "VerticalLabel", 13, TT_FLAG, PAGER_DECOR_VERTICAL_LABEL_ID, NULL}
	,
	{0, NULL, 0, 0, 0}
};

SyntaxDef     PagerDecorationSyntax = {
	',',
	'\n',
	PagerDecorationTerms,
	0,										   /* use default hash size */
	' ',
	" ",
	"\t",
	"Module:Pager Decoration options",
	"PagerDecorations",
	"Visual attributes of the Pager window",
	NULL,
	0
};

#define PAGER_FEEL_TERMS \
    {0, "DontDrawBg", 8, TT_FLAG, PAGER_DRAW_BG_ID, NULL}, \
    {0, "StartIconic", 11, TT_FLAG, PAGER_START_ICONIC_ID, NULL}

#define PAGER_LOOK_TERMS \
    {0, "Align", 5, TT_FLAG, PAGER_ALIGN_ID, &AlignSyntax}, \
    {0, "SmallFont", 9, TT_FONT, PAGER_SMALL_FONT_ID, NULL}, \
    {TF_DONT_SPLIT, "ShadeButton", 11, TT_OPTIONAL_PATHNAME, PAGER_SHADE_BUTTON_ID, NULL}, \
    {TF_INDEXED|TF_QUOTES_OPTIONAL, "Style", 5, TT_QUOTED_TEXT, PAGER_STYLE_ID, NULL}, \
    {0, "ActiveDeskBevel", 15,   TT_FLAG, PAGER_ActiveBevel_ID   , &BevelSyntax}, \
    {0, "InActiveDeskBevel", 17, TT_FLAG, PAGER_InActiveBevel_ID , &BevelSyntax}, \
    {TF_DONT_REMOVE_COMMENTS, "Decoration", 10, TT_TEXT, PAGER_DECORATION_ID, &PagerDecorationSyntax}

#define PAGER_PRIVATE_TERMS \
	{0, "Geometry", 8, TT_GEOMETRY, PAGER_GEOMETRY_ID, NULL}, \
    {0, "IconGeometry", 12, TT_GEOMETRY, PAGER_ICON_GEOMETRY_ID, NULL}, \
    {0, "Rows", 4, TT_INTEGER, PAGER_ROWS_ID, NULL}, \
    {0, "Columns", 7, TT_INTEGER, PAGER_COLUMNS_ID, NULL}, \
    {0, "StickyIcons", 11, TT_FLAG, PAGER_STICKY_ICONS_ID, NULL}, \
    {TF_INDEXED, "Label", 5, TT_TEXT, PAGER_LABEL_ID, NULL}

/* we have sybsyntax for this one too, just like for MyStyle */

TermDef       PagerFeelTerms[] = {
	/* Feel */
	PAGER_FEEL_TERMS,
	BALLOON_FEEL_TERMS,
	{0, NULL, 0, 0, 0}
};

SyntaxDef     PagerFeelSyntax = {
	'\n',
	'\0',
	PagerFeelTerms,
	0,										   /* use default hash size */
	'\t',
	"",
	"\t",
	"PagerFeel",
	"PagerFeel",
	"AfterStep virtual desktop navigation module feel",
	NULL,
	0
};


/* use ModuleMyStyleSyntax for MyStyles */
/*	INCLUDE_MYSTYLE, */

TermDef       PagerLookTerms[] = {
	/* Look */
	PAGER_LOOK_TERMS,
/* now special cases that should be processed by it's own handlers */
	BALLOON_LOOK_TERMS,
	{0, NULL, 0, 0, 0}
};

SyntaxDef     PagerLookSyntax = {
	'\n',
	'\0',
	PagerLookTerms,
	0,										   /* use default hash size */
	'\t',
	"",
	"\t",
	"PagerLook",
	"PagerLook",
	"AfterStep virtual desktop navigation module look",
	NULL,
	0
};

TermDef       PagerPrivateTerms[] = {
	PAGER_PRIVATE_TERMS,
	BALLOON_FLAG_TERM,
	{0, NULL, 0, 0, 0}
};

SyntaxDef     PagerPrivateSyntax = {
	'\n',
	'\0',
	PagerPrivateTerms,
	0,										   /* use default hash size */
	'\t',
	"",
	"\t",
	"Pager",
	"Pager",
	"AfterStep virtual desktop navigation module",
	NULL,
	0
};



TermDef       PagerTerms[] = {
	/* Feel */
	PAGER_FEEL_TERMS,
	/* Look */
	PAGER_LOOK_TERMS,
	/* private stuff */
	PAGER_PRIVATE_TERMS,
/* including MyStyles definitions processing */
	INCLUDE_MYSTYLE,
/* now special cases that should be processed by it's own handlers */
	BALLOON_TERMS,
	{0, NULL, 0, 0, 0}
};

SyntaxDef     PagerSyntax = {
	'\n',
	'\0',
	PagerTerms,
	0,										   /* use default hash size */
	' ',
	"",
	"\t",
	"Module:Pager",
	"Pager",
	"AfterStep module for virtual desktop navigation",
	NULL,
	0
};

PagerConfig  *
CreatePagerConfig (int ndesks)
{
	PagerConfig  *config = (PagerConfig *) safecalloc (1, sizeof (PagerConfig));

	/* let's initialize Pager's config with some nice values: */
	init_asgeometry (&(config->icon_geometry));
	init_asgeometry (&(config->geometry));
	config->ndesks = ndesks;
	config->labels = CreateStringArray (ndesks);
	config->styles = CreateStringArray (ndesks);
	config->align = 0;
	config->flags = PAGER_FLAGS_DEFAULT;
	config->set_flags = 0;
	config->small_font_name = NULL;
	config->rows = 1;
	config->columns = ndesks;
	config->selection_color = NULL;
	config->grid_color = NULL;
	config->border_width = 1;
	config->border_color = NULL;
	config->style_defs = NULL;
	config->shade_button[0] = NULL;
	config->shade_button[1] = NULL;

	config->balloon_conf = NULL;
	config->more_stuff = NULL;
	config->gravity = NorthWestGravity;

	return config;
}

void
DestroyPagerConfig (PagerConfig * config)
{
	int           i;

	if (config->labels)
	{
		for (i = 0; i < config->ndesks; ++i)
			if (config->labels[i])
				free (config->labels[i]);
		free (config->labels);
	}
	if (config->styles)
	{
		for (i = 0; i < config->ndesks; ++i)
			if (config->styles[i])
				free (config->styles[i]);
		free (config->styles);
	}
	destroy_string (&(config->selection_color));
	destroy_string (&(config->grid_color));
	destroy_string (&(config->border_color));

/*
    if( config->shade_button[0] )
        free (config->shade_button[0]);
    if( config->shade_button[1] )
        free (config->shade_button[1]);
 */
	Destroy_balloonConfig (config->balloon_conf);
	DestroyFreeStorage (&(config->more_stuff));
	DestroyMyStyleDefinitions (&(config->style_defs));
	free (config);
}

void
ReadDecorations (PagerConfig * config, FreeStorageElem * pCurr)
{
	ConfigItem    item;

	item.memory = NULL;

	for (; pCurr; pCurr = pCurr->next)
	{
		if (pCurr->term == NULL)
			continue;

		if (pCurr->term->type == TT_FLAG)
		{
			switch (pCurr->term->id)
			{
			 case PAGER_DECOR_NOLABEL_ID:
				 config->flags &= ~USE_LABEL;
				 set_flags (config->set_flags, USE_LABEL);
				 break;
			 case PAGER_DECOR_NOSEPARATOR_ID:
				 config->flags &= ~PAGE_SEPARATOR;
				 set_flags (config->set_flags, PAGE_SEPARATOR);
				 break;
			 case PAGER_DECOR_NOSELECTION_ID:
				 config->flags &= ~SHOW_SELECTION;
				 set_flags (config->set_flags, SHOW_SELECTION);
				 break;
			 case PAGER_DECOR_LABEL_BELOW_ID:
				 config->flags |= LABEL_BELOW_DESK;
				 set_flags (config->set_flags, LABEL_BELOW_DESK);
				 break;
			 case PAGER_DECOR_HIDE_INACTIVE_ID:
				 config->flags |= HIDE_INACTIVE_LABEL;
				 set_flags (config->set_flags, HIDE_INACTIVE_LABEL);
				 break;
			 case PAGER_DECOR_VERTICAL_LABEL_ID:
				 config->flags |= VERTICAL_LABEL;
				 set_flags (config->set_flags, VERTICAL_LABEL);
				 break;
			}
		} else
		{
			if (!ReadConfigItem (&item, pCurr))
				continue;

			switch (pCurr->term->id)
			{
			 case PAGER_DECOR_SEL_COLOR_ID:
				 config->selection_color = item.data.string;
				 config->flags |= SHOW_SELECTION;
				 set_flags (config->set_flags, PAGER_SET_SELECTION_COLOR | SHOW_SELECTION);
				 break;
			 case PAGER_DECOR_GRID_COLOR_ID:
				 config->grid_color = item.data.string;
				 config->flags |= DIFFERENT_GRID_COLOR;
				 set_flags (config->set_flags, PAGER_SET_GRID_COLOR | DIFFERENT_GRID_COLOR);
				 break;
			 case PAGER_DECOR_BORDER_WIDTH_ID:
				 config->border_width = item.data.integer;
				 set_flags (config->set_flags, PAGER_SET_BORDER_WIDTH);
				 break;
			 case PAGER_DECOR_BORDER_COLOR_ID:
				 config->border_color = item.data.string;
				 config->flags |= DIFFERENT_BORDER_COLOR;
				 set_flags (config->set_flags, PAGER_SET_BORDER_COLOR | DIFFERENT_BORDER_COLOR);
				 break;
			 default:
				 item.ok_to_free = 1;
			}
		}
	}
	ReadConfigItem (&item, NULL);
}

PagerConfig  *
ParsePagerOptions (const char *filename, char *myname, int desk1, int desk2)
{
	ConfigData    cd;
	ConfigDef    *PagerConfigReader;
	int           ndesks = (desk2 - desk1) + 1;
	PagerConfig  *config = CreatePagerConfig (ndesks);

	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem    item;

	cd.filename = filename;
	PagerConfigReader = InitConfigReader (myname, &PagerSyntax, CDT_Filename, cd, NULL);
	if (!PagerConfigReader)
		return config;

	item.memory = NULL;
	PrintConfigReader (PagerConfigReader);
	ParseConfig (PagerConfigReader, &Storage);

	/* getting rid of all the crap first */
	StorageCleanUp (&Storage, &(config->more_stuff), CF_DISABLED_OPTION);

	config->balloon_conf = Process_balloonOptions (Storage, NULL, BALLOON_ID_START);
	config->style_defs = free_storage2MyStyleDefinitionsList (Storage);

	for (pCurr = Storage; pCurr; pCurr = pCurr->next)
	{
		if (pCurr->term == NULL)
			continue;

		if (pCurr->term->type == TT_FLAG)
		{
			switch (pCurr->term->id)
			{
			 case PAGER_ALIGN_ID:
				 set_flags (config->set_flags, PAGER_SET_ALIGN);
				 config->align = ParseAlignOptions (pCurr->sub);
				 break;
			 case PAGER_DRAW_BG_ID:
				 config->flags &= ~REDRAW_BG;
				 set_flags (config->set_flags, REDRAW_BG);
				 break;
			 case PAGER_START_ICONIC_ID:
				 config->flags |= START_ICONIC;
				 set_flags (config->set_flags, START_ICONIC);
				 break;
			 case PAGER_FAST_STARTUP_ID:
				 config->flags |= FAST_STARTUP;
				 set_flags (config->set_flags, FAST_STARTUP);
				 break;
			 case PAGER_SET_ROOT_ID:
				 config->flags |= SET_ROOT_ON_STARTUP;
				 set_flags (config->set_flags, SET_ROOT_ON_STARTUP);
				 break;
			 case PAGER_STICKY_ICONS_ID:
				 set_flags (config->flags, STICKY_ICONS);
				 set_flags (config->set_flags, STICKY_ICONS);
				 break;
			 case PAGER_ActiveBevel_ID:
				 set_flags (config->set_flags, PAGER_SET_ACTIVE_BEVEL);
				 config->active_desk_bevel = ParseBevelOptions (pCurr->sub);
				 break;
			 case PAGER_InActiveBevel_ID:
				 set_flags (config->set_flags, PAGER_SET_INACTIVE_BEVEL);
				 config->inactive_desk_bevel = ParseBevelOptions (pCurr->sub);
				 break;
			}
		} else
		{
			if (!ReadConfigItem (&item, pCurr))
				continue;
			if ((pCurr->term->flags & TF_INDEXED) && (item.index < desk1 || item.index > desk2))
			{
				item.ok_to_free = 1;
				continue;
			}

			switch (pCurr->term->id)
			{
			 case PAGER_GEOMETRY_ID:
				 config->geometry = item.data.geometry;
				 set_flags (config->set_flags, PAGER_SET_GEOMETRY);
				 break;
			 case PAGER_ICON_GEOMETRY_ID:
				 config->icon_geometry = item.data.geometry;
				 set_flags (config->set_flags, PAGER_SET_ICON_GEOMETRY);
				 break;
			 case PAGER_SMALL_FONT_ID:
				 config->small_font_name = item.data.string;
				 set_flags (config->set_flags, PAGER_SET_SMALL_FONT);
				 break;
			 case PAGER_ROWS_ID:
				 config->rows = (int)item.data.integer;
				 set_flags (config->set_flags, PAGER_SET_ROWS);
				 break;
			 case PAGER_COLUMNS_ID:
				 config->columns = (int)item.data.integer;
				 set_flags (config->set_flags, PAGER_SET_COLUMNS);
				 break;
			 case PAGER_LABEL_ID:
				 config->labels[item.index - desk1] = item.data.string;
				 break;
			 case PAGER_STYLE_ID:
				 config->styles[item.index - desk1] = item.data.string;
				 break;
			 case PAGER_SHADE_BUTTON_ID:
				 destroy_string (&(config->shade_button[0]));
				 destroy_string (&(config->shade_button[1]));
				 if (item.data.string)
				 {
					 register char *tmp = parse_filename (item.data.string, &(config->shade_button[0]));

					 while (isspace (*tmp))
						 ++tmp;
					 if (*tmp != '\0')
						 parse_filename (tmp, &(config->shade_button[1]));
				 }
				 set_flags (config->set_flags, PAGER_SET_SHADE_BUTTON);
				 break;
				 /* decoration options */
			 case PAGER_DECORATION_ID:
				 ReadDecorations (config, pCurr->sub);
				 item.ok_to_free = 1;
				 break;
			 default:
				 item.ok_to_free = 1;
			}
		}
	}

	ReadConfigItem (&item, NULL);
	DestroyConfig (PagerConfigReader);
	DestroyFreeStorage (&Storage);
	/*    PrintMyStyleDefinitions( config->style_defs ); */
	return config;
}

/* returns:
 *            0 on success
 *              1 if data is empty
 *              2 if ConfigWriter cannot be initialized
 *
 */
#if 0
int
WritePagerOptions (const char *filename, char *myname, int desk1, int desk2, PagerConfig * config, unsigned long flags)
{
	ConfigDef    *PagerConfigWriter = NULL;
	FreeStorageElem *Storage = NULL, **tail = &Storage;
	char         *Decorations = NULL;

	if (config == NULL)
		return 1;
	if ((PagerConfigWriter = InitConfigWriter (myname, &PagerSyntax, CDT_Filename, (void *)filename)) == NULL)
		return 2;

	CopyFreeStorage (&Storage, config->more_stuff);

	if (config->balloon_conf)
		tail = balloon2FreeStorage (&PagerSyntax, tail, config->balloon_conf);
	/* building free storage here */
	/* geometry */
	tail = Geometry2FreeStorage (&PagerSyntax, tail, &(config->geometry), PAGER_GEOMETRY_ID);
	/* icon_geometry */
	tail = Geometry2FreeStorage (&PagerSyntax, tail, &(config->icon_geometry), PAGER_ICON_GEOMETRY_ID);
	/* labels */
	tail = StringArray2FreeStorage (&PagerSyntax, tail, config->labels, desk1, desk2, PAGER_LABEL_ID);
	/* styles */
	tail = StringArray2FreeStorage (&PagerSyntax, tail, config->styles, desk1, desk2, PAGER_STYLE_ID);
	/* align */
	tail = Integer2FreeStorage (&PagerSyntax, tail, config->align, PAGER_ALIGN_ID);
	/* flags */
	if (!(config->flags & REDRAW_BG))
		tail = Flag2FreeStorage (&PagerSyntax, tail, PAGER_DRAW_BG_ID);
	if (config->flags & START_ICONIC)
		tail = Flag2FreeStorage (&PagerSyntax, tail, PAGER_START_ICONIC_ID);
	if (config->flags & FAST_STARTUP)
		tail = Flag2FreeStorage (&PagerSyntax, tail, PAGER_FAST_STARTUP_ID);
	if (config->flags & SET_ROOT_ON_STARTUP)
		tail = Flag2FreeStorage (&PagerSyntax, tail, PAGER_SET_ROOT_ID);
	if (config->flags & STICKY_ICONS)
		tail = Flag2FreeStorage (&PagerSyntax, tail, PAGER_STICKY_ICONS_ID);

	/* small_font_name */
	tail = String2FreeStorage (&PagerSyntax, tail, config->small_font_name, PAGER_SMALL_FONT_ID);
	/* rows */
	tail = Integer2FreeStorage (&PagerSyntax, tail, config->rows, PAGER_ROWS_ID);
	/* columns */
	tail = Integer2FreeStorage (&PagerSyntax, tail, config->columns, PAGER_COLUMNS_ID);

	/* now storing PagerDecorations */
	{
		ConfigDef    *DecorConfig = InitConfigWriter (myname, &PagerDecorationSyntax, CDT_Data, NULL);
		FreeStorageElem *DecorStorage = NULL, **d_tail = &DecorStorage;

		/* flags */
		if (!(config->flags & ~USE_LABEL))
			d_tail = Flag2FreeStorage (&PagerDecorationSyntax, d_tail, PAGER_DECOR_NOLABEL_ID);
		if (!(config->flags & PAGE_SEPARATOR))
			d_tail = Flag2FreeStorage (&PagerDecorationSyntax, d_tail, PAGER_DECOR_NOSEPARATOR_ID);
		if (!(config->flags & SHOW_SELECTION))
			d_tail = Flag2FreeStorage (&PagerDecorationSyntax, d_tail, PAGER_DECOR_NOSELECTION_ID);
		if (config->flags & LABEL_BELOW_DESK)
			d_tail = Flag2FreeStorage (&PagerDecorationSyntax, d_tail, PAGER_DECOR_LABEL_BELOW_ID);
		if (config->flags & HIDE_INACTIVE_LABEL)
			d_tail = Flag2FreeStorage (&PagerDecorationSyntax, d_tail, PAGER_DECOR_HIDE_INACTIVE_ID);

		/* selection_color */
		d_tail = String2FreeStorage (&PagerDecorationSyntax, d_tail, config->selection_color, PAGER_DECOR_SEL_COLOR_ID);
		/* border_width */
		d_tail =
			Integer2FreeStorage (&PagerDecorationSyntax, d_tail, config->border_width, PAGER_DECOR_BORDER_WIDTH_ID);
		/* grid_color */
		d_tail = String2FreeStorage (&PagerDecorationSyntax, d_tail, config->grid_color, PAGER_DECOR_GRID_COLOR_ID);
		/* border_color */
		d_tail = String2FreeStorage (&PagerDecorationSyntax, d_tail, config->border_color, PAGER_DECOR_BORDER_COLOR_ID);

		WriteConfig (DecorConfig, DecorStorage, CDT_Data, (void **)&Decorations, 0);
		DestroyFreeStorage (&DecorStorage);
		DestroyConfig (DecorConfig);
		if (DecorStorage)
			DestroyFreeStorage (&DecorStorage);

		tail = String2FreeStorage (&PagerSyntax, tail, Decorations, PAGER_DECORATION_ID);
		if (Decorations)
		{
			free (Decorations);
			Decorations = NULL;
		}
	}

	/* writing config into the file */
	WriteConfig (PagerConfigWriter, Storage, CDT_Filename, (void **)&filename, flags);
	DestroyFreeStorage (&Storage);
	DestroyConfig (PagerConfigWriter);

	if (Storage)
	{
		fprintf (stderr, "\n%s:Config Writing warning: Not all Free Storage discarded! Trying again...", myname);
		DestroyFreeStorage (&Storage);
		fprintf (stderr, (Storage != NULL) ? " failed." : " success.");
	}
	return 0;
}
#endif
