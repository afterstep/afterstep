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


void
statement2free_storage (ConfigDef * config)
{
	FreeStorageElem  *pNext;
	TermDef      *pterm = config->current_term;
LOCAL_DEBUG_OUT( "checking for foreign option ...%s", "" );
	if (IsForeignOption (config))
		return;
	
	if (pterm->type == TT_COMMENT || pterm->type == TT_INLINE_COMMENT )
		return;
	if (pterm->type == TT_ANY )
	{	
#ifdef UNKNOWN_KEYWORD_WARNING
		if( !get_flags( config->flags, CP_IgnoreUnknown ) )
		  	config_error (config, " unknown keyword encountered");
#endif
		return;
	}

LOCAL_DEBUG_OUT( "adding storage ...%s", "" );
	if ((pNext = AddFreeStorageElem (config->syntax, config->current_tail->storage, pterm, ID_ANY)) == NULL)
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

	config->current_tail->storage = &(pNext->next);
	if (pterm->sub_syntax)
		ProcessSubSyntax (config, &(pNext->sub), pterm->sub_syntax);
	return;
}


int ParseConfig (ConfigDef * config, FreeStorageElem ** tail)
{
	config->statement_handler = statement2free_storage;
	set_flags( config->flags, CP_IgnoreForeign );
	return config2tree_storage(config, (ASTreeStorageModel **)tail);	   
}	 

FreeStorageElem *
tline_subsyntax_parse(const char *keyword, char *tline, FILE * fd, char *myname, SyntaxDef *syntax, SpecialFunc special, FreeStorageElem **foreign_options)
{
    FilePtrAndData fpd ;
    ConfigDef    *ConfigReader ;
    FreeStorageElem *storage = NULL, *more_stuff = NULL;
	ConfigData cd ;
    if( syntax == NULL )
        return NULL;

    fpd.fp = fd ;
	if( keyword ) 
	{
	    fpd.data = safemalloc( strlen(keyword) + 1 + strlen(tline)+1+1 ) ;
	    sprintf( fpd.data, "%s %s\n", keyword, tline );
	}else
	{
	    fpd.data = safemalloc( strlen(tline)+1+1 ) ;
		sprintf( fpd.data, "%s\n", tline );
	}
	LOCAL_DEBUG_OUT( "fd(%p)->tline(\"%s\")->fpd.data(\"%s\")", fd, tline, fpd.data );
	cd.fileptranddata = &fpd ;
    ConfigReader = InitConfigReader (myname, syntax, CDT_FilePtrAndData, cd, special);
    free( fpd.data );

    if (!ConfigReader)
        return NULL;

	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &storage);

	/* getting rid of all the crap first */
    StorageCleanUp (&storage, &more_stuff, CF_DISABLED_OPTION);
    DestroyFreeStorage (&more_stuff);

	if( foreign_options )
		StorageCleanUp (&storage, foreign_options, CF_PHONY_OPTION|CF_FOREIGN_OPTION);


	DestroyConfig (ConfigReader);

	return storage;
}

FreeStorageElem *
file2free_storage(const char *filename, char *myname, SyntaxDef *syntax, SpecialFunc special, FreeStorageElem **foreign_options )
{
	ConfigData cd ;
	ConfigDef    *config_reader ;
	FreeStorageElem *storage = NULL ;
	
	cd.filename = filename ;
	config_reader = InitConfigReader (myname, syntax, CDT_Filename, cd, special);
	if (!config_reader)
		return NULL;

	PrintConfigReader (config_reader);
	ParseConfig(config_reader, &storage);

	/* getting rid of all the crap first */
	if( foreign_options )
		StorageCleanUp (&storage, foreign_options, CF_DISABLED_OPTION|CF_PHONY_OPTION|CF_FOREIGN_OPTION);

	DestroyConfig (config_reader);   
	return storage;
}	 


#ifdef TEST_PARSER_FS

#include "../libAfterConf/afterconf.h"

