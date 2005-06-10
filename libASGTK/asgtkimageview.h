/* 
 * Shamless rip-off from the GIMP - hence the original copyright : 
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

#ifndef ASTHUMBBOX_H_HEADER_INCLUDED
#define ASTHUMBBOX_H_HEADER_INCLUDED


#define ASGTK_TYPE_IMAGE_VIEW            (asgtk_image_view_get_type ())
#define ASGTK_IMAGE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASGTK_TYPE_IMAGE_VIEW, ASGtkImageView))
#define ASGTK_IMAGE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ASGTK_TYPE_IMAGE_VIEW, ASGtkImageViewClass))
#define ASGTK_IS_IMAGE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASGTK_TYPE_IMAGE_VIEW))
#define ASGTK_IS_IMAGE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ASGTK_TYPE_IMAGE_VIEW))
#define ASGTK_IMAGE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ASGTK_TYPE_IMAGE_VIEW, ASGtkImageView))


struct ASImageListEntry;
struct ASImage;

typedef struct _ASGtkImageView
{
	GtkAspectFrame       parent_instance;
	
	struct ASImageListEntry *image_entry;
	/* if set, that will cause image to be tiled to this size and 
	 * then scaled down to control's size */
	int 			  aspect_x, aspect_y ;
	int 			  view_width, view_height ;
	
	GtkWidget     *view;

}ASGtkImageView;

typedef struct _ASGtkImageViewClass
{
  GtkAspectFrameClass  parent_class;
}ASGtkImageViewClass;


GType       asgtk_image_view_get_type  (void) G_GNUC_CONST;

GtkWidget * asgtk_image_view_new       ();

void        asgtk_image_view_set_entry ( ASGtkImageView *iv,
                                         struct ASImageListEntry *image_entry);
void        asgtk_image_view_set_aspect ( ASGtkImageView *iv,
								   		  int aspect_x, int aspect_y );

GdkPixbuf *ASImage2GdkPixbuf( struct ASImage *im, Bool copy ); 


#endif  /*  ASTHUMBBOX_H_HEADER_INCLUDED  */
