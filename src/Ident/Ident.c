/*
 * Copyright (c) 2004 Sasha Vasko <sasha@aftercode.net>
 * Copyright (c) 1999 Rafal Wierzbicki <rafal@mcss.mcmaster.ca>
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (c) 1994 Nobutaka Suzuki
 * Copyright (c) 1994 Robert Nation
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

/*#define DO_CLOCKING      */
#define LOCAL_DEBUG
#define EVENT_TRACE

#include "../../configure.h"
#include "../../libAfterStep/asapp.h"

#include <signal.h>
#include <unistd.h>

#include "../../libAfterImage/afterimage.h"

#include "../../libAfterStep/afterstep.h"
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/mystyle.h"
#include "../../libAfterStep/mystyle_property.h"
#include "../../libAfterStep/parser.h"
#include "../../libAfterStep/clientprops.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterStep/decor.h"
#include "../../libAfterStep/aswindata.h"
#include "../../libAfterStep/balloon.h"
#include "../../libAfterStep/event.h"

#include "../../libAfterConf/afterconf.h"

/**********************************************************************/
/*  AfterStep specific global variables :                             */
/**********************************************************************/
/**********************************************************************/
/*  WinList local variables :                                         */
/**********************************************************************/
typedef struct {
    Window       main_window;
    ASCanvas    *main_canvas;
}ASIdentState ;

ASIdentState IdentState = {};

IdentConfig *Config = NULL ;
/**********************************************************************/
Window get_target_window();


int
main( int argc, char **argv )
{
    /* Save our program name - for error messages */
    InitMyApp (CLASS_IDENT, argc, argv, NULL, NULL, 0 );

    set_signal_handler( SIGSEGV );

    ConnectX( &Scr, 0 );
    ConnectAfterStep (WINDOW_CONFIG_MASK |
                      WINDOW_NAME_MASK |
                      M_END_WINDOWLIST);
    Config = CreateIdentConfig ();

    /* Request a list of all windows, while we load our config */
    SendInfo ("Send_WindowList", 0);

    LoadBaseConfig ( GetBaseOptions);
	LoadColorScheme();
	LoadConfig ("ident", GetOptions);
    CheckConfigSanity();

	if (MyArgs.src_window == 0)
		MyArgs.src_window = get_target_window();

    IdentState.main_window = make_ident_window();
    IdentState.main_canvas = create_ascanvas( IdentState.main_window );
    set_root_clip_area( IdentState.main_canvas );
    
	/* And at long last our main loop : */
    HandleEvents();
	return 0 ;
}

void HandleEvents()
{
    ASEvent event;
    Bool has_x_events = False ;
    while (True)
    {
        while((has_x_events = XPending (dpy)))
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

void
DeadPipe (int nonsense)
{
	static int already_dead = False ; 
	if( already_dead ) 
		return;/* non-reentrant function ! */
	already_dead = True ;
    
	window_data_cleanup();

    if( WinListState.main_canvas )
        destroy_ascanvas( &WinListState.main_canvas );
    if( WinListState.main_window )
        XDestroyWindow( dpy, WinListState.main_window );
    
	FreeMyAppResources();
    
	if( Config )
        DestroyIdentConfig(Config);

#ifdef DEBUG_ALLOCS
    print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

    XFlush (dpy);			/* need this for SetErootPixmap to take effect */
	XCloseDisplay (dpy);		/* need this for SetErootPixmap to take effect */
    exit (0);
}

void
CheckConfigSanity()
{
    char buf[256];

    if( Config == NULL )
        Config = CreateIdentConfig ();

    mystyle_get_property (Scr.wmprops);

    sprintf( buf, "*%sTile", get_application_name() );
    LOCAL_DEBUG_OUT("Attempting to use style \"%s\"", buf);
    Scr.Look.MSWindow[BACK_UNFOCUSED] = mystyle_find_or_default( buf );
    LOCAL_DEBUG_OUT("Will use style \"%s\"", Scr.Look.MSWindow[BACK_UNFOCUSED]->name);
}


void
GetBaseOptions (const char *filename)
{
	ReloadASEnvironment( NULL, NULL, NULL, False );
}

void
GetOptions (const char *filename)
{
    int i ;
    START_TIME(option_time);
    IdentConfig *config = ParseIdentOptions( filename, MyName );

    if (config->style_defs)
        ProcessMyStyleDefinitions (&(config->style_defs));
    SHOW_TIME("Config parsing",option_time);
}
/****************************************************************************/
/* PROCESSING OF AFTERSTEP MESSAGES :                                       */
/****************************************************************************/
void
process_message (send_data_type type, send_data_type *body)
{
    LOCAL_DEBUG_OUT( "received message %lX", type );
	if( type == M_END_WINDOWLIST )
	{
	}else if( (type&WINDOW_PACKET_MASK) != 0 )
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );
        struct ASWindowData *saved_wd = wd ;
        ASTBarData *tbar = wd?wd->bar:NULL;
        WindowPacketResult res ;

		show_progress( "message %X window %X data %p", type, body[0], wd );
		res = handle_window_packet( type, body, &wd );
		if( res == WP_DataCreated )
        {
        }else if( res == WP_DataChanged )
		{	
		}else if( res == WP_DataDeleted )
		{	
		}
	}
}

