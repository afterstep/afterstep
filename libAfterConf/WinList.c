/*
 * Copyright (c) 2002 Sasha Vasko <sasha at aftercode.net>
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
#include "../include/afterbase.h"
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

TermDef       WinListTerms[] = {
    {0, "Geometry", 8, TT_GEOMETRY, WINLIST_Geometry_ID, NULL},
    {0, "MinSize", 7, TT_GEOMETRY, WINLIST_MinSize_ID, NULL},
    {0, "MaxSize", 7, TT_GEOMETRY, WINLIST_MaxSize_ID, NULL},
    {0, "MaxRows", 7, TT_INTEGER, WINLIST_MaxRows_ID, NULL},
    {0, "MaxColumns", 10, TT_INTEGER, WINLIST_MaxColumns_ID, NULL},
    {0, "MaxColWidth", 11, TT_INTEGER, WINLIST_MaxColWidth_ID, NULL},
    {0, "MinColWidth", 11, TT_INTEGER, WINLIST_MinColWidth_ID, NULL},
    {0, "UseName", 7, TT_INTEGER, WINLIST_UseName_ID, NULL},
    {0, "Align", 5, TT_FLAG, WINLIST_Align_ID, &AlignSyntax},
    {0, "Bevel", 5, TT_FLAG, WINLIST_Bevel_ID, &BevelSyntax},
    {0, "FBevel", 6, TT_FLAG, WINLIST_FBevel_ID, &BevelSyntax},
    {0, "UBevel", 6, TT_FLAG, WINLIST_UBevel_ID, &BevelSyntax},
    {0, "SBevel", 6, TT_FLAG, WINLIST_SBevel_ID, &BevelSyntax},
    {0, "FocusedBevel", 12, TT_FLAG, WINLIST_FBevel_ID, &BevelSyntax},
    {0, "StickyBevel", 10, TT_FLAG, WINLIST_SBevel_ID, &BevelSyntax},
    {0, "UnfocusedBevel", 14, TT_FLAG, WINLIST_UBevel_ID, &BevelSyntax},
    {0, "CompositionMethod", 17, TT_UINTEGER, WINLIST_CM_ID, NULL},
    {0, "FCompositionMethod", 18, TT_UINTEGER, WINLIST_FCM_ID, NULL},
    {0, "UCompositionMethod", 18, TT_UINTEGER, WINLIST_UCM_ID, NULL},
    {0, "SCompositionMethod", 18, TT_UINTEGER, WINLIST_SCM_ID, NULL},
    {0, "Spacing", 7, TT_UINTEGER, WINLIST_Spacing_ID, NULL},
    {0, "HSpacing", 8, TT_UINTEGER, WINLIST_HSpacing_ID, NULL},
    {0, "VSpacing", 8, TT_UINTEGER, WINLIST_VSpacing_ID, NULL},

    {TF_DONT_SPLIT, "Action", 6, TT_TEXT, WINLIST_Action_ID, NULL},
    {0, "UnfocusedStyle", 14, TT_TEXT, WINLIST_UnfocusedStyle_ID, NULL},
    {0, "FocusedStyle", 12, TT_TEXT, WINLIST_FocusedStyle_ID, NULL},
    {0, "StickyStyle", 11, TT_TEXT, WINLIST_StickyStyle_ID, NULL},
    {0, "FillRowsFirst", 13, TT_FLAG, WINLIST_FillRowsFirst_ID, NULL},
    {0, "UseSkipList", 11, TT_FLAG, WINLIST_UseSkipList_ID, NULL},
    {0, "ShapeToContents", 15, TT_FLAG, WINLIST_ShapeToContents_ID, NULL},
    {TF_DEPRECIATED, "Orientation", 11, TT_TEXT, WINLIST_Orientation_ID, NULL},
    {TF_DEPRECIATED, "MaxWidth", 8, TT_UINTEGER, WINLIST_MaxWidth_ID, NULL},
/* including MyStyles definitions processing */
	INCLUDE_MYSTYLE,

/* now special cases that should be processed by it's own handlers */
    BALLOON_TERMS,
    {0, NULL, 0, 0, 0}
};

SyntaxDef     WinListSyntax = {
	'\n',
	'\0',
	WinListTerms,
	0,										   /* use default hash size */
    ' ',
	"",
	"\t",
	"WinList configuration",
	NULL,
	0
};


