/****************************************************************************
 * This module is based on Twm, but has been SIGNIFICANTLY modified
 * by Rob Nation
 * by Bo Yang
 * by Frank Fejes
 * by Eric Tremblay
 * Rafal Wierzbicki 1998 (rubber bands) <rafal@mcss.mcmaster.ca>
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

#include "../../configure.h"

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/string.h"
#include "../../include/functions.h"

static int    dragx;						   /* all these variables are used */
static int    dragy;						   /* in resize operations */
static int    dragWidth;
static int    dragHeight;

static int    origx;
static int    origy;
static int    origWidth;
static int    origHeight;

static int    ymotion, xmotion;
static int    last_width, last_height;
extern int    menuFromFrameOrWindowOrTitlebar;
extern Window PressedW;

/****************************************************************************
 *
 * Starts a window resize operation
 *
 ****************************************************************************/
void
resize_window (Window w, ASWindow * tmp_win, int val1, int val2, int val1_unit, int val2_unit)
{
	Bool          finished = FALSE, done = FALSE;
	int           x, y, delta_x, delta_y;
	Window        ResizeWindow;
	extern int    Stashed_X, Stashed_Y;
	Bool          flags;
	Bool          opaque_resize = False;

	if ((w == None) || (tmp_win == NULL))
		return;

	/* Already checked this in functions.c, but its here too incase
	 * there's a resize on initial placement. */
	if (check_allowed_function2 (F_RESIZE, tmp_win) == 0)
	{
		XBell (dpy, Scr.screen);
		return;
	}
	/* can't resize icons */
	if (tmp_win->flags & ICONIFIED)
		return;

	ResizeWindow = tmp_win->frame;

	if ((val1 != 0) && (val2 != 0))
	{
		dragWidth = val1 * val1_unit / 100;
		dragHeight = val2 * val2_unit / 100;

		ConstrainSize (tmp_win, &dragWidth, &dragHeight);
		SetupFrame (tmp_win, tmp_win->frame_x, tmp_win->frame_y, dragWidth, dragHeight, FALSE);

		ResizeWindow = None;
		return;
	}
	InstallRootColormap ();
	if (menuFromFrameOrWindowOrTitlebar)
	{
		/* warp the pointer to the cursor position from before menu appeared */
		XWarpPointer (dpy, None, Scr.Root, 0, 0, 0, 0, Stashed_X, Stashed_Y);
		XFlush (dpy);
	}
	if (!GrabEm (MOVE))
	{
		XBell (dpy, Scr.screen);
		return;
	}

	/* handle problems with edge-wrapping while resizing */
	flags = Scr.flags;
	Scr.flags &= ~(EdgeWrapX | EdgeWrapY);

	XGetGeometry (dpy, (Drawable) ResizeWindow, &JunkRoot,
				  &dragx, &dragy, (unsigned int *)&dragWidth,
				  (unsigned int *)&dragHeight, &JunkBW, &JunkDepth);

	if ((dragWidth * dragHeight) <
		((Scr.OpaqueResize * Scr.MyDisplayWidth * Scr.MyDisplayHeight) / 100))
		opaque_resize = True;

	if ((!opaque_resize) || !(tmp_win->flags & MAPPED))
		XGrabServer (dpy);

	dragx += tmp_win->bw;
	dragy += tmp_win->bw;
	origx = dragx;
	origy = dragy;
	origWidth = dragWidth;
	origHeight = dragHeight;
	ymotion = xmotion = 0;

	/* pop up a resize dimensions window */
	XMapRaised (dpy, Scr.SizeWindow);
	last_width = 0;
	last_height = 0;
	DisplaySize (tmp_win, origWidth, origHeight, True);

	/* Get the current position to determine which border to resize */
	if ((PressedW != Scr.Root) && (PressedW != None))
	{
		if (PressedW == tmp_win->side)		   /* bottom */
			ymotion = -1;
		if (PressedW == tmp_win->corners[0])
		{									   /* lower left */
			ymotion = -1;
			xmotion = 1;
		}
		if (PressedW == tmp_win->corners[1])
		{									   /* lower right */
			ymotion = -1;
			xmotion = -1;
		}
		if (PressedW == tmp_win->fw[FR_SW])
		{
			ymotion = -1;
			xmotion = 1;
		}
		if (PressedW == tmp_win->fw[FR_SE])
		{
			ymotion = -1;
			xmotion = -1;
		}
		if (PressedW == tmp_win->fw[FR_NE])
		{
			ymotion = 1;
			xmotion = -1;
		}
		if (PressedW == tmp_win->fw[FR_NW])
		{
			ymotion = 1;
			xmotion = 1;
		}
		if (PressedW == tmp_win->fw[FR_S])
			ymotion = -1;
		if (PressedW == tmp_win->fw[FR_N])
			ymotion = 1;
	}

	/* draw the rubber-band window */
	if ((!opaque_resize) || !(tmp_win->flags & MAPPED))
		MoveOutline ( /*Scr.Root, */ tmp_win, dragx - tmp_win->bw, dragy - tmp_win->bw,
					 dragWidth + 2 * tmp_win->bw,
					 dragHeight + 2 * tmp_win->bw);

	/* loop to resize */
	while (!finished)
	{
        ASMaskEvent (ButtonPressMask | ButtonReleaseMask | KeyPressMask |
                     ButtonMotionMask | PointerMotionMask | ExposureMask, &Event);

        if (Event.type == MotionNotify) /* discard any extra motion events before a release */
            while (ASCheckMaskEvent (ButtonMotionMask|ButtonReleaseMask|PointerMotionMask, &Event))
				if (Event.type == ButtonRelease)
					break;

		done = FALSE;
		/* Handle a limited number of key press events to allow mouseless
		 * operation */
		if (Event.type == KeyPress)
			Keyboard_shortcuts (&Event, ButtonRelease, 20);
		switch (Event.type)
		{
		 case ButtonPress:
			 XAllowEvents (dpy, ReplayPointer, CurrentTime);
		 case KeyPress:
			 done = TRUE;
			 break;

		 case ButtonRelease:
			 finished = TRUE;
			 done = TRUE;
			 break;

		 case MotionNotify:
			 x = Event.xmotion.x_root;
			 y = Event.xmotion.y_root;
			 /* need to move the viewport */
			 HandlePaging (tmp_win, Scr.EdgeScrollX, Scr.EdgeScrollY, &x, &y,
						   &delta_x, &delta_y, False);
			 origx -= delta_x;
			 origy -= delta_y;
			 dragx -= delta_x;
			 dragy -= delta_y;

			 DoResize (x, y, tmp_win, opaque_resize);
			 done = TRUE;
		 default:
			 break;
		}
		if (!done)
		{

			if ((!opaque_resize) || !(tmp_win->flags & MAPPED))
				MoveOutline ( /*Scr.Root, */ tmp_win, 0, 0, 0, 0);

			DispatchEvent ();

			if ((!opaque_resize) || !(tmp_win->flags & MAPPED))
				MoveOutline ( /*Scr.Root, */ tmp_win, dragx - tmp_win->bw, dragy - tmp_win->bw,
							 dragWidth + 2 * tmp_win->bw, dragHeight + 2 * tmp_win->bw);

		}
	}

	/* erase the rubber-band */
	if ((!opaque_resize) || !(tmp_win->flags & MAPPED))
		MoveOutline ( /*Scr.Root, */ tmp_win, 0, 0, 0, 0);

	/* pop down the size window */
	XUnmapWindow (dpy, Scr.SizeWindow);

	ConstrainSize (tmp_win, &dragWidth, &dragHeight);
	SetupFrame (tmp_win, dragx - tmp_win->bw, dragy - tmp_win->bw, dragWidth, dragHeight, FALSE);

	UninstallRootColormap ();
	ResizeWindow = None;
	XUngrabServer (dpy);
	UngrabEm ();
	Scr.flags |= flags & (EdgeWrapX | EdgeWrapY);
	UpdateVisibility ();
	return;
}


