/*
 * Copyright (c) 2001,2000,1999 Sasha Vasko <sashav@sprintmail.com>
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

#include "config.h"

#define LOCAL_DEBUG

#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xlibint.h>
#include <X11/Intrinsic.h>

#include "afterbase.h"
#include "asvisual.h"

void _XInitImageFuncPtrs(XImage*);

static int  get_shifts (unsigned long mask);
static int  get_bits (unsigned long mask);

CARD32 color2pixel32bgr(ASVisual *asv, CARD32 encoded_color, void *pixel);
CARD32 color2pixel32rgb(ASVisual *asv, CARD32 encoded_color, void *pixel);
CARD32 color2pixel24bgr(ASVisual *asv, CARD32 encoded_color, void *pixel);
CARD32 color2pixel24rgb(ASVisual *asv, CARD32 encoded_color, void *pixel);
CARD32 color2pixel16bgr(ASVisual *asv, CARD32 encoded_color, void *pixel);
CARD32 color2pixel16rgb(ASVisual *asv, CARD32 encoded_color, void *pixel);
CARD32 color2pixel15bgr(ASVisual *asv, CARD32 encoded_color, void *pixel);
CARD32 color2pixel15rgb(ASVisual *asv, CARD32 encoded_color, void *pixel);
void pixel2color32rgb(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue);
void pixel2color32bgr(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue);
void pixel2color24rgb(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue);
void pixel2color24bgr(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue);
void pixel2color16rgb(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue);
void pixel2color16bgr(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue);
void pixel2color15rgb(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue);
void pixel2color15bgr(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue);

void ximage2scanline32( ASVisual *asv, XImage *xim, ASScanline *sl, int y,  register unsigned char *xim_data );
void ximage2scanline16( ASVisual *asv, XImage *xim, ASScanline *sl, int y,  register unsigned char *xim_data );
void ximage2scanline15( ASVisual *asv, XImage *xim, ASScanline *sl, int y,  register unsigned char *xim_data );
void scanline2ximage32( ASVisual *asv, XImage *xim, ASScanline *sl, int y,  register unsigned char *xim_data );
void scanline2ximage16( ASVisual *asv, XImage *xim, ASScanline *sl, int y,  register unsigned char *xim_data );
void scanline2ximage15( ASVisual *asv, XImage *xim, ASScanline *sl, int y,  register unsigned char *xim_data );

int
asvisual_empty_XErrorHandler (Display * dpy, XErrorEvent * event)
{
    return 0;
}

/***************************************************************************
 ***************************************************************************
 * ASVisual :
 * encoding/decoding/querying/setup
 ***************************************************************************/

