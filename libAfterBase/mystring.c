/****************************************************************************
 *
 * Copyright (c) 2000 Sasha Vasko <sashav@sprintmail.com>
 * some previous unknown authors that has never left their copyrights :)
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
 ****************************************************************************/

#include "config.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "astypes.h"
#include "audit.h"
#include "output.h"
#include "safemalloc.h"
#include "mystring.h"

int
mystrcasecmp (const char *s1, const char *s2)
{
	char          c1, c2;

	if (s1 == NULL || s2 == NULL)
		return (s1 == s2) ? 0 : 1;
	while (*s1 && *s2)
	{
		/* in some BSD implementations, tolower(c) is not defined
		 * unless isupper(c) is true */
		c1 = *s1++;
		if (isupper (c1))
			c1 = tolower (c1);
		c2 = *s2++;
		if (isupper (c2))
			c2 = tolower (c2);

		if (c1 != c2)
			return (c1 - c2);
	}
	return (*s1 == *s2) ? 0 : 1;
}

int
mystrncasecmp (const char *s1, const char *s2, size_t n)
{
	register int  c1, c2;

	for (;;)
	{
		if (!n)
			return (0);
		c1 = *s1, c2 = *s2;
		if (!c1 || !c2)
			return (c1 - c2);
		if (isupper (c1))
			c1 += 32;
		if (isupper (c2))
			c2 += 32;
		if (c1 != c2)
			return (c1 - c2);
		n--, s1++, s2++;
	}
}

#undef mystrdup
#undef mystrndup
#undef safemalloc
void* safemalloc(size_t);
char         *
mystrdup (const char *str)
{
	char         *c = NULL;

	if (str)
	{
		c = safemalloc (strlen (str) + 1);
		strcpy (c, str);
	}
	return c;
}

char         *
mystrndup (const char *str, size_t n)
{
	char         *c = NULL;

	if (str)
	{
		c = safemalloc (n + 1);
		strncpy (c, str, n);
		c[n] = '\0';
	}
	return c;
}

/* very usefull utility function to update the value of any string property */
void
set_string_value (char **target, char *string, unsigned long *set_flags, unsigned long flag)
{
	if (target != NULL && string != NULL)
	{
		if (*target != string)
		{
			if (*target)
				free (*target);
			*target = string;
			if (set_flags)
				set_flags (*set_flags, flag);
		}
	}
}

