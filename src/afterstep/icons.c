
/*
 * Copyright (C) 1998 Sasha Vasko <sasha at aftercode.net>
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

#include "asinternals.h"

#include <unistd.h>
#include <signal.h>
#include <X11/Intrinsic.h>


/***********************************************************************/
/* new IconBox handling :                                              */
/***********************************************************************/

void
destroy_asiconbox( ASIconBox **pib )
{
	if( pib && *pib)
	{
		register ASIconBox *ib = *pib ;
		if( ib->areas )
			free( ib->areas );
		if( ib->icons )
            destroy_asbidirlist( &(ib->icons) );
		free( ib );
	  	*pib = NULL ;
	}
}

ASIconBox *
get_iconbox( int desktop )
{
	ASIconBox *ib = NULL ;
	if( IsValidDesk(desktop) )
	{
        if( get_flags(Scr.Feel.flags, StickyIcons) )
			ib = Scr.default_icon_box ;
		else
			if( Scr.icon_boxes )
                if( get_hash_item( Scr.icon_boxes, AS_HASHABLE(desktop), (void**)&ib ) != ASH_Success )
					ib = NULL ;
		if( ib == NULL )
		{
			ib = safecalloc( 1, sizeof( ASIconBox ));
            if( Scr.Look.configured_icon_areas_num == 0 || Scr.Look.configured_icon_areas == NULL )
			{
				ib->areas_num = 1 ;
                ib->areas = safecalloc( 1, sizeof(ASGeometry) );
				ib->areas->width = Scr.MyDisplayWidth ;
				ib->areas->height = Scr.MyDisplayHeight ;
                ib->areas->flags = 0;
			}else
			{
                register int i = Scr.Look.configured_icon_areas_num ;
				ib->areas_num = i ;
                ib->areas = safecalloc( ib->areas_num, sizeof(ASGeometry) );
				while( --i >= 0 )
                    ib->areas[i] = Scr.Look.configured_icon_areas[i];
			}
			ib->icons = create_asbidirlist( NULL );
            if( get_flags(Scr.Feel.flags, StickyIcons) )
				Scr.default_icon_box = ib ;
			else
			{
				if( Scr.icon_boxes == NULL )
					Scr.icon_boxes = create_ashash( 3, NULL, NULL, NULL );

				if( add_hash_item( Scr.icon_boxes, AS_HASHABLE(desktop), &ib ) != ASH_Success )
					destroy_asiconbox( &ib );
			}
		}
	}
	return ib;
}

typedef struct ASIconRearrangeAux
{
    int curr_area ;
    int last_x, last_y, last_width, last_height ;
    ASIconBox *ib;
}ASIconRearrangeAux;

Bool rearrange_icon_iter_func(void *data, void *aux_data)
{
    ASWindow *asw = (ASWindow*) data ;
    ASIconRearrangeAux *rd = (ASIconRearrangeAux *)aux_data ;
    int width = 0, height = 0, title_width = 0, title_height = 0;
    int whole_width = 0, whole_height = 0 ;
    int x, y, box_x = 0, box_y = 0;

	if( AS_ASSERT(asw) || AS_ASSERT(rd) )
		return False;

	LOCAL_DEBUG_OUT( "asw(%p)->magic(%lX)->icon_canvas(%p)->icon_title_canvas(%p)->icon_button(%p)->icon_title(%p)",
					 asw, asw->magic, asw->icon_canvas, asw->icon_title_canvas, asw->icon_button, asw->icon_title );
    if( asw->icon_canvas == NULL || rd->ib == NULL )
        return True;

    if( asw->icon_button )
    {
        width = calculate_astbar_width( asw->icon_button );
        height = calculate_astbar_height( asw->icon_button );
    }
    if( asw->icon_title )
    {
        title_width = calculate_astbar_width( asw->icon_title );
        title_height = calculate_astbar_height( asw->icon_title );
    }

    whole_width = (width == 0)?title_width:width ;
    whole_height = height + title_height ;
    /* now we could determine where exactly to place icon to : */
    x = -whole_width ;
    y = -whole_height ;
	LOCAL_DEBUG_OUT( "entering loop : areas_num = %d", rd->ib->areas_num );
    while( rd->curr_area < rd->ib->areas_num )
    {
        ASGeometry *geom = &(rd->ib->areas[rd->curr_area]) ;
        int new_x = -1, new_y = -1 ;

LOCAL_DEBUG_OUT( "trying area #%d : %s%s, %dx%d%+d%+d", rd->curr_area, get_flags(geom->flags, XNegative)?"XNeg":"XPos", get_flags(geom->flags, YNegative)?"YNeg":"YPos", geom->width, geom->height, geom->x, geom->y );
LOCAL_DEBUG_OUT( "last: %dx%d%+d%+d", rd->last_width, rd->last_height, rd->last_x, rd->last_y );

        if( get_flags(geom->flags, XNegative) )
            new_x = rd->last_x - rd->last_width - whole_width ;
        else
            new_x = rd->last_x + rd->last_width ;

        if( new_x < 0 || new_x+whole_width > geom->width )
        {  /* lets try and go to the next row and see if that makes a difference */
            if( get_flags(geom->flags, YNegative) )
                rd->last_y = rd->last_y - rd->last_height ;
            else
                rd->last_y = rd->last_y + rd->last_height ;
            rd->last_x = get_flags( geom->flags, XNegative )?geom->width-1: 0 ;

            if( get_flags(geom->flags, XNegative) )
                new_x = rd->last_x - whole_width ;
        }else
            rd->last_x = new_x ;

        new_y = rd->last_y ;
        if( get_flags(geom->flags, YNegative) )
            new_y = rd->last_y - whole_height ;

LOCAL_DEBUG_OUT( "new : %+d%+d", new_x, new_y );
        if( new_x >= 0 && new_x+whole_width < geom->width &&
            new_y >= 0 && new_y+whole_height < geom->height )
        {
            x = new_x ;
            y = new_y ;
            box_x = geom->x ;
            box_y = geom->y ;
            break;
        }

        ++(rd->curr_area);
        if( rd->curr_area < rd->ib->areas_num )
        {
            geom = &(rd->ib->areas[rd->curr_area]) ;
            rd->last_x = get_flags( geom->flags, XNegative )?geom->width-1: 0 ;
            rd->last_y = get_flags( geom->flags, YNegative )?geom->height-1: 0 ;
        }else
        {
            rd->last_x = 0 ;
            rd->last_y = 0 ;
        }

        rd->last_width = 0 ;
        rd->last_height = 0 ;
    }
    /* placing the icon : */
LOCAL_DEBUG_OUT( "placing an icon at %+d%+d, base %+d%+d whole %dx%d button %dx%d", x, y, box_x, box_y, whole_width, whole_height, width, height );
    if( asw->icon_title_canvas && asw->icon_title_canvas != asw->icon_canvas )
    {
        moveresize_canvas( asw->icon_canvas, box_x+x, box_y+y, width, height );
        moveresize_canvas( asw->icon_title_canvas, box_x+x, box_y+y+height, width, height );
    }else
        moveresize_canvas( asw->icon_canvas, box_x+x, box_y+y, whole_width, whole_height );

    rd->last_width  = whole_width  ;
    if( whole_height > rd->last_height )
        rd->last_height = whole_height ;
    return True;
}

