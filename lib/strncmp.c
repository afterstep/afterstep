
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "../configure.h"
#include "../include/aftersteplib.h"

int
mystrncasecmp (const char *s1, const char *s2, size_t n)
{
  register int c1, c2;

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
