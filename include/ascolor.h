#ifndef ASCOLOR_H_
#define ASCOLOR_H_

#include <X11/Xmd.h>

/******* this stuff is related to random number generation to allow for ****/
/******* dithering in gradients                                         ****/

#define IB1	1
#define IB2	2
#define IB5	16
#define IB18	131072
#define STEPGFX_RND_MASK	(IB1+IB2+IB5)

#define RND32			CARD32	/* from Xmd.h */
#define GRADIENT_SEED		345824357

extern unsigned long stepgfx_rnd_seed;
extern RND32 stepgfx_rnd32_seed;
extern unsigned long stepgfx_rnd_mask;


#define RND_BITS_IF(on_true,on_false) \
if( stepgfx_rnd_seed&IB18 ) 			  \
{ 					  \
   stepgfx_rnd_seed=((stepgfx_rnd_seed^stepgfx_rnd_mask)<<1)|IB1 ;		  \
   on_true 				  \
} else {			 	  \
   stepgfx_rnd_seed<<=1;			 	  \
   on_false				  \
}

#define RND_BITS(newbit) \
{								 \
    newbit = ((stepgfx_rnd_seed>>17)&1)^((stepgfx_rnd_seed>>4)&1)^((stepgfx_rnd_seed>>1)&1)^(stepgfx_rnd_seed&1);\
    stepgfx_rnd_seed=(stepgfx_rnd_seed<<1)|newbit;			\
    newbit&=0x01;					 \
}

#define MAX_MY_RND32		0x00ffffffff
#ifdef WORD64
#define MY_RND32() \
(stepgfx_rnd32_seed = ((1664525L*stepgfx_rnd32_seed)&MAX_MY_RND32)+1013904223L)
#else
#define MY_RND32() \
(stepgfx_rnd32_seed = (1664525L*stepgfx_rnd32_seed)+1013904223L)
#endif
/***** ASCOLOR related stuff ******/

#define ASCOLOR			CARD32	/* from Xmd.h */
/* color is represented in 4 bytes as : 0000.00rr rrrr.rr0g gggg.ggg0 bbbb.bbbb
   so all the processing will be done in 24bpp regardless of actuall 
   colordepth.
   Such a representation makes possible agregate operation on whole 
   pixel vs. one for each color component.
   We may want to switch to 8byte representation if the need arises in 
   future.
   As the result only the following macros should be used, and
   non phisical size dependend code should be allowed in other places.
 */
#define AS_BAD_COLOR		0x80000000

#define ASCOLOR_MASK		0x03fdfeff

#define ASCOLOR_RED_MASK	0x03fc0000
#define ASCOLOR_GREEN_MASK	0x0001fe00
#define ASCOLOR_BLUE_MASK	0x000000ff

#define ASCOLOR_MAXCOMP		0x00ff

#define ASCOLOR_RED_OVERFLOW(c)	   ((c)&0x04000000)
#define ASCOLOR_GREEN_OVERFLOW(c)  ((c)&0x00020000)
#define ASCOLOR_BLUE_OVERFLOW(c)   ((c)&0x00000100)

/*#define ASCOLOR_16BPP_MASK      0x03e1f8f8
 */
#define ASCOLOR_16BPP_MASK      0x03e1f8f8
#define ASCOLOR_15BPP_MASK      0x03e1f0f8

#define AVERAGE_ASCOLORS(c1,c2) (((((c1)&ASCOLOR_MASK)+((c2)&ASCOLOR_MASK))>>1)&ASCOLOR_MASK)
#define ASCOLORS_RANGE(c1,c2)   ((((c1)&ASCOLOR_MASK)-((c2)&ASCOLOR_MASK))&ASCOLOR_MASK)
#define ASCOLOR_AVG_COMP(c)	\
    (((((((c)&ASCOLOR_BLUE_MASK)+(((c)&ASCOLOR_GREEN_MASK)>>9))>>1)+ \
        (((c)&ASCOLOR_RED_MASK)>>18)) >>1)&ASCOLOR_MASK)

#define ASCOLOR_MANHATTEN_RED(c1,c2)                               \
((((c1)>=(c2))?((c1)|0x00020000)-(c2):((c2)|0x00020000)-(c1))>>18)

#define ASCOLOR_MANHATTEN_GREEN(c1,c2)                             \
(((((c1)&0x0001fe00)>=((c2)&0x0001fe00))?	          \
   (((c1)&0x0001fe00)|0x00000100)-((c2)&0x0001fe00):      \
   (((c2)&0x0001fe00)|0x00000100)-((c1)&0x0001fe00))>>9)

#define ASCOLOR_MANHATTEN_BLUE(c1,c2)                     \
  ((((c1)&0x000000ff)>=((c2)&0x000000ff))?		  \
    ((c1)&0x000000ff)-((c2)&0x000000ff):                  \
    ((c2)&0x000000ff)-((c1)&0x000000ff))

#define ASCOLOR_MANHATTEN_DIST(c1,c2)                                \
(ASCOLOR_MANHATTEN_RED(c1,c2)+ASCOLOR_MANHATTEN_GREEN(c1,c2)+        \
 ASCOLOR_MANHATTEN_BLUE(c1,c2))