void
rearrange_iconbox( ASIconBox *ib )
{
    ASIconRearrangeAux aux_data ;

    aux_data.curr_area = 0 ;
    aux_data.last_x = get_flags( ib->areas[0].flags, XNegative )?ib->areas[0].width-1: 0 ;
    aux_data.last_y = get_flags( ib->areas[0].flags, YNegative )?ib->areas[0].height-1: 0 ;
    aux_data.last_width = 0 ;
    aux_data.last_height = 0 ;
    aux_data.ib = ib ;

    iterate_asbidirlist( ib->icons, rearrange_icon_iter_func, &aux_data, NULL, False);
}

Bool
add_iconbox_icon( ASWindow *asw )
{
    ASIconBox *ib = NULL ;
    if( AS_ASSERT(asw) )
        return False;

    /* TODO: we need to add this window to the list of icons,
     * and then place it in appropriate position : */
    ib = get_iconbox( ASWIN_DESK(asw) );
    append_bidirelem( ib->icons, asw );
    rearrange_iconbox( ib );
    return True;
}

Bool
remove_iconbox_icon( ASWindow *asw )
{
    ASIconBox *ib = NULL ;
    if( AS_ASSERT(asw) )
        return False;
    /* TODO: we need to remove this window from the list of icons */
    ib = get_iconbox( ASWIN_DESK(asw) );
    discard_bidirelem( ib->icons, asw );
    rearrange_iconbox( ib );
    return True;
}

Bool
change_iconbox_icon_desk( ASWindow *asw, int from_desk, int to_desk )
{
    return False;
}

void
on_icon_changed( ASWindow *asw )
{
//    if( AS_ASSERT(asw) )
        return;
    /* we probably need to reshuffle entire iconbox when that happen : */
    if( asw->icon_title )
    {
        int tbar_size = calculate_astbar_width( asw->icon_title );
        set_astbar_size( asw->icon_title, tbar_size, asw->icon_title?asw->icon_title->height:0 );
        render_astbar( asw->icon_title, asw->icon_title_canvas );
    }
    if( asw->icon_button )
    {


    }
    update_canvas_display( asw->icon_canvas );
    update_canvas_display( asw->icon_title_canvas );
}

void
rearrange_iconbox_icons( int desktop )
{
    rearrange_iconbox( get_iconbox( desktop ) );
}

