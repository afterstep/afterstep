/*
 * Copyright (c) 2003 Sasha Vasko <sasha at aftercode.net>
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
 */

#define LOCAL_DEBUG

#include "../configure.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/colorscheme.h"
#include "../libAfterStep/session.h"
#include "../libAfterStep/screen.h"
#include "../libAfterStep/kde.h"
#include "../libAfterBase/xml.h"

#include <unistd.h>

#include "afterconf.h"


/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the colors config
 * file
 *
 ****************************************************************************/
#define COLOR_TERM(n,len)  {TF_NO_MYNAME_PREPENDING, #n, len, TT_COLOR, COLOR_##n##_ID , NULL}

TermDef ColorTerms[] = {
	COLOR_TERM (Base, 4),
	COLOR_TERM (Inactive1, 9),
	COLOR_TERM (Inactive2, 9),
	COLOR_TERM (Active, 6),
	COLOR_TERM (InactiveText1, 13),
	COLOR_TERM (InactiveText2, 13),
	COLOR_TERM (ActiveText, 10),
	COLOR_TERM (HighInactive, 12),
	COLOR_TERM (HighActive, 10),
	COLOR_TERM (HighInactiveBack, 16),
	COLOR_TERM (HighActiveBack, 14),
	COLOR_TERM (HighInactiveText, 16),
	COLOR_TERM (HighActiveText, 14),
	COLOR_TERM (DisabledText, 12),
	COLOR_TERM (BaseDark, 8),
	COLOR_TERM (BaseLight, 9),
	COLOR_TERM (Inactive1Dark, 13),
	COLOR_TERM (Inactive1Light, 14),
	COLOR_TERM (Inactive2Dark, 13),
	COLOR_TERM (Inactive2Light, 14),
	COLOR_TERM (ActiveDark, 10),
	COLOR_TERM (ActiveLight, 11),
	COLOR_TERM (HighInactiveDark, 16),
	COLOR_TERM (HighInactiveLight, 17),
	COLOR_TERM (HighActiveDark, 14),
	COLOR_TERM (HighActiveLight, 15),
	COLOR_TERM (HighInactiveBackDark, 20),
	COLOR_TERM (HighInactiveBackLight, 21),
	COLOR_TERM (HighActiveBackDark, 24),
	COLOR_TERM (HighActiveBackLight, 25),
	COLOR_TERM (Cursor, 7),
	{TF_NO_MYNAME_PREPENDING, "Angle", 5, TT_UINTEGER, COLOR_Angle_ID, NULL}
	,

	{0, NULL, 0, 0, 0}
};


SyntaxDef ColorSyntax = {
	'\n',
	'\0',
	ColorTerms,
	0,														/* use default hash size */
	' ',
	"",
	"\t",
	"Custom Color Scheme",
	"ColorScheme",
	"defines color values for standard set of internal color names, to be used in other configuration files",
	NULL,
	0
};

ColorConfig *CreateColorConfig ()
{
	return safecalloc (1, sizeof (ColorConfig));
}

void DestroyColorConfig (ColorConfig * config)
{
	if (config) {
		DestroyFreeStorage (&(config->more_stuff));
		free (config);
	}
}


void PrintColorConfig (ColorConfig * config)
{
}

ColorConfig *ParseColorOptions (const char *filename, char *myname)
{
	ColorConfig *config = CreateColorConfig ();
	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem item;

	Storage =
			file2free_storage (filename, myname, &ColorSyntax, NULL,
												 &(config->more_stuff));
	if (Storage == NULL)
		return config;

	item.memory = NULL;

	LOCAL_DEBUG_OUT ("Storage = %p", Storage);

	for (pCurr = Storage; pCurr; pCurr = pCurr->next) {
		int index;

		LOCAL_DEBUG_OUT ("pCurr = %p", pCurr);
		if (pCurr->term == NULL)
			continue;
		if (!ReadConfigItem (&item, pCurr))
			continue;
		index = pCurr->term->id - COLOR_ID_START;
		LOCAL_DEBUG_OUT ("id %d, index = %d", pCurr->term->id, index);
		if (index >= 0 && index < ASMC_MainColors) {
			LOCAL_DEBUG_OUT ("index %d is \"%s\"", index, item.data.string);
			if (parse_argb_color
					(item.data.string,
					 &(config->main_colors[index])) != item.data.string) {
				LOCAL_DEBUG_OUT ("Parsed color %d as #%8.8lX", index,
												 (unsigned long)config->main_colors[index]);
				set_flags (config->set_main_colors, (0x01 << index));
			}
		} else if (pCurr->term->id == COLOR_Angle_ID) {
			config->angle = item.data.integer;
			set_flags (config->set_main_colors, COLOR_Angle);
		}
		item.ok_to_free = 1;
	}
	ReadConfigItem (&item, NULL);

	DestroyFreeStorage (&Storage);
	return config;
}

