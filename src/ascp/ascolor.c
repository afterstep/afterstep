/*
 * Copyright (c) 2003 Sasha Vasko <sasha@aftercode.net>
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

/*#define DO_CLOCKING      */
#define LOCAL_DEBUG
#define EVENT_TRACE

#include "../../configure.h"

#include "../../libAfterStep/asapp.h"

#include <signal.h>
#include <unistd.h>

#include "../../libAfterImage/afterimage.h"

#include "../../libAfterStep/afterstep.h"
#include "../../libAfterStep/colorscheme.h"

Atom _XA_WM_DELETE_WINDOW = None;
Window
create_top_level_window( ASVisual *asv, Window root, int x, int y,
                         unsigned int width, unsigned int height,
						 unsigned int border_width, unsigned long attr_mask,
						 XSetWindowAttributes *attr, char *app_class )
{
 	Window w = None;
#ifndef X_DISPLAY_MISSING
	char *tmp ;
	XTextProperty name;
	XClassHint class1;

	w = create_visual_window(asv, root, x, y, width, height, border_width, InputOutput, attr_mask, attr );

	tmp = (char*)get_application_name();
    XStringListToTextProperty (&tmp, 1, &name);

    class1.res_name = tmp;	/* for future use */
    class1.res_class = app_class;
    XSetWMProtocols (dpy, w, &_XA_WM_DELETE_WINDOW, 1);
    XSetWMProperties (dpy, w, &name, &name, NULL, 0, NULL, NULL, &class1);
    /* final cleanup */
    XFree ((char *) name.value);

#endif /* X_DISPLAY_MISSING */
	return w;
}
Pixmap
set_window_background_and_free( Window w, Pixmap p )
{
#ifndef X_DISPLAY_MISSING
	if( p != None && w != None )
	{
		XSetWindowBackgroundPixmap( dpy, w, p );
		XClearWindow( dpy, w );
		XFlush( dpy );
		XFreePixmap( dpy, p );
		p = None ;
	}
#endif /* X_DISPLAY_MISSING */
	return p ;
}

void
wait_closedown( Window w )
{
#ifndef X_DISPLAY_MISSING
    if( dpy == NULL || w == None )
		return ;

	XSelectInput (dpy, w, (StructureNotifyMask | ButtonPressMask|ButtonReleaseMask));

	while(w != None)
  	{
    	XEvent event ;
	    XNextEvent (dpy, &event);
  		switch(event.type)
		{
	  		case ClientMessage:
			    if ((event.xclient.format != 32) ||
	  			    (event.xclient.data.l[0] != _XA_WM_DELETE_WINDOW))
					break ;
		  	case ButtonPress:
				XDestroyWindow( dpy, w );
				XFlush( dpy );
				w = None ;
				break ;
		}
  	}
    XCloseDisplay (dpy);
	dpy = NULL ;
#endif
}

/****h* ascolor
 * NAME
 * ascompose is a tool to compose image(s) and display/save it based on
 * supplied XML input file.
 *
 * SYNOPSIS
 * ascolor -f file|-s string [-o file] [-t type] [-V]"
 * ascompose -f file|-s string [-o file] [-t type] [-V]"
 * ascompose -f file|-s string [-o file] [-t type] [-V] [-n]"
 * ascompose -f file|-s string [-o file] [-t type [-c compression_level]] [-V] [-r]"
 * ascompose [-h]
 * ascompose [-v]
 *
 * DESCRIPTION
 *
 * OPTIONS
 *    -h --help          display help and exit.
 *    -v --version       display version and exit.
 *    -V --verbose       increase verbosity. To increase verbosity level
 *                       use several of these, like: ascompose -V -V -V.
 *    -D --debug         maximum verbosity - show everything and
 *                       debug messages.
 * PORTABILITY
 * ascolor could be used both with and without X window system. It has
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
	printf("ascolor version 1.2\n");
}

void usage(void) {
	fprintf( stdout,
		"Usage:\n"
		"ascolor [-h] [-f file] [-o file] [-s string] [-t type] [-v] [-V]"
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
        "  -c --compress level compression level\n"
        "  -v --version       display version and exit\n"
		"  -V --verbose       increase verbosity\n"
		"  -D --debug         show everything and debug messages\n"
	);
}

/****** ascolor/sample
 * EXAMPLE
 * Here is the default script that gets executed by ascompose, if no
 * parameters are given :
 * SOURCE
 */
