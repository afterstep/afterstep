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

#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>						   /* for Bool, so Xutil.h will be happy */
#include <X11/Xutil.h>						   /* for XSizeHints, so afterstep.h will be happy */

#ifdef I18N
#include <X11/Xlocale.h>
#endif

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/parser.h"
#include "../../include/loadimg.h"
#include "../../include/module.h"

#include "dirtree.h"
#include "menus.h"
#include "globals.h"

#define	NEED_NAME 	(1<<0)
#define	NEED_PATH 	(1<<1)
#define	NEED_WINDOW 	(1<<2)
#define	NEED_WINIFNAME 	(1<<3)
#define	NEED_CMD 	(1<<4)

#define FUNC_TERM(txt,len,func)     	{0,txt,len,TT_TEXT,func,NULL, NULL}
#define FUNC_TERM2(flags,txt,len,func)  {flags,txt,len,TT_TEXT,func,NULL, NULL}

TermDef       FuncTerms[F_FUNCTIONS_NUM + 1] = {
	FUNC_TERM2 (NEED_NAME, "Nop", 3, F_NOP),   /* Nop      "name"|"" */
	FUNC_TERM2 (NEED_NAME, "Title", 5, F_TITLE),	/* Title    "name"    */
	FUNC_TERM ("Beep", 4, F_BEEP),			   /* Beep               */
	FUNC_TERM ("Quit", 4, F_QUIT),			   /* Quit     ["name"] */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "Restart", 7, F_RESTART),	/* Restart "name" WindowManagerName */
	FUNC_TERM ("Refresh", 7, F_REFRESH),	   /* Refresh  ["name"] */
#ifndef NO_VIRTUAL
	FUNC_TERM ("Scroll", 6, F_SCROLL),		   /* Scroll     horiz vert */
	FUNC_TERM ("GotoPage", 8, F_GOTO_PAGE),	   /* GotoPage   x     y    */
	FUNC_TERM ("TogglePage", 10, F_TOGGLE_PAGE),	/* TogglePage ["name"]   */
#endif
	FUNC_TERM ("CursorMove", 10, F_MOVECURSOR),	/* CursorMove horiz vert */
	FUNC_TERM2 (NEED_WINIFNAME, "WarpFore", 8, F_WARP_F),	/* WarpFore ["name" window_name] */
	FUNC_TERM2 (NEED_WINIFNAME, "WarpBack", 8, F_WARP_B),	/* WarpBack ["name" window_name] */
	FUNC_TERM2 (NEED_NAME | NEED_WINDOW, "Wait", 4, F_WAIT),	/* Wait      "name" window_name  */
	FUNC_TERM ("Desk", 4, F_DESK),			   /* Desk arg1 [arg2] */
#ifndef NO_WINDOWLIST
	FUNC_TERM ("WindowList", 10, F_WINDOWLIST),	/* WindowList [arg1 arg2] */
#endif
	FUNC_TERM2 (NEED_NAME, "PopUp", 5, F_POPUP),	/* PopUp    "popup_name" [popup_name] */
	FUNC_TERM2 (NEED_NAME, "Function", 8, F_FUNCTION),	/* Function "function_name" [function_name] */
#ifndef NO_TEXTURE
	FUNC_TERM ("MiniPixmap", 10, F_MINIPIXMAP),	/* MiniPixmap "name" */
