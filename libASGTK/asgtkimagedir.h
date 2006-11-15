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
struct _ASGtkImageDir;
	
typedef void (*_ASGtkImageDir_sel_handler)(struct _ASGtkImageDir *id, gpointer user_data);


typedef struct _ASGtkImageDir
{
	GtkScrolledWindow       parent_instance;
/* flags : */
/* mask of what columns to show : */
#define ASGTK_ImageDir_Col_Name_No	0
#define ASGTK_ImageDir_Col_Name		(0x01<<0)
#define ASGTK_ImageDir_Col_Type_No	1
#define ASGTK_ImageDir_Col_Type		(0x01<<ASGTK_ImageDir_Col_Type_No)
#define ASGTK_ImageDir_Col_Size_No	2
#define ASGTK_ImageDir_Col_Size		(0x01<<ASGTK_ImageDir_Col_Size_No)
#define ASGTK_ImageDir_Col_Date_No	3
#define ASGTK_ImageDir_Col_Date		(0x01<<ASGTK_ImageDir_Col_Date_No)
#define ASGTK_ImageDir_Col_Perms_No	4
#define ASGTK_ImageDir_Col_Perms	(0x01<<ASGTK_ImageDir_Col_Perms_No)
#define ASGTK_ImageDir_Cols			5  /* count of the above items */ 
#define ASGTK_ImageDir_Cols_All		(ASGTK_ImageDir_Col_Name|ASGTK_ImageDir_Col_Type| \
									 ASGTK_ImageDir_Col_Size|ASGTK_ImageDir_Col_Date| \
									 ASGTK_ImageDir_Col_Perms)
/* other flags : */
#define ASGTK_ImageDir_ListAll		(0x01<<16)  /* otherwise only the known types of files */ 
/* defaults : */
#define ASGTK_ImageDir_DefaultFlags (ASGTK_ImageDir_Col_Name)
	ASFlagType    flags ;
	char *fulldirname ;
	char *mini_extension ;
	struct ASImageListEntry *entries;
	struct ASImageListEntry *curr_selection;
	
	GtkTreeView     	*tree_view;
	GtkTreeModel    	*tree_model;
    GtkTreeViewColumn 	*columns[ASGTK_ImageDir_Cols];

	/* screw GTK signals - hate its guts */
	_ASGtkImageDir_sel_handler sel_change_handler;
	gpointer sel_change_user_data;
		
}ASGtkImageDir;

typedef struct _ASGtkImageDirClass
{
  GtkScrolledWindowClass  parent_class;

}ASGtkImageDirClass;


GType       asgtk_image_dir_get_type  (void) G_GNUC_CONST;

GtkWidget * asgtk_image_dir_new       ();

void  asgtk_image_dir_set_path( ASGtkImageDir *id, char *fulldirname );
void  asgtk_image_dir_set_mini( ASGtkImageDir *id, char *mini_extension );
void  asgtk_image_dir_set_title( ASGtkImageDir *id, const gchar *title );
void  asgtk_image_dir_set_sel_handler( ASGtkImageDir *id, _ASGtkImageDir_sel_handler sel_change_handler, gpointer user_data );
struct ASImageListEntry *asgtk_image_dir_get_selection( ASGtkImageDir *id );
void  asgtk_image_dir_refresh( ASGtkImageDir *id );
Bool asgtk_image_dir_make_mini_names( ASGtkImageDir *id, const char *name, char **name_return, char **fullname_return );

void  asgtk_image_dir_set_columns( ASGtkImageDir *id, ASFlagType columns );
void  asgtk_image_dir_set_list_all( ASGtkImageDir *id, Bool enable );

/* standard selection handler linking dir to ASGTKImageView window : */
void asgtk_image_dir2view_sel_handler(ASGtkImageDir *id, gpointer user_data);
FILE *open_xml_file_in_dir( ASGtkImageDir *id, const char *name, Bool mini );
Bool make_xml_from_string( ASGtkImageDir *id, const char*name, const char *str, Bool mini );
Bool make_mini_for_image_entry(ASGtkImageDir *id, struct ASImageListEntry *entry, const char *mini_fullfilename);




#endif  /*  ASGTKIMAGEDIR_H_HEADER_INCLUDED  */
