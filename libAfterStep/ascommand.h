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

void ascom_init( void );
void ascom_deinit( void );

void ascom_update_winlist( void );
char **ascom_get_win_names( void );
XRectangle *ascom_get_available_area( void );

void ascom_do( const char *operation, void *data);
void ascom_do_one(const char *operation, void *data, Bool last);

/* use with care and don't go casting 'round the constness. */
const ASWindowData *ascom_get_next_window(Bool last);

void ascom_pop_winlist(Bool last);
Bool winlist_is_empty(void);

void ascom_wait( void ); /* wait until all operations have been executed */

/* selection */
void select_all( Bool unselect );
Bool select_windows_by_pattern(const char *pattern, Bool just_one, Bool unselect);
void select_windows_on_screen( Bool unselect );
void select_windows_on_desk ( Bool unselect );
void select_windows_by_flag( ASFlagType flag, Bool unselect );
void select_windows_by_state_flag( ASFlagType flag, Bool unselect);
void select_untitled_windows( Bool unselect);
void select_focused_window( Bool unselect );

int num_selected_windows( void );

void clear_selection( void );

/* manipulate ascommand's state. */
void ascom_set_flag( ASFlagType flag );

#endif