WinListConfig *
CreateWinListConfig ()
{
	WinListConfig *config = (WinListConfig *) safecalloc (1, sizeof (WinListConfig));

    init_asgeometry (&(config->geometry));
    config->gravity = NorthWestGravity;
	config->max_rows = 1;
	config->show_name_type = ASN_Name;
    config->name_aligment = ALIGN_CENTER;
    config->balloon_conf = NULL;
    config->h_spacing = config->v_spacing = DEFAULT_TBAR_SPACING;
    config->fbevel = config->ubevel = config->sbevel = DEFAULT_TBAR_HILITE ;

	return config;
}

void
DestroyWinListConfig (WinListConfig * config)
{
	int           i;

	if (config->unfocused_style)
		free (config->unfocused_style);
	if (config->focused_style)
		free (config->focused_style);
	if (config->sticky_style)
		free (config->sticky_style);

	for (i = 0; i < MAX_MOUSE_BUTTONS; ++i)
		if (config->mouse_actions[i])
            destroy_string_list(config->mouse_actions[i]);

    Destroy_balloonConfig (config->balloon_conf);
	DestroyFreeStorage (&(config->more_stuff));
	DestroyMyStyleDefinitions (&(config->style_defs));

	free (config);
}

void
PrintWinListConfig (WinListConfig * config)
{
	int           i;

	fprintf (stderr, "WinListConfig = %p;\n", config);
	if (config == NULL)
		return;

	fprintf (stderr, "WinListConfig.RowsFirst = %s;\n",
			 get_flags (config->flags, WINLIST_FillRowsFirst) ? "True" : "False");
	fprintf (stderr, "WinListConfig.UseSkipList = %s;\n",
			 get_flags (config->flags, WINLIST_UseSkipList) ? "True" : "False");
	fprintf (stderr, "WinListConfig.set_flags = 0x%lX;\n", config->set_flags);
    fprintf (stderr, "WinListConfig.geometry.flags = 0x%X;\n", config->geometry.flags);
    fprintf (stderr, "WinListConfig.geometry = %+d%+d;\n", config->geometry.x, config->geometry.y);
	fprintf (stderr, "WinListConfig.gravity = %d;\n", config->gravity);
	fprintf (stderr, "WinListConfig.MinSize = %dx%d;\n", config->min_width, config->min_height);
	fprintf (stderr, "WinListConfig.MaxSize = %dx%d;\n", config->max_width, config->max_height);
	fprintf (stderr, "WinListConfig.MaxRows = %d;\n", config->max_rows);
	fprintf (stderr, "WinListConfig.MaxColumns = %d;\n", config->max_columns);
	fprintf (stderr, "WinListConfig.min_col_width = %d;\n", config->min_col_width);
	fprintf (stderr, "WinListConfig.max_col_width = %d;\n", config->max_col_width);

	fprintf (stderr, "WinListConfig.show_name_type = %d;\n", config->show_name_type);	/* 0, 1, 2, 3 */
    fprintf (stderr, "WinListConfig.name_aligment = %lX;\n", config->name_aligment);
    fprintf (stderr, "WinListConfig.fbevel = %lX;\n", config->fbevel);
    fprintf (stderr, "WinListConfig.ubevel = %lX;\n", config->ubevel);
    fprintf (stderr, "WinListConfig.sbevel = %lX;\n", config->sbevel);
    fprintf (stderr, "WinListConfig.fcm = %d;\n", config->fcm);
    fprintf (stderr, "WinListConfig.ucm = %d;\n", config->ucm);
    fprintf (stderr, "WinListConfig.scm = %d;\n", config->scm);
    fprintf (stderr, "WinListConfig.h_spacing = %d;\n", config->h_spacing);
    fprintf (stderr, "WinListConfig.v_spacing = %d;\n", config->v_spacing);

	fprintf (stderr, "WinListConfig.unfocused_style = %p;\n", config->unfocused_style);
	if (config->unfocused_style)
		fprintf (stderr, "WinListConfig.unfocused_style = \"%s\";\n", config->unfocused_style);
	fprintf (stderr, "WinListConfig.focused_style = %p;\n", config->focused_style);
	if (config->focused_style)
		fprintf (stderr, "WinListConfig.focused_style = \"%s\";\n", config->focused_style);
	fprintf (stderr, "WinListConfig.sticky_style = %p;\n", config->sticky_style);
	if (config->sticky_style)
		fprintf (stderr, "WinListConfig.sticky_style = \"%s\";\n", config->sticky_style);

	for (i = 0; i < MAX_MOUSE_BUTTONS; ++i)
	{
        fprintf (stderr, "WinListConfig.mouse_action[%d].list = %p;\n", i, config->mouse_actions[i]);
        if( config->mouse_actions[i] )
        {
            int k = 0 ;
            do
            {
                fprintf (stderr, "WinListConfig.mouse_action[%d].list[%d] = %p;\n", i, k, config->mouse_actions[i][k]);
                if (config->mouse_actions[i][k])
                    fprintf (stderr, "WinListConfig.mouse_action[%d].list[%d] = \"%s\";\n", i, k, config->mouse_actions[i][k]);
                else
                    break;
                ++k ;
            }while(1);

        }
	}
}

