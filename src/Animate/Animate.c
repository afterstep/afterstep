/*
 * Copyright (c) 1997 Frank Scheelen <scheelen@worldonline.nl>
 * Copyright (c) 1996 Alfredo Kengi Kojima (kojima@inf.ufrgs.br)
 * Copyright (c) 1996 Kaj Groner <kajg@mindspring.com>
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
 */

#define LOCAL_DEBUG
#define EVENT_TRACE


#include "../../configure.h"
#include "../../libAfterStep/asapp.h"
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/aswindata.h"
#include "../../libAfterStep/event.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterConf/afterconf.h"
#include <math.h>

#define AS_PI 3.14159265358979323846
#define ANIM_DELAY2     20

/* masks for AS pipe */
#define mask_lock_on_send	M_STATUS_CHANGE
#define mask_reg 			(WINDOW_PACKET_MASK|M_NEW_DESKVIEWPORT)
#define mask_off 0

AnimateConfig *Config = NULL ;

struct ASAnimateParams;
typedef Bool (*animate_iter_func)( struct ASAnimateParams *params );

typedef struct ASAnimateParams
{
	animate_iter_func iter_func ;

	int x, y, w, h ;
	int fx, fy, fw, fh ;
	float cx, cy, cw, ch;
	float xstep, ystep, wstep, hstep;
	float angle, final_angle;
	int iter ;
}ASAnimateParams;


void CheckConfigSanity();
void GetBaseOptions (const char *filename/* unused*/);
void GetOptions (const char *filename);
void HandleEvents();
void DispatchEvent (ASEvent * event);
void process_message (send_data_type type, send_data_type *body);

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
static void 
animate_points( XPoint *points, int count ) 
{
	LOCAL_DEBUG_OUT( "Drawing %d points:", count );	
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
	{
		int i ;
		for( i = 0 ; i < count ; ++i ) 
			fprintf(stderr, "\t\t%d: %+d%+d\n", i, points[i].x, points[i].y );	
	}	 
#endif
	XDrawLines (dpy, Scr.Root, Scr.DrawGC, points, 5, CoordModeOrigin);
	ASSync(False);
	sleep_a_millisec (Config->delay);
	XDrawLines (dpy, Scr.Root, Scr.DrawGC, points, 5, CoordModeOrigin);
}

void 
draw_animation_rectangle( int x, int y, int width, int height ) 
{
	LOCAL_DEBUG_OUT( "Drawing rectangle %dx%d%+d%+d", width, height, x, y );		
	XDrawRectangle (dpy, Scr.Root, Scr.DrawGC, x, y, width, height);	
}

/*
 * This makes a twisty iconify/deiconify animation for a window, similar to
 * MacOS.  Parameters specify the position and the size of the initial
 * window and the final window
 */
Bool
AnimateResizeTwist (ASAnimateParams *params)
{
    XPoint points[5];
	float a = (params->cw == 0)?0:atan (params->ch / params->cw);
    float d = sqrt ((params->cw / 2) * (params->cw / 2) + (params->ch / 2) * (params->ch / 2));

	points[0].x = params->cx + cos (params->angle - a) * d;
    points[0].y = params->cy + sin (params->angle - a) * d;
    points[1].x = params->cx + cos (params->angle + a) * d;
    points[1].y = params->cy + sin (params->angle + a) * d;
    points[2].x = params->cx + cos (params->angle - a + AS_PI) * d;
    points[2].y = params->cy + sin (params->angle - a + AS_PI) * d;
    points[3].x = params->cx + cos (params->angle + a + AS_PI) * d;
    points[3].y = params->cy + sin (params->angle + a + AS_PI) * d;
    points[4].x = params->cx + cos (params->angle - a) * d;
    points[4].y = params->cy + sin (params->angle - a) * d;
      
	animate_points( points, 5 );
	return False;
}

 /*
  * Add even more 3D feel to AfterStep by doing a flipping iconify.
  * Parameters specify the position and the size of the initial and the
  * final window.
  *
  * Idea: how about texture mapped, user definable free 3D movement
  * during a resize? That should get X on its knees all right! :)
  */
