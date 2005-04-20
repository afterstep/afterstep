/****************************************************************************
 * Copyright (c) 1999,2001 Sasha Vasko <sasha at aftercode.net>
 * Copyright (c) 1998 Rafal Wierzbicki  (rubber bands) <rafal@mcss.mcmaster.ca>
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
 * Completely rewritten from outline.c copyrighted by :
 *      by Rob Nation
 *      by Bo Yang
 *      by Frank Fejes
 *      by Eric Tremblay
 ****************************************************************************/

#include "../configure.h"

#define LOCAL_DEBUG
#include "asapp.h"
#include "../libAfterImage/afterimage.h"
#include "afterstep.h"
#include "screen.h"
#include "decor.h"
#include "moveresize.h"

#define THICKNESS 3

int
make_rectangle_segments (ASOutlineSegment *s, int x, int y, unsigned int width, unsigned int height)
{
	if( s )
	{
		/* Outline box : */
		s[1].x = s[0].x = x+1;
		s[0].y = y;
		s[1].y = y+height-1;
		s[1].size = s[0].size = width;
		s[1].vertical = s[0].vertical = False ;

		s[2].x = x;
		s[3].x = x+width-1;
		s[3].y = s[2].y = y+1;
		s[3].size = s[2].size = height;
		s[3].vertical = s[2].vertical = True ;
	}
	return 4;
}

int
make_lt_corner_segments (ASOutlineSegment *s, int x, int y, unsigned int width, unsigned int height)
{
	if( s )
	{
		s[0].x = x+1;
		s[0].y = y;
		s[0].size = width;
		s[0].vertical = False ;

		s[1].x = x;
		s[1].y = y+1;
		s[1].size = height;
		s[1].vertical = True ;
	}
	return 2;
}

int
make_br_corner_segments (ASOutlineSegment *s, int x, int y, unsigned int width, unsigned int height)
{
	if( s )
	{
		s[0].x = x-width-1;
		s[0].y = y-THICKNESS;
		s[0].size = width;
		s[0].vertical = False ;

		s[1].x = x-THICKNESS;
		s[1].y = y-height-1;
		s[1].size = height;
		s[1].vertical = True ;
	}
	return 2;
}

int
make_fvwm_segments (ASOutlineSegment *s, struct MRRectangle *geom, unsigned int scr_width, unsigned int scr_height)
{
	int count = 0 ;
	/* Outline box : */
	count += make_rectangle_segments(s, geom->x, geom->y, geom->width, geom->height);
	/* Inside grid : */
	count += make_rectangle_segments(s?&(s[count]):NULL, geom->x+1, geom->y+geom->height/3, geom->width-2, geom->height/3);
	count += make_rectangle_segments(s?&(s[count]):NULL, geom->x+geom->width/3, geom->y+1, geom->width/3, geom->height-2);
	return count;
}

int
make_box_segments (ASOutlineSegment *s, struct MRRectangle *geom, unsigned int scr_width, unsigned int scr_height)
{
	return make_rectangle_segments(s, geom->x, geom->y, geom->width, geom->height);
}

int
make_wmaker_segments (ASOutlineSegment *s, struct MRRectangle *geom, unsigned int scr_width, unsigned int scr_height)
{
	int count = 0 ;
	/* Outline box : */
	count += make_rectangle_segments(s, geom->x, geom->y, geom->width, geom->height);
	if( s )
	{
		s = &(s[count]);

		s[1].x = s[0].x = geom->x+1;
		s[0].y = geom->y+CORNER_WIDTH;
		s[1].y = geom->y+geom->height-BOUNDARY_WIDTH-THICKNESS;
		s[1].size = s[0].size = geom->width-2;
		s[1].vertical = s[0].vertical = False ;
	}
	return count + 2 ;
}

int
make_tek_segments (ASOutlineSegment *s, struct MRRectangle *geom, unsigned int scr_width, unsigned int scr_height)
{
	int count = 0 ;
	count += make_rectangle_segments(s, geom->x, geom->y, geom->width, geom->height);
	count += make_lt_corner_segments(s?&(s[count]):NULL, geom->x-6, geom->y-6, geom->width/2, geom->height/2);
	count += make_br_corner_segments(s?&(s[count]):NULL, geom->x+geom->width+6, geom->y+geom->height+6, geom->width/2, geom->height/2);

	return count;
}

