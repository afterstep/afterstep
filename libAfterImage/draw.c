/*
 * Copyright (c) 2004 Sasha Vasko <sasha at aftercode.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define LOCAL_DEBUG

#ifdef _WIN32
#include "win32/config.h"
#else
#include "config.h"
#endif

/*#define LOCAL_DEBUG*/
/*#define DO_CLOCKING*/

#include <ctype.h>
#ifdef _WIN32
# include "win32/afterbase.h"
#else
# include "afterbase.h"
#endif
#include "asvisual.h"
#include "asimage.h"

typedef struct ASDrawTool
{
	int width;
	int height;
	int center_x, center_y ;
	CARD8  *matrix ;
}ASDrawTool;

typedef struct ASDrawContext
{
	ASDrawTool *tool ;
	
	int canvas_width, canvas_height ;
	CARD8 *canvas ;

	int curr_x, curr_y ;

	void (*apply_tool_func)( struct ASDrawContext *ctx, int curr_x, int curr_y, CARD8 ratio );
}ASDrawContext;

#define AS_DRAW_BRUSHES	3

CARD8 _round1x1[1] =
{  255 };

CARD8 _round3x3[9] =
{ 64,  80,  64,
  80, 255,  80,
  64,  80,  64 };


CARD8 _round5x5[25] =
{  0,  24,  64,  24,   0,
  24,  64,  80,  64,  24,
  64,  80, 255,  80,  64,
  24,  64,  80,  64,  24,
   0,  24,  64,  24,   0,};

CARD8 _round5x5_solid[25] =
{  0,  255,  255,  255,   0,
  255,  255, 255,  255,  255,
  255,  255, 255,  255,  255,
  255,  255, 255,  255,  255,
   0,  255,  255,  255,   0,};

ASDrawTool StandardBrushes[AS_DRAW_BRUSHES] = 
{
	{1, 1, 0, 0, _round1x1},
	{3, 3, 1, 1, _round3x3},
	{5, 5, 2, 2, _round5x5_solid}
};	 

/*********************************************************************************/
/* auxilary functions : 											 */
/*********************************************************************************/

static void
apply_tool_2D( ASDrawContext *ctx, int curr_x, int curr_y, CARD8 ratio )
{
	if( ratio  !=  0 ) 
	{	
		CARD8 *src = ctx->tool->matrix ;
		int corner_x = curr_x - ctx->tool->center_x ; 
		int corner_y = curr_y - ctx->tool->center_y ; 
		int tw = ctx->tool->width ;
		int th = ctx->tool->height ;
		int cw = ctx->canvas_width ;
		int ch = ctx->canvas_height ;
		int aw = tw ; 
		int ah = th ;
		CARD8 *dst = ctx->canvas ; 
		int x, y ;

		if( corner_x+tw < 0 || corner_x >= cw || corner_y+th < 0 || corner_y >= ch ) 
			return ;
		 
		if( corner_y > 0 ) 
			dst += corner_y * cw ;
		else if( corner_y < 0 ) 
		{
			ah -= -corner_y ;
			src += -corner_y * tw ;
		}

		if( corner_x  > 0 ) 
			dst += corner_x ;  
		else if( corner_x < 0 )
		{	
			src += -corner_x ; 
			aw -= -corner_x ;
		}
	
		if( corner_x + tw > cw ) 
			aw = cw - corner_x;

		if( corner_y + th > ch ) 
			ah = ch - corner_y;
		
		if( ratio == 255 ) 
		{
			for( y = 0 ; y < ah ; ++y ) 
			{	
				for( x = 0 ; x < aw ; ++x ) 
				{
					if( dst[x] < src[x] ) 
						dst[x] = src[x] ;
					/*
					register CARD32 t = (CARD32)dst[x] + (CARD32)src[x] ;
					dst[x] = t > 255? 255 : t ;
			 		*/
				}
				src += tw ; 
				dst += cw ; 
			}
		}else if( ratio != 0 )
		{		 
			for( y = 0 ; y < ah ; ++y ) 
			{	
				for( x = 0 ; x < aw ; ++x ) 
				{
					CARD32 v = ratio ;
					v = (v*src[x])>>8 ;
					if( dst[x] < v ) 
						dst[x] = v ;
					/*
					register CARD32 t = (CARD32)dst[x] + (CARD32)src[x] ;
					dst[x] = t > 255? 255 : t ;
			 		*/
				}
				src += tw ; 
				dst += cw ; 
			}
		}
	}
}	   


