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


#include "../../configure.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/style.h"
#include "../../include/mystyle.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/


TermDef WinListTerms[] =
{
  {0, "Geometry", 8, TT_GEOMETRY, 		WINLIST_Geometry_ID, NULL, NULL},
  {0, "MinSize", 7, TT_GEOMETRY, 		WINLIST_MinSize_ID, NULL, NULL},
  {0, "MaxSize", 7, TT_GEOMETRY, 		WINLIST_MaxSize_ID, NULL, NULL},
  {0, "MaxColumns", 10, TT_INTEGER, 	WINLIST_MaxColumns_ID, NULL, NULL},
  {0, "MaxColWidth", 11, TT_INTEGER, 	WINLIST_MaxColWidth_ID, NULL, NULL},
  {0, "MinColWidth", 11, TT_INTEGER, 	WINLIST_MinColWidth_ID, NULL, NULL},
  {0, "UseName", 7, TT_INTEGER, 		WINLIST_UseName_ID, NULL, NULL},
  {0, "Justify", 7, TT_TEXT, 			WINLIST_Justify_ID, NULL, NULL},
  {0, "Action", 6, TT_TEXT, 			WINLIST_Action_ID, NULL, NULL},
  {0, "UnfocusedStyle", 14, TT_TEXT, 	WINLIST_UnfocusedStyle_ID, NULL, NULL},
  {0, "FocusedStyle", 12, TT_TEXT, 		WINLIST_FocusedStyle_ID, NULL, NULL},
  {0, "StickyStyle", 11, TT_TEXT, 		WINLIST_StickyStyle_ID, NULL, NULL},
  {0, "FillRowsFirst", 13, TT_FLAG,		WINLIST_FillRowsFirst_ID, NULL, NULL},
  {0, "UseSkipList", 11, TT_FLAG, 		WINLIST_UseSkipList_ID, NULL, NULL},
/* including MyStyles definitions processing */
  INCLUDE_MYSTYLE,

/* now special cases that should be processed by it's own handlers */
  {TF_SPECIAL_PROCESSING | TF_DONT_REMOVE_COMMENTS, "Balloons", 8, TT_FLAG, WINLIST_BALLOONS_ID, NULL, NULL},
  {TF_SPECIAL_PROCESSING | TF_DONT_REMOVE_COMMENTS, "BalloonBorderWidth", 18, TT_INTEGER, WINLIST_BALLOONS_ID, NULL, NULL},
  {TF_SPECIAL_PROCESSING | TF_DONT_REMOVE_COMMENTS, "BalloonBorderColor", 18, TT_COLOR, WINLIST_BALLOONS_ID, NULL, NULL},
  {TF_SPECIAL_PROCESSING | TF_DONT_REMOVE_COMMENTS, "BalloonYOffset", 14, TT_INTEGER, WINLIST_BALLOONS_ID, NULL, NULL},
  {TF_SPECIAL_PROCESSING | TF_DONT_REMOVE_COMMENTS, "BalloonDelay", 12, TT_INTEGER, WINLIST_BALLOONS_ID, NULL, NULL},
/* obsolete options : */
  {0, "HideGeometry", 12, TT_GEOMETRY, 	WINLIST_HideGeometry_ID, NULL, NULL},
  {0, "MaxWidth", 8, TT_INTEGER, 	    WINLIST_MaxWidth_ID, NULL, NULL},
  {0, "Orientation", 8, TT_TEXT, 	    WINLIST_Orientation_ID, NULL, NULL},
  {0, "NoAnchor", 8, TT_FLAG, 			WINLIST_NoAnchor_ID, NULL, NULL},
  {0, "UseIconNames", 12, TT_FLAG, 		WINLIST_UseIconNames_ID, NULL, NULL},
  {0, "AutoHide", 8, TT_FLAG, 			WINLIST_AutoHide_ID, NULL, NULL},
  {0, NULL, 0, 0, 0}
};

SyntaxDef WinListSyntax =
{
  '\n',
  '\0',
  WinListTerms,
  0,				/* use default hash size */
  NULL
};


int
WinListSpecialFunc (ConfigDef * conf_def, FreeStorageElem ** storage)
{
  if (conf_def->current_term->id == WINLIST_BALLOONS_ID)
    {
      balloon_parse (conf_def->tline, conf_def->fp);
      FlushConfigBuffer (conf_def);
    }
  return 1;
}

WinListConfig *
CreateWinListConfig ()
{
	WinListConfig *config = (WinListConfig *) safecalloc (1, sizeof (WinListConfig));

	config->gravity = NorthWestGravity;
	config->max_rows = 1 ;
	config->show_name_type = ASN_Name ;
	config->name_alignment = ASA_Left ;

    return config;
}

void
DestroyWinListConfig (WinListConfig * config)
{
	int i ;
	
	if( config->unfocused_style )
		free( config->unfocused_style );
	if( config->focused_style )
		free( config->focused_style );
	if( config->sticky_style )
		free( config->sticky_style );
		
	for( i = 0 ; i < MAX_MOUSE_BUTTONS; ++i )
		if( config->MouseAction[i] ) 
			free( config->MouseAction[i] );

  DestroyFreeStorage (&(config->more_stuff));
  DestroyMyStyleDefinitions (&(config->style_defs));

  free (config);
}

