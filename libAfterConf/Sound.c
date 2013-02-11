/*
 * Copyright (c) 2000 Andrew Ferguson <andrew@owsla.cjb.net>
 * Copyright (c) 1998 Sasha Vasko <sasha at aftercode.net>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#define LOCAL_DEBUG

#include "../configure.h"

#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/hints.h"
// :sG:!! Change to ALSA:
#ifdef HAVE_RPLAY_H
#include <rplay.h>
#endif

#include "afterconf.h"

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/

/* number after strings here is the len of str */
TermDef       EventTerms[] = {
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "add_window", 10, TT_FILENAME, EVENT_WindowAdded_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "window_names", 8, TT_FILENAME, EVENT_WindowNames_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "destroy_window", 14, TT_FILENAME, EVENT_WindowDestroyed_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "focus_change", 12, TT_FILENAME, EVENT_WindowActivated_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "raise_window", 12, TT_FILENAME, EVENT_WindowRaised_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "iconify", 7, TT_FILENAME, EVENT_WindowIconified_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "deiconify", 9, TT_FILENAME, EVENT_WindowDeiconified_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "shade", 5, TT_FILENAME, EVENT_WindowShaded_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "unshade", 7, TT_FILENAME, EVENT_WindowUnshaded_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "stick", 5, TT_FILENAME, EVENT_WindowStuck_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "unstick", 7, TT_FILENAME, EVENT_WindowUnstuck_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "maximize", 8, TT_FILENAME, EVENT_WindowMaximized_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "restore", 7, TT_FILENAME, EVENT_WindowRestored_ID, NULL}
	,

	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "stuck", 5, TT_FILENAME, EVENT_WindowStuck_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "unstuck", 7, TT_FILENAME, EVENT_WindowUnstuck_ID, NULL}
	,

	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "new_background", 14, TT_FILENAME, EVENT_BackgroundChanged_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "new_viewport", 12, TT_FILENAME, EVENT_DeskViewportChanged_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "startup", 7, TT_FILENAME, EVENT_Startup_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "shutdown", 8, TT_FILENAME, EVENT_Shutdown_ID, NULL}
	,

	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "config", 6, TT_FILENAME, EVENT_Config_ID, NULL}
	,
	{TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "module_config", 13, TT_FILENAME, EVENT_ModuleConfig_ID, NULL}
	,

	{0, NULL, 0, 0, 0}						   /* end of structure */
};

SyntaxDef     SoundEventsSyntax = {
	',',
	'\n',
	EventTerms,
	0,										   /* use default hash size */
	' ',
	"",
	"\t",
	"Module:Sound event configuration",
	"SoundEvents",
	"names for different windowing events",
	NULL,
	0
};


TermDef       SoundTerms[] = {
	{TF_DONT_SPLIT, "Path", 8, TT_TEXT, SOUND_PATH_ID, NULL}
	,
	{TF_DONT_SPLIT, "PcmDevice", 7, TT_TEXT, SOUND_PCMDEVICE_ID, NULL}
	,
	{0, "Delay", 5, TT_INTEGER, SOUND_DELAY_ID, NULL}
	,
	{0, "RplayHost", 9, TT_TEXT, SOUND_RPLAY_HOST_ID, NULL}
	,
	{0, "RplayPriority", 13, TT_INTEGER, SOUND_RPLAY_PRI_ID, NULL}
	,
	{0, "Debug", 11, TT_INTEGER, SOUND_DEBUG_ID, NULL}
	,
	{0, "", 0, TT_FLAG, SOUND_SOUND_ID, &SoundEventsSyntax}
	,
	{0, NULL, 0, 0, 0}						   /* end of structure */

};

SyntaxDef     SoundSyntax = {
	'\n',
	'\0',
	SoundTerms,
	0,										   /* use default hash size */
	' ',
	"",
	"\t",
	"Module:Sound",
	"Sound",
	"AfterStep module for playing sounds when windowing event occurs",
	NULL,
	0
};

/************** Create/Destroy SoundConfig ***************/

SoundConfig  *
CreateSoundConfig ()
{
	SoundConfig  *config = (SoundConfig *) safecalloc (1, sizeof (SoundConfig));

	/* let's initialize Sound's config with some nice values: */
/*
#ifdef HAVE_RPLAY_H
    config->rplay_volume = RPLAY_DEFAULT_VOLUME;
    config->rplay_priority = RPLAY_DEFAULT_PRIORITY;
#endif
*/
	return config;
}

void
DestroySoundConfig (SoundConfig * config)
{
	register int  i;

	for (i = EVENT_ID_END - EVENT_ID_START - 1; i >= 0; i--)
		if (config->sounds[i])
			free (config->sounds[i]);

	if (config->pcmdevice)
		free (config->pcmdevice);

	DestroyFreeStorage (&(config->more_stuff));
	free (config);
}

/************ Actual Parsing **************/

