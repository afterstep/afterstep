/*
 * Copyright (c) 1998, 1999 Ethan Fischer <allanon@crystaltokyo.com>
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

#include "../configure.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

/* #define LOCAL_DEBUG */

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/parse.h"
#include "../include/misc.h"
#include "../include/style.h"
#include "../include/loadimg.h"
#include "../include/parser.h"
#include "../include/confdefs.h"
#include "../include/mystyle.h"
/*#include "../include/ascolor.h"*/
/*#include "../include/stepgfx.h"*/
#include "../libAfterImage/afterimage.h"
#include "../include/screen.h"

/*
 * if you add a member to this list, or to the MyStyle structure,
 * remember to update mystyle_new(), mystyle_delete(), mystyle_merge_styles(),
 * mystyle_parse(), mystyle_get_property(), and mystyle_set_property()
 */
struct config mystyle_config[] =
{
  {"~MyStyle", set_func_arg, (char **) F_DONE},
  {"Inherit", set_func_arg, (char **) F_INHERIT},
  {"Font", set_func_arg, (char **) F_FONT},
  {"ForeColor", set_func_arg, (char **) F_FORECOLOR},
  {"BackColor", set_func_arg, (char **) F_BACKCOLOR},
  {"TextStyle", set_func_arg, (char **) F_TEXTSTYLE},
#ifndef NO_TEXTURE
  {"MaxColors", set_func_arg, (char **) F_MAXCOLORS},
  {"BackGradient", set_func_arg, (char **) F_BACKGRADIENT},
  {"BackMultiGradient", set_func_arg, (char **) F_BACKMULTIGRADIENT},
  {"BackPixmap", set_func_arg, (char **) F_BACKPIXMAP},
  {"BackTransPixmap", set_func_arg, (char **) F_BACKTRANSPIXMAP},
#endif
  {"DrawTextBackground", set_func_arg, (char **) F_DRAWTEXTBACKGROUND},
  {"", NULL, NULL}
};

MyStyle *mystyle_first;

/* the GCs are static, but they are still used outside mystyle.c */
static GC ForeGC = None;
static GC BackGC = None;
static GC ReliefGC = None;
static GC ShadowGC = None;

static int style_func;
static char *style_arg;

/* just a nice error printing function */
void
mystyle_error (char *format, char *name, char *text2)
{
  fprintf (stderr, "\n%s: MyStyle [%s] parsing error:", MyName, name);
  if (text2)
    fprintf (stderr, format, text2);
  else
    fprintf (stderr, format);
}

/* Creates tinting GC for drawable d, with color tintColor,
   using color combining function function */
GC
CreateTintGC (Drawable d, unsigned long tintColor, int function)
{
  XGCValues gcvalue;
  if (tintColor == None || d == None)
    return None;

  gcvalue.function = function;
  gcvalue.foreground = tintColor;
  gcvalue.background = tintColor;
  return XCreateGC (dpy, d, GCFunction | GCForeground | GCBackground,
		    &gcvalue);
}

CARD8
make_component_hilite( int cmp )
{
	if( cmp < 51 )
		cmp = 51 ;
	cmp = (cmp*14)/10 ;
	return (cmp > 255)?255:cmp;
}

/* This routine computes the hilite color from the background color */
inline ARGB32
GetHilite (ARGB32 background)
{
	return ((make_component_hilite((background&0xFF000000)>>24)<<24)&0xFF000000)|
		   ((make_component_hilite((background&0x00FF0000)>>16)<<16)&0x00FF0000)|
		   ((make_component_hilite((background&0x0000FF00)>>8 )<<8 )&0x0000FF00)|
		   ((make_component_hilite((background&0x000000FF)    )    )&0x000000FF);
}

/* This routine computes the shadow color from the background color */
inline ARGB32
GetShadow (ARGB32 background)
{
	return (background>>1)&0x7F7F7F7F;
}

inline ARGB32
GetAverage (ARGB32 foreground, ARGB32 background)
{
	CARD16 a, r, g, b ;
	a = ARGB32_ALPHA8(foreground)+ARGB32_ALPHA8(background) ;
	a /= 2 ;
	r = ARGB32_RED8(foreground)+ARGB32_RED8(background) ;
	r /= 2 ;
	g = ARGB32_GREEN8(foreground)+ARGB32_GREEN8(background) ;
	g /= 2 ;
	b = ARGB32_BLUE8(foreground)+ARGB32_BLUE8(background) ;
	b /= 2 ;
	return MAKE_ARGB32(a,r,g,b);
}




/* create the default style if necessary, and fill in the unset fields
 * of the other styles from the default */
void
mystyle_fix_styles (void)
{
  MyStyle *dflt;
  MyStyle *style;

/*
 * the user may not have set the default style, so set it here
 * all we need are reasonable defaults, and most defaults have been set
 *  by mystyle_new() already
 * we need FONT, FORECOLOR, and BACKCOLOR, at a minimum
 */
  if ((dflt = mystyle_find ("default")) == NULL)
    dflt = mystyle_new_with_name ("default");
  if ((dflt->set_flags & F_FORECOLOR) == 0)
    dflt->user_flags |= F_FORECOLOR;
  if ((dflt->set_flags & F_BACKCOLOR) == 0)
    {
      dflt->relief.fore = GetHilite (dflt->colors.back);
      dflt->relief.back = GetShadow (dflt->colors.back);
      dflt->user_flags |= F_BACKCOLOR;
    }
  if ((dflt->set_flags & F_FONT) == 0)
    {
      if (load_font (NULL, &dflt->font) == False)
	{
	  fprintf (stderr, "%s: couldn't load default font", MyName);
	  exit (1);
	}
      dflt->user_flags |= F_FONT;
    }
  dflt->set_flags = dflt->inherit_flags | dflt->user_flags;

/* fix up the styles, using the default style */
  for (style = mystyle_first; style != NULL; style = style->next)
    mystyle_merge_styles (dflt, style, False, False);
}

static void mystyle_draw_textline (Window w, Drawable text_trg, MyStyle * style, const char *text, int len, int x, int y);

#ifdef I18N
#undef FONTSET
#define FONTSET (*style).font.fontset
#endif

void
mystyle_get_text_geometry (MyStyle * style, const char *str, int len, int *width, int *height)
{
  const char *ptr;
  int mw = 0, mh = 0;

  while (len > 0)
    {
      int w;
      for (ptr = str; len > 0 && *ptr != '\n'; ptr++, len--);
      w = XTextWidth (style->font.font, str, ptr - str);
      if (mw < w)
	mw = w;
      mh += style->font.height;
      str = ptr + (*ptr == '\n');
    }
  if (width != NULL)
    *width = mw;
  if (height != NULL)
    *height = mh;
}