/***********************************************************************
 *
 *  Procedure:
 *      DoResize - move the rubberband around.  This is called for
 *                 each motion event when we are resizing
 *
 *  Inputs:
 *      x_root        - the X corrdinate in the root window
 *      y_root        - the Y corrdinate in the root window
 *      tmp_win       - the current afterstep window
 *	opaque_resize - should the resize be opaque
 *
 ************************************************************************/
void
DoResize (int x_root, int y_root, ASWindow * tmp_win, Bool opaque_resize)
{
	int           action = 0;

	if ((y_root <= origy) || ((ymotion == 1) && (y_root < origy + origHeight - 1)))
	{
		dragy = y_root;
		dragHeight = origy + origHeight - y_root;
		action = 1;
		ymotion = 1;
	} else if ((y_root >= origy + origHeight - 1) || ((ymotion == -1) && (y_root > origy)))
	{
		dragy = origy;
		dragHeight = 1 + y_root - dragy;
		action = 1;
		ymotion = -1;
	}
	if ((x_root <= origx) || ((xmotion == 1) && (x_root < origx + origWidth - 1)))
	{
		dragx = x_root;
		dragWidth = origx + origWidth - x_root;
		action = 1;
		xmotion = 1;
	}
	if ((x_root >= origx + origWidth - 1) || ((xmotion == -1) && (x_root > origx)))
	{
		dragx = origx;
		dragWidth = 1 + x_root - origx;
		action = 1;
		xmotion = -1;
	}
	if (action)
	{
		ConstrainSize (tmp_win, &dragWidth, &dragHeight);
		if (xmotion == 1)
			dragx = origx + origWidth - dragWidth;
		if (ymotion == 1)
			dragy = origy + origHeight - dragHeight;

		if ((!opaque_resize) || !(tmp_win->flags & MAPPED))
		{
			MoveOutline ( /*Scr.Root, */ tmp_win, dragx - tmp_win->bw, dragy - tmp_win->bw,
						 dragWidth + 2 * tmp_win->bw, dragHeight + 2 * tmp_win->bw);
		} else
		{
			SetupFrame (tmp_win, dragx - tmp_win->bw,
						dragy - tmp_win->bw, dragWidth, dragHeight, FALSE);
		}
	}
	DisplaySize (tmp_win, dragWidth, dragHeight, False);
}



