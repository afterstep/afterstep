#ifndef STEPGFX_H_
#define STEPGFX_H_

enum				/* texture types */
  {
    TEXTURE_SOLID = 0,

    TEXTURE_GRADIENT = 1,
    TEXTURE_HGRADIENT = 2,
    TEXTURE_HCGRADIENT = 3,
    TEXTURE_VGRADIENT = 4,
    TEXTURE_VCGRADIENT = 5,
    TEXTURE_GRADIENT_TL2BR = 6,
    TEXTURE_GRADIENT_BL2TR = 7,
    TEXTURE_GRADIENT_T2B = 8,
    TEXTURE_GRADIENT_L2R = 9,

    TEXTURE_PIXMAP = 128,
    TEXTURE_TRANSPARENT = 129,
    TEXTURE_TRANSPIXMAP = 130,

    TEXTURE_BUILTIN = 255
  };

/***** gradients function declarations ******/


void PseudoGradient (ASCOLOR * buf, size_t size, ASCOLOR col1, ASCOLOR col2);
unsigned MakeASColors (ASCOLOR * colors, unsigned short size, int npoints, ASCOLOR * points, double *offset, int maxcolors, unsigned short *runner);

ASCOLOR *XColors2ASCOLORS (XColor * xcolors, int npoints);


ASCOLOR MakeLight (ASCOLOR base);
ASCOLOR MakeDark (ASCOLOR base);

/* 
 * relief : 
 *  <0 - sunken
 *  =0 - flat
 *  >0 - rised
 * type - bitwise or of any of the following :
 *   bit 1 - direction
 *         = 0 - right way ( horisontal or left-top to right-bottom )
 *         = 1 - left  way ( vertical   or left-bottom to right-top )
 *   bit 2 - angle 
 *         = 0 - straight (vertical or horisontal)
 *         = 1 - diagonal
 */

#define ASRELIEF_SUNKEN	       -1
#define ASRELIEF_FLAT		0
#define ASRELIEF_RAISED	        1

#define ASGRAD_LEFT		(0x1<<0)
#define ASGRAD_DIAGONAL		(0x1<<1)

int draw_gradient (Display * dpy, Drawable d, int tx, int ty, int tw, int th,
		   int npoints, XColor * color, double *offset, int relief,
		   int type, int maxcolors, int finesse);

#ifdef I18N
void DrawTexturedText (Display * dpy, Drawable d, XFontStruct * font, XFontSet fontset, int x, int y, Pixmap gradient, char *text, int chars);
#else
void DrawTexturedText (Display * dpy, Drawable d, XFontStruct * font, int x, int y, Pixmap gradient, char *text, int chars);
#endif

#endif
