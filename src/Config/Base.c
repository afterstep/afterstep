/*
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*#define LOCAL_DEBUG*/
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
	{0, "ModulePath", 10, TT_PATHNAME, BASE_MODULE_PATH_ID, NULL, NULL}
	,
	{0, "IconPath", 8, TT_PATHNAME, BASE_ICON_PATH_ID, NULL, NULL}
	,
	{0, "PixmapPath", 10, TT_PATHNAME, BASE_PIXMAP_PATH_ID, NULL, NULL}
	,
	{0, "Path", 4, TT_PATHNAME, BASE_MYNAME_PATH_ID, NULL, NULL}
	,
	{0, "DeskTopSize", 11, TT_GEOMETRY, BASE_DESKTOP_SIZE_ID, NULL, NULL}
	,
	{0, "DeskTopScale", 12, TT_INTEGER, BASE_DESKTOP_SCALE_ID, NULL, NULL}
	,
	{0, NULL, 0, 0, 0}
};

SyntaxDef     BaseSyntax = {
	'\n',
	'\0',
	BaseTerms,
	0,										   /* use default hash size */
	NULL
};

BaseConfig   *
CreateBaseConfig ()
{
	BaseConfig   *config = (BaseConfig *) safemalloc (sizeof (BaseConfig));

	/* let's initialize Base config with some nice values: */
	config->module_path = NULL;
	config->icon_path = NULL;
	config->pixmap_path = NULL;
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
	if (config->icon_path)
		free (config->icon_path);
	if (config->pixmap_path)
		free (config->pixmap_path);
	if (config->myname_path)
		free (config->myname_path);
	DestroyFreeStorage (&(config->more_stuff));
	free (config);
}

BaseConfig   *
ParseBaseOptions (const char *filename, char *myname)
{
	ConfigDef    *ConfigReader = InitConfigReader (myname, &BaseSyntax, CDT_Filename, (void *)filename, NULL);
	BaseConfig   *config = CreateBaseConfig ();
	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem    item;

	LOCAL_DEBUG_OUT ("filename=\"%s\", myname = \"%s\", ConfigReader=%p", filename, myname, ConfigReader);
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
		LOCAL_DEBUG_OUT ("term %d, keyword\"%s\"", pCurr->term->id, pCurr->term->keyword);
		if (!ReadConfigItem (&item, pCurr))
			continue;
		LOCAL_DEBUG_OUT ("int val %ld", item.data.integer);
		switch (pCurr->term->id)
		{
		 case BASE_MODULE_PATH_ID:
			 config->module_path = item.data.string;
			 break;
		 case BASE_ICON_PATH_ID:
			 config->icon_path = item.data.string;
			 break;
		 case BASE_PIXMAP_PATH_ID:
			 config->pixmap_path = item.data.string;
			 break;
		 case BASE_MYNAME_PATH_ID:
			 config->myname_path = item.data.string;
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
