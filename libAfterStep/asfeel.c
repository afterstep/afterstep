/*
 * Copyright (c) 2000 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#define LOCAL_DEBUG

#include "../configure.h"

#include "../include/asapp.h"
#include <X11/cursorfont.h>


#include "../include/afterstep.h"
#include "../include/parse.h"
#include "../include/loadimg.h"
#include "../include/parser.h"
#include "../include/confdefs.h"
#include "../include/mystyle.h"
#include "../libAfterImage/afterimage.h"
#include "../include/screen.h"
#include "../include/balloon.h"
#include "../include/asfeel.h"

/**************************************************************************************
 * ASFeel initialization and destruction code.
 *************************************************************************************/

ASFeel *
create_asfeel()
{
	ASFeel *feel ;

	feel = safecalloc( 1, sizeof(ASFeel));

    feel->magic = MAGIC_ASFEEL ;

	set_flags( feel->flags, DoHandlePageing );
    feel->ShadeAnimationSteps = 12 ;

	feel->Xzap = 12;
	feel->Yzap = 12;
	feel->EdgeScrollX = feel->EdgeScrollY = -100000;
    feel->OpaqueMove = feel->OpaqueResize = 5;
	feel->ClickTime = 150;
    feel->XorValue = (((unsigned long)1) << Scr.d_depth) - 1 ;

	feel->no_snaping_mod = ShiftMask|ControlMask;

	return feel;
}

void
destroy_asfeel( ASFeel **feel )
{
	if( feel )
		if( *feel )
        {
            if((*feel)->magic == MAGIC_ASFEEL )
            {
//                register int i ;

                (*feel)->magic = 0 ;

                if( (*feel)->ComplexFunctions )
                    destroy_ashash( &((*feel)->ComplexFunctions));
                if( (*feel)->Popups )
                    destroy_ashash( &((*feel)->Popups));
                /* TODO: we have to do something about the cursors too */
//                free_feel_cursors( *feel );
//                for( i = 0 ; i < MAX_CURSORS ; i++ )
//                    if( (*feel)->custom_cursors[i] )
//                        destroy_ascursor( &((*feel)->custom_cursors[i]) );
            }
			free(*feel);
			*feel = NULL ;
		}
}

static unsigned int default_cursors[MAX_CURSORS] ={
    XC_left_ptr           ,
    XC_left_ptr           ,
    XC_left_ptr           ,
    XC_left_ptr           ,
    XC_fleur              ,
    XC_watch              ,
    XC_left_ptr           ,
    XC_dot                ,
    XC_pirate             ,
    XC_top_side           ,
    XC_right_side         ,
    XC_bottom_side        ,
    XC_left_side          ,
    XC_top_left_corner    ,
    XC_top_right_corner   ,
    XC_bottom_left_corner ,
    XC_bottom_right_corner };

void
set_feel_cursors (ASFeel *feel)
{
    register int i ;
    Cursor c ;
LOCAL_DEBUG_CALLER_OUT( "feel %p", feel);
    if( dpy == NULL || feel == NULL ) return ;
	/* define cursors */
    for( i = 0 ; i < MAX_CURSORS ; i++ )
    {
        c = XCreateFontCursor (dpy, default_cursors[i] );
        feel->cursors[i] = c ;
    }
}

void
apply_feel_cursor( Window w, ASFeel *feel, int cursor )
{
    if( feel && cursor >= 0 && cursor < MAX_CURSORS && w != None)
        XDefineCursor (dpy, w, feel->cursors[cursor]);
}

void
free_feel_cursors (ASFeel *feel)
{
    register int i ;

    if( dpy == NULL || feel == NULL ) return ;
    /* free cursors */
    for( i = 0 ; i < MAX_CURSORS ; i++ )
        if( feel->cursors[i] )
            XFreeCursor (dpy, feel->cursors[i] );
}

void
fix_feel_edge_scroll(ASFeel *feel)
{
LOCAL_DEBUG_CALLER_OUT( "feel %p", feel);
    /* If no edge scroll line is provided in the setup file, use a default */
    if (feel->EdgeScrollX == -100000)
        feel->EdgeScrollX = 25;
    if (feel->EdgeScrollY == -100000)
        feel->EdgeScrollY = feel->EdgeScrollX;

    if (get_flags(feel->flags, ClickToRaise) && (feel->AutoRaiseDelay == 0))
        feel->AutoRaiseDelay = -1;

	/* if edgescroll >1000 and < 100000m
	 * wrap at edges of desktop (a "spherical" desktop) */
    feel->flags &= ~(EdgeWrapX|EdgeWrapY);
    if (feel->EdgeScrollX >= 1000)
	{
        feel->EdgeScrollX /= 1000;
        feel->flags |= EdgeWrapX;
	}
    if (feel->EdgeScrollY >= 1000)
	{
        feel->EdgeScrollY /= 1000;
        feel->flags |= EdgeWrapY;
	}
}




