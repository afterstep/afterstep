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

#include "ASDocGen.h"
#include "xmlproc.h"

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

const char *PHPXrefFormat = "&nbsp;<? local_doc_url(\"%s.php\",\"%s\",\"%s%s\",$srcunset,$subunset) ?>\n ";
const char *PHPXrefFormatSetSrc = "&nbsp;<? local_doc_url(\"%s.php\",\"%s\",\"%s%s\",\"%s\",$subunset) ?>\n ";
const char *PHPXrefFormatUseSrc = "&nbsp;<? if ($src==\"\") $src=\"%s\"; local_doc_url(\"%s.php\",\"%s\",$src,$srcunset,$subunset) ?>\n ";
const char *PHPCurrPageFormat = "&nbsp;<b>%s</b>\n";

const char *AfterStepName = "AfterStep" ;
const char *GlossaryName = "Glossary :" ; 
const char *TopicIndexName = "Topic index :" ; 

const char *ASDocTypeExtentions[DocTypes_Count] = 
{
	"txt",
	"html",
	"php",
	"xml",
	"man",
	""
};

const char *StandardOptionsEntry = "_standard_options" ;
const char *MyStylesOptionsEntry = "_mystyles" ;
const char *BaseOptionsEntry = "_base_config" ;

const char *DocClassStrings[4][2] = 
{
	{"Overview", 	  ""},
	{"Base options",  "_base_config"},
	{"MyStyles", 	  "_mystyles"},
	{"Configuration", "_options"}
};	 

const char *HTML_CSS_File = "html_styles.css" ;

ASHashTable *DocBookVocabulary = NULL ;

ASHashTable *ProcessedSyntaxes = NULL ;
ASHashTable *Glossary = NULL ;
ASHashTable *Index = NULL ;

#define DATE_SIZE 64
char CurrentDateLong[DATE_SIZE] = "06/23/04";
char CurrentDateShort[DATE_SIZE] = "Jun 23,2004";

void check_syntax_source( const char *source_dir, SyntaxDef *syntax, Bool module );
void gen_syntax_doc( const char *source_dir, const char *dest_dir, SyntaxDef *syntax, ASDocType doc_type );
void gen_glossary( const char *dest_dir, ASDocType doc_type );
void gen_index( const char *dest_dir, ASDocType doc_type );

void write_doc_header( ASXMLInterpreterState *state );
void write_options_header( ASXMLInterpreterState *state );
void write_options_footer( ASXMLInterpreterState *state );
void write_doc_footer( ASXMLInterpreterState *state );
Bool make_doc_dir( const char *name );
Bool start_doc_file( const char * dest_dir, const char *doc_path, const char *doc_postfix, ASDocType doc_type, 
     	    	     const char *doc_name, const char *display_name, const char *display_purpose, 
					 ASXMLInterpreterState *state, 
				ASFlagType doc_class_mask, ASDocClass doc_class );
void end_doc_file( ASXMLInterpreterState *state );

