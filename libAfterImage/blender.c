/*
 * Copyright (c) 2000,2001 Sasha Vasko <sashav@sprintmail.com>
 *
 * This module is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "../configure.h"

/*#define LOCAL_DEBUG*/
/*#define DO_CLOCKING*/

#include "../include/aftersteplib.h"
#include <X11/Intrinsic.h>

#include "../include/afterstep.h"
#include "../include/screen.h"
#include "../include/asimage.h"

/*********************************************************************************/
/* colorspace conversion functions : 											 */
/*********************************************************************************/

inline CARD32
rgb2value( CARD32 red, CARD32 green, CARD32 blue )
{
	if( red > green )
		return MAX(red,blue);
	return MAX(green, blue);
}

inline CARD32
rgb2saturation( CARD32 red, CARD32 green, CARD32 blue )
{
	register int max_val, min_val ;
	if( red > green )
	{
		max_val = MAX(red,blue);
		min_val = MIN(green,blue);
	}else
	{
		max_val = MAX(green, blue) ;
		min_val = MIN(red,blue) ;
	}
	return max_val > 0 ? ((max_val - min_val)<<15)/(max_val>>1) : 0;
}

inline CARD32
rgb2hue( CARD32 red, CARD32 green, CARD32 blue )
{
	int max_val, min_val, hue = 0 ;
	if( red > green )
	{
		max_val = MAX(red,blue);
		min_val = MIN(green,blue);
	}else
	{
		max_val = MAX(green,blue);
		min_val = MIN(red,blue);
	}
	if( max_val != min_val)
	{
		int delta = max_val-min_val ;
#define HUE16_RANGE 		(85<<7)
		if( red == max_val )
		{
			if( green > blue )
				hue =              ((green - blue) * (HUE16_RANGE-1)) / delta ;
			else
				hue = HUE16_RANGE+ ((blue - green) * (HUE16_RANGE-1)) / delta ;
		}else if( green == max_val )
		{
			if( blue > red )
				hue = HUE16_RANGE*2 + ((blue-red ) * (HUE16_RANGE-1)) / delta ;
			else
				hue = HUE16_RANGE*3 + ((red -blue) * (HUE16_RANGE-1)) / delta ;
		}else if( red > green )
			hue     = HUE16_RANGE*4 + ((red -green)* (HUE16_RANGE-1)) / delta ;
		else
			hue     = HUE16_RANGE*5 + ((green- red)* (HUE16_RANGE-1)) / delta ;
	}
	return hue;
}

inline CARD32
rgb2hsv( CARD32 red, CARD32 green, CARD32 blue, CARD32 *saturation, CARD32 *value )
{
	int max_val, min_val, hue = 0 ;
	if( red > green )
	{
		max_val = MAX(red,blue);
		min_val = MIN(green,blue);
	}else
	{
		max_val = MAX(green,blue);
		min_val = MIN(red,blue);
	}
	*value = max_val ;
	if( max_val != min_val)
	{
		int delta = max_val-min_val ;
		*saturation = (delta<<15)/(max_val>>1);
		if( red == max_val )
		{
			if( green > blue )
			{
				hue =              ((green - blue) * (HUE16_RANGE-1)) / delta ;
				if( hue == 0 )
					hue = 1 ;                  /* valid hue must be != 0 */
			}else
				hue = HUE16_RANGE+ ((blue - green) * (HUE16_RANGE-1)) / delta ;
		}else if( green == max_val )
		{
			if( blue > red )
				hue = HUE16_RANGE*2 + ((blue-red ) * (HUE16_RANGE-1)) / delta ;
			else
				hue = HUE16_RANGE*3 + ((red -blue) * (HUE16_RANGE-1)) / delta ;
		}else if( red > green )
			hue     = HUE16_RANGE*4 + ((red -green)* (HUE16_RANGE-1)) / delta ;
		else
			hue     = HUE16_RANGE*5 + ((green- red)* (HUE16_RANGE-1)) / delta ;
	}else
		*saturation = 0 ;
	return hue;
}