Bool
AnimateResizeFlip (ASAnimateParams *params)
{
	XPoint points[5];
	float distortx = (params->cw / 10) - ((params->cw / 5) * sin (params->angle));
	float distortch = (params->ch / 2) * cos (params->angle);
	float midy = params->cy + (params->ch / 2);

	points[0].x = params->cx + distortx;
	points[0].y = midy - distortch;
	points[1].x = params->cx + params->cw - distortx;
	points[1].y = points[0].y;
	points[2].x = params->cx + params->cw + distortx;
	points[2].y = midy + distortch;
	points[3].x = params->cx - distortx;
	points[3].y = points[2].y;
	points[4].x = points[0].x;
	points[4].y = points[0].y;

	animate_points( points, 5 );
	return False;
}


 /*
  * And another one, this time around the Y-axis.
  */
Bool
AnimateResizeTurn (ASAnimateParams *params)
{
	XPoint points[5];
    float distorty = (params->ch / 10) - ((params->ch / 5) * sin (params->angle));
    float distortcw = (params->cw / 2) * cos (params->angle);
    float midx = params->cx + (params->cw / 2);

    points[0].x = midx - distortcw;
    points[0].y = params->cy + distorty;
    points[1].x = midx + distortcw;
    points[1].y = params->cy - distorty;
    points[2].x = points[1].x;
    points[2].y = params->cy + params->ch + distorty;
    points[3].x = points[0].x;
    points[3].y = params->cy + params->ch - distorty;
    points[4].x = points[0].x;
    points[4].y = points[0].y;

    animate_points( points, 5 );
	return False;
}


/*
 * This makes a zooming iconify/deiconify animation for a window, like most
 * any other icon animation out there.  Parameters specify the position and
 * the size of the initial window and the final window
 */
Bool
AnimateResizeZoom (ASAnimateParams *params)
{
	draw_animation_rectangle( (int) params->cx, (int) params->cy, (int) params->cw, (int) params->ch);
    ASSync(False);
    sleep_a_millisec ((Config->delay/10)+1);
    draw_animation_rectangle( (int) params->cx, (int) params->cy, (int) params->cw, (int) params->ch);
	return False;
}

/*
 * The effect of this is similar to AnimateResizeZoom but this time we
 * add lines to create a 3D effect.  The gotcha is that we have to do
 * something different depending on the direction we are zooming in.
 *
 * Andy Parker <parker_andy@hotmail.com>
 */
Bool
AnimateResizeZoom3D (ASAnimateParams *params)
{
	draw_animation_rectangle( (int) params->cx, (int) params->cy, (int) params->cw,  (int) params->ch);
	draw_animation_rectangle( params->x, params->y, params->w, params->h);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, (int) params->cx, (int) params->cy, params->x, params->y);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, ((int) params->cx + (int) params->cw), (int) params->cy, (params->x + params->w), params->y);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, ((int) params->cx + (int) params->cw), ((int) params->cy +(int) params->ch), (params->x + params->w), (params->y + params->h));
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, (int) params->cx, ((int) params->cy + (int) params->ch), params->x, (params->y + params->h));
	ASSync(False);
	sleep_a_millisec (Config->delay);
	draw_animation_rectangle( (int) params->cx, (int) params->cy, (int) params->cw, (int) params->ch);
	draw_animation_rectangle( params->x, params->y, params->w, params->h);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, (int) params->cx, (int) params->cy, params->x, params->y);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, ((int) params->cx + (int) params->cw), (int) params->cy, (params->x + params->w), params->y);
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, ((int) params->cx + (int) params->cw), ((int) params->cy + (int) params->ch), (params->x + params->w), (params->y + params->h));
	XDrawLine (dpy, Scr.Root, Scr.DrawGC, (int) params->cx, ((int) params->cy + (int) params->ch), params->x, (params->y + params->h));
	return False;
}

