/*
 * Copyright (C) 2005 Fabian Yamaguchi <fabiany at gmx.net>
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

#define LOCAL_DEBUG
#define EVENT_TRACE

#include "../configure.h"
#include "asapp.h"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>
#include <string.h>
#include <stdio.h>

#include "../libAfterImage/afterimage.h"

#include "afterstep.h"
#include "screen.h"
#include "module.h"
#include "mystyle.h"
#include "mystyle_property.h"
#include "parser.h"
#include "clientprops.h"
#include "wmprops.h"
#include "decor.h"
#include "aswindata.h"
#include "balloon.h"
#include "event.h"
#include "shape.h"

#include "../libAfterBase/aslist.h"
#include "../libAfterBase/ashash.h"


#include "ascommand.h"
#include "operations.h"

/* local definitions */

typedef void (*WinC_handler) (ASWindowData *wd, void *data );

typedef struct
{
	Window cl;
}client_item;

char *DEFAULT_PATTERN = "";


void DeadPipe(int);
Bool process_message ( send_data_type type, send_data_type *body );

ASBiDirList *extract_matches(ASBiDirList *src, const char *pattern);
Bool apply_operations(void *data, void *aux_data);
void destroy_client_item(void *data);
int atopixel (char *s, int size);


ASWinCommandState WinCommandState;

/*************** Public ****************/

/* Initialize ascom. A connection to the afterstep-sever
 * is established and all handlers are registered
 */

void
ascom_init ( int *arg_c, char **arg_v[] )
{
	int argc = *arg_c;
	char **argv = *arg_v;

	/* Save our program name - for error messages */
	set_DeadPipe_handler(DeadPipe);
	InitMyApp (CLASS_WINCOMMAND, argc, argv, NULL, NULL, OPTION_SINGLE|OPTION_RESTART );
	
	set_signal_handler( SIGSEGV );
	
	ConnectX( ASDefaultScr, 0 );
	
	memset( &WinCommandState, 0x00, sizeof(WinCommandState));
	
	WinCommandState.selected_wins = create_asbidirlist( NULL );
	WinCommandState.clients_order = create_asbidirlist( destroy_client_item );
	WinCommandState.operations = create_asbidirlist( NULL );
	
	WinCommandState.handlers = create_ashash(7, string_hash_value, string_compare,
						 string_destroy_without_data);
	
	/* Register handlers */
	add_hash_item(WinCommandState.handlers, AS_HASHABLE(strdup("move")), move_handler);
	add_hash_item(WinCommandState.handlers, AS_HASHABLE(strdup("resize")), resize_handler);
	add_hash_item(WinCommandState.handlers, AS_HASHABLE(strdup("kill")), kill_handler);
	add_hash_item(WinCommandState.handlers, AS_HASHABLE(strdup("jump")), jump_handler);
	add_hash_item(WinCommandState.handlers, AS_HASHABLE(strdup("ls")), ls_handler);
	add_hash_item(WinCommandState.handlers, AS_HASHABLE(strdup("iconify")), iconify_handler);
		
	
	
	ConnectAfterStep (WINDOW_CONFIG_MASK |
			  WINDOW_NAME_MASK |
			  M_END_WINDOWLIST |
			  M_NEW_DESKVIEWPORT, 0);
	
}


/*
  Clean up.
*/

void
ascom_deinit(void)
{
	destroy_ashash( &(WinCommandState.handlers) );
       
	destroy_asbidirlist( &(WinCommandState.operations) );
	destroy_asbidirlist( &(WinCommandState.selected_wins) );
	destroy_asbidirlist( &(WinCommandState.clients_order) );
	DeadPipe (0);
}



void
ascom_update_winlist( void )
{
	Bool complete = False;
	void clear_selection();
	SendInfo ("Send_WindowList", 0);
	
	while( ! complete )
	{
		ASMessage *msg = CheckASMessage (WAIT_AS_RESPONSE_TIMEOUT);
		if (msg)
		{
			complete = process_message (msg->header[1], msg->body);
			DestroyASMessage (msg);
		}
	}

}


