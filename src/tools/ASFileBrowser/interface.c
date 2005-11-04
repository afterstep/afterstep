/* 
 * Copyright (c) 2005 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#define LOCAL_DEBUG

#include "../../../configure.h"
#include "../../../libAfterStep/asapp.h"
#include "../../../libAfterImage/afterimage.h"
#include "../../../libAfterStep/screen.h"
#include "../../../libAfterStep/colorscheme.h"
#include "../../../libAfterStep/module.h"
#include "../../../libAfterStep/session.h"
#include "../../../libASGTK/asgtk.h"

#include <unistd.h>		   

#include "interface.h"

extern ASFileBrowserState AppState ;

#define DIR_TREE_WIDTH			300
#define DIR_TREE_HEIGHT			300
#define FILE_LIST_WIDTH			300
#define FILE_LIST_HEIGHT		200
#define PREVIEW_WIDTH  			480
#define PREVIEW_HEIGHT 			360
#define DEFAULT_MAX_TEXT_SIZE 	8192

/* ###################################################################### */

typedef enum 
{
	root_PrivateAfterStep = 0, 
	root_SharedAfterStep, 
	root_Home, 
	root_UsrShare, 
	root_UsrLocalShare,
	root_Root,
	root_StandardRoots,
	root_Other = root_StandardRoots
}ASFileBrowserRoot;

typedef struct ASFileBrowserMainFrame
{
	GtkWidget *view_tabs ;
	GtkWidget *view_image ;
	GtkWidget *view_text ;
	GtkWidget *view_hex ;
	GtkWidget *dirlist ;
	GtkWidget *filelist ;
	GtkTextBuffer * text_buffer ;
}ASFileBrowserMainFrame;
	   
typedef struct ASFileBrowserRootSelFrame
{
	GtkWidget *table;
	GtkActionGroup *action_group ;
	GtkWidget *path_combo ;
	GtkWidget *path_entry ;
	GtkWidget *file_chooser_btn ;
	GtkWidget *target_dirlist ;
	GtkTreeModel *root_models[root_StandardRoots];
	GtkTreePath *root_paths[root_StandardRoots];
	ASFileBrowserRoot last_nonroot_root;
}ASFileBrowserRootSelFrame;

static void
change_root_to( ASFileBrowserRootSelFrame *data, ASFileBrowserRoot root )	
{
	char *new_root = NULL ;
	int i ;
	GtkTreeModel *old_model ;

	if( root != root_Other )
	{
		/* disable other text controls */			
	}		   
	switch( root ) 
	{
		case root_PrivateAfterStep : 
			new_root = mystrdup(Session->ashome );
		    break ;					   
		case root_SharedAfterStep  :
			new_root = mystrdup(Session->asshare );
		    break ;					   
		case root_Home :
			new_root = mystrdup(getenv("HOME"));
		    break ;					   
		case root_UsrShare :
			new_root = mystrdup("/usr/share");
		    break ;					   
		case root_UsrLocalShare :
			new_root = mystrdup("/usr/local/share");
		    break ;					   
		case root_Root :
#ifdef __CYGWIN__
			new_root = mystrdup("/cygdrive");
#else
			new_root = mystrdup("/");
#endif
		    break ;					   
		case root_Other :
		    break ;			
	}	 
	
	if( root != root_Root ) 
		data->last_nonroot_root = root ; 


	old_model = asgtk_dir_tree_get_model( ASGTK_DIR_TREE(data->target_dirlist) );
	for( i = 0 ; i < root_StandardRoots ; ++i ) 
		if( data->root_models[i] == old_model ) 
		{
			if( data->root_paths[i] )
				gtk_tree_path_free( data->root_paths[i] );
			data->root_paths[i]	= asgtk_dir_tree_get_curr_path( ASGTK_DIR_TREE(data->target_dirlist) );				   
			break;
		}	 
	g_object_unref( old_model );
	if( root < root_StandardRoots )
	{	
		GtkTreeModel *new_model ;
		asgtk_dir_tree_set_root( ASGTK_DIR_TREE(data->target_dirlist), new_root, data->root_models[root] );
		new_model = asgtk_dir_tree_get_model( ASGTK_DIR_TREE(data->target_dirlist) );
		if( data->root_models[root] != new_model )
		{
			if( data->root_models[root]	)
				g_object_unref( data->root_models[root] );
			data->root_models[root] = new_model ; 
		}else
	  		 g_object_unref( new_model );	 
		if( data->root_paths[root] == NULL ) 
			data->root_paths[root] = gtk_tree_path_new_first ();
		asgtk_dir_tree_restore_curr_path( ASGTK_DIR_TREE(data->target_dirlist), data->root_paths[root] );				   
	}else
		asgtk_dir_tree_set_root( ASGTK_DIR_TREE(data->target_dirlist), new_root, NULL );

	if( new_root ) 
		free( new_root ); 
}