/* Main procedure finding and querying the best visual */
Bool
query_screen_visual( ASVisual *asv, Display *dpy, int screen, Window root, int default_depth )
{
	XVisualInfo *list = NULL;
	int nitems = 0 ;
	/* first  - attempt locating 24bpp TrueColor or DirectColor RGB or BGR visuals as the best cases : */
	/* second - attempt locating 32bpp TrueColor or DirectColor RGB or BGR visuals as the next best cases : */
	/* third  - lesser but still capable 16bpp 565 RGB or BGR modes : */
	/* forth  - even more lesser 15bpp 555 RGB or BGR modes : */
	/* nothing nice has been found - use whatever X has to offer us as a default :( */
	int i ;
	XSetWindowAttributes attr ;
	XColor black_xcol = { 0, 0x00, 0x00, 0x00, DoRed|DoGreen|DoBlue };
	XColor white_xcol = { 0, 0xFF, 0xFF, 0xFF, DoRed|DoGreen|DoBlue };
	static XVisualInfo templates[] =
		/* Visual, visualid, screen, depth, class      , red_mask, green_mask, blue_mask, colormap_size, bits_per_rgb */
		{{ NULL  , 0       , 0     , 24   , TrueColor  , 0xFF0000, 0x00FF00  , 0x0000FF , 0            , 0 },
		 { NULL  , 0       , 0     , 24   , TrueColor  , 0x0000FF, 0x00FF00  , 0xFF0000 , 0            , 0 },
		 { NULL  , 0       , 0     , 24   , TrueColor  , 0x0     , 0x0       , 0x0      , 0            , 0 },
		 { NULL  , 0       , 0     , 32   , TrueColor  , 0xFF0000, 0x00FF00  , 0x0000FF , 0            , 0 },
		 { NULL  , 0       , 0     , 32   , TrueColor  , 0x0000FF, 0x00FF00  , 0xFF0000 , 0            , 0 },
		 { NULL  , 0       , 0     , 32   , TrueColor  , 0x0     , 0x0       , 0x0      , 0            , 0 },
		 { NULL  , 0       , 0     , 16   , TrueColor  , 0xF800  , 0x07E0    , 0x001F   , 0            , 0 },
		 { NULL  , 0       , 0     , 16   , TrueColor  , 0x001F  , 0x07E0    , 0xF800   , 0            , 0 },
		 /* big endian or MBR_First modes : */
		 { NULL  , 0       , 0     , 16   , TrueColor  , 0x0     , 0xE007    , 0x0      , 0            , 0 },
		 /* some misrepresented modes that really are 15bpp : */
		 { NULL  , 0       , 0     , 16   , TrueColor  , 0x7C00  , 0x03E0    , 0x001F   , 0            , 0 },
		 { NULL  , 0       , 0     , 16   , TrueColor  , 0x001F  , 0x03E0    , 0x7C00   , 0            , 0 },
		 { NULL  , 0       , 0     , 16   , TrueColor  , 0x0     , 0xE003    , 0x0      , 0            , 0 },
		 { NULL  , 0       , 0     , 15   , TrueColor  , 0x7C00  , 0x03E0    , 0x001F   , 0            , 0 },
		 { NULL  , 0       , 0     , 15   , TrueColor  , 0x001F  , 0x03E0    , 0x7C00   , 0            , 0 },
		 { NULL  , 0       , 0     , 15   , TrueColor  , 0x0     , 0xE003    , 0x0      , 0            , 0 },
/* no suitable TrueColorMode found - now do the same thing to DirectColor :*/
		 { NULL  , 0       , 0     , 24   , DirectColor, 0xFF0000, 0x00FF00  , 0x0000FF , 0            , 0 },
		 { NULL  , 0       , 0     , 24   , DirectColor, 0x0000FF, 0x00FF00  , 0xFF0000 , 0            , 0 },
		 { NULL  , 0       , 0     , 24   , DirectColor, 0x0     , 0x0       , 0x0      , 0            , 0 },
		 { NULL  , 0       , 0     , 32   , DirectColor, 0xFF0000, 0x00FF00  , 0x0000FF , 0            , 0 },
		 { NULL  , 0       , 0     , 32   , DirectColor, 0x0000FF, 0x00FF00  , 0xFF0000 , 0            , 0 },
		 { NULL  , 0       , 0     , 32   , DirectColor, 0x0     , 0x0       , 0x0      , 0            , 0 },
		 { NULL  , 0       , 0     , 16   , DirectColor, 0xF800  , 0x07E0    , 0x001F   , 0            , 0 },
		 { NULL  , 0       , 0     , 16   , DirectColor, 0x001F  , 0x07E0    , 0xF800   , 0            , 0 },
		 { NULL  , 0       , 0     , 16   , DirectColor, 0x0     , 0xE007    , 0x0      , 0            , 0 },
		 { NULL  , 0       , 0     , 16   , DirectColor, 0x7C00  , 0x03E0    , 0x001F   , 0            , 0 },
		 { NULL  , 0       , 0     , 16   , DirectColor, 0x001F  , 0x03E0    , 0x7C00   , 0            , 0 },
		 { NULL  , 0       , 0     , 16   , DirectColor, 0x0     , 0xE003    , 0x0      , 0            , 0 },
		 { NULL  , 0       , 0     , 15   , DirectColor, 0x7C00  , 0x03E0    , 0x001F   , 0            , 0 },
		 { NULL  , 0       , 0     , 15   , DirectColor, 0x001F  , 0x03E0    , 0x7C00   , 0            , 0 },
		 { NULL  , 0       , 0     , 15   , DirectColor, 0x0     , 0xE003    , 0x0      , 0            , 0 },
		 { NULL  , 0       , 0     , 0    , 0          , 0       , 0         , 0        , 0            , 0 },
		} ;

	if( asv == NULL )
		return False ;
	memset( asv, 0x00, sizeof(ASVisual));

	memset( &attr, 0x00, sizeof( attr ));
	for( i = 0 ; templates[i].depth != 0 ; i++ )
	{
		int k ;
		int           (*oldXErrorHandler) (Display *, XErrorEvent *) = NULL;
		int mask = VisualScreenMask|VisualDepthMask|VisualClassMask ;

		templates[i].screen = screen ;
		if( templates[i].red_mask != 0 )
			mask |= VisualRedMaskMask;
		if( templates[i].green_mask != 0 )
			mask |= VisualGreenMaskMask ;
		if( templates[i].blue_mask != 0 )
			mask |= VisualBlueMaskMask ;
		list = XGetVisualInfo( dpy, mask, &(templates[i]), &nitems );
		oldXErrorHandler = XSetErrorHandler (asvisual_empty_XErrorHandler);
		for( k = 0  ; k < nitems ; k++ )
		{
			Window       w = None, wjunk;
			unsigned int width, height, ujunk ;
			int          junk;
			/* try and use default colormap when possible : */
			if( asv->visual_info.visual == DefaultVisual( dpy, (screen) ) )
				attr.colormap = DefaultColormap( dpy, screen );
			else
				attr.colormap = XCreateColormap( dpy, root, list[k].visual, AllocNone);
			XAllocColor( dpy, attr.colormap, &black_xcol );
			XAllocColor( dpy, attr.colormap, &white_xcol );
			attr.border_pixel = black_xcol.pixel ;
/*fprintf( stderr, "checking out visual ID %d, class %d, depth = %d mask = %X,%X,%X\n", list[k].visualid, list[k].class, list[k].depth, list[k].red_mask, list[k].green_mask, list[k].blue_mask 	);*/
			w = XCreateWindow (dpy, root, -10, -10, 10, 10, 0, 0, CopyFromParent, list[k].visual, CWColormap|CWBorderPixmap|CWBorderPixel, &attr );
			if( w != None && XGetGeometry (dpy, w, &wjunk, &junk, &junk, &width, &height, &ujunk, &ujunk))
			{
				/* don't really care what's in it since we do not use it anyways : */
				asv->visual_info = list[k] ;
				XDestroyWindow( dpy, w );
				break;
			}
			if( attr.colormap != DefaultColormap( dpy, screen ))
				XFreeColormap( dpy, attr.colormap );
		}
		XSetErrorHandler(oldXErrorHandler);
		if( list )
			XFree( list );
		list = NULL ;
		if( asv->visual_info.visual != NULL )
			break;
	}
	if( asv->visual_info.visual == NULL )
	{  /* we ain't found any decent Visuals - that's some crappy screen you have! */
		register int vclass = 6 ;
		while( --vclass >= 0 )
			if( XMatchVisualInfo( dpy, screen, default_depth, vclass, &(asv->visual_info) ) )
				break;
		if( vclass < 0 )
			return False;
		/* try and use default colormap when possible : */
		if( asv->visual_info.visual == DefaultVisual( dpy, screen ) )
			attr.colormap = DefaultColormap( dpy, screen );
		else
			attr.colormap = XCreateColormap( dpy, root, asv->visual_info.visual, AllocNone);
		XAllocColor( dpy, attr.colormap, &black_xcol );
		XAllocColor( dpy, attr.colormap, &white_xcol );
	}
	asv->dpy = dpy ;
	asv->colormap = attr.colormap ;
	asv->own_colormap = (attr.colormap != DefaultColormap( dpy, screen ));
	asv->white_pixel = white_xcol.pixel ;
	asv->black_pixel = black_xcol.pixel ;
	return True;
}

