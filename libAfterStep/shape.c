/****************************************************************************
 *
 * Copyright (c) 2003 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/

#define LOCAL_DEBUG
#include "../configure.h"
#include "../include/afterbase.h"
#include "shape.h"

ASVector *
create_shape()
{
	return create_asvector( sizeof(XRectangle) );
}

void
destroy_shape( ASVector **shape )
{
	destroy_asvector( shape );
}


/* using int type here to avoid problems with signed/unsigned comparisonand for simplier and faster code */
static Bool
assimilate_rectangle( int new_x, int new_y, int new_width, int new_height, ASVector *shape )
{
	XRectangle *shape_rects = PVECTOR_HEAD(XRectangle, shape);
	int shape_rect_count = PVECTOR_USED( shape );
	int i ;
	Bool changed = False ;
	int selected = -1 ;
	int selected_area = 0 ;

	if( new_width == 0 || new_height == 0 )
		return False;

	LOCAL_DEBUG_OUT(" assimilating %dx%d%+d%+d", new_width, new_height, new_x, new_y );

	/* pass 1 : find rectangles that are entirely inside us :  */
	for( i = 0 ; i < shape_rect_count ; ++i )
	{
		if( shape_rects[i].x >= new_x &&
			shape_rects[i].y >= new_y &&
			shape_rects[i].x+shape_rects[i].width <= new_x+new_width &&
			shape_rects[i].y+shape_rects[i].height <= new_y+new_height )
		{
			LOCAL_DEBUG_OUT(" \tassimilated by %dx%d%+d%+d", shape_rects[i].width, shape_rects[i].height, shape_rects[i].x, shape_rects[i].y );
			shape_rects[i].x = new_x ;
			shape_rects[i].y = new_y ;
			shape_rects[i].width = new_width ;
			shape_rects[i].height = new_height ;
			return True;
		}
		/* horizontally overlapping */
		if( new_y == shape_rects[i].y &&
			new_height == shape_rects[i].height )
		{
			if( new_x + new_width >= shape_rects[i].x &&
				new_x <= shape_rects[i].x + shape_rects[i].width )
			{
				int w1 = new_x + new_width ;
				int w2 = shape_rects[i].x + shape_rects[i].width ;
				LOCAL_DEBUG_OUT(" \thorizontally overlapping %dx%d%+d%+d", shape_rects[i].width, shape_rects[i].height, shape_rects[i].x, shape_rects[i].y );
				shape_rects[i].x = min(new_x,(int)shape_rects[i].x)  ;
				shape_rects[i].width = max(w1, w2)-shape_rects[i].x;
				return True;
			}
		}
		/* vertically overlapping */
		if( new_x == shape_rects[i].x &&
			new_width == shape_rects[i].width )
		{
			if( new_y + new_height >= shape_rects[i].y &&
				new_y <= shape_rects[i].y + shape_rects[i].height )
			{
				int h1 = new_y + new_height ;
				int h2 = shape_rects[i].y + shape_rects[i].height ;
				LOCAL_DEBUG_OUT(" \tvertically overlapping %dx%d%+d%+d", shape_rects[i].width, shape_rects[i].height, shape_rects[i].x, shape_rects[i].y );
				shape_rects[i].y = min(new_y,(int)shape_rects[i].y)  ;
				shape_rects[i].height = max(h1, h2)-shape_rects[i].y;
				return True;
			}
		}
	}
	/* pass 2 : find largest intersecting rectangle :  */
	for( i = 0 ; i < shape_rect_count ; ++i )
	{
		int intersect_width = (shape_rects[i].x+(int)shape_rects[i].width - new_x);
		int intersect_height = (shape_rects[i].y+(int)shape_rects[i].height - new_y);

		if( shape_rects[i].x  > new_x )
		{
			if( shape_rects[i].x >= new_x+new_width )
				intersect_width = 0 ;
			else
				intersect_width -= shape_rects[i].x - new_x ;
		}
		if( shape_rects[i].y  > new_y )
		{
			if( shape_rects[i].y >= new_y+new_height )
				intersect_height = 0 ;
			else
				intersect_height -= shape_rects[i].y - new_y ;
		}
		if( intersect_width > 0 && intersect_height > 0 )
		{
			int new_area = intersect_width * intersect_height ;
			if( new_area > selected_area )
			{
				selected = i ;
				selected_area = new_area ;
			}
		}
	}
	if( selected >= 0 )
	{
		int x1 = shape_rects[selected].x ;
		int y1 = shape_rects[selected].y ;
		int x2 = x1+shape_rects[selected].width ;
		int y2 = y1+shape_rects[selected].height ;
		int top = (y1>new_y)?y1 - new_y:0 ;
		int bottom = (new_y+new_height > y2 )?new_y+new_height - y2:0 ;
		int left = (x1 > new_x)?x1 - new_x:0 ;
		int right = (new_x+new_width > x2)?new_x+new_width - x2:0 ;
		LOCAL_DEBUG_OUT(" \tintersected %dx%d%+d%+d", shape_rects[selected].width, shape_rects[selected].height, shape_rects[selected].x, shape_rects[selected].y );

		if( top > 0 )
		{ /* there could be entire row of new segments above selected rectangle */
			if( new_x < x1 )              /* left top segment */
				if( assimilate_rectangle( new_x, new_y, x1 - new_x,         top, shape ) )
					changed = True ;
			if( new_x + new_width > x2 )              /* right top segment */
				if( assimilate_rectangle( x2,    new_y, new_x+new_width-x2, top, shape ) )
					changed = True ;
			/* center top segment */
			if( assimilate_rectangle    ( new_x + left,    new_y, new_width-(left+right), top, shape ) )
				changed = True ;
		}

		if( bottom > 0 )
		{ /* there could be entire row of new segments above selected rectangle */
			if( new_x < x1 )              /* left bottom segment */
				if( assimilate_rectangle( new_x, y2, x1 - new_x, bottom, shape ) )
					changed = True ;
			if( new_x + new_width > x2 )              /* right bottom segment */
				if( assimilate_rectangle( x2, y2, new_x+new_width-x2, bottom, shape ) )
					changed = True ;
			/* center bottom segment */
			if( assimilate_rectangle( new_x+left, y2, new_width-(left+right), bottom, shape ) )
				changed = True ;
		}

		if( left > 0 )
		{ /* there could be left center segment */
			if( assimilate_rectangle( new_x, new_y+top, left, new_height-(top+bottom), shape ) )
				changed = True ;
		}
		if( right > 0 )
		{ /* there could be right center segment */
			if( assimilate_rectangle( new_x+new_width-right, new_y+top, right, new_height-(top+bottom), shape ) )
				changed = True ;
		}
	}else
	{	/* simply appending rectangle to the list : */
		XRectangle new_rect ;
		new_rect.x = new_x ;
		new_rect.y = new_y ;
		new_rect.width  = new_width ;
		new_rect.height = new_height ;
		LOCAL_DEBUG_OUT(" \tappending %dx%d%+d%+d as new", new_width, new_height, new_x, new_y );
		vector_insert_elem( shape, &new_rect, 1, NULL, False );
		changed = True ;
	}
	return changed;
}

