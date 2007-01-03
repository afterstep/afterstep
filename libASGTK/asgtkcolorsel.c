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

#define COLOR_LIST_WIDTH  150
#define COLOR_LIST_HEIGHT 210

/*  local function prototypes  */
static void asgtk_color_selection_class_init (ASGtkColorSelectionClass *klass);
static void asgtk_color_selection_init (ASGtkColorSelection *ib);
static void asgtk_color_selection_dispose (GObject *object);
static void asgtk_color_selection_finalize (GObject *object);
static void asgtk_color_selection_style_set (GtkWidget *widget, GtkStyle  *prev_style);

/*  private variables  */
static GtkDialogClass *parent_class = NULL;

GType
asgtk_color_selection_get_type (void)
{
  	static GType ib_type = 0;

  	if (!ib_type)
    {
    	static const GTypeInfo ib_info =
      	{
        	sizeof (ASGtkColorSelectionClass),
        	(GBaseInitFunc)     NULL,
        	(GBaseFinalizeFunc) NULL,
			(GClassInitFunc)    asgtk_color_selection_class_init,
        	NULL,           /* class_finalize */
        	NULL,           /* class_data     */
        	sizeof (ASGtkColorSelection),
        	0,              /* n_preallocs    */
        	(GInstanceInitFunc) asgtk_color_selection_init,
      	};

      	ib_type = g_type_register_static (	GTK_TYPE_DIALOG,
        	                                "ASGtkColorSelection",
            	                            &ib_info, 0);
    }

  	return ib_type;
}

static void
asgtk_color_selection_class_init (ASGtkColorSelectionClass *klass)
{
  	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->dispose   = asgtk_color_selection_dispose;
  	object_class->finalize  = asgtk_color_selection_finalize;

  	widget_class->style_set = asgtk_color_selection_style_set;

}

static void
asgtk_color_selection_init (ASGtkColorSelection *id)
{
}

static void
asgtk_color_selection_dispose (GObject *object)
{
  	ASGtkColorSelection *cs = ASGTK_COLOR_SELECTION (object);
	destroy_string( &(cs->curr_color_name) );
	
  	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
asgtk_color_selection_finalize (GObject *object)
{
  	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
asgtk_color_selection_style_set (GtkWidget *widget,
                          GtkStyle  *prev_style)
{
  /* ASGtkColorSelection *id = ASGTK_IMAGE_BROWSER (widget); */

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}

static void
asgtk_color_selection_cs_activate (GtkTreeView       *tree_view,
	   							  GtkTreePath       *path,
				 				  GtkTreeViewColumn *column,
				 				  gpointer           user_data)
{
  	ASGtkColorSelection *cs = ASGTK_COLOR_SELECTION (user_data);
  	GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  	GtkTreeIter iter;
  	char *colorname ;

  	gtk_tree_model_get_iter (model, &iter, path);
  	gtk_tree_model_get (model, &iter, 0, &colorname, -1);
	LOCAL_DEBUG_OUT( "colorname == \"%s\"", colorname );	
	asgtk_color_selection_set_color_by_name( cs, colorname );
}

static void
asgtk_color_selection_cs_select (GtkTreeSelection *selection, gpointer user_data)
{
  	ASGtkColorSelection *cs = ASGTK_COLOR_SELECTION (user_data);
  	char *colorname ;
	GtkTreeIter iter;
	GtkTreeModel *model;

  	if (gtk_tree_selection_get_selected (selection, &model, &iter)) 
	{
	  	gtk_tree_model_get (model, &iter, 0, &colorname, -1);
		LOCAL_DEBUG_OUT( "colorname == \"%s\"", colorname );	
		asgtk_color_selection_set_color_by_name( cs, colorname );
	}
}


static GtkWidget * 
asgtk_color_selection_create_cs_list( ASGtkColorSelection *cs )
{
	GtkWidget *scrolled_win, *model ;
	GtkTreeViewColumn *column ;
	GtkTreeIter iter;
	int i ;
		
	model = GTK_WIDGET(gtk_list_store_new (1, G_TYPE_STRING));
  	cs->cs_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
  	g_object_unref (model);
		
	column = gtk_tree_view_column_new_with_attributes (
				"Current AfterStep Colors", gtk_cell_renderer_text_new (), "text", 0, 
				NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  	gtk_tree_view_append_column (GTK_TREE_VIEW (cs->cs_list), column);

  	gtk_widget_set_size_request (cs->cs_list, COLOR_LIST_WIDTH, COLOR_LIST_HEIGHT);
	g_signal_connect ( cs->cs_list, "row_activated",
						G_CALLBACK (asgtk_color_selection_cs_activate), cs);
	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (cs->cs_list)), "changed",
		    		  G_CALLBACK (asgtk_color_selection_cs_select), cs);
	
  	scrolled_win = ASGTK_SCROLLED_WINDOW(GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS,GTK_SHADOW_IN);
	ASGTK_CONTAINER_ADD(scrolled_win, cs->cs_list);

  	gtk_widget_show (scrolled_win);
	colorize_gtk_tree_view_window( GTK_WIDGET(scrolled_win) );

	for( i = 0 ; i < ASMC_MainColors ; ++i ) 
	{
		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0, ASMainColorNames[i], -1);
	}	 

	return scrolled_win;
}

