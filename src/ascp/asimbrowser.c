/*
 * Copyright (c) 2003 Sasha Vasko <sasha@aftercode.net>
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

#include <X11/keysym.h>

struct
{
	Window main_window ;
	char *dir ;
	char *html_save, *html_save_dir ;
	Bool display ;
	ASGeometry geometry ;
	Bool user_geometry ;
	int gravity ;
}ASIMBrowserState;

void asimbrowser_usage()
{
	fprintf( stdout,
		"Usage:\n"
		"asimbrowse [-h] [-f file] [-o file] [-s string] [-t type] [-v] [-V]"
#ifndef X_DISPLAY_MISSING
			" [-n] [-r]"
#endif /* X_DISPLAY_MISSING */
			"\n"
		"  -h --help          display this help and exit\n"
#ifndef X_DISPLAY_MISSING
		"  -n --no-display    don't display the final image\n"
#endif /* X_DISPLAY_MISSING */
		"  -o --output file   output to file\n"
        "  -v --version       display version and exit\n"
		"  -V --verbose       increase verbosity\n"
		"  -D --debug         show everything and debug messages\n"
	);
}

void GetBaseOptions (const char *filename);
Window create_main_window();
void HandleEvents();
void do_colorscheme();
void do_change_param( int val );
void DispatchEvent (ASEvent * event);
void generate_dir_html();

char *default_html_save = "index.html";
char *default_html_save_dir = "html";

int main(int argc, char** argv)
{
	int i;

	/* Save our program name - for error messages */
    InitMyApp (CLASS_ASIMBROWSER, argc, argv, asimbrowser_usage, NULL, 0 );

	memset( &ASIMBrowserState, 0x00, sizeof(ASIMBrowserState ));
	ASIMBrowserState.html_save = default_html_save;
	ASIMBrowserState.html_save_dir = default_html_save_dir;
	ASIMBrowserState.display = 1;

	/* Parse command line. */
	for (i = 1 ; i < argc ; i++) {
		if( argv[i] == NULL )
			continue;
		if ((!strcmp(argv[i], "--output") || !strcmp(argv[i], "-o")) && i < argc + 1) {
			ASIMBrowserState.html_save = argv[++i];
		}else if ((!strcmp(argv[i], "--output-dir") || !strcmp(argv[i], "-O")) && i < argc + 1) {
			ASIMBrowserState.html_save_dir = argv[++i];
		} else if ((!strcmp(argv[i], "--dir") || !strcmp(argv[i], "-D")) && i < argc + 1) {
			ASIMBrowserState.dir = argv[++i];
		} else if (!strcmp(argv[i], "--no-display") || !strcmp(argv[i], "-n")) {
			ASIMBrowserState.display = 0;
		}
	}

	ConnectX( &Scr, PropertyChangeMask );
//    ConnectAfterStep ( 0 );
	InitSession();

	LoadBaseConfig (GetBaseOptions);

	if( !get_flags( ASIMBrowserState.geometry.flags, WidthValue ) )
		ASIMBrowserState.geometry.width = 640 ;
	if( !get_flags( ASIMBrowserState.geometry.flags, HeightValue ) )
		ASIMBrowserState.geometry.height = 480 ;
	if( !get_flags( ASIMBrowserState.geometry.flags, XValue ) )
		ASIMBrowserState.geometry.x = (Scr.MyDisplayWidth - ASIMBrowserState.geometry.width)/2 ;
	if( !get_flags( ASIMBrowserState.geometry.flags, YValue ) )
		ASIMBrowserState.geometry.y = (Scr.MyDisplayHeight - ASIMBrowserState.geometry.height)/2 ;

	if( ASIMBrowserState.display )
		ASIMBrowserState.main_window = create_main_window();

	if( ASIMBrowserState.display )
		HandleEvents();
	else
		generate_dir_html(ASIMBrowserState.dir, ASIMBrowserState.html_save_dir);
	return 0;
}

