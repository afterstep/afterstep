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

#include "../../configure.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"

void
get_window_geometry (ASWindow * t, int flags, int *x, int *y, int *w, int *h)
{
	int           tx = -999, ty = -999, tw = 0, th = 0;

	if (flags & ICONIFIED)
	{
		if (t->icon_pixmap_w != None)
		{
			tw = t->icon_p_width;
			th = t->icon_p_height;
			if ((Scr.flags & IconTitle) && ASWIN_HFLAGS(t, AS_IconTitle) &&
				(Textures.flags & SeparateButtonTitle))
				th += t->icon_t_height;
			tx = t->icon_p_x;
			ty = t->icon_p_y;
		} else if (t->icon_title_w != None)
		{
			tw = t->icon_t_width;
			th = t->icon_t_height;
			tx = t->icon_p_x;
			ty = t->icon_p_y;
		} else
		{
			tw = 0;
			th = 0;
			tx = t->frame_x;
			ty = t->frame_y;
		}
	} else if (flags & SHADED)
	{
		tw = t->title_width + 2 * t->boundary_width;
		th = t->title_height + 2 * t->boundary_width;
		tx = t->frame_x;
		ty = t->frame_y;
	} else
	{
		tw = t->frame_width;
		th = t->frame_height;
		tx = t->frame_x;
		ty = t->frame_y;
	}
	if (x != NULL)
		*x = tx;
	if (y != NULL)
		*y = ty;
	if (w != NULL)
		*w = tw;
	if (h != NULL)
		*h = th;
}

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

void place_aswindow( ASWindow *t, ASStatusHints *status )
{
#warning "Implement proper manuall placing of windows"  
	return ;
}

#if 0

/* Used to parse command line of clients for specific desk requests. */
/* Todo: check for multiple desks. */
static XrmDatabase db;
static XrmOptionDescRec table[] = {
	/* Want to accept "-workspace N" or -xrm "afterstep*desk:N" as options
	 * to specify the desktop. I have to include dummy options that
	 * are meaningless since Xrm seems to allow -w to match -workspace
	 * if there would be no ambiguity. */
	{"-workspacf", "*junk", XrmoptionSepArg, (caddr_t) NULL},
	{"-workspace", "*desk", XrmoptionSepArg, (caddr_t) NULL},
	{"-xrn", NULL, XrmoptionResArg, (caddr_t) NULL},
	{"-xrm", NULL, XrmoptionResArg, (caddr_t) NULL},
};

/**************************************************************************
 * figure out where a window belongs, in this order:
 * 1. if sticky, use current desk
 * 2. _XA_WIN_DESK, if set
 * 3. if the client requested a specific desk on the command line, use that
 * 4. if part of a window group, same desk as another member of the group
 * 5. if a transient, same desk as parent
 * 6. use StartsOnDesk from database, if set
 * 7. the current desk
 */
int
InvestigateWindowDesk (ASWindow * tmp_win)
{
	name_list     nl;

	style_init (&nl);
	style_fill_by_name (&nl, &(tmp_win->hints->names[0]));

	/* sticky windows always use the current desk */
	if (nl.off_flags & STICKY_FLAG)
		return Scr.CurrentDesk;

	/* determine desk property, if set */
	{
		Atom          atype;
		int           aformat;
		unsigned long nitems, bytes_remain;
		unsigned char *prop;
		extern Atom   _XA_WIN_DESK;

		if (XGetWindowProperty (dpy, tmp_win->w, _XA_WIN_DESK, 0L, 1L, True,
								AnyPropertyType, &atype, &aformat, &nitems,
								&bytes_remain, &prop) == Success && prop != NULL)
		{
			int           desk = *(unsigned long *)prop;

			XFree (prop);
			return desk;
		}
	}

	/* Find out if the client requested a specific desk on the command line. */
	{
		int           client_argc = 0;
		char        **client_argv = NULL, *str_type;
		XrmValue      rm_value;
		int           desk = 0;
		Bool          status;

		if (XGetCommand (dpy, tmp_win->w, &client_argv, &client_argc))
		{
			XrmParseCommand (&db, table, 4, "afterstep", &client_argc, client_argv);
			XFreeStringList (client_argv);
			status = XrmGetResource (db, "afterstep.desk", "AS.Desk", &str_type, &rm_value);
			if (status == True && rm_value.size)
				desk = strtol (rm_value.addr, NULL, 0);
			else
				status = False;
			XrmDestroyDatabase (db);
			db = NULL;
			if (status == True)
				return desk;
		}
	}

	/* Try to find the group leader or another window in the group */
	if ((tmp_win->wmhints) && (tmp_win->wmhints->flags & WindowGroupHint) &&
		(tmp_win->wmhints->window_group != None) && (tmp_win->wmhints->window_group != Scr.Root))
	{
		ASWindow     *t;

		for (t = Scr.ASRoot.next; t != NULL; t = t->next)
		{
			if ((t->w == tmp_win->wmhints->window_group) ||
				((t->wmhints) && (t->wmhints->flags & WindowGroupHint) &&
				 (t->wmhints->window_group == tmp_win->wmhints->window_group)))
				return t->Desk;
		}
	}

	/* Try to find the parent's desktop */
 	if (ASWIN_HFLAGS(tmp_win, AS_Transient) && (tmp_win->hints->transient_for != None) &&
		(tmp_win->hints->transient_for != Scr.Root))
	{
		ASWindow     *t;

		for (t = Scr.ASRoot.next; t != NULL; t = t->next)
		{
			if (t->w == tmp_win->hints->transient_for)
				return t->Desk;
		}
	}

	/* use StartsOnDesk, if set */
	if (nl.off_flags & STAYSONDESK_FLAG)
		return nl.Desk;

	return Scr.CurrentDesk;
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
/************************************************************************
 *
 *  Procedure:
 *	GetGravityOffsets - map gravity to (x,y) offset signs for adding
 *		to x and y when window is mapped to get proper placement.
 * 
 ************************************************************************/
struct _gravity_offset
{
	int           x, y;
};

void
GetGravityOffsets (ASWindow * tmp, int *xp, int *yp)
{
	static struct _gravity_offset gravity_offsets[11] = {
		{0, 0},								   /* ForgetGravity */
		{-1, -1},							   /* NorthWestGravity */
		{0, -1},							   /* NorthGravity */
		{1, -1},							   /* NorthEastGravity */
		{-1, 0},							   /* WestGravity */
		{0, 0},								   /* CenterGravity */
		{1, 0},								   /* EastGravity */
		{-1, 1},							   /* SouthWestGravity */
		{0, 1},								   /* SouthGravity */
		{1, 1},								   /* SouthEastGravity */
		{0, 0},								   /* StaticGravity */
	};
	register int  g = tmp->hints->gravity;

	if (g < ForgetGravity || g > StaticGravity)
		*xp = *yp = 0;
	else
	{
		*xp = (int)gravity_offsets[g].x;
		*yp = (int)gravity_offsets[g].y;
	}
	return;
}
