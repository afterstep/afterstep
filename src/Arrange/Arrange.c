/*
 * Copyright (C) 2005 Fabian Yamaguchi <fabiany at gmx.net>
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
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <math.h>

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
#include "../../libAfterStep/shape.h"
#include "../../libAfterStep/ascommand.h"
#include "../../libAfterStep/operations.h"

#include "../../libAfterBase/aslist.h"

#include "../../libAfterConf/afterconf.h"

/**********************************************************************/
/*  ARRANGE local variables :                                         */
/**********************************************************************/
#define ARRANGE_Untitled	(0x01<<0)
#define ARRANGE_Transient	(0x01<<1)
#define ARRANGE_Sticky		(0x01<<2)
#define ARRANGE_Maximized	(0x01<<3)
#define ARRANGE_All			(ARRANGE_Untitled|ARRANGE_Transient| \
							 ARRANGE_Sticky|ARRANGE_Maximized| ARRANGE_WinlistSkip)
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
#define ARRANGE_OffsetX_Set		(0x01<<14)
#define ARRANGE_OffsetY_Set		(0x01<<15)
#define ARRANGE_MaxWidth_Set	(0x01<<16)
#define ARRANGE_MaxHeight_Set	(0x01<<17)
#define ARRANGE_Tile_Horizontally	(0x01<<18)
#define ARRANGE_AllDesks                (0x01<<19)
#define ARRANGE_WinlistSkip             (0x01<<20)
#define ARRANGE_SmartTile                 (0x01<<21)

struct ASArrangeState
{
	ASFlagType flags ;
	int incx, incy ;
	int count ;
	
	int offset_x, offset_y ; 
	int max_width, max_height ;
	
	int curr_x, curr_y ;

	int *elem, *group;
	int *elem_size, *group_size; /* in pixels */
	int start_elem; /* position of start_elem */

	int tile_width, tile_height;

	char *pattern;

	ASBiDirList *clients_order;
	   
}ArrangeState;

typedef struct
{
	Window cl;
}client_item;

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
void Arrange_DeadPipe(int);

int
atopixel (char *s, int size)
{
  	int l = strlen (s);
  	if (l < 1)
    	return 0;
  	if (s[l-1] == 'p' || s[l-1] == 'P' )
    {
	    /* number was followed by a p/P
	     * => number was already a pixel-value */
		s[l-1] = '\0' ;
		return atoi (s);
    }
  	/* return s percent of size */
	return (atoi (s) * size) / 100;
}

void
select_suitable_windows(void)
{

	/* Select all windows, then get
	 * rid of those we don't want. */
	select_all( False );
	
	LOCAL_DEBUG_OUT("# Number of selected windows: %d\n", num_selected_windows());

	/* we don't want to arrange fullscreen-windows */
        select_windows_by_flag( AS_Fullscreen, True);
	/* we do not want to arrange AfterSTep's modules */

	select_windows_by_flag( AS_Module, True);
	LOCAL_DEBUG_OUT("# Number of selected windows: %d\n", num_selected_windows());

	/* also we do not want to arrange AvoidCover windows : */
	select_windows_by_flag(AS_AvoidCover, True);

        /* Remove winlist-skip-windows if user doesn't want to
	   arrange them */
	if( !get_flags( ArrangeState.flags, ARRANGE_WinlistSkip))
		select_windows_by_flag(AS_SkipWinList, True);
	
	LOCAL_DEBUG_OUT("# Number of selected windows: %d\n", num_selected_windows());

	/* return if window is untitled and we don't want
	   to arrange untitled windows  */
	if( !get_flags( ArrangeState.flags, ARRANGE_Untitled ))
		select_untitled_windows(True);
	
	LOCAL_DEBUG_OUT("# Number of selected windows: %d\n", num_selected_windows());

	/* return if window is transient and we don't want
	   to arrange transient windows. */
	if( !get_flags( ArrangeState.flags, ARRANGE_Transient ))
		select_windows_by_flag(AS_Transient, True);
	
	LOCAL_DEBUG_OUT("# Number of selected windows: %d\n", num_selected_windows());

	/* return if window is sticky and we don't want
	   to arrange sticky windows. */
	if( !get_flags( ArrangeState.flags, ARRANGE_Sticky ))
		select_windows_by_state_flag(AS_Sticky, True);
	LOCAL_DEBUG_OUT("# Number of selected windows: %d\n", num_selected_windows());
	

	/*return if window is maximized and we don't want
	  to arrange maximized windows. */
	
	if( !get_flags( ArrangeState.flags, ARRANGE_Maximized ))
		select_windows_by_state_flag(AS_MaximizedX|AS_MaximizedY|AS_Fullscreen, True);

	LOCAL_DEBUG_OUT("# Number of selected windows: %d\n", num_selected_windows());

	/* return if window is not on current desk and we don't want
	   to arrange windows of all desks. */
	if( !get_flags( ArrangeState.flags, ARRANGE_AllDesks))
	{
		select_windows_on_desk(False);
		
		/* if user passed ARRANGE_AllDesks, it's safe to assume 
		 * he wants to arrange on all screens as well, so we
		 * only check for ARRANGE_Desk if AllDesks was not set */
		
	/* return if window is not on current screen and we don't want
	   to arrange windows on the whole desk. */
		if( !get_flags( ArrangeState.flags, ARRANGE_Desk))
			select_windows_on_screen(False);	
	}
	
	LOCAL_DEBUG_OUT("# Number of selected windows: %d\n", num_selected_windows());

	/* If a pattern was specified, only select windows matching the pattern*/
	if( ArrangeState.pattern )
		select_windows_by_pattern(ArrangeState.pattern, False, False);
	
}


