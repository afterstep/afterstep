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

#include "../include/asapp.h"
#include "../include/mystyle.h"
#include "../include/event.h"
#include "../include/screen.h"
#include "../include/decor.h"
#include "../include/balloon.h"

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
        set_astbar_hilite( BalloonState.active_bar, BalloonState.look.border_hilite );
        width = calculate_astbar_width( BalloonState.active_bar );
        if( width > Scr.MyDisplayWidth )
            width = Scr.MyDisplayWidth ;
        height = calculate_astbar_height( BalloonState.active_bar );
        if( height > Scr.MyDisplayHeight )
            height = Scr.MyDisplayHeight ;

        ASQueryPointerRootXY(&pointer_x, &pointer_y);
        x = pointer_x;
        y = pointer_y;
        x += BalloonState.look.xoffset ;
        if( x < 0 )
            x = 0 ;
        else if( x + width > Scr.MyDisplayWidth )
            x = Scr.MyDisplayWidth - width ;

        y += BalloonState.look.yoffset ;
        if( y < 0 )
            y = 0 ;
        else if( y + height > Scr.MyDisplayHeight )
            y = Scr.MyDisplayHeight - height ;
        /* we have to make sure that new window will not be under the pointer,
         * which will cause LeaveNotify and create a race condition : */
        dl = pointer_x - x ;
        if( dl >= 0 )
        {
            dr = x+ (int)width - pointer_x;
            if( dr >= 0 )
            {
                if( x+dl >= Scr.MyDisplayWidth )
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
                        if( y+du >= Scr.MyDisplayHeight )
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
//        x = 0 ;
//        y = 500 ;

        moveresize_canvas( BalloonState.active_canvas, x, y, width, height );
        handle_canvas_config( BalloonState.active_canvas );
        set_astbar_size( BalloonState.active_bar, width, height );
        move_astbar( BalloonState.active_bar, BalloonState.active_canvas, 0, 0 );
        render_astbar( BalloonState.active_bar, BalloonState.active_canvas );
    }
}

static void
display_active_balloon()
{
    LOCAL_DEBUG_CALLER_OUT( "active(%p)->window(%lX)->canvas(%p)", BalloonState.active, BalloonState.active_window, BalloonState.active_canvas );
    if( BalloonState.active != NULL && BalloonState.active_window != None &&
        BalloonState.active_canvas == NULL )
    {
        while (timer_remove_by_data (BalloonState.active));
        BalloonState.active_canvas = create_ascanvas(BalloonState.active_window);
        BalloonState.active_bar = create_astbar();
        add_astbar_label( BalloonState.active_bar, 0, 0, 0, ALIGN_CENTER, BalloonState.active->text );
        set_active_balloon_look();
        map_canvas_window( BalloonState.active_canvas, True );

        BalloonState.active->timer_action = BALLOON_TIMER_CLOSE;
        if( BalloonState.look.close_delay > 0 )
            timer_new (BalloonState.look.close_delay, &balloon_timer_handler, (void *)BalloonState.active);
    }
}

