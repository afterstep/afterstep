/*
 * Copyright (c) 2000 Sasha Vasko <sashav@sprintmail.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xmd.h>

#include "config.h"
#include "astypes.h"
#include "audit.h"
#include "mystring.h"
#include "safemalloc.h"
#include "xwrap.h"
#include "parse.h"

/****************************************************************************
 * parse_argb_color - should be used for all your color parsing needs
 ***************************************************************************/
const char *parse_argb_color( const char *color, CARD32 *pargb )
{
#define hextoi(h)   (isdigit(h)?((h)-'0'):(isupper(h)?((h)-'A'+10):((h)-'a'+10)))
	if( color )
	{
		if( *color == '#' )
		{
			CARD32 argb = 0 ;
			int len = 0 ;
			register const char *ptr = color+1 ;
			while( isxdigit(ptr[len]) ) len++;
			if( len >= 3)
			{
				if( (len&0x3) == 0 )
				{  /* we do have alpha channel !!! */
					len = len>>2 ;
					argb = (hextoi(ptr[0])<<28)&0xF0000000 ;
					if( len > 1 )
						argb |= (hextoi(ptr[1])<<24)&0x0F000000 ;
					else
						argb |= 0x0F000000;
					ptr += len ;
				}else
				{
					len = len/3 ;
					argb = 0xFF000000;
				}
				/* processing rest of the channels : */
				if( len == 1 )
				{
					argb |= 0x000F0F0F;
					argb |= (hextoi(ptr[0])<<20)&0x00F00000 ;
					argb |= (hextoi(ptr[1])<<12)&0x0000F000 ;
					argb |= (hextoi(ptr[2])<<4 )&0x000000F0 ;
					ptr += 3 ;
				}else
				{
					argb |= (hextoi(ptr[0])<<20)&0x00F00000 ;
					argb |= (hextoi(ptr[1])<<16)&0x000F0000 ;
					ptr += len ;
					argb |= (hextoi(ptr[0])<<12)&0x0000F000 ;
					argb |= (hextoi(ptr[1])<<8) &0x00000F00 ;
					ptr += len ;
					argb |= (hextoi(ptr[0])<<4 )&0x000000F0 ;
					argb |= (hextoi(ptr[1]))    &0x0000000F ;
					ptr += len ;
				}
				*pargb = argb ;
				return ptr;
			}
		}else if( *color )
		{
			XColor xcol, xcol_scr ;
			register const char *ptr = &(color[0]);
			/* does not really matter here what screen to use : */
			if( XLookupColor( dpy, DefaultColormap(dpy,DefaultScreen(dpy)), color, &xcol, &xcol_scr) )
				*pargb = 0xFF000000|((xcol.red<<8)&0x00FF0000)|(xcol.green&0x0000FF00)|((xcol.blue>>8)&0x000000FF);
			while( !isspace(*ptr) && *ptr != '\0' ) ptr++;
			return ptr;
		}
	}
	return color;
}

/****************************************************************************
 * Some usefull parsing functions
 ****************************************************************************/
char         *
find_doublequotes (char *ptr)
{
	if (ptr)
	{
		if (*ptr == '"')
			ptr++;

		if (*ptr != '"')
		{
			while ((ptr = strchr (ptr, '"')))
				if (*(ptr - 1) == '\\')
					ptr++;
				else
					break;
		}
	}
	return ptr;
}

/****************************************************************************
 * Copies a string into a new, malloc'ed string
 * Strips leading and trailing whitespace
 ****************************************************************************/

char         *
stripcpy (const char *source)
{
	const char   *ptr;

	for (; isspace (*source); source++);
	for (ptr = source + strlen (source); ptr > source && isspace (*(ptr - 1)); ptr--);
	return mystrndup (source, ptr - source);
}


/****************************************************************************
 *
 * Copies a string into a new, malloc'ed string
 * Strips all data before the first quote and after the second
 *
 ****************************************************************************/

char         *
stripcpy2 (const char *source, int tab_sensitive)
{
	char         *ptr = (char *)source, *tail;

	if (*ptr != '"')
		ptr = find_doublequotes (ptr);
	if (ptr != NULL)
	{
		if ((tail = find_doublequotes (ptr)) != NULL)
			return mystrndup (ptr + 1, tail - ptr - 1);
		return mystrdup (ptr + 1);
	}
	return mystrndup (source, 0);
}


/* here we'll strip comments and skip whitespaces */
char         *
stripcomments (char *source)
{
	register char *ptr;

	/* remove comments from the line */
	while (isspace (*source))
		source++;
	for (ptr = (char *)source; *ptr; ptr++)
	{
		if (*ptr == '"')
		{
			if ((ptr = find_doublequotes (ptr)) == NULL)
			{
				ptr = source + strlen (source);
				break;
			}
		}
		if (*ptr == '#')
		{									   /* checking if it is not a hex color code */
			int           i;

			for (i = 1; isxdigit (*(ptr + i)); i++);

			if (i < 4 || i > 13 || (*(ptr + i) && !isspace (*(ptr + i))))
			{
				for (ptr--; ptr > source && isspace (*ptr); ptr--);
				*(ptr + 1) = '\0';
				/* we'll exit the loop automagically */
			} else
				ptr += i - 1;
		}
	}
	for (ptr--; isspace (*ptr) && ptr > source; ptr--);
	*(ptr + 1) = '\0';
	return source;
}

