/*
 * This is the complete from scratch rewrite of the original WinList
 * module.
 *
 * Copyright (C) 2002 Sasha Vasko <sasha at aftercode.net>
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
 */

/*#define DO_CLOCKING      */
#define LOCAL_DEBUG

#include "../configure.h"

#include "asapp.h"
#include "afterstep.h"
#include "screen.h"
#include "module.h"
#include "mystyle.h"
#include "clientprops.h"
#include "wmprops.h"
#include "aswindata.h"
#include "decor.h"

/**********************************************************************/
/* Our data :                                        				  */
/**********************************************************************/
/* Storing window list as hash table hashed by client window ID :     */
ASHashTable *_as_Winlist = NULL;

static void
window_data_destroy( ASHashableValue val, void *data )
{
	ASWindowData *wd = data ;
	if( wd )
	{
		if( wd->magic == MAGIC_ASWindowData )
		{
			if( wd->window_name )
				free( wd->window_name );
			if( wd->icon_name )
				free( wd->icon_name );
			if( wd->res_class )
				free( wd->res_class );
			if( wd->res_name )
				free( wd->res_name );
            if( wd->canvas )
            {
                XDestroyWindow( dpy, wd->canvas->w );
                destroy_ascanvas( &(wd->canvas) );
            }
            if( wd->bar )
                destroy_astbar( &(wd->bar) );
			wd->magic = 0 ;
		}
		free( wd );
	}
}

void
destroy_window_data(ASWindowData *wd)
{
	if( wd == NULL ) return ;
	if( wd->magic == MAGIC_ASWindowData )
	{
		if( _as_Winlist )
			remove_hash_item( _as_Winlist, AS_HASHABLE(wd->client), NULL, True );
		else
			window_data_destroy( 0, wd );
	}else
		free( wd );
}

ASWindowData *
fetch_window_by_id( Window w )
{
	ASWindowData *wd = NULL ;
	if( _as_Winlist != NULL )
		if( get_hash_item( _as_Winlist, AS_HASHABLE(w), (void**)&wd ) != ASH_Success )
			wd = NULL ;
	return wd ;
}

ASWindowData *
add_window_data( ASWindowData *wd )
{
	if( wd == NULL )
		return NULL ;
	if( _as_Winlist == NULL )
		_as_Winlist = create_ashash( 7, NULL, NULL, window_data_destroy );
    show_activity( "adding window %X", wd->client );
	if( add_hash_item( _as_Winlist, AS_HASHABLE(wd->client), wd ) != ASH_Success )
	{
		window_data_destroy( 0, wd );
		wd = NULL ;
	}
	return wd ;
}

void
window_data_cleanup()
{
    if( _as_Winlist )
        destroy_ashash( &_as_Winlist );
}

