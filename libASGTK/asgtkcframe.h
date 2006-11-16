#ifndef ASGTKCFRAME_H_HEADER_INCLUDED
#define ASGTKCFRAME_H_HEADER_INCLUDED


#define ASGTK_TYPE_COLLAPSING_FRAME         (asgtk_collapsing_frame_get_type ())
#define ASGTK_COLLAPSING_FRAME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASGTK_TYPE_COLLAPSING_FRAME, ASGtkLookEdit))
#define ASGTK_COLLAPSING_FRAME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ASGTK_TYPE_COLLAPSING_FRAME, ASGtkLookEditClass))
#define ASGTK_IS_COLLAPSING_FRAME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASGTK_TYPE_COLLAPSING_FRAME))
#define ASGTK_IS_COLLAPSING_FRAME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ASGTK_TYPE_COLLAPSING_FRAME))
#define ASGTK_COLLAPSING_FRAME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ASGTK_TYPE_COLLAPSING_FRAME, ASGtkLookEdit))


struct _ASGtkCollapsingFrame;
	
typedef struct _ASGtkCollapsingFrame
{
	GtkFrame       parent_instance;

	GtkWidget      *header ; 
	
}ASGtkCollapsingFrame;

typedef struct _ASGtkCollapsingFrameClass
{
  GtkFrameClass  parent_class;

}ASGtkCollapsingFrameClass;


GType       asgtk_collapsing_frame_get_type  (void) G_GNUC_CONST;

GtkWidget * asgtk_collapsing_frame_new(const char *label, const char *hide_text );




#endif  /*  ASGTKCFRAME_H_HEADER_INCLUDED  */