#endif
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "Exec", 4, F_EXEC),	/* Exec   "name" command */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "Module", 6, F_MODULE),	/* Module "name" command */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "KillModuleByName", 16, F_KILLMODULEBYNAME),	/* KillModuleByName "name" module */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "QuickRestart", 12, F_QUICKRESTART),	/* QuickRestart "name" what */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "Background", 10, F_CHANGE_BACKGROUND),	/* Background "name" file_name */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "ChangeLook", 10, F_CHANGE_LOOK),	/* ChangeLook "name" file_name */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "ChangeFeel", 10, F_CHANGE_FEEL),	/* ChangeFeel "name" file_name */
	FUNC_TERM2 (TF_SYNTAX_TERMINATOR, "EndFunction", 11, F_ENDFUNC),
	FUNC_TERM2 (TF_SYNTAX_TERMINATOR, "EndPopup", 8, F_ENDPOPUP),

	/* this functions require window as aparameter */
	FUNC_TERM ("&nonsense&", 10, F_WINDOW_FUNC_START),	/* not really a command */
	FUNC_TERM ("Move", 4, F_MOVE),			   /* Move     ["name"] */
	FUNC_TERM ("Resize", 6, F_RESIZE),		   /* Resize   ["name"] */
	FUNC_TERM ("Raise", 5, F_RAISE),		   /* Raise    ["name"] */
	FUNC_TERM ("Lower", 5, F_LOWER),		   /* Lower    ["name"] */
	FUNC_TERM ("RaiseLower", 10, F_RAISELOWER),	/* RaiseLower ["name"] */
	FUNC_TERM ("PutOnTop", 8, F_PUTONTOP),	   /* PutOnTop  */
	FUNC_TERM ("PutOnBack", 9, F_PUTONBACK),   /* PutOnBack */
	FUNC_TERM ("SetLayer", 8, F_SETLAYER),	   /* SetLayer    layer */
	FUNC_TERM ("ToggleLayer", 11, F_TOGGLELAYER),	/* ToggleLayer layer1 layer2 */
	FUNC_TERM ("Shade", 5, F_SHADE),		   /* Shade    ["name"] */
	FUNC_TERM ("Delete", 6, F_DELETE),		   /* Delete   ["name"] */
	FUNC_TERM ("Destroy", 7, F_DESTROY),	   /* Destroy  ["name"] */
	FUNC_TERM ("Close", 5, F_CLOSE),		   /* Close    ["name"] */
	FUNC_TERM ("Iconify", 7, F_ICONIFY),	   /* Iconify  ["name"] value */
	FUNC_TERM ("Maximize", 8, F_MAXIMIZE),	   /* Maximize ["name"] [hori vert] */
	FUNC_TERM ("Stick", 5, F_STICK),		   /* Stick    ["name"] */
	FUNC_TERM ("Focus", 5, F_FOCUS),		   /* Focus */
	FUNC_TERM2 (NEED_WINIFNAME, "ChangeWindowUp", 14, F_CHANGEWINDOW_UP),	/* ChangeWindowUp   ["name" window_name ] */
	FUNC_TERM2 (NEED_WINIFNAME, "ChangeWindowDown", 16, F_CHANGEWINDOW_DOWN),	/* ChangeWindowDown ["name" window_name ] */
	FUNC_TERM ("GetHelp", 7, F_GETHELP),	   /* */
	FUNC_TERM ("PasteSelection", 14, F_PASTE_SELECTION),	/* */
	FUNC_TERM ("WindowsDesk", 11, F_CHANGE_WINDOWS_DESK),	/* WindowDesk "name" new_desk */
	/* end of window functions */
	/* these are commands  to be used only by modules */
	FUNC_TERM ("&nonsense&", 10, F_MODULE_FUNC_START),	/* not really a command */
	FUNC_TERM ("Send_WindowList", 15, F_SEND_WINDOW_LIST),	/* */
	FUNC_TERM ("SET_MASK", 8, F_SET_MASK),	   /* SET_MASK  mask */
	FUNC_TERM2 (NEED_CMD, "SET_NAME", 8, F_SET_NAME),	/* SET_NAME  name */
	FUNC_TERM ("UNLOCK", 6, F_UNLOCK),		   /* UNLOCK    1  */
	FUNC_TERM ("SET_FLAGS", 9, F_SET_FLAGS),   /* SET_FLAGS flags */
	/* these are internal commands */
	FUNC_TERM ("&nonsense&", 10, F_INTERNAL_FUNC_START),	/* not really a command */
	FUNC_TERM ("&raise_it&", 10, F_RAISE_IT),  /* should not be used by user */

	{0, NULL, 0, 0, 0}
};

