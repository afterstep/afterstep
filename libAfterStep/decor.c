/*
 * Copyright (C) 2002 Sasha Vasko <sasha at aftercode.net>
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

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "../configure.h"

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/style.h"
#include "../include/mystyle.h"
#include "../include/misc.h"
#include "../include/screen.h"
#include "../libAfterImage/afterimage.h"
#include "../include/decor.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

static GC     MaskGC = None;

/********************************************************************/
/* ASCanvas :                                                       */
/********************************************************************/
static Bool
refresh_canvas_config (ASCanvas * pc)
{
	Bool          changed = False;

	if (pc && pc->w != None)
	{
		int           root_x = pc->root_x, root_y = pc->root_y;
		int           x, y;
		unsigned int  width = pc->width, height = pc->height, udumm;
		Window        wdumm;

		XTranslateCoordinates (dpy, pc->w, Scr.Root, 0, 0, &root_x, &root_y, &wdumm);
		XGetGeometry (dpy, pc->w, &wdumm, &x, &y, &width, &height, &udumm, &udumm);
		changed = (root_x != pc->root_x || root_y != pc->root_y);
		pc->root_x = root_x;
		pc->root_y = root_y;
		if (width != pc->width || height != pc->height)
		{
			changed = True;
			if (pc->canvas)
			{
				XFreePixmap (dpy, pc->canvas);
				pc->canvas = None;
			}
			if (pc->mask)
			{
				XFreePixmap (dpy, pc->mask);
				pc->mask = None;
			}
			set_flags (pc->state, CANVAS_DIRTY | CANVAS_OUT_OF_SYNC);
			pc->width = width;
			pc->height = height;
		}
	}
	return changed;
}

Pixmap
get_canvas_canvas (ASCanvas * pc)
{
	if (pc == NULL)
		return None;
	if (pc->canvas == None)
	{
		pc->canvas = create_visual_pixmap (Scr.asv, Scr.Root, pc->width, pc->height, 0);
		set_flags (pc->state, CANVAS_DIRTY | CANVAS_OUT_OF_SYNC);
	}
	return pc->canvas;
}

Pixmap
get_canvas_mask (ASCanvas * pc)
{
	if (pc == NULL)
		return None;

	if (pc->mask == None)
	{
		pc->mask = create_visual_pixmap (Scr.asv, Scr.Root, pc->width, pc->height, 1);
		if (pc->mask)
		{
			if (MaskGC == None)
			{
				XGCValues     gcv;

				gcv.foreground = 0xFFFFFFFF;
				gcv.background = 0;

				MaskGC = XCreateGC (dpy, pc->mask, GCForeground | GCBackground, &gcv);
			}
			XFillRectangle (dpy, pc->mask, MaskGC, 0, 0, pc->width, pc->height);
			set_flags (pc->state, CANVAS_DIRTY | CANVAS_MASK_OUT_OF_SYNC);
		}
	}
	return pc->mask;
}

ASCanvas     *
create_ascanvas (Window w)
{
	ASCanvas     *pc = NULL;

	if (w)
	{
		pc = safecalloc (1, sizeof (ASCanvas));
		pc->w = w;
		refresh_canvas_config (pc);
	}
	return pc;
}

void
destroy_ascanvas (ASCanvas ** pcanvas)
{
	if (pcanvas)
	{
		ASCanvas     *pc = *pcanvas;

		if (pc)
		{
			if (pc->canvas)
				XFreePixmap (dpy, pc->canvas);
			if (pc->mask)
				XFreePixmap (dpy, pc->mask);
			memset (pc, 0x00, sizeof (ASCanvas));
			free (pc);
		}
		*pcanvas = NULL;
	}
}

Bool
handle_canvas_config (ASCanvas * canvas)
{
	return refresh_canvas_config (canvas);
}

