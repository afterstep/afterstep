/*
 * Copyright (c) 1999 Sasha Vasko <sasha at aftercode.net>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#define LOCAL_DEBUG
#include "../configure.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/mystyle.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/screen.h"
#include "../libAfterStep/session.h"

#include "afterconf.h"

#define DEBUG_MYSTYLE

TermDef       MyStyleTerms[] = {
/* including MyStyles definitions here so we can process them correctly */
	MYSTYLE_TERMS,
	{0, NULL, 0, 0, 0}
};

SyntaxDef     MyStyleSyntax = {
	'\n',
	'\0',
	MyStyleTerms,
	0,										   /* use default hash size */
	'\t',
	"",
	"",
	"Look MyStyle definition",
	"MyStyle",
	"defines combination of color, font, style, background to be used together",
	NULL,
	0
};

flag_options_xref MyStyleFlags[] = {
	ASCF_DEFINE_MODULE_FLAG_XREF (MYSTYLE, DrawTextBackground, MyStyleDefinition),
	{0, 0, 0}
};

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the MyStyle settings
 *
 ****************************************************************************/

MyStyleDefinition *
AddMyStyleDefinition (MyStyleDefinition ** tail)
{
	MyStyleDefinition *new_def = safecalloc (1, sizeof (MyStyleDefinition));

	new_def->next = *tail;
	*tail = new_def;

	return new_def;
}

void
free_MSD_back_grad (MyStyleDefinition * msd)
{
	if (msd->back_grad_npoints > 0)
	{
		if (msd->back_grad_colors)
		{
			register int  i = msd->back_grad_npoints;

			while (--i >= 0)
				if (msd->back_grad_colors[i])
					free (msd->back_grad_colors[i]);
			free (msd->back_grad_colors);
		}
		if (msd->back_grad_offsets)
			free (msd->back_grad_offsets);
	}
}

void
DestroyMyStyleDefinitions (MyStyleDefinition ** list)
{
	if (*list)
	{
		MyStyleDefinition *pnext = *list;

		while (pnext != NULL)
		{
			MyStyleDefinition *pdef = pnext;

			pnext = pdef->next;
			destroy_string (&(pdef->Name));
			destroy_string (&(pdef->Comment));
			if (pdef->inherit)
			{
				int           i = pdef->inherit_num;

				while (--i >= 0)
					destroy_string (&(pdef->inherit[i]));
				free (pdef->inherit);
			}
			destroy_string (&(pdef->Font));
			destroy_string (&(pdef->ForeColor));
			destroy_string (&(pdef->BackColor));
			destroy_string (&(pdef->back_pixmap));
			destroy_string (&(pdef->overlay));
			free_MSD_back_grad (pdef);
			DestroyFreeStorage (&(pdef->more_stuff));
			free (pdef);
		}
		*list = NULL;
	}
}

