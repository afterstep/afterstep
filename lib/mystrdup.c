#include <string.h>
#include "../configure.h"
#include "../include/aftersteplib.h"

#if defined(LOG_STRDUP_CALLS) && defined(DEBUG_ALLOCS)
char *
l_mystrdup (const char *file, int line, const char *str)
#else
char *
mystrdup (const char *str)
#endif
{
  char *c = NULL;
#if defined(LOG_STRDUP_CALLS) && defined(DEBUG_ALLOCS)
  log_call (file, line, "mystrdup", str);
#endif
  if (str)
    {
      c = safemalloc (strlen (str) + 1);
      strcpy (c, str);
    }
  return c;
}

#if defined(LOG_STRDUP_CALLS) && defined(DEBUG_ALLOCS)
char *
l_mystrndup (const char *file, int line, const char *str, size_t n)
#else
char *
mystrndup (const char *str, size_t n)
#endif
{
  char *c = NULL;
#if defined(LOG_STRDUP_CALLS) && defined(DEBUG_ALLOCS)
  log_call (file, line, "mystrndup", str);
#endif
  if (str)
    {
      c = safemalloc (n + 1);
      strncpy (c, str, n);
      c[n] = '\0';
    }
  return c;
}
