/*
 * Copyright (c) 1998 Sasha Vasko <sasha at aftercode.net>
 * Original idea: Copyright (c) 1998 Rafal Wierzbicki <rafal@mcss.mcmaster.ca>
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

#define TRUE 1
#define FALSE 0

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

/*#define DO_CLOCKING */
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
#include "../../include/mystyle.h"
#include "../../include/screen.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"
#include "../../include/module.h"
#include "../../include/ascolor.h"
#include "../../include/stepgfx.h"
#include "../../include/XImage_utils.h"
#include "../../include/pixmap.h"
#include "../../include/loadimg.h"
#include "../../include/background.h"

#ifdef DEBUG_ASETROOT
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


#ifdef DEBUG_ALLOCS
#undef XFreePixmap
#endif

/*************************************************************************
 *
 * Screen, font, etc info
 *
 **************************************************************************/
ASPipes Pipes;
ASAtom Atoms[] =
{XROOTPMAP_ID_ATOM_DEF,
 {None, "_AS_STYLE", XA_INTEGER},
 AS_BACKGROUND_ATOM_DEF,
 {None, "WM_STATE", XA_INTEGER},	/*  it is actually of WM_STATE type,
					   but we don't know this atom untill
					   we get it from X */
 {None, NULL, None}
};

#define ROOTPIXMAP_ATOM 	0
#define MYSTYLE_ATOM 		1
#define BACKGROUND_ATOM		2
#define WM_STATE_ATOM		3


char *MyName;
ScreenInfo Scr;			/* AS compatible screen information structure */
Display *dpy;			/* which display are we talking to */
int screen;

char *pixmapPath = NULL;

ASDeskBackArray DesksArray =
{NULL, 0};

#define ASETROOT_CONFIG "asetroot"

void
usage (void)
{
  printf ("Usage:\n%s [--version] [--help] [n [m]] [--loadonly|-l]|[--kill|-k]\n", MyName);
  printf ("If started without parameters, %s will load all configured backgrounds\n", MyName);
  printf ("into memory, and will be switching backgrounds for all desks.\n");
  printf ("You can limit number of desks that will be served, by specifying\n");
  printf ("lower range (n), and optionally upper range(m).\n\n");
  printf ("Note: if you only want to load images into memory and use other \n");
  printf ("      module to switch them when desk is changed, use --loadonly(-l) option.\n");
  printf ("      Later on you will be able to remove loaded images from the memory\n");
  printf ("      by running asetroot with --kill(-k) option.\n");

  exit (0);
}

void
ShowAsStarted ()
{
  XTextProperty name;
  Window w;
  XClassHint class1;
  XSizeHints shints;
  int waiting_for_withdrawal = 1;

  w = XCreateSimpleWindow (dpy, Scr.Root, -10000, -10000, 1, 1, 1, 0, 0);

  XStringListToTextProperty (&MyName, 1, &name);

  class1.res_name = MyName;	/* for future use */
  class1.res_class = "ASModule";

  shints.flags = USPosition | USSize;
  shints.min_width = shints.min_height = 4;
  shints.max_width = shints.max_height = 5;
  shints.base_width = shints.base_height = 4;

  XSetWMProperties (dpy, w, &name, &name, NULL, 0, &shints, NULL, &class1);
  /* showing window to let user see that we are doing something */
  XMapRaised (dpy, w);
  /* final cleanup */
  XFree ((char *) name.value);
  XFlush (dpy);
  sleep (1);			/* we have to give AS a chance to spot us */
  /* we will need to wait for PropertyNotify event indicating transition
     into Withdrawn state, so selecting event mask: */
  XSelectInput (dpy, w, PropertyChangeMask);

  XUnmapWindow (dpy, w);
  /* according to ICCCM we must follow this with synthetic UnmapNotify event */
  {
    XEvent e;
    e.xunmap.type = UnmapNotify;
    e.xunmap.event = Scr.Root;
    e.xunmap.window = w;
    e.xunmap.from_configure = False;

    XSendEvent (dpy, Scr.Root, False, SubstructureRedirectMask | SubstructureNotifyMask, &e);

    while (waiting_for_withdrawal)
      {
	XNextEvent (dpy, &e);
	if (e.type == PropertyNotify && e.xproperty.atom == Atoms[WM_STATE_ATOM].atom)
	  {
	    Atom act_type;
	    int act_format;
	    unsigned long nitems, bytes_after;
	    unsigned char *prop = NULL;

	    if (e.xproperty.state == PropertyDelete)
	      {
		break;
	      }
	    if (XGetWindowProperty (dpy, w, Atoms[WM_STATE_ATOM].atom, 0, 1,
				    False, Atoms[WM_STATE_ATOM].atom, &act_type, &act_format, &nitems,
				    &bytes_after, &prop) == Success)
	      if (prop)
		{
		  if (*prop == WithdrawnState)
		    waiting_for_withdrawal = 0;
		  XFree (prop);
		}
	  }
      }
    /* we are in withdrawn state now - can safely destroy our window */
  }

  XSelectInput (dpy, w, 0);
  XDestroyWindow (dpy, w);
  XFlush (dpy);
}