WinListConfig *
ParseWinListOptions (const char *filename, char *myname)
{
	ConfigDef *ConfigReader = InitConfigReader (myname, &WinListSyntax, CDT_Filename, (void *) filename, WinListSpecialFunc);
	WinListConfig *config = CreateWinListConfig ();

	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem item;
    MyStyleDefinition **styles_tail = &(config->style_defs);

	if (!ConfigReader)
  		return config;

    item.memory = NULL;
	PrintConfigReader (ConfigReader);
    ParseConfig (ConfigReader, &Storage);
	PrintFreeStorage (Storage);

    /* getting rid of all the crap first */
	StorageCleanUp (&Storage, &(config->more_stuff), CF_DISABLED_OPTION);

    for (pCurr = Storage; pCurr; pCurr = pCurr->next)
    {
    	if (pCurr->term == NULL)
			continue;

    	if (pCurr->term->type == TT_FLAG)
			switch (pCurr->term->id)
			{
				case PAGER_DRAW_BG_ID:
				    config->flags &= ~REDRAW_BG;
					set_flags( config->set_flags, REDRAW_BG ) ;
				    break;
				case PAGER_START_ICONIC_ID:
	  				config->flags |= START_ICONIC;
					set_flags( config->set_flags, START_ICONIC ) ;
	  				break;
				case PAGER_FAST_STARTUP_ID:
				    config->flags |= FAST_STARTUP;
					set_flags( config->set_flags, FAST_STARTUP ) ;
				    break;
			}
        else
		{
			if (!ReadConfigItem (&item, pCurr))
	    continue;

	  switch (pCurr->term->id)
	    {
	    case PAGER_GEOMETRY_ID:
	      config->geometry = item.data.geometry;
		  set_flags( config->set_flags, PAGER_SET_GEOMETRY ) ;
	      break;
	    case PAGER_ICON_GEOMETRY_ID:
	      config->icon_geometry = item.data.geometry;
		  set_flags( config->set_flags, PAGER_SET_ICON_GEOMETRY ) ;
	      break;
	    case PAGER_ALIGN_ID:
	      config->align = (int) item.data.integer;
		  set_flags( config->set_flags, PAGER_SET_ALIGN ) ;
	      break;
	    case PAGER_SMALL_FONT_ID:
	      config->small_font_name = item.data.string;
		  set_flags( config->set_flags, PAGER_SET_SMALL_FONT ) ;
	      break;
	    case PAGER_ROWS_ID:
	      config->rows = (int) item.data.integer;
		  set_flags( config->set_flags, PAGER_SET_ROWS ) ;
	      break;
	    case PAGER_COLUMNS_ID:
	      config->columns = (int) item.data.integer;
		  set_flags( config->set_flags, PAGER_SET_COLUMNS ) ;
	      break;
	    case PAGER_LABEL_ID:
	      config->labels[item.index - desk1] = item.data.string;
	      break;
#ifdef PAGER_BACKGROUND
	    case PAGER_STYLE_ID:
	      config->styles[item.index - desk1] = item.data.string;
	      break;
#endif
	    case MYSTYLE_START_ID:
	      styles_tail = ProcessMyStyleOptions (pCurr->sub, styles_tail);
	      item.ok_to_free = 1;
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
int
WriteWinListOptions (const char *filename, char *myname, WinListConfig * config, unsigned long flags)
{
  ConfigDef *PagerConfigWriter = NULL;
  FreeStorageElem *Storage = NULL, **tail = &Storage;
  char *Decorations = NULL;

  if (config == NULL)
    return 1;
  if ((PagerConfigWriter = InitConfigWriter (myname, &PagerSyntax, CDT_Filename, (void *) filename)) == NULL)
    return 2;

  CopyFreeStorage (&Storage, config->more_stuff);

  /* building free storage here */
  /* geometry */
  tail = Geometry2FreeStorage (&PagerSyntax, tail, &(config->geometry), PAGER_GEOMETRY_ID);
  /* icon_geometry */
  tail = Geometry2FreeStorage (&PagerSyntax, tail, &(config->icon_geometry), PAGER_ICON_GEOMETRY_ID);
  /* labels */
  tail = StringArray2FreeStorage (&PagerSyntax, tail, config->labels, desk1, desk2, PAGER_LABEL_ID);
  /* styles */
#ifdef PAGER_BACKGROUND
  tail = StringArray2FreeStorage (&PagerSyntax, tail, config->styles, desk1, desk2, PAGER_STYLE_ID);
#endif
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
    ConfigDef *DecorConfig = InitConfigWriter (myname, &PagerDecorationSyntax, CDT_Data, NULL);
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
    d_tail = Integer2FreeStorage (&PagerDecorationSyntax, d_tail, config->border_width, PAGER_DECOR_BORDER_WIDTH_ID);
    /* grid_color */
    d_tail = String2FreeStorage (&PagerDecorationSyntax, d_tail, config->grid_color, PAGER_DECOR_GRID_COLOR_ID);
    /* border_color */
    d_tail = String2FreeStorage (&PagerDecorationSyntax, d_tail, config->border_color, PAGER_DECOR_BORDER_COLOR_ID);

    WriteConfig (DecorConfig, &DecorStorage, CDT_Data, (void **) &Decorations, 0);
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
  WriteConfig (PagerConfigWriter, &Storage, CDT_Filename, (void **) &filename, flags);
  DestroyConfig (PagerConfigWriter);

  if (Storage)
    {
      fprintf (stderr, "\n%s:Config Writing warning: Not all Free Storage discarded! Trying again...", myname);
      DestroyFreeStorage (&Storage);
      fprintf (stderr, (Storage != NULL) ? " failed." : " success.");
    }
  return 0;
}
