#include "config.h"

#include <string.h>

/****h* libAfterImage/tutorials/ASView
 * SYNOPSIS
 * Simple image viewer based on libAfterImage.
 * DESCRIPTION
 * All we want to do here is to get image filename from the command line,
 * then load this image, and display it in simple window.
 * After that we would want to wait, until user closes our window.
 * SOURCE
 */

#include <libAfterImage/afterbase.h>
#include <libAfterImage/afterimage.h>
#include "common.h"

void usage()
{
	printf( "Usage: asview [-h]|[image]\n");
	printf( "Where: image - filename of the image to display.\n");
}


int main(int argc, char* argv[])
{
	char *image_file = "rose512.jpg" ;
	ASImage *im ;
	ARGB32 test_colors[] = {
		0xff0000,

		0xfe0100,	0xbe3f00,	0x807e00,

		0x7f7f00,

		0x7e8000, 	0x3fbe00,	0x01fe00,

		0x00ff00,

		0x00fe01,   0x00be3f,   0x00807e,

		0x007f7f,

		0x007e80,  	0x003fbe,	0x0001fe,

		0x0000ff,

		0x0100fe,   0x3F00be,	0x7e0080,

		0x7f007f,

		0x80007e,   0xbe003f,	0xfe0001,

		0xff0000,

		0x0
	};
	int i ;
	/* see ASView.1 : */
	set_application_name( argv[0] );
#if (HAVE_AFTERBASE_FLAG==1)
	set_output_threshold(OUTPUT_LEVEL_DEBUG);
#endif

	for ( i = 0 ; test_colors[i] != 0 ; ++i )
	{
		CARD32 hue16 = rgb2hue( ARGB32_RED16(test_colors[i]),
					            ARGB32_GREEN16(test_colors[i]),
					   		    ARGB32_BLUE16(test_colors[i]));
		fprintf( stderr, "%d:rgb = #%2.2lX%2.2lX%2.2lX, hue8 = %ld, hue16 = %ld\n", i,
			             ARGB32_RED8(test_colors[i]),
						 ARGB32_GREEN8(test_colors[i]),
						 ARGB32_BLUE8(test_colors[i]),
						 hue16>>8,hue16);
	}

	if( argc > 1 )
	{
		if( strcmp( argv[1], "-h" ) == 0 )
		{
			usage();
			return 0;
		}
		image_file = argv[1] ;
	}else
	{
		show_warning( "Image filename was not specified. Using default: \"%s\"", image_file );
		usage();
	}
	/* see ASView.2 : */
	im = file2ASImage( image_file, 0xFFFFFFFF, SCREEN_GAMMA, 0, NULL );

	if( im != NULL )
	{
#ifndef X_DISPLAY_MISSING
		Window w ;
		ASVisual *asv ;
		int screen, depth ;

	    dpy = XOpenDisplay(NULL);
		_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
		screen = DefaultScreen(dpy);
		depth = DefaultDepth( dpy, screen );
		/* see ASView.3 : */
		asv = create_asvisual( dpy, screen, depth, NULL );
		/* see ASView.4 : */
		w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
			                         im->width, im->height, 1, 0, NULL,
									 "ASView" );
		if( w != None )
		{
			Pixmap p ;

	  		XMapRaised   (dpy, w);
			/* see ASView.5 : */
			p = asimage2pixmap( asv, DefaultRootWindow(dpy), im, NULL,
				                False );
			destroy_asimage( &im );
			/* see common.c:set_window_background_and_free(): */
			p = set_window_background_and_free( w, p );
		}
		/* see common.c: wait_closedown() : */
		wait_closedown(w);
#else
		/* writing result into the file */
		ASImage2file( im, NULL, "asview.jpg", ASIT_Jpeg, NULL );
#endif
	}

    return 0 ;
}
/**************/

