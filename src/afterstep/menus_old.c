/*
 * Copyright (c) 2002 Jan Fedak <jack at mobil dot cz>
 * Copyright (C) 1993 Rob Nation
 * Copyright (C) 1995 Bo Yang
 * Copyright (C) 1996 Frank Fejes
 * Copyright (C) 1996 Alfredo Kojima
 * Copyright (C) 1998 Guylhem Aznar
 * Copyright (C) 1998, 1999 Ethan Fischer
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

/***********************************************************************
 *
 * afterstep menu code
 *
 ***********************************************************************/
#include "../../configure.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>						   /* for XA_PIXMAP */

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

#include "menus.h"

Bool          HandleMenuButtonPress (MenuRoot * menu, XEvent * event);
Bool          HandleMenuButtonRelease (MenuRoot * menu, XEvent * event);
Bool          HandleMenuEnterNotify (MenuRoot * menu, XEvent * event);
Bool          HandleMenuLeaveNotify (MenuRoot * menu, XEvent * event);
Bool          HandleMenuMotionNotify (MenuRoot * menu, XEvent * event);
Bool          HandleMenuExpose (MenuRoot * menu, XEvent * event);

Bool          pin_menu (MenuRoot * menu);
Bool          unpin_menu (MenuRoot * menu);
Bool          map_menu (MenuRoot * menu, int context);
Bool          unmap_menu (MenuRoot * menu);
Bool          configure_menu (MenuRoot * menu, MenuRoot * parent);
void          select_item (MenuRoot * menu, MenuItem * item);
void          unselect_item (MenuRoot * menu, MenuItem * item);
Bool          do_menu_move (MenuRoot * menu, int buttonx, int buttony);
void          do_menu_function (MenuRoot * menu, MenuItem * item, XEvent * event);

int           menuFromFrameOrWindowOrTitlebar = FALSE;

extern int    Context, Button;
extern ASWindow *ButtonWindow, *Tmp_win;
extern XEvent Event;
extern int    DrawMenuBorders;
extern int    TextureMenuItemsIndividually;

int           Stashed_X, Stashed_Y;

/* pointer grab variables follow */
Bool          menu_has_pointer = False;

/* the menu that grabbed */
MenuRoot     *pointer_menu = NULL;

/* the menu mode */
enum
{
	MWAIT,
	MCLICK,
	MHOLD
}
menu_mode;

/* when the grab took place */
//Time          menu_grab_time;

void          DrawTrianglePattern (Window, GC, GC, GC, int, int, int, int);
void          DrawSeparator (Window, GC, GC, int, int, int, int, int);
void          draw_icon (Window w, MyIcon * icon, GC iconGC, int x, int y);

extern XContext MenuContext;

/****************************************************************************
 *
 * Initiates a menu pop-up, closes unpinned menus
 *
 ***************************************************************************/
void
do_menu (MenuRoot * menu, MenuRoot * parent)
{
	configure_menu (menu, parent);
	map_menu (menu, Context);
}

/****************************************************************************
 *
 * positions a menu, and sets up the colors/pixmaps if necessary
 * use the parent's position to place the menu
 * if parent == NULL, use the current pointer position
 *
 ***************************************************************************/
Bool
configure_menu (MenuRoot * menu, MenuRoot * parent)
{
	int           x, y;

	/* In case we wind up with a move from a menu which is
	 * from a window border, we'll return to here to start
	 * the move */

	if (menu == NULL)
		return False;

	XQueryPointer (dpy, Scr.Root, &JunkRoot, &JunkChild, &x, &y, &JunkX, &JunkY, &JunkMask);
/* position new menu */
	if (parent != NULL)
	{
		MenuItem     *item;

		/* find our entry in our parent menu */
		for (item = parent->first; item != NULL; item = item->next)
			if (item->menu == menu)
				break;
		/* Offset the new menu depending on the geometry of its parent menu */
		if (item != NULL)
		{
			x = parent->x + parent->width;
			y = parent->y;
			if (!(Scr.flags & MenusHigh))
				y += item->y_offset - menu->first->y_height;
		}

		menu->aw = parent->aw;
	} else
	{
		Stashed_X = x;
		Stashed_Y = y;
		/* save the window we were called from */
		menu->aw = ButtonWindow;
		x -= (menu->width >> 1);
		y -= (menu->first->y_height >> 1);
		if (Tmp_win != NULL)
		{
			Window        child;
			ASWindow     *win = Tmp_win;

			/* the x-coordinate calculation for buttons is wrong - should
			 * probably use XGetGeometry to get the right number */
			if ((Context & C_LALL) && win->left_w[Button] != None)
				XTranslateCoordinates (dpy, win->left_w[Button], Scr.Root, 0, 0, &x, &y, &child);
			if ((Context & C_RALL) && win->right_w[Button] != None)
				XTranslateCoordinates (dpy, win->right_w[Button], Scr.Root, 0, 0, &x, &y, &child);
			if (Context & (C_LALL | C_RALL | C_TITLE))
			{
				if (ASWIN_HFLAGS(win, AS_VerticalTitle))
					x = win->frame_x + win->bw + win->title_x + win->title_width + win->bw;
				else
					y = win->frame_y + win->bw + win->title_y + win->title_height + win->bw;
			}
		}
	}

	/* clip to screen */
	if (x > Scr.MyDisplayWidth - menu->width - 2)
		x = Scr.MyDisplayWidth - menu->width - 2;
	if (x < 0)
		x = 0;

	if (y > Scr.MyDisplayHeight - menu->height - 2)
		y = Scr.MyDisplayHeight - menu->height - 2;
	if (y < 0)
		y = 0;

	/* Warp pointer to middle of top line */
#ifndef NO_WARPPOINTERTOMENU
	XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
				  Scr.MyDisplayHeight, x + (menu->width >> 1), y + (menu->first->y_height >> 1));
