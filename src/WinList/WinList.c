 /*
    * Copyright (c) 1998 Rafal Wierzbicki <rafal@mcss.mcmaster.ca>
    * Copyright (c) 1998 Michal Vitecek <M.Vitecek@sh.cvut.cz>
    * Copyright (C) 1998 Sasha Vasko <sasha at aftercode.net>
    * Copyright (c) 1998 Makoto Kato <m_kato@ga2.so-net.ne.jp>
    * Copyright (c) 1998 Rene Fichter <ceezaer@cyberspace.org>
    * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
    * Copyright (c) 1994 Mike Finger <mfinger@mermaid.micro.umn.edu>
    *                    or <Mike_Finger@atk.com>
    * Copyright (c) 1994 Robert Nation
    * Copyright (c) 1994 Nobutaka Suzuki
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
    * $Id: WinList.c,v 1.3 2002/05/01 06:15:00 sashav Exp $
  */

#define TRUE 1
#define FALSE 0

#include "WinList.h"

Window window = None; /* app window */
ASImage *back_image = NULL ;
int w_height = 5;              /* window height */
int w_width = 5;               /* window width  */
int Balloons = 0;               /* flag for running with balloons enabled */
Display *dpy;                   /* display, needed by AS libs */
int screen;	         /* for balloons and mystyles*/
ScreenInfo Scr;                   /* root window */
char *MyName = NULL;     /* module name, needed by AS libs */
ButtonArray buttons;        /* array of buttons */
List windows;                    /* list of same */

/* Window and look related variables */
static char *pixmap_path = NULL;
static char *icon_path = NULL;
static int AutoHide = 0;
static MyStyle *Style = NULL;
/*static MyStyle *hide_style = NULL; */
static Atom _AS_STYLE;
static Atom _XROOTPMAP_ID;

/* X stuff */
static int hidden = 1;

/* File type information */
static int fd_width;
static int fd[2];
static int x_fd;

int fontheight;
static Atom wm_del_win;
static Atom MwmAtom = None;
/* Module related information */
static int WindowIsUp = 0;
static int win_grav;
static int hide_x = 0, hide_y = 0, hide_width = -1, hide_height;
int win_x = 0, win_y = 0, anchor_x = 0, anchor_y = 0;
static int Pressed = 0, ButPressed = 0;
static int current_desk;
static int intransit = 0;

static char *ClickAction[3] =
{NULL, NULL, NULL};
static char *EnterAction, *geometry = NULL, *hide_geometry = NULL;
int UseSkipList = 0, Anchor = 1, UseIconNames = 0, OrientVert = TRUE;
int Justify = 0;
unsigned int MaxWidth = 0;
Balloon *hide_balloon = NULL;

/******************************************************************************
  Main - Setup the XConnection,request the window list and loop forever
    Based on main() from Ident:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
int
main (int argc, char **argv)
{
  char *display_name = NULL;
  char *buf;
  int i;
  char *global_config_file = NULL;

  /* Save our program name - for error messages */
  SetMyName (argv[0]);
  i = ProcessModuleArgs (argc, argv, &(global_config_file), NULL, NULL, NULL);

  /* Dead pipe == dead AfterStep */
  signal (SIGPIPE, DeadPipe);
  signal (SIGQUIT, DeadPipe);
  signal (SIGSEGV, DeadPipe);
  signal (SIGTERM, DeadPipe);

#ifdef I18N
  if (setlocale (LC_CTYPE, AFTER_LOCALE) == NULL)
    fprintf (stderr, "%s: cannot set locale\n", MyName);
#endif

  x_fd = ConnectX (&Scr, display_name, PropertyChangeMask);
  screen = Scr.screen ;
  _AS_STYLE = XInternAtom (dpy, "_AS_STYLE", False);
  _XROOTPMAP_ID = XInternAtom (dpy, "_XROOTPMAP_ID", False);
  XSetErrorHandler (error_handler);

  fd_width = GetFdWidth ();
  fd[0] = fd[1] = ConnectAfterStep (mask_reg);

  w_height = 50;
  w_width = 50;

  buf = safemalloc (1 + strlen (MyName) + 10);
  sprintf (buf, "*%s", MyName);

  /* get MyStyles */
  mystyle_get_property (dpy, Scr.Root, _AS_STYLE, XA_INTEGER);

  /* check if *WinList is in mystyles */
  if ((Style = mystyle_find (buf)) == NULL)
    /* if not create our own */
    {
      Style = mystyle_new_with_name (buf);
    }
  free (buf);


  LoadBaseConfig (global_config_file, ParseBaseOptions);
  LoadConfig (global_config_file, "winlist", ParseOptions);

  mystyle_fix_styles ();

  if (Balloons)
    {
      balloon_setup (dpy);
      balloon_set_style (dpy, mystyle_find_or_default ("*WinListBalloon"));
    }

  /* Setup the XConnection */
  StartMeUp ();
  InitArray (&buttons, 0, 0, w_width, fontheight + 6);
  InitList (&windows);
  MakeMeWindow ();
  SendInfo (fd, "Send_WindowList\n", 0);

  while (1)
    {
      XEvent event;

      if (!My_XNextEvent (dpy, x_fd, fd[1], process_message, &event))
	timer_handle ();	/* handle timeout events */
      else if (hidden || !balloon_handle_event (&event))
	DispatchEvent (&event);
    }
  return 0;
}

