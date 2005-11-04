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
	iv->image_entry = NULL ;
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
	int scaled_w, scaled_h;
	int tiled_h, tiled_w ;
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

 	scaled_w = im->width ; 
	scaled_h = im->height ;

	if( get_flags( iv->flags, ASGTK_IMAGE_VIEW_SCALE_TO_VIEW) )
	{	
		if( get_flags( iv->flags, ASGTK_IMAGE_VIEW_TILE_TO_ASPECT) && 
			iv->aspect_x > 0 && iv->aspect_y > 0 )
		{	
			scaled_w = (im->width * view_w )/ iv->aspect_x ; 
			scaled_h = (im->height * view_h )/ iv->aspect_y ;
		}else
		{	
			scaled_w = view_w ;
			scaled_h = view_h ;	
		}
	}else if( get_flags( iv->flags, ASGTK_IMAGE_VIEW_SCALE_TO_ASPECT) && 
			  iv->aspect_x > 0 && iv->aspect_y > 0 )
	{	
		scaled_w = iv->aspect_x ; 
		scaled_h = iv->aspect_y ; 
	}
		
	tiled_w = scaled_w ; 
	tiled_h = scaled_h ;

	if( get_flags( iv->flags, ASGTK_IMAGE_VIEW_TILE_TO_ASPECT) && 
		iv->aspect_x > 0 && iv->aspect_y > 0 )
	{
		if( get_flags( iv->flags, ASGTK_IMAGE_VIEW_SCALE_TO_VIEW) )
		{
			if( tiled_w < view_w ) 
				tiled_w = view_w ;
			if( tiled_h < view_h ) 
				tiled_h = view_h ;
		}else	 
		{	
			if( tiled_w < iv->aspect_x ) 
				tiled_w = iv->aspect_x ;
			if( tiled_h < iv->aspect_y ) 
				tiled_h = iv->aspect_y ;
		}		
	}	 
	if( get_flags( iv->flags, ASGTK_IMAGE_VIEW_TILE_TO_VIEW) )
	{
		if( tiled_w < view_w ) 
			tiled_w = view_w ;
		if( tiled_h < view_h ) 
			tiled_h = view_h ;
	}	 


	LOCAL_DEBUG_OUT( "scaled size is %dx%d, tiled size %dx%d", scaled_w, scaled_h, tiled_w, tiled_h );
	if( scaled_w != im->width || scaled_h != im->height ) 
	{	
		scaled = scale_asimage( asv, im, scaled_w, scaled_h, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );			
		if( scaled ) 
			im = scaled ;
	}
	
	if( tiled_w != im->width || tiled_h != im->height ) 
	{	
		tiled = tile_asimage(   asv, im, 0, 0, tiled_w, tiled_h, 
								TINT_LEAVE_SAME, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );				   
		if( tiled ) 
			im = tiled ;
	}

	pb = ASImage2GdkPixbuf( im );
	if( tiled ) 
		destroy_asimage( &tiled );
	if( scaled ) 
		destroy_asimage( &scaled );
	if( pb ) 
	{	
		gtk_image_set_from_pixbuf( GTK_IMAGE(iv->view), pb );
		gdk_pixbuf_unref( pb ); 		   
	}
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

static void 
setup_asgtk_image_view_layout_vert( ASGtkImageView *iv )	
{
	GtkWidget *main_vbox ;
	
	/***************************************/
	/* Layout code : */   
	main_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show (main_vbox);
	gtk_container_add (GTK_CONTAINER (iv), main_vbox);

	gtk_box_pack_start( GTK_BOX(main_vbox), iv->frame, TRUE, TRUE, 0 );
	
	gtk_box_pack_end (GTK_BOX (main_vbox), iv->tools_hbox, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (main_vbox), iv->details_frame, FALSE, FALSE, 5);
	   
}

static void 
setup_asgtk_image_view_layout_hor( ASGtkImageView *iv )	
{
	GtkWidget *main_hbox, *buttons_vbox, *buttons_frame ;
	
	/***************************************/
	/* Layout code : */   
	main_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show (main_hbox);
	gtk_container_add (GTK_CONTAINER (iv), main_hbox);

	gtk_box_pack_start( GTK_BOX(main_hbox), iv->frame, TRUE, TRUE, 0 );

	buttons_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show (buttons_vbox);
	gtk_box_pack_end (GTK_BOX (main_hbox), buttons_vbox, FALSE, FALSE, 0);
	
	buttons_frame = gtk_frame_new(NULL);
  	gtk_widget_show (buttons_frame);
	gtk_container_set_border_width(GTK_CONTAINER(buttons_frame), 0);
	gtk_frame_set_shadow_type (GTK_FRAME (buttons_frame), GTK_SHADOW_NONE);
	colorize_gtk_widget( buttons_frame, get_colorschemed_style_normal() );
	gtk_container_add (GTK_CONTAINER (buttons_frame), iv->tools_hbox);
	
	gtk_box_pack_start (GTK_BOX (buttons_vbox), iv->details_frame, FALSE, FALSE, 5);
	gtk_box_pack_end (GTK_BOX (buttons_vbox), buttons_frame, TRUE, TRUE, 0);
}

