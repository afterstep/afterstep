/*
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

/***********************************************************************
 *
 * afterstep window-list popup code
 *
 ***********************************************************************/

#include "../../configure.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "menus.h"

/* I tried to include "limits.h" to get these values, but it
 * didn't work for some reason */
/* Minimum and maximum values a `signed int' can hold.  */
#define MY_INT_MIN (- MY_INT_MAX - 1)
#define MY_INT_MAX 2147483647

#ifndef NO_WINDOWLIST
static int    winlist_val1;
static int    winlist_val2;
#endif /* ! NO_WINDOWLIST */

extern XContext MenuContext;

/*
 * Change by PRB (pete@tecc.co.uk), 31/10/93.  Prepend a hot key
 * specifier to each item in the list.  This means allocating the
 * memory for each item (& freeing it) rather than just using the window
 * title directly.  */
MenuRoot     *
update_windowList (void)
{
	MenuRoot     *mr = NULL;

#ifndef NO_WINDOWLIST
	int           val1 = winlist_val1;
	int           val2 = winlist_val2;
	ASWindow     *t;
	char         *desk;
	int           last_desk_done = MY_INT_MIN;
	int           next_desk;
	char          scut = '0';				   /* Current short cut key */
	FunctionData  fdata;

	/* find the menu if it is has already been created */
	for (mr = Scr.first_menu; mr != NULL; mr = mr->next)
		if (strncmp ("CurrentDesk: ", mr->name, 13) == 0)
		{
			DeleteMenuRoot (mr);
			break;
		}

	init_func_data (&fdata);
	fdata.func = F_TITLE;

	desk = string_from_int (Scr.CurrentDesk);
	fdata.name = safemalloc (strlen (desk) + 13 + 1);
	sprintf (fdata.name, "CurrentDesk: %s", desk);
	free (desk);

	mr = NewMenuRoot (fdata.name);
	MenuItemFromFunc (mr, &fdata);

	next_desk = 0;
	while (next_desk != MY_INT_MAX)
	{
		/* Sort window list by desktop number */
		if ((val1 < 2) && (val1 > -2))
		{
			next_desk = MY_INT_MAX;
			for (t = Scr.ASRoot.next; t != NULL; t = t->next)
			{
				if ((ASWIN_DESK(t) > last_desk_done) && (ASWIN_DESK(t) < next_desk))
					next_desk = ASWIN_DESK(t);
			}
		} else if ((val1 < 4) && (val1 > -4))
		{
			if (last_desk_done == MY_INT_MIN)
				next_desk = Scr.CurrentDesk;
			else
				next_desk = MY_INT_MAX;
		} else
		{
			if (last_desk_done == MY_INT_MIN)
				next_desk = val2;
			else
				next_desk = MY_INT_MAX;
		}
		last_desk_done = next_desk;
		for (t = Scr.ASRoot.next; t != NULL; t = t->next)
		{
			if ((ASWIN_DESK(t) == next_desk) && (!get_flags(t->hints->flags, AS_SkipWinList)))
			{
				fdata.func = F_RAISE_IT;
				fdata.name = mystrdup ((val1 & 0x0001) ? ASWIN_ICON_NAME(t) : ASWIN_NAME(t));
				fdata.func_val[0] = (val_type) t;
				fdata.func_val[1] = (val_type) t->w;
				if (++scut == ('9' + 1))
					scut = 'A';				   /* Next shortcut key */
				fdata.hotkey = scut;
				MenuItemFromFunc (mr, &fdata);
			}
		}
	}
	MakeMenu (mr);
/*  mr->is_transient = True; */
#endif
	return mr;
}

void
do_windowList (int val1, int val2)
{
#ifndef NO_WINDOWLIST
	MenuRoot     *mr;

	winlist_val1 = val1;
	winlist_val2 = val2;
	mr = update_windowList ();
	do_menu (mr, NULL);
#endif
}
