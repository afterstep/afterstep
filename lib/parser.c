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

/*#define DO_CLOCKING      */

#undef UNKNOWN_KEYWORD_WARNING

#include "../configure.h"

#include <fcntl.h>
#include <unistd.h>

#ifdef DO_CLOCKING
#include <time.h>
#endif

#include "../include/asapp.h"
#include "../include/afterstep.h"
#include "../include/parser.h"

char         *_disabled_keyword = DISABLED_KEYWORD;

void
PrepareSyntax (SyntaxDef * syntax)
{
	if (syntax)
	{
		register int  i;

		InitHash (syntax);
		BuildHash (syntax);
		for (i = 0; syntax->terms[i].keyword; i++)
			if (syntax->terms[i].sub_syntax)
				PrepareSyntax (syntax->terms[i].sub_syntax);
	}
}

void
FreeSyntaxHash (SyntaxDef * syntax)
{
	if (syntax)
	{
		register int  i;

		if (syntax->term_hash)
			free (syntax->term_hash);
		syntax->term_hash = NULL;

		for (i = 0; syntax->terms[i].keyword; i++)
			if (syntax->terms[i].sub_syntax)
				FreeSyntaxHash (syntax->terms[i].sub_syntax);
	}
}

/* Syntax and storage stack operations */
void
PushSyntax (ConfigDef * config, SyntaxDef * syntax)
{
	SyntaxStack  *pnew = (SyntaxStack *) safemalloc (sizeof (SyntaxStack));

	/* saving our status to be able to come back to it later */
	if (config->current_syntax)
	{
		config->current_syntax->current_term = config->current_term;
		config->current_syntax->current_flags = config->current_flags;
	}

	pnew->next = config->current_syntax;
	pnew->syntax = syntax;
	config->current_syntax = pnew;
	config->syntax = syntax;
}

int
PopSyntax (ConfigDef * config)
{
	if (config->current_syntax->next)
	{
		SyntaxStack  *pold = config->current_syntax;

		config->current_syntax = config->current_syntax->next;
		config->syntax = config->current_syntax->syntax;

		/* restoring our status */
		config->current_term = config->current_syntax->current_term;
		config->current_flags = config->current_syntax->current_flags;

		free (pold);
		return 1;
	}
	return 0;
}

void
PushStorage (ConfigDef * config, FreeStorageElem ** tail)
{
	StorageStack *pnew = (StorageStack *) safemalloc (sizeof (StorageStack));

	pnew->tail = tail;
	pnew->next = config->current_tail;
	config->current_tail = pnew;
}

int
PopStorage (ConfigDef * config)
{
    if ( config->current_tail && config->current_tail->next)
	{
		StorageStack *pold = config->current_tail;

		config->current_tail = config->current_tail->next;
		free (pold);
		return 1;
	}
	return 0;
}


/* Creating and Initializing new ConfigDef */
ConfigDef    *
NewConfig (char *myname, SyntaxDef * syntax, ConfigDataType type, void *source, SpecialFunc special)
{
	ConfigDef    *new_conf;

	if (myname == NULL)
		return NULL;

	new_conf = (ConfigDef *) safemalloc (sizeof (ConfigDef));
	new_conf->special = special;
	new_conf->fd = -1;
	new_conf->fp = NULL;
	new_conf->bNeedToCloseFile = 0;
	if (source)
		switch (type)
		{
		 case CDT_Filename:
			 {
				 char         *realfilename = PutHome ((char *)source);

				 if (!realfilename)
				 {
					 free (new_conf);
					 return NULL;
				 }
				 new_conf->fd = open (realfilename, O_RDONLY);
#ifdef DEBUG_PARSER
				 fprintf( stderr, "%s:%d > reading config from \"%s\"\n", __FILE__, __LINE__, realfilename );
#endif
				 free (realfilename);
				 new_conf->bNeedToCloseFile = 1;
			 }
			 break;
		 case CDT_FilePtr:
			 new_conf->fp = (FILE *) source;
			 new_conf->fd = fileno (new_conf->fp);
			 break;
		 case CDT_FileDesc:
			 new_conf->fd = *((int *)source);
			 break;
		 case CDT_Data:
			 break;
		}

	if (new_conf->fd != -1 && new_conf->fp == NULL)
		new_conf->fp = fdopen (new_conf->fd, "rt");

	new_conf->myname = (char *)safemalloc (strlen (myname) + 1);
	strcpy (new_conf->myname, myname);
	new_conf->current_syntax = NULL;
	new_conf->current_tail = NULL;
	PushSyntax (new_conf, syntax);

	PrepareSyntax (syntax);

	/* allocated to store lines read from the file */
	new_conf->buffer = NULL;
	new_conf->buffer_size = 0;

	/* this is the current parsing information */
	new_conf->tline = new_conf->tdata = new_conf->tline_start = NULL;
	new_conf->current_term = NULL;
	new_conf->current_data_size = MAXLINELENGTH + 1;
	new_conf->current_data = (char *)safemalloc (new_conf->current_data_size);
	new_conf->current_data_len = 0;
	new_conf->cursor = NULL;
	return new_conf;
}

/* reader initialization */
ConfigDef    *
InitConfigReader (char *myname, SyntaxDef * syntax, ConfigDataType type, void *source, SpecialFunc special)
{
	ConfigDef    *new_conf = NewConfig (myname, syntax, type, source, special);

	if (new_conf == NULL)
		return NULL;
	if (source == NULL)
	{
		DestroyConfig (new_conf);
		return NULL;
	}

	if (type == CDT_Data)
	{
		/* allocate to store entire data */
		new_conf->buffer = (char *)safemalloc (strlen ((char *)source) + 1);
		strcpy (new_conf->buffer, (char *)source);
	} else
	{
		new_conf->buffer = (char *)safemalloc (MAXLINELENGTH + 1);
		new_conf->buffer[0] = '\0';
	}

	/* this is the current parsing information */
	new_conf->cursor = &(new_conf->buffer[0]);

	return new_conf;
}

