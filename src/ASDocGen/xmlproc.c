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

#include "ASDocGen.h"
#include "xmlproc.h"

#define TAG_INFO_AND_ID(tagname)	#tagname, DOCBOOK_##tagname##_ID

ASDocTagHandlingInfo SupportedDocBookTagInfo[DOCBOOK_SUPPORTED_IDS] = 
{
	{ TAG_INFO_AND_ID(unknown), NULL, NULL },
	{ TAG_INFO_AND_ID(id), NULL, NULL },
	{ TAG_INFO_AND_ID(arg), start_arg_tag, end_arg_tag },
	{ TAG_INFO_AND_ID(url), NULL, NULL },
	{ TAG_INFO_AND_ID(para), start_para_tag, end_para_tag },
	{ TAG_INFO_AND_ID(term), start_term_tag, end_term_tag },
	{ TAG_INFO_AND_ID(code), start_code_tag, end_code_tag },
	{ TAG_INFO_AND_ID(ulink), start_ulink_tag, end_ulink_tag },
	{ TAG_INFO_AND_ID(title), start_title_tag, end_title_tag },
	{ TAG_INFO_AND_ID(group), start_group_tag, end_group_tag },
	{ TAG_INFO_AND_ID(label), NULL, NULL },
	{ TAG_INFO_AND_ID(width), NULL, NULL },
	{ TAG_INFO_AND_ID(depth), NULL, NULL },
	{ TAG_INFO_AND_ID(align), NULL, NULL },
	{ TAG_INFO_AND_ID(anchor), start_anchor_tag, end_anchor_tag },
	{ TAG_INFO_AND_ID(option), start_option_tag, end_option_tag },
	{ TAG_INFO_AND_ID(choice), NULL, NULL },
	{ TAG_INFO_AND_ID(valign), NULL, NULL },
	{ TAG_INFO_AND_ID(command), start_command_tag, end_command_tag },
	{ TAG_INFO_AND_ID(example), start_example_tag, end_example_tag },
	{ TAG_INFO_AND_ID(linkend), NULL, NULL },
	{ TAG_INFO_AND_ID(section), start_section_tag, end_section_tag },
	{ TAG_INFO_AND_ID(fileref), NULL, NULL },
	{ TAG_INFO_AND_ID(refsect1), start_refsect1_tag, end_refsect1_tag },
	{ TAG_INFO_AND_ID(emphasis), start_emphasis_tag, end_emphasis_tag },
	{ TAG_INFO_AND_ID(listitem), start_listitem_tag, end_listitem_tag },
	{ TAG_INFO_AND_ID(imagedata), start_imagedata_tag, end_imagedata_tag },
	{ TAG_INFO_AND_ID(formalpara), start_formalpara_tag, end_formalpara_tag },	
	{ TAG_INFO_AND_ID(cmdsynopsis), start_cmdsynopsis_tag, end_cmdsynopsis_tag },
	{ TAG_INFO_AND_ID(replaceable), start_emphasis_tag, end_emphasis_tag },
	{ TAG_INFO_AND_ID(mediaobject), NULL, NULL },
	{ TAG_INFO_AND_ID(imageobject), NULL, NULL },
	{ TAG_INFO_AND_ID(variablelist), start_variablelist_tag, end_variablelist_tag },
	{ TAG_INFO_AND_ID(varlistentry), start_varlistentry_tag, end_varlistentry_tag },
	{ TAG_INFO_AND_ID(literallayout), start_literallayout_tag, end_literallayout_tag }
};	 

/*************************************************************************/
xml_elem_t* 
find_super_section( xml_elem_t* owner, const char *id )
{
	xml_elem_t* sub ;
	for( sub = owner->child ; sub != NULL ; sub = sub->next )	
	{
		Bool match_found = False ;
		xml_elem_t* attr, *attr_curr ;
		if( sub->tag_id != DOCBOOK_section_ID ) 
			continue;
		attr = xml_parse_parm(sub->parm, DocBookVocabulary);	 
		for( attr_curr = attr ; attr_curr ; attr_curr = attr_curr->next ) 
			if( attr_curr->tag_id == DOCBOOK_id_ID ) 
				break;
		match_found = ( attr_curr!= NULL && strncmp( attr_curr->parm, id, strlen(attr_curr->parm)) == 0 ) ;
		LOCAL_DEBUG_OUT( "xml_elem_delete for attr %p", attr );
		xml_elem_delete(NULL, attr);					
		
		if( match_found ) 
			return find_super_section( sub, id );
	}  	   
	return owner;
}	 
 


