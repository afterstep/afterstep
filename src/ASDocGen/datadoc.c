/*
 * Copyright (c) 2003, 2004 Sasha Vasko <sasha@aftercode.net>
 *
 * This module is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

/*#define DO_CLOCKING      */
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
#include "docfile.h"
#include "datadoc.h"

/* <li><b><A href="graphics.php?inc=icons/normal/index">normal</A></b></li> */

typedef struct ASData2xmlState
{
	ASFlagType flags ;

	xml_elem_t* doc ;
	xml_elem_t* curr_section ;

}ASData2xmlState;


#if 0
struct
{
	char *dir ;
	char *html_save, *html_save_dir ;
	char *html_back  ;
	char *link_format ;
}ASIMBrowserState;

void generate_dir_html();

char *default_html_save = "index.html";
char *default_html_save_dir = "html";
char *default_html_back = "background.jpg";

int main(int argc, char** argv)
{
	int i;

	memset( &ASIMBrowserState, 0x00, sizeof(ASIMBrowserState ));
	ASIMBrowserState.html_save = default_html_save;
	ASIMBrowserState.html_save_dir = default_html_save_dir;
	ASIMBrowserState.html_back = default_html_back;

	if( ASIMBrowserState.link_format != NULL ) 
	{
		int i = 0;
		char *ptr = ASIMBrowserState.link_format;
		do
		{
			ptr = strchr( ptr, '%' );	
			if( ptr )
			{
				++ptr ;
				++i ;
			}
		}while( ptr != NULL );	
		if( i > 2 ) 
			ASIMBrowserState.link_format = NULL ;	 
	}
	if( ASIMBrowserState.link_format == NULL ) 
		ASIMBrowserState.link_format = "<li><b><A href=\"%s/%s\">%s</A></b></li>"; 


	generate_dir_html(ASIMBrowserState.dir, ASIMBrowserState.html_save_dir);
	return 0;
}

