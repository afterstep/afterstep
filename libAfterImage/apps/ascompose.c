/*
 * Copyright (c) 2001 Sasha Vasko <sasha@aftercode.net>
 * Copyright (c) 2001 Eric Kowalski <eric@beancrock.net>
 * Copyright (c) 2001 Ethan Fisher <allanon@crystaltokyo.com>
 *
 * This module is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#define LOCAL_DEBUG

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if defined(SHAPE) && !defined(X_DISPLAY_MISSING)
#include <X11/extensions/shape.h>
#endif /* SHAPE */
#include "../afterbase.h"
#include "../afterimage.h"
#include "common.h"



/****h* libAfterImage/ascompose
 * NAME
 * ascompose is a tool to compose image(s) and display/save it based on
 * supplied XML input file.
 *
 * SYNOPSIS
 * ascompose -f file|-s string [-o file] [-t type] [-V]
 * ascompose -i include_file [-i more_include_file ... ]
 * 			 -f file|-s string [-o file] [-t type] [-V]
 * ascompose -f file|-s string [-o file] [-t type] [-V] [-n]
 * ascompose -f file|-s string [-o file] [-t type [-c compression_level]] 
 * 			 [-V] [-r]
 * ascompose [-h]
 * ascompose [-v]
 *
 * DESCRIPTION
 * ascompose reads supplied XML data, and manipulates image accordingly.
 * It could transform images from files of any supported file format,
 * draw gradients, render antialiased texturized text, perform
 * superimposition of arbitrary number of images, and save images into
 * files of any of supported output file formats.
 *
 * At any point, the result of any operation could be assigned a name,
 * and later on referenced under this name.
 *
 * At any point during the script processing, result of any operation
 * could be saved into a file of any supported file types.
 *
 * Internal image format is 32bit ARGB with 8bit per channel.
 *
 * Last image referenced, will be displayed in X window, unless -n option
 * is specified. If -r option is specified, then this image will be
 * displayed in root window of X display, effectively setting a background
 * for a desktop. If -o option is specified, this image will also be
 * saved into the file or requested type.
 *
 * ascompose can be compiled to not reference X Window System, thus
 * allowing it to be used on web servers and any other place. It does not
 * even require X libraries in that case.
 *
 * Supported file types for input are :
 * XPM   - via internal code, or libXpm library.
 * JPEG  - via libJpeg library.
 * PNG   - via libPNG library.
 * XCF   - via internal code. For now XCF support is not complete as it
 *         does not merge layers.
 * PPM/PNM - via internal code.
 * BMP, ICO, CUR - via internal code.
 * GIF   - via libungif library.
 * TIFF  - via libtiff library (including alpha channel support).
 * see libAfterImage/ASImageFileTypes for more.
 *
 * Supported file types for output :
 * XPM   - via internal code, or libXpm library.
 * JPEG  - via libJpeg library.
 * PNG   - via libPNG library.
 * GIF   - via libungif library.
 * TIFF  - via libtiff library (including alpha channel support).
 *
 * OPTIONS
 *    -h --help          display help and exit.
 *    -f --file file     an XML file to use as input.
 *    -s --string string an XML string to use as input.
 *    -n --no-display    don't display the last referenced image.
 *    -r --root-window   draw last referenced image image on root window.
 *    -o --output file   output last referenced image in to a file.
 *                       You should use -t to specify what file type to
 *                       use. Filenames are meaningless when it comes to
 *                       determining what file type to use.
 *    -t --type type     type of file to output to.
 *    -c --compress level compression level.
 *    -v --version       display version and exit.
 *    -V --verbose       increase verbosity. To increase verbosity level
 *                       use several of these, like: ascompose -V -V -V.
 *    -D --debug         maximum verbosity - show everything and
 *                       debug messages.
 *    -i --include file  include file as input prior to processing main 
 * 						 file.
 * PORTABILITY
 * ascompose could be used both with and without X window system. It has
 * been tested on most UNIX flavors on both 32 and 64 bit architecture.
 * It has also been tested under CYGWIN environment on Windows 95/NT/2000
 * USES
 * libAfterImage         all the image manipulation routines.
 * libAfterBase          Optionally. Misc data handling such as hash
 *                       tables and console io. Must be used when compiled
 *                       without X Window support.
 * libJPEG               JPEG image format support.
 * libPNG                PNG image format support.
 * libungif              GIF image format support.
 * libTIFF               TIFF image format support.
 * AUTHOR
 * Ethan Fisher          <allanon at crystaltokyo dot com>
 * Sasha Vasko           <sasha at aftercode dot net>
 * Eric Kowalski         <eric at beancrock dot net>
 *******/


ASVisual *asv;
int verbose = 0;

void version(void) {
	printf("ascompose version 1.2\n");
}

