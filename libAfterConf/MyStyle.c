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


#include "../configure.h"

#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/mystyle.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/screen.h"

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
    "Old style Gradient definition",
	NULL,
	0
};


/* just a nice error printing function */
void          mystyle_error (char *format, char *name, char *text2);

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the MyStyle settings
 *
 ****************************************************************************/

MyStyleDefinition *
AddMyStyleDefinition (MyStyleDefinition ** tail)
{
	MyStyleDefinition *new_def = (MyStyleDefinition *) safemalloc (sizeof (MyStyleDefinition));

	/* let's initialize Base config with some nice values: */
	new_def->name = NULL;
	new_def->inherit = NULL;
	new_def->inherit_num = 0;
	new_def->font = NULL;
	new_def->fore_color = NULL;
	new_def->back_color = NULL;
	new_def->text_style = 0;
	new_def->max_colors = 256;
	new_def->backgradient_type = 0;
	new_def->backgradient_from = NULL;
	new_def->backgradient_to = NULL;

	new_def->back_pixmap_type = 0;
	new_def->back_pixmap = NULL;
	new_def->draw_text_background = 0;
	new_def->set_flags = 0;
	new_def->finished = 0;
	new_def->next = *tail;
	*tail = new_def;

	new_def->more_stuff = NULL;

	return new_def;
}

void
DestroyMyStyleDefinitions (MyStyleDefinition ** list)
{
	if (*list)
	{
		DestroyMyStyleDefinitions (&((*list)->next));
		if ((*list)->name)
			free ((*list)->name);
		if ((*list)->inherit)
		{
			if ((*list)->inherit[0])
				free ((*list)->inherit[0]);
			free ((*list)->inherit);
		}
		if ((*list)->font)
			free ((*list)->font);
		if ((*list)->fore_color)
			free ((*list)->fore_color);
		if ((*list)->back_color)
			free ((*list)->back_color);
		if ((*list)->back_pixmap)
			free ((*list)->back_pixmap);
		if ((*list)->backgradient_from)
			free ((*list)->backgradient_from);
		if ((*list)->backgradient_to)
			free ((*list)->backgradient_to);

		DestroyFreeStorage (&((*list)->more_stuff));

		free (*list);
		*list = NULL;
	}
}

void
PrintMyStyleDefinitions (MyStyleDefinition * list)
{
#ifdef DEBUG_MYSTYLE
	if (list)
	{
		PrintMyStyleDefinitions (list->next);
		if (list->name)
			fprintf (stderr, "\nMyStyle \"%s\"", list->name);
		if (list->inherit)
			if (list->inherit[0])
				fprintf (stderr, "\n\tInherit %s", list->inherit[0]);
		fprintf (stderr, "\n\tInheritCount %d", list->inherit_num);
		if (list->font)
			fprintf (stderr, "\n\tFont %s", list->font);
		if (list->fore_color)
			fprintf (stderr, "\n\tForeColor %s", list->fore_color);
		if (list->back_color)
			fprintf (stderr, "\n\tBackColor %s", list->back_color);
		fprintf (stderr, "\n\tTextStyle %d", list->text_style);
		fprintf (stderr, "\n\tMaxColors %d", list->max_colors);
		fprintf (stderr, "\n\tBackGradient %d\t%s\t%s", list->backgradient_type, list->backgradient_from,
				 list->backgradient_to);
		if (list->back_pixmap)
			fprintf (stderr, "\n\tBackPixmap %d %s", list->back_pixmap_type, list->back_pixmap);
		else if (list->set_flags & F_BACKPIXMAP)
			fprintf (stderr, "\n\tBackPixmap %d", list->back_pixmap_type);
		fprintf (stderr, "\n\tDrawTextBackground \"%d\"", list->draw_text_background);
		if (list->finished)
			fprintf (stderr, "\n~MyStyle");
	}
#endif
}

