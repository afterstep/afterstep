/*
 * This is the complete from scratch rewrite of the original WinList 
 * module.
 *
 * Copyright (C) 2002 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
/*#define DO_CLOCKING      */

#include "../../configure.h"

#include <signal.h>

#ifdef I18N
#include <X11/Xlocale.h>
#endif

#define IN_MODULE
#define MODULE_X_INTERFACE

#include "../../include/afterbase.h"
#include "../../libAfterImage/afterimage.h"
#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/mystyle.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"
#include "../../include/clientprops.h"
#include "../../include/wmprops.h"

struct ASWindowData;
#define WINDOW_PACKET_MASK 0xFFFFFFFF

/**********************************************************************/
/*  AfterStep specific global variables :                             */
/**********************************************************************/
char *MyName;           /* our executable name                        */
ScreenInfo Scr;			/* AS compatible screen information structure */
/**********************************************************************/

/**********************************************************************/
/* Our configuration options :                                        */
/**********************************************************************/
BaseConfig *Base = NULL;
/**********************************************************************/

void
usage (void)
{
  printf ("Usage:\n"
  	      "%s [-d|--display display_name] [-v|--version] [-h|--help]\n", MyName);
  exit (0);
}

void GetBaseOptions (const char *filename);
void GetOptions (const char *filename);
void process_message (unsigned long type, unsigned long *body);
void DispatchEvent (XEvent * Event);

int 
main( int argc, char **argv )
{
	int i ;
	char *global_config_file = NULL;
	int x_fd ;
	int as_fd[2] ;
	
	set_application_name(argv[0]);
	SetMyName (argv[0]);
	
	set_signal_handler( SIGSEGV );
	set_output_threshold( OUTPUT_LEVEL_PROGRESS );

	i = ProcessModuleArgs (argc, argv, &global_config_file, NULL, NULL, usage);
	
#ifdef I18N
	if (setlocale (LC_ALL, AFTER_LOCALE) == NULL)
  		show_error("cannot set locale.");
#endif

	x_fd = ConnectX (&Scr, display_name, PropertyChangeMask);
	if( !x_fd ) 
		return 1 ;

	as_fd[0] = as_fd[1] = 
	        ConnectAfterStep (M_ADD_WINDOW |
		  					  M_CONFIGURE_WINDOW |
							  M_DESTROY_WINDOW |
							  M_FOCUS_CHANGE |
							  M_ICONIFY |
							  M_ICON_LOCATION |
							  M_DEICONIFY |
							  M_ICON_NAME |
							  M_WINDOW_NAME |
							  M_END_WINDOWLIST);
	if( !as_fd[0] ) 
		return 1 ;

	intern_hint_atoms ();
	intern_wmprop_atoms ();

	/* Request a list of all windows, while we load our config */
    SendInfo (as_fd, "Send_WindowList", 0);
	
	LoadBaseConfig (global_config_file, GetBaseOptions);
    LoadConfig (global_config_file, "pager", GetOptions);

	/* And at long last our main loop : */
	while (1)
    {
        XEvent event;
		if (!My_XNextEvent (dpy, x_fd, as_fd[0], process_message, &event))
  			timer_handle ();	/* handle timeout events */
		else
  		{
    		balloon_handle_event (&event);
    		DispatchEvent (&event);
  		}
    }

	return 0 ;
}

void
DeadPipe (int nonsense)
{
#ifdef DEBUG_ALLOCS
	int i;
/* normally, we let the system clean up, but when auditing time comes
 * around, it's best to have the books in order... */
    balloon_init (1);
	if( Base ) 
		DestroyBaseConfig(&Base);

    {
    	GC foreGC, backGC, reliefGC, shadowGC;

    	mystyle_get_global_gcs (mystyle_first, &foreGC, &backGC, &reliefGC, &shadowGC);
    	while (mystyle_first)
			mystyle_delete (mystyle_first);
    	XFreeGC (dpy, foreGC);
    	XFreeGC (dpy, backGC);
    	XFreeGC (dpy, reliefGC);
    	XFreeGC (dpy, shadowGC);
    }

    print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

    XFlush (dpy);			/* need this for SetErootPixmap to take effect */
	XCloseDisplay (dpy);		/* need this for SetErootPixmap to take effect */
    exit (0);
}


void
GetBaseOptions (const char *filename)
{
    BaseConfig *config = ParseBaseOptions (filename, MyName);
	
    if (!config)
	    exit (0);			/* something terrible has happend */

    if( Base!= NULL ) 
		DestroyBaseConfig (Base);
	Base = config ;
	
	Scr.image_manager = create_image_manager( NULL, 2.2, Base->pixmap_path, Base->icon_path, NULL );
//	Scr.font_manager = create_font_manager( NULL, NULL );
}

void
GetOptions (const char *filename)
{

	/* TODO: implement some options here : */
}

/****************************************************************************/
/* PROCESSING OF AFTERSTEP MESSAGES :                                       */
/****************************************************************************/
void
process_message (unsigned long type, unsigned long *body)
{
	if( (type&WINDOW_PACKET_MASK) != 0 )  
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );
		show_progress( "message %X window %X data %p", type, body[0], wd );
		handle_window_packet( type, body, &wd );
	}
}

void
DispatchEvent (XEvent * Event)
{
    switch (Event->type)
    {
	    case ConfigureNotify:
	        break;
	    case Expose:
	        break;
	    case ButtonPress:
	    case ButtonRelease:
			break;
	    case ClientMessage:
    	    if ((Event->xclient.format == 32) &&
		  	    (Event->xclient.data.l[0] == _XA_WM_DELETE_WINDOW))
			{
			    exit (0);
			}
	        break;
	    case PropertyNotify:
			break;
    }
}
