/*
 * Copyright (c) 2000 Sasha Vasko <sasha at aftercode.net>
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

#define LOCAL_DEBUG

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include "astypes.h"
#include "safemalloc.h"
#include "output.h"
#include "layout.h"


/* these are used to perform different size/position adjustments in order
 * to avoid dynamic memory allocations :
 */
static int as_layout_fixed_width[ASLAYOUT_MAX_SIZE], as_layout_fixed_height[ASLAYOUT_MAX_SIZE] ;
static int as_layout_width[ASLAYOUT_MAX_SIZE], as_layout_height[ASLAYOUT_MAX_SIZE] ;
static int as_layout_x[ASLAYOUT_MAX_SIZE], as_layout_y[ASLAYOUT_MAX_SIZE] ;


/**********************************************************************/
/* Creation/Destruction                                               */
ASLayout *
create_aslayout( unsigned int dim_x, unsigned int dim_y )
{
    ASLayout *layout = NULL ;

	if( dim_x && dim_y )
	{
		layout = safecalloc( 1, sizeof(ASLayout));
		layout->dim_x = dim_x ;
		layout->dim_y = dim_y ;
		if( dim_x > 0 )
			layout->cols = safecalloc( dim_x, sizeof(ASLayoutElem*));
		if( dim_y > 0 )
			layout->rows = safecalloc( dim_y, sizeof(ASLayoutElem*));
	}
    return layout;
}

static inline int
destroy_layout_row( ASLayoutElem **prow )
{
	register ASLayoutElem *pelem = *prow;
	int count = 0 ;
	while( pelem )
	{
		register ASLayoutElem *tmp = pelem->right ;
        free( pelem );
		pelem = tmp ;
		++count;
	}
	*prow = NULL ;
	return count;
}

void
destroy_aslayout( ASLayout **playout )
{
    if( playout )
	{
		register ASLayout *layout = *playout ;
        if( layout )
        {
            register int i ;
            for( i = 0 ; i < layout->dim_y ; i++ )
				destroy_layout_row( &(layout->rows[i]));
			destroy_layout_row( &(layout->disabled) );

			if( layout->rows )
				free( layout->rows );
			if( layout->cols )
				free( layout->cols );
			layout->dim_x = layout->dim_y = 0 ;
            free( layout );
            *playout = NULL ;
        }
	}
}

/**********************************************************************/
/* addition/extraction of elements :                                  */
void
insert_layout_elem( ASLayout *layout,
	  				ASLayoutElem *elem,
                 	unsigned int h_slot, unsigned int v_slot,
                 	unsigned int h_span, unsigned int v_span )
{
    if( layout )
    {
        ASLayoutElem **pelem, **pelem2 ;

		if( h_slot >= ASLAYOUT_MAX_SIZE )
            h_slot = ASLAYOUT_MAX_SIZE-1 ;
        if( v_slot >= ASLAYOUT_MAX_SIZE )
            v_slot = ASLAYOUT_MAX_SIZE-1 ;
        if( h_span > ASLAYOUT_MAX_SIZE-h_slot )
            h_span = ASLAYOUT_MAX_SIZE-h_slot ;
        if( v_span > ASLAYOUT_MAX_SIZE-v_slot )
            v_span = ASLAYOUT_MAX_SIZE-v_slot ;

		if( layout->dim_x < h_slot+h_span )
		{
            layout->cols = realloc( layout->cols, (h_slot+h_span)*sizeof(ASLayoutElem*) );
			memset( layout->cols+ layout->dim_x, 0x00, ((h_slot+h_span) - layout->dim_x) *sizeof( ASLayoutElem*) );
			layout->dim_x = h_slot+h_span ;
		}
		if( layout->dim_y < v_slot+v_span )
		{
            layout->rows = realloc( layout->rows, (v_slot+v_span)*sizeof(ASLayoutElem*) );
			memset( layout->rows+ layout->dim_y, 0x00, ((v_slot+v_span) - layout->dim_y) *sizeof( ASLayoutElem*) );
			layout->dim_y = v_slot+v_span ;
		}

        for( pelem = &(layout->rows[v_slot]); *pelem ; pelem = &((*pelem)->right) )
			if( (*pelem)->column >= h_slot )
				break ;
        for( pelem2 = &(layout->cols[h_slot]); *pelem2 ; pelem2 = &((*pelem2)->below) )
        {
			if( (*pelem2)->row >= v_slot )
				break ;
        }
		if( *pelem && *pelem == *pelem2 )
		{
            elem->right = (*pelem)->right ;
            elem->below = (*pelem)->below ;
			(*pelem)->right = (*pelem)->below = NULL ;
			free( pelem );
		}else
		{
            elem->right = *pelem ;
            elem->below = *pelem2 ;
			++(layout->count);
		}
		*pelem = elem ;
		*pelem2 = elem ;
        elem->h_span = h_span ;
        elem->v_span = v_span ;
        elem->row = v_slot ;
        elem->column = h_slot ;
    }
}

