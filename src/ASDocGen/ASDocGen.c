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
#include "robodoc.h"
#include "docfile.h"
#include "datadoc.h"

SyntaxDef* TopLevelSyntaxes[] =
{
    &BaseSyntax,
    &ColorSyntax,
    &LookSyntax,
    &FeelSyntax,
    &AutoExecSyntax,
    &DatabaseSyntax,
#define MODULE_SYNTAX_START 6
    &PagerSyntax,
    &WharfSyntax,
    &WinListSyntax,
	&WinTabsSyntax,
    &AnimateSyntax,
    &AudioSyntax,
	NULL
};	 

const char *StandardSourceEntries[] = 
{
	"_related",	   
	"_synopsis",	
#define OPENING_PARTS_END   2	 
	"_overview",	   
	"_examples",	   
	"_footnotes",	   
	NULL
};

const char *PHPXrefFormat = "&nbsp;<? local_doc_url(\"%s.php\",\"%s\",\"%s%s\",$srcunset,$subunset) ?>\n ";
const char *PHPXrefFormatSetSrc = "&nbsp;<? local_doc_url(\"%s.php\",\"%s\",\"%s%s\",\"%s\",$subunset) ?>\n ";
const char *PHPXrefFormatUseSrc = "&nbsp;<? if ($src==\"\") $src=\"%s\"; local_doc_url(\"%s.php\",\"%s\",$src,$srcunset,$subunset) ?>\n ";
const char *PHPCurrPageFormat = "&nbsp;<b>%s</b>\n";

const char *AfterStepName = "AfterStep" ;
const char *UserGlossaryName = "Glossary" ; 
const char *UserTopicIndexName = "Topic index" ; 
const char *APIGlossaryName = "API Glossary" ; 
const char *APITopicIndexName = "API Topic index" ; 
const char *GlossaryName; 
const char *TopicIndexName; 

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

typedef struct {
	char *src_file ;
	char *descr_short ;
	char *descr_long ;				   
}ASApiSourceDescr;

ASApiSourceDescr libAfterImage_Sources[] = 
{	
	{"afterimage.h", "AFTERImage", "overview of libAfterImage image library"}, 
	{"asimagexml.c", "AFTERImage XML tags", "XML schema to be used for scripting image manipulation by AfterStep and ascompose"}, 
	{"asvisual.h", "ASVisual", "abstraction layer on top of X Visuals, focusing on color handling" },
  	{"asimage.h", "ASImage", "internal structures and methods used for image manipulation in libAfterImage" },
	{"blender.h", "ASImage Blending", "functionality for blending of image data using diofferent algorithms"}, 
	{"export.h", "ASImage Export", "functionality for writing images into files"}, 
	{"import.h", "ASImage Import", "functionality for reading images from files"},
	{"imencdec.h", "ASImage Encoding/decoding", "encoding/decoding ASImage data from/to usable data structures"}, 
	{"transform.h", "ASImage Transformations", "transformations available for ASImages"},
	{"ximage.h", "XImage", "functionality for displaying ASImages on X display"},
	{"ascmap.h", "Indexed Image handling", "defines main structures and function for image quantization"},
	{"asfont.h", "ASFont", "text drawing functionality and handling of TTF and X11 fonts"}, 
	{"char2uni.h", "Unicode", "handling on Unicode, UTF-8 and localized 8 bit encodings"}, 
	{"apps/asview.c",  "AFTERImage example 1: Image viewer","demonstrates loading and displaying of images"}, 
	{"apps/asscale.c", "AFTERImage example 2: Image scaling","demonstrates image loading and scaling"}, 
	{"apps/astile.c",  "AFTERImage example 3: Image tiling/croping","demonstrates image tiling/cropping and tinting"}, 
	{"apps/asmerge.c", "AFTERImage example 4: Image blending","demonstrates blending of multiple image using different algorithms"},
	{"apps/asgrad.c",  "AFTERImage example 5: Gradient rendering","demonstrates rendering of multi point linear gradients"}, 
	{"apps/asflip.c",  "AFTERImage example 6: Image rotation","demonstrates flipping image in 90 degree increments"}, 
	{"apps/astext.c",  "AFTERImage example 7: Text rendering","demonstrates antialiased texturized text rendering"},
	{"apps/common.c",  "AFTERImage examples common","common functions used in other examples "}, 
	{"apps/ascompose.c", "AFTERImage XML script processor","provides access to libAfterImage functionality, using scripts written in custom XML dialect"}, 
	{NULL, NULL, NULL}
};



