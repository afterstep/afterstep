/*
 * Copyright (c) 2000 Sasha Vasko <sasha at aftercode.net>
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
//#include "../libAfterStep/parse.h"
#include "../libAfterStep/screen.h"

#include "afterconf.h"

/*****************************************************************************
 *
 * This contains a set of function and syntax definition for parsing
 * AS function commands
 *
 ****************************************************************************/
extern SyntaxDef WharfSyntax;

SyntaxDef     PopupFuncSyntax = {
	'\n',
	'\0',
	FuncTerms,
	0,										   /* use default hash size */
	' ',
	"\t",
	"\t",
    "Popup/Complex function definition",
	NULL,
	0
};

void ProcessStatement (ConfigDef * config);

unsigned long
TrailingFuncSpecial (ConfigDef * config, FreeStorageElem ** storage, int skip_tokens)
{
	register char *cur;

	if (config == NULL || storage == NULL)
		return SPECIAL_BREAK;

    /* processing binding item key, context and modifyers : */
	ProcessStatement (config);
    /* since we have have subconfig of Functions that has \n as line terminator
     * we are going to get entire line again at config->cursor
     * so lets skip skip_tokens tokens,
     * since those are not parts of following function */
    cur = tokenskip( config->cursor, skip_tokens );

	if (*cur != '\0')
	{
        TermDef      *pterm;

		config->cursor = cur;
        /* we are at the beginning of the function definition right now - lets process it :*/
        /* read in entire function definition */
		GetNextStatement (config, 1);
        /* lets find us the term for this definition */
        if ((pterm = FindStatementTerm (config->tline, config->syntax)) != NULL)
        {   /* we have a valid function definition : */
			config->current_term = pterm;
            /* we do not want to continue processing the rest of the config as
             * a functions : */
			config->current_flags |= CF_LAST_OPTION;
            /* lets parse in our function
             * ( it will be in subelem of the parent FreeStorageElem) : */
            ProcessStatement (config);
        }
	}
    /* already done processing current statement - let parser know about it : */
    return SPECIAL_SKIP;
}


FreeStorageElem **
Func2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, FunctionData * func)
{
	TermDef      *pterm;
	FreeStorageElem *new_elem;

	if (func == NULL || tail == NULL)
		return tail;

    FuncTerms[F_Folder].sub_syntax = &WharfSyntax ;

	pterm = FindTerm (&FuncSyntax, TT_ANY, func->func);
	/* adding balloon free storage here */
	if ((new_elem = AddFreeStorageElem (syntax, tail, pterm, WHARF_Wharf_ID)) != NULL)
	{
		char         *argv[1 + MAX_FUNC_ARGS];
		int           len = 0;

		register int  i;
		register char *dst;
		register char *src;

		for (i = 0; i <= MAX_FUNC_ARGS; i++)
			argv[i] = NULL;

		if (func->name)
		{
			src = func->name;
			len += 1 + strlen (src) + 1 + 1;
			dst = argv[new_elem->argc] = safemalloc (len);
			*(dst++) = '"';
			while (*src)
				*(dst++) = *(src++);
			*(dst++) = '"';
			*(dst) = '\0';
			new_elem->argc++;
		}

		if (get_flags (pterm->flags, USES_NUMVALS))
		{
            int max_args = MAX_FUNC_ARGS ;
            long default_val = 0 ;

            if( (default_val = default_func_val( func->func ))!= 0 )
                for ( ; 0 < max_args; max_args--)
                    if( func->func_val[max_args-1] != default_val )
                        break;

            for (i = 0; i < max_args; i++)
			{
				int           val_len;

                if ((src = string_from_int ((int)(func->func_val[i]))) == NULL)
					continue;
				if (*src == '\0')
					continue;

				val_len = strlen (src);

				if (func->unit[i] == '\0')
					argv[new_elem->argc] = src;
				else
				{
					char         *tmp = src;

					val_len++;
					dst = argv[new_elem->argc] = safemalloc (val_len + 1);
					while (*src)
						*(dst++) = *(src++);
					*(dst++) = func->unit[i];
					*dst = '\0';
					free (tmp);
				}
				len += val_len + 1;
				new_elem->argc++;
			}
		} else if (func->text != NULL)
		{
			argv[new_elem->argc] = mystrdup (func->text);
			len += strlen (argv[new_elem->argc]) + 1;
			new_elem->argc++;
		}
		if (len > 0)
		{
			new_elem->argv = (char **)safemalloc (sizeof (char *) * new_elem->argc);

			dst = safemalloc (len);
			for (i = 0; i < new_elem->argc; i++)
			{
				new_elem->argv[i] = dst;
				if ((src = argv[i]) != NULL)
				{
					while (*src)
						*(dst++) = *(src++);
					*(dst++) = '\0';
					free (argv[i]);
				}
			}
		}
		tail = &(new_elem->next);
	}
	return tail;
}