void
destroy_asvisual( ASVisual *asv )
{
	if( asv )
	{
	 	if( asv->own_colormap )
	 	{
	 		if( asv->colormap )
	 			XFreeColormap( asv->dpy, asv->colormap );
	 	}
	 	if( asv->as_colormap )
	 		free( asv->as_colormap );
	}
}

int as_colormap_type2size( int type )
{
	switch( type )
	{
		case ACM_3BPP :
			return 8 ;
		case ACM_6BPP :
			return 64 ;
		case ACM_12BPP :
			return 4096 ;
		default:
			return 0 ;
	}
}

Bool
visual2visual_prop( ASVisual *asv, size_t *size,
								   unsigned long *version,
								   unsigned long **data )
{
	int cmap_size = 0 ;
	unsigned long *prop ;

	if( asv == NULL || data == NULL)
		return False;

	cmap_size = as_colormap_type2size( asv->as_colormap_type );

	if( cmap_size > 0 && asv->as_colormap == NULL )
		return False ;

	prop = safemalloc( *size ) ;
	prop[0] = asv->visual_info.visualid ;
	prop[1] = asv->colormap ;
	prop[2] = asv->black_pixel ;
	prop[3] = asv->white_pixel ;
	prop[4] = asv->as_colormap_type ;
	if( cmap_size > 0 )
	{
		register int i;
		for( i = 0 ; i < cmap_size ; i++ )
			prop[i+5] = asv->as_colormap[i] ;
	}
	if( size )
		*size = (1+1+2+1+cmap_size)*sizeof(unsigned long);
	if( version )
		*version = (1<<16)+0;                        /* version is 1.0 */
	*data = prop ;
	return True;
}

