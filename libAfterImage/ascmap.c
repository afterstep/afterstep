/* This file contains code colormapping of the image                */
/********************************************************************/
/* Copyright (c) 2001 Sasha Vasko <sasha at aftercode.net>           */
/********************************************************************/
/*
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
 *
 */

#include "config.h"

/*#define LOCAL_DEBUG*/
/*#define DO_CLOCKING*/

#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>

#include "afterbase.h"

#include "asimage.h"
#include "import.h"
#include "export.h"
#include "ascmap.h"

/***********************************************************************************/
/* reduced colormap building code :                                                */
/***********************************************************************************/
static inline ASMappedColor *new_mapped_color( CARD32 red, CARD32 green, CARD32 blue, CARD32 indexed )
{
	register ASMappedColor *pnew = malloc( sizeof( ASMappedColor ));
	if( pnew != NULL )
	{
		pnew->red   = INDEX_UNSHIFT_RED  (red) ;
		pnew->green = INDEX_UNSHIFT_GREEN(green) ;
		pnew->blue  = INDEX_UNSHIFT_BLUE (blue) ;
		pnew->indexed = indexed ;
		pnew->count = 1 ;
		pnew->cmap_idx = -1 ;
		pnew->next = NULL ;
/*LOCAL_DEBUG_OUT( "indexed color added: 0x%X(%d): #%2.2X%2.2X%2.2X", indexed, indexed, red, green, blue );*/
	}
	return pnew;
}

void
add_index_color( ASSortedColorHash *index, CARD32 indexed, unsigned int slot, CARD32 red, CARD32 green, CARD32 blue )
{
	ASSortedColorBucket *stack ;
	ASMappedColor **pnext ;

	stack = &(index->buckets[slot]);
	pnext = &(stack->head);

	++(stack->count);

	if( stack->tail )
	{
		if( indexed == stack->tail->indexed )
		{
			++(stack->tail->count);
			return;
		}else if( indexed > stack->tail->indexed )
			pnext = &(stack->tail);
	}
	while( *pnext )
	{
		register ASMappedColor *pelem = *pnext ;/* to avoid double redirection */
		if( pelem->indexed == indexed )
		{
			++(pelem->count);
			return ;
		}else if( pelem->indexed > indexed )
		{
			register ASMappedColor *pnew = new_mapped_color( red, green, blue, indexed );
			if( pnew )
			{
				++(index->count_unique);
				pnew->next = pelem ;
				*pnext = pnew ;
				return;
			}
		}
		pnext = &(pelem->next);
	}
	/* we want to avoid memory overflow : */
	if( (*pnext = new_mapped_color( red, green, blue, indexed )) != NULL )
	{
		stack->tail = (*pnext);
		++(index->count_unique);
	}
}

void destroy_colorhash( ASSortedColorHash *index, Bool reusable )
{
	if( index )
	{
		int i ;
		for( i = 0 ; i < index->buckets_num ; i++ )
			while( index->buckets[i].head )
			{
				ASMappedColor *to_free = index->buckets[i].head;
				index->buckets[i].head = to_free->next ;
				free( to_free );
			}
		if( !reusable )
			free( index );
	}
}

#ifdef LOCAL_DEBUG
void
check_colorindex_counts( ASSortedColorHash *index )
{
	int i ;
	int count_unique = 0;

	for( i = 0 ; i < index->buckets_num ; i++ )
	{
		register ASMappedColor *pelem = index->buckets[i].head ;
		int row_count = 0 ;
		while( pelem != NULL )
		{
			count_unique++ ;
			if( pelem->cmap_idx < 0 )
				row_count += pelem->count ;
			pelem = pelem->next ;
		}
		if( row_count != index->buckets[i].count )
			fprintf( stderr, "bucket %d counts-> %d : %d\n", i, row_count, index->buckets[i].count );
	}
	fprintf( stderr, "total unique-> %d : %d\n", count_unique, index->count_unique );

}
#endif

