#ifndef FRAME_H
#define FRAME_H
#ifndef NO_TEXTURE		/* NO_TEXTURE == No Frames! */

struct MyStyle ;
struct ASImage;

extern int DecorateFrames;
extern char *FrameN;
extern char *FrameS;
extern char *FrameE;
extern char *FrameW;
extern char *FrameNE;
extern char *FrameNW;
extern char *FrameSE;
extern char *FrameSW;

typedef struct ASBarData {
#define BAR_STATE_UNFOCUSED		0
#define BAR_STATE_FOCUSED		1
	struct MyStyle *style[2] ;
	struct ASImage *back[2]  ;
	struct ASImage *label[2] ;
	
	struct ASImage *left_shape, *center_shape, *right_shape ;

}ASBarData ;


extern void frame_init (int free_resources);
extern void frame_create_gcs ();
extern void frame_draw_frame (ASWindow *);
extern void frame_free_data (ASWindow *, Bool);
extern Bool frame_create_windows (ASWindow *);
extern void frame_set_positions (ASWindow *);




#endif /* !NO_TEXTURE */
#endif /* FRAME_H */
