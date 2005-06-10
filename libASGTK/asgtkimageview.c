/* 
 * Copyright (C) 2005 Sasha Vasko
 * Shamless rip-off from the GIMP - hence the original copyright : 
 * 
 * 
 * The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define LOCAL_DEBUG
#include "../configure.h"

#include "../include/afterbase.h"
#include "../libAfterImage/afterimage.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/screen.h"


#include "asgtk.h"
#include "asgtkimageview.h"

/*  local function prototypes  */


/*  private variables  */

static GtkAspectFrameClass *parent_class = NULL;

static void asgtk_image_view_class_init (ASGtkImageViewClass *klass);
static void asgtk_image_view_init (ASGtkImageView *iv);
static void asgtk_image_view_dispose (GObject *object);
static void asgtk_image_view_finalize (GObject *object);
static void asgtk_image_view_style_set (GtkWidget *widget, GtkStyle  *prev_style);


GType
asgtk_image_view_get_type (void)
{
  	static GType iv_type = 0;

  	if (! iv_type)
    {
    	static const GTypeInfo iv_info =
      	{
        	sizeof (ASGtkImageViewClass),
        	(GBaseInitFunc)     NULL,
        	(GBaseFinalizeFunc) NULL,
			(GClassInitFunc)    asgtk_image_view_class_init,
        	NULL,           /* class_finalize */
        	NULL,           /* class_data     */
        	sizeof (ASGtkImageView),
        	0,              /* n_preallocs    */
        	(GInstanceInitFunc) asgtk_image_view_init,
      	};

      	iv_type = g_type_register_static (	GTK_TYPE_ASPECT_FRAME,
        	                                "ASGtkImageView",
            	                            &iv_info, 0);
    }

  	return iv_type;
}

static void
asgtk_image_view_class_init (ASGtkImageViewClass *klass)
{
  	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->dispose   = asgtk_image_view_dispose;
  	object_class->finalize  = asgtk_image_view_finalize;

  	widget_class->style_set = asgtk_image_view_style_set;
}

static void
asgtk_image_view_init (ASGtkImageView *iv)
{
 	gtk_frame_set_shadow_type (GTK_FRAME (iv), GTK_SHADOW_IN);
	iv->image_entry = NULL ;
	iv->aspect_x = -1 ;
	iv->aspect_y = -1 ;
	iv->view_width = -1 ; 
	iv->view_height = -1 ; 
	gtk_aspect_frame_set( GTK_ASPECT_FRAME(iv), 0.5, 0.5, 1.0, FALSE );

}