/* using int type here to avoid problems with signed/unsigned comparisonand for simplier and faster code */
static Bool
subtract_rectangle( int new_x, int new_y, int new_width, int new_height, ASVector *shape )
{
	XRectangle *shape_rects = PVECTOR_HEAD(XRectangle, shape);
	int shape_rect_count = PVECTOR_USED( shape );
	int i ;
	Bool changed = False ;
	XRectangle segments[4] ;
	int seg_count ;

	if( new_width == 0 || new_height == 0 )
		return False;

	LOCAL_DEBUG_OUT(" assimilating %dx%d%+d%+d", new_width, new_height, new_x, new_y );

	/* pass 1 : find rectangles that are entirely inside us and delete them all :  */
	i = shape_rect_count ;
	while( --i >= 0 )
	{
		if( shape_rects[i].x >= new_x &&
			shape_rects[i].y >= new_y &&
			shape_rects[i].x+shape_rects[i].width <= new_x+new_width &&
			shape_rects[i].y+shape_rects[i].height <= new_y+new_height )
		{
			LOCAL_DEBUG_OUT(" \ttrashing %dx%d%+d%+d", shape_rects[i].width, shape_rects[i].height, shape_rects[i].x, shape_rects[i].y );
			vector_remove_index( shape, i );
			changed = True ;
		}
	}
	/* pass 2 : adjust all the intersected rectangles :  */
	for( i = 0 ; i < shape_rect_count ; ++i )
	{
		int left   = shape_rects[i].x ;
		int right  = shape_rects[i].x + shape_rects[i].width ;
		int top    = shape_rects[i].y ;
		int bottom = shape_rects[i].y + shape_rects[i].height ;
		int s_left = new_x ;
		int s_right = new_x + new_width ;
		int s_top = new_y ;
		int s_bottom = new_y + new_height ;


		if( left >= s_right || top >= s_bottom || right <= s_left || bottom <= s_top )
			continue;
		seg_count = 0 ;
		if( top < s_top )
		{/* there will be top portion */
			segments[seg_count].x = left ;
			segments[seg_count].y = top ;
			segments[seg_count].width = right - left ;
			segments[seg_count].height = s_top - top ;
			++seg_count ;
		}
		if( left < s_left )
		{/* there will be left segment */
			segments[seg_count].x = left ;
			segments[seg_count].y = max(top,s_top) ;
			segments[seg_count].width = s_left - left ;
			segments[seg_count].height = min(s_bottom, bottom) - segments[seg_count].y ;
			++seg_count ;
		}
		if( right > s_right )
		{/* there will be right segment */
			segments[seg_count].x = s_right ;
			segments[seg_count].y = max(top,s_top) ;
			segments[seg_count].width = right - s_right ;
			segments[seg_count].height = min(s_bottom, bottom) - segments[seg_count].y ;
			++seg_count ;
		}
		if( bottom > s_bottom )
		{/* there will be right segment */
			segments[seg_count].x = left ;
			segments[seg_count].y = s_bottom ;
			segments[seg_count].width = right - left ;
			segments[seg_count].height = bottom - s_bottom ;
			++seg_count ;
		}
		shape_rects[i] = segments[--seg_count] ;
		if( seg_count > 0 )
		{
			append_vector( shape, &segments[0], seg_count );
			shape_rects = PVECTOR_HEAD(XRectangle, shape);
			shape_rect_count = PVECTOR_USED( shape );
		}
	}

	return changed;
}


