#ifndef ASIMAGE_HEADER_FILE_INCLUDED
#define ASIMAGE_HEADER_FILE_INCLUDED

#include "asvisual.h"
#include "blender.h"

struct ASImageDecoder;
struct ASImageOutput;
struct ASScanline;

/****h* libAfterImage/asimage.h
 * SYNOPSIS
 * Defines main stratures and function for image manipulation
 * DESCRIPTION
 * libAfterImage provides powerfull functionality to load, store
 * and transform images on the client side. Images could be compressed
 * using special run length encoding, with different degree of compression
 * allowing flexibly select speed/memory use ratio.
 *
 * Transformations can be performed with different degree of quality.
 * Internal engine uses 24.8 bits per channel per pixel.
 * Error diffusion algorithms could be used to transform it back into 8
 * bit without quality loss.
 *
 * Any Transformation could be performed with the result written directly
 * into XImage, so that it could e displayed faster.
 *
 * Complex interpolation algorithms are used to perform scaling operations,
 * thus yelding very good quality. All the transformations are performed
 * in integer math, thus it yelding greater speeds.
 *
 * SEE ALSO
 * Structures :
 * 		ASImage
 * 		ASImageBevel
 * 		ASImageDecoder
 * 		ASImageOutput
 * 		ASImageLayer
 * 		ASGradient
 *
 * Functions :
 * 		asimage_init(), asimage_start(), create_asimage(), destroy_asimage()
 *
 *   Low level scanline handling :
 * 		prepare_scanline(), free_scanline(), asimage_add_line(),
 * 		asimage_add_line_mono(), asimage_print_line()
 *
 *   ASImageOutput control :
 * 		start_image_output(), set_image_output_back_color(),
 * 		toggle_image_output_direction(), stop_image_output()
 *
 *   X Server transfer :
 * 		ximage2asimage(), pixmap2asimage(), asimage2ximage(),
 * 		asimage2mask_ximage(), asimage2pixmap(), asimage2mask()
 *
 *   Transformations :
 * 		scale_asimage(), tile_asimage(), merge_layers(), make_gradient(),
 * 		flip_asimage()
 ******
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

/****s* libAfterImage/ASImage
 * NAME
 * ASImage
 * SYNOPSIS
 * ASImage is main structure to hold image data.
 * DESCRIPTION
 * Images are stored internally split into ARGB channels, each split
 * into scanline. Each scanline is stored the following format to allow
 * for RLE compression :
 * component := <line><line>...<line>
 * line      := <block><block>...<block><EOL>
 * block     := <EOL>|<simple_block>|<long_block>|<direct_block>
 *
 * EOL       := 00000000 (all zero bits)
 *
 * simple_block  := <ctrl_byte1><value_byte>
 * ctrl_byte1    := 00NNNNNN (first two bits are 0 remaining are length)
 *
 * long_block    := <ctrl_byte2><more_length_byte><value_byte>
 * ctrl_byte2    := 01NNNNNN (NNNNNN are high 6 bits of length)
 * more_length_byte := low 8 bits of length
 *
 * direct_block  := <ctrl_byte3><value_byte><value_byte>...<value_byte>
 * ctrl_byte3    := [1NNNNNNN|11111111] (first bit is 1, remaining are length.
 *                  If it is all 1's - then remaining part of the line up untill
 *                  image width is monolitic uncompressed direct block)
 * SEE ALSO
 *  asimage_init()
 *  asimage_start()
 *  create_asimage()
 *  destroy_asimage()
 * SOURCE
 */
typedef struct ASImage
{
  unsigned int width, height;       /* size of the image in pixels */
  /* pointers to arraus of scanlines of particular channel: */
  CARD8 **alpha,
  		**red,
		**green,
		**blue;
  CARD8 **channels[IC_NUM_CHANNELS];/* merely a shortcut so we can somewhat
									 * simplify code in loops */
  /* internal buffer used for compression/decompression */
  CARD8 *buffer;
  unsigned int buf_used, buf_len;   /* allocated and used size */

  unsigned int max_compressed_width;/* effectively limits compression to speed things up */

  XImage *ximage ;                  /* pointer to XImage created as the result of
									 * transformations whenever we request it to output
									 * into XImage ( see to_xim parameter ) */
}
ASImage;
/*******/
/****d* libAfterImage/LIMITS
 * NAME
 * MAX_IMPORT_IMAGE_SIZE
 * MAX_BEVEL_OUTLINE
 * FUNCTION
 * MAX_IMPORT_IMAGE_SIZE 	effectively limits size of the allowed images to
 * be loaded from files. That is needed to be able to filter out corrupt files.
 *
 * MAX_BEVEL_OUTLINE 		Limit on bevel outline to be drawn around the image.
 * SOURCE
 */
