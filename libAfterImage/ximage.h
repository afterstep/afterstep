#ifndef LIBAFTERIMAGE_XIMAGE_HEADER_FILE_INCLUDED
#define LIBAFTERIMAGE_XIMAGE_HEADER_FILE_INCLUDED

#include "asvisual.h"
#include "blender.h"
#include "asimage.h"

/****h* libAfterImage/ximage.h
 * SYNOPSIS
 * Defines conversion to and from XImages and Pixmaps.
 * DESCRIPTION
 * ximage2asimage()	- convert XImage structure into ASImage
 * pixmap2asimage()	- convert X11 pixmap into ASImage
 * asimage2ximage()	- convert ASImage into XImage
 * asimage2mask_ximage() - convert alpha channel of ASImage into XImage
 * asimage2pixmap()	- convert ASImage into Pixmap ( possibly using
 * 					  precreated XImage )
 * asimage2mask() 	- convert alpha channel of ASImage into 1 bit
 * 				  	  mask Pixmap.
 * SEE ALSO
 * Other libAfterImage modules :
 *          ascmap.h asfont.h asimage.h asvisual.h blender.h export.h
 *          import.h transform.h ximage.h
 * AUTHOR
 * Sasha Vasko <sasha at aftercode dot net>
 ******
 */

/****f* libAfterImage/ximage/ximage2asimage()
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
/****f* libAfterImage/ximage/pixmap2asimage()
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

/****f* libAfterImage/ximage/asimage2ximage()
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
/****f* libAfterImage/ximage/asimage2mask_ximage()
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
/****f* libAfterImage/ximage/asimage2pixmap()
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
/****f* libAfterImage/ximage/asimage2mask()
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


#endif
