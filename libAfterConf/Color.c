/*
 * Copyright (c) 2003 Sasha Vasko <sasha at aftercode.net>
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


#include "../configure.h"

#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/colorscheme.h"

#include "afterconf.h"


/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the colors config
 * file
 *
 ****************************************************************************/
#define ASCOLOR_TERM(name,len)  \
    {TF_NO_MYNAME_PREPENDING, #name, len, TT_COLOR, ASCOLOR_##name##_ID , NULL},

TermDef       ASColorTerms[] = {
	ASCOLOR_TERM(Base,4),
	ASCOLOR_TERM(Inactive1,9),
	ASCOLOR_TERM(Inactive2,9),
	ASCOLOR_TERM(Active,6),
	ASCOLOR_TERM(InactiveText1,13),
	ASCOLOR_TERM(InactiveText2,13),
	ASCOLOR_TERM(ActiveText,10),
	ASCOLOR_TERM(HighInactive,12),
	ASCOLOR_TERM(HighActive,10),
	ASCOLOR_TERM(HighInactiveBack,16),
	ASCOLOR_TERM(HighActiveBack,14),
	ASCOLOR_TERM(HighInactiveText,16),
	ASCOLOR_TERM(HighActiveText,14),
	ASCOLOR_TERM(DisabledText,12),
	ASCOLOR_TERM(BaseDark,8),
	ASCOLOR_TERM(BaseLight,9),
	ASCOLOR_TERM(Inactive1Dark,13),
	ASCOLOR_TERM(Inactive1Light,14),
	ASCOLOR_TERM(Inactive2Dark,13),
	ASCOLOR_TERM(Inactive2Light,14),
	ASCOLOR_TERM(ActiveDark,10),
	ASCOLOR_TERM(ActiveLight,11),
	ASCOLOR_TERM(HighInactiveDark,16),
	ASCOLOR_TERM(HighInactiveLight,17),
	ASCOLOR_TERM(HighActiveDark,14),
	ASCOLOR_TERM(HighActiveLight,15),
	ASCOLOR_TERM(HighInactiveBackDark,20),
	ASCOLOR_TERM(HighInactiveBackLight,21),
	ASCOLOR_TERM(HighActiveBackDark,24),
	ASCOLOR_TERM(HighActiveBackLight,25),

	{TF_NO_MYNAME_PREPENDING, "Angle", 5, TT_UINTEGER, ASCOLOR_Angle_ID, NULL},
	{0, NULL, 0, 0, 0}
};

SyntaxDef     ASColorSyntax = {
	'\n',
	'\0',
    ASColorTerms,
	0,										   /* use default hash size */
    ' ',
	"",
	"\t",
    "AfterStep custom Color Scheme",
	NULL,
	0
};


void
PrintColorScheme (ASColorScheme *config )
{
}

ASColorScheme   *
ParseBaseOptions (const char *filename, char *myname)
{
	ConfigDef    *ConfigReader = InitConfigReader (myname, &BaseSyntax, CDT_Filename, (void *)filename,
												   NULL);
	BaseConfig   *config = CreateBaseConfig ();
	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem    item;

	if (!ConfigReader)
		return config;

	item.memory = NULL;
	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &Storage);
	PrintFreeStorage (Storage);

	/* getting rid of all the crap first */
	StorageCleanUp (&Storage, &(config->more_stuff), CF_DISABLED_OPTION);

	for (pCurr = Storage; pCurr; pCurr = pCurr->next)
	{
		if (pCurr->term == NULL)
			continue;
		if (!ReadConfigItem (&item, pCurr))
			continue;
		switch (pCurr->term->id)
		{
		 case BASE_MODULE_PATH_ID:
			 set_string_value( &(config->module_path), item.data.string, NULL, 0 );
			 break;
		 case BASE_AUDIO_PATH_ID:
			 set_string_value( &(config->audio_path), item.data.string, NULL, 0 );
			 break;
		 case BASE_ICON_PATH_ID:
			 set_string_value( &(config->icon_path), item.data.string, NULL, 0 );
			 break;
		 case BASE_PIXMAP_PATH_ID:
			 set_string_value( &(config->pixmap_path), item.data.string, NULL, 0 );
			 break;
		 case BASE_FONT_PATH_ID:
			 set_string_value( &(config->font_path), item.data.string, NULL, 0 );
			 break;
		 case BASE_CURSOR_PATH_ID:
			 set_string_value( &(config->cursor_path), item.data.string, NULL, 0 );
			 break;
		 case BASE_MYNAME_PATH_ID:
			 set_string_value( &(config->myname_path), item.data.string, NULL, 0 );
			 break;
		 case BASE_DESKTOP_SIZE_ID:
			 config->desktop_size = item.data.geometry;
			 /* errorneous value check */
			 if (!(config->desktop_size.flags & WidthValue))
				 config->desktop_size.width = 1;
			 if (!(config->desktop_size.flags & HeightValue))
				 config->desktop_size.height = 1;
			 config->desktop_size.flags = WidthValue | HeightValue;
			 break;
		 case BASE_DESKTOP_SCALE_ID:
			 config->desktop_scale = item.data.integer;
			 /* errorneous value check */
			 if (config->desktop_scale < 1)
				 config->desktop_scale = 1;
			 break;
		 default:
			 item.ok_to_free = 1;
		}
	}
	ReadConfigItem (&item, NULL);

	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);
	return config;
}

