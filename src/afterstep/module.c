/*
 * Copyright (c) 1999 Ethan Fischer <allanon@crystaltokyo.com>
 * Copyright (C) 1998 Guylhem Aznar
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

/***********************************************************************
 *
 * code for launching afterstep modules.
 *
 ***********************************************************************/

#include "../../configure.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>		/* for chmod() */
#include <sys/types.h>
#include <sys/un.h>		/* for struct sockaddr_un */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"

#include "globals.h"
#include "menus.h"

int module_fd;
int npipes;

extern char *global_base_file;

module_t *Module = NULL;

AFTER_INLINE int PositiveWrite (int module, unsigned long *ptr, int size);
void DeleteQueueBuff (int module);
void AddToQueue (int module, unsigned long *ptr, int size, int done);

extern ASDirs as_dirs;

int
module_setup_socket (Window w, const char *display_string)
{
  char *tmp;

#ifdef __CYGWIN__
	{										   /* there are problems under Windows as home dir could be anywhere  -
											    * so we just use /tmp since there will only be 1 user anyways :) */
		tmp = safemalloc (4 + 9 + strlen (display_string) + 1);
		/*sprintf (tmp, "/tmp/connect.%s", display_string); */
		sprintf (tmp, "/tmp/as-connect.%ld", Scr.screen);
		fprintf (stderr, "using %s for intermodule communications.\n", tmp);
	}
#else
	{
        char *tmpdir = getenv("TMPDIR");
        static char *default_tmp_dir = "/tmp" ;
        if( tmpdir != NULL )
            if( CheckDir( tmpdir ) < 0 )
                tmpdir = NULL ;

        if( tmpdir == NULL )
            tmpdir = default_tmp_dir ;
        tmp = safemalloc (strlen(tmpdir)+11+32 + strlen (display_string) + 1);
        sprintf (tmp, "%s/afterstep-%d.%s", tmpdir, getuid(), display_string);
	}
#endif

  XChangeProperty (dpy, w, _AS_MODULE_SOCKET, XA_STRING, 8,
		   PropModeReplace, (unsigned char *) tmp, strlen (tmp));
  module_fd = module_listen (tmp);
  free (tmp);

  XSync (dpy, 0);

  return (module_fd >= 0);
}

/* create a named UNIX socket, and start watching for connections */
int
module_listen (const char *socket_name)
{
  int fd;

  /* create an unnamed socket */
  if ((fd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
      fprintf (stderr, "%s: unable to create UNIX socket: ", MyName);
      perror ("");
    }

  /* remove the socket file */
  if (fd >= 0)
    {
      if (unlink (socket_name) == -1 && errno != ENOENT)
	{
	  fprintf (stderr, "%s: unable to delete file '%s': ", MyName, socket_name);
	  perror ("");
	  close (fd);
	  fd = -1;
	}
    }

  /* bind the socket to a name */
  if (fd >= 0)
    {
      struct sockaddr_un name;

      name.sun_family = AF_UNIX;
      strcpy (name.sun_path, socket_name);

      if (bind (fd, (struct sockaddr *) &name, sizeof (struct sockaddr_un)) == -1)
	{
	  fprintf (stderr, "%s: unable to bind socket to name '%s': ", MyName, socket_name);
	  perror ("");
	  close (fd);
	  fd = -1;
	}
    }

  /* set file permissions to 0700 (read/write/execute by owner only) */
  if (fd >= 0)
    {
      if (chmod (socket_name, 0700) == -1)
	{
	  fprintf (stderr, "%s: unable to set socket permissions to 0700: ", MyName);
	  perror ("");
	  close (fd);
	  fd = -1;
	}
    }

  /* start listening for incoming connections */
  if (fd >= 0)
    {
      if (listen (fd, 254) == -1)
	{
	  fprintf (stderr, "%s: unable to listen on socket: ", MyName);
	  perror ("");
	  close (fd);
	  fd = -1;
	}
    }

  /* set non-blocking I/O mode */
  if (fd >= 0)
    {
      if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK) == -1)
	{
	  fprintf (stderr, "%s: unable to set non-blocking I/O: ", MyName);
	  perror ("");
	  close (fd);
	  fd = -1;
	}
    }

  /* mark as close-on-exec so other programs won't inherit the socket */
  if (fd >= 0)
    {
      if (fcntl (fd, F_SETFD, 1) == -1)
	{
	  fprintf (stderr, "%s: unable to set close-on-exec for socket: ", MyName);
	  perror ("");
	  close (fd);
	  fd = -1;
	}
    }

  return fd;
}