#endif

	menu->x = x;
	menu->y = y;

	setup_menu_pixmaps (menu);

	if (menu->is_mapped == True)
	{
		XMoveResizeWindow (dpy, menu->w, menu->x, menu->y, menu->width, menu->height);
		XRaiseWindow (dpy, menu->w);
	}
	return True;
}

/****************************************************************************
 *
 * sets up a menu's colors/pixmaps if necessary
 *
 ***************************************************************************/
void
setup_menu_pixmaps (MenuRoot * menu)
{
#ifndef NO_TEXTURE
	int           title_y = 0, title_height, item_y = 0, item_height, width;
	int           x_offset = 0, y_offset = 0;

	if (menu == NULL)
		return;
	if (Scr.d_depth >= 8)
	{
		width = menu->width;
		title_height = 1;
		{
			/* find the largest single (title) menu item */
			MenuItem     *i;

			for (i = (*menu).first; i != NULL; i = (*i).next)
				if ((*i).func == F_TITLE && title_height < (*i).y_height)
				{
					title_y = (*i).y_offset;
					title_height = (*i).y_height;
				}
		}
		item_height = 1;
		if (TextureMenuItemsIndividually == 0 ||
			Scr.MSMenuItem->texture_type >= TEXTURE_TRANSPARENT)
		{
			/* find the largest span of non-titles */
			MenuItem     *i;

			for (i = (*menu).first; i != NULL; i = (*i).next)
			{
				int           h;
				MenuItem     *j;

				for (; (*i).func == F_TITLE && (*i).next != NULL; i = (*i).next);
				j = i;
				for (; (*i).next != NULL && (*(*i).next).func != F_TITLE; i = (*i).next);
				h = (*i).y_offset + (*i).y_height - (*j).y_offset;
				if (item_height < h)
				{
					item_y = (*j).y_offset;
					item_height = h;
				}
			}
		} else
			/* if (TextureMenuItemsIndividually == 1) */
		{
			/* find the largest single (non-title) menu item */
			MenuItem     *i;

			for (i = (*menu).first; i != NULL; i = (*i).next)
				if ((*i).func != F_TITLE && item_height < (*i).y_height)
					item_height = (*i).y_height;
		}
		if (DrawMenuBorders == 1)
		{
			title_height -= 3;
			item_height -= 3;
			x_offset += 1;
			y_offset += 1;
			width -= 2;
		} else if (DrawMenuBorders == 2 || DrawMenuBorders == 3)
		{
			x_offset += 1;
			width -= 2;
		}
		if (menu->titlebg != None && Scr.MSMenuTitle->texture_type >= TEXTURE_TRANSPARENT)
		{
			XFreePixmap (dpy, menu->titlebg);
			menu->titlebg = None;
		}
		if (menu->itembg != None && Scr.MSMenuItem->texture_type >= TEXTURE_TRANSPARENT)
		{
			XFreePixmap (dpy, menu->itembg);
			menu->itembg = None;
		}
		if (menu->itemhibg != None && Scr.MSMenuHilite->texture_type >= TEXTURE_TRANSPARENT)
		{
			XFreePixmap (dpy, menu->itemhibg);
			menu->itemhibg = None;
		}
		if (width > 0)
		{
			if (menu->titlebg == None && Scr.MSMenuTitle->texture_type > 0 && title_height > 0)
				menu->titlebg =
					mystyle_make_pixmap_overlay (Scr.MSMenuTitle, menu->x + x_offset,
												 menu->y + title_y + y_offset, width, title_height,
												 None);
			if (item_height > 0)
			{
				if (menu->itembg == None && Scr.MSMenuItem->texture_type > 0)
					menu->itembg =
						mystyle_make_pixmap_overlay (Scr.MSMenuItem, menu->x + x_offset,
													 menu->y + item_y + y_offset, width,
													 item_height, None);
				if (menu->itemhibg == None && Scr.MSMenuHilite->texture_type > 0)
					menu->itemhibg =
						mystyle_make_pixmap_overlay (Scr.MSMenuHilite, menu->x + x_offset,
													 menu->y + item_y + y_offset, width,
													 item_height, None);
			}
		}
	}
#endif /* !NO_TEXTURE */
	XSetWindowBackground (dpy, (*menu).w, (*Scr.MSMenuItem).colors.back);
}

/***********************************************************************
 *
 *  Procedure:
 *	RelieveRectangle - add relief lines to a rectangular window
 *
 ***********************************************************************/
void
RelieveRectangle (Window win, int x, int y, int w, int h, GC Hilite, GC Shadow)
{
	XDrawLine (dpy, win, Hilite, x, y, w + x - 1, y);

	XDrawLine (dpy, win, Shadow, x, h + y - 1, w + x - 1, h + y - 1);
	XDrawLine (dpy, win, Shadow, w + x - 1, y, w + x - 1, h + y - 1);

	XDrawLine (dpy, win, Hilite, x, y, x, h + y - 1);
}

/***********************************************************************
 *
 *  Procedure:
 *	RelieveHalfRectangle - add relief lines to the sides only of a
 *      rectangular window
 *
 ***********************************************************************/

void
RelieveHalfRectangle (Window win, int x, int y, int w, int h, GC Hilite, GC Shadow)
{

	XDrawLine (dpy, win, Hilite, x, y - 1, x, h + y);

/* ßß */ XDrawLine (dpy, win, Hilite, x + 1, y, x + 1, h + y - 1);

	XDrawLine (dpy, win, Shadow, w + x - 1, y - 1, w + x - 1, h + y);
	XDrawLine (dpy, win, Shadow, w + x - 2, y, w + x - 2, h + y - 1);
}