void usage(void) {
	fprintf( stdout,
		"Usage:\n"
		"ascompose [options] [-f file|-|-s string] [-o file]"
		"Available options :\n"
		"  -h --help          display this help and exit\n"
        "  -v --version       display version and exit\n"
		" Input options : \n"
		"  -f --file file     an XML file to use as input\n"
		"  					  use '-' for filename to read input from STDIN\n"
		"  -s --string string an XML string to use as input\n"
		"  -i --include file  process file prior to processing other input\n"
		" Output options : \n"
#ifndef X_DISPLAY_MISSING
		"  -g --geometry WxX+X+Y  set window geometry \n"
		"  -T --title  title  set window's title\n"
		"     --override      override window Manager's controls \n"
		"                     (use for splash windows to avoid window frame)\n"
		"     --center        center window on screen\n"
		"     --topmost       raise window to the top\n"
		"     --no-shape      do not shape window\n"
		"  -n --no-display    don't display the final image\n"
		"  -r --root-window   draw result image on root window\n"
#endif /* X_DISPLAY_MISSING */
		"  -o --output file   output to file\n"
		"  -t --type type     type of file to output to\n"
        "  -c --compress level compression level\n"
		" Feedback options : \n"
		"  -V --verbose       increase verbosity\n"
		"  -q --quiet	      output as little information as possible\n"
		"  -D --debug         show everything and debug messages\n"
		" Interactive options : \n"
		"  -I --interactive   run ascompose in interactive mode - tags are processed,\n" 
		"                     as soon as they are closed.\n"
		"     --timeout value time to wait in between displaying images\n"
		"     --click-timeout seconds\n" 
		"                     time to wait for user click before moving on. -1 - forever.\n"
		"     --endless       endlessly loop through file or string\n"
		"     --dont-clear    don't clear window before displaying next image\n"
		" Note that when -I option is used in conjunction with input from\n" 
		" string or a file - ascompose will endlesly loop through the contents\n"
		" untill it is killed - usefull for slideshow type of activity.\n"
		" When input comes from STDIN, then ascompose will loop untill Ctrl+D\n"
		" is received (EOF).\n"
		"\n"
		"  -C --clipboard     run ascompose waiting for data being copied into clipboard,\n" 
		"                     and displaying/processing it, if it is xml.\n"
		" Examples: \n"
		" To display image.jpg on root window : \n"
		"   ascompose -r -s \"<img src=image.jpg/>\""
		" To display image.jpg on root window scaling it to screen size: \n"
		"   ascompose -r -s \"<scale width=xroot.width height=proportional><img src=image.jpg/></scale>\"\n"
	);
}

/****** libAfterImage/ascompose/sample
 * EXAMPLE
 * Here is the default script that gets executed by ascompose, if no
 * parameters are given :
 * SOURCE
 */
#if 1
static char* default_doc_str = "\
<composite op=hue>\
  <composite op=add>\
    <scale width=512 height=proportional>\
		<img id=rose src=rose512.jpg/></scale>\
    <tile width=512 height=384><img src=back.xpm/></tile>\
  </composite>\
  <tile width=512 height=384><img src=fore.xpm/></tile>\
</composite>\
<printf format=\"original image width=%d\n\" var=\"rose.width\"/>\
<printf format=\"original image height=%d\n\" var=\"rose.height\"/>\
<printf format=\
\"original image size in pixels=%d\n\" val=$rose.width*$rose.height/>";
#else	
static char* default_doc_str = "\
  <composite op=add>\
    <scale width=512 height=proportional>\
		<img id=rose src=rose512.jpg/></scale>\
    <tile width=512 height=384><img src=back.xpm/></tile>\
  </composite>";
#endif
/*******/
char *load_stdin();	

typedef struct ASComposeWinProps
{
	Bool center ;

	int geom_x, geom_y ;
	unsigned int geom_width, geom_height ;
	unsigned long geom_flags ;

	Bool override_redirect ;
	int timeout ;
	int click_timeout ;
	Bool on_top ;
	const char *title ;
	Bool no_shape ; 

	Bool mapped ;
	Bool dont_clear;
	
	int last_x, last_y ; 
	unsigned int last_width, last_height ;
	Pixmap last_root_pmap ; 
	ASImage *last_root_im ;
	
	Pixmap canvas;
	int canvas_width, canvas_height;
}ASComposeWinProps;

Window showimage(ASImage* im, Bool looping, Window main_window, ASComposeWinProps *props, int dst_x, int dst_y);
Window make_main_window(Bool on_root, ASComposeWinProps *props);	

int screen = 0, depth = 0;

