#include "config.h"

#include <string.h>
#include <stdlib.h>

/****h* libAfterImage/tutorials/ASText
 * NAME
 * Tutorial 7: Text rendering.
 * SYNOPSIS
 * libAfterImage application for rendering text.
 * DESCRIPTION
 * New steps described in this tutorial are :
 * ASText.1. .
 * ASText.2. .
 * SEE ALSO
 * Tutorial 1: ASView  - explanation of basic steps needed to use
 *                       libAfterImage and some other simple things.
 * Tutorial 2: ASScale - image scaling basics.
 * Tutorial 3: ASTile  - image tiling and tinting.
 * Tutorial 4: ASMerge - scaling and blending of arbitrary number of
 *                       images.
 * Tutorial 5: ASGradient - drawing multipoint linear gradients.
 * Tutorial 6: ASFlip  - image rotation.
 * SOURCE
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>

#include "afterimage.h"
#include "common.h"

/* Usage:  astext <font[:size]> [<text> [<text_color>
 *                [<foreground_image> [<background_image>]]]]
 */

int main(int argc, char* argv[])
{
	Window w ;
	ASVisual *asv ;
	int screen, depth ;
	char *font_name = "fixed";
	int size = 12 ;
	char *text = "Smart Brown Dog jumps Over The Lazy Fox, and falls into the ditch.";
	ARGB32 text_color = ARGB32_WHITE;
	char *fore_image_file = NULL ;
	char *back_image_file = "test.xpm" ;
	ASFontManager *fontman = NULL;
	ASFont  *font = NULL;
	ASImage *fore_im = NULL, *back_im = NULL;

	/* see ASView.1 : */
	set_application_name( argv[0] );

	if( argc > 1 )
		font_name = argv[1] ;
	if( argc > 2 )
		text = argv[2] ;
	if( argc > 3 )
		parse_argb_color( argv[3], &text_color );
	if( argc > 4 )
		fore_image_file = argv[4] ;
	if( argc > 5 )
		back_image_file = argv[5] ;

    dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( dpy, screen );

	/* see ASView.2 : */
	im = file2ASImage( image_file, 0xFFFFFFFF, SCREEN_GAMMA, 0, NULL );

	/* see ASView.3 : */
	asv = create_asvisual( dpy, screen, depth, NULL );
	/* see ASView.4 : */
	w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
		                         tile_width, tile_height, 1, 0, NULL,
								 "ASText" );
	if( w != None )
	{
		Pixmap p ;
		ASImage *flipped_im ;

		XSelectInput (dpy, w, (StructureNotifyMask | ButtonPress));
	  	XMapRaised   (dpy, w);
		/* see ASText.3 : */
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
/****f* libAfterImage/tutorials/ASText.1 [7.1]
 * SYNOPSIS
 * Step 1. .
 * DESCRIPTION
 * EXAMPLE
 * SEE ALSO
 ********/
/****f* libAfterImage/tutorials/ASText.2 [7.2]
 * SYNOPSIS
 * Step 2. .
 * DESCRIPTION
 * EXAMPLE
 * NOTES
 * SEE ALSO
 ********/
