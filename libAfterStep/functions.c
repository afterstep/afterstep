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

/*#define DO_CLOCKING      */

#include "../configure.h"

#include "asapp.h"
#include "afterstep.h"
#include "screen.h"
#include "parser.h"
#include "functions.h"

/*************************************************************************/
/* parsing code :
 */
/*************************************************************************/
/*The keys must be in lower case! */
struct charstring win_contexts[] = {
	{'w', C_WINDOW},
	{'t', C_TITLE},
	{'i', C_ICON},
	{'r', C_ROOT},
	{'f', C_FRAME},
	{'s', C_SIDEBAR},
	{'1', C_L1},
	{'2', C_R1},
	{'3', C_L2},
	{'4', C_R2},
	{'5', C_L3},
	{'6', C_R3},
	{'7', C_L4},
	{'8', C_R4},
	{'9', C_L5},
	{'0', C_R5},
	{'a', C_WINDOW | C_TITLE | C_ICON | C_ROOT | C_FRAME | C_SIDEBAR |
	 C_L1 | C_L2 | C_L3 | C_L4 | C_L5 | C_R1 | C_R2 | C_R3 | C_R4 | C_R5},
	{0, 0}
};

/* The keys musat be in lower case! */
struct charstring key_modifiers[] = {
	{'s', ShiftMask},
	{'c', ControlMask},
	{'m', Mod1Mask},
	{'1', Mod1Mask},
	{'2', Mod2Mask},
	{'3', Mod3Mask},
	{'4', Mod4Mask},
	{'5', Mod5Mask},
	{'a', AnyModifier},
	{'n', 0},
	{0, 0}
};


/*
 * Turns a  string context of context or modifier values into an array of
 * true/false values (bits)
 * returns pointer to the fisrt character after context specification
 */

char         *
parse_context (char *string, int *output, struct charstring *table)
{
	register int  j;
	register char *ptr;
	char          tmp1;

	*output = 0;
	for (ptr = string; isspace (*ptr); ptr++);
	for (; *ptr && !isspace (*ptr); ptr++)
	{
		/* in some BSD implementations, tolower(c) is not defined
		 * unless isupper(c) is true */
		tmp1 = *ptr;
		if (isupper (tmp1))
			tmp1 = tolower (tmp1);
		/* end of ugly BSD patch */
		for (j = 0; table[j].key != 0; j++)
			if (tmp1 == table[j].key)
			{
				*output |= table[j].value;
				break;
			}
		if (table[j].key == 0)
		{
            show_error("undefined context specifyer [%c] encountered in [%s].", *ptr, string);
		}
	}
	return ptr;
}

int
parse_modifier( char *tline )
{
    int mods = 0;
    parse_context (tline, &mods, key_modifiers);
    return mods;
}

int parse_win_context (char *tline)
{
    int ctxs = 0;
    parse_context (tline, &ctxs, win_contexts);
    return ctxs;
}

char *parse_modifier_str( char *tline, int *mods_ret )
{
    int mods = 0;
    tline = parse_context (tline, &mods, key_modifiers);
    *mods_ret = mods;
    return tline;
}

char *parse_win_context_str (char *tline, int *ctxs_ret)
{
    int ctxs = 0;
    tline = parse_context (tline, &ctxs, win_contexts);
    *ctxs_ret = ctxs ;
    return tline;
}

TermDef      *
txt2fterm (const char *txt, int quiet)
{
	TermDef      *fterm;

	if (FuncSyntax.term_hash == NULL)
        PrepareSyntax (&FuncSyntax);
    if ((fterm = FindStatementTerm ((char *)txt, &FuncSyntax)) == NULL && !quiet)
        show_error ("unknown function name in function specification [%s].\n", txt);

	return fterm;
}

TermDef      *
func2fterm (FunctionCode func, int quiet)
{
	register int  i;

	/* in most cases that should work : */
	if (FuncTerms[func].id == func)
		return &(FuncTerms[func]);

	/* trying fallback if it did not : */
	for (i = 0; i < F_FUNCTIONS_NUM; i++)
		if (FuncTerms[i].id == func)
			return &(FuncTerms[i]);

	/* something terribly wrong has happened : */
	return NULL;
}

