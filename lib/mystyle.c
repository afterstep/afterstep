/*
 * Copyright (c) 1998, 1999 Ethan Fischer <allanon@crystaltokyo.com>
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

#include "../include/aftersteplib.h"
#include "../include/XImage_utils.h"
#include "../include/loadimg.h"
#include "../include/parse.h"
#include "../include/parser.h"
#include "../include/confdefs.h"
#include "../include/mystyle.h"
#include "../include/ascolor.h"
#include "../include/stepgfx.h"
#include "../include/pixmap.h"

/*
 * if you add a member to this list, or to the MyStyle structure, 
 * remember to update mystyle_new(), mystyle_delete(), mystyle_merge_styles(), 
 * mystyle_parse(), mystyle_get_property(), and mystyle_set_property()
 */
struct config mystyle_config[] =
{
  {"~MyStyle", set_func_arg, (char **) F_DONE},
  {"Inherit", set_func_arg, (char **) F_INHERIT},
  {"Font", set_func_arg, (char **) F_FONT},
  {"ForeColor", set_func_arg, (char **) F_FORECOLOR},
  {"BackColor", set_func_arg, (char **) F_BACKCOLOR},
  {"TextStyle", set_func_arg, (char **) F_TEXTSTYLE},
#ifndef NO_TEXTURE
  {"MaxColors", set_func_arg, (char **) F_MAXCOLORS},
  {"BackGradient", set_func_arg, (char **) F_BACKGRADIENT},
  {"BackMultiGradient", set_func_arg, (char **) F_BACKMULTIGRADIENT},
  {"BackPixmap", set_func_arg, (char **) F_BACKPIXMAP},
  {"BackTransPixmap", set_func_arg, (char **) F_BACKTRANSPIXMAP},
#endif
  {"DrawTextBackground", set_func_arg, (char **) F_DRAWTEXTBACKGROUND},
  {"", NULL, NULL}
};

MyStyle *mystyle_first;

/* the GCs are static, but they are still used outside mystyle.c */
static GC ForeGC = None;
static GC BackGC = None;
static GC ReliefGC = None;
static GC ShadowGC = None;

static int style_func;
static char *style_arg;

/* just a nice error printing function */
void
mystyle_error (char *format, char *name, char *text2)
{
  fprintf (stderr, "\n%s: MyStyle [%s] parsing error:", MyName, name);
  if (text2)
    fprintf (stderr, format, text2);
  else
    fprintf (stderr, format);
}

/* Creates tinting GC for drawable d, with color tintColor,
   using color combining function function */
GC
CreateTintGC (Drawable d, unsigned long tintColor, int function)
{
  XGCValues gcvalue;
  if (tintColor == None || d == None)
    return None;

  gcvalue.function = function;
  gcvalue.foreground = tintColor;
  gcvalue.background = tintColor;
  return XCreateGC (dpy, d, GCFunction | GCForeground | GCBackground,
		    &gcvalue);
}

/* create the default style if necessary, and fill in the unset fields 
 * of the other styles from the default */
void
mystyle_fix_styles (void)
{
  MyStyle *dflt;
  MyStyle *style;

/* 
 * the user may not have set the default style, so set it here
 * all we need are reasonable defaults, and most defaults have been set 
 *  by mystyle_new() already
 * we need FONT, FORECOLOR, and BACKCOLOR, at a minimum
 */
  if ((dflt = mystyle_find ("default")) == NULL)
    dflt = mystyle_new_with_name ("default");
  if ((dflt->set_flags & F_FORECOLOR) == 0)
    dflt->user_flags |= F_FORECOLOR;
  if ((dflt->set_flags & F_BACKCOLOR) == 0)
    {
      dflt->relief.fore = GetHilite (dflt->colors.back);
      dflt->relief.back = GetShadow (dflt->colors.back);
      dflt->user_flags |= F_BACKCOLOR;
    }
  if ((dflt->set_flags & F_FONT) == 0)
    {
      if (load_font (NULL, &dflt->font) == False)
	{
	  fprintf (stderr, "%s: couldn't load default font", MyName);
	  exit (1);
	}
      dflt->user_flags |= F_FONT;
    }
  dflt->set_flags = dflt->inherit_flags | dflt->user_flags;

/* fix up the styles, using the default style */
  for (style = mystyle_first; style != NULL; style = style->next)
    mystyle_merge_styles (dflt, style, False, False);
}

static void mystyle_draw_textline (Window w, MyStyle * style, const char *text, int len, int x, int y);

#ifdef I18N
#undef FONTSET
#define FONTSET (*style).font.fontset
#endif
void
mystyle_draw_vertical_text (Window w, MyStyle * style, const char *text, int x, int y)
{
  char str[2];
  str[1] = '\0';
  for (; *text != '\0'; text++)
    {
      int width = XTextWidth ((*style).font.font, text, 1);
      str[0] = *text;
      mystyle_draw_textline (w, style, str, 1, x + ((*style).font.width - width) / 2, y);
      y += (*style).font.height;
    }
}

