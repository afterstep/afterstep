
/****************************************************************************
 * This module is based on Twm, but has been SIGNIFICANTLY modified 
 * Copyright (c) 1998, 1999 Ethan Fischer <allanon@crystaltokyo.com>
 * by Rob Nation 
 * by Bo Yang
 * by Frank Fejes
 *****************************************************************************/

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


/***********************************************************************
 *
 * afterstep window border drawing code
 *
 ***********************************************************************/

#include "../../configure.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/ascolor.h"
#include "../../include/stepgfx.h"

#include "../../include/pixmap.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

/* macro to change window background color/pixmap */
#define ChangeWindowColor(window) {\
        if(NewColor)\
           {\
             XChangeWindowAttributes(dpy,window,valuemask, &attributes);\
             XClearWindow(dpy,window);\
           }\
         }

extern Window PressedW;
extern char *TextTextureStyle ;


/**************************************************************
 * 
 * Sets the window background to the correct texture 
 * 
 **************************************************************/
#define STATE_FOCUSED	0
#define STATE_UNFOCUSED 1
#define STATE_STICKY	2
void
SetBackgroundTexture (ASWindow * t, Window win, MyStyle * style, Pixmap cache)
{
  int need_to_free_cache = 0;

  if (style->texture_type == TEXTURE_SOLID)
    XSetWindowBackground (dpy, win, style->colors.back);
#ifndef NO_TEXTURE
  else if (style->texture_type < TEXTURE_PIXMAP)
    {
      /* gradients */
      if (cache == None)
	{
	  int width, height;
	  XGetGeometry (dpy, win, &JunkRoot, &JunkX, &JunkY, &width, &height, &JunkBW, &JunkDepth);
	  cache = mystyle_make_pixmap (style, width, height, None);
	  if (cache != None)
	    need_to_free_cache = 1;
	}
      XSetWindowBackgroundPixmap (dpy, win, cache);
    }
  else if (style->texture_type == TEXTURE_PIXMAP)
    XSetWindowBackgroundPixmap (dpy, win, style->back_icon.pix);
  else if (style->texture_type >= TEXTURE_TRANSPARENT)
    {
      if (cache == None)
	XSetWindowBackgroundPixmap (dpy, win, ParentRelative);
      else
	XSetWindowBackgroundPixmap (dpy, win, cache);
    }
#endif /* NO_TEXTURE */

  XClearWindow (dpy, win);
  XFlush (dpy);

  if (need_to_free_cache)
    {
      /* don't change the window background, or the result will be whatever 
       * the background is changed to here */
      XFreePixmap (dpy, cache);
    }
}

/************************************************
 *                                              * 
 * a more general version of SetTitleBackground *
 * to set the background of the icon title      *
 *                                              *
 ************************************************/

int
SetBackground (ASWindow * t, Window win)
{
  MyStyle *style;

  if (t == Scr.Hilite)
    style = t->style_focus;
  else if (t->flags & STICKY)
    style = t->style_sticky;
  else
    style = t->style_unfocus;

  SetBackgroundTexture (t, win, style, None);
  return style->texture_type;
}

/************************************************************************
 * 
 * Sets the window borders to the correct background
 * 
 **********************************************************************/
int
SetTitleBackground (ASWindow * t, Bool focused)
{
  MyStyle *style;
  Pixmap cache = None;

  /* the title background */
  if (focused)
    {
      style = t->style_focus;
#ifndef NO_TEXTURE
      cache = t->backPixmap;
#endif /* !NO_TEXTURE */
    }
  else if (t->flags & STICKY)
    {
      style = t->style_sticky;
#ifndef NO_TEXTURE
      cache = t->backPixmap3;
#endif /* !NO_TEXTURE */
    }
  else
    {
      style = t->style_unfocus;
#ifndef NO_TEXTURE
      cache = t->backPixmap2;
#endif /* !NO_TEXTURE */
    }

  /* only use cache for gradients and transparent */
  if (!((style->texture_type >= TEXTURE_GRADIENT && style->texture_type < TEXTURE_PIXMAP) ||
	style->texture_type >= TEXTURE_TRANSPARENT))
    cache = None;

  SetBackgroundTexture (t, t->title_w, style, cache);
  return style->texture_type;
}

void
SetBottomBackground (ASWindow * t, Bool focused)
{
  MyStyle *style;
  Pixmap cache = None;

  /* the title background */
  if (focused)
    {
      style = t->style_focus;
#ifndef NO_TEXTURE
      cache = t->backPixmap;
#endif /* !NO_TEXTURE */
    }
  else if (t->flags & STICKY)
    {
      style = t->style_sticky;
#ifndef NO_TEXTURE
      cache = t->backPixmap3;
#endif /* !NO_TEXTURE */
    }
  else
    {
      style = t->style_unfocus;
#ifndef NO_TEXTURE
      cache = t->backPixmap2;
#endif /* !NO_TEXTURE */
    }

  /* only use cache for gradients */
  if (!(style->texture_type >= TEXTURE_GRADIENT && style->texture_type < TEXTURE_PIXMAP))
    cache = None;

  /* middle bottom frame */
  SetBackgroundTexture (t, t->side, style, cache);
  /* side bottom frames */
  SetBackgroundTexture (t, t->corners[0], style, cache);
  SetBackgroundTexture (t, t->corners[1], style, cache);
}

void
draw_button (ASWindow * t, Window w, int index, GC DrawGC, GC ReliefGC, GC ShadowGC)
{
  int style = Scr.button_style[index];
  int width = Scr.button_width[index];
  int height = Scr.button_height[index];
  Pixmap pix;
  Pixmap mask;

  if (PressedW != w)
    {
      pix = Scr.button_pixmap[index];
      mask = Scr.button_pixmap_mask[index];
      RelieveWindow (t, w, 0, 0, width, height, ReliefGC, ShadowGC,
		     BOTTOM_HILITE | RIGHT_HILITE);
    }
  else
    {
      pix = Scr.dbutton_pixmap[index];
      mask = Scr.dbutton_pixmap_mask[index];
      RelieveWindow (t, w, 0, 0, width, height, ShadowGC, ReliefGC,
		     BOTTOM_HILITE | RIGHT_HILITE);
    }

  switch (style)
    {
    case XPM_BUTTON_STYLE:
#ifdef SHAPE
      XShapeCombineMask (dpy, w, ShapeBounding, 0, 0, mask, ShapeSet);
#endif /* SHAPE */
      XCopyArea (dpy, pix, w, DrawGC, 0, 0, width, height, 0, 0);
      break;

    default:
      afterstep_err ("Old 10x10 button styles should not be used\n", NULL, NULL, NULL);
      Done (0, NULL);
    }
}