int
module_accept (int socket_fd)
{
  int fd;
  int len = sizeof (struct sockaddr_un);
  struct sockaddr_un name;

  fd = accept (socket_fd, (struct sockaddr *) &name, &len);
  if (fd < 0 && errno != EWOULDBLOCK)
    {
      fprintf (stderr, "%s: error accepting connection: ", MyName);
      perror ("");
    }
  /* set non-blocking I/O mode */
  if (fd >= 0)
    {
      if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK) == -1)
	{
	  fprintf (stderr, "%s: unable to set non-blocking I/O for module socket: ", MyName);
	  perror ("");
	  close (fd);
	  fd = -1;
	}
    }
  /* mark as close-on-exec so other programs won't inherit the socket */
  if (fd >= 0)
    {
      if (fcntl (fd, F_SETFD, 1) == -1)
	{
	  fprintf (stderr, "%s: unable to set close-on-exec for module socket: ", MyName);
	  perror ("");
	  close (fd);
	  fd = -1;
	}
    }
  if (fd >= 0)
    {
      int i;
      /* Look for an available pipe slot */
      i = 0;
      while ((i < npipes) && (Module[i].fd >= 0))
	i++;
      if (i >= npipes)
	{
	  fprintf (stderr, "%s: too many modules!\n", MyName);
	  close (fd);
	  return -1;
	}
      /* add pipe to afterstep's active pipe list */
      Module[i].name = mystrdup ("unknown module");
      Module[i].fd = fd;
      Module[i].active = 0;
      Module[i].mask = MAX_MASK;
      Module[i].output_queue = NULL;
    }
  return fd;
}

void
module_init (int free_resources)
{
  if (free_resources)
    {
      ClosePipes ();
      if (Module != NULL)
	free (Module);
    }

  Module = NULL;
}

void
module_setup (void)
{
  int i;

  npipes = GetFdWidth ();

  Module = safemalloc (sizeof (module_t) * npipes);

  for (i = 0; i < npipes; i++)
    {
      Module[i].fd = -1;
      Module[i].active = -1;
      Module[i].mask = MAX_MASK;
      Module[i].output_queue = NULL;
      Module[i].name = NULL;
      Module[i].ibuf.len = 0;
      Module[i].ibuf.done = 0;
      Module[i].ibuf.text = NULL;
    }
}

void
ClosePipes (void)
{
  int i;
  for (i = 0; i < npipes; i++)
    KillModule (i, 0);
}

void
executeModule (char *action, FILE * fd, char **win, int *context)
{
  int val;
  char command[256];
  char *cptr;
  char *aptr = NULL;
  char *end;
  char *arg0;
  extern char *ModulePath;

  if (action == NULL)
    return;

  sprintf (command, "%.255s", action);
  command[255] = '\0';

  for (cptr = command; isspace (*cptr); cptr++);

  for (end = cptr; !isspace (*end) && *end != '\0'; end++);

  if (*end != '\0')
    {
      for (*end++ = '\0'; isspace (*end); end++);
      if (*end != '\0')
	aptr = end;
    }

  arg0 = findIconFile (cptr, ModulePath, X_OK);
  if (arg0 == NULL)
    {
      fprintf (stderr, "%s: no such module %s in path %s\n", MyName, cptr, ModulePath);
      return;
    }

  val = fork ();
  if (val == 0)
    {
      /* this is the child */
      extern Bool shall_override_config_file;
      int i = 0;
      char *args[10];
      char arg1[40];
      char arg2[40];

      args[i++] = arg0;

      args[i++] = "--window";
      sprintf (arg1, "%lx", (unsigned long) win);
      args[i++] = arg1;

      args[i++] = "--context";
      sprintf (arg2, "%lx", (unsigned long) context);
      args[i++] = arg2;

      if (shall_override_config_file)
	{
	  args[i++] = "-f";
	  args[i++] = global_base_file;
	}

      if (aptr != NULL)
	args[i++] = aptr;

      args[i++] = NULL;

      execvp (arg0, args);
      fprintf (stderr, "%s: execution of module '%s' failed: ", MyName, arg0);
      perror ("");
      exit (1);
    }

  free (arg0);
}