void
DispatchEvent (ASEvent * event)
{
    ASWindowData *pointer_wd = NULL ;

    SHOW_EVENT_TRACE(event);

    switch (event->x.type)
    {
	    case ConfigureNotify:
            {
                ASFlagType changes = handle_canvas_config( IdentState.main_canvas );
                if( changes != 0 )
                    set_root_clip_area( IdentState.main_canvas );

                if( get_flags( changes, CANVAS_RESIZED ) )
				{	
                
				}else if( changes != 0 )        /* moved - update transparency ! */
                {
					
                   /*  int i ;
                    	for( i = 0 ; i < Ident.windows_num ; ++i )
                        	update_astbar_transparency( WinListState.window_order[i]->bar, WinListState.main_canvas, False );
					*/
                }
            }
	        break;
        case ButtonPress:
            break;
        case ButtonRelease:
			break;
        case EnterNotify :
        case LeaveNotify :
        case MotionNotify :
            break ;
	    case ClientMessage:
            if ((event->x.xclient.format == 32) &&
                (event->x.xclient.data.l[0] == _XA_WM_DELETE_WINDOW))
			{
                DeadPipe(0);
			}
	        break;
	    case PropertyNotify:
			LOCAL_DEBUG_OUT( "property %s(%lX), _XROOTPMAP_ID = %lX, event->w = %lX, root = %lX", XGetAtomName(dpy, event->x.xproperty.atom), event->x.xproperty.atom, _XROOTPMAP_ID, event->w, Scr.Root );
            if( event->x.xproperty.atom == _XROOTPMAP_ID && event->w == Scr.Root )
            {
                int i ;
                LOCAL_DEBUG_OUT( "root background updated!%s","");
                safe_asimage_destroy( Scr.RootImage );
                Scr.RootImage = NULL ;
				/*
                for( i = 0 ; i < WinListState.windows_num ; ++i )
                    if( update_astbar_transparency( WinListState.window_order[i]->bar, WinListState.main_canvas, True ) )
						render_astbar( WinListState.window_order[i]->bar, WinListState.main_canvas );
				 */
		        if( is_canvas_dirty( IdentState.main_canvas ) )
				{
            		update_canvas_display( IdentState.main_canvas );
					update_canvas_display_mask (IdentState.main_canvas, True);
				}
            }else if( event->x.xproperty.atom == _AS_STYLE )
			{
                int i ;
				LOCAL_DEBUG_OUT( "AS Styles updated!%s","");
				handle_wmprop_event (Scr.wmprops, &(event->x));
				mystyle_list_destroy_all(&(Scr.Look.styles_list));
				LoadColorScheme();
				CheckConfigSanity();
				/* now we need to update everything */
			}
			break;
    }
}
/**********************************************************************
 * If no application window was indicated on the command line, prompt
 * the user to select one
 *********************************************************************/
Window
get_target_window ()
{
	XEvent eventp;
	int val = -10, trials;
	Window target = None;

	trials = 0;
	while ((trials < 100) && (val != GrabSuccess))
    {
		val = XGrabPointer (dpy, Scr.Root, True,
				  		   	ButtonReleaseMask,
			  				GrabModeAsync, GrabModeAsync, Scr.Root,
			  				XCreateFontCursor (dpy, XC_crosshair),
			  				CurrentTime);
      	if( val != GrabSuccess )
			sleep_a_little (100);
		trials++;
    }
  	if (val != GrabSuccess)
    {
    	show_error( "Couldn't grab the cursor!\n", MyName);
      	DeadPipe();
    }
  	XMaskEvent (dpy, ButtonReleaseMask, &eventp);
  	XUngrabPointer (dpy, CurrentTime);
  	ASSync(0);
  	target = eventp.xany.window;
  	if( eventp.xbutton.subwindow != None )
    	target = eventp.xbutton.subwindow;
	return target;
}


/*************************************************************************
 *
 * End of window list, open an x window and display data in it
 *
 ************************************************************************/
