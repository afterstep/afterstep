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

#include "../configure.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/mystyle.h"
#include "../libAfterStep/parser.h"

#include "afterconf.h"

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/

TermDef       IdentTerms[] = {
/* including MyStyles definitions processing */
	INCLUDE_MYSTYLE,

	{0, NULL, 0, 0, 0}						   /* end of structure */

};

SyntaxDef     IdentSyntax = {
	'\n',
	'\0',
	IdentTerms,
	0,										   /* use default hash size */
	' ',
	"",
	"\t",
	"Module:Ident",
	"Ident",
	"AfterStep module displaying properties of selected window",
	NULL,
	0
};



IdentConfig  *
CreateIdentConfig ()
{
	IdentConfig  *config = (IdentConfig *) safecalloc (1, sizeof (IdentConfig));

	/* let's initialize Ident's config with some nice values: */

    config->style_defs = NULL;
    config->more_stuff = NULL;

	return config;
}

void
DestroyIdentConfig (IdentConfig * config)
{
    DestroyMyStyleDefinitions (&(config->style_defs));
	DestroyFreeStorage (&(config->more_stuff));
	free (config);
}

IdentConfig  *
ParseIdentOptions (const char *filename, char *myname)
{
	ConfigData    cd ;
	ConfigDef    *IdentConfigReader ; 
	IdentConfig  *config = CreateIdentConfig ();
	FreeStorageElem *Storage = NULL;

	cd.filename = filename ;
	IdentConfigReader = InitConfigReader (myname, &IdentSyntax, CDT_Filename, cd, NULL);
	if (!IdentConfigReader)
		return config;

	PrintConfigReader (IdentConfigReader);
	ParseConfig (IdentConfigReader, &Storage);

	/* getting rid of all the crap first */
	StorageCleanUp (&Storage, &(config->more_stuff), CF_DISABLED_OPTION);

	config->style_defs = free_storage2MyStyleDefinitionsList (Storage);

	DestroyConfig (IdentConfigReader);
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
WriteIdentOptions (const char *filename, char *myname, IdentConfig * config, unsigned long flags)
{
	ConfigDef    *IdentConfigWriter = NULL;
	FreeStorageElem *Storage = NULL, **tail = &Storage;
	ConfigData cd ;

	if (config == NULL)
		return 1;
	cd.filename = filename ;
	if ((IdentConfigWriter = InitConfigWriter (myname, &IdentSyntax, CDT_Filename, cd)) == NULL)
		return 2;

	CopyFreeStorage (&Storage, config->more_stuff);
    if (config->style_defs)
        tail = MyStyleDefs2FreeStorage (&IdentSyntax, tail, config->style_defs);

	/* building free storage here */
	/* writing config into the file */
	cd.filename = filename ;
	WriteConfig (IdentConfigWriter, &Storage, CDT_Filename, &cd, flags);
	DestroyConfig (IdentConfigWriter);

	if (Storage)
	{
		fprintf (stderr, "\n%s:Config Writing warning: Not all Free Storage discarded! Trying again...", myname);
		DestroyFreeStorage (&Storage);
		fprintf (stderr, (Storage != NULL) ? " failed." : " success.");
	}
	return 0;
}
