/*
 * Copyright (c) 2002 Sasha Vasko <sasha at aftercode dot net>
 * Copyright (c) 1999 Ethan Fischer <allanon@crystaltokyo.com>
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

#define LOCAL_DEBUG

#include "../configure.h"
#include "asapp.h"
#include "screen.h"
#include "mystyle.h"
#include "mystyle_property.h"
#include "wmprops.h"

void
mystyle_list_set_property (ASWMProps * wmprops, ASHashTable * list)
{
	CARD32       *prop;
	int           i, nelements;
	ASHashIterator iterator;

	nelements = 0;
	if (!start_hash_iteration (list, &iterator))
		return;
	do
	{
#define MYSTYLE_STATIC_ELEM_NUM (9 + 4 + 2 + 7)
		nelements += MYSTYLE_STATIC_ELEM_NUM + ((MyStyle *) curr_hash_data (&iterator))->gradient.npoints * 4;
	}
	while (next_hash_item (&iterator));

	prop = safemalloc (sizeof (CARD32) * nelements);

	i = 0;
	start_hash_iteration (list, &iterator);
	do
	{
		MyStyle      *style = (MyStyle *) curr_hash_data (&iterator);

/*    show_warning( "style \"%s\" set_flags = %X", style->name, style->set_flags );
 */
		prop[i++] = style->set_flags;
		prop[i++] = XInternAtom (dpy, style->name, False);
		prop[i++] = style->text_style;
		prop[i++] = XInternAtom (dpy, style->font.name, False);
		prop[i++] = style->colors.fore;
		prop[i++] = style->colors.back;
		prop[i++] = style->relief.fore;
		prop[i++] = style->relief.back;
		prop[i++] = style->texture_type;
		prop[i++] = style->slice_x_start;
		prop[i++] = style->slice_x_end;
		prop[i++] = style->slice_y_start;
		prop[i++] = style->slice_y_end;
		prop[i++] = style->blur_x;
		prop[i++] = style->blur_y;
		if (style->back_icon.pix == None)
			make_icon_pixmaps (&(style->back_icon), False);
		prop[i++] = style->back_icon.pix;
		prop[i++] = style->back_icon.mask;
		prop[i++] = style->tint;
		prop[i++] = style->back_icon.alpha;
		prop[i++] = style->overlay_type;
		prop[i++] = (style->overlay) ? XInternAtom (dpy, style->overlay->name, False) : None;
		prop[i++] = style->gradient.npoints;
		{
			int           k;

			for (k = 0; k < style->gradient.npoints; k++)
			{
				prop[i++] = style->gradient.color[k];
				prop[i++] = style->gradient.color[k];	/* for compatibility with older version */
				prop[i++] = style->gradient.color[k];	/* for compatibility with older version */
				LOCAL_DEBUG_OUT ("gradient color at offset %f is %lX", style->gradient.offset[k],
								 style->gradient.color[k]);
				prop[i++] = style->gradient.offset[k] * 0x1000000;
			}
		}
	}
	while (next_hash_item (&iterator));
	/* set the property version to 1.2 */
	set_as_style (wmprops, nelements * sizeof (CARD32), (1 << 8) + 5, prop);
	free (prop);
}

void
mystyle_set_property (ASWMProps * wmprops)
{
	mystyle_list_set_property (wmprops, ASDefaultScr->Look.styles_list);
}