XSizeHints mysizehints;
void
list_end (void)
{
  XGCValues gcv;
  unsigned long gcm;
  int lmax, height;
  XEvent Event;
  Window JunkRoot, JunkChild;
  int JunkX, JunkY;
  unsigned int JunkMask;
  int x, y;

  if (!found)
    {
/*    fprintf(stderr,"%s: Couldn't find app window\n", MyName); */
      exit (0);
    }
  close (fd[0]);
  close (fd[1]);

  target.gnome_enabled = gnome_hints (app_win);

  /* load the font */
  if (!load_font (font_string, &font))
    {
      exit (1);
    }

  /* make window infomation list */
  MakeList ();

  /* size and create the window */
  lmax = max_col1 + max_col2 + 22;

  height = 22 * font.height;

  mysizehints.flags =
    USSize | USPosition | PWinGravity | PResizeInc | PBaseSize | PMinSize | PMaxSize;
  /* subtract one for the right/bottom border */
  mysizehints.width = lmax + 10;
  mysizehints.height = height + 10;
  mysizehints.width_inc = 1;
  mysizehints.height_inc = 1;
  mysizehints.base_height = mysizehints.height;
  mysizehints.base_width = mysizehints.width;
  mysizehints.min_height = mysizehints.height;
  mysizehints.min_width = mysizehints.width;
  mysizehints.max_height = mysizehints.height;
  mysizehints.max_width = mysizehints.width;
  XQueryPointer (dpy, Scr.Root, &JunkRoot, &JunkChild,
		 &x, &y, &JunkX, &JunkY, &JunkMask);
  mysizehints.win_gravity = NorthWestGravity;

  if ((y + height + 100) > Scr.MyDisplayHeight)
    {
      y = Scr.MyDisplayHeight - 2 - height - 10;
      mysizehints.win_gravity = SouthWestGravity;
    }
  if ((x + lmax + 100) > Scr.MyDisplayWidth)
    {
      x = Scr.MyDisplayWidth - 2 - lmax - 10;
      if ((y + height + 100) > Scr.MyDisplayHeight)
	mysizehints.win_gravity = SouthEastGravity;
      else
	mysizehints.win_gravity = NorthEastGravity;
    }
  mysizehints.x = x;
  mysizehints.y = y;


#define BW 1
  if (Scr.d_depth < 2)
    {
      back_pix = Scr.asv->black_pixel ;
      fore_pix = Scr.asv->white_pixel ;
    }
  else
    {
		ARGB32 color = ARGB32_Black;
		parse_argb_color( BackColor, &color );
		ARGB2PIXEL(Scr.asv, color, &back_pix );
		color = ARGB32_White;
		parse_argb_color( ForeColor, &color );
		ARGB2PIXEL(Scr.asv, color, &fore_pix );
    }

  main_win = create_visual_window(Scr.asv, Scr.Root, mysizehints.x, mysizehints.y,
				  mysizehints.width, mysizehints.height,
				  BW, InputOutput, 0, NULL);
  XSetWindowBackground( dpy, main_win, back_pix );
  XSetTransientForHint (dpy, main_win, app_win);
  wm_del_win = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
  XSetWMProtocols (dpy, main_win, &wm_del_win, 1);

  XSetWMNormalHints (dpy, main_win, &mysizehints);
  XSelectInput (dpy, main_win, MW_EVENTS);
  change_window_name (MyName);

  gcm = GCForeground | GCBackground | GCFont;
  gcv.foreground = fore_pix;
  gcv.background = back_pix;
  gcv.font = font.font->fid;
  NormalGC = create_visual_gc(Scr.asv, Scr.Root, gcm, &gcv);
  XMapWindow (dpy, main_win);

  /* Window is created. Display it until the user clicks or deletes it. */
  while (1)
    {
      XNextEvent (dpy, &Event);
      switch (Event.type)
	{
	case Expose:
	  if (Event.xexpose.count == 0)
	    RedrawWindow ();
	  break;
	case KeyRelease:
	case ButtonRelease:
	  freelist ();
	  exit (0);
	case ClientMessage:
	  if (Event.xclient.format == 32 && Event.xclient.data.l[0] == wm_del_win)
	    {
	      freelist ();
	      exit (0);
	    }
	default:
	  break;
	}
    }

}




/**************************************************************************
*
* Add s1(string at first column) and s2(string at second column) to itemlist
*
 *************************************************************************/
void
AddToList (char *s1, char *s2)
{
  int tw1, tw2;
  struct Item *item, *cur = itemlistRoot;

  tw1 = XTextWidth (font.font, s1, strlen (s1));
  tw2 = XTextWidth (font.font, s2, strlen (s2));
  max_col1 = max_col1 > tw1 ? max_col1 : tw1;
  max_col2 = max_col2 > tw2 ? max_col2 : tw2;

  item = (struct Item *) safemalloc (sizeof (struct Item));

  item->col1 = s1;
  item->col2 = s2;
  item->next = NULL;

  if (cur == NULL)
    itemlistRoot = item;
  else
    {
      while (cur->next != NULL)
	cur = cur->next;
      cur->next = item;
    }
}