#if 0
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/* Old stuff :                                                         */
/***********************************************************************/
void
UpdateIconShape (ASWindow * tmp_win)
{
#ifdef SHAPE
	XRectangle    rect;

	/* assume we will be shaping the icon */
	tmp_win->flags |= SHAPED_ICON;

	/* don't do anything if there's no window or the window isn't ours */
	if (tmp_win->icon_pixmap_w == None || !(tmp_win->flags & ICON_OURS))
		tmp_win->flags &= ~SHAPED_ICON;

	/* don't do anything if the icon pixmap is solid and covers the whole window */
	if (tmp_win->icon_p_width <= tmp_win->icon_pm_width &&
		tmp_win->icon_p_height <= tmp_win->icon_pm_height && tmp_win->icon_pm_mask == None)
		tmp_win->flags &= ~SHAPED_ICON;

	/* initialize icon to completely transparent if we need to shape, else
	 * set the icon to completely opaque and return */
	if (tmp_win->flags & SHAPED_ICON)
	{
		rect.x = 0;
		rect.y = 0;
		rect.width = tmp_win->icon_p_width;
		rect.height = tmp_win->icon_p_height;
		XShapeCombineRectangles (dpy, tmp_win->icon_pixmap_w, ShapeBounding,
								 0, 0, &rect, 1, ShapeSubtract, Unsorted);
	} else
	{
		XShapeCombineMask (dpy, tmp_win->icon_pixmap_w, ShapeBounding, 0, 0, None, ShapeSet);
		return;
	}

#ifndef NO_TEXTURE
	{										   /* add background mask */
		MyStyle      *style = mystyle_find_or_default ("ButtonPixmap");
		int           y, x;

		if ((style->set_flags & F_BACKPIXMAP) && style->back_icon.mask != None)
			for (y = 0; y < tmp_win->icon_p_height; y += style->back_icon.height)
				for (x = 0; x < tmp_win->icon_p_width; x += style->back_icon.width)
					XShapeCombineMask (dpy, tmp_win->icon_pixmap_w, ShapeBounding, x, y,
									   style->back_icon.mask, ShapeUnion);
	}
#endif /* !NO_TEXTURE */

	/* add titlebar, if necessary */
    if (!(Scr.look_flags & SeparateButtonTitle) && (Scr.flags & IconTitle) &&
		 ASWIN_HFLAGS(tmp_win, AS_IconTitle))
	{
		rect.x = tmp_win->icon_t_x;
		rect.y = tmp_win->icon_t_y;
		rect.width = tmp_win->icon_t_width;
		rect.height = tmp_win->icon_t_height;
		XShapeCombineRectangles (dpy, tmp_win->icon_pixmap_w, ShapeBounding,
								 0, 0, &rect, 1, ShapeUnion, Unsorted);
	}

	/* add icon pixmap background, if necessary */
	if (tmp_win->icon_pm_mask != None)
		XShapeCombineMask (dpy, tmp_win->icon_pixmap_w, ShapeBounding, tmp_win->icon_pm_x,
						   tmp_win->icon_pm_y, tmp_win->icon_pm_mask, ShapeUnion);
	else
	{
		rect.x = tmp_win->icon_pm_x;
		rect.y = tmp_win->icon_pm_y;
		rect.width = tmp_win->icon_pm_width;
		rect.height = tmp_win->icon_pm_height;
		XShapeCombineRectangles (dpy, tmp_win->icon_pixmap_w, ShapeBounding,
								 0, 0, &rect, 1, ShapeUnion, Unsorted);
	}
#endif
}