/*********************************************************************************/
/* drawing functions : 											 */
/*********************************************************************************/
static void
ctx_draw_line_solid( ASDrawContext *ctx, int from_x, int from_y, int to_x, int to_y )
{
	int x, y, end, dir = 1;
	int dx = to_x - from_x ; 
	int dy = to_y - from_y ; 
	if( dx < 0 ) 
		dx = -dx ;
	if( dy < 0 ) 
		dy = -dy ;
	
	if( dx >= dy ) 
	{	
      	int Dy = -dx + 2*dy;
      	int inct = 2*dy;
		int incf = 2*(dy - dx);
		
		if( to_y > from_y ) 
		{	x = from_x ; y = from_y ; end = to_x ; }
		else
		{	x = to_x ; y = to_y ; end = from_x ; 	}
		
		if( end < x ) 
			dir = -1 ;				 
      	
		ctx->apply_tool_func( ctx, x, y, 255 );
      	while(x != end)
		{
        	x += dir;
        	if(Dy > 0)
			{
          		Dy += incf;
          		++y;
        	}else 
				Dy += inct;
        	ctx->apply_tool_func( ctx, x, y, 255 );
      	}
	}else
	{
      	int Dx = -dy + 2*dx;
      	int inct = 2*dx;
		int incf = 2*(dx - dy);
		
		if( to_x > from_x ) 
		{	y = from_y ; x = from_x ; end = to_y ; }
		else
		{	y = to_y ; x = to_x ; end = from_y ; 	}
		
		if( end < y ) 
			dir = -1 ;				 
      	
		ctx->apply_tool_func( ctx, x, y, 255 );
      	while(y != end)
		{
        	y += dir;
        	if(Dx > 0)
			{
          		Dx += incf;
          		++x;
        	}else 
				Dx += inct;
        	ctx->apply_tool_func( ctx, x, y, 255 );
      	}
	}		 
}	 

