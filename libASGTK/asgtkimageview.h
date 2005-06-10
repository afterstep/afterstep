#ifndef ASGTKIMAGEVIEW_H_HEADER_INCLUDED
#define ASGTKIMAGEVIEW_H_HEADER_INCLUDED


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


#endif  /*  ASGTKIMAGEVIEW_H_HEADER_INCLUDED  */
