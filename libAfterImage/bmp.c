/* This file contains code for unified image loading from many file formats */
/********************************************************************/
/* Copyright (c) 2001 Sasha Vasko <sasha at aftercode.net>           */
/********************************************************************/
/*
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

#undef LOCAL_DEBUG
#undef DO_CLOCKING
#undef DEBUG_TRANSP_GIF

#ifdef _WIN32
#include "win32/config.h"
#include <windows.h>
#else
#include "config.h"
#endif
#ifdef _WIN32
# include "win32/afterbase.h"
#else
# include "afterbase.h"
#endif

#include "bmp.h"

void 
dib_data_to_scanline( ASScanline *buf, 
                      BITMAPINFOHEADER *bmp_info, CARD8 *gamma_table, 
					  CARD8 *data, CARD8 *cmap, int cmap_entry_size) 
{	
	unsigned int x ; 
	switch( bmp_info->biBitCount )
	{
		case 1 :
			for( x = 0 ; x < (int)bmp_info->biWidth ; x++ )
			{
				int entry = (data[x>>3]&(1<<(x&0x07)))?cmap_entry_size:0 ;
				buf->red[x] = cmap[entry+2];
				buf->green[x] = cmap[entry+1];
				buf->blue[x] = cmap[entry];
			}
			break ;
		case 4 :
			for( x = 0 ; x < (int)bmp_info->biWidth ; x++ )
			{
				int entry = data[x>>1];
				if(x&0x01)
					entry = ((entry>>4)&0x0F)*cmap_entry_size ;
				else
					entry = (entry&0x0F)*cmap_entry_size ;
				buf->red[x] = cmap[entry+2];
				buf->green[x] = cmap[entry+1];
				buf->blue[x] = cmap[entry];
			}
			break ;
		case 8 :
			for( x = 0 ; x < (int)bmp_info->biWidth ; x++ )
			{
				int entry = data[x]*cmap_entry_size ;
				buf->red[x] = cmap[entry+2];
				buf->green[x] = cmap[entry+1];
				buf->blue[x] = cmap[entry];
			}
			break ;
		case 16 :
			for( x = 0 ; x < (int)bmp_info->biWidth ; ++x )
			{
				CARD8 c1 = data[x] ;
				CARD8 c2 = data[++x];
				buf->blue[x] =    c1&0x1F;
				buf->green[x] = ((c1>>5)&0x07)|((c2<<3)&0x18);
				buf->red[x] =   ((c2>>2)&0x1F);
			}
			break ;
		default:
			raw2scanline( data, buf, gamma_table, buf->width, False, (bmp_info->biBitCount==32));
	}
}

BITMAPINFO *
ASImage2DBI( ASVisual *asv, ASImage *im, 
		     int offset_x, int offset_y,
			 unsigned int to_width,
			 unsigned int to_height,
  			 void **pBits )
{
	BITMAPINFO *bmp_info = NULL;
	ASImageDecoder *imdec ;
	int y, max_y = to_height;	
	int tiling_step = 0 ;
	START_TIME(started);

LOCAL_DEBUG_CALLER_OUT( "src = %p, offset_x = %d, offset_y = %d, to_width = %d, to_height = %d, tint = #%8.8lX", src, offset_x, offset_y, to_width, to_height, tint );
	if( im== NULL || (imdec = start_image_decoding(asv, im, SCL_DO_ALL, offset_x, offset_y, to_width, 0, NULL)) == NULL )
	{
		LOCAL_DEBUG_OUT( "failed to start image decoding%s", "");
		return NULL;
	}
	
	if( to_height > im->height )
	{
		tiling_step = im->height ;
		max_y = im->height ;
	}
	/* create bmp_info struct */
	/* allocate DIB bits : */
	for( y = 0 ; y < max_y ; y++  )
	{
		imdec->decode_image_scanline( imdec );
		/* convert to DIB bits : */
	}
	
	stop_image_decoding( &imdec );

	SHOW_TIME("", started);
	return bmp_info;
}

#ifdef _WIN32




#endif

