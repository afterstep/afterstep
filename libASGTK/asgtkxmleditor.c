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
#include "asgtkxmleditor.h"

/*  local function prototypes  */
static void asgtk_xml_editor_class_init (ASGtkXMLEditorClass *klass);
static void asgtk_xml_editor_init (ASGtkXMLEditor *iv);
static void asgtk_xml_editor_dispose (GObject *object);
static void asgtk_xml_editor_finalize (GObject *object);
static void asgtk_xml_editor_style_set (GtkWidget *widget, GtkStyle  *prev_style);


/*  private variables  */
static GtkWindowClass *parent_class = NULL;

GType
asgtk_xml_editor_get_type (void)
{
  	static GType id_type = 0;

  	if (! id_type)
    {
    	static const GTypeInfo id_info =
      	{
        	sizeof (ASGtkXMLEditorClass),
        	(GBaseInitFunc)     NULL,
        	(GBaseFinalizeFunc) NULL,
			(GClassInitFunc)    asgtk_xml_editor_class_init,
        	NULL,           /* class_finalize */
        	NULL,           /* class_data     */
        	sizeof (ASGtkXMLEditor),
        	0,              /* n_preallocs    */
        	(GInstanceInitFunc) asgtk_xml_editor_init,
      	};

      	id_type = g_type_register_static (	GTK_TYPE_WINDOW,
        	                                "ASGtkXMLEditor",
            	                            &id_info, 0);
    }

  	return id_type;
}

static void
asgtk_xml_editor_class_init (ASGtkXMLEditorClass *klass)
{
  	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->dispose   = asgtk_xml_editor_dispose;
  	object_class->finalize  = asgtk_xml_editor_finalize;

  	widget_class->style_set = asgtk_xml_editor_style_set;

}

static void
asgtk_xml_editor_init (ASGtkXMLEditor *id)
{
	id->entry = NULL ;
}

static void
asgtk_xml_editor_dispose (GObject *object)
{
  	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR (object);
	if( xe->entry ) 
		unref_asimage_list_entry(xe->entry);
  	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
asgtk_xml_editor_finalize (GObject *object)
{
  	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
asgtk_xml_editor_style_set (GtkWidget *widget,
                          GtkStyle  *prev_style)
{
  /* ASGtkXMLEditor *id = ASGTK_XML_EDITOR (widget); */

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}

static void
on_refresh_clicked(GtkButton *button, gpointer user_data)
{
	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR(user_data);

}

static void
on_save_clicked(GtkButton *button, gpointer user_data)
{
	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR(user_data);

}

static void
on_save_as_clicked(GtkButton *button, gpointer user_data)
{
	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR(user_data);

}

static void        
on_text_changed  (GtkTextBuffer *textbuffer, gpointer user_data)
{
	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR(user_data);
	
	gtk_widget_show( xe->refresh_btn );
	gtk_widget_show( xe->save_btn );
	gtk_widget_show( xe->save_as_btn );
}

/*  public functions  */
GtkWidget *
asgtk_xml_editor_new ()
{
	ASGtkXMLEditor *xe;
	GtkWidget *main_vbox;
	GtkWidget *scrolled_window ; 
	GtkWidget *panes ; 
  	
    xe = g_object_new (ASGTK_TYPE_XML_EDITOR, NULL);

	gtk_container_set_border_width( GTK_CONTAINER (xe), 3 );
	main_vbox = gtk_vbox_new (FALSE, 0);
  	gtk_widget_show (main_vbox);
	gtk_container_add (GTK_CONTAINER (xe), main_vbox);
	gtk_box_set_spacing( GTK_BOX(main_vbox), 3 );

	panes = gtk_vpaned_new();
	gtk_widget_show( panes );
	gtk_box_pack_start (GTK_BOX (main_vbox), panes, TRUE, TRUE, 0);

	xe->image_view = ASGTK_IMAGE_VIEW(asgtk_image_view_new());
	gtk_widget_set_size_request (GTK_WIDGET(xe->image_view), 400, 300);
	gtk_paned_pack1 (GTK_PANED (panes), GTK_WIDGET(xe->image_view), TRUE, TRUE);
	gtk_widget_show (GTK_WIDGET(xe->image_view));
	asgtk_image_view_enable_tiling( xe->image_view, FALSE );
	   
	xe->text_view = gtk_text_view_new ();
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request (scrolled_window, 600, 300);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), GTK_WIDGET(xe->text_view));
	gtk_widget_show( scrolled_window );
	gtk_widget_show( xe->text_view );

	gtk_paned_pack2 (GTK_PANED (panes), scrolled_window, TRUE, TRUE);
    
	colorize_gtk_window( GTK_WIDGET(xe) );

	xe->refresh_btn = asgtk_add_button_to_box( NULL, GTK_STOCK_REFRESH, NULL, G_CALLBACK(on_refresh_clicked), xe );
	xe->save_btn = asgtk_add_button_to_box( NULL, GTK_STOCK_SAVE, NULL, G_CALLBACK(on_save_clicked), xe );
	xe->save_as_btn = asgtk_add_button_to_box( NULL, GTK_STOCK_SAVE_AS, NULL, G_CALLBACK(on_save_as_clicked), xe );

	gtk_widget_hide( xe->refresh_btn );
	gtk_widget_hide( xe->save_btn );
	gtk_widget_hide( xe->save_as_btn );

	asgtk_image_view_add_tool( xe->image_view, xe->refresh_btn, 0 );
	asgtk_image_view_add_tool( xe->image_view, xe->save_btn, 0 );
	asgtk_image_view_add_tool( xe->image_view, xe->save_as_btn, 0 );

   	g_signal_connect (gtk_text_view_get_buffer (GTK_TEXT_VIEW (xe->text_view)), 
					  "changed",  G_CALLBACK (on_text_changed), xe);


	LOCAL_DEBUG_OUT( "created image ASGtkXMLEditor object %p", xe );	
	return GTK_WIDGET (xe);
}

void   
asgtk_xml_editor_set_entry ( ASGtkXMLEditor *xe,
                             struct ASImageListEntry *entry)
{
	GtkTextBuffer *buffer;

	g_return_if_fail (ASGTK_IS_XML_EDITOR (xe));

	LOCAL_DEBUG_OUT( " ASGtk XML Editor object's %p entry to %p", xe, entry );
	
	unref_asimage_list_entry(xe->entry);

	if( entry && entry->type != ASIT_XMLScript )
		entry = NULL ; 
	
	xe->entry = ref_asimage_list_entry(entry) ;
	asgtk_image_view_set_entry ( xe->image_view, entry );

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (xe->text_view));
	if (xe->entry)
    {
#if 1	
		char * text = load_file( xe->entry->fullfilename );
		gtk_text_buffer_set_text (buffer, text, -1);
#endif
	}else
	{	
		gtk_text_buffer_set_text (buffer, NULL, 0);
	}

	gtk_widget_hide( xe->refresh_btn );
	gtk_widget_hide( xe->save_btn );
	gtk_widget_hide( xe->save_as_btn );

}



