/* This file contains code colormapping of the image                */
/********************************************************************/
/* Copyright (c) 2001 Sasha Vasko <sashav@sprintmail.com>           */
/********************************************************************/
/*
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

#include "config.h"

/*#define LOCAL_DEBUG*/
#define DO_CLOCKING

#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
/* <setjmp.h> is used for the optional error recovery mechanism */

#ifdef HAVE_PNG
/* Include file for users of png library. */
#include <png.h>
#else
#include <setjmp.h>
#endif

#include "afterbase.h"

#include "asimage.h"
#include "import.h"
#include "export.h"
#include "ascmap.h"

/***********************************************************************************/
/* reduced colormap building code :                                                */
/***********************************************************************************/
static inline ASMappedColor *new_mapped_color( CARD8 red, CARD8 green, CARD8 blue, CARD32 indexed )
{
	register ASMappedColor *pnew = malloc( sizeof( ASMappedColor ));
	if( pnew != NULL )
	{
		pnew->red = red ;
		pnew->green = green ;
		pnew->blue = blue ;
		pnew->indexed = indexed ;
		pnew->count = 1 ;
		pnew->cmap_idx = -1 ;
		pnew->next = NULL ;
/*LOCAL_DEBUG_OUT( "indexed color added: 0x%X(%d): #%2.2X%2.2X%2.2X", indexed, indexed, red, green, blue );*/
	}
	return pnew;
}

int
add_index_color( ASSortedColorIndex *index, CARD32 red, CARD32 green, CARD32 blue )
{
	ASSortedColorStack *stack ;
	ASMappedColor **pnext ;
	CARD32 indexed ;

	green = green << 2 ;
	blue = blue <<1 ;
	indexed = MAKE_INDEXED_COLOR(red,green,blue);
	green = green >> 2 ;
	blue = blue >>1 ;
	stack = &(index->stacks[(indexed>>SLOTS_OFFSET)&SLOTS_MASK]);
	pnext = &(stack->head);

	++(stack->count);

	if( stack->tail ) 
	{
		if( indexed == stack->tail->indexed )
			return ++(stack->tail->count);
		else if( indexed > stack->tail->indexed )
			pnext = &(stack->tail);
	}			
	while( *pnext )
	{
		register ASMappedColor *pelem = *pnext ;/* to avoid double redirection */
		if( pelem->indexed == indexed )
			return ++(pelem->count);
		else if( pelem->indexed > indexed )
		{
			register ASMappedColor *pnew = new_mapped_color( red, green, blue, indexed );
			if( pnew )
			{
				++(index->count_unique);
				pnew->next = pelem ;
				*pnext = pnew ;
				return 1;
			}
		}
		pnext = &(pelem->next);
	}
	/* we want to avoid memory overflow : */
	if( (*pnext = new_mapped_color( red, green, blue, indexed )) != NULL )
	{
		stack->tail = (*pnext);
		++(index->count_unique);
		return 1;
	}
	return -1;
}

void destroy_colorindex( ASSortedColorIndex *index, Bool reusable )
{
	if( index )
	{
		int i ;
		for( i = 0 ; i < COLOR_STACK_SLOTS ; i++ )
			while( index->stacks[i].head )
			{
				ASMappedColor *to_free = index->stacks[i].head;
				index->stacks[i].head = to_free->next ;
				free( to_free );
			}
		if( !reusable )
			free( index );
	}
}

static inline void
add_colormap_item( register ASColormapEntry *pentry, ASMappedColor *pelem, int cmap_idx )
{
	pentry->red = pelem->red ;
	pentry->green = pelem->green ;
	pentry->blue = pelem->blue ;
	pelem->cmap_idx = cmap_idx ;
LOCAL_DEBUG_OUT( "colormap entry added: %d: #%2.2X%2.2X%2.2X",cmap_idx, pelem->red, pelem->green, pelem->blue );
}