WindowPacketResult
handle_window_packet(unsigned long type, unsigned long *data, ASWindowData **pdata)
{
	ASWindowData *wd;
	WindowPacketResult res = WP_Handled ;
	if( (type&WINDOW_PACKET_MASK)==0 || pdata == NULL )
		return WP_Error ;

	wd = *pdata ;
	if( wd == NULL )
	{
		if( (type&WINDOW_CONFIG_MASK) )
		{
			wd = safecalloc(1, sizeof(ASWindowData));
			wd->magic   = MAGIC_ASWindowData ;
			wd->client  = data[0] ;
			wd->frame   = data[1] ;
			wd->ref_ptr = data[2] ;
			if( (wd = add_window_data(wd)) == NULL )
				return WP_Error ;
		}else
			return WP_Error ;
	}else
	{
	    if( wd->magic   != MAGIC_ASWindowData ||
	        wd->client  != data[0] ||
		    wd->frame   != data[1] ||
		    wd->ref_ptr != data[2] )
			return WP_Error ;
		if( type == M_DESTROY_WINDOW )
		{
			destroy_window_data( wd );
			*pdata = NULL ;
			return WP_DataDeleted ;
		}else if( (type&WINDOW_STATE_MASK) )
		{
			if( type == M_FOCUS_CHANGE )
            {
                res = WP_DataChanged ;
                wd->focused = data[3] ;
            }
			return res ;
		}
	}
	if( (type&WINDOW_NAME_MASK) )
	{/* read in the name */
		char **dst = NULL ;
        unsigned long *pbuf = &(data[3]);
        char *new_name = deserialize_string( &pbuf, NULL );
LOCAL_DEBUG_OUT( "name received \"%s\"", new_name );
		switch(type)
		{
			case M_WINDOW_NAME : dst = &(wd->window_name); break;
			case M_ICON_NAME   : dst = &(wd->icon_name); break;
			case M_RES_CLASS   : dst = &(wd->res_class); break;
			case M_RES_NAME    : dst = &(wd->res_name ); break;
			default :
                free( new_name );
                return WP_Error ;
		}
		if( *dst )
		{
			if( strcmp(*dst, new_name ) == 0 )
            {
                free( new_name );
				return WP_Handled ;
            }
			free( *dst );
		}
        *dst = new_name ;
		return WP_DataChanged ;
	}

    /* otherwise we need to read full config */
#define CHECK_CHANGE_VAL(v,n,r) do{if((v)!=(n)){(r)=WP_DataChanged;(v)=(n);};}while(0)
	CHECK_CHANGE_VAL(wd->frame_rect.x		,data[3],res);
	CHECK_CHANGE_VAL(wd->frame_rect.y		,data[4],res);
	CHECK_CHANGE_VAL(wd->frame_rect.width	,data[5],res);
	CHECK_CHANGE_VAL(wd->frame_rect.height	,data[6],res);
	CHECK_CHANGE_VAL(wd->desk				,data[7],res);
    CHECK_CHANGE_VAL(wd->state_flags        ,data[8],res);
    CHECK_CHANGE_VAL(wd->flags              ,data[9],res);
    CHECK_CHANGE_VAL(wd->hints.base_width   ,data[10],res);
    CHECK_CHANGE_VAL(wd->hints.base_height  ,data[11],res);
    CHECK_CHANGE_VAL(wd->hints.width_inc    ,data[12],res);
    CHECK_CHANGE_VAL(wd->hints.height_inc   ,data[13],res);
    CHECK_CHANGE_VAL(wd->hints.min_width    ,data[14],res);
    CHECK_CHANGE_VAL(wd->hints.min_height   ,data[15],res);
    CHECK_CHANGE_VAL(wd->hints.max_width    ,data[16],res);
    CHECK_CHANGE_VAL(wd->hints.max_height   ,data[17],res);
    CHECK_CHANGE_VAL(wd->hints.win_gravity  ,data[18],res);
    CHECK_CHANGE_VAL(wd->icon_title         ,data[19],res);
    CHECK_CHANGE_VAL(wd->icon               ,data[20],res);
    CHECK_CHANGE_VAL(wd->icon_rect.x        ,data[21],res);
    CHECK_CHANGE_VAL(wd->icon_rect.y        ,data[22],res);
    CHECK_CHANGE_VAL(wd->icon_rect.width    ,data[23],res);
    CHECK_CHANGE_VAL(wd->icon_rect.height   ,data[24],res);

	/* returning result : */
	res = (*pdata != wd)?WP_DataCreated:res ;
	*pdata = wd ;
	return res ;
}

void
iterate_window_data( iter_list_data_handler iter_func, void *aux_data)
{
    if( iter_func && _as_Winlist )
    {
        ASHashIterator  i;
        if( start_hash_iteration (_as_Winlist, &i) )
            do
            {
                ASWindowData *data = curr_hash_data (&i);
                if( !iter_func( data, aux_data ) )
                    break;
            }while( next_hash_item( &i ));
    }
}

