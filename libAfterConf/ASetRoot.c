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
#define LOCAL_DEBUG
#include "../configure.h"

#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/background.h"
#include "../libAfterStep/screen.h"
#include "../libAfterImage/afterimage.h"

#include "afterconf.h"

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the asetroot config
 * file
 *
 ****************************************************************************/

TermDef       MyBackgroundTerms[] = {
    {TF_NO_MYNAME_PREPENDING, "MyBackground", 12, TT_QUOTED_TEXT, BGR_MYBACKGROUND, NULL},
    {TF_NO_MYNAME_PREPENDING | TF_DONT_SPLIT | TF_DONT_REMOVE_COMMENTS |TF_INDEXED, "Use", 3, TT_OPTIONAL_PATHNAME, BGR_USE, NULL},
    {TF_NO_MYNAME_PREPENDING, "Cut", 3, TT_GEOMETRY, BGR_CUT, NULL},
    {TF_NO_MYNAME_PREPENDING | TF_DONT_REMOVE_COMMENTS, "Tint", 4, TT_COLOR, BGR_TINT, NULL},
    {TF_NO_MYNAME_PREPENDING, "Scale", 5, TT_GEOMETRY, BGR_SCALE, NULL},
    {TF_NO_MYNAME_PREPENDING, "Align", 5, TT_INTEGER, BGR_ALIGN, NULL},
    {TF_NO_MYNAME_PREPENDING | TF_INDEXED, "Pad", 3, TT_COLOR, BGR_PAD, NULL},
    {TF_NO_MYNAME_PREPENDING | TF_SYNTAX_TERMINATOR, "~MyBackground", 13,TT_FLAG, BGR_MYBACKGROUND_END, NULL},
    {0, NULL, 0, 0, 0}
};

SyntaxDef     MyBackgroundSyntax = {
	'\n',
	'\0',
	MyBackgroundTerms,
	0,										   /* use default hash size */
    ' ',
	"",
	"\t",
	"MyBackground definition",
	NULL,
    0
};


TermDef       ASetRootTerms[] = {
    {TF_INDEXED | TF_DONT_SPLIT | TF_NO_MYNAME_PREPENDING, "DeskBack", 8, TT_QUOTED_TEXT, BGR_DESK_BACK, NULL},
/* including MyStyles definitions processing */
	INCLUDE_MYSTYLE,
    {TF_NO_MYNAME_PREPENDING, "MyBackground", 12, TT_QUOTED_TEXT, BGR_MYBACKGROUND, &MyBackgroundSyntax},
	{0, NULL, 0, 0, 0}
};

SyntaxDef     ASetRootSyntax = {
	'\n',
	'\0',
	ASetRootTerms,
	0,										   /* use default hash size */
    ' ',
	"",
	"\t",
    "asetroot configuration",
	NULL,
    0
};

/*****************  Create/Destroy MyBackgroundConfig *****************/
MyBackgroundConfig *
CreateMyBackgroundConfig ()
{
	MyBackgroundConfig *config = (MyBackgroundConfig *) safemalloc (sizeof (MyBackgroundConfig));

	config->name = NULL;
	config->flags = 0;
	config->data = NULL;
    init_asgeometry (&(config->cut));
	config->tint = NULL;
	init_asgeometry (&(config->scale));
	config->pad = NULL;
	config->next = NULL;

	return config;
}

void
DestroyMyBackgroundConfig (MyBackgroundConfig ** head)
{
	if (head)
	{
		MyBackgroundConfig *cur = *head, *next;

		while (cur)
		{
			if (cur->name)
				free (cur->name);
			if (cur->data)
				free (cur->data);
			if (cur->tint)
				free (cur->tint);
			if (cur->pad)
				free (cur->pad);
			next = cur->next;
			free (cur);
			cur = next;
		}
		*head = NULL;
	}
}

/*****************  Create/Destroy DeskBackConfig     *****************/
DeskBackConfig *
CreateDeskBackConfig ()
{
	DeskBackConfig *config = (DeskBackConfig *) safemalloc (sizeof (DeskBackConfig));

	config->desk = -1;
	config->back_name = NULL;
	config->back = NULL;
	config->next = NULL;

	return config;
}

