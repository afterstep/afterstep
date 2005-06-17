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


#include "asgtk.h"
#include "asgtkai.h"
#include "asgtkimagedir.h"
#include "asgtkimageview.h"
#include "asgtkimagebrowser.h"

#define DIR_LIST_WIDTH   200
#define DIR_LIST_HEIGHT  150
#define FILE_LIST_WIDTH  200
#define FILE_LIST_HEIGHT 210
#define PREVIEW_WIDTH  480
#define PREVIEW_HEIGHT 360


/*  local function prototypes  */
static void asgtk_image_browser_class_init (ASGtkImageBrowserClass *klass);
static void asgtk_image_browser_init (ASGtkImageBrowser *ib);
static void asgtk_image_browser_dispose (GObject *object);
static void asgtk_image_browser_finalize (GObject *object);
static void asgtk_image_browser_style_set (GtkWidget *widget, GtkStyle  *prev_style);

/*  private variables  */
static GtkWindowClass *parent_class = NULL;

GType
asgtk_image_browser_get_type (void)
{
  	static GType ib_type = 0;

  	if (!ib_type)
    {
    	static const GTypeInfo ib_info =
      	{
        	sizeof (ASGtkImageBrowserClass),
        	(GBaseInitFunc)     NULL,
        	(GBaseFinalizeFunc) NULL,
			(GClassInitFunc)    asgtk_image_browser_class_init,
        	NULL,           /* class_finalize */
        	NULL,           /* class_data     */
        	sizeof (ASGtkImageBrowser),
        	0,              /* n_preallocs    */
        	(GInstanceInitFunc) asgtk_image_browser_init,
      	};

      	ib_type = g_type_register_static (	GTK_TYPE_WINDOW,
        	                                "ASGtkImageBrowser",
            	                            &ib_info, 0);
    }

  	return ib_type;
}

static void
asgtk_image_browser_class_init (ASGtkImageBrowserClass *klass)
{
  	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->dispose   = asgtk_image_browser_dispose;
  	object_class->finalize  = asgtk_image_browser_finalize;

  	widget_class->style_set = asgtk_image_browser_style_set;

}

static void
asgtk_image_browser_init (ASGtkImageBrowser *id)
{
}