void
PrintMyStyleDefinitions (MyStyleDefinition * list)
{
#ifdef DEBUG_MYSTYLE
	while (list)
	{
		if (list->Name)
			fprintf (stderr, "\nMyStyle \"%s\"", list->Name);
		if (list->Comment)
			fprintf (stderr, "\n\tComment \"%s\"", list->Comment);
		if (list->inherit)
		{
			int           i;

			for (i = 0; i < list->inherit_num; ++i)
				if (list->inherit[i])
					fprintf (stderr, "\n\tInherit \"%s\"", list->inherit[i]);
		}
		fprintf (stderr, "\n\tInheritCount %d", list->inherit_num);
		if (list->Font)
			fprintf (stderr, "\n\tFont %s", list->Font);
		if (list->ForeColor)
			fprintf (stderr, "\n\tForeColor %s", list->ForeColor);
		if (list->BackColor)
			fprintf (stderr, "\n\tBackColor %s", list->BackColor);

		fprintf (stderr, "\n\tTextStyle %d", list->TextStyle);
		fprintf (stderr, "\n\tSliceXStart %d", list->SliceXStart);
		fprintf (stderr, "\n\tSliceXEnd %d", list->SliceXEnd);
		fprintf (stderr, "\n\tSliceYStart %d", list->SliceYStart);
		fprintf (stderr, "\n\tSliceYEnd %d", list->SliceYEnd);
		fprintf (stderr, "\n\tBlurSize %dx%d", list->BlurSize.width, list->BlurSize.height);
		if (get_flags (list->set_flags, MYSTYLE_DrawTextBackground))
		{
			fprintf (stderr, "\n\tDrawTextBackground %d", get_flags (list->flags, MYSTYLE_DrawTextBackground) ? 1 : 0);
		}
		if (list->overlay && list->overlay_type > TEXTURE_GRADIENT_END)
			fprintf (stderr, "\n\tOverlay %d \"%s\"", list->overlay_type, list->overlay);

		if (list->texture_type > 0)
		{
			if (list->texture_type > TEXTURE_GRADIENT_END)
			{
				if (list->back_pixmap)
					fprintf (stderr, "\n\tBackPixmap %d %s", list->texture_type, list->back_pixmap);
				else
					fprintf (stderr, "\n\tBackPixmap %d", list->texture_type);
			} else
			{
				register int  i = 0;

				fprintf (stderr, "\n\tBackMultiGradient %d", list->back_grad_type);
				while (i < list->back_grad_npoints)
				{
					fprintf (stderr, "\t%s %f", list->back_grad_colors[i], list->back_grad_offsets[i]);
					++i;
				}
			}
		}
		fprintf (stderr, "\n#set_flags = 0x%lX", (unsigned long)list->set_flags);
		fprintf (stderr, "\n#flags = 0x%lX", (unsigned long)list->flags);
		fprintf (stderr, "\n~MyStyle\n");
		list = list->next;
	}
#endif
}

void
HandleMyStyleSpecialTerm (MyStyleDefinition * msd, FreeStorageElem * fs)
{
	ConfigItem    item;

	item.memory = NULL;

	if (!ReadConfigItem (&item, fs))
		return;

	switch (fs->term->id)
	{
	 case MYSTYLE_Inherit_ID:
		 {
			 int           pos = msd->inherit_num++;

			 msd->inherit = realloc (msd->inherit, sizeof (char *) * msd->inherit_num);
			 msd->inherit[pos] = item.data.string;
		 }
		 break;
	 case MYSTYLE_BackGradient_ID:
		 if (fs->argc != 3)
			 show_error ("invalid number of colors in BackGradient definition (%d)", fs->argc);
		 else
		 {
			 free_MSD_back_grad (msd);

			 msd->back_grad_type = item.data.integer;
			 msd->texture_type = mystyle_parse_old_gradient (msd->back_grad_type, 0, 0, NULL);
			 msd->back_grad_npoints = 2;
			 msd->back_grad_colors = safemalloc (2 * sizeof (char *));
			 msd->back_grad_offsets = safemalloc (2 * sizeof (double));
			 msd->back_grad_colors[0] = mystrdup (fs->argv[1]);
			 msd->back_grad_colors[1] = mystrdup (fs->argv[2]);
			 msd->back_grad_offsets[1] = 0.0;
			 msd->back_grad_offsets[1] = 1.0;
		 }
		 break;
	 case MYSTYLE_BackMultiGradient_ID:
		 if (fs->argc < 4)
			 show_error ("invalid number of colors in BackMultiGradient definition (%d)", fs->argc);
		 else
		 {
			 int           i;

			 free_MSD_back_grad (msd);

			 msd->back_grad_type = item.data.integer;
			 msd->texture_type = mystyle_parse_old_gradient (msd->back_grad_type, 0, 0, NULL);
			 msd->back_grad_npoints = ((fs->argc - 1) + 1) / 2;
			 msd->back_grad_colors = safecalloc (msd->back_grad_npoints, sizeof (char *));
			 msd->back_grad_offsets = safecalloc (msd->back_grad_npoints, sizeof (double));
			 for (i = 0; i < msd->back_grad_npoints; ++i)
			 {
				 msd->back_grad_colors[i] = mystrdup (fs->argv[i * 2 + 1]);
				 if (i * 2 + 2 < fs->argc)
					 msd->back_grad_offsets[i] = atof (fs->argv[i * 2 + 2]);
			 }
			 msd->back_grad_offsets[msd->back_grad_npoints - 1] = 1.0;
		 }
		 break;
	 case MYSTYLE_BackPixmap_ID:
		 {
			 set_string (&(msd->back_pixmap), item.data.string);
			 msd->texture_type = item.index;
		 }
		 break;
	 case MYSTYLE_Overlay_ID:
		 {
			 if (fs->argc > 1)
			 {
				 set_string (&(msd->overlay), stripcpy2 (fs->argv[1], False));
				 msd->overlay_type = item.data.integer;
			 } else
				 show_error
					 ("Error in MyStyle \"%s\": Overlay option uses format :  \"Overlay <type> <mystyle name>\". Ignoring.",
					  msd->Name);
		 }
		 break;
	 default:
		 item.ok_to_free = 1;
	}
	ReadConfigItem (&item, NULL);
}

