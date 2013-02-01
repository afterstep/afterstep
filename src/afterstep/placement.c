/*
 * Copyright (C) 2005 Fabian Yamaguchi
 * Copyright (C) 2004-2005 Sasha Vasko
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

#define LOCAL_DEBUG
#include "../../configure.h"

#include "asinternals.h"
#include "../../libAfterStep/moveresize.h"


struct ASWindowGridAuxData{
	ASGrid *grid;
	long    desk;
	int     min_layer;
	Bool    frame_only ;
	int vx, vy;
	Bool ignore_avoid_cover ;
	ASWindow *target;
};

typedef struct ASFreeRectangleAuxData{
	ASVector *list;
	long    desk;
	int     min_layer; /* applies only to non-AvoidCover windows */
	int		max_layer; /* applies to all windows - including Avoid Cover */
	Bool    frame_only ;
	ASGeometry area ;
	ASWindow *to_skip ;
}ASFreeRectangleAuxData;

typedef struct ASAvoidCoverAuxData
{
	ASWindow    *new_aswin;
	ASWindowBox *aswbox ;
	ASGeometry  *area ;
}ASAvoidCoverAuxData;


/*************************************************************************/
/* here we build vector of rectangles, representing one available
 * space each :
 */
/*************************************************************************/

Bool
get_free_rectangles_iter_func(void *data, void *aux_data)
{
	ASFreeRectangleAuxData *fr_data  = (ASFreeRectangleAuxData*)aux_data ;
	ASWindow *asw = (ASWindow*)data ;

	if( asw 
		&& (!IsValidDesk(fr_data->desk) || ASWIN_DESK(asw) == fr_data->desk) 
		&& asw != fr_data->to_skip
		&& !ASWIN_GET_FLAGS(asw, AS_Fullscreen|AS_ShortLived ) 
		&& fr_data->max_layer >= ASWIN_LAYER(asw)
		&& (ASWIN_HFLAGS(asw, AS_AvoidCover) || fr_data->min_layer <= ASWIN_LAYER(asw)))
	{
		int min_vx = fr_data->area.x, max_vx = fr_data->area.x+fr_data->area.width ;
		int min_vy = fr_data->area.y, max_vy = fr_data->area.y+fr_data->area.height ;
		int x, y;
		unsigned int width, height, bw;

		if( ASWIN_GET_FLAGS(asw, AS_Iconic ) )
		{
			if( asw->icon_canvas != asw->icon_title_canvas && asw->icon_title_canvas != NULL )
			{
				get_current_canvas_geometry( asw->icon_title_canvas, &x, &y, &width, &height, &bw );
				x += Scr.Vx ;
				y += Scr.Vy ;
				if( x+width+bw >= min_vx && x-bw < max_vx &&  y+height+bw >= min_vy && y-bw < max_vy )
					subtract_rectangle_from_list( fr_data->list, x-bw, y-bw, x+width+bw, y+height+bw );
			}
			get_current_canvas_geometry( asw->icon_canvas, &x, &y, &width, &height,&bw );
		}else
			get_current_canvas_geometry( asw->frame_canvas, &x, &y, &width, &height,&bw );
		x += Scr.Vx ;
		y += Scr.Vy ;
		LOCAL_DEBUG_OUT( "frame_geom = %dx%d%+d%+d, h_limits = %d,%d; v_limits = %d,%d", 
						  width, height, x-bw, y-bw, min_vx, max_vx, min_vy, max_vy );
		if( x+(int)width+(int)bw >= min_vx && x-(int)bw < max_vx &&  y+(int)height+(int)bw >= min_vy && y-(int)bw < max_vy )
			subtract_rectangle_from_list( fr_data->list, x-(int)bw, y-(int)bw, x+(int)width+(int)bw, y+(int)height+(int)bw );
	}

	return True;
}

static ASVector *
build_free_space_list( ASWindow *to_skip, ASGeometry *area, int min_layer, int max_layer )
{
	ASVector *list = create_asvector( sizeof(XRectangle) );
	ASFreeRectangleAuxData aux_data ;
	XRectangle seed_rect;

	aux_data.to_skip = to_skip ;
	aux_data.list = list ;
	aux_data.desk = Scr.CurrentDesk;
	aux_data.min_layer = min_layer;
	aux_data.max_layer = max_layer;
	aux_data.area = *area;

	/* must seed the list with the single rectangle representing the area : */
	seed_rect.x = area->x ;
	seed_rect.y = area->y ;
	seed_rect.width = area->width ;
	seed_rect.height = area->height ;

	append_vector( list, &seed_rect, 1 );

	iterate_asbidirlist( Scr.Windows->clients, get_free_rectangles_iter_func, (void*)&aux_data, NULL, False );

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
	print_rectangles_list( list );
#endif

	return list;
}

/*************************************************************************
 * here we build the grid  to facilitate avoid cover and snap-to-grid
 * while moving resizing window
 *************************************************************************/
Bool
get_aswindow_grid_iter_func(void *data, void *aux_data)
{
	ASWindow *asw = (ASWindow*)data ;
	struct ASWindowGridAuxData *grid_data = (struct ASWindowGridAuxData*)aux_data;

	if( asw && (!IsValidDesk(grid_data->desk) || ASWIN_DESK(asw) == grid_data->desk) )
	{
		int outer_gravity = Scr.Feel.EdgeAttractionWindow ;
		int inner_gravity = Scr.Feel.EdgeAttractionWindow ;
		if( ASWIN_HFLAGS(asw, AS_AvoidCover) && !grid_data->ignore_avoid_cover &&
			asw != grid_data->target )
			inner_gravity = -1 ;
		else if( inner_gravity == 0 || grid_data->min_layer > ASWIN_LAYER(asw))
			return True;

		if( ASWIN_GET_FLAGS(asw, AS_Iconic ) )
		{
			add_canvas_grid( grid_data->grid, asw->icon_canvas, outer_gravity, inner_gravity, get_flags(Scr.Feel.flags,StickyIcons));
			if( asw->icon_canvas != asw->icon_title_canvas )
				add_canvas_grid( grid_data->grid, asw->icon_title_canvas, outer_gravity, inner_gravity, get_flags(Scr.Feel.flags,StickyIcons));
		}else
		{
			add_canvas_grid( grid_data->grid, asw->frame_canvas, outer_gravity, inner_gravity, ASWIN_GET_FLAGS(asw, AS_Sticky));
			if( !grid_data->frame_only )
				add_canvas_grid( grid_data->grid, asw->client_canvas, outer_gravity/2, (inner_gravity*2)/3, ASWIN_GET_FLAGS(asw, AS_Sticky));
		}
	}
	return True;
}

ASGrid*
make_desktop_grid(int desk, int min_layer, Bool frame_only, ASWindow *target )
{
	struct ASWindowGridAuxData grid_data ;
	int resist = Scr.Feel.EdgeResistanceMove ;
	int attract = Scr.Feel.EdgeAttractionScreen ;
	int i ;
	ASVector *free_space_list = NULL;
	XRectangle *rects = NULL;
	int w = target->status->width ;
	int h = target->status->height ;
	ASGeometry area ;

	grid_data.desk = desk ;
	grid_data.min_layer = min_layer ;
	grid_data.frame_only = frame_only ;
	grid_data.grid = safecalloc( 1, sizeof(ASGrid));
	grid_data.grid->curr_vx = Scr.Vx;
	grid_data.grid->curr_vy = Scr.Vy;
	grid_data.ignore_avoid_cover = True ;
	grid_data.target = target ;
#if 0
	area.x = vx ;
	area.y = vy ;
	area.width = Scr.MyDisplayWidth ;
	area.height = Scr.MyDisplayHeight ;
#else
	area.x = 0 ;
	area.y = 0 ;
	area.width = Scr.VxMax + Scr.MyDisplayWidth ;
	area.height = Scr.VyMax + Scr.MyDisplayHeight ;
#endif
	/* even though we are not limited to free space - it is best to avoid windows with AvoidCover
	 * bit set */
	free_space_list =  build_free_space_list( target, &area, AS_LayerHighest, AS_LayerHighest );
	rects = PVECTOR_HEAD(XRectangle,free_space_list);

	i = PVECTOR_USED(free_space_list);
	/* now we need to find the biggest rectangle : */
	while( --i >= 0 )
		if( rects[i].width >= w && rects[i].height >= h )
		{
			grid_data.ignore_avoid_cover = False ;
			break;
		}
	destroy_asvector( &free_space_list );

//    add_canvas_grid( grid_data.grid, Scr.RootCanvas, resist, attract, vx, vy );

	add_gridline( grid_data.grid, 0,                   0, Scr.MyDisplayWidth,  resist, attract, ASGL_Absolute );
	add_gridline( grid_data.grid, Scr.MyDisplayHeight, 0, Scr.MyDisplayWidth,  attract, resist, ASGL_Absolute );
	add_gridline( grid_data.grid, 0,                   0, Scr.MyDisplayHeight, resist, attract, ASGL_Absolute|ASGL_Vertical );
	add_gridline( grid_data.grid, Scr.MyDisplayWidth , 0, Scr.MyDisplayHeight, attract, resist, ASGL_Absolute|ASGL_Vertical );

	/* add all the window edges for this desktop : */
	iterate_asbidirlist( Scr.Windows->clients, get_aswindow_grid_iter_func, (void*)&grid_data, NULL, False );

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
	print_asgrid( grid_data.grid );
#endif

	return grid_data.grid;
}

