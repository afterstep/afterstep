/*
 * Copyright (c) 1998,1999,2000 Ethan Fischer <allanon@crystaltokyo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/****************************************************************************
 *
 * dirtree.c: converts a menu directory tree into a configuration file
 *
 ***************************************************************************/

#define DIRTREE_C

#include <sys/stat.h>

#include "../../configure.h"

#include "../../libAfterStep/asapp.h"
#include "dirtree.h"
void          init_func_data (FunctionData * data);
int           txt2func (const char *text, FunctionData * fdata, int quiet);
int           free_func_data (FunctionData * data);

dirtree_compar_f dirtree_compar_list[] = {
	dirtree_compar_order,
	dirtree_compar_type,
	dirtree_compar_alpha,
	NULL
};

dirtree_t    *
dirtree_new (void)
{
	dirtree_t    *tree = safecalloc (1,sizeof (dirtree_t));

	init_func_data (&tree->command);
	tree->order = 10000;

	return tree;
}

void
dirtree_remove (dirtree_t * tree)
{
	if (tree->parent != NULL && tree->parent->child != NULL)
	{
		if (tree->parent->child == tree)
			tree->parent->child = tree->next;
		else
		{
			dirtree_t    *t;

			for (t = tree->parent->child; t->next != NULL; t = t->next)
				if (t->next == tree)
				{
					t->next = tree->next;
					break;
				}
		}
		tree->next = tree->parent = NULL;
	}
}

void
dirtree_delete (dirtree_t * tree)
{
	/* find and remove ourself from our parent's list */
	dirtree_remove (tree);

	/* kill our children */
	while (tree->child != NULL)
		dirtree_delete (tree->child);

	/* free members */
	if (tree->name != NULL)
		free (tree->name);
	if (tree->stripped_name != NULL && tree->stripped_name != tree->name )
		free (tree->stripped_name);
	if (tree->path != NULL)
		free (tree->path);
	if (tree->icon != NULL)
		free (tree->icon);
	if (tree->extension != NULL)
		free (tree->extension);
	if (tree->minipixmap_extension != NULL)
		free (tree->minipixmap_extension);
	free_func_data (&tree->command);

	free (tree);
}

int
no_dots_except_include (struct dirent *e)
{
	return !(e->d_name[0] == '.' && mystrcasecmp (e->d_name, ".include"));
}

char         *
make_absolute (const char *path1, const char *path2)
{
	char         *path;

	if (*path2 == '/' || *path2 == '~')
		path = PutHome (path2);
	else
		/* relative path */
	{
		path = safemalloc (strlen (path1) + strlen (path2) + 2);
		sprintf (path, "%s/%s", path1, path2);
	}
	return path;
}

char *strip_extension( char *name, char *ext )
{
	if( name && ext )
	{
		int           nlen = strlen (name);
		int           elen = strlen (ext);

		if (nlen >= elen)
		{
			if (!strcmp (name + nlen - elen, ext))
				return mystrndup(name, nlen - elen);
			else if (!strncmp (name, ext, elen))
				return mystrndup(name+elen, nlen - elen);
		}
	}
	return name;
}

/* assumes that tree->name and tree->path are already filled in */
void
dirtree_fill_from_dir (dirtree_t * tree)
{
	struct direntry **list;
	int           i, n;

	n = my_scandir (tree->path, &list, no_dots_except_include, NULL);
	for (i = 0; i < n; i++)
	{
		dirtree_t    *t = dirtree_new ();

		t->name = mystrdup (list[i]->d_name);
		t->path = make_absolute (tree->path, t->name);
		if (S_ISDIR (list[i]->d_mode))
			t->flags |= DIRTREE_DIR;
		t->mtime = list[i]->d_mtime;

		dirtree_fill_from_dir (t);
		t->parent = tree;
		t->next = tree->child;
		tree->child = t;

		free (list[i]);
	}
	if (n > 0)
	{
		tree->flags |= DIRTREE_DIR;
		free (list);
	}
}

dirtree_t    *
dirtree_new_from_dir (const char *dir)
{
	dirtree_t    *tree = dirtree_new ();
	const char   *p;
	const char   *e;

	for (e = dir + strlen (dir); e > dir && (*e - 1) == '/'; e--);
	for (p = e; p > dir && *(p - 1) != '/'; p--);
	tree->name = mystrndup (p, e - p);
	tree->path = mystrdup (dir);

	dirtree_fill_from_dir (tree);

	return tree;
}