/*  public functions  */
GtkWidget * 
asgtk_color_selection_new ()
{
	ASGtkColorSelection *cs;
    GtkWidget *main_vbox, *main_hbox;
	GtkWidget *scrolled_window ;

    cs = g_object_new (ASGTK_TYPE_COLOR_SELECTION, NULL);
	colorize_gtk_window( GTK_WIDGET(cs) );	
	gtk_container_set_border_width( GTK_CONTAINER (cs), 5 );
	
	main_vbox = GTK_DIALOG(cs)->vbox ; 

	main_hbox = gtk_hbox_new (FALSE, 0);
  	gtk_widget_show (main_hbox);
	gtk_box_pack_start (GTK_BOX (main_vbox), main_hbox, TRUE, TRUE, 0);
	
	gtk_box_set_spacing( GTK_BOX(main_hbox), 5 );


	scrolled_window = asgtk_color_selection_create_cs_list( cs );
	gtk_box_pack_start (GTK_BOX (main_hbox), scrolled_window, FALSE, FALSE, 0);

	cs->color_sel = gtk_color_selection_new();
	gtk_widget_show( cs->color_sel );
	gtk_box_pack_end (GTK_BOX (main_hbox), cs->color_sel, TRUE, TRUE, 0);

	LOCAL_DEBUG_OUT( "created image ASGtkColorSelection object %p", cs );	
	return GTK_WIDGET (cs);
}

GtkWidget*
asgtk_color_selection_add_main_button( ASGtkColorSelection *cs, const char *stock, GCallback func, gpointer user_data )
{
	if(!ASGTK_IS_COLOR_SELECTION (cs))
		return NULL;
	
	//gtk_widget_show (cs->separator);
	return asgtk_add_button_to_box( GTK_BOX (GTK_DIALOG(cs)->action_area), stock, NULL, func, user_data );   
}	   

void
asgtk_color_selection_set_color_by_name( ASGtkColorSelection *cs, const char *colorname )
{
	GdkColor gc ;
	ARGB32 argb ;
	
	if(!ASGTK_IS_COLOR_SELECTION (cs))
		return ;
	
	if( parse_argb_color( colorname, &argb ) != colorname )
	{
		gc.red = ARGB32_RED16(argb);
		gc.green = ARGB32_GREEN16(argb);
		gc.blue = ARGB32_BLUE16(argb) ;
		gc.pixel = argb ;
		gtk_color_selection_set_current_color (GTK_COLOR_SELECTION(cs->color_sel), &gc);
		gtk_color_selection_set_current_alpha( GTK_COLOR_SELECTION(cs->color_sel), ARGB32_ALPHA16(argb) );
		
		if( cs->curr_color_name ) 
			free( cs->curr_color_name );
		cs->curr_color_name = mystrdup( colorname );
	}
}

char *
asgtk_color_selection_get_color_str(ASGtkColorSelection *cs)
{
	GdkColor gc ;
	int alpha ; 
	
	if(!ASGTK_IS_COLOR_SELECTION (cs))
		return NULL;

	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION(cs->color_sel), &gc );
	alpha = gtk_color_selection_get_current_alpha( GTK_COLOR_SELECTION(cs->color_sel) );
	
	if( cs->curr_color_name	 )
	{
		ARGB32 argb ;
		if( parse_argb_color( cs->curr_color_name, &argb ) != cs->curr_color_name )
		{
			if( ARGB32_RED16(argb) == gc.red && 
				ARGB32_GREEN16(argb) == gc.green &&
				ARGB32_BLUE16(argb) == gc.blue && 
				ARGB32_ALPHA16(argb) == alpha ) 
				return mystrdup(cs->curr_color_name);
		}
	}	 
	return GdkColor2string( &gc, alpha );
}	 