void
ResizeIconWindow (ASWindow * tmp_win)
{
	MyStyle      *button_title_focus = mystyle_find_or_default ("ButtonTitleFocus");
	MyStyle      *button_title_sticky = mystyle_find_or_default ("ButtonTitleSticky");
	MyStyle      *button_title_unfocus = mystyle_find_or_default ("ButtonTitleUnfocus");
	const char   *name =
		ASWIN_ICON_NAME(tmp_win)!= NULL ? ASWIN_ICON_NAME(tmp_win) :
		(ASWIN_NAME(tmp_win) !=	NULL )? ASWIN_NAME(tmp_win) : "";
	int           len, width, height, t_width, t_height;

	/* set the base icon size */
	tmp_win->icon_p_width = tmp_win->icon_pm_width;
	tmp_win->icon_p_height = tmp_win->icon_pm_height;
	tmp_win->icon_t_x = tmp_win->icon_t_y = 0;
	tmp_win->icon_t_width = tmp_win->icon_t_height = 0;
	tmp_win->icon_pm_x = tmp_win->icon_pm_y = 0;

	/* calculate the icon title width and height */
	len = strlen (name);
	mystyle_get_text_geometry (button_title_focus, name, len, &t_width, &t_height);
	mystyle_get_text_geometry (button_title_sticky, name, len, &width, &height);
	t_width = MAX (t_width, width);
	t_height = MAX (t_height, height);
	mystyle_get_text_geometry (button_title_unfocus, name, len, &width, &height);
	t_width = MAX (t_width, width);
	t_height = MAX (t_height, height);

	/* take care of icon title */
	if ((Scr.flags & IconTitle) && ASWIN_HFLAGS(tmp_win, AS_IconTitle))
	{
        if (Scr.look_flags & SeparateButtonTitle)
		{
			tmp_win->icon_t_x = 0;
			tmp_win->icon_t_y = 0;
			tmp_win->icon_t_width = t_width + 6;
			tmp_win->icon_t_height = t_height + 6;
		} else
		{
			tmp_win->icon_t_x = 0;
			tmp_win->icon_t_y = 0;
			tmp_win->icon_t_width = tmp_win->icon_p_width;
			tmp_win->icon_t_height = t_height + 2;
			tmp_win->icon_p_height += tmp_win->icon_t_height;
			tmp_win->icon_pm_y += t_height + 2;
		}
	}

	/* if the icon is ours, add a border around it */
	if ((tmp_win->flags & ICON_OURS) && (tmp_win->icon_p_height > 0) &&
        !(Scr.look_flags & IconNoBorder))
	{
		tmp_win->icon_p_width += 4;
		tmp_win->icon_p_height += 4;
		tmp_win->icon_pm_x += 2;
		tmp_win->icon_pm_y += 2;

		/* title goes inside the border */
		if ((Scr.flags & IconTitle) && ASWIN_HFLAGS(tmp_win, AS_IconTitle) &&
            !(Scr.look_flags & SeparateButtonTitle))
		{
			tmp_win->icon_t_x = 2;
			tmp_win->icon_t_y = 2;
		}
	}

	/* honor ButtonSize */
	if (Scr.ButtonWidth > 0)
	{
		/* title goes inside the border */
		if ((Scr.flags & IconTitle) && ASWIN_HFLAGS(tmp_win, AS_IconTitle) &&
            !(Scr.look_flags & SeparateButtonTitle))
			tmp_win->icon_t_width -= tmp_win->icon_p_width - Scr.ButtonWidth;
		tmp_win->icon_pm_x -= (tmp_win->icon_p_width - Scr.ButtonWidth) / 2;
		tmp_win->icon_p_width = Scr.ButtonWidth;
	}
	if (Scr.ButtonHeight > 0)
	{
		tmp_win->icon_pm_y -= (tmp_win->icon_p_height - Scr.ButtonHeight) / 2;
		tmp_win->icon_p_height = Scr.ButtonHeight;
	}
}

/****************************************************************************
 *
 * Creates an icon window as needed
 *
 ****************************************************************************/
void
CreateIconWindow (ASWindow * tmp_win)
{
	unsigned long valuemask;				   /* mask for create windows */
	XSetWindowAttributes attributes;		   /* attributes for create windows */

	tmp_win->flags &= ~(ICON_OURS | XPM_FLAG | PIXMAP_OURS | SHAPED_ICON);
	tmp_win->icon_pixmap_w = None;
	tmp_win->icon_pm_pixmap = None;
	tmp_win->icon_pm_mask = None;			   /* messo io */
	tmp_win->icon_pm_depth = 0;

	if (tmp_win->flags & SUPPRESSICON)
		return;

	/* First, see if it was specified in config. files  */
	tmp_win->icon_pm_file = SearchIcon (tmp_win);
	GetIcon (tmp_win);
	ResizeIconWindow (tmp_win);

	valuemask = CWBorderPixel | CWCursor | CWEventMask | CWBackPixmap;
	attributes.background_pixmap = ParentRelative;
	attributes.border_pixel = Scr.asv->black_pixel;
	attributes.cursor = Scr.ASCursors[DEFAULT];
	attributes.event_mask = AS_ICON_TITLE_EVENT_MASK;

	destroy_icon_windows( tmp_win );
    if ((Scr.look_flags & SeparateButtonTitle) && (Scr.flags & IconTitle) &&
		ASWIN_HFLAGS(tmp_win, AS_IconTitle))
	{
		tmp_win->icon_title_w =
			create_visual_window (Scr.asv, Scr.Root, -999, -999, 16, 16, 0,
								  InputOutput, valuemask, &attributes);
	}

	if ( ASWIN_HFLAGS(asw, AS_ClientIcon|AS_ClientIconPixmap) != AS_ClientIcon )
	{
		tmp_win->icon_pixmap_w =
			create_visual_window (Scr.asv, Scr.Root, -999, -999, 16, 16, 0,
								  InputOutput, valuemask, &attributes);
	} else
	{
		attributes.event_mask = AS_ICON_EVENT_MASK;

		valuemask = CWEventMask;
		XChangeWindowAttributes (dpy, tmp_win->icon_pixmap_w, valuemask, &attributes);
	}

	if (tmp_win->icon_title_w != None)
	{
        register_aswindow( tmp_win->icon_title_w, tmp_win );
		XDefineCursor (dpy, tmp_win->icon_title_w, Scr.ASCursors[DEFAULT]);
		GrabIconButtons (tmp_win, tmp_win->icon_title_w);
		GrabIconKeys (tmp_win, tmp_win->icon_title_w);
	}

	if (tmp_win->icon_pixmap_w != None)
	{
        register_aswindow( tmp_win->icon_pixmap_w, tmp_win );
		XDefineCursor (dpy, tmp_win->icon_pixmap_w, Scr.ASCursors[DEFAULT]);
		GrabIconButtons (tmp_win, tmp_win->icon_pixmap_w);
		GrabIconKeys (tmp_win, tmp_win->icon_pixmap_w);
	}

#ifdef SHAPE
	UpdateIconShape (tmp_win);
#endif /* SHAPE */

}