void
mystyle_get_text_geometry (MyStyle * style, const char *str, int len, int *width, int *height)
{
  const char *ptr;
  int mw = 0, mh = 0;

  while (len > 0)
    {
      int w;
      for (ptr = str; len > 0 && *ptr != '\n'; ptr++, len--);
      w = XTextWidth (style->font.font, str, ptr - str);
      if (mw < w)
	mw = w;
      mh += style->font.height;
      str = ptr + (*ptr == '\n');
    }
  if (width != NULL)
    *width = mw;
  if (height != NULL)
    *height = mh;
}

void
mystyle_draw_text (Window w, MyStyle * style, const char *text, int x, int y)
{
  mystyle_set_global_gcs (style);
  if ((style->set_flags & F_DRAWTEXTBACKGROUND) && (style->flags & F_DRAWTEXTBACKGROUND))
    {
      XSetFillStyle (dpy, BackGC, FillSolid);
      XFillRectangle (dpy, w, BackGC, x - 2, y - style->font.y - 2, XTextWidth (style->font.font, text, strlen (text)) + 4, style->font.height + 4);
#ifndef NO_TEXTURE
      if ((style->texture_type != 0) && (style->back_icon.pix != None))
	XSetFillStyle (dpy, BackGC, FillTiled);
#endif /* !NO_TEXTURE */
    }

  while (*text != '\0')
    {
      const char *ptr;
      for (ptr = text; *ptr != '\0' && *ptr != '\n'; ptr++);
      mystyle_draw_textline (w, style, text, ptr - text, x, y);
      y += style->font.height;
      text = (*ptr == '\n') ? ptr + 1 : ptr;
    }
}

static void
mystyle_draw_textline (Window w, MyStyle * style, const char *text, int len, int x, int y)
{
  switch (style->text_style)
    {
/* 3d look #1 */
    case 1:
      XDrawString (dpy, w, ReliefGC, x - 2, y - 2, text, len);
      XDrawString (dpy, w, ReliefGC, x - 1, y - 2, text, len);
      XDrawString (dpy, w, ReliefGC, x + 0, y - 2, text, len);
      XDrawString (dpy, w, ReliefGC, x + 1, y - 2, text, len);
      XDrawString (dpy, w, ReliefGC, x + 2, y - 2, text, len);

      XDrawString (dpy, w, ReliefGC, x - 2, y - 1, text, len);
      XDrawString (dpy, w, ReliefGC, x - 2, y + 0, text, len);
      XDrawString (dpy, w, ReliefGC, x - 2, y + 1, text, len);
      XDrawString (dpy, w, ReliefGC, x - 2, y + 2, text, len);

      XDrawString (dpy, w, ShadowGC, x + 2, y + 0, text, len);
      XDrawString (dpy, w, ShadowGC, x + 2, y + 1, text, len);
      XDrawString (dpy, w, ShadowGC, x + 2, y + 2, text, len);
      XDrawString (dpy, w, ShadowGC, x + 1, y + 2, text, len);
      XDrawString (dpy, w, ShadowGC, x + 0, y + 2, text, len);

      XDrawString (dpy, w, ForeGC, x + 0, y + 0, text, len);
      break;
/* 3d look #2 */
    case 2:
      XDrawString (dpy, w, ShadowGC, x - 1, y - 1, text, len);
      XDrawString (dpy, w, ReliefGC, x + 0, y + 0, text, len);
      XDrawString (dpy, w, ForeGC, x + 1, y + 1, text, len);
      break;
/* normal text */
    default:
    case 0:
      XDrawString (dpy, w, ForeGC, x + 0, y + 0, text, len);
      break;
    }
}