void
mystyle_get_property (ASWMProps * wmprops)
{
	CARD32       *prop;
	size_t        i, n;
	CARD32        version;

	if ((prop = wmprops->as_styles_data) == NULL)
		return;
	/* do we know how to handle this version? */
	version = wmprops->as_styles_version;
	/* do we know how to handle this version? */
	if (version != (1 << 8) + 5)
	{
		show_error ("style property has unknown version %d.%d", (int)version >> 8, (int)version & 0xff);
		return;
	}

	n = wmprops->as_styles_size / sizeof (CARD32);
	for (i = 0; i < n;)
	{
		MyStyle      *style;
		char         *name;
		CARD32        flags = prop[i + 0];
		Pixmap        back_pix, back_mask, back_alpha;

		name = XGetAtomName (dpy, prop[i + 1]);
		if ((style = mystyle_find (name)) == NULL)
			style = mystyle_new_with_name (name);
		XFree (name);

		/* free up any resources that are already in use */
		if (style->user_flags & F_FONT)
			unload_font (&style->font);
		if (style->user_flags & F_BACKGRADIENT)
		{
			free (style->gradient.color);
			free (style->gradient.offset);
		}
		if (style->back_icon.image)
		{
			safe_asimage_destroy (style->back_icon.image);
			style->back_icon.image = NULL;
		}

		style->user_flags = 0;
		style->inherit_flags = flags;
		style->text_style = prop[i + 2];

		if (style->inherit_flags & F_FONT)
		{
#ifdef MODULE_REUSE_LOADED_FONT
			XFontStruct  *font = XQueryFont (dpy, prop[i + 3]);

			style->font.name = NULL;
			style->font.font = font;
			if (font != NULL)
			{
				style->font.width = font->max_bounds.rbearing + font->min_bounds.lbearing;
				style->font.height = font->ascent + font->descent;
				style->font.y = font->ascent;
			}
#else
			name = XGetAtomName (dpy, prop[i + 3]);
#if 1
			set_string (&(style->font.name), mystrdup (name));
#else
			load_font (name, &(style->font));
#endif
			XFree (name);
			style->user_flags |= F_FONT;
			style->inherit_flags &= ~F_FONT;
#endif
		}
		style->colors.fore = prop[i + 4];
		style->colors.back = prop[i + 5];
		LOCAL_DEBUG_OUT ("style(%p:\"%s\")->colors(#%lX,#%lX)->flags(0x%lX)->inherit_flags(0x%lX)", style, style->name,
						 style->colors.fore, style->colors.back, flags, style->inherit_flags);
		style->relief.fore = prop[i + 6];
		style->relief.back = prop[i + 7];
		style->texture_type = prop[i + 8];
		style->slice_x_start = prop[i + 9];
		style->slice_x_end = prop[i + 10];
		style->slice_y_start = prop[i + 11];
		style->slice_y_end = prop[i + 12];
		style->blur_x = prop[i + 13];
		style->blur_y = prop[i + 14];
		LOCAL_DEBUG_OUT ("style(%p:\"%s\")->slicing( x: %d to %d, y: %d to %d )", style, style->name,
						 style->slice_x_start, style->slice_x_end, style->slice_y_start, style->slice_y_end);
		back_pix = prop[i + 15];
		back_mask = prop[i + 16];
		style->tint = prop[i + 17];
		back_alpha = prop[i + 18];
		set_flags (style->user_flags, F_EXTERNAL_BACKPIX | F_EXTERNAL_BACKMASK);
		LOCAL_DEBUG_OUT ("overlay props = %lu, 0x%lX", prop[i + 19], prop[i + 20]);
		style->overlay_type = prop[i + 19];
		if (prop[i + 20] != None)
		{
			name = XGetAtomName (dpy, prop[i + 20]);
			style->overlay = mystyle_find (name);
			XFree (name);
		}


		style->gradient.npoints = prop[i + 21];
/*	  show_warning( "checking if gradient data in style \"%s\": (set flags are : 0x%X)", style->name, style->inherit_flags);
 */
		if (style->gradient.npoints > 0)
		{
			size_t        k;

			style->gradient.type = mystyle_translate_grad_type (style->texture_type);
			style->gradient.color = NEW_ARRAY (ARGB32, style->gradient.npoints);
			style->gradient.offset = NEW_ARRAY (double, style->gradient.npoints);

			for (k = 0; k < style->gradient.npoints; k++)
			{
				style->gradient.color[k] = prop[i + MYSTYLE_STATIC_ELEM_NUM + k * 4 + 0];
				/*
				   style->gradient.color[k].green = prop[i + 16 + k * 4 + 1];
				   style->gradient.color[k].blue = prop[i + 16 + k * 4 + 2];
				 */
				style->gradient.offset[k] = (double)prop[i + MYSTYLE_STATIC_ELEM_NUM + k * 4 + 3] / 0x1000000;
				LOCAL_DEBUG_OUT ("gradient color at offset %f is %lX", style->gradient.offset[k],
								 style->gradient.color[k]);
			}
			style->user_flags |= F_BACKGRADIENT;
			style->inherit_flags &= ~F_BACKGRADIENT;
		}
		/* if there's a backpixmap, make sure it's valid and get its geometry */
		if (back_pix != None)
		{
			icon_from_pixmaps (&(style->back_icon), back_pix, back_mask, back_alpha);
			if (style->back_icon.image != NULL)
				clear_flags (style->inherit_flags, F_BACKPIXMAP | F_BACKTRANSPIXMAP);
		}

		if (style->back_icon.pix == None &&
			style->texture_type != TEXTURE_TRANSPARENT && style->texture_type != TEXTURE_TRANSPARENT_TWOWAY)
		{
			if (style->inherit_flags & F_BACKPIXMAP)
			{
				style->texture_type = 0;
				style->back_icon.mask = None;
				style->inherit_flags &= ~(F_BACKPIXMAP | F_BACKTRANSPIXMAP);
			} else if (style->inherit_flags & F_BACKTRANSPIXMAP)
			{
				fprintf (stderr, "Transpixmap requested, but no pixmap detected for style '%s'\n", style->name);
				fprintf (stderr, "This is a bug, please report it!\n");
				style->inherit_flags &= ~F_BACKTRANSPIXMAP;
			}
		}

		i += MYSTYLE_STATIC_ELEM_NUM + style->gradient.npoints * 4;
		LOCAL_DEBUG_OUT ("user_flags = 0x%lX, inherit_flags = 0x%lX", style->user_flags, style->inherit_flags);
		style->set_flags = style->user_flags | style->inherit_flags;
	}

	/* force update of global gcs */
	mystyle_fix_styles ();
}

void
mystyle_handle_property_event (XEvent * event)
{
	handle_wmprop_event (ASDefaultScr->wmprops, event);
	mystyle_list_destroy_all (&(ASDefaultScr->Look.styles_list));
	mystyle_get_property (ASDefaultScr->wmprops);
}