/****************************************************************************
 *
 * Draws the icon window
 *
 ****************************************************************************/
void
DrawIconWindow (ASWindow * win)
{
	GC            ForeGC, BackGC, Shadow, Relief;
	const char   *name =
		ASWIN_ICON_NAME(win) != NULL ? ASWIN_ICON_NAME(win) :
		(ASWIN_NAME(win) != NULL) ? ASWIN_NAME(win) : "";
	MyStyle      *button_pixmap, *button_title;

	if (win->flags & SUPPRESSICON)
		return;

	/* flush any waiting expose events */
	if (win->icon_title_w != None)
		flush_expose (win->icon_title_w);

	if (win->icon_pixmap_w != None)
		flush_expose (win->icon_pixmap_w);

	/* decide which styles we'll use */
	button_pixmap = mystyle_find_or_default ("ButtonPixmap");
	if (Scr.Hilite == win)
		button_title = mystyle_find_or_default ("ButtonTitleFocus");
	else if (win->flags & STICKY)
		button_title = mystyle_find_or_default ("ButtonTitleSticky");
	else
		button_title = mystyle_find_or_default ("ButtonTitleUnfocus");

	/* draw the icon pixmap window */
	if ((win->flags & ICON_OURS) && win->icon_pixmap_w != None)
	{
		/* reset the icon title window's width */
		XMoveResizeWindow (dpy, win->icon_pixmap_w, win->icon_p_x,
						   win->icon_p_y, win->icon_p_width, win->icon_p_height);

		/* draw the background */
		SetBackgroundTexture (win, win->icon_pixmap_w, button_pixmap, None);

		/* set the GCs for the button pixmap first */
		mystyle_get_global_gcs (button_pixmap, &ForeGC, &BackGC, &Relief, &Shadow);

		/* draw the icon pixmap */
		if (win->icon_pm_pixmap != None)
		{
			if (win->icon_pm_mask)
			{
				XSetClipOrigin (dpy, ForeGC, win->icon_pm_x, win->icon_pm_y);
				XSetClipMask (dpy, ForeGC, win->icon_pm_mask);
			}

			if (win->icon_pm_depth == Scr.d_depth)
				XCopyArea (dpy, win->icon_pm_pixmap, win->icon_pixmap_w,
						   ForeGC, 0, 0, win->icon_pm_width,
						   win->icon_pm_height, win->icon_pm_x, win->icon_pm_y);
			else
				XCopyPlane (dpy, win->icon_pm_pixmap, win->icon_pixmap_w,
							ForeGC, 0, 0, win->icon_pm_width,
							win->icon_pm_height, win->icon_pm_x, win->icon_pm_y, 1);

			if (win->icon_pm_mask)
				XSetClipMask (dpy, ForeGC, None);
		}

		/* draw the relief */
        if (!(win->flags & SHAPED_ICON) && !(Scr.look_flags & IconNoBorder))
			RelieveWindow (win, win->icon_pixmap_w, 0, 0,
						   win->icon_p_width, win->icon_p_height, Relief, Shadow, FULL_HILITE);

		/* draw the title text, if necessary */
        if (!(Scr.look_flags & SeparateButtonTitle) && (Scr.flags & IconTitle) &&
			  ASWIN_HFLAGS(win, AS_IconTitle))
		{
			int           x, y, w, h;

			mystyle_get_text_geometry (button_title, name, strlen (name), &w, &h);
			/* if icon title is too large for the available space, show the first part */
			x = win->icon_t_x + MAX (0, (win->icon_t_width - w) / 2);
			y = win->icon_t_y + MAX (0, (win->icon_t_height - h) / 2);
			mystyle_get_global_gcs (button_title, &ForeGC, &BackGC, &Relief, &Shadow);
			XFillRectangle (dpy, win->icon_pixmap_w, BackGC, win->icon_t_x, win->icon_t_y,
							win->icon_t_width, win->icon_t_height);
			mystyle_draw_text (win->icon_pixmap_w, button_title, name, x, y + button_title->font.y);
		}
	}

	/* draw the icon title window */
	if (win->icon_title_w != None)
	{
		int           x, y, w, h;

		/* if there is no icon pixmap window, the effective icon width is
		 * the same as the title width */
		int           p_width =
			(win->icon_pixmap_w == None) ? win->icon_t_width : win->icon_p_width;
		/* if this is the focus window, show the whole title */
		int           t_width = (Scr.Hilite == win) ? win->icon_t_width : p_width;

		/* reset the window width */
		XMoveResizeWindow (dpy, win->icon_title_w,
						   win->icon_p_x - (t_width - p_width) / 2,
						   win->icon_p_y + win->icon_p_height, t_width, win->icon_t_height);

		/* draw the background */
		SetBackgroundTexture (win, win->icon_title_w, button_title, None);

		/* setup our GCs */
		mystyle_get_global_gcs (button_title, &ForeGC, &BackGC, &Relief, &Shadow);

		/* draw the relief */
		RelieveWindow (win, win->icon_title_w, win->icon_t_x, win->icon_t_y, t_width,
					   win->icon_t_height, Relief, Shadow, FULL_HILITE);

		/* place the text */
		mystyle_get_text_geometry (button_title, name, strlen (name), &w, &h);
		x = win->icon_t_x + 3 + MAX (0, ((t_width - 6) - w) / 2);
		y = (win->icon_t_height - h) / 2;
		y = (win->icon_t_height - h) / 2;

		/* draw the icon title */
		mystyle_draw_text (win->icon_title_w, button_title, name, x, y + button_title->font.y);
	}
}


