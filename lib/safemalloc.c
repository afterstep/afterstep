#include <stdlib.h>
#include <stdio.h>
#include "../configure.h"
#include "../include/aftersteplib.h"

#ifdef DEBUG_ALLOCS
#include <string.h>
#undef malloc
#undef safemalloc
#endif /* DEBUG_ALLOCS */

/* always undef free, as it will be both redefined with and without
   DEBUG_ALLOC */
#undef free

/***********************************************************************
 *
 *  Procedure:
 *	safemalloc - mallocs specified space or exits if there's a
 *		     problem
 *
 ***********************************************************************/
static int use_tmp_heap = 0;

int
set_use_tmp_heap (int on)
{
  int toret = use_tmp_heap;
  use_tmp_heap = on;
  return toret;
}

void *
safemalloc (size_t length)
{
  char *ptr;

  if (length <= 0)
    length = 1;

  ptr = malloc (length);

  if (ptr == (char *) 0)
    {
      fprintf (stderr, "malloc of %d bytes failed. Exiting\n", length);
      exit (1);
    }
  return ptr;
}

void
safefree (void *ptr)
{
  if( ptr )
    free (ptr);
}
