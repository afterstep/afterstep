/* 
 * Copyright (C) 2005 Sasha Vasko <sasha at aftercode.net>
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
 *
 */

#define LOCAL_DEBUG
#include "../configure.h"

#include "../include/afterbase.h"
#include "../libAfterImage/afterimage.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/screen.h"

#include <unistd.h>		   

#include "asgtk.h"
#include "asgtkai.h"
#include "asgtkimagedir.h"
#include "asgtkimageview.h"

/*  local function prototypes  */
static void asgtk_image_dir_class_init (ASGtkImageDirClass *klass);
static void asgtk_image_dir_init (ASGtkImageDir *iv);
static void asgtk_image_dir_dispose (GObject *object);
static void asgtk_image_dir_finalize (GObject *object);
static void asgtk_image_dir_style_set (GtkWidget *widget, GtkStyle  *prev_style);


/*  private variables  */
static GtkScrolledWindowClass *parent_class = NULL;

GType
asgtk_image_dir_get_type (void)
{
  	static GType id_type = 0;

  	if (! id_type)
    {
    	static const GTypeInfo id_info =
      	{
        	sizeof (ASGtkImageDirClass),
        	(GBaseInitFunc)     NULL,
        	(GBaseFinalizeFunc) NULL,
			(GClassInitFunc)    asgtk_image_dir_class_init,
        	NULL,           /* class_finalize */
        	NULL,           /* class_data     */
        	sizeof (ASGtkImageDir),
        	0,              /* n_preallocs    */
        	(GInstanceInitFunc) asgtk_image_dir_init,
      	};

      	id_type = g_type_register_static (	GTK_TYPE_SCROLLED_WINDOW,
        	                                "ASGtkImageDir",
            	                            &id_info, 0);
    }

  	return id_type;
}

static void
asgtk_image_dir_class_init (ASGtkImageDirClass *klass)
{
  	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->dispose   = asgtk_image_dir_dispose;
  	object_class->finalize  = asgtk_image_dir_finalize;

  	widget_class->style_set = asgtk_image_dir_style_set;

}

static void
asgtk_image_dir_init (ASGtkImageDir *id)
{
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (id),
				    				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	id->flags = ASGTK_ImageDir_DefaultFlags ; 
	id->fulldirname = NULL ; 
	id->entries = NULL ;
}

