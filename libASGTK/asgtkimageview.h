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
	GtkFrame       	parent_instance;
	
	struct ASImageListEntry *image_entry;
	/* if set, that will cause image to be tiled to this size and 
	 * then scaled down to control's size */
	int 			  aspect_x, aspect_y ;
	int 			  view_width, view_height ;
	
	GtkWidget	   	*frame;
	GtkWidget      	*view;
	GtkWidget      	*scrolled_window;

	GtkWidget 		*details_hbox;
	GtkWidget 		*tools_hbox;
	GtkWidget		*details_label ;


#define ASGTK_IMAGE_VIEW_NO_SCALING		(0x01<<0)
#define ASGTK_IMAGE_VIEW_NO_TILING		(0x01<<1)
	ASFlagType     flags;

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

void 		asgtk_image_view_enable_scaling( ASGtkImageView *iv, Bool enabled );
void 		asgtk_image_view_enable_tiling( ASGtkImageView *iv, Bool enabled );
void 		asgtk_image_view_add_detail( ASGtkImageView *iv, GtkWidget *detail, int spacing );
void 		asgtk_image_view_add_tool( ASGtkImageView *iv, GtkWidget *tool, int spacing );

#endif  /*  ASGTKIMAGEVIEW_H_HEADER_INCLUDED  */
