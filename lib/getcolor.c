#include "../configure.h"
#include "../include/aftersteplib.h"

/****************************************************************************
 * 
 * Loads a single color
 *
 ****************************************************************************/
unsigned long
GetColor (char *name)
{
  XColor color;
  int screen = DefaultScreen(dpy);

  color.pixel = 0;
  if (!XParseColor (dpy, Scr.asv->colormap, name, &color))
    fprintf (stderr, "%s: can't parse %s\n", MyName, name);
  else if (!XAllocColor (dpy, Scr.asv->colormap, &color))
    fprintf (stderr, "%s: can't alloc %s\n", MyName, name);
  return color.pixel;
}

/****************************************************************************
 *
 * This routine computes the shadow color from the background color
 *
 ****************************************************************************/

unsigned long
GetShadow (unsigned long background)
{
  XColor bg_color;
  int screen = DefaultScreen(dpy);

  bg_color.pixel = background;
  XQueryColor (dpy, Scr.asv->colormap, &bg_color);

  /* pure black: use gray */
  if (bg_color.red == 0 && bg_color.green == 0 && bg_color.blue == 0)
    bg_color.red = bg_color.green = bg_color.blue = 0xffff;

  bg_color.red = (bg_color.red & 0xffff) >> 1;
  bg_color.green = (bg_color.green & 0xffff) >> 1;
  bg_color.blue = (bg_color.blue & 0xffff) >> 1;

  if (!XAllocColor (dpy, Scr.asv->colormap, &bg_color))
    {
      fprintf (stderr, "%s: can't alloc shadow color", MyName);
      bg_color.pixel = background;
    }

  return bg_color.pixel;
}

/****************************************************************************
 *
 * This routine computes the hilight color from the background color
 *
 ****************************************************************************/

unsigned long
GetHilite (unsigned long background)
{
  XColor bg_color, white_p;
  int screen = DefaultScreen(dpy);

  bg_color.pixel = background;
  XQueryColor (dpy, Scr.asv->colormap, &bg_color);

  white_p.pixel = WhitePixel (dpy, screen);
  XQueryColor (dpy, Scr.asv->colormap, &white_p);

  /* pure black: use gray */
  if (bg_color.red == 0 && bg_color.green == 0 && bg_color.blue == 0)
    bg_color.red = bg_color.green = bg_color.blue = 0x8924;

#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#define max(a,b) (((a)>(b)) ? (a) : (b))
#endif

  bg_color.red = max ((white_p.red / 5), bg_color.red);
  bg_color.green = max ((white_p.green / 5), bg_color.green);
  bg_color.blue = max ((white_p.blue / 5), bg_color.blue);

  bg_color.red = min (white_p.red, (bg_color.red * 140) / 100);
  bg_color.green = min (white_p.green, (bg_color.green * 140) / 100);
  bg_color.blue = min (white_p.blue, (bg_color.blue * 140) / 100);

#undef min
#ifdef max
#undef max
#endif

  if (!XAllocColor (dpy, Scr.asv->colormap, &bg_color))
    {
      fprintf (stderr, "%s: can't alloc hilight color", MyName);
      bg_color.pixel = background;
    }

  return bg_color.pixel;
}
