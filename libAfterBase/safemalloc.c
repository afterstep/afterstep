#include <stdlib.h>
#include <stdio.h>
#include "../configure.h"
#include "../include/aftersteplib.h"

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

	if( length > MAX_BLOCK )
		longer_then_max_block++ ;
	else
		memory[length-1].allocations++ ;
	
    ptr = malloc (length);

	if (ptr == (char *)0)
	{
		fprintf (stderr, "malloc of %d bytes failed. Exiting\n", length);
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


/*
 * Dynamically growing buffer implementation :
 */
#ifdef DEBUG_ALLOCS
#include "../include/audit.h"
#endif /* DEBUG_ALLOCS */

void *alloc_dyn_buffer( dynamic_buffer *buf, size_t new_size )
{
    if( buf == NULL || new_size == 0 ) return NULL ;
    if( buf->allocated < new_size )
    {
        buf->allocated = new_size ;
        if( buf->memory ) free( buf->memory );
        buf->memory = safemalloc( new_size );
    }
    buf->used = 0 ;
    return buf->memory;
}

void *realloc_dyn_buffer( dynamic_buffer *buf, size_t new_size )
{
    if( buf == NULL || new_size == 0  ) return NULL ;
    if( buf->allocated < new_size )
    {
        buf->allocated = new_size ;
        if( buf->memory )
        {
            buf->memory = realloc( buf->memory, new_size );
            if ( buf->memory == NULL )
            {
                buf->allocated = 0 ;
                buf->used = 0 ;
            }
        }else
            buf->memory = safemalloc( new_size );
    }
    return buf->memory;
}

dynamic_buffer *append_dyn_buffer( dynamic_buffer *buf, void * data, size_t size )
{
    if( buf == NULL || size == 0 ) return buf ;

    if( buf->allocated < buf->used+size )
        realloc_dyn_buffer( buf, buf->used+size + ((buf->used+size)>>3) );

    memcpy( buf->memory+buf->used, data, size );
    buf->used += size ;

    return buf;
}

void
flush_dyn_buffer( dynamic_buffer *buf )
{
    if( buf )
        buf->used = 0 ;
}

void
free_dyn_buffer( dynamic_buffer *buf )
{
    if( buf )
    {
        if( buf->memory )
        {
            free( buf->memory );
            buf->memory = NULL ;
        }
        buf->used = buf->allocated = 0 ;
    }
}