void gen_code_doc( 	const char *source_dir, const char *dest_dir, 
	 			  	const char *file, 
				  	const char *display_name,
				  	const char *display_purpose,
			  		ASDocType doc_type );

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
				++i ;
				if( mystrcasecmp( argv[i], "plain" ) == 0 || mystrcasecmp( argv[i], "text" ) == 0) 
					target_type = DocType_Plain ; 														   
				else if( mystrcasecmp( argv[i], "html" ) == 0 ) 
					target_type = DocType_HTML ; 														   
				else if( mystrcasecmp( argv[i], "php" ) == 0 ) 
					target_type = DocType_PHP ; 														   
				else if( mystrcasecmp( argv[i], "xml" ) == 0 ) 
					target_type = DocType_XML ; 														   
				else if( mystrcasecmp( argv[i], "nroff" ) == 0 ) 
					target_type = DocType_NROFF ; 														   
				else if( mystrcasecmp( argv[i], "source" ) == 0 ) 
					target_type = DocType_Source ; 														   
				else
					show_error( "unknown target type \"%s\"" );
			}else if( (strcmp( argv[i], "-s" ) == 0 || strcmp( argv[i], "-css" ) == 0) && i+1 < argc && argv[i+1] != NULL ) 
			{
				++i ;				
				HTML_CSS_File = argv[i] ;
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
	Glossary = create_ashash( 0, string_hash_value, string_compare, string_destroy );
	Index = create_ashash( 0, string_hash_value, string_compare, string_destroy );
	if( target_type < DocType_Source )
	{	
		time_t curtime;
    	struct tm *loctime;
		
		DocBookVocabulary = create_ashash( 7, casestring_hash_value, casestring_compare, string_destroy_without_data );
		for( i = 1 ; i < DOCBOOK_SUPPORTED_IDS ; ++i )
			add_hash_item( DocBookVocabulary, AS_HASHABLE(SupportedDocBookTagInfo[i].tag), (void*)(SupportedDocBookTagInfo[i].tag_id));
		
		/* Get the current time. */
		curtime = time (NULL);
     	/* Convert it to local time representation. */
		loctime = localtime (&curtime);
		strftime(CurrentDateLong, DATE_SIZE, "%b %e %Y", loctime);
		strftime(CurrentDateShort, DATE_SIZE, "%m/%d/%Y", loctime);
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
		gen_syntax_doc( source_dir, destination_dir, NULL, target_type );
		gen_glossary( destination_dir, target_type );
		gen_index( destination_dir, target_type );
		gen_code_doc( "../../libAfterImage", destination_dir, 
			  		  "asimagexml.c", 
			  		  "libAfterImage XML tags",
			  		  "XML tags supported by ascompose and AfterStep in xml images",
			  		  target_type );
	}else
		check_syntax_source( source_dir, NULL, True );

	 
	if( dpy )   
    	XCloseDisplay (dpy);
    return 0;
}

