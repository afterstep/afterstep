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
#include "../../libAfterConf/afterconf.h"
#include "../../libAfterStep/colorscheme.h"

struct
{
	Window main_window ;

	char* doc_save;
	char* doc_save_type;
    char *doc_compress;
	Bool display ;
	Bool onroot ;
	ARGB32 base_color ;
	int angle ;

	ASColorScheme *cs ;

	ASGeometry geometry ;
	int gravity ;

	Bool user_geometry ;

	ASImage *cs_im ;

}ASColorState;

int verbose = 0;
Window MainWindow

void ascolor_usage(void) {
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
static char* default_doc_str = " \
	<composite> \
		<gradient angle=45 width=640 height=480 colors=\"BaseDark BaseLight\"/> \
		<composite x=20 y=40> \
			<bevel id=\"inactive_back_beveled\" colors=\"Inactive1Light Inactive1Dark\" border=\"2 2 3 3\" solid=0> \
 				<gradient id=\"inactive_back\" angle=90 width=340 height=25 colors=\"Inactive1Dark Inactive1Light\"/> \
			</bevel> \
			<text x=5 y=3 font=\"DefaultBold.ttf\" point=22 fgcolor=\"InactiveText1\">Unfocused title bar text</text> \
		</composite> \
		<composite x=10 y=290> \
			<recall srcid=\"inactive_back_beveled\"/> \
			<text x=5 y=3 font=\"DefaultBold.ttf\" point=22 fgcolor=\"InactiveText1\">Unfocused title bar text</text> \
		</composite> \
		<composite x=290 y=250> \
			<recall srcid=\"inactive_back_beveled\"/> \
			<text x=5 y=3 font=\"DefaultBold.ttf\" point=22 fgcolor=\"InactiveText1\">Unfocused title bar text</text> \
		</composite> \
		<composite x=250 y=80> \
			<bevel colors=\"Inactive2Light Inactive2Dark\" border=\"2 2 3 3\" solid=0> \
				<gradient angle=90 width=350 height=25 colors=\"Inactive2Dark Inactive2Light\"/> \
			</bevel> \
			<text x=5 y=3 font=\"DefaultBold.ttf\" point=22 fgcolor=\"InactiveText2\">Sticky title bar text</text> \
		</composite> \
		<composite x=150 y=60> \
			<bevel colors=\"ActiveLight ActiveDark\" border=\"2 2 3 3\" solid=0> \
				<gradient angle=90 width=400 height=25 colors=\"ActiveDark ActiveLight\"/> \
			</bevel> \
			<text x=5 y=3 font=\"DefaultBold.ttf\" point=22 fgcolor=\"ActiveText\">Focused title bar text</text> \
		</composite> \
		\
		<composite x=50 y=200> \
			<bevel colors=\"HighInactiveLight HighInactiveDark\" border=\"2 2 3 3\" solid=0> \
				<gradient angle=90 width=180 height=20 colors=\"HighInactiveDark HighInactiveLight\"/> \
			</bevel> \
			<text x=5 y=3 font=\"DefaultBold.ttf\" point=20 fgcolor=\"InactiveText1\">Unfocused menu</text> \
		</composite> \
		<composite x=50 y=220> \
			<bevel id=\"inactive_menu_item_back\" colors=\"HighInactiveBackLight HighInactiveBackDark\" border=\"2 2 3 3\" solid=0> \
				<gradient angle=90 width=180 height=20 colors=\"HighInactiveBackDark HighInactiveBackLight\"/> \
			</bevel> \
			<text x=5 y=3 font=\"DefaultBoldOblique.ttf\" point=20 fgcolor=\"HighInactiveText\">Menu item 1</text> \
		</composite> \
		<composite x=50 y=240> \
			<recall srcid=\"inactive_menu_item_back\"/> \
			<text x=5 y=3 font=\"DefaultBoldOblique.ttf\" point=20 fgcolor=\"DisabledText\">Disabled item </text> \
		</composite> \
		<composite x=50 y=260> \
			<recall srcid=\"inactive_menu_item_back\"/> \
			<text x=5 y=3 font=\"DefaultBoldOblique.ttf\" point=20 fgcolor=\"HighInactiveText\">Menu item 2</text> \
		</composite> \
		<composite x=50 y=280> \
			<recall srcid=\"inactive_menu_item_back\"/> \
			<text x=5 y=3 font=\"DefaultBoldOblique.ttf\" point=20 fgcolor=\"HighInactiveText\">Menu item 3</text> \
		</composite> \
		\
		<composite x=220 y=230> \
			<bevel colors=\"HighActiveLight HighActiveDark\" border=\"2 2 3 3\" solid=0> \
				<gradient angle=90 width=180 height=20 colors=\"HighActiveDark HighActiveLight\"/> \
			</bevel> \
			<text x=5 y=3 font=\"DefaultBold.ttf\" point=20 fgcolor=\"ActiveText\">Focused menu</text> \
		</composite> \
		<composite x=220 y=250> \
			<recall srcid=\"inactive_menu_item_back\"/> \
			<text x=5 y=3 font=\"DefaultBoldOblique.ttf\" point=20 fgcolor=\"HighInactiveText\">Menu item 1</text> \
		</composite> \
		<composite x=220 y=270> \
			<bevel colors=\"HighActiveBackLight HighActiveBackDark\" border=\"2 2 3 3\" solid=0> \
				<gradient angle=90 width=180 height=20 colors=\"HighActiveBackDark HighActiveBackLight\"/> \
			</bevel> \
			<text x=5 y=3 font=\"DefaultBoldOblique.ttf\" point=20 fgcolor=\"HighActiveText\">Selected item</text> \
		</composite> \
		<composite x=220 y=290> \
			<recall srcid=\"inactive_menu_item_back\"/> \
			<text x=5 y=3 font=\"DefaultBoldOblique.ttf\" point=20 fgcolor=\"DisabledText\">Disabled item </text> \
		</composite> \
		\
		<composite x=320 y=380> \
			<bevel colors=\"HighInactiveLight HighInactiveDark\" border=\"2 2 3 3\" solid=0> \
				<gradient angle=90 width=180 height=20 colors=\"HighInactiveDark HighInactiveLight\"/> \
			</bevel> \
			<text x=5 y=3 font=\"DefaultBold.ttf\" point=20 fgcolor=\"InactiveText1\">Unfocused menu</text> \
		</composite> \
		<composite x=320 y=400> \
			<recall srcid=\"inactive_menu_item_back\"/> \
			<text x=5 y=3 font=\"DefaultBoldOblique.ttf\" point=20 fgcolor=\"DisabledText\">Disabled item </text> \
		</composite> \
		<composite x=320 y=420> \
			<recall srcid=\"inactive_menu_item_back\"/> \
			<text x=5 y=3 font=\"DefaultBoldOblique.ttf\" point=20 fgcolor=\"HighInactiveText\">Menu item 2</text> \
		</composite> \
		<composite x=320 y=440> \
			<recall srcid=\"inactive_menu_item_back\"/> \
			<text x=5 y=3 font=\"DefaultBoldOblique.ttf\" point=20 fgcolor=\"HighInactiveText\">Menu item 3</text> \
		</composite> \
		\
	</composite> \
";
/*******/
void GetBaseOptions (const char *filename);
Window create_main_window();
void HandleEvents();
void do_colorscheme();


int main(int argc, char** argv) {
	ASImage* im = NULL;
	char* doc_str = default_doc_str;
	char* doc_file = NULL;
	char* doc_save = NULL;
	char* doc_save_type = NULL;
    char *doc_compress = NULL ;
	int i;
	int display = 1, onroot = 0;
	ARGB32 base_color = 0xff00448f;
	ASColorScheme *cs ;
	int angle = ASCS_DEFAULT_ANGLE ;

	/* Save our program name - for error messages */
    InitMyApp (CLASS_ASCOLOR, argc, argv, ascolor_usage, NULL, 0 );

	memset( &ASColorState, 0x00, sizeof(ASColorState ));

	/* Parse command line. */
	for (i = 1 ; i < argc ; i++) {
		if( argv[i] == NULL )
			continue;
		if ((!strcmp(argv[i], "--base_color") || !strcmp(argv[i], "-b")) && i+1 < argc) {
			parse_argb_color( argv[++i], &base_color );
			show_progress ( "base color set to #%lX", base_color );
		} else if ((!strcmp(argv[i], "--angle") || !strcmp(argv[i], "-a")) && i+1 < argc) {
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
		}else if (!strcmp(argv[i], "--no-display") || !strcmp(argv[i], "-n")) {
			display = 0;
		}else if ((!strcmp(argv[i], "--root-window") || !strcmp(argv[i], "-r")) && i < argc + 1) {
			onroot = 1;
		}
	}

	/* Load the document from file, if one was given. */
	if (ASColorState.doc_file) {
		ASColorState.doc_str = load_file(ASColorState.doc_file);
		if (!ASColorState.doc_str) {
			show_error("Unable to load file [%s]: %s.", ASColorState.doc_file, strerror(errno));
			exit(1);
		}
	}
	/* Automagically determine the output type, if none was given. */
	if (ASColorState.doc_save && !ASColorState.doc_save_type)
	{
		ASColorState.doc_save_type = strrchr(ASColorState.doc_save, '.');
		if (ASColorState.doc_save_type)
			ASColorState.doc_save_type++;
	}


	ConnectX( &Scr, PropertyChangeMask );
    ConnectAfterStep ( 0 );

	LoadBaseConfig (GetBaseOptions);

	if( !get_flags( ASColorState.geometry.flags, WidthValue ) )
		ASColorState.geometry.width = 640 ;
	if( !get_flags( ASColorState.geometry.flags, HeightValue ) )
		ASColorState.geometry.height = 480 ;
	if( !get_flags( ASColorState.geometry.flags, XValue ) )
		ASColorState.geometry.x = (Scr.MyDisplayWidth - ASColorState.geometry.width)/2 ;
	if( !get_flags( ASColorState.geometry.flags, YValue ) )
		ASColorState.geometry.y = (Scr.MyDisplayHeight - ASColorState.geometry.height)/2 ;

	if( ASColorState.display )
		ASColorState.main_window = create_main_window();

	do_colorscheme();

	HandleEvents();
	return 0;
}

void
GetBaseOptions (const char *filename)
{
    START_TIME(started);

	ReloadASEnvironment( NULL, NULL, NULL );

    SHOW_TIME("BaseConfigParsingTime",started);
}

void
DeadPipe (int nonsense)
{
    FreeMyAppResources();

#ifdef DEBUG_ALLOCS
    print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

    XFlush (dpy);			/* need this for SetErootPixmap to take effect */
	XCloseDisplay (dpy);		/* need this for SetErootPixmap to take effect */
    exit (0);
}

void
HandleEvents()
{
    ASEvent event;
    Bool has_x_events = False ;
    while (True)
    {
        while((has_x_events = XPending (dpy)))
        {
            if( ASNextEvent (&(event.x), True) )
            {
                event.client = NULL ;
                setup_asevent_from_xevent( &event );
                DispatchEvent( &event );
            }
        }
        module_wait_pipes_input ( NULL );
    }
}

void
DispatchEvent (ASEvent * event)
{
    SHOW_EVENT_TRACE(event);

    event->client = NULL ;
    event->widget = NULL ;

    switch (event->x.type)
    {
	    case ConfigureNotify:
            break;
        case KeyPress :
            break ;
        case KeyRelease :
            break ;
        case ButtonPress:
            break;
        case ButtonRelease:
            break;
	    case ClientMessage:
            LOCAL_DEBUG_OUT("ClientMessage(\"%s\",data=(%lX,%lX,%lX,%lX,%lX)", XGetAtomName( dpy, event->x.xclient.message_type ), event->x.xclient.data.l[0], event->x.xclient.data.l[1], event->x.xclient.data.l[2], event->x.xclient.data.l[3], event->x.xclient.data.l[4]);
            if ( event->x.xclient.format == 32 &&
                 event->x.xclient.data.l[0] == _XA_WM_DELETE_WINDOW )
			{
                DeadPipe(0);
            }else if( event->x.xclient.format == 32 &&
                      event->x.xclient.message_type == _AS_BACKGROUND && event->x.xclient.data.l[1] != None )
            {
            }

	        break;
	    case PropertyNotify:
            if( event->x.xproperty.atom == _XROOTPMAP_ID && event->w == Scr.Root )
            {
                LOCAL_DEBUG_OUT( "root background updated!%s","");
                safe_asimage_destroy( Scr.RootImage );
                Scr.RootImage = NULL ;
            }
            break;
        default:
            return;
    }
}


Window
create_main_window()
{
	Window        w;
	XSizeHints    shints;
	ExtendedWMHints extwm_hints ;
	int x, y ;
    int width = ASColorState.geometry.width;
    int height = ASColorState.geometry.height;
    XSetWindowAttributes attr;

    LOCAL_DEBUG_OUT("configured geometry is %dx%d%+d%+d", width, height, ASColorState.geometry.x, ASColorState.geometry.y );
	switch( ASColorState.gravity )
	{
		case NorthEastGravity :
            x = Scr.MyDisplayWidth - width + ASColorState.geometry.x ;
            y = ASColorState.geometry.y ;
			break;
		case SouthEastGravity :
            x = Scr.MyDisplayWidth - width + ASColorState.geometry.x ;
            y = Scr.MyDisplayHeight - height + ASColorState.geometry.y ;
			break;
		case SouthWestGravity :
            x = ASColorState.geometry.x ;
            y = Scr.MyDisplayHeight - height + ASColorState.geometry.y ;
			break;
		case NorthWestGravity :
		default :
            x = ASColorState.geometry.x ;
            y = ASColorState.geometry.y ;
			break;
	}
    attr.event_mask = StructureNotifyMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask ;
    w = create_visual_window( Scr.asv, Scr.Root, x, y, width, height, 4, InputOutput, CWEventMask, &attr);
    set_client_names( w, MyName, MyName, CLASS_ASCOLOR, MyName );

    Scr.RootClipArea.x = x;
    Scr.RootClipArea.y = y;
    Scr.RootClipArea.width = width;
    Scr.RootClipArea.height = height;

    shints.flags = USSize|PMinSize|PResizeInc|PWinGravity;
    if( ASColorState.user_geometry )
        shints.flags |= USPosition ;
    else
        shints.flags |= PPosition ;

    shints.min_width = 640;
    shints.min_height = 480;
    shints.width_inc = 1;
    shints.height_inc = 1;
	shints.win_gravity = ASColorState.gravity ;

	extwm_hints.pid = getpid();
    extwm_hints.flags = EXTWM_PID|EXTWM_TypeMenu ;

	set_client_hints( w, NULL, &shints, AS_DoesWmDeleteWindow, &extwm_hints );

	/* showing window to let user see that we are doing something */
	XMapRaised (dpy, w);
    LOCAL_DEBUG_OUT( "mapping main window at %ux%u%+d%+d", width, height,  x, y );
    /* final cleanup */
	XFlush (dpy);
	sleep (1);								   /* we have to give AS a chance to spot us */
	/* we will need to wait for PropertyNotify event indicating transition
	   into Withdrawn state, so selecting event mask: */
	XSelectInput (dpy, w, PropertyChangeMask|StructureNotifyMask);
	return w ;
}

void
do_colorscheme()
{
	if( ASColorState.cs )
		free( ASColorState.cs );

	/* Done with the image, finally. */
	if( ASColorState.cs_im )
	{
		destroy_asimage(&ASColorState.cs_im);
		ASColorState.cs_im = NULL ;
	}

	/* now we need to calculate color scheme and populate XML env vars with colors */
	ASColorState.cs = make_ascolor_scheme( ASColorState.base_color, ASColorState.angle );
	populate_ascs_colors_rgb( ASColorState.cs );
	populate_ascs_colors_xml( ASColorState.cs );

	ASColorState.cs_im = compose_asimage_xml(asv, NULL, NULL, ASColorState.doc_str, ASFLAGS_EVERYTHING, False, None, NULL);

	if( ASColorState.cs_im == NULL )
	{
		show_error( "failed to generate colorscheme image. Exiting." );
		exit(1);
	}

	/* Save the result image if desired. */
	if (ASColorState.doc_save && ASColorState.doc_save_type) {
        if(!save_asimage_to_file(ASColorState.doc_save, ASColorState.cs_im, ASColorState.doc_save_type, ASColorState.doc_compress, NULL, 0, 1))
		{
			show_error("Save failed.");
		}else
		{
			show_progress("Save successful.");
		}
	}

	/* Display the image if desired. */
	if (ASColorState.display)
	{
		Pixmap p = asimage2pixmap( Scr.asv, Scr.Root, ASColorState.cs_im, NULL, False );
		XSetWindowBackgroundPixmap( dpy, ASColorState.main_window, p );
		XClearWindow( dpy, ASColorState.main_window );
		XFlush( dpy );
		XFreePixmap( dpy, p );
	}
}

