/* 
 * Copyright (c) 2000 Sasha Vasko <sashav at sprintmail.com>
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (C) 1994 Mark Boyns <boyns@sdsu.edu>
 * Copyright (C) 1994 Mark Scott <mscott@mcd.mot.com>
 * Copyright (C) 1994 Szijarto Szabolcs <saby@sch.bme.hu>
 * Copyright (C) 1994 Robert Nation
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

/*
 * afterstep includes:
 */
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MODULE_X_INTERFACE

#include "../../configure.h"
#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/style.h"
#include "../../include/parse.h"
#include "../../include/screen.h"
#include "../../include/module.h"

#define BUILTIN_STARTUP		MAX_MESSAGES
#define BUILTIN_SHUTDOWN	MAX_MESSAGES+1
#define BUILTIN_UNKNOWN		MAX_MESSAGES+2
#define MAX_BUILTIN			3
  
#define MAX_SOUNDS			(MAX_MESSAGES+MAX_BUILTIN)		

/*
 * rplay stuff:
 */
#ifdef HAVE_RPLAY_H
#include <rplay.h>
int           rplay_fd = -1;

/* define the rplay table */
RPLAY        *rplay_table[MAX_MESSAGES + MAX_BUILTIN];
/*RPLAY        *rplay_table[MAX_SOUNDS];*/
char         *host = NULL;
int           volume = RPLAY_DEFAULT_VOLUME;
int           priority = RPLAY_DEFAULT_PRIORITY;
#endif

/* globals */
char         *MyName = NULL;
ScreenInfo    Scr;
Display      *dpy;							   /* which display are we talking to */
int           screen;
static int    fd_width;
static int    fd[2];
static int    x_fd;
char         *audio_play_cmd_line, *audio_play_dir;
time_t        audio_delay = 0;				   /* seconds */

/* prototypes */
Bool          SetupSound ();

void          Loop (int *);
void          process_message (unsigned long, unsigned long *);
void          DeadPipe (int);
void          ParseOptions (const char *);
void          done (int);

/* this is the pointer to one of the play functions, chosen in SetupSound() : */
Bool (*audio_play) (short) = NULL;
/* these are possible players defined below : */
#ifdef HAVE_RPLAY_H
Bool audio_player_rplay(short) ;
#endif
Bool audio_player_cat(short) ;
Bool audio_player_stdout(short) ;
Bool audio_player_ext(short) ;


/* define the message table */
char         *messages[] = {
	"toggle_paging",
	"new_page",
	"new_desk",
	"add_window",
	"raise_window",
	"lower_window",
	"configure_window",
	"focus_change",
	"destroy_window",
	"iconify",
	"deiconify",
	"window_name",
	"icon_name",
	"res_class",
	"res_name",
	"end_windowlist",
	"icon_location",
	"map",
	"shade",
	"unshade",
	"lockonsend",
	"new_background",
    "new_theme",
/* add builtins here */
	"startup",
	"shutdown",
	"unknown",
};

/* here we'll store what we read from config file : */
char         *sound_config[MAX_SOUNDS];

/* then we add path to it and store it in this table : */
char         *sound_table[MAX_SOUNDS];

char  		 *ext_cmd = NULL;

void
usage (void)
{
  printf ("Usage:\n" "%s [-f [config file]] [-v|--version] [-h|--help]\n", MyName);
	exit (0);
}

/* error_handler:
 * catch X errors, display the message and continue running.
 */
int
error_handler (Display * disp, XErrorEvent * event)
{
	fprintf (stderr,
			 "%s: internal error, error code %d, request code %d, minor code %d.\n",
			 MyName, event->error_code, event->request_code, event->minor_code);
	return 0;
}