#define ADJUST_GAMMA8_INV(c,ig)	\
((CARD8)(pow(((double)(c))/255,(ig))*255.0+0.5))

#define ADJUST_GAMMA8(c,gm)	ADJUST_GAMMA8_INV(c,1.0/(gm))

#define ADJUST_GAMMA16_INV(c,ig)	\
((CARD16)(pow(((double)(c))/65535,(ig))*65535.0+0.5))

#define ADJUST_GAMMA16(c,gm)	ADJUST_GAMMA16_INV(c,1.0/(gm))

#define ASCOLOR_GAMMA(c,g)	\
((((ASCOLOR)(pow((double)((c)&ASCOLOR_BLUE_MASK)/255,(1.0/(g)))*255.0+0.5))&ASCOLOR_BLUE_MASK)|          \
 (((ASCOLOR)(pow((double)((c)&ASCOLOR_GREEN_MASK)/130560.0,(1.0/(g)))*130560.0+0.5))&ASCOLOR_GREEN_MASK)|\
 (((ASCOLOR)(pow((double)((c)&ASCOLOR_RED_MASK)/66846720.0,(1.0/(g)))*66846720.0+0.5))&ASCOLOR_RED_MASK) \
)

#define ASCOLOR_RED16(c)	(((c)&ASCOLOR_RED_MASK)>>10)
#define ASCOLOR_GREEN16(c)	(((c)&ASCOLOR_GREEN_MASK)>>1)
#define ASCOLOR_BLUE16(c)	(((c)&ASCOLOR_BLUE_MASK)<<8)

#define ASCOLOR_RED8(c)		(((c)&ASCOLOR_RED_MASK)>>18)
#define ASCOLOR_GREEN8(c)	(((c)&ASCOLOR_GREEN_MASK)>>9)
#define ASCOLOR_BLUE8(c)	((c)&ASCOLOR_BLUE_MASK)

#define MAKE_ASCOLOR_RGB16(r,g,b)	(((((ASCOLOR)(r))&0xff00)<<10)| \
				         ((((ASCOLOR)(g))&0xff00)<<1 )| \
					 ((((ASCOLOR)(b))&0xff00)>>8)  )

#define MAKE_ASCOLOR_RGB16_GAMMA_I(r,g,b,ig)  \
MAKE_ASCOLOR_RGB16( ADJUST_GAMMA16_INV(r,ig), \
		    ADJUST_GAMMA16_INV(g,ig), \
		    ADJUST_GAMMA16_INV(b,ig))

#define MAKE_ASCOLOR_RGB16_GAMMA(r,g,b,gm) \
MAKE_ASCOLOR_RGB16( ADJUST_GAMMA16(r,gm),  \
		    ADJUST_GAMMA16(g,gm),  \
		    ADJUST_GAMMA16(b,gm))

#define MAKE_ASCOLOR_RGB8(r,g,b)	(((((ASCOLOR)(r))&0x00ff)<<18)| \
					 ((((ASCOLOR)(g))&0x00ff)<<9 )| \
					 (((ASCOLOR)(b))&0x00ff)       )

#define MAKE_ASCOLOR_RGB8_GAMMA_I(r,g,b,ig)   \
MAKE_ASCOLOR_RGB8(   ADJUST_GAMMA8_INV(r,ig), \
		     ADJUST_GAMMA8_INV(g,ig), \
		     ADJUST_GAMMA8_INV(b,ig))

#define MAKE_ASCOLOR_RGB8_GAMMA(r,g,b,gm) \
MAKE_ASCOLOR_RGB8(   ADJUST_GAMMA8(r,gm), \
		     ADJUST_GAMMA8(g,gm), \
		     ADJUST_GAMMA8(b,gm))

#define MAKE_ASCOLOR_GRAY8(b)	        (((((ASCOLOR)(b))&0x00ff)<<18)| \
					 ((((ASCOLOR)(b))&0x00ff)<<9 )| \
					 (((ASCOLOR)(b))&0x00ff)       )

#define MAKE_ASCOLOR_GRAY8_GAMMA_I(b,ig)   \
MAKE_ASCOLOR_GRAY8(  ADJUST_GAMMA8_INV(b,ig))

#define MAKE_ASCOLOR_GRAY8_GAMMA(b,g)   \
MAKE_ASCOLOR_GRAY8(  ADJUST_GAMMA8(b,g))


/* 15bpp - 5,5,5 */
#define ASCOLOR2PIXEL15(c)		((((c)>>11)&0x007c00)|   \
					 (((c)>>7 )&0x0003e0)|   \
					 (((c)>>3 )&0x00001f))
/* 16bpp - 5,6,5 */
#define ASCOLOR2PIXEL16(c)		((((c)>>10)&0x00f800)|   \
					 (((c)>>6 )&0x0007e0)|   \
					 (((c)>>3 )&0x00001f))
/* 32 & 24bpp - 8,8,8 */
#define ASCOLOR2PIXEL24RGB(c)		((((c)>>2)&0x00ff0000)|   \
					 (((c)>>1)&0x0000ff00)|	  \
					 ( (c)    &0x000000ff))