char         *
strip_whitespace (char *str)
{
	char         *ptr;

	/* strip trailing whitespace */
	for (ptr = str + strlen (str); ptr > str && isspace (*(ptr - 1)); ptr--)
		*(ptr - 1) = '\0';
	/* skip leading whitespace */
	for (ptr = str; isspace (*ptr); ptr++);
	return ptr;
}


/****************************************************************************
 *
 * Copies a whitespace separated token into a new, malloc'ed string
 * Strips leading and trailing whitespace
 *
 ****************************************************************************/

char         *
tokencpy (const char *source)
{
	const char   *ptr;

	for (; isspace (*source); source++);
	for (ptr = source; !isspace (*ptr) && *ptr; ptr++);
	return mystrndup (source, ptr - source);
}

char *
tokenskip( char *start, unsigned int n_tokens )
{
    register int    i ;
    register char   *cur = start ;

    if( cur == NULL ) return cur ;
    for (i = 0; i < n_tokens; i++)
	{
		while (!isspace (*cur) && *cur)
        {   /* we have to match doublequotes if we encounter any, to allow for spaces in filenames */
			if (*cur == '"')
			{
				register char *ptr = find_doublequotes (cur);

				if (ptr)
					while (cur != ptr)
						cur++;
			}
			cur++;
		}

		while (isspace (*cur) && *cur)
			cur++;
		if (*cur == '\0')
			break;
	}
    return cur;
}

/****************************************************************************
 *
 * Matches text from config to a table of strings.
 *
 ****************************************************************************/

struct config *
find_config (struct config *table, const char *text)
{
	for (; strlen (table->keyword); table++)
		if (!mystrncasecmp (text, table->keyword, strlen (table->keyword)))
			return table;

	return NULL;
}

/****************************************************************************
 * copy src to dest, backslash-escaping all non-alphanumeric characters in
 * src; write a maximum of maxlen chars to dest, including terminating zero
 ****************************************************************************/
int
quotestr (char *dest, const char *src, int maxlen)
{
	int           n = maxlen;

	/* require at least enough space for the terminating zero */
	if (maxlen < 1)
		return maxlen - n;
	n--;
	while (n && *src)
	{
		if (!isalnum (*src) && n > 1)
		{
			*dest++ = '\\';
			n--;
		}
		*dest++ = *src++;
		n--;
	}
	*dest = '\0';
	return maxlen - n;
}

/****************************************************************************
 *
 * Copies a whitespace separated token into a new, malloc'ed string
 * Strips leading and trailing whitespace
 * returns pointer to the end of token in source string
 ****************************************************************************/

char         *
parse_token (const char *source, char **trg)
{
	char         *ptr;

	for (; isspace (*source); source++);
	for (ptr = (char *)source; !isspace (*ptr) && *ptr; ptr++);
	*trg = mystrndup (source, ptr - source);
	return ptr;
}

char         *
parse_tab_token (const char *source, char **trg)
{
	char         *ptr;

	for (; isspace (*source); source++);
	for (ptr = (char *)source; *ptr != '\t' && *ptr; ptr++);
	*trg = mystrndup (source, ptr - source);
	return ptr;
}

char         *
parse_filename (const char *source, char **trg)
{
	for (; isspace (*source); source++);

	if (*source == '"')
	{
		if ((*trg = stripcpy2 (source, 0)) != NULL)
			source += strlen (*trg) + 2;
	} else
		source = parse_token (source, trg);
	return (char *)source;
}

/* used in ParseMouse and in communication with modules */
char         *
parse_func_args (char *tline, char *unit, int *func_val)
{
	int           sign = 0;

	while (isspace (*tline))
		tline++;
	*func_val = 0;
	if (*tline == '-')
		sign = -1;
	else if (*tline == '+')
		sign = 1;
	if (sign != 0)
		tline++;
	while (isdigit (*tline))
	{
		*func_val = (*func_val) * 10 + (int)((*tline) - '0');
		tline++;
	}
	if (*tline && !isspace (*tline))
		*unit = *tline;
	else
		*unit = '\0';
	if (sign != 0)
		*func_val *= sign;
	return tline;
}

char         *
string_from_int (int param)
{
	char         *mem;
	register int  i;
	int           neg = 0;

	if (param == 0)
		return mystrdup ("0");
	if (param < 0)
	{
		param = -param;
		neg = 1;
	}
	for (i = 1; param >> (i * 3); i++);
	if (neg)
		i++;
	mem = safemalloc (i + 1);
	if (neg)
		mem[0] = '-';
	sprintf (&(mem[neg]), "%u", param);
	return mem;
}

static char _as_hex_to_char_table[] = "0123456789ABCDEF";