/***********************************************************************
 *
 *  Procedure:
 *      PaintEntry - draws a single entry in a poped up menu
 *
 ***********************************************************************/

void
PaintEntry (MenuRoot * mr, MenuItem * mi)
{
	/*
	 * get texture from
	 * (0, bg_y_offset, width, y_height)
	 *
	 * put texture (or color) at
	 * (x_offset, y_offset, width, y_height)
	 */
	int           x_offset, y_offset, width, y_height;
	int           text_y;
	Bool          is_selected;
	MyStyle      *style;
	GC            foreGC;
	GC            shadowGC;
	GC            reliefGC;
	GC            backGC;

#ifndef NO_TEXTURE
	int           bg_y_offset;
	Pixmap        background = None;
#endif

	is_selected = (mi->is_hilited && mi->func != F_TITLE && mi->strlen > 0 &&
				   check_allowed_function (mi)) ? True : False;

/* setup our colors */
	if (mi->func == F_TITLE)
		style = Scr.MSMenuTitle;
	else if (is_selected == True)
		style = Scr.MSMenuHilite;
	else if (check_allowed_function (mi))
		style = Scr.MSMenuItem;
	else
		style = Scr.MSMenuStipple;

/* set the GCs */
	mystyle_get_global_gcs (style, &foreGC, &backGC, &reliefGC, &shadowGC);

	width = mr->width;
	y_height = mi->y_height;
	x_offset = 0;
	y_offset = mi->y_offset;

	if (DrawMenuBorders == 1)
	{
		width -= 2;
		y_height -= 3;
		x_offset += 1;
		y_offset += 1;
	}
	if (DrawMenuBorders == 2 || DrawMenuBorders == 3)
	{
		width -= 2;
		x_offset += 1;
	}
#ifndef NO_TEXTURE
	bg_y_offset = 0;
	/* find largest span of items (including this one) which are non-titles */
	if ((TextureMenuItemsIndividually == 0 || style->texture_type >= TEXTURE_TRANSPARENT) &&
		mi->func != F_TITLE)
	{
		MenuItem     *i;

		for (i = mi; (*i).prev != NULL && (*(*i).prev).func != F_TITLE; i = (*i).prev);
		bg_y_offset = (*mi).y_offset - (*i).y_offset;
	}
#endif

#ifndef NO_TEXTURE
/* setup our textures */
	if (mi->func == F_TITLE)
		background = mr->titlebg;
	else if (is_selected == True)
		background = mr->itemhibg;
	else
		background = mr->itembg;
#endif

/* draw the background */
#ifndef NO_TEXTURE
	if (background != None)
	{
		XCopyArea (dpy, background, mr->w, backGC,
				   0, bg_y_offset, width, y_height, x_offset, y_offset);
	} else if (style->texture_type >= TEXTURE_TRANSPARENT)
	{
		XSetWindowBackgroundPixmap (dpy, mr->w, None);
		XClearArea (dpy, mr->w, x_offset, y_offset, width, y_height, False);
	} else
#endif
	{
		XSetWindowBackground (dpy, mr->w, (*style).colors.back);
		XClearArea (dpy, mr->w, x_offset, y_offset, width, y_height, False);
	}

/* draw the menu borders (the relief pattern) :
 * DrawMenuBorders == 0 : no border
 * DrawMenuBorders == 1 : border around each item
 * DrawMenuBorders == 2 : title border, border around all items
 * DrawMenuBorders == 3 : title border, border around all items, border around hilited item
 */

	if (DrawMenuBorders == 1)
	{
		RelieveRectangle (mr->w, -1, mi->y_offset, mr->width + 1, mi->y_height, reliefGC, shadowGC);
		RelieveRectangle (mr->w, 0, mi->y_offset, mr->width, mi->y_height - 1, reliefGC, shadowGC);
	} else if (DrawMenuBorders == 2 || DrawMenuBorders == 3)
	{
		if (mi->func == F_TITLE)
			RelieveRectangle (mr->w, 0, mi->y_offset, mr->width, mi->y_height, reliefGC, shadowGC);
		else
		{
			MenuItem     *i;
			XRectangle    rect;
			int           y = mi->y_offset;
			int           h = mi->y_height;

			rect.x = 0;
			rect.width = mr->width;
			rect.y = mi->y_offset;
			rect.height = mi->y_height;
			XSetClipRectangles (dpy, reliefGC, 0, 0, &rect, 1, YXSorted);
			XSetClipRectangles (dpy, shadowGC, 0, 0, &rect, 1, YXSorted);
			for (i = mi->prev; i != NULL && (*i).func != F_TITLE; i = (*i).prev)
			{
				y = i->y_offset;
				h += i->y_height;
			}
			for (i = mi->next; i != NULL && (*i).func != F_TITLE; i = (*i).next)
				h += i->y_height;
			RelieveRectangle (mr->w, 0, y, mr->width, h, reliefGC, shadowGC);
			XSetClipMask (dpy, reliefGC, None);
			XSetClipMask (dpy, shadowGC, None);
		}
	}
	if (DrawMenuBorders == 3 && is_selected)
	{
		RelieveRectangle (mr->w, 1, mi->y_offset, mr->width - 2, mi->y_height, reliefGC, shadowGC);
	}
/* draw the icon */
	if ((*mi).icon.pix != None)
		draw_icon ((*mr).w, &(*mi).icon, foreGC, 5, y_offset + (y_height - (*mi).icon.height) / 2);

/* draw a separator */
	if (mi->func == F_NOP && mi->strlen == 0 && DrawMenuBorders != 1)
		DrawSeparator (mr->w, reliefGC, shadowGC,
					   x_offset, y_offset, x_offset + width, y_offset, 0);

/* draw the popup indicator */
	if (mi->func == F_POPUP)
	{
#ifndef NO_TEXTURE
		if (Scr.MenuArrow.pix == None)
#endif /* !NO_TEXTURE */
		{
			int           d = (y_height - 7) / 2;

			DrawTrianglePattern (mr->w, shadowGC, reliefGC, shadowGC, mr->width - 13,
								 y_offset + d - 1, mr->width - 6, y_offset + d + 7);
		}
#ifndef NO_TEXTURE
		else
		{
			draw_icon ((*mr).w, &Scr.MenuArrow, foreGC,
					   x_offset + width - Scr.MenuArrow.width,
					   y_offset + (y_height - Scr.MenuArrow.height) / 2);
		}
#endif /* !NO_TEXTURE */
	}
#ifndef NO_TEXTURE
/* draw the pin state indicator */
	if (mi->func == F_TITLE)
	{
		if ((mr->is_pinned == True) && (Scr.MenuPinOn.pix != None))
			draw_icon ((*mr).w, &Scr.MenuPinOn, foreGC,
					   x_offset + width - Scr.MenuPinOn.width,
					   y_offset + (y_height - Scr.MenuPinOn.height) / 2);
		if ((mr->is_pinned == False) && (Scr.MenuPinOff.pix != None))
			draw_icon ((*mr).w, &Scr.MenuPinOff, foreGC,
					   x_offset + width - Scr.MenuPinOff.width,
					   y_offset + (y_height - Scr.MenuPinOff.height) / 2);
	}
#endif /* !NO_TEXTURE */
/* draw the menu text */
	text_y = y_offset + (y_height - (*style).font.height) / 2 + (*style).font.y;

	if (mi->strlen > 0)
		mystyle_draw_text (mr->w, style, mi->item, mi->x, text_y);
	if (mi->strlen2 > 0)
		mystyle_draw_text (mr->w, style, mi->item2, mi->x2, text_y);
	if (mi->hotkey != 0)
	{
		int           x;
		char          hk[2];

		hk[0] = mi->hotkey;
		hk[1] = '\0';
		x = mr->width - 6 - XTextWidth ((*style).font.font, hk, 1);
		if (mi->func == F_POPUP)
			x -= 15;
		mystyle_draw_text (mr->w, style, hk, x, text_y);
	}
}

