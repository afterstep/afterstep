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

#include <string.h>

#include "../configure.h"

#include "asapp.h"
#include "../libAfterImage/afterimage.h"
#include "screen.h"
#include "shape.h"
#include "canvas.h"


inline Bool
get_current_canvas_geometry( ASCanvas * pc, int *px, int *py, unsigned int *pwidth, unsigned int *pheight, unsigned int *pbw )
{
	Window wdumm ;
	unsigned int udumm ;
	int dumm;
	if( pc == NULL )
		return False;

	if( px == NULL ) px = &dumm ;
	if( py == NULL ) py = &dumm ;
	if( pwidth == NULL ) pwidth = &udumm ;
	if( pheight == NULL ) pheight = &udumm ;
	if( pbw == NULL ) pbw = &udumm ;

	return (XGetGeometry(dpy, pc->w, &wdumm, px, py, pwidth, pheight, pbw, &udumm )!= 0);
}

static void
set_canvas_shape_to_rectangle( ASCanvas * pc )
{
#ifdef SHAPE
	unsigned int width, height, bw ;
	XRectangle    rect;
	rect.x = 0;
	rect.y = 0;
#ifdef STRICT_GEOMETRY
	get_current_canvas_geometry( pc, NULL, NULL, &width, &height, &bw );
#else
	width = pc->width ;
	height = pc->height ;
	bw = pc->bw ;
#endif
	rect.width  = width+bw*2;
	rect.height = height+bw*2;
	LOCAL_DEBUG_OUT( "XShapeCombineRectangles(%lX) (%dx%d%+d%+d)", pc->w, rect.width, rect.height, -bw, -bw );
	XShapeCombineRectangles ( dpy, pc->w, ShapeBounding,
								-bw, -bw, &rect, 1, ShapeSet, Unsorted);
	set_flags( pc->state, CANVAS_SHAPE_SET );
#endif
}

static void
set_canvas_shape_to_nothing( ASCanvas * pc )
{
#ifdef SHAPE
	LOCAL_DEBUG_OUT( "XShapeCombineMask(%lX) (clearing mask)",  pc->w );
	XShapeCombineMask( dpy, pc->w, ShapeBounding, 0, 0, None, ShapeSet );
	clear_flags( pc->state, CANVAS_SHAPE_SET );
#endif
	if( pc->shape )
		destroy_shape( &pc->shape );
}




