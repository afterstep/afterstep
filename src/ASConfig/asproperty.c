/*
 * Copyright (c) 2004 Sasha Vasko <sasha@aftercode.net>
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

#define LOCAL_DEBUG
#define EVENT_TRACE

#include "../../configure.h"
#include "../../libAfterStep/asapp.h"
#include "../../libAfterStep/afterstep.h"
#include "../../libAfterStep/parser.h"
#include <unistd.h>
#include "../../libAfterConf/afterconf.h"

#include "configfile.h"
#include "asproperty.h"


/*************************************************************************/
static int ASProperty_CurrSeqNo = 0 ;
/*************************************************************************/
ASStorageID 
encode_string( const char *str ) 
{
	if( str == NULL ) 
		return 0;
	return store_data( NULL, (CARD8*)str, strlen(str)+1, ASStorage_RLEDiffCompress|ASStorage_NotTileable, 0);
}

int 
decode_string( ASStorageID id, char *buffer, int buffer_length, int *stored_length )
{
	int bytes_out = 0 ;

	if( stored_length )
		*stored_length = 0 ;

	if( id != 0 && buffer && buffer_length > 0 )
	{
		bytes_out = fetch_data(NULL, id, &buffer[0], 0, buffer_length-1, 0, stored_length);	
		buffer[bytes_out] = '\0' ;
	}
	return bytes_out;		
}	   

char*
decode_string_alloc( ASStorageID id )
{
	char tmp_buffer[1] ;
	int stored_length = 0 ;

	if( stored_length )
		stored_length = 0 ;

	if( id != 0 )
	{
		fetch_data(NULL, id, &tmp_buffer[0], 0, 1, 0, &stored_length);	
		if( stored_length > 0 ) 
		{
			char *str = safemalloc( stored_length );
		 	fetch_data(NULL, id, str, 0, stored_length, 0, &stored_length);	 		   
			return str;
		}	 
	}
	return NULL;		
}	   

/*************************************************************************/
static const char *_unknown = "unknown" ;
static const char *_wharf_button = "Item" ;
const char *
get_property_keyword( ASProperty *prop )
{
	if( prop && prop->id != 0 ) 
	{
		const char *kw ;
		if( prop->id == WHARF_Wharf_ID )
			return _wharf_button;
		if( (kw = keyword_id2keyword(prop->id)) != NULL )
			return kw;
	}	 
	return _unknown;
}		   
/*************************************************************************/
void destroy_property( void *data );

ASProperty *
create_property( int id, ASPropContentsType type, const char *name, Bool tree )
{
	ASProperty *prop = safecalloc( 1, sizeof(ASProperty));
	prop->seq_no = ++ASProperty_CurrSeqNo ;
	prop->id = id ;
	prop->type = type ;
	prop->name = mystrdup(name) ;
	prop->order = -1 ;
	if( tree ) 
		prop->sub_props = create_asbidirlist( destroy_property ) ;
	return prop;
}

inline void
append_property( ASProperty *owner, ASProperty *prop )
{
	if( owner && prop )
		append_bidirelem( owner->sub_props, prop );	
}	 

inline void
prepend_property( ASProperty *owner, ASProperty *prop )
{
	if( owner && prop )
		prepend_bidirelem( owner->sub_props, prop );	
}	 

ASProperty *
add_integer_property( int id, int val, ASProperty *owner )
{
	ASProperty *prop = create_property( id, ASProp_Integer, NULL, False );
	prop->contents.integer = val ;
	
	append_property( owner, prop );

	return prop;	 
}	 

ASProperty *
add_char_property( int id, char c, ASProperty *owner )
{
	ASProperty *prop = create_property( id, ASProp_Char, NULL, False );
	prop->contents.c = c ;
	
	append_property( owner, prop );

	return prop;	 
}	 

ASProperty *
add_string_property( int id, char *str, ASProperty *owner )
{
	ASProperty *prop = create_property( id, ASProp_Data, NULL, False );
	prop->contents.data = encode_string( str );
	
	append_property( owner, prop );

	return prop;	 
}	