static void
mystyle_do_draw_text (Window w, Drawable text_trg, MyStyle * style, const char *text, int x, int y)
{
  while (*text != '\0')
    {
      const char *ptr;
      for (ptr = text; *ptr != '\0' && *ptr != '\n'; ptr++);
      mystyle_draw_textline (w, text_trg, style, text, ptr - text, x, y);
      y += style->font.height;
      text = (*ptr == '\n') ? ptr + 1 : ptr;
    }
}

void
mystyle_draw_texturized_text (Window w, MyStyle *style, MyStyle *fore_texture, const char *text, int x, int y)
{
	int width, height ;
	Pixmap fore_pix = None;
	
    mystyle_set_global_gcs (style);
	mystyle_get_text_geometry (style, text, strlen(text), &width, &height);
	if ((style->set_flags & F_DRAWTEXTBACKGROUND) && (style->flags & F_DRAWTEXTBACKGROUND))
    {
  		XSetFillStyle (dpy, BackGC, FillSolid);
    	XFillRectangle (dpy, w, BackGC, x - 2, y - style->font.y - 2, width + 4, height + 4);
#ifndef NO_TEXTURE
    	if ((style->texture_type != 0) && (style->back_icon.pix != None))
			XSetFillStyle (dpy, BackGC, FillTiled);
#endif /* !NO_TEXTURE */
    }
	
    if( fore_texture != NULL ) 
	{
		if( fore_texture->texture_type <= TEXTURE_PIXMAP )
			fore_pix = mystyle_make_pixmap( fore_texture, width, height, None );
		else
		{
			Window ch ;
			int root_x = x, root_y = y;
			XTranslateCoordinates( dpy, w, Scr.Root, x, y, &root_x, &root_y, &ch );
			fore_pix = mystyle_make_pixmap_overlay( fore_texture, root_x, root_y, width, height, None );
		}
	}
	
	if( fore_pix != None ) 
	{
	    Pixmap mask;
		GC old_fore_gc = ForeGC ;
		XGCValues gcv;
		
		mask = XCreatePixmap (dpy, Scr.Root, width + 1, height + 1, 1);
	    gcv.foreground = 0;
	    gcv.function = GXcopy;
	    gcv.font = style->font.font->fid;
	    ForeGC = XCreateGC (dpy, mask, GCFunction | GCForeground | GCFont, &gcv);
	    XFillRectangle (dpy, mask, ForeGC, 0, 0, width, height);
	    XSetForeground (dpy, ForeGC, 1);
		
		mystyle_do_draw_text ( w, mask, style, text, x, y);
		
		XFreeGC( dpy, ForeGC );
		ForeGC = old_fore_gc ;
		XSetClipOrigin (dpy, ForeGC, x, y);
	    XSetClipMask (dpy, ForeGC, mask);
	    XCopyArea (dpy, fore_pix, w, ForeGC, 0, 0, width, height, x, y);
		XSetClipMask (dpy, ForeGC, None);
		XFreePixmap( dpy, mask );
		XFreePixmap( dpy, fore_pix );
	}else
		mystyle_do_draw_text ( w, w, style, text, x, y);
}

void
mystyle_draw_text (Window w, MyStyle * style, const char *text, int x, int y)
{
	mystyle_draw_texturized_text (w, style, NULL, text, x, y);
}

void
mystyle_draw_vertical_text (Window w, MyStyle * style, const char *text, int x, int y)
{
	char *rotated = make_tricky_text( (char*)text );
	if( rotated )
	{
		mystyle_draw_texturized_text (w, style, NULL, rotated, x, y);
		free( rotated );
	}
}

void
mystyle_draw_texturized_vertical_text (Window w, MyStyle * style, MyStyle *fore_texture, const char *text, int x, int y)
{
	char *rotated = make_tricky_text( (char*)text );
	if( rotated )
	{
		mystyle_draw_texturized_text (w, style, fore_texture, rotated, x, y);
		free( rotated );
	}
}

static void
mystyle_draw_textline (Window w, Drawable text_trg, MyStyle * style, const char *text, int len, int x, int y)
{
  switch (style->text_style)
    {
/* 3d look #1 */
    case 1:
      XDrawString (dpy, w, ReliefGC, x - 2, y - 2, text, len);
      XDrawString (dpy, w, ReliefGC, x - 1, y - 2, text, len);
      XDrawString (dpy, w, ReliefGC, x + 0, y - 2, text, len);
      XDrawString (dpy, w, ReliefGC, x + 1, y - 2, text, len);
      XDrawString (dpy, w, ReliefGC, x + 2, y - 2, text, len);

      XDrawString (dpy, w, ReliefGC, x - 2, y - 1, text, len);
      XDrawString (dpy, w, ReliefGC, x - 2, y + 0, text, len);
      XDrawString (dpy, w, ReliefGC, x - 2, y + 1, text, len);
      XDrawString (dpy, w, ReliefGC, x - 2, y + 2, text, len);

      XDrawString (dpy, w, ShadowGC, x + 2, y + 0, text, len);
      XDrawString (dpy, w, ShadowGC, x + 2, y + 1, text, len);
      XDrawString (dpy, w, ShadowGC, x + 2, y + 2, text, len);
      XDrawString (dpy, w, ShadowGC, x + 1, y + 2, text, len);
      XDrawString (dpy, w, ShadowGC, x + 0, y + 2, text, len);

      XDrawString (dpy, text_trg, ForeGC, x + 0, y + 0, text, len);
      break;
/* 3d look #2 */
    case 2:
      XDrawString (dpy, w, ShadowGC, x - 1, y - 1, text, len);
      XDrawString (dpy, w, ReliefGC, x + 0, y + 0, text, len);
      XDrawString (dpy, text_trg, ForeGC, x + 1, y + 1, text, len);
      break;
/* normal text */
    default:
    case 0:
      XDrawString (dpy, text_trg, ForeGC, x + 0, y + 0, text, len);
      break;
    }
}

int
mystyle_translate_grad_type( int type )
{
	switch( type ) 
	{
  		case TEXTURE_GRADIENT   : return  GRADIENT_TopLeft2BottomRight ;
		
	    case TEXTURE_HGRADIENT  : 
		case TEXTURE_HCGRADIENT : return  GRADIENT_Left2Right ;
		
	    case TEXTURE_VGRADIENT  : 
  		case TEXTURE_VCGRADIENT : return GRADIENT_Top2Bottom ;
	    case TEXTURE_GRADIENT_TL2BR : return GRADIENT_TopLeft2BottomRight ;
	    case TEXTURE_GRADIENT_BL2TR : return  GRADIENT_BottomLeft2TopRight ;
	    case TEXTURE_GRADIENT_T2B :   return  GRADIENT_Top2Bottom ;
	    case TEXTURE_GRADIENT_L2R :   return  GRADIENT_Left2Right ;
		default : return -1 ;
	}
}