/* move tree2's children to tree1 */
void
dirtree_move_children (dirtree_t * tree1, dirtree_t * tree2)
{
	if (tree2->child != NULL)
	{
		dirtree_t    *t1, *t2 = NULL;

		for (t1 = tree2->child; t1 != NULL; t2 = t1, t1 = t1->next)
			t1->parent = tree1;
		if (t2 != NULL)
		{
			t2->next = tree1->child;
			tree1->child = tree2->child;
		}
		tree2->child = NULL;
	}
}

void
dirtree_set_command (dirtree_t * tree, struct FunctionData *command, int recurse)
{
	dirtree_t    *t;

	for (t = tree->child; t != NULL; t = t->next)
	{
		t->command = *command;
		if (t->command.text != NULL)
			t->command.text = mystrdup (t->command.text);
		if (recurse)
			dirtree_set_command (t, command, 1);
	}
}

int
dirtree_parse (dirtree_t * tree, const char *file)
{
	FILE         *fp;
	char         *str;

	if ((fp = fopen (file, "r")) == NULL)
		return 1;
	str = safemalloc (8192);
	while (fgets (str, 8192, fp) != NULL)
	{
		char         *ptr;

		ptr = strip_whitespace (str);
		/* ignore comments and blank lines */
		if (*ptr == '#' || *ptr == '\0')
			continue;
		if (!mystrncasecmp (ptr, "include", 7))
		{
			char         *path;
			dirtree_t    *t;

			for (ptr += 7; isspace (*ptr); ptr++);
			if (*ptr != '"')
				continue;
			path = ++ptr;
			for (; *ptr != '\0' && *ptr != '"'; ptr++);
			if (*ptr == '"')
				for (*ptr++ = '\0'; isspace (*ptr); ptr++);
			path = make_absolute (tree->path, path);
			t = dirtree_new_from_dir (path);
			free (path);
			if (t != NULL)
			{
				if (*ptr != '\0')
				{
					txt2func (ptr, &t->command, False);
					dirtree_set_command (t, &t->command, 1);
				}

				/* included dir might have a .include */
				dirtree_parse_include (t);
				dirtree_move_children (tree, t);
				dirtree_delete (t);
			}
		} else if (!mystrncasecmp (ptr, "keepname", 8))
			tree->flags |= DIRTREE_KEEPNAME;
		else if (!mystrncasecmp (ptr, "extension", 9))
		{
			char         *tmp;

			for (ptr += 9; isspace (*ptr); ptr++);
			for (tmp = ptr + strlen (ptr); tmp > ptr && isspace (*(tmp - 1)); tmp--);
			if (tmp != ptr)
				tree->extension = mystrndup (ptr, tmp - ptr);
		}else if (!mystrncasecmp (ptr, "miniextension", 13))
		{
			char         *tmp;

			for (ptr += 13; isspace (*ptr); ptr++);
			for (tmp = ptr + strlen (ptr); tmp > ptr && isspace (*(tmp - 1)); tmp--);
			if (tmp != ptr)
				tree->minipixmap_extension = mystrndup (ptr, tmp - ptr);
		} else if (!mystrncasecmp (ptr, "minipixmap", 10))
		{
			for (ptr += 10; isspace (*ptr); ptr++);
			if (tree->icon != NULL)
				free (tree->icon);
			tree->icon = mystrdup (ptr);
		} else if (!mystrncasecmp (ptr, "command", 7))
		{
			for (ptr += 7; isspace (*ptr); ptr++);
			txt2func (ptr, &tree->command, False);
			dirtree_set_command (tree, &tree->command, 0);
		} else if (!mystrncasecmp (ptr, "order", 5))
			tree->order = strtol (ptr + 5, NULL, 10);
		else if (!mystrncasecmp (ptr, "name", 4))
		{
			for (ptr += 4; isspace (*ptr); ptr++);
			if (tree->name != NULL)
				free (tree->name);
			tree->name = mystrdup (ptr);
		}
	}
	free (str);
	fclose (fp);
	return 0;
}

void
dirtree_parse_include (dirtree_t * tree)
{
	dirtree_t    *t;

	/* parse the first .include */
	for (t = tree->child; t != NULL; t = t->next)
		if (t->name[0] == '.')
		{
			dirtree_remove (t);
			dirtree_parse (tree, t->path);
			dirtree_delete (t);
			break;
		}

	/* nuke any other .include's */
	for (t = tree->child; t != NULL; t = t->next)
		if (t->name[0] == '.')
			dirtree_delete (t);

	for (t = tree->child; t != NULL; t = t->next)
		dirtree_parse_include (t);
}

void
dirtree_remove_order (dirtree_t * tree)
{
	dirtree_t    *t;
	char         *ptr;
	int           order = strtol (tree->name, &ptr, 10);

	if (ptr != tree->name && *ptr == '_')
	{
		tree->order = order;
		memmove (tree->name, ptr + 1, strlen (ptr + 1) + 1);
	}
	for (t = tree->child; t != NULL; t = t->next)
		dirtree_remove_order (t);
}