#define MAX_IMPORT_IMAGE_SIZE 	4000
#define MAX_BEVEL_OUTLINE 		10
/******/

/* Auxilary data structures : */
/****s* libAfterImage/ASImageBevel
 * NAME
 * ASImageBevel
 * SYNOPSIS
 * ASImageBevel describes bevel to be drawn around the image.
 * DESCRIPTION
 * Bevel is used to create 3D effect while drawing buttons, or any other image that
 * needs to be framed. Bevel is drawn using 2 primary colors: one for top and left
 * sides - hi color, and another for bottom and right sides - low color. There are
 * additionally 3 auxilary colors: hihi is used for the edge of top-left corner,
 * hilo is used for the edge of top-right and bottom-left corners, and lolo is used
 * for the edge of bottom-right corner. Colors are specifyed as ARGB and contain
 * alpha component, thus allowing for semitransparent bevels.
 *
 * Bevel consists of outline and inline. Outline is drawn outside of the image
 * boundaries and its size adds to image size as the result. Alpha component of
 * the outline is constant. Inline is drawn on top of the image and its alpha
 * component is fading towards the center of the image, thus creating illusion
 * of smooth disappering edge.
 * SOURCE
 */

typedef struct ASImageBevel
{
	ASFlagType type ;			               /* reserved for future use */
	/* primary bevel colors */
	ARGB32		hi_color, lo_color ;
	/* these will be placed in the corners */
	ARGB32		hihi_color, hilo_color, lolo_color ;
	unsigned short left_outline, top_outline, right_outline, bottom_outline;
	unsigned short left_inline, top_inline, right_inline, bottom_inline;
}ASImageBevel;

/*******/

/****s* libAfterImage/ASImageDecoder
 * NAME
 * ASImageDecoder
 * SYNOPSIS
 * ASImageDecoder describes the status of reading any particular ASImage, as
 * well as providing detail on how it should be done.
 * DESCRIPTION
 * ASImageDecoder works as an abstraction layer and as the way to automate several
 * operations. Most of the transformations in libAfterImage are performed as operations
 * on ASScanline data structure, that holds all or some of the channels of
 * single image scanline. In order to automate data extraction from ASImage into
 * ASScanline ASImageDecoder has been designed. It has following features :
 * 1) All missing scanlines, or channels of scanlines will be filled with supplied
 * back_color
 * 2) It is possible to leave out some channels of the image, extracting only
 * subset of channels. It is done by setting only needed flags in filter member.
 * 3) It is possible to extract subimage of the image by setting offset_x and offset_y
 * to top-left corner of subimage, out_width - to width of the subimage and calling
 * decode_image_scanline method as many times as height of the subimage.
 * 4) It is possible to apply bevel to extracted subimage, by setting bevel member
 * to specific ASImageBevel structure.
 * Extracted Scanlines will be stored in buffer and it will be updated after each
 * call to decode_image_scanline.
 * SOURCE
 */
typedef void (*decode_image_scanline_func)( struct ASImageDecoder *imdec );

typedef struct ASImageDecoder
{
	ASVisual 	   *asv;
	ASImage 	   *im ;
	ASFlagType 		filter;		 /* flags that mask set of channels to be extracted from
								  * the image */

#define ARGB32_DEFAULT_BACK_COLOR	ARGB32_Black  /* default background color is #FF000000 */

	ARGB32	 		back_color;  /* we fill missing scanlines with this - default - black*/
	unsigned int    offset_x,    /* left margin on source image before which we skip everything */
					out_width;   /* actuall length of the output scanline */
	unsigned int 	offset_y;	 /* top margin */
								 /* there is no need for out_height - if we go out of the
								  * image size - we simply reread lines from the beginning
                                  */
	ASImageBevel	*bevel;      /* bevel to wrap everything around with */

	/* scanline buffer containing current scanline */
	ASScanline 		buffer;

	/* internal data : */
	unsigned short   bevel_h_addon, bevel_v_addon ;
	int 			next_line ;
	decode_image_scanline_func decode_image_scanline ;
}ASImageDecoder;
/********/

/****d* libAfterImage/asimage/quality
 * FUNCTION
 * Defines level of output quality/speed ratio
 * SOURCE
 */
#define ASIMAGE_QUALITY_DEFAULT	-1
#define ASIMAGE_QUALITY_POOR	0
#define ASIMAGE_QUALITY_FAST	1
#define ASIMAGE_QUALITY_GOOD	2
#define ASIMAGE_QUALITY_TOP		3

