#ifndef ASIMAGE_HEADER_FILE_INCLUDED
#define ASIMAGE_HEADER_FILE_INCLUDED

#include "asvisual.h"
#include "blender.h"

struct ASImageDecoder;
struct ASImageOutput;
struct ASScanline;

/****h* libAfterImage/asimage.h
 * SYNOPSIS
 * Defines main structures and function for image manipulation.
 * DESCRIPTION
 * libAfterImage provides powerful functionality to load, store
 * and transform images. It allows for smaller memory utilization by
 * utilizing run-length encoding of the image data. There could be
 * different levels of compression selected, allowing to choose best
 * speed/memory ratio.
 *
 * Transformations can be performed with different degree of quality.
 * Internal engine uses 24.8 bits per channel per pixel. As the result
 * there are no precision loss, while performing complex calculations.
 * Error diffusion algorithms could be used to transform it back into 8
 * bit without quality loss.
 *
 * Any Transformation could be performed with the result written directly
 * into XImage, so that it could be displayed faster.
 *
 * Complex interpolation algorithms are used to perform scaling
 * operations, thus yielding very good quality. All the transformations
 * are performed in integer math, with the result of greater speeds.
 * Optional MMX inline assembly has been incorporated into some
 * procedures, and allows to achieve considerably better performance on
 * compatible CPUs.
 *
 * SEE ALSO
 * Structures :
 *          ASImage
 *          ASImageBevel
 *          ASImageDecoder
 *          ASImageOutput
 *          ASImageLayer
 *          ASGradient
 *
 * Functions :
 *          asimage_init(), asimage_start(), create_asimage(),
 *          destroy_asimage()
 *
 *   Encoding :
 *          asimage_add_line(),	asimage_add_line_mono(),
 *          asimage_print_line()
 *
 *   Decoding
 *          start_image_decoding(), stop_image_decoding()
 *
 *   Output :
 *          start_image_output(), set_image_output_back_color(),
 *          toggle_image_output_direction(), stop_image_output()
 *
 *   X11 conversions :
 *          ximage2asimage(), pixmap2asimage(), asimage2ximage(),
 *          asimage2mask_ximage(), asimage2pixmap(), asimage2mask()
 *
 *   Transformations :
 *          scale_asimage(), tile_asimage(), merge_layers(), make_gradient(),
 *          flip_asimage()
 *
 * Other libAfterImage modules :
 *          asvisual.h, import.h, blender.h, asfont.h
 * AUTHOR
 * Sasha Vasko <sashav at sprintmail dot com>
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
 * ctrl_byte3    := [1NNNNNNN|11111111] (first bit is 1, remaining are
 * 					length. If it is all 1's - then remaining part of
 * 					the line up until image width is monolithic
 * 					uncompressed direct block)
 * SEE ALSO
 *  asimage_init()
 *  asimage_start()
 *  create_asimage()
 *  destroy_asimage()
 * SOURCE
 */
struct ASImageAlternative;

typedef struct ASImage
{
  unsigned int width, height;       /* size of the image in pixels */

  /* pointers to arrays of scanlines of particular channel: */
  CARD8 **alpha,
  		**red,
		**green,
		**blue;
  CARD8 **channels[IC_NUM_CHANNELS];/* merely a shortcut so we can
									 * somewhat simplify code in loops */

  /* internal buffer used for compression/decompression */
  CARD8 *buffer;
  unsigned int buf_used, buf_len;   /* allocated and used size */

  unsigned int max_compressed_width;/* effectively limits compression to
									 * speed things up */

  struct ASImageAlternative
  {  /* alternative forms of ASImage storage : */
   	XImage *ximage ;                /* pointer to XImage created as the
					 				 * result of transformations whenever
									 * we request it to output into
									 * XImage ( see to_xim parameter ) */
	XImage *mask_ximage ;           /* XImage of depth 1 that could be
									 * used to store mask of the image */
	ARGB32 *argb32 ;                /* array of widthxheight ARGB32
									 * values */
  }alt;

} ASImage;
/*******/

typedef enum {
	ASA_ASImage = 0,
    ASA_XImage,
	ASA_MaskXImage,
	ASA_ARGB32,
	ASA_Formats
}ASAltImFormats;

/****d* libAfterImage/LIMITS
 * NAME
 * MAX_IMPORT_IMAGE_SIZE
 * MAX_BEVEL_OUTLINE
 * FUNCTION
 *	MAX_IMPORT_IMAGE_SIZE	effectively limits size of the allowed
 *							images to be loaded from files. That is
 * 							needed to be able to filter out corrupt files.
 *
 * 	MAX_BEVEL_OUTLINE		Limit on bevel outline to be drawn around
 * 							the image.
 * SOURCE
 */
#define MAX_IMPORT_IMAGE_SIZE 	4000
#define MAX_BEVEL_OUTLINE 		10
/******/