Bool
make_canvas_rectangle (ASCanvas * pc, ASImage * im, int x, int y, int *cx, int *cy, int *cwidth, int *cheight)
{
	*cwidth = im->width;
	*cheight = im->height;
	*cx = x;
	*cy = y;
	if (x + *cwidth <= 0 || x > pc->width)
		return False;
	if (y + *cheight <= 0 || y > pc->height)
		return False;

	if (*cx < 0)
	{
		*cwidth += *cx;
		*cx = 0;
	}
	if (*cx + *cwidth > pc->width)
		*cwidth = pc->width - *cx;
	if (*cy < 0)
	{
		*cheight += *cy;
		*cy = 0;
	}
	if (*cy + *cheight > pc->height)
		*cheight = pc->height - *cy;
	return True;
}


Bool
draw_canvas_image (ASCanvas * pc, ASImage * im, int x, int y)
{
	Pixmap        p;
	int           real_x, real_y;
	int           width, height;

	if (im == NULL || pc == NULL)
		return False;

	if ((p = get_canvas_canvas (pc)) == None)
		return False;

	if (!make_canvas_rectangle (pc, im, x, y, &real_x, &real_y, &width, &height))
		return False;

	LOCAL_DEBUG_OUT ("drawing image %dx%d at %dx%d%+d%+d", im->width, im->height, width, height, real_x, real_y);
	if (asimage2drawable (Scr.asv, p, im, NULL, real_x - x, real_y - y, real_x, real_y, width, height, True))
	{
		set_flags (pc->state, CANVAS_OUT_OF_SYNC);
		return True;
	}
	return False;
}


Bool
draw_canvas_mask (ASCanvas * pc, ASImage * im, int x, int y)
{
	Pixmap        mask;
	int           real_x, real_y;
	int           width, height;

	if (im == NULL || pc == NULL)
		return False;

	if ((mask = get_canvas_mask (pc)) == None)
		return False;

	if (!make_canvas_rectangle (pc, im, x, y, &real_x, &real_y, &width, &height))
		return False;
	LOCAL_DEBUG_OUT ("drawing mask %dx%d at %dx%d%+d%+d", im->width, im->height, width, height, real_x, real_y);
	if (asimage2alpha_drawable (Scr.asv, mask, im, MaskGC, real_x - x, real_y - y, real_x, real_y, width, height, True))
	{
		set_flags (pc->state, CANVAS_MASK_OUT_OF_SYNC);
		return True;
	}
	return False;
}

void
fill_canvas_mask (ASCanvas * pc, int win_x, int win_y, int width, int height, int val)
{
	int           real_x, real_y;
	int           real_width, real_height;

	if (pc->mask != None)
	{
		real_x = MAX (win_x, 0);
		real_y = MAX (win_y, 0);
		real_width = width - (real_x - win_x);
		real_height = height - (real_y - win_y);
		if (real_width > 0 && real_height > 0 && real_x < pc->width && real_y < pc->height)
		{
			if (real_x + real_width > pc->width)
				real_width = pc->width - real_x;
			if (real_y + real_height > pc->height)
				real_height = pc->height - real_y;
			XFillRectangle (dpy, pc->mask, MaskGC, real_x, real_y, real_width, real_height);
		}
	}
}

void
update_canvas_display (ASCanvas * pc)
{
	if (pc && pc->w != None)
	{
		if (pc->canvas)
		{
#ifdef SHAPE
			if (pc->mask)
				XShapeCombineMask (dpy, pc->w, ShapeBounding, 0, 0, pc->mask, ShapeSet);
#endif
			XSetWindowBackgroundPixmap (dpy, pc->w, pc->canvas);
			XClearWindow (dpy, pc->w);

			XSync (dpy, False);
			clear_flags (pc->state, CANVAS_DIRTY | CANVAS_OUT_OF_SYNC);
		}
		if (pc->mask)
		{
			/* todo: add shaped windows support here : */
		}
	}
}

void
resize_canvas (ASCanvas * pc, unsigned int width, unsigned int height)
{
	/* Setting background to None to avoid background pixmap tiling 
	 * while resizing */
	if (pc->width < width || pc->height < height)
		XSetWindowBackgroundPixmap (dpy, pc->w, None);
	XResizeWindow (dpy, pc->w, width, height);
}

