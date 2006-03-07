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
#include "parser.h"
#include "freestor.h"
#include "functions.h"
#include "../libAfterBase/xml.h"

char         *_disabled_keyword = DISABLED_KEYWORD;
char         *_unknown_keyword = "unknown";

static ASHashTable *Keyword2IDHash = NULL ; 


void 
register_keyword_id( const char *keyword, int id ) 
{
	ASHashData hdata = {0};

	if( Keyword2IDHash == NULL )
		Keyword2IDHash = create_ashash( 0, NULL, NULL, NULL );

	hdata.cptr = (char*)keyword ; 
	add_hash_item( Keyword2IDHash, AS_HASHABLE(id), hdata.vptr );
}	   

const char*
keyword_id2keyword( int id )	
{
	ASHashData hdata = {0};

	if( Keyword2IDHash == NULL )	
		return _unknown_keyword;
	
	if( get_hash_item(Keyword2IDHash, AS_HASHABLE(id), &hdata.vptr) == ASH_Success ) 
		return hdata.cptr;
	
	return _unknown_keyword;
}

void
BuildHash (SyntaxDef * syntax)
{
	register int  i;

	if (syntax->term_hash_size <= 0)
		syntax->term_hash_size = TERM_HASH_SIZE;
	if (syntax->term_hash == NULL)
		syntax->term_hash = create_ashash (syntax->term_hash_size, option_hash_value, option_compare, NULL);
	for (i = 0; syntax->terms[i].keyword; i++)
	{	
		add_hash_item (syntax->term_hash, AS_HASHABLE(syntax->terms[i].keyword), (void *)&(syntax->terms[i]));
		register_keyword_id( syntax->terms[i].keyword, syntax->terms[i].id );
	}
}

