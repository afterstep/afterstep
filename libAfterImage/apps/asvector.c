#include "config.h"

#include <string.h>
#include <stdlib.h>


/****h* libAfterImage/tutorials/ASVector
 * NAME
 * ASVector
 * SYNOPSIS
 * libAfterImage application for drawing image representing array of 
 * floating point values ( scientiofic data ).
 * DESCRIPTION
 * SEE ALSO
 * Tutorial 1: ASView  - explanation of basic steps needed to use
 *                       libAfterImage and some other simple things.
 * Tutorial 2: ASScale - image scaling basics.
 * Tutorial 3: ASTile  - image tiling and tinting.
 * Tutorial 4: ASMerge - scaling and blending of arbitrary number of
 *                       images.
 * SOURCE
 */

#include "../afterbase.h"
#include "../afterimage.h"
#include "common.h"

#if 1
CARD16 chan_alpha[] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
								0xffff, 0xffff, 0xffff, 0xffff};
CARD16 chan_red  [] = {0x0000, 0x7000, 0x0000, 0x0000, 0x0000, 
								0xffff, 0xffff, 0x7000, 0xffff};
CARD16 chan_green[] = {0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
								0xffff, 0x0000, 0x0000, 0xffff};
CARD16 chan_blue [] = {0x0000, 0x7000, 0xffff, 0xffff, 0x0000, 
								0x0000, 0x0000, 0x0000, 0xffff};
#else
CARD16 chan_alpha[] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
								0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
CARD16 chan_red[] =   {0x0000, 0x0000, 0x0000, 0x0000, 0x8888, 
								0xffff, 0xffff, 0xffff, 0xff00};
CARD16 chan_green[]=  {0x0000, 0x0000, 0x8888, 0x8888, 0x0000, 
								0x0000, 0x8888, 0x8888, 0xff00};
CARD16 chan_blue[] =  {0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 
								0x0000, 0xffff, 0x0000, 0xff00};
#endif

double points[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8} ;


void usage()
{
}

int main(int argc, char* argv[])
{
	ASVisual *asv ;
	int screen = 0, depth = 0;
	unsigned int to_width= 200, to_height = 200;
	ASVectorPalette  palette = { 9, &(points[0]), 
								{&chan_blue[0], &chan_green[0], 
								 &chan_red[0], &chan_alpha[0]}, 
								 ARGB32_Black} ;
	double vector[5 * 5] ;
/*	double vector[200*200] ; */
	ASImage *vect_im = NULL;
	ASImage *temp_im = NULL;
	int x, y ;

	/* see ASView.1 : */
	set_application_name( argv[0] );
#if (HAVE_AFTERBASE_FLAG==1)
	set_output_threshold(OUTPUT_LEVEL_DEBUG);
#endif

	for( y = 5 ; y > 0 ; --y ) 
		for( x = 1 ; x <= 5 ; ++x ) 
		  vector[(5-y)*5+x-1] = (y-1)+(x-1);
#ifndef X_DISPLAY_MISSING
    dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( dpy, screen );
#endif

	/* see ASView.3 : */
	asv = create_asvisual( dpy, screen, depth, NULL );
	/* see ASGrad.2 : */
	temp_im = create_asimage_from_vector( asv, &vector[0],
							5, 5,
/*						to_width, to_height,  */
							&palette,
#ifndef X_DISPLAY_MISSING
							 ASA_XImage,
#else
							 ASA_ASImage,
#endif
							0, ASIMAGE_QUALITY_POOR );


#if 0
    vect_im = temp_im ;
#else
	vect_im = scale_asimage(asv, temp_im, to_width, to_height,
                            ASA_ASImage, 100, ASIMAGE_QUALITY_POOR);
#endif
	if( vect_im )
	{
#ifndef X_DISPLAY_MISSING
		/* see ASView.4 : */
		Window w = create_top_level_window( asv,
		                                    DefaultRootWindow(dpy), 
											32, 32,
		                        			to_width, to_height, 1, 0, 
											NULL,
											"ASVector", NULL );
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
		{
			CARD8 * buffer ; 
			int size ;		   
	  		if( ASImage2PNGBuff( vect_im, &buffer, &size, NULL ) ) 
			{
				FILE * pf = fopen("asvector.png", "w" ) ; 
				if( pf ) 
				{
					fwrite( buffer, size, 1, pf );
					fclose(pf);
				}	 
			}else
				show_error( "failed to encode image as PNG"); 	 
		}
		ASImage2file( vect_im, NULL, "asvector_copy.png", ASIT_Png, NULL );
		destroy_asimage( &vect_im );
#endif
	}
    return 0 ;
}
/**************/

