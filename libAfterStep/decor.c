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

//#define LOCAL_DEBUG

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
static ASFlagType
refresh_canvas_config (ASCanvas * pc)
{
    ASFlagType          changed = 0;

	if (pc && pc->w != None)
	{
		int           root_x = pc->root_x, root_y = pc->root_y;
        unsigned int  width = pc->width, height = pc->height;
		Window        wdumm;

		XTranslateCoordinates (dpy, pc->w, Scr.Root, 0, 0, &root_x, &root_y, &wdumm);
        if(root_x != pc->root_x)
            set_flags(changed, CANVAS_X_CHANGED);
        if( root_y != pc->root_y);
            set_flags(changed, CANVAS_Y_CHANGED);
		pc->root_x = root_x;
		pc->root_y = root_y;

        get_drawable_size(pc->w, &width, &height);
        if( width != pc->width )
            set_flags(changed, CANVAS_WIDTH_CHANGED);
        if( height != pc->height )
            set_flags(changed, CANVAS_HEIGHT_CHANGED);

		if (width != pc->width || height != pc->height)
		{
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
LOCAL_DEBUG_CALLER_OUT( "ASCanvas(%p)->canvas(%lX)", pc,pc->canvas );
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

ASFlagType
handle_canvas_config (ASCanvas * canvas)
{
    ASFlagType res ;
LOCAL_DEBUG_CALLER_OUT( "canvas(%p)->window(%lx)->orig_geom(%ux%u%+d%+d)", canvas, canvas->w, canvas->width, canvas->height, canvas->root_x, canvas->root_y );
    res = refresh_canvas_config (canvas);
LOCAL_DEBUG_CALLER_OUT( "canvas(%p)->window(%lx)->new__geom(%ux%u%+d%+d)->change(0x%lX)", canvas, canvas->w, canvas->width, canvas->height, canvas->root_x, canvas->root_y, res );
    return res;
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

	LOCAL_DEBUG_CALLER_OUT ("pc(%p)->im(%p)->x(%d)->y(%d)", pc, im, x, y);
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

Bool
is_canvas_dirty( ASCanvas *pc )
{
    return pc?get_flags (pc->state, CANVAS_DIRTY|CANVAS_OUT_OF_SYNC):False;
}

void
resize_canvas (ASCanvas * pc, unsigned int width, unsigned int height)
{
LOCAL_DEBUG_CALLER_OUT( "canvas(%p)->window(%lx)->geom(%ux%u)", pc, pc->w, width, height );
    /* Setting background to None to avoid background pixmap tiling
	 * while resizing */
    if( width>MAX_POSITION )
    {
        AS_ASSERT(0);
        width = pc->width ;
    }else if( AS_ASSERT(width))
        width = 1 ;
    if( height>MAX_POSITION )
    {
        AS_ASSERT(0);
        height = pc->height ;
    }else if( AS_ASSERT(height))
        height = 1;
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
    if( width>MAX_POSITION )
    {
        AS_ASSERT(0);
        width = pc->width ;
    }else if( AS_ASSERT(width))
        width = 1 ;
    if( height>MAX_POSITION )
    {
        AS_ASSERT(0);
        height = pc->height ;
    }else if( AS_ASSERT(height))
        height = 1;

    if ((pc->width < width || pc->height < height) && !get_flags( pc->state, CANVAS_CONTAINER ))
		XSetWindowBackgroundPixmap (dpy, pc->w, None);
    XMoveResizeWindow (dpy, pc->w, x, y, width, height);
}

void
move_canvas (ASCanvas * pc, int x, int y)
{
LOCAL_DEBUG_CALLER_OUT( "canvas(%p)->window(%lx)->geom(%+d%+d)", pc, pc->w, x, y );
    XMoveWindow (dpy, pc->w, x, y);
}


void
unmap_canvas_window( ASCanvas *pc )
{
    if( pc && pc->w != None )
        XUnmapWindow( dpy, pc->w );
}

void
map_canvas_window( ASCanvas *pc, Bool raised )
{
    if( pc && pc->w != None )
    {
        if( raised )
            XMapRaised( dpy, pc->w );
        else
            XMapWindow( dpy, pc->w );
    }
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
/*        safe_asimage_destroy (btn->pressed); */
        btn->pressed = NULL ;
    }

    if( btn->unpressed )
    {
/*        safe_asimage_destroy (btn->unpressed); */
        btn->unpressed = NULL ;
    }
    btn->current = NULL ;
}

void
set_tbtn_images( ASTBtnData* btn, struct button_t *from )
{
    Bool pressed = False;
    if( AS_ASSERT(btn) || from == NULL )
        return ;

    if( btn->current != NULL )
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
/* ASBtnBlock :                                                    */
/********************************************************************/
void
free_asbtn_block( ASTile* tile )
{
    register ASBtnBlock *blk = &(tile->data.bblock);
    register int i = blk->buttons_num ;
    while( --i >= 0 )
        free_tbtn_images( &(blk->buttons[i]) );

    free( blk->buttons );
    memset( blk, 0x00, sizeof(ASBtnBlock ) );
    ASSetTileType(tile,AS_TileFreed);
}

static void
build_btn_block( ASTile *tile,
                 struct button_t *from_list, ASFlagType mask, unsigned int count,
                 int left_margin, int top_margin, int spacing, int order,
                 unsigned long context_base )
{

    unsigned int real_count = 0 ;
    unsigned short max_width = 0, max_height = 0 ;
    register int i = count ;
    ASBtnBlock *blk = &(tile->data.bblock) ;
    LOCAL_DEBUG_CALLER_OUT( "lm(%d),tm(%d),sp(%d),or(%d),cb(%lX)", left_margin, top_margin, spacing, order, context_base );
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
        int pos = get_flags(order, TBTN_ORDER_REVERSE)?-left_margin:left_margin ;

        blk->buttons = safecalloc( real_count, sizeof(ASTBtnData));
		blk->buttons_num = real_count ;
        i = count-1 ;
        while( i >= 0 && k >= 0 )
        {
            if( (mask&(0x01<<i)) != 0 && (from_list[i].unpressed.image || from_list[i].pressed.image))
            {
                set_tbtn_images( &(blk->buttons[k]), &(from_list[i]) );
                blk->buttons[k].context = context_base<<i ;
                --k ;
            }
            --i ;
        }

                /* top_margin surrounds button block from both sides ! */
        if( get_flags(order, TBTN_ORDER_VERTICAL) )
        {
            tile->width = max_width + top_margin*2 ;
            tile->height = left_margin ;
        }else
        {
            tile->width =  left_margin ;
            tile->height = max_height+ top_margin*2 ;
        }

        k = 0 ;
        while( k < real_count )
        {
            if( get_flags(order, TBTN_ORDER_VERTICAL) )
            {
                blk->buttons[k].x = left_margin+((max_width - blk->buttons[k].width)>>1) ;
                tile->height += blk->buttons[k].height+spacing ;
                if( get_flags(order, TBTN_ORDER_REVERSE) )
                {
                    pos -= blk->buttons[k].height ;
                    blk->buttons[k].y = pos ;
                    pos -= spacing ;
                }else
                {
                    blk->buttons[k].y = pos ;
                    pos += blk->buttons[k].height ;
                    pos += spacing ;
                }
            }else
            {
                blk->buttons[k].y = top_margin+((max_height - blk->buttons[k].height)>>1) ;
                tile->width += blk->buttons[k].width+spacing ;
                if( get_flags(order, TBTN_ORDER_REVERSE) )
                {
                    pos -= blk->buttons[k].width ;
                    blk->buttons[k].x = pos ;
                    pos -= spacing ;
                }else
                {
                    blk->buttons[k].x = pos ;
                    pos += blk->buttons[k].width ;
                    pos += spacing ;
                }
            }
            ++k ;
        }
        if( get_flags(order, TBTN_ORDER_REVERSE) )
        {
	    k = real_count ;
            if( get_flags(order, TBTN_ORDER_VERTICAL) )
                while(  --k >= 0 )
                    blk->buttons[k].y += tile->height ;
            else
                while(  --k >= 0 )
                    blk->buttons[k].x += tile->width ;
        }
    }
    ASSetTileSublayers(*tile,real_count);
}

static int
check_btn_point( ASTile *tile, int x, int y )
{
	ASBtnBlock *bb = &(tile->data.bblock);
    register int i = bb->buttons_num ;
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
    return C_NO_CONTEXT;
}

static int
set_asbtn_block_layer( ASTile* tile, ASImageLayer *layer, unsigned int state )
{
    register ASBtnBlock *bb = &(tile->data.bblock);
    register int i = bb->buttons_num ;
    while( --i >= 0 )
    {
        register ASTBtnData *btn = &(bb->buttons[i]) ;
        layer[i].im = btn->current ;
        layer[i].dst_x = tile->x + btn->x ;
        layer[i].dst_y = tile->y + btn->y ;
        layer[i].clip_width  = layer[i].im->width ;
        layer[i].clip_height = layer[i].im->height ;
    }
    return bb->buttons_num;
}


/********************************************************************/
/* ASIcon :                                                         */
/********************************************************************/
static void
free_asicon( ASTile* tile )
{
    if (tile->data.icon)
    {
        safe_asimage_destroy (tile->data.icon);
        tile->data.icon = NULL ;
    }
    ASSetTileType(tile,AS_TileFreed);
}

static int
set_asicon_layer( ASTile* tile, ASImageLayer *layer, unsigned int state )
{
    layer->im = tile->data.icon;
    if( layer->im == NULL )
        return 0;
    layer->dst_x = tile->x;
    layer->dst_y = tile->y ;
    layer->clip_width  = layer->im->width ;
    layer->clip_height = layer->im->height ;
    return 1;
}
/********************************************************************/
/* ASLabel :                                                        */
/********************************************************************/
static void
free_aslabel( ASTile* tile )
{
    register ASLabel *lbl = &(tile->data.label);
    register int i ;

    for (i = 0; i < BAR_STATE_NUM; ++i)
    {
        if (lbl->rendered[i])
            destroy_asimage (&(lbl->rendered[i]));
        lbl->rendered[i] = NULL ;
    }
    if( lbl->text )
    {
        free( lbl->text );
        lbl->text = NULL ;
    }
    ASSetTileType(tile,AS_TileFreed);
}

static void
aslabel_style_changed(  ASTile* tile, MyStyle *style, unsigned int state )
{
    register ASLabel *lbl = &(tile->data.label);
    register int i ;
    ASImage *im;
    int flip = ASTileFlip(*tile);
    if (lbl->rendered[state] != NULL)
        destroy_asimage( &(lbl->rendered[state]) );

    im = mystyle_draw_text_image (style, lbl->text);
LOCAL_DEBUG_OUT( "state(%d)->style(\"%s\")->text(\"%s\")->image(%p)", state, style?style->name:"none", lbl->text, im );
    if( flip != 0 )
    {
        int w = im->width ;
        int h = im->height ;
        if( get_flags( flip, FLIP_VERTICAL ) )
        {
            w = im->height ;
            h = im->width ;
        }
        lbl->rendered[state] = flip_asimage ( Scr.asv,
                                              im,
                                              0, 0, im->height, im->width,
                                              flip, ASA_ASImage,
                                              100, ASIMAGE_QUALITY_DEFAULT );
    }else
        lbl->rendered[state] = im;

    tile->width = 0 ;
    tile->height = 0 ;
    for( i = 0 ; i < BAR_STATE_NUM; ++i)
        if( lbl->rendered[i] )
        {
            if( tile->width < lbl->rendered[i]->width )
                tile->width = lbl->rendered[i]->width;
            if( tile->height < lbl->rendered[i]->height )
                tile->height = lbl->rendered[i]->height;
        }
}

static int
set_aslabel_layer( ASTile* tile, ASImageLayer *layer, unsigned int state )
{
    register ASLabel *lbl = &(tile->data.label);
    layer->im = lbl->rendered[state] ;
    if( layer->im == NULL )
        if( (layer->im = lbl->rendered[(~state)&BAR_STATE_FOCUS_MASK] ) == NULL )
            return 0;
    layer->dst_x = tile->x ;
    layer->dst_y = tile->y ;
    layer->clip_width  = layer->im->width ;
    layer->clip_height = layer->im->height ;
    return 1;
}


/********************************************************************/
/* ASTBarData :                                                     */
/********************************************************************/
struct
{
    char *name;
    void (*free_astile_handler)( ASTile* tile );
    int  (*check_point_handler)( ASTile* tile, int x, int y );
    void (*on_style_changed_handler)( ASTile* tile, MyStyle *style, unsigned int state );
    int  (*set_layer_handler)( ASTile* tile, ASImageLayer *layer, unsigned int state );

}ASTileTypeHandlers[AS_TileTypes] =
{
{"Spacer",  NULL, NULL, NULL},
{"Buttons", free_asbtn_block, check_btn_point, NULL,                  set_asbtn_block_layer},
{"Icon",    free_asicon,      NULL,            NULL,                  set_asicon_layer},
{"Label",   free_aslabel,     NULL,            aslabel_style_changed, set_aslabel_layer},
{"none",    NULL, NULL, NULL},
{"none",    NULL, NULL, NULL},
{"none",    NULL, NULL, NULL},
{"freed",    NULL, NULL, NULL}
};

void print_astbar_tiles( ASTBarData *tbar)
{
    register int l;
    show_progress( "tbar %p has %d tiles :", tbar, tbar->tiles_num );
    for( l = 0 ; l < tbar->tiles_num ; ++l )
    {
        show_progress( "\t %3.3d: %s flags(%X) %ux%u%+d%+d data.raw = (0x%lx, 0x%lx, 0x%lx)", l, ASTileTypeHandlers[ASTileType(tbar->tiles[l])].name,
                       tbar->tiles[l].flags,
                       tbar->tiles[l].width, tbar->tiles[l].height, tbar->tiles[l].x, tbar->tiles[l].y,
                       tbar->tiles[l].data.raw[0], tbar->tiles[l].data.raw[1], tbar->tiles[l].data.raw[2] );
    }
}

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

            if( tbar->tiles )
            {
                for( i = 0 ; i < tbar->tiles_num ; ++i )
                {
                    int type = ASTileType(tbar->tiles[i]);
                    if( ASTileTypeHandlers[type].free_astile_handler )
                        ASTileTypeHandlers[type].free_astile_handler( &(tbar->tiles[i]) );
                }
                free( tbar->tiles );
            }

            for (i = 0; i < BAR_STATE_NUM; ++i)
			{
				if (tbar->back[i])
					destroy_asimage (&(tbar->back[i]));
            }

			memset (tbar, 0x00, sizeof (ASTBarData));
            free( tbar );
            *ptbar = NULL ;
		}
}

unsigned int
calculate_astbar_height (ASTBarData * tbar)
{
    int height = 0 ;
    int row_height[AS_TileRows] = {0};

    if (tbar)
    {
        register int i = tbar->tiles_num ;
        while( --i >= 0 )
            if( ASTileType(tbar->tiles[i]) != AS_TileFreed )
            {
                register int row = ASTileRow(tbar->tiles[i]);
                if( row_height[row] < tbar->tiles[i].height )
                    row_height[row] = tbar->tiles[i].height;
            }

        for( i = 0 ; i < AS_TileRows ; ++i )
            if( row_height[i] > 0 )
            {
                if( height > 0 )
                    height += tbar->v_spacing ;
                height += row_height[i] ;
            }
        if( height > 0 )
            height += tbar->v_border<<1 ;

        height += tbar->top_bevel + tbar->bottom_bevel;
    }
    return height;
}

unsigned int
calculate_astbar_width (ASTBarData * tbar)
{
    int width = 0 ;
    int col_width[AS_TileColumns] = {0};

    if (tbar)
    {
        register int i = tbar->tiles_num ;
#ifdef LOCAL_DEBUG
/*         print_astbar_tiles(tbar); */
#endif
        while( --i >= 0 )
            if( ASTileType(tbar->tiles[i]) != AS_TileFreed )
            {
                register int col = ASTileCol(tbar->tiles[i]);
                if( col_width[col] < tbar->tiles[i].width )
                    col_width[col] = tbar->tiles[i].width;
            }

        for( i = 0 ; i < AS_TileColumns ; ++i )
            if( col_width[i] > 0 )
            {
                if( width > 0 )
                    width += tbar->h_spacing ;
                width += col_width[i] ;
            }
        if( width > 0 )
            width += tbar->h_border<<1 ;

        width += tbar->left_bevel + tbar->right_bevel;
    }
    return width;
}

Bool
set_astbar_size (ASTBarData * tbar, unsigned int width, unsigned int height)
{
	Bool          changed = False;

    if (tbar )
	{
        unsigned int  w = width;
        unsigned int  h = height;

        if( w >= MAX_POSITION )
            w = tbar->width ;
        else if( w == 0  )
            w = 1 ;
        if( h >= MAX_POSITION )
            h = tbar->height ;
        else if( h == 0  )
            h = 1 ;

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
            mystyle_make_bevel (tbar->style[i], &bevel, BAR_FLAGS2HILITE(tbar->state), False);
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
set_astbar_hilite( ASTBarData *tbar, ASFlagType hilite )
{
    Bool          changed = False;
    if (tbar)
	{
        changed = (BAR_FLAGS2HILITE(tbar->state) != (hilite&HILITE_MASK));
        tbar->state = (tbar->state&BAR_FLAGS_HILITE_MASK)|HILITE2BAR_FLAGS(hilite);
        if (changed )
            update_astbar_bevel_size (tbar);
	}
	return changed;
}

static inline void
set_astile_styles( ASTBarData *tbar, ASTile *tile, int state )
{
    register int i ;
    for (i = 0; i < BAR_STATE_NUM; ++i)
        if((i == state || state == -1) && tbar->style[i] )
            if( ASTileTypeHandlers[ASTileType(*tile)].on_style_changed_handler != NULL )
                ASTileTypeHandlers[ASTileType(*tile)].on_style_changed_handler( tile, tbar->style[i], i );
}

Bool
set_astbar_style_ptr (ASTBarData * tbar, unsigned int state, MyStyle *style)
{
	Bool          changed = False;
LOCAL_DEBUG_OUT( "bar(%p)->state(%d)->style_name(\"%s\")", tbar, state, style?style->name:"" );
    if (tbar && state < BAR_STATE_NUM )
	{
        changed = (style != tbar->style[state]);
		tbar->style[state] = style;
/*LOCAL_DEBUG_OUT( "style(%p)->changed(%d)->tiles_num(%d)", style, changed, tbar->tiles_num );*/
        if (changed )
		{
            register int i = tbar->tiles_num;
            while ( --i >= 0 )
                set_astile_styles( tbar, &(tbar->tiles[i]), state );

            if( tbar->back[state] )
            {
                destroy_asimage (&(tbar->back[state]));
                tbar->back[state] = NULL;
            }
		}
		update_astbar_bevel_size (tbar);
	}
	return changed;
}

Bool
set_astbar_style (ASTBarData * tbar, unsigned int state, const char *style_name)
{
	Bool          changed = False;
    if (tbar && state < BAR_STATE_NUM)
        return set_astbar_style_ptr (tbar, state, mystyle_find_or_default (style_name));
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

static ASTile *
add_astbar_tile( ASTBarData *tbar, int type, unsigned char col, unsigned char row, int flip, int align )
{
    int new_idx = -1;
    ASFlagType align_flags = align ;
    /* try 1: see if we have any tiles that has been freed */
    while( ++new_idx < tbar->tiles_num )
        if( ASTileType(tbar->tiles[new_idx]) == AS_TileFreed )
            break;

    /* try 2: allocate new memory : */
    /* allocating memory if 4 tiles increments for more efficient memory management: */
    if( new_idx == tbar->tiles_num )
    {
        if( (tbar->tiles_num &0x0003) == 0)
            tbar->tiles = realloc( tbar->tiles, (((tbar->tiles_num>>2)+1)<<2)*sizeof(ASTile));
        ++(tbar->tiles_num);
    }

    if( get_flags( flip, FLIP_VERTICAL ) )
        align_flags = (((align&PAD_H_MASK)>>PAD_H_OFFSET)<<PAD_V_OFFSET)|
                      (((align&PAD_V_MASK)>>PAD_V_OFFSET)<<PAD_H_OFFSET) ;

    if( get_flags( flip, FLIP_UPSIDEDOWN ) )
        align_flags = ((align_flags&0x0005)<<1)|((align_flags&(0x0005<<1))>>1);

    memset( &(tbar->tiles[new_idx]), 0x00, sizeof(ASTile));
    tbar->tiles[new_idx].flags = (type&AS_TileTypeMask)|
                                 ((col<<AS_TileColOffset)&AS_TileColMask)|
                                 ((row<<AS_TileRowOffset)&AS_TileRowMask)|
                                 ((flip<<AS_TileFlipOffset)&AS_TileFlipMask)|
                                 ((align_flags<<AS_TilePadOffset)&AS_TilePadMask);
    return &(tbar->tiles[new_idx]);
}

Bool
delete_astbar_tile( ASTBarData *tbar, int idx )
{

    if( tbar != NULL && idx < tbar->tiles_num )
    {
        register int i ;
        for( i = 0 ; i < tbar->tiles_num ; ++i )
            if( i == idx || idx < 0 )
            {
                int type = ASTileType(tbar->tiles[i]);
                if( ASTileTypeHandlers[type].free_astile_handler )
                    ASTileTypeHandlers[type].free_astile_handler( &(tbar->tiles[i]) );
                else if( type != AS_TileFreed )
                {
                    memset( &(tbar->tiles[i]), 0x00, sizeof(ASTile));
                    tbar->tiles[i].flags = AS_TileFreed ;
                }
            }
        return True;
    }
    return False;
}

int
add_astbar_spacer( ASTBarData *tbar, unsigned char col, unsigned char row, int flip, int align,
                   unsigned short width, unsigned short height)
{
    if( tbar )
    {
        ASTile *tile = add_astbar_tile( tbar, AS_TileSpacer, col, row, flip, align );
        tile->width = width ;
        tile->height = height ;
        return tbar->tiles_num-1;
    }
    return -1;
}

int
add_astbar_btnblock( ASTBarData * tbar, unsigned char col, unsigned char row, int flip, int align,
                     struct button_t *from_list, ASFlagType mask, unsigned int count,
                     int left_margin, int top_margin, int spacing, int order,
                     unsigned long context_base)
{
    if( tbar )
    {
        ASTile *tile = add_astbar_tile( tbar, AS_TileBtnBlock, col, row, flip, align );
        build_btn_block( tile, from_list, mask, count, left_margin, top_margin,
                         spacing, order, context_base );
        return tbar->tiles_num-1;
    }
    return -1;
}

int
add_astbar_icon( ASTBarData * tbar, unsigned char col, unsigned char row, int flip, int align, ASImage *icon)
{
    if( tbar && icon )
    {
        ASTile *tile = add_astbar_tile( tbar, AS_TileIcon, col, row, flip, align );
        if( flip == 0 )
        {
            if( (tile->data.icon = dup_asimage( icon )) == NULL )
                    tile->data.icon = clone_asimage( icon, 0xFFFFFFFF );
        }else
            tile->data.icon = flip_asimage( Scr.asv, icon, 0, 0, icon->width, icon->height, flip, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );

        tile->width = tile->data.icon->width;
        tile->height = tile->data.icon->height;
        ASSetTileSublayers(*tile,1);
        return tbar->tiles_num-1;
    }
    return -1;
}

int
add_astbar_label( ASTBarData * tbar, unsigned char col, unsigned char row, int flip, int align, const char *text)
{
LOCAL_DEBUG_CALLER_OUT( "label \"%s\"", text );
    if( tbar )
    {
        ASTile *tile = add_astbar_tile( tbar, AS_TileLabel, col, row, flip, align );
        ASLabel *lbl = &(tile->data.label);
        lbl->text = mystrdup(text);

        set_astile_styles( tbar, tile, -1 );

        ASSetTileSublayers(*tile,1);
        return tbar->tiles_num-1;
    }
    return -1;
}

Bool
change_astbar_label (ASTBarData * tbar, int index, const char *label)
{
	Bool          changed = False;

LOCAL_DEBUG_CALLER_OUT( "tbar(%p)->index(%d)->label(\"%s\")", tbar, index, label );
    if (tbar && tbar->tiles)
	{
        ASLabel *lbl = &(tbar->tiles[index].data.label);
        if( ASTileType(tbar->tiles[index]) != AS_TileLabel )
            return False;

        if (label == NULL)
		{
            if ((changed = (lbl->text != NULL)))
			{
                free (lbl->text);
                lbl->text = NULL;
			}
        } else if (lbl->text == NULL)
		{
			changed = True;
            lbl->text = mystrdup (label);
        } else if ((changed = (strcmp (lbl->text, label) != 0)))
		{
            free (lbl->text);
            lbl->text = mystrdup (label);
		}
		if (changed)
            set_astile_styles( tbar, &(tbar->tiles[index]), -1 );
    }
	return changed;
}


Bool
change_astbar_first_label (ASTBarData * tbar, const char *label)
{
    if (tbar)
	{
        register int i ;
        for( i = 0 ; i < tbar->tiles_num ; ++i )
            if( ASTileType(tbar->tiles[i]) == AS_TileLabel )
                return change_astbar_label (tbar, i, label);
    }
    return False;
}

Bool
move_astbar (ASTBarData * tbar, ASCanvas * pc, int win_x, int win_y)
{
	Bool          changed = False;

	if (tbar && pc)
	{
        int root_x, root_y;
        if( win_x >= MAX_POSITION || win_x <= -MAX_POSITION )
            win_x = tbar->win_x ;
        if( win_y >= MAX_POSITION || win_y <= -MAX_POSITION)
            win_y = tbar->win_y ;

        root_x = pc->root_x + win_x;
        root_y = pc->root_y + win_y;

		changed = (root_x != tbar->root_x || root_y != tbar->root_y);
		tbar->root_x = root_x;
		tbar->root_y = root_y;
		changed = changed || (win_x != tbar->win_x || win_y != tbar->win_y);
		tbar->win_x = win_x;
		tbar->win_y = win_y;
LOCAL_DEBUG_OUT( "tbar(%p)->root_geom(%ux%u%+d%+d)->win_pos(%+d%+d)->changed(%x)", tbar, tbar->width, tbar->height, root_x, root_y, win_x, win_y, changed );
	}
	return changed;
}

Bool
update_astbar_root_pos( ASTBarData *tbar, ASCanvas *pc )
{
    Bool          changed = False;

	if (tbar && pc)
	{
        int           root_x = pc->root_x + tbar->win_x, root_y = pc->root_y + tbar->win_y;

		changed = (root_x != tbar->root_x || root_y != tbar->root_y);
		tbar->root_x = root_x;
		tbar->root_y = root_y;
LOCAL_DEBUG_OUT( "tbar(%p)->root_pos(%+d%+d)", tbar, root_x, root_y );
	}
	return changed;
}

int make_tile_pad( Bool pad_before, Bool pad_after, int cell_size, int tile_size )
{
    if( cell_size <= tile_size )
        return 0;
    if( !pad_before && pad_after )
        return 0;
    if( pad_before && !pad_after )
        return cell_size-tile_size;
    return (cell_size-tile_size)>>1;
}

Bool
render_astbar (ASTBarData * tbar, ASCanvas * pc)
{
    ASImage      *back;
	MyStyle      *style;
	ASImageBevel  bevel;
    ASImageLayer *layers;
	ASImage      *merged_im;
	int           state;
	ASAltImFormats fmt = ASA_XImage;
    int l ;
    short col_width[AS_TileColumns] = {0};
    short row_height[AS_TileRows] = {0};
    short col_x[AS_TileColumns] = {0};
    short row_y[AS_TileRows] = {0};
    unsigned char padded_cols[AS_TileColumns] = {0};
    unsigned char padded_rows[AS_TileRows] = {0};
    int padded_cols_count = 0, padded_rows_count = 0;
    short space_left_x, space_left_y ;
    int x = 0, y = 0 ;
    int good_layers = 0;

	/* input control : */
LOCAL_DEBUG_CALLER_OUT("tbar(%p)->pc(%p)", tbar, pc );
	if (tbar == NULL || pc == NULL || pc->w == None)
		return False;
	state = get_flags (tbar->state, BAR_STATE_FOCUS_MASK);
	style = tbar->style[state];
LOCAL_DEBUG_OUT("style(%p)->geom(%ux%u%+d%+d)", style, tbar->width, tbar->height, tbar->root_x, tbar->root_y );
	if (style == NULL)
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
LOCAL_DEBUG_OUT("back(%p)", back );
	if (back == NULL)
	{
		tbar->back[state] = back = mystyle_make_image (style, tbar->root_x, tbar->root_y, tbar->width, tbar->height);
LOCAL_DEBUG_OUT("back-try2(%p)", back );
		if (back == NULL)
			return False;
	}

    mystyle_make_bevel (style, &bevel, BAR_FLAGS2HILITE(tbar->state), get_flags (tbar->state, BAR_STATE_PRESSED_MASK));
	/* in unfocused and unpressed state we render pixmap and set
	 * window's background to it
	 * in focused state or in pressed state we render to
	 * the window directly, and we'll need to be handling the expose
	 * events
	 */

    /* very complicated layout code : */
    /* pass 1: first we determine width/height of each row/column, as well as count of layers : */
    for( l = 0 ; l < tbar->tiles_num ; ++l )
        if( ASTileType(tbar->tiles[l]) != AS_TileFreed )
        {
            register int pos = ASTileCol(tbar->tiles[l]);
            good_layers += ASTileSublayers(tbar->tiles[l]);
            if( col_width[pos] < tbar->tiles[l].width )
                col_width[pos] = tbar->tiles[l].width;
            if( get_flags( tbar->tiles[l].flags, AS_TileHPadMask ) )
                ++padded_cols[pos] ;
            pos = ASTileRow(tbar->tiles[l]);
            if( row_height[pos] < tbar->tiles[l].height )
                row_height[pos] = tbar->tiles[l].height;
            if( get_flags( tbar->tiles[l].flags, AS_TileVPadMask ) )
                ++padded_rows[pos] ;
        }

    /* pass 2: see how much space we have left that needs to be padded to some rows/columns : */
    space_left_x = tbar->width - (tbar->left_bevel+tbar->right_bevel+tbar->h_border*2);
    space_left_y = tbar->height- (tbar->top_bevel+tbar->bottom_bevel+tbar->v_border*2);
    for( l = 0 ; l < AS_TileColumns ; ++l )
    {
        if( col_width[l] > 0 )
            space_left_x -= col_width[l]+tbar->h_spacing ;
        if( row_height[l] > 0 )
            space_left_y -= row_height[l]+tbar->v_spacing ;
        if( padded_cols[l] )
            ++padded_cols_count;
        if( padded_rows[l] )
            ++padded_rows_count;
    }
    space_left_x += tbar->h_spacing ;
    space_left_y += tbar->v_spacing ;
    /* pass 3: now we determine spread padding among affected cols : */
    if( padded_cols_count > 0 && space_left_x != 0)
        for( l = 0 ; l < AS_TileColumns ; ++l )
        {
            if( padded_cols[l] > 0 )
            {
                register int change = space_left_x/padded_cols_count ;
                if( change == 0 )
                    change = (space_left_x>0)?1:-1 ;
                col_width[l] += change;
                if( col_width[l] < 0 )
                {
                    change -= col_width[l];
                    col_width[l] = 0 ;
                }
                space_left_x -= change ;
                if( space_left_x == 0 )
                    break;
            }
        }
    /* pass 4: now we determine spread padding among affected rows : */
    if( padded_rows_count > 0 && space_left_y != 0)
        for( l = 0 ; l < AS_TileRows ; ++l )
        {
            if( padded_rows[l] > 0 )
            {
                register int change = space_left_y/padded_rows_count ;
                if( change == 0 )
                    change = (space_left_y>0)?1:-1 ;
                row_height[l] += change;
                if( row_height[l] < 0 )
                {
                    change -= row_height[l];
                    row_height[l] = 0;
                }
                space_left_y -= change ;
                if( space_left_y == 0 )
                    break;
            }
        }

    /* pass 5: now we determine offset of each row/column : */
    x = tbar->left_bevel+tbar->h_border ;
    y = tbar->top_bevel+tbar->v_border ;
    for( l = 0 ; l < AS_TileColumns ; ++l )
    {
        col_x[l] = x ;
        row_y[l] = y ;
        if( col_width[l] > 0 )
            x += col_width[l]+tbar->h_spacing ;
        if( row_height[l] > 0 )
            y += row_height[l]+tbar->v_spacing ;
    }
    /* Done with layout */

    layers = create_image_layers (good_layers+1);
	layers[0].im = back;
	layers[0].bevel = &bevel;
	layers[0].clip_width = tbar->width - (tbar->left_bevel + tbar->right_bevel);
	layers[0].clip_height = tbar->height - (tbar->top_bevel + tbar->bottom_bevel);

    /* now we need to loop through tiles and add them to the layers list at correct locations */
    good_layers = 1;
    for( l = 0 ; l < tbar->tiles_num ; ++l )
    {
        int type = ASTileType(tbar->tiles[l]);
        if( ASTileTypeHandlers[type].set_layer_handler )
        {
            int row =  ASTileRow(tbar->tiles[l]);
            int col =  ASTileCol(tbar->tiles[l]);

            tbar->tiles[l].x = col_x[col] + make_tile_pad(  get_flags(tbar->tiles[l].flags, AS_TilePadLeft),
                                                            get_flags(tbar->tiles[l].flags, AS_TilePadRight),
                                                            col_width[col], tbar->tiles[l].width );
            tbar->tiles[l].y = row_y[row] + make_tile_pad(  get_flags(tbar->tiles[l].flags, AS_TilePadTop),
                                                            get_flags(tbar->tiles[l].flags, AS_TilePadBottom),
                                                            row_height[row],tbar->tiles[l].height);
            good_layers += ASTileTypeHandlers[type].set_layer_handler(&(tbar->tiles[l]), &(layers[good_layers]), state );
        }
    }

#ifdef LOCAL_DEBUG
    show_progress("MERGING TBAR %p image %dx%d FROM:",
                  tbar, tbar->width, tbar->height, tbar->tiles_num);
    print_astbar_tiles(tbar);
    show_progress("USING %d layers:",
                  tbar, tbar->width, tbar->height, good_layers);
    for( l = 0 ; l < good_layers ; ++l )
    {
        show_progress( "\t %3.3d: %p %+d%+d %ux%u%+d%+d", l, layers[l].im,
                       layers[l].dst_x, layers[l].dst_y,
                       layers[l].clip_width, layers[l].clip_height, layers[l].clip_x, layers[l].clip_y );
    }
#endif
#ifdef SHAPE
	if (style->texture_type == TEXTURE_SHAPED_PIXMAP || style->texture_type == TEXTURE_SHAPED_SCALED_PIXMAP)
		fmt = ASA_ASImage;
	else if (pc->mask)
		fill_canvas_mask (pc, tbar->win_x, tbar->win_y, tbar->width, tbar->height, 1);
#endif
    merged_im = merge_layers (Scr.asv, &layers[0], good_layers, tbar->width, tbar->height, fmt, 0, ASIMAGE_QUALITY_DEFAULT);
    free( layers );

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
        if( old_focused != focused && pc != NULL )
            render_astbar( tbar, pc );
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
        if( old_pressed != pressed && pc != NULL )
            render_astbar( tbar, pc );
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
            int tmp_context ;
            int i ;
            context = tbar->context ;
            for( i = 0 ; i < tbar->tiles_num ; ++i )
            {
                int type = ASTileType(tbar->tiles[i]);

                if( ASTileTypeHandlers[type].check_point_handler )
                {
                    int tile_x = root_x - tbar->tiles[i].x ;
                    int tile_y = root_y - tbar->tiles[i].y ;
                    if( tile_x >= 0 && tile_y >= 0 && tile_x < tbar->tiles[i]. width && tile_y < tbar->tiles[i].height )
                        if( (tmp_context = ASTileTypeHandlers[type].check_point_handler( &(tbar->tiles[i]), tile_x, tile_y )) != C_NO_CONTEXT )
                        {
                            context = tmp_context ;
                            break;
                        }
                }
            }
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
