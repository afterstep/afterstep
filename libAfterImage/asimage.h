#ifndef ASIMAGE_HEADER_FILE_INCLUDED
#define ASIMAGE_HEADER_FILE_INCLUDED

#include "asvisual.h"
#include "blender.h"

#define MAX_IMPORT_IMAGE_SIZE 	4000

/* RLE format :
component := <line><line>...<line>
line      := <block><block>...<block><EOL>
block     := <EOL>|<simple_block>|<long_block>|<direct_block>

EOL       := 00000000 (all zero bits)

simple_block  := <ctrl_byte1><value_byte>
ctrl_byte1    := 00NNNNNN (first two bits are 0 remaining are length)

long_block    := <ctrl_byte2><more_length_byte><value_byte>
ctrl_byte2    := 01NNNNNN (NNNNNN are high 6 bits of length)
more_length_byte := low 8 bits of length

direct_block  := <ctrl_byte3><value_byte><value_byte>...<value_byte>
ctrl_byte3    := [1NNNNNNN|11111111] (first bit is 1, remaining are length.
 * 				If it is all 1's - then remaining part of the line up untill
 * 			    image width is monolitic uncompressed direct block)

*/

/* this is value */
#define RLE_EOL					0x00
/* this are masks */
#define RLE_DIRECT_B 			0x80
#define RLE_DIRECT_TAIL			0xFF
#define RLE_LONG_B 				0x40
/* this one is inverted mask */
#define RLE_SIMPLE_B_INV  		0xC0

/* this are masks to filter out control bits: */
#define RLE_DIRECT_D 			0x7F
#define RLE_LONG_D 				0x3F
#define RLE_SIMPLE_D  			0x3F

#define RLE_MAX_DIRECT_LEN      (RLE_DIRECT_D-1)
#define RLE_MAX_SIMPLE_LEN     	63
#define RLE_MAX_LONG_LEN     	(64*256)
#define RLE_THRESHOLD			1

typedef struct ASImage
{
  unsigned int width, height;
  CARD8 **alpha, **red, **green, **blue;
  CARD8 **channels[IC_NUM_CHANNELS];/* merely a shortcut for faster translation to the
									 * above pointers*/

  CARD8 *buffer;
  unsigned int buf_used, buf_len;

  unsigned int max_compressed_width;           /* effectively limits compression to speed things up */

  XImage *ximage ;
}
ASImage;

typedef struct ASImageBevel
{
#define MAX_BEVEL_OUTLINE 10
	ASFlagType type ;
	ARGB32		hi_color, lo_color ;
	unsigned short left_outline, top_outline, right_outline, bottom_outline;
	unsigned short left_inline, top_inline, right_inline, bottom_inline;
}ASImageBevel;

/* Auxilary data structures : */
typedef struct ASImageDecoder
{
	ASVisual 	   *asv;
	ASImage 	   *im ;
	ASFlagType 		filter;

#define ARGB32_DEFAULT_BACK_COLOR	ARGB32_Black

	ARGB32	 		back_color;  /* we fill missing scanlines with this - default - black*/
	unsigned int    offset_x,    /* left margin on source image before which we skip everything */
					out_width;   /* actuall length of the output scanline */
	unsigned int 	offset_y;	 /* top margin */
								 /* there is no need for out_height - if we go out of the
								  * image size - we simply reread lines from the beginning
                                  */
	ASImageBevel	*bevel;      /* bevel to wrap everything around with */
	unsigned short   bevel_h_addon, bevel_v_addon ;

	ASScanline 		buffer;
	int 			next_line ;
}ASImageDecoder;

/* This is static piece of data that tell us what is the status of
 * the output stream, and allows us to easily put stuff out :       */
#define ASIMAGE_QUALITY_DEFAULT	-1
#define ASIMAGE_QUALITY_POOR	0
#define ASIMAGE_QUALITY_FAST	1
#define ASIMAGE_QUALITY_GOOD	2
#define ASIMAGE_QUALITY_TOP		3

#define MAX_GRADIENT_DITHER_LINES 	ASIMAGE_QUALITY_TOP+1


struct ASImageOutput;
struct ASScanline;
typedef void (*encode_image_scanline_func)( struct ASImageOutput *imout, ASScanline *to_store );
typedef void (*output_image_scanline_func)( struct ASImageOutput *, ASScanline *, int );

typedef struct ASImageOutput
{
	ASVisual 	   *asv;
	ASImage *im ;
	XImage *xim ;
	Bool to_xim ;
	unsigned char *xim_line;
	int            height, bpl;
	ASScanline buffer[2], *used, *available;
	CARD32 chan_fill[4];
	int buffer_shift;   /* -1 means - buffer is empty */
	int next_line ;
	unsigned int tiling_step;       /* each line written will be repeated with this
									 * step untill we exceed image size */
	Bool bottom_to_top;

	int quality ;/* see above */
	output_image_scanline_func output_image_scanline ;  /* high level interface -
														 * division/error diffusion
														 * as well as encoding */
	encode_image_scanline_func encode_image_scanline ;  /* low level interface -
														 * encoding only */
}ASImageOutput;