/***********************************************************************
 *
 *  Procedure:
 *      DisplaySize - display the size in the dimensions window
 *
 *  Inputs:
 *      tmp_win - the current afterstep window
 *      width   - the width of the rubber band
 *      height  - the height of the rubber band
 *
 ***********************************************************************/
void
DisplaySize (ASWindow * tmp_win, int width, int height, Bool Init)
{
	char          str[100];
	int           dwidth, dheight, offset;
	GC            foreGC;

	if (last_width == width && last_height == height)
		return;

	last_width = width;
	last_height = height;

	get_client_geometry (tmp_win, 0, 0, width, height, NULL, NULL, &dwidth, &dheight);
	dwidth -= tmp_win->hints->base_width;
	dheight -= tmp_win->hints->base_height;
	dwidth /= tmp_win->hints->width_inc;
	dheight /= tmp_win->hints->height_inc;

	(void)sprintf (str, " %4d x %-4d ", dwidth, dheight);

	offset = (Scr.SizeStringWidth + SIZE_HINDENT * 2
			  - XTextWidth ((*Scr.MSFWindow).font.font, str, strlen (str))) / 2;

	if (Init)
	{
		XClearWindow (dpy, Scr.SizeWindow);
		mystyle_set_window_background (Scr.SizeWindow, Scr.MSFWindow);
		if (Scr.d_depth >= 2)
		{
			GC            reliefGC, shadowGC;

			mystyle_get_global_gcs (Scr.MSFWindow, NULL, NULL, &reliefGC, &shadowGC);
			RelieveWindow (tmp_win,
						   Scr.SizeWindow, 0, 0,
						   Scr.SizeStringWidth + SIZE_HINDENT * 2,
						   (*Scr.MSFWindow).font.height + SIZE_VINDENT * 2,
						   reliefGC, shadowGC, FULL_HILITE);
		}
	} else
	{
		XClearArea (dpy, Scr.SizeWindow, SIZE_HINDENT, SIZE_VINDENT,
					Scr.SizeStringWidth, (*Scr.MSFWindow).font.height, False);
	}

#ifdef I18N
#undef FONTSET
#define FONTSET (*Scr.MSFWindow).font.fontset
#endif
	mystyle_get_global_gcs (Scr.MSFWindow, &foreGC, NULL, NULL, NULL);
	XDrawString (dpy, Scr.SizeWindow, foreGC, offset,
				 (*Scr.MSFWindow).font.font->ascent + SIZE_VINDENT, str, 13);
}