static void
asgtk_image_browser_dispose (GObject *object)
{
  	/*ASGtkImageBrowser *ib = ASGTK_IMAGE_BROWSER (object); */
  	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
asgtk_image_browser_finalize (GObject *object)
{
  	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
asgtk_image_browser_style_set (GtkWidget *widget,
                          GtkStyle  *prev_style)
{
  /* ASGtkImageBrowser *id = ASGTK_IMAGE_BROWSER (widget); */

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}

static char *asgtk_image_browser_make_fullfilename( ASGtkImageBrowser *ib, const char *filename ) 
{			   
	if( filename[0] == '.' && filename[1] == '\0' ) 
		return mystrdup(ib->current_dir);
	
	if( filename[0] == '.' && filename[1] == '.' && filename[2] == '\0' ) 
	{	
		char *sep = strrchr(ib->current_dir, '/'); 
		if( sep > ib->current_dir ) 
			return mystrndup(ib->current_dir, sep - ib->current_dir );
		
		return mystrdup( "/" );
	}
	
	if(filename[0] == '/')
		return mystrdup(filename);
	
	return make_file_name( ib->current_dir, filename) ;
}

static void
asgtk_image_browser_dir_activate (GtkTreeView       *tree_view,
	   							  GtkTreePath       *path,
				 				  GtkTreeViewColumn *column,
				 				  gpointer           user_data)
{
  	ASGtkImageBrowser *ib = ASGTK_IMAGE_BROWSER (user_data);
  	GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  	GtkTreeIter iter;
  	char *filename ;

  	gtk_tree_model_get_iter (model, &iter, path);
  	gtk_tree_model_get (model, &iter, 0, &filename, -1);
	LOCAL_DEBUG_OUT( "filename == \"%s\"", filename );	
	/* TODO : change directory/populate file list */  
	if( filename[0] == '.' && filename[1] == '\0' ) 
	{	
	 	/* already in current dir - do nothing */	
	}else
	{
		filename = asgtk_image_browser_make_fullfilename( ib, filename );
		asgtk_image_browser_change_dir( ib, filename ); 
		free( filename );
	}
}

static void
asgtk_image_browser_dir_select (GtkTreeSelection *selection, gpointer user_data)
{
  	ASGtkImageBrowser *ib = ASGTK_IMAGE_BROWSER (user_data);
  	char *filename ;
	GtkTreeIter iter;
	GtkTreeModel *model;

  	if (gtk_tree_selection_get_selected (selection, &model, &iter)) 
	{
  		gtk_tree_model_get (model, &iter, 0, &filename, -1);
		LOCAL_DEBUG_OUT( "filename == \"%s\"", filename );	
		/* TODO : change directory/populate file list */  
		if( filename[0] == '.' && filename[1] == '\0' ) 
			asgtk_image_dir_set_path(ASGTK_IMAGE_DIR(ib->image_dir), ib->current_dir);
		else 
		{	 
			filename = asgtk_image_browser_make_fullfilename( ib, filename );
			asgtk_image_dir_set_path(ASGTK_IMAGE_DIR(ib->image_dir), filename);
			free (filename);
		}
	}
}

static void
on_root_clicked(GtkButton *button, gpointer user_data)
{
  	ASGtkImageBrowser *ib = ASGTK_IMAGE_BROWSER (user_data);
	asgtk_image_browser_change_dir( ib, "/" ); 
}

static void
on_home_clicked(GtkButton *button, gpointer user_data)
{
  	ASGtkImageBrowser *ib = ASGTK_IMAGE_BROWSER (user_data);
	asgtk_image_browser_change_dir( ib, "~/" ); 
}

static void find_combobox_entry(GtkWidget *widget, gpointer data)
{
  	ASGtkImageBrowser *ib = ASGTK_IMAGE_BROWSER (data);
	if( GTK_IS_ENTRY(widget) )
		ib->path_entry = widget ;
}	 

static void asgtk_image_browser_path_entry( GtkWidget *widget, gpointer data )
{
 	ASGtkImageBrowser *ib = ASGTK_IMAGE_BROWSER (data);
	if( ib->path_entry ) 
	{	
  		const gchar *entry_text = gtk_entry_get_text (GTK_ENTRY (ib->path_entry));
		asgtk_image_browser_change_dir( ib, entry_text ); 
	}
}

static void 
asgtk_image_browser_path_changed( GtkWidget *widget, gpointer data )
{
 	ASGtkImageBrowser *ib = ASGTK_IMAGE_BROWSER (data);
	if( ib ) 
	{	
  		const gchar *entry_text = asgtk_combo_box_get_active_text(GTK_COMBO_BOX(ib->path_combo));
		asgtk_image_browser_change_dir( ib, entry_text ); 
	}
}

static void 
asgtk_image_browser_scale_toggle( GtkWidget *checkbutton, gpointer data )
{
  	ASGtkImageBrowser *ib = ASGTK_IMAGE_BROWSER (data);
	asgtk_image_view_enable_scaling( ib->preview, GTK_TOGGLE_BUTTON (checkbutton)->active );	
}
	
	

static void 
asgtk_image_browser_create_dir_list( ASGtkImageBrowser *ib, GtkWidget *list_vbox	)
{
	GtkWidget *scrolled_win, *model ;
	GtkTreeViewColumn *column ;
		
	model = GTK_WIDGET(gtk_list_store_new (1, G_TYPE_STRING));
  	ib->dir_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
  	g_object_unref (model);
		
	column = gtk_tree_view_column_new_with_attributes (
				"Folders", gtk_cell_renderer_text_new (), "text", 0, 
				NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  	gtk_tree_view_append_column (GTK_TREE_VIEW (ib->dir_list), column);

  	gtk_widget_set_size_request (ib->dir_list, DIR_LIST_WIDTH, DIR_LIST_HEIGHT);
	g_signal_connect ( ib->dir_list, "row_activated",
						G_CALLBACK (asgtk_image_browser_dir_activate), ib);
	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (ib->dir_list)), "changed",
		    		  G_CALLBACK (asgtk_image_browser_dir_select), ib);
	
  	scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), GTK_SHADOW_IN);  
  	gtk_container_add (GTK_CONTAINER (scrolled_win), ib->dir_list);
  	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
									GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  	gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 0);
	if (GTK_IS_PANED (list_vbox))
		gtk_paned_pack1 (GTK_PANED (list_vbox), GTK_WIDGET(scrolled_win), TRUE, TRUE);
	else
		gtk_box_pack_end (GTK_BOX (list_vbox), GTK_WIDGET(scrolled_win), TRUE, TRUE, 0);
  		
  	gtk_widget_show (ib->dir_list);
  	gtk_widget_show (scrolled_win);
	colorize_gtk_tree_view_window( GTK_WIDGET(scrolled_win) );
}