int
SetTransparency (ASWindow * t)
{
  int updated = 0;
#ifndef NO_TEXTURE
  int x = t->frame_x + t->title_x + t->bw, y = t->frame_y + t->title_y + t->bw;
  /* ignore tiny windows */
  if (t->title_width <= 2 || t->title_height <= 2)
    return updated;
  if (t->style_focus->texture_type >= TEXTURE_TRANSPARENT)
    {
      if (t->backPixmap != None)
	XFreePixmap (dpy, t->backPixmap);
      t->backPixmap = mystyle_make_pixmap_overlay (t->style_focus, x, y, t->title_width, t->title_height, None);
      updated = 1;
    }
  if (t->style_unfocus->texture_type >= TEXTURE_TRANSPARENT)
    {
      if (t->backPixmap2 != None)
	XFreePixmap (dpy, t->backPixmap2);
      t->backPixmap2 = mystyle_make_pixmap_overlay (t->style_unfocus, x, y, t->title_width, t->title_height, None);
      updated = 1;
    }
  if (t->style_sticky->texture_type >= TEXTURE_TRANSPARENT)
    {
      if (t->backPixmap3 != None)
	XFreePixmap (dpy, t->backPixmap3);
      t->backPixmap3 = mystyle_make_pixmap_overlay (t->style_sticky, x, y, t->title_width, t->title_height, None);
      updated = 1;
    }
#endif /* !NO_TEXTURE */
  return updated;
}

/****************************************************************************
 *
 * Redraws the windows borders
 *
 ****************************************************************************/
void
SetBorder (ASWindow * t, Bool is_focus, Bool force, Bool Mapped,
	   Window expose_win)
{
  int i, x;
  GC DrawGC, ReliefGC, ShadowGC;
  Bool NewColor = False;
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  static unsigned int corners[2];
  MyStyle *style;

  corners[0] = BOTTOM_HILITE | LEFT_HILITE;
  corners[1] = BOTTOM_HILITE | RIGHT_HILITE;

  if (!t)
    return;

  /* don't re-draw just for kicks */
  if ((!force) && (Scr.Hilite != t))
    return;

  if (Scr.Hilite == t)
    {
      Scr.Hilite = NULL;
      NewColor = True;
    }
/* take care of the previously highlighted window */
  if ((is_focus) && (Scr.Hilite != t))
    {

      /* make sure that the previously highlighted window got unhighlighted */
      if (Scr.Hilite != NULL)
	SetBorder (Scr.Hilite, False, False, True, None);
      Scr.Hilite = t;
    }
/* figure out our style */
  if (is_focus)
    {
      style = t->style_focus;
    }
  else if (t->flags & STICKY)
    {
      style = t->style_sticky;
    }
  else
    {
      style = t->style_unfocus;
    }

/* set the GCs */
  mystyle_get_global_gcs (style, &DrawGC, NULL, &ReliefGC, &ShadowGC);

/* icons are handled separately */
  if (t->flags & ICONIFIED)
    {
      DrawIconWindow (t);
      return;
    }
  attributes.border_pixel = (*style).colors.back;
  attributes.background_pixel = (*style).colors.back;;
  valuemask = CWBorderPixel | CWBackPixel;

  if (t->flags & (TITLE | BORDER))
    {
      XSetWindowBorder (dpy, t->Parent, BlackPixel (dpy, DefaultScreen (dpy)));
      XSetWindowBorder (dpy, t->frame, BlackPixel (dpy, DefaultScreen (dpy)));
    }
  if (t->flags & TITLE)
    SetTitleBar (t, is_focus, False);
  if (t->flags & BORDER)
    {
      /* draw relief lines */
      x = t->frame_width - 2 * t->corner_width + t->bw;
      SetBottomBackground (t, is_focus);
      RelieveWindow (t, t->side, 0, 0, x, t->boundary_height - t->bw,
		     ReliefGC, ShadowGC, 0x0004);
      for (i = 0; i < 2; i++)
	{
	  RelieveWindow (t, t->corners[i], 0, 0, t->corner_width,
			 t->boundary_height, ReliefGC, ShadowGC, corners[i]);

	  if (t->boundary_height > 1)
	    RelieveParts (t, i, ReliefGC, ShadowGC);
	  else
	    RelieveParts (t, i, ReliefGC, ShadowGC);
	}
    }
}

/****************************************************************************
 *
 *  Redraws just the title bar
 *
 ****************************************************************************/