FunctionData *
String2Func ( const char *string, FunctionData *p_fdata, Bool quiet )
{
    FreeStorageElem *storage;
    TermDef      *fterm ;
    char *ptr = (char*)string ;
    ConfigItem item;
    int res ;

    if( ptr == NULL ) return NULL ;

    FuncTerms[F_Folder].sub_syntax = &WharfSyntax ;

    item.memory = NULL ;
	item.ok_to_free = False ;

    ptr = strip_whitespace( ptr );
    if ((fterm = txt2fterm (ptr, quiet)) == NULL)
        return NULL;

    storage = safecalloc( 1, sizeof(FreeStorageElem) ) ;
    storage->term = fterm ;

    ptr+= storage->term->keyword_len;
    while ( !isspace((int)*ptr) && *ptr )ptr++;
    if (!(fterm->flags & NEED_CMD))
        ptr = stripcomments( ptr );
    else
        ptr = strip_whitespace( ptr );

    args2FreeStorage (storage, ptr, strlen (ptr));
    res = ReadConfigItem( &item, storage );
    DestroyFreeStorage( &storage );

    if( res != 1 )
        return NULL ;

    if( p_fdata == NULL )
        return item.data.function ;

    copy_func_data( p_fdata, (item.data.function));
	memset( item.data.function, 0x00, sizeof(FunctionData) );
    item.ok_to_free =True ;
    ReadConfigItem( &item, NULL );
    return p_fdata;
}

ComplexFunction *
FreeStorage2ComplexFunction( FreeStorageElem *storage, ConfigItem *item, struct ASHashTable *list )
{
    ComplexFunction *cf = NULL ;
    ConfigItem l_item;

    if( storage && storage->sub)
    {
        if( item == NULL)
        {
		    l_item.memory = NULL ;
            if(ReadConfigItem (&l_item, storage))
                item = &l_item ;
        }

        if( item != NULL )
        {
            cf = new_complex_func( list, item->data.string );
            item->ok_to_free = 1;               /* gets copied anyways */
        }
        if( cf )
        {
            FreeStorageElem *pCurr;
            int start = cf->items_num ;
            FunctionData *tmp = cf->items;

            for( pCurr = storage->sub->next ; pCurr ; pCurr = pCurr->next )
                cf->items_num++ ;
            /* using more complex approach in order to zero new memory */
            /* tmp should never be anything but NULL, since we replace
             * old functions rather then append to those */
            cf->items = safecalloc( cf->items_num, sizeof(FunctionData));
            if( tmp )
            {
                memcpy( cf->items, tmp, start*sizeof(FunctionData) );
                free( tmp );
            }
            for( pCurr = storage->sub->next ; pCurr ; pCurr = pCurr->next )
            {
                if( pCurr->term )
                    if( pCurr->term->type == TT_FUNCTION && ReadConfigItem (item, pCurr))
                    {
                        if( item->data.function->func != F_ENDFUNC )
                        {
                            copy_func_data( &(cf->items[start++]), (item->data.function));
                            memset( item->data.function, 0x00, sizeof(FunctionData) );
                        }
                        item->ok_to_free =True ;
                    }
            }
            cf->items_num = start ;
        }
    }
    return cf;
}

FreeStorageElem **
ComplexFunction2FreeStorage( SyntaxDef *syntax, FreeStorageElem **tail, ComplexFunction *cf )
{
    FreeStorageElem **new_tail ;
    register int i ;

    if (cf == NULL)
		return tail;
    if (cf->magic != MAGIC_COMPLEX_FUNC )
		return tail;

    new_tail = QuotedString2FreeStorage (syntax, tail, NULL, cf->name, FEEL_Function_ID);
	if ( new_tail == tail )
		return tail;

    tail = &((*tail)->sub);

    for (i = 0; i < cf->items_num; i++)
        tail = Func2FreeStorage(&PopupFuncSyntax, tail, &(cf->items[i]));

    tail = Flag2FreeStorage (&PopupFuncSyntax, tail, F_ENDFUNC);

    return new_tail;

}

