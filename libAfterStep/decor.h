#ifndef DECOR_H_HEADER
#define DECOR_H_HEADER
#ifndef NO_TEXTURE		/* NO_TEXTURE == No Frames! */

struct MyStyle ;
struct ASImage;
struct icon_t;
struct button_t;
struct ASImageManager;

typedef struct ASCanvas
{
#define CANVAS_DIRTY				(0x01<<0)
#define CANVAS_OUT_OF_SYNC			(0x01<<1)
#define CANVAS_MASK_OUT_OF_SYNC		(0x01<<2)

/* these are not really a states but rather flags : */
#define CANVAS_CONTAINER            (0x01<<16) /* user drawn - should not maintain our canvas Pixmap,
                                                * also should monitor chnages in user defined shape,
                                                * and update mask pixmap accordingly */
    ASFlagType  state ;
	Window w;
	int root_x, root_y;
	unsigned int width, height ;

	Pixmap canvas;
	Pixmap mask;
    /* 32 bytes */
}ASCanvas;

typedef struct ASTBtnData{
    struct ASImage *unpressed ;
    struct ASImage *pressed ;
    struct ASImage *current ;
    unsigned short width, height ;
    short x, y ;
    unsigned long context ;
    /* 20 bytes */
}ASTBtnData;

#define TBTN_ORDER_L2R      0
#define TBTN_ORDER_T2B      1
#define TBTN_ORDER_R2L      2
#define TBTN_ORDER_B2T      3
#define TBTN_ORDER_VERTICAL (0x01<<0)
#define TBTN_ORDER_REVERSE  (0x01<<1)
#define TBTN_ORDER_MASK     (TBTN_ORDER_VERTICAL|TBTN_ORDER_REVERSE)

typedef struct ASTBtnBlock {
    ASTBtnData      *buttons;                  /* array of [count] structures */
    unsigned int  count ;
    unsigned int  width, height;
    /* 16 bytes */
}ASTBtnBlock;