static merge_scanlines_func mystyle_merge_scanlines_func_xref[] =
{
   allanon_scanlines,
   alphablend_scanlines,
   add_scanlines,
   colorize_scanlines,
   darken_scanlines,
   diff_scanlines,
   dissipate_scanlines,
   hue_scanlines,
   lighten_scanlines,
   overlay_scanlines,
   saturate_scanlines,
   screen_scanlines,
   sub_scanlines,
   tint_scanlines,
   value_scanlines,
   /* just a filler below : */
   alphablend_scanlines,
   alphablend_scanlines,
   alphablend_scanlines,
   NULL
};

ASImage *
mystyle_make_image( MyStyle * style, int root_x, int root_y, int width, int height )
{
    ASImage *im = NULL;
	Pixmap root_pixmap ;
	unsigned int root_w, root_h ;
#ifndef NO_TEXTURE
    if( width < 1 )    width = 1 ;
    if( height < 1 )   height = 1 ;
LOCAL_DEBUG_OUT( "style \"%s\", texture_type = %d, im = %p, tint = 0x%X", style->name, style->texture_type, style->back_icon.image, style->tint );
	switch( style->texture_type )
	{
	    case TEXTURE_SOLID :
			im = create_asimage(width, height, 0);
			im->back_color = style->colors.back ;
			break;
			
  		case TEXTURE_GRADIENT   : 
		
	    case TEXTURE_HGRADIENT  : 
		case TEXTURE_HCGRADIENT : 
		
	    case TEXTURE_VGRADIENT  : 
  		case TEXTURE_VCGRADIENT : 
	    case TEXTURE_GRADIENT_TL2BR :
	    case TEXTURE_GRADIENT_BL2TR :
	    case TEXTURE_GRADIENT_T2B :  
	    case TEXTURE_GRADIENT_L2R :   
			im = make_gradient( Scr.asv, &(style->gradient), width, height, 0xFFFFFFFF, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT  );
			break;
		case TEXTURE_SHAPED_SCALED_PIXMAP :
		case TEXTURE_SCALED_PIXMAP :
			im = scale_asimage( Scr.asv, style->back_icon.image, 
			                    width, height, 
							    ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT  );
			break;
		case TEXTURE_SHAPED_PIXMAP :
		case TEXTURE_PIXMAP :
			im = tile_asimage( Scr.asv, style->back_icon.image, 
			                   0, 0, width, height, TINT_LEAVE_SAME, 
							   ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT  );
			break;
		default :
			if( Scr.RootImage == NULL )  
			{
				root_pixmap = ValidatePixmap (None, 1, 1, &root_w, &root_h);
				if( root_pixmap )
				{
#if 0	
					Scr.RootImage = pixmap2ximage( Scr.asv, root_pixmap, 0, 0, root_w, root_h, AllPlanes, 100 );
#else
					Scr.RootImage = pixmap2asimage( Scr.asv, root_pixmap, 0, 0, root_w, root_h, AllPlanes, False, 100 );
#endif		
				}
			}else
			{
				root_w = Scr.RootImage->width ;
				root_h = Scr.RootImage->height ;
			}
LOCAL_DEBUG_OUT( "RootImage = %p", Scr.RootImage );

			if ( Scr.RootImage == NULL )
				return NULL ;
				
			if ( style->texture_type == TEXTURE_TRANSPARENT || 
				 style->texture_type == TEXTURE_TRANSPARENT_TWOWAY )
			{
				im = tile_asimage ( Scr.asv, Scr.RootImage, root_x, root_y,
  				  				    width,  height, style->tint,
									ASA_ASImage,
									0, ASIMAGE_QUALITY_DEFAULT );
			}else
			{
				ASImageLayer layers[2];
				ASImage *scaled_im = NULL ;
				int index = 0 ; 

	  			init_image_layers( &layers[0], 2 );

				layers[0].im = Scr.RootImage;
				if( style->texture_type >= TEXTURE_SCALED_TRANSPIXMAP &&
				  	style->texture_type < TEXTURE_SCALED_TRANSPIXMAP_END )
					index = style->texture_type-TEXTURE_SCALED_TRANSPIXMAP ;
				else if( style->texture_type >= TEXTURE_TRANSPIXMAP &&
				  		 style->texture_type < TEXTURE_TRANSPIXMAP_END )
					index = style->texture_type-TEXTURE_TRANSPIXMAP ;
					
LOCAL_DEBUG_OUT( "index = %d", index );
	  	  		layers[0].merge_scanlines = mystyle_merge_scanlines_func_xref[index] ;
				layers[0].dst_x = 0 ;
				layers[0].dst_y = 0 ;
				layers[0].clip_x = root_x ;
				layers[0].clip_y = root_y ;
				layers[0].clip_width = width ;
				layers[0].clip_height = height ;

			    layers[1].im = style->back_icon.image ;
				if( style->texture_type >= TEXTURE_SCALED_TRANSPIXMAP &&
				  	style->texture_type < TEXTURE_SCALED_TRANSPIXMAP_END )
	    		{
					scaled_im = scale_asimage( Scr.asv, layers[1].im, width, height, 
											   ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );
					if( scaled_im ) 
						layers[1].im = scaled_im ;
				}
				layers[1].merge_scanlines = layers[0].merge_scanlines ;		
				layers[1].dst_x = 0 ;
				layers[1].dst_y = 0 ;
				layers[1].clip_x = 0 ;
		  		layers[1].clip_y = 0 ;
				layers[1].clip_width = width ;
  				layers[1].clip_height = height ;

				im = merge_layers( Scr.asv, &layers[0], 2,
		      	                   width, height,
								   ASA_ASImage,
							       0, ASIMAGE_QUALITY_DEFAULT );
				if( scaled_im ) 
					destroy_asimage( &scaled_im );
			}
	}
#endif /* NO_TEXTURE */
	return im ;
}

icon_t
mystyle_make_icon (MyStyle * style, int width, int height, Pixmap cache)
{
	icon_t icon = {None, None, 0, 0};
#ifndef NO_TEXTURE
	icon.image = mystyle_make_image( style, 0, 0, width, height );
	if( icon.image )
	{
		icon.pix = asimage2pixmap( Scr.asv, Scr.Root, icon.image, NULL, True );
		if( style->texture_type <= TEXTURE_PIXMAP ) 
			icon.mask = asimage2mask  ( Scr.asv, Scr.Root, icon.image, NULL, False );
        icon.width = icon.image->width;
	    icon.height = icon.image->height;
	}
#endif /* NO_TEXTURE */
	return icon;
}

