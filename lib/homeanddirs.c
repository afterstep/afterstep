/*
 * Handle directories & files
 *
 * Copyright (c) 1998 Ethan Ficher <allanon@crystaltokyo.com>
 * Copyright (c) 1998 Michael Vitecek <M.Vitecek@sh.cvut.cz>
 * Copyright (c) 1998 Chris Ridd <c.ridd@isode.com>
 * Copyright (c) 1997 Guylhem AZNAR <guylhem@oeil.qc.ca>
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "../configure.h"
#include "../include/aftersteplib.h"

time_t FileModifiedTime (const char *filename);

/*
 * get the date stamp on a file
 */
time_t
FileModifiedTime (const char *filename)
{
  struct stat st;
  time_t stamp = 0;
  if (stat (filename, &st) != -1)
    stamp = st.st_mtime;
  return stamp;
}

char *
make_file_name (const char *path, const char *file)
{
  register int i = 0;
  register char *ptr;
  char *filename;

  /* getting length */
  for (ptr = (char *) path; *ptr; i++, ptr++);
  i++;
  for (ptr = (char *) file; *ptr; i++, ptr++);
  i++;
  ptr = filename = safemalloc (i);
  /* copying path */
  for (i = 0; *(path + i); i++, ptr++)
    *ptr = *(path + i);
  /* copying filename */
  *(ptr++) = '/';
  for (i = 0; *(file + i); i++, ptr++)
    *ptr = *(file + i);
  *ptr = *(file + i);		/* zero byte */

  return filename;
}

/* copy file1 into file2 */
int
CopyFile (const char *realfilename1, const char *realfilename2)
{
  FILE *targetfile, *sourcefile;
  int c;

  targetfile = fopen (realfilename2, "w");
  if (targetfile == NULL)
    {
      fprintf (stderr, "can't open %s !\n", realfilename2);
      return (-1);
    }
  sourcefile = fopen (realfilename1, "r");
  if (sourcefile == NULL)
    {
      fprintf (stderr, "can't open %s !\n", realfilename1);
      return (-2);
    }
  while ((c = getc (sourcefile)) != EOF)
    putc (c, targetfile);

  fclose (targetfile);
  fclose (sourcefile);
  return 0;
}

int
CheckMode (const char *file, int mode)
{
  struct stat st;

  if ((stat (file, &st) == -1) || (st.st_mode & S_IFMT) != mode)
    return (-1);
  else
    return (0);
}

char *
PutHome (const char *path_with_home)
{
  static char *home = NULL;	/* the HOME environment variable */
  static char default_home[3] = "./";
  static int home_len = 0;
  char *str, *ptr;
  register int i;

  if (path_with_home == NULL)
    return NULL;
  /* home dir ? */
  if (*path_with_home != '~' || *(path_with_home + 1) != '/')
    return mystrdup (path_with_home);

  if (home == NULL)
    {
      if ((home = getenv ("HOME")) == NULL)
	home = &(default_home[0]);
      home_len = strlen (home);
    }

  for (i = 2; *(path_with_home + i); i++);
  str = safemalloc (home_len + 1 + i);
  for (ptr = str + home_len + i; i > 0; i--, ptr--)
    *ptr = *(path_with_home + i);
  *ptr = '/';
  for (i = 0; i < home_len; i++)
    *(str + i) = *(home + i);

  return str;
}