void setup_aswindow_moveresize(ASWindow *asw,  struct ASMoveResizeData *mvrdata )
{
	if( asw->frame_data && asw->tbar )
	{
		if( asw->frame_data->condense_titlebar != NO_ALIGN /* && mvrdata->move_only  */)  
		{
			if( ASWIN_HFLAGS(asw, AS_VerticalTitle ) )
				mvrdata->title_west = asw->tbar->width ;
			else
				mvrdata->title_north = asw->tbar->height ;
		}
	}
	raise_scren_panframes( ASDefaultScr );
	mvrdata->below_sibling = get_lowest_panframe(ASDefaultScr);
	set_moveresize_restrains( mvrdata, asw->hints, asw->status);
	mvrdata->grid = make_desktop_grid(Scr.CurrentDesk, AS_LayerDesktop, False, asw);
	Scr.moveresize_in_progress = mvrdata ;
}

void apply_aswindow_moveresize(struct ASMoveResizeData *data)
{
	ASWindow *asw = window2ASWindow( AS_WIDGET_WINDOW(data->mr));
LOCAL_DEBUG_OUT( "%dx%d%+d%+d", data->curr.width, data->curr.height, data->curr.x, data->curr.y);
	if( asw && !ASWIN_GET_FLAGS(asw,AS_Dead) )
	{
		int new_width = data->curr.width ;
		int new_height = data->curr.height ;
		Bool server_grabbed = False ; 
		if( ASWIN_GET_FLAGS( asw, AS_Shaded ) )
		{
			new_width = asw->status->width ;
			new_height = asw->status->height ;
#if 0
			/* lets only move us as we are in shaded state : */
			move_canvas(  asw->frame_canvas, data->curr.x, data->curr.y );
		}else
		{
			if( data->curr.width != data->last.width || 
				data->curr.height != data->last.height )
			{
				int client_width = data->curr.width - (asw->frame_canvas->width - asw->client_canvas->width) ; 
				int client_height = data->curr.height - (asw->frame_canvas->height - asw->client_canvas->height) ; 
				XGrabServer(dpy);
				server_grabbed = True ;
				resize_canvas( asw->client_canvas, client_width, client_height);
			}
			moveresize_canvas(  asw->frame_canvas, data->curr.x, data->curr.y, data->curr.width, data->curr.height );
			ASSync(False);
#endif		
		}
		if( ASWIN_GET_FLAGS( asw, AS_ShapedDecor|AS_Shaped ) && 
			(data->curr.width > data->last.width || data->curr.height > data->last.height) )
		{/* this greately reduces flickering on resizing of shaped windows : */
			XRectangle    rect;
			rect.x = 0;
			rect.y = 0;
			moveresize_canvas(  asw->frame_canvas, data->curr.x, data->curr.y, data->curr.width, data->curr.height );
			if( data->curr.width > data->last.width ) 
			{
				if( get_flags(asw->frame_data->condense_titlebar, ALIGN_LEFT) && asw->tbar )
					rect.y = asw->tbar->height ; 
				rect.width  = data->curr.width - data->last.width;
				rect.height = (int)data->last.height - rect.y;
				XShapeCombineRectangles ( dpy, asw->frame, ShapeBounding,
										  data->last.width, 0, &rect, 1, ShapeUnion, Unsorted);
			}
			if( data->curr.height > data->last.height ) 
			{
				if( get_flags(asw->frame_data->condense_titlebar, ALIGN_RIGHT) && asw->tbar )
					rect.x = asw->tbar->width ; 
				rect.width  = (int)data->curr.width - rect.x;
				rect.height = data->curr.height - data->last.height;
				XShapeCombineRectangles ( dpy, asw->frame, ShapeBounding,
										  0, data->last.height, &rect, 1, ShapeUnion, Unsorted);
			}
		}
		moveresize_aswindow_wm( asw, data->curr.x, data->curr.y, new_width, new_height, False);
		if( server_grabbed ) 
			XUngrabServer(dpy);
	}
}

void apply_aswindow_move(struct ASMoveResizeData *data)
{
	ASWindow *asw = window2ASWindow( AS_WIDGET_WINDOW(data->mr));
LOCAL_DEBUG_OUT( "%dx%d%+d%+d", asw->status->width, asw->status->height, data->curr.x, data->curr.y);
	if( asw && !ASWIN_GET_FLAGS(asw,AS_Dead) )
	{
		/* lets only move us as we maybe in shaded state : */
		move_canvas(  asw->frame_canvas, data->curr.x, data->curr.y );
		ASSync(False);
		moveresize_aswindow_wm( asw, data->curr.x, data->curr.y, asw->status->width, asw->status->height, False);
	}
}

void complete_aswindow_moveresize(struct ASMoveResizeData *data, Bool cancelled)
{
	ASWindow *asw = window2ASWindow( AS_WIDGET_WINDOW(data->mr));
	if( asw && !ASWIN_GET_FLAGS(asw,AS_Dead))
	{
		if( cancelled )
		{
			SHOW_CHECKPOINT;
			LOCAL_DEBUG_OUT( "%dx%d%+d%+d", data->start.width, data->start.height, data->start.x, data->start.y);
			moveresize_aswindow_wm( asw, data->start.x, data->start.y, data->start.width, data->start.height, False );
		}else
		{
			SHOW_CHECKPOINT;
			LOCAL_DEBUG_OUT( "%dx%d%+d%+d", data->curr.width, data->curr.height, data->curr.x, data->curr.y);
			moveresize_aswindow_wm( asw, data->curr.x, data->curr.y, data->curr.width, data->curr.height, False );
		}
		ASWIN_CLEAR_FLAGS( asw, AS_MoveresizeInProgress );
		asw->frame_canvas->root_x = -10000 ; 
		asw->frame_canvas->root_y = -10000;
		asw->frame_canvas->width = 1 ; 
		asw->frame_canvas->height = 1;
		
		on_window_moveresize( asw, asw->frame );
		broadcast_config (M_CONFIGURE_WINDOW, asw);
	}
	Scr.moveresize_in_progress = NULL ;
}

void complete_aswindow_move(struct ASMoveResizeData *data, Bool cancelled)
{
	ASWindow *asw = window2ASWindow( AS_WIDGET_WINDOW(data->mr));
	if( asw && !ASWIN_GET_FLAGS(asw,AS_Dead))
	{
		if( cancelled )
		{
			SHOW_CHECKPOINT;
			LOCAL_DEBUG_OUT( "%dx%d%+d%+d", data->start.width, data->start.height, data->start.x, data->start.y);
			moveresize_aswindow_wm( asw, data->start.x, data->start.y, data->start.width, data->start.height, False );
		}else
		{
			SHOW_CHECKPOINT;
			LOCAL_DEBUG_OUT( "%dx%d%+d%+d", data->start.width, data->start.height, data->curr.x, data->curr.y);
			moveresize_aswindow_wm( asw, data->curr.x, data->curr.y, data->start.width, data->start.height, False );
		}
		
		ASWIN_CLEAR_FLAGS( asw, AS_MoveresizeInProgress );
		asw->frame_canvas->root_x = -10000 ; 
		asw->frame_canvas->root_y = -10000;
		on_window_moveresize( asw, asw->frame );
		broadcast_config (M_CONFIGURE_WINDOW, asw);
	}
	Scr.moveresize_in_progress = NULL ;
}