void
DestroyDeskBackConfig (DeskBackConfig ** head)
{
	if (head)
	{
		DeskBackConfig *cur = *head, *next;

		while (cur)
		{
			if (cur->back_name)
				free (cur->back_name);

			next = cur->next;
			free (cur);
			cur = next;
		}
		*head = NULL;
	}
}

/*****************  Create/Destroy ASetRootConfig     *****************/

ASetRootConfig *
CreateASetRootConfig ()
{
	ASetRootConfig *config = (ASetRootConfig *) safemalloc (sizeof (ASetRootConfig));

	/* let's initialize ASetRoot config with some nice values: */
	config->my_backs = NULL;
	config->my_desks = NULL;
	config->style_defs = NULL;

	config->more_stuff = NULL;

	return config;
}

void
DestroyASetRootConfig (ASetRootConfig * config)
{
	if (config->my_desks)
		DestroyDeskBackConfig (&(config->my_desks));
	if (config->my_backs)
		DestroyMyBackgroundConfig (&(config->my_backs));
	DestroyMyStyleDefinitions (&(config->style_defs));
	DestroyFreeStorage (&(config->more_stuff));
	free (config);
}

/***********************************************************************/
/*      Actual Parsing                                                 */
/***********************************************************************/

MyBackgroundConfig *
ParseMyBackgroundOptions (FreeStorageElem * Storage, char *myname)
{
	MyBackgroundConfig *config = CreateMyBackgroundConfig ();
	FreeStorageElem *pCurr;
	ConfigItem    item;

	item.memory = NULL;
	PrintFreeStorage (Storage);
	for (pCurr = Storage; pCurr; pCurr = pCurr->next)
	{
		if (pCurr->term == NULL)
			continue;

        LOCAL_DEBUG_OUT( "options(%p)->keyword(\"%s\")", pCurr, pCurr->term->keyword );

		if (pCurr->term->id == BGR_MYBACKGROUND_END)
		{
			config->flags |= BGFLAG_COMPLETE;
			continue;
		}
		if (!ReadConfigItem (&item, pCurr))
		{
			if (pCurr->term->id == BGR_SCALE)
				config->flags |= BGFLAG_SCALE;
			continue;
		}

		switch (pCurr->term->id)
		{
		 case BGR_MYBACKGROUND:
			 config->name = item.data.string;
			 break;
		 case BGR_USE:
			 config->data = item.data.string;
			 config->flags &= ~(BGFLAG_FILE | BGFLAG_MYSTYLE);
			 if (item.index == 0)
				 config->flags |= BGFLAG_FILE;
			 else if (item.index == 1)
				 config->flags |= BGFLAG_MYSTYLE;
			 break;
		 case BGR_CUT:
			 config->cut = item.data.geometry;
			 config->flags |= BGFLAG_CUT;
			 break;
		 case BGR_TINT:
			 config->tint = item.data.string;
			 config->flags |= BGFLAG_TINT;
			 break;
		 case BGR_SCALE:
			 config->scale = item.data.geometry;
			 config->flags |= BGFLAG_SCALE;
			 break;
		 case BGR_ALIGN:
			 config->flags &= ~(BGFLAG_ALIGN_CENTER | BGFLAG_ALIGN_RIGHT | BGFLAG_ALIGN_BOTTOM);
			 config->flags |= BGFLAG_ALIGN;
			 if (item.data.integer & 0x1)
				 config->flags |= BGFLAG_ALIGN_RIGHT;
			 if (item.data.integer & 0x2)
				 config->flags |= BGFLAG_ALIGN_BOTTOM;
			 item.ok_to_free = 1;
			 break;
		 case BGR_PAD:
			 config->pad = item.data.string;
			 config->flags &= ~(BGFLAG_PAD_VERT | BGFLAG_PAD_HOR);
			 config->flags |= BGFLAG_PAD;
			 if (item.index & 0x1)
				 config->flags |= BGFLAG_PAD_HOR;
			 if (item.index & 0x2)
				 config->flags |= BGFLAG_PAD_VERT;
			 break;
		 default:
			 item.ok_to_free = 1;
		}
	}
	ReadConfigItem (&item, NULL);

    set_flags( config->flags, BGFLAG_BAD);
    if (config->name == NULL)
        show_error("Background Definition error: name is empty !");
    else if (!(config->flags & BGFLAG_COMPLETE))
        show_error("Background Definition error: [%s] not terminated properly !", config->name);
    else
        clear_flags( config->flags, BGFLAG_BAD);

	if (config->flags & BGFLAG_BAD)
		DestroyMyBackgroundConfig (&config);
	return config;
}

