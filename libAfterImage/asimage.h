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
 * SEE ALSO
 * Structures :
 *          ASImage
 *          ASImageManager
 *          ASImageBevel
 *          ASImageDecoder
 *          ASImageOutput
 *          ASImageLayer
 *          ASGradient
 *
 * Functions :
 *          asimage_init(), asimage_start(), create_asimage(),
 *          clone_asimage(), destroy_asimage()
 *
 *   ImageManager Reference counting and managing :
 *          create_image_manager(), destroy_image_manager(),
 *          store_asimage(), fetch_asimage(), dup_asimage(),
 *          release_asimage(), release_asimage_by_name()
 *
 *   Layers helper functions :
 *          init_image_layers(), create_image_layers(),
 *          destroy_image_layers()
 *
 *   Encoding :
 *          asimage_add_line(),	asimage_add_line_mono(),
 *          asimage_print_line(), get_asimage_chanmask(),
 *          move_asimage_channel(), copy_asimage_channel(),
 *          copy_asimage_lines()
 *
 *   Decoding
 *          start_image_decoding(), stop_image_decoding(),
 *          asimage_decode_line (), set_decoder_shift(),
 *          set_decoder_back_color()
 *
 *   Output :
 *          start_image_output(), set_image_output_back_color(),
 *          toggle_image_output_direction(), stop_image_output()
 *
 * Other libAfterImage modules :
 *          ascmap.h asfont.h asimage.h asvisual.h blender.h export.h
 *          import.h transform.h ximage.h
 * AUTHOR
 * Sasha Vasko <sasha at aftercode dot net>
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
struct ASImageManager;

/* magic number identifying ASFont data structure */
#define MAGIC_ASIMAGE            0xA3A314AE

/****d* libAfterImage/ASAltImFormats
 * FUNCTION
 * Identifies what output format should be used for storing the
 * transformation result. Also identifies what data is currently stored
 * in alt member of ASImage structure.
 * SOURCE
 */
typedef enum {
	ASA_ASImage = 0,
    ASA_XImage,
	ASA_MaskXImage,
	ASA_ARGB32,
	ASA_Vector,       /* This cannot be used for transformation's result
					   * format */
	ASA_Formats
}ASAltImFormats;
/*******/

typedef struct ASImage
{

  unsigned long magic ;

  unsigned int width, height;       /* size of the image in pixels */

  /* pointers to arrays of scanlines of particular channel: */
  CARD8 **alpha,
  		**red,
		**green,
		**blue;
  CARD8 **channels[IC_NUM_CHANNELS];/* merely a shortcut so we can
									 * somewhat simplify code in loops */

  ARGB32 back_color ;               /* background color of the image, so
									 * we could discard everything that
									 * matches it, and then restore it
									 * back. */
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
	double *vector ;			    /* scientific data that should be used
									 * in conjunction with
									 * ASScientificPalette to produce
									 * actuall ARGB data */
  }alt;

  struct ASImageManager *imageman;  /* if not null - then image could be
									 * referenced by some other code */
  int                    ref_count ;/* this will tell us what us how many
									 * times */
  char                  *name ;     /* readonly copy of image name */
} ASImage;
/*******/

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
#define MAX_BEVEL_OUTLINE 		100
#define MAX_SEARCH_PATHS		8      /* prudently limiting ourselfs */
/******/

/****s* libAfterImage/ASImageManager
 * NAME
 * ASImageManager
 * DESCRIPTION
 * Global data identifying connection to external libraries, as well as
 * images location paths.
 * This structure could be used to maintain repository of loaded images
 * in order to avoid loading same image more then once.
 * It holds hash of loaded image names.
 *
 * SOURCE
 */
typedef struct ASImageManager
{
	ASHashTable  *image_hash ;
	/* misc stuff that may come handy : */
	char 	     *search_path[MAX_SEARCH_PATHS+1];
	double 		  gamma ;
}ASImageManager;
/*************/


/* Auxiliary data structures : */
/****s* libAfterImage/ASScientificPalette
 * NAME
 * ASScientificPalette
 * DESCRIPTION
 * Contains pallette allowing us to map double values in vector image
 * data into actuall ARGB values.
 * SOURCE
 */
