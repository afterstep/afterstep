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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <string.h>

#include "../configure.h"
#include "../include/aftersteplib.h"

#if HAVE_UNAME
/* define mygethostname() by using uname() */

#include <sys/utsname.h>

Bool
mygethostname (char *client, size_t length)
{
	struct utsname sysname;

    if( client == NULL )
        return False;
    uname (&sysname);
	strncpy (client, sysname.nodename, length);
    return (*client != '\0');
}

#else
#if HAVE_GETHOSTNAME
/* define mygethostname() by using gethostname() :-) */

Bool
mygethostname (char *client, size_t length)
{
    if( client == NULL )
        return False;
	gethostname (client, length);
    return (*client != '\0');
}

#else
Bool
mygethostname (char *client, size_t length)
{
	*client = 0;
    return False ;
}

#endif
#endif
