/*
 * Copyright (c) 2000,2001 Sasha Vasko <sasha at aftercode.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License.
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

#undef LOCAL_DEBUG
#undef DO_CLOCKING
#undef DEBUG_HSV_ADJUSTMENT
#define USE_64BIT_FPU
#undef NEED_RBITSHIFT_FUNCS

#ifdef _WIN32
#include "win32/config.h"
#else
#include "config.h"
#endif

#ifdef DO_CLOCKING
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#include <math.h>


#ifdef _WIN32
# include "win32/afterbase.h"
#else
# include "afterbase.h"
#endif
#include "asvisual.h"
#include "blender.h"
#include "asimage.h"
#include "imencdec.h"
#include "transform.h"

/* ******************************************************************************/
/* below goes all kinds of funky stuff we can do with scanlines : 			   */
/* ******************************************************************************/
/* this will enlarge array based on count of items in dst per PAIR of src item with smoothing/scatter/dither */
/* the following formulas use linear approximation to calculate   */
/* color values for new pixels : 				  				  */
/* for scale factor of 2 we use this formula :    */
/* C = (-C1+3*C2+3*C3-C4)/4 					  */
/* or better :				 					  */
/* C = (-C1+5*C2+5*C3-C4)/8 					  */
#define INTERPOLATE_COLOR1(c) 			   	((c)<<QUANT_ERR_BITS)  /* nothing really to interpolate here */
#define INTERPOLATE_COLOR2(c1,c2,c3,c4)    	((((c2)<<2)+(c2)+((c3)<<2)+(c3)-(c1)-(c4))<<(QUANT_ERR_BITS-3))
#define INTERPOLATE_COLOR2_V(c1,c2,c3,c4)    	((((c2)<<2)+(c2)+((c3)<<2)+(c3)-(c1)-(c4))>>3)
/* for scale factor of 3 we use these formulas :  */
/* Ca = (-2C1+8*C2+5*C3-2C4)/9 		  			  */
/* Cb = (-2C1+5*C2+8*C3-2C4)/9 		  			  */
/* or better : 									  */
/* Ca = (-C1+5*C2+3*C3-C4)/6 		  			  */
/* Cb = (-C1+3*C2+5*C3-C4)/6 		  			  */
#define INTERPOLATE_A_COLOR3(c1,c2,c3,c4)  	(((((c2)<<2)+(c2)+((c3)<<1)+(c3)-(c1)-(c4))<<QUANT_ERR_BITS)/6)
#define INTERPOLATE_B_COLOR3(c1,c2,c3,c4)  	(((((c2)<<1)+(c2)+((c3)<<2)+(c3)-(c1)-(c4))<<QUANT_ERR_BITS)/6)
#define INTERPOLATE_A_COLOR3_V(c1,c2,c3,c4)  	((((c2)<<2)+(c2)+((c3)<<1)+(c3)-(c1)-(c4))/6)
#define INTERPOLATE_B_COLOR3_V(c1,c2,c3,c4)  	((((c2)<<1)+(c2)+((c3)<<2)+(c3)-(c1)-(c4))/6)
/* just a hypotesus, but it looks good for scale factors S > 3: */
/* Cn = (-C1+(2*(S-n)+1)*C2+(2*n+1)*C3-C4)/2S  	  			   */
/* or :
 * Cn = (-C1+(2*S+1)*C2+C3-C4+n*(2*C3-2*C2)/2S  			   */
/*       [ T                   [C2s]  [C3s]]   			       */
#define INTERPOLATION_Cs(c)	 		 	    ((c)<<1)
/*#define INTERPOLATION_TOTAL_START(c1,c2,c3,c4,S) 	(((S)<<1)*(c2)+((c3)<<1)+(c3)-c2-c1-c4)*/
#define INTERPOLATION_TOTAL_START(c1,c2,c3,c4,S) 	((((S)<<1)+1)*(c2)+(c3)-(c1)-(c4))
#define INTERPOLATION_TOTAL_STEP(c2,c3)  	((c3<<1)-(c2<<1))
#define INTERPOLATE_N_COLOR(T,S)		  	(((T)<<(QUANT_ERR_BITS-1))/(S))

#define AVERAGE_COLOR1(c) 					((c)<<QUANT_ERR_BITS)
#define AVERAGE_COLOR2(c1,c2)				(((c1)+(c2))<<(QUANT_ERR_BITS-1))
#define AVERAGE_COLORN(T,N)					(((T)<<QUANT_ERR_BITS)/N)

static inline void
enlarge_component12( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* expected len >= 2  */
	register int i = 0, k = 0;
	register int c1 = src[0], c4;
	--len; --len ;
	while( i < len )
	{
		c4 = src[i+2];
		/* that's right we can do that PRIOR as we calculate nothing */
		dst[k] = INTERPOLATE_COLOR1(src[i]) ;
		if( scales[i] == 2 )
		{
			register int c2 = src[i], c3 = src[i+1] ;
			c3 = INTERPOLATE_COLOR2(c1,c2,c3,c4);
			dst[++k] = (c3&0xFF000000 )?0:c3;
		}
		c1 = src[i];
		++k;
		++i;
	}

	/* to avoid one more if() in loop we moved tail part out of the loop : */
	if( scales[i] == 1 )
		dst[k] = INTERPOLATE_COLOR1(src[i]);
	else
	{
		register int c2 = src[i], c3 = src[i+1] ;
		c2 = INTERPOLATE_COLOR2(c1,c2,c3,c3);
		dst[k] = (c2&0xFF000000 )?0:c2;
	}
	dst[k+1] = INTERPOLATE_COLOR1(src[i+1]);
}

static inline void
enlarge_component23( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* expected len >= 2  */
	register int i = 0, k = 0;
	register int c1 = src[0], c4 = src[1];
	if( scales[0] == 1 )
	{/* special processing for first element - it can be 1 - others can only be 2 or 3 */
		dst[k] = INTERPOLATE_COLOR1(src[0]) ;
		++k;
		++i;
	}
	--len; --len ;
	while( i < len )
	{
		register int c2 = src[i], c3 = src[i+1] ;
		c4 = src[i+2];
		dst[k] = INTERPOLATE_COLOR1(c2) ;
		if( scales[i] == 2 )
		{
			c3 = INTERPOLATE_COLOR2(c1,c2,c3,c3);
			dst[++k] = (c3&0x7F000000 )?0:c3;
		}else
		{
			dst[++k] = INTERPOLATE_A_COLOR3(c1,c2,c3,c4);
			if( dst[k]&0x7F000000 )
				dst[k] = 0 ;
			c3 = INTERPOLATE_B_COLOR3(c1,c2,c3,c3);
			dst[++k] = (c3&0x7F000000 )?0:c3;
		}
		c1 = c2 ;
		++k;
		++i;
	}
	/* to avoid one more if() in loop we moved tail part out of the loop : */
	{
		register int c2 = src[i], c3 = src[i+1] ;
		dst[k] = INTERPOLATE_COLOR1(c2) ;
		if( scales[i] == 2 )
		{
			c2 = INTERPOLATE_COLOR2(c1,c2,c3,c3);
			dst[k+1] = (c2&0x7F000000 )?0:c2;
		}else
		{
			if( scales[i] == 1 )
				--k;
			else
			{
				dst[++k] = INTERPOLATE_A_COLOR3(c1,c2,c3,c3);
				if( dst[k]&0x7F000000 )
					dst[k] = 0 ;
				c2 = INTERPOLATE_B_COLOR3(c1,c2,c3,c3);
  				dst[k+1] = (c2&0x7F000000 )?0:c2;
			}
		}
	}
 	dst[k+2] = INTERPOLATE_COLOR1(src[i+1]) ;
}

/* this case is more complex since we cannot really hardcode coefficients
 * visible artifacts on smooth gradient-like images
 */
static inline void
enlarge_component( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* we skip all checks as it is static function and we want to optimize it
  * as much as possible */
	int i = 0;
	int c1 = src[0];
	register int T ;
	--len ;
	if( len < 1 )
	{
		CARD32 c = INTERPOLATE_COLOR1(c1) ;
		for( i = 0 ; i < scales[0] ; ++i )
			dst[i] = c;
		return;
	}
	do
	{
		register short S = scales[i];
		register int step = INTERPOLATION_TOTAL_STEP(src[i],src[i+1]);

		if( i+1 == len )
			T = INTERPOLATION_TOTAL_START(c1,src[i],src[i+1],src[i+1],S);
		else
			T = INTERPOLATION_TOTAL_START(c1,src[i],src[i+1],src[i+2],S);

/*		LOCAL_DEBUG_OUT( "pixel %d, S = %d, step = %d", i, S, step );*/
		if( step )
		{
			register int n = 0 ;
			do
			{
				dst[n] = (T&0x7F000000)?0:INTERPOLATE_N_COLOR(T,S);
				if( ++n >= S ) break;
				T = (int)T + (int)step;
			}while(1);
			dst += n ;
		}else
		{
			register CARD32 c = (T&0x7F000000)?0:INTERPOLATE_N_COLOR(T,S);
			while(--S >= 0){	dst[S] = c;	}
			dst += scales[i] ;
		}
		c1 = src[i];
	}while(++i < len );
	*dst = INTERPOLATE_COLOR1(src[i]) ;
/*LOCAL_DEBUG_OUT( "%d pixels written", k );*/
}

static inline void
enlarge_component_dumb( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* we skip all checks as it is static function and we want to optimize it
  * as much as possible */
	int i = 0, k = 0;
	do
	{
		register CARD32 c = INTERPOLATE_COLOR1(src[i]);
		int max_k = k+scales[i];
		do
		{
			dst[k] = c ;
		}while( ++k < max_k );
	}while( ++i < len );
}

/* this will shrink array based on count of items in src per one dst item with averaging */
static inline void
shrink_component( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* we skip all checks as it is static function and we want to optimize it
  * as much as possible */
	register int i = -1, k = -1;
	while( ++k < len )
	{
		register int reps = scales[k] ;
		register int c1 = src[++i];
/*LOCAL_DEBUG_OUT( "pixel = %d, scale[k] = %d", k, reps );*/
		if( reps == 1 )
			dst[k] = AVERAGE_COLOR1(c1);
		else if( reps == 2 )
		{
			++i;
			dst[k] = AVERAGE_COLOR2(c1,src[i]);
		}else
		{
			reps += i-1;
			while( reps > i )
			{
				++i ;
				c1 += src[i];
			}
			{
				register short S = scales[k];
				dst[k] = AVERAGE_COLORN(c1,S);
			}
		}
	}
}
static inline void
shrink_component11( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		dst[i] = AVERAGE_COLOR1(src[i]);
}