/*************************************************************************/

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
  									"<command>%s</command> [<ulink url=\"%s#standard_options_list\">standard options</ulink>] \n"
									"</cmdsynopsis>\n", module_name, AfterStepName );
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
					if( syntax->terms[k].keyword == NULL || get_flags( syntax->terms[k].flags, TF_OBSOLETE) ) 
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
			if( !get_flags( syntax->terms[i].flags, TF_OBSOLETE) )
			{	
				SyntaxDef *sub_syntax = syntax->terms[i].sub_syntax ; 
				if( sub_syntax == &PopupFuncSyntax ) 
					sub_syntax = &FuncSyntax ;
				if (sub_syntax)
					check_syntax_source( source_dir, sub_syntax, False );
				if( isalnum( syntax->terms[i].keyword[0] ) )					
					check_option_source( syntax_dir, syntax->terms[i].keyword, sub_syntax, module?syntax->doc_path:NULL ) ;
			}
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
gen_syntax_doc( const char *source_dir, const char *dest_dir, SyntaxDef *syntax, ASDocType doc_type )
{
	ASXMLInterpreterState state;
	const char *doc_path = AfterStepName ;
	char *syntax_dir = NULL ;
	int i ;
	ASFlagType doc_class_mask = 0 ;

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

	if( doc_type == DocType_PHP ) 
	{
		int overview_size = 0 ;
		int tmp ;
		/* we generate upto 4 files in PHP mode : overview, Base config, MyStyles and Config Options
		 * Overview and Config Options are always present. Others may be ommited if source is missing 
		 * If Overview is too small - say < 1024 bytes - it could be bundled with Config Options */	   
		
		set_flags( doc_class_mask, DOC_CLASS_Overview );
		LOCAL_DEBUG_OUT( "Checking what parts to generate ...%s", "");
		if( (tmp = check_xml_contents( syntax_dir, MyStylesOptionsEntry )) > 0)
			set_flags( doc_class_mask, DOC_CLASS_MyStyles );
		LOCAL_DEBUG_OUT( "MyStyle size = %d", tmp );
		if((tmp = check_xml_contents( syntax_dir, BaseOptionsEntry )) > 0)
			set_flags( doc_class_mask, DOC_CLASS_BaseConfig );
		LOCAL_DEBUG_OUT( "Base size = %d", tmp );
		for( i = 0 ; StandardSourceEntries[i] ; ++i )
			overview_size += check_xml_contents( syntax_dir, StandardSourceEntries[i] );
		if( syntax == NULL ) 
			overview_size += 0 ;
		LOCAL_DEBUG_OUT( "overview size = %d", overview_size );
		if( overview_size > OVERVIEW_SIZE_THRESHOLD )
			set_flags( doc_class_mask, DOC_CLASS_Options );
	}else
		doc_class_mask = DOC_CLASS_None	;
	   
	if( !start_doc_file( dest_dir, doc_path, NULL, doc_type, 
						 syntax?syntax->doc_path:NULL, 
						 syntax?syntax->display_name:NULL, 
						 syntax?syntax->display_purpose:NULL, 
						 &state, doc_class_mask, DocClass_Overview ) )	 
		return ;
	
	if( doc_type != DocType_PHP ) 
	{	
		/* BODY *************************************************************************/
		i = 0 ;
		if( syntax == NULL ) 
		{	
			convert_xml_file( syntax_dir, StandardSourceEntries[0], &state );
			++i ;
			convert_xml_file( syntax_dir, StandardOptionsEntry, &state );
		}
		for( ; i < OPENING_PARTS_END ; ++i ) 
			convert_xml_file( syntax_dir, StandardSourceEntries[i], &state );
		if( syntax ) 
		{	
			convert_xml_file( syntax_dir, BaseOptionsEntry, &state );
			convert_xml_file( syntax_dir, MyStylesOptionsEntry, &state );
		}
	}else
	{
		i = 0 ;
		if( syntax == NULL ) 
		{	
			convert_xml_file( syntax_dir, StandardSourceEntries[0], &state );
			++i ;
			convert_xml_file( syntax_dir, StandardOptionsEntry, &state );
		}
		for( ; StandardSourceEntries[i] ; ++i ) 
			convert_xml_file( syntax_dir, StandardSourceEntries[i], &state );
		
		if( get_flags( doc_class_mask, DOC_CLASS_Options ) )
		{
			end_doc_file( &state );	 	  
			start_doc_file(  dest_dir, doc_path, "_options", doc_type,
							 syntax?syntax->doc_path:NULL, 
							 syntax?syntax->display_name:NULL, 
							 syntax?syntax->display_purpose:NULL, 
							 &state, doc_class_mask, DocClass_Options );
			fprintf( state.dest_fp, "<UL>\n" );
		}	 
	}	 
	LOCAL_DEBUG_OUT( "starting config_options%s", "" );	
	if( syntax && state.dest_fp )
	{	
		write_options_header( &state );
		for (i = 0; syntax->terms[i].keyword; i++)
		{	
			SyntaxDef *sub_syntax = syntax->terms[i].sub_syntax ; 
			if( sub_syntax == &PopupFuncSyntax ) 
				sub_syntax = &FuncSyntax ;
			
			if (sub_syntax)
				gen_syntax_doc( source_dir, dest_dir, sub_syntax, doc_type );
			if( isalnum( syntax->terms[i].keyword[0] ) )					
				convert_xml_file( syntax_dir, syntax->terms[i].keyword, &state );
		}
		write_options_footer( &state );	  
	}
	LOCAL_DEBUG_OUT( "done with config_options%s", "" );
	
	if( doc_type != DocType_PHP ) 
	{
		for( i = OPENING_PARTS_END ; StandardSourceEntries[i] ; ++i ) 
			convert_xml_file( syntax_dir, StandardSourceEntries[i], &state );
	}else if( state.dest_fp )
	{
		if( state.doc_class == DocClass_Options )
			fprintf( state.dest_fp, "</UL>\n" );
		if( get_flags( doc_class_mask, DOC_CLASS_BaseConfig ) )
		{	
			end_doc_file( &state );	 	  	 		
			start_doc_file( dest_dir, doc_path, BaseOptionsEntry, doc_type,
							syntax?syntax->doc_path:NULL, 
							syntax?syntax->display_name:NULL, 
							syntax?syntax->display_purpose:NULL, 
							&state, doc_class_mask, DocClass_BaseConfig );
			convert_xml_file( syntax_dir, BaseOptionsEntry, &state );
		}
		if( get_flags( doc_class_mask, DOC_CLASS_MyStyles ) )
		{	
			end_doc_file( &state );	 	  	 		
			start_doc_file( dest_dir, doc_path, MyStylesOptionsEntry, doc_type, 
							syntax?syntax->doc_path:NULL, 
							syntax?syntax->display_name:NULL, 
							syntax?syntax->display_purpose:NULL, 
							&state, doc_class_mask, DocClass_MyStyles );
			convert_xml_file( syntax_dir, MyStylesOptionsEntry, &state );
		}
	}		 


	/* FOOTER ***********************************************************************/
	end_doc_file( &state );	 	
			   
	if( syntax )
		add_hash_item( ProcessedSyntaxes, AS_HASHABLE(syntax), NULL );   
	
	free( syntax_dir );
}

