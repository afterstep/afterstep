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
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/event.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterConf/afterconf.h"

#define MAX_SOUNDS			AFTERSTEP_EVENTS_NUM

/*
 * rplay stuff:
 */
#ifdef HAVE_RPLAY_H
#include <rplay.h>
int           rplay_fd = -1;

/* define the rplay table */
RPLAY        *rplay_table[MAX_SOUNDS];
#endif

AudioConfig 	*Config = NULL ;

/* Event mask - we want all events */
#define mask_reg MAX_MASK

/* prototypes */
Bool SetupSound ();

void DeadPipe (int);
void GetBaseOptions (const char *filename/* unused */);
void GetOptions (const char *filename);
void HandleEvents();
void DispatchEvent (ASEvent * event);
void process_message (send_data_type type, send_data_type *body);

/* this is the pointer to one of the play functions, chosen in SetupSound() : */
Bool (*audio_play) (short) = NULL;
/* these are possible players defined below : */
#ifdef HAVE_RPLAY_H
Bool audio_player_rplay(short) ;
#endif
Bool audio_player_cat(short) ;
Bool audio_player_stdout(short) ;
Bool audio_player_ext(short) ;
void DeadPipe(int);

/* then we add path to it and store it in this table : */
char         *sound_table[MAX_SOUNDS];

int
main (int argc, char **argv)
{
	/* Save our program name - for error messages */
	set_DeadPipe_handler(DeadPipe);
    InitMyApp (CLASS_AUDIO, argc, argv, NULL, NULL, 0 );

    ConnectX( ASDefaultScr, PropertyChangeMask );
    ConnectAfterStep ( mask_reg);
	
	Config = CreateAudioConfig();
	
	LOCAL_DEBUG_OUT("parsing Options ...%s","");
    LoadBaseConfig (GetBaseOptions);
	LoadColorScheme();
    LoadConfig ("audio", GetOptions);

#ifdef HAVE_RPLAY_H
	memset( rplay_table, 0x00, sizeof(RPLAY *)*MAX_SOUNDS);
#endif
	memset( sound_table, 0x00, sizeof(char *)*MAX_SOUNDS);

	if (!SetupSound ())
	{
		show_error("Failed to initialize sound playing routines. Please check your config.");
		return 1;
	}

	/*
	 * Play the startup sound.
	 */
	audio_play (EVENT_Startup);
	SendInfo ( "Nop", 0);
    HandleEvents();

	return 0;
}

Bool SetupSoundEntry( int code, char *sound )
{
  char *filename1, *filename2 ;

	if( sound == NULL ) return False;
	if( code < 0 || code > MAX_SOUNDS ) return False ;

    filename1 = mystrdup(sound);
    replaceEnvVar (&filename1);
    if( CheckFile( filename1 ) < 0 && Environment->audio_path != NULL )
    {
        filename2 = safemalloc( strlen(Environment->audio_path)+1+strlen(filename1)+1 );
        sprintf( filename2, "%s/%s", Environment->audio_path, filename1 );
        free( filename1 );
        filename1 = filename2 ;
        if( CheckFile( filename2 ) < 0 )
        {
            free( filename1 );
            filename1 = NULL ;
        }
    }
    if( sound_table[code] ) 
		free (sound_table[code]);
    sound_table[code] = filename1 ;

#ifdef HAVE_RPLAY_H
	if( Config->rplay_host == NULL )
		sound = sound_table[code] ;

    if( sound )
	{
		if( rplay_table[code] )
			rplay_destroy( rplay_table[code] );
		rplay_table[code] = rplay_create (RPLAY_PLAY);
		rplay_set (rplay_table[code], RPLAY_APPEND,
                   RPLAY_SOUND, sound,
                   RPLAY_PRIORITY, Config->rplay_priority,
                   RPLAY_VOLUME, Config->rplay_volume, NULL);
	}
#endif
	return True ;
}

