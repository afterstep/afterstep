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

/*#define DO_CLOCKING      */

#define TRUE 1
#define FALSE

#include "../configure.h"

#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include <X11/Xmd.h>

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/ashash.h"

ASHashKey default_hash_func(ASHashableValue value, ASHashKey hash_size)
{
    return value.long_val%hash_size;
}

Bool default_compare_func(ASHashableValue value1, ASHashableValue value2)
{
    return (value1.long_val==value2.long_val);
}

void 
init_ashash( ASHashTable *hash, Bool freeresources )
{
    if( hash )
    {
	if( freeresources )
	    if( hash->buckets ) free( hash->buckets );
	memset( hash, 0x00, sizeof( ASHashTable ));
    }
}

ASHashTable*
create_ashash( ASHashKey size, 
               ASHashKey (*hash_func)(ASHashableValue, ASHashKey), 
	       Bool      (*compare_func)(ASHashableValue, ASHashableValue),
	       void (*item_destroy_func)(ASHashItem *))
{
  ASHashTable *hash;
    
    if( size <= 0 ) return NULL ;
    
    hash = safemalloc( sizeof(ASHashTable) );  
    init_ashash( hash, False );
    
    hash->buckets = safemalloc( sizeof(ASHashBucket)*size );
    memset( hash->buckets, 0x00, sizeof(ASHashBucket)*size );
    
    hash->size = size ;
    
    if( hash_func )     hash->hash_func = hash_func ;
    else		hash->hash_func = default_hash_func ;

    if( compare_func )  hash->compare_func = compare_func ;    
    else		hash->compare_func = default_compare_func ;    
    
    hash->item_destroy_func = item_destroy_func ;
    
    return hash ;
}

static void
destroy_ashash_bucket( ASHashBucket *bucket, void (*item_destroy_func)(ASHashItem *))
{
  register ASHashItem 	*item, *next ;
    for( item = *bucket ; item != NULL ; item = next )
    {
	next = item->next ;
	if( item_destroy_func )
	    item_destroy_func( item );
	else
	    free( item );
    } 
    *bucket = NULL ;
}

void 
destroy_ashash( ASHashTable **hash )
{
    if( *hash )
    {
      register int i ;
        for( i = (*hash)->size-1 ; i >=0 ; i-- )
	    if( (*hash)->buckets[i] )
		destroy_ashash_bucket( &((*hash)->buckets[i]), (*hash)->item_destroy_func );
		
	init_ashash( *hash, True );    
	free( *hash );
	*hash = NULL ;
    }
}

static ASHashResult 
add_item_to_bucket( ASHashBucket *bucket, 
                    ASHashItem *item, 
		    Bool (*compare_func)(ASHashableValue, ASHashableValue) )
{
  ASHashItem *tmp ;
    /* first check if we already have this item */
    for( tmp = *bucket ; tmp != NULL ; tmp = tmp->next )
	if( compare_func( tmp->value, item->value ) )
	    return (tmp->data == item->data )?ASH_ItemExistsSame:ASH_ItemExistsDiffer ;
    /* now actually add this item */
    item->next = (*bucket) ;
    *bucket = item ;
    return ASH_Success ;
}

ASHashResult add_hash_item( ASHashTable *hash, ASHashableValue value, void *data )
{
  ASHashKey key ;
  ASHashItem *item ;
  
    if( hash == NULL ) return False ;

    key = hash->hash_func( value, hash->size );	
    if( key >= hash->size ) return False ;
    
    item = safemalloc( sizeof(ASHashItem) );	
    item->next = NULL ;
    item->value = value ;
    item->data = data ;
    
    return add_item_to_bucket( &(hash->buckets[key]), item, hash->compare_func );
}

void*
get_hash_item( ASHashTable *hash, ASHashableValue value )
{
  ASHashKey key ;
  ASHashItem *item = NULL;
  
    if( hash ) 
    {
	key = hash->hash_func( value, hash->size );	
	if( key < hash->size )
	{
	    
	/* TODO */
	
	}
    }
    return item ;
}




/************************************************************************/
/************************************************************************/
/* 	Some usefull implementations 					*/
/************************************************************************/
ASHashKey option_hash_value (ASHashableValue value, ASHashKey hash_size)
{
  ASHashKey hash_key = 0;
  register int i;
  char* opt = value.string_val ;
  register char c ;
  
  for (i = 0; i < ((sizeof (ASHashKey) - sizeof (char)) << 3); i++)
    {
      c = *(opt++) ;
      if (c == '\0' || !isalnum (c))
	break;
      if( isupper(c) ) c = tolower (c);
      hash_key += (((ASHashKey) c) << i);
    }
  return hash_key % hash_size;
}

ASHashKey casestring_hash_value (ASHashableValue value, ASHashKey hash_size)
{
  ASHashKey hash_key = 0;
  register int i;
  char* string = value.string_val ;
  register char c ;

  for (i = 0; i < ((sizeof (ASHashKey) - sizeof (char)) << 3); i++)
    {
      c = *(string++) ;
      if (c == '\0')
	break;
      if( isupper(c) ) c = tolower (c);
      hash_key += (((ASHashKey) c) << i);
    }
  return hash_key % hash_size;
}

ASHashKey string_hash_value (ASHashableValue value, ASHashKey hash_size)
{
  ASHashKey hash_key = 0;
  register int i;
  char* string = value.string_val ;
  register char c ;
  
  for (i = 0; i < ((sizeof (ASHashKey) - sizeof (char)) << 3); i++)
    {
      c = *(string++) ;
      if (c == '\0')
	break;
      hash_key += (((ASHashKey) c) << i);
    }
  return hash_key % hash_size;
}

ASHashKey color_hash_value(ASHashableValue value, ASHashKey hash_size)
{
  register CARD32 h;
  int i = 1;
  register unsigned long long_val = value.long_val;

  h = (long_val&0x0ff);
  long_val = long_val>>8 ;

  while (i++<4)
    { /* for the first 4 bytes we can use simplier loop as there can be no overflow */
      h = (h << 4) + (long_val&0x0ff);
      long_val = long_val>>8 ;
    }
  while ( i++ < sizeof (unsigned long))
    { /* this loop will only be used on computers with long > 32bit */
     register CARD32 g ;
     
      h = (h << 4) + (long_val&0x0ff);
      long_val = long_val>>8 ;
      if ((g = h & 0xf0000000))
      {
	h ^= g >> 24;
        h &= 0x0fffffff;
      }
    }
  return (ASHashKey)(h % (CARD32)hash_size);
}

