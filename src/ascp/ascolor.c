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
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/event.h"
#include "../../libAfterStep/wmprops.h"

struct
{
	Window main_window ;

	char *doc_str ;
	char *doc_file;
	char *img_save;
	char *img_save_type;
    char *img_compress;
	char *cs_save;
	Bool display ;
	Bool onroot ;

	ARGB32 base_color ;
	int base_hue, base_sat, base_val ;
	int angle ;

	ASColorScheme *cs ;

	ASGeometry geometry ;
	int gravity ;

	Bool user_geometry ;

	ASImage *cs_im ;

	enum
	{
		ASC_PARAM_BaseHue =0,
		ASC_PARAM_BaseSat	,
		ASC_PARAM_BaseVal	,
		ASC_PARAM_Angle
	} curr_param ;

}ASColorState;

void ascolor_usage()
{
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
void do_change_param( int val );
void DispatchEvent (ASEvent * event);

char *default_cs_save = "colorscheme";

int main(int argc, char** argv)
{
	int i;

	/* Save our program name - for error messages */
    InitMyApp (CLASS_ASCOLOR, argc, argv, ascolor_usage, NULL, 0 );

	memset( &ASColorState, 0x00, sizeof(ASColorState ));
	ASColorState.doc_str = default_doc_str;
	ASColorState.cs_save = default_cs_save;
	ASColorState.display = 1;
	ASColorState.base_color = 0xff00448f;
	ASColorState.angle = ASCS_DEFAULT_ANGLE ;

	/* Parse command line. */
	for (i = 1 ; i < argc ; i++) {
		if( argv[i] == NULL )
			continue;
		if ((!strcmp(argv[i], "--base_color") || !strcmp(argv[i], "-b")) && i+1 < argc) {
			parse_argb_color( argv[++i], &(ASColorState.base_color) );
			show_progress ( "base color set to #%lX", (ASColorState.base_color) );
		} else if ((!strcmp(argv[i], "--angle") || !strcmp(argv[i], "-a")) && i+1 < argc) {
			ASColorState.angle = atoi(argv[++i]);
		} else if ((!strcmp(argv[i], "--file") || !strcmp(argv[i], "-f")) && i < argc + 1) {
			ASColorState.doc_file = argv[++i];
		} else if ((!strcmp(argv[i], "--string") || !strcmp(argv[i], "-s")) && i < argc + 1) {
			ASColorState.doc_str = argv[++i];
		} else if ((!strcmp(argv[i], "--save-image") || !strcmp(argv[i], "-i")) && i < argc + 1) {
			ASColorState.img_save = argv[++i];
		} else if ((!strcmp(argv[i], "--type") || !strcmp(argv[i], "-t")) && i < argc + 1) {
			ASColorState.img_save_type = argv[++i];
        } else if ((!strcmp(argv[i], "--compress") || !strcmp(argv[i], "-c")) && i < argc + 1) {
            ASColorState.img_compress = argv[++i];
		} else if ((!strcmp(argv[i], "--output") || !strcmp(argv[i], "-o")) && i < argc + 1) {
			ASColorState.cs_save = argv[++i];
		} else if (!strcmp(argv[i], "--no-display") || !strcmp(argv[i], "-n")) {
			ASColorState.display = 0;
		} else if ((!strcmp(argv[i], "--root-window") || !strcmp(argv[i], "-r")) && i < argc + 1) {
			ASColorState.onroot = 1;
		}
	}

	/* Load the document from file, if one was given. */
	if (ASColorState.doc_file)
	{
		ASColorState.doc_str = load_file(ASColorState.doc_file);
		if (!ASColorState.doc_str)
		{
			show_error("Unable to load file [%s]: %s.", ASColorState.doc_file, strerror(errno));
			exit(1);
		}
	}
	/* Automagically determine the output type, if none was given. */
	if (ASColorState.img_save && !ASColorState.img_save_type)
	{
		ASColorState.img_save_type = strrchr(ASColorState.img_save, '.');
		if (ASColorState.img_save_type)
			ASColorState.img_save_type++;
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
			{
				int val = 10 ;
				if( (event->x.xbutton.state&ControlMask) )
					val = 1 ;
				if( event->x.xbutton.button == Button1 )
					val = -val ;
				do_change_param(val);
			}
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
    attr.event_mask = StructureNotifyMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|PropertyChangeMask ;
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

	forget_asimage_name( Scr.image_manager, "inactive_back_beveled" );
 	forget_asimage_name( Scr.image_manager, "inactive_back" );
	forget_asimage_name( Scr.image_manager, "inactive_menu_item_back" );

	/* now we need to calculate color scheme and populate XML env vars with colors */
	ASColorState.cs = make_ascolor_scheme( ASColorState.base_color, ASColorState.angle );

	ASColorState.base_hue = ASColorState.cs->base_hue ;
	ASColorState.base_sat = ASColorState.cs->base_sat ;
	ASColorState.base_val = ASColorState.cs->base_val ;

	populate_ascs_colors_rgb( ASColorState.cs );
	populate_ascs_colors_xml( ASColorState.cs );

	ASColorState.cs_im = compose_asimage_xml(Scr.asv, NULL, NULL, ASColorState.doc_str, ASFLAGS_EVERYTHING, False, None, NULL);

	if( ASColorState.cs_im == NULL )
	{
		show_error( "failed to generate colorscheme image. Exiting." );
		exit(1);
	}

	/* Save the result image if desired. */
	if (ASColorState.img_save && ASColorState.img_save_type)
	{
        if(!save_asimage_to_file(ASColorState.img_save, ASColorState.cs_im, ASColorState.img_save_type, ASColorState.img_compress, NULL, 0, 1))
			show_error("Image Save failed.");
		else
			show_progress("Image Save successful.");
	}

	/* Save the result colorscheme if desired. */
	if ( ASColorState.cs_save )
	{
		ColorConfig *config = ASColorScheme2ColorConfig( ASColorState.cs );

		if( WriteColorOptions (ASColorState.cs_save, MyName, config, 0) == 0 )
			show_error("Color Scheme Saved to \"%s\".", ASColorState.cs_save);
		else
			show_progress("Color Scheme Save to \"%s\" unsuccessful.", ASColorState.cs_save);
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

#define normalize_percent_val(v)  (((v)<0)?0:(((v)>=100)?99:(v)))

void
do_change_param( int val )
{
	switch( ASColorState.curr_param )
	{
		case ASC_PARAM_BaseHue  :
			ASColorState.base_hue = normalize_degrees_val(ASColorState.base_hue + val) ;
			break ;
		case ASC_PARAM_BaseSat  :
			ASColorState.base_sat = normalize_percent_val(ASColorState.base_sat + val) ;
			break ;
		case ASC_PARAM_BaseVal  :
			ASColorState.base_val = normalize_percent_val(ASColorState.base_val + val) ;
			break ;
		case ASC_PARAM_Angle    :
			ASColorState.angle += val ;
			if( ASColorState.angle < 10 )
				ASColorState.angle = 10 ;
			else if( ASColorState.angle > 60 )
				ASColorState.angle = 60 ;
			break ;
	}
	ASColorState.base_color = make_color_scheme_argb( 0xFFFF, ASColorState.base_hue,
		                                                      ASColorState.base_sat,
															  ASColorState.base_val );

	do_colorscheme();
}

