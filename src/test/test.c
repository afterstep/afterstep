/*
 * Copyright (C) 2003 Sasha Vasko <sasha at aftercode.net>
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
#define LOCAL_DEBUG
#define EVENT_TRACE

#include "../../configure.h"

#include "../../libAfterStep/asapp.h"

#include <signal.h>
#include <unistd.h>

#include "../../libAfterImage/afterimage.h"

#include "../../libAfterStep/afterstep.h"
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/mystyle.h"
#include "../../libAfterStep/mystyle_property.h"
#include "../../libAfterStep/parser.h"
#include "../../libAfterStep/clientprops.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterStep/decor.h"
#include "../../libAfterStep/aswindata.h"
#include "../../libAfterStep/balloon.h"
#include "../../libAfterStep/event.h"

#include "../../libAfterConf/afterconf.h"

/**********************************************************************/
/*  AfterStep specific global variables :                             */
/**********************************************************************/
typedef struct {
	Window main_window ;
	ASCanvas *main_canvas ;
}ASTestState ;

ASTestState TestState = { 0 };

/**********************************************************************/
/**********************************************************************/
/* Our configuration options :                                        */
/**********************************************************************/

void DoTest()
{
	char *name ;
	int i ;
	unsigned int c = 192 ;
	XDeleteProperty (dpy, TestState.main_window, _XA_NET_WM_NAME);
	XDeleteProperty (dpy, TestState.main_window, _XA_NET_WM_ICON_NAME);
	name = safemalloc( 128+1 );
	for ( i = 0 ; i < 128 && c < 255 ; ++i)
	{
		name[i] = (char)c ;
		++c ;
	}
	name[i] = '\0' ;

	set_text_property (TestState.main_window, XA_WM_NAME, &name, 1, TPE_String);
	set_text_property (TestState.main_window, XA_WM_ICON_NAME, &name, 1, TPE_String);
	free( name );

}
/**********************************************************************/

void GetBaseOptions (const char *filename);
void HandleEvents();
void DispatchEvent (ASEvent * Event);
Window make_test_window();
void process_message (unsigned long type, unsigned long *body);

int
main( int argc, char **argv )
{
    /* Save our program name - for error messages */
    InitMyApp (CLASS_TEST, argc, argv, NULL, NULL, 0 );

    set_signal_handler( SIGSEGV );

    ConnectX( &Scr, 0 );
    ConnectAfterStep (0);
    balloon_init (False);

    LoadBaseConfig ( GetBaseOptions);
	LoadColorScheme();

    TestState.main_window = make_test_window();
    TestState.main_canvas = create_ascanvas( TestState.main_window );
    set_root_clip_area(TestState.main_canvas );

	DoTest();

	/* And at long last our main loop : */
    HandleEvents();
	return 0 ;
}

void HandleEvents()
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
        module_wait_pipes_input ( process_message );
    }
}

void
DeadPipe (int nonsense)
{
    FreeMyAppResources();

    if( TestState.main_canvas )
        destroy_ascanvas( &TestState.main_canvas );
    if( TestState.main_window )
        XDestroyWindow( dpy, TestState.main_window );

#ifdef DEBUG_ALLOCS
    print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

    XFlush (dpy);			/* need this for SetErootPixmap to take effect */
	XCloseDisplay (dpy);		/* need this for SetErootPixmap to take effect */
    exit (0);
}

void
GetBaseOptions (const char *filename)
{
	ReloadASEnvironment( NULL, NULL, NULL, False );
}

/****************************************************************************/
/* PROCESSING OF AFTERSTEP MESSAGES :                                       */
/****************************************************************************/
void
process_message (unsigned long type, unsigned long *body)
{
    LOCAL_DEBUG_OUT( "received message %lX", type );
}

void
DispatchEvent (ASEvent * event)
{
    ASWindowData *pointer_wd = NULL ;

    SHOW_EVENT_TRACE(event);

    if( (event->eclass & ASE_POINTER_EVENTS) != 0 )
    {
    }

    switch (event->x.type)
    {
	    case ConfigureNotify:
            {
                ASFlagType changes = handle_canvas_config( TestState.main_canvas );
                if( changes != 0 )
                    set_root_clip_area( TestState.main_canvas );

                if( get_flags( changes, CANVAS_RESIZED ) )
				{
                }else if( changes != 0 )        /* moved - update transparency ! */
				{
				}
            }
	        break;
        case ButtonPress:
            break;
        case ButtonRelease:
			break;
        case EnterNotify :
			if( event->x.xcrossing.window == Scr.Root )
			{
				withdraw_active_balloon();
				break;
			}
        case LeaveNotify :
        case MotionNotify :
            break ;
	    case ClientMessage:
            if ((event->x.xclient.format == 32) &&
                (event->x.xclient.data.l[0] == _XA_WM_DELETE_WINDOW))
			{
                DeadPipe(0);
			}
	        break;
	    case PropertyNotify:
			LOCAL_DEBUG_OUT( "property %s(%lX), _XROOTPMAP_ID = %lX, event->w = %lX, root = %lX", XGetAtomName(dpy, event->x.xproperty.atom), event->x.xproperty.atom, _XROOTPMAP_ID, event->w, Scr.Root );
            if( event->x.xproperty.atom == _XROOTPMAP_ID && event->w == Scr.Root )
            {
                LOCAL_DEBUG_OUT( "root background updated!%s","");
                safe_asimage_destroy( Scr.RootImage );
                Scr.RootImage = NULL ;
            }else if( event->x.xproperty.atom == _AS_STYLE )
			{
				LOCAL_DEBUG_OUT( "AS Styles updated!%s","");
				handle_wmprop_event (Scr.wmprops, &(event->x));
				mystyle_list_destroy_all(&(Scr.Look.styles_list));
				LoadColorScheme();
				/* now we need to update everything */
			}
			break;
    }
}

/********************************************************************/
/* showing our main window :                                        */
/********************************************************************/
Window
make_test_window()
{
	Window        w;
	XSizeHints    shints;
	ExtendedWMHints extwm_hints ;
	int x, y ;
    unsigned int width = 300;
    unsigned int height = 300;

	x = (Scr.MyDisplayWidth-width)/2;
	y = (Scr.MyDisplayHeight-height)/2;

	w = create_visual_window( Scr.asv, Scr.Root, x, y, width, height, 0, InputOutput, 0, NULL);

	set_client_names( w, "Test", "TEST", CLASS_TEST, MyName );

	shints.flags = USPosition|USSize|PMinSize|PMaxSize|PBaseSize|PWinGravity;
	shints.min_width = shints.min_height = 4;
	shints.base_width = shints.base_height = 4;
	shints.win_gravity = NorthEastGravity ;

	extwm_hints.pid = getpid();
	extwm_hints.flags = EXTWM_PID ;

	set_client_hints( w, NULL, &shints, AS_DoesWmDeleteWindow, &extwm_hints );

	/* showing window to let user see that we are doing something */
	XMapRaised (dpy, w);
	/* final cleanup */
	XFlush (dpy);
	sleep (1);								   /* we have to give AS a chance to spot us */
	/* we will need to wait for PropertyNotify event indicating transition
	   into Withdrawn state, so selecting event mask: */
    XSelectInput (dpy, w, PropertyChangeMask|StructureNotifyMask|
                          ButtonPressMask|ButtonReleaseMask|PointerMotionMask|
                          KeyPressMask|KeyReleaseMask|
                          EnterWindowMask|LeaveWindowMask);

	return w ;
}