void
GetBaseOptions (const char *filename)
{
    START_TIME(started);

	ReloadASEnvironment( NULL, NULL, NULL, False );

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
			{
				int val = 10 ;
				if( (event->x.xkey.state&ControlMask) )
					val = 1 ;
				LOCAL_DEBUG_OUT( "keysym = 0x%lX", XLookupKeysym (&(event->x.xkey),0) );
				switch( XLookupKeysym (&(event->x.xkey), 0) )
				{
					case XK_a :
					    break ;
					case XK_h :
				    	break ;
					case XK_s :
					    break ;
					case XK_v :
					    break ;
					case XK_Left :
				    	break ;
					case XK_Right :
				    	break ;
				}
			}
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
    int width = ASIMBrowserState.geometry.width;
    int height = ASIMBrowserState.geometry.height;
    XSetWindowAttributes attr;

    LOCAL_DEBUG_OUT("configured geometry is %dx%d%+d%+d", width, height, ASIMBrowserState.geometry.x, ASIMBrowserState.geometry.y );
	switch( ASIMBrowserState.gravity )
	{
		case NorthEastGravity :
            x = Scr.MyDisplayWidth - width + ASIMBrowserState.geometry.x ;
            y = ASIMBrowserState.geometry.y ;
			break;
		case SouthEastGravity :
            x = Scr.MyDisplayWidth - width + ASIMBrowserState.geometry.x ;
            y = Scr.MyDisplayHeight - height + ASIMBrowserState.geometry.y ;
			break;
		case SouthWestGravity :
            x = ASIMBrowserState.geometry.x ;
            y = Scr.MyDisplayHeight - height + ASIMBrowserState.geometry.y ;
			break;
		case NorthWestGravity :
		default :
            x = ASIMBrowserState.geometry.x ;
            y = ASIMBrowserState.geometry.y ;
			break;
	}
    attr.event_mask = StructureNotifyMask|ButtonPressMask|KeyPressMask|ButtonReleaseMask|PointerMotionMask|PropertyChangeMask ;
    w = create_visual_window( Scr.asv, Scr.Root, x, y, width, height, 4, InputOutput, CWEventMask, &attr);
    set_client_names( w, MyName, MyName, CLASS_ASCOLOR, MyName );

    Scr.RootClipArea.x = x;
    Scr.RootClipArea.y = y;
    Scr.RootClipArea.width = width;
    Scr.RootClipArea.height = height;

    shints.flags = USSize|PMinSize|PResizeInc|PWinGravity;
    if( ASIMBrowserState.user_geometry )
        shints.flags |= USPosition ;
    else
        shints.flags |= PPosition ;

    shints.min_width = 640;
    shints.min_height = 480;
    shints.width_inc = 1;
    shints.height_inc = 1;
	shints.win_gravity = ASIMBrowserState.gravity ;

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


void generate_dir_html( char *dir, char *html_dir )
{
	FILE *of = stdout ;
	int count = 0;
	ASImageListEntry *im_list = get_asimage_list( Scr.asv, dir,
	              						     LOAD_PREVIEW, Scr.image_manager->gamma, 0, 0,
											 0, &count );

	if( im_list && count > 0 )
	{
		ASImageListEntry *curr = im_list ;
		int im_no = 0 ;
		if( ASIMBrowserState.html_save )
		{
			char *html_save = safemalloc( (html_dir?strlen( html_dir )+1:0)+ strlen(ASIMBrowserState.html_save)+1 );
			if( html_dir )
				sprintf( html_save, "%s/%s", html_dir, ASIMBrowserState.html_save );
			else
				strcpy( html_save, ASIMBrowserState.html_save);

			if( (of = fopen( html_save, "wb" )) == NULL )
			{
				show_error( "failed to open output file \"%s\"", html_save );
				exit(1);
			}
			free( html_save );
		}
		fprintf( of, "<HTML><HEAD><title>%s</title></head><body><h3>%s</h3>", dir, dir );
		while( curr )
		{
			if( mystrcasecmp(curr->name, "CREDITS") != 0)
			{
				int name_len = strlen( curr->name );
				int odir_name_len = html_dir?strlen( html_dir )+1:0;
				char *preview_name = safemalloc( odir_name_len + name_len + 1 + 3 + 1 );
				char *ext_name = safemalloc(odir_name_len + name_len + 1 + 4 + 1 );
				ASImageExportParams params ;
				if( odir_name_len > 0 )
				{
					sprintf( preview_name, "%s/%s.png", html_dir, curr->name );
					sprintf( ext_name, "%s/%s.html", html_dir, curr->name );
				}else
				{
					strcpy( preview_name, curr->name );
					strcpy( &(preview_name[name_len]), ".png" );
					strcpy( ext_name, curr->name );
					strcpy( &(ext_name[name_len]), ".html" );
				}
#if 1
				params.png.flags = EXPORT_ALPHA ;
				params.png.compression = 6 ;
				params.png.type = ASIT_Png ;
				if( !ASImage2file( curr->preview, NULL, preview_name,ASIT_Png, &params ) )
#else
				params.jpeg.flags = EXPORT_ALPHA ;
				params.jpeg.quality = 100 ;
				params.jpeg.type = ASIT_Jpeg ;
				if( !ASImage2file( curr->preview, NULL, preview_name,ASIT_Jpeg, &params ) )
#endif
				{
					show_warning( "failed to save \"%s\" as png preview - skipping", curr->name );
				}else
				{
					FILE *frame_of ;
					fprintf( of, "<A href=\"%s.html\"><IMG ALT=\"%s\" SRC=\"%s.png\"></A>\n", curr->name, curr->name, curr->name );

					frame_of = fopen( ext_name, "wb" );
					if( frame_of )
					{
						static char *img_type_names[ASIT_Unknown+1] =
						{
							"Xpm",
							"ZCompressedXpm",
							"GZCompressedXpm",
							"Png",
							"Jpeg",
							"Xcf",
							"Ppm",
							"Pnm",
							"Bmp",
							"Ico",
							"Cur",
							"Gif",
							"Tiff",
							"XMLScript",
							"Xbm",
							"Targa",
							"Pcx",
							"Unknown"
						};
						fprintf( frame_of, "<HTML>\n<HEAD><TITLE>%s</TITLE></HEAD>\n<BODY>\n", curr->name );
						fprintf( frame_of, "<h3>%s</h3>\n", curr->name );
						fprintf( frame_of, "<h4>Size :</h4>&nbsp;&nbsp;&nbsp;&nbsp;%dx%d\n",
						     	curr->preview?curr->preview->width:0, curr->preview?curr->preview->height:0 );
						fprintf( frame_of, "<h4>Full path :</h4>&nbsp;&nbsp;&nbsp;&nbsp;%s\n", curr->fullfilename );
						fprintf( frame_of, "<h4>Preview :</h4>&nbsp;&nbsp;&nbsp;&nbsp;<IMG ALT=\"%s\" SRC=\"%s.png\">\n", curr->name, curr->name );
						fprintf( frame_of, "<h4>Type :</h4>&nbsp;&nbsp;&nbsp;&nbsp;%s\n", img_type_names[curr->type] );
						if( curr->type == ASIT_XMLScript )
						{
							FILE *xml_of = fopen( curr->fullfilename, "rb" );
							int c;
							fprintf( frame_of, "<h4>XML text :</h4><PRE>\n" );
							if( xml_of )
							{
	   							while( (c = fgetc(xml_of)) != EOF )
								{
									if( c == '<' )
										fprintf( frame_of, "&lt;" );
									else if( c == '>' )
										fprintf( frame_of, "&gt;" );
									else
										fputc( c, frame_of );
								}
								fclose( xml_of );
							}else
								fprintf( frame_of, "error: failed to open xml file!\n" );
							fprintf( frame_of, "</PRE>\n" );
						}
						fprintf( frame_of, "\n</body></HTML>\n");
						fclose( frame_of );
					}
				}
				free( preview_name );
				free( ext_name );
			}
			curr = curr->next ;
			++im_no ;
		}
		curr = im_list ;
		while( curr )
		{
			if( mystrcasecmp(curr->name, "CREDITS") == 0)
			{
				FILE *credits_of = fopen( curr->fullfilename, "rb" );
				int c;
				fprintf( of, "<h4>CREDITS :</h4><PRE>\n" );
				if( credits_of )
				{
					while( (c = fgetc(credits_of)) != EOF )
					{
						if( c == '<' )
							fprintf( of, "&lt;" );
						else if( c == '>' )
							fprintf( of, "&gt;" );
						else
							fputc( c, of );
					}
					fclose( credits_of );
				}else
					fprintf( of, "error: failed to open CREDITS file!\n" );
				fprintf( of, "</PRE>\n" );
			}
			curr = curr->next ;
		}
		fprintf( of, "\n</body></HTML>");

		if( of != stdout )
			fclose( of );
	}
}
