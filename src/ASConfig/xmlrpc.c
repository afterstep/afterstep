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
#include <unistd.h>
#include "../../libAfterStep/parser.h"
#include "../../libAfterConf/afterconf.h"

#include "xmlproc.h"

#define TAG_INFO_AND_ID(tagname)	#tagname, XMLRPC_##tagname##_ID

ASXmlRPCTagHandlingInfo SupportedXmlRPCTagInfo[XMLRPC_SUPPORTED_IDS] = 
{
	{ TAG_INFO_AND_ID(unknown), 	NULL, NULL },
	{ TAG_INFO_AND_ID(id), 			NULL, NULL },
	{ TAG_INFO_AND_ID(i4), 			NULL, NULL },
	{ "int", XMLRPC_int_ID, 		NULL, NULL },
	{ TAG_INFO_AND_ID(data), 		NULL, NULL },
	{ TAG_INFO_AND_ID(name), 		NULL, NULL },
	{ TAG_INFO_AND_ID(array), 		NULL, NULL },
	{ TAG_INFO_AND_ID(fault), 		NULL, NULL },
	{ TAG_INFO_AND_ID(param), 		NULL, NULL },
	{ TAG_INFO_AND_ID(value), 		NULL, NULL },
	{ TAG_INFO_AND_ID(base64), 		NULL, NULL },
	{ "double", XMLRPC_double_ID, NULL, NULL },
	{ TAG_INFO_AND_ID(member), 		NULL, NULL },
	{ TAG_INFO_AND_ID(params), 		NULL, NULL },
	{ TAG_INFO_AND_ID(string), 		NULL, NULL },
	{ "struct", XMLRPC_struct_ID, NULL, NULL },
	{ TAG_INFO_AND_ID(boolean), 	NULL, NULL },
	{ TAG_INFO_AND_ID(methodCall), 	NULL, NULL },
	{ TAG_INFO_AND_ID(methodName), 	NULL, NULL },
	{ TAG_INFO_AND_ID(methodResponse), 					NULL, NULL },
	{ "dateTime.iso8601", XMLRPC_dateTime_iso8601_ID,	NULL, NULL },
	{ 0, NULL, NULL, NULL }
};	 

ASHashTable *XmlRPCVocabulary = NULL ;

/*************************************************************************/
void 
init_xml_rpc_vocabulary()
{
	if( XmlRPCVocabulary == NULL ) 
	{	
		XmlRPCVocabulary = create_ashash( 7, casestring_hash_value, casestring_compare, string_destroy_without_data );
		for( i = 1 ; i < XMLRPC_SUPPORTED_IDS ; ++i )
			add_hash_item( XmlRPCVocabulary, AS_HASHABLE(SupportedXmlRPCTagInfo[i].tag), (void*)(SupportedXmlRPCTagInfo[i].tag_id));
	}
}

void destroy_xml_rpc_value_func( void *data )
{
	ASXmlRPCValue *val = data ;
	if( val ) 
	{
		if( val->type == XMLRPC_array_ID || val->type == XMLRPC_struct_ID )
		{	
			if( val->value.members != NULL ) 
				destroy_asbidirlist( &(val->value.members) );
		}else if( val->value.cdata ) 
			free( val->value.cdata );
		if( val->name ) 
			free( val->name );
		free( val );
	}		 
}	 

ASXmlRPCValue *
create_xml_rpc_value( SupportedXMLRPCTagIDs	type, char *name ) 
{
	ASXmlRPCValue *val = safecalloc( 1, sizeof( ASXmlRPCValue ) );
	val->type = type ; 
	if( val->type == XMLRPC_array_ID || val->type == XMLRPC_struct_ID )
		val->value.members = create_asbidirlist(destroy_xml_rpc_value_func);
	if( name ) 
		val->name = mystrdup( name );
}

