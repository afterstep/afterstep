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
	"_examples",	   
	"_related",	   
	"_footnotes",	   
	NULL
};

void check_syntax_source( const char *source_dir, SyntaxDef *syntax );
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
	/* Save our program name - for error messages */
    InitMyApp (CLASS_ASDOCGEN, argc, argv, NULL, NULL, 0 );

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
		check_syntax_source( source_dir, TopLevelSyntaxes[i] );
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
