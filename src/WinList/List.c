/*
 * Copyright (c) 1999 Rafal Wierzbicki <rafal@mcss.mcmaster.ca>
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
 * $Id: List.c,v 1.1 2000/10/20 03:27:57 sashav Exp $
 */

#include "../../configure.h"
#include <stdio.h>
#include <stdlib.h>
#include "List.h"
#include "../../include/aftersteplib.h"
#include "../../include/module.h"

#ifdef BROKEN_SUN_HEADERS
#include "../../include/sun_headers.h"
#endif

#ifdef NEEDS_ALPHA_HEADER
#include "../../include/alpha_header.h"
#endif /* NEEDS_ALPHA_HEADER */


/******************************************************************************
  InitList - Initialize the list
******************************************************************************/
void
InitList (List * list)
{
  list->head = list->tail = NULL;
  list->count = 0;
}

/******************************************************************************
  AddItem - Allocates spaces for and appends an item to the list
******************************************************************************/
void
AddItem (List * list, long id, long flags)
{
  Item *new;
  new = (Item *) safemalloc (sizeof (Item));
  new->id = id;
  new->name = NULL;
  new->flags = flags;
  new->next = NULL;

  if (list->tail == NULL)
    list->head = list->tail = new;
  else
    {
      list->tail->next = new;
      list->tail = new;
    }
  list->count++;
}

/******************************************************************************
  FindItem - Find the item in the list matching the id
******************************************************************************/
int
FindItem (List * list, long id)
{
  Item *temp;
  int i;
  for (i = 0, temp = list->head; temp != NULL && temp->id != id; i++, temp = temp->next);
  if (temp == NULL)
    return -1;
  return i;
}

/******************************************************************************
  UpdateItem - Update the item in the list, setting name & flags as necessary.
******************************************************************************/
int
UpdateItemName (List * list, long id, char *string)
{
  Item *temp;
  int i;
  for (i = 0, temp = list->head; temp != NULL && id != temp->id; i++, temp = temp->next);
  if (temp == NULL)
    return -1;
  if (string != NULL)
    {
      if (temp->name == NULL)
	temp->name = (char *) safemalloc (strlen (string) + 1);
      else
	{
	  if (strlen (string) == strlen (temp->name))
	    {
	      if (strcmp (string, temp->name) == 0)
		return -1;
	    }
	  temp->name = (char *) realloc (temp->name, strlen (string) + 1);
	}
    }
  strcpy (temp->name, string);
  return i;
}

int
UpdateItemFlags (List * list, long id, long flags)
{
  Item *temp;
  int i;
  for (i = 0, temp = list->head; temp != NULL && id != temp->id; i++, temp = temp->next);
  if (temp == NULL)
    return -1;
  if (flags != -1)
    temp->flags = flags;
  return i;
}

/******************************************************************************
  FreeItem - Frees allocated space for an Item
******************************************************************************/
void
FreeItem (Item * ptr)
{
  if (ptr != NULL)
    {
      if (ptr->name != NULL)
	free (ptr->name);
      free (ptr);
    }
}

/******************************************************************************
  DeleteItem - Deletes an item from the list
******************************************************************************/
int
DeleteItem (List * list, long id)
{
  Item *temp, *temp2;
  int i;
  if (list->head == NULL)
    return -1;
  if (list->head->id == id)
    {
      temp2 = list->head;
      temp = list->head = list->head->next;
      i = -1;
    }
  else
    {
      for (i = 0, temp = list->head; temp->next != NULL && temp->next->id != id;
	   i++, temp = temp->next);
      if (temp->next == NULL)
	return -1;
      temp2 = temp->next;
      temp->next = temp2->next;
    }

  if (temp2 == list->tail)
    list->tail = temp;

  FreeItem (temp2);
  list->count--;
  return i + 1;
}

/******************************************************************************
  FreeList - Free the entire list of Items
******************************************************************************/
void
FreeList (List * list)
{
  Item *temp, *temp2;
  for (temp = list->head; temp != NULL;)
    {
      temp2 = temp;
      temp = temp->next;
      FreeItem (temp2);
    }
  list->count = 0;
}

/******************************************************************************
  PrintList - Print the list of item on the console. (Debugging)
******************************************************************************/
void
PrintList (List * list)
{
  Item *temp;
  fprintf (stderr, "List of Items:\n");
  fprintf (stderr, "   %10s %-15s %-15s %-15s %-15s Flgs\n", "ID", "Name", "I-Name",
	   "R-Name", "R-Class");
  fprintf (stderr, "   ---------- --------------- --------------- --------------- --------------- ----\n");
  for (temp = list->head; temp != NULL; temp = temp->next)
    {
      fprintf (stderr, "   %10ld %-15.15s %4ld\n", temp->id,
	       (temp->name == NULL) ? "<null>" : temp->name,
	       temp->flags);
    }
}

/******************************************************************************
  ItemName - Return the name of an Item
******************************************************************************/
char *
ItemName (List * list, int n)
{
  Item *temp;
  int i;
  for (i = 0, temp = list->head; temp != NULL && i < n; i++, temp = temp->next);
  if (temp == NULL)
    return NULL;
  return temp->name;
}

/******************************************************************************
  ItemFlags - Return the flags for an item
******************************************************************************/
long
ItemFlags (List * list, long id)
{
  Item *temp;
  for (temp = list->head; temp != NULL && id != temp->id; temp = temp->next);
  if (temp == NULL)
    return -1;
  else
    return temp->flags;
}

/******************************************************************************
  XorFlags - Exclusive of the flags with the specified value.
******************************************************************************/
long
XorFlags (List * list, int n, long value)
{
  Item *temp;
  int i;
  long ret;
  for (i = 0, temp = list->head; temp != NULL && i < n; i++, temp = temp->next)
    if (temp == NULL)
      return -1;
  ret = temp->flags;
  temp->flags ^= value;
  return ret;
}

/******************************************************************************
  ItemCount - Return the number of items inthe list
******************************************************************************/
int
ItemCount (List * list)
{
  return list->count;
}

/******************************************************************************
  ItemID - Return the ID of the item in the list.
******************************************************************************/
long
ItemID (List * list, int n)
{
  Item *temp;
  int i;
  for (i = 0, temp = list->head; temp != NULL && i < n; i++, temp = temp->next);
  if (temp == NULL)
    return -1;
  return temp->id;
}

/******************************************************************************
  CopyItem - Copy an item from one list to another
******************************************************************************/
void
CopyItem (List * dest, List * source, int n)
{
  Item *temp;
  int i;
  for (i = 0, temp = source->head; temp != NULL && i < n; i++, temp = temp->next);
  if (temp == NULL)
    return;
  AddItem (dest, temp->id, temp->flags);
  UpdateItemName (dest, temp->id, temp->name);
  DeleteItem (source, temp->id);
}
