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
		aw = corner_x + tw - cw;
	if( corner_y + th > ch ) 
		ah = corner_y + th - ch;

	for( y = 0 ; y < ah ; ++y ) 
	{	
		for( x = 0 ; x < aw ; ++x ) 
			dst[x] += src[x] ;
		src += tw ; 
		dst += cw ; 
	}
}	   

/*********************************************************************************/
/* drawing functions : 											 */
/*********************************************************************************/


Bool 
draw_line_solid( ASDrawContext *ctx )
{
	ASDrawPoint *dest = (ASDrawPoint*)(ctx->draw_data);
	int start_x, start_y, end_x, end_y ; 

	if( dest->y < ctx->curr_y ) 
	{
		start_x = ctx->curr_x ; 
		start_y = ctx->curr_y ; 
		end_x = dest->x ; 
		end_y = dest->y ; 
	}else
	{
		end_x = ctx->curr_x ; 
		end_y = ctx->curr_y ; 
		start_x = dest->x ; 
		start_y = dest->y ; 
	}		 
	
	if( start_x < end_x ) 
	{
		int x = start_x, y = start_y;
      	int dx = end_x - x;  
	  	int dy = end_y - y;
      	int Dy = -dx + 2*dy;
      	int inct = 2*dy;
      	int incf = 2*(dy - dx);
      	
		apply_tool( ctx, x, y );
      	while(x != end_x)
		{
        	++x;
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
		int x = start_x, y = start_y;
      	int dx = end_x - x;  
	  	int dy = end_y - y;
      	int Dy = -dx + 2*dy;
      	int inct = 2*dy;
      	int incf = 2*(dy - dx);
      	
		apply_tool( ctx, x, y );
      	while(x != end_x)
		{
        	--x;
        	if(Dy > 0)
			{
          		Dy += incf;
          		++y;
        	}else 
				Dy += inct;
        	apply_tool( ctx, x, y );
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