void 
root_selection_changed( GtkAction *action, GtkRadioAction *current, ASFileBrowserRootSelFrame *data )
{
	ASFileBrowserRoot root = gtk_radio_action_get_current_value(current);
	change_root_to( data, root );
}

void on_hide_contents_toggle(GtkToggleButton *hide_button, ASFileBrowserRootSelFrame *data)
{
	if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(hide_button) ) ) 		
	{	
		gtk_widget_hide (data->table);
		change_root_to( data, root_Root );
	}else
	{	
		gtk_widget_show (data->table);
		change_root_to( data, data->last_nonroot_root );
	}
}	 


GtkWidget *
build_root_selection_frame(ASFileBrowserRootSelFrame *data, GtkWidget *dirlist)
{
	GtkTable *table;
	GtkWidget *btn ;
	GtkActionGroup *action_group ;
	GtkWidget *path_combo ;
	GtkWidget *path_entry = NULL;
	GtkWidget* file_chooser_btn ;

#define ROOT_SELECTION_ENTRIES_NUM	6
	static GtkRadioActionEntry root_sel_entries[ROOT_SELECTION_ENTRIES_NUM] = {
		{"root_PrivateAfterStep", NULL, "Private AfterStep", NULL, 
			"Private directory conmtaining AfterStep files. Typically ~/.afterstep", 
			root_PrivateAfterStep},	  
		{"root_SharedAfterStep", NULL, "Shared AfterStep" , NULL, 
			"System-wide shared directory conmtaining AfterStep files. Typically /usr/local/share/afterstep", 
			root_SharedAfterStep},	
		{"root_Home", NULL, "Home"             , NULL, 
			"Private Home directory", 
			root_Home},	
		{"root_UsrShare", NULL, "/usr/share"       , NULL, 
			"System-wide shared files directory /usr/share", 
			root_UsrShare},	 
		{"root_UsrLocalShare", NULL, "/usr/local/share" , NULL, 
			"System-wide shared files directory /usr/local/share", 	
			root_UsrLocalShare},	  
		{"root_Other", NULL, "Other : "         , NULL, 
			"Custom location in the filesystem tree", 				
			root_Other},	  
	} ;
	int root_sel_cells[ROOT_SELECTION_ENTRIES_NUM][4] = 
	{	{0, 1, 0, 1},	
	 	{1, 2, 0, 1},
	 	{2, 3, 0, 1},
	 	{3, 4, 0, 1},
	 	{4, 5, 0, 1},
	 	{0, 1, 1, 2}
	};	 
	int i ;
	GtkWidget *frame = gtk_frame_new( NULL );
	GtkWidget *hbox = gtk_hbox_new( FALSE, 0 );
	GtkWidget *label = gtk_label_new( "Select directory tree to browse : ");
	GtkWidget *checkbox = gtk_check_button_new_with_label( "( hide and show entire filesystem )" );


	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
	gtk_box_pack_end (GTK_BOX (hbox), checkbox, TRUE, TRUE, 5);
	gtk_widget_show_all (hbox);
	gtk_widget_show (hbox);
	gtk_frame_set_label_widget( GTK_FRAME(frame), hbox );

	table = GTK_TABLE(gtk_table_new( 5, 2, FALSE ));
	g_signal_connect ((gpointer) checkbox, "clicked", G_CALLBACK (on_hide_contents_toggle), data);
	
	gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET(table));
	gtk_container_set_border_width( GTK_CONTAINER (frame), 5 );
	gtk_container_set_border_width( GTK_CONTAINER (table), 5 );
	gtk_table_set_row_spacings( table, 5 );
	gtk_table_set_col_spacings( table, 5 );

	action_group = gtk_action_group_new( "RootSelection" );
	gtk_action_group_add_radio_actions( action_group, root_sel_entries, ROOT_SELECTION_ENTRIES_NUM, 
										root_PrivateAfterStep, G_CALLBACK(root_selection_changed), data );

	for( i = 0 ; i  < ROOT_SELECTION_ENTRIES_NUM ; ++i ) 
	{	
		btn = gtk_toggle_button_new();
		gtk_table_attach_defaults (table, btn,  root_sel_cells[i][0], root_sel_cells[i][1], 
												root_sel_cells[i][2], root_sel_cells[i][3]);	
		gtk_action_connect_proxy(gtk_action_group_get_action(action_group,root_sel_entries[i].name), btn );
	}

	path_combo = gtk_combo_box_entry_new_text();
	colorize_gtk_edit(path_combo);

	file_chooser_btn = gtk_button_new_with_label( "Browse" );
	colorize_gtk_edit(path_combo);
	hbox = gtk_hbox_new( FALSE, 0 );
	gtk_box_pack_start (GTK_BOX (hbox), path_combo, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), file_chooser_btn, FALSE, FALSE, 0);
	gtk_widget_show_all (hbox);

	gtk_table_attach_defaults (table, hbox,  1, 5, 1, 2 );
	
	if( GTK_IS_CONTAINER(path_combo) )
		gtk_container_forall( GTK_CONTAINER(path_combo), find_combobox_entry, &path_entry );
	/* if above succeeded then path_entry should be not NULL here : */
	/* TODO : insert proper change handlers and data pointers here : */
	if( path_entry ) 
		g_signal_connect ( G_OBJECT (path_entry), "activate",
		      			   G_CALLBACK (NULL), (gpointer) NULL);
	g_signal_connect (G_OBJECT(path_combo), "changed",
			  			G_CALLBACK (NULL), (gpointer) NULL);

	gtk_widget_show_all (GTK_WIDGET(table));
	gtk_widget_show (GTK_WIDGET(table));
	gtk_widget_set_size_request ( frame, -1, -1);
	colorize_gtk_widget( frame, get_colorschemed_style_normal() );

	data->table 		   = GTK_WIDGET(table) ;
	data->action_group 	   = action_group ;
	data->path_combo       = path_combo ;
	data->path_entry       = path_entry ;
	data->file_chooser_btn = file_chooser_btn ;
	data->target_dirlist   = dirlist ;

	change_root_to( data, root_PrivateAfterStep );
	   
	return frame;
}	   

