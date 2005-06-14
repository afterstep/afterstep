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

#include "operations.h"

#define LOCAL_DEBUG
#define EVENT_TRACE

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


#include "ascommand.h"

extern ASWinCommandState WinCommandState;

void move_handler(ASWindowData *wd, void *data)
{
	move_params *params = (move_params *) data;
	/* used by SendNumCommand */
	send_signed_data_type vals[2] ;	
	send_signed_data_type units[2] ;
	
	LOCAL_DEBUG_OUT("Move handler called");

	/* Indicate that we're talking pixels. */
	units[0] = units[1] = 1;
	vals[0] = params->x; vals[1] = params->y;
	/* Move window */
	SendNumCommand ( F_MOVE, NULL, &(vals[0]), &(units[0]), wd->client );
	
}

void resize_handler(ASWindowData *wd, void *data)
{
	resize_params *params = (resize_params *) data;
        /* used by SendNumCommand */
	send_signed_data_type vals[2] ;	
	send_signed_data_type units[2] ;
	
	LOCAL_DEBUG_OUT("Resize handler called");

	/* Indicate that we're talking pixels. */
	units[0] = units[1] = 1;
	vals[0] = params->width; vals[1] = params->height;
	/* Move window */
	SendNumCommand ( F_RESIZE, NULL, &(vals[0]), &(units[0]), wd->client );

}

void kill_handler(ASWindowData *wd, void *data)
{
	LOCAL_DEBUG_OUT("Kill handler called");
	SendNumCommand(F_DESTROY, NULL, NULL, NULL, wd->client);
}

void jump_handler(ASWindowData *wd, void *data)
{
	/* used by SendNumCommand */
	send_signed_data_type vals[1] ;	
	send_signed_data_type units[1] ;
	
	LOCAL_DEBUG_OUT("Jump handler called");

	/* Indicate that we're talking pixels. */
	units[0] = 1;
	vals[0] = -1;
	
	/* Deiconify window if necessary */
	if(get_flags( wd->state_flags, AS_Iconic))
		SendNumCommand(F_ICONIFY, NULL, &(vals[0]), &(units[0]), wd->client);
	/* Give window focus */
	SendNumCommand(F_FOCUS, NULL, NULL, NULL, wd->client);
}

void ls_handler(ASWindowData *wd, void *data)
{
	fprintf( stdout, "Name: %s\n", wd->window_name );
	fprintf( stdout, "X: %ld\n", wd->frame_rect.x );
	fprintf( stdout, "Y: %ld\n", wd->frame_rect.y );
	fprintf( stdout, "Width: %ld\n", wd->frame_rect.width );
	fprintf( stdout, "Height: %ld\n", wd->frame_rect.height );
	fprintf( stdout, "\n" );
}

void iconify_handler(ASWindowData *wd, void *data)
{
	/* used by SendNumCommand */
	send_signed_data_type vals[1] ;	
	send_signed_data_type units[1] ;
	
	LOCAL_DEBUG_OUT("Iconify handler called");

	/* Indicate that we're talking pixels. */
	units[0] = 1;
	vals[0] = 1;
	
	/* Iconify window if not iconified */
	if(! get_flags( wd->state_flags, AS_Iconic))
		SendNumCommand(F_ICONIFY, NULL, &(vals[0]), &(units[0]), wd->client);
	
}

