/*
 * Copyright (c) 1999 Ethan Fischer <allanon@crystaltokyo.com>
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

/**********************************************************************
 *
 * code for parsing the afterstep style command
 *
 ***********************************************************************/
#include "../../configure.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "../../include/aftersteplib.h"
#include "../../include/style.h"
#include "../../include/parse.h"
#include "../../include/afterstep.h"
#include "../../include/misc.h"
#include "../../include/screen.h"

/************************************************************************
 * ReapChildren - wait() for all dead child processes
 ************************************************************************/

#ifdef HAVE_SYS_WAIT_H
#define ReapChildren()  while ((waitpid(-1, NULL, WNOHANG)) > 0);
#elif defined (HAVE_WAIT3)
#define ReapChildren()  while ((wait3(NULL, WNOHANG, NULL)) > 0);
#endif


void
style_parse (char *text, FILE * fd, char **list, int *junk)
{
  char *name, *line;
  char *restofline;
  name_list *nl;
  int len;

  name = stripcpy2 (text, 0);
  /* in case there was no argument! */
  if (name == NULL)
    return;

  restofline = stripcpy3 (text, False);
  line = restofline;

  if (restofline == NULL)
    return;

  nl = style_new ();
  nl->name = name;

  while ((*restofline != 0) && (*restofline != '\n'))
    {
      while (isspace ((unsigned char) *restofline))
	restofline++;
      if (mystrncasecmp (restofline, "Icon", 4) == 0)
	{
	  restofline = ReadFileName (restofline + 4, &nl->icon_file, &len);
	  if (len > 0)
	    {
	      nl->off_flags |= ICON_FLAG;
	      nl->on_flags |= SUPPRESSICON_FLAG;
	    }
	  else
	    nl->on_flags |= SUPPRESSICON_FLAG;
	}
      else if (!mystrncasecmp (restofline, "FocusStyle", 10))
	{
	  restofline = ReadFileName (restofline + 10, &nl->style_focus, &len);
	  if (nl->style_focus != NULL)
	    nl->off_flags |= STYLE_FOCUS_FLAG;
	  else
	    nl->on_flags |= STYLE_FOCUS_FLAG;
	}
      else if (!mystrncasecmp (restofline, "UnfocusStyle", 12))
	{
	  restofline = ReadFileName (restofline + 12, &nl->style_unfocus, &len);
	  if (nl->style_unfocus != NULL)
	    nl->off_flags |= STYLE_UNFOCUS_FLAG;
	  else
	    nl->on_flags |= STYLE_UNFOCUS_FLAG;
	}
      else if (!mystrncasecmp (restofline, "StickyStyle", 11))
	{
	  restofline = ReadFileName (restofline + 11, &nl->style_sticky, &len);
	  if (nl->style_sticky != NULL)
	    nl->off_flags |= STYLE_STICKY_FLAG;
	  else
	    nl->on_flags |= STYLE_STICKY_FLAG;
	}
      else if (mystrncasecmp (restofline, "NoIconTitle", 11) == 0)
	{
	  nl->off_flags |= NOICON_TITLE_FLAG;
	  restofline += 11;
	}
      else if (mystrncasecmp (restofline, "IconTitle", 9) == 0)
	{
	  nl->on_flags |= NOICON_TITLE_FLAG;
	  restofline += 9;
	}
      else if (mystrncasecmp (restofline, "NoIcon", 6) == 0)
	{
	  restofline += 6;
	  nl->off_flags |= SUPPRESSICON_FLAG;
	}
      else if (mystrncasecmp (restofline, "NoFocus", 7) == 0)
	{
	  restofline += 7;
	  nl->off_flags |= NOFOCUS_FLAG;
	}
      else if (mystrncasecmp (restofline, "NoTitle", 7) == 0)
	{
	  restofline += 7;
	  nl->off_flags |= NOTITLE_FLAG;
	}
      else if (mystrncasecmp (restofline, "Title", 5) == 0)
	{
	  restofline += 5;
	  nl->on_flags |= NOTITLE_FLAG;
	}
      else if (mystrncasecmp (restofline, "NoHandles", 9) == 0)
	{
	  restofline += 9;
	  nl->off_flags |= NOHANDLES_FLAG;
	}
      else if (mystrncasecmp (restofline, "Handles", 7) == 0)
	{
	  restofline += 7;
	  nl->on_flags |= NOHANDLES_FLAG;
	}
      else if (mystrncasecmp (restofline, "NoButton", 8) == 0)
	{
	  int butt;
	  restofline = ReadIntValue (restofline + 8, &butt);
	  nl->off_buttons |= (1 << (butt - 1));
	}
      else if (mystrncasecmp (restofline, "Button", 6) == 0)
	{
	  int butt;
	  restofline = ReadIntValue (restofline + 6, &butt);
	  nl->on_buttons |= (1 << (butt - 1));
	}
      else if (mystrncasecmp (restofline, "WindowListSkip", 14) == 0)
	{
	  restofline += 14;
	  nl->off_flags |= LISTSKIP_FLAG;
	}
      else if (mystrncasecmp (restofline, "WindowListHit", 13) == 0)
	{
	  restofline += 13;
	  nl->on_flags |= LISTSKIP_FLAG;
	}
      else if (mystrncasecmp (restofline, "CirculateSkip", 13) == 0)
	{
	  restofline += 13;
	  nl->off_flags |= CIRCULATESKIP_FLAG;
	}
      else if (mystrncasecmp (restofline, "CirculateHit", 12) == 0)
	{
	  restofline += 12;
	  nl->on_flags |= CIRCULATESKIP_FLAG;
	}
      else if (mystrncasecmp (restofline, "StartIconic", 11) == 0)
	{
	  restofline += 11;
	  nl->off_flags |= START_ICONIC_FLAG;
	}
      else if (mystrncasecmp (restofline, "StartNormal", 11) == 0)
	{
	  restofline += 11;
	  nl->on_flags |= START_ICONIC_FLAG;
	}
      else if (mystrncasecmp (restofline, "Layer", 5) == 0)
	{
	  restofline = ReadIntValue (restofline + 5, &nl->layer);
	  nl->off_flags |= LAYER_FLAG;
	}
      else if (mystrncasecmp (restofline, "StaysOnTop", 10) == 0)
	{
	  restofline += 10;
	  nl->layer = 1;
	  nl->off_flags |= LAYER_FLAG;
	}
      else if (mystrncasecmp (restofline, "StaysPut", 8) == 0)
	{
	  restofline += 8;
	  nl->layer = 0;
	  nl->off_flags |= LAYER_FLAG;
	}
      else if (mystrncasecmp (restofline, "StaysOnBack", 11) == 0)
	{
	  restofline += 11;
	  nl->layer = -1;
	  nl->off_flags |= LAYER_FLAG;
	}
      else if (!mystrncasecmp (restofline, "AvoidCover", 10))
	{
	  restofline += 10;
	  nl->off_flags |= AVOID_COVER_FLAG;
	}
      else if (!mystrncasecmp (restofline, "AllowCover", 10))
	{
	  restofline += 10;
	  nl->on_flags |= AVOID_COVER_FLAG;
	}
      else if (!mystrncasecmp (restofline, "VerticalTitle", 13))
	{
	  restofline += 13;
	  nl->off_flags |= VERTICAL_TITLE_FLAG;
	}
      else if (!mystrncasecmp (restofline, "HorizontalTitle", 15))
	{
	  restofline += 15;
	  nl->on_flags |= VERTICAL_TITLE_FLAG;
	}
      else if (mystrncasecmp (restofline, "Sticky", 6) == 0)
	{
	  nl->off_flags |= STICKY_FLAG;
	  restofline += 6;
	}
      else if (mystrncasecmp (restofline, "Slippery", 8) == 0)
	{
	  nl->on_flags |= STICKY_FLAG;
	  restofline += 8;
	}
      else if (mystrncasecmp (restofline, "BorderWidth", 11) == 0)
	{
	  nl->off_flags |= BW_FLAG;
	  restofline = ReadIntValue (restofline + 11, &nl->border_width);
	}
      else if (mystrncasecmp (restofline, "HandleWidth", 11) == 0)
	{
	  restofline = ReadIntValue (restofline + 11, &nl->resize_width);
	  nl->off_flags |= NOBW_FLAG;
	}
      else if (mystrncasecmp (restofline, "StartsOnDesk", 12) == 0)
	{
	  restofline = ReadIntValue (restofline + 12, &nl->Desk);
	  nl->off_flags |= STAYSONDESK_FLAG;
	}
      else if (mystrncasecmp (restofline, "ViewportX", 9) == 0)
	{
	  restofline = ReadIntValue (restofline + 9, &nl->ViewportX);
	  nl->off_flags |= VIEWPORTX_FLAG;
	}
      else if (mystrncasecmp (restofline, "ViewportY", 9) == 0)
	{
	  restofline = ReadIntValue (restofline + 9, &nl->ViewportY);
	  nl->off_flags |= VIEWPORTY_FLAG;
	}
      else if (mystrncasecmp (restofline, "StartsAnywhere", 14) == 0)
	{
	  restofline += 14;
	  nl->on_flags |= STAYSONDESK_FLAG;
	}
      else if (mystrncasecmp (restofline, "DefaultGeometry", 15) == 0)
	{
	  restofline = parse_geometry( restofline+15,
				                  &nl->PreposX, &nl->PreposY,
              					  &nl->PreposWidth, &nl->PreposHeight,
								  &(nl->PreposFlags) );
	  nl->off_flags |= PREPOS_FLAG;
	}
      while (isspace ((unsigned char) *restofline))
	restofline++;
      if (*restofline == ',')
	restofline++;
      else if ((*restofline != 0) && (*restofline != '\n'))
	{
	  afterstep_err ("bad style command in line %s at %s",
			 text, restofline, NULL);
	  return;
	}
    }
  /* capture default icons */
  if (strcmp (nl->name, "*") == 0)
    {
      if (nl->off_flags & ICON_FLAG)
	Scr.DefaultIcon = nl->icon_file;
      nl->off_flags &= ~ICON_FLAG;
      nl->icon_file = NULL;
    }
  free (line);
}