int main(int argc, char** argv) {

	ASImage* im = NULL;
	char* doc_str = default_doc_str;
	char* doc_file = NULL;
	char* doc_save = NULL;
	char* doc_save_type = NULL;
    char *doc_compress = NULL ;
	int i;
	int display = 1, onroot = 0;
	Bool quiet = False ;
	enum
	{
		COMPOSE_Once = 0,
		COMPOSE_Interactive,
		COMPOSE_XClipboard
	}compose_type = COMPOSE_Once ;
	Bool endless_loop = False ; 
	Window main_window = None ;
	ASComposeWinProps main_window_props ;
	
	char* ascompose_locale ;
	memset(&main_window_props, 0x00, sizeof( main_window_props));
	main_window_props.click_timeout = -1;

	/* see ASView.1 : */
	set_application_name(argv[0]);
	
	ascompose_locale = mystrdup(getenv("LANG"));

    if( ascompose_locale && strlen(ascompose_locale) > 0)
    {
    	as_set_charset( parse_charset_name( ascompose_locale ));
    }else
	{
#ifdef I18N
		show_warning ("LANG environment variable is not set - use -L \"locale\" command line option to define locale");
#endif
	}


	/* scrap asvisual so we can work on include files ( not displaying anything ) */
	asv = create_asvisual(NULL, 0, 32, NULL);

	/* Parse command line. */
	for (i = 1 ; i < argc ; i++) {
		if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			version();
			usage();
			exit(0);
		} else if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-v")) {
			version();
			exit(0);
		} else if (!strcmp(argv[i], "--quiet") || !strcmp(argv[i], "-q")) {
#if (HAVE_AFTERBASE_FLAG==1)
			set_output_threshold(0);
#endif
			verbose = 0; quiet = True ;
		} else if (!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-V")) {
#if (HAVE_AFTERBASE_FLAG==1)
			set_output_threshold(OUTPUT_VERBOSE_THRESHOLD);
#endif
			verbose++;
		} else if (!strcmp(argv[i], "--debug") || !strcmp(argv[i], "-D")) {
#if (HAVE_AFTERBASE_FLAG==1)
			set_output_threshold(OUTPUT_LEVEL_DEBUG);
#endif
			verbose+=2;
		} else if ((!strcmp(argv[i], "--file") || !strcmp(argv[i], "-f")) && i < argc + 1) {
			doc_file = argv[++i];
		} else if ((!strcmp(argv[i], "--include") || !strcmp(argv[i], "-i")) && i < argc + 1) 
		{
			char *incl_str = load_file(argv[++i]);
	  		if (!incl_str) 
			{
				fprintf(stderr, "Unable to load file [%s]: %s.\n", argv[i], strerror(errno));
			}else
			{
				ASImage *im = compose_asimage_xml(asv, NULL, NULL, incl_str, ASFLAGS_EVERYTHING, verbose, None, NULL);
				free( incl_str );
				if( im )
					destroy_asimage(&im);
			}
		} else if ((!strcmp(argv[i], "--string") || !strcmp(argv[i], "-s")) && i < argc + 1) {
			doc_str = argv[++i];
	   	} else if ((!strcmp(argv[i], "--output") || !strcmp(argv[i], "-o")) && i < argc + 1) {
			doc_save = argv[++i];
		} else if ((!strcmp(argv[i], "--type") || !strcmp(argv[i], "-t")) && i < argc + 1) {
			doc_save_type = argv[++i];
        } else if ((!strcmp(argv[i], "--compress") || !strcmp(argv[i], "-c")) && i < argc + 1) {
            doc_compress = argv[++i];
		} else if (!strcmp(argv[i], "--interactive") || !strcmp(argv[i], "-I")) {
            compose_type = COMPOSE_Interactive ;
		} else if (strcmp(argv[i], "--click-timeout") == 0 && i < argc + 1) {
			main_window_props.click_timeout = strtod( argv[++i ], NULL );
		} else if (strcmp(argv[i], "--timeout") == 0 && i < argc + 1) {
			main_window_props.timeout = strtod( argv[++i ], NULL );
		} else if (!strcmp(argv[i], "--endless")) {
            endless_loop = True ;
		} else if (!strcmp(argv[i], "--dont-clear")) {
            main_window_props.dont_clear = True ;
		}
#ifndef X_DISPLAY_MISSING
		  else if ((!strcmp(argv[i], "--geometry") || !strcmp(argv[i], "-g")) && i < argc + 1) {
			main_window_props.geom_flags = XParseGeometry ( argv[++i], 
															&main_window_props.geom_x,
															&main_window_props.geom_y,
                      					 					&main_window_props.geom_width, 
															&main_window_props.geom_height);
		} else if (strcmp(argv[i], "--override") == 0 ) {
			main_window_props.override_redirect = True;
		} else if ((!strcmp(argv[i], "--title") || !strcmp(argv[i], "-T")) && i < argc + 1) {
			main_window_props.title = argv[++i];
		} else if (strcmp(argv[i], "--center") == 0 ) {
			main_window_props.center = True;
		} else if (strcmp(argv[i], "--topmost") == 0 ) {
			main_window_props.on_top = True;
		} else if (strcmp(argv[i], "--no-shape") == 0 ) {
			main_window_props.no_shape = True;
		} else if (!strcmp(argv[i], "--clipboard") || !strcmp(argv[i], "-C")) {
			compose_type = COMPOSE_XClipboard;
		}   else if (!strcmp(argv[i], "--no-display") || !strcmp(argv[i], "-n")) {
			display = 0;
		} else if ((!strcmp(argv[i], "--root-window") || !strcmp(argv[i], "-r")) && i < argc + 1) {
			onroot = 1;
		}
#endif /* X_DISPLAY_MISSING */
	}
	
	destroy_asvisual( asv, False );
	asv = NULL ;
    
	dpy = NULL ;
#ifndef X_DISPLAY_MISSING
    if( display )
    {
		LOCAL_DEBUG_OUT( "Opening display ...%s", "");
        dpy = XOpenDisplay(NULL);
		LOCAL_DEBUG_OUT( "Done: %p", dpy);
		if( dpy )
		{	
        	_XA_WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
        	screen = DefaultScreen(dpy);
        	depth = DefaultDepth(dpy, screen);
		}
    }