void 
mystyle_free_icon_resources( icon_t icon ) 
{
	if( icon.pix ) 
		UnloadImage( icon.pix );
	if( icon.mask ) 
		UnloadMask( icon.mask );
	if( icon.image ) 
		safe_asimage_destroy(icon.image);
}

icon_t
mystyle_make_icon_overlay (MyStyle * style, int root_x, int root_y, int width, int height, Pixmap cache)
{
  icon_t icon = {None, None, 0, 0};
#ifndef NO_TEXTURE
	icon.image = mystyle_make_image( style, root_x, root_y, width, height );
	if( icon.image )
	{
		icon.pix = asimage2pixmap( Scr.asv, Scr.Root, icon.image, NULL, False );
		if( style->texture_type <= TEXTURE_PIXMAP ) 
			icon.mask = asimage2mask  ( Scr.asv, Scr.Root, icon.image, NULL, False );
        icon.width = icon.image->width;
	    icon.height = icon.image->height;
	}
#endif /* NO_TEXTURE */
  return icon;
}

Pixmap
mystyle_make_pixmap (MyStyle * style, int width, int height, Pixmap cache)
{
  Pixmap p ;
  icon_t icon = mystyle_make_icon (style, width, height, cache);
  p = icon.pix ;
  icon.pix = None ;
  mystyle_free_icon_resources( icon );
  return p ;
}

Pixmap
mystyle_make_pixmap_overlay (MyStyle * style, int root_x, int root_y, int width, int height, Pixmap cache)
{
  Pixmap p ;
  icon_t icon = mystyle_make_icon_overlay (style, root_x, root_y, width, height, cache);
  p = icon.pix ;
  icon.pix = None ;
  mystyle_free_icon_resources( icon );
  return p ;
}

/* set a window's background for XClearArea */
void
mystyle_set_window_background (Window w, MyStyle * style)
{
#ifndef NO_TEXTURE
  if (style->texture_type == 128 && style->set_flags & F_BACKPIXMAP)
    XSetWindowBackgroundPixmap (dpy, w, style->back_icon.pix);
  else if (style->texture_type == 129)
    XSetWindowBackgroundPixmap (dpy, w, ParentRelative);
  else
#endif /* NO_TEXTURE */
    XSetWindowBackground (dpy, w, style->colors.back);
}

/*
 * set the standard global drawing GCs
 * avoids resetting the GCs when possible
 * if passed a NULL pointer, forces update of the cached GCs if possible
 * WARNING: the GCs are invalid until this function is called
 */
void
mystyle_set_global_gcs (MyStyle * style)
{
  static MyStyle *old_style = NULL;
  static int make_GCs = 1;

/* make the drawing GCs if necessary */
  if (make_GCs)
    {
      XGCValues gcv;
      unsigned long gcm;

      gcv.graphics_exposures = False;
      gcm = GCGraphicsExposures;
      ForeGC = create_visual_gc (Scr.asv, Scr.Root, gcm, &gcv);
      BackGC = create_visual_gc (Scr.asv, Scr.Root, gcm, &gcv);
      ReliefGC = create_visual_gc (Scr.asv, Scr.Root, gcm, &gcv);
      ShadowGC = create_visual_gc (Scr.asv, Scr.Root, gcm, &gcv);
      make_GCs = 0;
    }

/* set the GCs if the style is different */
  if (style == NULL && old_style != NULL)
    {
      MyStyle *s;
      /* force update the current style if it still exists */
      for (s = mystyle_first; s != NULL && s != old_style; s = s->next);
      if (s != NULL)
	mystyle_set_gcs (s, ForeGC, BackGC, ReliefGC, ShadowGC);
    }
  else if (old_style != style)
    {
      old_style = style;
      mystyle_set_gcs (style, ForeGC, BackGC, ReliefGC, ShadowGC);
    }
}

void
mystyle_set_gcs (MyStyle * style, GC foreGC, GC backGC, GC reliefGC, GC shadowGC)
{
  XGCValues gcv;
  unsigned long gcm;

/* set the drawing GC */
  ARGB2PIXEL(Scr.asv, style->colors.fore, &gcv.foreground);
  ARGB2PIXEL(Scr.asv, style->colors.back, &gcv.background);
  gcv.font = style->font.font->fid;
  gcv.fill_style = FillSolid;
  gcm = GCForeground | GCBackground | GCFont | GCFillStyle;
  if (foreGC != None)
    XChangeGC (dpy, foreGC, gcm, &gcv);

  ARGB2PIXEL(Scr.asv, style->colors.back, &gcv.foreground);
#ifndef NO_TEXTURE
  if ((style->texture_type != 0) && (style->back_icon.pix != None))
    {
      gcv.fill_style = FillTiled;
      gcv.tile = style->back_icon.pix;
      gcm |= GCTile;
    }
#endif /* NO_TEXTURE */
  if (backGC != None)
    XChangeGC (dpy, backGC, gcm, &gcv);

/* set the relief GC */
  ARGB2PIXEL(Scr.asv, style->relief.fore, &gcv.foreground);
  ARGB2PIXEL(Scr.asv, style->relief.back, &gcv.background);
  gcv.fill_style = FillSolid;
  gcm = GCForeground | GCBackground | GCFont | GCFillStyle;
  if (reliefGC != None)
    XChangeGC (dpy, reliefGC, gcm, &gcv);

/* set the shadow GC */
  ARGB2PIXEL(Scr.asv, style->relief.back, &gcv.foreground);
  ARGB2PIXEL(Scr.asv, style->relief.fore, &gcv.background);
  gcv.fill_style = FillSolid;
  gcm = GCForeground | GCBackground | GCFont | GCFillStyle;
  if (shadowGC != None)
    XChangeGC (dpy, shadowGC, gcm, &gcv);
}

void
mystyle_get_global_gcs (MyStyle * style, GC * foreGC, GC * backGC, GC * reliefGC, GC * shadowGC)
{
  mystyle_set_global_gcs (style);
  if (foreGC != NULL)
    *foreGC = ForeGC;
  if (backGC != NULL)
    *backGC = BackGC;
  if (reliefGC != NULL)
    *reliefGC = ReliefGC;
  if (shadowGC != NULL)
    *shadowGC = ShadowGC;
}

