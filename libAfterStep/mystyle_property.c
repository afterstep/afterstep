/*
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

#include "../configure.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "../include/aftersteplib.h"
#include "../include/mystyle.h"
#include "../include/mystyle_property.h"
#include "../include/asproperty.h"

#ifdef I18N
#define MAX_FONTSET_NAME_LENGTH  256
#endif

void
mystyle_set_property (Display * dpy, Window w, Atom name, Atom type)
{
  MyStyle *style;
  unsigned long *prop;
  int i, nelements;

  nelements = 0;
  for (style = mystyle_first; style != NULL; style = style->next)
    {
      nelements += 9;
#ifndef NO_TEXTURE
      nelements += 7 + style->gradient.npoints * 4;
#endif /* NO_TEXTURE */
    }

  prop = safemalloc (sizeof (unsigned long) * nelements);

  i = 0;
  for (style = mystyle_first; style != NULL; style = style->next)
    {
      prop[i++] = style->set_flags;
      prop[i++] = XInternAtom (dpy, style->name, False);
      prop[i++] = style->text_style;
#ifndef I18N
      prop[i++] = style->font.font->fid;
#else
      prop[i++] = XInternAtom (dpy, style->font.name, False);
#endif
      prop[i++] = style->colors.fore;
      prop[i++] = style->colors.back;
      prop[i++] = style->relief.fore;
      prop[i++] = style->relief.back;
      prop[i++] = style->texture_type;
#ifndef NO_TEXTURE
      prop[i++] = style->max_colors;
      prop[i++] = style->back_icon.pix;
      prop[i++] = style->back_icon.mask;
      prop[i++] = style->tint.red;
      prop[i++] = style->tint.green;
      prop[i++] = style->tint.blue;
      prop[i++] = style->gradient.npoints;
      {
	int k;
	for (k = 0; k < style->gradient.npoints; k++)
	  {
	    prop[i++] = style->gradient.color[k].red;
	    prop[i++] = style->gradient.color[k].green;
	    prop[i++] = style->gradient.color[k].blue;
	    prop[i++] = style->gradient.offset[k] * 0x1000000;
	  }
      }
#endif /* NO_TEXTURE */
    }
  /* set the property version to 1.1 */
  set_as_property (dpy, w, name, (unsigned long *) prop, nelements * sizeof (unsigned long), (1 << 8) + 1);
  free (prop);
}

