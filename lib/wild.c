#include <stdio.h>
#include <string.h>

#include "../configure.h"
#include "../include/aftersteplib.h"

/*****************************************************************************
 *	Does `string' match `pattern'? '*' in pattern matches any sub-string
 *	(including the null string) '?' matches any single char. For use
 *	by filenameforall. Note that '*' matches across directory boundaries
 *
 *      This code donated by  Paul Hudson <paulh@harlequin.co.uk>    
 *      It is public domain, no strings attached. No guarantees either.
 *		Modified by Emanuele Caratti <wiz@iol.it>
 *
 *****************************************************************************/

int
matchWildcards (const char *pattern, const char *string)
{
  if (pattern == NULL)
    return 1;
  else if (*pattern == '*' && !*(pattern + 1))
    return 1;

  if (string == NULL)
    return 0;

  while (*string && *pattern)
    {
      if (*pattern == '?')
	{
	  /* match any character */
	  pattern++;
	  string++;
	}
      else if (*pattern == '*')
	{
	  /* see if the rest of the pattern matches any trailing substring
	   * of the string. */
	  pattern++;
	  if (*pattern == 0)
	    {
	      return 1;		/* trailing * must match rest */
	    }
	  while (1)
	    {
	      while (*string && (*string != *pattern))
		string++;
	      if (!*string)
		return 0;
	      if (matchWildcards (pattern, string))
		return 1;
	      string++;
	    }

	}
      else
	{
	  if (*pattern == '\\')
	    pattern++;		/* has strange, but harmless effects if the last
				   * character is a '\\' */
	  if (*pattern++ != *string++)
	    {
	      return 0;
	    }
	}
    }
  if ((*pattern == 0) && (*string == 0))
    return 1;
  if ((*string == 0) && (strcmp (pattern, "*") == 0))
    return 1;
  return 0;
}