/* returns:
 *		0 on success
 *		1 if data is empty
 *		2 if ConfigWriter cannot be initialized
 *
 */
int
WriteColorOptions (const char *filename, char *myname,
									 ColorConfig * config, unsigned long flags)
{
	ConfigDef *ColorConfigWriter = NULL;
	FreeStorageElem *Storage = NULL, **tail = &Storage;
	ConfigData cd;

	char color_buffer[128];
	int i;

	if (config == NULL)
		return 1;
	cd.filename = filename;
	if ((ColorConfigWriter =
			 InitConfigWriter (myname, &ColorSyntax, CDT_Filename, cd)) == NULL)
		return 2;

	CopyFreeStorage (&Storage, config->more_stuff);

	/* building free storage here */
	LOCAL_DEBUG_OUT ("0x%lX", config->set_main_colors);
	for (i = 0; i < ASMC_MainColors; ++i) {
		ARGB32 c = config->main_colors[i];
		CARD32 a, r, g, b, h, s, v;
		char *tmp[1];
		FreeStorageElem **pelem;

		a = ARGB32_ALPHA8 (c);
		r = ARGB32_RED16 (c);
		g = ARGB32_GREEN16 (c);
		b = ARGB32_BLUE16 (c);
		h = rgb2hsv (r, g, b, &s, &v);
		h = hue162degrees (h);
		s = val162percent (s);
		v = val162percent (v);
		r = r >> 8;
		g = g >> 8;
		b = b >> 8;
		tmp[0] = &(color_buffer[0]);
		sprintf (color_buffer,
						 "#%2.2lX%2.2lX%2.2lX%2.2lX  \t\t# or ahsv(%ld,%ld,%ld,%ld) or argb(%ld,%ld,%ld,%ld)",
						 (unsigned long)a, (unsigned long)r, (unsigned long)g,
						 (unsigned long)b, (long)a, (long)h, (long)s, (long)v, (long)a,
						 (long)r, (long)g, (long)b);
		pelem = tail;
		LOCAL_DEBUG_OUT ("tail  = %p", tail);
		tail =
				Strings2FreeStorage (&ColorSyntax, tail, &(tmp[0]), 1,
														 COLOR_ID_START + i);

		LOCAL_DEBUG_OUT ("i = %d, tail  = %p, *pelem = %p", i, tail, pelem);
		if (*pelem && !get_flags (config->set_main_colors, 0x01 << i)
				&& i != ASMC_Base)
			set_flags ((*pelem)->flags, CF_DISABLED_OPTION);
	}

	/* colorscheme angle */
	if (get_flags (config->set_main_colors, COLOR_Angle))
		tail =
				Integer2FreeStorage (&ColorSyntax, tail, NULL, config->angle,
														 COLOR_Angle_ID);

	/* writing config into the file */
	cd.filename = filename;
	WriteConfig (ColorConfigWriter, Storage, CDT_Filename, &cd, flags);
	DestroyFreeStorage (&Storage);
	DestroyConfig (ColorConfigWriter);

	if (Storage) {
		fprintf (stderr,
						 "\n%s:Config Writing warning: Not all Free Storage discarded! Trying again...",
						 myname);
		DestroyFreeStorage (&Storage);
		fprintf (stderr, (Storage != NULL) ? " failed." : " success.");
	}
	return 0;
}

ColorConfig *ASColorScheme2ColorConfig (ASColorScheme * cs)
{
	ColorConfig *config = NULL;

	if (cs) {
		int i;

		config = CreateColorConfig ();
		config->angle = cs->angle;
		for (i = 0; i < ASMC_MainColors; ++i)
			config->main_colors[i] = cs->main_colors[i];
		config->set_main_colors = cs->set_main_colors;
	}
	return config;
}