void 
gen_glossary( const char *dest_dir, ASDocType doc_type )
{
	ASXMLInterpreterState state;
	if( (doc_type == DocType_HTML	|| doc_type == DocType_PHP ) && Glossary->items_num > 0 )
	{	
		ASHashableValue *values = safecalloc( Glossary->items_num, sizeof(ASHashableValue));
		ASHashData *data = safecalloc( Glossary->items_num, sizeof(ASHashData));
		int items_num, col_length, i ;
		int col_end[3], col_curr[3], col_count = 3 ;
		Bool has_items = True, col_skipped[3] = {True, True, True};
		char c = '\0' ; 
		if( !start_doc_file( dest_dir, "Glossary", NULL, doc_type, NULL, NULL, NULL, &state, DOC_CLASS_None, DocClass_Glossary ) )	 
			return ;
		items_num = sort_hash_items (Glossary, values, (void**)data, 0);
		
		fprintf( state.dest_fp, "<p>\n" );
		for( i = 0 ; i < items_num ; ++i ) 
		{
			if( ((char*)values[i])[0] != c ) 
			{
				c = ((char*)values[i])[0] ;
				fprintf( state.dest_fp, "<A href=\"#glossary_%c\">%c</A> ", c, c );
			}	 
		}	 
		fprintf( state.dest_fp, "<hr>\n<p><table width=100%% cellpadding=0 cellspacing=0>\n" );
		
		if( state.doc_type == DocType_PHP	)
			col_count = 2 ;
		col_length = (items_num+col_count-1)/col_count ;

		col_curr[0] = 0 ; 
		col_end[0] = col_curr[1] = col_length ;
		col_end[1] = col_curr[2] = col_length*2 ;
		col_end[2] = items_num ;

		while( has_items )
		{
			int col ;
			fprintf( state.dest_fp, "<TR>" );
			has_items = False ; 
			for( col = 0 ; col < col_count ; ++col )
			{		
				int item = col_curr[col] ; 
				fprintf( state.dest_fp, "<TD width=33%% valign=top>" );
				if( item < col_end[col] && item < items_num ) 		   
				{	
					has_items = True ;
					col_skipped[col] = !col_skipped[col] && item > 0 && ((char*)values[item])[0] != ((char*)values[item-1])[0] ;
					if( !col_skipped[col] )
					{	  
						if( state.doc_type == DocType_HTML	)
							fprintf( state.dest_fp, "<A href=\"%s\">%s</A>", data[item].cptr, (char*)values[item] );
						else if( doc_type == DocType_PHP ) 
							fprintf( state.dest_fp, PHPXrefFormat, "visualdoc",(char*)values[item], data[item].cptr, "" );
						++(col_curr[col]) ; 
						col_skipped[col] = False ;
					}else
						fprintf( state.dest_fp, "<A name=\"glossary_%c\"> </A>", ((char*)values[item])[0] );
				}
				fprintf( state.dest_fp, " </TD>" );
			}
			fprintf( state.dest_fp, "</TR>\n" );
		}	 
		fprintf( state.dest_fp, "</table>\n" );

		free( data );
		free( values );
		end_doc_file( &state );	 	  
	}
}