static void 
asgtk_image_view_make_parts( ASGtkImageView *iv, Bool horizontal )	
{
	iv->frame = gtk_aspect_frame_new(NULL, 0.5, 0.5, 1.0, TRUE);
 	gtk_frame_set_shadow_type (GTK_FRAME (iv->frame), GTK_SHADOW_NONE);
	gtk_widget_show (iv->frame);
	colorize_gtk_widget( iv->frame, get_colorschemed_style_normal() );
	
	iv->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  	gtk_widget_show (iv->scrolled_window);
	gtk_container_add (GTK_CONTAINER (iv->frame), iv->scrolled_window);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (iv->scrolled_window), GTK_SHADOW_NONE );
	
	if( get_flags( iv->flags, ASGTK_IMAGE_VIEW_SCALE_TO_VIEW|ASGTK_IMAGE_VIEW_TILE_TO_VIEW) ) 
	{	
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (iv->scrolled_window),
				    					GTK_POLICY_NEVER, GTK_POLICY_NEVER);
	}else
	{
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (iv->scrolled_window),
					    				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	}	
	colorize_gtk_widget( GTK_WIDGET(iv->scrolled_window), get_colorschemed_style_normal() );
	
	iv->view = gtk_image_new_from_pixbuf(NULL);
  	gtk_widget_show (GTK_WIDGET(iv->view));
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (iv->scrolled_window), GTK_WIDGET(iv->view));
	colorize_gtk_widget( GTK_WIDGET(iv->view), get_colorschemed_style_normal() );

	g_signal_connect ((gpointer) iv->scrolled_window, "size-allocate",
                       G_CALLBACK (asgtk_imview_view_size_alloc), iv);
	iv->details_label = gtk_label_new(NO_IMAGE_TEXT);
	gtk_widget_show (iv->details_label);
	
	iv->details_frame = gtk_frame_new(NULL);
  	gtk_widget_show (iv->details_frame);
	gtk_container_set_border_width(GTK_CONTAINER(iv->details_frame), 1);
	gtk_frame_set_shadow_type (GTK_FRAME (iv->details_frame), GTK_SHADOW_IN );
	colorize_gtk_widget( iv->details_frame, get_colorschemed_style_normal() );
	
	iv->tools_hbox = horizontal?gtk_vbox_new (FALSE,0):gtk_hbutton_box_new ();
	gtk_container_set_border_width(GTK_CONTAINER(iv->tools_hbox), 0);
	if( GTK_IS_BUTTON_BOX(iv->tools_hbox) )
		gtk_button_box_set_layout(GTK_BUTTON_BOX(iv->tools_hbox), GTK_BUTTONBOX_END/*SPREAD*/);
  	gtk_widget_show (iv->tools_hbox);
	
	iv->details_hbox = horizontal?gtk_vbutton_box_new ():gtk_hbutton_box_new ();
	gtk_container_set_border_width(GTK_CONTAINER(iv->details_hbox), 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(iv->details_hbox), GTK_BUTTONBOX_EDGE);
  	gtk_widget_show (iv->details_hbox);
	
  	gtk_container_add (GTK_CONTAINER (iv->details_frame), iv->details_hbox);
	gtk_box_pack_end (GTK_BOX (iv->details_hbox), iv->details_label, TRUE, TRUE, 0);

}
/*  public functions  */

GtkWidget *
asgtk_image_view_new ()
{
	ASGtkImageView *iv;
  	
    iv = g_object_new (ASGTK_TYPE_IMAGE_VIEW, NULL);
	
	asgtk_image_view_make_parts( iv, False );	
	setup_asgtk_image_view_layout_vert( iv );	   

	LOCAL_DEBUG_OUT( "created image ASGtk view object %p", iv );	
	return GTK_WIDGET (iv);
}

