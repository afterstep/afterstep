/*
 * Copyright (c) 1998 Michal Vitecek <M.Vitecek@sh.cvut.cz>
 * Copyright (c) 1998 Sasha Vasko <sasha at aftercode.net>
 * Copyright (C) 1998 Ethan Fischer
 * Copyright (C) 1998 Guylhem Aznar
 * Copyright (C) 1996 Alfredo K. Kojima
 * Copyright (C) 1996 Beat Christen
 * Copyright (C) 1996 Kaj Groner
 * Copyright (C) 1996 Frank Fejes
 * Copyright (C) 1996 mj@dfv.rwth-aachen.de
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

#define TRUE 1
#define FALSE 0
#define DOUBLECLICKTIME 1

#include "../../configure.h"

#ifdef ISC
#include <sys/bsdtypes.h>	/* Saul */
#endif

#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif
#ifdef I18N
#include <X11/Xlocale.h>
#endif

#include "Wharf.h"
#include "../../include/iconbg.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */


#ifdef ENABLE_DND
#include "OffiX/DragAndDrop.h"
#include "OffiX/DragAndDropTypes.h"
#endif


#ifdef ENABLE_SOUND
#define WHEV_PUSH		0
#define WHEV_CLOSE_FOLDER	1
#define WHEV_OPEN_FOLDER	2
#define WHEV_CLOSE_MAIN		3
#define WHEV_OPEN_MAIN		4
#define WHEV_DROP		5
#define MAX_EVENTS		6

int SoundActive = 0;
char *Sounds[6] =
{".", ".", ".", ".", ".", "."};
char *SoundPlayer = NULL;
char *SoundPath = ".";

pid_t SoundThread;
int PlayerChannel[2];

char *ModulePath = AFTER_DIR;
#endif

/* exported to aslib */
char *MyName;
Display *dpy;

int x_fd, fd_width;
int ROWS = FALSE;
int Rows = 1, Columns = 1;

ScreenInfo Scr;

/* absolete - stored in Scr
 *Window Root;
 *GC NormalGC, HiReliefGC ;

 *long d_depth;
 */

int flags;
Bool DoWithdraw = 1;
Bool NoBorder = 0;
Bool Pushed = 0;
Bool Pushable = 1;
int ForceWidth = 0, ForceHeight = 0;

int AnimationStyle = 0, AnimateMain = 0;
int PushStyle = 0;
int AnimationDir = 1;

int Width, Height, win_x, win_y;
unsigned int display_width, display_height;

#define MW_EVENTS   (ExposureMask | ButtonReleaseMask |\
		     ButtonPressMask | LeaveWindowMask | EnterWindowMask |\
		     StructureNotifyMask)
int num_folderbuttons = MAX_BUTTONS;
int max_icon_width = 30, max_icon_height = 0;
int root_x = -100000, root_y = -100000, root_w = -1, root_h = -1, root_gravity = NorthWestGravity;
int new_desk = 0;
int pageing_enabled = 1;
int ready = 0;

int fd[2];

button_info *new_button (button_info * button);
void delete_button (button_info * button);
void update_folder_shape (folder_info * folder);

folder_info *new_folder (void);
void delete_folder (folder_info * folder);
folder_info *first_folder = NULL;
folder_info *root_folder;	/* the main wharf */
folder_info *current_folder;

ASImage *back_pixmap = NULL;
ASImageManager *imman =  NULL;
char *iconPath = NULL;
char *pixmapPath = NULL;

Atom _XA_WM_DELETE_WINDOW;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_NAME;
Atom _XROOTPMAP_ID;
Atom _AS_STYLE;
#ifdef ENABLE_DND
Atom DndProtocol;
Atom DndSelection;
#endif

MyStyle *Style = NULL;
int Withdrawn;
int CornerX, CornerY;		/* location of withdrawn Wharf window */
int AnimateSteps = 1;
int AnimateStepsMain = 1;
int AnimateDelay = 0;

#define DIR_TOLEFT	1
#define DIR_TORIGHT	2
#define DIR_TOUP	3
#define DIR_TODOWN	4

int WharfXNextEvent (Display * dpy, XEvent * event);

#ifdef ENABLE_SOUND
void
waitchild (int bullshit)
{
  int stat;

  wait (&stat);
  SoundActive = 0;
}

#endif

unsigned int lock_mods[256];
void FindLockMods (void);

/*#define DEBUG */
#ifdef DEBUG
void print_folder_hierarchy (folder_info * folder);
Bool is_folder_consistent (folder_info * folder);
#endif /*DEBUG */

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
	  "%s [-f [config file]] [-v|--version] [-h|--help]\n", MyName);
  exit (0);
}

/***********************************************************************
 *
 *  Procedure:
 *	main - start of afterstep
 *
 ***********************************************************************
 */
int
main (int argc, char **argv)
{
  char set_mask_mesg[50];
  char configfile[255];
  char *realconfigfile;
  char *temp;
  int i;
  char *global_config_file = NULL;

  /* Save our program name - for error messages */
  temp = strrchr (argv[0], '/');
  MyName = temp ? temp + 1 : argv[0];
  set_application_name( argv[0] );

  for (i = 1; i < argc && *argv[i] == '-'; i++)
    {
      if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
	usage ();
      else if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "--version"))
	version ();
      else if (!strcmp (argv[i], "-w") || !strcmp (argv[i], "--window"))
	i++;
      else if (!strcmp (argv[i], "-c") || !strcmp (argv[i], "--context"))
	i++;
      else if (!strcmp (argv[i], "-f") && i + 1 < argc)
	global_config_file = argv[++i];
    }

  /* Dead pipe == dead AfterStep */
  signal (SIGPIPE, DeadPipe);
  set_signal_handler( SIGSEGV );
  x_fd = ConnectX( &Scr, display_name, PropertyChangeMask);

  /* connect to AfterStep */
  temp = module_get_socket_property (RootWindow (dpy, Scr.screen));
  fd[0] = fd[1] = module_connect (temp);
  XFree (temp);
  if (fd[0] < 0)
    {
      fprintf (stderr, "%s: unable to establish connection to AfterStep\n", MyName);
      exit (1);
    }
  temp = safemalloc (9 + strlen (MyName) + 1);
  sprintf (temp, "SET_NAME %s", MyName);
  SendInfo (fd, temp, None);
  free (temp);

#ifdef I18N
  if (setlocale (LC_CTYPE, AFTER_LOCALE) == NULL)
    fprintf (stderr, "%s: cannot set locale\n", MyName);
#endif

  root_folder = new_folder ();
  first_folder = root_folder;
  /* need to set current_folder for ParseOptions */
  current_folder = root_folder;

  fd_width = GetFdWidth ();

  XSetErrorHandler (ASErrorHandler);

  _XA_WM_DELETE_WINDOW = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
  _XA_WM_PROTOCOLS = XInternAtom (dpy, "WM_PROTOCOLS", False);
  _XROOTPMAP_ID = XInternAtom (dpy, "_XROOTPMAP_ID", False);
  _AS_STYLE = XInternAtom (dpy, "_AS_STYLE", False);
#ifdef ENABLE_DND
  DndProtocol = XInternAtom (dpy, "DndProtocol", False);
  DndSelection = XInternAtom (dpy, "DndSelection", False);
#endif

  display_width = Scr.MyDisplayWidth ;
  display_height = Scr.MyDisplayHeight ;

  sprintf (set_mask_mesg, "SET_MASK %lu\n",
	   (unsigned long) (M_TOGGLE_PAGING |
			    M_NEW_DESK |
			    M_END_WINDOWLIST |
			    M_MAP |
			    M_RES_NAME |
			    M_RES_CLASS |
			    M_WINDOW_NAME));

  SendInfo (fd, set_mask_mesg, 0);

  mystyle_get_property (dpy, Scr.Root, _AS_STYLE, XA_INTEGER);

  if (global_config_file != NULL)
    {
      ParseBaseOptions (global_config_file);
      ParseOptions (global_config_file);
    }
  else
    {
      sprintf (configfile, "%s/base.%dbpp", AFTER_DIR, DefaultDepth (dpy, Scr.screen));
      realconfigfile = (char *) PutHome (configfile);
      if (CheckFile (realconfigfile) == -1)
	{
	  free (realconfigfile);
	  sprintf (configfile, "%s/base.%dbpp", AFTER_SHAREDIR, DefaultDepth (dpy, Scr.screen));
	  realconfigfile = PutHome (configfile);
	}
      ParseBaseOptions (realconfigfile);
      free (realconfigfile);

      sprintf (configfile, "%s/wharf", AFTER_DIR);
      realconfigfile = (char *) PutHome (configfile);

      if ((CheckFile (realconfigfile)) == -1)
	{
	  free (realconfigfile);
	  sprintf (configfile, "%s/wharf", AFTER_SHAREDIR);
	  realconfigfile = PutHome (configfile);
	}
      ParseOptions (realconfigfile);
      free (realconfigfile);
    }

  /* fix up any unset (but necessary) style fields */
  mystyle_fix_styles ();

  balloon_setup (dpy);
  balloon_set_style (dpy, mystyle_find_or_default ("*WharfBalloon"));

  if ((*root_folder).count == 0)
    {
      fprintf (stderr, "%s: No Buttons defined. Quitting\n", MyName);
      exit (0);
    }
#ifdef ENABLE_SOUND
  /* startup sound subsystem */
  if (SoundActive)
    {
      if (pipe (PlayerChannel) < 0)
	{
	  fprintf (stderr, "%s: could not create pipe. Disabling sound\n", MyName);
	  SoundActive = 0;
	}
      else
	{
	  signal (SIGCHLD, waitchild);
	  SoundThread = fork ();
	  if (SoundThread < 0)
	    {
	      fprintf (stderr, "%s: could not fork(). Disabling sound",
		       MyName);
	      perror (".");
	      SoundActive = 0;
	    }
	  else if (SoundThread == 0)
	    {			/* in the sound player process */
	      char *margv[9], *name;
	      int i;

	      margv[0] = "ASSound";
	      name = findIconFile ("ASSound", ModulePath, X_OK);
	      if (name == NULL)
		{
		  fprintf (stderr, "Wharf: couldn't find ASSound\n");
		  SoundActive = 0;
		}
	      else
		{
		  margv[1] = safemalloc (16);
		  close (PlayerChannel[1]);
		  sprintf (margv[1], "%x", PlayerChannel[0]);
		  if (SoundPlayer != NULL)
		    margv[2] = SoundPlayer;
		  else
		    margv[2] = "-";
		  for (i = 0; i < MAX_EVENTS; i++)
		    {
		      if (Sounds[i][0] == '.')
			{
			  margv[i + 3] = Sounds[i];
			}
		      else
			{
			  margv[i + 3] = safemalloc (strlen (Sounds[i])
						  + strlen (SoundPath) + 4);
			  sprintf (margv[i + 3], "%s/%s", SoundPath, Sounds[i]);
			}
		    }
		  margv[i + 3] = NULL;
		  execvp (name, margv);
		  fprintf (stderr, "Wharf: couldn't spawn ASSound\n");
		}
	      exit (1);
	    }
	  else
	    {			/* in parent */
	      close (PlayerChannel[0]);
	    }
	}
    }
#endif
  back_pixmap = NULL;
  CreateIconPixmap ();

  /* set up folders and create windows */
  CreateWindow ();

  OpenFolder (root_folder);

#ifdef DEBUG
  /* check to make sure the root_folder is self-consistent! */
  if (is_folder_consistent (root_folder) == False)
    printf ("Consistency check failed!\n");
  print_folder_hierarchy (root_folder);
