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

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <libAfterImage/afterbase.h>
#include <libAfterImage/afterimage.h>
#include "common.h"

/****h* libAfterImage/ascompose
 * NAME
 * ascompose is a tool to compose image(s) and display/save it based on
 * supplied XML input file.
 *
 * SYNOPSIS
 * ascompose -f file|-s string [-o file] [-t type] [-V]"
 * ascompose -f file|-s string [-o file] [-t type] [-V]"
 * ascompose -f file|-s string [-o file] [-t type] [-V] [-n]"
 * ascompose -f file|-s string [-o file] [-t type] [-V] [-r]"
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
 *    -v --version       display version and exit.
 *    -V --verbose       increase verbosity. To increase verbosity level
 *                       use several of these, like: ascompose -V -V -V.
 *    -D --debug         maximum verbosity - show everything and
 *                       debug messages.
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
		"ascompose [-h] [-f file] [-o file] [-s string] [-t type] [-v] [-V]"
#ifndef X_DISPLAY_MISSING
			" [-n] [-r]"
#endif /* X_DISPLAY_MISSING */
			"\n"
		"  -h --help          display this help and exit\n"
		"  -f --file file     an XML file to use as input\n"
#ifndef X_DISPLAY_MISSING
		"  -n --no-display    don't display the final image\n"
		"  -r --root-window   draw result image on root window\n"
#endif /* X_DISPLAY_MISSING */
		"  -o --output file   output to file\n"
		"  -s --string string an XML string to use as input\n"
		"  -t --type type     type of file to output to\n"
		"  -v --version       display version and exit\n"
		"  -V --verbose       increase verbosity\n"
		"  -D --debug         show everything and debug messages\n"
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
    <scale width=512 height=384><img src=rose512.jpg/></scale>\
    <tile width=512 height=384><img src=back.xpm/></tile>\
  </composite>\
  <tile width=512 height=384><img src=fore.xpm/></tile>\
</composite>\
";
/*******/
int main(int argc, char** argv) {
	ASImage* im = NULL;
	char* doc_str = default_doc_str;
	char* doc_file = NULL;
	char* doc_save = NULL;
	char* doc_save_type = NULL;
	int i;
	int display = 1, onroot = 0;

	/* see ASView.1 : */
	set_application_name(argv[0]);
	/* Parse command line. */
	for (i = 1 ; i < argc ; i++) {
		if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			version();
			usage();
			exit(0);
		} else if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-v")) {
			version();
			exit(0);
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
		} else if ((!strcmp(argv[i], "--string") || !strcmp(argv[i], "-s")) && i < argc + 1) {
			doc_str = argv[++i];
		} else if ((!strcmp(argv[i], "--output") || !strcmp(argv[i], "-o")) && i < argc + 1) {
			doc_save = argv[++i];
		} else if ((!strcmp(argv[i], "--type") || !strcmp(argv[i], "-t")) && i < argc + 1) {
			doc_save_type = argv[++i];
		}
#ifndef X_DISPLAY_MISSING
		  else if (!strcmp(argv[i], "--no-display") || !strcmp(argv[i], "-n")) {
			display = 0;
		} else if ((!strcmp(argv[i], "--root-window") || !strcmp(argv[i], "-r")) && i < argc + 1) {
			onroot = 1;
		}
#endif /* X_DISPLAY_MISSING */
	}

	/* Load the document from file, if one was given. */
	if (doc_file) {
		doc_str = load_file(doc_file);
		if (!doc_str) {
			fprintf(stderr, "Unable to load file [%s]: %s.\n", doc_file, strerror(errno));
			exit(1);
		}
	}

#ifndef X_DISPLAY_MISSING
	dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth(dpy, screen);
#endif

	asv = create_asvisual(dpy, screen, depth, NULL);

	im = compose_asimage_xml(asv, NULL, NULL, doc_str, ASFLAGS_EVERYTHING, verbose, None, NULL);

	if (doc_file && doc_str && doc_str != default_doc_str) free(doc_str);

	/* Automagically determine the output type, if none was given. */
	if (doc_save && !doc_save_type) {
		doc_save_type = strrchr(doc_save, '.');
		if (doc_save_type) doc_save_type++;
	}

	/* Save the result image if desired. */
	if (doc_save && doc_save_type) {
		if(!save_asimage_to_file(doc_save, im, doc_save_type, NULL, NULL, 0, 1)) {
			show_error("Save failed.");
		} else {
			show_progress("Save successful.");
		}
	}

	/* Display the image if desired. */
	if (display) {
		showimage(im, onroot);
	}

	/* Done with the image, finally. */
	destroy_asimage(&im);

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