/****************************************************************************
 *
 * draw_icon - draw a MyIcon
 *
 ****************************************************************************/

void
draw_icon (Window w, MyIcon * icon, GC iconGC, int x, int y)
{
	if ((*icon).mask != None)
	{
		Globalgcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
		Globalgcv.clip_mask = (*icon).mask;
		Globalgcv.clip_x_origin = x;
		Globalgcv.clip_y_origin = y;
		XChangeGC (dpy, iconGC, Globalgcm, &Globalgcv);
	}
	XCopyArea (dpy, (*icon).pix, w, iconGC, 0, 0, (*icon).width, (*icon).height, x, y);
	if ((*icon).mask != None)
	{
		Globalgcv.clip_mask = None;
		XChangeGC (dpy, iconGC, Globalgcm, &Globalgcv);
	}
}

/****************************************************************************
 * Procedure:
 *	DrawUnderline() - Underline a character in a string (pete@tecc.co.uk)
 *
 * Calculate the pixel offsets to the start of the character position we
 * want to underline and to the next character in the string.  Shrink by
 * one pixel from each end and the draw a line that long two pixels below
 * the character...
 *
 ****************************************************************************/

void
DrawUnderline (MyStyle * style, Window w, int x, int y, char *txt, int posn, int n)
{
	int           off1 = XTextWidth (style->font.font, txt, posn);
	int           off2 = XTextWidth (style->font.font, txt, posn + n) - 1;
	GC            drawGC;

	mystyle_get_global_gcs (style, &drawGC, NULL, NULL, NULL);

	XDrawLine (dpy, w, drawGC, x + off1, y + 2, x + off2, y + 2);
}

/****************************************************************************
 *
 *  Draws two horizontal lines to form a separator
 *
 ****************************************************************************/

void
DrawSeparator (Window w, GC TopGC, GC BottomGC, int x1, int y1, int x2, int y2, int extra_off)
{
	XDrawLine (dpy, w, TopGC, x1, y1, x2, y2);
	XDrawLine (dpy, w, BottomGC, x1 - extra_off, y1 + 1, x2 + extra_off, y2 + 1);
}

/****************************************************************************
 *
 *  Draws a little Triangle pattern within a window
 *
 ****************************************************************************/


void
DrawTrianglePattern (Window w, GC GC1, GC GC2, GC GC3, int l, int u, int r, int b)
{
	int           m;

	m = (u + b) / 2;

	XDrawLine (dpy, w, GC1, l, u, l, b);

	XDrawLine (dpy, w, GC2, l, b, r, m);
	XDrawLine (dpy, w, GC3, r, m, l, u);
}

/***********************************************************************
 *
 *  Procedure:
 *	PaintMenu - draws the entire menu
 *
 ***********************************************************************/

void
PaintMenu (MenuRoot * mr, XEvent * e)
{
	MenuItem     *mi;

	for (mi = mr->first; mi != NULL; mi = mi->next)
	{
		/* be smart about handling the expose, redraw only the entries
		 * that we need to
		 */
		if (e->xexpose.y < (mi->y_offset + mi->y_height) &&
			(e->xexpose.y + e->xexpose.height) > mi->y_offset)
		{
			PaintEntry (mr, mi);
		}
	}
	XSync (dpy, 0);
	return;
}

/***********************************************************************
 *
 *  finds the entry which the pointer is over
 *
 ***********************************************************************/
