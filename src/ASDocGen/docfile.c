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
#include "docfile.h"

const char *HTMLHeaderFormat = "<A name=\"page_top\"></A>\n"
							   "<A href=\"index.html\">%s</A>&nbsp;&nbsp;<A href=\"Glossary.html\">%s</A><p>\n" ;

const char *HTMLHeaderFormatAPI = "<A name=\"page_top\"></A>\n"
							   "<A href=\"../index.html\">Main index</A>"
							   "<A href=\"index.html\">%s</A>&nbsp;&nbsp;<A href=\"Glossary.html\">%s</A><p>\n" ;

const char *HTMLHeaderFormatNoIndex = "<A name=\"page_top\"></A>\n"
							   "<A href=\"%sindex.html\">Go back</A><p>\n" ;

/*************************************************************************/
void 
write_doc_header( ASXMLInterpreterState *state )
{
	char *css = NULL ;
	int i ;
		
	++(state->header_depth);
	switch( state->doc_type ) 
	{
		case DocType_Plain :
			fprintf( state->dest_fp, "%s\n\n", state->display_name );
			break;
		case DocType_HTML :
			fprintf( state->dest_fp, "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
					  		"<html>\n"
					  		"<head><meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\">\n"
  					  		"<title>%s</title>\n", state->display_name );

			css = load_file( HTML_CSS_File );
			if( css  ) 
			{
				int len = strlen( css );
				if( len > 0 ) 
					fwrite( css, 1, len, state->dest_fp );	   
			}	 
			
			if( CurrHtmlBackFile != NULL ) 
				fprintf( state->dest_fp, "</head>\n<body BACKGROUND=\"%s\">\n", CurrHtmlBackFile );
			else
				fprintf( state->dest_fp, "</head>\n<body>\n" );

			if( TopicIndexName == NULL )
				fprintf( state->dest_fp, HTMLHeaderFormatNoIndex, (strcmp(state->doc_name,"index") == 0)?"../":"" );
			else if( TopicIndexName == APITopicIndexName )
				fprintf( state->dest_fp, HTMLHeaderFormatAPI, TopicIndexName, GlossaryName );
			else
				fprintf( state->dest_fp, HTMLHeaderFormat, TopicIndexName, GlossaryName );				

			if( state->display_purpose[0] != '\0' )
				fprintf( state->dest_fp, "<h1>%s</h1><font size=4>%s</font><hr>\n", state->display_name, state->display_purpose );
			else
				fprintf( state->dest_fp, "<h1>%s</h1>\n", state->display_name );

			break;
 		case DocType_PHP :	
			fprintf( state->dest_fp, PHPXrefFormat, "visualdoc","Index","visualselect", "" );

/*			if( TopicIndexName == APITopicIndexName )
				fprintf( state->dest_fp, PHPXrefFormat, "visualdoc","Main Index","index", "" );
 */			
			if( state->doc_class != DocClass_TopicIndex )
				fprintf( state->dest_fp, PHPXrefFormat, "visualdoc",
						 TopicIndexName,
						 (TopicIndexName == APITopicIndexName)?"API/index":"index", "" );
			else
				fprintf( state->dest_fp, PHPCurrPageFormat, TopicIndexName );
			
			if( state->doc_class != DocClass_Glossary )
				fprintf( state->dest_fp, PHPXrefFormat, "visualdoc",GlossaryName,
						(GlossaryName == APIGlossaryName)?"API/Glossary":"Glossary", "" );
			else
				fprintf( state->dest_fp, PHPCurrPageFormat, GlossaryName );

			fprintf( state->dest_fp, PHPXrefFormat, "visualdoc","F.A.Q.","faq", "" );
			fprintf( state->dest_fp, PHPXrefFormat, "visualdoc","Copyright","authors", "" );
			fprintf( state->dest_fp, "<hr>\n" );
fprintf( state->dest_fp, "<br><b>%s</b><br><br><hr>\n", state->display_name );
			for( i = 0 ; i < DocClass_TopicIndex ; ++i ) 
			{
				if( get_flags( state->doc_class_mask, (0x01<<i)	) )
				{
					if( state->doc_class != i )
					{
						if( state->doc_class == DocClass_Overview ) 
							fprintf( state->dest_fp, PHPXrefFormatSetSrc, "visualdoc",DocClassStrings[i][0], state->doc_name, DocClassStrings[i][1], state->doc_name );
						else
							fprintf( state->dest_fp, PHPXrefFormat, "visualdoc",DocClassStrings[i][0], state->doc_name, DocClassStrings[i][1] );
					}else
						fprintf( state->dest_fp, PHPCurrPageFormat, DocClassStrings[i][0] );
				}	 
			}	 
			fprintf( state->dest_fp, "<hr>\n" );
			break ;
		case DocType_XML :
			fprintf( state->dest_fp, "<!DOCTYPE article PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\"\n"
                  					 "\"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\" [\n"
									 "<!ENTITY %s SYSTEM \"%s.xml\" NDATA SGML>\n"
									 "]>\n", state->doc_name, state->doc_name );
			fprintf( state->dest_fp, "<article id=\"index\">\n"
									 "<articleinfo>\n"
   									 "<authorgroup>\n"
      								 "<corpauthor>\n"
      								 "<ulink url=\"http://www.afterstep.org\">AfterStep Window Manager</ulink>\n"
      								 "</corpauthor>\n"
      								 "</authorgroup>\n");
			if( state->display_purpose[0] != '\0' )
				fprintf( state->dest_fp, "<title>%s</title>\n", state->display_name );
			else
				fprintf( state->dest_fp, "<title>%s - %s</title>\n", state->display_name, state->display_purpose );
			fprintf( state->dest_fp, "<releaseinfo>%s</releaseinfo>\n"
									 "</articleinfo>\n", VERSION );
			break ;
		case DocType_NROFF :
			fprintf( state->dest_fp, ".\\\" t\n"
									 ".\\\" @(#)%s.%d		%s\n", state->doc_name, CurrentManType, CurrentDateShort);
			fprintf( state->dest_fp, ".TH %s 1 \"AfterStep v.%s\" \"%s\" \"AfterStep X11 window manager\"\n", state->doc_name, VERSION, CurrentDateLong );
			fprintf( state->dest_fp, ".UC\n"
									 ".SH NAME\n"
									 "\\fB%s\\fP", state->doc_name );
			if( state->display_purpose[0] != '\0' )
				fprintf( state->dest_fp, "\\ - %s", state->display_purpose );
			fputc( '\n', state->dest_fp );
		    break ;
		default:
			break;
	}
}

