#include "config.h"

#include <string.h>
#include <stdlib.h>

#define LOCAL_DEBUG 

#include "../afterbase.h"
#include "../afterimage.h"
#include "common.h"
#include "../char2uni.h"

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
	printf( "  Where: font - TrueType font's filename or X font spec or alias;\n");
	printf( "         size - size in points for TrueType fonts;\n");
	printf( "         text - text to be drawn;\n");
	printf( "         3D_style - 3D style of text. One of the following:\n");
	printf( "             0 - plain 2D tetx, 1 - embossed, 2 - sunken, 3 - shade above,\n");
	printf( "             4 - shade below, 5 - embossed thick 6 - sunken thick.\n");
	printf( "         resize_type - tells how texture/image should be transformed to fit\n");
	printf( "                       the text size. Could be: scale or tile. Default is tile\n");

}

int main(int argc, char* argv[])
{
	ASVisual *asv = NULL ;
	int screen = 0, depth = 0;
	char *font_name = "test.ttf";
	int size = 15 ;
	char default_text[257] = {0xd0, 0x9d, 0x2e, 0xd0, 0x94, 0xd0, 0xbe, 0xd0, 0xb1, 0xd1, 0x80, 0xd0, 0xbe, 0xd1, 0x85, 0xd0, 0xbe, 0xd1, 0x82, 0xd0, 0xbe, 0xd0, 0xb2, 0xd0, 0xb0, 0x2c, 0x20, 0xd0, 0x92, 0x2e, 0xd0, 0x9f, 0xd1, 0x8f, 0xd1, 0x82, 0xd0, 0xbd, 0xd0, 0xb8, 0xd1, 0x86, 0xd0, 0xba, 0xd0, 0xb8, 0xd0, 0xb9, 0x2e, 0x20, 0xd0, 0x92, 0xd0, 0xb5, 0xd1, 0x81, 0xd0, 0xb5, 0xd0, 0xbb, 0xd1, 0x8b, 0xd0, 0xb5, 0x20, 0xd1, 0x80, 0xd0, 0xb5, 0xd0, 0xb1, 0xd1, 0x8f, 0xd1, 0x82, 0xd0, 0xb0, 0x20, 0x2d, 0x20, 0x4d, 0x6f, 0x7a, 0x69, 0x6c, 0x6c, 0x61, 0x20, 0x7b, 0x42, 0x75, 0x69, 0x6c, 0x64, 0x20, 0x49, 0x44, 0x3a, 0x20, 0x32, 0x30, 0x30, 0x32, 0x30, 0x35, 0x32, 0x39, 0x31, 0x38, 0x7d, 0x00, 0x00, 0x00};
	char *text = &default_text[0];
	ARGB32 text_color = ARGB32_White, back_color = ARGB32_Black;
	char *text_color_name = "#FFFFFFFF", *back_color_name = "#FF000000";
	char *fore_image_file = "fore.xpm", *back_image_file = "back.xpm" ;
	Bool scale_fore_image = False, scale_back_image = False ;
	ASImage *fore_im = NULL, *back_im = NULL;
	ASImage *text_im = NULL ;
	ASText3DType type_3d = AST_ShadeBelow ;
	struct ASFontManager *fontman = NULL;
	struct ASFont  *font = NULL;
	unsigned int width, height ;
	int i;
	int text_margin = size/2 ;


	/* see ASView.1 : */
	set_application_name( argv[0] );
#if (HAVE_AFTERBASE_FLAG==1)
	set_output_threshold(OUTPUT_LEVEL_DEBUG);
#endif

#if 0
	for( i = 0x21 ; i < 256 ;++i ) 
	{
		default_text[k++] = i ;
		if( k%50 == 0 ) 
			default_text[k++] = '\n' ;
	}
	default_text[i-0x21] = '\0' ;
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
		if( i+1 < argc )
		{
			if( strncmp( argv[i], "-l", 2 ) == 0 )
			{
				/* we need to set our charset here : */
				as_set_charset( parse_charset_name( argv[i+1] ));
			}else if( strncmp( argv[i], "-f", 2 ) == 0 )
				font_name = argv[i+1] ;
			else if( strncmp( argv[i], "-s", 2 ) == 0 )
			{
				size = atoi(argv[i+1]);
				text_margin = size/2 ;
			}else if( strncmp( argv[i], "-t", 2 ) == 0 )
				text = argv[i+1] ;
			else if( strncmp( argv[i], "-S", 2 ) == 0 )
			{
				type_3d = atoi(argv[i+1]);
				if( type_3d >= AST_3DTypes )
				{
					show_error( "3D type is wrong. Using 2D Plain instead.");
					type_3d = AST_Plain ;
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

	for( i = 0 ; text[i] ; ) 
	{
		LOCAL_DEBUG_OUT( "pos = %u, char = %c, code = %u, unicode = %lu/%u, raw = %4.4X(%lu)", i, text[i], (unsigned char)text[i], CHAR2UNICODE(text[i]), ((as_current_charset[((unsigned short)text[i])&0x007F])),as_current_charset[((unsigned int)text[i])&0x007F], (CARD32)as_current_charset[((unsigned int)text[i])&0x007F] );
		++i ;
	}
#ifndef X_DISPLAY_MISSING
    dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( dpy, screen );
#endif

	/* see ASText.1 : */
	if( (fontman = create_font_manager( dpy, NULL, NULL )) != NULL )
		font = get_asfont( fontman, font_name, 0, size, ASF_GuessWho );

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
	get_utf8_text_size( text, font, type_3d, &width, &height );
	/*get_text_size( text, font, type_3d, &width, &height );*/
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
		}
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
		}
	}

	/* see ASText.3 : */
	text_im = draw_utf8_text( text, font, type_3d, 0 );
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
		back_im->back_color = back_color ;
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
		layers[1].clip_width = fore_im->width ;
		layers[1].clip_height = fore_im->height ;
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