MenuItem     *
find_entry (MenuRoot * menu, int x, int y)
{
	MenuItem     *mi;

	/*int x, y;
	   XQueryPointer (dpy, (*menu).w, &JunkRoot, &JunkChild,
	   &JunkX, &JunkY, &x, &y, &JunkMask);
	 */

	/* look for the entry that the mouse is in */
	for (mi = (*menu).first; mi != NULL; mi = mi->next)
		if (y >= mi->y_offset && y < mi->y_offset + mi->y_height)
			break;
	if ((x < 0) || (x >= (*menu).width))
		mi = NULL;

	return mi;
}

/***********************************************************************
 *
 *  Procedure:
 *	Updates menu display to reflect the highlighted item
 *
 ***********************************************************************/

int
FindEntry (MenuRoot * menu, XEvent * event)
{
	MenuItem     *old_item;
	MenuItem     *new_item;
	int           retval = MENU_NOP;

/*  int x, y; */

	/* find the hilited menu item (if any) */
	for (old_item = (*menu).first; old_item != NULL; old_item = old_item->next)
		if ((*old_item).is_hilited == True)
			break;

	new_item = find_entry (menu, event->xmotion.x, event->xmotion.y);

	/* shouldn't need to do another XQueryPointer after find_entry */
/*  XQueryPointer (dpy, (*menu).w, &JunkRoot, &JunkChild,
		 &JunkX, &JunkY, &x, &y, &JunkMask); */

	/* if we're not on the same item, change hilites */
/*fprintf( stderr, __FUNCTION__ "%d: new = %p, old = %p\n", __LINE__, new_item, old_item );*/
	if (new_item != old_item)
	{
		if (old_item != NULL)
			unselect_item (menu, old_item);
		if (new_item != NULL)
			select_item (menu, new_item);
	}
	if ((new_item != NULL) &&
		(new_item->func == F_POPUP) &&
		(menu->is_pinned == False) &&
		(event->xmotion.x > (menu->width * 3 / 4)) && new_item->menu != NULL)
	{
		/* create a new sub-menu */
		if ((new_item->menu->is_mapped == False) || (new_item->menu->is_pinned == True))
		{
			configure_menu (new_item->menu, menu);
			if (new_item->menu->is_mapped == False)
				map_menu (new_item->menu, (*menu).context);
			if (new_item->menu->is_pinned == True)
				unpin_menu (new_item->menu);
		}
	}
	return retval;
}

/***********************************************************************
 * Procedure
 * 	menuShortcuts() - Menu keyboard processing (pete@tecc.co.uk)
 *
 * Function called from UpdateMenu instead of Keyboard_Shortcuts()
 * when a KeyPress event is received.  If the key is alphanumeric,
 * then the menu is scanned for a matching hot key.  Otherwise if
 * it was the escape key then the menu processing is aborted.
 * If none of these conditions are true, then the default processing
 * routine is called.
 ***********************************************************************/
void
menuShortcuts (MenuRoot * menu, XEvent * ev)
{
	MenuItem     *mi;
	KeySym        keysym = XLookupKeysym (&ev->xkey, 0);

	if (menu_has_pointer)
		menu_mode = MCLICK;

	/* Try to match hot keys */
	if ((((keysym >= XK_a) && (keysym <= XK_z)) ||	/* Only consider alphabetic */
		 ((keysym >= XK_A) && (keysym <= XK_Z)) ||	/* Only consider alphabetic */
		 ((keysym >= XK_0) && (keysym <= XK_9))) && menu->first)
	{										   /* ...or numeric keys     */
		if (islower (keysym))
			keysym = toupper (keysym);
		/* Search menu for matching hotkey */
		for (mi = menu->first->next; mi; mi = mi->next)
			if (keysym == mi->hotkey)
			{
				if (menu_has_pointer == True)
					unmap_menu (pointer_menu);
				do_menu_function (menu, mi, ev);
				return;
			}
	}
	switch (keysym)
	{										   /* Other special keyboard handling */
	 case XK_Escape:						   /* Escape key pressed. Abort */
		 unmap_menu (menu);
		 break;

		 /* Nothing special --- Allow other shortcuts (cursor movement)    */
	 default:
		 {
			 int           x, y;

			 XQueryPointer (dpy, menu->w, &JunkRoot, &JunkChild, &x, &y, &JunkX, &JunkY, &JunkMask);
			 mi = find_entry (menu, x, y);
			 if (mi == NULL)
				 mi = menu->first;
			 if (mi)
				 Keyboard_shortcuts (ev, ButtonRelease, mi->y_height);
		 }
		 break;
	}
}

/***********************************************************************
 *
 *  interactively moves a menu
 *  closes the menu if the menu is not moved
 *  buttonx and buttony are in local coords to the menu window
 *  returns whether or not menu was actually moved
 *
 ***********************************************************************/
Bool
do_menu_move (MenuRoot * menu, int buttonx, int buttony)
{
	ASWindow      win;
	int           x, y;
	Bool          was_moved;

	/* fake an ASWindow, so moveLoop will be happy */
	win.frame = (*menu).w;
	win.w = None;
	win.bw = 0;
	win.flags = 0;
	win.frame_x = (*menu).x;
	win.frame_y = (*menu).y;
	win.frame_width = (*menu).width;
	win.frame_height = (*menu).height;
	XTranslateCoordinates (dpy, (*menu).w, Scr.Root, buttonx, buttony, &x, &y, &JunkChild);
	x = (*menu).x - x;
	y = (*menu).y - y;
	/* let our grab go, so we can change the grab cursor */
	if (menu_has_pointer == True)
		UngrabEm ();
	GrabEm (MOVE);
	XRaiseWindow (dpy, (*menu).w);

	moveLoop (&win, x, y, (*menu).width, (*menu).height, &x, &y, True, False);
	/* grab the pointer back */
	UngrabEm ();

	if (menu_has_pointer == True)
		GrabEm (MENU);

	was_moved = (((*menu).x != x) || ((*menu).y != y));

	if (was_moved == True)
		move_menu (menu, x, y);

	return was_moved;
}