icon_t
mystyle_make_icon (MyStyle * style, int width, int height, Pixmap cache)
{
  icon_t icon =
  {None, None, 0, 0};
#ifndef NO_TEXTURE
  GC gc, mgc = None;
  XGCValues gcv;
  unsigned long gcm;
  int screen = DefaultScreen(dpy);

  icon.pix = XCreatePixmap (dpy, RootWindow (dpy, screen), width, height, DefaultDepth (dpy, screen));
  gcv.foreground = style->colors.back;
  gcv.background = style->colors.fore;
  gcv.fill_style = FillSolid;
  gcv.graphics_exposures = False;
  gcm = GCForeground | GCBackground | GCFillStyle | GCGraphicsExposures;
  if (style->texture_type == 128)
    {
      gcv.fill_style = FillTiled;
      gcv.tile = style->back_icon.pix;
      gcm |= GCTile;
      if (style->back_icon.mask != None)
	icon.mask = XCreatePixmap (dpy, RootWindow (dpy, screen), width, height, 1);
    }
  gc = XCreateGC (dpy, icon.pix, gcm, &gcv);
  if (icon.mask != None)
    {
      gcv.tile = style->back_icon.mask;
      gcm = GCFillStyle | GCGraphicsExposures | GCTile;
      mgc = XCreateGC (dpy, style->back_icon.mask, gcm, &gcv);
    }
  switch (style->texture_type)
    {
    case TEXTURE_PIXMAP:
      if (icon.mask != None)
	XFillRectangle (dpy, icon.mask, mgc, 0, 0, width, height);
      /* fall through to case TEXTURE_SOLID */
    case TEXTURE_SOLID:
      XFillRectangle (dpy, icon.pix, gc, 0, 0, width, height);
      break;
    default:			/* gradients */
      if (style->texture_type <= TEXTURE_SOLID || style->texture_type >= TEXTURE_PIXMAP)
	{
	  XFreePixmap (dpy, icon.pix);
	  icon.pix = None;
	}
      /* use the cache if we can */
      if (cache != None && width < DisplayWidth (dpy, screen) &&
	  height < DisplayHeight (dpy, screen))
	{
	  XCopyArea (dpy, cache, icon.pix, gc, 0, 0, width, height, 0, 0);
	}
      else
	{
	  draw_gradient (dpy, icon.pix, 0, 0, width, height,
			 style->gradient.npoints, style->gradient.color,
			 style->gradient.offset, 0, style->texture_type,
			 style->max_colors, 40);
	}
      break;
    }
  XFreeGC (dpy, gc);
  if (mgc != None)
    XFreeGC (dpy, mgc);
  if (icon.pix != None)
    {
      icon.width = width;
      icon.height = height;
    }
#endif /* NO_TEXTURE */
  return icon;
}

icon_t
mystyle_make_icon_overlay (MyStyle * style, int root_x, int root_y, int width, int height, Pixmap cache)
{
  icon_t icon =
  {None, None, 0, 0};
  if (style->texture_type < TEXTURE_TRANSPARENT)
    icon = mystyle_make_icon (style, width, height, cache);
#ifndef NO_TEXTURE
  else if (style->texture_type == TEXTURE_TRANSPARENT)
    fill_with_darkened_background (&icon.pix, style->tint, 0, 0, width, height, root_x, root_y, 0);
  else if (style->texture_type == TEXTURE_TRANSPIXMAP)
    fill_with_pixmapped_background (&icon.pix, style->back_icon.image, 0, 0, width, height, root_x, root_y, 0);
  if (icon.pix != None)
    {
      icon.width = width;
      icon.height = height;
    }
#endif /* !NO_TEXTURE */
  return icon;
}

Pixmap
mystyle_make_pixmap (MyStyle * style, int width, int height, Pixmap cache)
{
  icon_t icon = mystyle_make_icon (style, width, height, cache);
  if (icon.mask != None)
    XFreePixmap (dpy, icon.mask);
  return icon.pix;
}

Pixmap
mystyle_make_pixmap_overlay (MyStyle * style, int root_x, int root_y, int width, int height, Pixmap cache)
{
  icon_t icon = mystyle_make_icon_overlay (style, root_x, root_y, width, height, cache);
  if (icon.mask != None)
    XFreePixmap (dpy, icon.mask);
  return icon.pix;
}

/* set a window's background for XClearArea */
void
mystyle_set_window_background (Window w, MyStyle * style)
{
#ifndef NO_TEXTURE
  if (style->texture_type == 128 && style->set_flags & F_BACKPIXMAP)
    XSetWindowBackgroundPixmap (dpy, w, style->back_icon.pix);
  else if (style->texture_type == 129)
    XSetWindowBackgroundPixmap (dpy, w, ParentRelative);
  else
#endif /* NO_TEXTURE */
    XSetWindowBackground (dpy, w, style->colors.back);
}

/*
 * set the standard global drawing GCs
 * avoids resetting the GCs when possible
 * if passed a NULL pointer, forces update of the cached GCs if possible
 * WARNING: the GCs are invalid until this function is called
 */
void
mystyle_set_global_gcs (MyStyle * style)
{
  static MyStyle *old_style = NULL;
  static int make_GCs = 1;

/* make the drawing GCs if necessary */
  if (make_GCs)
    {
      XGCValues gcv;
      unsigned long gcm;
      int screen = DefaultScreen(dpy);
      
      gcv.graphics_exposures = False;
      gcm = GCGraphicsExposures;
      ForeGC = XCreateGC (dpy, RootWindow (dpy, screen), gcm, &gcv);
      BackGC = XCreateGC (dpy, RootWindow (dpy, screen), gcm, &gcv);
      ReliefGC = XCreateGC (dpy, RootWindow (dpy, screen), gcm, &gcv);
      ShadowGC = XCreateGC (dpy, RootWindow (dpy, screen), gcm, &gcv);

      make_GCs = 0;
    }

/* set the GCs if the style is different */
  if (style == NULL && old_style != NULL)
    {
      MyStyle *s;
      /* force update the current style if it still exists */
      for (s = mystyle_first; s != NULL && s != old_style; s = s->next);
      if (s != NULL)
	mystyle_set_gcs (s, ForeGC, BackGC, ReliefGC, ShadowGC);
    }
  else if (old_style != style)
    {
      old_style = style;
      mystyle_set_gcs (style, ForeGC, BackGC, ReliefGC, ShadowGC);
    }
}