ASLayoutElem *
gather_layout_elems( ASLayout *layout )
{
    ASLayoutElem *head = NULL;
	if( layout && layout->count > 0 )
	{
		register int i ;

        head = layout->disabled ;
        layout->disabled = NULL ;
		for( i = 0 ; i < layout->dim_y ; ++i )
		{
			register ASLayoutElem *pelem = layout->rows[i] ;
			if( pelem )
			{
				while( pelem->right )
				{
					pelem->below = NULL ;
					pelem = pelem->right ;
				}
				pelem->below = NULL ;
				pelem->right = head ;
				head = layout->rows[i] ;
				layout->rows[i] = NULL ;
			}
		}
        for( i = 0 ; i < layout->dim_x ; i++ )
            layout->cols[i] = NULL ;
		layout->count = 0 ;
    }
	return head ;
}

void
flush_layout_elems( ASLayout *layout )
{
    if( layout && layout->count > 0 )
    {
        register int i ;
        for( i = 0 ; i < layout->dim_y ; i++ )
			destroy_layout_row( &(layout->rows[i]));
		destroy_layout_row( &(layout->disabled) );
		layout->count = 0 ;
	}
}

static ASLayoutElem **
get_layout_context_ptr( ASLayout *layout, int context )
{
	register ASLayoutElem **pelem = NULL ;
    register int i ;
    for( i = 0 ; i < layout->dim_y ; ++i )
    	for( pelem = &(layout->rows[i]) ; *pelem ; pelem = &((*pelem)->right) )
        	if( (*pelem)->context == context )
				return pelem ;
	return NULL;
}

ASLayoutElem *
extract_layout_context( ASLayout *layout, int context )
{
    ASLayoutElem *elem = NULL;
    if( layout && layout->count )
    {
        register ASLayoutElem **pelem = NULL ;
		if((pelem = get_layout_context_ptr( layout, context )) != NULL )
		{
            elem = *pelem ;
            *pelem = elem->right ;
            for( pelem = &(layout->cols[elem->column]) ; *pelem ; pelem = &((*pelem)->below) )
                if( *pelem == elem )
                {
                    *pelem = elem->below;
                    break;
                }
			--(layout->count) ;
		}
    }
    return elem ;
}

