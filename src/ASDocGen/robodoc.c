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

#undef LOCAL_DEBUG
#define EVENT_TRACE

#include "../../configure.h"
#include "../../libAfterStep/asapp.h"
#include <unistd.h>
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/aswindata.h"
#include "../../libAfterStep/event.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterStep/parser.h"
#include "../../libAfterConf/afterconf.h"

#include "ASDocGen.h"
#include "xmlproc.h"
#include "docfile.h"
#include "robodoc.h"

void 
skip_robodoc_line( ASRobodocState *robo_state )
{
	register int curr = 0;
	register const char *ptr = &(robo_state->source[robo_state->curr]) ;
	LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d, ptr[curr] = 0x%x", robo_state->curr, robo_state->len, ptr[curr] );
	while( ptr[curr] && ptr[curr] != '\n' ) ++curr ;
	if( ptr[curr] || curr < robo_state->len ) 
		++curr ;
	LOCAL_DEBUG_OUT("robo_state> curr = %d(0x%x), len = %d, ptr[curr] = '%c'(0x%x)", robo_state->curr, robo_state->curr, robo_state->len, ptr[curr], ptr[curr] );
	robo_state->curr += curr ;
}	

void
find_robodoc_section( ASRobodocState *robo_state )
{
	while( robo_state->curr < (robo_state->len - 7) )
	{
		if( robo_state->source[robo_state->curr] == '/' ) 
		{
			int count = 0 ;
			const char *ptr = &(robo_state->source[++(robo_state->curr)]) ;

			while(ptr[count] == '*' || (count == 4 && isalpha(ptr[count])))
				++count ;
			if( count >= 6 && (ptr[count] == ' ' || get_flags(robo_state->flags, ASRS_InsideSection)))
				return ; 				       /* we found it ! */
		}
		skip_robodoc_line( robo_state );
	}	
	/* completed searching through the file */
	robo_state->curr = robo_state->len ;
}

/* supported remap types ( see ROBODOC docs ): 
 * c -- Header for a class.
 * d -- Header for a constant (from define).
 * f -- Header for a function.
 * h -- Header for a module in a project.
 * m -- Header for a method.
 * s -- Header for a structure.
 * t -- Header for a types.
 * u -- Header for a unittest.
 * v -- Header for a variable.
 * * -- Generic header for every thing else.
 */
/* supported subsection headers :
 * 	NAME -- Item name plus a short description.
 *  COPYRIGHT -- Who own the copyright : "(c) <year>-<year> by <company/person>"
 *  SYNOPSIS, USAGE -- How to use it.
 *  FUNCTION, DESCRIPTION, PURPOSE -- What does it do.
 *  AUTHOR -- Who wrote it.
 *  CREATION DATE -- When did the work start.
 *  MODIFICATION HISTORY, HISTORY -- Who has done which changes and when.
 *  INPUTS, ARGUMENTS, OPTIONS, PARAMETERS, SWITCHES -- What can we feed into it.
 *  OUTPUT, SIDE EFFECTS -- What output is made.
 *  RESULT, RETURN VALUE -- What do we get returned.
 *  EXAMPLE -- A clear example of the items use.
 *  NOTES -- Any annotations
 *  DIAGNOSTICS -- Diagnostic output
 *  WARNINGS, ERRORS -- Warning and error-messages.
 *  BUGS -- Known bugs.
 *  TODO, IDEAS -- What to implement next and ideas.
 *  PORTABILITY -- Where does it come from, where will it work.
 *  SEE ALSO -- References to other functions, man pages, other documentation.
 *  METHODS, NEW METHODS -- OOP methods.
 *  ATTRIBUTES, NEW ATTRIBUTES -- OOP attributes
 *  TAGS -- Tag-item description.
 *  COMMANDS -- Command description.
 *  DERIVED FROM -- OOP super class.
 *  DERIVED BY -- OOP sub class.
 *  USES, CHILDREN -- What modules are used by this one.
 *  USED BY, PARENTS -- Which modules do use this one.
 *  SOURCE -- Source code inclusion. 
 **/