ASColorScheme *ColorConfig2ASColorScheme (ColorConfig * config)
{
	ASColorScheme *cs = NULL;

	if (config) {
		int i;
		int angle = ASCS_DEFAULT_ANGLE;

		if (get_flags (config->set_main_colors, COLOR_Angle))
			angle = config->angle;
		cs = make_ascolor_scheme (config->main_colors[ASMC_Base], angle);
		for (i = 0; i < ASMC_MainColors; ++i)
			if (i != ASMC_Base
					&& get_flags (config->set_main_colors, (0x01 << i)))
				cs->main_colors[i] = config->main_colors[i];

		for (i = 0; i < ASMC_MainColors; ++i)
			make_color_scheme_hsv (cs->main_colors[i], &(cs->main_hues[i]),
														 &(cs->main_saturations[i]),
														 &(cs->main_values[i]));

		cs->set_main_colors = config->set_main_colors;
	}
	return cs;
}

void LoadColorScheme ()
{
	ASColorScheme *cs = NULL;
	const char *const_configfile;

	/* first we need to load the colorscheme */
	if ((const_configfile =
			 get_session_file (Session, get_screen_current_desk (NULL),
												 F_CHANGE_COLORSCHEME, False)) != NULL) {
		ColorConfig *config = ParseColorOptions (const_configfile, MyName);

		if (config) {
			cs = ColorConfig2ASColorScheme (config);
			DestroyColorConfig (config);
			show_progress ("COLORSCHEME loaded from \"%s\" ...",
										 const_configfile);
		} else
			show_progress
					("COLORSCHEME file format is unrecognizeable in \"%s\" ...",
					 const_configfile);

	} else
		show_warning ("COLORSCHEME is not set");

	if (cs == NULL)
		cs = make_default_ascolor_scheme ();

	populate_ascs_colors_rgb (cs);
	populate_ascs_colors_xml (cs);
	free (cs);
}

Bool
translate_gtkrc_template_file (const char *template_fname,
															 const char *output_fname)
{
	static char buffer[MAXLINELENGTH];
	FILE *src_fp = NULL, *dst_fp = NULL;
	int good_strings = 0;

	if (template_fname == NULL || output_fname == NULL)
		return False;;

	src_fp = fopen (template_fname, "r");
	dst_fp = fopen (output_fname, "w");
	if (dst_fp == NULL)
		show_warning ("Failed to open file \"%s\" for writing", output_fname);
	if (src_fp != NULL && dst_fp != NULL) {
		while (fgets (&buffer[0], MAXLINELENGTH, src_fp)) {
			int i = 0;

			while (isspace (buffer[i]))
				++i;
			if (buffer[i] != '\n' && buffer[i] != '#' && buffer[i] != '\0'
					&& buffer[i] != '\r') {
				++good_strings;
				if (strncmp (&buffer[i], "fg[", 3) == 0 ||
						strncmp (&buffer[i], "bg[", 3) == 0 ||
						strncmp (&buffer[i], "text[", 5) == 0 ||
						strncmp (&buffer[i], "base[", 5) == 0 ||
						strncmp (&buffer[i], "light[", 6) == 0 ||
						strncmp (&buffer[i], "dark[", 5) == 0
						|| strncmp (&buffer[i], "mid[", 4) == 0) {
					while (buffer[i] != '\0' && buffer[i] != '\"'
								 && buffer[i] != '\{')
						++i;
					if (buffer[i] == '\"') {
						char *token = &buffer[i + 1];

						if (isalpha (token[0])) {
							int len = 0;

							while (token[len] != '\0' && token[len] != '\"')
								++len;
							if (token[len] == '\"' && len > 0) {
								ARGB32 argb;

								if (parse_argb_color (token, &argb) != token) {
									fwrite (&(buffer[0]), 1, i + 1, dst_fp);
									fprintf (dst_fp, "#%2.2lX%2.2lX%2.2lX",
													 (unsigned long)ARGB32_RED8 (argb),
													 (unsigned long)ARGB32_GREEN8 (argb),
													 (unsigned long)ARGB32_BLUE8 (argb));
									fwrite (&(token[len]), 1, strlen (&(token[len])),
													dst_fp);
									continue;
								}
							}
						}
					}
				}
			}
			fwrite (&buffer[0], 1, strlen (&buffer[0]), dst_fp);
		}

	}
	if (src_fp)
		fclose (src_fp);
	if (dst_fp)
		fclose (dst_fp);
	return (good_strings > 0);
}