Bool
CheckMyStyleDefinitionSanity (MyStyleDefinition * msd)
{
	if (msd == NULL)
		return False;

	if (msd->Name == NULL)
	{
		show_error ("MyStyle without name is encountered and will be ignored.");
		return False;
	}

	if (msd->texture_type < TEXTURE_SOLID || msd->texture_type > TEXTURE_TEXTURED_END)
	{
		show_error
			("Error in MyStyle \"%s\": unsupported texture type [%d] in BackPixmap setting. Assuming default of [128] instead.",
			 msd->Name, msd->texture_type);
		msd->texture_type = msd->back_pixmap ? TEXTURE_PIXMAP : TEXTURE_TRANSPARENT;
	}

	if (msd->overlay_type != 0)
		if (msd->overlay_type < TEXTURE_TEXTURED_START || msd->texture_type > TEXTURE_TEXTURED_END)
		{
			show_error ("Error in MyStyle \"%s\": unsupported overlay type [%d]. Assuming default of [131] instead.",
						msd->Name, msd->overlay_type);
			msd->overlay_type = TEXTURE_TRANSPIXMAP_ALPHA;
		}


	return True;
}

MyStyle      *
mystyle_find_or_get_from_file (struct ASHashTable * list, const char *name)
{
	MyStyle      *ms;

	if ((ms = mystyle_list_find (list, name)) == NULL)
	{
		char         *fn = make_session_data_file (Session, False, S_IFREG, MYSTYLES_DIR, name, NULL);

		if (fn == NULL)
			fn = make_session_data_file (Session, False, S_IFREG, MYSTYLES_DIR, "mystyle.", name, NULL);
		if (fn == NULL)
			fn = make_session_data_file (Session, True, S_IFREG, MYSTYLES_DIR, name, NULL);
		if (fn == NULL)
			fn = make_session_data_file (Session, True, S_IFREG, MYSTYLES_DIR, "mystyle.", name, NULL);
		if (fn != NULL)
		{
			FreeStorageElem *Storage = NULL;
			MyStyleDefinition *msd = NULL;

			Storage = file2free_storage (fn, MyName, &MyStyleSyntax, NULL, NULL);
			if (Storage)
			{
				msd = free_storage_elem2MyStyleDefinition (Storage, name);
				DestroyFreeStorage (&Storage);
				if (msd != NULL)
				{
					ms = mystyle_create_from_definition (list, msd);
					DestroyMyStyleDefinitions (&msd);
				}
			}
			free (fn);
		}
	}
	return ms;
}

