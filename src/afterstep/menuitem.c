/*
 * Copyright (c) 2002 Jan Fedak <jack at mobil dot cz>
 * Copyright (c) 1998 Sasha Vasko <sasha at aftercode.net>
 * spawned from configure.c and module.c using code from various ppl :
 * Copyright (c) 1998 Rafal Wierzbicki <rafal@mcss.mcmaster.ca>
 * Copyright (c) 1998 Michal Vitecek <fuf@fuf.sh.cvut.cz>
 * Copyright (c) 1998 Nat Makarevitch <nat@linux-france.com>
 * Copyright (c) 1998 Mike Venaccio <venaccio@aero.und.edu>
 * Copyright (c) 1998 Ethan Fischer <allanon@crystaltokyo.com>
 * Copyright (c) 1998 Mike Venaccio <venaccio@aero.und.edu>
 * Copyright (c) 1998 Chris Ridd <c.ridd@isode.com>
 * Copyright (c) 1997 Raphael Goulais <velephys@hol.fr>
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (C) 1996 Frank Fejes
 * Copyright (C) 1995 Bo Yang
 * Copyright (C) 1993 Robert Nation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/****************************************************************************
 *
 * Configure.c: reads the configuration files, interprets them,
 * and sets up menus, bindings, colors, and fonts as specified
 *
 ***************************************************************************/

#include "../../configure.h"
#define LOCAL_DEBUG
#include "../../include/asapp.h"
#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/screen.h"
#include "../../include/parser.h"
#include "../../include/loadimg.h"
#include "../../include/module.h"
#include "../../include/decor.h"

#include "dirtree.h"
#include "asinternals.h"

/* The keys must be in lower case! */
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
			error_point ();
			fprintf (stderr, "undefined context specifyer [%c] encountered in [%s].", *ptr, string);
		}
	}
	return ptr;
}

