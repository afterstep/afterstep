/*
 * Copyright (C) 1996 Frank Fejes
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

#define LOCAL_DEBUG
#include "../../configure.h"

#include "../../include/asapp.h"
#include "../../include/afterstep.h"
#include "../../include/screen.h"
#include "../../include/decor.h"

#include "asinternals.h"


struct ASWindowGridAuxData{
    ASGrid *grid;
    long    desk;
    int     min_layer;
    Bool    frame_only ;
};

Bool
get_aswindow_grid_iter_func(void *data, void *aux_data)
{
    ASWindow *asw = (ASWindow*)data ;
    struct ASWindowGridAuxData *grid_data = (struct ASWindowGridAuxData*)aux_data;

    if( asw && ASWIN_DESK(asw) == grid_data->desk )
    {
        int outer_gravity = Scr.Feel.EdgeAttractionWindow ;
        int inner_gravity = Scr.Feel.EdgeAttractionWindow ;
        if( ASWIN_HFLAGS(asw, AS_AvoidCover) )
            inner_gravity = -1 ;
        else if( inner_gravity == 0 )
            return True;

        if( ASWIN_GET_FLAGS(asw, AS_Iconic ) )
        {
            add_canvas_grid( grid_data->grid, asw->icon_canvas, outer_gravity, inner_gravity );
            if( asw->icon_canvas != asw->icon_title_canvas )
                add_canvas_grid( grid_data->grid, asw->icon_title_canvas, outer_gravity, inner_gravity );
        }else
        {
            add_canvas_grid( grid_data->grid, asw->frame_canvas, outer_gravity, inner_gravity );
            add_canvas_grid( grid_data->grid, asw->client_canvas, outer_gravity/2, (inner_gravity*2)/3 );
        }
    }
    return True;
}

ASGrid*
make_desktop_grid(int desk, int min_layer, Bool frame_only )
{
    struct ASWindowGridAuxData grid_data ;
    int resist = Scr.Feel.EdgeResistanceMove ;
    int attract = Scr.Feel.EdgeAttractionScreen ;

    grid_data.desk = desk ;
    grid_data.min_layer = min_layer ;
    grid_data.frame_only = frame_only ;
    grid_data.grid = safecalloc( 1, sizeof(ASGrid));
    add_canvas_grid( grid_data.grid, Scr.RootCanvas, resist, attract );
    /* add all the window edges for this desktop : */
    iterate_asbidirlist( Scr.Windows->clients, get_aswindow_grid_iter_func, (void*)&grid_data, NULL, False );

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
    print_asgrid( grid_data.grid );
#endif

    return grid_data.grid;
}

static Bool do_smart_placement( ASWindow *asw, ASWindowBox *aswbox )
{
    return False;
}

static Bool do_random_placement( ASWindow *asw, ASWindowBox *aswbox, Bool free_space_only )
{
    return False;
}

static Bool do_tile_placement( ASWindow *asw, ASWindowBox *aswbox )
{
    return False;
}

static Bool do_cascade_placement( ASWindow *asw, ASWindowBox *aswbox )
{
    return False;
}

static Bool do_manual_placement( ASWindow *asw, ASWindowBox *aswbox )
{
    return False;
}


static Bool
place_aswindow_in_windowbox( ASWindow *asw, ASWindowBox *aswbox, ASUsePlacementStrategy which)
{
    if( which == ASP_UseMainStrategy )
    {
        if( aswbox->main_strategy == ASP_SmartPlacement )
            return do_smart_placement( asw, aswbox );
        else if( aswbox->main_strategy == ASP_RandomPlacement )
            return do_random_placement( asw, aswbox, True );
        else if( aswbox->main_strategy == ASP_Tile )
            return do_tile_placement( asw, aswbox );
    }else
    {
        if( aswbox->backup_strategy == ASP_RandomPlacement )
            return do_random_placement( asw, aswbox, False );
        else if( aswbox->backup_strategy == ASP_Cascade )
            return do_cascade_placement( asw, aswbox );
        else if( aswbox->backup_strategy == ASP_Manual )
            return do_manual_placement( asw, aswbox );
    }
    return False;
}