static Bool
prepare_animate_params( ASAnimateParams *params, ASRectangle *from, ASRectangle *to )
{
	AnimateResizeType type = Config->resize ;
	int iterations = Config->iterations ; 

	if( type == ART_None )
		return False;
	if( type == ART_Random ) 
		type = (rand () + (from->x * from->y + from->width * from->height + to->x)) % ART_Random;
	switch (type)
    {
    	case ART_Twist:	params->iter_func = AnimateResizeTwist;	break;
    	case ART_Flip: params->iter_func = AnimateResizeFlip ;     break;
    	case ART_Turn: params->iter_func = AnimateResizeTurn ;     break;
    	case ART_Zoom: params->iter_func = AnimateResizeZoom ;     break;
    	default: params->iter_func = AnimateResizeZoom3D ;  break;
    }		
	params->x = from->x ;
	params->y = from->y ;
	params->w = from->width ;
	params->h = from->height ;
	params->fx = to->x ;
	params->fy = to->y ;
	params->fw = to->width ;
	params->fh = to->height ;

	if( type == ART_Twist ) 
	{	
		params->x += params->w / 2;
		params->y += params->h / 2;
		params->fx += params->fw / 2;
		params->fy += params->fh / 2;
	}	
	if( iterations == 0 ) 
		iterations = ANIMATE_DEFAULT_ITERATIONS ;
  	params->xstep = (float) (params->fx - params->x) / iterations;
  	params->ystep = (float) (params->fy - params->y) / iterations;
  	params->wstep = (float) (params->fw - params->w) / iterations;
  	params->hstep = (float) (params->fh - params->h) / iterations;

  	params->cx = (float) params->x;
  	params->cy = (float) params->y;
  	params->cw = (float) params->w;
	params->ch = (float) params->h;

	if( type == ART_Zoom3D && to->width + to->height <= from->width + from->height)
	{
		params->x = params->fx ;
		params->y = params->fy ;
		params->w = params->fw ;
		params->h = params->fh ;	   
	}	 

	params->final_angle = 2 * AS_PI * Config->twist;
  	params->angle = 0;
	params->iter = 0 ;
	return True;
}

void 
do_animate( ASRectangle *from, ASRectangle *to ) 
{
	ASAnimateParams params ;
	Bool do_brake = False ;
	int iterations = Config->iterations ; 
	LOCAL_DEBUG_OUT( "preapring params to animate from %ldx%ld%+ld%+ld to %ldx%ld%+ld%+ld", 
					 from->width, from->height, from->x, from->y,
					 to->width, to->height, to->x, to->y );
	if( !prepare_animate_params( &params, from, to ) )
		return;
	if( iterations == 0 ) 
		iterations = ANIMATE_DEFAULT_ITERATIONS ;

	grab_server();
	do
	{
		do_brake = params.iter_func( &params );
					   
		++params.iter ;
	  	params.cx += params.xstep;
      	params.cy += params.ystep;
      	params.cw += params.wstep;
      	params.ch += params.hstep;
	  	params.angle += (float) (2 * AS_PI * Config->twist / iterations);

		LOCAL_DEBUG_OUT( "iter = %d of %d, angle = %f, final = %f", params.iter, iterations, params.angle, params.final_angle );
	}while( !do_brake && params.iter < iterations && params.angle < params.final_angle );
	ASFlush ();
	ungrab_server();
}	  

#if 0
/*
 * This makes an animation that looks like that light effect
 * when you turn off an old TV.
 * Used for window destruction
 */
void
AnimateClose (int x, int y, int w, int h)
{
  int i, step;

  if (h > 4)
    {
      step = h * 4 / Animate.iterations;
      if (step == 0)
	{
	  step = 2;
	}
      for (i = h; i >= 2; i -= step)
	{
	  XDrawRectangle (dpy, Scr.Root, DrawGC, x, y, w, i);
	  XFlush (dpy);
	  sleep_a_millisec (ANIM_DELAY2);
	  XDrawRectangle (dpy, Scr.Root, DrawGC, x, y, w, i);
	  y += step / 2;
	}
    }
  if (w < 2)
    return;
  step = w * 4 / Animate.iterations;
  if (step == 0)
    {
      step = 2;
    }
  for (i = w; i >= 0; i -= step)
    {
      XDrawRectangle (dpy, Scr.Root, DrawGC, x, y, i, 2);
      XFlush (dpy);
      sleep_a_little (ANIM_DELAY2 * 1000);
      XDrawRectangle (dpy, Scr.Root, DrawGC, x, y, i, 2);
      x += step / 2;
    }
  sleep_a_little (100000);
  XFlush (dpy);
}
#endif

