/*
 * Copyright (C) 1999 Sasha Vasko   <sasha at aftercode.net>
 * Copyright (C) 1999 Ethan Fischer <allanon@crystaltokyo.com>
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

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>

#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "../configure.h"
#include "../include/aftersteplib.h"
#include "../include/module.h"


char *
module_get_socket_property (Window w)
{
  char *data;
  int actual_format;
  Atom name, actual_type;
  unsigned long junk;

  name = XInternAtom (dpy, "_AS_MODULE_SOCKET", False);

  if (XGetWindowProperty (dpy, w, name, 0, ~0, False, AnyPropertyType, &actual_type, &actual_format, &junk, &junk, (unsigned char **) &data) != Success)
    return NULL;

  if (actual_type != XA_STRING || actual_format != 8)
    {
      XFree (data);
      return NULL;
    }

  return data;
}

int
module_connect (const char *socket_name)
{
  int fd;

  /* create an unnamed socket */
  if ((fd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
      fprintf (stderr, "%s: unable to create UNIX socket: ", MyName);
      perror ("");
    }

  /* connect to the named socket */
  if (fd >= 0)
    {
      struct sockaddr_un name;

      name.sun_family = AF_UNIX;
      strcpy (name.sun_path, socket_name);

      if (connect (fd, (struct sockaddr *) &name, sizeof (struct sockaddr_un)))
	{
	  fprintf (stderr, "%s: unable to connect to socket '%s': ", MyName, name.sun_path);
	  perror ("");
	  close (fd);
	  fd = -1;
	}
    }

  return fd;
}

int GetFdWidth (void);

ASMessage *
CheckASMessageFine (int fd, int t_sec, int t_usec)
{
  fd_set in_fdset;
  ASMessage *msg = NULL;
  struct timeval tv;

  FD_ZERO (&in_fdset);
  FD_SET (fd, &in_fdset);
  tv.tv_sec = t_sec;
  tv.tv_usec = t_usec;
#ifdef __hpux
  while (select (fd+1, (int *) &in_fdset, 0, 0, (t_sec < 0) ? NULL : &tv) == -1)
    if (errno != EINTR)
      break;
#else
  while (select (fd+1, &in_fdset, 0, 0, (t_sec < 0) ? NULL : &tv) == -1)
    if (errno != EINTR)
      break;
#endif
  if (FD_ISSET (fd, &in_fdset))
    {
      msg = (ASMessage *) safemalloc (sizeof (ASMessage));
      if (ReadASPacket (fd, msg->header, &(msg->body)) <= 0)
	{
	  free (msg);
	  msg = NULL;
	}
    }

  return msg;
}

void
DestroyASMessage (ASMessage * msg)
{
  if (msg)
    {
      if (msg->body)
	free (msg->body);
      free (msg);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	SendInfo - send a command back to afterstep
 *
 ***********************************************************************/
void
SendInfo (int *fd, char *message, unsigned long window)
{
  size_t w;

  if (message != NULL)
    {
      write (fd[0], &window, sizeof (unsigned long));
      w = strlen (message);
      write (fd[0], &w, sizeof (int));
      write (fd[0], message, w);

      /* keep going */
      w = 1;
      write (fd[0], &w, sizeof (int));
    }
}


/* some */
void
SetMyName (char *argv0)
{
  char *temp = strrchr (argv0, '/');
  /* Save our program name - for error messages */
  MyName = temp ? temp + 1 : argv0;
}

void DeadPipe (int nonsense);

int
ConnectAfterStep (unsigned long message_mask)
{
  char *temp;
  char mask_mesg[32];
  int as_fd[2];
  /* connect to AfterStep */
  /* Dead pipe == AS died */
  signal (SIGPIPE, DeadPipe);
  temp = module_get_socket_property (RootWindow (dpy, DefaultScreen(dpy)));
  as_fd[0] = as_fd[1] = module_connect (temp);
  XFree (temp);

  if (as_fd[0] < 0)
    {
      fprintf (stderr, "%s: unable to establish connection to AfterStep\n", MyName);
      exit (1);
    }

  temp = safemalloc (9 + strlen (MyName) + 1);
  sprintf (temp, "SET_NAME %s", MyName);
  SendInfo (as_fd, temp, None);
  free (temp);

  sprintf (mask_mesg, "SET_MASK %lu\n", (unsigned long) message_mask);
  SendInfo (as_fd, mask_mesg, None);
  return as_fd[0];
}

void
LoadBaseConfig (char *global_config_file, void (*read_base_options_func) (const char *))
{
  if (global_config_file != NULL)
    read_base_options_func (global_config_file);
  else
    {
      char *realconfigfile;
      char configfile[255];
      int depth = DefaultDepth (dpy, DefaultScreen (dpy));
      sprintf (configfile, "%s/base.%dbpp", AFTER_DIR, depth);
      realconfigfile = (char *) PutHome (configfile);
      if (CheckFile (realconfigfile) == -1)
	{
	  free (realconfigfile);
	  sprintf (configfile, "%s/base.%dbpp", AFTER_SHAREDIR, depth);
	  realconfigfile = PutHome (configfile);
	}
      read_base_options_func (realconfigfile);
      free (realconfigfile);
    }
}

void
LoadConfig (char *global_config_file, char *config_file_name, void (*read_options_func) (const char *))
{
  if (global_config_file != NULL)
    read_options_func (global_config_file);
  else
    {
      char *realconfigfile;
      char configfile[255];
      sprintf (configfile, "%s/%s", AFTER_DIR, config_file_name);
      realconfigfile = (char *) PutHome (configfile);
      if ((CheckFile (realconfigfile)) == -1)
	{
	  free (realconfigfile);
	  sprintf (configfile, "%s/%s", AFTER_SHAREDIR, config_file_name);
	  realconfigfile = PutHome (configfile);
	}
      read_options_func (realconfigfile);
      free (realconfigfile);
    }
}

/**********************************************************************/
/*              ModuleInfo functions                                  */
/**********************************************************************/
void
default_version_func (void)
{
  printf ("%s version %s\n", MyName, VERSION);
  exit (0);
}
void (*custom_version_func) (void) = default_version_func;

int
ProcessModuleArgs (int argc, char **argv, char **global_config_file, unsigned long *app_window, unsigned long *app_context, void (*custom_usage_func) (void))
{
  int i;
  for (i = 1; i < argc && *argv[i] == '-'; i++)
    {
      if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
	{
	  if (custom_usage_func)
	    custom_usage_func ();
	  else
	    exit (0);
	}
      else if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "--version"))
	{
	  if (custom_version_func)
	    custom_version_func ();
	  else
	    exit (0);
	}
      else if (!strcmp (argv[i], "-w") || !strcmp (argv[i], "--window"))
	{
	  i++;
	  if (app_window)
	    *app_window = strtol (argv[i], NULL, 16);
	}
      else if (!strcmp (argv[i], "-c") || !strcmp (argv[i], "--context"))
	{
	  i++;
	  if (app_context)
	    *app_context = strtol (argv[i], NULL, 16);
	}
      else if (!strcmp (argv[i], "-f") && i + 1 < argc)
	{
	  i++;
	  if (global_config_file)
	    *global_config_file = argv[i];
	}
      else
	return i;

    }
  return i;
}