int
txt2func (const char *text, FunctionData * fdata, int quiet)
{
	TermDef      *fterm;

	for (; isspace (*text); text++);
	fterm = txt2fterm (text, quiet);
	if (fterm != NULL)
	{
		init_func_data (fdata);
		fdata->func = fterm->id;
		for (; !isspace (*text) && *text; text++);
		for (; isspace (*text); text++);
		if (*text)
		{
			const char   *ptr = text + strlen (text);

			for (; isspace (*(ptr - 1)); ptr--);
			fdata->text = mystrndup (text, ptr - text);
		}
	}
	return (fterm != NULL);
}

int
parse_func (const char *text, FunctionData * data, int quiet)
{
	TermDef      *fterm;
	char         *ptr;
	int           curr_arg = 0;
	int           sign = 0;

	init_func_data (data);
	for (ptr = (char *)text; isspace (*ptr); ptr++);
	if (*ptr == '\0')
	{
		if (!quiet)
            show_error ("empty function specification encountered.%s");
		return -1;
	}

	if ((fterm = txt2fterm (ptr, quiet)) == NULL)
		return -2;

	if (IsInternFunc (fterm->id))
		return 0;

	while (!isspace (*ptr) && *ptr)
		ptr++;
	data->func = fterm->id;
	if (fterm->flags & TF_SYNTAX_TERMINATOR)
		return 0;

    set_func_val (data, -1, default_func_val(data->func));

	/* now let's do actual parsing */
	if (!(fterm->flags & NEED_CMD))
		ptr = stripcomments (ptr);
	else
	{										   /* we still want to strip trailing whitespaces */
		char         *tail = ptr + strlen (ptr) - 1;

		for (; isspace (*tail) && tail > ptr; tail--);
		*(tail + 1) = '\0';
	}
	/* this function is very often called so we try to use as little
	   calls to other function as possible */
	for (; *ptr; ptr++)
	{
		if (!isspace (*ptr))
		{
			int           is_text = 0;

			if (*ptr == '"')
			{
				char         *tail = ptr;
				char         *text;

				while (*(tail + 1) && *(tail + 1) != '"')
					tail++;
				if (*(tail + 1) == '\0')
				{
                    show_error ("impaired doublequotes encountered in [%s].", ptr);
					return -3;
				}
				text = mystrndup (ptr + 1, (tail - ptr));
				if (data->name == NULL)
					data->name = text;
				else if (data->text == NULL)
					data->text = text;
				ptr = tail + 1;
			} else if (isdigit (*ptr))
			{
				int           count;
				char          unit;
				int           val;

				for (count = 1; isdigit (*(ptr + count)); count++);
				if (*(ptr + count) != '\0' && !isspace (*(ptr + count)))
					is_text = (!isspace (*(ptr + count + 1)) && *(ptr + count + 1) != '\0') ? 1 : 0;
				if (is_text == 0)
					ptr = parse_func_args (ptr, &unit, &val) - 1;
				if (curr_arg < MAX_FUNC_ARGS)
				{
					data->func_val[curr_arg] = (sign != 0) ? val * sign : val;
					data->unit[curr_arg] = unit;
					curr_arg++;
				}
			} else if (*ptr == '-')
			{
				if (sign == 0)
				{
					sign--;
					continue;
				} else
					is_text = 1;
			} else if (*ptr == '+')
			{
				if (sign == 0)
				{
					sign++;
					continue;
				} else
					is_text = 1;
			} else
				is_text = 1;

			if (is_text)
			{
				if (sign != 0)
					ptr--;
				if (data->text == NULL)
				{
					if (fterm->flags & NEED_CMD)
					{
						data->text = mystrdup (ptr);
						break;
					}
					ptr = parse_token (ptr, &(data->text)) - 1;
				} else
					while (*(ptr + 1) && !isspace (*(ptr + 1)))
						ptr++;
			}
			sign = 0;
		}
	}

    decode_func_units (data);
	data->hotkey = scan_for_hotkey (data->name);

	/* now let's check for valid number of arguments */
	if ((fterm->flags & NEED_NAME) && data->name == NULL)
	{
        show_error ("function specification requires \"name\" in [%s].", text);
		return -4;
	}
	if (data->text == NULL)
	{
		if ((fterm->flags & NEED_WINDOW) || ((fterm->flags & NEED_WINIFNAME) && data->name != NULL))
		{
            show_error ("function specification requires window name in [%s].", text);
			return -5;
		}
		if (fterm->flags & NEED_CMD)
		{
            show_error ("function specification requires shell command or full file name in [%s].", text);
			return -6;
		}
	}
/*
   if( data->func == F_SCROLL )
   {
   fprintf( stderr,"Function parsed: [%s] [%s] [%s] [%d] [%d] [%c]\n",fterm->keyword,data->name,data->text,data->func_val[0], data->func_val[1],data->hotkey );
   fprintf( stderr,"from: [%s]\n", text );
   }
 */
	return 0;
}

