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

	LOCAL_DEBUG_OUT("parm = \"%s\"", sect->parm?sect->parm:"(NULL)" );

	xml_state->curr_section = sect ;

	if( strlen(id) > 0 ) 
	{
		title = ADD_TAG(sect,title);  
		append_CDATA_line( title, id, strlen(id) );
		LOCAL_DEBUG_OUT("title = %p", title );
	}
	
	return sect;		   
}

xml_elem_t*
add_file_info_li( xml_elem_t* varlist, const char *term)
{
	xml_elem_t* entry ;
	xml_elem_t* entry_term ;
	xml_elem_t* entry_li ;
		
	entry = ADD_TAG(varlist,varlistentry);
	entry_term = ADD_TAG(entry,term);
	append_CDATA_line( entry_term, term, strlen(term) );
	entry_li = ADD_TAG(entry,listitem);
	return entry_li;
}	   

void
add_file_info_item( xml_elem_t* varlist, const char *term, const char *info )
{
	xml_elem_t* entry_li = add_file_info_li( varlist, term);
	xml_elem_t* entry_p ;
		
	entry_p = ADD_TAG(entry_li,para);
	append_CDATA_line( entry_p, info, strlen(info) );
}	   

void 
make_data_file_info( ASImageListEntry *df, ASXMLInterpreterState *parent_state, Bool preview_saved )
{
#if 1
	ASXMLInterpreterState state;
	ASData2xmlState		  xml_state ;

	xml_elem_t* varlist ;
	xml_elem_t* ptr ;

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
 	
	if( !start_doc_file( parent_state->dest_dir, df->name, NULL, parent_state->doc_type, df->name, df->name, df->fullfilename, &state, DOC_CLASS_None, DocClass_Overview ) )	  	 
			return ;

	memset( &xml_state, 0x00, sizeof(xml_state));
	xml_state.doc = xml_elem_new();
	xml_state.doc->tag = mystrdup(XML_CONTAINER_STR) ;
	xml_state.doc->tag_id = XML_CONTAINER_ID ;
	
	LOCAL_DEBUG_OUT( "doc = %p, doc->child = %p", xml_state.doc, xml_state.doc->child );

	/* generate file contents : */
	add_data_section_header2xml( &xml_state, "Details" );
	varlist = ADD_TAG(xml_state.curr_section,variablelist);

	if( df->preview )
	{	
		char tmp[64];
		sprintf( &tmp[0], "%dx%d", df->preview->width, df->preview->height );
		add_file_info_item( varlist, "Size : ", &tmp[0] );
	}
	add_file_info_item( varlist, "Full path : ", df->fullfilename );
	if( df->type < ASIT_Unknown)
		add_file_info_item( varlist, "Type : ", img_type_names[df->type]);
	
	
	if( preview_saved )
	{
		xml_elem_t* entry_li = add_file_info_li( varlist, "Preview : ");
		xml_elem_t*	medobj = ADD_TAG(entry_li,mediaobject);
		xml_elem_t*	imgobj = ADD_TAG(medobj,imageobject);
		xml_elem_t*	imgdat = ADD_TAG(imgobj,imagedata);

		imgdat->parm = safemalloc( 28+strlen(df->name)+32);
		sprintf( imgdat->parm, "fileref=\"%s.png\"", df->name );
	}
	
	if( df->type == ASIT_XMLScript || !preview_saved )
	{
		char *doc_str = load_file(df->fullfilename);
		if( doc_str )
		{	
			xml_elem_t*	credits_section;
			xml_elem_t*	litlayout;
			
			if( df->type == ASIT_XMLScript )
			{
				credits_section = add_data_section_header2xml( &xml_state, 
															   (strstr( doc_str,"<body>")!=NULL)?"HTML document : ":"XML text : ");		   
			}else		  
				credits_section = add_data_section_header2xml( &xml_state, "ASCII text: ");		   
			
			litlayout = ADD_TAG(credits_section,literallayout);
			append_CDATA_line( litlayout, doc_str, strlen(doc_str) );
			free( doc_str );
		}
	}

	/* now lets write it out into file : */
	for (ptr = xml_state.doc->child ; ptr ; ptr = ptr->next) 
		convert_xml_tag( ptr, NULL, &state );
	
	end_doc_file( &state );	 	  
	LOCAL_DEBUG_OUT( "xml_elem_delete for doc %p", xml_state.doc );
	xml_elem_delete(NULL, xml_state.doc);
#endif
}

int
data_fname_filter (const char *d_name)
{
	int name_len = strlen( d_name );
	if( d_name[0] == '.' ||
		(name_len >= 7 && strcmp(d_name+name_len-7, ".tar.gz") == 0) ||
		(name_len >= 5 && strcmp(d_name+name_len-5, ".mini") == 0) ||
		(name_len >= 5 && strcmp(d_name+name_len-5, ".html") == 0))
		return False;
	printf( "\b%c", d_name[0] );
	fflush( stdout );
	return True;
}



void 
convert_data_file( const char *source_dir, const char *dst_dir, ASXMLInterpreterState *state, ASData2xmlState *xml_state)
{
	int count = 0;
	Bool files_sect_added = False ;
	ASImageListEntry *curr ;
	ASImageListEntry *im_list = get_asimage_list( Scr.asv, source_dir, LOAD_PREVIEW, 
												  SCREEN_GAMMA, 0, 0,
												  0, &count, data_fname_filter );

	LOCAL_DEBUG_OUT( "im_list = %p, count = %d", im_list, count );
	
	printf( "\b" );
	fflush( stdout );
	   
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

/*			printf( "[%s]", curr->name );
			fflush(stdout);
*/
			if( !files_sect_added )
			{	
				add_data_section_header2xml( xml_state, "Files" );
				files_sect_added = True ;
			}
			
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
			link->parm = safemalloc( 6+name_len+32);
			sprintf( link->parm, "url=\"%s\"", curr->name );
			
			make_data_file_info( curr, state, preview_saved );

			LOCAL_DEBUG_OUT( "%s", link->parm );
			
			printf( "." );
			fflush(stdout);

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
				free( doc_str );
			}
			break;
		}
		curr = curr->next ;
	}

	curr = im_list ;
	while( curr )
	{
		ASImageListEntry *to_delete = curr ; 
		curr = curr->next ;
		if( to_delete->preview ) 
			safe_asimage_destroy( to_delete->preview );
		if( to_delete->name )
			free( to_delete->name );
		if( to_delete->fullfilename )
			free( to_delete->fullfilename );
		free( to_delete );
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
		if (S_ISDIR (list[i]->d_mode) && strcasecmp(list[i]->d_name, "doc") != 0 )
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
		free(list[i]);
	}
	free( list );   
	printf( "\nCataloguing %s  ", source_dir );
	fflush( stdout );

	convert_data_file( source_dir, dest_dir, &state, &xml_state );
	
	/*xml_print(xml_state.doc); */

	for (ptr = xml_state.doc->child ; ptr ; ptr = ptr->next) 
		convert_xml_tag( ptr, NULL, &state );
	end_doc_file( &state );	 	  
	
	LOCAL_DEBUG_OUT( "xml_elem_delete for doc %p", xml_state.doc );
	xml_elem_delete(NULL, xml_state.doc);
	free( short_fname );
}


