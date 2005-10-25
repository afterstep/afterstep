#ifndef ASGTKDIRTREE_H_HEADER_INCLUDED
#define ASGTKDIRTREE_H_HEADER_INCLUDED


#define ASGTK_TYPE_DIR_TREE            (asgtk_dir_tree_get_type ())
#define ASGTK_DIR_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASGTK_TYPE_DIR_TREE, ASGtkDirTree))
#define ASGTK_DIR_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ASGTK_TYPE_DIR_TREE, ASGtkDirTreeClass))
#define ASGTK_IS_DIR_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASGTK_TYPE_DIR_TREE))
#define ASGTK_IS_DIR_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ASGTK_TYPE_DIR_TREE))
#define ASGTK_DIR_TREE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ASGTK_TYPE_DIR_TREE, ASGtkDirTree))


struct ASImageListEntry;
struct ASImage;
struct _ASGtkDirTree;
	
typedef void (*_ASGtkDirTree_sel_handler)(struct _ASGtkDirTree *dt, gpointer user_data);


typedef struct _ASGtkDirTree
{
	GtkScrolledWindow       parent_instance;
	
	char *root ;
	
	GtkTreeView     	*tree_view;
	GtkTreeModel    	*tree_model;
	GtkCellRenderer 	*cell;
    GtkTreeViewColumn 	*column;

	/* screw GTK signals - hate its guts */
	_ASGtkDirTree_sel_handler sel_change_handler;
	gpointer sel_change_user_data;
		
}ASGtkDirTree;

typedef struct _ASGtkDirTreeClass
{
  GtkScrolledWindowClass  parent_class;

}ASGtkDirTreeClass;


GType       asgtk_dir_tree_get_type  (void) G_GNUC_CONST;

GtkWidget * asgtk_dir_tree_new       ();

void  asgtk_dir_tree_set_root( ASGtkDirTree *dt, char *fulldirname );
void  asgtk_dir_tree_set_title( ASGtkDirTree *dt, const gchar *title );
void  asgtk_dir_tree_set_sel_handler( ASGtkDirTree *dt, _ASGtkDirTree_sel_handler sel_change_handler, gpointer user_data );
char *asgtk_dir_tree_get_selection( ASGtkDirTree *dt );
void  asgtk_dir_tree_refresh( ASGtkDirTree *dt );



#endif  /*  ASGTKIMAGEDIR_H_HEADER_INCLUDED  */