# define CONFIG_FILE	"~/triangular_wharf"
# define CONFIG_SYNTAX	&WharfSyntax
# define CONFIG_SPECIAL	NULL   /*WharfSpecialFunc*/
# define CONFIG_MYNAME  "MyWharf"

extern SyntaxDef StyleSyntax ;
int 
main( int argc, char ** argv ) 
{
	
	char *fullfilename;
	FreeStorageElem * tree ;
	InitMyApp ("TestParserFS", argc, argv, NULL, NULL, 0 );
	LinkAfterStepConfig();
	InitSession();
#if 0	
	fullfilename = PutHome( CONFIG_FILE );

	
	tree = file2free_storage(fullfilename, CONFIG_MYNAME, CONFIG_SYNTAX, CONFIG_SPECIAL, NULL );
#else
	{
		ConfigData    cd ;
		ConfigDef    *ConfigReader;

		cd.data = mystrdup("DefaultGeometry 581x340+607+96 , Layer 0, Slippery, StartsOnDesk 0, ViewportX 0, ViewportY 0, StartNormal") ;
		ConfigReader = InitConfigReader ("afterstep", &StyleSyntax, CDT_Data, cd, NULL);
		ParseConfig (ConfigReader, &tree);
	}
#endif
	freestorage_print(CONFIG_MYNAME, CONFIG_SYNTAX,tree, 0);	   
	DestroyFreeStorage (&tree);
	FreeMyAppResources();
#   ifdef DEBUG_ALLOCS
	print_unfreed_mem ();
#   endif /* DEBUG_ALLOCS */
	return 1;
}
#endif












/***************************************************************************/
/*                   config Writing stuff                                  */
/***************************************************************************/

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
		}else if (pElem->flags & CF_COMMENTED_OPTION) /*C1: DISABLED_KEYWORD_SIZE */
		{
			*(ptr++) = COMMENTS_CHAR;
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
			ptr = parser_add_terminator (ptr, csyntax->terminator);
		else if (pElem->term->sub_syntax != NULL)
		{
			if (pElem->term->sub_syntax->terminator == csyntax->terminator)
				ptr = parser_add_terminator (ptr, csyntax->terminator);
			else if (csyntax->terminator == '\0' && pElem->term->sub_syntax->terminator != '\0')
				ptr = parser_add_terminator (ptr, pElem->term->sub_syntax->terminator);
		}

		if (*(ptr - 1) != csyntax->terminator) /* V6: min(csyntax->prepend_sub,1) */
		{									   /* we don't want for tokens to get joined together */
			src = csyntax->prepend_sub;
			if (src != NULL && *src != '\0')
				while (*src)
					*(ptr++) = *(src++);
			else
				ptr = parser_add_terminator (ptr, csyntax->token_separator);
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

	while (GetNextStatement (config))
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
WriteConfig (ConfigDef * config, FreeStorageElem *storage,
			 ConfigDataType target_type, ConfigData *target, unsigned long flags)
{
#ifdef WITH_CONFIG_WRITER
	FreeStorageElem *copy = NULL;
	struct WriteBuffer t_buffer;
	int           t_fd = -1;

	if (config == NULL || storage == NULL || target == NULL)
		return -1;

	CopyFreeStorage (&copy, storage);

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
		ScanAndWriteExistant (config, &copy, &t_buffer, flags);
	/* now writing remaining elements */
	WriteRemnants (config, &t_buffer, copy);
	
	DestroyFreeStorage (&copy);

	t_buffer.buffer[t_buffer.used] = config->syntax->file_terminator;
	t_buffer.buffer[t_buffer.used + 1] = '\0';

	if (config->syntax->file_terminator != '\0')
		t_buffer.used++;

	/* now saving buffer into file if we need to */
	switch (target_type)
	{
	 case CDT_Filename:
		 t_fd = open (target->filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		 break;
	 case CDT_FilePtr:
     case CDT_FilePtrAndData:
         t_fd = fileno (target->fileptr);
		 break;
	 case CDT_FileDesc:
		 t_fd = *(target->filedesc);
		 break;
	 case CDT_Data:
		 target->data = t_buffer.buffer;
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
