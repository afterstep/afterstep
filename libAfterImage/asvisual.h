#ifndef _ASVISUAL_H_HEADER_INCLUDED
#define _ASVISUAL_H_HEADER_INCLUDED

/***********************************************************************
 * afterstep per-screen data include file
 ***********************************************************************/
typedef CARD32 ARGB32;
#define ARGB32_White    		0xFFFFFFFF
#define ARGB32_Black    		0xFF000000
#define ARGB32_DEFAULT_BACK_COLOR	ARGB32_Black  /* default background color is #FF000000 */

#define ARGB32_ALPHA_CHAN		3
#define ARGB32_RED_CHAN			2
#define ARGB32_GREEN_CHAN		1
#define ARGB32_BLUE_CHAN		0
#define ARGB32_CHANNELS			4

typedef enum
{
  IC_BLUE	= ARGB32_BLUE_CHAN ,
  IC_GREEN	= ARGB32_GREEN_CHAN,
  IC_RED 	= ARGB32_RED_CHAN  ,
  IC_ALPHA  = ARGB32_ALPHA_CHAN,
  IC_NUM_CHANNELS = ARGB32_CHANNELS
}
ColorPart;

#define MAKE_ARGB32(a,r,g,b)	(((a)<<24)|(((r)&0x00FF)<<16)|(((g)&0x00FF)<<8)|((b)&0x00FF))
#define MAKE_ARGB32_GREY(a,l)	(((a)<<24)|(((l)&0x00FF)<<16)|(((l)&0x00FF)<<8)|((l)&0x00FF))
#define ARGB32_ALPHA8(c)		(((c)>>24)&0x00FF)
#define ARGB32_RED8(c)			(((c)>>16)&0x00FF)
#define ARGB32_GREEN8(c)	 	(((c)>>8 )&0x00FF)
#define ARGB32_BLUE8(c)			( (c)     &0x00FF)
#define ARGB32_CHAN8(c,i)		(((c)>>((i)<<3))&0x00FF)
#define MAKE_ARGB32_CHAN8(v,i)	(((v)&0x0000FF)<<((i)<<3))
#define MAKE_ARGB32_CHAN16(v,i)	((((v)&0x00FF00)>>8)<<((i)<<3))

typedef struct ColorPair
{
  ARGB32 fore;
  ARGB32 back;
}ColorPair;

typedef struct ASScanline
{
#define SCL_DO_BLUE         (0x01<<ARGB32_BLUE_CHAN )
#define SCL_DO_GREEN        (0x01<<ARGB32_GREEN_CHAN)
#define SCL_DO_RED          (0x01<<ARGB32_RED_CHAN  )
#define SCL_DO_ALPHA		(0x01<<ARGB32_ALPHA_CHAN)
#define SCL_DO_COLOR		(SCL_DO_RED|SCL_DO_GREEN|SCL_DO_BLUE)
#define SCL_DO_ALL			(SCL_DO_RED|SCL_DO_GREEN|SCL_DO_BLUE|SCL_DO_ALPHA)
	CARD32	 	   flags ;
	CARD32        *buffer ;
	CARD32        *blue, *green, *red, *alpha ;
	CARD32	      *channels[IC_NUM_CHANNELS];
	CARD32        *xc3, *xc2, *xc1;            /* since some servers require
												* BGR mode here we store what
												* goes into what color component in XImage */
	ARGB32         back_color;
	unsigned int   width, shift;
	unsigned int   offset_x ;
/*    CARD32 r_mask, g_mask, b_mask ; */
}ASScanline;

/****f* libAfterImage/asimage/prepare_scanline()
 * SYNOPSIS
 * 	ASScanline *prepare_scanline ( unsigned int width,
 * 							  	   unsigned int shift,
 * 								   ASScanline *reusable_memory,
 * 								   Bool BGR_mode);
 * DESCRIPTION
 * 	This function allocates memory ( if reusable_memory is NULL ) for the new
 * 	ASScanline structure. Structures buffers gets allocated to hold scanline
 * 	data of at least width pixel wide.
 * INPUTS
 *********/