static void
asgtk_image_view_dispose (GObject *object)
{
  	/* ASGtkImageView *iv = ASGTK_IMAGE_VIEW (object); */

  	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
asgtk_image_view_finalize (GObject *object)
{
  	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
asgtk_image_view_style_set (GtkWidget *widget,
                          GtkStyle  *prev_style)
{
  ASGtkImageView *iv = ASGTK_IMAGE_VIEW (widget);

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_modify_bg (iv->view, GTK_STATE_NORMAL,
                        &widget->style->base[GTK_STATE_NORMAL]);
  gtk_widget_modify_bg (iv->view, GTK_STATE_INSENSITIVE,
                        &widget->style->base[GTK_STATE_NORMAL]);
}

static void free_buffer (guchar *pixels, gpointer data)
{
	g_free (pixels);
}

GdkPixbuf *
ASImage2GdkPixbuf( ASImage *im, Bool copy ) 
{
	GdkPixbuf *pb = NULL ; 
	if( im ) 
	{
		int k = 0, i;
		int size = im->width*im->height;
		guchar *data ;
		ASImageDecoder *imdec;
		
		data = safemalloc( size*4 );
		if ((imdec = start_image_decoding(get_screen_visual(NULL), im, SCL_DO_ALL, 0, 0, im->width, im->height, NULL)) != NULL )
		{	 
			for (i = 0; i < (int)im->height; i++)
			{	
				CARD32 *r, *g, *b, *a ; 
				int x ; 
				imdec->decode_image_scanline( imdec ); 
				r = imdec->buffer.red ;
				g = imdec->buffer.green ;
				b = imdec->buffer.blue ;
				a = imdec->buffer.alpha ;
				for( x = 0 ; x < im->width ; ++x ) 
				{
					data[k] = r[x];
					data[++k] = g[x];
					data[++k] = b[x];
					data[++k] = a[x];
					++k;
				}	 
			}
			stop_image_decoding( &imdec );
		}
		

		pb = gdk_pixbuf_new_from_data( data, GDK_COLORSPACE_RGB, True, 8, im->width, im->height, im->width*4, free_buffer, NULL );
		if( pb == NULL ) 
			free( data );
	}	 
	return pb;
}	 

static void
display_image_view(ASGtkImageView *iv)	
{
	ASImage *im = iv->image_entry?iv->image_entry->preview:NULL ;
	GdkPixbuf *pb = NULL ; 
	ASImage *scaled = NULL, *tiled = NULL; 
	int scaled_w ;
	int scaled_h ;
	ASVisual *asv = get_screen_visual(NULL);
	int view_w, view_h ; 
			
	if( im == NULL ) 
	{	
		gtk_image_set_from_pixbuf( GTK_IMAGE(iv->view), NULL );
		return;
	}
#if 1
	view_w = iv->view_width ;
	view_h = iv->view_height ;
	if( view_w <= 0 || view_h <= 0 )
		return ;
	if( iv->aspect_x > 0 && iv->aspect_y > 0 )
	{	
		scaled_w = (im->width * view_w )/ iv->aspect_x ; 
		scaled_h = (im->height * view_h )/ iv->aspect_y ;
	}else
	{
		scaled_w = view_w ;
		scaled_h = view_h ;	
		
	}	 
	
	LOCAL_DEBUG_OUT( "scaled size is %dx%d", scaled_w, scaled_h );
	if( scaled_w != im->width || scaled_h != im->height ) 
	{	
		scaled = scale_asimage( asv, im, scaled_w, scaled_h, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );			
		if( scaled && ( scaled_w != view_w || scaled_h != view_h ))
		{
			tiled = tile_asimage(   asv, scaled, 0, 0, 
									view_w, view_h, 
									TINT_LEAVE_SAME, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );				   
			if( tiled ){LOCAL_DEBUG_OUT( "tiled size is %dx%d", tiled->width, tiled->height );}
		}	 
	}
	pb = ASImage2GdkPixbuf( tiled?tiled:(scaled?scaled:im), False );
	if( tiled ) 
		destroy_asimage( &tiled );
	if( scaled ) 
		destroy_asimage( &scaled );
	gtk_image_set_from_pixbuf( GTK_IMAGE(iv->view), pb );
	gdk_pixbuf_unref( pb ); 		   
	LOCAL_DEBUG_OUT( "####!!! recquisition is %dx%d",  
						GTK_WIDGET(iv->view)->requisition.width,  
						GTK_WIDGET(iv->view)->requisition.height );
#endif
}



static void 
asgtk_imview_view_size_alloc  ( GtkWidget *widget,
								GtkAllocation *allocation,
								gpointer user_data )
{
	ASGtkImageView *iv = ASGTK_IMAGE_VIEW (user_data);
	int view_w, view_h ; 
	int w = allocation->width ;		   
	int h = allocation->height ;
	view_w = GTK_WIDGET(iv->view)->requisition.width ;
	view_h = GTK_WIDGET(iv->view)->requisition.height ;
	
	LOCAL_DEBUG_OUT( "####!!! SizeAlloc for %p is %dx%d%+d%+d", widget, allocation->width, allocation->height, allocation->x, allocation->y );
    LOCAL_DEBUG_OUT( "####!!! recquisition was %dx%d",  widget->requisition.width,  widget->requisition.height );
    LOCAL_DEBUG_OUT( "####!!! view size is     %dx%d",  iv->view_width, iv->view_height );

#if 1 	                       /* if size changed - refresh */
	if( iv->view_width != w || iv->view_height != h ) 
	{
		iv->view_width = w ;
		iv->view_height = h ;
		display_image_view(iv);	 
	}	 
#endif
}								  
static void
set_aspect_ratio_from_image(ASGtkImageView *iv)
{
	if (iv->image_entry && iv->image_entry->preview )
		gtk_aspect_frame_set( GTK_ASPECT_FRAME(iv), 0.5, 0.5, 
								(gfloat)iv->image_entry->preview->width/
								(gfloat)iv->image_entry->preview->height, FALSE );
    else
		gtk_aspect_frame_set( GTK_ASPECT_FRAME(iv), 0.5, 0.5, 1.0, FALSE );
}




/*  public functions  */

GtkWidget *
asgtk_image_view_new ()
{
	ASGtkImageView *iv;
  	
    iv = g_object_new (ASGTK_TYPE_IMAGE_VIEW, NULL);

	iv->view = gtk_image_new_from_pixbuf(NULL);
  	gtk_widget_show (GTK_WIDGET(iv->view));
	gtk_container_add (GTK_CONTAINER (iv), GTK_WIDGET(iv->view));

	g_signal_connect ((gpointer) iv->view, "size-allocate",
                       G_CALLBACK (asgtk_imview_view_size_alloc), iv);
	LOCAL_DEBUG_OUT( "created image ASGtk view object %p", iv );	
	return GTK_WIDGET (iv);
}

void
asgtk_image_view_set_entry ( ASGtkImageView *iv,
                             ASImageListEntry *image_entry)
{
	g_return_if_fail (ASGTK_IS_IMAGE_VIEW (iv));

	LOCAL_DEBUG_OUT( " ASGtk image view object's %p entry to %p", iv, image_entry );
	iv->image_entry = image_entry ;
	
	if (iv->image_entry)
    {
#if 1	
		if( iv->image_entry->preview == NULL && 
			iv->image_entry->type <= ASIT_Supported )
		{
			/* show empty screen while loading background for now : */
			display_image_view(iv);
			
			LOCAL_DEBUG_OUT( "imman = %p, fullname = \"%s\"", get_screen_image_manager(NULL), iv->image_entry->fullfilename );
			iv->image_entry->preview = get_asimage( get_screen_image_manager(NULL),
													iv->image_entry->fullfilename, 
													0xffffffff, 100 );
		}	  
		if( iv->aspect_x <= 0 || iv->aspect_y <= 0 )
			set_aspect_ratio_from_image(iv);
		LOCAL_DEBUG_OUT( " ASGtk image view loaded image %p", iv->image_entry->preview );
    	/* redisplay */
		display_image_view(iv);
#endif
	}
}

void        
asgtk_image_view_set_aspect ( ASGtkImageView *iv,
					   		  int aspect_x, int aspect_y )
{
	g_return_if_fail (ASGTK_IS_IMAGE_VIEW (iv));
	
	if( aspect_x <= 0 || aspect_y <= 0 )
	{
		iv->aspect_x = iv->aspect_y = -1 ; 
		set_aspect_ratio_from_image( iv );
	}else	 
	{	
		iv->aspect_x = aspect_x ; 
		iv->aspect_y = aspect_y ; 
		gtk_aspect_frame_set( GTK_ASPECT_FRAME(iv), 0.5, 0.5, (gfloat)aspect_x/(gfloat)aspect_y, FALSE );
	}
	
	display_image_view(iv);
}

