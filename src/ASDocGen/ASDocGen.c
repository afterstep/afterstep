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
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/aswindata.h"
#include "../../libAfterStep/event.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterStep/parser.h"
#include "../../libAfterConf/afterconf.h"

/* masks for AS pipe */
#define mask_reg 0
#define mask_off 0

SyntaxDef* TopLevelSyntaxes[] =
{
    &BaseSyntax,
    &ColorSyntax,
    &DatabaseSyntax,
    &FeelSyntax,
    &AutoExecSyntax,
    &LookSyntax,
#define MODULE_SYNTAX_START 6
    &AnimateSyntax,
    &AudioSyntax,
    &PagerSyntax,
    &WharfSyntax,
    &WinListSyntax,
	&WinTabsSyntax,
	NULL
};	 

const char *StandardSourceEntries[] = 
{
	"_synopsis",	
	"_overview",	   
#define OPENING_PARTS_END   2	 
	"_examples",	   
	"_related",	   
	"_footnotes",	   
	NULL
};

const char *StandardOptionsEntry = "_standard_options" ;
const char *MyStylesOptionsEntry = "_mystyles" ;
const char *BaseOptionsEntry = "_base_config" ;

typedef enum { 
	DocType_Plain = 0,
	DocType_HTML,
	DocType_PHP,
	DocType_XML,
	DocType_NROFF,
	DocType_Source,
	DocTypes_Count
}ASDocType;

const char *ASDocTypeExtentions[DocTypes_Count] = 
{
	"txt",
	"html",
	"php",
	"xml",
	"man",
	""
};

typedef struct ASXMLInterpreterState {
#define ASXMLI_LiteralLayout	(0x01<<0)	
#define ASXMLI_InsideLink		(0x01<<1)	  
#define ASXMLI_FirstArg 		(0x01<<2)	  
#define ASXMLI_LinkIsURL		(0x01<<3)	  
#define ASXMLI_LinkIsLocal		(0x01<<4)	  
#define ASXMLI_InsideExample			(0x01<<5)	  
	ASFlagType flags;
	
	FILE *dest_fp ;
	ASDocType doc_type ;
	int header_depth ;
	int group_depth ;
	char *curr_url_page ;
	char *curr_url_anchor ;

}ASXMLInterpreterState ;

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

void start_section_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_section_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

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

void start_listitem_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_listitem_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

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
emphasis
listitem
cmdsynopsis
replaceable
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
	DOCBOOK_ulink_ID,	  
	DOCBOOK_title_ID,	  
	DOCBOOK_group_ID,	  
	DOCBOOK_label_ID,	  
	DOCBOOK_anchor_ID,
	DOCBOOK_option_ID,	  
	DOCBOOK_choice_ID,	  
	DOCBOOK_command_ID,	  
	DOCBOOK_example_ID,	  
	DOCBOOK_linkend_ID,	  
	DOCBOOK_section_ID,	  
	DOCBOOK_refsect1_ID,	  
	DOCBOOK_emphasis_ID,	  
	DOCBOOK_listitem_ID,	  
	DOCBOOK_cmdsynopsis_ID,	  
	DOCBOOK_replaceable_ID,	  
	DOCBOOK_variablelist_ID,	  
	DOCBOOK_varlistentry_ID,	  
	DOCBOOK_literallayout_ID,
	
	DOCBOOK_SUPPORTED_IDS

}SupportedDocBookXMLTagIDs;

#define TAG_INFO_AND_ID(tagname)	#tagname, DOCBOOK_##tagname##_ID

