#include <stdio.h>
#include <string.h>

#include "../configure.h"
#include "../include/aftersteplib.h"


/************************************************************************
 *
 * Concatentates 3 strings
 *
 *************************************************************************/

char *
CatString3 (const char *a, const char *b, const char *c)
{
  char *CatS;
  int length = 0;

  /* compute the length of the final string */
  if (a != NULL)
    length = strlen (a);
  if (b != NULL)
    length += strlen (b);
  if (c != NULL)
    length += strlen (c);

  CatS = (char *) safemalloc (length + 1);
  /* make sure the string is zero lentgh */
  *CatS = '\0';

  /* copy the strings if defined */
  if (a != NULL)
    strcat (CatS, a);
  if (b != NULL)
    strcat (CatS, b);
  if (c != NULL)
    strcat (CatS, c);

  return (CatS);
}