/*************************************************************************/
/* placement routines : */
/*************************************************************************/
static void
apply_placement_result( ASStatusHints *status, XRectangle *anchor, ASHints *hints, ASFlagType flags, int vx, int vy, unsigned int width, unsigned int height )
{
#define apply_placement_result_asw(asw,flags,vx,vy,width,height)  apply_placement_result((asw)->status, &((asw)->anchor), (asw)->hints, flags, vx, vy, width, height )    
	
	if( get_flags( flags, XValue ) )
	{
		status->x = vx ;
		if( !get_flags (status->flags, AS_Sticky) )
			status->x -= status->viewport_x ;
		else
			status->x -= Scr.Vx ;
	}
	if( get_flags( flags, YValue ) )
	{
		status->y = vy ;
		if( !get_flags (status->flags, AS_Sticky) )
			status->y -= status->viewport_y ;
		else
			status->y -= Scr.Vy ;
	}
	if( get_flags( flags, WidthValue ) && width > 0 )
		status->width = width ;

	if( get_flags( flags, HeightValue ) && height > 0 )
		status->height = height ;

	status2anchor( anchor, hints, status, Scr.VxMax+Scr.MyDisplayWidth, Scr.VyMax+Scr.MyDisplayHeight);
}

static int
move_placement_left( ASVector *free_space_list, int x, int y, int w, int h ) 
{
	XRectangle *rects = PVECTOR_HEAD(XRectangle,free_space_list);
	int i = PVECTOR_USED(free_space_list);
	while( --i >= 0 )
		if( y >= rects[i].y && y+h <= rects[i].y+(int)rects[i].height && rects[i].x < x && rects[i].x+(int)rects[i].width >= x )
			x = rects[i].x ;
	return x ;
}

static int
move_placement_right( ASVector *free_space_list, int x, int y, int w, int h ) 
{
	XRectangle *rects = PVECTOR_HEAD(XRectangle,free_space_list);
	int i = PVECTOR_USED(free_space_list);
	while( --i >= 0 )
		if( y >= rects[i].y && y+h <= rects[i].y+(int)rects[i].height && rects[i].x <= x+w && rects[i].x+(int)rects[i].width > x+w )
			x = rects[i].x + (int)rects[i].width  - w ;
	return x ;
}

static int
move_placement_up( ASVector *free_space_list, int x, int y, int w, int h ) 
{
	XRectangle *rects = PVECTOR_HEAD(XRectangle,free_space_list);
	int i = PVECTOR_USED(free_space_list);
	while( --i >= 0 )
		if( x >= rects[i].x && x+w <= rects[i].x+(int)rects[i].width  && rects[i].y < y && rects[i].y+(int)rects[i].height >= y )
			y = rects[i].y ;
	return y ;
}

static int
move_placement_down( ASVector *free_space_list, int x, int y, int w, int h ) 
{
	XRectangle *rects = PVECTOR_HEAD(XRectangle,free_space_list);
	int i = PVECTOR_USED(free_space_list);
	while( --i >= 0 )
		if( x >= rects[i].x && x+w <= rects[i].x+(int)rects[i].width  && rects[i].y <= y+h && rects[i].y+(int)rects[i].height > y+h )
			y = rects[i].y + (int)rects[i].height  - h ;
	return y ;
}

