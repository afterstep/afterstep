/*
 * Copyright (c) 2001 Sasha Vasko <sasha at aftercode.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <stdio.h>

/*#define LOCAL_DEBUG*/

#include "config.h"
#include "astypes.h"
#include "output.h"
#include "safemalloc.h"
#include "selfdiag.h"
#include "aslist.h"
#include "audit.h"

#define DEALLOC_CACHE_SIZE      1024
static ASBiDirElem* deallocated_mem[DEALLOC_CACHE_SIZE+10] ;
static unsigned int deallocated_used = 0 ;

static void dealloc_bidirelem( ASBiDirElem *e )
{
	if( deallocated_used < DEALLOC_CACHE_SIZE )
	{
		deallocated_mem[deallocated_used++] = e ;
	}else
		free( e );
}

static ASBiDirElem *alloc_bidirelem()
{
	if( deallocated_used > 0  )
	{
		return deallocated_mem[--deallocated_used];
	}else
		return safecalloc( 1, sizeof(ASBiDirElem));
}


ASBiDirList *create_asbidirlist(destroy_list_data_handler destroy_func)
{
    ASBiDirList *l = safecalloc( 1, sizeof(ASBiDirList) ) ;
	l->destroy_func = destroy_func ;

    return l;
}

void
purge_asbidirlist( register ASBiDirList *l )
{
	if( l->destroy_func )
		while( l->head )
		{
			ASBiDirElem *e = l->head ;
			l->head = e->next ;
			if( e->data  )
				l->destroy_func( e->data );
			dealloc_bidirelem( e );
			--(l->count);
		}
	else
		while( l->head )
		{
			ASBiDirElem *e = l->head ;
			l->head = e->next ;
			dealloc_bidirelem( e );
			--(l->count);
		}
}

void destroy_asbidirlist( ASBiDirList **pl )
{
    if( pl )
        if( *pl )
        {
			purge_asbidirlist( *pl );
            free( *pl );
            *pl = NULL ;
        }
}

void *
append_bidirelem( ASBiDirList *l, void *data )
{
	ASBiDirElem *e ;
    if( l == NULL ) return data ;

	e = alloc_bidirelem();
	e->data = data ;
	e->prev = l->tail ;
	if( l->tail )
		l->tail->next = e ;
	else
		l->head = e ;
	l->tail = e ;
	++(l->count);

	return data;
}

void *
prepend_bidirelem( ASBiDirList *l, void *data )
{
	ASBiDirElem *e ;
    if( l == NULL ) return data ;

	e = alloc_bidirelem();
	e->data = data ;
	e->next = l->head ;
	if( l->head )
		l->head->prev = e ;
	else
		l->tail = e ;
	l->head = e ;
	++(l->count);

	return data;
}

void *
insert_bidirelem_after( ASBiDirList *l, void *data, ASBiDirElem *after )
{
	ASBiDirElem *e ;
    if( l == NULL ) return data ;
	/* don't check if after is in the list since it could be time-consuming */
	if( after == NULL )
		return append_bidirelem( l, data );

	e = alloc_bidirelem();
	e->data = data ;
	e->prev = after ;
	e->next = after->next ;
	if( after->next )
		after->next->prev = e ;
	after->next = e ;
	if( after == l->tail )
		l->tail = e ;

	++(l->count);

	return data;
}

void *
insert_bidirelem_before( ASBiDirList *l, void *data, ASBiDirElem *before )
{
	ASBiDirElem *e ;
    if( l == NULL ) return data ;
	/* don't check if after is in the list since it could be time-consuming */
	if( before == NULL )
		return prepend_bidirelem( l, data );

	e = alloc_bidirelem();
	e->data = data ;
	e->next = before ;
	e->prev = before->prev ;
	if( before->prev )
		before->prev->next = e ;
	before->prev = e ;
	if( before == l->head )
		l->head = e ;

	++(l->count);

	return data;
}

void
destroy_bidirelem( ASBiDirList *l, ASBiDirElem *elem )
{
	if( l == NULL || elem == NULL )
		return ;
	if( elem == l->head )
		l->head = elem->next ;
	if( elem == l->tail )
		l->tail = elem->prev ;
	if( elem->next )
		elem->next->prev = elem->prev ;
	if( elem->prev )
		elem->prev->next = elem->next ;
	if( l->destroy_func && elem->data )
		l->destroy_func( elem->data );
	--(l->count);
	dealloc_bidirelem( elem );
}

void
pop_bidirelem( ASBiDirList *l, ASBiDirElem *elem )
{
	if( l == NULL || elem == NULL )
		return ;
	if( elem == l->head )
        return ;
    if( elem == l->tail )
		l->tail = elem->prev ;
	if( elem->next )
		elem->next->prev = elem->prev ;
	if( elem->prev )
		elem->prev->next = elem->next ;
    elem->prev = NULL ;
    elem->next = l->head ;
    if( l->head )
        l->head->prev = elem ;
    l->head = elem ;
}

void
discard_bidirelem( ASBiDirList *l, void *data )
{
	ASBiDirElem *elem ;
	if( l )
	{
		for( elem = l->head ; elem ; elem = elem->next )
			if( elem->data == data )
				break ;
		if( elem )
			destroy_bidirelem( l, elem );
	}
}