/***********************************************************************
 *
 *  Procedure:
 *	Handles an X event in a menu
 *
 *  if menu == NULL, attempts to infer the menu from the event
 *
 *  Returns:
 *      False if the event was not handled
 *      True if the event was handled
 *
 ***********************************************************************/


Bool
HandleMenuEvent (MenuRoot * menu, XEvent * event)
{
	Bool          done = False;
	int           x, y;

	/* try to figure out where the event came from */
	if (menu == NULL)
		for (menu = Scr.first_menu; menu != NULL; menu = (*menu).next)
			if ((*event).xany.window == (*menu).w)
				break;

	/* if there is a pointer grab and we got an event not in our window,
	 * report to the window that did the grab; this allows us to catch
	 * button events outside the menu window
	 */
	if ((menu == NULL) && (menu_has_pointer == True) &&
		(((*event).type == ButtonPress) || ((*event).type == ButtonRelease)))
		menu = pointer_menu;

	/* if there is a pointer grab, and we get an EnterNotify or LeaveNotify
	 * for some other window, then steal it so we won't lose our grab
	 */
/*fprintf( stderr, "%s:%d Received event %d  for menu %p\n", __FUNCTION__, __LINE__, event->type, menu );*/

	if ((menu == NULL) && (menu_has_pointer == True) &&
		(((*event).type == EnterNotify) || ((*event).type == LeaveNotify)))
		return True;

	if (menu == NULL)
		return False;

/*fprintf( stderr, "%s:%d event menu for menu %p is %s\n", __FUNCTION__, __LINE__, menu, (*menu).is_mapped?"mapped":"unmapped" );*/
	/*
	 * ignore events in an unmapped menu
	 * these can come from menus which will soon be unmapped, but hasn't
	 * been yet
	 */
	if ((*menu).is_mapped == False)
		return True;

	/* discard extra motion events, keeping only the last one */
	if ((*event).type == MotionNotify)
	{
		while (XCheckTypedWindowEvent (dpy, (*menu).w, MotionNotify, event) == True);

		x = event->xmotion.x_root;
		y = event->xmotion.y_root;
		if ((x >= (*menu).x) && (x < (*menu).x + (*menu).width))
		{
			if (y < 2)
			{
				if ((*menu).y < 0)
				{
					y = (*menu).y + ((*menu).first->y_height - 2);
					if (y > 0)
						y = 0;
					move_menu (menu, (*menu).x, y);
					FindEntry (menu, event);
					XSync (dpy, 0);
				}
			} else if (y > Scr.MyDisplayHeight - 3)
			{
				if ((*menu).y + (*menu).height > Scr.MyDisplayHeight - 2)
				{
					y = (*menu).y - ((*menu).first->y_height - 2);
					if (y + (*menu).height < Scr.MyDisplayHeight - 2)
						y = (Scr.MyDisplayHeight - 2) - (*menu).height;
					move_menu (menu, (*menu).x, y);
					FindEntry (menu, event);
					XSync (dpy, 0);
				}
			}
		}
	}
	if ((*event).type == KeyPress)
	{
		done = True;
		/* Handle a limited number of key press events to allow mouseless
		 * operation */
		menuShortcuts (menu, event);
	}
	/* check the time and change modes if necessary */
	if ((menu_has_pointer == True) && (menu_mode == MWAIT))
        if (Scr.last_Timestamp > Scr.menu_grab_Timestamp + Scr.ClickTime)
			menu_mode = MHOLD;

	switch ((*event).type)
	{
	 case ButtonPress:
		 done = HandleMenuButtonPress (menu, event);
		 break;

	 case ButtonRelease:
		 done = HandleMenuButtonRelease (menu, event);
		 break;

	 case EnterNotify:
		 done = HandleMenuEnterNotify (menu, event);
		 break;

	 case LeaveNotify:
		 done = HandleMenuLeaveNotify (menu, event);
		 break;

	 case MotionNotify:
		 done = HandleMenuMotionNotify (menu, event);
		 break;

	 case Expose:
		 done = HandleMenuExpose (menu, event);
		 break;
	}
	return done;
}

/*
 * if the menu is pinned,
 *   if in title,
 *     if on pin icon,
 *       unmap menu
 *     else
 *       move the menu
 *   else
 *     if on submenu,
 *       close open unpinned menus
 *     execute item
 * else if mode is MCLICK
 *   if in title,
 *   if in title,
 *     pin menu
 *     close open unpinned menus
 * else (if mode is MWAIT or MHOLD)
 *   do nothing (this shouldn't happen...)
 */
Bool
HandleMenuButtonPress (MenuRoot * menu, XEvent * event)
{
	MenuItem     *item;

	item = find_entry (menu, event->xbutton.x, event->xbutton.y);
	if ((*menu).is_pinned == True)
	{
		if ((item != NULL) && ((*item).func == F_TITLE))
		{
#ifndef NO_TEXTURE
			if ((*event).xbutton.x > (*menu).width - Scr.MenuPinOn.width)
			{
				unmap_menu (menu);
			} else
#endif /* !NO_TEXTURE */
			{
				do_menu_move (menu, (*event).xbutton.x, (*event).xbutton.y);
			}
		} else if (item != NULL)
		{
			if ((item->func == F_POPUP) && (menu_has_pointer == True))
				unmap_menu (pointer_menu);
			do_menu_function (menu, item, event);
		}
	} else if (menu_mode == MCLICK)
	{
		if ((item != NULL) && ((*item).func == F_TITLE))
		{
			if (pin_menu (menu) == False)
				unmap_menu (menu);
			if (menu_has_pointer == True)
				unmap_menu (pointer_menu);
		}
	}
	return True;
}