static void
asgtk_image_browser_file_sel_handler(ASGtkImageDir *id, gpointer user_data)
{
	ASGtkImageBrowser *ib = ASGTK_IMAGE_BROWSER(user_data);
	g_return_if_fail (ASGTK_IS_IMAGE_DIR (id));
	if( ib ) 
	{	
		ASImageListEntry *le = asgtk_image_dir_get_selection( id ); 
		asgtk_image_view_set_entry ( ib->preview, le);
		if( le )
			unref_asimage_list_entry( le );
	}
}


/*  public functions  */
GtkWidget * 
asgtk_image_browser_new ()
{
	ASGtkImageBrowser *ib;
    GtkWidget *main_vbox;
	GtkWidget *separator;
	GtkWidget *shortcuts_hbox, *btn ; 
	GtkWidget *list_vbox, *preview_vbox ; 

    ib = g_object_new (ASGTK_TYPE_IMAGE_BROWSER, NULL);
	colorize_gtk_window( GTK_WIDGET(ib) );	

	gtk_container_set_border_width( GTK_CONTAINER (ib), 5 );
	main_vbox = gtk_vbox_new (FALSE, 0);
  	gtk_widget_show (main_vbox);
	gtk_container_add (GTK_CONTAINER (ib), main_vbox);
	gtk_box_set_spacing( GTK_BOX(main_vbox), 5 );

	shortcuts_hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (shortcuts_hbox);
	gtk_box_pack_start (GTK_BOX (main_vbox), shortcuts_hbox, FALSE, FALSE, 0);
	gtk_box_set_spacing( GTK_BOX(shortcuts_hbox), 5 );

	btn = asgtk_add_button_to_box( GTK_BOX (shortcuts_hbox), GTK_STOCK_GOTO_TOP, "/", G_CALLBACK(on_root_clicked), ib );   
	gtk_widget_set_size_request ( btn, 50, -1);
	btn = asgtk_add_button_to_box( GTK_BOX (shortcuts_hbox), GTK_STOCK_HOME, "~/", G_CALLBACK(on_home_clicked), ib );   
	gtk_widget_set_size_request ( btn, 50, -1);

	ib->path_combo = gtk_combo_box_entry_new_text();
	//gtk_widget_set_size_request ( ib->path_combo, DIR_LIST_WIDTH+300-105 , -1);
	gtk_box_pack_end (GTK_BOX (shortcuts_hbox), ib->path_combo, TRUE, TRUE, 10);
	gtk_widget_show (ib->path_combo);
	colorize_gtk_edit(ib->path_combo);
	ib->path_entry = NULL ; 
	if( GTK_IS_CONTAINER(ib->path_combo) )
		gtk_container_forall( GTK_CONTAINER(ib->path_combo), find_combobox_entry, ib );
	if( ib->path_entry ) 
		g_signal_connect ( G_OBJECT (ib->path_entry), "activate",
		      			   G_CALLBACK (asgtk_image_browser_path_entry), (gpointer) ib);
	g_signal_connect (G_OBJECT(ib->path_combo), "changed",
			  			G_CALLBACK (asgtk_image_browser_path_changed), (gpointer) ib);


#if 0
	ib->list_hbox = gtk_hbox_new (FALSE, 0);
#else
  	ib->list_hbox = gtk_hpaned_new ();
	colorize_gtk_widget( ib->list_hbox, get_colorschemed_style_normal() );  
#endif
  	gtk_widget_show (ib->list_hbox);
  	gtk_box_pack_start (GTK_BOX (main_vbox), ib->list_hbox, TRUE, TRUE, 0);

  	ib->main_buttons_hbox = gtk_hbutton_box_new ();
  	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_SPREAD);
  	gtk_widget_show (ib->main_buttons_hbox);
  	gtk_box_pack_end (GTK_BOX (main_vbox), ib->main_buttons_hbox, FALSE, FALSE, 5);


    /* separator really goes above the buttons box, so it is added second from the end ! */
	separator = gtk_hseparator_new();
	gtk_widget_show (separator);
	gtk_box_pack_end (GTK_BOX (main_vbox), separator, FALSE, FALSE, 5);

	colorize_gtk_widget( GTK_WIDGET(separator), get_colorschemed_style_button() );  
	   
	/* now designing dirlist controls : */