MyStyleDefinition **
ProcessMyStyleOptions (FreeStorageElem * options, MyStyleDefinition ** tail)
{
	ConfigItem    item;

	item.memory = NULL;

	for (; options; options = options->next)
	{
		if (options->term->id < MYSTYLE_ID_START || options->term->id > MYSTYLE_ID_END)
			continue;

		if (options->term == NULL)
			continue;
		if (options->term->type == TT_FLAG)
		{
			switch (options->term->id)
			{
			 case MYSTYLE_DONE_ID:
				 if (*tail != NULL)
					 if ((*tail)->name != NULL)
					 {
						 (*tail)->finished = 1;
						 while (*tail)
							 tail = &((*tail)->next);	/* just in case */
						 break;
					 }
				 mystyle_error ("Unexpected MyStyle definition termination. Ignoring.", "Unknown", NULL);
				 DestroyMyStyleDefinitions (tail);
				 break;
			 default:
				 ReadConfigItem (&item, NULL);
				 return tail;
			}
			continue;
		}

		if (!ReadConfigItem (&item, options))
			continue;

		if (*tail == NULL)
			AddMyStyleDefinition (tail);
#define SET_SET_FLAG1(f)  SET_SET_FLAG((**tail),f)
		switch (options->term->id)
		{
		 case MYSTYLE_START_ID:
			 if ((*tail)->name != NULL)
			 {
				 mystyle_error ("Previous MyStyle definition [%s] was not terminated correctly, and will be ignored.",
								item.data.string, (*tail)->name);
				 DestroyMyStyleDefinitions (tail);
				 AddMyStyleDefinition (tail);
			 }
			 (*tail)->name = item.data.string;
			 break;
		 case MYSTYLE_INHERIT_ID:
			 AddStringToArray (&((*tail)->inherit_num), &((*tail)->inherit), item.data.string);
			 item.ok_to_free = 1;
			 break;
		 case MYSTYLE_FONT_ID:
			 REPLACE_STRING (((*tail)->font), item.data.string);
			 SET_SET_FLAG1 (F_FONT);

			 break;
		 case MYSTYLE_FORECOLOR_ID:
			 REPLACE_STRING (((*tail)->fore_color), item.data.string);
			 SET_SET_FLAG1 (F_FORECOLOR);
			 break;
		 case MYSTYLE_BACKCOLOR_ID:
			 REPLACE_STRING (((*tail)->back_color), item.data.string);
			 SET_SET_FLAG1 (F_BACKCOLOR);
			 break;
		 case MYSTYLE_TEXTSTYLE_ID:
			 (*tail)->text_style = item.data.integer;
			 SET_SET_FLAG1 (F_TEXTSTYLE);
			 break;
		 case MYSTYLE_MAXCOLORS_ID:
			 (*tail)->max_colors = item.data.integer;
			 SET_SET_FLAG1 (F_MAXCOLORS);
			 break;
		 case MYSTYLE_BACKGRADIENT_ID:
			 {
				 (*tail)->backgradient_type = item.data.integer;
				 if ((*tail)->backgradient_from)
					 free ((*tail)->backgradient_from);
				 if ((*tail)->backgradient_to)
					 free ((*tail)->backgradient_to);
				 if (options->argc > 1 && options->argv[1])
					 (*tail)->backgradient_from = mystrdup (options->argv[1]);
				 else
					 (*tail)->backgradient_from = NULL;

				 if (options->argc > 2 && options->argv[2])
					 (*tail)->backgradient_to = mystrdup (options->argv[2]);
				 else
					 (*tail)->backgradient_to = NULL;

				 SET_SET_FLAG1 (F_BACKGRADIENT);
			 }
			 break;
		 case MYSTYLE_BACKPIXMAP_ID:
			 (*tail)->back_pixmap_type = item.data.integer;
			 if ((*tail)->back_pixmap)
				 free ((*tail)->back_pixmap);
			 if (options->argc > 1 && options->argv[1])
				 (*tail)->back_pixmap = mystrdup (options->argv[1]);
			 else
				 (*tail)->back_pixmap = NULL;
			 SET_SET_FLAG1 (F_BACKPIXMAP);
			 break;
		 case MYSTYLE_DRAWTEXTBACKGROUND_ID:
			 (*tail)->draw_text_background = item.data.integer;
			 SET_SET_FLAG1 (F_DRAWTEXTBACKGROUND);
			 break;
		 default:
			 item.ok_to_free = 1;
			 ReadConfigItem (&item, NULL);
			 return tail;
		}
	}
	ReadConfigItem (&item, NULL);
	return tail;
}