void
AutoPlaceStickyIcons (void)
{
	ASWindow     *t;

	/* first pass: move the icons so they don't interfere with each other */
	for (t = Scr.ASRoot.next; t != NULL; t = t->next)
	{
		if (((t->flags & STICKY) || (Scr.flags & StickyIcons)) &&
			(t->flags & ICONIFIED) && (!(t->flags & ICON_MOVED)) && (!(t->flags & ICON_UNMAPPED)))
			t->icon_p_x = t->icon_p_y = -999;
	}
	/* second pass: autoplace the icons */
	for (t = Scr.ASRoot.next; t != NULL; t = t->next)
	{
		if (((t->flags & STICKY) || (Scr.flags & StickyIcons)) &&
			(t->flags & ICONIFIED) && (!(t->flags & ICON_MOVED)) && (!(t->flags & ICON_UNMAPPED)))
			AutoPlace (t);
	}
}

/***********************************************************************
 *
 *  Procedure:
 *	AutoPlace - Find a home for an icon
 *
 ************************************************************************/
void
AutoPlace (ASWindow * t)
{
	int           base_x, base_y;

	/* New! Put icon in same page as the center of the window */
	/* Not a good idea for StickyIcons */
	if ((Scr.flags & StickyIcons) || (t->flags & STICKY))
	{
		base_x = 0;
		base_y = 0;
	} else
	{
		base_x = ((t->frame_x + Scr.Vx + (t->frame_width >> 1)) /
				  Scr.MyDisplayWidth) * Scr.MyDisplayWidth - Scr.Vx;
		base_y = ((t->frame_y + Scr.Vy + (t->frame_height >> 1)) /
				  Scr.MyDisplayHeight) * Scr.MyDisplayHeight - Scr.Vy;
	}
	if (t->flags & ICON_MOVED)
	{
		/* just make sure the icon is on this screen */
		t->icon_p_x = t->icon_p_x % Scr.MyDisplayWidth + base_x;
		t->icon_p_y = t->icon_p_y % Scr.MyDisplayHeight + base_y;
		if (t->icon_p_x < 0)
			t->icon_p_x += Scr.MyDisplayWidth;
		if (t->icon_p_y < 0)
			t->icon_p_y += Scr.MyDisplayHeight;
	} else if (get_flags(t->hints->flags, AS_ClientIconPosition))
	{
		t->icon_p_x = t->hints->icon_x;
		t->icon_p_y = t->hints->icon_y;
	} else
	{
		int           i, real_x = 0, real_y = 0, width, height;
		int           loc_ok = 0;
		unsigned long flags = t->flags;

		/* hack -- treat window as iconified for placement */
		t->flags |= ICONIFIED;

		get_window_geometry (t, t->flags, NULL, NULL, &width, &height);

		if (width)
			width += 2 * t->bw;
		if (height)
			height += 2 * t->bw;
		/* check all boxes in order */
		for (i = 0; i < Scr.NumBoxes && !loc_ok; i++)
		{
			int           twidth = width, theight = height;

			/* If the window is taller than the icon box, ignore the icon height
			 * when figuring where to put it. Same goes for the width. This
			 * should permit reasonably graceful handling of big icons. */
			if (twidth >= Scr.IconBoxes[i][2] - Scr.IconBoxes[i][0])
				twidth = Scr.IconBoxes[i][2] - Scr.IconBoxes[i][0];
			if (theight >= Scr.IconBoxes[i][3] - Scr.IconBoxes[i][1])
				theight = Scr.IconBoxes[i][3] - Scr.IconBoxes[i][1];

			loc_ok = SmartPlacement (t, &real_x, &real_y, twidth, theight,
									 Scr.IconBoxes[i][0] + base_x,
									 Scr.IconBoxes[i][1] + base_y,
									 Scr.IconBoxes[i][2] - Scr.IconBoxes[i][0],
									 Scr.IconBoxes[i][3] - Scr.IconBoxes[i][1], 0);
			if (!loc_ok)
				loc_ok = SmartPlacement (t, &real_x, &real_y, twidth, theight,
										 Scr.IconBoxes[i][0] + base_x,
										 Scr.IconBoxes[i][1] + base_y,
										 Scr.IconBoxes[i][2] - Scr.IconBoxes[i][0],
										 Scr.IconBoxes[i][3] - Scr.IconBoxes[i][1], 1);

			/* right-align or bottom-align the icon as necessary */
			if (twidth < width && Scr.MyDisplayWidth - Scr.IconBoxes[i][2] < Scr.IconBoxes[i][0])
				real_x = Scr.IconBoxes[i][2] - width;
			if (theight < height && Scr.MyDisplayHeight - Scr.IconBoxes[i][3] < Scr.IconBoxes[i][1])
				real_y = Scr.IconBoxes[i][3] - height;
		}
		/* provide a default location, in case we have no icon boxes,
		 * or none of the icon boxes are suitable */
		if (!loc_ok)
		{
			real_x = 0;
			real_y = 0;
		}
		t->icon_p_x = real_x;
		t->icon_p_y = real_y;

		/* restore original flags value */
		t->flags = flags;
	}

	if (t->icon_pixmap_w != None)
		XMoveWindow (dpy, t->icon_pixmap_w, t->icon_p_x, t->icon_p_y);

	if (t->icon_title_w != None)
		XMoveWindow (dpy, t->icon_title_w, t->icon_p_x, t->icon_p_y + t->icon_p_height);

    SendPacket( -1, M_ICON_LOCATION, 7, t->w, t->frame,
			   (unsigned long)t, t->icon_p_x, t->icon_p_y, t->icon_p_width, t->icon_p_height);
}



