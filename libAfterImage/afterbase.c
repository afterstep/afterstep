/*
 * Copyright (C) 2001 Sasha Vasko <sasha at aftercode.net>
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
#ifdef _WIN32
#include "win32/config.h"
#else
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#include <sys/stat.h>

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

#ifdef _WIN32
#include "win32/afterbase.h"
#include <io.h>
#include <windows.h>
#define access _access
#else
#include "afterbase.h"
#endif

#ifdef X_DISPLAY_MISSING
#include "colornames.h"
#endif

/*#include <X11/Xlib.h>*/

Display *dpy = NULL ;
char    *asim_ApplicationName = NULL ;

void
asim_set_application_name (char *argv0)
{
	char *temp = &(argv0[0]);
	do
	{	/* Save our program name - for error messages */
		register int i = 1 ;                   /* we don't use standard strrchr since there
												* seems to be some wierdness in
												* CYGWIN implementation of it. */
		asim_ApplicationName =  temp ;
		while( temp[i] && temp[i] != '/' ) ++i ;
		temp = temp[i] ? &(temp[i]): NULL ;
	}while( temp != NULL );
}

const char *
asim_get_application_name()
{
	return asim_ApplicationName;
}

static unsigned int asim_as_output_threshold = OUTPUT_DEFAULT_THRESHOLD ;

unsigned int
asim_get_output_threshold()
{
	return asim_as_output_threshold ;
}

unsigned int
asim_set_output_threshold( unsigned int threshold )
{
    unsigned int old = asim_as_output_threshold;
    asim_as_output_threshold = threshold ;
    return old;
}


/* from libAfterBase/output.c : */
Bool asim_show_error( const char *error_format, ...)
{
    if( OUTPUT_LEVEL_ERROR <= get_output_threshold())
    {
        va_list ap;
        fprintf (stderr, "%s ERROR: ", get_application_name() );
        va_start (ap, error_format);
        vfprintf (stderr, error_format, ap);
        va_end (ap);
        fprintf (stderr, "\n" );
        return True;
    }
    return False;
}

Bool asim_show_warning( const char *warning_format, ...)
{
    if( OUTPUT_LEVEL_WARNING <= get_output_threshold())
    {
        va_list ap;
        fprintf (stderr, "%s warning: ", get_application_name() );
        va_start (ap, warning_format);
        vfprintf (stderr, warning_format, ap);
        va_end (ap);
        fprintf (stderr, "\n" );
        return True;
    }
    return False;
}

Bool asim_show_progress( const char *msg_format, ...)
{
    if( OUTPUT_LEVEL_PROGRESS <= get_output_threshold())
    {
        va_list ap;
        fprintf (stderr, "%s : ", get_application_name() );
        va_start (ap, msg_format);
        vfprintf (stderr, msg_format, ap);
        va_end (ap);
        fprintf (stderr, "\n" );
        return True;
    }
    return False;
}


Bool asim_show_debug( const char *file, const char *func, int line, const char *msg_format, ...)
{
    if( OUTPUT_LEVEL_DEBUG <= get_output_threshold())
    {
        va_list ap;
        fprintf (stderr, "%s debug msg: %s:%s():%d: ", get_application_name(), file, func, line );
        va_start (ap, msg_format);
        vfprintf (stderr, msg_format, ap);
        va_end (ap);
        fprintf (stderr, "\n" );
        return True;
    }
    return False;
}


void asim_nonGNUC_debugout( const char *format, ...)
{
    va_list ap;
    fprintf (stderr, "%s: ", get_application_name() );
    va_start (ap, format);
    vfprintf (stderr, format, ap);
    va_end (ap);
    fprintf (stderr, "\n" );
}

inline void asim_nonGNUC_debugout_stub( const char *format, ...)
{}

/* from libAfterBase/fs.c : */

