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

#include <readline/readline.h>
#include <readline/history.h>

#include "../../libAfterConf/afterconf.h"

/* Flags: */
#define WINCOMMAND_ActOnAll	    (0x01<<0)
#define WINCOMMAND_Desk             (0x01<<1)
#define WINCOMMAND_AllDesks         (0x01<<2)


typedef void (*WinC_handler)(ASWindowData *wd);

struct ASWinCommandState;

typedef struct
{
	char *name;
	void (*exec_wrapper)(struct ASWinCommandState *state, const char *action);
	void (*init_defaults)(struct ASWinCommandState *state);
} action_t ;

typedef struct ASWinCommandState
{
	ASFlagType flags ;
	char *pattern;
	
	int x_dest, y_dest; /* Move */
	int new_width, new_height; /* resize */
	int desk; /* send to desk */

}ASWinCommandState;

/******** Prototypes *********************/
void no_args_wrapper(ASWinCommandState *state, const char *action);
void move_wrapper(ASWinCommandState *state, const char *action);
void send_to_desk_wrapper(ASWinCommandState *state, const char *action);
void jump_wrapper(ASWinCommandState *state, const char *action);
void resize_wrapper(ASWinCommandState *state, const char *action);
void group_wrapper(ASWinCommandState *state, const char *action);

void default_defaults(ASWinCommandState *state);
void jump_defaults(ASWinCommandState *state);
/*****************************************/

