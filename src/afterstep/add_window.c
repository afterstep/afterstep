/*
 * Copyright (c) 1998 Makoto Kato <m_kato@ga2.so-net.ne.jp>
 * Copyright (C) 1997 Dong-hwa Oh <siage@nownuri.net>
 * Copyright (C) 1997 Tomonori <manome@itlb.te.noda.sut.ac.jp>
 * Copyright (C) 1995 Bo Yang
 * Copyright (C) 1993 Robert Nation
 * Copyright (C) 1993 Frank Fejes
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
 * This module is based on Twm, but has been SIGNIFICANTLY modified
 * by Rob Nation
 * by Bo Yang
 * by Dong-hwa Oh <siage@nownuri.net>          ) korean support patch/
 * by Tomonori <manome@itlb.te.noda.sut.ac.jp> )
 ****************************************************************************/

/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/


/**********************************************************************
 *
 * Add a new window, put the titlbar and other stuff around
 * the window
 *
 **********************************************************************/
#include "../../configure.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/parser.h"
#include "../../include/clientprops.h"
#include "../../include/hints.h"

#include "menus.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#include <X11/Xresource.h>
#endif /* SHAPE */

extern int    LastWarpIndex;
char          NoName[] = "Untitled";		   /* name if no name is specified */

void
init_titlebar_windows (ASWindow * tmp_win, Bool free_resources)
{
	if (free_resources)
	{
		if (tmp_win->title_w != None)
		{
			XDestroyWindow (dpy, tmp_win->title_w);
			XDeleteContext (dpy, tmp_win->title_w, ASContext);
		}
	}
	tmp_win->title_w = None;
}

/*
 * Create the titlebar.
 * We place the window anywhere, and let SetupFrame() take care of
 *  putting it where it belongs.
 */
Bool
create_titlebar_windows (ASWindow * tmp_win)
{
	unsigned long valuemask;
	XSetWindowAttributes attributes;

	if (!ASWIN_HFLAGS(tmp_win, AS_Titlebar))
		return False;

	valuemask = CWBackPixmap | CWBorderPixel | CWCursor | CWEventMask;
	attributes.background_pixmap = ParentRelative;
	attributes.border_pixel = Scr.asv->black_pixel;
	if (Scr.flags & BackingStore)
	{
		valuemask |= CWBackingStore;
		attributes.backing_store = WhenMapped;
	}
	attributes.event_mask = (ButtonPressMask | ButtonReleaseMask | ExposureMask |
							 EnterWindowMask | LeaveWindowMask);
	attributes.cursor = Scr.ASCursors[TITLE_CURSOR];
	tmp_win->title_x = -999;
	tmp_win->title_y = -999;
	tmp_win->title_w = create_visual_window (Scr.asv, tmp_win->frame,
											 tmp_win->title_x, tmp_win->title_y,
											 tmp_win->title_width,
											 (tmp_win->title_height + tmp_win->bw), 0, InputOutput,
											 valuemask, &attributes);
	XSaveContext (dpy, tmp_win->title_w, ASContext, (caddr_t) tmp_win);
	return True;
}

void
init_titlebutton_windows (ASWindow * tmp_win, Bool free_resources)
{
	int           i;

	if (free_resources)
	{
		for (i = 0; i < (TITLE_BUTTONS>>1); i++)
		{
			if (tmp_win->left_w[i] != None)
			{
				balloon_delete (balloon_find (tmp_win->left_w[i]));
				XDestroyWindow (dpy, tmp_win->left_w[i]);
				XDeleteContext (dpy, tmp_win->left_w[i], ASContext);
			}
			if (tmp_win->right_w[i] != None)
			{
				balloon_delete (balloon_find (tmp_win->left_w[i]));
				XDestroyWindow (dpy, tmp_win->right_w[i]);
				XDeleteContext (dpy, tmp_win->right_w[i], ASContext);
			}
		}
	}
	for (i = 0; i < (TITLE_BUTTONS>>1); i++)
	{
		tmp_win->left_w[i] = None;
		tmp_win->right_w[i] = None;
	}
	tmp_win->nr_right_buttons = 0;
	tmp_win->nr_left_buttons = 0;
	tmp_win->space_taken_left_buttons = 0;
	tmp_win->space_taken_right_buttons = 0;
}

/*
   ** returns a newline delimited list of the Mouse functions bound to a
   ** given context, in human readable form
 */