/* returns:
 *		0 on success
 *		1 if data is empty
 *		2 if ConfigWriter cannot be initialized
 *
 */
int
WriteBaseOptions (const char *filename, char *myname, BaseConfig * config, unsigned long flags)
{
	ConfigDef    *BaseConfigWriter = NULL;
	FreeStorageElem *Storage = NULL, **tail = &Storage;

	if (config == NULL)
		return 1;
	if ((BaseConfigWriter = InitConfigWriter (myname, &BaseSyntax, CDT_Filename, (void *)filename)) == NULL)
		return 2;

	CopyFreeStorage (&Storage, config->more_stuff);

	/* building free storage here */

	/* module_path */
	tail = String2FreeStorage (&BaseSyntax, tail, config->module_path, BASE_MODULE_PATH_ID);

	/* icon_path */
	tail = String2FreeStorage (&BaseSyntax, tail, config->icon_path, BASE_ICON_PATH_ID);

	/* pixmap_path */
	tail = String2FreeStorage (&BaseSyntax, tail, config->pixmap_path, BASE_PIXMAP_PATH_ID);

	/* cursor_path */
	tail = String2FreeStorage (&BaseSyntax, tail, config->cursor_path, BASE_CURSOR_PATH_ID);

	/* audio_path */
	tail = String2FreeStorage (&BaseSyntax, tail, config->audio_path, BASE_AUDIO_PATH_ID);

	/* myname_path */
	tail = String2FreeStorage (&BaseSyntax, tail, config->myname_path, BASE_MYNAME_PATH_ID);

	/* desktop_size */
	tail = Geometry2FreeStorage (&BaseSyntax, tail, &(config->desktop_size), BASE_DESKTOP_SIZE_ID);

	/* desktop_scale */
    tail = Integer2FreeStorage (&BaseSyntax, tail, NULL, config->desktop_scale, BASE_DESKTOP_SCALE_ID);

	/* writing config into the file */
	WriteConfig (BaseConfigWriter, &Storage, CDT_Filename, (void **)&filename, flags);
	DestroyConfig (BaseConfigWriter);

	if (Storage)
	{
		fprintf (stderr, "\n%s:Config Writing warning: Not all Free Storage discarded! Trying again...", myname);
		DestroyFreeStorage (&Storage);
		fprintf (stderr, (Storage != NULL) ? " failed." : " success.");
	}
	return 0;
}