static inline void
reverse_component( register CARD32 *src, register CARD32 *dst, int *unused, int len )
{
	register int i = 0;
	src += len-1 ;
	do
	{
		dst[i] = src[-i];
	}while(++i < len );
}

static inline void
add_component( CARD32 *src, CARD32 *incr, int *scales, int len )
{
	int i = 0;

	len += len&0x01;
#if 1
#ifdef HAVE_MMX
	if( asimage_use_mmx )
	{
		double *ddst = (double*)&(src[0]);
		double *dinc = (double*)&(incr[0]);
		len = len>>1;
		do{
			asm volatile
       		(
            	"movq %0, %%mm0  \n\t" /* load 8 bytes from src[i] into MM0 */
            	"paddd %1, %%mm0 \n\t" /* MM0=src[i]>>1              */
            	"movq %%mm0, %0  \n\t" /* store the result in dest */
				: "=m" (ddst[i])       /* %0 */
				:  "m"  (dinc[i])       /* %2 */
	        );
		}while( ++i < len );
	}else
#endif
#endif
	{
		register int c1, c2;
		do{
			c1 = (int)src[i] + (int)incr[i] ;
			c2 = (int)src[i+1] + (int)incr[i+1] ;
			src[i] = c1;
			src[i+1] = c2;
			i += 2 ;
		}while( i < len );
	}
}

#ifdef NEED_RBITSHIFT_FUNCS
static inline void
rbitshift_component( register CARD32 *src, register CARD32 *dst, int shift, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		dst[i] = src[i]>>shift;
}
#endif

static inline void
start_component_interpolation( CARD32 *c1, CARD32 *c2, CARD32 *c3, CARD32 *c4, register CARD32 *T, register CARD32 *step, CARD16 S, int len)
{
	register int i;
	for( i = 0 ; i < len ; i++ )
	{
		register int rc2 = c2[i], rc3 = c3[i] ;
		T[i] = INTERPOLATION_TOTAL_START(c1[i],rc2,rc3,c4[i],S)/(S<<1);
		step[i] = INTERPOLATION_TOTAL_STEP(rc2,rc3)/(S<<1);
	}
}

static inline void
component_interpolation_hardcoded( CARD32 *c1, CARD32 *c2, CARD32 *c3, CARD32 *c4, register CARD32 *T, CARD32 *unused, CARD16 kind, int len)
{
	register int i;
	if( kind == 1 )
	{
		for( i = 0 ; i < len ; i++ )
		{
#if 1
			/* its seems that this simple formula is completely sufficient
			   and even better then more complicated one : */
			T[i] = (c2[i]+c3[i])>>1 ;
#else
    		register int minus = c1[i]+c4[i] ;
			register int plus  = (c2[i]<<1)+c2[i]+(c3[i]<<1)+c3[i];

			T[i] = ( (plus>>1) < minus )?(c2[i]+c3[i])>>1 :
								   		 (plus-minus)>>2;
#endif
		}
	}else if( kind == 2 )
	{
		for( i = 0 ; i < len ; i++ )
		{
    		register int rc1 = c1[i], rc2 = c2[i], rc3 = c3[i] ;
			T[i] = INTERPOLATE_A_COLOR3_V(rc1,rc2,rc3,c4[i]);
		}
	}else
		for( i = 0 ; i < len ; i++ )
		{
    		register int rc1 = c1[i], rc2 = c2[i], rc3 = c3[i] ;
			T[i] = INTERPOLATE_B_COLOR3_V(rc1,rc2,rc3,c4[i]);
		}
}

#ifdef NEED_RBITSHIFT_FUNCS
static inline void
divide_component_mod( register CARD32 *data, CARD16 ratio, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		data[i] /= ratio;
}

static inline void
rbitshift_component_mod( register CARD32 *data, int bits, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		data[i] = data[i]>>bits;
}
#endif
void
print_component( register CARD32 *data, int nonsense, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		fprintf( stderr, " %8.8lX", (long)data[i] );
	fprintf( stderr, "\n");
}

static inline void
tint_component_mod( register CARD32 *data, CARD16 ratio, int len )
{
	register int i ;
	if( ratio == 255 )
		for( i = 0 ; i < len ; ++i )
			data[i] = data[i]<<8;
	else if( ratio == 128 )
		for( i = 0 ; i < len ; ++i )
			data[i] = data[i]<<7;
	else if( ratio == 0 )
		for( i = 0 ; i < len ; ++i )
			data[i] = 0;
	else
		for( i = 0 ; i < len ; ++i )
			data[i] = data[i]*ratio;
}

static inline void
make_component_gradient16( register CARD32 *data, CARD16 from, CARD16 to, CARD8 seed, int len )
{
	register int i ;
	long incr = (((long)to<<8)-((long)from<<8))/len ;

	if( incr == 0 )
		for( i = 0 ; i < len ; ++i )
			data[i] = from;
	else
	{
		long curr = from<<8;
		curr += ((long)(((CARD32)seed)<<8) > incr)?incr:((CARD32)seed)<<8 ;
		for( i = 0 ; i < len ; ++i )
		{/* we make calculations in 24bit per chan, then convert it back to 16 and
		  * carry over half of the quantization error onto the next pixel */
			data[i] = curr>>8;
			curr += ((curr&0x00FF)>>1)+incr ;
		}
	}
}


static inline void
copytintpad_scanline( ASScanline *src, ASScanline *dst, int offset, ARGB32 tint )
{
	register int i ;
	CARD32 chan_tint[4], chan_fill[4] ;
	int color ;
	int copy_width = src->width, dst_offset = 0, src_offset = 0;

	if( offset+(int)src->width < 0 || offset > (int)dst->width )
		return;
	chan_tint[IC_RED]   = ARGB32_RED8  (tint)<<1;
	chan_tint[IC_GREEN] = ARGB32_GREEN8(tint)<<1;
	chan_tint[IC_BLUE]  = ARGB32_BLUE8 (tint)<<1;
	chan_tint[IC_ALPHA] = ARGB32_ALPHA8(tint)<<1;
	chan_fill[IC_RED]   = ARGB32_RED8  (dst->back_color)<<dst->shift;
	chan_fill[IC_GREEN] = ARGB32_GREEN8(dst->back_color)<<dst->shift;
	chan_fill[IC_BLUE]  = ARGB32_BLUE8 (dst->back_color)<<dst->shift;
	chan_fill[IC_ALPHA] = ARGB32_ALPHA8(dst->back_color)<<dst->shift;
	if( offset < 0 )
		src_offset = -offset ;
	else
		dst_offset = offset ;
	copy_width = MIN( src->width-src_offset, dst->width-dst_offset );

	dst->flags = src->flags ;
	for( color = 0 ; color < IC_NUM_CHANNELS ; ++color )
	{
		register CARD32 *psrc = src->channels[color]+src_offset;
		register CARD32 *pdst = dst->channels[color];
		int ratio = chan_tint[color];
/*	fprintf( stderr, "channel %d, tint is %d(%X), src_width = %d, src_offset = %d, dst_width = %d, dst_offset = %d psrc = %p, pdst = %p\n", color, ratio, ratio, src->width, src_offset, dst->width, dst_offset, psrc, pdst );
*/
		{
/*			register CARD32 fill = chan_fill[color]; */
			for( i = 0 ; i < dst_offset ; ++i )
				pdst[i] = 0;
			pdst += dst_offset ;
		}

		if( get_flags(src->flags, 0x01<<color) )
		{
			if( ratio >= 254 )
				for( i = 0 ; i < copy_width ; ++i )
					pdst[i] = psrc[i]<<8;
			else if( ratio == 128 )
				for( i = 0 ; i < copy_width ; ++i )
					pdst[i] = psrc[i]<<7;
			else if( ratio == 0 )
				for( i = 0 ; i < copy_width ; ++i )
					pdst[i] = 0;
			else
				for( i = 0 ; i < copy_width ; ++i )
					pdst[i] = psrc[i]*ratio;
		}else
		{
		    ratio = ratio*chan_fill[color];
			for( i = 0 ; i < copy_width ; ++i )
				pdst[i] = ratio;
			set_flags( dst->flags, (0x01<<color));
		}
		{
/*			register CARD32 fill = chan_fill[color]; */
			for( ; i < (int)dst->width-dst_offset ; ++i )
				pdst[i] = 0;
/*				print_component(pdst, 0, dst->width ); */
		}
	}
}

/* **********************************************************************************************/
/* drawing gradient on scanline :  															   */
/* **********************************************************************************************/
void
make_gradient_scanline( ASScanline *scl, ASGradient *grad, ASFlagType filter, ARGB32 seed )
{
	if( scl && grad && filter != 0 )
	{
		int offset = 0, step, i, max_i = grad->npoints - 1 ;
		ARGB32 last_color = ARGB32_Black ;
		int last_idx = 0;
		double last_offset = 0., *offsets = grad->offset ;
		int *used = safecalloc(max_i+1, sizeof(int));
		/* lets find the color of the very first point : */
		for( i = 0 ; i <= max_i ; ++i )
			if( offsets[i] <= 0. )
			{
				last_color = grad->color[i] ;
				last_idx = i ;
				used[i] = 1 ;
				break;
			}

		for( i = 0  ; i <= max_i ; i++ )
		{
			register int k ;
			int new_idx = -1 ;
			/* now lets find the next point  : */
			for( k = 0 ; k <= max_i ; ++k )
			{
				if( used[k]==0 && offsets[k] >= last_offset )
				{
					if( new_idx < 0 )
						new_idx = k ;
					else if( offsets[new_idx] > offsets[k] )
						new_idx = k ;
					else
					{
						register int d1 = new_idx-last_idx ;
						register int d2 = k - last_idx ;
						if( d1*d1 > d2*d2 )
							new_idx = k ;
					}
				}
			}
			if( new_idx < 0 )
				break;
			used[new_idx] = 1 ;
			step = (int)((grad->offset[new_idx] * (double)scl->width) - (double)offset) ;
/*			fprintf( stderr, __FUNCTION__":%d>last_offset = %f, last_color = %8.8X, new_idx = %d, max_i = %d, new_offset = %f, new_color = %8.8X, step = %d, offset = %d\n", __LINE__, last_offset, last_color, new_idx, max_i, offsets[new_idx], grad->color[new_idx], step, offset ); */
			if( step > (int)scl->width-offset )
				step = (int)scl->width-offset ;
			if( step > 0 )
			{
				int color ;
				for( color = 0 ; color < IC_NUM_CHANNELS ; ++color )
					if( get_flags( filter, 0x01<<color ) )
					{
						LOCAL_DEBUG_OUT("channel %d from #%4.4lX to #%4.4lX, ofset = %d, step = %d",
	 	 									color, ARGB32_CHAN8(last_color,color)<<8, ARGB32_CHAN8(grad->color[new_idx],color)<<8, offset, step );
						make_component_gradient16( scl->channels[color]+offset,
												   (CARD16)(ARGB32_CHAN8(last_color,color)<<8),
												   (CARD16)(ARGB32_CHAN8(grad->color[new_idx],color)<<8),
												   (CARD8)ARGB32_CHAN8(seed,color),
												   step);
					}
				offset += step ;
			}
			last_offset = offsets[new_idx];
			last_color = grad->color[new_idx];
			last_idx = new_idx ;
		}
		scl->flags = filter ;
		free( used );
	}
}