void 
write_options_header( ASXMLInterpreterState *state )
{
	++(state->header_depth);
	switch( state->doc_type ) 
	{
		case DocType_Plain :
			fputs( "\nCONFIGURATION OPTIONS : \n", state->dest_fp );
			break;
		case DocType_HTML :
			if( state->pre_options_size > 1024 )
				fprintf( state->dest_fp,"<p><A href=\"index.html\">%s</A>&nbsp;&nbsp;<A href=\"Glossary.html\">%s</A>&nbsp;&nbsp;<A href=\"#page_top\">Back to Top</A><hr>\n", 
						 TopicIndexName, GlossaryName );
			fprintf( state->dest_fp,"\n<UL><LI><A NAME=\"options\"></A><h3>CONFIGURATION OPTIONS :</h3>\n"
							  		"<DL>\n");   
			break;
 		case DocType_PHP :	
			fprintf( state->dest_fp, "\n<LI><A NAME=\"options\"></A><B>CONFIGURATION OPTIONS :</B><P>\n"
							  "<DL>\n");   
		    break ;
		case DocType_XML :
			fprintf( state->dest_fp, "\n<section label=\"options\" id=\"options\"><title>CONFIGURATION OPTIONS :</title>\n" );
			break;
		case DocType_NROFF :
			fprintf( state->dest_fp, "\n.SH CONFIGURATION OPTIONS\n" );
		    break ;
		default:
			break;
	}
}

void 
write_options_footer( ASXMLInterpreterState *state )
{
	switch( state->doc_type ) 
	{
		case DocType_Plain :
			fprintf( state->dest_fp, "\n");
			break;
		case DocType_HTML :
			fprintf( state->dest_fp, "\n</DL></P></LI>\n</UL>\n");
			break;
 		case DocType_PHP :	                   
			fprintf( state->dest_fp, "\n</DL></P></LI>\n");   
		    break ;
		case DocType_XML :
			fprintf( state->dest_fp, "\n</section>\n" );
			break ;
		case DocType_NROFF :
			/* not needed */
		    break ;
		default:
			break;
	}
	--(state->header_depth);
}