GtkWidget *
asgtk_image_view_new_horizontal ()
{
	ASGtkImageView *iv;
  	
    iv = g_object_new (ASGTK_TYPE_IMAGE_VIEW, NULL);
	
	asgtk_image_view_make_parts( iv, True );	
	setup_asgtk_image_view_layout_hor( iv );	   

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
		asgtk_image_view_refresh ( iv, (iv->image_entry->preview == NULL) );
		
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
asgtk_image_view_set_resize( ASGtkImageView *iv, unsigned long resize_flags, unsigned long set_mask )
{
	unsigned long new_flags; 
	g_return_if_fail (ASGTK_IS_IMAGE_VIEW (iv));

	new_flags = (iv->flags & (~set_mask))|(resize_flags&set_mask);
	if( new_flags == iv->flags ) 
		return;
		
	iv->flags = new_flags ;
	if( get_flags( new_flags, ASGTK_IMAGE_VIEW_SCALE_TO_VIEW|ASGTK_IMAGE_VIEW_TILE_TO_VIEW) ) 
	{	
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (iv->scrolled_window),
				    					GTK_POLICY_NEVER, GTK_POLICY_NEVER);
	}else
	{
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (iv->scrolled_window),
					    				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
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
		if( GTK_IS_VBOX(iv->tools_hbox) ) 
		{
			//gtk_widget_set_size_request( tool, iv->details_label->requisition.width, -1 );	
			gtk_box_pack_end (GTK_BOX (iv->tools_hbox), tool, FALSE, FALSE, spacing );		   		   
		}else	 
 			gtk_box_pack_start (GTK_BOX (iv->tools_hbox), tool, TRUE, TRUE, spacing );		   		
		gtk_container_set_border_width(GTK_CONTAINER(iv->tools_hbox), 1);
	}
}

void 		
asgtk_image_view_refresh( ASGtkImageView *iv, Bool reload_file )
{
	g_return_if_fail (ASGTK_IS_IMAGE_VIEW (iv));
	if( iv->image_entry ) 
	{	
		
		if( reload_file && 
			iv->image_entry->type <= ASIT_Supported )
		{
			if( iv->image_entry->preview )
			{	
				safe_asimage_destroy(iv->image_entry->preview);	  
				iv->image_entry->preview = NULL ;
			}
			/* show empty screen while loading background for now : */
			display_image_view(iv);
			
			LOCAL_DEBUG_OUT( "imman = %p, fullname = \"%s\"", get_screen_image_manager(NULL), iv->image_entry->fullfilename );
			iv->image_entry->preview = get_asimage( get_screen_image_manager(NULL),
													iv->image_entry->fullfilename, 
													0xffffffff, 100 );
			LOCAL_DEBUG_OUT( " ASGtk image view loaded image %p", iv->image_entry->preview );
		
		}	  


		if( iv->image_entry->preview ) 
		{	
			Bool ratio_changed = False ;
			char *details_text ; 

			details_text = format_asimage_list_entry_details( iv->image_entry, GTK_IS_VBUTTON_BOX(iv->details_hbox) );
			gtk_label_set_text( GTK_LABEL(iv->details_label), details_text );
			free( details_text ); 
		
			if( iv->aspect_x <= 0 || iv->aspect_y <= 0 )
				ratio_changed = set_aspect_ratio_from_image(iv);
			LOCAL_DEBUG_OUT( " ASGtk image view refreshing image %p", iv->image_entry->preview );
    		/* redisplay */
			if( !ratio_changed ) 
				display_image_view(iv);
			else
				gtk_image_set_from_stock( GTK_IMAGE(iv->view), GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_BUTTON );
		}
	}
}	 


void 
asgtk_image_view_screen_aspect_toggle( GtkWidget *checkbutton, gpointer data )
{
  	ASGtkImageView *iv = ASGTK_IMAGE_VIEW (data);
	if( GTK_TOGGLE_BUTTON (checkbutton)->active ) 
	{	
		asgtk_image_view_set_aspect ( iv, get_screen_width(NULL), get_screen_height(NULL) );
		asgtk_image_view_set_resize ( iv, 
									  ASGTK_IMAGE_VIEW_TILE_TO_ASPECT, 
									  ASGTK_IMAGE_VIEW_TILE_TO_ASPECT );
	}else
	{
		asgtk_image_view_set_aspect ( iv, -1, -1 );
		asgtk_image_view_set_resize ( iv, 0, ASGTK_IMAGE_VIEW_TILE_TO_ASPECT );
	}	 
}

void 
asgtk_image_view_scale_to_view_toggle( GtkWidget *checkbutton, gpointer data )
{
  	ASGtkImageView *iv = ASGTK_IMAGE_VIEW (data);
	if( GTK_TOGGLE_BUTTON (checkbutton)->active ) 
		asgtk_image_view_set_resize ( iv, ASGTK_IMAGE_VIEW_SCALE_TO_VIEW, 
									  	  ASGTK_IMAGE_VIEW_SCALE_TO_VIEW );
	else
		asgtk_image_view_set_resize ( iv, 0, ASGTK_IMAGE_VIEW_SCALE_TO_VIEW);
}



