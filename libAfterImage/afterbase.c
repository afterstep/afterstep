/*
 * Copyright (C) 2001 Sasha Vasko <sashav@sprintmal.com>
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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>

#include "config.h"
#include "afterbase.h"

/* from libAfterBase/output.c : */
Bool asim_show_error( const char *error_format, ...)
{
    if( OUTPUT_LEVEL_ERROR <= get_output_threshold())
    {
        va_list ap;
        fprintf (stderr, "%s ERROR: ", get_application_name() );
        va_start (ap, error_format);
        vfprintf (stderr, error_format, ap);
        va_end (ap);
        fprintf (stderr, "\n" );
        return True;
    }
    return False;
}

Bool asim_show_warning( const char *warning_format, ...)
{
    if( OUTPUT_LEVEL_WARNING <= get_output_threshold())
    {
        va_list ap;
        fprintf (stderr, "%s warning: ", get_application_name() );
        va_start (ap, warning_format);
        vfprintf (stderr, warning_format, ap);
        va_end (ap);
        fprintf (stderr, "\n" );
        return True;
    }
    return False;
}

void asim_nonGNUC_debugout( const char *format, ...)
{
    va_list ap;
    fprintf (stderr, "%s: ", get_application_name() );
    va_start (ap, format);
    vfprintf (stderr, format, ap);
    va_end (ap);
    fprintf (stderr, "\n" );
}

inline void asim_nonGNUC_debugout_stub( const char *format, ...)
{}

/* from libAfterBase/fs.c : */

int		asim_check_file_mode (const char *file, int mode)
{
	struct stat   st;

	if ((stat (file, &st) == -1) || (st.st_mode & S_IFMT) != mode)
		return (-1);
	else
		return (0);
}

char         *
asim_put_file_home (const char *path_with_home)
{
	static char  *home = NULL;				   /* the HOME environment variable */
	static char   default_home[3] = "./";
	static int    home_len = 0;
	char         *str, *ptr;
	register int  i;

	if (path_with_home == NULL)
		return NULL;
	/* home dir ? */
	if (path_with_home[0] != '~' || path_with_home[1] != '/')
		return mystrdup (path_with_home);

	if (home == NULL)
	{
		if ((home = getenv ("HOME")) == NULL)
			home = &(default_home[0]);
		home_len = strlen (home);
	}

	for (i = 2; path_with_home[i]; i++);
	str = safemalloc (home_len + i);

	for (ptr = str + home_len-1; i > 0; i--)
		ptr[i] = path_with_home[i];
	for (i = 0; i < home_len; i++)
		str[i] = home[i];

	return str;
}

char   *asim_find_file (const char *file, const char *pathlist, int type)
{
	char 		  *path;
	register int   len;
	int            max_path = 0;
	register char *ptr;
	register int   i;

	if (file == NULL)
		return NULL;
	if (*file == '/' || *file == '~' || ((pathlist == NULL) || (*pathlist == '\0')))
	{
		path = put_file_home (file);
		if (access (path, type) == 0)
			return path;
		free (path);
		return NULL;
	}

	for (i = 0; file[i]; i++);
	len = i ;
	for (ptr = (char *)pathlist; *ptr; ptr += i)
	{
		if (*ptr == ':')
			ptr++;
		for (i = 0; ptr[i] && ptr[i] != ':'; i++);
		if (i > max_path)
			max_path = i;
	}

	path = safemalloc (max_path + 1 + len + 1);

	/* Search each element of the pathlist for the icon file */
	while( pathlist[0] != 0 )
	{
		if (pathlist[0] == ':')
			++pathlist;
		ptr = (char*)pathlist ;
		for (i = 0; ptr[i] && ptr[i] != ':'; i++)
			path[i] = ptr[i];
		pathlist += i;
		if (i == 0)
			continue;
		path[i] = '/';
		ptr = &(path[i+1]);
		i = -1 ;
		do
		{
			++i;
			ptr[i] = file[i];
		}while( file[i] != '\0' );
		if (access (path, type) == 0)
			return path;
	}
	/* Hmm, couldn't find the file.  Return NULL */
	free (path);
	return NULL;
}
