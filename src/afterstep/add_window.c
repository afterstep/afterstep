/*
 * Copyright (c) 2002 Sasha Vasko <sasha@aftercode.net>
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

/**********************************************************************
 *
 * Add a new window, put the titlbar and other stuff around
 * the window
 *
 **********************************************************************/
#define LOCAL_DEBUG

#include "../../configure.h"

#include "asinternals.h"

/* icon geometry relative to the root window :                      */
Bool get_icon_root_geometry( ASWindow *asw, ASRectangle *geom )
{
    ASCanvas *canvas = NULL ;
    if( AS_ASSERT(asw) || AS_ASSERT(geom) )
        return False;

    geom->height = 0 ;
    if( asw->icon_canvas )
    {
        canvas = asw->icon_canvas ;
        if( asw->icon_title_canvas && asw->icon_title_canvas != canvas )
            geom->height =  asw->icon_title_canvas->height ;
    }else if( asw->icon_title_canvas )
        canvas = asw->icon_title_canvas ;

    if( canvas )
    {
        geom->x = canvas->root_x ;
        geom->y = canvas->root_y ;
        geom->width  = canvas->width ;
        geom->height += canvas->height ;
        return True;
    }
    return False;
}

/* this gets called when Root background changes : */
void
update_window_transparency( ASWindow *asw )
{
/* TODO: */

}

#define GetNormalBarHeight(h,b,od)  \
    do{if((b)!=NULL){ *((od)->in_width)=(b)->width; *((od)->in_height)=(b)->height;(h) = *((od)->out_height);} \
       else (h) = 0; \
      }while(0)


static unsigned int make_corner_addon( ASOrientation *od, ASTBarData *longbar, ASTBarData *corner1, ASTBarData *corner2 )
{
    unsigned int longbar_height = 0, c1_height = 0, c2_height = 0 ;
    GetNormalBarHeight( longbar_height, longbar, od );
    GetNormalBarHeight( c1_height, corner1, od );
    GetNormalBarHeight( c2_height, corner2, od );
    if( c1_height >= c2_height )
        return (c1_height > longbar_height)?c1_height - longbar_height:0;
    else
        return (c2_height > longbar_height)?c2_height - longbar_height:0;
}

static void
resize_canvases( ASWindow *asw, ASOrientation *od, unsigned int normal_width, unsigned int normal_height, unsigned int *frame_sizes )
{
    unsigned short tbar_size = frame_sizes[od->tbar_side] ;
    unsigned short sbar_size = frame_sizes[od->sbar_side] ;

    /* we need to determine if corners are bigger then frame bars on sbar and tbar : */
    tbar_size += make_corner_addon( od, asw->frame_bars[od->tbar_side], asw->frame_bars[od->tbar_mirror_corners[0]], asw->frame_bars[od->tbar_mirror_corners[1]] );
    sbar_size += make_corner_addon( od, asw->frame_bars[od->sbar_side], asw->frame_bars[od->sbar_mirror_corners[0]], asw->frame_bars[od->sbar_mirror_corners[1]] );

    *(od->in_width)  = normal_width ;
    *(od->in_height) = tbar_size;
    if( asw->frame_sides[od->tbar_side] )
        moveresize_canvas( asw->frame_sides[od->tbar_side], 0, 0, *(od->out_width), *(od->out_height) );

    *(od->in_x) = 0 ;
    *(od->in_y) = normal_height - sbar_size ;
    *(od->in_height) = sbar_size;
    if( asw->frame_sides[od->sbar_side] )
        moveresize_canvas( asw->frame_sides[od->sbar_side], *(od->out_x), *(od->out_y), *(od->out_width), *(od->out_height) );

    /* for left and right sides - somewhat twisted logic - we mirror sides over lt2rb diagonal in case of
     * vertical title orientation. That allows us to apply simple x/y switching instead of complex
     * calculations. Note that we only do that for placement purposes. Contexts and images are still taken
     * from MyFrame parts as if it was rotated counterclockwise instead of mirrored.
     */
    *(od->in_x) = 0 ;
    *(od->in_y) = tbar_size ;
    *(od->in_width)  = frame_sizes[od->left_mirror_side] ;
    *(od->in_height) = normal_height-(tbar_size+sbar_size) ;
    if( asw->frame_sides[od->left_mirror_side] )
        moveresize_canvas( asw->frame_sides[od->left_mirror_side], *(od->out_x), *(od->out_y), *(od->out_width), *(od->out_height));

    *(od->in_x) = normal_width-frame_sizes[od->right_mirror_side] ;
    *(od->in_width)  = frame_sizes[od->right_mirror_side] ;
    if( asw->frame_sides[od->right_mirror_side] )
        moveresize_canvas( asw->frame_sides[od->right_mirror_side], *(od->out_x), *(od->out_y), *(od->out_width), *(od->out_height));
}

#if 0
static unsigned short
frame_side_height(ASCanvas *c1, ASCanvas *c2 )
{
    unsigned short h = 0 ;
    if( c1 )
        h += c1->height ;
    if( c2 )
        h += c2->height ;
    return h;
}

static unsigned short
frame_side_width(ASCanvas *c1, ASCanvas *c2 )
{
    unsigned short w = 0 ;
    if( c1 )
        w += c1->width ;
    if( c2 )
        w += c2->width ;
    return w;
}

static unsigned short
frame_corner_height(ASTBarData *c1, ASTBarData *c2 )
{
    unsigned short h = 0 ;
    if( c1 )
        h += c1->height ;
    if( c2 )
        h += c2->height ;
    return h;
}

static unsigned short
frame_corner_width(ASTBarData *c1, ASTBarData *c2 )
{
    unsigned short w = 0 ;
    if( c1 )
        w += c1->width ;
    if( c2 )
        w += c2->width ;
    return w;
}
#endif

static int
make_shade_animation_step( ASWindow *asw, ASOrientation *od )
{
    int step_size = 0;
    if( asw->tbar )
    {
        int steps = asw->shading_steps;

        if( steps > 0 )
        {
            int from_size, to_size ;
            *(od->in_width) = asw->frame_canvas->width ;
            *(od->in_height) = asw->frame_canvas->height ;
            from_size = *(od->out_height);
            if( ASWIN_GET_FLAGS( asw, AS_Shaded ) )
            {
                *(od->in_width) = asw->tbar->width ;
                *(od->in_height) = asw->tbar->height ;
            }else
            {
                *(od->in_width) = asw->status->width ;
                *(od->in_height) = asw->status->height ;
            }
            to_size = *(od->out_height);

            if(from_size != to_size)
            {
                int step_delta = (from_size - to_size)/steps ;
                if( step_delta == 0 )
                    step_delta = (from_size > to_size)?1:-1;
LOCAL_DEBUG_OUT( "@@ANIM to(%d)->from(%d)->delta(%d)->step(%d)", to_size, from_size, step_delta, steps );

                step_size = from_size-step_delta;
                --(asw->shading_steps);
            }
        }else if( !ASWIN_GET_FLAGS( asw, AS_Shaded ) )
            return 0;
        else
        {
            *(od->in_width) = asw->tbar->width ;
            *(od->in_height) = asw->tbar->height ;
            return *(od->out_height);
        }
    }
    return step_size;
}

inline static Bool
move_resize_frame_bar( ASTBarData *tbar, ASCanvas *canvas, int normal_x, int normal_y, unsigned int normal_width, unsigned int normal_height, Bool force_render )
{
    if( set_astbar_size( tbar, normal_width, normal_height) )
        force_render = True;
    if( move_astbar( tbar, canvas, normal_x, normal_y ) )
        force_render = True;
    if( force_render )
        render_astbar( tbar, canvas );
    return force_render;
}

inline static Bool
move_resize_corner( ASTBarData *bar, ASCanvas *canvas, ASOrientation *od, int normal_y, unsigned int normal_width, unsigned int normal_height, Bool left, Bool force_render )
{
    unsigned int w = bar->width;
    unsigned int h = bar->height;

    *(od->in_y) = normal_y;

    if( od == &VertOrientation )
        w = normal_height ;
    else
        h = normal_height ;
    *(od->in_width) = w ;
    *(od->in_height) = h ;
    *(od->in_x) = left?0:(normal_width-(*(od->out_width)));
    return move_resize_frame_bar( bar, canvas, *(od->out_x), *(od->out_y), w, h, force_render );
}

inline static Bool
move_resize_longbar( ASTBarData *bar, ASCanvas *canvas, ASOrientation *od,
                     int normal_y, unsigned int normal_width,
                     unsigned int corner_size1, unsigned int corner_size2,
                     Bool vertical, Bool force_render )
{
    unsigned int w = bar->width;
    unsigned int h = bar->height;
    int bar_size ;

    /* swapping bar height in case of vertical orientation : */
    *(od->in_width) = w ;
    *(od->in_height) = h ;
    if( vertical )
    {
        bar_size = *(od->out_width);
        *(od->in_width)  = bar_size ;
        *(od->in_height) = normal_width ;
        *(od->in_x) = normal_y;
        *(od->in_y) = corner_size1;
    }else
    {
        bar_size = *(od->out_height);
        *(od->in_height) = bar_size ;
        if( corner_size1+corner_size2 > normal_width )
            *(od->in_width) = 1;
        else
            *(od->in_width) = normal_width-(corner_size1+corner_size2) ;
        *(od->in_x) = corner_size1;
        *(od->in_y) = normal_y;
    }

    return move_resize_frame_bar( bar, canvas, *(od->out_x), *(od->out_y), *(od->out_width), *(od->out_height), force_render );
}