/* Auxiliary data structures : */
/****s* libAfterImage/ASImageBevel
 * NAME
 * ASImageBevel
 * SYNOPSIS
 * ASImageBevel describes bevel to be drawn around the image.
 * DESCRIPTION
 * Bevel is used to create 3D effect while drawing buttons, or any other
 * image that needs to be framed. Bevel is drawn using 2 primary colors:
 * one for top and left sides - hi color, and another for bottom and
 * right sides - low color. There are additionally 3 auxiliary colors:
 * hihi is used for the edge of top-left corner, hilo is used for the
 * edge of top-right and bottom-left corners, and lolo is used for the
 * edge of bottom-right corner. Colors are specified as ARGB and contain
 * alpha component, thus allowing for semitransparent bevels.
 *
 * Bevel consists of outline and inline. Outline is drawn outside of the
 * image boundaries and its size adds to image size as the result. Alpha
 * component of the outline is constant. Inline is drawn on top of the
 * image and its alpha component is fading towards the center of the
 * image, thus creating illusion of smooth disappearing edge.
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
 * ASImageDecoder describes the status of reading any particular ASImage,
 * as well as providing detail on how it should be done.
 * DESCRIPTION
 * ASImageDecoder works as an abstraction layer and as the way to
 * automate several operations. Most of the transformations in
 * libAfterImage are performed as operations on ASScanline data
 * structure, that holds all or some of the channels of single image
 * scanline. In order to automate data extraction from ASImage into
 * ASScanline ASImageDecoder has been designed.
 *
 * It has following features :
 * 1) All missing scanlines, or channels of scanlines will be filled with
 * supplied back_color
 * 2) It is possible to leave out some channels of the image, extracting
 * only subset of channels. It is done by setting only needed flags in
 * filter member.
 * 3) It is possible to extract sub-image of the image by setting offset_x
 * and offset_y to top-left corner of sub-image, out_width - to width of
 * the sub-image and calling decode_image_scanline method as many times
 * as height of the sub-image.
 * 4) It is possible to apply bevel to extracted sub-image, by setting
 * bevel member to specific ASImageBevel structure.
 *
 * Extracted Scanlines will be stored in buffer and it will be updated
 * after each call to decode_image_scanline().
 * SOURCE
 */
typedef void (*decode_image_scanline_func)(struct ASImageDecoder *imdec);

typedef struct ASImageDecoder
{
	ASVisual 	   *asv;
	ASImage 	   *im ;
	ASFlagType 		filter;		 /* flags that mask set of channels to
								  * be extracted from the image */

	ARGB32	 		back_color;  /* we fill missing scanlines with this
								  * default - black*/
	unsigned int    offset_x,    /* left margin on source image before
								  * which we skip everything */
					out_width;   /* actual length of the output scanline */
	unsigned int 	offset_y,	 /* top margin */
                    out_height;
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
 * ASImageOutput describes the output state of the transformation result.
 * It is used to transparently write results into ASImage or XImage with
 * different levels of quality.
 * DESCRIPTION
 * libAfterImage allows for transformation result to be stored in both
 * ASImage ( useful for long term storage and subsequent processing )
 * and XImage ( useful for transfer of the result onto the X Server).
 * At the same time there are 4 different quality levels of output
 * implemented. They differ in the way special technics, like error
 * diffusion and interpolation are applyed, and allow for fine grained
 * selection of quality/speed ratio. ASIMAGE_QUALITY_GOOD should be good
 * enough for most applications.
 * The following additional output features are implemented :
 * 1) Filling of the missing channels with supplied values.
 * 2) Error diffusion to improve quality while converting from internal
 * 	  24.8 format to 8 bit format.
 * 3) Tiling of the output. If tiling_step is greater then 0, then each
 * 	  scanlines will be copied into lines found tiling_step one from
 * 	  another, upto the edge of the image.
 * 4) Reverse order of output. Output image will be mirrored along y
 * 	  axis if bottom_to_top is set to True.
 * NOTES
 * The output_image_scanline method should be called for each scanline
 * to be stored. Convenience functions listed below should be used to
 * safely alter state of the output instead of direct manipulation of
 * the data members. (makes you pity you don't write in C++ doesn't it ?)
 * SEE ALSO
 * start_image_output()
 * set_image_output_back_color()
 * toggle_image_output_direction()
 * stop_image_output()
 * SOURCE
 */
typedef void (*encode_image_scanline_func)( struct ASImageOutput *imout,
											ASScanline *to_store );
typedef void (*output_image_scanline_func)( struct ASImageOutput *,
											ASScanline *, int );

typedef struct ASImageOutput
{
	ASVisual 		*asv;
	ASImage  		*im ;
	ASAltImFormats   out_format ;
	CARD32 			 chan_fill[4];
	int 			 buffer_shift;  /* -1 means - buffer is empty,
									 * 0 - no shift,
									 * 8 - use 8 bit precision */
	int 			 next_line ;    /* next scanline to be written */
	unsigned int	 tiling_step;   /* each line written will be repeated
									 * with this step until we exceed
									 * image size */
	int 	    	 bottom_to_top; /* -1 if we should output in
									 * bottom to top order, +1 otherwise*/

	int     		 quality ;		/* see above */

	output_image_scanline_func
		output_image_scanline ;  /* high level interface - division,
								  * error diffusion as well as encoding */
	encode_image_scanline_func
		encode_image_scanline ;  /* low level interface - encoding only */

	/* internal data members : */
	ASScanline 		 buffer[2], *used, *available;
}ASImageOutput;
/********/

/****s* libAfterImage/asimage/ASImageLayer
 * NAME
 * ASImageLayer
 * SYNOPSIS
 * ASImageLayer specifies parameters of the image superimposition.
 * DESCRIPTION
 * libAfterImage allows for simultaneous superimposition (overlaying) of
 * arbitrary number of images. To facilitate this ASImageLayer structure
 * has been created in order to specify parameters of each image
 * participating in overlaying operation. Images need not to be exact
 * same size. For each image its position on destination is specified
 * via dst_x and dst_y data members. Each image maybe tiled and clipped
 * to fit into rectangle specified by clip_x, clip_y, clip_width,
 * clip_height ( in image coordinates - not destination ). Missing
 * scanlines/channels of the image will be filled with back_color.
 * Entire image will be tinted using tint parameter prior to overlaying.
 * Bevel specified by bevel member will be drawn over image prior to
 * overlaying. Specific overlay method has to be specified.
 * merge_scanlines method is pointer to a function,
 * that accepts 2 ASScanlines as arguments and performs overlaying of
 * first one with the second one.
 * There are 15 different merge_scanline methods implemented in
 * libAfterImage, including alpha-blending, tinting, averaging,
 * HSV and HSL colorspace operations, etc.
 * SEE ALSO
 * merge_layers()
 * blender.h
 * SOURCE
 */

