/*
 * Copyright (c) 1998 Sasha Vasko <sashav@sprintmail.com>
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

TermDef PagerDecorationTerms[] =
{
  {TF_NO_MYNAME_PREPENDING, "NoDeskLabel", 11, TT_FLAG, PAGER_DECOR_NOLABEL_ID, NULL, NULL},
  {TF_NO_MYNAME_PREPENDING, "NoPageSeparator", 15, TT_FLAG, PAGER_DECOR_NOSEPARATOR_ID, NULL, NULL},
  {TF_NO_MYNAME_PREPENDING, "NoSelection", 11, TT_FLAG, PAGER_DECOR_NOSELECTION_ID, NULL, NULL},
  {TF_NO_MYNAME_PREPENDING, "SelectionColor", 14, TT_COLOR, PAGER_DECOR_SEL_COLOR_ID, NULL, NULL},
  {TF_NO_MYNAME_PREPENDING, "GridColor", 9, TT_COLOR, PAGER_DECOR_GRID_COLOR_ID, NULL, NULL},
  {TF_NO_MYNAME_PREPENDING, "DeskBorderWidth", 15, TT_INTEGER, PAGER_DECOR_BORDER_WIDTH_ID, NULL, NULL},
  {TF_NO_MYNAME_PREPENDING, "DeskBorderColor", 15, TT_COLOR, PAGER_DECOR_BORDER_COLOR_ID, NULL, NULL},
  {TF_NO_MYNAME_PREPENDING, "LabelBelowDesk", 14, TT_FLAG, PAGER_DECOR_LABEL_BELOW_ID, NULL, NULL},
  {TF_NO_MYNAME_PREPENDING, "HideInactiveLabels", 18, TT_FLAG, PAGER_DECOR_HIDE_INACTIVE_ID, NULL, NULL},
  {0, NULL, 0, 0, 0}
};

SyntaxDef PagerDecorationSyntax =
{
  ',',
  '\n',
  PagerDecorationTerms,
  0,				/* use default hash size */
  NULL
};

TermDef PagerTerms[] =
{
  {0, "Geometry", 8, TT_GEOMETRY, PAGER_GEOMETRY_ID, NULL, NULL},
  {0, "IconGeometry", 12, TT_GEOMETRY, PAGER_ICON_GEOMETRY_ID, NULL, NULL},
  {0, "Align", 5, TT_INTEGER, PAGER_ALIGN_ID, NULL, NULL},
  {0, "DontDrawBg", 8, TT_FLAG, PAGER_DRAW_BG_ID, NULL, NULL},
  {0, "SmallFont", 9, TT_FONT, PAGER_SMALL_FONT_ID, NULL, NULL},
  {0, "StartIconic", 11, TT_FLAG, PAGER_START_ICONIC_ID, NULL, NULL},
  {0, "Rows", 4, TT_INTEGER, PAGER_ROWS_ID, NULL, NULL},
  {0, "Columns", 7, TT_INTEGER, PAGER_COLUMNS_ID, NULL, NULL},
  {0, "StickyIcons", 11, TT_FLAG, PAGER_STICKY_ICONS_ID, NULL, NULL},
  {TF_INDEXED, "Label", 5, TT_TEXT, PAGER_LABEL_ID, NULL, NULL},
#ifdef PAGER_BACKGROUND
  {TF_INDEXED, "Style", 5, TT_TEXT, PAGER_STYLE_ID, NULL, NULL},
#endif
/* including MyStyles definitions processing */
  INCLUDE_MYSTYLE,
/* we have sybsyntax for this one too, just like for MyStyle */
  {TF_DONT_REMOVE_COMMENTS, "Decoration", 10, TT_TEXT, PAGER_DECORATION_ID, &PagerDecorationSyntax, NULL},
/* now special cases that should be processed by it's own handlers */
  {TF_SPECIAL_PROCESSING | TF_DONT_REMOVE_COMMENTS, "Balloons", 8, TT_FLAG, PAGER_BALLOONS_ID, NULL, NULL},
  {TF_SPECIAL_PROCESSING | TF_DONT_REMOVE_COMMENTS, "BalloonBorderWidth", 18, TT_INTEGER, PAGER_BALLOONS_ID, NULL, NULL},
  {TF_SPECIAL_PROCESSING | TF_DONT_REMOVE_COMMENTS, "BalloonBorderColor", 18, TT_COLOR, PAGER_BALLOONS_ID, NULL, NULL},
  {TF_SPECIAL_PROCESSING | TF_DONT_REMOVE_COMMENTS, "BalloonYOffset", 14, TT_INTEGER, PAGER_BALLOONS_ID, NULL, NULL},
  {TF_SPECIAL_PROCESSING | TF_DONT_REMOVE_COMMENTS, "BalloonDelay", 12, TT_INTEGER, PAGER_BALLOONS_ID, NULL, NULL},
  {0, NULL, 0, 0, 0}
};

SyntaxDef PagerSyntax =
{
  '\n',
  '\0',
  PagerTerms,
  0,				/* use default hash size */
  NULL
};


int
PagerSpecialFunc (ConfigDef * conf_def, FreeStorageElem ** storage)
{
  if (conf_def->current_term->id == PAGER_BALLOONS_ID)
    {
      balloon_parse (conf_def->tline, conf_def->fp);
      FlushConfigBuffer (conf_def);
    }
  return 1;
}

