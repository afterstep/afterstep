#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "../configure.h"
#include "../include/aftersteplib.h"

/***************************************************************************
 *
 * A simple routine to copy a string, stripping spaces and mallocing
 * space for the new string 
 ***************************************************************************/
void
CopyString (char **dest, char *source)
{
	size_t        len;
	char         *start;

	while (((isspace ((unsigned char)*source)) && (*source != '\n')) && (*source != 0))
	{
		source++;
	}
	len = 0;
	start = source;
	while ((*source != '\n') && (*source != 0))
	{
		len++;
		source++;
	}

	source--;

	while ((isspace ((unsigned char)*source)) && (*source != 0) && (len > 0))
	{
		len--;
		source--;
	}
	*dest = safemalloc (len + 1);
	strncpy (*dest, start, len);
	(*dest)[len] = 0;
}
