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
/*************************************************************************/

Bool
xml2rpc_packet( ASXmlRPCPacket* packet )
{
	
	
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