MyStyle *
mystyle_new (void)
{
  MyStyle *style = (MyStyle *) safemalloc (sizeof (MyStyle));

  style->next = mystyle_first;
  mystyle_first = style;

  style->user_flags = 0;
  style->inherit_flags = 0;
  style->set_flags = 0;
  style->flags = 0;
  style->name = NULL;
  style->text_style = 0;
  style->font.name = NULL;
  style->font.font = NULL;
#ifdef I18N
  style->font.fontset = NULL;
#endif
  style->colors.fore = ARGB32_White;
  style->colors.back = ARGB32_Black;
  style->relief.fore = style->colors.back;
  style->relief.back = style->colors.fore;
  style->texture_type = 0;
#ifndef NO_TEXTURE
  style->gradient.npoints = 0;
  style->gradient.color = NULL;
  style->gradient.offset = NULL;
  style->back_icon.pix = None;
  style->back_icon.mask = None;
  if (Scr.d_depth > 16)
    style->max_colors = 128;
  else if (Scr.d_depth > 8)
    style->max_colors = 32;
  else
    style->max_colors = 10;
  style->tint = TINT_LEAVE_SAME;
  style->back_icon.image = NULL;
#endif
  return style;
}

MyStyle *
mystyle_new_with_name (char *name)
{
  MyStyle *style = mystyle_new ();

  if (name != NULL)
    {
      style->name = (char *) safemalloc (strlen (name) + 1);
      strcpy (style->name, name);
    }
  return style;
}

/*
 * deletes memory alloc'd for style (including members)
 */

void
mystyle_delete (MyStyle * style)
{
  MyStyle *s1;
  MyStyle *s2;

  /* remove ourself from the list */
  for (s1 = s2 = mystyle_first; s1 != NULL; s2 = s1, s1 = s1->next)
    if (s1 == style)
      break;
  if (s1 != NULL)
    {
      if (mystyle_first == s1)
	mystyle_first = s1->next;
      else
	s2->next = s1->next;
    }
  /* delete members */
  if (style->name != NULL)
    free (style->name);
  if (style->user_flags & F_FONT)
    {
      unload_font (&style->font);
    }
#ifndef NO_TEXTURE
  if (style->user_flags & F_BACKGRADIENT)
    {
      free (style->gradient.color);
      free (style->gradient.offset);
    }
  if (style->user_flags & F_BACKPIXMAP)
    {
      UnloadImage (style->back_icon.pix);
    }
  if (style->back_icon.image)
    destroy_asimage(&(style->back_icon.image));
#endif

  /* free our own mem */
  free (style);
}

/* find a style by name */
MyStyle *
mystyle_find (const char *name)
{
  MyStyle *style = NULL;
  if (name != NULL)
    for (style = mystyle_first; style != NULL; style = style->next)
      if (!mystrcasecmp (style->name, name))
	break;
  return style;
}

/* find a style by name or return the default style */
MyStyle *
mystyle_find_or_default (const char *name)
{
  MyStyle *style = mystyle_find (name);
  if (style == NULL)
    style = mystyle_find ("default");
  return style;
}

/*
 * merges two styles, with the result in child
 * if override == False, will not override fields already set
 * if copy == True, copies instead of inheriting; this is important, because
 *   inherited members are deleted when their real parent is deleted; don't
 *   inherit if the parent style could go away before the child
 */