/*
 * if the menu is not pinned,
 *   if mode is MWAIT
 *     change mode to MCLICK
 *   else if mode is MCLICK
 *     if in title,
 *       do nothing
 *     else if on item,
 *       close open unpinned menus
 *       execute item
 *     else (not in menu)
 *       close open unpinned menus
 *   else (if mode is MHOLD)
 *     if in title,
 *       pin menu
 *       close open unpinned menus
 *     else if on item,
 *       close open unpinned menus
 *       execute item
 *     else (not in menu)
 *       close open unpinned menus
 */
Bool
HandleMenuButtonRelease (MenuRoot * menu, XEvent * event)
{
	MenuItem     *item;

	item = find_entry (menu, event->xbutton.x, event->xbutton.y);
	if ((*menu).is_pinned == False)
	{
		if (menu_mode == MWAIT)
			menu_mode = MCLICK;
		else if (menu_mode == MCLICK)
		{
			if (item == NULL)
			{
				if (menu_has_pointer == True)
					unmap_menu (pointer_menu);
			} else if ((*item).func != F_TITLE)
			{
				if (menu_has_pointer == True)
					unmap_menu (pointer_menu);
				do_menu_function (menu, item, event);
			}
		} else
		{
			if (item == NULL)
			{
				if (menu_has_pointer == True)
					unmap_menu (pointer_menu);
			} else if ((*item).func == F_TITLE)
			{
				if (pin_menu (menu) == False)
					unmap_menu (menu);
				if (menu_has_pointer == True)
					unmap_menu (pointer_menu);
			} else
			{
				if (menu_has_pointer == True)
					unmap_menu (pointer_menu);
				do_menu_function (menu, item, event);
			}
		}
	}
	return True;
}

Bool
HandleMenuEnterNotify (MenuRoot * menu, XEvent * event)
{
	return True;
}

/*
 * if the menu is pinned,
 *   deselect all items
 * else (menu is unpinned)
 *   deselect all items which are not mapped, unpinned submenus
 */
Bool
HandleMenuLeaveNotify (MenuRoot * menu, XEvent * event)
{
	MenuItem     *item;

/*fprintf( stderr, __FUNCTION__ ":%d crossing_mode = %d ( Normal = %d )\n", __LINE__, (*event).xcrossing.mode, NotifyNormal );*/
	if ((*event).xcrossing.mode != NotifyNormal)
		return True;
	if ((*menu).is_pinned == True)
	{
		for (item = (*menu).first; item != NULL; item = (*item).next)
			if ((*item).is_hilited == True)
				unselect_item (menu, item);
	} else
	{
		for (item = (*menu).first; item != NULL; item = (*item).next)
			if (((*item).is_hilited == True) &&
				!(((*item).menu != NULL) &&
				  ((*(*item).menu).is_mapped == True) && ((*(*item).menu).is_pinned == False)))
			{
/*fprintf( stderr, __FUNCTION__ ":%d unselecting\n", __LINE__ );*/
				unselect_item (menu, item);
			}
	}
	return True;
}

Bool
HandleMenuMotionNotify (MenuRoot * menu, XEvent * event)
{
	FindEntry (menu, event);
	return True;
}


Bool
HandleMenuExpose (MenuRoot * menu, XEvent * event)
{
	PaintMenu (menu, event);
	return True;
}

void
do_menu_function (MenuRoot * menu, MenuItem * item, XEvent * event)
{
	if ((*menu).context & (C_WINDOW | C_FRAME | C_TITLE | C_SIDEBAR | C_LALL | C_RALL))
		menuFromFrameOrWindowOrTitlebar = TRUE;
	if ((item->func != F_NOP) && check_allowed_function (item))
	{
		ASWindow     *aswin = (*menu).aw;

		if (aswin != NULL)
		{
			ExecuteFunction (item->func, item->action,
							 aswin->frame,
							 aswin, event, Context,
							 item->val1, item->val2,
							 item->val1_unit, item->val2_unit, item->menu, -1);
		} else
		{
			ExecuteFunction (item->func, item->action,
							 None, None, event,
							 Context, item->val1,
							 item->val2, item->val1_unit, item->val2_unit, item->menu, -1);
		}
	}
	menuFromFrameOrWindowOrTitlebar = FALSE;
}

/*
 * move a menu window
 */
void
move_menu (MenuRoot * menu, int x, int y)
{
	(*menu).x = x;
	(*menu).y = y;
	XMoveWindow (dpy, (*menu).w, (*menu).x, (*menu).y);
	XRaiseWindow (dpy, (*menu).w);
	if (Scr.MSMenuTitle->texture_type >= TEXTURE_TRANSPARENT ||
		Scr.MSMenuItem->texture_type >= TEXTURE_TRANSPARENT ||
		Scr.MSMenuHilite->texture_type >= TEXTURE_TRANSPARENT)
	{
		MenuItem     *mi;

		setup_menu_pixmaps (menu);
		for (mi = menu->first; mi != NULL; mi = mi->next)
		{
			if (mi->func == F_TITLE && Scr.MSMenuTitle->texture_type >= TEXTURE_TRANSPARENT)
				PaintEntry (menu, mi);
			else if (mi->is_hilited && Scr.MSMenuHilite->texture_type >= TEXTURE_TRANSPARENT)
				PaintEntry (menu, mi);
			else if (Scr.MSMenuItem->texture_type >= TEXTURE_TRANSPARENT)
				PaintEntry (menu, mi);
		}
	}
}

/*
 * select a menu item
 */
void
select_item (MenuRoot * menu, MenuItem * item)
{
	(*item).is_hilited = True;
	if ((*menu).is_mapped == True)
		if ((*item).func != F_TITLE)
			PaintEntry (menu, item);
}