void
write_doc_cdata( const char *cdata, int len, ASXMLInterpreterState *state )
{
	int i ;
	if( state->doc_type == DocType_XML || 
		state->doc_type == DocType_HTML || 
		state->doc_type == DocType_PHP )	  
	{
		int token_start = 0;
		Bool special = False ;
		for( i = 0 ; i < len ; ++i ) 
		{
			if( (!isalnum(cdata[i]) && cdata[i] != '_'  && cdata[i] != '(' && cdata[i] != ')') || special )
			{
				if( token_start < i )
				{	
					if( get_flags( state->flags, ASXMLI_InsideLink) )
					{	
						fwrite( &(cdata[token_start]), 1, i-token_start, state->dest_fp );	 
					}else
					{
						/* need to try and insert an URL here if token is a keyword */
						char *token = mystrndup( &(cdata[token_start]), i - token_start );
						ASHashData hdata ;
						if( !special && get_hash_item(Links, AS_HASHABLE(token), &(hdata.vptr)) == ASH_Success ) 
						{
							if( state->doc_type == DocType_HTML	)
								fprintf( state->dest_fp, "<A href=\"%s\">%s</A>", hdata.cptr, token );
							else if( state->doc_type == DocType_PHP ) 
								fprintf( state->dest_fp, PHPXrefFormat, "visualdoc",token, hdata.cptr, "" );
						}else
					 		fwrite( token, 1, i-token_start, state->dest_fp );	
						free( token ); 
					}
				}	 
				token_start = i+1 ;
				
				if( cdata[i] == '&' )
					special = ( translate_special_sequence( &(cdata[i]), len-i,  NULL ) == '\0' );
				if( cdata[i] == ';' && special ) 		   
					special = False ;
				switch( cdata[i] )
				{
					case '<' : fwrite( "&lt;", 1, 4, state->dest_fp );     break ;	
					case '>' : fwrite( "&gt;", 1, 4, state->dest_fp );     break ;	 
					case '"' : fwrite( "&quot;", 1, 6, state->dest_fp );     break ;	 
					case '&' : 	if( !special ) 
								{			
									fwrite( "&amp;", 1, 5, state->dest_fp );     
									break ;	 
								}
								/* otherwise falling through ! */
					default:
						fputc( cdata[i], state->dest_fp );
				}	 
			}
		}				
		if( i > token_start ) 
			fwrite( &(cdata[token_start]), 1, i-token_start, state->dest_fp );	 
	}else
	{
		for( i = 0 ; i < len ; ++i ) 
		{
			int c_len = 0 ;
			int c = translate_special_sequence( &(cdata[i]), len-i, &c_len ) ;
			
			if( c != '\0' )
				i += c_len-1 ;	
			else
				c = cdata[i];
			
			fputc( c, state->dest_fp );
		}		   
	}	 
}
	