/* debugging stuff */
#ifdef DEBUG_PARSER
void
PrintSyntax (SyntaxDef * syntax)
{
	int           i;

	fprintf (stderr, "\nSentence Terminator: [0x%2.2x]", syntax->terminator);
	fprintf (stderr, "\nConfig Terminator:   [0x%2.2x]", syntax->file_terminator);
	fprintf (stderr, "\nTerm's hash table:");
	for (i = 0; i < syntax->term_hash_size; i++)
	{
		TermDef      *pterm = syntax->term_hash[i];

		if (pterm)
			fprintf (stderr, "\n  Hash value %d:", i);
		while (pterm)
		{
			fprintf (stderr, "\n\t\t[%lx][%d][%d][%s]", pterm->flags, pterm->type, pterm->id, pterm->keyword);
			if (pterm->sub_syntax)
			{
				fprintf (stderr, "\n SubSyntax :");
				PrintSyntax (pterm->sub_syntax);
			}
			pterm = pterm->brother;
		}
	}
	fprintf (stderr, "\nEnd of Term's hash table.");
}


void
PrintConfigReader (ConfigDef * config)
{
	PrintSyntax (config->syntax);
}

void
PrintFreeStorage (FreeStorageElem * storage)
{
	int           i;

	while (storage)
	{
		fprintf (stderr, "\nTerm's keyword: [%s]", storage->term->keyword);
		fprintf (stderr, "\nData:");
		for (i = 0; i < storage->argc; i++)
			fprintf (stderr, "\n  %d:\t[%s]", i, storage->argv[i]);
		PrintFreeStorage (storage->sub);
		storage = storage->next;
	}
}
#endif


/* reader de-initialization */
void
DestroyConfig (ConfigDef * config)
{
	free (config->myname);
	if (config->buffer)
		free (config->buffer);
	if (config->current_data)
		free (config->current_data);
	while (PopSyntax (config));
	if (config->current_syntax);
	free (config->current_syntax);
	while (PopStorage (config));
	if (config->current_tail)
		free (config->current_tail);
	if (config->syntax)
		FreeSyntaxHash (config->syntax);
	if (config->bNeedToCloseFile && config->fd != -1)
		close (config->fd);
	free (config);
}

char         *
GetToNextLine (ConfigDef * config)
{
	for (; *(config->cursor) != '\0' && *(config->cursor) != config->syntax->terminator; config->cursor++)
		if (*(config->cursor) == config->syntax->file_terminator)
			return NULL;

	if (*(config->cursor) != '\0')
		config->cursor++;
	if (*(config->cursor) == '\0')
	{
		if (config->fp)
		{
			if (!fgets (config->buffer, MAXLINELENGTH, config->fp))
				return NULL;
			config->cursor = &(config->buffer[0]);
		} else
			return NULL;
	}

	return (config->cursor);
}

void
config_error (ConfigDef * config, char *error)
{
	if (config)
	{
		char         *eol = strchr (config->tline, '\n');

		if (eol)
			*eol = '\0';
		show_error (":%s in [%.50s]", error, config->tline);
		if (eol)
			*eol = '\n';
	}
}



/* this function finds next valid statement:
   - not comments,
   - not an empty line,
   - if prepended with * - should have MyName at the beginning
   and :
   - sets tline to the beginning of the keyword (not MyName),
   - copies arguments into the current_data
   - sets current_data_len
   it will read data from the file as needed untill the end of file
   or config->syntax.file_terminator or EOF is reached
   Return: NULL if end of config reached, otherwise same as tline.
 */

