/*
 * Copyright (c) 2000 Andrew Ferguson <andrew@owsla.cjb.net>
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

/*#define DO_CLOCKING      */

#include "../configure.h"

#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include <X11/Xmd.h>

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/ashash.h"

ASHashKey default_hash_func (ASHashableValue value, ASHashKey hash_size)
{
	return value.long_val % hash_size;
}

long
default_compare_func (ASHashableValue value1, ASHashableValue value2)
{
	return ((long)value1.long_val - (long)value2.long_val);
}

long
desc_long_compare_func (ASHashableValue value1, ASHashableValue value2)
{
    return ((long)value2.long_val - (long)value1.long_val);
}

void
init_ashash (ASHashTable * hash, Bool freeresources)
{
	if (hash)
	{
		if (freeresources)
			if (hash->buckets)
				free (hash->buckets);
		memset (hash, 0x00, sizeof (ASHashTable));
	}
}

ASHashTable  *
create_ashash (ASHashKey size,
			   ASHashKey (*hash_func) (ASHashableValue, ASHashKey),
			   long (*compare_func) (ASHashableValue, ASHashableValue),
			   void (*item_destroy_func) (ASHashableValue, void *))
{
	ASHashTable  *hash;

	if (size <= 0)
		size = DEFAULT_HASH_SIZE;

	hash = safemalloc (sizeof (ASHashTable));
	init_ashash (hash, False);

	hash->buckets = safemalloc (sizeof (ASHashBucket) * size);
	memset (hash->buckets, 0x00, sizeof (ASHashBucket) * size);

	hash->size = size;

	if (hash_func)
		hash->hash_func = hash_func;
	else
		hash->hash_func = default_hash_func;

	if (compare_func)
		hash->compare_func = compare_func;
	else
		hash->compare_func = default_compare_func;

	hash->item_destroy_func = item_destroy_func;

	return hash;
}

static void
destroy_ashash_bucket (ASHashBucket * bucket, void (*item_destroy_func) (ASHashableValue, void *))
{
	register ASHashItem *item, *next;

	for (item = *bucket; item != NULL; item = next)
	{
		next = item->next;
		if (item_destroy_func)
			item_destroy_func (item->value, item->data);
		free (item);
	}
	*bucket = NULL;
}

void
print_ashash (ASHashTable * hash, void (*item_print_func) (ASHashableValue value))
{
	register int  i;
	ASHashItem   *item;

	for (i = 0; i < hash->size; i++)
	{
		if (hash->buckets[i] == NULL)
			continue;
		fprintf (stderr, "Bucket # %d:", i);
		for (item = hash->buckets[i]; item != NULL; item = item->next)
			if (item_print_func)
				item_print_func (item->value);
			else
				fprintf (stderr, "[0x%lX(%ld)]", item->value.long_val, item->value.long_val);
		fprintf (stderr, "\n");
	}
}

void
destroy_ashash (ASHashTable ** hash)
{
	if (*hash)
	{
		register int  i;

		for (i = (*hash)->size - 1; i >= 0; i--)
			if ((*hash)->buckets[i])
				destroy_ashash_bucket (&((*hash)->buckets[i]), (*hash)->item_destroy_func);

		init_ashash (*hash, True);
		free (*hash);
		*hash = NULL;
	}
}

static        ASHashResult
add_item_to_bucket (ASHashBucket * bucket, ASHashItem * item, long (*compare_func) (ASHashableValue, ASHashableValue))
{
	ASHashItem  **tmp;

	/* first check if we already have this item */
	for (tmp = bucket; *tmp != NULL; tmp = &((*tmp)->next))
	{
		register long res = compare_func ((*tmp)->value, item->value);

		if (res == 0)
			return ((*tmp)->data == item->data) ? ASH_ItemExistsSame : ASH_ItemExistsDiffer;
		else if (res > 0)
			break;
	}
	/* now actually add this item */
	item->next = (*tmp);
	*tmp = item;
	return ASH_Success;
}