void
PrepareSyntax (SyntaxDef * syntax)
{
	if (syntax)
	{
		register int  i;

 		LOCAL_DEBUG_OUT( "syntax = \"%s\", recursion = %d", syntax->display_name, syntax->recursion ); 		
		if (syntax->recursion > 0)
			return;
		syntax->recursion++;

		if (syntax->term_hash == NULL)
			BuildHash (syntax);
		
		for (i = 0; syntax->terms[i].keyword; i++)
			if (syntax->terms[i].sub_syntax)
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
	SyntaxStack  *pnew = (SyntaxStack *) safecalloc (1, sizeof (SyntaxStack));

	/* saving our status to be able to come back to it later */
	if (config->current_syntax)
	{
		config->current_syntax->current_term = config->current_term;
		config->current_syntax->current_flags = config->current_flags;
	}
	config->current_flags = CF_NONE ;
	config->current_term = NULL ;
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
	LOCAL_DEBUG_OUT("%p: \"%s\", old is %p:  current_prepend = [%s]\n",
			 syntax, syntax->display_name, config->syntax, config->current_prepend);
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
		
		LOCAL_DEBUG_OUT("%p: \"%s\", old is %p: term = \"%s\"\n",
						 config->syntax, config->syntax->display_name, 
						 pold->syntax,
						 config->current_term?config->current_term->keyword:"");
		
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
PushStorage (ConfigDef * config, void *storage)
{
	StorageStack *pnew = (StorageStack *) safecalloc (1, sizeof (StorageStack));

	pnew->storage = storage;
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
config_error (ConfigDef * config, char *err_text)
{
	if (config)
	{
		char         *eol = strchr (config->tline, '\n');

		if (eol)
			*eol = '\0';
		show_error ("in %s (line %d):%s[%.50s]",
				    config->current_syntax->syntax->display_name, config->line_count, err_text, config->tline);
		if (eol)
			*eol = '\n';
  	}
}

/* Creating and Initializing new ConfigDef */
ConfigDef    *
NewConfig (char *myname, SyntaxDef * syntax, ConfigDataType type, ConfigData  source, SpecialFunc special, int create)
{
	ConfigDef    *new_conf;

	if (myname == NULL)
		return NULL;

    new_conf = (ConfigDef *) safecalloc (1, sizeof (ConfigDef));
	new_conf->special = special;
	new_conf->fd = -1;
	new_conf->fp = NULL;
    new_conf->flags = 0;
	if (source.vptr != NULL )
		switch (type)
		{
		 case CDT_Filename:
			 {
				 char         *realfilename = put_file_home (source.filename);

				 if (!realfilename)
				 {
					 free (new_conf);
					 return NULL;
				 }
				 new_conf->fd =
#ifdef __CYGWIN__
                     open (realfilename, create ? O_CREAT | O_RDONLY |O_BINARY: O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP|O_BINARY);
#else
                     open (realfilename, create ? O_CREAT | O_RDONLY: O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP);
#endif
				 free (realfilename);
                 set_flags( new_conf->flags, CP_NeedToCloseFile);
			 }
			 break;
		 case CDT_FilePtr:
			 new_conf->fp = source.fileptr;
			 new_conf->fd = fileno (new_conf->fp);
			 break;
		 case CDT_FileDesc:
			 new_conf->fd = *source.filedesc;
			 break;
		 case CDT_Data:
			 break;
         case CDT_FilePtrAndData :
             new_conf->fp = source.fileptranddata->fp;
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
InitConfigReader (char *myname, SyntaxDef * syntax, ConfigDataType type, ConfigData source, SpecialFunc special)
{
	ConfigDef    *new_conf = NewConfig (myname, syntax, type, source, special, False);

	if (new_conf == NULL)
		return NULL;
	if (source.vptr == NULL)
	{
		DestroyConfig (new_conf);
		return NULL;
	}

	if (type == CDT_Data)
	{
		/* allocate to store entire data */
		new_conf->buffer_size = strlen (source.data) + 1;
		new_conf->buffer = (char *)safemalloc (new_conf->buffer_size);
		strcpy (new_conf->buffer, source.data);
		new_conf->bytes_in = new_conf->buffer_size - 1;
    } else if( type == CDT_FilePtrAndData )
    {
        FilePtrAndData *fpd = source.fileptranddata;
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
	if (config->current_syntax)
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

void
ProcessSubSyntax (ConfigDef * config, void *storage, SyntaxDef * syntax)
{
	PushStorage (config, storage);
	LOCAL_DEBUG_OUT("Old_syntax = \"%s\", new_syntax = \"%s\"", config->syntax->display_name, syntax->display_name );
	if (config->syntax->terminator == syntax->file_terminator)
	{										   /* need to push back term's data into config buffer */
		int skip_tokens = 0 ; 
		config->cursor = config->tdata;
		skip_tokens = GetTermUseTokensCount(config->current_term);
		if ( get_flags(config->current_term->flags, TF_NAMED|TF_INDEXED|TF_DIRECTION_INDEXED) )
			++skip_tokens ; 
		if( skip_tokens > 0 )
			config->cursor = tokenskip( config->cursor, skip_tokens );
	} else if (config->syntax->terminator == syntax->terminator)
	{										   /* need to push back entire term's line into config buffer */
		config->cursor = config->tline_start;
	}

/*  fprintf( stderr, "\nprocessing as subsyntax: [%s]", config->cursor );
*/
	PushSyntax (config, syntax);
}

void ProcessStatement(ConfigDef *config)
{
	if( config && config->statement_handler ) 
		config->statement_handler(config);
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
						if (cur[i] == '\0' || cur[i] != _disabled_keyword[i])
							break;
					if (i == DISABLED_KEYWORD_SIZE)
					{						   /* comments - skip entire line */
						config->current_flags |= CF_DISABLED_OPTION;
						/* let's skip few spaces here */
						while (isspace ((int)cur[i]) && cur[i] != terminator) ++i;
						if (cur[i] == '\0' || cur[i] == terminator)
							break;				   /* not a valid option */
						cur = &cur[i];
					}else
						set_flags( config->current_flags, CF_PUBLIC_OPTION);/* comments are always public */
				}

				if (*cur == MYNAME_CHAR)
				{							   /* check if we have MyName here */
					register char *mname = config->myname;
				
                    print_trimmed_str(  "private option at", cur );
					++cur;
					for( i = 0 ; mname[i] != '\0' && cur[i] != '\0' ; ++i ) 
						if (tolower (mname[i]) != tolower (cur[i]) )
							break;
					if (mname[i] != '\0')
					{						   /* that was a foreign optiion - belongs to the other executable */
						if (my_only)
							break;
						set_flags( config->current_flags, CF_FOREIGN_OPTION );
						/* keeping *MyName part of the token  without leading * for foreign options */
					}else
						cur+=i;  /* skipping *MyName part of the token */ 
				} else
                {
                    if( *cur == terminator || *cur == file_terminator )   /* public option keyword may not be empty ! */
                        break;
                    set_flags( config->current_flags, CF_PUBLIC_OPTION);
                    print_trimmed_str( "public option at", cur );
                }
				config->tline = cur;		   /*that will be the begginnig of the term */

				/* now we should copy everything from after the first space to
				   config->current_data and set current_data_len ; (unless its a comment) */
				if( *cur != COMMENTS_CHAR )
				{	
                	i = 0 ;
                	while (cur[i] && !isspace ((int)cur[i]) && cur[i] != terminator && cur[i] != file_terminator)
                    	++i;
                	while (isspace ((int)cur[i]) && cur[i] != terminator && cur[i] != file_terminator)
                    	++i;
				}else
					i = 1;

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
							return NULL;
						}
                        data = config->current_data;
					}
                    data[i] = cur[i];
				}
                LOCAL_DEBUG_OUT( "%d bytes of data stored", i );
                cur = &(cur[i]);
				/* now let's go back and remove trailing spaces */
				if (config->tdata[0] == file_terminator)
					set_flags(config->current_flags, CF_LAST_OPTION);
				else
				{
                    while( --i >= 0 )
					{
						if (config->tdata[i] == file_terminator)
							set_flags(config->current_flags, CF_LAST_OPTION);
						if (!isspace ((int)data[i]))
							break;
					}
					i++;
				}
				/* since \0 is our normal data terminator we really should trigger
				   end of the configuration when the file ends : */
				if (file_terminator == '\0')
					clear_flags(config->current_flags, CF_LAST_OPTION);
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


TermDef _as_comments_term =	{TF_NO_MYNAME_PREPENDING, "#", 1, TT_COMMENT, TT_COMMENT, NULL};
TermDef _as_unknown_term_public  =	{TF_NO_MYNAME_PREPENDING, "unknown", 7, TT_ANY, TT_ANY, NULL};
TermDef _as_unknown_term_private =	{0, "unknown", 7, TT_ANY, TT_ANY, NULL};

TermDef      *
FindStatementTerm (char *tline, SyntaxDef * syntax)
{
    ASHashData hdata = {0};
    LOCAL_DEBUG_OUT( "looking for pterm for [%c%c%c...]in hash table  %p of the syntax %s ", tline[0], tline[1], tline[2], syntax->term_hash, syntax->display_name );
	if( tline[0] == COMMENTS_CHAR )
		return &_as_comments_term;

	if( isspace(tline[0]) )
	{
		int i = 0; 
		while( isspace(tline[i]) ) 	   ++i;
		if( tline[i] == '~') 
			if( get_hash_item (syntax->term_hash, AS_HASHABLE(&tline[i]), &hdata.vptr)==ASH_Success  )
				return hdata.vptr;
	}	 
    if( get_hash_item (syntax->term_hash, AS_HASHABLE(tline), &hdata.vptr)!=ASH_Success  )
		hdata.vptr = NULL;
    LOCAL_DEBUG_OUT( "FOUND pterm %p", hdata.vptr );
    return hdata.vptr;
}



/* main parsing procedure */
int
config2tree_storage (ConfigDef * config, ASTreeStorageModel **tail, Bool ignore_foreign)
{
	int           TopLevel = 0;
	unsigned long flags;

	PushStorage (config, tail);
	/* get line */
	while (!TopLevel)
	{
		while (GetNextStatement (config, ignore_foreign))
		{									   /* untill not end of text */
			flags = 0x00;
			TermDef *pterm = NULL ; 

#ifdef DEBUG_PARSER
			fprintf (stderr, "\nSentence Found:[%.50s ...]\n,\tData=\t[%s]", config->tline, config->current_data);
			fprintf (stderr, "\nLooking for the Term...");
#endif
			/* find term */
			if( get_flags( config->current_flags, CF_FOREIGN_OPTION ) ) 
			{
				int i = 0 ; 
				do
				{	
					++i;       /* it's ok - we can start with 1, since myname should have at least 1 char */
					pterm = FindStatementTerm (&(config->tline[i]), config->syntax);
				}while( pterm == NULL && !isspace(config->tline[i]) && config->tline[i] != '\0' );
			}else
				pterm = FindStatementTerm (config->tline, config->syntax);

			if ( pterm == NULL )
				pterm = get_flags( config->current_flags, CF_PUBLIC_OPTION)?
				 			&_as_unknown_term_public : &_as_unknown_term_private ;
			
			config->current_term = pterm;
			
			LOCAL_DEBUG_OUT("Term:[%s]", config->current_term->keyword);
			if( isspace(config->tline[0]) &&  pterm->keyword_len > 0 &&
				mystrncasecmp(pterm->keyword, config->current_data, pterm->keyword_len) == 0 ) 
			{              /* we need to skip one token in current_data :  */
				char *src = tokenskip( config->current_data, 1 ) ;
				char *dst = config->current_data ;
				if( src != dst ) 
				{
					int i = 0 ; 
					do{ dst[i] = src[i]; }while( src[++i] );
					dst[i] = '\0';
				}		
			}	 
			if (get_flags( pterm->flags, TF_OBSOLETE))
				config_error (config, "Heh, It seems that I've encountered obsolete config option. I'll ignore it for now, Ok ?!");
			if (get_flags( pterm->flags, TF_PHONY))
				set_flags( config->flags, CF_PHONY_OPTION );
  			if (get_flags( pterm->flags, TF_SPECIAL_PROCESSING))
			{
				if (config->special)
				{
					flags = (*(config->special)) (config);
					if (get_flags (flags, SPECIAL_BREAK))
						break;
					if (get_flags (flags, SPECIAL_STORAGE_ADDED))
					{
						ASTreeStorageModel **ctail = config->current_tail->storage;
						while(*ctail) ctail = &((*ctail)->next);
						tail = config->current_tail->storage = ctail;
					}
				}
			}
			if (!get_flags (flags, SPECIAL_SKIP))
				ProcessStatement (config);
			/* Process Statement may alter config's state as it may dive into subconfig - 
			 * must make sure that did not happen : */
			if( config->current_term ) 
			{	
				if ( get_flags(config->current_term->flags, TF_SYNTAX_TERMINATOR) || 
						IsLastOption (config) )
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
InitConfigWriter (char *myname, SyntaxDef * syntax, ConfigDataType type, ConfigData source)
{
#ifdef WITH_CONFIG_WRITER
	ConfigDef    *new_conf = NewConfig (myname, syntax, type, source, NULL, True);

	if (new_conf == NULL)
		return NULL;
	if (source.vptr != NULL)
	{
		if (type == CDT_Data)
		{
			if ((new_conf->buffer_size = strlen (source.data)) > 0)
			{
				new_conf->buffer = safemalloc (++(new_conf->buffer_size));
				strcpy (new_conf->buffer, source.data);
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
/* writes block of text in to the output buffer, enlarging it if needed.
 * it starts with block_start and end with block_end or '\0'
 * if block_end is NULL
 */

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
parser_add_terminator (char *ptr, char terminator)
{
	if (*ptr != terminator && (*ptr == '\0' && *(ptr - 1) != terminator))
		*(ptr++) = terminator;
	return ptr;
}

