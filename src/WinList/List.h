/*
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
 */

/* Structure definitions */
typedef struct item {
  long id;
  char *name;
  long flags;
  struct item *next;
} Item;

typedef struct {
  Item *head,*tail;
  int count;
} List;

/* Function Prototypes */
void InitList(List *list);
void AddItem(List *list, long id, long flags);
int FindItem(List *list, long id);
int UpdateItemName(List *list, long id, char *string);
int UpdateItemFlags(List *list, long id, long flags);
void FreeItem(Item *ptr);
int DeleteItem(List *list,long id);
void FreeList(List *list);
void PrintList(List *list);
char *ItemName(List *list, int n);
long ItemFlags(List *list, long id );
long XorFlags(List *list, int n, long value);
int ItemCount(List *list);
long ItemID(List *list, int n);
void CopyItem(List *dest,List *source,int n);