void
style_init (name_list * nl)
{
  nl->next = NULL;

  nl->name = NULL;
  nl->on_flags = 0;
  nl->off_flags = 0;
  nl->icon_file = NULL;
  nl->Desk = 0;
  nl->layer = 0;
  nl->border_width = 0;
  nl->resize_width = 0;
  nl->ViewportX = -1;
  nl->ViewportY = -1;
  nl->PreposX = -1;
  nl->PreposY = -1;

  nl->off_buttons = 0;
  nl->on_buttons = 0;

  nl->style_focus = NULL;
  nl->style_unfocus = NULL;
  nl->style_sticky = NULL;
}

name_list *
style_new (void)
{
  name_list *nl;

  nl = (name_list *) safemalloc (sizeof (name_list));

  style_init (nl);

  /* add the style to the end of the list of styles */
  if (Scr.TheList == NULL)
    Scr.TheList = nl;
  else
    {
      name_list *nptr;
      for (nptr = Scr.TheList; nptr->next != NULL; nptr = nptr->next);
      nptr->next = nl;
    }

  return nl;
}

void
style_delete (name_list * style)
{
  name_list *s1;
  name_list *s2;

  /* remove ourself from the list */
  for (s1 = s2 = Scr.TheList; s1 != NULL; s2 = s1, s1 = s1->next)
    if (s1 == style)
      break;
  if (s1 != NULL)
    {
      if (Scr.TheList == s1)
	Scr.TheList = s1->next;
      else
	s2->next = s1->next;
    }

  /* delete members */
  if (style->name != NULL)
    free (style->name);
  if (style->icon_file != NULL)
    free (style->icon_file);
  if (style->style_focus != NULL)
    free (style->style_focus);
  if (style->style_sticky != NULL)
    free (style->style_sticky);
  if (style->style_unfocus != NULL)
    free (style->style_unfocus);

  /* free our own mem */
  free (style);
}

