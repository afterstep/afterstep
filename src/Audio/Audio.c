/*
 * Copyright (c) 2000 Sasha Vasko <sasha at aftercode.net>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * afterstep includes:
 */
#define LOCAL_DEBUG
#define EVENT_TRACE


#include "../../configure.h"
#include "../../libAfterStep/asapp.h"

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
RPLAY        *rplay_table[MAX_SOUNDS];
#endif

/* globals */
static int    fd_width;
static int    fd[2];
static int    x_fd;

AudioConfig 	*config = NULL ;
SessionConfig 	*Session = NULL ;
char         	*AudioPath = NULL ;

/* Event mask - we want all events */
#define mask_reg MAX_MASK

/* prototypes */
Bool          SetupSound ();

void          Loop (int *);
void          process_message (unsigned long, unsigned long *);
void          DeadPipe (int);
void          GetOptions (const char *filename);
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

/* then we add path to it and store it in this table : */
char         *sound_table[MAX_SOUNDS];

int
main (int argc, char **argv)
{
	/* Save our program name - for error messages */
	InitMyApp (CLASS_AUDIO, argc, argv, NULL, OPTION_SINGLE|OPTION_RESTART);
	signal (SIGQUIT, DeadPipe);
	signal (SIGSEGV, DeadPipe);
	signal (SIGTERM, DeadPipe);

	x_fd = ConnectX (PropertyChangeMask, False);
	InternUsefulAtoms ();

	Session = GetASSession (Scr.d_depth, MyArgs.override_home, MyArgs.override_share);
	if( MyArgs.override_config )
		SetSessionOverride( Session, MyArgs.override_config );

	/* connect to AfterStep */
	fd_width = get_fd_width ();
	fd[0] = fd[1] = ConnectAfterStep (mask_reg, Scr.wmprops);

#ifdef HAVE_RPLAY_H
	memset( rplay_table, 0x00, sizeof(RPLAY *)*MAX_SOUNDS);
#endif
	memset( sound_table, 0x00, sizeof(char *)*MAX_SOUNDS);

	GetSessionPaths(Session, NULL, &AudioPath, NULL, NULL, NULL, NULL);
    GetOptions("audio");

	if (!SetupSound ())
	{
		show_error("Failed to initialize sound playing routines. Please check your config.");
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

Bool SetupSoundEntry( int code, char *sound )
{
  char *filename1, *filename2 ;

	if( sound == NULL ) return False;
	if( code < 0 || code > MAX_SOUNDS ) return False ;

    filename1 = mystrdup(sound);
    replaceEnvVar (&filename1);
    if( CheckFile( filename1 ) < 0 && AudioPath != NULL )
    {
        filename2 = safemalloc( strlen(AudioPath)+1+strlen(filename1)+1 );
        sprintf( filename2, "%s/%s", AudioPath, filename1 );
        free( filename1 );
        filename1 = filename2 ;
        if( CheckFile( filename2 ) < 0 )
        {
            free( filename1 );
            filename1 = NULL ;
        }
    }
    if( sound_table[code] ) free (sound_table[code]);
    sound_table[code] = filename1 ;

#ifdef HAVE_RPLAY_H
	if( config->rplay_host == NULL )
		sound = sound_table[code] ;

    if( sound )
	{
		if( rplay_table[code] )
			rplay_destroy( rplay_table[code] );
		rplay_table[code] = rplay_create (RPLAY_PLAY);
		rplay_set (rplay_table[code], RPLAY_APPEND,
                   RPLAY_SOUND, sound,
                   RPLAY_PRIORITY, config->rplay_priority,
                   RPLAY_VOLUME, config->rplay_volume, NULL);
	}
#endif
	return True ;
}

Bool SetupSound ()
{
	register int i ;
	int sounds_configured = 0 ;

    if( config == NULL ) return False ;
    /* first lets build our sounds table and see if we actually have anything to play :*/

	for( i = 0 ; i < MAX_SOUNDS ; i++ )
		if( SetupSoundEntry( i, config->sounds[i] ) )
			sounds_configured++ ;

	if( sounds_configured <= 0 )
	{
		show_error( "no sounds configured");
		return False ;
	}

	/* now lets choose what player to use, and if we can use it at all : */
#ifdef HAVE_RPLAY_H
    /*
	 * Builtin rplay support is enabled when AudioPlayCmd == builtin-rplay.
	 */
    if (!mystrcasecmp (config->playcmd, "builtin-rplay"))
	{
        if( config->rplay_host )
        {
            replaceEnvVar (&(config->rplay_host));
            rplay_fd = rplay_open (config->rplay_host);
        }else if( (rplay_fd = rplay_open_display ()) < 0 )
			rplay_fd = rplay_open ("localhost");

		if (rplay_fd < 0)
		{
			rplay_perror (MyName);
			return False ;
		}
		audio_play = audio_player_rplay ;
        return True;
	}
#endif
    if( mystrcasecmp (config->playcmd, "builtin-cat")== 0 )
	{
		audio_play = audio_player_cat ;
    }else if( mystrcasecmp (config->playcmd, "builtin-stdout")== 0)
	{
		audio_play = audio_player_stdout ;
	}else
	{
		audio_play = audio_player_ext ;
        if( config->playcmd == NULL )
            config->playcmd = mystrdup("");
	}

	return (audio_play!=NULL) ;
}

/***********************************************************************
 *  Configuration reading procedures :
 *  GetOptions      - read the sound configuration file.
 ***********************************************************************/
void
GetOptions (const char *filename)
{
	char *realfilename = MakeSessionFile( Session, filename, False );
    if( config )
	{
        DestroyAudioConfig( config );
		config = NULL ;
	}

	if( realfilename )
	{
  		config = ParseAudioOptions( realfilename, MyName );
		free( realfilename );
	}

    if( config == NULL )
	{
  		show_error("no audio configuration available - exiting");
  		exit(0);
    }
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
			code = asmsg->header[1];
			if (asmsg->header[0] == START_FLAG && code >= 0 && code < MAX_MESSAGES )
			{
				if( code == ASM_PlaySound)
					if( asmsg->header[2] > 3 && asmsg->body )
						SetupSoundEntry( code, (char*)&(asmsg->body[3]));
			}else
				code = -1;
			DestroyASMessage (asmsg);
			now = time (0);
            if (code >= 0 && now >= (last_time + (time_t) config->delay))
            {   /* Play the sound. */
                if( audio_play ( code ) )
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
    if( !rplay_table[sound] )
        sound = BUILTIN_UNKNOWN ;
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
    if( !sound_table[sound] )
        sound = BUILTIN_UNKNOWN ;

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
    if( !sound_table[sound] )
        sound = BUILTIN_UNKNOWN ;

    return audio_player_cat_file( sound, stdout );
}

/******************************************************************/
/*     stuff for running external app to play the sound		      */
/******************************************************************/
Bool audio_player_ext(short sound)
{
    if( check_singleton_child(AUDIO_SINGLETON_ID,False) > 0 ) return False ;/* previous command still running */
    if( !sound_table[sound] )
        sound = BUILTIN_UNKNOWN ;

    if( sound_table[sound] == NULL ) return False; /* nothing to do */

    spawn_child( SOUNDPLAYER, AUDIO_SINGLETON_ID, -1, None, 0, True, False, config->playcmd, sound_table[sound], NULL );

    return True ;
}
