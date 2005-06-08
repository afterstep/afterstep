/****************************************************************************
 * Copyright (c) 2000,2001 Sasha Vasko <sasha at aftercode.net>
 * Copyright (c) 1999 Ethan Fisher <allanon@crystaltokyo.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/
/***********************************************************************
 * desktop cover for houskeeping mode
 ***********************************************************************/
#define LOCAL_DEBUG

#include "../../configure.h"

#include "asinternals.h"

#include <stdarg.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "../../libAfterStep/colorscheme.h"

/**************************************************************************
 * Different Desktop change Animation types
 **************************************************************************/

static CARD32  rnd32_seed = 345824357;

#define MAX_MY_RND32        0x00ffffffff
#ifdef WORD64
#define MY_RND32() \
(rnd32_seed = ((1664525L*rnd32_seed)&MAX_MY_RND32)+1013904223L)
#else
#define MY_RND32() \
(rnd32_seed = (1664525L*rnd32_seed)+1013904223L)
#endif

#define BLOCKS_NUM	10
#define LEVELS_NUM 	10

struct ASDeskAniBlocks
{
	Window cover ;
	int steps, steps_done ;
	unsigned int open_height ;
	unsigned char *stripes[LEVELS_NUM];
	XRectangle    blocks[BLOCKS_NUM];
	int           off_y[LEVELS_NUM];
	 
};

void  do_anim_shape_blocks( void *vdata );


void
desk_anim_shape_blocks (ScreenInfo *scr, Window cover, unsigned int steps)
{
#ifdef SHAPE
	XRectangle    main_b = { 0, 0, scr->MyDisplayWidth, scr->MyDisplayHeight };
	struct ASDeskAniBlocks *data = safecalloc( 1, sizeof(struct ASDeskAniBlocks));
	int           y_dim = scr->MyDisplayHeight / steps;
	int i;

	for (i = 0; i < LEVELS_NUM; i++)
	{	
		data->off_y[i] = (i - (LEVELS_NUM - 1)) * y_dim;
		data->stripes[i] = safecalloc( BLOCKS_NUM, 1 );
	}

	XShapeCombineRectangles (dpy, cover, ShapeBounding, 0, 0, &main_b, 1, ShapeSet, Unsorted);
	data->cover = cover ; 
	data->steps = steps ; 
	
	timer_new (20, do_anim_shape_blocks, data);	   	 
#endif
}

void 
do_anim_shape_blocks( void *vdata )
{
#ifdef SHAPE
 	struct ASDeskAniBlocks *data = (struct ASDeskAniBlocks*) vdata ; 
	XRectangle    main_b = { 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight };
	int           ratio = MAX_MY_RND32 / LEVELS_NUM;
	int           x_dim = Scr.MyDisplayWidth / BLOCKS_NUM;
    int           y_dim = Scr.MyDisplayHeight / data->steps;
	int           level, th;
	unsigned char *tmp;

	if (y_dim < 2)
		y_dim = 2;


	th = MAX_MY_RND32;
        
	level = LEVELS_NUM ; 
    while ( --level >= 0 )
	{
		int           blocks_used;
		int  i = 0;
		
		th -= ratio;
		if (data->off_y[level] < 0)
			continue;
		blocks_used = 0;
		
		tmp = data->stripes[level];
		for (i = 0; i < BLOCKS_NUM; i++)
		{
			if (tmp[i] == 0 && MY_RND32 () > th)
			{
				data->blocks[blocks_used].y = 0 ;
				data->blocks[blocks_used].x = i * x_dim;
				data->blocks[blocks_used].width = x_dim;
				data->blocks[blocks_used].height = y_dim;
				blocks_used++;
				tmp[i] = 1;
			}
		}
		XShapeCombineRectangles (dpy, data->cover, ShapeBounding, 0, data->off_y[level], data->blocks,
									blocks_used, ShapeSubtract, Unsorted);
		XFlush (dpy);
	}
	if (data->off_y[0] >= 0)
		data->open_height += y_dim;

	tmp = data->stripes[0];
	for (level = 0; level < LEVELS_NUM; level++)
	{
		data->off_y[level] += y_dim;
		if (level < LEVELS_NUM - 1)
			data->stripes[level] = data->stripes[level + 1];
	}
	data->stripes[LEVELS_NUM - 1] = tmp;
	memset (tmp, 0x00, BLOCKS_NUM);
	
	main_b.height = data->open_height ;
	if (main_b.height > 0)
		XShapeCombineRectangles (dpy, data->cover, ShapeBounding, 0, 0, &main_b, 1, ShapeSubtract,
									Unsorted);

	++data->steps_done ;
	if( data->steps_done >= data->steps ) 
	{
		for (level = 0; level < LEVELS_NUM; level++)		
			free( data->stripes[level] );
		XDestroyWindow (dpy, data->cover);
		free( data );
	}else
		timer_new (20, do_anim_shape_blocks, vdata);	   	 
#endif
}

