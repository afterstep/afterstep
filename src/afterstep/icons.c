
/*
 * Copyright (C) 2005 Fabian Yamaguchi <fabiany at gmx.net>
 * Copyright (C) 1998 Sasha Vasko <sasha at aftercode.net>
 * Copyright (C) 1995 Bo Yang
 * Copyright (C) 1993 Robert Nation
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#define LOCAL_DEBUG

#include "../../configure.h"

#include "asinternals.h"

#include <unistd.h>
#include <signal.h>
#include <X11/Intrinsic.h>


/***********************************************************************/
/* new IconBox handling :                                              */
/***********************************************************************/

void
destroy_asiconbox( ASIconBox **pib )
{
	if( pib && *pib)
	{
		register ASIconBox *ib = *pib ;
		if( ib->areas )
			free( ib->areas );
		if( ib->icons )
            destroy_asbidirlist( &(ib->icons) );
		free( ib );
	  	*pib = NULL ;
	}
}

ASIconBox *
get_iconbox( int desktop )
{
	ASIconBox *ib = NULL ;
	if( IsValidDesk(desktop) )
	{
        if( get_flags(Scr.Feel.flags, StickyIcons) )
			ib = Scr.default_icon_box ;
		else
			if( Scr.icon_boxes )
			{
				ASHashData hdata ;
                if( get_hash_item( Scr.icon_boxes, AS_HASHABLE(desktop), &hdata.vptr ) != ASH_Success )
					ib = NULL ;
				else
					ib = hdata.vptr ;
			}
		if( ib == NULL )
		{
			ib = safecalloc( 1, sizeof( ASIconBox ));
            if( Scr.Look.configured_icon_areas_num == 0 || Scr.Look.configured_icon_areas == NULL )
			{
				ib->areas_num = 1 ;
                ib->areas = safecalloc( 1, sizeof(ASGeometry) );
				ib->areas->width = Scr.MyDisplayWidth ;
				ib->areas->height = Scr.MyDisplayHeight ;
                ib->areas->flags = 0;
			}else
			{
                register int i = Scr.Look.configured_icon_areas_num ;
				ib->areas_num = i ;
                ib->areas = safecalloc( ib->areas_num, sizeof(ASGeometry) );
				while( --i >= 0 )
                    ib->areas[i] = Scr.Look.configured_icon_areas[i];
			}
			ib->icons = create_asbidirlist( NULL );
            if( get_flags(Scr.Feel.flags, StickyIcons) )
				Scr.default_icon_box = ib ;
			else
			{
				if( Scr.icon_boxes == NULL )
					Scr.icon_boxes = create_ashash( 3, NULL, NULL, NULL );

				if( add_hash_item( Scr.icon_boxes, AS_HASHABLE(desktop), ib ) != ASH_Success )
					destroy_asiconbox( &ib );
			}
		}
	}
	return ib;
}

typedef struct ASIconRearrangeAux
{
    int curr_area ;
    int last_x, last_y, last_width, last_height ;
    ASIconBox *ib;
}ASIconRearrangeAux;

void fix_iconbox_area( ASGeometry *geom, int min_width, int min_height )
{
	if( geom->width  <= min_width )
	{
		if( get_flags(geom->flags, XNegative) )
			geom->x -= min_width-geom->width ;
		geom->width = min_width ;
	}
	if( geom->height  < min_height )
	{
		if( get_flags(geom->flags, YNegative) )
			geom->y -= min_height-geom->height ;
		geom->height = min_height ;
	}
}