void generate_dir_html( char *dir, char *html_dir )
{
	FILE *of = stdout ;
	int count = 0;
	ASImageListEntry *curr ;
	int im_no = 0 ;
	struct direntry  **list;
	int list_len, i ;
	Bool dir_header_printed = False ;


	ASImageListEntry *im_list = get_asimage_list( Scr.asv, dir,
	              						     LOAD_PREVIEW, Scr.image_manager->gamma, 0, 0,
											 0, &count );

	if( ASIMBrowserState.html_save )
	{
		char *html_save = safemalloc( (html_dir?strlen( html_dir )+1:0)+ strlen(ASIMBrowserState.html_save)+1 );
		if( html_dir )
			sprintf( html_save, "%s/%s", html_dir, ASIMBrowserState.html_save );
		else
			strcpy( html_save, ASIMBrowserState.html_save);

		if( (of = fopen( html_save, "wb" )) == NULL )
		{
			show_error( "failed to open output file \"%s\"", html_save );
			exit(1);
		}
		free( html_save );
	}
	fprintf( of, "<HTML><HEAD><title>%s</title></head>", dir );
	fprintf( of, "<body BACKGROUND=\"%s\" BGCOLOR=#34404C TEXT=#F5F8FAA LINK=#8888FF VLINK=#88FF88>", ASIMBrowserState.html_back );
	fprintf( of, "<font color=#F5F8FA><h2>%s</h2>", dir );
	fprintf( of, "<h4>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<A href=\"../%s\">Go Back</A></h4>\n", ASIMBrowserState.html_save);

	list_len = my_scandir ((char*)dir, &list, ignore_dots, NULL);
	for (i = 0; i < list_len; i++)
		if (S_ISDIR (list[i]->d_mode))
		{
			char * subdir = make_file_name( dir, list[i]->d_name );
			char * html_subdir = html_dir?make_file_name( html_dir, list[i]->d_name ):mystrdup(list[i]->d_name);
			char * back_src = make_file_name( html_dir?html_dir:"./", ASIMBrowserState.html_back );
			char * back_dst = make_file_name( html_subdir, ASIMBrowserState.html_back );

			if( !dir_header_printed )
			{
				fprintf( of, "<h3>Subdirectories : </h3>\n<ul>\n");
				dir_header_printed = True ;
			}
			fprintf( of, "<li><b><A href=\"%s/%s\">%s</A></b></li>\n", list[i]->d_name, ASIMBrowserState.html_save, list[i]->d_name);
			CheckOrCreate (html_subdir);
			copy_file( back_src, back_dst );
			free( back_src );
			free( back_dst );
			generate_dir_html( subdir, html_subdir );
			free( subdir );
			free( html_subdir );
		}
	if( dir_header_printed ) 
		fprintf( of, "</ul>\n");
	curr = im_list ;
	while( curr )
	{
		if( mystrcasecmp(curr->name, "CREDITS") != 0 )
		{
			int name_len = strlen( curr->name );
			int odir_name_len = html_dir?strlen( html_dir )+1:0;
			char *preview_name = safemalloc( odir_name_len + name_len + 1 + 3 + 1 );
			char *ext_name = safemalloc(odir_name_len + name_len + 1 + 4 + 1 );
			Bool preview_saved = False ;
			FILE *frame_of ;

			if( odir_name_len > 0 )
			{
				sprintf( preview_name, "%s/%s.png", html_dir, curr->name );
				sprintf( ext_name, "%s/%s.html", html_dir, curr->name );
			}else
			{
				strcpy( preview_name, curr->name );
				strcpy( ext_name, curr->name );
				strcpy( &(ext_name[name_len]), ".html" );
				strcpy( &(preview_name[name_len]), ".png" );
			}
			if( curr->preview )
			{
				ASImageExportParams params ;
				if( curr->type != ASIT_Jpeg &&
					get_flags( get_asimage_chanmask(curr->preview), SCL_DO_ALPHA) &&
					curr->preview->width < 200 && curr->preview->height < 200 )
				{
					params.png.flags = EXPORT_ALPHA ;
					params.png.compression = 6 ;
					params.png.type = ASIT_Png ;
					preview_saved = ASImage2file( curr->preview, NULL, preview_name,ASIT_Png, &params );
				}else
				{
					params.jpeg.flags = EXPORT_ALPHA ;
					params.jpeg.quality = 100 ;
					params.jpeg.type = ASIT_Jpeg ;
					preview_saved = ASImage2file( curr->preview, NULL, preview_name,ASIT_Jpeg, &params );
				}

				if( !preview_saved )
					show_warning( "failed to save \"%s\" as png preview - skipping", curr->name );
			}
			if( preview_saved )
				fprintf( of, "<A href=\"%s.html\"><IMG ALT=\"%s\" SRC=\"%s.png\"></A>\n", curr->name, curr->name, curr->name );
			else
				fprintf( of, "<br>&nbsp;&nbsp;&nbsp;&nbsp;<A href=\"%s.html\">%s</A><br>\n", curr->name, curr->name );

			frame_of = fopen( ext_name, "wb" );
			if( frame_of )
			{
				Bool valid_html = False ;
				static char *img_type_names[ASIT_Unknown+1] =
				{
					"Xpm",
					"ZCompressedXpm",
					"GZCompressedXpm",
					"Png",
					"Jpeg",
					"Xcf",
					"Ppm",
					"Pnm",
					"Bmp",
					"Ico",
					"Cur",
					"Gif",
					"Tiff",
					"XML",
					"Xbm",
					"Targa",
					"Pcx",
					"Unknown"
				};
				fprintf( frame_of, "<HTML>\n<HEAD><TITLE>%s</TITLE></HEAD>\n", curr->name );
				fprintf( frame_of, "<BODY BACKGROUND=\"%s\" BGCOLOR=#34404C TEXT=#F5F8FA LINK=#8888FF VLINK=#88FF88>\n", ASIMBrowserState.html_back );
				fprintf( frame_of, "<h3>%s</h3>\n", curr->name );
				fprintf( frame_of, "<h4>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<A href=\"%s\">Go Back</A></h4>", ASIMBrowserState.html_save);

				if( curr->preview )
					fprintf( frame_of, "<h4>Size :</h4>&nbsp;&nbsp;&nbsp;&nbsp;%dx%d\n", curr->preview->width, curr->preview->height );

				fprintf( frame_of, "<h4>Full path :</h4>&nbsp;&nbsp;&nbsp;&nbsp;%s\n", curr->fullfilename );
				if( preview_saved )
					fprintf( frame_of, "<h4>Preview :</h4>&nbsp;&nbsp;&nbsp;&nbsp;<IMG border=3 ALT=\"%s\" SRC=\"%s.png\">\n", curr->name, curr->name );
				fprintf( frame_of, "<h4>Type :</h4>&nbsp;&nbsp;&nbsp;&nbsp;%s\n", img_type_names[curr->type] );
				if( curr->type == ASIT_XMLScript || !preview_saved )
				{
					FILE *xml_of = fopen( curr->fullfilename, "rb" );
					int c;
					if( curr->type == ASIT_XMLScript )
					{
						if( curr->preview )
							fprintf( frame_of, "<h4>XML text :</h4><PRE>\n" );
						else
						{

							if( xml_of )
							{
								int body_count = -1 ;
								while( (c = fgetc(xml_of)) != EOF && body_count <= 4 )
								{
									if( c == '<' && body_count == -1 )
										++body_count;
									else if( (c == 'b' || c == 'B') && body_count == 0 )
										++body_count;
									else if( (c == 'o' || c == 'O') && body_count == 1 )
										++body_count;
									else if( (c == 'd' || c == 'D') && body_count == 2 )
										++body_count;
									else if( (c == 'y' || c == 'Y') && body_count == 3 )
										++body_count;
									else if(  c == '>' && body_count == 4 )
										++body_count;
									else if( !isspace( c ) || (body_count > 0 && body_count < 4) )
									{
										body_count = -1 ;
									}
								}
								if( body_count == 5 )
								{
									valid_html = True ;
									fprintf( frame_of, "<h4>HTML document :</h4>\n" );
								}else
								{
									fprintf( frame_of, "<h4>XML text :</h4><PRE>\n" );
									fseek( xml_of, 0, SEEK_SET );
								}
							}
						}
					}else
						fprintf( frame_of, "<h4>ASCII text :</h4><PRE>\n" );
					if( xml_of )
					{

	   					while( (c = fgetc(xml_of)) != EOF )
						{
							if( valid_html )
								fputc( c, frame_of );
							else if( !isascii( c ) )
								fprintf( frame_of, "#%2.2X;", c );
							else if( c == '<' )
								fprintf( frame_of, "&lt;" );
							else if( c == '>' )
								fprintf( frame_of, "&gt;" );
							else
								fputc( c, frame_of );
						}
						fclose( xml_of );
					}else
						fprintf( frame_of, "error: failed to open xml file!\n" );
					if( !valid_html )
						fprintf( frame_of, "</PRE>\n" );
				}
				if( !valid_html )
					fprintf( frame_of, "\n</body></HTML>\n");
				fclose( frame_of );
			}
			free( preview_name );
			free( ext_name );
		}
		curr = curr->next ;
		++im_no ;
	}
	curr = im_list ;
	while( curr )
	{
		if( mystrcasecmp(curr->name, "CREDITS") == 0)
		{
			FILE *credits_of = fopen( curr->fullfilename, "rb" );
			int c;
			fprintf( of, "<h4>CREDITS :</h4><PRE>\n" );
			if( credits_of )
			{
				while( (c = fgetc(credits_of)) != EOF )
				{
					if( c == '<' )
						fprintf( of, "&lt;" );
					else if( c == '>' )
						fprintf( of, "&gt;" );
					else
						fputc( c, of );
				}
				fclose( credits_of );
			}else
				fprintf( of, "error: failed to open CREDITS file!\n" );
			fprintf( of, "</PRE>\n" );
		}
		curr = curr->next ;
	}
	fprintf( of, "\n</body></HTML>");

	if( of != stdout )
		fclose( of );
}