static Bool do_smart_placement( ASWindow *asw, ASWindowBox *aswbox, ASGeometry *area )
{
	ASVector *free_space_list =  build_free_space_list( asw, area, ASWIN_LAYER(asw), AS_LayerHighest );
	XRectangle *rects = PVECTOR_HEAD(XRectangle,free_space_list);
	int i, selected = -1 ;
	unsigned short w = asw->status->width;
	unsigned short h = asw->status->height;
	unsigned short dw = w>100?w*5/100:5, dh = h>=100?h*5/100:5 ;
	int spacer_x = -1 ;
	int spacer_y = -1 ;

	LOCAL_DEBUG_OUT( "size=%dx%d, delta=%dx%d", w, h, dw, dh );
	/* now we have to find the optimal rectangle from the list */
	/* pass 1: find rectangle that fits both width and height with margin +- 5% of the window size */
	i = PVECTOR_USED(free_space_list);
	while( --i >= 0 )
		if( rects[i].width >= w && rects[i].height >=  h &&
			rects[i].width - w < dw && rects[i].height - h < dh )
		{
			if( selected >= 0 )
				if( rects[i].width*rects[i].height >= rects[selected].width*rects[selected].height )
					continue;
			selected = i;
		}
	LOCAL_DEBUG_OUT( "pass1: %d", selected );
	
	if( selected < 0 )
	{	
		i = PVECTOR_USED(free_space_list);	 
		if( w > 200 && h > 200 )
		{ /* simply find the biggest rectangle that fits  */
			while( --i >= 0 )	
				if( rects[i].width >= w && rects[i].height >= h  )
				{
					if( selected >= 0 )
						if( rects[i].width*rects[i].height <= rects[selected].width*rects[selected].height )			
							continue;
					selected = i ;
				}	 
			LOCAL_DEBUG_OUT( "pass2a: %d", selected );
		}else if( w > h ) 
		{	/* try and fit it by the horizontal edge of the screen */
			while( --i >= 0 )	  
				if( rects[i].width >= w && rects[i].height >= h && 
					(rects[i].y < 100 || rects[i].y + rects[i].height > area->y+area->height - 100)  )
				{
					 selected = i ;	
					 if( rects[i].y >= 100 ) 
						spacer_y = rects[i].height - h ;
					 break;
				}
			LOCAL_DEBUG_OUT( "pass2b: %d", selected );
		}else		 
		{	
			/* try and fit it by the vertical edge of the screen */
			while( --i >= 0 )	  
				if( rects[i].width >= w && rects[i].height >= h && 
					(rects[i].x < 100 || rects[i].x + rects[i].width> area->x+area->width - 100)  )
				{
					 selected = i ;	
					 if( rects[i].x >= 100 ) 
						spacer_x = rects[i].width - w ;
					 break;
				}
			LOCAL_DEBUG_OUT( "pass2c: %d", selected );
		}	
	}
	/* if width < height then swap passes 2 and 3 */
	/* pass 2: find rectangle that fits width within margin +- 5% of the window size  and has maximum height
	 * left after placement */
	if( selected < 0 )
	{
		i = PVECTOR_USED(free_space_list);
		if( w >= h )
		{
			while( --i >= 0 )
				if( rects[i].width >= w && rects[i].height >=  h && rects[i].width - w < dw )
				{
					if( selected >= 0 )
						if( rects[i].height < rects[selected].height )
							continue;
					selected = i;
				}
		}else
		{
			while( --i >= 0 )
				if( rects[i].width >= w && rects[i].height >=  h && rects[i].height - h < dh )
				{
					if( selected >= 0 )
						if( rects[i].width < rects[selected].width )
							continue;
					selected = i;
				}
		}
	}
	LOCAL_DEBUG_OUT( "pass2: %d", selected );
	/* pass 3: find rectangle that fits height within margin +- 5% of the window size  and has maximum width
	 * left after placement */
	if( selected < 0 )
	{
		i = PVECTOR_USED(free_space_list);
		if( w >= h )
		{
			while( --i >= 0 )
				if( rects[i].width >= w && rects[i].height >=  h && rects[i].height - h < dh )
				{
					if( selected >= 0 )
						if( rects[i].width < rects[selected].width )
							continue;
					selected = i;
				}
		}else
		{
			while( --i >= 0 )
				if( rects[i].width >= w && rects[i].height >=  h && rects[i].width - w < dw )
				{
					if( selected >= 0 )
						if( rects[i].height < rects[selected].height )
							continue;
					selected = i;
				}
		}
	}
	LOCAL_DEBUG_OUT( "pass3: %d", selected );
	/* pass 4: if width >= height then find rectangle with smallest width difference and largest height difference */
	if( selected < 0 && w >= h )
	{
		i = PVECTOR_USED(free_space_list);
		while( --i >= 0 )
			if( rects[i].width >= w && rects[i].height >=  h )
			{
				selected = i ;
				break;
			}
		while( --i >= 0 )
			if( rects[i].width >= w && rects[i].height >=  h )
			{
				int dw = (rects[i].width > w)?rects[i].width - w:1;
				int dw_sel = (rects[selected].width > w)?rects[selected].width - w:1;
				if( ((rects[i].height - h)*Scr.MyDisplayWidth)/dw > 
					((rects[selected].height - h)*Scr.MyDisplayWidth)/dw_sel  )
					selected = i;
			}
	}
	LOCAL_DEBUG_OUT( "pass4: %d", selected );
	/* pass 5: if width < height then find rectangle with biggest width difference and smallest height difference */
	if( selected < 0 )
	{
		i = PVECTOR_USED(free_space_list);
		while( --i >= 0 )
			if( rects[i].width >= w && rects[i].height >=  h )
			{
				selected = i ;
				break;
			}
		while( --i >= 0 )
			if( rects[i].width >= w && rects[i].height >=  h )
			{
				int dh = (rects[i].height > h)?rects[i].height - h:1;
				int dh_sel = (rects[selected].height > h)?rects[selected].height - h:1;
				if( ((rects[i].width - w)*Scr.MyDisplayHeight)/dh > 
					((rects[selected].width - w)*Scr.MyDisplayHeight)/dh_sel  )
					selected = i;
			}
	}
	LOCAL_DEBUG_OUT( "pass5: %d", selected );

	if( selected >= 0 )
	{
		int target_x, target_y ;
		int dx, dy ;
		int move_left, move_up; 
		Bool changed ; 
		if( spacer_x < 0 )
		{	
			spacer_x = 0 ;
			if( rects[selected].width > w )
			{
				int to_right = (area->x+(int)area->width) - (rects[selected].x + (int)rects[selected].width) ; 
				if( to_right < rects[selected].x - area->x ) 
					spacer_x = (int)rects[selected].width - (int)w ; 
			}	 
		}
		if( spacer_y < 0 ) 
		{	
			spacer_y = 0 ;
			if( rects[selected].height > h)
			{	
				int to_bottom = (area->y+(int)area->height) - (rects[selected].y + (int)rects[selected].height) ; 
				if( to_bottom < rects[selected].y - area->y ) 
					spacer_y = (int)rects[selected].height - (int)h ; 
			}
		}
		target_x = rects[selected].x+spacer_x ;
		target_y = rects[selected].y+spacer_y ;
		
		do 
		{	
			int new_x = target_x, new_y = target_y ;
			changed = False ; 
			dx = target_x - area->x ; 
			dy = target_y - area->y ; 
		
			move_left = ( dx < area->width - (dx+w) ) ;
			if( !move_left )
				dx = area->width - (dx+w);

			move_up = ( dy < area->height - (dy+h) ) ;
			if( !move_up )
				dy = area->height - (dy+h);
			/* we want to place window as close as possible to the edge of the area */
			if( dx > dy ) 
			{
				if( move_left ) 
					new_x = move_placement_left( free_space_list, target_x, target_y, w, h );
				else
					new_x = move_placement_right( free_space_list, target_x, target_y, w, h );
				if( dy > 0 ) 
				{	
					if( move_up ) 
						new_y = move_placement_up( free_space_list, target_x, target_y, w, h );
					else
						new_y = move_placement_down( free_space_list, target_x, target_y, w, h );		
				}
			
			}else if( dy > 0 ) 
			{
				if( move_up ) 
					new_y = move_placement_up( free_space_list, target_x, target_y, w, h );
				else
					new_y = move_placement_down( free_space_list, target_x, target_y, w, h );		
				if( dx > 0 ) 
				{	
					if( move_left ) 
						new_x = move_placement_left( free_space_list, target_x, target_y, w, h );
					else
						new_x = move_placement_right( free_space_list, target_x, target_y, w, h );
				}
			}
			LOCAL_DEBUG_OUT( "move_left = %d, move_up = %d, new = %+d%+d, org = %+d%+d", move_left, move_up, new_x, new_y, target_x, target_y );
			changed = (target_x != new_x || target_y != new_y );
			target_x = new_x ;
			target_y = new_y ;
		}while( changed );
		apply_placement_result_asw( asw, XValue|YValue, target_x, target_y, 0, 0 );
		LOCAL_DEBUG_OUT( "success: status(%+d%+d), anchor(%+d,%+d)", asw->status->x, asw->status->y, asw->anchor.x, asw->anchor.y );
	}else
	{
		LOCAL_DEBUG_OUT( "failed%s","");
	}

	destroy_asvector( &free_space_list );
	return (selected >= 0) ;
}

static Bool do_random_placement( ASWindow *asw, ASWindowBox *aswbox, ASGeometry *area, Bool free_space_only )
{
	int selected = -1 ;
	unsigned int w = asw->status->width;
	unsigned int h = asw->status->height;
	static   CARD32 rnd32_seed = 345824357;
	ASVector *free_space_list = NULL;
	XRectangle *rects = NULL;
	int i ;
	long selected_deficiency = 1000000000 ;

#ifndef MY_RND32
#define MAX_MY_RND32		0x00ffffffff
#ifdef WORD64
#define MY_RND32() (rnd32_seed = ((1664525L*rnd32_seed)&MAX_MY_RND32)+1013904223L)
#else
#define MY_RND32() (rnd32_seed = (1664525L*rnd32_seed)+1013904223L)
#endif
#endif

	if( rnd32_seed == 345824357)
		rnd32_seed += time(NULL);

	/* even though we are not limited to free space - it is best to avoid windows with AvoidCover
	 * bit set */
	free_space_list =  build_free_space_list( asw, area,
											  free_space_only?ASWIN_LAYER(asw):AS_LayerHighest, AS_LayerHighest );
	rects = PVECTOR_HEAD(XRectangle,free_space_list);

	i = PVECTOR_USED(free_space_list);
	while( --i >= 0 )
	{	
		if( rects[i].width >= w && rects[i].height >=  h )
		{
			selected_deficiency = 0;
			if( selected >= 0 )
			{
				CARD32 r = MY_RND32();
				if( (r & 0x00000100) == 0 )
					continue;;

			}
			selected = i ;
		}
		else if( selected_deficiency > 0 )
		{
			int deficiency = 0 ;
			if( rects[i].width < w ) 
				deficiency += h * (w - rects[i].width );
			if( rects[i].height < h ) 
			{	
				deficiency += w * (h - rects[i].height );
				if( rects[i].width < w ) 
					deficiency -= (w - rects[i].width )*(h - rects[i].height );
			}		
#if 0		                                   
			/* we may use it if we are required to place window, so we can 
			 * select the largest area available. But ordinarily we should 
			 * default to manuall placement instead! : */
			if( deficiency < selected_deficiency || selected < 0 )
			{
				selected = i ; 
				selected_deficiency = deficiency ; 	
			}	 
#endif
		}	 
	}
	if( selected >= 0 )
	{
		unsigned int new_x = 0, new_y = 0;
		if( rects[selected].width > w )
		{
			new_x = MY_RND32();
			new_x = (new_x % ( rects[selected].width - w ));
		}

		if( rects[selected].height > h )
		{
			new_y = MY_RND32();
			new_y = (new_y % ( rects[selected].height - h ));
		}
		LOCAL_DEBUG_OUT( "rect %dx%d%+d%+d, new_pos = %+d%+d", 
						 rects[selected].width, rects[selected].height, rects[selected].x, rects[selected].y, new_x, new_y );
		apply_placement_result_asw( asw, XValue|YValue, rects[selected].x+new_x, rects[selected].y+new_y, 0, 0 );
		LOCAL_DEBUG_OUT( "success: status(%+d%+d), anchor(%+d,%+d)", asw->status->x, asw->status->y, asw->anchor.x, asw->anchor.y );
	}else
	{
		LOCAL_DEBUG_OUT( "failed%s","");
	}

	destroy_asvector( &free_space_list );
	return (selected >= 0);
}