typedef struct ASImageLayer
{
	ASImage *im;

	int dst_x, dst_y;						/* placement in overall
											 * composition */

	/* clip area could be partially outside of the image -
	 * image gets tiled in it */
	int clip_x, clip_y;
	unsigned int clip_width, clip_height;

	ARGB32 back_color ;                  	/* what we want to fill
											 * missing scanlines with */
	ARGB32 tint ;                      		/* if 0 - no tint */
	ASImageBevel *bevel ;					/* border to wrap layer with
											 * (for buttons, etc.)*/
	int merge_mode ;                     	/* reserved for future use */
	merge_scanlines_func merge_scanlines ;	/* overlay method */
	void *data;                           	/* hook to hung data on */
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
#define GRADIENT_BottomLeft2TopRight    (GRADIENT_TYPE_DIAG| \
										 GRADIENT_TYPE_ORIENTATION)
/********/

/****s* libAfterImage/ASGradient
 * NAME
 * ASGradient
 * SYNOPSIS
 * ASGradient describes how gradient is to be drawn.
 * DESCRIPTION
 * libAfterImage includes functionality to draw multipoint gradients in
 * 4 different directions left->right, top->bottom and diagonal
 * lefttop->rightbottom and bottomleft->topright. Each gradient described
 * by type, number of colors (or anchor points), ARGB values for each
 * color and offsets of each point from the beginning of gradient in
 * fractions of entire length. There should be at least 2 anchor points.
 * very first point should have offset of 0. and last point should have
 * offset of 1. Gradients are drawn in ARGB colorspace, so it is possible
 * to have semitransparent gradients.
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
/****d* libAfterImage/asimage/tint
 * FUNCTION
 * We use 32 bit ARGB values to define how tinting should be done.
 * The formula for tinting particular channel data goes like that:
 * tinted_data = (image_data * tint)/128
 * So if tint channel value is greater then 127 - same channel will be
 * brighter in destination image; if it is lower then 127 - same channel
 * will be darker in destination image. Tint channel value of 127
 * ( or 0x7F hex ) does not change anything.
 * Alpha channel is tinted as well, allowing for creation of
 * semitransparent images. Calculations are performed in 24.8 format -
 * with 8 bit precision. Result is saturated to avoid overflow, and
 * precision is carried over to next pixel ( error diffusion ), when con
 * verting 24.8 to 8 bit format.
 * SOURCE
 */
#define TINT_NONE			0
#define TINT_LEAVE_SAME     (0x7F7F7F7F)
#define TINT_HALF_DARKER	(0x3F3F3F3F)
#define TINT_HALF_BRIGHTER	(0xCFCFCFCF)
#define TINT_RED			(0x7F7F0000)
#define TINT_GREEN			(0x7F007F00)
#define TINT_BLUE			(0x7F00007F)
/********/
/****d* libAfterImage/asimage/compression
 * FUNCTION
 * Defines the level of compression to attempt on ASImage scanlines.
 * valid values are in range of 0 to 100, with 100 being the highest
 * compression.
 * SOURCE
 */
#define ASIM_COMPRESSION_NONE       0
#define ASIM_COMPRESSION_FULL	   100
/********/

/****f* libAfterImage/asimage/asimage_init()
 * SYNOPSIS
 * void asimage_init (ASImage * im, Bool free_resources);
 * INPUTS
 * im             - pointer to valid ASImage structure
 * free_resources - if True will make function attempt to free
 *                  all non-NULL pointers.
 * DESCRIPTION
 * 	frees datamembers of the supplied ASImage structure, and
 * 	initializes it to all 0.
 *********/
/****f* libAfterImage/asimage/asimage_start()
 * SYNOPSIS
 * void asimage_start (ASImage * im, unsigned int width,
 *                                   unsigned int height,
 *                                   unsigned int compression);
 * INPUTS
 * im          - pointer to valid ASImage structure
 * width       - width of the image
 * height      - height of the image
 * compression - level of compression to perform on image data.
 *               compression has to be in range of 0-100 with 100
 *               signifying highest level of compression.
 * DESCRIPTION
 * Allocates memory needed to store scanline of the image of supplied
 * size. Assigns all the data members valid values. Makes sure that
 * ASImage structure is ready to store image data.
 * NOTES
 * In order to resize ASImage structure after asimage_start() has been
 * called, asimage_init() must be invoked to free all the memory, and
 * then asimage_start() has to be called with new dimensions.
 *********/
/****f* libAfterImage/asimage/asimage_sta()
 * SYNOPSIS
 * void move_asimage_channel( ASImage *dst, int channel_dst,
 *                            ASImage *src, int channel_src );
 * INPUTS
 * dst         - ASImage which will have its channel substituted;
 * channel_dst - what channel to move data to;
 * src         - ASImage which will donate its channel to dst;
 * channel_src - what source image channel to move data from.
 * DESCRIPTION
 * MOves channel data from one ASImage to another, while discarding
 * what was already in destination's channel.
 * NOTES
 * Source image (donor) will loose its channel data, as it will be
 * moved to destination ASImage. Also there is a condition that both
 * images must be of the same width - otherwise function returns
 * without doing anything. If height is different - the minimum of
 * two will be used.
 *********/
