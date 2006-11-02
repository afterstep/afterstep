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

static ASBalloonState DefaultBalloonState = { {0, 5, 5, 0, 10000, 0, NULL, 5, 5 },
											  NULL, NULL, NULL, None, NULL};

static void balloon_timer_handler (void *data);

static void
set_active_balloon_look( ASBalloonState *state )
{
    int x = 0, y = 0;
    unsigned int width, height ;
	
	if( state == NULL ) 
		state = &DefaultBalloonState ;
		
    if( state->active_bar )
    {
        int pointer_x, pointer_y ;
        int dl, dr, du, dd ;
		state->active_bar->h_border = state->look.TextPaddingX ;
		state->active_bar->v_border = state->look.TextPaddingY ;
        set_astbar_style_ptr( state->active_bar, BAR_STATE_UNFOCUSED, state->look.Style );
        set_astbar_hilite( state->active_bar, BAR_STATE_UNFOCUSED, state->look.BorderHilite );
        width = calculate_astbar_width( state->active_bar );
        if( width > ASDefaultScrWidth )
            width = ASDefaultScrWidth ;
        height = calculate_astbar_height( state->active_bar );
        if( height > ASDefaultScrHeight )
            height = ASDefaultScrHeight ;

        ASQueryPointerRootXY(&pointer_x, &pointer_y);
        x = pointer_x;
        y = pointer_y;
        x += state->look.XOffset ;
        if( x < 0 )
            x = 0 ;
        else if( x + width > ASDefaultScrWidth )
            x = ASDefaultScrWidth - width ;

        y += state->look.YOffset ;
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

        moveresize_canvas( state->active_canvas, x, y, width, height );
        handle_canvas_config( state->active_canvas );
        set_astbar_size( state->active_bar, width, height );
        move_astbar( state->active_bar, state->active_canvas, 0, 0 );
        render_astbar( state->active_bar, state->active_canvas );
        update_canvas_display (state->active_canvas);
    }
}