void 
convert_xml_tag( xml_elem_t *doc, xml_elem_t **rparm, ASXMLInterpreterState *state )
{
	xml_elem_t* parm = NULL;	
	xml_elem_t* ptr ;
	const char *tag_name = doc->tag;
	
	if( state->doc_type != DocType_XML ) 
	{
		parm = xml_parse_parm(doc->parm, DocBookVocabulary);	   
		if( doc->tag_id > 0 && doc->tag_id < DOCBOOK_SUPPORTED_IDS ) 
			if( SupportedDocBookTagInfo[doc->tag_id].handle_start_tag ) 
				SupportedDocBookTagInfo[doc->tag_id].handle_start_tag( doc, parm, state ); 
	}else
	{
		
		if( doc->tag_id == DOCBOOK_refsect1_ID ) 
			tag_name = SupportedDocBookTagInfo[DOCBOOK_section_ID].tag;

		fprintf( state->dest_fp, "<%s", tag_name ); 
		if( doc->parm ) 
		{
			char *ptr = NULL;
			if( doc->tag_id == DOCBOOK_ulink_ID )
				ptr = strchr( doc->parm, '#' );
			if( ptr != NULL )
			{
				*ptr = '\0' ;
				fprintf( state->dest_fp, " %s.html#%s", doc->parm, ptr+1 );	  
				*ptr = '#' ;
			}else		  
				fprintf( state->dest_fp, " %s", doc->parm );
		}
		fputc( '>', state->dest_fp );

	}	 
	for (ptr = doc->child ; ptr ; ptr = ptr->next) 
	{
		LOCAL_DEBUG_OUT( "handling tag's data \"%s\"", ptr->parm );
		if (ptr->tag_id == XML_CDATA_ID ) 
		{
			const char *data_ptr = ptr->parm ;
			int len = 0 ;
			if( state->doc_type == DocType_NROFF ) 
			{
				while( data_ptr[0] == '\n' ) 
					++data_ptr ;
			}	
			len = strlen( data_ptr ); 
			if( len > 0 && state->doc_type == DocType_NROFF) 
			{
				int i ;	  
				while( len > 0 && data_ptr[len-1] == '\n' ) 
					--len ;
				/* we want to skip as much whitespace as possible in NROFF */
				for( i = 0 ; i < len ; ++i ) 
					if( !isspace(data_ptr[i]) )
						break;
				if( i == len ) 
					len = 0 ;
				else if( !get_flags( state->flags, ASXMLI_LiteralLayout ) )
				{
					int from , to ;
					Bool written = 0 ;
					for( i = 0 ; i < len ; ++i ) 
					{
						while( isspace(data_ptr[i]) && i < len ) ++i ;
						from = i ; 
						while( !isspace(data_ptr[i]) && i < len ) ++i ;
						to = i ; 
						if( to > from  ) 
						{	
							if( written > 0 ) 
								fputc( ' ', state->dest_fp );
							write_doc_cdata( &data_ptr[from], to-from, state );
							written += to - from ;
						}
					}	 
					len = 0 ;
				}	 
			}
			if( len > 0 ) 
				write_doc_cdata( data_ptr, len, state );
		}else 
			convert_xml_tag( ptr, NULL, state );
	}
	LOCAL_DEBUG_OUT( "handling end tag with func %p", SupportedDocBookTagInfo[doc->tag_id].handle_end_tag );
	if( state->doc_type != DocType_XML ) 
	{
		if( doc->tag_id > 0 && doc->tag_id < DOCBOOK_SUPPORTED_IDS ) 
			if( SupportedDocBookTagInfo[doc->tag_id].handle_end_tag ) 
				SupportedDocBookTagInfo[doc->tag_id].handle_end_tag( doc, parm, state ); 
		LOCAL_DEBUG_OUT( "xml_elem_delete for parm %p", parm );
		if (rparm) *rparm = parm; 
		else xml_elem_delete(NULL, parm);
	}else
	{	
		fprintf( state->dest_fp, "</%s>", tag_name );
	}
	
}