void 
gen_index( const char *dest_dir, ASDocType doc_type )
{
	ASXMLInterpreterState state;
	if( (doc_type == DocType_HTML	|| doc_type == DocType_PHP ) && Index->items_num > 0 )
	{	
		ASHashableValue *values = safecalloc( Index->items_num, sizeof(ASHashableValue));
		ASHashData *data = safecalloc( Index->items_num, sizeof(ASHashData));
		int items_num, i ;
		Bool sublist = False ; 
		char *sublist_name= NULL ; 
		int sublist_name_len = 1 ;
		if( !start_doc_file( dest_dir, "index", NULL, doc_type, NULL, NULL, NULL, &state, DOC_CLASS_None, DocClass_TopicIndex ) )	
			return ;
		items_num = sort_hash_items (Index, values, (void**)data, 0);
		
		fprintf( state.dest_fp, "<hr>\n<p><UL class=\"dense\">\n" );

		for( i = 0 ; i < items_num ; ++i ) 
		{
			char *item_text = (char*)values[i];
			if( sublist_name ) 
			{	
				if( strncmp( item_text, sublist_name, sublist_name_len ) == 0 ) 
				{
					if( !sublist ) 
					{
						fprintf( state.dest_fp, "\n<UL>\n" );
	  					sublist = True ;
					}
					item_text += sublist_name_len ;
					while( *item_text != '\0' && isspace( *item_text ) ) 
						++item_text ;
				}else if( sublist ) 
				{	
					sublist = False ;
					fprintf( state.dest_fp, "\n</UL>\n" );
				}
			}
			if( !sublist ) 
			{	
				sublist_name = item_text ; 
				sublist_name_len = strlen( sublist_name );
			}
			fprintf( state.dest_fp, "<LI class=\"dense\">" );

			if( state.doc_type == DocType_HTML	)
				fprintf( state.dest_fp, "<A href=\"%s\">%s</A>", data[i].cptr, item_text );
			else if( doc_type == DocType_PHP ) 
			{
				char *url = data[i].cptr ;
				char *ptr = &(url[strlen(url)-4]) ;	  
				if( *ptr == '.' ) 
					*ptr = '\0';
				fprintf( state.dest_fp, PHPXrefFormat, "visualdoc",item_text, url, "" );
				if( *ptr == '\0' ) 
					*ptr = '.';
			}
			fprintf( state.dest_fp, "</LI>\n" );
		}	 
			
		if( sublist ) 
			fprintf( state.dest_fp, "</UL>\n" );
		fprintf( state.dest_fp, "</UL>\n" );

		free( data );
		free( values );
		end_doc_file( &state );	 	  
	}
}

typedef struct ASRobodocState
{

#define ASRS_InsideSection			(0x01<<0)	  

	ASFlagType flags ;
 	const char *source ;
	int len ;
	int curr ;	   

	xml_elem_t* doc ;
	xml_elem_t* curr_section ;
	xml_elem_t* curr_subsection ;
}ASRobodocState;

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
		LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d", robo_state->curr, robo_state->len );
		if( robo_state->source[robo_state->curr] == '/' ) 
		{
			int count = 0 ;
			const char *ptr = &(robo_state->source[++(robo_state->curr)]) ;

			while(ptr[count] == '*' || (count == 4 && isalpha(ptr[count])))
				++count ;
			if( count == 6 && (ptr[count] == ' ' || get_flags(robo_state->flags, ASRS_InsideSection)))
				return ; 				       /* we foiund it ! */
		}
		skip_robodoc_line( robo_state );
	}	
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