int
HandleModuleInput (int channel)
{
  Window w;
  char *text;
  int len, done;
  void *ptr;

  /* read a window id */
  done = Module[channel].ibuf.done;
  len = sizeof (Window);
  ptr = &Module[channel].ibuf.window;
  if (done < len)
    {
      int n = read (Module[channel].fd, ptr + done, len - done);
      if (n == 0 || (n == -1 && errno != EINTR && errno != EAGAIN))
	{
	  KillModule (channel, 101 + n);
	  return -1;
	}
      if (n == -1)
	return 0;
      Module[channel].ibuf.done += n;
      if (done + n < len)
	return 0;
    }

  /* the pipe is active */
  Module[channel].active = 1;

  /* read an afterstep bultin command line */
  done = Module[channel].ibuf.done - sizeof (Window);
  len = sizeof (int);
  ptr = &Module[channel].ibuf.size;
  if (done < len)
    {
      int n = read (Module[channel].fd, ptr + done, len - done);
      if (n == 0 || (n == -1 && errno != EINTR && errno != EAGAIN))
	{
	  KillModule (channel, 103 + n);
	  return -1;
	}
      if (n == -1)
	return 0;
      Module[channel].ibuf.done += n;
      if (done + n < len)
	return 0;
    }

  /* max command length is 255 */
  if (Module[channel].ibuf.size > 255)
    {
      fprintf (stderr, "%s: command from module '%s' is too big (%d)\n", MyName, Module[channel].name, Module[channel].ibuf.size);
      Module[channel].ibuf.size = 255;
    }

  /* need to be able to read in command */
  if (Module[channel].ibuf.len < Module[channel].ibuf.size + 1)
    {
      Module[channel].ibuf.len = Module[channel].ibuf.size + 1;
      Module[channel].ibuf.text = realloc (Module[channel].ibuf.text, Module[channel].ibuf.len);
    }

  /* read an afterstep builtin command line */
  done = Module[channel].ibuf.done - sizeof (Window) - sizeof (int);
  len = Module[channel].ibuf.size;
  ptr = Module[channel].ibuf.text;
  if (done < len)
    {
      int n = read (Module[channel].fd, ptr + done, len - done);
      if (n == 0 || (n == -1 && errno != EINTR && errno != EAGAIN))
	{
	  KillModule (channel, 105 + n);
	  return -1;
	}
      if (n == -1)
	return 0;
      Module[channel].ibuf.done += n;
      if (done + n < len)
	return 0;
    }

  /* get continue command */
  done = Module[channel].ibuf.done - sizeof (Window) - sizeof (int) - Module[channel].ibuf.size;
  len = sizeof (int);
  ptr = &Module[channel].ibuf.cont;
  if (done < len)
    {
      int n = read (Module[channel].fd, ptr + done, len - done);
      if (n == 0 || (n == -1 && errno != EINTR && errno != EAGAIN))
	{
	  KillModule (channel, 107 + n);
	  return -1;
	}
      if (n == -1)
	return 0;
      Module[channel].ibuf.done += n;
      if (done + n < len)
	return 0;
    }

  /* check continue to make sure we're still talking properly */
  if (Module[channel].ibuf.cont != 1)
    KillModule (channel, 104);

  /* done reading command */
  Module[channel].ibuf.done = 0;

  /* set window id */
  w = Module[channel].ibuf.window;

  /* null-terminate the command line */
  text = Module[channel].ibuf.text;
  text[Module[channel].ibuf.size] = '\0';

  if (strlen (text))
    return RunCommand (text, channel, w);

  return 0;
}