static void
ctx_draw_line_solid_aa( ASDrawContext *ctx, int from_x, int from_y, int to_x, int to_y )
{
	int x, y, end, dir = 1;
	int dx = to_x - from_x ; 
	int dy = to_y - from_y ; 
	if( dx < 0 ) 
		dx = -dx ;
	if( dy < 0 ) 
		dy = -dy ;

	if( dx == 0 || dy == 0 ) 
		return ctx_draw_line_solid( ctx, from_x, from_y, to_x, to_y );
	
	if( dx >= dy ) 
	{	
		int ratio2 = 0x007FFFFF/dx ; 
		CARD32 value = 0x003FFFFF; 
		CARD32 value_incr = ratio2*dy ; 
		int value_decr = (dx - dy)*ratio2 ;

		
		if( to_y > from_y ) 
		{	x = from_x ; y = from_y ; end = to_x ; }
		else
		{	x = to_x ; y = to_y ; end = from_x ; 	}
		
		if( end < x ) 	dir = -1 ;				 
      	
		ctx->apply_tool_func( ctx, x, y, 255 );
/*		LOCAL_DEBUG_OUT( "x = %d, y = %d, dir = %d, value = %d, incr = %d, decr = %d, ratio = %d, value_incr = %d", 
						 x, y, dir, value, incr, decr, ratio, value_incr );
  */			
      	while(x != end)
		{
        	x += dir;
        	if( (int)value > value_decr)
			{
				value = (int)value - value_decr ;
          		++y;
        	}else 
				value += value_incr ; 

			{
				register int above = (value&0x00FF0000)>>16;
				/* LOCAL_DEBUG_OUT( "x = %d, y = %d, value = %d, above = %d",  x, y, value, above ); */
				switch( (above>>5)&0x03 )
				{
					case 0 :  /* 0 - 32 */ 
						above = 128 - above ;
						ctx->apply_tool_func( ctx, x, y-1, above ) ;
						ctx->apply_tool_func( ctx, x, y, ~(above>>1) ) ; 
						break ;	  
					case 1 :  /* 32 - 64 */ 
						{
							int a1 = (above - 32) ;
							ctx->apply_tool_func( ctx, x, y+1, a1 ) ;
							above = (~above)&0x7f ;
							ctx->apply_tool_func( ctx, x, y-1, above - a1 ) ;
							ctx->apply_tool_func( ctx, x, y, 255 ) ; 
						}
						break ;
					case 2 :  /* 64 - 96 */  
						{
							int a1 = (96 - above) ;	  
							ctx->apply_tool_func( ctx, x, y-1, a1 ) ;
							ctx->apply_tool_func( ctx, x, y, 255 ) ; 
							ctx->apply_tool_func( ctx, x, y+1, above - a1 ) ;
						}
						break ;
					case 3 :  /* 96 - 128 */ 
						{
							above -= ((~above)&0x7f)>>1 ;	  
							ctx->apply_tool_func( ctx, x, y, ~(above>>1) ) ; 
							ctx->apply_tool_func( ctx, x, y+1, above ) ;
						}
						break ;
				}
			}
      	}
	}else
	{
		int ratio2 = 0x007FFFFF/dy ; 
		CARD32 value = 0x003FFFFF; 
		CARD32 value_incr = ratio2*dx ; 
		int value_decr = (dy - dx)*ratio2 ;

		
		if( to_x > from_x ) 
		{	y = from_y ; x = from_x ; end = to_y ; }
		else
		{	y = to_y ; x = to_x ; end = from_y ; 	}
		
		if( end < y ) 	dir = -1 ;				 
      	
		ctx->apply_tool_func( ctx, x, y, 255 );
/*		LOCAL_DEBUG_OUT( "x = %d, y = %d, dir = %d, value = %d, incr = %d, decr = %d, ratio = %d, value_incr = %d", 
						 x, y, dir, value, incr, decr, ratio, value_incr );
  */			
      	while(y != end)
		{
        	y += dir;
        	if( (int)value > value_decr)
			{
				value = (int)value - value_decr ;
          		++x;
        	}else 
				value += value_incr ; 

			{
				register int above = (value&0x00FF0000)>>16;
				/* LOCAL_DEBUG_OUT( "x = %d, y = %d, value = %d, above = %d",  x, y, value, above ); */
				switch( (above>>5)&0x03 )
				{
					case 0 :  /* 0 - 32 */ 
						above = 128 - above ;
						ctx->apply_tool_func( ctx, x-1, y, above ) ;
						ctx->apply_tool_func( ctx, x, y, ~(above>>1) ) ; 
						break ;	  
					case 1 :  /* 32 - 64 */ 
						{
							int a1 = (above - 32) ;
							ctx->apply_tool_func( ctx, x+1, y, a1 ) ;
							above = (~above)&0x7f ;
							ctx->apply_tool_func( ctx, x-1, y, above - a1 ) ;
							ctx->apply_tool_func( ctx, x, y, 255 ) ; 
						}
						break ;
					case 2 :  /* 64 - 96 */  
						{
							int a1 = (96 - above) ;	  
							ctx->apply_tool_func( ctx, x-1, y, a1 ) ;
							ctx->apply_tool_func( ctx, x, y, 255 ) ; 
							ctx->apply_tool_func( ctx, x+1, y, above - a1 ) ;
						}
						break ;
					case 3 :  /* 96 - 128 */ 
						{
							above -= ((~above)&0x7f)>>1 ;	  
							ctx->apply_tool_func( ctx, x, y, ~(above>>1) ) ; 
							ctx->apply_tool_func( ctx, x+1, y, above ) ;
						}
						break ;
				}
			}
      	}
	}		 
}	 