void
dirtree_merge (dirtree_t * tree)
{
	dirtree_t    *t;
	/* PASS1: merge all the subdirs of the current dir */
	for (t = tree->child; t != NULL; t = t->next)
	{
		if( t->stripped_name == NULL )
		{
			t->stripped_name = tree->extension?strip_extension( t->name, tree->extension ):t->name;
			if( t->stripped_name == t->name && tree->minipixmap_extension )
			{
				t->stripped_name = strip_extension( t->name, tree->minipixmap_extension );
				if( t->stripped_name != t->name )
					set_flags( t->flags, DIRTREE_MINIPIXMAP );
			}
		}
		if (t->flags & DIRTREE_DIR)
		{
			dirtree_t    *t2;

			for (t2 = t->next; t2 != NULL;)
			{
				if ((t2->flags & DIRTREE_DIR) && !strcmp (t->name, t2->name))
				{
					if (t2->order != -1)
						t->order = t2->order;
					dirtree_remove (t2);
					dirtree_move_children (t, t2);
					dirtree_delete (t2);
					t2 = t->next;
				} else
					t2 = t2->next;
			}
		}
	}
	/* PASS2: attach all the minipixmaps : */
	for (t = tree->child; t != NULL; t = t->next)
		if( get_flags( t->flags, DIRTREE_MINIPIXMAP ))
		{	/* let us try and find matching filename */
			dirtree_t    *t2;

			for (t2 = tree->child; t2 != NULL; t2 = t2->next )
			{
				if ( t2 != t && !strcmp(t2->stripped_name, t->stripped_name ) )
				{
					if( t2->icon )
						free( t2->icon );
					t2->icon = mystrdup( t->path );
				}
			}
		}

	/* PASS3: merge all the subdirs : */
	for (t = tree->child; t != NULL; t = t->next)
		dirtree_merge (t);
}

/* sort entries based on the array dirtree_compar_list */
int
dirtree_compar (const dirtree_t ** d1, const dirtree_t ** d2)
{
	int           diff = 0;
	dirtree_compar_f *compar;

	for (compar = dirtree_compar_list; diff == 0 && *compar != NULL; compar++)
		diff = (*compar) (d1, d2);
	return diff;
}

/* sort entries based their order; 1 comes before 2 */
int
dirtree_compar_order (const dirtree_t ** d1, const dirtree_t ** d2)
{
	return (**d1).order - (**d2).order;
}

/* sort entries based on their type; directories come first */
int
dirtree_compar_type (const dirtree_t ** d1, const dirtree_t ** d2)
{
	return ((**d2).flags & DIRTREE_DIR) - ((**d1).flags & DIRTREE_DIR);
}

/* sort entries based on their names; A comes before Z */
int
dirtree_compar_alpha (const dirtree_t ** d1, const dirtree_t ** d2)
{
	return strcmp ((**d1).name, (**d2).name);
}

/* sort entries based on their mtimes; old entries before new entries */
int
dirtree_compar_mtime (const dirtree_t ** d1, const dirtree_t ** d2)
{
	return (**d1).mtime - (**d2).mtime;
}

void
dirtree_sort (dirtree_t * tree)
{
	int           i, n;
	dirtree_t    *t;
	dirtree_t   **list;

	if (tree->child == NULL)
		return;

	for (n = 0, t = tree->child; t != NULL; t = t->next, n++);
	list = (dirtree_t **) safemalloc (n * sizeof (dirtree_t *));
	for (n = 0, t = tree->child; t != NULL; t = t->next, n++)
		list[n] = t;
	qsort (list, n, sizeof (dirtree_t *), (int (*)())dirtree_compar);
	tree->child = list[0];
	for (i = 1; i < n; i++)
		list[i - 1]->next = list[i];
	list[n - 1]->next = NULL;
	free (list);

	for (t = tree->child; t != NULL; t = t->next)
		dirtree_sort (t);
}

int
dirtree_set_id (dirtree_t * tree, int id)
{
	dirtree_t    *t;

	tree->flags = (tree->flags & ~DIRTREE_ID) | id;
	id++;
	for (t = tree->child; t != NULL; t = t->next)
		id = dirtree_set_id (t, id);
	return id;
}