ASDocTagHandlingInfo SupportedRoboDocTagInfo[ROBODOC_SUPPORTED_IDs+1] = 
{
 	{"NAME",			  				 ROBODOC_NAME_ID },
	{"COPYRIGHT",                        ROBODOC_COPYRIGHT_ID},
	{"SYNOPSIS",                         ROBODOC_SYNOPSIS_ID},
 	{"USAGE",                            ROBODOC_USAGE_ID},
 	{"FUNCTION",                         ROBODOC_FUNCTION_ID},
	{"DESCRIPTION",                      ROBODOC_DESCRIPTION_ID},
	{"PURPOSE",                          ROBODOC_PURPOSE_ID},
	{"AUTHOR",                           ROBODOC_AUTHOR_ID},
	{"CREATION DATE",                    ROBODOC_CREATION_DATE_ID},
	{"MODIFICATION HISTORY",             ROBODOC_MODIFICATION_HISTORY_ID},
	{"HISTORY",                          ROBODOC_HISTORY_ID},
 	{"INPUTS",                           ROBODOC_INPUTS_ID},
	{"ARGUMENTS",                        ROBODOC_ARGUMENTS_ID},
	{"OPTIONS",                          ROBODOC_OPTIONS_ID},
	{"PARAMETERS",                       ROBODOC_PARAMETERS_ID},
	{"SWITCHES",                         ROBODOC_SWITCHES_ID},
 	{"OUTPUT",                           ROBODOC_OUTPUT_ID},
	{"SIDE EFFECTS",                     ROBODOC_SIDE_EFFECTS_ID},
 	{"RESULT",                           ROBODOC_RESULT_ID},
	{"RETURN VALUE",                     ROBODOC_RETURN_VALUE_ID},
 	{"EXAMPLE",                          ROBODOC_EXAMPLE_ID},
 	{"NOTES",                            ROBODOC_NOTES_ID},
 	{"DIAGNOSTICS",                      ROBODOC_DIAGNOSTICS_ID},
 	{"WARNINGS",                         ROBODOC_WARNINGS_ID},
	{"ERRORS",                           ROBODOC_ERRORS_ID},
 	{"BUGS",                             ROBODOC_BUGS_ID},
 	{"TODO",                             ROBODOC_TODO_ID},
	{"IDEAS",                            ROBODOC_IDEAS_ID},
 	{"PORTABILITY",                      ROBODOC_PORTABILITY_ID},
 	{"SEE ALSO",                         ROBODOC_SEE_ALSO_ID},
 	{"METHODS",                          ROBODOC_METHODS_ID},
	{"NEW METHODS",                      ROBODOC_NEW_METHODS_ID},
 	{"ATTRIBUTES",                       ROBODOC_ATTRIBUTES_ID},
	{"NEW ATTRIBUTES",                   ROBODOC_NEW_ATTRIBUTES_ID},
 	{"TAGS",                             ROBODOC_TAGS_ID},
 	{"COMMANDS",                         ROBODOC_COMMANDS_ID},
 	{"DERIVED FROM",                     ROBODOC_DERIVED_FROM_ID},
 	{"DERIVED BY",                       ROBODOC_DERIVED_BY_ID},
 	{"USES",                             ROBODOC_USES_ID},
	{"CHILDREN",                         ROBODOC_CHILDREN_ID},
 	{"USED BY",                          ROBODOC_USED_BY_ID},
	{"PARENTS",                          ROBODOC_PARENTS_ID},
 	{"SOURCE",                           ROBODOC_SOURCE_ID},
    {NULL, 								ROBODOC_SUPPORTED_IDs}                 
};

xml_elem_t*
robodoc_section_header2xml( ASRobodocState *robo_state )
{
	xml_elem_t* section = xml_elem_new();
	xml_elem_t* owner = robo_state->doc ;
	xml_elem_t* title = NULL ;
	const char *ptr = &(robo_state->source[robo_state->curr]) ;
	int i = 0 ;
	int id_length = 0 ;
	char *id = "";
	char saved = ' ';
	
	LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d", robo_state->curr, robo_state->len );
		   
	section->tag = mystrdup( "section" );
	section->tag_id = DOCBOOK_section_ID ;  
	
	while( ptr[i] != 0 && ptr[i] != '\n' ) ++i ;
	robo_state->curr += i+1 ;
	while( i > 0 && isspace(ptr[i])) --i ;
	if( i > 6 ) 
	{	
		id_length = i ;
		i = 6 ; 
		while( i < id_length && isspace(ptr[i]) ) ++i; 	
		if( i < id_length ) 
		{
			id = (char*)(&ptr[i]);
			id_length += 1-i ;	 
			saved = id[id_length];
			id[id_length] = '\0' ;
		}else
			id_length = 0 ;	 
	}else                                      /* not a valid section header */
		return NULL ;

	section->parm = safemalloc( 5+2+1+2+2+2+id_length+1+1 );
	sprintf(section->parm, "remap=\"%c\" id=\"%s\"", ptr[4], id );
	

	owner = find_super_section( owner, id );
	xml_insert(owner, section);
	
	
	robo_state->curr_section = section ;
	robo_state->curr_subsection = NULL ;	
	
	
	if( id_length > 0 ) 
	{
		title = xml_elem_new();	  
		title->tag = mystrdup( "title" );
		title->tag_id = DOCBOOK_title_ID ;  
		xml_insert(section, title);
		
		append_CDATA_line( title, id, id_length );
		id[id_length] = saved ;
	}
	
	return section;		   
}