/* **********************************************************************************************/
/* Scaling code ; 																			   */
/* **********************************************************************************************/
Bool
check_scale_parameters( ASImage *src, unsigned int *to_width, unsigned int *to_height )
{
	if( src == NULL )
		return False;

	if( *to_width == 0 )
		*to_width = src->width ;
	else if( *to_width < 2 )
		*to_width = 2 ;
	if( *to_height == 0 )
		*to_height = src->height ;
	else if( *to_height < 2 )
		*to_height = 2 ;
	return True;
}

int *
make_scales( int from_size, int to_size, int tail )
{
	int *scales ;
    int smaller = MIN(from_size,to_size);
    int bigger  = MAX(from_size,to_size);
	register int i = 0, k = 0;
	int eps;
    LOCAL_DEBUG_OUT( "from %d to %d tail %d", from_size, to_size, tail );
	if( from_size < to_size )
    {
        smaller-=tail;
        bigger-=tail ;
    }
    if( smaller <= 0 )
		smaller = 1;
    if( bigger <= 0 )
		bigger = 1;
	scales = safecalloc( smaller+tail, sizeof(int));
    eps = -bigger/2;
    LOCAL_DEBUG_OUT( "smaller %d, bigger %d, eps %d", smaller, bigger, eps );
    /* now using Bresengham algoritm to fiill the scales :
	 * since scaling is merely transformation
	 * from 0:bigger space (x) to 0:smaller space(y)*/
	for ( i = 0 ; i < bigger ; i++ )
	{
		++scales[k];
		eps += smaller;
        LOCAL_DEBUG_OUT( "scales[%d] = %d, i = %d, k = %d, eps %d", k, scales[k], i, k, eps );
        if( eps+eps >= bigger )
		{
			++k ;
			eps -= bigger ;
		}
	}
	return scales;
}

/* *******************************************************************/
void
scale_image_down( ASImageDecoder *imdec, ASImageOutput *imout, int h_ratio, int *scales_h, int* scales_v)
{
	ASScanline dst_line, total ;
	int k = -1;
	int max_k 	 = imout->im->height,
		line_len = MIN(imout->im->width, imdec->out_width);

	prepare_scanline( imout->im->width, QUANT_ERR_BITS, &dst_line, imout->asv->BGR_mode );
	prepare_scanline( imout->im->width, QUANT_ERR_BITS, &total, imout->asv->BGR_mode );
	while( ++k < max_k )
	{
		int reps = scales_v[k] ;
		imdec->decode_image_scanline( imdec );
		total.flags = imdec->buffer.flags ;
		CHOOSE_SCANLINE_FUNC(h_ratio,imdec->buffer,total,scales_h,line_len);

		while( --reps > 0 )
		{
			imdec->decode_image_scanline( imdec );
			total.flags = imdec->buffer.flags ;
			CHOOSE_SCANLINE_FUNC(h_ratio,imdec->buffer,dst_line,scales_h,line_len);
			SCANLINE_FUNC(add_component,total,dst_line,NULL,total.width);
		}

		imout->output_image_scanline( imout, &total, scales_v[k] );
	}
	free_scanline(&dst_line, True);
	free_scanline(&total, True);
}

void
scale_image_up( ASImageDecoder *imdec, ASImageOutput *imout, int h_ratio, int *scales_h, int* scales_v)
{
	ASScanline src_lines[4], *c1, *c2, *c3, *c4 = NULL;
	int i = 0, max_i,
		line_len = MIN(imout->im->width, imdec->out_width),
		out_width = imout->im->width;
	ASScanline step ;

	prepare_scanline( out_width, 0, &(src_lines[0]), imout->asv->BGR_mode);
	prepare_scanline( out_width, 0, &(src_lines[1]), imout->asv->BGR_mode);
	prepare_scanline( out_width, 0, &(src_lines[2]), imout->asv->BGR_mode);
	prepare_scanline( out_width, 0, &(src_lines[3]), imout->asv->BGR_mode);
	prepare_scanline( out_width, QUANT_ERR_BITS, &step, imout->asv->BGR_mode );

/*	set_component(src_lines[0].red,0x00000000,0,out_width*3); */
	imdec->decode_image_scanline( imdec );
	src_lines[1].flags = imdec->buffer.flags ;
	CHOOSE_SCANLINE_FUNC(h_ratio,imdec->buffer,src_lines[1],scales_h,line_len);

	step.flags = src_lines[0].flags = src_lines[1].flags ;

	SCANLINE_FUNC(copy_component,src_lines[1],src_lines[0],0,out_width);

	imdec->decode_image_scanline( imdec );
	src_lines[2].flags = imdec->buffer.flags ;
	CHOOSE_SCANLINE_FUNC(h_ratio,imdec->buffer,src_lines[2],scales_h,line_len);

	i = 0 ;
	max_i = imdec->out_height-1 ;
	do
	{
		int S = scales_v[i] ;
		c1 = &(src_lines[i&0x03]);
		c2 = &(src_lines[(i+1)&0x03]);
		c3 = &(src_lines[(i+2)&0x03]);
		c4 = &(src_lines[(i+3)&0x03]);

		if( i+1 < max_i )
		{
			imdec->decode_image_scanline( imdec );
			c4->flags = imdec->buffer.flags ;
			CHOOSE_SCANLINE_FUNC(h_ratio,imdec->buffer,*c4,scales_h,line_len);
		}
		/* now we'll prepare total and step : */
        if( S > 0 )
        {
            imout->output_image_scanline( imout, c2, 1);
            if( S > 1 )
            {
                if( S == 2 )
                {
                    SCANLINE_COMBINE(component_interpolation_hardcoded,*c1,*c2,*c3,*c4,*c1,*c1,1,out_width);
                    imout->output_image_scanline( imout, c1, 1);
                }else if( S == 3 )
                {
                    SCANLINE_COMBINE(component_interpolation_hardcoded,*c1,*c2,*c3,*c4,*c1,*c1,2,out_width);
                    imout->output_image_scanline( imout, c1, 1);
                    SCANLINE_COMBINE(component_interpolation_hardcoded,*c1,*c2,*c3,*c4,*c1,*c1,3,out_width);
                    imout->output_image_scanline( imout, c1, 1);
                }else
                {
                    SCANLINE_COMBINE(start_component_interpolation,*c1,*c2,*c3,*c4,*c1,step,(CARD8)S,out_width);
                    do
                    {
                        imout->output_image_scanline( imout, c1, 1);
                        if((--S)<=1)
                            break;
                        SCANLINE_FUNC(add_component,*c1,step,NULL,out_width );
                    }while(1);
                }
            }
        }
	}while( ++i < max_i );
    imout->output_image_scanline( imout, c3, 1);
	free_scanline(&step, True);
	free_scanline(&(src_lines[3]), True);
	free_scanline(&(src_lines[2]), True);
	free_scanline(&(src_lines[1]), True);
	free_scanline(&(src_lines[0]), True);
}

void
scale_image_up_dumb( ASImageDecoder *imdec, ASImageOutput *imout, int h_ratio, int *scales_h, int* scales_v)
{
	ASScanline src_line;
	int	line_len = MIN(imout->im->width, imdec->out_width);
	int	out_width = imout->im->width;

	prepare_scanline( out_width, QUANT_ERR_BITS, &src_line, imout->asv->BGR_mode );

	imout->tiling_step = 1 ;
	while( imdec->next_line < (int)imdec->out_height )
	{
		imdec->decode_image_scanline( imdec );
		src_line.flags = imdec->buffer.flags ;
		CHOOSE_SCANLINE_FUNC(h_ratio,imdec->buffer,src_line,scales_h,line_len);
		imout->tiling_range = scales_v[imdec->next_line];
		imout->output_image_scanline( imout, &src_line, 1);
		imout->next_line += scales_v[imdec->next_line]-1;
	}
	free_scanline(&src_line, True);
}


static inline ASImage *
create_destination_image( unsigned int width, unsigned int height, ASAltImFormats format, 
						  unsigned int compression, ARGB32 back_color )
{
	ASImage *dst = create_asimage(width, height, compression);
	if( dst )
	{
		if( format != ASA_ASImage )
			set_flags( dst->flags, ASIM_DATA_NOT_USEFUL );
	
		dst->back_color = back_color ;
	}
	return dst ;
}
						  

