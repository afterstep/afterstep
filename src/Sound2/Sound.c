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
// This is for the debugging.. and prolly will just remove it      
#include <time.h>

// AS Includes
#include "../../configure.h"
#include "../../libAfterStep/asapp.h"
#include "../../libAfterStep/screen.h"  
#include "../../libAfterStep/event.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterConf/afterconf.h"
#include "../../libAfterStep/clientprops.h"
#include "Sound.h"
// Defines
#define MAX_SOUNDS AFTERSTEP_EVENTS_NUM
#define mask_reg MAX_MASK

// from afterconf.h ?
SoundConfig     *Config = NULL;

// asapp.h ?
void DeadPipe (int);
void GetBaseOptions (const char *filename/* unused */);
void GetOptions (const char *filename);
void HandleEvents();
void DispatchEvent (ASEvent * event);  
void proc_message (send_data_type type, send_data_type *body);

// Sound2 Functions
int as_snd2_init_stg1();
int as_snd2_init_stg2();
//int as_snd2_waveheader(char *WaveFile);
int as_snd2_playsound(const char *SoundName);

// ?
Bool (*sound_play) (short) = NULL;

// Global Vars
struct SND2dev SND2devS;
SND2devP = &SND2devS;
       
int main(int argc,char ** argv)
{
    // Standard AS Stuff
    set_DeadPipe_handler(DeadPipe);
    InitMyApp(CLASS_SOUND,argc,argv,NULL,NULL,0);
    LinkAfterStepConfig();
    
    ConnectX(ASDefaultScr,PropertyChangeMask);
    ConnectAfterStep(mask_reg,0);
    
    // need to figure out what this is for...
    Config = CreateSoundConfig();

    LoadBaseConfig(GetBaseOptions);
    LoadConfig("sound",GetOptions);
    
    //fprintf(stdout,"--Audio Conf: %i",Config->delay);

    as_snd2_init_stg1();
    as_snd2_init_stg2();
    
    as_snd2_playsound("test");
    
    HandleEvents();
    
    return 0;
}

int as_snd2_init_stg1()
{
    // Stage 1 Sound2 module Init.
    
    typedef char *String;
    int Snd2DEBUG, ret1;
    String pcmDeviceID = "plughw:0,0"; // Need to make Config File opt for setting this.
    //String pcmDeviceID = "default";

    Snd2DEBUG = 1;

    if (Snd2DEBUG) { fprintf(stdout,"Opening PCM Device....\n"); }
    
    // (Returned PCM Handle, ASCII Ident of Handle, Wanted Stream, Open Mode)
    snd_pcm_open(&SND2devS.pcm_handle,pcmDeviceID,SND_PCM_STREAM_PLAYBACK,0);
  
    if (Snd2DEBUG) { fprintf(stdout,"Allocating Hardware Param Structure....\n"); }
    
    // (Returned Pointer)
    ret1 = snd_pcm_hw_params_malloc(&SND2devS.hw_params);
    
    if (Snd2DEBUG) { fprintf(stdout,"Init Hardware Param Structure...\n");}
    
    // (PCM Handle, Config Space)
    snd_pcm_hw_params_any(SND2devS.pcm_handle,SND2devS.hw_params);
    
    if (Snd2DEBUG) { fprintf(stdout,"Setting Access Type...\n"); }
    
    // (PCM Handle, Config Space, Access Type)
    snd_pcm_hw_params_set_access(SND2devS.pcm_handle,SND2devS.hw_params,SND_PCM_ACCESS_RW_INTERLEAVED);
    
    if (Snd2DEBUG) { fprintf(stdout,"Setting Sample Format...\n"); }
    
    // (PCM Handle, Config Space, Format)
    // SND_PCM_FORMAT_S16_LE = Signed 16-bit Little Endian
    // May need to var-ize last part, and detect cpu type on configure.
    snd_pcm_hw_params_set_format(SND2devS.pcm_handle,SND2devS.hw_params,SND_PCM_FORMAT_S16_LE);
    
    if (Snd2DEBUG) { fprintf(stdout,"Stage 1 Init Complete!\n\n"); }
    
    return 1;
}