/****f* libAfterImage/asimage/create_asimage()
 * SYNOPSIS
 * ASImage *create_asimage( unsigned int width,
 *                          unsigned int height,
 *                          unsigned int compression);
 * INPUTS
 * width       - desired image width
 * height      - desired image height
 * compression - compression level in new ASImage( see asimage_start()
 *               for more ).
 * RETURN VALUE
 * Pointer to newly allocated and initialized ASImage structure on
 * Success. NULL in case of any kind of error - that should never happen.
 * DESCRIPTION
 * Performs memory allocation for the new ASImage structure, as well as
 * initialization of allocated structure based on supplied parameters.
 *********/
/****f* libAfterImage/asimage/destroy_asimage()
 * SYNOPSIS
 * void destroy_asimage( ASImage **im );
 * INPUTS
 * im				- pointer to valid ASImage structure.
 * DESCRIPTION
 * frees all the memory allocated for specified ASImage. If there was
 * XImage attached to it - it will be deallocated as well.
 * EXAMPLE
 * asview.c: ASView.5
 *********/
void asimage_init (ASImage * im, Bool free_resources);
void asimage_start (ASImage * im, unsigned int width, unsigned int height, unsigned int compression);
void move_asimage_channel( ASImage *dst, int channel_dst, ASImage *src, int channel_src );
ASImage *create_asimage( unsigned int width, unsigned int height, unsigned int compression);
void destroy_asimage( ASImage **im );

/****h* libAfterImage/asimage/Encoding
 * DESCRIPTION
 * asimage_add_line()       - encode raw scanline data
 * asimage_add_line_mono()  - encode scanline to have all the same pixels
 * asimage_print_line()     - print stored scanline to stderr.
 ************/
/****f* libAfterImage/asimage/asimage_add_line()
 * SYNOPSIS
 * size_t asimage_add_line ( ASImage * im, ColorPart color,
 *                           CARD32 * data, unsigned int y);
 * INPUTS
 * im      - pointer to valid ASImage structure
 * color   - color channel's number
 * data    - raw channel data of 32 bits per pixel - only lowest 8 bits
 *           gets encoded.
 * y       - image row starting with 0
 * RETURN VALUE
 * asimage_add_line() return size of the encoded channel scanline in
 * bytes. On failure it will return 0.
 * DESCRIPTION
 * Encodes raw data of the single channel into ASImage channel scanline.
 * based on compression level selected for this ASImage all or part of
 * the scanline will be RLE encoded.
 *********/
/****f* libAfterImage/asimage/asimage_add_line_mono()
 * SYNOPSIS
 * size_t asimage_add_line_mono ( ASImage * im, ColorPart color,
 *                                CARD8 value, unsigned int y);
 * INPUTS
 * im				- pointer to valid ASImage structure
 * color			- color channel's number
 * value			- value for the channel
 * y 				- image row starting with 0
 * RETURN VALUE
 * asimage_add_line_mono() return size of the encoded channel scanline
 * in bytes. On failure it will return 0.
 * DESCRIPTION
 * encodes ASImage channel scanline to have same color components
 * value in every pixel. Useful for vertical gradients for example.
 *********/
size_t asimage_add_line (ASImage * im, ColorPart color, CARD32 * data, unsigned int y);
size_t asimage_add_line_mono (ASImage * im, ColorPart color, CARD8 value, unsigned int y);

/****d* libAfterImage/asimage/verbosity
 * FUNCTION
 * This are flags that define what should be printed by
 * asimage_print_line():
 * 	VRB_LINE_SUMMARY	- print only summary for each scanline
 * 	VRB_LINE_CONTENT 	- print summary and data for each scanline
 * 	VRB_CTRL_EXPLAIN 	- print summary, data and control codes for each
 * 						  scanline
 * SOURCE
 */
#define VRB_LINE_SUMMARY 	(0x01<<0)
#define VRB_LINE_CONTENT 	(0x01<<1)
#define VRB_CTRL_EXPLAIN 	(0x01<<2)
#define VRB_EVERYTHING		(VRB_LINE_SUMMARY|VRB_CTRL_EXPLAIN| \
							 VRB_LINE_CONTENT)
/*********/
/****f* libAfterImage/asimage/asimage_print_line()
 * SYNOPSIS
 * 	unsigned int asimage_print_line ( ASImage * im, ColorPart color,
 *									  unsigned int y,
 * 									  unsigned long verbosity);
 * INPUTS
 * im				- pointer to valid ASImage structure
 * color			- color channel's number
 * y 				- image row starting with 0
 * verbosity		- verbosity level - any combination of flags is
 *                  allowed
 * RETURN VALUE
 * amount of memory used by this particular channel of specified
 * scanline.
 * DESCRIPTION
 * asimage_print_line() prints data stored in specified image scanline
 * channel. That may include simple summary of how much memory is used,
 * actual visible data, and/or RLE control codes. That helps to see
 * how effectively data is encoded.
 *
 * Useful mostly for debugging purposes.
 *********/
unsigned int asimage_print_line (ASImage * im, ColorPart color,
				 unsigned int y, unsigned long verbosity);

/****h* libAfterImage/asimage/Decoding
 * DESCRIPTION
 * start_image_decoding()   - allocates and initializes decoder
 *                            structure.
 * stop_image_decoding()    - finishes decoding, frees all allocated
 *                            memory.
 ************/