void DeadPipe (int nonsense);
void process_message (unsigned long type, unsigned long *body);
void list_new_desk (unsigned long *body);
void list_new_background (unsigned long *body);
void list_unknown (unsigned long *body);
int IsDeskInRange (long desks);
void DeleteMyBackgrounds (ASDeskBackArray * backs, int verbose);
void UpdateBackgroundProperty ();
void GetOptions (const char *filename);
void GetBaseOptions (const char *filename);
void
DispatchEvent (XEvent * Event)
{
};

typedef struct
  {
    char *global_config_file;
    int desk1, desk2;
#define ACTION_DEFAULT	0
#define ACTION_LOADONLY	1
#define ACTION_CLEANUP	2
    int action;
  }
ASetRootEnvironment;

ASetRootEnvironment MyEnv =
{NULL, -1, -1, ACTION_DEFAULT};

/***********************************************************************
 *
 *   Procedure:
 *   main - start of module
 *
 ***********************************************************************/
int
main (int argc, char **argv)
{
  int i, desks[2] =
  {-1, -1}, curr_desk = 0;

  /* it's too complicated to track when to purge pixmap cache in case
     of changed backgrounds, so we just turn it off completely .
     Methods to avoid image loading from the same file twice are
     built in to the asetroot configuration file !!!!!
   */
  set_use_pixmap_ref (FALSE);

  /* Save our program name - for error messages */
  SetMyName (argv[0]);

  i = ProcessModuleArgs (argc, argv, &(MyEnv.global_config_file), NULL, NULL, usage);
  for (; i < argc; i++)
    {
      char *cptr = argv[i];
      int bad_option;
      while (*cptr)
	{
	  bad_option = 0;
	  while (isspace (*cptr))
	    cptr++;
	  if (*cptr == '\0')
	    break;
	  if (isdigit (*cptr))
	    {
	      if (curr_desk < 2)
		{
		  desks[curr_desk] = atoi (cptr);
		  curr_desk++;
		}
	      else
		bad_option = 1;
	    }
	  else if (*cptr == '-')
	    {
	      if (*(++cptr) == '-')
		{
		  if (mystrncasecmp (cptr, "loadonly", 8) == 0)
		    MyEnv.action = ACTION_LOADONLY;
		  else if (mystrncasecmp (cptr, "kill", 4) == 0)
		    MyEnv.action = ACTION_CLEANUP;
		  else
		    bad_option = 1;
		}
	      else if (*(cptr + 1) == ' ' || *(cptr + 1) == '\0')
		{
		  if (*cptr == 'l')
		    MyEnv.action = ACTION_LOADONLY;
		  else if (*cptr == 'k')
		    MyEnv.action = ACTION_CLEANUP;
		  else
		    bad_option = 1;
		  cptr++;
		}
	      else
		bad_option = 1;
	    }
	  else
	    bad_option = 1;

	  if (bad_option)
	    {
	      fprintf (stderr, "\n%s: unknown option %s!", MyName, cptr);
	      usage ();
	    }
	  while (!isspace (*cptr) && *cptr != '\0')
	    cptr++;
	}
    }

  if (desks[1] >= desks[0])
    {
      MyEnv.desk1 = desks[0];
      MyEnv.desk2 = desks[1];
    }
  else
    {
      MyEnv.desk1 = desks[1];
      MyEnv.desk2 = desks[0];
    }
  /* Dead pipe == dead AfterStep */
  signal (SIGPIPE, DeadPipe);
  signal (SIGQUIT, DeadPipe);
  signal (SIGSEGV, DeadPipe);
  signal (SIGTERM, DeadPipe);
  signal (SIGKILL, DeadPipe);

  LOG2 ("\n%s connecting to X ...", MyName)
    Pipes.x_fd = ConnectX (&Scr, display_name, 0);
  InitAtoms (dpy, Atoms);
  /* enable root pixmap property setting */
  SetRootPixmapPropertyID (Atoms[ROOTPIXMAP_ATOM].atom);

  if (MyEnv.action == ACTION_DEFAULT)
    Pipes.fd[0] = Pipes.fd[1] = ConnectAfterStep (M_NEW_DESK | M_NEW_BACKGROUND);

  LOG2 ("\n%s is parsing Options ...", MyName)
    if (MyEnv.action != ACTION_CLEANUP)
    {
      LoadBaseConfig (MyEnv.global_config_file, GetBaseOptions);
      LoadConfig (MyEnv.global_config_file, ASETROOT_CONFIG, GetOptions);
    }

  LOG2 ("\n%s starting The Loop ...", MyName)
    if (MyEnv.action == ACTION_LOADONLY)
    {
      XSetCloseDownMode (dpy, RetainPermanent);
      if (GetRootPixmap (Atoms[ROOTPIXMAP_ATOM].atom) == None)
	BackgroundSetForDesk (&DesksArray, (MyEnv.desk1 >= 0) ? MyEnv.desk1 : 0);
      ShowAsStarted ();
      DeadPipe (0);
    }
  else if (MyEnv.action == ACTION_CLEANUP)
    {
      Pixmap pix = None;
      GetBackgroundsProperty (&DesksArray, Atoms[BACKGROUND_ATOM].atom);
      if (DesksArray.desks_num > 0)
	{
	  if ((pix = GetRootPixmap (Atoms[ROOTPIXMAP_ATOM].atom)) != None)
	    {
	      for (i = 0; i < DesksArray.desks_num; i++)
		if (DesksArray.desks[i].data_type == XA_PIXMAP && pix == DesksArray.desks[i].data.pixmap)
		  {
		    pix = None;
		    break;
		  }
	      if (pix == None);
	      XChangeProperty (dpy, Scr.Root, Atoms[ROOTPIXMAP_ATOM].atom, XA_PIXMAP, 32, PropModeReplace, (char *) &pix, 1);
	    }
	  XDeleteProperty (dpy, Scr.Root, Atoms[BACKGROUND_ATOM].atom);
	  XFlush (dpy);
	  sleep (1);
	  /* now proceed to freeing all the pixmaps */
	  DeleteMyBackgrounds (&DesksArray, 1);
	}
      ShowAsStarted ();
      XCloseDisplay (dpy);
    }
  else
    {
      XEvent event;
      /* strange things may happen if two different asetroots are servicing
         same range of desks. but to reduce risks - we add this code : */
      Scr.CurrentDesk = (MyEnv.desk1 >= 0) ? MyEnv.desk1 : 0;
      if (GetRootPixmap (Atoms[ROOTPIXMAP_ATOM].atom) == None)
	BackgroundSetForDesk (&DesksArray, Scr.CurrentDesk);

      ShowAsStarted ();
      while (1)
	if (My_XNextEvent (dpy, Pipes.x_fd, Pipes.fd[1], process_message, &event))
	  if (event.xproperty.atom == Atoms[BACKGROUND_ATOM].atom)
	    {
	      ASDeskBackArray current =
	      {NULL, 0};
	      ASDeskBack *back;
	      GetBackgroundsProperty (&current, Atoms[BACKGROUND_ATOM].atom);
	      for (i = 0; i < DesksArray.desks_num; i++)
		if ((back = FindDeskBack (&current, DesksArray.desks[i].desk)) != NULL)
		  DesksArray.desks[i] = *back;
	      FreeDeskBackArray (&current, FALSE);
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
  unsigned long *body_to_use = NULL;
  LOG4 ("\n%s: received AfterStep message #%ld, body[0]=%lu", MyName, type, body[0])

    while (type == M_NEW_DESK)
    {
      ASMessage *next_msg;
      if ((next_msg = CheckASMessage (Pipes.fd[1], 0)) == NULL)
	break;
      type = next_msg->header[1];
      LOG4 ("\n%s: received AfterStep message #%ld, body[0]=%lu", MyName, type, next_msg->body[0])
	if (type != M_NEW_DESK)
	list_new_desk ((body_to_use) ? body_to_use : body);
      if (body_to_use)
	free (body_to_use);
      body_to_use = next_msg->body;
      free (next_msg);
    }
  if (body_to_use == NULL)
    body_to_use = body;

  switch (type)
    {
    case M_NEW_DESK:
      list_new_desk (body);
      break;
    case M_NEW_BACKGROUND:
      list_new_background (body);
      break;
    default:
      list_unknown (body);
      break;
    }
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
/* here we must remove our backgrounds from backgrounds information list
   as they are no longer valid */
  ASDeskBackArray dummy =
  {NULL, 0}, *saved;
  int i = 0;

  /* merely duplicating data, as we want to destroy old pixmaps,
     only after setting all X properties to None        */
  if (MyEnv.action == ACTION_DEFAULT)
    {
      saved = UpdateDeskBackArray (&dummy, &DesksArray);
      for (i = 0; i < DesksArray.desks_num; i++)
	{
	  if (DesksArray.desks[i].data_type == XA_PIXMAP ||
	      DesksArray.desks[i].MyStyle != None)
	    DesksArray.desks[i].data.pixmap = None;
	}
      UpdateBackgroundProperty ();
      free (DesksArray.desks);
      FreeDeskBackArray (saved, TRUE);
      free (saved);
    }
  else
    {
      FreeDeskBackArray (&DesksArray, FALSE);
    }

#ifdef DEBUG_ALLOCS
/* normally, we let the system clean up, but when auditing time comes
 * around, it's best to have the books in order... */
  {
    free (pixmapPath);
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

  XFlush (dpy);
  XCloseDisplay (dpy);
  exit (0);
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
  LOG2 ("\n%s:list_new_desk():\tDone!", MyName)
    if (body[0] != 10000)
    {
      Scr.CurrentDesk = body[0];
      if (IsDeskInRange (Scr.CurrentDesk))
	BackgroundSetForDesk (&DesksArray, Scr.CurrentDesk);
    }
}

/***********************************************************************
 *
 *   Procedure:
 *         list_new_background - reread config file & reconfigure on fly
 *
 ***********************************************************************/
void
list_new_background (unsigned long *body)
{
/* need to add code here to reread config file */
  ASDeskBackArray dummy =
  {NULL, 0}, *saved;

  /* merely duplicating data, as we want to destroy old pixmaps,
     only after setting all X properties to updated data        */
  saved = UpdateDeskBackArray (&dummy, &DesksArray);
  /* that will also set X property for backgrounds info for us */
  LoadConfig (MyEnv.global_config_file, ASETROOT_CONFIG, GetOptions);
  /* informing everybody about new root background */
#ifdef DEBUG_ASETROOT
  fprintf (stderr, "setting new desk background to: \n");
  PrintDeskBackArray (&DesksArray);
#endif
  BackgroundSetForDesk (&DesksArray, Scr.CurrentDesk);

  if (saved)
    {
      long saved_curr_desk = Scr.CurrentDesk;
      /* we don't want to reset root pixmap ID property, don't we ? */
      Scr.CurrentDesk = 10000;
      FreeDeskBackArray (saved, TRUE);
      Scr.CurrentDesk = saved_curr_desk;
      free (saved);
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

/******************************************************************/
/* configuration reading stuff                                    */
/******************************************************************/
int
IsDeskInRange (long desk)
{
  return ((MyEnv.desk1 < 0 || desk >= MyEnv.desk1) &&
	  (MyEnv.desk2 < 0 || desk <= MyEnv.desk2));
}

void
DeleteMyBackgrounds (ASDeskBackArray * backs, int verbose)
{
  Pixmap *deleted_pixmaps;
  register int i, k;
  if (backs->desks_num <= 0)
    return;
  deleted_pixmaps = (Pixmap *) safemalloc (backs->desks_num * sizeof (Pixmap));
  for (i = 0; i < backs->desks_num; i++)
    deleted_pixmaps[i] = None;

  if (verbose)
    fprintf (stderr, "\n");
  for (i = 0; i < backs->desks_num; i++)
    if (IsDeskInRange (backs->desks[i].desk) &&
	IsPurePixmap (&(backs->desks[i])))
      {
	for (k = 0; k < backs->desks_num; k++)
	  if (deleted_pixmaps[k] == backs->desks[i].data.pixmap)
	    {
	      backs->desks[i].data.pixmap = None;
	      break;
	    }
	if (backs->desks[i].data.pixmap != None)
	  {
	    int skip = 0;
	    if (ValidatePixmap (backs->desks[i].data.pixmap, True, False, NULL, NULL) != None)
	      XFreePixmap (dpy, backs->desks[i].data.pixmap);
	    else
	      {
		skip = 1;
		if (verbose)
		  fprintf (stderr, "%s: invalid root background pixmap %ld from the desk #%ld - skipping.\n",
		  MyName, backs->desks[i].data.pixmap, backs->desks[i].desk);
	      }

	    for (k = 0; k < backs->desks_num; k++)
	      if (deleted_pixmaps[k] == None)
		{
		  deleted_pixmaps[k] = backs->desks[i].data.pixmap;
		  break;
		}
	    if (verbose && !skip)
	      fprintf (stderr, "%s: Freed root background pixmap %ld from the desk #%ld.\n",
		 MyName, backs->desks[i].data.pixmap, backs->desks[i].desk);
	    XFlush (dpy);
	    backs->desks[i].data.pixmap = None;
	  }
	else if (verbose)
	  fprintf (stderr, "%s: already freed root background pixmap %ld from the desk #%ld.\n",
		   MyName, deleted_pixmaps[k], backs->desks[i].desk);
      }
  free (deleted_pixmaps);
}

void
UpdateBackgroundProperty ()
{
  ASDeskBackArray current =
  {NULL, 0};
  ASDeskBackArray *updated;
/* making sure that only one client at a time updates backgrounds property */
  XGrabServer (dpy);
  GetBackgroundsProperty (&current, Atoms[BACKGROUND_ATOM].atom);

  PrintDeskBackArray (&current);
  PrintDeskBackArray (&DesksArray);
  if (MyEnv.action == ACTION_LOADONLY)
    {				/* we need to destroy all the backgrounds in our range */
      int i, k;
#if      1
      for (i = 0; i < current.desks_num; i++)
	if (IsDeskInRange (current.desks[i].desk) &&
	    IsPurePixmap (&(current.desks[i])))
	  {
	    ASDeskBack *back;
	    /* this background is about to be deleted -
	       let's see if anyone else is using it too : */
	    for (k = 0; k < current.desks_num; k++)
	      if (!IsDeskInRange (current.desks[k].desk) &&
		  IsPurePixmap (&(current.desks[k])) &&
	       current.desks[k].data.pixmap == current.desks[i].data.pixmap)
		{
		  back = FindDeskBack (&DesksArray, current.desks[i].desk);
		  if (back)
		    {
		      current.desks[k].data = back->data;
		      current.desks[k].data_type = back->data_type;
		      current.desks[k].MyStyle = back->MyStyle;;
		    }
		}
	  }
#endif
      DeleteMyBackgrounds (&current, 0);

    }

  if ((updated = UpdateDeskBackArray (&current, &DesksArray)) != NULL)
    {
      PrintDeskBackArray (updated);
      SetBackgroundsProperty (updated, Atoms[BACKGROUND_ATOM].atom);
      FreeDeskBackArray (updated, FALSE);
      free (updated);
      XFlush (dpy);
    }
  if (current.desks)
    free (current.desks);
  XUngrabServer (dpy);
}



void
DuplicateDeskBack (ASDeskBack * trg, ASDeskBack * src)
{
  trg->data_type = src->data_type;
  trg->data = src->data;
  trg->MyStyle = src->MyStyle;
}

/* just a stub so far */
Pixmap
DoTransformPixmap (Pixmap src, MyBackgroundConfig * back)
{
/* create GC */
  GC gc;
  XGCValues values;
  unsigned long mask = 0;
  Pixmap trg;
  int junk;
  Window root;
  unsigned int width, height;
  int x = 0, y = 0;
  ShadingInfo *shading = NULL;
  unsigned int screen_width = Scr.MyDisplayWidth;
  unsigned int screen_height = Scr.MyDisplayHeight;

  if (src == None)
    return src;
  if (back->flags & BGFLAG_PAD)
    {
      XColor pad_col;
      XParseColor (dpy, DefaultColormap (dpy, Scr.screen), back->pad, &pad_col);
      MyAllocColor (&pad_col);
      values.foreground = pad_col.pixel;
      mask = GCForeground;
    }
  if ((gc = XCreateGC (dpy, Scr.Root, mask, &values)) == None)
    return src;
  /* cut and tint */
  XGetGeometry (dpy, src, &root, &junk, &junk, &width, &height, &junk, &junk);

  if (back->flags & BGFLAG_TINT)
    {
      shading = (ShadingInfo *) safemalloc (sizeof (ShadingInfo));
      INIT_SHADING ((*shading))
	XParseColor (dpy, DefaultColormap (dpy, Scr.screen), back->tint, &(shading->tintColor));
    }

  if (back->flags & BGFLAG_CUT)
    {
      x = (back->cut.x < width) ? back->cut.x : 0;
      y = (back->cut.y < height) ? back->cut.y : 0;
      if (back->cut.width >= width - x || back->cut.width <= 0)
	width -= x;
      else
	width = back->cut.width;
      if (back->cut.height >= height - x || back->cut.height <= 0)
	height -= y;
      else
	height = back->cut.height;
    }
  if (back->flags & BGFLAG_CUT || shading)
    {
      if ((trg = ShadePixmap (src, x, y, width, height, gc, shading)) != None)
	{
	  UnloadImage (src);
	  src = trg;
	}
    }
  if (shading)
    free (shading);
	
  if( Scr.xinerama_screens && Scr.xinerama_screens_num > 1 ) 
  {
	  register int i = Scr.xinerama_screens_num ;
	  while( --i > 0 )
		  if( Scr.xinerama_screens[i].x == 0 && Scr.xinerama_screens[i].y == 0 ) 
			  break ;
	  screen_width = Scr.xinerama_screens[i].width ;
	  screen_height = Scr.xinerama_screens[i].height ;
  }	
	
  /* scale */
  if (back->flags & BGFLAG_SCALE)
    {
      unsigned int old_width = width, old_height = height;

      if (back->scale.width > 0)
	width = back->scale.width;
      if (back->scale.height > 0)
	height = back->scale.height;
      if (back->scale.width <= 0 && back->scale.height <= 0)
	{
	  width = screen_width;
	  height = screen_height;
	}

      if ((trg = ScalePixmap (src, old_width, old_height, width, height, gc, NULL)) != None)
	{
	  UnloadImage (src);
	  src = trg;
	}
    }
  /* align + pad   */
  if (back->flags & BGFLAG_PAD)
    {
      unsigned int old_width = width, old_height = height;

      if (back->flags & BGFLAG_PAD_HOR)
	width = screen_width;
      if (back->flags & BGFLAG_PAD_VERT)
	height = screen_height;
      if (!(back->flags & BGFLAG_PAD_HOR) && !(back->flags & BGFLAG_PAD_VERT))
	{
	  width = screen_width;
	  height = screen_height;
	}

      x = (back->flags & BGFLAG_ALIGN_RIGHT) ? width - old_width : 0;
      y = (back->flags & BGFLAG_ALIGN_BOTTOM) ? height - old_height : 0;
      if ((back->flags & BGFLAG_ALIGN) &&
	  !(back->flags & BGFLAG_ALIGN_BOTTOM) &&
	  !(back->flags & BGFLAG_ALIGN_RIGHT))
	{
	  x = (width - old_width) / 2;
	  y = (height - old_height) / 2;
	}

      if ((trg = XCreatePixmap (dpy, Scr.Root, width, height, Scr.d_depth)) != None)
	{
	  XFillRectangle (dpy, trg, gc, 0, 0, width, height);
	  if (old_width > width)
	    old_width = width;
	  if (old_height > height)
	    old_height = height;
	  XCopyArea (dpy, src, trg, gc, 0, 0, old_width, old_height, x, y);
	  UnloadImage (src);
	  src = trg;
	}
    }

  XFreeGC (dpy, gc);
  return src;
}

/* loading background information and converting it into useble form */
void
ProcessNewBackground (MyBackgroundConfig * back, ASDeskBack * desk)
{
  desk->MyStyle = None;
  desk->data.pixmap = None;
  desk->data_type = None;
  if (back->data == NULL)
    return;
  if (back->data[0] == '\0')
    return;

  LOG3 ("\n Ok to process new back[%s] for desk %d", back->data, desk->desk)
  /* process data based on flags */
  /* if it is image - then load image and set data_type = pixmap and Pixmap */
  /* if it is image of wrong format - go to command */
  /* if it is Style - then set MyStyle to atom of MyStyle's name and data = 0 */
  /* if it is command - then build complete command line and create XAtom */
    if (back->flags & BGFLAG_FILE)
    {
      char *realfilename = PutHome (back->data);

      if (realfilename)
	{
	  if ((desk->data.pixmap = LoadImage (dpy, Scr.Root, 1024, realfilename)) != None)
	    desk->data_type = XA_PIXMAP;
	  LOG3 ("\n loaded file %s with pixmap id %d", realfilename, desk->data.pixmap)
	    free (realfilename);
	}
      if (desk->data.pixmap != None)
	{
	  /* do all the transformations if it is the pixmap */
	  desk->data.pixmap = DoTransformPixmap (desk->data.pixmap, back);
	  return;
	}
    }
  if (back->flags & BGFLAG_MYSTYLE)
    {				/* try to find style */
      if ((desk->MyStyle = XInternAtom (dpy, back->data, True)) == None)
	fprintf (stderr, "%s: cannot use undefined style [%s] for background of desk #%ld!", MyName, back->data, desk->desk);

    }
  else
    /* command should be executed to set background */
    {
      char *full_cmd, *prefix;
      int pref_len;

      prefix = mystrdup (XIMAGELOADER);
      if (!(back->flags & BGFLAG_FILE))
	{			/* we should get all command options supplied so cut prefix to
				   only a prog name */
	  /*assuming we don't have spaces here in path to prog itself - only option
	     separator spaces */
	  char *ptr = strchr (prefix, ' ');
	  if (ptr)
	    *ptr = '\0';
	}
      pref_len = strlen (prefix);
      full_cmd = (char *) safemalloc (pref_len + 1 + strlen (back->data) + 1);
      strcpy (full_cmd, prefix);
      full_cmd[pref_len] = ' ';
      strcpy (&(full_cmd[pref_len + 1]), back->data);
      desk->data.atom = XInternAtom (dpy, full_cmd, False);
      free (full_cmd);
      free (prefix);
    }
}

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/

void
GetOptions (const char *filename)
{
#ifdef DO_CLOCKING
  clock_t started = clock ();
#endif
  ASetRootConfig *config;
  int i;
  int using_tmp_heap = 1;

  struct BackCrossref
    {
      MyBackgroundConfig *back;
      ASDeskBack *desk;
    }
   *back_crossref;

  int backs_num = 0, backs_filled = 0;
  MyBackgroundConfig *curr_back;
  DeskBackConfig *curr_desk;

  config = ParseASetRootOptions (filename, MyName);
  if (!config)
    exit (0);			/* something terrible has happend */
#ifdef DO_CLOCKING
  fprintf (stderr, "\n Config parsing time (clocks): %lu\n", clock () - started);
#endif

  /* processing all mystyles first as we will need them in place in order
     to use backgrounds */
  /* reading global styles first */
  if (MyEnv.action == ACTION_DEFAULT)
    {
      mystyle_get_property (dpy, Scr.Root, Atoms[MYSTYLE_ATOM].atom, Atoms[MYSTYLE_ATOM].type);
      if (config->style_defs)
	ProcessMyStyleDefinitions (&(config->style_defs), pixmapPath);
    }

  /* now lets convert config into the list of desks and list
     ASDeskBack's */
  /* first let's see how many backgrounds we have defined max */
  for (curr_back = config->my_backs; curr_back;
       curr_back = curr_back->next)
    backs_num++;
  back_crossref = (struct BackCrossref *)
    safemalloc (sizeof (struct BackCrossref) * backs_num);
  for (i = 0; i < backs_num; i++)
    back_crossref[i].back = NULL;
  /* now let's count our desks */
  DesksArray.desks_num = 0;	/* just in case */
  for (curr_desk = config->my_desks; curr_desk;
       curr_desk = curr_desk->next)
    if (IsDeskInRange (curr_desk->desk) &&
	curr_desk->back != NULL)
      DesksArray.desks_num++;
  if (DesksArray.desks)
    free (DesksArray.desks);
  DesksArray.desks = (ASDeskBack *) safemalloc (sizeof (ASDeskBack) * DesksArray.desks_num);

  /* now we are ready to do the real thing */
  i = 0;
  for (curr_desk = config->my_desks; curr_desk;
       curr_desk = curr_desk->next)
    {
      int k;
      LOG2 ("\n processing desk %d", curr_desk->desk)
	if (!IsDeskInRange (curr_desk->desk) ||		/* out of range ! */
	    curr_desk->back == NULL)
	continue;
      /* Now  let's add record to DesksArray and do all the neccessary
         image preparation */
      DesksArray.desks[i].desk = curr_desk->desk;
      /* first check if we already have this background loaded */
      for (k = 0; k < backs_filled; k++)
	if (back_crossref[k].back == curr_desk->back)
	  break;
      if (k < backs_filled && back_crossref[k].desk)
	{			/* just copy stuff over */
	  DuplicateDeskBack (&(DesksArray.desks[i]), back_crossref[k].desk);
	}
      else
	{			/* process new background */
	  ProcessNewBackground (curr_desk->back, &(DesksArray.desks[i]));
	  back_crossref[k].back = curr_desk->back;
	  back_crossref[k].desk = &(DesksArray.desks[i]);
	  if (k == backs_filled)
	    backs_filled++;
	}
      if (MyEnv.action == ACTION_LOADONLY)
	{
	  if (IsPurePixmap (&(DesksArray.desks[i])))
	    fprintf (stderr, "%s: loaded root background for desk# %d (pixmap   id is %ld).\n", MyName, curr_desk->desk, DesksArray.desks[i].data.pixmap);
	  else if (DesksArray.desks[i].MyStyle != None)
	    fprintf (stderr, "%s: loaded root background for desk# %d (MyStyle  id is %ld).\n", MyName, curr_desk->desk, DesksArray.desks[i].MyStyle);
	  else
	    fprintf (stderr, "%s: loaded root background for desk# %d (resource id is %ld).\n", MyName, curr_desk->desk, DesksArray.desks[i].data.atom);

	}
      LOG3 ("\n desk #%d aded %d", i, DesksArray.desks[i]);
      i++;
    }
  free (back_crossref);
#ifdef DO_CLOCKING
  fprintf (stderr, "\n Background processing time (clocks): %lu\n", clock () - started);
#endif
  /* letting everybody know about what will be used as backgrounds */
  UpdateBackgroundProperty ();

  /* uff, here we are at the end at last - let's clean the mess now */
  DestroyASetRootConfig (config);
#if defined(DEBUG_ASETROOT) && defined(DEBUG_ALLOCS)
  fprintf (stderr, "\n Unfreed memory at the end of startup phase:\n");
  print_unfreed_mem ();
#endif
#ifdef DO_CLOCKING
  fprintf (stderr, "\n Total startup time (clocks): %lu\n", clock () - started);
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
#ifdef DO_CLOCKING
  clock_t started = clock ();
#endif
  BaseConfig *config = ParseBaseOptions (filename, MyName);

  if (!config)
    exit (0);			/* something terrible has happend */

  pixmapPath = config->pixmap_path;
  config->pixmap_path = NULL;	/* setting it to NULL so it would not be
				   deallocated by DestroyBaseConfig */
  DestroyBaseConfig (config);

#ifdef DO_CLOCKING
  fprintf (stderr, "\n Base Config parsing time (clocks): %lu\n", clock () - started);
#endif

}