static Bool
do_maximized_placement( ASWindow *asw, ASWindowBox *aswbox, ASGeometry *area)
{
	int selected = -1 ;
	unsigned int w = asw->status->width;
	unsigned int h = asw->status->height;
#ifdef HAVE_XINERAMA
	unsigned int x = asw->status->x;
	unsigned int y = asw->status->y;
#endif    
	ASVector *free_space_list = NULL;
	XRectangle *rects = NULL;
	int i ;

	/* even though we are not limited to free space - it is best to avoid windows with AvoidCover
	 * bit set */
	free_space_list =  build_free_space_list( asw, area, AS_LayerHighest, AS_LayerHighest );
	rects = PVECTOR_HEAD(XRectangle,free_space_list);

	i = PVECTOR_USED(free_space_list);
	/* now we need to find the biggest rectangle : */
	while( --i >= 0 )
		if( rects[i].width >= w && rects[i].height >=  h )
		{
		  /* if a rect has been selected */  
	  if( selected > 0 )
			{
			  /* if this rect is smaller than the selected one */  
		  if( rects[i].width * rects[i].height < rects[selected].width * rects[selected].height )
					continue;
			}
		  /* select this rectangle because it's bigger */  
	  selected = i ;
		}
		

		/* if a rect has NOT been selected*/
		if( selected  < 0 ) 
	{	                       /* we simply select the biggest area available : */
		i = PVECTOR_USED(free_space_list);
		while( --i >= 0 )
		{
		if( selected > 0 )
			{
				if( rects[i].width * rects[i].height < rects[selected].width * rects[selected].height )
					continue;
			}
			selected = i ;
		}
	}

		 
	if( selected >= 0 )
	{
		ASFlagType flags = 0 ;
		int max_width = rects[selected].width ;
		int max_height = rects[selected].height ;

#ifdef HAVE_XINERAMA
		/* the following block makes sure windows
		  are not maximized over multiply heads.*/
	
		int i;
		XRectangle *s = Scr.xinerama_screens;
		int dest_size = -1;
		int dest_rect =  0;
		int inter_width;
		int inter_height;
		
		if( s != NULL)
		{

		/* select the xinerama-screen holding most of the
		  window as it is before maximizing it */
		for(i = 0; i< Scr.xinerama_screens_num; ++i)
		{
			/* if window is not on this xin-rect at all. */
			if( ( x < s[i].x && x > s[i].x + s[i].width )||
			( y < s[i].y && y > s[i].y + s[i].height) )
				continue;
		
			/* window starts left of screen */
			if(s[i].x  > x)
				/* and ends on the screen */
				if( s[i].x + s[i].width > x + w)
					inter_width = x + w - s[i].x;
				else
					inter_width = s[i].width;
			else
				if( s[i].x + s[i].width > x + w )
					inter_width = s[i].width ;
				else
					inter_width = s[i].x + s[i].width - x;
			
			
			/* window starts above of screen */
			if(s[i].y  > y)
				/* and ends on the screen */
				if( s[i].y + s[i].height > y + h)
					inter_height = y + h - s[i].y;
				else
					inter_height = s[i].height;
			else
				if( s[i].y + s[i].height > y + h )
					inter_height = s[i].height ;
				else
					inter_height = s[i].y + s[i].height - y;
			
			if(inter_width * inter_height > dest_size )
			{
				/* I like this rect better than the last. */
				dest_rect = i;
				dest_size = inter_width * inter_height;
			}
				
			
		}
	
		
		if(rects[selected].x < s[dest_rect].x )
			rects[selected].x = s[dest_rect].x;
		if(rects[selected].y < s[dest_rect].y )
			rects[selected].y = s[dest_rect].y;
		if( max_width > s[dest_rect].width)
			max_width = s[dest_rect].width;
		if( max_height > s[dest_rect].height)
			max_height = s[dest_rect].height;
		}

#endif /* XINERAMA */

		
		save_aswindow_anchor( asw, ASWIN_GET_FLAGS( asw, AS_MaximizedX), ASWIN_GET_FLAGS( asw, AS_MaximizedY) );

		if( ASWIN_GET_FLAGS( asw, AS_MaximizedX)  )
			set_flags( flags, XValue|WidthValue );
		if( ASWIN_GET_FLAGS( asw, AS_MaximizedY)  )
			set_flags( flags, YValue|HeightValue );

		if( asw->maximize_ratio_x > 0 )
			max_width = (asw->maximize_ratio_x * max_width)/ 100 ;
		if( asw->maximize_ratio_y > 0 )
			max_height = (asw->maximize_ratio_y * max_height)/ 100 ;

		apply_placement_result_asw( asw, flags, rects[selected].x, rects[selected].y, max_width, max_height );
		LOCAL_DEBUG_OUT( "success: status(%+d%+d), anchor(%+d,%+d)", asw->status->x, asw->status->y, asw->anchor.x, asw->anchor.y );
	}else
	{
		LOCAL_DEBUG_OUT( "failed%s","");
	}

	destroy_asvector( &free_space_list );
	return (selected >= 0);
}



static Bool do_tile_placement( ASWindow *asw, ASWindowBox *aswbox, ASGeometry *area )
{
	int selected = -1 ;
	unsigned short w = asw->status->width;
	unsigned short h = asw->status->height;
	ASVector *free_space_list = NULL;
	XRectangle *rects = NULL;
	int i ;

	free_space_list =  build_free_space_list( asw, area, ASWIN_LAYER(asw), AS_LayerHighest );
	rects = PVECTOR_HEAD(XRectangle,free_space_list);

	i = PVECTOR_USED(free_space_list);
	while( --i >= 0 )
		if( rects[i].width >= w && rects[i].height >=  h )
		{
			if( selected > 0 )
			{
				if( get_flags( aswbox->flags, ASA_VerticalPriority ) )
				{
					if( get_flags( aswbox->flags, ASA_ReverseOrder ) || get_flags (aswbox->flags, ASA_ReverseOrderH ) )
					{
						if( rects[selected].x > rects[i].x )
							continue;
						if( rects[selected].x == rects[i].x && rects[selected].y > rects[i].y )
							continue;
					}else
					{
						if( rects[selected].x < rects[i].x )
							continue;
						if( rects[selected].x == rects[i].x && rects[selected].y < rects[i].y )
							continue;
					}
				}else if( get_flags( aswbox->flags, ASA_ReverseOrder ) || get_flags( aswbox->flags, ASA_ReverseOrderV ) )
				{
					if( rects[selected].y > rects[i].y )
						continue;
					if( rects[selected].y == rects[i].y && rects[selected].x > rects[i].x )
						continue;
				}else
				{
					if( rects[selected].y < rects[i].y )
						continue;
					if( rects[selected].y == rects[i].y && rects[selected].x < rects[i].x )
						continue;
				}

			}
			selected = i ;
		}

	if( selected >= 0 )
	{
		int spacer_x = aswbox->x_spacing ;
		int spacer_y = aswbox->y_spacing ;
		if(rects[selected].width < w+spacer_x)
			spacer_x = rects[selected].width - w ;
		if(rects[selected].height < h+spacer_y)
			spacer_y = rects[selected].height - h ;
		apply_placement_result_asw( asw, XValue|YValue, rects[selected].x+spacer_x, rects[selected].y+spacer_y, 0, 0 );
		LOCAL_DEBUG_OUT( "success: status(%+d%+d), anchor(%+d,%+d)", asw->status->x, asw->status->y, asw->anchor.x, asw->anchor.y );
	}else
	{
		LOCAL_DEBUG_OUT( "failed%s","");
	}

	destroy_asvector( &free_space_list );
	return (selected >= 0) ;
}

