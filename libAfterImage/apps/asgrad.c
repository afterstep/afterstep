#include "config.h"

#include <string.h>
#include <stdlib.h>


/****h* libAfterImage/tutorials/ASGrad
 * NAME
 * Tutorial 5: Gradients.
 * SYNOPSIS
 * libAfterImage application for drawing multipoint linear gradients.
 * DESCRIPTION
 * New steps described in this tutorial are :
 * ASGrad.1. Building gradient specs.
 * ASGrad.2. Actual rendering gradient.
 * SEE ALSO
 * Tutorial 1: ASView  - explanation of basic steps needed to use
 *                       libAfterImage and some other simple things.
 * Tutorial 2: ASScale - image scaling basics.
 * Tutorial 3: ASTile  - image tiling and tinting.
 * Tutorial 4: ASMerge - scaling and blending of arbitrary number of
 *                       images.
 * SOURCE
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>

#include "afterimage.h"
#include "common.h"

ARGB32 default_colors[] = {
	0xFF000000,
	0xFF700070,                                /* violet */
	0xFF0000FF,                                /* blue   */
	0xFF00FFFF,                                /* cyan   */
	0xFF00FF00,
	0XFFFFFF00,
	0XFFFF0000,
	0XFF700000,
	0xFF8080A0,
	0xFFE0E0FF,
};
double default_offsets[] = { 0, 0.1, 0.15, 0.20, 0.35, 0.45, 0.50, 0.55, 0.65, 0.8, 1.0} ;


void usage()
{
	printf( "  Usage: asgrad -h | <geometry> <gradient_type> <color1> <offset2> <color2> [ <offset3> <color3> ...]\n");
	printf( "  Where: geometry - size of the resulting image and window;\n");
	printf( "         gradient_type - One of the fiollowing values :\n");
	printf( "            0 - linear   left-to-right gradient,\n");
	printf( "            1 - diagonal lefttop-to-rightbottom,\n");
	printf( "            2 - linear   top-to-bottom gradient,\n");
	printf( "            3 - diagonal righttop-to-leftbottom;\n");
	printf( "         offset   - floating point value from 0.0 to 1.0\n");
}