#if 0
FunctionData *
String2Func ( const char *string, FunctionData *p_fdata, Bool quiet )
{
    if( p_fdata )
        free_func_data( p_fdata );
    else
        p_fdata = safecalloc( 1, sizeof(FunctionData));

    LOCAL_DEBUG_OUT( "parsing message \"%s\"", string );
    if( parse_func( string, p_fdata, quiet ) < 0 )
    {
        LOCAL_DEBUG_OUT( "parsing failed%s", "" );
        free_func_data( p_fdata );
        free( p_fdata );
        p_fdata = NULL;
    }else
        LOCAL_DEBUG_OUT( "parsing success with func = %d", p_fdata->func );
    return p_fdata;
}

#endif
/****************************************************************************/
/* FunctionData - related code 	                                            */
void
init_func_data (FunctionData * data)
{
	int           i;

	if (data)
	{
		data->func = F_NOP;
		for (i = 0; i < MAX_FUNC_ARGS; i++)
		{
			data->func_val[i] = DEFAULT_OTHERS;
			data->unit_val[i] = 0;
			data->unit[i] = '\0';
		}
		data->hotkey = '\0';
		data->name = data->text = NULL;
	}
}

void
copy_func_data (FunctionData * dst, FunctionData * src)
{
	if (dst && src)
	{
		register int  i;

		dst->func = src->func;
		dst->name = src->name;
		dst->text = src->text;
		for (i = 0; i < MAX_FUNC_ARGS; i++)
		{
			dst->func_val[i] = src->func_val[i];
			dst->unit[i] = src->unit[i];
		}
	}
}

void
dup_func_data (FunctionData * dst, FunctionData * src)
{
	if (dst && src)
	{
		register int  i;

		dst->func = src->func;
		dst->name = mystrdup(src->name);
		dst->text = mystrdup(src->text);
		for (i = 0; i < MAX_FUNC_ARGS; i++)
		{
			dst->func_val[i] = src->func_val[i];
			dst->unit[i] = src->unit[i];
		}
	}
}


void
set_func_val (FunctionData * data, int arg, int value)
{
	int           i;

	if (arg >= 0 && arg < MAX_FUNC_ARGS)
		data->func_val[arg] = value;
	else
		for (i = 0; i < MAX_FUNC_ARGS; i++)
			data->func_val[i] = value;
}

int
free_func_data (FunctionData * data)
{
	if (data)
	{
		if (data->name)
		{
			free (data->name);
			data->name = NULL;
		}
		if (data->text)
		{
			free (data->text);
			data->text = NULL;
		}
		return data->func;
	}
	return F_NOP;
}

void
destroy_func_data( FunctionData **pdata )
{
    if( pdata && *pdata )
    {
        free_func_data( *pdata );
        free( *pdata );
        *pdata = NULL ;
    }
}

long
default_func_val( FunctionCode func )
{
    long val = 0 ;
    switch (func)
	{
	 case F_MAXIMIZE:
         val = DEFAULT_MAXIMIZE;
		 break;
	 case F_MOVE:
	 case F_RESIZE:
         val = INVALID_POSITION;
		 break;
     default:
         break;
	}
    return val;
}

void
decode_func_units (FunctionData * data)
{
    register int  i;
    int defaults[MAX_FUNC_ARGS] = { Scr.MyDisplayWidth, Scr.MyDisplayHeight };

	for (i = 0; i < MAX_FUNC_ARGS; i++)
		switch (data->unit[i])
		{
		 case 'p':
		 case 'P':
			 data->unit_val[i] = 100;
			 break;
		 default:
             data->unit_val[i] = defaults[i];
		}
}

/**********************************************************************
 * complex function management code :
 **********************************************************************/
void
really_destroy_complex_func(ComplexFunction *cf)
{
    if( cf )
    {
        if( cf->magic == MAGIC_COMPLEX_FUNC )
        {
            register int i ;

            cf->magic = 0 ;
            if( cf->name )
                free( cf->name );
            if( cf->items )
            {
                for( i = 0 ; i < cf->items_num ; i++ )
                    free_func_data( &(cf->items[i]) );
                free( cf->items );
            }
            free( cf );
        }
    }
}

