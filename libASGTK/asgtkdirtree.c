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
#include "asgtkdirtree.h"

#define TREE_NAME		0
#define TREE_FULLPATH   1
#define TREE_SCANNED	2
#define TREE_COLUMNS	3


/*  local function prototypes  */
static void asgtk_dir_tree_class_init (ASGtkDirTreeClass *klass);
static void asgtk_dir_tree_init (ASGtkDirTree *iv);
static void asgtk_dir_tree_dispose (GObject *object);
static void asgtk_dir_tree_finalize (GObject *object);
static void asgtk_dir_tree_style_set (GtkWidget *widget, GtkStyle  *prev_style);
static void asgtk_dir_tree_refresh_child( GtkTreeStore *dir_store, GtkTreeIter *parent, char *fullchildname, int level );


/*  private variables  */
static GtkScrolledWindowClass *parent_class = NULL;

GType
asgtk_dir_tree_get_type (void)
{
  	static GType id_type = 0;

  	if (! id_type)
    {
    	static const GTypeInfo id_info =
      	{
        	sizeof (ASGtkDirTreeClass),
        	(GBaseInitFunc)     NULL,
        	(GBaseFinalizeFunc) NULL,
			(GClassInitFunc)    asgtk_dir_tree_class_init,
        	NULL,           /* class_finalize */
        	NULL,           /* class_data     */
        	sizeof (ASGtkDirTree),
        	0,              /* n_preallocs    */
        	(GInstanceInitFunc) asgtk_dir_tree_init,
      	};

      	id_type = g_type_register_static (	GTK_TYPE_SCROLLED_WINDOW,
        	                                "ASGtkDirTree",
            	                            &id_info, 0);
    }

  	return id_type;
}

static void
asgtk_dir_tree_class_init (ASGtkDirTreeClass *klass)
{
  	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->dispose   = asgtk_dir_tree_dispose;
  	object_class->finalize  = asgtk_dir_tree_finalize;

  	widget_class->style_set = asgtk_dir_tree_style_set;

}

static void
asgtk_dir_tree_init (ASGtkDirTree *id)
{
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (id),
				    				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (id), GTK_SHADOW_IN);      
	id->root = NULL ; 
}

