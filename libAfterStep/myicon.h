#ifndef AS_MYICON_H_HEADER_INCLUDED
#define AS_MYICON_H_HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

struct ASImage;
struct ASImageManager;
struct ASVisual;

typedef struct icon_t
  {
    struct ASImage *image;		/* ASImage of pix, to reduce XGetImage() calls */
    Pixmap pix;			/* icon pixmap */
    Pixmap mask;		/* icon mask */
    Pixmap alpha;		/* icon 8-bit alpha channel pixmap */
    int width;			/* icon width */
    int height;			/* icon height */
	Atom im_name ;
  }
icon_t;

typedef icon_t MyIcon;

typedef enum {
	ASB_State_Up = 0,
	ASB_State_Down,
    ASB_StateCount
}ASButtonStates ;

typedef struct ASButton
{
    ASButtonStates     state;
    char              *shapes[ASB_StateCount];    /* icons to draw when button is any of the states */
}ASButton;

typedef struct button_t
{
    MyIcon unpressed;		/* icon to draw when button is not pressed */
    MyIcon pressed;		    /* icon to draw when button is pressed */
/*    Bool is_pressed;         is the button pressed? */
	unsigned int width, height ;
    int context ;
}button_t;

typedef button_t MyButton ;

void asimage2icon( struct ASImage *im, icon_t *icon );
void make_icon_pixmaps (icon_t * icon, Bool ignore_alpha);
void icon_from_pixmaps( MyIcon *icon, Pixmap pix, Pixmap mask, Pixmap alpha );


Bool load_icon (icon_t *icon, const char *filename, struct ASImageManager *imman );
void free_icon_resources( icon_t *icon );
void destroy_icon(icon_t **picon);

void destroy_asbutton( ASButton *btn, Bool reusable );
Bool load_button( button_t *button, char **filenames, struct ASImageManager *imman );
Bool scale_button( button_t *button, int width, int height, struct ASVisual *asv );
void free_button_resources( button_t *button );

#ifdef __cplusplus
}
#endif


#endif /* #define AS_MYICON_H_HEADER_INCLUDED */