ASHashResult
add_hash_item (ASHashTable * hash, ASHashableValue value, void *data)
{
	ASHashKey     key;
	ASHashItem   *item;
	ASHashResult  res;

	if (hash == NULL)
        return ASH_BadParameter;

	key = hash->hash_func (value, hash->size);
	if (key >= hash->size)
        return ASH_BadParameter;

	item = safemalloc (sizeof (ASHashItem));
	item->next = NULL;
	item->value = value;
	item->data = data;

	res = add_item_to_bucket (&(hash->buckets[key]), item, hash->compare_func);
	if (res == ASH_Success)
	{
		hash->items_num++;
		if (hash->buckets[key]->next == NULL)
			hash->buckets_used++;
	} else
		free (item);
	return res;
}

static ASHashItem **
find_item_in_bucket (ASHashBucket * bucket,
					 ASHashableValue value, long (*compare_func) (ASHashableValue, ASHashableValue))
{
	register ASHashItem **tmp;
	register long res;

	/* first check if we already have this item */
	for (tmp = bucket; *tmp != NULL; tmp = &((*tmp)->next))
	{
		res = compare_func ((*tmp)->value, value);
		if (res == 0)
			return tmp;
		else if (res > 0)
			break;
	}
	return NULL;
}

ASHashResult
get_hash_item (ASHashTable * hash, ASHashableValue value, void **trg)
{
	ASHashKey     key;
	ASHashItem  **pitem = NULL;

	if (hash)
	{
		key = hash->hash_func (value, hash->size);
		if (key < hash->size)
			pitem = find_item_in_bucket (&(hash->buckets[key]), value, hash->compare_func);
	}
	if (pitem)
		if (*pitem)
		{
			if (trg)
				*trg = (*pitem)->data;
			return ASH_Success;
		}
	return ASH_ItemNotExists;
}

ASHashResult
remove_hash_item (ASHashTable * hash, ASHashableValue value, void **trg, Bool destroy)
{
	ASHashKey     key = 0;
	ASHashItem  **pitem = NULL;

	if (hash)
	{
		key = hash->hash_func (value, hash->size);
		if (key < hash->size)
			pitem = find_item_in_bucket (&(hash->buckets[key]), value, hash->compare_func);
	}
	if (pitem)
		if (*pitem)
		{
			ASHashItem   *next;

			if (trg)
				*trg = (*pitem)->data;

			next = (*pitem)->next;
			if (hash->item_destroy_func && destroy)
				hash->item_destroy_func ((*pitem)->value, (trg) ? NULL : (*pitem)->data);
			free (*pitem);
			*pitem = next;
			if (hash->buckets[key] == NULL)
				hash->buckets_used--;
			hash->items_num--;

			return ASH_Success;
		}
	return ASH_ItemNotExists;
}

unsigned long
sort_hash_items (ASHashTable * hash, ASHashableValue * values, void **data, unsigned long max_items)
{
	if (hash)
	{
		ASHashBucket *buckets;
		register ASHashKey i;
		ASHashKey     k = 0, top = hash->buckets_used - 1, bottom = 0;
		unsigned long count_in = 0;

		if (hash->buckets_used == 0 || hash->items_num == 0)
			return 0;

		if (max_items == 0)
			max_items = hash->items_num;

		buckets = safemalloc (hash->buckets_used * sizeof (ASHashBucket));
		for (i = 0; i < hash->size; i++)
			if (hash->buckets[i])
				buckets[k++] = hash->buckets[i];
		while (max_items-- > 0)
		{
			k = bottom;
			for (i = bottom + 1; i <= top; i++)
			{
				if (buckets[i])
					if (hash->compare_func (buckets[k]->value, buckets[i]->value) > 0)
						k = i;
			}
            if (values)
				*(values++) = buckets[k]->value;
			if (data)
				*(data++) = buckets[k]->data;
			count_in++;
			buckets[k] = buckets[k]->next;
			/* a little optimization : */
			while (buckets[bottom] == NULL && bottom < top)
				bottom++;
			while (buckets[top] == NULL && top > bottom)
				top--;
			if( buckets[top] == NULL ) 
				break;
		}
		free (buckets);
		return count_in;
	}
	return 0;
}