static Bool do_cascade_placement( ASWindow *asw, ASWindowBox *aswbox, ASGeometry *area )
{
	int newpos = aswbox->cascade_pos + 25;
	int x = newpos, y = newpos;

	if( get_flags( aswbox->flags, ASA_ReverseOrder ) ||
		( get_flags( aswbox->flags, ASA_ReverseOrderV ) &&
		  get_flags( aswbox->flags, ASA_ReverseOrderH ) ) ) 
	{
		x = ((int)(area->width) + area->x) - newpos ;
		y = ((int)(area->height) + area->y) - newpos ;
	}else if( get_flags( aswbox->flags, ASA_ReverseOrderV ) )
	{
		x = newpos + area->x ;
		y = ((int)(area->height) + area->y) - newpos ;
	}else if( get_flags( aswbox->flags, ASA_ReverseOrderH ) )
	{
		x = ((int)(area->width) + area->x) - newpos ;
		y = newpos + area->y ;
	}else
	{
		x = newpos + area->x ;
		y = newpos + area->y ;
	}

	if( x + asw->status->width > area->x+ area->width )
		x = (area->x+area->width - asw->status->width);
	if( y + asw->status->height > area->y+ area->height )
		y = (area->y+area->height - asw->status->height);

	asw->status->x = x - asw->status->viewport_x ;
	asw->status->y = y - asw->status->viewport_y ;

	aswbox->cascade_pos = newpos ;

	apply_placement_result_asw( asw, XValue|YValue, x, y, 0, 0 );

	return True;
}


static Bool do_manual_placement( ASWindow *asw, ASWindowBox *aswbox, ASGeometry *area )
{
	ASMoveResizeData *mvrdata;
	int start_x = 0, start_y = 0;

	ConfigureNotifyLoop();

	ASQueryPointerRootXY( &start_x, &start_y);
	move_canvas( asw->frame_canvas, start_x-2, start_y-2 );
	handle_canvas_config( asw->frame_canvas );

/*    moveresize_canvas( asw->frame_canvas, ((int)Scr.MyDisplayWidth - (int)asw->status->width)/2,
										  ((int)Scr.MyDisplayHeight - (int)asw->status->height)/2,
										   asw->status->width, asw->status->height );
	moveresize_canvas( asw->client_canvas, 0, 0, asw->status->width, asw->status->height );
	handle_canvas_config( asw->frame_canvas );
 */
	if( asw->status->width*asw->status->height < (Scr.Feel.OpaqueMove*Scr.MyDisplayWidth*Scr.MyDisplayHeight) / 100 )
	{
		LOCAL_DEBUG_OUT( "Mapping client window %lX", asw->client_canvas->w );
		map_canvas_window( asw->client_canvas, True );
		map_canvas_window( asw->frame_canvas, False );
		if( get_desktop_cover_window() != None ) 
		{
			Window w[2] ;
			w[0] = get_desktop_cover_window() ;
			w[1] = asw->frame ;
			XRaiseWindow( dpy, w[0] );
			XRestackWindows( dpy, w, 2 );
			ASSync(False);
		}			   
	}
	ASSync(False);
	ASWIN_SET_FLAGS( asw, AS_MoveresizeInProgress );
	mvrdata = move_widget_interactively(Scr.RootCanvas,
										asw->frame_canvas,
										NULL,
										apply_aswindow_move,
										complete_aswindow_move );
	if( mvrdata )
	{
		setup_aswindow_moveresize(asw, mvrdata );
		InteractiveMoveLoop ();
	}else
		ASWIN_CLEAR_FLAGS( asw, AS_MoveresizeInProgress );
	/* window may have been destroyed while we were placing it */
	 return (ASWIN_GET_FLAGS(asw,AS_Dead) == 0 && !get_flags(asw->wm_state_transition, ASWT_TO_WITHDRAWN ));
}

static int
move_to_closest_position (ASWindow *asw, ASGeometry *area, ASStatusHints *status, int *x_inout, int *y_inout, int max_layer )
{
	int x = *x_inout;
	int y = *y_inout;
	int selected_x = x ;
	int selected_y = y ;
	int w = status->width;
	int h = status->height;
	int i, selected = -1, selected_factor = 0;;

	LOCAL_DEBUG_OUT( "current=%dx%d%+d%+d", w, h, x, y );
	/* now we have to find the optimal rectangle from the list */
	ASVector *free_space_list =  build_free_space_list( asw, area, AS_LayerHighest, max_layer );
	XRectangle *rects = PVECTOR_HEAD(XRectangle,free_space_list);
	i = PVECTOR_USED(free_space_list);
	while( --i >= 0 )
		if( rects[i].width >= w && rects[i].height >=  h  )
		{
			int new_x = rects[i].x, new_y = rects[i].y ;
			int max_x = rects[i].x + rects[i].width - w ;
			int max_y = rects[i].y + rects[i].height - h ;
			int new_factor ;
			if( new_x < x )
				new_x = min( x, max_x );
			if( new_y < y )
				new_y = min( y, max_y );
			new_factor = (x - new_x) * (x - new_x) + (y - new_y) * (y - new_y);

			if( selected >= 0 && new_factor > selected_factor )
					continue;
			selected_x = new_x ;
			selected_y = new_y ;
			selected_factor = new_factor ;
			selected = i;
		}
	LOCAL_DEBUG_OUT( "selected: %d, %+d%+d", selected, selected_x, selected_y );
	destroy_asvector( &free_space_list );
	*x_inout = selected_x;
	*y_inout = selected_y;
	
	return selected;
}


static Bool 
find_closest_position( ASWindow *asw, ASGeometry *area, ASStatusHints *status, int *closest_x, int *closest_y, int max_layer )
{
	int selected_x = status->x+Scr.Vx;
	int selected_y = status->y+Scr.Vy;
	/* pass 1: find rectangle that is the closest to current position :  */
	int selected = move_to_closest_position (asw, area, status, &selected_x, &selected_y, max_layer);	
	if( selected >= 0 ) 
	{
		*closest_x = selected_x ;    
		*closest_y = selected_y ;    
	}    
	return (selected >= 0);
}

static Bool do_pointer_placement( ASWindow *asw, ASWindowBox *aswbox, ASGeometry *area )
{
	int x = area->x, y = area->y;

	ASQueryPointerRootXY( &x, &y);
	
/*fprintf( stderr, "%d: x = %d, y = %d\n", __LINE__, x, y);*/
	x += Scr.Vx - (int)asw->status->width/2;
	y += Scr.Vy - (int)asw->status->height/2;
/*fprintf( stderr, "%d: x = %d, y = %d\n", __LINE__, x, y);*/
	if ( x < area->x ) 
		x = area->x;
	else if( x + asw->status->width > area->x+ area->width )
		x = (area->x+area->width - asw->status->width);

	if ( y < area->y ) 
		y = area->y;
	else if( y + asw->status->height > area->y+ area->height )
		y = (area->y+area->height - asw->status->height);

	move_to_closest_position (asw, area, asw->status, &x, &y, AS_LayerHighest);

/*fprintf( stderr, "%d: x = %d, y = %d\n", __LINE__, x, y);*/
	asw->status->x = x - asw->status->viewport_x ;
	asw->status->y = y - asw->status->viewport_y ;

	apply_placement_result_asw( asw, XValue|YValue, x, y, 0, 0 );

	return True;
}

static Bool do_closest_placement( ASWindow *asw, ASWindowBox *aswbox, ASGeometry *area )
{
	int selected_x = 0 ;
	int selected_y = 0 ;

	if (find_closest_position (asw, area, asw->status, &selected_x, &selected_y, AS_LayerHighest))
	{
		apply_placement_result_asw( asw, XValue|YValue, selected_x, selected_y, 0, 0 );
		LOCAL_DEBUG_OUT( "success: status(%+d%+d), anchor(%+d,%+d)", asw->status->x, asw->status->y, asw->anchor.x, asw->anchor.y );
		return True;
	}else
	{
		LOCAL_DEBUG_OUT( "failed%s","");
	}

	return False ;
}

