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

static char  *DefaultMyStyleName = "default";


void
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
	CARD32 a = make_component_hilite (ARGB32_ALPHA8 (background));
	CARD32 r = make_component_hilite (ARGB32_RED8 (background));
	CARD32 g = make_component_hilite (ARGB32_GREEN8 (background));
	CARD32 b = make_component_hilite (ARGB32_BLUE8 (background));

	return MAKE_ARGB32 (a, r, g, b);
}

/* This routine computes the shadow color from the background color */
inline        ARGB32
GetShadow (ARGB32 background)
{
	CARD16        a, r, g, b;

	a = (ARGB32_ALPHA8 (background))*3/4;
	r = (ARGB32_RED8 (background))*3/4;
	g = (ARGB32_GREEN8 (background))*3/4;
	b = (ARGB32_BLUE8 (background))*3/4;
	
	return MAKE_ARGB32 (a, r, g, b);
}

inline        ARGB32
GetAverage (ARGB32 foreground, ARGB32 background)
{
	CARD16        a, r, g, b;

	a = (ARGB32_ALPHA8 (foreground) + ARGB32_ALPHA8 (background))/2;
	r = (ARGB32_RED8 (foreground) + ARGB32_RED8 (background))/2;
	g = (ARGB32_GREEN8 (foreground) + ARGB32_GREEN8 (background))/2;
	b = (ARGB32_BLUE8 (foreground) + ARGB32_BLUE8 (background))/2;
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
        if( (list = ASDefaultScr->Look.styles_list) == NULL )
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
ASImage *
grab_root_asimage( ScreenInfo *scr, Window target, Bool screenshot )
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
		scr = ASDefaultScr ;
	/* this only works if we use DefaultVisual - same visual as the Root window :*/
	if( scr->asv->visual_info.visual != DefaultVisual( dpy, DefaultScreen(dpy) ) )
		return NULL ;

	if( target != None && target != scr->Root )
	{
		Window wdumm ;
    	unsigned int udumm ;
		unsigned int bw = 0 ;
		int tx, ty ;
		unsigned int tw, th ;

	    if( XGetGeometry(dpy, target, &wdumm, &tx, &ty, &tw, &th, &bw, &udumm )!= 0 )
		{
			/* we need root window position : */
			XTranslateCoordinates (dpy, target, scr->Root, 0, 0, &tx, &ty, &wdumm);
			x = tx - bw;
			y = ty - bw;
			width = tw + bw + bw;
			height = th + bw + bw ;
		}
	}

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
	if( x + width > ASDefaultScrWidth )
		width = (int)(ASDefaultScrWidth) - x ;
	if( y + height > ASDefaultScrHeight )
		height = (int)(ASDefaultScrHeight) - y ;

	if( height < 0 || width < 0 )
		return NULL ;

	attr.background_pixmap = screenshot?None:ParentRelative ;
	attr.backing_store = NotUseful ;
	attr.event_mask = ExposureMask ;
	attr.override_redirect = True ;

	LOCAL_DEBUG_OUT( "grabbing root image from %dx%d%+d%+d", width, height, x, y );

    src = create_visual_window( scr->asv, scr->Root, x, y, width, height, 0, CopyFromParent,
				  				CWBackPixmap|CWBackingStore|CWOverrideRedirect|CWEventMask,
				  				&attr);

	if( src == None ) return NULL ;
	grab_server();
	grabbed = True ;
	XMapRaised( dpy, src );
	XSync(dpy, False );
	start_ticker(1);
	/* now we have to wait for our window to become mapped - waiting for Expose */
	for( tick_count = 0 ; !XCheckWindowEvent( dpy, src, ExposureMask, &event ) && tick_count < 100 ; tick_count++)
  		/*sleep_a_millisec(500); */
		wait_tick();
	if( tick_count < 100 )
        if( (root_im = pixmap2asimage( scr->asv, src, 0, 0, width, height, AllPlanes, False, 100 )) != NULL )
		{
			if( (scr->RootClipArea.y < 0 || scr->RootClipArea.y < 0) && !screenshot )
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
	ungrab_server();
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
	tmp = flip_asimage( ASDefaultVisual, im, 0, 0, width, height, flip, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
	if( tmp != NULL )
	{
		safe_asimage_destroy( im );
		im = tmp;
	}
	return im;
}

static ASImage *
clip_root_pixmap( Pixmap root_pixmap, int width, int height )
{
	ASImage *result = NULL ;
	XRectangle *clip_area = &(ASDefaultScr->RootClipArea) ; 

	if (root_pixmap)
	{
    	ASImage *tmp_root ;
		int root_x = 0, root_y = 0 ;
		int clip_x = clip_area->x ;
		int clip_y = clip_area->y ;

		while( clip_x < 0 )
			clip_x += ASDefaultScrWidth ;
		clip_x %= width ;
		while( clip_y < 0 )
			clip_y += ASDefaultScrHeight ;
		clip_y %= height ;

		/* at this point clip_x and clip_y fall into root pixmap */

		if( clip_x + clip_area->width  <= width  ) 
		{
			root_x = clip_x ; 
			width = clip_area->width ;
		}
		if( clip_y + clip_area->height <= height  ) 
		{
			root_y = clip_y ; 
			height = clip_area->height  ;
		}
		/* fprintf( stderr, "RootPixmap2RootImage %dx%d%+d%+d", root_w, root_h, root_x,
		root_y); */
    	tmp_root = pixmap2asimage (ASDefaultVisual, root_pixmap, root_x, root_y, width, height, AllPlanes, False, 100);

		LOCAL_DEBUG_OUT ("Root pixmap ASImage = %p, size = %dx%d", tmp_root, tmp_root?tmp_root->width:0, tmp_root?tmp_root->height:0);
    	if( tmp_root )
    	{
        	if( clip_x == root_x && clip_y == root_y &&
            	clip_area->width == width && clip_area->height == height )
        	{
            	result = tmp_root ;
        	}else
        	{
            	result = tile_asimage ( ASDefaultVisual, tmp_root,
										(clip_x == root_x)?0:clip_area->x, 
										(clip_y == root_y)?0:clip_area->y,
                                        clip_area->width, clip_area->height, TINT_NONE,
                                        ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT);
            	destroy_asimage( &tmp_root );
        	}
    	}
    }
	return result;
}	 

static ASImage      *
mystyle_make_image_int (MyStyle * style, int root_x, int root_y, int crop_x, int crop_y, int crop_width, int crop_height, int scale_width, int scale_height, int flip, ASImage *underlayment, int overlay_type )
{
	ASImage      *im = NULL;
	Pixmap        root_pixmap;
	Bool transparency = TransparentMS(style) ;
	int preflip_width, preflip_height ;
	int width, height ; 
	
	if(underlayment ) 
	{
		if( overlay_type >= TEXTURE_SCALED_TRANSPIXMAP )
			overlay_type = TEXTURE_TRANSPIXMAP + (overlay_type - TEXTURE_SCALED_TRANSPIXMAP);
		if( overlay_type < TEXTURE_TRANSPIXMAP || overlay_type >= TEXTURE_TRANSPIXMAP_END ) 
			overlay_type = TEXTURE_TRANSPIXMAP_ALPHA ;
		
		if( transparency )
			transparency = (overlay_type != TEXTURE_TRANSPIXMAP_ALPHA);
	} 

	if( crop_x < 0 ) 
	{
		root_x += crop_x ; 
		width = scale_width-crop_x ; 
		crop_x = 0 ;
		if( width < crop_width )
			width = crop_width;
	}else
	{
		width = crop_width + crop_x ; 
		if( width < scale_width ) 
			width = scale_width ;
	}

	if( crop_y < 0 ) 
	{
		root_y += crop_y ; 
		height = scale_height-crop_y ; 
		crop_y = 0 ;
		if( height < crop_height )
			height = crop_height;
	}else
	{
		height = crop_height + crop_y ; 
		if( height < scale_height ) 
			height = scale_height ;
	}
	
	if (width < 1)
		width = 1;
	if (height < 1)
		height = 1;
		
		
    LOCAL_DEBUG_OUT ("style \"%s\", texture_type = %d, im = %p, tint = 0x%lX, geom=(%dx%d%+d%+d), flip = %d", style->name, style->texture_type,
                     style->back_icon.image, style->tint, width, height, root_x, root_y, flip);

	if( transparency )
	{	/* in this case we need valid root image : */
		
		if (ASDefaultScr->RootImage == NULL)
		{
			unsigned int  root_w, root_h;
			root_pixmap = ValidatePixmap (None, 1, 1, &root_w, &root_h);
			LOCAL_DEBUG_OUT ("obtained Root pixmap = %lX", root_pixmap);
			ASDefaultScr->RootImage = clip_root_pixmap( root_pixmap, root_w, root_h );
             
			if( ASDefaultScr->RootImage == NULL )
                ASDefaultScr->RootImage = grab_root_asimage( ASDefaultScr, None, False );
		}
        LOCAL_DEBUG_OUT ("RootImage = %p clip(%ux%u%+d%+d) size(%dx%d)", ASDefaultScr->RootImage, ASDefaultScr->RootClipArea.width, ASDefaultScr->RootClipArea.height, ASDefaultScr->RootClipArea.x, ASDefaultScr->RootClipArea.y, ASDefaultScr->RootImage?ASDefaultScr->RootImage->width:0, ASDefaultScr->RootImage?ASDefaultScr->RootImage->height:0);
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
		 	im = make_gradient (ASDefaultVisual, grad, width, height, 0xFFFFFFFF, ASA_ASImage, 0,
							 ASIMAGE_QUALITY_DEFAULT);
			if( grad != &(style->gradient) )
				destroy_asgradient( &grad );
		}
		break;
	 case TEXTURE_SHAPED_SCALED_PIXMAP:
	 case TEXTURE_SCALED_PIXMAP:
	 	if( get_flags( style->set_flags, F_SLICE ) )
		{
			im = slice_asimage2(ASDefaultVisual, style->back_icon.image, 
								style->slice_x_start, style->slice_x_end,
								style->slice_y_start, style->slice_y_end,
								preflip_width, preflip_height, True, 
								ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);			   
		}else	 
			im = scale_asimage (ASDefaultVisual, style->back_icon.image, 
								preflip_width, preflip_height, 
								ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
		if( flip != 0 )
			im = mystyle_flip_image( im, width, height, flip );
		break;
	 case TEXTURE_SHAPED_PIXMAP:
	 case TEXTURE_PIXMAP:
	 	 if( get_flags( style->set_flags, F_SLICE ) )
		 {
		 	im = slice_asimage2(ASDefaultVisual, style->back_icon.image, 
								style->slice_x_start, style->slice_x_end,
								style->slice_y_start, style->slice_y_end,
								preflip_width, preflip_height, False, 
								ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);			   
		 }else
		 {	
		 	im = tile_asimage ( ASDefaultVisual, style->back_icon.image,
								0, 0, 
								preflip_width, preflip_height, TINT_LEAVE_SAME, 
								ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
		 }
		 LOCAL_DEBUG_OUT( "back_icon.image = %p,im = %p, preflip_size=%dx%d", style->back_icon.image, im, preflip_width, preflip_height );
		 if( flip != 0 )
		 	im = mystyle_flip_image( im, width, height, flip );
		 break;
	}

	if( transparency )
	{
		if (ASDefaultScr->RootImage != NULL)
		{
			if (style->texture_type == TEXTURE_TRANSPARENT || style->texture_type == TEXTURE_TRANSPARENT_TWOWAY)
			{
                im = tile_asimage ( ASDefaultVisual, ASDefaultScr->RootImage, 
									root_x-ASDefaultScr->RootClipArea.x, root_y-ASDefaultScr->RootClipArea.y,
									width, height, style->tint, 
									ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
			} else
			{
				 ASImageLayer  layers[2];
				 ASImage      *scaled_im = NULL;
				 Bool do_scale = False ;
				 int           index = 1;      /* default is alphablending !!! */

				 init_image_layers (&layers[0], 2);

				 layers[0].im = ASDefaultScr->RootImage;
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
                 layers[0].clip_x = root_x-ASDefaultScr->RootClipArea.x;
                 layers[0].clip_y = root_y-ASDefaultScr->RootClipArea.y;
				 layers[0].clip_width = width;
				 layers[0].clip_height = height;

				 layers[1].im = im?im:style->back_icon.image;
				 	
				 do_scale = (style->texture_type >= TEXTURE_SCALED_TRANSPIXMAP &&
					 		 style->texture_type < TEXTURE_SCALED_TRANSPIXMAP_END);

				if( get_flags( style->set_flags, F_SLICE ) )
				{
					scaled_im = slice_asimage2 (ASDefaultVisual, layers[1].im, 
												style->slice_x_start, style->slice_x_end,
												style->slice_y_start, style->slice_y_end,
												preflip_width, preflip_height, do_scale, 
												ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);			   
 					LOCAL_DEBUG_OUT( "image scliced to %p", scaled_im ) ;
				}else if( do_scale )  
				{
					scaled_im = scale_asimage ( ASDefaultVisual, layers[1].im, 
												preflip_width, preflip_height,
											    ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
 					LOCAL_DEBUG_OUT( "image scaled to %p", scaled_im ) ;
				}
				
				 if (scaled_im)	 
				 {	
					 /* here layers[1].im is always style->back_icon.image, so we should not destroy it ! */
					 scaled_im = mystyle_flip_image( scaled_im, width, height, flip );
					 layers[1].im = scaled_im ;
					 LOCAL_DEBUG_OUT( "image flipped to %p", layers[1].im ) ;
					 /* scaled_im got destroyed in mystyle_flip_image if it had to be */
				 }else if( flip != 0 && layers[1].im == style->back_icon.image )
				 {
					/* safely assuming that im is NULL at this point,( see logic above ) */
					 layers[1].im = im = flip_asimage( ASDefaultVisual, layers[1].im, 
					 								   0, 0, 
													   width, height, flip, 
													   ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
				 }
				 layers[1].merge_scanlines = layers[0].merge_scanlines;
				 layers[1].dst_x = 0;
				 layers[1].dst_y = 0;
				 layers[1].clip_x = 0;
				 layers[1].clip_y = 0;
				 layers[1].clip_width = width;
				 layers[1].clip_height = height;

				{
					ASImage *tmp = merge_layers(ASDefaultVisual, &layers[0], 2, 
												width, height, 
												ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
					if( tmp )
					{
						if( im )
						{
							if (style->texture_type == TEXTURE_SHAPED_SCALED_PIXMAP ||
							    style->texture_type == TEXTURE_SHAPED_PIXMAP)
							{
								/*we need to keep alpha channel intact */
								LOCAL_DEBUG_OUT( "copying alpha channel from %p to %p", im, tmp );
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
		}else
			show_warning( "MyStyle \"%s\" : failed to accure Root background image", style->name );
	}
    /* simply creating solid color image under no circumstances we want to return NULL here */
	if( im == NULL )
	{
		im = create_asimage( width, height, 100 );
		im->back_color = style->colors.back ;
		show_warning( "MyStyle \"%s\" : failed to generate background image - using solid fill instead with color #0x%8.8X", style->name, style->colors.back );
	}

	if( underlayment )
	{
		 ASImageLayer  layers[2];
		 int           index = 1;      /* default is alphablending !!! */

		 init_image_layers (&layers[0], 2);

		 layers[0].im = underlayment;
		 index = overlay_type - TEXTURE_TRANSPIXMAP;

		 LOCAL_DEBUG_OUT ("index = %d", index);
		 layers[0].merge_scanlines = mystyle_merge_scanlines_func_xref[index];
         layers[0].dst_x = 0;
         layers[0].dst_y = 0;
         layers[0].clip_x = 0;
         layers[0].clip_y = 0;
		 layers[0].clip_width = width;
		 layers[0].clip_height = height;

		 layers[1].im = im;
		 layers[1].merge_scanlines = layers[0].merge_scanlines;
		 layers[1].dst_x = 0;
		 layers[1].dst_y = 0;
		 layers[1].clip_x = 0;
		 layers[1].clip_y = 0;
		 layers[1].clip_width = width;
		 layers[1].clip_height = height;

		{
			ASImage *tmp = merge_layers(ASDefaultVisual, &layers[0], 2, 
										width, height, 
										ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
			if( tmp )
			{
				if( im )
				{
					if (style->texture_type == TEXTURE_SHAPED_SCALED_PIXMAP ||
						style->texture_type == TEXTURE_SHAPED_PIXMAP)
					{
						/*we need to keep alpha channel intact */
						LOCAL_DEBUG_OUT( "copying alpha channel from %p to %p", im, tmp );
						copy_asimage_channel(tmp, IC_ALPHA, im, IC_ALPHA);
					}
					safe_asimage_destroy (im);
				}
				im = tmp ;
			}
		}
	}

	if( style->overlay ) 
	{
		ASImage *overlayed = mystyle_make_image_int (style->overlay, root_x, root_y, crop_x, crop_y, crop_width, crop_height, scale_width, scale_height, flip, im, style->overlay_type );
/* 		fprintf( stderr, "overlay_style = %p\n", style->overlay ); */
		if( overlayed ) 
		{
			if( im && overlayed != im ) 
				safe_asimage_destroy (im);
			im = overlayed ;
		}			
	}
	if( crop_x > 0 || crop_y > 0 || 
		(crop_width > 0 && crop_width < im->width ) ||
		(crop_height > 0 && crop_height < im->height ) ) 
	{
		ASImage *cropped_im = tile_asimage( ASDefaultVisual, im,
											crop_x, crop_y,
											crop_width, crop_height,
											TINT_LEAVE_SAME,
											ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT );
		if( cropped_im ) 
		{
			if( im && cropped_im != im ) 
				safe_asimage_destroy (im);
			im = cropped_im ;
		}			
	}
	return im;
}

ASImage      *
mystyle_make_image(MyStyle * style, int root_x, int root_y, int width, int height, int flip )
{
	if( style != NULL ) 
		return mystyle_make_image_int (style, root_x, root_y, 0, 0, width, height, width, height, flip, NULL, 0 );
	return NULL ;
}

ASImage      *
mystyle_crop_image(MyStyle * style, int root_x, int root_y, int crop_x, int crop_y, int width, int height, int scale_width, int scale_height, int flip )
{
	if( style != NULL ) 
		return mystyle_make_image_int (style, root_x, root_y, crop_x, crop_y, width, height, scale_width, scale_height, flip, NULL, 0 );
	return NULL ;
}


/*************************************************************************/
/* Mystyle creation/deletion                                             */
/*************************************************************************/
static void
mystyle_free_resources( MyStyle *style )
{
    if( style->magic == MAGIC_MYSTYLE )
    {
        if (get_flags( style->user_flags, F_FONT))
        {
            unload_font (&style->font);
        }
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
	style->gradient.npoints = 0;
	style->gradient.color = NULL;
	style->gradient.offset = NULL;
	style->back_icon.pix = None;
	style->back_icon.mask = None;
	style->back_icon.alpha = None;
    style->tint = TINT_LEAVE_SAME;
	style->back_icon.image = NULL;
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
    MyStyle      *style = NULL ;
	ASHashData   hdata = {0} ;

    if( name == NULL ) return NULL ;

    if( list == NULL )
    {
        if( ASDefaultScr->Look.styles_list == NULL )
            if( (ASDefaultScr->Look.styles_list = mystyle_list_init())== NULL ) return NULL;
        list = ASDefaultScr->Look.styles_list ;
    }

    if( get_hash_item( list, AS_HASHABLE(name), &hdata.vptr ) == ASH_Success )
	{
        if( (style = hdata.vptr) != NULL )
        {
            if( style->magic == MAGIC_MYSTYLE )
                return style;
            else
                remove_hash_item( list, (ASHashableValue)name, NULL, True );
        }
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
        plist = &(ASDefaultScr->Look.styles_list) ;
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
    ASHashData hdata = {0};
    if( list == NULL )
        list = ASDefaultScr->Look.styles_list ;

    if( list && name )
        if( get_hash_item( list, AS_HASHABLE((char*)name), &hdata.vptr ) != ASH_Success )
            hdata.vptr = NULL ;
    return hdata.vptr;
}

MyStyle *
mystyle_list_find_or_default (struct ASHashTable *list, const char *name)
{
    ASHashData hdata = {0};

    if( list == NULL )
        list = ASDefaultScr->Look.styles_list ;

    if( name == NULL )
        name = DefaultMyStyleName ;
    if( list && name )
        if( get_hash_item( list, AS_HASHABLE((char*)name), &hdata.vptr ) != ASH_Success )
            if( get_hash_item( list, AS_HASHABLE(DefaultMyStyleName), &hdata.vptr ) != ASH_Success )
                hdata.vptr = NULL ;
    return hdata.vptr;
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
mystyle_merge_font( MyStyle *style, MyFont *font, Bool override)
{
    if ((override == True) && (style->user_flags & F_FONT))
        unload_font (&style->font);
    if ((override == True) || !(style->set_flags & F_FONT))
    {
		set_string( &(style->font.name), mystrdup(font->name));
        set_flags(style->user_flags, F_FONT);
        clear_flags(style->inherit_flags, F_FONT);
    }
}

void
mystyle_merge_styles (MyStyle * parent, MyStyle * child, Bool override, Bool copy)
{
	if( parent == NULL || child == NULL ) 
		return;
	if (parent->set_flags & F_FONT)
        mystyle_merge_font( child, &(parent->font), override);

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
	if (parent->set_flags & F_SLICE)
	{
		if ((override == True) || !(child->set_flags & F_SLICE))
		{
			child->slice_x_start = parent->slice_x_start;
			child->slice_x_end = parent->slice_x_end;
			child->slice_y_start = parent->slice_y_start;
			child->slice_y_end = parent->slice_y_end;
			if (copy == False)
			{
				child->user_flags &= ~F_SLICE;
				child->inherit_flags |= F_SLICE;
			} else
			{
				child->user_flags |= F_SLICE;
				child->inherit_flags &= ~F_SLICE;
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
				clear_flags (child->user_flags, F_BACKPIXMAP | F_BACKTRANSPIXMAP);
				set_flags (child->inherit_flags, F_BACKPIXMAP);
				if (get_flags (parent->set_flags, F_BACKTRANSPIXMAP))
					set_flags (child->inherit_flags, F_BACKTRANSPIXMAP);
			} else
			{
				GC            gc = create_visual_gc (ASDefaultVisual, ASDefaultRoot, 0, NULL);

				child->back_icon.pix =
					XCreatePixmap (dpy, ASDefaultRoot, parent->back_icon.width, parent->back_icon.height,
								   ASDefaultVisual->visual_info.depth);
				XCopyArea (dpy, parent->back_icon.pix, child->back_icon.pix, gc, 0, 0, parent->back_icon.width,
						   parent->back_icon.height, 0, 0);
				if (parent->back_icon.mask != None)
				{
					GC            mgc = XCreateGC (dpy, parent->back_icon.mask, 0, NULL);

					child->back_icon.mask =
						XCreatePixmap (dpy, ASDefaultRoot, parent->back_icon.width, parent->back_icon.height, 1);
					XCopyArea (dpy, parent->back_icon.mask, child->back_icon.mask, mgc, 0, 0, parent->back_icon.width,
							   parent->back_icon.height, 0, 0);
					XFreeGC (dpy, mgc);
				}
				if (parent->back_icon.alpha != None)
				{
					GC            mgc = XCreateGC (dpy, parent->back_icon.alpha, 0, NULL);

					child->back_icon.alpha =
						XCreatePixmap (dpy, ASDefaultRoot, parent->back_icon.width, parent->back_icon.height, 8);
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
	if (parent->set_flags & F_OVERLAY)
	{
		if ((override == True) || !(child->set_flags & F_OVERLAY))
		{
			child->flags &= ~F_OVERLAY;
			child->flags |= parent->flags & F_OVERLAY;
			child->overlay = parent->overlay ;
			child->overlay_type = parent->overlay_type ;
			if (copy == False)
			{
				child->user_flags &= ~F_OVERLAY;
				child->inherit_flags |= F_OVERLAY;
			} else
			{
				child->user_flags |= F_OVERLAY;
				child->inherit_flags &= ~F_OVERLAY;
			}
		}
	}
	child->set_flags = child->user_flags | child->inherit_flags;
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
	if( gradient )
	{
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
		gradient->type = mystyle_translate_grad_type(type) ;
	}
	return type;
}

void
mystyle_merge_colors (MyStyle * style, int type, char *fore, char *back,
					   char *gradient, char *pixmap)
{
	if( style == NULL ) 
		return ;
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
            if ( load_icon(&(style->back_icon), pixmap, ASDefaultScr->image_manager ))
            {
                style->texture_type = type;
                set_flags(style->user_flags, F_BACKPIXMAP);
            } else
                show_error ("failed to load image file \"%s\" in MyStyle \"%s\".", pixmap, style->name);
        }
	}
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
	if (style && (hilite&HILITE_MASK) != 0 && 
		(hilite&(NO_HILITE_INLINE|NO_HILITE_OUTLINE)) != (NO_HILITE_INLINE|NO_HILITE_OUTLINE))
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
		bevel->hilo_color = GetAverage (bevel->hi_color, bevel->lo_color);
#if 1
        if( !get_flags (hilite, NO_HILITE_OUTLINE) )
        {
			if (get_flags (hilite, NORMAL_HILITE) )
            {
                bevel->left_outline = bevel->top_outline = bevel->right_outline = bevel->bottom_outline = 1;
                bevel->left_inline = bevel->top_inline =
                    bevel->right_inline = bevel->bottom_inline = extra_hilite + get_flags( hilite, NO_HILITE_INLINE )?0:1;
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
			if( !get_flags( hilite, NO_HILITE_INLINE ) )
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
			if( !get_flags( hilite, NO_HILITE_INLINE ) )
				bevel->top_inline++;
            if( get_flags (hilite, NO_HILITE_OUTLINE) )
                bevel->top_inline += extra_hilite ;
        }
		if (get_flags (hilite, RIGHT_HILITE))
		{
			bevel->right_outline++;
			if( !get_flags( hilite, NO_HILITE_INLINE ) )
				bevel->right_inline++;
            if( get_flags (hilite, NO_HILITE_OUTLINE) )
                bevel->right_inline += extra_hilite ;
		}
		if (get_flags (hilite, BOTTOM_HILITE))
		{
			bevel->bottom_outline++;
			if( !get_flags( hilite, NO_HILITE_INLINE ) )
				bevel->bottom_inline++;
            if( get_flags (hilite, NO_HILITE_OUTLINE) )
                bevel->bottom_inline += extra_hilite ;
        }
/* experimental code */
#if 1
		if( !get_flags( hilite, NO_HILITE_INLINE ) )
		{	
			if( bevel->top_outline > 1 ) 
			{	
				bevel->top_inline += bevel->top_outline - 1 ;
				bevel->top_outline = 1 ;
			}
			if( bevel->left_outline > 1 ) 
			{	
				bevel->left_inline += bevel->left_outline - 1 ;
				bevel->left_outline = 1 ;
			}
			if( bevel->right_outline > 1 ) 
			{	
				bevel->right_inline += bevel->right_outline - 1 ;
				bevel->right_outline = 1 ;
			}
			if( bevel->bottom_outline > 1 ) 
			{	
				bevel->bottom_inline += bevel->bottom_outline - 1 ;
				bevel->bottom_outline = 1 ;
			}
		}
#endif
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
		/* load fonts on demand only - no need to waste memory if ain't never gonna use it ! */
		if ( style->font.as_font == NULL )
			load_font( NULL, &style->font );

		if (style->font.as_font)
        {
			ASTextAttributes attr = {ASTA_VERSION_1, ASTA_UseTabStops, AST_Plain, ASCT_Char, 8, 0, NULL, 0, ARGB32_White };

			attr.type = style->text_style ;
			attr.fore_color = style->colors.fore ;
		
			switch( encoding )
			{
				case AS_Text_ASCII : attr.char_type = ASCT_Char;	break ;
				case AS_Text_UTF8  : attr.char_type = ASCT_UTF8; 	break ;
				case AS_Text_UNICODE: attr.char_type = ASCT_Unicode; 	break ;
			}
			
			im = draw_fancy_text( text, style->font.as_font, &attr, 100, 0 );

			LOCAL_DEBUG_OUT( "encoding is %ld, im is %p, back_color is %lX", encoding, im, style->colors.fore );
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
	{
		if (style->font.as_font == NULL )
			load_font( NULL, &style->font );

		if (style->font.as_font)
			return style->font.as_font->max_height;
	}
	return 1;
}

void
mystyle_get_text_size (MyStyle * style, const char *text, unsigned int *width, unsigned int *height )
{
	if (style && text)
	{
		if (style->font.as_font == NULL )
			load_font( NULL, &style->font );

		if (style->font.as_font)
			get_text_size( text, style->font.as_font, style->text_style, width, height );
	}
}

