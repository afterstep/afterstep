#ifndef ASPROPERTY_H_HEADER_INCLUDED
#define ASPROPERTY_H_HEADER_INCLUDED

struct ASConfigFile;

/*************************************************************************/
typedef enum 
{
	ASProp_Phony = 0,
	ASProp_Integer,
	ASProp_Data,
	ASProp_File,
	ASProp_Char,
	ASProp_ContentsTypes		   
}ASPropContentsType;

typedef struct ASProperty {

#define ASProp_Indexed				(0x01<<0)	
#define ASProp_Merged				(0x01<<1)	  
#define ASProp_Disabled				(0x01<<2)	  
	ASFlagType flags ;
	
	ASStorageID id ;                 /* same a options IDs from autoconf.h */

	ASPropContentsType type ;
	char *name ;
	int index ;
	int order ;                /* sort order if  > -1 */

	union {
		int 		 integer ;
		ASStorageID  data;			
		struct ASConfigFile *config_file ;
		char 		 c ;
	}contents;
	
	ASBiDirList *sub_props ;	   

}ASProperty;

/*************************************************************************/

ASStorageID encode_string( const char *str );
int decode_string( ASStorageID id, char *buffer, int buffer_length, int *stored_length );
char* decode_string_alloc( ASStorageID id );

/*************************************************************************/
void destroy_property( void *data );
ASProperty *create_property( int id, ASPropContentsType type, const char *name, Bool tree );
void append_property( ASProperty *owner, ASProperty *prop );
void prepend_property( ASProperty *owner, ASProperty *prop );
ASProperty *add_integer_property( int id, int val, ASProperty *owner );
ASProperty *add_char_property( int id, char c, ASProperty *owner );
ASProperty *add_string_property( int id, char *str, ASProperty *owner );
void set_property_index( ASProperty *prop, int index );

void merge_property_list( ASProperty *src, ASProperty *dst );
void dup_property_contents( ASProperty *src, ASProperty *dst, Bool dup_sub_props );
ASProperty *dup_property( ASProperty *src, Bool dup_sub_props );
void destroy_property( void *data );

/*************************************************************************/
ASProperty *find_property_by_id( ASProperty *owner, int id );
ASProperty *find_property_by_id_name( ASProperty *owner, int id, const char *name );
/*************************************************************************/
void remove_property_by_id( ASProperty *owner, int id );
void remove_property_by_id_name( ASProperty *owner, int id, const char *name );


/*************************************************************************/
#endif /* ASPROPERTY_H_HEADER_INCLUDED */