Bool
visual_prop2visual( ASVisual *asv, Display *dpy, int screen,
								   size_t size,
								   unsigned long version,
								   unsigned long *data )
{
	XVisualInfo templ, *list ;
	int nitems = 0 ;
	int cmap_size = 0 ;

	if( asv == NULL )
		return False;

	asv->dpy = dpy ;

	if( size < (1+1+2+1)*sizeof(unsigned long) ||
		(version&0x00FFFF) != 0 || (version>>16) != 1 || data == NULL )
		return False;

	if( data[0] == None || data[1] == None ) /* we MUST have valid colormap and visualID !!!*/
		return False;

	templ.screen = screen ;
	templ.visualid = data[0] ;

	list = XGetVisualInfo( dpy, VisualScreenMask|VisualIDMask, &templ, &nitems );
	if( list == NULL || nitems == 0 )
		return False;   /* some very bad visual ID has been requested :( */

	asv->visual_info = *list ;
	XFree( list );

	if( asv->own_colormap && asv->colormap )
		XFreeColormap( dpy, asv->colormap );

	asv->colormap = data[1] ;
	asv->own_colormap = False ;
	asv->black_pixel = data[2] ;
	asv->white_pixel = data[3] ;
	asv->as_colormap_type = data[4];

	cmap_size = as_colormap_type2size( asv->as_colormap_type );

	if( cmap_size > 0 )
	{
		register int i ;
		if( asv->as_colormap )
			free( asv->as_colormap );
		asv->as_colormap = safemalloc( cmap_size );
		for( i = 0 ; i < cmap_size ; i++ )
			asv->as_colormap[i] = data[i+5];
	}else
		asv->as_colormap_type = ACM_None ;     /* just in case */

	return True;
}

Bool
setup_truecolor_visual( ASVisual *asv )
{
	XVisualInfo *vi = &(asv->visual_info) ;

	if( vi->class != TrueColor )
		return False;

	asv->BGR_mode = ((vi->red_mask&0x0010)!=0) ;
	asv->rshift = get_shifts (vi->red_mask);
	asv->gshift = get_shifts (vi->green_mask);
	asv->bshift = get_shifts (vi->blue_mask);
	asv->rbits = get_bits (vi->red_mask);
	asv->gbits = get_bits (vi->green_mask);
	asv->bbits = get_bits (vi->blue_mask);
	asv->true_depth = vi->depth ;
	asv->msb_first = (ImageByteOrder(asv->dpy)==MSBFirst);

	if( asv->true_depth == 16 && ((vi->red_mask|vi->blue_mask)&0x8000) == 0 )
		asv->true_depth = 15;
	/* setting up conversion handlers : */
	switch( asv->true_depth )
	{
		case 24 :
		case 32 :
			asv->color2pixel_func     = (asv->BGR_mode)?color2pixel32bgr:color2pixel32rgb ;
			asv->pixel2color_func     = (asv->BGR_mode)?pixel2color32bgr:pixel2color32rgb ;
			asv->ximage2scanline_func = ximage2scanline32 ;
			asv->scanline2ximage_func = scanline2ximage32 ;
		    break ;
/*		case 24 :
			scr->color2pixel_func     = (bgr_mode)?color2pixel24bgr:color2pixel24rgb ;
			scr->pixel2color_func     = (bgr_mode)?pixel2color24bgr:pixel2color24rgb ;
			scr->ximage2scanline_func = ximage2scanline24 ;
			scr->scanline2ximage_func = scanline2ximage24 ;
		    break ;
  */	case 16 :
			asv->color2pixel_func     = (asv->BGR_mode)?color2pixel16bgr:color2pixel16rgb ;
			asv->pixel2color_func     = (asv->BGR_mode)?pixel2color16bgr:pixel2color16rgb ;
			asv->ximage2scanline_func = ximage2scanline16 ;
			asv->scanline2ximage_func = scanline2ximage16 ;
		    break ;
		case 15 :
			asv->color2pixel_func     = (asv->BGR_mode)?color2pixel15bgr:color2pixel15rgb ;
			asv->pixel2color_func     = (asv->BGR_mode)?pixel2color15bgr:pixel2color15rgb ;
			asv->ximage2scanline_func = ximage2scanline15 ;
			asv->scanline2ximage_func = scanline2ximage15 ;
		    break ;
	}
	return (asv->ximage2scanline_func != NULL) ;
}