void
complex_function_destroy(ASHashableValue value, void *data)
{
    ComplexFunction *cf = data ;

    if( (char*)value )
        free( (char*)value );
    if( cf && cf->magic == MAGIC_COMPLEX_FUNC )
    {
        if( cf->name == (char*)value )
            cf->name = NULL ;
        really_destroy_complex_func(cf);
    }else if(data)
        free(data);
}

void
init_list_of_funcs(struct ASHashTable **list, Bool force)
{
    if( list == NULL ) return ;

    if( force && *list != NULL )
        destroy_ashash( list );

    if( *list == NULL )
        *list = create_ashash( 0, casestring_hash_value,
                                  casestring_compare,
                                  complex_function_destroy);
}

/* list could be NULL here : */
ComplexFunction *
new_complex_func( struct ASHashTable *list, char *name )
{
    ComplexFunction *cf = NULL ;

    if( name == NULL )
        return NULL ;
    /* enlisting complex function is optional */
    cf = (ComplexFunction*) safecalloc (1, sizeof(ComplexFunction));
    cf->name = mystrdup(name);
    cf->magic = MAGIC_COMPLEX_FUNC ;
    if( list )
    {
        remove_hash_item( list, AS_HASHABLE(name), NULL, True);  /* we want only one copy */
        if( add_hash_item( list, AS_HASHABLE(cf->name), cf) != ASH_Success )
        {
            really_destroy_complex_func( cf );
            cf = NULL ;
        }
    }
    return cf;
}

ComplexFunction *
find_complex_func( struct ASHashTable *list, char *name )
{
    ComplexFunction *cf = NULL ;
    if( name && list )
        if( get_hash_item( list, AS_HASHABLE(name), (void**)&cf) != ASH_Success )
            cf = NULL ; /* we are being paranoid */
    return cf;
}

/***************************************************************
 * MenuData code :
 ***************************************************************/
/****************************************************************************
 * Implementing create-destroy functions :
 ****************************************************************************/
void
menu_data_item_destroy(MenuDataItem *mdi)
{
LOCAL_DEBUG_CALLER_OUT( "menu_data_item_destroy(\"%s\",%p)", ((mdi && mdi->fdata)?(mdi->fdata->name):"NULL"), mdi);
    if( mdi )
    {
        if( mdi->magic == MAGIC_MENU_DATA_ITEM )
        {
            mdi->magic = 0 ;
LOCAL_DEBUG_OUT( "freeing func data %p", mdi->fdata );
            if( mdi->fdata )
            {
                free_func_data( mdi->fdata );
                free( mdi->fdata );
            }
            if( mdi->minipixmap )
                free(mdi->minipixmap);
            if (mdi->item != NULL)
                free (mdi->item);
            if (mdi->item2 != NULL)
                free (mdi->item2);
        }
        free(mdi);
    }
}


void
menu_data_destroy(ASHashableValue value, void *data)
{
    MenuData *md = data ;
LOCAL_DEBUG_CALLER_OUT( "menu_data_destroy(\"%s\", %p)", (char*)value, data );
    if( (char*)value )
        free( (char*)value );
    if( md )
    {
        MenuDataItem *mdi ;
        if( md->magic == MAGIC_MENU_DATA )
        {
            md->magic = 0 ;
            if( md->name != (char*)value )
                free( md->name );

            while( (mdi=md->first) != NULL )
            {
                md->first = mdi->next ;
                mdi->next = NULL ;
                menu_data_item_destroy( mdi );
            }

        }
        free(data);
    }
}

MenuData    *
new_menu_data( ASHashTable *list, char *name )
{
    MenuData *md = NULL ;

    if( name == NULL )
        return NULL ;
    if( list == NULL ) return NULL;

    if( get_hash_item( list, (ASHashableValue)name, (void**)&md) == ASH_Success )
        return md;

    md = (MenuData*) safecalloc (1, sizeof(MenuData));
    md->name = mystrdup(name);
    md->magic = MAGIC_MENU_DATA ;
    if( add_hash_item( list, (ASHashableValue)(md->name), md) != ASH_Success )
    {
        menu_data_destroy( (ASHashableValue)md->name, md );
        md = NULL ;
    }
    return md;
}

MenuData*
find_menu_data( ASHashTable *list, char *name )
{
    MenuData *md = NULL ;

    if( name && list )
        if( get_hash_item( list, AS_HASHABLE(name), (void**)&md) != ASH_Success )
            md = NULL ; /* we are being paranoid */
    return md;
}



