
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "../../configure.h"
#include "../../include/aftersteplib.h"
#include "../../include/myscandir.h"

/*
 * Non-NULL select and dcomp pointers are *NOT* tested, but should be OK.
 * They are not used by afterstep however, so this implementation should
 * be good enough.
 *
 * c.ridd@isode.com
 */
int
my_scandir (char *dirname, struct direntry *(*namelist[]),
	    int (*select) (struct dirent *),
	    int (*dcomp) (struct direntry **, struct direntry **))
{
  DIR *d;
  struct dirent *e;		/* Pointer to static struct inside readdir() */
  struct direntry **nl;		/* Array of pointers to dirents */
  struct direntry **nnl;
  int n;			/* Count of nl used so far */
  int sizenl;			/* Number of entries in nl array */
  int j;
  size_t realsize;
  char *filename;		/* For building filename to pass to stat */
  char *p;			/* Place where filename starts */
  struct stat buf;


  d = opendir (dirname);

  if (d == NULL)
    return -1;

  filename = (char *) safemalloc (strlen (dirname) + PATH_MAX + 2);
  if (filename == NULL)
    {
      closedir (d);
      return -1;
    }
  strcpy (filename, dirname);
  p = filename + strlen (filename);
  *p++ = '/';
  *p = 0;			/* Just in case... */

  nl = NULL;
  n = 0;
  sizenl = 0;

  while ((e = readdir (d)) != NULL)
    {
      if ((select == NULL) || select (e))
	{
	  /* add */
	  if (sizenl == n)
	    {
	      /* Grow array */
	      sizenl += 32;	/* arbitrary delta */
	      nnl = realloc (nl, sizenl * sizeof (struct direntry *));
	      if (nnl == NULL)
		{
		  /* Free the old array */
		  for (j = 0; j < n; j++)
		    free (nl[j]);
		  free (nl);
		  free (filename);
		  closedir (d);
		  return -1;
		}
	      nl = nnl;
	    }
	  realsize = offsetof (struct direntry, d_name) +strlen (e->d_name) + 1;
	  nl[n] = (struct direntry *) safemalloc (realsize);
	  if (nl[n] == NULL)
	    {
	      for (j = 0; j < n; j++)
		free (nl[j]);
	      free (nl);
	      free (filename);
	      closedir (d);
	      return -1;
	    }
	  /* Fill in the fields using stat() */
	  strcpy (p, e->d_name);
	  if (stat (filename, &buf) == -1)
	    {
	      for (j = 0; j <= n; j++)
		free (nl[j]);
	      free (nl);
	      free (filename);
	      closedir (d);
	      return -1;
	    }
	  nl[n]->d_mode = buf.st_mode;
	  nl[n]->d_mtime = buf.st_mtime;
	  strcpy (nl[n]->d_name, e->d_name);
	  n++;
	}
    }
  free (filename);

  if (closedir (d) == -1)
    {
      free (nl);
      return -1;
    }
  if (n == 0)
  {
	if( nl ) 
		free( nl );
/* OK, but not point sorting or freeing anything */
    return 0;
  }
  *namelist = realloc (nl, n * sizeof (struct direntry *));

  if (*namelist == NULL)
    {
      for (j = 0; j < n; j++)
	free (nl[j]);
      free (nl);
      return -1;
    }
  /* Optionally sort the list */
  if (dcomp)
    qsort (*namelist, n, sizeof (struct direntry *), (int (*)()) dcomp);

  /* Return the count of the entries */
  return n;
}

/*
 * Use this function as the select argument to my_scandir to make it ignore
 * all files and directories starting with "."
 */
int
ignore_dots (struct dirent *e)
{
  return (e->d_name[0] != '.');
}