#endif /*DEBUG */

  FindLockMods ();

  /* request a window list, since this triggers a response which
   * will tell us the current desktop and paging status, needed to
   * indent buttons correctly */
  SendInfo (fd, "Send_WindowList", 0);
  Loop ();
  return 0;
}

/***********************************************************************
 *
 *  Procedure:
 *	find_folder - find the folder corresponding to a window
 *  returns the matched folder, or NULL if no matching folder was found
 *
 ***********************************************************************/

folder_info *
find_folder (Window w)
{
  folder_info *folder;

  for (folder = first_folder; folder != NULL; folder = (*folder).next)
    {
      if (w == (*folder).win)
	return folder;
      else
	{
	  button_info *button;
	  for (button = (*folder).first; button != NULL; button = (*button).next)
	    if ((w == (*button).IconWin) || (w == (*button).swallowed_win))
	      return folder;
	}
    }
  return NULL;
}

/***********************************************************************
 *
 *  Procedure:
 *	find_button - find the button a ButtonPress happened in
 *  returns the matched button, or NULL if no matching button was found
 *
 ***********************************************************************/

button_info *
find_button (Window w, int x, int y)
{
  folder_info *folder;
  button_info *button;

  for (folder = first_folder; folder != NULL; folder = (*folder).next)
    {
      if (w == (*folder).win)
	{
	  for (button = (*folder).first; button != NULL; button = (*button).next)
	    if ((x >= (*button).x) && (y >= (*button).y))
	      if ((x < (*button).x + (*button).width) && (y < (*button).y + (*button).height))
		return button;
	}
      else
	{
	  for (button = (*folder).first; button != NULL; button = (*button).next)
	    if ((w == (*button).IconWin) || (w == (*button).swallowed_win))
	      return button;
	}
    }
  return NULL;
}

/***********************************************************************
 *
 *  Procedure:
 *	unmap_folders - unmap mapped subfolders
 *
 ***********************************************************************/