/***********************************************************************
 *
 *  Procedure:
 *      ConstrainSize - adjust the given width and height to account for the
 *              constraints imposed by size hints
 *
 *      The general algorithm, especially the aspect ratio stuff, is
 *      borrowed from uwm's CheckConsistency routine.
 *
 ***********************************************************************/

void
ConstrainSize (ASWindow * tmp_win, int *widthp, int *heightp)
{
#define makemult(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
#define _min(a,b) (((a) < (b)) ? (a) : (b))
	int           minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
	int           baseWidth, baseHeight;
	int           dwidth = *widthp, dheight = *heightp;

	int maxAspectX  = tmp_win->hints->max_aspect.x ;
	int maxAspectY  = tmp_win->hints->max_aspect.y ;
	int minAspectX  = tmp_win->hints->min_aspect.x ;
	int minAspectY  = tmp_win->hints->min_aspect.y ;

	get_client_geometry (tmp_win, 0, 0, *widthp, *heightp, NULL, NULL, &dwidth, &dheight);

	minWidth = tmp_win->hints->min_width;
	minHeight = tmp_win->hints->min_height;

/*  minHeight = 1; */

	baseWidth = tmp_win->hints->base_width;
	baseHeight = tmp_win->hints->base_height;

	maxWidth = tmp_win->hints->max_width;
	maxHeight = tmp_win->hints->max_height;

/*    maxWidth = Scr.VxMax + Scr.MyDisplayWidth;
   maxHeight = Scr.VyMax + Scr.MyDisplayHeight; */

	xinc = tmp_win->hints->width_inc;
	yinc = tmp_win->hints->height_inc;
	if( xinc == 0 )
		xinc = 1 ;
	if( yinc == 0 )
		yinc = 1;

	/*
	 * First, clamp to min and max values
	 */
	if (dwidth < minWidth)
		dwidth = minWidth;
	if (dheight < minHeight)
		dheight = minHeight;

	if (dwidth > maxWidth)
		dwidth = maxWidth;
	if (dheight > maxHeight)
		dheight = maxHeight;


	/*
	 * Second, fit to base + N * inc
	 */
	dwidth = ((dwidth - baseWidth) / xinc * xinc) + baseWidth;
	dheight = ((dheight - baseHeight) / yinc * yinc) + baseHeight;


	/*
	 * Third, adjust for aspect ratio
	 */
	if( maxAspectX == 0 ) maxAspectX  = 1 ;
	if( maxAspectY == 0 ) maxAspectY  = 1 ;
	if( minAspectX == 0 ) minAspectX  = 1 ;
	if( minAspectY == 0 ) minAspectY  = 1 ;
	/*
	 * The math looks like this:
	 *
	 * minAspectX    dwidth     maxAspectX
	 * ---------- <= ------- <= ----------
	 * minAspectY    dheight    maxAspectY
	 *
	 * If that is multiplied out, then the width and height are
	 * invalid in the following situations:
	 *
	 * minAspectX * dheight > minAspectY * dwidth
	 * maxAspectX * dheight < maxAspectY * dwidth
	 *
	 */

	if (get_flags(tmp_win->hints->flags, AS_Aspect))
	{
		if (minAspectX * dheight > minAspectY * dwidth)
		{
			delta = makemult (minAspectX * dheight / minAspectY - dwidth, xinc);
			if (dwidth + delta <= maxWidth)
				dwidth += delta;
			else
			{
				delta = makemult (dheight - dwidth * minAspectY / minAspectX, yinc);
				if (dheight - delta >= minHeight)
					dheight -= delta;
			}
		}
		if (maxAspectX * dheight < maxAspectY * dwidth)
		{
			delta = makemult (dwidth * maxAspectY / maxAspectX - dheight, yinc);
			if (dheight + delta <= maxHeight)
				dheight += delta;
			else
			{
				delta = makemult (dwidth - maxAspectX * dheight / maxAspectY, xinc);
				if (dwidth - delta >= minWidth)
					dwidth -= delta;
			}
		}
	}
	/*
	 * Fourth, account for border width and title height
	 */
	get_frame_geometry (tmp_win, 0, 0, dwidth, dheight, NULL, NULL, widthp, heightp);

	return;
}


