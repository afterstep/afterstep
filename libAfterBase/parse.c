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
     (tmp != NULL) && (*tmp != 0) && (*tmp != ',') && (*tmp != '\n'); tmp++)
    (*len)++;

  if (*fname != NULL)
    free (*fname);

  if (*len > 0)
    *fname = mystrndup (restofline, *len);
  else
    *fname = NULL;

  return tmp;
}

char*
find_doublequotes( char* ptr )
{
  if( ptr )
  {
    if( *ptr == '"' ) ptr++ ;

    if( *ptr != '"' )
    {
        while( (ptr= strchr(ptr,'"')))
    	    if( *(ptr-1) == '\\' ) ptr++ ;
	    else break ;
    }
  }
  return ptr ;
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
  for (ptr = source + strlen (source); ptr > source && isspace (*(ptr - 1));
       ptr--);
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
  char *ptr = (char*)source, *tail;
  if( *ptr != '"' )
      ptr = find_doublequotes(ptr);
  if( ptr != NULL )
  {
      if( (tail = find_doublequotes( ptr ))!= NULL ) 
          return mystrndup (ptr+1, tail - ptr - 1);
      return mystrdup( ptr+1 );
  }
  return mystrndup (source, 0);
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
	  if ((ptr = find_doublequotes(ptr)) == NULL)
	  {
	    ptr = source+strlen(source);
	    break;
	  }
	}
      if (*ptr == '#')
	{			/* checking if it is not a hex color code */
	  int i;
	  for (i = 1; isxdigit (*(ptr + i)); i++);

	  if (i < 4 || i > 13 || (*(ptr + i) && !isspace (*(ptr + i))))
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
parse_filename (const char *source, char **trg)
{
  for (; isspace (*source); source++);

  if (*source == '"')
    {
      if ((*trg = stripcpy2 (source, 0)) != NULL)
	source += strlen (*trg) + 2;
    }
  else
    source = parse_token (source, trg);
  return (char *) source;
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

/***********************************************************************
 * Procedure:
 *	scanForHotkeys - Look for hotkey markers in a function "name"
 * Inputs:
 *	txt	- string to scan
 * Output:      - hotkey found
 ***********************************************************************/
char
scan_for_hotkey (char *txt)
{
  char hotkey = '\0';
  if (txt != NULL)
    for (; *txt != '\0'; txt++)	/* Scan whole string                 */
      if (*txt == '&')
	{			/* A hotkey marker?                  */
	  char *tmp;		/* Copy the string down over it      */
	  for (tmp = txt; *tmp != '\0'; tmp++)
	    *tmp = *(tmp + 1);
	  if (*txt != '&')	/* Not Just an escaped &            */
	    hotkey = *txt;
	}
  return hotkey;
}


/* Conversion of the coma separated values into a list of strings */
char* get_comma_item( char* ptr, char** item_start, char** item_end )
{
  
    if( ptr == NULL ) return NULL ;
    while( *ptr && (isspace(*ptr) || *ptr == ',' ))  ptr++ ;
    if( *ptr == '\0' ) return NULL ;
    
    *item_end = *item_start = ptr ;
    if( *ptr == '"' )
    {
	if( (*item_end = find_doublequotes( ptr )) == NULL ) return NULL ;
    	ptr = *item_end ;
	while( *ptr && !(isspace(*ptr) || *ptr == ',' ))  ptr++ ;
    }else
    {
	while( *ptr && *ptr != ',' ) ptr++ ;
	*item_end = ptr ;
    }
    return ptr ; 
}

char** comma_string2list( char* string )
{
  register char *ptr ;
  int items ;
  char **list = NULL, *item_start = NULL, *item_end = NULL ;

    if( string == NULL ) return NULL ;
    
    ptr = string; items = 0 ;
    while( *ptr )
    { 
        if( (ptr = get_comma_item( ptr, &item_start, &item_end )) == NULL ) break ;
	items++ ;	
    }	
    if( items > 0 ) 
    {
      register int i ;

	list = (char**)safemalloc( sizeof(char*)*(items+1) );
	memset( list, 0x00, sizeof(char*)*(items+1) );
	ptr = string;	
	for( i = 0 ; i < items ; i++ )
	{ 
    	    if( (ptr = get_comma_item( ptr, &item_start, &item_end )) == NULL ) break ;
	    list[i] = mystrndup( item_start, (item_end-item_start) ) ;
	}
    }	
    return list ;	
}    

char* list2comma_string(char** list)
{
  char *string = NULL ;
    if( list )
    {
      int len = 0 ;
      register int i = 0;
      
      for( i = 0 ; list[i] ; i++ ) 
        if( *list[i] ) len += strlen( list[i] )+1;

      if( len > 0 )
      {
        register char *src, *dst ;
	    dst = string = (char*)safemalloc(len);  
	    for( i = 0 ; list[i] ; i++ ) 
	    {
		src = list[i] ;
		if( *src )
		{
		    while( *src )
		    {
			*(dst++) = *(src++);
		    }
		    *(dst++) = ',' ;
		}
	    }
	    *(dst-1) = '\0' ;
      }
    }
    return string ;
}