static char* default_doc_str = "\
	<composite> \
		<gradient angle=45 width=640 height=480 colors=\"BaseDark BaseLight\"/> \
		<composite x=20 y=40> \
			<bevel colors=\"Inactive1Light Inactive1Dark\" border=\"2 2 3 3\" solid=0> \
 				<gradient angle=90 width=300 height=25 colors=\"Inactive1Dark Inactive1Light\"/> \
			</bevel> \
			<text x=5 y=3 font=\"DefaultBold.ttf\" point=22 fgcolor=\"InactiveText1\" bgcolor=#00000000>Unfocused title bar text</text> \
		</composite> \
		<composite x=250 y=80> \
			<bevel colors=\"Inactive2Light Inactive2Dark\" border=\"2 2 3 3\" solid=0> \
				<gradient angle=90 width=250 height=25 colors=\"Inactive2Dark Inactive2Light\"/> \
			</bevel> \
			<text x=5 y=3 font=\"DefaultBold.ttf\" point=22 fgcolor=\"InactiveText2\" bgcolor=#00000000>Sticky title bar text</text> \
		</composite> \
		<composite x=150 y=100> \
			<bevel colors=\"ActiveLight ActiveDark\" border=\"2 2 3 3\" solid=0> \
				<gradient angle=90 width=320 height=25 colors=\"ActiveDark ActiveLight\"/> \
			</bevel> \
			<text x=5 y=3 font=\"DefaultBold.ttf\" point=22 fgcolor=\"ActiveText\" bgcolor=#00000000>Focused title bar text</text> \
		</composite> \
	</composite> \
";
/*******/
int main(int argc, char** argv) {
	ASImage* im = NULL;
	char* doc_str = default_doc_str;
	char* doc_file = NULL;
	char* doc_save = NULL;
	char* doc_save_type = NULL;
    char *doc_compress = NULL ;
	int i;
	int display = 1, onroot = 0;
	ARGB32 base_color = 0xff0000ef;
	ASColorScheme *cs ;
	int angle = ASCS_DEFAULT_ANGLE ;

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
			set_output_threshold(OUTPUT_VERBOSE_THRESHOLD);
			verbose++;
		} else if (!strcmp(argv[i], "--debug") || !strcmp(argv[i], "-D")) {
			set_output_threshold(OUTPUT_LEVEL_DEBUG);
			verbose+=2;
		} else if ((!strcmp(argv[i], "--base_color") || !strcmp(argv[i], "-b")) && i < argc + 1) {
			parse_argb_color( argv[++i], &base_color );
		} else if ((!strcmp(argv[i], "--angle") || !strcmp(argv[i], "-a")) && i < argc + 1) {
			angle = atoi(argv[++i]);
		} else if ((!strcmp(argv[i], "--file") || !strcmp(argv[i], "-f")) && i < argc + 1) {
			doc_file = argv[++i];
		} else if ((!strcmp(argv[i], "--string") || !strcmp(argv[i], "-s")) && i < argc + 1) {
			doc_str = argv[++i];
		} else if ((!strcmp(argv[i], "--output") || !strcmp(argv[i], "-o")) && i < argc + 1) {
			doc_save = argv[++i];
		} else if ((!strcmp(argv[i], "--type") || !strcmp(argv[i], "-t")) && i < argc + 1) {
			doc_save_type = argv[++i];
        } else if ((!strcmp(argv[i], "--compress") || !strcmp(argv[i], "-c")) && i < argc + 1) {
            doc_compress = argv[++i];
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
			show_error("Unable to load file [%s]: %s.", doc_file, strerror(errno));
			exit(1);
		}
	}

    dpy = NULL ;
#ifndef X_DISPLAY_MISSING
    if( display )
    {
        dpy = XOpenDisplay(NULL);
        _XA_WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
        screen = DefaultScreen(dpy);
        depth = DefaultDepth(dpy, screen);
    }
#endif

	asv = create_asvisual(dpy, screen, depth, NULL);

	/* now we need to calculate color scheme and populate XML env vars with colors */
	cs = make_ascolor_scheme( base_color, angle );
	populate_ascs_colors_rgb( cs );
	populate_ascs_colors_xml( cs );
	free( cs );                                /* no longer needed */

	im = compose_asimage_xml(asv, NULL, NULL, doc_str, ASFLAGS_EVERYTHING, verbose, None, NULL);

	if (doc_file && doc_str && doc_str != default_doc_str) free(doc_str);

	/* Automagically determine the output type, if none was given. */
	if (doc_save && !doc_save_type) {
		doc_save_type = strrchr(doc_save, '.');
		if (doc_save_type) doc_save_type++;
	}

	/* Save the result image if desired. */
	if (doc_save && doc_save_type) {
        if(!save_asimage_to_file(doc_save, im, doc_save_type, doc_compress, NULL, 0, 1)) {
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

