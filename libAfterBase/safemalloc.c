/*
 * Copyright (c) 2001 Sasha Vasko <sashav@sprintmail.com>
 *   and many others, who has not left their copyrights here :)
 * 
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "selfdiag.h"

#ifdef DEBUG_ALLOCS
#include <string.h>
#undef malloc
#undef safemalloc
#undef safecalloc
#endif /* DEBUG_ALLOCS */

/* always undef free, as it will be both redefined with and without
   DEBUG_ALLOC */
#undef free

typedef struct memory_ctrl
{
	size_t used ;
	size_t total ;
	size_t allocations ;
	size_t deallocations ;
}memory_ctrl;

#define MAX_BLOCK	8192
static memory_ctrl memory[MAX_BLOCK] ;
static int longer_then_max_block = 0 ;


void         *
safemalloc (size_t length)
{
	char         *ptr;

	if (length <= 0)
		length = 1;
#ifdef DEBUG_ALLOCS
	else if( length > 1000000 )
	{
		show_error( "attempt to allocate too much memory (%d) at : ", length );
		print_simple_backtrace();
		exit(0);
	}
#endif

	if( length > MAX_BLOCK )
		longer_then_max_block++ ;
	else
		memory[length-1].allocations++ ;

    ptr = malloc (length);

	if (ptr == (char *)0)
	{
		char *suicide = NULL;
		fprintf (stderr, "malloc of %d bytes failed. Exiting\n", length);
		*suicide = 1 ;
		exit (1);
	}
	return ptr;
}

void         *
safecalloc (size_t num, size_t blength)
{
	char         *ptr;

    if (blength <= 0)
        blength = 1;
#ifdef DEBUG_ALLOCS
	else if( blength > 1000000 )
	{
		show_error( "attempt to allocate too much memory (%d) at : ", blength );
		print_simple_backtrace();
		exit(0);
	}
#endif
    if (num <= 0)
        num = 1;

	if( blength > MAX_BLOCK )
		longer_then_max_block += num ;
	else
		memory[blength-1].allocations+=num ;

    ptr = calloc (num, blength);

	if (ptr == (char *)0)
	{
        fprintf (stderr, "calloc of %d blocks of %d bytes each failed. Exiting\n", num, blength);
		exit (1);
	}
	return ptr;
}

void
safefree (void *ptr)
{
    if (ptr)
		free (ptr);
}

void
dump_memory()
{
	size_t i ;
	char filename[512];/* = malloc(strlen(MyName)+1+6+1);*/
	FILE *f ;

	sprintf( filename, "%s.allocs", MyName );

	f = fopen( filename, "w" );
	for( i = 0 ; i < MAX_BLOCK ; i++ )
		fprintf( f, "%u\t\t%u\n", i, memory[i].allocations );
	fprintf( f, "greater then %u\t\t%u\n", i, longer_then_max_block );
	fclose( f );
}