void
mystyle_merge_styles (MyStyle * parent, MyStyle * child, Bool override, Bool copy)
{
  if (parent->set_flags & F_FONT)
    {
      if ((override == True) && (child->user_flags & F_FONT))
	unload_font (&child->font);
      if ((override == True) || !(child->set_flags & F_FONT))
	{
	  if (copy == False)
	    {
#ifdef I18N
	      load_font (parent->font.name, &child->font);
#else
	      child->font = parent->font;
#endif
	      child->user_flags &= ~F_FONT;
	      child->inherit_flags |= F_FONT;
	    }
	  else
	    {
	      if (load_font (parent->font.name, &child->font) == True)
		{
		  child->user_flags |= F_FONT;
		  child->inherit_flags &= ~F_FONT;
		}
	    }
	}
    }
  if (parent->set_flags & F_TEXTSTYLE)
    {
      if ((override == True) || !(child->set_flags & F_TEXTSTYLE))
	{
	  child->text_style = parent->text_style;

	  if (copy == False)
	    {
	      child->user_flags &= ~F_TEXTSTYLE;
	      child->inherit_flags |= F_TEXTSTYLE;
	    }
	  else
	    {
	      child->user_flags |= F_TEXTSTYLE;
	      child->inherit_flags &= ~F_TEXTSTYLE;
	    }
	}
    }
  if (parent->set_flags & F_FORECOLOR)
    {
      if ((override == True) || !(child->set_flags & F_FORECOLOR))
	{
	  if (override == True)
	    child->texture_type = parent->texture_type;
	  child->colors.fore = parent->colors.fore;
	  if (copy == False)
	    {
	      child->user_flags &= ~F_FORECOLOR;
	      child->inherit_flags |= F_FORECOLOR;
	    }
	  else
	    {
	      child->user_flags |= F_FORECOLOR;
	      child->inherit_flags &= ~F_FORECOLOR;
	    }
	}
    }
  if (parent->set_flags & F_BACKCOLOR)
    {
      if ((override == True) || !(child->set_flags & F_BACKCOLOR))
	{
	  child->colors.back = parent->colors.back;
	  child->relief = parent->relief;
	  if (copy == False)
	    {
	      child->user_flags &= ~F_BACKCOLOR;
	      child->inherit_flags |= F_BACKCOLOR;
	    }
	  else
	    {
	      child->user_flags |= F_BACKCOLOR;
	      child->inherit_flags &= ~F_BACKCOLOR;
	    }
	}
    }
#ifndef NO_TEXTURE
  if (parent->set_flags & F_MAXCOLORS)
    {
      if ((override == True) || !(child->set_flags & F_MAXCOLORS))
	{
	  child->max_colors = parent->max_colors;
	  if (copy == False)
	    {
	      child->user_flags &= ~F_MAXCOLORS;
	      child->inherit_flags |= F_MAXCOLORS;
	    }
	  else
	    {
	      child->user_flags |= F_MAXCOLORS;
	      child->inherit_flags &= ~F_MAXCOLORS;
	    }
	}
    }
  if (parent->set_flags & F_BACKGRADIENT)
    {
      if ((override == True) || !(child->set_flags & F_BACKGRADIENT))
	{
	  if (override == True)
	    child->texture_type = parent->texture_type;
	  child->gradient = parent->gradient;
	  if (copy == False)
	    {
	      child->user_flags &= ~F_BACKGRADIENT;
	      child->inherit_flags |= F_BACKGRADIENT;
	    }
	  else
	    {
	      child->user_flags |= F_BACKGRADIENT;
	      child->inherit_flags &= ~F_BACKGRADIENT;
	    }
	}
    }
	if (parent->set_flags & F_BACKPIXMAP)
    {
    	if ((override == True) && (child->user_flags & F_BACKPIXMAP))
		{
			mystyle_free_icon_resources( child->back_icon );
			memset( &(child->back_icon), 0x00, sizeof(child->back_icon));
		}
    	if ((override == True) || !(child->set_flags & F_BACKPIXMAP))
		{
			if (override == True)
	  			child->texture_type = parent->texture_type;
			if ( (parent->texture_type == TEXTURE_TRANSPARENT || 
			      parent->texture_type == TEXTURE_TRANSPARENT_TWOWAY) && 
			     (override == True || 
				  (child->texture_type != TEXTURE_TRANSPARENT && 
				   child->texture_type != TEXTURE_TRANSPARENT_TWOWAY)))
		    {
    			child->tint = parent->tint;
		    }
			if ( !copy )
	  		{
	    		child->back_icon = parent->back_icon;
	    		clear_flags(child->user_flags, F_BACKPIXMAP|F_BACKTRANSPIXMAP);
	    		set_flags(child->inherit_flags, F_BACKPIXMAP );
				if( get_flags( parent->set_flags, F_BACKTRANSPIXMAP ) )
					set_flags( child->inherit_flags, F_BACKTRANSPIXMAP );
	  		}else
	  		{
	    		GC gc = create_visual_gc (Scr.asv, Scr.Root, 0, NULL);
		    	child->back_icon.pix = XCreatePixmap (dpy, Scr.Root, parent->back_icon.width, parent->back_icon.height, Scr.asv->visual_info.depth);
		    	XCopyArea (	dpy, parent->back_icon.pix, child->back_icon.pix, gc,
	  						0, 0, parent->back_icon.width, parent->back_icon.height, 0, 0);
	    		if (parent->back_icon.mask != None)
				{
					GC mgc = XCreateGC (dpy, parent->back_icon.mask, 0, NULL);
					child->back_icon.pix = XCreatePixmap (dpy, Scr.Root, parent->back_icon.width, parent->back_icon.height, 1);
					XCopyArea (dpy, parent->back_icon.pix, child->back_icon.pix, mgc,
			  				   0, 0, parent->back_icon.width, parent->back_icon.height, 0, 0);
					XFreeGC (dpy, mgc);
				}
				if( parent->back_icon.image ) 
					child->back_icon.image = dup_asimage(parent->back_icon.image);
				else 
				    child->back_icon.image = 0 ;
	  			child->back_icon.width = parent->back_icon.width;
	    		child->back_icon.height = parent->back_icon.height;
		    	child->user_flags |= F_BACKPIXMAP | (parent->set_flags & F_BACKTRANSPIXMAP);
		    	child->inherit_flags &= ~(F_BACKPIXMAP | F_BACKTRANSPIXMAP);
		    	XFreeGC (dpy, gc);
	  		}
		}
    }
#endif /* NO_TEXTURE */
    if (parent->set_flags & F_DRAWTEXTBACKGROUND)
    {
      if ((override == True) || !(child->set_flags & F_DRAWTEXTBACKGROUND))
	{
	  child->flags &= ~F_DRAWTEXTBACKGROUND;
	  child->flags |= parent->flags & F_DRAWTEXTBACKGROUND;
	  if (copy == False)
	    {
	      child->user_flags &= ~F_DRAWTEXTBACKGROUND;
	      child->inherit_flags |= F_DRAWTEXTBACKGROUND;
	    }
	  else
	    {
	      child->user_flags |= F_DRAWTEXTBACKGROUND;
	      child->inherit_flags &= ~F_DRAWTEXTBACKGROUND;
	    }
	}
    }
  child->set_flags = child->user_flags | child->inherit_flags;
}

void
mystyle_parse (char *tline, FILE * fd, char **ppath, int *junk2)
{
  MyStyle *style;
  char *newline;
  char *name = stripcpy2 (tline, 0);

  if( Scr.image_manager == NULL )
  {
	  char *pixmap_path = *ppath ;
	  if( pixmap_path == NULL )  
		  pixmap_path = getenv( "IMAGE_PATH" );
	  if( pixmap_path == NULL )  
		  pixmap_path = getenv( "PATH" );
	  Scr.image_manager = create_image_manager( NULL, 2.2, pixmap_path, NULL );
  }
  
  if (name == NULL)
    {
      fprintf (stderr, "%s: bad style name '%s'", MyName, tline);
      return;
    }
  newline = safemalloc (MAXLINELENGTH + 1);
/* if this style was already defined, find it */
  if ((style = mystyle_find (name)) != NULL)
    {
      free (name);
      name = style->name;
    }
  else
    {
      style = mystyle_new ();
      style->name = name;
    }

  while (fgets (newline, MAXLINELENGTH, fd))
    {
      char *p;
      p = stripcomments (newline);
      if (*p != '\0')
	if (mystyle_parse_member (style, p, *ppath) != False)
	  break;
    }

  free (newline);
}

/*
 * parse a style member, for example:
 *   mystyle_parse_member(mystyle_find("default"), "BackColor black");
 * this function will likely modify the string argument
 * returns 1 when a "~MyStyle" is parsed
 */