GtkWidget *
asgtk_text_view_new()
{
	
	return gtk_text_view_new();	
}

GtkWidget *
asgtk_hex_view_new()
{
  	return gtk_text_view_new();		
}

static void 
dir_tree_sel_handler( ASGtkDirTree *dt, gpointer user_data)
{
	ASFileBrowserMainFrame *data = (ASFileBrowserMainFrame *)user_data;
	char *curr_path = asgtk_dir_tree_get_selection( dt );
	if( curr_path ) 
	{
		asgtk_image_dir_set_path(ASGTK_IMAGE_DIR(data->filelist), curr_path );
		free( curr_path );		
	}	 
}

static void
filelist_sel_handler(ASGtkImageDir *id, gpointer user_data)
{
	ASFileBrowserMainFrame *data = (ASFileBrowserMainFrame *)user_data;
	if( data ) 
	{	
		ASImageListEntry *le = asgtk_image_dir_get_selection( id ); 
		asgtk_image_view_set_entry ( ASGTK_IMAGE_VIEW(data->view_image), le);
		if( le )
		{
			Bool bin = (le->type == ASIT_Xpm       || le->type == ASIT_XMLScript ||
						le->type == ASIT_HTML || le->type == ASIT_XML ); 

			if( le->type == ASIT_Unknown ) 
				gtk_widget_hide( data->view_image );
			else
				gtk_widget_show( data->view_image );
			
			load_asimage_list_entry_data( le, DEFAULT_MAX_TEXT_SIZE ); 
			if( le->type == ASIT_Unknown ) 
			{
				int i = le->buffer_size ; 
				register char *ptr = le->buffer ;
				while ( --i >= 0 )	if( ptr[i] == '\0' || iscntrl(ptr[i]) )	break;
				bin = (i >= 0);				
			}	 
			if( !bin )
			{                  /* use text view */
				gtk_widget_show( data->view_text );
				gtk_widget_hide( data->view_hex );
				gtk_text_buffer_set_text( data->text_buffer, le->buffer, le->buffer_size );
				gtk_text_view_set_buffer( GTK_TEXT_VIEW(data->view_text), data->text_buffer );
			}else
			{				   /* use hex view */	
				gtk_widget_show( data->view_hex );
				gtk_widget_hide( data->view_text );
				gtk_text_view_set_buffer( GTK_TEXT_VIEW(data->view_hex), data->text_buffer );
			}	 
			
			unref_asimage_list_entry( le );
		}
	}
}