ASLayoutElem *
find_layout_context( ASLayout *layout, int context )
{
    if( layout && layout->count )
    {
        register ASLayoutElem **pelem = NULL ;
		if((pelem = get_layout_context_ptr( layout, context )) != NULL )
			return *pelem ;
    }
    return NULL ;
}
/**********************************************************************/
/* enable/disable element                                             */
void
disable_layout_elem( ASLayout *layout, ASLayoutElem **pelem )
{
    if( layout && pelem && *pelem )
    {
        ASLayoutElem *elem = *pelem ;

        /* step one - taking out of horisontal circulation : */
        *pelem = elem->right ;
        /* step two - taking out of vertical circulation :) */
        for( pelem = &(layout->cols[elem->column]) ; *pelem ; pelem = &((*pelem)->below) )
            if( *pelem == elem )
                break;
        if( *pelem )
            *pelem = elem->below ;
        elem->below = NULL;
        /* step three - stashing away for later reuse :*/
        elem->right = layout->disabled ;
        layout->disabled = elem ;
		--(layout->count);
    }
}

void
enable_layout_elem( ASLayout *layout, ASLayoutElem **pelem )
{
    if( layout && pelem && *pelem )
    {
        ASLayoutElem *elem = *pelem ;

        /* step one - taking out of disabled list : */
        *pelem = elem->right ;
        /* step two - reinserting us into The MATRIX !!!! BWUHAHAHA */
        elem->right = NULL ;
        if( elem->column+elem->h_span <= layout->dim_x &&
            elem->row+elem->v_span <= layout->dim_y)
            insert_layout_elem( layout, elem, elem->column, elem->row, elem->h_span, elem->v_span );
        else
			free( elem );
    }
}

int
disable_layout_context( ASLayout *layout, int context, Bool batch )
{
    int found = 0 ;
    if( layout )
    {
        register ASLayoutElem **pelem = NULL ;
        register int i ;
        for( i = 0 ; i < layout->dim_y ; i++ )
            for( pelem = &(layout->rows[i]) ; *pelem ; pelem = &((*pelem)->right) )
            {
                if( (*pelem)->context == context )
                {
                    disable_layout_elem( layout, pelem );
                    found++;
                }
            }
    }
    if( found > 0 && !batch )
    {
/*
        mylayout_moveresize( widget->layout, widget->width, widget->height, True );
        mywidget_update_fixed_size( widget, False );
*/
	}
    return found ;
}

int
enable_layout_context( ASLayout *layout, int context, Bool batch )
{
    int found = 0 ;
	if( layout )
    {
        register ASLayoutElem **pelem = NULL ;
        for( pelem = &(layout->disabled) ; *pelem ; pelem = &((*pelem)->right) )
            if( (*pelem)->context == context )
            {
                enable_layout_elem( layout, pelem );
                found++;
            }
    }
    if( found > 0 && !batch )
    {
/*        mylayout_moveresize( widget->layout, widget->width, widget->height, True );
        mywidget_update_fixed_size( widget, False );
 */
    }
    return found ;
}


/**********************************************************************/
/* spacing and side offset management                                 */
Bool
set_layout_spacing( ASLayout *layout, unsigned int h_border, unsigned int v_border, unsigned int h_spacing, unsigned int v_spacing )
{
	Bool changed = False ;
    if( layout )
	{
		changed = (	layout->h_border  != h_border ||
  	    			layout->v_border  != v_border ||
					layout->h_spacing != h_spacing ||
					layout->v_spacing != v_spacing );
		if( changed )
		{
    		layout->h_border = h_border ;
  	    	layout->v_border = v_border ;
  			layout->h_spacing = h_spacing ;
  			layout->v_spacing = v_spacing ;
		}
	}
	return changed;
}

Bool
set_layout_offsets( ASLayout *layout, int east, int north, int west, int south )
{
    Bool changed = False ;
    if( layout )
    {
		changed = ( layout->offset_east  != east  ||
  					layout->offset_north != north ||
      				layout->offset_west  != west  ||
					layout->offset_south != south );
		if( changed )
		{
	    	layout->offset_east  = east  ;
  			layout->offset_north = north ;
      		layout->offset_west  = west  ;
      		layout->offset_south = south ;
		}
	}
	return changed;
}