Bool SetupSound ()
{
	register int i ;
	int sounds_configured = 0 ;

    if( Config == NULL ) return False ;
    /* first lets build our sounds table and see if we actually have anything to play :*/

	for( i = 0 ; i < MAX_SOUNDS ; i++ )
		if( SetupSoundEntry( i, Config->sounds[i] ) )
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
    if (!mystrcasecmp (Config->playcmd, "builtin-rplay"))
	{
        if( Config->rplay_host )
        {
            replaceEnvVar (&(Config->rplay_host));
            rplay_fd = rplay_open (Config->rplay_host);
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
    if( mystrcasecmp (Config->playcmd, "builtin-cat")== 0 )
	{
		audio_play = audio_player_cat ;
    }else if( mystrcasecmp (Config->playcmd, "builtin-stdout")== 0)
	{
		audio_play = audio_player_stdout ;
	}else
	{
		audio_play = audio_player_ext ;
        if( Config->playcmd == NULL )
            Config->playcmd = mystrdup("");
	}

	return (audio_play!=NULL) ;
}

/***********************************************************************
 *  Configuration reading procedures :
 *  GetOptions      - read the sound configuration file.
 ***********************************************************************/
void
GetBaseOptions (const char *filename/* unused*/)
{
    START_TIME(started);
	ReloadASEnvironment( NULL, NULL, NULL, False );
    SHOW_TIME("BaseConfigParsingTime",started);
}


void
GetOptions (const char *filename)
{
    AudioConfig *config = ParseAudioOptions (filename, MyName);
	int i ;
    START_TIME(option_time);

    /* Need to merge new config with what we have already :*/
    /* now lets check the config sanity : */
    /* mixing set and default flags : */
    Config->set_flags |= config->set_flags;

  	if( config->playcmd != NULL ) 
		set_string_value( &(Config->playcmd), config->playcmd, NULL, 0 );
	for( i = 0 ; i < AFTERSTEP_EVENTS_NUM ; ++i ) 
  		if( config->sounds[i] != NULL ) 
			set_string_value( &(Config->sounds[i]), config->sounds[i], NULL, 0 );
    
	if( get_flags(config->set_flags, AUDIO_SET_DELAY) )
        Config->delay = config->delay;
    if( get_flags(config->set_flags, AUDIO_SET_RPLAY_HOST) && config->rplay_host != NULL )
		set_string_value( &(Config->rplay_host), config->rplay_host, NULL, 0 );
    if( get_flags(config->set_flags, AUDIO_SET_RPLAY_PRIORITY) )
        Config->rplay_priority = config->rplay_priority;
    if( get_flags(config->set_flags, AUDIO_SET_RPLAY_VOLUME) )
        Config->rplay_volume = config->rplay_volume;

    DestroyAudioConfig (config);
    SHOW_TIME("Config parsing",option_time);
}

/***********************************************************************
 *
 *  Procedure:
 *	Loop - wait for data to process
 *
 ***********************************************************************/
void HandleEvents()
{
	ASEvent event;
    Bool has_x_events = False ;
    while (True)
    {
        while((has_x_events = XPending (dpy)) )
        {
            if( ASNextEvent (&(event.x), True) )
            {
                event.client = NULL ;
                setup_asevent_from_xevent( &event );
                DispatchEvent( &event );
			}
        }
        module_wait_pipes_input ( process_message );
    }
}

void
DispatchEvent (ASEvent * event)
{
    SHOW_EVENT_TRACE(event);
    switch (event->x.type)
    {
	    case PropertyNotify:
			handle_wmprop_event (Scr.wmprops, &(event->x));
			return ;
        default:
            return;
    }
}

void
process_message (send_data_type type, send_data_type *body)
{
	time_t        now = 0;
	static time_t last_time = 0;
	int code = -1 ;
	
    LOCAL_DEBUG_OUT( "received message %lX", type );
		
	if( type == M_PLAY_SOUND ) 
	{
		CARD32 *pbuf = &(body[4]);
       	char *new_name = deserialize_string( &pbuf, NULL );
  		SetupSoundEntry( EVENT_PlaySound, new_name);
		free( new_name );
		code = EVENT_PlaySound ;
	}else if( (type&WINDOW_PACKET_MASK) != 0 )
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );
		WindowPacketResult res ;
        /* saving relevant client info since handle_window_packet could destroy the actuall structure */
		ASFlagType old_state = wd?wd->state_flags:0 ; 
        show_activity( "message %lX window %X data %p", type, body[0], wd );
		res = handle_window_packet( type, body, &wd );
		LOCAL_DEBUG_OUT( "result = %d", res );
        if( res == WP_DataCreated )
		{	
			code = EVENT_WindowAdded ;
		}else if( res == WP_DataChanged )
		{	
			
		}else if( res == WP_DataDeleted )
        {
			code = EVENT_WindowDestroyed ;
        }
    }else if( type == M_NEW_DESKVIEWPORT )
    {
		code = EVENT_DeskViewportChanged ;
	}else if( type == M_NEW_CONFIG )
	{
	 	code = EVENT_Config ;
	}else if( type == M_NEW_MODULE_CONFIG )
	{
		code = EVENT_ModuleConfig ;
	}
    now = time (0);
	if( code >= 0 )
	    if ( now >= (last_time + (time_t) Config->delay))
			if( audio_play ( code ) )
    	    	last_time = now;
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
	audio_play (EVENT_Shutdown);
    if( Config )
        DestroyAudioConfig (Config);

    FreeMyAppResources();
#ifdef DEBUG_ALLOCS
    print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

    XFlush (dpy);
    XCloseDisplay (dpy);
    exit (0);
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
    if( sound_table[sound] )
	    return audio_player_cat_file( sound, stdout );
	return False;
}

/******************************************************************/
/*     stuff for running external app to play the sound		      */
/******************************************************************/
Bool audio_player_ext(short sound)
{
    if( check_singleton_child(AUDIO_SINGLETON_ID,False) > 0 ) return False ;/* previous command still running */
    if( sound_table[sound] == NULL ) return False; /* nothing to do */

    spawn_child( SOUNDPLAYER, AUDIO_SINGLETON_ID, -1, None, 0, True, False, Config->playcmd, sound_table[sound], NULL );

    return True ;
}
