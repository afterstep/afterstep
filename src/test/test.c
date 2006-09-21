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

void DoTest_locale()
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

void DoTest_Title()
{
	int i ;
#if 0	
	char charset[] = "`1234567890-=~!@#$%^&*()_+qwertyuiop[]sdfghjkl;'/.,mnbvcxzQWERTYUIOP{}|:LKJHGFDSAZXCVBNM<>?";
	char name[32];
	char res_name[256];
	char res_class[256];
	char icon_name[256];
	static int num = 12345;


	while(1)
	{
		int seed = time(NULL)+num;
		for( i = 0 ; i < sizeof(name) ; ++i ) 
			name[i] = charset[(seed+i)%(sizeof(charset)-1)];
		name[i-1] = '\0' ;
		sprintf( &res_name[0], "res_name :%s", &name[0] );
		sprintf( &res_class[0], "res_class :%s", &name[0] );
		sprintf( &icon_name[0], "icon_name :%s", &name[0] );				
		set_client_names (TestState.main_window, &name[0], &icon_name[0], &res_class[0], &res_name[0]);
		ASSync(False);
	}
#endif
	set_client_names (TestState.main_window, "test_no_icon", "test_no_icon", "test_no_icon", "test_no_icon");
	ASSync(False);
	sleep_a_millisec( 100 );
	set_client_names (TestState.main_window, "test_has_icon", "test_has_icon", "test_has_icon", "test_has_icon");
	ASSync(False);
	sleep_a_millisec( 100 );
	set_client_names (TestState.main_window, "test_no_icon", "test_no_icon", "test_no_icon", "test_no_icon");
	ASSync(False);

}


#define uint16_t CARD16

void DoTest_colorscheme()
{
    CARD16 red = 0, green = 0, blue = 0 ;
    FILE *stream = fopen ("ascs.html", "w" );


    fprintf( stream, "<HTML><BODY>\n" );
    while( red < 257 ) 
    {    
        if( red == 256 ) 
            --red ;
        green = 0 ;
        while( green < 257 ) 
        {
            if( green == 256 ) 
                --green ;
            blue = 0 ;
            while( blue < 257 ) 
            {
                ARGB32 base_fore = ARGB32_White;
                if( blue == 256 ) 
                    --blue ;
                ASColorScheme *acs = make_ascolor_scheme( MAKE_ARGB32(0xFF, red, green, blue), 15 );
#define COLOR_RGB(c)   (CARD16)ARGB32_RED8(acs->main_colors[c]), (CARD16)ARGB32_GREEN8(acs->main_colors[c]), (CARD16)ARGB32_BLUE8(acs->main_colors[c]) 

                fprintf( stream, "<TABLE BGCOLOR=\"#%2.2X%2.2X%2.2X\" CELLPADDING=5 CELLSPACING=10>", COLOR_RGB(ASMC_Base ) );
                if( ASCS_BLACK_O_WHITE_CRITERIA16(ARGB32_RED16(acs->main_colors[ASMC_Base]), ARGB32_GREEN16(acs->main_colors[ASMC_Base]), (uint16_t)ARGB32_BLUE16(acs->main_colors[ASMC_Base])) ) 
                    base_fore = ARGB32_Black ; 
                fprintf( stream, "<TR><TD ROWSPAN=3><font color=\"#%2.2X%2.2X%2.2X\">Base: #%2.2X%2.2X%2.2X, BaseFore: #%2.2X%2.2X%2.2X</font></TD>", 
                                 (uint16_t)ARGB32_RED8(base_fore), (uint16_t)ARGB32_GREEN8(base_fore), (uint16_t)ARGB32_BLUE8(base_fore),                                  
                                 COLOR_RGB(ASMC_Base), 
                                 (uint16_t)ARGB32_RED8(base_fore), (uint16_t)ARGB32_GREEN8(base_fore), (uint16_t)ARGB32_BLUE8(base_fore));
                fprintf( stream, "<TD BGCOLOR=\"#%2.2X%2.2X%2.2X\"><font color=\"#%2.2X%2.2X%2.2X\">Inactive1: #%2.2X%2.2X%2.2X, InactiveFore1:#%2.2X%2.2X%2.2X</font></TD></TR>", 
                                 COLOR_RGB(ASMC_Inactive1),
                                 COLOR_RGB(ASMC_InactiveText1),
                                 COLOR_RGB(ASMC_Inactive1), 
                                 COLOR_RGB(ASMC_InactiveText1) );
                fprintf( stream, "<TR><TD BGCOLOR=\"#%2.2X%2.2X%2.2X\"><font color=\"#%2.2X%2.2X%2.2X\">Active: #%2.2X%2.2X%2.2X, ActiveFore:#%2.2X%2.2X%2.2X</font></TD></TR>", 
                                 COLOR_RGB(ASMC_Active),
                                 COLOR_RGB(ASMC_ActiveText),
                                 COLOR_RGB(ASMC_Active),
                                 COLOR_RGB(ASMC_ActiveText) );
                fprintf( stream, "<TR><TD BGCOLOR=\"#%2.2X%2.2X%2.2X\"><font color=\"#%2.2X%2.2X%2.2X\">Inactive2: #%2.2X%2.2X%2.2X, InactiveFore2:#%2.2X%2.2X%2.2X</font></TD></TR>", 
                                 COLOR_RGB(ASMC_Inactive2),
                                 COLOR_RGB(ASMC_InactiveText2),
                                 COLOR_RGB(ASMC_Inactive2),
                                 COLOR_RGB(ASMC_InactiveText2) );
                            
                fprintf( stream, "</TABLE>\n<p>\n" );
                blue += 64 ; 
            }
            green += 64 ; 
        }    
        red += 64 ;    
    }
    fprintf( stream, "</BODY></HTML>\n" );
    fclose( stream );
    exit (0);
}    


