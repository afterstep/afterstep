#ifndef TRANSFORM_HEADER_FILE_INCLUDED
#define TRANSFORM_HEADER_FILE_INCLUDED

#include "asvisual.h"
#include "blender.h"
#include "asimage.h"

/****h* libAfterImage/transform.h
 * SYNOPSIS
 * Defines transformations that could be performed on ASImage.
 * DESCRIPTION
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
 *   Transformations :
 *          scale_asimage(), tile_asimage(), merge_layers(), make_gradient(),
 *          flip_asimage(), pad_asimage(), blur_asimage_gauss(),
 *          fill_asimage()
 *
 * Other libAfterImage modules :
 *          ascmap.h asfont.h asimage.h asvisual.h blender.h export.h
 *          import.h transform.h ximage.h
 * AUTHOR
 * Sasha Vasko <sasha at aftercode dot net>
 ******
 */
/****f* libAfterImage/transform/scale_asimage()
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
/****f* libAfterImage/transform/tile_asimage()
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
/****f* libAfterImage/transform/merge_layers()
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
/****f* libAfterImage/transform/make_gradient()
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
/****f* libAfterImage/transform/flip_asimage()
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
/****f* libAfterImage/transform/mirror_asimage()
 * SYNOPSIS
 * ASImage *mirror_asimage ( struct ASVisual *asv,
 *                           ASImage *src,
 *                           int offset_x, int offset_y,
 *                           unsigned int to_width,
 *                           unsigned int to_height,
 *                           Bool vertical, ASAltImFormats out_format,
 *                           unsigned int compression_out, int quality );
 * INPUTS
 * asv          - pointer to valid ASVisual structure
 * src          - source ASImage
 * offset_x     - left clip margin
 * offset_y     - right clip margin
 * to_width     - desired width of the resulting image
 * to_height    - desired height of the resulting image
 * vertical     - mirror in vertical direction.
 * out_format 	- optionally describes alternative ASImage format that
 *                should be produced as the result - XImage, ARGB32, etc.
 * compression_out - compression level of resulting image in range 0-100.
 * quality      - output quality
 * RETURN VALUE
 * returns newly created and encoded ASImage on success, NULL of failure.
 * DESCRIPTION
 * mirror_asimage() will create new image of requested size, it will then
 * tile source image based on offset_x, offset_y, and destination size,
 * and it will mirror it in vertical or horizontal direction.
 *********/
/****f* libAfterImage/transform/pad_asimage()
 * SYNOPSIS
 * ASImage *pad_asimage( ASVisual *asv, ASImage *src,
 *                      int dst_x, int dst_y,
 *                      unsigned int to_width,
 *                      unsigned int to_height,
 *                      ARGB32 color,
 *                      ASAltImFormats out_format,
 *                      unsigned int compression_out, int quality );
 * INPUTS
 * asv          - pointer to valid ASVisual structure
 * src          - source ASImage
 * dst_x, dst_y - placement of the source image relative to the origin of
 *                destination image
 * to_width     - width of the destination image
 * to_height    - height of the destination image
 * color        - ARGB32 color value to pad with.
 * out_format 	- optionally describes alternative ASImage format that
 *                should be produced as the result - XImage, ARGB32, etc.
 * compression_out - compression level of resulting image in range 0-100.
 * quality      - output quality
 * RETURN VALUE
 * returns newly created and encoded ASImage on success, NULL of failure.
 * DESCRIPTION
 * enlarges ASImage, padding it with specified color on each side in
 * accordance with requested geometry.
 *********/
/****f* libAfterImage/transform/blur_asimage_gauss()
 * SYNOPSIS
 * ASImage* blur_asimage_gauss( ASVisual* asv, ASImage* src,
 *                              double horz, double vert,
 *                              ASAltImFormats out_format,
 *                              unsigned int compression_out, int quality );
 * INPUTS
 * asv          - pointer to valid ASVisual structure
 * src          - source ASImage
 * horz         - horizontal radius of the blurr
 * vert         - vertical radius of the blurr
 * out_format 	- optionally describes alternative ASImage format that
 *                should be produced as the result - XImage, ARGB32, etc.
 * compression_out - compression level of resulting image in range 0-100.
 * quality      - output quality
 * RETURN VALUE
 * returns newly created and encoded ASImage on success, NULL of failure.
 * DESCRIPTION
 * Performs Gaussian blurr of the image ( usefull fro drop shadows and
 * the likes ).
 *********/
/****f* libAfterImage/transform/fill_asimage()
 * SYNOPSIS
 * Bool fill_asimage( ASVisual *asv, ASImage *im,
 *                    int x, int y, int width, int height,
 *                    ARGB32 color );
 * INPUTS
 * asv           - pointer to valid ASVisual structure
 * im            - ASImage to fill with the color
 * x, y          - left-top corner of the rectangle to fill.
 * width, height - size of the rectangle to fill.
 * color         - ARGB32 color value to fill rectangle with.
 * RETURN VALUE
 * True on success, False on failure.
 * DESCRIPTION
 * Fills rectangle within the existing ASImage with specified color.
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
ASImage *mirror_asimage( ASVisual *asv, ASImage *src,
				         int offset_x, int offset_y,
						 unsigned int to_width,
			             unsigned int to_height,
			             Bool vertical, ASAltImFormats out_format,
						 unsigned int compression_out, int quality );
ASImage *pad_asimage(   ASVisual *asv, ASImage *src,
		      			int dst_x, int dst_y,
			  			unsigned int to_width,
			  			unsigned int to_height,
			  			ARGB32 color,
			  			ASAltImFormats out_format,
			  			unsigned int compression_out, int quality );
ASImage* blur_asimage_gauss( ASVisual* asv, ASImage* src,
	                         double horz, double vert,
                             ASAltImFormats out_format,
							 unsigned int compression_out, int quality);

Bool fill_asimage( ASVisual *asv, ASImage *im,
               	   int x, int y, int width, int height,
				   ARGB32 color );


#endif
