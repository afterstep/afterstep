#ifndef ASGTKXMLEDITOR_H_HEADER_INCLUDED
#define ASGTKXMLEDITOR_H_HEADER_INCLUDED


#define ASGTK_TYPE_XML_VIEW  (asgtk_xml_view_get_type ())
#define ASGTK_XML_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASGTK_TYPE_XML_VIEW, ASGtkXMLView))
#define ASGTK_XML_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ASGTK_TYPE_XML_VIEW, ASGtkXMLViewClass))
#define ASGTK_IS_XML_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASGTK_TYPE_XML_VIEW))
#define ASGTK_IS_XML_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ASGTK_TYPE_XML_VIEW))
#define ASGTK_XML_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ASGTK_TYPE_XML_VIEW, ASGtkXMLView))


struct ASImage;
struct ASImageListEntry;
struct _ASGtkXMLView;
struct _ASGtkXMLEditor;
struct _ASGtkColorSelection;
	
typedef void (*_ASGtkXMLEditor_handler)(struct _ASGtkXMLView *xe, gpointer user_data, Bool new_file);

typedef struct _ASGtkXMLView
{
	GtkContainer       parent_instance ;
	
	struct ASImageListEntry *entry ;

	ASGtkImageView 	*image_view ;
	GtkWidget     	*text_view ;

	GtkWidget       *refresh_btn ; 
	GtkWidget       *save_btn ; 
	GtkWidget       *save_as_btn ; 
	GtkWidget       *render_selection_btn ; 
	GtkWidget       *validate_btn ; 

	GtkTreeView     *tags_list;
	GtkWidget       *add_tag_button;
	GtkWidget       *tag_example;
	GtkWidget	    *help_view;

	GtkWidget		*color_browser_btn ;
	struct _ASGtkColorSelection *color_sel ;

	Bool 			 dirty ;

	/* screw GTK signals - hate its guts */
	_ASGtkXMLEditor_handler file_change_handler;
	gpointer file_change_user_data;
		

			
}ASGtkXMLView;

typedef struct _ASGtkXMLViewClass
{
  GtkVBoxClass  parent_class;

}ASGtkXMLViewClass;


GType       asgtk_xml_view_get_type  (void) G_GNUC_CONST;

GtkWidget * asgtk_xml_view_new       ();
void        asgtk_xml_view_set_entry ( ASGtkXMLView *xe,
                                         struct ASImageListEntry *entry);

void  		asgtk_xml_view_file_change_handler( ASGtkXMLView *xe, 
												_ASGtkXMLEditor_handler change_handler, 
												gpointer user_data );

#define ASGTK_TYPE_XML_EDITOR            (asgtk_xml_editor_get_type ())
#define ASGTK_XML_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASGTK_TYPE_XML_EDITOR, ASGtkXMLEditor))
#define ASGTK_XML_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ASGTK_TYPE_XML_EDITOR, ASGtkXMLEditorClass))
#define ASGTK_IS_XML_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASGTK_TYPE_XML_EDITOR))
#define ASGTK_IS_XML_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ASGTK_TYPE_XML_EDITOR))
#define ASGTK_XML_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ASGTK_TYPE_XML_EDITOR, ASGtkXMLEditor))

/* nice encapsulation of the above to be used as a top level window */
typedef struct _ASGtkXMLEditor
{
	GtkWindow       parent_instance ;
	
	ASGtkXMLView   *xml_view;
}ASGtkXMLEditor;

typedef struct _ASGtkXMLEditorClass
{
  GtkWindowClass  parent_class;

}ASGtkXMLEditorClass;


GType       asgtk_xml_editor_get_type  (void) G_GNUC_CONST;

GtkWidget * asgtk_xml_editor_new       ();
void        asgtk_xml_editor_set_entry ( ASGtkXMLEditor *xe,
                                         struct ASImageListEntry *entry);

void  		asgtk_xml_editor_file_change_handler( ASGtkXMLEditor *xe, 
												  _ASGtkXMLEditor_handler change_handler, 
												  gpointer user_data );

#endif  /*  ASGTKIMAGEDIR_H_HEADER_INCLUDED  */