Bool 
convert_xml_file( const char *syntax_dir, const char *file, ASXMLInterpreterState *state )
{
	char *source_file ;
	char *doc_str ; 
	Bool empty_file = False ;
	
	source_file = make_file_name( syntax_dir, file );
	doc_str = load_file(source_file);
	LOCAL_DEBUG_OUT( "file %s loaded into {%s}", source_file, doc_str );
	if( doc_str != NULL )
	{
		xml_elem_t* doc;
		xml_elem_t* ptr;
		
		if( file[0] == '_' && !get_flags( state->flags, ASXMLI_ProcessingOptions )) 
			state->pre_options_size += strlen(doc_str) ;
		else
			set_flags( state->flags, ASXMLI_ProcessingOptions );

		doc = xml_parse_doc(doc_str, DocBookVocabulary);
		LOCAL_DEBUG_OUT( "file %s parsed, child is %p", source_file, doc->child );
		if( doc->child ) 
		{
			LOCAL_DEBUG_OUT( "child tag = \"%s\", childs child = %p", doc->child->tag, doc->child->child);
			empty_file  = ( doc->child->tag_id == DOCBOOK_section_ID && 
							doc->child->child == NULL );
			if( doc->child->child ) 
			{
				empty_file  = ( doc->child->child->tag_id == XML_CDATA_ID && doc->child->child->next == NULL ); 
				LOCAL_DEBUG_OUT( "childs child tag = \"%s\", parm = \"%s\"", doc->child->child->tag, doc->child->child->parm);
			}	 
	   	}	 
		LOCAL_DEBUG_OUT( "file %s %s", source_file, empty_file?"empty":"not empty" );
		if( !empty_file )
		{	
			for (ptr = doc->child ; ptr ; ptr = ptr->next) 
			{
				LOCAL_DEBUG_OUT( "converting child <%s>", ptr->tag );
	  			convert_xml_tag( ptr, NULL, state );
				LOCAL_DEBUG_OUT( "done converting child <%s>", ptr->tag );
			}
		}
		/* Delete the xml. */
		LOCAL_DEBUG_OUT( "deleting xml %p", doc );
		xml_elem_delete(NULL, doc);
		LOCAL_DEBUG_OUT( "freeing doc_str %p", doc_str );
		free( doc_str );		
	}	 	   
	LOCAL_DEBUG_OUT( "done with %s", source_file );
	free( source_file );
	fprintf( state->dest_fp, "\n" );
	return !empty_file;
}

int 
check_xml_contents( const char *syntax_dir, const char *file )
{
	char *source_file ;
	char *doc_str ; 
	int size = 0 ;
	
	source_file = make_file_name( syntax_dir, file );
	doc_str = load_file(source_file);
	if( doc_str != NULL )
	{
		xml_elem_t* doc;
		size = strlen( doc_str );
		doc = xml_parse_doc(doc_str, DocBookVocabulary);
		if( doc->child ) 
		{
			if( doc->child->tag_id == DOCBOOK_section_ID && doc->child->child == NULL )
				size = 0 ;
			else if( doc->child->child ) 
			{
				if( doc->child->child->tag_id == XML_CDATA_ID && doc->child->child->next == NULL )
					size = 0;
			}	 
		}	 
		/* Delete the xml. */
		LOCAL_DEBUG_OUT( "xml_elem_delete for doc %p", doc );
		xml_elem_delete(NULL, doc);
		free( doc_str );		
	}	 	   
	free( source_file );
	return size;
}

/*************************************************************************/
void 
close_link( ASXMLInterpreterState *state )
{
	if( get_flags( state->flags, ASXMLI_InsideLink ) )
	{	
		if( state->doc_type == DocType_HTML || 
			(state->doc_type == DocType_PHP && !get_flags(state->flags, ASXMLI_LinkIsLocal)))
		{
			fwrite( "</A>", 1, 4, state->dest_fp );	   
		}else if( state->doc_type == DocType_PHP )
		{
			fprintf( state->dest_fp, "\",\"%s\",$srcunset,$subunset) ?>", state->curr_url_page);	   
			 							
		}	 
		clear_flags( state->flags, ASXMLI_InsideLink|ASXMLI_LinkIsLocal|ASXMLI_LinkIsURL );
		if( state->curr_url_page )
			free( state->curr_url_page );
		if( state->curr_url_anchor )
			free( state->curr_url_anchor );
		state->curr_url_anchor = state->curr_url_page = NULL ;
	}
}


void 
add_anchor( xml_elem_t *attr, ASXMLInterpreterState *state )
{
	while( attr ) 
	{
		if( attr->tag_id == DOCBOOK_id_ID ) 
		{
			close_link(state);
			if( state->doc_type == DocType_HTML || state->doc_type == DocType_PHP)
				fprintf( state->dest_fp, "\n<A NAME=\"%s\">", attr->parm );
			state->curr_url_anchor = mystrdup(attr->parm);
			clear_flags( state->flags, ASXMLI_LinkIsLocal|ASXMLI_LinkIsURL );
			set_flags( state->flags, ASXMLI_InsideLink );
			break;	
		}	 
		attr = attr->next ;	
	}	 
		   
}	 

