#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "../configure.h"
#include "../include/aftersteplib.h"

int
GetFdWidth (void)
{
  int r;
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