int
main( int argc, char **argv )
{
	int i ;
	int nargc = 0 ;
	XRectangle *area;
	Bool resize_touched = False;
	/* Save our program name - for error messages */
	set_DeadPipe_handler(Arrange_DeadPipe);
	InitMyApp (CLASS_ARRANGE, argc, argv, NULL, NULL, OPTION_SINGLE|OPTION_RESTART );
	set_signal_handler( SIGSEGV );
	ConnectX( ASDefaultScr, 0 );

	memset( &ArrangeState, 0x00, sizeof(ArrangeState));
	
	
	/* Set some sane default values if still unset */
	
	ArrangeState.max_width = Scr.MyDisplayWidth ;
	ArrangeState.max_height = Scr.MyDisplayHeight ;
	ArrangeState.incx = 20 ; 
	ArrangeState.incy = 20 ;
	

    /* Check the Name of the Program to see wether to tile or cascade*/
	if( mystrcasecmp( MyName , "Tile" ) == 0 )
	{
		set_flags( ArrangeState.flags, ARRANGE_Tile	);
		set_flags( ArrangeState.flags, ARRANGE_Resize);
	}

	for( i = 1 ; i< argc ; ++i)
	{
		if( argv[i] == NULL )
			continue;
		/*
		  From manpage of old Cascade:
		  Up to four numbers can be placed in the command line
		  that are not switches.
		  The first pair specify an x and y offset to start
		  the first window (default 0, 0). The second pair
		  specify a maximal width and height for layered windows.
		*/
		if( isdigit( argv[i][0] ) )
		{
			if( nargc == 0 ) 
			{	
				ArrangeState.offset_x = atopixel( argv[i],  Scr.MyDisplayWidth );
				set_flags( ArrangeState.flags, ARRANGE_OffsetX_Set );
			}else if( nargc == 1 ) 
			{	
				ArrangeState.offset_y = atopixel( argv[i], Scr.MyDisplayHeight  );
				set_flags( ArrangeState.flags, ARRANGE_OffsetY_Set );
			}else if( nargc == 2 ) 
			{	
				ArrangeState.max_width = atopixel( argv[i], Scr.MyDisplayWidth );
				set_flags( ArrangeState.flags, ARRANGE_MaxWidth_Set );
			}else if( nargc == 3 ) 
			{	
				ArrangeState.max_height = atopixel( argv[i], Scr.MyDisplayHeight );
				set_flags( ArrangeState.flags, ARRANGE_MaxHeight_Set );
			}
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
				
				        case 'H': /* For tiling: Tile horizontally (Default is vertically ) */
					
						set_flags( ArrangeState.flags, ARRANGE_Tile_Horizontally );
				        break;
				}
			}else if( mystrcasecmp( argv[i], "-desk" ) == 0 )
			{
				/* Causes all windows on the desk to be cascaded instead of the current screen only. */
				set_flags( ArrangeState.flags, ARRANGE_Desk );
			}else if( mystrcasecmp( argv[i], "-alldesks") == 0)
			{
				set_flags(ArrangeState.flags, ARRANGE_AllDesks);
			}
			else if( mystrcasecmp( argv[i], "-flatx" ) == 0 )
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
				resize_touched = True;
			}else if( mystrcasecmp( argv[i], "-noresize") == 0)
			{
				clear_flags( ArrangeState.flags, ARRANGE_Resize );
				resize_touched = True;
			}else if( mystrcasecmp( argv[i], "-tile") == 0 )
			{
				set_flags( ArrangeState.flags, ARRANGE_Tile	);
				if(! resize_touched )
					set_flags(ArrangeState.flags, ARRANGE_Resize );
			}
			else if( mystrcasecmp( argv[i], "-smart") == 0)
			{
				set_flags(ArrangeState.flags, ARRANGE_SmartTile);
			}else if( mystrcasecmp( argv[i], "-pattern") == 0 && i+1 <argc && argv[i+1] != NULL)
			{
				ArrangeState.pattern = argv[i+1];
			}
			/* these apply to tiling only : */    
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

	ascom_init();
	/* Request a list of all windows, while we load our config */
	ascom_update_winlist();

	LoadBaseConfig ( GetBaseOptions);
    
	/* fix area */
	area = ascom_get_available_area();
	
	if( !get_flags( ArrangeState.flags, ARRANGE_OffsetX_Set ) )
		ArrangeState.offset_x = area->x;
		
	if( !get_flags( ArrangeState.flags, ARRANGE_OffsetY_Set ) )
		ArrangeState.offset_y = area->y;
				
	if( !get_flags( ArrangeState.flags, ARRANGE_MaxWidth_Set ) )
		ArrangeState.max_width = area->width;
	
	if( !get_flags( ArrangeState.flags, ARRANGE_MaxHeight_Set ) )
		ArrangeState.max_height = area->height;
	
	free(area);

	/************/

	select_suitable_windows();
	
	/* rearrange windows */		   

	if( get_flags( ArrangeState.flags, ARRANGE_Tile	) ) 
		tile_windows();
	else
		cascade_windows();

	ascom_wait();

	/* exit */
	Arrange_DeadPipe (0);
			
	return 0 ;
}