static void 
setup_active_balloon_tiles( ASBalloonState *state )
{
	if( state->active->type == ASBalloon_Text )
	    add_astbar_label( state->active_bar, 0, 0, 0, ALIGN_CENTER, 1, 1, state->active->data.text.text, state->active->data.text.encoding );
	else 
	{
		LOCAL_DEBUG_OUT( "balloon is image: filename = \"%s\", image = %p", state->active->data.image.filename, state->active->data.image.image );
		if( state->active->data.image.image == NULL )
		{
			ASImage *im = get_asimage( ASDefaultScr->image_manager, state->active->data.image.filename, ASFLAGS_EVERYTHING, 100 );
			if( im )
			{
				int scale_height = Scr.MyDisplayHeight/5 ; 
				int scale_width = (scale_height * im->width) / im->height ; 
				int tile_width = scale_width; 
				if( scale_width > Scr.MyDisplayWidth/3 )
					tile_width = Scr.MyDisplayWidth/4 ;
				if( im->width > tile_width || im->height > scale_height ) 
				{
					ASImage *tmp = scale_asimage( ASDefaultVisual, im, scale_width, scale_height, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
					safe_asimage_destroy( im );
					im = tmp ; 
					if( tile_width < scale_width ) 
					{
						tmp = tile_asimage( ASDefaultVisual, im, 0, 0, tile_width, scale_height, TINT_NONE, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
						safe_asimage_destroy( im );
						im = tmp ;
					}
				}
				state->active->data.image.image = im ;
			} 
		}	
		if( state->active->data.image.image != NULL ) 
		    add_astbar_icon( state->active_bar, 0, 0, 0, ALIGN_CENTER, state->active->data.image.image );
	}
}


static void
display_active_balloon( ASBalloonState *state )
{
	if( state == NULL ) 
		state = &DefaultBalloonState ; 
    LOCAL_DEBUG_CALLER_OUT( "active(%p)->window(%lX)->canvas(%p)", state->active, state->active_window, state->active_canvas );
    if( state->active != NULL && state->active_window != None &&
        state->active_canvas == NULL )
    {
		ASTBarData *tbar = state->active->owner ;
        while (timer_remove_by_data (state->active));
		/* we must check if active_window still has mouse pointer !!! */
		if( tbar != NULL ) 
		{
			int prx, pry ;
			if( ASQueryPointerRootXY( &prx, &pry ) )
				if( prx < tbar->root_x && pry < tbar->root_y &&
					prx >= tbar->root_x + tbar->width && pry >= tbar->root_y + tbar->height )
				{
					LOCAL_DEBUG_OUT( "active_geom = %dx%d%+d%+d, pointer at %+d%+d",
									 tbar->width, tbar->height, tbar->root_x, tbar->root_y, prx, pry );
					state->active = NULL ;
					state->active_window = None ;
					return ;
				}
		}
        state->active_canvas = create_ascanvas(state->active_window);
        state->active_bar = create_astbar();
		setup_active_balloon_tiles( state );
        set_active_balloon_look(state);
        map_canvas_window( state->active_canvas, True );

        state->active->timer_action = BALLOON_TIMER_CLOSE;
        if( state->look.CloseDelay > 0 )
            timer_new (state->look.CloseDelay, &balloon_timer_handler, (void *)state->active);
    }
}

void
withdraw_active_balloon_from( ASBalloonState *state )
{
	if( state == NULL )
		state = &DefaultBalloonState ; 
    LOCAL_DEBUG_CALLER_OUT( "state = %p, active = %p", state, state->active );

    if( state->active != NULL )
    {
        while (timer_remove_by_data (state->active));
        if( state->active_bar != NULL )
            destroy_astbar( &(state->active_bar) );
        if( state->active_canvas != NULL )
        {
            unmap_canvas_window( state->active_canvas );
            destroy_ascanvas( &(state->active_canvas) );
        }
        state->active = NULL ;
    }
}

void
withdraw_active_balloon()
{
	withdraw_active_balloon_from( NULL );
}

static void
balloon_timer_handler (void *data)
{
    ASBalloon      *balloon = (ASBalloon *) data;
	ASBalloonState *state = balloon->state ; 
	
    LOCAL_DEBUG_CALLER_OUT( "%p", data );
	switch (balloon->timer_action)
	{
        case BALLOON_TIMER_OPEN:
            if( balloon == state->active )
                display_active_balloon (state);
            break;
        case BALLOON_TIMER_CLOSE:
            if( balloon == state->active )
                withdraw_active_balloon_from (state);
            break;
	}
}
/*************************************************************************/
static void
balloon_init_state (ASBalloonState *state, int free_resources)
{
	if( state == NULL ) 
		state = &DefaultBalloonState ; 
		
    LOCAL_DEBUG_CALLER_OUT( "%d", free_resources );
    if( free_resources )
    {
        if( state->active != NULL )
            withdraw_balloon (state->active);
        if( state->active_window != None )
        {
            safely_destroy_window (state->active_window );
            state->active_window = None ;
        }
    }
    memset(&(state->look), 0x00, sizeof(ASBalloonLook));
    state->look.CloseDelay = 10000;
    state->look.XOffset = 5 ;
    state->look.YOffset = 5 ;
    state->active = NULL ;
    state->active_bar = NULL ;
    state->active_canvas = NULL ;
    LOCAL_DEBUG_OUT( "Balloon window is %lX", state->active_window );
}

void
cleanup_default_balloons()
{
	balloon_init_state (NULL, True);
}

ASBalloonState *
create_balloon_state()
{
	ASBalloonState *state = safecalloc( 1, sizeof(ASBalloonState));
	ASBalloonState **c  = &(DefaultBalloonState.next);

	balloon_init_state (state, False);
	
	while( *c != NULL ) c = &((*c)->next);
	*c = state ;  

	return state ;
}

void
destroy_balloon_state(ASBalloonState **pstate)
{
	if( pstate ) 
	{
		if( *pstate ) 
		{
			ASBalloonState **c  = &(DefaultBalloonState.next);
	
			while( *c != NULL && *c != *pstate) c = &((*c)->next);
			if( *c != NULL ) 
				*c = (*c)->next ; 
			(*pstate)->next = NULL ; 
			balloon_init_state (*pstate, True);
			if( *pstate != &DefaultBalloonState ) 
				free( *pstate );
			*pstate = NULL ; 
		}
	}	
}


static void
display_balloon_int( ASBalloon *balloon, Bool ignore_delay )
{
	ASBalloonState *state = balloon->state ; 
	
    LOCAL_DEBUG_OUT( "show = %d, active = %p", state->look.show, state->active );
    if( !state->look.show || state->active == balloon )
        return;

    if( state->active != NULL )
        withdraw_active_balloon_from(state);

    state->active = balloon ;
    if( ignore_delay || state->look.Delay <= 0 )
        display_active_balloon(state);
    else
    {
        while (timer_remove_by_data (balloon));
        balloon->timer_action = BALLOON_TIMER_OPEN;
        timer_new (state->look.Delay, &balloon_timer_handler, (void *)balloon);
    }
}

void
display_balloon( ASBalloon *balloon )
{
    LOCAL_DEBUG_CALLER_OUT( "%p", balloon );
    if( balloon )
		display_balloon_int( balloon, False );
}

void
display_balloon_nodelay( ASBalloon *balloon )
{
    LOCAL_DEBUG_CALLER_OUT( "%p", balloon );
    if( balloon )
		display_balloon_int( balloon, True );
}

void
withdraw_balloon( ASBalloon *balloon )
{
    LOCAL_DEBUG_OUT( "%p", balloon );
    if( balloon == NULL )
        balloon = DefaultBalloonState.active ;
    if( balloon != NULL )
	{
		ASBalloonState *state = balloon->state; 
	    LOCAL_DEBUG_OUT( "state = %p, active = %p", state, state->active );
    	if( balloon == state->active )
        	withdraw_active_balloon_from(state);
	}
}

void
set_balloon_state_look( ASBalloonState *state, ASBalloonLook *blook )
{
	if( state == NULL ) 
		state = &DefaultBalloonState ;
    state->look = *blook ;
    LOCAL_DEBUG_CALLER_OUT( "state = %p, active_window = %lX", state, state->active_window );
    if( state->look.show && state->active_window == None )
    {
        XSetWindowAttributes attr ;
        attr.override_redirect = True;
		attr.event_mask = ButtonPressMask ;
        state->active_window = create_visual_window( ASDefaultVisual, ASDefaultRoot, -10, -10, 1, 1, 0, InputOutput, CWOverrideRedirect|CWEventMask, &attr );
        LOCAL_DEBUG_OUT( "Balloon window is %lX", state->active_window );
    }
    set_active_balloon_look(state);
}

void
set_balloon_look( ASBalloonLook *blook )
{
	set_balloon_state_look( NULL, blook );
}

ASBalloonState * 
is_balloon_click( XEvent *xe ) 
{
	register ASBalloonState *state  = &DefaultBalloonState;

	for( ; state != NULL ; state = state->next)
	    if( state->active_window != None ) 
		{
			LOCAL_DEBUG_OUT( "Balloon window is %lX, xbutton.window = %lX, subwindow = %lX", state->active_window, xe->xbutton.window, xe->xbutton.subwindow );
			if( xe->xbutton.window == state->active_window || 
				xe->xbutton.subwindow == state->active_window )
				break;
		}
	return state ;
}	 

ASBalloon *
create_asballoon_for_state (ASBalloonState *state, ASTBarData *owner)
{
    ASBalloon *balloon = NULL ;
 	
    balloon = safecalloc( 1, sizeof(ASBalloon) );
    balloon->owner = owner ;
	balloon->state = (state==NULL)?&DefaultBalloonState:state;
    return balloon;
}

static void destroy_asballoon_data( ASBalloon *balloon ) 
{
	if( balloon->type == ASBalloon_Text )
	{ 	
       	destroy_string( &(balloon->data.text.text) );
	}else 
	{
       	destroy_string( &(balloon->data.image.filename) );
		if( balloon->data.image.image )
		{
			safe_asimage_destroy( balloon->data.image.image );
			balloon->data.image.image = NULL ;
		}
	}
}
inline static void set_asballoon_data_text( ASBalloon *balloon, const char *text, unsigned long encoding ) 
{
    balloon->data.text.text = mystrdup(text);
	balloon->data.text.encoding = encoding ;
	balloon->type = ASBalloon_Text ; 
}

ASBalloon *
create_asballoon_with_text_for_state ( ASBalloonState *state, ASTBarData *owner, const char *text, unsigned long encoding)
{
    ASBalloon *balloon = create_asballoon_for_state(state, owner);
    if( balloon )
		set_asballoon_data_text( balloon, text, encoding );
    return balloon;
}

ASBalloon *
create_asballoon (ASTBarData *owner)
{
	return  create_asballoon_for_state (NULL, owner);
}

ASBalloon *
create_asballoon_with_text ( ASTBarData *owner, const char *text, unsigned long encoding)
{
	return  create_asballoon_with_text_for_state (NULL, owner, text, encoding);
}

void
destroy_asballoon( ASBalloon **pballoon )
{
    if( pballoon && *pballoon )
    {
		ASBalloonState *state = (*pballoon)->state ; 
        if( *pballoon == state->active )
            withdraw_active_balloon_from(state);
		destroy_asballoon_data( *pballoon );
        free( *pballoon );
        *pballoon = NULL ;
    }
}


void
balloon_set_text (ASBalloon * balloon, const char *text, unsigned long encoding)
{
	if( balloon ) 
	{
		ASBalloonState *state = balloon->state ; 
		int old_type = balloon->type ;
		
    	if( balloon->data.text.text == text )
        	return ;
		destroy_asballoon_data( balloon );
		set_asballoon_data_text( balloon, text, encoding );

    	if( balloon == state->active &&	state->active_bar != NULL )
    	{
			if( old_type != balloon->type ) 
			{
				delete_astbar_tile( state->active_bar, -1 );
				setup_active_balloon_tiles( state );
			}else
        		change_astbar_first_label (state->active_bar, balloon->data.text.text, encoding );
           	set_active_balloon_look(balloon->state);
    	}
	}
}

void
balloon_set_image_from_file (ASBalloon * balloon, const char *filename)
{
	if( balloon && filename ) 
	{
		ASBalloonState *state = balloon->state ; 
		
		destroy_asballoon_data( balloon );
		balloon->data.image.filename = mystrdup( filename );
		balloon->type = ASBalloon_Image ; 

    	if( balloon == state->active &&
        	state->active_bar != NULL )
    	{
			delete_astbar_tile( state->active_bar, -1 );
			setup_active_balloon_tiles( state );
           	set_active_balloon_look(balloon->state);
    	}
	}
}