unsigned int
add_colormap_items( ASSortedColorIndex *index, unsigned int start, unsigned int stop, unsigned int quota, unsigned int base, ASColormapEntry *entries )
{
	int cmap_idx = 0 ;
	int i ;
	if( quota >= index->count_unique )
	{
		for( i = start ; i < stop ; i++ )
		{
			register ASMappedColor *pelem = index->stacks[i].head ;
			while ( pelem != NULL )
			{
				add_colormap_item( &(entries[cmap_idx]), pelem, base++ );
				index->stacks[i].count -= pelem->count ;
				++cmap_idx ;
				pelem = pelem->next ;
			}
		}
	}else
	{
		int total = 0 ;
		int subcount = 0 ;
		ASMappedColor *best = NULL ;
		for( i = start ; i <= stop ; i++ )
			total += index->stacks[i].count ;

		for( i = start ; i <= stop ; i++ )
		{
			register ASMappedColor *pelem = index->stacks[i].head ;
			while ( pelem != NULL /*&& cmap_idx < quota*/ )
			{
				if( pelem->cmap_idx < 0 )
				{
					if( best == NULL )
						best = pelem ;
					else if( best->count < pelem->count )
						best = pelem ;
					else if( best->count == pelem->count &&
						     subcount >= (total>>2) && subcount <= (total>>1)*3 )
						best = pelem ;
					subcount += pelem->count*quota ;
LOCAL_DEBUG_OUT( "count = %d subtotal = %d, quota = %d, idx = %d, i = %d, total = %d", pelem->count, subcount, quota, cmap_idx, i, total );
					if( subcount >= total )
					{
						add_colormap_item( &(entries[cmap_idx]), best, base++ );
						index->stacks[i].count -= best->count ;
						++cmap_idx ;
						subcount -= total ;
						best = NULL ;
					}
				}
				pelem = pelem->next ;
			}
		}
	}
	return cmap_idx ;
}

void
fix_colorindex_shortcuts( ASSortedColorIndex *index )
{
	int i ;
	int last_good = -1, next_good = -1;

	index->last_found = -1 ;

	for( i = 0 ; i < COLOR_STACK_SLOTS ; i++ )
	{
		register ASMappedColor **pelem = &(index->stacks[i].head) ;
		register ASMappedColor **tail = &(index->stacks[i].tail) ;
		while( *pelem != NULL )
		{
			if( (*pelem)->cmap_idx < 0 )
			{
				ASMappedColor *to_free = *pelem ;
				*pelem = (*pelem)->next ;
				free( to_free );
			}else
			{
				*tail = *pelem ;
				pelem = &((*pelem)->next);
			}				
		}
	}
	for( i = 0 ; i < COLOR_STACK_SLOTS ; i++ )
	{
		if( next_good < 0 )
		{
			for( next_good = i ; next_good < COLOR_STACK_SLOTS ; next_good++ )
				if( index->stacks[next_good].head )
					break;
			if( next_good >= COLOR_STACK_SLOTS )
				next_good = last_good ;
		}
		if( index->stacks[i].head )
		{
			last_good = i;
			next_good = -1;
		}else
		{
			if( last_good < 0 || ( i-last_good >= next_good-i && i < next_good ) )
				index->stacks[i].good_offset = next_good-i ;
			else
				index->stacks[i].good_offset = last_good-i ;
		}
	}
}

ASColormap *
color_index2colormap( ASSortedColorIndex *index, unsigned int max_colors, ASColormap *reusable_memory )
{
	ASColormap *cmap = reusable_memory;
	int cmap_idx = 0 ;
	int i ;

	if( cmap == NULL )
		cmap = safecalloc( 1, sizeof(ASColormap));
	cmap->count = MIN(max_colors,index->count_unique);
	cmap->entries = safemalloc( cmap->count*sizeof( ASColormapEntry) );
	/* now lets go ahead and populate colormap : */
	if( index->count_unique <= max_colors )
	{
		add_colormap_items( index, 0, COLOR_STACK_SLOTS, index->count_unique, 0, cmap->entries);
	}else
	while( cmap_idx < max_colors )
	{
		long total = 0 ;
		long subcount = 0 ;
		int start_slot = 0 ;
		int quota = max_colors-cmap_idx ;
		
		for( i = 0 ; i <= COLOR_STACK_SLOTS ; i++ )
			total += index->stacks[i].count ;

		for( i = 0 ; i < COLOR_STACK_SLOTS ; i++ )
		{
			subcount += index->stacks[i].count*quota ;
/*LOCAL_DEBUG_OUT( "count = %d, subtotal = %d, to_add = %d, idx = %d, i = %d, total = %d", index->stacks[i].count, subcount, subcount/total, cmap_idx, i, total );*/
			if( subcount >= total )
			{	/* we need to add subcount/index->count items */
				int to_add = subcount/total ;
				if( i == COLOR_STACK_SLOTS-1 && to_add < max_colors-cmap_idx )
					to_add = max_colors-cmap_idx;
				cmap_idx += add_colormap_items( index, start_slot, i, to_add, cmap_idx, &(cmap->entries[cmap_idx]));
				subcount %= total;
				start_slot = i+1;
			}
		}
	}
	fix_colorindex_shortcuts( index );
	return cmap;
}