/* error_handler:
 * catch X errors, display the message and continue running.
 */
int
error_handler (Display * disp, XErrorEvent * event)
{
  fprintf (stderr, "%s: internal error, error code %d, request code %d, minor code %d.\n",
	 MyName, event->error_code, event->request_code, event->minor_code);
  return 0;
}


/***********************************************************************
  Detected a broken pipe - time to exit
    Based on DeadPipe() from Ident:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
 **********************************************************************/
void
DeadPipe (int sig)
{
  switch (sig)
    {
    case SIGSEGV:
      fprintf (stderr, "Segmentation fault in WinList, please send a bug report\n");
      exit (-1);
      break;
    case SIGQUIT:
      fprintf (stderr, "SIGQUITting\n");
      break;
    case SIGTERM:
    default:
      break;
    }
  ShutMeDown (1);
}

/******************************************************************************
  WaitForExpose - Used to wait for expose event so we don't draw too early
******************************************************************************/
void
WaitForExpose (void)
{
  XEvent Event;
  while (1)
    {
      XNextEvent (dpy, &Event);
      if (Event.type == Expose)
	{
	  if (Event.xexpose.count == 0)
	    break;
	}
    }
}

/******************************************************************************
  ParseOptions - Parse the configuration file AfterStep to us to use
    Based on part of main() from Ident:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
      Changed by Michal Vitecek, 1998
      Additions by Rafal Wierzbicki, 1998
******************************************************************************/
void
ParseOptions (const char *file)
{
  char *line, *tline;
  FILE *ptr;
  int len;

  LinkAction ("Click1 Iconify -1,Raise");
  LinkAction ("Click2 Iconify");
  LinkAction ("Click3 Lower");

  if ((ptr = fopen (file, "r")) == NULL)
    {
      fprintf (stderr, "%s: can\'t open config file %s", MyName, file);
      exit (1);
    }

  line = (char *) safemalloc (MAXLINELENGTH);
  len = strlen (MyName);

  while ((tline = fgets (line, MAXLINELENGTH, ptr)) != NULL)
    {
      while (isspace (*tline))
	tline++;
      if ((*tline == '*') && (!mystrncasecmp (tline + 1, MyName, len)))
	{
	  tline += len + 1;
	  if (balloon_parse (tline, ptr))
	    {
	      Balloons = 1;
	      continue;
	    }
	  else if (!mystrncasecmp (tline, "Geometry", 8))
	    {
	      if (geometry)
		free (geometry);
	      CopyString (&geometry, tline + 8);
	    }
	  else if (!mystrncasecmp (tline, "HideGeometry", 12))
	    {
	      if (hide_geometry)
		free (hide_geometry);
	      CopyString (&hide_geometry, tline + 12);
	    }
	  else if (!mystrncasecmp (tline, "NoAnchor", 8))
	    Anchor = 0;
	  else if (!mystrncasecmp (tline, "Action", 6))
	    LinkAction (tline + 6);
	  else if (!mystrncasecmp (tline, "UseSkipList", 11))
	    UseSkipList = 1;
	  else if (!mystrncasecmp (tline, "UseIconNames", 12))
	    UseIconNames = 1;
	  else if (!mystrncasecmp (tline, "Justify", 7))
	    {
	      tline += 7;
	      while (isspace (*tline))
		tline++;
	      switch (tolower (*tline))
		{
		case 'l':
		  Justify = -1;
		  break;
		case 'c':
		  Justify = 0;
		  break;
		case 'r':
		  Justify = 1;
		  break;
		}
	    }
	  else if (!mystrncasecmp (tline, "MaxWidth", 8))
	    {
	      MaxWidth = atoi (tline + 8);
	      if (MaxWidth <= 1)
		MaxWidth = 0;
	    }
	  else if (!mystrncasecmp (tline, "Orientation", 11))
	    {
	      if (!mystrncasecmp (tline + 12, "across", 6))
		OrientVert = FALSE;
	    }
	  else if (!mystrncasecmp (tline, "AutoHide", 8))
	    {
	      AutoHide = atoi (tline + 8);
	    }
	}
      else if (!mystrncasecmp (tline, "MyStyle", 7))
	{
	  mystyle_parse (tline + 7, ptr, &pixmap_path, NULL);
	}
    }
  free (line);
  fclose (ptr);
}

/* ParseBaseOptions - parse the appropriate base.xxbpp file
 * sets pixmap and icon paths to appropriate values
 * based on ParseOptions, Rafal Wierzbicki 1998.
 */
