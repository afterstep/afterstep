/*
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

#define TRUE 1
#define FALSE

#define YES "Yes"
#define NO  "No"

#include "../../configure.h"

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#ifdef I18N
#include <X11/Xlocale.h>
#endif

#define IN_MODULE
#define MODULE_X_INTERFACE
#include "../../libAfterImage/afterimage.h"
#include "../../include/afterstep.h"
#include "../../libAfterStep/asapp.h"
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/module.h"
#include "Ident.h"

char *MyName;
int fd_width;
int fd[2];

Display *dpy;			/* which display are we talking to */
int x_fd;

char *BackColor = "white";
char *ForeColor = "black";
char *font_string = "fixed";

Pixel back_pix, fore_pix;
GC NormalGC;
Window main_win;
Window app_win;
MyFont font;
volatile int x_error = 0;

int Width, Height, win_x, win_y;

#define MW_EVENTS   (ExposureMask | ButtonReleaseMask | KeyReleaseMask)

static Atom wm_del_win;

struct target_struct target;
int found = 0;

ScreenInfo Scr;

struct Item *itemlistRoot = NULL;
int max_col1, max_col2;
char id[15], desktop[10], swidth[10], sheight[10], borderw[10], geometry[30];

void
version (void)
{
  printf ("%s version %s\n", MyName, VERSION);
  exit (0);
}

void
usage (void)
{
  printf ("Usage:\n"
	  "%s [-f [config file]] [-v|--version] [-h|--help] [--window window-id]\n", MyName);
  exit (0);
}

/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
int
main (int argc, char **argv)
{
  char configfile[255];
  char *realconfigfile;
  char *temp;
  int i;
  char *global_config_file = NULL;

  /* Save our program name - for error messages */
  temp = strrchr (argv[0], '/');
  MyName = temp ? temp + 1 : argv[0];

  for (i = 1; i < argc && *argv[i] == '-'; i++)
    {
      if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
	usage ();
      else if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "--version"))
	version ();
      else if (!strcmp (argv[i], "-w") || !strcmp (argv[i], "--window"))
	app_win = strtol (argv[++i], NULL, 16);
      else if (!strcmp (argv[i], "-c") || !strcmp (argv[i], "--context"))
	i++;
      else if (!strcmp (argv[i], "-f") && i + 1 < argc)
	global_config_file = argv[++i];
    }

#ifdef I18N
  if (setlocale (LC_CTYPE, AFTER_LOCALE) == NULL)
    fprintf (stderr, "%s: cannot set locale\n", MyName);
#endif

  /* Dead pipe == dead AfterStep */
  signal (SIGPIPE, DeadPipe);
  set_signal_handler(SIGSEGV);

  x_fd = ConnectX( &Scr, 0 );
  /* connect to AfterStep */
  fd[0] = fd[1] = ConnectAfterStep( M_CONFIGURE_WINDOW|
								    M_WINDOW_NAME|
								    M_ICON_NAME|
									M_RES_CLASS|
								    M_RES_NAME|
									M_END_WINDOWLIST
                                     );

  /* scan config file for set-up parameters */
  /* Colors and fonts */

  if (global_config_file != NULL)
    ParseOptions (global_config_file);
  else
    {
      sprintf (configfile, "%s/ident", AFTER_DIR);
      realconfigfile = PutHome (configfile);

      if ((CheckFile (realconfigfile)) == -1)
	{
	  free (realconfigfile);
	  sprintf (configfile, "%s/ident", AFTER_SHAREDIR);
	  realconfigfile = PutHome (configfile);
	}
      ParseOptions (realconfigfile);
      free (realconfigfile);
    }

  if (app_win == 0)
    GetTargetWindow (&app_win);

  fd_width = GetFdWidth ();

  /* Create a list of all windows */
  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendInfo ("Send_WindowList", 0);

  Loop (fd);

  return 0;
}


