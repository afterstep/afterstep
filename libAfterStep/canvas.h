#ifndef CANVAS_H_HEADER
#define CANVAS_H_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(DEBUG_ALLOCS) && defined(DEBUG_TRACE)
#define TRACE_update_canvas_display
#define TRACE_render_astbar
#endif

/* define that if you want all geometry being updated just before setting shape*/
#undef	STRICT_GEOMETRY 


struct MyStyle ;
struct ASImage;
struct icon_t;
struct button_t;
struct ASImageManager;
struct ASBalloon;
struct ASVector;
struct ASGrid;

typedef struct ASCanvas
{
#define CANVAS_DIRTY				(0x01<<0)
#define CANVAS_OUT_OF_SYNC			(0x01<<1)
#define CANVAS_MASK_OUT_OF_SYNC		(0x01<<2)
#define CANVAS_CONFIG_INVALID		(0x01<<3)
#define CANVAS_MAPPED				(0x01<<4)

/* these are not really a states but rather flags : */
#define CANVAS_CONTAINER            (0x01<<16) /* user drawn - should not maintain our canvas Pixmap,
                                                * also should monitor chnages in user defined shape,
                                                * and update mask pixmap accordingly */
#define CANVAS_FORCE_MASK           (0x01<<18)  /* forces rendering of the canvas mask even if MyStyle is not shaped */
#define CANVAS_SHAPE_SET            (0x01<<19)  /* */

#define IsCanvasShaped(pc)            get_flags((pc)->state,CANVAS_SHAPE_SET)

    ASFlagType  state ;
	Window w;
	int root_x, root_y;
	unsigned short width, height ;
	unsigned int bw ;

	Pixmap 		saved_canvas ;
	struct ASVector 	*saved_shape ;

	/* these two are shortcuts to above values */
	Pixmap canvas;
	struct ASVector *shape;                     /* vector of XRectangles */
    /* 32 bytes */
}ASCanvas;

/* synonim for backporting of parts of as-devel */
#define ASWidget ASCanvas
#define AS_WIDGET_SCREEN(c) (ASDefaultScr)
#define AS_WIDGET_WINDOW(c) ((c)?(c)->w:None)
#define AS_WIDGET_X(c)      ((c)?(int)(c)->root_x:0)
#define AS_WIDGET_Y(c)      ((c)?(int)(c)->root_y:0)
#define AS_WIDGET_WIDTH(c)  ((c)?(int)(c)->width:0)
#define AS_WIDGET_HEIGHT(c) ((c)?(int)(c)->height:0)
#define AS_WIDGET_BW(c) 	((c)?(int)(c)->bw:0)
#define AS_WIDGET_FEEL(c)   (&(ASDefaultScr->Feel))
#define AS_WIDGET_LOOK(c)   (&(ASDefaultScr->Look))



ASCanvas* create_ascanvas(Window w);
ASCanvas* create_ascanvas_container (Window w);

void destroy_ascanvas( ASCanvas **pcanvas );
#define CANVAS_X_CHANGED            (0x01<<0)
#define CANVAS_Y_CHANGED            (0x01<<1)
#define CANVAS_MOVED                (CANVAS_X_CHANGED|CANVAS_Y_CHANGED)
#define CANVAS_WIDTH_CHANGED        (0x01<<2)
#define CANVAS_HEIGHT_CHANGED       (0x01<<3)
#define CANVAS_RESIZED              (CANVAS_WIDTH_CHANGED|CANVAS_HEIGHT_CHANGED)

ASFlagType handle_canvas_config( ASCanvas *canvas ); /* Returns True if moved/resized */
Bool get_canvas_position( ASCanvas *pc, Window *pparent, int *px, int *py, unsigned int *pbw );
void clear_canvas_shape (ASCanvas * pc, Bool force_for_container);
void invalidate_canvas_config( ASCanvas *canvas );
Bool get_current_canvas_size( ASCanvas * pc, unsigned int *pwidth, unsigned int *pheight );
Bool get_current_canvas_geometry( ASCanvas * pc, int *px, int *py, unsigned int *pwidth, unsigned int *pheight, unsigned int *pbw );


Pixmap get_canvas_canvas( ASCanvas *pc );
Pixmap get_canvas_mask( ASCanvas *pc );
Bool draw_canvas_image( ASCanvas *pc, struct ASImage *im, int x, int y );
void fill_canvas_mask (ASCanvas * pc, int win_x, int win_y, int width, int height);
Bool draw_canvas_mask (ASCanvas * pc, ASImage * im, int x, int y);

#ifdef TRACE_update_canvas_display
void  trace_update_canvas_display (ASCanvas * pc, const char *file, int line);
#define update_canvas_display(p)  trace_update_canvas_display((p),__FILE__,__LINE__)
#else
void update_canvas_display( ASCanvas *pc );
#endif
void update_canvas_display_mask (ASCanvas * pc, Bool force);

Bool save_canvas( ASCanvas *pc );
Bool swap_save_canvas( ASCanvas *pc );

void invalidate_canvas_save( ASCanvas *pc );
Bool restore_canvas( ASCanvas *pc );

Bool check_canvas_shaped( ASCanvas *pc);
Bool refresh_container_shape( ASCanvas *pc );
Bool combine_canvas_shape (ASCanvas *parent, ASCanvas *child );
Bool combine_canvas_shape_at_geom (ASCanvas *parent, ASCanvas *child, int child_x, int child_y, int child_w, int child_h, int child_bw );
Bool combine_canvas_shape_at (ASCanvas *parent, ASCanvas *child, int child_x, int child_y );



ASFlagType resize_canvas( ASCanvas *pc, unsigned int width, unsigned int height );
void move_canvas (ASCanvas * pc, int x, int y);
ASFlagType moveresize_canvas (ASCanvas * pc, int x, int y, unsigned int width, unsigned int height);
void map_canvas_window( ASCanvas *pc, Bool raised );
void unmap_canvas_window( ASCanvas *pc );
void reparent_canvas_window( ASCanvas *pc, Window dst, int x, int y );
void quietly_reparent_canvas( ASCanvas *pc, Window dst, long event_mask, Bool use_root_pos, Window below );


Bool is_canvas_needs_redraw( ASCanvas *pc );
Bool is_canvas_dirty( ASCanvas *pc );
void add_canvas_grid( struct ASGrid *grid, ASCanvas *canvas, int outer_gravity, int inner_gravity, int vx, int vy );
void set_root_clip_area( ASCanvas *canvas );

void send_canvas_configure_notify(ASCanvas *parent, ASCanvas *canvas);


#ifdef __cplusplus
}
#endif

#endif

