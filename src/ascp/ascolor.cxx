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

#include "asdocview.h"

#include <X11/keysym.h>

struct ASDoc* create_ascolorscheme_doc();
Bool open_ascolorscheme_file( struct ASDoc *doc, const char *filename );    
Bool save_ascolorscheme_file( struct ASDoc *doc, const char *filename );    
Bool check_ascolorscheme_sanity( struct ASDoc *doc );    
void destroy_ascolorscheme_doc( struct ASDoc **pdoc );


ASDocType ASColorschemeDocType = 
{
#define DOC_TYPE_NAME_ASColorscheme    "AfterStep Colorscheme"
    DOC_TYPE_NAME_ASColorscheme,
    0,
    
    create_ascolorscheme_doc,
    open_ascolorscheme_file,   
    save_ascolorscheme_file,    
    check_ascolorscheme_sanity,
    destroy_ascolorscheme_doc 
};    

typedef struct ASColorschemeDocData
{ /* colorscheme doc specific stuff : */
    unsigned long magic ; 
    ASColorScheme *cs ;

}ASColorschemeDoc;    


/*************************************************************************/
/* views                                                                 */
/*************************************************************************/
struct ASView* create_ascolorscheme_xml_view( struct ASDoc *doc);
Bool           open_ascolorscheme_xml_view( struct ASView *view);
void           refresh_ascolorscheme_xml_view(struct ASView *view);
Bool           close_ascolorscheme_xml_view( struct ASView *view);
void           destroy_ascolorscheme_xml_view( struct ASView **pview );


ASViewType ASColorschemeXmlViewType = 
{
#define VIEW_TYPE_NAME_ASColorschemeXml "AfterStep Colorscheme XML Image"    
    VIEW_TYPE_NAME_ASColorschemeXml,
    0,
    
    create_ascolorscheme_xml_view,
    open_ascolorscheme_xml_view,
    refresh_ascolorscheme_xml_view,
    close_ascolorscheme_xml_view,
    destroy_ascolorscheme_xml_view
};    

typedef struct ASColorschemeXmlViewData
{
    unsigned long magic ; 

    ASImage *cs_im ;        
    enum
	{
		ASC_PARAM_BaseHue =0,
		ASC_PARAM_BaseSat	,
		ASC_PARAM_BaseVal	,
		ASC_PARAM_Angle
	} curr_param ;
    
}ASColorschemeXmlViewData;

ASViewType ASCPMainFrameViewType = 
{
#define VIEW_TYPE_NAME_ASCP "AfterStep Control Panel"    
    VIEW_TYPE_NAME_ASCP,
    0,
    
    create_ascp_main_frame_view,
    open_ascp_main_frame_view,
    refresh_ascp_main_frame_view,
    close_ascp_main_frame_view,
    destroy_ascp_main_frame_view
};    

typedef struct ASCPMainFrameViewData
{
    unsigned long magic ; 
    /* add some data here */
    
}ASCPMainFrameViewData;


ASDocViewManager *DocViewManager = NULL ; 


int main(int argc, char** argv)
{
	int i;

	/* Save our program name - for error messages */
    InitMyApp (CLASS_ASCOLOR, argc, argv, ascolor_usage, NULL, 0 );

    DocViewManager = safecalloc( 1, sizeof(ASDocViewManager) );
    register_doc_type ( DocViewManager, ASColorschemeDocType);
    register_view_type ( DocViewManager, ASColorschemeXmlViewType);
    register_doc_view_template ( DocViewManager, DOC_TYPE_NAME_ASColorscheme, VIEW_TYPE_NAME_ASColorschemeXml) ;


    ConnectX( &Scr, PropertyChangeMask );
    ConnectAfterStep ( 0 );

	LoadBaseConfig (GetBaseOptions);

    create_main_window();
    HandleEvents();
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

Window
create_main_window()
{
	Window        w;
	XSizeHints    shints;
	ExtendedWMHints extwm_hints ;
    int x = 0, y = 0;
    int width = 640;
    int height = 480;
    XSetWindowAttributes attr;

    attr.event_mask = StructureNotifyMask|ButtonPressMask|KeyPressMask|ButtonReleaseMask|PointerMotionMask|PropertyChangeMask ;
    w = create_visual_window( Scr.asv, Scr.Root, x, y, width, height, 4, InputOutput, CWEventMask, &attr);
    set_client_names( w, MyName, MyName, CLASS_ASCOLOR, MyName );

    Scr.RootClipArea.x = x;
    Scr.RootClipArea.y = y;
    Scr.RootClipArea.width = width;
    Scr.RootClipArea.height = height;

    shints.flags = USSize|PMinSize|PResizeInc|PWinGravity;
    shints.flags |= PPosition ;

    shints.min_width = 640;
    shints.min_height = 480;
    shints.width_inc = 1;
    shints.height_inc = 1;
    shints.win_gravity = NorthWestGravity ;

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


