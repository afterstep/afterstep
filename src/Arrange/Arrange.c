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
#include <stdlib.h>

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
#define ARRANGE_OffsetX_Set		(0x01<<14)
#define ARRANGE_OffsetY_Set		(0x01<<15)
#define ARRANGE_MaxWidth_Set	(0x01<<16)
#define ARRANGE_MaxHeight_Set	(0x01<<17)
#define ARRANGE_Tile_Horizontally	(0x01<<18)

struct ASArrangeState
{
	ASFlagType flags ;
	int incx, incy ;
	int count ;
	
	int offset_x, offset_y ; 
	int max_width, max_height ;
	
	int curr_x, curr_y ;

	int *elem, *group;
	int *elem_size, *group_size;
	int start_elem;

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
void DeadPipe(int);
void fix_available_area();

void
destroy_client_item(void *data)
{
	free((client_item *) data);
}

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

	memset( &ArrangeState, 0x00, sizeof(ArrangeState));
	ArrangeState.incx = 20 ; 
	ArrangeState.incy = 20 ;
	ArrangeState.clients_order = create_asbidirlist( destroy_client_item );

    /* Check the Name of the Program to see wether to tile or cascade*/
	if( mystrcasecmp( MyName , "Tile" ) == 0 )
		set_flags( ArrangeState.flags, ARRANGE_Tile	);

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

Bool
window_is_suitable(ASWindowData *wd)
{
	/* we do not want to arrange AfterSTep's modules */
	if( get_flags( wd->flags, AS_Module|AS_SkipWinList ) == (AS_Module|AS_SkipWinList))
		return False;
	/* also we do not want to arrange AvoidCover windows : */
	if( get_flags( wd->flags, AS_AvoidCover ) )
		return False;
	/* return if window is untitled and we don't want
	   to arrange untitled windows  */
	if( !get_flags( ArrangeState.flags, ARRANGE_Untitled ) && wd->window_name == NULL)
		return False;
	/* return if window is transient and we don't want
	   to arrange transient windows. */
	if( !get_flags( ArrangeState.flags, ARRANGE_Transient ) && get_flags( wd->flags, AS_Transient ) )
		return False;
	/* return if window is sticky and we don't want
	   to arrange sticky windows. */
	if( !get_flags( ArrangeState.flags, ARRANGE_Sticky ) && get_flags( wd->state_flags, AS_Sticky ) )
		return False;
	/* return if window is maximized and we don't want
	   to arrange maximized windows. */
	if( !get_flags( ArrangeState.flags, ARRANGE_Maximized ) && get_flags( wd->state_flags, AS_MaximizedX|AS_MaximizedY ) )
		return False;

	/* Passed all tests. You're in. */
	return True;
}


/****************************************************************************/
/* PROCESSING OF AFTERSTEP MESSAGES :                                       */
/****************************************************************************/
void
process_message (send_data_type type, send_data_type *body)
{
	client_item *new_item;
	
  LOCAL_DEBUG_OUT( "received message %lX", type );

	if( type == M_END_WINDOWLIST )
	{
		/* rearrange windows */		   
		fix_available_area();
		if( get_flags( ArrangeState.flags, ARRANGE_Tile	) ) 
			tile_windows();
		else
			cascade_windows();
		/* exit */
		destroy_asbidirlist( &(ArrangeState.clients_order) );
		DeadPipe (0);
	}else if( (type&WINDOW_PACKET_MASK) != 0 )
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );
		
		show_progress( "message %X window %X ", type, body[0] );
		if( handle_window_packet( type, body, &wd ) == WP_DataCreated )
		{
			new_item = safemalloc( sizeof(client_item) );
			new_item->cl = wd->client;
			append_bidirelem( ArrangeState.clients_order, new_item);
		}
	}
}