int
make_tek2_segments (ASOutlineSegment *s, struct MRRectangle *geom, unsigned int scr_width, unsigned int scr_height)
{
	int count = 0 ;
	count += make_rectangle_segments(s, geom->x, geom->y, geom->width, geom->height);
	count += make_lt_corner_segments(s?&(s[count]):NULL, geom->x-12, geom->y-12, geom->width/3, geom->height/3);
	count += make_lt_corner_segments(s?&(s[count]):NULL, geom->x-6, geom->y-6, geom->width/2, geom->height/2);
	count += make_br_corner_segments(s?&(s[count]):NULL, geom->x+geom->width+6, geom->y+geom->height+6, geom->width/2, geom->height/2);
	count += make_br_corner_segments(s?&(s[count]):NULL, geom->x+geom->width+12, geom->y+geom->height+12, geom->width/3, geom->height/3);
	return count;
}

int
make_corner_segments(ASOutlineSegment *s, struct MRRectangle *geom, unsigned int scr_width, unsigned int scr_height)
{
	int           corner_height, corner_width;
	int count = 0 ;

	corner_height = geom->height / 8;
	corner_width = geom->width / 8;

	corner_width = FIT_IN_RANGE( 5, corner_width, 32 );
	corner_height = FIT_IN_RANGE( 5, corner_height, 32 );

	count += make_lt_corner_segments(s?&(s[count]):NULL, geom->x, geom->y, corner_width, corner_height);
	count += make_br_corner_segments(s?&(s[count]):NULL, geom->x+geom->width, geom->y+geom->height, corner_width, corner_height);

	/* crosshair */
	if( s )
	{
		s = &(s[count]);

		s[0].x = geom->x + geom->width/2 - 5;
		s[0].y = geom->y + geom->height/2 - 1;
		s[0].vertical = False ;

		s[1].x = geom->x + geom->width/2 - 1;
		s[1].y = geom->y + geom->height/2 - 5;
		s[1].vertical = True ;

		s[1].size = s[0].size = 11;
	}
	return count + 2;
}

int
make_hash_segments (ASOutlineSegment *s, struct MRRectangle *geom, unsigned int scr_width, unsigned int scr_height)
{
	int count = 0 ;
	count += make_rectangle_segments(s, geom->x, geom->y, geom->width, geom->height);
	if( s )
	{
		s = &(s[count]);
		s[1].x = s[0].x = 0 ;
		s[0].y = geom->y-1 ;
		s[1].y = (geom->y+geom->height-THICKNESS)+1;
		s[1].size = s[0].size = scr_width ;
		s[1].vertical = s[0].vertical = False ;

		s[2].x =  geom->x-1 ;
		s[3].x = (geom->x+geom->width-THICKNESS)+1;
		s[3].y = s[2].y = 0 ;
		s[3].size = s[2].size = scr_height ;
		s[3].vertical = s[2].vertical = True ;
	}

	return count + 4;
}

int
make_twm_segments (ASOutlineSegment *s, struct MRRectangle *geom, unsigned int scr_width, unsigned int scr_height)
{
       int count = 0 ;
       /* Outline box : */
       count += make_rectangle_segments(s, geom->x, geom->y, geom->width, geom->height);
       /* Inside grid : */
       if(s) {
               s = &(s[count]);
               s[0].y = geom->y + geom->height/3;
               s[1].y = geom->y + geom->height - geom->height/3;
               s[1].x = s[0].x = geom->x + 1;
               s[1].size = s[0].size = geom->width - 1;
               s[1].vertical = s[0].vertical = False;

               s[2].x = geom->x + geom->width/3;
               s[3].x = geom->x + geom->width - geom->width/3;
               s[3].y = s[2].y = geom->y + 1;
               s[3].size = s[2].size = geom->height - 1;
               s[3].vertical = s[2].vertical = True;
       }
       return count + 4;
}


