#include "config.h"

#include <string.h>
#include <stdlib.h>

/****h* libAfterImage/tutorials/ASText
 * NAME
 * Tutorial 7: Text rendering.
 * SYNOPSIS
 * libAfterImage application for rendering texturized text.
 * DESCRIPTION
 * In this tutorial we will attempt to render arbitrary text in window,
 * with optional texturized background and foreground. We shall also
 * surround rendered text with beveled frame, creating an illusion of a
 * button.
 *
 * New steps described in this tutorial are :
 * ASText.1. Openning and closing fonts.
 * ASText.2. Approximating rendered text size.
 * ASText.3. Rendering texturized text.
 * ASText.4. Merging foreground and background with bevel.
 * SEE ALSO
 * Tutorial 1: ASView  - explanation of basic steps needed to use
 *                       libAfterImage and some other simple things.
 * Tutorial 2: ASScale - image scaling basics.
 * Tutorial 3: ASTile  - image tiling and tinting.
 * Tutorial 4: ASMerge - scaling and blending of arbitrary number of
 *                       images.
 * Tutorial 5: ASGrad  - drawing multipoint linear gradients.
 * Tutorial 6: ASFlip  - image rotation.
 * SOURCE
 */

#include "../afterbase.h"
#include "../afterimage.h"
#include "common.h"

/* Usage:  astext [-f font] [-s size] [-t text] [-S 3D_style]
                  [-c text_color] [-b background_color]
                  [-T foreground_texture] [-B background_image]
				  [-r foreground_resize_type] [-R background_resize_type]
 */

#define BEVEL_HI_WIDTH 20
#define BEVEL_LO_WIDTH 30
#define BEVEL_ADDON    (BEVEL_HI_WIDTH+BEVEL_LO_WIDTH)

void usage()
{
	printf( "  Usage:   astext [-h] [-f font] [-s size] [-t text] [-S 3D_style] \n");
	printf( "                  [-c text_color] [-b background_color]\n");
	printf( "                  [-T foreground_texture] [-B background_image]\n");
	printf( "      			   [-r foreground_resize_type] [-R background_resize_type]\n");
	printf( "      			   [-m ]\n");
	printf( "  Where: font - TrueType font's filename or X font spec or alias;\n");
	printf( "         size - size in points for TrueType fonts;\n");
	printf( "         text - text to be drawn;\n");
	printf( "         3D_style - 3D style of text. One of the following:\n");
	printf( "             0 - plain 2D tetx, 1 - embossed, 2 - sunken, 3 - shade above,\n");
	printf( "             4 - shade below, 5 - embossed thick 6 - sunken thick.\n");
	printf( "         resize_type - tells how texture/image should be transformed to fit\n");
	printf( "                       the text size. Could be: scale or tile. Default is tile\n");
	printf( "         -m make font monospaced. \n");


}

