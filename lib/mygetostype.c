#include <string.h>

#include "../configure.h"
#include "../include/aftersteplib.h"

char* mygetostype (void);

#if HAVE_UNAME
#include <sys/utsname.h>

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
char*
mygetostype (void)
{
  return NULL;
}
#endif
