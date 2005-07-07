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
#include "../libAfterStep/colorscheme.h"


#include "asgtk.h"
#include "asgtkai.h"
#include "asgtkcolorsel.h"
#include "asgtkgradient.h"

#define COLOR_LIST_WIDTH  150
#define COLOR_LIST_HEIGHT 210
#define COLOR_PREVIEW_WIDTH 	48
#define COLOR_PREVIEW_HEIGHT 	24
#define PREVIEW_WIDTH  320
#define PREVIEW_HEIGHT 240

/*  local function prototypes  */
static void asgtk_gradient_class_init (ASGtkGradientClass *klass);
static void asgtk_gradient_init (ASGtkGradient *ib);
static void asgtk_gradient_dispose (GObject *object);
static void asgtk_gradient_finalize (GObject *object);
static void asgtk_gradient_style_set (GtkWidget *widget, GtkStyle  *prev_style);

/*  private variables  */
static GtkDialogClass *parent_class = NULL;

GType
asgtk_gradient_get_type (void)
{
  	static GType ib_type = 0;

  	if (!ib_type)
    {
    	static const GTypeInfo ib_info =
      	{
        	sizeof (ASGtkGradientClass),
        	(GBaseInitFunc)     NULL,
        	(GBaseFinalizeFunc) NULL,
			(GClassInitFunc)    asgtk_gradient_class_init,
        	NULL,           /* class_finalize */
        	NULL,           /* class_data     */
        	sizeof (ASGtkGradient),
        	0,              /* n_preallocs    */
        	(GInstanceInitFunc) asgtk_gradient_init,
      	};

      	ib_type = g_type_register_static (	GTK_TYPE_DIALOG,
        	                                "ASGtkGradient",
            	                            &ib_info, 0);
    }

  	return ib_type;
}

static void
asgtk_gradient_class_init (ASGtkGradientClass *klass)
{
  	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->dispose   = asgtk_gradient_dispose;
  	object_class->finalize  = asgtk_gradient_finalize;

  	widget_class->style_set = asgtk_gradient_style_set;

}

static void
asgtk_gradient_init (ASGtkGradient *id)
{
}

static void
asgtk_gradient_dispose (GObject *object)
{
  	ASGtkGradient *cs = ASGTK_GRADIENT (object);
	if( cs->color_sel ) 
	{	
		gtk_widget_destroy( cs->color_sel );
		cs->color_sel = NULL ;
	}
	
  	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
asgtk_gradient_finalize (GObject *object)
{
  	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
asgtk_gradient_style_set (GtkWidget *widget,
                          GtkStyle  *prev_style)
{
  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}

static void
asgtk_gradient_color_activate (GtkTreeView       *tree_view,
	   							  GtkTreePath       *path,
				 				  GtkTreeViewColumn *column,
				 				  gpointer           user_data)
{
//  	ASGtkGradient *ge = ASGTK_GRADIENT (user_data);
  	GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  	GtkTreeIter iter;
  	char *colorname ;

  	gtk_tree_model_get_iter (model, &iter, path);
  	gtk_tree_model_get (model, &iter, 0, &colorname, -1);
	LOCAL_DEBUG_OUT( "colorname == \"%s\"", colorname );	
	/* TODO  */
}

static void
asgtk_gradient_color_select (GtkTreeSelection *selection, gpointer user_data)
{
//  	ASGtkGradient *ge = ASGTK_GRADIENT (user_data);
  	char *colorname ;
	GtkTreeIter iter;
	GtkTreeModel *model;

  	if (gtk_tree_selection_get_selected (selection, &model, &iter)) 
	{
	  	gtk_tree_model_get (model, &iter, 0, &colorname, -1);
		LOCAL_DEBUG_OUT( "colorname == \"%s\"", colorname );	
 		/* TODO  */	
	}
}


static GtkWidget * 
asgtk_gradient_create_color_list( ASGtkGradient *ge )
{
	GtkWidget *scrolled_win, *model ;
	GtkTreeViewColumn *column ;
//	GtkTreeIter iter;
//	int i ;
		
	model = GTK_WIDGET(gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_FLOAT));
  	ge->point_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
  	g_object_unref (model);
		
	column = gtk_tree_view_column_new_with_attributes (
				"point color", gtk_cell_renderer_text_new (), "text", 0, 
				NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  	gtk_tree_view_append_column (GTK_TREE_VIEW (ge->point_list), column);

  	gtk_widget_set_size_request (ge->point_list, COLOR_LIST_WIDTH, COLOR_LIST_HEIGHT);
	g_signal_connect ( ge->point_list, "row_activated",
						G_CALLBACK (asgtk_gradient_color_activate), ge);
	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (ge->point_list)), "changed",
		    		  G_CALLBACK (asgtk_gradient_color_select), ge);
	
  	scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), GTK_SHADOW_IN);  
  	gtk_container_add (GTK_CONTAINER (scrolled_win), ge->point_list);
  	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
									GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  	gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 0);
	
  	gtk_widget_show (ge->point_list);
	colorize_gtk_tree_view_window( GTK_WIDGET(scrolled_win) );

	/* TODO init gradient points list maybe: */
	//for( i = 0 ; i < ASMC_MainColors ; ++i ) 
	//{
	//	gtk_list_store_append (GTK_LIST_STORE(model), &iter);
	//	gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0, ASMainColorNames[i], -1);
	//}	 

	return scrolled_win;
}

