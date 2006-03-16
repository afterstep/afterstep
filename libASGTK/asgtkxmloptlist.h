#ifndef ASGTKXMLOPTLIST_H_HEADER_INCLUDED
#define ASGTKXMLOPTLIST_H_HEADER_INCLUDED


#define ASGTK_TYPE_XML_OPT_LIST         (asgtk_xml_opt_list_get_type ())
#define ASGTK_XML_OPT_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASGTK_TYPE_XML_OPT_LIST, ASGtkXmlOptList))
#define ASGTK_XML_OPT_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ASGTK_TYPE_XML_OPT_LIST, ASGtkXmlOptListClass))
#define ASGTK_IS_XML_OPT_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASGTK_TYPE_XML_OPT_LIST))
#define ASGTK_IS_XML_OPT_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ASGTK_TYPE_XML_OPT_LIST))
#define ASGTK_XML_OPT_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ASGTK_TYPE_XML_OPT_LIST, ASGtkXmlOptList))


struct xml_elem_t;
struct ASXmlOptionTreeContext;
struct _ASGtkXmlOptList;
	
typedef void (*_ASGtkXmlOptList_sel_handler)(struct _ASGtkXmlOptList *xol, gpointer user_data);


typedef struct _ASGtkXmlOptList
{
	GtkScrolledWindow       parent_instance;
/* flags : */
/* mask of what columns to show : */
#define ASGTK_XmlOptList_Col_Module_No	0
#define ASGTK_XmlOptList_Col_Module		(0x01<<0)
#define ASGTK_XmlOptList_Col_Keyword_No	1
#define ASGTK_XmlOptList_Col_Keyword	(0x01<<ASGTK_XmlOptList_Col_Keyword_No)
#define ASGTK_XmlOptList_Col_Id_No		2
#define ASGTK_XmlOptList_Col_Id			(0x01<<ASGTK_XmlOptList_Col_Id_No)
#define ASGTK_XmlOptList_Col_Value_No	3
#define ASGTK_XmlOptList_Col_Value		(0x01<<ASGTK_XmlOptList_Col_Value_No)
#define ASGTK_XmlOptList_Cols			4

#define ASGTK_XmlOptList_Cols_All		(ASGTK_XmlOptList_Col_Module|ASGTK_XmlOptList_Col_Keyword| \
									 	 ASGTK_XmlOptList_Col_Id|ASGTK_XmlOptList_Col_Value)
/* other flags : */
#define ASGTK_XmlOptList_ListForeign		(0x01<<16)  /* otherwise only the known types of files */ 
/* defaults : */
#define ASGTK_XmlOptList_DefaultFlags 	(ASGTK_XmlOptList_Cols_All)

	ASFlagType    flags ;
	char *configfilename ;
	struct ASXmlOptionTreeContext *opt_list_context ;
	
	GtkTreeView     	*tree_view;
	GtkTreeModel    	*tree_model;
    GtkTreeViewColumn 	*columns[ASGTK_XmlOptList_Cols];

	/* screw GTK signals - hate its guts */
	_ASGtkXmlOptList_sel_handler sel_change_handler;
	gpointer sel_change_user_data;
		
}ASGtkXmlOptList;

typedef struct _ASGtkXmlOptListClass
{
  GtkScrolledWindowClass  parent_class;

}ASGtkXmlOptListClass;


GType       asgtk_xml_opt_list_get_type  (void) G_GNUC_CONST;

GtkWidget * asgtk_xml_opt_list_new       ();

void  asgtk_xml_opt_list_set_configfile( ASGtkXmlOptList *self, char *fulldirname );
void  asgtk_xml_opt_list_set_context( ASGtkXmlOptList *self, struct ASXmlOptionTreeContext *context );
void  asgtk_xml_opt_list_set_title( ASGtkXmlOptList *self, const gchar *title );
void  asgtk_xml_opt_list_set_sel_handler( ASGtkXmlOptList *self, _ASGtkXmlOptList_sel_handler sel_change_handler, gpointer user_data );
struct ASXmlOptionTreeContext *asgtk_xml_opt_list_get_selection( ASGtkXmlOptList *self );

void  asgtk_xml_opt_list_set_columns( ASGtkXmlOptList *self, ASFlagType columns );
void  asgtk_xml_opt_list_set_list_all( ASGtkXmlOptList *self, Bool enable );


#endif  /*  ASGTKIMAGEDIR_H_HEADER_INCLUDED  */