char         *
GetNextStatement (ConfigDef * config, int my_only)
{
	char         *cur = config->cursor;		   /* don't forget to save it back ! */
	register char *data;
	char          terminator = config->syntax->terminator;
	char          file_terminator = config->syntax->file_terminator;

	while (1)
	{
		config->tline_start = cur;			   /* remember beginning of the entire config option */
		for (; *cur != '\0' && *cur != terminator; cur++)
		{
			if (*cur == file_terminator)
				return NULL;
			if (!isspace (*cur))
			{
				register int  i;
				config->current_flags = CF_NONE;

				if (*cur == COMMENTS_CHAR)
				{							   /* let's check for DISABLE keyword */
					for (i = 1; i < DISABLED_KEYWORD_SIZE; i++)
						if (cur[i] == '\0' || cur[i] != _disabled_keyword[i])
							break;
					if (i < DISABLED_KEYWORD_SIZE)
					{						   /* comments - skip entire line */
						i = 0 ;
						while (cur[i] != '\n' && cur[i] != '\0') ++i;
						config->cursor = &(cur[i]);
						break;
					}
					config->current_flags |= CF_DISABLED_OPTION;
					/* let's skip few spaces here */
					while( isspace (cur[i]) &&
						   cur[i] != terminator &&
					       cur[i] != file_terminator ) ++i;
					if ( cur[i] == '\0' ||
					     cur[i] == terminator ||
					     cur[i] == file_terminator)
						break;				   /* not a valid option */
					cur += i;
				}
#if 0
				if (*cur == MYNAME_CHAR)
				{							   /* check if we have MyName here */
					for (i = 0; *cur != '\0' && config->myname[i] != '\0'; i++)
						if (tolower (config->myname[i]) != tolower (*(++cur)))
							break;
					if (config->myname[i] != '\0')
					{						   /* that was a foreign optiion - belongs to the other executable */
						if (my_only)
							break;
						config->current_flags |= CF_FOREIGN_OPTION;
					}
					cur++;
				} else
					config->current_flags |= CF_PUBLIC_OPTION;

				config->tline = cur;		   /*that will be the begginnig of the term */

				/* now we should copy everything from after the first space to
				   config->current_data and set current_data_len ; */
				for (; !isspace (*cur) && *cur != config->syntax->terminator && (*cur); cur++);
				for (; isspace (*cur) && *cur && *cur != config->syntax->terminator; cur++);
				config->tdata = cur;		   /* that will be the beginning of our data */
				for (i = 0; *(cur + i) && *(cur + i) != config->syntax->terminator; i++)
				{
					/* buffer overrun prevention */
					if (i >= config->current_data_size)
					{
						char         *new_data;
						register int  k = i;

						config->current_data_size += MAXLINELENGTH >> 3;
						new_data = (char *)safemalloc (config->current_data_size);
						for (; k; k--)
							new_data[k] = config->current_data[k];
						free (config->current_data);
						config->current_data = new_data;
					}
					config->current_data[i] = *(cur + i);
				}
				/* now let's go back and remove trailing spaces */
				{
					int           i_saved = i;
					for (i--; i >= 0; i--)
						if (!isspace (config->current_data[i]))
							break;
					i++;
					config->current_data_len = i;
					config->current_data[i] = '\0';
					i = i_saved;
				}
				if (*(cur + i) && *(cur + i) == config->syntax->terminator)
					i++;
				config->cursor = cur + i;	   /* Saving position for future use */
#else
				if (*cur == MYNAME_CHAR)
				{							   /* check if we have MyName here */
					register char *mname = config->myname;
					++cur ;
					i = 0 ;
					while (cur[i] != '\0' && mname[i] != '\0')
					{
						if (tolower (mname[i]) != tolower (cur[i]))
							break;
						++i ;
					}
					if (mname[i] != '\0')
					{						   /* that was a foreign optiion - belongs to the other executable */
						if (my_only)
							break;
						config->current_flags |= CF_FOREIGN_OPTION;
					}
					cur += i;
				} else
					config->current_flags |= CF_PUBLIC_OPTION;

				config->tline = cur;		   /*that will be the begginnig of the term */

				/* now we should copy everything from after the first space to
				   config->current_data and set current_data_len ; */
				i = 0 ;
				while ( cur[i] && !isspace ((int)cur[i]) &&
				        cur[i] != terminator && cur[i] != file_terminator) ++i;
				while ( cur[i] && isspace ((int)cur[i]) &&
				        cur[i] != terminator && cur[i] != file_terminator) ++i;

				config->tdata = cur = &(cur[i]); /* that will be the beginning of our data */
				data = config->current_data;

				for (i = 0; cur[i] && cur[i] != terminator && cur[i] != file_terminator; i++)
				{
					/* buffer overrun prevention */
					if (i >= config->current_data_size)
					{
						config->current_data_size += MAXLINELENGTH >> 3;
						config->current_data = realloc (config->current_data, config->current_data_size);
						if (config->current_data == NULL)
						{
							config_error (config, "Not enough memory to hold option's arguments");
							exit (0);
						}
						data = config->current_data;
					}
					data[i] = cur[i];
				}
				cur += i ;
				/* now let's go back and remove trailing spaces */
				if (config->tdata[0] == file_terminator)
					config->current_flags |= CF_LAST_OPTION;
				else
				{
					for (i--; i >= 0; i--)
					{
						if (config->tdata[i] == file_terminator)
							config->current_flags |= CF_LAST_OPTION;
						if (!isspace ((int)data[i]))
							break;
					}
					i++;
				}
				/* since \0 is our normal data terminator we really should trigger
				   end of the configuration when the file ends : */
				if (file_terminator == '\0')
					config->current_flags &= ~CF_LAST_OPTION;
				config->current_data_len = i;
				config->current_data[i] = '\0';

				if (*cur && *cur == terminator)
					cur++;
				config->cursor = cur;		   /* Saving position for future use */
#endif
				return config->tline;
			}
		}
		/* reading file here */
		if ((cur = GetToNextLine (config)) == NULL)
			return NULL;
	}

	return NULL;
}

void
ProcessSubSyntax (ConfigDef * config, FreeStorageElem ** tail, SyntaxDef * syntax)
{
	PushStorage (config, tail);

	if (config->syntax->terminator == syntax->file_terminator)
	{										   /* need to push back term's data into config buffer */
		config->cursor = config->tdata;
		if (config->current_term->flags & TF_NAMED_SUBCONFIG)
			/* we are supposed to skip single quoted text in here, or unquoted token */
		{
			register char *ptr = config->cursor;

			if (ptr[0] == '"')
			{
				ptr = find_doublequotes (ptr);
				if (!ptr)
				{
					config_error (config, "Unmatched doublequotes detected");
					ptr = config->cursor;
				} else
					ptr++;					   /* skipping current doubleqoute */
			} else							   /*simply skipping to the next space */
				for (; *ptr && !isspace ((int)*ptr); ptr++);
			config->cursor = ptr;
		}
	} else if (config->syntax->terminator == syntax->terminator)
	{										   /* need to push back entire term's line into config buffer */
		config->cursor = config->tline_start;
	}
	PushSyntax (config, syntax);
}