void
mystyle_get_property (Display * dpy, Window w, Atom name, Atom type)
{
  unsigned long *prop;
  size_t i, n;
  unsigned long version;

  if ((prop = get_as_property (dpy, w, name, &n, &version)) == NULL)
    return;
  /* do we know how to handle this version? */
  if (version != (1 << 8) + 1)
    {
      fprintf (stderr, "%s: style property has unknown version %d.%d\n", MyName, (int) version >> 8, (int) version & 0xff);
      return;
    }

  n /= sizeof (unsigned long);
  for (i = 0; i < n;)
    {
      MyStyle *style;
      char *name;
      unsigned long flags = prop[i + 0];

      name = XGetAtomName (dpy, prop[i + 1]);
      if ((style = mystyle_find (name)) == NULL)
	style = mystyle_new_with_name (name);
      XFree (name);

      /* free up any resources that are already in use */
      if (style->user_flags & F_FONT)
	unload_font (&style->font);
#ifndef NO_TEXTURE
      if (style->user_flags & F_BACKGRADIENT)
	{
	  free (style->gradient.color);
	  free (style->gradient.offset);
	}
      if (style->user_flags & F_BACKPIXMAP)
	{
	  if (style->back_icon.pix != None)
	    XFreePixmap (dpy, style->back_icon.pix);
	  if (style->back_icon.mask != None)
	    XFreePixmap (dpy, style->back_icon.mask);
	}
      if (style->user_flags & F_BACKTRANSPIXMAP)
	XDestroyImage (style->back_icon.image);
#endif /* NO_TEXTURE */

      style->user_flags = 0;
      style->inherit_flags = flags;
      style->text_style = prop[i + 2];

      if (style->inherit_flags & F_FONT)
	{
#ifndef I18N
	  XFontStruct *font = XQueryFont (dpy, prop[i + 3]);
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
	  load_font (name, &style->font);
	  XFree (name);
#endif
	}
      style->colors.fore = prop[i + 4];
      style->colors.back = prop[i + 5];
      style->relief.fore = prop[i + 6];
      style->relief.back = prop[i + 7];
      style->texture_type = prop[i + 8];
#ifndef NO_TEXTURE
      style->max_colors = prop[i + 9];
      style->back_icon.pix = prop[i + 10];
      style->back_icon.mask = prop[i + 11];
      style->tint.red = prop[i + 12];
      style->tint.green = prop[i + 13];
      style->tint.blue = prop[i + 14];
      style->gradient.npoints = prop[i + 15];
      if (style->inherit_flags & F_BACKGRADIENT)
	{
	  size_t k;
	  style->gradient.color = NEW_ARRAY (XColor, style->gradient.npoints);
	  style->gradient.offset = NEW_ARRAY (double, style->gradient.npoints);
	  for (k = 0; k < style->gradient.npoints; k++)
	    {
	      style->gradient.color[k].red = prop[i + 16 + k * 4 + 0];
	      style->gradient.color[k].green = prop[i + 16 + k * 4 + 1];
	      style->gradient.color[k].blue = prop[i + 16 + k * 4 + 2];
	      style->gradient.offset[k] = (double) prop[i + 16 + k * 4 + 3] / 0x1000000;
	    }
	  style->user_flags |= F_BACKGRADIENT;
	  style->inherit_flags &= ~F_BACKGRADIENT;
	}
      /* if there's a backpixmap, make sure it's valid and get its geometry */
      if (style->inherit_flags & F_BACKPIXMAP)
	{
	  Window junk_w;
	  int junk;
	  if( style->back_icon.pix != None )
		  if (!XGetGeometry (dpy, style->back_icon.pix, &junk_w, &junk, &junk, &style->back_icon.width, &style->back_icon.height, &junk, &junk))
			  style->back_icon.pix = None ;
	  
	  if (style->back_icon.pix == None && style->texture_type != 129 )
	    {
	      style->texture_type = 0;
	      style->back_icon.mask = None;
	      style->inherit_flags &= ~(F_BACKPIXMAP | F_BACKTRANSPIXMAP);
	    }
	}
      else if (style->inherit_flags & F_BACKTRANSPIXMAP)
	{
	  fprintf (stderr, "Transpixmap requested, but no pixmap detected for style '%s'\n", style->name);
	  fprintf (stderr, "This is a bug, please report it!\n");
	  style->inherit_flags &= ~F_BACKTRANSPIXMAP;
	}

      /* set back_icon.image if necessary */
      if (style->inherit_flags & F_BACKTRANSPIXMAP)
	{
	  MyStyle *tmp;
	  /* try to find another style with the same pixmap to inherit from */
	  for (tmp = mystyle_first; tmp != NULL; tmp = tmp->next)
	    {
	      if (tmp == style)
		continue;
	      if ((tmp->set_flags & (F_BACKPIXMAP | F_BACKTRANSPIXMAP)) && style->back_icon.pix == tmp->back_icon.pix)
		{
		  style->back_icon.image = tmp->back_icon.image;
		  break;
		}
	    }
	  /* that failed, so create the XImage here */
	  if (tmp == NULL)
	    {
	      style->back_icon.image = XGetImage (dpy, style->back_icon.pix, 0, 0, style->back_icon.width, style->back_icon.height, AllPlanes, ZPixmap);
	      style->user_flags |= F_BACKTRANSPIXMAP;
	      style->inherit_flags &= ~F_BACKTRANSPIXMAP;
	    }
	}
      i += 7 + style->gradient.npoints * 4;
#endif /* NO_TEXTURE */
      i += 9;
      style->set_flags = style->user_flags | style->inherit_flags;
    }

  /* force update of global gcs */
  mystyle_fix_styles ();
  mystyle_set_global_gcs (NULL);

  XFree (prop);
}