Bool
setup_pseudo_visual( ASVisual *asv  )
{
	XVisualInfo *vi = &(asv->visual_info) ;

	if( vi->class == TrueColor )
		return False;
	/* we need to allocate new usable list of colors based on available bpp */
	asv->true_depth = vi->depth ;

	/* then we need to set up hooks : */
			asv->ximage2scanline_func = ximage2scanline32 ;
			asv->scanline2ximage_func = scanline2ximage32 ;
	return True;
}

void
setup_as_colormap( ASVisual *asv )
{
	int cmap_size ;
	int i = 0;
	int max_chan_val, curr_chan_val, offset, offset_xcol ;

	if( asv == NULL || asv->as_colormap != NULL )
		return ;

	if( asv->true_depth >= 12 )
	{
		asv->as_colormap_type = ACM_12BPP ;
		max_chan_val = 0x0F ;
		offset = 4 ;
	}else if( asv->true_depth >= 8 )
	{
		asv->as_colormap_type = ACM_6BPP ;
		max_chan_val = 0x03 ;
		offset = 2 ;
	}else
	{
		asv->as_colormap_type = ACM_3BPP ;
		max_chan_val = 0x01 ;
		offset = 1 ;
	}
	offset_xcol = 16-offset ;

	cmap_size = as_colormap_type2size( asv->as_colormap_type );
	asv->as_colormap = safemalloc( cmap_size * sizeof(unsigned long));

	/* initializing to some sensible defaults : */
	while( i < (cmap_size>>1) )
		asv->as_colormap[i++] = asv->black_pixel ;
	while( i <  cmap_size )
		asv->as_colormap[i++] = asv->white_pixel ;

	curr_chan_val = max_chan_val ;
	while( curr_chan_val > 0 )
	{
		XColor xcol ;
		unsigned int cell ;
		unsigned int green, red, blue ;
		/* 1) green */
		xcol.green = curr_chan_val<<offset_xcol ;
		xcol.red = 0 ;
		xcol.blue = 0 ;
		if( XAllocColor( asv->dpy, asv->colormap, &xcol ) == 0 )
		{
			show_debug( __FILE__, __FUNCTION__, __LINE__, "failed to allocate color #%2.2X%2.2X%2.2X", (xcol.red&0xFF00)>>8, (xcol.green&0xFF00)>>8, (xcol.blue&0xFF00)>>8 );
			break ;
		}
		cell = (xcol.green<<offset);
/*
		while( xcol.red < curr_chan_val )
		{
			while( xcol.blue < curr_chan_val )
			{
			}
		}
  */		/* 2) red */
		/* 3) blue */
		/* 4) green-red */
		/* 5) green-blue */
		/* 6) blue-red */
		--curr_chan_val;
	}


}

/*********************************************************************/
/* handy utility functions for creation of windows/pixmaps/XImages : */
/*********************************************************************/
Window
create_visual_window( ASVisual *asv, Window parent,
					  int x, int y, unsigned int width, unsigned int height,
					  unsigned int border_width, unsigned int class,
 					  unsigned long mask, XSetWindowAttributes *attributes )
{
	XSetWindowAttributes my_attr ;

	if( asv == NULL || parent == None )
		return None ;
LOCAL_DEBUG_OUT( "Colormap %lX, parent %lX, %ux%u%+d%+d, bw = %d, class %d",
				  asv->colormap, parent, width, height, x, y, border_width,
				  class );
	if( attributes == NULL )
	{
		attributes = &my_attr ;
		memset( attributes, 0x00, sizeof(XSetWindowAttributes));
		mask = 0;
	}

	if( width < 1 )
		width = 1 ;
	if( height < 1 )
		height = 1 ;

	if( class == InputOnly )
	{
		border_width = 0 ;
		if( (mask&INPUTONLY_LEGAL_MASK) != mask )
			show_warning( " software BUG detected : illegal InputOnly window's mask 0x%lX - overriding", mask );
		mask &= INPUTONLY_LEGAL_MASK ;
	}else
	{
		if( !get_flags(mask, CWColormap ) )
		{
			attributes->colormap = asv->colormap ;
			set_flags(mask, CWColormap );
		}
		if( !get_flags(mask, CWBorderPixmap ) )
		{
			attributes->border_pixmap = None ;
			set_flags(mask, CWBorderPixmap );
		}
		if( !get_flags(mask, CWBorderPixel ) )
		{
			attributes->border_pixel = asv->black_pixel ;
			set_flags(mask, CWBorderPixel );
		}
	}
	return XCreateWindow (asv->dpy, parent, x, y, width, height, border_width, 0,
						  class, asv->visual_info.visual,
	                      mask, attributes);
}