/* create and format freestorage elem from the statement found */
void
ProcessStatement (ConfigDef * config)
{
	FreeStorageElem *pNext;
	TermDef      *pterm = config->current_term;

	if (IsForeignOption (config))
		return;

	if ((pNext = AddFreeStorageElem (config->syntax, config->current_tail->tail, pterm, ID_ANY)) == NULL)
		return;

	if (config->current_data_len <= 0)
	{
		pNext->argv = NULL;
		pNext->argc = 0;
	} else
	{
		int           i, count;
		char         *cur;

		pNext->flags = config->current_flags;

		if (!(pterm->flags & TF_DONT_SPLIT))
		{
			cur = config->current_data;
			for (pNext->argc = 0; *cur;)
			{
				for (count = 0; !isspace (*cur) && *cur; count++)
					cur++;
				while (isspace (*cur) && *cur)
					cur++;
				if (count)
					pNext->argc++;
			}
		} else if (pterm->flags & TF_INDEXED)
			pNext->argc = 2;
		else
			pNext->argc = 1;

		if (pNext->argc == 0)
			pNext->argv = NULL;
		else
		{
			pNext->argv = CreateStringArray (pNext->argc);
			pNext->argv[0] = (char *)safemalloc (config->current_data_len + 1);

			cur = config->current_data;
			i = 0;
			if (pterm->flags & TF_DONT_SPLIT)
			{
				if (pterm->flags & TF_INDEXED)
				{							   /* first token should be index thou */
					for (count = 0; !isspace (*cur) && *cur; count++)
						pNext->argv[0][count] = *(cur++);
					while (isspace (*cur) && *cur)
						cur++;
					pNext->argv[0][count] = '\0';
					i++;
					pNext->argv[i] = &(pNext->argv[0][count + 1]);
				}
				/* the rest of the text is treated as single token */
				strcpy (pNext->argv[i], cur);
			} else
			{
				for (; i < pNext->argc; i++)
				{
					for (count = 0; !isspace (*cur) && *cur; count++)
						pNext->argv[i][count] = *(cur++);
					while (isspace (*cur) && *cur)
						cur++;
					if (!count)
						break;

					pNext->argv[i][count] = '\0';
					if ((i + 1) < pNext->argc)
						pNext->argv[i + 1] = &(pNext->argv[i][count + 1]);
				}
			}
		}
	}
	config->current_tail->tail = &(pNext->next);
	if (pterm->sub_syntax)
		ProcessSubSyntax (config, &(pNext->sub), pterm->sub_syntax);
	return;
}



