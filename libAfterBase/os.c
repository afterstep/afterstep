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

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "config.h"
#include "astypes.h"
#include "output.h"
#include "audit.h"
#include "mystring.h"
#include "safemalloc.h"
#include "os.h"

#if defined (__sun__) && defined (SVR4)
/* Solaris has sysinfo instead of gethostname.  */
#include <sys/systeminfo.h>
#endif

#if HAVE_UNAME
/* define mygethostname() by using uname() */

#include <sys/utsname.h>

Bool
mygethostname (char *client, size_t length)
{
	struct utsname sysname;

    if( client == NULL )
        return 0;
    uname (&sysname);
	strncpy (client, sysname.nodename, length);
    return (*client != '\0');
}

/* return a string indicating the OS type (i.e. "Linux", "SINIX-D", ... ) */
char*
mygetostype (void)
{
  char* str = NULL;
  struct utsname sysname;

  if (uname (&sysname) != -1)
    str = mystrdup(sysname.sysname);

  return NULL;
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

char*
mygetostype (void)
{
  return NULL;
}

#endif

int
get_fd_width (void)
{
	int           r;

#ifdef _SC_OPEN_MAX
	r = sysconf (_SC_OPEN_MAX);
#else
	r = getdtablesize ();
#endif
#ifdef FD_SETSIZE
	return (r > FD_SETSIZE ? FD_SETSIZE : r);
#else
	return r;
#endif
}