/****f* libAfterImage/asimage/start_image_decoding()
 * SYNOPSIS
 * ASImageDecoder *start_image_decoding( ASVisual *asv,ASImage *im,
 *                                       ASFlagType filter,
 *                                       int offset_x, int offset_y,
 *                                       unsigned int out_width,
 *                                       unsigned int out_height,
 *                                       ASImageBevel *bevel );
 * INPUTS
 * asv      - pointer to valid ASVisual structure ( needed mostly
 * 			to see if we are in BGR mode or not );
 * im       - ASImage we are going to decode;
 * filter   - bitmask where set bits mark channels that has to be
 * 			decoded.
 * offset_x - left margin inside im, from which we should start
 * 			reading pixel data, effectively clipping source image.
 * offset_y - top margin inside im, from which we should start
 * 			reading scanlines, effectively clipping source image.
 * 			Note that when edge of the image is reached,
 * 			subsequent requests for scanlines will wrap around to
 * 			the top of the image, and not offset_y.
 * out_width- width of the scanline needed. If it is larger then
 * 			source image - then image data will be tiled in it.
 * 			If it is smaller - then image data will be clipped.
 * out_height - height of the output drawable. -1 means that same as
 *          image height. if out_height is greater then image height,
 *          then image will be tiled.
 * bevel    - NULL or pointer to valid ASImageBevel structure if
 * 			decoded data should be overlayed with bevel at the
 * 			time of decoding.
 * RETURN VALUE
 * start_image_decoding() returns pointer to newly allocated
 * ASImageDecoder structure on success, NULL on failure.
 * DESCRIPTION
 * Normal process of reading image data from ASImage consists of
 * 3 steps :
 * 1) start decoding by calling start_image_decoding.
 * 2) call decode_image_scanline() method of returned structure, for
 * each scanline upto desired height of the target image. Decoded data
 * will be returned in buffer member of the ASImageDecoder structure.
 * 3) finish decoding and deallocated all the used memory by calling
 * stop_image_decoding()
 *********/
/****f* libAfterImage/asimage/stop_image_decoding()
 * SYNOPSIS
 * void stop_image_decoding( ASImageDecoder **pimdec );
 * INPUTS
 * pimdec   - pointer to pointer to structure, previously created
 * 			by start_image_decoding.
 * RETURN VALUE
 * pimdec	- pointer to ASImageDecoder will be reset to NULL.
 * SEE ALSO
 * start_image_decoding()
 *******/

ASImageDecoder *start_image_decoding( ASVisual *asv,ASImage *im, ASFlagType filter,
									  int offset_x, int offset_y,
									  unsigned int out_width,
									  unsigned int out_height,
									  ASImageBevel *bevel );
void stop_image_decoding( ASImageDecoder **pimdec );

/****h* libAfterImage/asimage/Output
 * DESCRIPTION
 * start_image_output               - initializes output structure
 * set_image_output_back_color      - changes background color of output
 * toggle_image_output_direction    - reverses vertical direction of
 *                                    output
 * stop_image_output                - finishes output, frees all the
 *                                    allocated memory.
 ************/
/****f* libAfterImage/asimage/start_image_output()
 * SYNOPSIS
 * ASImageOutput *start_image_output ( struct ASVisual *asv,
 *                                     ASImage *im,
 *                                     ASAltImFormats format,
 *                                     int shift, int quality );
 * INPUTS
 * asv      - pointer to valid ASVisual structure
 * im       - destination ASImage
 * format   - indicates that output should be written into alternative
 *            format, such as supplied XImage, ARGB32 array etc.
 * shift    - precision of scanline data. Supported values are 0 - no
 *            precision, and 8 - 24.8 precision. Value of that argument
 *            defines by how much scanline data is shifted rightwards.
 * quality  - what algorithms should be used while writing data out, i.e.
 *            full error diffusion, fast error diffusion, no error
 *            diffusion.
 * DESCRIPTION
 * start_image_output() creates and initializes new ASImageOutput
 * structure based on supplied parameters. Created structure can be
 * subsequently used to write scanlines into destination image.
 * It is effectively hiding differences of XImage and ASImage and other
 * available output formats.
 * outpt_image_scanline() method of the structure can be used to write
 * out single scanline. Each written scanlines moves internal pointer to
 * the next image line, and possibly writes several scanlines at once if
 * tiling_step member is not 0.
 **********/
/****f* libAfterImage/asimage/set_image_output_back_color()
 * SYNOPSIS
 * void set_image_output_back_color ( ASImageOutput *imout,
 *                                    ARGB32 back_color );
 * INPUTS
 * imout		- ASImageOutput structure, previously created with
 *  			  start_image_output();
 * back_color	- new background color value in ARGB format. This color
 *  			  will be used to fill empty parts of outgoing scanlines.
 *********/
/****f* libAfterImage/asimage/toggle_image_output_direction()
 * SYNOPSIS
 * void toggle_image_output_direction( ASImageOutput *imout );
 * INPUTS
 * imout		- ASImageOutput structure, previously created with
 *  			  start_image_output();
 * DESCRIPTION
 * reverses vertical direction output. If previously scanlines has
 * been written from top to bottom, for example, after this function is
 * called they will be written in opposite direction. Current line does
 * not change, unless it points to the very first or the very last
 * image line. In this last case it will be moved to the opposing end of
 * the image.
 *********/
