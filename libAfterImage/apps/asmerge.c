#include "config.h"

#include <string.h>
#include <malloc.h>
#include <stdlib.h>


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
 * Step 1. Layers.
 * Step 2. Merging methods.
 * Step 3. Layer parameters.
 * Step 4. Actual blending of the set of images.
 * SEE ALSO
 * Tutorial 1: ASView  - explanation of basic steps needed to use
 *                       libAfterImage and some other simple things.
 * Tutorial 2: ASScale - image scaling basics.
 * Tutorial 3: ASTile  - image tiling and tinting
 * SOURCE
 */

#include <libAfterImage/afterbase.h>
#include <libAfterImage/afterimage.h>
#include "common.h"

char *burning_rose[] =
{
	"asmerge",
	"rose512.jpg",
	"add",
	"back.xpm:512x386",
	"hue",
	"fore.xpm:512x386"
};

void usage()
{
	printf( "Usage: asmerge [-h]|[image op1 image1 [op2 image2 [...]]]\n");
	printf( "Where: image  - is background image filename\n");
	printf( "       image1 - is first overlay's filename\n");
	printf( "       op1,op2,... - overlay operation. Supported operations are :\n");
	list_scanline_merging( stdout,
	        "         %-15.15s- %s\n");
}

int main(int argc, char* argv[])
{
	ASVisual *asv ;
	int screen = 0, depth = 0;
	int to_width = 1, to_height = 1;
	ASImageLayer *layers ;
	int layers_num = 0, i;
	ASImage *merged_im ;

	/* see ASView.1 : */
	set_application_name( argv[0] );
#if (HAVE_AFTERBASE_FLAG==1)
	set_output_threshold(OUTPUT_LEVEL_DEBUG);
#endif
	if( argc == 2 && strncmp(argv[1],"-h", 2) == 0 )
	{
		usage();
		return 0;
	}
	if( argc <= 3 )
	{
		show_error( "not enough arguments, please see usage:%s", " ");
		usage() ;
		printf( "Using the default, \"The Burning Rose\", composition :\n");
		printf( "\n\trose512.jpg add back.xpm:512x386 hue fore.xpm:512x386\n");
		argv = &(burning_rose[0]) ;
		argc = 6;
	}

#ifndef X_DISPLAY_MISSING
	dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( dpy, screen );
#endif
	/* see ASView.3 : */
	asv = create_asvisual( dpy, screen, depth, NULL );

	/* see ASMerge.1 : */
	layers = safecalloc( argc/2, sizeof(ASImageLayer) );

	for( i = 1 ; i < argc ; i++ )
	{
		int x = 0, y = 0;
		unsigned int width, height ;
		int geom_flags = 0 ;
		char *separator;
		char *filename ;
		/* see ASMerge.2 */
		if( i > 1 )
		{
			/* see blend_scanlines_name2func() : */
			if((layers[layers_num].merge_scanlines =
				 blend_scanlines_name2func( argv[i] )) == NULL )
				continue ;
			++i ;
		}
		if( (separator = strchr( argv[i], ':' )) != NULL )
		{   /* see ASTile.1 : */
			geom_flags = XParseGeometry( separator+1, &x, &y, &width, &height);
			filename = mystrndup( argv[i], separator-argv[i] );
		}else
			filename = argv[i] ;
		layers[layers_num].im = file2ASImage( filename, 0xFFFFFFFF,
			                                  SCREEN_GAMMA, 100, NULL );
		if( filename != argv[i] )
			free( filename );
		if( layers[layers_num].im != NULL )
		{
		 	if( !get_flags(geom_flags, WidthValue) )
		 		width = layers[layers_num].im->width  ;
		 	if( !get_flags(geom_flags, HeightValue) )
		 		height = layers[layers_num].im->height ;
			/* see ASMerge.3 : */
			if( layers[layers_num].merge_scanlines == NULL )
				layers[layers_num].merge_scanlines =
					alphablend_scanlines ;
			layers[layers_num].clip_width = width ;
			layers[layers_num].clip_height = height ;
			if( layers_num > 0 )
			{
				layers[layers_num].dst_x = x ;
				layers[layers_num].dst_y = y ;
			}else
			{
				to_width = width ;
				to_height = height ;
				if( width != layers[layers_num].im->width ||
				    height != layers[layers_num].im->height )
				{
					ASImage *scaled_bottom ;
					/* see ASScale.2 : */
					scaled_bottom = scale_asimage( asv, layers[layers_num].im,
											  	width, height, False, 100,
											  	ASIMAGE_QUALITY_DEFAULT );
					destroy_asimage( &(layers[layers_num].im) );
					layers[layers_num].im = scaled_bottom ;
				}
			}
			++layers_num ;
		}
	}

	if( layers_num <= 0 )
	{
		show_error( "there is no images to merge. Aborting");
		return 2;
	}

	/* see ASMerge.4 */
	merged_im = merge_layers( asv, layers, layers_num,
		                      to_width, to_height,
#ifndef X_DISPLAY_MISSING
							  ASA_XImage,
#else
							  ASA_ASImage,
#endif
							  0, ASIMAGE_QUALITY_DEFAULT );
	while( --layers_num >= 0 )
		destroy_asimage( &(layers[layers_num].im) );
	free( layers );

	if( merged_im )
	{
#ifndef X_DISPLAY_MISSING
	/* see ASView.4 : */
  		Window w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
					                         to_width, to_height, 1, 0, NULL,
											 "ASMerge" );
		if( w != None )
		{
			Pixmap p ;

		  	XMapRaised   (dpy, w);
			/* see ASView.5 : */
			p = asimage2pixmap( asv, DefaultRootWindow(dpy), merged_im,
			                NULL, True );

			destroy_asimage( &merged_im );
			/* see common.c: set_window_background_and_free() : */
			p = set_window_background_and_free( w, p );
			/* see common.c: wait_closedown() : */
			wait_closedown(w);
		}
		if( dpy )
  	  		XCloseDisplay (dpy);
#else
		/* writing result into the file */
		ASImage2file( merged_im, NULL, "asmerge.jpg", ASIT_Jpeg, NULL );
		destroy_asimage( &merged_im );
#endif
	}