int
main (int argc, char **argv)
{
	int           i;
	char         *global_config_file = NULL;

	/* Save our program name - for error messages */
	SetMyName (argv[0]);

	i = ProcessModuleArgs (argc, argv, &(global_config_file), NULL, NULL, usage);

	/* Dead pipe == AS died */
	signal (SIGPIPE, DeadPipe);
	signal (SIGQUIT, DeadPipe);
	signal (SIGSEGV, DeadPipe);
	signal (SIGTERM, DeadPipe);

	x_fd = ConnectX (&Scr, display_name, PropertyChangeMask);
	XSetErrorHandler (error_handler);

	/* connect to AfterStep */
	fd_width = GetFdWidth ();
	fd[0] = fd[1] = ConnectAfterStep (MAX_MASK);

	LoadConfig (global_config_file, "audio", ParseOptions);

	if (!SetupSound ())
	{
		fprintf (stderr,
				 "%s: Failed to initialize sound playing routines. Please check your config.\n", MyName);
		return 1;
	}

	/*
	 * Play the startup sound.
	 */
	audio_play (BUILTIN_STARTUP);
	SendInfo (fd, "Nop", 0);
	Loop (fd);

	return 0;
}

Bool SetupSound ()
{
	register int i ;
	int sounds_configured = 0 ;
	char *tmp ;
	int  max_sound_len = 0 ;
	/* first lets build our sounds table and see if we actually have anything to play :*/
	for( i = 0 ; i < MAX_SOUNDS ; i++ )
		if( sound_config[i] ) 
		{
		  int len ;
			sounds_configured++ ;
			if( *(sound_config[i]) != '/' || strchr(sound_config[i], '$' ) != NULL )
			{
				tmp = safemalloc( strlen(audio_play_dir)+1+strlen(sound_config[i])+1 );
				sprintf( tmp, "%s/%s", audio_play_dir, sound_config[i] );	   
				sound_table[i] = PutHome( tmp );
				free( tmp );
			}else
				sound_table[i] = mystrdup( sound_config[i] );
			len = strlen( sound_table[i] );
			if( len > max_sound_len )
				max_sound_len = len ;
		}
	
	if( sounds_configured <= 0 ) 
	{
		fprintf( stderr, "%s: no sounds configured.", MyName );
		return False ;
	}
	  
	/* now lets choose what player to use, and if we can use it at all : */
#ifdef HAVE_RPLAY_H
	/*
	 * Builtin rplay support is enabled when AudioPlayCmd == builtin-rplay.
	 */
	if (!mystrcasecmp (audio_play_cmd_line, "builtin-rplay"))
	{
		if( host ) rplay_fd = rplay_open (host);
		else if( (rplay_fd = rplay_open_display ()) < 0 ) 
			rplay_fd = rplay_open ("localhost");

		if (rplay_fd < 0)
		{
			rplay_perror (MyName);
			return False ;
		}
		audio_play = audio_player_rplay ;
		for( i = 0 ; i < MAX_SOUNDS ; i++ )
			if( sound_table[i] )
			{
				rplay_table[i] = rplay_create (RPLAY_PLAY);
				rplay_set (rplay_table[i], RPLAY_APPEND,
						   RPLAY_SOUND, (host)?sound_config[i]:sound_table[i],
						   RPLAY_PRIORITY, priority, RPLAY_VOLUME, volume, NULL);
			}
		return True;
	}
#endif
	if( mystrcasecmp (audio_play_cmd_line, "builtin-cat")== 0 )
	{
		audio_play = audio_player_cat ;
	}else if( mystrcasecmp (audio_play_cmd_line, "builtin-stdout")== 0)
	{
		audio_play = audio_player_stdout ;
	}else
	{
		audio_play = audio_player_ext ;
		if( audio_play_cmd_line == NULL )
			audio_play_cmd_line = mystrdup("");
		ext_cmd = safemalloc( strlen(SOUNDPLAYER) + 1 + strlen(audio_play_cmd_line) + 1 + max_sound_len + 1 );
	}
	
	return (audio_play!=NULL) ;
}

/***********************************************************************
 *
 *  Procedure:
 *	ParseOptions - read the sound configuration file.
 *
 ***********************************************************************/