/**********************************************************************/
/* fixed size handling :                                              */
void
get_layout_fixed_size( ASLayout *layout, CARD32 *fixed_width, CARD32 *fixed_height )
{
    int width = 0, height = 0 ;
    if( layout && layout->count > 0 )
    {
        register int i ;
        if( fixed_width )
        {
            width = layout->v_border ;
            for( i = 0 ; i < layout->dim_x ; i++ )
            {
                register ASLayoutElem *pelem = layout->cols[i];
                register int fw = 0 ;
                while ( pelem )
                {
                    if( get_flags( pelem->flags, LF_FixedWidth ) )
					{
                        LOCAL_DEBUG_OUT( " layout %lX found item with fixed width %d at %dx%d",
                                         (unsigned long)layout, pelem->fixed_width, pelem->row, pelem->column );
                        if( pelem->fixed_width+pelem->bw > fw )
                            fw = pelem->fixed_width+pelem->bw ;
                    }
					pelem = pelem->below ;
                }
                if( fw > 0 )
                    width += fw+layout->v_spacing ;
            }
            if( width > 0 )
                width += layout->v_border + ((int)layout->v_border - (int)layout->v_spacing)+layout->offset_east+layout->offset_west ;
        }
        if( fixed_height )
        {
            height = 0 ;
            for( i = 0 ; i < layout->dim_y ; i++ )
            {
                register ASLayoutElem *pelem = layout->rows[i];
                register int fh = 0 ;
                while ( pelem )
                {
                    if( get_flags( pelem->flags, LF_FixedHeight ) )
					{
LOCAL_DEBUG_OUT( " layout %lX found item with fixed height %d at %dx%d", (unsigned long)layout, pelem->fixed_height, pelem->row, pelem->column );
                        if( pelem->fixed_height+pelem->bw > fh )
                            fh = pelem->fixed_height+pelem->bw ;
					}
                    pelem = pelem->right ;
                }
                if( fh > 0 )
                    height += fh+layout->h_spacing ;
            }
            if( height >0 )
                height += layout->h_border +((int)layout->h_border - (int)layout->h_spacing) + (int)layout->offset_north +(int)layout->offset_south ;
        }
    }
LOCAL_DEBUG_OUT( " layout %lX FIXED WIDTH is %d FIXED HEIGHT is %d", (unsigned long)layout, width, height );
    if( fixed_width )
        *fixed_width = width ;
    if( fixed_height )
        *fixed_height = height ;
}

ASFlagType
set_layout_context_fixed_size( ASLayout *layout, int context, unsigned int width, unsigned int height )
{
    if( layout && layout->count > 0 )
    {
        ASLayoutElem **pelem = get_layout_context_ptr( layout, context );
		LOCAL_DEBUG_OUT( "setting fixedsize of context %d(%p) to %dx%d", context, *pelem, width, height );
		if( pelem != NULL )
		{
            register ASLayoutElem *elem = *pelem ;
			elem->fixed_width = width ;
			elem->fixed_height = height ;
			return elem->flags&LF_FixedSize;
		}
    }
	return 0;
}

Bool
get_layout_context_size( ASLayout *layout, int context, int *x, int *y, unsigned int *width, unsigned int *height )
{
    if( layout && layout->count > 0 )
    {
        ASLayoutElem **pelem = get_layout_context_ptr( layout, context );
		if( pelem != NULL )
		{
            register ASLayoutElem *elem = *pelem ;
			if( x )
				*x = elem->x ;
			if( y )
				*y = elem->y ;
			if( width )
				*width = elem->width ;
			if( height )
				*height = elem->height ;
			return True;
		}
    }
	return False;
}