/***********************************************************************
 *
 *  Procedure:
 *	DeIconify a window
 *
 ***********************************************************************/
void
DeIconify (ASWindow * tmp_win)
{
    ASWindow     *t;
	int           new_x, new_y, w2, h2;

	/* now de-iconify transients */
    for (t = Scr.ASRoot.next; t != NULL; t = t->next)
	{
		if ((t == tmp_win) ||
		    (get_flags(t->hints->flags, AS_Transient) && t->hints->transient_for == tmp_win->w))
		{
            iconify_window( t, False );
        }
	}

    /* autoplace sticky icons so they don't wind up over a stationary icon */
	AutoPlaceStickyIcons ();
}


/****************************************************************************
 *
 * Iconifies the selected window
 *
 ****************************************************************************/
void
Iconify (ASWindow * tmp_win)
{
	ASWindow     *t;
	XWindowAttributes winattrs;
	unsigned long eventMask;

	XGetWindowAttributes (dpy, tmp_win->w, &winattrs);
	eventMask = winattrs.your_event_mask;

	if ((tmp_win) && (tmp_win == Scr.Hilite) && (Scr.flags & ClickToFocus) && (tmp_win->next))
	{
		SetFocus (tmp_win->next->w, tmp_win->next, False);
	}
	/* iconify transients first */
	for (t = Scr.ASRoot.next; t != NULL; t = t->next)
	{
		if ((t == tmp_win) ||
		    (get_flags(t->hints->flags, AS_Transient) && t->hints->transient_for == tmp_win->w))
		{
			iconify_window( t, True );
		}
	}
}

/****************************************************************************
 *
 * Changes a window's icon, redoing the Window and/or Pixmap as necessary
 *
 ****************************************************************************/
void
ChangeIcon (ASWindow * win)
{

	if (win->flags & PIXMAP_OURS)
	{
		if (win->icon_pm_pixmap != None)
			UnloadImage (win->icon_pm_pixmap);
		win->icon_pm_pixmap = win->icon_pm_mask = None;
	}

	/* redo the icon if the window should be iconified */
	if (win->flags & ICONIFIED)
	{
		CreateIconWindow (win);
		AutoPlace (win);
		if (ASWIN_DESK(win) == Scr.CurrentDesk)
		{
			if (win->icon_pixmap_w != None)
				XMapWindow (dpy, win->icon_pixmap_w);
			if (win->icon_title_w != None)
				XMapWindow (dpy, win->icon_title_w);
		}
	}
}

/****************************************************************************
 *
 * Scan Style list for an icon file name
 *
 ****************************************************************************/
char         *
SearchIcon (ASWindow * tmp_win)
{
	return tmp_win->hints->icon_file;
}

void
RedoIconName (ASWindow * Tmp_win)
{
	if (Tmp_win->flags & SUPPRESSICON)
		return;

	if (Tmp_win->icon_title_w == None)
		return;

	/* resize the icon title window */
	ResizeIconWindow (Tmp_win);

	/* clear the icon window, and trigger a re-draw via an expose event */
	if (Tmp_win->flags & ICONIFIED)
		XClearArea (dpy, Tmp_win->icon_title_w, 0, 0, 0, 0, True);
}

static int    GetBitmapFile (ASWindow * tmp_win);
static int    GetColorIconFile (ASWindow * tmp_win);
static int    GetIconWindow (ASWindow * tmp_win);
static int    GetIconBitmap (ASWindow * tmp_win);

