/*
 * Copyright (C) 1998 Ethan Fischer <allanon@crystaltokyo.com>
 * Based on code by Guylhem Aznar <guylhem@oeil.qc.ca>
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

#define LOCAL_DEBUG

#include "../configure.h"

#include <X11/Intrinsic.h>
#include "asapp.h"
#include "mystyle.h"
#include "event.h"
#include "screen.h"
#include "decor.h"
#include "balloon.h"

static ASBalloonState BalloonState = {{0},NULL, NULL, NULL, None};

static void balloon_timer_handler (void *data);

static void
set_active_balloon_look()
{
    int x = 0, y = 0;
    unsigned int width, height ;
    if( BalloonState.active_bar )
    {
        int pointer_x, pointer_y ;
        int dl, dr, du, dd ;
        set_astbar_style_ptr( BalloonState.active_bar, BAR_STATE_UNFOCUSED, BalloonState.look.style );
        set_astbar_hilite( BalloonState.active_bar, BAR_STATE_UNFOCUSED, BalloonState.look.border_hilite );
        width = calculate_astbar_width( BalloonState.active_bar );
        if( width > ASDefaultScrWidth )
            width = ASDefaultScrWidth ;
        height = calculate_astbar_height( BalloonState.active_bar );
        if( height > ASDefaultScrHeight )
            height = ASDefaultScrHeight ;

        ASQueryPointerRootXY(&pointer_x, &pointer_y);
        x = pointer_x;
        y = pointer_y;
        x += BalloonState.look.xoffset ;
        if( x < 0 )
            x = 0 ;
        else if( x + width > ASDefaultScrWidth )
            x = ASDefaultScrWidth - width ;

        y += BalloonState.look.yoffset ;
        if( y < 0 )
            y = 0 ;
        else if( y + height > ASDefaultScrHeight )
            y = ASDefaultScrHeight - height ;
        /* we have to make sure that new window will not be under the pointer,
         * which will cause LeaveNotify and create a race condition : */
        dl = pointer_x - x ;
        if( dl >= 0 )
        {
            dr = x+ (int)width - pointer_x;
            if( dr >= 0 )
            {
                if( x+dl >= ASDefaultScrWidth )
                    dl = dr+1 ;                /* dl is not a good alternative since it
                                                * will move us off the screen */
                du = pointer_y - y ;
                if( du >= 0 )
                {
                    dd = y + (int)height - pointer_y ;
                    if( dd >= 0 )
                    {
                        int dh = min(dl,dr);
                        int dv = min(du,dd);
                        if( y+du >= ASDefaultScrHeight )
                        {
                            du = dd+1 ;
                            dv = dd ;
                        }
                        if( dh < dv )
                        {
                            if( dl > dr )
                                x -= dh+1 ;
                            else
                                x += dh+1 ;
                        }else if( du > dd )
                            y -= dv+1 ;
                        else
                            y += dv+1 ;
                    }
                }
            }
        }
        LOCAL_DEBUG_OUT( "Pointer at (%dx%d)", pointer_x, pointer_y );

        moveresize_canvas( BalloonState.active_canvas, x, y, width, height );
        handle_canvas_config( BalloonState.active_canvas );
        set_astbar_size( BalloonState.active_bar, width, height );
        move_astbar( BalloonState.active_bar, BalloonState.active_canvas, 0, 0 );
        render_astbar( BalloonState.active_bar, BalloonState.active_canvas );
        update_canvas_display (BalloonState.active_canvas);
    }
}

static void
display_active_balloon()
{
    LOCAL_DEBUG_CALLER_OUT( "active(%p)->window(%lX)->canvas(%p)", BalloonState.active, BalloonState.active_window, BalloonState.active_canvas );
    if( BalloonState.active != NULL && BalloonState.active_window != None &&
        BalloonState.active_canvas == NULL )
    {
		int prx, pry ;
		ASTBarData *tbar = BalloonState.active->owner ;
        while (timer_remove_by_data (BalloonState.active));
		/* we must check if active_window still has mouse pointer !!! */
		if( ASQueryPointerRootXY( &prx, &pry ) )
			if( prx < tbar->root_x && pry < tbar->root_y &&
				prx >= tbar->root_x + tbar->width && pry >= tbar->root_y + tbar->height )
			{
				LOCAL_DEBUG_OUT( "active_geom = %dx%d%+d%+d, pointer at %+d%+d",
								 tbar->width, tbar->height, tbar->root_x, tbar->root_y, prx, pry );
				BalloonState.active = NULL ;
				BalloonState.active_window = None ;
				return ;
			}
        BalloonState.active_canvas = create_ascanvas(BalloonState.active_window);
        BalloonState.active_bar = create_astbar();
        add_astbar_label( BalloonState.active_bar, 0, 0, 0, ALIGN_CENTER, 1, 1, BalloonState.active->text, BalloonState.active->encoding );
        set_active_balloon_look();
        map_canvas_window( BalloonState.active_canvas, True );

        BalloonState.active->timer_action = BALLOON_TIMER_CLOSE;
        if( BalloonState.look.close_delay > 0 )
            timer_new (BalloonState.look.close_delay, &balloon_timer_handler, (void *)BalloonState.active);
    }
}