/********************************************************************/
/* showing our main window :                                        */
/********************************************************************/
Bool
count_managed_windows(void *data, void *aux_data)
{
	ASWindowData *wd = fetch_window_by_id( ((client_item *) data)->cl );
	int *c = (int *) aux_data;
	if(window_is_suitable(wd))
		(*c)++;
	return True;
}


Bool
tile_window(void *data, void *aux_data)
{
	
	ASWindowData *wd = fetch_window_by_id( ((client_item *) data)->cl );

	/* used by SendNumCommand */
	send_signed_data_type vals[2] ;	
	send_signed_data_type units[2] ;

	if(! window_is_suitable( wd ))
	  return True; /* Next window please */

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
		SendNumCommand ( F_RAISE, NULL, NULL, NULL, wd->client );
	
	
	/* Indicate that we're talking pixels. */
	units[0] = units[1] = 1;
	vals[0] = ArrangeState.curr_x; vals[1] = ArrangeState.curr_y;
	/* Move window */
	SendNumCommand ( F_MOVE, NULL, &(vals[0]), &(units[0]), wd->client );

	
	vals[0] = ArrangeState.max_width ; 
	vals[1] = ArrangeState.max_height ;
	SendNumCommand ( F_RESIZE, NULL, &(vals[0]), &(units[0]), wd->client );
	
	
	(*(ArrangeState.elem))+= *ArrangeState.elem_size;
	return True;
}

void 
tile_windows()
{
	int n_windows = 0;
	iterate_asbidirlist( ArrangeState.clients_order,
			     count_managed_windows, &n_windows, NULL, False);
	
	/* If number of elements per group was not set */
	if(ArrangeState.count == 0)
	  ArrangeState.count = n_windows; /*Put all elements in one group*/
	
	int n_groups = n_windows / ArrangeState.count;
	if(n_windows % ArrangeState.count)
		n_groups++;
	
	LOCAL_DEBUG_OUT("ngroups: %d", n_groups);

	ArrangeState.curr_x = ArrangeState.offset_x ;
	ArrangeState.curr_y = ArrangeState.offset_y ;
	

	if(get_flags( ArrangeState.flags, ARRANGE_Tile_Horizontally))
	{
		ArrangeState.start_elem = ArrangeState.curr_x;
		ArrangeState.elem = &ArrangeState.curr_x;
		ArrangeState.group = &ArrangeState.curr_y;
		
		/* Watchout: max_width is now maximum width of a single window */
		ArrangeState.max_width =
			abs(ArrangeState.max_width - ArrangeState.curr_x)
			/ ArrangeState.count;
		ArrangeState.max_height =
			abs(ArrangeState.max_height - ArrangeState.curr_y)
			/ n_groups ;
		
		ArrangeState.elem_size = &ArrangeState.max_width; 
		ArrangeState.group_size =  &ArrangeState.max_height;
		
	}else
	{
		ArrangeState.start_elem = ArrangeState.curr_y;
		ArrangeState.elem = &ArrangeState.curr_y;
		ArrangeState.group = &ArrangeState.curr_x;
		
		/* Watchout: max_width is now maximum width of a single window */
		ArrangeState.max_width =
			abs(ArrangeState.max_width - ArrangeState.curr_x)
			/ n_groups;
		ArrangeState.max_height =
			abs(ArrangeState.max_height - ArrangeState.curr_y)
			/ ArrangeState.count ;
		
		ArrangeState.elem_size = &ArrangeState.max_height;
		ArrangeState.group_size = &ArrangeState.max_width ;
	}
	

	iterate_asbidirlist( ArrangeState.clients_order, tile_window,
			     NULL, NULL, get_flags( ArrangeState.flags, ARRANGE_Reversed));
}	 


