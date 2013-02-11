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

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/
TermDef       AnimateResizeTerms[] = {
	{TF_NO_MYNAME_PREPENDING, "Twist", 5, TT_FLAG, ANIMATE_ResizeTwist_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "Flip", 4, TT_FLAG, ANIMATE_ResizeFlip_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "Turn", 4, TT_FLAG, ANIMATE_ResizeTurn_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "Zoom", 4, TT_FLAG, ANIMATE_ResizeZoom_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "Zoom3D", 6, TT_FLAG, ANIMATE_ResizeZoom3D_ID, NULL}
	,
	{TF_NO_MYNAME_PREPENDING, "Random", 6, TT_FLAG, ANIMATE_ResizeRandom_ID, NULL}
	,
	{0, NULL, 0, 0, 0}						   /* end of structure */
};

SyntaxDef     AnimateResizeSyntax = {
	'\0',
	'\n',
	AnimateResizeTerms,
	7,
	' ',
	"",
	"\t",
	"Module:Animate AnimateResize modes",
	"AnimateTypes",
	"animation modes used in the Animate module's config",
	NULL,
	0
};

TermDef       AnimateTerms[] = {
	{0, "Color", 5, TT_COLOR, ANIMATE_COLOR_ID, NULL}
	,
	{0, "Delay", 5, TT_INTEGER, ANIMATE_DELAY_ID, NULL}
	,
	{0, "Iterations", 10, TT_INTEGER, ANIMATE_ITERATIONS_ID, NULL}
	,
	{0, "Twist", 5, TT_INTEGER, ANIMATE_TWIST_ID, NULL}
	,
	{0, "Width", 5, TT_INTEGER, ANIMATE_WIDTH_ID, NULL}
	,
	{0, "Resize", 6, TT_FLAG, ANIMATE_RESIZE_ID, &AnimateResizeSyntax}
	,
	{0, NULL, 0, 0, 0}						   /* end of structure */

};

SyntaxDef     AnimateSyntax = {
	'\n',
	'\0',
	AnimateTerms,
	7,
	' ',
	"",
	"\t",
	"Module:Animate",
	"Animate",
	"AfterStep module animating windows being iconified/deiconified",
	NULL,
	0
};



AnimateConfig *
CreateAnimateConfig ()
{
	AnimateConfig *config = (AnimateConfig *) safecalloc (1, sizeof (AnimateConfig));

	config->iterations = ANIMATE_DEFAULT_ITERATIONS;
	config->delay = ANIMATE_DEFAULT_DELAY;
	return config;
}

void
DestroyAnimateConfig (AnimateConfig * config)
{
	if (config->color)
		free (config->color);
	DestroyFreeStorage (&(config->more_stuff));
	free (config);
}

AnimateConfig *
ParseAnimateOptions (const char *filename, char *myname)
{
	ConfigData    cd;
	ConfigDef    *AnimateConfigReader;
	AnimateConfig *config = CreateAnimateConfig ();

	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem    item;

	cd.filename = filename;
	AnimateConfigReader = InitConfigReader (myname, &AnimateSyntax, CDT_Filename, cd, NULL);

	if (!AnimateConfigReader)
		return config;

	item.memory = NULL;
	PrintConfigReader (AnimateConfigReader);
	ParseConfig (AnimateConfigReader, &Storage);

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
			 case ANIMATE_COLOR_ID:
				 config->color = item.data.string;
				 break;
			 case ANIMATE_DELAY_ID:
				 set_flags (config->set_flags, ANIMATE_SET_DELAY);
				 config->delay = (int)item.data.integer;
				 break;
			 case ANIMATE_ITERATIONS_ID:
				 set_flags (config->set_flags, ANIMATE_SET_ITERATIONS);
				 config->iterations = (int)item.data.integer;
				 break;
			 case ANIMATE_TWIST_ID:
				 config->twist = (int)item.data.integer;
				 set_flags (config->set_flags, ANIMATE_SET_TWIST);
				 break;
			 case ANIMATE_WIDTH_ID:
				 config->width = (int)item.data.integer;
				 set_flags (config->set_flags, ANIMATE_SET_WIDTH);
				 break;
			 case ANIMATE_RESIZE_ID:
				 if (pCurr->sub && pCurr->sub->term)
				 {
					 set_flags (config->set_flags, ANIMATE_SET_RESIZE);
					 config->resize = pCurr->sub->term->id - ANIMATE_RESIZE_ID_START;
				 }
				 break;
			 default:
				 item.ok_to_free = 1;
			}
		}
	}

	ReadConfigItem (&item, NULL);
	DestroyConfig (AnimateConfigReader);
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
WriteAnimateOptions (const char *filename, char *myname, AnimateConfig * config, unsigned long flags)
{
	ConfigDef    *AnimateConfigWriter = NULL;
	FreeStorageElem *Storage = NULL, **tail = &Storage;
	ConfigData    cd;

	if (config == NULL)
		return 1;

	cd.filename = filename;
	if ((AnimateConfigWriter = InitConfigWriter (myname, &AnimateSyntax, CDT_Filename, cd)) == NULL)
		return 2;

	CopyFreeStorage (&Storage, config->more_stuff);

	/* building free storage here */

	/* color */
	tail = String2FreeStorage (&AnimateSyntax, tail, config->color, ANIMATE_COLOR_ID);

	/* delay */
	tail = Integer2FreeStorage (&AnimateSyntax, tail, NULL, config->delay, ANIMATE_DELAY_ID);

	/* iterations */
	tail = Integer2FreeStorage (&AnimateSyntax, tail, NULL, config->iterations, ANIMATE_ITERATIONS_ID);

	/* twist */
	tail = Integer2FreeStorage (&AnimateSyntax, tail, NULL, config->twist, ANIMATE_TWIST_ID);

	/* width */
	tail = Integer2FreeStorage (&AnimateSyntax, tail, NULL, config->width, ANIMATE_WIDTH_ID);

	/* resize */
	if (config->resize < ANIMATE_RESIZE_ID_END - ANIMATE_RESIZE_ID_START)
	{
		FreeStorageElem **d_tail = tail;

		tail = Flag2FreeStorage (&AnimateSyntax, tail, ANIMATE_RESIZE_ID);
		if (*d_tail)
			Flag2FreeStorage (&AnimateResizeSyntax, &((*d_tail)->sub), config->resize + ANIMATE_RESIZE_ID_START);
	}

	/* writing config into the file */
	WriteConfig (AnimateConfigWriter, Storage, CDT_Filename, &cd, flags);
	DestroyFreeStorage (&Storage);
	DestroyConfig (AnimateConfigWriter);

	if (Storage)
	{
		show_error ("Config Writing warning: Not all Free Storage discarded! Trying again...");
		DestroyFreeStorage (&Storage);
		show_error ((Storage != NULL) ? " failed." : " success.");
	}
	return 0;
}
