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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#define LOCAL_DEBUG

#include "../include/config.h"

#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include "astypes.h"
#include "safemalloc.h"
#include "output.h"
#include "layout.h"

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

static inline void
destroy_layout_row( ASLayoutElem **prow )
{
	register ASLayoutElem *pelem = *prow;
	while( pelem )
	{
		register ASLayoutElem *tmp = pelem->right ;
        free( pelem );
		pelem = tmp ;
	}
	*prow = NULL ;
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
		ASLayoutElem *elem = safecalloc( 1, sizeof(ASLayoutElem) );

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
	if( layout )
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
    }
	return head ;
}

ASLayoutElem *
extract_layout_context( ASLayout *layout, int context )
{
    ASLayoutElem *elem = NULL;
    if( layout )
    {
        register ASLayoutElem **pelem = NULL ;
        register int i ;
        for( i = 0 ; i < layout->dim_y ; ++i )
            for( pelem = &(layout->rows[i]) ; *pelem ; pelem = &((*pelem)->right) )
                if( (*pelem)->context == context )
                {
                    elem = *pelem ;
                    *pelem = elem->right ;
                    for( pelem = &(layout->cols[elem->column]) ; *pelem ; pelem = &((*pelem)->below) )
                        if( *pelem == elem )
                        {
                            *pelem = elem->below;
                            break;
                        }
                }
    }
    return elem ;
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
void
set_layout_spacing( ASLayout *layout, unsigned int h_border, unsigned int v_border, unsigned int h_spacing, unsigned int v_spacing )
{
    if( layout )
	{
    	layout->h_border = h_border ;
  	    layout->v_border = v_border ;
  		layout->h_spacing = h_spacing ;
  		layout->v_spacing = v_spacing ;
	}
}

void
set_layout_offsets( ASLayout *layout, int east, int north, int west, int south )
{
    if( layout )
    {
	    layout->offset_east  = east  ;
  		layout->offset_north = north ;
      	layout->offset_west  = west  ;
      	layout->offset_south = south ;
    }
}

/**********************************************************************/
/* fixed size handling :                                              */
void
get_layout_fixed_size( ASLayout *layout, unsigned short *fixed_width, unsigned short *fixed_height )
{
    int width = 0, height = 0 ;
    if( layout )
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

