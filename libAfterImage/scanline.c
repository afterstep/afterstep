/*
 * Copyright (c) 2001,2000,1999 Sasha Vasko <sasha at aftercode.net>
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

#define LOCAL_DEBUG
#undef DEBUG_SL2XIMAGE
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
 
#ifdef _WIN32
# include "win32/afterbase.h"
#else
# include "afterbase.h"
#endif
#include "asvisual.h"


/* ********************* ASScanline ************************************/
ASScanline*
prepare_scanline( unsigned int width, unsigned int shift, ASScanline *reusable_memory, Bool BGR_mode  )
{
	register ASScanline *sl = reusable_memory ;
	size_t aligned_width;
	void *ptr;

	if( sl == NULL )
		sl = safecalloc( 1, sizeof( ASScanline ) );
	else
		memset( sl, 0x00, sizeof(ASScanline));

	if( width == 0 ) width = 1 ;
	sl->width 	= width ;
	sl->shift   = shift ;
	/* we want to align data by 8 byte boundary (double)
	 * to allow for code with less ifs and easier MMX/3Dnow utilization :*/
	aligned_width = width + (width&0x00000001);
	sl->buffer = ptr = safecalloc (1, ((aligned_width*4)+16)*sizeof(CARD32)+8);
	if (ptr == NULL)
	{
		if (sl != reusable_memory)
			free (sl);
		return NULL;
	}

	sl->xc1 = sl->red 	= (CARD32*)((((long)ptr+7)>>3)*8);
	sl->xc2 = sl->green = sl->red   + aligned_width;
	sl->xc3 = sl->blue 	= sl->green + aligned_width;
	sl->alpha 	= sl->blue  + aligned_width;

	sl->channels[IC_RED] = sl->red ;
	sl->channels[IC_GREEN] = sl->green ;
	sl->channels[IC_BLUE] = sl->blue ;
	sl->channels[IC_ALPHA] = sl->alpha ;

	if( BGR_mode )
	{
		sl->xc1 = sl->blue ;
		sl->xc3 = sl->red ;
	}
	/* this way we can be sure that our buffers have size of multiplies of 8s
	 * and thus we can skip unneeded checks in code */
#if 0
	/* initializing padding into 0 to avoid any garbadge carry-over
	 * bugs with diffusion: */
	sl->red[aligned_width-1]   = 0;
	sl->green[aligned_width-1] = 0;
	sl->blue[aligned_width-1]  = 0;
	sl->alpha[aligned_width-1] = 0;
#endif	
	sl->back_color = ARGB32_DEFAULT_BACK_COLOR;

	return sl;
}

void
free_scanline( ASScanline *sl, Bool reusable )
{
	if( sl )
	{
		if( sl->buffer )
			free( sl->buffer );
		if( !reusable )
			free( sl );
	}
}

void
interpolate_scanline_CFA_RGGB_diff(ASScanline **lines, int offset, int chan_rb)
{
  int width = lines[0]->width;
  int x = offset + 2, max_x = width - 4;
  int chan_br = (chan_rb == ARGB32_RED_CHAN) ? ARGB32_BLUE_CHAN : ARGB32_RED_CHAN;
  CARD32 *R = lines[2]->channels[chan_rb], *R0 = lines[0]->channels[chan_rb], *R4 = lines[4]->channels[chan_rb];
  CARD32 *G = lines[2]->green, *G1 = lines[1]->green, *G3 = lines[3]->green;
  CARD32 *B = lines[2]->channels[chan_br], *B1 = lines[1]->channels[chan_br], *B3 = lines[3]->channels[chan_br];
  
  int gr_v = ((G1[x]+G3[x])<<2) + R0[x] + R4[x];
  int gr_h = ((G[x-1]+G[x+1])<<2) + R[x-2] + R[x+2];
  int gr = ((gr_v + gr_h)/2 - R[x]*10)/8;
  
  while (x < max_x)
  {
  	int v;
    {/* calculating blue and green for the red cell */
      int br_l = ((B1[x-1]+B3[x+1])<<2) + R0[x-2] + R4[x+2];
      int br_r = ((B1[x+1]+B3[x-1])<<2) + R0[x+2] + R4[x-2];
      
	  v = (((br_l+br_r)/2) - R[x]*10)/8 + (int)R[x];
      B[x] = v < 0 ? 0 : v;

	  v = (int)R[x] + gr;
      G[x] = v < 0 ? 0 : v;
      
	  ++x;
    }
    {/* calculating red and blue for the green cell */
      int bg, bg_v, bg_h1, bg_h2;
      int gr_next;
      gr_v = ((G1[x+1]+G3[x+1])<<2) + R0[x+1] + R4[x+1];
      gr_h = ((G[x]+G[x+2])<<2) + R[x-1] + R[x+3];
      gr_next = ((gr_v + gr_h)/2 - R[x+1]*10)/8;
      
	  v = (int)G[x] - (gr_next+gr)/2;
      R[x] = v < 0 ? 0 : v;

      gr = gr_next;
      /* and now - for more complicated matter - b-g calculation */
      /* vertical is easy :*/
      bg_v = ((B1[x]+B3[x])<<2) + lines[0]->green[x] + lines[4]->green[x];
	  bg_v -= G[x]*10;
      /* horizontal we split into 2 :*/
      bg_h1 = (B1[x]<<3);
	  bg_h1 -= ((G1[x-1]+G1[x+1])<<2);
      bg_h2 = (B3[x]<<3);
	  bg_h2 -= ((G3[x-1]+G3[x+1])<<2);

	  v = (int)G[x] + (bg_v+bg_h1+bg_h2)/24;
      B[x] = v < 0 ? 0 : v;
	  ++x;
    }
  }
}