ASDocTagHandlingInfo SupportedDocBookTagInfo[DOCBOOK_SUPPORTED_IDS] = 
{
	{ TAG_INFO_AND_ID(unknown), NULL, NULL },
	{ TAG_INFO_AND_ID(id), NULL, NULL },
	{ TAG_INFO_AND_ID(arg), start_arg_tag, end_arg_tag },
	{ TAG_INFO_AND_ID(url), NULL, NULL },
	{ TAG_INFO_AND_ID(para), start_para_tag, end_para_tag },
	{ TAG_INFO_AND_ID(term), start_term_tag, end_term_tag },
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

ASHashTable *DocBookVocabulary = NULL ;

ASHashTable *ProcessedSyntaxes = NULL ;

void check_syntax_source( const char *source_dir, SyntaxDef *syntax, Bool module );
void gen_syntax_doc( const char *source_dir, const char *dest_dir, SyntaxDef *syntax, ASDocType doc_type );




/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
void
DeadPipe (int foo)
{
    FreeMyAppResources();
#ifdef DEBUG_ALLOCS
    print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

	if( dpy ) 
	{	
    	XFlush (dpy);
    	XCloseDisplay (dpy);
	}
    exit (0);
}

int
main (int argc, char **argv)
{
	int i ; 
	char *source_dir = "source" ;
	const char *destination_dir = NULL ;
	ASDocType target_type = DocType_Source ;
	/* Save our program name - for error messages */
    InitMyApp (CLASS_ASDOCGEN, argc, argv, NULL, NULL, 0 );

    for( i = 1 ; i< argc ; ++i)
	{
		LOCAL_DEBUG_OUT( "argv[%d] = \"%s\", original argv[%d] = \"%s\"", i, argv[i], i, MyArgs.saved_argv[i]);	  
		if( argv[i] != NULL  )
		{
			if( (strcmp( argv[i], "-t" ) == 0 || strcmp( argv[i], "-target" ) == 0) && i+1 < argc && argv[i+1] != NULL ) 
			{
				if( mystrcasecmp( argv[i+1], "plain" ) == 0 || mystrcasecmp( argv[i+1], "text" ) == 0) 
					target_type = DocType_Plain ; 														   
				else if( mystrcasecmp( argv[i+1], "html" ) == 0 ) 
					target_type = DocType_HTML ; 														   
				else if( mystrcasecmp( argv[i+1], "php" ) == 0 ) 
					target_type = DocType_PHP ; 														   
				else if( mystrcasecmp( argv[i+1], "xml" ) == 0 ) 
					target_type = DocType_XML ; 														   
				else if( mystrcasecmp( argv[i+1], "nroff" ) == 0 ) 
					target_type = DocType_NROFF ; 														   
				else if( mystrcasecmp( argv[i+1], "source" ) == 0 ) 
					target_type = DocType_Source ; 														   
				else
					show_error( "unknown target type \"%s\"" );
			}	 
		}
	}		  
	if( destination_dir == NULL ) 
		destination_dir = ASDocTypeExtentions[target_type] ;
#if 0

    ConnectX( &Scr, PropertyChangeMask );
    ConnectAfterStep ( mask_reg);
	
  	SendInfo ( "Nop \"\"", 0);
#endif
	ProcessedSyntaxes = create_ashash( 7, pointer_hash_value, NULL, NULL );
	if( target_type < DocType_Source )
	{	
		DocBookVocabulary = create_ashash( 7, casestring_hash_value, casestring_compare, string_destroy_without_data );
		for( i = 1 ; i < DOCBOOK_SUPPORTED_IDS ; ++i )
			add_hash_item( DocBookVocabulary, AS_HASHABLE(SupportedDocBookTagInfo[i].tag), (void*)(SupportedDocBookTagInfo[i].tag_id));
	}
	i = 0 ; 
	LOCAL_DEBUG_OUT( "Starting main action... %s", "" );
	while( TopLevelSyntaxes[i] )
	{
	    /* 2 modes of operation : */
		if( target_type < DocType_Source ) /* 1) generate HTML doc structure */
 			gen_syntax_doc( source_dir, destination_dir, TopLevelSyntaxes[i], target_type );
		else	/* 2) create directory structure for source docs and all the missing files */
			check_syntax_source( source_dir, TopLevelSyntaxes[i], (i >= MODULE_SYNTAX_START) );
		++i ;	
	}	 
	/* we need to generate some top level files for afterstep itself : */
	if( target_type < DocType_Source ) /* 1) generate HTML doc structure */
	{
		/* TODO: */	  
		gen_syntax_doc( source_dir, destination_dir, NULL, target_type );
	}else
		check_syntax_source( source_dir, NULL, True );

	 
	if( dpy )   
    	XCloseDisplay (dpy);
    return 0;
}

/*************************************************************************/

Bool
make_doc_dir( const char *name )
{	 	  
	mode_t        perms = 0755;
   	show_progress ("Creating %s ... ", name);	
	if (mkdir (name, perms))
	{
    	show_error ("Failed to create documentation dir \"%s\" !\nPlease check permissions or contact your sysadmin !",
				 name);
		return False;
	}
    show_progress ("\t created.");
	return True;
}

void
write_standard_options_source( FILE *f )
{
	int i = 0;
	fprintf( f, "<anchor id=\"standard_options_list\"/>\n"
				"<refsect1>\n"
				"<title>STANDARD OPTIONS</title>\n"
				"<para>The following is the list of command line options supported by"
				" all AfterStep modules and applications.</para>" 
				"<variablelist>\n" );

	while( as_standard_cmdl_options[i].long_opt ) 
	{
		fprintf( f, "	<varlistentry>\n" );
		if( as_standard_cmdl_options[i].short_opt  )
  			fprintf( f, "	<term>-%s | --%s", as_standard_cmdl_options[i].short_opt, as_standard_cmdl_options[i].long_opt );
		else
  			fprintf( f, "	<term>     --%s", as_standard_cmdl_options[i].long_opt );
        
		if( get_flags( as_standard_cmdl_options[i].flags, CMO_HasArgs ) )
			fprintf( f, " <replaceable>val</replaceable>" );	

		fprintf( f, "</term>\n" 
					"	<listitem>\n" ); 
		fprintf( f, "   <para>%s. ", as_standard_cmdl_options[i].descr1 );
		if( as_standard_cmdl_options[i].descr2 )
			fprintf( f, " %s.", as_standard_cmdl_options[i].descr2 );
		fprintf( f, "   </para>\n"
					"   </listitem>\n"
					"	</varlistentry>\n" );
		++i ;
	}	 
	fprintf( f, "</variablelist>\n"
				"</refsect1>\n" );
}

void 
check_option_source( const char *syntax_dir, const char *option, SyntaxDef *sub_syntax, const char *module_name)
{
	char *filename = make_file_name( syntax_dir, option );
	if( CheckFile( filename ) != 0 || mystrcasecmp( option, StandardOptionsEntry ) == 0) 
	{
		FILE *f = fopen( filename, "w" );
		if( f )
		{	
			if( option[0] != '_' ) 
			{	
				fprintf( f, "<varlistentry id=\"options.%s\">\n"
  							"	<term>%s</term>\n"
  							"	<listitem>\n"
							"		<para>FIXME: add proper description here.</para>\n",
							option, option );
				if( sub_syntax ) 
					fprintf( f, "		<para>See Also: "
								"<ulink url=\"%s#synopsis\">%s</ulink> for further details.</para>\n",
							 sub_syntax->doc_path, sub_syntax->display_name );
				fprintf( f,	"	</listitem>\n"
  							"</varlistentry>\n" ); 
			}else
			{
				fprintf( f, "<section label=\"%s\" id=\"%s\">\n", &(option[1]), &(option[1]) ); 
				if( module_name )
				{	
					if( mystrcasecmp( &(option[1]), "synopsis" ) == 0 ) 
					{
						fprintf( f, "<cmdsynopsis>\n"
  									"<command>%s</command> [<ulink url=\"AfterStep#standard_options_list\">standard options</ulink>] \n"
									"</cmdsynopsis>\n", module_name );
					}else if( mystrcasecmp( option, StandardOptionsEntry ) == 0 ) 
					{
						write_standard_options_source( f );
					}
				}		
						 
				fprintf( f, "</section>\n" ); 
			}	 
			fclose(f); 	 
		}
		show_progress( "Option %s is missing - created empty source \"%s\".", option, filename );
	}	 
	free( filename );
}

void 
generate_main_source( const char *source_dir )
{
	int i ;
	for (i = 0; StandardSourceEntries[i] != NULL ; ++i)
		check_option_source( source_dir, StandardSourceEntries[i], NULL, "afterstep");	
	check_option_source( source_dir, StandardOptionsEntry, NULL, "afterstep");	 
}

void 
check_syntax_source( const char *source_dir, SyntaxDef *syntax, Bool module )
{
	int i ;
	char *syntax_dir = NULL ;
	char *obsolete_dir ;
	struct direntry  **list = NULL;
	int list_len ;

	if( syntax )
	{	
		if( get_hash_item( ProcessedSyntaxes, AS_HASHABLE(syntax), NULL ) == ASH_Success )
			return ;

		if( syntax->doc_path != NULL && syntax->doc_path[0] != '\0' )
			syntax_dir = make_file_name (source_dir, syntax->doc_path); 
	}
	if( syntax_dir == NULL ) 
		syntax_dir = mystrdup( source_dir );
	
	obsolete_dir = make_file_name (syntax_dir, "obsolete" );
	
	if( CheckDir(syntax_dir) != 0 )
		if( !make_doc_dir( syntax_dir ) ) 
		{
			free( syntax_dir );
			return;
		}

	if( syntax ) 
	{	
		add_hash_item( ProcessedSyntaxes, AS_HASHABLE(syntax), NULL );   

		/* pass one: lets see which of the existing files have no related options : */
		list_len = my_scandir ((char*)syntax_dir, &list, ignore_dots, NULL);
		for (i = 0; i < list_len; i++)
		{	
			int k ;
			if (!S_ISDIR (list[i]->d_mode))
			{	
				char *name = list[i]->d_name ;
				show_progress( "checking \"%s\" ... ", name );
				if( name[0] != '_' ) 
				{	
					for (k = 0; syntax->terms[k].keyword; k++)
						if( mystrcasecmp(name, syntax->terms[k].keyword ) == 0 ) 
							break;
					if( syntax->terms[k].keyword == NULL ) 
					{
						/* obsolete option - move it away */
						char *obsolete_fname = make_file_name (obsolete_dir, name );
						char *fname = make_file_name (syntax_dir, name );
						if( CheckDir(obsolete_dir) != 0 )
							if( make_doc_dir( obsolete_dir ) ) 
							{
								copy_file (fname, obsolete_fname);
								show_progress( "Option \"%s\" is obsolete - moving away!", name );
								unlink(fname);
							}
						free( fname );
						free( obsolete_fname ); 
					}	 
				}
			}		
			free( list[i] );
		}
		if( list )
			free (list);
		/* pass two: lets see which options are missing : */
		for (i = 0; syntax->terms[i].keyword; i++)
		{	
			if (syntax->terms[i].sub_syntax)
				check_syntax_source( source_dir, syntax->terms[i].sub_syntax, False );
			if( isalnum( syntax->terms[i].keyword[0] ) )					
				check_option_source( syntax_dir, syntax->terms[i].keyword, syntax->terms[i].sub_syntax, module?syntax->doc_path:NULL ) ;
		}
		for (i = module?0:1; StandardSourceEntries[i] != NULL ; ++i)
			check_option_source( syntax_dir, StandardSourceEntries[i], NULL, module?syntax->doc_path:NULL ) ;
		if( module ) 
		{
			check_option_source( syntax_dir, BaseOptionsEntry, NULL, syntax->doc_path ) ;
			check_option_source( syntax_dir, MyStylesOptionsEntry, NULL, syntax->doc_path ) ;
		}	 
	}else
		generate_main_source( syntax_dir );

	free( obsolete_dir );
	free( syntax_dir );
}	 

/*************************************************************************/
/*************************************************************************/
/* Document Generation code : 											 */
/*************************************************************************/
void 
convert_source_tag( xml_elem_t *doc, xml_elem_t **rparm, ASXMLInterpreterState *state )
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
			fprintf( state->dest_fp, "%s", ptr->parm );
		else 
			convert_source_tag( ptr, NULL, state );
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
convert_source_file( const char *syntax_dir, const char *file, ASXMLInterpreterState *state )
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
				convert_source_tag( ptr, NULL, state );
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

static Bool
start_doc_file( const char * dest_dir, const char *doc_path, const char *doc_postfix, SyntaxDef *syntax, ASDocType 	doc_type, ASXMLInterpreterState *state )
{
	char *dest_file = safemalloc( strlen( dest_dir ) + 1 + strlen(doc_path)+(doc_postfix?strlen(doc_postfix):0)+5+1 );
	char *ptr ;
	FILE *dest_fp ;
	Bool dst_dir_exists = True;

	sprintf( dest_file, "%s/%s%s.%s", dest_dir, doc_path, doc_postfix?doc_postfix:"", ASDocTypeExtentions[doc_type] ); 
	LOCAL_DEBUG_OUT( "starting doc \"%s\"", dest_file );	
	ptr = dest_file; 
	while( *ptr == '/' ) ++ptr ;
	ptr = strchr( ptr, '/' );
	while( ptr != NULL )
	{
		*ptr = '\0' ;
		if( CheckDir(dest_file) != 0 )
			if( !make_doc_dir( dest_file ) ) 
			{
		 		dst_dir_exists = False ;
				break;
			}
 		*ptr = '/' ;
		ptr = strchr( ptr+1, '/' );
	}
	if( !dst_dir_exists ) 
	{
		free( dest_file );	  
		return False ;
	}
	
	dest_fp = fopen( dest_file, "wt" );
	if( dest_fp == NULL ) 
	{
		show_error( "Failed to open destination file \"%s\" for writing!", dest_file );
		free( dest_file );
		return False;
	}				   

	memset( state, 0x00, sizeof(state));
	state->flags = ASXMLI_FirstArg ;
	state->dest_fp = dest_fp ;
	state->doc_type = doc_type ; 

	/* HEADER ***********************************************************************/
	write_syntax_doc_header( syntax, state );

	free( dest_file );
	LOCAL_DEBUG_OUT( "File opened with fptr %p", state->dest_fp );
	return True;
}

static void
end_doc_file( SyntaxDef *syntax, ASXMLInterpreterState *state )	
{
	if( state->dest_fp );
	{
		write_syntax_doc_footer( syntax, state );	
		fclose( state->dest_fp );
	}
	memset( &state, 0x00, sizeof(state));
}

int 
check_source_contents( const char *syntax_dir, const char *file )
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

void 
gen_syntax_doc( const char *source_dir, const char *dest_dir, SyntaxDef *syntax, ASDocType doc_type )
{
	ASXMLInterpreterState state;
	char *doc_path = "AfterStep" ;
	char *syntax_dir = NULL ;
	int i ;
	Bool do_mystyles = False, do_base = False ;
	int overview_size = 0 ;

	if( syntax )
	{	
		if( get_hash_item( ProcessedSyntaxes, AS_HASHABLE(syntax), NULL ) == ASH_Success )
			return ;
		doc_path = syntax->doc_path ;
	}
	
	if( syntax != NULL && syntax->doc_path != NULL && syntax->doc_path[0] != '\0' )
		syntax_dir = make_file_name (source_dir, syntax->doc_path); 
	if( syntax_dir == NULL ) 
		syntax_dir = mystrdup( source_dir );
	
	if( !start_doc_file( dest_dir, doc_path, NULL, syntax, doc_type, &state ) )	
		return ;
	
	if( doc_type != DocType_PHP ) 
	{	
		/* BODY *************************************************************************/
		i = 0 ;
		if( syntax == NULL ) 
		{	
			convert_source_file( syntax_dir, StandardSourceEntries[0], &state );
			++i ;
			convert_source_file( syntax_dir, StandardOptionsEntry, &state );
		}
		for( ; i < OPENING_PARTS_END ; ++i ) 
			convert_source_file( syntax_dir, StandardSourceEntries[i], &state );
		if( syntax ) 
		{	
			convert_source_file( syntax_dir, BaseOptionsEntry, &state );
			convert_source_file( syntax_dir, MyStylesOptionsEntry, &state );
		}
	}else
	{
		/* we generate upto 4 files in PHP mode : overview, Base config, MyStyles and Config Options
		 * Overview and Config Options are always present. Others may be ommited if source is missing 
		 * If Overview is too small - say < 1024 bytes - it could be bundled with Config Options */	   
		if( syntax == NULL ) 
		 	overview_size = 1000000 ;	
		else
		{	
			int tmp ;
			LOCAL_DEBUG_OUT( "Checking what parts to generate ...%s", "");
			do_mystyles = ((tmp = check_source_contents( syntax_dir, MyStylesOptionsEntry )) > 0);
			LOCAL_DEBUG_OUT( "MyStyle size = %d", tmp );
			do_base = ((tmp = check_source_contents( syntax_dir, BaseOptionsEntry )) > 0);
			LOCAL_DEBUG_OUT( "Base size = %d", tmp );
			for( i = 0 ; StandardSourceEntries[i] ; ++i )
				overview_size += check_source_contents( syntax_dir, StandardSourceEntries[i] );
			LOCAL_DEBUG_OUT( "overview size = %d", overview_size );
		}
		for( i = 0 ; StandardSourceEntries[i] ; ++i ) 
		{
			LOCAL_DEBUG_OUT( "converting %s", StandardSourceEntries[i] );
			convert_source_file( syntax_dir, StandardSourceEntries[i], &state );
			LOCAL_DEBUG_OUT( "done %s", StandardSourceEntries[i] );
		}
		if( overview_size > 1024 )
		{
			end_doc_file( syntax, &state );	 	  
			start_doc_file( dest_dir, doc_path, "_options", syntax, doc_type, &state );
		}	 
	}	 
	LOCAL_DEBUG_OUT( "starting config_options%s", "" );	
	if( syntax && state.dest_fp )
	{	
		write_syntax_options_header( syntax, &state );
		for (i = 0; syntax->terms[i].keyword; i++)
		{	
			if (syntax->terms[i].sub_syntax)
				gen_syntax_doc( source_dir, dest_dir, syntax->terms[i].sub_syntax, doc_type );
			if( isalnum( syntax->terms[i].keyword[0] ) )					
				convert_source_file( syntax_dir, syntax->terms[i].keyword, &state );
		}
		write_syntax_options_footer( syntax, &state );	  
	}
	LOCAL_DEBUG_OUT( "done with config_options%s", "" );
	
	if( doc_type != DocType_PHP ) 
	{
		for( i = 0 ; i < OPENING_PARTS_END ; ++i ) 
			convert_source_file( syntax_dir, StandardSourceEntries[i], &state );
	}else if( state.dest_fp )
	{
		if( do_base )
		{	
			end_doc_file( syntax, &state );	 	  	 		
			start_doc_file( dest_dir, doc_path, BaseOptionsEntry, syntax, doc_type, &state );
			convert_source_file( syntax_dir, BaseOptionsEntry, &state );
		}
		if( do_mystyles )
		{	
			end_doc_file( syntax, &state );	 	  	 		
			start_doc_file( dest_dir, doc_path, MyStylesOptionsEntry, syntax, doc_type, &state );
			convert_source_file( syntax_dir, MyStylesOptionsEntry, &state );
		}
	}		 


	/* FOOTER ***********************************************************************/
	end_doc_file( syntax, &state );	 	
			   
	if( syntax )
		add_hash_item( ProcessedSyntaxes, AS_HASHABLE(syntax), NULL );   
	
	free( syntax_dir );
}

/*************************************************************************/
/*************************************************************************/
/* DocBook XML tags handlers :											 */
/*************************************************************************/
void 
write_syntax_doc_header( SyntaxDef *syntax, ASXMLInterpreterState *state )
{
	char *display_name = "AfterStep" ;
	char *doc_name = "AfterStep" ;
	if( syntax ) 
	{	
		display_name = syntax->display_name ;
		doc_name = syntax->doc_path ;
	}
	++(state->header_depth);
	switch( state->doc_type ) 
	{
		case DocType_Plain :
			fprintf( state->dest_fp, "%s\n\n", display_name );
			break;
		case DocType_HTML :
			fprintf( state->dest_fp, "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
					  		"<html>\n"
					  		"<head><meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\">\n"
  					  		"<title>%s</title>\n"
					  		"</head>\n"
					  		"<body>\n"
					  		"<h1>%s</h1>\n", display_name, display_name );
			break;
 		case DocType_PHP :	
			fputs( "\n<? local_doc_url(\"visualdoc.php\",\"Index\",\"visualselect\",$srcunset,$subunset) ?>\n", state->dest_fp );
		    break ;
		case DocType_XML :
			fprintf( state->dest_fp, "<!DOCTYPE article PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\"\n"
                  					 "\"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\" [\n"
									 "<!ENTITY %s SYSTEM \"%s.xml\" NDATA SGML>\n"
									 "]>\n", doc_name, doc_name );
			fprintf( state->dest_fp, "<article id=\"index\">\n"
									 "<articleinfo>\n"
   									 "<authorgroup>\n"
      								 "<corpauthor>\n"
      								 "<ulink url=\"http://www.afterstep.org\">AfterStep Window Manager</ulink>\n"
      								 "</corpauthor>\n"
      								 "</authorgroup>\n"
									 "<title>%s documentation</title>\n"
									 "<releaseinfo>%s</releaseinfo>\n"
									 "</articleinfo>\n", display_name, VERSION );
			break ;
		case DocType_NROFF :
		    break ;
		default:
			break;
	}
}

void 
write_syntax_options_header( SyntaxDef *syntax, ASXMLInterpreterState *state )
{
	++(state->header_depth);
	switch( state->doc_type ) 
	{
		case DocType_Plain :
			fputs( "\nCONFIGURATION OPTIONS : \n", state->dest_fp );
			break;
		case DocType_HTML :
			fprintf( state->dest_fp, "\n<LI><A NAME=\"options\"><h%d>CONFIGURATION OPTIONS :</h%d></A>\n"
							  "<DL>\n", state->header_depth, state->header_depth);   
			break;
 		case DocType_PHP :	
			fprintf( state->dest_fp, "\n<LI><A NAME=\"options\"><B>CONFIGURATION OPTIONS :</B></A>\n"
							  "<DL>\n");   
		    break ;
		case DocType_XML :
			fprintf( state->dest_fp, "\n<section label=\"options\" id=\"options\"><title>CONFIGURATION OPTIONS :</title>\n" );
			break;
		case DocType_NROFF :
		    break ;
		default:
			break;
	}
}

void 
write_syntax_options_footer( SyntaxDef *syntax, ASXMLInterpreterState *state )
{
	switch( state->doc_type ) 
	{
		case DocType_Plain :
			fprintf( state->dest_fp, "\n");
			break;
		case DocType_HTML :
 		case DocType_PHP :	 
			fprintf( state->dest_fp, "\n</DL></LI>\n");   
		    break ;
		case DocType_XML :
			fprintf( state->dest_fp, "\n</section>\n" );
			break ;
		case DocType_NROFF :
		    break ;
		default:
			break;
	}
	--(state->header_depth);
}


void 
write_syntax_doc_footer( SyntaxDef *syntax, ASXMLInterpreterState *state )
{
	char *display_name = "AfterStep" ;
	if( syntax ) 
		display_name = syntax->display_name ;
	switch( state->doc_type ) 
	{
		case DocType_Plain :
			break;
		case DocType_HTML :
			fprintf( state->dest_fp, "<p><FONT face=\"Verdana, Arial, Helvetica, sans-serif\" size=\"-2\">AfterStep version %s</a></FONT>\n",
					 VERSION ); 
			fprintf( state->dest_fp, "</body>\n</html>\n" );			
			break;
 		case DocType_PHP :	
		    break ;
		case DocType_XML :
			fprintf( state->dest_fp, "\n</article>\n" );
		    break ;
		case DocType_NROFF :
		    break ;
		default:
			break;
	}
	--(state->header_depth);
}

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
				if( state->curr_url_anchor == NULL ) 
					fprintf( state->dest_fp, "#%s\">", attr->parm );	
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
start_para_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP )
	{	
		close_link(state);
		fprintf( state->dest_fp, "<P class=\"dense\">" );	
	}
}

