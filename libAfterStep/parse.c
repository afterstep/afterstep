#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "../configure.h"
#include "../include/aftersteplib.h"
#include "../include/parse.h"

/****************************************************************************
 * 
 * Some usefull parsing functions
 *
 ****************************************************************************/
char *
ReadIntValue (char *restofline, int *value)
{
  sscanf (restofline, "%d", value);

  while (isspace ((unsigned char) *restofline))
    restofline++;
  while ((!isspace ((unsigned char) *restofline)) && (*restofline != 0) &&
	 (*restofline != ',') && (*restofline != '\n'))
    restofline++;
  while (isspace ((unsigned char) *restofline))
    restofline++;

  return restofline;
}

char *
ReadColorValue (char *restofline, char **color, int *len)
{
  char *tmp;
  *len = 0;

  while (isspace ((unsigned char) *restofline))
    restofline++;
  for (tmp = restofline; (tmp != NULL) && (*tmp != 0) && (*tmp != ',') &&
       (*tmp != '\n') && (*tmp != '/') && (!isspace ((unsigned char) *tmp));
       tmp++)
    (*len)++;

  if (*len > 0)
    {
      if (*color)
	free (*color);
      *color = safemalloc (*len + 1);
      strncpy (*color, restofline, *len);
      (*color)[*len] = 0;
    }

  return tmp;
}

char *
ReadFileName (char *restofline, char **fname, int *len)
{
  char *tmp;
  *len = 0;

  while (isspace ((unsigned char) *restofline))
    restofline++;
  for (tmp = restofline;
       (tmp != NULL) && (*tmp != 0) && (*tmp != ',') && (*tmp != '\n');
       tmp++)
    (*len)++;

  if (*fname != NULL)
    free (*fname);

  if (*len > 0)
    *fname = mystrndup (restofline, *len);
  else
    *fname = NULL;

  return tmp;
}

/****************************************************************************
 * 
 * Copies a string into a new, malloc'ed string
 * Strips leading and trailing whitespace
 *
 ****************************************************************************/

char *
stripcpy (const char *source)
{
  const char *ptr;
  for (; isspace (*source); source++);
  for (ptr = source + strlen (source); ptr > source && isspace (*(ptr - 1)); ptr--);
  return mystrndup (source, ptr - source);
}


/****************************************************************************
 * 
 * Copies a string into a new, malloc'ed string
 * Strips all data before the first quote and after the second
 *
 ****************************************************************************/

char *
stripcpy2 (const char *source, int tab_sensitive)
{
  const char *ptr;
  for (; *source != '"' && *source != '\0'; source++);
  if (*source == '\0')
    return NULL;
  source++;
  ptr = source;
  if (!tab_sensitive)
    for (; *ptr != '"' && *ptr != '\0'; ptr++);
  else
    for (; *ptr != '"' && *ptr != '\0' && *ptr != '\t'; ptr++);

  if (tab_sensitive == 2)
    {
      for (source = ptr; *source != '"' && *source != '\0'; source++);
      for (ptr = source; *ptr != '"' && *ptr != '\0'; ptr++);
    }
  return mystrndup (source, ptr - source);
}


/* here we'll strip comments and skip whitespaces */
char *
stripcomments (char *source)
{
  register char *ptr;
  /* remove comments from the line */
  while (isspace (*source))
    source++;
  for (ptr = (char *) source; *ptr; ptr++)
    {
      if (*ptr == '"')
	{
	  for (ptr++; *ptr && *ptr != '"'; ptr++);
	  if (*ptr == '\0')
	    break;
	}
      if (*ptr == '#')
	{			/* checking if it is not a hex color code */
	  int i;
	  for (i = 1; isxdigit (*(ptr + i)); i++);

	  if (i < 4 || i > 13 ||
	      (*(ptr + i) && !isspace (*(ptr + i)))
	    )
	    {
	      for (ptr--; ptr > source && isspace (*ptr); ptr--);
	      *(ptr + 1) = '\0';
	      /* we'll exit the loop automagically */
	    }
	  else
	    ptr += i - 1;
	}
    }
  for (ptr--; isspace (*ptr) && ptr > source; ptr--);
  *(ptr + 1) = '\0';
  return source;
}

char *
strip_whitespace (char *str)
{
  char *ptr;
  /* strip trailing whitespace */
  for (ptr = str + strlen (str); ptr > str && isspace (*(ptr - 1)); ptr--)
    *(ptr - 1) = '\0';
  /* skip leading whitespace */
  for (ptr = str; isspace (*ptr); ptr++);
  return ptr;
}


/****************************************************************************
 * 
 * Copies a whitespace separated token into a new, malloc'ed string
 * Strips leading and trailing whitespace
 *
 ****************************************************************************/

char *
tokencpy (const char *source)
{
  const char *ptr;
  for (; isspace (*source); source++);
  for (ptr = source; !isspace (*ptr) && *ptr; ptr++);
  return mystrndup (source, ptr - source);
}

/****************************************************************************
 *
 * Matches text from config to a table of strings.
 *
 ****************************************************************************/

struct config *
find_config (struct config *table, const char *text)
{
  for (; strlen (table->keyword); table++)
    if (!mystrncasecmp (text, table->keyword, strlen (table->keyword)))
      return table;

  return NULL;
}

/****************************************************************************
 * 
 * Copies a whitespace separated token into a new, malloc'ed string
 * Strips leading and trailing whitespace
 * returns pointer to the end of token in source string
 ****************************************************************************/

char *
parse_token (const char *source, char **trg)
{
  char *ptr;
  for (; isspace (*source); source++);
  for (ptr = (char *) source; !isspace (*ptr) && *ptr; ptr++);
  *trg = mystrndup (source, ptr - source);
  return ptr;
}

char *
parse_tab_token (const char *source, char **trg)
{
  char *ptr;
  for (; isspace (*source); source++);
  for (ptr = (char *) source; *ptr != '\t' && *ptr; ptr++);
  *trg = mystrndup (source, ptr - source);
  return ptr;
}


/* used in ParseMouse and in communication with modules */
char *
parse_func_args (char *tline, char *unit, int *func_val)
{
  int sign = 0;
  while (isspace (*tline))
    tline++;
  *func_val = 0;
  if (*tline == '-')
    sign = -1;
  else if (*tline == '+')
    sign = 1;
  if (sign != 0)
    tline++;
  while (isdigit (*tline))
    {
      *func_val = (*func_val) * 10 + (int) ((*tline) - '0');
      tline++;
    }
  if (*tline && !isspace (*tline))
    *unit = *tline;
  else
    *unit = '\0';
  if (sign != 0)
    *func_val *= sign;
  return tline;
}

char *
string_from_int (int param)
{
  char *mem;
  register int i;
  int neg = 0;
  if (param == 0)
    return mystrdup ("0");
  if (param < 0)
    {
      param = -param;
      neg = 1;
    }
  for (i = 1; param >> (i * 3); i++);
  if (neg)
    i++;
  mem = safemalloc (i + 1);
  if (neg)
    mem[0] = '-';
  sprintf (&(mem[neg]), "%u", param);
  return mem;
}
