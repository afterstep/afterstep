#ifndef AS_MYICON_H_HEADER_INCLUDED
#define AS_MYICON_H_HEADER_INCLUDED

struct ASImage;

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

#endif /* #define AS_MYICON_H_HEADER_INCLUDED */