char         *
list_functions_by_context (int context)
{
	MouseButton  *btn;
	char         *str = NULL;
	int           allocated_bytes = 0;

	for (btn = Scr.MouseButtonRoot; btn != NULL; btn = btn->NextButton)
		if (btn->Context & context)
		{
			extern SyntaxDef FuncSyntax;
			TermDef      *fterm;

			fterm = FindTerm (&FuncSyntax, TT_ANY, btn->func);
			if (fterm != NULL)
			{
				char         *ptr;

				if (str)
				{
					str = realloc (str, allocated_bytes + 1 + fterm->keyword_len + 32);
					ptr = str + allocated_bytes;
					*(ptr++) = '\n';
				} else
					ptr = str = safemalloc (fterm->keyword_len + 32);

				sprintf (ptr, "%s: ", fterm->keyword);
				ptr += fterm->keyword_len + 2;
				if (btn->Modifier & ShiftMask)
				{
					strcat (ptr, "Shift+");
					ptr += 8;
				}
				if (btn->Modifier & ControlMask)
				{
					strcat (ptr, "Ctrl+");
					ptr += 7;
				}
				if (btn->Modifier & Mod1Mask)
				{
					strcat (ptr, "Meta+");
					ptr += 7;
				}
				sprintf (ptr, "Button %d", btn->Button);
				allocated_bytes = strlen (str);
			}
		}
	return str;
}

/*
   ** button argument - is translated button number !!!!
   ** 0 1 2 3 4  text 9 8 7 6 5
   ** none of that old crap !!!!
 */
Bool
create_titlebutton_balloon (ASWindow * tmp_win, int b)
{
/*	int           n = button / 2 + 5 * (button & 1); */
	char         *str = NULL ;
	Window 		  w = None;

	if (!ASWIN_HFLAGS(tmp_win, AS_Titlebar) || 
	    Scr.buttons[b].width <= 0 || IsBtnDisabled(tmp_win, b ))
		return False;
		
	if (IsLeftButton(b))
	{ 
		str = list_functions_by_context (C_L1 << b);
		w = tmp_win->left_w[b];
	}else
	{
		int rb = RightButtonIdx(b);
		str = list_functions_by_context (C_R1 << rb);
		w = tmp_win->right_w[rb];
	}
	if( str )
	{
		if( w ) 
			balloon_new_with_text (dpy, w, str);
		free (str);
	}
	return True;
}

/*
 * Create the titlebar buttons.
 * We place the windows anywhere, and let SetupFrame() take care of
 *  putting them where they belong.
 */
Bool
create_titlebutton_windows (ASWindow * tmp_win)
{
	int           i;
	unsigned long valuemask;
	XSetWindowAttributes attributes;

	if (!ASWIN_HFLAGS(tmp_win, AS_Titlebar))
		return False;
	valuemask = CWBackPixmap | CWBorderPixel | CWCursor | CWEventMask;
	attributes.background_pixmap = ParentRelative;
	attributes.border_pixel = Scr.asv->black_pixel;
	if (Scr.flags & BackingStore)
	{
		valuemask |= CWBackingStore;
		attributes.backing_store = WhenMapped;
	}
	attributes.event_mask = (ButtonPressMask | ButtonReleaseMask | ExposureMask |
							 EnterWindowMask | LeaveWindowMask);
	attributes.cursor = Scr.ASCursors[SYS];
	tmp_win->nr_left_buttons = 0;
	tmp_win->nr_right_buttons = 0;
	tmp_win->space_taken_left_buttons = 0;
	tmp_win->space_taken_right_buttons = 0;
	for (i = 0; i < TITLE_BUTTONS; i++)
	{
		Window w ;
		if( Scr.buttons[i].width <= 0 || IsBtnDisabled(tmp_win, i ) )
			continue;
		w = create_visual_window (Scr.asv, tmp_win->frame, -999, -999,
		  						  Scr.buttons[i].width, Scr.buttons[i].height,
								  0, InputOutput, valuemask, &attributes);
		XSaveContext (dpy, w, ASContext, (caddr_t) tmp_win);

		if (IsLeftButton(i))
		{
			tmp_win->left_w[i] = w ;
			tmp_win->nr_left_buttons++;
			tmp_win->space_taken_left_buttons +=
				Scr.buttons[i].width + Scr.TitleButtonSpacing;
		}else
		{
			int rb = RightButtonIdx(i);
			tmp_win->right_w[rb] = w ;
			tmp_win->nr_right_buttons++;
			tmp_win->space_taken_right_buttons +=
				Scr.buttons[i].width + Scr.TitleButtonSpacing;
		}
		create_titlebutton_balloon (tmp_win, i);
	}

	/* if left buttons exist, remove one extraneous space so title is shown correctly */
	if (tmp_win->space_taken_left_buttons)
		tmp_win->space_taken_left_buttons -= Scr.TitleButtonSpacing;
	/* if right buttons exist, remove one extraneous space so title is shown correctly */
	if (tmp_win->space_taken_right_buttons)
		tmp_win->space_taken_right_buttons -= Scr.TitleButtonSpacing;

	/* add borders if Scr.TitleButtonStyle == 0 */
	if (!Scr.TitleButtonStyle)
	{
		tmp_win->space_taken_left_buttons += 6;
		tmp_win->space_taken_right_buttons += 6;
	}

	return True;
}