struct ASDeskAniMove
{
	Window cover ;
	int steps, steps_done ;
	int dirx, diry ;
	int px, py ; 		
};

void  do_anim_slide( void *vdata );
void  do_anim_shrink( void *vdata );

void
desk_anim_slide (ScreenInfo *scr, Window cover, int dirx, int diry, unsigned int steps)
{
	struct ASDeskAniMove *data = safecalloc( 1, sizeof(struct ASDeskAniMove)); 	

	data->cover = cover ;
	data->dirx = dirx ;
	data->diry = diry ;
	data->steps = steps ;

	timer_new (20, do_anim_slide, data);	   	 
}

void 
do_anim_slide( void *vdata )
{
	struct ASDeskAniMove *data = (struct ASDeskAniMove*)vdata ;
		
    int           inc = Scr.MyDisplayWidth / data->steps;
	
	if (inc == 0)
		inc = 1;
	data->px += data->dirx * inc;
	data->py += data->diry * inc;
	XMoveWindow (dpy, data->cover, data->px, data->py);
	XFlush (dpy);
	++(data->steps_done);
	if( data->steps_done >= data->steps ) 
	{
		XDestroyWindow (dpy, data->cover);
		free( data );
	}else
		timer_new (20, do_anim_slide, vdata);	   	 
}


void
desk_anim_shrink (ScreenInfo *scr, Window cover, int dirx, int diry, unsigned int steps)
{
    
	struct ASDeskAniMove *data = safecalloc( 1, sizeof(struct ASDeskAniMove)); 	

	data->cover = cover ;
	data->dirx = dirx ;
	data->diry = diry ;
	data->steps = steps ;
	data->px = scr->MyDisplayWidth ;
	data->py = scr->MyDisplayHeight ;

	timer_new (20, do_anim_shrink, data);	   	 
}
	  
void 
do_anim_shrink( void *vdata )
{
	struct ASDeskAniMove *data = (struct ASDeskAniMove*)vdata ;
	int           inc = Scr.MyDisplayWidth / data->steps;
	
	if (inc == 0)
		inc = 1;
	
	data->px += data->dirx * inc;
	if (data->px < 1)
		data->px = 1;
	data->py += data->diry * inc;
	if (data->py < 1)
		data->py = 1;
	XResizeWindow (dpy, data->cover, data->px, data->py);
	XSync (dpy, False);

	++(data->steps_done);
	if( data->steps_done >= data->steps ) 
	{
		XDestroyWindow (dpy, data->cover);
		free( data );
	}else
		timer_new (20, do_anim_shrink, data);	   	 
}

void
desk_anim_slideN (ScreenInfo *scr, Window cover, unsigned int steps)
{
	desk_anim_slide (scr, cover, 0, -1, steps);
}

void
desk_anim_slideW (ScreenInfo *scr, Window cover, unsigned int steps)
{
	desk_anim_slide (scr, cover, -1, 0, steps);
}

void
desk_anim_slideE (ScreenInfo *scr, Window cover, unsigned int steps)
{
	desk_anim_slide (scr, cover, 1, 0, steps);
}

void
desk_anim_slideS (ScreenInfo *scr, Window cover, unsigned int steps)
{
	desk_anim_slide (scr, cover, 0, 1, steps);
}