void
MakeList (void)
{
  int bw, width, height, x1, y1, x2, y2;
  char loc[20];

  bw = 2 * target.border_w;
  width = target.frame_w - bw;
  height = target.frame_h - target.title_h - bw;

  sprintf (desktop, "%ld", target.desktop);
  sprintf (id, "0x%x", (unsigned int) target.id);
  sprintf (swidth, "%d", width);
  sprintf (sheight, "%d", height);
  sprintf (borderw, "%ld", target.border_w);

  AddToList ("Name:", target.name);
  AddToList ("Icon Name:", target.icon_name);
  AddToList ("Class:", target.class);
  AddToList ("Resource:", target.res);
  AddToList ("Window ID:", id);
  AddToList ("Desk:", desktop);
  AddToList ("Width:", swidth);
  AddToList ("Height:", sheight);
  AddToList ("BoundaryWidth:", borderw);
  AddToList ("Sticky:", (target.flags & AS_Sticky ? YES : NO));
/*  AddToList ("Ontop:", (target.flags & ONTOP ? YES : NO)); */
  AddToList ("NoTitle:", (target.flags & AS_Titlebar ? NO : NO));
  AddToList ("Iconified:", (target.flags & AS_Icon ? YES : NO));
  AddToList ("AvoidCover:", (target.flags & AS_AvoidCover ? YES : NO));
/*  AddToList ("SkipFocus:", (target.flags & NOFOCUS ? YES : NO)); */
  AddToList ("Shaded:", (target.flags & AS_Shaded ? YES : NO));
  AddToList ("Maximized:", (target.flags & AS_MaxSize ? YES : NO));
  AddToList ("WindowListSkip:", (target.flags & AS_SkipWinList ? YES : NO));
  AddToList ("CirculateSkip:", (target.flags & AS_DontCirculate ? YES : NO));
  AddToList ("Transient:", (target.flags & AS_Transient ? YES : NO));
  AddToList ("GNOME Enabled:", ((target.gnome_enabled == False) ? NO : YES));

  switch (target.gravity)
    {
    case ForgetGravity:
      AddToList ("Gravity:", "Forget");
      break;
    case NorthWestGravity:
      AddToList ("Gravity:", "NorthWest");
      break;
    case NorthGravity:
      AddToList ("Gravity:", "North");
      break;
    case NorthEastGravity:
      AddToList ("Gravity:", "NorthEast");
      break;
    case WestGravity:
      AddToList ("Gravity:", "West");
      break;
    case CenterGravity:
      AddToList ("Gravity:", "Center");
      break;
    case EastGravity:
      AddToList ("Gravity:", "East");
      break;
    case SouthWestGravity:
      AddToList ("Gravity:", "SouthWest");
      break;
    case SouthGravity:
      AddToList ("Gravity:", "South");
      break;
    case SouthEastGravity:
      AddToList ("Gravity:", "SouthEast");
      break;
    case StaticGravity:
      AddToList ("Gravity:", "Static");
      break;
    default:
      AddToList ("Gravity:", "Unknown");
      break;
    }
  x1 = target.frame_x;
  if (x1 < 0)
    x1 = 0;
  x2 = Scr.MyDisplayWidth - x1 - target.frame_w;
  if (x2 < 0)
    x2 = 0;
  y1 = target.frame_y;
  if (y1 < 0)
    y1 = 0;
  y2 = Scr.MyDisplayHeight - y1 - target.frame_h;
  if (y2 < 0)
    y2 = 0;
  width = (width - target.base_w) / target.width_inc;
  height = (height - target.base_h) / target.height_inc;

  sprintf (loc, "%dx%d", width, height);
  strcpy (geometry, loc);

  if ((target.gravity == EastGravity) || (target.gravity == NorthEastGravity) ||
      (target.gravity == SouthEastGravity))
    sprintf (loc, "-%d", x2);
  else
    sprintf (loc, "+%d", x1);
  strcat (geometry, loc);

  if ((target.gravity == SouthGravity) || (target.gravity == SouthEastGravity) ||
      (target.gravity == SouthWestGravity))
    sprintf (loc, "-%d", y2);
  else
    sprintf (loc, "+%d", y1);
  strcat (geometry, loc);
  AddToList ("Geometry:", geometry);
}

void
freelist (void)
{
  struct Item *cur = itemlistRoot, *cur2;

  while (cur != NULL)
    {
      cur2 = cur;
      cur = cur->next;
      free (cur2);
    }
}