static void
asgtk_dir_tree_dispose (GObject *object)
{
  	ASGtkDirTree *dt = ASGTK_DIR_TREE (object);
	destroy_string( &(dt->root) );
	
  	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
asgtk_dir_tree_finalize (GObject *object)
{
  	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
asgtk_dir_tree_style_set (GtkWidget *widget,
                          GtkStyle  *prev_style)
{
  /* ASGtkDirTree *id = ASGTK_DIR_TREE (widget); */

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}

static void
asgtk_dir_tree_sel_handler(GtkTreeSelection *selection, gpointer user_data)
{
  	ASGtkDirTree *dt = ASGTK_DIR_TREE(user_data); 
	GtkTreeIter iter;
	GtkTreeModel *model;

  	if (gtk_tree_selection_get_selected (selection, &model, &iter)) 
	{
    	gtk_tree_model_get (model, &iter, TREE_FULLPATH, &(dt->curr_selection), -1);
  	}else
	{	
		dt->curr_selection = NULL ;
	}
		
	if( dt->sel_change_handler )
		dt->sel_change_handler( dt, dt->sel_change_user_data ); 
}
static void
asgtk_dir_tree_expand_handler(GtkTreeView *treeview,
                              GtkTreeIter *iter,
                              GtkTreePath *path,
                              gpointer user_data)
{
  	ASGtkDirTree *dt = ASGTK_DIR_TREE(user_data); 
	GtkTreeModel *model = dt->tree_model;
	GtkTreeIter child_iter ;

	gtk_tree_path_down( path );
	gtk_tree_model_get_iter( model, &child_iter, path );

	do
	{
		char *fullpath ;
		int scanned = 0 ;
		gtk_tree_model_get(	model, &child_iter,
			     			TREE_FULLPATH, &fullpath, 
		     				TREE_SCANNED, &scanned,  -1);
		if( scanned == 1 ) 
			break;

		asgtk_dir_tree_refresh_child( GTK_TREE_STORE(model), &child_iter, fullpath, 1 );
		gtk_tree_store_set (GTK_TREE_STORE (model), &child_iter, TREE_SCANNED, 1, -1);
	}while(gtk_tree_model_iter_next (model, &child_iter));	
}

/*  public functions  */
GtkWidget *
asgtk_dir_tree_new ()
{
	ASGtkDirTree *dt;
	GtkTreeSelection *selection;
  	
    dt = g_object_new (ASGTK_TYPE_DIR_TREE, NULL);

	dt->tree_view = GTK_TREE_VIEW(gtk_tree_view_new());
	dt->tree_model = GTK_TREE_MODEL(gtk_tree_store_new (TREE_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT));

	gtk_container_add (GTK_CONTAINER(dt), GTK_WIDGET(dt->tree_view));
    gtk_tree_view_set_model (dt->tree_view, dt->tree_model);
    gtk_widget_show (GTK_WIDGET(dt->tree_view));
    dt->cell = gtk_cell_renderer_text_new ();

    dt->column = gtk_tree_view_column_new_with_attributes ("", dt->cell, "text", 0, NULL);
    gtk_tree_view_append_column (dt->tree_view, GTK_TREE_VIEW_COLUMN (dt->column));
	
	selection = gtk_tree_view_get_selection(dt->tree_view);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
   	g_signal_connect (selection, "changed",  G_CALLBACK (asgtk_dir_tree_sel_handler), dt);
   	g_signal_connect (dt->tree_view, "row-expanded",  G_CALLBACK (asgtk_dir_tree_expand_handler), dt);
	
	colorize_gtk_tree_view_window( GTK_WIDGET(dt) );
	LOCAL_DEBUG_OUT( "created image ASGtkDirTree object %p", dt );	
	return GTK_WIDGET (dt);
}

void  
asgtk_dir_tree_set_root( ASGtkDirTree *dt, char *root, GtkTreeModel *saved_model )
{
	GtkTreeModel *old_model ; 
	g_return_if_fail (ASGTK_IS_DIR_TREE (dt));
	
	if( dt->root == NULL && root == NULL ) 
		return;
	if( dt->root && root && strcmp(dt->root, root)== 0  ) 
		return;
	destroy_string( &(dt->root) ); 

	if( root ) 
		dt->root = mystrdup(root);
	
	dt->curr_selection = NULL ;
	old_model = GTK_TREE_MODEL(dt->tree_model); 

	if( saved_model != NULL ) 
		dt->tree_model = g_object_ref( saved_model );
    else
		dt->tree_model = GTK_TREE_MODEL(gtk_tree_store_new (TREE_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT));
    gtk_tree_view_set_model (dt->tree_view, dt->tree_model);
	
	asgtk_dir_tree_refresh( dt );		 
	g_object_unref( old_model );

}	 

GtkTreeModel *
asgtk_dir_tree_get_model( ASGtkDirTree *dt )
{
	return g_object_ref( dt->tree_model );
}	 

void  
asgtk_dir_tree_set_title( ASGtkDirTree *dt, const gchar *title )
{
	g_return_if_fail (ASGTK_IS_DIR_TREE (dt));
	gtk_tree_view_column_set_title( dt->column, title );	
}

void  
asgtk_dir_tree_set_sel_handler( ASGtkDirTree *dt, _ASGtkDirTree_sel_handler sel_change_handler, gpointer user_data )
{
	g_return_if_fail (ASGTK_IS_DIR_TREE (dt));

	dt->sel_change_handler = sel_change_handler ; 
	dt->sel_change_user_data = user_data ;
	
}

char *
asgtk_dir_tree_get_selection(ASGtkDirTree *dt )
{
	char *result = NULL ; 

	if( ASGTK_IS_DIR_TREE (dt) )
		result = mystrdup(dt->curr_selection);
	
	return result;	 
}

static void  
asgtk_dir_tree_refresh_child( GtkTreeStore *dir_store, GtkTreeIter *parent, char *fullchildname, int level )
{
	if( --level >= 0 )
	{	
		GtkTreeIter iter;
		struct direntry  **list = NULL;
		int n = my_scandir (fullchildname, &list, NULL, NULL);
		
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
						char * fulldirname = make_file_name( fullchildname, list[i]->d_name );
						gtk_tree_store_append (dir_store, &iter, parent);
						gtk_tree_store_set (dir_store, &iter, TREE_NAME, list[i]->d_name, TREE_FULLPATH, fulldirname, TREE_SCANNED, 0, -1);
						if( level > 0 )
							asgtk_dir_tree_refresh_child( dir_store, &iter, fulldirname, level-1 );
						free( fulldirname );
					}
				}	 
				free( list[i] );
			}
			free (list);
		}
	}
} 