ASImageOutput *start_image_output( struct ASVisual *asv, ASImage *im, XImage *xim, Bool to_xim, int shift, int quality );
void set_image_output_back_color( ASImageOutput *imout, ARGB32 back_color );
void toggle_image_output_direction( ASImageOutput *imout );
void stop_image_output( ASImageOutput **pimout );

typedef struct ASImageLayer
{
	ASImage *im;
	int dst_x, dst_y;
	/* clip area could be partially outside of the image - iumage gets tiled in it */
	int clip_x, clip_y;
	unsigned int clip_width, clip_height;
	ARGB32 back_color ;                        /* what we want to fill missing scanlines with */
	ARGB32 tint ;                              /* if 0 - no tint */
	ASImageBevel *bevel ;					   /* border to wrap layer with (for buttons, etc.)*/
	int merge_mode ;
	merge_scanlines_func merge_scanlines ;
	void *data;                                /* hook to hung data on */
}ASImageLayer;

typedef struct ASGradient
{
#define GRADIENT_TYPE_MASK          0x0003
#define GRADIENT_TYPE_ORIENTATION   0x0002
#define GRADIENT_TYPE_DIAG          0x0001

#define GRADIENT_Left2Right        		0
#define GRADIENT_TopLeft2BottomRight	GRADIENT_TYPE_DIAG
#define GRADIENT_Top2Bottom				GRADIENT_TYPE_ORIENTATION
#define GRADIENT_BottomLeft2TopRight    (GRADIENT_TYPE_DIAG|GRADIENT_TYPE_ORIENTATION)
	int			type;

	int         npoints;
	ARGB32     *color;
    double     *offset;
}ASGradient;

#define FLIP_VERTICAL       (0x01<<0)
#define FLIP_UPSIDEDOWN		(0x01<<1)

void asimage_init (ASImage * im, Bool free_resources);
void asimage_start (ASImage * im, unsigned int width, unsigned int height, unsigned int compression);

ASScanline*prepare_scanline( unsigned int width, unsigned int shift, ASScanline *reusable_memory, Bool BGR_mode);
void       free_scanline( ASScanline *sl, Bool reusable );

size_t asimage_add_line (ASImage * im, ColorPart color, CARD32 * data, unsigned int y);
size_t asimage_add_line_mono (ASImage * im, ColorPart color, CARD8 value, unsigned int y);

/* usefull for debugging : (returns memory usage)*/
unsigned int asimage_print_line (ASImage * im, ColorPart color,
				 unsigned int y, unsigned long verbosity);
/* this are verbosity flags : */
#define VRB_LINE_SUMMARY 	(0x01<<0)
#define VRB_LINE_CONTENT 	(0x01<<1)
#define VRB_CTRL_EXPLAIN 	(0x01<<2)
#define VRB_EVERYTHING		(VRB_LINE_SUMMARY|VRB_CTRL_EXPLAIN|VRB_LINE_CONTENT)

ASImage *ximage2asimage (struct ASVisual *asv, XImage * xim, unsigned int compression);
ASImage *pixmap2asimage (struct ASVisual *asv, Pixmap p, int x, int y,
	                     unsigned int width, unsigned int height,
		  				 unsigned long plane_mask, Bool keep_cache, unsigned int compression);

XImage* asimage2ximage  (struct ASVisual *asv, ASImage *im);
XImage* asimage2mask_ximage (struct ASVisual *asv, ASImage *im);
Pixmap  asimage2pixmap  (struct ASVisual *asv, Window root, ASImage *im, GC gc, Bool use_cached);
Pixmap  asimage2mask    (struct ASVisual *asv, Window root, ASImage *im, GC gc, Bool use_cached);

/* manipulations : */
ASImage *scale_asimage( struct ASVisual *asv, ASImage *src, unsigned int to_width, unsigned int to_height,
						Bool to_xim, unsigned int compression_out, int quality );
ASImage *tile_asimage ( struct ASVisual *asv, ASImage *src, int offset_x, int offset_y,
  					    unsigned int to_width,  unsigned int to_height, ARGB32 tint,
						Bool to_xim, unsigned int compression_out, int quality );
ASImage *merge_layers ( struct ASVisual *asv, ASImageLayer *layers, int count,
			  		    unsigned int dst_width, unsigned int dst_height,
			  		    Bool to_xim, unsigned int compression_out, int quality );
ASImage *make_gradient( struct ASVisual *asv, struct ASGradient *grad,
               			unsigned int width, unsigned int height, ASFlagType filter,
  			   			Bool to_xim, unsigned int compression_out, int quality  );
ASImage *flip_asimage( struct ASVisual *asv, ASImage *src,
		 		       int offset_x, int offset_y,
			  		   unsigned int to_width, unsigned int to_height,
					   int flip, Bool to_xim, unsigned int compression_out, int quality );



#endif
