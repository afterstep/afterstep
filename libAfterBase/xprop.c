/*
 * Copyright (C) 2000 Sasha Vasko <sashav@sprintmal.com>
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

#include "../configure.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xproto.h>
#include <X11/Xmd.h>

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/screen.h"
#include "../include/parse.h"
#include "../include/xprop.h"

/* X property access : */
Bool
intern_atom_list (AtomXref * list)
{
	int           nitems = 0, i = 0;
	char        **names;
	Atom         *atoms;
	Bool          res = False;

	if (list)
	{
		for (i = 0; list[i].name != NULL; i++);
		nitems = i;
		if (nitems > 0)
		{
			names = (char **)safemalloc (sizeof (char *) * nitems);

			atoms = (Atom *) safemalloc (sizeof (Atom) * nitems);
			memset (atoms, 0x00, sizeof (Atom) * nitems);
			for (i = 0; i < nitems; i++)
				names[i] = list[i].name;

			res = (XInternAtoms (dpy, names, nitems, False, atoms) != 0);

			for (i = 0; i < nitems; i++)
			{
				list[i].atom = atoms[i];
				*(list[i].variable) = atoms[i];
			}
			free (atoms);
			free (names);
		}
	}
	return res;
}

void
translate_atom_list (ASFlagType *trg, AtomXref * xref, Atom * list, long nitems)
{
	if (trg && list && xref && nitems > 0)
	{
		register int  i;
		register AtomXref *curr;

		for (i = 0; i < nitems && list[i] != None; i++)
			for (curr = xref; curr->atom != None; curr++)
				if (curr->atom == list[i])
				{
					set_flags (*trg, curr->flag);
					break;
				}
	}
}

void
print_list_hints( stream_func func, void* stream, ASFlagType flags, AtomXref *xref, const char *prompt )
{
  register int i ;
    if( !pre_print_check( &func, &stream, (void*)flags, NULL ) ) return ;

    func( stream, "%s.flags = 0x%X;\n", prompt, flags );
    for( i = 0 ; xref[i].name ; i++ )
        if( get_flags(flags, xref[i].flag) )
            func( stream, "%s.atom[%d] = %s;\n", prompt, i, xref[i].name );
}

Bool
read_32bit_proplist (Window w, Atom property, long estimate, CARD32 ** list, long *nitems)
{
	Bool          res = False;

	if (w != None && property != None && list && nitems)
	{
		Atom          actual_type;
		int           actual_format;
        ASFlagType bytes_after;

		if (estimate <= 0)
			estimate = 1;

		res =
			(XGetWindowProperty
			 (dpy, w, property, 0, estimate, False, AnyPropertyType,
			  &actual_type, &actual_format, nitems, &bytes_after, (unsigned char **)list) == 0);
		/* checking property sanity */
		res = (res && *nitems > 0 && actual_format == 32);

		if (bytes_after > 0 && res)
		{
			XFree (*list);
			res =
				(XGetWindowProperty
				 (dpy, w, property, 0, estimate + (bytes_after >> 2), False,
				  actual_type, &actual_type, &actual_format, nitems, &bytes_after, (unsigned char **)list) == 0);

			res = (res && (*nitems > 0));	   /* bad property */
		}

		if (!res)
		{
			if (*list && *nitems > 0)
				XFree (*list);
			*nitems = 0;
			*list = NULL;
		}
	}
	return res;
}

Bool read_text_property (Window w, Atom property, XTextProperty ** trg)
{
	Bool          res = False;

	if (w != None && property != None && trg)
	{
		if (*trg)
		{
			if ((*trg)->value)
				XFree ((*trg)->value);
		} else
			*trg = (XTextProperty *) safemalloc (sizeof (XTextProperty));

		if (XGetTextProperty (dpy, w, *trg, property) == 0)
		{
			free ((*trg));
			*trg = NULL;
		} else
			res = True;
	}
	return res;
}

void
print_text_property( stream_func func, void* stream, XTextProperty *tprop, const char *prompt )
{
    if( pre_print_check( &func, &stream, tprop, NULL ) )
    {
        func( stream, "%s.value = \"%s\";\n", prompt, tprop->value );
        func( stream, "%s.format = %d;\n", prompt, tprop->format );
    }
}

void
free_text_property (XTextProperty ** trg)
{
    if( *trg )
    {
        if( (*trg)->value ) XFree ((*trg)->value);
        free( *trg );
        *trg = NULL ;
    }
}

Bool read_32bit_property (Window w, Atom property, CARD32 * trg)
{
	Bool          res = False;

	if (w != None && property != None && trg)
	{
		Atom          actual_type;
		int           actual_format;
        ASFlagType bytes_after;
		CARD32       *data;
		long          nitems;

		res =
			(XGetWindowProperty
			 (dpy, w, property, 0, 1, False, AnyPropertyType, &actual_type,
			  &actual_format, &nitems, &bytes_after, (unsigned char **)&data) == 0);

		/* checking property sanity */
		res = (res && nitems > 0 && actual_format == 32);

		if (res)
			*trg = *data;
		if (data && nitems > 0)
			XFree (data);
	}
	return res;
}
/**** conversion routines taking in consideration different encoding of X properties ****/
char *
text_property2string( XTextProperty *tprop)
{
    char *text = NULL ;
    if( tprop )
    {
        if (tprop->value)
		{
#ifdef I18N
            if (tprop->encoding != XA_STRING)
			{
                char  **list;
                int     num, res ;

                tprop->nitems = strlen (tprop->value);
                res = XmbTextPropertyToTextList (dpy, &tprop, &list, &num);
                if ( res >= Success && num > 0 && *list)
                    text = stripcpy( *list );
                if ( res >= Success && *list)
                    XFreeStringList( list );
            }
#endif
            if( text == NULL && *(tprop->value) )
                text = stripcpy(tprop->value);
        }
    }
    return text;
}

/*************************************************************************/
/* Writing properties here :                                             */
/*************************************************************************/
void
set_32bit_property (Window w, Atom property, Atom type, CARD32 data)
{
    if (w != None && property != None )
	{
        XChangeProperty (dpy, Scr.Root, property, type?type:XA_CARDINAL, 32,
                         PropModeReplace, (unsigned char *)&data, 1);
    }
}

void
set_multi32bit_property (Window w, Atom property, Atom type, int items, ...)
{
    if (w != None && property != None )
	{
        if( items > 0 )
        {
            CARD32 *data = safemalloc( items*sizeof(CARD32));
            register int i = 0;
            va_list ap;

            va_start(ap,items);
            while( i < items )
                data[i++] = va_arg(ap,CARD32);
            va_end(ap);

            XChangeProperty (dpy, Scr.Root, property, type?type:XA_CARDINAL, 32,
                             PropModeReplace, (unsigned char *)&data, items);
        }else
        {
            XChangeProperty (dpy, Scr.Root, property,
                             type?type:XA_CARDINAL, 32, PropModeReplace, NULL, 0);
        }
    }
}