void 
bind_aswindow_styles(ASWindow *t)
{
	char **styles_names = t->hints->mystyle_names ;
	
	t->style_focus = styles_names[BACK_FOCUSED]?mystyle_find (styles_names[BACK_FOCUSED]):NULL ;
	if( t->style_focus == NULL )
		t->style_focus = Scr.MSFWindow;
	t->style_unfocus = styles_names[BACK_UNFOCUSED]?mystyle_find (styles_names[BACK_UNFOCUSED]):NULL ;
	if( t->style_unfocus == NULL )
		t->style_unfocus = Scr.MSUWindow;
	t->style_sticky = styles_names[BACK_STICKY]?mystyle_find (styles_names[BACK_STICKY]):NULL ;
	if( t->style_sticky == NULL )
		t->style_sticky = Scr.MSSWindow;
}

/****************************************************************************
 * Interprets the property MOTIF_WM_HINTS, sets decoration and functions
 * accordingly
 *****************************************************************************/
void
SelectDecor (ASWindow * t)
{
	int border_width =0, resize_width = 0;
	ASHints *hints = t->hints ;
	ASFlagType tflags = hints->flags ;

	border_width = get_flags(tflags, AS_Border)?hints->border_width:Scr.NoBoundaryWidth;
	resize_width = get_flags(tflags, AS_Handles)?hints->handle_width:Scr.BoundaryWidth;
	
#ifdef SHAPE
	if (t->wShaped)
	{
		tflags &= ~(AS_Border | AS_Frame);
		clear_flags(hints->function_mask,AS_FuncResize);
	}
#endif
	/* Assume no decorations, and build up */
	t->flags &= ~(FRAME);
	t->boundary_width = 0;
	t->boundary_height = 0;
	t->corner_width = 0;
	t->title_width = 0;
	t->title_height = 0;
	t->button_height = 0;

	if (!get_flags(hints->function_mask,AS_FuncResize))
	{
		/* a wide border, with corner tiles */
#ifndef NO_TEXTURE
		if (DecorateFrames && get_flags(tflags,AS_Frame))
			t->flags |= FRAME;
#endif /* !NO_TEXTURE */
	}
	if (!get_flags(hints->function_mask,AS_FuncPopup))
		disable_titlebuttons_with_function (t, F_POPUP);
	if (!get_flags(hints->function_mask,AS_FuncMinimize))
		disable_titlebuttons_with_function (t, F_ICONIFY);
	if (!get_flags(hints->function_mask,AS_FuncMaximize))
		disable_titlebuttons_with_function (t, F_MAXIMIZE);

	t->boundary_width = 0;
	t->boundary_height = ASWIN_HFLAGS(t, AS_Handles) ? Scr.BoundaryWidth : 0;
	t->corner_width = 16 + t->boundary_height;

	if( get_flags(tflags,AS_Frame) )
		t->bw = 0 ;
	else if( get_flags(tflags, AS_Handles))
		t->bw = border_width;
	else
		t->bw = 1;
		
	t->button_height = t->title_height - 7;
}

ASWindow *window2ASWindow( Window w )
{
	ASWindow *asw = Scr.ASRoot.next;
	
	while( asw && asw->w != w )	asw = asw->next;
	
	return asw;
}

/*
 * The following function was written for
 * new hints management code in libafterstep :
 */