/*
  op can contain an unlimited amount of
  commands separated by whitespace. These
  commands are then executed in order.
*/
void
ascom_do( const char *op, void *data )
{
	char *iter, *haystack, *copy;

	LOCAL_DEBUG_OUT("ascom_do called: op = %s", op);

	purge_asbidirlist( WinCommandState.operations );
	
	copy = haystack = strdup( op );
	while ( (iter = strtok( haystack , " ") ) )
	{
		LOCAL_DEBUG_OUT("Adding operation: %s", iter);
		append_bidirelem(WinCommandState.operations, strdup(iter) );	
		haystack = NULL;
	}

	free( copy );

	
	if(WinCommandState.selected_wins->head == NULL)
	{
		LOCAL_DEBUG_OUT("No windows were selected.");
		return;
	}
	
	
	iterate_asbidirlist( WinCommandState.selected_wins, apply_operations, data,
			     NULL, False);
	
}

void
ascom_set_flag( ASFlagType fl )
{
	set_flags(WinCommandState.flags, fl);
}

/*
  Before exiting, make sure to call this function.
  This'll make sure all commands have been sent
  to the server.
*/

void
ascom_wait( void )
{
	/* Hack: Request another window-list. Next time we
	 * receive M_END_WINDOWLIST we can be sure all of our
	 * move/resize/whatever commands have been executed and
	 * it's safe to die. */
	ascom_update_winlist();
}


/* Selection-functions */
void
select_all( Bool add)
{
	ASBiDirElem *curr;
	

	if( ! add )
	{
		purge_asbidirlist( WinCommandState.selected_wins );
		return;
	}
	
	for( curr = WinCommandState.clients_order->head;
	     curr != NULL; curr = curr->next)
	{
		
		append_bidirelem( WinCommandState.selected_wins, curr->data );
	}
	
}

void
select_windows_by_pattern( const char *pattern, Bool add, Bool just_one)
{
	ASBiDirElem *curr;
	ASWindowData *wd;
	regex_t my_reg;
	
	if(regcomp( &my_reg, pattern, REG_EXTENDED | REG_ICASE ) != 0)
	{
		LOCAL_DEBUG_OUT("Error compiling regex");
		return;
	}
	
	for( curr = WinCommandState.clients_order->head;
	     curr != NULL; curr = curr->next)
	{
		wd = fetch_window_by_id( ((client_item *)curr->data)->cl );
		
		/* If the pattern matches */
		if(regexec( &my_reg, wd->window_name, 0, NULL, 0) == 0)
		{
			if( add )
				append_bidirelem( WinCommandState.selected_wins, curr->data );
			else
				discard_bidirelem ( WinCommandState.selected_wins, curr->data ); 
			if( just_one )
			{
				regfree(&my_reg);
				return;
			}		
		}	
	}
	
	regfree(&my_reg);
}

void
select_windows_on_screen( Bool add)
{
	ASBiDirElem *curr;
	ASWindowData *wd;

	for( curr = WinCommandState.clients_order->head;
	     curr != NULL; curr = curr->next)
	{
		
		wd = fetch_window_by_id( ((client_item *)curr->data)->cl );
		
		/* skip this window if it's not on current-desk. */
		if( (wd->desk != Scr.CurrentDesk) )
			continue;
		/* skip this window if it's not on current-screen */
		if((  ( wd->frame_rect.x < Scr.Vx )
		       || ( wd->frame_rect.y < Scr.Vy )
		       || ( wd->frame_rect.x > (Scr.MyDisplayWidth + Scr.Vx) )
		       || ( wd->frame_rect.y > (Scr.MyDisplayHeight + Scr.Vy) ))
			)
			continue;
		
		if( add )
			append_bidirelem( WinCommandState.selected_wins, curr->data );
		else
			discard_bidirelem ( WinCommandState.selected_wins, curr->data );
		
	}
	
}