#define MAX_GRADIENT_DITHER_LINES 	ASIMAGE_QUALITY_TOP+1
/*******/


/****s* libAfterImage/asimage/ASImageOutput
 * NAME
 * ASImageOutput
 * SYNOPSIS
 * ASImageOutput describes the output state of the transformation result. It is used
 * to transparently write results into ASImage or XImage with different levels of
 * quality.
 * DESCRIPTION
 * libAfterImage allows for transformation result to be stored in both ASImage ( usefull
 * for long term storage and subsequent processing ) and XImage ( usefull for transfer of
 * the result onto the X Server). At the same time there are 4 different quality levels
 * of output implemented. They differ in the way special technics, like error diffusion
 * and interpolation are applyed, and allow for fine grained selection of quality/speed
 * ratio. ASIMAGE_QUALITY_GOOD should be good enough for most applications.
 * The following additional output features are implemented :
 * 1) Filling of the missing channels with supplied values.
 * 2) Error diffusion to improve quality while converting from internal 24.8 format
 *    to 8 bit format.
 * 3) Tiling of the output. If tiling_step is greater then 0, then each scanlines will
 *    be copied into lines found tiling_step one from another, upto the edge of
 * 	  the image.
 * 4) Reverse order of output. Output image will be mirrored along y axis if
 *    bottom_to_top is set to True.
 * NOTES
 * The output_image_scanline method should be called for each scanline to be stored.
 * Convinience functions listed below should be used to safely alter state of the output
 * instead of direct manipulation of the data members. (makes you pity you don't write in C++
 * doesn't it ?)
 * SEE ALSO
 * start_image_output()
 * set_image_output_back_color()
 * toggle_image_output_direction()
 * stop_image_output()
 * SOURCE
 */
typedef void (*encode_image_scanline_func)( struct ASImageOutput *imout, ASScanline *to_store );
typedef void (*output_image_scanline_func)( struct ASImageOutput *, ASScanline *, int );

typedef struct ASImageOutput
{
	ASVisual 		*asv;
	ASImage  		*im ;
	XImage 			*xim ;
	Bool 			 to_xim ;
	CARD32 			 chan_fill[4];
	int 			 buffer_shift;  /* -1 means - buffer is empty, 0 - no shift,
									 *  8 - use 8 bit presicion */
	int 			 next_line ;    /* next scanlinline to be written */
	unsigned int	 tiling_step;   /* each line written will be repeated with this
									 * step untill we exceed image size */
	Bool 			 bottom_to_top; /* True if we should output in bottom to top order*/

	int 			 quality ;/* see above */

	output_image_scanline_func 	output_image_scanline ;  /* high level interface -
														  * division/error diffusion
														  * as well as encoding */
	encode_image_scanline_func 	encode_image_scanline ;  /* low level interface -
														  * encoding only */
	/* internal data members : */
	unsigned char 	*xim_line;
	int            	 height, bpl;
	ASScanline 		 buffer[2], *used, *available;
}ASImageOutput;
/********/

/****s* libAfterImage/asimage/ASImageLayer
 * NAME
 * ASImageLayer
 * SYNOPSIS
 * ASImageLayer specifies parameters of the image superimposition.
 * DESCRIPTION
 * libAfterImage allows for simultaneous superimposition (overlaying) of arbitrary number of
 * images. To facilitate this ASImageLayer structure has been created in order to
 * specify parameters of each image participating in overlaying operation.
 * Images need not to be exact same size. For each image its position on destination is
 * specifyed via dst_x and dst_y data members. Each image maybe tiled and clipped to fit
 * into rectangle spcifyed by clip_x, clip_y, clip_width, clip_height ( in image coordinates -
 * not destination ). Missing scanlines/channels of the image will be filled with back_color.
 * Entire image will be tinted using tint parameter prior to overlaying. Bevel specified
 * by bevel member will be drawn over image prior to overlaying. Specific overlay
 * method has to be specifyed. Method is pointer to a function, that accepts 2 ASScanlines
 * as arguments and perfoms overlaying of first one with the second one. There are
 * 15 different methods implemented in libAfterImage, including alphablending, tinting,
 * averaging, HSV and HSL colorspace operations, etc.
 * SEE ALSO
 * merge_layers()
 * libAfterImage/blender
 * SOURCE
 */

