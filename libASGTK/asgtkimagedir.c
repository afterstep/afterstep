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
 	gtk_frame_set_shadow_type (GTK_FRAME (id), GTK_SHADOW_IN);
	id->title = NULL ; 
	id->fulldirname = NULL ; 
	id->entries = NULL ;
}

static void
asgtk_image_dir_dispose (GObject *object)
{
  	/* ASGtkImageDir *id = ASGTK_IMAGE_DIR (object); */

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


/*  public functions  */
GtkWidget *
asgtk_image_dir_new ()
{
	ASGtkImageDir *id;
  	
    id = g_object_new (ASGTK_TYPE_IMAGE_DIR, NULL);
#if 0
	iv->view = gtk_image_new_from_pixbuf(NULL);
  	gtk_widget_show (GTK_WIDGET(iv->view));
	gtk_container_add (GTK_CONTAINER (iv), GTK_WIDGET(id->view));
#endif
	LOCAL_DEBUG_OUT( "created image ASGtkImageDir object %p", id );	
	return GTK_WIDGET (id);
}

void  
asgtk_image_dir_set_path( char *fulldirname )
{
	
}	 

void  
asgtk_image_dir_set_title( char *title )
{
	
	
}

void  
asgtk_image_dir_set_sel_handler( void (*handler)(GtkTreeSelection *selection, gpointer user_data), gpointer user_data )
{
	
	
}

struct ASImageListEntry *
asgtk_image_dir_get_selection()
{
	ASImageListEntry *result = NULL ; 

	return result;	
}

void  asgtk_image_dir_refresh()
{
	
	
}	 