void
desk_anim_slideNW (ScreenInfo *scr, Window cover, unsigned int steps)
{
	desk_anim_slide (scr, cover, -1, -1, steps);
}

void
desk_anim_slideNE (ScreenInfo *scr, Window cover, unsigned int steps)
{
	desk_anim_slide (scr, cover, 1, -1, steps);
}

void
desk_anim_slideSE (ScreenInfo *scr, Window cover, unsigned int steps)
{
	desk_anim_slide (scr, cover, 1, 1, steps);
}

void
desk_anim_slideSW (ScreenInfo *scr, Window cover, unsigned int steps)
{
	desk_anim_slide (scr, cover, -1, 1, steps);
}

void
desk_anim_shrinkN (ScreenInfo *scr, Window cover, unsigned int steps)
{
	desk_anim_shrink(scr, cover, 0, -1, steps);
}

void
desk_anim_shrinkNW (ScreenInfo *scr, Window cover, unsigned int steps)
{
	desk_anim_shrink(scr, cover, -1, -1, steps);
}

void
desk_anim_shrinkW (ScreenInfo *scr, Window cover, unsigned int steps)
{
	desk_anim_shrink(scr, cover, -1, 0, steps);
}


/*************************************************************************/
/* main interface                                                        */
/*************************************************************************/

static int    _as_desktop_cover_recursion = 0 ;
static Window _as_desktop_cover = None ;
static GC     _as_desktop_cover_gc = NULL ; 
static XFontStruct *_as_desktop_cover_xfs = NULL ;
static int    _as_progress_line = 0 ;
static int    _as_progress_cursor = 0 ;

#define ANIMATIONS_NUM	13
static void   (*DeskAnimations[ANIMATIONS_NUM]) (ScreenInfo *,Window,unsigned int) =
{
	NULL,
		desk_anim_slideNW,
		desk_anim_slideN,
		desk_anim_slideNE,
		desk_anim_slideE,
		desk_anim_slideSE,
		desk_anim_slideS,
		desk_anim_slideSW,
		desk_anim_slideW,
		desk_anim_shrinkN,
		desk_anim_shrinkNW,
		desk_anim_shrinkW,
		desk_anim_shape_blocks};

void
remove_desktop_cover()
{
    
    if (_as_desktop_cover )
	{
        --_as_desktop_cover_recursion ;
        if(  _as_desktop_cover_recursion == 0 )
        {    
            int steps = Scr.Feel.desk_cover_animation_steps ; 
            int type = Scr.Feel.desk_cover_animation_type ;
            
            XSync (dpy, False);

            if (steps > 0 && type >= 0 && DeskAnimations[type % ANIMATIONS_NUM])
				DeskAnimations[type % ANIMATIONS_NUM] (ASDefaultScr, _as_desktop_cover, steps);
			else
            	XDestroyWindow (dpy, _as_desktop_cover);
            _as_desktop_cover = None ;
            
            _as_progress_line = 0 ;
            _as_progress_cursor = 0 ;

            XSync (dpy, False);
        }
	}
}

Window 
get_desktop_cover_window()
{
    return _as_desktop_cover;    
}    

void 
restack_desktop_cover()
{
    if( _as_desktop_cover )
        XRaiseWindow( dpy, _as_desktop_cover );
}    

