/*
 * Copyright (c) 2000 Sasha Vasko <sashav@sprintmail.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <stdlib.h>
#include <stdio.h>

/*#define LOCAL_DEBUG*/

#include "config.h"
#include "astypes.h"
#include "output.h"
#include "safemalloc.h"
#include "asvector.h"

ASVector *create_asvector( size_t unit )
{
    ASVector *v = NULL ;
    if( unit > 0 )
    {
        v = safecalloc( 1, sizeof(ASVector) );
        v->unit = unit ;
LOCAL_DEBUG_CALLER_OUT("%p, %d", v, unit );
    }
    return v;
}

void destroy_asvector( ASVector **v )
{
    if( v )
        if( *v )
        {
            free_vector( *v );
            free( *v );
            *v = NULL ;
        }
}

void *
alloc_vector( ASVector *v, size_t new_size )
{
    if( v == NULL || new_size == 0 ) return NULL ;
    if( v->allocated < new_size )
    {
        if( new_size*v->unit < 32 )
            v->allocated = (32/v->unit)+1 ;
        else
            v->allocated = new_size ;

        if( v->memory ) free( v->memory );
        v->memory = safemalloc( v->allocated * v->unit );
    }
    v->used = 0 ;
    return v->memory;
}

void *
realloc_vector( ASVector *v, size_t new_size )
{
    if( v == NULL || new_size == 0  ) return NULL ;
    if( v->allocated < new_size )
    {
        if( new_size*v->unit < 32 )
            v->allocated = (32/v->unit)+1 ;
        else
            v->allocated = new_size ;

        if( v->memory )
        {
#ifdef DEBUG_ALLOCS
			if( v->allocated > 10000 )
			{
				show_error( "attempt to reallocate too much memory (%d) at : ", v->allocated );
				print_simple_backtrace();
				exit(0);
			}
#endif
            v->memory = realloc( v->memory, v->allocated * v->unit );
            if ( v->memory == NULL )
            {
                v->allocated = 0 ;
                v->used = 0 ;
            }
        }else
            v->memory = safemalloc( v->allocated * v->unit );
    }
LOCAL_DEBUG_CALLER_OUT("%p, %lu(*%d)| new memory = %p", v, (unsigned long)new_size, v?v->unit:0, v->memory );
    return v->memory;
}

ASVector *
append_vector( ASVector *v, void * data, size_t size )
{
LOCAL_DEBUG_CALLER_OUT("0x%lX, 0x%lX, %lu", (unsigned long)v, (unsigned long)data, (unsigned long)size );
    if( v == NULL || size == 0 ) return v ;

    if( v->allocated < v->used+size )
        realloc_vector( v, v->used+size + (v->used+size)/8 );

	if( data )
	{
        memcpy( v->memory+(v->used*v->unit), data, size*v->unit );
	    v->used += size ;
	}

    return v;
}

/* finds index of the first element in the vector that is exactly matching specifyed
 * data */
inline size_t vector_find_data( ASVector *v, void *data )
{
    register int i ;
    /* word copying is usually faster then raw memory copying */
    if( v->unit == sizeof(long*) )
    {   /* 4 or 8 byte pointer copying  */
        register long** src = (long**)(v->memory);
        register long* trg = *((long**)data);
        for( i = 0 ; i < (int)(v->used) ; i++ )
            if( trg == src[i] )
                break;
    }else if( v->unit == 2 )
    {   /* 2 byte copying  */
        register CARD16 *src = (CARD16*)(v->memory);
        register CARD16 trg = *((CARD16*)data);
        for( i = 0 ; i < (int)(v->used) ; i++ )
            if( trg == src[i] )
                break;
    }else if( v->unit == 1 )
    {
        register CARD8 *src = (CARD8*)(v->memory);
        register CARD8 trg = *((CARD8*)data);
        for( i = 0 ; i < (int)(v->used) ; i++ )
            if( trg == src[i] )
                break;
    }else
    {   /* raw copy */
        register CARD8 *src = v->memory ;
        register CARD8 *trg = (CARD8*)data ;
        register int k ;
        for( i = 0 ; i < (int)(v->used) ; i++ )
        {
            for( k = 0 ; k < (int)(v->unit) ; k++ )
                if( src[k] != trg[k] )
                    break;
            if( k >= (int)(v->unit) )
                break;

            src += v->unit ;
        }
    }
    return i;
}

