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
#include "asgtkimageview.h"

#define NO_IMAGE_TEXT 	"No image to display"


/*  local function prototypes  */
static void asgtk_image_view_class_init (ASGtkImageViewClass *klass);
static void asgtk_image_view_init (ASGtkImageView *iv);
static void asgtk_image_view_dispose (GObject *object);
static void asgtk_image_view_finalize (GObject *object);
static void asgtk_image_view_style_set (GtkWidget *widget, GtkStyle  *prev_style);

/*  private variables  */
static GtkFrameClass *parent_class = NULL;

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

      	iv_type = g_type_register_static (	GTK_TYPE_FRAME,
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
	iv->image_entry = NULL ;
	iv->aspect_x = -1 ;
	iv->aspect_y = -1 ;
	iv->view_width = -1 ; 
	iv->view_height = -1 ; 
	gtk_frame_set_shadow_type (GTK_FRAME (iv), GTK_SHADOW_NONE);
}

static void
asgtk_image_view_dispose (GObject *object)
{
  	ASGtkImageView *iv = ASGTK_IMAGE_VIEW (object);
	unref_asimage_list_entry(iv->image_entry);
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
		gtk_image_set_from_stock( GTK_IMAGE(iv->view), GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_BUTTON );
		return;
	}
#if 1
	view_w = iv->view_width ;
	view_h = iv->view_height ;
	if( view_w <= 0 || view_h <= 0 )
		return ;
	if( !get_flags( iv->flags, ASGTK_IMAGE_VIEW_NO_SCALING ) )
	{	
		if( iv->aspect_x > 0 && iv->aspect_y > 0 && 
			!get_flags( iv->flags, ASGTK_IMAGE_VIEW_NO_TILING ) )
		{	
			scaled_w = (im->width * view_w )/ iv->aspect_x ; 
			scaled_h = (im->height * view_h )/ iv->aspect_y ;
		}else
		{
			scaled_w = view_w ;
			scaled_h = view_h ;	
		
		}	 
	}else
	{	
	 	scaled_w = im->width ; 
		scaled_h = im->height ;
	}			  
	LOCAL_DEBUG_OUT( "scaled size is %dx%d", scaled_w, scaled_h );
	if( scaled_w != im->width || scaled_h != im->height ) 
	{	
		scaled = scale_asimage( asv, im, scaled_w, scaled_h, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );			
		im = scaled ;
	}
	

	if( !get_flags( iv->flags, ASGTK_IMAGE_VIEW_NO_TILING ) )
	{	
		if( im->width != view_w || im->height != view_h )
		{
			tiled = tile_asimage(   asv, scaled, 0, 0, 
									view_w, view_h, 
									TINT_LEAVE_SAME, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );				   
			if( tiled )
			{
				im = tiled ;
				LOCAL_DEBUG_OUT( "tiled size is %dx%d", tiled->width, tiled->height );
			}
		}	 
	}
	
	pb = ASImage2GdkPixbuf( im );
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
	int w = allocation->width - 4 ;		   
	int h = allocation->height - 4 ;
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

static gfloat
get_old_ratio(ASGtkImageView *iv )
{
	gfloat old_ratio ;
	g_object_get (G_OBJECT(iv->frame),"ratio", &old_ratio, NULL );	
	return old_ratio;
}	 


static Bool
set_aspect_ratio_from_image(ASGtkImageView *iv)
{
	gfloat old_ratio, new_ratio ; 
	if (iv->image_entry && iv->image_entry->preview )
		new_ratio = (gfloat)iv->image_entry->preview->width/
					(gfloat)iv->image_entry->preview->height;
    else
		new_ratio = 1.0;
	old_ratio = get_old_ratio(iv);
	if( old_ratio == new_ratio ) 
		return False;
	
	gtk_aspect_frame_set( GTK_ASPECT_FRAME(iv->frame), 0.5, 0.5, new_ratio, FALSE );
	return True;
}

/*  public functions  */

