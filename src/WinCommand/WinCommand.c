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
#define WINCOMMAND_ActOnAll	(0x01<<0)
#define WINCOMMAND_Desk         (0x01<<1)
#define WINCOMMAND_AllDesks     (0x01<<2)

typedef void (*WinC_handler)(ASWindowData *wd);

struct ASWinCommandState
{
	ASFlagType flags ;
	char *pattern;
	ASBiDirList *operations;
	
	int x_dest, y_dest; /* Move */
	int new_width, new_height; /* resize */

}WinCommandState;


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


int
main( int argc, char **argv )
{
	int i ;
	ASBiDirElem *curr;
	char *command;

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
			}
			
			/* generic */
			else if( mystrcasecmp( argv[i], "-all") == 0)
				set_flags( WinCommandState.flags, WINCOMMAND_ActOnAll );
			
			else if( mystrcasecmp( argv[i], "-pattern") == 0 && i+1 < argc && argv[i] != NULL)
				WinCommandState.pattern = argv[i+1];
			
		}else
		{	
			LOCAL_DEBUG_OUT("Adding operation: %s", argv[i]);
			append_bidirelem(WinCommandState.operations, argv[i]);
		}
	}
	
	if( WinCommandState.pattern == NULL)
		WinCommandState.pattern = DEFAULT_PATTERN;
	
	ascom_init(&argc, &argv);
	ascom_update_winlist();

	/* apply operations */
	for( curr = WinCommandState.operations->head;
	     curr != NULL; curr = curr->next)
	{
		command = (char *) curr->data;
		LOCAL_DEBUG_OUT("command: %s", command);
		
		/* if this command needs no arguments and just_one = false */
		if(mystrcasecmp ( command, "iconify") == 0 || 
		   mystrcasecmp (command, "kill")  == 0||
		   mystrcasecmp(command, "ls") == 0)
		{
			select_windows_by_pattern(WinCommandState.pattern, True, False);
			ascom_do(command, NULL);
		}
		else if(mystrcasecmp ( command, "jump") == 0)
		{
			select_windows_by_pattern(WinCommandState.pattern, True, True);
			ascom_do(command, NULL);
		}
		else if(mystrcasecmp ( command, "move") == 0)
		{
			move_params params;
			params.x = WinCommandState.x_dest;
			params.y = WinCommandState.y_dest;
			select_windows_by_pattern(WinCommandState.pattern, True, False);
			ascom_do(command, &params);
		}
		else if(mystrcasecmp ( command, "resize") == 0)
		{
			resize_params params;
			params.width = WinCommandState.new_width;
			params.height = WinCommandState.new_height;
			select_windows_by_pattern(WinCommandState.pattern, True, False);
			ascom_do(command, &params);
		}
		
	}
	
	ascom_wait();
	ascom_deinit();
	Quit_WinCommand();
	
	return 0 ;
}