inline void
hsv2rgb (CARD32 hue, CARD32 saturation, CARD32 value, CARD32 *red, CARD32 *green, CARD32 *blue)
{
	if (saturation == 0)
	{
    	*blue = *green = *red = value;
	}else
	{
		int range = hue/HUE16_RANGE ;
		int delta = ((saturation*(value>>1))>>15) ;
		int min_val = value - delta;
		int mid_val = (hue - HUE16_RANGE*range) * delta / (HUE16_RANGE-1)  + min_val ;
		switch( range )
		{
			case 0 :                           /* red was max, then green  */
				*red = value ;
				*green = mid_val ;
				*blue = min_val ;
			    break ;
			case 1 :                           /* red was max, then blue   */
				*red = value ;
				*blue = mid_val ;
				*green = min_val ;
			    break ;
			case 2 :                           /* green was max, then blue */
				*green = value ;
				*blue = mid_val ;
				*red = min_val ;
			    break ;
			case 3 :                           /* green was max, then red  */
				*green = value ;
				*red = mid_val ;
				*blue = min_val ;
			    break ;
			case 4 :                           /* blue was max, then red   */
				*blue  = value ;
				*red   = mid_val ;
				*green = min_val ;
			    break ;
			case 5 :                           /* blue was max, then green */
				*blue  = value ;
				*green = mid_val ;
				*red   = min_val ;
			    break ;
		}
	}
}

inline CARD32                                         /* returns luminance */
rgb2luminance (CARD32 red, CARD32 green, CARD32 blue )
{
	int max_val, min_val;
	if( red > green )
	{
		max_val = MAX(red,blue);
		min_val = MIN(green,blue);
	}else
	{
		max_val = MAX(green,blue);
		min_val = MIN(red,blue);
	}
	return (max_val+min_val)>>1;
}

inline CARD32                                         /* returns hue */
rgb2hls (CARD32 red, CARD32 green, CARD32 blue, CARD32 *luminance, CARD32 *saturation )
{
	int max_val, min_val, hue = 0 ;
	if( red > green )
	{
		max_val = MAX(red,blue);
		min_val = MIN(green,blue);
	}else
	{
		max_val = MAX(green,blue);
		min_val = MIN(red,blue);
	}
	*luminance = (max_val+min_val)>>1;

	if( max_val != min_val )
	{
		int delta = max_val-min_val ;
		if( *luminance == 0 ) ++(*luminance); 
		else if( *luminance == 0x0000FFFF ) --(*luminance); 
		*saturation = (*luminance < 0x00008000 )?
							(delta<<15)/ *luminance :
							(delta<<15)/ (0x0000FFFF - *luminance);
		if( red == max_val )
		{
			if( green > blue )
			{
				hue =              ((green - blue) * (HUE16_RANGE-1)) / delta ;
				if( hue == 0 )
					hue = 1 ;                  /* valid hue must be != 0 */
			}else
				hue = HUE16_RANGE+ ((blue - green) * (HUE16_RANGE-1)) / delta ;
		}else if( green == max_val )
		{
			if( blue > red )
				hue = HUE16_RANGE*2 + ((blue-red ) * (HUE16_RANGE-1)) / delta ;
			else
				hue = HUE16_RANGE*3 + ((red -blue) * (HUE16_RANGE-1)) / delta ;
		}else if( red > green )
			hue     = HUE16_RANGE*4 + ((red -green)* (HUE16_RANGE-1)) / delta ;
		else
			hue     = HUE16_RANGE*5 + ((green- red)* (HUE16_RANGE-1)) / delta ;

	}else
		*saturation = 0 ;
	return hue;
}

inline void
hls2rgb (CARD32 hue, CARD32 luminance, CARD32 saturation, CARD32 *red, CARD32 *green, CARD32 *blue)
{
	if (saturation == 0)
	{
    	*blue = *green = *red = luminance;
	}else
	{
		int range = hue/HUE16_RANGE ;
		int delta = ( luminance < 0x00008000 )?
						(saturation*luminance)>>15 :
	                    (saturation*(0x0000FFFF-luminance))>>15 ;
		int min_val = ((luminance<<1)-delta)>>1 ;
		int mid_val = (hue - HUE16_RANGE*range) * delta / (HUE16_RANGE-1)  + min_val ;
		switch( range )
		{
			case 0 :                           /* red was max, then green  */
				*red = delta+min_val ;
				*green = mid_val ;
				*blue = min_val ;
			    break ;
			case 1 :                           /* red was max, then blue   */
				*red = delta+min_val ;
				*blue = mid_val ;
				*green = min_val ;
			    break ;
			case 2 :                           /* green was max, then blue */
				*green = delta+min_val ;
				*blue = mid_val ;
				*red = min_val ;
			    break ;
			case 3 :                           /* green was max, then red  */
				*green = delta+min_val ;
				*red = mid_val ;
				*blue = min_val ;
			    break ;
			case 4 :                           /* blue was max, then red   */
				*blue  = delta+min_val ;
				*red   = mid_val ;
				*green = min_val ;
			    break ;
			case 5 :                           /* blue was max, then green */
				*blue  = delta+min_val ;
				*green = mid_val ;
				*red   = min_val ;
			    break ;
		}
	}
}

