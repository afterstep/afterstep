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

#include "../../include/asapp.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/screen.h"
#include "../../include/functions.h"
#include "../../include/mystyle.h"
#include "../../include/decor.h"

#include "asinternals.h"

#define WIN_HINTS_DO_NOT_COVER    (1<<5)	   /*attempt to not cover this window */

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

/****************************************************************************
 *
 * Checks the function "function", and sees if it
 * is an allowed function for window t,  according to the motif way of life.
 * This routine is used to decide if we should refuse to perform a function.
 *
 ****************************************************************************/
int
check_allowed_function2 (int func, ASHints *hints)
{
	if (func == F_NOP)
		return 0;

    if( hints )
	{
        int mask = function2mask( func );
        if( func == F_SHADE )
            return get_flags( hints->flags, AS_Titlebar ) ;
        else if( mask != 0 )
            return get_flags( hints->function_mask, mask );
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
check_allowed_function (FunctionData *fdata, ASHints *hints)
{
    int func = fdata->func ;
    int i ;
    ComplexFunction *cfunc ;

    if (func != F_FUNCTION)
        return check_allowed_function2 (func, hints);

    if( (cfunc = get_complex_function( fdata->name ) ) == NULL )
        return 0;

    for( i = 0 ; i < cfunc->items_num ; ++i )
        if( cfunc->items[i].func == F_FUNCTION )
        {
            if( check_allowed_function (&(cfunc->items[i]), hints) == 0 )
                return 0;
        }else if( check_allowed_function2 (cfunc->items[i].func, hints) == 0 )
            return 0;
    return 1;
}


ASFlagType
compile_titlebuttons_mask (ASHints *hints)
{
    ASFlagType mask = hints->disabled_buttons ;
    int           i;

	for (i = 0; i < TITLE_BUTTONS; i++)
	{
        if (Scr.Look.buttons[i].unpressed.image != NULL )
        {
            int context = IsLeftButton(i)?(context = C_L1 << i):(C_R1 << (i-FIRST_RIGHT_TBTN));
            MouseButton  *func;

            for (func = Scr.Feel.MouseButtonRoot; func != NULL; func = (*func).NextButton)
                if ( (func->Context & context) != 0 )
                    if( !check_allowed_function (func->fdata,hints ) )
                    {
                        mask |= 0x01<<i ;
                        break;
                    }
        }
    }
    return mask;
}

int
estimate_titlebar_size( ASHints *hints )
{
    int width = 0 ;
    if( hints )
    {
        ASTBarData *tbar = create_astbar();
        ASFlagType btn_mask = compile_titlebuttons_mask (hints);
        tbar->h_spacing = DEFAULT_TBAR_SPACING ;
        tbar->v_spacing = DEFAULT_TBAR_SPACING ;
        /* left buttons : */
        add_astbar_btnblock(tbar, 0, 0, 0, NO_ALIGN,
                            &(Scr.Look.buttons[0]), ~btn_mask,
                            TITLE_BUTTONS_PERSIDE,
                            Scr.Look.TitleButtonXOffset, Scr.Look.TitleButtonYOffset, Scr.Look.TitleButtonSpacing,
                            TBTN_ORDER_L2R, C_L1 );
        /* label */
        add_astbar_label(   tbar, 1, 0, 0, ALIGN_LEFT, hints->names[0]);
        /* right buttons : */
        add_astbar_btnblock(tbar, 2, 0, 0, NO_ALIGN,
                            &(Scr.Look.buttons[TITLE_BUTTONS_PERSIDE]), (~btn_mask)>>TITLE_BUTTONS_PERSIDE,
                            TITLE_BUTTONS_PERSIDE,
                            Scr.Look.TitleButtonXOffset, Scr.Look.TitleButtonYOffset, Scr.Look.TitleButtonSpacing,
                            TBTN_ORDER_R2L, C_R1 );

        set_astbar_style_ptr( tbar, BAR_STATE_UNFOCUSED, Scr.Look.MSMenu[MENU_BACK_TITLE] );
        set_astbar_style_ptr( tbar, BAR_STATE_FOCUSED, Scr.Look.MSMenu[MENU_BACK_HILITE] );
        width = calculate_astbar_width( tbar );
        destroy_astbar( &tbar );
    }
    return width;
}


