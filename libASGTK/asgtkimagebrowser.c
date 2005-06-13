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

/*  local function prototypes  */
static void asgtk_image_browser_class_init (ASGtkImageBrowserClass *klass);
static void asgtk_image_browser_init (ASGtkImageBrowser *ib);
static void asgtk_image_browser_dispose (GObject *object);
static void asgtk_image_browser_finalize (GObject *object);
static void asgtk_image_browser_style_set (GtkWidget *widget, GtkStyle  *prev_style);

static void asgtk_image_browser_close (ASGtkImageBrowser *ib);

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

static void
asgtk_image_browser_close (ASGtkImageBrowser *ib)
{
	/* Synthesize delete_event to close dialog. */
   	GtkWidget *widget = GTK_WIDGET (ib);
  	GdkEvent *event;

  	event = gdk_event_new (GDK_DELETE);
  
  	event->any.window = g_object_ref (widget->window);
  	event->any.send_event = TRUE;
  
  	gtk_main_do_event (event);
  	gdk_event_free (event);
}

void
asgtk_image_browser_close_clicked(GtkButton       *button,
                                  gpointer         user_data)
{
	asgtk_image_browser_close (ASGTK_IMAGE_BROWSER(user_data));
}


/*  public functions  */
GtkWidget *
asgtk_image_browser_new ()
{
	ASGtkImageBrowser *ib;
    GtkWidget *main_vbox;
  	GtkWidget *close_button;
	GtkWidget *separator, *buttons_hbox;
	GtkWidget *list_vbox, *preview_vbox ; 
  	
    ib = g_object_new (ASGTK_TYPE_IMAGE_BROWSER, NULL);
	
	main_vbox = gtk_vbox_new (FALSE, 0);
  	gtk_widget_show (main_vbox);
	gtk_container_add (GTK_CONTAINER (ib), main_vbox);

  	ib->list_hbox = gtk_hbox_new (FALSE, 0);
  	gtk_widget_show (ib->list_hbox);
  	gtk_box_pack_start (GTK_BOX (main_vbox), ib->list_hbox, TRUE, TRUE, 5);

  	buttons_hbox = gtk_hbutton_box_new ();
  	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_SPREAD);
  	gtk_widget_show (buttons_hbox);
  	gtk_box_pack_end (GTK_BOX (main_vbox), buttons_hbox, FALSE, FALSE, 5);


    /* separator really goes above the buttons box, so it is added second from the end ! */
	separator = gtk_hseparator_new();
	gtk_widget_show (separator);
	gtk_box_pack_end (GTK_BOX (main_vbox), separator, FALSE, FALSE, 5);

	colorize_gtk_widget( GTK_WIDGET(separator), get_colorschemed_style_button() );  
	   
  	close_button = gtk_button_new_from_stock ("gtk-close");
  	gtk_widget_show (close_button);
  	gtk_box_pack_start (GTK_BOX (buttons_hbox), close_button, FALSE, FALSE, 20);
  	gtk_widget_set_size_request (close_button, 150, -1);
	colorize_gtk_widget( GTK_WIDGET(close_button), get_colorschemed_style_button() );  

  	g_signal_connect ((gpointer) close_button, "clicked",
    	                G_CALLBACK (asgtk_image_browser_close_clicked), ib);
	
	/* now designing dirlist controls : */
	list_vbox = gtk_vbox_new (FALSE, 0);
  	gtk_widget_show (list_vbox);
	gtk_container_add (GTK_CONTAINER (ib->list_hbox), list_vbox);
	
	ib->image_dir = ASGTK_IMAGE_DIR(asgtk_image_dir_new());
	gtk_box_pack_start (GTK_BOX (list_vbox), GTK_WIDGET(ib->image_dir), TRUE, TRUE, 0);
	gtk_widget_set_size_request (GTK_WIDGET(ib->image_dir), 200, 300);
	gtk_widget_show (GTK_WIDGET(ib->image_dir));
	colorize_gtk_widget( GTK_WIDGET(ib->image_dir), get_colorschemed_style_button());
	gtk_widget_set_style( GTK_WIDGET(ib->image_dir), get_colorschemed_style_normal());

	ib->current_dir = PutHome("~/");
	asgtk_image_dir_set_path(ASGTK_IMAGE_DIR(ib->image_dir), ib->current_dir);

	/* now designing preview controls : */
  	preview_vbox = gtk_vbox_new (FALSE, 0);
  	gtk_widget_show (preview_vbox);
  	gtk_box_pack_end (GTK_BOX (ib->list_hbox), preview_vbox, TRUE, TRUE, 5);

	ib->preview = ASGTK_IMAGE_VIEW(asgtk_image_view_new());
	gtk_widget_set_size_request (GTK_WIDGET(ib->preview), 300, 400);
	gtk_box_pack_start (GTK_BOX (preview_vbox), GTK_WIDGET(ib->preview), TRUE, TRUE, 0);
	gtk_widget_show (GTK_WIDGET(ib->preview));

	ib->buttons_hbox = gtk_hbutton_box_new ();
  	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_SPREAD);
  	gtk_widget_show (ib->buttons_hbox);
  	gtk_box_pack_end (GTK_BOX (preview_vbox), ib->buttons_hbox, FALSE, FALSE, 5);

	LOCAL_DEBUG_OUT( "created image ASGtkImageBrowser object %p", ib );	
	return GTK_WIDGET (ib);
}


