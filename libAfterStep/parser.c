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

#undef LOCAL_DEBUG
#undef DO_CLOCKING
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
#include "../include/freestor.h"
#include "../include/functions.h"

char         *_disabled_keyword = DISABLED_KEYWORD;

void
BuildHash (SyntaxDef * syntax)
{
	register int  i;

	if (syntax->term_hash_size <= 0)
		syntax->term_hash_size = TERM_HASH_SIZE;
	if (syntax->term_hash == NULL)
		syntax->term_hash = create_ashash (syntax->term_hash_size, option_hash_value, option_compare, NULL);
	for (i = 0; syntax->terms[i].keyword; i++)
		add_hash_item (syntax->term_hash, (ASHashableValue) syntax->terms[i].keyword, (void *)&(syntax->terms[i]));
}

TermDef      *
FindStatementTerm (char *tline, SyntaxDef * syntax)
{
    TermDef *pterm = NULL;
    LOCAL_DEBUG_OUT( "looking for pterm in hash table  %p of the syntax %s ", syntax->term_hash, syntax->display_name );
    if (get_hash_item (syntax->term_hash, AS_HASHABLE(tline), (void**)&pterm)!=ASH_Success  )
		pterm = NULL;
    LOCAL_DEBUG_OUT( "FOUND pterm %p", syntax->term_hash );
    return pterm;
}

void
PrepareSyntax (SyntaxDef * syntax)
{
	if (syntax)
	{
		register int  i;

		if (syntax->recursion > 0)
			return;
		syntax->recursion++;

		BuildHash (syntax);
		for (i = 0; syntax->terms[i].keyword; i++)
			if (syntax->terms[i].sub_syntax)
				/* this should prevent us from endless recursion */
				if (syntax->terms[i].sub_syntax->term_hash == NULL)
					PrepareSyntax (syntax->terms[i].sub_syntax);

		syntax->recursion--;
	}
}