/***************
 *
 * Procedures:
 *
 *	draw_fvwm_outline ()
 *	draw_box_outline ()
 *	draw_barndoor_outline ()
 *	draw_wmaker_outline ()
 *	draw_tek_outline ()
 *	draw_tek2_outline ()
 *	draw_corners_outline ()
 *	draw_hash_outline ()
 *	draw_grid_outline ()
 *
 *	These functions draw/undraw the actual outlines on the root window.
 *	The first 3 were repeated code in MoveOutline (), the others were added by me.
 *
 *	- delt.    11 may '99
 *
 **************/

void
draw_fvwm_outline (int x, int y, int width, int height)
{
	XRectangle    rects[5];

	rects[0].x = x;
	rects[0].y = y;
	rects[0].width = width;
	rects[0].height = height;
	rects[1].x = x + 1;
	rects[1].y = y + 1;
	rects[1].width = width - 2;
	rects[1].height = height - 2;
	rects[2].x = x + 2;
	rects[2].y = y + 2;
	rects[2].width = width - 4;
	rects[2].height = height - 4;
	rects[3].x = x + 3;
	rects[3].y = y + 3 + (height - 6) / 3;
	rects[3].width = width - 6;
	rects[3].height = (height - 6) / 3;
	rects[4].x = x + 3 + (width - 6) / 3;
	rects[4].y = y + 3;
	rects[4].width = (width - 6) / 3;
	rects[4].height = (height - 6);
	XDrawRectangles (dpy, Scr.Root, Scr.DrawGC, rects, 5);
}											   /* draw_fvwm_outline ()  */

void
draw_twm_outline (int x, int y, int width, int height)
{
  XDrawRectangle (dpy, Scr.Root, Scr.DrawGC, x, y, width, height);
  XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + 1, y + height / 3, x - 1 + width, y + height / 3);
  XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + 1, y + height - height / 3, x - 1 + width, y + height - height / 3);
  XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + width / 3, y + 1, x + width / 3, y - 1 + height);
  XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + width - width / 3, y + 1, x + width - width / 3, y - 1 + height);
}				/* draw_twm_outline ()  */

void
draw_box_outline (int x, int y, int width, int height)
{
	XDrawRectangle (dpy, Scr.Root, Scr.DrawGC, x, y, width, height);
}

void
draw_barndoor_outline (int x, int y, int width, int height)
{
	draw_box_outline (x, y, width, height);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + 1, y + 1, x + width - 1, y + height - 1);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + width - 1, y + 1, x + 1, y + height - 1);
}

void
draw_wmaker_outline (int x, int y, int width, int height, ASWindow * win)
{
	/* frame window */
	draw_box_outline (x, y, width, height);

	if (win->flags & SHADED || win->flags & ICONIFIED || win == NULL)
		return;

	/* titlebar */
	if (ASWIN_HFLAGS(win,AS_Titlebar))
	{
		if (ASWIN_HFLAGS(win,AS_VerticalTitle))
			XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + win->title_width, y + 1,
					   x + win->title_width, y + height - 1);
		else
			XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + 1, y + win->title_height,
					   x + width - 1, y + win->title_height);
	}
	/* lowbar */
	if (ASWIN_HFLAGS(win,AS_Handles))
		XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + 1, y + height - 6, x + width - 1, y + height - 6);

	/* window borders */
	if (ASWIN_HFLAGS(win, AS_Frame))
	{
		if (ASWIN_HFLAGS(win,AS_VerticalTitle))
			draw_box_outline (x + Scr.fs[FR_W].w + win->title_width,
							  y + Scr.fs[FR_N].h, width - Scr.fs[FR_W].w -
							  Scr.fs[FR_E].w - win->title_width, height -
							  Scr.fs[FR_N].h - Scr.fs[FR_S].h);
		else
			draw_box_outline (x + Scr.fs[FR_W].w, y + Scr.fs[FR_N].h +
							  win->title_height, width - Scr.fs[FR_W].w -
							  Scr.fs[FR_E].w, height - Scr.fs[FR_N].h -
							  Scr.fs[FR_S].h - win->title_height);
	}
}

