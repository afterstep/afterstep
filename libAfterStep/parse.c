#include "../configure.h"

#include "../include/asapp.h"
#include "../include/parse.h"

/****************************************************************************
 *
 * Some usefull parsing functions
 *
 ****************************************************************************/
char         *
ReadIntValue (char *restofline, int *value)
{
	sscanf (restofline, "%d", value);

	while (isspace ((unsigned char)*restofline))
		restofline++;
	while ((!isspace ((unsigned char)*restofline)) && (*restofline != 0) &&
		   (*restofline != ',') && (*restofline != '\n'))
		restofline++;
	while (isspace ((unsigned char)*restofline))
		restofline++;

	return restofline;
}

char         *
ReadColorValue (char *restofline, char **color, int *len)
{
	char         *tmp;

	*len = 0;

	while (isspace ((unsigned char)*restofline))
		restofline++;
	for (tmp = restofline; (tmp != NULL) && (*tmp != 0) && (*tmp != ',') &&
		 (*tmp != '\n') && (*tmp != '/') && (!isspace ((unsigned char)*tmp)); tmp++)
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

char         *
ReadFileName (char *restofline, char **fname, int *len)
{
	char         *tmp;

	*len = 0;

	while (isspace ((unsigned char)*restofline))
		restofline++;
	for (tmp = restofline; (tmp != NULL) && (*tmp != 0) && (*tmp != ',') && (*tmp != '\n'); tmp++)
		(*len)++;

	if (*fname != NULL)
		free (*fname);

	if (*len > 0)
		*fname = mystrndup (restofline, *len);
	else
		*fname = NULL;

	return tmp;
}