void
unmap_folders (folder_info * folder)
{
  button_info *button;

  for (button = (*folder).first; button != NULL; button = (*button).next)
    {
      if ((*button).folder != NULL)
	{
	  unmap_folders ((*button).folder);
	  if (get_flags (button->folder->flags, WF_Mapped))
	    CloseFolder ((*button).folder);
	}
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	Loop - wait for data to process
 *
 ***********************************************************************/

void
Loop (void)
{
  button_info *CurrentButton = NULL;
  button_info *NewButton;
  XEvent Event;
  int bl = -1;
  time_t t, tl = (time_t) 0;
  int CancelPush = 0;
  folder_info *folder;
  button_info *button;

  while (1)
    {
      if (WharfXNextEvent (dpy, &Event) && !balloon_handle_event (&Event))
	{
	  switch (Event.type)
	    {
	    case Expose:
#if 0		
	      for (folder = first_folder; folder != NULL; folder = (*folder).next)
		if (Event.xany.window == (*folder).win)
		  {
		    RedrawWindow (folder, NULL);
		    for (button = (*folder).first; button != NULL; button = (*button).next)
		      RedrawUnpushedOutline (button);
		  }
#endif		  
	      break;

	    case ButtonPress:
	      if (Event.xbutton.button != Button1)
		{
		  if (Event.xbutton.button == Button3 &&
		      (button = find_button (Event.xbutton.window, Event.xbutton.x, Event.xbutton.y)) != NULL &&
		      (folder = button->parent) != NULL &&
		      folder->parent == NULL &&
		      (DoWithdraw == 1 || (DoWithdraw == 2 && (folder->first == button || button->next == NULL))))
		    {
		      unmap_folders (root_folder);
		      if (Withdrawn)
			{
			  Withdrawn = 0;
#ifdef ENABLE_SOUND
			  PlaySound (WHEV_OPEN_MAIN);
#endif
			  place_folders (root_folder);
			  OpenFolder (root_folder);
			}
		      else
			{
			  int x, y;
			  Window junk;

#ifdef ENABLE_SOUND
			  PlaySound (WHEV_CLOSE_MAIN);
#endif
			  XTranslateCoordinates (dpy, Event.xbutton.window,
						 (*root_folder).win,
					   Event.xbutton.x, Event.xbutton.y,
						 &x, &y, &junk);

			  /*
			   * this is broken - if the first button and last button have
			   * different geometries, unexpected results could occur;
			   * fortunately, this probably won't happen often -Ethan (6/10/98)
			   */
			  if (ROWS)
			    {	/* horizontal */
			      if ((*root_folder).y > display_height / 2)
				{
				  CornerY = display_height - (*(*folder).first).height;
				}
			      else
				{
				  CornerY = 0;
				}
			      if (x > (*root_folder).width / 2)
				{
				  CornerX = display_width - (*(*folder).first).width;
				  AnimationDir = DIR_TOLEFT;
				}
			      else
				{
				  CornerX = 0;
				  AnimationDir = DIR_TORIGHT;
				}
			    }
			  else
			    {	/* vertical */
			      if ((*root_folder).x > display_width / 2)
				{
				  CornerX = display_width - (*(*folder).first).width;
				}
			      else
				{
				  CornerX = 0;
				}
			      if (y > (*root_folder).height / 2)
				{
				  CornerY = display_height - (*(*folder).first).height;
				  AnimationDir = DIR_TOUP;
				}
			      else
				{
				  CornerY = 0;
				  AnimationDir = DIR_TODOWN;
				}
			    }
			  CloseFolder (root_folder);
			  XMoveWindow (dpy, (*root_folder).win, CornerX, CornerY);
			  Withdrawn = 1;
			  place_folders (root_folder);
			}
		    }
		  break;
		}
#ifdef ENABLE_SOUND
	      PlaySound (WHEV_PUSH);
#endif
	      CancelPush = 0;

	      if ((CurrentButton = find_button (Event.xbutton.window, Event.xbutton.x, Event.xbutton.y)) == NULL)
		break;

	      if ((*CurrentButton).swallow == 1 ||
		  (*CurrentButton).swallow == 2 ||
		  (*CurrentButton).action == NULL)
		break;

	      if (CurrentButton->swallow == 4)
		{
		  SendInfo (fd, CurrentButton->swallow_command, 0);
		  CurrentButton->swallow = 1;
		  break;
		}

	      if (Pushable)
		{
		  Pushed = 1;
		  GenerateButtonImage(CurrentButton, True);
		}
	      if (mystrncasecmp ((*CurrentButton).action, "Folder", 6) == 0)
		{
		  unsigned long was_mapped = get_flags (CurrentButton->folder->flags, WF_Mapped);
		  unmap_folders ((*CurrentButton).parent);
		  if (!was_mapped)
		    MapFolder ((*CurrentButton).folder);
		}
	      break;
	    case EnterNotify:
	      CancelPush = 0;
	      break;
	    case LeaveNotify:
	      CancelPush = 1;
	      break;
	    case ConfigureNotify:
	      if ((button = find_button (Event.xmap.window, 0, 0)) != NULL && button->swallowed_win == Event.xconfigure.window)
		{
		  /* make really, really sure the swallowed window has no border */
		  if (Event.xconfigure.border_width)
		    XSetWindowBorderWidth (dpy, (*button).swallowed_win, 0);
		  /* make really, really sure the swallowed window is in the right spot */
		  if (Event.xconfigure.x != (button->width - Event.xconfigure.width) / 2 ||
		      Event.xconfigure.y != (button->height - Event.xconfigure.height) / 2)
		    {
		      XMoveWindow (dpy, button->swallowed_win, (button->width - Event.xconfigure.width) / 2, (button->height - Event.xconfigure.height) / 2);
#ifdef SHAPE
		      update_folder_shape (button->parent);
#endif /* !SHAPE */
		    }
		}
	      break;
	    case DestroyNotify:
	      if ((button = find_button (Event.xdestroywindow.window, 0, 0)) != NULL)
		{
		  /* if a swallowed app died, go to swallow mode 5 (wait
		   * for button click to relaunch) */
		  if (button->swallowed_win == Event.xdestroywindow.window)
		    {
		      button->swallow = 4;
		      button->swallowed_win = None;
		    }
		}
	      break;
	    case MapNotify:
	      if ((folder = find_folder (Event.xmap.window)) != NULL)
		{
		  /* make really, really sure the window is unmapped if it's supposed to be */
		  if (folder != root_folder && !get_flags (folder->flags, WF_Mapped))
		    CloseFolder (folder);
		}
	      break;
#ifdef ENABLE_DND
	    case ClientMessage:
	      if (Event.xclient.message_type == DndProtocol)
		{
		  unsigned long dummy_r, size;
		  Atom dummy_a;
		  int dummy_f;
		  unsigned char *data, *Command;
		  Bool has_str, has_int;

		  Window dummy_rt, dummy_c;
		  int dummy_x, dummy_y, base, pos_x, pos_y;
		  unsigned int dummy;

/*                  if (Event.xclient.data.l[0]!=DndFile ||
   Event.xclient.data.l[0]!=DndFiles ||
   Event.xclient.data.l[0]!=DndExe
   )
   break; */

		  XQueryPointer (dpy, Event.xclient.window,
				 &dummy_rt, &dummy_c,
				 &dummy_x, &dummy_y,
				 &pos_x, &pos_y,
				 &dummy);
		  base = 0;
		  if ((button = find_button (Event.xclient.window, pos_x, pos_y)) == NULL ||
		      button->drop_action == NULL || get_flags (button->flags, WB_Transient))
		    break;

		  if (Pushable)
		    {
		      RedrawPushed (button);
		      XSync (dpy, 0);
		    }
		  XGetWindowProperty (dpy, Scr.Root, DndSelection, 0L,
				      100000L, False, AnyPropertyType,
				      &dummy_a, &dummy_f,
				      &size, &dummy_r,
				      &data);
		  if (Event.xclient.data.l[0] == DndFiles)
		    {
		      for (dummy_r = 0; dummy_r < size - 1; dummy_r++)
			{
			  if (data[dummy_r] == 0)
			    data[dummy_r] = ' ';
			}
		    }
#ifdef ENABLE_SOUND
		  PlaySound (WHEV_DROP);
#endif
		  Command = safemalloc (strlen ((*button).drop_action) +
					strlen (data) + 10 + 1);
		  has_str = (strstr ((*button).drop_action, "%s") != NULL) ? True : False;
		  has_int = (strstr ((*button).drop_action, "%d") != NULL) ? True : False;
		  if (has_str && has_int)
		    sprintf (Command, (*button).drop_action, data, Event.xclient.data.l[0]);
		  else if (has_str)
		    sprintf (Command, (*button).drop_action, data);
		  else if (has_int)
		    sprintf (Command, (*button).drop_action, Event.xclient.data.l[0]);
		  else
		    sprintf (Command, (*button).drop_action);
		  SendInfo (fd, Command, 0);
		  free (Command);
		  if (Pushable)
		    {
		      sleep_a_little (50000);
		      RedrawUnpushed (button);
		    }
		}
	      break;
#endif /* ENABLE_DND */
	    case ButtonRelease:
	      if (Event.xbutton.button != Button1 ||
		  CurrentButton == NULL ||
		  (*CurrentButton).swallow == 1 ||
		  (*CurrentButton).swallow == 2 ||
		  (*CurrentButton).swallow == 4 ||
		  (*CurrentButton).action == NULL)
		{
		  break;
		}
	      if (Pushable)
		{
		  Pushed = 0;
		  XClearArea( dpy, CurrentButton->IconWin, 0, 0, CurrentButton->width, CurrentButton->height, True );
//		  RedrawUnpushed (CurrentButton);
		}
	      if (CancelPush)
		break;

	      if ((NewButton = find_button (Event.xbutton.window, Event.xbutton.x, Event.xbutton.y)) == NULL)
		break;

	      if (NewButton == CurrentButton)
		{
		  t = time (0);
		  bl = -1;
		  tl = -1;
		  if (mystrncasecmp ((*CurrentButton).action, "Folder", 6) != 0)
		    {
		      unmap_folders (root_folder);
		      SendInfo (fd, (*CurrentButton).action, 0);
		    }
		}
	      break;

	    case PropertyNotify:
	      if (Event.xproperty.atom == _XROOTPMAP_ID && Style->texture_type >= TEXTURE_TRANSPARENT && Style->texture_type < TEXTURE_BUILTIN)
		update_transparency (root_folder);
	      if (Event.xproperty.atom == _AS_STYLE)
		{
		  mystyle_get_property (dpy, Scr.Root, _AS_STYLE, XA_INTEGER);
		  balloon_set_style (dpy, mystyle_find_or_default ("*WharfBalloon"));
		  CreateIconPixmap ();
		  update_look (root_folder);
		  place_folders (root_folder);
		}
	      break;

	    default:
	      break;
	    }
	}
    }
  return;
}

/*
 * the animations for OpenFolder and CloseFolder on the root are messed up!
 * I'm not sure which button to leave showing when the Wharf is withdrawn,
 * and the calculation for final width and height is wrong for DIR_TOUP and
 * DIR_TOLEFT anyway.
 * - Ethan (6/6/98)
 */

void
OpenFolder (folder_info * folder)
{
  int winc, hinc;
  int cx, cy, cw, ch;
  Window win;
  int isize;

  set_flags (folder->flags, WF_Mapped);

  if (get_flags (folder->flags, WF_NeedTransUpdate))
    update_transparency (folder);

  if ((*folder).parent == NULL)
    {
      winc = 64 / AnimateStepsMain;
      hinc = 64 / AnimateStepsMain;
    }
  else
    {
      winc = 64 / AnimateSteps;
      hinc = 64 / AnimateSteps;
    }

  win = (*folder).win;
  if ((*folder).parent == NULL)
    {
      if ((*folder).direction == DIR_TOLEFT || (*folder).direction == DIR_TORIGHT)
	isize = (*(*folder).first).width;
      else
	isize = (*(*folder).first).height;
    }
  else
    {
      if ((*folder).direction == DIR_TOLEFT || (*folder).direction == DIR_TORIGHT)
	isize = winc;
      else
	isize = hinc;
    }
  cx = (*folder).x;
  cy = (*folder).y;
  ch = (*folder).height;
  cw = (*folder).width;
  if (AnimationStyle != 0)
    switch ((*folder).direction)
      {
      case DIR_TOLEFT:
	cx = (*folder).x + (*folder).width - isize;
	XMoveResizeWindow (dpy, win, cx, (*folder).y, isize, (*folder).height);
	XMapWindow (dpy, win);
	for (cw = isize; cw <= (*folder).width; cw += winc)
	  {
	    sleep_a_little (AnimateDelay / 2);
	    XMoveResizeWindow (dpy, win, cx, (*folder).y, cw, (*folder).height);
	    XSync (dpy, 0);
	    cx -= winc;
	  }
	break;
      case DIR_TORIGHT:
	XMoveResizeWindow (dpy, win, (*folder).x, (*folder).y, isize, (*folder).height);
	XMapWindow (dpy, win);
	for (cw = isize; cw <= (*folder).width; cw += winc)
	  {
	    sleep_a_little (AnimateDelay / 2);
	    XMoveResizeWindow (dpy, win, (*folder).x, (*folder).y, cw, (*folder).height);
	    XSync (dpy, 0);
	  }
	break;
      case DIR_TOUP:
	cy = (*folder).y + (*folder).height - isize;
	XMoveResizeWindow (dpy, win, (*folder).x, cy, (*folder).width, isize);
	XMapWindow (dpy, win);
	for (ch = isize; ch <= (*folder).height; ch += hinc)
	  {
	    sleep_a_little (AnimateDelay / 2);
	    XMoveResizeWindow (dpy, win, (*folder).x, cy, (*folder).width, ch);
	    XSync (dpy, 0);
	    cy -= hinc;
	  }
	break;
      case DIR_TODOWN:
	XMoveResizeWindow (dpy, win, (*folder).x, (*folder).y, (*folder).width, isize);
	XMapWindow (dpy, win);
	for (ch = isize; ch <= (*folder).height; ch += hinc)
	  {
	    sleep_a_little (AnimateDelay / 2);
	    XMoveResizeWindow (dpy, win, (*folder).x, (*folder).y, (*folder).width, ch);
	    XSync (dpy, 0);
	  }
	break;
      default:
	XBell (dpy, 100);
	fprintf (stderr, "WHARF INTERNAL BUG in OpenFolder()\n");
	exit (-1);
      }
  if (cw != (*folder).width || ch != (*folder).height || (*folder).x != cx || cy != (*folder).y || AnimationStyle == 0)
    XMoveResizeWindow (dpy, win, (*folder).x, (*folder).y, (*folder).width, (*folder).height);

  if (AnimationStyle == 0)
    XMapWindow (dpy, win);
}



void
CloseFolder (folder_info * folder)
{
  int winc, hinc;
  int cx, cy, cw, ch;
  int x, y, w, h, junk_depth, junk_bd;
  int fsize, direction;
  Window win, junk_win;

#ifdef ENABLE_SOUND
  PlaySound (WHEV_CLOSE_FOLDER);
#endif
  if ((*folder).parent == NULL)
    {
      winc = 64 / AnimateStepsMain;
      hinc = 64 / AnimateStepsMain;
      direction = AnimationDir;
      if (direction == DIR_TOUP || direction == DIR_TODOWN)
	fsize = (*(*folder).first).height;
      else
	fsize = (*(*folder).first).width;
    }
  else
    {
      winc = 64 / AnimateSteps;
      hinc = 64 / AnimateSteps;
      direction = (*folder).direction;
      if (direction == DIR_TOUP || direction == DIR_TODOWN)
	fsize = hinc;
      else
	fsize = winc;
    }
  win = (*folder).win;
  if (AnimationStyle != 0)
    {
      XGetGeometry (dpy, win, &junk_win, &x, &y, &w, &h, &junk_bd, &junk_depth);
      XTranslateCoordinates (dpy, win, Scr.Root, x, y, &x, &y, &junk_win);
      switch (direction)
	{
	case DIR_TOLEFT:
	  cx = x;
	  for (cw = w; cw >= fsize; cw -= winc)
	    {
	      XMoveResizeWindow (dpy, win, cx, y, cw, h);
	      XSync (dpy, 0);
	      sleep_a_little (AnimateDelay);
	      cx += winc;
	    }
	  break;
	case DIR_TORIGHT:
	  for (cw = w; cw >= fsize; cw -= winc)
	    {
	      XMoveResizeWindow (dpy, win, x, y, cw, h);
	      XSync (dpy, 0);
	      sleep_a_little (AnimateDelay);
	    }
	  break;
	case DIR_TOUP:
	  cy = y;
	  for (ch = h; ch >= fsize; ch -= hinc)
	    {
	      XMoveResizeWindow (dpy, win, x, cy, w, ch);
	      XSync (dpy, 0);
	      sleep_a_little (AnimateDelay);
	      cy += hinc;
	    }
	  break;
	case DIR_TODOWN:
	  for (ch = h; ch >= fsize; ch -= hinc)
	    {
	      XMoveResizeWindow (dpy, win, x, y, w, ch);
	      XSync (dpy, 0);
	      sleep_a_little (AnimateDelay);
	    }
	  break;
	default:
	  XBell (dpy, 100);
	  fprintf (stderr, "WHARF INTERNAL BUG in CloseFolder()\n");
	  exit (-1);
	}
    }
  if (folder == root_folder)
    {
      XResizeWindow (dpy, win, (*(*folder).first).width, (*(*folder).first).height);
    }
  else
    {
      XUnmapWindow (dpy, win);
      clear_flags (folder->flags, WF_Mapped);
    }
}


void
MapFolder (folder_info * folder)
{
#ifdef ENABLE_SOUND
  PlaySound (WHEV_OPEN_FOLDER);
#endif

  OpenFolder (folder);
}


void 
GenerateButtonImage(button_info * button, Bool pushed)
{
	ASImage *im = button->completeIcon ;
	Pixmap p;
	if( pushed || NoBorder == 0 ) 
	{
		ASImageLayer layer ;
		ASImageBevel bevel ;
	
		init_image_layers( &layer, 1 );
		layer.im = im ;
		layer.clip_width = button->width ;
		layer.clip_height = button->height;
		
		memset( &bevel, 0x00, sizeof(bevel));
		if( pushed )
		{
			if( PushStyle == 0 )
				layer.clip_x = layer.clip_y = 2 ;
			bevel.hi_color = GetShadow(Style->colors.back) ;
			bevel.lo_color = GetHilite(Style->colors.back) ;
			bevel.hihi_color = GetShadow (bevel.hi_color) ;
			bevel.hilo_color = GetAverage(bevel.hi_color, bevel.lo_color);
			bevel.lolo_color = GetHilite (bevel.lo_color) ;
			bevel.left_inline = bevel.top_inline = 6 ;
			bevel.right_inline = bevel.bottom_inline = 4 ;
			bevel.left_outline = bevel.top_outline = 2 ;
			bevel.right_outline = bevel.bottom_outline = 1 ;
		}else
		{
			if( PushStyle == 0 )
				layer.clip_x = layer.clip_y = 1 ;
			bevel.hi_color = GetHilite(Style->colors.back) ;
			bevel.lo_color = GetShadow(Style->colors.back) ;
			bevel.hihi_color = GetHilite (bevel.hi_color) ;
			bevel.hilo_color = GetAverage(bevel.hi_color, bevel.lo_color);
			bevel.lolo_color = GetShadow (bevel.lo_color) ;
			bevel.left_inline = bevel.top_inline = 4 ;
			bevel.right_inline = bevel.bottom_inline = 6 ;
			bevel.left_outline = bevel.top_outline = 1 ;
			bevel.right_outline = bevel.bottom_outline = 2 ;
		}
		if( NoBorder == 0 ) 
		{
			layer.bevel = &bevel ;
			if( PushStyle == 0 )
				layer.clip_x = layer.clip_y = (pushed)?2:1 ;
			layer.clip_width -= 3;
			layer.clip_height -= 3;
		}else if( PushStyle > 0 )
		{
			layer.dst_x = layer.dst_y = (pushed)?2:1 ;
			layer.clip_width -= layer.dst_x;
			layer.clip_height -= layer.dst_y;
		}
		im = merge_layers( Scr.asv, &layer, 1, button->width, button->height, 
				  		   ASA_XImage, 0, ASIMAGE_QUALITY_DEFAULT );
	}
	if( im )
	{
		if( pushed && PushStyle != 0 )  
		{
			asimage2drawable( Scr.asv, button->IconWin, im, NULL, 
			                  0, 0, 0, 0, 
							  button->width, button->height,
							  True );
		}else
		{
			p = asimage2pixmap( Scr.asv, Scr.Root, im, NULL, True );
			if( p )
			{
				XSetWindowBackgroundPixmap( dpy, button->IconWin, p );
				XClearWindow( dpy, button->IconWin );
				XSync( dpy, False );
				XFreePixmap( dpy, p );
			}
		}
		if( im != button->completeIcon ) 
			destroy_asimage( &im );
	}
}


void 
GenerateFolderImages(folder_info * folder)
{
	register button_info *button ;
    for (button = (*folder).first; button != NULL; button = (*button).next)
		GenerateButtonImage(button, False);
}

/*******************************************************************
 *
 * Create the background icon pixmap
 *
 ******************************************************************/
void
CreateIconPixmap (void)
{
    if (back_pixmap != NULL)
	  	destroy_asimage( &back_pixmap );
    /* create the icon pixmap */
	if ( Style->texture_type != TEXTURE_BUILTIN || 
  	     (back_pixmap = GetXPMData (( const char**)button_xpm)) == NULL )
    {
    	int width = 64, height = 64;
#ifndef NO_TEXTURE
    	if ((Style->texture_type >= TEXTURE_PIXMAP && Style->texture_type < TEXTURE_BUILTIN) && 
	  	     Style->back_icon.image != NULL)
		{
			width = Style->back_icon.image->width;
			height = Style->back_icon.image->height;
		}
#endif /* !NO_TEXTURE */
		back_pixmap = mystyle_make_image (Style, 0, 0, width, height);
    }
}

/************************************************************************
 *
 * resizes a folder and its buttons
 * requires: button widths and heights are set
 *           folder window and button windows must already exist
 * the folder width and height is calculated by this function
 *
 ***********************************************************************/
void
place_buttons (folder_info * folder)
{
  button_info *button;
  int x, y, n, w, h;
  int tmp_width = (*folder).width;
  int tmp_height = (*folder).height;

  if (((*folder).direction == DIR_TOUP) || ((*folder).direction == DIR_TODOWN))
    {
      button_info *first = (*folder).first;
      x = y = w = h = n = 0;
      for (button = (*folder).first; button != NULL; button = (*button).next)
	{
	  (*button).x = w;
	  (*button).y = y;
	  y += (*button).height;
	  if (x < (*button).width)
	    x = (*button).width;
	  if ((++n >= Rows && folder == root_folder) || (*button).next == NULL)
	    {
	      button_info *b;
	      w += x;
	      if (h < y)
		h = y;
	      /* make sure the button geometry matches the folder */
	      for (b = first; b != (*button).next; b = (*b).next)
		(*b).width = x;
	      first = (*button).next;
	      x = y = n = 0;
	    }
	}
      (*folder).width = w;
      (*folder).height = h;
      /* reverse the order for DIR_TOUP folders */
      if (folder != root_folder && (*folder).direction == DIR_TOUP)
	for (button = (*folder).first; button != NULL; button = (*button).next)
	  (*button).y = (*folder).height - ((*button).y + (*button).height);
    }
  else
    {
      button_info *first = (*folder).first;
      x = y = w = h = n = 0;
      for (button = (*folder).first; button != NULL; button = (*button).next)
	{
	  (*button).x = x;
	  (*button).y = h;
	  x += (*button).width;
	  if (y < (*button).height)
	    y = (*button).height;
	  if ((++n >= Columns && folder == root_folder) || (*button).next == NULL)
	    {
	      button_info *b;
	      h += y;
	      if (w < x)
		w = x;
	      /* make sure the button geometry matches the folder */
	      for (b = first; b != NULL; b = (*b).next)
		(*b).height = y;
	      first = (*button).next;
	      x = y = n = 0;
	    }
	}
      (*folder).width = w;
      (*folder).height = h;
      /* reverse the order for DIR_TOLEFT folders */
      if (folder != root_folder && (*folder).direction == DIR_TOLEFT)
	for (button = (*folder).first; button != NULL; button = (*button).next)
	  (*button).x = (*folder).width - ((*button).x + (*button).width);
    }

  if (((*folder).width != tmp_width) || ((*folder).height != tmp_height))
    {
      /* resize the folder */
      XResizeWindow (dpy, (*folder).win, (*folder).width, (*folder).height);

      /* reconfigure the buttons */
      for (button = (*folder).first; button != NULL; button = (*button).next)
	{
	  Window root;
	  int width, height, junk;
	  XGetGeometry (dpy, (*button).IconWin, &root, &x, &y, &width, &height, &junk, &junk);
	  if (((*button).x != x) || ((*button).y != y))
	    XMoveWindow (dpy, (*button).IconWin, (*button).x, (*button).y);
	  if (((*button).width != width) || ((*button).height != height))
	    {
	      XResizeWindow (dpy, (*button).IconWin,
			     (*button).width, (*button).height);
		  RenderButtonIcon( button );
		  GenerateButtonImage( button, False );
	      /* if there is a swallowed window, recenter it */
	      if ((*button).swallowed_win != None)
		{
		  XSetWindowBorderWidth (dpy, (*button).swallowed_win, 0);
		  XGetGeometry (dpy, (*button).swallowed_win, &root, &x, &y, &width, &height, &junk, &junk);
		  XMoveWindow (dpy, (*button).swallowed_win,
			       ((*button).width - width) / 2,
			       ((*button).height - height) / 2);
		}
	    }
	}

      /* if this is the root folder, we also need to move the folder */
      if (folder == root_folder)
	{
	  if ((root_gravity == SouthWestGravity) || (root_gravity == SouthEastGravity))
	    (*root_folder).y += tmp_height - (*folder).height;
	  if ((root_gravity == NorthEastGravity) || (root_gravity == SouthEastGravity))
	    (*root_folder).x += tmp_width - (*folder).width;
	  XMoveWindow (dpy, (*folder).win, (*root_folder).x, (*root_folder).y);
	}
    }

  update_folder_shape (folder);
}

void
update_folder_shape (folder_info * folder)
{
#ifdef SHAPE
  button_info *button;
  clear_flags ((*folder).flags, WF_Shaped);
  for (button = (*folder).first; button != NULL; button = (*button).next)
    if (get_flags ((*button).flags, WB_Shaped))
      {
	set_flags ((*folder).flags, WF_Shaped);
	break;
      }
  /* folder was shaped, but isn't now - remove mask */
  if (!get_flags ((*folder).flags, WF_Shaped) && ((*folder).mask != None))
    {
      XFreePixmap (dpy, (*folder).mask);
      (*folder).mask = None;
      XShapeCombineMask (dpy, (*folder).win, ShapeBounding, 0, 0, None, ShapeSet);
    }
  /* reshape the folder */
  if (get_flags ((*folder).flags, WF_Shaped))
    {
      GC ShapeGC;
      if ((*folder).mask != None)
	XFreePixmap (dpy, (*folder).mask);
      (*folder).mask = XCreatePixmap (dpy, Scr.Root, (*folder).width, (*folder).height, 1);
      ShapeGC = XCreateGC (dpy, (*folder).mask, 0, NULL);
      XSetForeground (dpy, ShapeGC, 0);
      XFillRectangle (dpy, (*folder).mask, ShapeGC, 0, 0, (*folder).width, (*folder).height);
      XSetForeground (dpy, ShapeGC, 1);
      /* add button masks to folder mask */
      for (button = (*folder).first; button != NULL; button = (*button).next)
	{
	  XCopyArea (dpy, (*button).mask, (*folder).mask, ShapeGC, 0, 0, (*button).width, (*button).height, (*button).x, (*button).y);
	  if ((*button).swallowed_win != None)
	    {
	      int junk;
	      Bool is_shaped = False;
	      XShapeQueryExtents (dpy, (*button).swallowed_win, &is_shaped, &junk, &junk, &junk, &junk, &junk, &junk, &junk, &junk, &junk);
	      if (!is_shaped)
		{
		  Window root;
		  int x, y, width, height;
		  XGetGeometry (dpy, (*button).swallowed_win, &root, &x, &y, &width, &height, &junk, &junk);
		  XFillRectangle (dpy, (*folder).mask, ShapeGC, (*button).x + x, (*button).y + y, width, height);
		}
	    }
	}
      XShapeCombineMask (dpy, (*folder).win, ShapeBounding, 0, 0, (*folder).mask, ShapeSet);
      /* combine shaped window masks with the folder mask */
      for (button = (*folder).first; button != NULL; button = (*button).next)
	if ((*button).swallowed_win != None)
	  {
	    Window root;
	    int x, y, width, height, junk;
	    Bool is_shaped = False;
	    XSync (dpy, 0);
	    XSetWindowBorderWidth (dpy, (*button).swallowed_win, 0);
	    XGetGeometry (dpy, (*button).swallowed_win, &root, &x, &y, &width, &height, &junk, &junk);
	    XShapeQueryExtents (dpy, (*button).swallowed_win, &is_shaped, &junk, &junk, &junk, &junk, &junk, &junk, &junk, &junk, &junk);
	    if (is_shaped == True)
	      {
		XShapeCombineShape (dpy, (*folder).win, ShapeBounding, (*button).x + x, (*button).y + y, (*button).swallowed_win, ShapeBounding, ShapeUnion);
	      }
	  }
      XFreeGC (dpy, ShapeGC);
    }
#endif /* SHAPE */
}

/************************************************************************
 *
 * calculates folder x and y
 * requires: folder widths and heights are set
 *           all buttons' x and y are set
 * offsets all subfolders of folder using folder's geometry
 *
 ***********************************************************************/
void
place_folders (folder_info * folder)
{
  button_info *b;

  /* figure out root_folder location */
  if (folder->parent == NULL || folder->parent->parent == NULL)
    {
      if (!Withdrawn)
	{

	  folder->x = root_x;
	  folder->y = root_y;
	  if (root_gravity == NorthEastGravity || root_gravity == SouthEastGravity)
	    folder->x += DisplayWidth (dpy, Scr.screen) - (folder->width + 1);
	  if (root_gravity == SouthWestGravity || root_gravity == SouthEastGravity)
	    folder->y += DisplayHeight (dpy, Scr.screen) - (folder->height + 2);
	}
      else
	{
	  folder->x = CornerX;
	  folder->y = CornerY;
	}
    }
  else
    {
      folder_info *f = folder->parent->parent;
      b = folder->parent;
      folder->x = f->x + b->x;
      folder->y = f->y + b->y;
      switch (folder->direction)
	{
	case DIR_TOUP:
	  folder->y -= folder->height;
	  break;
	case DIR_TODOWN:
	  folder->y += f->height;
	  break;
	case DIR_TOLEFT:
	  folder->x -= folder->width;
	  break;
	case DIR_TORIGHT:
	  folder->x += f->width;
	  break;
	}
      if (folder->x + folder->width > display_width)
	folder->x = display_width - folder->width;
      if (folder->y + folder->height > display_height)
	folder->y = display_height - folder->height;
    }

  /* update the transparency if need be */
  update_transparency (folder);

  XMoveWindow (dpy, folder->win, folder->x, folder->y);

  /* do the children */
  for (b = folder->first; b != NULL; b = b->next)
    if (b->folder != NULL)
      place_folders (b->folder);
}

/************************************************************************
 *
 * updates transparency for a folder
 *
 ***********************************************************************/
void
update_transparency (folder_info * folder)
{
  int is_shaped = 0, was_shaped = get_flags (folder->flags, WF_Shaped) ? 1 : 0;
  button_info *b;
  if (Style->texture_type < TEXTURE_TRANSPARENT ||
      Style->texture_type >= TEXTURE_BUILTIN || folder->win == None)
    return;
  if (!get_flags (folder->flags, WF_Mapped))
    {
      set_flags (folder->flags, WF_NeedTransUpdate);
      return;
    }
  clear_flags (folder->flags, WF_NeedTransUpdate);
  for (b = (*folder).first; b != NULL; b = b->next)
    {
		  RenderButtonIcon( b );
		  GenerateButtonImage( b, False );
      if (get_flags (b->flags, WB_Shaped))
	is_shaped = 1;
      if (b->folder)
	update_transparency (b->folder);
    }
  if (is_shaped != was_shaped)
    update_folder_shape (folder);
}

/************************************************************************
 *
 * updates look for a folder (recursively)
 *
 ***********************************************************************/
void
update_look (folder_info * folder)
{
  button_info *b;
  for (b = (*folder).first; b != NULL; b = b->next)
    {
	  RenderButtonIcon( b );
	  GenerateButtonImage( b, False );
      if ((*b).folder != NULL)
	update_look ((*b).folder);
    }
  place_buttons (folder);
  XSetWindowBackgroundPixmap (dpy, (*folder).win, None /*back_pixmap.icon*/);
}

/************************************************************************
 *
 * Sizes and creates the window for a folder, its buttons, and its
 * child folders (recursively).  The direction of the top folder must
 * be set, but the direction of the child folders will be calculated.
 *
 ***********************************************************************/
void
CreateFolderWindow (folder_info * folder)
{
  button_info *button;
  XSizeHints hints;

  /* create the folder window */
  (*folder).win = XCreateSimpleWindow (dpy, Scr.Root, 0, 0,
				       64, 64, 0, 0, Style->colors.back);

  /* set the folder window background */
/*  XSetWindowBackgroundPixmap (dpy, (*folder).win, back_pixmap.icon); */

  /* figure out the button sizes and create their windows */
  for (button = (*folder).first; button != NULL; button = (*button).next)
    {
      if (!get_flags ((*button).flags, WB_UserSetSize))
	  {
		/* TODO: why do we need this : */
		(*button).width = (ForceWidth > 0)?ForceWidth:back_pixmap->width;
		(*button).height = (ForceHeight > 0)?ForceHeight:back_pixmap->height;
	  }
      (*button).IconWin = CreateButtonIconWindow ((*folder).win);
      XSelectInput (dpy, (*button).IconWin, EnterWindowMask | LeaveWindowMask);
    }

  /* make a balloon for each button */
  for (button = (*folder).first; button != NULL; button = (*button).next)
    if (!get_flags (button->flags, WB_Transient))
      balloon_new_with_text (dpy, (*button).IconWin, (*button).title);

  /* place the buttons and resize the folder window */
  place_buttons (folder);

  /*
   * claim the user set the position, so the WM will let us
   * position our own window
   */
  hints.flags = USPosition;
  XSetWMNormalHints (dpy, (*folder).win, &hints);

  /* turn on events */
  XSelectInput (dpy, (*folder).win, MW_EVENTS);

  /* if this is a folder, recurse */
  for (button = (*folder).first; button != NULL; button = (*button).next)
    {
      if ((*button).folder != NULL)
	{
	  if (((*folder).direction == DIR_TOUP) && (root_gravity == SouthEastGravity))
	    (*(*button).folder).direction = DIR_TOLEFT;

	  if (((*folder).direction == DIR_TOUP) && (root_gravity == SouthWestGravity))
	    (*(*button).folder).direction = DIR_TORIGHT;

	  if (((*folder).direction == DIR_TODOWN) && (root_gravity == NorthEastGravity))
	    (*(*button).folder).direction = DIR_TOLEFT;

	  if (((*folder).direction == DIR_TODOWN) && (root_gravity == NorthWestGravity))
	    (*(*button).folder).direction = DIR_TORIGHT;

	  if (((*folder).direction == DIR_TOLEFT) && (root_gravity == SouthEastGravity))
	    (*(*button).folder).direction = DIR_TOUP;

	  if (((*folder).direction == DIR_TOLEFT) && (root_gravity == NorthEastGravity))
	    (*(*button).folder).direction = DIR_TODOWN;

	  if (((*folder).direction == DIR_TORIGHT) && (root_gravity == SouthWestGravity))
	    (*(*button).folder).direction = DIR_TOUP;

	  if (((*folder).direction == DIR_TORIGHT) && (root_gravity == NorthWestGravity))
	    (*(*button).folder).direction = DIR_TODOWN;

	  CreateFolderWindow ((*button).folder);
	}
    }

  /* map the folder, buttons, and swallowed windows */
  XMapSubwindows (dpy, (*folder).win);
}

/************************************************************************
 *
 * Sizes and creates the windows
 *
 ***********************************************************************/
void
CreateWindow (void)
{
  int n;
  button_info *button;

  /* figure out window gravity */
  if (root_x > -100000)
    {
      if ((flags & XNegative) && (flags & YNegative))
	root_gravity = SouthEastGravity;
      else if ((flags & XNegative) && !(flags & YNegative))
	root_gravity = NorthEastGravity;
      else if (!(flags & XNegative) && (flags & YNegative))
	root_gravity = SouthWestGravity;
      else if (!(flags & XNegative) && !(flags & YNegative))
	root_gravity = NorthWestGravity;
    }
  /* figure out root_folder direction */
  if (!ROWS)
    {
      (*root_folder).direction = DIR_TODOWN;
      if ((root_gravity == SouthEastGravity) || (root_gravity == SouthWestGravity))
	(*root_folder).direction = DIR_TOUP;
    }
  else
    {
      (*root_folder).direction = DIR_TORIGHT;
      if ((root_gravity == NorthEastGravity) || (root_gravity == SouthEastGravity))
	(*root_folder).direction = DIR_TOLEFT;
    }

  /* count items in folder */
  for (button = (*root_folder).first, n = 0; button != NULL; n++, button = (*button).next);
  /* figure out rows and columns, rounding up */
  if (!ROWS)
    Rows = (n + Columns - 1) / Columns;
  else
    Columns = (n + Rows - 1) / Rows;

  CreateFolderWindow (root_folder);

  XSetWMProtocols (dpy, (*root_folder).win, &_XA_WM_DELETE_WINDOW, 1);

  change_window_name (MyName);

  /* place folder windows */
  place_folders (root_folder);
}

void
nocolor (char *a, char *b)
{
  fprintf (stderr, "%s: can't %s %s\n", MyName, a, b);
}

/************************************************************************
 *
 * Dead pipe handler
 *
 ***********************************************************************/
void
DeadPipe (int nonsense)
{
#ifdef ENABLE_SOUND
  int val = -1;
  write (PlayerChannel[1], &val, sizeof (val));
  if (SoundThread != 0)
    kill (SoundThread, SIGUSR1);
#endif

  /* ignore another SIGPIPE while shutting down */
  signal (SIGPIPE, SIG_IGN);

  if (root_folder != NULL)
    delete_folder (root_folder);
  XSync (dpy, 0);

#ifdef DEBUG_ALLOCS
/* normally, we let the system clean up, but when auditing time comes
 * around, it's best to have the books in order... */
  {
    GC gc1, gc2, gc3, gc4;

#ifdef ENABLE_SOUND
    if (SoundPlayer != NULL)
      free (SoundPlayer);
    if (SoundPath != NULL)
      free (SoundPath);
    if (ModulePath != NULL)
      free (ModulePath);
#endif
	if( back_pixmap )
	{
		safe_asimage_destroy( back_pixmap );
		back_pixmap = NULL ;
	}
	if( imman ) 
	{
	  destroy_image_manager( imman, False );
	  imman = NULL ;
	}
    if (iconPath != NULL)
      free (iconPath);
    if (pixmapPath != NULL)
      free (pixmapPath);

    /* kill the styles */
    while (mystyle_first != NULL)
      mystyle_delete (mystyle_first);

    /* kill the global GCs so they won't show up in the memory audit */
    mystyle_get_global_gcs (Style, &gc1, &gc2, &gc3, &gc4);
    XFreeGC (dpy, gc1);
    XFreeGC (dpy, gc2);
    XFreeGC (dpy, gc3);
    XFreeGC (dpy, gc4);

    balloon_init (1);

    print_unfreed_mem ();
  }
#endif /* DEBUG_ALLOCS */

  exit (0);
}

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/
void
ParseOptions (char *filename)
{
  FILE *fd;
  char *line, *tline, *tmp;
  char *buf;
  int g_x, g_y;
  int len;
  unsigned width, height;

  /* create our style */
  buf = safemalloc (1 + strlen (MyName) + 4 + 1);
  sprintf (buf, "*%sTile", MyName);
  if ((Style = mystyle_find (buf)) == NULL)
    {
      Style = mystyle_new_with_name (buf);
      /* default to builtin texture */
      Style->texture_type = TEXTURE_BUILTIN;
    }
  free (buf);

  if ((fd = fopen (filename, "r")) == NULL)
    {
      fprintf (stderr, "%s: can't open config file %s", MyName, filename);
      exit (1);
    }

  line = (char *) safemalloc (MAXLINELENGTH);
  buf = safemalloc (MAXLINELENGTH + 30);
  len = strlen (MyName);
  while ((tline = fgets (line, MAXLINELENGTH, fd)) != NULL)
    {
      while (isspace (*tline))
	tline++;
      if (*tline == '*')
	{
	  tline++;

	  if (!mystrncasecmp (tline, MyName, len))
	    {
	      tline += len;
	      for (tmp = tline + strlen (tline) - 1; tmp >= tline && isspace (*tmp); *tmp-- = '\0');
	      if (balloon_parse (tline, fd))
		continue;
	      else if (!mystrncasecmp (tline, "Geometry", 8))
		{
		  for (tmp = tline + 8; isspace (*tmp); tmp++);
		  flags = XParseGeometry (tmp, &g_x, &g_y, &width, &height);
		  if (flags & WidthValue)
		    root_w = width;
		  if (flags & HeightValue)
		    root_h = height;
		  if (flags & XValue)
		    root_x = g_x;
		  if (flags & YValue)
		    root_y = g_y;
		}
	      else if (!mystrncasecmp (tline, "Rows", 4))
		{
		  ROWS = TRUE;
		  Rows = strtol (tline + 4, NULL, 10);
		}
	      else if (!mystrncasecmp (tline, "Columns", 7))
		{
		  ROWS = FALSE;
		  Columns = strtol (tline + 7, NULL, 10);
		}
	      else if (!mystrncasecmp (tline, "NoPush", 6))
		{
		  Pushable = 0;
		}
	      else if (!mystrncasecmp (tline, "FullPush", 8))
		{
		  PushStyle = 1;
		}
	      else if (!mystrncasecmp (tline, "NoBorder", 8))
		{
		  NoBorder = 1;
		}
	      else if (!mystrncasecmp (tline, "WithdrawStyle", 13))
		{
		  DoWithdraw = strtol (tline + 13, NULL, 10);
		}
	      /* the NoWithdraw option is undocumented, deprecated, and
	         ** may be removed at Wharf's maintainer's discretion */
	      else if (!mystrncasecmp (tline, "NoWithdraw", 10))
		{
		  DoWithdraw = 0;
		}
	      else if (!mystrncasecmp (tline, "ForceSize", 9))
		{
		  int t = strtol (tline + 9, &tmp, 0);
		  if (tmp != tline + 9)
		    {
		      char *tmp2;
		      ForceWidth = MAX (0, t);
		      t = strtol (tmp, &tmp2, 0);
		      if (tmp2 != tmp)
			ForceHeight = MAX (0, t);
		    }
		  else
		    {
		      ForceWidth = 64;
		      ForceHeight = 64;
		    }
		}
/* TextureType, MaxColors, BgColor, TextureColor, and Pixmap are obsolete */
	      else if (!mystrncasecmp (tline, "TextureType", 11))
		{
		  int t = strtol (tline + 11, NULL, 10);
		  if (t >= 0)
		    (*Style).texture_type = t;
		}
	      else if (!mystrncasecmp (tline, "MaxColors", 9))
		{
		  mystyle_parse_member (Style, tline, pixmapPath);
		}
	      else if (!mystrncasecmp (tline, "BgColor", 7))
		{
		  int t = Style->texture_type;
		  sprintf (buf, "BackColor %s", tline + 7);
		  mystyle_parse_member (Style, buf, pixmapPath);
		  Style->texture_type = t;
		}
	      else if (!mystrncasecmp (tline, "TextureColor", 12))
		{
		  int t = Style->texture_type;
		  sprintf (buf, "BackGradient 1 %s", tline + 12);
		  mystyle_parse_member (Style, buf, pixmapPath);
		  Style->texture_type = t;
		}
	      else if (!mystrncasecmp (tline, "Pixmap", 6))
		{
		  int t = Style->texture_type;
		  sprintf (buf, "BackPixmap 128 %s", tline + 6);
		  mystyle_parse_member (Style, buf, pixmapPath);
		  Style->texture_type = t;
		}
	      else if (!mystrncasecmp (tline, "AnimateStepsMain", 16))
		{
		  sscanf (tline + 16, "%d", &AnimateStepsMain);
		  if (AnimateStepsMain < 1)
		    AnimateStepsMain = 1;
		}
	      else if (!mystrncasecmp (tline, "AnimateSteps", 12))
		{
		  sscanf (tline + 12, "%d", &AnimateSteps);
		  if (AnimateSteps < 1)
		    AnimateSteps = 1;
		}
	      else if (!mystrncasecmp (tline, "AnimateDelay", 12))
		{
		  sscanf (tline + 13, "%d", &AnimateDelay);
		  if (AnimateDelay < 0)
		    AnimateDelay = 0;
		  AnimateDelay *= 10000;
		}
	      else if (!mystrncasecmp (tline, "AnimateMain", 11))
		{
		  AnimateMain = 1;
		}
	      else if (!mystrncasecmp (tline, "Animate", 7))
		{
		  if ((*(tline + 8) != 'M') && (*(tline + 9) != 'm'))
		    AnimationStyle = 1;
		}
#ifdef ENABLE_SOUND
	      else if (!mystrncasecmp (tline, "Player", 6))
		{
		  CopyString (&SoundPlayer, tline + 6);
		}
	      else if (!mystrncasecmp (tline, "Sound", 5))
		{
		  bind_sound (tline + 5);
		  SoundActive = 1;
		}
#endif
	      else
		{
		  /* check if this is a invalid option */
		  if (!isspace (*tline))
		    fprintf (stderr, "%s:invalid option %s\n", MyName, tline);
		  else
		    match_stringWharf (tline);
		}
	    }
	}
      else if (!mystrncasecmp (tline, "MyStyle", 7))
	mystyle_parse (tline + 7, fd, &pixmapPath, NULL);
    }
  free (buf);
  free (line);
  fclose (fd);

#ifndef NO_TEXTURE
  if (Style->texture_type == TEXTURE_PIXMAP && Style->back_icon.pix == None)
#endif /* !NO_TEXTURE */
    Style->texture_type = TEXTURE_BUILTIN;
}


/* Parse base file */

void
ParseBaseOptions (char *filename)
{
  FILE *fd;
  char line[MAXLINELENGTH * 2];
  char *tline;

  if ((fd = fopen (filename, "r")) == NULL)
    {
      fprintf (stderr, "%s: can't open base file %s", MyName, filename);
      exit (1);
    }
  tline = fgets (line, sizeof (line), fd);
  while (tline != NULL)
    {
      while (isspace (*tline))
	tline++;

/* Parse base file */
      if (strlen (tline) > 1)
	{
	  if (!mystrncasecmp (tline, "IconPath", 8))
	    {
	      CopyString (&iconPath, tline + 8);
	      replaceEnvVar (&iconPath);
	    }
	  else if (!mystrncasecmp (tline, "PixmapPath", 10))
	    {
	      CopyString (&pixmapPath, tline + 10);
	      replaceEnvVar (&pixmapPath);
	    }
#ifdef ENABLE_SOUND
	  else if (!mystrncasecmp (tline, "AudioPath", 9))
	    {
	      CopyString (&SoundPath, tline + 9);
	      replaceEnvVar (&SoundPath);
	    }
	  else if (!mystrncasecmp (tline, "ModulePath", 11))
	    {
	      CopyString (&ModulePath, tline + 11);
	      replaceEnvVar (&ModulePath);
	    }
#endif
	}
      tline = fgets (line, sizeof (line), fd);
    }
  fclose (fd);
  if( imman ) 
	  destroy_image_manager( imman, False );
  imman = create_image_manager( NULL, 2.2, pixmapPath, iconPath, NULL );
}


/*
 * Gets a word of a given index in the line, stripping any blanks
 * The returned word is newly allocated
 */
#ifdef ENABLE_SOUND
char *
get_token (char *tline, int index)
{
  char *start, *end;
  int i, c, size;
  char *word;

  index++;			/* index is 0 based */
  size = strlen (tline);
  i = c = 0;
  start = end = tline;
  while (i < index && c < size)
    {
      start = end;
      while (isspace (*start) && c < size)
	{
	  start++;
	  c++;
	}
      end = start;
      while (!isspace (*end) && c < size)
	{
	  end++;
	  c++;
	}
      if (end == start)
	return NULL;
      i++;
    }
  if (i < index)
    return NULL;
  word = safemalloc (end - start + 1);
  strncpy (word, start, end - start);
  word[end - start] = 0;
  return word;
}

/**************************************************************************
 *
 * Parses a sound binding
 *
 **************************************************************************/
void
bind_sound (char *tline)
{
  char *event, *sound;

  event = get_token (tline, 0);
  if (event == NULL)
    {
      fprintf (stderr, "%s:bad sound binding %s\n", MyName, tline);
      return;
    }
  sound = get_token (tline, 1);
  if (sound == NULL)
    {
      free (event);
      fprintf (stderr, "%s:bad sound binding %s\n", MyName, tline);
      return;
      ;
    }
  if (strcmp (event, "open_folder") == 0)
    {
      Sounds[WHEV_OPEN_FOLDER] = sound;
    }
  else if (strcmp (event, "close_folder") == 0)
    {
      Sounds[WHEV_CLOSE_FOLDER] = sound;
    }
  else if (strcmp (event, "open_main") == 0)
    {
      Sounds[WHEV_OPEN_MAIN] = sound;
    }
  else if (strcmp (event, "close_main") == 0)
    {
      Sounds[WHEV_CLOSE_MAIN] = sound;
    }
  else if (strcmp (event, "push") == 0)
    {
      Sounds[WHEV_PUSH] = sound;
    }
  else if (strcmp (event, "drop") == 0)
    {
      Sounds[WHEV_DROP] = sound;
    }
  else
    {
      fprintf (stderr, "%s:bad event %s in sound binding\n", MyName, event);
      free (sound);
    }
  free (event);
  return;
}
#endif /* ENABLE_SOUND */

/**************************************************************************
 *
 * Parses a button command line from the config file
 *
 *************************************************************************/
void
match_stringWharf (char *tline)
{
  int len, i, i2, j, k;
  char *ptr, *start, *end, *tmp;
  struct button_info *actual;
  struct button_info *button;
  char *filename = NULL ;

  /* skip spaces */
  while (isspace (*tline) && (*tline != '\n'))
    tline++;

  /* read next word. Its the button label. Users can specify ""
   * NoIcon, or whatever to skip the label */
  /* read to next space */
  start = tline;
  end = tline;
  while ((!isspace (*end)) && (*end != '\n') && (*end != '\0'))
    end++;
  len = end - start;
  ptr = safemalloc (len + 1);
  strncpy (ptr, start, len);
  ptr[len] = 0;

  if (strncmp (ptr, "~Folder", 7) == 0)
    {
      if ((*current_folder).parent == NULL)
	fprintf (stderr, "%s: can't close top level folder\n", MyName);
      else
	{
	  /* get rid of empty folders */
	  if ((*current_folder).first == NULL)
	    {
	      folder_info *tmp = (*(*current_folder).parent).parent;
	      fprintf (stderr, "%s: removing empty folder\n", MyName);
	      delete_button ((*current_folder).parent);
	      current_folder = tmp;
	    }
	  else
	    current_folder = (*(*current_folder).parent).parent;
	}
      free (ptr);
      return;
    }
  /* add a new button */
  if ((*current_folder).first == NULL)
    actual = (*current_folder).first = new_button (NULL);
  else
    {
      for (actual = (*current_folder).first; (*actual).next != NULL; actual = (*actual).next);
      (*actual).next = new_button (NULL);
      actual = (*actual).next;
    }
  (*current_folder).count++;
  (*actual).parent = current_folder;

  actual->title = ptr;

  /* remove duplications */
  for (button = (*current_folder).first; button != NULL; button = (*button).next)
    {
      if ((button != actual) && (strcmp ((*button).title, (*actual).title) == 0))
	{
	  delete_button (actual);
	  actual = button;
	  break;
	}
    }

  /* read next word. Its the icon bitmap/pixmap label. Users can specify ""
   * NoIcon, or whatever to skip the label */
  /* read to next space */
  start = end;
  /* skip spaces */
  while (isspace (*start) && (*start != '\n'))
    start++;
  end = start;
  while ((!isspace (*end)) && (*end != '\n') && (*end != 0))
    end++;
  len = end - start;
  ptr = safemalloc (len + 1);
  strncpy (ptr, start, len);
  ptr[len] = 0;
  /* separate icon files to be overlaid */
  i2 = len;
  j = k = 0;
  for (i = actual->num_icons; actual->num_icons < MAX_OVERLAY; i++)
  {
      while (ptr[j] != ',' && j < i2)	j++;
	  
	  filename = filename?realloc(filename, j - k + 1):safemalloc(j - k + 1);
      strncpy (filename, &(ptr[k]), j - k);
      filename[j-k] = 0;
	  if( j-k > 1 || filename[0] != '-' )
		  if( (actual->icons[actual->num_icons] = get_asimage( imman, filename, 0xFFFFFFFF, 100)) != NULL ) ;
		      actual->num_icons++;
      j++;
      k = j;
      if (j >= i2)
		break;
  }
  if( filename ) 
	  free(filename);
  free (ptr);
  tline = end;
  /* skip spaces */
  while (isspace (*tline) && (*tline != '\n'))
    tline++;
#ifdef ENABLE_DND
  if (!mystrncasecmp (tline, "dropexec", 8))
    {
      /* get command to execute for dropped stuff */

      tline = strstr (tline, "Exec");
      len = strlen (tline);
      tmp = tline + len - 1;
      while (((isspace (*tmp)) || (*tmp == '\n')) && (tmp >= tline))
	{
	  tmp--;
	  len--;
	}
      ptr = safemalloc (len + 1);
      actual->drop_action = ptr;
      strncpy (ptr, tline, len);
      ptr[len] = 0;
    }
  else
#endif
  if (mystrncasecmp (tline, "swallow", 7) == 0 || mystrncasecmp (tline, "maxswallow", 10) == 0)
    {
      int error = 0;
      start = tline;
      len = 0;
      if (!mystrncasecmp (tline, "maxswallow", 10))
	{
	  set_flags (actual->flags, WB_MaxSize);
	  tline += 10;
	}
      else
	{
	  clear_flags (actual->flags, WB_MaxSize);
	  tline += 7;
	}
      if (!mystrncasecmp (tline, "module", 6))
	{
	  set_flags (actual->flags, WB_Module);
	  tline += 6;
	  len += 7;
	}
      else
	{
	  clear_flags (actual->flags, WB_Module);
	  len += 5;
	}
      for (; (*tline != 0) && (*tline != '"'); tline++);
      if (*tline != '"')
	error = 1;
      if (!error)
	{
	  tline++;
	  for (i = 0; (tline[i] != 0) && (tline[i] != '"'); i++);
	  if (i <= 0 || tline[i] != '"')
	    error = 2;
	}
      if (!error)
	{
	  len += i + 3;
	  actual->hangon = mystrndup (tline, i);
	  actual->swallow = 1;
	  tline += i + 1;
	  for (; isspace (*tline); tline++);
	  tmp = tline + strlen (tline);
	  while ((tmp > tline) && isspace (tmp[-1]))
	    tmp--;
	  if (tmp - tline < 1)
	    error = 3;
	}
      if (!error)
	{
	  len += tmp - tline;
	  if (actual->swallow_command != NULL)
	    free (actual->swallow_command);
	  actual->swallow_command = safemalloc (len + 1);
	  sprintf (actual->swallow_command, "%s \"%s\" %.*s",
		   get_flags (actual->flags, WB_Module) ? "Module" : "Exec",
		   actual->hangon, (int) (tmp - tline), tline);
	  SendInfo (fd, actual->swallow_command, 0);
	}
      if (error)
	{
	  printf ("%s: error %d in line: %s", MyName, error, start);
	  clear_flags (actual->flags, WB_MaxSize | WB_Module);
	  if (actual->hangon != NULL)
	    free (actual->hangon);
	  actual->hangon = NULL;
	  actual->swallow = 0;
	}
    }
  else if (mystrncasecmp (tline, "Size", 4) == 0)
    {
      (*actual).width = strtol (tline + 4, &tmp, 10);
      (*actual).height = strtol (tmp, NULL, 10);
      if (((*actual).width > 0) && ((*actual).height > 0))
	set_flags ((*actual).flags, WB_UserSetSize);
    }
  else if (mystrncasecmp (tline, "Transient", 9) == 0)
    {
      set_flags (actual->flags, WB_Transient);
      if (actual->action != NULL)
	free (actual->action);
      actual->action = NULL;
    }
  else
    /* these commands set actual->action */
    {
      len = strlen (tline);
      tmp = tline + len - 1;
      while (((isspace (*tmp)) || (*tmp == '\n')) && (tmp >= tline))
	{
	  tmp--;
	  len--;
	}
      ptr = mystrndup (tline, len);

      if (mystrncasecmp (ptr, "Folder", 6) == 0)
	{
	  struct folder_info *folder = new_folder ();
	  (*folder).next = first_folder;
	  first_folder = folder;
	  current_folder = first_folder;
	  (*actual).folder = current_folder;
	  (*(*actual).folder).parent = actual;
	}

      if (get_flags (actual->flags, WB_Transient))
	{
	  free (ptr);
	}
      else
	{
	  if (actual->action != NULL)
	    free (actual->action);
	  actual->action = ptr;
	}
    }
  return;
}

/**************************************************************************
 *  Change the window name displayed in the title bar.
 **************************************************************************/
void
change_window_name (char *str)
{
  XTextProperty name;
  folder_info *folder;

  if (XStringListToTextProperty (&str, 1, &name) == 0)
    {
      fprintf (stderr, "%s: cannot allocate window name", MyName);
      return;
    }
  XSetWMName (dpy, (*root_folder).win, &name);
  XSetWMIconName (dpy, (*root_folder).win, &name);
  for (folder = first_folder; folder != NULL; folder = (*folder).next)
    {
      XSetWMName (dpy, (*folder).win, &name);
      XSetWMIconName (dpy, (*folder).win, &name);
    }
  XFree (name.value);
}



/***************************************************************************
 *
 * Waits for next X event, or for an auto-raise timeout.
 *
 ****************************************************************************/
int
WharfXNextEvent (Display * dpy, XEvent * event)
{
  fd_set in_fdset;
  unsigned long header[3];
  int count;
  static int miss_counter = 0;
  unsigned long *body;
  struct timeval tv;
  struct timeval *t = NULL;

  if (XPending (dpy))
    {
      XNextEvent (dpy, event);
      return 1;
    }

  FD_ZERO (&in_fdset);
  FD_SET (x_fd, &in_fdset);
  FD_SET (fd[1], &in_fdset);
  if (timer_delay_till_next_alarm ((time_t *) & tv.tv_sec, (time_t *) & tv.tv_usec))
    t = &tv;

#ifdef __hpux
  select (fd_width, (int *) &in_fdset, NULL, NULL, t);
#else
  select (fd_width, &in_fdset, NULL, NULL, t);
#endif

  if (FD_ISSET (x_fd, &in_fdset))
    {
      if (XPending (dpy))
	{
	  XNextEvent (dpy, event);
	  miss_counter = 0;
	  return 1;
	}
      else
	miss_counter++;
      if (miss_counter > 100)
	DeadPipe (0);
    }
  if (FD_ISSET (fd[1], &in_fdset))
    {
      if ((count = ReadASPacket (fd[1], header, &body)) > 0)
	{
	  process_message (header[1], body);
	  free (body);
	}
    }
  /* handle timeout events */
  timer_handle ();

  return 0;
}

void
CheckForHangon (unsigned long *body)
{
  folder_info *folder;
  button_info *button;
  char *cbody;
  Bool success = False;

  cbody = (char *) &body[3];
  /* first pass: make sure this window isn't being swallowed already */
  for (folder = first_folder; folder != NULL; folder = (*folder).next)
    for (button = (*folder).first; button != NULL; button = (*button).next)
      if ((*button).swallowed_win == (Window) body[0])
	return;

  /* second pass: start swallowing the app */
  for (folder = first_folder; folder != NULL; folder = (*folder).next)
    {
      for (button = (*folder).first; button != NULL; button = (*button).next)
	if ((*button).hangon != NULL)
	  {
	    if (strcmp (cbody, (*button).hangon) == 0)
	      {
		if ((*button).swallow == 1)
		  {
		    (*button).swallow = 2;
		    (*button).swallowed_win = (Window) body[0];
		  }
		success = True;
		break;
	      }
	  }
      if (success == True)
	break;
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
    case M_TOGGLE_PAGING:
      pageing_enabled = body[0];
	  update_transparency(root_folder);
//      RedrawWindow (root_folder, NULL);
      break;
    case M_NEW_DESK:
      new_desk = body[0];
	  update_transparency(root_folder);
//      RedrawWindow (root_folder, NULL);
      break;
    case M_END_WINDOWLIST:
//      RedrawWindow (root_folder, NULL);
      break;
    case M_MAP:
      swallow (body);
      break;
    case M_RES_NAME:
    case M_RES_CLASS:
    case M_WINDOW_NAME:
      CheckForHangon (body);
      break;
    default:
      break;
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	new_folder - malloc and setup a new folder
 *
 ***********************************************************************
 */
folder_info *
new_folder (void)
{
  folder_info *folder = (folder_info *) safemalloc (sizeof (folder_info));

  (*folder).next = NULL;
  (*folder).win = None;
  (*folder).parent = NULL;
  (*folder).first = NULL;
  (*folder).count = 0;
  (*folder).x = 0;
  (*folder).y = 0;
  (*folder).width = 1;
  (*folder).height = 1;
  (*folder).direction = DIR_TODOWN;
#ifdef SHAPE
  (*folder).mask = None;
#endif
  (*folder).flags = 0;

  return folder;
}


/***********************************************************************
 *
 *  Procedure:
 *	new_button - malloc (if necessary) and setup a new button
 *
 ***********************************************************************
 */
button_info *
new_button (button_info * button)
{
  int i;

  if (button == NULL)
    button = NEW (button_info);

  (*button).next = NULL;
  (*button).parent = NULL;
  (*button).folder = NULL;
  (*button).action = NULL;
  (*button).title = NULL;
  (*button).num_icons = 0;
  for (i = 0; i < MAX_OVERLAY; i++)
    (*button).icons[i] = NULL;
  (*button).x = 0;
  (*button).y = 0;
  (*button).width = 0;
  (*button).height = 0;
  (*button).completeIcon = NULL;
  (*button).mask = None ;
  (*button).IconWin = None;
  (*button).swallowed_win = None;
  (*button).hangon = NULL;
  (*button).swallow_command = NULL;
  (*button).swallow = 0;
#ifdef ENABLE_DND
  (*button).drop_action = NULL;
#endif
#ifdef SHAPE
  (*button).flags = 0;
#endif

  return button;
}

#ifdef DEBUG
/***********************************************************************
 *
 *  Procedure:
 *  print_folder_hierarchy - print the titles of all buttons in a folder
 *
 ***********************************************************************
 */

void
print_folder_hierarchy (folder_info * folder)
{
  static int depth = 0;
  button_info *button;

  printf ("%*sis folder (%d,%d,%d,%d)\n",
	  depth, "",
	  (*folder).x, (*folder).y, (*folder).width, (*folder).height);
  for (button = (*folder).first; button != NULL; button = (*button).next)
    {
      printf ("%*s'%s' at (%d,%d,%d,%d)\n",
	      depth, "", (*button).title,
	      (*button).x, (*button).y,
	      (*button).width, (*button).height);
      if ((*button).folder != NULL)
	{
	  depth++;
	  print_folder_hierarchy ((*button).folder);
	  depth--;
	}
    }
}

/***********************************************************************
 *
 *  Procedure:
 *  is_folder_consistent - make sure a folder has nothing wrong with it
 *
 ***********************************************************************
 */

Bool
is_folder_consistent (folder_info * folder)
{
  int i;
  button_info *button;
  folder_info *f;

  /* are we in the folder list? */
  for (f = first_folder; (f != NULL) && (f != folder); f = (*f).next);
  if (f == NULL)
    return False;

  /* does our parent think we're their child? */
  if (((*folder).parent != NULL) && ((*(*folder).parent).folder != folder))
    return False;

  /* do we have the right number of buttons? */
  for (button = (*folder).first, i = 0; button != NULL; i++, button = (*button).next);
  if (i != (*folder).count)
    return False;

  /* do our children think we're their parent? */
  for (button = (*folder).first; button != NULL; button = (*button).next)
    if ((*button).parent != folder)
      return False;

  /* are our subfolders consistent? */
  for (button = (*folder).first; button != NULL; button = (*button).next)
    if ((*button).folder != NULL)
      if (is_folder_consistent ((*button).folder) == False)
	return False;

  return True;
}
#endif /*DEBUG_CONSISTENCY_CHECK */

/***********************************************************************
 *
 *  Procedure:
 *	delete_folder - delete a folder
 *
 ***********************************************************************
 */
void
delete_folder (folder_info * folder)
{
  /* remove ourself from the folder list */
  if (folder == first_folder)
    first_folder = (*folder).next;
  else if (first_folder != NULL)
    {
      folder_info *f;
      for (f = first_folder; (*f).next != NULL; f = (*f).next)
	if ((*f).next == folder)
	  {
	    (*f).next = (*folder).next;
	    break;
	  }
    }

  /* tell our parent we're divorcing it */
  if ((*folder).parent != NULL)
    (*(*folder).parent).folder = NULL;

  /* delete our buttons */
  while ((*folder).first != NULL)
    delete_button ((*folder).first);

  /* delete our window */
  if ((*folder).win != None)
    XDestroyWindow (dpy, (*folder).win);

#ifdef SHAPE
  /* delete the shape mask */
  if ((*folder).mask != None)
    XFreePixmap (dpy, (*folder).mask);
#endif

  /* finally, free our own mem */
  free (folder);
}

/***********************************************************************
 *
 *  Procedure:
 *	delete_button - delete a button
 *
 ***********************************************************************
 */
void
delete_button (button_info * button)
{
  int i;

  /* delete subfolders */
  if ((*button).folder != NULL)
    delete_folder ((*button).folder);

  /* remove ourselves from our parent's list */
  /* warning! this does not modify rows & cols at all */
  if ((*button).parent != NULL)
    {
      button_info *ptr1;
      button_info *ptr2;

      ptr1 = (*(*button).parent).first;
      ptr2 = NULL;
      while (ptr1 != NULL)
	{
	  if (ptr1 == button)
	    {
	      if (ptr2 == NULL)
		(*(*button).parent).first = (*ptr1).next;
	      else
		(*ptr2).next = (*ptr1).next;
	      (*(*button).parent).count--;
	      break;
	    }
	  ptr2 = ptr1;
	  ptr1 = (*ptr1).next;
	}
    }
  /* delete the action & title */
  if ((*button).action != NULL)
    free ((*button).action);
  if ((*button).title != NULL)
    free ((*button).title);

  /* delete the icons */
  for (i = 0; i < (*button).num_icons; i++)
	if( (*button).icons[i] )
	  destroy_asimage( &(button->icons[i]) );

  if ((*button).completeIcon)
      destroy_asimage(&((*button).completeIcon));
	  
  if( (*button).mask  )
	  XFreePixmap( dpy, (*button).mask );

  /* delete swallowed windows, but not modules (AfterStep handles those) */
  if (button->swallowed_win != None && !get_flags (button->flags, WB_Module))
    {
      send_clientmessage ((*button).swallowed_win, _XA_WM_DELETE_WINDOW, CurrentTime);
      XSync (dpy, 0);
    }
  /* delete the icon window */
  if ((*button).IconWin != None)
    {
      balloon_delete (balloon_find ((*button).IconWin));
      XDestroyWindow (dpy, (*button).IconWin);
    }

  if ((*button).hangon != NULL)
    free ((*button).hangon);

  if ((*button).swallow_command != NULL)
    free ((*button).swallow_command);

#ifdef ENABLE_DND
  if ((*button).drop_action != NULL)
    free ((*button).drop_action);
#endif

  /* finally, free our own mem */
  free (button);
}


/***************************************************************************
 *
 * ICCCM Client Messages - Section 4.2.8 of the ICCCM dictates that all
 * client messages will have the following form:
 *
 *     event type	ClientMessage
 *     message type	_XA_WM_PROTOCOLS
 *     window		tmp->w
 *     format		32
 *     data[0]		message atom
 *     data[1]		time stamp
 *
 ****************************************************************************/
void
send_clientmessage (Window w, Atom a, Time timestamp)
{
  XClientMessageEvent ev;

  ev.type = ClientMessage;
  ev.window = w;
  ev.message_type = _XA_WM_PROTOCOLS;
  ev.format = 32;
  ev.data.l[0] = a;
  ev.data.l[1] = timestamp;
  XSendEvent (dpy, w, False, 0L, (XEvent *) & ev);
}


void
swallow (unsigned long *body)
{
  folder_info *folder;
  button_info *button;
  Window root;
  int width, height, junk;
  long supplied;

  for (folder = first_folder; folder != NULL; folder = (*folder).next)
    for (button = (*folder).first; button != NULL; button = (*button).next)
      if (((*button).swallowed_win == (Window) body[0]) &&
	  ((*button).swallow == 2))
	{
	  unsigned *mods = lock_mods;

	  (*button).swallow = 3;
	  /* watch for border width changes */
	  XSelectInput (dpy, (*button).swallowed_win, StructureNotifyMask);
	  /* "Swallow" the window! */
	  XReparentWindow (dpy, (*button).swallowed_win, (*button).IconWin,
			   -999, -999);
	  XSync (dpy, 0);
	  XMapWindow (dpy, (*button).swallowed_win);
	  do
	    {
	      /* grab button 1 if this button performs an action */
	      if ((*button).action != NULL)
		XGrabButton (dpy, Button1, *mods,
			     (*button).swallowed_win,
			     False, ButtonPressMask | ButtonReleaseMask,
			     GrabModeAsync, GrabModeAsync, None, None);
	      /* grab button 3 if this is the root folder */
	      if (DoWithdraw && (*button).parent != NULL && (*(*button).parent).parent == NULL)
		XGrabButton (dpy, Button3, *mods,
			     (*button).swallowed_win,
			     False, ButtonPressMask | ButtonReleaseMask,
			     GrabModeAsync, GrabModeAsync, None, None);
	    }
	  while (*mods++);
	  /* find the size of the swallowed window so we can center it */
	  XGetGeometry (dpy, (*button).swallowed_win,
			&root, &junk, &junk,
			&width, &height,
			&junk, &junk);
	  if (!get_flags ((*button).flags, WB_UserSetSize) &&
	      get_flags ((*button).flags, WB_MaxSize))
	    {
	      (*button).width = width;
	      (*button).height = height;
	      XMoveWindow (dpy, (*button).swallowed_win, 0, 0);
	      place_buttons ((*button).parent);
	      place_folders ((*button).parent);
	    }
	  else
	    {
	      width = (width < (*button).width) ? width : (*button).width;
	      height = (height < (*button).height) ? height : (*button).height;
	      XMoveResizeWindow (dpy, (*button).swallowed_win,
				 ((*button).width - width) / 2,
				 ((*button).height - height) / 2,
				 width, height);
#ifdef SHAPE
	      place_buttons ((*button).parent);
	      place_folders ((*button).parent);
#endif /* SHAPE */
	    }
	  /* try to set the window border width; doesn't work very well,
	   * so we do it again when we get the window ConfigureNotify */
	  XSetWindowBorderWidth (dpy, (*button).swallowed_win, 0);
	  if (!XGetWMNormalHints (dpy, (*button).swallowed_win,
				  &(*button).hints,
				  &supplied))
	    (*button).hints.flags = 0;

//	  RedrawWindow (folder, NULL);
	}
}



void
FindLockMods (void)
{
  int m, i, knl;
  char *kn;
  KeySym ks;
  KeyCode kc, *kp;
  unsigned lockmask, *mp;
  XModifierKeymap *mm = XGetModifierMapping (dpy);
  lockmask = LockMask;
  if (mm)
    {
      kp = mm->modifiermap;
      for (m = 0; m < 8; m++)
	{
	  for (i = 0; i < mm->max_keypermod; i++)
	    {
	      if ((kc = *kp++) &&
		  ((ks = XKeycodeToKeysym (dpy, kc, 0)) != NoSymbol))
		{
		  kn = XKeysymToString (ks);
		  knl = strlen (kn);
		  if ((knl > 6) && (mystrcasecmp (kn + knl - 4, "lock") == 0))
		    lockmask |= (1 << m);
		}
	    }
	}
      XFreeModifiermap (mm);
    }
  lockmask &= ~(ShiftMask | ControlMask);

  mp = lock_mods;
  for (m = 0, i = 1; i < 256; i++)
    {
      if ((i & lockmask) > m)
	m = *mp++ = (i & lockmask);
    }
  *mp = 0;
}


/***********************************************************************
 *
 *  Procedure:
 *      ConstrainSize - adjust the given width and height to account for the
 *              constraints imposed by size hints
 *
 *      The general algorithm, especially the aspect ratio stuff, is
 *      borrowed from uwm's CheckConsistency routine.
 *
 ***********************************************************************/
void
ConstrainSize (XSizeHints * hints, int *widthp, int *heightp)
{
#define makemult(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
#define _min(a,b) (((a) < (b)) ? (a) : (b))


  int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
  int baseWidth, baseHeight;
  int dwidth = *widthp, dheight = *heightp;

  if (hints->flags & PMinSize)
    {
      minWidth = hints->min_width;
      minHeight = hints->min_height;
      if (hints->flags & PBaseSize)
	{
	  baseWidth = hints->base_width;
	  baseHeight = hints->base_height;
	}
      else
	{
	  baseWidth = hints->min_width;
	  baseHeight = hints->min_height;
	}
    }
  else if (hints->flags & PBaseSize)
    {
      minWidth = hints->base_width;
      minHeight = hints->base_height;
      baseWidth = hints->base_width;
      baseHeight = hints->base_height;
    }
  else
    {
      minWidth = 1;
      minHeight = 1;
      baseWidth = 1;
      baseHeight = 1;
    }

  if (hints->flags & PMaxSize)
    {
      maxWidth = hints->max_width;
      maxHeight = hints->max_height;
    }
  else
    {
      maxWidth = 10000;
      maxHeight = 10000;
    }
  if (hints->flags & PResizeInc)
    {
      xinc = hints->width_inc;
      yinc = hints->height_inc;
    }
  else
    {
      xinc = 1;
      yinc = 1;
    }

  /*
   * First, clamp to min and max values
   */
  if (dwidth < minWidth)
    dwidth = minWidth;
  if (dheight < minHeight)
    dheight = minHeight;

  if (dwidth > maxWidth)
    dwidth = maxWidth;
  if (dheight > maxHeight)
    dheight = maxHeight;


  /*
   * Second, fit to base + N * inc
   */
  dwidth = ((dwidth - baseWidth) / xinc * xinc) + baseWidth;
  dheight = ((dheight - baseHeight) / yinc * yinc) + baseHeight;


  /*
   * Third, adjust for aspect ratio
   */
#define maxAspectX hints->max_aspect.x
#define maxAspectY hints->max_aspect.y
#define minAspectX hints->min_aspect.x
#define minAspectY hints->min_aspect.y
  /*
   * The math looks like this:
   *
   * minAspectX    dwidth     maxAspectX
   * ---------- <= ------- <= ----------
   * minAspectY    dheight    maxAspectY
   *
   * If that is multiplied out, then the width and height are
   * invalid in the following situations:
   *
   * minAspectX * dheight > minAspectY * dwidth
   * maxAspectX * dheight < maxAspectY * dwidth
   *
   */

  if (hints->flags & PAspect)
    {
      if (minAspectX * dheight > minAspectY * dwidth)
	{
	  delta = makemult (minAspectX * dheight / minAspectY - dwidth,
			    xinc);
	  if (dwidth + delta <= maxWidth)
	    dwidth += delta;
	  else
	    {
	      delta = makemult (dheight - dwidth * minAspectY / minAspectX,
				yinc);
	      if (dheight - delta >= minHeight)
		dheight -= delta;
	    }
	}
      if (maxAspectX * dheight < maxAspectY * dwidth)
	{
	  delta = makemult (dwidth * maxAspectY / maxAspectX - dheight,
			    yinc);
	  if (dheight + delta <= maxHeight)
	    dheight += delta;
	  else
	    {
	      delta = makemult (dwidth - maxAspectX * dheight / maxAspectY,
				xinc);
	      if (dwidth - delta >= minWidth)
		dwidth -= delta;
	    }
	}
    }
  *widthp = dwidth;
  *heightp = dheight;
  return;
}


#ifdef ENABLE_SOUND
void
PlaySound (int event)
{
  int timestamp;

  if (!SoundActive)
    return;
  if (Sounds[event] == NULL)
    return;
  write (PlayerChannel[1], &event, sizeof (event));
  timestamp = clock ();
  write (PlayerChannel[1], &timestamp, sizeof (timestamp));
  /*
     kill(SoundThread,SIGUSR1);
   */
}
#endif

int
x_error_handler (Display * dpy, XErrorEvent * error)
{
  fprintf (stderr, "%s: X Error\n"
	   "%*s  Request %d, Error %d %d, Type: %d\n",
	   MyName,
	   strlen (MyName), "", error->request_code, error->error_code, error->minor_code, error->type);
  return 0;
}