static void
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
        BalloonState.active_window = create_visual_window( Scr.asv, Scr.Root, -10, -10, 1, 1, 0, InputOutput, CWOverrideRedirect, &attr );
        LOCAL_DEBUG_OUT( "Balloon window is %lX", BalloonState.active_window );
    }
    set_active_balloon_look();
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
create_asballoon_with_text ( ASTBarData *owner, const char *text)
{
    ASBalloon *balloon = create_asballoon(owner);
    if( balloon )
        balloon->text = mystrdup(text);
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
balloon_set_text (ASBalloon * balloon, const char *text)
{
    if( balloon->text == text )
        return ;
    if( balloon->text )
        free( balloon->text );
    balloon->text = mystrdup( text );
    if( balloon == BalloonState.active &&
        BalloonState.active_bar != NULL )
    {
        if( change_astbar_first_label (BalloonState.active_bar, balloon->text ) )
            set_active_balloon_look();
    }
}

/*************************************************************************/
/* old stuff :                                                           */
/*************************************************************************/
#if 0
Bool          balloon_show = 0;

static void   balloon_draw (Balloon * balloon);
static void   balloon_map (Balloon * balloon);
static void   balloon_unmap (Balloon * balloon);
static void   balloon_timer_handler (void *data);

static Balloon *balloon_first = NULL;
static Balloon *balloon_current = NULL;
static int    balloon_yoffset = 0;
static int    balloon_border_width = 0;
static Pixel  balloon_border_color = 0;
static Window balloon_window = None;
static int    balloon_delay = 0;
static int    balloon_close_delay = 10000;
static MyStyle *balloon_style = NULL;

Bool
balloon_parse (char *tline, FILE * fd)
{
	Bool          handled = False;

	while (isspace (*tline))
		tline++;

	/* check - is this one of our keywords? */
	if (mystrncasecmp (tline, "Balloon", 7))
		return False;

	if (mystrncasecmp (tline, "Balloons", 8) == 0)
	{
		handled = True;
		balloon_show = True;
	} else if (mystrncasecmp (tline, "BalloonBorderColor", 18) == 0)
	{
		int           len;

		handled = True;
		for (tline += 18; isspace (*tline); tline++);
		for (len = 0; tline[len] != '\0' && !isspace (tline[len]); len++);
		tline[len] = '\0';
		parse_argb_color (tline, &balloon_border_color);
	} else if (mystrncasecmp (tline, "BalloonBorderWidth", 18) == 0)
	{
		handled = True;
		balloon_border_width = strtol (&tline[18], NULL, 10);
		if (balloon_border_width < 0)
			balloon_border_width = 0;
	} else if (mystrncasecmp (tline, "BalloonYOffset", 14) == 0)
	{
		handled = True;
		balloon_yoffset = strtol (&tline[14], NULL, 10);
	} else if (mystrncasecmp (tline, "BalloonDelay", 12) == 0)
	{
		handled = True;
		balloon_delay = strtol (&tline[12], NULL, 10);
		if (balloon_delay < 0)
			balloon_delay = 0;
	} else if (mystrncasecmp (tline, "BalloonCloseDelay", 12) == 0)
	{
		handled = True;
		balloon_close_delay = strtol (&tline[12], NULL, 10);
		if (balloon_close_delay < 100)
			balloon_close_delay = 100;
	}

	return handled;
}

/* call this after parsing config files to finish initialization */
void
balloon_setup (Display * dpy)
{
	XSizeHints    hints;
	XSetWindowAttributes attributes;

	attributes.override_redirect = True;
	attributes.border_pixel = balloon_border_color;
	balloon_window = XCreateWindow (dpy, DefaultRootWindow (dpy), 0, 0, 64, 64,
									balloon_border_width, CopyFromParent,
									InputOutput, CopyFromParent, CWOverrideRedirect | CWBorderPixel, &attributes);
	XSelectInput (dpy, balloon_window, ExposureMask);
	hints.flags = USPosition;
	XSetWMNormalHints (dpy, balloon_window, &hints);
}

/* free all resources and reinitialize */
void
balloon_init (int free_resources)
{
	if (free_resources)
	{
		while (balloon_first != NULL)
			balloon_delete (balloon_first);
		if (balloon_window != None);
        safely_destroy_window (balloon_window);
	}

	balloon_first = NULL;
	balloon_current = NULL;
	balloon_yoffset = 0;
	balloon_border_width = 0;
	balloon_border_color = 0;
	balloon_window = None;
	balloon_delay = 0;
	balloon_style = NULL;
	balloon_close_delay = 10000;
}

/*
 * parent must be selecting EnterNotify and LeaveNotify events
 * if balloon_set_rectangle() will be used, parent must be selecting
 * MotionNotify
 */
Balloon      *
balloon_new (Display * dpy, Window parent)
{
	Balloon      *balloon;

	balloon = NEW (Balloon);

	(*balloon).next = balloon_first;
	balloon_first = balloon;

	(*balloon).dpy = dpy;
	(*balloon).parent = parent;
	(*balloon).text = NULL;
	/* a width of -1 means that the whole window triggers the balloon */
	(*balloon).px = (*balloon).py = (*balloon).pwidth = (*balloon).pheight = -1;
	/* an x of -1 means that the balloon should be placed according to YOffset */
	(*balloon).x = (*balloon).y = -1;

	return balloon;
}

Balloon      *
balloon_new_with_text (Display * dpy, Window parent, char *text)
{
	Balloon      *balloon;

	balloon = balloon_new (dpy, parent);
	if (text != NULL)
		(*balloon).text = mystrdup (text);

	return balloon;
}

Balloon      *
balloon_find (Window parent)
{
	Balloon      *balloon;

	for (balloon = balloon_first; balloon != NULL; balloon = (*balloon).next)
		if ((*balloon).parent == parent)
			break;
	return balloon;
}

/* balloon_delete() checks for NULL so that:

 *   balloon_delete(balloon_find(win));
 *
 * will always work
 */
void
balloon_delete (Balloon * balloon)
{
	if (balloon == NULL)
		return;

	if (balloon_current == balloon)
		balloon_unmap (balloon);

	while (timer_remove_by_data (balloon));

	if (balloon_first == balloon)
		balloon_first = (*balloon).next;
	else if (balloon_first != NULL)
	{
		Balloon      *ptr;

		for (ptr = balloon_first; (*ptr).next != NULL; ptr = (*ptr).next)
			if ((*ptr).next == balloon)
				break;
		if ((*ptr).next == balloon)
			(*ptr).next = (*balloon).next;
	}

	if ((*balloon).text != NULL)
		free ((*balloon).text);

	free (balloon);
}

void
balloon_set_style (Display * dpy, MyStyle * style)
{
	Balloon      *tmp = balloon_current;

	if (style == NULL)
		return;

	if (tmp != NULL)
		balloon_unmap (tmp);
	balloon_style = style;
	if (style != NULL)
		XSetWindowBackground (dpy, balloon_window, style->colors.back);
	if (tmp != NULL)
		balloon_map (tmp);
}

void
balloon_set_text (Balloon * balloon, const char *text)
{
	if (balloon)
	{
		if ((*balloon).text != NULL)
			free ((*balloon).text);
		(*balloon).text = (text != NULL) ? mystrdup (text) : NULL;
		if (balloon_current == balloon)
			balloon_draw (balloon);
	}
}

static void
balloon_draw (Balloon * balloon)
{
	XClearWindow (balloon->dpy, balloon_window);
	if (balloon->text != NULL)
		mystyle_draw_text (balloon_window, balloon_style, balloon->text, 2, 1 + balloon_style->font.y);
}

static void
balloon_map (Balloon * balloon)
{
	if (balloon_current == balloon)
		return;
	if ((*balloon).text != NULL)
	{
		int           w = 4;
		int           h = 4;

		mystyle_get_text_geometry (balloon_style, balloon->text, strlen (balloon->text), &w, &h);
		w += 4;
		h += 4;
		/* there can be only one! */
		if (balloon_current != NULL)
			balloon_unmap (balloon_current);
		if ((*balloon).x == -1)
		{
			XWindowChanges wc;
			Window        root;
			int           screen = DefaultScreen ((*balloon).dpy);
			int           x, y, width, height, border, junk;

			XGetGeometry ((*balloon).dpy, (*balloon).parent, &root, &x, &y, &width, &height, &border, &junk);
			XTranslateCoordinates ((*balloon).dpy, (*balloon).parent, root, 0, 0, &wc.x, &wc.y, &root);
			wc.width = w;
			wc.height = h;
			if ((*balloon).pwidth > 0)
			{
				wc.x += (*balloon).px;
				wc.y += (*balloon).py;
				width = (*balloon).pwidth;
				height = (*balloon).pheight;
			}
			wc.x += (width - wc.width) / 2 - balloon_border_width;
			if (balloon_yoffset >= 0)
				wc.y += balloon_yoffset + height + 2 * border;
			else
				wc.y += balloon_yoffset - wc.height - 2 * balloon_border_width;
			/* clip to screen */
			if (wc.x < 2)
				wc.x = 2;
			if (wc.x > DisplayWidth ((*balloon).dpy, screen) - wc.width - 2)
				wc.x = DisplayWidth ((*balloon).dpy, screen) - wc.width - 2;
			XConfigureWindow ((*balloon).dpy, balloon_window, CWX | CWY | CWWidth | CWHeight, &wc);
		} else
			XMoveResizeWindow ((*balloon).dpy, balloon_window, (*balloon).x, (*balloon).y, w, h);
		XMapRaised ((*balloon).dpy, balloon_window);
		balloon_current = balloon;
		balloon->timer_action = BALLOON_TIMER_CLOSE;
		timer_new (balloon_close_delay, &balloon_timer_handler, (void *)balloon);
	}
}

static void
balloon_unmap (Balloon * balloon)
{
	while (timer_remove_by_data (balloon));
	if (balloon_current == balloon)
	{
		XUnmapWindow ((*balloon).dpy, balloon_window);
		balloon_current = NULL;
	}
}

void
balloon_set_active_rectangle (Balloon * balloon, int x, int y, int width, int height)
{
	(*balloon).px = x;
	(*balloon).py = y;
	(*balloon).pwidth = width;
	(*balloon).pheight = height;
}

void
balloon_set_position (Balloon * balloon, int x, int y)
{
	(*balloon).x = x;
	(*balloon).y = y;
	if (balloon == balloon_current)
		XMoveWindow ((*balloon).dpy, balloon_window, x, y);
}

Bool
balloon_handle_event (XEvent * event)
{
	Balloon      *balloon;

	if (!balloon_show)
		return False;

	/* is this our event? */
	if ((*event).type != EnterNotify && (*event).type != LeaveNotify && (*event).type != MotionNotify &&
		(*event).type != Expose)
		return False;

	/* handle expose events specially */
	if ((*event).type == Expose)
	{
		if (event->xexpose.window != balloon_window || balloon_current == NULL)
			return False;
		balloon_draw (balloon_current);
		return True;
	}

	for (balloon = balloon_first; balloon != NULL; balloon = balloon->next)
	{
		if ((*balloon).parent != (*event).xany.window)
			continue;

		/* handle the event */
		switch ((*event).type)
		{
		 case EnterNotify:
			 if (event->xcrossing.detail != NotifyInferior && (*balloon).pwidth <= 0)
			 {
				 if (balloon_delay == 0)
					 balloon_map (balloon);
				 else
				 {
					 while (timer_remove_by_data (balloon));
					 balloon->timer_action = BALLOON_TIMER_OPEN;
					 timer_new (balloon_delay, &balloon_timer_handler, (void *)balloon);
				 }
			 }
			 break;
		 case LeaveNotify:
			 if (event->xcrossing.detail != NotifyInferior)
			 {
				 while (timer_remove_by_data (balloon));
				 balloon_unmap (balloon);
			 }
			 break;
		 case MotionNotify:
			 if ((*balloon).pwidth <= 0)
				 break;
			 if ((balloon == balloon_current || timer_find_by_data (balloon)) &&
				 (event->xmotion.x < (*balloon).px || event->xmotion.x >= (*balloon).px + (*balloon).pwidth ||
				  event->xmotion.y < (*balloon).py || event->xmotion.y >= (*balloon).py + (*balloon).pheight))
			 {
				 balloon_unmap (balloon);
			 } else if (balloon != balloon_current && !timer_find_by_data (balloon) &&
						event->xmotion.x >= (*balloon).px && event->xmotion.x < (*balloon).px + (*balloon).pwidth &&
						event->xmotion.y >= (*balloon).py && event->xmotion.y < (*balloon).py + (*balloon).pheight)
			 {
				 if (balloon_delay == 0)
					 balloon_map (balloon);
				 else
				 {
					 while (timer_remove_by_data (balloon));
					 balloon->timer_action = BALLOON_TIMER_OPEN;
					 timer_new (balloon_delay, &balloon_timer_handler, (void *)balloon);
				 }
			 }
			 break;
		}
	}

	return True;
}
#endif