void
mystyle_set_gcs (MyStyle * style, GC foreGC, GC backGC, GC reliefGC, GC shadowGC)
{
  XGCValues gcv;
  unsigned long gcm;

/* set the drawing GC */
  gcv.foreground = style->colors.fore;
  gcv.background = style->colors.back;
  gcv.font = style->font.font->fid;
  gcv.fill_style = FillSolid;
  gcm = GCForeground | GCBackground | GCFont | GCFillStyle;
  if (foreGC != None)
    XChangeGC (dpy, foreGC, gcm, &gcv);

  gcv.foreground = style->colors.back;
#ifndef NO_TEXTURE
  if ((style->texture_type != 0) && (style->back_icon.pix != None))
    {
      gcv.fill_style = FillTiled;
      gcv.tile = style->back_icon.pix;
      gcm |= GCTile;
    }
#endif /* NO_TEXTURE */
  if (backGC != None)
    XChangeGC (dpy, backGC, gcm, &gcv);

/* set the relief GC */
  gcv.foreground = style->relief.fore;
  gcv.background = style->relief.back;
  gcv.fill_style = FillSolid;
  gcm = GCForeground | GCBackground | GCFont | GCFillStyle;
  if (reliefGC != None)
    XChangeGC (dpy, reliefGC, gcm, &gcv);

/* set the shadow GC */
  gcv.foreground = style->relief.back;
  gcv.background = style->relief.fore;
  gcv.fill_style = FillSolid;
  gcm = GCForeground | GCBackground | GCFont | GCFillStyle;
  if (shadowGC != None)
    XChangeGC (dpy, shadowGC, gcm, &gcv);
}

void
mystyle_get_global_gcs (MyStyle * style, GC * foreGC, GC * backGC, GC * reliefGC, GC * shadowGC)
{
  mystyle_set_global_gcs (style);
  if (foreGC != NULL)
    *foreGC = ForeGC;
  if (backGC != NULL)
    *backGC = BackGC;
  if (reliefGC != NULL)
    *reliefGC = ReliefGC;
  if (shadowGC != NULL)
    *shadowGC = ShadowGC;
}

MyStyle *
mystyle_new (void)
{
  MyStyle *style = (MyStyle *) safemalloc (sizeof (MyStyle));
  int screen = DefaultScreen(dpy);
  
  style->next = mystyle_first;
  mystyle_first = style;

  style->user_flags = 0;
  style->inherit_flags = 0;
  style->set_flags = 0;
  style->flags = 0;
  style->name = NULL;
  style->text_style = 0;
  style->font.name = NULL;
  style->font.font = NULL;
#ifdef I18N
  style->font.fontset = NULL;
#endif
  style->colors.fore = GetColor ("white");
  style->colors.back = GetColor ("black");
  style->relief.fore = style->colors.back;
  style->relief.back = style->colors.fore;
  style->texture_type = 0;
#ifndef NO_TEXTURE
  style->gradient.npoints = 0;
  style->gradient.color = NULL;
  style->gradient.offset = NULL;
  style->back_icon.pix = None;
  style->back_icon.mask = None;
  if (DefaultDepth (dpy, screen) > 16)
    style->max_colors = 128;
  else if (DefaultDepth (dpy, screen) > 8)
    style->max_colors = 32;
  else
    style->max_colors = 10;
  style->tint.red = style->tint.green = style->tint.blue = 0xFFFF;
  style->back_icon.image = NULL;
#endif
  return style;
}

MyStyle *
mystyle_new_with_name (char *name)
{
  MyStyle *style = mystyle_new ();

  if (name != NULL)
    {
      style->name = (char *) safemalloc (strlen (name) + 1);
      strcpy (style->name, name);
    }
  return style;
}

/*
 * deletes memory alloc'd for style (including members)
 */

void
mystyle_delete (MyStyle * style)
{
  MyStyle *s1;
  MyStyle *s2;

  /* remove ourself from the list */
  for (s1 = s2 = mystyle_first; s1 != NULL; s2 = s1, s1 = s1->next)
    if (s1 == style)
      break;
  if (s1 != NULL)
    {
      if (mystyle_first == s1)
	mystyle_first = s1->next;
      else
	s2->next = s1->next;
    }
  /* delete members */
  if (style->name != NULL)
    free (style->name);
  if (style->user_flags & F_FONT)
    {
      unload_font (&style->font);
    }
#ifndef NO_TEXTURE
  if (style->user_flags & F_BACKGRADIENT)
    {
      free (style->gradient.color);
      free (style->gradient.offset);
    }
  if (style->user_flags & F_BACKPIXMAP)
    {
      UnloadImage (style->back_icon.pix);
    }
  if (style->user_flags & F_BACKTRANSPIXMAP)
    XDestroyImage (style->back_icon.image);
#endif

  /* free our own mem */
  free (style);
}

