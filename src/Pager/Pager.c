/*
 * Copyright (C) 1998 Eric Tremblay <deltax@pragma.net>
 * Copyright (c) 1998 Michal Vitecek <fuf@fuf.sh.cvut.cz>
 * Copyright (c) 1998 Doug Alcorn <alcornd@earthlink.net>
 * Copyright (c) 2002,1998 Sasha Vasko <sasha at aftercode.net>
 * Copyright (c) 1997 ric@giccs.georgetown.edu
 * Copyright (C) 1998 Makoto Kato <m_kato@ga2.so-net.ne.jp>
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (C) 1996 Rainer M. Canavan (canavan@Zeus.cs.bonn.edu)
 * Copyright (C) 1996 Dan Weeks
 * Copyright (C) 1994 Rob Nation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*#define DO_CLOCKING      */

#include "../../configure.h"

#include "../../include/asapp.h"
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#define IN_MODULE
#define MODULE_X_INTERFACE

#include "../../include/afterstep.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/parse.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"
#include "../../include/mystyle.h"
#include "../../include/balloon.h"
#include "../../include/aswindata.h"
#include "../../include/decor.h"



typedef struct ASPagerDesk {
    int desk ;
    ASCanvas   *main_canvas;
    ASTBarData *title;
    ASTBarData *background;
    Window     *separator_bars;                /* (rows-1)*(columns-1) */
}ASPagerDesk;

typedef struct ASPagerState
{
    ASFlagType flags ;

    ASCanvas    *main_canvas;
    ASCanvas    *icon_canvas;

    ASPagerDesk *desks ;
    int start_desk, desks_num ;

    int desk_rows,  desk_columns;
    int page_rows,  page_columns ;
    int desk_width, desk_height ; /* x and y size of desktop */
    int aspect_x,   aspect_y;

    int wait_as_response ;

}ASPagerState;

ASPagerState PagerState ={ 0, NULL, NULL, NULL, 0, 0 };
#define DEFAULT_BORDER_COLOR 0xFF808080

void
pager_usage (void)
{
	printf ("Usage:\n"
			"%s [--version] [--help] n m\n"
			"%*s where desktops n through m are displayed\n", MyName, (int)strlen (MyName), MyName);
	exit (0);
}

/***********************************************************************
 *
 *   Procedure:
 *   main - start of module
 *
 ***********************************************************************/
