/*
 * Copyright (C) 2007 Jeremy sG <sg at hacksess>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>

// AS Includes
#include "../../configure.h"
#include "../../libAfterStep/asapp.h"
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/event.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterConf/afterconf.h"
// Defines
#define MAX_SOUNDS AFTERSTEP_EVENTS_NUM
#define mask_reg MAX_MASK

// from afterconf.h ?
AudioConfig     *Config = NULL;

// asapp.h ?
void DeadPipe (int);
void GetBaseOptions (const char *filename/* unused */);
void GetOptions (const char *filename);
void HandleEvents();
void DispatchEvent (ASEvent * event);
void proc_message (send_data_type type, send_data_type *body);

Bool (*audio_play) (short) = NULL;

// audio.c
//char         *sound_table[MAX_SOUNDS];

int main (int argc,char ** argv)
{

        set_DeadPipe_handler(DeadPipe);
        InitMyApp (CLASS_SOUND, argc, argv, NULL, NULL, 0 );
        LinkAfterStepConfig();

        ConnectX( ASDefaultScr, PropertyChangeMask );
        ConnectAfterStep ( mask_reg, 0 );

        // declared via AudioConfig *Config.... (after defines..)
        Config = CreateAudioConfig();

	int err;
	snd_pcm_t *playback_handle;
	snd_pcm_hw_params_t *hw_params;
	int DEBUG1;
	int ret1;
	int SRate, dir;
	typedef char *String;
	//--
	FILE* WaveFile;
	int frames;
	SRate = 22000;
	DEBUG1 = *argv[1];
	
	fprintf(stdout,"ARGX: %i::%s :: %s\n",argc,argv[1],argv[2]);
	
	dir = 0;
	String pcmDeviceID = "default";

	if ((err = snd_pcm_open (&playback_handle, pcmDeviceID, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf (stderr, "cannot open audio device %s (%s)\n", 
			 argv[1],
			 snd_strerror (err));
		exit (1);
	}
	
	if (DEBUG1) { fprintf(stdout,"Opened PCM Device\n"); }
	   
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
			 snd_strerror (err));
		exit (1);
	}
			 
	if (DEBUG1) { fprintf(stdout,"Allocated Params: %i\n",err); }

	if ((err = snd_pcm_hw_params_any (playback_handle,hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
			 snd_strerror (err));
		exit (1);
	}
	
	if (DEBUG1) { fprintf(stdout,"Init Hardware Params\n"); }

	if ((err = snd_pcm_hw_params_set_access (playback_handle,hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
			 snd_strerror (err));
		exit (1);
	}
	
	if (DEBUG1) { fprintf(stdout,"Set access Type\n"); }

	if ((err = snd_pcm_hw_params_set_format (playback_handle,hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf (stderr, "cannot set sample format (%s)\n",
			 snd_strerror (err));
		exit (1);
	}
	
	if (DEBUG1) { fprintf(stdout,"Set Sample Format\n"); }

	// Last two args for this must be pointers. else it will cause a segfault.
	// ?: SRate wont work unless its pointer (in test app). when linked to AS,
	//  : it cant be a pointer.
	// target/chosen exact value is <,=,> val following dir (-1,0,1) :: hmm
	ret1 = snd_pcm_hw_params_set_rate_near(playback_handle,hw_params,SRate,&dir);
	
	if (DEBUG1) { fprintf(stdout,"Set Sample Rate: %i :: %i\n", ret1, dir); }

	if ((err = snd_pcm_hw_params_set_channels (playback_handle,hw_params, 2)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n",snd_strerror(err));
		exit (1);
	}
	
	if (DEBUG1) { fprintf(stdout,"Channel Count\n"); }

	if ((err = snd_pcm_hw_params (playback_handle,hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n",
			 snd_strerror (err));
		exit (1);
	}

	if (DEBUG1) { fprintf(stdout,"Params Set\n"); }

	snd_pcm_hw_params_free (hw_params);
	
	if (DEBUG1) { fprintf(stdout,"Params Free\n"); }

	if ((err = snd_pcm_prepare (playback_handle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
			 snd_strerror (err));
		exit (1);
	}
	
	if (DEBUG1) { fprintf(stdout,"Prepare to Play!\n"); }
	
// --------------------------------------

	int size, read;
	long r;
	char *buffer;

	WaveFile = fopen("online.wav","rb");
	if (WaveFile == NULL) { fprintf(stdout,"File Open Failed\n"); }
	fprintf(stdout,"Wave: %p\n",WaveFile);
	
	fseek(WaveFile,0,SEEK_END);
	size = ftell(WaveFile);
	rewind(WaveFile);
	fseek(WaveFile,128,SEEK_SET);
	
	frames = snd_pcm_bytes_to_frames(playback_handle,size);
	
	fprintf(stdout,"Size: %d FRAMES: %d\n",size,frames);

	buffer = (char *)malloc(size);
	while ((read = fread(buffer,4,frames,WaveFile)) > 0)
	{
		r = snd_pcm_writei(playback_handle,buffer,frames);
	}
// --------------------------------------
	snd_pcm_close (playback_handle);
	
	HandleEvents();
	
//	exit (0);

        return 0;
}
// ------ READ HEADER ------
/*
int asSound_rfileh(char *FileName)
{
    // Find out what kind of file we got... 
    FILE *WaveFile;
    char *buffer1;
    int *Head1;

    WaveFile = fopen("online.wav","rb");
    // Alloc mem for fread.. 
    buffer1 = (char*)malloc(4096);
    // Read one element of 4 bytes.
    // (the first part of wave header)
    Head1 = fread(buffer1,4,1,WaveFile);
    if (Head1 == "RIFF")
    {
        fprintf("Header is RIFF\n");
    } else {
        fprintf("Header is Other?\n");
    }
}*/
// -------------------------
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
        module_wait_pipes_input ( proc_message );
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
void proc_message (send_data_type type, send_data_type *body)
{
        time_t        now = 0;
        static time_t last_time = 0;
        int code = -1 ;
        
    	LOCAL_DEBUG_OUT( "rcvd mssage %lX", type );
                
    if( type == M_PLAY_SOUND )
    {
                show_activity("TYPE: M_PLAY_SOUND\n");
                CARD32 *pbuf = &(body[4]);
        	char *new_name = deserialize_string( &pbuf, NULL );
                SetupSoundEntry( EVENT_PlaySound, new_name);
                free( new_name );
                code = EVENT_PlaySound ;
                show_activity("PLAY_SND 2: [%i]\n",code);
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
                show_activity("WindowADD: [%i]\n",code);
    }else if( res == WP_DataChanged ) 
    {
              show_activity("DATA Changed???");
    }else if( res == WP_DataDeleted )
    {
                code = EVENT_WindowDestroyed ;
                show_activity("WindowDEST: [%i]\n",code);
    }
    }else if( type == M_NEW_DESKVIEWPORT )
    {
                code = EVENT_DeskViewportChanged ;
                // This should be returing "14" for code
                show_activity("ViewPort CHANGE: [%i]\n",code);
    }else if( type == M_NEW_CONFIG )
    {
                code = EVENT_Config ;
                show_activity("New CONFIG: [%i]",code);
    }else if( type == M_NEW_MODULE_CONFIG )
    {
                code = EVENT_ModuleConfig ;
                show_activity("New MOD CONF: [%i]",code);
    }
    else if ( type == M_FOCUS_CHANGE )
    {
                show_activity("Focus Change");
    }
    
    else if ( type == EVENT_WindowShaded )
    {
                code = EVENT_WindowShaded;
                show_activity("Window SHADE: [%i]",code);
    }
    else if ( type == EVENT_WindowUnshaded )
    {
                code = EVENT_WindowUnshaded;
                show_activity("Window UNShade: [%i]",code);
    }
    else if ( type == EVENT_WindowRaised )
    {
                code = EVENT_WindowRaised;
                show_activity("Window Raised: [%i]",code);
    }
    else { show_activity("UKNOWN: %X",type); }
    now = time (0);
    if( code >= 0 )
    {
           show_activity("Code >= 0: [%i]",code);
           if ( now >= (last_time + (time_t) Config->delay))
           {
                if( ply_sound( code ) ) { last_time = now; }
           }
    }
}
int ply_sound(int code)
{
    show_activity("Play Sound! %i\n",code);
    
    return 1;
}
Bool SetupSoundEntry(int code, char *sound)
{
    show_activity("SETsndENT: code [%i] SND [%c]\n",code,*sound);
    return True;
}
