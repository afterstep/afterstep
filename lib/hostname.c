#include <string.h>

#include "../configure.h"
#include "../include/aftersteplib.h"

#if HAVE_UNAME
/* define mygethostname() by using uname() */

#include <sys/utsname.h>

int
mygethostname (char *client, size_t length)
{
  struct utsname sysname;

  uname (&sysname);
  strncpy (client, sysname.nodename, length);
  return 0;
}

#else
#if HAVE_GETHOSTNAME
/* define mygethostname() by using gethostname() :-) */

int
mygethostname (char *client, size_t length)
{
  gethostname (client, length);
}

#else
int
mygethostname (char *client, size_t length)
{
  *client = 0;
}

#endif
#endif