const char *HTML_CSS_File = "html_styles.css" ;
const char *FAQ_HTML_CSS_File = "faq_html_styles.css" ;
static const char *HTML_DATA_BACKGROUND_File = "background.jpg" ;
/*static const char *HTML_HELP_BACKGROUND_File = NULL ; */
const char *CurrHtmlBackFile = NULL ;

ASHashTable *DocBookVocabulary = NULL ;

ASHashTable *ProcessedSyntaxes = NULL ;
ASHashTable *Glossary = NULL ;
ASHashTable *Index = NULL ;
ASHashTable *UserLinks = NULL ;
ASHashTable *APILinks = NULL ;
ASHashTable *Links = NULL ;

int DocGenerationPass = 0 ;
int CurrentManType = 1 ;

#define DATE_SIZE 64
char CurrentDateLong[DATE_SIZE] = "06/23/04";
char CurrentDateShort[DATE_SIZE] = "Jun 23,2004";

void check_syntax_source( const char *source_dir, SyntaxDef *syntax, Bool module );
void gen_syntax_doc( const char *source_dir, const char *dest_dir, SyntaxDef *syntax, ASDocType doc_type );
void gen_glossary( const char *dest_dir, const char *file, ASDocType doc_type );
void gen_index( const char *dest_dir, const char *file, ASDocType doc_type, Bool user_docs );
void gen_faq_doc( const char *source_dir, const char *dest_dir, ASDocType doc_type );

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
void
DeadPipe (int foo)
{
	{
		static int already_dead = False ; 
		if( already_dead ) 	return;/* non-reentrant function ! */
		already_dead = True ;
	}
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

void
asdocgen_usage (void)
{
	printf ("Usage:\n"
			"%s\t\t[-t |--target plain|text|html|php|xml|nroff|source] [-s | -css stylesheets_file]\n"
			"\t\t\t[--faq-css stylesheets_file] [--html-data-back background] [-d | --data]\n"
			"\t\t\t[-S | --source source_dir] [-D | --dst destination_dir]\n"
			"-t | --target 		- selects oputput file format\n"
			"-s | --css 		- selects which file to get HTML style sheets from\n"
			"     --faq-css    	- selects which file to get HTML style sheets from for FAQs\n"
			"	  --html-data-back - which image file to use as HTML background ( default background.jpg )\n"
			"-d | --data        - generate HTML catalogue of image/data files\n"
			"-S | --source      - specifies dir to read XML source or data source from\n"
			"-D | --dst         - specifies destination directory - where to wriote stuff to\n",
			MyName);
	exit (0);
}


int
main (int argc, char **argv)
{
	int i ; 
	char *source_dir = NULL ;
	const char *destination_dir = NULL ;
	Bool do_data = False;
	ASDocType target_type = DocType_Source ;
	/* Save our program name - for error messages */
	set_DeadPipe_handler(DeadPipe);
    InitMyApp (CLASS_ASDOCGEN, argc, argv, asdocgen_usage, NULL, 0 );
	LinkAfterStepConfig();
	InitSession();
    for( i = 1 ; i< argc ; ++i)
	{
		LOCAL_DEBUG_OUT( "argv[%d] = \"%s\", original argv[%d] = \"%s\"", i, argv[i]?argv[i]:"(null)", i, MyArgs.saved_argv[i]);	  
		if( argv[i] != NULL  )
		{
			if( (strcmp( argv[i], "-t" ) == 0 || strcmp( argv[i], "--target" ) == 0) && i+1 < argc && argv[i+1] != NULL ) 
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
			}else if( (strcmp( argv[i], "-s" ) == 0 || strcmp( argv[i], "--css" ) == 0) && i+1 < argc && argv[i+1] != NULL ) 
			{
				++i ;				
				HTML_CSS_File = argv[i] ;
			}else if( strcmp( argv[i], "--faq-css" ) == 0 && i+1 < argc && argv[i+1] != NULL ) 
			{
				++i ;				
				FAQ_HTML_CSS_File = argv[i] ;
			}else if( strcmp( argv[i], "--html-data-back" ) == 0 && i+1 < argc && argv[i+1] != NULL ) 
			{
				++i ;				
				if( strcasecmp( argv[i], "none") == 0 ) 
					HTML_DATA_BACKGROUND_File = NULL ;
				else
					HTML_DATA_BACKGROUND_File = argv[i] ;
			}else if( (strcmp( argv[i], "-d" ) == 0 || strcmp( argv[i], "--data" ) == 0) ) 
			{
				do_data = True ;
			}else if( (strcmp( argv[i], "-S" ) == 0 || strcmp( argv[i], "--source" ) == 0) && i+1 < argc && argv[i+1] != NULL ) 
			{
				++i ;				
				source_dir = argv[i] ;
			}else if( (strcmp( argv[i], "-D" ) == 0 || strcmp( argv[i], "--dst" ) == 0) && i+1 < argc && argv[i+1] != NULL ) 
			{
				++i ;				
				destination_dir = argv[i] ;
			}
		}
	}		  
	if( destination_dir == NULL ) 
		destination_dir = do_data?"data":ASDocTypeExtentions[target_type] ;
	if( source_dir == NULL ) 
		source_dir = do_data?"../../afterstep":"source" ;

#if 0

    ConnectAfterStep ( mask_reg, 0);
	
  	SendInfo ( "Nop \"\"", 0);
#endif
	ProcessedSyntaxes = create_ashash( 7, pointer_hash_value, NULL, NULL );
	Glossary = create_ashash( 0, string_hash_value, string_compare, string_destroy );
	Index = create_ashash( 0, string_hash_value, string_compare, string_destroy );
	UserLinks = create_ashash( 0, string_hash_value, string_compare, string_destroy );
	APILinks = create_ashash( 0, string_hash_value, string_compare, string_destroy );

	Links = UserLinks;

	GlossaryName = UserGlossaryName ; 
	TopicIndexName = UserTopicIndexName ; 

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
	
	if( target_type >= DocType_Source ) /* 1) generate HTML doc structure */
	{
		while( TopLevelSyntaxes[i] )
		{	/* create directory structure for source docs and all the missing files */
			check_syntax_source( source_dir, TopLevelSyntaxes[i], (i >= MODULE_SYNTAX_START) );
			++i ;	
		}
		check_syntax_source( source_dir, NULL, True );
	}else if( do_data )
	{	
		char *env_path1 = NULL, *env_path2 = NULL ;
		ASColorScheme *cs = NULL ;
		
	    if ((dpy = XOpenDisplay (MyArgs.display_name)))
		{
			Scr.MyDisplayWidth = DisplayWidth (dpy, Scr.screen);
			Scr.MyDisplayHeight = DisplayHeight (dpy, Scr.screen);

		    Scr.asv = create_asvisual (dpy, Scr.screen, DefaultDepth(dpy,Scr.screen), NULL);
		}else
		{		    
			Scr.asv = create_asvisual(NULL, 0, 32, NULL);
		}
		
		asxml_var_insert("xroot.width", 640);
    	asxml_var_insert("xroot.height", 480);
		
		env_path1 = getenv( "IMAGE_PATH" ) ;
		env_path2 = getenv( "PATH" );
		if( env_path1 == NULL ) 
		{
			env_path1 = env_path2;
			env_path2 = NULL ;
		}
	    Scr.image_manager = create_image_manager( NULL, 2.2, env_path1, env_path2, NULL );
		set_xml_image_manager( Scr.image_manager );
        
		env_path1 = getenv( "FONT_PATH" ) ;
		Scr.font_manager = create_font_manager( dpy, env_path1, NULL );
		set_xml_font_manager( Scr.font_manager );

		/*ReloadASEnvironment( NULL, NULL, NULL, False ); */
		cs = make_default_ascolor_scheme();
		populate_ascs_colors_rgb( cs );
		populate_ascs_colors_xml( cs );
		free( cs );

		TopicIndexName = NULL ;
		
		CurrHtmlBackFile = HTML_DATA_BACKGROUND_File ;
		gen_data_doc( 	source_dir, destination_dir?destination_dir:"data", "",
			  		  	"Installed data files - fonts, images and configuration",
			  			target_type );

		flush_ashash( Glossary );
		flush_ashash( Index );
	}else
	{
		char *api_dest_dir ;
		api_dest_dir = make_file_name( destination_dir, "API" );
		
		GlossaryName = UserGlossaryName ; 
		TopicIndexName = UserTopicIndexName ; 
		Links = UserLinks;

		DocGenerationPass = 2 ;
		while( --DocGenerationPass >= 0 ) 
		{
			gen_code_doc( "../../libAfterImage", destination_dir, 
			  		  	"asimagexml.c", 
			  		  	"AfterImage XML",
			  		  	"XML schema to be used for scripting image manipulation by AfterStep and ascompose",
			  		  	target_type );
		
			/* we need to generate some top level files for afterstep itself : */
			gen_syntax_doc( source_dir, destination_dir, NULL, target_type );
		
			for( i = 0 ; TopLevelSyntaxes[i] ; ++i )
				gen_syntax_doc( source_dir, destination_dir, TopLevelSyntaxes[i], target_type );
			
			if( DocGenerationPass == 0 ) 
			{	
				gen_faq_doc( source_dir, destination_dir, target_type );
				gen_glossary( destination_dir, "Glossary", target_type );
				gen_index( destination_dir, "index", target_type, True );
			}
			flush_ashash( ProcessedSyntaxes );
		}
		flush_ashash( Glossary );
		flush_ashash( Index );
		
		GlossaryName = APIGlossaryName ; 
		TopicIndexName = APITopicIndexName ; 
		Links = APILinks;
		DocGenerationPass = 2 ;
		
		CurrentManType = 3 ;
		
		while( --DocGenerationPass >= 0 ) 
		{
			int s ;
			for( s = 0 ; libAfterImage_Sources[s].src_file != NULL ; ++s ) 
			{	
				gen_code_doc( 	"../../libAfterImage", api_dest_dir, 
			  			  		libAfterImage_Sources[s].src_file, 
			  			  		libAfterImage_Sources[s].descr_short,
			  		  			libAfterImage_Sources[s].descr_long,
			  		  			target_type );
			}
			if( DocGenerationPass == 0 ) 
			{	
				gen_glossary( api_dest_dir, "Glossary", target_type );
				gen_index( api_dest_dir, "index", target_type, False );
			}
			flush_ashash( Glossary );
			flush_ashash( Index );
		}		  


	}		 
	
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
						Bool no_dir = False ;
						if( CheckDir(obsolete_dir) != 0 )
							no_dir = !make_doc_dir( obsolete_dir ) ;
						if( !no_dir )
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
				if( sub_syntax == pPopupFuncSyntax ) 
					sub_syntax = pFuncSyntax ;
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
			if( sub_syntax == pPopupFuncSyntax ) 
				sub_syntax = pFuncSyntax ;
			
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
gen_faq_doc( const char *source_dir, const char *dest_dir, ASDocType doc_type )
{
	ASXMLInterpreterState state;
	char *faq_dir = NULL ;
	ASFlagType doc_class_mask = DOC_CLASS_None	;
	struct direntry  **list = NULL;
	int list_len, i ;

	faq_dir = make_file_name( source_dir, "FAQ" );

	if( !start_doc_file( dest_dir, "afterstep_faq", NULL, doc_type, 
						 "afterstep_faq", 
						 "AfterStep FAQ",
						 "This document is an ever growing set of questions, statements, ideas and complaints about AfterStep version 2.0", 
						 &state, doc_class_mask, DocClass_FAQ ) )	 
		return ;
	
	/* BODY *************************************************************************/
	set_flags( state.flags, ASXMLI_OrderSections );
	list_len = my_scandir ((char*)faq_dir, &list, ignore_dots, NULL);
	for (i = 0; i < list_len; i++)
	{	
		if ( !S_ISDIR (list[i]->d_mode) )
			convert_xml_file( faq_dir, list[i]->d_name, &state );
		free(list[i]);
	}
	if( list ) 
		free( list );   
	
	/* FOOTER ***********************************************************************/
	end_doc_file( &state );	 	
	
	free( faq_dir );
}


void 
gen_glossary( const char *dest_dir, const char *file, ASDocType doc_type )
{
	ASXMLInterpreterState state;
	LOCAL_DEBUG_OUT( "Glossary has %ld items", Glossary->items_num);
	if( (doc_type == DocType_HTML	|| doc_type == DocType_PHP ) && Glossary->items_num > 0 )
	{	
		ASHashableValue *values;
		ASHashData *data;
		int items_num, col_length, i ;
		int col_end[3], col_curr[3], col_count = 3 ;
		Bool has_items = True, col_skipped[3] = {True, True, True};
		char c = '\0' ; 
				
		if( !start_doc_file( dest_dir, file, NULL, doc_type, NULL, NULL, NULL, &state, DOC_CLASS_None, DocClass_Glossary ) )	 
			return ;
		
		LOCAL_DEBUG_OUT( "sorting hash items : ... %s", "" );
		values = safecalloc( Glossary->items_num, sizeof(ASHashableValue));
		data = safecalloc( Glossary->items_num, sizeof(ASHashData));
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
gen_index( const char *dest_dir, const char *file, ASDocType doc_type, Bool user_docs )
{
	ASXMLInterpreterState state;
	if( (doc_type == DocType_HTML	|| doc_type == DocType_PHP ) && Index->items_num > 0 )
	{	
		ASHashableValue *values;
		ASHashData *data;
		int items_num, i ;
		Bool sublist = False ; 
		char *sublist_name= NULL ; 
		int sublist_name_len = 1 ;
		if( !start_doc_file( dest_dir, file, NULL, doc_type, NULL, NULL, NULL, &state, DOC_CLASS_None, DocClass_TopicIndex ) )	
			return ;
		LOCAL_DEBUG_OUT( "sorting hash items : ... %s", "" );
		values = safecalloc( Index->items_num, sizeof(ASHashableValue));
		data = safecalloc( Index->items_num, sizeof(ASHashData));
		items_num = sort_hash_items (Index, values, (void**)data, 0);
		
		if( user_docs )
		{	
			if( doc_type == DocType_PHP )
			{	
				fprintf( state.dest_fp, PHPXrefFormat, "visualdoc","Developer documentation index","API/index", "" );
				fprintf( state.dest_fp, PHPXrefFormat, "visualdoc","Installed data files catalogue","data/index", "" );
			}else if( doc_type == DocType_HTML )
			{	
 				fprintf( state.dest_fp,  "<A href=\"API/index.html\">Developer documentation index</A>&nbsp;&nbsp;\n" );
				fprintf( state.dest_fp,  "<A href=\"data/index.html\">Installed data files catalogue</A>\n" );			   
			}
		}else
		{	  
			if( doc_type == DocType_PHP )
			{
				fprintf( state.dest_fp, PHPXrefFormat, "visualdoc","User documentation index","index", "" );
				fprintf( state.dest_fp, PHPXrefFormat, "visualdoc","Installed data files catalogue","data/index", "" );
			}
			else if( doc_type == DocType_HTML )
			{
 				fprintf( state.dest_fp,  "<A href=\"../index.html\">User documentation index</A>&nbsp;&nbsp;\n" );
				fprintf( state.dest_fp,  "<A href=\"../data/index.html\">Installed data files catalogue</A>\n" );
			}
		}
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