#endif
	if( dpy == NULL && doc_file == NULL && doc_str == default_doc_str )
		doc_file = strdup("-");

	/* Automagically determine the output type, if none was given. */
	if (doc_save && !doc_save_type) {
		doc_save_type = strrchr(doc_save, '.');
		if (doc_save_type) doc_save_type++;
	}

	LOCAL_DEBUG_OUT( "Creating visual ...%s", "");
	asv = create_asvisual(dpy, screen, depth, NULL);
	LOCAL_DEBUG_OUT( "Done: %p", asv);

	/* Load the document from file, if one was given. */
	if( compose_type == COMPOSE_Once ) 
	{	   
		if (doc_file) {
			if( strcmp( doc_file, "-") == 0 ) 
				doc_str = load_stdin();
			else
				doc_str = load_file(doc_file);
			if (!doc_str) 
			{
				show_error("Unable to load file [%s]: %s.\n", doc_file, strerror(errno));
				exit(1);
			}
		}
		
		im = compose_asimage_xml(asv, NULL, NULL, doc_str, ASFLAGS_EVERYTHING, verbose, None, NULL);
		/* Save the result image if desired. */
		if (doc_save && doc_save_type) 
		{
        	if(!save_asimage_to_file(doc_save, im, doc_save_type, doc_compress, NULL, 0, 1)) 
				show_error("Save failed.");
			else
				show_progress("Save successful.");
		}
		/* Display the image if desired. */
		if (display && dpy)
		{
			showimage(im, False, make_main_window(onroot, &main_window_props), &main_window_props, 0, 0);
			LOCAL_DEBUG_OUT( "Image %p displayed", im );
		}
		/* Done with the image, finally. */
		if( im ) 
		{	
			safe_asimage_destroy( im );
			im = NULL ;
		}
	}else if( compose_type == COMPOSE_Interactive )
	{
		FILE *fp = stdin ;
		int doc_str_len = 0;
		if (doc_file && strcmp( doc_file, "-") != 0 ) 
			fp = fopen( doc_file, "rt" );
		if( doc_str ) 
			doc_str_len = strlen( doc_str );
				   
		if( fp != NULL || doc_str_len > 0 )
		{
			ASImageManager *my_imman = create_generic_imageman(NULL);
			ASFontManager  *my_fontman = create_generic_fontman(asv->dpy, NULL);
			int char_count = 0 ;
			ASXmlBuffer xb ; 
			
			memset( &xb, 0x00, sizeof(xb));
	 		
			if (display && dpy) 
				main_window = make_main_window( onroot, &main_window_props );

			do
			{
				reset_xml_buffer( &xb );
				if( fp ) 	  
				{
					int c ;
					show_progress("Please enter your xml text :" );

					while( (c = fgetc(fp)) != EOF ) 
					{
						char cc = c; 
						while( xb.state >= 0 && spool_xml_tag( &xb, &cc, 1 ) <= 0)
						{	
							LOCAL_DEBUG_OUT("[%c] : state=%d, tags_count=%d, level = %d, tag_type = %d", 
								             cc, xb.state, xb.tags_count, xb.level, xb.tag_type );
						}
						LOCAL_DEBUG_OUT("[%c] : state=%d, tags_count=%d, level = %d, tag_type = %d", 
								        cc, xb.state, xb.tags_count, xb.level, xb.tag_type );

						++char_count ;
						if( ( xb.state == ASXML_Start && xb.tags_count > 0 && xb.level == 0) || 
							  xb.state < 0 ) 
							break;
					}		   
					if (c == EOF)
					{	
						if (fp != stdin && endless_loop) 
						{	
							fseek( fp, 0L, SEEK_SET );
							char_count = 0 ;
							if( xb.state == ASXML_Start && xb.tags_count == 0 ) 
								continue;
						}else if( xb.tags_count == 0 ) 
							break;
					}
				}else
				{
					if( char_count >= doc_str_len ) 
					{
						if( !endless_loop ) 	  
							break;
						char_count = 0 ;
					}
					while( char_count < doc_str_len ) 
					{
						char_count += spool_xml_tag( &xb, &doc_str[char_count], doc_str_len - char_count );							   
						if( ( xb.state == ASXML_Start && xb.tags_count > 0 && xb.level == 0) || 
							  xb.state < 0 ) 
							break;
					}												   
				}		 
				if( xb.state == ASXML_Start && xb.tags_count > 0 && xb.level == 0 ) 
				{
					int dst_x = 0, dst_y = 0;
					xml_elem_t* doc;

					if( !display || dpy == NULL || !quiet ) 
						printf("<success tag_count=%d/>\n", xb.tags_count );
					
					add_xml_buffer_chars( &xb, "", 1 );
					LOCAL_DEBUG_OUT("buffer: [%s]", xb.buffer );

					if ((doc = xml_parse_doc(xb.buffer, NULL)) != NULL)
					{
						xml_elem_t *tmp, *parm = doc->child?xml_parse_parm(doc->child->parm, NULL):NULL;
						im = compose_asimage_xml_from_doc(asv, my_imman, my_fontman, doc, ASFLAGS_EVERYTHING, verbose, None, NULL, -1, -1);

						if (parm)
						{
							for (tmp = parm ; tmp ; tmp = tmp->next)
							{
								if (!strcmp(tmp->tag, "x")) dst_x = parse_math(tmp->parm, NULL, 0);
								else if (!strcmp(tmp->tag, "y")) dst_y = parse_math(tmp->parm, NULL, 0);
							}
							xml_elem_delete(NULL, parm);
						}
						xml_elem_delete(NULL, doc);
					}
					if( im ) 
					{
						/* Save the result image if desired. */
						if (doc_save && doc_save_type) 
						{
        					if(!save_asimage_to_file(doc_save, im, doc_save_type, doc_compress, NULL, 0, 1)) 
								show_error("Save failed.");
							else
								show_progress("Save successful.");
						}
						/* Display the image if desired. */
						if (display && dpy) 
							main_window = showimage(im, True, main_window, &main_window_props, dst_x, dst_y);

						safe_asimage_destroy(im);
						im = NULL ;
					}					
				}else if( fp == stdin && xb.state == ASXML_Start && xb.tags_count == 0 && xb.level == 0 ) 
				{
					if( !display || dpy == NULL || !quiet ) 
						printf("<success tag_count=%d/>\n", xb.tags_count );						  
					if (!endless_loop)
						break;
				}else
				{
					if( !display || dpy == NULL || !quiet ) 
					{	
						xml_elem_t *msg = format_xml_buffer_state (&xb);
						if (msg)
						{
							xml_print(msg);
							xml_elem_delete(NULL, msg);
						}
					}
					if (!endless_loop)
						break;
				}
			}while( !display || dpy == NULL || main_window != None);
			if( xb.buffer )
				free( xb.buffer );
			destroy_image_manager(my_imman, False);
			destroy_font_manager(my_fontman, False);
		}
		if( fp && fp != stdin ) 
			fclose( fp );
	}