ASLayoutElem *
find_layout_point( ASLayout *layout, int x, int y, ASLayoutElem *start )
{
	if( layout && layout->count > 0 )
	{
		register int col = start? start->column : 0 ;

		x -= layout->x ;
		y -= layout->y ;

		for( ; col < layout->dim_x ; ++col )
		{
			register ASLayoutElem *pelem = layout->cols[col] ;
			if( start && start->column == col )
				pelem = start->below ;
			if( pelem )
			{
				if( pelem->x > x )
					return NULL;
				for( ; pelem != NULL ; pelem = pelem->below )
				{
					if( pelem->y > y )
						break;
					if( pelem->x+pelem->width > x && pelem->y+pelem->height > y )
						return pelem;
				}
			}
		}
	}
	return NULL ;
}
/****************************************************************************************
 * The following are dynamic methods and return value that indicates if any of the cached
 * pixmaps has to be rebuild
 ****************************************************************************************/
static int
collect_sizes( ASLayout *layout, int *layout_size, int *layout_fixed_size, Bool h_direction )
{
    int dim ;
    ASLayoutElem **chains ;
    register int i ;
    int spacing_needed = 0 ;
    int max_span;
    ASFlagType fixed_flag ;
	unsigned int spacing ;

    if( h_direction )
    {
        dim = layout->dim_x ;
        chains = layout->cols ;
        fixed_flag = LF_FixedWidth ;
		spacing = layout->h_spacing ;
    }else
    {
        dim = layout->dim_y ;
        chains = layout->rows ;
        fixed_flag = LF_FixedHeight ;
		spacing = layout->v_spacing ;
    }

    /* PASS 1 : we mark all the dead columns with fixed width -1 */
    for( i = 0 ; i < dim  ; i++ )
        layout_fixed_size[i] = chains[i]?0:-1 ;
    /* PASS 2 : we calculate fixed size for columns tarting with elements that has span on 1
     *          and increasing it untill it reaches up to DIM : */
    for( max_span = 1; max_span <= dim  ; max_span++ )
    {
        for( i = dim-max_span ; i >= 0  ; i-- )
        {
            register ASLayoutElem *pelem = chains[i];
            while( pelem )
            {
                unsigned int span ;
                int fixed_size ;
                ASLayoutElem *pnext ;

                if( h_direction )
                {
                    span = pelem->h_span;
                    pnext = pelem->below;
                    fixed_size = get_flags( pelem->flags, LF_FixedWidth )?pelem->fixed_width+(pelem->bw<<1):0 ;
                }else
                {
                    span = pelem->v_span;
                    pnext = pelem->right;
                    fixed_size = get_flags( pelem->flags, LF_FixedHeight )?pelem->fixed_height+(pelem->bw<<1):0 ;
                }

                if( span == max_span && fixed_size > 0 )
                {
                    register int k ;
                    for( k = i + span - 1 ; k > i ; k-- )/* working around span: width = width - spanned_width */
                        if( layout_fixed_size[k] > 0 )
                            fixed_size -= layout_fixed_size[k]+spacing;
                    if( fixed_size > 0 )
                    {
                        if( layout_fixed_size[i] == 0 )
                            layout_fixed_size[i] = fixed_size ;
                        else if( fixed_size > layout_fixed_size[i] )
                        {
                            int limit = i+span ;
                            for( k = i+1 ; k < limit ; k++ )
                                if( layout_fixed_size[k] == 0 )
                                {
                                    layout_fixed_size[k] = layout_fixed_size[i]-(fixed_size+spacing) ;
                                    fixed_size = layout_fixed_size[i] ;
                                }
                            if( fixed_size > layout_fixed_size[i] )
                                layout_fixed_size[i] = fixed_size ;
                        }
                    }
                }
                pelem = pnext ;
            }
        }
    }
    /* PASS 3 : we collect all the existing sizes  */
    if( layout_size != NULL )
    {
        for( i = dim-1 ; i >=0  ; i-- )
        {
            register ASLayoutElem *pelem = chains[i];
            layout_size[i] = 0 ;
            while( pelem )
            {
                unsigned int span ;
                int size ;
                ASLayoutElem *pnext ;
                if( h_direction )
                {
                    span = pelem->h_span;
                    size = pelem->width ;
                    pnext = pelem->below;
                }else
                {
                    span = pelem->v_span;
                    size = pelem->height ;
                    pnext = pelem->right;
                }
                size += (pelem->bw<<1) ;
                if( size > 0 )
                {
                    register int k ;
                    for( k = i + span - 1 ; k > i ; k-- )/* working around span: width = width - spanned_width */
                        if( layout_fixed_size[k] > 0 )
                            size -= layout_size[k]+spacing;
                    if( layout_size[i] < size )  /* this really should be the same in all rows */
                        layout_size[i] = size ;
                }
                pelem = pnext ;
            }
        }
    }
    /* PASS 4 : we mark all the columns that has fixed size 0 yet are overlapped
     * by any fixed item as hidden (-1)  */
    for( i = dim-1 ; i >=0  ; i-- )
    {
        register ASLayoutElem *pelem = chains[i];
        while( pelem )
        {
            unsigned int span ;
            ASLayoutElem *pnext ;
            if( h_direction )
            {
                span = pelem->h_span;
                pnext = pelem->below;
            }else
            {
                span = pelem->v_span;
                pnext = pelem->right;
            }
            if( get_flags( pelem->flags, fixed_flag ) )
            {
                register int k ;
                for( k = i + span - 1 ; k >= i ; k-- )/* working around span: width = width - spanned_width */
                    if( layout_fixed_size[k] == 0 )
                        layout_fixed_size[k] = -1 ;
            }
            pelem = pnext ;
        }
    }
    /* PASS 5 : we collect interelement spacing used  */
    for( i = dim-1 ; i >0  ; i-- )
        if( layout_fixed_size[i] >= 0 )
            spacing_needed += spacing ;

    return spacing_needed;
}

