/*
 * Copyright (c) 2002 Jan Fedak <jack at mobil dot cz>
 * Copyright (c) 1998,2003 Sasha Vasko <sasha at aftercode.net>
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

#undef LOCAL_DEBUG

#include "../../configure.h"

#include "asinternals.h"
#include "../../libAfterStep/desktop_category.h"

#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "dirtree.h"

/***************************************************************************/
/*                      Menu functions                                     */
/***************************************************************************/
MenuData     *
FindPopup (const char *name, int quiet)
{
    MenuData     *md = NULL;

	if (name == NULL)
	{
		if (!quiet)
            show_error ("Empty Popup name specifyed");
        return md;
	}

    md = find_menu_data( Scr.Feel.Popups, (char*)name );
    if (!quiet && md == NULL )
        show_error ("Popup \"%s\" not defined!", name);
    return md;
}

/***********************************************************************
 *  Procedure:
 *  CreateMenuData - allocates and initializes new menu root object
 *  Returned Value:
 *  (MenuData *)
 ***********************************************************************/
MenuData     *
CreateMenuData (char *name)
{
	MenuData *md;
    if( Scr.Feel.Popups == NULL )
        init_list_of_menus(&(Scr.Feel.Popups), True);
    md = new_menu_data( Scr.Feel.Popups, name );
	md->recent_items = Scr.Feel.recent_submenu_items ;
	return md;
}

void print_func_data(const char *file, const char *func, int line, FunctionData *data);