void
Arrange_DeadPipe (int nonsense)
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
	ReloadASEnvironment( NULL, NULL, NULL, False, False );
}


/********************************************************************/
/* showing our main window :                                        */
/********************************************************************/

void 
tile_windows()
{
	int n_groups;
	int n_windows = num_selected_windows();
	Bool reverse = get_flags(ArrangeState.flags, ARRANGE_Reversed);

	LOCAL_DEBUG_OUT("Number of windows to be arranged: %d\n", n_windows);
	LOCAL_DEBUG_OUT("max_width/max_height: %d/%d\n", ArrangeState.max_width, ArrangeState.max_height);
		
        /* If number of elements per group was not set */
	if(ArrangeState.count == 0)
	{
		if( ! get_flags(ArrangeState.flags, ARRANGE_SmartTile) )
			ArrangeState.count = n_windows; /*Put all elements in one group*/
		else
			ArrangeState.count = (int) sqrt(n_windows);
	}
	n_groups = n_windows / ArrangeState.count;
	/* If not all windows fit in n_groups groups, an
	 * extra group for remaining windows is needed.*/
	if(n_windows % ArrangeState.count)
		n_groups++;
	
	LOCAL_DEBUG_OUT("ngroups: %d", n_groups);

	ArrangeState.curr_x = ArrangeState.offset_x ;
	ArrangeState.curr_y = ArrangeState.offset_y ;
	
	LOCAL_DEBUG_OUT("curr_x/curr_y: %d/%d\n", ArrangeState.curr_x, ArrangeState.curr_y);

	if(get_flags( ArrangeState.flags, ARRANGE_Tile_Horizontally))
	{
		ArrangeState.start_elem = ArrangeState.curr_x;
		ArrangeState.elem = &ArrangeState.curr_x;
		ArrangeState.group = &ArrangeState.curr_y;
		
		ArrangeState.tile_width =
			ArrangeState.max_width / ArrangeState.count;
		ArrangeState.tile_height =
			ArrangeState.max_height/ n_groups ;
		
		ArrangeState.elem_size = &ArrangeState.tile_width; 
		ArrangeState.group_size =  &ArrangeState.tile_height;
		
	}else
	{
		ArrangeState.start_elem = ArrangeState.curr_y;
		ArrangeState.elem = &ArrangeState.curr_y;
		ArrangeState.group = &ArrangeState.curr_x;
		
		ArrangeState.tile_width =
			ArrangeState.max_width / n_groups;
		ArrangeState.tile_height =
			ArrangeState.max_height / ArrangeState.count ;
		
		ArrangeState.elem_size = &ArrangeState.tile_height;
		ArrangeState.group_size = &ArrangeState.tile_width ;
	}
	
	while(! winlist_is_empty() )
	{
		move_params p;
		const ASWindowData *wd;
		
		/* If group is full */
		if(*(ArrangeState.elem) ==
		   (ArrangeState.start_elem +
		    ArrangeState.count * *(ArrangeState.elem_size)))
		{
			/* Make new group */
			*(ArrangeState.group)+= *(ArrangeState.group_size);
			/* Make this the first item of the new group */
			*(ArrangeState.elem) = ArrangeState.start_elem;
		}
		

		/* Raise the client if allowed */
		if( !get_flags( ArrangeState.flags, ARRANGE_NoRaise ) )
			ascom_do_one("raise", NULL, reverse);
		
		
		/* If the client is iconified, deiconify it. */
		ascom_do_one("deiconify", NULL, reverse);
		
		p.x = ArrangeState.curr_x;
		p.y = ArrangeState.curr_y;
	
		/* Move window */
		ascom_do_one("move", &p, reverse);
		
		wd = ascom_get_next_window( reverse );

		/* Make sure we don't stretch if ARRANGE_NoStretch was set */
		if ( !( get_flags( ArrangeState.flags, ARRANGE_NoStretch) &&
			(    (wd->frame_rect.width < ArrangeState.tile_width)
			     || (wd->frame_rect.height < ArrangeState.tile_height)
				)))
		{
			resize_params rparams;
			rparams.width = ArrangeState.tile_width ; 
			rparams.height = ArrangeState.tile_height ;
                        /* Resize window if allowed*/
			if( get_flags(ArrangeState.flags, ARRANGE_Resize ))
				ascom_do_one("resize", &rparams, reverse);
		}

		
		/* Transfer window onto CurrentDesk */
		if( wd->desk != Scr.CurrentDesk )
		{
			send_to_desk_params p;
			p.desk = Scr.CurrentDesk;
			LOCAL_DEBUG_OUT("#CurrentDesk: %d", Scr.CurrentDesk);

			ascom_do_one("sendtodesk", &p, reverse);
		}
		
		(*(ArrangeState.elem))+= *ArrangeState.elem_size;
		
		ascom_pop_winlist(reverse);		
	}

}	 