void 
write_doc_footer( ASXMLInterpreterState *state )
{
	switch( state->doc_type ) 
	{
		case DocType_Plain :
			break;
		case DocType_HTML :
			if( TopicIndexName != NULL )
				fprintf( state->dest_fp,"<p><A href=\"index.html\">Topic index</A>&nbsp;&nbsp;<A href=\"Glossary.html\">Glossary</A>&nbsp;&nbsp;<A href=\"#page_top\">Back to Top</A>\n"); 
			fprintf( state->dest_fp, 
					"<hr>\n<p><FONT face=\"Verdana, Arial, Helvetica, sans-serif\" size=\"-2\">AfterStep version %s</a></FONT>\n"
					"</body>\n</html>\n", VERSION );			   
			break;
 		case DocType_PHP :	
		    break ;
		case DocType_XML :
			fprintf( state->dest_fp, "\n</article>\n" );
		    break ;
		case DocType_NROFF :
			/* not needed */
		    break ;
		default:
			break;
	}
	--(state->header_depth);
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

Bool
start_doc_file( const char * dest_dir, const char *doc_path, const char *doc_postfix, ASDocType doc_type, 
                const char *doc_name, const char *display_name, const char *display_purpose, 
				ASXMLInterpreterState *state, 
				ASFlagType doc_class_mask, ASDocClass doc_class )
{
	char *dest_file = safemalloc( strlen(doc_path)+(doc_postfix?strlen(doc_postfix):0)+5+1 );
	char *dest_path;
	char *ptr ;
	FILE *dest_fp ;
	Bool dst_dir_exists = True;

	sprintf( dest_file, "%s%s.%s", doc_path, doc_postfix?doc_postfix:"", ASDocTypeExtentions[doc_type] ); 
	dest_path = make_file_name( dest_dir, dest_file );			  
	LOCAL_DEBUG_OUT( "starting doc \"%s\"", dest_path );	
	ptr = dest_path; 
	while( *ptr == '/' ) ++ptr ;
	ptr = strchr( ptr, '/' );
	while( ptr != NULL )
	{
		*ptr = '\0' ;
		if( CheckDir(dest_path) != 0 )
			if( !make_doc_dir( dest_path ) ) 
			{
		 		dst_dir_exists = False ;
				break;
			}
 		*ptr = '/' ;
		ptr = strchr( ptr+1, '/' );
	}
	if( !dst_dir_exists ) 
	{
		free( dest_path );	  
		free( dest_file );	  
		return False ;
	}
	
	if( DocGenerationPass > 0 ) 
		dest_fp = fopen( "/dev/null", "wt" );
	else 
	{	
		dest_fp = fopen( dest_path, "wt" );
		chmod (dest_path, 0644);
		if( CurrHtmlBackFile ) 
		{
			char * back_dst = make_file_name( dest_dir, CurrHtmlBackFile );			  
			copy_file( CurrHtmlBackFile, back_dst );
			free( back_dst );
		}
	}
	if( dest_fp == NULL ) 
	{
		show_error( "Failed to open destination file \"%s\" for writing!", dest_path );
		free( dest_path );
		free( dest_file );
		return False;
	}				   

	memset( state, 0x00, sizeof(ASXMLInterpreterState));
	state->flags = ASXMLI_FirstArg ;
	state->doc_name = doc_path?doc_path:AfterStepName ;
	state->display_name = display_name?display_name:AfterStepName ;
	state->display_purpose = display_purpose?display_purpose:"X11 window manager" ;
	state->dest_fp = dest_fp ;
	state->dest_file = dest_file ;
	state->dest_dir = dest_dir ;
	state->doc_type = doc_type ; 
	state->doc_class_mask = (doc_class_mask==0)?0xFFFFFFFF:doc_class_mask;
	state->doc_class = doc_class ;
	if( state->doc_class == DocClass_Glossary ) 
	{	
		state->display_name = GlossaryName ; 
		state->display_purpose = "" ;
	}else if( state->doc_class == DocClass_TopicIndex ) 
	{	
		state->display_name = TopicIndexName ; 
		state->display_purpose = "" ;
	}


	{
		int index_name_len = strlen(state->display_name)+1;
		char *index_name ;
		if( doc_postfix && doc_postfix[0] != '\0' )
			index_name_len += 1+strlen(doc_postfix);
		index_name = safemalloc( index_name_len );
		if( doc_postfix && doc_postfix[0] != '\0' )
		{
			int i ;
			sprintf( index_name, "%s%s", state->display_name, doc_postfix );
			for( i = 0 ; index_name[i] ; ++i ) 
				if( index_name[i] == '_' )
					index_name[i] = ' ';
		}else
			strcpy( index_name, state->display_name );

		if( strcmp(index_name, "Topic index") != 0 && 
			strcmp(index_name, "Glossary") != 0 && 
			strncmp( index_name, "API ", 4 ) != 0 )
   			add_hash_item( Index, AS_HASHABLE(index_name), (void*)mystrdup(dest_file) );   
	}
	/* HEADER ***********************************************************************/
	write_doc_header( state );
	free( dest_path );	
	LOCAL_DEBUG_OUT( "File opened with fptr %p", state->dest_fp );
	return True;
}

void
end_doc_file( ASXMLInterpreterState *state )	
{
	if( state->dest_fp );
	{
		write_doc_footer( state );	
		fclose( state->dest_fp );
	}
	if( state->dest_file ) 
	{
		free( state->dest_file );
		state->dest_file = NULL ;
	}	 

	memset( &state, 0x00, sizeof(state));
}