void
SetTitleBar (ASWindow * t, Bool is_focus, Bool NewTitle)
{
  int i, text_x = 0, text_y = 0, text_w = 0;
  GC DrawGC, ReliefGC, ShadowGC;
  char *txt = NULL;
  MyStyle *style, *text_texture_style = NULL;
#ifdef I18N
#undef FONTSET
#define FONTSET (*style).font.fontset
#endif /* I18N */

  if ((t == NULL) || (!(t->flags & TITLE)))
    return;

/* set the GCs */
  if (is_focus)
    style = t->style_focus;
  else if (t->flags & STICKY)
    style = t->style_sticky;
  else
    style = t->style_unfocus;
  mystyle_get_global_gcs (style, &DrawGC, NULL, &ReliefGC, &ShadowGC);

#ifndef NO_TEXTURE
  if (!(Textures.flags & TitlebarNoPush))
#endif /* NO_TEXTURE */

    if (PressedW == t->title_w)
      {
	GC tGC = ShadowGC;
	ShadowGC = ReliefGC;
	ReliefGC = tGC;
      }
  flush_expose (t->title_w);

  if (t->name != (char *) NULL)
    {
      if (t->flags & VERTICAL_TITLE)
	{
	  txt = fit_vertical_text ((*style).font, t->name,
				   strlen (t->name), t->title_height -
				   t->space_taken_left_buttons -
				   t->space_taken_right_buttons - 6);
	  text_w = strlen (txt) * style->font.height;
	  if (text_w > t->title_height - 4)
	    text_w = t->title_height - 4;
	}
      else
	{
	  txt = fit_horizontal_text ((*style).font, t->name,
				     strlen (t->name), t->title_width -
				     t->space_taken_left_buttons -
				     t->space_taken_right_buttons - 6);
	  text_w = XTextWidth ((*style).font.font, txt, strlen (txt));
	  if (text_w > t->title_width - 4)
	    text_w = t->title_width - 4;
	}
      if (text_w < 0)
	text_w = 0;
    }

  /* compute the vertical position for the title */
  if (t->flags & VERTICAL_TITLE)
    text_x = (t->title_width - (*style).font.width) / 2;
  else
    text_y = (t->title_height - (*style).font.height) / 2 + (*style).font.y;

/* figure out where the title text goes */
  if (t->flags & VERTICAL_TITLE)
    switch (Scr.TitleTextAlign)
      {
      case JUSTIFY_LEFT:
	text_y = (*style).font.y + 6 + t->space_taken_left_buttons;
	break;
      case JUSTIFY_RIGHT:
	text_y = (*style).font.y + t->title_height - (text_w + t->space_taken_right_buttons + 6);
	break;
      case JUSTIFY_CENTER:
      default:
	text_y = (*style).font.y + (t->title_height - t->space_taken_left_buttons - t->space_taken_right_buttons - text_w) / 2 + t->space_taken_left_buttons;
	break;
      }
  else
    switch (Scr.TitleTextAlign)
      {
      case JUSTIFY_LEFT:
	text_x = t->space_taken_left_buttons + 6;
	break;
      case JUSTIFY_RIGHT:
	text_x = t->title_width - text_w - t->space_taken_right_buttons - 6;
	break;
      case JUSTIFY_CENTER:
      default:
	text_x = (t->title_width - text_w) / 2;
	break;
      }

  if (NewTitle)
    XClearWindow (dpy, t->title_w);

  /* draw titletext : for mono, we clear an area in the title bar where the
   * window title goes, so that its more legible. For color, no need */
  if (Scr.d_depth < 2)
    {
      RelieveWindow (t, t->title_w, 0, 0, text_x - 2, t->title_height,
		     ReliefGC, ShadowGC, BOTTOM_HILITE);
      RelieveWindow (t, t->title_w, text_x + text_w + 2, 0,
		     t->title_width - text_w - text_x - 2, t->title_height,
		     ReliefGC, ShadowGC, BOTTOM_HILITE);
      XFillRectangle (dpy, t->title_w,
		      (PressedW == t->title_w ? ShadowGC : ReliefGC),
		      text_x - 2, 0, text_w + 4, t->title_height);

      XDrawLine (dpy, t->title_w, ShadowGC, text_x + text_w + 1, 0,
		 text_x + text_w + 1, t->title_height);
      if (t->name != (char *) NULL)
	XDrawString (dpy, t->title_w, DrawGC, text_x, text_y,
		     txt, strlen (txt));
    }
  else
    {
#ifndef NO_TEXTURE
      int type;

      type = SetTitleBackground (t, is_focus);

      if (!(Textures.flags & TitlebarNoPush) || (type == 0) || (type == TEXTURE_PIXMAP))
	{
	  RelieveWindow (t, t->title_w, 0, 0, t->bp_width, t->bp_height + 1,
			 ReliefGC, ShadowGC, BOTTOM_HILITE | RIGHT_HILITE);
	  XFlush (dpy);
	}
      if (t->name != NULL)
	{
		if (is_focus && TextTextureStyle != NULL)
		    text_texture_style = mystyle_find(TextTextureStyle);

	    if (t->flags & VERTICAL_TITLE)
	    	mystyle_draw_texturized_vertical_text (t->title_w, style, text_texture_style, txt, text_x, text_y);
		else
	    	mystyle_draw_texturized_text (t->title_w, style, text_texture_style, txt, text_x, text_y);
	}
#else /* NO_TEXTURE defined */
      if (t->name != (char *) NULL)
	XDrawString (dpy, t->title_w, DrawGC, text_x, text_y,
		     t->name, strlen (t->name));

      RelieveWindow (t, t->title_w, 0, 0, t->title_width, t->title_height,
		     ReliefGC, ShadowGC, BOTTOM_HILITE | RIGHT_HILITE);
#endif /* NO_TEXTURE */
    }
  if (txt)
    free (txt);
  flush_expose (t->title_w);
  for (i = 0; i < Scr.nr_left_buttons; i++)
    if (t->left_w[i] != None)
      {
	flush_expose (t->left_w[i]);
	draw_button (t, t->left_w[i], i * 2 + 1, DrawGC, ReliefGC, ShadowGC);
      }

  for (i = 0; i < Scr.nr_right_buttons; i++)
    if (t->right_w[i] != None)
      {
	flush_expose (t->right_w[i]);
	draw_button (t, t->right_w[i], (i * 2 + 2) % 10, DrawGC, ReliefGC, ShadowGC);
      }


  XFlush (dpy);
}

/****************************************************************************
 *
 *  Draws the relief pattern around a window
 *
 ****************************************************************************/
AFTER_INLINE void
RelieveWindow (ASWindow * t, Window win,
	       int x, int y, int w, int h,
	       GC ReliefGC, GC ShadowGC, int hilite)
{
  XSegment seg[4];
  int i;
  int edge;

  if (!win)
    return;

  edge = 0;
  if (win == t->side)
    edge = -1;
  if (win == t->corners[0])
    edge = 1;
  if (win == t->corners[1])
    edge = 2;

  i = 0;
  seg[i].x1 = x;
  seg[i].y1 = y;
  seg[i].x2 = w + x - 1;
  seg[i++].y2 = y;

  seg[i].x1 = x;
  seg[i].y1 = y;
  seg[i].x2 = x;
  seg[i++].y2 = h + y - 1;

  if (((t->boundary_height > 2) || (edge == 0)) &&
      ((t->boundary_height > 3) || (edge < 1)) &&
      (((edge == 0) || (t->boundary_height > 3)) && (hilite & TOP_HILITE)))
    {
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + w - 2;
      seg[i++].y2 = y + 1;
    }
  if (((t->boundary_height > 2) || (edge == 0)) &&
      ((t->boundary_height > 3) || (edge < 1)) &&
      (((edge == 0) || (t->boundary_height > 3)) && (hilite & LEFT_HILITE)))
    {
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + 1;
      seg[i++].y2 = y + h - 2;
    }
  XDrawSegments (dpy, win, ReliefGC, seg, i);
  i = 0;
  seg[i].x1 = x;
  seg[i].y1 = y + h - 1;
  seg[i].x2 = w + x - 1;
  seg[i++].y2 = y + h - 1;

  if (((t->boundary_height > 2) || (edge == 0)) &&
    (((edge == 0) || (t->boundary_height > 3)) && (hilite & BOTTOM_HILITE)))
    {
      seg[i].x1 = x + 1;
      seg[i].y1 = y + h - 2;
      seg[i].x2 = x + w - 2;
      seg[i++].y2 = y + h - 2;
    }
  seg[i].x1 = x + w - 1;
  seg[i].y1 = y;
  seg[i].x2 = x + w - 1;
  seg[i++].y2 = y + h - 1;

  if (((t->boundary_height > 2) || (edge == 0)) &&
      (((edge == 0) || (t->boundary_height > 3)) && (hilite & RIGHT_HILITE)))
    {
      seg[i].x1 = x + w - 2;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + w - 2;
      seg[i++].y2 = y + h - 2;
    }
  XDrawSegments (dpy, win, ShadowGC, seg, i);

}