/* find a style by name */
MyStyle *
mystyle_find (const char *name)
{
  MyStyle *style = NULL;
  if (name != NULL)
    for (style = mystyle_first; style != NULL; style = style->next)
      if (!mystrcasecmp (style->name, name))
	break;
  return style;
}

/* find a style by name or return the default style */
MyStyle *
mystyle_find_or_default (const char *name)
{
  MyStyle *style = mystyle_find (name);
  if (style == NULL)
    style = mystyle_find ("default");
  return style;
}

/*
 * merges two styles, with the result in child
 * if override == False, will not override fields already set
 * if copy == True, copies instead of inheriting; this is important, because 
 *   inherited members are deleted when their real parent is deleted; don't 
 *   inherit if the parent style could go away before the child
 */
void
mystyle_merge_styles (MyStyle * parent, MyStyle * child, Bool override, Bool copy)
{
  int screen = DefaultScreen(dpy);

  if (parent->set_flags & F_FONT)
    {
      if ((override == True) && (child->user_flags & F_FONT))
	unload_font (&child->font);
      if ((override == True) || !(child->set_flags & F_FONT))
	{
	  if (copy == False)
	    {
#ifdef I18N
	      load_font (parent->font.name, &child->font);
#else
	      child->font = parent->font;
#endif
	      child->user_flags &= ~F_FONT;
	      child->inherit_flags |= F_FONT;
	    }
	  else
	    {
	      if (load_font (parent->font.name, &child->font) == True)
		{
		  child->user_flags |= F_FONT;
		  child->inherit_flags &= ~F_FONT;
		}
	    }
	}
    }
  if (parent->set_flags & F_TEXTSTYLE)
    {
      if ((override == True) || !(child->set_flags & F_TEXTSTYLE))
	{
	  child->text_style = parent->text_style;

	  if (copy == False)
	    {
	      child->user_flags &= ~F_TEXTSTYLE;
	      child->inherit_flags |= F_TEXTSTYLE;
	    }
	  else
	    {
	      child->user_flags |= F_TEXTSTYLE;
	      child->inherit_flags &= ~F_TEXTSTYLE;
	    }
	}
    }
  if (parent->set_flags & F_FORECOLOR)
    {
      if ((override == True) || !(child->set_flags & F_FORECOLOR))
	{
	  if (override == True)
	    child->texture_type = parent->texture_type;
	  child->colors.fore = parent->colors.fore;
	  if (copy == False)
	    {
	      child->user_flags &= ~F_FORECOLOR;
	      child->inherit_flags |= F_FORECOLOR;
	    }
	  else
	    {
	      child->user_flags |= F_FORECOLOR;
	      child->inherit_flags &= ~F_FORECOLOR;
	    }
	}
    }
  if (parent->set_flags & F_BACKCOLOR)
    {
      if ((override == True) || !(child->set_flags & F_BACKCOLOR))
	{
	  child->colors.back = parent->colors.back;
	  child->relief = parent->relief;
	  if (copy == False)
	    {
	      child->user_flags &= ~F_BACKCOLOR;
	      child->inherit_flags |= F_BACKCOLOR;
	    }
	  else
	    {
	      child->user_flags |= F_BACKCOLOR;
	      child->inherit_flags &= ~F_BACKCOLOR;
	    }
	}
    }
#ifndef NO_TEXTURE
  if (parent->set_flags & F_MAXCOLORS)
    {
      if ((override == True) || !(child->set_flags & F_MAXCOLORS))
	{
	  child->max_colors = parent->max_colors;
	  if (copy == False)
	    {
	      child->user_flags &= ~F_MAXCOLORS;
	      child->inherit_flags |= F_MAXCOLORS;
	    }
	  else
	    {
	      child->user_flags |= F_MAXCOLORS;
	      child->inherit_flags &= ~F_MAXCOLORS;
	    }
	}
    }
  if (parent->set_flags & F_BACKGRADIENT)
    {
      if ((override == True) || !(child->set_flags & F_BACKGRADIENT))
	{
	  if (override == True)
	    child->texture_type = parent->texture_type;
	  child->gradient = parent->gradient;
	  if (copy == False)
	    {
	      child->user_flags &= ~F_BACKGRADIENT;
	      child->inherit_flags |= F_BACKGRADIENT;
	    }
	  else
	    {
	      child->user_flags |= F_BACKGRADIENT;
	      child->inherit_flags &= ~F_BACKGRADIENT;
	    }
	}
    }
  if (parent->set_flags & F_BACKPIXMAP)
    {
      if ((override == True) && (child->user_flags & F_BACKPIXMAP))
	{
	  UnloadImage (child->back_icon.pix);
	  if (child->user_flags & F_BACKTRANSPIXMAP)
	    XDestroyImage (child->back_icon.image);
	}
      if ((override == True) || !(child->set_flags & F_BACKPIXMAP))
	{
	  if (override == True)
	    child->texture_type = parent->texture_type;
	  if (copy == False)
	    {
	      child->back_icon = parent->back_icon;
	      child->user_flags &= ~(F_BACKPIXMAP | F_BACKTRANSPIXMAP);
	      child->inherit_flags |= F_BACKPIXMAP | (parent->set_flags & F_BACKTRANSPIXMAP);
	    }
	  else
	    {
	      GC gc = DefaultGC (dpy, screen);
	      child->back_icon.pix = XCreatePixmap (dpy, RootWindow (dpy, screen), parent->back_icon.width, parent->back_icon.height, DefaultDepth (dpy, screen));
	      XCopyArea (dpy, parent->back_icon.pix, child->back_icon.pix, gc,
	      0, 0, parent->back_icon.width, parent->back_icon.height, 0, 0);
	      if (parent->back_icon.mask != None)
		{
		  GC mgc = XCreateGC (dpy, parent->back_icon.mask, 0, NULL);
		  child->back_icon.pix = XCreatePixmap (dpy, RootWindow (dpy, screen), parent->back_icon.width, parent->back_icon.height, 1);
		  XCopyArea (dpy, parent->back_icon.pix, child->back_icon.pix, mgc,
			     0, 0, parent->back_icon.width, parent->back_icon.height, 0, 0);
		  XFreeGC (dpy, mgc);
		}
	      if (parent->set_flags & F_BACKTRANSPIXMAP)
		{
		  child->back_icon.image = CreateXImageAndData (dpy, DefaultVisual (dpy, screen), DefaultDepth (dpy, screen), ZPixmap, 0, parent->back_icon.image->width, parent->back_icon.image->height);
		  memcpy (child->back_icon.image, parent->back_icon.image, GetXImageDataSize (child->back_icon.image));
		}
	      child->back_icon.width = parent->back_icon.width;
	      child->back_icon.height = parent->back_icon.height;
	      child->user_flags |= F_BACKPIXMAP | (parent->set_flags & F_BACKTRANSPIXMAP);
	      child->inherit_flags &= ~(F_BACKPIXMAP | F_BACKTRANSPIXMAP);
	    }
	}
    }
  if (parent->texture_type == TEXTURE_TRANSPARENT && (override == True || child->texture_type != TEXTURE_TRANSPARENT))
    {
      child->tint = parent->tint;
    }
#endif /* NO_TEXTURE */
  if (parent->set_flags & F_DRAWTEXTBACKGROUND)
    {
      if ((override == True) || !(child->set_flags & F_DRAWTEXTBACKGROUND))
	{
	  child->flags &= ~F_DRAWTEXTBACKGROUND;
	  child->flags |= parent->flags & F_DRAWTEXTBACKGROUND;
	  if (copy == False)
	    {
	      child->user_flags &= ~F_DRAWTEXTBACKGROUND;
	      child->inherit_flags |= F_DRAWTEXTBACKGROUND;
	    }
	  else
	    {
	      child->user_flags |= F_DRAWTEXTBACKGROUND;
	      child->inherit_flags &= ~F_DRAWTEXTBACKGROUND;
	    }
	}
    }
  child->set_flags = child->user_flags | child->inherit_flags;
}