static Bool
place_aswindow_in_windowbox( ASWindow *asw, ASWindowBox *aswbox, ASUsePlacementStrategy which, Bool force)
{
	ASGeometry area ;
	Bool res = False ;

	if(ASWIN_GET_FLAGS(asw,AS_Dead) || get_flags(asw->wm_state_transition, ASWT_TO_WITHDRAWN ))
		return False;

	area = aswbox->area ;
	if( !get_flags( aswbox->flags, ASA_Virtual ) )
	{
		area.x += Scr.Vx; /*asw->status->viewport_x ;*/
		area.y += Scr.Vy; /*asw->status->viewport_y ;*/
		if( !force )
		{
			if( area.x >= Scr.VxMax + Scr.MyDisplayWidth )
				return False;
			if( area.y >= Scr.VyMax + Scr.MyDisplayHeight )
				return False;
		}
		if( area.width <= 0 )
			area.width = (Scr.VxMax + Scr.MyDisplayWidth)- area.x ;
		else if( area.x + area.width > Scr.VxMax + Scr.MyDisplayWidth )
			area.width = Scr.VxMax + Scr.MyDisplayWidth - area.x ;
		if( area.height <= 0 )
			area.height = (Scr.VyMax + Scr.MyDisplayHeight)- area.y ;
		else if( area.y + area.height > Scr.VyMax + Scr.MyDisplayHeight )
			area.height = Scr.VyMax + Scr.MyDisplayHeight - area.y ;
	}
	LOCAL_DEBUG_OUT( "placement area is %dx%d%+d%+d", area.width, area.height, area.x, area.y );

	if( !force )
	{
		if( get_flags( asw->status->flags, AS_StartViewportX ) )
			if( asw->status->viewport_x < area.x || asw->status->viewport_x >= area.x +area.width )
				return False;
		if( get_flags( asw->status->flags, AS_StartViewportY ) )
			if( asw->status->viewport_y < area.y || asw->status->viewport_y >= area.y +area.height )
				return False;
	}

	if( ASWIN_GET_FLAGS(asw, AS_MaximizedX|AS_MaximizedY ) )
		return do_maximized_placement( asw, aswbox, &area );

	if( which == ASP_UseMainStrategy )
	{
		if( aswbox->main_strategy == ASP_SmartPlacement )
			return do_smart_placement( asw, aswbox, &area );
		else if( aswbox->main_strategy == ASP_RandomPlacement )
			return do_random_placement( asw, aswbox, &area, True );
		else if( aswbox->main_strategy == ASP_Tile )
			return do_tile_placement( asw, aswbox, &area );
		else if( aswbox->main_strategy == ASP_UnderPointer )
			res = do_pointer_placement( asw, aswbox, &area );
		if( force )
			do_tile_placement( asw, aswbox, &area );
	}else
	{
		if( aswbox->backup_strategy == ASP_RandomPlacement )
			res = do_random_placement( asw, aswbox, &area, False );
		else if( aswbox->backup_strategy == ASP_Cascade )
			res = do_cascade_placement( asw, aswbox, &area );
		else if( aswbox->backup_strategy == ASP_UnderPointer )
			res = do_pointer_placement( asw, aswbox, &area );

		if( aswbox->backup_strategy == ASP_Manual || (force && !res) )
			res = do_manual_placement( asw, aswbox, &area );
	}
	return res;
}


Bool place_aswindow( ASWindow *asw )
{
	/* if window has predefined named windowbox for it - we use only this windowbox
	 * otherwise we use all suitable windowboxes in two passes :
	 *   we first try and apply main strategy to place window in the empty space for each box
	 *   if all fails we apply backup strategy of the default windowbox
	 */
	ASWindowBox *aswbox = NULL ;

	LOCAL_DEBUG_CALLER_OUT( "%p", asw );
	if( AS_ASSERT(asw))
		return False;

	LOCAL_DEBUG_OUT( "hints(%p),status(%p)", asw->hints, asw->status );
	if( AS_ASSERT(asw->hints) || AS_ASSERT(asw->status)  )
		return False;

	LOCAL_DEBUG_OUT( "status->geom(%dx%d%+d%+d), anchor->geom(%dx%d%+d%+d)",
					 asw->status->width, asw->status->height, asw->status->x, asw->status->y,
					 asw->anchor.width, asw->anchor.height, asw->anchor.x, asw->anchor.y );

	if( asw->hints->windowbox_name )
	{
		aswbox = find_window_box( &(Scr.Feel), asw->hints->windowbox_name );
		if( aswbox != NULL )
		{
			if( !place_aswindow_in_windowbox( asw, aswbox, ASP_UseMainStrategy, False ) )
				return place_aswindow_in_windowbox( asw, aswbox, ASP_UseBackupStrategy, True );
			return True;
		}
	}

	/* search for a window-box if none was specified.*/
	if( aswbox == NULL )
	{
		int i, t;
	ASWindowBox **aswbox_sorted = safemalloc(sizeof(ASWindowBox *) * Scr.Feel.window_boxes_num);
	aswbox = &(Scr.Feel.window_boxes[0]);
	
	/* the following code will make sure window-boxes which have the
	   virtual-flag set and are not on the current viewport will
	   be considered for placement after all others. */
	t = Scr.Feel.window_boxes_num;
	for( i = 0; i < Scr.Feel.window_boxes_num; ++i)
	{
		if( get_flags(aswbox[i].flags, ASA_Virtual)
			&& (( asw->status->viewport_x / Scr.MyDisplayWidth
			  != aswbox[i].area.x / Scr.MyDisplayWidth ) ||
			( asw->status->viewport_y / Scr.MyDisplayHeight
			  != aswbox[i].area.y / Scr.MyDisplayHeight) )
		  )
			/* place window-box at the end */
			aswbox_sorted[--t] = &aswbox[i];
		else
			/* place window-box at the front */
			aswbox_sorted[i - Scr.Feel.window_boxes_num + t ] = &aswbox[i];
	}
	
		for( i = 0 ; i < Scr.Feel.window_boxes_num ; ++i )
		{
			LOCAL_DEBUG_OUT("window_box \"%s\": main_strategy = %d, backup_strategy = %d", 
							aswbox_sorted[i]->name, aswbox_sorted[i]->main_strategy, aswbox_sorted[i]->backup_strategy);
			
			if( IsValidDesk(aswbox_sorted[i]->desk) && aswbox_sorted[i]->desk != asw->status->desktop )
				continue;
			if( aswbox_sorted[i]->min_layer > asw->status->layer || aswbox_sorted[i]->max_layer < asw->status->layer )
				continue;
			if( aswbox_sorted[i]->min_width > asw->status->width || (aswbox_sorted[i]->max_width > 0 && aswbox_sorted[i]->max_width < asw->status->width) )
				continue;
			if( aswbox_sorted[i]->min_height > asw->status->height || (aswbox_sorted[i]->max_height > 0 && aswbox_sorted[i]->max_height < asw->status->height) )
			continue;


			if( ASWIN_GET_FLAGS(asw, AS_MaximizedX|AS_MaximizedY ) )
			{
			  int win_x = get_flags(aswbox_sorted[i]->flags, ASA_Virtual) ? asw->status->viewport_x: asw->status->x;
			  int win_y = get_flags(aswbox_sorted[i]->flags, ASA_Virtual) ? asw->status->viewport_y: asw->status->y;
			  
			  
			  if( aswbox_sorted[i]->area.x > win_x +(int)(asw->status->width) || 
					aswbox_sorted[i]->area.y > win_y +(int)(asw->status->height)||
					aswbox_sorted[i]->area.x+(int)aswbox[i].area.width < win_x ||
					aswbox_sorted[i]->area.y+(int)aswbox[i].area.height < win_y )
					continue;
			}	 

			if( ASWIN_GET_FLAGS(asw, AS_MaximizedX|AS_MaximizedY ) )
			{
				Bool retval = place_aswindow_in_windowbox( asw, aswbox_sorted[i], ASP_UseBackupStrategy, True );
				free( aswbox_sorted );
				return retval;
			}			
			else if( place_aswindow_in_windowbox( asw, aswbox_sorted[i], ASP_UseMainStrategy , False )){
				free( aswbox_sorted );
				return True;
			}        
	}
	free( aswbox_sorted );
	}
	return place_aswindow_in_windowbox( asw, Scr.Feel.default_window_box, ASP_UseBackupStrategy, True );
}