MyStyle      *
mystyle_create_from_definition (struct ASHashTable * list, MyStyleDefinition * def)
{
	int           i;
	MyStyle      *style;

	if (def == NULL || def->Name == NULL)
		return NULL;

	if ((style = mystyle_find (def->Name)) == NULL)
		style = mystyle_new_with_name (def->Name);

	if (def->inherit)
	{
		for (i = 0; i < def->inherit_num; ++i)
		{
			MyStyle      *parent;

			if (def->inherit[i])
			{
				if ((parent = mystyle_find_or_get_from_file (list, def->inherit[i])) != NULL)
					mystyle_merge_styles (parent, style, True, False);
				else
					show_error ("unknown style to inherit: %s\n", def->inherit[i]);
			}
		}
	}
	if (def->Font)
	{
		if (get_flags (style->user_flags, F_FONT))
		{
			unload_font (&style->font);
			clear_flags (style->user_flags, F_FONT);
		}
		set_string (&(style->font.name), mystrdup (def->Font));
		set_flags (style->user_flags, F_FONT);
	}
	if (get_flags (def->set_flags, MYSTYLE_TextStyle))
	{
		set_flags (style->user_flags, F_TEXTSTYLE);
		style->text_style = def->TextStyle;
	}
	if (get_flags (def->set_flags, MYSTYLE_SLICE_SET))
	{
		set_flags (style->user_flags, F_SLICE);
		style->slice_x_start = def->SliceXStart;
		style->slice_x_end = def->SliceXEnd;
		style->slice_y_start = def->SliceYStart;
		style->slice_y_end = def->SliceYEnd;
	}
	if (get_flags (def->set_flags, MYSTYLE_BlurSize))
	{
		set_flags (style->user_flags, F_BLUR);
		style->blur_x = def->BlurSize.width;
		style->blur_y = def->BlurSize.height;
	}
	if (def->ForeColor)
	{
		if (parse_argb_color (def->ForeColor, &(style->colors.fore)) != def->ForeColor)
			set_flags (style->user_flags, F_FORECOLOR);
		else
			show_error ("unable to parse Forecolor \"%s\"", def->ForeColor);
	}
	if (def->BackColor)
	{
		if (parse_argb_color (def->BackColor, &(style->colors.back)) != def->BackColor)
		{
			style->relief.fore = GetHilite (style->colors.back);
			style->relief.back = GetShadow (style->colors.back);
			set_flags (style->user_flags, F_BACKCOLOR);
		} else
			show_error ("unable to parse BackColor \"%s\"", def->BackColor);
	}
	if (def->overlay != NULL)
	{
		MyStyle      *o = mystyle_find_or_get_from_file (list, def->overlay);

		if (o != NULL)
		{
			style->overlay = o;
			style->overlay_type = def->overlay_type;
		} else
			show_error ("unknown style to be overlayed with: %s\n", def->overlay);
		set_flags (style->user_flags, F_OVERLAY);
	}

	if (def->texture_type > 0 && def->texture_type <= TEXTURE_GRADIENT_END)
	{
		int           type = def->back_grad_type;
		ASGradient    gradient = { 0 };
		Bool          success = False;

		if (type <= TEXTURE_OLD_GRADIENT_END)
		{
			ARGB32        color1 = ARGB32_White, color2 = ARGB32_Black;

			parse_argb_color (def->back_grad_colors[0], &color1);
			parse_argb_color (def->back_grad_colors[1], &color2);
			if ((type = mystyle_parse_old_gradient (type, color1, color2, &gradient)) >= 0)
			{
				if (style->user_flags & F_BACKGRADIENT)
				{
					free (style->gradient.color);
					free (style->gradient.offset);
				}
				style->gradient = gradient;
				style->gradient.type = mystyle_translate_grad_type (type);
				LOCAL_DEBUG_OUT ("style %p type = %d", style, style->gradient.type);
				success = True;
			} else
				show_error ("Error in MyStyle \"%s\": invalid gradient type %d", style->name, type);
		} else
		{
			int           i;

			gradient.npoints = def->back_grad_npoints;
			gradient.color = safecalloc (gradient.npoints, sizeof (ARGB32));
			gradient.offset = safecalloc (gradient.npoints, sizeof (double));
			for (i = 0; i < gradient.npoints; ++i)
			{
				ARGB32        color = ARGB32_White;

				parse_argb_color (def->back_grad_colors[i], &color);
				gradient.color[i] = color;
				gradient.offset[i] = def->back_grad_offsets[i];
			}
			gradient.offset[0] = 0.0;
			gradient.offset[gradient.npoints - 1] = 1.0;
			if (style->user_flags & F_BACKGRADIENT)
			{
				free (style->gradient.color);
				free (style->gradient.offset);
			}
			style->gradient = gradient;
			style->gradient.type = mystyle_translate_grad_type (type);
			success = True;
		}
		if (success)
		{
			style->texture_type = def->texture_type;
			set_flags (style->user_flags, F_BACKGRADIENT);
		} else
		{
			if (gradient.color != NULL)
				free (gradient.color);
			if (gradient.offset != NULL)
				free (gradient.offset);
			show_error ("Error in MyStyle \"%s\": bad gradient", style->name);
		}
	} else if (def->texture_type > TEXTURE_GRADIENT_END && def->texture_type <= TEXTURE_TEXTURED_END)
	{
		int           type = def->texture_type;

		if (get_flags (style->user_flags, F_BACKPIXMAP))
		{
			mystyle_free_back_icon (style);
			clear_flags (style->user_flags, F_BACKTRANSPIXMAP | F_BACKPIXMAP);
		}
		clear_flags (style->inherit_flags, F_BACKTRANSPIXMAP | F_BACKPIXMAP);
		LOCAL_DEBUG_OUT ("calling mystyle_free_back_icon for style %p", style);

		if (type == TEXTURE_TRANSPARENT || type == TEXTURE_TRANSPARENT_TWOWAY)
		{									   /* treat second parameter as ARGB tint value : */
			if (parse_argb_color (def->back_pixmap, &(style->tint)) == def->back_pixmap)
				style->tint = TINT_LEAVE_SAME; /* use no tinting by default */
			else if (type == TEXTURE_TRANSPARENT)
				style->tint = (style->tint >> 1) & 0x7F7F7F7F;	/* converting old style tint */
/*LOCAL_DEBUG_OUT( "tint is 0x%X (from %s)",  style->tint, tmp);*/
			set_flags (style->user_flags, F_BACKPIXMAP);
			style->texture_type = type;
		} else
		{									   /* treat second parameter as an image filename : */
			if (load_icon (&(style->back_icon), def->back_pixmap, get_screen_image_manager (NULL)))
			{
				set_flags (style->user_flags, F_BACKPIXMAP);
				if (type >= TEXTURE_TRANSPIXMAP)
					set_flags (style->user_flags, F_BACKTRANSPIXMAP);
				style->texture_type = type;
			} else
				show_error ("Parsing MyStyle \"%s\" failed to load image file \"%s\".", style->name, def->back_pixmap);
		}
		LOCAL_DEBUG_OUT ("MyStyle \"%s\": BackPixmap %d image = %p, tint = 0x%lX", style->name,
						 style->texture_type, style->back_icon.image, style->tint);
	}
	if (get_flags (def->set_flags, MYSTYLE_DrawTextBackground))
	{
		if (get_flags (def->flags, MYSTYLE_DrawTextBackground))
			set_flags (style->user_flags, F_DRAWTEXTBACKGROUND);
		else
		{
			clear_flags (style->user_flags, F_DRAWTEXTBACKGROUND);
			clear_flags (style->inherit_flags, F_DRAWTEXTBACKGROUND);
		}
	}

	clear_flags (style->inherit_flags, style->user_flags);
	style->set_flags = style->inherit_flags | style->user_flags;

	return style;
}