/********************************************************************/
/* ASCanvas :                                                       */
/********************************************************************/
static ASFlagType
refresh_canvas_config (ASCanvas * pc)
{
	ASFlagType          changed = 0;

	if (pc && pc->w != None)
	{
		int           root_x = pc->root_x, root_y = pc->root_y, dumm;
		unsigned int  width = pc->width, height = pc->height, udumm;
		Window        wdumm;
		unsigned int  bw ;

		XTranslateCoordinates (dpy, pc->w, ASDefaultRoot, 0, 0, &root_x, &root_y, &wdumm);
		XGetGeometry( dpy, pc->w, &wdumm, &dumm, &dumm, &udumm, &udumm, &bw, &udumm );
		root_x -= bw ;
		root_y -= bw ;
		if(root_x != pc->root_x)
			set_flags(changed, CANVAS_X_CHANGED);
		if( root_y != pc->root_y)
			set_flags(changed, CANVAS_Y_CHANGED);
		pc->root_x = root_x;
		pc->root_y = root_y;
		pc->bw = bw ;

		if( !get_drawable_size(pc->w, &width, &height) )
			return 0;/* drawable is bad */
		if( width != pc->width )
			set_flags(changed, CANVAS_WIDTH_CHANGED);
		if( height != pc->height )
			set_flags(changed, CANVAS_HEIGHT_CHANGED);

	clear_flags (pc->state, CANVAS_CONFIG_INVALID);

		if (width != pc->width || height != pc->height)
		{
			destroy_visual_pixmap(ASDefaultVisual, &(pc->saved_canvas));
			destroy_visual_pixmap(ASDefaultVisual, &(pc->canvas));

			if (pc->saved_shape )
				destroy_shape( &(pc->saved_shape) );

			if (pc->shape )
			{
				destroy_shape( &(pc->shape) );
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
		pc->canvas = create_visual_pixmap (ASDefaultVisual, ASDefaultRoot, pc->width, pc->height, 0);
		set_flags (pc->state, CANVAS_DIRTY | CANVAS_OUT_OF_SYNC);
		XSetWindowBackgroundPixmap (dpy, pc->w, pc->canvas);
	}
	return pc->canvas;
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
		LOCAL_DEBUG_CALLER_OUT( "<<#########>>canvas %p created for w: %lX; geom = %dx%d%+d%+d, bw = %d", pc, pc->w, pc->width, pc->height, pc->root_x, pc->root_y, pc->bw );
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
		LOCAL_DEBUG_CALLER_OUT( "<<#########>>container canvas %p created for w: %lX; geom = %dx%d%+d%+d, bw = %d", pc, pc->w, pc->width, pc->height, pc->root_x, pc->root_y , pc->bw);
	}
	return pc;
}

void
destroy_ascanvas (ASCanvas ** pcanvas)
{
	if (pcanvas)
	{
		ASCanvas     *pc = *pcanvas;

		LOCAL_DEBUG_CALLER_OUT( "<<#########>>destroying canvas %p for window %lX", pc, pc?pc->w:None );
		if (pc)
		{
			destroy_visual_pixmap(ASDefaultVisual, &(pc->saved_canvas));
			destroy_visual_pixmap(ASDefaultVisual, &(pc->canvas));
			LOCAL_DEBUG_OUT( "saved_shape = %p, shape = %p", pc->saved_shape, pc->shape ); 
			if (pc->saved_shape && pc->saved_shape != pc->shape)
				destroy_shape( &(pc->saved_shape));
			if (pc->shape)
				destroy_shape( &(pc->shape));
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

void
invalidate_canvas_config( ASCanvas *pc )
{
	if( pc )
	{
		if( get_flags( pc->state, CANVAS_CONFIG_INVALID) )
		return;

		LOCAL_DEBUG_OUT( "resizing to %dx%d", pc->width+1, pc->height+1 );
		XResizeWindow( dpy, pc->w, pc->width+1, pc->height + 1 );
#ifdef SHAPE
		if ( !get_flags( pc->state, CANVAS_CONTAINER )  &&
			  get_flags( pc->state, CANVAS_SHAPE_SET ) )
			set_canvas_shape_to_nothing( pc );
#endif

		pc->width = 0;
		pc->height = 0;
		destroy_visual_pixmap(ASDefaultVisual, &(pc->canvas));
		destroy_visual_pixmap(ASDefaultVisual, &(pc->saved_canvas));
		if (pc->shape)
			destroy_shape( &(pc->shape) );
		if (pc->saved_shape)
			destroy_shape( &(pc->saved_shape) );
		set_flags (pc->state, CANVAS_DIRTY | CANVAS_OUT_OF_SYNC | CANVAS_MASK_OUT_OF_SYNC);
		set_flags( pc->state, CANVAS_CONFIG_INVALID);
	}
}

Bool
get_canvas_position( ASCanvas *pc, Window *pparent, int *px, int *py, unsigned int *pbw )
{
	Window wdumm;
	int dumm;
	unsigned int udumm;

	if( pparent == NULL )
		pparent = &wdumm;
	if( px == NULL )
		px = &dumm;
	if( py == NULL )
		py = &dumm;
	if( pbw == NULL )
		pbw = &udumm;
	if( pc )
		if( XGetGeometry( dpy, pc->w, pparent, px, py, &udumm, &udumm, pbw, &udumm ) )
			return True;
	return False;
}

Bool
make_canvas_rectangle (ASCanvas * pc, ASImage * im, int x, int y, int *cx, int *cy, int *cwidth, int *cheight)
{
	if( pc == NULL || im == NULL || cx == NULL || cy == NULL ||cwidth == NULL || cheight == NULL ) 
		return False;
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
	Bool 		  done = False;

	LOCAL_DEBUG_CALLER_OUT ("pc(%p)->im(%p)->x(%d)->y(%d)", pc, im, x, y);
	if (im == NULL || pc == NULL)
		return False;
	if ((p = get_canvas_canvas (pc)) == None)
		return False;

	if (!make_canvas_rectangle (pc, im, x, y, &real_x, &real_y, &width, &height))
		return False;

	LOCAL_DEBUG_OUT ("drawing image %dx%d at %dx%d%+d%+d", im->width, im->height, width, height, real_x, real_y);
	if( get_flags( ASDefaultVisual->glx_support, ASGLX_UseForImageTx ) )
		done = asimage2drawable_gl( ASDefaultVisual, p, im, 
									real_x - x, real_y - y, real_x, real_y,
									width, height, 
									pc->width, pc->height, 
									False );
	if( !done ) 		
		done = asimage2drawable (ASDefaultVisual, p, im, ASDefaultDrawGC, real_x - x, real_y - y, real_x, real_y, width, height, True);

	if ( done )
	{
		set_flags (pc->state, CANVAS_OUT_OF_SYNC);
		XClearArea( dpy, pc->w, real_x, real_y, width, height, True );
		XSync (dpy, False);
	}
	return done;
}

void
fill_canvas_mask (ASCanvas * pc, int win_x, int win_y, int width, int height)
{
	int           real_x, real_y;
	int           real_width, real_height;

	if( pc == NULL ) 
		return;

	if (pc->shape != None && !get_flags( pc->state, CANVAS_CONTAINER ) )
	{
		real_x = MAX (win_x, 0);
		real_y = MAX (win_y, 0);
		real_width = width - (real_x - win_x);
		real_height = height - (real_y - win_y);
		if (real_width > 0 && real_height > 0 && real_x < pc->width && real_y < pc->height)
		{
			int res ;
			XRectangle rect ;
			rect.x = real_x ;
			rect.y = real_y ;
			rect.width  = real_width ;
			rect.height = real_height ;
			LOCAL_DEBUG_OUT( "filling mask at %dx%d%+d%+d", rect.width, rect.height, rect.x, rect.y );
			res = add_shape_rectangles( pc->shape, &rect, 1, 0, 0, pc->width, pc->height );
			if( res )
				set_flags (pc->state, CANVAS_MASK_OUT_OF_SYNC);
		}
	}
}


Bool
draw_canvas_mask (ASCanvas * pc, ASImage * im, int x, int y)
{
	XRectangle *rects ;
	unsigned int rects_count = 0;
	Bool res = True;
	int           real_x, real_y;
	int           width, height;

	if (im == NULL || pc == NULL)
		return False;

	if (!make_canvas_rectangle (pc, im, x, y, &real_x, &real_y, &width, &height))
		return False;

	rects = get_asimage_channel_rects( im, IC_ALPHA, 10, &rects_count );
	LOCAL_DEBUG_OUT("%d rectangles generated", rects_count );

	if( pc->shape == NULL )
		pc->shape = create_shape();

	if( rects != NULL && rects_count > 0 )
	{	
		res = add_shape_rectangles( pc->shape, rects, rects_count, real_x, real_y, pc->width+pc->bw, pc->height+pc->bw );
		free( rects );
	}

	if( res )
		set_flags (pc->state, CANVAS_MASK_OUT_OF_SYNC);
	return res;
}

inline Bool
get_current_canvas_size( ASCanvas * pc, unsigned int *pwidth, unsigned int *pheight )
{
	if( pc == NULL )
		return False;

	*pwidth = pc->width;
	*pheight = pc->height;
	if( get_flags( pc->state, CANVAS_CONFIG_INVALID) )
		return get_drawable_size( pc->w, pwidth, pheight );
	return True;
}



void
update_canvas_display_mask (ASCanvas * pc, Bool force)
{
#ifdef SHAPE
LOCAL_DEBUG_CALLER_OUT( "canvas(%p)", pc );
	if( pc == NULL )
		return ;
	LOCAL_DEBUG_OUT( "window(%lx)->canvas_pixmap(%lx)->size(%dx%d)", pc->w, pc->canvas, pc->width, pc->height );
	if ( pc->w != None )
	{
		if( force || !get_flags( pc->state, CANVAS_CONTAINER ))
		{
			if (pc->shape  )
			{
				LOCAL_DEBUG_OUT( "XShapeCombineREctangles(%lX)set canvas mask to %d rectangles", pc->w, PVECTOR_USED(pc->shape) );
				if( PVECTOR_USED(pc->shape) > 0 )
					XShapeCombineRectangles (dpy, pc->w, ShapeBounding, 0, 0, PVECTOR_HEAD(XRectangle, pc->shape), PVECTOR_USED(pc->shape), ShapeSet, Unsorted);
				else           /* we are still shaped - but completely opaque */
				{
					XRectangle nothing ;
					nothing.x = pc->width/2 ;
					nothing.y = pc->height/2 ;
					nothing.width = nothing.height = 1 ;
					XShapeCombineRectangles (dpy, pc->w, ShapeBounding, 0, 0, &nothing, 1, ShapeSet, Unsorted);
				}	 
				if( pc->bw > 0 ) 
				{
					XRectangle border[4] ; 
					border[0].x = -pc->bw ; 	
					border[0].y = -pc->bw ; 
					border[0].width = pc->width+pc->bw*2 ; 
					border[0].height = pc->bw ; 
					border[1].x = -pc->bw ; 	   
					border[1].y = -pc->bw ; 
					border[1].width = pc->bw ; 
					border[1].height = pc->height+pc->bw*2 ; 
					border[2].x = -pc->bw ; 	   
					border[2].y = pc->height ; 
					border[2].width = pc->width+pc->bw*2 ; 
					border[2].height = pc->bw ; 
					border[3].x = pc->width ; 	   
					border[3].y = -pc->bw ; 
					border[3].width = pc->bw ; 
					border[3].height = pc->height+pc->bw*2 ; 
					XShapeCombineRectangles (dpy, pc->w, ShapeBounding, 0, 0, &(border[0]), 4, ShapeUnion, Unsorted);					
				}	 
				set_flags( pc->state, CANVAS_SHAPE_SET );
			}else if( get_flags( pc->state, CANVAS_SHAPE_SET ) )
				set_canvas_shape_to_nothing( pc );
			XSync (dpy, False);
			clear_flags (pc->state, CANVAS_MASK_OUT_OF_SYNC );
		}
	}
#endif
}


#ifdef TRACE_update_canvas_display
#undef update_canvas_display
void update_canvas_display (ASCanvas * pc);
void  trace_update_canvas_display (ASCanvas * pc, const char *file, int line)
{
	fprintf (stderr, "D>%s(%d):update_canvas_display(%p)\n", file, line, pc);
	update_canvas_display(pc);
}
#endif


void
update_canvas_display (ASCanvas * pc)
{
	LOCAL_DEBUG_CALLER_OUT( "canvas(%p)", pc );
	if( pc == NULL )
		return ;

	LOCAL_DEBUG_OUT( "window(%lx)->canvas_pixmap(%lx)->size(%dx%d)", pc->w, pc->canvas, pc->width, pc->height );
	if (pc && pc->w != None )
	{
		if( !get_flags( pc->state, CANVAS_CONTAINER ) )
		{
			if (pc->canvas)
			{
#ifdef SHAPE
				update_canvas_display_mask (pc, False);
#endif
				XSetWindowBackgroundPixmap (dpy, pc->w, pc->canvas);
				XClearWindow (dpy, pc->w);

				XSync (dpy, False);
				clear_flags (pc->state, CANVAS_DIRTY | CANVAS_OUT_OF_SYNC | CANVAS_MASK_OUT_OF_SYNC );
			}
		}
	}
}

void
invalidate_canvas_save( ASCanvas *pc )
{
	if( pc )
	{
		destroy_visual_pixmap( ASDefaultVisual, &(pc->saved_canvas) );
		destroy_shape( &(pc->saved_shape) );
	}
}


Bool
restore_canvas( ASCanvas *pc )
{
	if( pc )
	{
		if( pc->saved_canvas == None )
			return False ;

		destroy_visual_pixmap( ASDefaultVisual, &(pc->canvas) );
		pc->canvas = pc->saved_canvas ;
		pc->saved_canvas = None ;


		destroy_shape( &(pc->shape) );
		pc->shape = pc->saved_shape ;
		pc->saved_shape = NULL ;

		update_canvas_display( pc );
		return True;
	}
	return False ;
}

Bool
save_canvas( ASCanvas *pc )
{
	if( pc )
	{
		destroy_visual_pixmap( ASDefaultVisual, &(pc->saved_canvas) );
		pc->saved_canvas = pc->canvas ;
		pc->canvas = None ;

		destroy_shape( &(pc->saved_shape) );
		pc->saved_shape = pc->shape ;
		pc->shape = NULL ;

		set_flags (pc->state, CANVAS_MASK_OUT_OF_SYNC|CANVAS_OUT_OF_SYNC);
		return (pc->saved_canvas != None) ;
	}
	return False;
}

Bool
swap_save_canvas( ASCanvas *pc )
{
	Pixmap 		tmp_canvas ;
	struct ASVector 	*tmp_shape ;
	
	if( pc )
	{
		tmp_canvas = pc->saved_canvas ;
		pc->saved_canvas = pc->canvas ;
		pc->canvas = tmp_canvas ;

		tmp_shape = pc->saved_shape ;
		pc->saved_shape = pc->shape ;
		pc->shape = tmp_shape ;

		set_flags (pc->state, CANVAS_MASK_OUT_OF_SYNC|CANVAS_OUT_OF_SYNC);
		return (pc->canvas != None) ;
	}
	return False;
	
}


void
clear_canvas_shape (ASCanvas * pc, Bool force_for_container)
{
	LOCAL_DEBUG_CALLER_OUT( "canvas(%p)", pc );
	if( pc == NULL )
		return ;

	LOCAL_DEBUG_OUT( "window(%lx)->canvas_pixmap(%lx)->size(%dx%d)", pc->w, pc->canvas, pc->width, pc->height );
	if ( pc->w != None )
	{
		if( !get_flags( pc->state, CANVAS_CONTAINER ) || force_for_container )
		{
#ifdef SHAPE
			if (pc->canvas && get_flags( pc->state, CANVAS_SHAPE_SET ) )
				set_canvas_shape_to_rectangle( pc );  /* just in case  */
			set_canvas_shape_to_nothing( pc );
#endif
			if( pc->shape )
				destroy_shape( &pc->shape );
			set_flags (pc->state, CANVAS_DIRTY | CANVAS_OUT_OF_SYNC);
		}
	}
}

Bool
check_canvas_shaped( ASCanvas *pc)
{
	Bool           boundingShaped= False;
#ifdef SHAPE
	if( pc ) 
	{	
		int           dumm;
		unsigned      udumm;
		XShapeQueryExtents (dpy, pc->w,
							&boundingShaped, &dumm, &dumm, &udumm, &udumm, &dumm, &dumm, &dumm, &udumm, &udumm);
	}
#endif
	return boundingShaped ;
}

Bool
refresh_container_shape( ASCanvas *pc )
{
	int res = False;
	if( pc && get_flags( pc->state, CANVAS_CONTAINER ) )
	{
		int count, order ;
		XRectangle *rects;
#ifdef SHAPE
		rects = XShapeGetRectangles( dpy, pc->w, ShapeBounding, &count, &order );
#endif
		if( rects )
		{
			unsigned int curr_width = 1, curr_height = 1 ;
			if( pc->shape == NULL )
				pc->shape = create_shape();
			else
				flush_vector( pc->shape );
			get_drawable_size( pc->w, &curr_width, &curr_height );
			res = add_shape_rectangles( pc->shape, rects, count, 0, 0, curr_width+pc->bw*2, curr_height+pc->bw*2 );
			XFree( rects );

			if( res )
				set_flags (pc->state, CANVAS_MASK_OUT_OF_SYNC);
		}else if( pc->shape )
		{
			destroy_shape( &(pc->shape) );
			set_flags (pc->state, CANVAS_MASK_OUT_OF_SYNC);
			res = True ;
		}
	}
	return res;
}


Bool
combine_canvas_shape_at_geom (ASCanvas *parent, ASCanvas *child, int child_x, int child_y, int child_width, int child_height, int child_bw )
{
	int res = False ;
#ifdef SHAPE
LOCAL_DEBUG_OUT( "parent(%p),child(%p)", parent, child );
	if (parent && child )
	{
		unsigned int parent_width, parent_height ;
		LOCAL_DEBUG_OUT( "parent->shape(%p)", parent->shape );
		if( parent->shape == NULL )
			return False;

		LOCAL_DEBUG_OUT( "child->shape(%p)", child->shape );
		if( child->shape!= NULL && PVECTOR_USED(child->shape) == 0 )
		{
			LOCAL_DEBUG_OUT( "child->shape has no rectangles%s", "" );	  
			return False ;     /* child has an empty shape */
		}

#ifdef STRICT_GEOMETRY
		get_current_canvas_size( parent, &parent_width, &parent_height );
#else
		parent_width = parent->width ;
		parent_height = parent->height ;
#endif

		if( child_x > parent_width || child_y > parent_height ||
			child_x + child_width <= 0 || child_y + child_height <= 0 )
		{
#ifdef LOCAL_DEBUG
			if( get_flags( child->state, CANVAS_CONTAINER ) )
				LOCAL_DEBUG_OUT( "container shape ignored - out of bounds: %dx%d%+d%+d parents: %dx%d%+d%+d",
								 child_width, child_height, child_x, child_y,
								 parent_width, parent_height, parent->root_x, parent->root_y );
#endif
			return False;
		}

#if 1
		if( child->shape && PVECTOR_USED(child->shape) > 0 )
		{
			LOCAL_DEBUG_OUT( "adding %d child's shape rectangles", PVECTOR_USED(child->shape) );
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
			{
				int i, max_i = PVECTOR_USED(child->shape) ;
				XRectangle *r = PVECTOR_HEAD(XRectangle, child->shape);
				for ( i = 0 ; i < max_i ; ++i  )
					fprintf( stderr, "\t\t r[%d] = %dx%d%+d%+d\n", i, r[i].width, r[i].height, r[i].x, r[i].y );
			}
			LOCAL_DEBUG_OUT( "child geom = %dx%d%+d%+d, bw = %d", child_width, child_height, child_x, child_y, child_bw );			   
#endif
			res = add_shape_rectangles( parent->shape, PVECTOR_HEAD(XRectangle, child->shape), PVECTOR_USED(child->shape), child_x+child_bw, child_y+child_bw, parent_width, parent_height );
			if( res && child_bw > 0 ) 
			{
				XRectangle border[4] ;		
				border[0].x = 0 ; 
				border[0].y = 0 ; 
				border[0].width = child_width+child_bw ; 
				border[0].height = child_bw ; 
				border[1].x = 0 ; 
				border[1].y = child_bw ; 
				border[1].width = child_bw ; 
				border[1].height = child_height ; 
				border[2].x = child_width+child_bw ; 
				border[2].y = 0 ; 
				border[2].width = child_bw ; 
				border[2].height = child_height+child_bw*2 ; 
				border[3].x = 0 ; 
				border[3].y = child_height+child_bw ; 
				border[3].width = child_width+child_bw*2 ; 
				border[3].height = child_bw ; 
				res = add_shape_rectangles( parent->shape, &(border[0]), 4, child_x, child_y, parent_width, parent_height );
			}				   
		}else
#endif			
		{
			XRectangle rect ;
			rect.x = child_x ;
			rect.y = child_y ;
			rect.width = child_width + child_bw*2 ;
			rect.height = child_height + child_bw*2 ;
			LOCAL_DEBUG_OUT( "adding child's shape as whole rectangle: %dx%d%+d%+d", rect.width, rect.height, rect.x, rect.y );

			res = add_shape_rectangles( parent->shape, &rect, 1, 0, 0, parent_width+parent->bw*2, parent_height+parent->bw*2 );
		}
	}
#endif
	return res;
}

Bool
combine_canvas_shape (ASCanvas *parent, ASCanvas *child )
{
	int res = False ;
#ifdef SHAPE
	if (parent && child )
	{
		int child_x = 0 ;
		int child_y = 0 ;
		unsigned int width, height, bw;

		if( parent->shape == NULL )
			return False;
#ifndef STRICT_GEOMETRY
		get_current_canvas_geometry( child, &child_x, &child_y, &width, &height, &bw );
#else
		child_x = child->root_x - parent->root_x ;
		child_y = child->root_y - parent->root_y ;
		width = child->width ;
		height = child->height ;
		bw = child->bw ;
#endif
		res = combine_canvas_shape_at_geom (parent, child, child_x, child_y, width, height, bw );
	}
#endif
	return res;
}


Bool
combine_canvas_shape_at (ASCanvas *parent, ASCanvas *child, int child_x, int child_y )
{
#ifdef SHAPE
	if( child )
	{
		unsigned int width, height, bw;
#ifndef STRICT_GEOMETRY
		get_current_canvas_geometry( child, NULL, NULL, &width, &height, &bw );
#else
		width = child->width ;
		height = child->height ;
		bw = child->bw ;
#endif

		return combine_canvas_shape_at_geom ( parent, child, child_x, child_y, width, height, bw );
	}
#endif
	return False;
}

Bool
is_canvas_needs_redraw( ASCanvas *pc )
{
	return pc?get_flags (pc->state, CANVAS_DIRTY):False;
}

Bool
is_canvas_dirty( ASCanvas *pc )
{
	return pc?get_flags (pc->state, CANVAS_DIRTY|CANVAS_OUT_OF_SYNC|CANVAS_MASK_OUT_OF_SYNC):False;
}


ASFlagType
configure_canvas (ASCanvas * pc, int x, int y, unsigned int width, unsigned int height, ASFlagType mask )
{
	ASFlagType changes = 0 ;
	XWindowChanges xwc ; 
	int curr_x, curr_y;
	unsigned int curr_width, curr_height, curr_bw ;
	
	if( pc == NULL ) 
		return 0;

	get_current_canvas_geometry( pc, &curr_x, &curr_y, &curr_width, &curr_height, &curr_bw );
	
	LOCAL_DEBUG_CALLER_OUT( "canvas(%p)->window(%lx)->geom(%ux%u%+d%+d)", pc, pc->w, width, height, x, y );
	/* Setting background to None to avoid background pixmap tiling
	 * while resizing */
	if( !get_flags( mask, CWWidth ) )
		width = pc->width ; 
	else if( width>MAX_POSITION )
	{
#ifdef DEBUG_ALLOCS
		AS_ASSERT(0);
#endif
		width = pc->width ;
	}else if( AS_ASSERT(width))
		width = 1 ;

	if( !get_flags( mask, CWHeight ) )
		height = pc->height ; 
	else if( height>MAX_POSITION )
	{
#ifdef DEBUG_ALLOCS
		AS_ASSERT(0);
#endif
		height = pc->height ;
	}else if( AS_ASSERT(height))
		height = 1;

	if( get_flags( mask, CWX ) && curr_x != x )
		set_flags( changes, CANVAS_X_CHANGED );
	if( get_flags( mask, CWY ) && curr_y != y )
		set_flags( changes, CANVAS_Y_CHANGED );
	if( width != curr_width ) set_flags( changes, CANVAS_WIDTH_CHANGED );
	if( height != curr_height ) set_flags( changes, CANVAS_HEIGHT_CHANGED );

	if( changes == 0 ) 
		return 0;
	xwc.x = x ; 
	xwc.y = y ; 
	xwc.width = width ; 
	xwc.height = height ; 
	
	if ((pc->width < width || pc->height < height) && !get_flags( pc->state, CANVAS_CONTAINER ))
		XSetWindowBackgroundPixmap (dpy, pc->w, None);
	
	LOCAL_DEBUG_OUT( "XConfigureWindow( %lX, %lX, %dx%d%+d%+d );", pc->w, mask, width, height, x, y );
	XConfigureWindow (dpy, pc->w, mask, &xwc);
/*fprintf( stderr, "client_changes: ( %lX, %lX, %dx%d%+d%+d ); was %dx%d%+d%+d\n", pc->w, mask, width, height, x, y, curr_width, curr_height, curr_x, curr_y );*/
	return changes;
}

ASFlagType 
resize_canvas (ASCanvas * pc, unsigned int width, unsigned int height)
{
	if( pc == NULL ) 
		return 0;

	return configure_canvas (pc, 0, 0, width, height, CWWidth|CWHeight );
}



ASFlagType
moveresize_canvas (ASCanvas * pc, int x, int y, unsigned int width, unsigned int height)
{
	if( pc == NULL ) 
		return 0;
	return configure_canvas (pc, x, y, width, height, CWX|CWY|CWWidth|CWHeight );
}


void
move_canvas (ASCanvas * pc, int x, int y)
{
	if( pc == NULL ) 
		return;

LOCAL_DEBUG_CALLER_OUT( "canvas(%p)->window(%lx)->geom(%+d%+d)", pc, pc->w, x, y );
	XMoveWindow (dpy, pc->w, x, y);
}


void
unmap_canvas_window( ASCanvas *pc )
{
	if( pc && pc->w != None )
	{	
		XUnmapWindow( dpy, pc->w );
		clear_flags( pc->state, CANVAS_MAPPED );
	}
}

void
map_canvas_window( ASCanvas *pc, Bool raised )
{
	LOCAL_DEBUG_CALLER_OUT( "pc=%p", pc );
	if( pc == NULL ) 
		return;

	LOCAL_DEBUG_OUT( "raised = %s", raised ? "True":"False" );
	if( pc->w != None )
	{
		if( raised )
			XMapRaised( dpy, pc->w );
		else
			XMapWindow( dpy, pc->w );
		set_flags( pc->state, CANVAS_MAPPED );
	}
}

void
quietly_reparent_canvas( ASCanvas *pc, Window dst, long event_mask, Bool use_root_pos, Window below )
{
	if( pc )
	{
		int x = 0, y = 0 ;
		unsigned int bw = pc->bw;
		Window parent = None ;
		
		if( dst == None ) 
			dst = ASDefaultRoot ;            

		if( use_root_pos )
		{
			x = pc->root_x ;
			y = pc->root_y ;
		}else
			get_canvas_position( pc, &parent, &x, &y, &bw );

		if( parent != dst )
		{
			/* blocking UnmapNotify events since that may bring us into Withdrawn state */
			XSelectInput (dpy, pc->w, event_mask & ~StructureNotifyMask);
			LOCAL_DEBUG_OUT( "XReparentWindow( %lX, %lX, %+d%+d ), bw = %d", pc->w, dst, x, y, bw );
			if( below != None && get_flags( pc->state, CANVAS_MAPPED )) 
			{
				Window w[2] ;
				XUnmapWindow( dpy, pc->w );
				XReparentWindow( dpy, pc->w, (dst!=None)?dst:ASDefaultRoot, x, y );
				w[0] = below ; 
				w[1] = pc->w ; 
				XRestackWindows( dpy, w, 2 );
				XMapWindow( dpy, pc->w );
			}else		  
				XReparentWindow( dpy, pc->w, (dst!=None)?dst:ASDefaultRoot, x, y );
			XSelectInput (dpy, pc->w, event_mask );
		}
	}
}

void
reparent_canvas_window( ASCanvas *pc, Window dst, int x, int y )
{
	if( pc )
	{
		if( dst == None ) 
			dst = ASDefaultRoot ;            
		LOCAL_DEBUG_OUT( "XReparentWindow( %lX, %lX, +0+0 )", pc->w, dst );
		XReparentWindow( dpy, pc->w, dst, 0, 0 );
	}
}

void
add_canvas_grid( ASGrid *grid, ASCanvas *canvas, int outer_gravity, int inner_gravity, int vx, int vy )
{
	if( canvas && grid )
	{
		int x = canvas->root_x + vx ;
		int y = canvas->root_y + vy ;
LOCAL_DEBUG_CALLER_OUT( "(%p,%ux%u%+d%+d)", canvas, canvas->width, canvas->height, canvas->root_x, canvas->root_y );
		add_gridline( &(grid->h_lines), y,                x, x+canvas->width+canvas->bw*2,  outer_gravity, inner_gravity );
		add_gridline( &(grid->h_lines), y+canvas->height+canvas->bw*2, x, x+canvas->width+canvas->bw*2,  inner_gravity, outer_gravity );
		add_gridline( &(grid->v_lines), x,                y, y+canvas->height+canvas->bw*2, outer_gravity, inner_gravity );
		add_gridline( &(grid->v_lines), x+canvas->width+canvas->bw*2,  y, y+canvas->height+canvas->bw*2, inner_gravity, outer_gravity );
	}
}

void
set_root_clip_area( ASCanvas *canvas )
{
	if( canvas )
	{	
		ASDefaultScr->RootClipArea.x = canvas->root_x+(int)canvas->bw;
		ASDefaultScr->RootClipArea.y = canvas->root_y+(int)canvas->bw;
		ASDefaultScr->RootClipArea.width  = canvas->width;
		ASDefaultScr->RootClipArea.height = canvas->height;
		if( ASDefaultScr->RootImage )
		{
			safe_asimage_destroy( ASDefaultScr->RootImage );
			ASDefaultScr->RootImage = NULL ;
		}
	}
}

/*************************************************************************/

void
send_canvas_configure_notify(ASCanvas *parent, ASCanvas *canvas)
{
LOCAL_DEBUG_CALLER_OUT( "%p,%p", parent, canvas );
	if( canvas )
	{
		XEvent client_event ;
		unsigned int uwidth, uheight, ubw ;

		client_event.type = ConfigureNotify;
		client_event.xconfigure.display = dpy;
		client_event.xconfigure.event = canvas->w;
		client_event.xconfigure.window = canvas->w;

		get_current_canvas_geometry( canvas, &(client_event.xconfigure.x),
										 &(client_event.xconfigure.y),
										 &uwidth, &uheight, &ubw );
		client_event.xconfigure.width = uwidth ;
		client_event.xconfigure.height = uheight;
		client_event.xconfigure.border_width = ubw;

		if( parent )
		{
#if 1
			Window wdumm;
			XTranslateCoordinates (dpy, canvas->w, ASDefaultRoot, 0, 0, &(client_event.xconfigure.x),
										 &(client_event.xconfigure.y), &wdumm);
#else /* we maybe called before parent's geometry is updated - so don't trust it! */
			client_event.xconfigure.x += parent->root_x+(int)parent->bw;
			client_event.xconfigure.y += parent->root_y+(int)parent->bw;
#endif			
			/* Real ConfigureNotify events say we're above title window, so ... */
			/* what if we don't have a title ????? */
			client_event.xconfigure.above = parent->w;
		}else
			client_event.xconfigure.above = ASDefaultRoot;
		client_event.xconfigure.override_redirect = False;
		LOCAL_DEBUG_OUT( "geom= %dx%d%+d%+d", client_event.xconfigure.width, client_event.xconfigure.height, client_event.xconfigure.x, client_event.xconfigure.y );
		XSendEvent (dpy, canvas->w, False, StructureNotifyMask, &client_event);
	}
}

