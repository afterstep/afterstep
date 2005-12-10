/*
 * Copyright (C) 1999 Sasha Vasko <sasha at aftercode.net>
 * merged with envvar.c originally created by :
 * Copyright (C) 1999 Ethan Fischer <allanon@crystaltokyo.com>
 * Copyright (C) 1998 Pierre Clerissi <clerissi@pratique.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#undef LOCAL_DEBUG

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# if HAVE_SYS_DIRENT_H
#  include <sys/dirent.h>
#  define NAMLEN(dirent) strlen((dirent)->d_name)
# else
#  define dirent direct
#  define NAMLEN(dirent) (dirent)->d_namlen
#  if HAVE_SYS_NDIR_H
#   include <sys/ndir.h>
#  endif
#  if HAVE_SYS_DIR_H
#   include <sys/dir.h>
#  endif
#  if HAVE_NDIR_H
#   include <ndir.h>
#  endif
# endif
#endif

/* Even if limits.h is included, allow PATH_MAX to sun unices */
#ifndef PATH_MAX
#define PATH_MAX 255
#endif


#include "astypes.h"
#include "mystring.h"
#include "safemalloc.h"
#include "fs.h"
#include "output.h"
#include "audit.h"


/*
 * get the date stamp on a file
 */
time_t get_file_modified_time (const char *filename)
{
	struct stat   st;
	time_t        stamp = 0;

	if (stat (filename, &st) != -1)
		stamp = st.st_mtime;
	return stamp;
}

int
check_file_mode (const char *file, int mode)
{
	struct stat   st;

	if ((stat (file, &st) == -1) || (st.st_mode & S_IFMT) != mode)
		return (-1);
	else
		return (0);
}

/* copy file1 into file2 */
int
copy_file (const char *realfilename1, const char *realfilename2)
{
	FILE         *targetfile, *sourcefile;
	int           c;

#ifdef __CYGWIN__
    targetfile = fopen (realfilename2, "wb");
#else
    targetfile = fopen (realfilename2, "w");
#endif
    if (targetfile == NULL)
	{
		fprintf (stderr, "can't open %s !\n", realfilename2);
		return (-1);
	}
#ifdef __CYGWIN__
    sourcefile = fopen (realfilename1, "rb");
#else
    sourcefile = fopen (realfilename1, "r");
#endif
    if (sourcefile == NULL)
	{
		fprintf (stderr, "can't open %s !\n", realfilename1);
		fclose (targetfile);
		return (-2);
	}
	while ((c = getc (sourcefile)) != EOF)
		putc (c, targetfile);

	fclose (targetfile);
	fclose (sourcefile);
	return 0;
}

char*
load_binary_file(const char* realfilename, long *file_size_return)
{
	struct stat st;
	FILE* fp;
	char* data = NULL ;

	/* Get the file size. */
	if (stat(realfilename, &st)) return NULL;
	/* Open the file. */
	fp = fopen(realfilename, "rb");
	if ( fp != NULL ) 
	{
		long len ; 
		/* Read in the file. */
		data = safemalloc(st.st_size + 1);
		len = fread(data, 1, st.st_size, fp);
		if( file_size_return ) 
			*file_size_return = len ; 
		fclose(fp);
	}
	return data;
}

char*
load_file(const char* realfilename)
{
	long len;
	char* str = load_binary_file( realfilename, &len );

	if (str != NULL && len >= 0) 
		str[len] = '\0';

	return str;
}

void
parse_file_name(const char *filename, char **path, char **file)
{
	register int i = 0 ;
	register char *ptr = (char*)filename;
	int len = 0 ;
	while( ptr[i] ) ++i ;
	len = i ;
	while( --i >= 0 )
		if( ptr[i] == '/' )
			break;
	++i ;
	if( path )
		*path = mystrndup(ptr, i);
	if( file )
		*file = mystrndup(&(ptr[i]), len-i);
}

char         *
make_file_name (const char *path, const char *file)
{
	register int  i = 0;
	register char *ptr;
	char         *filename;
	int 		  len;

	if( file == NULL ) 
	{
		if( path == NULL ) 
			return NULL;	
		return mystrdup(path);
	}else if( path == NULL ) 
		return mystrdup(file);			 
	/* getting length */
	for (ptr = (char *)path; ptr[i]; i++);
	len = i+1;
	ptr = (char *)file ;
	for ( i = 0; ptr[i]; i++);
	len += i+1;
	ptr = filename = safemalloc (len);
	/* copying path */
	for (i = 0; path[i]; i++)
		ptr[i] = path[i];
	/* copying filename */
	ptr[i] = '/';
	ptr += i+1 ;
	for (i = 0; file[i]; i++)
		ptr[i] = file[i];
	ptr[i] = '\0';						   /* zero byte */

	return filename;
}