DeskBackConfig *
ParseDeskBackOptions (ConfigItem * item, char *myname)
{
	DeskBackConfig *config = CreateDeskBackConfig ();
    Bool failed = True ;

	config->desk = item->index;
	config->back_name = item->data.string;
	item->ok_to_free = 0;

	if (config->desk < 0)
        show_error("Desk Background Definition error: bad desk number !");
    else if (config->back_name == NULL)
        show_error("Desk Background Definition error:  #%d has empty background name!", config->desk);
    else
        failed = False;

    if( failed )
		DestroyDeskBackConfig (&config);
    return config;
}

void
FixDeskBacks (ASetRootConfig * config)
{
	DeskBackConfig *curr_desk;

	if (!config)
		return;
	for (curr_desk = config->my_desks; curr_desk; curr_desk = curr_desk->next)
	{
		MyBackgroundConfig *curr_back;

		for (curr_back = config->my_backs; curr_back; curr_back = curr_back->next)
			if (mystrcasecmp (curr_desk->back_name, curr_back->name) == 0)
			{
				curr_desk->back = curr_back;
				break;
			}
		if (curr_desk->back == NULL)
		{									   /* need to create new back as plain image file,
											      using name from desk definition */
			curr_desk->back = CreateMyBackgroundConfig ();
			curr_desk->back->next = config->my_backs;
			config->my_backs = curr_desk->back;
			curr_desk->back->name = (char *)safemalloc (strlen (curr_desk->back_name) + 1);
			strcpy (curr_desk->back->name, curr_desk->back_name);
			curr_desk->back->data = (char *)safemalloc (strlen (curr_desk->back_name) + 1);
			strcpy (curr_desk->back->data, curr_desk->back_name);
			curr_desk->back->flags |= BGFLAG_FILE | BGFLAG_COMPLETE;
		}
	}
}

ASetRootConfig *
ParseASetRootOptions (const char *filename, char *myname)
{
	ConfigDef    *ConfigReader = InitConfigReader (myname, &ASetRootSyntax, CDT_Filename, (void *)filename, NULL);
	ASetRootConfig *config = CreateASetRootConfig ();
	MyBackgroundConfig **backs_tail = &(config->my_backs);
	DeskBackConfig **desks_tail = &(config->my_desks);
	MyStyleDefinition **styles_tail = &(config->style_defs);
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

		if (pCurr->term->id == BGR_MYBACKGROUND)
		{
			if ((*backs_tail = ParseMyBackgroundOptions (pCurr->sub, myname)) != NULL)
				backs_tail = &((*backs_tail)->next);
			continue;
		}

		if (!ReadConfigItem (&item, pCurr))
			continue;
		switch (pCurr->term->id)
		{
		 case BGR_DESK_BACK:
			 if ((*desks_tail = ParseDeskBackOptions (&item, myname)) != NULL)
				 desks_tail = &((*desks_tail)->next);
			 break;
		 case MYSTYLE_START_ID:
			 styles_tail = ProcessMyStyleOptions (pCurr->sub, styles_tail);
			 item.ok_to_free = 1;
			 break;
		 default:
			 item.ok_to_free = 1;
		}
	}
	ReadConfigItem (&item, NULL);
	FixDeskBacks (config);

	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);
	return config;
}