GtkWidget *
asgtk_image_view_new ()
{
	ASGtkImageView *iv;
	GtkWidget *main_vbox, *details_frame ;
  	
    iv = g_object_new (ASGTK_TYPE_IMAGE_VIEW, NULL);
	main_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show (main_vbox);
	gtk_container_add (GTK_CONTAINER (iv), main_vbox);

	iv->frame = gtk_aspect_frame_new(NULL, 0.5, 0.5, 1.0, TRUE);
 	gtk_frame_set_shadow_type (GTK_FRAME (iv->frame), GTK_SHADOW_NONE);
	gtk_widget_show (iv->frame);
	gtk_box_pack_start( GTK_BOX(main_vbox), iv->frame, TRUE, TRUE, 0 );
	colorize_gtk_widget( iv->frame, get_colorschemed_style_normal() );
	
	iv->tools_hbox = gtk_hbutton_box_new ();
	gtk_container_set_border_width(GTK_CONTAINER(iv->tools_hbox), 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(iv->tools_hbox), GTK_BUTTONBOX_END/*SPREAD*/);
  	gtk_widget_show (iv->tools_hbox);
  	gtk_box_pack_end (GTK_BOX (main_vbox), iv->tools_hbox, FALSE, FALSE, 0);

	details_frame = gtk_frame_new(NULL);
  	gtk_widget_show (details_frame);
  	gtk_box_pack_end (GTK_BOX (main_vbox), details_frame, FALSE, FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(details_frame), 1);
	gtk_frame_set_shadow_type (GTK_FRAME (details_frame), GTK_SHADOW_IN );
	colorize_gtk_widget( details_frame, get_colorschemed_style_normal() );

	iv->details_hbox = gtk_hbutton_box_new ();
	gtk_container_set_border_width(GTK_CONTAINER(iv->details_hbox), 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(iv->details_hbox), GTK_BUTTONBOX_EDGE);
  	gtk_widget_show (iv->details_hbox);
  	gtk_container_add (GTK_CONTAINER (details_frame), iv->details_hbox);
	
	iv->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  	gtk_widget_show (iv->scrolled_window);
	gtk_container_add (GTK_CONTAINER (iv->frame), iv->scrolled_window);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (iv->scrolled_window), GTK_SHADOW_NONE );
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (iv->scrolled_window),
				    					GTK_POLICY_NEVER, GTK_POLICY_NEVER);
	colorize_gtk_widget( GTK_WIDGET(iv->scrolled_window), get_colorschemed_style_normal() );
	
	iv->view = gtk_image_new_from_pixbuf(NULL);
  	gtk_widget_show (GTK_WIDGET(iv->view));
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (iv->scrolled_window), GTK_WIDGET(iv->view));
	colorize_gtk_widget( GTK_WIDGET(iv->view), get_colorschemed_style_normal() );

	g_signal_connect ((gpointer) iv->scrolled_window, "size-allocate",
                       G_CALLBACK (asgtk_imview_view_size_alloc), iv);
	iv->details_label = gtk_label_new(NO_IMAGE_TEXT);
	gtk_widget_show (iv->details_label);
	gtk_box_pack_end (GTK_BOX (iv->details_hbox), iv->details_label, TRUE, TRUE, 0);

	LOCAL_DEBUG_OUT( "created image ASGtk view object %p", iv );	
	return GTK_WIDGET (iv);
}

void
asgtk_image_view_set_entry ( ASGtkImageView *iv,
                             ASImageListEntry *image_entry)
{
	g_return_if_fail (ASGTK_IS_IMAGE_VIEW (iv));

	LOCAL_DEBUG_OUT( " ASGtk image view object's %p entry to %p", iv, image_entry );
	unref_asimage_list_entry(iv->image_entry);
	iv->image_entry = ref_asimage_list_entry(image_entry) ;
	
	if (iv->image_entry)
    {
#if 1	
		Bool ratio_changed = False ;
		char *details_text ; 

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
		
		details_text = format_asimage_list_entry_details( iv->image_entry );
		gtk_label_set_text( GTK_LABEL(iv->details_label), details_text );
		free( details_text ); 
		
		if( iv->aspect_x <= 0 || iv->aspect_y <= 0 )
			ratio_changed = set_aspect_ratio_from_image(iv);
		LOCAL_DEBUG_OUT( " ASGtk image view loaded image %p", iv->image_entry->preview );
    	/* redisplay */
		if( !ratio_changed ) 
			display_image_view(iv);
		else
			gtk_image_set_from_stock( GTK_IMAGE(iv->view), GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_BUTTON );
#endif
	}else
	{	
		gtk_label_set_text( GTK_LABEL(iv->details_label), NO_IMAGE_TEXT );
		gtk_image_set_from_stock( GTK_IMAGE(iv->view), GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_BUTTON );
	}
		
}

