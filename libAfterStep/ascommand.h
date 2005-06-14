#ifndef AS_COMMAND_HEADER_INCLUDED
#define AS_COMMAND_HEADER_INCLUDED

#include "../configure.h"
#include "asapp.h"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>
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


/* Flags: none right now */

extern char *DEFAULT_PATTERN;

typedef struct
{
	ASFlagType flags ;
	ASBiDirList *clients_order;
	ASBiDirList *selected_wins;
	ASBiDirList *operations;
	ASHashTable *handlers;
	
	int x_dest, y_dest; /* Move */
	int new_width, new_height;

} ASASCommandState;

void ascom_init( int *arg_c, char **arg_v[] );
void ascom_deinit( void );

void ascom_update_winlist( void );
void ascom_do( const char *operation, void *data );

void ascom_wait( void ); /* wait until all operations have been executed */

/* selection */
void select_all(Bool add);
void select_windows_by_pattern(const char *pattern, Bool add, Bool just_one);
void select_windows_on_screen( Bool add );
void select_windows_on_desk ( Bool add );

void select_focused_window( Bool add );

void clear_selection( void );

/* manipulate ascommand's state. */
void ascom_set_flag( ASFlagType flag );

#endif
