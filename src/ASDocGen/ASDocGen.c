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
	ASFlagType flags;
	
	FILE *dest_fp ;
	ASDocType doc_type ;
	int header_depth ;
	int group_depth ;
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

void start_part_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_part_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_title_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_title_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

void start_olink_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );
void end_olink_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state );

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



/* supported DocBook tags : 
	<part label="overview" id="overview">
	<refsect1 id='usage'>
	<title>
	<para>
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
	<olink linkend="Bevel#synopsis">
	<example>

	Here is the complete vocabulary :
id
arg
part
para
term
title
group
olink
label
option
choice
command
example
linkend
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
	DOCBOOK_part_ID,	  
	DOCBOOK_para_ID,	  
	DOCBOOK_term_ID,	  
	DOCBOOK_title_ID,	  
	DOCBOOK_group_ID,	  
	DOCBOOK_olink_ID,	  
	DOCBOOK_label_ID,	  
	DOCBOOK_option_ID,	  
	DOCBOOK_choice_ID,	  
	DOCBOOK_command_ID,	  
	DOCBOOK_example_ID,	  
	DOCBOOK_linkend_ID,	  
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
	{ TAG_INFO_AND_ID(part), start_part_tag, end_part_tag },
	{ TAG_INFO_AND_ID(para), start_para_tag, end_para_tag },
	{ TAG_INFO_AND_ID(term), start_term_tag, end_term_tag },
	{ TAG_INFO_AND_ID(title), start_title_tag, end_title_tag },
	{ TAG_INFO_AND_ID(group), start_group_tag, end_group_tag },
	{ TAG_INFO_AND_ID(olink), start_olink_tag, end_olink_tag },
	{ TAG_INFO_AND_ID(label), NULL, NULL },
	{ TAG_INFO_AND_ID(option), start_option_tag, end_option_tag },
	{ TAG_INFO_AND_ID(choice), NULL, NULL },
	{ TAG_INFO_AND_ID(command), start_command_tag, end_command_tag },
	{ TAG_INFO_AND_ID(example), start_example_tag, end_example_tag },
	{ TAG_INFO_AND_ID(linkend), NULL, NULL },
	{ TAG_INFO_AND_ID(refsect1), start_part_tag, end_part_tag },
	{ TAG_INFO_AND_ID(emphasis), start_emphasis_tag, end_emphasis_tag },
	{ TAG_INFO_AND_ID(listitem), start_listitem_tag, start_listitem_tag },
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
	char *destination_dir = "html" ;
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
			}	 
		}
	}		  
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
	while( TopLevelSyntaxes[i] )
	{
	    /* 2 modes of operation : */
		if( target_type < DocType_Source ) /* 1) generate HTML doc structure */
 			gen_syntax_doc( source_dir, destination_dir, TopLevelSyntaxes[i], target_type );
		else	/* 2) create directory structure for source docs and all the missing files */
			check_syntax_source( source_dir, TopLevelSyntaxes[i], (i >= MODULE_SYNTAX_START) );
		++i ;	
	}	 
	 
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
	fprintf( f, "<refsect1 id='standard_options'><title>STANDARD OPTIONS</title>\n"
				"<variablelist>\n" );

	while( as_standard_cmdl_options[i].long_opt ) 
	{
		fprintf( f, "	<varlistentry>\n" );
		if( as_standard_cmdl_options[i].short_opt  )
  			fprintf( f, "	<term>-%5.5s | --%s</term>\n", as_standard_cmdl_options[i].short_opt, as_standard_cmdl_options[i].long_opt );
		else
  			fprintf( f, "	<term>         --%s</term>\n", as_standard_cmdl_options[i].long_opt );
		fprintf( f, "	<listitem>\n" ); 
		fprintf( f, "   <para>%s</para>\n", as_standard_cmdl_options[i].descr1 );
		if( as_standard_cmdl_options[i].descr2 )
			fprintf( f, "   <para>%s</para>\n", as_standard_cmdl_options[i].descr2 );
		fprintf( f, "   </listitem>\n"
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
	if( CheckFile( filename ) != 0 ) 
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
								"<olink linkend=\"%s#synopsis\">%s</olink> for further details.</para>\n",
							 sub_syntax->doc_path, sub_syntax->display_name );
				fprintf( f,	"	</listitem>\n"
  							"</varlistentry>\n" ); 
			}else
			{
				fprintf( f, "<part label=\"%s\" id=\"%s\">\n", &(option[1]), &(option[1]) ); 
				if( module_name )
				{	
					if( mystrcasecmp( &(option[1]), "synopsis" ) == 0 ) 
					{
						fprintf( f, "<cmdsynopsis>\n"
  									"<command>%s</command>\n"
									"[<olink linkend=\"AfterStep#standard_options\">standard options</olink>] \n"
									"</cmdsynopsis>\n", module_name );
					}else if( mystrcasecmp( &(option[1]), "overview" ) == 0 && 
						 	  mystrcasecmp( module_name, "afterstep" ) == 0 ) 
					{
						write_standard_options_source( f );
					}
				}		
						 
				fprintf( f, "</part>\n" ); 
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
}