/**********************************************************************/

void GetBaseOptions (const char *filename);
void HandleEvents();
void DispatchEvent (ASEvent * Event);
Window make_test_window();
void process_message (unsigned long type, unsigned long *body);
void DeadPipe(int);

int
main( int argc, char **argv )
{
    /* Save our program name - for error messages */
	set_DeadPipe_handler(DeadPipe);
    InitMyApp (CLASS_TEST, argc, argv, NULL, NULL, 0 );

    set_signal_handler( SIGSEGV );

    ConnectX( ASDefaultScr, 0 );
    ConnectAfterStep (0, 0);

    LoadBaseConfig ( GetBaseOptions);
	LoadColorScheme();

    TestState.main_window = make_test_window();
    TestState.main_canvas = create_ascanvas( TestState.main_window );
    set_root_clip_area(TestState.main_canvas );

    //DoTest_Title();

    //DoTest_colorscheme();

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
	ReloadASEnvironment( NULL, NULL, NULL, False, False );
}

/****************************************************************************/
/* PROCESSING OF AFTERSTEP MESSAGES :                                       */
/****************************************************************************/
void
process_message (unsigned long type, unsigned long *body)
{
    LOCAL_DEBUG_OUT( "received message %lX", type );
}

void toggle_urgency_hint()
{
	static Bool urgency_is_on = True ;
	XWMHints 	  hints;
	
	hints.flags = urgency_is_on?UrgencyHint:0 ;
	XSetWindowBackground( dpy, TestState.main_window, urgency_is_on?0xFFFF0000:0xFFFFFFFF);
	XClearWindow( dpy, TestState.main_window );

	urgency_is_on = !urgency_is_on ;

	XSetWMHints (dpy, TestState.main_window, &hints);
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
        case ButtonPress: toggle_urgency_hint(); 
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
            if( event->x.xproperty.atom == _AS_BACKGROUND )
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
	XWMHints 	  hints;
	XSizeHints    shints;
	ExtendedWMHints extwm_hints ;
	int x, y ;
    unsigned int width = 300;
    unsigned int height = 300;
	XSetWindowAttributes attr ;

	x = (Scr.MyDisplayWidth-width)/2;
	y = (Scr.MyDisplayHeight-height)/2;

	attr.background_pixel = Scr.asv->black_pixel ;

	w = create_visual_window( Scr.asv, Scr.Root, x, y, width, height, 0, InputOutput, CWBackPixel, &attr);

	set_client_names( w, "Test", "TEST", CLASS_TEST, MyName );

	hints.flags = UrgencyHint ;

	shints.flags = USPosition|USSize|PMinSize|PBaseSize|PWinGravity;
	shints.min_width = shints.min_height = 64;
	shints.base_width = shints.base_height = 64;
	shints.win_gravity = NorthEastGravity ;

	extwm_hints.pid = getpid();
	extwm_hints.flags = EXTWM_PID;//|EXTWM_StateSet ;

	extwm_hints.state_flags = 0; //EXTWM_StateDemandsAttention ;

	set_client_hints( w, NULL, &shints, AS_DoesWmDeleteWindow, &extwm_hints );
	set_client_cmd (w);

	/* showing window to let user see that we are doing something */
	XMapRaised (dpy, w);
	/* final cleanup */
	XFlush (dpy);
	sleep (1);								   /* we have to give AS a chance to spot us */
	/* we will need to wait for PropertyNotify event indicating transition
	   into Withdrawn state, so selecting event mask: */
	  XSelectInput (dpy, w, ButtonPressMask|ButtonReleaseMask);
      
/*    XSelectInput (dpy, w, PropertyChangeMask|StructureNotifyMask|
                          ButtonPressMask|ButtonReleaseMask|PointerMotionMask|
                          KeyPressMask|KeyReleaseMask|
                          EnterWindowMask|LeaveWindowMask);

	*/
	return w ;
}