WinListConfig *
ParseWinListOptions (const char *filename, char *myname)
{
	ConfigDef    *ConfigReader =
        InitConfigReader (myname, &WinListSyntax, CDT_Filename, (void *)filename, NULL);
	WinListConfig *config = CreateWinListConfig ();

	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem    item;
	MyStyleDefinition **styles_tail = &(config->style_defs);

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
		if (pCurr->term == NULL)
			continue;

		if (pCurr->term->type == TT_FLAG)
        {
            switch (pCurr->term->id)
            {
                case WINLIST_FillRowsFirst_ID:
                    SET_CONFIG_FLAG (config->flags, config->set_flags, WINLIST_FillRowsFirst);
                    break;
                case WINLIST_UseSkipList_ID:
                    SET_CONFIG_FLAG (config->flags, config->set_flags, WINLIST_UseSkipList);
                    break;
                case WINLIST_UseIconNames_ID:
                    if (!get_flags (config->set_flags, WINLIST_UseName))
                    {
                        set_flags (config->set_flags, WINLIST_UseName);
                        config->show_name_type = ASN_IconName;
                    }
                    break;
                case WINLIST_Align_ID :
                    set_flags( config->set_flags, WINLIST_Align );
                    config->name_aligment = ParseAlignOptions( pCurr->sub );
                    break ;
                case WINLIST_Bevel_ID :
                    set_flags( config->set_flags, WINLIST_Bevel );
                    config->ubevel = config->sbevel = config->fbevel = ParseBevelOptions( pCurr->sub );
                    break ;
                case WINLIST_FBevel_ID :
                    set_flags( config->set_flags, WINLIST_FBevel );
                    config->fbevel = ParseBevelOptions( pCurr->sub );
                    break ;
                case WINLIST_UBevel_ID :
                    set_flags( config->set_flags, WINLIST_UBevel );
                    config->ubevel = ParseBevelOptions( pCurr->sub );
                    break ;
                case WINLIST_SBevel_ID :
                    set_flags( config->set_flags, WINLIST_SBevel );
                    config->sbevel = ParseBevelOptions( pCurr->sub );
                    break ;
                case WINLIST_ShapeToContents_ID:
                    SET_CONFIG_FLAG (config->flags, config->set_flags, WINLIST_ShapeToContents);
                    break;
            }
        }else
		{
			if (!ReadConfigItem (&item, pCurr))
				continue;

			switch (pCurr->term->id)
			{
                case WINLIST_Geometry_ID:
                    set_flags (config->set_flags, WINLIST_Geometry);
                    config->geometry = item.data.geometry ;
                    break;
                case WINLIST_MinSize_ID:
                    set_flags (config->set_flags, WINLIST_MinSize);
                    if (get_flags (item.data.geometry.flags, WidthValue))
                        config->min_width = item.data.geometry.width;
                    else
                        config->min_width = 0;
                    if (get_flags (item.data.geometry.flags, HeightValue))
                        config->min_height = item.data.geometry.height;
                    else
                        config->min_height = 0;
                    break;
                case WINLIST_MaxWidth_ID:
                    set_flags (config->set_flags, WINLIST_MinSize|WINLIST_MaxSize);
                    config->max_width = config->min_width = item.data.integer;
                    break;
                case WINLIST_MaxSize_ID:
                    set_flags (config->set_flags, WINLIST_MaxSize);
                    if (get_flags (item.data.geometry.flags, WidthValue))
                        config->max_width = item.data.geometry.width;
                    else
                        config->max_width = 0;
                    if (get_flags (item.data.geometry.flags, HeightValue))
                        config->max_height = item.data.geometry.height;
                    else
                        config->max_height = 0;
                    break;
                case WINLIST_MaxRows_ID:
                    set_flags (config->set_flags, WINLIST_MaxRows);
                    config->max_rows = item.data.integer;
                    break;
                case WINLIST_MaxColumns_ID:
                    set_flags (config->set_flags, WINLIST_MaxColumns);
                    config->max_columns = item.data.integer;
                    break;
                case WINLIST_MinColWidth_ID:
                    set_flags (config->set_flags, WINLIST_MinColWidth);
                    config->min_col_width = item.data.integer;
                    break;
                case WINLIST_MaxColWidth_ID:
                    set_flags (config->set_flags, WINLIST_MaxColWidth);
                    config->max_col_width = item.data.integer;
                    break;
                case WINLIST_UseName_ID:
                    set_flags (config->set_flags, WINLIST_UseName);
                    config->show_name_type = item.data.integer % ASN_NameTypes;
                    break;
                case WINLIST_Action_ID:
                    {
                        char         *ptr = item.data.string;
                        int           action_no = 0, i;

                        if (mystrncasecmp (ptr, "Click", 5) == 0)
                            ptr += 5;
                        if (isdigit (ptr[0]))
                        {
                            action_no = atoi (ptr);
                            if (action_no <= 0)
                                action_no = 1;
                            --action_no;
                            action_no %= MAX_MOUSE_BUTTONS;
                            i = 0;
                            while (!isspace (ptr[i]) && ptr[i])
                                ++i;
                            while (isspace (ptr[i]) && ptr[i])
                                ++i;
                            ptr += i;
                        }
                        if (*ptr)
                        {
                            destroy_string_list( config->mouse_actions[action_no] );
                            config->mouse_actions[action_no] = comma_string2list( ptr );
                        }
                        item.ok_to_free = 1;
                    }
                    break;
                case WINLIST_UnfocusedStyle_ID:
                    REPLACE_STRING (config->unfocused_style, item.data.string);
                    break;
                case WINLIST_FocusedStyle_ID:
                    REPLACE_STRING (config->focused_style, item.data.string);
                    break;
                case WINLIST_StickyStyle_ID:
                    REPLACE_STRING (config->sticky_style, item.data.string);
                    break;
                case MYSTYLE_START_ID:
                    styles_tail = ProcessMyStyleOptions (pCurr->sub, styles_tail);
                    item.ok_to_free = 1;
                    break;
                case WINLIST_Orientation_ID:
                    if (mystrncasecmp (item.data.string, "across", 6) == 0)
                        SET_CONFIG_FLAG (config->flags, config->set_flags, WINLIST_FillRowsFirst);
                    item.ok_to_free = 1;
                    break;
                case WINLIST_CM_ID :
                    set_flags( config->set_flags, WINLIST_CM );
                    config->ucm = config->scm = config->fcm = item.data.integer;
                    break ;
                case WINLIST_FCM_ID :
                    set_flags( config->set_flags, WINLIST_FCM );
                    config->fcm = item.data.integer;
                    break ;
                case WINLIST_UCM_ID :
                    set_flags( config->set_flags, WINLIST_UCM );
                    config->ucm = item.data.integer;
                    break ;
                case WINLIST_SCM_ID :
                    set_flags( config->set_flags, WINLIST_SCM );
                    config->scm = item.data.integer;
                    break ;
                case WINLIST_Spacing_ID :
                    set_flags( config->set_flags, WINLIST_H_SPACING|WINLIST_V_SPACING );
                    config->h_spacing = config->v_spacing = item.data.integer;
                    break ;
                case WINLIST_HSpacing_ID :
                    set_flags( config->set_flags, WINLIST_H_SPACING );
                    config->h_spacing = item.data.integer;
                    break ;
                case WINLIST_VSpacing_ID :
                    set_flags( config->set_flags, WINLIST_V_SPACING );
                    config->v_spacing = item.data.integer;
                    break ;

                default:
                    item.ok_to_free = 1;
			}
		}
	}
	ReadConfigItem (&item, NULL);
	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);
/*    PrintMyStyleDefinitions( config->style_defs ); */
	return config;

}


#if 0
/* returns:
 *            0 on success
 *              1 if data is empty
 *              2 if ConfigWriter cannot be initialized
 *
 */
int
WriteWinListOptions (const char *filename, char *myname, WinListConfig * config, unsigned long flags)
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
		tail = balloon2FreeStorage (&WinListSyntax, tail, config->balloon_conf);

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

		WriteConfig (DecorConfig, &DecorStorage, CDT_Data, (void **)&Decorations, 0);
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
	WriteConfig (PagerConfigWriter, &Storage, CDT_Filename, (void **)&filename, flags);
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
