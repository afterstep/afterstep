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
#undef DO_CLOCKING

#include "../configure.h"
#include "asapp.h"
#include "afterstep.h"
#include "mystyle.h"
#include "screen.h"
#include "../libAfterImage/afterimage.h"
#include "myicon.h"
#include "canvas.h"
#include "decor.h"
#include "event.h"
#include "balloon.h"
#include "shape.h"

static ASTBarData  *FocusedBar = NULL;          /* currently focused bar with balloon shown for it */

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
            if( btn->balloon )
                destroy_asballoon( &(btn->balloon) );
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
    {
        free_tbtn_images( &(blk->buttons[i]) );
        if( blk->buttons[i].balloon )
                destroy_asballoon( &(blk->buttons[i].balloon) );
    }

    free( blk->buttons );
    memset( blk, 0x00, sizeof(ASBtnBlock ) );
    ASSetTileType(tile,AS_TileFreed);
}

static void
build_btn_block( ASTile *tile,
                 struct button_t **from_list, ASFlagType context_mask, unsigned int count,
                 int left_margin, int top_margin, int spacing, int order )
{

    unsigned int real_count = 0 ;
    unsigned short max_width = 0, max_height = 0 ;
    register int i = count ;
    ASBtnBlock *blk = &(tile->data.bblock) ;
    LOCAL_DEBUG_CALLER_OUT( "lm(%d),tm(%d),sp(%d),or(%d)", left_margin, top_margin, spacing, order );
    if( count > 0 )
        if( !AS_ASSERT( from_list ) )
            while( --i >= 0 )
                if( from_list[i] != NULL &&
                    (context_mask&from_list[i]->context) != 0 &&
                    (from_list[i]->unpressed.image || from_list[i]->pressed.image))
                {
                    ++real_count ;
                    if( from_list[i]->width > max_width )
                        max_width = from_list[i]->width ;
                    if( from_list[i]->height > max_height )
                        max_height = from_list[i]->height ;
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
            if( from_list[i] != NULL &&
                ( context_mask&from_list[i]->context) != 0 &&
                (from_list[i]->unpressed.image || from_list[i]->pressed.image))
            {
                set_tbtn_images( &(blk->buttons[k]), from_list[i] );
                blk->buttons[k].context = from_list[i]->context ;
                --k ;
            }
            --i ;
        }

                /* top_margin surrounds button block from both sides ! */
        if( get_flags(order, TBTN_ORDER_VERTICAL) )
        {
            tile->width = max_width + top_margin*2 ;
            tile->height = left_margin*2 ;
            /*pos = get_flags(order, TBTN_ORDER_REVERSE)?-top_margin:top_margin ; */
        }else
        {
            tile->width =  left_margin*2 ;
            tile->height = max_height+ top_margin*2 ;
        }

        k = 0 ;
        while( k < real_count )
        {
            if( get_flags(order, TBTN_ORDER_VERTICAL) )
            {
                blk->buttons[k].x = top_margin+((max_width - blk->buttons[k].width)>>1) ;
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
		if( get_flags(order, TBTN_ORDER_VERTICAL) )
		{
			if( tile->height > left_margin*2 )
				tile->height -= spacing ;
		}else
		{
			if( tile->width > left_margin*2 )
				tile->width -= spacing ;
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
LOCAL_DEBUG_OUT( "real_count=%d", real_count );
    }else
    {
        tile->width = 1 ;
        tile->height = 1 ;
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

Bool
set_tbtn_pressed( ASBtnBlock *bb, int context )
{
    register int i = bb->buttons_num ;
    Bool changed = False ;
    while( --i >= 0 )
    {
        register ASTBtnData *btn = &(bb->buttons[i]) ;
        ASImage *new_current ;
        new_current = (context&(btn->context))?btn->pressed:btn->unpressed ;
        if( new_current == NULL )
            new_current = (btn->pressed==NULL)?btn->unpressed:btn->pressed ;
        if( new_current != btn->current )
        {
            changed = True;
            btn->current = new_current;
        }
    }
    return changed;
}


static int
set_asbtn_block_layer( ASTile* tile, ASImageLayer *layer, unsigned int state, ASImage **scrap_images, int max_width, int max_height )
{
    register ASBtnBlock *bb = &(tile->data.bblock);
    register int i = bb->buttons_num ;
    while( --i >= 0 )
    {
        register ASTBtnData *btn = &(bb->buttons[i]) ;
		if( btn && btn->current )
		{
      		layer[i].im = btn->current ;
	        layer[i].dst_x = tile->x + btn->x ;
  		    layer[i].dst_y = tile->y + btn->y ;
      		layer[i].clip_width  = layer[i].im->width ;
	        layer[i].clip_height = layer[i].im->height ;
		}
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
set_asicon_layer( ASTile* tile, ASImageLayer *layer, unsigned int state, ASImage **scrap_images, int max_width, int max_height )
{
    int dst_width, dst_height ;
    ASImage *im = tile->data.icon ;

    if( im == NULL )
        return 0;

    dst_width  = im->width ;
    dst_height = im->height ;

    /* if( ASTileResizeable( *tile ) ) */
    {
        if( get_flags( tile->flags, AS_TileHScale ) )
            dst_width = max_width ;
		else if( get_flags( tile->flags, AS_TileHResize ) && max_width < im->width )
		{
			if( get_flags( tile->flags, AS_TilePadLeft ) )
			{
				if( get_flags( tile->flags, AS_TilePadRight ) )
					layer->clip_x = ((int)im->width - max_width)/2 ;
				else
					layer->clip_x = ((int)im->width - max_width) ;
			}
		}
        if( get_flags( tile->flags, AS_TileVScale ) )
            dst_height = max_height ;
        else if( get_flags( tile->flags, AS_TileVResize ) && max_height < im->height )
		{
			if( get_flags( tile->flags, AS_TilePadTop ) )
			{
				if( get_flags( tile->flags, AS_TilePadBottom ) )
					layer->clip_y = ((int)im->height - max_height)/2 ;
				else
					layer->clip_y = ((int)im->height - max_height ) ;
			}
		}
    }
	LOCAL_DEBUG_OUT( "flags = %lX, dst_size = %dx%d, im_size = %dx%d, max_size = %dx%d, clip = %+d%+d", tile->flags, dst_width, dst_height, im->width, im->height, max_width, max_height, layer->clip_x, layer->clip_y );
    if( im->width != dst_width || im->height != dst_height )
    {
        im = scale_asimage( Scr.asv, im, dst_width, dst_height, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );
        if( im == NULL )
            im = tile->data.icon ;
        else
            *scrap_images = im ;
    }

    layer->im = im;
    layer->dst_x = tile->x;
    layer->dst_y = tile->y ;
    if( ASTileHResizeable(*tile) )
        layer->clip_width  = max_width ;
    else
        layer->clip_width  = layer->im->width ;

    if( ASTileVResizeable(*tile) )
        layer->clip_height = max_height ;
    else
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

    im = mystyle_draw_text_image (style, lbl->text, lbl->encoding);
LOCAL_DEBUG_OUT( "state(%d)->style(\"%s\")->text(\"%s\")->image(%p)->flip(%d)", state, style?style->name:"none", lbl->text, im, flip );
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
                                              0, 0, w, h,
                                              flip, ASA_ASImage,
                                              100, ASIMAGE_QUALITY_DEFAULT );
        safe_asimage_destroy( im );
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
	if( get_flags( flip, FLIP_VERTICAL ) )
	{
   		tile->width += lbl->v_padding*2 ;
		tile->height += lbl->h_padding*2 ;
	}else
	{
   		tile->width += lbl->h_padding*2 ;
		tile->height += lbl->v_padding*2 ;
	}
}

static int
set_aslabel_layer( ASTile* tile, ASImageLayer *layer, unsigned int state, ASImage **scrap_images, int max_width, int max_height )
{
    register ASLabel *lbl = &(tile->data.label);
    CARD32 alpha ;
	short h_pad = lbl->h_padding ;
	short v_pad = lbl->v_padding ;

    layer->im = lbl->rendered[state] ;
    if( layer->im == NULL )
        if( (layer->im = lbl->rendered[(~state)&BAR_STATE_FOCUS_MASK] ) == NULL )
            return 0;

	if( get_flags( ASTileFlip(*tile), FLIP_VERTICAL ) )
	{
		h_pad = lbl->v_padding ;
		v_pad = lbl->h_padding ;
	}

    layer->dst_x = tile->x + h_pad;
    layer->dst_y = tile->y + v_pad ;
    if( layer->im->width < max_width )
    {
        if( layer->im->width < tile->width )
            layer->dst_x += make_tile_pad( get_flags(tile->flags, AS_TilePadLeft), 
                                           get_flags(tile->flags, AS_TilePadRight), 
                                           tile->width-h_pad*2, layer->im->width );
        layer->clip_width  = layer->im->width ;
    }else
        layer->clip_width  = ( max_width > h_pad ) ? max_width - h_pad: 1;
    if( layer->im->height < max_height )
    {
        if( layer->im->height < tile->height )
            layer->dst_y += make_tile_pad( get_flags(tile->flags, AS_TilePadTop), 
                                           get_flags(tile->flags, AS_TilePadBottom), 
                                           tile->height-v_pad*2, layer->im->height );
        layer->clip_height  = layer->im->height ;
    }else
        layer->clip_height  = ( max_height > v_pad )? max_height - v_pad : 1;

    layer->clip_height = min(layer->im->height, max_height) ;
    alpha = ARGB32_ALPHA8(layer->im->back_color);
    if( alpha < 0x00FF && alpha > 1 )
        layer->tint = MAKE_ARGB32((alpha>>1),0x007F,0x007F,0x007F);
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
    int  (*set_layer_handler)( ASTile* tile, ASImageLayer *layer, unsigned int state, ASImage **scrap_images, int max_width, int max_height );

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
        show_progress( "\t %3.3d: [%2.2d][%2.2d] %s flags(%X) %ux%u%+d%+d data.raw = (0x%lx, 0x%lx, 0x%lx)",
					   l, ASTileCol(tbar->tiles[l]), ASTileRow(tbar->tiles[l]),
					   ASTileTypeHandlers[ASTileType(tbar->tiles[l])].name,
                       tbar->tiles[l].flags,
                       tbar->tiles[l].width, tbar->tiles[l].height, tbar->tiles[l].x, tbar->tiles[l].y,
                       tbar->tiles[l].data.raw[0], tbar->tiles[l].data.raw[1], tbar->tiles[l].data.raw[2] );
    }
}

ASTBarData   *
create_astbar ()
{
	ASTBarData   *tbar = safecalloc (1, sizeof (ASTBarData));

    set_flags( tbar->state, BAR_FLAGS_REND_PENDING );
	LOCAL_DEBUG_CALLER_OUT( "<<#########>>created tbar %p", tbar );
    tbar->rendered_root_x = tbar->rendered_root_y = 0xFFFF;
    tbar->composition_method[0] = TEXTURE_TRANSPIXMAP_ALPHA ;
    tbar->composition_method[1] = TEXTURE_TRANSPIXMAP_ALPHA ;
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

			LOCAL_DEBUG_CALLER_OUT( "<<#########>>destroying tbar %p", tbar );
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
            if( tbar == FocusedBar )
            {
                withdraw_balloon( NULL );
                FocusedBar = NULL;
            }
            if( tbar->balloon )
                destroy_asballoon( &(tbar->balloon) );

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
            if( ASTileType(tbar->tiles[i]) != AS_TileFreed &&
                !ASTileIgnoreHeight(tbar->tiles[i]))
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
#if	defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
         print_astbar_tiles(tbar); 
#endif
        while( --i >= 0 )
            if( ASTileType(tbar->tiles[i]) != AS_TileFreed &&
                !ASTileIgnoreWidth(tbar->tiles[i]))
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
            set_flags( tbar->state, BAR_FLAGS_REND_PENDING );
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
            mystyle_make_bevel (tbar->style[i], &bevel, tbar->hilite[i], False);
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
set_astbar_hilite( ASTBarData *tbar, unsigned int state, ASFlagType hilite )
{
    Bool          changed = False;
LOCAL_DEBUG_CALLER_OUT( "%p,%d,0x%lX", tbar, state, hilite );
    if (tbar && state < BAR_STATE_NUM)
	{
        changed = (tbar->hilite[state] != (hilite&HILITE_MASK));
        tbar->hilite[state] = (hilite&HILITE_MASK);
        if (changed )
        {
            update_astbar_bevel_size (tbar);
            set_flags( tbar->state, BAR_FLAGS_REND_PENDING );
        }
	}
	return changed;
}

Bool set_astbar_composition_method( ASTBarData *tbar, unsigned int state, unsigned char method )
{
    Bool          changed = False;
    if (tbar && state < BAR_STATE_NUM)
	{
        changed = (tbar->composition_method[state] != method);
        tbar->composition_method[state] = method ;
        if (changed )
            set_flags( tbar->state, BAR_FLAGS_REND_PENDING );
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
            set_flags( tbar->state, BAR_FLAGS_REND_PENDING );
            update_astbar_bevel_size (tbar);
        }
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
                      (((align&PAD_V_MASK)>>PAD_V_OFFSET)<<PAD_H_OFFSET)|
                      (((align&RESIZE_H_MASK)>>RESIZE_H_OFFSET)<<RESIZE_V_OFFSET)|
                      (((align&RESIZE_V_MASK)>>RESIZE_V_OFFSET)<<RESIZE_H_OFFSET) ;

    if( get_flags( flip, FLIP_UPSIDEDOWN ) )
        align_flags = ((align_flags&0x0005)<<1)|((align_flags&(0x0005<<1))>>1)|(align_flags&RESIZE_MASK);

	LOCAL_DEBUG_CALLER_OUT( "type = %d, col = %d, row = %d, flip = %d, align_flags = 0x%lX", type, col, row, flip, align_flags );

	clear_flags( align_flags, FIT_LABEL_SIZE);
	if( get_flags( flip, FLIP_VERTICAL ) )
	{
		if( get_flags( align, FIT_LABEL_WIDTH ) )
        	set_flags( align_flags, FIT_LABEL_HEIGHT );
    	if( get_flags( align, FIT_LABEL_HEIGHT ) )
        	set_flags( align_flags, FIT_LABEL_WIDTH );
	}else
	{

		if( get_flags( align, FIT_LABEL_WIDTH ) )
        	set_flags( align_flags, FIT_LABEL_WIDTH );
    	if( get_flags( align, FIT_LABEL_HEIGHT ) )
        	set_flags( align_flags, FIT_LABEL_HEIGHT );
	}
	LOCAL_DEBUG_CALLER_OUT( "type = %d, flip = %d, align = 0x%X, align_flags = 0x%lX", type, flip, align, align_flags );
    align_flags &= (PAD_MASK|RESIZE_MASK|FIT_LABEL_SIZE);

    memset( &(tbar->tiles[new_idx]), 0x00, sizeof(ASTile));
    tbar->tiles[new_idx].flags = (type&AS_TileTypeMask)|
                                 ((col<<AS_TileColOffset)&AS_TileColMask)|
                                 ((row<<AS_TileRowOffset)&AS_TileRowMask)|
                                 ((flip<<AS_TileFlipOffset)&AS_TileFlipMask)|
                                 ((align_flags<<AS_TileFloatingOffset));
    set_flags( tbar->state, BAR_FLAGS_REND_PENDING );
    return &(tbar->tiles[new_idx]);
}

Bool
delete_astbar_tile( ASTBarData *tbar, int idx )
{

    if( tbar != NULL && idx < (int)(tbar->tiles_num) )
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
        set_flags( tbar->state, BAR_FLAGS_REND_PENDING );
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
                     struct button_t **from_list, ASFlagType context_mask, unsigned int count,
                     int left_margin, int top_margin, int spacing, int order)
{
    if( tbar )
    {
        ASTile *tile = add_astbar_tile( tbar, AS_TileBtnBlock, col, row, flip, align );
/*      if( get_flags( flip, FLIP_VERTICAL ) )
        {
            int tmp = top_margin ; 
            top_margin = left_margin ; 
            left_margin = tmp ;    
       }    
 */ 
        build_btn_block( tile, from_list, context_mask, count, left_margin, top_margin,
                         spacing, order );
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
		{
			int dst_width = icon->width, dst_height = icon->height ;
			if( get_flags( flip, FLIP_VERTICAL ) )
			{
				dst_width = icon->height ;
				dst_height = icon->width ;
			}
            tile->data.icon = flip_asimage( Scr.asv, icon, 0, 0, dst_width, dst_height, flip, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
		}
        tile->width = tile->data.icon->width;
        tile->height = tile->data.icon->height;
        ASSetTileSublayers(*tile,1);
        return tbar->tiles_num-1;
    }
    return -1;
}

int
add_astbar_label( ASTBarData * tbar, unsigned char col, unsigned char row, int flip, int align, short h_padding, short v_padding, const char *text, unsigned long encoding)
{
LOCAL_DEBUG_CALLER_OUT( "label \"%s\"", text?text:"null" );
    if( tbar )
    {
        ASTile *tile = add_astbar_tile( tbar, AS_TileLabel, col, row, flip, align );
        ASLabel *lbl = &(tile->data.label);
        lbl->text = mystrdup(text);
		lbl->encoding = encoding ;
		lbl->h_padding = h_padding ;
		lbl->v_padding = v_padding ;
        set_astile_styles( tbar, tile, -1 );

        ASSetTileSublayers(*tile,1);
        return tbar->tiles_num-1;
    }
    return -1;
}

Bool
change_astbar_label (ASTBarData * tbar, int index, const char *label, unsigned long encoding )
{
	Bool          changed = False;

LOCAL_DEBUG_CALLER_OUT( "tbar(%p)->index(%d)->label(\"%s\")", tbar, index, label?label:"null" );
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
        } else if ((changed = (strcmp ((char*)(lbl->text), label) != 0)))
		{
            free (lbl->text);
            lbl->text = mystrdup (label);
		}
		if( !changed && lbl->encoding != encoding )
			changed = True ;
		lbl->encoding = encoding ;
		if (changed)
        {
            set_astile_styles( tbar, &(tbar->tiles[index]), -1 );
            set_flags( tbar->state, BAR_FLAGS_REND_PENDING );
        }
    }
	return changed;
}


Bool
change_astbar_first_label (ASTBarData * tbar, const char *label, unsigned long encoding)
{
    if (tbar)
	{
        register int i ;
        for( i = 0 ; i < tbar->tiles_num ; ++i )
            if( ASTileType(tbar->tiles[i]) == AS_TileLabel )
                return change_astbar_label (tbar, i, label, encoding);
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

        root_x = pc->root_x + (int)pc->bw + win_x;
        root_y = pc->root_y + (int)pc->bw + win_y;

		changed = (root_x != tbar->root_x || root_y != tbar->root_y);
		tbar->root_x = root_x;
		tbar->root_y = root_y;
		if( changed )
		{
			Bool redraw = False ;
			register int  i = BAR_STATE_NUM;
			while (--i >= 0)
			{
				if (tbar->style[i] && TransparentMS(tbar->style[i]))
            	{
					if (tbar->back[i])
						destroy_asimage (&(tbar->back[i]));
                	if( i == ((tbar->state)&BAR_STATE_FOCUS_MASK) )
                    	redraw = True;
            	}
			}
        	if( redraw )
            	set_flags( tbar->state, BAR_FLAGS_REND_PENDING );
		}
		changed = changed || (win_x != tbar->win_x || win_y != tbar->win_y);
		tbar->win_x = win_x;
		tbar->win_y = win_y;
        if( changed )
            set_flags( tbar->state, BAR_FLAGS_REND_PENDING );
LOCAL_DEBUG_OUT( "tbar(%p)->root_geom(%ux%u%+d%+d)->win_pos(%+d%+d)->changed(%x)", tbar, tbar->width, tbar->height, root_x, root_y, win_x, win_y, changed );
	}
	return changed;
}

int make_tile_pad( Bool pad_before, Bool pad_after, int cell_size, int tile_size )
{
    if( cell_size <= tile_size )
        return 0;
	if( !pad_before )
        return 0;
    if( !pad_after )
        return cell_size-tile_size;
    return (cell_size-tile_size)>>1;
}

Bool
set_astbar_focused (ASTBarData * tbar, ASCanvas * pc, Bool focused)
{
	if (tbar)
	{
        int          old_focused = get_flags (tbar->state, BAR_STATE_FOCUS_MASK)?1:0;
		int          new_focused = focused?1:0;

		if (focused)
			set_flags (tbar->state, BAR_STATE_FOCUSED);
		else
			clear_flags (tbar->state, BAR_STATE_FOCUSED);
        if( old_focused != new_focused && pc != NULL )
            render_astbar( tbar, pc );
        return (new_focused != old_focused);
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


        if( old_pressed != pressed )
            set_flags(tbar->state, BAR_FLAGS_REND_PENDING);
        if( get_flags(tbar->state, BAR_FLAGS_REND_PENDING) && pc != NULL )
            render_astbar( tbar, pc );
        return ((pressed?1:0)!=old_pressed);
	}
	return False;
}

Bool
set_astbar_btn_pressed (ASTBarData * tbar, int context)
{
	if (tbar)
	{
        int i ;
        Bool changed = False ;
        for( i = 0 ; i < tbar->tiles_num ; ++i )
            if( ASTileType(tbar->tiles[i]) == AS_TileBtnBlock )
                if( set_tbtn_pressed( &(tbar->tiles[i].data.bblock), context ) )
                    changed = True;
        if( changed )
            set_flags(tbar->state, BAR_FLAGS_REND_PENDING);
        return changed;
	}
	return False;
}


Bool
update_astbar_transparency (ASTBarData * tbar, ASCanvas * pc, Bool force)
{
	int           root_x, root_y;
    Bool          changed = False;
    Bool          redraw = False;

	if (tbar == NULL || pc == NULL)
        return False;;

	root_x = pc->root_x + (int)pc->bw + tbar->win_x;
	root_y = pc->root_y + (int)pc->bw + tbar->win_y;
    if ((changed = (root_x != tbar->root_x || root_y != tbar->root_y || Scr.RootImage == NULL || force )))
	{
        register int  i = BAR_STATE_NUM;

        tbar->root_x = root_x;
		tbar->root_y = root_y;

		while (--i >= 0)
		{
			if (tbar->style[i] && TransparentMS(tbar->style[i]))
            {
				if (tbar->back[i])
					destroy_asimage (&(tbar->back[i]));
                if( i == ((tbar->state)&BAR_STATE_FOCUS_MASK) )
                    redraw = True;
            }
		}
        if( redraw )
            set_flags( tbar->state, BAR_FLAGS_REND_PENDING );
    }
    return redraw;
}

Bool
is_astbar_shaped( ASTBarData *tbar, int state )
{
    Bool shaped = False ;
    MyStyle *style ;
    if( state < 0 || state == BAR_STATE_UNFOCUSED )
    {
        if( (style = tbar->style[BAR_STATE_UNFOCUSED]) != NULL )
            if( style->texture_type == TEXTURE_SHAPED_PIXMAP ||
                style->texture_type == TEXTURE_SHAPED_SCALED_PIXMAP )
                shaped = True ;
    }
    if( state < 0 || state == BAR_STATE_FOCUSED )
    {
        if( (style = tbar->style[BAR_STATE_FOCUSED]) != NULL )
            if( style->texture_type == TEXTURE_SHAPED_PIXMAP ||
                style->texture_type == TEXTURE_SHAPED_SCALED_PIXMAP )
                shaped = True ;
    }
    return shaped;
}

#ifdef TRACE_render_astbar
#undef render_astbar
Bool render_astbar (ASTBarData * tbar, ASCanvas * pc);
Bool
trace_render_astbar (ASTBarData * tbar, ASCanvas * pc, const char *file, int line)
{
    Bool res;
    fprintf (stderr, "D>%s(%d):render_astbar(%p,%p)\n", file, line, tbar, pc);
    res = render_astbar (tbar, pc);
	fprintf (stderr,
             "D>%s(%d):render_astbar(%p,%p) returned %d\n", file, line, tbar, pc, res);
    if( tbar && res <= 0 )
        fprintf( stderr, "D>%s(%d):render_astbar tbar data: state %lX, %ux%u%+d%+d, root %+d%+d, styles %p,%p, tiles_num %d, tiles %p, canvas %X\n" ,
                 file, line, tbar->state, tbar->width, tbar->height, tbar->win_x, tbar->win_y, tbar->root_x, tbar->root_y, tbar->style[0], tbar->style[1], tbar->tiles_num, tbar->tiles, pc->canvas );
    return res;
}
#endif
Bool
render_astbar (ASTBarData * tbar, ASCanvas * pc)
{
    START_TIME(started);
    ASImage      *back;
	MyStyle      *style;
	ASImageBevel  bevel;
    ASImageLayer *layers;
    ASImage     **scrap_images = NULL;
	ASImage      *merged_im;
	int           state;
	ASAltImFormats fmt = ASA_ScratchXImage;
    int l ;
    short col_width[AS_TileColumns] = {0};
    short row_height[AS_TileRows] = {0};
    short col_x[AS_TileColumns] = {0};
    short row_y[AS_TileRows] = {0};
    short floating_cols[AS_TileColumns] = {0};
    short floating_rows[AS_TileRows] = {0};
    int floating_cols_count = 0, floating_rows_count = 0;
    short space_left_x, space_left_y ;
    int x = 0, y = 0 ;
    int good_layers = 0;
    Bool res = False;
    Bool render_mask = False ;
    merge_scanlines_func merge_func = alphablend_scanlines ;
    int h_bevel_size = 0, v_bevel_size = 0 ;

	/* input control : */
LOCAL_DEBUG_CALLER_OUT("tbar(%p)->pc(%p)", tbar, pc );
	if (tbar == NULL || pc == NULL || pc->w == None)
        return -3;
	state = get_flags (tbar->state, BAR_STATE_FOCUS_MASK);
	style = tbar->style[state];
LOCAL_DEBUG_OUT("style(%p)->geom(%ux%u%+d%+d)->hilite(0x%X)", style, tbar->width, tbar->height, tbar->root_x, tbar->root_y, tbar->hilite[state] );

    if (style == NULL)
        return -2;
    
    mystyle_make_bevel (style, &bevel, tbar->hilite[state], get_flags (tbar->state, BAR_STATE_PRESSED_MASK));
    h_bevel_size = bevel.left_outline+bevel.right_outline ;
    v_bevel_size = bevel.top_outline+bevel.bottom_outline ;

    /* validating our images : */
	if ((back = tbar->back[state]) != NULL)
	{
        if( tbar->root_x != pc->root_x + (int)pc->bw + tbar->win_x ||
            tbar->root_y != pc->root_y + (int)pc->bw + tbar->win_y )
            update_astbar_transparency( tbar, pc, False );

        if (back->width != tbar->width || back->height != tbar->height ||
			((tbar->rendered_root_x != tbar->root_x || tbar->rendered_root_y != tbar->root_y) &&
			 style->texture_type > TEXTURE_PIXMAP))
		{
			destroy_asimage (&(tbar->back[state]));
			tbar->back[state] = back = NULL;
		}
	}
LOCAL_DEBUG_OUT("back(%p), vertical?%s", back, get_flags( tbar->state, BAR_FLAGS_VERTICAL )?"Yes":"No" );
	if (back == NULL)
	{
        back = mystyle_make_image (style, 
                                   tbar->root_x+bevel.left_outline, 
                                   tbar->root_y+bevel.top_outline , 
                                   tbar->width, tbar->height, 
                                   get_flags( tbar->state, BAR_FLAGS_VERTICAL )?FLIP_VERTICAL:0);
		tbar->back[state] = back ;
LOCAL_DEBUG_OUT("back-try2(%p)", back );
		if (back == NULL)
            return -1;
	}

    /*mystyle_make_bevel (style, &bevel, 0, get_flags (tbar->state, BAR_STATE_PRESSED_MASK));
	 * in unfocused and unpressed state we render pixmap and set
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
			if( !ASTileIgnoreWidth(tbar->tiles[l]) )
			{
            	if( col_width[pos] < tbar->tiles[l].width )
                	col_width[pos] = tbar->tiles[l].width;
                /* floating column has at least one element padded/resizable in horizontal dir,
                 * and no fixed elements */
                if( !ASTileHFloating(tbar->tiles[l]) )
                    floating_cols[pos] = -1 ;
                else if( floating_cols[pos] >= 0 )
                    ++floating_cols[pos] ; 
            }
            
            pos = ASTileRow(tbar->tiles[l]);
			if( !ASTileIgnoreHeight(tbar->tiles[l]) )
			{
            	if( row_height[pos] < tbar->tiles[l].height )
                	row_height[pos] = tbar->tiles[l].height;
                /* floating row has at least one element padded/resizable in vertical dir,
                 * and no fixed elements */
                if( !ASTileVFloating(tbar->tiles[l]) )
                    floating_rows[pos] = -1;
                else if( floating_rows[pos] >= 0 ) 
                    ++floating_rows[pos] ;
            }
        }
    /* pass 2: see how much space we have left that needs to be floating to some rows/columns : */
    space_left_x = tbar->width - (h_bevel_size+tbar->h_border*2+bevel.left_inline+bevel.right_inline);
    space_left_y = tbar->height- (v_bevel_size+tbar->v_border*2+bevel.top_inline+bevel.bottom_inline);
    LOCAL_DEBUG_OUT( "from: space_left_x = %d, space_left_y = %d", space_left_x, space_left_y );
    for( l = 0 ; l < AS_TileColumns ; ++l )
    {
        if( col_width[l] > 0 )
            space_left_x -= col_width[l]+tbar->h_spacing ;
        if( row_height[l] > 0 )
            space_left_y -= row_height[l]+tbar->v_spacing ;
        if( floating_cols[l] > 0 )
            ++floating_cols_count;
        else
            floating_cols[l] = 0 ;
        if( floating_rows[l] > 0)
            ++floating_rows_count;
        else
            floating_rows[l] = 0 ;
    }
    space_left_x += tbar->h_spacing ;
    space_left_y += tbar->v_spacing ;
    LOCAL_DEBUG_OUT( "to  : space_left_x = %d, space_left_y = %d, floating_cols = %d, floating_rows = %d", 
                     space_left_x, space_left_y, floating_cols_count, floating_rows_count );
    LOCAL_DEBUG_OUT( "h_spacing = %d, v_spacing = %d", tbar->h_spacing, tbar->v_spacing );
    /* pass 3: now we determine spread padding among affected cols : */
    if( floating_cols_count > 0 && space_left_x != 0)
        for( l = 0 ; l < AS_TileColumns ; ++l )
        {
            if( floating_cols[l] > 0 )
            {
                register int change = space_left_x/floating_cols_count ;
                if( change == 0 )
                    change = (space_left_x>0)?1:-1 ;
                col_width[l] += change;
                if( col_width[l] < 0 )
                {
                    change -= col_width[l];
                    col_width[l] = 0 ;
                }
                --floating_cols_count ;
                space_left_x -= change ;
                if( space_left_x == 0 )
                    break;
            }
        }
    /* pass 4: now we determine spread padding among affected rows : */
    if( floating_rows_count > 0 && space_left_y != 0)
        for( l = 0 ; l < AS_TileRows ; ++l )
        {
            if( floating_rows[l] > 0 )
            {
                register int change = space_left_y/floating_rows_count ;
                if( change == 0 )
                    change = (space_left_y>0)?1:-1 ;
                row_height[l] += change;
                if( row_height[l] < 0 )
                {
                    change -= row_height[l];
                    row_height[l] = 0;
                }
                --floating_rows_count ;
                space_left_y -= change ;
                if( space_left_y == 0 )
                    break;
            }
        }

    /* pass 5: now we determine offset of each row/column : */
    x = bevel.left_outline+bevel.left_inline+tbar->h_border ;
    y = bevel.top_outline +bevel.top_inline+tbar->v_border ;
    for( l = 0 ; l < AS_TileColumns ; ++l )
    {
        col_x[l] = x ;
        row_y[l] = y ;
        if( col_width[l] > 0 )
            x += col_width[l]+tbar->h_spacing ;
        if( row_height[l] > 0 )
            y += row_height[l]+tbar->v_spacing ;
    }
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
    for( l = 0 ; l < AS_TileColumns ; ++l )
        show_progress("\tcolumn[%d] = %d%+d floating?%d", l, col_width[l], col_x[l], floating_cols[l]);
    for( l = 0 ; l < AS_TileRows ; ++l )
        show_progress("\trow[%d] = x%d%+d floating?%d", l, row_height[l], row_y[l], floating_rows[l]);
#endif
    /* Done with layout */

    layers = create_image_layers (good_layers+1);
    scrap_images = safecalloc( good_layers+1, sizeof(ASImage*));
	layers[0].im = back;
	layers[0].bevel = &bevel;
    if( tbar->width > h_bevel_size )
        layers[0].clip_width = tbar->width - h_bevel_size;
    else
        layers[0].clip_width = 1;

    if( tbar->height > v_bevel_size )
        layers[0].clip_height = tbar->height - v_bevel_size;
    else
        layers[0].clip_height = 1;

    /* now we need to loop through tiles and add them to the layers list at correct locations */
    good_layers = 1;
    for( l = 0 ; l < tbar->tiles_num ; ++l )
    {
        int type = ASTileType(tbar->tiles[l]);
        if( ASTileTypeHandlers[type].set_layer_handler )
        {
            int row =  ASTileRow(tbar->tiles[l]);
            int col =  ASTileCol(tbar->tiles[l]);
            int pad_x = 0, pad_y = 0 ;
            if( !ASTileHResizeable( tbar->tiles[l] ) )
                pad_x = make_tile_pad(  get_flags(tbar->tiles[l].flags, AS_TilePadLeft),
                                        get_flags(tbar->tiles[l].flags, AS_TilePadRight),
                                        col_width[col], tbar->tiles[l].width ) ;
            tbar->tiles[l].x = col_x[col] + pad_x;

            if( !ASTileVResizeable( tbar->tiles[l] ) )
                pad_y = make_tile_pad(  get_flags(tbar->tiles[l].flags, AS_TilePadTop),
                                        get_flags(tbar->tiles[l].flags, AS_TilePadBottom),
                                        row_height[row],tbar->tiles[l].height);
            tbar->tiles[l].y = row_y[row] + pad_y;
            good_layers += ASTileTypeHandlers[type].set_layer_handler(&(tbar->tiles[l]), &(layers[good_layers]), state, &(scrap_images[good_layers]), col_width[col]-pad_x, row_height[row]-pad_y );
        }
    }
    merge_func = mystyle_translate_texture_type( tbar->composition_method[state] );
    for( l = 1 ; l < good_layers ; ++l )
        layers[l].merge_scanlines = merge_func ;

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
    show_progress("MERGING TBAR %p image %dx%d using merge_func %d FROM:",
                  tbar, tbar->width, tbar->height, tbar->composition_method[state] );
    print_astbar_tiles(tbar);
    show_progress("USING %d layers:", good_layers);
    for( l = 0 ; l < good_layers ; ++l )
    {
        show_progress( "\t %3.3d: %p %+d%+d %ux%u%+d%+d", l, layers[l].im,
                       layers[l].dst_x, layers[l].dst_y,
                       layers[l].clip_width, layers[l].clip_height, layers[l].clip_x, layers[l].clip_y );
    }
#endif
    render_mask = (style->texture_type == TEXTURE_SHAPED_PIXMAP ||
                   style->texture_type == TEXTURE_SHAPED_SCALED_PIXMAP ||
                   get_flags( pc->state, CANVAS_FORCE_MASK ));
#ifdef SHAPE
    LOCAL_DEBUG_OUT( "render_mask = %d, shape = %p", render_mask, pc->shape );
    if ( render_mask )
		fmt = ASA_ASImage;
	else if (pc->shape)
 		fill_canvas_mask (pc, tbar->win_x, tbar->win_y, tbar->width, tbar->height );
#endif
    merged_im = merge_layers (Scr.asv, &layers[0], good_layers, tbar->width, tbar->height, fmt, 0, ASIMAGE_QUALITY_DEFAULT);
    for( l = 0 ; l < good_layers ; ++l )
        if( scrap_images[l] )
            destroy_asimage( &(scrap_images[l]) );
    free( scrap_images );
    free( layers );

	if (merged_im)
	{
        res = draw_canvas_image (pc, merged_im, tbar->win_x, tbar->win_y);

#ifdef SHAPE
        if ( render_mask)
            draw_canvas_mask (pc, merged_im, tbar->win_x, tbar->win_y);
#endif
		destroy_asimage (&merged_im);
        if( res )
            clear_flags( tbar->state, BAR_FLAGS_REND_PENDING );
    }
    SHOW_TIME("rendering",started);
    return res;
}

int
check_astbar_point( ASTBarData *tbar, int root_x, int root_y )
{
    int context = C_NO_CONTEXT ;
    if( tbar )
    {
LOCAL_DEBUG_OUT( "bar's geometry = %dx%d%+d%+d, pointer posish = %+d%+d", tbar->width, tbar->height, tbar->root_x, tbar->root_y, 	root_x, root_y );
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

void
on_astbar_pointer_action( ASTBarData *tbar, int context, Bool leave )
{
    LOCAL_DEBUG_CALLER_OUT( "%p, %s, %d", tbar, context2text(context), leave );
    if( tbar == NULL )
    {
        tbar = FocusedBar ;
        leave = True ;
    }
    LOCAL_DEBUG_OUT( "%p, %s, %d", tbar, context2text(context), leave );
    if( tbar )
    {
        ASBalloon *balloon = tbar->balloon;
        if( context != 0 && context != C_TITLE && context != tbar->context)
        {
            int i = tbar->tiles_num ;
            LOCAL_DEBUG_OUT( "looking for a tile with context %X in the set of %d tiles", context, i );
            while( --i >= 0 )
            {
                if( ASTileType(tbar->tiles[i]) == AS_TileBtnBlock )
                {
                    ASBtnBlock *bb = (ASBtnBlock*)&(tbar->tiles[i].data.bblock) ;
                    int k = bb->buttons_num ;
                    LOCAL_DEBUG_OUT( "tile %d is a button block - lets see if any of %d buttons have context", i, k );
                    while( --k >= 0 )
                        if( bb->buttons[k].context == context )
                        {
                            balloon = bb->buttons[k].balloon ;
                            LOCAL_DEBUG_OUT( "button %d has required contex. balloon = %p", k, balloon );
                            break;
                        }
                    if( k >= 0 )
                        break;
                }
            }
        }
        if( leave || balloon == NULL )
        {
            withdraw_balloon( balloon );
            if( tbar == FocusedBar )
                FocusedBar = NULL ;
        }else
        {
            display_balloon( balloon );
            FocusedBar = tbar ;
        }
    }
}

void
set_astbar_balloon( ASTBarData *tbar, int context, const char *text, unsigned long encoding )
{
    if( tbar != NULL )
    {
        if( context != 0 && context != C_TITLE && context != tbar->context)
        {
            int i = tbar->tiles_num ;
            LOCAL_DEBUG_OUT( "looking for a tile with context %X in the set of %d tiles", context, i );
            while( --i >= 0 )
            {
                if( ASTileType(tbar->tiles[i]) == AS_TileBtnBlock )
                {
                    ASBtnBlock *bb = (ASBtnBlock*)&(tbar->tiles[i].data.bblock);
                    int k = bb->buttons_num ;
                    LOCAL_DEBUG_OUT( "tile %d is a button block - lets see if any of %d buttons have context", i, k );
                    while( --k >= 0 )
                        if( bb->buttons[k].context == context )
                        {
                            if( bb->buttons[k].balloon != NULL )
                            {
                                balloon_set_text (bb->buttons[k].balloon, text, encoding);
                                LOCAL_DEBUG_OUT( "changed balloon for tbar(%p)->context(0x%X)->button(%d)->text(%s)->balloon(%p)",
                                                 tbar, context, k, text, bb->buttons[k].balloon );
                            }else
                            {
                                bb->buttons[k].balloon = create_asballoon_with_text( tbar, text, encoding );
                                LOCAL_DEBUG_OUT( "created balloon for tbar(%p)->context(0x%X)->button(%d)->text(%s)->balloon(%p)",
                                                 tbar, context, k, text, bb->buttons[k].balloon );
                            }
                            return ;
                        }
                }
            }
        }else
        {
            if( tbar->balloon != NULL )
            {
                balloon_set_text (tbar->balloon, text, encoding);
                LOCAL_DEBUG_OUT( "changed tbar balloon for tbar(%p)->context(0x%X)->text(%s)->balloon(%p)",
                                                 tbar, context, text, tbar->balloon );
            }else
            {
                tbar->balloon = create_asballoon_with_text( tbar, text, encoding );
                LOCAL_DEBUG_OUT( "created tbar balloon for tbar(%p)->context(0x%X)->text(%s)->balloon(%p)",
                                 tbar, context, text, tbar->balloon );
            }

        }
    }
}


