#ifndef ASGTKIMAGEDIR_H_HEADER_INCLUDED
#define ASGTKIMAGEDIR_H_HEADER_INCLUDED


#define ASGTK_TYPE_IMAGE_DIR            (asgtk_image_dir_get_type ())
#define ASGTK_IMAGE_DIR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASGTK_TYPE_IMAGE_DIR, ASGtkImageDir))
#define ASGTK_IMAGE_DIR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ASGTK_TYPE_IMAGE_DIR, ASGtkImageDirClass))
#define ASGTK_IS_IMAGE_DIR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASGTK_TYPE_IMAGE_DIR))
#define ASGTK_IS_IMAGE_DIR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ASGTK_TYPE_IMAGE_DIR))
#define ASGTK_IMAGE_DIR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ASGTK_TYPE_IMAGE_DIR, ASGtkImageDir))


struct ASImageListEntry;
struct ASImage;

typedef struct _ASGtkImageDir
{
	GtkScrolledWindow       parent_instance;
	
	char *title ;
	char *fulldirname ;
	struct ASImageListEntry *entries;
	
	GtkTreeView     	*tree_view;
	GtkTreeModel    	*tree_model;
	GtkCellRenderer 	*cell;
    GtkTreeViewColumn 	*column;


}ASGtkImageDir;

typedef struct _ASGtkImageDirClass
{
  GtkAspectFrameClass  parent_class;
}ASGtkImageDirClass;


GType       asgtk_image_dir_get_type  (void) G_GNUC_CONST;

GtkWidget * asgtk_image_dir_new       ();

void  asgtk_image_dir_set_path( char *fulldirname );
void  asgtk_image_dir_set_title( char *title );
void  asgtk_image_dir_set_sel_handler( void (*handler)(GtkTreeSelection *selection, gpointer user_data), gpointer user_data );
struct ASImageListEntry *asgtk_image_dir_get_selection();
void  asgtk_image_dir_refresh();


#endif  /*  ASGTKIMAGEDIR_H_HEADER_INCLUDED  */
