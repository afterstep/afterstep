/*
 * Copyright (c) 1998 Rafal Wierzbicki <rafal@mcss.mcmaster.ca>
 * Copyright (c) 1998 Makoto Kato <m_kato@ga2.so-net.ne.jp>
 * Copyright (c) 1998 Rene Fichter <ceezaer@cyberspace.org>
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (c) 1994 Mike Finger <mfinger@mermaid.micro.umn.edu>
 *                    or <Mike_Finger@atk.com>
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
 * $Id: ButtonArray.c,v 1.1 2000/10/20 03:27:57 sashav Exp $
 */

#include "../../configure.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/ascolor.h"
#include "../../include/stepgfx.h"

#include "ButtonArray.h"
#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#define max(a,b) (((a)>(b)) ? (a) : (b))
#endif


extern Display *dpy;
extern Window window;

extern int w_width, w_height, win_x, win_y;
extern int Justify;
extern int OrientVert;
extern int MaxWidth;
extern int Balloons;
/******************************************************************************
  InitArray - Initialize the arrary of buttons
******************************************************************************/
void
InitArray (ButtonArray * array, int x, int y, int w, int h)
{
  array->count = 0;
  array->head = array->tail = NULL;
  array->x = x;
  array->y = y;
  array->w = w;
  array->h = h;
}

/******************************************************************************
  UpdateArray - Update the array specifics.  x,y, width, height
******************************************************************************/
void
UpdateArray (MyStyle * Style, ButtonArray * array, int x, int y, int w, int h)
{
  Button *temp;
  if (x != -1)
    array->x = x;
  if (y != -1)
    array->y = y;
  if (w != -1)
    array->w = w;
  /*if (h != -1)
     array->h = h; */
  array->h = Style->font.height + 6;
  for (temp = array->head; temp != NULL; temp = temp->next)
    {
      temp->needsupdate = 1;
    }
}

/******************************************************************************
  AddButton - Allocate space for and add the button to the bottom
******************************************************************************/
int
AddButton (MyStyle * Style, ButtonArray * array, char *title, int up)
{
  Button *new;
  new = (Button *) safemalloc (sizeof (Button));
  new->title = safemalloc (strlen (title) + 1);
  new->truncate_title = NULL;
  new->balloon = NULL;
  strcpy (new->title, title);
  new->up = up;
  new->tw = XTextWidth (Style->font.font, title, strlen (title));
  new->truncatewidth = 0;
  new->next = NULL;
  new->needsupdate = 1;
  new->set = 0;

  if (array->head == NULL)
    array->head = array->tail = new;
  else
    {
      array->tail->next = new;
      array->tail = new;
    }
  array->count++;
  return (array->count - 1);
}

/***************************************************************************
 * CurrentButton - translates mouse cursor position to the button it is 
 *                 pointed to
 ****************************************************************************/
Button *
CurrentButton (ButtonArray * array, int x, int y)
{
  Window root, child;
  int root_x, root_y, wx, wy;
  unsigned int key_buttons;
  Button *temp = NULL;
  int button;

  XQueryPointer (dpy, window, &root, &child, &root_x, &root_y, &wx, &wy,
		 &key_buttons);
  if (array->count == 0 || w_height == 0)
    return temp;

  if (OrientVert)
    button = wy / (w_height / array->count);
  else
    button = wx / (w_width / array->count);

  if (button > array->count)
    button = array->count;

  temp = find_n (array, button);
  return (temp);
}
/******************************************************************************
  UpdateButton - Change the name/state of a button
******************************************************************************/
int
UpdateButton (MyStyle * Style, ButtonArray * array, int butnum, char *title, int up)
{
  Button *temp;
  temp = find_n (array, butnum);
  if (temp != NULL)
    {
      if (title != NULL)
	{
	  temp->title = (char *) realloc (temp->title, strlen (title) + 1);
	  strcpy (temp->title, title);
	  temp->tw = XTextWidth (Style->font.font, title, strlen (title));
	  if (temp->truncate_title)
	    free (temp->truncate_title);
	  temp->truncate_title = NULL;
	  temp->truncatewidth = 0;
	}
      if (up != -1)
	temp->up = up;
    }
  else
    return -1;
  temp->needsupdate = 1;
  return 1;
}