void 
check_syntax_source( const char *source_dir, SyntaxDef *syntax, Bool module )
{
	int i ;
	char *syntax_dir ;
	char *obsolete_dir ;
	struct direntry  **list = NULL;
	int list_len ;

	if( get_hash_item( ProcessedSyntaxes, AS_HASHABLE(syntax), NULL ) == ASH_Success )
		return ;

	if( syntax->doc_path == NULL || syntax->doc_path[0] == '\0' )
		syntax_dir = mystrdup( source_dir );
	else
		syntax_dir = make_file_name (source_dir, syntax->doc_path); 
	obsolete_dir = make_file_name (syntax_dir, "obsolete" );
	
	if( CheckDir(syntax_dir) != 0 )
		if( !make_doc_dir( syntax_dir ) ) 
		{
			free( syntax_dir );
			return;
		}

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
	for (i = 0; StandardSourceEntries[i] != NULL ; ++i)
		check_option_source( syntax_dir, StandardSourceEntries[i], NULL, module?syntax->doc_path:NULL ) ;

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
	xml_elem_t* parm = xml_parse_parm(doc->parm, DocBookVocabulary);	
	xml_elem_t* ptr ;
	
	if( doc->tag_id > 0 && doc->tag_id < DOCBOOK_SUPPORTED_IDS ) 
		if( SupportedDocBookTagInfo[doc->tag_id].handle_start_tag ) 
			SupportedDocBookTagInfo[doc->tag_id].handle_start_tag( doc, parm, state ); 
	for (ptr = doc->child ; ptr ; ptr = ptr->next) 
	{
		if (ptr->tag_id == XML_CDATA_ID ) 
			fprintf( state->dest_fp, "%s", ptr->parm );
		else 
			convert_source_tag( ptr, NULL, state );
	}
	if( doc->tag_id > 0 && doc->tag_id < DOCBOOK_SUPPORTED_IDS ) 
		if( SupportedDocBookTagInfo[doc->tag_id].handle_end_tag ) 
			SupportedDocBookTagInfo[doc->tag_id].handle_end_tag( doc, parm, state ); 
	if (rparm) *rparm = parm; 
	else xml_elem_delete(NULL, parm);
}

void 
convert_source_file( const char *syntax_dir, const char *file, ASXMLInterpreterState *state )
{
	char *source_file ;
	char *doc_str ; 
	
	source_file = make_file_name( syntax_dir, file );
	doc_str = load_file(source_file);
	if( doc_str != NULL )
	{
		xml_elem_t* doc;
		xml_elem_t* ptr;
		doc = xml_parse_doc(doc_str, DocBookVocabulary);
		for (ptr = doc->child ; ptr ; ptr = ptr->next) 
			convert_source_tag( ptr, NULL, state );
		/* Delete the xml. */
		xml_elem_delete(NULL, doc);
		free( doc_str );		
	}	 	   
	free( source_file );
}