int
main (int argc, char **argv)
{
    int itemp, i;
    char *cptr = NULL ;
    char *global_config_file = NULL;
    int desk1 = 0, desk2 = 0;
    int x_fd ;
	int as_fd[2] ;

    /* Save our program name - for error messages */
    InitMyApp (CLASS_PAGER, argc, argv, pager_usage, NULL, 0 );

    for( i = 1 ; i< argc && argv[i] == NULL ; ++i);
    if( i < argc )
    {
        cptr = argv[i];
        while (isspace (*cptr))
            cptr++;
        desk1 = atoi (cptr);
        while (!(isspace (*cptr)) && (*cptr))
            cptr++;
        while (isspace (*cptr))
            cptr++;

        if (!(*cptr) && i+1 < argc )
            cptr = argv[i + 1];
    }
	if (cptr)
        desk2 = atoi (cptr);
	else
        desk2 = desk1;

    if (desk2 < desk1)
	{
        PagerState.desks_num = (desk1-desk2)+1 ;
        PagerState.start_desk = desk2 ;
    }else
    {
        PagerState.desks_num = (desk2-desk1)+1 ;
        PagerState.start_desk = desk1 ;
    }

    PagerState.desks = safecalloc (PagerState.desks_num, sizeof (ASPagerDesk));

    if( (x_fd = ConnectX( &Scr, MyArgs.display_name, PropertyChangeMask )) < 0 )
    {
        show_error("failed to initialize window manager session. Aborting!", Scr.screen);
        exit(1);
    }
    if (fcntl (x_fd, F_SETFD, 1) == -1)
	{
        show_error ("close-on-exec failed");
        exit (3);
	}

    if (get_flags( MyArgs.flags, ASS_Debugging))
        set_synchronous_mode(True);
    XSync (dpy, 0);


    as_fd[0] = as_fd[1] = ConnectAfterStep (M_ADD_WINDOW |
                                            M_CONFIGURE_WINDOW |
                                            M_DESTROY_WINDOW |
                                            M_FOCUS_CHANGE |
                                            M_NEW_PAGE |
                                            M_NEW_DESK |
                                            M_NEW_BACKGROUND |
                                            M_RAISE_WINDOW |
                                            M_LOWER_WINDOW |
                                            M_ICONIFY |
                                            M_ICON_LOCATION |
                                            M_DEICONIFY |
                                            M_ICON_NAME |
                                            M_END_WINDOWLIST);

    LOCAL_DEBUG_OUT("parsing Options ...");
//    LoadBaseConfig (global_config_file, GetBaseOptions);
//    LoadConfig (global_config_file, "pager", GetOptions);

    /* Create a list of all windows */
    /* Request a list of all windows,
     * wait for ConfigureWindow packets */
    SendInfo (as_fd, "Send_WindowList", 0);

    LOCAL_DEBUG_OUT("starting The Loop ...");
    while (1)
    {
        XEvent event;
        if (PagerState.wait_as_response > 0)
        {
            ASMessage *msg = CheckASMessage (as_fd[1], WAIT_AS_RESPONSE_TIMEOUT);
            if (msg)
            {
                process_message (msg->header[1], msg->body);
                DestroyASMessage (msg);
            }
        }else
        {
            if (!My_XNextEvent (dpy, x_fd, as_fd[1], process_message, &event))
                timer_handle ();    /* handle timeout events */
            else
            {
                balloon_handle_event (&event);
                DispatchEvent (&event);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/

void
GetOptions (const char *filename)
{
  PagerConfig *config = ParsePagerOptions (filename, MyName, PagerState.start_desk, PagerState.start_desk+PagerState.desks_num);
  int i;
#ifdef DO_CLOCKING
  clock_t started = clock ();
#endif

/*   WritePagerOptions( filename, MyName, Pager.desk1, Pager.desk2, config, WF_DISCARD_UNKNOWN|WF_DISCARD_COMMENTS );
 */

#if 0
  for( i = 0 ; i < PAGER_FLAGS_MAX_SHIFT ; ++i )
	  if( get_flags( config->set_flags, (0x01<<i)) )
	  {
		  if( get_flags( config->flags, (0x01<<i)) )
			  set_flags(Pager.Flags, (0x01<<i));
		  else
			  clear_flags(Pager.Flags, (0x01<<i));
	  }

  if( get_flags( config->set_flags, PAGER_SET_ALIGN ) )
	  Look.TitleAlign = config->align;
  if( get_flags( config->set_flags, PAGER_SET_ROWS ) )
	  Pager.Rows = config->rows;
  if( get_flags( config->set_flags, PAGER_SET_COLUMNS ) )
	  Pager.Columns = config->columns;

  if( Pager.Rows == 0 )
	  Pager.Rows = 1 ;
  if( Pager.Columns == 0 )
	  Pager.Columns = ((Pager.desk2-Pager.desk1)+Pager.Rows-1) / Pager.Rows ;
  else if( Pager.Rows*Pager.Columns  < Pager.desk2-Pager.desk1 )
  	  Pager.Rows = ((Pager.desk2-Pager.desk1)+Pager.Columns-1) / Pager.Columns ;

  if( get_flags( config->set_flags, PAGER_SET_GEOMETRY ) )
  {
    if (config->geometry.flags & WidthValue)
	  window_w = config->geometry.width;
    if (config->geometry.flags & HeightValue)
	  window_h = config->geometry.height;
    if (config->geometry.flags & XValue)
	  {
  	    window_x = config->geometry.x;
    	usposition = 1;
        if (config->geometry.flags & XNegative)
	  	  window_x_negative = 1;
      }
	if (config->geometry.flags & YValue)
  	{
  	    window_y = config->geometry.y;
    	usposition = 1;
        if (config->geometry.flags & YNegative)
			window_y_negative = 1;
    }
  }
/*
  if( window_w <= 0 )
	window_w = Pager.xSize*Pager.Columns/Scr.VScale ;
  if( window_h <= 0 )
	window_h = Pager.ySize*Pager.Rows/Scr.VScale ;
fprintf( stderr, "windows size will be %dx%d (%dx%d) %d\n", window_w, window_h, Pager.Columns, Pager.Rows, Scr.VScale );
*/

  if( get_flags( config->set_flags, PAGER_SET_ICON_GEOMETRY ) )
  {
    if (config->icon_geometry.flags & WidthValue)
	  icon_w = config->icon_geometry.width;
    if (config->icon_geometry.flags & HeightValue)
	  icon_h = config->icon_geometry.height;
    if (config->icon_geometry.flags & XValue)
	  icon_x = config->icon_geometry.x;
    if (config->icon_geometry.flags & YValue)
	  icon_y = config->icon_geometry.y;
  }

  for (i = 0; i < Pager.ndesks; i++)
	  if( config->labels[i] )
	  {
		if (Pager.Desks[i].label)
		  free (Pager.Desks[i].label);
	    Pager.Desks[i].label = config->labels[i];
  	  }
#ifdef PAGER_BACKGROUND
    for (i = 0; i < Pager.ndesks; i++)
	{
	  if( config->styles[i] )
	  {
  	    if (Pager.Desks[i].StyleName)
			free (Pager.Desks[i].StyleName);
	    Pager.Desks[i].StyleName = config->styles[i];
  	  }
	}
#endif
/*
  if( icon_w <= 0 )
	icon_w = 64 ;
  if( icon_h <= 0 )
	icon_h = 64 ;
*/

  if (config->small_font_name)
    {
      load_font (config->small_font_name, &(Look.windowFont));
      free (config->small_font_name);
    }
  else if( get_flags( config->set_flags, PAGER_SET_SMALL_FONT ) )
    Look.windowFont.font = NULL;

  if (config->selection_color)
    {
      parse_argb_color(config->selection_color, &(Look.SelectionColor));
      free (config->selection_color);
    }else if( get_flags( config->set_flags, PAGER_SET_SELECTION_COLOR ) )
	  Look.SelectionColor = DEFAULT_BORDER_COLOR;

  if (config->grid_color)
    {
      parse_argb_color(config->grid_color, &Look.GridColor);
      free (config->grid_color);
    }else if( get_flags( config->set_flags, PAGER_SET_GRID_COLOR ) )
      Look.GridColor = DEFAULT_BORDER_COLOR;
  if (config->border_color)
    {
      parse_argb_color(config->border_color, &Look.BorderColor );
      free (config->border_color);
    }else if( get_flags( config->set_flags, PAGER_SET_BORDER_COLOR ) )
      Look.BorderColor  = DEFAULT_BORDER_COLOR;

  if( get_flags( config->set_flags, PAGER_SET_BORDER_WIDTH ) )
	  Look.DeskBorderWidth = config->border_width;

  if (config->style_defs)
    ProcessMyStyleDefinitions (&(config->style_defs), pixmapPath);

#endif
  DestroyPagerConfig (config);

#ifdef DO_CLOCKING
  fprintf (stderr, "\n Config parsing time (clocks): %lu\n", clock () - started);
#endif

}


/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the base file
 *
 ****************************************************************************/
void
GetBaseOptions (const char *filename)
{

  BaseConfig *config = ParseBaseOptions (filename, MyName);
#ifdef DO_CLOCKING
  clock_t started = clock ();
#endif

  if (!config)
    exit (0);			/* something terrible has happend */

  PixmapPath = config->pixmap_path;
  replaceEnvVar (&PixmapPath);
  ModulePath = config->module_path;
  replaceEnvVar (&ModulePath);
  config->pixmap_path = NULL;	/* setting it to NULL so it would not be
				   deallocated by DestroyBaseConfig */
  config->module_path = NULL;

  if (config->desktop_size.flags & WidthValue)
    PagerState.page_columns = config->desktop_size.width;
  if (config->desktop_size.flags & HeightValue)
    PagerState.page_rows = config->desktop_size.height;

  Scr.VScale = config->desktop_scale;

  DestroyBaseConfig (config);

  Scr.Vx = 0;
  Scr.Vy = 0;

  Scr.VxMax = (PagerState.page_columns - 1) * Scr.MyDisplayWidth;
  Scr.VyMax = (PagerState.page_rows - 1) * Scr.MyDisplayHeight;
  PagerState.desk_width = Scr.VxMax + Scr.MyDisplayWidth;
  PagerState.desk_height = Scr.VyMax + Scr.MyDisplayHeight;

#ifdef DO_CLOCKING
  fprintf (stderr, "\n Base Config parsing time (clocks): %lu\n", clock () - started);
#endif

}
