#ifndef XMLPROC_H_HEADER_INCLUDED
#define XMLPROC_H_HEADER_INCLUDED

typedef void (*tag_handler)( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

typedef struct ASDocTagHandlingInfo
{
	char *tag ;
	int tag_id ;
	tag_handler handle_start_tag ;
	tag_handler handle_end_tag ;

}ASDocTagHandlingInfo;

void start_group_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_group_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_arg_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_arg_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_option_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_option_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_para_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_para_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_formalpara_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_formalpara_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_section_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_section_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_refsect1_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_refsect1_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_anchor_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_anchor_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_title_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_title_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_ulink_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_ulink_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_emphasis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_emphasis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_cmdsynopsis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_cmdsynopsis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
	
void start_command_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_command_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_example_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_example_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_term_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_term_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_code_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_code_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_listitem_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_listitem_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_imagedata_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_imagedata_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_variablelist_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_variablelist_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_varlistentry_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_varlistentry_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_literallayout_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_literallayout_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void write_syntax_doc_header( SyntaxDef *syntax, ASXMLInterpreterState *state );
void write_syntax_options_header( SyntaxDef *syntax, ASXMLInterpreterState *state );
void write_syntax_options_footer( SyntaxDef *syntax, ASXMLInterpreterState *state );
void write_syntax_doc_footer( SyntaxDef *syntax, ASXMLInterpreterState *state );

/* supported DocBook tags : 
	<section label="overview" id="overview">
	<refsect1 id='usage'>
	<title>
	<para>
    <anchor>
	<command>
	<emphasis>
	<literallayout>
	<variablelist>
	<varlistentry>
	<term>
	<listitem>
	<option>
	<cmdsynopsis>
	<arg choice='opt'>
	<replaceable>
	<group choice='opt'>
	<ulink url="Bevel#synopsis">
	<example>
    <code>

	Here is the complete vocabulary :
id
arg
para
term
title
group
ulink
label
anchor
option
choice
command
example
linkend
section
refsect1
refsect1
emphasis
listitem
cmdsynopsis
replaceable
mediaobject
variablelist
varlistentry
literallayout
*/


typedef enum 
{
	DOCBOOK_unknown_ID = 0,	  
	DOCBOOK_id_ID,	  
	DOCBOOK_arg_ID,	  
	DOCBOOK_url_ID,	  
	DOCBOOK_para_ID,	  
	DOCBOOK_term_ID,	  
	DOCBOOK_code_ID,
	DOCBOOK_ulink_ID,	  
	DOCBOOK_title_ID,	  
	DOCBOOK_group_ID,	  
	DOCBOOK_label_ID,	  
	DOCBOOK_width_ID,	  
	DOCBOOK_depth_ID,
	DOCBOOK_align_ID,
	DOCBOOK_anchor_ID,
	DOCBOOK_option_ID,	  
	DOCBOOK_choice_ID,	  
	DOCBOOK_valign_ID,	  
	DOCBOOK_command_ID,	  
	DOCBOOK_example_ID,	  
	DOCBOOK_linkend_ID,	  
	DOCBOOK_section_ID,	  
	DOCBOOK_fileref_ID,	  
	DOCBOOK_refsect1_ID,	  
	DOCBOOK_refsect2_ID,	  
	DOCBOOK_emphasis_ID,	  
	DOCBOOK_listitem_ID,	  
	DOCBOOK_imagedata_ID,
	DOCBOOK_formalpara_ID,
	DOCBOOK_cmdsynopsis_ID,	  
	DOCBOOK_replaceable_ID,	  
	DOCBOOK_mediaobject_ID,
	DOCBOOK_imageobject_ID,
	DOCBOOK_variablelist_ID,	  
	DOCBOOK_varlistentry_ID,	  
	DOCBOOK_literallayout_ID,
	
	DOCBOOK_SUPPORTED_IDS

}SupportedDocBookXMLTagIDs;

extern ASDocTagHandlingInfo SupportedDocBookTagInfo[DOCBOOK_SUPPORTED_IDS];

void convert_xml_tag( xml_elem_t *doc, xml_elem_t **rparm, ASXMLInterpreterState *state );
Bool convert_xml_file( const char *syntax_dir, const char *file, ASXMLInterpreterState *state );
int check_xml_contents( const char *syntax_dir, const char *file );

void append_cdata( xml_elem_t *cdata_tag, const char *line, int len );
void append_CDATA_line( xml_elem_t *tag, const char *line, int len );
xml_elem_t* find_super_section( xml_elem_t* owner, const char *id );

#endif /*XMLPROC_H_HEADER_INCLUDED */