Pixmap
create_visual_pixmap( ASVisual *asv, Window root, unsigned int width, unsigned int height, unsigned int depth )
{
	Pixmap p = None ;
	if( asv != NULL )
		p = XCreatePixmap( asv->dpy, root, MAX(width,1), MAX(height,1), (depth==0)?asv->true_depth:depth );
	return p;
}

int
My_XDestroyImage (ximage)
	 XImage       *ximage;
{
	if (ximage->data != NULL)
		free ((char *)ximage->data);
	if (ximage->obdata != NULL)
		free ((char *)ximage->obdata);
	XFree ((char *)ximage);
	return 1;
}

XImage*
create_visual_ximage( ASVisual *asv, unsigned int width, unsigned int height, unsigned int depth )
{
	register XImage *ximage;
	unsigned long dsize;
	char         *data;

	if( asv == NULL )
		return NULL;

	ximage = XCreateImage (asv->dpy, asv->visual_info.visual, (depth==0)?asv->true_depth:depth, ZPixmap, 0, NULL, MAX(width,1), MAX(height,1), asv->dpy->bitmap_unit, 0);
	if (ximage != NULL)
	{
		_XInitImageFuncPtrs (ximage);
		ximage->obdata = NULL;
		ximage->f.destroy_image = My_XDestroyImage;
		dsize = ximage->bytes_per_line*ximage->height;
	    if (((data = (char *)safecalloc (1, dsize)) == NULL) && (dsize > 0))
		{
			XFree ((char *)ximage);
			return (XImage *) NULL;
		}
		ximage->data = data;
	}
	return ximage;
}

/****************************************************************************/
/* Color manipulation functions :                                           */
/****************************************************************************/


/* misc function to calculate number of bits/shifts */

static int
get_shifts (unsigned long mask)
{
	register int  i = 1;

	while (mask >> i)
		i++;

	return i - 1;							   /* can't be negative */
}

static int
get_bits (unsigned long mask)
{
	register int  i;

	for (i = 0; mask; mask >>= 1)
		if (mask & 1)
			i++;

	return i;								   /* can't be negative */
}
/***************************************************************************/
/* Screen color format -> AS color format conversion ; 					   */
/***************************************************************************/
/***************************************************************************/
/* Screen color format -> AS color format conversion ; 					   */
/***************************************************************************/
/* this functions convert encoded color values into real pixel values, and
 * return half of the quantization error so we can do error diffusion : */
/* encoding scheme : 0RRRrrrr rrrrGGgg ggggggBB bbbbbbbb
 * where RRR, GG and BB are overflow bits so we can do all kinds of funky
 * combined adding, note that we don't use 32'd bit as it is a sign bit */

CARD32 color2pixel32bgr(ASVisual *asv, CARD32 encoded_color, void *pixel)
{

	return 0;
}
CARD32 color2pixel32rgb(ASVisual *asv, CARD32 encoded_color, void *pixel)
{
	return 0;
}
CARD32 color2pixel24bgr(ASVisual *asv, CARD32 encoded_color, void *pixel)
{
	return 0;
}
CARD32 color2pixel24rgb(ASVisual *asv, CARD32 encoded_color, void *pixel)
{
	return 0;
}
CARD32 color2pixel16bgr(ASVisual *asv, CARD32 encoded_color, void *pixel)
{
	return (encoded_color>>1)&0x00300403;
}
CARD32 color2pixel16rgb(ASVisual *asv, CARD32 encoded_color, void *pixel)
{
	return (encoded_color>>1)&0x00300403;
}
CARD32 color2pixel15bgr(ASVisual *asv, CARD32 encoded_color, void *pixel)
{
	return (encoded_color>>1)&0x00300C03;
}
CARD32 color2pixel15rgb(ASVisual *asv, CARD32 encoded_color, void *pixel)
{
	return (encoded_color>>1)&0x00300C03;
}

void pixel2color32rgb(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue)
{}
void pixel2color32bgr(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue)
{}
void pixel2color24rgb(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue)
{}
void pixel2color24bgr(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue)
{}
void pixel2color16rgb(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue)
{}
void pixel2color16bgr(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue)
{}
void pixel2color15rgb(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue)
{}
void pixel2color15bgr(ASVisual *asv, unsigned long pixel, CARD32 *red, CARD32 *green, CARD32 *blue)
{}


