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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "../configure.h"

#include <stdlib.h>
#include <stdio.h>

#include <X11/Xmd.h>

#include "../configure.h"
#include "../include/aftersteplib.h"
#include "../include/asvector.h"

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
            v->memory = realloc( v->memory, v->allocated * v->unit );
            if ( v->memory == NULL )
            {
                v->allocated = 0 ;
                v->used = 0 ;
            }
        }else
            v->memory = safemalloc( v->allocated * v->unit );
    }
    return v->memory;
}

ASVector *
append_vector( ASVector *v, void * data, size_t size )
{
    if( v == NULL || size == 0 ) return v ;

    if( v->allocated < v->used+size )
        realloc_vector( v, v->used+size + ((v->used+size)>>3) );

    memcpy( v->memory+v->used, data, size*v->unit );
    v->used += size ;

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

inline void vector_move_data_up( ASVector *v, int offset, int length )
{
    register int i ;
    /* word copying is usually faster then raw memory copying */
    if( v->unit == sizeof(long*) )
    {   /* 4 or 8 byte pointer copying  */
        register long** src = (long**)(v->memory);
        register long** trg = src+length;
        for( i = v->used-1 ; i >= offset ; i-- )
            trg[i] = src[i] ;
    }else if( v->unit == 2 )
    {   /* 2 byte copying  */
        register CARD16 *src = (CARD16*)(v->memory);
        register CARD16 *trg = src+length;
        for( i = v->used-1 ; i >= offset ; i-- )
            trg[i] = src[i] ;
    }else
    {   /* raw copy */
        register CARD8 *src = v->memory ;
        register CARD8 *trg = src+(length*v->unit) ;
        int start = offset*v->unit ;
        for( i = (v->used-1)*v->unit ; i >= start ; i-- )
            trg[i] = src[i] ;
    }
    v->used+=length ;
}

inline void vector_move_data_down( ASVector *v, int offset, int length )
{
    register int i ;
    /* word copying is usually faster then raw memory copying */
    if( v->unit == sizeof(long*) )
    {   /* 4 or 8 byte pointer copying  */
        register long** trg = (long**)(v->memory);
        register long** src = trg+length;
        for( i = offset ; i < v->used ; i++ )
            trg[i] = src[i] ;
    }else if( v->unit == 2 )
    {   /* 2 byte copying  */
        register CARD16* trg = (CARD16*)(v->memory);
        register CARD16* src = trg+length;
        for( i = offset ; i < v->used ; i++ )
            trg[i] = src[i] ;
    }else
    {   /* raw copy */
        register CARD8 *trg = v->memory ;
        register CARD8 *src = trg+(length*v->unit) ;
        int end = (v->used)*(v->unit);
        for( i = offset*v->unit ; i < end ; i++ )
            trg[i] = src[i] ;
    }
    v->used-=length ;
}

inline void vector_set_data( ASVector *v, void *data, int offset, int length)
{
    register int i ;
    /* word copying is usually faster then raw memory copying */
    if( v->unit == sizeof(long*) )
    {
        register long** trg = ((long**)(v->memory))+offset;
        register long** src = (long**)data ;
fprintf( stderr, "inserting %d long* at %d\n", length, offset );
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
        register CARD8 *trg = ((CARD8*)(v->memory))+offset;
        register CARD8 *src = (CARD8*)data ;
        for( i = 0 ; i < length ; i++ )
            trg[i] = src[i] ;
    }
}

int
vector_insert_elem( ASVector *v, void *data, size_t size, void *sibling, int before )
{
    size_t index = 0;

    if( v == NULL || data == NULL || size == 0 )
        return -1;

    if( v->allocated < v->used+size )
        realloc_vector( v, v->used+size + ((v->used+size)>>3) );

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
fprintf( stderr, "inserting into vector at %d\n", index );
    if( index < v->used )
        vector_move_data_up(v,index,size);
    else
        v->used+=size ;

    vector_set_data(v,data,index,size);
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
    vector_move_data_down( v, index, 1 );
    return 1;
}

int
vector_remove_index( ASVector *v, size_t index )
{
    if( v == NULL )
        return 0;
    if( index >= v->used )
        return 0;

    vector_move_data_down( v, index, 1 );
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