void
ParseBaseOptions (const char *file)
{
  char line[MAXLINELENGTH];
  char *tline;
  FILE *ptr;

  if ((ptr = fopen (file, "r")) == NULL)
    {
      fprintf (stderr, "%s: can\'t open config file %s", MyName, file);
      exit (1);
    }

  tline = fgets (line, sizeof (line), ptr);
  while (tline != NULL)
    {
      while (isspace (*tline))
	tline++;
      if (strlen (tline) > 1)
	{
	  if (!mystrncasecmp (tline, "IconPath", 8))
	    CopyString (&icon_path, tline + 8);
	  else if (!mystrncasecmp (tline, "PixmapPath", 10))
	    CopyString (&pixmap_path, tline + 10);
	}
      tline = fgets (line, sizeof (line), ptr);
    }
  fclose (ptr);
}

int
GetWinWidth (int total)
{
  int i, tw, width = 0;
  char *temp;

  if (MaxWidth > 0)
    return MaxWidth;

  if (total <= 0)
    total = ItemCount (&windows);
  for (i = 0; i < total; i++)
    {
      temp = ItemName (&windows, i);
      if (temp != NULL)
	{
	  tw = 10 + XTextWidth (Style->font.font, temp, strlen (temp));
	  tw += XTextWidth (Style->font.font, "()", 2);
	  width = max (width, tw);
	}
    }
  return width;
}

int
GetWinHeight (int total)
{
  int height;
  height = fontheight + 6 + 1;
  if (OrientVert)
    {
      if (total <= 0)
	total = ItemCount (&windows);
      height = total * height;
    }
  return height - 1;
}

/******************************************************************************
  DispatchEvent - Process all the X events we get
******************************************************************************/
int fromExpose = 0;
int bAdjustingWindow = 0;

void
DispatchEvent (XEvent * event)
{
  int num;
  char buffer[10];

  switch (event->type)
    {
    case ButtonRelease:
      if (Pressed)
	{
	  num = WhichButton (&buttons, event->xbutton.x, event->xbutton.y);
	  if (num != -1)
	    {
	      send_as_mesg (ClickAction[event->xbutton.button - 1],
			    ItemID (&windows, num));
	      SwitchButton (Style, &buttons, num);
	    }
	}
      Pressed = 0;
      ButPressed = -1;
      break;

    case ButtonPress:
      if (hidden)
	break;
      num = WhichButton (&buttons, event->xbutton.x, event->xbutton.y);
      if (num != -1)
	{
	  SwitchButton (Style, &buttons, num);
	  ButPressed = num;
	}
      else
	ButPressed = -1;
      Pressed = 1;
      break;

    case Expose:
      fromExpose = 0;
      if (event->xexpose.count == 0)
	{

	  if (!hidden)
	    {
	      /* this needs work, redraw only what's needed */
	      DrawButtonArray (Style, &buttons, TRUE);
	    }
	  else
	    {
	      hide_window ();
	      fromExpose = 1;
	    }
	}
      break;

    case KeyPress:
      num = XLookupString (&(event->xkey), buffer, 10, NULL, 0);
      if (num == 1)
	{
	  if (buffer[0] == 'q' || buffer[0] == 'Q')
	    ShutMeDown (0);
	  else if (buffer[0] == 'i' || buffer[0] == 'I')
	    PrintList (&windows);
	  else if (buffer[0] == 'b' || buffer[0] == 'B')
	    {
	      PrintButtons (&buttons);
	    }
	  else if (buffer[0] == 'u' || buffer[0] == 'U')
	    {
	      AutoHide = FALSE;
	      unhide_winlist ();
	    }
	  else if (buffer[0] == 'h' || buffer[0] == 'H')
	    {
	      AutoHide = TRUE;
	      hide_winlist (TRUE);
	    }
	}
      break;

    case ClientMessage:
      if ((event->xclient.format == 32) &&
	  (event->xclient.data.l[0] == wm_del_win))
	ShutMeDown (0);

    case EnterNotify:
      /*if (hidden)
         { */
      unhide_winlist ();
      /*  break; */
      /*} */
      if (!SomeButtonDown (event->xcrossing.state))
	break;
      num = WhichButton (&buttons, event->xcrossing.x, event->xcrossing.y);
      if (num != -1)
	{
	  SwitchButton (Style, &buttons, num);
	  ButPressed = num;
	}
      else
	ButPressed = -1;
      Pressed = 1;
      break;

    case LeaveNotify:
      if (!SomeButtonDown (event->xcrossing.state))
	{
	  if (AutoHide && !hidden && !intransit)
	    hide_winlist (False);
	  break;
	}
      if (ButPressed != -1)
	SwitchButton (Style, &buttons, ButPressed);
      Pressed = 0;
      if (AutoHide && !hidden)
	hide_winlist (False);
      break;

    case MotionNotify:
      if (!Pressed)
	{
	  break;
	}
      if (hidden)
	break;
      num = WhichButton (&buttons, event->xmotion.x, event->xmotion.y);
      if (num == ButPressed)
	break;
      if (ButPressed != -1)
	SwitchButton (Style, &buttons, ButPressed);
      if (num != -1)
	{
	  SwitchButton (Style, &buttons, num);
	  ButPressed = num;
	}
      else
	ButPressed = -1;
      break;
    case PropertyNotify:
      if (event->xproperty.atom == _AS_STYLE)
	{
	  mystyle_get_property (dpy, Scr.Root, _AS_STYLE, XA_INTEGER);
	  balloon_set_style (dpy, mystyle_find_or_default ("*WinListBalloon"));
	  update_look ();
	  update_winlist_background ();
	}
      else if (event->xproperty.atom == _XROOTPMAP_ID)
	{
	  update_winlist_background ();
	  if (hidden)
	    update_hidden_winlist ();
	}
      break;
    case ConfigureNotify:
      /* only process if event was from outside generated movement */
      if (bAdjustingWindow)
	{
	  bAdjustingWindow--;
	  break;
	}

      if (hidden || (w_height == event->xconfigure.height && w_width == event->xconfigure.width))
	{
	  Window root, parent, *children, w = window;
	  unsigned int nchildren;
	  unsigned int dummy;
	  int new_x = 0, new_y = 0;

	  while (XQueryTree (dpy, w, &root, &parent, &children, &nchildren))
	    {
	      if (children)
		XFree (children);
	      if (root == parent)
		break;
	      w = parent;
	    }
	  if (w != None && w != window)
	    XGetGeometry (dpy, w, &root, &new_x, &new_y, &dummy, &dummy, &dummy, &dummy);
	  else
	    {
	      new_x = event->xconfigure.x;
	      new_y = event->xconfigure.y;
	    }

	  if (hidden)
	    {
	      hide_x = new_x;
	      hide_y = new_y;
	      hide_width = event->xconfigure.width;
	      hide_height = event->xconfigure.height;
	    }
	  else
	    {
	      win_x = new_x;
	      win_y = new_y;
	      if (win_grav == SouthEastGravity || win_grav == NorthEastGravity)
		anchor_x = win_x + GetWinWidth (0);
	      else
		anchor_x = win_x;
	      if (win_grav == SouthEastGravity || win_grav == SouthWestGravity)
		anchor_y = win_y + GetWinHeight (0);
	      else
		anchor_y = win_y;
	    }
	}
      break;
    }
}


