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

#define TEXT_MARGIN 10
#define BEVEL_HI_WIDTH 3
#define BEVEL_LO_WIDTH 2
#define BEVEL_ADDON    (BEVEL_HI_WIDTH+BEVEL_LO_WIDTH)
#define TEXT_3D_TYPE   AST_Embossed

int main(int argc, char* argv[])
{
	Window w ;
	ASVisual *asv = NULL ;
	int screen, depth ;
	char *font_name = "fixed";
	int size = 32 ;
	char *text = "Smart Brown Dog jumps Over The Lazy Fox, and falls into the ditch.";
	ARGB32 text_color = ARGB32_White;
	char *fore_image_file = NULL ;
	char *back_image_file = "test.xpm" ;
	ASImage *fore_im = NULL, *back_im = NULL;
	struct ASFontManager *fontman = NULL;
	struct ASFont  *font = NULL;
	unsigned int width, height ;

	/* see ASView.1 : */
	set_application_name( argv[0] );
	set_output_threshold(OUTPUT_LEVEL_DEBUG);

	if( argc > 1 )
	{
		char *ptr ;
		if( (ptr = strchr(argv[1], ':' )) != NULL )
		{
			size = atoi( ptr+1 );
			font_name = mystrndup( argv[1], ptr - argv[1] );
		}else
			font_name = argv[1] ;
	}else
	{
		show_warning( "Using defaults!\nUsage:  astext <font[:size]> [<text> [<text_color>\n"
 					                   "\t\t[<foreground_image> [<background_image>]]]]" );
	}
	if( argc > 2 )
		text = argv[2] ;

    dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( dpy, screen );

	if( argc > 3 )
		parse_argb_color( argv[3], &text_color );
	if( argc > 4 )
		fore_image_file = argv[4] ;
	if( argc > 5 )
		back_image_file = argv[5] ;

	/* see ASText.1 : */
	if( (fontman = create_font_manager( dpy, NULL, NULL )) != NULL )
		font = get_asfont( fontman, font_name, 0, size, ASF_GuessWho );

	if( font == NULL )
	{
		show_error( "unable to load requested font \"%s\". Aborting.", font_name );
		return 1;
	}

	/* see ASView.3 : */
	asv = create_asvisual( dpy, screen, depth, NULL );

	if( back_image_file ) /* see ASView.2 : */
		back_im = file2ASImage( back_image_file, 0xFFFFFFFF,
		                        SCREEN_GAMMA, 0, NULL );
	/* see ASText.2 : */
	get_text_size( text, font, TEXT_3D_TYPE, &width, &height );
	if( fore_image_file )
	{
		ASImage *tmp = file2ASImage( fore_image_file, 0xFFFFFFFF,
		               		         SCREEN_GAMMA, 0, NULL );
		if( tmp )
		{
			if( tmp->width != width || tmp->height != height )
			{   /* see ASScale.2 : */
				fore_im = scale_asimage( asv, tmp, width, height,
				                         ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );
				destroy_asimage( &tmp );
			}else
				fore_im = tmp ;
		}
	}
	width  += TEXT_MARGIN*2 ;
	height += TEXT_MARGIN*2 ;
	/* see ASView.4 : */
	w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
		                         width+BEVEL_ADDON, height+BEVEL_ADDON, 1, 0, NULL,
								 "ASText" );
	if( w != None )
	{
		Pixmap p ;
		ASImage *text_im ;
		ASImage *rendered_im ;
		ASImageLayer layers[2] ;
		ASImageBevel bevel = {0, 0xFFDDDDDD, 0xFF555555, 0xFFFFFFFF, 0xFF777777, 0xFF444444,
		                      BEVEL_HI_WIDTH, BEVEL_HI_WIDTH,
							  BEVEL_LO_WIDTH, BEVEL_LO_WIDTH,
							  BEVEL_HI_WIDTH, BEVEL_HI_WIDTH,
							  BEVEL_LO_WIDTH, BEVEL_LO_WIDTH } ;

		XSelectInput (dpy, w, (StructureNotifyMask | ButtonPressMask|ButtonReleaseMask));
	  	XMapRaised   (dpy, w);

		/* see ASText.3 : */
		text_im = draw_text( text, font, TEXT_3D_TYPE, 0 );
		if( fore_im )
		{
			move_asimage_channel( fore_im, text_im, IC_ALPHA );
			destroy_asimage( &text_im );
		}else
			fore_im = text_im ;
		/* see ASText.4 : */
		memset( &(layers[0]), 0x00, sizeof(layers) );
		layers[0].im = back_im ;
		layers[0].dst_x = 0 ;
		layers[0].dst_y = 0 ;
		layers[0].clip_width = width ;
		layers[0].clip_height = height ;
		layers[0].merge_scanlines = alphablend_scanlines ;
		layers[0].back_color = ARGB32_Black ;
		layers[0].bevel = &bevel ;
		layers[1].im = fore_im ;
		layers[1].dst_x = TEXT_MARGIN+BEVEL_HI_WIDTH ;
		layers[1].dst_y = TEXT_MARGIN+BEVEL_HI_WIDTH ;
		layers[1].clip_width = fore_im->width ;
		layers[1].clip_height = fore_im->height ;
		layers[1].back_color = text_color ;
		layers[1].merge_scanlines = alphablend_scanlines ;
		rendered_im = merge_layers( asv, &(layers[0]), 2,
									width+BEVEL_ADDON, height+BEVEL_ADDON,
									ASA_XImage, 0, ASIMAGE_QUALITY_DEFAULT);
		destroy_asimage( &fore_im );
		destroy_asimage( &back_im );
		/* see ASView.5 : */
		p = asimage2pixmap( asv, DefaultRootWindow(dpy), rendered_im,
				            NULL, True );
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
				XDestroyWindow( dpy, w );
				XFlush( dpy );
				w = None ;
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
	/* see ASText.1 : */
	destroy_font( font );
	destroy_font_manager( fontman, False );
    if( dpy )
        XCloseDisplay (dpy);
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
 * EXAMPLE
 *     if( (fontman = create_font_manager( dpy, NULL, NULL )) != NULL )
 *         font = get_asfont( fontman, font_name, 0, size, ASF_GuessWho);
 *     if( font == NULL )
 *     {
 *         show_error( "unable to load requested font \"%s\". Aborting.",
 *                     font_name );
 *         return 1;
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
 * EXAMPLE
 *     get_text_size( text, font, &width, &height );
 *     if( fore_image_file )
 *     {
 *         ASImage *tmp = file2ASImage( fore_image_file, 0xFFFFFFFF,
 *                                      SCREEN_GAMMA, 0, NULL );
 *         if( tmp )
 *         {
 *             if( tmp->width != width || tmp->height != height )
 *             {
 *                 fore_im = scale_asimage( asv, tmp, width, height,
 *                     False, 0, ASIMAGE_QUALITY_DEFAULT );
 *                 destroy_asimage( &tmp );
 *             }else
 *                 fore_im = tmp ;
 *         }
 *     }
 * NOTES
 * SEE ALSO
 * get_text_size(), scale_asimage()
 ********/
/****f* libAfterImage/tutorials/ASText.3 [7.3]
 * SYNOPSIS
 * Step 3. Rendering texturized text.
 * DESCRIPTION
 * EXAMPLE
 *     text_im = draw_text( text, font, 0 );
 *     if( fore_im )
 *     {
 *         move_asimage_channel( fore_im, text_im, IC_ALPHA );
 *         destroy_asimage( &text_im );
 *     }else
 *         fore_im = text_im ;
 * NOTES
 * SEE ALSO
 * draw_text(), move_asimage_channel()
 ********/
/****f* libAfterImage/tutorials/ASText.4 [7.4]
 * SYNOPSIS
 * Step 4. Merging foreground and background with bevel.
 * DESCRIPTION
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
 *                                 True, 0, ASIMAGE_QUALITY_DEFAULT);
 *     destroy_asimage( &fore_im );
 *     destroy_asimage( &back_im );
 * NOTES
 * SEE ALSO
 * ASImageLayer, ASImageBevel, merge_layers()
 ********/

