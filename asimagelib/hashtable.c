/*
 *  Copyright (c) 1997 Alfredo K. Kojima
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "../configure.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "../include/hashtable.h"

extern void *safemalloc (size_t size);

#define INIT_TABLE_SIZE	239

/*
 *----------------------------------------------------------------------
 * table_init--
 *      Initializes a hashtable with a initial size = INIT_TABLE_SIZE
 * The table must already be allocated.
 * 
 * Returns:
 *      The table.
 *---------------------------------------------------------------------- 
 */
HashTable *
table_init (HashTable * table)
{
  table->elements = 0;
  table->size = INIT_TABLE_SIZE;
  table->table = safemalloc (INIT_TABLE_SIZE * sizeof (HashEntry *));
  table->last = NULL;
  memset (table->table, 0, INIT_TABLE_SIZE * sizeof (HashEntry *));

  return table;
}

static unsigned long
hash_ikey (unsigned long key)
{
  unsigned long h = 0, g;
  int i;
  char skey[sizeof (int)];

  memcpy (skey, &key, sizeof (unsigned long));
  for (i = 0; i < sizeof (unsigned long); i++)
    {
      h = (h << 4) + skey[i];
      if ((g = h & 0xf0000000))
	h ^= g >> 24;
      h &= ~g;
    }
  return h;
}



static void
rebuild_itable (HashTable * table)
{
  HashTable newtable;
  HashEntry *entry;
  int i;

  newtable.last = NULL;
  newtable.elements = 0;
  newtable.size = table->size * 2;
  newtable.table = safemalloc (newtable.size * sizeof (HashEntry *));
  memset (newtable.table, 0, newtable.size * sizeof (HashEntry *));
  for (i = 0; i < table->size; i++)
    {
      entry = table->table[i];
      while (entry)
	{
	  table_iput (&newtable, entry->key, entry->data);
	  entry = entry->next;
	}
    }
  table_idestroy (table);
  table->elements = newtable.elements;
  table->size = newtable.size;
  table->table = newtable.table;
}


void
table_iput (HashTable * table, unsigned long key, unsigned long data)
{
  unsigned long hkey;
  HashEntry *nentry;

  nentry = safemalloc (sizeof (HashEntry));
  nentry->key = key;
  nentry->data = data;
  hkey = hash_ikey (key) % table->size;
  /* collided */
  if (table->table[hkey] != NULL)
    {
      nentry->next = table->table[hkey];
      table->table[hkey] = nentry;
    }
  else
    {
      nentry->next = NULL;
      table->table[hkey] = nentry;
    }
  table->elements++;

  nentry->nptr = NULL;
  nentry->pptr = table->last;
  if (table->last)
    table->last->nptr = nentry;
  table->last = nentry;

  if (table->elements > (table->size * 3) / 2)
    {
#ifdef DEBUG
      printf ("rebuilding hash table...\n");
#endif
      rebuild_itable (table);
    }
}


int
table_iget (HashTable * table, unsigned long key, unsigned long *data)
{
  unsigned long hkey;
  HashEntry *entry;

  hkey = hash_ikey (key) % table->size;
  entry = table->table[hkey];
  while (entry != NULL)
    {
      if (entry->key == key)
	{
	  *data = entry->data;
	  return 1;
	}
      entry = entry->next;
    }
  return 0;
}



static HashEntry *
delete_fromilist (HashTable * table, HashEntry * entry, unsigned long key)
{
  HashEntry *next;

  if (entry == NULL)
    return NULL;
  if (entry->key == key)
    {
      if (table->last == entry)
	table->last = entry->pptr;
      if (entry->nptr)
	entry->nptr->pptr = entry->pptr;
      if (entry->pptr)
	entry->pptr->nptr = entry->nptr;
      next = entry->next;
      free (entry);
      return next;
    }
  entry->next = delete_fromilist (table, entry->next, key);
  return entry;
}

void
table_idelete (HashTable * table, unsigned long key)
{
  unsigned long hkey;

  hkey = hash_ikey (key) % table->size;
  table->table[hkey] = delete_fromilist (table, table->table[hkey], key);
  table->elements--;
}


void
table_idestroy (HashTable * table)
{
  HashEntry *entry, *next;
  int i;

  for (i = 0; i < table->size; i++)
    {
      entry = table->table[i];
      while (entry)
	{
	  next = entry->next;
	  entry = next;
	}
    }
  free (table->table);
}
