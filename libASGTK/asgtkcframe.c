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
#include "asgtkcframe.h"

/*  local function prototypes  */
static void asgtk_collapsing_frame_class_init (ASGtkCollapsingFrameClass *klass);
static void asgtk_collapsing_frame_init (ASGtkCollapsingFrame *self);
static void asgtk_collapsing_frame_dispose (GObject *object);
static void asgtk_collapsing_frame_finalize (GObject *object);
static void asgtk_collapsing_frame_style_set (GtkWidget *widget, GtkStyle  *prev_style);


/*  private variables  */
static GtkFrameClass *parent_class = NULL;

GType
asgtk_collapsing_frame_get_type (void)
{
  	static GType id_type = 0;

  	if (! id_type)
    {
    	static const GTypeInfo id_info =
      	{
        	sizeof (ASGtkCollapsingFrameClass),
        	(GBaseInitFunc)     NULL,
        	(GBaseFinalizeFunc) NULL,
			(GClassInitFunc)    asgtk_collapsing_frame_class_init,
        	NULL,           /* class_finalize */
        	NULL,           /* class_data     */
        	sizeof (ASGtkCollapsingFrame),
        	0,              /* n_preallocs    */
        	(GInstanceInitFunc) asgtk_collapsing_frame_init,
      	};

      	id_type = g_type_register_static (	GTK_TYPE_FRAME,
        	                                "ASGtkCollapsingFrame",
            	                            &id_info, 0);
    }

  	return id_type;
}

static void
asgtk_collapsing_frame_class_init (ASGtkCollapsingFrameClass *klass)
{
  	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->dispose   = asgtk_collapsing_frame_dispose;
  	object_class->finalize  = asgtk_collapsing_frame_finalize;

  	widget_class->style_set = asgtk_collapsing_frame_style_set;

}

static void
asgtk_collapsing_frame_init (ASGtkCollapsingFrame *self)
{
}

static void
asgtk_collapsing_frame_dispose (GObject *object)
{
/*  	ASGtkCollapsingFrame *self = ASGTK_COLLAPSING_FRAME (object); */
  	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
asgtk_collapsing_frame_finalize (GObject *object)
{
  	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
asgtk_collapsing_frame_style_set (GtkWidget *widget,
                              GtkStyle  *prev_style)
{
  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}


static void collapse_container_part(GtkWidget *widget, gpointer data)
{
	if( data ) 
		gtk_widget_hide(widget);
	else
		gtk_widget_show(widget);
}

static void on_collapse_toggle(GtkToggleButton *hide_button, ASGtkCollapsingFrame *self)
{
	Bool collapsed = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(hide_button) );

	gtk_container_forall( GTK_CONTAINER(self), collapse_container_part, (gpointer)collapsed );
	if( collapsed )
		gtk_widget_show (self->header);

	/* TODO : call specified callback to notify owner */

}	 


/*  public functions  */
GtkWidget *
asgtk_collapsing_frame_new (const char *label, const char *hide_text )
{
	ASGtkCollapsingFrame *self = g_object_new( ASGTK_TYPE_COLLAPSING_FRAME, NULL );

	GtkWidget *hbox = gtk_hbox_new( FALSE, 0 );
	GtkWidget *label_widget = gtk_label_new( label );
	GtkWidget *checkbox = gtk_check_button_new_with_label( hide_text?hide_text:"( collapse this frame)" );


	gtk_box_pack_start (GTK_BOX (hbox), label_widget, TRUE, TRUE, 5);
	gtk_box_pack_end (GTK_BOX (hbox), checkbox, TRUE, TRUE, 5);
	gtk_widget_show_all (hbox);
	gtk_widget_show (hbox);
	gtk_frame_set_label_widget( GTK_FRAME(self), hbox );
	g_signal_connect ((gpointer) checkbox, "clicked", G_CALLBACK (on_collapse_toggle), self);

	self->header = hbox ;
	self->collapse_button = checkbox ;
	
/*	colorize_gtk_tree_view_window( GTK_WIDGET(self) ); */
/*	gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET(gtk_label_new( "something or other" ))); */


	LOCAL_DEBUG_OUT( "created image ASGtkCollapsingFrame object %p", self );	
	return GTK_WIDGET (self);
}


void 
asgtk_collapsing_frame_set_open(ASGtkCollapsingFrame *self, Bool open ) 
{ 
	g_return_if_fail (ASGTK_IS_COLLAPSING_FRAME (self));
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(self->collapse_button), open?FALSE:TRUE );	
}