/*************************************************************************/
/* scanline blending 													 */
/*************************************************************************/

void
alphablend_scanlines( ASScanline *bottom, ASScanline *top, int unused )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *alpha = top->alpha, *ba = bottom->alpha;
	register CARD32 *br = bottom->red, *bg = bottom->green, *bb = bottom->blue;
	register CARD32 *tr = top->red, *tg = top->green, *tb = top->blue;

	while( ++i < max_i )
	{
		int a = alpha[i] ;
		if( a >= 0x0000FF00 )
		{
			br[i] = tr[i] ;
			bg[i] = tg[i] ;
			bb[i] = tb[i] ;
			ba[i] = 0x0000FF00;
		}else if( a > 0 )
		{
			ba[i] += a ;
			if( ba[i] > 0x0000FFFF )
				ba[i] = 0x0000FFFF ;
			a = (a>>8) ;
			br[i] = (br[i]*(255-a)+tr[i]*a)>>8 ;
			bg[i] = (bg[i]*(255-a)+tg[i]*a)>>8 ;
			bb[i] = (bb[i]*(255-a)+tb[i]*a)>>8 ;
		}
	}
}

void    /* this one was first implemented on XImages by allanon :) - mode 131  */
allanon_scanlines( ASScanline *bottom, ASScanline *top, int unused )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *b = bottom->buffer, *t = top->buffer ;
	max_i = (max_i + (max_i&0x0001)) * 4 ;
	while( ++i < max_i )
		b[i] = (b[i]+t[i])>>1 ;
}

void    /* this one was first implemented on XImages by allanon :) - mode 131  */
tint_scanlines( ASScanline *bottom, ASScanline *top, int unused )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *b = bottom->buffer, *t = top->buffer ;
	max_i = (max_i + (max_i&0x0001)) * 4 ;
	while( ++i < max_i )
	{
		b[i] = (b[i]*(t[i]>>1))>>15 ;
	}
}

void    /* addition with saturation : */
add_scanlines( ASScanline *bottom, ASScanline *top, int unused )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *b = bottom->buffer, *t = top->buffer ;
	register CARD32 *ba = bottom->alpha, *ta = top->alpha ;
	while( ++i < max_i )
	{
		b[i] = (b[i]+t[i]) ;
		if( b[i] > 0x0000FFFF )
			b[i] = 0x0000FFFF ;
		if( ta[i] < ba[i] )
			ba[i] = ta[i] ;
	}
	b = bottom->green ;
	t = top->green ;
	max_i = (max_i + (max_i&0x0001)) * 2 ;
	for( i = 0 ; i < max_i ; ++i )
	{
		b[i] = (b[i]+t[i]) ;
		if( b[i] > 0x0000FFFF )
			b[i] = 0x0000FFFF ;
	}
}

void    /* substruction with saturation : */
sub_scanlines( ASScanline *bottom, ASScanline *top, int unused )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *b = bottom->buffer, *t = top->buffer ;
	register CARD32 *ba = bottom->alpha, *ta = top->alpha ;
	max_i = (max_i + (max_i&0x0001)) * 3 ;
	while( ++i < max_i )
	{
		int res = (int)b[i] - (int)t[i] ;
		b[i] = res < 0 ? 0: res ;
		if( ta[i] < ba[i] )
			ba[i] = ta[i] ;
	}
}