/* *****************************************************************************/
/* ASImage transformations : 												  */
/* *****************************************************************************/
ASImage *
scale_asimage( ASVisual *asv, ASImage *src, unsigned int to_width, unsigned int to_height,
			   ASAltImFormats out_format, unsigned int compression_out, int quality )
{
	ASImage *dst = NULL ;
	ASImageOutput  *imout ;
	ASImageDecoder *imdec;
	int h_ratio ;
	int *scales_h = NULL, *scales_v = NULL;
	START_TIME(started);

	if( !check_scale_parameters(src,&to_width,&to_height) )
		return NULL;
	if( (imdec = start_image_decoding(asv, src, SCL_DO_ALL, 0, 0, 0, 0, NULL)) == NULL )
		return NULL;

	dst = create_destination_image( to_width, to_height, out_format, compression_out, src->back_color );

	if( to_width == src->width )
		h_ratio = 0;
	else if( to_width < src->width )
		h_ratio = 1;
	else
	{
		if ( quality == ASIMAGE_QUALITY_POOR )
			h_ratio = 1 ;
		else if( src->width > 1 )
		{
			h_ratio = (to_width/(src->width-1))+1;
			if( h_ratio*(src->width-1) < to_width )
				++h_ratio ;
		}else
			h_ratio = to_width ;
		++h_ratio ;
	}
	scales_h = make_scales( src->width, to_width, ( quality == ASIMAGE_QUALITY_POOR )?0:1 );
	scales_v = make_scales( src->height, to_height, ( quality == ASIMAGE_QUALITY_POOR )?0:1 );
#ifdef LOCAL_DEBUG
	{
	  register int i ;
	  for( i = 0 ; i < MIN(src->width, to_width) ; i++ )
		fprintf( stderr, " %d", scales_h[i] );
	  fprintf( stderr, "\n" );
	  for( i = 0 ; i < MIN(src->height, to_height) ; i++ )
		fprintf( stderr, " %d", scales_v[i] );
	  fprintf( stderr, "\n" );
	}
#endif
#ifdef HAVE_MMX
	mmx_init();
#endif
	if((imout = start_image_output( asv, dst, out_format, QUANT_ERR_BITS, quality )) == NULL )
	{
        destroy_asimage( &dst );
	}else
	{
		if( to_height <= src->height ) 					   /* scaling down */
			scale_image_down( imdec, imout, h_ratio, scales_h, scales_v );
		else if( quality == ASIMAGE_QUALITY_POOR || src->height <= 3 ) 
			scale_image_up_dumb( imdec, imout, h_ratio, scales_h, scales_v );
		else
			scale_image_up( imdec, imout, h_ratio, scales_h, scales_v );
		stop_image_output( &imout );
	}
#ifdef HAVE_MMX
	mmx_off();
#endif
	free( scales_h );
	free( scales_v );
	stop_image_decoding( &imdec );
	SHOW_TIME("", started);
	return dst;
}

ASImage *
tile_asimage( ASVisual *asv, ASImage *src,
		      int offset_x, int offset_y,
			  unsigned int to_width,
			  unsigned int to_height,
			  ARGB32 tint,
			  ASAltImFormats out_format, unsigned int compression_out, int quality )
{
	ASImage *dst = NULL ;
	ASImageDecoder *imdec ;
	ASImageOutput  *imout ;
	START_TIME(started);

LOCAL_DEBUG_CALLER_OUT( "src = %p, offset_x = %d, offset_y = %d, to_width = %d, to_height = %d, tint = #%8.8lX", src, offset_x, offset_y, to_width, to_height, tint );
	if( src== NULL || (imdec = start_image_decoding(asv, src, SCL_DO_ALL, offset_x, offset_y, to_width, 0, NULL)) == NULL )
	{
		LOCAL_DEBUG_OUT( "failed to start image decoding%s", "");
		return NULL;
	}

	dst = create_destination_image( to_width, to_height, out_format, compression_out, src->back_color );

#ifdef HAVE_MMX
	mmx_init();
#endif
	if((imout = start_image_output( asv, dst, out_format, (tint!=0)?8:0, quality)) == NULL )
	{
		LOCAL_DEBUG_OUT( "failed to start image output%s", "");
        destroy_asimage( &dst );
    }else
	{
		int y, max_y = to_height;
LOCAL_DEBUG_OUT("tiling actually...%s", "");
		if( to_height > src->height )
		{
			imout->tiling_step = src->height ;
			max_y = src->height ;
		}
		if( tint != 0 )
		{
			for( y = 0 ; y < max_y ; y++  )
			{
				imdec->decode_image_scanline( imdec );
				tint_component_mod( imdec->buffer.red, (CARD16)(ARGB32_RED8(tint)<<1), to_width );
				tint_component_mod( imdec->buffer.green, (CARD16)(ARGB32_GREEN8(tint)<<1), to_width );
  				tint_component_mod( imdec->buffer.blue, (CARD16)(ARGB32_BLUE8(tint)<<1), to_width );
				tint_component_mod( imdec->buffer.alpha, (CARD16)(ARGB32_ALPHA8(tint)<<1), to_width );
				imout->output_image_scanline( imout, &(imdec->buffer), 1);
			}
		}else
			for( y = 0 ; y < max_y ; y++  )
			{
				imdec->decode_image_scanline( imdec );
				imout->output_image_scanline( imout, &(imdec->buffer), 1);
			}
		stop_image_output( &imout );
	}
#ifdef HAVE_MMX
	mmx_off();
#endif
	stop_image_decoding( &imdec );

	SHOW_TIME("", started);
	return dst;
}

ASImage *
merge_layers( ASVisual *asv,
				ASImageLayer *layers, int count,
			  	unsigned int dst_width,
			  	unsigned int dst_height,
			  	ASAltImFormats out_format, unsigned int compression_out, int quality )
{
	ASImage *fake_bg = NULL ;
	ASImage *dst = NULL ;
	ASImageDecoder **imdecs ;
	ASImageOutput  *imout ;
	ASImageLayer *pcurr = layers;
	int i ;
	ASScanline dst_line ;
	START_TIME(started);

LOCAL_DEBUG_CALLER_OUT( "dst_width = %d, dst_height = %d", dst_width, dst_height );
	
	prepare_scanline( dst_width, QUANT_ERR_BITS, &dst_line, asv->BGR_mode );
	dst_line.flags = SCL_DO_ALL ;

	dst = create_destination_image( dst_width, dst_height, out_format, compression_out, ARGB32_DEFAULT_BACK_COLOR );
	if( dst == NULL )
		return NULL;

	imdecs = safecalloc( count+20, sizeof(ASImageDecoder*));

/*  don't really know why the hell we need that ???
 *  if( pcurr->im == NULL )
		pcurr->im = fake_bg = create_asimage( 1, 1, 0 );
 */
	for( i = 0 ; i < count ; i++ )
	{
		/* all laayers but first must have valid image or solid_color ! */
		if( (pcurr->im != NULL || pcurr->solid_color != 0 || i == 0) &&
			pcurr->dst_x < (int)dst_width && pcurr->dst_x+(int)pcurr->clip_width > 0 )
		{
			imdecs[i] = start_image_decoding(asv, pcurr->im, SCL_DO_ALL,
				                             pcurr->clip_x, pcurr->clip_y,
											 pcurr->clip_width, pcurr->clip_height,
											 pcurr->bevel);
			if( pcurr->bevel_width != 0 && pcurr->bevel_height != 0 )
				set_decoder_bevel_geom( imdecs[i],
				                        pcurr->bevel_x, pcurr->bevel_y,
										pcurr->bevel_width, pcurr->bevel_height );
  			if( pcurr->tint == 0 && i != 0 )
				set_decoder_shift( imdecs[i], 8 );
			if( pcurr->im == NULL )
				set_decoder_back_color( imdecs[i], pcurr->solid_color );
		}
		if( pcurr->next == pcurr )
			break;
		else
			pcurr = (pcurr->next!=NULL)?pcurr->next:pcurr+1 ;
	}
	if( i < count )
		count = i+1 ;
#ifdef HAVE_MMX
	mmx_init();
#endif

	if(imdecs[0] == NULL || (imout = start_image_output( asv, dst, out_format, QUANT_ERR_BITS, quality)) == NULL )
	{
		for( i = 0 ; i < count ; i++ )
			if( imdecs[i] )
				stop_image_decoding( &(imdecs[i]) );

        destroy_asimage( &dst );
    }else
	{
		int y, max_y = 0;
		int min_y = dst_height;
		int bg_tint = (layers[0].tint==0)?0x7F7F7F7F:layers[0].tint ;
		int bg_bottom = layers[0].dst_y+layers[0].clip_height+imdecs[0]->bevel_v_addon ;
LOCAL_DEBUG_OUT("blending actually...%s", "");
		pcurr = layers ;
		for( i = 0 ; i < count ; i++ )
		{
			if( imdecs[i] )
			{
				unsigned int layer_bottom = pcurr->dst_y+pcurr->clip_height ;
				if( pcurr->dst_y < min_y )
					min_y = pcurr->dst_y;
				layer_bottom += imdecs[i]->bevel_v_addon ;
				if( (int)layer_bottom > max_y )
					max_y = layer_bottom;
			}
			pcurr = (pcurr->next!=NULL)?pcurr->next:pcurr+1 ;
		}
		if( min_y < 0 )
			min_y = 0 ;
		else if( min_y >= (int)dst_height )
			min_y = dst_height ;
			
		if( max_y >= (int)dst_height )
			max_y = dst_height ;
		else
			imout->tiling_step = max_y ;

LOCAL_DEBUG_OUT( "min_y = %d, max_y = %d", min_y, max_y );
		dst_line.back_color = imdecs[0]->back_color ;
		dst_line.flags = 0 ;
		for( y = 0 ; y < min_y ; ++y  )
			imout->output_image_scanline( imout, &dst_line, 1);
		dst_line.flags = SCL_DO_ALL ;
		pcurr = layers ;
		for( i = 0 ; i < count ; ++i )
		{
			if( imdecs[i] && pcurr->dst_y < min_y  )
				imdecs[i]->next_line = min_y - pcurr->dst_y ;
			pcurr = (pcurr->next!=NULL)?pcurr->next:pcurr+1 ;
		}
		for( ; y < max_y ; ++y  )
		{
			if( layers[0].dst_y <= y && bg_bottom > y )
				imdecs[0]->decode_image_scanline( imdecs[0] );
			else
			{
				imdecs[0]->buffer.back_color = imdecs[0]->back_color ;
				imdecs[0]->buffer.flags = 0 ;
			}
			copytintpad_scanline( &(imdecs[0]->buffer), &dst_line, layers[0].dst_x, bg_tint );
			pcurr = layers[0].next?layers[0].next:&(layers[1]) ;
			for( i = 1 ; i < count ; i++ )
			{
				if( imdecs[i] && pcurr->dst_y <= y &&
					pcurr->dst_y+(int)pcurr->clip_height+(int)imdecs[i]->bevel_v_addon > y )
				{
					register ASScanline *b = &(imdecs[i]->buffer);
					CARD32 tint = pcurr->tint ;
					imdecs[i]->decode_image_scanline( imdecs[i] );
					if( tint != 0 )
					{
						tint_component_mod( b->red,   (CARD16)(ARGB32_RED8(tint)<<1),   b->width );
						tint_component_mod( b->green, (CARD16)(ARGB32_GREEN8(tint)<<1), b->width );
  					   	tint_component_mod( b->blue,  (CARD16)(ARGB32_BLUE8(tint)<<1),  b->width );
					  	tint_component_mod( b->alpha, (CARD16)(ARGB32_ALPHA8(tint)<<1), b->width );
					}
					pcurr->merge_scanlines( &dst_line, b, pcurr->dst_x );
				}
				pcurr = (pcurr->next!=NULL)?pcurr->next:pcurr+1 ;
			}
			imout->output_image_scanline( imout, &dst_line, 1);
		}
		dst_line.back_color = imdecs[0]->back_color ;
		dst_line.flags = 0 ;
		for( ; y < (int)dst_height ; y++  )
			imout->output_image_scanline( imout, &dst_line, 1);
		stop_image_output( &imout );
	}
#ifdef HAVE_MMX
	mmx_off();
#endif
	for( i = 0 ; i < count ; i++ )
		if( imdecs[i] != NULL )
		{
			stop_image_decoding( &(imdecs[i]) );
		}
	free( imdecs );
	if( fake_bg )
	{
		if( layers[0].im == fake_bg )
			layers[0].im = NULL ;
		destroy_asimage( &fake_bg );
	}
	free_scanline( &dst_line, True );
	SHOW_TIME("", started);
	return dst;
}