MenuDataItem *
new_menu_data_item( MenuData *menu )
{
    MenuDataItem * mdi = NULL;
    if( menu )
    {
        mdi = safecalloc(1, sizeof(MenuDataItem));
		mdi->magic = MAGIC_MENU_DATA_ITEM ;
        mdi->prev = menu->last ;
        if( menu->first == NULL )
            menu->first = mdi ;
        else
            menu->last->next = mdi ;
        menu->last = mdi ;
        ++(menu->items_num);
    }
    return mdi;
}

void
assign_minipixmap( MenuDataItem *mdi, char *minipixmap )
{
#ifndef NO_TEXTURE
    if( mdi && minipixmap )
    {
        mdi->minipixmap = mystrdup( minipixmap );
    }
#endif
}

static void
check_availability( MenuDataItem *mdi )
{
    clear_flags( mdi->flags, MD_Disabled );
#ifndef NO_AVAILABILITYCHECK
    if (mdi->fdata->func == F_EXEC)
    {
        if (!is_executable_in_path (mdi->fdata->text))
        {
            set_flags( mdi->flags, MD_Disabled );
        }
    }
#endif /* NO_AVAILABILITYCHECK */
}

void
add_menu_data_item( MenuData *menu, int func, char *name, char *minipixmap )
{
    MenuDataItem *mdi = new_menu_data_item(menu);

    if( mdi )
    {
        mdi->fdata->func = func ;
        mdi->fdata->name = mystrdup(name);
        check_availability( mdi );
        assign_minipixmap( mdi, minipixmap );
    }
}

void
add_menu_fdata_item( MenuData *menu, FunctionData *fdata, char *minipixmap )
{
    MenuDataItem *mdi ;

    if( fdata )
        if( (mdi = new_menu_data_item(menu)) != NULL )
        {
            copy_func_data( mdi->fdata, fdata);
            memset( fdata, 0x00, sizeof( FunctionData ) );
            check_availability( mdi );
            assign_minipixmap( mdi, minipixmap );
        }
}

/* this is very often used function so we optimize it as good as we can */
int
parse_menu_item_name (MenuDataItem * item, char **name)
{
	register int  i;
	register char *ptr = *name;

	if (ptr == NULL || item == NULL)
		return -1;

	for (i = 0; *ptr && *ptr != '\t'; ptr++, i++);
	if (*ptr == '\0')
	{
		item->item = *name;					   /* avoid memory allocation as much as possible */
		*name = NULL;						   /* that will prevent us from memory deallocation */
		item->item2 = NULL;
	} else
	{
		item->item = mystrndup (*name, i);
		ptr++;
        item->item2 = mystrdup (ptr);
	}
	return 0;
}

void
menu_data_item_from_func (MenuData * menu, FunctionData * fdata)
{
    MenuDataItem     *item = NULL;

    if( fdata == NULL )
        return ;
    if (fdata->func == F_MINIPIXMAP)
	{
		if (menu->last)
			item = menu->last;
		else
		{
            item = new_menu_data_item (menu);
			item->fdata = fdata;
		}
        item->minipixmap = mystrdup(fdata->name);
    } else
    {
        item = new_menu_data_item(menu);
		if (parse_menu_item_name (item, &(fdata->name)) >= 0)
			item->fdata = fdata;
	}
	if( item == NULL || item->fdata != fdata )
	{
		free_func_data (fdata);					   /* insurance measure */
		free( fdata );
	}
}

void
print_func_data(const char *file, const char *func, int line, FunctionData *data)
{
    fprintf( stderr, "%s:%s:%s:%d>!!FUNC ", get_application_name(), file, func, line);
    if( data == NULL )
        fprintf( stderr, "NULL Function\n");
    else
    {
        TermDef      *term = func2fterm (data->func, True);
        if( term == NULL )
            fprintf( stderr, "Invalid Function %d\n", data->func);
        else
        {
            fprintf( stderr, "%s \"%s\" text[%s] ", term->keyword, data->name?data->name:"", data->text?data->text:"" );
            if( data->text == NULL )
            {
                fprintf( stderr, "val0[%ld%c] ", data->func_val[0], (data->unit[0]=='\0')?' ':data->unit[0] );
                fprintf( stderr, "val1[%ld%c] ", data->func_val[1], (data->unit[1]=='\0')?' ':data->unit[1] );
            }
            fprintf( stderr, "(popup=%p)\n", data->popup );
        }
    }
}



