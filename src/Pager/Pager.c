/*
 * Copyright (C) 1998 Eric Tremblay <deltax@pragma.net>
 * Copyright (c) 1998 Michal Vitecek <fuf@fuf.sh.cvut.cz>
 * Copyright (c) 1998 Doug Alcorn <alcornd@earthlink.net>
 * Copyright (c) 1998 Sasha Vasko <sasha at aftercode.net>
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

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/time.h>
#ifdef DO_CLOCKING
#include <time.h>
#endif

#ifdef ISC			/* Saul */
#include <sys/bsdtypes.h>	/* Saul */
#endif /* Saul */

#include <stdlib.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#ifdef I18N
#include <X11/Xlocale.h>
#endif

#define IN_MODULE
#define MODULE_X_INTERFACE

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"
#include "../../include/mystyle.h"
#include "../../include/background.h"

Pixmap GetRootPixmap (Atom);

#include "Pager.h"

#ifdef DEBUG_PAGER
#define LOG1(a)       fprintf( stderr, a );
#define LOG2(a,b)    fprintf( stderr, a, b );
#define LOG3(a,b,c)    fprintf( stderr, a, b, c );
#define LOG4(a,b,c,d)    fprintf( stderr, a, b, c, d );
#else
#define LOG1(a)
#define LOG2(a,b)
#define LOG3(a,b,c)
#define LOG4(a,b,c,d)
#endif

#define DEFAULT_BORDER_COLOR "grey50"

/*************************************************************************
 *
 * Screen, font, etc info
 *
 **************************************************************************/
PagerInfo Pager;		/* global data holding object */
PagerLook Look =
{
  {NULL, NULL, NULL, NULL, NULL},
  {NULL, NULL, NULL},
  NULL,
  1,
  1,
  0, None,
  0, 0,
  {NULL, NULL,
#ifdef I18N
   NULL,
#endif
   0, 0, 0},
  0
};
ASPipes Pipes;

ASAtom Atoms[] =
{
  {None, "WM_DELETE_WINDOW", None},
  XROOTPMAP_ID_ATOM_DEF,
  {None, "_AS_STYLE", XA_INTEGER},
  AS_BACKGROUND_ATOM_DEF,
  {None, NULL, None}
};

/*PagerWinAttr WinAttributes[WIN_TYPES_NUM] =
   {{0, None, None},
   {0, None, None},
   {0, None, None}};
 */

char *MyName;
ScreenInfo Scr;			/* AS compatible screen information structure */
Display *dpy;			/* which display are we talking to */
int screen;

int window_w = 0, window_h = 0, window_x = 0, window_y = 0, window_x_negative = 0,
  window_y_negative = 0;
int icon_x = -10000, icon_y = -10000, icon_w = 0, icon_h = 0;
int usposition = 0;

char *pixmapPath = NULL;
char *modulePath = NULL;

