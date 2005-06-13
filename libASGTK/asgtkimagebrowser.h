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
	GtkWidget 				 *list_hbox;
	struct _ASGtkImageDir	 *image_dir ; 
	struct _ASGtkImageView *preview ; 
	GtkWidget *buttons_hbox;

}ASGtkImageBrowser;

typedef struct _ASGtkImageBrowserClass
{
  	GtkWindowClass  parent_class;
	
}ASGtkImageBrowserClass;


GType       asgtk_image_browser_get_type  (void) G_GNUC_CONST;

GtkWidget * asgtk_image_browser_new       ();


#endif  /*  ASGTKIMAGEBROWSER_H_HEADER_INCLUDED  */