Bool
avoid_covering_aswin_iter_func(void *data, void *aux_data)
{
	ASWindow *asw = (ASWindow*)data ;
	ASAvoidCoverAuxData *ac_aux_data = (ASAvoidCoverAuxData*)aux_data;
	ASWindow *new_aswin = ac_aux_data->new_aswin ;
	ASWindowBox *aswbox = ac_aux_data->aswbox;
	ASGeometry *area = ac_aux_data->area;

	if( asw && ASWIN_DESK(asw) == ASWIN_DESK(new_aswin) && asw != new_aswin )
	{
		ASStatusHints *n = new_aswin->status;
		ASStatusHints *o = asw->status ;
		int dn, ds, dw, de, min_dh ;
		
		LOCAL_DEBUG_OUT( "comparing to %dx%d%+d%+d, layer = %d", asw->status->width, asw->status->height, asw->status->x, asw->status->y, ASWIN_LAYER(asw) );

		/* we want to move out even lower layer windows so they would not be overlapped by us */
		if( /*ASWIN_LAYER(asw) < ASWIN_LAYER(new_aswin) ||*/ ASWIN_GET_FLAGS(asw, AS_Iconic ))
			return True;

		dw = o->x + (int)o->width - n->x;		
		de = (int)(n->x+n->width) - o->x;		   
		dn = o->y + (int)o->height - n->y;		   
		ds = (int)(n->y+n->height) - o->y;		   
		LOCAL_DEBUG_OUT( "deltas : w=%d e=%d s=%d n=%d", dw, de, dn, ds );
		if( dw > 0 && de > 0 && dn > 0 && ds > 0 )
		{
			ASGeometry clip_area = *area ;
			
			min_dh = min(dw, de);
			if( min_dh < dn && min_dh < ds ) 
			{	
				if( dw <= de ) 
				{/* better move window westwards */
					clip_area.width = (n->x <= clip_area.x)?1:  n->x - clip_area.x ;	  
				}else /* better move window eastwards */
				{	
					int d = n->x + (int)n->width - clip_area.x ;
					clip_area.x = n->x + n->width ;	  	  
					clip_area.width = (d >= clip_area.width)?1: clip_area.width - d ;
				}
			}else if( dn <= ds ) /* better move window southwards */
				clip_area.height = (n->y <= clip_area.y)?1: n->y - clip_area.y ;	  							  
			else	
			{	
				int d = n->y + (int)n->height - clip_area.y ;
				clip_area.y = n->y + n->height ;	  	   
				clip_area.height = (d >= clip_area.height)?1: clip_area.height - d ;
			}
			LOCAL_DEBUG_OUT( "adjusted area is %dx%d%+d%+d", clip_area.width, clip_area.height, clip_area.x, clip_area.y );
			/* move only affected windows : */
			if( do_closest_placement( asw, aswbox, &clip_area ) )
			{
				anchor2status ( asw->status, asw->hints, &(asw->anchor));
				/* now lets actually resize the window : */
				apply_window_status_size(asw, get_orientation_data(asw));
			}
		}
	}
	return True;
}

void do_enforce_avoid_cover(ASWindow *asw )
{
	if( asw && ASWIN_HFLAGS( asw, AS_AvoidCover|AS_ShortLived ) == AS_AvoidCover )
	{
		ASWindowBox aswbox ;
		ASAvoidCoverAuxData aux_data ;
		/* we need to move all the res of the window out of the area occupied by us */
		LOCAL_DEBUG_OUT( "status = %dx%d%+d%+d, layer = %d", asw->status->width, asw->status->height, asw->status->x, asw->status->y, ASWIN_LAYER(asw) );
	/* if window has predefined named windowbox for it - we use only this windowbox
	 * otherwise we use all suitable windowboxes in two passes :
	 *   we first try and apply main strategy to place window in the empty space for each box
	 *   if all fails we apply backup strategy of the default windowbox
	 */
		aux_data.new_aswin = asw ;
		aux_data.aswbox = &aswbox;
		aswbox.name = mystrdup("default");
		aswbox.area.x = Scr.Vx;
		aswbox.area.y = Scr.Vy ;
		aswbox.area.width = Scr.MyDisplayWidth ;
		aswbox.area.height = Scr.MyDisplayHeight ;
		aswbox.main_strategy = ASP_Manual ;
		aswbox.backup_strategy = ASP_Manual ;
		/* we should enforce this one : */
		aswbox.desk = INVALID_DESK ;
		aswbox.min_layer = AS_LayerLowest;
		aswbox.max_layer = AS_LayerHighest;

		aux_data.area = &(aswbox.area);

		iterate_asbidirlist( Scr.Windows->clients, avoid_covering_aswin_iter_func, (void*)&aux_data, NULL, False );

		free( aswbox.name );

	}
}

void
delayed_enforce_avoid_cover( void* vdata )
{
	ASWindow *asw = (ASWindow*)vdata;
	if( asw && asw->magic == MAGIC_ASWINDOW ) 
		do_enforce_avoid_cover( asw );
}

void
enforce_avoid_cover( ASWindow *asw )
{
	/* we do not want to enforce avoid cover right away - clients may want to reposition 
	   themselves automatically */

	if( asw ) 
		if( ASWIN_HFLAGS(asw, AS_AvoidCover|AS_ShortLived) == AS_AvoidCover )
		{
/* we don't want to remove by  data as there are other functions associated wit this
*			while( timer_remove_by_data( (void*)asw ) ); */
			timer_new (500, delayed_enforce_avoid_cover, (void*)asw);	
		}
}


void obey_avoid_cover(ASWindow *asw, ASStatusHints *tmp_status, XRectangle *tmp_anchor, int max_layer )
{
	if( asw )
	{
		ASWindowBox aswbox ;
		int selected_x = 0 ;
		int selected_y = 0 ;
		int left = Scr.Vx, right = Scr.Vx+Scr.MyDisplayWidth ;
		int top = Scr.Vy, bottom = Scr.Vy+Scr.MyDisplayHeight ;

		/* we need to move all the res of the window out of the area occupied by us */
		LOCAL_DEBUG_OUT( "status = %dx%d%+d%+d, layer = %d", asw->status->width, asw->status->height, asw->status->x, asw->status->y, ASWIN_LAYER(asw) );
	/* if window has predefined named windowbox for it - we use only this windowbox
	 * otherwise we use all suitable windowboxes in two passes :
	 *   we first try and apply main strategy to place window in the empty space for each box
	 *   if all fails we apply backup strategy of the default windowbox
	 */
		LOCAL_DEBUG_OUT( "max_layer = %d", max_layer);

		aswbox.name = mystrdup("default");
		if( !ASWIN_GET_FLAGS(asw, AS_Sticky))
		{
			if( asw->status->x < 0 )
				left = 0 ; 
			if( asw->status->x+(int)asw->status->width >= Scr.MyDisplayWidth )		   
				right = Scr.VxMax + Scr.MyDisplayWidth ; 
			if( asw->status->y < 0 )
				top = 0 ; 
			if( asw->status->y+(int)asw->status->height >= Scr.MyDisplayHeight )		   
				bottom = Scr.VyMax + Scr.MyDisplayHeight ; 
		}	 
#if 0
		aswbox.area.x = Scr.Vx;
		aswbox.area.y = Scr.Vy ;
		aswbox.area.width = Scr.MyDisplayWidth ;
		aswbox.area.height = Scr.MyDisplayHeight ;
#else
		aswbox.area.x = left;
		aswbox.area.y = top;
		aswbox.area.width = right - left ;
		aswbox.area.height = bottom - top ;
#endif
		aswbox.main_strategy = ASP_Manual ;
		aswbox.backup_strategy = ASP_Manual ;
		/* we should enforce this one : */
		aswbox.desk = INVALID_DESK ;
		aswbox.min_layer = AS_LayerLowest;
		aswbox.max_layer = AS_LayerHighest;
			
		if( find_closest_position (asw, &(aswbox.area), tmp_status, &selected_x, &selected_y, max_layer) )
			apply_placement_result (tmp_status, tmp_anchor, asw->hints, XValue|YValue, selected_x, selected_y, 0, 0);            

		free (aswbox.name);
	}
}

/**************************************************************************
 * ************************************************************************
 * ************************************************************************
 **************************************************************************/
#if 0
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
			/* if window is completely in this xinerama-screen */
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
				grab_server();
				DragWidth = tmp_win->frame_width + 2 * tmp_win->bw;
				DragHeight = tmp_win->frame_height + 2 * tmp_win->bw;
				XMapRaised (dpy, Scr.SizeWindow);
				moveLoop (tmp_win, 0, 0, DragWidth, DragHeight, &xl, &yt, False, True);
				XUnmapWindow (dpy, Scr.SizeWindow);
				ungrab_server();
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