void 
write_syntax_doc_header( SyntaxDef *syntax, ASXMLInterpreterState *state )
{
	++(state->header_depth);
	switch( state->doc_type ) 
	{
		case DocType_Plain :
			fprintf( state->dest_fp, "%s\n\n", syntax->display_name );
			break;
		case DocType_HTML :
			fprintf( state->dest_fp, "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
					  		"<html>\n"
					  		"<head><meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\">\n"
  					  		"<title>%s</title>\n"
					  		"</head>\n"
					  		"<body>\n"
					  		"<h1>%s</h1>\n", syntax->display_name, syntax->display_name );
			break;
 		case DocType_PHP :	
		    break ;
		case DocType_XML :
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
			fprintf( state->dest_fp, "\nCONFIGURATION OPTIONS : \n");
			break;
		case DocType_HTML :
			fprintf( state->dest_fp, "\n<A NAME=\"options\"><h%d>CONFIGURATION OPTIONS :</h%d></A>\n"
							  "<DL>\n", state->header_depth, state->header_depth);   
			break;
 		case DocType_PHP :	
		    break ;
		case DocType_XML :
			break ;
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
			fprintf( state->dest_fp, "\n</DL>\n");   
			break;
 		case DocType_PHP :	
		    break ;
		case DocType_XML :
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
	switch( state->doc_type ) 
	{
		case DocType_Plain :
			break;
		case DocType_HTML :
			fprintf( state->dest_fp, "</body>\n</html>\n" );			
			break;
 		case DocType_PHP :	
		    break ;
		case DocType_XML :
		    break ;
		case DocType_NROFF :
		    break ;
		default:
			break;
	}
	--(state->header_depth);
}


void 
gen_syntax_doc( const char *source_dir, const char *dest_dir, SyntaxDef *syntax, ASDocType doc_type )
{
	char *dest_file, *ptr ;
	FILE *dest_fp ; 
	Bool dst_dir_exists = True ;
	ASXMLInterpreterState state;

	if( get_hash_item( ProcessedSyntaxes, AS_HASHABLE(syntax), NULL ) == ASH_Success )
		return ;

	dest_file = safemalloc( strlen( dest_dir ) + 1 + strlen(syntax->doc_path) + 5+1 );
	sprintf( dest_file, "%s/%s.%s", dest_dir, syntax->doc_path, ASDocTypeExtentions[doc_type] ); 
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
		return ;
	}
	
	dest_fp = fopen( dest_file, "wt" );
	if( dest_fp == NULL ) 
	{
		show_error( "Failed to open destination file \"%s\" for writing!", dest_file );
		free( dest_file );
		return ;
	}				   

	add_hash_item( ProcessedSyntaxes, AS_HASHABLE(syntax), NULL );   
	memset( &state, 0x00, sizeof(state));
	state.flags = ASXMLI_FirstArg ;
	state.dest_fp = dest_fp ;
	state.doc_type = doc_type ; 

	/* HEADER ***********************************************************************/
	write_syntax_doc_header( syntax, &state );
	/* BODY *************************************************************************/
	{
		int i ;
		char *syntax_dir ;
		syntax_dir = make_file_name( source_dir, syntax->doc_path );
		for( i = 0 ; i < OPENING_PARTS_END ; ++i ) 
			convert_source_file( syntax_dir, StandardSourceEntries[i], &state );
		
		write_syntax_options_header( syntax, &state );
		for (i = 0; syntax->terms[i].keyword; i++)
		{	
			if (syntax->terms[i].sub_syntax)
				gen_syntax_doc( source_dir, dest_dir, syntax->terms[i].sub_syntax, doc_type );
			if( isalnum( syntax->terms[i].keyword[0] ) )					
				convert_source_file( syntax_dir, syntax->terms[i].keyword, &state );
		}
		write_syntax_options_footer( syntax, &state );	  
		for( i = OPENING_PARTS_END ; StandardSourceEntries[i]; ++i ) 
			convert_source_file( syntax_dir, StandardSourceEntries[i], &state );
		free( syntax_dir );
	}
	/* FOOTER ***********************************************************************/
	write_syntax_doc_footer( syntax, &state );	
	fclose( dest_fp );
}

/*************************************************************************/
/*************************************************************************/
/* DocBook XML tags handlers :											 */
/*************************************************************************/
void 
add_html_anchor( xml_elem_t *attr, ASXMLInterpreterState *state )
{
	while( attr ) 
	{
		if( attr->tag_id == DOCBOOK_id_ID ) 
		{
			if( get_flags( state->flags, ASXMLI_InsideLink ) )
				fprintf( state->dest_fp, "</A>" );
			fprintf( state->dest_fp, "\n<A NAME=\"%s\">", attr->parm );
			set_flags( state->flags, ASXMLI_InsideLink );
			break;	
		}	 
		attr = attr->next ;	
	}	 
		   
}	 

