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

#undef LOCAL_DEBUG

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../afterbase.h"
#include "../afterimage.h"
#include "common.h"

/****h* libAfterImage/ascompose
 * NAME
 * ascompose is a tool to compose image(s) and display/save it based on
 * supplied XML input file.
 *
 * SYNOPSIS
 * ascompose -f file|-s string [-o file] [-t type] [-V]"
 * ascompose -i include_file [-i more_include_file ... ]-f file|-s string [-o file] [-t type] [-V]"
 * ascompose -f file|-s string [-o file] [-t type] [-V] [-n]"
 * ascompose -f file|-s string [-o file] [-t type [-c compression_level]] [-V] [-r]"
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
 *    -i --include file  include file as input prior to processing main file.
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
 *****/


void showimage(ASImage* im, int onroot);

int screen = 0, depth = 0;

ASVisual *asv;
int verbose = 0;

void version(void) {
	printf("ascompose version 1.2\n");
}

void usage(void) {
	fprintf( stdout,
		"Usage:\n"
		"ascompose [-h] [-f file|-] [-o file] [-s string] [-t type] [-v] [-V]"
#ifndef X_DISPLAY_MISSING
			" [-n] [-r]"
#endif /* X_DISPLAY_MISSING */
			"\n"
		"  -h --help          display this help and exit\n"
        "  -v --version       display version and exit\n"
		" Input options : \n"
		"  -f --file file     an XML file to use as input\n"
		"  					  use '-' for filename to read input from STDIN\n"
		"  -s --string string an XML string to use as input\n"
		"  -i --include file  process file prior to processing other input\n"
		" Output options : \n"
#ifndef X_DISPLAY_MISSING
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
		"  -I --interactivee  run ascompose in interactive mode - tags are processed,\n" 
		"                     as soon as they are closed.\n"
		" Note that when -I option is used in conjunction with input from\n" 
		" string or a file - ascompose will endlesly loop through the contents\n"
		" untill it is killed - usefull for slideshow type of activity.\n"
		" When input comes from STDIN, then ascompose will loop untill Ctrl+D\n"
		" is received (EOF).\n"
	);
}

/****** libAfterImage/ascompose/sample
 * EXAMPLE
 * Here is the default script that gets executed by ascompose, if no
 * parameters are given :
 * SOURCE
 */
static char* default_doc_str = "\
<composite op=hue>\
  <composite op=add>\
    <scale width=512 height=proportional><img id=rose src=rose512.jpg/></scale>\
    <tile width=512 height=384><img src=back.xpm/></tile>\
  </composite>\
  <tile width=512 height=384><img src=fore.xpm/></tile>\
</composite>\
<printf format=\"original image width=%d\n\" var=\"rose.width\"/>\
<printf format=\"original image height=%d\n\" var=\"rose.height\"/>\
<printf format=\"original image size in pixels=%d\n\" val=$rose.width*$rose.height/>\
";
/*******/
/* <printf format="original image height=%d\n" var="rose.height"/>
	<printf format="original image size in pixels=%d\n" val=$rose.width*$rose.height/>
 */ 
	
char *load_stdin();	

int main(int argc, char** argv) {
	ASImage* im = NULL;
	char* doc_str = default_doc_str;
	char* doc_file = NULL;
	char* doc_save = NULL;
	char* doc_save_type = NULL;
    char *doc_compress = NULL ;
	int i;
	int display = 1, onroot = 0;
	Bool interactive = False;

	/* see ASView.1 : */
	set_application_name(argv[0]);

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
			verbose = 0;
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
            interactive = True;
		}
#ifndef X_DISPLAY_MISSING
		  else if (!strcmp(argv[i], "--no-display") || !strcmp(argv[i], "-n")) {
			display = 0;
		} else if ((!strcmp(argv[i], "--root-window") || !strcmp(argv[i], "-r")) && i < argc + 1) {
			onroot = 1;
		}
#endif /* X_DISPLAY_MISSING */
	}
	
	destroy_asvisual( asv, False );
    
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
	if( !interactive ) 
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
			showimage(im, onroot);
		/* Done with the image, finally. */
		if( im ) 
			destroy_asimage(&im);
	}else
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
				}else
				{
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
					printf("<success tag_count=%d/>\n", xb.tags_count );
					add_xml_buffer_chars( &xb, "", 1 );
					LOCAL_DEBUG_OUT("buffer: [%s]", xb.buffer );
	 				im = compose_asimage_xml(asv, my_imman, my_fontman, xb.buffer, ASFLAGS_EVERYTHING, verbose, None, NULL);					
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
							showimage(im, onroot);
						safe_asimage_destroy(im);
						im = NULL ;
					}					
				}else
				{
					printf("<error code=%d text=\"", xb.state );	  
					switch( xb.state ) 
					{
						case ASXML_BadStart : printf( "Text encountered before opening tag bracket - not XML format" ); break;
						case ASXML_BadTagName : printf( "Invalid characters in tag name" );break;
						case ASXML_UnexpectedSlash : printf( "Unexpected '/' encountered");break;
						case ASXML_UnmatchedClose : printf( "Closing tag encountered without opening tag" );break;
						case ASXML_BadAttrName : printf( "Invalid characters in attribute name" );break;
						case ASXML_MissingAttrEq : printf( "Attribute name not followed by '=' character" );break;
						default:
							printf( "Premature end of the input");break;
					}
					printf("\" level=%d tag_count=%d/>\n", xb.level ,xb.tags_count );	  
					break;
				}
			}while(1);
			if( xb.buffer )
				free( xb.buffer );
			destroy_image_manager(my_imman, False);
			destroy_font_manager(my_fontman, False);
		}
		if( fp && fp != stdin ) 
			fclose( fp );
	}	 

	if (doc_file && doc_str && doc_str != default_doc_str) free(doc_str);


#ifdef DEBUG_ALLOCS
	print_unfreed_mem();
#endif

	return 0;
}

void showimage(ASImage* im, int onroot) {
#ifndef X_DISPLAY_MISSING
	if (im && onroot) {
		Pixmap p = asimage2pixmap(asv, DefaultRootWindow(dpy), im, NULL, False);
		p = set_window_background_and_free(DefaultRootWindow(dpy), p);
		wait_closedown(DefaultRootWindow(dpy));
	}

	if(im && !onroot)
	{
		/* see ASView.4 : */
		Window w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
			                         im->width, im->height, 1, 0, NULL,
									 "ASView" );
		if( w != None )
		{
			Pixmap p ;

			XSelectInput (dpy, w, (StructureNotifyMask | ButtonPress));
	  		XMapRaised   (dpy, w);
			/* see ASView.5 : */
			p = asimage2pixmap( asv, DefaultRootWindow(dpy), im, NULL,
				                False );
			/* see common.c:set_window_background_and_free(): */
			p = set_window_background_and_free( w, p );
		}
		/* see common.c: wait_closedown() : */
		wait_closedown(w);
	}

    if( dpy )
        XCloseDisplay (dpy);
#endif /* X_DISPLAY_MISSING */
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