xml_elem_t*
handle_varlist_line( ASRobodocState *robo_state, xml_elem_t* sec_tag, int len, int *skipped )
{
	xml_elem_t* ll_tag = NULL ;
	xml_elem_t* varlist_tag = NULL;		
	xml_elem_t* tmp ;
	const char *ptr = &(robo_state->source[robo_state->curr]);
	Bool is_term = !isspace(ptr[0]);

	*skipped = 0 ;

	for(tmp = sec_tag->child; tmp != NULL ; tmp = tmp->next ) 
		if( tmp->next == NULL && tmp->tag_id == DOCBOOK_variablelist_ID ) 
			varlist_tag = tmp ;
	if( varlist_tag != NULL || is_term )
	{	
		xml_elem_t* varentry_tag = NULL;		   
		xml_elem_t* item_tag = NULL;		   
		if( varlist_tag == NULL ) 
		{
			varlist_tag = xml_elem_new();
			varlist_tag->tag = mystrdup("variablelist") ;
			varlist_tag->tag_id = DOCBOOK_variablelist_ID ;
			xml_insert(sec_tag, varlist_tag);
		}	 
		if( !is_term )
		{	
			for(tmp = varlist_tag->child; tmp != NULL ; tmp = tmp->next ) 
				if( tmp->next == NULL && tmp->tag_id == DOCBOOK_varlistentry_ID ) 
					varentry_tag = tmp ;
		}
		if( varentry_tag == NULL ) 
		{
			xml_elem_t* term_tag = NULL;		   
			char *ptr_body, *ptr_term ;
			varentry_tag = xml_elem_new();
			varentry_tag->tag = mystrdup("varlistentry") ;
			varentry_tag->tag_id = DOCBOOK_varlistentry_ID ;
			xml_insert(varlist_tag, varentry_tag);
				
			term_tag = xml_elem_new();
			term_tag->tag = mystrdup("term") ;
			term_tag->tag_id = DOCBOOK_term_ID ;
			xml_insert(varentry_tag, term_tag);

			ptr_term = tokencpy (ptr);
			append_CDATA_line( term_tag, ptr_term, strlen(ptr_term) ); 
			free( ptr_term );

			ptr_body = tokenskip( (char*)ptr, 1 );
			if( ptr_body - ptr  > len ) 
				*skipped = len ;	  
			else
				*skipped = ptr_body - ptr ;
		}	
		for(tmp = varentry_tag->child; tmp != NULL ; tmp = tmp->next ) 
			if( tmp->next == NULL && tmp->tag_id == DOCBOOK_listitem_ID ) 
				item_tag = tmp ;
		if( item_tag == NULL ) 
		{
			item_tag = xml_elem_new();
			item_tag->tag = mystrdup("listitem") ;
			item_tag->tag_id = DOCBOOK_listitem_ID ;
			xml_insert(varentry_tag, item_tag);
		}	 
		ll_tag = item_tag ;
	}
	return ll_tag;
}