void    /* absolute pixel value difference : */
diff_scanlines( ASScanline *bottom, ASScanline *top, int unused )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *b = bottom->buffer, *t = top->buffer ;
	register CARD32 *ba = bottom->alpha, *ta = top->alpha ;
	while( ++i < max_i )
	{
		register int res = (int)b[i] - (int)t[i] ;
		b[i] = res < 0 ? -res: res ;
		if( ta[i] < ba[i] )
			ba[i] = ta[i] ;
	}
	b = bottom->green ;
	t = top->green ;
	max_i = (max_i + (max_i&0x0001)) * 2 ;
	for( i = 0 ; i < max_i ; ++i )
	{
		register int res = (int)b[i] - (int)t[i] ;
		b[i] = res < 0 ? -res: res ;
	}
}

void    /* darkest of the two makes it in : */
darken_scanlines( ASScanline *bottom, ASScanline *top, int unused )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *b = bottom->buffer, *t = top->buffer ;
	max_i = (max_i + (max_i&0x0001)) * 4 ;
	while( ++i < max_i )
	{
		if( t[i] < b[i] )
			b[i] = t[i] ;
	}
}

void    /* lightest of the two makes it in : */
lighten_scanlines( ASScanline *bottom, ASScanline *top, int unused )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *b = bottom->buffer, *t = top->buffer ;
	register CARD32 *ba = bottom->alpha, *ta = top->alpha ;
	while( ++i < max_i )
	{
		if( t[i] > b[i] )
			b[i] = t[i] ;
		if( ta[i] < ba[i] )
			ba[i] = ta[i] ;
	}
	b = bottom->green ;
	t = top->green ;
	max_i = (max_i + (max_i&0x0001)) * 2 ;
	for( i = 0 ; i < max_i ; ++i )
	{
		if( t[i] > b[i] )
			b[i] = t[i] ;
	}
}

void    /* guess what this one does - I could not :) */
screen_scanlines( ASScanline *bottom, ASScanline *top, int unused )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *b = bottom->buffer, *t = top->buffer ;
	register CARD32 *ba = bottom->alpha, *ta = top->alpha ;
	max_i = (max_i + (max_i&0x0001)) * 3 ;
	while( ++i < max_i )
	{
		int res1 = 0x0000FFFF - (int)b[i] ;
		int res2 = 0x0000FFFF - (int)t[i] ;
		res1 = 0x0000FFFF - ((res1*res2)>>16);
		if( res1 < 0 )
			res1 = 0 ;
		b[i] = res1 < 0 ? 0 : res1;
	}
	max_i = bottom->width ;
	for( i = 0 ; i < max_i ; ++i )
	{
		if( ta[i] < ba[i] )
			ba[i] = ta[i] ;
	}
}

void    /* somehow overlays bottom with top : */
overlay_scanlines( ASScanline *bottom, ASScanline *top, int unused )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *b = bottom->buffer, *t = top->buffer ;
	register CARD32 *ba = bottom->alpha, *ta = top->alpha ;
	max_i = (max_i + (max_i&0x0001)) * 3 ;
	while( ++i < max_i )
	{

		int tmp_screen = 0x0000FFFF - (((0x0000FFFF - (int)b[i]) * (0x0000FFFF - (int)t[i])) >> 16);
		int tmp_mult   = (b[i] * t[i]) >> 16;
		int res = (b[i] * tmp_screen + (0x0000FFFF - (int)b[i]) * tmp_mult) >> 16;
		b[i] = res < 0 ? 0 : res;
	}
	max_i = bottom->width ;
	for( i = 0 ; i < max_i ; ++i )
	{
		if( ta[i] < ba[i] )
			ba[i] = ta[i] ;
	}
}

void
hue_scanlines( ASScanline *bottom, ASScanline *top, int mode )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *ta = top->alpha, *ba = bottom->alpha;
	register CARD32 *br = bottom->red, *bg = bottom->green, *bb = bottom->blue;
	register CARD32 *tr = top->red, *tg = top->green, *tb = top->blue;

	while( ++i < max_i )
	{
		CARD32 hue = rgb2hue( tr[i], tg[i], tb[i]);
		if( hue > 0 )
		{
			CARD32 saturation = rgb2saturation( br[i], bg[i], bb[i]);
			CARD32 value = rgb2value( br[i], bg[i], bb[i]);;

			hsv2rgb(hue, saturation, value, &br[i], &bg[i], &bb[i]);

		}
		if( ta[i] < ba[i] )
			ba[i] = ta[i] ;
	}
}