void
add_local_link( xml_elem_t *attr, ASXMLInterpreterState *state )
{
	while( attr ) 
	{
		if( attr->tag_id == DOCBOOK_linkend_ID ||  attr->tag_id == DOCBOOK_url_ID ) 
		{
			char *ptr ;
			int l ;

			close_link(state);
			ptr = strchr( attr->parm, '#' );
			if( ptr != NULL ) 
			{
				*ptr = '\0' ;
				state->curr_url_page = mystrdup( attr->parm );
				state->curr_url_anchor = mystrdup( ptr+1 );
				*ptr = '#' ;
			}else
			{
				state->curr_url_page = mystrdup( attr->parm );	  
				state->curr_url_anchor = NULL ;
			}
			l = strlen(state->curr_url_page);
			clear_flags( state->flags, ASXMLI_LinkIsLocal );
			if( state->curr_url_page[l-1] != '/' && 
				( l < 5 || mystrcasecmp( &(state->curr_url_page[l-5]), ".html" ) != 0) && 
				( l < 4 || mystrcasecmp( &(state->curr_url_page[l-4]), ".php" ) != 0) &&
				( l < 4 || mystrcasecmp( &(state->curr_url_page[l-4]), ".htm" ) != 0) )
			{
				set_flags( state->flags, ASXMLI_LinkIsLocal );
			}

			if( state->doc_type == DocType_HTML || 
				(state->doc_type == DocType_PHP && !get_flags( state->flags, ASXMLI_LinkIsLocal ))) 
			{
				fprintf( state->dest_fp, "\n<A href=\"%s", state->curr_url_page );	   
				if( get_flags( state->flags, ASXMLI_LinkIsLocal ) ) 	  
					fwrite( ".html", 1, 5, state->dest_fp );	
				if( state->curr_url_anchor != NULL ) 
					fprintf( state->dest_fp, "#%s", state->curr_url_anchor );	
				fwrite( "\">", 1, 2, state->dest_fp );
			}else if( state->doc_type == DocType_PHP ) 
			{
				fprintf( state->dest_fp, "<? local_doc_url(\"visualdoc.php\",\"" );
			}
			set_flags( state->flags, ASXMLI_InsideLink|ASXMLI_LinkIsURL  );
			break;	
		}	 
		attr = attr->next ;	
	}	 
}	 

void
add_glossary_item( xml_elem_t* doc, ASXMLInterpreterState *state )
{	   
	xml_elem_t *cdata = find_tag_by_id( doc->child, XML_CDATA_ID );
	char *term_text = mystrdup(cdata?cdata->parm:"") ;
	char *orig_term_text = term_text ;
	int i ; 

	LOCAL_DEBUG_OUT( "term_text = \"%s\"", term_text );
	while(*term_text)
	{	
		if( isalnum(*term_text) )
			break;
		++term_text ;
	}
	i = 0 ;
	while( isalnum(term_text[i]) ) ++i ;
	term_text[i] = '\0' ;

	if( term_text[0] != '\0' ) /* need to add glossary term */
	{
	 	char *target = NULL, *target2 ;
		char *term = NULL, *term2 ;
		char *ptr = &(state->dest_file[strlen(state->dest_file)-4]);
		if( state->doc_type == DocType_PHP && *ptr == '.')
			*ptr = '\0' ;
		target = safemalloc( strlen( state->dest_file)+5+1+strlen(state->curr_url_anchor)+1);
		sprintf( target, "%s#%s", state->dest_file, state->curr_url_anchor );
		if( state->doc_type == DocType_PHP && *ptr == '\0' )
			*ptr = '.' ;
		
		target2 = mystrdup(target);
		term2 = mystrdup(term_text);
		if( add_hash_item( Links, AS_HASHABLE(term2), (void*)target2 ) != ASH_Success ) 
		{
			free( target2 );
			free( term2 );	   
		}	 
		term = safemalloc( strlen( term_text)+ 1 + 1 +strlen( state->doc_name ) + 1 +1 );
		sprintf( term, "%s (%s)", term_text, state->doc_name );
		LOCAL_DEBUG_OUT( "term = \"%s\"", term );				   
		if( add_hash_item( Glossary, AS_HASHABLE(term), (void*)target ) != ASH_Success ) 
		{
			free( target );
			free( term );	   
		}
	}	 
	
	free(orig_term_text);
}