xml_elem_t*
handle_formalpara_line( ASRobodocState *robo_state, xml_elem_t* sec_tag, int len, int *skipped )
{
	xml_elem_t* ll_tag = NULL ;
	xml_elem_t* fp_tag = NULL;		
	xml_elem_t* tmp ;
	const char *ptr = &(robo_state->source[robo_state->curr]);
	Bool is_term = !isspace(ptr[0]);

	*skipped = 0 ;

   	if( get_flags( robo_state->flags, ASRS_TitleAdded ) )	
		for(tmp = sec_tag->child; tmp != NULL ; tmp = tmp->next ) 
			if( tmp->next == NULL && tmp->tag_id == DOCBOOK_formalpara_ID ) 
				fp_tag = tmp ;
	if( fp_tag != NULL || is_term )
	{	
		xml_elem_t* title_tag = NULL;		   
		xml_elem_t* para_tag = NULL;		   

		if( fp_tag == NULL ) 
		{
			fp_tag = xml_elem_new();
			fp_tag->tag = mystrdup("formalpara") ;
			fp_tag->tag_id = DOCBOOK_formalpara_ID ;
			xml_insert(sec_tag, fp_tag);
		}	 
		if( is_term )
		{	
			title_tag = find_tag_by_id(fp_tag->child, DOCBOOK_title_ID );
			
	 		if( title_tag == NULL ) 
			{
				char *ptr_title ;
				char *ptr_body ;
				title_tag = xml_elem_new();
				title_tag->tag = mystrdup("title") ;
				title_tag->tag_id = DOCBOOK_title_ID ;
				xml_insert(fp_tag, title_tag);
				
				ptr_title = tokencpy (ptr);
				append_CDATA_line( title_tag, ptr_title, strlen(ptr_title) ); 
				fp_tag->parm = safemalloc( 4+strlen(ptr_title)+1+1 );
				sprintf( fp_tag->parm, "id=\"%s\"", ptr_title );
				free( ptr_title );

				ptr_body = tokenskip( (char*)ptr, 1 );
				if( ptr_body - ptr  > len ) 
					*skipped = len ;	  
				else 
					*skipped = ptr_body - ptr ;

				set_flags( robo_state->flags, ASRS_TitleAdded );
			}	 
		}
		for(tmp = fp_tag->child; tmp != NULL ; tmp = tmp->next ) 
			if( tmp->next == NULL && tmp->tag_id == DOCBOOK_para_ID ) 
				para_tag = tmp ;

		if( para_tag == NULL ) 
		{			   
			para_tag = xml_elem_new();
			para_tag->tag = mystrdup("para") ;
			para_tag->tag_id = DOCBOOK_para_ID ;
			xml_insert(fp_tag, para_tag);
		}	
		ll_tag = para_tag ;
	}
	return ll_tag;
}


void 
append_robodoc_line( ASRobodocState *robo_state, int len )
{
	xml_elem_t* sec_tag = NULL;
	xml_elem_t* ll_tag = NULL;
	const char *ptr = &(robo_state->source[robo_state->curr]);
	int skipped = 0 ;

	LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d", robo_state->curr, robo_state->len );
	if( robo_state->curr_subsection == NULL ) 
		sec_tag = robo_state->curr_section ;
	else
		sec_tag = robo_state->curr_subsection ;

	if( robo_state->curr_subsection != NULL )
	{	
		if( get_flags( robo_state->flags, ASRS_FormalPara )) 
			ll_tag = handle_formalpara_line( robo_state, sec_tag, len, &skipped );
		else if( get_flags( robo_state->flags, ASRS_VarlistSubsection )) 
			ll_tag = handle_varlist_line( robo_state, sec_tag, len, &skipped );
		ptr += skipped ; 
		len -= skipped ;
	}

	if( ll_tag == NULL )
	{
		xml_elem_t *tmp = sec_tag->child;
		do
		{
	   		tmp = find_tag_by_id(tmp, DOCBOOK_literallayout_ID ); 
			if( tmp ) 
			{	
				ll_tag = tmp ;
				tmp = tmp->next ;
			}
		}while( tmp != NULL );
	}			
	if( ll_tag == NULL )
	{
		ll_tag = xml_elem_new();
		ll_tag->tag = mystrdup("literallayout") ;
		ll_tag->tag_id = DOCBOOK_literallayout_ID ;
		xml_insert(sec_tag, ll_tag);
	}	 
	append_CDATA_line( ll_tag, ptr, len );
}