/*************************************************************************
 * Clip functions 
 *************************************************************************/
Bool 
clip_line( int k, int x0, int y0, int cw, int ch, int *x, int*y ) 	
{
	int new_x = *x ; 
	int new_y = *y ;

	if( new_x < 0 ) 
	{
		new_x = 0 ;
		new_y = (-x0 / k) + y0 ;
	}	 
	if( new_y < 0 ) 
	{
		new_y = 0 ;
		new_x = -y0 * k + x0 ; 
	}	 
	if( new_x < 0 ) 
		return False;

	/* here both new_x and new_y are non-negative */
	if( new_x >= cw ) 
	{
		new_x = cw - 1 ;
		new_y = (new_x - x0)/k  + y0 ; 
		if( new_y < 0 ) 
			return False;	   
	}		 
	if( new_y >= ch ) 
	{
		new_y = ch - 1 ;
		new_x = (new_y - y0) * k  + x0 ; 
		if( new_x < 0 || new_x >= cw ) 
			return False;	   
	}		 
	*x = new_x ; 
	*y = new_y ;
	return True;
}

/*********************************************************************************/
/* context functions : 											 				 */
/*********************************************************************************/
ASDrawContext *
create_draw_context( unsigned int width, unsigned int height )
{
	ASDrawContext *ctx = safecalloc( 1, sizeof(ASDrawContext));
	
	ctx->canvas_width = width == 0 ? 1 : width ; 
	ctx->canvas_height = height == 0 ? 1 : height ;
	ctx->canvas = safecalloc(  ctx->canvas_width*ctx->canvas_height, sizeof(CARD8));

	ctx->tool = &(StandardBrushes[0]) ;  
	ctx->apply_tool_func = apply_tool_2D ; 
				   
	return ctx;
}	   

Bool
asim_set_brush( ASDrawContext *ctx, int brush ) 
{
	if( brush >= 0 && brush < AS_DRAW_BRUSHES && ctx != NULL ) 
	{
		ctx->tool = &(StandardBrushes[brush]) ;  
		ctx->apply_tool_func = apply_tool_2D ; 
		return True;
	}	 
	return False;
}

void
asim_move_to( ASDrawContext *ctx, int dst_x, int dst_y )
{
	if( ctx ) 
	{
		ctx->curr_x = dst_x ; 	
		ctx->curr_y = dst_y ; 
	}		 
}