Bool rearrange_icon_iter_func(void *data, void *aux_data)
{
    ASWindow *asw = (ASWindow*) data ;
    ASIconRearrangeAux *rd = (ASIconRearrangeAux *)aux_data ;
    int width = 0, height = 0, title_width = 0, title_height = 0;
    int whole_width = 0, whole_height = 0 ;
    int x, y, box_x = 0, box_y = 0;
    Bool placed = False ;
    ASGeometry *geom = NULL ;

    int new_x = -1, new_y = -1 ;

    /*
      p stands for primary coordinate, the coordinate which is changed
      first when trying to place a new icon. In the past, this
      was the x coordinate. In that case the y coordinate is the
      secondary coordinate (s), that means we move it only if the primary
      coordinate is no longer valid after moving it. (Start a new
      row if row is full)
    */

    int *new_p, *new_s;
    int *geom_size_p, *geom_size_s;
    unsigned int PNegative, SNegative;
    int *last_p, *last_s;
    int *whole_p, *whole_s;
    int *last_p_size, *last_s_size;
    

    Bool vertical = get_flags(Scr.Look.flags, IconsGrowVertically);
    
    
    if( AS_ASSERT(asw) || AS_ASSERT(rd) )
		return False;

	LOCAL_DEBUG_OUT( "asw(%p)->magic(%lX)->icon_canvas(%p)->icon_title_canvas(%p)->icon_button(%p)->icon_title(%p)",
					 asw, asw->magic, asw->icon_canvas, asw->icon_title_canvas, asw->icon_button, asw->icon_title );
    if( asw->icon_canvas == NULL || rd->ib == NULL )
        return True;

    if( asw->icon_button )
    {
		if( Scr.Look.ButtonWidth == 0 ) 
			width = calculate_astbar_width( asw->icon_button );
		else
			width = Scr.Look.ButtonWidth ; 
		if( Scr.Look.ButtonHeight == 0 ) 
        	height = calculate_astbar_height( asw->icon_button );
		else
			height = Scr.Look.ButtonHeight ;
    }
    if( asw->icon_title )
    {
		title_width = calculate_astbar_width( asw->icon_title );
		if( title_width < width ) 
			title_width = width ;
        title_height = calculate_astbar_height( asw->icon_title );
    	LOCAL_DEBUG_OUT( "title_size = %dx%d", title_width, title_height );
	}

	if( get_flags(Scr.Look.flags, SeparateButtonTitle) ) 
	{	
		if( title_width > width ) 
		{	
			if( title_width > width*3 ) 
				title_width = width * 3 ;
			whole_width = title_width ;
		}else
			whole_width = width ;
	}else
	    whole_width = (width == 0)? title_width : width ;

    whole_height = height + title_height ;
    /* now we could determine where exactly to place icon to : */
    x = -whole_width ;
    y = -whole_height ;

    if( rd->curr_area < 0 && rd->ib->areas_num > 0 )
   	{
		rd->curr_area = 0 ;
    	geom = &(rd->ib->areas[0]) ;
		fix_iconbox_area( geom, whole_width, whole_height );
        rd->last_x = get_flags( geom->flags, XNegative )?geom->width: 0 ;
        rd->last_y = get_flags( geom->flags, YNegative )?geom->height: 0 ;
	}else
	{
		geom = &(rd->ib->areas[rd->curr_area]) ;
		fix_iconbox_area( geom, whole_width, whole_height );
	}


    /* Set primary/secondary-pointers */
    if( vertical )
    {
	    
	    new_p = &new_y;
	    new_s = &new_x;
	    geom_size_p = &(geom->height);
	    geom_size_s = &(geom->width);
	    PNegative = YNegative;
	    SNegative = XNegative;
	    last_p = &(rd->last_y);
	    last_s = &(rd->last_x);
	    whole_p = &whole_height;
	    whole_s = &whole_width;
	    last_p_size = &(rd->last_height);
	    last_s_size = &(rd->last_width);
	    
    }else{
	    
	    new_p = &new_x;
	    new_s = &new_y;
	    geom_size_p = &(geom->width);
	    geom_size_s = &(geom->height);
	    PNegative = XNegative;
	    SNegative = YNegative;
	    last_p = &(rd->last_x);
	    last_s = &(rd->last_y);
	    whole_p = &whole_width;
	    whole_s = &whole_height;
	    last_p_size = &(rd->last_width);
	    last_s_size = &(rd->last_height);
    }
    

	LOCAL_DEBUG_OUT( "entering loop : areas_num = %d", rd->ib->areas_num );
    while( rd->curr_area < rd->ib->areas_num )
    {
	    new_x = new_y = -1;

LOCAL_DEBUG_OUT( "trying area #%d : %s%s, %dx%d%+d%+d", rd->curr_area, get_flags(geom->flags, XNegative)?"XNeg":"XPos", get_flags(geom->flags, YNegative)?"YNeg":"YPos", geom->width, geom->height, geom->x, geom->y );
LOCAL_DEBUG_OUT( "last: %dx%d%+d%+d", rd->last_width, rd->last_height, rd->last_x, rd->last_y );

           /* change primary coordinate */ 
		if( get_flags(geom->flags, PNegative) )
		  *new_p = *last_p - *whole_p ;
		else
		  *new_p = *last_p + *last_p_size ;
		
		/* if primary-coordinate is now invalid */
        if( *new_p < 0 || *new_p + *whole_p > *geom_size_p )
        {  /* lets try and go to the next row and see if that makes a difference */
          /* change secondary coordinate */  
	  if( get_flags(geom->flags, SNegative) )
                *last_s -=  *last_s_size ;
            else
                *last_s += *last_s_size ;
            
	  /* reset primary coordinate */
	    *last_p = get_flags( geom->flags, PNegative )? *geom_size_p - 1: 0 ;

	    /* adjust primary coordinate if PNegative */
            if( get_flags(geom->flags, PNegative) )
                *(new_p) = *last_p -= *whole_p ;
        }else
            *last_p = *new_p ;

        *new_s = *last_s ;
        *new_p = *last_p ;
	
	/* Adjust secondary coordinate if SNegative*/
	if( get_flags(geom->flags, SNegative) )
            *new_s = *last_s - *whole_s ;

LOCAL_DEBUG_OUT( "new : %+d%+d", new_x, new_y );
        if( new_x >= 0 && new_x+whole_width <= geom->width &&
            new_y >= 0 && new_y+whole_height <= geom->height )
        {
            x = new_x ;
            y = new_y ;
            box_x = geom->x ;
            box_y = geom->y ;
			placed = True ;
            break;
        }

        ++(rd->curr_area);
        if( rd->curr_area < rd->ib->areas_num )
        {
            geom = &(rd->ib->areas[rd->curr_area]) ;
			fix_iconbox_area( geom, whole_width, whole_height );
            rd->last_x = get_flags( geom->flags, XNegative )?geom->width: 0 ;
            rd->last_y = get_flags( geom->flags, YNegative )?geom->height: 0 ;
        }else
        {
            rd->last_x = 0 ;
            rd->last_y = 0 ;
        }

        rd->last_width = 0 ;
        rd->last_height = 0 ;
    }
    /* placing the icon : */
LOCAL_DEBUG_OUT( "placing an icon at %+d%+d, base %+d%+d whole %dx%d button %dx%d", x, y, box_x, box_y, whole_width, whole_height, width, height );
    if( asw->icon_title_canvas && asw->icon_title_canvas != asw->icon_canvas )
    {
        moveresize_canvas( asw->icon_canvas, box_x+x+(title_width-width)/2, box_y+y, width, height );
        moveresize_canvas( asw->icon_title_canvas, box_x+x, box_y+y+height, title_width, title_height );
    }else
        moveresize_canvas( asw->icon_canvas, box_x+x, box_y+y, whole_width, whole_height );

    *last_p_size  = *whole_p  ;
    if( *whole_s > *last_s_size )
        *last_s_size= *whole_s ;
    return True;
}

