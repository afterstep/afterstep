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

typedef struct ASDrawPoint
{
	int x, y;
}ASDrawPoint ;

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

	void *draw_data ;
	/* This function changes curr_x and curr_y if nesseccary and 
	 * returns False when drawing should stop */ 
	Bool (*draw_func)( struct ASDrawContext *ctx );  

}ASDrawContext;

#define AS_DRAW_BRUSHES	1

CARD8 _round5x5[25] =
{  0,  24,  64,  24,   0,
  24,  64,  80,  64,  24,
  64,  80, 255,  80,  64,
  24,  64,  80,  64,  24,
   0,  24,  64,  24,   0,};

ASDrawTool StandardBrushes[AS_DRAW_BRUSHES] = 
{
	{5, 5, 2, 2, _round5x5}
};	 

/*********************************************************************************/
/* auxilary functions : 											 */
/*********************************************************************************/

static inline void
apply_tool( ASDrawContext *ctx, int curr_x, int curr_y )
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
}	   

static inline void
apply_tool_aa( ASDrawContext *ctx, int curr_x, int curr_y, CARD8 ratio )
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


/*********************************************************************************/
/* drawing functions : 											 */
/*********************************************************************************/


static Bool 
ctx_draw_line_solid( ASDrawContext *ctx )
{
	ASDrawPoint *dest = (ASDrawPoint*)(ctx->draw_data);
	int x, y, end, dir = 1;

	int dx = dest->x - ctx->curr_x ; 
	int dy = dest->y - ctx->curr_y ; 
	if( dx < 0 ) 
		dx = -dx ; 
	if( dy < 0 ) 
		dy = -dy ;
	
	if( dx >= dy ) 
	{	
      	int Dy = -dx + 2*dy;
      	int inct = 2*dy;
		int incf = 2*(dy - dx);
		
		if( dest->y > ctx->curr_y ) 
		{
			x = ctx->curr_x ; 
			y = ctx->curr_y ; 
			end = dest->x ; 
		}else
		{
			x = dest->x ; 
			y = dest->y ; 
			end = ctx->curr_x ; 
		}
		
		if( end < x ) 
			dir = -1 ;				 
      	
		apply_tool( ctx, x, y );
      	while(x != end)
		{
        	x += dir;
        	if(Dy > 0)
			{
          		Dy += incf;
          		++y;
        	}else 
				Dy += inct;
        	apply_tool( ctx, x, y );
      	}
	}else
	{
      	int Dx = -dy + 2*dx;
      	int inct = 2*dx;
		int incf = 2*(dx - dy);
		
		if( dest->x > ctx->curr_x ) 
		{
			y = ctx->curr_y ; 
			x = ctx->curr_x ; 
			end = dest->y ; 
		}else
		{
			y = dest->y ; 
			x = dest->x ; 
			end = ctx->curr_y ; 
		}
		
		if( end < y ) 
			dir = -1 ;				 
      	
		apply_tool( ctx, x, y );
      	while(y != end)
		{
        	y += dir;
        	if(Dx > 0)
			{
          		Dx += incf;
          		++x;
        	}else 
				Dx += inct;
        	apply_tool( ctx, x, y );
      	}
	}		 

	ctx->curr_x = dest->x ;
	ctx->curr_y = dest->y ;

	return False ;
}	 

