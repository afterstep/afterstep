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
# include "win32/config.h"
# include <windows.h>
# include "win32/afterbase.h"
#else
# include "config.h"
# include <string.h>
# include "afterbase.h"
#endif

#include "asimage.h"
#include "imencdec.h"
#include "bmp.h"


void 
dib_data_to_scanline( ASScanline *buf, 
                      BITMAPINFOHEADER *bmp_info, CARD8 *gamma_table, 
					  CARD8 *data, CARD8 *cmap, int cmap_entry_size) 
{	
	int x ; 
	switch( bmp_info->biBitCount )
	{
		case 1 :
			for( x = 0 ; x < bmp_info->biWidth ; x++ )
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
	CARD8 *bits = NULL, *curr ;
	int line_size, pad ; 
	ASImageDecoder *imdec ;
	int y, max_y = to_height;	
	int tiling_step = 0 ;
	CARD32 *r, *g, *b ;	
	START_TIME(started);

LOCAL_DEBUG_CALLER_OUT( "src = %p, offset_x = %d, offset_y = %d, to_width = %d, to_height = %d", im, offset_x, offset_y, to_width, to_height );
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
	bmp_info = (BITMAPINFO *)safecalloc( 1, sizeof(BITMAPINFO) );
	bmp_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmp_info->bmiHeader.biWidth = to_width ;
	bmp_info->bmiHeader.biHeight = to_height ;
	bmp_info->bmiHeader.biPlanes = 1 ;
	bmp_info->bmiHeader.biBitCount = 24 ;
	bmp_info->bmiHeader.biCompression = BI_RGB ;
	bmp_info->bmiHeader.biSizeImage = 0 ;
	bmp_info->bmiHeader.biClrUsed = 0 ;
	bmp_info->bmiHeader.biClrImportant = 0 ;
	/* allocate DIB bits : */
	line_size = ((to_width*3+3)/4)*4;          /* DWORD aligned */
	pad = line_size-(to_width*3) ;
	bits = (CARD8 *)safemalloc(line_size * to_height);
	curr = bits + line_size * to_height ;

	r = imdec->buffer.red ;
	g = imdec->buffer.green ;
	b = imdec->buffer.blue ;
	for( y = 0 ; y < max_y ; y++  )
	{
		register int x = to_width;
		imdec->decode_image_scanline( imdec );
		/* convert to DIB bits : */
		curr -= pad ;
		while( --x >= 0 ) 
		{
			curr -= 3 ; 
			curr[0] = b[x] ; 	
			curr[1] = g[x] ; 
			curr[2] = r[x] ; 
		}	 
		if( tiling_step > 0 ) 
		{
			CARD8 *tile ;
			int offset = tiling_step ; 
			while( y + offset < (int)to_height ) 
			{	 	 
				tile = curr - offset*line_size ; 
				memcpy( tile, curr, line_size );
				offset += tiling_step ;
			}
		}	 
	}
	
	stop_image_decoding( &imdec );

	SHOW_TIME("", started);
	*pBits = bits ;
	return bmp_info;
}

ASImage      *
bitmap2asimage (unsigned char *xim, int width, int height, unsigned int compression)
{
	ASImage      *im = NULL;
	int           i, bpl;
	ASScanline    xim_buf;

    if( xim == NULL )
		return NULL ;

	im = create_asimage( width, height, compression);
	prepare_scanline( width, 0, &xim_buf, True );

	if( xim )
	{
	    bpl = (width*32)>>3 ;
	    if( bpl == 0 )
		    bpl = 1 ;
	    else
		    bpl = (bpl+3)/4;
	    bpl *= 4;
		for (i = 0; i < height; i++) {
			raw2scanline( xim, &xim_buf, 0, width, False, True);
   		    asimage_add_line (im, IC_RED,   xim_buf.red, i);
		    asimage_add_line (im, IC_GREEN, xim_buf.green, i);
		    asimage_add_line (im, IC_BLUE,  xim_buf.blue, i);
			xim += bpl;
		}
	}
	free_scanline(&xim_buf, True);

	return im;
}

