/*
 * Copyright (C) 2002 Sasha Vasko <sasha at aftercode.net>
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

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "../configure.h"

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/style.h"
#include "../include/mystyle.h"
#include "../include/misc.h"
#include "../include/screen.h"
#include "../libAfterImage/afterimage.h"
#include "../include/decor.h"


ASTBarData* create_astbar()
{
	ASTBarData* tbar = safecalloc( 1, sizeof(ASTBarData) );
	tbar->rendered_root_x = tbar->rendered_root_y = 0xFFFFFFFF ;
	return tbar ;
}

void destroy_astbar( ASTBarData **ptbar )
{
	if( ptbar ) 
		if( *ptbar ) 
		{
			ASTBarData *tbar = *ptbar ;
			register int i ;
			for( i = 0 ; i < BAR_STATE_NUM ; ++i ) 
			{
				if( tbar->back[i] ) 
					destroy_asimage( &(tbar->back[i]) );
				if( tbar->label[i] ) 
					destroy_asimage( &(tbar->label[i]) );
			}
			if( tbar->left_shape ) 
				destroy_asimage( &(tbar->left_shape));
			if( tbar->center_shape ) 
				destroy_asimage( &(tbar->center_shape));
			if( tbar->right_shape ) 
				destroy_asimage( &(tbar->right_shape));
				
			memset( tbar, 0x00, sizeof(ASTBarData) );
		}
}

unsigned int 
get_astbar_label_width( ASTBarData *tbar )
{
	int size[2] = {1,1} ;
	int i ;
	for( i = 0 ; i < 2 ; ++i )
	{
		if( tbar->label[i] == NULL )
			tbar->label[i] = mystyle_draw_text_image( tbar->style[i], tbar->label_text );
		
		if( tbar->label[i] )
			size[0] = tbar->label[i]->width ;
	}
	return MAX( size[0], size[1] );
}

unsigned int 
get_astbar_label_height( ASTBarData *tbar )
{
	int size[2] = {1,1} ;
	int i ;
	for( i = 0 ; i < 2 ; ++i )
		size[i] = mystyle_get_font_height( tbar->style[i] );
	return MAX( size[0], size[1] );
}


Bool 
set_astbar_size( ASTBarData *tbar, unsigned int width, unsigned int height )
{
	Bool changed = False ;
	if( tbar ) 
	{
		unsigned int w = (width > 0)?width : 1 ;	
		unsigned int h = (height > 0)? height : 1 ;	
		changed = (w != tbar->width || h != tbar->height );
		tbar->width = w ;
		tbar->height = h ;
	}
	return changed ;
}

Bool 
set_astbar_style( ASTBarData *tbar, unsigned int state, const char *style_name )
{
	Bool changed = False ;
	if( tbar && state < BAR_STATE_NUM ) 
	{
		MyStyle *style = mystyle_find_or_default(style_name);
		changed = (style != tbar->style[state]);
		tbar->style[state] = style ;
		if( changed && tbar->back[state] ) 
		{
			destroy_asimage( &(tbar->back[state]) );
			tbar->back[state] = NULL ;
		}
	}
	return changed ;
}

Bool 
set_astbar_label( ASTBarData *tbar, const char *label )
{
	Bool changed = False ;
	if( tbar ) 
	{
		if( label == NULL ) 
		{
			if((changed = (tbar->label_text != NULL )))
			{
				free( tbar->label_text );
				tbar->label_text = NULL ;
			} 
		}else if( tbar->label_text == NULL ) 
		{
			changed = True ;
			tbar->label_text = mystrdup( label );
		}else if( (changed = (strcmp( tbar->label_text, label ) != 0 )) )
		{
			free( tbar->label_text );
			tbar->label_text = mystrdup( label );
		}
		if( changed )
		{
			register int i ;
			for( i = 0 ; i < BAR_STATE_NUM ; ++i ) 
			{
				if( tbar->label[i] ) 
				{
					destroy_asimage( &(tbar->label[i]) );
					tbar->label[i] = NULL ;
				}
			}
		}
	}
	return changed ;
}

Bool 
move_astbar( ASTBarData *tbar, Window w, int win_x, int win_y )
{
	Bool changed = False ;
	int root_x = tbar->root_x, root_y = tbar->root_y ;
	Window wdumm ;
	XTranslateCoordinates( dpy, w, Scr.Root, win_x, win_y, &root_x, &root_y, &wdumm );
	if( tbar ) 
	{
		changed = (root_x != tbar->root_x || root_y != tbar->root_y );
		tbar->root_x = root_x ;
		tbar->root_y = root_y ;
		changed = changed || ( win_x != tbar->win_x || win_y != tbar->win_y );
		tbar->win_x = win_x ;
		tbar->win_y = win_y ;
	}
	return changed ;
}

Bool render_astbar( ASTBarData *tbar, Window w, 
                    unsigned int state, Bool pressed, 
					int clip_x, int clip_y, 
					unsigned int clip_width, unsigned int clip_height )
{
	ASImage *back, *label_im ;
	MyStyle *style ;
	ASImageBevel bevel ;
	ASImageLayer layers[2] ;

	/* input control : */
	if( tbar == NULL || w == None || state >= BAR_STATE_NUM )
		return False ;
	if( clip_x > tbar->width || clip_y > tbar->height ||
	    clip_width == 0 || clip_height == 0 ) 
		return False ;
	if( (style = tbar->style[state]) == NULL ) 
		return False ;
		
	/* validating our images : */	 
	if( (back = tbar->back[state]) != NULL ) 
	{
		if( back->width != tbar->width || back->height != tbar->height ||
		    ((tbar->rendered_root_x != tbar->root_x || tbar->rendered_root_y != tbar->root_y) &&
			  style->texture_type > TEXTURE_PIXMAP))
		{
			destroy_asimage( &(tbar->back[state]) );
			tbar->back[state]	= back = NULL ;
		}
	}
	if( back == NULL ) 
	{
		tbar->back[state]	= back = mystyle_make_image( style, 
		                                                 tbar->root_x, tbar->root_y,
														 tbar->width, tbar->height );

		if( back == NULL ) 
			return False ;	
	}
	
	if( (label_im = tbar->label[state]) == NULL && tbar->label_text != NULL )
	{
		label_im = mystyle_draw_text_image( style, tbar->label_text );
	}

	mystyle_make_bevel( style, &bevel, FULL_HILITE, pressed );
	/* in unfocused and unpressed state we render pixmap and set 
	 * window's background to it 
	 * in focused state or in pressed state we render to 
	 * the window directly, and we'll need to be handling the expose
	 * events
	 */
	init_image_layers(&layers[0], 2 );
	if( state == BAR_STATE_UNFOCUSED ) 
	{
		ASImage *merged_im ;
		layers[0].im = back ;
		layers[0].bevel = &bevel ;
		layers[0].clip_width = clip_width ;
		layers[0].clip_height = clip_height ;		
		layers[1].im = label_im ;
		layers[1].clip_width = clip_width ;
		layers[1].clip_height = clip_height ;		
		
		merged_im = merge_layers( Scr.asv, &layers[0], 2, 
		                          clip_width, clip_height,
								  ASA_XImage, 0, ASIMAGE_QUALITY_DEFAULT );
		if( merged_im ) 
		{
			Pixmap p = asimage2pixmap( Scr.asv, Scr.Root, merged_im, NULL, True );
			if( p != None ) 
			{
				XSetWindowBackgroundPixmap( dpy, w, p );
				XClearWindow( dpy, w );
				XFreePixmap( dpy, p );
				XSync( dpy, False );
			}	
			destroy_asimage( &merged_im );			
			return (p!=None);
		}
	}else
	{
		ASImage *merged_im ;
		layers[0].im = back ;
		layers[0].bevel = &bevel ;
		layers[0].clip_x = clip_x ;
		layers[0].clip_y = clip_y ;		
		layers[0].clip_width = clip_width ;
		layers[0].clip_height = clip_height ;		
		layers[1].im = label_im ;
		layers[1].clip_x = clip_x ;
		layers[1].clip_y = clip_y ;		
		layers[1].clip_width = clip_width ;
		layers[1].clip_height = clip_height ;		
		
		merged_im = merge_layers( Scr.asv, &layers[0], 2, 
		                          clip_width, clip_height,
								  ASA_XImage, 0, ASIMAGE_QUALITY_DEFAULT );
		if( merged_im ) 
		{
			asimage2drawable( Scr.asv, Scr.Root, merged_im, NULL, 
			                  0, 0, clip_x, clip_y, clip_width, clip_height, True );
			destroy_asimage( &merged_im );
			return True ;
		}
	}
	return False ;
}
