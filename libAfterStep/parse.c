#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

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

/**************************************************************/
/* Backported from afterstep-devel on 01/06/2002 by Sasha Vasko : */

char         *
parse_signed_int (register char *tline, int *val_return, int *sign_return)
{
	int  val = 0, sign = 0;
	register int i = 0 ;

	while (isspace ((int)tline[i])) ++i;

	switch( tline[i] )
	{ /* handling constructs like --10 or -+10 which is equivalent to -0-10or -0+10 */
		case '\0' : sign = 5 ; --i; break ;
		case '-' : 	sign = -1;
					if( tline[i+1] == '-' )
					{ ++i ; sign = -2 ; }
					else if( tline[i+1] == '+' )
					{ ++i ; sign = 3 ; }
					break;
		case '+' :	++sign;
					if( tline[i+1] == '-' )
					{ ++i ; sign = -3 ; }
					else if( tline[i+1] == '+' )
					{ ++i ; sign = 2 ; }
					break;
		case '=' :  break; /* skipping */
		case 'x' :
		case 'X' :  sign = 4; break;
	  default : --i ;
	}
	while (isdigit ((int)tline[++i]))
		val = val * 10 + (int)(tline[i] - '0');

	if( val_return )
		*val_return = (sign < 0)?-val:val ;
	if( sign_return )
		*sign_return = sign;
	return tline+i;
}

char         *
parse_func_args (char *tline, char *unit, int *func_val)
{
	tline = parse_signed_int( tline, func_val, NULL );

	*unit = *tline;
	if (isspace ((int)*tline))
		*unit = '\0' ;
	return tline[0]?tline+1:tline;
}

char         *
parse_geometry (register char *tline,
                int *x_return, int *y_return,
                unsigned int *width_return,
  				unsigned int *height_return,
				int* flags_return )
{
	int flags = 0 ;
	int sign, val ;

	tline = parse_signed_int( tline, &val, &sign );
	if( sign == 0 )
	{
		if( width_return )
		{
			*width_return = val ;
			set_flags( flags, WidthValue );
		}
		tline = parse_signed_int( tline, &val, &sign );
	}
	if( sign == 4 )
	{
		if( height_return )
		{
			*height_return = val ;
			set_flags( flags, HeightValue );
		}
		tline = parse_signed_int( tline, &val, &sign );
	}
	if( sign == 0 )
		sign = 1 ;
	if( sign == 1 || sign == -1)
	{
		if( x_return )
		{
			*x_return = val ;
			set_flags( flags, ( sign < 0 )?XNegative|XValue:XValue );
		}
		tline = parse_signed_int( tline, &val, &sign );
	}else if( sign != 5 )
	{
		if( x_return )
		{
			*x_return = 0 ;
			set_flags( flags, ( sign == -2 || sign == 3 )?XNegative|XValue:XValue );
		}
	}

	if( sign != 5 && y_return )
	{
		*y_return = val ;
		set_flags( flags, ( sign < 0 )?YNegative|YValue:YValue );
	}

	if( flags_return )
		*flags_return = flags ;

	return tline;
}
