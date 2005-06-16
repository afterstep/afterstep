#ifndef ASGTKIMAGEBROWSER_H_HEADER_INCLUDED
#define ASGTKIMAGEBROWSER_H_HEADER_INCLUDED


#define ASGTK_TYPE_IMAGE_BROWSER            (asgtk_image_browser_get_type ())
#define ASGTK_IMAGE_BROWSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASGTK_TYPE_IMAGE_BROWSER, ASGtkImageBrowser))
#define ASGTK_IMAGE_BROWSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ASGTK_TYPE_IMAGE_BROWSER, ASGtkImageBrowserClass))
#define ASGTK_IS_IMAGE_BROWSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASGTK_TYPE_IMAGE_BROWSER))
#define ASGTK_IS_IMAGE_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ASGTK_TYPE_IMAGE_BROWSER))
#define ASGTK_IMAGE_BROWSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ASGTK_TYPE_IMAGE_BROWSER, ASGtkImageBrowser))


struct ASImageListEntry;
struct ASImage;
struct _ASGtkImageView;
struct _ASGtkImageDir;
	
typedef struct _ASGtkImageBrowser
{
	GtkWindow       parent_instance;
	
	char *current_dir ;
	GtkWidget 				 *dir_list ;
	struct _ASGtkImageDir	 *image_dir ; 
	struct _ASGtkImageView   *preview ; 
	GtkWidget 				 *list_hbox;
	GtkWidget 				 *main_buttons_hbox;
	GtkWidget 				 *path_combo, *path_entry ; 
	GtkWidget 				 *scale_check_box ;

}ASGtkImageBrowser;

typedef struct _ASGtkImageBrowserClass
{
  	GtkWindowClass  parent_class;
	
}ASGtkImageBrowserClass;


GType       asgtk_image_browser_get_type  (void) G_GNUC_CONST;

GtkWidget *asgtk_image_browser_new       ();
GtkWidget *asgtk_image_browser_add_main_button( ASGtkImageBrowser *ib, const char *stock, GCallback func, gpointer user_data );
GtkWidget *asgtk_image_browser_add_selection_button( ASGtkImageBrowser *ib, const char *stock, GCallback func );
void asgtk_image_browser_refresh( ASGtkImageBrowser *ib );
void asgtk_image_browser_change_dir( ASGtkImageBrowser *ib, const char *dir );


#endif  /*  ASGTKIMAGEBROWSER_H_HEADER_INCLUDED  */
