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

/*#define DO_CLOCKING      */
#define LOCAL_DEBUG
#define EVENT_TRACE

#include "../../configure.h"
#include "../../libAfterStep/asapp.h"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>

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

/* Flags: */
#define WINCOMMAND_ActOnAll	(0x01<<0)
#define WINCOMMAND_Desk         (0x01<<1)
#define WINCOMMAND_AllDesks     (0x01<<2)

typedef void (*WinC_handler)(ASWindowData *wd);

struct ASWinCommandState
{
	ASFlagType flags ;
	char *pattern;
	ASBiDirList *clients_order;
	ASBiDirList *operations;
	ASHashTable *handlers;
	
	int x_dest, y_dest; /* Move */

}WinCommandState;

typedef struct
{
	Window cl;
}client_item;

/* Prototypes: */

void GetBaseOptions (const char *filename);
void HandleEvents();
void process_message (send_data_type type, send_data_type *body);
void DeadPipe(int);

ASBiDirList *extract_matches(ASBiDirList *src, const char *pattern);
Bool apply_operations(void *data, void *aux_data);
void destroy_client_item(void *data);
int atopixel (char *s, int size);
void Quit_WinCommand(void);

/* Prototypes - Handlers: */

void move_handler(ASWindowData *wd);
void kill_handler(ASWindowData *wd);
void jump_handler(ASWindowData *wd);
void ls_handler(ASWindowData *wd);

void move_handler(ASWindowData *wd)
{
	/* used by SendNumCommand */
	send_signed_data_type vals[2] ;	
	send_signed_data_type units[2] ;
	
	LOCAL_DEBUG_OUT("Move handler called");

	/* Indicate that we're talking pixels. */
	units[0] = units[1] = 1;
	vals[0] = WinCommandState.x_dest; vals[1] = WinCommandState.y_dest;
	/* Move window */
	SendNumCommand ( F_MOVE, NULL, &(vals[0]), &(units[0]), wd->client );
	
}

void kill_handler(ASWindowData *wd)
{
	LOCAL_DEBUG_OUT("Kill handler called");
}

void jump_handler(ASWindowData *wd)
{
	/* used by SendNumCommand */
	send_signed_data_type vals[2] ;	
	send_signed_data_type units[2] ;
	
	LOCAL_DEBUG_OUT("Jump handler called");

	/* Indicate that we're talking pixels. */
	units[0] = units[1] = 1;
	vals[0] = -1;
	vals[1] = -1;

	/* Deiconify window if necessary */
	if(get_flags( wd->state_flags, AS_Iconic))
		SendNumCommand(F_ICONIFY, NULL, &(vals[0]), &(units[0]), wd->client);
	/* Give window focus */
	SendNumCommand(F_FOCUS, NULL, &(vals[0]), &(units[0]), wd->client);
}

void ls_handler(ASWindowData *wd)
{
	printf("%s\n", wd->window_name);
}

/* Returns a list of windows which match the given pattern
   or NULL if no window matches. */

ASBiDirList *
extract_matches(ASBiDirList *src, const char *pattern)
{
	regex_t my_reg;
	ASBiDirElem *curr;
	ASWindowData *wd;
	ASBiDirList *dest = NULL;

	/* No destruction-function passed so that nothing is
	   destroyed when list is destroyed since these client_items
	   still have to be available to clients_order. */
	
	if(regcomp( &my_reg, pattern, REG_EXTENDED | REG_ICASE ) != 0)
	{
		LOCAL_DEBUG_OUT("Error compiling regex");
		return NULL;
	}
	
        /* Foreach element of the window-list */
	for( curr = src->head; curr != NULL; curr = curr->next)
	{
		wd = fetch_window_by_id( ((client_item *)curr->data)->cl );
		/* If the pattern matches */
		if(regexec( &my_reg, wd->window_name, 0, NULL, 0) == 0)
		{
			if(!dest)
				dest = create_asbidirlist( NULL );
			
			LOCAL_DEBUG_OUT("%s matches pattern", wd->window_name);
			append_bidirelem( dest, curr->data);
			if( !get_flags( WinCommandState.flags, WINCOMMAND_ActOnAll))
			{
				regfree(&my_reg);
				return dest;
			}		
		}
	}
	regfree(&my_reg);
	return dest;
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
		((WinC_handler) h) ( wd );
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


void
HandleEvents(void)
{
    while (True)
    {
        module_wait_pipes_input ( process_message );
    }
}

void
Quit_WinCommand(void)
{
	destroy_ashash( &(WinCommandState.handlers) );
	destroy_asbidirlist( &(WinCommandState.operations) );
	destroy_asbidirlist( &(WinCommandState.clients_order) );
	DeadPipe (0);
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
	ReloadASEnvironment( NULL, NULL, NULL, False, False );
}


/****************************************************************************/
/* PROCESSING OF AFTERSTEP MESSAGES :                                       */
/****************************************************************************/
void
process_message (send_data_type type, send_data_type *body)
{
	ASBiDirList *matches;
	client_item *new_item;
	LOCAL_DEBUG_OUT( "received message %lX", type );
  
	if( type == M_END_WINDOWLIST )
	{
		matches = extract_matches(WinCommandState.clients_order, WinCommandState.pattern );
		if(!matches)
		{
			LOCAL_DEBUG_OUT("No windows matches pattern %s", WinCommandState.pattern);
			Quit_WinCommand();
		}
		
		iterate_asbidirlist( matches, apply_operations, NULL,
				     NULL, False);
		
		destroy_asbidirlist( &matches );

		Quit_WinCommand();

	}else if( (type&WINDOW_PACKET_MASK) != 0 )
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );
		
		if( handle_window_packet( type, body, &wd ) == WP_DataCreated )
		{
			new_item = safemalloc( sizeof(client_item) );
			new_item->cl = wd->client;
			append_bidirelem( WinCommandState.clients_order, new_item);
		}
	}else if( type == M_NEW_DESKVIEWPORT )
	{
		LOCAL_DEBUG_OUT("M_NEW_DESKVIEWPORT(desk = %ld,Vx=%ld,Vy=%ld)", body[2], body[0], body[1]);
		Scr.CurrentDesk = body[2];
		Scr.Vx = body[0];
		Scr.Vy = body[1];
	}
}