Bool place_aswindow( ASWindow *asw )
{
    /* if window has predefined named windowbox for it - we use only this windowbox
     * otherwise we use all suitable windowboxes in two passes :
     *   we first try and apply main strategy to place window in the empty space for each box
     *   if all fails we apply backup strategy of the default windowbox
     */
    ASWindowBox *aswbox = NULL ;

    if( AS_ASSERT(asw))
        return False;
    if( AS_ASSERT(asw->hints) || AS_ASSERT(asw->status)  )
        return False;

    if( asw->hints->windowbox_name )
    {
        aswbox = find_window_box( &(Scr.Feel), asw->hints->windowbox_name );
        if( aswbox != NULL )
        {
            if( !place_aswindow_in_windowbox( asw, aswbox, ASP_UseMainStrategy ) )
                return place_aswindow_in_windowbox( asw, aswbox, ASP_UseBackupStrategy );
            return True;
        }
    }

    if( aswbox == NULL )
    {
        int i = Scr.Feel.window_boxes_num;
        aswbox = &(Scr.Feel.window_boxes[0]);
        while( --i >= 0 )
        {
            if( aswbox[i].desk != asw->status->desktop )
                continue;
            if( aswbox[i].min_layer > asw->status->layer || aswbox[i].max_layer < asw->status->layer )
                continue;
            if( aswbox[i].min_width > asw->status->width || (aswbox[i].max_width > 0 && aswbox[i].max_width < asw->status->width) )
                continue;
            if( aswbox[i].min_height > asw->status->height || (aswbox[i].max_height > 0 && aswbox[i].max_height < asw->status->height) )
                continue;

            if( place_aswindow_in_windowbox( asw, &(aswbox[i]), ASP_UseMainStrategy ) )
                return True;
        }
    }
    return place_aswindow_in_windowbox( asw, Scr.Feel.default_window_box, ASP_UseBackupStrategy );
}

#if 0



/*
 * pass 0: do not ignore windows behind the target window's layer
 * pass 1: ignore windows behind the target window's layer
 */
int
SmartPlacement (ASWindow * t, int *x, int *y, int width, int height, int rx, int ry, int rw, int rh,
				int pass)
{
	int           test_x = 0, test_y;
	int           loc_ok = 0;
	ASWindow     *twin;
	int           xb = rx, xmax = rx + rw - width, xs = 1;
	int           yb = ry, ymax = ry + rh - height, ys = 1;

	if (rw < width || rh < height)
		return loc_ok;

	/* if closer to the right edge than the left, scan from right to left */
	if (Scr.MyDisplayWidth - (rx + rw) < rx)
	{
		xb = rx + rw - width;
		xs = -1;
	}

	/* if closer to the bottom edge than the top, scan from bottom to top */
	if (Scr.MyDisplayHeight - (ry + rh) < ry)
	{
		yb = ry + rh - height;
		ys = -1;
	}

	for (test_y = yb; ry <= test_y && test_y <= ymax && !loc_ok; test_y += ys)
		for (test_x = xb; rx <= test_x && test_x <= xmax && !loc_ok; test_x += xs)
		{
			int           tx, ty, tw, th;

			loc_ok = 1;

			for (twin = Scr.ASRoot.next; twin != NULL && loc_ok; twin = twin->next)
			{
				/* ignore windows on other desks, and our own window */
				if (ASWIN_DESK(twin) != ASWIN_DESK(t) || twin == t)
					continue;

				/* ignore non-iconified windows, if we're iconified and not using
				 * StubbornIconPlacement */
				if (!(twin->flags & ICONIFIED) && (t->flags & ICONIFIED) &&
					!(Scr.flags & StubbornIconPlacement))
					continue;

				/* ignore iconified windows, if we're not iconified and not using
				 * StubbornPlacement */
				if ((twin->flags & ICONIFIED) && !(t->flags & ICONIFIED) &&
					!(Scr.flags & StubbornPlacement))
					continue;

				/* ignore a window on a lower layer, unless it's an AvoidCover
				 * window or instructed to pay attention to it (ie, pass == 0) */
				if (!(twin->flags & ICONIFIED) && ASWIN_LAYER(twin) < ASWIN_LAYER(t) &&
					!ASWIN_HFLAGS(twin, AS_AvoidCover) && pass)
					continue;

				get_window_geometry (twin, twin->flags, &tx, &ty, &tw, &th);
				tw += 2 * twin->bw;
				th += 2 * twin->bw;
				if (tx <= test_x + width && tx + tw >= test_x &&
					ty <= test_y + height && ty + th >= test_y)
				{
					loc_ok = 0;
					if (xs > 0)
						test_x = tx + tw;
					else
						test_x = tx - width;
				}
			}
		}
	if (loc_ok)
	{
		*x = test_x - xs;
		*y = test_y - ys;
	}
	return loc_ok;
}

/**************************************************************************
 *
 * Handles initial placement and sizing of a new window
 * Returns False in the event of a lost window.
 *
 **************************************************************************/