/*************************************************************************/
/* DocBook XML tags handlers :											 */
/*************************************************************************/

void 
start_para_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP )
	{	
		close_link(state);
		fprintf( state->dest_fp, "<P class=\"dense\">" );	
	}else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\n" );
		 
}

void 
end_para_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	close_link(state);
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP )
	{
		fwrite( "</P>", 1, 4, state->dest_fp );	
	}else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\n" );
}

void 
start_formalpara_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	add_anchor( parm, state );
	set_flags( state->flags, ASXMLI_FormalPara );
}

void 
end_formalpara_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	close_link(state);
	clear_flags( state->flags, ASXMLI_FormalPara );
}

void 
start_command_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		fwrite( "<B>", 1, 3, state->dest_fp );	
	else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, " \\fB");
}

void 
end_command_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		fwrite( "</B>", 1, 4, state->dest_fp );	
	else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\\fP ");
}

void 
start_emphasis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		fwrite( "<I>", 1, 3, state->dest_fp );	
	else if( state->doc_type == DocType_NROFF && doc->tag_id != DOCBOOK_replaceable_ID)
		fprintf( state->dest_fp, " \\fI");
}

void 
end_emphasis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		fwrite( "</I>", 1, 4, state->dest_fp );	
	else if( state->doc_type == DocType_NROFF && doc->tag_id != DOCBOOK_replaceable_ID )
		fprintf( state->dest_fp, "\\fP ");
}

void 
start_literallayout_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
 	close_link(state);
	set_flags( state->flags, ASXMLI_LiteralLayout );
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP)
	{	
		fprintf( state->dest_fp, "<PRE>");	
	}else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\n.nf\n");
}

void 
end_literallayout_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	clear_flags( state->flags, ASXMLI_LiteralLayout );
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP)
	{	
		fwrite( "</PRE>", 1, 6, state->dest_fp );	  
	}else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\n.fi ");
}

void 
start_variablelist_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	close_link(state);
	if( state->doc_type == DocType_HTML || state->doc_type == DocType_PHP	)
		fprintf( state->dest_fp, "<DL class=\"dense\">" );	
}

void 
end_variablelist_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		fwrite( "</DL>", 1, 5, state->dest_fp );	
	else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\n");
}

void 
start_varlistentry_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		add_anchor( parm, state );
	else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\n.IP ");

}

void 
end_varlistentry_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )			
		close_link(state);
//	else if( state->doc_type == DocType_NROFF )
//		fprintf( state->dest_fp, "\n");
}

void 
start_term_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
#if 1
	if( state->doc_type == DocType_HTML || state->doc_type == DocType_PHP	 )
	{	
		if( get_flags( state->flags, ASXMLI_InsideLink ) && state->curr_url_anchor != NULL ) 
		{            
			add_glossary_item( doc, state ); 
			close_link(state);
		}	 
	}
#endif
	if( state->doc_type == DocType_HTML || state->doc_type == DocType_PHP	 )
		fprintf( state->dest_fp, "<DT class=\"dense\"><B>" );	
	else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\"");
}

void 
end_term_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML || state->doc_type == DocType_PHP	 )
		fwrite( "</B></DT>", 1, 9, state->dest_fp );	
	else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\"\n");
}	

void 
start_listitem_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 || state->doc_type == DocType_PHP	   )
		fprintf( state->dest_fp, "<DD class=\"dense\">" );	
}

void 
end_listitem_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	close_link(state);
	if( state->doc_type == DocType_HTML || state->doc_type == DocType_PHP	  	)
		fwrite( "</DD>", 1, 5, state->dest_fp );	
}

