/*
 * Copyright (c) 2002 Sasha Vasko <sasha@aftercode.net>
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

#define LOCAL_DEBUG

#include "asapp.h"
#include "afterstep.h"
#include "parser.h"
#include "mystyle.h"
#include "screen.h"
#include "../libAfterImage/afterimage.h"

/*
 * if you add a member to this list, or to the MyStyle structure,
 * remember to update mystyle_new(), mystyle_delete(), mystyle_merge_styles(),
 * mystyle_parse(), mystyle_get_property(), and mystyle_set_property()
 */
struct config mystyle_config[] = {
	{"~MyStyle", set_func_arg, (char **)F_DONE},
	{"Inherit", set_func_arg, (char **)F_INHERIT},
	{"Font", set_func_arg, (char **)F_FONT},
	{"ForeColor", set_func_arg, (char **)F_FORECOLOR},
	{"BackColor", set_func_arg, (char **)F_BACKCOLOR},
	{"TextStyle", set_func_arg, (char **)F_TEXTSTYLE},
#ifndef NO_TEXTURE
	{"MaxColors", set_func_arg, (char **)F_MAXCOLORS},
	{"BackGradient", set_func_arg, (char **)F_BACKGRADIENT},
	{"BackMultiGradient", set_func_arg, (char **)F_BACKMULTIGRADIENT},
	{"BackPixmap", set_func_arg, (char **)F_BACKPIXMAP},
	{"BackTransPixmap", set_func_arg, (char **)F_BACKTRANSPIXMAP},
#endif
	{"DrawTextBackground", set_func_arg, (char **)F_DRAWTEXTBACKGROUND},
	{"", NULL, NULL}
};

static int    style_func;
static char  *style_arg;

static char  *DefaultMyStyleName = "default";


static void
mystyle_free_back_icon( MyStyle *style )
{
    if( !get_flags (style->inherit_flags, F_BACKPIXMAP) )
    {
        if( get_flags( style->user_flags, F_EXTERNAL_BACKPIX ) )
            style->back_icon.pix = None ;
        if( get_flags( style->user_flags, F_EXTERNAL_BACKMASK ) )
        {
            style->back_icon.mask = None ;
            style->back_icon.alpha = None ;
        }

        free_icon_resources (style->back_icon);
    }
    memset (&(style->back_icon), 0x00, sizeof (style->back_icon));
}

/* just a nice error printing function */
void
mystyle_error (char *format, char *name, char *text2)
{
    if (text2)
        show_error ("MyStyle [%s] parsing error: %s %s", name, format, text2);
    else
        show_error ("MyStyle [%s] parsing error: %s", name, format);
}

/* Creates tinting GC for drawable d, with color tintColor,
   using color combining function function */
GC
CreateTintGC (Drawable d, unsigned long tintColor, int function)
{
	XGCValues     gcvalue;

	if (tintColor == None || d == None)
		return None;

	gcvalue.function = function;
	gcvalue.foreground = tintColor;
	gcvalue.background = tintColor;
	return XCreateGC (dpy, d, GCFunction | GCForeground | GCBackground, &gcvalue);
}

CARD8
make_component_hilite (int cmp)
{
	if (cmp < 51)
		cmp = 51;
	cmp = (cmp * 12) / 10;
	return (cmp > 255) ? 255 : cmp;
}

/* This routine computes the hilite color from the background color */
inline        ARGB32
GetHilite (ARGB32 background)
{
	return ((make_component_hilite ((background & 0xFF000000) >> 24) << 24) & 0xFF000000) |
		((make_component_hilite ((background & 0x00FF0000) >> 16) << 16) & 0x00FF0000) |
		((make_component_hilite ((background & 0x0000FF00) >> 8) << 8) & 0x0000FF00) |
		((make_component_hilite ((background & 0x000000FF))) & 0x000000FF);
}

/* This routine computes the shadow color from the background color */
inline        ARGB32
GetShadow (ARGB32 background)
{
	return (background >> 1) & 0x7F7F7F7F;
}

inline        ARGB32
GetAverage (ARGB32 foreground, ARGB32 background)
{
	CARD16        a, r, g, b;

	a = ARGB32_ALPHA8 (foreground) + ARGB32_ALPHA8 (background);
	a = (a<<3)/10;
	r = ARGB32_RED8 (foreground) + ARGB32_RED8 (background);
	r = (r<<3)/10;
	g = ARGB32_GREEN8 (foreground) + ARGB32_GREEN8 (background);
	g = (g<<3)/10;
	b = ARGB32_BLUE8 (foreground) + ARGB32_BLUE8 (background);
	b = (b<<3)/10;
	return MAKE_ARGB32 (a, r, g, b);
}

/* create the default style if necessary, and fill in the unset fields
 * of the other styles from the default */
void
mystyle_list_fix_styles (ASHashTable *list)
{
	MyStyle      *dflt;
	MyStyle      *style;
    ASHashIterator iterator ;

    if( list == NULL )
        if( (list = Scr.Look.styles_list) == NULL )
            return;

/*
 * the user may not have set the default style, so set it here
 * all we need are reasonable defaults, and most defaults have been set
 *  by mystyle_new() already
 * we need FONT, FORECOLOR, and BACKCOLOR, at a minimum
 */
    if ((dflt = mystyle_list_find (list, DefaultMyStyleName)) == NULL)
        dflt = mystyle_list_new (list, DefaultMyStyleName);
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
            show_error("couldn't load the default font");
			exit (1);
		}
		dflt->user_flags |= F_FONT;
	}
	dflt->set_flags = dflt->inherit_flags | dflt->user_flags;