action_t Actions[] = 
{
	{"center", no_args_wrapper, default_defaults },
	{"center jump", jump_wrapper, jump_defaults },
	{"group", group_wrapper, jump_defaults},
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
atopixel (const char *val, int size)
{
	
  	int l = strlen (val);
	int res = 0; 
  	if (l >= 1)
	{	
  		if (val[l-1] == 'p' || val[l-1] == 'P' )
		{
			char *clean_val = mystrndup( val, l-1 );
			/* number was followed by a p/P
		 	* => number was already a pixel-value */
			res = atoi (clean_val);
		}else /* return s percent of size */
			res = (atoi (val) * size) / 100 ;
	}  	
	return res;
}



/* init-defaults */

void default_defaults(ASWinCommandState *state)
{
	set_flags( state->flags, WINCOMMAND_ActOnAll );
}

void jump_defaults(ASWinCommandState *state)
{
	set_flags( state->flags, WINCOMMAND_AllDesks );
	clear_flags( state->flags, WINCOMMAND_ActOnAll );
}

/* exec_wrappers */

void
no_args_wrapper(ASWinCommandState *state, const char *action)
{
	LOCAL_DEBUG_OUT("no_args_wrapper called: %s", action);
	
	ascom_do(action, NULL);
}

void
move_wrapper(ASWinCommandState *state, const char *action)
{
	move_params p;
	
	LOCAL_DEBUG_OUT("move_wrapper called: %s", action);
	
	p.x = state->x_dest;
	p.y = state->y_dest;
	
	ascom_do(action, &p);
}

void
send_to_desk_wrapper(ASWinCommandState *state, const char *action)
{
	send_to_desk_params p;
	p.desk = state->desk;
	
	LOCAL_DEBUG_OUT("send_to_desk_wrapper called: %s, desk = %d",
			action, p.desk);
	
	ascom_do(action, &p);
}

void
jump_wrapper(ASWinCommandState *state, const char *action)
{
	LOCAL_DEBUG_OUT("jump_wrapper called: %s", action);
	ascom_do(action, NULL);
}

void
resize_wrapper(ASWinCommandState *state, const char *action)
{
	resize_params p;
	
	p.width = state->new_width;
	p.height = state->new_height;

	ascom_do("resize", &p);
}

/* DO NOT RUN WITH SUID-PRIVILIDGES */
void
group_wrapper(ASWinCommandState *state, const char *action)
{
#define WINTABS_COMMAND_PATTERN "WinTabs --pattern \"posix:%s\" &"	
	if( state->pattern != NULL ) 
	{	
		char* com = safemalloc(sizeof(WINTABS_COMMAND_PATTERN)+strlen(state->pattern)) ;
		/* Launch WinTabs */
		sprintf(com, WINTABS_COMMAND_PATTERN, state->pattern);
		SendTextCommand ( F_MODULE, "-", com, None);
		free( com );
	}
}

typedef enum ASWinCommandParamCode
{
	ASWC_BadParam = -2,
	ASWC_BadVal = -1,			
	ASWC_Ok_ValUnused = 0,
	ASWC_Ok_ValUsed = 1
}ASWinCommandParamCode;
	
ASWinCommandParamCode    
set_WinCommandParam( ASWinCommandState *state, const char *param, const char *val )
{
	int val_used = 0 ; 
	if( state == NULL || param == NULL ) 
		return ASWC_BadParam;	  

	if( param[0] == '-' ) 
		++param ;
	if( mystrcasecmp ( param, "x") == 0)
	{
		if( val == NULL || !isdigit(val[0]) ) 
			return ASWC_BadVal;
		state->x_dest = atopixel( val, Scr.MyDisplayWidth);
		++val_used;
	}else if( mystrcasecmp ( param, "y") == 0)
	{
		if( val == NULL || !isdigit(val[0]) ) 
			return ASWC_BadVal;
		state->y_dest = atopixel( val, Scr.MyDisplayHeight);
		++val_used;
	}else			/* Resize */
		if( mystrcasecmp ( param, "width") == 0 )
	{
		if( val == NULL || !isdigit(val[0]) ) 
			return ASWC_BadVal;
		state->new_width = atopixel ( val, Scr.MyDisplayWidth);
		++val_used;
	}else if( mystrcasecmp ( param, "height") == 0 )
	{
		if( val == NULL || !isdigit(val[0]) ) 
			return ASWC_BadVal;
		state->new_height = atopixel ( val, Scr.MyDisplayHeight);
		++val_used;
	}else if( mystrcasecmp ( param, "new_desk") == 0 )
	{
		if( val == NULL || !isdigit(val[0]) ) 
			return ASWC_BadVal;
		state->desk = atoi( val );
		++val_used;
	}
			
	/* generic */
	else if( mystrcasecmp( param, "all") == 0)
		set_flags( state->flags, WINCOMMAND_ActOnAll );
	else if( mystrcasecmp( param, "alldesks") == 0)
		set_flags( state->flags, WINCOMMAND_AllDesks );
	else if( mystrcasecmp( param, "desk") == 0)
		set_flags( state->flags, WINCOMMAND_Desk );
	else if( mystrcasecmp( param, "pattern") == 0 )
	{
		if( val == NULL ) 
			return ASWC_BadVal;
		destroy_string( &(state->pattern) );
		state->pattern = mystrdup(val);
		++val_used;
	}else
	{
		return ASWC_BadParam;	  		
	}
	return ( val_used > 0 )?ASWC_Ok_ValUsed:ASWC_Ok_ValUnused;	  
}

int
main( int argc, char **argv )
{
	int i ;
	ASBiDirElem *curr;
	char *command;
	action_t *a;
	ASWinCommandState WinCommandState ; 

	InitMyApp (CLASS_WINCOMMAND, argc, argv, NULL, NULL, OPTION_SINGLE|OPTION_RESTART );
	ConnectX( ASDefaultScr, 0 );

	ASBiDirList *operations = create_asbidirlist( NULL );
	
	/* Initialize State */
	memset( &WinCommandState, 0x00, sizeof(WinCommandState));
	

	/* Traverse arguments */
	for( i = 1 ; i< argc ; ++i)
	{
		if(argv[i] == NULL)
			continue;
		
		/* If it's a flag */
		if(argv[i][0] == '-')
		{
			switch( set_WinCommandParam( &WinCommandState, argv[i], (i+1<argc)?argv[i+1]:NULL ) )
			{
				case ASWC_BadParam :
				case ASWC_BadVal :	
					fprintf( stderr, "bad parameter [%s]\n", argv[i] );
					break; 		   
				case ASWC_Ok_ValUsed :
					++i;
				case ASWC_Ok_ValUnused :
					break ;
			}	 
		}else				
		{	
			LOCAL_DEBUG_OUT("Adding operation: %s", argv[i]);
			append_bidirelem(operations, argv[i]);
		}
	}
	
	if( WinCommandState.pattern == NULL)
		WinCommandState.pattern = mystrdup(DEFAULT_PATTERN);
	
	if( operations->count > 0 ) 
	{	
		ascom_init();
		ascom_update_winlist();

		/* execute default_handlers */
		for( curr = operations->head; curr != NULL; curr = curr->next)
			if ( (a = get_action_by_name( (char *) curr->data)) )
				a->init_defaults(&WinCommandState);
	
		/* honor flags */
		if( get_flags( WinCommandState.flags, WINCOMMAND_Desk))
			select_windows_on_desk(False);
		else if( ! get_flags( WinCommandState.flags, WINCOMMAND_AllDesks))
			select_windows_on_screen(False);

		if ( ! select_windows_by_pattern(WinCommandState.pattern,
			 !get_flags(WinCommandState.flags, WINCOMMAND_ActOnAll), False) )
		LOCAL_DEBUG_OUT("warning: invalid pattern. Reverting to default.");
	
		/* apply operations */
		for( curr = operations->head;  curr != NULL; curr = curr->next)
		{
			command = (char *) curr->data;
			LOCAL_DEBUG_OUT("command: %s", command);
		
			if ( (a = get_action_by_name( (char *) curr->data)) )
				a->exec_wrapper(&WinCommandState, (char *) curr->data);
		}
		ascom_wait();
		ascom_deinit();
	}else/* interactive mode */
	{
		char *line_read = NULL ;
		while( (line_read = readline(">")) != NULL )
		{
			char *ptr = line_read; 
			char *cmd = NULL ; 
			
			ptr = parse_token (ptr, &cmd);
			if( cmd != NULL && cmd[0] != '\0' ) 
			{	
				if( mystrcasecmp(cmd, "quit") == 0 )
					break;
				if( mystrcasecmp(cmd, "set") == 0 )
				{
					char *param = 	NULL ; 
					ptr = parse_token (ptr, &param);
					while( isspace(*ptr) ) ++ptr ;
					switch( set_WinCommandParam( &WinCommandState, param, ptr) )
					{
						case ASWC_BadParam :
						case ASWC_BadVal :	
							printf("bad parameter [%s]\n", argv[i] );
							break; 		   
						case ASWC_Ok_ValUsed :
						case ASWC_Ok_ValUnused :
							add_history (line_read);
							printf( "ok\n");
							break ;
					}	 
				}else if( (a = get_action_by_name( cmd )) )
				{	
					a->init_defaults(&WinCommandState);

					ascom_init();
					ascom_update_winlist();
					if( get_flags( WinCommandState.flags, WINCOMMAND_Desk))
						select_windows_on_desk(False);
					else if( ! get_flags( WinCommandState.flags, WINCOMMAND_AllDesks))
						select_windows_on_screen(False);

					if ( ! select_windows_by_pattern(WinCommandState.pattern,
			 		 	!get_flags(WinCommandState.flags, WINCOMMAND_ActOnAll), False) )
					LOCAL_DEBUG_OUT("warning: invalid pattern. Reverting to default.");
				
					a->exec_wrapper(&WinCommandState, ptr);

					ascom_wait();
					ascom_deinit();
					add_history (line_read);
					printf( "ok\n");
	 			}else
				{
					/* try to parse it as AS function */	
					printf( "bad command\n");
				}	 
				free( cmd ) ;
			}
			free( line_read );
		}
		printf( "\nbye bye\n" );		   
	}	 
	destroy_asbidirlist( &operations );	

	return 0 ;
}