void
ParseOptions (const char *filename)
{
  FILE *file;
  char *line, *tline;
  int len;

  file = fopen (filename, "r");
  if (file != NULL)
    {
      line = (char *) safemalloc (MAXLINELENGTH);
      len = strlen (MyName);
      while ((tline = fgets (line, MAXLINELENGTH, file)) != NULL)
	{
	  while (isspace (*tline))
	    tline++;
	  if ((*tline == '*') && (!mystrncasecmp (tline + 1, MyName, len)))
	    {
	      tline += len + 1;
	      if (!mystrncasecmp (tline, "Font", 4))
        font_string = stripcpy(tline + 4);
	      else if (!mystrncasecmp (tline, "Fore", 4))
        ForeColor = stripcpy( tline + 4);
	      else if (!mystrncasecmp (tline, "Back", 4))
        BackColor = stripcpy( tline + 4);
	    }
	}
      fclose (file);
      free (line);
    }
}

/**************************************************************************
 *
 * Read the entire window list from AfterStep
 *
 *************************************************************************/
void
Loop (int *fd)
{
  unsigned long header[3], *body;
  int count;

  while (1)
    {
      if ((count = ReadASPacket (fd[1], header, &body)) > 0)
	{
	  process_message (header[1], body);
	  free (body);
	}
    }
}


/**************************************************************************
 *
 * Process window list messages
 *
 *************************************************************************/
void
process_message (unsigned long type, unsigned long *body)
{
  switch (type)
    {
    case M_CONFIGURE_WINDOW:
      list_configure (body);
      break;
    case M_WINDOW_NAME:
      list_window_name (body);
      break;
    case M_ICON_NAME:
      list_icon_name (body);
      break;
    case M_RES_CLASS:
      list_class (body);
      break;
    case M_RES_NAME:
      list_res_name (body);
      break;
    case M_END_WINDOWLIST:
      list_end ();
      break;
    default:
      break;

    }
}




/***********************************************************************
 *
 * Detected a broken pipe - time to exit
 *
 **********************************************************************/
void
DeadPipe (int nonsense)
{
  freelist ();
  exit (0);
}

/***********************************************************************
 *
 * Got window configuration info - if its our window, safe data
 *
 ***********************************************************************/
void
list_configure (unsigned long *body)
{
  if ((app_win == (Window) body[1]) || (app_win == (Window) body[0])
      || ((body[19] != 0) && (app_win == (Window) body[19]))
      || ((body[19] != 0) && (app_win == (Window) body[20])))
    {
      app_win = body[1];
      target.id = body[0];
      target.frame = body[1];
      target.frame_x = body[3];
      target.frame_y = body[4];
      target.frame_w = body[5];
      target.frame_h = body[6];
      target.desktop = body[7];
      target.flags = body[8];
      target.title_h = body[9];
      target.border_w = body[10];
      target.base_w = body[11];
      target.base_h = body[12];
      target.width_inc = body[13];
      target.height_inc = body[14];
      target.gravity = body[21];
      found = 1;
    }
}

/*************************************************************************
 *
 * Capture  Window name info
 *
 ************************************************************************/
void
list_window_name (unsigned long *body)
{
  if ((app_win == (Window) body[1]) || (app_win == (Window) body[0]))
    {
      strncpy (target.name, (char *) &body[3], 255);
    }
}

/*************************************************************************
 *
 * Capture  Window Icon name info
 *
 ************************************************************************/
void
list_icon_name (unsigned long *body)
{
  if ((app_win == (Window) body[1]) || (app_win == (Window) body[0]))
    {
      strncat (target.icon_name, (char *) &body[3], 255);
    }
}


/*************************************************************************
 *
 * Capture  Window class name info
 *
 ************************************************************************/
void
list_class (unsigned long *body)
{
  if ((app_win == (Window) body[1]) || (app_win == (Window) body[0]))
    {
      strncat (target.class, (char *) &body[3], 255);
    }
}


