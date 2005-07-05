#ifndef ASGTKCOLORSEL_H_HEADER_INCLUDED
#define ASGTKCOLORSEL_H_HEADER_INCLUDED


#define ASGTK_TYPE_COLOR_SELECTION            (asgtk_color_selection_get_type ())
#define ASGTK_COLOR_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASGTK_TYPE_COLOR_SELECTION, ASGtkColorSelection))
#define ASGTK_COLOR_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ASGTK_TYPE_COLOR_SELECTION, ASGtkColorSelectionClass))
#define ASGTK_IS_COLOR_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASGTK_TYPE_COLOR_SELECTION))
#define ASGTK_IS_COLOR_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ASGTK_TYPE_COLOR_SELECTION))
#define ASGTK_COLOR_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ASGTK_TYPE_COLOR_SELECTION, ASGtkColorSelection))


struct ASImageListEntry;
struct ASImage;
struct _ASGtkImageView;
struct _ASGtkImageDir;
	
typedef struct _ASGtkColorSelection
{
	GtkDialog       parent_instance;
	
	GtkWidget 		*cs_list;
	GtkWidget 		*color_sel;

	char *curr_color_name ;

}ASGtkColorSelection;

typedef struct _ASGtkColorSelectionClass
{
  	GtkDialogClass  parent_class;
	
}ASGtkColorSelectionClass;


GType       asgtk_color_selection_get_type  (void) G_GNUC_CONST;

GtkWidget *asgtk_color_selection_new       ();
GtkWidget *asgtk_color_selection_add_main_button( ASGtkColorSelection *cs, const char *stock, GCallback func, gpointer user_data );
void asgtk_color_selection_set_color_by_name( ASGtkColorSelection *cs, const char *colorname );
char *asgtk_color_selection_get_color_str(ASGtkColorSelection *cs);

#endif  /*  ASGTKCOLORSEL_H_HEADER_INCLUDED  */