char         *
hex_to_buffer_reverse(void *data, size_t bytes, char* buffer)
{

    char *p = buffer;
    unsigned char *d = data;
    register int   i = bytes, k = 0 ;

    if( data == NULL || buffer == NULL )
        return buffer ;

    while( i > 0 )
    {
        i-- ;
        p[k++] = _as_hex_to_char_table[d[i]>>4];
        p[k++] = _as_hex_to_char_table[d[i]&0x0F];
    }
    return p+k;
}

char         *
hex_to_buffer(void *data, size_t bytes, char* buffer)
{
    char *p = buffer;
    unsigned char *d = data;
    register int   i = bytes, k = 0 ;

    if( data == NULL || buffer == NULL )
        return buffer ;

    while( i < bytes )
    {
        p[k++] = _as_hex_to_char_table[d[i]>>4];
        p[k++] = _as_hex_to_char_table[d[i]&0x0F];
        i++ ;
    }
    return p+k;
}

/***********************************************************************
 * Procedure:
 *	scanForHotkeys - Look for hotkey markers in a function "name"
 * Inputs:
 *	txt	- string to scan
 * Output:      - hotkey found
 ***********************************************************************/
char
scan_for_hotkey (char *txt)
{
	char          hotkey = '\0';

	if (txt != NULL)
		for (; *txt != '\0'; txt++)			   /* Scan whole string                 */
			if (*txt == '&')
			{								   /* A hotkey marker?                  */
				char         *tmp;			   /* Copy the string down over it      */

				for (tmp = txt; *tmp != '\0'; tmp++)
					*tmp = *(tmp + 1);
				if (*txt != '&')			   /* Not Just an escaped &            */
					hotkey = *txt;
			}
	return hotkey;
}


/* Conversion of the coma separated values into a list of strings */
char         *
get_comma_item (char *ptr, char **item_start, char **item_end)
{

	if (ptr == NULL)
		return NULL;
	while (*ptr && (isspace (*ptr) || *ptr == ','))
		ptr++;
	if (*ptr == '\0')
		return NULL;

	*item_end = *item_start = ptr;
	if (*ptr == '"')
	{
		if ((*item_end = find_doublequotes (ptr)) == NULL)
			return NULL;
		ptr = *item_end;
		while (*ptr && !(isspace (*ptr) || *ptr == ','))
			ptr++;
	} else
	{
		while (*ptr && *ptr != ',')
			ptr++;
		*item_end = ptr;
	}
	return ptr;
}

char        **
comma_string2list (char *string)
{
	register char *ptr;
	int           items;
	char        **list = NULL, *item_start = NULL, *item_end = NULL;

	if (string == NULL)
		return NULL;

	ptr = string;
	items = 0;
	while (*ptr)
	{
		if ((ptr = get_comma_item (ptr, &item_start, &item_end)) == NULL)
			break;
		items++;
	}
	if (items > 0)
	{
		register int  i;

		list = (char **)safemalloc (sizeof (char *) * (items + 1));
		memset (list, 0x00, sizeof (char *) * (items + 1));

		ptr = string;
		for (i = 0; i < items; i++)
		{
			if ((ptr = get_comma_item (ptr, &item_start, &item_end)) == NULL)
				break;
			list[i] = mystrndup (item_start, (item_end - item_start));
		}
	}
	return list;
}

char         *
list2comma_string (char **list)
{
	char         *string = NULL;

	if (list)
	{
		int           len = 0;
		register int  i = 0;

		for (i = 0; list[i]; i++)
			if (*list[i])
				len += strlen (list[i]) + 1;

		if (len > 0)
		{
			register char *src, *dst;

			dst = string = (char *)safemalloc (len);
			for (i = 0; list[i]; i++)
			{
				src = list[i];
				if (*src)
				{
					while (*src)
					{
						*(dst++) = *(src++);
					}
					*(dst++) = ',';
				}
			}
			*(dst - 1) = '\0';
		}
	}
	return string;
}

char *
make_tricky_text( char *src )
{
    int len = 0, longest_line = 0;
    int pos = 0;
    char *trg ;
    register int i, k ;

    for( i = 0 ; src[i] ; i++ )
        if( src[i] == '\n' )
        {
            if( len > longest_line )
                longest_line = len;
            len = 0 ;
            pos++ ;
        }else
            len++ ;

    if( len > longest_line )
        longest_line = len;
    pos++ ;
    trg = safemalloc( longest_line*(pos+1)+1 );
    i = 0 ;
    for( pos = 0 ; pos < longest_line ; pos++ )
    {
        len = 0 ;
        for( k = 0 ; src[k] ; k++ )
        {
            if( src[k] == '\n' )
            {
                if( len <= pos )
                    trg[i++] = ' ';
                len = 0 ;
            }else
            {
                if( len == pos )
                    trg[i++] = src[k] ;
                len++ ;
            }
        }
        trg[i++] = '\n' ;
    }
    if( i )
        i-- ;
    trg[i] = '\0' ;
    return trg;
}

