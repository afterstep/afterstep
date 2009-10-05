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
#include "../../libAfterStep/session.h"

#include <X11/keysym.h>

/* <li><b><A href="graphics.php?inc=icons/normal/index">normal</A></b></li> */

struct
{
	Window main_window ;
	char *dir ;
	char *html_save, *html_save_dir ;
	char *html_back  ;
	char *link_format ;
	Bool display ;
	ASGeometry geometry ;
	Bool user_geometry ;
	int gravity ;
}ASIMBrowserState;

void asimbrowser_usage()
{
	fprintf( stdout,
		"Usage:\n"
		"asimbrowse [-h] [-f file] [-o file] [-s string] [-t type] [-v] [-V] [-l format]"
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
		"  -l --link-format   Set C format string to be used as a template for URLs\n"
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
char *default_html_back = "background.jpg";

int main(int argc, char** argv)
{
	int i;

	/* Save our program name - for error messages */
    InitMyApp (CLASS_ASIMBROWSER, argc, argv, NULL, asimbrowser_usage, 0 );

	memset( &ASIMBrowserState, 0x00, sizeof(ASIMBrowserState ));
	ASIMBrowserState.html_save = default_html_save;
	ASIMBrowserState.html_save_dir = default_html_save_dir;
	ASIMBrowserState.html_back = default_html_back;
	ASIMBrowserState.display = 1;

	/* Parse command line. */
	for (i = 1 ; i < argc ; i++) {
		if( argv[i] == NULL )
			continue;
		if ((!strcmp(argv[i], "--output") || !strcmp(argv[i], "-o")) && i < argc + 1) {
			ASIMBrowserState.html_save = argv[++i];
		}else if ((!strcmp(argv[i], "--background") || !strcmp(argv[i], "-b")) && i < argc + 1) {
			ASIMBrowserState.html_back = argv[++i];
		}else if ((!strcmp(argv[i], "--output-dir") || !strcmp(argv[i], "-O")) && i < argc + 1) {
			ASIMBrowserState.html_save_dir = argv[++i];
		} else if ((!strcmp(argv[i], "--dir") || !strcmp(argv[i], "-D")) && i < argc + 1) {
			ASIMBrowserState.dir = argv[++i];
		} else if (!strcmp(argv[i], "--no-display") || !strcmp(argv[i], "-n")) {
			ASIMBrowserState.display = 0;
		} else if((!strcmp(argv[i], "--link-format") || !strcmp(argv[i], "-l")) && i < argc + 1){
			ASIMBrowserState.link_format = argv[++i];
		}
	}

	ConnectX( ASDefaultScr, 0 );
/*    ConnectAfterStep ( 0 ); */
	InitSession();

	LoadBaseConfig (GetBaseOptions);
	LoadColorScheme();

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
	if( ASIMBrowserState.link_format != NULL ) 
	{
		int i = 0;
		char *ptr = ASIMBrowserState.link_format;
		do
		{
			ptr = strchr( ptr, '%' );	
			if( ptr )
			{
				++ptr ;
				++i ;
			}
		}while( ptr != NULL );	
		if( i > 2 ) 
			ASIMBrowserState.link_format = NULL ;	 
	}
	if( ASIMBrowserState.link_format == NULL ) 
		ASIMBrowserState.link_format = "<li><b><A href=\"%s/%s\">%s</A></b></li>"; 


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

	ReloadASEnvironment( NULL, NULL, NULL, False, False );

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
	set_client_cmd (w);

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
	ASImageListEntry *curr ;
	int im_no = 0 ;
	struct direntry  **list;
	int list_len, i ;
	Bool dir_header_printed = False ;


	ASImageListEntry *im_list = get_asimage_list( Scr.asv, dir,
	              						     LOAD_PREVIEW, Scr.image_manager->gamma, 0, 0,
											 0, &count, ignore_dots );

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
	fprintf( of, "<HTML><HEAD><title>%s</title></head>", dir );
	fprintf( of, "<body BACKGROUND=\"%s\" BGCOLOR=#34404C TEXT=#F5F8FAA LINK=#8888FF VLINK=#88FF88>", ASIMBrowserState.html_back );
	fprintf( of, "<font color=#F5F8FA><h2>%s</h2>", dir );
	fprintf( of, "<h4>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<A href=\"../%s\">Go Back</A></h4>\n", ASIMBrowserState.html_save);

	list_len = my_scandir ((char*)dir, &list, ignore_dots, NULL);
	for (i = 0; i < list_len; i++)
		if (S_ISDIR (list[i]->d_mode))
		{
			char * subdir = make_file_name( dir, list[i]->d_name );
			char * html_subdir = html_dir?make_file_name( html_dir, list[i]->d_name ):mystrdup(list[i]->d_name);
			char * back_src = make_file_name( html_dir?html_dir:"./", ASIMBrowserState.html_back );
			char * back_dst = make_file_name( html_subdir, ASIMBrowserState.html_back );

			if( !dir_header_printed )
			{
				fprintf( of, "<h3>Subdirectories : </h3>\n<ul>\n");
				dir_header_printed = True ;
			}
			fprintf( of, "<li><b><A href=\"%s/%s\">%s</A></b></li>\n", list[i]->d_name, ASIMBrowserState.html_save, list[i]->d_name);
			CheckOrCreate (html_subdir);
			copy_file( back_src, back_dst );
			free( back_src );
			free( back_dst );
			generate_dir_html( subdir, html_subdir );
			free( subdir );
			free( html_subdir );
		}
	if( dir_header_printed ) 
		fprintf( of, "</ul>\n");
	curr = im_list ;
	while( curr )
	{
		if( mystrcasecmp(curr->name, "CREDITS") != 0 )
		{
			int name_len = strlen( curr->name );
			int odir_name_len = html_dir?strlen( html_dir )+1:0;
			char *preview_name = safemalloc( odir_name_len + name_len + 1 + 3 + 1 );
			char *ext_name = safemalloc(odir_name_len + name_len + 1 + 4 + 1 );
			Bool preview_saved = False ;
			FILE *frame_of ;

			if( odir_name_len > 0 )
			{
				sprintf( preview_name, "%s/%s.png", html_dir, curr->name );
				sprintf( ext_name, "%s/%s.html", html_dir, curr->name );
			}else
			{
				strcpy( preview_name, curr->name );
				strcpy( ext_name, curr->name );
				strcpy( &(ext_name[name_len]), ".html" );
				strcpy( &(preview_name[name_len]), ".png" );
			}
			if( curr->preview )
			{
				ASImageExportParams params ;
				if( curr->type != ASIT_Jpeg &&
					get_flags( get_asimage_chanmask(curr->preview), SCL_DO_ALPHA) &&
					curr->preview->width < 200 && curr->preview->height < 200 )
				{
					params.png.flags = EXPORT_ALPHA ;
					params.png.compression = 6 ;
					params.png.type = ASIT_Png ;
					preview_saved = ASImage2file( curr->preview, NULL, preview_name,ASIT_Png, &params );
				}else
				{
					params.jpeg.flags = EXPORT_ALPHA ;
					params.jpeg.quality = 100 ;
					params.jpeg.type = ASIT_Jpeg ;
					preview_saved = ASImage2file( curr->preview, NULL, preview_name,ASIT_Jpeg, &params );
				}

				if( !preview_saved )
					show_warning( "failed to save \"%s\" as png preview - skipping", curr->name );
			}
			if( preview_saved )
				fprintf( of, "<A href=\"%s.html\"><IMG ALT=\"%s\" SRC=\"%s.png\"></A>\n", curr->name, curr->name, curr->name );
			else
				fprintf( of, "<br>&nbsp;&nbsp;&nbsp;&nbsp;<A href=\"%s.html\">%s</A><br>\n", curr->name, curr->name );

			frame_of = fopen( ext_name, "wb" );
			if( frame_of )
			{
				Bool valid_html = False ;
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
					"XML",
					"Xbm",
					"Targa",
					"Pcx",
					"Unknown"
				};
				fprintf( frame_of, "<HTML>\n<HEAD><TITLE>%s</TITLE></HEAD>\n", curr->name );
				fprintf( frame_of, "<BODY BACKGROUND=\"%s\" BGCOLOR=#34404C TEXT=#F5F8FA LINK=#8888FF VLINK=#88FF88>\n", ASIMBrowserState.html_back );
				fprintf( frame_of, "<h3>%s</h3>\n", curr->name );
				fprintf( frame_of, "<h4>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<A href=\"%s\">Go Back</A></h4>", ASIMBrowserState.html_save);

				if( curr->preview )
					fprintf( frame_of, "<h4>Size :</h4>&nbsp;&nbsp;&nbsp;&nbsp;%dx%d\n", curr->preview->width, curr->preview->height );

				fprintf( frame_of, "<h4>Full path :</h4>&nbsp;&nbsp;&nbsp;&nbsp;%s\n", curr->fullfilename );
				if( preview_saved )
					fprintf( frame_of, "<h4>Preview :</h4>&nbsp;&nbsp;&nbsp;&nbsp;<IMG border=3 ALT=\"%s\" SRC=\"%s.png\">\n", curr->name, curr->name );
				fprintf( frame_of, "<h4>Type :</h4>&nbsp;&nbsp;&nbsp;&nbsp;%s\n", img_type_names[curr->type] );
				if( curr->type == ASIT_XMLScript || !preview_saved )
				{
					FILE *xml_of = fopen( curr->fullfilename, "rb" );
					int c;
					if( curr->type == ASIT_XMLScript )
					{
						if( curr->preview )
							fprintf( frame_of, "<h4>XML text :</h4><PRE>\n" );
						else
						{

							if( xml_of )
							{
								int body_count = -1 ;
								while( (c = fgetc(xml_of)) != EOF && body_count <= 4 )
								{
									if( c == '<' && body_count == -1 )
										++body_count;
									else if( (c == 'b' || c == 'B') && body_count == 0 )
										++body_count;
									else if( (c == 'o' || c == 'O') && body_count == 1 )
										++body_count;
									else if( (c == 'd' || c == 'D') && body_count == 2 )
										++body_count;
									else if( (c == 'y' || c == 'Y') && body_count == 3 )
										++body_count;
									else if(  c == '>' && body_count == 4 )
										++body_count;
									else if( !isspace( c ) || (body_count > 0 && body_count < 4) )
									{
										body_count = -1 ;
									}
								}
								if( body_count == 5 )
								{
									valid_html = True ;
									fprintf( frame_of, "<h4>HTML document :</h4>\n" );
								}else
								{
									fprintf( frame_of, "<h4>XML text :</h4><PRE>\n" );
									fseek( xml_of, 0, SEEK_SET );
								}
							}
						}
					}else
						fprintf( frame_of, "<h4>ASCII text :</h4><PRE>\n" );
					if( xml_of )
					{

	   					while( (c = fgetc(xml_of)) != EOF )
						{
							if( valid_html )
								fputc( c, frame_of );
							else if( !isascii( c ) )
								fprintf( frame_of, "#%2.2X;", c );
							else if( c == '<' )
								fprintf( frame_of, "&lt;" );
							else if( c == '>' )
								fprintf( frame_of, "&gt;" );
							else
								fputc( c, frame_of );
						}
						fclose( xml_of );
					}else
						fprintf( frame_of, "error: failed to open xml file!\n" );
					if( !valid_html )
						fprintf( frame_of, "</PRE>\n" );
				}
				if( !valid_html )
					fprintf( frame_of, "\n</body></HTML>\n");
				fclose( frame_of );
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
