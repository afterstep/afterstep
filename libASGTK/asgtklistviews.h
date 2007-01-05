#ifndef ASGTKLISTVIEWS_H_HEADER_INCLUDED
#define ASGTKLISTVIEWS_H_HEADER_INCLUDED


#define ASGTK_TYPE_SIMPLE_LIST         		(asgtk_simple_list_get_type ())
#define ASGTK_SIMPLE_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASGTK_TYPE_SIMPLE_LIST, ASGtkSimpleList))
#define ASGTK_SIMPLE_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ASGTK_TYPE_SIMPLE_LIST, ASGtkSimpleListClass))
#define ASGTK_IS_SIMPLE_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASGTK_TYPE_SIMPLE_LIST))
#define ASGTK_IS_SIMPLE_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ASGTK_TYPE_SIMPLE_LIST))
#define ASGTK_SIMPLE_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ASGTK_TYPE_SIMPLE_LIST, ASGtkSimpleList))


struct _ASGtkSimpleList;

typedef void (*_ASGtkList_sel_handler)(GObject *selection_owner, gpointer user_data);
	
typedef struct _ASGtkSimpleList
{
	GtkTreeView       parent_instance;

	GtkTreeModel    	*tree_model;
	GtkCellRenderer 	*cell;
    GtkTreeViewColumn 	*column;


	char *curr_selection ;
	
	/* screw GTK signals - hate its guts */
	_ASGtkList_sel_handler sel_change_handler;
	GObject *sel_change_owner;
	
	int items_count ; 
	
}ASGtkSimpleList;

typedef struct _ASGtkSimpleListClass
{
  GtkTreeViewClass  parent_class;

}ASGtkSimpleListClass;


GType       asgtk_simple_list_get_type  (void) G_GNUC_CONST;

GtkWidget * asgtk_simple_list_new(const char *header );
void  asgtk_simple_list_purge( ASGtkSimpleList *self );
void  asgtk_simple_list_append( ASGtkSimpleList *self, const char *name, gpointer data );
void  asgtk_simple_list_set_sel_handling( ASGtkSimpleList *self, 
										  GObject *owner, _ASGtkList_sel_handler handler );




#endif  /*  ASGTKLISTVIEWS_H_HEADER_INCLUDED  */
