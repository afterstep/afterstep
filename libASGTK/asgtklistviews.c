/* 
 * Copyright (C) 2006 Sasha Vasko <sasha at aftercode.net>
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
#include "../libAfterStep/parser.h"

#include <unistd.h>		   

#include "asgtk.h"
#include "asgtkai.h"
#include "asgtklistviews.h"

/*  local function prototypes  */
static void asgtk_simple_list_class_init (ASGtkSimpleListClass *klass);
static void asgtk_simple_list_init (ASGtkSimpleList *self);
static void asgtk_simple_list_dispose (GObject *object);
static void asgtk_simple_list_finalize (GObject *object);
static void asgtk_simple_list_style_set (GtkWidget *widget, GtkStyle  *prev_style);


/*  private variables  */
static GtkTreeViewClass *simple_list_parent_class = NULL;

GType
asgtk_simple_list_get_type (void)
{
  	static GType id_type = 0;

  	if (! id_type)
    {
    	static const GTypeInfo id_info =
      	{
        	sizeof (ASGtkSimpleListClass),
        	(GBaseInitFunc)     NULL,
        	(GBaseFinalizeFunc) NULL,
			(GClassInitFunc)    asgtk_simple_list_class_init,
        	NULL,           /* class_finalize */
        	NULL,           /* class_data     */
        	sizeof (ASGtkSimpleList),
        	0,              /* n_preallocs    */
        	(GInstanceInitFunc) asgtk_simple_list_init,
      	};

      	id_type = g_type_register_static (	GTK_TYPE_TREE_VIEW,
        	                                "ASGtkSimpleList",
            	                            &id_info, 0);
    }

  	return id_type;
}

static void
asgtk_simple_list_class_init (ASGtkSimpleListClass *klass)
{
  	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  	simple_list_parent_class = g_type_class_peek_parent (klass);

  	object_class->dispose   = asgtk_simple_list_dispose;
  	object_class->finalize  = asgtk_simple_list_finalize;

  	widget_class->style_set = asgtk_simple_list_style_set;

}

static void
asgtk_simple_list_init (ASGtkSimpleList *self)
{
}

static void
asgtk_simple_list_dispose (GObject *object)
{
  	ASGtkSimpleList *self = ASGTK_SIMPLE_LIST (object);
	destroy_string(&self->curr_selection);
  	G_OBJECT_CLASS (simple_list_parent_class)->dispose (object);
}

static void
asgtk_simple_list_finalize (GObject *object)
{
  	G_OBJECT_CLASS (simple_list_parent_class)->finalize (object);
}

static void
asgtk_simple_list_style_set (GtkWidget *widget,
                              GtkStyle  *prev_style)
{
  GTK_WIDGET_CLASS (simple_list_parent_class)->style_set (widget, prev_style);
}

static void
asgtk_simple_list_sel_handler(GtkTreeSelection *selection, gpointer user_data)
{
  	ASGtkSimpleList *self = ASGTK_SIMPLE_LIST(user_data); 
	GtkTreeIter iter;
	GtkTreeModel *model;
	gpointer ptr = NULL ;

	destroy_string(&self->curr_selection);
  	if (gtk_tree_selection_get_selected (selection, &model, &iter)) 
	{
		char *tmp = NULL ; 
    	gtk_tree_model_get (model, &iter, 0, &tmp, -1);
		self->curr_selection = tmp?mystrdup(tmp):NULL ;
    	gtk_tree_model_get (model, &iter, 1, &ptr, -1);
  	}
		
	if( self->sel_change_handler )
		self->sel_change_handler( self->selection_owner, ptr ); 
}


/*  public functions  */
GtkWidget *
asgtk_simple_list_new (const char *header )
{
	ASGtkSimpleList *self = g_object_new( ASGTK_TYPE_SIMPLE_LIST, NULL );
	GtkTreeSelection *selection;

	self->tree_model = GTK_TREE_MODEL(gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER));

    gtk_tree_view_set_model (GTK_TREE_VIEW(self), self->tree_model);
    self->cell = gtk_cell_renderer_text_new ();

    self->column = gtk_tree_view_column_new_with_attributes (header?header:"", self->cell, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW(self), GTK_TREE_VIEW_COLUMN (self->column));
	
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
   	g_signal_connect (selection, "changed",  G_CALLBACK (asgtk_simple_list_sel_handler), self);


	LOCAL_DEBUG_OUT( "created image ASGtkSimpleList object %p", self );	
	return GTK_WIDGET (self);
}

void  asgtk_simple_list_purge( ASGtkSimpleList *self )
{
	g_return_if_fail (ASGTK_IS_SIMPLE_LIST(self));
	gtk_list_store_clear( GTK_LIST_STORE (self->tree_model) );
	self->items_count = 0 ; 
	asgtk_simple_list_sel_handler(gtk_tree_view_get_selection(GTK_TREE_VIEW(self)), self);	
}

void  asgtk_simple_list_append( ASGtkSimpleList *self, const char *name, gpointer data )
{
	GtkTreeIter iter;
	gtk_list_store_append (GTK_LIST_STORE (self->tree_model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (self->tree_model), &iter, 
						0, name, 
						1, data, -1);
	++(self->items_count);
	if( self->items_count == 1 ) 
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(self)),&iter);
	else if( self->curr_selection != NULL && strcmp(name, self->curr_selection) == 0 ) 
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(self)),&iter);

}