PagerConfig *
CreatePagerConfig (int ndesks)
{
  PagerConfig *config = (PagerConfig *) safemalloc (sizeof (PagerConfig));
  /* let's initialize Pager's config with some nice values: */
  InitMyGeometry (&(config->icon_geometry));
  InitMyGeometry (&(config->geometry));
  config->labels = CreateStringArray (ndesks);
#ifdef PAGER_BACKGROUND
  config->styles = CreateStringArray (ndesks);
#else
  config->styles = NULL;
#endif
  config->align = 0;
  config->flags = PAGER_FLAGS_DEFAULT;
  config->small_font_name = NULL;
  config->rows = 1;
  config->columns = ndesks;
  config->selection_color = NULL;
  config->grid_color = NULL;
  config->border_width = 1;
  config->border_color = NULL;
  config->style_defs = NULL;

  config->more_stuff = NULL;

  return config;
}

void
DestroyPagerConfig (PagerConfig * config)
{
  free (config->labels);
#ifdef PAGER_BACKGROUND
  free (config->styles);
#endif
  DestroyFreeStorage (&(config->more_stuff));
  DestroyMyStyleDefinitions (&(config->style_defs));
  free (config);
}

void
ReadDecorations (PagerConfig * config, FreeStorageElem * pCurr)
{
  ConfigItem item;

  item.memory = NULL;

  for (; pCurr; pCurr = pCurr->next)
    {
      if (pCurr->term == NULL)
	continue;

      if (pCurr->term->type == TT_FLAG)
	switch (pCurr->term->id)
	  {
	  case PAGER_DECOR_NOLABEL_ID:
	    config->flags &= ~USE_LABEL;
	    break;
	  case PAGER_DECOR_NOSEPARATOR_ID:
	    config->flags &= ~PAGE_SEPARATOR;
	    break;
	  case PAGER_DECOR_NOSELECTION_ID:
	    config->flags &= ~SHOW_SELECTION;
	    break;
	  case PAGER_DECOR_LABEL_BELOW_ID:
	    config->flags |= LABEL_BELOW_DESK;
	    break;
	  case PAGER_DECOR_HIDE_INACTIVE_ID:
	    config->flags |= HIDE_INACTIVE_LABEL;
	    break;
	  }
      else
	{
	  if (!ReadConfigItem (&item, pCurr))
	    continue;

	  switch (pCurr->term->id)
	    {

	    case PAGER_DECOR_SEL_COLOR_ID:
	      config->selection_color = item.data.string;
	      config->flags |= SHOW_SELECTION;
	      break;
	    case PAGER_DECOR_GRID_COLOR_ID:
	      config->grid_color = item.data.string;
	      config->flags |= DIFFERENT_GRID_COLOR;
	      break;
	    case PAGER_DECOR_BORDER_WIDTH_ID:
	      config->border_width = item.data.integer;
	      break;
	    case PAGER_DECOR_BORDER_COLOR_ID:
	      config->border_color = item.data.string;
	      config->flags |= DIFFERENT_BORDER_COLOR;
	      break;
	    default:
	      item.ok_to_free = 1;
	    }
	}
    }
  ReadConfigItem (&item, NULL);
}

PagerConfig *
ParsePagerOptions (const char *filename, char *myname, int desk1, int desk2)
{
  ConfigDef *PagerConfigReader = InitConfigReader (myname, &PagerSyntax, CDT_Filename, (void *) filename, PagerSpecialFunc);
  PagerConfig *config = CreatePagerConfig ((desk2 - desk1) + 1);

  FreeStorageElem *Storage = NULL, *pCurr;
  ConfigItem item;
  MyStyleDefinition **styles_tail = &(config->style_defs);

  if (!PagerConfigReader)
    return config;

  item.memory = NULL;
  PrintConfigReader (PagerConfigReader);
  ParseConfig (PagerConfigReader, &Storage);
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
	    break;
	  case PAGER_START_ICONIC_ID:
	    config->flags |= START_ICONIC;
	    break;
	  case PAGER_FAST_STARTUP_ID:
	    config->flags |= FAST_STARTUP;
	    break;
	  case PAGER_SET_ROOT_ID:
	    config->flags |= SET_ROOT_ON_STARTUP;
	    break;
	  case PAGER_STICKY_ICONS_ID:
	    config->flags |= STICKY_ICONS;
	    break;
	  }
      else
	{
	  if (!ReadConfigItem (&item, pCurr))
	    continue;
	  if ((pCurr->term->flags & TF_INDEXED) &&
	      (item.index < desk1 || item.index > desk2))
	    {
	      item.ok_to_free = 1;
	      continue;
	    }

	  switch (pCurr->term->id)
	    {
	    case PAGER_GEOMETRY_ID:
	      config->geometry = item.data.geometry;
	      break;
	    case PAGER_ICON_GEOMETRY_ID:
	      config->icon_geometry = item.data.geometry;
	      break;
	    case PAGER_ALIGN_ID:
	      config->align = (int) item.data.integer;
	      break;
	    case PAGER_SMALL_FONT_ID:
	      config->small_font_name = item.data.string;
	      break;
	    case PAGER_ROWS_ID:
	      config->rows = (int) item.data.integer;
	      break;
	    case PAGER_COLUMNS_ID:
	      config->columns = (int) item.data.integer;
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
WritePagerOptions (const char *filename, char *myname, int desk1, int desk2, PagerConfig * config, unsigned long flags)
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