void
DeadPipe (int nonsense)
{
  signal (SIGPIPE, DeadPipe);
}


void
KillModule (int channel, int place)
{
  if (Module[channel].fd > 0)
    close (Module[channel].fd);

  Module[channel].fd = -1;
  Module[channel].active = -1;

  while (Module[channel].output_queue != NULL)
    DeleteQueueBuff (channel);
  if (Module[channel].name != NULL)
    {
      free (Module[channel].name);
      Module[channel].name = NULL;
    }
  Module[channel].ibuf.len = 0;
  Module[channel].ibuf.done = 0;
  if (Module[channel].ibuf.text != NULL)
    {
      free (Module[channel].ibuf.text);
      Module[channel].ibuf.text = NULL;
    }
}

void
KillModuleByName (char *name)
{
  int i;

  if (name == NULL)
    return;

  for (i = 0; i < npipes; i++)
    {
      if (Module[i].fd > 0)
	{
	  if ((Module[i].name != NULL) && (strcmp (name, Module[i].name) == 0))
	    KillModule (i, 20);
	}
    }
  return;
}

void
Broadcast (unsigned long event_type, unsigned long num_datum,...)
{
  int i;
  va_list ap;
  unsigned long *body;

  body = safemalloc ((3 + num_datum) * sizeof (unsigned long));

  body[0] = START_FLAG;
  body[1] = event_type;
  body[2] = 3 + num_datum;

  va_start (ap, num_datum);
  for (i = 0; i < num_datum; i++)
    body[3 + i] = va_arg (ap, unsigned long);
  va_end (ap);

  for (i = 0; i < npipes; i++)
    PositiveWrite (i, body, body[2] * sizeof (unsigned long));

  free (body);
}

void
SendPacket (int module, unsigned long event_type, unsigned long num_datum,...)
{
  int i;
  va_list ap;
  unsigned long *body;

  body = safemalloc ((3 + num_datum) * sizeof (unsigned long));

  body[0] = START_FLAG;
  body[1] = event_type;
  body[2] = 3 + num_datum;

  va_start (ap, num_datum);
  for (i = 0; i < num_datum; i++)
    body[3 + i] = va_arg (ap, unsigned long);
  va_end (ap);

  PositiveWrite (module, body, body[2] * sizeof (unsigned long));

  free (body);
}

void
SendConfig (int module, unsigned long event_type, ASWindow * t)
{
  MyStyle *style;

  if (t == Scr.Hilite)
    style = t->style_focus;
  else if (t->flags & STICKY)
    style = t->style_sticky;
  else
    style = t->style_unfocus;

  SendPacket (module, event_type, 24, t->w, t->frame, (unsigned long) t,
	      t->frame_x, t->frame_y, t->frame_width, t->frame_height,
	      t->Desk, t->flags, t->title_height, t->boundary_width,
	      t->hints.base_width, t->hints.base_height, t->hints.width_inc,
	      t->hints.height_inc, t->hints.min_width, t->hints.min_height,
	      t->hints.max_width, t->hints.max_height, t->icon_title_w,
	      t->icon_pixmap_w, t->hints.win_gravity, style->colors.fore,
	      style->colors.back);
}


void
BroadcastConfig (unsigned long event_type, ASWindow * t)
{
  MyStyle *style;

  if (t == Scr.Hilite)
    style = t->style_focus;
  else if (t->flags & STICKY)
    style = t->style_sticky;
  else
    style = t->style_unfocus;

  Broadcast (event_type, 24, t->w, t->frame, (unsigned long) t,
	     t->frame_x, t->frame_y, t->frame_width, t->frame_height,
	     t->Desk, t->flags, t->title_height, t->boundary_width,
	     t->hints.base_width, t->hints.base_height, t->hints.width_inc,
	     t->hints.height_inc, t->hints.min_width, t->hints.min_height,
	     t->hints.max_width, t->hints.max_height, t->icon_title_w,
	     t->icon_pixmap_w, t->hints.win_gravity, style->colors.fore,
	     style->colors.back);
}