#ifndef X_DISPLAY_MISSING		  	
	else if( compose_type == COMPOSE_XClipboard && dpy )
	{
		Atom clipboard_prop ;
		ASXmlBuffer xb ; 
		int nbytes = 0 ;
		char *bytes = NULL ;
		int char_count = 0 ;
		ASImageManager *my_imman = create_generic_imageman(NULL);
		ASFontManager  *my_fontman = create_generic_fontman(asv->dpy, NULL);
			
		memset( &xb, 0x00, sizeof(xb));
		if (display) 
			main_window = make_main_window( onroot, &main_window_props );
		
		XSelectInput( dpy, DefaultRootWindow(dpy), PropertyChangeMask );
		clipboard_prop = XInternAtom( dpy, "CUT_BUFFER0", False );
		while( main_window || !display ) 
		{
    		XEvent event ;
			Bool show_next = False ;
			
			XNextEvent (dpy, &event);
  			switch(event.type)
			{
				case PropertyNotify :
					if( event.xproperty.atom == clipboard_prop ) 
					{
						if( bytes ) 
							XFree(bytes);
						bytes = XFetchBytes( dpy, &nbytes );
						char_count = 0 ; 
						show_next = True ;
					}	 
				    break ;
	  			case ClientMessage:
					if (event.xclient.format == 32 &&
	  					event.xclient.data.l[0] == _XA_WM_DELETE_WINDOW)
					{
						if( main_window != DefaultRootWindow(dpy) )
							XDestroyWindow( dpy, main_window );
						XFlush( dpy );
						main_window = None ;
					}
					break;
				case ButtonPress:
					if( nbytes > char_count ) 
						show_next = True ;
					else if( main_window != DefaultRootWindow(dpy) )
						XUnmapWindow( dpy, main_window );
					break;
			}
			if( show_next ) 
			{
				reset_xml_buffer( &xb );
				while( char_count < nbytes ) 
				{
					char_count += spool_xml_tag( &xb, &bytes[char_count], nbytes - char_count );							   
					if( ( xb.state == ASXML_Start && xb.tags_count > 0 && xb.level == 0) || 
						xb.state < 0 ) 
						break;
				}												   
				
				if( xb.state == ASXML_Start && xb.tags_count > 0 && xb.level == 0 ) 
				{
					xml_elem_t* doc;
					int dst_x = 0, dst_y = 0;
					
					add_xml_buffer_chars( &xb, "", 1 );
					LOCAL_DEBUG_OUT("buffer: [%s]", xb.buffer );

					if ((doc = xml_parse_doc(xb.buffer, NULL)) != NULL)
					{
						xml_elem_t *tmp, *parm = doc->child?xml_parse_parm(doc->child->parm, NULL):NULL;
						im = compose_asimage_xml_from_doc(asv, my_imman, my_fontman, doc, ASFLAGS_EVERYTHING, verbose, None, NULL, -1, -1);
						if (parm)
						{
							for (tmp = parm ; tmp ; tmp = tmp->next)
							{
								if (!strcmp(tmp->tag, "x")) dst_x = parse_math(tmp->parm, NULL, 0);
								else if (!strcmp(tmp->tag, "y")) dst_y = parse_math(tmp->parm, NULL, 0);
							}
							xml_elem_delete(NULL, parm);
						}
						xml_elem_delete(NULL, doc);
					}

					if( im ) 
					{
						/* Save the result image if desired. */
						if (doc_save && doc_save_type) 
						{
        					if(!save_asimage_to_file(doc_save, im, doc_save_type, doc_compress, NULL, 0, 1)) 
								show_error("Save failed.");
							else
								show_progress("Save successful.");
						}
						/* Display the image if desired. */
						if (display && dpy) 
							main_window = showimage(im, True, main_window, &main_window_props, dst_x, dst_y);
						safe_asimage_destroy(im);
						im = NULL ;
					}					
				}
			}	 
		}	 

		if( bytes ) 
			XFree( bytes );
		if( xb.buffer )
			free( xb.buffer );
		destroy_image_manager(my_imman, False);
		destroy_font_manager(my_fontman, False);
	}		 
