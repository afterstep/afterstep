#ifndef ASGTKLOOKEDIT_H_HEADER_INCLUDED
#define ASGTKLOOKEDIT_H_HEADER_INCLUDED


#define ASGTK_TYPE_LOOK_EDIT         (asgtk_look_edit_get_type ())
#define ASGTK_LOOK_EDIT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASGTK_TYPE_LOOK_EDIT, ASGtkLookEdit))
#define ASGTK_LOOK_EDIT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ASGTK_TYPE_LOOK_EDIT, ASGtkLookEditClass))
#define ASGTK_IS_LOOK_EDIT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASGTK_TYPE_LOOK_EDIT))
#define ASGTK_IS_LOOK_EDIT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ASGTK_TYPE_LOOK_EDIT))
#define ASGTK_LOOK_EDIT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ASGTK_TYPE_LOOK_EDIT, ASGtkLookEdit))


struct _ASGtkLookEdit;
struct FreeStorageElem;
struct SyntaxDef;
	
typedef struct _ASGtkLookEdit
{
	GtkVBox       parent_instance;

	ASFlagType      flags ;
	char *configfilename ;
	char *myname ;
	struct SyntaxDef *syntax ;
	struct FreeStorageElem  *free_store ;
	
	
	GtkWidget 	*mystyles_frame ;
	GtkWidget 	*myframes_frame ;
	GtkWidget 	*balloons_frame ;		
	GtkWidget 	*buttons_frame ;	
	GtkWidget 	*backgrounds_frame ;
	GtkWidget 	*look_frame ;
}ASGtkLookEdit;

typedef struct _ASGtkLookEditClass
{
  GtkVBoxClass  parent_class;

}ASGtkLookEditClass;


GType       asgtk_look_edit_get_type  ( ) G_GNUC_CONST;

GtkWidget * asgtk_look_edit_new       ( const char *myname, struct SyntaxDef *syntax );

void  asgtk_look_edit_set_configfile( ASGtkLookEdit *self, char *fulldirname );
void  asgtk_look_edit_reload( ASGtkLookEdit *self );



#endif  /*  ASGTKLOOKEDIT_H_HEADER_INCLUDED  */