void  asgtk_dir_tree_refresh( ASGtkDirTree *dt )
{
	GtkTreeStore *dir_store ;
	GtkTreeIter iter;

	g_return_if_fail (ASGTK_IS_DIR_TREE (dt));

	dir_store = GTK_TREE_STORE (dt->tree_model);

	gtk_tree_store_clear( dir_store );
	
	if( dt->root != NULL ) 
	{	
		gtk_tree_store_append (dir_store, &iter, NULL);
		gtk_tree_store_set (dir_store, &iter, TREE_NAME, dt->root, TREE_FULLPATH, dt->root, TREE_SCANNED, 1, -1);
		asgtk_dir_tree_refresh_child( dir_store, &iter, dt->root, 2 );
	} 
#if 0	
	curr_sel = mystrdup(dt->curr_selection?dt->curr_selection->name:"");

	gtk_list_store_clear( GTK_LIST_STORE (dt->tree_model) );
	destroy_asimage_list( &(dt->entries) );
	dt->curr_selection = NULL ;
	if( dt->fulldirname ) 
	{
		int count ;
		GtkTreeIter iter;
		ASImageListEntry *curr ;
		int mini_ext_len = dt->mini_extension?strlen(dt->mini_extension): 0;
	
		dt->entries = get_asimage_list( get_screen_visual(NULL),  dt->fulldirname,
	              	   			        0, get_screen_image_manager(NULL)->gamma, 0, 0,
										0, &count, NULL );
		
		curr = dt->entries ;
		while( curr )
		{
			Bool mini = False ;
			LOCAL_DEBUG_OUT( "adding item \"%s\"", curr->name );			
			if( mini_ext_len > 1 ) 
			{
				if( dt->mini_extension[mini_ext_len-1] == '.' ) 
					mini = (strncmp (curr->name, dt->mini_extension, mini_ext_len ) == 0);
				else
				{
					int name_len = strlen( curr->name );
					if( name_len > mini_ext_len ) 
						mini = (strncmp (&(curr->name[name_len-mini_ext_len]), dt->mini_extension, mini_ext_len ) == 0);
				}
			}	 
			if( !mini && curr->type <= ASIT_Supported ) 
			{	
        		gtk_list_store_append (GTK_LIST_STORE (dt->tree_model), &iter);
				gtk_list_store_set (GTK_LIST_STORE (dt->tree_model), &iter, 0, curr->name, 1, curr, -1);
				if( ++items == 1 ) 
					gtk_tree_selection_select_iter(gtk_tree_view_get_selection(dt->tree_view),&iter);
				else if( strcmp(curr->name, curr_sel) == 0 ) 
					gtk_tree_selection_select_iter(gtk_tree_view_get_selection(dt->tree_view),&iter);
			}
			curr = curr->next ;
		}
	}		   
	if( curr_sel ) 
		free( curr_sel );
	if( items == 0 ) 
	{	
		asgtk_dir_tree_sel_handler(gtk_tree_view_get_selection(dt->tree_view), dt);
	}
#endif
}	 

GtkTreePath *
asgtk_dir_tree_get_curr_path( ASGtkDirTree *dt )
{
	GtkTreePath *path = NULL ; 
	GtkTreeViewColumn *focus_column = NULL ;
	gtk_tree_view_get_cursor( dt->tree_view, &path, &focus_column);	
	return path;
}	 

void 
asgtk_dir_tree_restore_curr_path( ASGtkDirTree *dt, GtkTreePath *path )
{
	if( path ) 
	{	
		gtk_tree_view_expand_to_path(dt->tree_view, path);	
		gtk_tree_view_set_cursor(dt->tree_view, path, dt->column, FALSE);	 
	}	
}