inline void vector_move_data_up( ASVector *v, int index, int offset, int length )
{
    register int i ;
    /* word copying is usually faster then raw memory copying */
    if( length == -1 )
        length = v->used ;
    if( v->unit == sizeof(long*) )
    {   /* 4 or 8 byte pointer copying  */
        register long** src = (long**)(v->memory);
        register long** trg = src+offset;
        for( i = length-1 ; i >= index ; i-- )
            trg[i] = src[i] ;
    }else if( v->unit == 2 )
    {   /* 2 byte copying  */
        register CARD16 *src = (CARD16*)(v->memory);
        register CARD16 *trg = src+offset;
        for( i = length-1 ; i >= index ; i-- )
            trg[i] = src[i] ;
    }else
    {   /* raw copy */
        register CARD8 *src = v->memory ;
        register CARD8 *trg = src+(offset*v->unit) ;
        int start = index*v->unit ;
        for( i = (length-1)*v->unit ; i >= start ; i-- )
            trg[i] = src[i] ;
    }
    v->used+=offset ;
}

inline void vector_move_data_down( ASVector *v, int index, int offset, int length )
{
    register int i ;
    /* word copying is usually faster then raw memory copying */
    if( length == -1 )
        length = v->used ;
    if( v->unit == sizeof(long*) )
    {   /* 4 or 8 byte pointer copying  */
        register long** trg = (long**)(v->memory);
        register long** src = trg+offset;
        for( i = index ; i < length ; i++ )
            trg[i] = src[i] ;
    }else if( v->unit == 2 )
    {   /* 2 byte copying  */
        register CARD16* trg = (CARD16*)(v->memory);
        register CARD16* src = trg+offset;
        for( i = index ; i < length ; i++ )
            trg[i] = src[i] ;
    }else
    {   /* raw copy */
        register CARD8 *trg = v->memory ;
        register CARD8 *src = trg+(offset*v->unit) ;
        int end = (length)*(v->unit);
        for( i = index*v->unit ; i < end ; i++ )
            trg[i] = src[i] ;
    }
    v->used-=offset ;
}

inline void vector_set_data( ASVector *v, void *data, int offset, int length)
{
    register int i ;
    /* word copying is usually faster then raw memory copying */
    if( v->unit == sizeof(long*) )
    {
        register long** trg = ((long**)(v->memory))+offset;
        register long** src = (long**)data ;
        for( i = 0 ; i < length ; i++ )
            trg[i] = src[i] ;
    }else if( v->unit == 2 )
    {
        register CARD16 *trg = ((CARD16*)(v->memory))+offset;
        register CARD16 *src = (CARD16*)data ;
        for( i = 0 ; i < length ; i++ )
            trg[i] = src[i] ;
    }else
    {
        register CARD8 *trg = ((CARD8*)(v->memory))+(offset*v->unit);
        register CARD8 *src = (CARD8*)data ;
        for( i = length*v->unit - 1 ; i >=0 ; i-- )
            trg[i] = src[i] ;
    }
}

int
vector_insert_elem( ASVector *v, void *data, size_t size, void *sibling, int before )
{
    size_t index = 0;
LOCAL_DEBUG_CALLER_OUT("0x%lX, 0x%lX, %lu, 0x%lX, %d", (unsigned long)v, (unsigned long)data, (unsigned long)size, (unsigned long)sibling, before );

    if( v == NULL || data == NULL || size == 0 )
        return -1;

    if( v->allocated < v->used+size )
        realloc_vector( v, v->used+size + (v->used+size)/8 );

    if( sibling == NULL )
    {
        if( !before )
            index = v->used ;
    }else
    {
        index = vector_find_data(v,sibling);
        if( index == v->used )
        {
            if( before )
                index = 0;
        }else if( !before )
            index ++ ;
    }
    if( index < v->used )
        vector_move_data_up(v,index,size,-1);
    else
        v->used+=size ;

    vector_set_data(v,data,index,size);
    return index;
}