/*
 * unselect a menu item
 *
 * an unselected item promises:
 *   all unpinned submenus of this item are unmapped
 */

void
unselect_item (MenuRoot * menu, MenuItem * item)
{
	(*item).is_hilited = False;
	if ((*menu).is_mapped == True)
		if ((*item).func != F_TITLE)
			PaintEntry (menu, item);

	if ((*menu).is_pinned == False)
	{
		MenuRoot     *sub = (*item).menu;

		if ((sub != NULL) && ((*sub).is_mapped == True))
			if ((*sub).is_pinned == False)
				unmap_menu (sub);
	}
}

/*
 * unpin a menu
 *
 * an mapped, unpinned menu promises there is a pointer grab
 */
Bool
unpin_menu (MenuRoot * menu)
{
	Bool          success = False;

	if ((*menu).is_mapped == True)
	{
		MenuItem     *item;

		success = True;
		if (menu_has_pointer == False)
			if ((success = GrabEm (MENU)) == True)
			{
				menu_has_pointer = True;
				pointer_menu = menu;
				menu_mode = MWAIT;
                Scr.menu_grab_Timestamp = Scr.last_Timestamp;
				InstallRootColormap ();
			}
		(*menu).is_pinned = !success;
		for (item = (*menu).first; item != NULL; item = (*item).next)
			if ((*item).func == F_TITLE)
				PaintEntry (menu, item);
	}
	return success;
}



/*
 * pin a menu
 *
 * a mapped, pinned menu promises:
 *   this menu is not transient
 *   this menu has not grabbed the pointer
 *   this menu has no mapped, unpinned submenus
 */

Bool
pin_menu (MenuRoot * menu)
{
	if (menu == pointer_menu)
	{
		UngrabEm ();
		menu_has_pointer = False;
		pointer_menu = NULL;
		UninstallRootColormap ();
	}
	if ((*menu).is_transient == True)
		return False;
	if ((*menu).is_mapped == False)
		return False;
	if ((*menu).is_pinned == False)
	{
		MenuItem     *item;

		(*menu).is_pinned = True;
		for (item = (*menu).first; item != NULL; item = (*item).next)
			if ((*item).func == F_TITLE)
				PaintEntry (menu, item);
		for (item = (*menu).first; item != NULL; item = (*item).next)
			if (((*item).menu != NULL) &&
				((*(*item).menu).is_mapped == True) && ((*(*item).menu).is_pinned == False))
				unmap_menu ((*item).menu);
	}
	return True;
}

/***********************************************************************
 *
 *  Procedure:
 *	map_menu - map a pull down menu
 *
 *  Inputs:
 *	menu	- the root pointer of the menu to pop up
 *      context - the context (C_WINDOW, C_FRAME, ...)
 *
 ***********************************************************************/

Bool
map_menu (MenuRoot * menu, int context)
{
	XMoveResizeWindow (dpy, (*menu).w, (*menu).x, (*menu).y, (*menu).width, (*menu).height);
	XSelectInput (dpy, menu->w, EnterWindowMask | LeaveWindowMask |
				  KeyPressMask | KeyReleaseMask |
				  ButtonPressMask | ButtonReleaseMask |
				  ButtonMotionMask | PointerMotionMask | ExposureMask);
	XMapRaised (dpy, (*menu).w);
	menu->is_mapped = True;

	if ((unpin_menu (menu) != True) && (pin_menu (menu) != True))
	{
		unmap_menu (menu);
		return False;
	}
	XSetInputFocus (dpy, (*menu).w, RevertToParent, CurrentTime);
	(*menu).context = context;

	return True;
}


/***********************************************************************
 *
 *  Procedure:
 *	unmap_menu - unhighlight the current menu selection and
 *		take down the open, unpinned submenus
 *
 ***********************************************************************/

Bool
unmap_menu (MenuRoot * menu)
{
	MenuItem     *item;

/*fprintf( stderr, __FUNCTION__":%d menu %p\n", __LINE__, menu );*/
	/* ßß caused window menus to fail; should be here, though! the problem
	 * is the timing and side-effects of mapping/unmapping menus */
	/* (*menu).aw = NULL; */

	/* attempt to pin the menu, which removes any pointer grab */
	if (pointer_menu == menu)
		pin_menu (menu);

	/* close all mapped, unpinned children of this menu */
	for (item = (*menu).first; item != NULL; item = (*item).next)
		if (((*item).menu != NULL) &&
			((*(*item).menu).is_mapped == True) && ((*(*item).menu).is_pinned == False))
			unmap_menu ((*item).menu);

	/* turn off all items */
	for (item = (*menu).first; item != NULL; item = (*item).next)
		item->is_hilited = 0;

	XUnmapWindow (dpy, (*menu).w);

	(*menu).is_mapped = False;

	if ((*menu).is_transient == True)
	{
		DeleteMenuRoot (menu);
	}
	return True;
}


/***************************************************************************
 *
 * Wait for all mouse buttons to be released
 * This can ease some confusion on the part of the user sometimes
 *
 * Discard superflous button events during this wait period.
 *
 ***************************************************************************/

void
WaitForButtonsUp ()
{
	Bool          AllUp = False;
	XEvent        JunkEvent;
	unsigned int  mask;

	while (!AllUp)
	{
		XAllowEvents (dpy, ReplayPointer, CurrentTime);
		XQueryPointer (dpy, Scr.Root, &JunkRoot, &JunkChild, &JunkX, &JunkY, &JunkX, &JunkY, &mask);

		if ((mask & (Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask)) == 0)
			AllUp = True;
	}
	XSync (dpy, 0);
    while (ASCheckMaskEvent (ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, &JunkEvent))
	{
		XAllowEvents (dpy, ReplayPointer, CurrentTime);
	}

}
