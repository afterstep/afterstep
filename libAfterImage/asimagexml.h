#ifndef ASIMAGEXML_HEADER_FILE_INCLUDED
#define ASIMAGEXML_HEADER_FILE_INCLUDED

#include "asimage.h"

#ifdef __cplusplus
extern "C" {
#endif

#define xml_tagchar(a) (isalnum(a) || (a) == '-' || (a) == '_')

/* We don't trust the math library to actually provide this number.*/
#undef PI
#define PI 180

#define XML_CDATA_STR 		"CDATA"
#define XML_CONTAINER_STR	"CONTAINER"
#define XML_CDATA_ID		-2
#define XML_CONTAINER_ID	-1
#define XML_UNKNOWN_ID		 0

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

#define ASIM_XML_ENABLE_SAVE 	(0x01<<0)
#define ASIM_XML_ENABLE_SHOW 	(0x01<<1)

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

typedef struct 
{
	char *buffer ;
	int allocated, used ;

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




struct ASImageManager ;
struct ASFontManager ;

void set_xml_image_manager( struct ASImageManager *imman );
void set_xml_font_manager( struct ASFontManager *fontman );
struct ASImageManager *create_generic_imageman(const char *path);		   
struct ASFontManager *create_generic_fontman(Display *dpy, const char *path);

void asxml_var_insert(const char* name, int value);
int asxml_var_get(const char* name);

void asxml_var_init(void);
void asxml_var_cleanup(void);


ASImage *
compose_asimage_xml(ASVisual *asv,
                    struct ASImageManager *imman,
					struct ASFontManager *fontman,
					char *doc_str, ASFlagType flags,
					int verbose, Window display_win,
					const char *path);

void show_asimage(ASVisual *asv, ASImage* im, Window w, long delay);
ASImage* build_image_from_xml( ASVisual *asv,
                               struct ASImageManager *imman,
							   struct ASFontManager *fontman,
							   xml_elem_t* doc, xml_elem_t** rparm,
							   ASFlagType flags, int verbose, Window display_win);
double parse_math(const char* str, char** endptr, double size);
xml_elem_t* xml_parse_parm(const char* parm, ASHashTable *vocabulary);
void xml_print(xml_elem_t* root);
xml_elem_t* xml_elem_new(void);
xml_elem_t* xml_elem_remove(xml_elem_t** list, xml_elem_t* elem);
void xml_elem_delete(xml_elem_t** list, xml_elem_t* elem);
xml_elem_t* xml_parse_doc(const char* str, ASHashTable *vocabulary);
int xml_parse(const char* str, xml_elem_t* current, ASHashTable *vocabulary);
void xml_insert(xml_elem_t* parent, xml_elem_t* child);
xml_elem_t *find_tag_by_id( xml_elem_t *chain, int id );
xml_elem_t *create_CDATA_tag();


char* lcstring(char* str);
Bool save_asimage_to_file(const char* file2bsaved, ASImage *im,
	    			      const char* strtype,
						  const char *compress,
						  const char *opacity,
			  			  int delay, int replace);

void reset_xml_buffer( ASXmlBuffer *xb );
void add_xml_buffer_chars( ASXmlBuffer *xb, char *tmp, int len );
int spool_xml_tag( ASXmlBuffer *xb, char *tmp, int len );
char translate_special_sequence( const char *ptr, int len,  int *seq_len );
void append_cdata( xml_elem_t *cdata_tag, const char *line, int len );
void append_CDATA_line( xml_elem_t *tag, const char *line, int len );



#ifdef __cplusplus
}
#endif

#endif /*#ifndef ASIMAGEXML_HEADER_FILE_INCLUDED*/