unsigned long
list_hash_items (ASHashTable * hash, ASHashableValue * values, void **data, unsigned long max_items)
{
	unsigned long count_in = 0;

	if (hash)
		if (hash->buckets_used > 0 && hash->items_num > 0)
		{
			register ASHashItem *item;
			ASHashKey     i;

			if (max_items == 0)
				max_items = hash->items_num;

			for (i = 0; i < hash->size; i++)
				for (item = hash->buckets[i]; item != NULL; item = item->next)
				{
					if (values)
						*(values++) = item->value;
					if (data)
						*(data++) = item->data;
					if (++count_in >= max_items)
						return count_in;
				}
		}
	return count_in;
}

/***** Iterator functionality *****/

Bool
start_hash_iteration (ASHashTable * hash, ASHashIterator * iterator)
{
	if (iterator && hash)
	{
		register int  i;

		for (i = 0; i < hash->size; i++)
			if (hash->buckets[i] != NULL)
				break;
		if (i < hash->size)
		{
			iterator->hash = hash;
			iterator->curr_bucket = i;
            iterator->curr_item = &(hash->buckets[i]);
			return True;
		}
	}
	return False;
}

Bool
next_hash_item (ASHashIterator * iterator)
{
	if (iterator)
		if (iterator->hash && iterator->curr_item)
		{
            ASHashItem **curr = iterator->curr_item;

            if( *curr )
                curr = &((*curr)->next) ;

            iterator->curr_item = curr ;

            if (*curr == NULL)
			{
				register int  i;

				for (i = iterator->curr_bucket + 1; i < iterator->hash->size; i++)
					if (iterator->hash->buckets[i] != NULL)
						break;
				if (i < iterator->hash->size)
				{
                    iterator->curr_item = &(iterator->hash->buckets[i]);
					iterator->curr_bucket = i ;
                }
            }

            return (*(iterator->curr_item) != NULL);
		}
	return False;
}

void
remove_curr_hash_item (ASHashIterator * iterator, Bool destroy)
{
	if (iterator)
		if (iterator->hash && iterator->curr_item)
		{
            ASHashItem *removed = *(iterator->curr_item);

            if(removed)
            {
                ASHashTable *hash = iterator->hash ;
                ASHashKey    key = iterator->curr_bucket ;

                *(iterator->curr_item) = removed->next ;
                removed->next = NULL ;

                if (hash->item_destroy_func && destroy)
                    hash->item_destroy_func (removed->value, removed->data);
                free (removed);
                if (hash->buckets[key] == NULL)
                    hash->buckets_used--;
                hash->items_num--;
            }
		}
}

inline ASHashableValue
curr_hash_value (ASHashIterator * iterator)
{
	if (iterator)
	{
        if (iterator->curr_item && *(iterator->curr_item))
            return (*(iterator->curr_item))->value;
	}
	return (ASHashableValue) ((char *)NULL);
}

inline void         *
curr_hash_data (ASHashIterator * iterator)
{
	if (iterator)
	{
        if (iterator->curr_item && *(iterator->curr_item))
            return (*(iterator->curr_item))->data;
	}
	return NULL;
}

/************************************************************************/
/************************************************************************/
/* 	Some usefull implementations 					*/
/************************************************************************/
/* case sensitive strings hash */
ASHashKey
string_hash_value (ASHashableValue value, ASHashKey hash_size)
{
	ASHashKey     hash_key = 0;
	register int  i = 0;
	char         *string = value.string_val;
	register char c;

	do
	{
		c = string[i];
		if (c == '\0')
			break;
		hash_key += (((ASHashKey) c) << i);
		++i ;
	}while( i < ((sizeof (ASHashKey) - sizeof (char)) << 3) );
	return hash_key % hash_size;
}

long
string_compare (ASHashableValue value1, ASHashableValue value2)
{
	register char *str1 = value1.string_val;
	register char *str2 = value2.string_val;
	register int   i = 0 ;

	if (str1 == str2)
		return 0;
	if (str1 == NULL)
		return -1;
	if (str2 == NULL)
		return 1;
	do
	{
		if (str1[i] != str2[i])
			return (long)(str1[i]) - (long)(str2[i]);

	}while( str1[i++] );
	return 0;
}