/* fix up the styles, using the default style */
    if( start_hash_iteration( list, &iterator ))
        do
        {
			Bool transparent = False ;
            style = (MyStyle*)curr_hash_data(&iterator);
            if( style != dflt )
                mystyle_merge_styles (dflt, style, False, False);
			if(  style->texture_type == TEXTURE_SHAPED_SCALED_PIXMAP ||
				 style->texture_type == TEXTURE_SHAPED_PIXMAP ||
		 		style->texture_type > TEXTURE_PIXMAP )
			{
				transparent = True;
			}else if (  style->texture_type >= TEXTURE_GRADIENT_START &&
						style->texture_type <= TEXTURE_GRADIENT_END	)
			{
				/* need to check if any of the gradient colors has alpha channel < 0xff */
				int i = style->gradient.npoints ;
				while( --i >= 0 )
					if( ARGB32_ALPHA8(style->gradient.color[i]) < 0x00F0 )
					{
						transparent = True ;
						break;
					}
			}
			if( transparent )
			{
				set_flags( style->user_flags, F_TRANSPARENT );
				set_flags( style->set_flags, F_TRANSPARENT );
			}
        }while(next_hash_item( &iterator ));
}

void
mystyle_fix_styles (void)
{
    mystyle_list_fix_styles (NULL);
}

int
mystyle_translate_grad_type (int type)
{
	switch (type)
	{
	 case TEXTURE_GRADIENT:
		 return GRADIENT_TopLeft2BottomRight;

	 case TEXTURE_HGRADIENT:
	 case TEXTURE_HCGRADIENT:
		 return GRADIENT_Left2Right;

	 case TEXTURE_VGRADIENT:
	 case TEXTURE_VCGRADIENT:
		 return GRADIENT_Top2Bottom;
	 case TEXTURE_GRADIENT_TL2BR:
		 return GRADIENT_TopLeft2BottomRight;
	 case TEXTURE_GRADIENT_BL2TR:
		 return GRADIENT_BottomLeft2TopRight;
	 case TEXTURE_GRADIENT_T2B:
		 return GRADIENT_Top2Bottom;
	 case TEXTURE_GRADIENT_L2R:
		 return GRADIENT_Left2Right;
	 default:
		 return -1;
	}
}

/****************************************************************************
 * grab a section of the screen and darken it
 ***************************************************************************/
static ASImage *
grab_root_asimage( ScreenInfo *scr )
{
	XSetWindowAttributes attr ;
    XEvent event ;
    int tick_count = 0 ;
    Bool grabbed = False ;
	Window src;
	ASImage *root_im = NULL ;
	int x = scr->RootClipArea.x, y = scr->RootClipArea.y ;
	int width = scr->RootClipArea.width, height = scr->RootClipArea.height ;

	if( scr == NULL )
		scr = &Scr ;
	/* this only works if we use DefaultVisual - same visual as the Root window :*/
	if( scr->asv->visual_info.visual != DefaultVisual( dpy, DefaultScreen(dpy) ) )
		return NULL ;

	if( x < 0 )
	{
		width += x ;
		x = 0 ;
	}
	if( y < 0 )
	{
		height += y ;
		y = 0 ;
	}
	if( x + width > Scr.MyDisplayWidth )
		width = (int)(Scr.MyDisplayWidth) - x ;
	if( y + height > Scr.MyDisplayHeight )
		height = (int)(Scr.MyDisplayHeight) - y ;

	if( height < 0 || width < 0 )
		return NULL ;

	attr.background_pixmap = ParentRelative ;
	attr.backing_store = Always ;
	attr.event_mask = ExposureMask ;
	attr.override_redirect = True ;

	LOCAL_DEBUG_OUT( "grabbing root image from %dx%d%+d%+d", width, height, x, y );

    src = create_visual_window( scr->asv, scr->Root, x, y, width, height, 0, CopyFromParent,
				  				CWBackPixmap|CWBackingStore|CWOverrideRedirect|CWEventMask,
				  				&attr);

	if( src == None ) return NULL ;
	XGrabServer( dpy );
	grabbed = True ;
	XMapRaised( dpy, src );
	XSync(dpy, False );
	start_ticker(1);
	/* now we have to wait for our window to become mapped - waiting for Expose */
	for( tick_count = 0 ; !XCheckWindowEvent( dpy, src, ExposureMask, &event ) && tick_count < 100 ; tick_count++)
  		wait_tick();
	if( tick_count < 100 )
        if( (root_im = pixmap2ximage( scr->asv, src, 0, 0, width, height, AllPlanes, 0 )) != NULL )
		{
			if( scr->RootClipArea.y < 0 || scr->RootClipArea.y < 0 )
			{
				ASImage *tmp ;

				x = ( scr->RootClipArea.x < 0 )? -scr->RootClipArea.x : 0 ;
				y = ( scr->RootClipArea.y < 0 )? -scr->RootClipArea.y : 0 ;

				tmp = tile_asimage (scr->asv, root_im,
                                    x, y, scr->RootClipArea.width, scr->RootClipArea.height, TINT_NONE,
                                   ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT);
				if( tmp )
				{
                	destroy_asimage( &root_im );
					root_im = tmp ;
				}
			}
		}
	XDestroyWindow( dpy, src );
	XUngrabServer( dpy );
	return root_im ;
}