#if 0										   /* no longer used */
void
dirtree_output_tree (FILE * fp, dirtree_t * tree, int recurse)
{
	extern struct config func_config[];
	dirtree_t    *t;
	char         *buf;

	/* first pass: print children */
	if (recurse)
		for (t = tree->child; t != NULL; t = t->next)
			if (t->flags & DIRTREE_DIR)
				dirtree_output_tree (fp, t, recurse);

	/* second pass: print self */
	buf = safemalloc (8192);
	if (tree->flags & DIRTREE_KEEPNAME)
		fprintf (fp, "PopUp \"%s\"\n", tree->name);
	else
		fprintf (fp, "PopUp \"%d\"\n", tree->flags & DIRTREE_ID);
	fprintf (fp, "  Title \"%s\"\n", tree->name);
	fprintf (fp, "  MiniPixmap \"%s\"\n", tree->icon != NULL ? tree->icon : "mini-menu.xpm");
	for (t = tree->child; t != NULL; t = t->next)
	{
		if (t->flags & DIRTREE_DIR)
		{
			if (t->flags & DIRTREE_KEEPNAME)
				fprintf (fp, "  PopUp \"%s\" %s\n", t->name, t->name);
			else
				fprintf (fp, "  PopUp \"%s\" %d\n", t->name, t->flags & DIRTREE_ID);

			if (t->icon != NULL)
				fprintf (fp, "  MiniPixmap \"%s\"\n", t->icon);
			else if (t->flags & DIRTREE_DIR)
				fprintf (fp, "  MiniPixmap \"mini-folder.xpm\"\n");
		} else if (t->command && t->command->keyword)
		{
			fprintf (fp, "  %s \"%s\" %s\n", t->command->keyword, t->name, t->path);
			if (t->icon != NULL)
				fprintf (fp, "  MiniPixmap \"%s\"\n", t->icon);
		} else
		{
			FILE         *fp2 = fopen (t->path, "r");

			/* try to load a command */
			if (fp2 != NULL && fgets (buf, 8192, fp2) != NULL)
			{
				struct config *config = find_config (func_config, buf);
				char         *ptr = strip_whitespace (buf);

				if (config != NULL && !isspace (buf[strlen (config->keyword)]))
					config = NULL;
				if (config == NULL && 13 + strlen (t->name) + strlen (ptr) < 8192)
				{
					memmove (ptr + 13 + strlen (t->name), ptr, strlen (ptr) + 1);
					sprintf (ptr, "Exec \"%s\" exec", t->name);
					ptr[strlen (ptr)] = ' ';
				}
				if (config == NULL || !mystrcasecmp (config->keyword, "Exec"))
				{
#ifndef NO_AVAILABILITYCHECK
					char         *tmp;

					for (tmp = ptr + 4; isspace (*tmp); tmp++);
					if (*tmp == '"')
					{
						for (tmp++; *tmp != '\0' && *tmp != '"'; tmp++);
						if (*tmp == '"')
						{
							for (tmp++; isspace (*tmp); tmp++);
							if (!is_executable_in_path (tmp))
							{
								if (config != NULL)
									memcpy (ptr, "Nop ", 4);
								else
									sprintf (ptr = buf, "Nop \"%s\"", t->name);
							}
						}
					}
#endif /* NO_AVAILABILITYCHECK */
				}
				fprintf (fp, "  %s\n", ptr);
			} else
				fprintf (fp, "  Exec \"%s\" exec %s\n", t->name, t->name);
			/* check for a MiniPixmap */
			if (fp2 != NULL && fgets (buf, 8192, fp2) != NULL)
			{
				char         *ptr = strip_whitespace (buf);

				if (!mystrncasecmp (ptr, "MiniPixmap", 10))
					fprintf (fp, "  %s\n", ptr);
			}
			if (t->icon != NULL)
				fprintf (fp, "  MiniPixmap \"%s\"\n", t->icon);
			fclose (fp2);
		}
	}
	fprintf (fp, "EndPopUp\n\n");
	free (buf);
}
#endif

/* debugging code */
#if 0
void
dirtree_print_tree (dirtree_t * tree, int depth)
{
	dirtree_t    *t;

	fprintf (stderr, "%*s%s%s(%s:%s:%d:%x)\n", depth, "", tree->name,
			 (tree->flags & DIRTREE_DIR) ? "/ " : " ", tree->command->keyword, tree->icon,
			 tree->order, tree->flags);
	for (t = tree->child; t != NULL; t = t->next)
		dirtree_print_tree (t, depth + 1);
}

void
dirtree_print_tree_from_dir (const char *dir)
{
	dirtree_t    *tree;

	tree = dirtree_new_from_dir (dir);
	dirtree_parse_include (tree);
	dirtree_remove_order (tree);
	dirtree_merge (tree);
	dirtree_sort (tree);
	dirtree_set_id (tree, 0);
	dirtree_output_tree (stderr, tree, 1);
	dirtree_delete (tree);
}

void
dirtree_main (void)
{
	dirtree_print_tree_from_dir ("/root/GNUstep/Library/AfterStep/start");
}
#endif
