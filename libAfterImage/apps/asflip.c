#include "config.h"

#include <string.h>
#include <stdlib.h>

/****h* libAfterImage/tutorials/ASFlip
 * NAME
 * ASFlip
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
 * Tutorial 5: ASGrad  - drawing multipoint linear gradients.
 * SOURCE
 */

#include "../afterbase.h"
#include "../afterimage.h"
#include "common.h"

void usage()
{
	printf( "Usage: asflip [-h]|[[-f flip]|[-m vertical] "
			"[-g geom] image]");
	printf( "\nWhere: image - is image filename\n");
	printf( "       flip  - rotation angle in degrees. "
			"90, 180 and 270 degrees supported\n");
	printf( "       geom  - source image is tiled using this geometry, "
			"prior to rotation\n");
	printf( "       vertical - 1 - mirror image in vertical direction, "
			"0 - horizontal\n");
}

int main(int argc, char* argv[])
{
	Display *dpy = NULL;
	ASVisual *asv ;
	int screen = 0 , depth = 0 ;
	char *image_file = "rose512.jpg" ;
	int flip = FLIP_VERTICAL;
	Bool vertical = False, mirror = False ;
	int tile_x, tile_y, geom_flags = 0;
	unsigned int tile_width, tile_height ;
	ASImage *im = NULL;
	ASImage *flipped_im = NULL ;

	/* see ASView.1 : */
	set_application_name( argv[0] );

	if( argc > 1 )
	{
		int i = 1 ;
		if( strcmp( argv[1], "-h" ) == 0 )
		{
			usage();
			return 0;
		}
		for( i = 1 ; i < argc ; i++ )
		{
			if( argv[i][0] == '-' && i < argc-1 )
			{
				switch(argv[i][1])
				{
					case 'm' :
						mirror = True;
						vertical = atoi(argv[i+1]) ;
					    break ;
					case 'f' :			/* see ASFlip.1 */
						mirror = False;
						flip = atoi(argv[i+1])/90 ;
					    break ;
					case 'g' :   		/* see ASTile.2 : */
	    				geom_flags = XParseGeometry( argv[i+1],
							                         &tile_x, &tile_y,
		        				        			 &tile_width,
													 &tile_height );
					    break ;
				}
				++i ;
			}else
				image_file = argv[i] ;
		}
	}else
		usage();

#ifndef X_DISPLAY_MISSING
    dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( dpy, screen );
#endif
	/* see ASView.2 : */
	im = file2ASImage( image_file, 0xFFFFFFFF, SCREEN_GAMMA, 0, getenv("IMAGE_PATH"), NULL );
	if( im == NULL )
		return 1;

	/* Making sure tiling geometry is sane : */
	if( !get_flags(geom_flags, XValue ) )
		tile_x = 0 ;
	if( !get_flags(geom_flags, YValue ) )
		tile_y = 0 ;
	if( !get_flags(geom_flags, WidthValue ) )
	{
		if( !mirror )
			tile_width = (get_flags(flip,FLIP_VERTICAL))?
						 	im->height:im->width ;
		else
			tile_width = im->width ;
	}
	if( !get_flags(geom_flags, HeightValue ) )
	{
		if( !mirror )
			tile_height = (get_flags(flip,FLIP_VERTICAL))?
							im->width:im->height;
		else
			tile_height = im->height ;
	}
	printf( "%s: tiling image \"%s\" to %dx%d%+d%+d and then "
			"flipping it by %d degrees\n",
		    get_application_name(), image_file,
			tile_width, tile_height,tile_x, tile_y, flip*90 );

	/* see ASView.3 : */
	asv = create_asvisual( dpy, screen, depth, NULL );

	/* see ASFlip.2 : */
	if( !mirror )
		flipped_im = flip_asimage( 	asv, im,
			                       	tile_x, tile_y,
									tile_width, tile_height,
				       	 			flip,
				                	ASA_ASImage, 0, 
									ASIMAGE_QUALITY_DEFAULT );
	else
		flipped_im = mirror_asimage(asv, im,
			                       	tile_x, tile_y,
									tile_width, tile_height,
				       	 			vertical,
				                	ASA_ASImage, 0, 
									ASIMAGE_QUALITY_DEFAULT );
	destroy_asimage( &im );

	if( flipped_im )
	{
#ifndef X_DISPLAY_MISSING
		/* see ASView.4 : */
		Window w = create_top_level_window( asv, DefaultRootWindow(dpy), 
											32, 32,
			  		      	                tile_width, tile_height, 
											1, 0, NULL,
											"ASFlip", image_file );
		if( w != None )
		{
			Pixmap p ;

		  	XMapRaised   (dpy, w);
			/* see ASView.5 : */
			p = asimage2pixmap( asv, DefaultRootWindow(dpy), flipped_im,
					            NULL, True );
			destroy_asimage( &flipped_im );
			/* see common.c: set_window_background_and_free() : */
			p = set_window_background_and_free( w, p );
			/* see common.c: wait_closedown() : */
		}
		wait_closedown(w);
		dpy = NULL;
#else
		/* writing result into the file */
		ASImage2file( flipped_im, NULL, "asflip.jpg", ASIT_Jpeg, NULL );
		destroy_asimage( &flipped_im );
#endif
	}
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
 * image gets tiled to supplied rectangle, and then gets rotated to
 * requested degree.
 * EXAMPLE
 *  	flipped_im = flip_asimage(asv, im,
 * 			                      tile_x, tile_y,
 * 								  tile_width, tile_height,
 * 				       	 		  flip,
 * 				                  ASA_XImage,0, 
 * 								  ASIMAGE_QUALITY_DEFAULT );
 * 		destroy_asimage( &im );
 * NOTES
 * As far as we need to render rotated image right away - we set to_xim
 * parameter to True, so that image will be rotated into XImage. Right
 * after rotation is done - we can destroy original image.
 * SEE ALSO
 * flip_asimage()
 ********/
