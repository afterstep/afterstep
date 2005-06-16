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
Bool selection_in_progress = False;


ASBiDirList *extract_matches(ASBiDirList *src, const char *pattern);
Bool apply_operations(void *data, void *aux_data);
void destroy_client_item(void *data);


ASASCommandState ASCommandState;

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
			append_bidirelem( ASCommandState.clients_order, new_item);
		
		}else if( res == WP_DataDeleted )
		{
			/* Delete element */
			for( curr = ASCommandState.clients_order->head;
			     curr != NULL; curr = curr->next)
			{
				if( ((client_item *)(curr->data)) ->cl  == wd->client)
					destroy_bidirelem( ASCommandState.clients_order, curr);
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

Bool
apply_operations(void *data, void *aux_data)
{
	ASWindowData *wd = fetch_window_by_id( ((client_item *) data)->cl );
	ASBiDirElem *curr;
	void *h;
	
	for(curr = (ASCommandState.operations)->head;
	    curr != NULL; curr = curr->next)
	{
		/* If lookup wasn't successful, move along */
		if(get_hash_item( ASCommandState.handlers,
				  AS_HASHABLE(curr->data), &h) != ASH_Success)
		{
			LOCAL_DEBUG_OUT("handler %s not found", (char *) curr->data);
			continue;
		}		
		LOCAL_DEBUG_OUT("executing handler %s for %s", (char *) curr->data, wd->window_name);
		((WinC_handler) h) ( wd, aux_data );
	}
	return True;
}

void
destroy_client_item(void *data)
{
	free((client_item *) data);
}


/****************** /private ************************/


/*************** Public ****************/

/* Initialize ascom. A connection to the afterstep-sever
 * is established and all handlers are registered
 */

void
ascom_init ( void )
{

	set_DeadPipe_handler(DeadPipe);
	set_signal_handler( SIGSEGV );
	
	
	memset( &ASCommandState, 0x00, sizeof(ASCommandState));
	
	ASCommandState.selected_wins = create_asbidirlist( NULL );
	ASCommandState.clients_order = create_asbidirlist( destroy_client_item );
	ASCommandState.operations = create_asbidirlist( NULL );
	
	ASCommandState.handlers = create_ashash(7, string_hash_value, string_compare,
						 string_destroy_without_data);
	
	/* Register handlers */
	add_hash_item(ASCommandState.handlers, AS_HASHABLE(strdup("move")), move_handler);
	add_hash_item(ASCommandState.handlers, AS_HASHABLE(strdup("resize")), resize_handler);
	add_hash_item(ASCommandState.handlers, AS_HASHABLE(strdup("kill")), kill_handler);
	add_hash_item(ASCommandState.handlers, AS_HASHABLE(strdup("jump")), jump_handler);
	add_hash_item(ASCommandState.handlers, AS_HASHABLE(strdup("ls")), ls_handler);
	add_hash_item(ASCommandState.handlers, AS_HASHABLE(strdup("iconify")), iconify_handler);
	add_hash_item(ASCommandState.handlers, AS_HASHABLE(strdup("sendtodesk")), send_to_desk_handler);
	add_hash_item(ASCommandState.handlers, AS_HASHABLE(strdup("center")), center_handler);

	
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
	destroy_ashash( &(ASCommandState.handlers) );
       
	destroy_asbidirlist( &(ASCommandState.operations) );
	destroy_asbidirlist( &(ASCommandState.selected_wins) );
	destroy_asbidirlist( &(ASCommandState.clients_order) );
	DeadPipe (0);
}



void
ascom_update_winlist( void )
{
	Bool complete = False;
	clear_selection();
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

/* run ascom_update_winlist first */
char **
ascom_get_win_names( void )
{
	ASBiDirElem *curr;
	ASWindowData *wd;
	int n_names = 0;
	char **ret = NULL;
	int i = 0;

	if(ASCommandState.clients_order->head == NULL)
		return NULL;

	for( curr = ASCommandState.clients_order->head;
	     curr != NULL; curr = curr->next)
		n_names++;
	
	ret = safemalloc(sizeof(char *) * n_names + 1);
	
	for( curr = ASCommandState.clients_order->head;
	     curr != NULL; curr = curr->next)
	{
		wd = fetch_window_by_id( ((client_item *)curr->data)->cl );
		ret[i++] = strdup(wd->window_name);
	}
	
	ret[n_names] = NULL;
	
	return ret;
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

	destroy_asbidirlist( &ASCommandState.operations );
	ASCommandState.operations = create_asbidirlist(NULL);

	copy = haystack = strdup( op );
	while ( (iter = strtok( haystack , " ") ) )
	{
		LOCAL_DEBUG_OUT("Adding operation: %s", iter);
		append_bidirelem(ASCommandState.operations, strdup(iter) );	
		haystack = NULL;
	}

	free( copy );

	
	if(ASCommandState.selected_wins->head == NULL)
	{
		LOCAL_DEBUG_OUT("No windows were selected.");
		return;
	}
	
	
	iterate_asbidirlist( ASCommandState.selected_wins, apply_operations, data,
			     NULL, False);
	
}

void
ascom_set_flag( ASFlagType fl )
{
	set_flags(ASCommandState.flags, fl);
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
select_all( Bool unselect )
{
	ASBiDirElem *curr;
	ASBiDirList *new_selection = create_asbidirlist(NULL);
	
	if(selection_in_progress)
	  curr = ASCommandState.selected_wins->head;
	else
	  curr = ASCommandState.clients_order->head;
	
	for( ;
	     curr != NULL; curr = curr->next)
	{
		if( !unselect )
			append_bidirelem( new_selection, curr->data );
	}
	
	destroy_asbidirlist(&ASCommandState.selected_wins);
	ASCommandState.selected_wins = new_selection;
	
	selection_in_progress = True;
}

void
select_windows_by_pattern( const char *pattern, Bool just_one, Bool unselect)
{
	ASBiDirElem *curr;
	ASWindowData *wd;
	regex_t my_reg;
	int ret;

	ASBiDirList *new_selection = create_asbidirlist(NULL);
	
	if(regcomp( &my_reg, pattern, REG_EXTENDED | REG_ICASE ) != 0)
	{
		LOCAL_DEBUG_OUT("Error compiling regex");
		return;
	}
	
	if( selection_in_progress)
		curr = ASCommandState.selected_wins->head;
	else
		curr = ASCommandState.clients_order->head;
	
	for( ; curr != NULL; curr = curr->next)
	{
		wd = fetch_window_by_id( ((client_item *)curr->data)->cl );
		
		
                ret = regexec( &my_reg, wd->window_name, 0, NULL, 0);
		if( ((ret == 0) && !unselect) || ( (ret != 0) && unselect) )
		{
			
			append_bidirelem( new_selection, curr->data );
		
			if( new_selection->head && just_one )
			break;
			
		}
		
	}
	
	
	destroy_asbidirlist(&ASCommandState.selected_wins);
	ASCommandState.selected_wins = new_selection;
	regfree(&my_reg);
	selection_in_progress = True;
}

void
select_windows_on_screen( Bool unselect )
{
	ASBiDirElem *curr;
	ASWindowData *wd;
	ASBiDirList *new_selection = create_asbidirlist(NULL);

	if(selection_in_progress)
		curr = ASCommandState.selected_wins->head;
	else
		curr = ASCommandState.clients_order->head;

	for( ;curr != NULL; curr = curr->next)
	{
		
		wd = fetch_window_by_id( ((client_item *)curr->data)->cl );
		

		if( unselect )
		{
		
			/* add this window if it's not on current screen or desk */
			if( (wd->desk != Scr.CurrentDesk) || 
			    
			    (  ( wd->frame_rect.x < Scr.Vx )
			       || ( wd->frame_rect.y < Scr.Vy )
			       || ( wd->frame_rect.x > (Scr.MyDisplayWidth + Scr.Vx) )
			       || ( wd->frame_rect.y > (Scr.MyDisplayHeight + Scr.Vy) ))
			    
				)
				append_bidirelem( new_selection, curr->data );

		}else
		{
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
			
			append_bidirelem( new_selection, curr->data );
		}
		
	}
	
	destroy_asbidirlist(&ASCommandState.selected_wins);
	ASCommandState.selected_wins = new_selection;
	selection_in_progress = True;
}

void
select_windows_on_desk( Bool unselect )
{
	ASBiDirElem *curr;
	ASWindowData *wd;
	ASBiDirList *new_selection = create_asbidirlist(NULL);

	
	if(selection_in_progress)
		curr = ASCommandState.selected_wins->head;
	else
		curr = ASCommandState.clients_order->head;
	

	for( ; curr != NULL; curr = curr->next)
	{
		
		wd = fetch_window_by_id( ((client_item *)curr->data)->cl );
	
		if( (unselect && (wd->desk != Scr.CurrentDesk) ) || 
		    (!unselect && (wd->desk == Scr.CurrentDesk)) )
			append_bidirelem( new_selection, curr->data );
		
	}
	
	destroy_asbidirlist(&ASCommandState.selected_wins);
	ASCommandState.selected_wins = new_selection;
	selection_in_progress = True;
}

/* not working right now */
void
select_focused_window( Bool unselect )
{
	ASBiDirElem *curr;
	ASWindowData *wd;
	ASBiDirList *new_selection = create_asbidirlist(NULL);
	
	if(selection_in_progress)
		curr = ASCommandState.selected_wins->head;
	else
		curr = ASCommandState.clients_order->head;

	for( ;
	     curr != NULL; curr = curr->next)
	{
		
		wd = fetch_window_by_id( ((client_item *)curr->data)->cl );
		
		if( (wd->focused && !unselect) || (!wd->focused && unselect))
		{
			
			append_bidirelem( new_selection, curr->data );
			destroy_asbidirlist(&ASCommandState.selected_wins);
			ASCommandState.selected_wins = new_selection;
			selection_in_progress = True;
			return;
		}		

	}

	destroy_asbidirlist(&ASCommandState.selected_wins);
	ASCommandState.selected_wins = new_selection;
	selection_in_progress = True;
}


void
clear_selection( void )
{
	destroy_asbidirlist(&ASCommandState.selected_wins);
	selection_in_progress = False;
}

/****************** /public **********************/