/*************************************************************************
 *
 * Capture  Window resource info
 *
 ************************************************************************/
void
list_res_name (unsigned long *body)
{
  if ((app_win == (Window) body[1]) || (app_win == (Window) body[0]))
    {
      strncat (target.res, (char *) &body[3], 255);
    }
}

Bool
gnome_hints (Window id)
{
  Atom type;
  int format;
  unsigned long length, after;
  unsigned char *data;
  Atom Sup_check;

  Sup_check = XInternAtom (dpy, "_WIN_SUPPORTING_WM_CHECK", False);
  if (Sup_check == None)
    return False;
  if (app_win == None)
    return False;
  if (XGetWindowProperty (dpy, Scr.Root, Sup_check, 0L, 1L, False, AnyPropertyType,
			  &type, &format, &length, &after, &data) == Success)
    {
      if (type == XA_CARDINAL && format == 32 && length == 1)
	{
	  Window win = *(long *) data;
	  unsigned char *data1;
	  if (XGetWindowProperty (dpy, win, Sup_check, 0L, 1L, False,
				  AnyPropertyType, &type, &format, &length,
				  &after, &data1) == Success)
	    {
	      if (type == XA_CARDINAL && format == 32 && length == 1)
		{
		  XFree (data);
		  XFree (data1);
		  return True;
		}
	      XFree (data1);
	    }
	}
      XFree (data);
    }
  return False;
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



/**********************************************************************
 *
 * If no application window was indicated on the command line, prompt
 * the user to select one
 *
 *********************************************************************/
void
GetTargetWindow (Window * app_win)
{
  XEvent eventp;
  int val = -10, trials;

  trials = 0;
  while ((trials < 100) && (val != GrabSuccess))
    {
      val = XGrabPointer (dpy, Scr.Root, True,
			  ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, Scr.Root,
			  XCreateFontCursor (dpy, XC_crosshair),
			  CurrentTime);
      if (val != GrabSuccess)
	{
	  sleep_a_little (1000);
	}
      trials++;
    }
  if (val != GrabSuccess)
    {
      fprintf (stderr, "%s: Couldn't grab the cursor!\n", MyName);
      exit (1);
    }
  XMaskEvent (dpy, ButtonReleaseMask, &eventp);
  XUngrabPointer (dpy, CurrentTime);
  XSync (dpy, 0);
  *app_win = eventp.xany.window;
  if (eventp.xbutton.subwindow != None)
    *app_win = eventp.xbutton.subwindow;
}

/************************************************************************
 *
 * Draw the window
 *
 ***********************************************************************/
void
RedrawWindow (void)
{
  int fontheight, i = 0;
  struct Item *cur = itemlistRoot;

  fontheight = font.height;

  while (cur != NULL)
    {
      /* first column */
#undef FONTSET
#define FONTSET font.fontset
      XDrawString (dpy, main_win, NormalGC, 5, 5 + font.font->ascent + i * fontheight,
		   cur->col1, strlen (cur->col1));
      /* second column */
      XDrawString (dpy, main_win, NormalGC, 10 + max_col1, 5 + font.font->ascent + i * fontheight,
		   cur->col2, strlen (cur->col2));
      ++i;
      cur = cur->next;
    }
}

/**************************************************************************
 *  Change the window name displayed in the title bar.
 **************************************************************************/
void
change_window_name (char *str)
{
  XTextProperty name;

  if (XStringListToTextProperty (&str, 1, &name) == 0)
    {
      fprintf (stderr, "%s: cannot allocate window name", MyName);
      return;
    }
  XSetWMName (dpy, main_win, &name);
  XSetWMIconName (dpy, main_win, &name);
  XFree (name.value);
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


void
nocolor (char *a, char *b)
{
  fprintf (stderr, "InitBanner: can't %s %s\n", a, b);
}
