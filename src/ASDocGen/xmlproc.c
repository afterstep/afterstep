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
	{ TAG_INFO_AND_ID(anchor), start_anchor_tag, end_anchor_tag },
	{ TAG_INFO_AND_ID(option), start_option_tag, end_option_tag },
	{ TAG_INFO_AND_ID(choice), NULL, NULL },
	{ TAG_INFO_AND_ID(command), start_command_tag, end_command_tag },
	{ TAG_INFO_AND_ID(example), start_example_tag, end_example_tag },
	{ TAG_INFO_AND_ID(linkend), NULL, NULL },
	{ TAG_INFO_AND_ID(section), start_section_tag, end_section_tag },
	{ TAG_INFO_AND_ID(refsect1), start_section_tag, end_section_tag },
	{ TAG_INFO_AND_ID(emphasis), start_emphasis_tag, end_emphasis_tag },
	{ TAG_INFO_AND_ID(listitem), start_listitem_tag, end_listitem_tag },
	{ TAG_INFO_AND_ID(cmdsynopsis), start_cmdsynopsis_tag, end_cmdsynopsis_tag },
	{ TAG_INFO_AND_ID(replaceable), start_emphasis_tag, end_emphasis_tag },
	{ TAG_INFO_AND_ID(variablelist), start_variablelist_tag, end_variablelist_tag },
	{ TAG_INFO_AND_ID(varlistentry), start_varlistentry_tag, end_varlistentry_tag },
	{ TAG_INFO_AND_ID(literallayout), start_literallayout_tag, end_literallayout_tag }
};	 

/*************************************************************************/
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
							fwrite( &data_ptr[from], 1, to-from, state->dest_fp );
							written += to - from ;
						}
					}	 
					len = 0 ;
				}	 
			}
			if( len > 0 ) 
				fwrite( data_ptr, 1, len, state->dest_fp );
		}else 
			convert_xml_tag( ptr, NULL, state );
	}
	if( state->doc_type != DocType_XML ) 
	{
		if( doc->tag_id > 0 && doc->tag_id < DOCBOOK_SUPPORTED_IDS ) 
			if( SupportedDocBookTagInfo[doc->tag_id].handle_end_tag ) 
				SupportedDocBookTagInfo[doc->tag_id].handle_end_tag( doc, parm, state ); 
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
				convert_xml_tag( ptr, NULL, state );
		}
		/* Delete the xml. */
		xml_elem_delete(NULL, doc);
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
	char *term_text = NULL ; 
#if 1
	if( state->doc_type == DocType_HTML || state->doc_type == DocType_PHP	 )
	{	
		if( get_flags( state->flags, ASXMLI_InsideLink ) && state->curr_url_anchor != NULL ) 
		{                                         
			xml_elem_t *ptr = doc->child ;
			while( ptr ) 
			{	
				if( ptr->tag_id == XML_CDATA_ID ) 
				{
					term_text = ptr->parm ;
					break;
				}
				ptr = ptr->next ;
			}
			if( term_text != NULL ) /* need to add glossary term */
			{
	 			char *target = NULL ;
				char *term = NULL ;
				char *ptr = &(state->dest_file[strlen(state->dest_file)-4]);
				if( state->doc_type == DocType_PHP && *ptr == '.')
					*ptr = '\0' ;
				target = safemalloc( strlen( state->dest_file)+5+1+strlen(state->curr_url_anchor)+1);
				sprintf( target, "%s#%s", state->dest_file, state->curr_url_anchor );
				if( state->doc_type == DocType_PHP && *ptr == '\0' )
					*ptr = '.' ;
				
				term = safemalloc( strlen( term_text)+ 1 + 1 +strlen( state->doc_name ) + 1 +1 );
				sprintf( term, "%s (%s)", term_text, state->doc_name );
			   	add_hash_item( Glossary, AS_HASHABLE(term), (void*)target );   
			}	 
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
start_section_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	++(state->header_depth);	
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
	{
		add_anchor( parm, state );
		if( doc->tag_id == DOCBOOK_section_ID ) 
			fwrite( "<UL>", 1, 4, state->dest_fp );
		else if( doc->tag_id == DOCBOOK_refsect1_ID ) 
			fwrite( "<LI>", 1, 4, state->dest_fp );
	}else if( state->doc_type == DocType_NROFF && doc->tag_id != DOCBOOK_section_ID )
		fprintf( state->dest_fp, "\n.SH ");
}

void 
end_section_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	--(state->header_depth);	
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
	{
		if( doc->tag_id == DOCBOOK_section_ID ) 
			fwrite( "</UL>", 1, 5, state->dest_fp );
		else if( doc->tag_id == DOCBOOK_refsect1_ID ) 
			fwrite( "</LI>", 1, 5, state->dest_fp );
	}
}

void 
start_title_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
		fprintf( state->dest_fp, "<h%d>", state->header_depth );	
	else if( state->doc_type == DocType_PHP )
		fprintf( state->dest_fp, "<B>");	
}

void 
end_title_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
	{	
		fprintf( state->dest_fp, "</h%d>", state->header_depth );	
	}else if( state->doc_type == DocType_PHP )
		fprintf( state->dest_fp, "</B><br>");	
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