void
mystyle_parse (char *tline, FILE * fd, char **ppath, int *junk2)
{
  MyStyle *style;
  char *newline;
  char *name = stripcpy2 (tline, 0);

  if (name == NULL)
    {
      fprintf (stderr, "%s: bad style name '%s'", MyName, tline);
      return;
    }
  newline = safemalloc (MAXLINELENGTH + 1);
/* if this style was already defined, find it */
  if ((style = mystyle_find (name)) != NULL)
    {
      free (name);
      name = style->name;
    }
  else
    {
      style = mystyle_new ();
      style->name = name;
    }

  while (fgets (newline, MAXLINELENGTH, fd))
    {
      char *p;
      p = stripcomments (newline);
      if (*p != '\0')
	if (mystyle_parse_member (style, p, *ppath) != False)
	  break;
    }

  free (newline);
}

/*
 * parse a style member, for example:
 *   mystyle_parse_member(mystyle_find("default"), "BackColor black");
 * this function will likely modify the string argument
 * returns 1 when a "~MyStyle" is parsed
 */
int
mystyle_parse_member (MyStyle * style, char *str, const char *PixmapPath)
{
  int done = 0;
  struct config *config = find_config (mystyle_config, str);
  int screen = DefaultScreen(dpy);
  
  style_func = F_ERROR;
  style_arg = NULL;
    
  if (config != NULL)
    config->action (str + strlen (config->keyword), NULL, config->arg, config->arg2);
  else
    fprintf (stderr, "%s: unknown style command: %s\n", MyName, str);
  if (style_arg == NULL)
    {
      fprintf (stderr, "%s: bad style arg: %s\n", MyName, str);
      return 0;
    }
  else
    {
      style->inherit_flags &= ~style_func;
      switch (style_func)
	{
	case F_INHERIT:
	  {
	    MyStyle *parent = mystyle_find (style_arg);
	    if (parent != NULL)
	      mystyle_merge_styles (parent, style, True, False);
	    else
	      fprintf (stderr, "%s: unknown style: %s\n", MyName, style_arg);
	  }
	  break;
	case F_FONT:
	  if (style->user_flags & style_func)
	    unload_font (&style->font);
	  style->user_flags &= ~style_func;
	  if (load_font (style_arg, &style->font) == True)
	    style->user_flags |= style_func;
	  break;
	case F_TEXTSTYLE:
	  style->text_style = strtol (style_arg, NULL, 10);
	  style->user_flags |= style_func;
	  break;
	case F_FORECOLOR:
	  style->colors.fore = GetColor (style_arg);
	  style->user_flags |= style_func;
	  break;
	case F_BACKCOLOR:
	  style->texture_type = 0;
	  style->colors.back = GetColor (style_arg);
	  style->relief.fore = GetHilite (style->colors.back);
	  style->relief.back = GetShadow (style->colors.back);
	  style->user_flags |= style_func;
	  break;
#ifndef NO_TEXTURE
	case F_MAXCOLORS:
	  style->max_colors = strtol (style_arg, NULL, 10);
	  style->user_flags |= style_func;
	  break;
	case F_BACKGRADIENT:
	  {
	    char *ptr;
	    int type = strtol (style_arg, &ptr, 10);
	    char *color1 = strtok (ptr, " \t\v\r\n\f");
	    char *color2 = strtok (NULL, " \t\v\r\n\f");
	    gradient_t gradient;
	    if (color1 != NULL && color2 != NULL && (type = mystyle_parse_old_gradient (type, color1, color2, &gradient)) >= 0)
	      {
		if (style->user_flags & F_BACKGRADIENT)
		  {
		    free (style->gradient.color);
		    free (style->gradient.offset);
		  }
		style->gradient = gradient;
		style->texture_type = type;
		style->user_flags |= style_func;
	      }
	    else
	      fprintf (stderr, "%s: bad gradient: %s\n", MyName, style_arg);
	  }
	  break;
	case F_BACKMULTIGRADIENT:
	  {
	    char *ptr;
	    int type = strtol (style_arg, &ptr, 10);
	    gradient_t gradient;
	    int error = 0;

	    gradient.npoints = 0;
	    gradient.color = NULL;
	    gradient.offset = NULL;
	    if (type < TEXTURE_GRADIENT_TL2BR || type >= TEXTURE_PIXMAP)
	      error = 4;
	    while (!error && ptr != NULL && *ptr != '\0')
	      {
		XColor color;
		double offset;
		if (!error)
		  {
		    char *name;
		    for (name = ptr; isspace (*name); name++);
		    for (ptr = name; *ptr != '\0' && !isspace (*ptr); ptr++);
		    if (*ptr != '\0')
		      *ptr++ = '\0';
		    if (!XParseColor (dpy, DefaultColormap (dpy, screen), name, &color))
		      error = 1;
		  }
		if (!error)
		  {
		    char *ptr2;
		    offset = strtod (ptr, &ptr2);
		    if (ptr == ptr2)
		      error = 2;
		    ptr = ptr2;
		  }
		if (!error)
		  {
		    if (gradient.npoints > 0 && offset < gradient.offset[gradient.npoints - 1])
		      error = 5;
		  }
		if (!error)
		  {
		    gradient.npoints++;
		    gradient.color = realloc (gradient.color, sizeof (XColor) * gradient.npoints);
		    gradient.offset = realloc (gradient.offset, sizeof (double) * gradient.npoints);
		    gradient.color[gradient.npoints - 1] = color;
		    gradient.offset[gradient.npoints - 1] = offset;
		    for (; isspace (*ptr); ptr++);
		  }
	      }
	    if (!error)
	      {
		if (gradient.npoints < 2)
		  error = 3;
	      }
	    if (!error)
	      {
		gradient.offset[0] = 0.0;
		gradient.offset[gradient.npoints - 1] = 1.0;
		if (style->user_flags & F_BACKGRADIENT)
		  {
		    free (style->gradient.color);
		    free (style->gradient.offset);
		  }
		style->gradient = gradient;
		style->texture_type = type;
		style->user_flags |= F_BACKGRADIENT;
	      }
	    else
	      {
		if (gradient.color != NULL)
		  free (gradient.color);
		if (gradient.offset != NULL)
		  free (gradient.offset);
		fprintf (stderr, "%s: bad gradient (error %d): %s\n", MyName, error, style_arg);
	      }
	  }
	  break;
	case F_BACKPIXMAP:
	  {
	    char *ptr;
	    int type = strtol (style_arg, &ptr, 10);
	    char *tmp = stripcpy (ptr);
	    int colors = -1;
	    style->inherit_flags &= ~F_BACKTRANSPIXMAP;
	    if (style->set_flags & F_MAXCOLORS)
	      colors = style->max_colors;
	    if (type == 129)
	      {
		style->texture_type = type;
		style->tint.pixel = 0;
		if (strlen (tmp))
		  style->tint.pixel = GetColor (tmp);
		if (style->tint.pixel == 0)	/* use no tinting by default */
		  style->tint.pixel = GetColor ("white");

		XQueryColor (dpy, DefaultColormap (dpy, screen), &style->tint);
	      }
	    else
	      {
		char *path;
		Pixmap pix, mask;
		if ((path = findIconFile (tmp, PixmapPath, R_OK)) != NULL &&
		    (pix = LoadImageWithMask (dpy, RootWindow (dpy, screen), colors, path, &mask)) != None)
		  {
		    Window r;
		    int d, width, height;
		    XGetGeometry (dpy, pix, &r, &d, &d, &width, &height, &d, &d);
		    style->back_icon.width = width;
		    style->back_icon.height = height;
		    if (style->user_flags & style_func)
		      UnloadImage (style->back_icon.pix);
		    style->back_icon.pix = pix;
		    style->back_icon.mask = mask;
		    style->user_flags |= style_func;
		    style->texture_type = type;
		    /* now set the transparency image, if necessary */
		    style->inherit_flags &= ~F_BACKTRANSPIXMAP;
		    if (style->user_flags & F_BACKTRANSPIXMAP)
		      XDestroyImage (style->back_icon.image);
		    if (type == TEXTURE_TRANSPIXMAP)
		      {
			style->back_icon.image = XGetImage (dpy, pix, 0, 0, width, height, AllPlanes, ZPixmap);
			style->user_flags |= F_BACKTRANSPIXMAP;
		      }
		  }
		else
		  fprintf (stderr, "%s: unable to load pixmap: '%s'\n", MyName, tmp);

		if (path != NULL)
		  free (path);
	      }
	    free (tmp);
	  }
	  break;
#endif
	case F_DRAWTEXTBACKGROUND:
	  style->inherit_flags &= ~style_func;
	  style->user_flags |= style_func;
	  if (style_arg[0] != '\0' && strtol (style_arg, NULL, 10) == 0)
	    style->flags &= ~style_func;
	  else
	    style->flags |= style_func;
	  break;

	case F_DONE:
	  done = 1;
	  break;

	case F_ERROR:
	default:
	  fprintf (stderr, "%s: unknown style command: %s\n", MyName, str);
	  break;
	}
    }
  style->set_flags = style->inherit_flags | style->user_flags;
  free (style_arg);
  return done;
}