/* **************************************************************************************/
/* GRADIENT drawing : 																   */
/* **************************************************************************************/
static void
make_gradient_left2right( ASImageOutput *imout, ASScanline *dither_lines, int dither_lines_num, ASFlagType filter )
{
	int line ;

	imout->tiling_step = dither_lines_num;
	for( line = 0 ; line < dither_lines_num ; line++ )
		imout->output_image_scanline( imout, &(dither_lines[line]), 1);
}

static void
make_gradient_top2bottom( ASImageOutput *imout, ASScanline *dither_lines, int dither_lines_num, ASFlagType filter )
{
	int y, height = imout->im->height, width = imout->im->width ;
	int line ;
	ASScanline result;
	CARD32 chan_data[MAX_GRADIENT_DITHER_LINES] = {0,0,0,0};
LOCAL_DEBUG_CALLER_OUT( "width = %d, height = %d, filetr = 0x%lX, dither_count = %d\n", width, height, filter, dither_lines_num );
	prepare_scanline( width, QUANT_ERR_BITS, &result, imout->asv->BGR_mode );
	for( y = 0 ; y < height ; y++ )
	{
		int color ;

		result.flags = 0 ;
		result.back_color = ARGB32_DEFAULT_BACK_COLOR ;
		LOCAL_DEBUG_OUT( "line: %d", y );
		for( color = 0 ; color < IC_NUM_CHANNELS ; color++ )
			if( get_flags( filter, 0x01<<color ) )
			{
				Bool dithered = False ;
				for( line = 0 ; line < dither_lines_num ; line++ )
				{
					/* we want to do error diffusion here since in other places it only works
						* in horisontal direction : */
					CARD32 c = dither_lines[line].channels[color][y] + ((dither_lines[line].channels[color][y+1]&0xFF)>>1);
					if( (c&0xFFFF0000) != 0 )
						chan_data[line] = ( c&0x7F000000 )?0:0x0000FF00;
					else
						chan_data[line] = c ;

					if( chan_data[line] != chan_data[0] )
						dithered = True;
				}
				LOCAL_DEBUG_OUT( "channel: %d. Dithered ? %d", color, dithered );

				if( !dithered )
				{
					result.back_color = (result.back_color&(~MAKE_ARGB32_CHAN8(0xFF,color)))|
										MAKE_ARGB32_CHAN16(chan_data[0],color);
					LOCAL_DEBUG_OUT( "back_color = %8.8lX", result.back_color);
				}else
				{
					register CARD32  *dst = result.channels[color] ;
					for( line = 0 ; line  < dither_lines_num ; line++ )
					{
						register int x ;
						register CARD32 d = chan_data[line] ;
						for( x = line ; x < width ; x+=dither_lines_num )
						{
							dst[x] = d ;
						}
					}
					set_flags(result.flags, 0x01<<color);
				}
			}
		imout->output_image_scanline( imout, &result, 1);
	}
	free_scanline( &result, True );
}

static void
make_gradient_diag_width( ASImageOutput *imout, ASScanline *dither_lines, int dither_lines_num, ASFlagType filter, Bool from_bottom )
{
	int line = 0;
	/* using bresengham algorithm again to trigger horizontal shift : */
	unsigned short smaller = imout->im->height;
	unsigned short bigger  = imout->im->width;
	register int i = 0;
	int eps;
LOCAL_DEBUG_CALLER_OUT( "width = %d, height = %d, filetr = 0x%lX, dither_count = %d, dither width = %d\n", bigger, smaller, filter, dither_lines_num, dither_lines[0].width );

	if( from_bottom )
		toggle_image_output_direction( imout );
	eps = -(bigger>>1);
	for ( i = 0 ; i < bigger ; i++ )
	{
		eps += smaller;
		if( (eps << 1) >= bigger )
		{
			/* put scanline with the same x offset */
			dither_lines[line].offset_x = i ;
			imout->output_image_scanline( imout, &(dither_lines[line]), 1);
			if( ++line >= dither_lines_num )
				line = 0;
			eps -= bigger ;
		}
	}
}

static void
make_gradient_diag_height( ASImageOutput *imout, ASScanline *dither_lines, int dither_lines_num, ASFlagType filter, Bool from_bottom )
{
	int line = 0;
	unsigned short width = imout->im->width, height = imout->im->height ;
	/* using bresengham algorithm again to trigger horizontal shift : */
	unsigned short smaller = width;
	unsigned short bigger  = height;
	register int i = 0, k =0;
	int eps;
	ASScanline result;
	int *offsets ;

	prepare_scanline( width, QUANT_ERR_BITS, &result, imout->asv->BGR_mode );
	offsets = safemalloc( sizeof(int)*width );
	offsets[0] = 0 ;

	eps = -(bigger>>1);
	for ( i = 0 ; i < bigger ; i++ )
	{
		++offsets[k];
		eps += smaller;
		if( (eps << 1) >= bigger )
		{
			++k ;
			if( k < width )
				offsets[k] = offsets[k-1] ; /* seeding the offset */
			eps -= bigger ;
		}
	}

	if( from_bottom )
		toggle_image_output_direction( imout );

	result.flags = (filter&SCL_DO_ALL);
	if( (filter&SCL_DO_ALL) == SCL_DO_ALL )
	{
		for( i = 0 ; i < height ; i++ )
		{
			for( k = 0 ; k < width ; k++ )
			{
				int offset = i+offsets[k] ;
				CARD32 **src_chan = &(dither_lines[line].channels[0]) ;
				result.alpha[k] = src_chan[IC_ALPHA][offset] ;
				result.red  [k] = src_chan[IC_RED]  [offset] ;
				result.green[k] = src_chan[IC_GREEN][offset] ;
				result.blue [k] = src_chan[IC_BLUE] [offset] ;
				if( ++line >= dither_lines_num )
					line = 0 ;
			}
			imout->output_image_scanline( imout, &result, 1);
		}
	}else
	{
		for( i = 0 ; i < height ; i++ )
		{
			for( k = 0 ; k < width ; k++ )
			{
				int offset = i+offsets[k] ;
				CARD32 **src_chan = &(dither_lines[line].channels[0]) ;
				if( get_flags(filter, SCL_DO_ALPHA) )
					result.alpha[k] = src_chan[IC_ALPHA][offset] ;
				if( get_flags(filter, SCL_DO_RED) )
					result.red[k]   = src_chan[IC_RED]  [offset] ;
				if( get_flags(filter, SCL_DO_GREEN) )
					result.green[k] = src_chan[IC_GREEN][offset] ;
				if( get_flags(filter, SCL_DO_BLUE) )
					result.blue[k]  = src_chan[IC_BLUE] [offset] ;
				if( ++line >= dither_lines_num )
					line = 0 ;
			}
			imout->output_image_scanline( imout, &result, 1);
		}
	}

	free( offsets );
	free_scanline( &result, True );
}

static ARGB32
get_best_grad_back_color( ASGradient *grad )
{
	ARGB32 back_color = 0 ;
	int chan ;
	for( chan = 0 ; chan < IC_NUM_CHANNELS ; ++chan )
	{
		CARD8 best = 0;
		unsigned int best_size = 0;
		register int i = grad->npoints;
		while( --i > 0 )
		{ /* very crude algorithm, detecting biggest spans of the same color :*/
			CARD8 c = ARGB32_CHAN8(grad->color[i], chan );
			unsigned int span = grad->color[i]*20000;
			if( c == ARGB32_CHAN8(grad->color[i-1], chan ) )
			{
				span -= grad->color[i-1]*2000;
				if( c == best )
					best_size += span ;
				else if( span > best_size )
				{
					best_size = span ;
					best = c ;
				}
			}
		}
		back_color |= MAKE_ARGB32_CHAN8(best,chan);
	}
	return back_color;
}

