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

#define LOCAL_DEBUG

#include "../../configure.h"

#include "asinternals.h"

#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../../libAfterStep/loadimg.h"

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
    if( Scr.Feel.Popups == NULL )
        init_list_of_menus(&(Scr.Feel.Popups), True);
    return new_menu_data( Scr.Feel.Popups, name );
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

	/* make title */
	fdata = create_named_function( F_TITLE, tree->name);
	/* We exploit that scan_for_hotkey removes & (marking hotkey) from name */
	scan_for_hotkey (fdata->name);
    MenuDataItemFromFunc (menu, fdata);
    if (tree->icon != NULL)
    {/* should default to: "mini-menu.xpm" */
        fdata = create_named_function( F_MINIPIXMAP, tree->icon);
        MenuDataItemFromFunc (menu, fdata);
	}

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
            if( reload_submenus )
                fdata->popup = dirtree_make_menu2 (t, buf, reload_submenus);
            else
                fdata->popup = FindPopup( t->name, True );
/*            if( fdata->popup == NULL )
                fdata->func = F_NOP;
*/
			fdata->hotkey = scan_for_hotkey (fdata->name);
			if (t->flags & DIRTREE_KEEPNAME)
				fdata->text = mystrdup (t->name);
			else
				fdata->text = string_from_int (t->flags & DIRTREE_ID);

            MenuDataItemFromFunc (menu, fdata);
            if (t->icon)
			{
                /* should default to "mini-folder.xpm" */
                fdata = create_named_function( F_MINIPIXMAP, t->icon);
                MenuDataItemFromFunc (menu, fdata);
			}
/************* Done creating Popup Title entry : ************************/
		} else if (t->command.func != F_NOP)
		{
			fdata = create_named_function(t->command.func, t->name);
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

            if (t->icon != NULL)
            {  /* no defaults !!! */
				fdata = create_named_function(F_MINIPIXMAP, t->icon);
                MenuDataItemFromFunc (menu, fdata);
			}
        } else
		{
			FILE         *fp2 = fopen (t->path, "r");

			/* try to load a command */
            fdata = create_named_function(F_EXEC, NULL);
            if (fp2 != NULL && fgets (buf, MAXLINELENGTH, fp2) != NULL)
			{
				if (parse_func (buf, fdata, True) < 0) /* data is actuall shell command line */
					fdata->text = stripcpy (buf);
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
            MenuDataItemFromFunc (menu, fdata);
LOCAL_DEBUG_OUT( "3:fdata->name = %p\"%s\"", fdata->name, fdata->name );
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
                    MenuDataItemFromFunc (menu, fdata);
                else
                {
                    free_func_data(fdata);
                    free(fdata);
                }
			}
            if (fp2)
				fclose (fp2);
		}
	}
	return menu;
}

#if 0
/****************************************************************************
 * Generates the window for a menu
 ****************************************************************************/
void
MakeMenu (MenuData * mr)
{
    MenuDataItem     *cur;
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
                (cur->item == NULL) ? 0 : XTextWidth ((*Scr.MSMenuDataItem).font.font, cur->item,
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
        width = XTextWidth ((*Scr.MSMenuDataItem).font.font, cur->item2, cur->strlen2);


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
            cur->y_height = (*Scr.MSMenuDataItem).font.height + HEIGHT_EXTRA + 2;
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
}
#endif