void
fix_colorindex_shortcuts( ASSortedColorHash *index )
{
	int i ;
	int last_good = -1, next_good = -1;

	index->last_found = -1 ;

	for( i = 0 ; i < index->buckets_num ; i++ )
	{
		register ASMappedColor **pelem = &(index->buckets[i].head) ;
		register ASMappedColor **tail = &(index->buckets[i].tail) ;
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
	for( i = 0 ; i < index->buckets_num ; i++ )
	{
		if( next_good < 0 )
		{
			for( next_good = i ; next_good < index->buckets_num ; next_good++ )
				if( index->buckets[next_good].head )
					break;
			if( next_good >= index->buckets_num )
				next_good = last_good ;
		}
		if( index->buckets[i].head )
		{
			last_good = i;
			next_good = -1;
		}else
		{
			if( last_good < 0 || ( i-last_good >= next_good-i && i < next_good ) )
				index->buckets[i].good_offset = next_good-i ;
			else
				index->buckets[i].good_offset = last_good-i ;
		}
	}
}



static inline void
add_colormap_item( register ASColormapEntry *pentry, ASMappedColor *pelem, int cmap_idx )
{
	pentry->red   = pelem->red ;
	pentry->green = pelem->green ;
	pentry->blue  = pelem->blue ;
	pelem->cmap_idx = cmap_idx ;
LOCAL_DEBUG_OUT( "colormap entry added: %d: #%2.2X%2.2X%2.2X",cmap_idx, pelem->red, pelem->green, pelem->blue );
}

unsigned int
add_colormap_items( ASSortedColorHash *index, unsigned int start, unsigned int stop, unsigned int quota, unsigned int base, ASColormapEntry *entries )
{
	int cmap_idx = 0 ;
	int i ;
	if( quota >= index->count_unique )
	{
		for( i = start ; i < stop ; i++ )
		{
			register ASMappedColor *pelem = index->buckets[i].head ;
			while ( pelem != NULL )
			{
				add_colormap_item( &(entries[cmap_idx]), pelem, base++ );
				index->buckets[i].count -= pelem->count ;
				++cmap_idx ;
				pelem = pelem->next ;
			}
		}
	}else
	{
		int total = 0 ;
		int subcount = 0 ;
		ASMappedColor *best = NULL ;
		int best_slot = start;
		for( i = start ; i <= stop ; i++ )
			total += index->buckets[i].count ;

		for( i = start ; i <= stop ; i++ )
		{
			register ASMappedColor *pelem = index->buckets[i].head ;
			while ( pelem != NULL /*&& cmap_idx < quota*/ )
			{
				if( pelem->cmap_idx < 0 )
				{
					if( best == NULL )
					{
						best = pelem ;
						best_slot = i ;
					}else if( best->count < pelem->count )
					{
						best = pelem ;
						best_slot = i ;
					}
					else if( best->count == pelem->count &&
						     subcount >= (total>>2) && subcount <= (total>>1)*3 )
					{
						best = pelem ;
						best_slot = i ;
					}
					subcount += pelem->count*quota ;
LOCAL_DEBUG_OUT( "count = %d subtotal = %d, quota = %d, idx = %d, i = %d, total = %d", pelem->count, subcount, quota, cmap_idx, i, total );
					if( subcount >= total )
					{
						add_colormap_item( &(entries[cmap_idx]), best, base++ );
						index->buckets[best_slot].count -= best->count ;
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

ASColormap *
color_hash2colormap( ASColormap *cmap, unsigned int max_colors )
{
	int cmap_idx = 0 ;
	int i ;
	ASSortedColorHash *index = NULL ;

	if( cmap == NULL || cmap->hash == NULL )
		return NULL;

	index = cmap->hash ;
	cmap->count = MIN(max_colors,index->count_unique);
	cmap->entries = safemalloc( cmap->count*sizeof( ASColormapEntry) );
	/* now lets go ahead and populate colormap : */
	if( index->count_unique <= max_colors )
	{
		add_colormap_items( index, 0, index->buckets_num, index->count_unique, 0, cmap->entries);
	}else
	while( cmap_idx < max_colors )
	{
		long total = 0 ;
		long subcount = 0 ;
		int start_slot = 0 ;
		int quota = max_colors-cmap_idx ;

		for( i = 0 ; i <= index->buckets_num ; i++ )
			total += index->buckets[i].count ;

		for( i = 0 ; i < index->buckets_num ; i++ )
		{
			subcount += index->buckets[i].count*quota ;
LOCAL_DEBUG_OUT( "count = %d, subtotal = %d, to_add = %d, idx = %d, i = %d, total = %d", index->buckets[i].count, subcount, subcount/total, cmap_idx, i, total );
			if( subcount >= total )
			{	/* we need to add subcount/index->count items */
				int to_add = subcount/total ;
				if( i == index->buckets_num-1 && to_add < max_colors-cmap_idx )
					to_add = max_colors-cmap_idx;
				cmap_idx += add_colormap_items( index, start_slot, i, to_add, cmap_idx, &(cmap->entries[cmap_idx]));
				subcount %= total;
				start_slot = i+1;
			}
		}
		if( quota == max_colors-cmap_idx )
			break;
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
		if( cmap->hash )
			destroy_colorhash( cmap->hash, False );
		if( !reusable )
			free( cmap );
	}
}

int
get_color_index( ASSortedColorHash *index, CARD32 indexed, unsigned int slot )
{
	ASSortedColorBucket *stack ;
	ASMappedColor *pnext, *lesser ;
	int offset ;

	if( index->last_found == indexed )
		return index->last_idx;
	index->last_found = indexed ;

LOCAL_DEBUG_OUT( "index = %X(%d), slot = %d, offset = %d", indexed, indexed, slot, index->buckets[slot].good_offset );
	if( (offset = index->buckets[slot].good_offset) != 0 )
		slot += offset ;
	stack = &(index->buckets[slot]);
LOCAL_DEBUG_OUT( "first_good = %X(%d), last_good = %X(%d)", stack->head->indexed, stack->head->indexed, stack->tail->indexed, stack->tail->indexed );
	if( offset < 0 || stack->tail->indexed <= indexed )
		return (index->last_idx=stack->tail->cmap_idx);
	if( offset > 0 || stack->head->indexed >= indexed )
		return (index->last_idx=stack->head->cmap_idx);

	lesser = stack->head ;
	for( pnext = lesser; pnext != NULL ; pnext = pnext->next )
	{
LOCAL_DEBUG_OUT( "lesser = %X(%d), pnext = %X(%d)", lesser->indexed, lesser->indexed, pnext->indexed, pnext->indexed );
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

inline int asimage_decode_line (ASImage * im, ColorPart color, CARD32 * to_buf, unsigned int y, unsigned int skip, unsigned int out_width);

int *
colormap_asimage( ASImage *im, ASColormap *cmap, unsigned int max_colors, unsigned int dither, int opaque_threshold )
{
	int *mapped_im = NULL;
	int buckets_num  = MAX_COLOR_BUCKETS;
	ASScanline scl ;
	CARD32 *a, *r, *g, *b ;
	START_TIME(started);

	int *dst ;
	int y ;
	register int x ;


	if( im == NULL || cmap == NULL || im->width == 0 )
		return NULL;
	if( max_colors == 0 )
		max_colors = 256 ;
	if( dither == -1 )
		dither = 4 ;
	else if( dither >= 8 )
		dither = 7 ;
	switch( dither )
	{
		case 0 :
		case 1 :
		case 2 : buckets_num = 4096 ;
		    break ;
		case 3 :
		case 4 : buckets_num = 1024 ;
		    break ;
		case 5 :
		case 6 : buckets_num = 64 ;
		    break ;
		case 7 : buckets_num = 8 ;
		    break ;
	}

	dst = mapped_im = safemalloc( im->width*im->height*sizeof(int));
	cmap->hash = safecalloc( 1, sizeof(ASSortedColorHash) );
	cmap->hash->buckets = safecalloc( buckets_num, sizeof( ASSortedColorBucket ) );
	cmap->hash->buckets_num = buckets_num ;
	prepare_scanline( im->width, 0, &scl, False );

	a = scl.alpha ;
	r = scl.red ;
	g = scl.green ;
	b = scl.blue ;

	for( x = 0; x < scl.width ; x++ )
		a[x] = 0x00FF ;

	for( y = 0 ; y < im->height ; y++ )
	{
		int red = 0, green = 0, blue = 0;
		asimage_decode_line (im, IC_RED,   r,   y, 0, scl.width);
		asimage_decode_line (im, IC_GREEN, g,   y, 0, scl.width);
		asimage_decode_line (im, IC_BLUE,  b,   y, 0, scl.width);
		if( opaque_threshold > 0 )
		{
			if( (x = asimage_decode_line (im, IC_ALPHA, a, y, 0, scl.width)) <= scl.width )
				while( x < scl.width )
					a[x++] = 0x00FF ;
			else
				cmap->has_opaque = True;
		}
		switch( dither )
		{
			case 0 :
				for( x = 0; x < scl.width ; x++ )
				{
					if( a[x] < opaque_threshold )	dst[x] = -1 ;
					else
					{
						red   = INDEX_SHIFT_RED(r[x]);
						green = INDEX_SHIFT_GREEN(g[x]);
						blue  = INDEX_SHIFT_BLUE(b[x]);
						dst[x] = MAKE_INDEXED_COLOR24(red,green,blue);
						add_index_color( cmap->hash, dst[x], ((dst[x]>>12)&0x0FFF), red, green, blue);
					}
				}
			    break ;
			case 1 :
				for( x = 0; x < scl.width ; x++ )
				{
					if( a[x] < opaque_threshold )	dst[x] = -1 ;
					else
					{
						red   = INDEX_SHIFT_RED(r[x]);
						green = INDEX_SHIFT_GREEN(g[x]);
						blue  = INDEX_SHIFT_BLUE(b[x]);
						dst[x] = MAKE_INDEXED_COLOR21(red,green,blue);
						add_index_color( cmap->hash, dst[x], ((dst[x]>>12)&0x0FFF), red, green, blue);
					}
				}
			    break ;
			case 2 :                           /* 666 */
				{
					for( x = 0 ; x < scl.width ; ++x )
					{
						red   = INDEX_SHIFT_RED  ((red  +r[x]>255)?255:red+r[x]);
						green = INDEX_SHIFT_GREEN((green+g[x]>255)?255:green+g[x]);
						blue  = INDEX_SHIFT_BLUE ((blue +b[x]>255)?255:blue+b[x]);
						if( a[x] < opaque_threshold )
							dst[x] = -1 ;
						else
						{
							dst[x] = MAKE_INDEXED_COLOR18(red,green,blue);
							add_index_color( cmap->hash, dst[x], ((dst[x]>>12)&0x0FFF), red, green, blue);
						}
						red   = INDEX_UNESHIFT_RED(red,1)&0x01 ;
						green = INDEX_UNESHIFT_GREEN(green,1)&0x01 ;
						blue  = INDEX_UNESHIFT_BLUE(blue,1)  &0x01 ;
					}
				}
			    break ;
			case 3 :                           /* 555 */
				{
					for( x = 0 ; x < scl.width ; ++x )
					{
						red   = INDEX_SHIFT_RED  ((red  +r[x]>255)?255:red+r[x]);
						green = INDEX_SHIFT_GREEN((green+g[x]>255)?255:green+g[x]);
						blue  = INDEX_SHIFT_BLUE ((blue +b[x]>255)?255:blue+b[x]);
						if( a[x] < opaque_threshold )
							dst[x] = -1 ;
						else
						{
							dst[x] = MAKE_INDEXED_COLOR15(red,green,blue);
							add_index_color( cmap->hash, dst[x], ((dst[x]>>14)&0x03FF), red, green, blue);
						}
						red   = INDEX_UNESHIFT_RED(red,1)    &0x03 ;
						green = INDEX_UNESHIFT_GREEN(green,1)&0x03 ;
						blue  = INDEX_UNESHIFT_BLUE(blue,1)  &0x03 ;
					}
				}
			    break ;
			case 4 :                           /* 444 */
				{
					for( x = 0 ; x < scl.width ; ++x )
					{
						red   = INDEX_SHIFT_RED  ((red  +r[x]>255)?255:red+r[x]);
						green = INDEX_SHIFT_GREEN((green+g[x]>255)?255:green+g[x]);
						blue  = INDEX_SHIFT_BLUE ((blue +b[x]>255)?255:blue+b[x]);
						if( a[x] < opaque_threshold )
							dst[x] = -1 ;
						else
						{
							dst[x] = MAKE_INDEXED_COLOR12(red,green,blue);
							add_index_color( cmap->hash, dst[x], ((dst[x]>>14)&0x3FF), red, green, blue);
						}
						red   = INDEX_UNESHIFT_RED(red,1)    &0x07 ;
						green = INDEX_UNESHIFT_GREEN(green,1)&0x07 ;
						blue  = INDEX_UNESHIFT_BLUE(blue,1)  &0x07 ;
					}
				}
			    break ;
			case 5 :                           /* 333 */
				{
					for( x = 0 ; x < scl.width ; ++x )
					{
						red   = INDEX_SHIFT_RED  ((red  +r[x]>255)?255:red+r[x]);
						green = INDEX_SHIFT_GREEN((green+g[x]>255)?255:green+g[x]);
						blue  = INDEX_SHIFT_BLUE ((blue +b[x]>255)?255:blue+b[x]);
						if( a[x] < opaque_threshold )
							dst[x] = -1 ;
						else
						{
							dst[x] = MAKE_INDEXED_COLOR9(red,green,blue);
							add_index_color( cmap->hash, dst[x], ((dst[x]>>18)&0x03F), red, green, blue);
						}
						red   = INDEX_UNESHIFT_RED(red,1)&0x0F ;
						green = INDEX_UNESHIFT_GREEN(green,1)&0x0F ;
						blue  = INDEX_UNESHIFT_BLUE(blue,1)  &0x0F ;
					}
				}
			    break ;
			case 6 :                           /* 222 */
				{
					for( x = 0 ; x < scl.width ; ++x )
					{
						red   = INDEX_SHIFT_RED  ((red  +r[x]>255)?255:red+r[x]);
						green = INDEX_SHIFT_GREEN((green+g[x]>255)?255:green+g[x]);
						blue  = INDEX_SHIFT_BLUE ((blue +b[x]>255)?255:blue+b[x]);
						if( a[x] < opaque_threshold )
							dst[x] = -1 ;
						else
						{
							dst[x] = MAKE_INDEXED_COLOR6(red,green,blue);
							add_index_color( cmap->hash, dst[x], ((dst[x]>>18)&0x03F), red, green, blue);
						}
						red   = INDEX_UNESHIFT_RED(red,1)&0x01F ;
						green = INDEX_UNESHIFT_GREEN(green,1)&0x01F ;
						blue  = INDEX_UNESHIFT_BLUE(blue,1)  &0x01F ;
					}
				}
			    break ;
			case 7 :                           /* 111 */
				{
					for( x = 0 ; x < scl.width ; ++x )
					{
						red   = INDEX_SHIFT_RED  ((red  +r[x]>255)?255:red+r[x]);
						green = INDEX_SHIFT_GREEN((green+g[x]>255)?255:green+g[x]);
						blue  = INDEX_SHIFT_BLUE ((blue +b[x]>255)?255:blue+b[x]);
						if( a[x] < opaque_threshold )
							dst[x] = -1 ;
						else
						{
							dst[x] = MAKE_INDEXED_COLOR3(red,green,blue);
							add_index_color( cmap->hash, dst[x], ((dst[x]>>21)&0x07), red, green, blue);
						}
						red   = INDEX_UNESHIFT_RED(red,1)&0x03F ;
						green = INDEX_UNESHIFT_GREEN(green,1)&0x03F ;
						blue  = INDEX_UNESHIFT_BLUE(blue,1)  &0x03F ;
					}
				}
			    break ;
		}
		dst += im->width ;
	}
	free_scanline(&scl, True);
	SHOW_TIME("color indexing",started);

#ifdef LOCAL_DEBUG
check_colorindex_counts( cmap->hash );
#endif

	LOCAL_DEBUG_OUT("building colormap%s","");
	color_hash2colormap( cmap, max_colors );
	SHOW_TIME("colormap calculation",started);

#ifdef LOCAL_DEBUG
check_colorindex_counts( cmap->hash );
#endif

	dst = mapped_im ;
	for( y = 0 ; y < im->height ; ++y )
	{
		switch( dither )
		{
			case 0 :
			case 1 :
			case 2 :
				for( x = 0 ; x < im->width ; ++x )
					if( dst[x] >= 0 )
						dst[x] = get_color_index( cmap->hash, dst[x], ((dst[x]>>12)&0x0FFF));
					else
						dst[x] = cmap->count ;
				break;
			case 3 :
			case 4 :
				for( x = 0 ; x < im->width ; ++x )
				{
					LOCAL_DEBUG_OUT( "(%d,%d)", x, y );
					if( dst[x] >= 0 )
						dst[x] = get_color_index( cmap->hash, dst[x], ((dst[x]>>14)&0x03FF));
					else
						dst[x] = cmap->count ;
				}
				break;
			case 5 :
			case 6 :
				for( x = 0 ; x < im->width ; ++x )
					if( dst[x] >= 0 )
						dst[x] = get_color_index( cmap->hash, dst[x], ((dst[x]>>18)&0x03F));
					else
						dst[x] = cmap->count ;
			    break ;
			case 7 :
				for( x = 0 ; x < im->width ; ++x )
					if( dst[x] >= 0 )
						dst[x] = get_color_index( cmap->hash, dst[x], ((dst[x]>>21)&0x007));
					else
						dst[x] = cmap->count ;
			    break ;
		}
		dst += im->width ;
	}

	return mapped_im ;
}