#endif
	if (doc_file && doc_str && doc_str != default_doc_str) free(doc_str);

#if !defined(X_DISPLAY_MISSING)    
	if (main_window_props.canvas)
		XFreePixmap(dpy, main_window_props.canvas); 
	if( dpy )
        XCloseDisplay (dpy);
#endif
	LOCAL_DEBUG_OUT( "display Closed%s","");
#ifdef DEBUG_ALLOCS
	if (main_window_props.last_root_im)
		safe_asimage_destroy(main_window_props.last_root_im);
	asxml_var_cleanup();
	LOCAL_DEBUG_OUT( "display Closed%s","");
	custom_color_cleanup();
	LOCAL_DEBUG_OUT( "display Closed%s","");
    build_xpm_colormap( NULL );
	LOCAL_DEBUG_OUT( "display Closed%s","");
	flush_default_asstorage();
	LOCAL_DEBUG_OUT( "display Closed%s","");
//	destroy_asvisual( asv, False );
	LOCAL_DEBUG_OUT( "display Closed%s","");
    flush_ashash_memory_pool();
	LOCAL_DEBUG_OUT( "display Closed%s","");
	print_unfreed_mem();
	print_asimage_registry();
#endif

	return 0;
}

Window 
make_main_window(Bool onroot, ASComposeWinProps *props)	
{
	Window w = None ;
#ifndef X_DISPLAY_MISSING		  
	XSetWindowAttributes attributes;

	if( onroot ) 
	{	
		w = DefaultRootWindow(dpy);
		props->last_x = 0 ;
		props->last_y = 0 ;
		props->last_width = 0 ;
		props->last_height = 0 ;
	}else
	{
		attributes.override_redirect = props->override_redirect ; 
		w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
				                        100, 30, 0, CWOverrideRedirect, &attributes, 
										"ASCompose",
										props->title );
		props->last_x = 32 ;
		props->last_y = 32 ;
		props->last_width = 100 ;
		props->last_height = 30 ;
		XSelectInput (dpy, w, (StructureNotifyMask|ButtonPressMask|ButtonReleaseMask));
	}	 
	props->last_root_pmap = None ;
	if( props->last_root_im ) 
		safe_asimage_destroy(props->last_root_im);
	props->last_root_im = NULL ; 
		

#endif	 
	return w;
}

Bool
set_root_pixmap_property(long pmap) /* Must have long type to work with XChangeProp on 64 bit machines !!!*/
{
	Window root = DefaultRootWindow(dpy);
	char  *names[2] = {"_XROOTPMAP_ID", "ESETROOT_PMAP_ID"};
	Atom  atoms[2];
	int i;

	if (XInternAtoms (dpy, &(names[0]), 2, True, &(atoms[0])) != 0)
	{
		Pixmap pmaps[2] = {0, 0};
	    Atom type;
    	int format;
	    unsigned long nitems, after;
		union 
		{
			unsigned char *uc_ptr ;
			long 		  *long_ptr ;
		}data;

		for (i = 0 ; i < 2 ; ++i)
			if (atoms[i])
			{
				LOCAL_DEBUG_OUT("atoms[%d] = %X", i, atoms[i]);
				data.long_ptr = NULL ;
		        XGetWindowProperty(dpy, root, atoms[i], 0L, 1L, False, AnyPropertyType, &type, &format, &nitems, &after, &data.uc_ptr);
				if (data.long_ptr == NULL)
					break;
				pmaps[i] = data.long_ptr[0] ;
				LOCAL_DEBUG_OUT("pmaps[%d] = %X", i, pmaps[i]);
				if (format != 32 || nitems == 0 || pmaps[i] != pmaps[0] || type != XA_PIXMAP) 
					break;
			}
		if (i>= 2 && pmaps[0]) 
		{
			LOCAL_DEBUG_OUT("killing client for pmap %X", pmaps[0]);
           	XKillClient(dpy, pmaps[0]);
		}
    }
	for (i = 0 ; i < 2 ; ++i)
	{
		if (!atoms[i])
			if ( (atoms[i] = XInternAtom (dpy, names[i], False)) == None)
				break;
    	XChangeProperty(dpy, root, atoms[i], XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pmap, 1);
	}
	if (i < 2)
		return False;
	
	XSetCloseDownMode(dpy, RetainPermanent);
	return True;
}

Bool
wait_x_timeout(int timeout)
{
	if (timeout < 0) /* wait forever */
	{
		XEvent evt;
		XPeekEvent(dpy, &evt);
		return True;
	}else if (timeout > 0)
	{
		fd_set        in_fdset;
		int x_fd = XConnectionNumber (dpy);
		struct timeval tv;
		struct timeval *t = NULL;
		int retval = 0 ;
			
		FD_ZERO (&in_fdset);
		FD_SET (x_fd, &in_fdset);
		tv.tv_sec = timeout/1000 ;
		tv.tv_usec = (timeout%1000)*1000;
		t = &tv;
		retval = PORTABLE_SELECT(x_fd+1,&in_fdset,NULL,NULL,t);
		return (retval > 0);
	}
	
	return XPending(dpy);
}

