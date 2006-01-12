/*
 * Copyright (c) 2005 Sasha Vasko <sasha at aftercode.net>
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

#undef LOCAL_DEBUG
#undef DO_CLOCKING
#undef UNKNOWN_KEYWORD_WARNING

#include "../configure.h"

#include <fcntl.h>
#include <unistd.h>

#ifdef DO_CLOCKING
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#endif
#include "asapp.h"
#include "afterstep.h"
#include "functions.h"
#include "parser.h"
#include "../libAfterBase/xml.h"


/* AfterStep config into xml tree code : */

static xml_elem_t*
parse_config2xml_tree(ConfigDef *config_reader)
{
	xml_elem_t* tree = NULL ; 
#if 0  	                       /* old code  */
	int           TopLevel = 0;
	unsigned long flags;

	PushStorage (config, tail);
	/* get line */
	while (!TopLevel)
	{
		while (GetNextStatement (config, 1))
		{									   /* untill not end of text */
			flags = 0x00;
#ifdef DEBUG_PARSER
			fprintf (stderr, "\nSentence Found:[%.50s ...]\n,\tData=\t[%s]", config->tline, config->current_data);
			fprintf (stderr, "\nLooking for the Term...");
#endif
			/* find term */
			if ((config->current_term = FindStatementTerm (config->tline, config->syntax)))
			{
#ifdef DEBUG_PARSER
				fprintf (stderr, "\nTerm Found:[%s]", config->current_term->keyword);
#endif
				if (get_flags( config->current_term->flags, TF_OBSOLETE))
					config_error (config, "Heh, It seems that I've encountered obsolete config option. I'll ignore it for now, Ok ?!");
				if (get_flags( config->current_term->flags, TF_PHONY))
					set_flags( config->flags, CF_PHONY_OPTION );
  				if (get_flags( config->current_term->flags, TF_SPECIAL_PROCESSING))
				{
					if (config->special)
					{
						FreeStorageElem **ctail = config->current_tail->tail;

						flags = (*(config->special)) (config, ctail);
						if (get_flags (flags, SPECIAL_BREAK))
							break;
						if (get_flags (flags, SPECIAL_STORAGE_ADDED))
						{
							for (ctail = config->current_tail->tail; (*ctail); ctail = &((*ctail)->next));
							tail = config->current_tail->tail = ctail;
						}
					}
				}
				if (!get_flags (flags, SPECIAL_SKIP))
					ProcessStatement (config);

				if ((config->current_term->flags & TF_SYNTAX_TERMINATOR) || IsLastOption (config))
					break;
			} else
			{
#ifdef UNKNOWN_KEYWORD_WARNING
				config_error (config, " unknown keyword encountered");
#endif
                if (IsLastOption (config))
					break;
			}
		}									   /* end while( GetNextStatement() ) */
		/* trying to see if we can get to higher level syntax */
		if (!PopSyntax (config))
			TopLevel = 1;
		if (!PopStorage (config))
			TopLevel = 1;
		while ((TopLevel != 1) &&
			   ((config->current_term &&
				 (config->current_term->flags & TF_SYNTAX_TERMINATOR)) || IsLastOption (config)))
		{
			if (!PopSyntax (config))
				TopLevel = 1;
			if (!PopStorage (config))
				TopLevel = 1;
		}
	}										   /* end while( !TopLevel ) */
#endif
	return tree;
}

xml_elem_t*
file2xml_tree(const char *filename, char *myname, SyntaxDef *syntax, SpecialFunc special )
{
	ConfigData cd ;
	ConfigDef    *config_reader ;
	xml_elem_t *tree = NULL ;
	
	cd.filename = filename ;
	config_reader = InitConfigReader (myname, syntax, CDT_Filename, cd, special);
	if ( config_reader != NULL )
	{
		tree = parse_config2xml_tree(config_reader);
		DestroyConfig (config_reader);   
	}
	return tree;
}	 