void 
start_imagedata_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 || state->doc_type == DocType_PHP	   )
	{	
		const char *url = NULL ;
		const char *align = NULL ;
		const char *valign = NULL ;
		const char *width = NULL ;
		const char *height = NULL ;
		while( parm ) 
		{	
			switch( parm->tag_id ) 
			{
				case DOCBOOK_width_ID : width = parm->parm ; break ;
				case DOCBOOK_depth_ID : height = parm->parm ; break ;
				case DOCBOOK_align_ID : align = parm->parm ; break ;
				case DOCBOOK_valign_ID : valign = parm->parm ; break ;	  
				case DOCBOOK_fileref_ID : url = parm->parm ; break ;	  
			}	 
			parm = parm->next ;
		}		

		fprintf( state->dest_fp, "<IMG src=\"%s\"", url );	
		if( align != NULL ) 
			fprintf( state->dest_fp, " align=\"%s\"", align );	 
		if( valign != NULL ) 
			fprintf( state->dest_fp, " valign=\"%s\"", valign );	 
		if( width != NULL ) 
			fprintf( state->dest_fp, " width=\"%s\"", width );	 
		if( height != NULL ) 
			fprintf( state->dest_fp, " height=\"%s\"", height );	 
		fwrite( ">", 1, 1, state->dest_fp );	  
	}
}

void 
end_imagedata_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	close_link(state);
	if( state->doc_type == DocType_HTML || state->doc_type == DocType_PHP	  	)
		fwrite( "</IMG>", 1, 6, state->dest_fp );	
}


void 
start_section_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	++(state->header_depth);	
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
	{
		/*if( state->doc_type == DocType_HTML ) 
			fwrite( "<HR>\n", 1, 5, state->dest_fp );
		 */
		add_anchor( parm, state );
		fwrite( "<UL>", 1, 4, state->dest_fp );
	}
}

void 
end_section_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	--(state->header_depth);	
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
	{
		fwrite( "</UL>", 1, 5, state->dest_fp );
	}
}

void 
start_refsect1_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	++(state->header_depth);	
	set_flags( state->flags, ASXMLI_RefSection );
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
	{
		add_anchor( parm, state );
		fwrite( "<LI>", 1, 4, state->dest_fp );
	}else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\n.SH ");
}

void 
end_refsect1_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	--(state->header_depth);	
	clear_flags( state->flags, ASXMLI_RefSection );
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
	{
			fwrite( "</LI>", 1, 5, state->dest_fp );
	}
}

void 
start_title_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
	{
		if( get_flags( state->flags, ASXMLI_FormalPara ) )
		{
			add_glossary_item( doc, state ); 	  
			close_link(state);
			fprintf( state->dest_fp, "<B>" );	
		}else if( get_flags( state->flags, ASXMLI_RefSection ) ) 
			fprintf( state->dest_fp, "<p class=\"refsect_header\"><B>" );	  
		else
			fprintf( state->dest_fp, "<p class=\"sect_header\"><B>" );	   
	}else if( state->doc_type == DocType_PHP )
	{	
		if( get_flags( state->flags, ASXMLI_FormalPara ) )
		{
			add_glossary_item( doc, state ); 	  
			close_link(state);
		}
		fprintf( state->dest_fp, "<B>");	   
	}
}

void 
end_title_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
	{	
		fprintf( state->dest_fp, "</B>" );	   
		if( !get_flags( state->flags, ASXMLI_FormalPara ) )
			fprintf( state->dest_fp, "</p>");	  
	}else if( state->doc_type == DocType_PHP )
	{	
		fprintf( state->dest_fp, "</B>");	
		if( !get_flags( state->flags, ASXMLI_FormalPara ) )
			fprintf( state->dest_fp, "<br>");	  
	}
	close_link(state);
}

void 
start_cmdsynopsis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	++(state->header_depth);	
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
	{
		add_anchor( parm, state );
		if( state->doc_type == DocType_PHP ) 
			fprintf( state->dest_fp, "<LI><b>SYNOPSIS</b><p>" );	   			  
		else
			fprintf( state->dest_fp, "<LI><h%d>SYNOPSIS</h%d>", state->header_depth, state->header_depth );	   			  
		close_link(state);
	}else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, ".SH SYNOPSIS\n");
}

