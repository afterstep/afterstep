#include "config.h"

#include <string.h>
#include <time.h>


/****h* libAfterImage/tutorials/ASTile
 * SYNOPSIS
 * Simple program based on libAfterImage to tile and tint image.
 * DESCRIPTION
 * All we want to do here is to get image filename, tint color and
 * desired geometry from the command line. We then load this image, and
 * proceed on to tiling it based on parameters. Tiling geometry
 * specifies rectangular shape on limitless plane on which original
 * image is tiled. While we are at tiling the image we also tint it to
 * specified color, or to some random value derived from the current
 * time in seconds elapsed since 1971.
 * We then display the result in simple window.
 * After that we would want to wait, until user closes our window.
 *
 * In this tutorial we will only explain new steps, not described in
 * previous tutorial. New steps described in this tutorial are :
 * ASTile.1. Parsing ARGB32 tinting color.
 * ASTile.2. Parsing geometry spec.
 * ASTile.3. Tiling and tinting ASImage.
 * SEE ALSO
 * ASView - explanation of basic steps needed to use libAfterImage and
 *          some other simple things.
 * SOURCE
 */

#include <libAfterImage/afterbase.h>
#include <libAfterImage/afterimage.h>
#include "common.h"

void usage()
{
	printf( "Usage: astile [-h]|[[-g geometry][-t tint_color] image]\n");
	printf( "Where: image    - source image filename.\n");
	printf( "       geometry - width and height of the resulting image,\n");
	printf( "                  and x, y of the origin of the tiling on source image.\n");
	printf( "       tint_color - color to tint image with.( defaults to current time :)\n");
}