int as_snd2_init_stg2()
{
    // Following Init depends on the file type.
    // since we're setting Sample rate, channel count (mono,stereo), etc.
    
    int Snd2DEBUG;
    int SRate, CHcount, dir;
    
    SRate = 22000;
    CHcount = 2;

    Snd2DEBUG = 1;

    if (Snd2DEBUG) { fprintf(stdout,"Setting Sample Rate...\n"); }
    
    // (PCM Handle, Config Space, Sample Rate, Sub Unit Direction)
    snd_pcm_hw_params_set_rate_near(SND2devS.pcm_handle,SND2devS.hw_params,SRate,&dir);
    
    if (Snd2DEBUG) { fprintf(stdout,"Setting Channel Count...\n"); }
    
    // (PCM Handle, Config Space, Channel Count)
    snd_pcm_hw_params_set_channels(SND2devS.pcm_handle,SND2devS.hw_params,CHcount);
    
    if (Snd2DEBUG) { fprintf(stdout,"Apply Hardware Params...\n"); }
    
    // (PCM Handle, Config Space)
    snd_pcm_hw_params(SND2devS.pcm_handle,SND2devS.hw_params);
    
    if (Snd2DEBUG) { fprintf(stdout,"Free Hardware Space Container...\n"); }
    
    // (Config Space)
    snd_pcm_hw_params_free(SND2devS.hw_params);
    
    if (Snd2DEBUG) { fprintf(stdout,"Stage 2 Init Complete!\n\n"); }
    
    //HandleEvents(SND2devP);
    
    return 1;
}
/*
int as_snd2_waveheader(char *WaveFile)
{
    // Read Wave file header info
    FILE *WaveFileR
    char *headBuff;
    int *wavHead;
    
    WaveFileR = fopen(WaveFile,"rb");
    
    headBuff = (char*)malloc(256);
    
}
*/
int as_snd2_playsound(const char *SndName)
{
    // Lets play some sound now!
    
    char *SFbuffer;
    int SNDFileSize, SNDFileFrames, SNDFileRead;
    FILE *SNDFile;
    
    SNDFile = fopen("online.wav","rb");
    if (SNDFile == NULL) { fprintf(stdout,"Failed to open Sound File\n"); exit(1); }

//    if (Snd2DEBUG) { fprintf(stdout,"Prepare To Play!...\n"); }
    snd_pcm_prepare(SND2devS.pcm_handle);    

    fprintf(stdout,"Play Sound: %s\n",SndName);
    
    fseek(SNDFile,0,SEEK_END); // We're trying to get the full file size here...
    SNDFileSize = ftell(SNDFile);
    rewind(SNDFile); // Now that we got size, go back to beginning
    fseek(SNDFile,128,SEEK_SET); // we're skipping first 128 bytes. its unplayable (header info)
    
    SNDFileFrames = snd_pcm_bytes_to_frames(SND2devS.pcm_handle,SNDFileSize);
    
    SFbuffer = (char*)malloc(SNDFileSize);
    while ((SNDFileRead = fread(SFbuffer,4,SNDFileFrames,SNDFile)) > 0)
    {
          snd_pcm_writei(SND2devS.pcm_handle,SFbuffer,SNDFileFrames);
    }
    
    return 1;
}

/* ======== STANDARD AS STUFF ============ */
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
void DispatchEvent (ASEvent * event)
{
    SHOW_EVENT_TRACE(event);
    switch (event->x.type)  
    {
        case PropertyNotify:
            handle_wmprop_event (Scr.wmprops, &(event->x));
            break;
    }
}    
void proc_message(send_data_type type, send_data_type *body)
{
    int StateChange, code = -1;
    
    if (type == M_PLAY_SOUND) { show_activity("M_PLY_SND"); } // Need to find whats calling this type

    else if ((type & WINDOW_PACKET_MASK) != 0)
    {
        struct ASWindowData *wd = fetch_window_by_id(body[0]);
        WindowPacketResult res;
        
        ASFlagType old_state = wd ? wd->state_flags:0;
        res = handle_window_packet(type,body,&wd);
        
        if (res == WP_DataCreated)
        {
            code = EVENT_WindowAdded;
        }
        else if (res == WP_DataChanged)
        {
            if (type == M_FOCUS_CHANGE)
            {	
                show_activity("FOC_CHANGE"); 
                as_snd2_playsound("focus");
            }
            
            StateChange = wd->state_flags ^ old_state;
            if (StateChange)
            {
                if (StateChange & AS_Shaded)
                {
                    show_activity("STATE: Shade");
                    as_snd2_playsound("shade");
                }
                if (StateChange & AS_Iconic)
                {	
                    show_activity("STATE: Iconic"); 
                    as_snd2_playsound("iconic");
                }
                if (StateChange & AS_Sticky)
                {	
                    show_activity("STATE: Sticky");
                    as_snd2_playsound("sticky");
                }
                if (StateChange & AS_MaximizedY);
                {
                    show_activity("STATE: MaximizedY");
                    as_snd2_playsound("maximized");
                }
            }
            res = -1;
        }
        else if (res == WP_DataDeleted)
        {	code = EVENT_WindowDestroyed; }
        
    }
    else if (type == M_NEW_DESKVIEWPORT)
    {
        show_activity("-STATE: VIEWPORT CHANGE");
        as_snd2_playsound("viewport");
    }
}
void
GetBaseOptions (const char *filename/* unused*/)
{
    START_TIME(started);
        ReloadASEnvironment( NULL, NULL, NULL, False, False );
    SHOW_TIME("BaseConfigParsingTime",started);
}
void
GetOptions (const char *filename)
{
    fprintf(stdout,"*****GETOPTIONS START******\n");
    fprintf(stdout,"[%s]--FName: %s\n",MyName,filename);
    SoundConfig *config = ParseSoundOptions (filename,MyName);

    int i ;
    START_TIME(option_time);
    
 //   fprintf(stdout,"--1DELAY: %i\n",config->delay);
  //  fprintf(stdout,"--CMD: %s\n",config->playcmd);
//    fprintf(stdout,"--PTH: %s\n",config->sounds[15]);

    /* Need to merge new config with what we have already :*/
    /* now lets check the config sanity : */
    /* mixing set and default flags : */
    Config->set_flags |= config->set_flags;

//    if( config->playcmd != NULL )
//        set_string( &(Config->playcmd), config->playcmd);
        
    for( i = 0 ; i < AFTERSTEP_EVENTS_NUM ; ++i ) 
    {
        if( config->sounds[i] != NULL )
        {
            fprintf(stdout,"--SOUNDS: [%i]: %s\n",i,config->sounds[i]);
            set_string( &(Config->sounds[i]), config->sounds[i]);
        }
    }
    
    // This delay option seems... unneeded.
//    if( get_flags(config->set_flags, AUDIO_SET_DELAY) )
//    {
//        Config->delay = config->delay;
//        fprintf(stdout,"---DELAY: %i",Config->delay);
//    }

    DestroySoundConfig (config);
    SHOW_TIME("Config parsing",option_time);
    fprintf(stdout,"******GETOPTIONS END*****\n");
}
 