void
saturate_scanlines( ASScanline *bottom, ASScanline *top, int mode )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *ta = top->alpha, *ba = bottom->alpha;
	register CARD32 *br = bottom->red, *bg = bottom->green, *bb = bottom->blue;
	register CARD32 *tr = top->red, *tg = top->green, *tb = top->blue;

	while( ++i < max_i )
	{
		CARD32 saturation, value;
		CARD32 hue = rgb2hsv( br[i], bg[i], bb[i], &saturation, &value);

		saturation = rgb2saturation( tr[i], tg[i], tb[i]);
		hsv2rgb(hue, saturation, value, &br[i], &bg[i], &bb[i]);
		if( ta[i] < ba[i] )
			ba[i] = ta[i] ;
	}
}

void
value_scanlines( ASScanline *bottom, ASScanline *top, int mode )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *ta = top->alpha, *ba = bottom->alpha;
	register CARD32 *br = bottom->red, *bg = bottom->green, *bb = bottom->blue;
	register CARD32 *tr = top->red, *tg = top->green, *tb = top->blue;

	while( ++i < max_i )
	{
		CARD32 saturation, value;
		CARD32 hue = rgb2hsv( br[i], bg[i], bb[i], &saturation, &value);

		value = rgb2value( tr[i], tg[i], tb[i]);
		hsv2rgb(hue, saturation, value, &br[i], &bg[i], &bb[i]);

		if( ta[i] < ba[i] )
			ba[i] = ta[i] ;
	}
}

void
colorize_scanlines( ASScanline *bottom, ASScanline *top, int mode )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *ta = top->alpha, *ba = bottom->alpha;
	register CARD32 *br = bottom->red, *bg = bottom->green, *bb = bottom->blue;
	register CARD32 *tr = top->red, *tg = top->green, *tb = top->blue;

	while( ++i < max_i )
	{
#if 1
		CARD32 luminance, saturation ;
		CARD32 hue = rgb2hls( tr[i], tg[i], tb[i], &luminance, &saturation );

		luminance = rgb2luminance( br[i], bg[i], bb[i]);
		hls2rgb(hue, luminance, saturation, &br[i], &bg[i], &bb[i]);
#else
		CARD32 h, l, s, r, g, b;
		h = rgb2hls( br[i], bg[i], bb[i], &l, &s );
		hls2rgb( h, l, s, &r, &g, &b );
		if( r > br[i]+10 || r < br[i] - 10 )
		{
			fprintf( stderr, "%X.%X.%X -> %X.%X.%X -> %X.%X.%X\n",  br[i], bg[i], bb[i], h, l, s, r, g, b );
			fprintf( stderr, "%d.%d.%d -> %d.%d.%d -> %d.%d.%d\n",  br[i], bg[i], bb[i], h, l, s, r, g, b );
		}
#endif

		if( ta[i] < ba[i] )
			ba[i] = ta[i] ;
	}
}

void
dissipate_scanlines( ASScanline *bottom, ASScanline *top, int unused )
{
	register int i = -1, max_i = bottom->width ;
	register CARD32 *alpha = top->alpha, *ba = bottom->alpha;
	register CARD32 *br = bottom->red, *bg = bottom->green, *bb = bottom->blue;
	register CARD32 *tr = top->red, *tg = top->green, *tb = top->blue;
	static   CARD32 rnd32_seed = 345824357;	
#define MAX_MY_RND32		0x00ffffffff
#ifdef WORD64
#define MY_RND32() \
(rnd32_seed = ((1664525L*rnd32_seed)&MAX_MY_RND32)+1013904223L)
#else
#define MY_RND32() \
(rnd32_seed = (1664525L*rnd32_seed)+1013904223L)
#endif

	/* add some randomization here  if (rand < alpha) - combine */
	while( ++i < max_i )
	{
		int a = alpha[i] ;
		if( a > 0 && (MY_RND32()&0x0000FFFF) < a )
		{
			ba[i] += a ;
			if( ba[i] > 0x0000FFFF )
				ba[i] = 0x0000FFFF ;
			a = (a>>8) ;
			br[i] = (br[i]*(255-a)+tr[i]*a)>>8 ;
			bg[i] = (bg[i]*(255-a)+tg[i]*a)>>8 ;
			bb[i] = (bb[i]*(255-a)+tb[i]*a)>>8 ;
		}
	}
}

/*********************************************************************************/
/* The end !!!! 																 */
/*********************************************************************************/