void
usage (void)
{
  printf ("Usage:\n"
	  "%s [--version] [--help] n m\n"
	  "%*s where desktops n through m are displayed\n"
	  ,MyName, (int) strlen (MyName), MyName);
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
  char *cptr;
  char *display_name = NULL;
  char *global_config_file = NULL;

  /* explicitly initializing parameters before everything else */
  Pager.bStarted = 0;		/* not started yet */
  Pager.CurentRootBack = 0;	/* no root background yet */
  Pager.Flags = PAGER_FLAGS_DEFAULT;

  /* Save our program name - for error messages */
  SetMyName (argv[0]);
  i = ProcessModuleArgs (argc, argv, &(global_config_file), NULL, NULL, usage);
  if (i == argc)
    usage ();


#ifdef I18N
  if (setlocale (LC_ALL, AFTER_LOCALE) == NULL)
    fprintf (stderr, "%s: cannot set locale.\n", MyName);
#endif

  LOG2 ("\n%s is starting ...", MyName)

    cptr = argv[i];
  while (isspace (*cptr))
    cptr++;
  Pager.desk1 = atoi (cptr);
  while (!(isspace (*cptr)) && (*cptr))
    cptr++;
  while (isspace (*cptr))
    cptr++;
  if (!(*cptr))
    cptr = argv[i + 1];

  if (cptr)
    Pager.desk2 = atoi (cptr);
  else
    Pager.desk2 = Pager.desk1;

  if (Pager.desk2 < Pager.desk1)
    {
      itemp = Pager.desk1;
      Pager.desk1 = Pager.desk2;
      Pager.desk2 = itemp;
    }
  Pager.ndesks = Pager.desk2 - Pager.desk1 + 1;

  Pager.Desks = (DeskInfo *) safemalloc (Pager.ndesks * sizeof (DeskInfo));
  Look.DeskStyles = (MyStyle **) safemalloc (Pager.ndesks * sizeof (MyStyle *));
  LOG2 ("\n%s is Initializing desks ...", MyName)
    for (i = 0; i < Pager.ndesks; i++)
    InitDesk (i);

  Pipes.fd_width = GetFdWidth ();

  LOG2 ("\n%s connecting to X ...", MyName)
    Pipes.x_fd = ConnectX (&Scr, display_name, PropertyChangeMask);
  screen = Scr.screen ;
  InitAtoms (dpy, Atoms);

  Pipes.fd[0] = Pipes.fd[1] = ConnectAfterStep (M_ADD_WINDOW |
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

  LOG2 ("\n%s is parsing Options ...", MyName)
    LoadBaseConfig (global_config_file, GetBaseOptions);
  LoadConfig (global_config_file, "pager", GetOptions);

#ifdef PAGER_BACKGROUND
  /* retrieve root bckgrounds data */
  Pager.CurentRootBack = GetRootPixmap (Atoms[ATOM_ROOT_PIXMAP].atom);
  RetrieveBackgrounds ();

  /* enable root pixmap property setting */
  SetRootPixmapPropertyID (Atoms[ATOM_ROOT_PIXMAP].atom);
#endif

  /* open a pager window */
  LOG2 ("\n%s is initializing windows ...", MyName)
    initialize_pager ();

  /* Create a list of all windows */
  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendInfo (Pipes.fd, "Send_WindowList", 0);

  LOG2 ("\n%s starting The Loop ...", MyName)
    while (1)
    {
      XEvent event;
      if (WaitASResponse > 0)
	{
	  ASMessage *msg = CheckASMessage (Pipes.fd[1], WAIT_AS_RESPONSE_TIMEOUT);
	  if (msg)
	    {
	      process_message (msg->header[1], msg->body);
	      DestroyASMessage (msg);
	    }
	}
      else
	{
	  if (!My_XNextEvent (dpy, Pipes.x_fd, Pipes.fd[1], process_message, &event))
	    timer_handle ();	/* handle timeout events */
	  else
	    {
	      balloon_handle_event (&event);
	      DispatchEvent (&event);
	    }
	}
    }

  return 0;
}

/***********************************************************************
 *
 *   Procedure:
 *   Process message - examines packet types, and takes appropriate action
 *
 ***********************************************************************/
void
process_message (unsigned long type, unsigned long *body)
{
  LOG4 ("\n%s: received AfterStep message #%ld, body[0]=%lu", MyName, type, body[0])
    switch (type)
    {
    case M_ADD_WINDOW:
      list_configure (body);
      break;
    case M_CONFIGURE_WINDOW:
      list_configure (body);
      break;
    case M_DESTROY_WINDOW:
      list_destroy (body);
      break;
    case M_FOCUS_CHANGE:
      list_focus (body);
      break;
    case M_NEW_PAGE:
      list_new_page (body);
      break;
    case M_NEW_DESK:
      list_new_desk (body);
      break;
    case M_NEW_BACKGROUND:
      list_new_background (body);
      break;
    case M_RAISE_WINDOW:
      list_raise (body);
      break;
    case M_LOWER_WINDOW:
      list_lower (body);
      break;
    case M_ICONIFY:
    case M_ICON_LOCATION:
      list_iconify (body);
      break;
    case M_DEICONIFY:
      list_deiconify (body);
      break;
    case M_ICON_NAME:
      list_icon_name (body);
      break;
    case M_END_WINDOWLIST:
      list_end ();
      break;
    default:
      list_unknown (body);
      return;
    }
  if (WaitASResponse > 0)
    WaitASResponse--;
}


/***********************************************************************
 *
 *   Procedure:
 *   SIGPIPE handler - SIGPIPE means afterstep is dying
 *
 ***********************************************************************/

void
DeadPipe (int nonsense)
{
#ifdef DEBUG_ALLOCS
  int i;
/* normally, we let the system clean up, but when auditing time comes
 * around, it's best to have the books in order... */
  {
    DeleteWindows (Pager.Start);
    for (i = 0; i < Pager.ndesks; i++)
      {
	if (Pager.Desks[i].label)
	  free (Pager.Desks[i].label);
	if (Pager.Desks[i].StyleName)
	  free (Pager.Desks[i].StyleName);
      }
    FreeDeskBackArray (&(Pager.Backgrounds), 0);
    free (Pager.Desks);
    LookCleanUp ();
    free (Look.DeskStyles);
    if (pixmapPath)
      free (pixmapPath);
    if (modulePath)
      free (modulePath);
    balloon_init (1);

    {
      GC foreGC, backGC, reliefGC, shadowGC;

      mystyle_get_global_gcs (mystyle_first, &foreGC, &backGC, &reliefGC, &shadowGC);
      while (mystyle_first)
	mystyle_delete (mystyle_first);
      XFreeGC (dpy, foreGC);
      XFreeGC (dpy, backGC);
      XFreeGC (dpy, reliefGC);
      XFreeGC (dpy, shadowGC);
    }

    print_unfreed_mem ();
  }

#endif /* DEBUG_ALLOCS */

  XFlush (dpy);			/* need this for SetErootPixmap to take effect */
  XCloseDisplay (dpy);		/* need this for SetErootPixmap to take effect */
  exit (0);
}

PagerWindow *
FindWindow (Window w)
{
  PagerWindow *t;
  if (w != None)
    for (t = Pager.Start; t != NULL; t = t->next)
      if (t->w == w)
	return t;

  return NULL;
}

void
DeleteWindows (PagerWindow * t)
{
  if (t != NULL)
    {
      DeleteWindows (t->next);
      /* remove window from the chain */
      DestroyView (t);
      free (t->icon_name);
      free (t);
      if (Pager.FocusWin == t)
	Pager.FocusWin = NULL;
    }
}

void
ConfigureWindow (PagerWindow * t, unsigned long *body)
{
  if (t && body)
    {
      t->frame = body[1];
      t->frame_x = body[3];
      t->frame_y = body[4];
      t->frame_width = body[5];
      t->frame_height = body[6];
      t->title_height = body[9];
      t->border_width = body[10];
      t->flags = body[8];
      t->icon_w = body[19];
      t->icon_pixmap_w = body[20];
      if (t->flags & ICONIFIED)
	{
	  t->x = t->icon_x;
	  t->y = t->icon_y;
	  t->width = t->icon_width;
	  t->height = t->icon_height;
	  if (t->flags & SUPPRESSICON)
	    {
	      t->x = -10000;
	      t->y = -10000;
	    }
	}
      else if (t->flags & SHADED)
	{
	  t->x = t->frame_x;
	  t->y = t->frame_y;
	  if (t->flags & VERTICAL_TITLE)
	    {
	      Window root;
	      int junk;
	      unsigned int ujunk;
	      XGetGeometry (dpy, t->frame, &root, &junk, &junk,
			    &(t->width), &(t->height), &ujunk, &ujunk);
	    }
	  else
	    {
	      t->width = t->frame_width;
	      t->height = t->title_height;
	    }
	}
      else
	{
	  t->x = t->frame_x;
	  t->y = t->frame_y;
	  t->width = t->frame_width;
	  t->height = t->frame_height;
	}
    }
}

/***********************************************************************
 *
 *   Procedure:
 *   list_add - displays packet contents to stderr
 *
 ***********************************************************************/
void
list_add (unsigned long *body)
{
  PagerWindow *t, **prev;

  t = Pager.Start;
  prev = &Pager.Start;
  while (t != NULL)
    {
      prev = &(t->next);
      t = t->next;
    }
  *prev = (PagerWindow *) safemalloc (sizeof (PagerWindow));
  (*prev)->w = body[0];
  (*prev)->desk = body[7];
  (*prev)->next = NULL;
  (*prev)->icon_name = NULL;

  ConfigureWindow (*prev, body);
  AddNewWindow (*prev);
}

/***********************************************************************
 *
 *   Procedure:
 *   list_configure - displays packet contents to stderr
 *
 ***********************************************************************/
void
list_configure (unsigned long *body)
{
  PagerWindow *t;

  if ((t = FindWindow (body[0])) == NULL)
    list_add (body);
  else
    {
      ConfigureWindow (t, body);
      if (t->desk == body[7])
	{
	  LOG3 ("\nMoveResizePagerView of [%s] from list_configure: desk = %d", t->icon_name, t->desk)
	    MoveResizePagerView (t);
	}
      else
	{
	  LOG4 ("\nChangeDeskForWindow of [%s] from list_configure: from %d to %ld", t->icon_name, t->desk, body[7]);
	  ChangeDeskForWindow (t, body[7]);
	}
      Hilight (t);
    }
}

/***********************************************************************
 *
 *   Procedure:
 *   list_destroy - displays packet contents to stderr
 *
 ***********************************************************************/
void
list_destroy (unsigned long *body)
{
  PagerWindow *t, **prev;
  Window target_w;

  target_w = body[0];
  t = Pager.Start;
  prev = &Pager.Start;
  while ((t != NULL) && (t->w != target_w))
    {
      prev = &(t->next);
      t = t->next;
    }
  if (t != NULL)
    {
      /* remove from the chain first */
      if (prev != NULL)
	*prev = t->next;
      t->next = NULL;
      DeleteWindows (t);
    }
}

/***********************************************************************
 *
 *   Procedure:
 *   list_focus - displays packet contents to stderr
 *
 ***********************************************************************/
void
list_focus (unsigned long *body)
{
  PagerWindow *t = FindWindow (body[0]), *temp;

  if (t != Pager.FocusWin && t != NULL)
    {
      temp = Pager.FocusWin;
      Pager.FocusWin = t;
      Hilight (temp);
      Hilight (Pager.FocusWin);
    }
}

/***********************************************************************
 *
 *   Procedure:
 *   list_new_page - displays packet contents to stderr
 *
 ***********************************************************************/
void
list_new_page (unsigned long *body)
{
  Scr.Vx = (long) body[0];
  Scr.Vy = (long) body[1];

  if (body[2] != 10000)
    {
      LOG4 ("\nlist_new_page: desk = %d, Vx=%d, Vy=%d", Scr.CurrentDesk, Scr.Vx, Scr.Vy)
	MovePage ();
      MoveStickyWindows ();
    }
}

/***********************************************************************
 *
 *   Procedure:
 *   list_new_desk - displays packet contents to stderr
 *
 ***********************************************************************/
void
list_new_desk (unsigned long *body)
{
  int oldDesk = Scr.CurrentDesk;

  Scr.CurrentDesk = (long) body[0];
  if (oldDesk != 10000)		/* always dehilight old desk */
    HilightDesk (oldDesk - Pager.desk1, -1);

  if (body[0] != 10000)
    {
      LOG3 ("\nlist_new_desk(New=%d, Old=%d):\t", Scr.CurrentDesk, oldDesk)

	LOG1 ("\n\t\t\t\t MovePage()")
	MovePage ();		/* icon  stuff moving */
      if (Scr.CurrentDesk >= Pager.desk1 && Scr.CurrentDesk <= Pager.desk2)
	{
#ifdef PAGER_BACKGROUND
	  if (Pager.Flags & REDRAW_BG)
	    BackgroundSetForDesk (&(Pager.Backgrounds), Scr.CurrentDesk);
#endif
	  /* hilight New Desk */
	  HilightDesk (Scr.CurrentDesk - Pager.desk1, -1);
	}
      MoveStickyWindows ();
    }

  LOG2 ("\n%s:list_new_desk():\tDone!", MyName)
}

/***********************************************************************
 *
 *   Procedure:
 *         list_new_background - reread config file & reconfigure on fly
 *
 ***********************************************************************/
void BackgroundSetCommand (char *cmd);
void KillOldDrawing ();

void
list_new_background (unsigned long *body)
{
/* need to add code here to rerun background loading module/app */
#ifdef PAGER_BACKGROUND
  if (Pager.Flags & REDRAW_BG &&
      Scr.CurrentDesk >= Pager.desk1 &&
      Scr.CurrentDesk <= Pager.desk2)
    {
      char *cmd, *arg0;
      KillOldDrawing ();
      if ((arg0 = findIconFile ("asetroot", modulePath, X_OK)) == NULL)
	{
	  fprintf (stderr, "\n%s: cannot find asetroot to set new background.", MyName);
	  return;
	}
      cmd = (char *) safemalloc (strlen (arg0) + 1 + 5 + 16 + 16);
      sprintf (cmd, "%s -l %d %d", arg0, Pager.desk1, Pager.desk2);
      BackgroundSetCommand (cmd);
      free (cmd);
      free (arg0);
      sleep (1);
    }
#endif
}

/***********************************************************************
 *
 *   Procedure:
 *   list_raise - displays packet contents to stderr
 *
 ***********************************************************************/
void
list_raise (unsigned long *body)
{
  PagerWindow *t;

  if ((t = FindWindow (body[0])) != NULL)
    {
      if (t->PagerView != None)
	XRaiseWindow (dpy, t->PagerView);
      if (t->IconView != None)
	XRaiseWindow (dpy, t->IconView);
    }
}

/***********************************************************************
 *
 *   Procedure:
 *   list_lower - displays packet contents to stderr
 *
 ***********************************************************************/
void
list_lower (unsigned long *body)
{
  PagerWindow *t;

  if ((t = FindWindow (body[0])) != NULL)
    {
      if (t->PagerView != None)
	{
	  XLowerWindow (dpy, t->PagerView);
	  if (t->desk == Scr.CurrentDesk)
	    LowerFrame (1);
	}
      if (t->IconView != None)
	XLowerWindow (dpy, t->IconView);
    }
}


/***********************************************************************
 *
 *   Procedure:
 *   list_unknow - handles an unrecognized packet.
 *
 ***********************************************************************/
void
list_unknown (unsigned long *body)
{
  /*   fprintf(stderr,"Unknown packet type\n"); */
}

/***********************************************************************
 *
 *   Procedure:
 *   list_iconify - displays packet contents to stderr
 *
 ***********************************************************************/
void
list_iconify (unsigned long *body)
{
  PagerWindow *t;
  if ((t = FindWindow (body[0])) == NULL)
    return;

  t->frame = body[1];
  t->icon_x = body[3];
  t->icon_y = body[4];
  t->icon_width = body[5];
  t->icon_height = body[6];
  t->flags |= ICONIFIED;
  if (t->flags & SUPPRESSICON)
    {
      t->x = -10000;
      t->y = -10000;
    }
  else
    {
      t->x = t->icon_x;
      t->y = t->icon_y;
    }
  t->width = t->icon_width;
  t->height = t->icon_height;
  MoveResizePagerView (t);
}


/***********************************************************************
 *
 *   Procedure:
 *   list_deiconify - displays packet contents to stderr
 *
 ***********************************************************************/

void
list_deiconify (unsigned long *body)
{
  PagerWindow *t;
  if ((t = FindWindow (body[0])) != NULL)
    {
      t->flags &= ~ICONIFIED;
      t->x = t->frame_x;
      t->y = t->frame_y;
      t->width = t->frame_width;
      t->height = t->frame_height;
      MoveResizePagerView (t);
      Hilight (t);
    }
}


/***********************************************************************
 *
 *   Procedure:
 *   list_icon_name - displays packet contents to stderr
 *
 ***********************************************************************/
void
list_icon_name (unsigned long *body)
{
  PagerWindow *t;
  Balloon *balloon;

  if ((t = FindWindow (body[0])) != NULL)
    {
      if (t->icon_name != NULL)
	free (t->icon_name);
      CopyString (&t->icon_name, (char *) (&body[3]));

      if (t->PagerView != None)
	{
	  LabelWindow (t);
	  if ((balloon = balloon_find (t->PagerView)) != NULL)
	    balloon_set_text (balloon, t->icon_name);
	}
      if (t->IconView != None)
	{
	  if ((balloon = balloon_find (t->IconView)) != NULL)
	    balloon_set_text (balloon, t->icon_name);
	  LabelIconWindow (t);

	}
    }
}

/***********************************************************************
 *
 *   Procedure:
 *   list_end - displays packet contents to stderr
 *
 ***********************************************************************/
void
list_end (void)
{
  unsigned int nchildren, i;
  Window root, parent, *children;
  PagerWindow *ptr;

  if (XQueryTree (dpy, Scr.Root, &root, &parent, &children, &nchildren))
    {
      for (i = 0; i < nchildren; i++)
	for (ptr = Pager.Start; ptr != NULL; ptr = ptr->next)
	  if ((ptr->frame == children[i]) || (ptr->icon_w == children[i]) ||
	      (ptr->icon_pixmap_w == children[i]))
	    {
	      if (ptr->PagerView != None)
		XRaiseWindow (dpy, ptr->PagerView);
	      if (ptr->IconView != None)
		XRaiseWindow (dpy, ptr->IconView);
	    }

      if (nchildren > 0 && children)
	XFree ((char *) children);
    }
}

/*****************************************************************************
 *
 * This routine is responsible for reading Styles from shared property
 * or config file
 *
 ****************************************************************************/

char *PagerStyleNames[] =
{"*%sFWindowStyle", "*%sSWindowStyle", "*%sUWindowStyle",
 "*%sActiveDesk", "*%sInActiveDesk"};
char *PagerDefaultStyleNames[] =
{
  "focused_window_style",
  "sticky_window_style",
  "unfocused_window_style",
  NULL,
  NULL};
char *PagerDefaultColors[][2] =
{
  {"white", "blue"},
  {"white", "green"},
  {"white", "Gray40"},
  {"Gray30", "Gray70"},
  {"Gray70", "Gray30"}};

#define STYLE_MAX_NAME_LEN  12

void
LookCleanUp ()
{
  int i;
/* flashing allocated resources first */
  if (Look.GridGC)
    {
      XFreeGC (dpy, Look.GridGC);
      Look.GridGC = None;
    }

  for (i = 0; i < WIN_TYPES_NUM; i++)
    {
      if (Look.windowFont.font)
	if (Look.WinForeGC[i])
	  XFreeGC (dpy, Look.WinForeGC[i]);

      Look.WinForeGC[i] = None;
    }
}

void
GetLook (FILE * fd)
{
  static int need_to_setup_balloons = 1;
  int i;
  char *buf = safemalloc (1 + strlen (MyName) + STYLE_MAX_NAME_LEN + 1);
  MyStyle *Style;

  LookCleanUp ();

  /* reading new styles information */
  mystyle_get_property (dpy, Scr.Root, Atoms[ATOM_STYLES].atom, Atoms[ATOM_STYLES].type);
  for (i = 0; i < STYLE_MAX_STYLE; i++)
    {
      /* create our style */
      sprintf (buf, PagerStyleNames[i], MyName);
      if ((Style = mystyle_find (buf)) == NULL && PagerDefaultStyleNames[i])
	Style = mystyle_find (PagerDefaultStyleNames[i]);
      LOG4 ("\n%s: %s Style is %s", MyName, buf, Style ? "Found" : "NotFound")
	if (!Style)
	{
	  Style = mystyle_new_with_name (buf);
	  Look.bNeedToFixStyles = 1;
	}
      if (!(Style->set_flags & F_FORECOLOR))
	{
	  Style->colors.fore = GetColor (PagerDefaultColors[i][0]);
	  Style->set_flags |= F_FORECOLOR;
	}
      if (!(Style->set_flags & F_BACKCOLOR))
	{
	  Style->colors.back = GetColor (PagerDefaultColors[i][1]);
	  Style->set_flags |= F_BACKCOLOR;
	}
#ifdef I18N
      if (!(Style->set_flags & F_FONT))
	{
	  load_font (NULL, &Style->font);
	  Style->set_flags |= F_FONT;
	}
#endif
      Look.Styles[i] = Style;
    }
  free (buf);

/* looking for desk styles */
  for (i = 0; i < Pager.ndesks; i++)
    {
      if (Pager.Desks[i].StyleName == NULL)
	{			/* trying default here - which is "*PagerDesk#" */
	  char *def_style = safemalloc (1 + strlen (MyName) + 4 + 16);
	  sprintf (def_style, "*%sDesk%d", MyName, i + Pager.desk1);
	  if ((Style = mystyle_find (def_style)) == NULL)
	    {			/* if it's not present - then use scaled root pixmap */
	      free (def_style);
	      continue;
	    }
	  Pager.Desks[i].StyleName = def_style;
	}
      else if ((Style = mystyle_find (Pager.Desks[i].StyleName)) == NULL)
	{
	  Style = mystyle_new_with_name (Pager.Desks[i].StyleName);
	  Look.bNeedToFixStyles = 1;
	}
      Look.DeskStyles[i] = Style;
    }

  if (Look.bNeedToFixStyles)
    {
      mystyle_fix_styles ();
      Look.bNeedToFixStyles = 0;
    }

  /* initialize balloons if necessary */
  if (need_to_setup_balloons)
    {
      balloon_setup (dpy);
      need_to_setup_balloons = 0;
    }
  balloon_set_style (dpy, mystyle_find_or_default ("*PagerBalloon"));
}

int
GetFontHeight (MyStyle * style)
{
  int height = 0;
  if (style)
    if (style->set_flags & F_FONT)
      {
	height = style->font.height;
	switch (style->text_style)
	  {
	  case 1:
	    height += 4;
	    break;
	  case 2:
	    height += 2;
	    break;
	  }

      }
  return height;
}

void
FixLook ()
{
  GC gc;
  XGCValues gcvalues;
  int i;
  extern int label_h;

  label_h = 0;
  if (Pager.Flags & USE_LABEL)
    for (i = 0; i < Pager.ndesks; i++)
      if (Pager.Desks[i].label != NULL)		/* display label only if we have any labels specifyed */
	{
	  int active_h = GetFontHeight (Look.Styles[STYLE_ADESK]);
	  int inactive_h = GetFontHeight (Look.Styles[STYLE_INADESK]);

	  label_h = (active_h < inactive_h) ? inactive_h : active_h;
	  if (label_h > 0)
	    label_h += 2;
	  break;
	}

  if (Look.GridGC == None)
    {
      XGCValues gcvalues;
      if (!(Pager.Flags & DIFFERENT_GRID_COLOR))
	Look.GridColor = (Look.Styles[STYLE_INADESK]->colors).fore;

      gcvalues.foreground = Look.GridColor;
      Look.GridGC = XCreateGC (dpy, Scr.Root, GCForeground, &gcvalues);
    }

  if (Look.windowFont.font)
    gcvalues.font = Look.windowFont.font->fid;

  for (i = 0; i < WIN_TYPES_NUM; i++)
    {
      if (!Look.WinForeGC[i])
	{
	  mystyle_get_global_gcs (Look.Styles[i], &gc, NULL, NULL, NULL);

	  if (Look.windowFont.font)
	    {
	      Look.WinForeGC[i] = XCreateGC (dpy, Scr.Root, GCFont, &gcvalues);
	      XCopyGC (dpy, gc, ~GCFont, Look.WinForeGC[i]);
	    }
	  else
	    Look.WinForeGC[i] = None;
	}
    }
}

void
ApplyLook ()
{
  PagerWindow *t;
  long Desk = 0;
  for (; Desk < Pager.ndesks; Desk++)
    if (Look.DeskStyles[Desk])
      DecorateDesk (Desk);

  DecoratePager ();
  for (t = Pager.Start; t != NULL; t = t->next)
    Hilight (t);
}

void
OnLookUpdated ()
{
  extern int label_h;
  int old_label_h = label_h, old_border_width = Look.DeskBorderWidth;
  int new_window_w = window_w, new_window_h = window_h;

  GetLook (NULL);
  FixLook ();

  if (old_label_h != label_h)
    new_window_h += (label_h - old_label_h) * Pager.Rows;

  if (old_border_width != Look.DeskBorderWidth)
    {
      new_window_h += (Look.DeskBorderWidth - old_border_width) * Pager.Rows * 2;
      new_window_w += (Look.DeskBorderWidth - old_border_width) * Pager.Rows * 2;
    }

  if (window_w != new_window_w || window_h != new_window_h)
    {
      XResizeWindow (dpy, Pager.Pager_w, new_window_w, new_window_h);
/*      ReConfigureEx(new_window_w, new_window_h); */
    }
  else
    ApplyLook ();
}

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/

void
GetOptions (const char *filename)
{
  PagerConfig *config = ParsePagerOptions (filename, MyName, Pager.desk1, Pager.desk2);
  int i;
#ifdef DO_CLOCKING
  clock_t started = clock ();
#endif

  if (!config)
    exit (0);			/* something terrible has happend */
/* example of using config writer :
 */
/*   WritePagerOptions( filename, MyName, Pager.desk1, Pager.desk2, config, WF_DISCARD_UNKNOWN|WF_DISCARD_COMMENTS );
 */

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
  if (config->icon_geometry.flags & WidthValue)
    icon_w = config->icon_geometry.width;
  if (config->icon_geometry.flags & HeightValue)
    icon_h = config->icon_geometry.height;
  if (config->icon_geometry.flags & XValue)
    icon_x = config->icon_geometry.x;
  if (config->icon_geometry.flags & YValue)
    icon_y = config->icon_geometry.y;

  for (i = 0; i < Pager.ndesks; i++)
    {
      if (Pager.Desks[i].label)
	free (Pager.Desks[i].label);
      Pager.Desks[i].label = config->labels[i];
    }
#ifdef PAGER_BACKGROUND
  for (i = 0; i < Pager.ndesks; i++)
    {
      if (Pager.Desks[i].StyleName)
	free (Pager.Desks[i].StyleName);
      Pager.Desks[i].StyleName = config->styles[i];
    }
#endif
  Pager.Flags = config->flags;
  Look.TitleAlign = config->align;
  Pager.Rows = config->rows;
  Pager.Columns = config->columns;
  if (config->small_font_name)
    {
      load_font (config->small_font_name, &(Look.windowFont));
      free (config->small_font_name);
    }
  else
    Look.windowFont.font = NULL;

  if (config->selection_color)
    {
      Look.SelectionColor = GetColor (config->selection_color);
      free (config->selection_color);
    }else
      Look.SelectionColor = GetColor (DEFAULT_BORDER_COLOR);

  if (config->grid_color)
    {
      Look.GridColor = GetColor (config->grid_color);
      free (config->grid_color);
    }else
      Look.GridColor = GetColor (DEFAULT_BORDER_COLOR);
  if (config->border_color)
    {
      Look.BorderColor = GetColor (config->border_color);
      free (config->border_color);
    }else
      Look.BorderColor  = GetColor (DEFAULT_BORDER_COLOR);

  Look.DeskBorderWidth = config->border_width;

  if (config->style_defs)
    ProcessMyStyleDefinitions (&(config->style_defs), pixmapPath);

  DestroyPagerConfig (config);

  LOG1 ("\n GetLook()")
    GetLook (NULL);

  LOG1 ("\n FixLook()")
    FixLook ();

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

  pixmapPath = config->pixmap_path;
  replaceEnvVar (&pixmapPath);
  modulePath = config->module_path;
  replaceEnvVar (&modulePath);
  config->pixmap_path = NULL;	/* setting it to NULL so it would not be
				   deallocated by DestroyBaseConfig */
  config->module_path = NULL;

  if (config->desktop_size.flags & WidthValue)
    Pager.PageColumns = config->desktop_size.width;
  if (config->desktop_size.flags & HeightValue)
    Pager.PageRows = config->desktop_size.height;

  Scr.VScale = config->desktop_scale;

  DestroyBaseConfig (config);

  Scr.MyDisplayWidth = DisplayWidth (dpy, Scr.screen);
  Scr.MyDisplayHeight = DisplayHeight (dpy, Scr.screen);
  Scr.Vx = 0;
  Scr.Vy = 0;

  Scr.VxMax = (Pager.PageColumns - 1) * Scr.MyDisplayWidth;
  Scr.VyMax = (Pager.PageRows - 1) * Scr.MyDisplayHeight;
  Pager.xSize = Scr.VxMax + Scr.MyDisplayWidth;
  Pager.ySize = Scr.VyMax + Scr.MyDisplayHeight;

#ifdef DO_CLOCKING
  fprintf (stderr, "\n Base Config parsing time (clocks): %lu\n", clock () - started);
#endif

}