char         *
put_file_home (const char *path_with_home)
{
	static char  *home = NULL;				   /* the HOME environment variable */
	static char   default_home[3] = "./";
	static int    home_len = 0;
	char         *str = NULL, *ptr;
	register int  i;
	if (path_with_home == NULL)
		return NULL;
	/* home dir ? */
	if ( strncmp(  path_with_home, "$HOME/", 6 ) == 0 )
		path_with_home += 5 ;
	else if (path_with_home[0] == '~' && path_with_home[1] == '/')
		path_with_home += 1 ;
	else
		return mystrdup(path_with_home);

	if (home == NULL)
	{
		if ((home = getenv ("HOME")) == NULL)
			home = &(default_home[0]);
		home_len = strlen (home);
	}

	for (i = 0; path_with_home[i]; i++);
	str = safemalloc (home_len + i + 1);
	for (ptr = str + home_len; i >= 0; i--)
		ptr[i] = path_with_home[i];
	for (i = 0; i < home_len; i++)
		str[i] = home[i];
	return str;
}


/****************************************************************************
 *
 * Find the specified icon file somewhere along the given path.
 *
 * There is a possible race condition here:  We check the file and later
 * do something with it.  By then, the file might not be accessible.
 * Oh well.
 *
 ****************************************************************************/
/* supposedly pathlist should not include any environment variables
   including things like ~/
 */

char         *
find_file (const char *file, const char *pathlist, int type)
{
	char 		  *path;
	register int   len;
	int            max_path = 0;
	register char *ptr;
	register int   i;
	Bool local = False ;
LOCAL_DEBUG_CALLER_OUT( "file \"%s\", pathlist = \"%s\"", file, pathlist );
	if (file == NULL)
		return NULL;

	if (*file == '/' || *file == '~' || ((pathlist == NULL) || (*pathlist == '\0')))
		local = True ;
	else if( file[0] == '.' && (file[1] == '/' || (file[1] == '.' && file[2] == '/'))) 
		local = True ;
	else if( strncmp( file, "$HOME", 5) == 0 ) 
		local = True ;
	if( local ) 
	{
		path = put_file_home (file);
		if ( access (path, type) == 0 )
		{
			return path;
		}
		free (path);
		return NULL;
	}
/*	return put_file_home(file); */
	for (i = 0; file[i]; ++i);
	len = i ;
	for (ptr = (char *)pathlist; *ptr; ptr += i)
	{
		if (*ptr == ':')
			ptr++;
		for (i = 0; ptr[i] && ptr[i] != ':'; i++);
		if (i > max_path)
			max_path = i;
	}

	path = safemalloc (max_path + 1 + len + 1 + 100);
	strcpy( path+max_path+1, file );
	path[max_path] = '/' ;

	ptr = (char*)&(pathlist[0]) ;
	while( ptr[0] != '\0' )
	{
		for( i = 0 ; ptr[i] == ':' ; ++i );
		ptr += i ;
		for( i = 0 ; ptr[i] != ':' && ptr[i] != '\0' ; ++i );
		if( i > 0 && ptr[i-1] == '/' )
			i-- ;
		if( i > 0 )
		{
			register char *try_path = path+max_path-i;
			strncpy( try_path, ptr, i );
LOCAL_DEBUG_OUT( "errno = %d, file %s: checking path \"%s\"", errno, file, try_path );
			if ( access(try_path, type) == 0 )
			{
				char* res = mystrdup(try_path);
				free( path );
LOCAL_DEBUG_OUT( " found at: \"%s\"", res );
				return res;
			}
#ifdef LOCAL_DEBUG
			else
				show_system_error(" looking for file %s:", file );
#endif
		}
		if( ptr[i] == '/' )
			ptr += i+1 ;
		else
			ptr += i ;
	}
	free (path);
	return NULL;
}

char         *
find_envvar (char *var_start, int *end_pos)
{
	register int  i;
	static char tmp[256];

	if (var_start[0] == '{')
	{
		for (i = 1; var_start[i] && var_start[i] != '}' && i < 255; i++)
			tmp[i-1] = var_start[i] ;
		tmp[i-1] = '\0' ;
	} else
	{	
		for (i = 0; (isalnum ((int)var_start[i]) || var_start[i] == '_') && i < 255; i++)
			tmp[i] = var_start[i] ;
		tmp[i] = '\0';
	}		
	*end_pos = i;
	if (var_start[i] == '}')
		(*end_pos)++;
	
	return getenv (tmp);
}