void
select_windows_on_desk( Bool add)
{
	ASBiDirElem *curr;
	ASWindowData *wd;

	for( curr = WinCommandState.clients_order->head;
	     curr != NULL; curr = curr->next)
	{
		
		wd = fetch_window_by_id( ((client_item *)curr->data)->cl );
		
		/* skip this window if it's not on current-desk. */
		if( (wd->desk != Scr.CurrentDesk) )
			continue;
		
		if( add )
			append_bidirelem( WinCommandState.selected_wins, curr->data );
		else
			discard_bidirelem ( WinCommandState.selected_wins, curr->data );
		
	}
	
}

/* not working right now */
void
select_focused_window( Bool add )
{
	ASBiDirElem *curr;
	ASWindowData *wd;

	for( curr = WinCommandState.clients_order->head;
	     curr != NULL; curr = curr->next)
	{
		
		wd = fetch_window_by_id( ((client_item *)curr->data)->cl );
		
		if( wd->focused )
		{
			if( add )
				append_bidirelem( WinCommandState.selected_wins, curr->data );
			else
				discard_bidirelem ( WinCommandState.selected_wins, curr->data );
			
			return;
		}		

	}

}


void
clear_selection( void )
{
	purge_asbidirlist(WinCommandState.selected_wins );
}

/****************** /public **********************/

/****************** private *************************/

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


Bool
apply_operations(void *data, void *aux_data)
{
	ASWindowData *wd = fetch_window_by_id( ((client_item *) data)->cl );
	ASBiDirElem *curr;
	void *h;
	
	for(curr = (WinCommandState.operations)->head;
	    curr != NULL; curr = curr->next)
	{
		/* If lookup wasn't successful, move along */
		if(get_hash_item( WinCommandState.handlers,
				  AS_HASHABLE(curr->data), &h) != ASH_Success)
		{
			LOCAL_DEBUG_OUT("handler %s not found", (char *) curr->data);
			continue;
		}		
		((WinC_handler) h) ( wd, aux_data );
	}
	return True;
}

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

/****************************************************************************/
/* PROCESSING OF AFTERSTEP MESSAGES :                                       */
/****************************************************************************/
Bool
process_message (send_data_type type, send_data_type *body)
{
	client_item *new_item;
	WindowPacketResult res;
	ASBiDirElem *curr;

	LOCAL_DEBUG_OUT( "received message %lX", type );
  
	if( type == M_END_WINDOWLIST )
	{
		
		return True;

	}else if( (type&WINDOW_PACKET_MASK) != 0 )
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );
		
		if( (res = handle_window_packet( type, body, &wd )) == WP_DataCreated )
		{
			new_item = safemalloc( sizeof(client_item) );
			new_item->cl = wd->client;
			append_bidirelem( WinCommandState.clients_order, new_item);
		
		}else if( res == WP_DataDeleted )
		{
			/* Delete element */
			for( curr = WinCommandState.clients_order->head;
			     curr != NULL; curr = curr->next)
			{
				if( ((client_item *)(curr->data)) ->cl  == wd->client)
					destroy_bidirelem( WinCommandState.clients_order, curr);
			}
			
		}
	
	}else if( type == M_NEW_DESKVIEWPORT )
	{
		LOCAL_DEBUG_OUT("M_NEW_DESKVIEWPORT(desk = %ld,Vx=%ld,Vy=%ld)", body[2], body[0], body[1]);
		Scr.CurrentDesk = body[2];
		Scr.Vx = body[0];
		Scr.Vy = body[1];
	}

	return False;
}


/****************** /private ************************/

int main(int argc, char **argv)
{
	move_params mp = {200, 200};
	
	ascom_init( &argc, &argv );
	ascom_update_winlist();
	
	/* select_all( True ); */
	/* select_windows_on_screen(True);*/
	/* select_windows_by_pattern("emacs", False, False); */
	select_focused_window( True );
	
	ascom_do("iconify", &mp);
	
	ascom_wait();
	
	ascom_deinit();
	return 0;
}