void
rearrange_iconbox( ASIconBox *ib )
{
    ASIconRearrangeAux aux_data ;

	if( ib == NULL )
		return ;
    aux_data.curr_area = -1 ;
    aux_data.last_x = 0;
    aux_data.last_y = 0;
    aux_data.last_width = 0 ;
    aux_data.last_height = 0 ;
    aux_data.ib = ib ;

    iterate_asbidirlist( ib->icons, rearrange_icon_iter_func, &aux_data, NULL, False);
}

Bool
add_iconbox_icon( ASWindow *asw )
{
    ASIconBox *ib = NULL ;
    if( AS_ASSERT(asw) )
        return False;

    /* we need to add this window to the list of icons,
     * and then place it in appropriate position : */
    if( (ib = get_iconbox( ASWIN_DESK(asw) )) == NULL )
		return False;
   	
	append_bidirelem( ib->icons, asw );
   	rearrange_iconbox( ib );
    
	return True;
}

Bool
remove_iconbox_icon( ASWindow *asw )
{
    ASIconBox *ib = NULL ;
	Bool success = False ;
    
	if( AS_ASSERT(asw) )
        return False;
    /* we need to remove this window from the list of icons - 
	 * going through all existing icon boxes just in case : */
	if( Scr.default_icon_box )
	{	
		ib = Scr.default_icon_box ;
		if( ib && ib->icons ) 
			if( discard_bidirelem( ib->icons, asw ) ) 
			{
				rearrange_iconbox( ib );			
				success = True;
			}
	}
	
	if( Scr.icon_boxes ) 
	{
		ASHashIterator iterator ; 
	    if( start_hash_iteration( Scr.icon_boxes, &iterator ))
        do
        {
			ib = (ASIconBox*)curr_hash_data(&iterator);
			if( discard_bidirelem( ib->icons, asw ) ) 
			{
				rearrange_iconbox( ib );			
				success = True;
			}
        }while(next_hash_item( &iterator ));	
	}

    return success;
}

Bool
change_iconbox_icon_desk( ASWindow *asw, int from_desk, int to_desk )
{
    return False;
}

void
on_icon_changed( ASWindow *asw )
{
	if( AS_ASSERT(asw) )
        return;
    /* we probably need to reshuffle entire iconbox when that happen : */
    if( asw->icon_title )
    {
        int width = calculate_astbar_width( asw->icon_title );
        int height = calculate_astbar_height( asw->icon_title );
		if( width <  Scr.Look.ButtonWidth ) 
			width = Scr.Look.ButtonWidth ;
        set_astbar_size( asw->icon_title, width, height );
		if( asw->icon_title_canvas && asw->icon_title_canvas != asw->icon_canvas ) 
		{	
	    	resize_canvas( asw->icon_title_canvas, width, height );
			handle_canvas_config( asw->icon_title_canvas );
		}
        render_astbar( asw->icon_title, asw->icon_title_canvas?asw->icon_title_canvas:asw->icon_canvas );
    }
    if( asw->icon_button )
    {
        render_astbar( asw->icon_button, asw->icon_canvas );
	}
    update_canvas_display( asw->icon_canvas );
    update_canvas_display( asw->icon_title_canvas );
}

void
rearrange_iconbox_icons( int desktop )
{
    rearrange_iconbox( get_iconbox( desktop ) );
}