static char *
do_replace_envvar (const char *path)
{
	char         *data = (char*)path, *tmp;
	char         *home = getenv ("HOME");
	int           pos = 0, len, home_len = 0;

	if (path == NULL)
		return NULL;
	if (*path == '\0')
		return (char*)path;
	len = strlen (path);
	if (home)
		home_len = strlen (home);

	while (*(data + pos))
	{
		char         *var;
		int           var_len, end_pos;

		while (*(data + pos) != '$' && *(data + pos))
		{
			if (*(data + pos) == '~' && *(data + pos + 1) == '/')
			{
				if (pos > 0)
					if (*(data + pos - 1) != ':')
					{
						pos += 2;
						continue;
					}
				if (home == NULL)
					*(data + (pos++)) = '.';
				else
				{
					len += home_len;
					tmp = safemalloc (len);
					strncpy (tmp, data, pos);
					strcpy (tmp + pos, home);
					strcpy (tmp + pos + home_len, data + pos + 1);
					if( data != path )
						free (data);
					data = tmp;
					pos += home_len;
				}
			}
			pos++;
		}
		if (*(data + pos) == '\0')
			break;
		/* found $ sign - trying to replace var */
		if ((var = find_envvar (data + pos + 1, &end_pos)) == NULL)
		{
			++pos;
			continue;
		}
		var_len = strlen (var);
		len += var_len;
		tmp = safemalloc (len);
		strncpy (tmp, data, pos);
		strcpy (tmp + pos, var);
		strcpy (tmp + pos + var_len, data + pos + end_pos + 1);
		if( data != path )
			free (data);
		data = tmp;
	}
	return data;
}

void
replace_envvar (char **path)
{
	char         *res = do_replace_envvar( *path );
	if( res != *path )
	{
		free( *path );
		*path = res ;
	}
}

char*
copy_replace_envvar (const char *path)
{
	char         *res = do_replace_envvar( path );
	return ( res == path )?mystrdup( res ):res;
}

/*
 * only checks first word in name, to allow full command lines with
 * options to be passed
 */
int
is_executable_in_path (const char *name)
{
	static char  *cache = NULL;
	static int    cache_result = 0, cache_len = 0, cache_size = 0;
	static char  *env_path = NULL;
	static int    max_path = 0;
	register int  i;

	if (name == NULL)
	{
		if (cache)
		{
			free (cache);
			cache = NULL;
		}
		cache_size = 0;
		cache_len = 0;
		if (env_path)
		{
			free (env_path);
			env_path = NULL;
		}
		max_path = 0;
		return 0;
	}

	/* cut leading "exec" enclosed in spaces */
	for (; isspace ((int)*name); name++);
	if (!mystrncasecmp(name, "exec", 4) && isspace ((int)name[4]))
		name += 4;
	for (; isspace ((int)*name); name++);
	if (*name == '\0')
		return 0;

	for (i = 0; name[i] && !isspace ((int)name[i]); i++);
	if (i == 0)
		return 0;

	if (cache)
		if (i == cache_len && strncmp (cache, name, i) == 0)
			return cache_result;

	if (i > cache_size)
	{
		if (cache)
			free (cache);
		/* allocating slightly more space then needed to avoid
		   too many reallocations */
		cache = (char *)safemalloc (i + (i >> 1) + 1);
		cache_size = i + (i >> 1);
	}
	strncpy (cache, name, i);
	cache[i] = '\0';
	cache_len = i;

	if (*cache == '/')
		cache_result = (CheckFile (cache) == 0) ? 1 : 0;
	else
	{
		char         *ptr, *path;
		struct stat   st;

		if (env_path == NULL)
		{
			env_path = mystrdup (getenv ("PATH"));
			replace_envvar (&env_path);
			for (ptr = env_path; *ptr; ptr += i)
			{
				if (*ptr == ':')
					ptr++;
				for (i = 0; ptr[i] && ptr[i] != ':'; i++);
				if (i > max_path)
					max_path = i;
			}
		}
		path = safemalloc (max_path + cache_len + 2);
		cache_result = 0;
		for (ptr = env_path; *ptr && cache_result == 0; ptr += i)
		{
			if (*ptr == ':')
				ptr++;
			for (i = 0; ptr[i] && ptr[i] != ':'; i++)
				path[i] = ptr[i];
			path[i] = '/';
			path[i + 1] = '\0';
			strcat (path, cache);
			if ((stat (path, &st) != -1) && (st.st_mode & S_IXUSR))
				cache_result = 1;
			LOCAL_DEBUG_OUT( "%s found \"%s\"", path, cache_result?"":"not" );
		}
		free (path);
	}
	return cache_result;
}