void
string_destroy (ASHashableValue value, void *data)
{
	if (value.string_val != NULL)
		free (value.string_val);
	if (data != value.string_val && data != NULL)
		free (data);
}

void
string_print (ASHashableValue value)
{
	fprintf (stderr, "[%s]", value.string_val);
}

/* variation for case-unsensitive strings */
ASHashKey
casestring_hash_value (ASHashableValue value, ASHashKey hash_size)
{
	ASHashKey     hash_key = 0;
	register int  i = 0;
	char         *string = value.string_val;
	register char c;

	do
	{
		c = string[i];
		if (c == '\0')
			break;
		if (isupper (c))
			c = tolower (c);
		hash_key += (((ASHashKey) c) << i);
		++i;
	}while(i < ((sizeof (ASHashKey) - sizeof (char)) << 3));

	return hash_key % hash_size;
}

long
casestring_compare (ASHashableValue value1, ASHashableValue value2)
{
	register char *str1 = value1.string_val;
	register char *str2 = value2.string_val;
	register int   i = 0;

	if (str1 == str2)
		return 0;
	if (str1 == NULL)
		return -1;
	if (str2 == NULL)
		return 1;
	do
	{
		char          u1, u2;

		u1 = str1[i];
		u2 = str2[i];
		if (islower (u1))
			u1 = toupper (u1);
		if (islower (u2))
			u2 = toupper (u2);
		if (u1 != u2)
			return (long)u1 - (long)u2;
	}while( str1[i++] );
	return 0;
}

/* variation for config option type of strings */
ASHashKey
option_hash_value (ASHashableValue value, ASHashKey hash_size)
{
	ASHashKey     hash_key = 0;
	register int  i = 0;
	char         *opt = value.string_val;
	register char c;

	do
	{
		c = opt[i];
#define VALID_OPTION_CHAR(c)		(isalnum (c) || (c) == '~' || (c) == '_')
		if (c == '\0' || !VALID_OPTION_CHAR(c))
			break;
		if (isupper (c))
			c = tolower (c);
		hash_key += (((ASHashKey) c) << i);
		++i;
	}while( i < ((sizeof (ASHashKey) - sizeof (char)) << 3) );
	return hash_key % hash_size;
}

long
option_compare (ASHashableValue value1, ASHashableValue value2)
{
	char *str1 = value1.string_val;
	char *str2 = value2.string_val;
	register int i = 0;

	if (str1 == str2)
		return 0;
	if (str1 == NULL)
		return -1;
	if (str2 == NULL)
		return 1;
	while ( str1[i] && str2[i] )
	{
		register char          u1, u2;

		u1 = str1[i];
		u2 = str2[i];
		if( !VALID_OPTION_CHAR(u1) )
			return ( VALID_OPTION_CHAR(u2) )? (long)u1 - (long)u2: 0;
		++i ;

		if (islower (u1))
			u1 = toupper (u1);
		if (islower (u2))
			u2 = toupper (u2);
		if (u1 != u2)
			return (long)u1 - (long)u2;
	}
	if(  str1[i] == str2[i] )
		return 0;
	else if( str1[i] )
		return ( VALID_OPTION_CHAR(str1[i]) )?(long)(str1[i]):0;
	else
		return ( VALID_OPTION_CHAR(str2[i]) )? 0 - (long)(str2[i]):0;
}


/* colors hash */

ASHashKey
color_hash_value (ASHashableValue value, ASHashKey hash_size)
{
	register CARD32 h;
	int           i = 1;
	register unsigned long long_val = value.long_val;

	h = (long_val & 0x0ff);
	long_val = long_val >> 8;

	while (i++ < 4)
	{										   /* for the first 4 bytes we can use simplier loop as there can be no overflow */
		h = (h << 4) + (long_val & 0x0ff);
		long_val = long_val >> 8;
	}
	while (i++ < sizeof (unsigned long))
	{										   /* this loop will only be used on computers with long > 32bit */
		register CARD32 g;

		h = (h << 4) + (long_val & 0x0ff);
		long_val = long_val >> 8;
		if ((g = h & 0xf0000000))
		{
			h ^= g >> 24;
			h &= 0x0fffffff;
		}
	}
	return (ASHashKey) (h % (CARD32) hash_size);
}