void          mystyle_create_from_definition (MyStyleDefinition * def);

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
				mystyle_create_from_definition (pCurr);
			DestroyMyStyleDefinitions (list);
			mystyle_fix_styles ();
		}
}

void
mystyle_create_from_definition (MyStyleDefinition * def)
{
	int           i;
	MyStyle      *style;

	if (def == NULL)
		return;
	if (def->finished == 0 || def->name == NULL)
		return;

	if ((style = mystyle_find (def->name)) == NULL)
	{
		style = mystyle_new_with_name (def->name);
		def->name = NULL;					   /* so it wont get deallocated */
	}

	if (def->inherit)
		for (i = 0; i < def->inherit_num; ++i)
		{
			MyStyle      *parent;

			if (def->inherit[i])
			{
				if ((parent = mystyle_find (def->inherit[i])) != NULL)
					mystyle_merge_styles (parent, style, True, False);
				else
					mystyle_error ("unknown style to inherit: %s\n", style->name, def->inherit[i]);
			}
		}
	if (def->font)
	{
		style->inherit_flags &= ~F_FONT;
		if (style->user_flags & F_FONT)
			unload_font (&style->font);

		if (load_font (def->font, &style->font) == True)
			style->user_flags |= F_FONT;
		else
			style->user_flags &= ~F_FONT;
	}

	if (def->fore_color)
	{
		if (parse_argb_color (def->fore_color, &(style->colors.fore)) != def->fore_color)
		{
			style->inherit_flags &= ~F_FORECOLOR;
			style->user_flags |= F_FORECOLOR;
		}
	}
	if (def->back_color)
	{
		if (parse_argb_color (def->back_color, &(style->colors.back)) != def->back_color)
		{
			style->inherit_flags &= ~F_BACKCOLOR;
			style->texture_type = 0;
			style->relief.fore = GetHilite (style->colors.back);
			style->relief.back = GetShadow (style->colors.back);
			style->user_flags |= F_BACKCOLOR;
		}
	}
	if (def->set_flags & F_TEXTSTYLE)
	{
		style->text_style = def->text_style;
		style->inherit_flags &= F_TEXTSTYLE;
		style->user_flags |= ~F_TEXTSTYLE;
	}
#ifndef NO_TEXTURE
	if (def->set_flags & F_MAXCOLORS)
	{
		style->max_colors = def->max_colors;
		style->user_flags |= F_MAXCOLORS;
		style->inherit_flags &= ~F_MAXCOLORS;
	}
	if (def->set_flags & F_BACKGRADIENT)
	{
		style->inherit_flags &= ~F_BACKGRADIENT;
		if (def->backgradient_from && def->backgradient_to)
		{
			int           type = def->backgradient_type;
			ARGB32        c1, c2;
			ASGradient    gradient;

			parse_argb_color (def->backgradient_from, &c1);
			parse_argb_color (def->backgradient_to, &c2);
			if ((type = mystyle_parse_old_gradient (type, c1, c2, &gradient)) >= 0)
			{
				if (style->user_flags & F_BACKGRADIENT)
				{
					free (style->gradient.color);
					free (style->gradient.offset);
				}
				gradient.type = mystyle_translate_grad_type (type);
				style->gradient = gradient;
				style->texture_type = type;
				style->user_flags |= F_BACKGRADIENT;
			} else
				mystyle_error ("bad gradient.\n", style->name, NULL);
		} else
			mystyle_error ("incomplete gradient definition.\n", style->name, NULL);

	}
	if ((def->set_flags & F_BACKPIXMAP))
	{
		int           type = def->back_pixmap_type;
		char         *tmp = def->back_pixmap;

		clear_flags (style->inherit_flags, F_BACKTRANSPIXMAP | F_BACKPIXMAP);
		free_icon_resources (style->back_icon);
		memset (&(style->back_icon), 0x00, sizeof (style->back_icon));

		if (type < TEXTURE_TEXTURED_START || type >= TEXTURE_TEXTURED_END)
		{
			show_error
				("Error in MyStyle \"%s\": unsupported texture type [%d] in BackPixmap setting. Assuming default of [128] instead.",
				 style->name, type);
			type = TEXTURE_PIXMAP;
		}
		if (type == TEXTURE_TRANSPARENT || type == TEXTURE_TRANSPARENT_TWOWAY)
		{									   /* treat second parameter as ARGB tint value : */
			if (parse_argb_color (tmp, &(style->tint)) == tmp)
				style->tint = TINT_LEAVE_SAME; /* use no tinting by default */
			else if (type == TEXTURE_TRANSPARENT)
				style->tint = (style->tint >> 1) & 0x7F7F7F7F;	/* converting old style tint */
			/*LOCAL_DEBUG_OUT( "tint is 0x%X (from %s)",  style->tint, tmp); */
			set_flags (style->user_flags, F_BACKPIXMAP);
			style->texture_type = type;
		} else
		{									   /* treat second parameter as an image filename : */
			if (load_icon (&(style->back_icon), tmp, Scr.image_manager))
			{
				set_flags (style->user_flags, F_BACKPIXMAP);
				if (type >= TEXTURE_TRANSPIXMAP)
					set_flags (style->user_flags, F_BACKTRANSPIXMAP);
				style->texture_type = type;
			} else
				mystyle_error (style->name, "failed to load image file \"%s\".", tmp);
		}
		LOCAL_DEBUG_OUT ("MyStyle \"%s\": BackPixmap %d image = %p, tint = 0x%X", style->name,
						 style->texture_type, style->back_icon.image, style->tint);

#if 0
		char         *path;
		Pixmap        pix, mask;

		if ((path = findIconFile (def->back_pixmap, PixmapPath, R_OK)) != NULL &&
			(pix = LoadImageWithMask (dpy, RootWindow (dpy, DefaultScreen (dpy)), colors, path, &mask)) != None)
		{
			Window        r;
			int           d;

			XGetGeometry (dpy, pix, &r, &d, &d, &style->back_icon.width, &style->back_icon.height, &d, &d);
			if (style->user_flags & F_BACKPIXMAP)
			{
				free_icon_resources (style->back_icon);
			}
			style->back_icon.pix = pix;
			style->back_icon.mask = mask;
			style->texture_type = def->back_pixmap_type;
			style->user_flags |= F_BACKPIXMAP;
		} else
			mystyle_error ("unable to load pixmap: '%s'\n", style->name, def->back_pixmap);

		if (path != NULL)
			free (path);
	}
	style->inherit_flags &= ~F_BACKPIXMAP;
#endif
    }