typedef struct ASImageLayer
{
	ASImage *im;

	int dst_x, dst_y;						   /* placement in overall composition */

	/* clip area could be partially outside of the image - iumage gets tiled in it */
	int clip_x, clip_y;
	unsigned int clip_width, clip_height;

	ARGB32 back_color ;                        /* what we want to fill missing scanlines with */
	ARGB32 tint ;                              /* if 0 - no tint */
	ASImageBevel *bevel ;					   /* border to wrap layer with (for buttons, etc.)*/
	int merge_mode ;                           /* reserved for future use */
	merge_scanlines_func merge_scanlines ;     /* overlay method */
	void *data;                                /* hook to hung data on */
}ASImageLayer;
/********/

/****d* libAfterImage/asimage/GRADIENT_TYPE_flags
 * FUNCTION
 * Combination of this flags defines the way gradient is rendered.
 * SOURCE
 */
#define GRADIENT_TYPE_MASK          0x0003
#define GRADIENT_TYPE_ORIENTATION   0x0002
#define GRADIENT_TYPE_DIAG          0x0001
/********/

/****d* libAfterImage/asimage/GRADIENT_TYPE
 * FUNCTION
 * This are named combinations of above flags to define type of gradient.
 * SEE ALSO
 * GRADIENT_TYPE_flags
 * SOURCE
 */
#define GRADIENT_Left2Right        		0
#define GRADIENT_TopLeft2BottomRight	GRADIENT_TYPE_DIAG
#define GRADIENT_Top2Bottom				GRADIENT_TYPE_ORIENTATION
#define GRADIENT_BottomLeft2TopRight    (GRADIENT_TYPE_DIAG|GRADIENT_TYPE_ORIENTATION)
/********/

/****s* libAfterImage/ASGradient
 * NAME
 * ASGradient
 * SYNOPSIS
 * ASGradient describes how gradient is to be drawn.
 * DESCRIPTION
 * libAfterImage includes functionality to draw multipoint gradients in 4 different
 * directions left->right, top->bottom and diagonal lefttop->rightbottom and
 * bottomleft->topright. Each gradient described by type, number of colors
 * (or anchor points), ARGB values for each color and offsets of each point from the
 * beginning of gradient in fractions of entire length. There should be at least 2
 * anchor points. very first point should have offset of 0. and last point should
 * have offset of 1. Gradients are drawn in ARGB colorspace, so it is possible to have
 * semitransparent gradients.
 * SEE ALSO
 * make_gradient()
 * SOURCE
 */

typedef struct ASGradient
{
	int			type;       /* see GRADIENT_TYPE above */

	int         npoints;    /* number of anchor points */
	ARGB32     *color;      /* ARGB color values for each anchor point  */
    double     *offset;     /* offset of each point from the beginning in
							 * fractions of entire length */
}ASGradient;

/********/

/****d* libAfterImage/asimage/flip
 * FUNCTION
 * This are flags that define rotation angle.
 * FLIP_VERTICAL defines rotation of 90 degrees counterclockwise.
 * FLIP_UPSIDEDOWN defines rotation of 180 degrees counterclockwise.
 * combined they define rotation of 270 degrees counterclockwise.
 * SOURCE
 */
#define FLIP_VERTICAL       (0x01<<0)
#define FLIP_UPSIDEDOWN		(0x01<<1)
/********/

/****f* libAfterImage/asimage/asimage_init()
 * SYNOPSIS
 * 	void asimage_init (ASImage * im, Bool free_resources);
 * DESCRIPTION
 * 	frees datamembers of the supplied ASImage structure, and
 * 	initializes it to all 0.
 * INPUTS
 *	im 				- pointer to valid ASImage structure
 *	free_resources  - if True will make function attempt to free
 * 					  all non-NULL pointers.
 *********/
/****f* libAfterImage/asimage/asimage_start()
 * SYNOPSIS
 * 	void asimage_start (ASImage * im, unsigned int width,
 * 									  unsigned int height,
 * 									  unsigned int compression);
 * DESCRIPTION
 * 	Allocates memory needed to store scanline of the image of supplied size.
 *  Assigns all the data members valid values. Makes sure that ASImage structure
 *	is ready to store image data.
 * INPUTS
 *	im 				- pointer to valid ASImage structure
 *	width			- width of the image
 * 	height			- height of the image
 * 	compression		- level of compression to perform on image data.
 * 					  compression has to be in range of 0-100 with 100
 * 					  signifying highest level of compression.
 * NOTES
 * 	in order to resize ASImage structure after asimage_start() has been called, asimage_init()
 * must be invoked to free all the memory, and then asimage_start() has to be called with
 * new dimentions.
 *********/
/****f* libAfterImage/asimage/create_asimage()
 * SYNOPSIS
 * 	ASImage *create_asimage( unsigned int width,
 * 							 unsigned int height,
 * 							 unsigned int compression);
 * DESCRIPTION
 * INPUTS
 *********/
