/*
 * Copyright (C) 1998 Sasha Vasko <sasha at aftercode.net>
 * Copyright (c) 1996 by Alfredo Kojima
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

/*
 * Todo:
 * realtime audio mixing
 * replace the Audio module
 */

#include "../../configure.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/StringDefs.h>

#include "../../include/aftersteplib.h"
#include "../../include/module.h"
#define AUDIO_DEVICE		"/dev/audio"

ScreenInfo Scr;

typedef struct LList
  {
    struct LList *next;
    struct LList *prev;
    int data;
    int timestamp;
  }
LList;

typedef struct SoundEntry
  {
    char *data;
    int size;
  }
SoundEntry;

/* timeout time for sound playing in seconds */
#define PLAY_TIMEOUT 1

/* table of audio data (not filenames) */
SoundEntry **SoundTable;

int LastSound = 0;
char *SoundPlayer;

Display *dpy;			/* which display are we talking to */
int screen;

char *MyName;
LList *PlayQueue = NULL, *OldestQueued = NULL;

int fd[2];

/*
 * Register --
 *      makes a new sound entry
 */
void
Register (char *sound_file, int code)
{
  int file;
  struct stat stat_buf;

  if (sound_file[0] == '.' && sound_file[1] == 0)
    {
      SoundTable[code] = (SoundEntry *) safemalloc (sizeof (SoundEntry));
      SoundTable[code]->size = -1;
      return;			/* dummy sound */
    }
  if (stat (sound_file, &stat_buf) < 0)
    {
      fprintf (stderr, "%s: sound file %s not found\n", MyName,
	       sound_file);
      return;
    }
  SoundTable[code] = (SoundEntry *) safemalloc (sizeof (SoundEntry));
  if (SoundPlayer != NULL)
    {
      SoundTable[code]->data = safemalloc (strlen (sound_file));
      SoundTable[code]->size = 1;
      strcpy (SoundTable[code]->data, sound_file);
      return;
    }
  SoundTable[code]->data = safemalloc (stat_buf.st_size);
  file = open (sound_file, O_RDONLY);
  if (file < 0)
    {
      fprintf (stderr, "%s: can't open sound file %s\n", MyName, sound_file);
      free (SoundTable[code]->data);
      free (SoundTable[code]);
      SoundTable[code] = NULL;
      return;
    }
  SoundTable[code]->size = read (file, SoundTable[code]->data, stat_buf.st_size);
  if (SoundTable[code]->size < 1)
    {
      fprintf (stderr, "%s: error reading sound file %s\n", MyName,
	       sound_file);
      free (SoundTable[code]->data);
      free (SoundTable[code]);
      close (file);
      SoundTable[code] = NULL;
      return;
    }
  close (file);
}

/*
 * PlaySound --
 *      plays a sound
 */

void
PlaySound (int sid)
{
  if ((sid < LastSound) && (SoundTable[sid]->size <= 0))
    return;
  if ((sid >= LastSound) || (sid < 0) || SoundTable[sid] == NULL)
    {
      fprintf (stderr, "%s: request to play invalid sound received\n",
	       MyName);
      return;
    }
  if (SoundPlayer != NULL)
    {
      pid_t child;

      child = fork ();
      if (child < 0)
	return;
      else if (child == 0)
	{
	  execlp (SoundPlayer, SoundPlayer, SoundTable[sid]->data, NULL);
	}
      else
	{
	  while (wait (NULL) < 0);
	}
      /*
         sprintf(cmd,"%s %s",SoundPlayer,SoundTable[sid]->data);
         system(cmd);
       */
      return;
    }
#if 0
  audio = open (AUDIO_DEVICE, O_WRONLY | O_NONBLOCK);
  if ((audio < 0) && errno == EAGAIN)
    {
      sleep (1);
      audio = open (AUDIO_DEVICE, O_WRONLY | O_NONBLOCK);
      if (audio < 0)
	return;
    }
  write (audio, SoundTable[sid]->data, SoundTable[sid]->size);
  close (audio);
  audio = -1;
#endif
}

void
DoNothing (int foo)
{
  signal (SIGUSR1, DoNothing);
}

/*
 * HandleRequest --
 *       play requested sound
 * sound -1 is a quit command
 *
 * Note:
 *      Something not good will happed if a new play request
 * arrives before exiting the handler
 */
void
HandleRequest (int foo)
{
  int sound, timestamp;
/*
   signal(SIGUSR1, DoNothing);
 */
  read (fd[0], &sound, sizeof (sound));
  read (fd[0], &timestamp, sizeof (timestamp));
  if (sound < 0)
    {
      printf ("exitting ASSound..\n");
      exit (0);
    }
  if ((clock () - timestamp) < PLAY_TIMEOUT)
    PlaySound (sound);
#if 0
  tmp = safemalloc (sizeof (LList));
  tmp->data = sound;
  tmp->timestamp = clock ();
  tmp->next = PlayQueue;
  if (PlayQueue == NULL)
    {
      OldestQueued = tmp;
      tmp->prev = NULL;
    }
  else
    {
      PlayQueue->prev = tmp;
    }
  PlayQueue = tmp;
  signal (SIGUSR1, HandleRequest);
#endif
}

void
DeadPipe (int nonsense)
{
  exit (0);
}

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
	"%s [--version] [--help] player [entry1 [entry2 [...]]]\n", MyName);
  exit (0);
}

/*
 * Program startup.
 * Arguments:
 * argv[1] - pipe for reading data
 * argv[2] - the name of the sound player to be used. ``-'' indicates
 *      internal player.
 * argv[3]... - filenames of sound files with code n-3
 */
int
main (int argc, char **argv)
{
  char *temp;
  int i, k;
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
	i++;
      else if (!strcmp (argv[i], "-c") || !strcmp (argv[i], "--context"))
	i++;
      else if (!strcmp (argv[i], "-f") && i + 1 < argc)
	global_config_file = argv[++i];
      else if (!strcmp (argv[i], "-"))
	/* this is the default case */ ;
    }

  /* Dead pipe == dead AfterStep */
  signal (SIGPIPE, DeadPipe);

  if ((dpy = XOpenDisplay ("")) == NULL)
    {
      fprintf (stderr, "%s: couldn't open display %s\n",
	       MyName, XDisplayName (""));
      exit (1);
    }
  screen = DefaultScreen (dpy);

  /* connect to AfterStep */
  temp = module_get_socket_property (RootWindow (dpy, screen));
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

  signal (SIGUSR1, HandleRequest);

  if (i < argc)
    SoundPlayer = argv[i++];

  SoundTable = (SoundEntry **) safemalloc (sizeof (SoundEntry *) * (argc - i));
  for (k = 0; i < argc; i++, k++)
    {
      Register (argv[i], k);
    }
  LastSound = k;
  while (1)
    {
#if 0
      LList *tmp;
      while (OldestQueued != NULL)
	{
	  if ((clock () - OldestQueued->timestamp) < PLAY_TIMEOUT)
	    PlaySound (OldestQueued->data);
	  tmp = OldestQueued->prev;
	  free (OldestQueued);
	  OldestQueued = tmp;
	}
      pause ();
#endif
      HandleRequest (0);
    }
  return 0;
}
