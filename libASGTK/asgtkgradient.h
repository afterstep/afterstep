#ifndef ASGTKGRADIENT_H_HEADER_INCLUDED
#define ASGTKGRADIENT_H_HEADER_INCLUDED


#define ASGTK_TYPE_GRADIENT			          (asgtk_gradient_get_type ())
#define ASGTK_GRADIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASGTK_TYPE_GRADIENT, ASGtkGradient))
#define ASGTK_GRADIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ASGTK_TYPE_GRADIENT, ASGtkGradientClass))
#define ASGTK_IS_GRADIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASGTK_TYPE_GRADIENT))
#define ASGTK_IS_GRADIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ASGTK_TYPE_GRADIENT))
#define ASGTK_GRADIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ASGTK_TYPE_GRADIENT, ASGtkGradient))


struct ASImageListEntry;
struct ASImage;
struct _ASGtkImageView;
struct _ASGtkImageDir;
struct ASImageListEntry;
struct ASGradient;
struct ASGradientPoint;

typedef struct _ASGtkGradient
{
	GtkDialog       parent_instance;

	int type ;    
	ASBiDirList 			*points ; 
	struct ASGradientPoint  *current_point ; 

	struct _ASGtkImageView  *image_view ;
	GtkWidget 		*point_list;
	GtkListStore    *point_list_model;
	GtkWidget		*screen_width_check, *screen_height_check ;   
	GtkWidget 		*width_entry, *height_entry ;
	GtkWidget		*color_entry, *offset_entry ; 
	GtkWidget		*color_preview ;
	GtkWidget       *size_frame ; 

	GtkWidget 		*l2r_radio ;
	GtkWidget 		*tl2br_radio ;
	GtkWidget 		*t2b_radio ;
	GtkWidget 		*bl2tr_radio ;

	GtkWidget 		*apply_btn, *delete_btn ;

	int color_preview_width, color_preview_height ;

}ASGtkGradient;

typedef struct _ASGtkGradientClass
{
  	GtkDialogClass  parent_class;
	
}ASGtkGradientClass;


GType       asgtk_gradient_get_type  (void) G_GNUC_CONST;

GtkWidget *asgtk_gradient_new       ();

#endif  /*  ASGTKCOLORSEL_H_HEADER_INCLUDED  */