void
mystyle_parse_set_style (char *text, FILE * fd, char **style, int *junk2)
{
  char *name = stripcpy2 (text, 0);
  if (name != NULL)
    {
      *(MyStyle **) style = mystyle_find (name);
      if (*style == NULL)
	fprintf (stderr, "%s: unknown style name: '%s'\n", MyName, name);
      free (name);
    }
}

void
set_func_arg (char *text, FILE * fd, char **value, int *junk)
{
  style_arg = stripcpy (text);
  style_func = (unsigned long) value;
}

/*
 * convert an old two-color gradient to a multi-point gradient
 */
int
mystyle_parse_old_gradient (int type, const char *color1, const char *color2, gradient_t * gradient)
{
  int cylindrical = 0;
  XColor c1, c2;
  int screen = DefaultScreen(dpy);
  if (!XParseColor (dpy, DefaultColormap (dpy, screen), color1, &c1) ||
      !XParseColor (dpy, DefaultColormap (dpy, screen), color2, &c2))
    return -1;
  switch (type)
    {
    case TEXTURE_GRADIENT:
      type = TEXTURE_GRADIENT_TL2BR;
      break;
    case TEXTURE_HGRADIENT:
      type = TEXTURE_GRADIENT_T2B;
      break;
    case TEXTURE_HCGRADIENT:
      type = TEXTURE_GRADIENT_T2B;
      cylindrical = 1;
      break;
    case TEXTURE_VGRADIENT:
      type = TEXTURE_GRADIENT_L2R;
      break;
    case TEXTURE_VCGRADIENT:
      type = TEXTURE_GRADIENT_L2R;
      cylindrical = 1;
      break;
    default:
      break;
    }
  gradient->npoints = 2 + cylindrical;
  gradient->color = NEW_ARRAY (XColor, gradient->npoints);
  gradient->offset = NEW_ARRAY (double, gradient->npoints);
  gradient->color[0] = c1;
  gradient->color[1] = c2;
  if (cylindrical)
    gradient->color[2] = c1;
  gradient->offset[0] = 0.0;
  if (cylindrical)
    gradient->offset[1] = 0.5;
  gradient->offset[gradient->npoints - 1] = 1.0;
  return type;
}
