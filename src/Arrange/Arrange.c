/*
 * Copyright (C) 2004 Sasha Vasko <sasha at aftercode.net>
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
/*  ARRANGE local variables :                                         */
/**********************************************************************/
#define ARRANGE_Untitled	(0x01<<0)
#define ARRANGE_Transient	(0x01<<1)
#define ARRANGE_Sticky		(0x01<<2)
#define ARRANGE_Maximized	(0x01<<3)
#define ARRANGE_All			(ARRANGE_Untitled|ARRANGE_Transient| \
							 ARRANGE_Sticky|ARRANGE_Maximized)
#define ARRANGE_Reversed	(0x01<<4)
#define ARRANGE_Desk		(0x01<<5)
#define ARRANGE_FlatX		(0x01<<6)
#define ARRANGE_FlatY		(0x01<<7)
#define ARRANGE_IncX		(0x01<<8)
#define ARRANGE_IncY		(0x01<<9)
#define ARRANGE_NoRaise		(0x01<<10)
#define ARRANGE_NoStretch	(0x01<<11)
#define ARRANGE_Resize		(0x01<<12)
#define ARRANGE_Tile		(0x01<<13)

struct ASArrangeState
{
	ASFlagType flags ;
	int incx, incy ;
	int count ;
	
	int offset_x, offset_y ; 
	int max_width, max_height ;
	
	int curr_x, curr_y ;
	   
}ArrangeState;

/**********************************************************************/
/**********************************************************************/
/* Our configuration options :                                        */
/**********************************************************************/

/**********************************************************************/
void GetBaseOptions (const char *filename);
void HandleEvents();
void process_message (send_data_type type, send_data_type *body);
void tile_windows();
void cascade_windows();
void DeadPipe(int);


int
atopixel (char *s, int size)
{
  	int l = strlen (s);
  	if (l < 1)
    	return 0;
  	if (s[l-1] == 'p' || s[l-1] == 'P' )
    {
		s[l-1] = '\0' ;
		return atoi (s);
    }
  	return (atoi (s) * size) / 100;
}


int
main( int argc, char **argv )
{
	int i ;
	int nargc = 0 ;
    /* Save our program name - for error messages */
	set_DeadPipe_handler(DeadPipe);
    InitMyApp (CLASS_ARRANGE, argc, argv, NULL, NULL, OPTION_SINGLE|OPTION_RESTART );

    set_signal_handler( SIGSEGV );

    ConnectX( ASDefaultScr, 0 );

	if( mystrcasecmp( MyName , "Tile" ) == 0 )
		set_flags( ArrangeState.flags, ARRANGE_Tile	);

	for( i = 1 ; i< argc ; ++i)
	{
		if( argv[i] == NULL )
			continue;
		if( isdigit( argv[i][0] ) )
		{
			if( nargc == 0 ) 
				ArrangeState.offset_x = atoi( argv[i] );
			if( nargc == 1 ) 
				ArrangeState.offset_y = atoi( argv[i] );
			if( nargc == 2 ) 
				ArrangeState.max_width = atoi( argv[i] );
			if( nargc == 3 ) 
				ArrangeState.max_height = atoi( argv[i] );
			++nargc ;
		}else if( argv[i][0] == '-' ) 
		{
			if( argv[i][1] == '\0' ) 
				continue;
			else if( argv[i][2] == '\0' ) 
			{	
				switch( argv[i][1] )
				{
					case 'u' :     	/* Causes untitled windows to also be affected (implied by \-all). */
						set_flags( ArrangeState.flags, ARRANGE_Untitled );
				    	break ;				   
					case 't' :	   	/* Causes transient windows to also be affected (implied by \-all). */	
						set_flags( ArrangeState.flags, ARRANGE_Transient );
				    	break ;
					case 's' :		/* Causes sticky windows to also be affected (implied by \-all). */
						set_flags( ArrangeState.flags, ARRANGE_Sticky );
				    	break ;
					case 'm' :		/* Causes maximized windows to also be affected (implied by \-all). */
						set_flags( ArrangeState.flags, ARRANGE_Maximized );
				    	break ;
					case 'a' :	   /* Causes \fIall\fP window styles to be affected, even ones with the WindowListSkip style. */
						set_flags( ArrangeState.flags, ARRANGE_All );
				    	break ;
					case 'r' :	   /* Reverses the window sequence. */	
						set_flags( ArrangeState.flags, ARRANGE_Reversed );
				    	break ;
				}
			}else if( mystrcasecmp( argv[i], "-desk" ) == 0 )
			{
				/* Causes all windows on the desk to be cascaded instead of the current screen only. */
				set_flags( ArrangeState.flags, ARRANGE_Desk );
			}else if( mystrcasecmp( argv[i], "-flatx" ) == 0 )
			{
				/* Inhibits border width increment. */
				set_flags( ArrangeState.flags, ARRANGE_FlatX );
			}else if( mystrcasecmp( argv[i], "-flaty" ) == 0 )
			{ 
				/* Inhibits border height increment. */
				set_flags( ArrangeState.flags, ARRANGE_FlatY );
			}else if( mystrcasecmp( argv[i], "-incx" ) == 0 && i+1 < argc && argv[i+1] != NULL )
			{ 
				/* Specifies a horizontal increment which is successively added to
					cascaded windows.  \fIarg\fP is a percentage of screen width, or pixel
					value if a \fIp\fP is suffixed.  Default is zero.
				 */
				++i ;
				ArrangeState.incx = atopixel (argv[i], Scr.MyDisplayWidth);
				set_flags( ArrangeState.flags, ARRANGE_IncX );
			}else if( mystrcasecmp( argv[i], "-incy" ) == 0 && i+1 < argc && argv[i+1] != NULL )
			{
				/* Specifies a vertical increment which is successively added to cascaded
					windows.  \fIarg\fP is a percentage of screen height, or pixel value
					if a \fIp\fP is suffixed.  Default is zero. 
				 */
				++i ;
				ArrangeState.incy = atopixel (argv[i], Scr.MyDisplayHeight);
				set_flags( ArrangeState.flags, ARRANGE_IncY );
			}else if( mystrcasecmp( argv[i], "-noraise" ) == 0 )
			{
				/* Inhibits window raising, leaving the depth ordering intact. */
				set_flags( ArrangeState.flags, ARRANGE_NoRaise );
			}else if( mystrcasecmp( argv[i], "-nostretch" ) == 0 )
			{ 
				/* Inhibits window expansion when using the \-resize option.  Windows
				   will only shrink to fit the maximal width and height (if given).
				 */
				set_flags( ArrangeState.flags, ARRANGE_NoStretch );
			}else if( mystrcasecmp( argv[i], "-resize" ) == 0 )
			{ 
				/* Forces all windows to resize to the constrained width and height (if given). */
				set_flags( ArrangeState.flags, ARRANGE_Resize );
			}else if( mystrcasecmp( argv[i], "-tile") == 0 )
				set_flags( ArrangeState.flags, ARRANGE_Tile	);
			/* these applies to tiling only : */    
			else if( mystrcasecmp( argv[i], "-mn") == 0 && i+1 < argc )
			{
				/* Tiles up to \fIarg\fP windows in tile direction.  If more windows
				   exist, a new direction row or column is created (in effect, a matrix is created).
				 */
				++i ;
				ArrangeState.count = atoi( argv[i] );
			}
		}							   
	}

    ConnectAfterStep (WINDOW_CONFIG_MASK |
                      WINDOW_NAME_MASK |
                      M_END_WINDOWLIST, 0);

    /* Request a list of all windows, while we load our config */
    SendInfo ("Send_WindowList", 0);

    LoadBaseConfig ( GetBaseOptions);

	/* And at long last our main loop : */
    HandleEvents();
	return 0 ;
}

