#ifndef DOCFILE_H_HEADER_INCLUDED
#define DOCFILE_H_HEADER_INCLUDED

struct ASXMLInterpreterState;

void write_doc_header( struct ASXMLInterpreterState *state );
void write_options_header( struct ASXMLInterpreterState *state );
void write_options_footer( struct ASXMLInterpreterState *state );
void write_doc_footer( struct ASXMLInterpreterState *state );
Bool make_doc_dir( const char *name );
Bool start_doc_file(const char * dest_dir, const char *doc_path, const char *doc_postfix, ASDocType doc_type, 
                	const char *doc_name, const char *display_name, const char *display_purpose, 
					struct ASXMLInterpreterState *state, 
					ASFlagType doc_class_mask, ASDocClass doc_class );
void end_doc_file( struct ASXMLInterpreterState *state );

#endif