void
myback_parse (char *tline, FILE * fd, char **myname, int *mylook)
{
    FilePtrAndData fpd ;
    ConfigDef    *ConfigReader ;
    MyBackgroundConfig *back_config = NULL;
    FreeStorageElem *Storage = NULL, *more_stuff = NULL;
    MyLook *look = (MyLook*)mylook ;
    MyBackground *myback = NULL ;

    if( look == NULL )
        look = &(Scr.Look);

    fpd.fp = fd ;
    fpd.data = safemalloc( 12+1+strlen(tline)+1+1 ) ;
    sprintf( fpd.data, "MyBackground %s\n", tline );
LOCAL_DEBUG_OUT( "fd(%p)->tline(\"%s\")->fpd.data(\"%s\")", fd, tline, fpd.data );
    ConfigReader = InitConfigReader ((char*)myname, &MyBackgroundSyntax, CDT_FilePtrAndData, (void *)&fpd, NULL);
    free( fpd.data );

    if (!ConfigReader)
        return ;

	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &Storage);
	PrintFreeStorage (Storage);

	/* getting rid of all the crap first */
    StorageCleanUp (&Storage, &more_stuff, CF_DISABLED_OPTION);
    DestroyFreeStorage (&more_stuff);

    back_config = ParseMyBackgroundOptions (Storage, (char*)myname);

	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);

    if( back_config == NULL )
        return;

    /* now we have to turn MyBackgroundConfig into MyBackground structure :*/
    myback = create_myback( back_config->name );

    if( get_flags(back_config->flags, BGFLAG_FILE) )
        myback->type = MB_BackImage ;
    else if( get_flags(back_config->flags, BGFLAG_MYSTYLE ) )
        myback->type = MB_BackMyStyle ;
    else
        myback->type = MB_BackCmd ;

    myback->data = back_config->data ;
    back_config->data = NULL ;
    if( get_flags( back_config->flags, BGFLAG_CUT ) )
        myback->cut = back_config->cut ;
    if( get_flags( back_config->flags, BGFLAG_SCALE ) )
	{
        myback->scale = back_config->scale ;
		if( !get_flags( back_config->scale.flags, WidthValue ) )
			myback->scale.width = Scr.MyDisplayWidth ;
		if( !get_flags( back_config->scale.flags, HeightValue ) )
			myback->scale.height = Scr.MyDisplayHeight ;
		set_flags( myback->scale.flags, WidthValue|HeightValue );
	}

    myback->tint = TINT_LEAVE_SAME ;
    if( get_flags( back_config->flags, BGFLAG_TINT ) && back_config->tint )
        parse_argb_color( back_config->tint, &(myback->tint));
    myback->pad_color = ARGB32_Black ;
    if( get_flags( back_config->flags, BGFLAG_PAD ) && back_config->pad )
        parse_argb_color( back_config->pad, &(myback->pad_color));

    myback->align_flags = NO_ALIGN ;
    if( get_flags( back_config->flags, BGFLAG_PAD_HOR ) )
    {
        if( get_flags( back_config->flags, BGFLAG_ALIGN_RIGHT ) )
            myback->align_flags = ALIGN_RIGHT ;
        else if( get_flags( back_config->flags, BGFLAG_ALIGN_CENTER ) )
            myback->align_flags = ALIGN_HCENTER ;
        else
            myback->align_flags = ALIGN_LEFT ;
    }
    if( get_flags( back_config->flags, BGFLAG_PAD_VERT ) )
    {
        if( get_flags( back_config->flags, BGFLAG_ALIGN_BOTTOM ) )
            myback->align_flags |= ALIGN_BOTTOM ;
        else if( get_flags( back_config->flags, BGFLAG_ALIGN_CENTER ) )
            myback->align_flags |= ALIGN_VCENTER ;
        else
            myback->align_flags = ALIGN_TOP ;
    }
    LOCAL_DEBUG_OUT( "myback added: name(\"%s\")->type(%d)->data(\"%s\")", myback->name, myback->type, myback->data );
    add_myback(look, myback);

    /* final cleanup : */
    DestroyMyBackgroundConfig (&back_config);
}