ASImage*
make_gradient( ASVisual *asv, ASGradient *grad,
               unsigned int width, unsigned int height, ASFlagType filter,
  			   ASAltImFormats out_format, unsigned int compression_out, int quality  )
{
	ASImage *im = NULL ;
	ASImageOutput *imout;
	int line_len = width;
	START_TIME(started);
LOCAL_DEBUG_CALLER_OUT( "type = 0x%X, width=%d, height = %d, filter = 0x%lX", grad->type, width, height, filter );
	if( asv == NULL || grad == NULL )
		return NULL;
	if( width == 0 )
		width = 2;
 	if( height == 0 )
		height = 2;

	im = create_destination_image( width, height, out_format, compression_out, get_best_grad_back_color( grad ) );

	if( get_flags(grad->type,GRADIENT_TYPE_ORIENTATION) )
		line_len = height ;
	if( get_flags(grad->type,GRADIENT_TYPE_DIAG) )
		line_len = MAX(width,height)<<1 ;
	if((imout = start_image_output( asv, im, out_format, QUANT_ERR_BITS, quality)) == NULL )
	{
        destroy_asimage( &im );
    }else
	{
		int dither_lines = MIN(imout->quality+1, MAX_GRADIENT_DITHER_LINES) ;
		ASScanline *lines;
		int line;
		static ARGB32 dither_seeds[MAX_GRADIENT_DITHER_LINES] = { 0, 0xFFFFFFFF, 0x7F0F7F0F, 0x0F7F0F7F };

		if( dither_lines > (int)im->height || dither_lines > (int)im->width )
			dither_lines = MIN(im->height, im->width) ;

		lines = safecalloc( dither_lines, sizeof(ASScanline));
		for( line = 0 ; line < dither_lines ; line++ )
		{
			prepare_scanline( line_len, QUANT_ERR_BITS, &(lines[line]), asv->BGR_mode );
			make_gradient_scanline( &(lines[line]), grad, filter, dither_seeds[line] );
		}
		switch( get_flags(grad->type,GRADIENT_TYPE_MASK) )
		{
			case GRADIENT_Left2Right :
				make_gradient_left2right( imout, lines, dither_lines, filter );
  	    		break ;
			case GRADIENT_Top2Bottom :
				make_gradient_top2bottom( imout, lines, dither_lines, filter );
				break ;
			case GRADIENT_TopLeft2BottomRight :
			case GRADIENT_BottomLeft2TopRight :
				if( width >= height )
					make_gradient_diag_width( imout, lines, dither_lines, filter,
											 (grad->type==GRADIENT_BottomLeft2TopRight));
				else
					make_gradient_diag_height( imout, lines, dither_lines, filter,
											  (grad->type==GRADIENT_BottomLeft2TopRight));
				break ;
			default:
				break;
		}
		stop_image_output( &imout );
		for( line = 0 ; line < dither_lines ; line++ )
			free_scanline( &(lines[line]), True );
		free( lines );
	}
	SHOW_TIME("", started);
	return im;
}

/* ***************************************************************************/
/* Image flipping(rotation)													*/
/* ***************************************************************************/
ASImage *
flip_asimage( ASVisual *asv, ASImage *src,
		      int offset_x, int offset_y,
			  unsigned int to_width,
			  unsigned int to_height,
			  int flip,
			  ASAltImFormats out_format, unsigned int compression_out, int quality )
{
	ASImage *dst = NULL ;
	ASImageOutput  *imout ;
	ASFlagType filter = SCL_DO_ALL;
	START_TIME(started);

LOCAL_DEBUG_CALLER_OUT( "offset_x = %d, offset_y = %d, to_width = %d, to_height = %d", offset_x, offset_y, to_width, to_height );
	if( src )
		filter = get_asimage_chanmask(src);

	dst = create_destination_image( to_width, to_height, out_format, compression_out, src->back_color);

#ifdef HAVE_MMX
	mmx_init();
#endif
	if((imout = start_image_output( asv, dst, out_format, 0, quality)) == NULL )
	{
        destroy_asimage( &dst );
    }else
	{
		ASImageDecoder *imdec ;
		ASScanline result ;
		int y;
LOCAL_DEBUG_OUT("flip-flopping actually...%s", "");
		prepare_scanline( to_width, 0, &result, asv->BGR_mode );
		if( (imdec = start_image_decoding(asv, src, filter, offset_x, offset_y,
		                                  get_flags( flip, FLIP_VERTICAL )?to_height:to_width,
										  get_flags( flip, FLIP_VERTICAL )?to_width:to_height, NULL)) != NULL )
		{
            if( get_flags( flip, FLIP_VERTICAL ) )
			{
				CARD32 *chan_data ;
				size_t  pos = 0;
				int x ;
				CARD32 *a = imdec->buffer.alpha ;
				CARD32 *r = imdec->buffer.red ;
				CARD32 *g = imdec->buffer.green ;
				CARD32 *b = imdec->buffer.blue;

				chan_data = safemalloc( to_width*to_height*sizeof(CARD32));
                result.back_color = src->back_color;
				result.flags = filter ;
/*				memset( a, 0x00, to_height*sizeof(CARD32));
				memset( r, 0x00, to_height*sizeof(CARD32));
				memset( g, 0x00, to_height*sizeof(CARD32));
				memset( b, 0x00, to_height*sizeof(CARD32));
  */			for( y = 0 ; y < (int)to_width ; y++ )
				{
					imdec->decode_image_scanline( imdec );
					for( x = 0; x < (int)to_height ; x++ )
					{
						chan_data[pos++] = MAKE_ARGB32( a[x],r[x],g[x],b[x] );
					}
				}

				if( get_flags( flip, FLIP_UPSIDEDOWN ) )
				{
					for( y = 0 ; y < (int)to_height ; ++y )
					{
						pos = y + (int)(to_width-1)*(to_height) ;
						for( x = 0 ; x < (int)to_width ; ++x )
						{
							result.alpha[x] = ARGB32_ALPHA8(chan_data[pos]);
							result.red  [x] = ARGB32_RED8(chan_data[pos]);
							result.green[x] = ARGB32_GREEN8(chan_data[pos]);
							result.blue [x] = ARGB32_BLUE8(chan_data[pos]);
							pos -= to_height ;
						}
						imout->output_image_scanline( imout, &result, 1);
					}
				}else
				{
					for( y = to_height-1 ; y >= 0 ; --y )
					{
						pos = y ;
						for( x = 0 ; x < (int)to_width ; ++x )
						{
							result.alpha[x] = ARGB32_ALPHA8(chan_data[pos]);
							result.red  [x] = ARGB32_RED8(chan_data[pos]);
							result.green[x] = ARGB32_GREEN8(chan_data[pos]);
							result.blue [x] = ARGB32_BLUE8(chan_data[pos]);
							pos += to_height ;
						}
						imout->output_image_scanline( imout, &result, 1);
					}
				}
				free( chan_data );
			}else
			{
				toggle_image_output_direction( imout );
/*                fprintf( stderr, __FUNCTION__":chanmask = 0x%lX", filter ); */
				for( y = 0 ; y < (int)to_height ; y++  )
				{
					imdec->decode_image_scanline( imdec );
                    result.flags = imdec->buffer.flags = imdec->buffer.flags & filter ;
                    result.back_color = imdec->buffer.back_color ;
                    SCANLINE_FUNC_FILTERED(reverse_component,imdec->buffer,result,0,to_width);
					imout->output_image_scanline( imout, &result, 1);
				}
			}
			stop_image_decoding( &imdec );
		}
		free_scanline( &result, True );
		stop_image_output( &imout );
	}
#ifdef HAVE_MMX
	mmx_off();
#endif
	SHOW_TIME("", started);
	return dst;
}

ASImage *
mirror_asimage( ASVisual *asv, ASImage *src,
		      int offset_x, int offset_y,
			  unsigned int to_width,
			  unsigned int to_height,
			  Bool vertical, ASAltImFormats out_format,
			  unsigned int compression_out, int quality )
{
	ASImage *dst = NULL ;
	ASImageOutput  *imout ;
	START_TIME(started);

	LOCAL_DEBUG_CALLER_OUT( "offset_x = %d, offset_y = %d, to_width = %d, to_height = %d", offset_x, offset_y, to_width, to_height );
	dst = create_destination_image( to_width, to_height, out_format, compression_out, src->back_color);

#ifdef HAVE_MMX
	mmx_init();
#endif
	if((imout = start_image_output( asv, dst, out_format, 0, quality)) == NULL )
	{
        destroy_asimage( &dst );
    }else
	{
		ASImageDecoder *imdec ;
		ASScanline result ;
		int y;
		if( !vertical )
			prepare_scanline( to_width, 0, &result, asv->BGR_mode );
LOCAL_DEBUG_OUT("miroring actually...%s", "");
		if( (imdec = start_image_decoding(asv, src, SCL_DO_ALL, offset_x, offset_y,
		                                  to_width, to_height, NULL)) != NULL )
		{
			if( vertical )
			{
				toggle_image_output_direction( imout );
				for( y = 0 ; y < (int)to_height ; y++  )
				{
					imdec->decode_image_scanline( imdec );
					imout->output_image_scanline( imout, &(imdec->buffer), 1);
				}
			}else
			{
				for( y = 0 ; y < (int)to_height ; y++  )
				{
					imdec->decode_image_scanline( imdec );
					result.flags = imdec->buffer.flags ;
					result.back_color = imdec->buffer.back_color ;
					SCANLINE_FUNC(reverse_component,imdec->buffer,result,0,to_width);
					imout->output_image_scanline( imout, &result, 1);
				}
			}
			stop_image_decoding( &imdec );
		}
		if( !vertical )
			free_scanline( &result, True );
		stop_image_output( &imout );
	}
#ifdef HAVE_MMX
	mmx_off();
#endif
	SHOW_TIME("", started);
	return dst;
}