void
FreeSyntaxHash (SyntaxDef * syntax)
{
	if (syntax)
	{
		register int  i;

		if (syntax->recursion > 0)
			return;
		syntax->recursion++;

		if (syntax->term_hash)
			destroy_ashash (&(syntax->term_hash));

		for (i = 0; syntax->terms[i].keyword; i++)
			if (syntax->terms[i].sub_syntax)
				/* this should prevent us from endless recursion */
				if (syntax->terms[i].sub_syntax->term_hash != NULL)
					FreeSyntaxHash (syntax->terms[i].sub_syntax);

		syntax->recursion--;

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
	if (config->syntax && syntax->terminator == '\n')
	{										   /* handling prepending */
		SyntaxDef    *csyntax = config->syntax;
		int           len = strlen (csyntax->prepend_sub);
		register int  i;
		char         *tmp;

		if (config->current_prepend_allocated - config->current_prepend_size <= len)
		{
			config->current_prepend_allocated += len + 1;
			tmp = safemalloc (config->current_prepend_allocated);
			if (config->current_prepend)
			{
				strcpy (tmp, config->current_prepend);
				free (config->current_prepend);
			}
			config->current_prepend = tmp;
		}
		tmp = &(config->current_prepend[config->current_prepend_size]);

		for (i = 0; i <= len; i++)
			tmp[i] = csyntax->prepend_sub[i];

		config->current_prepend_size += len;

	}
#ifdef DEBUG_PARSER
	fprintf (stderr, "PushSyntax(%s, old is 0x%lX) current_prepend = [%s]\n",
			 syntax->display_name, (unsigned long)config->syntax, config->current_prepend);
#endif
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
#ifdef DEBUG_PARSER
		if (config->current_term)
			fprintf (stderr,
				 "PopSyntax(%s, flags = 0x%lX, term = [%s] ; old is 0x%lX)\n",
				 config->syntax->display_name, config->current_flags,
				 config->current_term->keyword, (unsigned long)(pold->syntax));
		else
			fprintf (stderr,
				 "PopSyntax(%s, flags = 0x%lX, term = [%s] ; old is 0x%lX)\n",
 				 config->syntax->display_name, config->current_flags,
 				 "", (unsigned long)(pold->syntax));
#endif
		if (pold->syntax->terminator == '\n')
		{
			if ((config->current_prepend_size -= strlen (config->syntax->prepend_sub)) >= 0)
				config->current_prepend[config->current_prepend_size] = '\0';
		}
/*fprintf( stderr, "PopSyntax(%s) current_prepend = [%s]\n", pold->syntax->display_name, config->current_prepend );
*/
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
	if (config->current_tail)
		if (config->current_tail->next)
		{
			StorageStack *pold = config->current_tail;

			config->current_tail = config->current_tail->next;
			free (pold);
			return 1;
		}
	return 0;
}


void
config_error (ConfigDef * config, char *error)
{
	if (config)
	{
		char         *eol = strchr (config->tline, '\n');

		if (eol)
			*eol = '\0';
		show_error ("in %s (line %d):%s[%.50s]",
				    config->current_syntax->syntax->display_name, config->line_count, error, config->tline);
		if (eol)
			*eol = '\n';
  	}
}

/* Creating and Initializing new ConfigDef */
ConfigDef    *
NewConfig (char *myname, SyntaxDef * syntax, ConfigDataType type, void *source, SpecialFunc special, int create)
{
	ConfigDef    *new_conf;

	if (myname == NULL)
		return NULL;

    new_conf = (ConfigDef *) safecalloc (1, sizeof (ConfigDef));
	new_conf->special = special;
	new_conf->fd = -1;
	new_conf->fp = NULL;
    new_conf->flags = 0;
	if (source)
		switch (type)
		{
		 case CDT_Filename:
			 {
				 char         *realfilename = put_file_home ((char *)source);

				 if (!realfilename)
				 {
					 free (new_conf);
					 return NULL;
				 }
				 new_conf->fd =
                     open (realfilename, create ? O_CREAT | O_RDONLY |O_BINARY: O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP|O_BINARY);
				 free (realfilename);
                 set_flags( new_conf->flags, CP_NeedToCloseFile);
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
         case CDT_FilePtrAndData :
             new_conf->fp = ((FilePtrAndData*)source)->fp;
			 new_conf->fd = fileno (new_conf->fp);
             set_flags( new_conf->flags, CP_ReadLines );
            break ;

		}

	if (new_conf->fd != -1 && new_conf->fp == NULL)
        new_conf->fp = fdopen (new_conf->fd, get_flags(new_conf->flags, CP_ReadLines)?"rt":"rb");

	new_conf->myname = mystrdup(myname);
	new_conf->current_syntax = NULL;
	new_conf->current_tail = NULL;
	new_conf->current_prepend = NULL;
	new_conf->current_prepend_size = 0;
	new_conf->current_prepend_allocated = 0;

	PushSyntax (new_conf, syntax);

	PrepareSyntax (syntax);

	/* allocated to store lines read from the file */
	new_conf->buffer = NULL;
	new_conf->buffer_size = 0;
	new_conf->bytes_in = 0;

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
	ConfigDef    *new_conf = NewConfig (myname, syntax, type, source, special, False);

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
		new_conf->buffer_size = strlen ((char *)source) + 1;
		new_conf->buffer = (char *)safemalloc (new_conf->buffer_size);
		strcpy (new_conf->buffer, (char *)source);
		new_conf->bytes_in = new_conf->buffer_size - 1;
    } else if( type == CDT_FilePtrAndData )
    {
        FilePtrAndData *fpd = (FilePtrAndData*)source;
        int buf_len = fpd->data?strlen( fpd->data ):0;
        if( buf_len < MAXLINELENGTH )
            buf_len = MAXLINELENGTH ;
        new_conf->buffer_size = buf_len ;
        new_conf->buffer = (char *)safemalloc (buf_len + 1);
        if( fpd->data )
            strcpy (new_conf->buffer, fpd->data);
        else
            new_conf->buffer[0] = '\0';
    }else
	{
		new_conf->buffer_size = MAXLINELENGTH + 1;
		new_conf->buffer = (char *)safemalloc (new_conf->buffer_size);
		new_conf->buffer[0] = '\0';
	}

	/* this is the current parsing information */
	new_conf->cursor = &(new_conf->buffer[0]);
	new_conf->line_count = 1;

	return new_conf;
}

/* debugging stuff */
#ifdef DEBUG_PARSER
void
PrintSyntax (SyntaxDef * syntax)
{
	if (syntax->recursion > 0)
		return;
	syntax->recursion++;

	fprintf (stderr, "\nSentence Terminator: [0x%2.2x]", syntax->terminator);
	fprintf (stderr, "\nConfig Terminator:   [0x%2.2x]", syntax->file_terminator);
	syntax->recursion--;
}


void
PrintConfigReader (ConfigDef * config)
{
	PrintSyntax (config->syntax);
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
	if (config->current_prepend)
		free (config->current_prepend);
	if (config->current_tail)
		free (config->current_tail);
	if (config->syntax)
		FreeSyntaxHash (config->syntax);
    if (get_flags(config->flags, CP_NeedToCloseFile) && config->fd != -1)
		close (config->fd);
	free (config);
}

void print_trimmed_str( char *prompt, char * str )
{
#ifdef LOCAL_DEBUG
    int i = 0;
    char tmp ;

    while( str[i] && i < 20 )
    {
        if( str[i] == '\n' )
            str[i] = '}';
        ++i ;
    }
    tmp = str[i] ;
    str[i] = '\0';
    LOCAL_DEBUG_OUT( "first %d chars of %s:\"%s\"", i, prompt, str );
    str[i] = tmp;
    while( --i >= 0 )
        if( str[i] == '}' )
            str[i] = '\n';
#endif
}

char         *
GetToNextLine (ConfigDef * config)
{
	register char terminator = config->syntax->terminator;
	register char file_terminator = config->syntax->file_terminator;
	register char *cur = config->cursor, *buffer_end = &(config->buffer[config->bytes_in]);

	for (; *cur != '\0' && cur < buffer_end; cur++)
	{
		if (*cur == '\n')
			config->line_count++;
		if (*cur == terminator)
			break;
		if (*cur == file_terminator)
			break;
	}
	if (cur < buffer_end && *cur != '\0')
		cur++;

	config->cursor = cur;

	if (cur >= buffer_end)
	{
        if (config->fp)
		{
            if( get_flags( config->flags, CP_ReadLines ) )
            {
LOCAL_DEBUG_OUT( "Reading Lines ...%s", "" );
                if (!fgets (config->buffer, config->buffer_size, config->fp))
                    return NULL;
                config->bytes_in = strlen(config->buffer);
                config->cursor = &(config->buffer[0]);
            }else
            {
                register int  i;
                register char *ptr = config->buffer ;

LOCAL_DEBUG_OUT( "Reading Buffer ...%s", "" );
                config->bytes_in = read (config->fd, ptr, config->buffer_size);
                if (config->bytes_in <= 0)
                    return NULL;
                print_trimmed_str( "new data begins with", ptr );
                /* now we want to get back to the last end-of-line
                so not to break statements in half */
                for (i = config->bytes_in - 1; i >= 0; i--)
                    if (ptr[i] == '\n')
                        break;
                i++;
                if (i > 0)
                {
                    lseek (config->fd, i - (config->bytes_in), SEEK_CUR);
                    config->bytes_in = i;
                }
                ptr[config->bytes_in] = '\0';
#ifdef __CYGWIN__                              /* fuck Microsoft !!!!! */
                while( --i >= 0 )
                    if( ptr[i] == 0x0D )
                        ptr[i] = 0x0A;
#endif
            }
            config->cursor = &(config->buffer[0]);
        } else
			return NULL;
	} else if (*cur == file_terminator)
		return NULL;

	return (config->cursor);
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
#ifdef __CYGWIN__
            if (!isspace ((int)*cur) && *cur != 0x0D)
#else
            if (!isspace ((int)*cur))
#endif
			{
				register int  i;

				config->current_flags = CF_NONE;

				if (*cur == COMMENTS_CHAR)
				{							   /* let's check for DISABLE keyword */
                    print_trimmed_str( "comments at", cur );
					for (i = 1; i < DISABLED_KEYWORD_SIZE; i++)
						if (*(cur + i) == '\0' || *(cur + i) != _disabled_keyword[i])
							break;
					if (i < DISABLED_KEYWORD_SIZE)
					{						   /* comments - skip entire line */
						while (*cur != '\n' && *cur != '\0')
							cur++;
						config->cursor = cur;
						break;
					}
					config->current_flags |= CF_DISABLED_OPTION;
					/* let's skip few spaces here */
					cur = cur + i;
					while (isspace ((int)*cur) && *cur != terminator)
						cur++;
					if (*cur == '\0' || *cur == terminator)
						break;				   /* not a valid option */
				}

				if (*cur == MYNAME_CHAR)
				{							   /* check if we have MyName here */
					register char *mname = config->myname;

                    print_trimmed_str(  "private option at", cur );
					while (*cur != '\0' && *mname != '\0')
					{
						if (tolower (*(mname++)) != tolower (*(++cur)))
							break;
					}
					if (*mname != '\0')
					{						   /* that was a foreign optiion - belongs to the other executable */
						if (my_only)
							break;
						config->current_flags |= CF_FOREIGN_OPTION;
					}
					cur++;
				} else
                {

                    if( *cur == terminator || *cur == file_terminator )   /* public option keyword may not be empty ! */
                        break;
                    config->current_flags |= CF_PUBLIC_OPTION;
                    print_trimmed_str( "public option at", cur );
                }
				config->tline = cur;		   /*that will be the begginnig of the term */

				/* now we should copy everything from after the first space to
				   config->current_data and set current_data_len ; */
                i = 0 ;
                while (cur[i] && !isspace ((int)cur[i]) && cur[i] != terminator && cur[i] != file_terminator)
                    ++i;
                while ((*cur) && isspace ((int)cur[i]) && cur[i] != terminator && cur[i] != file_terminator)
                    ++i;
                cur = &(cur[i]);           /* that will be the beginning of our data */
                config->tdata = cur;
                print_trimmed_str( "data start at :", cur );
				data = config->current_data;
                for (i = 0; cur[i] && cur[i] != terminator && cur[i] != file_terminator; ++i)
				{
					/* buffer overrun prevention */
					if (i >= config->current_data_size)
					{
						config->current_data_size += MAXLINELENGTH >> 3;
						config->current_data = (char *)realloc (config->current_data, config->current_data_size);
						if (config->current_data == NULL)
						{
							config_error (config, "Not enough memory to hold option's arguments");
							exit (0);
						}
                        data = config->current_data;
					}
                    data[i] = cur[i];
				}
                LOCAL_DEBUG_OUT( "%d bytes of data stored", i );
                cur = &(cur[i]);
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
                LOCAL_DEBUG_OUT( "%d bytes of clean data stored", i );
				config->current_data_len = i;
				config->current_data[i] = '\0';

				if (*cur && *cur == terminator)
					cur++;
				config->cursor = cur;		   /* Saving position for future use */
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
	register char *ptr;

	PushStorage (config, tail);
	if (config->syntax->terminator == syntax->file_terminator)
	{										   /* need to push back term's data into config buffer */
		config->cursor = config->tdata;
		if (config->current_term->flags & TF_NAMED_SUBCONFIG)
			/* we are supposed to skip single quoted text in here, or unquoted token */
		{
			if (*(ptr = config->cursor) == '"')
			{
				ptr = find_doublequotes (ptr);
				if (!ptr)
				{
					config_error (config, "Unmatched doublequotes detected\n");
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

/*  fprintf( stderr, "\nprocessing as subsyntax: [%s]", config->cursor );
*/
	PushSyntax (config, syntax);
}

void
ProcessStatement (ConfigDef * config)
{
	FreeStorageElem *pNext;
	TermDef      *pterm = config->current_term;
LOCAL_DEBUG_OUT( "checking for foreign option ...%s", "" );
	if (IsForeignOption (config))
		return;

LOCAL_DEBUG_OUT( "adding storage ...%s", "" );
	if ((pNext = AddFreeStorageElem (config->syntax, config->current_tail->tail, pterm, ID_ANY)) == NULL)
		return;

LOCAL_DEBUG_OUT( "parsing stuff ...%s", "" );
	pNext->flags = config->current_flags;

	if (config->current_data_len > 0 && !(pterm->flags & TF_DONT_REMOVE_COMMENTS))
	{
		stripcomments (config->current_data);
		config->current_data_len = strlen (config->current_data);
	}
    print_trimmed_str( "config->current_data", config->current_data );
    LOCAL_DEBUG_OUT( "curr_data_len = %d", config->current_data_len);

	args2FreeStorage (pNext, config->current_data, config->current_data_len);

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
				if (config->current_term->flags & TF_OBSOLETE)
					config_error (config, "Heh, It seems that I've encountered obsolete config option. I'll ignore it for now, Ok ?!");
  				if (config->current_term->flags & TF_SPECIAL_PROCESSING)
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

	return 1;
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

/***************************************************************************/
/***************************************************************************/
/*                   config Writing stuff                                  */
/***************************************************************************/
/* writer initialization */
ConfigDef    *
InitConfigWriter (char *myname, SyntaxDef * syntax, ConfigDataType type, void *source)
{
#ifdef WITH_CONFIG_WRITER
	ConfigDef    *new_conf = NewConfig (myname, syntax, type, source, NULL, True);

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
				{
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
					} else
					{						   /* file is empty, so continuing onto creating a new file */
						new_conf->buffer = safemalloc (1);
						new_conf->buffer[0] = '\0';
						new_conf->buffer_size = 1;
					}
				}
		}
		if (new_conf->buffer == NULL)
		{
			DestroyConfig (new_conf);
			return NULL;
		}
		new_conf->bytes_in = new_conf->buffer_size - 1;

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
		size_t        add_size = t_buffer->used + bytes_to_add + 2 - t_buffer->allocated;

		if (add_size < (t_buffer->allocated >> 3))
			add_size = (t_buffer->allocated >> 3);
		if (t_buffer->buffer != NULL && t_buffer->allocated > 0)
			t_buffer->buffer = realloc (t_buffer->buffer, t_buffer->allocated + add_size);
		else
			t_buffer->buffer = safemalloc (t_buffer->allocated + add_size);
		t_buffer->allocated += add_size;
	}
	memcpy (t_buffer->buffer + t_buffer->used, block_start, bytes_to_add);
	t_buffer->used += bytes_to_add;
}


char         *
terminate (char *ptr, char terminator)
{
	if (*ptr != terminator && (*ptr == '\0' && *(ptr - 1) != terminator))
		*(ptr++) = terminator;
	return ptr;
}

/*
 * creates string representing freeStorage's elem and writes it into the
 * output buffer
 */
static void
WriteFreeStorageElem (ConfigDef * config, struct WriteBuffer *t_buffer, FreeStorageElem * pElem, int level)
{
	static char  *elem_buf = NULL;
	static long   buf_allocated = 0;

	size_t        elem_buf_size = 0, params_size;
	register SyntaxDef *csyntax;
	register char *ptr;

	if (pElem == NULL || config->syntax == NULL)
	{
		if (elem_buf)
			free (elem_buf);
		elem_buf = NULL;
		buf_allocated = 0;
		return;
	}
	if (pElem->term == NULL)
		return;

	csyntax = config->syntax;
	/* now lets approximate our memory size needed to accomodate complete config line : */
	/* ( allowing some slack just for simplier algorithm, since few bytes would not really matter ) */
	/* first goes constants (denoted as C# below) : */
	elem_buf_size += DISABLED_KEYWORD_SIZE	   /* C1 */
		+ 1									   /* C2 */
		+ 1									   /* C3 */
		+ pElem->argc						   /* C4 */
		+ 1;								   /* C5 */
	/* now calculating variable lengths (denoted as V# below ): */
	if ((ptr = config->current_prepend) != NULL)	/* V1 */
		while (*(ptr++))
			elem_buf_size++;

	ptr = csyntax->prepend_one;				   /* V2 */
	while (*(ptr++))
		elem_buf_size++;

	if ((ptr = config->myname) != NULL)		   /* V3 */
		while (*(ptr++))
			elem_buf_size++;

	if ((ptr = pElem->term->keyword) != NULL)  /* V4 */
		elem_buf_size += pElem->term->keyword_len;

	params_size = GetStringArraySize (pElem->argc, pElem->argv);	/* V5 */
	elem_buf_size += params_size;

	if ((ptr = csyntax->prepend_sub) != NULL)  /* V6 */
		while (*(ptr++))
			elem_buf_size++;
	elem_buf_size++;

	if (elem_buf_size > 0)
	{
		register char *src;

		if (buf_allocated < elem_buf_size)
		{
			buf_allocated = elem_buf_size + (elem_buf_size >> 3);	/* to anticipate future buffer grows */
			if (elem_buf != NULL)
				free (elem_buf);
			elem_buf = (char *)safemalloc (buf_allocated);
		}
		/*below is what's actually is using our memory: */
		ptr = elem_buf;
		if (config->current_prepend)		   /*V1: config->current_prepend */
		{
			src = config->current_prepend;
			while (*src)
				*(ptr++) = *(src++);
		}

		src = csyntax->prepend_one;			   /*V2: csyntax->prepend_one */
		while (*src)
			*(ptr++) = *(src++);

		if (pElem->flags & CF_DISABLED_OPTION) /*C1: DISABLED_KEYWORD_SIZE */
		{
			src = _disabled_keyword;
			while (*src)
				*(ptr++) = *(src++);
			*(ptr++) = csyntax->token_separator;	/*C2: csyntax->token_separator */
		}

		if (!(pElem->term->flags & TF_NO_MYNAME_PREPENDING)
			&& !(pElem->flags & CF_PUBLIC_OPTION) && config->myname != NULL)
		{
			*(ptr++) = MYNAME_CHAR;			   /*C3: MYNAME_CHAR */
			src = config->myname;			   /*V3: config->myname */
			while (*src)
				*(ptr++) = *(src++);
		}
		if (pElem->term->keyword)			   /*V4: pElem->term->keyword */
		{
			src = pElem->term->keyword;
			while (*src)
				*(ptr++) = *(src++);
		}
		/* some beautification is in order here */
		if (params_size)
		{
			int           i;

			*(ptr++) = csyntax->token_separator;	/*C4: argc*csyntax->token_separator */
			for (i = 0; i < pElem->argc; i++)
				if ((src = pElem->argv[i]) != NULL)
				{
					if (i)
						*(ptr++) = csyntax->token_separator;
					while (*src)
						*(ptr++) = *(src++);   /*V5: argv[i] */
				}
		}
		*ptr = '\0';						   /* so that termination will work */
		if (pElem->sub == NULL || pElem->term->sub_syntax == NULL)	/*C5: csyntax->terminator */
			ptr = terminate (ptr, csyntax->terminator);
		else if (pElem->term->sub_syntax != NULL)
		{
			if (pElem->term->sub_syntax->terminator == csyntax->terminator)
				ptr = terminate (ptr, csyntax->terminator);
			else if (csyntax->terminator == '\0' && pElem->term->sub_syntax->terminator != '\0')
				ptr = terminate (ptr, pElem->term->sub_syntax->terminator);
		}

		if (*(ptr - 1) != csyntax->terminator) /* V6: min(csyntax->prepend_sub,1) */
		{									   /* we don't want for tokens to get joined together */
			src = csyntax->prepend_sub;
			if (src != NULL && *src != '\0')
				while (*src)
					*(ptr++) = *(src++);
			else
				ptr = terminate (ptr, csyntax->token_separator);
		}

		WriteBlock (t_buffer, elem_buf, ptr);
	}
	if (pElem->sub)
	{
		FreeStorageElem *psub;

		if (pElem->term->sub_syntax)
			PushSyntax (config, pElem->term->sub_syntax);
		for (psub = pElem->sub; psub; psub = psub->next)
			WriteFreeStorageElem (config, t_buffer, psub, level + 1);
		if (pElem->term->sub_syntax)
		{
			PopSyntax (config);
			if ((pElem->term->sub_syntax->file_terminator) != '\0')
			{
				*elem_buf = pElem->term->sub_syntax->file_terminator;
				WriteBlock (t_buffer, elem_buf, elem_buf + 1);
			}
		}
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
	{										   /* until not end of text */
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

		pterm = FindStatementTerm (config->tline, config->syntax);
		if (IsPublicOption (config))
		{
			int           write_it = 1;

			if (pterm)
				write_it = !(pterm->flags & TF_NO_MYNAME_PREPENDING);
			if (write_it)
			{
				if (!(flags & WF_DISCARD_PUBLIC))
					WriteBlock (t_buffer, config->tline_start, config->cursor);
				continue;
			}
		}

		/* find term */
		if (pterm)
		{
			pCurr = FindTermAndRemove (storage, pterm);
			if (pCurr)
				WriteFreeStorageElem (config, t_buffer, pCurr, 0);
		} else if (!(flags & WF_DISCARD_UNKNOWN))
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
WriteConfig (ConfigDef * config, FreeStorageElem ** storage,
			 ConfigDataType target_type, void **target, unsigned long flags)
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
     case CDT_FilePtrAndData:
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
	WriteFreeStorageElem (NULL, NULL, NULL, 0);	/* freeing statically allocated buffer */

	return t_buffer.used;
#else
	return 0;
#endif
}