/* Cascade */

void 
cascade_windows()
{
	
	ArrangeState.curr_x = ArrangeState.offset_x ;
	ArrangeState.curr_y = ArrangeState.offset_y ;
	
	
	while( !winlist_is_empty() )
	{
		move_params p;
		const ASWindowData *wd;
		Bool reverse = get_flags(ArrangeState.flags, ARRANGE_Reversed);

		/* Raise the client if allowed */
		if( !get_flags( ArrangeState.flags, ARRANGE_NoRaise ) )
			ascom_do_one("raise", NULL, reverse);
		
		
		/* If the client is iconified, deiconify it. */
		ascom_do_one("deiconify", NULL, reverse);
	
		/* move window */
		p.x = ArrangeState.curr_x;
		p.y = ArrangeState.curr_y;
		ascom_do_one("move", &p, reverse);
		
		ArrangeState.curr_x += ArrangeState.incx ;
		ArrangeState.curr_y += ArrangeState.incy ;

		wd = ascom_get_next_window(reverse);

		if( get_flags( ArrangeState.flags, ARRANGE_Resize ) )
		{
			/* Make sure we don't stretch of ARRANGE_NoStretch was set */
			if ( !( get_flags( ArrangeState.flags, ARRANGE_NoStretch) &&
				(    (wd->frame_rect.width < ArrangeState.max_width)
				     || (wd->frame_rect.height < ArrangeState.max_height)
					)))
			{
				resize_params rp;
				rp.width = ArrangeState.max_width ; 
				rp.height = ArrangeState.max_height ;
				if( get_flags( ArrangeState.flags, ARRANGE_Resize) )
					ascom_do_one("resize", &rp, reverse);
			}
		}
		
		/* Transfer window onto CurrentDesk */
		if( wd->desk != Scr.CurrentDesk )
		{
			send_to_desk_params p;
			p.desk = Scr.CurrentDesk;
	
			ascom_do_one("sendtodesk", &p, reverse);
		}
		
		
		ascom_pop_winlist(reverse);
	}

}