void
ParseOptions (const char *config_file)
{
	FILE          *fp;
	char          *buf;
	char          *tline;
	register char *p, *token ;
	int            i, len;

	if ((fp = fopen (config_file, "r")) == NULL)
	{
		done (1);
	}
	/*
	 * Intialize all the sounds.
	 */
	for (i = 0; i < MAX_SOUNDS; i++)
		sound_config[i] = NULL;

	buf = (char *)safemalloc (MAXLINELENGTH);
	len = strlen (MyName);
	while ((tline = fgets (buf, MAXLINELENGTH, fp)) != NULL)
	{
		/*
		 * Search for *ASAudio.
		 */
		p = stripcomments( tline );
 		if ((*p == '*') && (!mystrncasecmp (p+1, MyName, len)))
		{
			p += len + 1;
			for( token = p ; *token && !isspace(*token) ; token++ );
			while( isspace(*token) ) token++ ; /* that will get us to the first token */
			if( *token == '\0' ) continue ; 

			if (!mystrncasecmp (p, "PlayCmd", 7))
			{
				if (audio_play_cmd_line)
					free (audio_play_cmd_line);
				audio_play_cmd_line = PutHome (token);
			} else if (!mystrncasecmp (p, "Path", 4))
			{
				if (audio_play_dir)
					free (audio_play_dir);
				audio_play_dir = PutHome (token);
			} else if (!mystrncasecmp (p, "Delay", 5))
			{
				audio_delay = atoi (token);
			}
#ifdef HAVE_RPLAY_H
			/*
			 * Check for rplay configuration options.
			 */
			else if (!mystrncasecmp (p, "RplayHost", 9))
			{
				if (host)
					free (host);
				host = PutHome (token);
			} else if (!mystrncasecmp (p, "RplayVolume", 11))
			{
				volume = atoi (token);
			} else if (!mystrncasecmp (p, "RplayPriority", 13))
			{
				priority = atoi (token);
			}
#endif
			else if( isspace( *p )) /* Audio <message_type> <audio_file> */
			{
			  int message_len ; 
			  
				for( p = token ; *token && !isspace(*token) ; token++ );
				message_len = token - p ;
				while( isspace(*token) ) token++ ; /* that will get us to the first token */
				if( *token == '\0' ) continue ;    /* no sound requested */

				for (i = 0; i < MAX_SOUNDS; i++)
					if (!mystrncasecmp (p, messages[i], message_len))
					{
						if( sound_config[i] ) free( sound_config[i] );
						sound_config[i] = mystrdup (token);
						break ;
					}
			}
		}
	}
	free (buf);
	fclose (fp);
}

/***********************************************************************
 *
 *  Procedure:
 *	Loop - wait for data to process
 *
 ***********************************************************************/
void
Loop (int *fd)
{
	unsigned long code;
	ASMessage    *asmsg = NULL;
	time_t        now = 0;
	static time_t last_time = 0;

	while (1)
	{
		asmsg = CheckASMessage (fd[1], -1);
		if (asmsg)
		{
			if (asmsg->header[0] == START_FLAG)
				for (code = 0; (0x01 << code) != asmsg->header[1] && code < MAX_MESSAGES; code++);
			else
				code = -1;
			DestroyASMessage (asmsg);
			now = time (0);
			if (code >= 0 && now >= (last_time + (time_t) audio_delay))
			{	/* Play the sound. */
				if( audio_play ((sound_table[code]) ? code : BUILTIN_UNKNOWN) )
					last_time = now;
			}
		}
	}
}


/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means afterstep is dying
 *
 ***********************************************************************/
void
DeadPipe (int nonsense)
{
	done (0);
}

/***********************************************************************
 *
 *  Procedure:
 *	done - common exit point for Audio.
 *
 ***********************************************************************/
void
done (int n)
{
	audio_play (BUILTIN_SHUTDOWN);
	/* printf("shutdown \n"); */
	exit (n);
}