void
DeadPipe (int foo)
{
	{
		static int already_dead = False ; 
		if( already_dead ) 	return;/* non-reentrant function ! */
		already_dead = True ;
	}
    if( Config )
        DestroyAnimateConfig (Config);

    FreeMyAppResources();
#ifdef DEBUG_ALLOCS
    print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

    XFlush (dpy);
    XCloseDisplay (dpy);
    exit (0);
}

int
main (int argc, char **argv)
{
	/* Save our program name - for error messages */
	set_DeadPipe_handler(DeadPipe);
    InitMyApp (CLASS_ANIMATE, argc, argv, NULL, NULL, 0 );

	LinkAfterStepConfig();

    ConnectX( ASDefaultScr, 0 );
    ConnectAfterStep ( mask_reg, mask_lock_on_send );
	
	Config = CreateAnimateConfig();
	
	LOCAL_DEBUG_OUT("parsing Options ...%s","");
    LoadBaseConfig (GetBaseOptions);
	LoadColorScheme();
    LoadConfig ("animate", GetOptions);

    CheckConfigSanity();
  
  	SendInfo ( "Nop \"\"", 0);
    
	Scr.Vx = Scr.wmprops->as_current_vx ; 
	Scr.Vy = Scr.wmprops->as_current_vy ;
	LOCAL_DEBUG_OUT("Viewport is %+d%+d. Starting The Loop ...",Scr.Vx, Scr.Vy);
    HandleEvents();

    return 0;
}

void HandleEvents()
{
    ASEvent event;
    Bool has_x_events = False ;
    while (True)
    {
        while((has_x_events = XPending (dpy)) )
        {
            if( ASNextEvent (&(event.x), True) )
            {
                event.client = NULL ;
                setup_asevent_from_xevent( &event );
                DispatchEvent( &event );
			}
        }
        module_wait_pipes_input ( process_message );
    }
}
/*************************************************************************/
/* Config stuff : */
/*************************************************************************/
void CheckConfigSanity()
{
	ARGB32 argb = ARGB32_White ;
	long    pixel = Scr.asv->white_pixel ; 
	XGCValues gcv;

	if( Config->color ) 
		if( parse_argb_color( Config->color, &argb ) != Config->color ) 
			ARGB2PIXEL(Scr.asv,argb,&pixel);

	if( Config->iterations == 0 ) 
		Config->iterations = ANIMATE_DEFAULT_ITERATIONS ;
	if( Config->twist == 0 ) 
		Config->twist = 1 ;
	if( Config->delay == 0 ) 
		Config->delay = 1 ;
		
	/* Config->resize = ART_Zoom ; */
		
	gcv.line_width = Config->width;
  	gcv.function = GXxor;
  	gcv.foreground = pixel;
  	gcv.background = pixel;
  	gcv.subwindow_mode = IncludeInferiors;
	if( Scr.DrawGC == NULL ) 
	{	
		Scr.DrawGC = XCreateGC (dpy, Scr.Root, 
								GCFunction|GCForeground|GCLineWidth|GCBackground|GCSubwindowMode,
								&gcv);
	}else			
	{
		XChangeGC(	dpy, Scr.DrawGC, 
					GCFunction|GCForeground|GCLineWidth|GCBackground|GCSubwindowMode,
					&gcv);	
	}	 
}

void
GetBaseOptions (const char *filename/* unused*/)
{
    START_TIME(started);
	ReloadASEnvironment( NULL, NULL, NULL, False, False );
    SHOW_TIME("BaseConfigParsingTime",started);
}