void
init_func_data (FunctionData * data)
{
	int           i;

	data->func = F_NOP;
	for (i = 0; i < MAX_FUNC_ARGS; i++)
	{
		data->func_val[i] = 0;
		data->unit_val[i] = 0;
		data->unit[i] = '\0';
	}
	data->hotkey = '\0';
	data->name = data->text = NULL;
	data->popup = NULL;
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

void
parse_func_units (FunctionData * data)
{
	int           i;
	int           defaults[MAX_FUNC_ARGS] = { Scr.MyDisplayWidth,
		Scr.MyDisplayHeight
	};

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

void
free_func_hash ()
{
	if (FuncSyntax.term_hash)
	{
		free (FuncSyntax.term_hash);
		FuncSyntax.term_hash = NULL;
	}
}

TermDef      *
txt2fterm (const char *txt, int quiet)
{
	TermDef      *fterm;

	if (FuncSyntax.term_hash == NULL)
	{
		InitHash (&FuncSyntax);
		BuildHash (&FuncSyntax);
	}
	if ((fterm = FindStatementTerm ((char *)txt, &FuncSyntax)) == NULL && !quiet)
		str_error ("unknown function name in function specification [%s].\n", txt);

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
			str_error ("empty function specification encountered.%s\n", "");
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

	if (data->func == F_MAXIMIZE)
		set_func_val (data, -1, 100);

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
					str_error ("impaired doublequotes encountered in [%s].\n", ptr);
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

	parse_func_units (data);
	data->hotkey = scan_for_hotkey (data->name);

	/* now let's check for valid number of arguments */
	if ((fterm->flags & NEED_NAME) && data->name == NULL)
	{
		str_error ("function specification requires \"name\" in [%s].\n", text);
		return -4;
	}
	if (data->text == NULL)
	{
		if ((fterm->flags & NEED_WINDOW) || ((fterm->flags & NEED_WINIFNAME) && data->name != NULL))
		{
			str_error ("function specification requires window name in [%s].\n", text);
			return -5;
		}
		if (fterm->flags & NEED_CMD)
		{
			str_error ("function specification requires shell command or full file name in [%s].\n",
					   text);
			return -6;
		}
	}
	if ((data->func == F_POPUP) || (data->func == F_FUNCTION))
	{
		if ((data->popup = FindPopup (data->name, (data->text != NULL))) == NULL)
			if (data->text)
				data->popup = FindPopup (data->text, False);

		if (data->popup == NULL)
			data->func = F_NOP;
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

int
free_func_data (FunctionData * data)
{
    if( data )
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
    return 0;
}

FunctionData *
String2Func ( const char *string, FunctionData *p_fdata, Bool quiet )
{
    if( p_fdata )
        free_func_data( p_fdata );
    else
        p_fdata = safecalloc( 1, sizeof(FunctionData));

    if( parse_func( string, p_fdata, quiet ) <= 0 )
    {
        free_func_data( p_fdata );
        free( p_fdata );
        p_fdata = NULL;
    }
    return p_fdata;
}

/***************************************************************************/
/*                      Menu functions                                     */
/***************************************************************************/

/* this is very often used function so we optimize it as good as we can */
int
parse_menu_item_name (MenuItem * item, char **name)
{
	register int  i;
	register char *ptr = *name;

	if (ptr == NULL || item == NULL)
		return -1;

	for (i = 0; *ptr && *ptr != '\t'; ptr++, i++);
	item->strlen = i;
	if (*ptr == '\0')
	{
		item->item = *name;					   /* avoid memory allocation as much as possible */
		*name = NULL;						   /* that will prevent us from memory deallocation */
		item->item2 = NULL;
		item->strlen2 = 0;
	} else
	{
		item->item = mystrndup (*name, i);
		ptr++;
		item->strlen2 = strlen (ptr);
        item->item2 = mystrndup (ptr, item->strlen2);
	}
	return 0;
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
 * Implementing create-destroy functions for menus:
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
#ifndef NO_TEXTURE
            if( mdi->minipixmap )
                free(mdi->minipixmap);
#endif
            if (mdi->item != NULL)
                free (mdi->item);
            if (mdi->item2 != NULL)
                free (mdi->item2);
            free_icon_resources( mdi->icon );
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

#if 0
            /* unmap if necessary */
            if (md->is_mapped == True)
                unmap_menu (md);
#ifndef NO_TEXTURE
            /*  free background pixmaps */
            if (md->titlebg != None)
                XFreePixmap (dpy, md->titlebg);
            if (md->itembg != None)
                XFreePixmap (dpy, md->itembg);
            if (md->itemhibg != None)
                XFreePixmap (dpy, md->itemhibg);
#endif
            if (md->w != None)
            {
                XDestroyWindow (dpy, md->w);
            }
#endif

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

void
init_list_of_menus(ASHashTable **list, Bool force)
{
    if( list == NULL ) return ;

    if( force && *list != NULL )
        destroy_ashash( list );

    if( *list == NULL )
        *list = create_ashash( 0, casestring_hash_value,
                                  casestring_compare,
                                  menu_data_destroy);
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

MenuRoot     *
FindPopup (char *name, int quiet)
{
	MenuRoot     *mr = NULL;

	if (name == NULL)
	{
		if (!quiet)
			str_error ("%s\n", "Empty Popup name specifyed!");
		return mr;
	}

    mr = find_menu_data( Scr.Feel.Popups, name );
    if (!quiet && mr == NULL )
		str_error ("Popup [%s] not defined!\n", name);
	return mr;
}

MenuItem     *
CreateMenuItem ()
{
	MenuItem     *item = safecalloc (1, sizeof (MenuItem));
    item->magic = MAGIC_MENU_DATA_ITEM ;
	return item;
}

MenuItem     *
NewMenuItem (MenuRoot * menu)
{
	MenuItem     *item = CreateMenuItem ();

	item->prev = menu->last;
	if (menu->first == NULL)
		menu->first = item;
	else
		menu->last->next = item;
	menu->last = item;

	item->item_num = menu->items++;
	return item;
}

/***********************************************************************
 *  Procedure:
 *	CreateMenuRoot - allocates and initializes new menu root object
 *  Returned Value:
 *	(MenuRoot *)
 ***********************************************************************/
MenuRoot     *
CreateMenuRoot (char *name)
{
    if( Scr.Feel.Popups == NULL )
        init_list_of_menus(&(Scr.Feel.Popups), True);
    return new_menu_data( Scr.Feel.Popups, name );
}

void
MenuItemFromFunc (MenuRoot * menu, FunctionData * fdata)
{
	MenuItem     *item = NULL;

    if( fdata == NULL )
        return ;
#ifndef NO_TEXTURE
	if (fdata->func == F_MINIPIXMAP)
	{
		if (menu->last)
			item = menu->last;
		else
		{
			item = NewMenuItem (menu);
			item->fdata = fdata;
		}
		GetIconFromFile (fdata->name, &item->icon, -1);
	} else
#endif /* !NO_TEXTURE */
	{
		item = NewMenuItem (menu);
		if (parse_menu_item_name (item, &(fdata->name)) >= 0)
			item->fdata = fdata;
	}
	if( item == NULL || item->fdata != fdata )
	{
		free_func_data (fdata);					   /* insurance measure */
		free( fdata );
	}
}

Bool
MenuItemParse (void *data, const char *buf)
{
    MenuRoot *menu = (MenuRoot*)data;
	FunctionData  *fdata;

	if (buf == NULL)
        return False;
	for (; isspace (*buf); buf++);
	if (*buf == '\0' || *buf == '#' || *buf == '*')
        return False;

	fdata = safecalloc( 1, sizeof(FunctionData));
	if (parse_func (buf, fdata, False) < 0)
        return False;
	if (fdata->func != F_ENDPOPUP && fdata->func != F_ENDFUNC)
	{
#ifndef NO_TEXTURE
        if (fdata->func != F_MINIPIXMAP && !get_flags( Scr.Look.flags, MenuMiniPixmaps))
#endif /* !NO_TEXTURE */
		{
			MenuItemFromFunc (menu, fdata);
			fdata = NULL ;
            return True;
		}
	}
	if( fdata )
	{
		free_func_data (fdata);
		free( fdata );
	}
    return False ;
}

Bool
FunctionItemParse (void *data, const char *buf)
{
    ComplexFunction *func = (ComplexFunction*)data;
    FunctionData  fdata;

	if (buf == NULL)
        return False;
	for (; isspace (*buf); buf++);
	if (*buf == '\0' || *buf == '#' || *buf == '*')
        return False;

    init_func_data( &fdata );
    if (parse_func (buf, &fdata, False) < 0)
        return False;
    if (fdata.func != F_ENDPOPUP && fdata.func != F_ENDFUNC)
	{
        if (fdata.func != F_MINIPIXMAP)
        {
            func->items = realloc(func->items, sizeof(FunctionData)*(func->items_num+1));
            func->items[func->items_num] = fdata ;
            ++(func->items_num);
            return True;
		}
	}
    free_func_data (&fdata);
    return False ;
}

/****************************************************************************
 *
 *  Processes a menu body definition
 *
 ****************************************************************************/

int
ParseBody (void *data, FILE * fd, Bool (*item_func)(void *, const char*))
{
	int           success = 0;
	char         *buf;
	register char *ptr;

	buf = safemalloc (MAXLINELENGTH + 1);
	while ((ptr = fgets (buf, MAXLINELENGTH, fd)) != NULL)
	{
		while (isspace (*ptr))
			ptr++;
        if (!item_func(data, ptr))
			if (mystrncasecmp ("End", ptr, 3) == 0)
			{
				success = 1;
				break;
			}
	}
	free (buf);
	return success;
}

/****************************************************************************
 *  Parses a popup definition
 ****************************************************************************/
void
ParsePopupEntry (char *tline, FILE * fd, char **junk, int *junk2)
{
    char         *name =  stripcpy2 (tline, 0);
    MenuRoot     *mr = CreateMenuRoot (name);
    free( name );
    ParseBody(mr, fd, MenuItemParse);
}

/****************************************************************************
 *  Parses a popup definition
 ****************************************************************************/
void
ParseFunctionEntry (char *tline, FILE * fd, char **junk, int *junk2)
{
    char         *name =  stripcpy2 (tline, 0);
    ComplexFunction  *func = NULL ;

    if( Scr.Feel.ComplexFunctions == NULL )
        init_list_of_funcs(&(Scr.Feel.ComplexFunctions), True);

    func = new_complex_func( Scr.Feel.ComplexFunctions, name);
    free( name );

    ParseBody(func, fd, FunctionItemParse);
}

/****************************************************************************
 *
 *  Parses a mouse binding
 *
 ****************************************************************************/
void
ParseMouseEntry (char *tline, FILE * fd, char **junk, int *junk2)
{
	MouseButton  *temp;
	int           button = 0;
	int           contexts, mods;
	FunctionData  *fdata;

	while (isspace (*tline))
		tline++;
	while (isdigit (*tline))
	{
		button = button * 10 + (int)((*tline) - '0');
		tline++;
	}

	tline = parse_context (tline, &contexts, win_contexts);
	tline = parse_context (tline, &mods, key_modifiers);
	fdata = safecalloc( 1, sizeof(FunctionData) );
	if (parse_func (tline, fdata, False) < 0)
	{
		free_func_data (fdata);				   /* just in case */
		free(fdata);
		return;
	}
	if ((contexts & C_WINDOW) && (((mods == 0) || mods == AnyModifier)))
        Scr.Feel.buttons2grab &= ~(1 << (button - 1));

    temp = Scr.Feel.MouseButtonRoot;
    Scr.Feel.MouseButtonRoot = (MouseButton *) safemalloc (sizeof (MouseButton));
    Scr.Feel.MouseButtonRoot->NextButton = temp;

    Scr.Feel.MouseButtonRoot->fdata = fdata;

    Scr.Feel.MouseButtonRoot->Button = button;
    Scr.Feel.MouseButtonRoot->Context = contexts;
    Scr.Feel.MouseButtonRoot->Modifier = mods;
}

/****************************************************************************
 *
 *  Processes a line with a key binding
 *
 ****************************************************************************/

void
ParseKeyEntry (char *tline, FILE * fd, char **junk, int *junk2)
{
	char         *name = NULL;
	KeySym        keysym;
	KeyCode       keycode;
	int           contexts, mods;
	FunctionData  *fdata;
	int           i, min, max;

	tline = parse_token (tline, &name);
	tline = parse_context (tline, &contexts, win_contexts);
	tline = parse_context (tline, &mods, key_modifiers);

	fdata = safecalloc( 1, sizeof(FunctionData));
    if (parse_func (tline, fdata, False) >= 0)
	{
		/*
		 * Don't let a 0 keycode go through, since that means AnyKey to the
		 * XGrabKey call in GrabKeys().
		 */
		keysym = XStringToKeysym (name);
		if (keysym != NoSymbol && (keycode = XKeysymToKeycode (dpy, keysym)) != 0)
		{
			XDisplayKeycodes (dpy, &min, &max);
			for (i = min; i <= max; i++)
				if (XKeycodeToKeysym (dpy, i, 0) == keysym)
					break;

			if (i <= max)
			{
				FuncKey      *tmp = (FuncKey *) safemalloc (sizeof (FuncKey));

                tmp->next = Scr.Feel.FuncKeyRoot;
                Scr.Feel.FuncKeyRoot = tmp;

				tmp->name = name;
				name = NULL;
				tmp->keycode = i;
				tmp->cont = contexts;
				tmp->mods = mods;
				tmp->fdata = fdata;
				fdata = NULL ;
			}
		}
	}
	if( fdata )
	{
		free_func_data (fdata);				   /* just in case */
		free( fdata );
	}
	if (name)
		free (name);
}

  /****************************************************************************
 * generates menu from directory tree
 ****************************************************************************/

inline FunctionData *
create_named_function( int func, char *name)
{
	FunctionData *fdata = safecalloc( 1, sizeof(FunctionData) );
	init_func_data (fdata);
	fdata->func = func;
    if( name )
        fdata->name = mystrdup (name);
	return fdata ;
}

 /* we assume buf is at least MAXLINELENGTH bytes */

MenuRoot     *
dirtree_make_menu2 (dirtree_t * tree, char *buf)
{
/*  extern struct config* func_config; */
	dirtree_t    *t;
	MenuRoot     *menu;
	FunctionData *fdata;

	/* make self */
	if (tree->flags & DIRTREE_KEEPNAME)
        menu = CreateMenuRoot (tree->name);
	else
	{
		sprintf (buf, "%d", tree->flags & DIRTREE_ID);
        menu = CreateMenuRoot (buf);
	}

	/* make title */
	fdata = create_named_function( F_TITLE, tree->name);
	/* We exploit that scan_for_hotkey removes & (marking hotkey) from name */
	scan_for_hotkey (fdata->name);
	MenuItemFromFunc (menu, fdata);
#ifndef NO_TEXTURE
    if (get_flags( Scr.Look.flags, MenuMiniPixmaps))
	{
		fdata = create_named_function( F_MINIPIXMAP,
		                               tree->icon != NULL ? tree->icon : "mini-menu.xpm");
		MenuItemFromFunc (menu, fdata);
	}
#endif /* !NO_TEXTURE */

	for (t = tree->child; t != NULL; t = t->next)
	{
		if (t->parent != NULL && t->parent->extension != NULL)
		{
			int           nlen = strlen (t->name);
			int           elen = strlen (t->parent->extension);

			if (nlen >= elen)
			{
				if (!strcmp (t->name + nlen - elen, t->parent->extension))
					t->name[nlen - elen] = '\0';
				else if (!strncmp (t->name, t->parent->extension, elen))
					memmove (t->name, t->name + elen, nlen - elen + 1);
			}
		}
		if (t->flags & DIRTREE_DIR)
		{
/************* Creating Popup Title entry : ************************/
			fdata = create_named_function(F_POPUP, t->name);
			if ((fdata->popup = dirtree_make_menu2 (t, buf)) == NULL)
				fdata->func = F_NOP;

			fdata->hotkey = scan_for_hotkey (fdata->name);
			if (t->flags & DIRTREE_KEEPNAME)
				fdata->text = mystrdup (t->name);
			else
				fdata->text = string_from_int (t->flags & DIRTREE_ID);

			MenuItemFromFunc (menu, fdata);
#ifndef NO_TEXTURE
            if (get_flags( Scr.Look.flags, MenuMiniPixmaps))
			{
				fdata = create_named_function( F_MINIPIXMAP, t->icon != NULL ? t->icon : "mini-folder.xpm");
				MenuItemFromFunc (menu, fdata);
			}
#endif /* !NO_TEXTURE */
/************* Done creating Popup Title entry : ************************/
		} else if (t->command.func != F_NOP)
		{
			fdata = create_named_function(t->command.func, t->name);
			if (t->command.text)
			{
				fdata->text =
					NEW_ARRAY (char, strlen (t->command.text) + 1 + 2 * strlen (t->path) + 1);
				sprintf (fdata->text, "%s %s", t->command.text, t->path);
				/* quote the string so the shell doesn't mangle it */
				if (t->command.func == F_EXEC)
					quotestr (fdata->text + strlen (t->command.text) + 1, t->path,
							  2 * strlen (t->path) + 1);
			} else
				fdata->text = mystrdup (t->path);
			MenuItemFromFunc (menu, fdata);

#ifndef NO_TEXTURE
            if (get_flags( Scr.Look.flags, MenuMiniPixmaps) && t->icon != NULL)
			{
				fdata = create_named_function(F_MINIPIXMAP, t->icon);
				MenuItemFromFunc (menu, fdata);
			}
#endif /* !NO_TEXTURE */
		} else
		{
			FILE         *fp2 = fopen (t->path, "r");

			/* try to load a command */
            fdata = create_named_function(F_EXEC, NULL);
            if (fp2 != NULL && fgets (buf, MAXLINELENGTH, fp2) != NULL)
			{
				if (parse_func (buf, fdata, True) < 0) /* data is actuall shell command line */
					fdata->text = stripcpy (buf);
				else
					fdata->func = F_NOP ;
			} else
				fdata->text = mystrdup (t->name);
            if( fdata->name == NULL )
                fdata->name = mystrdup( t->name );
#ifndef NO_AVAILABILITYCHECK
			if (fdata->func == F_EXEC)
				if (!is_executable_in_path (fdata->text))
					fdata->func = F_NOP;
#endif /* NO_AVAILABILITYCHECK */
LOCAL_DEBUG_OUT( "1:fdata->name = \"%s\"", t->name );
LOCAL_DEBUG_OUT( "2:fdata->name = %p\"%s\"", fdata->name, fdata->name );
			MenuItemFromFunc (menu, fdata);
LOCAL_DEBUG_OUT( "3:fdata->name = %p\"%s\"", fdata->name, fdata->name );
#ifndef NO_TEXTURE
			/* check for a MiniPixmap */
            if (get_flags( Scr.Look.flags, MenuMiniPixmaps))
			{
				int           parsed = 0;

                fdata = create_named_function(F_MINIPIXMAP, NULL);
				if (fp2 != NULL && fgets (buf, MAXLINELENGTH, fp2) != NULL)
				{
					if (parse_func (buf, fdata, True) >= 0)
						parsed = (fdata->func == F_MINIPIXMAP);
				}
                if (t->icon != NULL && !parsed)
				{
					free_func_data (fdata);
					fdata->func = F_MINIPIXMAP;
					fdata->name = mystrdup (t->icon);
					parsed = 1;
				}
				if (parsed)
					MenuItemFromFunc (menu, fdata);
                else
                {
                    free_func_data(fdata);
                    free(fdata);
                }
			}
#endif /* !NO_TEXTURE */
			if (fp2)
				fclose (fp2);
		}
	}
	return menu;
}

/****************************************************************************
 *
 * Generates the window for a menu
 *
 ****************************************************************************/

void
MakeMenu (MenuRoot * mr)
{
#if 0
    MenuItem     *cur;
	unsigned long valuemask = (CWEventMask | CWCursor);
	XSetWindowAttributes attributes;
	int           width, y, t;
	XSizeHints    hints;
	Bool          was_pinned = False;

	/* lets first size the window accordingly */
	mr->width = 0;
	for (cur = mr->first; cur != NULL; cur = cur->next)
	{
		/* calculate item width */
        if (cur->fdata->func == F_TITLE)
		{
			int           d = 0;

			width =
				(cur->item == NULL) ? 0 : XTextWidth ((*Scr.MSMenuTitle).font.font, cur->item,
													  cur->strlen);

#ifndef NO_TEXTURE
			if (Scr.MenuPinOn.pix != None)
				if (d < 5 + Scr.MenuPinOn.width + 1)
					d = 5 + Scr.MenuPinOn.width + 1;
			if (Scr.MenuPinOff.pix != None)
				if (d < 5 + Scr.MenuPinOff.width + 1)
					d = 5 + Scr.MenuPinOff.width + 1;
#endif /* !NO_TEXTURE */
			width += d;
		} else
		{
			width =
				(cur->item == NULL) ? 0 : XTextWidth ((*Scr.MSMenuItem).font.font, cur->item,
													  cur->strlen);
			t = (cur->item == NULL) ? 0 : XTextWidth ((*Scr.MSMenuHilite).font.font, cur->item,
													  cur->strlen);

			if (width < t)
				width = t;
		}

		if ((*cur).icon.pix != None)
			width += (*cur).icon.width + 5;
        if (cur->fdata->func == F_POPUP)
			width += 15;
        if (cur->fdata->hotkey)
			width += 15;
		if (width <= 0)
			width = 1;
		if (width > mr->width)
			mr->width = width;

		t = XTextWidth ((*Scr.MSMenuHilite).font.font, cur->item2, cur->strlen2);
		width = XTextWidth ((*Scr.MSMenuItem).font.font, cur->item2, cur->strlen2);


		if (width < t)
			width = t;
		if (width < 0)
			width = 0;
		if (width > mr->width2)
			mr->width2 = width;
		if ((width == 0) && (cur->strlen2 > 0))
			mr->width2 = 1;

		/* calculate item height */
        if (cur->fdata->func == F_TITLE)
		{
			/* Title */
			cur->y_height = (*Scr.MSMenuTitle).font.height + HEIGHT_EXTRA + 2;
#ifndef NO_TEXTURE
			if (Scr.MenuPinOn.pix != None)
				if ((*cur).y_height < Scr.MenuPinOn.height + 3)
					(*cur).y_height = Scr.MenuPinOn.height + 3;
			if (Scr.MenuPinOff.pix != None)
				if ((*cur).y_height < Scr.MenuPinOff.height + 3)
					(*cur).y_height = Scr.MenuPinOff.height + 3;
#endif /* !NO_TEXTURE */
        } else if (cur->fdata->func == F_NOP && cur->item != NULL && *cur->item == 0)
		{
			/* Separator */
			cur->y_height = HEIGHT_SEPARATOR;
		} else
		{
			/* Normal text entry */
			cur->y_height = (*Scr.MSMenuItem).font.height + HEIGHT_EXTRA + 2;
			t = (*Scr.MSMenuHilite).font.height + HEIGHT_EXTRA + 2;
			if (cur->y_height < t)
				cur->y_height = t;
		}
		if ((*cur).icon.pix != None)
			if ((*cur).y_height < (*cur).icon.height + 3)
				(*cur).y_height = (*cur).icon.height + 3;
	}
	mr->width += 15;
	if (mr->width2 > 0)
		mr->width += 5;

	/* now place the items */
	for (y = 0, cur = mr->first; cur != NULL; cur = cur->next)
	{
		cur->y_offset = y;
		cur->x = 5;
		if ((*cur).icon.pix != None)
			cur->x += (*cur).icon.width + 5;
		if (mr->width2 == 0)
		{
			cur->x2 = cur->x;
		} else
		{
			cur->x2 = mr->width - 5;
		}
		y += cur->y_height;
	}
	if ((*mr).is_mapped == True && (*mr).is_pinned == True)
		was_pinned = True;
	if ((*mr).is_mapped == True)
		unmap_menu (mr);
	mr->height = y;

	/* free resources */
#ifndef NO_TEXTURE
	if ((*mr).titlebg != None)
		XFreePixmap (dpy, (*mr).titlebg);
	if ((*mr).itembg != None)
		XFreePixmap (dpy, (*mr).itembg);
	if ((*mr).itemhibg != None)
		XFreePixmap (dpy, (*mr).itemhibg);
	(*mr).titlebg = None;
	(*mr).itembg = None;
	(*mr).itemhibg = None;
#endif
	if ((*mr).w != None)
	{
		XDestroyWindow (dpy, (*mr).w);
		XDeleteContext (dpy, (*mr).w, MenuContext);
	}

	attributes.event_mask = (ExposureMask | EnterWindowMask);
	attributes.cursor = Scr.ASCursors[MENU];
#ifndef NO_SAVEUNDERS
	attributes.save_under = TRUE;
	valuemask |= CWSaveUnder;
#endif
	mr->width = mr->width + mr->width2;
/*  if( mr->width <= 0 ) mr->width = 20 ;
   if( mr->width >= Scr.MyDisplayWidth ) mr->width = Scr.MyDisplayWidth-20 ;
   if( mr->height <= 0 ) mr->height = 20 ;
   if( mr->height >= Scr.MyDisplayHeight ) mr->height = Scr.MyDisplayHeight-20 ;
 */
	if (mr->width <= 0)
		mr->width = 2;
	if (mr->height <= 0)
		mr->height = 2;

	mr->w = create_visual_window (Scr.asv, Scr.Root, 0, 0, mr->width,
								  mr->height, 0, InputOutput, valuemask, &attributes);
	/* allow us to set a menu's position before calling AddWindow */
	hints.flags = USPosition;
	XSetNormalHints (dpy, mr->w, &hints);
	XSaveContext (dpy, mr->w, MenuContext, (caddr_t) mr);

	/* if this menu is supposed to be mapped, map it */
	if (was_pinned == True)
	{
		setup_menu_pixmaps (mr);
		map_menu (mr, (*mr).context);
		pin_menu (mr);
	}
#endif
}


