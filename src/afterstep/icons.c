
/*
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

				if( add_hash_item( Scr.icon_boxes, AS_HASHABLE(desktop), &ib ) != ASH_Success )
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

	if( AS_ASSERT(asw) || AS_ASSERT(rd) )
		return False;

	LOCAL_DEBUG_OUT( "asw(%p)->magic(%lX)->icon_canvas(%p)->icon_title_canvas(%p)->icon_button(%p)->icon_title(%p)",
					 asw, asw->magic, asw->icon_canvas, asw->icon_title_canvas, asw->icon_button, asw->icon_title );
    if( asw->icon_canvas == NULL || rd->ib == NULL )
        return True;

    if( asw->icon_button )
    {
        width = calculate_astbar_width( asw->icon_button );
        height = calculate_astbar_height( asw->icon_button );
    }
    if( asw->icon_title )
    {
        title_width = calculate_astbar_width( asw->icon_title );
        title_height = calculate_astbar_height( asw->icon_title );
    }

    whole_width = (width == 0)?title_width:width ;
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

	LOCAL_DEBUG_OUT( "entering loop : areas_num = %d", rd->ib->areas_num );
    while( rd->curr_area < rd->ib->areas_num )
    {
        int new_x = -1, new_y = -1 ;

LOCAL_DEBUG_OUT( "trying area #%d : %s%s, %dx%d%+d%+d", rd->curr_area, get_flags(geom->flags, XNegative)?"XNeg":"XPos", get_flags(geom->flags, YNegative)?"YNeg":"YPos", geom->width, geom->height, geom->x, geom->y );
LOCAL_DEBUG_OUT( "last: %dx%d%+d%+d", rd->last_width, rd->last_height, rd->last_x, rd->last_y );

		if( get_flags(geom->flags, XNegative) )
            new_x = rd->last_x - rd->last_width - whole_width ;
        else
            new_x = rd->last_x + rd->last_width ;

        if( new_x < 0 || new_x+whole_width > geom->width )
        {  /* lets try and go to the next row and see if that makes a difference */
            if( get_flags(geom->flags, YNegative) )
                rd->last_y = rd->last_y - rd->last_height ;
            else
                rd->last_y = rd->last_y + rd->last_height ;
            rd->last_x = get_flags( geom->flags, XNegative )?geom->width-1: 0 ;

            if( get_flags(geom->flags, XNegative) )
                new_x = rd->last_x - whole_width ;
        }else
            rd->last_x = new_x ;

        new_y = rd->last_y ;
        if( get_flags(geom->flags, YNegative) )
            new_y = rd->last_y - whole_height ;

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
        moveresize_canvas( asw->icon_canvas, box_x+x, box_y+y, width, height );
        moveresize_canvas( asw->icon_title_canvas, box_x+x, box_y+y+height, width, height );
    }else
        moveresize_canvas( asw->icon_canvas, box_x+x, box_y+y, whole_width, whole_height );

    rd->last_width  = whole_width  ;
    if( whole_height > rd->last_height )
        rd->last_height = whole_height ;
    return True;
}

void
rearrange_iconbox( ASIconBox *ib )
{
    ASIconRearrangeAux aux_data ;

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

    /* TODO: we need to add this window to the list of icons,
     * and then place it in appropriate position : */
    ib = get_iconbox( ASWIN_DESK(asw) );
    append_bidirelem( ib->icons, asw );
    rearrange_iconbox( ib );
    return True;
}

Bool
remove_iconbox_icon( ASWindow *asw )
{
    ASIconBox *ib = NULL ;
    if( AS_ASSERT(asw) )
        return False;
    /* TODO: we need to remove this window from the list of icons */
    ib = get_iconbox( ASWIN_DESK(asw) );
    discard_bidirelem( ib->icons, asw );
    rearrange_iconbox( ib );
    return True;
}

Bool
change_iconbox_icon_desk( ASWindow *asw, int from_desk, int to_desk )
{
    return False;
}

void
on_icon_changed( ASWindow *asw )
{
    /* TODO : */
	/* if( AS_ASSERT(asw) ) */
        return;
    /* we probably need to reshuffle entire iconbox when that happen : */
    if( asw->icon_title )
    {
        int tbar_size = calculate_astbar_width( asw->icon_title );
        set_astbar_size( asw->icon_title, tbar_size, asw->icon_title?asw->icon_title->height:0 );
        render_astbar( asw->icon_title, asw->icon_title_canvas );
    }
    if( asw->icon_button )
    {


    }
    update_canvas_display( asw->icon_canvas );
    update_canvas_display( asw->icon_title_canvas );
}

void
rearrange_iconbox_icons( int desktop )
{
    rearrange_iconbox( get_iconbox( desktop ) );
}