/****f* libAfterImage/asimage/destroy_asimage()
 * SYNOPSIS
 * 	void destroy_asimage( ASImage **im );
 * DESCRIPTION
 * INPUTS
 *********/
void asimage_init (ASImage * im, Bool free_resources);
void asimage_start (ASImage * im, unsigned int width, unsigned int height, unsigned int compression);
ASImage *create_asimage( unsigned int width, unsigned int height, unsigned int compression);
void destroy_asimage( ASImage **im );

/****f* libAfterImage/asimage/prepare_scanline()
 * SYNOPSIS
 * 	ASScanline *prepare_scanline ( unsigned int width,
 * 							  	   unsigned int shift,
 * 								   ASScanline *reusable_memory,
 * 								   Bool BGR_mode);
 * DESCRIPTION
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

/****f* libAfterImage/asimage/asimage_add_line()
 * SYNOPSIS
 * 	size_t asimage_add_line (ASImage * im, ColorPart color,
 * 							 CARD32 * data, unsigned int y);
 * DESCRIPTION
 * INPUTS
 *********/
/****f* libAfterImage/asimage/asimage_add_line_mono()
 * SYNOPSIS
 * 	size_t asimage_add_line_mono (ASImage * im, ColorPart color,
 * 								  CARD8 value, unsigned int y);
 * DESCRIPTION
 * INPUTS
 *********/
size_t asimage_add_line (ASImage * im, ColorPart color, CARD32 * data, unsigned int y);
size_t asimage_add_line_mono (ASImage * im, ColorPart color, CARD8 value, unsigned int y);

/* this are verbosity flags : */
#define VRB_LINE_SUMMARY 	(0x01<<0)
#define VRB_LINE_CONTENT 	(0x01<<1)
#define VRB_CTRL_EXPLAIN 	(0x01<<2)
#define VRB_EVERYTHING		(VRB_LINE_SUMMARY|VRB_CTRL_EXPLAIN|VRB_LINE_CONTENT)

/****f* libAfterImage/asimage/asimage_print_line()
 * SYNOPSIS
 * 	unsigned int asimage_print_line ( ASImage * im, ColorPart color,
 *									  unsigned int y,
 * 									  unsigned long verbosity);
 * DESCRIPTION
 * INPUTS
 *********/
unsigned int asimage_print_line (ASImage * im, ColorPart color,
				 unsigned int y, unsigned long verbosity);

/****f* libAfterImage/asimage/start_image_output()
 * SYNOPSIS
 * 	ASImageOutput *start_image_output ( struct ASVisual *asv,
 * 										ASImage *im, XImage *xim,
 * 										Bool to_xim,
 * 										int shift, int quality );
 * DESCRIPTION
 * INPUTS
 *********/
/****f* libAfterImage/asimage/set_image_output_back_color()
 * SYNOPSIS
 * 	void set_image_output_back_color (  ASImageOutput *imout,
 * 										ARGB32 back_color );
 * DESCRIPTION
 * INPUTS
 *********/
/****f* libAfterImage/asimage/toggle_image_output_direction()
 * SYNOPSIS
 * 	void toggle_image_output_direction( ASImageOutput *imout );
 * DESCRIPTION
 * INPUTS
 *********/
/****f* libAfterImage/asimage/stop_image_output()
 * SYNOPSIS
 * 	void stop_image_output( ASImageOutput **pimout );
 * DESCRIPTION
 * INPUTS
 *********/
ASImageOutput *start_image_output( struct ASVisual *asv, ASImage *im, XImage *xim, Bool to_xim, int shift, int quality );
void set_image_output_back_color( ASImageOutput *imout, ARGB32 back_color );
void toggle_image_output_direction( ASImageOutput *imout );
void stop_image_output( ASImageOutput **pimout );

ASImage *ximage2asimage (struct ASVisual *asv, XImage * xim, unsigned int compression);
ASImage *pixmap2asimage (struct ASVisual *asv, Pixmap p, int x, int y,
	                     unsigned int width, unsigned int height,
		  				 unsigned long plane_mask, Bool keep_cache, unsigned int compression);

XImage  *asimage2ximage  (struct ASVisual *asv, ASImage *im);
XImage  *asimage2mask_ximage (struct ASVisual *asv, ASImage *im);
Pixmap   asimage2pixmap  (struct ASVisual *asv, Window root, ASImage *im, GC gc, Bool use_cached);
Pixmap   asimage2mask    (struct ASVisual *asv, Window root, ASImage *im, GC gc, Bool use_cached);

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