GtkWidget *
build_main_frame(ASFileBrowserMainFrame *data)
{
	GtkWidget *h_paned ; 
	GtkWidget *v_paned ; 
	GtkWidget *view_tabs ;
	GtkWidget *view_image ;
	GtkWidget *view_text ;
	GtkWidget *view_hex ;
	GtkWidget *dirlist ;
	GtkWidget *filelist ;
	GtkWidget *frame = gtk_frame_new( NULL );
	
	h_paned = gtk_hpaned_new();
	gtk_container_add (GTK_CONTAINER (frame), h_paned);
	
	v_paned = gtk_vpaned_new();
	gtk_paned_add1 (GTK_PANED (h_paned), v_paned);

	view_tabs = gtk_notebook_new();
	gtk_paned_add2 (GTK_PANED (h_paned), view_tabs);
	
	view_image = asgtk_image_view_new();
	gtk_widget_set_size_request (view_image, PREVIEW_WIDTH, PREVIEW_HEIGHT);
	asgtk_image_view_set_resize ( ASGTK_IMAGE_VIEW(view_image), 0/*ASGTK_IMAGE_VIEW_SCALE_TO_VIEW*/, ASGTK_IMAGE_VIEW_RESIZE_ALL );
	gtk_notebook_append_page (GTK_NOTEBOOK (view_tabs), view_image, gtk_label_new("AS image"));
	
	view_text = asgtk_text_view_new();
	gtk_notebook_append_page (GTK_NOTEBOOK (view_tabs), view_text, gtk_label_new("AS text"));

	view_hex = asgtk_hex_view_new();
	gtk_notebook_append_page (GTK_NOTEBOOK (view_tabs), view_hex, gtk_label_new("AS hex"));

	gtk_widget_show_all (view_tabs);

	dirlist = asgtk_dir_tree_new();
	gtk_widget_set_size_request (dirlist, DIR_TREE_WIDTH, DIR_TREE_HEIGHT);
	gtk_paned_add1 (GTK_PANED (v_paned), dirlist);

	filelist = asgtk_image_dir_new();
	asgtk_image_dir_set_columns( ASGTK_IMAGE_DIR(filelist), 
									ASGTK_ImageDir_Col_Name|
									ASGTK_ImageDir_Col_Type|
									ASGTK_ImageDir_Col_Size|
									ASGTK_ImageDir_Col_Date );
	asgtk_image_dir_set_list_all( ASGTK_IMAGE_DIR(filelist), True );
	gtk_widget_set_size_request (filelist, FILE_LIST_WIDTH, FILE_LIST_HEIGHT);
	gtk_paned_add2 (GTK_PANED (v_paned), filelist);

	gtk_widget_show_all (v_paned);
	gtk_widget_show_all (h_paned);
	gtk_widget_show (h_paned);
	
	colorize_gtk_widget( frame, get_colorschemed_style_normal() );
	
	data->view_tabs = view_tabs ;
	data->view_image= view_image ;
	data->view_text = view_text ;
	data->view_hex  = view_hex ;
	data->dirlist   = dirlist ;
	data->filelist  = filelist ;
	
	asgtk_dir_tree_set_sel_handler(ASGTK_DIR_TREE(dirlist), dir_tree_sel_handler, data);
	asgtk_image_dir_set_sel_handler( ASGTK_IMAGE_DIR(filelist), filelist_sel_handler, data);

	data->text_buffer = gtk_text_buffer_new(NULL); 

	return frame;
}