void 
end_para_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	close_link(state);
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP )
	{
		fwrite( "</P>", 1, 4, state->dest_fp );	
	}
}


void 
start_command_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		fwrite( "<B>", 1, 3, state->dest_fp );	
}

void 
end_command_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		fwrite( "</B>", 1, 4, state->dest_fp );	
}

void 
start_emphasis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		fwrite( "<I>", 1, 3, state->dest_fp );	
}

void 
end_emphasis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		fwrite( "</I>", 1, 4, state->dest_fp );	
}

void 
start_literallayout_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
 	close_link(state);
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP)
	{	
		fprintf( state->dest_fp, "<PRE>");	
	}
}

void 
end_literallayout_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP)
	{	
		fwrite( "</PRE>", 1, 6, state->dest_fp );	  
	}
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
}

void 
start_varlistentry_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		add_anchor( parm, state );
}

void 
end_varlistentry_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )			
		close_link(state);
}

void 
start_term_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	close_link(state);
	if( state->doc_type == DocType_HTML || state->doc_type == DocType_PHP	 )
		fprintf( state->dest_fp, "<DT class=\"dense\"><B>" );	
}

void 
end_term_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML || state->doc_type == DocType_PHP	 )
		fwrite( "</B></DT>", 1, 9, state->dest_fp );	
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
	}
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
	}
}

void 
end_cmdsynopsis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	--(state->header_depth);	
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		fwrite( "</LI>", 1, 5, state->dest_fp );
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
}

void 
start_arg_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( !get_flags(state->flags, ASXMLI_FirstArg ) && state->group_depth > 0 )
		fwrite( "| ", 1, 2, state->dest_fp );
	clear_flags(state->flags, ASXMLI_FirstArg );
	if( check_choice( parm ) ) 
		fputc( '[', state->dest_fp );
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		fwrite( "<B>", 1, 3, state->dest_fp );
}

void 
end_arg_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	--(state->group_depth);	
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP )
		fwrite( "</B>", 1, 4, state->dest_fp );
	if( check_choice( parm ) ) 
		fputc( ']', state->dest_fp );
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
	}
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
start_ulink_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
	{
		add_local_link( parm, state );
	}	
}
	 
void 
end_ulink_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	|| state->doc_type == DocType_PHP	 )
		close_link(state);
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