int main(int argc, char* argv[])
{
	Window w ;
	ASVisual *asv ;
	int screen, depth ;
	char *image_file = "rose512.jpg" ;
	ARGB32 tint_color = time(NULL);
	int tile_x, tile_y, tile_width, tile_height, geom_flags = 0;
	ASImage *im ;

	/* see ASView.1 : */
	set_application_name( argv[0] );

	/* parse_argb_color can only be useda after display is open : */
    dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( dpy, screen );

	if( argc > 1 )
	{
		int i ;

		if( strncmp( argv[1], "-h", 2 ) == 0 )
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
					case 't' :			/* see ASTile.1 : */
						if( parse_argb_color( argv[i+1], &tint_color ) ==
							argv[i+1] )
							show_warning( "unable to parse tint color - default used: #%8.8X",
				            			  tint_color );
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
	{
		show_warning( "no image file or tint color specified - defaults used: \"%s\" #%8.8lX",
		              image_file, tint_color );
		usage();
	}

	/* see ASView.2 : */
	im = file2ASImage( image_file, 0xFFFFFFFF, SCREEN_GAMMA, 0, NULL );

	/* Making sure tiling geometry is sane : */
	if( !get_flags(geom_flags, XValue ) )
		tile_x = im->width/2 ;
	if( !get_flags(geom_flags, YValue ) )
		tile_y = im->height/2 ;
	if( !get_flags(geom_flags, WidthValue ) )
		tile_width = im->width*2 ;
	if( !get_flags(geom_flags, HeightValue ) )
		tile_height = im->height*2;
	printf( "%s: tiling image \"%s\" to %dx%d%+d%+d tinting with #%8.8lX\n",
		    get_application_name(), image_file, tile_width, tile_height,
			tile_x, tile_y, tint_color );

	if( im != NULL )
	{
		/* see ASView.3 : */
		asv = create_asvisual( dpy, screen, depth, NULL );
		/* see ASView.4 : */
		w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
			                         tile_width, tile_height, 1, 0, NULL,
									 "ASTile" );
		if( w != None )
		{
			Pixmap p ;
			ASImage *tinted_im ;

			XSelectInput (dpy, w, (StructureNotifyMask | ButtonPress));
	  		XMapRaised   (dpy, w);
			/* see ASTile.3 : */
			tinted_im = tile_asimage( asv, im, tile_x, tile_y,
				                      tile_width, tile_height,
				                      tint_color, ASA_XImage, 0,
									  ASIMAGE_QUALITY_TOP );
			destroy_asimage( &im );
			/* see ASView.5 : */
			p = asimage2pixmap( asv, DefaultRootWindow(dpy), tinted_im,
				                NULL, True );
			destroy_asimage( &tinted_im );
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

/****f* libAfterImage/tutorials/ASTile.1 [3.1]
 * SYNOPSIS
 * Step 1. Color parsing.
 * DESCRIPTION
 * libAfterImage utilizes function provided by libAfterBase for color
 * parsing. In case libAfterBase is unavailable - libAfterImage
 * includes its own copy of that function. This function differs from
 * standard XParseColor in a way that it allows for parsing of alpha
 * channel in addition to red, green and blue. It autodetects if value
 * include alpha channel or not, using the following logic:
 * If number of hex digits in color spec is divisible by 4 and is not
 * equal to 12 then first digits are treated as alpha channel.
 * In case named color is specified or now apha channel is specified
 * alpha value of 0xFF will be used, marking this color as solid.
 * EXAMPLE
 *     if( parse_argb_color( argv[i+1], &tint_color ) == argv[i+1] )
 * 	       show_warning( "unable to parse tint color - default used: #%8.8X",
 *                        tint_color );
 * NOTES
 * On success parse_argb_color returns pointer to the character
 * immidiately following color specification in original string.
 * Therefore test for returned value to be equal to original string will
 * can be used to detect error.
 * SEE ALSO
 * libAfterBase, parse_argb_color(), ARGB32
 *******/
/****f* libAfterImage/tutorials/ASTile.2 [3.2]
 * SYNOPSIS
 * Step 2. Parsing the geometry.
 * DESCRIPTION
 * Geometry can be specified in WIDTHxHEIGHT+X+Y format. Accordingly we
 * use standard X function to parse it: XParseGeometry. Returned flags
 * tell us what values has been specified. We only have to fill the rest
 * with some sensible defaults. Default x is width/2, y is height/2, and
 * default size is same as image's width.
 * EXAMPLE
 *     geom_flags = XParseGeometry ( argv[i+1], &tile_x, &tile_y,
 *                                            &tile_width, &tile_height );
 * SEE ALSO
 * ASScale.1
 ********/
/****f* libAfterImage/tutorials/ASTile.3 [3.3]
 * SYNOPSIS
 * Step 3. Actuall tiling of the image.
 * DESCRIPTION
 * Actuall tiling is quite simple - just call tile_asimage and it will
 * generate new ASImage containing tiled and tinted image. For the sake
 * of example we set quality to TOP, but normally GOOD quality is quite
 * sufficient, and is a default. Again, compression is set to 0 since we
 * do not intend to store image for long time. Even better we don't need
 * to store it at all - all we need is XImage, so we can transfer it to
 * the server easily. That is why to_xim argument is set to ASA_XImage.
 * As the result obtained ASImage will not have any data in its buffers,
 * but it will have ximage member set to point to valid XImage.
 * Subsequently we enjoy that convinience, by setting use_cached to True
 * in call to asimage2pixmap(). That ought to save us alot of processing.
 *
 * Tinting works in both directions - it can increase intensity of the
 * color or decrease it. If any particular channel of the tint_color is
 * greater then 127 then intensity is increased, otherwise its decreased.
 * EXAMPLE
 *     tinted_im = tile_asimage( asv, im, tile_x, tile_y,
 *                               tile_width, tile_height,
 *                               tint_color,
 *                               ASA_XImage, 0, ASIMAGE_QUALITY_TOP );
 *     destroy_asimage( &im );
 * NOTES
 * SEE ALSO
 * tile_asimage().
 ********/