void
create_main_window (void)
{
    GtkWidget *main_vbox;
	GtkWidget *root_sel_frame ; 
	GtkWidget *main_frame ; 
	ASFileBrowserMainFrame *main_frame_data = safecalloc( 1, sizeof(ASFileBrowserMainFrame));
	ASFileBrowserRootSelFrame *root_sel_frame_data = safecalloc( 1, sizeof(ASFileBrowserRootSelFrame));

  	AppState.main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  	gtk_window_set_title (GTK_WINDOW (AppState.main_window), "AfterStep File Browser");

	colorize_gtk_window( AppState.main_window ); 	  

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (AppState.main_window), main_vbox);

	main_frame = build_main_frame(main_frame_data);
	root_sel_frame = build_root_selection_frame(root_sel_frame_data, main_frame_data->dirlist);
	  	
	gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET(root_sel_frame), FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (main_vbox), main_frame, TRUE, TRUE, 0);
  	gtk_widget_show_all (main_vbox);
  	gtk_widget_show (main_vbox);
 	
}

void
on_destroy(GtkWidget *widget, gpointer user_data)
{
	if( AppState.xml_editor ) 
		gtk_widget_destroy( GTK_WIDGET(AppState.xml_editor) );
		
	gtk_main_quit();
}

void 
init_ASFileBrowser()
{
	memset( &AppState, 0x00, sizeof(AppState));
	
	create_main_window(); 
#if 0	
	create_backs_list();
	create_list_preview();
	
	AppState.sel_apply_button = asgtk_add_button_to_box( NULL, GTK_STOCK_APPLY, NULL, G_CALLBACK(on_list_apply_clicked), AppState.backs_list );
	AppState.make_xml_button = asgtk_add_button_to_box( NULL, GTK_STOCK_PROPERTIES, "Make XML", G_CALLBACK(on_make_xml_clicked), AppState.backs_list );
	AppState.edit_xml_button = asgtk_add_button_to_box( NULL, GTK_STOCK_PROPERTIES, "Edit XML", G_CALLBACK(on_edit_xml_clicked), AppState.backs_list );
	AppState.make_mini_button = asgtk_add_button_to_box( NULL, GTK_STOCK_PROPERTIES, "Make mini", G_CALLBACK(on_make_mini_clicked), AppState.backs_list );
	
	AppState.sel_del_button = asgtk_add_button_to_box( NULL, GTK_STOCK_DELETE, NULL, G_CALLBACK(on_list_del_clicked), AppState.backs_list );

	gtk_widget_hide(AppState.edit_xml_button);

	asgtk_image_view_add_tool( ASGTK_IMAGE_VIEW(AppState.list_preview), AppState.sel_apply_button, 0 );
	asgtk_image_view_add_tool( ASGTK_IMAGE_VIEW(AppState.list_preview), AppState.make_mini_button, 5 );
	asgtk_image_view_add_tool( ASGTK_IMAGE_VIEW(AppState.list_preview), AppState.make_xml_button, 5 );
	asgtk_image_view_add_tool( ASGTK_IMAGE_VIEW(AppState.list_preview), AppState.edit_xml_button, 5 );
	asgtk_image_view_add_tool( ASGTK_IMAGE_VIEW(AppState.list_preview), AppState.sel_del_button, 5 );
	
	asgtk_image_dir_set_sel_handler( ASGTK_IMAGE_DIR(AppState.backs_list), backs_list_sel_handler, AppState.list_preview);

	reload_private_backs_list();

#endif	
	g_signal_connect (G_OBJECT (AppState.main_window), "destroy", G_CALLBACK (on_destroy), NULL);
  	gtk_widget_show (AppState.main_window);
}	 