static merge_scanlines_func mystyle_merge_scanlines_func_xref[] = {
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

merge_scanlines_func
mystyle_translate_texture_type( int texture_type )
{
    register int index = 1 ;                   /* default is alphablending */
    if (texture_type >= TEXTURE_SCALED_TRANSPIXMAP &&
        texture_type < TEXTURE_SCALED_TRANSPIXMAP_END)
        index = texture_type - TEXTURE_SCALED_TRANSPIXMAP;
    else if (texture_type >= TEXTURE_TRANSPIXMAP && texture_type < TEXTURE_TRANSPIXMAP_END)
        index = texture_type - TEXTURE_TRANSPIXMAP;
    return  mystyle_merge_scanlines_func_xref[index];
}

static inline ASImage *
mystyle_flip_image( ASImage *im, int width, int height, int flip )
{
	ASImage *tmp ;
	if(flip == 0)
		return im;
	tmp = flip_asimage( Scr.asv, im, 0, 0, width, height, flip, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
	if( tmp != NULL )
	{
		safe_asimage_destroy( im );
		im = tmp;
	}
	return im;
}

ASImage      *
mystyle_make_image (MyStyle * style, int root_x, int root_y, int width, int height, int flip )
{
	ASImage      *im = NULL;
	Pixmap        root_pixmap;
	unsigned int  root_w, root_h;
	Bool transparency = False ;
	int preflip_width, preflip_height ;

#ifndef NO_TEXTURE
	if (width < 1)
		width = 1;
	if (height < 1)
		height = 1;
    LOCAL_DEBUG_OUT ("style \"%s\", texture_type = %d, im = %p, tint = 0x%lX, geom=(%dx%d%+d%+d), flip = %d", style->name, style->texture_type,
                     style->back_icon.image, style->tint, width, height, root_x, root_y, flip);

	transparency = TransparentMS(style);

	if(  transparency )
	{	/* in this case we need valid root image : */
		 if (Scr.RootImage == NULL)
		 {
			 root_pixmap = ValidatePixmap (None, 1, 1, &root_w, &root_h);
			 LOCAL_DEBUG_OUT ("obtained Root pixmap = %lX", root_pixmap);
			 if (root_pixmap)
			 {
                ASImage *tmp_root ;
#if 0
                Scr.RootImage = pixmap2ximage (Scr.asv, root_pixmap, 0, 0, root_w, root_h, AllPlanes, 100);
#else
                tmp_root = pixmap2asimage (Scr.asv, root_pixmap, 0, 0, root_w, root_h, AllPlanes, False, 100);
#endif
                if( tmp_root )
                {
                    if( Scr.RootClipArea.x == 0 && Scr.RootClipArea.y == 0 &&
                        Scr.RootClipArea.width == Scr.MyDisplayWidth &&
                        Scr.RootClipArea.height == Scr.MyDisplayHeight )
                    {
                        Scr.RootImage = tmp_root ;
                    }else
                    {
                        Scr.RootImage = tile_asimage (Scr.asv, Scr.RootImage,
                                                    Scr.RootClipArea.x, Scr.RootClipArea.y,
                                                    Scr.RootClipArea.width, Scr.RootClipArea.height, TINT_NONE,
                                                    ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT);
                        destroy_asimage( &tmp_root );
                    }
                }
             }
             if( Scr.RootImage == NULL )
             {
                if( (Scr.RootImage = grab_root_asimage( &Scr )) != NULL )
                {
                    root_w = Scr.RootImage->width;
                    root_h = Scr.RootImage->height;
                }
             }
		 } else
		 {
			 root_w = Scr.RootImage->width;
			 root_h = Scr.RootImage->height;
		 }
         LOCAL_DEBUG_OUT ("RootImage = %p clip(%ux%u%+d%+d) size(%dx%d)", Scr.RootImage, Scr.RootClipArea.width, Scr.RootClipArea.height, Scr.RootClipArea.x, Scr.RootClipArea.y, root_w, root_h);
	}
	if( get_flags( flip, FLIP_VERTICAL )  )
	{
		preflip_width = height ;
		preflip_height = width ;
	}else
	{
		preflip_width = width ;
		preflip_height = height ;
	}

	switch (style->texture_type)
	{
	 case TEXTURE_SOLID:
		 im = create_asimage (width, height, 0);
		 im->back_color = style->colors.back;
		 break;

	 case TEXTURE_GRADIENT:

	 case TEXTURE_HGRADIENT:
	 case TEXTURE_HCGRADIENT:

	 case TEXTURE_VGRADIENT:
	 case TEXTURE_VCGRADIENT:
	 case TEXTURE_GRADIENT_TL2BR:
	 case TEXTURE_GRADIENT_BL2TR:
	 case TEXTURE_GRADIENT_T2B:
	 case TEXTURE_GRADIENT_L2R:
	 	{
			ASGradient *grad = flip_gradient( &(style->gradient), flip );
 			LOCAL_DEBUG_OUT( "orig grad type = %d, translated grad_type = %d, texture_type = %d", style->gradient.type, grad->type, style->texture_type );
		 	im = make_gradient (Scr.asv, grad, width, height, 0xFFFFFFFF, ASA_ASImage, 0,
							 ASIMAGE_QUALITY_DEFAULT);
			if( grad != &(style->gradient) )
				destroy_asgradient( &grad );
		}
		break;
	 case TEXTURE_SHAPED_SCALED_PIXMAP:
	 case TEXTURE_SCALED_PIXMAP:
		 im = scale_asimage (Scr.asv, style->back_icon.image, preflip_width, preflip_height, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
		 im = mystyle_flip_image( im, width, height, flip );
		 break;
	 case TEXTURE_SHAPED_PIXMAP:
	 case TEXTURE_PIXMAP:
		 im = tile_asimage (Scr.asv, style->back_icon.image,
							0, 0, preflip_width, preflip_height, TINT_LEAVE_SAME, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
		 im = mystyle_flip_image( im, width, height, flip );
		 break;
	}

	if( transparency )
	{
		if (Scr.RootImage == NULL)
		{  /* simply creating solid color image */
			if( im == NULL )
			{
				im = create_asimage( width, height, 100 );
				im->back_color = style->colors.back ;
			}
		}else
		{
			if (style->texture_type == TEXTURE_TRANSPARENT || style->texture_type == TEXTURE_TRANSPARENT_TWOWAY)
			{
                im = tile_asimage (Scr.asv, Scr.RootImage, root_x-Scr.RootClipArea.x, root_y-Scr.RootClipArea.y,
									width, height, style->tint, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
			} else
			{
				 ASImageLayer  layers[2];
				 ASImage      *scaled_im = NULL;
				 int           index = 0;

				 init_image_layers (&layers[0], 2);

				 layers[0].im = Scr.RootImage;
				 if (style->texture_type == TEXTURE_SHAPED_SCALED_PIXMAP &&
					 style->texture_type == TEXTURE_SHAPED_PIXMAP)
				 {
					 index = 1 ;               /* alphablending  */
				 }else if (style->texture_type >= TEXTURE_SCALED_TRANSPIXMAP &&
					 style->texture_type < TEXTURE_SCALED_TRANSPIXMAP_END)
					 index = style->texture_type - TEXTURE_SCALED_TRANSPIXMAP;
				 else if (style->texture_type >= TEXTURE_TRANSPIXMAP && style->texture_type < TEXTURE_TRANSPIXMAP_END)
					 index = style->texture_type - TEXTURE_TRANSPIXMAP;

				 LOCAL_DEBUG_OUT ("index = %d", index);
				 layers[0].merge_scanlines = mystyle_merge_scanlines_func_xref[index];
                 layers[0].dst_x = 0;
                 layers[0].dst_y = 0;
                 layers[0].clip_x = root_x-Scr.RootClipArea.x;
                 layers[0].clip_y = root_y-Scr.RootClipArea.y;
				 layers[0].clip_width = width;
				 layers[0].clip_height = height;

				 layers[1].im = im?im:style->back_icon.image;
				 if (style->texture_type >= TEXTURE_SCALED_TRANSPIXMAP &&
					 style->texture_type < TEXTURE_SCALED_TRANSPIXMAP_END)
				 {
					 scaled_im = scale_asimage (Scr.asv, layers[1].im, preflip_width, preflip_height,
												ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
					 /* here layers[1].im is always style->back_icon.image, so we should not destroy it ! */
					 if (scaled_im)
						 layers[1].im = mystyle_flip_image( scaled_im, width, height, flip );
						 /* scaled_im got destroyed in mystyle_flip_image if it had to be */
				 }
				 if( flip != 0 && layers[1].im == style->back_icon.image )
				 {
					/* safely assuming that im is NULL at this point,( see logic above ) */
					 layers[1].im = im = flip_asimage( Scr.asv, layers[1].im, 0, 0, width, height, flip, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
				 }
				 layers[1].merge_scanlines = layers[0].merge_scanlines;
				 layers[1].dst_x = 0;
				 layers[1].dst_y = 0;
				 layers[1].clip_x = 0;
				 layers[1].clip_y = 0;
				 layers[1].clip_width = width;
				 layers[1].clip_height = height;

				{
					ASImage *tmp = merge_layers (Scr.asv, &layers[0], 2, width, height, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
					if( tmp )
					{
						if( im )
						{
							if (style->texture_type == TEXTURE_SHAPED_SCALED_PIXMAP &&
							    style->texture_type == TEXTURE_SHAPED_PIXMAP)
							{
								/*we need to keep alpha channel intact */
								copy_asimage_channel(tmp, IC_ALPHA, im, IC_ALPHA);
							}
							safe_asimage_destroy (im);
						}
						im = tmp ;
					}
				}
				if (scaled_im)
					destroy_asimage (&scaled_im);
			 }
		}
	}
#endif /* NO_TEXTURE */
	return im;
}

icon_t
mystyle_make_icon (MyStyle * style, int width, int height, Pixmap cache)
{
	icon_t        icon = { None, None, None, 0, 0 };

#ifndef NO_TEXTURE
	asimage2icon (mystyle_make_image (style, 0, 0, width, height, 0),
				  &icon, (style->texture_type < TEXTURE_TEXTURED_START));
#endif /* NO_TEXTURE */
	return icon;
}

icon_t
mystyle_make_icon_overlay (MyStyle * style, int root_x, int root_y, int width, int height, Pixmap cache)
{
	icon_t        icon = { None, None, 0, 0 };

#ifndef NO_TEXTURE
	asimage2icon (mystyle_make_image (style, root_x, root_y, width, height, 0),
				  &icon, (style->texture_type < TEXTURE_TEXTURED_START));
#endif /* NO_TEXTURE */
	return icon;
}

Pixmap
mystyle_make_pixmap (MyStyle * style, int width, int height, Pixmap cache)
{
	Pixmap        p;
	icon_t        icon = mystyle_make_icon (style, width, height, cache);

	p = icon.pix;
	icon.pix = None;
	free_icon_resources (icon);
	return p;
}

Pixmap
mystyle_make_pixmap_overlay (MyStyle * style, int root_x, int root_y, int width, int height, Pixmap cache)
{
	Pixmap        p;
	icon_t        icon = mystyle_make_icon_overlay (style, root_x, root_y, width, height, cache);

	p = icon.pix;
	icon.pix = None;
	free_icon_resources (icon);
	return p;
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

/*************************************************************************/
/* Mystyle creation/deletion                                             */
/*************************************************************************/
static void
mystyle_free_resources( MyStyle *style )
{
    if( style->magic == MAGIC_MYSTYLE )
    {
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
		if( !get_flags (style->inherit_flags, F_BACKTRANSPIXMAP) )
        {
            LOCAL_DEBUG_OUT( "calling mystyle_free_back_icon for style %p", style );
            mystyle_free_back_icon(style);
        }
#endif
    }
}

void
mystyle_init (MyStyle  *style)
{
    style->user_flags = 0;
	style->inherit_flags = 0;
	style->set_flags = 0;
	style->flags = 0;
	style->name = NULL;
	style->text_style = 0;
	style->font.name = NULL;
	style->font.as_font = NULL;
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
	style->back_icon.alpha = None;
    style->tint = TINT_LEAVE_SAME;
	style->back_icon.image = NULL;
#endif
}


void
mystyle_destroy (ASHashableValue value, void *data)
{
    if ((char*)value != NULL)
	{
/*	fprintf( stderr, "destroying mystyle [%s]\n", value.string_val ); */
        free ((char*)value);               /* destroying our name */
	}
    if ( data != NULL)
    {
        MyStyle *style = (MyStyle*) data ;
        mystyle_free_resources( style );
        style->magic = 0 ;                 /* invalidating memory block */
        free (data);
    }
}

ASHashTable *
mystyle_list_init()
{
    ASHashTable *list = NULL;

    list = create_ashash( 0, casestring_hash_value, casestring_compare, mystyle_destroy );

    return list;
}

MyStyle *
mystyle_list_new (struct ASHashTable *list, char *name)
{
    MyStyle      *style ;

    if( name == NULL ) return NULL ;

    if( list == NULL )
    {
        if( Scr.Look.styles_list == NULL )
            if( (Scr.Look.styles_list = mystyle_list_init())== NULL ) return NULL;
        list = Scr.Look.styles_list ;
    }

    if( get_hash_item( list, AS_HASHABLE(name), (void**)&style ) == ASH_Success )
        if( style )
        {
            if( style->magic == MAGIC_MYSTYLE )
                return style;
            else
                remove_hash_item( list, (ASHashableValue)name, NULL, True );
        }

    style = (MyStyle *) safecalloc (1, sizeof (MyStyle));

    mystyle_init(style);
    style->name = mystrdup( name );

    if( add_hash_item( list, AS_HASHABLE(style->name), style ) != ASH_Success )
    {   /* something terrible has happen */
        if( style->name )
            free( style->name );
        free( style );
        return NULL;
    }
    style->magic = MAGIC_MYSTYLE ;
    return style;
}

MyStyle      *
mystyle_new_with_name (char *name)
{
    if (name == NULL)
        return NULL ;
    return mystyle_list_new( NULL, name );
}


/* destruction of all mystyle records : */
void
mystyle_list_destroy_all( ASHashTable **plist )
{
    if( plist == NULL )
        plist = &Scr.Look.styles_list ;
    destroy_ashash( plist );
}

void
mystyle_destroy_all()
{
    mystyle_list_destroy_all( NULL );
}


/*
 * MyStyle Lookup functions :
 */
MyStyle *
mystyle_list_find (struct ASHashTable *list, const char *name)
{
    MyStyle *style = NULL;
    if( list == NULL )
        list = Scr.Look.styles_list ;

    if( list && name )
        if( get_hash_item( list, AS_HASHABLE((char*)name), (void**)&style ) != ASH_Success )
            style = NULL ;
    return style;
}

MyStyle *
mystyle_list_find_or_default (struct ASHashTable *list, const char *name)
{
    MyStyle *style = NULL;

    if( list == NULL )
        list = Scr.Look.styles_list ;

    if( name == NULL )
        name = DefaultMyStyleName ;
    if( list && name )
        if( get_hash_item( list, AS_HASHABLE((char*)name), (void**)&style ) != ASH_Success )
            if( get_hash_item( list, AS_HASHABLE(DefaultMyStyleName), (void**)&style ) != ASH_Success )
                style = NULL ;
    return style;
}

/* find a style by name */
MyStyle      *
mystyle_find (const char *name)
{
    return mystyle_list_find(NULL, name);
}

/* find a style by name or return the default style */
MyStyle      *
mystyle_find_or_default (const char *name)
{
    return mystyle_list_find_or_default( NULL, name);
}

/*
 * merges two styles, with the result in child
 * if override == False, will not override fields already set
 * if copy == True, copies instead of inheriting; this is important, because
 *   inherited members are deleted when their real parent is deleted; don't
 *   inherit if the parent style could go away before the child
 */
void
mystyle_merge_font( MyStyle *style, MyFont *font, Bool override, Bool copy)
{
    if ((override == True) && (style->user_flags & F_FONT))
        unload_font (&style->font);
    if ((override == True) || !(style->set_flags & F_FONT))
    {
        if (copy == False)
        {
#ifdef I18N
            load_font (font->name, &style->font);
#else
            style->font = *font;
#endif
            style->user_flags &= ~F_FONT;
            style->inherit_flags |= F_FONT;
        } else
        {
            if (load_font (font->name, &style->font) == True)
            {
                style->user_flags |= F_FONT;
                style->inherit_flags &= ~F_FONT;
            }
        }
    }
}

void
mystyle_merge_styles (MyStyle * parent, MyStyle * child, Bool override, Bool copy)
{
	if (parent->set_flags & F_FONT)
        mystyle_merge_font( child, &(parent->font), override, copy);

    if (parent->set_flags & F_TEXTSTYLE)
	{
		if ((override == True) || !(child->set_flags & F_TEXTSTYLE))
		{
			child->text_style = parent->text_style;

			if (copy == False)
			{
				child->user_flags &= ~F_TEXTSTYLE;
				child->inherit_flags |= F_TEXTSTYLE;
			} else
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
			} else
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
			} else
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
			} else
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
			} else
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
            LOCAL_DEBUG_OUT( "calling mystyle_free_back_icon for style %p", child );
            mystyle_free_back_icon(child);
		}
		if ((override == True) || !(child->set_flags & F_BACKPIXMAP))
		{
			if (override == True)
				child->texture_type = parent->texture_type;
			if ((parent->texture_type == TEXTURE_TRANSPARENT ||
				 parent->texture_type == TEXTURE_TRANSPARENT_TWOWAY) &&
				(override == True ||
				 (child->texture_type != TEXTURE_TRANSPARENT && child->texture_type != TEXTURE_TRANSPARENT_TWOWAY)))
			{
				child->tint = parent->tint;
			}
			if (!copy)
			{
				child->back_icon = parent->back_icon;
/*				if (parent->back_icon.image)
					child->back_icon.image = dup_asimage (parent->back_icon.image);
*/				clear_flags (child->user_flags, F_BACKPIXMAP | F_BACKTRANSPIXMAP);
				set_flags (child->inherit_flags, F_BACKPIXMAP);
				if (get_flags (parent->set_flags, F_BACKTRANSPIXMAP))
					set_flags (child->inherit_flags, F_BACKTRANSPIXMAP);
			} else
			{
				GC            gc = create_visual_gc (Scr.asv, Scr.Root, 0, NULL);

				child->back_icon.pix =
					XCreatePixmap (dpy, Scr.Root, parent->back_icon.width, parent->back_icon.height,
								   Scr.asv->visual_info.depth);
				XCopyArea (dpy, parent->back_icon.pix, child->back_icon.pix, gc, 0, 0, parent->back_icon.width,
						   parent->back_icon.height, 0, 0);
				if (parent->back_icon.mask != None)
				{
					GC            mgc = XCreateGC (dpy, parent->back_icon.mask, 0, NULL);

					child->back_icon.mask =
						XCreatePixmap (dpy, Scr.Root, parent->back_icon.width, parent->back_icon.height, 1);
					XCopyArea (dpy, parent->back_icon.mask, child->back_icon.mask, mgc, 0, 0, parent->back_icon.width,
							   parent->back_icon.height, 0, 0);
					XFreeGC (dpy, mgc);
				}
				if (parent->back_icon.alpha != None)
				{
					GC            mgc = XCreateGC (dpy, parent->back_icon.alpha, 0, NULL);

					child->back_icon.alpha =
						XCreatePixmap (dpy, Scr.Root, parent->back_icon.width, parent->back_icon.height, 8);
					XCopyArea (dpy, parent->back_icon.alpha, child->back_icon.alpha, mgc, 0, 0, parent->back_icon.width,
							   parent->back_icon.height, 0, 0);
					XFreeGC (dpy, mgc);
				}
				if (parent->back_icon.image)
					child->back_icon.image = dup_asimage (parent->back_icon.image);
				else
					child->back_icon.image = 0;
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
			} else
			{
				child->user_flags |= F_DRAWTEXTBACKGROUND;
				child->inherit_flags &= ~F_DRAWTEXTBACKGROUND;
			}
		}
	}
	child->set_flags = child->user_flags | child->inherit_flags;
}

void
mystyle_parse (char *tline, FILE * fd, char **pjunk, int *junk2)
{
	MyStyle      *style;
	char         *newline;
	char         *name = stripcpy2 (tline, 0);

    if (name == NULL)
	{
        show_error("bad style name '%s'", tline);
		return;
	}

/* if this style was already defined, find it */
    if ((style = mystyle_find (name)) == NULL)
        style = mystyle_new_with_name (name);
    free( name );

    newline = safemalloc (MAXLINELENGTH + 1);
    while (fgets (newline, MAXLINELENGTH, fd))
	{
        char         *p = stripcomments (newline);
		if (*p != '\0')
			if (mystyle_parse_member (style, p) != False)
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
mystyle_parse_member (MyStyle * style, char *str)
{
	int           done = 0;
	struct config *config = find_config (mystyle_config, str);

	style_func = F_ERROR;
	style_arg = NULL;

	if (config != NULL)
		config->action (str + strlen (config->keyword), NULL, config->arg, config->arg2);
	else
        mystyle_error(style->name, "unknown style command: %s", str);
	if (style_arg == NULL)
	{
        mystyle_error(style->name, "bad style argument: %s", str);
		return 0;
	} else
	{
		style->inherit_flags &= ~style_func;
		switch (style_func)
		{
		 case F_INHERIT:
			 {
				 MyStyle      *parent = mystyle_find (style_arg);

				 if (parent != NULL)
					 mystyle_merge_styles (parent, style, True, False);
				 else
                     mystyle_error(style->name, "unknown style to be inherited: %s", style_arg);
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
			 if (parse_argb_color (style_arg, &(style->colors.fore)) != style_arg)
				 style->user_flags |= style_func;
			 else
                 mystyle_error(style->name, "unable to parse Forecolor \"%s\"", style_arg);
			 break;
		 case F_BACKCOLOR:
			 style->texture_type = 0;
			 if (parse_argb_color (style_arg, &(style->colors.back)) != style_arg)
			 {
				 style->relief.fore = GetHilite (style->colors.back);
				 style->relief.back = GetShadow (style->colors.back);
				 style->user_flags |= style_func;
			 } else
                 mystyle_error(style->name, "unable to parse Backcolor \"%s\"", style_arg);
			 break;
#ifndef NO_TEXTURE
		 case F_MAXCOLORS:
			 style->max_colors = strtol (style_arg, NULL, 10);
			 style->user_flags |= style_func;
			 break;
		 case F_BACKGRADIENT:
			 {
				 char         *ptr, *ptr1;
				 int           type = strtol (style_arg, &ptr, 10);
				 ARGB32        color1 = 0, color2 = 0;
				 register int  i = 0;

				 while (!isspace (ptr[i]) && ptr[i] != '\0')
					 ++i;
				 while (isspace (ptr[i]))
					 ++i;
				 if (ptr[i] == '\0')
				 {
                     mystyle_error(style->name, "missing gradient colors: %s", style_arg);
				 } else
				 {
					 ptr = &ptr[i];
					 ptr1 = (char *)parse_argb_color (ptr, &color1);
/*show_warning("in MyStyle \"%s\":c1 = #%X(%s)", style->name, color1, ptr );			*/
					 if (ptr1 != ptr)
					 {
						 int           k = 0;

						 while (isspace (ptr1[k]))
							 ++k;
						 ptr = &ptr1[k];
						 if (parse_argb_color (ptr, &color2) != ptr)
						 {
							 ASGradient    gradient;

/*show_warning("in MyStyle \"%s\":c1 = #%X, c2 = #%X", style->name, color1, color2 );*/
							 if ((type = mystyle_parse_old_gradient (type, color1, color2, &gradient)) >= 0)
							 {
								 if (style->user_flags & F_BACKGRADIENT)
								 {
									 free (style->gradient.color);
									 free (style->gradient.offset);
								 }
								 style->gradient = gradient;
								 style->gradient.type = mystyle_translate_grad_type (type);
								 style->texture_type = type;
								 style->user_flags |= style_func;
								 LOCAL_DEBUG_OUT( "style %p type = %d", style, style->gradient.type );
							 } else
                                 show_error("Error in MyStyle \"%s\": invalid gradient type %d", style->name, type);
						 } else
                             mystyle_error(style->name, "in MyStyle \"%s\":can't parse second color \"%s\"", ptr);
					 } else
                         mystyle_error(style->name, "in MyStyle \"%s\":can't parse first color \"%s\"", ptr);
					 if (!(style->user_flags & style_func))
                         mystyle_error(style->name, "bad gradient: \"%s\"", style_arg);
				 }
			 }
			 break;
		 case F_BACKMULTIGRADIENT:
			 {
				 char         *ptr, *ptr1;
				 int           type = strtol (style_arg, &ptr, 10);
				 ASGradient    gradient;
				 int           error = 0;

				 gradient.npoints = 0;
				 gradient.color = NULL;
				 gradient.offset = NULL;
				 if (type < TEXTURE_GRADIENT_TL2BR || type >= TEXTURE_PIXMAP)
					 error = 4;
				 for (; ptr && isspace (*ptr); ptr++);
				 while (!error && ptr != NULL && *ptr != '\0')
				 {
					 ARGB32        color;

					 ptr1 = (char *)parse_argb_color (ptr, &color);
					 if (ptr1 == ptr)
						 error = 1;
					 else
					 {
						 double        offset = 0.0;

						 ptr = ptr1;
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
					 style->gradient = gradient;
					 style->gradient.type = mystyle_translate_grad_type (type);
					 style->texture_type = type;
					 style->user_flags |= F_BACKGRADIENT;
					 style_func = F_BACKGRADIENT;
					 LOCAL_DEBUG_OUT( "style %p, type = %d, npoints = %d", style, style->gradient.type, style->gradient.npoints );
				 } else
				 {
					 if (gradient.color != NULL)
						 free (gradient.color);
					 if (gradient.offset != NULL)
						 free (gradient.offset);
                     show_error("Error in MyStyle \"%s\": bad gradient (error %d): \"%s\"; at [%s]", style->name, error, style_arg, ptr?ptr:"");
				 }
			 }
			 break;
		 case F_BACKPIXMAP:
			 {
				 char         *ptr;
				 int           type = strtol (style_arg, &ptr, 10);
				 char         *tmp = stripcpy (ptr);

				 clear_flags (style->inherit_flags, F_BACKTRANSPIXMAP | F_BACKPIXMAP);
                 LOCAL_DEBUG_OUT( "calling mystyle_free_back_icon for style %p", style );
                 mystyle_free_back_icon(style);

				 if (type < TEXTURE_TEXTURED_START || type >= TEXTURE_TEXTURED_END)
				 {
                     show_error("Error in MyStyle \"%s\": unsupported texture type [%d] in BackPixmap setting. Assuming default of [128] instead.", style->name, type);
					 type = TEXTURE_PIXMAP;
				 }
				 if (type == TEXTURE_TRANSPARENT || type == TEXTURE_TRANSPARENT_TWOWAY)
				 {							   /* treat second parameter as ARGB tint value : */
					 if (parse_argb_color (tmp, &(style->tint)) == tmp)
						 style->tint = TINT_LEAVE_SAME;	/* use no tinting by default */
					 else if (type == TEXTURE_TRANSPARENT)
						 style->tint = (style->tint >> 1) & 0x7F7F7F7F;	/* converting old style tint */
/*LOCAL_DEBUG_OUT( "tint is 0x%X (from %s)",  style->tint, tmp);*/
					 set_flags (style->user_flags, style_func);
					 style->texture_type = type;
				 } else
                 {  /* treat second parameter as an image filename : */
                     if ( load_icon(&(style->back_icon), tmp, Scr.image_manager ))
					 {
						 set_flags (style->user_flags, style_func);
						 if (type >= TEXTURE_TRANSPIXMAP)
							 set_flags (style->user_flags, F_BACKTRANSPIXMAP);
						 style->texture_type = type;
					 } else
                         mystyle_error(style->name, "failed to load image file \"%s\".", tmp);
				 }
				 LOCAL_DEBUG_OUT ("MyStyle \"%s\": BackPixmap %d image = %p, tint = 0x%lX", style->name,
								  style->texture_type, style->back_icon.image, style->tint);
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
             mystyle_error(style->name, "unknown style command: [%s]", str);
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
	char         *name = stripcpy2 (text, 0);

	if (name != NULL)
	{
		*(MyStyle **) style = mystyle_find (name);
		if (*style == NULL)
            show_error("unknown style name: \"%s\"", name);
		free (name);
	}
}

void
set_func_arg (char *text, FILE * fd, char **value, int *junk)
{
	style_arg = stripcpy (text);
	style_func = (unsigned long)value;
}

/*
 * convert an old two-color gradient to a multi-point gradient
 */
int
mystyle_parse_old_gradient (int type, ARGB32 c1, ARGB32 c2, ASGradient * gradient)
{
	int           cylindrical = 0;

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
	gradient->type = type ;
	return type;
}

void
mystyle_merge_colors (MyStyle * style, int type, char *fore, char *back,
					   char *gradient, char *pixmap)
{
	if ((fore != NULL) && !((*style).user_flags & F_FORECOLOR))
	{
		if (parse_argb_color (fore, &((*style).colors.fore)) != fore)
			(*style).user_flags |= F_FORECOLOR;
	}
	if ((back != NULL) && !((*style).user_flags & F_BACKCOLOR))
	{
		if (parse_argb_color (back, &((*style).colors.back)) != back)
		{
			(*style).relief.fore = GetHilite ((*style).colors.back);
			(*style).relief.back = GetShadow ((*style).colors.back);
			(*style).user_flags |= F_BACKCOLOR;
		}
	}
#ifndef NO_TEXTURE
    if (type >= 0)
	{
		switch (type)
		{
		 case TEXTURE_GRADIENT:
			 style->texture_type = TEXTURE_GRADIENT_TL2BR;
			 break;
		 case TEXTURE_HGRADIENT:
			 style->texture_type = TEXTURE_GRADIENT_L2R;
			 break;
		 case TEXTURE_HCGRADIENT:
			 style->texture_type = TEXTURE_GRADIENT_L2R;
			 break;
		 case TEXTURE_VGRADIENT:
			 style->texture_type = TEXTURE_GRADIENT_T2B;
			 break;
		 case TEXTURE_VCGRADIENT:
			 style->texture_type = TEXTURE_GRADIENT_T2B;
			 break;
		 default:
			 style->texture_type = type;
			 break;
		}
	}
	if ((type > 0) && (type < TEXTURE_PIXMAP) && !((*style).user_flags & F_BACKGRADIENT))
	{
		if (gradient != NULL)
		{
			ARGB32        c1, c2 = 0;
			ASGradient    grad;
			char         *ptr;

			ptr = (char *)parse_argb_color (gradient, &c1);
			parse_argb_color (ptr, &c2);
			if (ptr != gradient && (type = mystyle_parse_old_gradient (type, c1, c2, &grad)) >= 0)
			{
				if (style->user_flags & F_BACKGRADIENT)
				{
					free (style->gradient.color);
					free (style->gradient.offset);
				}
				style->gradient = grad;
				grad.type = mystyle_translate_grad_type (type);
				style->texture_type = type;
				style->user_flags |= F_BACKGRADIENT;
			} else
                show_error ("bad gradient definition in look file: %s", gradient);
		}
    } else if ((type == TEXTURE_PIXMAP) && !get_flags(style->user_flags, F_BACKPIXMAP))
	{
		if (pixmap != NULL)
		{
/* treat second parameter as an image filename : */
            if ( load_icon(&(style->back_icon), pixmap, Scr.image_manager ))
            {
                set_flags (style->user_flags, style_func);
                style->texture_type = type;
                set_flags(style->user_flags, F_BACKPIXMAP);
            } else
                show_error ("failed to load image file \"%s\" in MyStyle \"%s\".", pixmap, style->name);
        }
	}
#endif
	(*style).set_flags = (*style).user_flags | (*style).inherit_flags;
}

void
mystyle_inherit_font (MyStyle * style, MyFont * font)
{
	/* NOTE: these should have inherit_flags set, so the font is only
	 *       unloaded once */
	if (style != NULL && !(style->set_flags & F_FONT))
	{
		style->font = *font;
		style->inherit_flags |= F_FONT;
		style->user_flags &= ~F_FONT;		   /* to prevent confusion */
		style->set_flags = style->user_flags | style->inherit_flags;
	}
}


ASImageBevel *
mystyle_make_bevel (MyStyle * style, ASImageBevel * bevel, int hilite, Bool reverse)
{
	if (style && hilite != 0)
	{
        int extra_hilite = get_flags (hilite, EXTRA_HILITE)?2:0;
		if (bevel == NULL)
			bevel = safecalloc (1, sizeof (ASImageBevel));
		else
			memset (bevel, 0x00, sizeof (ASImageBevel));
		if (reverse != 0)
		{
			bevel->lo_color = style->relief.fore;
			bevel->lolo_color = GetHilite (style->relief.fore);
			bevel->hi_color = style->relief.back;
			bevel->hihi_color = GetShadow (style->relief.back);
		} else
		{
			bevel->hi_color = style->relief.fore;
			bevel->hihi_color = GetHilite (style->relief.fore);
			bevel->lo_color = style->relief.back;
			bevel->lolo_color = GetShadow (style->relief.back);
		}
		bevel->hilo_color = GetAverage (style->relief.fore, style->relief.back);
#if 1
        if( !get_flags (hilite, NO_HILITE_OUTLINE) )
        {
            if (get_flags (hilite, NORMAL_HILITE) )
            {
                bevel->left_outline = bevel->top_outline = bevel->right_outline = bevel->bottom_outline = 1;
                bevel->left_inline = bevel->top_inline =
                    bevel->right_inline = bevel->bottom_inline = extra_hilite + 1;
            } else
            {
#ifndef DONT_HILITE_PLAIN
                bevel->left_inline = bevel->top_inline = bevel->right_inline = bevel->bottom_inline = extra_hilite;
#endif
            }
        }
#endif
        if (get_flags (hilite, LEFT_HILITE))
		{
			bevel->left_outline++;
			bevel->left_inline++;
            if( get_flags (hilite, NO_HILITE_OUTLINE) )
            {
                bevel->left_outline++;
                bevel->left_inline += extra_hilite ;
            }
		}
		if (get_flags (hilite, TOP_HILITE))
		{
			bevel->top_outline++;
			bevel->top_inline++;
            if( get_flags (hilite, NO_HILITE_OUTLINE) )
                bevel->top_inline += extra_hilite ;
        }
		if (get_flags (hilite, RIGHT_HILITE))
		{
			bevel->right_outline++;
			bevel->right_inline++;
            if( get_flags (hilite, NO_HILITE_OUTLINE) )
                bevel->right_inline += extra_hilite ;
		}
		if (get_flags (hilite, BOTTOM_HILITE))
		{
			bevel->bottom_outline++;
			bevel->bottom_inline++;
            if( get_flags (hilite, NO_HILITE_OUTLINE) )
                bevel->bottom_inline += extra_hilite ;
        }
	} else if (bevel)
		memset (bevel, 0x00, sizeof (ASImageBevel));

	return bevel;
}

ASImage      *
mystyle_draw_text_image (MyStyle * style, const char *text, unsigned long encoding)
{
	ASImage      *im = NULL;

	if (style && text)
	{
		if (style->font.as_font)
        {
			switch( encoding )
			{
				case AS_Text_ASCII :
					im = draw_text (text, style->font.as_font, style->text_style, 100);
				    break ;
				case AS_Text_UTF8 :
					im = draw_utf8_text (text, style->font.as_font, style->text_style, 100);
				    break ;
				case AS_Text_UNICODE :
					im = draw_unicode_text ((CARD32*)text, style->font.as_font, style->text_style, 100);
				    break ;
			}
LOCAL_DEBUG_OUT( "im is %p, back_color is %lX", im, style->colors.fore );
            if (im)
            {
				im->back_color = style->colors.fore;
            }
		}
	}
	return im;
}

unsigned int
mystyle_get_font_height (MyStyle * style)
{
	if (style)
		if (style->font.as_font)
			return style->font.as_font->max_height;
	return 1;
}