#if 0
	list_vbox = gtk_vbox_new (FALSE, 0);
#else
  	list_vbox = gtk_vpaned_new ();
	colorize_gtk_widget( list_vbox, get_colorschemed_style_normal() );  
#endif
	gtk_widget_show (list_vbox);

	if (GTK_IS_PANED (ib->list_hbox))
		gtk_paned_pack1 (GTK_PANED (ib->list_hbox), list_vbox, FALSE, FALSE);
	else 
		gtk_box_pack_start (GTK_BOX (ib->list_hbox), list_vbox, FALSE, FALSE, 0);
	
	/* dir list : */
	asgtk_image_browser_create_dir_list( ib, list_vbox );
	/* image file list : */
	ib->image_dir = ASGTK_IMAGE_DIR(asgtk_image_dir_new());
	if (GTK_IS_PANED (list_vbox))
		gtk_paned_pack2 (GTK_PANED (list_vbox), GTK_WIDGET(ib->image_dir), TRUE, TRUE);
	else
		gtk_box_pack_end (GTK_BOX (list_vbox), GTK_WIDGET(ib->image_dir), TRUE, TRUE, 0);

	gtk_widget_set_size_request (GTK_WIDGET(ib->image_dir), FILE_LIST_WIDTH, FILE_LIST_HEIGHT);
	gtk_widget_show (GTK_WIDGET(ib->image_dir));

	asgtk_image_dir_set_title(ASGTK_IMAGE_DIR(ib->image_dir), "Image files:");

	/* now designing preview controls : */
  	preview_vbox = gtk_vbox_new (FALSE, 0);
  	gtk_widget_show (preview_vbox);
	if (GTK_IS_PANED (ib->list_hbox))
		gtk_paned_pack2 (GTK_PANED (ib->list_hbox), preview_vbox, TRUE, TRUE);
	else 
		gtk_box_pack_end (GTK_BOX (ib->list_hbox), preview_vbox, TRUE, TRUE, 5);

	ib->preview = ASGTK_IMAGE_VIEW(asgtk_image_view_new());
	gtk_widget_set_size_request (GTK_WIDGET(ib->preview), PREVIEW_WIDTH, PREVIEW_HEIGHT);
	gtk_box_pack_start (GTK_BOX (preview_vbox), GTK_WIDGET(ib->preview), TRUE, TRUE, 0);
	gtk_widget_show (GTK_WIDGET(ib->preview));
	asgtk_image_view_enable_tiling( ib->preview, FALSE );	   

	ib->scale_check_box = gtk_check_button_new_with_label( "Scale to fit" );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(ib->scale_check_box), TRUE );
	gtk_widget_show (ib->scale_check_box);
	colorize_gtk_widget( ib->scale_check_box, get_colorschemed_style_normal() );  
	g_signal_connect (G_OBJECT (ib->scale_check_box), "toggled",
	              	  G_CALLBACK (asgtk_image_browser_scale_toggle), (gpointer) ib);
	
	gtk_button_set_alignment( GTK_BUTTON(ib->scale_check_box), 1.0, 0.5);
	asgtk_image_view_add_detail( ib->preview, ib->scale_check_box, 0 );

	/* changing to default dir : */
	asgtk_image_dir_set_sel_handler( ASGTK_IMAGE_DIR(ib->image_dir), asgtk_image_browser_file_sel_handler, ib);
	asgtk_image_browser_change_dir( ib, NULL );	
	
	LOCAL_DEBUG_OUT( "created image ASGtkImageBrowser object %p", ib );	
	return GTK_WIDGET (ib);
}