int
mystyle_parse_member (MyStyle * style, char *str, const char *PixmapPath)
{
  int done = 0;
  struct config *config = find_config (mystyle_config, str);

  style_func = F_ERROR;
  style_arg = NULL;

  if (config != NULL)
    config->action (str + strlen (config->keyword), NULL, config->arg, config->arg2);
  else
    fprintf (stderr, "%s: unknown style command: %s\n", MyName, str);
  if (style_arg == NULL)
    {
      fprintf (stderr, "%s: bad style arg: %s\n", MyName, str);
      return 0;
    }
  else
    {
      style->inherit_flags &= ~style_func;
      switch (style_func)
	{
	case F_INHERIT:
	  {
	    MyStyle *parent = mystyle_find (style_arg);
	    if (parent != NULL)
	      mystyle_merge_styles (parent, style, True, False);
	    else
	      fprintf (stderr, "%s: unknown style: %s\n", MyName, style_arg);
	  }
	  break;
	case F_FONT:
	  if (style->user_flags & style_func)
	    unload_font (&style->font);
	  style->user_flags &= ~style_func;
	  if (load_font (style_arg, &style->font) == True)
	    style->user_flags |= style_func;
	  break;
	case F_TEXTSTYLE:
	  style->text_style = strtol (style_arg, NULL, 10);
	  style->user_flags |= style_func;
	  break;
	case F_FORECOLOR:
	  if( parse_argb_color( style_arg, &(style->colors.fore)) != style_arg )
		  style->user_flags |= style_func;
	  else
		  show_error( "in MyStyle \"%s\": unable to parse Forecolor \"%s\"", style->name, style_arg );
	  break;
	case F_BACKCOLOR:
	  style->texture_type = 0;
	  if( parse_argb_color( style_arg, &(style->colors.back)) != style_arg )
	  {
		  style->relief.fore = GetHilite (style->colors.back);
		  style->relief.back = GetShadow (style->colors.back);
		  style->user_flags |= style_func;
	  }else
		  show_error( "in MyStyle \"%s\": unable to parse Backcolor \"%s\"", style->name, style_arg );
	  break;
#ifndef NO_TEXTURE
	case F_MAXCOLORS:
	  style->max_colors = strtol (style_arg, NULL, 10);
	  style->user_flags |= style_func;
	  break;
	case F_BACKGRADIENT:
	  {
	    char *ptr, *ptr1;
	    int type = strtol (style_arg, &ptr, 10);
		ARGB32 color1 = 0, color2 = 0 ;
		register int i = 0;
		while( !isspace(ptr[i]) && ptr[i] != '\0' ) ++i;
		while( isspace(ptr[i]) ) ++i;
		if( ptr[i] == '\0' )
		{
		    show_error("missing gradient colors: %s", style_arg);
		}else
		{
			ptr = &ptr[i] ;
			ptr1 = (char*)parse_argb_color( ptr, &color1 );
/*show_warning("in MyStyle \"%s\":c1 = #%X(%s)", style->name, color1, ptr );			*/
			if( ptr1 != ptr )
			{
				int k = 0 ;
				while( isspace(ptr1[k]) ) ++k;
				ptr = &ptr1[k] ;
				if( parse_argb_color( ptr, &color2 ) != ptr )
				{
				    ASGradient gradient;
/*show_warning("in MyStyle \"%s\":c1 = #%X, c2 = #%X", style->name, color1, color2 );*/
	  				if ((type = mystyle_parse_old_gradient (type, color1, color2, &gradient)) >= 0)
				    {
						if (style->user_flags & F_BACKGRADIENT)
						{
		  					free (style->gradient.color);
			  				free (style->gradient.offset);
						}
						style->gradient.type = mystyle_translate_grad_type( type );
						style->gradient = gradient;
						style->texture_type = type;
						style->user_flags |= style_func;
		    		}
					else
						show_error( "in MyStyle \"%s\":invalid gradient type %d", style->name, type );
				}else
					show_error( "in MyStyle \"%s\":can't parse second color \"%s\"", style->name, ptr );
			}else
				show_error( "in MyStyle \"%s\":can't parse first color \"%s\"", style->name, ptr );
			if( !(style->user_flags&style_func) )
			    show_error("bad gradient: %s", style_arg);
		}
	  }
	  break;
	case F_BACKMULTIGRADIENT:
	  {
	    char *ptr, *ptr1;
	    int type = strtol (style_arg, &ptr, 10);
	    ASGradient gradient;
	    int error = 0;

	    gradient.npoints = 0;
	    gradient.color = NULL;
	    gradient.offset = NULL;
	    if (type < TEXTURE_GRADIENT_TL2BR || type >= TEXTURE_PIXMAP)
	      error = 4;
	    while (!error && ptr != NULL && *ptr != '\0')
	    {
			ARGB32 color ;
			ptr1 = (char*)parse_argb_color( ptr, &color );
			if( ptr1 == ptr ) 
				error = 1 ;
			else
			{
				double offset = 0.0;
				ptr = ptr1 ;
			    offset = strtod (ptr, &ptr1);
			    if (ptr == ptr1)
		    		error = 2;
				else
				{
  			  		ptr = ptr1;
				    gradient.npoints++;
				    gradient.color = realloc (gradient.color, sizeof (XColor) * gradient.npoints);
				    gradient.offset = realloc (gradient.offset, sizeof (double) * gradient.npoints);
				    gradient.color[gradient.npoints - 1] = color;
				    gradient.offset[gradient.npoints - 1] = offset;
				    for (; isspace (*ptr); ptr++);
				}	
			}
		}
	    if (!error)
    	{
			gradient.offset[0] = 0.0;
			gradient.offset[gradient.npoints - 1] = 1.0;
			if (style->user_flags & F_BACKGRADIENT)
			{
		  		free (style->gradient.color);
		  		free (style->gradient.offset);
			}
			style->gradient.type = mystyle_translate_grad_type( type );
			style->gradient = gradient;
			style->texture_type = type;
			style->user_flags |= F_BACKGRADIENT;
			style_func = F_BACKGRADIENT;
	    }
	    else
	    {
			if (gradient.color != NULL)
				free (gradient.color);
			if (gradient.offset != NULL)
				free (gradient.offset);
			fprintf (stderr, "%s: bad gradient (error %d): %s\n", MyName, error, style_arg);
	    }
	  }
	  break;
	case F_BACKPIXMAP:
	  {
	    char *ptr;
	    int type = strtol (style_arg, &ptr, 10);
	    char *tmp = stripcpy (ptr);

	    clear_flags( style->inherit_flags, F_BACKTRANSPIXMAP|F_BACKPIXMAP);
		mystyle_free_icon_resources(style->back_icon);
		memset( &(style->back_icon), 0x00, sizeof(style->back_icon));
		
		if( type < TEXTURE_TEXTURED_START || type >= TEXTURE_TEXTURED_END ) 
		{
			show_error( "Mystyle \"%s\" has unsupported texture type [%d] in BackPixmap setting. Assuming default of [128] instead.", style->name, type );
			type = TEXTURE_PIXMAP ;			
		}
		if( type == TEXTURE_TRANSPARENT || type == TEXTURE_TRANSPARENT_TWOWAY ) 
		{ /* treat second parameter as ARGB tint value : */
			if( parse_argb_color( tmp, &(style->tint)) == tmp )
				style->tint = TINT_LEAVE_SAME; /* use no tinting by default */
			else if( type == TEXTURE_TRANSPARENT ) 
				style->tint = (style->tint>>1)&0x7F7F7F7F; /* converting old style tint */
/*LOCAL_DEBUG_OUT( "tint is 0x%X (from %s)",  style->tint, tmp);*/
			set_flags( style->user_flags, style_func );
			style->texture_type = type ;
		}else
		{ /* treat second parameter as an image filename : */
			ASImage *im = get_asimage(Scr.image_manager, tmp, 0xFFFFFFFF, 100 ); 
			if( im )
			{
				style->back_icon.image = im ;
			    style->back_icon.width = im->width;
			    style->back_icon.height = im->height;
			    style->back_icon.pix = asimage2pixmap(Scr.asv, Scr.Root, im, NULL, False);
			    style->back_icon.mask = asimage2mask(Scr.asv, Scr.Root, im, NULL, False);
				set_flags( style->user_flags, style_func );
				if( type >= TEXTURE_TRANSPIXMAP )
					set_flags( style->user_flags, F_BACKTRANSPIXMAP );
				style->texture_type = type ;
			}else
				show_error( "failed to load image file \"%s\" in MyStyle \"%s\".", tmp, style->name );	
		}
LOCAL_DEBUG_OUT( "MyStyle \"%s\": BackPixmap %d image = %p, tint = 0x%X", style->name, style->texture_type, style->back_icon.image, style->tint );
	    free (tmp);
	  }
	  break;
#endif
	case F_DRAWTEXTBACKGROUND:
	  style->inherit_flags &= ~style_func;
	  style->user_flags |= style_func;
	  if (style_arg[0] != '\0' && strtol (style_arg, NULL, 10) == 0)
	    style->flags &= ~style_func;
	  else
	    style->flags |= style_func;
	  break;

	case F_DONE:
	  done = 1;
	  break;

	case F_ERROR:
	default:
	  fprintf (stderr, "%s: unknown style command: %s\n", MyName, str);
	  break;
	}
    }
  style->set_flags = style->inherit_flags | style->user_flags;
/*  show_warning( "style \"%s\" set_flags = %X, style_func = %x", style->name, style->set_flags, style_func );
  show_warning( "text = \"%s\"", str );
*/
  free (style_arg);
  return done;
}

