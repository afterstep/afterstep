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

	if (!(tmp_win->flags & TITLE))
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
		for (i = 0; i < 5; i++)
		{
			if (tmp_win->right_w[i] != None)
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
	for (i = 0; i < 5; i++)
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
   ** button argument:
   ** 0 == leftmost button
   ** 1 == rightmost button
   ** 2 == next to leftmost button
   ** 3 == next to rightmost button
   ** etc.
 */
Bool
create_titlebutton_balloon (ASWindow * tmp_win, int button)
{
	int           n = button / 2 + 5 * (button & 1);
	char         *str;

	if (!(tmp_win->flags & TITLE))
		return False;
	if (!(button & 1) && button / 2 < Scr.nr_left_buttons &&
		Scr.buttons[button + 1].width > 0 &&
		!(tmp_win->buttons & (BUTTON1 << button)))
	{
		if ((str = list_functions_by_context (C_L1 << n)) != NULL)
		{
			balloon_new_with_text (dpy, tmp_win->left_w[button / 2], str);
			free (str);
		}
	}
	if ((button & 1) && button / 2 < Scr.nr_right_buttons &&
		Scr.buttons[(button + 1) % 10].width > 0 &&
		!(tmp_win->buttons & (BUTTON1 << button)))
	{
		if ((str = list_functions_by_context (C_L1 << n)) != NULL)
		{
			balloon_new_with_text (dpy, tmp_win->right_w[button / 2], str);
			free (str);
		}
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

	if (!(tmp_win->flags & TITLE))
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
	for (i = 0; i < 5; i++)
	{
		if ((i < Scr.nr_left_buttons) &&
			(Scr.buttons[i + i + 1].width > 0) &&
			!(tmp_win->buttons & (BUTTON1 << (i + i))))
		{
			tmp_win->left_w[i] =
				create_visual_window (Scr.asv, tmp_win->frame, -999, -999,
									  Scr.buttons[i + i + 1].width, Scr.buttons[i + i + 1].height,
									  0, InputOutput, valuemask, &attributes);
			XSaveContext (dpy, tmp_win->left_w[i], ASContext, (caddr_t) tmp_win);
			tmp_win->nr_left_buttons++;
			tmp_win->space_taken_left_buttons +=
				Scr.buttons[i + i + 1].width + Scr.TitleButtonSpacing;
			create_titlebutton_balloon (tmp_win, i + i);
		}
		if ((i < Scr.nr_right_buttons) &&
			(Scr.buttons[(i + i + 2) % 10].width > 0) &&
			!(tmp_win->buttons & (BUTTON1 << (i * 2 + 1))))
		{
			tmp_win->right_w[i] =
				create_visual_window (Scr.asv, tmp_win->frame, -999, -999,
									  Scr.buttons[(i + i + 2) % 10].width,
									  Scr.buttons[(i + i + 2) % 10].height, 0, InputOutput,
									  valuemask, &attributes);
			XSaveContext (dpy, tmp_win->right_w[i], ASContext, (caddr_t) tmp_win);
			tmp_win->nr_right_buttons++;
			tmp_win->space_taken_right_buttons +=
				Scr.buttons[(i + i + 2) % 10].width + Scr.TitleButtonSpacing;
			create_titlebutton_balloon (tmp_win, i + i + 1);
		}
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
	name_list     nl;

#ifdef I18N
	char        **list;
	int           num;
#endif
	extern Bool   PPosOverride;
	ASRawHints    raw_hints ;
    ASHints      *hints  = NULL;
    ASStatusHints status;


	/* allocate space for the afterstep window */
	tmp_win = (ASWindow *) calloc (1, sizeof (ASWindow));
	if (tmp_win == (ASWindow *) 0)
		return NULL;

	NeedToResizeToo = False;

	init_titlebar_windows (tmp_win, False);
	init_titlebutton_windows (tmp_win, False);

	tmp_win->flags = VISIBLE;
	tmp_win->w = w;
	tmp_win->cmap_windows = (Window *) NULL;
	tmp_win->layer = 0;

	if (XGetGeometry (dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
					  &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
	{
		free ((char *)tmp_win);
		return (NULL);
	}
	
	if( collect_hints( &Scr, w, HINT_ANY, &raw_hints ) )
    {
        if( is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
            print_hints( NULL, NULL, &raw_hints );
        hints = merge_hints( &raw_hints, NULL, &status, Scr.supported_hints, HINT_ANY, NULL );
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
			show_warning( "Failed to merge window management hints for window %X", w );
		tmp_win->hints = hints ;
    }else
		show_warning( "Unable to obtain window management hints for window %X", w );


/*  fprintf( stderr, "[%s]: %dx%d%+d%+d\n", tmp_win->name, JunkWidth, JunkHeight, JunkX, JunkY );
*/
	tmp_win->focus_sequence = 1;
	SetCirculateSequence (tmp_win, -1);

	FetchWmProtocols (tmp_win);
	FetchWmColormapWindows (tmp_win);
	if (!XGetWindowAttributes (dpy, tmp_win->w, &tmp_win->attr))
		tmp_win->attr.colormap = Scr.ASRoot.attr.colormap;

	tmp_win->wmhints = XGetWMHints (dpy, tmp_win->w);

	if (XGetTransientForHint (dpy, tmp_win->w, &tmp_win->transientfor))
		tmp_win->flags |= TRANSIENT;
	else
		tmp_win->flags &= ~TRANSIENT;

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


	/* if the window is in the NoTitle list, or is a transient,
	 *  dont decorate it.
	 * If its a transient, and DecorateTransients was specified,
	 *  decorate anyway
	 */
#ifndef NO_TEXTURE
	/*  Assume that we'll decorate */
	if (DecorateFrames)
		tmp_win->flags |= FRAME;
	else
#endif /* !NO_TEXTURE */
		tmp_win->flags |= BORDER;
	tmp_win->flags |= TITLE;

	style_init (&nl);

	nl.off_buttons = tmp_win->buttons;
	style_fill_by_name (&nl, &(tmp_win->hints->names[0]));
	tmp_win->buttons = nl.off_buttons;

	if (!(nl.off_flags & STYLE_FOCUS_FLAG) ||
		(tmp_win->style_focus = mystyle_find (nl.style_focus)) == NULL)
		tmp_win->style_focus = Scr.MSFWindow;
	if (!(nl.off_flags & STYLE_UNFOCUS_FLAG) ||
		(tmp_win->style_unfocus = mystyle_find (nl.style_unfocus)) == NULL)
		tmp_win->style_unfocus = Scr.MSUWindow;
	if (!(nl.off_flags & STYLE_STICKY_FLAG) ||
		(tmp_win->style_sticky = mystyle_find (nl.style_sticky)) == NULL)
		tmp_win->style_sticky = Scr.MSSWindow;

	GetMwmHints (tmp_win);

	SelectDecor (tmp_win, nl.off_flags, nl.border_width, nl.resize_width);

	if (nl.ViewportX >= 0 || nl.ViewportY >= 0)
	{
		if (nl.off_flags & STICKY_FLAG)
			nl.ViewportX = nl.ViewportY = -1;
		else
		{
			if (nl.ViewportX < 0)
				nl.ViewportX = Scr.Vx;
			if (nl.ViewportY < 0)
				nl.ViewportY = Scr.Vy;
			if (!PPosOverride)
				MoveViewport (nl.ViewportX, nl.ViewportY, 0);
		}
	}

	if (nl.off_flags & NOFOCUS_FLAG)
		tmp_win->focus_var = 1;
	else
		tmp_win->focus_var = 0;

	if (nl.off_flags & NOFOCUS_FLAG)
		tmp_win->flags |= NOFOCUS;
	if (nl.off_flags & START_ICONIC_FLAG)
		tmp_win->flags |= STARTICONIC;
	if (nl.off_flags & LAYER_FLAG)
		tmp_win->layer = nl.layer;
	if (nl.off_flags & AVOID_COVER_FLAG)
		tmp_win->flags |= AVOID_COVER;
	if (nl.off_flags & VERTICAL_TITLE_FLAG)
		tmp_win->flags |= VERTICAL_TITLE;
	if (nl.off_flags & STICKY_FLAG)
		tmp_win->flags |= STICKY;
	if (nl.off_flags & LISTSKIP_FLAG)
		tmp_win->flags |= WINDOWLISTSKIP;
	if (nl.off_flags & CIRCULATESKIP_FLAG)
		tmp_win->flags |= CIRCULATESKIP;

	if (!(nl.off_flags & NOHANDLES_FLAG))
		tmp_win->flags |= HANDLES;
	else
		tmp_win->flags &= ~FRAME;

/*  if (!(nl.off_flags & NOFRAME_FLAG))
   tmp_win->flags |= FRAME;     */

	if (nl.off_flags & SUPPRESSICON_FLAG)
		tmp_win->flags |= SUPPRESSICON;
	if (nl.off_flags & NOICON_TITLE_FLAG)
		tmp_win->flags |= NOICON_TITLE;

	if (Scr.flags & SuppressIcons)
		tmp_win->flags |= SUPPRESSICON;

	/* find a suitable icon pixmap */
	if (nl.off_flags & ICON_FLAG)
	{
		/* an icon was specified */
		tmp_win->icon_pm_file = nl.icon_file;
	} else if ((Scr.flags & KeepIconWindows) && (tmp_win->wmhints != NULL)
			   && (tmp_win->wmhints->flags & (IconWindowHint | IconPixmapHint)))
	{
		/* window has its own icon */
		tmp_win->icon_pm_file = NULL;
	} else
	{
		/* use default icon */
		tmp_win->icon_pm_file = Scr.DefaultIcon;
	}

	GetWindowSizeHints (tmp_win);

	if (get_flags (nl.off_flags, PREPOS_FLAG))
	{
		if (!get_flags (tmp_win->normal_hints.flags, USPosition))
		{
			if (get_flags (nl.PreposFlags, XValue))
			{
				if (get_flags (nl.PreposFlags, XNegative))
					tmp_win->attr.x = Scr.MyDisplayWidth + nl.PreposX;
				else
					tmp_win->attr.x = nl.PreposX;
			}
			if (get_flags (nl.PreposFlags, YValue))
			{
				if (get_flags (nl.PreposFlags, YNegative))
					tmp_win->attr.y = Scr.MyDisplayHeight + nl.PreposY;
				else
					tmp_win->attr.y = nl.PreposY;
			}
		}
		if (!get_flags (tmp_win->normal_hints.flags, USSize))
		{
			if (get_flags (nl.PreposFlags, WidthValue))
				tmp_win->attr.width = nl.PreposWidth;
			if (get_flags (nl.PreposFlags, HeightValue))
				tmp_win->attr.height = nl.PreposHeight;
		}
	}
	/* size and place the window */
	set_titlebar_geometry (tmp_win);

	get_frame_geometry (tmp_win, tmp_win->attr.x, tmp_win->attr.y, tmp_win->attr.width,
						tmp_win->attr.height, NULL, NULL, &tmp_win->frame_width,
						&tmp_win->frame_height);
	ConstrainSize (tmp_win, &tmp_win->frame_width, &tmp_win->frame_height);

	if (!PlaceWindow (tmp_win, nl.off_flags, nl.Desk))
	{
		Destroy (tmp_win, False);
		return NULL;
	}

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
		XFree (tmp_win->wmhints);
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
	if (tmp_win->flags & BORDER)
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
	if (tmp_win->flags & BORDER)
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

	tmp_win->icon_pm_pixmap = None;
	tmp_win->icon_pm_mask = None;

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
	if (tmp_win->flags & BORDER)
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
		for (i = 0; i < MAX_BUTTONS; i++)
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

	FetchWmProtocols (tmp_win);
	FetchWmColormapWindows (tmp_win);
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

				for (i = 0; i < MAX_BUTTONS; i++)
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

/***********************************************************************
 *
 *  Procedure:
 *	FetchWMProtocols - finds out which protocols the window supports
 *
 *  Inputs:
 *	tmp - the afterstep window structure to use
 *
 ***********************************************************************/
void
FetchWmProtocols (ASWindow * tmp)
{
	unsigned long flags = 0L;
	Atom         *protocols = NULL, *ap;
	int           i, n;
	Atom          atype;
	int           aformat;
	unsigned long bytes_remain, nitems;

	if (tmp == NULL)
		return;
	/* First, try the Xlib function to read the protocols.
	 * This is what Twm uses. */
	if (XGetWMProtocols (dpy, tmp->w, &protocols, &n))
	{
		for (i = 0, ap = protocols; i < n; i++, ap++)
		{
			if (*ap == (Atom) _XA_WM_TAKE_FOCUS)
				flags |= DoesWmTakeFocus;
			if (*ap == (Atom) _XA_WM_DELETE_WINDOW)
				flags |= DoesWmDeleteWindow;
		}
		if (protocols)
			XFree ((char *)protocols);
	} else
	{
		/* Next, read it the hard way. mosaic from Coreldraw needs to 
		 * be read in this way. */
		if ((XGetWindowProperty (dpy, tmp->w, _XA_WM_PROTOCOLS, 0L, 10L, False,
								 _XA_WM_PROTOCOLS, &atype, &aformat, &nitems,
								 &bytes_remain, (unsigned char **)&protocols)) == Success)
		{
			for (i = 0, ap = protocols; i < nitems; i++, ap++)
			{
				if (*ap == (Atom) _XA_WM_TAKE_FOCUS)
					flags |= DoesWmTakeFocus;
				if (*ap == (Atom) _XA_WM_DELETE_WINDOW)
					flags |= DoesWmDeleteWindow;
			}
			if (protocols)
				XFree ((char *)protocols);
		}
	}
	tmp->flags |= flags;
	return;
}

/***********************************************************************
 *
 *  Procedure:
 *	GetWindowSizeHints - gets application supplied size info
 *
 *  Inputs:
 *	tmp - the afterstep window structure to use
 *
 ***********************************************************************/
void
GetWindowSizeHints (ASWindow * tmp)
{
	long          supplied = 0;

	if (!XGetWMNormalHints (dpy, tmp->w, &tmp->normal_hints, &supplied))
		tmp->normal_hints.flags = 0;

	/* Beat up our copy of the hints, so that all important field are
	 * filled in! */
	if (tmp->normal_hints.flags & PResizeInc)
	{
		if (tmp->normal_hints.width_inc == 0)
			tmp->normal_hints.width_inc = 1;
		if (tmp->normal_hints.height_inc == 0)
			tmp->normal_hints.height_inc = 1;
	} else
	{
		tmp->normal_hints.width_inc = 1;
		tmp->normal_hints.height_inc = 1;
	}

	/*
	 * ICCCM says that PMinSize is the default if no PBaseSize is given,
	 * and vice-versa.
	 */

	if (!(tmp->normal_hints.flags & PBaseSize))
	{
		if (tmp->normal_hints.flags & PMinSize)
		{
			tmp->normal_hints.base_width = tmp->normal_hints.min_width;
			tmp->normal_hints.base_height = tmp->normal_hints.min_height;
		} else
		{
			tmp->normal_hints.base_width = 0;
			tmp->normal_hints.base_height = 0;
		}
	}
	if (!(tmp->normal_hints.flags & PMinSize))
	{
		tmp->normal_hints.min_width = tmp->normal_hints.base_width;
		tmp->normal_hints.min_height = tmp->normal_hints.base_height;
	}
	if (!(tmp->normal_hints.flags & PMaxSize))
	{
		tmp->normal_hints.max_width = MAX_WINDOW_WIDTH;
		tmp->normal_hints.max_height = MAX_WINDOW_HEIGHT;
	}
	if (tmp->normal_hints.max_width < tmp->normal_hints.min_width)
		tmp->normal_hints.max_width = MAX_WINDOW_WIDTH;
	if (tmp->normal_hints.max_height < tmp->normal_hints.min_height)
		tmp->normal_hints.max_height = MAX_WINDOW_HEIGHT;

	/* Zero width/height windows are bad news! */
	if (tmp->normal_hints.min_height <= 0)
		tmp->normal_hints.min_height = 1;
	if (tmp->normal_hints.min_width <= 0)
		tmp->normal_hints.min_width = 1;

	if (!(tmp->normal_hints.flags & PWinGravity))
	{
		tmp->normal_hints.win_gravity = NorthWestGravity;
		tmp->normal_hints.flags |= PWinGravity;
	}
}