/******************************************************************************
  RemoveButton - Delete a button from the list
******************************************************************************/
void
RemoveButton (ButtonArray * array, int butnum)
{
  Button *temp, *temp2;
  if (butnum == 0)
    {
      temp2 = array->head;
      temp = array->head = array->head->next;
    }
  else
    {
      temp = find_n (array, butnum - 1);
      if (temp == NULL)
	return;
      temp2 = temp->next;
      temp->next = temp2->next;
    }

  if (array->count > 0)
    array->count--;

  if (array->tail == temp2)
    array->tail = temp;

  FreeButton (temp2);

  if (temp != array->head)
    temp = temp->next;
  for (; temp != NULL; temp = temp->next)
    temp->needsupdate = 1;
}

/******************************************************************************
  find_n - Find the nth button in the list (Use internally)
******************************************************************************/
Button *
find_n (ButtonArray * array, int n)
{
  Button *temp;
  int i;
  temp = array->head;
  for (i = 0; i < n && temp != NULL; i++, temp = temp->next);
  return temp;
}

/******************************************************************************
  FreeButton - Free space allocated to a button
******************************************************************************/
void
FreeButton (Button * ptr)
{
  if (ptr != NULL)
    {
      if (ptr->balloon)
	balloon_delete (ptr->balloon);
      if (ptr->title != NULL)
	free (ptr->title);
      if (ptr->truncate_title)
	free (ptr->truncate_title);
      free (ptr);
    }
}

/******************************************************************************
  FreeAllButtons - Free the whole array of buttons
******************************************************************************/
void
FreeAllButtons (ButtonArray * array)
{
  Button *temp, *temp2;
  for (temp = array->head; temp != NULL;)
    {
      temp2 = temp;
      temp = temp->next;
      FreeButton (temp2);
    }
}

/******************************************************************************
  DoButton - Draw the specified button.  (Used internally)
                - minor additions to this function to allow for 3d text
                   Rafal Wierzbicki 1998
******************************************************************************/
void
DoButton (MyStyle * Style, Button * ptr, int x, int y, int w, int h)
{
  int up, Fontheight, newx;
  int posx, posy;
  GC fore_gc, back_gc, shadow_gc, relief_gc;
  char *tmp = NULL;

  mystyle_get_global_gcs (Style, &fore_gc, &back_gc, &relief_gc, &shadow_gc);

  /* Buttons change size, it's best to just delete a balloon and recreate it
   * than to cope with changing sizes and titles */

  if (Balloons)
    {
      if (ptr->balloon)
	balloon_delete (ptr->balloon);

      ptr->balloon = balloon_new_with_text (dpy, window, ptr->title);
      balloon_set_active_rectangle (ptr->balloon, x, y, w, h);
    }

  up = ptr->up;
  Fontheight = Style->font.height;

  XClearArea (dpy, window, x, y, w, h, False);
  XDrawLine (dpy, window, (up) ? relief_gc : shadow_gc,
	     x, y, x + w - 1, y);
  XDrawLine (dpy, window, (up) ? relief_gc : shadow_gc,
	     x, y + 1, x + w - 2, y + 1);
  XDrawLine (dpy, window, (up) ? relief_gc : shadow_gc,
	     x, y, x, y + h - 1);
  XDrawLine (dpy, window, (up) ? relief_gc : shadow_gc,
	     x + 1, y, x + 1, y + h - 2);
  XDrawLine (dpy, window, (up) ? shadow_gc : relief_gc,
	     x, y + h, x + w, y + h);
  XDrawLine (dpy, window, (up) ? shadow_gc : relief_gc,
	     x + 1, y + h - 1, x + w, y + h - 1);
  XDrawLine (dpy, window, (up) ? shadow_gc : relief_gc,
	     x + w, y + h, x + w, y);
  XDrawLine (dpy, window, (up) ? shadow_gc : relief_gc,
	     x + w - 1, y + h, x + w - 1, y + 1);

  if ((w - ptr->tw) < 8)
    {

      if (ptr->truncatewidth != w)
	{
	  char *string = ptr->title;
	  int title_len = strlen (string);

	  while (title_len && (w - XTextWidth (Style->font.font, string, title_len)) < 8)
	    {
	      title_len--;
	      if (Justify >= 0)
		string++;
	      if (Justify == 0)
		title_len--;
#ifdef I18N
	      if (IsDBCSLeadByte (ptr->title, (Justify > 0) ? string - ptr->title : title_len) == DBCS_SECOND)
		{
		  if (Justify > 0)
		    string++;
		  title_len--;
		}
#endif
	    }

	  ptr->truncatewidth = w;

	  if (ptr->truncate_title)
	    free (ptr->truncate_title);
	  ptr->truncate_title = (char *) safemalloc (title_len + 1);
	  strncpy (ptr->truncate_title, string, title_len);
	  ptr->truncate_title[title_len] = '\0';
	}
      tmp = ptr->truncate_title;
    }
  else
    tmp = ptr->title;

  if (Justify == 0)		/* centering */
    {
      int twidth = XTextWidth (Style->font.font, tmp, strlen (tmp));
      newx = max ((w - twidth) / 2, 4);
    }
  else
    newx = 4;

  posx = x + newx + (up ? 0 : 1);
  posy = y + 3 + Style->font.y + (up ? 0 : 1);

  if (tmp)
    mystyle_draw_text (window, Style, tmp, posx, posy);
  ptr->needsupdate = 0;
}