/********************************************************************/
/* ASTBarData :                                                     */
/********************************************************************/
ASTBarData   *
create_astbar ()
{
	ASTBarData   *tbar = safecalloc (1, sizeof (ASTBarData));

	tbar->rendered_root_x = tbar->rendered_root_y = 0xFFFFFFFF;
	return tbar;
}

void
destroy_astbar (ASTBarData ** ptbar)
{
	if (ptbar)
		if (*ptbar)
		{
			ASTBarData   *tbar = *ptbar;
			register int  i;

			for (i = 0; i < BAR_STATE_NUM; ++i)
			{
				if (tbar->back[i])
					destroy_asimage (&(tbar->back[i]));
				if (tbar->label[i])
					destroy_asimage (&(tbar->label[i]));
			}
			if (tbar->left_shape)
				destroy_asimage (&(tbar->left_shape));
			if (tbar->center_shape)
				destroy_asimage (&(tbar->center_shape));
			if (tbar->right_shape)
				destroy_asimage (&(tbar->right_shape));

			memset (tbar, 0x00, sizeof (ASTBarData));
		}
}

unsigned int
get_astbar_label_width (ASTBarData * tbar)
{
	int           size[2] = { 1, 1 };
	int           i;

	if (tbar == NULL)
		return 0;
	for (i = 0; i < 2; ++i)
	{
		if (tbar->label[i] == NULL)
			tbar->label[i] = mystyle_draw_text_image (tbar->style[i], tbar->label_text);

		if (tbar->label[i])
			size[0] = tbar->label[i]->width;
	}
	return MAX (size[0], size[1]);
}

unsigned int
get_astbar_label_height (ASTBarData * tbar)
{
	int           size[2] = { 1, 1 };
	int           i;

	if (tbar == NULL)
		return 0;
	for (i = 0; i < 2; ++i)
		size[i] = mystyle_get_font_height (tbar->style[i]);
	return MAX (size[0], size[1]);
}

unsigned int
calculate_astbar_height (ASTBarData * tbar)
{
	int           size = 0;

	if (tbar == NULL)
		return 0;
	size = get_astbar_label_height (tbar);
	size += tbar->top_bevel + 2 + tbar->bottom_bevel + 2;
	return size;
}

unsigned int
calculate_astbar_width (ASTBarData * tbar)
{
	int           size = 0;

	if (tbar == NULL)
		return 0;
	size = get_astbar_label_width (tbar);
	size += tbar->left_bevel + 2 + tbar->right_bevel + 2;
	return size;
}

Bool
set_astbar_size (ASTBarData * tbar, unsigned int width, unsigned int height)
{
	Bool          changed = False;

	if (tbar)
	{
		unsigned int  w = (width > 0) ? width : 1;
		unsigned int  h = (height > 0) ? height : 1;

		changed = (w != tbar->width || h != tbar->height);
		LOCAL_DEBUG_CALLER_OUT ("resizing TBAR %p from %dx%d to %dx%d", tbar, tbar->width, tbar->height, w, h);
		tbar->width = w;
		tbar->height = h;
		if (changed)
		{
			register int  i;

			for (i = 0; i < BAR_STATE_NUM; ++i)
				if (tbar->back[i])
					destroy_asimage (&(tbar->back[i]));
		}
	}
	return changed;
}

#define ASTBAR_HILITE	(BOTTOM_HILITE|RIGHT_HILITE)

static void
update_astbar_bevel_size (ASTBarData * tbar)
{
	ASImageBevel  bevel;
	int           i;

	tbar->left_bevel = tbar->top_bevel = tbar->right_bevel = tbar->bottom_bevel = 0;
	for (i = 0; i < BAR_STATE_NUM; ++i)
		if (tbar->style[i])
		{
			mystyle_make_bevel (tbar->style[i], &bevel, ASTBAR_HILITE, False);
			if (tbar->left_bevel < bevel.left_outline)
				tbar->left_bevel = bevel.left_outline;
			if (tbar->top_bevel < bevel.top_outline)
				tbar->top_bevel = bevel.top_outline;
			if (tbar->right_bevel < bevel.right_outline)
				tbar->right_bevel = bevel.right_outline;
			if (tbar->bottom_bevel < bevel.bottom_outline)
				tbar->bottom_bevel = bevel.bottom_outline;
		}
}

