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

#include <libAfterImage/afterbase.h>
#include <libAfterImage/afterimage.h>
#include "common.h"

#if 1
CARD16 chan_alpha[] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
CARD16 chan_red  [] = {0x0000, 0x7000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0x7000, 0xffff};
CARD16 chan_green[] = {0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xffff, 0x0000, 0x0000, 0xffff};
CARD16 chan_blue [] = {0x0000, 0x7000, 0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff};
#else
CARD16 chan_alpha[] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
CARD16 chan_red[] =   {0x0000, 0x0000, 0x0000, 0x0000, 0x8888, 0xffff, 0xffff, 0xffff, 0xff00};
CARD16 chan_green[]=  {0x0000, 0x0000, 0x8888, 0x8888, 0x0000, 0x0000, 0x8888, 0x8888, 0xff00};
CARD16 chan_blue[] =  {0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xffff, 0x0000, 0xff00};
#endif

double points[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8} ;


void usage()
{
}

int main(int argc, char* argv[])
{
	ASVisual *asv ;
	int screen = 0, depth = 0;
	int dummy, geom_flags = 0;
	unsigned int to_width= 200, to_height = 200;
	ASVectorPalette  palette = { 9, &(points[0]), {&chan_blue[0], &chan_green[0], &chan_red[0], &chan_alpha[0]}, ARGB32_Black} ;
	double vector[200*200] ;
	ASImage *vect_im = NULL;
	int x, y ;

	/* see ASView.1 : */
	set_application_name( argv[0] );
#if (HAVE_AFTERBASE_FLAG==1)
	set_output_threshold(OUTPUT_LEVEL_DEBUG);
#endif

	for( y = 200 ; y > 0 ; --y ) 
		for( x = 1 ; x <= 200 ; ++x ) 
		  vector[(200-y)*200+x-1] = (y-1)/40+(x-1)/40 ;
#ifndef X_DISPLAY_MISSING
    dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( dpy, screen );
#endif

	/* see ASView.3 : */
	asv = create_asvisual( dpy, screen, depth, NULL );
	/* see ASGrad.2 : */
	vect_im = create_asimage_from_vector( asv, &vector[0],
							to_width, to_height,
							&palette,
#ifndef X_DISPLAY_MISSING
							 ASA_XImage,
#else
							 ASA_ASImage,
#endif
							0, ASIMAGE_QUALITY_POOR );

	if( vect_im )
	{
#ifndef X_DISPLAY_MISSING
		/* see ASView.4 : */
		Window w = create_top_level_window( asv,
		                                    DefaultRootWindow(dpy), 32, 32,
		                        			to_width, to_height, 1, 0, NULL,
											"ASGradient" );
		if( w != None )
		{
			Pixmap p ;

		  	XMapRaised   (dpy, w);
			/* see ASView.5 : */
			p = asimage2pixmap( asv, DefaultRootWindow(dpy), vect_im,
					            NULL, True );
			destroy_asimage( &vect_im );
			/* see common.c: set_window_background_and_free() : */
			p = set_window_background_and_free( w, p );
			/* see common.c: wait_closedown() : */
			wait_closedown(w);
		}
	    if( dpy )
  		    XCloseDisplay (dpy);
#else
		ASImage2file( vect_im, NULL, "asvector.jpg", ASIT_Jpeg, NULL );
		destroy_asimage( &vect_im );
#endif
	}
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
