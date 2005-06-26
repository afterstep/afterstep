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
#include "../../libAfterStep/ascommand.h"
#include "../../libAfterStep/operations.h"
#include "../../libAfterStep/screen.h"
#include "../../libAfterBase/aslist.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "../../libAfterConf/afterconf.h"

/* Flags: */
#define WINCOMMAND_ActOnAll	    (0x01<<0)
#define WINCOMMAND_Desk             (0x01<<1)
#define WINCOMMAND_AllDesks         (0x01<<2)


typedef void (*WinC_handler)(ASWindowData *wd);

typedef struct
{
	char *name;
	void (*exec_wrapper)(const char *action);
	void (*init_defaults)(void);
} action_t ;

struct ASWinCommandState
{
	ASFlagType flags ;
	char *pattern;
	ASBiDirList *operations;
	
	int x_dest, y_dest; /* Move */
	int new_width, new_height; /* resize */
	int desk; /* send to desk */

}WinCommandState;

/******** Prototypes *********************/
void no_args_wrapper(const char *action);
void move_wrapper(const char *action);
void send_to_desk_wrapper(const char *action);
void jump_wrapper(const char *action);
void resize_wrapper(const char *action);

void default_defaults(void);
void jump_defaults(void);
/*****************************************/

action_t Actions[] = 
{
	{"center", no_args_wrapper, default_defaults },
	{"center jump", jump_wrapper, jump_defaults },
	{"iconify", no_args_wrapper, default_defaults },
	{"jump", jump_wrapper, jump_defaults},
	{"kill", no_args_wrapper, default_defaults},
	{"move", move_wrapper, default_defaults},
	{"resize", resize_wrapper, default_defaults},
	{"sendtodesk", send_to_desk_wrapper, default_defaults},
	{ NULL, NULL, NULL}
};

action_t *get_action_by_name(const char *needle)
{
	int i;
	LOCAL_DEBUG_OUT("needle: %s", needle);

	for( i = 0; Actions[i].name != NULL; i++)
		if(mystrcasecmp(Actions[i].name, needle) == 0)
			return &Actions[i];
	LOCAL_DEBUG_OUT("get_action_by_name returns NULL");
	return NULL;
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
Quit_WinCommand(void)
{
	destroy_asbidirlist( &(WinCommandState.operations) );
}

/* init-defaults */

void default_defaults(void)
{
	set_flags( WinCommandState.flags, WINCOMMAND_ActOnAll );
}

void jump_defaults(void)
{
	set_flags( WinCommandState.flags, WINCOMMAND_AllDesks );
	clear_flags( WinCommandState.flags, WINCOMMAND_ActOnAll );
}

/* exec_wrappers */

void
no_args_wrapper(const char *action)
{
	LOCAL_DEBUG_OUT("no_args_wrapper called: %s", action);
	select_windows_by_pattern
		(WinCommandState.pattern, False, False);
	ascom_do(action, NULL);
}

void
move_wrapper(const char *action)
{
	move_params p;
	
	LOCAL_DEBUG_OUT("move_wrapper called: %s", action);
	
	p.x = WinCommandState.x_dest;
	p.y = WinCommandState.y_dest;
	select_windows_by_pattern
		(WinCommandState.pattern, False, False);
	ascom_do(action, &p);
}

void
send_to_desk_wrapper(const char *action)
{
	send_to_desk_params p;
	p.desk = WinCommandState.desk;
	
	LOCAL_DEBUG_OUT("send_to_desk_wrapper called: %s, desk = %d",
			action, p.desk);
	
	select_windows_by_pattern
		(WinCommandState.pattern, False, False);
	ascom_do(action, &p);
}

void
jump_wrapper(const char *action)
{
	LOCAL_DEBUG_OUT("jump_wrapper called: %s", action);

	select_windows_by_pattern
		(WinCommandState.pattern, True, False);

	ascom_do(action, NULL);
}

void
resize_wrapper(const char *action)
{
	resize_params p;
	
	p.width = WinCommandState.new_width;
	p.height = WinCommandState.new_height;

	ascom_do("resize", &p);
}

int
main( int argc, char **argv )
{
	int i ;
	ASBiDirElem *curr;
	char *command;
	action_t *a;

	InitMyApp (CLASS_WINCOMMAND, argc, argv, NULL, NULL, OPTION_SINGLE|OPTION_RESTART );
	ConnectX( ASDefaultScr, 0 );

	
	/* Initialize State */
	memset( &WinCommandState, 0x00, sizeof(WinCommandState));
	WinCommandState.operations = create_asbidirlist( NULL );
	
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
			/* Resize */
			else if( mystrcasecmp ( argv[i], "-width") == 0 && i+1 < argc && argv[i] != NULL)
			{
				i++;
				WinCommandState.new_width = atopixel ( argv[i], Scr.MyDisplayWidth);
			}else if( mystrcasecmp ( argv[i], "-height") == 0 && i+1 < argc && argv[i] != NULL)
			{
				i++;
				WinCommandState.new_height = atopixel ( argv[i], Scr.MyDisplayHeight);
			}else if( mystrcasecmp ( argv[i], "-new_desk") == 0 && i+1 < argc && argv[i] != NULL)
			{
				i++;
				WinCommandState.desk = atoi( argv[i] );
			}
			
			/* generic */
			else if( mystrcasecmp( argv[i], "-all") == 0)
				set_flags( WinCommandState.flags, WINCOMMAND_ActOnAll );
			
			else if( mystrcasecmp( argv[i], "-alldesks") == 0)
				set_flags( WinCommandState.flags, WINCOMMAND_AllDesks );
			
			else if( mystrcasecmp( argv[i], "-desk") == 0)
				set_flags( WinCommandState.flags, WINCOMMAND_Desk );
			
			else if( mystrcasecmp( argv[i], "-pattern") == 0 && i+1 < argc && argv[i] != NULL)
			{
				WinCommandState.pattern = argv[i+1];
				i++;
			}

		}else
		{	
			LOCAL_DEBUG_OUT("Adding operation: %s", argv[i]);
			append_bidirelem(WinCommandState.operations, argv[i]);
		}
	}
	
	if( WinCommandState.pattern == NULL)
		WinCommandState.pattern = DEFAULT_PATTERN;
	
	ascom_init();
	ascom_update_winlist();

	/* execute default_handlers */
	for( curr = WinCommandState.operations->head;
	     curr != NULL; curr = curr->next)
		if ( (a = get_action_by_name( (char *) curr->data)) )
			a->init_defaults();
	
	/* honor flags */
	if( get_flags( WinCommandState.flags, WINCOMMAND_Desk))
		select_windows_on_desk(False);
	else if( ! get_flags( WinCommandState.flags, WINCOMMAND_AllDesks))
		select_windows_on_screen(False);

	select_windows_by_pattern(WinCommandState.pattern,
				  !get_flags(WinCommandState.flags, WINCOMMAND_ActOnAll), False);
	
	/* apply operations */
	for( curr = WinCommandState.operations->head;
	     curr != NULL; curr = curr->next)
	{
		command = (char *) curr->data;
		LOCAL_DEBUG_OUT("command: %s", command);
		
		if ( (a = get_action_by_name( (char *) curr->data)) )
			a->exec_wrapper((char *) curr->data);
		
	}
	
	ascom_wait();
	ascom_deinit();
	Quit_WinCommand();
	
	return 0 ;
}