void
mystyle_parse_set_style (char *text, FILE * fd, char **style, int *junk2)
{
  char *name = stripcpy2 (text, 0);
  if (name != NULL)
    {
      *(MyStyle **) style = mystyle_find (name);
      if (*style == NULL)
	fprintf (stderr, "%s: unknown style name: '%s'\n", MyName, name);
      free (name);
    }
}

void
set_func_arg (char *text, FILE * fd, char **value, int *junk)
{
  style_arg = stripcpy (text);
  style_func = (unsigned long) value;
}

/*
 * convert an old two-color gradient to a multi-point gradient
 */
int
mystyle_parse_old_gradient (int type, ARGB32 c1, ARGB32 c2, ASGradient *gradient)
{
  int cylindrical = 0;
  switch (type)
    {
    case TEXTURE_GRADIENT:
      type = TEXTURE_GRADIENT_TL2BR;
      break;
    case TEXTURE_HGRADIENT:
      type = TEXTURE_GRADIENT_T2B;
      break;
    case TEXTURE_HCGRADIENT:
      type = TEXTURE_GRADIENT_T2B;
      cylindrical = 1;
      break;
    case TEXTURE_VGRADIENT:
      type = TEXTURE_GRADIENT_L2R;
      break;
    case TEXTURE_VCGRADIENT:
      type = TEXTURE_GRADIENT_L2R;
      cylindrical = 1;
      break;
    default:
      break;
    }
  gradient->npoints = 2 + cylindrical;
  gradient->color = NEW_ARRAY (ARGB32, gradient->npoints);
  gradient->offset = NEW_ARRAY (double, gradient->npoints);
  gradient->color[0] = c1;
  gradient->color[1] = c2;
  if (cylindrical)
    gradient->color[2] = c1;
  gradient->offset[0] = 0.0;
  if (cylindrical)
    gradient->offset[1] = 0.5;
  gradient->offset[gradient->npoints - 1] = 1.0;
  return type;
}

ASImageBevel *
mystyle_make_bevel (MyStyle *style, ASImageBevel *bevel, int hilite, Bool reverse)
{
    if( style && hilite != 0 )
    {
		if( bevel == NULL )
			bevel = safecalloc( 1, sizeof(ASImageBevel));
		else
			memset( bevel, 0x00, sizeof(ASImageBevel));
		if( reverse != 0 )
		{
			bevel->lo_color = style->relief.fore ;
			bevel->lolo_color = GetHilite(style->relief.fore) ;
			bevel->hi_color = style->relief.back ;
			bevel->hihi_color = GetShadow(style->relief.back) ;
		}else
		{
			bevel->hi_color = style->relief.fore ;
			bevel->hihi_color = GetHilite(style->relief.fore) ;
			bevel->lo_color = style->relief.back ;
			bevel->lolo_color = GetShadow(style->relief.back) ;
		}
		bevel->hilo_color = GetAverage(style->relief.fore, style->relief.back) ;
		if( get_flags( hilite, FULL_HILITE ) )
		{
			bevel->left_outline = bevel->top_outline = bevel->right_outline = bevel->bottom_outline = 1;
        	bevel->left_inline = bevel->top_inline =
 			bevel->right_inline = bevel->bottom_inline = get_flags( hilite, EXTRA_HILITE)?3:1 ;
		}else
		{
#ifndef DONT_HILITE_PLAIN
			if( get_flags( hilite, EXTRA_HILITE) )
			{
 				bevel->left_inline = bevel->top_inline =
				bevel->right_inline = bevel->bottom_inline = 2 ;
			}
#endif
		}

		if( get_flags( hilite, LEFT_HILITE) )
		{	bevel->left_outline++ ;		bevel->left_inline++ ; }
		if( get_flags( hilite, TOP_HILITE) )
		{	bevel->top_outline++ ;		bevel->top_inline++  ; }
		if( get_flags( hilite, RIGHT_HILITE) )
		{	bevel->right_outline++ ; 	bevel->right_inline++; }
		if( get_flags( hilite, BOTTOM_HILITE) )
		{	bevel->bottom_outline++ ;	bevel->bottom_inline++;}

	}else if( bevel )
		memset( bevel, 0x00, sizeof(ASImageBevel));

	return bevel;
}

ASImage *
mystyle_draw_text_image( MyStyle *style, const char *text )
{
	ASImage *im = NULL ;
	if( style && text )
	{
		if( style->font.as_font )
		{
			/* todo: implement text rendering using libAfterImage : */	
			im = draw_text( text, style->font.as_font, style->text_style, 100 );
			if( im )
				im->back_color = style->colors.fore ;
		}
	}
	return im ;
}

unsigned int
mystyle_get_font_height( MyStyle *style )
{
	if( style ) 
		if( style->font.as_font )
			return style->font.as_font->max_height ;
	return 1 ;
}