/* AdjustWindow - Resize the window according to maxwidth by number of buttons
 * mess, fix this.
 */


int
AdjustWindow ()
{
  int new_width = 0, new_height = 0, bAdjusted = 0, total;

  if ((total = ItemCount (&windows)) == 0)
    return bAdjusted;

  new_width = GetWinWidth (total);
  new_height = GetWinHeight (total);

  if (WindowIsUp && (new_height != w_height || new_width != w_width))
    {
      if (!hidden)
	{
	  if (win_grav == SouthEastGravity || win_grav == NorthEastGravity)
	    win_x = anchor_x - new_width;
	  if (win_grav == SouthEastGravity || win_grav == SouthWestGravity)
	    win_y = anchor_y - new_height;
	  bAdjustingWindow++;
	  XMoveResizeWindow (dpy, window, win_x, win_y, new_width, new_height);
	  bAdjusted = 1;
	}
    }
  UpdateArray (Style, &buttons, -1, -1, new_width, -1);
  if (new_height > 0)
    w_height = new_height;
  if (new_width > 0)
    w_width = new_width;

#ifndef NO_TEXTURE
  if (bAdjusted)
    update_winlist_background ();
#endif

  return bAdjusted;

}

/******************************************************************************
  makename - Based on the flags return me '(name)' or 'name'
******************************************************************************/
char *
makename (char *string, long flags)
{
  char *ptr;
  ptr = safemalloc (strlen (string) + 3);
  if (flags & ICONIFIED)
    strcpy (ptr, "(");
  else
    strcpy (ptr, "");
  strcat (ptr, string);
  if (flags & ICONIFIED)
    strcat (ptr, ")");
  return ptr;
}

/******************************************************************************
  LinkAction - Link an response to a users action
******************************************************************************/
void
LinkAction (char *string)
{
  char *temp;
  temp = string;
  while (isspace (*temp))
    temp++;
  if (mystrncasecmp (temp, "Click1", 6) == 0)
    {
      if (ClickAction[0])
	free (ClickAction[0]);
      CopyString (&ClickAction[0], &temp[6]);
    }
  else if (mystrncasecmp (temp, "Click2", 6) == 0)
    {
      if (ClickAction[1])
	free (ClickAction[1]);
      CopyString (&ClickAction[1], &temp[6]);
    }
  else if (mystrncasecmp (temp, "Click3", 6) == 0)
    {
      if (ClickAction[2])
	free (ClickAction[2]);
      CopyString (&ClickAction[2], &temp[6]);
    }
  else if (mystrncasecmp (temp, "Enter", 5) == 0)
    {
      if (EnterAction)
	free (EnterAction);
      CopyString (&EnterAction, &temp[5]);
    }
}

/* unhide_winlist:
 * unhides WinList if hidden
 */

void
unhide_winlist (void)
{

  if (buttons.count == 0)
    return;

  XMoveResizeWindow (dpy, window, win_x, win_y, w_width, w_height);

  DrawButtonArray (Style, &buttons, 1);
  hidden = FALSE;
  update_winlist_background ();
}

/* update_winlist_background:
 * updates WinList's background (relies on MyStyles)
 */

