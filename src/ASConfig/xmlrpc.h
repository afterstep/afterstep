#ifndef XMLRPC_H_HEADER_INCLUDED
#define XMLRPC_H_HEADER_INCLUDED

/* supported tags : 
 *
 * as_config_request == <command> prop1 [prop2]
 * prop == <prop prop_attributes>prop|value</prop>
 * prop_attributes == [id=%d|keyword=%d] [name="name"] [index="idx"] [sub_items=0|1] [read_only=0|1]
 * value == <value type=Phony|Integer|Data|File|Char [size=size]>hex_data</value>
 * 
 * as_config_replay == <success>|<failure>|<prop_list>|<value>
 * 
 * success == <success/>
 * failure == <failure/>
 * prop_list == <list>prop prop ... </list>
 * 
 * as_config_notice == <changed> prop </changed>
 * 
 * or in xml-rpc : 
 * 
 * commands :
 * 		list|new|get|set|copy|delete|save|load|order_down|order_up
 * 
 * request : 
 * <methodCall><methodName>command_name</methodName>
 * 		<params>
 *    		<param><value><struct>
 * 				[<member><name>PropertyPath</name>
 * 						<value><array><data>prop ...</data></array></value>
 *				</member>]
 * 				[<member><name>DestinationPath</name>
 * 						<value><array><data>prop ...</data></array></value>
 *				</member>]
 * 				[<member><name>Value</name>
 * 						<value>...</value>
 *				</member>]
 * 				[<member><name>Filename</name>
 * 						<value><string>filename</string></value>
 *				</member>]
 * 				[<member><name>Property</name>
 * 						prop
 *				</member>]
 * 			</struct></value></param>
 *  	</params>
 * </methodCall>
 * 
 * property_path :
 * 
 * prop :
 * <value><struct>
 * 		<member><name>id</name><value><int>property_id</int></value></member>
 * 		<member><name>keyword</name><value><string>property_keyword</string></value></member>
 * 		<member><name>name</name><value><string>property_name</string></value></member>
 * 		<member><name>index</name><value><int>property_index</int></value></member>
 * 		<member><name>order</name><value><int>order_no</int></value></member>
 * 		<member><name>sub_items</name><value><boolean>0|1</boolean></value></member>
 * 		<member><name>read_only</name><value><boolean>0|1</boolean></value></member>
 * </struct></value>
 * 
 * replay :
 * <methodResponse>
 *   	<params>
 *      	<param>
 * 				property_list | value
 *          </param>
 *      </params>
 * </methodResponse>
 * 
 * property_list :
 * <array>
 * 		<data>prop ...</data>
 * </array>
 * 
 * change notice : 
 * <methodCall><methodName>property changed</methodName>
 * 		<params>
 *    		<param>property_path</param>
 *  	</params>
 * </methodCall>
 * 
 * 
 * 
 * Here is the complete vocabulary :
 * i4
 * int
 * data
 * name
 * array
 * fault
 * param
 * value
 * base64
 * double
 * member
 * params
 * string
 * struct
 * boolean
 * methodCall
 * methodName
 * methodResponse
 * dateTime.iso8601
 * 
 * 
*/


typedef enum 
{
	XMLRPC_unknown_ID = 0,	  
	XMLRPC_i4_ID,
	XMLRPC_int_ID,
	XMLRPC_data_ID,
	XMLRPC_name_ID,
	XMLRPC_array_ID,
	XMLRPC_fault_ID,
	XMLRPC_param_ID,
	XMLRPC_value_ID,
	XMLRPC_base64_ID,
	XMLRPC_double_ID,
	XMLRPC_member_ID,
	XMLRPC_params_ID,
	XMLRPC_string_ID,
	XMLRPC_struct_ID,
	XMLRPC_boolean_ID,
	XMLRPC_methodCall_ID,
	XMLRPC_methodName_ID,
	XMLRPC_methodResponse_ID,
	XMLRPC_dateTime_iso8601_ID,
	
	XMLRPC_SUPPORTED_IDS

}SupportedXMLRPCTagIDs;

struct ASXmlRPCParam;
struct ASXmlRPCStruct;

typedef struct ASXmlRPCValue
{
	SupportedXMLRPCTagIDs type ;  /* i4, int, double, base64, string, boolean, struct, array */ 
	union
	{
		int		i;
		double	d;
		CARD8  	*b64;
		char 	*string;
		Bool    b;
		ASBiDirList 	*s ;   /* list of named values - members of the struct */
		ASBiDirList  	*a ;   /* list of values */
	
	}data ;
	
	int size ;
	char *name ;               /* optional - only if data member of the struct */

}ASXmlRPCValue;


typedef struct ASXmlRPCPacket
{
	Bool response ;
	const char *name ;
	ASBiDirList *params ;      /* set of ASXmlRPCValue structures for different params */
}ASXmlRPCPacket;

typedef struct ASXmlRPCState {
	ASXmlRPCPacket *curr_packet ;
	ASXmlRPCValue  *curr_val ;

	char *xml ;
	int  size, allocated_size ;
}ASXmlRPCState;

typedef void (*xmlrpc_tag_handler)( xml_elem_t *doc, xml_elem_t *parm, ASXmlRPCState *state );

typedef struct ASXmlRPCTagHandlingInfo
{
	char *tag ;
	int tag_id ;
	xmlrpc_tag_handler handle_start_tag ;
	xmlrpc_tag_handler handle_end_tag ;

}ASXmlRPCTagHandlingInfo;

void start__tag( xml_elem_t *doc, xml_elem_t *parm, ASXmlRPCState *state );
void end__tag( xml_elem_t *doc, xml_elem_t *parm, ASXmlRPCState *state );

extern ASXmlRPCTagHandlingInfo SupportedXmlRPCTagInfo[XMLRPC_SUPPORTED_IDS];

#endif /*XMLRPC_H_HEADER_INCLUDED */
