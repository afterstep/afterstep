#include "config.h"

#include <string.h>


/****h* libAfterImage/tutorials/ASScale
 * SYNOPSIS
 * Simple program based on libAfterImage to scale an image.
 * DESCRIPTION
 * We will load image and scale it up to new size, specified as command
 * line arguments
 * We then display the result in simple window.
 * After that we would want to wait, until user closes our window.
 *
 * In this tutorial we will only explain new steps, not described in
 * previous tutorial. New steps described in this tutorial are :
 * ASScale.1. Parsing geometry spec.
 * ASScale.2. Scaling ASImage.
 * SEE ALSO
 * Tutorial 1: ASView - explanation of basic steps needed to use
 *                      libAfterImage and some other simple things.
 * SOURCE
 */

#include <libAfterImage/afterbase.h>
#include <libAfterImage/afterimage.h>
#include "common.h"

void usage()
{
	printf( "Usage: asscale [-h]|[image [WIDTH[xHEIGHT]]]\n");
	printf( "Where: image - is image filename.\n");
	printf( "       WIDTH - width to scale image to.( Naturally :)\n");
	printf( "       HEIGHT- height to scale image to.\n");
}

int main(int argc, char* argv[])
{
	char *image_file = "rose512.jpg" ;
	int dummy, geom_flags = 0;
	unsigned int to_width, to_height ;
	ASImage *im ;

	/* see ASView.1 : */
	set_application_name( argv[0] );

	if( argc > 1 )
	{
		if( strncmp( argv[1], "-h", 2 ) == 0 )
		{
			usage();
			return 0;
		}
		image_file = argv[1] ;
		if( argc > 2 )   /* see ASScale.1 : */
			geom_flags = XParseGeometry( argv[2], &dummy, &dummy,
			                             &to_width, &to_height );
	}else
	{
		show_warning( "no image file or scale geometry - defaults used: \"%s\" ",
		              image_file );
		usage();
	}
	/* see ASView.2 : */
	im = file2ASImage( image_file, 0xFFFFFFFF, SCREEN_GAMMA, 0, NULL );

	if( im != NULL )
	{
		ASVisual *asv ;
		ASImage *scaled_im ;
		/* Making sure tiling geometry is sane : */
		if( !get_flags(geom_flags, WidthValue ) )
			to_width = im->width*2 ;
		if( !get_flags(geom_flags, HeightValue ) )
			to_height = im->height*2;
		printf( "%s: scaling image \"%s\" to %dx%d by factor of %fx%f\n",
			    get_application_name(), image_file, to_width, to_height,
				(double)to_width/(double)(im->width),
				(double)to_height/(double)(im->height) );

#ifndef X_DISPLAY_MISSING
		{
			Window w ;
			int screen, depth ;

		    dpy = XOpenDisplay(NULL);
			_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
			screen = DefaultScreen(dpy);
			depth = DefaultDepth( dpy, screen );
			/* see ASView.3 : */
			asv = create_asvisual( dpy, screen, depth, NULL );
			/* see ASView.4 : */
			w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
				                         to_width, to_height, 1, 0, NULL,
										 "ASScale" );
			if( w != None )
			{
				Pixmap p ;

		  		XMapRaised   (dpy, w);
				/* see ASScale.2 : */
				scaled_im = scale_asimage( asv, im, to_width, to_height,
					                       ASA_XImage, 0, ASIMAGE_QUALITY_DEFAULT );
				destroy_asimage( &im );
				/* see ASView.5 : */
				p = asimage2pixmap( asv, DefaultRootWindow(dpy), scaled_im,
					                NULL, True );
				destroy_asimage( &scaled_im );
				/* see common.c: set_window_background_and_free() : */
				p = set_window_background_and_free( w, p );
			}
			/* see common.c: wait_closedown() : */
			wait_closedown(w);
		}
#else
		asv = create_asvisual( NULL, 0, 0, NULL );
		scaled_im = scale_asimage( asv, im, to_width, to_height,
			                       ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );
		/* writing result into the file */
		ASImage2file( scaled_im, NULL, "asscale.jpg", ASIT_Jpeg, NULL );
		destroy_asimage( &scaled_im );
		destroy_asimage( &im );
#endif
	}
    return 0 ;
}
/**************/

/****f* libAfterImage/tutorials/ASScale.1 [2.1]
 * SYNOPSIS
 * Step 1. Parsing the geometry.
 * DESCRIPTION
 * Geometry can be specified in WIDTHxHEIGHT+X+Y format. Accordingly we
 * use standard X function to parse it: XParseGeometry. Returned flags
 * tell us what values has been specified. Since we only need size -
 * we check if it is specified and if not - simply default it to twice
 * as big as original image. Accordingly we use dummy variable to pass
 * to XParseGeometry.
 * EXAMPLE
 *     geom_flags = XParseGeometry( argv[3], &dummy, &dummy,
 *                                  &to_width, &to_height );
 ********/
/****f* libAfterImage/tutorials/ASScale.2 [2.2]
 * SYNOPSIS
 * Step 2. Actual scaling the image.
 * DESCRIPTION
 * scale_asimage() scales image both up and down, and is very easy to
 * use - just pass it new size. In this example we use default quality.
 * Default is equivalent to GOOD which should be sufficient in
 * most cases. Compression is set to 0 since we do not intend to store
 * image for long time. Even better - we don't need to store it at all -
 * all we need is XImage, so we can transfer it to the server easily.
 * That is why to_xim argument is set to ASA_XImage. As the result obtained
 * ASImage will not have any data in its buffers, but it will have
 * ximage member set to point to valid XImage. Subsequently we enjoy
 * that convenience, by setting use_cached to True in call to
 * asimage2pixmap. That ought to save us alot of processing.
 *
 * Scaling algorithm is rather sophisticated and is implementing 4 point
 * interpolation. Which basically means that we try to approximate each
 * missing point as an extension of the trend of 4 neighboring points -
 * two on each side. Closest neighbor's have more weight then outside
 * ones. 2D scaling is performed by scaling each scanline first, and
 * then interpolating missing scanlines.
 * Scaling down is somewhat skimpier, as it amounts to simple averaging
 * of the multiple pixels. All calculations are done in integer math on
 * per channel basis, and with precision 24.8 bits per channel per pixel.
 *
 * EXAMPLE
 *     scaled_im = scale_asimage( asv, im, to_width, to_height,
 * 	                              ASA_XImage, 0, ASIMAGE_QUALITY_DEFAULT );
 *     destroy_asimage( &im );
 * NOTES
 * Scaling image up to very large height is much slower then to same
 * width due to algorithm specifics. Yet even on inferior hardware it
 * yields decent speeds.
 * When we successfully scaled image - we no longer need the original -
 * getting rid of it so it does not clog memory.
 * SEE ALSO
 * scale_asimage().
 ********/