typedef enum {
 	ROBODOC_NAME_ID = 0,
	ROBODOC_COPYRIGHT_ID,
	ROBODOC_SYNOPSIS_ID,
 	ROBODOC_USAGE_ID,
 	ROBODOC_FUNCTION_ID,
	ROBODOC_DESCRIPTION_ID,
	ROBODOC_PURPOSE_ID,
	ROBODOC_AUTHOR_ID,
	ROBODOC_CREATION_DATE_ID,
	ROBODOC_MODIFICATION_HISTORY_ID,
	ROBODOC_HISTORY_ID,
 	ROBODOC_INPUTS_ID,
	ROBODOC_ARGUMENTS_ID,
	ROBODOC_OPTIONS_ID,
	ROBODOC_PARAMETERS_ID,
	ROBODOC_SWITCHES_ID,
 	ROBODOC_OUTPUT_ID,
	ROBODOC_SIDE_EFFECTS_ID,
 	ROBODOC_RESULT_ID,
	ROBODOC_RETURN_VALUE_ID,
 	ROBODOC_EXAMPLE_ID,
 	ROBODOC_NOTES_ID,
 	ROBODOC_DIAGNOSTICS_ID,
 	ROBODOC_WARNINGS_ID,
	ROBODOC_ERRORS_ID,
 	ROBODOC_BUGS_ID,
 	ROBODOC_TODO_ID,
	ROBODOC_IDEAS_ID,
 	ROBODOC_PORTABILITY_ID,
 	ROBODOC_SEE_ALSO_ID,
 	ROBODOC_METHODS_ID,
	ROBODOC_NEW_METHODS_ID,
 	ROBODOC_ATTRIBUTES_ID,
	ROBODOC_NEW_ATTRIBUTES_ID,
 	ROBODOC_TAGS_ID,
 	ROBODOC_COMMANDS_ID,
 	ROBODOC_DERIVED_FROM_ID,
 	ROBODOC_DERIVED_BY_ID,
 	ROBODOC_USES_ID,
	ROBODOC_CHILDREN_ID,
 	ROBODOC_USED_BY_ID,
	ROBODOC_PARENTS_ID,
 	ROBODOC_SOURCE_ID,
	ROBODOC_SUPPORTED_IDs                          
}SupportedRoboDocTagIDs;

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
	id_length = i ;
	if( i > 6 ) 
	{	
		i = 6 ; 
		while( i < id_length && isspace(ptr[i]) ) ++i; 	
		if( i < id_length ) 
		{
			id = (char*)(&ptr[i]);
			id_length -= i ;	 
			saved = id[id_length];
			id[id_length] = '\0' ;
		}else
			id_length = 0 ;	 
	}

	section->parm = safemalloc( 5+2+1+2+2+2+id_length+1+1 );
	sprintf(section->parm, "remap=\"%c\" id=\"%s\"", ptr[4], id );
	
	if( id_length > 0 ) 
		id[id_length] = saved ;
	xml_insert(robo_state->doc, section);
	robo_state->curr_section = section ;
	robo_state->curr_subsection = NULL ;	
	return section;		   
}

void 
append_CDATA_line( xml_elem_t *tag, const char *line, int len )
{
	xml_elem_t *cdata_tag = tag->child ;
	LOCAL_DEBUG_CALLER_OUT("tag->tag = \"%s\", line_len = %d", tag->tag, len );

	while( cdata_tag != NULL ) 
	{
		if( cdata_tag->tag_id == XML_CDATA_ID ) 
			break;
		cdata_tag = cdata_tag->next ;
	}	 
	if( cdata_tag == NULL ) 
	{
		cdata_tag = xml_elem_new();
		cdata_tag->tag = mystrdup(XML_CDATA_STR) ;
		cdata_tag->tag_id = XML_CDATA_ID ;
		xml_insert(tag, cdata_tag);
	}	 
	if( cdata_tag->parm == NULL ) 
		cdata_tag->parm = mystrndup( line, len );
	else
	{
		int old_len = strlen(cdata_tag->parm);
		cdata_tag->parm = realloc( cdata_tag->parm, old_len+1+len+1 );
		cdata_tag->parm[old_len] = '\n' ; 
		strncpy( &(cdata_tag->parm[old_len+1]), line, len );
		cdata_tag->parm[old_len+1+len] = '\0';
	}	 
}

void 
append_robodoc_line( ASRobodocState *robo_state, int len )
{
	xml_elem_t* sec_tag = NULL;
	xml_elem_t* ll_tag = NULL;
	LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d", robo_state->curr, robo_state->len );
	if( robo_state->curr_subsection == NULL ) 
		sec_tag = robo_state->curr_section ;
	else
		sec_tag = robo_state->curr_subsection ;

	for(ll_tag = sec_tag->child; ll_tag != NULL ; ll_tag = ll_tag->next ) 
		if( ll_tag->tag_id == DOCBOOK_literallayout_ID ) 
			break;
	if( ll_tag == NULL )
	{
		ll_tag = xml_elem_new();
		ll_tag->tag = mystrdup("literallayout") ;
		ll_tag->tag_id = DOCBOOK_literallayout_ID ;
		xml_insert(sec_tag, ll_tag);
	}	 
	append_CDATA_line( ll_tag, &(robo_state->source[robo_state->curr]), len );
}