static Bool
move_resize_frame_bars( ASWindow *asw, int side, ASOrientation *od, unsigned int normal_width, unsigned int normal_height, Bool force_render )
{
    int corner_size1 = 0, corner_size2 = 0;
    Bool rendered = False ;
    int tbar_size = 0;
    ASCanvas *canvas = asw->frame_sides[side];
    ASTBarData *title = NULL, *corner1 = NULL, *longbar = NULL, *corner2 = NULL;
    Bool vertical = False ;

    LOCAL_DEBUG_CALLER_OUT("%p,%d, %ux%u, %s", asw, side, normal_width, normal_height, force_render?"force":"don't force" );
    longbar = asw->frame_bars[side];
    if( side == od->tbar_side )
    {
        title = asw->tbar ;
        corner1 = asw->frame_bars[od->tbar_mirror_corners[0]];
        corner2 = asw->frame_bars[od->tbar_mirror_corners[1]];
    }else if( side == od->sbar_side )
    {
        corner1 = asw->frame_bars[od->sbar_mirror_corners[0]];
        corner2 = asw->frame_bars[od->sbar_mirror_corners[1]];
    }else
        vertical = True ;

    if( title )
    {
        if( move_resize_longbar( title, canvas, od, 0, normal_width, 0, 0, False, force_render ) )
            rendered = True;
        tbar_size = *(od->out_height);
    }
    /* mirror_corner 0 */
    if( corner1 )
    {
        if( move_resize_corner( corner1, canvas, od, tbar_size, normal_width, normal_height, True, force_render ) )
            rendered = True ;
        corner_size1 = *(od->out_width);
    }
    /* mirror_corner 1 */
    if( corner2 )
    {
        if( move_resize_corner( corner2, canvas, od, tbar_size, normal_width, normal_height, False, force_render ) )
            rendered = True ;
        corner_size2 = *(od->out_width);
    }
    /* side */
    if( longbar )
    {
        if( title == NULL && corner_size1>0 && corner_size2 > 0 )                    /* we are in the sbar */
        {
            *(od->in_width) = longbar->width ;
            *(od->in_height) = longbar->height ;
            tbar_size = normal_height - (int)(*(od->out_height));
            if( tbar_size < 0 )
                tbar_size = 0 ;
        }
        if( move_resize_longbar( longbar, canvas, od,
                                 tbar_size, vertical?normal_height:normal_width,
                                 corner_size1, corner_size2,
                                 vertical, force_render ) )
            rendered = True;
    }

    return rendered;
}