Bool
set_astbar_style (ASTBarData * tbar, unsigned int state, const char *style_name)
{
	Bool          changed = False;

	if (tbar && state < BAR_STATE_NUM)
	{
		MyStyle      *style = mystyle_find_or_default (style_name);

		changed = (style != tbar->style[state]);
		tbar->style[state] = style;
		if (changed && tbar->back[state])
		{
			destroy_asimage (&(tbar->back[state]));
			tbar->back[state] = NULL;
		}
		update_astbar_bevel_size (tbar);
	}
	return changed;
}

Bool
set_astbar_label (ASTBarData * tbar, const char *label)
{
	Bool          changed = False;

	if (tbar)
	{
		if (label == NULL)
		{
			if ((changed = (tbar->label_text != NULL)))
			{
				free (tbar->label_text);
				tbar->label_text = NULL;
			}
		} else if (tbar->label_text == NULL)
		{
			changed = True;
			tbar->label_text = mystrdup (label);
		} else if ((changed = (strcmp (tbar->label_text, label) != 0)))
		{
			free (tbar->label_text);
			tbar->label_text = mystrdup (label);
		}
		if (changed)
		{
			register int  i;

			for (i = 0; i < BAR_STATE_NUM; ++i)
			{
				if (tbar->label[i])
				{
					destroy_asimage (&(tbar->label[i]));
					tbar->label[i] = NULL;
				}
			}
		}
	}
	return changed;
}

Bool
move_astbar (ASTBarData * tbar, ASCanvas * pc, int win_x, int win_y)
{
	Bool          changed = False;

	if (tbar && pc)
	{
		int           root_x = pc->root_x + win_x, root_y = pc->root_y + win_y;

		changed = (root_x != tbar->root_x || root_y != tbar->root_y);
		tbar->root_x = root_x;
		tbar->root_y = root_y;
		changed = changed || (win_x != tbar->win_x || win_y != tbar->win_y);
		tbar->win_x = win_x;
		tbar->win_y = win_y;
	}
	return changed;
}