void
add_html_local_link( xml_elem_t *attr, ASXMLInterpreterState *state )
{
	while( attr ) 
	{
		if( attr->tag_id == DOCBOOK_linkend_ID ) 
		{
			char *ptr ;
			if( get_flags( state->flags, ASXMLI_InsideLink ) )
				fprintf( state->dest_fp, "</A>" );
			ptr = strchr( attr->parm, '#' );
			if( ptr != NULL ) 
			{
				*ptr = '\0' ;
				fprintf( state->dest_fp, "\n<A href=\"%s.html#%s\">", attr->parm, ptr+1 );
				*ptr = '#' ;
			}else
				fprintf( state->dest_fp, "\n<A href=\"%s\">", attr->parm );
			set_flags( state->flags, ASXMLI_InsideLink );
			break;	
		}	 
		attr = attr->next ;	
	}	 
		   
}	 

void 
close_html_link( ASXMLInterpreterState *state )
{
	if( get_flags( state->flags, ASXMLI_InsideLink ) )
	{	
		fwrite( "</A>", 1, 4, state->dest_fp );	   
		clear_flags( state->flags, ASXMLI_InsideLink );
	}
}

void 
start_para_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
	{	
		close_html_link(state);
		fwrite( "<P>", 1, 3, state->dest_fp );	
	}
}

void 
end_para_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
	{
		close_html_link(state);
		fwrite( "</P>", 1, 4, state->dest_fp );	
	}
}


void 
start_command_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
		fwrite( "<B>", 1, 3, state->dest_fp );	
}

void 
end_command_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
		fwrite( "</B>", 1, 4, state->dest_fp );	
}

void 
start_emphasis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
		fwrite( "<I>", 1, 3, state->dest_fp );	
}

void 
end_emphasis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
		fwrite( "</I>", 1, 4, state->dest_fp );	
}

void 
start_literallayout_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
	{	
		close_html_link(state);
		fwrite( "<PRE>", 1, 5, state->dest_fp );	
	}
}

void 
end_literallayout_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
		fwrite( "</PRE>", 1, 6, state->dest_fp );	
}

void 
start_variablelist_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
	{
		close_html_link(state);
		fwrite( "<DL>", 1, 4, state->dest_fp );	
	}
}

void 
end_variablelist_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
		fwrite( "</DL>", 1, 5, state->dest_fp );	
}

void 
start_varlistentry_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
		add_html_anchor( parm, state );
}

void 
end_varlistentry_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )			
		close_html_link(state);
}

void 
start_term_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
		fwrite( "<DT>", 1, 4, state->dest_fp );	
}

void 
end_term_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
	{	
		fwrite( "</DT>", 1, 5, state->dest_fp );	
		close_html_link(state);
	}
}

void 
start_listitem_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
		fwrite( "<DD>", 1, 4, state->dest_fp );	
}

void 
end_listentry_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
	{	
		fwrite( "</DD>", 1, 5, state->dest_fp );	
		close_html_link(state);
	}
}

void 
start_part_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	++(state->header_depth);	
	if( state->doc_type == DocType_HTML	 )
		add_html_anchor( parm, state );
}

void 
end_part_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	--(state->header_depth);	
}

void 
start_title_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
		fprintf( state->dest_fp, "<h%d>", state->header_depth );	
}

void 
end_title_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
	{	
		fprintf( state->dest_fp, "</h%d>", state->header_depth );	
		close_html_link(state);
	}
}

void 
start_cmdsynopsis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	++(state->header_depth);	
	if( state->doc_type == DocType_HTML	 )
	{
		add_html_anchor( parm, state );
		fprintf( state->dest_fp, "<h%d>SYNOPSIS</h%d>", state->header_depth, state->header_depth );	   			  
		close_html_link(state);
	}
}

void 
end_cmdsynopsis_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	--(state->header_depth);	
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
	if( state->doc_type == DocType_HTML	 )
		fwrite( "<B>", 1, 3, state->dest_fp );
}

void 
end_arg_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	--(state->group_depth);	
	if( state->doc_type == DocType_HTML	 )
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
	if( state->doc_type == DocType_HTML	 )
	{
		add_html_anchor( parm, state );
		fprintf( state->dest_fp, " <I>Example : </I> " );	   			  
		close_html_link(state);
	}
}

void 
end_example_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
}

void 
start_olink_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
	{
		add_html_local_link( parm, state );
	}	
}
	 
void 
end_olink_tag( xml_elem_t *doc, xml_elem_t *parm, ASXMLInterpreterState *state )
{
	if( state->doc_type == DocType_HTML	 )
		close_html_link(state);
}	 