Bool 
cascade_window(void *data, void *aux_data)
{
	ASWindowData *wd = fetch_window_by_id( ((client_item *) data)->cl );
	if(wd == NULL)
		return True;

	send_signed_data_type vals[2] ;	
	send_signed_data_type units[2] ;	

	if(! window_is_suitable( wd ))
	  return True; /* Next window please */

	/* Raise the client if allowed */
	if( !get_flags( ArrangeState.flags, ARRANGE_NoRaise ) )
		SendNumCommand ( F_RAISE, NULL, NULL, NULL, wd->client );

	/* This is to indicate that values are in pixels : */
	units[0] = 1 ; 
	units[1] = 1 ;
	 
	vals[0] = ArrangeState.curr_x ; 
	vals[1] = ArrangeState.curr_y ;
	LOCAL_DEBUG_OUT( "moving client %lX \"%s\" to %+d%+d", wd->client, wd->window_name, ArrangeState.curr_x, ArrangeState.curr_y );
	ArrangeState.curr_x += ArrangeState.incx ;
	ArrangeState.curr_y += ArrangeState.incy ;
	
	SendNumCommand ( F_MOVE, NULL, &(vals[0]), &(units[0]), wd->client );

	if( get_flags( ArrangeState.flags, ARRANGE_Resize ) )
	{
		/* No-Stretch */
		if ( get_flags( ArrangeState.flags, ARRANGE_NoStretch) &&
		     (wd->frame_rect.width < ArrangeState.max_width))
			return True;
		
		vals[0] = ArrangeState.max_width ; 
		vals[1] = ArrangeState.max_height ;
		SendNumCommand ( F_RESIZE, NULL, &(vals[0]), &(units[0]), wd->client );
	}


	return True;   
}
	

void 
cascade_windows()
{
	ArrangeState.curr_x = ArrangeState.offset_x ;
	ArrangeState.curr_y = ArrangeState.offset_y ;
	
	
	iterate_asbidirlist( ArrangeState.clients_order, cascade_window,
			     NULL, NULL, get_flags( ArrangeState.flags, ARRANGE_Reversed));
}

Bool
fix_area(void *data, void *list)
{
	ASWindowData *wd = fetch_window_by_id( ((client_item *) data)->cl );
	
	if(wd == NULL)
		return True;
	
	if( get_flags( wd->flags, AS_AvoidCover ) && ! get_flags( wd->state_flags, AS_Iconic) ) 
	{	
		subtract_rectangle_from_list( (ASVector *) list, wd->frame_rect.x, wd->frame_rect.y, 
					      wd->frame_rect.x+(int)wd->frame_rect.width,
					      wd->frame_rect.y+(int)wd->frame_rect.height );	  
	}
	return True;
}

void 
fix_available_area()
{
	ASVector *list = create_asvector( sizeof(XRectangle) );
    XRectangle seed_rect;
	int i, largest = 0 ;
	XRectangle *rects ;

    /* must seed the list with the single rectangle representing the area : */
    seed_rect.x = 0 ;
    seed_rect.y = 0 ;
    seed_rect.width = Scr.MyDisplayWidth ;
    seed_rect.height = Scr.MyDisplayHeight ;

    append_vector( list, &seed_rect, 1 );
	

    iterate_asbidirlist( ArrangeState.clients_order, fix_area, list, NULL, False);
    
	print_rectangles_list(list);

	i = PVECTOR_USED(list);
	rects = PVECTOR_HEAD(XRectangle,list);
	while( --i > 0 ) 
	{	
    	if( rects[largest].width*rects[largest].height < rects[i].width*rects[i].height ) 
			largest = i ; 
	}

	if( !get_flags( ArrangeState.flags, ARRANGE_OffsetX_Set ) )
		ArrangeState.offset_x = rects[largest].x;
		
	if( !get_flags( ArrangeState.flags, ARRANGE_OffsetY_Set ) )
		ArrangeState.offset_y = rects[largest].y;
				
	if( !get_flags( ArrangeState.flags, ARRANGE_MaxWidth_Set ) )
		ArrangeState.max_width = rects[largest].width;
	
	if( !get_flags( ArrangeState.flags, ARRANGE_MaxHeight_Set ) )
		ArrangeState.max_height = rects[largest].height;
}