/***********************************************************************
 *
 * Procedure:
 *
 *    audio_play - actually plays sound from lookup table
 *
 **********************************************************************/
#ifdef HAVE_RPLAY_H
Bool audio_player_rplay(short sound)
{
  Bool res = False;
	if (rplay_fd != -1 && rplay_table[sound])
	{
		if (rplay (rplay_fd, rplay_table[sound]) < 0)
			rplay_perror (MyName);
		else
			res = True ;
	}
	return res ;
}
#endif

Bool audio_player_cat_file(short sound, FILE *af)
{
  FILE *sf ;
  Bool res = False;
	if (sound_table[sound] && af != NULL )
	{
		if( (sf = fopen( sound_table[sound], "rb" )) != NULL )
		{
			int c ;
			while( (c = fgetc(sf)) != EOF )
				fputc( c, af );
			fflush(af);
			res = True ;
			fclose(sf);
		}			  
	}
	return res ;
}

Bool audio_player_cat(short sound)
{
  FILE *af ;
  Bool res = False;
	if (sound_table[sound] )
	{
		if( (af = fopen( "/dev/audio", "wb" )) != NULL )
		{
			res = audio_player_cat_file( sound, af );
			fclose(af);
		}			  
	}
	return res ;
}

Bool audio_player_stdout(short sound)
{
    return audio_player_cat_file( sound, stdout );
}

/******************************************************************/
/*     stuff for running external app to play the sound		      */
/******************************************************************/
#ifdef HAVE_SYS_WAIT_H
#define WAIT_CHILDREN(pstatus)  waitpid(-1, pstatus, WNOHANG)
#elif defined (HAVE_WAIT3)
#define WAIT_CHILDREN(pstatus)  wait3(pstatus, WNOHANG, NULL)
#else
#define WAIT_CHILDREN(pstatus)  (-1)
#endif

static int child_PID = 0;
void
child_sighandler (int signum)
{
  int pid;
  int status;
    signal (SIGCHLD, child_sighandler);
    while (1)
    {
  	    pid = WAIT_CHILDREN (&status);
    	if (pid < 0 || pid == child_PID)
		    child_PID = 0;
	    if (pid == 0 || pid < 0)
		    break;
    }
}

/* 
   This should return 0 if process of running external app completed or killed.
   otherwise it returns > 0
 */
int
child_check(Bool kill_it_to_death)
{
  int i;
  int status;

    if (child_PID > 0)
	{
  	    if (kill_it_to_death)
		{
		  kill (child_PID, SIGTERM);
		  for (i = 0; i < 10; i++)	/* give it 10 sec to terminate */
	  	  {
	    	  sleep (1);
	    	  if (WAIT_CHILDREN (&status) <= 0)
			    break;
	  	  }
		  if (i >= 10)
	  		  kill (child_PID, SIGKILL);	/* no more mercy */
		  child_PID = 0;
		}
    }else if (child_PID < 0)
	    child_PID = 0;

    return child_PID;
}

void
child_run_cmd(char *cmd)
{
  signal (SIGCHLD, child_sighandler);
  if (!(child_PID = fork ()))	/* child process */
    {
      execl ("/bin/sh", "/bin/sh", "-c", cmd, (char *) NULL);

      /* if all is fine then the thread will exit here */
      /* so displaying error if not                    */
      fprintf (stderr, "\n%s: bad luck running command [%s].\n", MyName, cmd);
      exit (0);			/*thread completed */
    }
}


Bool audio_player_ext(short sound)
{

  if( child_check(False) > 0 ) return False ;/* previous command still running */
  if( sound_table[sound] == NULL || ext_cmd == NULL ) return False; /* nothing to do */
  
  sprintf( ext_cmd, "%s %s %s", SOUNDPLAYER, audio_play_cmd_line, sound_table[sound] );
  child_run_cmd( ext_cmd );
  
  return True ;	
}