void HandleEvents()
{
    while (True)
    {
        module_wait_pipes_input ( process_message );
	}
}

void
DeadPipe (int nonsense)
{
	static int already_dead = False ; 
	if( already_dead ) 
		return;/* non-reentrant function ! */
	already_dead = True ;
    
	window_data_cleanup();

	FreeMyAppResources();
    
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
process_message (send_data_type type, send_data_type *body)
{
    LOCAL_DEBUG_OUT( "received message %lX", type );

	if( type == M_END_WINDOWLIST )
	{
		/* rearrange windows */		
		if( set_flags( ArrangeState.flags, ARRANGE_Tile	) ) 
			tile_windows();
		else
			cascade_windows();
		/* exit */
		DeadPipe (0);
	}else if( (type&WINDOW_PACKET_MASK) != 0 )
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );

		show_progress( "message %X window %X ", type, body[0] );
		handle_window_packet( type, body, &wd );
	}
}


/********************************************************************/
/* showing our main window :                                        */
/********************************************************************/
void 
tile_windows()
{
	
	
}	 


Bool 
cascade_winlist_iter(void *data, void *aux_data)
{
	ASWindowData *wd = data ;	
	static char msg[256];
	if( !get_flags( ArrangeState.flags, ARRANGE_Untitled ) && get_flags( wd->flags, AS_Titlebar ) )
		return True;
	if( !get_flags( ArrangeState.flags, ARRANGE_Transient ) && get_flags( wd->flags, AS_Transient ) )
		return True;
	if( !get_flags( ArrangeState.flags, ARRANGE_Sticky ) && get_flags( wd->state_flags, AS_Sticky ) )
		return True;
	if( !get_flags( ArrangeState.flags, ARRANGE_Maximized ) && get_flags( wd->state_flags, AS_MaximizedX|AS_MaximizedY ) )
		return True;

	if( !get_flags( ArrangeState.flags, ARRANGE_NoRaise ) )
		SendInfo ( "Raise", wd->client );

	sprintf( &msg[0], "Move %up %up", ArrangeState.curr_x, ArrangeState.curr_y);
	SendInfo( &msg[0], wd->client );

	ArrangeState.curr_x += ArrangeState.incx ;
	ArrangeState.curr_y += ArrangeState.incy ;

	return True;   
}
	

void 
cascade_windows()
{
	ArrangeState.curr_x = ArrangeState.offset_x ;
	ArrangeState.curr_y = ArrangeState.offset_y ;
	iterate_window_data( cascade_winlist_iter, NULL);	
}