void
GetOptions (const char *filename)
{
    AnimateConfig *config = ParseAnimateOptions (filename, MyName);
    START_TIME(option_time);

    /* Need to merge new config with what we have already :*/
    /* now lets check the config sanity : */
    /* mixing set and default flags : */
    Config->set_flags |= config->set_flags;

  	if( config->color != NULL ) 
		set_string_value( &(Config->color), config->color, NULL, 0 );
    if( get_flags(config->set_flags, ANIMATE_SET_DELAY) )
        Config->delay = config->delay;
    if( get_flags(config->set_flags, ANIMATE_SET_ITERATIONS) )
        Config->iterations = config->iterations;
    if( get_flags(config->set_flags, ANIMATE_SET_TWIST) )
        Config->twist = config->twist;
    if( get_flags(config->set_flags, ANIMATE_SET_WIDTH) )
        Config->width = config->width;
    if( get_flags(config->set_flags, ANIMATE_SET_RESIZE) )
        Config->resize = config->resize;

    DestroyAnimateConfig (config);
    SHOW_TIME("Config parsing",option_time);
}
/*************************************************************************/
/* event/message processing : 											 */
/*************************************************************************/
void
DispatchEvent (ASEvent * event)
{
    SHOW_EVENT_TRACE(event);
    switch (event->x.type)
    {
	    case ClientMessage:
            LOCAL_DEBUG_OUT("ClientMessage(\"%s\",data=(%lX,%lX,%lX,%lX,%lX)", XGetAtomName( dpy, event->x.xclient.message_type ), event->x.xclient.data.l[0], event->x.xclient.data.l[1], event->x.xclient.data.l[2], event->x.xclient.data.l[3], event->x.xclient.data.l[4]);
            if ( event->x.xclient.format == 32 &&
                 event->x.xclient.data.l[0] == _XA_WM_DELETE_WINDOW )
			{
                DeadPipe(0);
            }
			return ;
	    case PropertyNotify:
			handle_wmprop_event (Scr.wmprops, &(event->x));
			return ;
        default:
            return;
    }
}

void
process_message (send_data_type type, send_data_type *body)
{
    LOCAL_DEBUG_OUT( "received message %lX", type );
	
	if( (type&WINDOW_PACKET_MASK) != 0 )
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );
		WindowPacketResult res ;
        /* saving relevant client info since handle_window_packet could destroy the actuall structure */
		ASFlagType old_state = wd?wd->state_flags:0 ; 
		ASRectangle saved_icon_rect = {0, 0, 0, 0}; 
		if( get_flags(old_state, AS_Iconic) ) 
		{
		 	saved_icon_rect = wd->icon_rect ;
		}				   
        show_activity( "message %lX window %X data %p", type, body[0], wd );
		res = handle_window_packet( type, body, &wd );
		LOCAL_DEBUG_OUT( "result = %d", res );
        if( res == WP_DataCreated )
		{	
		}else if( res == WP_DataChanged )
		{	
			LOCAL_DEBUG_OUT( "old_flags = %lX, new_flags = %lX", old_state, wd->state_flags );
			/* lets see if we got iconified or deiconified : */
			if( get_flags(old_state, AS_Iconic) != get_flags(wd->state_flags, AS_Iconic) )
			{
				ASRectangle from, to ;

				if( !get_flags( wd->state_flags, AS_Iconic)	)
				{                              	/* deiconified :  */
					from = saved_icon_rect ;
					to = wd->frame_rect ;
					if( !get_flags( wd->state_flags, AS_Sticky ) )
					{	
						to.x -= Scr.Vx ;
						to.y -= Scr.Vy ;
					}
				}else							/* iconified : */
				{
					from = wd->frame_rect ;
					if( !get_flags( wd->state_flags, AS_Sticky ) )
					{
						from.x -= Scr.Vx ;
						from.y -= Scr.Vy ;
					}
					to = wd->icon_rect ;
				}	 
				do_animate( &from, &to );
			}	 
		}else if( res == WP_DataDeleted )
        {
        }
    }else if( type == M_NEW_DESKVIEWPORT )
    {
		LOCAL_DEBUG_OUT("M_NEW_DESKVIEWPORT(desk = %ld,Vx=%ld,Vy=%ld)", body[2], body[0], body[1]);
		Scr.Vx = body[0] ;
		Scr.Vy = body[1] ;
		Scr.CurrentDesk = body[2] ;
	}

	if( type&M_STATUS_CHANGE  )
		SendInfo ("UNLOCK 1\n", 0);
	sleep_a_millisec(10);
}
/*************************************************************************/