#ifdef DEBUG_ALLOCS
	build_xpm_colormap(NULL);
	print_unfreed_mem();
#endif
	return 0 ;
}
/**************/

/****f* libAfterImage/tutorials/ASMerge.1 [4.1]
 * SYNOPSIS
 * Step 1. Layers.
 * DESCRIPTION
 * libAfterImage performs blending/merging of different images, using
 * arrays of ASImageLayer structures, with first element representing
 * the bottommost image. Each structure specifies exact position on
 * resulting image where overlay should be blended to, tint of the
 * overlay, size, to which overlay should be tiled, overlay's origin,
 * and overlay's background color. Arbitrary number of layers can be
 * merged whithin single run.
 *
 * Accordingly all that is needed to merge bunch of images is to create
 * array of ASImageLayer structures and fill it up appropriately.
 * EXAMPLE
 *     layers = safecalloc( argc/2, sizeof(ASImageLayer) );
 ********/
/****f* libAfterImage/tutorials/ASMerge.2 [4.2]
 * SYNOPSIS
 * Step 2. Merging methods.
 * DESCRIPTION
 * Each layer can be merged in using its own method. There are about 15
 * different methods implemented in libAfterImage, and user app can
 * implement other methods of its own. To specify method all that is
 * needed is to set merge_scanlines member of ASImageLayer to pointer
 * to the function, implementing specific method.
 *
 * libAfterImage provides facility to parse method name strings into
 * actuall function pointers. That could be used to simplifi scripting,
 * etc.
 * EXAMPLE
 * 	   if((layers[layers_num].merge_scanlines =
 * 	       blend_scanlines_name2func( argv[i] )) == NULL )
 * 		   continue ;
 * NOTE
 * All layers MUST have valid merge_scanlines pointer, even the
 * bottommost layer, despite the fact that it has nothing to be merged
 * with. If merge_scanlines is set to NULL - this layer will be ignored.
 * That could be used to turn on/off particular layers.
 ********/
/****f* libAfterImage/tutorials/ASMerge.3 [4.3]
 * SYNOPSIS
 * Step 3. Layer parameters.
 * DESCRIPTION
 * Several ASImageLayer members are mandatory and cannot be set to 0.
 * Such as : im - image to be merged; clip_width, clip_height - this will
 * be used to tile the image; merge_scanlines - must be set to a pointer
 * to the function implementing merging method. If any of this is set
 * to 0  - then layer will be ignored. The rest of the parameters are
 * optional. Note thou that tint parameter will tint overlay's RGB
 * components and alpha component, as the result it could be used to
 * make opaque images - semitransparent.
 * EXAMPLE
 *     if( layers[layers_num].merge_scanlines == NULL )
 *         layers[layers_num].merge_scanlines =
 *             alphablend_scanlines ;
 *     layers[layers_num].clip_width = width ;
 *     layers[layers_num].clip_height = height ;
 *     if( layers_num > 0 )
 *     {
 *         layers[layers_num].dst_x = x ;
 *         layers[layers_num].dst_y = y ;
 *     }
 ********/
/****f* libAfterImage/tutorials/ASMerge.4 [4.4]
 * SYNOPSIS
 * Step 4. Actual blending of the set of images.
 * DESCRIPTION
 * After set of layers has been prepared - it can be passed to
 * merge_layers() function, that will create new ASImage of specifyed
 * size, and then blend all the layers together to fill this image.
 * EXAMPLE
 * 		merged_im = merge_layers( asv, layers, layers_num,
 * 			                      to_width, to_height,
 * 			                      ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );
 * 		while( --layers_num >= 0 )
 * 			destroy_asimage( &(layers[layers_num].im) );
 * 		free( layers );
 * NOTES
 * After we've blended layers - we no longer need ASImageLayer array.
 * So proceeding to clean it up, by destroying overlay AsImages first,
 * and then freeing array itself.
 * SEE ALSO
 * merge_asimage().
 ********/