void
update_winlist_background ()
{
  int x, y, w, h;

  if (hidden)
    {
      x = hide_x;
      y = hide_y;
      w = hide_width;
      h = hide_height;
    }
  else
    {
      x = win_x;
      y = win_y;
      w = w_width;
      h = w_height;
    }

	if (Style->texture_type != 0 && w > 0 && h > 0)
    {
		if( back_image ) 
		    destroy_asimage( &back_image );
		back_image = mystyle_make_image( Style, x, y, w, h );
    }
	if (back_image != None)
    {
		Pixmap transpix = asimage2pixmap( Scr.asv, Scr.Root, back_image, NULL, True );
    	XSetWindowBackgroundPixmap (dpy, window, transpix);
    	XFreePixmap (dpy, transpix);
    }
    XClearWindow (dpy, window);

	if (!hidden)
  		DrawButtonArray (Style, &buttons, TRUE);

	XFlush (dpy);
}

/* update_look:
 * update the look when _AS_STYLE changed (MyStyles)
 */

void
update_look ()
{
  int new_height;
  int new_fh;
  int width = 0;
  char *buf;

  set_as_mask ((unsigned long) mask_off);

  new_fh = Style->font.height;

  width = (hidden) ? hide_width : w_width;

  if (OrientVert || buttons.count == 0)
    new_height = (buttons.count * (new_fh + 6 + 1) - 1);
  else
    new_height = new_fh + 6 + 1;

  XResizeWindow (dpy, window, width, new_height);

  buf = safemalloc (1 + strlen (MyName) + 10);
  sprintf (buf, "*%s", MyName);
  if ((Style = mystyle_find (buf)) == NULL)
    /* if not create our own */
    {				/* in case our style got destroyed somehow */
      Style = mystyle_new_with_name (buf);
    }
  free (buf);

  if (hidden)
    hide_height = new_height;
  else
    w_height = new_height;

  fontheight = new_fh;
  UpdateArray (Style, &buttons, -1, -1, -1, -1);
  DrawButtonArray (Style, &buttons, TRUE);

  set_as_mask ((unsigned long) mask_reg);
}

/* update_hidden_winlist:
 * resize hidden WinList when necessary
 */

void
update_hidden_winlist ()
{

  if (!hidden)
    return;
  hide_window ();
}

/* hide_winlist:
 * hide WinList, True forces it to be hidden
 */

void
hide_winlist (Boolean force)
{
  Window root, child;
  int root_x, root_y, x, y;
  unsigned int udummy;

  if (hidden)
    return;

  XQueryPointer (dpy, window, &root, &child, &root_x, &root_y, &x, &y,
		 &udummy);
  if (Balloons && !force)
    {
      hidden = FALSE;
      if (root_x < win_x + w_width && root_x > win_x)
	if (root_y < win_y + w_height && root_y > win_y)
	  return;
    }

  hide_window ();
}

/******************************************************************************
      MakeMeWindow - Create and setup the window we will need
******************************************************************************/