void 
on_color_preview_clicked( GtkWidget *btn, gpointer data )
{
	g_signal_stop_emission_by_name ((gpointer)btn,"clicked");
	asgtk_warning2( data, "blah Blah", NULL, NULL );
}

static void
update_color_preview( ASGtkGradient *ge, const char *color ) 
{
	ARGB32 argb ; 
	if( parse_argb_color( color, &argb ) != color ) 
	{
		GdkPixbuf *pb = solid_color2GdkPixbuf( argb, COLOR_PREVIEW_WIDTH-4, COLOR_PREVIEW_HEIGHT-4 );
		if( pb ) 
		{	
			gtk_image_set_from_pixbuf( GTK_IMAGE(ge->color_preview), pb );
			gdk_pixbuf_unref( pb ); 		   
		}
 	}
}
/*  public functions  */
GtkWidget * 
asgtk_gradient_new ()
{
	ASGtkGradient *ge;
    GtkWidget *main_vbox, *main_hbox;
	GtkWidget *scrolled_window ;
	GtkWidget *list_vbox ;
	GtkWidget *frame, *hbox, *vbox, *btn ; 

    ge = g_object_new (ASGTK_TYPE_GRADIENT, NULL);
	colorize_gtk_window( GTK_WIDGET(ge) );	
	gtk_container_set_border_width( GTK_CONTAINER (ge), 5 );
	
	main_vbox = GTK_DIALOG(ge)->vbox ; 

	main_hbox = gtk_hbox_new (FALSE, 5);
  	gtk_widget_show (main_hbox);
	gtk_box_pack_start (GTK_BOX (main_vbox), main_hbox, TRUE, TRUE, 0);
	
	list_vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (main_hbox), list_vbox, TRUE, TRUE, 0);

	ge->width_entry = gtk_spin_button_new_with_range( 1, 10000, 1 );
	ge->height_entry = gtk_spin_button_new_with_range( 1, 10000, 1 );
	
	ge->size_frame = gtk_frame_new("Rendered gradient size : ");
	gtk_box_pack_start (GTK_BOX (list_vbox), ge->size_frame, FALSE, FALSE, 5);
	colorize_gtk_tree_view_window( GTK_WIDGET(ge->size_frame) );

	hbox = gtk_hbox_new( FALSE, 5 );			   
	gtk_container_set_border_width( GTK_CONTAINER (hbox), 3 );
	gtk_container_add( GTK_CONTAINER(ge->size_frame), hbox );
	
	gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new("Width : "), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), ge->width_entry, FALSE, FALSE, 0);
	
	gtk_box_pack_end (GTK_BOX (hbox), ge->height_entry, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), gtk_label_new("Height : "), FALSE, FALSE, 0);
	gtk_widget_show_all (hbox);
	gtk_widget_show (hbox);

	scrolled_window = asgtk_gradient_create_color_list( ge );
	gtk_box_pack_start (GTK_BOX (list_vbox), scrolled_window, FALSE, FALSE, 0);
	
	ge->color_entry = gtk_entry_new_with_max_length(24);
	gtk_entry_set_width_chars( GTK_ENTRY(ge->color_entry), 16 );
#if 0
	ge->color_preview = gtk_color_button_new();
	g_signal_connect ((gpointer) ge->color_preview, "clicked", G_CALLBACK (on_color_preview_clicked), ge);
#else
	ge->color_preview = gtk_image_new();
	update_color_preview( ge, "#FFFF0000" );