GtkWidget*
asgtk_image_browser_add_main_button( ASGtkImageBrowser *ib, const char *stock, GCallback func, gpointer user_data )
{
	if(!ASGTK_IS_IMAGE_BROWSER (ib))
		return NULL;

	return asgtk_add_button_to_box( GTK_BOX (ib->main_buttons_hbox), stock, NULL, func, user_data );   
}	   

GtkWidget*
asgtk_image_browser_add_selection_button( ASGtkImageBrowser *ib, const char *stock, GCallback func )
{
	GtkWidget *btn ;
	if( !ASGTK_IS_IMAGE_BROWSER (ib))
		return NULL; 

	btn = asgtk_add_button_to_box( NULL, stock, NULL, func, ib->image_dir );   
 	asgtk_image_view_add_tool( ib->preview, btn, 0 );	
	return btn;
}	   

void 
asgtk_image_browser_refresh( ASGtkImageBrowser *ib )
{
	GtkListStore *dir_store ;
	GtkTreeIter iter;
	
	g_return_if_fail (ASGTK_IS_IMAGE_BROWSER (ib));
	LOCAL_DEBUG_OUT( " refreshing %p: current dir = \"%s\"", ib, ib->current_dir?ib->current_dir:"null" );
	asgtk_image_dir_set_path(ASGTK_IMAGE_DIR(ib->image_dir), ib->current_dir);
	dir_store = GTK_LIST_STORE (gtk_tree_view_get_model(GTK_TREE_VIEW(ib->dir_list)));	
	gtk_list_store_clear(dir_store);
   		
	if( ib->current_dir != NULL ) 
	{
		/* Add current dir entry as the first item :*/
		gtk_list_store_append (dir_store, &iter);
		gtk_list_store_set (dir_store, &iter, 0, ".", -1);
		
		if( strcmp(ib->current_dir, "/") != 0 ) 
		{	
			gtk_list_store_append (dir_store, &iter);
			gtk_list_store_set (dir_store, &iter, 0, "..", -1);
		}	

		/* Add subdirs :*/		
		{   	   
			struct direntry  **list = NULL;
	 		int n = my_scandir (ib->current_dir, &list, NULL, NULL);
			LOCAL_DEBUG_OUT( " scandir indicates %d entries in list", n );
			if( n > 0 )
			{
				int i ;
				for (i = 0; i < n; i++)
				{
					if (S_ISDIR (list[i]->d_mode) )
					{
						Bool skip = False ;
						if( list[i]->d_name[0] == '.' )
						{
							skip = ( list[i]->d_name[1] == '\0' ||
								     ( list[i]->d_name[1] == '.' && list[i]->d_name[2] == '\0' ) );
						}
						if( !skip ) 
						{	
							gtk_list_store_append (dir_store, &iter);
							gtk_list_store_set (dir_store, &iter, 0, list[i]->d_name, -1);
						}
					}	 
					free( list[i] );
				}
				free (list);
			}
		}

		/* Add dirs above us : */			   
		{
			char *above_dir = mystrdup( ib->current_dir );
			int i = strlen(above_dir)-1;
			while( --i > 0 )
			{
				if( above_dir[i] == '/' ) 
				{
					above_dir[i] = '\0' ; 	
					gtk_list_store_append (dir_store, &iter);
					gtk_list_store_set (dir_store, &iter, 0, above_dir, -1);
				}		 
			}	 
		}		
	}	
	if( ib->current_dir	 == NULL || strcmp(ib->current_dir, "/") != 0 ) 
	{	
		gtk_list_store_append (dir_store, &iter);
		gtk_list_store_set (dir_store, &iter, 0, "/", -1);
	}
	
}

