#ifndef DECOR_H_HEADER
#define DECOR_H_HEADER
#ifndef NO_TEXTURE		/* NO_TEXTURE == No Frames! */

struct MyStyle ;
struct ASImage;

typedef struct ASTBarData {
#define BAR_STATE_UNFOCUSED		0
#define BAR_STATE_FOCUSED		1
#define BAR_STATE_NUM			2
	int win_x, win_y ;
	int root_x, root_y;
	int rendered_root_x, rendered_root_y;
	unsigned int width, height ;
	char *label_text ;
	
	struct MyStyle *style[2] ;
	struct ASImage *back[2]  ;
	struct ASImage *label[2] ;
	
	struct ASImage *left_shape, *center_shape, *right_shape ;

}ASTBarData ;

ASTBarData* create_astbar();
void destroy_astbar( ASTBarData **ptbar );
unsigned int get_astbar_label_width( ASTBarData *tbar );
unsigned int get_astbar_label_height( ASTBarData *tbar );

Bool set_astbar_size( ASTBarData *tbar, unsigned int width, unsigned int height );
Bool set_astbar_style( ASTBarData *tbar, unsigned int state, const char *style_name );
Bool set_astbar_label( ASTBarData *tbar, const char *label );
Bool move_astbar( ASTBarData *tbar, Window w, int win_x, int win_y );
Bool render_astbar( ASTBarData *tbar, Window w, 
                    unsigned int state, Bool pressed, 
					int clip_x, int clip_y, 
					unsigned int clip_width, unsigned int clip_height );


#endif /* !NO_TEXTURE */
#endif /* DECOR_H_HEADER */