void cover_desktop()
{
    XSetWindowAttributes attributes;
	unsigned long valuemask;
	Window        w;
    XGCValues       gcvalue;

    ++_as_desktop_cover_recursion ;

    if( _as_desktop_cover != None ) 
        return ;

	valuemask = (CWBackPixmap | CWBackingStore | CWOverrideRedirect);
	attributes.background_pixmap = None;
	attributes.backing_store = NotUseful;
	attributes.override_redirect = True;

    w = create_visual_window(Scr.asv, Scr.Root, 0, 0,
                               Scr.MyDisplayWidth, Scr.MyDisplayHeight,
                               0, InputOutput, valuemask, &attributes);

	XMapRaised (dpy, w);
    XSync (dpy, False);
    if( _as_desktop_cover_gc == NULL ) 
    {
        unsigned long mask = GCFunction|GCForeground|GCBackground|GCGraphicsExposures ;
		CARD32 r16, g16, b16 ;

        if( _as_desktop_cover_xfs == NULL ) 
            _as_desktop_cover_xfs = XLoadQueryFont( dpy, "fixed" );

        if( _as_desktop_cover_xfs )
        {    
            gcvalue.font = _as_desktop_cover_xfs->fid;
            mask |= GCFont ;
        }
		
		LOCAL_DEBUG_OUT( "desk_anime_tint = %lX", Scr.Look.desktop_animation_tint );
		r16 = ARGB32_RED16(Scr.Look.desktop_animation_tint);
		g16 = ARGB32_GREEN16(Scr.Look.desktop_animation_tint);
		b16 = ARGB32_BLUE16(Scr.Look.desktop_animation_tint);
		if( ASCS_BLACK_O_WHITE_CRITERIA16(r16,g16,b16) )
		{
	        gcvalue.foreground = Scr.asv->black_pixel;
    	    gcvalue.background = Scr.asv->white_pixel;
		}else
		{			  
	        gcvalue.foreground = Scr.asv->white_pixel;
    	    gcvalue.background = Scr.asv->black_pixel;
		}
        gcvalue.function = GXcopy;
        gcvalue.graphics_exposures = 0;
    
        _as_desktop_cover_gc = create_visual_gc( Scr.asv, Scr.Root, mask, &gcvalue);
    }        
    if( _as_desktop_cover_gc )
    {
        unsigned long pixel ;
        ARGB2PIXEL(Scr.asv,Scr.Look.desktop_animation_tint,&pixel);
        gcvalue.foreground = pixel;
        gcvalue.function = GXand;
        XChangeGC( dpy, _as_desktop_cover_gc, GCFunction|GCForeground, &gcvalue );
        XFillRectangle(dpy, w, _as_desktop_cover_gc,
                        0, 0,
                        Scr.MyDisplayWidth, Scr.MyDisplayHeight);            
        gcvalue.foreground = Scr.asv->white_pixel;
        gcvalue.function = GXcopy;
        XChangeGC( dpy, _as_desktop_cover_gc, GCFunction|GCForeground, &gcvalue );
    }
    _as_desktop_cover = w ; 
}

void desktop_cover_cleanup()
{
    if( _as_desktop_cover_gc )
    {
        XFreeGC( dpy, _as_desktop_cover_gc );
        _as_desktop_cover_gc = NULL ;            
    }    

    if( _as_desktop_cover_xfs )
    {    
        XFreeFont( dpy, _as_desktop_cover_xfs );
        _as_desktop_cover_xfs = NULL ;
    }

    if( _as_desktop_cover )
    {    
        XDestroyWindow (dpy, _as_desktop_cover);
        _as_desktop_cover = None ;
    }

    _as_desktop_cover_recursion = 0 ;
}    

static char buffer[8192] ;

void 
display_progress( Bool new_line, const char *msg_format, ... )
{
    if( _as_desktop_cover && _as_desktop_cover_xfs && _as_desktop_cover_gc )
    {
        int x, y ; 
        int height ; 
        int len ; 
        
        va_list ap;
        va_start (ap, msg_format);
        vsnprintf (&buffer[0], 256, msg_format, ap);
        va_end (ap);
        
        len = strlen( &buffer[0] );
        if( _as_progress_cursor > 0 && new_line ) 
        {    
            ++_as_progress_line ;
            _as_progress_cursor = 0 ; 
        }
        /* and now we need to display the text on the screen */
        x = Scr.MyDisplayWidth/20 + _as_progress_cursor ; 
        height = _as_desktop_cover_xfs->ascent + _as_desktop_cover_xfs->descent + 5;
        y = Scr.MyDisplayHeight/5 + height * _as_progress_line ;
        _as_progress_cursor += XTextWidth( _as_desktop_cover_xfs, &buffer[0], len) + 10;
        XDrawString (dpy, _as_desktop_cover, _as_desktop_cover_gc, x, y, &buffer[0], len);
    }        
}    