void
set_property_index( ASProperty *prop, int index ) 
{
	if( prop ) 
	{	
		prop->index = index ; 	
		set_flags( prop->flags, ASProp_Indexed );
	}
}	 


void merge_property_list( ASProperty *src, ASProperty *dst );

void
dup_property_contents( ASProperty *src, ASProperty *dst, Bool dup_sub_props ) 
{
	if( src && dst )
	{	
		if( src->type == ASProp_Integer ) 
			dst->contents.integer = src->contents.integer ;
		else if( src->type == ASProp_Data ) 
			dst->contents.data = dup_data( NULL, src->contents.data );
		else if( src->type == ASProp_File )
			dst->contents.config_file = dup_config_file( src->contents.config_file );	

		if( dup_sub_props )
			if( src->sub_props != NULL ) 
				merge_property_list( src, dst );
	}
}

ASProperty *
dup_property( ASProperty *src, Bool dup_sub_props ) 
{
	ASProperty *dst	= NULL ;

	if( src )
	{
		dst = create_property( src->id, src->type, src->name, (src->sub_props != NULL) );
		dst->flags = src->flags ;
		dst->index = src->index ;
		dst->order = src->order ;
		dup_property_contents( src, dst, dup_sub_props ); 
	}
	
	return dst;	 
}
	   
void destroy_property( void *data )
{
	ASProperty *prop = (ASProperty*)data;

	if( prop )
	{	
		if( prop->name ) 
			free( prop->name );
		switch( prop->type ) 
		{
			case ASProp_Phony : break;
			case ASProp_Integer : break;
			case ASProp_Data : 
				forget_data(NULL, prop->contents.data); 
				break ;
			case ASProp_File : 
				destroy_config_file( prop->contents.config_file ); 
				break;
			case ASProp_Char : 
				break;
			default: break;
		}	 
	
		if( prop->sub_props ) 
			destroy_asbidirlist( &(prop->sub_props) ); 
		free( prop );
	}
}
/*************************************************************************/
ASProperty *
find_property_by_id( ASProperty *owner, int id )	 
{
	if( owner && owner->sub_props )
	{	
		ASBiDirElem *curr = LIST_START(owner->sub_props); 		
		while( curr ) 
		{
			ASProperty *prop = (ASProperty*)LISTELEM_DATA(curr) ;	  
			if( prop->id == id || id < 0  ) 
				return prop;
			LIST_GOTO_NEXT(curr);
		}
	}
	return NULL;
}

ASProperty *
find_property_by_id_name( ASProperty *owner, int id, const char *name )	 
{
	if( owner && owner->sub_props && name ) 
	{	
		ASBiDirElem *curr = LIST_START(owner->sub_props); 		
		while( curr ) 
		{
			ASProperty *prop = (ASProperty*)LISTELEM_DATA(curr) ;	  
			if( prop->id == id || id < 0 ) 
				if( prop->name && strcmp(prop->name , name ) == 0 )
					return prop;
			LIST_GOTO_NEXT(curr);
		}
	}
	return NULL;
}
/*************************************************************************/
void
remove_property_by_id( ASProperty *owner, int id )	 
{
	if( owner && owner->sub_props )
	{	
		ASBiDirElem *curr = LIST_START(owner->sub_props); 		   
		while( curr ) 
		{
			ASProperty *prop = (ASProperty*)LISTELEM_DATA(curr) ;	  
			if( prop->id == id ) 
			{	
				destroy_bidirelem( owner->sub_props, curr );
				return;
			}
			LIST_GOTO_NEXT(curr);
		}
	}
}

void
remove_property_by_id_name( ASProperty *owner, int id, const char *name )	 
{
	if( owner && owner->sub_props && name )
	{	
		ASBiDirElem *curr = LIST_START(owner->sub_props); 		
		while( curr ) 
		{
			ASProperty *prop = (ASProperty*)LISTELEM_DATA(curr) ;	  
			if( prop->id == id ) 
				if( prop->name && strcmp(prop->name , name ) == 0 )
				{
					destroy_bidirelem( owner->sub_props, curr );
					return ;
				}
			LIST_GOTO_NEXT(curr);
		}
	}
}


/*************************************************************************/