void destroy_colormap( ASColormap *cmap, Bool reusable )
{
	if( cmap )
	{
		if( cmap->entries )
			free( cmap->entries );
		if( !reusable )
			free( cmap );
	}
}

int
get_color_index( ASSortedColorIndex *index, CARD32 red, CARD32 green, CARD32 blue )
{
	ASSortedColorStack *stack ;
	ASMappedColor *pnext, *lesser ;
	CARD32 indexed ;
	int slot, offset ;

	green = green << 2 ;
	blue = blue <<1 ;
	indexed = MAKE_INDEXED_COLOR(red,green,blue);
	if( index->last_found == indexed )
		return index->last_idx;
	index->last_found = indexed ;

	slot = (indexed>>SLOTS_OFFSET)&SLOTS_MASK ;
/*LOCAL_DEBUG_OUT( "index = %X(%d), slot = %d, offset = %d", indexed, indexed, slot, index->stacks[slot].good_offset );*/
	if( (offset = index->stacks[slot].good_offset) != 0 )
		slot += offset ;
	stack = &(index->stacks[slot]);
/*LOCAL_DEBUG_OUT( "first_good = %X(%d), last_good = %X(%d)", stack->first_good->indexed, stack->first_good->indexed, stack->last_good->indexed, stack->last_good->indexed );*/
	if( offset < 0 || stack->tail->indexed <= indexed )
		return (index->last_idx=stack->tail->cmap_idx);
	if( offset > 0 || stack->head->indexed >= indexed )
		return (index->last_idx=stack->head->cmap_idx);

	lesser = stack->head ;
	for( pnext = lesser; pnext != NULL ; pnext = pnext->next )
	{
/*LOCAL_DEBUG_OUT( "lesser = %X(%d), pnext = %X(%d)", lesser->indexed, lesser->indexed, pnext->indexed, pnext->indexed );*/
			if( pnext->indexed >= indexed )
			{
				index->last_idx = ( pnext->indexed-indexed > indexed-lesser->indexed )?
						lesser->cmap_idx : pnext->cmap_idx ;
				return index->last_idx;
			}
			lesser = pnext ;
	}
	return stack->tail->cmap_idx;
}

void
quantize_scanline( ASScanline *scl, ASSortedColorIndex *index, ASColormap *cmap, int *dst, int transp_threshold, int transp_idx )
{
	int i ;
	register CARD32 *a = scl->alpha ;
#if 0
	register int red = 0, green = 0, blue = 0;
	for( i = 0 ; i < scl->width ; i++ )
		if( a[i] >= transp_threshold )
		{
			ASColormapEntry mc ;
			red += scl->red[i] ;
			if( red < 0 ) red = 0 ;
			else if( red > 255 ) red = 255 ;

			green += scl->green[i] ;
			if( green < 0 ) green = 0 ;
			else if( green > 255 ) green = 255 ;

			blue += scl->blue[i] ;
			if( blue < 0 ) blue = 0 ;
			else if( blue > 255 ) blue = 255 ;

			dst[i] = get_color_index( index, red, green, blue );
			mc = cmap->entries[dst[i]] ;
			if( red != mc.red || green != mc.green || blue != mc.blue )

			red   = (mc.red  -red  )>>1 ;
			green = (mc.green-green)>>1 ;
			blue  = (mc.blue -blue )>>1 ;
#endif
	register CARD32 *red = scl->red, *green = scl->green, *blue = scl->blue;

	if( transp_threshold > 0 )
	{
		for( i = 0 ; i < scl->width ; i++ )
			if( a[i] >= transp_threshold )
				dst[i] = get_color_index( index, red[i], green[i], blue[i] );
			else
				dst[i] = transp_idx ;
	}else
		for( i = 0 ; i < scl->width ; i++ )
			dst[i] = get_color_index( index, red[i], green[i], blue[i] );

}

