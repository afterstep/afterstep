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

#include "afterconf.h"


TermDef       CleanTerms[] = {
	{TF_INDEXED, "", 0, TT_FLAG, CLEAN_Clean_ID, &DummyFuncSyntax},
	{0, NULL, 0, 0, 0}						   /* end of structure */

};

SyntaxDef     CleanSyntax = {
	'\n',
	'\0',
	CleanTerms,
	7,
	' ',
	"",
	"\t",
	"Module:Clean",
	"Clean",
	"AfterStep module cleaning up desktop from unused windows",
	NULL,
	0
};


CleanConfig *
CreateCleanConfig ()
{
	CleanConfig *config = (CleanConfig *) safecalloc (1, sizeof (CleanConfig));
	return config;
}

void
DestroyCleanConfig (CleanConfig * config)
{
	DestroyFreeStorage (&(config->more_stuff));
	free (config);
}

CleanConfig *
ParseCleanOptions (const char *filename, char *myname)
{
	ConfigData cd ;
	ConfigDef    *CleanConfigReader;
	CleanConfig *config = CreateCleanConfig ();

	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem    item;

	cd.filename = filename ;
	CleanConfigReader = InitConfigReader (myname, &CleanSyntax, CDT_Filename, cd, NULL);

	if (!CleanConfigReader)
		return config;

	item.memory = NULL;
	PrintConfigReader (CleanConfigReader);
	ParseConfig (CleanConfigReader, &Storage);

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
			 case CLEAN_Clean_ID:
				 break;
			 default:
				 item.ok_to_free = 1;
			}
		}
	}

	ReadConfigItem (&item, NULL);
	DestroyConfig (CleanConfigReader);
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
WriteCleanOptions (const char *filename, char *myname, CleanConfig * config, unsigned long flags)
{
	ConfigDef    *CleanConfigWriter = NULL;
	FreeStorageElem *Storage = NULL/*, **tail = &Storage*/;
	ConfigData cd ;

	if (config == NULL)
		return 1;

	cd.filename = filename ;
	if ((CleanConfigWriter = InitConfigWriter (myname, &CleanSyntax, CDT_Filename, cd)) == NULL)
		return 2;

	CopyFreeStorage (&Storage, config->more_stuff);

	/* building free storage here */

	/* color */
	/* tail = String2FreeStorage (&CleanSyntax, tail, config->color, CLEAN_Clean_ID); */

	/* writing config into the file */
	WriteConfig (CleanConfigWriter, Storage, CDT_Filename, &cd, flags);
	DestroyFreeStorage (&Storage);
	DestroyConfig (CleanConfigWriter);

	if (Storage)
	{
		show_error ("Config Writing warning: Not all Free Storage discarded! Trying again...");
		DestroyFreeStorage (&Storage);
		show_error ((Storage != NULL) ? " failed." : " success.");
	}
	return 0;
}