int main(int argc, char* argv[])
{
	ASVisual *asv = NULL ;
	int screen = 0, depth = 0;
	char *font_name = "test.ttf";
	int size = 32 ;
#if 0	
	char *text ="CVS\t\t     command.c\tiobuff.c  rxvt.h.notmine  thai.c\n"
				"Makefile\t     command.o\tiobuff.o  scrbuff.c\t  thai.h\n"
				"Makefile.in\t     events.c\tlog	  scrbuff.h	  thai.o\n"
				"afterstep.h	     events.o	main.c	  scrbuff.o	  unicode\n"
				"aterm.exe	     feature.h	main.o	  screen.c	  utmp.c\n";
#else
 	char *text = "Smart Brown Dog jumps\nOver The Lazy Fox,\nand falls into the ditch.";
#endif
	ARGB32 text_color = ARGB32_White, back_color = ARGB32_Black;
	char *text_color_name = "#FFFFFFFF", *back_color_name = "#FF000000";
	char *fore_image_file = "fore.xpm", *back_image_file = "back.xpm" ;
	Bool scale_fore_image = False, scale_back_image = False ;
	ASImage *fore_im = NULL, *back_im = NULL;
	ASImage *text_im = NULL ;
	struct ASFontManager *fontman = NULL;
	struct ASFont  *font = NULL;
	unsigned int width, height ;
	int i ;
	int text_margin = size/2 ;
	Bool monospaced = False ;
	ASTextAttributes attr = {ASTA_VERSION_1, ASTA_UseTabStops, AST_ShadeBelow, ASCT_Char, 8, 0, NULL, 0 };
	
	/* see ASView.1 : */
	set_application_name( argv[0] );
#if (HAVE_AFTERBASE_FLAG==1)
	set_output_threshold(OUTPUT_LEVEL_DEBUG);
#endif

	if( argc == 1 )
		usage();
	else for( i = 1 ; i < argc ; i++ )
	{
		if( strncmp( argv[i], "-h", 2 ) == 0 )
		{
			usage();
			return 0;
		}
		if( strncmp( argv[i], "-m", 2 ) == 0 )
		{
	   		monospaced = True ;
		}else if( i+1 < argc )
		{
			if( strncmp( argv[i], "-f", 2 ) == 0 )
				font_name = argv[i+1] ;
			else if( strncmp( argv[i], "-s", 2 ) == 0 )
			{
				size = atoi(argv[i+1]);
				text_margin = size/2 ;
			}else if( strncmp( argv[i], "-t", 2 ) == 0 )
				text = argv[i+1] ;
			else if( strncmp( argv[i], "-S", 2 ) == 0 )
			{
				attr.type = atoi(argv[i+1]);
				if( attr.type >= AST_3DTypes )
				{
					show_error( "3D type is wrong. Using 2D Plain instead.");
					attr.type = AST_Plain ;
				}

			}else if( strncmp( argv[i], "-c", 2 ) == 0 )
				text_color_name = argv[i+1] ;
			else if( strncmp( argv[i], "-b", 2 ) == 0 )
				back_color_name = argv[i+1] ;
			else if( strncmp( argv[i], "-T", 2 ) == 0 )
				fore_image_file = argv[i+1] ;
			else if( strncmp( argv[i], "-B", 2 ) == 0 )
				back_image_file = argv[i+1] ;
			else if( strncmp( argv[i], "-r", 2 ) == 0 )
				scale_fore_image = (strcmp( argv[i+1], "scale") == 0);
			else if( strncmp( argv[i], "-R", 2 ) == 0 )
				scale_back_image = (strcmp( argv[i+1], "scale") == 0);
		}
	}


#ifndef X_DISPLAY_MISSING
    dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( dpy, screen );
#endif

	/* see ASText.1 : */
	if( (fontman = create_font_manager( dpy, NULL, NULL )) != NULL )
		font = get_asfont( fontman, font_name, 0, size, ASF_GuessWho|(monospaced?ASF_Monospaced:0) );

	if( font == NULL )
	{
		show_error( "unable to load requested font \"%s\". Falling back to \"fixed\".", font_name );
		font = get_asfont( fontman, "fixed", 0, size, ASF_GuessWho );
		if( font == NULL )
		{
			show_error( "font \"fixed\" is not available either. Aborting.");
			return 1;
		}
	}

	parse_argb_color( text_color_name, &text_color );
	parse_argb_color( back_color_name, &back_color );

	/* see ASView.3 : */
	asv = create_asvisual( dpy, screen, depth, NULL );

	/* see ASText.2 : */
	/*set_asfont_glyph_spacing( font, 10, 40 );*/
    /* Simple way : get_text_size( text, font, attr.type, &width, &height ); */
	/* Fancy way : */
	get_fancy_text_size( text, font, &attr, &width, &height, 0, NULL );
	if( fore_image_file )
	{
		ASImage *tmp = file2ASImage( fore_image_file, 0xFFFFFFFF,
		               		         SCREEN_GAMMA, 0, NULL );
		if( tmp )
		{
			if( tmp->width != width || tmp->height != height )
			{   /* see ASScale.2 : */
				if( scale_fore_image )
					fore_im = scale_asimage( asv, tmp, width, height,
  					                         ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );
				else
					fore_im = tile_asimage( asv, tmp, 0, 0, width, height, 0,
  					                         ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );
				destroy_asimage( &tmp );
			}else
				fore_im = tmp ;
		}else
			fore_im = NULL ;
	}
	width  += text_margin*2 + BEVEL_ADDON;
	height += text_margin*2 + BEVEL_ADDON;
	if( back_image_file )
	{ /* see ASView.2 : */
		ASImage *tmp = file2ASImage( back_image_file, 0xFFFFFFFF,
			                        SCREEN_GAMMA, 0, NULL );
		if( tmp )
		{
			if( scale_back_image && (tmp->width != width || tmp->height != height) )
			{   /* see ASScale.2 : */
				back_im = scale_asimage( asv, tmp, width, height,
				                         ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );
				destroy_asimage( &tmp );
			}else
				back_im = tmp ;
		}else
			back_im = NULL ;
	}

	/* see ASText.3 : */
    /* simple way : text_im = draw_text( text, font, attr.type, 0 ); */
	text_im = draw_fancy_text( text, font, &attr, 0, 0 );

/*
show_progress( "text_im->width = %d, text_im->height = %d", text_im->width, text_im->height );
show_progress( "fore_im->width = %d, fore_im->height = %d", fore_im->width, fore_im->height );
*/
	if( fore_im )
	{
		move_asimage_channel( fore_im, IC_ALPHA, text_im, IC_ALPHA );
		destroy_asimage( &text_im );
	}else
		fore_im = text_im ;

	/* see ASText.1 : */
	release_font( font );
	destroy_font_manager( fontman, False );

	if( fore_im )
	{
		ASImage *rendered_im ;
		ASImageLayer layers[2] ;
		ASImageBevel bevel = {0/*BEVEL_SOLID_INLINE*/, 0xFFDDDDDD, 0xFF555555, 0xFFFFFFFF, 0xFF777777, 0xFF222222,
		                      BEVEL_HI_WIDTH, BEVEL_HI_WIDTH,
							  BEVEL_LO_WIDTH, BEVEL_LO_WIDTH,
							  BEVEL_HI_WIDTH, BEVEL_HI_WIDTH,
							  BEVEL_LO_WIDTH, BEVEL_LO_WIDTH } ;

		/* see ASText.4 : */
		init_image_layers( &(layers[0]), 2 );
		if( back_im )
			back_im->back_color = back_color ;
		if( fore_im ) 
			fore_im->back_color = text_color ;
		layers[0].im = back_im ;
		layers[0].dst_x = 0 ;
		layers[0].dst_y = 0 ;
		layers[0].clip_width = width ;
		layers[0].clip_height = height ;
		layers[0].bevel = &bevel ;
		layers[1].im = fore_im ;
		layers[1].dst_x = text_margin+BEVEL_HI_WIDTH*2 ;
		layers[1].dst_y = text_margin+MIN((int)text_margin,((int)font->max_height-(int)font->max_ascend))/2+BEVEL_HI_WIDTH*2;
		if( fore_im ) 
		{	
			layers[1].clip_width = fore_im->width ;
			layers[1].clip_height = fore_im->height ;
		}
		rendered_im = merge_layers( asv, &(layers[0]), 2,
									width+BEVEL_ADDON, height+BEVEL_ADDON,
#ifndef X_DISPLAY_MISSING
									ASA_XImage,
#else
									ASA_ASImage,
#endif
									0, ASIMAGE_QUALITY_DEFAULT);
		destroy_asimage( &fore_im );
		destroy_asimage( &back_im );

		if( rendered_im )
		{
#ifndef X_DISPLAY_MISSING
			Window w;
			/* see ASView.4 : */
			w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
			      		                 width+BEVEL_ADDON, height+BEVEL_ADDON,
										 1, 0, NULL,
										 "ASText", text );
			if( w != None )
			{
				Pixmap p ;

			  	XMapRaised   (dpy, w);

				/* see ASView.5 : */
				p = asimage2pixmap( asv, DefaultRootWindow(dpy), rendered_im,
						            NULL, True );
				destroy_asimage( &rendered_im );
				/* see common.c: set_window_background_and_free() : */
				p = set_window_background_and_free( w, p );
				/* see common.c: wait_closedown() : */
				wait_closedown(w);
			}
		    if( dpy )
      			XCloseDisplay (dpy);
#else
			/* writing result into the file */
			ASImage2file( rendered_im, NULL, "astext.jpg", ASIT_Jpeg, NULL );
			destroy_asimage( &rendered_im );
#endif
		}
	}
    return 0 ;
}
/**************/
/****f* libAfterImage/tutorials/ASText.1 [7.1]
 * SYNOPSIS
 * Step 1. Openning and closing fonts.
 * DESCRIPTION
 * Before any text can be rendered using libAfterImage - desired font
 * has to be opened for use. Font opening process is two-step. At first
 * we need to create font manager ( ASFontManager ). That is done once,
 * and same font manger can be used throughout entire application. It
 * contains information about used external libraries, hash of opened
 * fonts, and some other info.
 *
 * When ASFontManager is created it could be used to obtain actuall fonts.
 * get_asfont() call is used to query cahce of the ASFontManager, to see
 * if the font has been loaded already, and if not - then it will load
 * the font and prepare it for drawing.
 *
 * EXAMPLE
 *     if( (fontman = create_font_manager( dpy, NULL, NULL )) != NULL )
 *         font = get_asfont( fontman, font_name, 0, size, ASF_GuessWho);
 *     if( font == NULL )
 *     {
 *         font = get_asfont( fontman, "fixed", 0, size, ASF_GuessWho );
 *         if( font == NULL )
 *         {
 *             show_error( "font \"fixed\" is not available either. Aborting.");
 *             return 1;
 *         }
 *     }
 *     ...
 *     destroy_font( font );
 *     destroy_font_manager( fontman, False );
 * SEE ALSO
 * create_font_manager(), get_asfont(), destroy_font(),
 * destroy_font_manager()
 ********/
