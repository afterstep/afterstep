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

#include "../../configure.h"

#include <signal.h>

#ifdef I18N
#include <X11/Xlocale.h>
#endif

#define IN_MODULE
#define MODULE_X_INTERFACE

#include "../../include/afterbase.h"
#include "../../libAfterImage/afterimage.h"
#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/mystyle.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"
#include "../../include/clientprops.h"
#include "../../include/wmprops.h"

/**********************************************************************/
/* Our data :                                        				  */
/**********************************************************************/
/* Storing window list as hash table hashed by client window ID :     */

typedef struct ASWindowData
{
#define MAGIC_ASWindowData	0xA3817D08
	unsigned long magic ;
	Window 	client;
	Window 	frame ;
	
	unsigned long 	ref_ptr ; /* address of the related ASWindow structure in 
	                             afterstep's own memory space - we use it as a 
							     reference, to know what object change is related to */
	ASRectangle 	frame_rect ;
	int 			desk ;
	unsigned long 	flags ;
	int 			tbar_height ;
	int 			sbar_height ; /* misteriously called boundary_width in 
	                                 afterstep proper for no apparent reason */
	XSizeHints 		hints ; 	  /* not sure why we need it here */
	
	Window 			icon_title ;
	Window          icon ;									 
	
	ARGB32 			fore, back ;  /* really is kinda useless here */
	
	char 			*window_name ;
	char 			*icon_name ;
	char 			*res_class ;
	char 			*res_name ;
	
	ASRectangle      icon_rect ;
	
	Bool 			 focused ;
	Bool			 iconic ;
	
}ASWindowData;
/**********************************************************************/
/* w, frame, t and the rest of the full window config */
#define WINDOW_CONFIG_MASK (M_ADD_WINDOW|M_CONFIGURE_WINDOW)
/* w, frame, t, icon_p_x, icon_p_y, icon_p_width, icon_p_height :*/
#define WINDOW_ICON_MASK   (M_ICONIFY|M_DEICONIFY)
/* w, frame, t, and then text :*/
#define WINDOW_NAME_MASK   (M_WINDOW_NAME|M_ICON_NAME|M_RES_CLASS|M_RES_NAME)
/* w, frame and t */
#define WINDOW_STATE_MASK  (M_FOCUS_CHANGE|M_DESTROY_WINDOW)

#define WINDOW_PACKET_MASK (WINDOW_CONFIG_MASK|WINDOW_ICON_MASK| \
                            WINDOW_NAME_MASK|WINDOW_STATE_MASK)

typedef enum { 
	WP_Error = -1,
	WP_Handled,
	WP_DataCreated,
	WP_DataChanged,
	WP_DataDeleted
}WindowPacketResult ;

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
	}
	free( wd ); 
}

ASWindowData *
fetch_window_by_id( Window w )
{
	ASWindowData *wd = NULL ;
	if( _as_Winlist != NULL ) 
		if( get_hash_item( _as_Winlist, AS_HASHABLE(w), &wd ) != ASH_Success ) 
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
	show_progress( "adding window %X", wd->client );
	if( add_hash_item( _as_Winlist, AS_HASHABLE(wd->client), wd ) != ASH_Success )
	{
		window_data_destroy( 0, wd );
		wd = NULL ;
	}
	return wd ;
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
				if( !wd->focused )
				{
					res = WP_DataChanged ;
					wd->focused = True ;
				}
			return res ;
		}
	}
	if( (type&WINDOW_NAME_MASK) )
	{/* read in the name */
		char **dst = NULL ;
		char *new_name = (char*)&(data[3]);
		switch(type)
		{
			case M_WINDOW_NAME : dst = &(wd->window_name); break;
			case M_ICON_NAME   : dst = &(wd->icon_name); break;
			case M_RES_CLASS   : dst = &(wd->res_class); break;
			case M_RES_NAME    : dst = &(wd->res_name ); break;
			default : 
				return WP_Error ;
		} 
		if( *dst ) 
		{
			if( strcmp(*dst, new_name ) == 0 ) 
				return WP_Handled ;
			free( *dst );
		}
		*dst = mystrdup( new_name );
		return WP_DataChanged ;
	}	    

	if( (type&WINDOW_ICON_MASK) )
	{ /* read in the icon data */
		if( !wd->iconic && type!=M_ICONIFY )
			return WP_Handled ; /* redundand deiconify */

		if( (wd->iconic?1:0) != ((type == M_ICONIFY)?1:0) )
			res = WP_DataChanged ;
			
		wd->iconic = (type == M_ICONIFY);
		if( wd->iconic )
		{		
			if( data[3] == wd->icon_rect.x && 
				data[4] == wd->icon_rect.y && 
				data[5] == wd->icon_rect.width && 
				data[6] == wd->icon_rect.height )
				return res ;
			wd->icon_rect.x = data[3] ;
			wd->icon_rect.y = data[4] ;
			wd->icon_rect.width = data[5] ;
			wd->icon_rect.height = data[6] ;
		}
		return res ;
	}	    
	
	/* otherwise we need to read full config */
#define CHECK_CHANGE_VAL(v,n,r) do{if((v)!=(n)){(r)=WP_DataChanged;(v)=(n);};}while(0)	
	CHECK_CHANGE_VAL(wd->frame_rect.x		,data[3],res);
	CHECK_CHANGE_VAL(wd->frame_rect.y		,data[4],res); 
	CHECK_CHANGE_VAL(wd->frame_rect.width	,data[5],res); 
	CHECK_CHANGE_VAL(wd->frame_rect.height	,data[6],res);
	CHECK_CHANGE_VAL(wd->desk				,data[7],res); 
	CHECK_CHANGE_VAL(wd->flags				,data[8],res); 
	CHECK_CHANGE_VAL(wd->tbar_height		,data[9],res); 
	CHECK_CHANGE_VAL(wd->sbar_height		,data[10],res);
    CHECK_CHANGE_VAL(wd->hints.base_width	,data[11],res); 
	CHECK_CHANGE_VAL(wd->hints.base_height	,data[12],res); 
	CHECK_CHANGE_VAL(wd->hints.width_inc	,data[13],res);
	CHECK_CHANGE_VAL(wd->hints.height_inc	,data[14],res); 
	CHECK_CHANGE_VAL(wd->hints.min_width	,data[15],res); 
	CHECK_CHANGE_VAL(wd->hints.min_height	,data[16],res);
    CHECK_CHANGE_VAL(wd->hints.max_width	,data[17],res); 
	CHECK_CHANGE_VAL(wd->hints.max_height	,data[18],res); 
	CHECK_CHANGE_VAL(wd->icon_title			,data[19],res);
	CHECK_CHANGE_VAL(wd->icon				,data[20],res); 
	CHECK_CHANGE_VAL(wd->hints.win_gravity	,data[21],res); 
	CHECK_CHANGE_VAL(wd->fore				,data[22],res);
	CHECK_CHANGE_VAL(wd->back				,data[23],res);
	
	/* returning result : */
	res = (*pdata != wd)?WP_DataCreated:res ;
	*pdata = wd ;
	return res ;
}