static Bool 
ctx_draw_line_solid_aa( ASDrawContext *ctx )
{
	ASDrawPoint *dest = (ASDrawPoint*)(ctx->draw_data);
	int x, y, end, dir = 1;

	int dx = dest->x - ctx->curr_x ; 
	int dy = dest->y - ctx->curr_y ; 
	if( dx < 0 ) 
		dx = -dx ;
	if( dy < 0 ) 
		dy = -dy ;

	/* if( dx == 0 || dy == 0 || dx == dy ) 
		return ctx_draw_line_solid( ctx ); */

	
	if( dx >= dy ) 
	{	
#define MAX_AA_INC		0x0000FFFF
		CARD32 inc = ((CARD32)dy * MAX_AA_INC)/(CARD32)dx; 
		CARD32 curr = 0;

		if( dest->y >= ctx->curr_y ) 
		{
			x = ctx->curr_x ; 
			y = ctx->curr_y ; 
			end = dest->x ; 
		}else
		{
			x = dest->x ; 
			y = dest->y ; 
			end = ctx->curr_x ; 
		}
		   
		apply_tool_aa( ctx, x, y, 255 ) ; 
		
		if( end < x ) 
			dir = -1 ;				 
		
		LOCAL_DEBUG_OUT( "inc = %ld, dx = %d, dy = %d, x = %d, y = %d, end = %d, dir = %d", inc, dx, dy, x, y, end, dir );
		while( x != end ) 
		{
			curr += inc ; 	
			if( curr >= MAX_AA_INC  )
			{
				curr = 0 ; 
				++y	;
			}	 
			x += dir ;
			{
				register int above = ((curr&0x00FF00)>>8);
				apply_tool_aa( ctx, x, y, 255 - above ) ; 
				apply_tool_aa( ctx, x, y+1, above ) ;
			}
		}
	}else
	{
      	int Dx = -dy + 2*dx;
      	int inct = 2*dx;
		int incf = 2*(dx - dy);
//		int minDx = -incf ; 
//		int maxDx = inct ;
		int rangeDx = inct-incf ;
		int ratio = 0x00FFFFFF/rangeDx ; 
		CARD32 curr = (Dx-incf)*ratio; 

		
		if( dest->x > ctx->curr_x ) 
		{
			y = ctx->curr_y ; 
			x = ctx->curr_x ; 
			end = dest->y ; 
		}else
		{
			y = dest->y ; 
			x = dest->x ; 
			end = ctx->curr_y ; 
		}
		
		if( end < y ) 
			dir = -1 ;				 
      	
		apply_tool( ctx, x, y );
      	while(y != end)
		{
        	y += dir;
        	if(Dx > 0)
			{
          		Dx += incf;
				curr = (Dx-incf)*ratio ;
          		++x;
        	}else 
			{	
				Dx += inct;
				curr += ratio*inct ; 
			}
			{
				register int above = (curr&0x00FF0000)>>16;
				if( above > 108 && above < 148 ) 
				{
					apply_tool_aa( ctx, x, y, 108 ) ; 
					apply_tool_aa( ctx, x+1, y, 108 ) ;
				}else
				{		 
					apply_tool_aa( ctx, x, y, 255 - above ) ; 
					apply_tool_aa( ctx, x+1, y, above ) ;
				}
			}
      	}
	}		 

	ctx->curr_x = dest->x ;
	ctx->curr_y = dest->y ;

	return False ;
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
				   
	return ctx;
}	   
	
void
asim_line_to( ASDrawContext *ctx, int dst_x, int dst_y ) 
{
	ASDrawPoint dst ;
	dst.x = dst_x ; 
	dst.y = dst_y ; 

	ctx->draw_data = &dst ; 
	ctx_draw_line_solid( ctx );
	ctx->draw_data = NULL ;
}	 


void
asim_line_to_aa( ASDrawContext *ctx, int dst_x, int dst_y ) 
{
	ASDrawPoint dst ;
	dst.x = dst_x ; 
	dst.y = dst_y ; 

	ctx->draw_data = &dst ; 
	ctx_draw_line_solid_aa( ctx );
	ctx->draw_data = NULL ;
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
	{
		ctx->curr_x = 0 ; 	  
		ctx->curr_y = 0 ; 
		asim_line_to_aa( ctx, 200, 200 ); 
	}
	asim_line_to_aa( ctx, 100, 10 );
	asim_line_to_aa( ctx, 10, 300 );

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

	return 1;
}
	

#endif

