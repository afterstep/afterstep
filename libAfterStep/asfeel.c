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

void
init_asfeel( ASFeel *feel )
{
    int i ;

    feel->buttons2grab = 7;
    feel->AutoReverse = 0;
    feel->Xzap = 12;
    feel->Yzap = 12;
    feel->EdgeScrollX = feel->EdgeScrollY = -100000;
    feel->EdgeResistanceScroll = feel->EdgeResistanceMove = 0;
    feel->EdgeAttractionScreen = 20;
    feel->EdgeAttractionWindow = 10;
    feel->OpaqueMove = 5;
    feel->OpaqueResize = 5;
    feel->ClickTime = 150;
    feel->AutoRaiseDelay = 0;
    feel->RaiseButtons = 0;
    feel->flags = DoHandlePageing;
    feel->XorValue = (((unsigned long)1) << Scr.d_depth) - 1;

    feel->no_snaping_mod = ShiftMask|ControlMask ;

    feel->MouseButtonRoot = NULL;
    feel->FuncKeyRoot = NULL;
    feel->Popups = NULL;
    feel->ComplexFunctions = NULL;
    feel->ShadeAnimationSteps = 12;

    for( i = 0 ; i < MAX_CURSORS; ++i )
        if( feel->cursors[i] )
            feel->cursors[i] = Scr.standard_cursors[i] ;
}

ASFeel *
create_asfeel()
{
	ASFeel *feel ;

	feel = safecalloc( 1, sizeof(ASFeel));

    feel->magic = MAGIC_ASFEEL ;
	init_asfeel( feel );
	return feel;
}

void
destroy_asfeel( ASFeel *feel, Bool reusable )
{
	if( feel )
    {
        if(feel->magic == MAGIC_ASFEEL )
        {
            register int i ;

            feel->magic = 0 ;

			while (feel->MouseButtonRoot != NULL)
			{
            	MouseButton  *mb = feel->MouseButtonRoot;

            	feel->MouseButtonRoot = mb->NextButton;
            	if (mb->fdata)
            	{
                	free_func_data( mb->fdata);
                	free (mb->fdata);
            	}
				free (mb);
			}
        	while (feel->FuncKeyRoot != NULL)
			{
            	FuncKey      *fk = feel->FuncKeyRoot;

            	feel->FuncKeyRoot = fk->next;
				if (fk->name != NULL)
					free (fk->name);
            	if (fk->fdata != NULL)
            	{
                	free_func_data(fk->fdata);
                	free (fk->fdata);
            	}
				free (fk);
			}
        	for( i = 0 ; i < MAX_CURSORS; ++i )
            	if( feel->cursors[i] && feel->cursors[i] != Scr.standard_cursors[i] )
            	{
                	XFreeCursor( dpy, feel->cursors[i] );
                	feel->cursors[i] = None ;
            	}
        	if( feel->Popups )
            	destroy_ashash( &feel->Popups );
        	if( feel->ComplexFunctions )
            	destroy_ashash( &feel->ComplexFunctions );
        }
		if( !reusable )
			free(feel);
		else
			memset( feel, 0x00, sizeof(ASFeel) );
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
check_feel_sanity(ASFeel *feel)
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

	feel->EdgeScrollX = feel->EdgeScrollX * Scr.MyDisplayWidth / 100;
    feel->EdgeScrollY = feel->EdgeScrollY * Scr.MyDisplayHeight / 100;

    if( feel->no_snaping_mod == 0 )
        feel->no_snaping_mod = ShiftMask ;

    if (Scr.VxMax == 0)
        clear_flags(feel->flags, EdgeWrapX);
    if (Scr.VyMax == 0)
        clear_flags(feel->flags, EdgeWrapY);

}

/*************************************************************************
 * WindowBox utility functions
 *************************************************************************/
ASWindowBox *create_aswindow_box()
{
	return safecalloc( 1, sizeof(ASWindowBox) );
}

void
destroy_aswindow_box( ASWindowBox **aswbox )
{
	if( aswbox )
		if( *aswbox )
		{
			if( (*aswbox)->name )
				free( (*aswbox)->name );

		}
}