void
draw_tek_outline (int x, int y, int width, int height)
{
	draw_box_outline (x, y, width, height);

	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x - 5, y - 6, x + (width / 2) + 3, y - 6);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x - 6, y - 6, x - 6, y + (height / 2) + 2);

	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + (width / 2) + 3,
			   y + height + 6, x + width + 6, y + height + 6);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + width + 6,
			   y + height + 5, x + width + 6, y + (height / 2) + 3);
}

void
draw_tek2_outline (int x, int y, int width, int height)
{
	draw_tek_outline (x, y, width, height);

	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x - 11, y - 12, x + (width / 3), y - 12);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x - 12, y - 12, x - 12, y + (height / 3));
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + (2 * width / 3),
			   y + height + 12, x + width + 12, y + height + 12);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + width + 12,
			   y + height + 11, x + width + 12, y + (2 * height / 3));

}

void
draw_corners_outline (int x, int y, int width, int height)
{
	int           corner_height, corner_width;

	corner_height = height / 8;
	corner_width = width / 8;

	if (corner_height > 32)
		corner_height = 32;
	if (corner_width > 32)
		corner_width = 32;

	/* top left */
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + 1, y, x + corner_width, y);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x, y, x, y + corner_height);

	/* top right */
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + width - 1, y, x + width - corner_width, y);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + width, y, x + width, y + corner_height);

	/* btm left */
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + 1, y + height, x + corner_width, y + height);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x, y + height, x, y + height - corner_height);

	/* btm right */
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + width - 1, y + height, x + width - corner_width,
			   y + height);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + width, y + height, x + width,
			   y + height - corner_height);

	/* crosshair */
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + (width / 2) - 4, y + (height / 2),
			   x + (width / 2) + 4, y + (height / 2));
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + (width / 2), y + (height / 2) - 4,
			   x + (width / 2), y + (height / 2) + 4);
}

void
draw_hash_outline (int x, int y, int width, int height)
{
	draw_box_outline (x + 1, y + 1, width - 2, height - 2);

	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x, 0, x, Scr.MyDisplayHeight);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, x + width, 0, x + width, Scr.MyDisplayHeight);

	XDrawLine (dpy, Scr.Root, Scr.DrawGC, 0, y, Scr.MyDisplayWidth, y);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, 0, y + height, Scr.MyDisplayWidth, y + height);
}

void
draw_grid_outline (int x, int y, int width, int height)
{
	int           i, numlines, screen_length;
	static XSegment *lines = NULL;

	if (lines == NULL)						   /* first time, malloc the right number of segments for screen size */
	{
		screen_length = Scr.MyDisplayWidth < Scr.MyDisplayHeight ?
			Scr.MyDisplayHeight : Scr.MyDisplayWidth;
		lines = malloc (sizeof (XSegment) * (screen_length / 64));
		/* fprintf (stderr, "draw_grid_outline () -- MALLOC'ING %ld BYTES",
		   (long int) sizeof (XSegment) * (screen_length / 64)); */
	}

	/* if */
	/* 2-pixel wide box */
	draw_box_outline (x, y, width, height);
	draw_box_outline (x + 1, y + 1, width - 2, height - 2);

	/* vertical lines */
	numlines = 0;
	for (i = 0; i < (x + width - 2); i += 64)
		if (i > x + 2)
		{
			lines[numlines].x1 = i;
			lines[numlines].y1 = y + 2;
			lines[numlines].x2 = i;
			lines[numlines].y2 = y + height - 2;
			numlines++;
		}
	XDrawSegments (dpy, Scr.Root, Scr.DrawGC, lines, numlines);

	/* horizontal lines */
	numlines = 0;
	for (i = 0; i < (y + height - 2); i += 64)
		if (i > y + 2)
		{
			lines[numlines].x1 = x + 2;
			lines[numlines].y1 = i;
			lines[numlines].x2 = x + width - 2;
			lines[numlines].y2 = i;
			numlines++;
		}
	XDrawSegments (dpy, Scr.Root, Scr.DrawGC, lines, numlines);
}