#endif
    if (def->set_flags & F_DRAWTEXTBACKGROUND)
    {
        style->user_flags |= F_DRAWTEXTBACKGROUND;
        style->inherit_flags &= ~F_DRAWTEXTBACKGROUND;
        if (def->draw_text_background == 0)
            style->flags &= ~F_DRAWTEXTBACKGROUND;
    }

    style->set_flags = style->inherit_flags | style->user_flags;
}

MyStyleDefinition *
GetMyStyleDefinition (MyStyleDefinition ** list, const char *name, Bool add_new)
{
	register MyStyleDefinition *style = NULL;

	if (name)
	{
		for (style = *list; style != NULL; style = style->next)
			if (mystrcasecmp (style->name, name) == 0)
				break;
		if (style == NULL && add_new)
		{									   /* add new style here */
			style = AddMyStyleDefinition (list);
			style->name = mystrdup (name);
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
		{
			if (style->font)
				free (style->font);
			style->font = mystrdup (new_font);
            SET_SET_FLAG (*style, F_FONT);
		}
		if (new_fcolor)
		{
			if (style->fore_color)
				free (style->fore_color);
			style->fore_color = mystrdup (new_fcolor);
			SET_SET_FLAG (*style, F_FORECOLOR);
		}
		if (new_bcolor)
		{
			if (style->back_color)
				free (style->back_color);
			style->back_color = mystrdup (new_bcolor);
			SET_SET_FLAG (*style, F_BACKCOLOR);
		}
		if (new_style >= 0)
		{
			style->text_style = new_style;
			SET_SET_FLAG (*style, F_TEXTSTYLE);
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

    if( !get_flags (style->set_flags, F_BACKPIXMAP) )
    {
#if 0
        TextureDef *t = add_texture_def( &(style->back), &(style->back_layers));
        type = pixmap_type2texture_type( type );
        if ( TEXTURE_IS_ICON(type) )
        {
            if (pixmap != NULL )
            {
                set_string_value (&(t->data.icon.image), pixmap, &(style->set_flags), F_BACKTEXTURE);
                t->type = type;
            }
        } else if ( TEXTURE_IS_GRADIENT(type) )
        {
            if (color_from != NULL && color_to != NULL)
            {
                make_old_gradient (t, type, color_from, color_to);
            }
        } else if( TEXTURE_IS_TRANSPARENT(type) && pixmap )
        {
            set_string_value (&(t->data.icon.tint), pixmap, &(style->set_flags), F_BACKTEXTURE);
            style->back[0].type = type;
        }
#endif
    }
}

FreeStorageElem **
MyStyleDef2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, MyStyleDefinition * def)
{
	register int  i;
	FreeStorageElem **new_tail;

	if (def == NULL)
		return tail;

	new_tail = QuotedString2FreeStorage (syntax, tail, NULL, def->name, MYSTYLE_START_ID);
	if (new_tail == tail)
		return tail;

	tail = &((*tail)->sub);

	for (i = 0; i < def->inherit_num; i++)
		tail = QuotedString2FreeStorage (&MyStyleSyntax, tail, NULL, def->inherit[i], MYSTYLE_INHERIT_ID);

	if (get_flags (def->set_flags, F_FONT))
		tail = String2FreeStorage (&MyStyleSyntax, tail, def->font, MYSTYLE_FONT_ID);

	if (get_flags (def->set_flags, F_FORECOLOR))
		tail = String2FreeStorage (&MyStyleSyntax, tail, def->fore_color, MYSTYLE_FORECOLOR_ID);

	if (get_flags (def->set_flags, F_BACKCOLOR))
		tail = String2FreeStorage (&MyStyleSyntax, tail, def->back_color, MYSTYLE_BACKCOLOR_ID);

	if (get_flags (def->set_flags, F_TEXTSTYLE))
        tail = Integer2FreeStorage (&MyStyleSyntax, tail, NULL, def->text_style, MYSTYLE_TEXTSTYLE_ID);

    if (get_flags (def->set_flags, F_BACKPIXMAP))
#if 0
		for( i = 0 ; i < def->back_layers ; i++ )
        	tail = TextureDef2FreeStorage (&MyStyleSyntax, tail, &(def->back[i]), MYSTYLE_BACKPIXMAP_ID);
#endif
	if (get_flags (def->set_flags, F_DRAWTEXTBACKGROUND))
       	tail = Integer2FreeStorage (&MyStyleSyntax, tail, NULL, def->draw_text_background, MYSTYLE_DRAWTEXTBACKGROUND_ID);

	tail = Flag2FreeStorage (&MyStyleSyntax, tail, MYSTYLE_DONE_ID);

	return new_tail;
}

FreeStorageElem **
MyStyleDefs2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, MyStyleDefinition * defs)
{
	while (defs)
	{
		tail = MyStyleDef2FreeStorage (syntax, tail, defs);
		defs = defs->next;
	}
	return tail;
}


