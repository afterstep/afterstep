#ifndef AS_MYICON_H_HEADER_INCLUDED
#define AS_MYICON_H_HEADER_INCLUDED

struct ASImage;
struct ASImageManager;

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

void asimage2icon( struct ASImage *im, icon_t *icon, Bool ignore_alpha );
Bool load_icon (icon_t *icon, const char *filename, struct ASImageManager *imman );
void free_icon_resources( icon_t icon );
void destroy_icon(icon_t **picon);

#endif /* #define AS_MYICON_H_HEADER_INCLUDED */