Bool
handle_robodoc_subtitle( ASRobodocState *robo_state, int len )
{
	const char *ptr = &(robo_state->source[robo_state->curr]);
	int i = 0; 
	int robodoc_id = -1 ;
	xml_elem_t* sec_tag = NULL;
	xml_elem_t* title_tag = NULL;
	xml_elem_t* title_text_tag = NULL;

	LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d", robo_state->curr, robo_state->len );
	while( isspace(ptr[i]) && i < len ) ++i ;
	while( isspace(ptr[len]) && i < len ) --len ;

	ptr = &(ptr[i]);
	len -= i ;
	if( len < 0 )
		return False;
	
	for( i = 0 ; i < ROBODOC_SUPPORTED_IDs ; ++i ) 
	 	if( strlen( SupportedRoboDocTagInfo[i].tag ) == len ) 		  
			if( strncmp( ptr, SupportedRoboDocTagInfo[i].tag, len ) == 0 ) 
			{
				robodoc_id = SupportedRoboDocTagInfo[i].tag_id ; 
				break;
			}	 
	if( robodoc_id < 0 ) 
		return False ;
	/* otherwise we have to create subsection */
	sec_tag = xml_elem_new();
	sec_tag->tag = mystrdup("refsect1") ;
	sec_tag->tag_id = DOCBOOK_refsect1_ID ;
	xml_insert(robo_state->curr_section, sec_tag);
	
	robo_state->curr_subsection = sec_tag ;

	title_tag = xml_elem_new();
	title_tag->tag = mystrdup("title") ;
	title_tag->tag_id = DOCBOOK_title_ID ;
	xml_insert(sec_tag, title_tag);
	title_text_tag = xml_elem_new();
	title_text_tag->tag = mystrdup(XML_CDATA_STR) ;
	title_text_tag->tag_id = XML_CDATA_ID ;
	title_text_tag->parm = mystrdup(SupportedRoboDocTagInfo[i].tag);
	xml_insert(title_tag, title_text_tag);
	return True;
}

void 
handle_robodoc_line( ASRobodocState *robo_state, int len )
{
	 /* skipping first 3 characters of the line */
	LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d, line_len = %d", robo_state->curr, robo_state->len, len );
	if( len < 4 ) 
	{	
		robo_state->curr += len ;
		return ;
	}
	robo_state->curr += 3 ;
	len -= 3 ;
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
	robodoc_section_header2xml( robo_state );

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
		
		if( i == 0 )
			++i ;
		if( section_end )
		{	
			if( strncmp( ptr, " */", 3 ) == 0 )
			{										/* line is the beginning of abe the end of the section */
				robo_state->curr += i ;
				handle_robodoc_quotation( robo_state );
			}else if( strncmp( ptr, " * ", 3 ) == 0 )
				handle_robodoc_line( robo_state, i );		 	  
		}else
			handle_robodoc_line( robo_state, i );
		ptr = &(robo_state->source[robo_state->curr]) ;
		i = 0 ;
		while( ptr[i] && ptr[i] != '\n' ) ++i ;
		robo_state->curr += i ;
	}	 
	robo_state->curr_subsection = NULL ;
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
		LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d", robo_state.curr, robo_state.len );
		find_robodoc_section( &robo_state );
		LOCAL_DEBUG_OUT("robo_state> curr = %d, len = %d", robo_state.curr, robo_state.len );
		if( robo_state.curr < robo_state.len )
			handle_robodoc_section( &robo_state );
		break;
	}	 
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
		
 	if( !start_doc_file( dest_dir, file, NULL, doc_type, file, display_name, display_purpose, &state, DOC_CLASS_None, DocClass_Overview ) )	  	
			return ;
	
	convert_code_file( source_dir, file, &state );
	end_doc_file( &state );	 	  
}