ASImage *
pad_asimage(  ASVisual *asv, ASImage *src,
		      int dst_x, int dst_y,
			  unsigned int to_width,
			  unsigned int to_height,
			  ARGB32 color,
			  ASAltImFormats out_format,
			  unsigned int compression_out, int quality )
{
	ASImage *dst = NULL ;
	ASImageOutput  *imout ;
	int clip_width, clip_height ;
	START_TIME(started);

LOCAL_DEBUG_CALLER_OUT( "dst_x = %d, dst_y = %d, to_width = %d, to_height = %d", dst_x, dst_y, to_width, to_height );
	if( src == NULL )
		return NULL ;

	if( to_width == src->width && to_height == src->height && dst_x == 0 && dst_y == 0 )
		return clone_asimage( src, SCL_DO_ALL );

	dst = create_destination_image( to_width, to_height, out_format, compression_out, src->back_color);

	clip_width = src->width ;
	clip_height = src->height ;
	if( dst_x < 0 )
		clip_width = MIN( (int)to_width, dst_x+clip_width );
	else
		clip_width = MIN( (int)to_width-dst_x, clip_width );
    if( dst_y < 0 )
		clip_height = MIN( (int)to_height, dst_y+clip_height);
	else
		clip_height = MIN( (int)to_height-dst_y, clip_height);
	if( (clip_width <= 0 || clip_height <= 0) )
	{                              /* we are completely outside !!! */
		dst->back_color = color ;
		return dst ;
	}

#ifdef HAVE_MMX
	mmx_init();
#endif
	if((imout = start_image_output( asv, dst, out_format, 0, quality)) == NULL )
	{
        destroy_asimage( &dst );
    }else
	{
		ASImageDecoder *imdec = NULL;
		ASScanline result ;
		int y;
		int start_x = (dst_x < 0)? 0: dst_x;
		int start_y = (dst_y < 0)? 0: dst_y;

		if( (int)to_width != clip_width || clip_width != (int)src->width )
		{
			prepare_scanline( to_width, 0, &result, asv->BGR_mode );
			imdec = start_image_decoding(  asv, src, SCL_DO_ALL,
			                               (dst_x<0)? -dst_x:0,
										   (dst_y<0)? -dst_y:0,
		                                    clip_width, clip_height, NULL);
		}

		result.back_color = color ;
		result.flags = 0 ;
LOCAL_DEBUG_OUT( "filling %d lines with %8.8lX", start_y, color );
		for( y = 0 ; y < start_y ; y++  )
			imout->output_image_scanline( imout, &result, 1);

		if( imdec )
			result.back_color = imdec->buffer.back_color ;
		if( (int)to_width == clip_width )
		{
			if( (int)clip_width == (int)src->width )
			{
LOCAL_DEBUG_OUT( "copiing %d lines", clip_height );
				copy_asimage_lines( dst, start_y, src, (dst_y < 0 )? -dst_y: 0, clip_height, SCL_DO_ALL );
				imout->next_line += clip_height ;
			}else
				for( y = 0 ; y < clip_height ; y++  )
				{
					imdec->decode_image_scanline( imdec );
					imout->output_image_scanline( imout, &(imdec->buffer), 1);
				}
		}else
		{
			for( y = 0 ; y < clip_height ; y++  )
			{
				int chan ;

				imdec->decode_image_scanline( imdec );
				result.flags = imdec->buffer.flags ;
				for( chan = 0 ; chan < IC_NUM_CHANNELS ; ++chan )
				{
	   				register CARD32 *chan_data = result.channels[chan] ;
	   				register CARD32 *src_chan_data = imdec->buffer.channels[chan]+((dst_x<0)? -dst_x : 0) ;
					CARD32 chan_val = ARGB32_CHAN8(color, chan);
					register int k = -1;
					for( k = 0 ; k < start_x ; ++k )
						chan_data[k] = chan_val ;
					chan_data += k ;
					for( k = 0 ; k < clip_width ; ++k )
						chan_data[k] = src_chan_data[k];
					chan_data += k ;
					k = to_width-(start_x+clip_width) ;
					while( --k >= 0 )
						chan_data[k] = chan_val ;
				}
				imout->output_image_scanline( imout, &result, 1);
			}
		}
		result.back_color = color ;
		result.flags = 0 ;
LOCAL_DEBUG_OUT( "filling %d lines with %8.8lX at the end", to_height-(start_y+clip_height), color );
		for( y = start_y+clip_height ; y < (int)to_height ; y++  )
			imout->output_image_scanline( imout, &result, 1);

		if( (int)to_width != clip_width || clip_width != (int)src->width )
		{
			stop_image_decoding( &imdec );
			free_scanline( &result, True );
		}
		stop_image_output( &imout );
	}
#ifdef HAVE_MMX
	mmx_off();
#endif
	SHOW_TIME("", started);
	return dst;
}


/**********************************************************************/

Bool fill_asimage( ASVisual *asv, ASImage *im,
               	   int x, int y, int width, int height,
				   ARGB32 color )
{
	ASImageOutput *imout;
	ASImageDecoder *imdec;
	START_TIME(started);

	if( asv == NULL || im == NULL )
		return False;
	if( x < 0 )
	{	width += x ; x = 0 ; }
	if( y < 0 )
	{	height += y ; y = 0 ; }

	if( width <= 0 || height <= 0 || x >= (int)im->width || y >= (int)im->height )
		return False;
	if( x+width > (int)im->width )
		width = (int)im->width-x ;
	if( y+height > (int)im->height )
		height = (int)im->height-y ;

	if((imout = start_image_output( asv, im, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT)) == NULL )
		return False ;
	else
	{
		int i ;
		imout->next_line = y ;
		if( x == 0 && width == (int)im->width )
		{
			ASScanline result ;
			result.flags = 0 ;
			result.back_color = color ;
			for( i = 0 ; i < height ; i++ )
				imout->output_image_scanline( imout, &result, 1);
		}else if ((imdec = start_image_decoding(asv, im, SCL_DO_ALL, 0, y, im->width, height, NULL)) != NULL )
		{
			CARD32 alpha = ARGB32_ALPHA8(color), red = ARGB32_RED8(color),
				   green = ARGB32_GREEN8(color), blue = ARGB32_BLUE8(color);
			CARD32 	*a = imdec->buffer.alpha + x ; 
			CARD32 	*r = imdec->buffer.red + x ;
			CARD32 	*g = imdec->buffer.green + x ;
			CARD32 	*b = imdec->buffer.blue + x  ;
			for( i = 0 ; i < height ; i++ )
			{
				register int k ;
				imdec->decode_image_scanline( imdec );
				for( k = 0 ; k < width ; ++k )
				{
					a[k] = alpha ;
					r[k] = red ;
					g[k] = green ;
					b[k] = blue ;
				}
				imout->output_image_scanline( imout, &(imdec->buffer), 1);
			}
			stop_image_decoding( &imdec );
		}
	}
	stop_image_output( &imout );
	SHOW_TIME("", started);
	return True;
}

/* ********************************************************************************/
/* Vector -> ASImage functions :                                                  */
/* ********************************************************************************/
Bool
colorize_asimage_vector( ASVisual *asv, ASImage *im,
						 ASVectorPalette *palette,
						 ASAltImFormats out_format,
						 int quality )
{
	ASImageOutput  *imout = NULL ;
	ASScanline buf ;
	int x, y, curr_point, last_point ;
    register double *vector ;
	double *points ;
	double *multipliers[IC_NUM_CHANNELS] ;
	START_TIME(started);

	if( im == NULL || palette == NULL || out_format == ASA_Vector )
		return False;

	if( im->alt.vector == NULL )
		return False;
	vector = im->alt.vector ;

	if((imout = start_image_output( asv, im, out_format, QUANT_ERR_BITS, quality)) == NULL )
		return False;
	/* as per ROOT ppl request double data goes from bottom to top,
	 * instead of from top to bottom : */
	if( !get_flags( im->flags, ASIM_VECTOR_TOP2BOTTOM) )
		toggle_image_output_direction(imout);

	prepare_scanline( im->width, QUANT_ERR_BITS, &buf, asv->BGR_mode );
	curr_point = palette->npoints/2 ;
	points = palette->points ;
	last_point = palette->npoints-1 ;
	buf.flags = 0 ;
	for( y = 0 ; y < IC_NUM_CHANNELS ; ++y )
	{
		if( palette->channels[y] )
		{
			multipliers[y] = safemalloc( last_point*sizeof(double));
			for( x = 0 ; x < last_point ; ++x )
			{
				if (points[x+1] == points[x])
      				multipliers[y][x] = 1;
				else
					multipliers[y][x] = (double)(palette->channels[y][x+1] - palette->channels[y][x])/
				                 	        (points[x+1]-points[x]);
/*				fprintf( stderr, "%e-%e/%e-%e=%e ", (double)palette->channels[y][x+1], (double)palette->channels[y][x],
				                 	        points[x+1], points[x], multipliers[y][x] );
 */
			}
/*			fputc( '\n', stderr ); */
			set_flags(buf.flags, (0x01<<y));
		}else
			multipliers[y] = NULL ;
	}
	for( y = 0 ; y < (int)im->height ; ++y )
	{
		for( x = 0 ; x < (int)im->width ;)
		{
			register int i = IC_NUM_CHANNELS ;
			double d ;

			if( points[curr_point] > vector[x] )
			{
				while( --curr_point >= 0 )
					if( points[curr_point] < vector[x] )
						break;
				if( curr_point < 0 )
					++curr_point ;
			}else
			{
				while( points[curr_point+1] < vector[x] )
					if( ++curr_point >= last_point )
					{
						curr_point = last_point-1 ;
						break;
					}
			}
			d = vector[x]-points[curr_point];
/*			fprintf( stderr, "%f|%d|%f*%f=%d(%f)+%d=", vector[x], curr_point, d, multipliers[0][curr_point], (int)(d*multipliers[0][curr_point]),(d*multipliers[0][curr_point]) , palette->channels[0][curr_point] ); */
			while( --i >= 0 )
				if( multipliers[i] )
				{/* the following calculation is the most expensive part of the algorithm : */
					buf.channels[i][x] = (int)(d*multipliers[i][curr_point])+palette->channels[i][curr_point] ;
/*					fprintf( stderr, "%2.2X.", buf.channels[i][x] ); */
				}
/*			fputc( ' ', stderr ); */
#if 1
			while( ++x < (int)im->width )
				if( vector[x] == vector[x-1] )
				{
					buf.red[x] = buf.red[x-1] ;
					buf.green[x] = buf.green[x-1] ;
					buf.blue[x] = buf.blue[x-1] ;
					buf.alpha[x] = buf.alpha[x-1] ;
				}else
					break;
#else
			++x ;
#endif
		}
/*		fputc( '\n', stderr ); */
		imout->output_image_scanline( imout, &buf, 1);
		vector += im->width ;
	}
	for( y = 0 ; y < IC_NUM_CHANNELS ; ++y )
		if( multipliers[y] )
			free(multipliers[y]);

	stop_image_output( &imout );
	free_scanline( &buf, True );
	SHOW_TIME("", started);
	return True;
}