static void
asgtk_image_dir_dispose (GObject *object)
{
  	ASGtkImageDir *id = ASGTK_IMAGE_DIR (object);
	destroy_string( &(id->fulldirname) );
  	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
asgtk_image_dir_finalize (GObject *object)
{
  	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
asgtk_image_dir_style_set (GtkWidget *widget,
                          GtkStyle  *prev_style)
{
  /* ASGtkImageDir *id = ASGTK_IMAGE_DIR (widget); */

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}

static void
asgtk_image_dir_sel_handler(GtkTreeSelection *selection, gpointer user_data)
{
  	ASGtkImageDir *id = ASGTK_IMAGE_DIR(user_data); 
	GtkTreeIter iter;
	GtkTreeModel *model;

  	if (gtk_tree_selection_get_selected (selection, &model, &iter)) 
	{
		gpointer p = NULL ;
    	gtk_tree_model_get (model, &iter, ASGTK_ImageDir_Cols, &p, -1);
		id->curr_selection = (ASImageListEntry*)p;
  	}else
		id->curr_selection = NULL ;
		
	if( id->sel_change_handler )
		id->sel_change_handler( id, id->sel_change_user_data ); 
}

void
asgtk_image_dir2view_sel_handler(ASGtkImageDir *id, gpointer user_data)
{
	ASGtkImageView *iv = ASGTK_IMAGE_VIEW(user_data);
	g_return_if_fail (ASGTK_IS_IMAGE_DIR (id));
	if( iv ) 
		asgtk_image_view_set_entry ( iv, id->curr_selection);
}



/*  public functions  */
GtkWidget *
asgtk_image_dir_new ()
{
	ASGtkImageDir *id;
	GtkTreeSelection *selection;
	const char *column_names[ASGTK_ImageDir_Cols] = {"Name", "Type", "Size(KB)", "MTime","Permission"};
	int i ;
	int default_columns = ASGTK_ImageDir_DefaultFlags&ASGTK_ImageDir_Cols_All ; 
  	
    id = g_object_new (ASGTK_TYPE_IMAGE_DIR, NULL);

	id->tree_view = GTK_TREE_VIEW(gtk_tree_view_new());
	id->tree_model = GTK_TREE_MODEL(gtk_list_store_new (ASGTK_ImageDir_Cols+1, G_TYPE_STRING, 
																			   G_TYPE_STRING, 
																			   G_TYPE_STRING, 
																			   G_TYPE_STRING, 
																			   G_TYPE_STRING, 
																			   G_TYPE_POINTER));

	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (id), GTK_SHADOW_IN);      
	
	gtk_container_add (GTK_CONTAINER(id), GTK_WIDGET(id->tree_view));
    gtk_tree_view_set_model (id->tree_view, id->tree_model);
    gtk_widget_show (GTK_WIDGET(id->tree_view));
	for( i = 0 ; i < ASGTK_ImageDir_Cols ; ++i ) 
	{
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	    id->columns[i] = gtk_tree_view_column_new_with_attributes (column_names[i], renderer, "text", i, NULL);
    }
	clear_flags( id->flags, ASGTK_ImageDir_Cols_All );
	asgtk_image_dir_set_columns( id, default_columns );
	
	selection = gtk_tree_view_get_selection(id->tree_view);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
   	g_signal_connect (selection, "changed",  G_CALLBACK (asgtk_image_dir_sel_handler), id);
	
	colorize_gtk_tree_view_window( GTK_WIDGET(id) );

	LOCAL_DEBUG_OUT( "created image ASGtkImageDir object %p", id );	
	return GTK_WIDGET (id);
}

void  
asgtk_image_dir_set_columns( ASGtkImageDir *id, ASFlagType columns )
{
	int i; 	
	for( i = 0 ; i < ASGTK_ImageDir_Cols ; ++i ) 
	{	
		ASFlagType flag = 0x01<<i ;
		if( get_flags(columns, flag) )
		{	
			gtk_tree_view_insert_column (id->tree_view, GTK_TREE_VIEW_COLUMN (id->columns[i]), i);
			set_flags( id->flags, flag );	
		}else if( get_flags( id->flags, flag ) ) 
		{
			gtk_tree_view_remove_column (id->tree_view, GTK_TREE_VIEW_COLUMN (id->columns[i]));
			clear_flags( id->flags, flag );	
		}	 
	}
}

void  
asgtk_image_dir_set_list_all( ASGtkImageDir *id, Bool enable )
{
	if( enable && get_flags(id->flags, ASGTK_ImageDir_ListAll ) )
		return ;
	if( !enable && !get_flags(id->flags, ASGTK_ImageDir_ListAll ) )
		return ;
	if( enable ) 
		set_flags(id->flags, ASGTK_ImageDir_ListAll );
	else
		clear_flags(id->flags, ASGTK_ImageDir_ListAll );

	asgtk_image_dir_refresh( id );		 		
}


void  
asgtk_image_dir_set_path( ASGtkImageDir *id, char *fulldirname )
{
	g_return_if_fail (ASGTK_IS_IMAGE_DIR (id));
	LOCAL_DEBUG_OUT( "id->fulldirname == \"%s\", fulldirname == \"%s\"", id->fulldirname, fulldirname ) ;
	if( id->fulldirname == NULL && fulldirname == NULL ) 
		return;
	if( id->fulldirname && fulldirname && strcmp(id->fulldirname, fulldirname)== 0  ) 
		return;
	destroy_string( &(id->fulldirname) ); 

	if( fulldirname ) 
		id->fulldirname = mystrdup(fulldirname);
	
	asgtk_image_dir_refresh( id );		 
}	 

void  
asgtk_image_dir_set_mini( ASGtkImageDir *id, char *mini_extension )
{
	g_return_if_fail (ASGTK_IS_IMAGE_DIR (id));
	
	if( id->mini_extension == NULL && mini_extension == NULL ) 
		return;
	if( id->mini_extension && mini_extension && strcmp(id->mini_extension, mini_extension)== 0  ) 
		return;
	destroy_string( &(id->mini_extension) );

	if( mini_extension ) 
		id->mini_extension = mystrdup(mini_extension);
	
	asgtk_image_dir_refresh( id );		 
}	 

void  
asgtk_image_dir_set_title( ASGtkImageDir *id, const gchar *title )
{
	g_return_if_fail (ASGTK_IS_IMAGE_DIR (id));
	gtk_tree_view_column_set_title( id->columns[0], title );	
}

void  
asgtk_image_dir_set_sel_handler( ASGtkImageDir *id, _ASGtkImageDir_sel_handler sel_change_handler, gpointer user_data )
{
	g_return_if_fail (ASGTK_IS_IMAGE_DIR (id));

	id->sel_change_handler = sel_change_handler ; 
	id->sel_change_user_data = user_data ;
	
}

struct ASImageListEntry *
asgtk_image_dir_get_selection(ASGtkImageDir *id )
{
	ASImageListEntry *result = NULL ; 

	if( ASGTK_IS_IMAGE_DIR (id) )
		result = ref_asimage_list_entry(id->curr_selection);
	
	return result;	 
}

const char *as_image_file_type_names_short[ASIT_Unknown+1] =
{
	"XPM" ,
	"ZXPM" ,
	"GZXPM" ,
	"PNG" ,
	"JPEG",
	"GIMP" ,
	"PPM" ,
	"PNM" ,
	"BMP" ,
	"ICO" ,
	"MS CUR" ,
	"GIF" ,
	"TIFF",
	"AS XML" ,
	"SVG",
	"XBM",
	"Targa",
	"PCX",
	"HTML",
	"XML",
	""
};


void  asgtk_image_dir_refresh( ASGtkImageDir *id )
{
	int items = 0 ;
	char *curr_sel ;
	g_return_if_fail (ASGTK_IS_IMAGE_DIR (id));

	curr_sel = mystrdup(id->curr_selection?id->curr_selection->name:"");

	gtk_list_store_clear( GTK_LIST_STORE (id->tree_model) );
	destroy_asimage_list( &(id->entries) );
	id->curr_selection = NULL ;
	if( id->fulldirname ) 
	{
		unsigned int count ;
		GtkTreeIter iter;
		ASImageListEntry *curr ;
		int mini_ext_len = id->mini_extension?strlen(id->mini_extension): 0;
	
		id->entries = get_asimage_list( get_screen_visual(NULL),  id->fulldirname,
	              	   			        0, get_screen_image_manager(NULL)->gamma, 0, 0,
										0, &count, NULL );
		
		curr = id->entries ;
		while( curr )
		{
			Bool mini = False ;
			LOCAL_DEBUG_OUT( "adding item \"%s\"", curr->name );			
			if( mini_ext_len > 1 ) 
			{
				if( id->mini_extension[mini_ext_len-1] == '.' ) 
					mini = (strncmp (curr->name, id->mini_extension, mini_ext_len ) == 0);
				else
				{
					int name_len = strlen( curr->name );
					if( name_len > mini_ext_len ) 
						mini = (strncmp (&(curr->name[name_len-mini_ext_len]), id->mini_extension, mini_ext_len ) == 0);
				}
			}	 
			if( (!mini && curr->type <= ASIT_Supported) || get_flags(id->flags, ASGTK_ImageDir_ListAll ) ) 
			{	
				char size_str[64] ; 
				char date_str[32] ; 
				struct tm *mt = gmtime(&(curr->d_mtime));
				sprintf(date_str, "%02d-%02d-%02d %02d:%02d",
						(mt->tm_year + 1900)%100,
						mt->tm_mon + 1,
						mt->tm_mday, 
						mt->tm_hour,
						mt->tm_min);
				sprintf( size_str, "%lu", (unsigned long)((curr->d_size+1023)/1024) );
        		
				gtk_list_store_append (GTK_LIST_STORE (id->tree_model), &iter);
				gtk_list_store_set (GTK_LIST_STORE (id->tree_model), &iter, 
									ASGTK_ImageDir_Col_Name_No, curr->name, 
									ASGTK_ImageDir_Col_Type_No, as_image_file_type_names_short[curr->type], 
									ASGTK_ImageDir_Col_Size_No, size_str, 
									ASGTK_ImageDir_Col_Date_No, date_str, 
									ASGTK_ImageDir_Cols, curr, -1);
				if( ++items == 1 ) 
					gtk_tree_selection_select_iter(gtk_tree_view_get_selection(id->tree_view),&iter);
				else if( strcmp(curr->name, curr_sel) == 0 ) 
					gtk_tree_selection_select_iter(gtk_tree_view_get_selection(id->tree_view),&iter);
			}
			curr = curr->next ;
		}
		/*
			:sG:
			The following 3 lines make the file lists sorted by
			the File Name, Ascending.
		*/
                GtkTreeSortable *sortable;
                sortable = GTK_TREE_SORTABLE(id->tree_model);
                gtk_tree_sortable_set_sort_column_id(sortable,ASGTK_ImageDir_Col_Name_No,GTK_SORT_ASCENDING);		
	}		   
	if( curr_sel ) 
		free( curr_sel );
	if( items == 0 ) 
	{	
		asgtk_image_dir_sel_handler(gtk_tree_view_get_selection(id->tree_view), id);
	}
}	 

Bool 
asgtk_image_dir_make_mini_names( ASGtkImageDir *id, const char *name, char **name_return, char **fullname_return ) 
{
	if( id->mini_extension && name ) 
	{			   
		int mini_ext_len = strlen(id->mini_extension);
		char *mini_filename = safemalloc( strlen(name)+mini_ext_len+1 );

		if( id->mini_extension[mini_ext_len-1] == '.' )
			sprintf(mini_filename,"%s%s", id->mini_extension, name );
		else
			sprintf(mini_filename,"%s%s", name, id->mini_extension );

		if( fullname_return ) 
			*fullname_return = make_file_name( id->fulldirname, mini_filename );

		if( name_return ) 
			*name_return = mini_filename ;
		else 
			free( mini_filename );	  
		return True;
	}
	return False;

}	   

FILE *
open_xml_file_in_dir( ASGtkImageDir *id, const char *name, Bool mini )	
{
	FILE *fp = NULL ; 
	char* fullfilename = NULL ; 
	return NULL;	
	if( mini ) 
	{		
		if ( !asgtk_image_dir_make_mini_names( id, name, NULL, &fullfilename ) ) 
			return NULL;
	}else
		fullfilename = make_file_name( id->fulldirname, name );
	
	if( CheckFile( fullfilename ) == 0 ) 
	{
		if( !mini ) 
			if( !asgtk_yes_no_question1( NULL, "It appears that you already have private background with name \"%s\". Would you like to overwrite it ?", name ) )
			{
				free( fullfilename );
				return NULL ; 
			}
		unlink( fullfilename );
	}	
	fp = fopen( fullfilename, "w" ); 
	if( fp == NULL ) 
		asgtk_warning2( NULL, "Failed to open file \"%s\" : %s.", fullfilename, g_strerror (errno) ); 	   				   		   			

	free( fullfilename );   
	return fp;
	
}

Bool 
make_xml_from_string( ASGtkImageDir *id, const char*name, const char *str, Bool mini )
{
	FILE *fp ; 

	if( name == NULL || str == NULL ) 
		return False;
	
	if( (fp = open_xml_file_in_dir( id, name, mini )) == NULL ) 
		return False;

	fprintf( fp, "%s\n", str);
	fclose(fp);
	
	return True;
}	 

Bool
make_mini_for_image_entry(ASGtkImageDir *id, ASImageListEntry *entry, const char *mini_fullfilename)
{
	Bool res = False ;
	if( entry->type == ASIT_XMLScript ) 
	{
		char *xml = safemalloc( 128 + strlen(entry->name) );
		sprintf( xml, "<scale width=\"$minipixmap.width\" height=\"$minipixmap.height\"><img src=\"%s\"/></scale>", entry->name );
 		res = make_xml_from_string( id, entry->name, xml, True );		
		free( xml );
	}else if( entry->preview )
	{		 
		ASImage *thumbnail = scale_asimage( get_screen_visual(NULL), entry->preview, 24, 24, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );
		if( thumbnail ) 
		{	
			res = save_asimage_to_file(mini_fullfilename, thumbnail, "png", "9", NULL, 0, True);
			destroy_asimage( &thumbnail );					
		}
	}	 
	return res;
}		