int main(int argc, char* argv[])
{
	Window w ;
	ASVisual *asv ;
	int screen, depth ;
	int dummy, to_width, to_height, geom_flags = 0;
	ASGradient grad ;
	ASGradient default_grad = { 1, 10, &(default_colors[0]), &(default_offsets[0])} ;

	/* see ASView.1 : */
	set_application_name( argv[0] );

	if( argc > 1 )
	{
	    if( strcmp( argv[1], "-h") == 0 )
	    {
			usage();
			return 0;
		}
	    /* see ASScale.1 : */
	    geom_flags = XParseGeometry( argv[1], &dummy, &dummy,
		                             &to_width, &to_height );
	}else
		usage();
	memset( &grad, 0x00, sizeof(ASGradient));

    dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( dpy, screen );

	if( argc >= 5 )
	{
		int i = 2;
		/* see ASGrad.1 : */
		grad.type = atoi( argv[2] );
		grad.npoints = 0 ;
		grad.color = safemalloc( ((argc-2)/2)*sizeof(ARGB32));
		grad.offset = safemalloc( ((argc-2)/2)*sizeof(double));
		while( ++i < argc )
		{
			if( grad.npoints > 0 )
			{
				if( i == argc-1 )
					grad.offset[grad.npoints] = 1.0;
				else
					grad.offset[grad.npoints] = atof( argv[i] );
				++i ;
			}

			/* see ASTile.1 : */
			if( parse_argb_color( argv[i], &(grad.color[grad.npoints])) != argv[i] )
				if(grad.offset[grad.npoints] >= 0. && grad.offset[grad.npoints]<= 1.0 )
					grad.npoints++ ;
		}
	}else
	{
		grad = default_grad ;
		if( argc >= 3 )
			grad.type = atoi( argv[2] );
	}

	if( grad.npoints <= 0 )
	{
		show_error( " not enough gradient points specified.");
		return 1;
	}

	/* Making sure tiling geometry is sane : */
	if( !get_flags(geom_flags, WidthValue ) )
		to_width  = DisplayWidth(dpy, screen)*2/3 ;
	if( !get_flags(geom_flags, HeightValue ) )
		to_height = DisplayHeight(dpy, screen)*2/3 ;
	printf( "%s: rendering gradient of type %d to %dx%d\n",
		    get_application_name(), grad.type&GRADIENT_TYPE_MASK, to_width, to_height );

	/* see ASView.3 : */
	asv = create_asvisual( dpy, screen, depth, NULL );
	/* see ASView.4 : */
	w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
		                         to_width, to_height, 1, 0, NULL,
								 "ASGradient" );
	if( w != None )
	{
		Pixmap p ;
		ASImage *grad_im ;

		XSelectInput (dpy, w, (StructureNotifyMask | ButtonPress));
	  	XMapRaised   (dpy, w);
		/* see ASGrad.2 : */
		grad_im = make_gradient( asv, &grad, to_width, to_height,
			        	            SCL_DO_ALL,
				                    ASA_XImage, 0, ASIMAGE_QUALITY_DEFAULT );
		/* see ASView.5 : */
		p = asimage2pixmap( asv, DefaultRootWindow(dpy), grad_im,
				            NULL, True );
		destroy_asimage( &grad_im );
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

/****f* libAfterImage/tutorials/ASGrad.1 [5.1]
 * SYNOPSIS
 * Step 1. Building gradient specs.
 * DESCRIPTION
 * Multipoint gradient is defined as set of color values with offsets
 * of each point from the beginning of the gradient on 1.0 scale.
 * Offsets of the first and last point in gradient should always be
 * 0. and 1.0 respectively, and other points should go in between.
 * For example 2 point gradient will have always offsets 0. and 1.0,
 * 3 points gradient will have 0. for first color, 1.0 for last color
 * and anything in between for middle color.
 * If offset is incorrect - point will be skipped at the time of
 * rendering.
 *
 * There are 4 types of gradients supported : horizontal, top-left to
 * bottom-right diagonal, vertical and top-right to bottom-left diagonal.
 * Any cilindrical gradient could be drawn as a 3 point gradient with
 * border colors being the same.
 *
 * Each gradient point has ARGB color, which means that it is possible
 * to draw gradients in alpha channel as well as RGB. That makes for
 * semitransparent gradients, fading gradients, etc.
 * EXAMPLE
 *  	grad.type = atoi( argv[2] );
 * 		grad.npoints = 0 ;
 * 		grad.color = safemalloc( ((argc-2)/2)*sizeof(ARGB32));
 * 		grad.offset = safemalloc( ((argc-2)/2)*sizeof(double));
 * 		while( ++i < argc )
 * 		{
 * 			if( grad.npoints > 0 )
 * 			{
 * 				if( i == argc-1 )
 * 					grad.offset[grad.npoints] = 1.0;
 * 				else
 * 					grad.offset[grad.npoints] = atof( argv[i] );
 * 				++i ;
 * 			}
 *  		if( parse_argb_color( argv[i], &(grad.color[grad.npoints]))
 *              != argv[i] )
 * 				if(grad.offset[grad.npoints] >= 0. &&
 *                 grad.offset[grad.npoints]<= 1.0 )
 * 					grad.npoints++ ;
 * 		}
 * SEE ALSO
 * ARGB32, parse_argb_color(), ASGradient
 ********/
/****f* libAfterImage/tutorials/ASGrad.2 [5.2]
 * SYNOPSIS
 * Step 2. Actually rendering gradient.
 * DESCRIPTION
 * All that is needed to draw gradient is to call make_gradient(),
 * passing pointer to ASGradient structure, that describes gradient.
 * Naturally size of the gradient is needed too. Another parameter is
 * filter - that is a bit mask that allows to draw gradient using only a
 * subset of the channels, represented by set bits. SCL_DO_ALL means
 * that all 4 channels must be rendered.
 * make_gradient() creates ASImage of requested size and fills it with
 * gradient. Special techinque based on error diffusion is utilized to
 * avoid sharp steps between grades of colors when limited range of
 * colors is used for gradient.
 * EXAMPLE
 * 		grad_im = make_gradient( asv, &grad, to_width, to_height,
 * 		        	             SCL_DO_ALL,
 *  		                     ASA_XImage, 0, ASIMAGE_QUALITY_DEFAULT );
 * NOTES
 * make_gradient(), ASScanline, ASImage.
 ********/