ASImageListEntry *
asgtk_image_view_get_entry( ASGtkImageView *iv )
{
	if( ASGTK_IS_IMAGE_VIEW (iv) )
		return ref_asimage_list_entry(iv->image_entry);
	return NULL ;
}

void        
asgtk_image_view_set_aspect ( ASGtkImageView *iv,
					   		  int aspect_x, int aspect_y )
{
	Bool changed = False ;
	g_return_if_fail (ASGTK_IS_IMAGE_VIEW (iv));
	
	if( aspect_x <= 0 || aspect_y <= 0 )
	{
		iv->aspect_x = iv->aspect_y = -1 ; 
		changed = set_aspect_ratio_from_image( iv );
	}else	 
	{	
		gfloat old_ratio, new_ratio ; 
		iv->aspect_x = aspect_x ; 
		iv->aspect_y = aspect_y ; 
		new_ratio = (gfloat)aspect_x/(gfloat)aspect_y ; 
		old_ratio = get_old_ratio(iv);
		if( old_ratio != new_ratio ) 
		{
			changed = True ;	  
			gtk_aspect_frame_set( GTK_ASPECT_FRAME(iv->frame), 0.5, 0.5, new_ratio, FALSE );
		}
	}
	if( !changed )
		display_image_view(iv);
	else
		gtk_image_set_from_pixbuf( GTK_IMAGE(iv->view), NULL );
}

void 
asgtk_image_view_enable_scaling( ASGtkImageView *iv, Bool enabled )
{
	g_return_if_fail (ASGTK_IS_IMAGE_VIEW (iv));
		
	if( enabled ) 
	{	
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (iv->scrolled_window),
				    					GTK_POLICY_NEVER, GTK_POLICY_NEVER);
		clear_flags( iv->flags, ASGTK_IMAGE_VIEW_NO_SCALING);
	}else
	{
		if( get_flags( iv->flags, ASGTK_IMAGE_VIEW_NO_TILING) )
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (iv->scrolled_window),
						    				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		set_flags( iv->flags, ASGTK_IMAGE_VIEW_NO_SCALING);
	}
	if( iv->image_entry ) 
	{
		display_image_view(iv);
	}
}

void 
asgtk_image_view_enable_tiling( ASGtkImageView *iv, Bool enabled )
{
	g_return_if_fail (ASGTK_IS_IMAGE_VIEW (iv));
		
	if( enabled ) 
	{
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (iv->scrolled_window),
				    					GTK_POLICY_NEVER, GTK_POLICY_NEVER);
		clear_flags( iv->flags, ASGTK_IMAGE_VIEW_NO_TILING);
	}else
	{
		if( get_flags( iv->flags, ASGTK_IMAGE_VIEW_NO_SCALING) )
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (iv->scrolled_window),
						    				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		set_flags( iv->flags, ASGTK_IMAGE_VIEW_NO_TILING);
	}
	if( iv->image_entry ) 
	{
		display_image_view(iv);
	}
}

void 
asgtk_image_view_add_detail( ASGtkImageView *iv, GtkWidget *detail, int spacing )
{
	g_return_if_fail (ASGTK_IS_IMAGE_VIEW (iv));
	if( detail )
	{	
		gtk_box_pack_start (GTK_BOX (iv->details_hbox), detail, FALSE, FALSE, spacing );		
		gtk_box_reorder_child (GTK_BOX (iv->details_hbox), iv->details_label, -1);
	}

}

void 
asgtk_image_view_add_tool( ASGtkImageView *iv, GtkWidget *tool, int spacing  )
{
	g_return_if_fail (ASGTK_IS_IMAGE_VIEW (iv));
	if( tool ) 
	{	
 		gtk_box_pack_start (GTK_BOX (iv->tools_hbox), tool, TRUE, TRUE, spacing );		   		
		gtk_container_set_border_width(GTK_CONTAINER(iv->tools_hbox), 1);
	}
}