void
set_xml_rpc_val_cdata( ASXmlRPCValue *val, const char *cdata )
{
	if( val->type != XMLRPC_array_ID && val->type != XMLRPC_struct_ID )
	{	
		val->value.cdata = mystrdup(cdata);
		val->cdata_size = strlen(val->value.cdata);
	}
}
	  
/*************************************************************************/

static void 
convert_xml_rpc_tag( xml_elem_t *doc, xml_elem_t **rparm, ASXMLInterpreterState *state )
{
	xml_elem_t* parm = NULL;	
	xml_elem_t* ptr ;
	const char *tag_name = doc->tag;
	ASXmlRPCTagHandlingInfo *tag_info = NULL ;
	
	parm = xml_parse_parm(doc->parm, XmlRPCVocabulary);	   
	if( doc->tag_id > 0 && doc->tag_id < XMLRPC_SUPPORTED_IDS ) 
		tag_info = &(SupportedXmlRPCTagInfo[doc->tag_id]);

	if( tag_info && tag_info->handle_start_tag ) 
		tag_info->handle_start_tag( doc, parm, state ); 
	
	for (ptr = doc->child ; ptr ; ptr = ptr->next) 
	{
		LOCAL_DEBUG_OUT( "handling tag's data \"%s\"", ptr->parm );
		if (ptr->tag_id == XML_CDATA_ID ) 
		{
			const char *data_ptr = ptr->parm ;
			int len = strlen( data_ptr ); 
			if( state->last_tag_id == XMLRPC_methodName_ID ) 
			{	               /* packet's name  */
				set_string_value( &(state->curr_packet->name), mystrdup( ptr->parm ), NULL, 0 );
			}else if( state->last_tag_id == XMLRPC_name_ID ) 
			{                  /* value's name */
				set_string_value( &(state->curr_name), mystrdup( ptr->parm ), NULL, 0 );
			}else if( state->curr_val )
			{                  /* value's data */
				set_xml_rpc_val_cdata( state->curr_val, ptr->parm );
			}
		}else 
			convert_xml_tag( ptr, NULL, state );
	}
	
	
	if( tag_info && tag_info->handle_end_tag ) 
		tag_info->handle_end_tag( doc, parm, state ); 
	
	if( doc->tag_id > 0 && doc->tag_id < XMLRPC_SUPPORTED_IDS ) 
		state->last_tag_id = doc->tag_id ;
	   
	if (rparm) *rparm = parm; 
	else xml_elem_delete(NULL, parm);
}


Bool
xml2rpc_packet( ASXmlRPCPacket* packet )
{
   	xml_elem_t* doc;

   	
	if( packet == NULL || packet->xml == NULL ) 
		return False;

	doc = xml_parse_doc(packet->xml, XmlRPCVocabulary);
	if( doc->child ) 
	{			   	
		xml_elem_t* ptr;
		ASXmlRPCState state ;

		memset( &state, 0x00, sizeof(state));

		state->curr_packet = packet ; 
		if( packet->name )
		{	
			free( packet->name );
			packet->name = NULL  ;
		}
		if( packet->params )
			purge_asbidirlist( packet->params );
		else
			packet->params = create_asbidirlist(destroy_xml_rpc_value_func);


		LOCAL_DEBUG_OUT( "child is %p", doc->child );
		for (ptr = doc->child ; ptr ; ptr = ptr->next) 
		{
			LOCAL_DEBUG_OUT( "converting child <%s>", ptr->tag );
	  		convert_xml_rpc_tag( ptr, NULL, packet );
			LOCAL_DEBUG_OUT( "done converting child <%s>", ptr->tag );
		}
	}
	/* Delete the xml. */
	LOCAL_DEBUG_OUT( "deleting xml %p", doc );
	xml_elem_delete(NULL, doc);

}

Bool
rpc_packet2xml( ASXmlRPCPacket* packet )
{
	
	
}	 

/*************************************************************************/
void 
start__tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLRPCState *state )
{
}
	
void 
end__tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLRPCState *state )
{
}