void
asim_line_to_generic( ASDrawContext *ctx, int dst_x, int dst_y, void (*func)(ASDrawContext*,int,int,int,int))
{
	if( ctx ) 
	{
		int from_x = ctx->curr_x ; 	
		int from_y = ctx->curr_y ; 
		int to_x = dst_x ; 	
		int to_y = dst_y ;
		int cw = ctx->canvas_width ; 
		int ch = ctx->canvas_height ; 
		
		asim_move_to( ctx, dst_x, dst_y );	 				   

		if( to_y == from_y ) 
		{
			if( to_y < 0 || to_y >= ch ) 
				return ; 
			if( from_x < 0 ) 
				from_x = 0 ; 
			else if( from_x >= cw ) 
				from_x = cw - 1 ; 		  
			if( to_x < 0 ) 
				to_x = 0 ; 
			else if( to_x >= cw ) 
				to_x = cw - 1 ; 		  
		}else if( to_x == from_x ) 
		{
			if( to_x < 0 || to_x >= ch ) 
				return ; 
			if( from_y < 0 ) 
				from_y = 0 ; 
			else if( from_y >= ch ) 
				from_y = ch - 1 ; 		  
			if( to_y < 0 ) 
				to_y = 0 ; 
			else if( to_y >= ch ) 
				to_y = ch - 1 ; 		  
		}else
		{			 
			int k = (to_x - from_x)/(to_y - from_y);
			int x0 = from_x ; 
			int y0 = from_y ;

			if( (from_x < 0 && to_x < 0 ) || (from_y < 0 && to_y < 0 ) ||
				(from_x >= cw && to_x >= cw ) || (from_y >= ch && to_y >= ch ))
				return ;
			
			if( !clip_line( k, x0, y0, cw, ch, &from_x, &from_y ) ) 
				return ; 	 
			if( !clip_line( k, x0, y0, cw, ch, &to_x, &to_y ) ) 
				return ; 	 
		}			
		if( from_x != to_x || from_y != to_y ) 
			func( ctx, from_x, from_y, to_x, to_y );
	}	 
} 
	   
void
asim_line_to( ASDrawContext *ctx, int dst_x, int dst_y ) 
{
	asim_line_to_generic( ctx, dst_x, dst_y, ctx_draw_line_solid);	
}	 


void
asim_line_to_aa( ASDrawContext *ctx, int dst_x, int dst_y ) 
{
	asim_line_to_generic( ctx, dst_x, dst_y, ctx_draw_line_solid_aa);	 		
}	 


Bool
apply_draw_context( ASImage *im, ASDrawContext *ctx, ASFlagType filter ) 
{
	int chan ;
	int width, height ;
	if( im == NULL || ctx == NULL || filter == 0 )
		return False;
	
	width = im->width ;
	height = im->height ;
	if( width != ctx->canvas_width || height != ctx->canvas_height )
		return False;
	
	for( chan = 0 ; chan < IC_NUM_CHANNELS;  chan++ )
		if( get_flags( filter, 0x01<<chan) )
		{
			int y;
			register ASStorageID *rows = im->channels[chan] ;
			register CARD8 *canvas_row = ctx->canvas ; 
			for( y = 0 ; y < height ; ++y )
			{	
				if( rows[y] ) 
					forget_data( NULL, rows[y] ); 
				rows[y] = store_data( NULL, (CARD8*)canvas_row, width, ASStorage_RLEDiffCompress, 0);
				canvas_row += width ; 
			}
		}
	return True;
}	   


/*********************************************************************************/
/* The end !!!! 																 */
/*********************************************************************************/
/*********************************************************************************/
/* Test container : 																 */
/*********************************************************************************/

#ifdef TEST_ASDRAW
#include "afterimage.h"