Bool
translate_kcsrc_template_file (const char *template_fname,
															 const char *output_fname)
{
/* this prolly needs to be rewritten using libAfterBase/kde.c code	: */
	int good_strings = 0;
	xml_elem_t *KDE_cs, *group;

	if (template_fname == NULL || output_fname == NULL)
		return False;;

	KDE_cs = load_KDE_config (template_fname);
	for (group = KDE_cs->child; group != NULL; group = group->next) {
		xml_elem_t *item;

		for (item = group->child; item != NULL; item = item->next)
			if (item->tag_id == KDEConfig_item && IsTagCDATA (item->child)) {
				char *parm = item->child->parm;

				++good_strings;
				if (*parm == '\"') {
					ARGB32 argb;

					++parm;
					if (parse_argb_color (parm, &argb) != parm) {
						char *tmp = safemalloc (32);

						sprintf (tmp, "%ld,%ld,%ld", (unsigned long)ARGB32_RED8 (argb),
										 (unsigned long)ARGB32_GREEN8 (argb),
										 (unsigned long)ARGB32_BLUE8 (argb));
						free (item->child->parm);
						item->child->parm = tmp;
					}
				}
			}
	}
	save_KDE_config (output_fname, KDE_cs);
	xml_elem_delete (NULL, KDE_cs);

	return (good_strings > 0);
}


Bool UpdateGtkRC (ASEnvironment * e)
{
	Bool result = False;
	char *src;

	/* first we need to load the colorscheme */
	if (e == NULL)
		return False;
	if (e->gtkrc_path != NULL && strcmp (e->gtkrc_path, "/dev/null") != 0) {
		src = make_session_file (Session, GTKRC_TEMPLATE_FILE, False);
		if (src)
			result = translate_gtkrc_template_file (src, e->gtkrc_path);

		destroy_string (&src);
	}

	if (e->gtkrc20_path != NULL
			&& strcmp (e->gtkrc20_path, "/dev/null") != 0) {
		src = make_session_file (Session, GTKRC20_TEMPLATE_FILE, False);
		/* first we need to load the colorscheme */
		if (src)
			if (translate_gtkrc_template_file (src, e->gtkrc20_path))
				result = True;
		destroy_string (&src);
	}
	return result;
}

static Bool
SetKDEGlobalsColorScheme (const char *new_cs_file, const char *cs_name)
{
	char *kdeglobals_fname = copy_replace_envvar (KDEGLOBALS_FILE);
	xml_elem_t *KDE_cs = load_KDE_config (new_cs_file);
	xml_elem_t *KDE_globals = load_KDE_config (kdeglobals_fname);
	xml_elem_t *KDE_group = get_KDE_config_group (KDE_globals, "KDE", True);
	xml_elem_t *CS_group =
			get_KDE_config_group (KDE_cs, "Color Scheme", False);

	if (CS_group) {
		xml_elem_t *WM_group = get_KDE_config_group (KDE_globals, "WM", True);
		xml_elem_t *General_group =
				get_KDE_config_group (KDE_globals, "General", True);

		merge_KDE_config_groups (CS_group, WM_group);
		merge_KDE_config_groups (CS_group, General_group);
	}

	set_KDE_config_group_item (KDE_group, "colorScheme",
														 cs_name ? cs_name : new_cs_file);

	save_KDE_config (kdeglobals_fname, KDE_globals);
	xml_elem_delete (NULL, KDE_globals);
	xml_elem_delete (NULL, KDE_cs);

	free (kdeglobals_fname);

	add_KDE_colorscheme (new_cs_file);
	return (CS_group != NULL);
}

Bool UpdateKCSRC ()
{
	Bool result = False;
	char *src = make_session_file (Session, KSCRC_TEMPLATE_FILE, False);
	char *dst = make_session_rc_file (Session, KCSRC_FILE);

	/* first we need to load the colorscheme */
	if (src && dst)
		result = translate_kcsrc_template_file (src, dst);

	if (result) {
		char *cs_filename = dst;
		char *kcs_path = copy_replace_envvar (KDECS_DIR);

		if (CheckDir (kcs_path) == 0) {
			char *fullcs_filename;

			cs_filename = mystrdup (AS_KCSRC_FILE);
			fullcs_filename = make_file_name (kcs_path, cs_filename);
			CopyFile (dst, fullcs_filename);
			free (fullcs_filename);
		}
		free (kcs_path);

		if (!SetKDEGlobalsColorScheme (dst, cs_filename))
			result = False;
		if (cs_filename != dst)
			free (cs_filename);
	}
	destroy_string (&src);
	destroy_string (&dst);

	return result;
}
