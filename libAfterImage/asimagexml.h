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

typedef struct xml_elem_t {
	struct xml_elem_t* next;
	struct xml_elem_t* child;
	char* tag;
	char* parm;
} xml_elem_t;

#define ASIM_XML_ENABLE_SAVE 	(0x01<<0)
#define ASIM_XML_ENABLE_SHOW 	(0x01<<1)

struct ASImageManager ;
struct ASFontManager ;

void set_xml_image_manager( struct ASImageManager *imman );
void set_xml_font_manager( struct ASFontManager *fontman );

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
xml_elem_t* xml_parse_parm(const char* parm);
void xml_print(xml_elem_t* root);
xml_elem_t* xml_elem_new(void);
xml_elem_t* xml_elem_remove(xml_elem_t** list, xml_elem_t* elem);
void xml_elem_delete(xml_elem_t** list, xml_elem_t* elem);
xml_elem_t* xml_parse_doc(const char* str);
int xml_parse(const char* str, xml_elem_t* current);
void xml_insert(xml_elem_t* parent, xml_elem_t* child);
char* lcstring(char* str);
Bool save_asimage_to_file(const char* file2bsaved, ASImage *im,
	    			      const char* strtype,
						  const char *compress,
						  const char *opacity,
			  			  int delay, int replace);

#ifdef __cplusplus
}
#endif

#endif /*#ifndef ASIMAGEXML_HEADER_FILE_INCLUDED*/


