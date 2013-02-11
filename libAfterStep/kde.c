/****************************************************************************
 *
 * Copyright (c) 2005 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/

#define LOCAL_DEBUG
#include "../configure.h"

#include "asapp.h"
#include "../libAfterBase/xml.h"
#include "afterstep.h"
#include "wmprops.h"
#include "desktop_category.h"
#include "kde.h"

/* KDE IPC implementation : */

void
KIPC_sendMessage (KIPC_Message msg, Window w, int data)
{
	XEvent        ev;

	ev.xclient.type = ClientMessage;
	ev.xclient.display = dpy;
	ev.xclient.window = w;
	ev.xclient.message_type = _KIPC_COMM_ATOM;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = msg;
	ev.xclient.data.l[1] = data;
	XSendEvent (dpy, w, False, 0L, &ev);
}

#define KDE_CONFIG_MAKE_TAG2(var,kind) 	do{  	(var)=xml_elem_new(); \
												(var)->tag = mystrdup(#kind); \
												(var)->tag_id = KDEConfig_##kind ; }while(0)

#define KDE_CONFIG_MAKE_TAG(kind) KDE_CONFIG_MAKE_TAG2(kind,kind)

static xml_elem_t *
make_kde_config_group_tag (const char *name)
{
	xml_elem_t   *group = NULL;
	int           name_len = 0;

	if (name)
		while (name[name_len] != ']' && name[name_len] != '\0' && name[name_len] != '\n')
			++name_len;
	KDE_CONFIG_MAKE_TAG (group);
	if (name_len > 0)
	{
		group->parm = safemalloc (4 + 1 + 1 + name_len + 1 + 1);
		sprintf (group->parm, "name=\"%*.*s\"", name_len, name_len, name);
	}										   /* WE ALLOW SECTIONS WITH NO NAMES */
	return group;
}

static xml_elem_t *
make_kde_config_comment_tag ()
{
	xml_elem_t   *comment = NULL;

	KDE_CONFIG_MAKE_TAG (comment);
	return comment;
}

static xml_elem_t *
make_kde_config_item_tag (const char *name, int *name_len_return)
{
	xml_elem_t   *item = NULL;

	if (name)
	{
		int           name_len = 0;

		while (name[name_len] != '=' && name[name_len] != '\0' && name[name_len] != '\n')
			++name_len;
		if (name_len > 0)
		{
			KDE_CONFIG_MAKE_TAG (item);
			item->parm = safemalloc (4 + 1 + 1 + name_len + 1 + 1);
			sprintf (item->parm, "name=\"%*.*s\"", name_len, name_len, name);
		}
		if (name_len_return)
			*name_len_return = name_len;
	}
	return item;
}


xml_elem_t   *
load_KDE_config (const char *realfilename)
{
	xml_elem_t   *config = create_CONTAINER_tag ();
	FILE         *fp = fopen (realfilename, "r");

	if (fp != NULL)
	{
		char          buffer[8192];
		xml_elem_t   *group = NULL;

		while (fgets (&buffer[0], sizeof (buffer), fp) != NULL)
		{
			xml_elem_t   *tag;
			int           i = 0;

			while (isspace (buffer[i]))
				++i;
			if (buffer[i] == '#')
			{
				++i;
				if ((tag = make_kde_config_comment_tag ()) != NULL)
				{
					int           len = strlen (&buffer[i]);

					while (len > 0 && isspace (buffer[i + len - 1]))
						--len;
					if (len > 0)
					{
						tag->child = create_CDATA_tag ();
						tag->child->parm = mystrndup (&buffer[i], len);
					}
					if (group == NULL)
					{
						group = make_kde_config_group_tag (NULL);
						config->child = group;
					}
					xml_insert (group, tag);
				}
			} else if (buffer[i] == '[')
			{
				++i;
				if ((tag = make_kde_config_group_tag (&buffer[i])) != NULL)
				{
					if (group)
						group->next = tag;
					else
						config->child = tag;
					group = tag;
				}
			} else if (buffer[i] != '\0')
			{
				int           name_len;

				tag = make_kde_config_item_tag (&buffer[i], &name_len);
				if (tag)
				{							   /* now we need to parse value and then possibly a comment : */
					char         *val = stripcpy (&buffer[i + name_len + 1]);

					if (group == NULL)
					{
						group = make_kde_config_group_tag (NULL);
						config->child = group;
					}

					xml_insert (group, tag);

					if (val)
					{
						if (val[0] != '\0')
						{
							tag->child = create_CDATA_tag ();
							tag->child->parm = val;
						} else
							free (val);
					}
				}
			}
		}

		fclose (fp);
	}
	return config;
}

static char  *
strip_name_value (char *parm)
{
	if (parm)
	{
		int           parm_len = strlen (parm);

		if (parm_len > 7 && strncmp (parm, "name=\"", 6) == 0)
		{
			return mystrndup (parm + 6, parm_len - 7);
		}
	}
	return NULL;
}

Bool
save_KDE_config (const char *realfilename, xml_elem_t * elem)
{
	FILE         *fp;
	int           sect_count = 0;

	if (elem == NULL)
		return False;
	if (elem->tag_id == XML_CONTAINER_ID)
		if ((elem = elem->child) == NULL)
			return False;

	if ((fp = fopen (realfilename, "w")) == NULL)
		return False;

	do
	{
		if (elem->tag_id == KDEConfig_group)
		{
			xml_elem_t   *child = elem->child;
			char         *name;

			++sect_count;
			name = strip_name_value (elem->parm);
			if (name != NULL)
			{
				if (sect_count > 1)
					fputc ('\n', fp);
				fprintf (fp, "[%s]\n", name);
				free (name);
			}

			while (child)
			{
				if (child->tag_id == KDEConfig_comment)
				{
					if (IsTagCDATA (child->child))
						fprintf (fp, "#%s\n", child->child->parm);
					else
						fprintf (fp, "#\n");
				} else if (child->tag_id == KDEConfig_item)
				{
					name = strip_name_value (child->parm);
					if (name != NULL)
					{
						fprintf (fp, "%s=", name);
						if (IsTagCDATA (child->child))
							fprintf (fp, "%s\n", child->child->parm);
						else
							fprintf (fp, "\n");
						free (name);
					}
				}
				child = child->next;
			}
		}
	}
	while ((elem = elem->next) != NULL);
	fclose (fp);
	return True;
}

static xml_elem_t *
find_KDE_config_item_by_key (xml_elem_t * group, const char *key, Bool create_if_missing)
{
	xml_elem_t   *item = group->child;

	while (item)
	{
		if (item->tag_id == KDEConfig_item && item->parm)
			if (strcmp (item->parm, key) == 0)
				break;
		item = item->next;
	}

	if (item != NULL && item->tag_id != KDEConfig_item)
		item = NULL;
	if (item == NULL && create_if_missing)
	{
		KDE_CONFIG_MAKE_TAG (item);
		item->parm = mystrdup (key);
		xml_insert (group, item);
	}

	return item;
}

static void
set_KDE_config_item_value (xml_elem_t * item, const char *value)
{
	while (item->child != NULL)
		xml_elem_delete (&(item->child), item->child);
	if (value)
	{
		item->child = create_CDATA_tag ();
		item->child->parm = mystrdup (value);
	}
}

void
merge_KDE_config_groups (xml_elem_t * from, xml_elem_t * to)
{

	if (from != NULL && to != NULL && from != to)
	{
		xml_elem_t   *src_child = from->child;

		while (src_child)
		{
			if (src_child->tag_id == KDEConfig_item && src_child->parm)
			{
				xml_elem_t   *dst_child;
				char         *val = NULL;

				dst_child = find_KDE_config_item_by_key (to, src_child->parm, True);
				if (src_child->child && src_child->child->tag_id == XML_CDATA_ID)
					val = src_child->child->parm;
				set_KDE_config_item_value (dst_child, val);
			}
			src_child = src_child->next;
		}
	}
}

static char  *
make_xml_name_key (const char *name)
{
	char         *key = NULL;

	if (name)
	{
		key = safemalloc (4 + 1 + 1 + strlen (name) + 1 + 1);
		sprintf (key, "name=\"%s\"", name);
	}
	return key;
}

xml_elem_t   *
get_KDE_config_group (xml_elem_t * config, const char *name, Bool create_if_missing)
{
	char         *key;
	xml_elem_t   *group = config;

	if (config == NULL)
		return False;

	if (config->tag_id == XML_CONTAINER_ID)
		if ((group = config->child) == NULL)
			return False;

	key = make_xml_name_key (name);

	do
	{
		if (group->tag_id == KDEConfig_group)
		{
			if (key == NULL && group->parm == NULL)
				break;
			if (key != NULL && group->parm != NULL)
				if (strcmp (key, group->parm) == 0)
					break;
		}
	}
	while ((group = group->next) != NULL);

	if (group != NULL && group->tag_id != KDEConfig_group)
		group = NULL;

	if (group == NULL && create_if_missing)
	{
		KDE_CONFIG_MAKE_TAG (group);
		group->parm = key;
		key = NULL;
		if (config->tag_id == XML_CONTAINER_ID)
			xml_insert (config, group);
		else
			group->next = config;
	}
	if (key)
		free (key);
	return group;
}

void
set_KDE_config_group_item (xml_elem_t * group, const char *item_name, const char *value)
{
	if (group != NULL && group->tag_id == KDEConfig_group)
	{
		char         *key = make_xml_name_key (item_name);

		if (key)
		{
			xml_elem_t   *item = find_KDE_config_item_by_key (group, key, True);

			set_KDE_config_item_value (item, value);
			free (key);
		}
	}
}

Bool
add_KDE_colorscheme (const char *new_cs_file)
{
	Bool          success = False;

#define KDE_CSRC_PATH  		"$KDEHOME/share/apps/kdisplay/color-schemes/"

	if (new_cs_file)
	{
		int           i = 1;
		char         *dst_path;
		char         *dst_full_fname;
		char         *fname = NULL;

		parse_file_name (new_cs_file, NULL, &fname);
		dst_path = copy_replace_envvar (KDE_CSRC_PATH);

		while (dst_path[i] != '\0')
		{
			if (dst_path[i] == '/')
			{
				char          t = dst_path[i];

				dst_path[i] = '\0';
				if (CheckDir (dst_path) != 0)
					mkdir (dst_path, 0755);
				dst_path[i] = t;
			}
			++i;
		}

		dst_full_fname = safemalloc (i + strlen (fname) + 1);
		sprintf (dst_full_fname, "%s%s", dst_path, fname);
		success = (CopyFile (new_cs_file, dst_full_fname) == 0);

		free (dst_full_fname);
		free (dst_path);
		free (fname);
	}

	return success;
}