int
vector_relocate_elem( ASVector *v, void *data, unsigned int new_index )
{
    size_t index = 0;

    if( v == NULL || data == NULL )
        return -1;

    index = vector_find_data(v,data);
    if( index >= v->used )
        return -1;
    if( index > new_index )
        vector_move_data_up(v,new_index,1,index-new_index);
    else if( index < new_index )
        vector_move_data_down(v,new_index,1,new_index-index);
    else
        return index;

    vector_set_data(v,data,new_index,1);
    return index;
}

int
vector_find_elem( ASVector *v, void *data )
{
    int index = -1 ;
    if( v && data )
        if( (index = vector_find_data(v,data)) >= (int)(v->used) )
            index = -1 ;
    return index;
}

int
vector_remove_elem( ASVector *v, void *data )
{
    int index ;
    if( v == NULL || data == NULL )
        return 0;

    if( (index = vector_find_data(v,data)) >= (int)(v->used) )
        return 0;
    vector_move_data_down( v, index, 1, -1 );
    return 1;
}

int
vector_remove_index( ASVector *v, size_t index )
{
    if( v == NULL )
        return 0;
    if( index >= v->used )
        return 0;

    vector_move_data_down( v, index, 1, -1 );
    return 1;
}

void
print_vector( stream_func func, void* stream, ASVector *v, char *name, void (*print_func)( stream_func func, void* stream, void *data ) )
{
    register int i ;

    if( !pre_print_check(&func, &stream, v, "empty vector."))
        return ;
    func( stream, "%s.memory = 0x%8.8X\n%s.unit = %d\n%s.used = %lu\n%s.allocated = %lu\n", name, (long)(v->memory), name, v->unit, name, v->used, name, v->allocated );
    if( print_func )
    {
        register CARD8 *src = v->memory ;
        for( i = 0 ; i < v->used ; i++ )
        {
            func( stream, "%s[%d] = \n", name, i );
            print_func( func, stream, src );
            src += v->unit ;
        }
    }else if( v->unit == sizeof(long*) )
    {   /* 4 or 8 byte pointer copying  */
        register long** src = (long**)(v->memory);
        for( i = 0 ; i < v->used ; i++ )
            func( stream, "%s[%d] = 0x%8.8X(%ld)\n", name, i, (long)src[i], (long)src[i] );
    }else if( v->unit == 2 )
    {   /* 2 byte copying  */
        register CARD16* src = (CARD16*)(v->memory);
        for( i = 0 ; i < v->used ; i++ )
            func( stream, "%s[%d] = 0x%4.4X(%d)\n", name, i, src[i], src[i] );
    }else if( v->unit == 1 )
    {
        register CARD8 *src = (CARD8*)(v->memory);
        for( i = 0 ; i < v->used ; i++ )
            func( stream, "%s[%d] = 0x%2.2X(%d)\n", name, i, (int)src[i], (int)src[i] );
    }else
    {   /* raw copy */
        register CARD8 *src = v->memory ;
        register int k ;
        for( i = 0 ; i < v->used ; i++ )
        {
            func( stream, "%s[%d] =\n", name, i );
            for( k = 0 ; k < v->unit ; k++ )
                func( stream, " 0x%2.2X\n", (int)src[k] );

            src += v->unit ;
        }
    }
}

void
flush_vector( ASVector *v )
{
    if( v )
        v->used = 0 ;
}

void
free_vector( ASVector *v )
{
    if( v )
    {
        if( v->memory )
        {
            free( v->memory );
            v->memory = NULL ;
        }
        v->used = v->allocated = 0 ;
    }
}