void
withdraw_active_balloon()
{
    if( BalloonState.active != NULL )
    {
        while (timer_remove_by_data (BalloonState.active));
        if( BalloonState.active_bar != NULL )
            destroy_astbar( &(BalloonState.active_bar) );
        if( BalloonState.active_canvas != NULL )
        {
            unmap_canvas_window( BalloonState.active_canvas );
            destroy_ascanvas( &(BalloonState.active_canvas) );
        }
        BalloonState.active = NULL ;
    }
}

static void
balloon_timer_handler (void *data)
{
    ASBalloon      *balloon = (ASBalloon *) data;

    LOCAL_DEBUG_CALLER_OUT( "%p", data );
	switch (balloon->timer_action)
	{
        case BALLOON_TIMER_OPEN:
            if( balloon == BalloonState.active )
                display_active_balloon ();
            break;
        case BALLOON_TIMER_CLOSE:
            if( balloon == BalloonState.active )
                withdraw_active_balloon ();
            break;
	}
}
/*************************************************************************/
void
balloon_init (int free_resources)
{
    LOCAL_DEBUG_CALLER_OUT( "%d", free_resources );
    if( free_resources )
    {
        if( BalloonState.active != NULL )
            withdraw_balloon (BalloonState.active);
        if( BalloonState.active_window != None )
        {
            safely_destroy_window (BalloonState.active_window );
            BalloonState.active_window = None ;
        }
    }
    memset(&(BalloonState.look), 0x00, sizeof(ASBalloonLook));
    BalloonState.look.close_delay = 10000;
    BalloonState.look.xoffset = 5 ;
    BalloonState.look.yoffset = 5 ;
    BalloonState.active = NULL ;
    BalloonState.active_bar = NULL ;
    BalloonState.active_canvas = NULL ;
    LOCAL_DEBUG_OUT( "Balloon window is %lX", BalloonState.active_window );
}

void
display_balloon( ASBalloon *balloon )
{
    LOCAL_DEBUG_CALLER_OUT( "%p", balloon );
    if( balloon == NULL )
        return;

    LOCAL_DEBUG_OUT( "show = %d, active = %p", BalloonState.look.show, BalloonState.active );
    if( !BalloonState.look.show || BalloonState.active == balloon )
        return;

    if( BalloonState.active != NULL )
        withdraw_active_balloon();

    BalloonState.active = balloon ;
    if( BalloonState.look.delay <= 0 )
        display_active_balloon();
    else
    {
        while (timer_remove_by_data (balloon));
        balloon->timer_action = BALLOON_TIMER_OPEN;
        timer_new (BalloonState.look.delay, &balloon_timer_handler, (void *)balloon);
    }
}

void
withdraw_balloon( ASBalloon *balloon )
{
    LOCAL_DEBUG_OUT( "%p", balloon );
    if( balloon == NULL )
        balloon = BalloonState.active ;
    if( balloon == BalloonState.active )
        withdraw_active_balloon();
}

void
set_balloon_look( ASBalloonLook *blook )
{
    BalloonState.look = *blook ;
    LOCAL_DEBUG_CALLER_OUT( "%lX", BalloonState.active_window );
    if( BalloonState.look.show && BalloonState.active_window == None )
    {
        XSetWindowAttributes attr ;
        attr.override_redirect = True;
		attr.event_mask = ButtonPressMask ;
        BalloonState.active_window = create_visual_window( ASDefaultVisual, ASDefaultRoot, -10, -10, 1, 1, 0, InputOutput, CWOverrideRedirect|CWEventMask, &attr );
        LOCAL_DEBUG_OUT( "Balloon window is %lX", BalloonState.active_window );
    }
    set_active_balloon_look();
}

Bool is_balloon_click( XEvent *xe ) 
{
    if( BalloonState.active_window == None ) 
		return False;
	LOCAL_DEBUG_OUT( "Balloon window is %lX, xbutton.window = %lX, subwindow = %lX", BalloonState.active_window, xe->xbutton.window, xe->xbutton.subwindow );
	return ( xe->xbutton.window == BalloonState.active_window || xe->xbutton.subwindow == BalloonState.active_window );
	
}	 

ASBalloon *
create_asballoon (ASTBarData *owner)
{
    ASBalloon *balloon = NULL ;
    if( owner )
    {
        balloon = safecalloc( 1, sizeof(ASBalloon) );
        balloon->owner = owner ;
    }
    return balloon;
}

ASBalloon *
create_asballoon_with_text ( ASTBarData *owner, const char *text, unsigned long encoding)
{
    ASBalloon *balloon = create_asballoon(owner);
    if( balloon )
	{
        balloon->text = mystrdup(text);
		balloon->encoding = encoding ;
	}
    return balloon;
}

void
destroy_asballoon( ASBalloon **pballoon )
{
    if( pballoon && *pballoon )
    {
        if( *pballoon == BalloonState.active )
            withdraw_active_balloon();
        if( (*pballoon)->text )
            free( (*pballoon)->text );
        free( *pballoon );
        *pballoon = NULL ;
    }
}


void
balloon_set_text (ASBalloon * balloon, const char *text, unsigned long encoding)
{
    if( balloon->text == text )
        return ;
    if( balloon->text )
        free( balloon->text );
    balloon->text = mystrdup( text );
	balloon->encoding = encoding ;
    if( balloon == BalloonState.active &&
        BalloonState.active_bar != NULL )
    {
        if( change_astbar_first_label (BalloonState.active_bar, balloon->text, encoding ) )
            set_active_balloon_look();
    }
}