Bool
render_astbar (ASTBarData * tbar, ASCanvas * pc)
{
	ASImage      *back, *label_im;
	MyStyle      *style;
	ASImageBevel  bevel;
	ASImageLayer  layers[2];
	ASImage      *merged_im;
	int           state;
	ASAltImFormats fmt = ASA_XImage;

	/* input control : */
	if (tbar == NULL || pc == NULL || pc->w == None)
		return False;
	state = get_flags (tbar->state, BAR_STATE_FOCUS_MASK);
	if ((style = tbar->style[state]) == NULL)
		return False;
	/* validating our images : */
	if ((back = tbar->back[state]) != NULL)
	{
		if (back->width != tbar->width || back->height != tbar->height ||
			((tbar->rendered_root_x != tbar->root_x || tbar->rendered_root_y != tbar->root_y) &&
			 style->texture_type > TEXTURE_PIXMAP))
		{
			destroy_asimage (&(tbar->back[state]));
			tbar->back[state] = back = NULL;
		}
	}
	if (back == NULL)
	{
		tbar->back[state] = back = mystyle_make_image (style, tbar->root_x, tbar->root_y, tbar->width, tbar->height);

		if (back == NULL)
			return False;
	}

	if ((label_im = tbar->label[state]) == NULL && tbar->label_text != NULL)
	{
		label_im = mystyle_draw_text_image (style, tbar->label_text);
		tbar->label[state] = label_im;
	}

	mystyle_make_bevel (style, &bevel, ASTBAR_HILITE, get_flags (tbar->state, BAR_STATE_PRESSED_MASK));
	/* in unfocused and unpressed state we render pixmap and set 
	 * window's background to it 
	 * in focused state or in pressed state we render to 
	 * the window directly, and we'll need to be handling the expose
	 * events
	 */
	init_image_layers (&layers[0], 2);
	layers[0].im = back;
	layers[0].bevel = &bevel;
	layers[0].clip_width = tbar->width - (tbar->left_bevel + tbar->right_bevel);
	layers[0].clip_height = tbar->height - (tbar->top_bevel + tbar->bottom_bevel);
	layers[1].im = label_im;
	layers[1].dst_x = tbar->left_bevel + 2;
	layers[1].dst_y = tbar->right_bevel + 2;
	layers[1].clip_width = label_im ? label_im->width : tbar->width;
	layers[1].clip_height = label_im ? label_im->height : tbar->height;

	LOCAL_DEBUG_CALLER_OUT ("MERGING TBAR %p image %dx%d from %p %dx%d and %p %dx%d",
							tbar, tbar->width, tbar->height,
							back, back ? back->width : -1, back ? back->height : -1,
							label_im, label_im ? label_im->width : -1, label_im ? label_im->height : -1);

#ifdef SHAPE
	if (style->texture_type == TEXTURE_SHAPED_PIXMAP || style->texture_type == TEXTURE_SHAPED_SCALED_PIXMAP)
		fmt = ASA_ASImage;
	else if (pc->mask)
		fill_canvas_mask (pc, tbar->win_x, tbar->win_y, tbar->width, tbar->height, 1);
#endif
	merged_im = merge_layers (Scr.asv, &layers[0], 2, tbar->width, tbar->height, fmt, 0, ASIMAGE_QUALITY_DEFAULT);

	if (merged_im)
	{
		Bool          res = draw_canvas_image (pc, merged_im, tbar->win_x, tbar->win_y);

#ifdef SHAPE
		if (style->texture_type == TEXTURE_SHAPED_PIXMAP || style->texture_type == TEXTURE_SHAPED_SCALED_PIXMAP)
			draw_canvas_mask (pc, merged_im, tbar->win_x, tbar->win_y);
#endif
		destroy_asimage (&merged_im);
		return res;
	}

	return False;
}

Bool
set_astbar_focused (ASTBarData * tbar, ASCanvas * pc, Bool focused)
{
	if (tbar)
	{
		Bool          old_focused = get_flags (tbar->state, BAR_STATE_FOCUS_MASK);

		if (focused)
			set_flags (tbar->state, BAR_STATE_FOCUSED);
		else
			clear_flags (tbar->state, BAR_STATE_FOCUSED);
		return old_focused;
	}
	return False;
}

Bool
set_astbar_pressed (ASTBarData * tbar, ASCanvas * pc, Bool pressed)
{
	if (tbar)
	{
		Bool          old_pressed = get_flags (tbar->state, BAR_STATE_PRESSED_MASK);

		if (pressed)
			set_flags (tbar->state, BAR_STATE_PRESSED);
		else
			clear_flags (tbar->state, BAR_STATE_PRESSED);
		return old_pressed;
	}
	return False;
}

void
update_astbar_transparency (ASTBarData * tbar, ASCanvas * pc)
{
	int           root_x, root_y;
	Bool          changed = False;;

	if (tbar == NULL || pc == NULL)
		return;

	root_x = pc->root_x + tbar->win_x;
	root_y = pc->root_y + tbar->win_y;
	if ((changed = (root_x != tbar->root_x || root_y != tbar->root_y)))
	{
		tbar->root_x = pc->root_x + tbar->win_x;
		tbar->root_y = pc->root_y + tbar->win_y;
		changed = True;
	}

	if (changed)
	{
		register int  i = BAR_STATE_NUM;

		while (--i >= 0)
		{
			if (tbar->style[i] && tbar->style[i]->texture_type >= TEXTURE_TRANSPARENT)
				if (tbar->back[i])
					destroy_asimage (&(tbar->back[i]));
		}
		render_astbar (tbar, pc);
	}
}