Bool
handle_robodoc_subtitle( ASRobodocState *robo_state, int len )
{
	const char *ptr = &(robo_state->source[robo_state->curr]);
	int i = 0; 
	int robodoc_id = -1 ;

	LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d", robo_state->curr, robo_state->len );
	while( isspace(ptr[i]) && i < len ) ++i ;
	while( isspace(ptr[len]) && i < len ) --len ;
	++len ;
	LOCAL_DEBUG_OUT("i = %d, len = %d", i, len );
	ptr = &(ptr[i]);
	len -= i ;
	if( len < 0 )
		return False;
	
	for( i = 0 ; i < ROBODOC_SUPPORTED_IDs ; ++i ) 
	 	if( strlen( SupportedRoboDocTagInfo[i].tag ) == len ) 		  
			if( strncmp( ptr, SupportedRoboDocTagInfo[i].tag, len ) == 0 ) 
			{
				robodoc_id = SupportedRoboDocTagInfo[i].tag_id ; 
				LOCAL_DEBUG_OUT("subtitle found ; \"%s\"", SupportedRoboDocTagInfo[i].tag );
				break;
			}	 
	if( robodoc_id < 0 ) 
		return False ;
	/* otherwise we have to create subsection */
	LOCAL_DEBUG_OUT( "curr_subsection = %p, robodoc_id = %d, last_robodoc_id = %d", 
					 robo_state->curr_subsection, robodoc_id, robo_state->last_robodoc_id );
	if( robo_state->last_robodoc_id != robodoc_id || robo_state->curr_subsection == NULL ) 
	{
		xml_elem_t* sec_tag = NULL;
		xml_elem_t* title_tag = NULL;
		xml_elem_t* title_text_tag = NULL;
		
		sec_tag = xml_elem_new();
		sec_tag->tag = mystrdup("refsect1") ;
		sec_tag->tag_id = DOCBOOK_refsect1_ID ;
		xml_insert(robo_state->curr_section, sec_tag);
		robo_state->curr_subsection = sec_tag ;
		robo_state->last_robodoc_id = robodoc_id ;
		
		title_tag = xml_elem_new();
		title_tag->tag = mystrdup("title") ;
		title_tag->tag_id = DOCBOOK_title_ID ;
		xml_insert(sec_tag, title_tag);
		
		title_text_tag = create_CDATA_tag();
		title_text_tag->parm = mystrdup(SupportedRoboDocTagInfo[i].tag);
		LOCAL_DEBUG_OUT("subtitle title = >%s<", title_text_tag->parm );
		xml_insert(title_tag, title_text_tag);
	
	}
	
	clear_flags( robo_state->flags, ASRS_VarlistSubsection|ASRS_FormalPara );
	clear_flags( robo_state->flags, ASRS_TitleAdded );
	
	if( robodoc_id == ROBODOC_ATTRIBUTES_ID || 
		robodoc_id == ROBODOC_NEW_ATTRIBUTES_ID ||
 		robodoc_id == ROBODOC_INPUTS_ID ||
		robodoc_id == ROBODOC_ARGUMENTS_ID ||
		robodoc_id == ROBODOC_OPTIONS_ID ||
		robodoc_id == ROBODOC_PARAMETERS_ID ||
		robodoc_id == ROBODOC_SWITCHES_ID ||
 		robodoc_id == ROBODOC_OUTPUT_ID)
	{
		set_flags( robo_state->flags, ASRS_VarlistSubsection );
	}else if( robodoc_id == ROBODOC_NAME_ID )
	{	
		set_flags( robo_state->flags, ASRS_FormalPara );
	}

	return True;
}

void 
handle_robodoc_line( ASRobodocState *robo_state, int len )
{
	 /* skipping first 2 characters of the line */
	int offset = 2;
	LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d, line_len = %d", robo_state->curr, robo_state->len, len );
	if( len < offset ) 
	{	
		robo_state->curr += len ;
		return ;
	}
	/* it could be an empty line in which  case we need not skip one more space */
	if( robo_state->source[robo_state->curr+offset] == ' ' )
		++offset ;
	robo_state->curr += offset ;
	len -= offset ;
	if( !handle_robodoc_subtitle( robo_state, len ) ) 
		append_robodoc_line( robo_state, len ); 
	robo_state->curr += len ;
}

void 
handle_robodoc_quotation(  ASRobodocState *robo_state )
{
	int start = robo_state->curr ; 
	int end = 0 ;
	find_robodoc_section( robo_state );
	end = robo_state->curr-1 ; 
	LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d", robo_state->curr, robo_state->len );
	if( end > start ) 
	{
		xml_elem_t *sec_tag = NULL ;
		xml_elem_t* code_tag = NULL;
		xml_elem_t* ll_tag = NULL;
		if( robo_state->curr_subsection == NULL ) 
			sec_tag = robo_state->curr_section ;
		else
			sec_tag = robo_state->curr_subsection ;
			
		code_tag = xml_elem_new();
		code_tag->tag = mystrdup("code") ;
		code_tag->tag_id = DOCBOOK_code_ID ;
		xml_insert(sec_tag, code_tag);

		ll_tag = xml_elem_new();
		ll_tag->tag = mystrdup("literallayout") ;
		ll_tag->tag_id = DOCBOOK_literallayout_ID ;
		xml_insert(code_tag, ll_tag);
		
		append_CDATA_line( ll_tag, &(robo_state->source[start]), end - start );
	}
}