#endif

xml_elem_t*
add_tag( xml_elem_t* owner, const char *tag_str, int tag_id )
{
	xml_elem_t* t = xml_elem_new() ;
	t->tag = mystrdup( tag_str );
	t->tag_id = tag_id ;  
	LOCAL_DEBUG_OUT( "tag = \"%s\", tag_id = %d", t->tag, t->tag_id );
	if( owner != NULL ) 
		xml_insert(owner, t);
	LOCAL_DEBUG_OUT( "added to owner = %p", owner );
	return t;
}

#define ADD_TAG(o,t)    add_tag( o, #t, DOCBOOK_##t##_ID)

xml_elem_t*
add_data_section_header2xml( ASData2xmlState *xml_state, const char *id )
{
	xml_elem_t* sect = NULL;
	xml_elem_t* title = NULL ;
	
	LOCAL_DEBUG_OUT("id = \"%s\", owner = %p, owner->child = %p", id, xml_state->doc, xml_state->doc->child );

	sect = ADD_TAG(xml_state->doc,section);  
 
	LOCAL_DEBUG_OUT("section=%p", sect );

	sect->parm = safemalloc( 4+strlen(id)+32+1 );
	sprintf(sect->parm, "id=\"%s\"", id );

	LOCAL_DEBUG_OUT("parm = \"%s\"", sect->parm );

	xml_state->curr_section = sect ;

	if( strlen(id) > 0 ) 
	{
		title = ADD_TAG(sect,title);  
		append_CDATA_line( title, id, strlen(id) );
		LOCAL_DEBUG_OUT("title = %p", title );
	}
	
	return sect;		   
}