Bool
PlaceWindow (ASWindow * tmp_win, unsigned long tflag, int Desk)
{
	int           xl = -1, yt = -1, DragWidth, DragHeight;
	extern Bool   PPosOverride;
	XRectangle    srect = { 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight };
	int           x, y;
	unsigned int  width, height;

	y = tmp_win->attr.y;
	x = tmp_win->attr.x;
	width = tmp_win->frame_width;
	height = tmp_win->frame_height;

#ifdef HAVE_XINERAMA
	if (Scr.xinerama_screens_num > 1)
	{
		register int  i;
		XRectangle   *s = Scr.xinerama_screens;

		for (i = 0; i < Scr.xinerama_screens_num; ++i)
		{
			if (s[i].x < x + width && s[i].x + s[i].width > x &&
				s[i].y < y + height && s[i].y + s[i].height > y)
			{
				srect = s[i];
				break;
			}
		}
	}
#endif /* XINERAMA */


	tmp_win->Desk = InvestigateWindowDesk (tmp_win);

	/* I think it would be good to switch to the selected desk
	 * whenever a new window pops up, except during initialization */
	if (!PPosOverride && Scr.CurrentDesk != tmp_win->Desk)
		changeDesks (0, tmp_win->Desk);

	/* Desk has been selected, now pick a location for the window */
	/*
	 *  If
	 *     o  the window is a transient, or
	 *
	 *     o  a USPosition was requested, or
	 *
	 *     o  Prepos flag was given
	 *
	 *   then put the window where requested.
	 *
	 *   If RandomPlacement was specified,
	 *       then place the window in a psuedo-random location
	 */
	if (!ASWIN_HFLAGS(tmp_win, AS_Transient) &&
		!(tmp_win->normal_hints.flags & USPosition) &&
		((Scr.flags & NoPPosition) || !(tmp_win->normal_hints.flags & PPosition)) &&
		!(PPosOverride) &&
		!(tflag & PREPOS_FLAG) &&
		!((tmp_win->wmhints) &&
		  (tmp_win->wmhints->flags & StateHint) &&
		  (tmp_win->wmhints->initial_state == IconicState)))
	{
		/* Get user's window placement, unless RandomPlacement is specified */
		if (Scr.flags & SMART_PLACEMENT)
		{
			if (!SmartPlacement (tmp_win, &xl, &yt,
								 tmp_win->frame_width + 2 * tmp_win->bw,
								 tmp_win->frame_height + 2 * tmp_win->bw,
								 srect.x, srect.y, srect.width, srect.height, 0))
				SmartPlacement (tmp_win, &xl, &yt,
								tmp_win->frame_width + 2 * tmp_win->bw,
								tmp_win->frame_height + 2 * tmp_win->bw,
								srect.x, srect.y, srect.width, srect.height, 1);
		}
		if (Scr.flags & RandomPlacement)
		{
			if (xl < 0)
			{
				/* place window in a random location */
				if (tmp_win->flags & VERTICAL_TITLE)
				{
					Scr.randomx += 2 * tmp_win->title_width;
					Scr.randomy += tmp_win->title_width;
				} else
				{
					Scr.randomx += tmp_win->title_height;
					Scr.randomy += 2 * tmp_win->title_height;
				}
				if (Scr.randomx > srect.x + (srect.width / 2))
					Scr.randomx = srect.x;
				if (Scr.randomy > srect.y + (srect.height / 2))
					Scr.randomy = srect.y;
				xl = Scr.randomx - tmp_win->old_bw;
				yt = Scr.randomy - tmp_win->old_bw;
			}

			if (xl + tmp_win->frame_width + 2 * tmp_win->bw > srect.width)
			{
				xl = srect.width - tmp_win->frame_width - 2 * tmp_win->bw;
				Scr.randomx = srect.x;
			}
			if (yt + tmp_win->frame_height + 2 * tmp_win->bw > srect.height)
			{
				yt = srect.height - tmp_win->frame_height - 2 * tmp_win->bw;
				Scr.randomy = srect.y;
			}
		}
		if (xl < 0)
		{
			if (GrabEm (POSITION))
			{
				/* Grabbed the pointer - continue */
				XGrabServer (dpy);
				DragWidth = tmp_win->frame_width + 2 * tmp_win->bw;
				DragHeight = tmp_win->frame_height + 2 * tmp_win->bw;
				XMapRaised (dpy, Scr.SizeWindow);
				moveLoop (tmp_win, 0, 0, DragWidth, DragHeight, &xl, &yt, False, True);
				XUnmapWindow (dpy, Scr.SizeWindow);
				XUngrabServer (dpy);
				UngrabEm ();
			} else
			{
				/* couldn't grab the pointer - better do something */
				XBell (dpy, Scr.screen);
				xl = 0;
				yt = 0;
			}
		}
		tmp_win->attr.y = yt;
		tmp_win->attr.x = xl;
	} else
	{
		/* the USPosition was specified, or the window is a transient,
		 * or it starts iconic so let it place itself */
		if (!(tmp_win->normal_hints.flags & USPosition))
		{
			if (width <= srect.width)
			{
				if (x < srect.x)
					x = srect.x;
				else if (x + width > srect.x + srect.width)
					x = srect.x + srect.width - width;
			}
			if (height <= srect.height)
			{
				if (y < srect.y)
					y = srect.y;
				else if (y + height > srect.y + srect.height)
					y = srect.y + srect.height - height;
			}
			tmp_win->attr.y = y;
			tmp_win->attr.x = x;
		}
	}
	aswindow_set_desk_property (tmp_win, tmp_win->Desk);
	return True;
}


#endif