static void
add_dir_to_history( ASGtkImageBrowser *ib, const char *dir ) 
{
	GtkTreeModel* model = gtk_combo_box_get_model( GTK_COMBO_BOX(ib->path_combo) );	
	GtkTreeIter  iter;

	g_signal_handlers_block_by_func(G_OBJECT(ib->path_combo), 
								    G_CALLBACK (asgtk_image_browser_path_changed), (gpointer) ib);
	if( gtk_tree_model_get_iter_first( model, &iter ) )
	{
		do
		{
			gchar *val = NULL ;
			gtk_tree_model_get (model, &iter, 0, &val, -1);
			if( val && strcmp( dir, val ) == 0 ) 
			{
				gtk_list_store_remove( GTK_LIST_STORE(model), &iter );
				break;	
			}	 
		}while(gtk_tree_model_iter_next(model, &iter));			 
	}	 
	gtk_combo_box_prepend_text (GTK_COMBO_BOX(ib->path_combo), dir );
	gtk_combo_box_set_active (GTK_COMBO_BOX(ib->path_combo), 0);
	g_signal_handlers_unblock_by_func(G_OBJECT(ib->path_combo), 
								    G_CALLBACK (asgtk_image_browser_path_changed), (gpointer) ib);

}

void
asgtk_image_browser_change_dir( ASGtkImageBrowser *ib, const char *dir ) 
{
	char * fullfilename ;
	int fullnamelen ;
	
	g_return_if_fail (ASGTK_IS_IMAGE_BROWSER (ib));
	
	if( dir == NULL )
		fullfilename = PutHome("~/");
	else if( dir[0] == '/' ) 
		fullfilename = mystrdup(dir);
	else
		fullfilename = PutHome(dir);

	fullnamelen = strlen( fullfilename ) ;
	while( fullnamelen > 1 && fullfilename[fullnamelen-1] == '/' )
	{	
		fullfilename[fullnamelen-1] = '\0' ; 
		--fullnamelen ;
	}

	LOCAL_DEBUG_OUT( " changing dir for  %p: current dir = \"%s\", new dir = \"%s\"", ib, ib->current_dir?ib->current_dir:"null", fullfilename );
	if( ib->current_dir == NULL || strcmp( ib->current_dir, fullfilename ) != 0 )
	{
		char *title = safemalloc( 128+fullnamelen+1 ); 
		if( ib->current_dir	)
			free( ib->current_dir );
		
		ib->current_dir = fullfilename ; 
		sprintf( title, "AfterStep image browser: %s", fullfilename );
		gtk_window_set_title(GTK_WINDOW(ib), title );
		free( title );
		add_dir_to_history( ib, fullfilename ); 
		asgtk_image_browser_refresh( ib );
	}	 
	
}	  

