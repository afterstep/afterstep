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
void process_message (send_data_type type, send_data_type *body);

// audio.c
char         *sound_table[MAX_SOUNDS];

int main (int argc,char ** argv)
{

        set_DeadPipe_handler(DeadPipe);
    InitMyApp (CLASS_AUDIO, argc, argv, NULL, NULL, 0 );
        LinkAfterStepConfig();

    ConnectX( ASDefaultScr, PropertyChangeMask );
    ConnectAfterStep ( mask_reg, 0 );

        Config = CreateAudioConfig();

        LOCAL_DEBUG_OUT("parsing Options ...%s","");
//    LoadBaseConfig (GetBaseOptions);
        LoadColorScheme();
  //  LoadConfig ("audio", GetOptions);



	int i;
	int err;
	short buf[128];
	snd_pcm_t *playback_handle;
	snd_pcm_hw_params_t *hw_params;
	int DEBUG1;
	int ret1;
	int SRate, dir;
	typedef char *String;
	//--
	FILE* WaveFile;
	int result;
	int frames;

	DEBUG1 = argv[1];
	
	SRate = 22000;
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
			 
	if (DEBUG1) { fprintf(stdout,"Allocated Params\n"); }

	if ((err = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
			 snd_strerror (err));
		exit (1);
	}
	
	if (DEBUG1) { fprintf(stdout,"Init Hardware Params\n"); }

	if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
			 snd_strerror (err));
		exit (1);
	}
	
	if (DEBUG1) { fprintf(stdout,"Set access Type\n"); }

	if ((err = snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf (stderr, "cannot set sample format (%s)\n",
			 snd_strerror (err));
		exit (1);
	}
	
	if (DEBUG1) { fprintf(stdout,"Set Sample Format\n"); }

	// Last two args for this must be pointers. else it will cause a segfault.
	// target/chosen exact value is <,=,> val following dir (-1,0,1) :: hmm
	ret1 = snd_pcm_hw_params_set_rate_near(playback_handle,hw_params,&SRate,&dir);
	
	if (DEBUG1) { fprintf(stdout,"Set Sample Rate: %i :: %i\n", ret1, &dir); }

	if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, 2)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n",
			 snd_strerror (err));
		exit (1);
	}
	
	if (DEBUG1) { fprintf(stdout,"Channel Count\n"); }

	if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
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
	exit (0);
}

