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
#include "../../configure.h"

#include "../../include/asapp.h"
#include "../../include/afterstep.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the base.<bpp> config
 * file
 *
 ****************************************************************************/

TermDef       BaseTerms[] = {
    {TF_NO_MYNAME_PREPENDING, "ModulePath", 10,     TT_PATHNAME, BASE_MODULE_PATH_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "AudioPath", 9,       TT_PATHNAME, BASE_AUDIO_PATH_ID , NULL},
    {TF_NO_MYNAME_PREPENDING, "IconPath", 8,        TT_PATHNAME, BASE_ICON_PATH_ID  , NULL},
    {TF_NO_MYNAME_PREPENDING, "PixmapPath", 10,     TT_PATHNAME, BASE_PIXMAP_PATH_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "FontPath", 8,        TT_PATHNAME, BASE_FONT_PATH_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "CursorPath", 10,     TT_PATHNAME, BASE_CURSOR_PATH_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "Path", 4,            TT_PATHNAME, BASE_MYNAME_PATH_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "DeskTopSize", 11,    TT_GEOMETRY, BASE_DESKTOP_SIZE_ID   , NULL},
    {TF_NO_MYNAME_PREPENDING, "DeskTopScale", 12,   TT_INTEGER,  BASE_DESKTOP_SCALE_ID  , NULL},
	{0, NULL, 0, 0, 0}
};

SyntaxDef     BaseSyntax = {
	'\n',
	'\0',
	BaseTerms,
	0,										   /* use default hash size */
	' ',
	"",
	"\t",
	"Base configuration",
	NULL,
	0
};

BaseConfig   *
CreateBaseConfig ()
{
	BaseConfig   *config = (BaseConfig *) safemalloc (sizeof (BaseConfig));

	/* let's initialize Base config with some nice values: */
	config->module_path = NULL;
	config->audio_path = NULL;
	config->icon_path = NULL;
	config->pixmap_path = NULL;
	config->font_path = NULL;
	config->cursor_path = NULL;
	config->myname_path = NULL;
	config->desktop_size.flags = WidthValue | HeightValue;
	config->desktop_size.width = config->desktop_size.height = 1;
	config->desktop_size.x = config->desktop_size.y = 0;
	config->desktop_scale = 32;

	config->more_stuff = NULL;

	return config;
}

void
DestroyBaseConfig (BaseConfig * config)
{
	if (config->module_path)
		free (config->module_path);
	if (config->audio_path)
		free (config->audio_path);
	if (config->icon_path)
		free (config->icon_path);
	if (config->pixmap_path)
		free (config->pixmap_path);
	if (config->font_path)
		free (config->font_path);
	if (config->cursor_path)
		free (config->cursor_path);
	if (config->myname_path)
		free (config->myname_path);
	DestroyFreeStorage (&(config->more_stuff));
	free (config);
}

BaseConfig   *
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

void
ExtractPath (BaseConfig * config,
			 char **module_path,
			 char **audio_path,
			 char **icon_path,
			 char **pixmap_path,
			 char **font_path,
			 char **cursor_path,
			 char **myname_path)
{
	register char *tmp ;
	if (config)
	{
		if (module_path)
		{
			tmp = copy_replace_envvar (config->module_path);
			set_string_value(module_path, tmp, NULL, 0 );
		}
		if (audio_path)
		{
			tmp = copy_replace_envvar (config->audio_path);
			set_string_value(audio_path, tmp, NULL, 0 );
		}
		if (icon_path)
		{
			tmp = copy_replace_envvar (config->icon_path);
			set_string_value(icon_path, tmp, NULL, 0 );
		}
		if (pixmap_path)
		{
			tmp = copy_replace_envvar (config->pixmap_path);
			set_string_value(pixmap_path, tmp, NULL, 0 );
		}
		if (font_path)
		{
			tmp = copy_replace_envvar (config->font_path);
			set_string_value(font_path, tmp, NULL, 0 );
		}
		if (cursor_path)
		{
			tmp = copy_replace_envvar (config->cursor_path);
			set_string_value(cursor_path, tmp, NULL, 0 );
		}
		if (myname_path)
		{
			tmp = copy_replace_envvar (config->myname_path);
			set_string_value(myname_path, tmp, NULL, 0 );
		}
	}
}

#if 0
void
BaseConfig2ASEnvironment( register BaseConfig *config, ASEnvironment **penv )
{
	register ASEnvironment *env = *penv;
	if( env == NULL )
		env = safecalloc( 1, sizeof( ASEnvironment ) );
	ExtractPath (config, &(env->module_path),
		            	&(env->audio_path),
						&(env->icon_path),
						&(env->pixmap_path),
						&(env->font_path),
						&(env->cursor_path),
						&(env->myname_path));
	env->desk_pages_h = config->desktop_size.width ;
	env->desk_pages_v = config->desktop_size.height ;
	*penv = env ;
}
#endif

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