/****f* libAfterImage/tutorials/ASText.2 [7.2]
 * SYNOPSIS
 * Step 2. Approximating rendered text size.
 * DESCRIPTION
 * Prior to actually drawing the text it is usefull to estimate the size
 * of the image, that rendered text will occupy, So that window can be
 * created of appropriate size, and othe interface elements laid out
 * accordingly. get_text_size() could be used to obtain rendered text
 * size without actually drawing it.
 * EXAMPLE
 *     get_text_size( text, font, type_3d, &width, &height );
 *     if( fore_image_file )
 *     {
 *         ASImage *tmp = file2ASImage( fore_image_file, 0xFFFFFFFF,
 *                                      SCREEN_GAMMA, 0, NULL );
 *         if( tmp )
 *         {
 *             if( tmp->width != width || tmp->height != height )
 *             {
 *                 if( scale_fore_image )
 *                     fore_im = scale_asimage( asv, tmp, width, height,
 *                                              ASA_ASImage, 0,
 *                                              ASIMAGE_QUALITY_DEFAULT );
 *                 else
 *                     fore_im = tile_asimage( asv, tmp, 0, 0,
 *                                             width, height, 0,
 *                                             ASA_ASImage, 0,
 *                                             ASIMAGE_QUALITY_DEFAULT );
 *                 destroy_asimage( &tmp );
 *             }else
 *                 fore_im = tmp ;
 *         }
 *     }
 * NOTES
 * In this particular example we either tile or scale foreground texture
 * to fit the text. In order to texturize the text  - we need to use
 * rendered text as an alpha channel on texture image. That can easily
 * be done only if both images are the same size.
 * SEE ALSO
 * get_text_size(), scale_asimage(), tile_asimage(), ASText.3
 ********/