void
RelieveParts (ASWindow * t, int i, GC hor, GC vert)
{
  XSegment seg[2];

  if (!t->corners[i])
    return;
  switch (i)
    {
    case 0:
      seg[0].x1 = t->boundary_width - 1;
      /* used to be -2 */
      seg[0].x2 = t->corner_width - 2;
      seg[0].y1 = t->bw;
      seg[0].y2 = t->bw;
      break;
    case 1:
      seg[0].x1 = 0;
      seg[0].x2 = t->corner_width - t->boundary_width;
      seg[0].y1 = t->bw;
      seg[0].y2 = t->bw;
      break;
    }
  XDrawSegments (dpy, t->corners[i], hor, seg, 1);
}

/***********************************************************************
 *
 *  Procedure:
 *      Setupframe - set window sizes, this was called from either
 *              AddWindow, EndResize, or HandleConfigureNotify.
 *
 *  Inputs:
 *      tmp_win - the ASWindow pointer
 *      x       - the x coordinate of the upper-left outer corner of the frame
 *      y       - the y coordinate of the upper-left outer corner of the frame
 *      w       - the width of the frame window w/o border
 *      h       - the height of the frame window w/o border
 *
 *  Special Considerations:
 *      This routine will check to make sure the window is not completely
 *      off the display, if it is, it'll bring some of it back on.
 *
 *      The tmp_win->frame_XXX variables should NOT be updated with the
 *      values of x,y,w,h prior to calling this routine, since the new
 *      values are compared against the old to see whether a synthetic
 *      ConfigureNotify event should be sent.  (It should be sent if the
 *      window was moved but not resized.)
 *
 ************************************************************************/