static void
adjust_sizes( unsigned int old_total, unsigned int new_total, unsigned int dim, int *sizes, int *fixed_items )
{
    register int i;
    int available = new_total ;
    int empty_count = 0, non_fixed_count = 0 ;
    int new_size, old_size ;

	/* first allocating space for fixed items */
    for( i = 0 ; i < dim ; i++ )
    {
        if( fixed_items[i] < 0 )
            sizes[i] = 0 ;
        else if( fixed_items[i] > 0 )
        {
            if( available <= 0 )
                sizes[i] = 0 ;
            else
			{
                sizes[i] = MIN( available, fixed_items[i]);
				available -= sizes[i] ;
			}
        }
	}
	/* then allocating space for non-fixed items */
    for( i = 0 ; i < dim ; i++ )
		if( fixed_items[i] == 0 )
        {
            non_fixed_count++;
            if( (old_size = sizes[i]) == 0 )
                empty_count++ ;
            else if( available <= 0 || old_total == 0 )
            {
                sizes[i] = 0 ;
            }else
            {

                new_size = (old_size*new_total)/old_total ;  /* trying to keep the ratio */
                sizes[i] = MIN( new_size, available );
            }
	        available -= sizes[i] ;
        }
    if( available > 0 && empty_count > 0)   /* we have to spread available space among empty non-fixed columns */
    {
        new_size = available/empty_count ;
        if( new_size <= 0 )
            new_size = 1 ;

        for( i = 0 ; i < dim && empty_count ; i++ )
            if( sizes[i] == 0 && fixed_items[i] == 0 )
            {
                sizes[i] = new_size ;
                empty_count-- ;
                if( (available -= new_size ) <= 0 )
                    break;
            }
    }
    if( available > 0 && non_fixed_count > 0 )   /* we have to spread available space among any non-fixed column */
    {
        new_size = available/non_fixed_count ;
        if( new_size <= 0 )
            new_size = 1 ;

        for( i = 0 ; i < dim && non_fixed_count > 0 ; i++ )
            if( fixed_items[i] == 0 )
            {
                if( non_fixed_count == 1 )
                    sizes[i] += available ;
                else
                    sizes[i] += new_size ;
                available -= new_size ;
                non_fixed_count-- ;
            }
    }
}