int
my_scandir_ext ( const char *dirname, int (*filter_func) (const char *),
				 Bool (*handle_direntry_func)( const char *fname, const char *fullname, struct stat *stat_info, void *aux_data), 
				 void *aux_data)
{
	DIR          *d;
	struct dirent *e;						   /* Pointer to static struct inside readdir() */
	int           n = 0;					   /* Count of nl used so far */
	char         *filename;					   /* For building filename to pass to stat */
	char         *p;						   /* Place where filename starts */
	struct stat   stat_info;

	d = opendir (dirname);

	if (d == NULL)
		return -1;

	filename = (char *)safemalloc (strlen (dirname) + PATH_MAX + 2);
	if (filename == NULL)
	{
		closedir (d);
		return -1;
	}
	strcpy (filename, dirname);
	p = filename + strlen (filename);
	if( *p != '/' )
	{	
		*p++ = '/';
		*p = 0;									   /* Just in case... */
	}
	
	while ((e = readdir (d)) != NULL)
	{
		if ((filter_func == NULL) || filter_func (&(e->d_name[0])))
		{
			/* Fill in the fields using stat() */
			strcpy (p, e->d_name);
			if (stat (filename, &stat_info) != -1)
			{	
				if( handle_direntry_func( e->d_name, filename, &stat_info, aux_data) )
					n++;
			}
		}
	}
	free (filename);

	if (closedir (d) == -1)
		return -1;
	/* Return the count of the entries */
	return n;
}


/*
 * Non-NULL select and dcomp pointers are *NOT* tested, but should be OK.
 * They are not used by afterstep however, so this implementation should
 * be good enough.
 *
 * c.ridd@isode.com
 */
/* essentially duplicates whats above, 
 * but for performance sake we don't want to merge them together :*/

int
my_scandir (char *dirname, struct direntry *(*namelist[]),
			int (*filter_func) (const char *), int (*dcomp) (struct direntry **, struct direntry **))
{
	DIR          *d;
	struct dirent *e;						   /* Pointer to static struct inside readdir() */
	struct direntry **nl;					   /* Array of pointers to dirents */
	struct direntry **nnl;
	int           n;						   /* Count of nl used so far */
	int           sizenl;					   /* Number of entries in nl array */
	int           j;
	char         *filename;					   /* For building filename to pass to stat */
	char         *p;						   /* Place where filename starts */
	struct stat   buf;

	*namelist = NULL ;

	d = opendir (dirname);

	if (d == NULL)
		return -1;

	filename = (char *)safemalloc (strlen (dirname) + PATH_MAX + 2);
	if (filename == NULL)
	{
		closedir (d);
		return -1;
	}
	strcpy (filename, dirname);
	p = filename + strlen (filename);
	if( *p != '/' )
	{	
		*p++ = '/';
		*p = 0;									   /* Just in case... */
	}

	nl = NULL;
	n = 0;
	sizenl = 0;

	while ((e = readdir (d)) != NULL)
	{
		if ((filter_func == NULL) || filter_func (&(e->d_name[0])))
		{
			/* add */
			if (sizenl == n)
			{
				/* Grow array */
				sizenl += 32;				   /* arbitrary delta */
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
			/* Fill in the fields using stat() */
			strcpy (p, e->d_name);
			if (stat (filename, &buf) != -1)
			{	
				size_t realsize = offsetof (struct direntry, d_name)+strlen (e->d_name) + 1;
				nl[n] = (struct direntry *)safemalloc (realsize);
				nl[n]->d_mode = buf.st_mode;
				nl[n]->d_mtime = buf.st_mtime;
				nl[n]->d_size  = buf.st_size;
				strcpy (nl[n]->d_name, e->d_name);
				n++;
			}
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
		if (nl)
			free (nl);
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
		qsort (*namelist, n, sizeof (struct direntry *), (int (*)())dcomp);

	/* Return the count of the entries */
	return n;
}

/*
 * Use this function as the filter_func argument to my_scandir to make it ignore
 * all files and directories starting with "."
 */
int
ignore_dots (const char *d_name)
{
	return (d_name[0] != '.');
}

int
no_dots_except_include (const char *d_name)
{
	if( d_name[0] != '.' || mystrcasecmp (d_name, ".include") == 0 )
	{	
		register  int i = 0;
		while( d_name[i] != '\0' ) ++i ; 
		return (i > 0 && d_name[i-1] != '~');
	}
	return False;
}

int
no_dots_except_directory (const char *d_name)
{
	if( d_name[0] != '.' || mystrcasecmp (d_name, ".directory") == 0 )
	{	
		register  int i = 0;
		while( d_name[i] != '\0' ) ++i ; 
		return (i > 0 && d_name[i-1] != '~');
	}
	return False;
}


