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

#include "../configure.h"

#include "../include/asapp.h"
#include "../include/afterstep.h"
#include "../include/mystyle.h"
#include "../include/screen.h"
#include "../libAfterImage/afterimage.h"
#include "../include/myicon.h"
#include "../include/decor.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

int    _as_frame_corner_xref[FRAME_SIDES+1] = {FR_NW, FR_NE, FR_SE, FR_SW, FR_NW};

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
            if (pc->mask && !get_flags( pc->state, CANVAS_CONTAINER ))
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
    if( get_flags( pc->state, CANVAS_CONTAINER ) )
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

    if (pc->mask == None && !get_flags( pc->state, CANVAS_CONTAINER ))
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

ASCanvas     *
create_ascanvas_container (Window w)
{
	ASCanvas     *pc = NULL;

	if (w)
	{
		pc = safecalloc (1, sizeof (ASCanvas));
		pc->w = w;
        pc->state = CANVAS_CONTAINER ;
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
            if (pc->mask && !get_flags( pc->state, CANVAS_CONTAINER ))
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

    if ((mask = get_canvas_mask (pc)) == None || get_flags( pc->state, CANVAS_CONTAINER ))
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

    if (pc->mask != None && !get_flags( pc->state, CANVAS_CONTAINER ))
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
LOCAL_DEBUG_CALLER_OUT( "canvas(%p)->window(%lx)->canvas_pixmap(%lx)", pc, pc->w, pc->canvas );
    if (pc && pc->w != None && !get_flags( pc->state, CANVAS_CONTAINER ))
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
    if ((pc->width < width || pc->height < height) && !get_flags( pc->state, CANVAS_CONTAINER ))
		XSetWindowBackgroundPixmap (dpy, pc->w, None);
	XResizeWindow (dpy, pc->w, width, height);
}

void
moveresize_canvas (ASCanvas * pc, int x, int y, unsigned int width, unsigned int height)
{
LOCAL_DEBUG_CALLER_OUT( "canvas(%p)->window(%lx)->geom(%ux%u%+d%+d)", pc, pc->w, width, height, x, y );
    /* Setting background to None to avoid background pixmap tiling
	 * while resizing */
    if ((pc->width < width || pc->height < height) && !get_flags( pc->state, CANVAS_CONTAINER ))
		XSetWindowBackgroundPixmap (dpy, pc->w, None);
    XMoveResizeWindow (dpy, pc->w, x, y, width, height);
}

void
unmap_canvas_window( ASCanvas *pc )
{
    if( pc && pc->w != None )
        XUnmapWindow( dpy, pc->w );
}

/********************************************************************/
/* ASTBtnData :                                                     */
/********************************************************************/
ASTBtnData*
create_astbtn()
{
    return safecalloc( 1, sizeof(ASTBtnData) );
}

static void
free_tbtn_images( ASTBtnData* btn )
{
    if( btn->pressed )
    {
        safe_asimage_destroy (btn->pressed);
        btn->pressed = NULL ;
    }

    if( btn->unpressed )
    {
        safe_asimage_destroy (btn->unpressed);
        btn->unpressed = NULL ;
    }
    btn->current = NULL ;
}

void
set_tbtn_images( ASTBtnData* btn, struct button_t *from )
{
    Bool pressed ;
    if( AS_ASSERT(btn) || from == NULL )
        return ;

    pressed = (btn->current == btn->pressed );
    free_tbtn_images( btn );

    btn->pressed   = from->pressed.image ;
    btn->unpressed = from->unpressed.image ;
    btn->current   = (pressed && btn->pressed)? btn->pressed : btn->unpressed ;
    btn->width = from->width ;
    btn->height = from->height ;
}

ASTBtnData*
make_tbtn( struct button_t *from )
{
    ASTBtnData* btn = NULL ;
    if( from )
    {
        btn = safecalloc( 1, sizeof( ASTBtnData ) );
        set_tbtn_images( btn, from );
    }
    return btn;
}

void
destroy_astbtn(ASTBtnData **ptbtn )
{
    if( !AS_ASSERT(ptbtn) )
    {
        ASTBtnData *btn = *ptbtn ;
        if( btn )
        {
            free_tbtn_images( btn );
            memset( btn, 0x00, sizeof(ASTBtnData ));
            free( btn );
        }
        *ptbtn = NULL ;
    }
}

/********************************************************************/
/* ASTBtnBlock :                                                    */
/********************************************************************/
ASTBtnBlock*
create_astbtn_block( unsigned int btn_count )
{
    ASTBtnBlock* blk = safecalloc( 1, sizeof(ASTBtnBlock));
    if( btn_count > 0 )
    {
        blk->buttons = safecalloc( btn_count, sizeof(ASTBtnData));
        blk->count = btn_count ;
    }
    return blk ;
}


ASTBtnBlock*
build_tbtn_block( struct button_t *from_list, ASFlagType mask, unsigned int count, int spacing, int order )
{
    ASTBtnBlock *blk = NULL ;
    unsigned int real_count = 0 ;
    unsigned short max_width = 0, max_height = 0 ;
    register int i = count ;
    if( count > 0 )
        if( !AS_ASSERT( from_list ) )
            while( --i >= 0 )
                if( (mask&(0x01<<i)) != 0 && (from_list[i].unpressed.image || from_list[i].pressed.image))
                {
                    ++real_count ;
                    if( from_list[i].width > max_width )
                        max_width = from_list[i].width ;
                    if( from_list[i].height > max_height )
                        max_height = from_list[i].height ;
                }
    if( real_count > 0 )
    {
        int k = real_count-1 ;
        int pos = 0 ;

        blk = create_astbtn_block( real_count );

        i = count-1 ;
        while( i >= 0 && k >= 0 )
        {
            if( (mask&(0x01<<i)) != 0 && (from_list[i].unpressed.image || from_list[i].pressed.image))
            {
                set_tbtn_images( &(blk->buttons[k]), &(from_list[i]) );
                blk->buttons[k].context = i ;
                --k ;
            }
            --i ;
        }

        k = get_flags(order, TBTN_ORDER_REVERSE)?real_count - 1:0;
        while( k >= 0 && k < real_count )
        {
            if( get_flags(order, TBTN_ORDER_VERTICAL) )
            {
                blk->buttons[k].x = (max_width - blk->buttons[k].width)>>1 ;
                if( get_flags(order, TBTN_ORDER_REVERSE) )
                {
                    pos -= blk->buttons[k].height ;
                    blk->buttons[k].y = pos ;
                    pos -= spacing ;
                    --k ;
                }else
                {
                    blk->buttons[k].y = pos ;
                    pos += blk->buttons[k].height ;
                    pos += spacing ;
                    ++k ;
                }
            }else
            {
                blk->buttons[k].y = (max_height - blk->buttons[k].height)>>1 ;
                if( get_flags(order, TBTN_ORDER_REVERSE) )
                {
                    pos -= blk->buttons[k].width ;
                    blk->buttons[k].x = pos ;
                    pos -= spacing ;
                    --k ;
                }else
                {
                    blk->buttons[k].x = pos ;
                    pos += blk->buttons[k].width ;
                    pos += spacing ;
                    ++k ;
                }
            }
        }
    }
    return blk ;
}

int
check_tbtn_point( ASTBtnBlock *bb, int x, int y )
{
    if( bb )
    {
        register int i = bb->count ;
        while( --i >= 0 )
        {
            register ASTBtnData *btn = &(bb->buttons[i]) ;
            int tmp = x - btn->x ;
            if( tmp >= 0 && tmp < btn->width )
            {
                tmp = y - btn->y ;
                if( tmp >= 0 && tmp < btn->height )
                    return btn->context;
            }
        }
    }
    return C_NO_CONTEXT;
}

void
destroy_astbtn_block(ASTBtnBlock **pb )
{
    if( AS_ASSERT(pb) )
    {
        ASTBtnBlock *blk = *pb ;
        if( blk )
        {
            register int i = blk->count ;
            while( --i >= 0 )
                free_tbtn_images( &(blk->buttons[i]) );

            free( blk->buttons );
            memset( blk, 0x00, sizeof(ASTBtnBlock ) );
            free( blk );
            *pb = NULL ;
        }
    }
}

/********************************************************************/
/* ASTBarData :                                                     */
/********************************************************************/
ASTBarData   *
create_astbar ()
{
	ASTBarData   *tbar = safecalloc (1, sizeof (ASTBarData));

    tbar->rendered_root_x = tbar->rendered_root_y = 0xFFFF;
	return tbar;
}

static inline void
flush_tbar_backs(ASTBarData *tbar)
{
    register int i;
    for (i = 0; i < BAR_STATE_NUM; ++i)
        if (tbar->back[i])
            destroy_asimage (&(tbar->back[i]));
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
            if (tbar->left_buttons)
                destroy_astbtn_block (&(tbar->left_buttons));
            if (tbar->right_buttons)
                destroy_astbtn_block (&(tbar->right_buttons));

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
set_astbar_image( ASTBarData *tbar, ASImage *image )
{
    if( tbar )
        if( tbar->back_image != image )
        {
            if( tbar->back_image )
            {
                safe_asimage_destroy( tbar->back_image );
                tbar->back_image = NULL ;
            }
            if( image )
                if( (tbar->back_image = dup_asimage( image )) == NULL )
                    tbar->back_image = clone_asimage( image, 0xFFFFFFFF );

            flush_tbar_backs(tbar);

            return True;
        }
    return False;
}

Bool
set_astbar_back_size( ASTBarData *tbar, unsigned short width, unsigned short height )
{
    if( tbar )
        if ( width != tbar->back_width || height != tbar->back_height )
        {
            tbar->back_width = width ;
            tbar->back_height = height ;
            flush_tbar_backs(tbar);

            return True;
        }
    return False;
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
set_astbar_btns( ASTBarData *tbar, ASTBtnBlock **btns, Bool left )
{
    Bool          changed = False;
    ASTBtnBlock  **trg ;
    if( tbar && btns )
    {
        trg = left?&(tbar->left_buttons):&(tbar->right_buttons);
        if( (changed = (*trg != *btns )) )
        {
            if( *trg )
                destroy_astbtn_block( trg );
            *trg = *btns ;
        }
        *btns = NULL ;
    }else if ( btns && *btns )
        destroy_astbtn_block( btns );
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
        int          old_focused = get_flags (tbar->state, BAR_STATE_FOCUS_MASK)?1:0;

		if (focused)
			set_flags (tbar->state, BAR_STATE_FOCUSED);
		else
			clear_flags (tbar->state, BAR_STATE_FOCUSED);
        return ((focused?1:0)!=old_focused);
	}
	return False;
}

Bool
set_astbar_pressed (ASTBarData * tbar, ASCanvas * pc, Bool pressed)
{
	if (tbar)
	{
        Bool          old_pressed = get_flags (tbar->state, BAR_STATE_PRESSED_MASK)?1:0;

		if (pressed)
			set_flags (tbar->state, BAR_STATE_PRESSED);
		else
			clear_flags (tbar->state, BAR_STATE_PRESSED);
        return ((pressed?1:0)!=old_pressed);
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

int
check_astbar_point( ASTBarData *tbar, int root_x, int root_y )
{
    int context = C_NO_CONTEXT ;
    if( tbar )
    {
        root_x -= tbar->root_x ;
        root_y -= tbar->root_y ;
        if(  0 <= root_x && tbar->width  > root_x &&
             0 <= root_y && tbar->height > root_y )
        {
            int btn_context ;
            context = tbar->context ;
            if( (btn_context = check_tbtn_point( tbar->left_buttons, root_x, root_y )) != C_NO_CONTEXT )
                context = btn_context ;
            else if( (btn_context = check_tbtn_point( tbar->right_buttons, root_x, root_y )) != C_NO_CONTEXT )
                context = btn_context ;
        }
    }
    return context;
}

/*************************************************************************/
/*************************************************************************/
/* MyFrame management :                                                  */
/*************************************************************************/
MyFrame *
create_myframe()
{
    MyFrame *frame = safecalloc( 1, sizeof(MyFrame));
    frame->magic = MAGIC_MYFRAME ;

    return frame;

}

MyFrame *
create_default_myframe()
{
    MyFrame *frame = create_myframe();
    frame->flags = C_SIDEBAR ;
    frame->part_width[FR_S] = BOUNDARY_WIDTH ;
    frame->part_width[FR_SW] = CORNER_WIDTH ;
    frame->part_width[FR_SE] = CORNER_WIDTH ;
    frame->part_length[FR_S] = 1;
    frame->part_length[FR_SW] = BOUNDARY_WIDTH ;
    frame->part_length[FR_SE] = BOUNDARY_WIDTH ;
    frame->spacing = 1;
    return frame ;
}

MyFrame *
myframe_find( const char *name )
{
    MyFrame *frame = Scr.Look.DefaultFrame ;
    if( name && Scr.Look.FramesList )
        if( get_hash_item( Scr.Look.FramesList, AS_HASHABLE(name), (void**)&frame) != ASH_Success )
            frame = Scr.Look.DefaultFrame ;
    return frame ;
}

void
myframe_load ( MyFrame * frame, ASImageManager *imman )
{
	register int  i;

	if (frame == NULL)
		return;
	for (i = 0; i < FRAME_PARTS; i++)
        if( frame->part_filenames[i] )
        {
            frame->parts[i] = safecalloc( 1, sizeof(icon_t));
            if( !load_icon (frame->parts[i], frame->part_filenames[i], imman) )
            {
                free( frame->parts[i] );
                frame->parts[i] = NULL;
            }
        }
}

Bool
filename2myframe_part (MyFrame *frame, int part, char *filename)
{
    if (filename && frame && part>= 0 && part < FRAME_PARTS)
	{
        if( frame->part_filenames[part] )
            free( frame->part_filenames[part] );
        frame->part_filenames[part] = mystrdup(filename);
        return True;
    }
    return False;
}

Bool
myframe_has_parts(const MyFrame *frame, ASFlagType mask)
{
    if( frame )
    {
        register int i ;
        for( i = 0 ; i < FRAME_PARTS ; ++i )
            if( (mask&(0x01<<i)) )
                if( !IsFramePart(frame,i) )
                    return False;
        return True;
    }
    return False;
}

void
destroy_myframe( MyFrame **pframe )
{
    MyFrame *pf = *pframe ;
    if( pf )
    {
        register int i = FRAME_PARTS;
        while( --i >= 0 )
            if( pf->parts[i] )
                destroy_icon( &(pf->parts[i]) );
        pf->magic = 0 ;
        free( pf );
        *pframe = NULL ;
    }
}

/*************************************************************************/