Bool
add_shape_rectangles( ASVector *shape, XRectangle *rects, unsigned int count, int x_origin, int y_origin, unsigned int clip_width, unsigned int clip_height )
{
	int i ;
	int new_x, new_y, new_width, new_height ;
	Bool changed = False ;

	if( shape == NULL || rects == NULL || count == 0 )
		return False;

	LOCAL_DEBUG_OUT("adding %d rectangles at %+d%+d clipped by %dx%d", count, x_origin, y_origin, clip_width, clip_height  );
	for( i = 0 ; i < count ; ++i )
	{
		new_x = rects[i].x + x_origin ;
		if( new_x >= (int)clip_width )
			continue;
		new_y = rects[i].y + y_origin ;
		if( new_y >= (int)clip_height )
			continue;
		new_width  =( new_x + (int)rects[i].width  > (int)clip_width  )?(int)clip_width  - new_x:(int)rects[i].width  ;
		new_height =( new_y + (int)rects[i].height > (int)clip_height )?(int)clip_height - new_y:(int)rects[i].height ;

		if( assimilate_rectangle( new_x, new_y, new_width, new_height, shape ) )
			changed = True ;
	}
	return changed;
}


Bool
print_shape( ASVector *shape )
{
	if( shape )
	{
		XRectangle *shape_rects = PVECTOR_HEAD(XRectangle, shape);
		int shape_rect_count = PVECTOR_USED( shape );
		int i ;

		show_progress( "Printing shape of %d rectangles : ", shape_rect_count );
		for( i = 0 ; i < shape_rect_count ; ++i )
			show_progress( "\trects[%d] = %dx%d%+d%+d;", i, shape_rects[i].width, shape_rects[i].height, shape_rects[i].x, shape_rects[i].y );
		return True;

	}
	return False;
}


Bool
add_shape_mask( struct ASVector *shape, struct ASImage *mask_im )
{

	return False;
}

Bool
subtract_shape_rectangle( ASVector *shape, XRectangle *rects, unsigned int count, int x_origin, int y_origin, unsigned int clip_width, unsigned int clip_height )
{
	int i ;
	int new_x, new_y, new_width, new_height ;
	Bool changed = False ;

	if( shape == NULL || rects == NULL || count == 0 )
		return False;

	LOCAL_DEBUG_OUT("subtract %d rectangles at %+d%+d clipped by %dx%d", count, x_origin, y_origin, clip_width, clip_height  );
	for( i = 0 ; i < count ; ++i )
	{
		new_x = rects[i].x + x_origin ;
		if( new_x >= (int)clip_width )
			continue;
		new_y = rects[i].y + y_origin ;
		if( new_y >= (int)clip_height )
			continue;
		new_width  =( new_x + (int)rects[i].width  > (int)clip_width  )?(int)clip_width  - new_x:(int)rects[i].width  ;
		new_height =( new_y + (int)rects[i].height > (int)clip_height )?(int)clip_height - new_y:(int)rects[i].height ;

		if( subtract_rectangle( new_x, new_y, new_width, new_height, shape ) )
			changed = True ;
	}
	return changed;
}

Bool
query_shape_from_window( struct ASVector *shape, Window w )
{

	return False;
}

Bool
apply_shape_to_window( struct ASVector *shape, Window w )
{

	return False;
}