/****f* libAfterImage/asimage/stop_image_output()
 * SYNOPSIS
 * void stop_image_output( ASImageOutput **pimout );
 * INPUTS
 * pimout		- pointer to pointer to ASImageOutput structure,
 *  			  previously created with call to	start_image_output().
 * RETURN VALUE
 * pimout		- pointer to ASImageOutput will be reset to NULL.
 * DESCRIPTION
 * Completes image output process. Flushes all the internal buffers.
 * Deallocates all the allocated memory. Resets pointer to NULL to
 * avoid dereferencing invalid pointers.
 *********/
ASImageOutput *start_image_output( struct ASVisual *asv, ASImage *im, ASAltImFormats format, int shift, int quality );
void set_image_output_back_color( ASImageOutput *imout, ARGB32 back_color );
void toggle_image_output_direction( ASImageOutput *imout );
void stop_image_output( ASImageOutput **pimout );

/****h* libAfterImage/asimage/X11
 * DESCRIPTION
 * ximage2asimage()	- convert XImage structure into ASImage
 * pixmap2asimage()	- convert X11 pixmap into ASImage
 * asimage2ximage()	- convert ASImage into XImage
 * asimage2mask_ximage() - convert alpha channel of ASImage into XImage
 * asimage2pixmap()	- convert ASImage into Pixmap ( possibly using
 * 					  precreated XImage )
 * asimage2mask() 	- convert alpha channel of ASImage into 1 bit
 * 				  	  mask Pixmap.
 *****************/

/****f* libAfterImage/asimage/ximage2asimage()
 * SYNOPSIS
 * ASImage *ximage2asimage ( struct ASVisual *asv, XImage * xim,
 *                           unsigned int compression );
 * INPUTS
 * asv  		 - pointer to valid ASVisual structure
 * xim  		 - source XImage
 * compression - degree of compression of resulting ASImage.
 * RETURN VALUE
 * pointer to newly allocated ASImage, containing encoded data, on
 * success. NULL on failure.
 * DESCRIPTION
 * ximage2asimage will attempt to create new ASImage with the same
 * dimensions as supplied XImage. XImage will be decoded based on
 * supplied ASVisual, and resulting scanlines will be encoded into
 * ASImage.
 *********/
/****f* libAfterImage/asimage/pixmap2asimage()
 * SYNOPSIS
 * ASImage *pixmap2asimage ( struct ASVisual *asv, Pixmap p,
 *                           int x, int y,
 *                           unsigned int width,
 *                           unsigned int height,
 *                           unsigned long plane_mask,
 *                           Bool keep_cache,
 *                           unsigned int compression );
 * INPUTS
 * asv  		  - pointer to valid ASVisual structure
 * p    		  - source Pixmap
 * x, y,
 * width, height- rectangle on Pixmap to be encoded into ASImage.
 * plane_mask   - limits color planes to be copied from Pixmap.
 * keep_cache   - indicates if we should keep XImage, used to copy
 *                image data from the X server, and attached it to ximage
 *                member of resulting ASImage.
 * compression  - degree of compression of resulting ASImage.
 * RETURN VALUE
 * pointer to newly allocated ASImage, containing encoded data, on
 * success. NULL on failure.
 * DESCRIPTION
 * pixmap2asimage will obtain XImage of the requested area of the
 * X Pixmap, and will encode it into ASImage using ximage2asimage()
 * function.
 *********/
ASImage *ximage2asimage (struct ASVisual *asv, XImage * xim, unsigned int compression);
ASImage *pixmap2asimage (struct ASVisual *asv, Pixmap p, int x, int y,
	                     unsigned int width, unsigned int height,
		  				 unsigned long plane_mask, Bool keep_cache, unsigned int compression);

/****f* libAfterImage/asimage/asimage2ximage()
 * SYNOPSIS
 * XImage  *asimage2ximage  (struct ASVisual *asv, ASImage *im);
 * INPUTS
 * asv  		- pointer to valid ASVisual structure
 * im    		- source ASImage
 * RETURN VALUE
 * On success returns newly created and encoded XImage of the same
 * colordepth as the supplied ASVisual. NULL on failure.
 * DESCRIPTION
 * asimage2ximage() creates new XImage of the exact same size as
 * supplied ASImage, and depth of supplied ASVisual. REd, Green and
 * Blue channels of ASImage then gets decoded, and encoded into XImage.
 * Missing scanlines get filled with black color.
 * NOTES
 * Returned pointer to XImage will also be stored in im->alt.ximage,
 * and It will be destroyed when XImage is destroyed, or reused in any
 * subsequent calls to asimage2ximage(). If any other behaviour is
 * desired - make sure you set im->alt.ximage to NULL, to dissociate
 * XImage object from ASImage.
 * SEE ALSO
 * create_visual_ximage()
 *********/
/****f* libAfterImage/asimage/asimage2mask_ximage()
 * SYNOPSIS
 * XImage  *asimage2mask_ximage (struct ASVisual *asv, ASImage *im);
 * INPUTS
 * asv   		- pointer to valid ASVisual structure
 * im    		- source ASImage
 * RETURN VALUE
 * On success returns newly created and encoded XImage of the depth 1.
 * NULL on failure.
 * DESCRIPTION
 * asimage2mask_ximage() creates new XImage of the exact same size as
 * supplied ASImage, and depth 1. Alpha channels of ASImage then gets
 * decoded, and encoded into XImage. If alpha channel is greater the
 * 127 it is encoded as 1, otherwise as 0.
 * Missing scanlines get filled with 1s as they signify absence of mask.
 * NOTES
 * Returned pointer to XImage will also be stored in im->alt.mask_ximage,
 * and It will be destroyed when XImage is destroyed, or reused in any
 * subsequent calls to asimage2mask_ximage(). If any other behaviour is
 * desired - make sure you set im->alt.mask_ximage to NULL, to dissociate
 * XImage object from ASImage.
 *********/