void 
end_cmdsynopsis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	--(state->header_depth);	
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		fwrite( "</LI>", 1, 5, state->dest_fp );
	else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\n");
}

int 
check_choice( xml_elem_t *parm )
{
	while( parm ) 
	{	
		if( parm->tag_id == DOCBOOK_choice_ID ) 
		{
			if( mystrcasecmp( parm->parm, "opt" ) == 0 ) 
				return 1;
			break;	
		}	 
		parm = parm->next ;
	}		
	return 0;
}

void 
start_group_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	++(state->group_depth);	
	set_flags(state->flags, ASXMLI_FirstArg );
	if( state->doc_type == DocType_NROFF )
		fputc( ' ', state->dest_fp );
	if( check_choice( parm ) ) 
		fputc( '[', state->dest_fp );
}

void 
end_group_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	--(state->group_depth);	
	if( state->group_depth <= 0 ) 
		set_flags(state->flags, ASXMLI_FirstArg );
	if( check_choice( parm ) ) 
		fputc( ']', state->dest_fp );
	if( state->doc_type == DocType_NROFF )
		fputc( ' ', state->dest_fp );
}

void 
start_arg_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( !get_flags(state->flags, ASXMLI_FirstArg ) && state->group_depth > 0 )
		fwrite( "| ", 1, 2, state->dest_fp );
	clear_flags(state->flags, ASXMLI_FirstArg );
	if( state->doc_type == DocType_NROFF )
		fputc( ' ', state->dest_fp );
	if( check_choice( parm ) ) 
		fputc( '[', state->dest_fp );
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		fwrite( "<B>", 1, 3, state->dest_fp );
	else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\\fI");
}

void 
end_arg_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	--(state->group_depth);	
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP )
		fwrite( "</B>", 1, 4, state->dest_fp );
	else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\\fP");
	if( check_choice( parm ) ) 
		fputc( ']', state->dest_fp );
	if( state->doc_type == DocType_NROFF )
		fputc( ' ', state->dest_fp );
}

void 
start_option_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
		fwrite( "<B>", 1, 3, state->dest_fp );
}

void 
end_option_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
		fwrite( "</B>", 1, 4, state->dest_fp );
}

void
start_example_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	set_flags( state->flags, ASXMLI_InsideExample );
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
	{
		add_anchor( parm, state );
		close_link(state);
		fprintf( state->dest_fp, "<P class=\"dense\"> <B>Example : </B> " );	   			  
		fprintf( state->dest_fp, "<div class=\"container\">");
	}else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\nExample : ");

}

void 
end_example_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	clear_flags( state->flags, ASXMLI_InsideExample );
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
	{
		fprintf( state->dest_fp, "</div><br></p>");
	}
}

void
start_code_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
	{
		add_anchor( parm, state );
		close_link(state);
		fprintf( state->dest_fp, "<P class=\"dense\">" );	   			  
		fprintf( state->dest_fp, "<div class=\"container\">");
	}else if( state->doc_type == DocType_NROFF )
		fprintf( state->dest_fp, "\nSource : ");

}

void 
end_code_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
	{
		fprintf( state->dest_fp, "</div><br></p>");
	}
}


void 
start_ulink_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
	{
		add_local_link( parm, state );
	}	
	if( state->doc_type == DocType_NROFF )
		fputc( ' ', state->dest_fp );
}
	 
void 
end_ulink_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		close_link(state);
	if( state->doc_type == DocType_NROFF )
		fputc( ' ', state->dest_fp );
}	 

void 
start_anchor_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		add_anchor( parm, state );	
}
	
void 
end_anchor_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		close_link(state);
}

/*************************************************************************/
void 
compile_links( xml_elem_t *doc, ASXMLInterpreterState *state )	
{
	while( doc ) 
	{
		
		if( doc->child )
 			compile_links( doc->child, state );	 			 
		doc = doc->next ;	
	}		  
	
}