SyntaxDef     FuncSyntax = {
	' ',
	'\0',
	FuncTerms,
	0,										   /* use default hash size */
	NULL
};

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
		item->item = mystrndup (ptr, item->strlen2);
	}
	return 0;
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
	for (mr = Scr.first_menu; mr != NULL; mr = (*mr).next)
		if (mystrcasecmp (mr->name, name) == 0)
			return mr;

	if (!quiet)
		str_error ("Popup [%s] not defined!\n", name);
	return mr;
}

MenuItem     *
CreateMenuItem ()
{
	MenuItem     *item = safemalloc (sizeof (MenuItem));

	item->prev = NULL;
	item->next = NULL;
	item->item = NULL;
	item->item2 = NULL;
	item->action = NULL;
	item->menu = NULL;
	item->x = 0;
	item->x2 = 0;
	item->y_offset = 0;
	item->y_height = 0;
	item->func = 0;
	item->val1 = 0;
	item->val2 = 0;
	item->val1_unit = 0;
	item->val2_unit = 0;
	item->is_hilited = False;
	item->strlen = 0;
	item->strlen2 = 0;
	item->hotkey = 0;
	item->menu = NULL;
	memset(&(item->icon), 0x00, sizeof(MyIcon));
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

void
DeleteMenuItem (MenuItem * item)
{
	if (item->item != NULL)
		free (item->item);
	if (item->item2 != NULL)
		free (item->item2);
	if (item->action != NULL)
		free (item->action);
	free_icon_resources( item->icon );

	free (item);
}


/***********************************************************************
 *  Procedure:
 *	CreateMenuRoot - allocates and initializes new menu root object
 *  Returned Value:
 *	(MenuRoot *)
 ***********************************************************************/
MenuRoot     *
CreateMenuRoot ()
{
	MenuRoot     *tmp;

	tmp = (MenuRoot *) safemalloc (sizeof (MenuRoot));
	tmp->name = NULL;
	tmp->first = NULL;
	tmp->last = NULL;
	tmp->items = 0;
	tmp->width = 0;
	tmp->width2 = 0;
	tmp->is_mapped = False;
	tmp->is_transient = False;
	tmp->is_pinned = False;
#ifndef NO_TEXTURE
	tmp->titlebg = None;
	tmp->itembg = None;
	tmp->itemhibg = None;
#endif
	tmp->w = None;
	tmp->aw = NULL;
	return (tmp);
}

/***********************************************************************
 *  Procedure:
 *	NewMenuRoot - create a new menu root and attach it to menu tree
 *  Returned Value:
 *	(MenuRoot *)
 *  Inputs:
 *	name	- the name of the menu root
 ***********************************************************************/
MenuRoot     *
NewMenuRoot (char *name)
{
	MenuRoot     *tmp = CreateMenuRoot ();

	tmp->next = Scr.first_menu;
	Scr.first_menu = tmp;
	tmp->name = (name == NULL) ? NULL : mystrdup (name);
	return (tmp);
}

void
DeleteMenuRoot (MenuRoot * menu)
{
	/* unmap if necessary */
	if (menu->is_mapped == True)
		unmap_menu (menu);

	/* remove ourself from the root menu list */
	if (Scr.first_menu == menu)
		Scr.first_menu = menu->next;
	else if (Scr.first_menu != NULL)
	{
		MenuRoot     *ptr;

		for (ptr = Scr.first_menu; ptr->next != NULL; ptr = ptr->next)
			if (ptr->next == menu)
			{
				ptr->next = menu->next;
				break;
			}
	}

	/* kill our children */
	while (menu->first != NULL)
	{
		MenuItem     *item = menu->first;

		menu->first = item->next;
		DeleteMenuItem (item);
	}

#ifndef NO_TEXTURE
	/*  free background pixmaps */
	if (menu->titlebg != None)
		XFreePixmap (dpy, menu->titlebg);
	if (menu->itembg != None)
		XFreePixmap (dpy, menu->itembg);
	if (menu->itemhibg != None)
		XFreePixmap (dpy, menu->itemhibg);
#endif
	if (menu->w != None)
	{
		XDestroyWindow (dpy, menu->w);
		XDeleteContext (dpy, menu->w, MenuContext);
	}

	/* become nameless */
	if (menu->name != NULL)
		free (menu->name);

	/* free our own mem */
	free (menu);
}


void
MenuItemFromFunc (MenuRoot * menu, FunctionData * fdata)
{
	MenuItem     *item;

#ifndef NO_TEXTURE
	if (fdata->func == F_MINIPIXMAP)
	{
		if (menu->last)
			item = menu->last;
		else
		{
			item = NewMenuItem (menu);
			item->func = fdata->func;
		}
		GetIconFromFile (fdata->name, &item->icon, -1);
	} else
#endif /* !NO_TEXTURE */
	{
		item = NewMenuItem (menu);
		if (parse_menu_item_name (item, &(fdata->name)) >= 0)
		{
			item->func = fdata->func;
			item->val1 = fdata->func_val[0];
			item->val2 = fdata->func_val[1];
			item->val1_unit = fdata->unit_val[0];
			item->val2_unit = fdata->unit_val[1];
			item->menu = fdata->popup;
			item->action = fdata->text;
			fdata->text = NULL;
			item->hotkey = fdata->hotkey;
		}
	}

	free_func_data (fdata);					   /* insurance measure */
}

MenuItem     *
MenuItemParse (MenuRoot * menu, const char *buf)
{
	FunctionData  fdata;

	if (buf == NULL)
		return NULL;
	for (; isspace (*buf); buf++);
	if (*buf == '\0' || *buf == '#' || *buf == '*')
		return NULL;

	if (parse_func (buf, &fdata, False) < 0)
		return NULL;
	if (fdata.func == F_ENDPOPUP || fdata.func == F_ENDFUNC)
		return NULL;
#ifndef NO_TEXTURE
	if (fdata.func == F_MINIPIXMAP && MenuMiniPixmaps)
		free_func_data (&fdata);
	else
#endif /* !NO_TEXTURE */
		MenuItemFromFunc (menu, &fdata);
	return menu->last;
}

/****************************************************************************
 *
 *  Processes a menu body definition
 *
 ****************************************************************************/

int
ParseMenuBody (MenuRoot * mr, FILE * fd)
{
	int           success = 0;
	char         *buf;
	register char *ptr;

	buf = safemalloc (MAXLINELENGTH + 1);
	while ((ptr = fgets (buf, MAXLINELENGTH, fd)) != NULL)
	{
		while (isspace (*ptr))
			ptr++;
		if (MenuItemParse (mr, ptr) == NULL)
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
 *
 *  Parses a popup definition
 *
 ****************************************************************************/
void
ParsePopupEntry (char *tline, FILE * fd, char **junk, int *junk2)
{
	MenuRoot     *mr = CreateMenuRoot ();
	static char   screen_init_func[128];
	static char   screen_restart_func[128];
	static int    screen_initialized = 0;

	if (ParseMenuBody (mr, fd))
	{
		MenuRoot     *old;

		mr->name = stripcpy2 (tline, 0);
		/* first let's remove old menu with the same name from main tree */
		for (old = Scr.first_menu; old != NULL; old = old->next)
			if (strcmp (mr->name, old->name) == 0)
			{
				DeleteMenuRoot (old);
				break;
			}
		/* now let's attach us to the main tree */
		mr->next = Scr.first_menu;
		Scr.first_menu = mr;

		if (screen_initialized == 0)
		{
			sprintf (screen_init_func, "InitScreen%ldFunction", Scr.screen);
			sprintf (screen_restart_func, "RestartScreen%ldFunction", Scr.screen);
			screen_initialized = 1;
		}

		if (strcmp (mr->name, "InitFunction") == 0 || strcmp (mr->name, screen_init_func) == 0)
			Scr.InitFunction = mr;
		else if (strcmp (mr->name, "RestartFunction") == 0 ||
				 strcmp (mr->name, screen_restart_func) == 0)
			Scr.RestartFunction = mr;
	} else
		free (mr);
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
	FunctionData  fdata;

	while (isspace (*tline))
		tline++;
	while (isdigit (*tline))
	{
		button = button * 10 + (int)((*tline) - '0');
		tline++;
	}

	tline = parse_context (tline, &contexts, win_contexts);
	tline = parse_context (tline, &mods, key_modifiers);
	if (parse_func (tline, &fdata, False) < 0)
		return;

	if ((contexts & C_WINDOW) && (((mods == 0) || mods == AnyModifier)))
		Scr.buttons2grab &= ~(1 << (button - 1));

	temp = Scr.MouseButtonRoot;
	Scr.MouseButtonRoot = (MouseButton *) safemalloc (sizeof (MouseButton));
	Scr.MouseButtonRoot->NextButton = temp;

	Scr.MouseButtonRoot->func = fdata.func;
	Scr.MouseButtonRoot->menu = fdata.popup;
	Scr.MouseButtonRoot->action = fdata.text;
	fdata.text = NULL;

	Scr.MouseButtonRoot->Button = button;
	Scr.MouseButtonRoot->Context = contexts;
	Scr.MouseButtonRoot->Modifier = mods;
	Scr.MouseButtonRoot->val1 = fdata.func_val[0];
	Scr.MouseButtonRoot->val2 = fdata.func_val[1];
	Scr.MouseButtonRoot->val1_unit = fdata.unit_val[0];
	Scr.MouseButtonRoot->val2_unit = fdata.unit_val[1];
	free_func_data (&fdata);				   /* no longer need name and action text */
}

/****************************************************************************
 *
 *  Processes a line with a key binding
 *
 ****************************************************************************/

void
ParseKeyEntry (char *tline, FILE * fd, char **junk, int *junk2)
{
	char         *name;
	KeySym        keysym;
	KeyCode       keycode;
	int           contexts, mods;
	FunctionData  fdata;
	int           i, min, max;

	tline = parse_token (tline, &name);
	tline = parse_context (tline, &contexts, win_contexts);
	tline = parse_context (tline, &mods, key_modifiers);

	if (parse_func (tline, &fdata, False) < 0)
		return;
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

			tmp->next = Scr.FuncKeyRoot;
			Scr.FuncKeyRoot = tmp;

			tmp->name = name;
			name = NULL;
			tmp->keycode = i;
			tmp->cont = contexts;
			tmp->mods = mods;
			tmp->func = fdata.func;
			tmp->action = fdata.text;
			fdata.text = NULL;
			tmp->val1 = fdata.func_val[0];
			tmp->val2 = fdata.func_val[1];
			tmp->val1_unit = fdata.unit_val[0];
			tmp->val2_unit = fdata.unit_val[1];
			tmp->menu = fdata.popup;
		}
	}
	free_func_data (&fdata);				   /* no longer need name and action text */
	if (name)
		free (name);
}

  /****************************************************************************
 * generates menu from directory tree
 ****************************************************************************/

 /* we assume buf is at least MAXLINELENGTH bytes */

MenuRoot     *
dirtree_make_menu2 (dirtree_t * tree, char *buf)
{
/*  extern struct config* func_config; */
	dirtree_t    *t;
	MenuRoot     *menu;
	FunctionData  fdata;

	init_func_data (&fdata);
	/* make self */
	if (tree->flags & DIRTREE_KEEPNAME)
		menu = NewMenuRoot (tree->name);
	else
	{
		sprintf (buf, "%d", tree->flags & DIRTREE_ID);
		menu = NewMenuRoot (buf);
	}

	/* make title */
	fdata.func = F_TITLE;
	fdata.name = mystrdup (tree->name);
	/* We exploit that scan_for_hotkey removes & (marking hotkey) from name */
	scan_for_hotkey (fdata.name);
	MenuItemFromFunc (menu, &fdata);
#ifndef NO_TEXTURE
	if (MenuMiniPixmaps)
	{
		fdata.func = F_MINIPIXMAP;
		fdata.name = mystrdup (tree->icon != NULL ? tree->icon : "mini-menu.xpm");
		MenuItemFromFunc (menu, &fdata);
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
			fdata.func = F_POPUP;
			fdata.name = mystrdup (t->name);
			if ((fdata.popup = dirtree_make_menu2 (t, buf)) == NULL)
				fdata.func = F_NOP;

			fdata.hotkey = scan_for_hotkey (fdata.name);
			if (t->flags & DIRTREE_KEEPNAME)
				fdata.text = mystrdup (t->name);
			else
				fdata.text = string_from_int (t->flags & DIRTREE_ID);

			MenuItemFromFunc (menu, &fdata);
#ifndef NO_TEXTURE
			if (MenuMiniPixmaps)
			{
				fdata.func = F_MINIPIXMAP;
				fdata.name = mystrdup (t->icon != NULL ? t->icon : "mini-folder.xpm");
				MenuItemFromFunc (menu, &fdata);
			}
#endif /* !NO_TEXTURE */
		} else if (t->command.func != F_NOP)
		{
			fdata.func = t->command.func;
			fdata.name = mystrdup (t->name);
			if (t->command.text)
			{
				fdata.text =
					NEW_ARRAY (char, strlen (t->command.text) + 1 + 2 * strlen (t->path) + 1);
				sprintf (fdata.text, "%s %s", t->command.text, t->path);
				/* quote the string so the shell doesn't mangle it */
				if (t->command.func == F_EXEC)
					quotestr (fdata.text + strlen (t->command.text) + 1, t->path,
							  2 * strlen (t->path) + 1);
			} else
				fdata.text = mystrdup (t->path);
			MenuItemFromFunc (menu, &fdata);

#ifndef NO_TEXTURE
			if (MenuMiniPixmaps && t->icon != NULL)
			{
				fdata.func = F_MINIPIXMAP;
				fdata.name = mystrdup (t->icon);
				MenuItemFromFunc (menu, &fdata);
			}
#endif /* !NO_TEXTURE */
		} else
		{
			FILE         *fp2 = fopen (t->path, "r");

			/* try to load a command */
			if (fp2 != NULL && fgets (buf, MAXLINELENGTH, fp2) != NULL)
			{
				if (parse_func (buf, &fdata, True) < 0)
				{							   /* data is actuall shell command line */
					fdata.func = F_EXEC;
					fdata.name = mystrdup (t->name);
					fdata.text = stripcpy (buf);
				}
			} else
			{
				fdata.func = F_EXEC;
				fdata.name = mystrdup (t->name);
				fdata.text = mystrdup (t->name);
			}
#ifndef NO_AVAILABILITYCHECK
			if (fdata.func == F_EXEC)
				if (!is_executable_in_path (fdata.text))
					fdata.func = F_NOP;
#endif /* NO_AVAILABILITYCHECK */
			MenuItemFromFunc (menu, &fdata);

#ifndef NO_TEXTURE
			/* check for a MiniPixmap */
			if (MenuMiniPixmaps)
			{
				int           parsed = 0;

				if (fp2 != NULL && fgets (buf, MAXLINELENGTH, fp2) != NULL)
				{
					if (parse_func (buf, &fdata, True) >= 0)
						parsed = (fdata.func == F_MINIPIXMAP);
				}
				if (t->icon != NULL && parsed == 0)
				{
					free_func_data (&fdata);
					fdata.func = F_MINIPIXMAP;
					fdata.name = mystrdup (t->icon);
					parsed = 1;
				}
				if (parsed)
					MenuItemFromFunc (menu, &fdata);
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
		if (cur->func == F_TITLE)
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
		if (cur->func == F_POPUP)
			width += 15;
		if (cur->hotkey)
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
		if (cur->func == F_TITLE)
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
		} else if (cur->func == F_NOP && cur->item != NULL && *cur->item == 0)
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

	return;
}

/* this will run command received from module */
int
RunCommand (char *cmd, int channel, Window w)
{
	int           Context;
	ASWindow     *tmp_win;
	FunctionData  fdata;
	int           toret = 0;
	extern module_t *Module;

/*fprintf( stderr, "Module command received: %s\n", cmd);
 */
	if (parse_func (cmd, &fdata, False) < 0)
		return toret;
/*fprintf( stderr,"Function parsed: [%s] [%s] [%d] [%d] [%c]\n",fdata.name,fdata.text,fdata.func_val[0], fdata.func_val[1] );
 */
	switch (fdata.func)
	{
	 case F_SET_MASK:
		 Module[channel].mask = fdata.func_val[0];
		 break;
	 case F_SET_NAME:
		 if (Module[channel].name != NULL)
			 free (Module[channel].name);
		 Module[channel].name = fdata.text;
		 fdata.text = NULL;
		 break;
	 case F_UNLOCK:
		 toret = 66;
		 break;
	 case F_SET_FLAGS:
		 {
			 int           xorflag;
			 Bool          update = False;

			 if (XFindContext (dpy, w, ASContext, (caddr_t *) & tmp_win) == XCNOENT)
				 break;
			 xorflag = tmp_win->hints->flags ^ fdata.func_val[0];
			 /*if (xorflag & STICKY)
			    Stick (tmp_win); */
			 if (xorflag & AS_SkipWinList)
			 {
				 tmp_win->hints->flags ^= AS_SkipWinList;
				 update_windowList ();
				 update = True;
			 }
			 if (xorflag & AS_AvoidCover)
			 {
				 tmp_win->hints->flags ^= AS_AvoidCover;
				 update = True;
			 }
			 if (xorflag & AS_Transient)
			 {
				 tmp_win->hints->flags ^= AS_Transient;
				 update = True;
			 }
			 if (xorflag & AS_DontCirculate)
			 {
				 tmp_win->hints->flags ^= AS_DontCirculate;
				 update = True;
			 }
			 if (update)
				 BroadcastConfig (M_CONFIGURE_WINDOW, tmp_win);
			 break;
		 }
	 default:
		 {
			 Event.xany.window = w;
			 if (XFindContext (dpy, w, ASContext, (caddr_t *) & tmp_win) == XCNOENT)
			 {
				 tmp_win = NULL;
				 w = None;
				 Event.xbutton.x_root = 0;
				 Event.xbutton.y_root = 0;
			 } else
			 {
				 Event.xbutton.x_root = tmp_win->frame_x;
				 Event.xbutton.y_root = tmp_win->frame_y;
			 }
			 Event.xany.type = ButtonRelease;
			 Event.xbutton.button = 1;
			 Event.xbutton.x = 0;
			 Event.xbutton.y = 0;
			 Event.xbutton.subwindow = None;
			 Context = GetContext (tmp_win, &Event, &w);
			 ExecuteFunction (fdata.func, fdata.text, w, tmp_win, &Event, Context,
							  fdata.func_val[0], fdata.func_val[1],
							  fdata.unit_val[0], fdata.unit_val[1], fdata.popup, channel);
		 }
	}
	free_func_data (&fdata);
	return toret;
}
