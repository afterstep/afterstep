#include "config.h"

#include <string.h>


/****h* libAfterImage/tutorials/ASMerge
 * SYNOPSIS
 * Scaling and blending of arbitrary number of images 
 * using libAfterImage.
 * DESCRIPTION
 * We will attempt to interpret command line arguments as sequence of
 * image filenames, geometries and blending types. We'll then try and
 * load all the images, scaling first one to requested size, and 
 * blending others at specifyed locations of the first image.
 * We then display the result in simple window.
 * After that we would want to wait, until user closes our window.
 *
 * New steps described in this tutorial are :
 * ASMerge.1. 
 * ASScale.2. Merging ASImage.
 * SEE ALSO
 * Tutorial 1: ASView  - explanation of basic steps needed to use
 *                       libAfterImage and some other simple things.
 * Tutorial 2: ASScale - image scaling basics.
 * Tutorial 3: ASTile  - image tiling and tinting
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
	int dummy, to_width, to_height, geom_flags = 0;
	ASImageLayer *layers ;
	int layers_num, good_layers_num = 0, i;


	/* see ASView.1 : */
	set_application_name( argv[0] );

	if( argc <= 3 )
	{
		show_warning( "not enough arguments.\n Usage: astile <image1[:geom1]> <op1> <image2[:geom2]> [<op2> <image3[:geom3]> ...]");
		return 1;
	}		
	
	layers_num = argc/2 ;
	layers = calloc( 1, sizeof(ASImageLayer) );
	
	for( i = 0 ; i < layers_num ; i++ )
	{
		merge_scanlines_func func = NULL ;

		if( i > 0 )
		{
			if((func = blend_scanlines_name2func( argv[i*2] )) == NULL ) 
				continue ;
			
		}
	}
	
	{
		image_file = argv[1] ;
		if( argc > 2 )   /* see ASScale.1 : */
			geom_flags = XParseGeometry( argv[2], &dummy, &dummy,
			                             &to_width, &to_height );
	}else
		show_warning( "no image file or tint color specified - defaults used: \"%s\" ",
		              image_file );

    dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( dpy, screen );
	/* see ASView.2 : */
	im = file2ASImage( image_file, 0xFFFFFFFF, SCREEN_GAMMA, 0, NULL );

	/* Making sure tiling geometry is sane : */
	if( !get_flags(geom_flags, WidthValue ) )
		to_width = im->width*2 ;
	if( !get_flags(geom_flags, HeightValue ) )
		to_height = im->height*2;
	printf( "%s: scaling image \"%s\" to %dx%d\n",
		    get_application_name(), image_file, to_width, to_height );

	if( im != NULL )
	{
		/* see ASView.3 : */
		asv = create_asvisual( dpy, screen, depth, NULL );
		/* see ASView.4 : */
		w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
			                         to_width, to_height, 1, 0, NULL,
									 "ASScale" );
		if( w != None )
		{
			Pixmap p ;
			ASImage *scaled_im ;

			XSelectInput (dpy, w, (StructureNotifyMask | ButtonPress));
	  		XMapRaised   (dpy, w);
			/* see ASScale.2 : */
			scaled_im = scale_asimage( asv, im, to_width, to_height,
				                       True, 0, ASIMAGE_QUALITY_DEFAULT );
			destroy_asimage( &im );
			/* see ASView.5 : */
			p = asimage2pixmap( asv, DefaultRootWindow(dpy), scaled_im,
				                NULL, True );
			destroy_asimage( &scaled_im );
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
	}
    if( dpy )
        XCloseDisplay (dpy);
    return 0 ;
}
/**************/

/****f* libAfterImage/tutorials/ASMerge.1 [4.1]
 * SYNOPSIS
 * Step 1.
 * DESCRIPTION
 * EXAMPLE
 ********/
/****f* libAfterImage/tutorials/ASMerge.2 [4.2]
 * SYNOPSIS
 * Step 2. Actual blending of the set of images.
 * DESCRIPTION
 * EXAMPLE
 * NOTES
 * SEE ALSO
 * merge_asimage().
 ********/