/* main parsing procedure */
int
ParseConfig (ConfigDef * config, FreeStorageElem ** tail)
{
	int           TopLevel = 0;

	PushStorage (config, tail);
	/* get line */
	while (!TopLevel)
	{
		while (GetNextStatement (config, 1))
		{									   /* untill not end of text */
#ifdef DEBUG_PARSER
	        fprintf (stderr, "\nSentence Found:[%s]\n,\tData=\t[%s]", config->tline, config->current_data);
    		fprintf (stderr, "\nLooking for the Term(syn=%p)...", config->syntax );
#endif
			/* find term */
			if ((config->current_term = FindStatementTerm (config->tline, config->syntax)))
			{
#ifdef DEBUG_PARSER
				fprintf (stderr, "\nTerm Found:[%s]", config->current_term->keyword);
#endif
				if (config->current_term->flags & TF_SPECIAL_PROCESSING)
				{
					if (config->special)
					{
						FreeStorageElem **ctail = config->current_tail->tail;

						if (!(*(config->special)) (config, ctail))
							break;
						for (; (*ctail); tail = &((*ctail)->next));
						config->current_tail->tail = ctail;
					}
				} else
					ProcessStatement (config);
				if ((config->current_term->flags & TF_SYNTAX_TERMINATOR)|| IsLastOption (config) )
					break;
			}else
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

	return 1;
}

/*********************************************************************************************/
/*                                     FreeStorage management                                */
/*********************************************************************************************/
/* Create new FreeStorage Elem and add it to the supplied storage's tail */
FreeStorageElem *
AddFreeStorageElem (SyntaxDef * syntax, FreeStorageElem ** tail, TermDef * pterm, int id)
{
	FreeStorageElem *new_elem = NULL;

	if (pterm == NULL)
		pterm = FindTerm (syntax, TT_ANY, id);

	if (pterm)
	{
		new_elem = (FreeStorageElem *) safemalloc (sizeof (FreeStorageElem));
		new_elem->term = pterm;
		new_elem->argc = 0;
		new_elem->argv = NULL;
		new_elem->flags = 0;
		new_elem->next = *tail;
		new_elem->sub = NULL;
		*tail = new_elem;
	}
	return new_elem;
}

/* Duplicate existing FreeStorage Elem */
FreeStorageElem *
DupFreeStorageElem (FreeStorageElem * source)
{
	FreeStorageElem *new_elem = NULL;

	if (source)
	{
		new_elem = (FreeStorageElem *) safemalloc (sizeof (FreeStorageElem));
		new_elem->term = source->term;
		new_elem->argc = source->argc;
		/* duplicating argv here */
		new_elem->argv = DupStringArray (source->argc, source->argv);
		new_elem->flags = source->flags;
		new_elem->next = NULL;
		new_elem->sub = NULL;
		if (new_elem->sub)
		{
			FreeStorageElem *psub, **pnew_sub = &(new_elem->sub);

			for (psub = new_elem->sub; psub; psub = psub->next)
			{
				*pnew_sub = DupFreeStorageElem (psub);
				pnew_sub = &((*pnew_sub)->next);
			}
		}
	}
	return new_elem;
}

void
ReverseFreeStorageOrder (FreeStorageElem ** storage)
{
	FreeStorageElem *pNewHead = NULL, *pNext;

	for (; *storage; *storage = pNext)
	{
		pNext = (*storage)->next;
		(*storage)->next = pNewHead;
		pNewHead = *storage;
	}
	*storage = pNewHead;
}

void
CopyFreeStorage (FreeStorageElem ** to, FreeStorageElem * from)
{
	FreeStorageElem *pNew;

	if (to == NULL)
		return;
	for (; from; from = from->next)
	{
		if ((pNew = DupFreeStorageElem (from)) == NULL)
			continue;
		pNew->next = *to;
		*to = pNew;
	}
}

int
CountFreeStorageElems (FreeStorageElem * storage)
{
	int           count = 0;

	for (; storage; storage = storage->next)
		count++;
	return count;
}

/* this one will scan list of FreeStorage elements and will move all elements with
   specifyed flags mask into the garbadge_bin
 */

void
StorageCleanUp (FreeStorageElem ** storage, FreeStorageElem ** garbadge_bin, unsigned long mask)
{
	FreeStorageElem **ppCurr, *pToRem;

	for (ppCurr = storage; *ppCurr; ppCurr = &((*ppCurr)->next))
	{
		while ((*ppCurr)->flags & mask)
		{
			pToRem = *ppCurr;
			*ppCurr = pToRem->next;
			pToRem->next = *garbadge_bin;
			*garbadge_bin = pToRem;
		}
		if ((*ppCurr)->sub)
			StorageCleanUp (&((*ppCurr)->sub), garbadge_bin, mask);
	}
}

/* memory deallocation */
void
DestroyFreeStorage (FreeStorageElem ** storage)
{
	if (storage)
		if (*storage)
		{
			DestroyFreeStorage (&((*storage)->next));
			DestroyFreeStorage (&((*storage)->sub));
			/* that will deallocate everything */
			if ((*storage)->argc && (*storage)->argv)
			{
				if ((*storage)->argv[0])
#ifdef DEBUG_PARSER
				{
					fprintf (stderr, "\n DestroyFreeStorage: deallocating [%s].", (*storage)->argv[0]);
#endif
					free ((*storage)->argv[0]);
#ifdef DEBUG_PARSER
				} else
					fprintf (stderr, "\n DestroyFreeStorage: no data to deallocate.");
#endif
				free ((*storage)->argv);
			}
			free (*storage);
			*storage = NULL;
		}
}

int
ReadFlagItem (unsigned long *set_flags, unsigned long *flags, FreeStorageElem * stored, flag_options_xref * xref)
{
	if (stored && xref)
	{
		Bool          value = True;

		if (stored->term->type != TT_FLAG || get_flags (stored->term->flags, TF_INDEXED))
			return 0;

		if (stored->argc > 1)
			value = (atol (stored->argv[1]) > 0);

		while (xref->flag != 0)
		{
			if (xref->id_on == stored->term->id)
				break;
			if (xref->id_off == stored->term->id)
			{
				value = !value;
				break;
			}
			xref++;
		}
		if (xref->flag != 0)
		{
			if (set_flags)
				set_flags (*set_flags, xref->flag);
			if (flags)
			{
				if (value)
					set_flags (*flags, xref->flag);
				else
					clear_flags (*flags, xref->flag);
			}
		}
		return 1;
	}
	return 0;
}


/* free storage post processing code */
int
ReadConfigItem (ConfigItem * item, FreeStorageElem * stored)
{
	if (item->memory && item->ok_to_free)
		free (item->memory);
	item->memory = NULL;

	if (stored)
	{
		int           pos = 0;

		item->memory = NULL;
		item->ok_to_free = 0;
		if (stored->argv == NULL || stored->argc == 0)
			return 0;
		if (stored->argv[pos] == NULL)
			return 0;
		if (stored->term->flags & TF_INDEXED)
		{
			item->index = atoi (stored->argv[pos++]);
			if (stored->argc < 2)
				return 0;
			if (!stored->argv[pos])
				return 0;
		}

		switch (stored->term->type)
		{
		 case TT_INTEGER:
			 item->data.integer = atol (stored->argv[pos++]);
			 break;
		 case TT_COLOR:
		 case TT_FONT:
		 case TT_FILENAME:
		 case TT_TEXT:
		 case TT_PATHNAME:
			 item->memory = safemalloc (strlen (stored->argv[pos]) + 1);
			 item->data.string = (char *)(item->memory);
			 strcpy (item->data.string, stored->argv[pos++]);
			 break;
		 case TT_QUOTED_TEXT:
			 {
				 register int  i = 1;
				 int           len = strlen (stored->argv[pos]);
				 char         *ptr = stored->argv[pos];

				 for (; *ptr != '\0'; ptr++)
					 if (*ptr == '"')
						 break;
				 for (; *(ptr + i) != '\0'; i++)
				 {
					 if (*(ptr + i) == '"')
						 break;
					 if (*(ptr + i) == '\\')
						 if (*(ptr + (++i)) == '\0')
							 break;
				 }
				 if (i < 2 || *ptr != '"')
				 {
					 fprintf (stderr, "\n%s: bad quoted string [%s]. Ignoring!", MyName, stored->argv[pos]);
					 return 0;
				 }
				 if (*(ptr + i) == '\0')
				 {
					 fprintf (stderr, "\n%s: terminating quote missing in [%s]. Ignoring!", MyName, stored->argv[pos]);
					 return 0;
				 }
				 len = i;
				 item->memory = safemalloc (len);
				 item->data.string = (char *)(item->memory);
				 len--;
				 for (ptr++, i = 0; i < len; i++)
					 item->data.string[i] = *(ptr + i);
				 item->data.string[i] = '\0';
				 pos++;
			 }
			 break;
		 case TT_GEOMETRY:
			 item->data.geometry.flags =
				 XParseGeometry (stored->argv[pos++],
								 &(item->data.geometry.x),
								 &(item->data.geometry.y), &(item->data.geometry.width), &(item->data.geometry.height));
			 break;
		}
		return 1;
	}
	return 0;
}

/****************************************************************************************/
/*                        Assorted utility functions                                    */
/****************************************************************************************/
/* service functions */
void
FlushConfigBuffer (ConfigDef * config)
{
	config->buffer[0] = '\0';
	config->cursor = &(config->buffer[0]);
}

char        **
CreateStringArray (size_t elem_num)
{
	char        **array = (char **)safemalloc (elem_num * sizeof (char *));
	int           i;

	for (i = 0; i < elem_num; i++)
		array[i] = NULL;
	return array;
}

size_t
GetStringArraySize (int argc, char **argv)
{
	size_t        size = 0;

	for (argc--; argc >= 0; argc--)
		if (argv[argc])
			size += strlen (argv[argc]) + 1;
	return size;
}

char        **
DupStringArray (int argc, char **argv)
{
	int           i;
	size_t        data_size;
	char        **array = CreateStringArray (argc);

	data_size = GetStringArraySize (argc, argv);
	if (data_size && array)
	{
		array[0] = (char *)safemalloc (data_size);
		for (i = 0; i < data_size; i++)
			array[0][i] = argv[0][i];
		for (i = 1; i < argc; i++)
			if (argv[i])
				array[i] = array[0] + (argv[i] - argv[0]);
	}
	return array;
}

void
AddStringToArray (int *argc, char ***argv, char *new_string)
{
	int           i;
	size_t        data_size;
	char        **array;

	if (new_string == NULL)
		return;

	array = CreateStringArray (*argc + 1);
	data_size = GetStringArraySize (*argc, *argv);
	if (data_size && array)
	{
		array[0] = (char *)safemalloc (data_size + strlen (new_string) + 1);
		for (i = 0; i < data_size; i++)
			array[0][i] = (*argv)[0][i];
		for (i = 1; i < *argc; i++)
			if ((*argv)[i])
				array[i] = array[0] + ((*argv)[i] - (*argv)[0]);
		array[i] = array[0] + data_size;
		strcpy (array[i], new_string);
		(*argc)++;
	}
}

/* end StringArray functionality */


void
init_asgeometry (ASGeometry * geometry)
{
	geometry->flags = XValue | YValue;
    geometry->x = geometry->y = 0;
    geometry->width = geometry->height = 1;
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/*                   config Writing stuff                                  */
/***************************************************************************/
/* writer initialization */
ConfigDef    *
InitConfigWriter (char *myname, SyntaxDef * syntax, ConfigDataType type, void *source)
{
#ifdef WITH_CONFIG_WRITER
	ConfigDef    *new_conf = NewConfig (myname, syntax, type, source, NULL);

	if (new_conf == NULL)
		return NULL;
	if (source != NULL)
	{
		if (type == CDT_Data)
		{
			if ((new_conf->buffer_size = strlen (source)) > 0)
			{
				new_conf->buffer = (char *)safemalloc (++(new_conf->buffer_size));
				strcpy (new_conf->buffer, (char *)source);
			}
		} else
		{									   /* reading entire file in memory here */
			struct stat   file_stats;

			if (new_conf->fd != -1)
				if (fstat (new_conf->fd, &file_stats) == 0)
					if ((new_conf->buffer_size = file_stats.st_size) > 0)
					{
						int           bytes_read;

						new_conf->buffer = (char *)safemalloc (++new_conf->buffer_size);
						if ((bytes_read = read (new_conf->fd, new_conf->buffer, file_stats.st_size)) > 0)
							new_conf->buffer[bytes_read] = '\0';
						else
						{
							free (new_conf->buffer);
							new_conf->buffer = NULL;
							new_conf->buffer_size = 0;
						}
					}
		}
		if (new_conf->buffer == NULL)
		{
			DestroyConfig (new_conf);
			return NULL;
		}
		new_conf->cursor = &(new_conf->buffer[0]);
	}

	return new_conf;
#else
	return NULL;
#endif
}

/***************************************************************************/
/* functions used in writing config */
#ifdef WITH_CONFIG_WRITER
/* Finds element in FreeStorage that represents this particular term
 * it then removes it from the list of elems and returns pointer to it
 */
FreeStorageElem *
FindTermAndRemove (FreeStorageElem ** storage, TermDef * pterm)
{
	FreeStorageElem **ppCurr = storage, *pRes;

	if (storage == NULL)
		return NULL;
	for (; *ppCurr != NULL; ppCurr = &((*ppCurr)->next))
		if ((*ppCurr)->term == pterm)
			break;

	pRes = *ppCurr;
	if (pRes != NULL)
	{
		*ppCurr = pRes->next;
		pRes->next = NULL;
	}

	return pRes;
}

/* writes block of text in to the output buffer, enlarging it if needed.
 * it starts with block_start and end with block_end or '\0'
 * if block_end is NULL
 */
struct WriteBuffer
{
	char         *buffer;
	size_t        allocated, used;
};

void
WriteBlock (struct WriteBuffer *t_buffer, char *block_start, char *block_end)
{
	size_t        bytes_to_add;

	if (t_buffer == NULL || block_start == NULL)
		return;
	bytes_to_add = (block_end == NULL) ? strlen (block_start) : block_end - block_start;
	if (bytes_to_add <= 0)
		return;
	/*make sure we always have 1 byte left at the end ( for trailing 0 ) */
	/*                     and 1 byte left for file terminator           */
	if (t_buffer->allocated < t_buffer->used + bytes_to_add + 2)
	{										   /* growing buffer by 1/8 of it's size */
		char         *tmp;
		size_t        add_size = t_buffer->used + bytes_to_add + 2 - t_buffer->allocated;

		if (add_size < (t_buffer->allocated >> 3))
			add_size = (t_buffer->allocated >> 3);
		tmp = (char *)safemalloc (t_buffer->allocated + add_size);
		memcpy (tmp, t_buffer->buffer, t_buffer->used);
		free (t_buffer->buffer);
		t_buffer->buffer = tmp;
		t_buffer->allocated += add_size;
	}
	memcpy (t_buffer->buffer + t_buffer->used, block_start, bytes_to_add);
	t_buffer->used += bytes_to_add;
}

/*
 * creates string representing freeStorage's elem and writes it into the
 * output buffer
 */
void
WriteFreeStorageElem (ConfigDef * config, struct WriteBuffer *t_buffer, FreeStorageElem * pElem, int level)
{
	char         *elem_buf;
	size_t        elem_buf_size = level, params_size;
	int           i;

	if (pElem == NULL)
		return;
	if (pElem->term == NULL)
		return;

	if (config->syntax->terminator != '\n')
		elem_buf_size = 0;

	if (pElem->flags & CF_DISABLED_OPTION)
		elem_buf_size += 1 + DISABLED_KEYWORD_SIZE;

	if (!(pElem->term->flags & TF_NO_MYNAME_PREPENDING) && !(pElem->flags & CF_PUBLIC_OPTION))
		elem_buf_size += 1 + strlen (config->myname);
	if (pElem->term->keyword)
		elem_buf_size += pElem->term->keyword_len;

	elem_buf_size++;						   /* for separating space */
	params_size = GetStringArraySize (pElem->argc, pElem->argv);
	elem_buf_size += params_size;


	elem_buf_size++;						   /* for sentense terminator */
	elem_buf_size++;						   /* for terminating '\0'    */

	if (elem_buf_size > 0)
	{
		char         *ptr;

		elem_buf = (char *)safemalloc (elem_buf_size);
		i = 0;
		if (config->syntax->terminator == '\n')
			for (; i < level; i++)
				elem_buf[i] = '\t';

		ptr = &(elem_buf[i]);
		if (pElem->flags & CF_DISABLED_OPTION)
		{
			strcpy (ptr, _disabled_keyword);
			ptr += DISABLED_KEYWORD_SIZE;
			*(ptr++) = ' ';
		}

		if (!(pElem->term->flags & TF_NO_MYNAME_PREPENDING) && !(pElem->flags & CF_PUBLIC_OPTION))
		{
			*(ptr++) = MYNAME_CHAR;
			strcpy (ptr, config->myname);
			ptr += strlen (config->myname);
		}
		if (pElem->term->keyword)
		{
			strcpy (ptr, pElem->term->keyword);
			ptr += strlen (pElem->term->keyword);
		}
		/* some beautification is in order here */
		i = 0;
		if (params_size)
		{
			*(ptr++) = '\t';
			for (; i < pElem->argc; i++)
				if (pElem->argv[i])
				{
					if (i)
						*(ptr++) = ' ';
					strcpy (ptr, pElem->argv[i]);
					ptr += strlen (pElem->argv[i]);
				}
		}

		if (*ptr != config->syntax->terminator && (*ptr == '\0' && *(ptr - 1) != config->syntax->terminator))
			*(ptr++) = config->syntax->terminator;
		*ptr = '\0';
		WriteBlock (t_buffer, elem_buf, NULL);
		free (elem_buf);
	}
	if (pElem->sub)
	{
		FreeStorageElem *psub;

		for (psub = pElem->sub; psub; psub = psub->next)
			WriteFreeStorageElem (config, t_buffer, psub, level + 1);
	}
}

void
WriteRemnants (ConfigDef * config, struct WriteBuffer *t_buffer, FreeStorageElem * pCurr)
{
/* > this function is writing options that are acually new for this config */
/* > as the result we want to write them into the begining of the file */
/* > so lets save current buffer contents, write remnants and then write buffer back */
/* I've changed that back to have later options override previous options */
/*                                                      Sasha
 */
/*  struct WriteBuffer rem_buffer ;
 *  int remnants_num = CountFreeStorageElems( pCurr );
 *
 *    if( remnants_num<=0 ) return ;
 *    rem_buffer.allocated = remnants_num*NORMAL_CONFIG_LINE+t_buffer->used+2 ;
 *    rem_buffer.buffer = (char*)safemalloc( rem_buffer.allocated );
 *    rem_buffer.used = 0 ;
 */
	for (; pCurr != NULL; pCurr = pCurr->next)
	{
		if (pCurr->term == NULL)
			continue;
/*      WriteFreeStorageElem( config, &rem_buffer, pCurr );
 */ WriteFreeStorageElem (config, t_buffer, pCurr, 0);
	}

/*    WriteBlock( &rem_buffer, t_buffer->buffer, t_buffer->buffer+t_buffer->used );
 *    free( t_buffer->buffer );
 *    t_buffer->allocated = rem_buffer.allocated ;
 *    t_buffer->used = rem_buffer.used ;
 *    t_buffer->buffer = rem_buffer.buffer ;
 */
}

void
ScanAndWriteExistant (ConfigDef * config, FreeStorageElem ** storage, struct WriteBuffer *t_buffer, unsigned long flags)
{
	char         *noise_start = config->cursor;
	FreeStorageElem *pCurr;

	while (GetNextStatement (config, 0))
	{										   /* untill not end of text */
		TermDef      *pterm;

		if (!(flags & WF_DISCARD_COMMENTS))
		{
			WriteBlock (t_buffer, noise_start, config->tline_start);
			noise_start = config->cursor;
		}
		if (IsForeignOption (config))
		{
			if (!(flags & WF_DISCARD_FOREIGN))
				WriteBlock (t_buffer, config->tline_start, config->cursor);
			continue;
		}
		if (IsPublicOption (config))
		{
			if (!(flags & WF_DISCARD_PUBLIC))
				WriteBlock (t_buffer, config->tline_start, config->cursor);
			continue;
		}

		/* find term */
		if ((pterm = FindStatementTerm (config->tline, config->syntax)))
		{
			pCurr = FindTermAndRemove (storage, pterm);
			if (pCurr)
				WriteFreeStorageElem (config, t_buffer, pCurr, 0);
		}
		if ((pterm == NULL || pCurr == NULL) && (!(flags & WF_DISCARD_UNKNOWN)))
			WriteBlock (t_buffer, config->tline_start, config->cursor);

		DestroyFreeStorage (&pCurr);		   /* no longer need it */
	}

	if (!(flags & WF_DISCARD_COMMENTS))
		WriteBlock (t_buffer, noise_start, config->cursor);

}

#endif
/***************************************************************************/
/* main writing procedure ( returns size of the data written )             */

long
WriteConfig (ConfigDef * config, FreeStorageElem ** storage, ConfigDataType target_type, void **target,
			 unsigned long flags)
{
#ifdef WITH_CONFIG_WRITER
	struct WriteBuffer t_buffer;
	int           t_fd = -1;

	if (config == NULL || storage == NULL || target == NULL)
		return -1;
	if (*storage == NULL)
		return -1;

	if (config->buffer_size <= 0)
	{
		t_buffer.allocated = MAXLINELENGTH;
		flags = WF_DISCARD_EVERYTHING;
	} else
		t_buffer.allocated = config->buffer_size;

	t_buffer.buffer = (char *)safemalloc (t_buffer.allocated);
	t_buffer.used = 0;
	/* scanning old file first to preserv comments */
	if (!(flags & WF_DISCARD_COMMENTS) ||
		!(flags & WF_DISCARD_UNKNOWN) || !(flags & WF_DISCARD_FOREIGN) || !(flags & WF_DISCARD_PUBLIC))
		ScanAndWriteExistant (config, storage, &t_buffer, flags);
	/* now writing remaining elements */
	WriteRemnants (config, &t_buffer, *storage);
	DestroyFreeStorage (storage);

	t_buffer.buffer[t_buffer.used] = config->syntax->file_terminator;
	t_buffer.buffer[t_buffer.used + 1] = '\0';

	if (config->syntax->file_terminator != '\0')
		t_buffer.used++;

	/* now saving buffer into file if we need to */
	switch (target_type)
	{
	 case CDT_Filename:
		 t_fd = open ((char *)(*target), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		 break;
	 case CDT_FilePtr:
		 t_fd = fileno ((FILE *) (*target));
		 break;
	 case CDT_FileDesc:
		 t_fd = *((int *)(*target));
		 break;
	 case CDT_Data:
		 (*target) = (void *)(t_buffer.buffer);
		 return t_buffer.used;
	}
	if (t_fd != -1)
	{
		write (t_fd, t_buffer.buffer, t_buffer.used);
		if (target_type == CDT_Filename)
			close (t_fd);
	}

	free (t_buffer.buffer);

	return t_buffer.used;
#else
	return 0;
#endif
}

/* helper functions for writing config */

char         *
MyIntToString (int value)
{
	int           charnum, test = value;
	char         *string;

	for (charnum = (test < 0) ? 1 : 0; test; charnum++)
		test /= 10;
	if (charnum == 0)
		charnum++;
	charnum++;
	string = (char *)safemalloc (charnum);
	sprintf (string, "%d", value);
	return string;
}

FreeStorageElem **
Integer2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, int value, int id)
{
	FreeStorageElem *new_elem = AddFreeStorageElem (syntax, tail, NULL, id);

	if (new_elem)
	{
		new_elem->argc = 1;
		new_elem->argv = (char **)safemalloc (sizeof (char *));
		new_elem->argv[0] = MyIntToString (value);
		tail = &(new_elem->next);
	}
	return tail;
}

FreeStorageElem **
String2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, char *string, int id)
{
	FreeStorageElem *new_elem;

	if (string)
		if (strlen (string))
			if ((new_elem = AddFreeStorageElem (syntax, tail, NULL, id)) != NULL)
			{
				new_elem->argc = 1;
				new_elem->argv = (char **)safemalloc (sizeof (char *));
				new_elem->argv[0] = (char *)safemalloc (strlen (string) + 1);
				strcpy (new_elem->argv[0], string);
				tail = &(new_elem->next);
			}
	return tail;
}

FreeStorageElem **
Geometry2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, ASGeometry * geometry, int id)
{
	char          geom_string[MAXLINELENGTH] = "";

	if (geometry)
	{
		if (geometry->flags & WidthValue)
			sprintf (geom_string, "=%d", geometry->width);
		if (geometry->flags & HeightValue)
			sprintf (geom_string + strlen (geom_string), "x%d", geometry->height);
		if (geometry->flags & XValue)
			sprintf (geom_string + strlen (geom_string), "%+d", geometry->x);
		if (geometry->flags & YValue)
			sprintf (geom_string + strlen (geom_string), "%+d", geometry->y);
		tail = String2FreeStorage (syntax, tail, geom_string, id);
	}
	return tail;
}

FreeStorageElem **
StringArray2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, char **strings, int index1, int index2, int id)
{
	FreeStorageElem *new_elem = NULL;
	int           i;
	char          ind_str[MAXLINELENGTH];
	TermDef      *pterm = FindTerm (syntax, TT_ANY, id);

	if (strings && pterm)
		for (i = 0; i < index2 - index1 + 1; i++)
		{
			if (strings[i] == NULL)
				continue;
			if ((new_elem = AddFreeStorageElem (syntax, tail, pterm, id)) == NULL)
				continue;
			new_elem->argc = 2;
			new_elem->argv = CreateStringArray (2);
			sprintf (ind_str, "%d", i + index1);
			new_elem->argv[0] = (char *)safemalloc (strlen (ind_str) + 1 + strlen (strings[i]) + 1);
			strcpy (new_elem->argv[0], ind_str);
			new_elem->argv[1] = new_elem->argv[0] + strlen (ind_str) + 1;
			strcpy (new_elem->argv[1], strings[i]);
			tail = &(new_elem->next);
		}
	return tail;
}