void ximage2scanline32(ASVisual *asv, XImage *xim, ASScanline *sl, int y,  register unsigned char *xim_data )
{
	register CARD32 *r = sl->xc1+sl->offset_x, *g = sl->xc2+sl->offset_x, *b = sl->xc3+sl->offset_x;
	register int i = MIN(xim->width,sl->width-sl->offset_x);
	register CARD8 *src = (CARD8*)(xim_data+(i-1)*4) ;
	if( asv->msb_first )
		do
		{
			--i ;
			{
				r[i] = src[1];
				g[i] = src[2];
				b[i] = src[3];
				src -= 4 ;
			}
		}while(i);
	else
		do
		{
			--i ;
			{
				r[i] = src[2];
				g[i] = src[1];
				b[i] = src[0];
				src -= 4 ;
			}
		}while(i);
}

void ximage2scanline16( ASVisual *asv, XImage *xim, ASScanline *sl, int y,  register unsigned char *xim_data )
{
	register int i = MIN(xim->width,sl->width-sl->offset_x)-1;
	register CARD16 *src = (CARD16*)xim_data ;
    register CARD32 *r = sl->xc1+sl->offset_x, *g = sl->xc2+sl->offset_x, *b = sl->xc3+sl->offset_x;
#ifdef WORDS_BIGENDIAN
	if( !asv->msb_first )
#else
	if( asv->msb_first )
#endif
		do
		{
#define ENCODE_MSBF_565(r,gh3,gl3,b)	(((gh3)&0x0007)|((gl3)&0xE000)|((r)&0x00F8)|((b)&0x1F00))
			r[i] =  (src[i]&0x00F8);
			g[i] = ((src[i]&0x0007)<<5)|((src[i]&0xE000)>>11);
			b[i] =  (src[i]&0x1F00)>>5;
		}while( --i >= 0);
	else
		do
		{
#define ENCODE_LSBF_565(r,g,b) (((g)&0x07E0)|((r)&0xF800)|((b)&0x001F))
			r[i] =  (src[i]&0xF800)>>8;
			g[i] =  (src[i]&0x07E0)>>3;
			b[i] =  (src[i]&0x001F)<<3;
		}while( --i >= 0);

}
void ximage2scanline15( ASVisual *asv, XImage *xim, ASScanline *sl, int y,  register unsigned char *xim_data )
{
	register int i = MIN(xim->width,sl->width-sl->offset_x)-1;
	register CARD16 *src = (CARD16*)xim_data ;
    register CARD32 *r = sl->xc1+sl->offset_x, *g = sl->xc2+sl->offset_x, *b = sl->xc3+sl->offset_x;
#ifdef WORDS_BIGENDIAN
	if( !asv->msb_first )
#else
	if( asv->msb_first )
#endif
		do
		{
#define ENCODE_MSBF_555(r,gh2,gl3,b)	(((gh2)&0x0003)|((gl3)&0xE000)|((r)&0x007C)|((b)&0x1F00))
			r[i] =  (src[i]&0x007C)<<1;
			g[i] = ((src[i]&0x0003)<<6)|((src[i]&0xE000)>>10);
			b[i] =  (src[i]&0x1F00)>>5;
		}while( --i >= 0);
	else
		do
		{
#define ENCODE_LSBF_555(r,g,b) (((g)&0x03E0)|((r)&0x7C00)|((b)&0x001F))
			r[i] =  (src[i]&0x7C00)>>7;
			g[i] =  (src[i]&0x03E0)>>2;
			b[i] =  (src[i]&0x001F)<<3;
		}while( --i >= 0);
}

void scanline2ximage32( ASVisual *asv, XImage *xim, ASScanline *sl, int y,  register unsigned char *xim_data )
{
	register CARD32 *r = sl->xc1+sl->offset_x, *g = sl->xc2+sl->offset_x, *b = sl->xc3+sl->offset_x;
	register int i = MIN(xim->width,sl->width-sl->offset_x);
	register CARD8 *src = (CARD8*)(xim_data+(i-1)*4) ;
	if( asv->msb_first )
		do
		{
			--i ;
			{
				src[1] = r[i];
 				src[2] = g[i];
				src[3] = b[i];
				src -= 4 ;
			}
		}while(i);
	else
		do
		{
			--i ;
			{
				src[2] = r[i];
				src[1] = g[i];
				src[0] = b[i];
				src -= 4 ;
			}
		}while(i);
}

