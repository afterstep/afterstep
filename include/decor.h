#ifndef DECOR_H_HEADER
#define DECOR_H_HEADER
#ifndef NO_TEXTURE		/* NO_TEXTURE == No Frames! */

struct MyStyle ;
struct ASImage;

typedef struct ASCanvas
{
#define CANVAS_DIRTY				(0x01<<0)
#define CANVAS_OUT_OF_SYNC			(0x01<<1)
#define CANVAS_MASK_OUT_OF_SYNC		(0x01<<2)
	ASFlagType	state ;
	Window w;
	int root_x, root_y;
	unsigned int width, height ;

	Pixmap canvas;
	Pixmap mask;
}ASCanvas;

typedef struct ASTBarData {
#define BAR_STATE_UNFOCUSED		0
#define BAR_STATE_FOCUSED		(0x01<<0)
#define BAR_STATE_NUM			2
#define BAR_STATE_FOCUS_MASK	(0x01<<0)
#define BAR_STATE_PRESSED		(0x01<<1)
#define BAR_STATE_PRESSED_MASK	(0x01<<1)

	ASFlagType	state ;
	int win_x, win_y ;
	int root_x, root_y;
	int rendered_root_x, rendered_root_y;
	unsigned int width, height ;
	char *label_text ;

	unsigned int left_bevel, top_bevel, right_bevel, bottom_bevel ;

	struct MyStyle 		*style[2] ;
	struct ASImage 		*back [2] ;
	struct ASImage 		*label[2] ;

	struct ASImage *left_shape, *center_shape, *right_shape ;

}ASTBarData ;

/*********************************************************************
 * Window decorations Frame can be defined as such :
 *
 * MyFrame "name"
 *     [Inherit     "name"]
 * #traditional form :
 *     [FrameN   <pixmap>]
 *     [FrameE   <pixmap>]
 *     [FrameS   <pixmap>]
 *     [FrameW   <pixmap>]
 *     [FrameNE  <pixmap>]
 *     [FrameNW  <pixmap>]
 *     [FrameSE  <pixmap>]
 *     [FrameSW  <pixmap>]
 * #alternative form :
 *     [Side        North|South|East|West|Any [<pixmap>]] - if pixmap is ommited -
 *                                                          empty bevel will be drawn
 *     [NoSide      North|South|East|West|Any]
 *     [Corner      NorthEast|SouthEast|NorthWest|SouthWest|Any <pixmap>] - if pixmap is ommited -
 *                                                                          empty bevel will be drawn
 *     [NoCorner    NorthEast|SouthEast|NorthWest|SouthWest|Any]
 *     [SideSize    North|South|East|West|Any <WIDTHxLENGTH>] - pixmap will be scaled to this size
 *     [CornerSize  NorthEast|SouthEast|NorthWest|SouthWest|Any <WIDTHxHEIGHT>]
 * ~MyFrame
 */
struct icon_t;

typedef struct MyFrame
{
#define MAGIC_MYFRAME           0xA351F385

    unsigned long magic ;
    char      *name;
    ASFlagType flags; /* first 8 bits represent one enabled side/corner each */
    struct icon_t    *parts[FRAME_PARTS];
    unsigned int part_width[FRAME_PARTS];
    unsigned int part_length[FRAME_PARTS];
    unsigned int spacing ;
}MyFrame;


ASCanvas* create_ascanvas(Window w);
void destroy_ascanvas( ASCanvas **pcanvas );
Bool handle_canvas_config( ASCanvas *canvas ); /* Returns True if moved/resized */
Pixmap get_canvas_canvas( ASCanvas *pc );
Pixmap get_canvas_mask( ASCanvas *pc );
Bool draw_canvas_image( ASCanvas *pc, ASImage *im, int x, int y );
void update_canvas_display( ASCanvas *pc );
void resize_canvas( ASCanvas *pc, unsigned int width, unsigned int height );



ASTBarData* create_astbar();
void destroy_astbar( ASTBarData **ptbar );
unsigned int get_astbar_label_width( ASTBarData *tbar );
unsigned int get_astbar_label_height( ASTBarData *tbar );
unsigned int calculate_astbar_height( ASTBarData *tbar );
unsigned int calculate_astbar_width( ASTBarData *tbar );


Bool set_astbar_size( ASTBarData *tbar, unsigned int width, unsigned int height );
Bool set_astbar_style( ASTBarData *tbar, unsigned int state, const char *style_name );
Bool set_astbar_label( ASTBarData *tbar, const char *label );
Bool move_astbar( ASTBarData *tbar, ASCanvas *pc, int win_x, int win_y );
Bool render_astbar( ASTBarData *tbar, ASCanvas *pc );

Bool set_astbar_focused( ASTBarData *tbar, ASCanvas *pc, Bool focused );
Bool set_astbar_pressed( ASTBarData *tbar, ASCanvas *pc, Bool pressed );
void update_astbar_transparency( ASTBarData *tbar, ASCanvas *pc );


MyFrame *create_myframe();
MyFrame *create_default_myframe();
void destroy_myframe( MyFrame **pframe );


#endif /* !NO_TEXTURE */
#endif /* DECOR_H_HEADER */