/****f* libAfterImage/asimage/free_scanline()
 * SYNOPSIS
 * 	void       free_scanline ( ASScanline *sl, Bool reusable );
 * DESCRIPTION
 * INPUTS
 *********/
ASScanline*prepare_scanline( unsigned int width, unsigned int shift, ASScanline *reusable_memory, Bool BGR_mode);
void       free_scanline( ASScanline *sl, Bool reusable );

typedef struct ASVisual
{
	Display      *dpy;
	XVisualInfo	  visual_info;
	/* this things are calculated based on Visual : */
	unsigned long rshift, gshift, bshift;
	unsigned long rbits,  gbits,  bbits;
	unsigned long true_depth;			   /* could be 15 when X reports 16 */
	Bool          BGR_mode;
	Bool 		  msb_first;
	/* we must have colormap so that we can safely create windows with different visuals
	 * even if we are in TrueColor mode : */
	Colormap 	  colormap;
	Bool          own_colormap;                /* tells us to free colormap when we done */
	unsigned long black_pixel, white_pixel;
	/* for PseudoColor mode we need some more stuff : */
	enum {
		ACM_None = 0,
		ACM_3BPP,
		ACM_6BPP,
		ACM_12BPP,
	} as_colormap_type ;/* there can only be 64 or 4096 entries so far ( 6 or 12 bpp) */
	unsigned long *as_colormap;     /* array of preallocated colors for PseudoColor mode */
	union
	{
		ARGB32 		  		*xref;
		struct ASHashTable  *hash;
	}as_colormap_reverse ;

	/* different usefull callbacks : */
	CARD32 (*color2pixel_func) 	  ( struct ASVisual *asv, CARD32 encoded_color, unsigned long *pixel);
	void   (*pixel2color_func)    ( struct ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue);
	void   (*ximage2scanline_func)( struct ASVisual *asv, XImage *xim, ASScanline *sl, int y,  unsigned char *xim_data );
	void   (*scanline2ximage_func)( struct ASVisual *asv, XImage *xim, ASScanline *sl, int y,  unsigned char *xim_data );
#define ARGB2PIXEL(asv,argb,pixel) 			(asv)->color2pixel_func((asv),(argb),(pixel))
#define GET_SCANLINE(asv,xim,sl,y,xim_data) (asv)->ximage2scanline_func((asv),(xim),(sl),(y),(xim_data))
#define PUT_SCANLINE(asv,xim,sl,y,xim_data) (asv)->scanline2ximage_func((asv),(xim),(sl),(y),(xim_data))

}ASVisual;

Bool query_screen_visual( ASVisual *asv, Display *dpy, int screen, Window root, int default_depth );
Bool setup_truecolor_visual( ASVisual *asv );
void setup_pseudo_visual( ASVisual *asv  );
void setup_as_colormap( ASVisual *asv );
/* the following is a shortcut aggregating above function together : */
ASVisual *create_asvisual( Display *dpy, int screen, int default_depth, ASVisual *reusable_memory );
void destroy_asvisual( ASVisual *asv, Bool reusable );
Bool visual2visual_prop( ASVisual *asv, size_t *size, unsigned long *version, unsigned long **data );
Bool visual_prop2visual( ASVisual *asv, Display *dpy, int screen,
								   size_t size, unsigned long version, unsigned long *data );
/* handy utility functions for creation of windows/pixmaps/XImages : */

/* this is from xc/programs/xserver/dix/window.h */
#define INPUTONLY_LEGAL_MASK (CWWinGravity | CWEventMask | \
		   				      CWDontPropagate | CWOverrideRedirect | CWCursor )

Window  create_visual_window( ASVisual *asv, Window parent,
							  int x, int y, unsigned int width, unsigned int height,
							  unsigned int border_width, unsigned int class,
 					  		  unsigned long mask, XSetWindowAttributes *attributes );
Pixmap  create_visual_pixmap( ASVisual *asv, Window root, unsigned int width, unsigned int height, unsigned int depth );
XImage* create_visual_ximage( ASVisual *asv, unsigned int width, unsigned int height, unsigned int depth );

#endif /* _SCREEN_ */