void scanline2ximage16( ASVisual *asv, XImage *xim, ASScanline *sl, int y,  register unsigned char *xim_data )
{
	register int i = MIN(xim->width,sl->width-sl->offset_x)-1;
	register CARD16 *src = (CARD16*)xim_data ;
    register CARD32 *r = sl->xc1+sl->offset_x, *g = sl->xc2+sl->offset_x, *b = sl->xc3+sl->offset_x;
	register CARD32 c = (r[i]<<20) | (g[i]<<10) | (b[i]);
#ifdef WORDS_BIGENDIAN
	if( !asv->msb_first )
#else
	if( asv->msb_first )
#endif
		do
		{
			src[i] = ENCODE_MSBF_565((c>>20),(c>>17),(c>>1),(c<<5));
			if( --i < 0 )
				break;
			/* carry over quantization error allow for error diffusion:*/
			c = ((c>>1)&0x00300403)+((r[i]<<20) | (g[i]<<10) | (b[i]));
			{
				register CARD32 d = c&0x300C0300 ;
				if( d )
				{
					if( c&0x30000000 )
						d |= 0x0FF00000;
					if( c&0x000C0000 )
						d |= 0x0003FC00 ;
					if( c&0x00000300 )
						d |= 0x000000FF ;
					c ^= d;
				}
/*fprintf( stderr, "c = 0x%X, d = 0x%X, c^d = 0x%X\n", c, d, c^d );*/
			}
		}while(1);
	else
		do
		{
			src[i] = ENCODE_LSBF_565((c>>12),(c>>7),(c>>3));
			if( --i < 0 )
				break;
			/* carry over quantization error allow for error diffusion:*/
			c = ((c>>1)&0x00300403)+((r[i]<<20) | (g[i]<<10) | (b[i]));
			{
				register CARD32 d = c&0x300C0300 ;
				if( d )
				{
					if( c&0x30000000 )
						d |= 0x0FF00000;
					if( c&0x000C0000 )
						d |= 0x0003FC00 ;
					if( c&0x00000300 )
						d |= 0x000000FF ;
					c ^= d;
				}
/*fprintf( stderr, "c = 0x%X, d = 0x%X, c^d = 0x%X\n", c, d, c^d );*/
			}
		}while(1);
}

void scanline2ximage15( ASVisual *asv, XImage *xim, ASScanline *sl, int y,  register unsigned char *xim_data )
{
	register int i = MIN(xim->width,sl->width-sl->offset_x)-1;
	register CARD16 *src = (CARD16*)xim_data ;
    register CARD32 *r = sl->xc1+sl->offset_x, *g = sl->xc2+sl->offset_x, *b = sl->xc3+sl->offset_x;
	register CARD32 c = (r[i]<<20) | (g[i]<<10) | (b[i]);
#ifdef WORDS_BIGENDIAN
	if( !asv->msb_first )
#else
	if( asv->msb_first )
#endif
		do
		{
			src[i] = ENCODE_MSBF_555((c>>21),(c>>18),(c>>2),(c<<5));
			if( --i < 0 )
				break;
			/* carry over quantization error allow for error diffusion:*/
			c = ((c>>1)&0x00300C03)+((r[i]<<20) | (g[i]<<10) | (b[i]));
			{
				register CARD32 d = c&0x300C0300 ;
				if( d )
				{
					if( c&0x30000000 )
						d |= 0x0FF00000;
					if( c&0x000C0000 )
						d |= 0x0003FC00 ;
					if( c&0x00000300 )
						d |= 0x000000FF ;
					c ^= d;
				}
/*fprintf( stderr, "c = 0x%X, d = 0x%X, c^d = 0x%X\n", c, d, c^d );*/
			}
		}while(1);
	else
		do
		{
			src[i] = ENCODE_LSBF_555((c>>13),(c>>8),(c>>3));
			if( --i < 0 )
				break;
			/* carry over quantization error allow for error diffusion:*/
			c = ((c>>1)&0x00300C03)+((r[i]<<20) | (g[i]<<10) | (b[i]));
			{
				register CARD32 d = c&0x300C0300 ;
				if( d )
				{
					if( c&0x30000000 )
						d |= 0x0FF00000;
					if( c&0x000C0000 )
						d |= 0x0003FC00 ;
					if( c&0x00000300 )
						d |= 0x000000FF ;
					c ^= d;
				}
/*fprintf( stderr, "c = 0x%X, d = 0x%X, c^d = 0x%X\n", c, d, c^d );*/
			}
		}while(1);
}