int		asim_check_file_mode (const char *file, int mode)
{
	struct stat   st;

	if ((stat (file, &st) == -1) || (st.st_mode & S_IFMT) != mode)
		return (-1);
	else
		return (0);
}

char         *
asim_put_file_home (const char *path_with_home)
{
	static char  *home = NULL;				   /* the HOME environment variable */
	static char   default_home[3] = "./";
	static int    home_len = 0;
	char         *str = NULL, *ptr;
	register int  i;
	if (path_with_home == NULL)
		return NULL;
	/* home dir ? */
	if (path_with_home[0] != '~' || path_with_home[1] != '/')
	{
		char *t = mystrdup (path_with_home);
		return t;
	}

	if (home == NULL)
	{
		if ((home = getenv ("HOME")) == NULL)
			home = &(default_home[0]);
		home_len = strlen (home);
	}

	for (i = 2; path_with_home[i]; i++);
	str = safemalloc (home_len + i);
	for (ptr = str + home_len-1; i > 0; i--)
		ptr[i] = path_with_home[i];
	for (i = 0; i < home_len; i++)
		str[i] = home[i];
	return str;
}

char*
asim_load_file(const char* realfilename)
{
	struct stat st;
	FILE* fp;
	char* str;
	int len;

	/* Get the file size. */
	if (stat(realfilename, &st)) return NULL;

	/* Open the file. */
	fp = fopen(realfilename, "rb");
	if (!(fp = fopen(realfilename, "rb"))) return NULL;

	/* Read in the file. */
	str = NEW_ARRAY(char, st.st_size + 1);
	len = fread(str, 1, st.st_size, fp);
	if (len >= 0) str[len] = '\0';

	fclose(fp);
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
void 
unix_path2dos_path( char *path )
{
	int i = strlen(path) ; 
	while( --i >= 0 ) 
		if( path[i] == '/' && ( i == 0 || path[i-1] != '/' ) )
			path[i] = '\\' ;
}		   


char         *
asim_find_file (const char *file, const char *pathlist, int type)
{
	char 		  *path;
	register int   len;
	int            max_path = 0;
	register char *ptr;
	register int   i;

	if (file == NULL)
		return NULL;
#ifdef _WIN32
#define PATH_SEPARATOR_CHAR ';'
#define PATH_CHAR '\\'
#else
#define PATH_SEPARATOR_CHAR ':'
#define PATH_CHAR '/'
#endif

	if (*file == PATH_CHAR || *file == '~' || ((pathlist == NULL) || (*pathlist == '\0')))
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
	for (i = 0; file[i]; i++);
	len = i ;
	for (ptr = (char *)pathlist; *ptr; ptr += i)
	{
		if (*ptr == ':' || *ptr == PATH_SEPARATOR_CHAR)
			ptr++;
		for (i = 0; ptr[i] && ptr[i] != ':' && ptr[i] != PATH_SEPARATOR_CHAR; i++);
		if (i > max_path)
			max_path = i;
	}

	path = safemalloc (max_path + 1 + len + 1 + 100);
	strcpy( path+max_path+1, file );
	path[max_path] = PATH_CHAR ;

	ptr = (char*)&(pathlist[0]) ;
	while( ptr[0] != '\0' )
	{
		for( i = 0 ; ptr[i] == ':' || ptr[i] == PATH_SEPARATOR_CHAR; ++i );
		ptr += i ;
		for( i = 0 ; ptr[i] != ':' && ptr[i] != PATH_SEPARATOR_CHAR && ptr[i] != '\0' ; ++i );
		if( i > 0 )
		{
			strncpy( path+max_path-i, ptr, i );
			if (access(path, type) == 0)
			{
				char* res = mystrdup(path+max_path-i);
				free( path );
				return res;
			}
		}
		ptr += i ;
	}
	free (path);
	return NULL;
}

static char         *
find_envvar (char *var_start, int *end_pos)
{
	char          backup, *name_start = var_start;
	register int  i;
	char         *var = NULL;

	if (var_start[0] == '{')
	{
		name_start++;
		for (i = 1; var_start[i] && var_start[i] != '}'; i++);
	} else
		for (i = 0; isalnum ((int)var_start[i]) || var_start[i] == '_'; i++);

	backup = var_start[i];
	var_start[i] = '\0';
	var = getenv (name_start);
	var_start[i] = backup;

	*end_pos = i;
	if (backup == '}')
		(*end_pos)++;
	return var;
}

static char *
do_replace_envvar (char *path)
{
	char         *data = path, *tmp;
	char         *home = getenv ("HOME");
	int           pos = 0, len, home_len = 0;

	if (path == NULL)
		return NULL;
	if (*path == '\0')
		return path;
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

char*
asim_copy_replace_envvar (char *path)
{
	char         *res = do_replace_envvar( path );
	return ( res == path )?mystrdup( res ):res;
}


/*******************************************************************/
/* from mystring.c : */
char         *
asim_mystrndup (const char *str, size_t n)
{
	char         *c = NULL;
	if (str)
	{
		c = malloc (n + 1);
		strncpy (c, str, n);
		c[n] = '\0';
	}
	return c;
}

#ifdef X_DISPLAY_MISSING
static int compare_xcolor_entries(const void *a, const void *b)
{
   return strcmp((const char *) a, ((const XColorEntry *) b)->name);
}

static int FindColor(const char *name, CARD32 *colorPtr)
{
   XColorEntry *found;

   found = bsearch(name, xColors, numXColors, sizeof(XColorEntry),
                   compare_xcolor_entries);
   if (found == NULL)
      return 0;

   *colorPtr = 0xFF000000|((found->red<<16)&0x00FF0000)|((found->green<<8)&0x0000FF00)|((found->blue)&0x000000FF);
   return 1;
}
#endif

/*******************************************************************/
/* from parse,c : */
const char *asim_parse_argb_color( const char *color, CARD32 *pargb )
{
#define hextoi(h)   (isdigit(h)?((h)-'0'):(isupper(h)?((h)-'A'+10):((h)-'a'+10)))
	if( color )
	{
		if( *color == '#' )
		{
			CARD32 argb = 0 ;
			int len = 0 ;
			register const char *ptr = color+1 ;
			while( isxdigit((int)ptr[len]) ) len++;
			if( len >= 3)
			{
				if( (len&0x3) == 0 && len != 12 )
				{  /* we do have alpha channel !!! */
					len = len>>2 ;
					argb = (hextoi((int)ptr[0])<<28)&0xF0000000 ;
					if( len > 1 )
						argb |= (hextoi((int)ptr[1])<<24)&0x0F000000 ;
					else
						argb |= 0x0F000000;
					ptr += len ;
				}else
				{
					len = len/3 ;
					argb = 0xFF000000;
				}
				/* processing rest of the channels : */
				if( len == 1 )
				{
					argb |= 0x000F0F0F;
					argb |= (hextoi((int)ptr[0])<<20)&0x00F00000 ;
					argb |= (hextoi((int)ptr[1])<<12)&0x0000F000 ;
					argb |= (hextoi((int)ptr[2])<<4 )&0x000000F0 ;
					ptr += 3 ;
				}else
				{
					argb |= (hextoi((int)ptr[0])<<20)&0x00F00000 ;
					argb |= (hextoi((int)ptr[1])<<16)&0x000F0000 ;
					ptr += len ;
					argb |= (hextoi((int)ptr[0])<<12)&0x0000F000 ;
					argb |= (hextoi((int)ptr[1])<<8) &0x00000F00 ;
					ptr += len ;
					argb |= (hextoi((int)ptr[0])<<4 )&0x000000F0 ;
					argb |= (hextoi((int)ptr[1]))    &0x0000000F ;
					ptr += len ;
				}
				*pargb = argb ;
				return ptr;
			}
		}else if( *color )
		{
			/* does not really matter here what screen to use : */
#ifdef X_DISPLAY_MISSING
			register const char *ptr = &(color[0]);
            if(!FindColor(color, pargb))
                return color;
    		while( !isspace((int)*ptr) && *ptr != '\0' ) ptr++;
			return ptr;
#else
			if( dpy == NULL )
				return color ;
			else
			{
				register const char *ptr = &(color[0]);
#ifndef X_DISPLAY_MISSING
				XColor xcol, xcol_scr ;
/* XXX Not sure if Scr.asv->colormap is always defined here.  If not,
** change back to DefaultColormap(dpy,DefaultScreen(dpy)). */
				if( XLookupColor( dpy, DefaultColormap(dpy,DefaultScreen(dpy)), color, &xcol, &xcol_scr) )
					*pargb = 0xFF000000|((xcol.red<<8)&0x00FF0000)|(xcol.green&0x0000FF00)|((xcol.blue>>8)&0x000000FF);
#endif
				while( !isspace((int)*ptr) && *ptr != '\0' ) ptr++;
				return ptr;
			}
#endif
		}
	}
	return color;
}

/*******************************************************************/
/* from ashash,c : */
ASHashKey asim_default_hash_func (ASHashableValue value, ASHashKey hash_size)
{
	return (ASHashKey)(value % hash_size);
}

long
asim_default_compare_func (ASHashableValue value1, ASHashableValue value2)
{
	return ((long)value1 - (long)value2);
}

long
asim_desc_long_compare_func (ASHashableValue value1, ASHashableValue value2)
{
    return ((long)value2 - (long)value1);
}

void
asim_init_ashash (ASHashTable * hash, Bool freeresources)
{
LOCAL_DEBUG_CALLER_OUT( " has = %p, free ? %d", hash, freeresources );
	if (hash)
	{
		if (freeresources)
			if (hash->buckets)
				free (hash->buckets);
		memset (hash, 0x00, sizeof (ASHashTable));
	}
}

ASHashTable  *
asim_create_ashash (ASHashKey size,
			   ASHashKey (*hash_func) (ASHashableValue, ASHashKey),
			   long (*compare_func) (ASHashableValue, ASHashableValue),
			   void (*item_destroy_func) (ASHashableValue, void *))
{
	ASHashTable  *hash;

	if (size <= 0)
		size = 63;

	hash = safemalloc (sizeof (ASHashTable));
	init_ashash (hash, False);

	hash->buckets = safemalloc (sizeof (ASHashBucket) * size);
	memset (hash->buckets, 0x00, sizeof (ASHashBucket) * size);

	hash->size = size;

	if (hash_func)
		hash->hash_func = hash_func;
	else
		hash->hash_func = asim_default_hash_func;

	if (compare_func)
		hash->compare_func = compare_func;
	else
		hash->compare_func = asim_default_compare_func;

	hash->item_destroy_func = item_destroy_func;

	return hash;
}

static void
destroy_ashash_bucket (ASHashBucket * bucket, void (*item_destroy_func) (ASHashableValue, void *))
{
	register ASHashItem *item, *next;

	for (item = *bucket; item != NULL; item = next)
	{
		next = item->next;
		if (item_destroy_func)
			item_destroy_func (item->value, item->data);
		free (item);
	}
	*bucket = NULL;
}

void
asim_destroy_ashash (ASHashTable ** hash)
{
LOCAL_DEBUG_CALLER_OUT( " hash = %p, *hash = %p", hash, *hash  );
	if (*hash)
	{
		register int  i;

		for (i = (*hash)->size - 1; i >= 0; i--)
			if ((*hash)->buckets[i])
				destroy_ashash_bucket (&((*hash)->buckets[i]), (*hash)->item_destroy_func);

		asim_init_ashash (*hash, True);
		free (*hash);
		*hash = NULL;
	}
}

static        ASHashResult
add_item_to_bucket (ASHashBucket * bucket, ASHashItem * item, long (*compare_func) (ASHashableValue, ASHashableValue))
{
	ASHashItem  **tmp;

	/* first check if we already have this item */
	for (tmp = bucket; *tmp != NULL; tmp = &((*tmp)->next))
	{
		register long res = compare_func ((*tmp)->value, item->value);

		if (res == 0)
			return ((*tmp)->data == item->data) ? ASH_ItemExistsSame : ASH_ItemExistsDiffer;
		else if (res > 0)
			break;
	}
	/* now actually add this item */
	item->next = (*tmp);
	*tmp = item;
	return ASH_Success;
}

#define DEALLOC_CACHE_SIZE      1024
static ASHashItem*  deallocated_mem[DEALLOC_CACHE_SIZE+10] ;
static unsigned int deallocated_used = 0 ;

ASHashResult
asim_add_hash_item (ASHashTable * hash, ASHashableValue value, void *data)
{
	ASHashKey     key;
	ASHashItem   *item;
	ASHashResult  res;

	if (hash == NULL)
        return ASH_BadParameter;

	key = hash->hash_func (value, hash->size);
	if (key >= hash->size)
        return ASH_BadParameter;

    if( deallocated_used > 0 )
        item = deallocated_mem[--deallocated_used];
    else
        item = safemalloc (sizeof (ASHashItem));

	item->next = NULL;
	item->value = value;
	item->data = data;

	res = add_item_to_bucket (&(hash->buckets[key]), item, hash->compare_func);
	if (res == ASH_Success)
	{
		hash->most_recent = item ;
		hash->items_num++;
		if (hash->buckets[key]->next == NULL)
			hash->buckets_used++;
	} else
		free (item);
	return res;
}

static ASHashItem **
find_item_in_bucket (ASHashBucket * bucket,
					 ASHashableValue value, long (*compare_func) (ASHashableValue, ASHashableValue))
{
	register ASHashItem **tmp;
	register long res;

	/* first check if we already have this item */
	for (tmp = bucket; *tmp != NULL; tmp = &((*tmp)->next))
	{
		res = compare_func ((*tmp)->value, value);
		if (res == 0)
			return tmp;
		else if (res > 0)
			break;
	}
	return NULL;
}

ASHashResult
asim_get_hash_item (ASHashTable * hash, ASHashableValue value, void **trg)
{
	ASHashKey     key;
	ASHashItem  **pitem = NULL;

	if (hash)
	{
		key = hash->hash_func (value, hash->size);
		if (key < hash->size)
			pitem = find_item_in_bucket (&(hash->buckets[key]), value, hash->compare_func);
	}
	if (pitem)
		if (*pitem)
		{
			if (trg)
				*trg = (*pitem)->data;
			return ASH_Success;
		}
	return ASH_ItemNotExists;
}

ASHashResult
asim_remove_hash_item (ASHashTable * hash, ASHashableValue value, void **trg, Bool destroy)
{
	ASHashKey     key = 0;
	ASHashItem  **pitem = NULL;

	if (hash)
	{
		key = hash->hash_func (value, hash->size);
		if (key < hash->size)
			pitem = find_item_in_bucket (&(hash->buckets[key]), value, hash->compare_func);
	}
	if (pitem)
		if (*pitem)
		{
			ASHashItem   *next;

			if( hash->most_recent == *pitem )
				hash->most_recent = NULL ;

			if (trg)
				*trg = (*pitem)->data;

			next = (*pitem)->next;
			if (hash->item_destroy_func && destroy)
				hash->item_destroy_func ((*pitem)->value, (trg) ? NULL : (*pitem)->data);

            if( deallocated_used < DEALLOC_CACHE_SIZE )
            {
                deallocated_mem[deallocated_used++] = *pitem ;
            }else
                free( *pitem );

            *pitem = next;
			if (hash->buckets[key] == NULL)
				hash->buckets_used--;
			hash->items_num--;

			return ASH_Success;
		}
	return ASH_ItemNotExists;
}

void asim_flush_ashash_memory_pool()
{
	/* we better disable errors as some of this data will belong to memory audit : */
	while( deallocated_used > 0 )
		free( deallocated_mem[--deallocated_used] );
}

/************************************************************************/
/************************************************************************/
/* 	Some usefull implementations 					*/
/************************************************************************/

/* case sensitive strings hash */
ASHashKey
asim_string_hash_value (ASHashableValue value, ASHashKey hash_size)
{
	ASHashKey     hash_key = 0;
	register int  i = 0;
	char         *string = (char*)value;
	register char c;

	do
	{
		c = string[i];
		if (c == '\0')
			break;
		hash_key += (((ASHashKey) c) << i);
		++i ;
	}while( i < ((sizeof (ASHashKey) - sizeof (char)) << 3) );
	return hash_key % hash_size;
}

long
asim_string_compare (ASHashableValue value1, ASHashableValue value2)
{
	register char *str1 = (char*)value1;
	register char *str2 = (char*)value2;
	register int   i = 0 ;

	if (str1 == str2)
		return 0;
	if (str1 == NULL)
		return -1;
	if (str2 == NULL)
		return 1;
	do
	{
		if (str1[i] != str2[i])
			return (long)(str1[i]) - (long)(str2[i]);

	}while( str1[i++] );
	return 0;
}

void
asim_string_destroy (ASHashableValue value, void *data)
{
	if ((char*)value != NULL)
		free ((char*)value);
	if (data != (void*)value && data != NULL)
		free (data);
}

/* variation for case-unsensitive strings */
ASHashKey
asim_casestring_hash_value (ASHashableValue value, ASHashKey hash_size)
{
	ASHashKey     hash_key = 0;
	register int  i = 0;
	char         *string = (char*)value;
	register int c;

	do
	{
		c = string[i];
		if (c == '\0')
			break;
		if (isupper (c))
			c = tolower (c);
		hash_key += (((ASHashKey) c) << i);
		++i;
	}while(i < ((sizeof (ASHashKey) - sizeof (char)) << 3));

	return hash_key % hash_size;
}

long
asim_casestring_compare (ASHashableValue value1, ASHashableValue value2)
{
	register char *str1 = (char*)value1;
	register char *str2 = (char*)value2;
	register int   i = 0;

	if (str1 == str2)
		return 0;
	if (str1 == NULL)
		return -1;
	if (str2 == NULL)
		return 1;
	do
	{
		int          u1, u2;

		u1 = str1[i];
		u2 = str2[i];
		if (islower (u1))
			u1 = toupper (u1);
		if (islower (u2))
			u2 = toupper (u2);
		if (u1 != u2)
			return (long)u1 - (long)u2;
	}while( str1[i++] );
	return 0;
}

int
asim_get_drawable_size (Drawable d, unsigned int *ret_w, unsigned int *ret_h)
{
	*ret_w = 0;
	*ret_h = 0;
#ifndef X_DISPLAY_MISSING
	if( d )
	{
		Window        root;
		unsigned int  ujunk;
		int           junk;
		if (XGetGeometry (dpy, d, &root, &junk, &junk, ret_w, ret_h, &ujunk, &ujunk) != 0)
			return 1;
	}
#endif
	return 0;
}

#ifdef X_DISPLAY_MISSING
int XParseGeometry (  char *string,int *x,int *y,
                      unsigned int *width,    /* RETURN */
					  unsigned int *height)    /* RETURN */
{
	show_error( "Parsing of geometry is not supported without either Xlib opr libAfterBase" );
	return 0;
}
void XDestroyImage( void* d){}
int XGetWindowAttributes( void*d, Window w, unsigned long m, void* s){  return 0;}
void *XGetImage( void* dpy,Drawable d,int x,int y,unsigned int width,unsigned int height, unsigned long m,int t)
{return NULL ;}
unsigned long XGetPixel(void* d, int x, int y){return 0;}
int XQueryColors(void* a,Colormap c,void* x,int m){return 0;}
#endif


/***************************************/
/* from sleep.c                        */
/***************************************/
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifndef _WIN32
# include <sys/times.h>
#endif
static clock_t _as_ticker_last_tick = 0;
static clock_t _as_ticker_tick_size = 1;
static clock_t _as_ticker_tick_time = 0;

/**************************************************************************
 * Sleep for n microseconds
 *************************************************************************/
void
sleep_a_little (int n)
{
#ifndef _WIN32
	struct timeval value;

	if (n <= 0)
		return;

	value.tv_usec = n % 1000000;
	value.tv_sec = n / 1000000;

#ifndef PORTABLE_SELECT
#ifdef __hpux
#define PORTABLE_SELECT(w,i,o,e,t)	select((w),(int *)(i),(int *)(o),(e),(t))
#else
#define PORTABLE_SELECT(w,i,o,e,t)	select((w),(i),(o),(e),(t))
#endif
#endif
	PORTABLE_SELECT (1, 0, 0, 0, &value);
#else /* win32 : */
	Sleep(n);
#endif
}

void
asim_start_ticker (unsigned int size)
{
#ifndef _WIN32
	struct tms    t;

	_as_ticker_last_tick = times (&t);		   /* in system ticks */
	if (_as_ticker_tick_time == 0)
	{
		register clock_t delta = _as_ticker_last_tick;
		/* calibrating clock - how many ms per cpu tick ? */
		sleep_a_little (100);
		_as_ticker_last_tick = times (&t);
		delta = _as_ticker_last_tick - delta ;
		if( delta <= 0 )
			_as_ticker_tick_time = 100;
		else
			_as_ticker_tick_time = 101 / delta;
	}
#else
	_as_ticker_tick_time = 1000;
	_as_ticker_last_tick = time(NULL) ;
#endif
	_as_ticker_tick_size = size;			   /* in ms */

}

void
asim_wait_tick ()
{
#ifndef _WIN32
	struct tms    t;
	register clock_t curr = (times (&t) - _as_ticker_last_tick) * _as_ticker_tick_time;
#else
	register int curr = (time(NULL) - _as_ticker_last_tick) * _as_ticker_tick_time;
#endif

	if (curr < _as_ticker_tick_size)
		sleep_a_little (_as_ticker_tick_size - curr);

#ifndef _WIN32
	_as_ticker_last_tick = times (&t);
#else
	_as_ticker_last_tick = time(NULL) ;
#endif
}

#ifndef _WIN32
/*
 * Non-NULL select and dcomp pointers are *NOT* tested, but should be OK.
 * They are not used by afterstep however, so this implementation should
 * be good enough.
 *
 * c.ridd@isode.com
 */
int
asim_my_scandir (char *dirname, struct direntry *(*namelist[]),
			int (*select) (const char *), int (*dcomp) (struct direntry **, struct direntry **))
{
	DIR          *d;
	struct dirent *e;						   /* Pointer to static struct inside readdir() */
	struct direntry **nl;					   /* Array of pointers to dirents */
	struct direntry **nnl;
	int           n;						   /* Count of nl used so far */
	int           sizenl;					   /* Number of entries in nl array */
	int           j;
	size_t        realsize;
	char         *filename;					   /* For building filename to pass to stat */
	char         *p;						   /* Place where filename starts */
	struct stat   buf;


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
	*p++ = '/';
	*p = 0;									   /* Just in case... */

	nl = NULL;
	n = 0;
	sizenl = 0;

	while ((e = readdir (d)) != NULL)
	{
		if ((select == NULL) || select (&(e->d_name[0])))
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
			realsize = offsetof (struct direntry, d_name)+strlen (e->d_name) + 1;
			nl[n] = (struct direntry *)safemalloc (realsize);
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
#endif
