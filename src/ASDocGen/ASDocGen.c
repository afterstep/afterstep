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
    &AnimateSyntax,
    &AudioSyntax,
    &BaseSyntax,
    &ColorSyntax,
    &DatabaseSyntax,
    &FeelSyntax,
    &AutoExecSyntax,
    &LookSyntax,
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

void check_syntax_source( const char *source_dir, SyntaxDef *syntax );
void make_syntax_html( const char *source_dir, const char *dest_dir, SyntaxDef *syntax );

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

typedef enum { 
	DocType_HTML = 0,
	DocType_PHP,
	DocType_XML,
	DocType_NROFF,
	DocType_Source,
	DocTypes_Count
}ASDocType;

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
				if( mystrcasecmp( argv[i+1], "html" ) == 0 ) 
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
    /* 2 modes of operation : */
	/* 1) create directory structure for source docs and all the missing files */
	i = 0 ; 
	while( TopLevelSyntaxes[i] )
	{
		switch( target_type ) 
		{
			case DocType_HTML :
				make_syntax_html( source_dir, destination_dir, TopLevelSyntaxes[i] );
			    break ;	  
			default:
				check_syntax_source( source_dir, TopLevelSyntaxes[i] );
		}
		++i ;	
	}	 
	/* 2) generate HTML doc structure */
	 
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
check_option_source( const char *syntax_dir, const char *option, const char *syntax_name)
{
	char *filename = make_file_name( syntax_dir, option );
	if( CheckFile( filename ) != 0 ) 
	{
		FILE *f = fopen( filename, "w" );
		if( f )
		{	
			if( option[0] != '_' ) 
			{	
				fprintf( f, "<varlistentry id=\"%s.options.%s\">\n"
  							"	<term>%s</term>\n"
  							"	<listitem>\n"
							"		<para>FIXME: add proper description here.</para>\n"   
							"	</listitem>\n"
  							"</varlistentry>\n", syntax_name, option, option ); 
			}else
			{
				fprintf( f, "<part label=\"%s\" id=\"%s.%s\">\n"
  							"</part>\n", &(option[1]), syntax_name, &(option[1]) ); 
			}	 
			fclose(f); 	 
		}
		show_progress( "Option %s is missing - created empty source \"%s\".", option, filename );
	}	 
	free( filename );
}


void 
check_syntax_source( const char *source_dir, SyntaxDef *syntax )
{
	int i ;
	char *syntax_dir ;
	char *obsolete_dir ;
	struct direntry  **list = NULL;
	int list_len ;

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
			check_syntax_source( source_dir, syntax->terms[i].sub_syntax );
		if( isalnum( syntax->terms[i].keyword[0] ) )					
			check_option_source( syntax_dir, syntax->terms[i].keyword, syntax->doc_path ) ;
	}
	for (i = 0; StandardSourceEntries[i] != NULL ; ++i)
		check_option_source( syntax_dir, StandardSourceEntries[i], syntax->doc_path ) ;

	free( obsolete_dir );
	free( syntax_dir );
}	 

typedef struct ASXMLInterpreterState {
#define ASXMLI_LiteralLayout	(0x01<<0)	
	ASFlagType flags;

}ASXMLInterpreterState ;

void
preprocess_tag( xml_elem_t *doc, xml_elem_t *parm, FILE *dest_fp, ASDocType doc_type, ASXMLInterpreterState *state )
{
	
}
	
void
postprocess_tag( xml_elem_t *doc, xml_elem_t *parm, FILE *dest_fp, ASDocType doc_type, ASXMLInterpreterState *state )
{
	
}

void 
convert_source_tag( xml_elem_t *doc, FILE *dest_fp, ASDocType doc_type, xml_elem_t **rparm, ASXMLInterpreterState *state )
{
	xml_elem_t* parm = xml_parse_parm(doc->parm, NULL);	
	xml_elem_t* ptr ;
	
	preprocess_tag( doc, parm, dest_fp, doc_type, state );	
	for (ptr = doc->child ; ptr ; ptr = ptr->next) 
	{
		if (ptr->tag_id == XML_CDATA_ID ) 
			fprintf( stderr, "%s", ptr->parm );
		else 
			convert_source_tag( ptr, dest_fp, doc_type, NULL, state );
	}
	postprocess_tag( doc, parm, dest_fp, doc_type, state );
	if (rparm) *rparm = parm; 
	else xml_elem_delete(NULL, parm);
}

void 
convert_source_file( const char *syntax_dir, const char *file, FILE *dest_fp, ASDocType doc_type )
{
	char *source_file ;
	char *doc_str ; 
	
	source_file = make_file_name( syntax_dir, file );
	doc_str = load_file(source_file);
	if( doc_str != NULL )
	{
		xml_elem_t* doc;
		xml_elem_t* ptr;
		ASXMLInterpreterState state = {0};

		doc = xml_parse_doc(doc_str, NULL);
		for (ptr = doc->child ; ptr ; ptr = ptr->next) 
			convert_source_tag( ptr, dest_fp, doc_type, NULL, &state );
		/* Delete the xml. */
		xml_elem_delete(NULL, doc);
		free( doc_str );		
	}	 	   
	free( source_file );
}


void 
make_syntax_html( const char *source_dir, const char *dest_dir, SyntaxDef *syntax )
{
	char *dest_file, *ptr ;
	FILE *dest_fp ; 
	Bool dst_dir_exists = True ;

	dest_file = safemalloc( strlen( dest_dir ) + 1 + strlen(syntax->doc_path) + 5+1 );
	sprintf( dest_file, "%s/%s.html", dest_dir, syntax->doc_path ); 
	ptr = dest_file; 
	while( *ptr == '/' ) ++ptr ;
	ptr = strchr( ptr, '/' );
	while( ptr != NULL )
	{
		*ptr = '\0' ;
		if( CheckDir(ptr) != 0 )
			if( !make_doc_dir( ptr ) ) 
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
	/* HTML HEADER ***********************************************************************/
	fprintf( dest_fp, "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
					  "<html>\n"
					  "<head><meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\">\n"
  					  "<title>%s</title>\n"
					  "</head>\n"
					  "<body>\n"
					  "<h1>%s</h1>\n", syntax->display_name, syntax->display_name );
	/* HTML BODY *************************************************************************/
	{
		int i ;
		char *syntax_dir ;
		syntax_dir = make_file_name( source_dir, syntax->doc_path );
		for( i = 0 ; i < OPENING_PARTS_END ; ++i ) 
			convert_source_file( syntax_dir, StandardSourceEntries[i], dest_fp, DocType_HTML );
		for (i = 0; syntax->terms[i].keyword; i++)
		{	
			if (syntax->terms[i].sub_syntax)
				make_syntax_html( source_dir, dest_dir, syntax->terms[i].sub_syntax );
			if( isalnum( syntax->terms[i].keyword[0] ) )					
				convert_source_file( syntax_dir, syntax->terms[i].keyword, dest_fp, DocType_HTML );
		}
		for( i = OPENING_PARTS_END ; StandardSourceEntries[i]; ++i ) 
			convert_source_file( syntax_dir, StandardSourceEntries[i], dest_fp, DocType_HTML );
		free( syntax_dir );
	}
	/* HTML FOOTER ***********************************************************************/
	fprintf( dest_fp, "</body>\n</html>\n" );		
	fclose( dest_fp );
}