static void
apply_sizes( int spacing, int start_margin, int dim, int *layout_size, int *layout_fixed_size, int *layout_pos )
{
    register int i ;

    layout_pos[0] = start_margin ;
    for( i = 1 ; i < dim  ; i++ )
    {
        layout_pos[i] = layout_pos[i-1]+layout_size[i-1];
        if( layout_fixed_size[i] >= 0 )
            layout_pos[i] += spacing;
    }
}

Bool
moveresize_layout( ASLayout *layout, unsigned int width, unsigned int height, Bool force )
{
    register int i ;
    int spacing_needed = 0 ;

    if( layout  == NULL )
		return False;

	width -= layout->offset_east+layout->offset_west+(layout->v_border<<1) ;
    height -= layout->offset_north+layout->offset_south+(layout->h_border<<1) ;

    if( width == layout->width && height == layout->height && !force )
        return False;
    /* first working on width/x position */
    spacing_needed = collect_sizes( layout, &(as_layout_width[0]), &(as_layout_fixed_width[0]), True );
    adjust_sizes( layout->width-spacing_needed, width-spacing_needed, layout->dim_x, &(as_layout_width[0]), &(as_layout_fixed_width[0]) );
    apply_sizes(layout->h_spacing, layout->offset_west+layout->v_border, layout->dim_x, &(as_layout_width[0]), &(as_layout_fixed_width[0]), &(as_layout_x[0]) );

    /* then working on height/y position */
    spacing_needed = collect_sizes( layout, &(as_layout_height[0]), &(as_layout_fixed_height[0]), False );
    adjust_sizes( layout->height-spacing_needed, height-spacing_needed, layout->dim_y, &(as_layout_height[0]), &(as_layout_fixed_height[0]) );
    apply_sizes(layout->v_spacing, layout->offset_north+layout->h_border, layout->dim_y, &(as_layout_height[0]), &(as_layout_fixed_height[0]), &(as_layout_y[0]) );

    /* now we can actually apply our calculations : */
    /* becouse of the static arrays we cann not recurse while we need
        * info in those arrays. So on the first pass we set all the x/y/w/h
        * of our elements, and on the second pass - we resize subwidgets
        */
    /* Pass 1: */
    for( i = 0 ; i < layout->dim_y  ; i++ )
        if( layout->rows[i] )
        {
            register ASLayoutElem *pelem = layout->rows[i];
            int elem_y = as_layout_y[i] ;

			do
			{
                register int k ;
	            unsigned int w = as_layout_width[pelem->column] ;
	            unsigned int h = as_layout_height[i] ;
				int elem_x = as_layout_x[pelem->column] ;

                for( k = pelem->column+pelem->h_span-1; k > pelem->column ; k-- )
				{
					if( as_layout_fixed_width[k] >= 0 && as_layout_width[k] > 0 )
                        w += as_layout_width[k]+layout->h_spacing ;
                }
				for( k = pelem->row+pelem->v_span-1; k > pelem->row ; k-- )
				{
			        if( as_layout_fixed_height[k] >= 0 && as_layout_height[k] > 0 )
						h += as_layout_height[k]+layout->v_spacing ;
				}
                LOCAL_DEBUG_OUT( "resizing context %d at [%d:%d] to %dx%d%+d%+d (fixed = %dx%d)", pelem->context, pelem->column, pelem->row, w, h, elem_x, elem_y, pelem->fixed_width, pelem->fixed_height );
                pelem->x = elem_x;
                pelem->y = elem_y;
                pelem->width = w - (pelem->bw<<1);
                pelem->height = h - (pelem->bw<<1);
			}while( (pelem = pelem->right) != NULL );
        }

	layout->width = width;
	layout->height = height;
	return True;
}

