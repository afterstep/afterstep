/*
 * Copyright (C) 1998 Sasha Vasko <sasha at aftercode.net>
 * Copyright (C) 1995 Bo Yang
 * Copyright (C) 1993 Robert Nation
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

#include "../../configure.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#ifdef NeXT
#include <fcntl.h>
#endif

#include "Wharf.h"

/****************************************************************************
 *
 * Creates an Icon Window
 *
 ****************************************************************************/
Window
CreateButtonIconWindow (Window win)
{
#ifndef NO_ICONS
  unsigned long valuemask;	/* mask for create windows */
  XSetWindowAttributes attributes;	/* attributes for create windows */

  /* This used to make buttons without icons explode when pushed
     if(((*button).icon_w == 0)&&((*button).icon_h == 0))
     return;
   */
  attributes.background_pixel = Style->colors.back;
  attributes.event_mask = ExposureMask;
  valuemask = CWEventMask | CWBackPixel;

  /* make sure the window geometry does not match the button, so
   * place_buttons() is forced to configure it */
  return create_visual_window( Scr.asv, win, 0, 0, 1, 1, 0, 
      					  	   CopyFromParent, valuemask, &attributes);
#else
  return None ;
#endif
}

/****************************************************************************
 *
 * Combines icon shape masks after a resize
 *
 ****************************************************************************/
void
RenderButtonIcon (button_info * button)
{
#ifndef NO_ICONS
  int i;
  int w, h;
  int xoff, yoff;
  ASImage *background = back_pixmap;
  int bMyIcon = 0;
  ASImageLayer *l ;
  int layers_num = 1 ;

	if (button->width <= 0 || button->height <= 0)
  		return;

    /* handle transparency */
	if (Style->texture_type >= TEXTURE_TRANSPARENT && Style->texture_type < TEXTURE_BUILTIN)
	{
  	    int x = 0, y = 0;
    	ASImage *im;

  		if (button->parent != NULL)
		{
			x = button->parent->x;
			y = button->parent->y;
		}
    	im = mystyle_make_image (Style, x + button->x, y + button->y, button->width, button->height);
    	if (im != NULL)
		{
			background = im;
			bMyIcon = 1;
		}
  	}
	
	layers_num += button->num_icons ;
	l = create_image_layers( layers_num );
	
	l[0].im = background ;
	l[0].solid_color = ARGB32_White ;
	l[0].clip_width = button->width ;
	l[0].clip_height = button->height ;	
	l[0].merge_scanlines = alphablend_scanlines ;
	
	if( (get_asimage_chanmask(button->icons[i])&SCL_DO_ALPHA) != 0 ) 
		set_flags(button->flags, WB_Shaped);
	
	for (i = 0; i < button->num_icons; i++)
	{
		ASImage *im = button->icons[i];
        w = im->width;
	    h = im->height;
  	    if (w < 1 || h < 1)
			continue;

  		if ((im->width >= button->width) &&
			(im->height >= button->height) &&
			(get_asimage_chanmask(im)&SCL_DO_ALPHA))
		    clear_flags (button->flags, WB_Shaped);

    	if (w > button->width)
	  		w = button->width;
		else if (w < 1)
			w = 1;

    	if (h > button->height)
			h = button->height;
		else if (h < 1)
			h = 1;
	
	    xoff = (button->width - w) / 2;
  	    yoff = (button->height - h) / 2;
    	if (xoff < 0)
			xoff = 0;
    	if (yoff < 0)
			yoff = 0;

		l[i+1].im = im ;
		l[i+1].dst_x = xoff ;
		l[i+1].dst_y = yoff ;
		l[i+1].clip_width = w ;
		l[i+1].clip_height = h ;
	}
	
	if( button->completeIcon ) 
		destroy_asimage( &(button->completeIcon) );
	button->completeIcon = merge_layers( Scr.asv, l, layers_num,
	                                     button->width, button->height,
									     ASA_ASImage,
										 100, ASIMAGE_QUALITY_DEFAULT );

	free( l );
    if (bMyIcon && background )
		destroy_asimage( &background );
#endif
}

/****************************************************************************
 * read background icons from data
 ****************************************************************************/
ASImage *
GetXPMData (const char **data)
{
	return xpm_data2ASImage( data, 0xFFFFFFFF, 2.2, NULL, 0, 100 );
}