#define ASCOLOR2PIXEL24BGR(c)		((((c)>>18)&0x000000ff)|   \
					 (((c)>>1) &0x0000ff00)|	  \
					 (((c)<<16)&0x00ff0000))

extern unsigned long (*ascolor_to_pixel_func) (ASCOLOR);

/* universal thing - will convert ascolor to pixel for any colordepth */
#define ASCOLOR2PIXEL(c)	ascolor_to_pixel_func(c)

void set_ascolor_depth (Window root, unsigned int depth);
/* returns max allowed manhatten distance between ASCOLORS for given colordepth */
int ascolor_max_span (unsigned int depth, int *red, int *green, int *blue);
int ascolors_width (ASCOLOR * points, int npoints, int depth);

int MyAllocColor (XColor * color);
#define MyAllocColorRGB16(r,g,b) 	ASCOLOR2PIXEL(MAKE_ASCOLOR_RGB16(r,g,b))
#define MyAllocColorRGB8(r,g,b) 	ASCOLOR2PIXEL(MAKE_ASCOLOR_RGB8(r,g,b))

extern int ascolor_true_depth;
extern int BGR_mode;

#define BYTES_PER_PIXEL ((ascolor_true_depth+7)>>3)

#ifndef GETPIXEL_PUTPIXEL
#define DECLARE_XIMAGE_CURSOR(pimg,cur,y) \
  unsigned char* cur  = (pimg)->data+(y)*((pimg)->bytes_per_line)

#define SET_XIMAGE_CURSOR(pimg,cur,x,y) \
     (cur = pimg->data+(y)*((pimg)->bytes_per_line)+(x)*BYTES_PER_PIXEL)

#define INCREMENT_XIMAGE_CURSOR(cur,offset) (cur += (offset))

#define PUT_ASCOLOR(pimg,color,cur,x,y)					\
     switch(ascolor_true_depth)						\
	 {								\
	    case  8 : *(cur++) = ASCOLOR2PIXEL(color);	break ;		\
	    case 15 :{ register ASCOLOR c1 = color&ASCOLOR_15BPP_MASK ;	\
			if (BGR_mode)					\
			  c1 = (c1>>18)|((c1<<18)&~0x3ffff)|(c1&0x3fe);	\
			if( pimg->byte_order == LSBFirst ){		\
		          *(cur++) = (((c1)>>7)&0x00e0)|((c1)>>3);	\
			  *(cur++) = ((c1)>>19)|((c1>>15)&0x00000003);	\
			}else{						\
			  *(cur++) = ((c1)>>18)|((c1>>14)&0x00000007);	\
		          *(cur++) = (((c1)>>6)&0x00e0)|((c1)>>3);	\
			}						\
		     }break ;						\
	    case 16 :{ register ASCOLOR c1 = color&ASCOLOR_16BPP_MASK ;	\
			if (BGR_mode)					\
			  c1 = (c1>>18)|((c1<<18)&~0x3ffff)|(c1&0x3fe);	\
			if( pimg->byte_order == LSBFirst ){		\
		          *(cur++) = (((c1)>>6)&0x00e0)|((c1)>>3);	\
			  *(cur++) = ((c1)>>18)|((c1>>14)&0x00000007);	\
			}else{						\
			  *(cur++) = ((c1)>>18)|((c1>>14)&0x00000007);	\
		          *(cur++) = (((c1)>>6)&0x00e0)|((c1)>>3);	\
			}						\
		     }break ;					        \
	    case 24 :						        \
		    if(BGR_mode != pimg->byte_order) {			\
			*(cur++) = (color)>>18 ;			\
			*(cur++) = (color)>>9;				\
			*(cur++) = (color) ;				\
		    }else{						\
			*(cur++) = color ;			        \
			*(cur++) = (color)>>9;				\
			*(cur++) = (color)>>18 ;		        \
		    }							\
		    break ;						\
	    case 32 :		 				        \
		    if( pimg->byte_order != LSBFirst ) *(cur++) = 0 ; 	\
		    if(BGR_mode != pimg->byte_order) {			\
			*(cur++) = (color)>>18 ;			\
			*(cur++) = (color)>>9;				\
			*(cur++) = (color) ;				\
		    }else{						\
			*(cur++) = (color) ;			        \
			*(cur++) = (color)>>9;				\
			*(cur++) = (color)>>18 ;		        \
		    }							\
		    if( pimg->byte_order == LSBFirst ) *(cur++) = 0 ; 	\
		    break ;						\
	  default : XPutPixel(pimg,x,y,ASCOLOR2PIXEL(color));		\
	  }

#else

#define DECLARE_XIMAGE_CURSOR(pimg,cur,y)   {}

#define SET_XIMAGE_CURSOR(pimg,cur,x,y)  {}

#define INCREMENT_XIMAGE_CURSOR(cur,offset) {}

#define PUT_ASCOLOR(pimg,color,cur,x,y)	XPutPixel(pimg,x,y,ASCOLOR2PIXEL(color))

#endif /* GETPIXEL_PUT_PIXEL */


#endif
