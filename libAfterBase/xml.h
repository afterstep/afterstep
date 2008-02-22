#ifndef ASXML_H_HEADER_FILE_INCLUDED
#define ASXML_H_HEADER_FILE_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

struct ASHashTable;

#define xml_tagchar(a) (isalnum(a) || (a) == '-' || (a) == '_')

#define XML_CDATA_STR 		"CDATA"
#define XML_CONTAINER_STR	"CONTAINER"
#define XML_CDATA_ID		-2
#define XML_CONTAINER_ID	-1
#define XML_UNKNOWN_ID		 0

#define IsCDATA(pe)    		((pe) && (pe)->tag_id == XML_CDATA_ID)
#define IsCONTAINER(pe)    	((pe) && (pe)->tag_id == XML_CONTAINER_ID)

typedef struct xml_elem_t {
	struct xml_elem_t* next;
	struct xml_elem_t* child;
	char* tag;
	int tag_id;
	char* parm;
} xml_elem_t;

typedef enum
{
	ASXML_Start 			= 0,			               
	ASXML_TagOpen 			= 1,
	ASXML_TagName 			= 2,
	ASXML_TagAttrOrClose 	= 3,
	ASXML_AttrName 			= 4,
	ASXML_AttrEq 			= 5,
	ASXML_AttrValueStart 	= 6,
	ASXML_AttrValue 		= 7,
	ASXML_AttrSlash 		= 8
} ASXML_ParserState;

typedef enum
{
	ASXML_BadStart = -1,
	ASXML_BadTagName = -2,
	ASXML_UnexpectedSlash = -3,
	ASXML_UnmatchedClose = -4,
	ASXML_BadAttrName = -5,
	ASXML_MissingAttrEq = -6
} ASXML_ParserError;

/* bb change : code moved to asimagexml.c
enum
{
	ASXML_Start 			= 0,			               
	ASXML_TagOpen 			= 1,
	ASXML_TagName 			= 2,
	ASXML_TagAttrOrClose 	= 3,
	ASXML_AttrName 			= 4,
	ASXML_AttrEq 			= 5,
	ASXML_AttrValueStart 	= 6,
	ASXML_AttrValue 		= 7,
	ASXML_AttrSlash 		= 8
} ASXML_ParserState;

enum
{
	ASXML_BadStart = -1,
	ASXML_BadTagName = -2,
	ASXML_UnexpectedSlash = -3,
	ASXML_UnmatchedClose = -4,
	ASXML_BadAttrName = -5,
	ASXML_MissingAttrEq = -6
} ASXML_ParserError;
*/

typedef struct ASXmlBuffer
{
	char *buffer ;
	int allocated, used, current ;

	int state ; 
	int level ;
	Bool verbatim;
	Bool quoted;
	
	enum
	{
		ASXML_OpeningTag = 0,
		ASXML_SimpleTag,
		ASXML_ClosingTag
	}tag_type ;

	int tags_count ;
}ASXmlBuffer;

#define IsTagCDATA(_tag)     ((_tag)!= NULL && (_tag)->tag_id == XML_CDATA_ID && (_tag)->parm != NULL)


void asxml_var_insert(const char* name, int value);
int asxml_var_get(const char* name);
int asxml_var_nget(char* name, int n);
void asxml_var_init(void);
void asxml_var_cleanup(void);

xml_elem_t* xml_elem_new(void);
xml_elem_t *create_CDATA_tag();
xml_elem_t *create_CONTAINER_tag();

xml_elem_t* xml_parse_parm(const char* parm, struct ASHashTable *vocabulary);
void xml_print(xml_elem_t* root);

void xml_elem_delete(xml_elem_t** list, xml_elem_t* elem);
xml_elem_t* xml_parse_doc(const char* str, struct ASHashTable *vocabulary);
int xml_parse(const char* str, xml_elem_t* current, struct ASHashTable *vocabulary);
void xml_insert(xml_elem_t* parent, xml_elem_t* child);
xml_elem_t *find_tag_by_id( xml_elem_t *chain, int id );

void reset_xml_buffer( ASXmlBuffer *xb );
void free_xml_buffer_resources (ASXmlBuffer *xb);
void add_xml_buffer_chars( ASXmlBuffer *xb, char *tmp, int len );
int spool_xml_tag( ASXmlBuffer *xb, char *tmp, int len );

/* reverse transformation - xml_elem into text. 
   If depth < 0 - no fancy formatting with identing will be added.
   If tags_count < 0 - all tags in the chain will be converted.
 */
Bool xml_tags2xml_buffer( xml_elem_t *tags, ASXmlBuffer *xb, int tags_count, int depth);

xml_elem_t *format_xml_buffer_state (ASXmlBuffer *xb);
char translate_special_sequence( const char *ptr, int len,  int *seq_len );
void append_cdata( xml_elem_t *cdata_tag, const char *line, int len );
void append_CDATA_line( xml_elem_t *tag, const char *line, int len );
char *interpret_ctrl_codes( char *text );




#ifdef __cplusplus
}
#endif

#endif /*#ifndef ASXML_H_HEADER_FILE_INCLUDED*/