void 
convert_data_file( const char *source_dir, const char *dst_dir, ASXMLInterpreterState *state, ASData2xmlState *xml_state)
{
	int count = 0;
	ASImageListEntry *curr ;
	ASImageListEntry *im_list = get_asimage_list( Scr.asv, source_dir, LOAD_PREVIEW, 
												  SCREEN_GAMMA, 0, 0,
												  0, &count );

	LOCAL_DEBUG_OUT( "im_list = %p, count = %d", im_list, count );
	add_data_section_header2xml( xml_state, "Files" );

	curr = im_list ;
	while( curr )
	{
		LOCAL_DEBUG_OUT( "curr = %p", curr );
		LOCAL_DEBUG_OUT( "curr->name = \"%s\"", curr->name );


		if( mystrcasecmp(curr->name, "CREDITS") != 0 )
		{
			int name_len = strlen( curr->name );
			int odir_name_len = dst_dir?strlen( dst_dir )+1:0;
			char *preview_name = safemalloc( odir_name_len + name_len + 1 + 3 + 1 );
			char *ext_name = safemalloc(odir_name_len + name_len + 1 + 4 + 1 );
			Bool preview_saved = False ;
			
			xml_elem_t*	link = NULL ;
			
			LOCAL_DEBUG_OUT( "curr->name = \"%s\", curr->preview = %p", curr->name, curr->preview );
			if( odir_name_len > 0 )
			{
				sprintf( preview_name, "%s/%s.png", dst_dir, curr->name );
				sprintf( ext_name, "%s/%s", dst_dir, curr->name );
			}else
			{
				strcpy( preview_name, curr->name );
				strcpy( ext_name, curr->name );
				strcpy( &(preview_name[name_len]), ".png" );
			}
			if( curr->preview )
			{
				ASImageExportParams params ;
				if( curr->type != ASIT_Jpeg &&
					get_flags( get_asimage_chanmask(curr->preview), SCL_DO_ALPHA) &&
					curr->preview->width < 200 && curr->preview->height < 200 )
				{
					params.png.flags = EXPORT_ALPHA ;
					params.png.compression = 6 ;
					params.png.type = ASIT_Png ;
					preview_saved = ASImage2file( curr->preview, NULL, preview_name,ASIT_Png, &params );
				}else
				{
					params.jpeg.flags = EXPORT_ALPHA ;
					params.jpeg.quality = 100 ;
					params.jpeg.type = ASIT_Jpeg ;
					preview_saved = ASImage2file( curr->preview, NULL, preview_name,ASIT_Jpeg, &params );
				}

				if( !preview_saved )
					show_warning( "failed to save \"%s\" as png preview - skipping", curr->name );
			}
			if( preview_saved )
			{	
				link = ADD_TAG(xml_state->curr_section,ulink);

				if( link ) 
				{	
					xml_elem_t*	medobj = ADD_TAG(link,mediaobject);
					xml_elem_t*	imgobj = ADD_TAG(medobj,imageobject);
					xml_elem_t*	imgdat = ADD_TAG(imgobj,imagedata);

					imgdat->parm = safemalloc( 28+name_len+32);
					sprintf( imgdat->parm, "fileref=\"%s.png\"", curr->name );
				}
			}else
			{	
				xml_elem_t*	p = ADD_TAG(xml_state->curr_section,para);
				link = ADD_TAG(p,ulink);
				append_CDATA_line( link, curr->name,name_len );
			}
			link->parm = safemalloc( 7+name_len+32);
			sprintf( link->parm, "href=\"%s\"", curr->name );
			LOCAL_DEBUG_OUT( "%s", link->parm );
	    }
		curr = curr->next ;
	}
	curr = im_list ;
	while( curr )
	{
		if( mystrcasecmp(curr->name, "CREDITS") == 0)
		{
			char *doc_str = load_file(curr->fullfilename);
			if( doc_str )
			{	
				xml_elem_t*	credits_section = add_data_section_header2xml( xml_state, "CREDITS" );
				xml_elem_t*	litlayout = ADD_TAG(credits_section,literallayout);
				
				append_CDATA_line( litlayout, doc_str, strlen(doc_str) );
			}
			break;
		}
		curr = curr->next ;
	}
}