#endif

	ge->offset_entry = gtk_spin_button_new_with_range( 0., 1., 0.01 );


	frame = gtk_frame_new("Change point attributes : ");
	gtk_box_pack_end (GTK_BOX (list_vbox), frame, FALSE, FALSE, 5);
	colorize_gtk_tree_view_window( GTK_WIDGET(frame) );

	vbox = gtk_vbox_new( FALSE, 5 );			   
	gtk_container_add( GTK_CONTAINER(frame), vbox );
	hbox = gtk_hbox_new( FALSE, 5 );			   
	gtk_container_set_border_width( GTK_CONTAINER (hbox), 3 );
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new("Color : "), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), ge->color_entry, FALSE, FALSE, 0);

	frame = gtk_frame_new(NULL);
	colorize_gtk_tree_view_window( GTK_WIDGET(frame) );
	gtk_widget_set_size_request( frame, COLOR_PREVIEW_WIDTH, COLOR_PREVIEW_HEIGHT );
	gtk_container_add( GTK_CONTAINER(frame), ge->color_preview );
	gtk_widget_show( ge->color_preview );
	gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
	
	gtk_box_pack_end (GTK_BOX (hbox), ge->offset_entry, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), gtk_label_new("Offset : "), FALSE, FALSE, 0);
	gtk_widget_show_all (hbox);
	gtk_widget_show (hbox);

	hbox = gtk_hbox_new( FALSE, 5 );			   
	gtk_container_set_border_width( GTK_CONTAINER (hbox), 3 );
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	btn = gtk_button_new_with_label(" Color selector ");
	colorize_gtk_widget( GTK_WIDGET(btn), get_colorschemed_style_button());
	gtk_box_pack_start (GTK_BOX (hbox), btn, FALSE, FALSE, 0);
	
	btn = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	colorize_gtk_widget( GTK_WIDGET(btn), get_colorschemed_style_button());
	gtk_box_pack_end (GTK_BOX (hbox), btn, FALSE, FALSE, 0);
	btn = gtk_button_new_from_stock(GTK_STOCK_ADD);
	colorize_gtk_widget( GTK_WIDGET(btn), get_colorschemed_style_button());
	gtk_box_pack_end (GTK_BOX (hbox), btn, FALSE, FALSE, 0);
	btn = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	colorize_gtk_widget( GTK_WIDGET(btn), get_colorschemed_style_button());
	gtk_box_pack_end (GTK_BOX (hbox), btn, FALSE, FALSE, 0);


	gtk_widget_show_all (hbox);
	gtk_widget_show (hbox);

	/* The preview : */
	ge->image_view = ASGTK_IMAGE_VIEW(asgtk_image_view_new());
	gtk_widget_set_size_request (GTK_WIDGET(ge->image_view), PREVIEW_WIDTH, PREVIEW_HEIGHT);
	gtk_box_pack_end (GTK_BOX (main_hbox), GTK_WIDGET(ge->image_view), TRUE, TRUE, 0);


  	gtk_widget_show_all (list_vbox);
  	gtk_widget_show_all (main_hbox);
	gtk_widget_hide(ge->image_view->details_frame);

	/* TODO add the rest of controls : */
#if 0
	{
		GdkColor gc ; 
		gc.red = 0xFFFF ; 
		gc.green = 0 ; 
		gc.blue = 0 ; 
		gtk_widget_modify_bg( ge->color_preview, GTK_STATE_NORMAL, &gc );
		gtk_widget_modify_bg( ge->color_preview, GTK_STATE_ACTIVE, &gc );
		gtk_widget_modify_bg( ge->color_preview, GTK_STATE_PRELIGHT, &gc );
		gtk_widget_modify_bg( ge->color_preview, GTK_STATE_SELECTED, &gc );
		gtk_widget_modify_bg( ge->color_preview, GTK_STATE_INSENSITIVE, &gc );
		gtk_widget_modify_fg( ge->color_preview, GTK_STATE_NORMAL, &gc );
		gtk_widget_modify_fg( ge->color_preview, GTK_STATE_ACTIVE, &gc );
		gtk_widget_modify_fg( ge->color_preview, GTK_STATE_PRELIGHT, &gc );
		gtk_widget_modify_fg( ge->color_preview, GTK_STATE_SELECTED, &gc );
		gtk_widget_modify_fg( ge->color_preview, GTK_STATE_INSENSITIVE, &gc );
	}
#endif
	LOCAL_DEBUG_OUT( "created image ASGtkGradient object %p", ge );	
	return GTK_WIDGET (ge);
}