typedef struct ASVectorPalette
{
	unsigned int npoints ;
	double *points ;
	CARD16 *channels[IC_NUM_CHANNELS] ;        /* ARGB data for key points. */
	ARGB32  default_color;
}ASVectorPalette;
/*************/

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
#define BEVEL_SOLID_INLINE	(0x01<<0)
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

	/* offsets of the drawn bevel baseline on resulting image : */
	int            bevel_left, bevel_top, bevel_right, bevel_bottom ;

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
 *
 * Also There is a trick in the way how output_image_scanline handles
 * empty scanlines while writing ASImage. If back_color of empty scanline
 * matches back_color of ASImageOutput - then particular line is erased!
 * If back_colors are same - then particular line of ASImage gets filled
 * with the back_color of ASScanline. First approach is usefull when
 * resulting image will be used in subsequent call to merge_layers - in
 * such case knowing back_color of image is good enough and we don't need
 * to store lines with the same color. In case where ASImage will be
 * converted into Pixmap/XImage - second approach is preferable, since
 * that conversion does not take into consideration image's back color -
 * we may want to change it in the future.
 *
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
 * clip_height ( in image coordinates - not destination ). If image is
 * missing, then area specified by dst_x, dst_y, clip_width, clip_height
 * will be filled with solid_color.
 * Entire image will be tinted using tint parameter prior to overlaying.
 * Bevel specified by bevel member will be drawn over image prior to
 * overlaying. Specific overlay method has to be specified.
 * merge_scanlines method is pointer to a function,
 * that accepts 2 ASScanlines as arguments and performs overlaying of
 * first one with the second one.
 * There are 15 different merge_scanline methods implemented in
 * libAfterImage, including alpha-blending, tinting, averaging,
 * HSV and HSL colorspace operations, etc.
 * NOTES
 * ASImageLayer s could be organized into chains using next pointers.
 * Since there could be a need to rearrange layers and maybe bypass some
 * layers - we need to provide for flexibility, while at the same time
 * allowing for simplicity of arrays. As the result next pointers could
 * be used to link together continuous arrays of layer, like so :
 * array1: [layer1(next==NULL)][layer2(next!=NULL)]
 *          ____________________________|
 *          V
 * array2: [layer3(next==NULL)][layer4(next==NULL)][layer5(next!=NULL)]
 *          ________________________________________________|
 *          V
 * array3: [layer6(next==NULL)][layer7(next==layer7)]
 *                                ^______|
 *
 * While iterating throught such a list we check for two conditions -
 * exceeding count of layers and layer pointing to self. When any of
 * that is met - we stopping iteration.
 * SEE ALSO
 * merge_layers()
 * blender.h
 * SOURCE
 */