/****f* libAfterImage/asimage/asimage2pixmap()
 * SYNOPSIS
 * Pixmap   asimage2pixmap  ( struct ASVisual *asv, Window root,
 *                            ASImage *im, GC gc, Bool use_cached);
 * INPUTS
 * asv  		- pointer to valid ASVisual structure
 * root 		- root window of destination screen
 * im    		- source ASImage
 * gc   		- precreated GC to use for XImage transfer. If NULL,
 *  			  asimage2pixmap() will use DefaultGC.
 * use_cached	- If True will make asimage2pixmap() to use XImage
 *  			  attached to ASImage, instead of creating new one. Only
 *  			  works if ASImage->ximage data member is not NULL.
 * RETURN VALUE
 * On success returns newly pixmap of the same colordepth as ASVisual.
 * None on failure.
 * DESCRIPTION
 * asimage2pixmap() creates new pixmap of exactly same size as
 * supplied ASImage. It then checks if it needs to encode XImage
 * from ASImage data, and calls asimage2ximage() if yes, it has to.
 * It then uses supplied gc or DefaultGC of the screen to transfer
 * XImage to the server and put it on Pixmap.
 * Missing scanlines get filled with black color.
 * EXAMPLE
 * asview.c: ASView.5
 * SEE ALSO
 * asimage2ximage()
 * create_visual_pixmap()
 *********/
/****f* libAfterImage/asimage/asimage2mask()
 * SYNOPSIS
 * Pixmap   asimage2mask ( struct ASVisual *asv, Window root,
 *                         ASImage *im, GC gc, Bool use_cached);
 * asv        - pointer to valid ASVisual structure
 * root       - root window of destination screen
 * im         - source ASImage
 * gc         - precreated GC for 1 bit deep drawables to use for
 *              XImage transfer. If NULL, asimage2mask() will create one.
 * use_cached - If True will make asimage2mask() to use mask XImage
 *  			attached to ASImage, instead of creating new one. Only
 *  			works if ASImage->alt.mask_ximage data member is not NULL.
 * RETURN VALUE
 * On success returns newly created pixmap of the colordepth 1.
 * None on failure.
 * DESCRIPTION
 * asimage2mask() creates new pixmap of exactly same size as
 * supplied ASImage. It then calls asimage2mask_ximage().
 * It then uses supplied gc, or creates new gc, to transfer
 * XImage to the server and put it on Pixmap.
 * Missing scanlines get filled with 1s.
 * SEE ALSO
 * asimage2mask_ximage()
 **********/
XImage  *asimage2ximage  (struct ASVisual *asv, ASImage *im);
XImage  *asimage2mask_ximage (struct ASVisual *asv, ASImage *im);
Pixmap   asimage2pixmap  (struct ASVisual *asv, Window root, ASImage *im, GC gc, Bool use_cached);
Pixmap   asimage2mask    (struct ASVisual *asv, Window root, ASImage *im, GC gc, Bool use_cached);

/****h* libAfterImage/asimage/Transformations
 * DESCRIPTION
 * scale_asimage()  - scale supplied image into new image of requested
 *                    size.
 * tile_asimage()   - tile image into new image of requested size,
 *                    optionally tinting it.
 * merge_layers()   - overlay arbitrary number of images
 * make_gradient()  - render gradient filled image
 * flip_asimage()   - rotate image in 90 degree increments
 *                    counterclockwise.
 *****************/

/****f* libAfterImage/asimage/scale_asimage()
 * SYNOPSIS
 * ASImage *scale_asimage( struct ASVisual *asv,
 *                         ASImage *src,
 *                         unsigned int to_width,
 *                         unsigned int to_height,
 *                         ASAltImFormats out_format,
 *                         unsigned int compression_out, int quality );
 * INPUTS
 * asv  		- pointer to valid ASVisual structure
 * src   		- source ASImage
 * to_width 	- desired width of the resulting image
 * to_height	- desired height of the resulting image
 * out_format 	- optionally describes alternative ASImage format that
 *                should be produced as the result - XImage, ARGB32, etc.
 * compression_out- compression level of resulting image in range 0-100.
 * quality  	- output quality
 * RETURN VALUE
 * returns newly created and encoded ASImage on success, NULL of failure.
 * DESCRIPTION
 * Scales source ASImage into new image of requested dimensions. If size
 * has to be reduced - then several neighboring pixels will be averaged
 * into single pixel. If size has to be increased then new pixels will
 * be interpolated based on values of four neighboring pixels.
 * EXAMPLE
 * ASScale
 *********/
/****f* libAfterImage/asimage/tile_asimage()
 * SYNOPSIS
 * ASImage *tile_asimage ( struct ASVisual *asv,
 *                         ASImage *src,
 *                         int offset_x,
 *                         int offset_y,
 *                         unsigned int to_width,
 *                         unsigned int to_height,
 *                         ARGB32 tint,
 *                         ASAltImFormats out_format,
 *                         unsigned int compression_out, int quality );
 * INPUTS
 * asv          - pointer to valid ASVisual structure
 * src          - source ASImage
 * offset_x     - left clip margin
 * offset_y     - right clip margin
 * to_width     - desired width of the resulting image
 * to_height    - desired height of the resulting image
 * tint         - ARGB32 value describing tinting color.
 * out_format 	- optionally describes alternative ASImage format that
 *                should be produced as the result - XImage, ARGB32, etc.
 * compression_out- compression level of resulting image in range 0-100.
 * quality      - output quality
 * RETURN VALUE
 * returns newly created and encoded ASImage on success, NULL of failure.
 * DESCRIPTION
 * Source ASImage will be tiled into newly created image of specified
 * size. offset_x and offset_y define origin on source image from which
 * tiling will start. If offset_x or offset_y is outside of the image
 * boundaries, then it will be reduced by whole number of image sizes to
 * fit inside the image. At the time of tiling image will be tinted
 * unless tint == 0.
 * EXAMPLE
 * ASTile
 *********/