/*
 * this function process a linked list of MyStyle definitions
 * and create MyStyle for each valid definition
 * this operation is destructive in the way that all
 * data members of definitions that are used in MyStyle will be
 * set to NULL, so to prevent them being deallocated by destroy function,
 * and/or being used in other places
 * ATTENTION !!!!!
 * MyStyleDefinitions become unusable as the result, and get's destroyed
 * pointer to a list becomes NULL !
 */
void
ProcessMyStyleDefinitions (MyStyleDefinition ** list)
{
	MyStyleDefinition *pCurr;

	if (list)
		if (*list)
		{
			for (pCurr = *list; pCurr; pCurr = pCurr->next)
				mystyle_create_from_definition (NULL, pCurr);
			DestroyMyStyleDefinitions (list);
			mystyle_fix_styles ();
		}
}

void
mystyle_parse (char *tline, FILE * fd, char **myname, int *mystyle_list)
{
	FreeStorageElem *Storage = NULL;
	MyStyleDefinition **list = (MyStyleDefinition **) mystyle_list, **tail;

	if (list == NULL)
		return;
	for (tail = list; *tail != NULL; tail = &((*tail)->next));

	Storage = tline_subsyntax_parse ("MyStyle", tline, fd, (char *)myname, &MyStyleSyntax, NULL, NULL);
	if (Storage)
	{
		*tail = free_storage_elem2MyStyleDefinition (Storage, NULL);
		DestroyFreeStorage (&Storage);
	}
}


