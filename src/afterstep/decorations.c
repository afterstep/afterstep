/*
 * Copyright (C) 1996 Frank Fejes
 * Copyright (C) 1996 Alfredo Kojima
 * Copyright (C) 1995 Bo Yang
 * Copyright (C) 1993 Robert Nation
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
 *
 */


/****************************************************************************
 *
 * Reads motif mwm window manager hints from a window, and makes necessary
 * adjustments for afterstep.
 *
 ****************************************************************************/

#include "../../configure.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"

#include "menus.h"

#define WIN_HINTS_DO_NOT_COVER    (1<<5)	   /*attempt to not cover this window */

extern ASWindow *Tmp_win;


unsigned long
GetGnomeState (ASWindow * t)
{
	int           actual_format;
	Atom          actual_type;
	unsigned long nitems, bytesafter;
	unsigned long state = 0;
	unsigned long *data;

	if (XGetWindowProperty (dpy, t->w, _XA_WIN_STATE, 0, 1, False, XA_CARDINAL,
							&actual_type, &actual_format, &nitems, &bytesafter,
							(unsigned char **)&data) == Success && data)
	{
		state = *data;
		XFree (data);
	}

	return state;
}

Bool
is_function_bound_to_button (int b, int function)
{
	MouseButton  *func;
	int           context;

	if (IsLeftButton(b))
		context = C_L1 << b;
	else
		context = C_R1 << (b-FIRST_RIGHT_TBTN);

	for (func = Scr.MouseButtonRoot; func != NULL; func = (*func).NextButton)
		if ((func->Context & context) && (func->func == function))
			return True;
	return False;
}

void
disable_titlebuttons_with_function (ASWindow * t, int function)
{
	int           i;

	for (i = 0; i < TITLE_BUTTONS; i++)
	{
		if (Scr.buttons[i].unpressed.image != NULL &&
			IsBtnEnabled(t,i) && is_function_bound_to_button (i, function))
			DisableBtn(t, i);
	}
}

/****************************************************************************
 *
 * Checks the function "function", and sees if it
 * is an allowed function for window t,  according to the motif way of life.
 * This routine is used to decide if we should refuse to perform a function.
 *
 ****************************************************************************/
int
check_allowed_function2 (int func, ASWindow * t)
{
	if (func == F_NOP)
		return 0;

	if( t )
	{
		ASHints *hints = t->hints ;
		switch( func )
		{
			case F_DELETE :
				if( !get_flags( hints->protocols, AS_DoesWmDeleteWindow ) )
					return 0;
			case F_CLOSE :
				return ( get_flags( hints->function_mask, AS_FuncClose ) )?1:0 ;
			case F_DESTROY :
				return ( get_flags( hints->function_mask, AS_FuncKill ) )?1:0 ;
			case F_MOVE :
				return (get_flags( hints->function_mask, AS_FuncMove ) )?1:0 ;
			case F_RESIZE :
				return (get_flags( hints->function_mask, AS_FuncResize ) )?1:0 ;
			case F_ICONIFY :
				return (!get_flags( t->status->flags, AS_Iconic) &&
				         get_flags( hints->function_mask, AS_FuncMinimize ) )?1:0 ;
			case F_MAXIMIZE :
				return (!get_flags( t->status->flags, AS_MaximizedX|AS_MaximizedY) &&
				         get_flags( hints->function_mask, AS_FuncMaximize ) )?1:0 ;
			case F_SHADE :
				return (!get_flags( t->status->flags, AS_Shaded) &&
				         get_flags( hints->flags, AS_Titlebar ) )?1:0 ;
		}
	}
	return 1;
}
/****************************************************************************
 *
 * Checks the function described in menuItem mi, and sees if it
 * is an allowed function for window Tmp_Win,
 * according to the motif way of life.
 *
 * This routine is used to determine whether or not to grey out menu items.
 *
 ****************************************************************************/
int
check_allowed_function (MenuItem * mi)
{
	/* Complex functions are a little tricky... ignore them for now */
	int func = mi->func ;

	if (func == F_FUNCTION && mi->item != NULL)
	{
		/* Hard part! What to do now? */
		/* Hate to do it, but for lack of a better idea,
		 * check based on the menu entry name */
		if (mystrncasecmp (mi->item, MOVE_STRING, strlen (MOVE_STRING)) == 0)
			func = F_MOVE;
		else if ( mystrncasecmp (mi->item, RESIZE_STRING1, strlen (RESIZE_STRING1)) == 0 ||
		          mystrncasecmp (mi->item, RESIZE_STRING2, strlen (RESIZE_STRING2)) == 0)
			func = F_RESIZE ;
		else if ( mystrncasecmp (mi->item, MINIMIZE_STRING, strlen (MINIMIZE_STRING)) == 0 ||
		          mystrncasecmp (mi->item, MINIMIZE_STRING2, strlen (MINIMIZE_STRING2)) == 0)
			func = F_ICONIFY ;
		else if ( mystrncasecmp (mi->item, MAXIMIZE_STRING, strlen (MAXIMIZE_STRING)) == 0)
			func = F_MAXIMIZE ;
		else if ( mystrncasecmp (mi->item, CLOSE_STRING1, strlen (CLOSE_STRING1)) == 0 ||
				  mystrncasecmp (mi->item, CLOSE_STRING2, strlen (CLOSE_STRING2)) == 0 ||
				  mystrncasecmp (mi->item, CLOSE_STRING3, strlen (CLOSE_STRING3)) == 0 ||
				  mystrncasecmp (mi->item, CLOSE_STRING4, strlen (CLOSE_STRING4)) == 0 )
			func = F_CLOSE;
		else
			return 1;
	}

	return check_allowed_function2 (func, Tmp_win);
}