void
SetupFrame (ASWindow * tmp_win, int x, int y, int w, int h, Bool sendEvent)
{
  XEvent client_event;
  XWindowChanges frame_wc, xwc;
  unsigned long frame_mask, xwcm;
  int cx, cy, i;
  Bool Moved = False, Resized = False;
  int xwidth, left, right;

/*    if (tmp_win->flags & SHADED)
 *    return;
 */

  /* if windows is not being maximized, save size in case of maximization */
  if (!(tmp_win->flags & MAXIMIZED))
    {
      tmp_win->orig_x = x;
      tmp_win->orig_y = y;
      tmp_win->orig_wd = w;
      tmp_win->orig_ht = h;
    }
  if (Scr.flags & DontMoveOff)
    {
      if (x + Scr.Vx + w < 16)
	x = 16 - Scr.Vx - w;
      if (y + Scr.Vy + h < 16)
	y = 16 - Scr.Vy - h;
    }

  /* if the window would be completely offdesk, move it 16 pixels ondesk */
  if (x + w + 2 * tmp_win->bw < -Scr.Vx)
    x = -Scr.Vx + 16 - (w + 2 * tmp_win->bw);
  if (y + h + 2 * tmp_win->bw < -Scr.Vy)
    y = -Scr.Vy + 16 - (h + 2 * tmp_win->bw);
  if (x >= Scr.MyDisplayWidth + Scr.VxMax - Scr.Vx)
    x = Scr.MyDisplayWidth + Scr.VxMax - Scr.Vx - 16;
  if (y >= Scr.MyDisplayHeight + Scr.VyMax - Scr.Vy)
    y = Scr.MyDisplayHeight + Scr.VyMax - Scr.Vy - 16;

  /*
   * According to the July 27, 1988 ICCCM draft, we should send a
   * "synthetic" ConfigureNotify event to the client if the window
   * was moved but not resized.
   */
  if ((x != tmp_win->frame_x || y != tmp_win->frame_y) &&
      (w == tmp_win->frame_width && h == tmp_win->frame_height))
    sendEvent = TRUE;

  if ((x - tmp_win->frame_x) % Scr.MyDisplayWidth || (y - tmp_win->frame_y) % Scr.MyDisplayHeight)
    Moved = True;

  if (w != tmp_win->frame_width || h != tmp_win->frame_height)
    Resized = True;

  tmp_win->frame_x = x;
  tmp_win->frame_y = y;
  tmp_win->frame_width = w;
  tmp_win->frame_height = h;

  get_parent_geometry (tmp_win, tmp_win->frame_width, tmp_win->frame_height, &cx, &cy, &tmp_win->attr.width, &tmp_win->attr.height);

  if (Resized)
    {
      left = tmp_win->nr_left_buttons;
      right = tmp_win->nr_right_buttons;

      if (tmp_win->flags & TITLE)
	{
	  set_titlebar_geometry (tmp_win);

	  xwcm = CWX | CWY | CWWidth | CWHeight;
	  xwc.x = tmp_win->title_x;
	  xwc.y = tmp_win->title_y;
	  xwc.width = tmp_win->title_width;
	  xwc.height = tmp_win->title_height;
	  XConfigureWindow (dpy, tmp_win->title_w, xwcm, &xwc);

	  if (tmp_win->flags & VERTICAL_TITLE)
	    {
	      xwcm = CWX | CWY;
	      /* if TitleButtonStyle == 0, add a border from the side, otherwise
	         start from the very side of the window */
	      if (!Scr.TitleButtonStyle)
		xwc.y = tmp_win->title_y + 3;
	      else
		xwc.y = tmp_win->title_y + 1;

	      for (i = 0; i < Scr.nr_left_buttons; i++)
		{
		  if (tmp_win->left_w[i] != None)
		    {
		      xwc.x = tmp_win->title_x + (tmp_win->title_width - Scr.button_width[i + i + 1]) / 2;
		      if (xwc.y + Scr.button_height[i + i + 1] + Scr.TitleButtonSpacing < tmp_win->title_y + tmp_win->title_height)
			XConfigureWindow (dpy, tmp_win->left_w[i], xwcm, &xwc);
		      else
			{
			  xwc.y = -999;
			  XConfigureWindow (dpy, tmp_win->left_w[i], xwcm, &xwc);
			}
		      xwc.y += Scr.button_height[i + i + 1] + Scr.TitleButtonSpacing;
		    }
		}

	      /* if TitleButtonStyle == 0, add a border from the side, otherwise
	         start from the very side of the window */
	      xwc.y = tmp_win->title_y + tmp_win->title_height;
	      if (!Scr.TitleButtonStyle)
		xwc.y -= 3;
	      else
		xwc.y -= 1;

	      for (i = 0; i < Scr.nr_right_buttons; i++)
		{
		  if (tmp_win->right_w[i] != None)
		    {
		      xwc.x = tmp_win->title_x + (tmp_win->title_width - Scr.button_width[(i + i + 2) % 10]) / 2;
		      xwc.y -= Scr.button_height[(i + i + 2) % 10] + Scr.TitleButtonSpacing;
		      if (xwc.y > tmp_win->title_y)
			XConfigureWindow (dpy, tmp_win->right_w[i], xwcm, &xwc);
		      else
			{
			  xwc.y = -999;
			  XConfigureWindow (dpy, tmp_win->right_w[i], xwcm, &xwc);
			}
		    }
		}
	    }
	  else
	    {
	      xwcm = CWX | CWY;
	      /* if TitleButtonStyle == 0, add a border from the side, otherwise
	         start from the very side of the window */
	      if (!Scr.TitleButtonStyle)
		xwc.x = tmp_win->title_x + 3;
	      else
		xwc.x = tmp_win->title_x + 1;

	      for (i = 0; i < Scr.nr_left_buttons; i++)
		{
		  if (tmp_win->left_w[i] != None)
		    {
		      xwc.y = tmp_win->title_y + (tmp_win->title_height - Scr.button_height[i + i + 1]) / 2;
		      if (xwc.x + Scr.button_width[i + i + 1] + Scr.TitleButtonSpacing < tmp_win->title_x + tmp_win->title_width)
			XConfigureWindow (dpy, tmp_win->left_w[i], xwcm, &xwc);
		      else
			{
			  xwc.x = -999;
			  XConfigureWindow (dpy, tmp_win->left_w[i], xwcm, &xwc);
			}
		      xwc.x += Scr.button_width[i + i + 1] + Scr.TitleButtonSpacing;
		    }
		}

	      /* if TitleButtonStyle == 0, add a border from the side, otherwise
	         start from the very side of the window */
	      xwc.x = tmp_win->title_x + tmp_win->title_width;
	      if (!Scr.TitleButtonStyle)
		xwc.x -= 3;
	      else
		xwc.x -= 1;

	      for (i = 0; i < Scr.nr_right_buttons; i++)
		{
		  if (tmp_win->right_w[i] != None)
		    {
		      xwc.y = tmp_win->title_y + (tmp_win->title_height - Scr.button_height[(i + i + 2) % 10]) / 2;
		      xwc.x -= Scr.button_width[(i + i + 2) % 10] + Scr.TitleButtonSpacing;
		      if (xwc.x > tmp_win->title_x)
			XConfigureWindow (dpy, tmp_win->right_w[i], xwcm, &xwc);
		      else
			{
			  xwc.x = -999;
			  XConfigureWindow (dpy, tmp_win->right_w[i], xwcm, &xwc);
			}
		    }
		}
	    }
	}
      if (tmp_win->flags & BORDER)
	{
	  xwidth = w - 2 * tmp_win->corner_width + tmp_win->bw;
	  xwcm = CWWidth | CWHeight | CWX | CWY;
	  if (xwidth < 2)
	    xwidth = 2;

	  xwc.x = tmp_win->corner_width;
	  xwc.y = h - tmp_win->boundary_height + tmp_win->bw;
	  xwc.height = tmp_win->boundary_height + tmp_win->bw;
	  xwc.width = xwidth;
	  XConfigureWindow (dpy, tmp_win->side, xwcm, &xwc);

	  xwcm = CWX | CWY;
	  for (i = 0; i < 2; i++)
	    {
	      if (i)
		xwc.x = w - tmp_win->corner_width + tmp_win->bw;
	      else
		xwc.x = 0;

	      xwc.y = h - tmp_win->boundary_height;
	      XConfigureWindow (dpy, tmp_win->corners[i], xwcm, &xwc);
	    }
	}
    }
#ifndef NO_TEXTURE
  if (tmp_win->flags & FRAME)
    {
      int i;

      frame_set_positions (tmp_win);
      for (i = 0; i < 8; i++)
	{
	  xwc.x = tmp_win->fp[i].x;
	  xwc.y = tmp_win->fp[i].y;
	  xwc.width = tmp_win->fp[i].w;
	  xwc.height = tmp_win->fp[i].h;
	  xwcm = CWWidth | CWHeight | CWX | CWY;
	  XConfigureWindow (dpy, tmp_win->fw[i], xwcm, &xwc);
	}
    }
#endif /* !NO_TEXTURE */

  if (tmp_win->attr.height <= 0)
    tmp_win->attr.height = tmp_win->hints.height_inc;

  XMoveResizeWindow (dpy, tmp_win->Parent, cx, cy, tmp_win->attr.width, tmp_win->attr.height);
  /* client window may need to be moved to 0,0 due to win_gravity */
  XMoveResizeWindow (dpy, tmp_win->w, 0, 0, tmp_win->attr.width, tmp_win->attr.height);

  /* 
   * fix up frame and assign size/location values in tmp_win
   */
  frame_mask = (CWX | CWY | CWWidth | CWHeight);
  get_window_geometry (tmp_win, tmp_win->flags & ~ICONIFIED, &frame_wc.x, &frame_wc.y, &frame_wc.width, &frame_wc.height);
  XConfigureWindow (dpy, tmp_win->frame, frame_mask, &frame_wc);

#ifndef NO_TEXTURE
  /* 
   * rebuild titlebar background gradients
   */
  if ((tmp_win->flags & TITLE) && tmp_win->title_width > 2 && tmp_win->title_height > 2)
    {
      /* focused titlebar */
      if (tmp_win->style_focus->texture_type > 0 && tmp_win->style_focus->texture_type < TEXTURE_PIXMAP &&
	  (tmp_win->backPixmap == None || tmp_win->bp_width != tmp_win->title_width || tmp_win->bp_height != tmp_win->title_height))
	{
	  if (tmp_win->backPixmap != None)
	    XFreePixmap (dpy, tmp_win->backPixmap);

	  tmp_win->backPixmap = mystyle_make_pixmap (tmp_win->style_focus, tmp_win->title_width, tmp_win->title_height, None);
	}
      /* unfocused titlebar */
      if (tmp_win->style_unfocus->texture_type > 0 && tmp_win->style_unfocus->texture_type < TEXTURE_PIXMAP &&
	  (tmp_win->backPixmap2 == None || tmp_win->bp_width != tmp_win->title_width || tmp_win->bp_height != tmp_win->title_height))
	{
	  if (tmp_win->backPixmap2 != None)
	    XFreePixmap (dpy, tmp_win->backPixmap2);

	  tmp_win->backPixmap2 = mystyle_make_pixmap (tmp_win->style_unfocus, tmp_win->title_width, tmp_win->title_height, None);
	}
      /* unfocused sticky titlebar */
      if (tmp_win->style_sticky->texture_type > 0 && tmp_win->style_sticky->texture_type < TEXTURE_PIXMAP &&
	  (tmp_win->backPixmap3 == None || tmp_win->bp_width != tmp_win->title_width || tmp_win->bp_height != tmp_win->title_height))
	{
	  if (tmp_win->backPixmap3 != None)
	    XFreePixmap (dpy, tmp_win->backPixmap3);

	  tmp_win->backPixmap3 = mystyle_make_pixmap (tmp_win->style_sticky, tmp_win->title_width, tmp_win->title_height, None);
	}
    }
  if ((tmp_win->flags & (TITLE | BORDER)) && (Moved || Resized))
    SetTransparency (tmp_win);
  tmp_win->bp_height = tmp_win->title_height;
  tmp_win->bp_width = tmp_win->title_width;

  /* redo the frame */
  if ((tmp_win->flags & FRAME) && Resized)
    frame_draw_frame (tmp_win);

#endif /* ! NO_TEXTURE */

#ifdef SHAPE
  if ((Resized) && (tmp_win->wShaped))
    {
      SetShape (tmp_win, w);
    }
#endif /* SHAPE */
  XSync (dpy, 0);
  if (sendEvent)
    {
      client_event.type = ConfigureNotify;
      client_event.xconfigure.display = dpy;
      client_event.xconfigure.event = tmp_win->w;
      client_event.xconfigure.window = tmp_win->w;

      client_event.xconfigure.x = x + cx + tmp_win->bw;
      client_event.xconfigure.y = y + cy + tmp_win->bw;
      client_event.xconfigure.width = tmp_win->attr.width;
      client_event.xconfigure.height = tmp_win->attr.height;
/*
   change 09/25/96
   if (client_event.xconfigure.height<0) 
   client_event.xconfigure.height=1;
 */

      if (client_event.xconfigure.height <= 0)
	client_event.xconfigure.height = tmp_win->hints.height_inc;


      client_event.xconfigure.border_width = tmp_win->bw;
      /* Real ConfigureNotify events say we're above title window, so ... */
      /* what if we don't have a title ????? */
      client_event.xconfigure.above = tmp_win->frame;
      client_event.xconfigure.override_redirect = False;
      XSendEvent (dpy, tmp_win->w, False, StructureNotifyMask, &client_event);
    }
  BroadcastConfig (M_CONFIGURE_WINDOW, tmp_win);
}