/***********************************************************************
 *
 *  Procedure:
 *	MoveOutline - move a window outline
 *
 *  Inputs:
 *	win	    - the ASWindow being outlined
 *	x	    - upper left x coordinate
 *	y	    - upper left y coordinate
 *	width	    - the width of the rectangle
 *	height	    - the height of the rectangle
 *
 ***********************************************************************/
void
MoveOutline ( /* Window root, */ ASWindow * win, int x, int y, int width, int height)
{
	/* Rubberband type

	 * 0 is the old default FVWM style look
	 * 1 is a single rectangle the size of the window.
	 * 2 is crossed-out outline.
	 * 3 is a WindowMaker style outline, showing the titlebar and handles, if the window has them.
	 * 4 is a cool tech-looking outline.
	 * 5 is another cool tech-looking outline.
	 * 6 is the corners of the window with a crosshair in the middle.
	 * 7 is lines spanning the whole screen framing the window. (CAD style)
	 * 8 is a 2-pixel wide frame containing a fixed grid.
     * 9 is like 0, except with a thinner outer border.	 
	 * (Might have afterstep default to 3 if allanon_, nekked, & the gang don't have any objections)
	 */
	extern int    RubberBand;
	static int    lastx = 0;
	static int    lasty = 0;
	static int    lastWidth = 0;
	static int    lastHeight = 0;

	/* if (win == NULL)
	   fprintf (stderr, "From MoveOutline () -- win == NULL\n"); */

	if (x == lastx && y == lasty && width == lastWidth && height == lastHeight)
		return;

	/* undraw the old one, if any */
	if (lastWidth || lastHeight)
	{
		switch (RubberBand)
		{
		 case 0:
			 draw_fvwm_outline (lastx, lasty, lastWidth, lastHeight);
			 break;
		 case 1:
			 draw_box_outline (lastx, lasty, lastWidth, lastHeight);
			 break;
		 case 2:
			 draw_barndoor_outline (lastx, lasty, lastWidth, lastHeight);
			 break;
		 case 3:
			 draw_wmaker_outline (lastx, lasty, lastWidth, lastHeight, win);
			 break;
		 case 4:
			 draw_tek_outline (lastx, lasty, lastWidth, lastHeight);
			 break;
		 case 5:
			 draw_tek2_outline (lastx, lasty, lastWidth, lastHeight);
			 break;
		 case 6:
			 draw_corners_outline (lastx, lasty, lastWidth, lastHeight);
			 break;
		 case 7:
			 draw_hash_outline (lastx, lasty, lastWidth, lastHeight);
			 break;
		 case 8:
			 draw_grid_outline (lastx, lasty, lastWidth, lastHeight);
			 break;
	 	case 9:
		 	 draw_twm_outline (lastx, lasty, lastWidth, lastHeight);
			 break;
			 /* this should normally not happen */
		 default:
			 draw_fvwm_outline (lastx, lasty, lastWidth, lastHeight);
			 /* fprintf (stderr,
			    "Hey, set your RubberBand to something reasonable!!\n");
			  */
			 break;
		}
	}

	lastx = x;
	lasty = y;
	lastWidth = width;
	lastHeight = height;

	/* draw the new one, if any */
	if (lastWidth || lastHeight)
	{
		switch (RubberBand)
		{
		 case 0:
			 draw_fvwm_outline (x, y, width, height);
			 break;
		 case 1:
			 draw_box_outline (x, y, width, height);
			 break;
		 case 2:
			 draw_barndoor_outline (x, y, width, height);
			 break;
		 case 3:
			 draw_wmaker_outline (x, y, width, height, win);
			 break;
		 case 4:
			 draw_tek_outline (x, y, width, height);
			 break;
		 case 5:
			 draw_tek2_outline (x, y, width, height);
			 break;
		 case 6:
			 draw_corners_outline (x, y, width, height);
			 break;
		 case 7:
			 draw_hash_outline (x, y, width, height);
			 break;
		 case 8:
			 draw_grid_outline (x, y, width, height);
			 break;
         case 9:
             draw_twm_outline (x, y, width, height);
			 break;
			 /* uh, which one dudes? */
		 default:
			 draw_fvwm_outline (x, y, width, height);
			 break;
		}
	}
}