void
GetIcon (ASWindow * tmp_win)
{
	tmp_win->icon_pm_height = 0;
	tmp_win->icon_pm_width = 0;
	tmp_win->flags &= ~(ICON_OURS | PIXMAP_OURS | XPM_FLAG | SHAPED_ICON);

	/* see if the app supplies its own icon window */
	if ((Scr.flags & KeepIconWindows) && GetIconWindow (tmp_win));
	/* try to get icon bitmap from the application */
	else if ((Scr.flags & KeepIconWindows) && GetIconBitmap (tmp_win));
	/* check for a monochrome bitmap */
	else if (GetBitmapFile (tmp_win));
	/* check for a color pixmap */
	else if (GetColorIconFile (tmp_win));
}

/****************************************************************************
 *
 * Looks for a monochrome icon bitmap file
 *
 ****************************************************************************/
int
GetBitmapFile (ASWindow * tmp_win)
{
	char         *path = NULL;
	int           HotX, HotY;
	extern char  *IconPath;
	int           found = 1;

	if (tmp_win->icon_pm_file == NULL ||
		(path = findIconFile (tmp_win->icon_pm_file, IconPath, R_OK)) == NULL)
		return 0;

	if (XReadBitmapFile (dpy, Scr.Root, path,
						 (unsigned int *)&tmp_win->icon_pm_width,
						 (unsigned int *)&tmp_win->icon_pm_height,
						 &tmp_win->icon_pm_pixmap, &HotX, &HotY) != BitmapSuccess)
	{
		found = 0;
		tmp_win->icon_pm_width = 0;
		tmp_win->icon_pm_height = 0;
	}
	tmp_win->flags |= PIXMAP_OURS | ICON_OURS;
	free (path);
	return found;
}

/****************************************************************************
 *
 * Looks for a color XPM icon file
 *
 ****************************************************************************/
int
GetColorIconFile (ASWindow * tmp_win)
{
	extern char  *PixmapPath;
	char         *path = NULL;
	unsigned int  dum;
	int           dummy;
	Window        root;
	int           found = 0;

	if (tmp_win->icon_pm_file == NULL ||
		(path = findIconFile (tmp_win->icon_pm_file, PixmapPath, R_OK)) == NULL)
		return 0;

	if ((tmp_win->icon_pm_pixmap =
		 LoadImageWithMask (dpy, Scr.Root, 256, path, &(tmp_win->icon_pm_mask))) != None)
	{
		found = 1;
		XGetGeometry (dpy, tmp_win->icon_pm_pixmap, &root, &dummy, &dummy,
					  &(tmp_win->icon_pm_width), &(tmp_win->icon_pm_height), &dum, &dum);

		tmp_win->flags |= XPM_FLAG | PIXMAP_OURS | ICON_OURS;
		tmp_win->icon_pm_depth = Scr.d_depth;
	}
	free (path);
	return found;
}

/****************************************************************************
 *
 * Looks for an application supplied bitmap or pixmap
 *
 ****************************************************************************/
int
GetIconBitmap (ASWindow * tmp_win)
{
	if ( !get_flags(tmp_win->hints->flags, AS_ClientIconPixmap) ||
		  tmp_win->hints->icon.pixmap == None)
		return 0;

	XGetGeometry (dpy, tmp_win->hints->icon.pixmap, &JunkRoot, &JunkX, &JunkY,
				  (unsigned int *)&tmp_win->icon_pm_width,
				  (unsigned int *)&tmp_win->icon_pm_height, &JunkBW, &JunkDepth);
	tmp_win->icon_pm_pixmap = tmp_win->hints->icon.pixmap;
	tmp_win->icon_pm_depth = JunkDepth;

	tmp_win->icon_pm_mask = tmp_win->hints->icon_mask;

	tmp_win->flags |= ICON_OURS;
	tmp_win->flags &= ~PIXMAP_OURS;

	return 1;
}

/****************************************************************************
 *
 * Looks for an application supplied icon window
 *
 ****************************************************************************/
int
GetIconWindow (ASWindow * tmp_win)
{
	if ( ((tmp_win->hints->flags &(AS_ClientIcon|AS_ClientIconPixmap)) != AS_ClientIcon) ||
		 tmp_win->hints->icon.window == None)
		return 0;

	if (XGetGeometry (dpy, tmp_win->hints->icon.window, &JunkRoot,
					  &JunkX, &JunkY, (unsigned int *)&tmp_win->icon_pm_width,
					  (unsigned int *)&tmp_win->icon_pm_height, &JunkBW, &JunkDepth) == 0)
	{
		fprintf (stderr, "Help! Bad Icon Window!\n");
	}
	tmp_win->icon_pm_width += JunkBW << 1;
	tmp_win->icon_pm_height += JunkBW << 1;
	/*
	 * Now make the new window the icon window for this window,
	 * and set it up to work as such (select for key presses
	 * and button presses/releases, set up the contexts for it,
	 * and define the cursor for it).
	 */
	/* Make sure that the window is a child of the root window ! */
	/* Olwais screws this up, maybe others do too! */
	tmp_win->icon_pixmap_w = tmp_win->hints->icon.window;

	XReparentWindow (dpy, tmp_win->icon_pixmap_w, Scr.Root, 0, 0);
	tmp_win->flags &= ~ICON_OURS;
	return 1;
}
#endif