void
MakeMeWindow (void)
{
  GC fore_gc, back_gc, shadow_gc, relief_gc;
  int width, height;
  Pixel fore, back;
  XSizeHints size_hints;
  long flags;

  back = Style->colors.back;
  fore = Style->colors.fore;

  size_hints.width = MaxWidth;
  size_hints.height = fontheight + 6;
  size_hints.flags = PPosition | PSize;
  if (!OrientVert)
    size_hints.flags |= PMinSize | PMaxSize;

  size_hints.x = 0;
  size_hints.y = 0;
  size_hints.max_width = MaxWidth;
  size_hints.max_height = fontheight + 6;
  size_hints.min_width = MaxWidth;
  size_hints.min_height = fontheight + 6;
  win_grav = NorthWestGravity;
  if (geometry != NULL)
    {
      flags = XParseGeometry (geometry, &anchor_x, &anchor_y, &width, &height);

      if (flags & WidthValue)
	w_width = size_hints.max_width = size_hints.min_width = width;
      if (flags & HeightValue)
	w_height = size_hints.max_height = size_hints.min_height = height;


      if (XValue & flags)
	{
	  if (XNegative & flags)
	    {
	      anchor_x += DisplayWidth (dpy, screen);
	      size_hints.x = anchor_x - size_hints.width;
	      win_grav = NorthEastGravity;
	    }
	  else
	    size_hints.x = anchor_x;
	  size_hints.flags |= USPosition;
	}
      if (YValue & flags)
	{
	  if (YNegative & flags)
	    {
	      anchor_y += DisplayHeight (dpy, screen);
	      size_hints.y = anchor_y - size_hints.height;
	      win_grav = (win_grav == NorthWestGravity) ? SouthWestGravity : SouthEastGravity;
	    }
	  else
	    size_hints.y = anchor_y;
	  size_hints.flags |= USPosition;
	}
    }

  window = XCreateSimpleWindow (dpy, Scr.Root, size_hints.x, size_hints.y,
				size_hints.width, size_hints.height,
				1, fore, back);
  ChangeWindowName (MyName);

  SetMwmHints (MWM_DECOR_ALL | MWM_DECOR_RESIZEH | MWM_DECOR_MAXIMIZE |
	       MWM_DECOR_MINIMIZE, MWM_FUNC_ALL | MWM_FUNC_RESIZE |
	       MWM_FUNC_MAXIMIZE | !MWM_FUNC_MINIMIZE, MWM_INPUT_MODELESS);

  XSetWMNormalHints (dpy, window, &size_hints);

  win_x = size_hints.x;
  win_y = size_hints.y;
  w_width = size_hints.width;
  w_height = size_hints.height;

  mystyle_get_global_gcs (Style, &fore_gc, &back_gc, &relief_gc, &shadow_gc);

  wm_del_win = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
  XSetWMProtocols (dpy, window, &wm_del_win, 1);

  XGrabButton (dpy, 1, AnyModifier, window, True, GRAB_EVENTS,
	       GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton (dpy, 2, AnyModifier, window, True, GRAB_EVENTS,
	       GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton (dpy, 3, AnyModifier, window, True, GRAB_EVENTS,
	       GrabModeAsync, GrabModeAsync, None, None);

  XSelectInput (dpy, window, (ExposureMask | KeyPressMask |
			      EnterWindowMask | LeaveWindowMask |
			      PointerMotionMask | StructureNotifyMask));
  XMapRaised (dpy, window);
  WaitForExpose ();
  WindowIsUp = 1;

  if (AutoHide)
    hide_winlist (TRUE);
}

/******************************************************************************
  StartMeUp - Do X initialization things
******************************************************************************/
void
StartMeUp ()
{

  fontheight = Style->font.height;
  w_width = XTextWidth (Style->font.font, "XXXXXXXXXXXXXXX", 10);
}

/******************************************************************************
  ShutMeDown - Do X client cleanup
******************************************************************************/
void
ShutMeDown (int exitstat)
{
  int i;

  FreeList (&windows);
  FreeAllButtons (&buttons);
  if (Balloons)
    balloon_init (1);
  if (WindowIsUp)
    XDestroyWindow (dpy, window);

  free (pixmap_path);
  free (icon_path);
  if (geometry)
    free (geometry);
  if (hide_geometry)
    free (hide_geometry);
  if (EnterAction)
    free (EnterAction);
  for (i = 0; i < 3; i++)
    if (ClickAction[i])
      free (ClickAction[i]);

  XCloseDisplay (dpy);

#ifdef DEBUG_ALLOCS
/* normally, we let the system clean up, but when auditing time comes
 * around, it's best to have the books in order... */
  {
    GC gc1, gc2, gc3, gc4;

    /* kill the styles */
    while (mystyle_first != NULL)
      mystyle_delete (mystyle_first);

    /* kill the global GCs so they won't show up in the memory audit */
    mystyle_get_global_gcs (Style, &gc1, &gc2, &gc3, &gc4);
    XFreeGC (dpy, gc1);
    XFreeGC (dpy, gc2);
    XFreeGC (dpy, gc3);
    XFreeGC (dpy, gc4);

    pixmap_ref_purge ();

    print_unfreed_mem ();
  }
#endif /* DEBUG_ALLOCS */

  exit (exitstat);
}

/******************************************************************************
  ChangeWindowName - Self explanitory
    Original work from Ident:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void
ChangeWindowName (char *str)
{
  XTextProperty name;
  if (XStringListToTextProperty (&str, 1, &name) == 0)
    {
      fprintf (stderr, "%s: cannot allocate window name.\n", MyName);
      return;
    }
  XSetWMName (dpy, window, &name);
  XSetWMIconName (dpy, window, &name);
  XFree (name.value);
}

/**************************************************************************
 *
 * Sets mwm hints
 *
 *************************************************************************/
/*
 *  Now, if we (hopefully) have MWW - compatible window manager ,
 *  say, mwm, ncdwm, or else, we will set useful decoration style.
 *  Never check for MWM_RUNNING property.May be considered bad.
 */
void
SetMwmHints (unsigned int value, unsigned int funcs, unsigned int input)
{
  PropMwmHints prop;

  if (MwmAtom == None)
    {
      MwmAtom = XInternAtom (dpy, "_MOTIF_WM_HINTS", False);
    }
  if (MwmAtom != None)
    {
      /* sh->mwm.decorations contains OR of the MWM_DECOR_XXXXX */
      prop.decorations = value;
      prop.functions = funcs;
      prop.inputMode = input;
      prop.flags = MWM_HINTS_DECORATIONS | MWM_HINTS_FUNCTIONS |
	MWM_HINTS_INPUT_MODE;

      /* HOP - LA! */
      XChangeProperty (dpy, window,
		       MwmAtom, MwmAtom,
		       32, PropModeReplace,
		       (unsigned char *) &prop,
		       PROP_MWM_HINTS_ELEMENTS);
    }
}

/* send_as_mesg:
 * send multiple  message to the pipe, have to be seperated by a ','
 */

void
send_as_mesg (const char *message, unsigned long id)
{
  char *delim;
  char *tmp_p, *tmp;

  tmp = (char *) safemalloc (strlen (message) + 1);
  strcpy (tmp, message);
  tmp_p = tmp;
  while ((delim = strchr (tmp_p, ',')) != NULL)
    {
      *delim = '\0';
      SendInfo (fd, tmp_p, id);
      tmp_p = delim + 1;
    }
  SendInfo (fd, tmp_p, id);
  free (tmp);
}

/* set_as_mask:
 * set the mask on the pipe
 */

void
set_as_mask (long unsigned mask)
{
  char set_mask_mesg[50];

  sprintf (set_mask_mesg, "SET_MASK %lu\n", mask);
  SendInfo (fd, set_mask_mesg, 0);
}

/* process_message:
 * process messages from the pipe.
 */

#define NONE 0
#define SOME 1
#define ALL 3
void
process_message (unsigned long type, unsigned long *body)
{
  int status = 0;
/*fprintf( stderr, "\n Winlist received AS message {%ld}, body[0] = %ld ;", type, body[0] ); */
  switch (type)
    {
    case M_CONFIGURE_WINDOW:
      status = list_configure (body);
      break;
    case M_ADD_WINDOW:
      status = list_add_window (body);
      break;
    case M_DESTROY_WINDOW:
      status = list_destroy_window (body);
      break;
    case M_ICONIFY:
      status = list_iconify (body);
      break;
    case M_DEICONIFY:
      status = list_deiconify (body);
      break;
    case M_WINDOW_NAME:
      status = list_window_name (body);
      break;
    case M_ICON_NAME:
      status = list_icon_name (body);
      break;
    case M_END_WINDOWLIST:
      status = list_end ();
      break;
    default:
      break;
    }

  if ((window == (Window) * (body + 1) || window == (Window) * body) &&
      type != M_CONFIGURE_WINDOW)
    status = 0;

  if (WindowIsUp && status)
    {
      if (buttons.count == 0 || !hidden)
	{
	  AdjustWindow ();
	  if (Style->texture_type >= TEXTURE_TRANSPARENT)
	    update_winlist_background ();
	  switch (status)
	    {
	    case ALL:
	      DrawButtonArray (Style, &buttons, 1);
	      break;
	    case SOME:
	      DrawButtonArray (Style, &buttons, 0);
	      break;
	    default:
	      break;
	    }
	}
    }
  if (hidden && WindowIsUp && status)
    update_hidden_winlist ();

}

/* list_configure:
 * pipe configure events
 */

int
list_configure (unsigned long *body)
{
  int ret;

  if ((window == (Window) body[1]) || (window == (Window) body[0])
      || ((body[19] != 0) && (window == (Window) body[19]))
      || ((body[19] != 0) && (window == (Window) body[20])))
    {
      ret = 0;
      if (current_desk != body[7])
	{
	  current_desk = body[7];
	  ret = SOME;
	}
      return ret;
    }
  else
    return (list_add_window (body));
}

/* list_add_window:
 * pipe add window events
 */

int
list_add_window (unsigned long *body)
{

  if (FindItem (&windows, body[0]) != -1)
    {
      if (UseSkipList && (body[8] & WINDOWLISTSKIP))
	return (list_destroy_window (body));
      return NONE;
    }

  if (!(body[8] & WINDOWLISTSKIP) || !UseSkipList)
    {
      AddItem (&windows, body[0], body[8]);
    }
  return NONE;
}

/* list_window_name:
 * pipe chage of window name events
 */

int
list_window_name (unsigned long *body)
{
  int i;
  char *name;
  char *string;

  if (UseIconNames)
    return NONE;

  if ((i = UpdateItemName (&windows, body[0], (char *) &body[3])) == -1)
    return NONE;

  string = (char *) &body[3];
  name = makename (string, ItemFlags (&windows, body[0]));
  if (UpdateButton (Style, &buttons, i, name, -1) == -1)
    {
      AddButton (Style, &buttons, name, 1);
    }
  free (name);
  return SOME;
}

/* list_icon_name:
 * pipe change of icon name events
 */

int
list_icon_name (unsigned long *body)
{
  int i;
  char *name;
  char *string;

  if (!UseIconNames)
    {
      return NONE;
    }

  if ((i = UpdateItemName (&windows, body[0], (char *) &body[3])) == -1)
    {
      return NONE;
    }

  string = (char *) &body[3];
  name = makename (string, ItemFlags (&windows, body[0]));
  if (UpdateButton (Style, &buttons, i, name, -1) == -1)
    {
      AddButton (Style, &buttons, name, 1);
    }
  free (name);
  if (buttons.count == 1 && hidden)
    unhide_winlist ();
  return SOME;
}

/* list_destroy_window:
 * pipe destroyed windows event
 */

int
list_destroy_window (unsigned long *body)
{
  int i;

  if ((i = DeleteItem (&windows, body[0])) == -1)
    return NONE;
  RemoveButton (&buttons, i);
  if (buttons.count == 0)
    hide_winlist (TRUE);
  return SOME;
}

/* list_end
 * end of window list from pipe
 */

int
list_end (void)
{
  AdjustWindow ();
  set_as_mask ((long unsigned) mask_reg);
  if (buttons.count == 0)
    hide_winlist (TRUE);
  return ALL;
}

/* list_deiconify:
 * window deiconified from the pipe
 */

int
list_deiconify (unsigned long *body)
{
  int i;
  long unsigned flags;
  char *string, *name;

  if ((i = FindItem (&windows, body[0])) == -1)
    return NONE;
  flags = ItemFlags (&windows, body[0]);

  if (!(flags & ICONIFIED))
    return NONE;
  flags ^= ICONIFIED;
  UpdateItemFlags (&windows, body[0], flags);
  string = ItemName (&windows, i);
  name = makename (string, flags);
  UpdateButton (Style, &buttons, i, name, -1);
  free (name);
  return SOME;
}

/* list_iconify:
 * window iconified from the pipe
 */

int
list_iconify (unsigned long *body)
{
  int i;
  long unsigned flags;
  char *string, *name;

  if ((i = FindItem (&windows, body[0])) == -1)
    return NONE;

  flags = ItemFlags (&windows, body[0]);

  if (flags & ICONIFIED)
    return NONE;
  flags ^= ICONIFIED;
  UpdateItemFlags (&windows, body[0], flags);
  string = ItemName (&windows, i);
  name = makename (string, flags);
  UpdateButton (Style, &buttons, i, name, -1);
  free (name);
  return SOME;
}

int
adjust_hide_geometry (int width, int height)
{
  long flags;
  unsigned int x, y, w, h;
  int bChanged = 0;
  static int bHideGeometrySet = 0;

  if (hide_geometry != NULL)
    {
      flags = XParseGeometry (hide_geometry, &x, &y, &w, &h);

      if (!(WidthValue & flags))
	w = width;
      if (!(HeightValue & flags))
	h = height;

      if (XValue & flags)
	{
	  if (XNegative & flags)
	    x = DisplayWidth (dpy, screen) + x - w;
	}
      else
	x = hide_x;

      if (YValue & flags)
	{
	  if (YNegative & flags)
	    y = DisplayHeight (dpy, screen) + y - h;
	}
      else
	y = hide_y;

      bHideGeometrySet = 1;
      free (hide_geometry);
      hide_geometry = NULL;

      if (hide_x == x && hide_y == y && hide_width == w && hide_height == h)
	return 0;
      hide_x = x;
      hide_y = y;
      hide_width = w;
      hide_height = h;
      bChanged++;
    }
  else if (!bHideGeometrySet)
    {
      if (hide_x != -10000 || hide_y != -10000)
	{
	  hide_x = -10000;
	  hide_y = -10000;
	  hide_width = width;
	  hide_height = height;
	  bChanged++;
	}
    }
  return bChanged;
}

void
DoHideWindow ()
{
  hidden = 1;
  bAdjustingWindow++;
  if (hide_width <= 0 || hide_height <= 0)
    XMoveWindow (dpy, window, hide_x, hide_y);
  else
    XMoveResizeWindow (dpy, window, hide_x, hide_y, hide_width, hide_height);
  update_winlist_background ();
}

void
make_hide_box (int fields, char **fieldname)
{
  int i;
  int width = 0, height = 0;

  for (i = 0; i < fields; i++)
    width += XTextWidth (Style->font.font, fieldname[i], strlen (fieldname[i])) + 3;
  height = Style->font.height + 4 + 2;

  if (adjust_hide_geometry (width, height) || !hidden)
    DoHideWindow ();

  /*  if (Balloons)
     {
     hide_balloon = balloon_new_with_text (dpy, window, "FIELDS FIELDS FIELDS");
     balloon_set_active_rectangle (hide_balloon, win_x, win_y, width, height);
     } */
}

void
draw_hide_boxes (int fields, char **field)
{
  int i;
  int x, y, text_width;
  int pre_x = 0, pre_y = 0;
  GC fore_gc;

  if (!fromExpose)
    XClearWindow (dpy, window);
  fromExpose = 0;

  mystyle_get_global_gcs (Style, &fore_gc, NULL, NULL, NULL);

  y = Style->font.height;
  if (y + 6 < hide_height)
    {
      pre_y = ((hide_height - y - 6) / 2) + y;
      y = hide_height - 6;
    }
  else
    pre_y = y;

  for (i = 0; i < fields; i++)
    {
      text_width = XTextWidth (Style->font.font, field[i], strlen (field[i]));
      x = (fields == 1) ? hide_width - 2 : text_width;

      XDrawRectangle (dpy, window, fore_gc, pre_x, 0, x, y + 4);
      if (fields == 1)
	pre_x = (x - text_width) / 2;
      mystyle_draw_text (window, Style, field[i], pre_x, pre_y);

      pre_x = pre_x + x + 2 + 1;
    }
}

void
hide_window ()
{
  char *names[1];

  names[0] = safemalloc (strlen (VERSION) + 11);
  strcpy (names[0], "AfterStep ");
  strcat (names[0], VERSION);

  make_hide_box (1, names);
  draw_hide_boxes (1, names);
  free (names[0]);

}