SoundConfig  *
ParseSoundOptions (const char *filename, char *myname)
{
	ConfigData    cd;
	ConfigDef    *SoundConfigReader;
	SoundConfig  *config = CreateSoundConfig ();

	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem    item;

	cd.filename = filename;
	SoundConfigReader = InitConfigReader (myname, &SoundSyntax, CDT_Filename, cd, NULL);
	if (!SoundConfigReader)
		return config;

	item.memory = NULL;
	PrintConfigReader (SoundConfigReader);
	ParseConfig (SoundConfigReader, &Storage);

	/* getting rid of all the crap first */
	StorageCleanUp (&Storage, &(config->more_stuff), CF_DISABLED_OPTION);

	for (pCurr = Storage; pCurr; pCurr = pCurr->next)
	{
		if (pCurr->term == NULL)
			continue;

		{
			if (!ReadConfigItem (&item, pCurr))
				continue;

			switch (pCurr->term->id)
			{
			 case SOUND_SOUND_ID:
				 if (pCurr->sub)
				 {
					 if (pCurr->sub->term && pCurr->sub->argv)
						 if (pCurr->sub->term->id >= EVENT_ID_START && pCurr->sub->term->id < EVENT_ID_END)
							 set_string (&(config->sounds[pCurr->sub->term->id - EVENT_ID_START]),
										 mystrdup (pCurr->sub->argv[0]));
				 }
				 break;
			 case SOUND_PCMDEVICE_ID:
				 //set_string( &(config->pcmdevice), item.data.string );
				 config->pcmdevice = item.data.string;
				 break;
			 case SOUND_PATH_ID:
				 config->path = item.data.string;
				 break;

			 case SOUND_DEBUG_ID:
				 config->debug = (int)item.data.integer;
				 break;
/*				 
			 case SOUND_DELAY_ID:
			 	 set_flags( config->set_flags, SOUND_SET_DELAY );
				 config->delay = (int)item.data.integer;
				 break;
	*/
				 /*
				    case SOUND_RPLAY_HOST_ID:
				    set_string_value( &(config->rplay_host), item.data.string, &(config->set_flags), SOUND_SET_RPLAY_HOST );
				    break;
				    case SOUND_RPLAY_PRI_ID:
				    set_flags( config->set_flags, SOUND_SET_RPLAY_PRIORITY );
				    config->rplay_priority = (int)item.data.integer;
				    break;
				    case SOUND_RPLAY_VOL_ID:
				    set_flags( config->set_flags, SOUND_SET_RPLAY_VOLUME );
				    config->rplay_volume = (int)item.data.integer;
				    break;
				  */
			 default:
				 item.ok_to_free = 1;
			}
		}
	}

	ReadConfigItem (&item, NULL);

	DestroyConfig (SoundConfigReader);
	DestroyFreeStorage (&Storage);
	return config;

}

int
WriteSoundOptions (const char *filename, char *myname, SoundConfig * config, unsigned long flags)
{
	ConfigDef    *SoundConfigWriter = NULL;
	FreeStorageElem *Storage = NULL, **tail = &Storage;
	register int  i;
	ConfigData    cd;

	if (config == NULL)
		return 1;
	cd.filename = filename;

	if ((SoundConfigWriter = InitConfigWriter (myname, &SoundSyntax, CDT_Filename, cd)) == NULL)
		return 2;
	CopyFreeStorage (&Storage, config->more_stuff);
	/* building free storage here */
	/* PCM Device */
	if (config->pcmdevice)
		tail = String2FreeStorage (&SoundSyntax, tail, config->pcmdevice, SOUND_PCMDEVICE_ID);

	if (config->debug)
		tail = Integer2FreeStorage (&SoundSyntax, tail, NULL, config->debug, SOUND_DEBUG_ID);
	/* delay */
/*
	if( get_flags(config->set_flags, SOUND_SET_DELAY) )
    	tail = Integer2FreeStorage (&SoundSyntax, tail, NULL, config->delay, SOUND_DELAY_ID);
*/
	/* rplay_host */
//  if (get_flags(config->set_flags, SOUND_SET_RPLAY_HOST) && config->rplay_host)
//        tail = String2FreeStorage (&SoundSyntax, tail, config->rplay_host, SOUND_RPLAY_HOST_ID);

	/* rplay_priority */
//  if (get_flags(config->set_flags, SOUND_SET_RPLAY_PRIORITY))
//        tail = Integer2FreeStorage (&SoundSyntax, tail, NULL, config->rplay_priority, SOUND_RPLAY_PRI_ID);

	/* rplay_volume */
//  if( get_flags(config->set_flags, SOUND_SET_RPLAY_VOLUME) )
//        tail = Integer2FreeStorage (&SoundSyntax, tail, NULL, config->rplay_volume, SOUND_RPLAY_VOL_ID);

	/* line structure */
	for (i = EVENT_ID_END - EVENT_ID_START - 1; i >= 0; i--)
		if (config->sounds[i])
		{
			FreeStorageElem **dtail = tail;

			tail = Flag2FreeStorage (&SoundSyntax, tail, SOUND_SOUND_ID);
			if (*dtail)
				Path2FreeStorage (&SoundEventsSyntax, &((*dtail)->sub), NULL, config->sounds[i], EVENT_ID_START + i);
		}

	/* writing config into the file */
	WriteConfig (SoundConfigWriter, Storage, CDT_Filename, &cd, flags);
	DestroyFreeStorage (&Storage);
	DestroyConfig (SoundConfigWriter);

	if (Storage)
	{
		fprintf (stderr, "\n%s:Config Writing warning: Not all Free Storage discarded! Trying again...", myname);
		DestroyFreeStorage (&Storage);
		fprintf (stderr, (Storage != NULL) ? " failed." : " success.");
	}
	return 0;
}