/****************************************************************************
 *
 * Sets up the shaped window borders 
 * 
 ****************************************************************************/
void
SetShape (ASWindow * tmp_win, int w)
{
#ifdef SHAPE
  int x, y;

  get_parent_geometry (tmp_win, tmp_win->frame_width, tmp_win->frame_height, &x, &y, NULL, NULL);
  XShapeCombineShape (dpy, tmp_win->frame, ShapeBounding, x + tmp_win->bw, y + tmp_win->bw,
		      tmp_win->w, ShapeBounding, ShapeSet);

  /* windows with titles */
  if (tmp_win->flags & TITLE)
    {
      XRectangle rect;

      rect.x = tmp_win->title_x - tmp_win->bw;
      rect.y = tmp_win->title_y - tmp_win->bw;
      rect.width = tmp_win->title_width + 2 * tmp_win->bw;
      rect.height = tmp_win->title_height + 2 * tmp_win->bw;

      XShapeCombineRectangles (dpy, tmp_win->frame, ShapeBounding,
			       0, 0, &rect, 1, ShapeUnion, Unsorted);
    }

  /* windows with handles */
  if (tmp_win->flags & BORDER)
    {
      XRectangle rect;

      rect.x = tmp_win->boundary_width - tmp_win->bw;
      rect.y = tmp_win->frame_height - tmp_win->boundary_height - tmp_win->boundary_width;
      rect.width = tmp_win->frame_width + 2 * tmp_win->bw;
      rect.height = tmp_win->boundary_height + tmp_win->bw;

      XShapeCombineRectangles (dpy, tmp_win->frame, ShapeBounding,
			       0, 0, &rect, 1, ShapeUnion, Unsorted);
    }

  /* update icon shape */
  if (tmp_win->icon_pixmap_w != None)
    UpdateIconShape (tmp_win);
#endif /* SHAPE */
}

/********************************************************************
 *
 * Sets the input focus to the indicated window.
 *
 **********************************************************************/