/****f* libAfterImage/asimage/merge_layers()
 * SYNOPSIS
 * ASImage *merge_layers  ( struct ASVisual *asv,
 *                          ASImageLayer *layers, int count,
 *                          unsigned int dst_width,
 *                          unsigned int dst_height,
 *                          ASAltImFormats out_format,
 *                          unsigned int compression_out, int quality);
 * INPUTS
 * asv          - pointer to valid ASVisual structure
 * layers       - array of ASImageLayer structures that will be rendered
 *                one on top of another. First element corresponds to
 *                the bottommost layer.
 * dst_width    - desired width of the resulting image
 * dst_height   - desired height of the resulting image
 * out_format 	- optionally describes alternative ASImage format that
 *                should be produced as the result - XImage, ARGB32, etc.
 * compression_out - compression level of resulting image in range 0-100.
 * quality      - output quality
 * RETURN VALUE
 * returns newly created and encoded ASImage on success, NULL of failure.
 * DESCRIPTION
 * merge_layers() will create new ASImage of requested size. It will then
 * go through all the layers, and fill image with composition.
 * Bottommost layer will be used unchanged and above layers will be
 * superimposed on it, using algorithm specified in ASImageLayer
 * structure of the overlaying layer. Layers may have smaller size
 * then destination image, and maybe placed in arbitrary locations. Each
 * layer will be padded to fit width of the destination image with all 0
 * effectively making it transparent.
 *********/
/****f* libAfterImage/asimage/make_gradient()
 * SYNOPSIS
 * ASImage *make_gradient ( struct ASVisual *asv,
 *                          struct ASGradient *grad,
 *                          unsigned int width,
 *                          unsigned int height,
 *                          ASFlagType filter,
 *                          ASAltImFormats out_format,
 *                          unsigned int compression_out, int quality);
 * INPUTS
 * asv          - pointer to valid ASVisual structure
 * grad         - ASGradient structure defining how gradient should be
 *                drawn
 * width        - desired width of the resulting image
 * height       - desired height of the resulting image
 * filter       - only channels corresponding to set bits will be
 *                rendered.
 * out_format 	- optionally describes alternative ASImage format that
 *                should be produced as the result - XImage, ARGB32, etc.
 * compression_out- compression level of resulting image in range 0-100.
 * quality      - output quality
 * RETURN VALUE
 * returns newly created and encoded ASImage on success, NULL of failure.
 * DESCRIPTION
 * make_gradient() will create new image of requested size and it will
 * fill it with gradient, described in structure pointed to by grad.
 * Different dithering techniques will be applied to produce nicer
 * looking gradients.
 *********/
/****f* libAfterImage/asimage/flip_asimage()
 * SYNOPSIS
 * ASImage *flip_asimage ( struct ASVisual *asv,
 *                         ASImage *src,
 *                         int offset_x, int offset_y,
 *                         unsigned int to_width,
 *                         unsigned int to_height,
 *                         int flip, ASAltImFormats out_format,
 *                         unsigned int compression_out, int quality );
 * INPUTS
 * asv          - pointer to valid ASVisual structure
 * src          - source ASImage
 * offset_x     - left clip margin
 * offset_y     - right clip margin
 * to_width     - desired width of the resulting image
 * to_height    - desired height of the resulting image
 * flip         - flip flags determining degree of rotation.
 * out_format 	- optionally describes alternative ASImage format that
 *                should be produced as the result - XImage, ARGB32, etc.
 * compression_out - compression level of resulting image in range 0-100.
 * quality      - output quality
 * RETURN VALUE
 * returns newly created and encoded ASImage on success, NULL of failure.
 * DESCRIPTION
 * flip_asimage() will create new image of requested size, it will then
 * tile source image based on offset_x, offset_y, and destination size,
 * and it will rotate it then based on flip value. Three rotation angles
 * supported 90, 180 and 270 degrees.
 *********/
ASImage *scale_asimage( struct ASVisual *asv, ASImage *src,
						unsigned int to_width, unsigned int to_height,
						ASAltImFormats out_format,
						unsigned int compression_out, int quality );
ASImage *tile_asimage ( struct ASVisual *asv, ASImage *src,
						int offset_x, int offset_y,
  					    unsigned int to_width,  unsigned int to_height, ARGB32 tint,
						ASAltImFormats out_format,
						unsigned int compression_out, int quality );
ASImage *merge_layers ( struct ASVisual *asv, ASImageLayer *layers, int count,
			  		    unsigned int dst_width, unsigned int dst_height,
			  		    ASAltImFormats out_format,
						unsigned int compression_out, int quality );
ASImage *make_gradient( struct ASVisual *asv, struct ASGradient *grad,
               			unsigned int width, unsigned int height, ASFlagType filter,
  			   			ASAltImFormats out_format,
						unsigned int compression_out, int quality  );
ASImage *flip_asimage( struct ASVisual *asv, ASImage *src,
		 		       int offset_x, int offset_y,
			  		   unsigned int to_width, unsigned int to_height,
					   int flip, ASAltImFormats out_format,
					   unsigned int compression_out, int quality );



#endif