void
style_fill_by_name (name_list * nl, const char *name, const char *icon_name, const char *res_name, const char *res_class)
{
  name_list *nptr;

  nl->off_flags = 0;
  nl->off_buttons = 0;
  for (nptr = Scr.TheList; nptr != NULL; nptr = nptr->next)
    {
      int match = 0;
      if (name != NULL && matchWildcards (nptr->name, name) == True)
	match = 1;
      if (icon_name != NULL && matchWildcards (nptr->name, icon_name) == True)
	match = 1;
      if (res_name != NULL && matchWildcards (nptr->name, res_name) == True)
	match = 1;
      if (res_class != NULL && matchWildcards (nptr->name, res_class) == True)
	match = 1;
      if (match)
	{
	  if (nptr->icon_file != NULL)
	    nl->icon_file = nptr->icon_file;
	  if (nptr->off_flags & STAYSONDESK_FLAG)
	    nl->Desk = nptr->Desk;
	  if (nptr->off_flags & VIEWPORTX_FLAG)
	    nl->ViewportX = nptr->ViewportX;
	  if (nptr->off_flags & VIEWPORTY_FLAG)
	    nl->ViewportY = nptr->ViewportY;
	  if (nptr->off_flags & PREPOS_FLAG)
	  {
	    nl->PreposX = nptr->PreposX;
	    nl->PreposY = nptr->PreposY;
	    nl->PreposWidth = nptr->PreposWidth;
	    nl->PreposHeight = nptr->PreposHeight;
	    nl->PreposFlags = nptr->PreposFlags;
	  }
	  if (nptr->off_flags & BW_FLAG)
	    nl->border_width = nptr->border_width;
	  if (nptr->off_flags & NOBW_FLAG)
	    nl->resize_width = nptr->resize_width;
	  if (nptr->off_flags & STYLE_FOCUS_FLAG)
	    nl->style_focus = nptr->style_focus;
	  if (nptr->off_flags & STYLE_UNFOCUS_FLAG)
	    nl->style_unfocus = nptr->style_unfocus;
	  if (nptr->off_flags & STYLE_STICKY_FLAG)
	    nl->style_sticky = nptr->style_sticky;
	  if (nptr->off_flags & LAYER_FLAG)
	    nl->layer = nptr->layer;
	  nl->off_flags |= nptr->off_flags;
	  nl->off_flags &= ~nptr->on_flags;
	  nl->off_buttons |= nptr->off_buttons;
	  nl->off_buttons &= ~nptr->on_buttons;
	}
    }
}