/******************************************************************************
  DrawButtonArray - Draw the whole array (all=1), or only those that need.
******************************************************************************/
void
DrawButtonArray (MyStyle * Style, ButtonArray * array, int all)
{
  Button *temp;
  int pos_x, pos_y, pos_h, pos_w;

  int i;
  for (temp = array->head, i = 0; temp != NULL; temp = temp->next, i++)
    if (temp->needsupdate || all)
      {
	if (OrientVert)
	  {
	    pos_x = array->x;
	    pos_y = array->y + (i * (array->h + 1));
	    pos_h = array->h;
	    pos_w = array->w;
	    DoButton (Style, temp, pos_x, pos_y, pos_w, pos_h);
	  }
	else
	  {
	    pos_x = array->x + (i * ((float) array->w / array->count));
	    pos_y = array->y;
	    pos_w = array->w / array->count;
	    pos_h = array->h;
	    DoButton (Style, temp, pos_x, pos_y, pos_w, pos_h);
	  }
      }
}

/******************************************************************************
  SwitchButton - Alternate the state of a button
******************************************************************************/
void
SwitchButton (MyStyle * Style, ButtonArray * array, int butnum)
{
  Button *temp;
  temp = find_n (array, butnum);
  temp->up = !temp->up;
  temp->needsupdate = 1;
  DrawButtonArray (Style, array, 0);
}

/******************************************************************************
  WhichButton - Based on x,y which button was pressed
******************************************************************************/
int
WhichButton (ButtonArray * array, int x, int y)
{
  int num;
  if (OrientVert)
    num = y / (array->h + 1);
  else
    num = x / ((float) array->w / array->count);

  if (x < array->x || x > array->x + array->w || num < 0 || num > array->count - 1)
    num = -1;

  return (num);
}

/******************************************************************************
  ButtonName - Return the name of the button
******************************************************************************/
char *
ButtonName (ButtonArray * array, int butnum)
{
  Button *temp;
  temp = find_n (array, butnum);
  return temp->title;
}

/******************************************************************************
  PrintButtons - Print the array of button names to the console. (Debugging)
******************************************************************************/
void
PrintButtons (ButtonArray * array)
{
  Button *temp;
  fprintf (stderr, "List of Buttons:\n");
  for (temp = array->head; temp != NULL; temp = temp->next)
    fprintf (stderr, "   %s is %s\n", temp->title, (temp->up) ? "Up" : "Down");
}

/******************************************************************************
  ButtonArrayMaxWidth - Calculate the width needed for the widest title
******************************************************************************/
int
ButtonArrayMaxWidth (ButtonArray * array)
{
  Button *temp;
  int x = 0;
  for (temp = array->head; temp != NULL; temp = temp->next)
    x = max (temp->tw, x);
  return x;
}

#ifdef I18N
/******************************************************************************
  IsDBCSLeadByte - 
******************************************************************************/
int
IsDBCSLeadByte (char *s, int i)
{
  int j;

  if (((unsigned char) s[i]) < (unsigned char) 0x80)
    return DBCS_NOT;

  for (j = i; j >= 0; j--)
    {
      if (((unsigned char) s[j]) >= (unsigned char) 0x80)
	continue;
      else
	{
	  if ((i - j) % 2)
	    return DBCS_LEAD;
	  else
	    return DBCS_SECOND;
	}
    }

  if (i % 2)
    return DBCS_LEAD;
  else
    return DBCS_SECOND;
}
#endif