/****f* libAfterImage/tutorials/ASView.1 [1.1]
 * SYNOPSIS
 * Step 1. Initialization.
 * DESCRIPTION
 * libAfterImage requires only 2 global things to be setup, and both of
 * those are inherited from libAfterBase: dpy - pointer to open X display -
 * naturally that is something we cannot live without; application name -
 * used in all the text output, such as error and warning messages and
 * also debugging messages if such are enabled.
 * The following two line are about all that is required to setup both
 * of this global variables :
 * EXAMPLE
 *     set_application_name( argv[0] );
 *     dpy = XOpenDisplay(NULL);
 * NOTES
 * First line is setting up application name from command line's
 * program name. Second opens up X display specified in DISPLAY env.
 * variable. Naturally based on application purpose different parameters
 * can be passed to these functions, such as some custom display string.
 * SEE ALSO
 * libAfterBase, set_application_name(), XOpenDisplay(), Display,
 *******/
/****f* libAfterImage/tutorials/ASView.2 [1.2]
 * SYNOPSIS
 * Step 2. Loading image file.
 * DESCRIPTION
 * At this point we are ready to load image from file into memory. Since
 * libAfterImage does not use any X facilities to store image - we don't
 * have to create any window or anything else yet. Even dpy is optional
 * here - it will only be used to try and parse names of colors from
 * .XPM images.
 * EXAMPLE
 *     im = file2ASImage( image_file, 0xFFFFFFFF, SCREEN_GAMMA, 0, NULL);
 * NOTES
 * We used compression set to 0, as we do not intend to store
 * image in memory for any considerable amount of time, and we want to
 * avoid additional processing overhead related to image compression.
 * If image was loaded successfully, which is indicated by returned
 * pointer being not NULL, we can proceed to creation of the window and
 * displaying of the image.
 * SEE ALSO
 * file2ASImage()
 ********/
/****f* libAfterImage/tutorials/ASView.3 [1.3]
 * SYNOPSIS
 * Step 3. Preparation of the visual.
 * DESCRIPTION
 * At this point we have to obtain Visual information, as window
 * creation is highly dependant on Visual being used. In fact when X
 * creates a window it ties it to a particular Visual, and all its
 * attributes, such as colormap, pixel values, pixmaps, etc. must be
 * associated with the same Visual. Accordingly we need to acquire
 * ASVisual structure, which is our abstraction layer from them naughty
 * X Visuals. :
 * EXAMPLE
 *     asv = create_asvisual( dpy, screen, depth, NULL );
 * NOTES
 * If any Window or Pixmap is created based on particular ASVisual, then
 * this ASVisual structure must not be destroyed untill all such
 * Windows and Pixmaps are destroyed.
 * SEE ALSO
 * See create_asvisual() for details.
 ********/
/****f* libAfterImage/tutorials/ASView.4 [1.4]
 * SYNOPSIS
 * Step 4. Preparation of the window.
 * DESCRIPTION
 * Creation of top level window consists of several steps of its own:
 * a) create the window of desired size and placement
 * b) set ICCCM hints on the window
 * c) select appropriate events on the window
 * c) map the window.
 * First two steps has been moved out into create_top_level_window()
 * function.
 * EXAMPLE
 *     w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
 *                                  im->width, im->height, 1, 0, NULL,
 *                                  "ASView" );
 *     if( w != None )
 *     {
 *         XSelectInput (dpy, w, (StructureNotifyMask | ButtonPress));
 *         XMapRaised   (dpy, w);
 *     }
 * NOTES
 * Map request should be made only for window that has all its hints set
 * up already, so that Window Manager can read them right away.
 * We want to map window as soon as possible so that User could see that
 * something really is going on, even before image is displayed.
 * SEE ALSO
 * ASImage, create_top_level_window()
 ********/
/****f* libAfterImage/tutorials/ASView.5 [1.5]
 * SYNOPSIS
 * Step 5. Displaying the image.
 * DESCRIPTION
 * The simplest way to display image in the window is to convert it
 * into Pixmap, then set Window's background to this Pixmap, and,
 * at last, clear the window, so that background shows up.
 * EXAMPLE
 *     p = asimage2pixmap( asv, DefaultRootWindow(dpy), im, NULL, False );
 *     destroy_asimage( &im );
 * NOTES
 * We no longer need ASImage after we transfered it onto the Pixmap, so
 * we better destroy it to conserve resources.
 * SEE ALSO
 * asimage2pixmap(), destroy_asimage(), set_window_background_and_free()
 ********/
