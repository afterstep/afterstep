/* 
 * Copyright (C) 2005 Sasha Vasko <sasha at aftercode.net>
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
#include "../configure.h"

#include "../include/afterbase.h"
#include "../libAfterImage/afterimage.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/screen.h"


#include "asgtk.h"
#include "asgtkai.h"

static void free_buffer (guchar *pixels, gpointer data)
{
	free (pixels);
}

GdkPixbuf *
ASImage2GdkPixbuf( ASImage *im ) 
{
	GdkPixbuf *pb = NULL ; 
	if( im ) 
	{
		int k = 0, i;
		int size = im->width*im->height;
		guchar *data ;
		ASImageDecoder *imdec;
		
		data = safemalloc( size*4 );
		if ((imdec = start_image_decoding(get_screen_visual(NULL), im, SCL_DO_ALL, 0, 0, im->width, im->height, NULL)) != NULL )
		{	 
			for (i = 0; i < (int)im->height; i++)
			{	
				CARD32 *r, *g, *b, *a ; 
				int x ; 
				imdec->decode_image_scanline( imdec ); 
				r = imdec->buffer.red ;
				g = imdec->buffer.green ;
				b = imdec->buffer.blue ;
				a = imdec->buffer.alpha ;
				for( x = 0 ; x < im->width ; ++x ) 
				{
					data[k] = r[x];
					data[++k] = g[x];
					data[++k] = b[x];
					data[++k] = a[x];
					++k;
				}	 
			}
			stop_image_decoding( &imdec );
		}
		

		pb = gdk_pixbuf_new_from_data( data, GDK_COLORSPACE_RGB, True, 8, im->width, im->height, im->width*4, free_buffer, NULL );
		if( pb == NULL ) 
			free( data );
	}	 
	return pb;
}	 