Bool
MenuDataItemParse (void *data, const char *buf)
{
    MenuData *menu = (MenuData*)data;
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
        MenuDataItemFromFunc (menu, fdata);
        fdata = NULL ;
        return True;
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
        if (!IsMinipixmapFunc(fdata.func))
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
    MenuData     *mr = CreateMenuData (name);
    free( name );
    ParseBody(mr, fd, MenuDataItemParse);
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

    tline = parse_win_context_str (tline, &contexts);
    tline = parse_modifier_str (tline, &mods);
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
    Scr.Feel.MouseButtonRoot = (MouseButton *) safecalloc (1, sizeof (MouseButton));
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
    tline = parse_win_context_str (tline, &contexts);
    tline = parse_modifier_str (tline, &mods);

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
			int m ;
			XDisplayKeycodes (dpy, &min, &max);
			for (m = 0; m <= 8; m++)   /* is 8 really the maximum number of modifiers to search through? */
			{
				for (i = min; i <= max; i++)
					if (XKeycodeToKeysym (dpy, i, m) == keysym)
						break;
			  	if (i <= max)
			    	break;
			}
	
			if (i <= max)
				keycode = i ; 

			if (keycode != 0 )
			{
				FuncKey      *tmp = (FuncKey *) safecalloc (1, sizeof (FuncKey));

                tmp->next = Scr.Feel.FuncKeyRoot;
                Scr.Feel.FuncKeyRoot = tmp;

				tmp->name = name;
				name = NULL;
				tmp->keycode = keycode;
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

 /* we assume buf is at least MAXLINELENGTH bytes */

void 
add_minipixmap_fro_dirtree_item( dirtree_t * tree, MenuData *menu )
{
	FunctionData *fdata = NULL ;
	if( tree->de != NULL && tree->de->fulliconname != NULL ) 
        fdata = create_named_function( F_SMALL_MINIPIXMAP, tree->de->fulliconname);
    else if (tree->icon != NULL) /* should default to: "mini-menu.xpm" */
        fdata = create_named_function(  get_flags(tree->flags, DIRTREE_ICON_IS_SMALL)?F_SMALL_MINIPIXMAP:F_MINIPIXMAP, 
										tree->icon);
	if( fdata ) 
		MenuDataItemFromFunc (menu, fdata);
}	 

MenuData     *
dirtree_make_menu2 (dirtree_t * tree, char *buf, Bool reload_submenus)
{
/*  extern struct config* func_config; */
	dirtree_t    *t;
    MenuData     *menu;
	FunctionData *fdata;

	/* make self */
	if (tree->flags & DIRTREE_KEEPNAME)
        menu = CreateMenuData (tree->name);
	else
	{
		sprintf (buf, "%d", tree->flags & DIRTREE_ID);
        menu = CreateMenuData (buf);
	}
	if( get_flags( tree->flags, DIRTREE_RECENT_ITEMS_SET ) )
		menu->recent_items = tree->recent_items ;

	/* make title */
	fdata = create_named_function( F_TITLE, tree->name);
	/* We exploit that scan_for_hotkey removes & (marking hotkey) from name */
	scan_for_hotkey (fdata->name);
    MenuDataItemFromFunc (menu, fdata);

	add_minipixmap_fro_dirtree_item( tree, menu );

	for (t = tree->child; t != NULL; t = t->next)
	{
		if (t->flags & DIRTREE_MINIPIXMAP)
			continue;
		if (t->flags & DIRTREE_DIR)
		{
/************* Creating Popup Title entry : ************************/
			fdata = create_named_function(F_POPUP, t->stripped_name);
            if( reload_submenus )
                fdata->popup = dirtree_make_menu2 (t, buf, reload_submenus);
            else
                fdata->popup = FindPopup( t->stripped_name, True );
/*            if( fdata->popup == NULL )
                fdata->func = F_NOP;
*/
			fdata->hotkey = scan_for_hotkey (fdata->name);
			if (t->flags & DIRTREE_KEEPNAME)
				fdata->text = mystrdup (t->stripped_name);
			else
				fdata->text = string_from_int (t->flags & DIRTREE_ID);

            MenuDataItemFromFunc (menu, fdata);

 			add_minipixmap_fro_dirtree_item( t, menu );
/************* Done creating Popup Title entry : ************************/
		} else if( t->de ) 
		{
			if( t->de->type == ASDE_TypeApplication ) 
			{	
				if( get_flags( t->de->flags, ASDE_Terminal ) )
					fdata = create_named_function(F_ExecInTerm, t->stripped_name);	   
				else if( get_flags( t->de->flags, ASDE_ASModule ) )
					fdata = create_named_function(F_MODULE, t->stripped_name);	   
				else
		 			fdata = create_named_function(F_EXEC, t->stripped_name);	
            	fdata->text = mystrdup( t->de->clean_exec );
            	MenuDataItemFromFunc (menu, fdata);
 				add_minipixmap_fro_dirtree_item( t, menu );
			}
		}else if (t->command.func != F_NOP)
		{
			fdata = create_named_function(t->command.func, t->stripped_name);
			if (t->command.text)
			{
                int cmd_len = strlen (t->command.text);
                int path_len = strlen (t->path);
                fdata->text = NEW_ARRAY (char, cmd_len + 1 + 2 * path_len + 1);
				sprintf (fdata->text, "%s %s", t->command.text, t->path);
				/* quote the string so the shell doesn't mangle it */
				if (t->command.func == F_EXEC)
                    quotestr (fdata->text + cmd_len + 1, t->path, 2 * path_len + 1);
			} else
				fdata->text = mystrdup (t->path);
            MenuDataItemFromFunc (menu, fdata);

 			add_minipixmap_fro_dirtree_item( t, menu );
        } else
		{
			FILE *fp2 = fopen (t->path, "r");
			int lines_read = 0 ;

			fdata = NULL ; 
			if( fp2 != NULL  )
			{
				Bool available = True ;
				char *name = NULL ;
				FunctionData *valid_func = NULL ; 
				FunctionData *minipixmap = NULL ;
				/* try to load a command */
				while( fgets (buf, MAXLINELENGTH, fp2) != NULL)
				{
					int parse_err ; 
					++lines_read;
					if( fdata == NULL ) 
						fdata = create_named_function(F_EXEC, NULL);
					parse_err = parse_func (buf, fdata, True);
					if ( parse_err < 0 && parse_err > FUNC_ERR_START ) /* data is actuall shell command line */
						fdata->text = stripcpy (buf);
					if( fdata->name == NULL )
		                fdata->name = mystrdup( t->stripped_name );
#ifndef NO_AVAILABILITYCHECK
					if ( fdata->func!= F_ExecInTerm && (IsSwallowFunc(fdata->func) || IsExecFunc(fdata->func)))
					{	
						available = is_executable_in_path (fdata->text);
						if( !available )
							fdata->func = F_NOP;
					}
#endif					
					if( IsMinipixmapFunc(fdata->func) )
					{
						if( minipixmap == NULL ) 
						{
							minipixmap = fdata ;
							fdata = NULL ; 
						}
					}else if( fdata->func != F_NOP ) 
					{
						if( valid_func == NULL ) 
						{
							valid_func = fdata ; 
							fdata = NULL ; 
						}					
					}
					
					if( fdata ) 
					{
						if( name == NULL && fdata->name != NULL ) 
						{
							name = fdata->name ;
							fdata->name = NULL ;
						}
						destroy_func_data( &fdata );
					}
				}
				if( available || get_flags(tree->flags, DIRTREE_SHOW_UNAVAILABLE))
				{	
					if( valid_func  ) 
						MenuDataItemFromFunc (menu, valid_func);
					else
					{
						fdata = create_named_function(F_NOP, name?name:t->stripped_name);
	  					fdata->text = mystrdup (t->name);
	          			MenuDataItemFromFunc (menu, fdata);
					}
					if( minipixmap ) 
						MenuDataItemFromFunc (menu, minipixmap);
					else if( t->icon != NULL )
					{
						fdata = create_named_function(F_MINIPIXMAP, t->icon);
	          			MenuDataItemFromFunc (menu, fdata);
					}
				}else
				{
					if( valid_func )
						destroy_func_data(&valid_func);
					if( minipixmap )
						destroy_func_data(&minipixmap);
				}
				if( name ) 
					free( name );
			}
			
			if( fp2 == NULL || lines_read == 0 )
			{
				Bool available = True ;
				fdata = create_named_function(F_EXEC, t->stripped_name);
				fdata->text = mystrdup (t->name);
#ifndef NO_AVAILABILITYCHECK
				available = is_executable_in_path (fdata->text);
				if( !available )		   
					fdata->func = F_NOP;
#endif			
				if( available || get_flags(tree->flags, DIRTREE_SHOW_UNAVAILABLE))
				{	
					MenuDataItemFromFunc (menu, fdata);
					if( t->icon != NULL )
					{
						fdata = create_named_function(F_MINIPIXMAP, t->icon);
	          			MenuDataItemFromFunc (menu, fdata);
					}
				}else if( fdata )
					destroy_func_data(&fdata);					   /* insurance measure */
			}
            
            if (fp2)
				fclose (fp2);
		}
	}
	return menu;
}

