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

#include "ascpui.h"

/**********************************************************************/
/*  ascp local variables :                                         */
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Our configuration options :                                        */
/**********************************************************************/
BaseConfig *Base = NULL;

/**********************************************************************/
void GetBaseOptions (const char *filename);
int  run_ascp();

int
main( int argc, char **argv )
{
    /* Save our program name - for error messages */
    InitMyApp (CLASS_ASCP, argc, argv, NULL, NULL, 0 );

    set_signal_handler( SIGSEGV );

    ConnectX( &Scr, PropertyChangeMask );
    ConnectAfterStep (M_FOCUS_CHANGE |
                      M_DESTROY_WINDOW |
                      WINDOW_CONFIG_MASK |
                      WINDOW_NAME_MASK |
                      M_END_WINDOWLIST);
    LoadBaseConfig ( GetBaseOptions);
	/* later :
    LoadConfig ("ascp", GetOptions);
    CheckConfigSanity();
	*/

	/* And at long last our main loop : */
	return run_ascp()
}

int
run_ascp()
{
	ASControlPanelUI  ascpui ;


	return Fl::run() ;
}

void
DeadPipe (int nonsense)
{
    FreeMyAppResources();

	if( Base )
        DestroyBaseConfig(Base);
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
    BaseConfig *config = ParseBaseOptions (filename, MyName);

    if (!config)
	    exit (0);			/* something terrible has happend */

    if( Base!= NULL )
		DestroyBaseConfig (Base);
	Base = config ;

    if( Scr.image_manager )
        destroy_image_manager( Scr.image_manager, False );
    if( Scr.font_manager )
        destroy_font_manager( Scr.font_manager, False );

    Scr.image_manager = create_image_manager( NULL, 2.2, Base->pixmap_path, Base->icon_path, NULL );
    Scr.font_manager = create_font_manager (dpy, Base->font_path, NULL);
}