Window showimage(ASImage* im, Bool looping, Window main_window, ASComposeWinProps *props, int dst_x, int dst_y ) 
{
#ifndef X_DISPLAY_MISSING
	int x = 32, y = 32;
	ASImage *orig_im = im ;
	unsigned int width, height;
	unsigned int shape_rects_count = 0;
	XRectangle *shape_rects = NULL ;
	Bool done = False ;
	Window root = DefaultRootWindow(dpy);
	int screen = DefaultScreen(dpy);
	int root_w = DisplayWidth (dpy, screen);
	int root_h = DisplayHeight (dpy, screen);
	Pixmap saved_canvas = None;

	if (im == NULL || main_window == None ) 
		return None;

	width = im->width + dst_x;
	height = im->height + dst_y;
	   
	if( main_window != root)
	{	
		Bool move = True ;
		
		if( get_flags( props->geom_flags, WidthValue) && props->geom_width > 0  )
			width = props->geom_width ;
		if( get_flags( props->geom_flags, HeightValue)&& props->geom_height > 0  )
			height = props->geom_height ;
		
		if( props->center ) 
		{	
			x = (root_w - width)/2;
			y = (root_h - height)/2;
		}else if( get_flags( props->geom_flags, XValue|YValue) )
		{
	 		x = props->geom_x ;
			y = props->geom_y ;
			if( get_flags( props->geom_flags, XNegative ) )
				x = root_w - width + x ;
			if( get_flags( props->geom_flags, YNegative ) )
				y = root_h - height + y ;
		}else 
			move = False ;

		if( move && (props->last_x != x || props->last_y != y))
		{	
			XMoveWindow( dpy, main_window, x, y );
			props->last_x = x ;
			props->last_y = y ;
		}
		if( props->last_width != width || props->last_height != height )
		{	
			XResizeWindow( dpy, main_window, width, height );
			props->last_width = width ;
			props->last_height = height ;
			saved_canvas = props->canvas;
			props->canvas = None; /* force resizng the pixmap !!! */
		}
	
		if( !props->mapped ) 
		{	
			if( props->geom_flags != 0 ) 
			{
				XSizeHints hints ;
				hints.flags = PWinGravity ; 
				if( get_flags( props->geom_flags, WidthValue|HeightValue) )
					hints.flags |= USSize ; 
				if( get_flags( props->geom_flags, XValue|YValue) )
					hints.flags |= USPosition ; 
				hints.win_gravity = NorthWestGravity ;
				if( get_flags( props->geom_flags, XNegative) && !get_flags( props->geom_flags, YNegative)  )
					hints.win_gravity = NorthEastGravity ;
				else if( !get_flags( props->geom_flags, XNegative) && get_flags( props->geom_flags, YNegative)  )
					hints.win_gravity = SouthWestGravity ;
				else if( get_flags( props->geom_flags, XNegative) && get_flags( props->geom_flags, YNegative)  )
					hints.win_gravity = SouthEastGravity ;
				XSetWMNormalHints( dpy, main_window, &hints );
			}	 
			XMapWindow( dpy, main_window);
			props->mapped = True ;
		}
		if( props->on_top ) 
			XRaiseWindow( dpy, main_window );
		if( get_flags(get_asimage_chanmask(im), SCL_DO_ALPHA))
		{	
#ifdef SHAPE
			if( !props->no_shape ) 
				shape_rects = get_asimage_channel_rects( im, IC_ALPHA, 10, &shape_rects_count );
#endif		   
#if 1		
			{		
				unsigned int root_pmap_width, root_pmap_height;
				Pixmap rp = GetRootPixmap(None);
				ASImage *transp_im = NULL , *tmp ;
				int root_x, root_y ; Window wdumm;
				XTranslateCoordinates( dpy, main_window, root, 0, 0, &root_x, &root_y, &wdumm);
					
				if (rp) 
				{
					if( props->last_root_pmap != rp ||
						props->last_root_im == NULL )   
					{
						if( props->last_root_im ) 
							safe_asimage_destroy(props->last_root_im);
						get_dpy_drawable_size(asv->dpy, rp, &root_pmap_width, &root_pmap_height);
						transp_im = pixmap2asimage(asv, rp, 0, 0, root_pmap_width, root_pmap_height, 0xFFFFFFFF, False, 0);
						props->last_root_pmap = rp ;
						props->last_root_im = transp_im ;   
					}else
					{
						width = props->last_root_im->width ; 
						height = props->last_root_im->height ;
						transp_im = props->last_root_im ;
					}	 
				}
		
				if( transp_im ) 
				{   /* Build the layers first. */	
					ASImageLayer *layers = create_image_layers( 2 );
					
					layers[0].im = transp_im ;
					layers[0].clip_x = root_x+dst_x;
					layers[0].clip_y = root_y+dst_y;
					layers[0].clip_width = im->width ;
					layers[0].clip_height = im->height ;
					layers[1].im = im ;
					layers[1].clip_width = im->width ;
					layers[1].clip_height = im->height ;
					tmp = merge_layers(asv, layers, 2, im->width, im->height, 
									   get_flags( asv->glx_support, ASGLX_UseForImageTx )?ASA_ASImage:ASA_XImage, 0, ASIMAGE_QUALITY_DEFAULT);
					if( tmp ) 
						im = tmp ;
					free( layers );
				}		
			}
#endif
		}		   
	}

	if (props->canvas == None)
	{
		int depth = 0;
		int pmap_width = width;
		int pmap_height = height;
		GC gc = NULL;
		if (main_window==root)
		{
			pmap_width = dst_x+im->width;
			pmap_height = dst_x+im->height;
			depth = DefaultDepth(dpy,screen);
			gc = DefaultGC(dpy, screen);
		}
		props->canvas = create_visual_pixmap( asv, main_window, pmap_width, pmap_height, DefaultDepth(dpy,screen));
		if (gc == NULL)
			gc = XCreateGC( dpy, main_window, 0, NULL);
		
		if (pmap_width != im->width || pmap_height != im->height)
			XFillRectangle( dpy, props->canvas, gc, 0, 0, pmap_width, pmap_height);

		if (saved_canvas)
            XCopyArea (dpy, saved_canvas, props->canvas, gc, 
			           0, 0, MIN(props->canvas_width,pmap_width), MIN(props->canvas_height,pmap_height), 
					   0, 0);
					   
		if (gc != DefaultGC(dpy, screen))
			XFreeGC (dpy, gc);

		props->canvas_width = pmap_width;
		props->canvas_height = pmap_height;
	}
	if (main_window==root)
	{
		XImage *xim = create_visual_scratch_ximage( asv, im->width, im->height, DefaultDepth(dpy,screen) );
		if( subimage2ximage (asv, im, 0, 0, xim)	)
		{	
			put_ximage( asv, xim, props->canvas, DefaultGC(dpy,screen), 0, 0, dst_x, dst_y, im->width, im->height );	
		}
		XDestroyImage( xim );				   
	}else
	{
		if( get_flags( asv->glx_support, ASGLX_UseForImageTx ) )
			done = asimage2drawable_gl( asv, props->canvas, im, 0, 0, 0, 0, 
										im->width, im->height, 
				   						im->width, im->height, 
										False );
		if( !done ) 
			asimage2drawable( asv, props->canvas, im, NULL, 0, 0, dst_x, dst_y, im->width, im->height, True);
	}

	if( im != orig_im ) 
	{	
		safe_asimage_destroy(im);
		im = orig_im ;
	}

	XSetWindowBackgroundPixmap( dpy, main_window, props->canvas);
	XClearWindow( dpy, main_window);
	XFlush(dpy);
	if (main_window == root && !looping)
		set_root_pixmap_property(props->canvas);
	XSync(dpy,False);
		
	if (saved_canvas)
		XFreePixmap( dpy, saved_canvas);

	if (main_window == root && !looping)
		return root;
	
#if 1
#ifdef SHAPE
	if( shape_rects == NULL || shape_rects_count == 0 ) 
		XShapeCombineMask( dpy, main_window, ShapeBounding, 0, 0, None, ShapeSet );
	else
	{	
		XShapeCombineRectangles (dpy, main_window, ShapeBounding, 0, 0, shape_rects, shape_rects_count, ShapeUnion, Unsorted);
		free( shape_rects );
		shape_rects = NULL ;
	}
#endif		   
#endif		   
	XSync(dpy, False);
	
	while(main_window != None)
  	{
    	XEvent event ;
		Bool do_close = False ;
		
		if ( props->timeout && !wait_x_timeout(props->timeout) )
		{
			if( looping ) 
				return main_window;
			do_close = True ;
		}	 
		if( !do_close ) 
		{	
			if (!XPending(dpy))
				if (!wait_x_timeout(props->click_timeout))
					if( looping ) 
						return main_window;

	    	XNextEvent (dpy, &event);
  			switch(event.type)
			{
	  			case ClientMessage:
			    	if (event.xclient.format == 32 &&
	  			    	event.xclient.data.l[0] == _XA_WM_DELETE_WINDOW)
					{
						do_close = True ;
					}
					break;
		  		case ButtonPress:
					LOCAL_DEBUG_OUT( "ButtonPress: looping = %d", looping);
					if( looping ) 
						return main_window;
					do_close = True ;
					break;
			}
		}
		if( do_close ) 
		{
			if( main_window != root )
				XDestroyWindow( dpy, main_window );
			XFlush( dpy );
			main_window = None ;
		}	 
  	}
	
#endif /* X_DISPLAY_MISSING */
	return main_window;
}


char *load_stdin()
{
#define BUFSIZE 512	
	char buffer[BUFSIZE] ;
	char *complete = safemalloc(8192) ; 
	int complete_allocated = 8192 ;
	int complete_curr = 0 ;
	int len ;
	
	while( fgets( &buffer[0], BUFSIZE, stdin ) != NULL )
	{
		len = strlen( &buffer[0] );
		if( complete_curr + len > complete_allocated ) 
		{
			complete_allocated+=len	  ;
	 		complete = realloc( complete, complete_allocated );
		}
		memcpy( &complete[complete_curr], &buffer[0], len ); 	  
		complete_curr += len ;
	}		 
	return complete;
}	 