void 
gen_data_doc( const char *source_dir, const char *dest_dir, 
			  const	char *display_name,
			  const char *display_purpose,
			  ASDocType doc_type )
{
	ASXMLInterpreterState state;
	ASData2xmlState		  xml_state ;
	char *slash ;
	char *short_fname ;
	struct direntry  **list;
	int list_len, i ;
	xml_elem_t* dir_header_section = NULL ;
	xml_elem_t* dir_header_varlist = NULL ;
	xml_elem_t* ptr ;

	if( (slash = strrchr( source_dir, '/' )) != NULL )
		short_fname = mystrdup( slash+1 );
	else
		short_fname = mystrdup( source_dir );

	memset( &xml_state, 0x00, sizeof(xml_state) );
 	if( !start_doc_file( dest_dir, "index", NULL, doc_type, short_fname, display_name, display_purpose, &state, DOC_CLASS_None, DocClass_Overview ) )	  	 
			return ;

	xml_state.doc = xml_elem_new();
	xml_state.doc->tag = mystrdup(XML_CONTAINER_STR) ;
	xml_state.doc->tag_id = XML_CONTAINER_ID ;
	
	LOCAL_DEBUG_OUT( "doc = %p, doc->child = %p", xml_state.doc, xml_state.doc->child );
	list_len = my_scandir ((char*)source_dir, &list, ignore_dots, NULL);
	for (i = 0; i < list_len; i++)
	{	
		if (S_ISDIR (list[i]->d_mode))
		{
			char *subdir = make_file_name( source_dir, list[i]->d_name );
			char *dest_subdir = dest_dir?make_file_name( dest_dir, list[i]->d_name ):mystrdup(list[i]->d_name);
			char *subdir_display_name = make_file_name( display_name, list[i]->d_name );
			xml_elem_t* entry ;
			xml_elem_t* entry_term ;
			xml_elem_t* entry_link ;

			if( dir_header_section == NULL )
			{	
				dir_header_section = add_data_section_header2xml( &xml_state, "Subdirectories" );
				dir_header_varlist = ADD_TAG(dir_header_section,variablelist);
			}
			
			entry = ADD_TAG(dir_header_varlist,varlistentry);
			entry_term = ADD_TAG(entry,term);
			entry_link = ADD_TAG(entry_term,ulink);
			entry_link->parm = safemalloc( 12+strlen(list[i]->d_name)+32 );
			sprintf( entry_link->parm,"url=\"%s/index\"", list[i]->d_name );
			append_CDATA_line( entry_link, list[i]->d_name, strlen(list[i]->d_name) );
			
			gen_data_doc( subdir, dest_subdir, 
			  			  subdir_display_name, subdir, doc_type );

			free( subdir );
			free( dest_subdir );
			free( subdir_display_name );
		}
	}
	   
	convert_data_file( source_dir, dest_dir, &state, &xml_state );
	
	for (ptr = xml_state.doc->child ; ptr ; ptr = ptr->next) 
		convert_xml_tag( ptr, NULL, &state );
	end_doc_file( &state );	 	  
	
	xml_elem_delete(NULL, xml_state.doc);
	free( short_fname );
}


