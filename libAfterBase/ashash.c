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