MyStyleDefinition *
GetMyStyleDefinition (MyStyleDefinition ** list, const char *name, Bool add_new)
{
	register MyStyleDefinition *style = NULL;

	if (name)
	{
		for (style = *list; style != NULL; style = style->next)
			if (mystrcasecmp (style->Name, name) == 0)
				break;
		if (style == NULL && add_new)
		{									   /* add new style here */
			style = AddMyStyleDefinition (list);
			style->Name = mystrdup (name);
		}
	}
	return style;
}


void
MergeMyStyleText (MyStyleDefinition ** list, const char *name,
				  const char *new_font, const char *new_fcolor, const char *new_bcolor, int new_style)
{
	register MyStyleDefinition *style = NULL;

	if ((style = GetMyStyleDefinition (list, name, True)) != NULL)
	{
		if (new_font)
			set_string (&(style->Font), mystrdup (new_font));
		if (new_fcolor)
			set_string (&(style->ForeColor), mystrdup (new_fcolor));
		if (new_bcolor)
			set_string (&(style->BackColor), mystrdup (new_bcolor));
		if (new_style >= 0)
		{
			style->TextStyle = new_style;
			set_flags (style->set_flags, MYSTYLE_TextStyle);
		}
	}
}

void
MergeMyStyleTextureOld (MyStyleDefinition ** list, const char *name,
						int type, char *color_from, char *color_to, char *pixmap)
{
	register MyStyleDefinition *style;

	if ((style = GetMyStyleDefinition (list, name, True)) == NULL)
		return;

#if 0
	if (!get_flags (style->set_flags, F_BACKPIXMAP))
	{
		TextureDef   *t = add_texture_def (&(style->back), &(style->back_layers));

		type = pixmap_type2texture_type (type);
		if (TEXTURE_IS_ICON (type))
		{
			if (pixmap != NULL)
			{
				set_string_value (&(t->data.icon.image), pixmap, &(style->set_flags), F_BACKTEXTURE);
				t->type = type;
			}
		} else if (TEXTURE_IS_GRADIENT (type))
		{
			if (color_from != NULL && color_to != NULL)
			{
				make_old_gradient (t, type, color_from, color_to);
			}
		} else if (TEXTURE_IS_TRANSPARENT (type) && pixmap)
		{
			set_string_value (&(t->data.icon.tint), pixmap, &(style->set_flags), F_BACKTEXTURE);
			style->back[0].type = type;
		}
	}
#endif
}

FreeStorageElem *
MyStyleSpecialTerms2FreeStorage (MyStyleDefinition * msd, SyntaxDef * syntax)
{
	int           i;
	FreeStorageElem *fs = NULL;
	FreeStorageElem **tail = &fs;

	for (i = 0; i < msd->inherit_num; i++)
		tail = QuotedString2FreeStorage (&MyStyleSyntax, tail, NULL, msd->inherit[i], MYSTYLE_Inherit_ID);

	tail = Flag2FreeStorage (&MyStyleSyntax, tail, MYSTYLE_DONE_ID);

#if 0

	if (get_flags (def->set_flags, F_BACKPIXMAP))

		for (i = 0; i < def->back_layers; i++)
			tail = TextureDef2FreeStorage (&MyStyleSyntax, tail, &(def->back[i]), MYSTYLE_BACKPIXMAP_ID);
	if (get_flags (def->set_flags, F_DRAWTEXTBACKGROUND))
		tail =
			Integer2FreeStorage (&MyStyleSyntax, tail, NULL, def->draw_text_background, MYSTYLE_DRAWTEXTBACKGROUND_ID);

#endif

	return fs;
}