Bool afterstep_parent_hints_func(Window parent, ASParentHints *dst )
{
	if( dst == NULL || parent == None ) return False ;

	memset( dst, 0x00, sizeof(ASParentHints));
	dst->parent = parent ;
	if( parent != Scr.Root )
	{
		ASWindow     *p;

        if ((p = window2ASWindow( parent )) == NULL) return False ;

        dst->desktop = p->status->desktop ;
		set_flags( dst->flags, AS_StartDesktop );

		/* we may need to move our viewport so that the parent becomes visible : */
        if ( !ASWIN_GET_FLAGS(p, AS_Iconic) )
		{
#if 0		
            int x, y ;
            unsigned int width, height ;
            x = p->status->x - p->decor->west ;
            y = p->status->y - p->decor->north ;
            width = p->status->width + p->decor->west + p->decor->east ;
            height = p->status->height + p->decor->north + p->decor->south ;

            if( (dst->viewport_x = calculate_viewport( &x, width, p->scr->Vx, p->scr->MyDisplayWidth, p->scr->VxMax)) != p->scr->Vx )
				set_flags( dst->flags, AS_StartViewportX );

            if( (dst->viewport_y = calculate_viewport( &y, height, p->scr->Vy, p->scr->MyDisplayHeight, p->scr->VyMax)) != p->scr->Vy )
				set_flags( dst->flags, AS_StartViewportY );
#endif				
		}
	}
	return True ;
}

static void
maximize_window_status( ASStatusHints *status, ASStatusHints *saved_status, ASStatusHints *adjusted_status, ASFlagType flags )
{
    ASRectangle rect ;
	rect.x = 0 ;
	rect.y = 0 ;
	rect.width = Scr.MyDisplayWidth ;
	rect.height = Scr.MyDisplayHeight ;

    if( get_flags( status->flags, AS_StartPositionUser|AS_StartPosition ) )
    {
        saved_status->x = status->x ;
        saved_status->y = status->y ;
        set_flags( saved_status->flags, AS_Position );
    }
    if( get_flags( status->flags, AS_StartSize ) )
    {
        saved_status->width = status->width ;
        saved_status->height = status->height ;
        set_flags( saved_status->flags, AS_Size );
    }
    if( get_flags( flags, AS_MaximizedX ) )
    {
        adjusted_status->x = rect.x ;
        adjusted_status->width = rect.width ;
    }
    if( get_flags( flags, AS_MaximizedY ) )
    {
        adjusted_status->y = rect.y ;
        adjusted_status->height = rect.height ;
    }
    set_flags( adjusted_status->flags, AS_Position|AS_Size );
}