int
main( int argc, char **argv )
{
	int i ;
	Bool pattern_read = False;

	/* Save our program name - for error messages */
	set_DeadPipe_handler(DeadPipe);
	InitMyApp (CLASS_WINCOMMAND, argc, argv, NULL, NULL, OPTION_SINGLE|OPTION_RESTART );
	
	set_signal_handler( SIGSEGV );
	
	ConnectX( ASDefaultScr, 0 );
	
	memset( &WinCommandState, 0x00, sizeof(WinCommandState));

	WinCommandState.clients_order = create_asbidirlist( destroy_client_item );
	WinCommandState.operations = create_asbidirlist( NULL );
	WinCommandState.handlers = create_ashash(7, string_hash_value, string_compare,
						 string_destroy_without_data);
	
	/* Register handlers */
	add_hash_item(WinCommandState.handlers, AS_HASHABLE(strdup("move")), move_handler);
	add_hash_item(WinCommandState.handlers, AS_HASHABLE(strdup("kill")), kill_handler);
	add_hash_item(WinCommandState.handlers, AS_HASHABLE(strdup("jump")), jump_handler);
	add_hash_item(WinCommandState.handlers, AS_HASHABLE(strdup("ls")), ls_handler);
	
	/* Traverse arguments */
	for( i = 1 ; i< argc ; ++i)
	{
		if(argv[i] == NULL)
			continue;
		
		/* If it's a flag */
		if(argv[i][0] == '-')
		{
			/* Move */
			if(mystrcasecmp ( argv[i], "-x") == 0 && i+1 < argc && argv[i] != NULL)
			{
				i++;
				WinCommandState.x_dest = atopixel( argv[i], Scr.MyDisplayWidth);
			}else if( mystrcasecmp ( argv[i], "-y") == 0 && i+1 < argc && argv[i] != NULL)
			{
				i++;
				WinCommandState.y_dest = atopixel( argv[i], Scr.MyDisplayHeight);
			}
			/* generic */
			else if( mystrcasecmp( argv[i], "-all") == 0)
				set_flags( WinCommandState.flags, WINCOMMAND_ActOnAll );
				
		}else
		{
			/*It's a pattern then. */
			if( !pattern_read )
			{	
			
				WinCommandState.pattern = argv[i];
				pattern_read = True;
			}else
			{
				/*Pattern was read already.
				  This is an operation. */
				append_bidirelem(WinCommandState.operations, argv[i]);
			}
		}
	}
	
	if(!pattern_read)
	{
		LOCAL_DEBUG_OUT("No pattern specified.");
		return 0;
	}

    ConnectAfterStep (WINDOW_CONFIG_MASK |
                      WINDOW_NAME_MASK |
                      M_END_WINDOWLIST|
		      M_NEW_DESKVIEWPORT, 0);

    /* Request a list of all windows, while we load our config */
    SendInfo ("Send_WindowList", 0);

    LoadBaseConfig ( GetBaseOptions);
    
	/* And at long last our main loop : */
    HandleEvents();
	return 0 ;
}