typedef struct ASTBarData {
#define BAR_STATE_UNFOCUSED		0
#define BAR_STATE_FOCUSED		(0x01<<0)
#define BAR_STATE_NUM			2
#define BAR_STATE_FOCUS_MASK	(0x01<<0)
#define BAR_STATE_PRESSED		(0x01<<1)
#define BAR_STATE_PRESSED_MASK	(0x01<<1)

#define BAR_FLAGS_VERTICAL      (0x01<<16)     /* vertical label */
#define BAR_FLAGS_IMAGE_BACK    (0x01<<17)     /* back represents an icon instead of  */

#define ASTBAR_HILITE   (BOTTOM_HILITE|RIGHT_HILITE)

    ASFlagType  state ;
    unsigned long context ;
    short win_x, win_y ;
    short root_x, root_y;
    /* 16 bytes */
    short rendered_root_x, rendered_root_y;
    unsigned short width, height ;
    unsigned char left_bevel, top_bevel, right_bevel, bottom_bevel ;
    /* 28 bytes */
    /* this is what we make our background from :*/
    struct MyStyle      *style[2] ;
    struct ASImage      *back_image ;
    unsigned short       back_width, back_height;
    /* this is the actuall generated background : */
    struct ASImage      *back [2] ;
    /* 52 bytes */

    char *label_text ;
    struct ASImage      *label[2] ;
    /* 64 bytes */
    struct ASTBtnBlock  *left_buttons, *right_buttons;
    /* 72 bytes */
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
typedef struct MyFrame
{
#define MAGIC_MYFRAME           0xA351F385

    unsigned long magic ;
    char      *name;
    ASFlagType flags; /* first 8 bits represent one enabled side/corner each */
    struct icon_t    *parts[FRAME_PARTS];
    char             *part_filenames[FRAME_PARTS];
    unsigned int part_width[FRAME_PARTS];
    unsigned int part_length[FRAME_PARTS];
#define MYFRAME_HOR_MASK    ((0x01<<FR_N)|(0x01<<FR_S))
#define MYFRAME_VERT_MASK   ((0x01<<FR_W)|(0x01<<FR_E))
#define IsSideVertical(side)  ((side) == FR_W || (side)== FR_E)
#define IsFrameCorner(p)   ((p)>=FRAME_SIDES)
#define IsFramePart(f,p)   ((f)->parts[(p)] || ((f)->part_width[(p)] && (f)->part_length[(p)]))

    unsigned int spacing ;
}MyFrame;

extern int    _as_frame_corner_xref[FRAME_SIDES+1];
#define LeftCorner(side)    _as_frame_corner_xref[(side)]
#define RightCorner(side)   _as_frame_corner_xref[(side)+1]

ASCanvas* create_ascanvas(Window w);
ASCanvas*create_ascanvas_container (Window w);

void destroy_ascanvas( ASCanvas **pcanvas );
Bool handle_canvas_config( ASCanvas *canvas ); /* Returns True if moved/resized */
Pixmap get_canvas_canvas( ASCanvas *pc );
Pixmap get_canvas_mask( ASCanvas *pc );
Bool draw_canvas_image( ASCanvas *pc, struct ASImage *im, int x, int y );
void update_canvas_display( ASCanvas *pc );
void resize_canvas( ASCanvas *pc, unsigned int width, unsigned int height );
void moveresize_canvas (ASCanvas * pc, int x, int y, unsigned int width, unsigned int height);
void unmap_canvas_window( ASCanvas *pc );



ASTBtnData *create_astbtn();
void        set_tbtn_images( ASTBtnData* btn, struct button_t *from );
ASTBtnData *make_tbtn( struct button_t *from );
int         check_tbtn_point( ASTBtnBlock *bb, int x, int y );
void        destroy_astbtn(ASTBtnData **ptbtn );

ASTBtnBlock* create_astbtn_block( unsigned int btn_count );
ASTBtnBlock* build_tbtn_block( struct button_t *from_list, ASFlagType mask, unsigned int count,
                               int left_margin, int top_margin, int spacing, int order );
void         destroy_astbtn_block(ASTBtnBlock **pb );


ASTBarData* create_astbar();
void destroy_astbar( ASTBarData **ptbar );
unsigned int get_astbar_label_width( ASTBarData *tbar );
unsigned int get_astbar_label_height( ASTBarData *tbar );
unsigned int calculate_astbar_height( ASTBarData *tbar );
unsigned int calculate_astbar_width( ASTBarData *tbar );


Bool set_astbar_size( ASTBarData *tbar, unsigned int width, unsigned int height );
Bool set_astbar_style( ASTBarData *tbar, unsigned int state, const char *style_name );
Bool set_astbar_image( ASTBarData *tbar, struct ASImage *image );
Bool set_astbar_back_size( ASTBarData *tbar, unsigned short width, unsigned short height );
Bool set_astbar_label( ASTBarData *tbar, const char *label );
Bool set_astbar_btns( ASTBarData *tbar, ASTBtnBlock **btns, Bool left );
Bool move_astbar( ASTBarData *tbar, ASCanvas *pc, int win_x, int win_y );
Bool render_astbar( ASTBarData *tbar, ASCanvas *pc );

Bool set_astbar_focused( ASTBarData *tbar, ASCanvas *pc, Bool focused );
Bool set_astbar_pressed( ASTBarData *tbar, ASCanvas *pc, Bool pressed );
void update_astbar_transparency( ASTBarData *tbar, ASCanvas *pc );
int  check_astbar_point( ASTBarData *tbar, int root_x, int root_y );


MyFrame *create_myframe();
MyFrame *create_default_myframe();
MyFrame *myframe_find( const char *name );
void myframe_load ( MyFrame * frame, struct ASImageManager *imman );
Bool filename2myframe_part (MyFrame *frame, int part, char *filename);
Bool myframe_has_parts(const MyFrame *frame, ASFlagType mask);
void destroy_myframe( MyFrame **pframe );


#endif /* !NO_TEXTURE */
#endif /* DECOR_H_HEADER */