typedef struct ASImageLayer
{
	ASImage *im;
	ARGB32   solid_color ;                  /* If im == NULL, then fill
											 * the area with this color. */

	int dst_x, dst_y;						/* placement in overall
											 * composition */

	/* clip area could be partially outside of the image -
	 * image gets tiled in it */
	int clip_x, clip_y;
	unsigned int clip_width, clip_height;

	ARGB32 tint ;                      		/* if 0 - no tint */
	ASImageBevel *bevel ;					/* border to wrap layer with
											 * (for buttons, etc.)*/

	/* if image is clipped then we need to specify offsets of bevel as
	 * related to clipped rectangle. Normally it should be :
	 * 0, 0, im->width, im->height. And if width/height left 0 - it will
	 * default to this values. Note that clipped image MUST be entirely
	 * inside the bevel rectangle. !!!*/
	int bevel_x, bevel_y;
	unsigned int bevel_width, bevel_height;

	int merge_mode ;                     	/* reserved for future use */
	merge_scanlines_func merge_scanlines ;	/* overlay method */
	struct ASImageLayer *next;              /* optional pointer to next
											 * layer. If it points to
											 * itself - then end of the
											 * chain.*/
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
#define FLIP_MASK			(FLIP_UPSIDEDOWN|FLIP_VERTICAL)
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

extern Bool asimage_use_mmx ;
int mmx_init(void);
int mmx_off(void);

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
/****f* libAfterImage/asimage/clone_asimage()
 * SYNOPSIS
 * ASImage *clone_asimage(ASImage *src, ASFlagType filter );
 * INPUTS
 * src      - original ASImage.
 * filter   - bitmask of channels to be copied from one image to another.
 * RETURN VALUE
 * New ASImage, as a copy of original image.
 * DESCRIPTION
 * Creates exact clone of the original ASImage, with same compression,
 * back_color and rest of the attributes. Only ASImage data will be
 * carried over. Any attached alternative forms of images (XImages, etc.)
 * will not be copied. Any channel with unset bit in filter will not be
 * copied. Image name, ASImageManager and ref_count will not be copied -
 * use store_asimage() afterwards and make sure you use different name,
 * to avoid clashes with original image.
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
ASImage *create_asimage( unsigned int width, unsigned int height, unsigned int compression);
ASImage *clone_asimage( ASImage *src, ASFlagType filter );
void destroy_asimage( ASImage **im );
Bool set_asimage_vector( ASImage *im, register double *vector );

/****h* libAfterImage/asimage/ImageManager
 * DESCRIPTION
 * create_image_manager()  - create ASImage management and reference
 *                           counting object.
 * destroy_image_manager() - destroy management obejct.
 * store_asimage()         - add ASImage to the refererence.
 * fetch_asimage()         - retrieve previously stored image, incrementing
 *                           reference count.
 * dup_asimage()           - increment reference count of stored ASImage.
 * release_asimage()       - decrement reference count/destroy ASImage.
 * release_asimage_by_name()
 *********/
/****f* libAfterImage/asimage/create_image_manager()
 * SYNOPSIS
 * ASImageManager *create_image_manager( ASImageManager *reusable_memory,
 *                                       double gamma, ... );
 * INPUTS
 * reusable_memory - optional pointer to a block of memory to be used to
 *                   store ASImageManager object.
 * double gamma    - value of gamma correction to be used while loading
 *                   images from files.
 * ...             - NULL terminated list of up to 8 PATH strings to list
 *                   locations at which images could be found.
 * DESCRIPTION
 * Creates ASImageManager object in memory and initializes it with
 * requested gamma value and PATH list. This Object will contain a hash
 * table referencing all the loaded images. When such object is used while
 * loading images from the file - gamma and PATH values will be used, so
 * that all the loaded and referenced images will have same parameters.
 * File name will be used as the image name, and if same file is attempted
 * to be loaded again - instead reference will be incremented, and
 * previously loaded image will be retyrned. All the images stored in
 * ASImageManager's table will contain a back pointer to it, and they must
 * be deallocated only by calling release_asimage(). destroy_asimage() will
 * refuse to deallocate such an image.
 *********/
/****f* libAfterImage/asimage/destroy_image_manager()
 * SYNOPSIS
 * void destroy_image_manager( struct ASImageManager *imman, Bool reusable );
 * INPUTS
 * imman           - pointer to ASImageManager object to be deallocated
 * reusable        - if True, then memory that holds object itself will
 *                   not be deallocated. Usefull when object is created
 *                   on stack.
 * DESCRIPTION
 * Destroys all the referenced images, PATH values and if reusable is False,
 * also deallocates object's memory.
 *********/
ASImageManager *create_image_manager( struct ASImageManager *reusable_memory, double gamma, ... );
void     destroy_image_manager( struct ASImageManager *imman, Bool reusable );

/****f* libAfterImage/asimage/store_asimage()
 * SYNOPSIS
 * Bool store_asimage( ASImageManager* imageman, ASImage *im, const char *name );
 * INPUTS
 * imageman        - pointer to valid ASImageManager object.
 * im              - pointer to the image to be stored.
 * name            - unique name of the image.
 * DESCRIPTION
 * Adds specifyed image to the ASImageManager's list of referenced images.
 * Stored ASImage could be deallocated only by release_asimage(), or when
 * ASImageManager object itself is destroyed.
 *********/
/****f* libAfterImage/asimage/fetch_asimage()
 * SYNOPSIS
 * ASImage *fetch_asimage( ASImageManager* imageman, const char *name );
 * INPUTS
 * imageman        - pointer to valid ASImageManager object.
 * name            - unique name of the image.
 * DESCRIPTION
 * Looks for image with the name in ASImageManager's list and if found,
 * it will increment reference count and return pointer to it.
 *********/
/****f* libAfterImage/asimage/dup_asimage()
 * SYNOPSIS
 * ASImage *dup_asimage( ASImage* im );
 * INPUTS
 * im              - pointer to already referenced image.
 * DESCRIPTION
 * Increments reference count on the specifyed ASImage.
 *********/
/****f* libAfterImage/asimage/release_asimage()
 * SYNOPSIS
 * int	release_asimage( ASImage *im );
 * INPUTS
 * im              - pointer to already referenced image.
 * DESCRIPTION
 * Decrements reference count on the ASImage object and destroys it if
 * reference count is below zero.
 *********/
/****f* libAfterImage/asimage/release_asimage_by_name()
 * SYNOPSIS
 * int release_asimage_by_name( ASImageManager *imman, char *name );
 * INPUTS
 * imageman        - pointer to valid ASImageManager object.
 * name            - unique name of the image.
 * DESCRIPTION
 * Finds ASImage known by specified name in ASImageManager's list and
 * then calls release_asimage() on that image.
 *********/
Bool     store_asimage( ASImageManager* imageman, ASImage *im, const char *name );
ASImage *fetch_asimage( ASImageManager* imageman, const char *name );
ASImage *dup_asimage  ( ASImage* im );         /* increment ref countif applicable */
int      release_asimage( ASImage *im );
int		 release_asimage_by_name( ASImageManager *imman, char *name );

/****h* libAfterImage/asimage/Layers
 * DESCRIPTION
 * init_image_layers()    - initialize set of ASImageLayer structures.
 * create_image_layers()  - allocate and initialize set of ASImageLayer's.
 * destroy_image_layers() - destroy set of ASImageLayer structures.
 *********/
/****f* libAfterImage/asimage/init_image_layers()
 * SYNOPSIS
 * inline void init_image_layers( register ASImageLayer *l, int count );
 * INPUTS
 * l              - pointer to valid ASImage structure.
 * count          - number of elements to initialize.
 * DESCRIPTION
 * Initializes array on ASImageLayer structures to sensible defaults.
 * Basically - all zeros and merge_scanlines == alphablend_scanlines.
 *********/
/****f* libAfterImage/asimage/create_image_layers()
 * SYNOPSIS
 * ASImageLayer *create_image_layers( int count );
 * INPUTS
 * count       - number of ASImageLayer structures in allocated array.
 * RETURN VALUE
 * Pointer to newly allocated and initialized array of ASImageLayer
 * structures on Success. NULL in case of any kind of error - that
 * should never happen.
 * DESCRIPTION
 * Performs memory allocation for the new array of ASImageLayer
 * structures, as well as initialization of allocated structure to
 * sensible defaults - merge_func will be set to alphablend_scanlines.
 *********/
/****f* libAfterImage/asimage/destroy_image_layers()
 * SYNOPSIS
 * void destroy_image_layers( register ASImageLayer *l,
 *                            int count,
 *                            Bool reusable );
 * l				- pointer to pointer to valid array of ASImageLayer
 *                    structures.
 * count            - number of structures in array.
 * reusable         - if True - then array itself will not be deallocates -
 *                    which is usable when it was allocated on stack.
 * DESCRIPTION
 * frees all the memory allocated for specified array of ASImageLayer s.
 * If there was ASImage and/or ASImageBevel attached to it - it will be
 * deallocated as well.
 *********/

inline void init_image_layers( register ASImageLayer *l, int count );
ASImageLayer *create_image_layers( int count );
void destroy_image_layers( register ASImageLayer *l, int count, Bool reusable );

/****h* libAfterImage/asimage/Encoding
 * DESCRIPTION
 * asimage_add_line()       - encode raw scanline data
 * asimage_add_line_mono()  - encode scanline to have all the same pixels
 * get_asimage_chanmask()   - determine what channels contain data.
 * asimage_print_line()     - print stored scanline to stderr.
 * asimage_decode_line()    - decode single scanline of the ASImage
 * move_asimage_channel()   - move channel's data from one image to another.
 * copy_asimage_channel()   - duplicate channel's data from one image to
 *                            another.
 * copy_asimage_lines()     - duplicate range of scanline from one image
 *                            to another.
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
/****f* libAfterImage/asimage/get_asimage_chanmask()
 * SYNOPSIS
 * ASFlagType get_asimage_chanmask( ASImage *im);
 * INPUTS
 * im         - valid ASImage object.
 * DESCRIPTION
 * goes throu all the scanlines of the ASImage and toggles bits representing
 * those components that have at least some data.
 *********/
/****f* libAfterImage/asimage/move_asimage_channel()
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
/****f* libAfterImage/asimage/copy_asimage_channel()
 * SYNOPSIS
 * void copy_asimage_channel( ASImage *dst, int channel_dst,
 *                            ASImage *src, int channel_src );
 * INPUTS
 * dst         - ASImage which will have its channel substituted;
 * channel_dst - what channel to copy data to;
 * src         - ASImage which will donate its channel to dst;
 * channel_src - what source image channel to copy data from.
 * DESCRIPTION
 * Same as move_asimage_channel() but makes copy of channel's data
 * instead of simply moving it from one image to another.
 *********/
/****f* libAfterImage/asimage/copy_asimage_lines()
 * SYNOPSIS
 * void copy_asimage_lines( ASImage *dst, unsigned int offset_dst,
 *                          ASImage *src, unsigned int offset_src,
 *                          unsigned int nlines, ASFlagType filter );
 * INPUTS
 * dst         - ASImage which will have its channel substituted;
 * offset_dst  - scanline in destination image to copy to;
 * src         - ASImage which will donate its channel to dst;
 * offset_src  - scanline in source image to copy data from;
 * nlines      - number of scanlines to be copied;
 * filter      - specifies what channels should be copied.
 * DESCRIPTION
 * Makes copy of scanline data for continuos set of scanlines, affecting
 * only those channels marked in filter.
 * NOTE
 * Images must be of the same width.
 *********/
size_t asimage_add_line (ASImage * im, ColorPart color, CARD32 * data, unsigned int y);
size_t asimage_add_line_mono (ASImage * im, ColorPart color, CARD8 value, unsigned int y);
ASFlagType get_asimage_chanmask( ASImage *im);
inline int asimage_decode_line (ASImage * im, ColorPart color, CARD32 * to_buf, unsigned int y, unsigned int skip, unsigned int out_width);
void move_asimage_channel( ASImage *dst, int channel_dst, ASImage *src, int channel_src );
void copy_asimage_channel( ASImage *dst, int channel_dst, ASImage *src, int channel_src );
void copy_asimage_lines( ASImage *dst, unsigned int offset_dst,
                    	 ASImage *src, unsigned int offset_src,
						 unsigned int nlines, ASFlagType filter );
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
void print_asimage( ASImage *im, int flags, char * func, int line );
/* the following 5 macros will in fact unfold into huge but fast piece of code : */
/* we make poor compiler work overtime unfolding all this macroses but I bet it  */
/* is still better then C++ templates :)									     */

#define ENCODE_SCANLINE(im,src,y) \
do{	asimage_add_line((im), IC_RED,   (src).red,   (y)); \
   	asimage_add_line((im), IC_GREEN, (src).green, (y)); \
   	asimage_add_line((im), IC_BLUE,  (src).blue,  (y)); \
	if( get_flags((src).flags,SCL_DO_ALPHA))asimage_add_line((im), IC_ALPHA, (src).alpha, (y)); \
  }while(0)

#define SCANLINE_FUNC(f,src,dst,scales,len) \
do{	if( (src).offset_x > 0 || (dst).offset_x > 0 ) \
		LOCAL_DEBUG_OUT( "(src).offset_x = %d. (dst).offset_x = %d", (src).offset_x, (dst).offset_x ); \
	f((src).red+(src).offset_x,  (dst).red+(dst).offset_x,  (scales),(len));		\
	f((src).green+(src).offset_x,(dst).green+(dst).offset_x,(scales),(len)); 		\
	f((src).blue+(src).offset_x, (dst).blue+(dst).offset_x, (scales),(len));   	\
	if(get_flags((src).flags,SCL_DO_ALPHA)) f((src).alpha+(src).offset_x,(dst).alpha+(dst).offset_x,(scales),(len)); \
  }while(0)

#define CHOOSE_SCANLINE_FUNC(r,src,dst,scales,len) \
 switch(r)                                              							\
 {  case 0:	SCANLINE_FUNC(shrink_component11,(src),(dst),(scales),(len));break;   	\
	case 1: SCANLINE_FUNC(shrink_component, (src),(dst),(scales),(len));	break;  \
	case 2:	SCANLINE_FUNC(enlarge_component12,(src),(dst),(scales),(len));break ; 	\
	case 3:	SCANLINE_FUNC(enlarge_component23,(src),(dst),(scales),(len));break;  	\
	default:SCANLINE_FUNC(enlarge_component,  (src),(dst),(scales),(len));        	\
 }

#define SCANLINE_MOD(f,src,p,len) \
do{	f((src).red+(src).offset_x,(p),(len));		\
	f((src).green+(src).offset_x,(p),(len));		\
	f((src).blue+(src).offset_x,(p),(len));		\
	if(get_flags((src).flags,SCL_DO_ALPHA)) f((src).alpha+(src).offset_x,(p),(len));\
  }while(0)

#define SCANLINE_COMBINE_slow(f,c1,c2,c3,c4,o1,o2,p,len)						   \
do{	f((c1).red,(c2).red,(c3).red,(c4).red,(o1).red,(o2).red,(p),(len));		\
	f((c1).green,(c2).green,(c3).green,(c4).green,(o1).green,(o2).green,(p),(len));	\
	f((c1).blue,(c2).blue,(c3).blue,(c4).blue,(o1).blue,(o2).blue,(p),(len));		\
	if(get_flags((c1).flags,SCL_DO_ALPHA)) f((c1).alpha,(c2).alpha,(c3).alpha,(c4).alpha,(o1).alpha,(o2).alpha,(p),(len));	\
  }while(0)

#define SCANLINE_COMBINE(f,c1,c2,c3,c4,o1,o2,p,len)						   \
do{	f((c1).red,(c2).red,(c3).red,(c4).red,(o1).red,(o2).red,(p),(len+(len&0x01))*3);		\
	if(get_flags((c1).flags,SCL_DO_ALPHA)) f((c1).alpha,(c2).alpha,(c3).alpha,(c4).alpha,(o1).alpha,(o2).alpha,(p),(len));	\
  }while(0)


/* note that we shift values by 8 to keep quanitzation error in   */
/* lower 1 byte for subsequent dithering 	:					  */
#define QUANT_ERR_BITS  	8
#define QUANT_ERR_MASK  	0x000000FF

inline void copy_component( register CARD32 *src, register CARD32 *dst, int *unused, int len );

/****h* libAfterImage/asimage/Decoding
 * DESCRIPTION
 * start_image_decoding()   - allocates and initializes decoder
 *                            structure.
 * set_decoder_shift()      - changes the shift value of decoder - 8 or 0.
 * set_decoder_back_color() - changes the back color to be used while
 *                            decoding the image.
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
/****f* libAfterImage/asimage/set_decoder_bevel_geom()
 * SYNOPSIS
 * void set_decoder_bevel_geom( ASImageDecoder *imdec, int x, int y,
 *                              unsigned int width, unsigned int height );
 * INPUTS
 * imdec   - pointer to pointer to structure, previously created
 *           by start_image_decoding.
 * x,y     - left top position of the inner border of the Bevel outline
 *           as related to the origin of subimage being decoded.
 * width,
 * height  - widtha and height of the inner border of the bevel outline.
 * DESCRIPTION
 * This function should be used to change default placement of the bevel
 * on decoded image. For example if you only need to render small part of
 * the button, that is being rendered from transparency image.
 * NOTE
 * This call modifies bevel_h_addon and bevel_v_addon of
 * ASImageDecoder structure.
 *******/
/****f* libAfterImage/asimage/set_decoder_shift()
 * SYNOPSIS
 * void set_decoder_shift( ASImageDecoder *imdec, int shift );
 * INPUTS
 * imdec   - pointer to pointer to structure, previously created
 *            by start_image_decoding.
 * shift   - new value to be used as the shift while decoding image.
 *           valid values are 8 and 0.
 * DESCRIPTION
 * This function should be used instead of directly modifyeing value of
 * shift memebr of ASImageDecoder structure.
 *******/
/****f* libAfterImage/asimage/set_decoder_back_color()
 * SYNOPSIS
 * void set_decoder_back_color( ASImageDecoder *imdec, ARGB32 back_color );
 * INPUTS
 * imdec      - pointer to pointer to structure, previously created
 *              by start_image_decoding.
 * back_color - ARGB32 color value to be used as the background color to
 *              fill empty spaces in decoded ASImage.
 * DESCRIPTION
 * This function should be used instead of directly modifyeing value of
 * back_color memebr of ASImageDecoder structure.
 *******/
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
void set_decoder_bevel_geom( ASImageDecoder *imdec, int x, int y,
     	                     unsigned int width, unsigned int height );
void set_decoder_shift( ASImageDecoder *imdec, int shift );
void set_decoder_back_color( ASImageDecoder *imdec, ARGB32 back_color );
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



#endif