Bool
init_aswindow_status( ASWindow *t, ASStatusHints *status ) 
{
	extern int PPosOverride ;
	ASStatusHints adjusted_status ;
	
	if( t->status == NULL )
		t->status = safecalloc(1, sizeof(ASStatusHints));

    if( get_flags( status->flags, AS_StartDesktop) && status->desktop != Scr.CurrentDesk )
        changeDesks( 0, status->desktop );
    t->status->desktop = Scr.CurrentDesk ;

    if( get_flags( status->flags, AS_StartViewportX))
        t->status->viewport_x = MIN(status->viewport_x,Scr.VxMax) ;
    else
        t->status->viewport_x = Scr.Vx ;

    if( get_flags( status->flags, AS_StartViewportY))
        t->status->viewport_y = MIN(status->viewport_y,Scr.VyMax) ;
    else
        t->status->viewport_y = Scr.Vy ;
    if( t->status->viewport_x != Scr.Vx || t->status->viewport_y != Scr.Vy )
        MoveViewport (t->status->viewport_x, t->status->viewport_y, 0);

    adjusted_status = *status ;

    if( PPosOverride )
        set_flags( adjusted_status.flags, AS_Position );
	else if( get_flags(Scr.flags, NoPPosition) &&
            !get_flags( status->flags, AS_StartPositionUser ) )
        clear_flags( adjusted_status.flags, AS_Position );

    if( get_flags( status->flags, AS_MaximizedX|AS_MaximizedY ))
        maximize_window_status( status, t->saved_status, &adjusted_status, status->flags );
    else if( !get_flags( adjusted_status.flags, AS_Position ))
        if( !place_aswindow( t, &adjusted_status ) )
            return False;

    if( is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
		print_status_hints( NULL, NULL, &adjusted_status );

	/* invalidating position in status so that change_placement would always work */
	t->status->x = adjusted_status.x-1000 ;
	t->status->y = adjusted_status.y-1000 ;
	t->status->width = adjusted_status.width-1000 ;
	t->status->height = adjusted_status.height-1000 ;

#if 0
    if( change_placement( t->scr, t->hints, t->status, &(t->anchor), &adjusted_status, t->scr->Vx, t->scr->Vy, adjusted_status.flags ) != 0 )
	{
        anchor_decor_client( t->decor, t->hints, t->status, &(t->anchor), t->scr->Vx, t->scr->Vy );
		configure_decor( t->decor, t->status );
	}
#endif	
    /* TODO: AS_Iconic */
	if (ASWIN_GET_FLAGS(t, AS_StartsIconic ))
		t->flags |= STARTICONIC;

	if( !ASWIN_GET_FLAGS(t, AS_StartLayer ) )
		ASWIN_LAYER(t) = AS_LayerNormal ;

    if( ASWIN_GET_FLAGS(t, AS_StartsSticky ) )
		t->flags |= STICKY;

    if( ASWIN_GET_FLAGS(t, AS_StartsShaded ) )
		t->flags |= SHADED;
	return True;
}

/***********************************************************************
 *
 *  Procedure:
 *	AddWindow - add a new window to the afterstep list
 *
 *  Returned Value:
 *	(ASWindow *) - pointer to the ASWindow structure
 *
 *  Inputs:
 *	w	- the window id of the window to add
 *	iconm	- flag to tell if this is an icon manager window
 *
 ***********************************************************************/
ASWindow     *
AddWindow (Window w)
{
	ASWindow     *tmp_win;					   /* new afterstep window structure */
	unsigned long valuemask;				   /* mask for create windows */
	XSetWindowAttributes attributes;		   /* attributes for create windows */
	int           i, width, height;
	int           a, b;
	extern Bool   NeedToResizeToo;
	extern ASWindow *colormap_win;

#ifdef I18N
	char        **list;
	int           num;
#endif
	ASRawHints    raw_hints ;
    ASHints      *hints  = NULL;
    ASStatusHints status;
	extern ASDatabase *Database;


	/* allocate space for the afterstep window */
	tmp_win = safecalloc (1, sizeof (ASWindow));
	if (tmp_win == (ASWindow *) 0)
		return NULL;

	NeedToResizeToo = False;

	init_titlebar_windows (tmp_win, False);
	init_titlebutton_windows (tmp_win, False);

	tmp_win->flags = VISIBLE;
	tmp_win->w = w;

	if (XGetGeometry (dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
					  &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
	{
		free ((char *)tmp_win);
		return (NULL);
	}

    set_parent_hints_func( afterstep_parent_hints_func ); /* callback for collect_hints() */
	
	if( collect_hints( &Scr, w, HINT_ANY, &raw_hints ) )
    {
        if( is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
            print_hints( NULL, NULL, &raw_hints );
        hints = merge_hints( &raw_hints, Database, &status, Scr.supported_hints, HINT_ANY, NULL );
        destroy_raw_hints( &raw_hints, True );
        if( hints )
        {
			show_debug( __FILE__, __FUNCTION__, __LINE__,  "Window management hints collected and merged for window %X", w );
            if( is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
			{
                print_clean_hints( NULL, NULL, hints );
				print_status_hints( NULL, NULL, &status );
			}
        }else
		{
			show_warning( "Failed to merge window management hints for window %X", w );
			free ((char *)tmp_win);
			return (NULL);
		}
		tmp_win->hints = hints ;
    }else
	{
		show_warning( "Unable to obtain window management hints for window %X", w );
		free ((char *)tmp_win);
		return (NULL);
	}

/*  fprintf( stderr, "[%s]: %dx%d%+d%+d\n", tmp_win->name, JunkWidth, JunkHeight, JunkX, JunkY );
*/
	tmp_win->focus_sequence = 1;
	SetCirculateSequence (tmp_win, -1);
	
	if (get_flags(tmp_win->hints->protocols,AS_DoesWmTakeFocus) )
		tmp_win->flags |= DoesWmTakeFocus;
	
	if (get_flags(tmp_win->hints->protocols,AS_DoesWmTakeFocus) )
		tmp_win->flags |= DoesWmDeleteWindow;

	if (!XGetWindowAttributes (dpy, tmp_win->w, &tmp_win->attr))
		tmp_win->attr.colormap = Scr.ASRoot.attr.colormap;

	if( get_flags(tmp_win->hints->flags,AS_Transient ) )
	{
		set_flags(tmp_win->flags, TRANSIENT);
	}else
		clear_flags(tmp_win->flags, ~TRANSIENT);

	tmp_win->old_bw = tmp_win->attr.border_width;

#ifdef SHAPE
	{
		int           xws, yws, xbs, ybs;
		unsigned      wws, hws, wbs, hbs;
		int           boundingShaped, clipShaped;

		XShapeSelectInput (dpy, tmp_win->w, ShapeNotifyMask);
		XShapeQueryExtents (dpy, tmp_win->w,
							&boundingShaped, &xws, &yws, &wws, &hws,
							&clipShaped, &xbs, &ybs, &wbs, &hbs);
		tmp_win->wShaped = boundingShaped;
	}
#endif /* SHAPE */

	bind_aswindow_styles(tmp_win);

	SelectDecor (tmp_win);

	if( get_flags(tmp_win->hints->flags,AS_AvoidCover))
		tmp_win->flags |= AVOID_COVER;
	if (get_flags(tmp_win->hints->flags,AS_SkipWinList))
		tmp_win->flags |= WINDOWLISTSKIP;
	if (get_flags(tmp_win->hints->flags,AS_DontCirculate))
		tmp_win->flags |= CIRCULATESKIP;
	if (!get_flags(tmp_win->hints->flags,AS_Icon) || get_flags(Scr.flags, SuppressIcons))
		tmp_win->flags |= SUPPRESSICON;
	else
	{	/* an icon was specified */
		tmp_win->icon_pm_file = NULL ;
		if( tmp_win->hints->icon.window != None )
		{
			
		}else
		{
			tmp_win->icon_pm_file = tmp_win->hints->icon_file;
			if(tmp_win->icon_pm_file == NULL )/* use default icon */
		  		tmp_win->icon_pm_file = Scr.DefaultIcon;
		}
	}
	tmp_win->icon_pm_pixmap = None;
	tmp_win->icon_pm_mask = None;

	if (!get_flags(tmp_win->hints->flags, AS_IconTitle))
		tmp_win->flags |= NOICON_TITLE;

	if( !init_aswindow_status( tmp_win, &status ) )
	{
		Destroy (tmp_win, False);
		return NULL;
	}


	/* Old stuff - needs to be updated to use tmp_win->hints :*/	
	/* size and place the window */
	set_titlebar_geometry (tmp_win);

	get_frame_geometry (tmp_win, tmp_win->attr.x, tmp_win->attr.y, tmp_win->attr.width,
						tmp_win->attr.height, NULL, NULL, &tmp_win->frame_width,
						&tmp_win->frame_height);
	ConstrainSize (tmp_win, &tmp_win->frame_width, &tmp_win->frame_height);

	get_frame_geometry (tmp_win, tmp_win->attr.x, tmp_win->attr.y, tmp_win->attr.width,
						tmp_win->attr.height, &tmp_win->frame_x, &tmp_win->frame_y,
						&tmp_win->frame_width, &tmp_win->frame_height);

	/*
	 * Make sure the client window still exists.  We don't want to leave an
	 * orphan frame window if it doesn't.  Since we now have the server
	 * grabbed, the window can't disappear later without having been
	 * reparented, so we'll get a DestroyNotify for it.  We won't have
	 * gotten one for anything up to here, however.
	 */
	XGrabServer (dpy);
	if (XGetGeometry (dpy, w, &JunkRoot, &JunkX, &JunkY,
					  &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
	{
		destroy_hints(tmp_win->hints, False);
		free ((char *)tmp_win);
		XUngrabServer (dpy);
		return (NULL);
	}
	XSetWindowBorderWidth (dpy, tmp_win->w, 0);

	tmp_win->flags &= ~ICONIFIED;
	tmp_win->flags &= ~ICON_UNMAPPED;
	tmp_win->flags &= ~MAXIMIZED;

	/* the newly created window will be on the top of the warp list */
	tmp_win->warp_index = 0;
	ChangeWarpIndex (++LastWarpIndex, F_WARP_F);

	/* add the window into the afterstep list */
	tmp_win->next = Scr.ASRoot.next;
	if (Scr.ASRoot.next != NULL)
		Scr.ASRoot.next->prev = tmp_win;
	tmp_win->prev = &Scr.ASRoot;
	Scr.ASRoot.next = tmp_win;

	/* create windows */

	valuemask = CWBorderPixel | CWCursor | CWEventMask;
	if (Scr.d_depth < 2)
	{
		attributes.background_pixmap = Scr.light_gray_pixmap;
		if (tmp_win->flags & STICKY)
			attributes.background_pixmap = Scr.sticky_gray_pixmap;
		valuemask |= CWBackPixmap;
	} else
	{
		attributes.background_pixmap = ParentRelative;
		valuemask |= CWBackPixmap;
	}
	attributes.border_pixel = Scr.asv->black_pixel;

	attributes.cursor = Scr.ASCursors[DEFAULT];
	attributes.event_mask = (SubstructureRedirectMask | ButtonPressMask |
							 ButtonReleaseMask | EnterWindowMask |
							 LeaveWindowMask | ExposureMask | FocusChangeMask);
	if (Scr.flags & SaveUnders)
	{
		valuemask |= CWSaveUnder;
		attributes.save_under = TRUE;
	}
	/* What the heck, we'll always reparent everything from now on! */
	tmp_win->frame =
		create_visual_window (Scr.asv, Scr.Root, tmp_win->frame_x, tmp_win->frame_y,
							  tmp_win->frame_width, tmp_win->frame_height,
							  tmp_win->bw, InputOutput, valuemask, &attributes);

	attributes.save_under = FALSE;

	/* Thats not all, we'll double-reparent the window ! */
	attributes.cursor = Scr.ASCursors[DEFAULT];
	tmp_win->Parent =
		create_visual_window (Scr.asv, tmp_win->frame, -999, -999, 16, 16,
							  tmp_win->bw, InputOutput, valuemask, &attributes);

#if 0
	/* ParentRelative is not safe unless we're sure that the bpp of the 
	   ** parent (root) window is the same as our visual's bpp. */
	XSetWindowBackgroundPixmap (dpy, tmp_win->frame, ParentRelative);
#endif
	XSetWindowBackgroundPixmap (dpy, tmp_win->Parent, ParentRelative);

	if (Scr.flags & BackingStore)
	{
		valuemask |= CWBackingStore;
		attributes.backing_store = WhenMapped;
	}
	attributes.event_mask = (ButtonPressMask | ButtonReleaseMask | ExposureMask |
							 EnterWindowMask | LeaveWindowMask);
	tmp_win->title_w = 0;
	/* title geometry can be anything - SetupFrame() will fix it */
	tmp_win->title_x = tmp_win->title_y = -999;
	if (ASWIN_HFLAGS(tmp_win, AS_Handles))
	{
		/* Just dump the windows any old place and left SetupFrame take
		 * care of the mess */
		for (i = 0; i < 2; i++)
		{
			attributes.cursor = Scr.ASCursors[BOTTOM_LEFT + i];
			tmp_win->corners[i] =
				create_visual_window (Scr.asv, tmp_win->frame, 0, 0,
									  tmp_win->corner_width, tmp_win->boundary_height,
									  0, InputOutput, valuemask, &attributes);
		}
	}
	if (ASWIN_HFLAGS(tmp_win, AS_Handles))
	{
		attributes.cursor = Scr.ASCursors[BOTTOM];
		tmp_win->side =
			create_visual_window (Scr.asv, tmp_win->frame, 0, 0, tmp_win->boundary_height,
								  tmp_win->boundary_height, 0, InputOutput, valuemask, &attributes);
	}
	create_titlebar_windows (tmp_win);
	create_titlebutton_windows (tmp_win);
#ifndef NO_TEXTURE
	frame_create_windows (tmp_win);
#endif /* !NO_TEXTURE */

	XMapSubwindows (dpy, tmp_win->frame);
	XRaiseWindow (dpy, tmp_win->Parent);
	XReparentWindow (dpy, tmp_win->w, tmp_win->Parent, 0, 0);

	valuemask = (CWEventMask | CWDontPropagate);
	attributes.event_mask = (StructureNotifyMask | PropertyChangeMask |
							 EnterWindowMask | LeaveWindowMask |
							 ColormapChangeMask | FocusChangeMask);
	attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;
	if (Scr.flags & AppsBackingStore)
	{
		valuemask |= CWBackingStore;
		attributes.backing_store = WhenMapped;
	}
	XChangeWindowAttributes (dpy, tmp_win->w, valuemask, &attributes);

	XAddToSaveSet (dpy, tmp_win->w);


	/*
	 * Reparenting generates an UnmapNotify event, followed by a MapNotify.
	 * Set the map state to FALSE to prevent a transition back to
	 * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
	 * again in HandleMapNotify.
	 */
	tmp_win->flags &= ~MAPPED;

	width = tmp_win->frame_width;
	height = tmp_win->frame_height;
	tmp_win->frame_width = 0;
	tmp_win->frame_height = 0;

#ifndef NO_TEXTURE
	tmp_win->backPixmap = None;
	tmp_win->backPixmap2 = None;
	tmp_win->backPixmap3 = None;
#endif
	SetupFrame (tmp_win, tmp_win->frame_x, tmp_win->frame_y, width, height, True);

	/* wait until the window is iconified and the icon window is mapped
	 * before creating the icon window 
	 */
	tmp_win->icon_title_w = None;
	GrabButtons (tmp_win);
	GrabKeys (tmp_win);
	XSaveContext (dpy, tmp_win->w, ASContext, (caddr_t) tmp_win);
	XSaveContext (dpy, tmp_win->frame, ASContext, (caddr_t) tmp_win);
	XSaveContext (dpy, tmp_win->Parent, ASContext, (caddr_t) tmp_win);
	if (ASWIN_HFLAGS(tmp_win, AS_Handles))
	{
		XSaveContext (dpy, tmp_win->side, ASContext, (caddr_t) tmp_win);
		for (i = 0; i < 2; i++)
		{
			XSaveContext (dpy, tmp_win->corners[i], ASContext, (caddr_t) tmp_win);
		}
	}
	RaiseWindow (tmp_win);
	XUngrabServer (dpy);

	XGetGeometry (dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
				  &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
	XTranslateCoordinates (dpy, tmp_win->frame, Scr.Root, JunkX, JunkY, &a, &b, &JunkChild);

	if (Scr.flags & ClickToFocus)
	{
		/* need to grab all buttons for window that we are about to
		 * unhighlight */
		for (i = 0; i < MAX_MOUSE_BUTTONS; i++)
			if (Scr.buttons2grab & (1 << i))
			{
				MyXGrabButton (dpy, i + 1, 0, tmp_win->frame,
							   True, ButtonPressMask, GrabModeSync,
							   GrabModeAsync, None, Scr.ASCursors[SYS]);
			}
	}
	BroadcastConfig (M_ADD_WINDOW, tmp_win);

	BroadcastName (M_WINDOW_NAME, tmp_win->w, tmp_win->frame,
				   (unsigned long)tmp_win, tmp_win->hints->names[0]);
	BroadcastName (M_ICON_NAME, tmp_win->w, tmp_win->frame,
				   (unsigned long)tmp_win, tmp_win->hints->icon_name);
	BroadcastName (M_RES_CLASS, tmp_win->w, tmp_win->frame,
				   (unsigned long)tmp_win, tmp_win->hints->res_class);
	BroadcastName (M_RES_NAME, tmp_win->w, tmp_win->frame,
				   (unsigned long)tmp_win, tmp_win->hints->res_name);

	if (!(XGetWindowAttributes (dpy, tmp_win->w, &(tmp_win->attr))))
		tmp_win->attr.colormap = Scr.ASRoot.attr.colormap;
	if (NeedToResizeToo)
	{
		XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
					  Scr.MyDisplayHeight,
					  tmp_win->frame_x + (tmp_win->frame_width >> 1),
					  tmp_win->frame_y + (tmp_win->frame_height >> 1));
		resize_window (tmp_win->w, tmp_win, 0, 0, 0, 0);
	}
	InstallWindowColormaps (colormap_win);
	if (!(tmp_win->flags & WINDOWLISTSKIP))
		update_windowList ();

	return (tmp_win);
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabButtons - grab needed buttons for the window
 *
 *  Inputs:
 *	tmp_win - the afterstep window structure to use
 *
 ***********************************************************************/
void
GrabButtons (ASWindow * tmp_win)
{
	MouseButton  *MouseEntry;

	MouseEntry = Scr.MouseButtonRoot;
	while (MouseEntry != (MouseButton *) 0)
	{
		if ((MouseEntry->func != (int)0) && (MouseEntry->Context & C_WINDOW))
		{
			if (MouseEntry->Button > 0)
			{
				MyXGrabButton (dpy, MouseEntry->Button,
							   MouseEntry->Modifier, tmp_win->w,
							   True, ButtonPressMask | ButtonReleaseMask,
							   GrabModeAsync, GrabModeAsync, None, Scr.ASCursors[DEFAULT]);
			} else
			{
				int           i;

				for (i = 0; i < MAX_MOUSE_BUTTONS; i++)
				{
					MyXGrabButton (dpy, i + 1, MouseEntry->Modifier, tmp_win->w,
								   True, ButtonPressMask | ButtonReleaseMask,
								   GrabModeAsync, GrabModeAsync, None, Scr.ASCursors[DEFAULT]);
				}
			}
		}
		MouseEntry = MouseEntry->NextButton;
	}
	return;
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabKeys - grab needed keys for the window
 *
 *  Inputs:
 *	tmp_win - the afterstep window structure to use
 *
 ***********************************************************************/
void
GrabKeys (ASWindow * tmp_win)
{
	FuncKey      *tmp;

	for (tmp = Scr.FuncKeyRoot; tmp != NULL; tmp = tmp->next)
	{
		if (tmp->cont & (C_WINDOW | C_TITLE | C_RALL | C_LALL | C_SIDEBAR))
			MyXGrabKey (dpy, tmp->keycode, tmp->mods, tmp_win->frame, True,
						GrabModeAsync, GrabModeAsync);
	}
	return;
}

