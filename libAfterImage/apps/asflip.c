#include "config.h"

#include <string.h>
#include <stdlib.h>

/****h* libAfterImage/tutorials/ASFlip
 * NAME
 * Tutorial 6: Image flipping.
 * SYNOPSIS
 * libAfterImage application for image rotation.
 * DESCRIPTION
 * New steps described in this tutorial are :
 * ASFlip.1. Flip value.
 * ASFlip.2. Rotating ASImage.
 * SEE ALSO
 * Tutorial 1: ASView  - explanation of basic steps needed to use
 *                       libAfterImage and some other simple things.
 * Tutorial 2: ASScale - image scaling basics.
 * Tutorial 3: ASTile  - image tiling and tinting.
 * Tutorial 4: ASMerge - scaling and blending of arbitrary number of
 *                       images.
 * Tutorial 5: ASGradient - drawing multipoint linear gradients.
 * SOURCE
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>

#include "afterimage.h"
#include "common.h"

int main(int argc, char* argv[])
{
	Window w ;
	ASVisual *asv ;
	int screen, depth ;
	char *image_file = "test.xpm" ;
	int flip = FLIP_VERTICAL;
	int tile_x, tile_y, tile_width, tile_height, geom_flags = 0;
	ASImage *im ;

	/* see ASView.1 : */
	set_application_name( argv[0] );

	if( argc > 1 )
	{
		image_file = argv[1] ;
		if( argc > 2 )
		{
			/* see ASFlip.1 */
			flip = atoi(argv[2])/90 ;
			if( argc > 3 )
			{
	    		/* see ASTile.1 : */
	    		geom_flags = XParseGeometry( argv[3], &tile_x, &tile_y,
		        		                     &tile_width, &tile_height );
			}
		}
	}

    dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( dpy, screen );

	/* see ASView.2 : */
	im = file2ASImage( image_file, 0xFFFFFFFF, SCREEN_GAMMA, 0, NULL );

	/* Making sure tiling geometry is sane : */
	if( !get_flags(geom_flags, XValue ) )
		tile_x = 0 ;
	if( !get_flags(geom_flags, YValue ) )
		tile_y = 0 ;
	if( !get_flags(geom_flags, WidthValue ) )
		tile_width = (get_flags(flip,FLIP_VERTICAL))?im->height:im->width ;
	if( !get_flags(geom_flags, HeightValue ) )
		tile_height = (get_flags(flip,FLIP_VERTICAL))?im->width:im->height;
	printf( "%s: tiling image \"%s\" to %dx%d%+d%+d and then flipping it by %d degrees\n",
		    get_application_name(), image_file,
			tile_width, tile_height,tile_x, tile_y, flip*90 );

	/* see ASView.3 : */
	asv = create_asvisual( dpy, screen, depth, NULL );
	/* see ASView.4 : */
	w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
		                         tile_width, tile_height, 1, 0, NULL,
								 "ASFlip" );
	if( w != None )
	{
		Pixmap p ;
		ASImage *flipped_im ;

		XSelectInput (dpy, w, (StructureNotifyMask | ButtonPress));
	  	XMapRaised   (dpy, w);
		/* see ASFlip.2 : */
		flipped_im = flip_asimage( 	asv, im,
			                       	tile_x, tile_y,
									tile_width, tile_height,
				       	 			flip,
				                	True, 0, ASIMAGE_QUALITY_DEFAULT );
		destroy_asimage( &im );
		/* see ASView.5 : */
		p = asimage2pixmap( asv, DefaultRootWindow(dpy), flipped_im,
				            NULL, True );
		destroy_asimage( &flipped_im );
		/* see common.c: set_window_background_and_free() : */
		p = set_window_background_and_free( w, p );
	}
	/* see ASView.6 : */
	while(w != None)
  	{
    	XEvent event ;
	    XNextEvent (dpy, &event);
  		switch(event.type)
		{
		  	case ButtonPress:
				break ;
	  		case ClientMessage:
			    if ((event.xclient.format == 32) &&
	  			    (event.xclient.data.l[0] == _XA_WM_DELETE_WINDOW))
		  		{
					XDestroyWindow( dpy, w );
					XFlush( dpy );
					w = None ;
				}
				break;
		}
  	}
    if( dpy )
        XCloseDisplay (dpy);
    return 0 ;
}
/**************/
/****f* libAfterImage/tutorials/ASFlip.1 [6.1]
 * SYNOPSIS
 * Step 1. Flip value.
 * DESCRIPTION
 * libAfterImage provides facility for rotating images in 90 degree
 * increments - flipping essentially. Accordingly flip parameter could
 * have 4 values - 0, -90, -180, -270 degrees.
 * EXAMPLE
 *     flip = atoi(argv[2])/90;
 * SEE ALSO
 * flip
 ********/
/****f* libAfterImage/tutorials/ASFlip.2 [6.2]
 * SYNOPSIS
 * Step 2. Flipping ASImage.
 * DESCRIPTION
 * Flipping can actually be combined with offset and tiling. Original
 * image gets tiled to suplied rectangle, and then gets rotated to
 * requested degree.
 * EXAMPLE
 *  	flipped_im = flip_asimage( 	asv, im,
 * 			                       	tile_x, tile_y,
 * 									tile_width, tile_height,
 * 				       	 			flip,
 * 				                	True, 0, ASIMAGE_QUALITY_DEFAULT );
 * 		destroy_asimage( &im );
 * NOTES
 * As far as we need to render rotated image right away - we set to_xim
 * parameter to True, so that image will be rotated into XImage. Right
 * after rotation is done - we can destroy original image.
 * SEE ALSO
 * flip_asimage()
 ********/