static void
apply_window_status_size(register ASWindow *asw, ASOrientation *od)
{
    /* note that icons are handled by iconbox */
    if( !ASWIN_GET_FLAGS( asw, AS_Iconic ) )
	{
        int step_size = make_shade_animation_step( asw, od );
LOCAL_DEBUG_OUT( "**CONFG Client(%lx(%s))->status(%ux%u%+d%+d,%s,%s(%d>-%d))", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", asw->status->width, asw->status->height, asw->status->x, asw->status->y, ASWIN_HFLAGS(asw, AS_VerticalTitle)?"Vert":"Horz", step_size>0?"Shaded":"Unshaded", asw->shading_steps, step_size );
        if( step_size > 0 )
        {
            if( asw->frame_sides[od->tbar_side] )
                XRaiseWindow( dpy, asw->frame_sides[od->tbar_side]->w );
            if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
                XLowerWindow( dpy, asw->w );
            if( ASWIN_HFLAGS(asw, AS_VerticalTitle) )
                moveresize_canvas( asw->frame_canvas, asw->status->x, asw->status->y,
                                   step_size, asw->status->height );
            else
                moveresize_canvas( asw->frame_canvas, asw->status->x, asw->status->y,
                                   asw->status->width, step_size );
        }else
        {
            if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
                XRaiseWindow( dpy, asw->w );
            moveresize_canvas(  asw->frame_canvas, asw->status->x, asw->status->y,
                                asw->status->width, asw->status->height );
        }
    }
}

static void
on_frame_bars_moved( ASWindow *asw, unsigned int side, ASOrientation *od)
{
    ASCanvas *canvas = asw->frame_sides[side];

    update_astbar_transparency(asw->frame_bars[side], canvas);
    if( side == od->tbar_side )
    {
        update_astbar_transparency(asw->tbar, canvas);
        update_astbar_transparency(asw->frame_bars[od->tbar_mirror_corners[0]], canvas);
        update_astbar_transparency(asw->frame_bars[od->tbar_mirror_corners[1]], canvas);
    }else if( side == od->sbar_side )
    {
        update_astbar_transparency(asw->frame_bars[od->sbar_mirror_corners[0]], canvas);
        update_astbar_transparency(asw->frame_bars[od->sbar_mirror_corners[1]], canvas);
    }

}

static void
update_window_frame_moved( ASWindow *asw, ASOrientation *od )
{
    int i ;
    handle_canvas_config (asw->client_canvas);
    if( asw->internal && asw->internal->on_moveresize )
        asw->internal->on_moveresize( asw->internal, None );

    for( i = 0 ; i < FRAME_SIDES ; ++i )
        if( asw->frame_sides[i] )
        {   /* canvas has beer resized - resize tbars!!! */
            handle_canvas_config (asw->frame_sides[i]);
            on_frame_bars_moved( asw, i, od);
        }
}

void
SendConfigureNotify(ASWindow *asw)
{
    XEvent client_event ;

    if( ASWIN_GET_FLAGS(asw, AS_Dead|AS_MoveresizeInProgress ) )
        return;

    client_event.type = ConfigureNotify;
    client_event.xconfigure.display = dpy;
    client_event.xconfigure.event = asw->w;
    client_event.xconfigure.window = asw->w;

    client_event.xconfigure.x = asw->client_canvas->root_x;
    client_event.xconfigure.y = asw->client_canvas->root_y;
    client_event.xconfigure.width = asw->client_canvas->width;
    client_event.xconfigure.height = asw->client_canvas->height;

    client_event.xconfigure.border_width = asw->status->border_width;
    /* Real ConfigureNotify events say we're above title window, so ... */
    /* what if we don't have a title ????? */
    client_event.xconfigure.above = asw->frame_canvas[FR_N].w?asw->frame_canvas[FR_N].w:asw->frame;
    client_event.xconfigure.override_redirect = False;
    XSendEvent (dpy, asw->w, False, StructureNotifyMask, &client_event);
}


/* this gets called when StructureNotify/SubstractureNotify arrives : */
void
on_window_moveresize( ASWindow *asw, Window w )
{
    int i ;
    ASOrientation *od ;
    unsigned int normal_width, normal_height ;

LOCAL_DEBUG_CALLER_OUT( "(%p,%lx,asw->w=%lx)", asw, w, asw->w );
    if( AS_ASSERT(asw) )
        return ;

    od = get_orientation_data(asw);

    if( w == asw->w )
    {  /* simply update client's size and position */
        ASFlagType changes ;
        if( ASWIN_GET_FLAGS(asw, AS_Dead) )
            return;
        changes = handle_canvas_config (asw->client_canvas);
        if( asw->internal && asw->internal->on_moveresize )
            asw->internal->on_moveresize( asw->internal, w );

        if( changes != 0 )
        {
            if( ASWIN_GET_FLAGS( asw, AS_Shaped|AS_ShapedDecor ) )
                SetShape( asw, 0 );
            broadcast_config (M_CONFIGURE_WINDOW, asw);
        }
    }else if( w == asw->frame )
    {/* resize canvases here :*/
        int changes = handle_canvas_config (asw->frame_canvas);
LOCAL_DEBUG_OUT( "changes=0x%X", changes );
        /* we must resize using current window size instead of event's size */
        *(od->in_width)  = asw->frame_canvas->width ;
        *(od->in_height) = asw->frame_canvas->height ;
        normal_width  = *(od->out_width)  ;
        normal_height = *(od->out_height) ;

        if( get_flags( changes, CANVAS_RESIZED ) )
        {
            register unsigned int *frame_size = &(asw->status->frame_size[0]) ;
            int step_size = make_shade_animation_step( asw, od );
            if( step_size <= 0 )  /* don't moveresize client window while shading !!!! */
            {
                resize_canvases( asw, od, normal_width, normal_height, frame_size );
                if( !ASWIN_GET_FLAGS(asw, AS_Shaded ) )  /* leave shaded client alone ! */
                    moveresize_canvas( asw->client_canvas,
                                    frame_size[FR_W],
                                    frame_size[FR_N],
                                    asw->frame_canvas->width-(frame_size[FR_W]+frame_size[FR_E]),
                                    asw->frame_canvas->height-(frame_size[FR_N]+frame_size[FR_S]));
            }else if( normal_height != step_size )
            {
                sleep_a_little(10000);
                /* we get smoother animation if we move decoration ahead of actually
                 * resizing frame window : */
                resize_canvases( asw, od, normal_width, step_size, frame_size );
                *(od->in_width)=normal_width ;
                *(od->in_height)=step_size ;
/*LOCAL_DEBUG_OUT( "**SHADE Client(%lx(%s))->(%d>-%d)", asw->w, ASWIN_NAME(asw)?ASWIN_NAME(asw):"noname", asw->shading_steps, step_size );*/
                resize_canvas( asw->frame_canvas, *(od->out_width), *(od->out_height));
                ASSync(False);
            }
        }else if( get_flags( changes, CANVAS_MOVED ) )
        {
            update_window_frame_moved( asw, od );
            broadcast_config (M_CONFIGURE_WINDOW, asw);
            /* also sent synthetic ConfigureNotify : */
            SendConfigureNotify(asw);
        }

        if( changes != 0 )
        {
            update_window_transparency( asw );
            if( get_flags( changes, CANVAS_RESIZED ) && ASWIN_GET_FLAGS( asw, AS_ShapedDecor|AS_Shaped ))
                SetShape( asw, 0 );
        }
    }else if( asw->icon_canvas && w == asw->icon_canvas->w )
    {
        ASFlagType changes = handle_canvas_config (asw->icon_canvas);
        LOCAL_DEBUG_OUT( "icon resized to %dx%d%+d%+d", asw->icon_canvas->width, asw->icon_canvas->height, asw->icon_canvas->root_x, asw->icon_canvas->root_y );
        if( get_flags(changes, CANVAS_RESIZED) )
        {
            unsigned short title_size = 0 ;
            if( asw->icon_title && (asw->icon_title_canvas == asw->icon_canvas || asw->icon_title_canvas == NULL ) )
            {
                title_size = calculate_astbar_height( asw->icon_title );
                move_astbar( asw->icon_title, asw->icon_canvas, 0, asw->icon_canvas->height - title_size );
                set_astbar_size( asw->icon_title, asw->icon_canvas->width, title_size );
                render_astbar( asw->icon_title, asw->icon_canvas );
                LOCAL_DEBUG_OUT( "title_size = %d", title_size );
            }
            set_astbar_size( asw->icon_button, asw->icon_canvas->width, asw->icon_canvas->height-title_size );
            render_astbar( asw->icon_button, asw->icon_canvas );
            update_canvas_display( asw->icon_canvas );
        }
        broadcast_config (M_CONFIGURE_WINDOW, asw);
    }else if( asw->icon_title_canvas && w == asw->icon_title_canvas->w )
    {
        if( handle_canvas_config(asw->icon_title_canvas) && asw->icon_title )
        {
            LOCAL_DEBUG_OUT( "icon_title resized to %dx%d%+d%+d", asw->icon_title_canvas->width, asw->icon_title_canvas->height, asw->icon_title_canvas->root_x, asw->icon_title_canvas->root_y );
            set_astbar_size( asw->icon_title, asw->icon_title_canvas->width, asw->icon_title->height );
            render_astbar( asw->icon_title, asw->icon_title_canvas );
            update_canvas_display( asw->icon_title_canvas );
        }
        broadcast_config (M_CONFIGURE_WINDOW, asw);
    }else  /* one of the frame canvases :*/
    {
        Bool found = False;
        for( i = 0 ; i < FRAME_SIDES ; ++i )
            if( asw->frame_sides[i] && asw->frame_sides[i]->w == w )
            {   /* canvas has beer resized - resize tbars!!! */
                Bool changes = handle_canvas_config (asw->frame_sides[i]);

                /* we must resize using current window size instead of event's size */
                *(od->in_width)  = asw->frame_sides[i]->width ;
                *(od->in_height) = asw->frame_sides[i]->height ;
                normal_width  = *(od->out_width)  ;
                normal_height = *(od->out_height) ;

                /* don't redraw window decoration while in the middle of animation : */
                if( asw->shading_steps<= 0 )
                {
                    if( move_resize_frame_bars( asw, i, od, normal_width, normal_height, changes) ||
                        changes )  /* now we need to show them on screen !!!! */
                    {
                        update_canvas_display( asw->frame_sides[i] );
                        if( ASWIN_GET_FLAGS( asw, AS_Shaped )  )
                            SetShape( asw, 0 );
                    }
                }
                found = True;
                break;
            }
        if( !found )
            if( asw->internal && asw->internal->on_moveresize )
                asw->internal->on_moveresize( asw->internal, w );

    }
    ASSync(False);
}

void
on_window_title_changed( ASWindow *asw, Bool update_display )
{
    if( AS_ASSERT(asw) )
        return ;
    if( asw->tbar )
    {
        ASCanvas *canvas = ASWIN_HFLAGS(asw, AS_VerticalTitle)?asw->frame_sides[FR_W]:asw->frame_sides[FR_N];
        if( change_astbar_first_label( asw->tbar, ASWIN_NAME(asw) ) )
            if( canvas && update_display )
            {
                render_astbar( asw->tbar, canvas );
                update_canvas_display( canvas );
            }
    }
    if( asw->icon_title )
    {
        if( change_astbar_first_label( asw->icon_title, ASWIN_ICON_NAME(asw) ) )
            on_icon_changed( asw );
    }
}

void
on_window_hints_changed( ASWindow *asw )
{
    static ASRawHints raw_hints ;
    ASHints *hints = NULL;

    if( AS_ASSERT(asw) )
        return ;

    if( !collect_hints( &Scr, asw->w, HINT_ANY, &raw_hints ) )
        return ;
    if( !is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
        print_hints( NULL, NULL, &raw_hints );
    hints = merge_hints( &raw_hints, Database, NULL, Scr.Look.supported_hints, HINT_ANY, NULL );
    destroy_raw_hints( &raw_hints, True );
    if( hints )
    {
        show_debug( __FILE__, __FUNCTION__, __LINE__,  "Window management hints collected and merged for window %X", asw->w );
        if( !is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
            print_clean_hints( NULL, NULL, hints );
    }else
    {
        show_warning( "Failed to merge window management hints for window %X", asw->w );
        return ;
    }

    untie_aswindow( asw );
    destroy_hints (asw->hints, True);
    asw->hints = hints ;
    tie_aswindow( asw );
    SelectDecor (asw);

    /* we need to do the complete refresh of decorations : */
    LOCAL_DEBUG_OUT( "redecorating window %p(\"%s\") to update to new hints...", asw, hints->names[0]?hints->names[0]:"none" );
    redecorate_window       ( asw, False );

    on_window_title_changed ( asw, False );
    on_window_status_changed( asw, False, True );

    broadcast_config (M_CONFIGURE_WINDOW, asw);

    broadcast_window_name( asw );
    broadcast_res_names( asw );
    broadcast_icon_name( asw );
}

void
on_window_status_changed( ASWindow *asw, Bool update_display, Bool reconfigured )
{
    char *unfocus_mystyle = NULL ;
    char *frame_unfocus_mystyle = NULL ;
    int i ;
    Bool changed = False;
    ASOrientation *od = get_orientation_data( asw );

    if( AS_ASSERT(asw) )
        return ;

LOCAL_DEBUG_CALLER_OUT( "(%p,%s Update display,%s Reconfigured)", asw, update_display?"":"Don't", reconfigured?"":"Not" );
    if( ASWIN_GET_FLAGS(asw, AS_Iconic ) )
    {
        unfocus_mystyle = ASWIN_GET_FLAGS(asw, AS_Sticky )?
                            AS_ICON_TITLE_STICKY_MYSTYLE:
                            AS_ICON_TITLE_UNFOCUS_MYSTYLE ;
        if( asw->icon_title )
            changed = set_astbar_style( asw->icon_title, BAR_STATE_UNFOCUSED, unfocus_mystyle );
        if( changed ) /* now we need to update icon title size */
            on_icon_changed( asw );
    }else
    {
        Bool decor_shaped = False ;
        int back_type;
        ASFlagType *frame_bevel, title_bevel ;
        int title_cm ;

        if( ASWIN_GET_FLAGS(asw, AS_Sticky ) )
        {
            back_type = BACK_STICKY ;
            frame_bevel = &(asw->frame_data->part_sbevel[0]);
            title_bevel = asw->frame_data->title_sbevel;
            title_cm    = asw->frame_data->title_scm ;
        }else
        {
            back_type = BACK_UNFOCUSED ;
            frame_bevel = &(asw->frame_data->part_ubevel[0]);
            title_bevel = asw->frame_data->title_ubevel;
            title_cm    = asw->frame_data->title_ucm;
        }

        unfocus_mystyle = asw->hints->mystyle_names[back_type];
        if( asw->frame_data->title_style_names[back_type] )
            unfocus_mystyle = asw->frame_data->title_style_names[back_type] ;
        if( unfocus_mystyle == NULL )
            unfocus_mystyle = Scr.Look.MSWindow[back_type]->name ;
        if( unfocus_mystyle == NULL )
            unfocus_mystyle = Scr.Look.MSWindow[BACK_UNFOCUSED]->name ;

        frame_unfocus_mystyle = (asw->frame_data->frame_style_names[back_type]==NULL)?
                                        unfocus_mystyle : asw->frame_data->frame_style_names[back_type];

        for( i = 0 ; i < FRAME_PARTS ; ++i )
            if( asw->frame_bars[i] )
            {
                if( set_astbar_style( asw->frame_bars[i], BAR_STATE_UNFOCUSED, frame_unfocus_mystyle ) )
                    changed = True ;
                if( set_astbar_hilite( asw->frame_bars[i], BAR_STATE_UNFOCUSED, frame_bevel[i] ) )
                    changed = True ;
                if( is_astbar_shaped( asw->frame_bars[i], -1 ) )
                    decor_shaped = True;
            }

        if( asw->tbar )
        {
            if( set_astbar_style( asw->tbar, BAR_STATE_UNFOCUSED, unfocus_mystyle ) )
                changed = True ;
            if( set_astbar_hilite( asw->tbar, BAR_STATE_UNFOCUSED, title_bevel ) )
                changed = True ;
            if( set_astbar_composition_method( asw->tbar, BAR_STATE_UNFOCUSED, title_cm ) )
                changed = True ;
            if( is_astbar_shaped( asw->tbar, -1 ) )
                decor_shaped = True;
        }
        if( decor_shaped )
            ASWIN_SET_FLAGS( asw, AS_ShapedDecor );
        else
            ASWIN_CLEAR_FLAGS( asw, AS_ShapedDecor );

        if( changed || reconfigured )
        {/* now we need to update frame sizes in status */
            unsigned int *frame_size = &(asw->status->frame_size[0]) ;
            unsigned short tbar_size = 0;
            if( ASWIN_HFLAGS(asw, AS_VerticalTitle) )
            {
                tbar_size = calculate_astbar_width( asw->tbar );
                /* we need that to set up tbar size : */
                set_astbar_size( asw->tbar, tbar_size, asw->tbar?asw->tbar->height:0 );
            }else
            {
                tbar_size = calculate_astbar_height( asw->tbar );
                /* we need that to set up tbar size : */
                set_astbar_size( asw->tbar, asw->tbar?asw->tbar->width:0, tbar_size );
            }
            for( i = 0 ; i < FRAME_SIDES ; ++i )
            {
                if( asw->frame_bars[i] )
                    frame_size[i] = IsSideVertical(i)?asw->frame_bars[i]->width:
                                                    asw->frame_bars[i]->height ;
                else
                    frame_size[i] = 0;
            }
            frame_size[od->tbar_side] += tbar_size ;
            anchor2status ( asw->status, asw->hints, &(asw->anchor));
        }
    }

    /* now we need to move/resize our frame window */
    apply_window_status_size(asw, od);
    if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
        set_client_state( asw->w, asw->status );
}

void
on_window_hilite_changed( ASWindow *asw, Bool focused )
{
    ASOrientation *od = get_orientation_data( asw );

LOCAL_DEBUG_CALLER_OUT( "(%p,%s focused)", asw, focused?"":"not" );
    if( AS_ASSERT(asw) )
        return;

    broadcast_focus_change( asw, focused );

    if(!ASWIN_GET_FLAGS(asw, AS_Iconic))
    {
        register int i = FRAME_PARTS;
        /* Titlebar */
        set_astbar_focused( asw->tbar, asw->frame_sides[od->tbar_side], focused );
        /* frame decor : */
        for( i = FRAME_PARTS; --i >= 0; )
            set_astbar_focused( asw->frame_bars[i], asw->frame_sides[od->tbar2canvas_xref[i]], focused );
        /* now posting all the changes on display :*/
        for( i = FRAME_SIDES; --i >= 0; )
            if( is_canvas_dirty(asw->frame_sides[i]) )
                update_canvas_display( asw->frame_sides[i] );
        if( asw->internal && asw->internal->on_hilite_changed )
            asw->internal->on_hilite_changed( asw->internal, NULL, focused );
        if( ASWIN_GET_FLAGS( asw, AS_ShapedDecor ) )
            SetShape( asw, 0 );
    }else /* Iconic !!! */
    {
        set_astbar_focused( asw->icon_button, asw->icon_canvas, focused );
        set_astbar_focused( asw->icon_title, asw->icon_title_canvas, focused );
        if( is_canvas_dirty(asw->icon_canvas) )
            update_canvas_display( asw->icon_canvas );
        if( is_canvas_dirty(asw->icon_title_canvas) )
            update_canvas_display( asw->icon_title_canvas );
    }
}

void
on_window_pressure_changed( ASWindow *asw, int pressed_context )
{
    ASOrientation *od = get_orientation_data( asw );
LOCAL_DEBUG_CALLER_OUT( "(%p,%s)", asw, context2text(pressed_context));

    if( AS_ASSERT(asw) || asw->status == NULL )
        return;

    if(!ASWIN_GET_FLAGS(asw, AS_Iconic))
    {
        register int i = FRAME_PARTS;
        /* Titlebar */
        set_astbar_btn_pressed (asw->tbar, pressed_context);  /* must go before next call to properly redraw :  */
        set_astbar_pressed( asw->tbar, asw->frame_sides[od->tbar_side], pressed_context&C_TITLE );
        /* frame decor : */
        for( i = FRAME_PARTS; --i >= 0; )
            set_astbar_pressed( asw->frame_bars[i], asw->frame_sides[od->tbar2canvas_xref[i]], pressed_context&(C_FrameN<<i) );
        /* now posting all the changes on display :*/
        for( i = FRAME_SIDES; --i >= 0; )
            if( is_canvas_dirty(asw->frame_sides[i]) )
                update_canvas_display( asw->frame_sides[i] );
        if( asw->internal && asw->internal->on_pressure_changed )
            asw->internal->on_pressure_changed( asw->internal, pressed_context&C_CLIENT );
    }else /* Iconic !!! */
    {
        set_astbar_pressed( asw->icon_button, asw->icon_canvas, pressed_context&C_IconButton );
        set_astbar_pressed( asw->icon_title, asw->icon_title_canvas, pressed_context&C_IconTitle );
        if( is_canvas_dirty(asw->icon_canvas) )
            update_canvas_display( asw->icon_canvas );
        if( is_canvas_dirty(asw->icon_title_canvas) )
            update_canvas_display( asw->icon_title_canvas );
    }
}

/********************************************************************/
/* end of ASWindow frame decorations management                     */

/****************************************************************************
 * Interprets the property MOTIF_WM_HINTS, sets decoration and functions
 * accordingly
 *****************************************************************************/
void
SelectDecor (ASWindow * t)
{
	ASHints *hints = t->hints ;
	ASFlagType tflags = hints->flags ;

#ifndef NO_TEXTURE
    if (!get_flags(hints->function_mask,AS_FuncResize) || !get_flags( Scr.Look.flags, DecorateFrames))
#endif /* !NO_TEXTURE */
	{
		/* a wide border, with corner tiles */
		if (get_flags(tflags,AS_Frame))
			clear_flags( t->hints->flags, AS_Frame );
	}
}

/*
 * The following function was written for
 * new hints management code in libafterstep :
 */
Bool
afterstep_parent_hints_func(Window parent, ASParentHints *dst )
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

void
save_aswindow_anchor( ASWindow *asw, Bool hor, Bool vert )
{
    if( hor && asw->saved_anchor.width == 0 )
    {
        asw->saved_anchor.x = asw->anchor.x ;
        asw->saved_anchor.width = asw->anchor.width ;
    }
    if( vert && asw->saved_anchor.height == 0 )
    {
        asw->saved_anchor.y = asw->anchor.y ;
        asw->saved_anchor.height = asw->anchor.height ;
    }
}

void
moveresize_aswindow_wm( ASWindow *asw, int x, int y, unsigned int width, unsigned int height, Bool save_anchor )
{
    if( !AS_ASSERT(asw) )
    {
        ASStatusHints scratch_status  = *(asw->status);
        scratch_status.x = x ;
        scratch_status.y = y ;
        if( width > 0 )
            scratch_status.width = width  ;
        if( height > 0 )
            scratch_status.height = height ;

        if( ASWIN_GET_FLAGS( asw, AS_Shaded ) )
        {    /* tricky tricky */
            if( ASWIN_HFLAGS(asw,AS_VerticalTitle ) )
                scratch_status.width = asw->status->width ;
            else
                scratch_status.height = asw->status->height ;
        }
        if( save_anchor )
            save_aswindow_anchor( asw, True, True );
        /* need to apply two-way conversion in order to make sure that size restrains are applied : */
        status2anchor( &(asw->anchor), asw->hints, &scratch_status, Scr.VxMax, Scr.VyMax);
        anchor2status ( asw->status, asw->hints, &(asw->anchor));

        /* now lets actually resize the window : */
        apply_window_status_size(asw, get_orientation_data(asw));
    }
}

Bool
init_aswindow_status( ASWindow *t, ASStatusHints *status )
{
    Bool pending_placement = False;

    if( t->status == NULL )
    {
        t->status = safecalloc(1, sizeof(ASStatusHints));
        *(t->status) = *status ;
    }
    if( get_flags( status->flags, AS_StartDesktop) && status->desktop != Scr.CurrentDesk )
        ChangeDesks( status->desktop );

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
        MoveViewport (t->status->viewport_x, t->status->viewport_y, False);

    if( !get_flags(AfterStepState, ASS_NormalOperation) )
    {
        int min_x, min_y, max_x, max_y ;
        int margin = Scr.MyDisplayWidth>>5 ;
        if (!ASWIN_GET_FLAGS( t, AS_Sticky ))
        {
            min_x = -Scr.Vx ;
            max_x = Scr.VxMax+Scr.MyDisplayWidth - Scr.Vx ;
            min_y = -Scr.Vy ;
            max_y = Scr.VyMax+Scr.MyDisplayHeight - Scr.Vy ;
        }else
        {
            min_x = 0 ;
            max_x = Scr.MyDisplayWidth ;
            min_y = 0 ;
            max_y = Scr.MyDisplayHeight ;
        }
        /* we have to make sure that window is visible !!!! */
        if( t->status->x < min_x + margin )
            t->status->x = min_x ;
        else if( t->status->x +t->status->width > max_x-margin )
            t->status->x = max_x - t->status->width ;
        if( t->status->y < min_y + margin )
            t->status->y = min_y ;
        else if( t->status->y +t->status->height > max_y-margin )
            t->status->y = max_y - t->status->height ;

        set_flags( t->status->flags, AS_Position );

    }else if( get_flags(Scr.Feel.flags, NoPPosition) &&
            !get_flags( status->flags, AS_StartPositionUser ) )
        clear_flags( t->status->flags, AS_Position );

    /* TODO: AS_Iconic */
	if( !ASWIN_GET_FLAGS(t, AS_StartLayer ) )
        ASWIN_LAYER(t) = AS_LayerNormal ;
    else if( ASWIN_LAYER(t) < AS_LayerLowest )
        ASWIN_LAYER(t) = AS_LayerLowest ;
    else if( ASWIN_LAYER(t) < AS_LayerHighest )
        ASWIN_LAYER(t) = AS_LayerHighest ;

    if( get_flags( status->flags, AS_MaximizedX|AS_MaximizedY ))
        pending_placement = True ;
    else if( !get_flags( t->status->flags, AS_Position ))
    {
        if( ! get_flags( t->status->flags, AS_StartsIconic ) )
        {
            int x, y ;
            if( get_flags( t->hints->flags, AS_Transient ) )
            {
                x = Scr.MyDisplayWidth / 2 ;
                y = Scr.MyDisplayHeight / 2 ;
            }else
            {
                ASQueryPointerRootXY( &x, &y );
                t->status->x = x - t->status->width/2 ;
                t->status->y = y - t->status->height/2 ;
            }
            pending_placement = True ;
        }
    }

    if( !is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
        print_status_hints( NULL, NULL, t->status );

    /* by now we have a valid position for the window: */
    set_flags( t->status->flags, AS_Position );

    status2anchor( &(t->anchor), t->hints, t->status, Scr.VxMax, Scr.VyMax);
    LOCAL_DEBUG_OUT( "status->geom=%dx%d%+d%+d,status->viewport=%+d%+d,anchor=%dx%d%+d%+d",
                     t->status->width, t->status->height, t->status->x, t->status->y,
                     t->status->viewport_x, t->status->viewport_y,
                     t->anchor.width, t->anchor.height, t->anchor.x, t->anchor.y );

    return pending_placement;
}

/***************************************************************************************/
/* iconify/deiconify code :                                                            */
/***************************************************************************************/
void
complete_wm_state_transition( ASWindow *asw, int state )
{
    asw->wm_state_transition = ASWT_StableState ;
    if( state == NormalState )
    {
        XMapSubwindows(dpy, asw->frame);
        XMapWindow(dpy, asw->frame);
    }else if( state == IconicState )
    {
        XUnmapWindow (dpy, asw->frame);
    }
    if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
        set_multi32bit_property (asw->w, _XA_WM_STATE, _XA_WM_STATE, 2, state, (state==IconicState)?asw->status->icon_window:None);
}

Bool
set_window_wm_state( ASWindow *asw, Bool iconify )
{
    XWindowAttributes attr;

LOCAL_DEBUG_CALLER_OUT( "client = %p, iconify = %d", asw, iconify );

    if( AS_ASSERT(asw) )
        return False;

    if( iconify )
    {
        asw->DeIconifyDesk = ASWIN_DESK(asw);
        if( asw->wm_state_transition == ASWT_StableState )
        {
			if( get_flags( asw->status->flags, AS_Iconic ) )
				return False;

			asw->wm_state_transition = ASWT_Normal2Iconic ;
            set_flags( asw->status->flags, AS_Iconic );
            if( get_flags( Scr.Feel.flags, StickyIcons ) || ASWIN_DESK(asw) == Scr.CurrentDesk )
                quietly_reparent_aswindow( asw, Scr.Root, True );
            else
                quietly_reparent_aswindow( asw, Scr.ServiceWin, True );
        }

        asw->status->icon_window = asw->icon_canvas? asw->icon_canvas->w:None ;

        if( asw->icon_canvas )
            asw->status->icon_window = asw->icon_canvas->w ;
        else if( asw->icon_title_canvas )
            asw->status->icon_window = asw->icon_title_canvas->w ;

        if (get_flags(Scr.Feel.flags, ClickToFocus) || get_flags(Scr.Feel.flags, SloppyFocus))
        {
            if (asw == Scr.Windows->focused)
                focus_prev_aswindow (asw);
        }

LOCAL_DEBUG_OUT( "unmaping client window 0x%lX", (unsigned long)asw->w );
        if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
        {
            if ( !XGetWindowAttributes (dpy, asw->w, &attr) )
                ASWIN_SET_FLAGS(asw, AS_Dead);
            else
            {
                if( attr.map_state != IsUnmapped )
                    XUnmapWindow (dpy, asw->w);
                else
                {
                    ASWIN_CLEAR_FLAGS( asw, AS_Mapped );
                    complete_wm_state_transition( asw, IconicState );
                }
            }
        }
        /* finally mapping the icon windows : */
        add_iconbox_icon( asw );
        restack_window( asw, None, Below );
        map_canvas_window(asw->icon_canvas, True );
        if( asw->icon_canvas != asw->icon_title_canvas )
            map_canvas_window(asw->icon_title_canvas, True );
        on_window_status_changed( asw, False, True );
LOCAL_DEBUG_OUT( "updating status to iconic for client %p(\"%s\")", asw, ASWIN_NAME(asw) );
    }else
    {   /* Performing transition IconicState->NormalState  */
        if( asw->wm_state_transition == ASWT_StableState )
        {
			if( !get_flags( asw->status->flags, AS_Iconic ) )
				return False;
            asw->wm_state_transition = ASWT_Iconic2Normal ;
            clear_flags( asw->status->flags, AS_Iconic );
            remove_iconbox_icon( asw );
            unmap_canvas_window(asw->icon_canvas );
            if( asw->icon_canvas != asw->icon_title_canvas )
                unmap_canvas_window(asw->icon_title_canvas );

            ASWIN_DESK(asw) = get_flags(Scr.Feel.flags, StubbornIcons)?asw->DeIconifyDesk:Scr.CurrentDesk;
            quietly_reparent_aswindow( asw, (ASWIN_DESK(asw)==Scr.CurrentDesk)?Scr.Root:Scr.ServiceWin, True );
        }

        asw->status->icon_window = None ;

        if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
        {
            if ( !XGetWindowAttributes (dpy, asw->w, &attr) )
                ASWIN_SET_FLAGS(asw, AS_Dead);
            else
            {
                /* TODO: make sure that the window is on this screen */
                if( attr.map_state == IsUnmapped )
                    XMapRaised (dpy, asw->w);
                else
                {
                    complete_wm_state_transition( asw, NormalState );
                    ASWIN_SET_FLAGS( asw, AS_Mapped );
                    if (get_flags( Scr.Feel.flags, ClickToFocus) )
                        activate_aswindow (asw, True, False);
                }
            }
        }
    }

    on_window_status_changed( asw, True, False );
    if( !get_flags(asw->wm_state_transition, ASWT_FROM_WITHDRAWN ) )
        broadcast_config(M_CONFIGURE_WINDOW, asw );

    return True;
}

Bool
bring_aswindow_on_vscreen( ASWindow *asw )
{
    int min_x, min_y, max_x, max_y ;
    int margin = Scr.MyDisplayWidth>>5 ;

    if( asw == NULL )
        return False;

    if (!ASWIN_GET_FLAGS( asw, AS_Sticky ))
    {
        min_x = -Scr.Vx ;
        max_x = Scr.VxMax+Scr.MyDisplayWidth - Scr.Vx ;
        min_y = -Scr.Vy ;
        max_y = Scr.VyMax+Scr.MyDisplayHeight - Scr.Vy ;
    }else
    {
        min_x = 0 ;
        max_x = Scr.MyDisplayWidth ;
        min_y = 0 ;
        max_y = Scr.MyDisplayHeight ;
    }
    if ( !ASWIN_GET_FLAGS( asw, AS_Iconic ) )
    {
        int new_x = asw->status->x ;
        int new_y = asw->status->y ;
        if( asw->status->x + asw->status->width < min_x + margin )
            new_x = min_x + margin - asw->status->width ;
        else if( asw->status->x > max_x - margin )
            new_x = max_x - margin ;

        if( asw->status->y + asw->status->height < min_y + margin )
            new_y = min_y + margin - asw->status->height ;
        else if( asw->status->y > max_y - margin )
            new_y = max_y - margin ;
        if( new_x != asw->status->x || new_y != asw->status->y )
            moveresize_aswindow_wm( asw, new_x, new_y, asw->status->width, asw->status->height, False );
    }
    return True;
}


Bool
make_aswindow_visible( ASWindow *asw, Bool deiconify )
{
    if (asw == NULL)
        return False;

    if( ASWIN_GET_FLAGS( asw, AS_Iconic ) )
    {
        if( deiconify )
        {/* TODO: deiconify here */}
    }

    if (ASWIN_DESK(asw) != Scr.CurrentDesk)
        ChangeDesks( ASWIN_DESK(asw));

    bring_aswindow_on_vscreen( asw );

#ifndef NO_VIRTUAL
    if (!ASWIN_GET_FLAGS( asw, AS_Sticky ))
    {
        int  dx, dy;
        int  cx = Scr.MyDisplayWidth/2 ;
        int  cy = Scr.MyDisplayHeight/2;

        if (ASWIN_GET_FLAGS( asw, AS_Iconic ) )
        {
            if( asw->icon_canvas )
            {
                cx = asw->icon_canvas->root_x + asw->icon_canvas->width / 2;
                cy = asw->icon_canvas->root_y + asw->icon_canvas->height / 2;
            }
        } else if( asw->frame_canvas )
        {
            cx = asw->frame_canvas->root_x + asw->frame_canvas->width / 2;
            cy = asw->frame_canvas->root_y + asw->frame_canvas->height / 2;
        }

        /* Put center of window on the visible screen */
        if ( get_flags( Scr.Feel.flags, CenterOnCirculate))
        {
            dx = cx - Scr.MyDisplayWidth / 2 + Scr.Vx;
            dy = cy - Scr.MyDisplayHeight / 2 + Scr.Vy;
        } else
        {
            dx = (cx + Scr.Vx) / Scr.MyDisplayWidth * Scr.MyDisplayWidth;
            dy = (cy + Scr.Vy) / Scr.MyDisplayHeight * Scr.MyDisplayHeight;
        }
        MoveViewport (dx, dy, True);
    }
#endif

    RaiseWindow (asw);
    if (!get_flags(Scr.Feel.flags, ClickToFocus))
    {
        int x, y ;
        /* need to to center on window */
        if( ASWIN_GET_FLAGS( asw, AS_Iconic ) )
        {
            if( asw->icon_title_canvas && asw->icon_canvas != asw->icon_title_canvas )
                on_window_moveresize( asw, asw->icon_title_canvas->w );
            if( asw->icon_canvas )
            {
                on_window_moveresize( asw, asw->icon_canvas->w );
                x = asw->icon_canvas->root_x;
                y = asw->icon_canvas->root_y;
            }else if( asw->icon_title_canvas )
            {
                x = asw->icon_title_canvas->root_x;
                y = asw->icon_title_canvas->root_y;
            }else
                return False;
        }else
        {
            on_window_moveresize( asw, asw->frame );
            x = asw->frame_canvas->root_x;
            y = asw->frame_canvas->root_y;
        }
        XWarpPointer (dpy, None, Scr.Root, 0, 0, 0, 0, x + Scr.Feel.Xzap, y + Scr.Feel.Yzap);
    }
    return True;
}

void
change_aswindow_layer( ASWindow *asw, int layer )
{
    if( AS_ASSERT(asw) )
        return;
    if( ASWIN_LAYER(asw) != layer )
    {
        remove_aswindow_from_layer( asw, ASWIN_LAYER(asw));
        ASWIN_LAYER(asw) = layer ;
        add_aswindow_to_layer( asw, layer );
        restack_window_list( ASWIN_DESK(asw), False );
        ASWIN_SET_FLAGS(asw, AS_Layer);
        set_client_state (asw->w, asw->status);
    }
}

void quietly_reparent_aswindow( ASWindow *asw, Window dst, Bool user_root_pos )
{
    if( asw )
    {
        quietly_reparent_canvas( asw->frame_canvas, dst, AS_FRAME_EVENT_MASK, user_root_pos );
        quietly_reparent_canvas( asw->icon_canvas, dst, AS_ICON_EVENT_MASK, user_root_pos );
        if( asw->icon_title_canvas != asw->icon_canvas )
            quietly_reparent_canvas( asw->icon_title_canvas, dst, AS_ICON_TITLE_EVENT_MASK, user_root_pos );
    }
}

void change_aswindow_desktop( ASWindow *asw, int new_desk )
{
    int old_desk ;
    if( AS_ASSERT(asw) )
        return ;
    if( ASWIN_DESK(asw) == new_desk || ASWIN_GET_FLAGS(asw, AS_Sticky))
        return ;

    old_desk = ASWIN_DESK(asw) ;
    ASWIN_DESK(asw) = new_desk ;
    if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
    {
        set_client_desktop( asw->w, new_desk );

        /* desktop changing : */
        if( new_desk == Scr.CurrentDesk )
        {
            quietly_reparent_aswindow( asw, Scr.Root, True );
        }else if( old_desk == Scr.CurrentDesk )
            quietly_reparent_aswindow( asw, Scr.ServiceWin, True );
        broadcast_config (M_CONFIGURE_WINDOW, asw);
    }
}

void toggle_aswindow_status( ASWindow *asw, ASFlagType flags )
{
    ASFlagType on_flags, off_flags ;
    Bool need_placement = False ;
    Bool reconfigured = False ;

    if( AS_ASSERT(asw) )
        return ;
    if( flags == 0 )
        return ;

    on_flags = (~(asw->status->flags))&flags ;
    off_flags = (asw->status->flags)&(~flags) ;
    asw->status->flags = on_flags|off_flags ;

    if( get_flags( flags, AS_Shaded ) )
        asw->shading_steps = Scr.Feel.ShadeAnimationSteps ;

    if( get_flags( flags, AS_MaximizedX) )
    {
        if( !ASWIN_GET_FLAGS( asw, AS_MaximizedX ) )
        {
            if( asw->saved_anchor.width > 0)
            {
                asw->anchor.x = asw->saved_anchor.x ;
                asw->anchor.width = asw->saved_anchor.width ;
                asw->saved_anchor.width = 0 ;  /* invalidating saved anchor */
                reconfigured = True ;
            }
        }else
            need_placement = True ;
    }
    if( get_flags( flags, AS_MaximizedY) )
    {
        if( !ASWIN_GET_FLAGS( asw, AS_MaximizedY ) )
        {
            if( asw->saved_anchor.height > 0)
            {
                asw->anchor.y = asw->saved_anchor.y ;
                asw->anchor.height = asw->saved_anchor.height ;
                asw->saved_anchor.height = 0 ; /* invalidating saved anchor */
                reconfigured = True ;
            }
        }else
            need_placement = True ;
    }

    if( need_placement )
        place_aswindow( asw );

    on_window_status_changed( asw, True, reconfigured );
}

/****************************************************************************
 *
 * Sets up the shaped window borders
 *
 ****************************************************************************/
void
SetShape (ASWindow *asw, int w)
{
#ifdef SHAPE
	if( asw )
	{
        if( ASWIN_GET_FLAGS(asw, AS_Dead) )
        {
            clear_canvas_shape( asw->frame_canvas );
            return;
        }

        if( ASWIN_GET_FLAGS( asw, AS_Iconic ) )
        {                                      /* todo: update icon's shape */

        }else
        {
            int i ;

            combine_canvas_shape (asw->frame_canvas, asw->client_canvas, True, True );
#if 0
            Window        wdumm;
			int client_x = 0, client_y = 0 ;
			unsigned int width, height, bw, unused_depth  ;
            XGetGeometry( dpy, asw->w, &wdumm, &client_x, &client_y, &width, &height, &bw, &unused_depth );

            if( ASWIN_GET_FLAGS( asw, AS_Shaped ) )
            {
				/* we must use Translate coordinates since some of the canvases may not have updated
				 * their config at the time */
    			LOCAL_DEBUG_OUT( "combining client shape at %+d%+d", client_x, client_y );
                XShapeCombineShape (dpy, asw->frame, ShapeBounding,
                                    client_x+bw,
                                    client_y+bw,
                                    asw->w, ShapeBounding, ShapeSet);
            }else
            {
                XRectangle    rect;
				/* we must use Translate coordinates since some of the canvases may not have updated
				 * their config at the time */
				rect.x = client_x ;
				rect.y = client_y ;
				LOCAL_DEBUG_OUT( "setting client shape to rectangle at %+d%+d", rect.x, rect.y );
                rect.width  = width+bw*2;
                rect.height = height+bw*2;

                XShapeCombineRectangles (dpy, asw->frame, ShapeBounding,
                                         0, 0, &rect, 1, ShapeSet, Unsorted);
            }
#endif
            for( i = 0 ; i < FRAME_SIDES ; ++i )
                if( asw->frame_sides[i] )
                    combine_canvas_shape( asw->frame_canvas, asw->frame_sides[i], False, False );
        }
    }
#if 0 /*old code : */
        int bw = asw->status->border_width ;
        if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
            XShapeCombineShape (dpy, asw->frame, ShapeBounding,
                                asw->status->x + bw,
                                asw->status->y + bw,
                                asw->w, ShapeBounding, ShapeSet);

		/* windows with titles */
		if (ASWIN_HFLAGS(asw,AS_Titlebar) && asw->tbar)
		{
			XRectangle    rect;

			rect.x = asw->tbar->win_x - bw;
			rect.y = asw->tbar->win_y - bw;
			rect.width  = asw->tbar->width  + 2*bw;
			rect.height = asw->tbar->height + 2*bw;

			XShapeCombineRectangles (dpy, asw->frame, ShapeBounding,
									 0, 0, &rect, 1, ShapeUnion, Unsorted);
		}
		/* TODO: add frame decorations shape */
		/* update icon shape */
        /*if (asw->icon_canvas != NULL)
            UpdateIconShape (asw); */
#endif /* old_code */
#endif /* SHAPE */
}

void
ClearShape (ASWindow *asw)
{
#ifdef SHAPE
    if( asw && asw->frame_canvas )
    {
	unsigned int width, height ;
        XRectangle    rect;

	get_drawable_size( asw->frame_canvas->w, &width, &height );
        rect.x = 0;
        rect.y = 0;
        rect.width  = width;
        rect.height = height;
        LOCAL_DEBUG_OUT( "setting shape to rectangle %dx%d", rect.width, rect.height );
        XShapeCombineRectangles ( dpy, asw->frame, ShapeBounding,
                                  0, 0, &rect, 1, ShapeSet, Unsorted);
    }
#endif /* SHAPE */
}

/********************************************************************
 * hides focus for the screen.
 **********************************************************************/
void
hide_focus()
{
    if (get_flags(Scr.Feel.flags, ClickToFocus) && Scr.Windows->ungrabbed != NULL)
        grab_aswindow_buttons( Scr.Windows->ungrabbed, False );

    Scr.Windows->focused = NULL;
    Scr.Windows->ungrabbed = NULL;
    XRaiseWindow(dpy, Scr.ServiceWin);
    XSetInputFocus (dpy, Scr.ServiceWin, RevertToParent, Scr.last_Timestamp);
    XSync(dpy, False );
}

/********************************************************************
 * Sets the input focus to the indicated window.
 **********************************************************************/
void
commit_circulation()
{
    ASWindow *asw = Scr.Windows->active ;
LOCAL_DEBUG_OUT( "circulation completed with active window being %p", asw );
    if( asw )
        if( vector_remove_elem( Scr.Windows->circulate_list, &asw ) == 1 )
        {
            LOCAL_DEBUG_OUT( "reinserting %p into the head of circulation list : ", asw );
            vector_insert_elem( Scr.Windows->circulate_list, &asw, 1, NULL, True );
        }
    Scr.Windows->warp_curr_index = -1 ;
}

Bool
focus_aswindow( ASWindow *asw )
{
    Bool          do_hide_focus = False ;
    Bool          do_nothing = False ;
    Window        w = None;

    if( asw )
    {
        if (!get_flags( AfterStepState, ASS_WarpingMode) )
            if( vector_remove_elem( Scr.Windows->circulate_list, &asw ) == 1 )
                vector_insert_elem( Scr.Windows->circulate_list, &asw, 1, NULL, True );

#if 0
        /* ClickToFocus focus queue manipulation */
        if ( asw != Scr.Focus )
            asw->focus_sequence = Scr.next_focus_sequence++;
#endif
        do_hide_focus = (ASWIN_DESK(asw) != Scr.CurrentDesk) ||
                        (ASWIN_GET_FLAGS( asw, AS_Iconic ) &&
                            asw->icon_canvas == NULL && asw->icon_title_canvas == NULL );

        if( !ASWIN_HFLAGS(asw, AS_AcceptsFocus) )
        {
            if( Scr.Windows->focused != NULL && ASWIN_DESK(Scr.Windows->focused) == Scr.CurrentDesk )
                do_nothing = True ;
            else
                do_hide_focus = True ;
        }
    }else
        do_hide_focus = True ;

    if (Scr.NumberOfScreens > 1 && !do_hide_focus )
	{
        Window pointer_root ;
        /* if pointer went onto another screen - we need to release focus
         * and let other screen's manager manage it from now on, untill
         * pointer comes back to our screen :*/
        ASQueryPointerRoot(&pointer_root,&w);
        if(pointer_root != Scr.Root)
        {
            do_hide_focus = True;
            do_nothing = False ;
        }
    }
    if( !do_nothing && do_hide_focus )
        hide_focus();
    if( do_nothing || do_hide_focus )
        return False;

    if (get_flags(Scr.Feel.flags, ClickToFocus) && Scr.Windows->ungrabbed != asw)
    {  /* need to grab all buttons for window that we are about to
        * unfocus */
        grab_aswindow_buttons( Scr.Windows->ungrabbed, False );
        grab_aswindow_buttons( asw, True );
        Scr.Windows->ungrabbed = asw;
    }

    if( ASWIN_GET_FLAGS(asw, AS_Iconic ) )
    { /* focus icon window or icon title of the iconic window */
        if( asw->icon_canvas && !ASWIN_GET_FLAGS(asw, AS_Dead) && validate_drawable(asw->icon_canvas->w, NULL, NULL) != None )
            w = asw->icon_canvas->w;
        else if( asw->icon_title_canvas )
            w = asw->icon_title_canvas->w;
    }else if( ASWIN_GET_FLAGS(asw, AS_Shaded ) )
    { /* focus frame window of shaded clients */
        w = asw->frame ;
    }else if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
    { /* clients with visible top window can get focus directly:  */
        w = asw->w ;
    }

    if( w == None )
        show_warning( "unable to focus window %lX for client %lX, frame %lX", w, asw->w, asw->frame );
    else if( !ASWIN_GET_FLAGS(asw, AS_Mapped) )
        show_warning( "unable to focus unmapped window %lX for client %lX, frame %lX", w, asw->w, asw->frame );
    else if( ASWIN_GET_FLAGS(asw, AS_UnMapPending) )
        show_warning( "unable to focus window %lX that is about to be unmapped for client %lX, frame %lX", w, asw->w, asw->frame );
    else
    {
        LOCAL_DEBUG_OUT( "focusing window %lX, client %lX, frame %lX", w, asw->w, asw->frame );
        XSetInputFocus (dpy, w, RevertToParent, Scr.last_Timestamp);
        if (get_flags(asw->hints->protocols, AS_DoesWmTakeFocus) && !ASWIN_GET_FLAGS(asw, AS_Dead))
            send_wm_protocol_request (asw->w, _XA_WM_TAKE_FOCUS, Scr.last_Timestamp);

        Scr.Windows->focused = asw ;
    }

    XSync(dpy, False );
    return True;
}

/*********************************************************************/
/* focus management goes here :                                      */
/*********************************************************************/
/* making window active : */
/* handing over actuall focus : */
Bool
focus_active_window()
{
    /* don't fiddle with focus if we are in housekeeping mode !!! */
LOCAL_DEBUG_OUT( "checking if we are in housekeeping mode (%ld)", get_flags(AfterStepState, ASS_HousekeepingMode) );
    if( get_flags(AfterStepState, ASS_HousekeepingMode) || Scr.Windows->active == NULL )
        return False ;

    if( Scr.Windows->focused == Scr.Windows->active )
        return True ;                          /* already has focus */

    return focus_aswindow( Scr.Windows->active );
}

Bool
activate_aswindow( ASWindow *asw, Bool force, Bool deiconify )
{
    Bool res = False ;
LOCAL_DEBUG_CALLER_OUT( "%p, %d, %d", asw, force, deiconify );
LOCAL_DEBUG_OUT("current focused is %p, active is %p", Scr.Windows->focused, Scr.Windows->active );
    if (asw == NULL)
        return False;

    if( force )
    {
        GrabEm (&Scr, Scr.Feel.cursors[SELECT]);     /* to prevent Enter Notify events to
                                                      be sent to us while shifting windows around */
        if( (res = make_aswindow_visible( asw, deiconify )) )
            Scr.Windows->active = asw ;   /* must do that prior to UngrabEm, so that window gets focused */
        UngrabEm ();
    }else
    {
        if( ASWIN_GET_FLAGS( asw, AS_Iconic ) )
        {
LOCAL_DEBUG_OUT( "Window is iconic - pending implementation%s","");
            if( deiconify )
            {/* TODO: deiconify here */}
            else
                return False;
        }
        if (ASWIN_DESK(asw) != Scr.CurrentDesk)
        {
LOCAL_DEBUG_OUT( "Window is on inactive desk - can't focus%s","");
            return False;
        }
        if( asw->status->x + asw->status->width < 0  || asw->status->x >= Scr.MyDisplayWidth ||
            asw->status->y + asw->status->height < 0 || asw->status->y >= Scr.MyDisplayHeight )
        {
LOCAL_DEBUG_OUT( "Window is out of the screen - can't focus%s","");
            return False;                      /* we are out of screen - can't focus */
        }
        Scr.Windows->active = asw ;   /* must do that prior to UngrabEm, so that window gets focused */
        focus_active_window();
    }
    return res;
}

/* second version of above : */
void
focus_next_aswindow( ASWindow *asw )
{
    ASWindow     *new_focus = NULL;

    if( get_flags(Scr.Feel.flags, ClickToFocus))
        new_focus = get_next_window (asw, NULL, 1);
    if( !activate_aswindow( new_focus, False, False) )
        hide_focus();
}

void
focus_prev_aswindow( ASWindow *asw )
{
    ASWindow     *new_focus = NULL;

    if( get_flags(Scr.Feel.flags, ClickToFocus))
        new_focus = get_next_window (asw, NULL, -1);
    if( !activate_aswindow( new_focus, False, False) )
        hide_focus();
}


void
hilite_aswindow( ASWindow *asw )
{
    if( Scr.Windows->hilited != asw )
    {
        if( Scr.Windows->hilited )
            on_window_hilite_changed (Scr.Windows->hilited, False);
        if( asw )
            on_window_hilite_changed (asw, True);
        Scr.Windows->hilited = asw ;
    }
}

void
hide_hilite()
{
    if( Scr.Windows->hilited != NULL )
    {
        on_window_hilite_changed (Scr.Windows->hilited, False);
        Scr.Windows->hilited = NULL ;
    }
}

void
press_aswindow( ASWindow *asw, int context )
{
    if( context == C_NO_CONTEXT )
    {
        if( Scr.Windows->pressed == asw )
            Scr.Windows->pressed = NULL;
    }else if( Scr.Windows->pressed != asw )
    {
        /* TODO :may need to do something to avoid recursion here */
        if( Scr.Windows->pressed != NULL )
            on_window_pressure_changed( Scr.Windows->pressed, C_NO_CONTEXT );
        Scr.Windows->pressed = asw ;
    }

    if( asw )
       on_window_pressure_changed( asw, context );
}

void
release_pressure()
{
    if( Scr.Windows->pressed != NULL )
    {
        on_window_pressure_changed (Scr.Windows->pressed, C_NO_CONTEXT);
        Scr.Windows->pressed = NULL ;
    }
}

void
warp_to_aswindow( ASWindow *asw, Bool deiconify )
{
    if( asw )
        activate_aswindow( asw, True, deiconify );
}



/*************************************************************************/
/* end of the focus management                                           */
/*************************************************************************/
/*************************************************************************/

void
init_aswindow(ASWindow * t, Bool free_resources )
{
	if (!t)
		return;
    if( free_resources && t->magic == MAGIC_ASWINDOW )
    {
        if( t->transients )
    	    destroy_asvector( &(t->transients) );
    	if( t->group_members )
        	destroy_asvector( &(t->group_members) );
        if( t->status )
        	free( t->status );
		if( t->hints )
		    destroy_hints( t->hints, False );
	}
    memset (t, 0x00, sizeof (ASWindow));
    t->magic = MAGIC_ASWINDOW ;
}

void
check_aswindow_shaped( ASWindow *asw )
{
    int           boundingShaped= False;
#ifdef SHAPE
    int           dumm, clipShaped= False;
    unsigned      udumm;
    XShapeQueryExtents (dpy, asw->w,
                        &boundingShaped, &dumm, &dumm, &udumm, &udumm,
                        &clipShaped, &dumm, &dumm, &udumm, &udumm);
#endif /* SHAPE */
    if( boundingShaped )
        ASWIN_SET_FLAGS( asw, AS_Shaped );
    else
        ASWIN_CLEAR_FLAGS( asw, AS_Shaped );
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
	ASRawHints    raw_hints ;
    ASHints      *hints  = NULL;
    ASStatusHints status;
    Bool pending_placement ;


	/* allocate space for the afterstep window */
	tmp_win = safecalloc (1, sizeof (ASWindow));
    init_aswindow( tmp_win, False );

    tmp_win->w = w;
    if (validate_drawable(w, NULL, NULL) == None)
	{
		free ((char *)tmp_win);
		return (NULL);
	}

    if( collect_hints( &Scr, w, HINT_ANY, &raw_hints ) )
    {
        if( !is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
            print_hints( NULL, NULL, &raw_hints );
        hints = merge_hints( &raw_hints, Database, &status, Scr.Look.supported_hints, HINT_ANY, NULL );
        destroy_raw_hints( &raw_hints, True );
        if( hints )
        {
			show_debug( __FILE__, __FUNCTION__, __LINE__,  "Window management hints collected and merged for window %X", w );
            if( !is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
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
    SelectDecor (tmp_win);

    tmp_win->wm_state_transition = get_flags(status.flags, AS_Iconic)?ASWT_Withdrawn2Iconic:ASWT_Withdrawn2Normal ;

    pending_placement = init_aswindow_status( tmp_win, &status );

#ifdef SHAPE
    XShapeSelectInput (dpy, tmp_win->w, ShapeNotifyMask);
#endif
    check_aswindow_shaped( tmp_win );
    /*
	 * Make sure the client window still exists.  We don't want to leave an
	 * orphan frame window if it doesn't.  Since we now have the server
	 * grabbed, the window can't disappear later without having been
	 * reparented, so we'll get a DestroyNotify for it.  We won't have
	 * gotten one for anything up to here, however.
	 */
    ASSync( False );
    XGrabServer (dpy);
    if (validate_drawable(tmp_win->w, NULL, NULL) == None)
	{
		destroy_hints(tmp_win->hints, False);
		free ((char *)tmp_win);
		XUngrabServer (dpy);
		return (NULL);
	}
    XSetWindowBorderWidth (dpy, tmp_win->w, 0);

	/* add the window into the afterstep list */
    enlist_aswindow( tmp_win );
    redecorate_window  ( tmp_win, False );
    on_window_title_changed ( tmp_win, False );

    if( pending_placement )
    {
        on_window_status_changed( tmp_win, False, True );
        if( !place_aswindow( tmp_win ) )
        {
            LOCAL_DEBUG_OUT( "window status initialization failed for %lX - destroying it", w );
            Destroy (tmp_win, False);
            return NULL;
        }
    }
    /* saving window management properties : */
    set_client_desktop( tmp_win->w, ASWIN_DESK(tmp_win) );
    set_window_wm_state( tmp_win, get_flags(status.flags, AS_Iconic) );
    RaiseWindow( tmp_win );

    /*
	 * Reparenting generates an UnmapNotify event, followed by a MapNotify.
	 * Set the map state to FALSE to prevent a transition back to
	 * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
	 * again in HandleMapNotify.
     */
	XUngrabServer (dpy);
    broadcast_config (M_ADD_WINDOW, tmp_win);
    broadcast_window_name( tmp_win );
    broadcast_res_names( tmp_win );
    broadcast_icon_name( tmp_win );

#if 0
/* TODO : */
	if (NeedToResizeToo)
	{
		XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
					  Scr.MyDisplayHeight,
					  tmp_win->frame_x + (tmp_win->frame_width >> 1),
					  tmp_win->frame_y + (tmp_win->frame_height >> 1));
		resize_window (tmp_win->w, tmp_win, 0, 0, 0, 0);
	}
#endif
    InstallWindowColormaps (tmp_win);

	return (tmp_win);
}

/* hints gets swallowed, but status does not : */
/* w must be unmapped !!!! */
ASWindow*
AddInternalWindow (Window w, ASInternalWindow **pinternal, ASHints **phints, ASStatusHints *status)
{
	ASWindow     *tmp_win;					   /* new afterstep window structure */
    ASHints      *hints = *phints ;

	/* allocate space for the afterstep window */
	tmp_win = safecalloc (1, sizeof (ASWindow));
    init_aswindow( tmp_win, False );

    tmp_win->w = w;
    if (validate_drawable(w, NULL, NULL) == None)
	{
		free ((char *)tmp_win);
		return (NULL);
	}

    tmp_win->internal = *pinternal ;
    *pinternal = NULL ;

    if( hints )
    {
        show_debug( __FILE__, __FUNCTION__, __LINE__,  "Window management hints supplied for window %X", w );
        if( !is_output_level_under_threshold(OUTPUT_LEVEL_HINTS) )
        {
            print_clean_hints( NULL, NULL, hints );
            print_status_hints( NULL, NULL, status );
        }
    }
    tmp_win->hints = hints ;
    *phints = NULL ;

    SelectDecor (tmp_win);

    tmp_win->wm_state_transition = get_flags(status->flags, AS_Iconic)?ASWT_Withdrawn2Iconic:ASWT_Withdrawn2Normal ;

    init_aswindow_status( tmp_win, status );

#ifdef SHAPE
    XShapeSelectInput (dpy, tmp_win->w, ShapeNotifyMask);
#endif

    XGrabServer (dpy);
    XSetWindowBorderWidth (dpy, tmp_win->w, 0);

	/* add the window into the afterstep list */
    enlist_aswindow( tmp_win );

    redecorate_window       ( tmp_win, False );
    on_window_title_changed ( tmp_win, False );
    set_window_wm_state( tmp_win, get_flags(status->flags, AS_Iconic) );
    RaiseWindow( tmp_win );
    /*
	 * Reparenting generates an UnmapNotify event, followed by a MapNotify.
	 * Set the map state to FALSE to prevent a transition back to
	 * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
	 * again in HandleMapNotify.
     */
    XUngrabServer (dpy);
    broadcast_config (M_ADD_WINDOW, tmp_win);
    broadcast_window_name( tmp_win );
    broadcast_res_names( tmp_win );
    broadcast_icon_name( tmp_win );

#if 0
/* TODO : */
	if (NeedToResizeToo)
	{
		XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
					  Scr.MyDisplayHeight,
					  tmp_win->frame_x + (tmp_win->frame_width >> 1),
					  tmp_win->frame_y + (tmp_win->frame_height >> 1));
		resize_window (tmp_win->w, tmp_win, 0, 0, 0, 0);
	}
#endif

	return (tmp_win);
}

/***************************************************************************
 * Handles destruction of a window
 ****************************************************************************/
void
Destroy (ASWindow *asw, Bool kill_client)
{
    static int nested_level = 0 ;
    Bool bad_window ;
    /*
	 * Warning, this is also called by HandleUnmapNotify; if it ever needs to
	 * look at the event, HandleUnmapNotify will have to mash the UnmapNotify
	 * into a DestroyNotify.
	 */
    if (AS_ASSERT(asw))
		return;
LOCAL_DEBUG_CALLER_OUT( "asw(%p)->internal(%p)->data(%p)", asw, asw->internal, asw->internal?asw->internal->data:NULL );

    /* we could be recursively called from delist_aswindow call - trying to prevent that : */
    if( nested_level > 0 )
        return;
    ++nested_level ;

    SendPacket (-1, M_DESTROY_WINDOW, 3, asw->w, asw->frame, (unsigned long)asw);

    XGrabServer( dpy );
    bad_window = (validate_drawable( asw->w, NULL, NULL ) == None );

    XUnmapWindow (dpy, asw->frame);
    if( !bad_window )
    {
        if( asw->internal == NULL )
            XRemoveFromSaveSet (dpy, asw->w);
        XSelectInput (dpy, asw->w, NoEventMask);
    }else
        ASWIN_SET_FLAGS( asw, AS_Dead );
    XSync (dpy, 0);

    UninstallWindowColormaps( asw );
    CheckWarpingFocusDestroyed(asw);
    CheckGrabbedFocusDestroyed(asw);

    if ( asw == Scr.Windows->focused )
        focus_prev_aswindow( asw );

    if ( asw == Scr.Windows->active )
        Scr.Windows->active = NULL;
    if ( asw == Scr.Windows->hilited )
        Scr.Windows->hilited = NULL;
    if ( asw == Scr.Windows->pressed )
        Scr.Windows->pressed = NULL;

    if (!kill_client && asw->internal == NULL )
        RestoreWithdrawnLocation (asw, True);

    if( asw->internal )
    {
        if( asw->internal->destroy )
            asw->internal->destroy( asw->internal );
        if( asw->internal->data ) /* in case destroy() above did not work properly */
            free( asw->internal->data );
        free( asw->internal );
        asw->internal = NULL ;
    }

    redecorate_window( asw, True );
    unregister_aswindow( asw->w );
    delist_aswindow( asw );

    init_aswindow( asw, True );

    XSync (dpy, 0);
    XUngrabServer( dpy );

    memset( asw, 0x00, sizeof(ASWindow));
    free (asw);
    --nested_level ;

	return;
}

/***********************************************************************
 *  Procedure:
 *	RestoreWithdrawnLocation
 *  Puts windows back where they were before afterstep took over
 ************************************************************************/
void
RestoreWithdrawnLocation (ASWindow * asw, Bool restart)
{
    int x = 0, y = 0;
    unsigned int width = 0, height = 0, bw = 0 ;
    Bool map_too = False ;

LOCAL_DEBUG_CALLER_OUT("%p, %d", asw, restart );

    if (AS_ASSERT(asw))
		return;

    if( asw->status )
    {
        if( asw->status->width > 0 )
            width = asw->status->width ;
        if( asw->status->height > 0 )
            width = asw->status->height ;
        if( get_flags( asw->status->flags, AS_StartBorderWidth) )
            bw = asw->status->border_width ;
        /* We'll get withdrawn
            * location in virtual coordinates, so that when window is mapped again
            * we'll get it in correct position on the virtual screen.
            * Besides when we map window we check its postion so it would not be
            * completely off the screen ( if PPosition only, since User can place
            * window anywhere he wants ).
            *                             ( Sasha )
            */

        make_detach_pos( asw->hints, asw->status, &(asw->anchor), &x, &y );

        if ( get_flags(asw->status->flags, AS_Iconic ))
        {
            /* if we don't do that while exiting AfterStep -
               we will loose the client while starting AS again, as window will be
               unmapped, AS will be waiting for it to be mapped by client, and
               client will not be aware that it should do so, as WM exits and
               startups are transparent for it */
            map_too = True ;
        }
    }
    /*
     * Prevent the receipt of an UnmapNotify, since that would
     * cause a transition to the Withdrawn state.
     */
    if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
    {
        XSelectInput (dpy, asw->w, NoEventMask);
        if( get_parent_window( asw->w ) == asw->frame )
        {
            ASStatusHints withdrawn_state ;
            /* !!! Most of it has been done in datach_basic_widget : */
            XReparentWindow (dpy, asw->w, Scr.Root, x, y);
            withdrawn_state.flags = AS_Withdrawn ;
            withdrawn_state.icon_window = None ;
            set_client_state( asw->w, &withdrawn_state );

            if( width > 0 && height > 0 )
                XResizeWindow( dpy, asw->w, width, height );
            XSetWindowBorderWidth (dpy, asw->w, bw);

            if( map_too )
                XMapWindow (dpy, asw->w);
            XSync( dpy, False );
        }
        /* signaling client that we no longer handle it : */
        set_multi32bit_property (asw->w, _XA_WM_STATE, _XA_WM_STATE, 2, WithdrawnState, None);
    }
}

/**********************************************************************/
/* The End                                                            */
/**********************************************************************/