void
BroadcastName (unsigned long event_type, unsigned long data1,
	       unsigned long data2, unsigned long data3, char *name)
{
  int l, i;
  unsigned long *body;


  if (name == NULL)
    return;
  l = (strlen (name) + 1) / (sizeof (unsigned long)) + 7;

  body = (unsigned long *) safemalloc (l * sizeof (unsigned long));

  body[0] = START_FLAG;
  body[1] = event_type;
  body[2] = l;

  body[3] = data1;
  body[4] = data2;
  body[5] = data3;
  strcpy ((char *) &body[6], name);


  for (i = 0; i < npipes; i++)
    PositiveWrite (i, (unsigned long *) body, l * sizeof (unsigned long));

  free (body);

}


void
SendName (int module, unsigned long event_type,
	  unsigned long data1, unsigned long data2,
	  unsigned long data3, char *name)
{
  int l;
  unsigned long *body;

  if (name == NULL)
    return;
  l = strlen (name) / (sizeof (unsigned long)) + 7;
  body = (unsigned long *) safemalloc (l * sizeof (unsigned long));

  body[0] = START_FLAG;
  body[1] = event_type;
  body[2] = l;

  body[3] = data1;
  body[4] = data2;
  body[5] = data3;
  strcpy ((char *) &body[6], name);

  PositiveWrite (module, (unsigned long *) body, l * sizeof (unsigned long));

  free (body);
}



#include <sys/errno.h>
AFTER_INLINE int
PositiveWrite (int module, unsigned long *ptr, int size)
{
  if ((Module[module].active < 0) || (!((Module[module].mask) & ptr[1])))
    return -1;

  AddToQueue (module, ptr, size, 0);

  if (Module[module].mask & M_LOCKONSEND)
    {
      int e;

      FlushQueue (module);

      do
	{
	  e = HandleModuleInput (module);
	}
      while (e != 66 && e != -1);
    }

  return size;
}

void
AddToQueue (int module, unsigned long *ptr, int size, int done)
{
  struct queue_buff_struct *c, *e;
  unsigned long *d;

  c = (struct queue_buff_struct *) safemalloc (sizeof (struct queue_buff_struct));
  c->next = NULL;
  c->size = size;
  c->done = done;
  d = (unsigned long *) safemalloc (size);
  c->data = d;
  memcpy (d, ptr, size);

  e = Module[module].output_queue;
  if (e == NULL)
    {
      Module[module].output_queue = c;
      return;
    }
  while (e->next != NULL)
    e = e->next;
  e->next = c;
}

void
DeleteQueueBuff (int module)
{
  struct queue_buff_struct *a;

  if (Module[module].output_queue == NULL)
    return;
  a = Module[module].output_queue;
  Module[module].output_queue = a->next;
  free (a->data);
  free (a);
  return;
}

void
FlushQueue (int module)
{
  char *dptr;
  struct queue_buff_struct *d;
  int a;
  extern int errno;

  if ((Module[module].active <= 0) || (Module[module].output_queue == NULL))
    return;

  while (Module[module].output_queue != NULL)
    {
      int fd = Module[module].fd;
      d = Module[module].output_queue;
      dptr = (char *) d->data;
      for (a = 0; d->done < d->size; d->done += a)
	{
	  a = write (fd, &dptr[d->done], d->size - d->done);
	  if (a < 0)
	    break;
	}
      /* the write returns EWOULDBLOCK or EAGAIN if the pipe is full.
       * (This is non-blocking I/O). SunOS returns EWOULDBLOCK, OSF/1
       * returns EAGAIN under these conditions. Hopefully other OSes
       * return one of these values too. Solaris 2 doesn't seem to have
       * a man page for write(2) (!) */
      if (a < 0 && (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR))
	return;
      else if (a < 0)
	{
	  KillModule (module, 123);
	  return;
	}
      DeleteQueueBuff (module);
    }
}