Bool
SetFocus (Window w, ASWindow * Fw, Bool circulated)
{
  int i;
  extern Time lastTimestamp;
  Bool focusAccepted = True;

  if (!circulated && Fw)
    SetCirculateSequence (Fw, 1);

  /* ClickToFocus focus queue manipulation */
  if (Fw && Fw != Scr.Focus && Fw != &Scr.ASRoot)
    Fw->focus_sequence = Scr.next_focus_sequence++;

  if (Scr.NumberOfScreens > 1)
    {
      XQueryPointer (dpy, Scr.Root, &JunkRoot, &JunkChild,
		     &JunkX, &JunkY, &JunkX, &JunkY, &JunkMask);
      if (JunkRoot != Scr.Root)
	{
	  if ((Scr.flags & ClickToFocus) && (Scr.Ungrabbed != NULL))
	    {
	      /* Need to grab buttons for focus window */
	      XSync (dpy, 0);
	      for (i = 0; i < MAX_BUTTONS; i++)
		if (Scr.buttons2grab & (1 << i))
		  MyXGrabButton (dpy, i + 1, 0, Scr.Ungrabbed->frame,
				 True, ButtonPressMask, GrabModeSync,
				 GrabModeAsync, None, Scr.ASCursors[SYS]);
	      Scr.Focus = NULL;
	      Scr.Ungrabbed = NULL;
	      XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, lastTimestamp);
	    }
	  return False;
	}
    }
  if (Fw && Fw->focus_var)
    return False;

  if (Fw && (Fw->Desk != Scr.CurrentDesk))
    {
      Fw = NULL;
      w = Scr.NoFocusWin;
      focusAccepted = False;
    }
  if ((Scr.flags & ClickToFocus) && (Scr.Ungrabbed != Fw))
    {
      /* need to grab all buttons for window that we are about to
       * unfocus */
      if (Scr.Ungrabbed != NULL)
	{
	  XSync (dpy, 0);
	  for (i = 0; i < MAX_BUTTONS; i++)
	    if (Scr.buttons2grab & (1 << i))
	      {
		MyXGrabButton (dpy, i + 1, 0, Scr.Ungrabbed->frame,
			       True, ButtonPressMask, GrabModeSync,
			       GrabModeAsync, None, Scr.ASCursors[SYS]);
	      }
	  Scr.Ungrabbed = NULL;
	}
      /* if we do click to focus, remove the grab on mouse events that
       * was made to detect the focus change */
      if ((Scr.flags & ClickToFocus) && (Fw != NULL))
	{
	  for (i = 0; i < MAX_BUTTONS; i++)
	    if (Scr.buttons2grab & (1 << i))
	      {
		MyXUngrabButton (dpy, i + 1, 0, Fw->frame);
	      }
	  Scr.Ungrabbed = Fw;
	}
    }

  /* try to give focus to shaded windows */
  if ((Fw != NULL) && (Fw->flags & SHADED))
    {
      if (Fw->frame != None)
	w = Fw->frame;
      else
	return False;
    }

  /* give focus to icons */
  if (Fw != NULL && (Fw->flags & ICONIFIED))
    {
      if (Fw->icon_title_w != None)
	w = Fw->icon_title_w;
      else if (Fw->icon_pixmap_w != None)
	w = Fw->icon_pixmap_w;
    }

  if (!((Fw) && (Fw->wmhints) && (Fw->wmhints->flags & InputHint) &&
	(Fw->wmhints->input == False)))
    {
      /* Window will accept input focus */
      XSetInputFocus (dpy, w, RevertToParent, lastTimestamp);
      Scr.Focus = Fw;
    }
  else if ((Scr.Focus) && (Scr.Focus->Desk == Scr.CurrentDesk))
    {
      /* Window doesn't want focus. Leave focus alone */
      /* XSetInputFocus (dpy,Scr.Hilite->w , RevertToParent, lastTimestamp); */
      focusAccepted = False;
    }
  else
    {
      XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, lastTimestamp);
      Scr.Focus = NULL;
      focusAccepted = False;
    }
  if ((Fw) && (Fw->flags & DoesWmTakeFocus))
    send_clientmessage (w, _XA_WM_TAKE_FOCUS, lastTimestamp);

  XSync (dpy, 0);

  return focusAccepted;
}

void
set_titlebar_geometry (ASWindow * t)
{
  /* corner geometry is based on the titlebar geometry */
  t->corner_width = 16;

  /* do the titlebar */
  t->title_x = t->boundary_width;
  t->title_y = t->boundary_width;
  t->title_width = 0;
  t->title_height = 0;
  if (t->flags & TITLE)
    {
      /* calculate the titlebar width and height */
      if (t->flags & VERTICAL_TITLE)
	{
	  int i, width = 2;

	  for (i = 0; i < 10; i++)
	    if (width < Scr.button_width[i])
	      width = Scr.button_width[i];

	  /* leave 3 pixel spacing on each side of the buttons */
	  if (Scr.TitleButtonStyle == 0)
	    width += 6;
	  if (width < t->style_focus->font.width + 2)
	    width = t->style_focus->font.width + 2;
	  if (width < t->style_unfocus->font.width + 2)
	    width = t->style_unfocus->font.width + 2;
	  if (width < t->style_sticky->font.width + 2)
	    width = t->style_sticky->font.width + 2;

	  t->title_height = t->attr.height;
	  t->title_width = width;

	  t->corner_width = width;
	  if (t->flags & FRAME)
	    t->title_height += Scr.fs[FR_N].h + Scr.fs[FR_S].h;
	}
      else
	{
	  int i, height = 2;

	  for (i = 0; i < 10; i++)
	    if (height < Scr.button_height[i])
	      height = Scr.button_height[i];

	  /* leave 3 pixel spacing on each side of the buttons */
	  if (Scr.TitleButtonStyle == 0)
	    height += 6;
	  if (height < t->style_focus->font.height + 2)
	    height = t->style_focus->font.height + 2;
	  if (height < t->style_unfocus->font.height + 2)
	    height = t->style_unfocus->font.height + 2;
	  if (height < t->style_sticky->font.height + 2)
	    height = t->style_sticky->font.height + 2;

	  t->title_height = height;
	  t->title_width = t->attr.width;
	  if (t->flags & FRAME)
	    t->title_width += Scr.fs[FR_E].w + Scr.fs[FR_W].w;
	  t->corner_width = height;
	}
    }
}

void
get_parent_geometry (ASWindow * t, int frame_width, int frame_height, int *parent_x_out, int *parent_y_out, int *parent_width_out, int *parent_height_out)
{
  int parent_x, parent_y, parent_width, parent_height;

  /* assume no window decorations */
  parent_x = t->boundary_width - t->bw;
  parent_y = t->boundary_width - t->bw;
  parent_width = frame_width - 2 * t->boundary_width;
  parent_height = frame_height - 2 * t->boundary_width;

  /* do the frame */
  if (t->flags & FRAME)
    {
      parent_x += Scr.fs[FR_W].w;
      parent_y += Scr.fs[FR_N].h;
      parent_width -= Scr.fs[FR_E].w + Scr.fs[FR_W].w;
      parent_height -= Scr.fs[FR_N].h + Scr.fs[FR_S].h;
    }

  /* do the titlebar */
  if (t->flags & TITLE)
    {
      if (t->flags & VERTICAL_TITLE)
	{
	  /* titlebar goes on the left */
	  parent_x += t->title_width + t->bw;
	  parent_width -= t->title_width + t->bw;
	}
      else
	{
	  /* titlebar goes on top */
	  parent_y += t->title_height + t->bw;
	  parent_height -= t->title_height + t->bw;
	}
    }

  /* do the lowbar */
  if (t->flags & BORDER)
    parent_height -= t->boundary_height;

  if (parent_x_out != NULL)
    *parent_x_out = parent_x;
  if (parent_y_out != NULL)
    *parent_y_out = parent_y;
  if (parent_width_out != NULL)
    *parent_width_out = parent_width;
  if (parent_height_out != NULL)
    *parent_height_out = parent_height;
}