/****f* libAfterImage/tutorials/ASText.3 [7.3]
 * SYNOPSIS
 * Step 3. Rendering texturized text.
 * DESCRIPTION
 * The most effective text texturization technique provided by
 * libAfterImage consists of substitution of the alpha channel of the
 * texture, with rendered text. That is possible since all the text is
 * rendered into alpha channel only. move_asimage_channel() call is used
 * to accomplish this operation. This call actually removes channel
 * data from the original image and stores it in destination image. If
 * there was something in destination image's channel  already - it will
 * be destroyed.
 * All kinds of nifty things could be done using this call, actually.
 * Like, for example, rendered text can be moved into green channel of
 * the texture, creating funky effect.
 * EXAMPLE
 *     text_im = draw_text( text, font, 0 );
 *     if( fore_im )
 *     {
 *         move_asimage_channel( fore_im, IC_ALPHA, text_im, IC_ALPHA );
 *         destroy_asimage( &text_im );
 *     }else
 *         fore_im = text_im ;
 * NOTES
 * move_asimage_channel() will only work if both images have exactly the
 * same size. It will do nothing otherwise.
 * SEE ALSO
 * draw_text(), move_asimage_channel()
 ********/
/****f* libAfterImage/tutorials/ASText.4 [7.4]
 * SYNOPSIS
 * Step 4. Merging foreground and background with bevel.
 * DESCRIPTION
 * On this step we have 2 images ready for us - background and texturized
 * text. At this point we need to merge them together by alpha-blending
 * text over background (remeber - text is alpha-channel of foreground
 * texture). At the same time we would like to add some nice bevel border
 * around entire image. To accomplish that task all we have to do is setup
 * ASImageLayer structure for both background and foreground, and apply
 * merge_layers on them. When it is done - we no longer need original
 * images and we destroy them to free up some memory.
 * EXAMPLE
 *     ASImageLayer layers[2] ;
 *     ASImageBevel bevel = {0, 0xFFDDDDDD, 0xFF555555,
 *                              0xFFFFFFFF, 0xFF777777, 0xFF444444,
 *                           BEVEL_HI_WIDTH, BEVEL_HI_WIDTH,
 *                           BEVEL_LO_WIDTH, BEVEL_LO_WIDTH,
 *                           BEVEL_HI_WIDTH, BEVEL_HI_WIDTH,
 *                           BEVEL_LO_WIDTH, BEVEL_LO_WIDTH } ;
 *     memset( &(layers[0]), 0x00, sizeof(layers) );
 *     layers[0].im = back_im ;
 *     layers[0].clip_width = width ;
 *     layers[0].clip_height = height ;
 *     layers[0].merge_scanlines = alphablend_scanlines ;
 *     layers[0].bevel = &bevel ;
 *     layers[1].im = fore_im ;
 *     layers[1].dst_x = TEXT_MARGIN ;
 *     layers[1].dst_y = TEXT_MARGIN ;
 *     layers[1].clip_width = fore_im->width ;
 *     layers[1].clip_height = fore_im->height ;
 *     layers[1].back_color = text_color ;
 *     layers[1].merge_scanlines = alphablend_scanlines ;
 *     rendered_im = merge_layers( asv, &(layers[0]), 2,
 *                                 width+BEVEL_ADDON, height+BEVEL_ADDON,
 *                                 ASA_XImage, 0, ASIMAGE_QUALITY_DEFAULT);
 *     destroy_asimage( &fore_im );
 *     destroy_asimage( &back_im );
 * NOTES
 * We have to remember that outside bevel border will addup to the image
 * size, hence we have to use width+BEVEL_ADDON , height+BEVEL_ADDON as
 * desired image size.
 * SEE ALSO
 * ASImageLayer, ASImageBevel, merge_layers()
 ********/