ASImage *
create_asimage_from_vector( ASVisual *asv, double *vector,
							unsigned int width, unsigned int height,
							ASVectorPalette *palette,
							ASAltImFormats out_format,
							unsigned int compression, int quality )
{
	ASImage *im = NULL;

	if( vector != NULL )
	{
		im = create_destination_image( width, height, out_format, compression, ARGB32_DEFAULT_BACK_COLOR);

		if( im != NULL )
		{
			if( set_asimage_vector( im, vector ) )
				if( palette )
					colorize_asimage_vector( asv, im, palette, out_format, quality );
		}
	}
	return im ;
}


/***********************************************************************
 * Gaussian blur code.
 **********************************************************************/

#undef PI
#define PI 3.141592526

static void calc_gauss(double radius, double* gauss);

static int radius;

static inline void
gauss_component(CARD32 *src, CARD32 *dst, double* gauss, int len)
{
	int x, j, r = radius - 1;
	for (x = 0 ; x < len ; x++) {
		register double v = 0.0;
		for (j = x - r ; j <= 0 ; j++) v += src[0] * gauss[x - j];
		for ( ; j < x ; j++) v += src[j] * gauss[x - j];
		v += src[x] * gauss[0];
		for (j = x + r ; j >= len ; j--) v += src[len - 1] * gauss[j - x];
		for ( ; j > x ; j--) v += src[j] * gauss[j - x];
		dst[x] = (CARD32)v;
	}
}

ASImage* blur_asimage_gauss(ASVisual* asv, ASImage* src, double horz, double vert, 
                            ASFlagType filter, 
							ASAltImFormats out_format, unsigned int compression_out, int quality)
{
	ASImage *dst = NULL;
	ASImageOutput *imout;
	ASImageDecoder *imdec;
	ASScanline result;
	int y;
	double* gauss = NULL;

	if (!src) return NULL;

	dst = create_destination_image( src->width, src->height, out_format, compression_out, src->back_color);

#ifdef HAVE_MMX
	mmx_init();
#endif

	imout = start_image_output(asv, dst, out_format, 0, quality);
    if (!imout)
    {
        destroy_asimage( &dst );
#ifdef HAVE_MMX
		mmx_off();
#endif
		return NULL;
	}

	imdec = start_image_decoding(asv, src, SCL_DO_ALL, 0, 0, dst->width, dst->height, NULL);
	if (!imdec) {
        destroy_asimage( &dst );
#ifdef HAVE_MMX
		mmx_off();
#endif
		return NULL;
	}

	gauss = safecalloc(MAX((int)horz, (int)vert), sizeof(double));

	/* First the horizontal pass. */
	if (horz >= 1.0) {
		/* My version. -Ethan */
		radius = (int)horz;
		calc_gauss(horz, gauss);
	}

	prepare_scanline(dst->width, 0, &result, asv->BGR_mode);

    for (y = 0 ; y < (int)dst->height ; y++)
    {
        ASFlagType lf = imdec->buffer.flags&filter ;
		imdec->decode_image_scanline(imdec);
		result.flags = imdec->buffer.flags;
		result.back_color = imdec->buffer.back_color;
        if( get_flags( lf, SCL_DO_RED ) )
            gauss_component( imdec->buffer.red, result.red, gauss, dst->width);
        else if( get_flags( result.flags, SCL_DO_RED ) )
            copy_component( imdec->buffer.red, result.red, 0, dst->width);
        if( get_flags( lf, SCL_DO_GREEN ) )
            gauss_component( imdec->buffer.green, result.green, gauss, dst->width);
        else if( get_flags( result.flags, SCL_DO_GREEN ) )
            copy_component( imdec->buffer.green, result.green, 0, dst->width);
        if( get_flags( lf, SCL_DO_BLUE ) )
            gauss_component( imdec->buffer.blue, result.blue, gauss, dst->width);
        else if( get_flags( result.flags, SCL_DO_BLUE ) )
            copy_component( imdec->buffer.blue, result.blue, 0, dst->width);
        if( get_flags( lf, SCL_DO_ALPHA ) )
            gauss_component( imdec->buffer.alpha, result.alpha, gauss, dst->width);
        else if( get_flags( result.flags, SCL_DO_ALPHA ) )
            copy_component( imdec->buffer.alpha, result.alpha, 0, dst->width);

        imout->output_image_scanline(imout, &result, 1);
	}

	stop_image_decoding(&imdec);
	free_scanline(&result, True);
	stop_image_output(&imout);

	free(gauss);

#ifdef HAVE_MMX
	mmx_off();
#endif

	return dst;
}

static void calc_gauss(double radius, double* gauss) {
	int i;
	double n, std_dev, sum = 0.0;
	if (radius <= 1.0) {
		gauss[0] = 1.0;
		return;
	}
	if (radius > 10.0) radius = 10.0;
	std_dev = (radius - 1) * 0.3003866304;
	n = 2 * std_dev * std_dev;
	for (i = 0 ; i < radius ; i++) {
		gauss[i] = exp(-i * i / n);
		sum += gauss[i] + gauss[i];
	}
	sum -= gauss[0];
	for (i = 0 ; i < radius ; i++) gauss[i] /= sum;
}

/***********************************************************************
 * Hue,saturation and lightness adjustments.
 **********************************************************************/
ASImage*
adjust_asimage_hsv( ASVisual *asv, ASImage *src,
				    int offset_x, int offset_y,
	  			    unsigned int to_width, unsigned int to_height,
					unsigned int affected_hue, unsigned int affected_radius,
					int hue_offset, int saturation_offset, int value_offset,
					ASAltImFormats out_format,
					unsigned int compression_out, int quality )
{
	ASImage *dst = NULL ;
	ASImageDecoder *imdec ;
	ASImageOutput  *imout ;
	START_TIME(started);

LOCAL_DEBUG_CALLER_OUT( "offset_x = %d, offset_y = %d, to_width = %d, to_height = %d, hue = %u", offset_x, offset_y, to_width, to_height, affected_hue );
	if( src && (imdec = start_image_decoding(asv, src, SCL_DO_ALL, offset_x, offset_y, to_width, 0, NULL)) == NULL )
		return NULL;

	dst = create_destination_image( to_width, to_height, out_format, compression_out, src->back_color);
#ifdef HAVE_MMX
	mmx_init();
#endif
	set_decoder_shift(imdec,8);
	if((imout = start_image_output( asv, dst, out_format, 8, quality)) == NULL )
	{
        destroy_asimage( &dst );
    }else
	{
	    CARD32 from_hue1 = 0, from_hue2 = 0, to_hue1 = 0, to_hue2 = 0 ;
		int y, max_y = to_height;

		affected_hue = normalize_degrees_val( affected_hue );
		affected_radius = normalize_degrees_val( affected_radius );
		if( affected_hue > affected_radius )
		{
			from_hue1 = degrees2hue16(affected_hue-affected_radius);
			if( affected_hue+affected_radius >= 360 )
			{
				to_hue1 = MAX_HUE16 ;
				from_hue2 = MIN_HUE16 ;
				to_hue2 = degrees2hue16(affected_hue+affected_radius-360);
			}else
				to_hue1 = degrees2hue16(affected_hue+affected_radius);
		}else
		{
			from_hue1 = degrees2hue16(affected_hue+360-affected_radius);
			to_hue1 = MAX_HUE16 ;
			from_hue2 = MIN_HUE16 ;
			to_hue2 = degrees2hue16(affected_hue+affected_radius);
		}
		hue_offset = degrees2hue16(hue_offset);
		saturation_offset = (saturation_offset<<16) / 100;
		value_offset = (value_offset<<16)/100 ;
LOCAL_DEBUG_OUT("adjusting actually...%s", "");
		if( to_height > src->height )
		{
			imout->tiling_step = src->height ;
			max_y = src->height ;
		}
		for( y = 0 ; y < max_y ; y++  )
		{
			register int x = imdec->buffer.width;
			CARD32 *r = imdec->buffer.red;
			CARD32 *g = imdec->buffer.green;
			CARD32 *b = imdec->buffer.blue ;
			long h, s, v ;
			imdec->decode_image_scanline( imdec );
			while( --x >= 0 )
			{
				if( (h = rgb2hue( r[x], g[x], b[x] )) != 0 )
				{
#ifdef DEBUG_HSV_ADJUSTMENT
					fprintf( stderr, "IN  %d: rgb = #%4.4lX.%4.4lX.%4.4lX hue = %ld(%d)        range is (%ld - %ld, %ld - %ld), dh = %d\n", __LINE__, r[x], g[x], b[x], h, ((h>>8)*360)>>8, from_hue1, to_hue1, from_hue2, to_hue2, hue_offset );
#endif

					if( affected_radius >= 180 ||
						(h >= (int)from_hue1 && h <= (int)to_hue1 ) ||
						(h >= (int)from_hue2 && h <= (int)to_hue2 ) )

					{
						s = rgb2saturation( r[x], g[x], b[x] ) + saturation_offset;
						v = rgb2value( r[x], g[x], b[x] )+value_offset;
						h += hue_offset ;
						if( h > MAX_HUE16 )
							h -= MAX_HUE16 ;
						else if( h == 0 )
							h =  MIN_HUE16 ;
						else if( h < 0 )
							h += MAX_HUE16 ;
						if( v < 0 ) v = 0 ;
						else if( v > 0x00FFFF ) v = 0x00FFFF ;

						if( s < 0 ) s = 0 ;
						else if( s > 0x00FFFF ) s = 0x00FFFF ;

						hsv2rgb ( (CARD32)h, (CARD32)s, (CARD32)v, &r[x], &g[x], &b[x]);

#ifdef DEBUG_HSV_ADJUSTMENT
						fprintf( stderr, "OUT %d: rgb = #%4.4lX.%4.4lX.%4.4lX hue = %ld(%ld)     sat = %ld val = %ld\n", __LINE__, r[x], g[x], b[x], h, ((h>>8)*360)>>8, s, v );
#endif
					}
				}
			}
			imdec->buffer.flags = 0xFFFFFFFF ;
			imout->output_image_scanline( imout, &(imdec->buffer), 1);
		}
		stop_image_output( &imout );
	}
#ifdef HAVE_MMX
	mmx_off();
#endif
	stop_image_decoding( &imdec );

	SHOW_TIME("", started);
	return dst;
}



/* ********************************************************************************/
/* The end !!!! 																 */
/* ********************************************************************************/