void
get_client_geometry (ASWindow * t, int frame_x, int frame_y, int frame_width, int frame_height, int *client_x_out, int *client_y_out, int *client_width_out, int *client_height_out)
{
  int grav_x, grav_y;
  int client_x, client_y, client_width, client_height;

  GetGravityOffsets (t, &grav_x, &grav_y);

  /* assume no window decorations */
  client_x = frame_x + t->boundary_width;
  client_y = frame_y + t->boundary_width;
  client_width = frame_width - 2 * t->boundary_width;
  client_height = frame_height - 2 * t->boundary_width;

  /* do the frame */
  if (t->flags & FRAME)
    {
      if (grav_x == 0)
	client_x += Scr.fs[FR_W].w;
      else if (grav_x == 1)
	client_x += Scr.fs[FR_W].w + Scr.fs[FR_E].w;
      if (grav_y == 0)
	client_y += Scr.fs[FR_N].h;
      else if (grav_y == 1)
	client_y += Scr.fs[FR_N].h + Scr.fs[FR_S].h;
      client_width -= Scr.fs[FR_E].w + Scr.fs[FR_W].w;
      client_height -= Scr.fs[FR_N].h + Scr.fs[FR_S].h;
    }

  /* do the border */
  if (grav_x == 0)
    client_x -= t->old_bw - t->bw;
  else if (grav_x == 1)
    client_x -= 2 * (t->old_bw - t->bw);

  if (grav_y == 0)
    client_y -= t->old_bw - t->bw;
  else if (grav_y == 1)
    client_y -= 2 * (t->old_bw - t->bw);

  /* do the titlebar */
  if (t->flags & TITLE)
    {
      if (t->flags & VERTICAL_TITLE)
	{
	  /* titlebar goes on the left */
	  if (grav_x != -1)
	    client_x += t->title_width + t->bw;
	  client_width -= t->title_width + t->bw;
	}
      else
	{
	  /* titlebar goes on top */
	  if (grav_y != -1)
	    client_y += t->title_height + t->bw;
	  client_height -= t->title_height + t->bw;
	}
    }

  /* do the lowbar */
  if (t->flags & BORDER)
    {
      client_height -= t->boundary_height;
      if (grav_y == 1)
	client_y += t->boundary_height;
    }

  if (client_x_out != NULL)
    *client_x_out = client_x;
  if (client_y_out != NULL)
    *client_y_out = client_y;
  if (client_width_out != NULL)
    *client_width_out = client_width;
  if (client_height_out != NULL)
    *client_height_out = client_height;
}

void
get_frame_geometry (ASWindow * t, int client_x, int client_y, int client_width, int client_height, int *frame_x_out, int *frame_y_out, int *frame_width_out, int *frame_height_out)
{
  int grav_x, grav_y;
  int frame_x, frame_y, frame_width, frame_height;

  GetGravityOffsets (t, &grav_x, &grav_y);

  /* assume no window decorations */
  frame_x = client_x - t->boundary_width;
  frame_y = client_y - t->boundary_width;
  frame_width = client_width + 2 * t->boundary_width;
  frame_height = client_height + 2 * t->boundary_width;

  /* do the frame */
  if (t->flags & FRAME)
    {
      if (grav_x == 0)
	frame_x -= Scr.fs[FR_W].w;
      else if (grav_x == 1)
	frame_x -= Scr.fs[FR_W].w + Scr.fs[FR_E].w;
      if (grav_y == 0)
	frame_y -= Scr.fs[FR_N].h;
      else if (grav_y == 1)
	frame_y -= Scr.fs[FR_N].h + Scr.fs[FR_S].h;
      frame_width += Scr.fs[FR_W].w + Scr.fs[FR_E].w;
      frame_height += Scr.fs[FR_N].h + Scr.fs[FR_S].h;
    }

  /* do the border */
  if (grav_x == 0)
    frame_x += t->old_bw - t->bw;
  else if (grav_x == 1)
    frame_x += 2 * (t->old_bw - t->bw);

  if (grav_y == 0)
    frame_y += t->old_bw - t->bw;
  else if (grav_y == 1)
    frame_y += 2 * (t->old_bw - t->bw);

  /* do the titlebar */
  if (t->flags & TITLE)
    {
      if (t->flags & VERTICAL_TITLE)
	{
	  /* titlebar goes on the left */
	  if (grav_x != -1)
	    frame_x -= t->title_width + t->bw;
	  frame_width += t->title_width + t->bw;
	}
      else
	{
	  /* titlebar goes on top */
	  if (grav_y != -1)
	    frame_y -= t->title_height + t->bw;
	  frame_height += t->title_height + t->bw;
	}
    }

  /* do the lowbar */
  if (t->flags & BORDER)
    {
      frame_height += t->boundary_height;
      if (grav_y == 1)
	frame_y -= t->boundary_height;
    }

  if (frame_x_out != NULL)
    *frame_x_out = frame_x;
  if (frame_y_out != NULL)
    *frame_y_out = frame_y;
  if (frame_width_out != NULL)
    *frame_width_out = frame_width;
  if (frame_height_out != NULL)
    *frame_height_out = frame_height;
}

/*
   ** the resize geometry conversion is from the client's current position 
   ** (ie, what XTranslateCoordinates(dpy, t->w, Root, 0, 0, ...) would 
   ** produce) to frame coordinates; this is the same as get_frame_geometry() 
   ** using CenterGravity and ignoring the old border width
 */
void
get_resize_geometry (ASWindow * t, int client_x, int client_y, int client_width, int client_height, int *frame_x_out, int *frame_y_out, int *frame_width_out, int *frame_height_out)
{
  int frame_x, frame_y, frame_width, frame_height;

  /* assume no window decorations */
  frame_x = client_x - t->boundary_width;
  frame_y = client_y - t->boundary_width;
  frame_width = client_width + 2 * t->boundary_width;
  frame_height = client_height + 2 * t->boundary_width;

  /* do the frame */
  if (t->flags & FRAME)
    {
      frame_x -= Scr.fs[FR_W].w;
      frame_y -= Scr.fs[FR_N].h;
      frame_width += Scr.fs[FR_W].w + Scr.fs[FR_E].w;
      frame_height += Scr.fs[FR_N].h + Scr.fs[FR_S].h;
    }

  /* do the border */
  frame_x -= t->bw;
  frame_y -= t->bw;

  /* do the titlebar */
  if (t->flags & TITLE)
    {
      if (t->flags & VERTICAL_TITLE)
	{
	  /* titlebar goes on the left */
	  frame_x -= t->title_width + t->bw;
	  frame_width += t->title_width + t->bw;
	}
      else
	{
	  /* titlebar goes on top */
	  frame_y -= t->title_height + t->bw;
	  frame_height += t->title_height + t->bw;
	}
    }

  /* do the lowbar */
  if (t->flags & BORDER)
    frame_height += t->boundary_height;

  if (frame_x_out != NULL)
    *frame_x_out = frame_x;
  if (frame_y_out != NULL)
    *frame_y_out = frame_y;
  if (frame_width_out != NULL)
    *frame_width_out = frame_width;
  if (frame_height_out != NULL)
    *frame_height_out = frame_height;
}
