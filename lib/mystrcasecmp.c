#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "../configure.h"
#include "../include/aftersteplib.h"

int
mystrcasecmp (const char *s1, const char *s2)
{
  char c1, c2;
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