as_outline_handler outline_handlers[MAX_RUBBER_BAND+1] =
{
    make_fvwm_segments,         /* 0 is the old default FVWM style look                                */
    make_box_segments,          /* 1 is a single rectangle the size of the window.                     */
    make_wmaker_segments,       /* 2 is a WindowMaker style outline, showing the titlebar and handles, if the window has them.*/
    make_tek_segments,          /* 3 is a cool tech-looking outline.                                   */
    make_tek2_segments,         /* 4 is another cool tech-looking outline.                             */
    make_corner_segments,       /* 5 is the corners of the window with a crosshair in the middle.      */
    make_hash_segments,         /* 6 is lines spanning the whole screen framing the window. (CAD style)*/
    make_twm_segments           /* 7 is the old twm style look.                                        */
};

/***********************************************************************
 *	MoveOutline - move a window outline
 ***********************************************************************/

ASOutlineSegment *
make_outline_segments( ASWidget *parent, MyLook *look )
{
	ASOutlineSegment *s ;
	XSetWindowAttributes attr;           /* attributes for create */
	int count = 0, i ;
	ScreenInfo *scr ;
    MRRectangle rdum = {0, 0, 10, 10};

	count = outline_handlers[look->RubberBand%(MAX_RUBBER_BAND+1)](NULL, &rdum, 0, 0 );

LOCAL_DEBUG_OUT( "outline should have %d segments", count );
	if( count <= 0 )
		return NULL;

    s = safecalloc( count+1, sizeof(ASOutlineSegment) );
	scr = AS_WIDGET_SCREEN(parent);
	attr.background_pixel 	= scr->asv->black_pixel ;
	attr.border_pixel 		= scr->asv->white_pixel ;
	attr.save_under 		= True ;
    attr.event_mask         = 0;/*ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
           PointerMotionMask | EnterWindowMask | LeaveWindowMask; */
	for( i = count-1 ; i >= 0 ; --i )
	{
		s[i].w = create_screen_window( AS_WIDGET_SCREEN(parent),
                                       AS_WIDGET_WINDOW(parent),
									   -20, -20, 10, 1, 1,  /* 1 pixel wide border */
		                               InputOutput,
                                       (CWBackPixel|CWBorderPixel|CWSaveUnder|CWEventMask), &attr);
		XMapRaised( dpy, s[i].w );
SHOW_CHECKPOINT;
	}
	return s;
}

void
move_outline( ASMoveResizeData * data )
{

	if( data && data->outline )
	{
        unsigned int mask = CWStackMode ;
		XWindowChanges xwc;
		register int i ;
		int rband = data->look->RubberBand%(MAX_RUBBER_BAND+1);
		ASOutlineSegment *s = data->outline ;
		int count  = outline_handlers[rband]( s, &(data->curr),
											  AS_WIDGET_WIDTH(data->parent),
											  AS_WIDGET_HEIGHT(data->parent) );
		if( count <= 0 )
			return;
		if( (xwc.sibling = data->below_sibling) != None )
		{
			mask |= CWSibling ;
            xwc.stack_mode = Below ;
		}else
			xwc.stack_mode = Above ;
        XConfigureWindow( dpy, data->geometry_display->w, mask, &xwc );
        xwc.sibling = data->geometry_display->w;
        mask |= CWSibling|CWX|CWY|CWWidth|CWHeight ;
        xwc.stack_mode = Below ;
		for( i = 0 ; i < count ; ++i )
		{
			xwc.x = s[i].x;
			xwc.y = s[i].y;
			xwc.width  = s[i].vertical? 1: MAX(s[i].size-2,1);
			xwc.height = s[i].vertical? MAX(s[i].size-2,1): 1;
			XConfigureWindow( dpy, s[i].w, mask, &xwc );
			xwc.sibling = s[i].w ;
        }
		/* hiding the rest of the frame : */
		while( s[i].w != None )
			XMoveResizeWindow( dpy, s[i++].w, -20, -20, -10, -10 );
		XSync( dpy, False );
	}
}

void
destroy_outline_segments( ASOutlineSegment **psegments )
{
	if( psegments == NULL )
		return ;
	if( *psegments )
	{
		register ASOutlineSegment *s = *psegments ;
		register int i = 0 ;
		while( s[i].w != None )
			XDestroyWindow( dpy, s[i++].w );
		XSync( dpy, False );
		free( s );
		*psegments = NULL ;
	}
}