int main(int argc, char **argv )
{
	ASVisual *asv ;
	ASImage *back = NULL ;
	ASImage *drawing1 = NULL ;
	ASImage *merged_im = NULL ;
	ASImageLayer *layers ;
	int layers_num = 0, i;
	int screen = 0, depth = 0;

	ASDrawContext *ctx ;

#define DRAW_TEST_SIZE 512	
	
	set_output_threshold( 10 );

#ifndef X_DISPLAY_MISSING
	dpy = XOpenDisplay(NULL);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( dpy, screen );
#endif

	asv = create_asvisual( dpy, screen, depth, NULL );


	back = create_asimage( DRAW_TEST_SIZE, DRAW_TEST_SIZE, 100 );
	back->back_color = ARGB32_White ;
	drawing1 = create_asimage( DRAW_TEST_SIZE, DRAW_TEST_SIZE, 100 );
	drawing1->back_color = ARGB32_Black ;


	ctx = create_draw_context(DRAW_TEST_SIZE, DRAW_TEST_SIZE);
	/* actuall drawing starts here */
//	for( i = 0 ; i < 50000 ; ++i ) 
	asim_move_to( ctx, 0, 0 ); 
	asim_line_to_aa( ctx, 200, 200 ); 
	asim_line_to_aa( ctx, 100, 10 );
	asim_line_to_aa( ctx, 10, 300 );
	asim_line_to_aa( ctx, 15, 400 );
	asim_move_to(ctx, 15, 420);  
	asim_line_to_aa( ctx, 200, 410 );
	asim_line_to_aa( ctx, 400, 440 );
		
	asim_move_to(ctx, 10, 0); 	  
	asim_set_brush( ctx, 1 ); 
	asim_line_to_aa( ctx, 210, 200 ); 
	asim_line_to_aa( ctx, 110, 10 );
	asim_line_to_aa( ctx, 20, 300 );
	asim_line_to_aa( ctx, 25, 400 );
	asim_move_to(ctx, 15, 430);  
	asim_line_to_aa( ctx, 200, 420 );
	asim_line_to_aa( ctx, 400, 450 );

	asim_move_to(ctx, 20, 0); 	  
	asim_set_brush( ctx, 2 ); 
	asim_line_to_aa( ctx, 220, 200 ); 
	asim_line_to_aa( ctx, 120, 10 );
	asim_line_to_aa( ctx, 30, 300 );
	asim_line_to_aa( ctx, 35, 400 );
	asim_move_to(ctx, 15, 440);    
	asim_line_to_aa( ctx, 200, 430 );
	asim_line_to_aa( ctx, 400, 460 );

    /****************************************/
	/* non-antialiased : */

	asim_move_to(ctx,  200, 0 ); 
	asim_set_brush( ctx, 0 ); 
	asim_line_to( ctx, 400, 200 ); 
	asim_line_to( ctx, 300, 10 );
	asim_line_to( ctx, 210, 300 );
	asim_line_to( ctx, 215, 400 );
	asim_move_to(ctx, 15, 450);  
	asim_line_to( ctx, 200, 440 );
	asim_line_to( ctx, 400, 470 );
		
	asim_move_to(ctx, 210, 0); 	  
	asim_set_brush( ctx, 1 ); 
	asim_line_to( ctx, 410, 200 ); 
	asim_line_to( ctx, 310, 10 );
	asim_line_to( ctx, 220, 300 );
	asim_line_to( ctx, 225, 400 );
	asim_move_to(ctx, 15, 460);  
	asim_line_to( ctx, 200, 450 );
	asim_line_to( ctx, 400, 480 );

	asim_move_to(ctx, 220, 0); 	  
	asim_set_brush( ctx, 2 ); 
	asim_line_to( ctx, 420, 200 ); 
	asim_line_to( ctx, 320, 10 );
	asim_line_to( ctx, 230, 300 );
	asim_line_to( ctx, 235, 400 );
	asim_move_to(ctx, 15, 470);    
	asim_line_to( ctx, 200, 460 );
	asim_line_to( ctx, 400, 490 );

#if 1
	/* commit drawing : */
	apply_draw_context( drawing1, ctx, SCL_DO_ALPHA ); 
	
	layers_num = 2 ;
	layers = create_image_layers( layers_num );
	
	layers[0].im = back ;
	layers[1].im = drawing1 ;
	layers[0].clip_width = DRAW_TEST_SIZE ;
	layers[0].clip_height = DRAW_TEST_SIZE ;
	layers[1].clip_width = DRAW_TEST_SIZE ;
	layers[1].clip_height = DRAW_TEST_SIZE ;
		
	merged_im = merge_layers( asv, layers, layers_num,
		                      DRAW_TEST_SIZE, DRAW_TEST_SIZE,
							  ASA_ASImage,
							  0, ASIMAGE_QUALITY_DEFAULT );


	ASImage2file( merged_im, NULL, "test_asdraw.png", ASIT_Png, NULL );
	destroy_asimage( &merged_im );
#endif

	return 0;
}
	

#endif