void 
handle_robodoc_section( ASRobodocState *robo_state )
{
	Bool section_end = False ;
	if( robodoc_section_header2xml( robo_state ) == NULL ) 
		return;
	set_flags(robo_state->flags, ASRS_InsideSection);
	LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d", robo_state->curr, robo_state->len );
	while( !section_end && robo_state->curr < robo_state->len ) 
	{
		const char *ptr = &(robo_state->source[robo_state->curr]) ;
		int i = 0 ;
		section_end = False ;
		LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d", robo_state->curr, robo_state->len );

		while( ptr[i] && ptr[i] != '\n' && !section_end) 
		{	
			section_end = ( ptr[i] == '*' && ptr[i+1] == '/' );
			++i ;
		}
		LOCAL_DEBUG_OUT("section_end = %d, i = %d", section_end, i );		
		if( section_end )
		{	
			if( i > 0 ) 
			{	
				if( strncmp( ptr, " */", 3 ) == 0 )
				{										/* line is the beginning of abe the end of the section */
					robo_state->curr += i+1 ;
					handle_robodoc_quotation( robo_state );
				}else if( strncmp( ptr, " * ", 3 ) == 0 )
					handle_robodoc_line( robo_state, i );		 	  
			}
		    if( ptr[i] != '\n' )
			{	
				ptr = &(robo_state->source[robo_state->curr]) ;
				i = 0 ;
				while( ptr[i] && ptr[i] != '\n' ) ++i ;
				robo_state->curr += i ;
			}
		}else
			handle_robodoc_line( robo_state, i+1 );
	}	 
	robo_state->curr_subsection = NULL ;
	clear_flags(robo_state->flags, ASRS_InsideSection);
}

xml_elem_t* 
robodoc2xml(const char *doc_str)
{
	xml_elem_t* doc = NULL ;
	ASRobodocState	robo_state ;

	memset( &robo_state, 0x00, sizeof(ASRobodocState));
	robo_state.source = doc_str ;
	robo_state.len = strlen( doc_str );

	LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d", robo_state.curr, robo_state.len );
	doc = xml_elem_new();
	doc->tag = mystrdup(XML_CONTAINER_STR) ;
	doc->tag_id = XML_CONTAINER_ID ;
	robo_state.doc = doc ;		
#if 1
	while( robo_state.curr < robo_state.len )
	{
		find_robodoc_section( &robo_state );
		if( robo_state.curr < robo_state.len )
			handle_robodoc_section( &robo_state );
	}	 
#endif	
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
	if( DocGenerationPass == 0 ) 
		xml_print(doc);
#endif
	return doc;
}

Bool 
convert_code_file( const char *source_dir, const char *file, ASXMLInterpreterState *state )
{
	char *source_file ;
	char *doc_str ; 
	Bool empty_file = False ;
	
	source_file = make_file_name( source_dir, file );
	doc_str = load_file(source_file);
/* 	LOCAL_DEBUG_OUT( "file %s loaded into {%s}", source_file, doc_str ); */
	if( doc_str != NULL )
	{
		xml_elem_t* doc;
		xml_elem_t* ptr;
		
		doc = robodoc2xml(doc_str);
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
		LOCAL_DEBUG_OUT( "xml_elem_delete for doc %p", doc );
		xml_elem_delete(NULL, doc);
		free( doc_str );		
	}	 	   
	LOCAL_DEBUG_OUT( "done with %s", source_file );
	free( source_file );
	fprintf( state->dest_fp, "\n" );
	return !empty_file;
}


void 
gen_code_doc( const char *source_dir, const char *dest_dir, 
			  const char *file, 
			  const char *display_name,
			  const char *display_purpose,
			  ASDocType doc_type )
{
	ASXMLInterpreterState state;
	char *dot = strrchr( file, '.' );
	char *slash ;
	char *short_fname;
	
	short_fname = dot?mystrndup( file, dot-file ): mystrdup( file ); 
	if( (slash = strrchr( short_fname, '/' )) != NULL )
	{	
		char *tmp = mystrdup( slash+1 );
		free( short_fname );
		short_fname = tmp ;
	}
		  
 	if( !start_doc_file( dest_dir, short_fname, NULL, doc_type, short_fname, display_name, display_purpose, &state, DOC_CLASS_None, DocClass_Overview ) )	  	
			return ;
	
	convert_code_file( source_dir, file, &state );
	end_doc_file( &state );	 	  
	free( short_fname );
}
