/*
 * Copyright (C) 1996 Frank Fejes
 * Copyright (C) 1996 Alfredo Kojima 
 * Copyright (C) 1995 Bo Yang
 * Copyright (C) 1993 Robert Nation
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
 * Reads motif mwm window manager hints from a window, and makes necessary
 * adjustments for afterstep. 
 * 
 ****************************************************************************/

#include "../../configure.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"

#include "menus.h"

extern Atom _XA_MwmAtom;
extern Atom _XA_WIN_STATE;

/* Motif WM window hints structure */
typedef struct
  {
    CARD32 flags;		/* window hints */
    CARD32 functions;		/* requested functions */
    CARD32 decorations;		/* requested decorations */
    INT32 inputMode;		/* input mode */
    CARD32 status;		/* status (ignored) */
  }
MwmHints;

/* Motif WM window hints */
#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)

/* Motif WM function flags */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

/* Motif WM decor flags */
#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)

/* Motif WM input modes */
#define MWM_INPUT_MODELESS 0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL 2
#define MWM_INPUT_FULL_APPLICATION_MODAL 3
#define MWM_INPUT_APPLICATION_MODAL MWM_INPUT_PRIMARY_APPLICATION_MODAL

#define PROP_MOTIF_WM_HINTS_ELEMENTS  5
#define PROP_MWM_HINTS_ELEMENTS       PROP_MOTIF_WM_HINTS_ELEMENTS

/* Gnome WM window hints */
#define WIN_HINTS_SKIP_FOCUS      (1<<0)	/*"alt-tab" skips this win */
#define WIN_HINTS_SKIP_WINLIST    (1<<1)	/*do not show in window list */
#define WIN_HINTS_SKIP_TASKBAR    (1<<2)	/*do not show on taskbar */
#define WIN_HINTS_GROUP_TRANSIENT (1<<3)	/*Reserved - definition is unclear */
#define WIN_HINTS_FOCUS_ON_CLICK  (1<<4)	/*app only accepts focus if clicked */
#define WIN_HINTS_DO_NOT_COVER    (1<<5)	/*attempt to not cover this window */

/* Gnome WM window states */
#define WIN_STATE_STICKY          (1<<0)	/*everyone knows sticky */
#define WIN_STATE_MINIMIZED       (1<<1)	/*Reserved - definition is unclear */
#define WIN_STATE_MAXIMIZED_VERT  (1<<2)	/*window in maximized V state */
#define WIN_STATE_MAXIMIZED_HORIZ (1<<3)	/*window in maximized H state */
#define WIN_STATE_HIDDEN          (1<<4)	/*not on taskbar but window visible */
#define WIN_STATE_SHADED          (1<<5)	/*shaded (MacOS / Afterstep style) */
#define WIN_STATE_HID_WORKSPACE   (1<<6)	/*not on current desktop */
#define WIN_STATE_HID_TRANSIENT   (1<<7)	/*owner of transient is hidden */
#define WIN_STATE_FIXED_POSITION  (1<<8)	/*window is fixed in position even */
#define WIN_STATE_ARRANGE_IGNORE  (1<<9)	/*ignore for auto arranging */

extern ASWindow *Tmp_win;

/****************************************************************************
 * 
 * Reads the property MOTIF_WM_HINTS
 *
 *****************************************************************************/
void
GetMwmHints (ASWindow * t)
{
  int actual_format;
  Atom actual_type;
  unsigned long nitems, bytesafter;
  int *mwm_hints;

  if (				/*(Scr.flags & (MWMDecorHints | MWMFunctionHints)) && */
   (XGetWindowProperty (dpy, t->w, _XA_MwmAtom, 0L, 20L, False, _XA_MwmAtom,
			&actual_type, &actual_format, &nitems, &bytesafter,
			(unsigned char **) &mwm_hints) == Success))
    {
      if (nitems >= 4)
	t->mwm_hints = mwm_hints;
    }
  else
    t->mwm_hints = NULL;
}

unsigned long
GetGnomeState (ASWindow * t)
{
  int actual_format;
  Atom actual_type;
  unsigned long nitems, bytesafter;
  unsigned long state = 0;
  unsigned long *data;

  if (XGetWindowProperty (dpy, t->w, _XA_WIN_STATE, 0, 1, False, XA_CARDINAL,
			  &actual_type, &actual_format, &nitems, &bytesafter,
			  (unsigned char **) &data) == Success && data)
    {
      state = *data;
      XFree (data);
    }

  return state;
}

unsigned long
SetGnomeState (ASWindow * t)
{
  unsigned long state = GetGnomeState (t);

  state &= ~(WIN_STATE_STICKY | WIN_STATE_MINIMIZED |
	     WIN_STATE_MAXIMIZED_HORIZ | WIN_STATE_MAXIMIZED_VERT |
	     WIN_STATE_SHADED);
  if (t->flags & STICKY)
    state |= WIN_STATE_STICKY;
  if (t->flags & ICONIFIED)
    state |= WIN_STATE_MINIMIZED;
  if (t->flags & MAXIMIZED)
    {
      if (t->orig_x != t->frame_x || t->orig_wd != t->frame_width)
	state |= WIN_STATE_MAXIMIZED_HORIZ;
      if (t->orig_y != t->frame_y || t->orig_ht != t->frame_height)
	state |= WIN_STATE_MAXIMIZED_VERT;
    }
/*if (t->flags & WINDOWLISTSKIP) state |= WIN_STATE_HIDDEN; */
  if (t->flags & SHADED)
    state |= WIN_STATE_SHADED;

  XChangeProperty (dpy, t->w, _XA_WIN_STATE, XA_CARDINAL, 32, PropModeReplace,
		   (unsigned char *) &state, 1);

  return state;
}

Bool
is_function_bound_to_button (int button, int function)
{
  MouseButton *func;
  int context;
  if (button & 1)
    context = C_L1 << (button / 2);
  else
    {
      if (button == 0)
	button = 10;
      context = C_R1 << ((button - 2) / 2);
    }
  for (func = Scr.MouseButtonRoot; func != NULL; func = (*func).NextButton)
    if ((func->Context & context) && (func->func == function))
      return True;
  return False;
}

void
disable_titlebuttons_with_function (ASWindow * t, int function)
{
  int i, j;
  for (i = 0; i < 10; i++)
    {
      j = (i != 9) ? (i + 1) : 0;
      if ((Scr.button_style[j] != NO_BUTTON_STYLE) &&
	  !(t->buttons & (BUTTON1 << i)) &&
	  is_function_bound_to_button (j, function))
	t->buttons |= BUTTON1 << i;
    }
}

/****************************************************************************
 * 
 * Interprets the property MOTIF_WM_HINTS, sets decoration and functions
 * accordingly
 *
 *****************************************************************************/
void
SelectDecor (ASWindow * t, unsigned long tflags, int border_width,
	     int resize_width)
{
  int decor;
  MwmHints *prop;

  if (!(tflags & BW_FLAG))
    border_width = Scr.NoBoundaryWidth;

  if (!(tflags & NOBW_FLAG))
    resize_width = Scr.BoundaryWidth;

  decor = MWM_DECOR_ALL;
  t->functions = MWM_FUNC_ALL;
  if (t->mwm_hints)
    {
      prop = (MwmHints *) t->mwm_hints;
      if ((Scr.flags & MWMDecorHints) && (prop->flags & MWM_HINTS_DECORATIONS))
	decor = prop->decorations;

      if ((Scr.flags & MWMFunctionHints) && (prop->flags & MWM_HINTS_FUNCTIONS))
	t->functions = prop->functions;
    }
  /* functions affect the decorations! if the user says
   * no iconify function, then the iconify button doesn't show
   * up. */
  if (t->functions & MWM_FUNC_ALL)
    {
      /* If we get ALL + some other things, that means to use
       * ALL except the other things... */
      t->functions &= ~MWM_FUNC_ALL;
      t->functions = (MWM_FUNC_RESIZE | MWM_FUNC_MOVE | MWM_FUNC_MINIMIZE |
		    MWM_FUNC_MAXIMIZE | MWM_FUNC_CLOSE) & (~(t->functions));
    }

  if ((Scr.flags & MWMFunctionHints) && (t->flags & TRANSIENT))
    t->functions &= ~(MWM_FUNC_MAXIMIZE | MWM_FUNC_MINIMIZE);

  if (decor & MWM_DECOR_ALL)
    {
      /* If we get ALL + some other things, that means to use
       * ALL except the other things... */
      decor &= ~MWM_DECOR_ALL;
      decor = (MWM_DECOR_BORDER | MWM_DECOR_RESIZEH | MWM_DECOR_TITLE |
	       MWM_DECOR_MENU | MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE)
	& (~decor);
    }
  /* Now I have the un-altered decor and functions, but with the
   * ALL attribute cleared and interpreted. I need to modify the
   * decorations that are affected by the functions */
  if (!(t->functions & MWM_FUNC_RESIZE))
    decor &= ~MWM_DECOR_RESIZEH;
  /* MWM_FUNC_MOVE has no impact on decorations. */
  if (!(t->functions & MWM_FUNC_MINIMIZE))
    decor &= ~MWM_DECOR_MINIMIZE;
  if (!(t->functions & MWM_FUNC_MAXIMIZE))
    decor &= ~MWM_DECOR_MAXIMIZE;
  /* MWM_FUNC_CLOSE has no impact on decorations. */

  /* This rule is implicit, but its easier to deal with if
   * I take care of it now */
  if (decor & (MWM_DECOR_MENU | MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE))
    decor |= MWM_DECOR_TITLE;

  /* Selected the mwm-decor field, now trim down, based on configuration
   * files entries */
  if ((tflags & NOTITLE_FLAG)
      || ((!(Scr.flags & DecorateTransients)) && (t->flags & TRANSIENT)))
    decor &= ~MWM_DECOR_TITLE;

  if ((tflags & NOHANDLES_FLAG)
      || ((!(Scr.flags & DecorateTransients)) && (t->flags & TRANSIENT)))
    decor &= ~MWM_DECOR_RESIZEH;

  if ((Scr.flags & MWMDecorHints) && (t->flags & TRANSIENT))
    decor &= ~(MWM_DECOR_MAXIMIZE | MWM_DECOR_MINIMIZE);

#ifdef SHAPE
  if (t->wShaped)
    decor &= ~(BORDER | MWM_DECOR_RESIZEH | FRAME);
#endif
  /* Assume no decorations, and build up */
  t->flags &= ~(BORDER | TITLE | FRAME);
  t->boundary_width = 0;
  t->boundary_height = 0;
  t->corner_width = 0;
  t->title_width = 0;
  t->title_height = 0;
  t->button_height = 0;

  if (decor & MWM_DECOR_TITLE)
    {
      /* a titlebar with no buttons in it */
      t->flags |= TITLE;
    }
  if (decor & MWM_DECOR_RESIZEH)
    {
      /* a wide border, with corner tiles */
#ifndef NO_TEXTURE
      if (DecorateFrames)
	t->flags |= FRAME;
      else
#endif /* !NO_TEXTURE */
	t->flags |= BORDER;
    }
  if (!(decor & MWM_DECOR_MENU))
    disable_titlebuttons_with_function (t, F_POPUP);
  if (!(decor & MWM_DECOR_MINIMIZE))
    disable_titlebuttons_with_function (t, F_ICONIFY);
  if (!(decor & MWM_DECOR_MAXIMIZE))
    disable_titlebuttons_with_function (t, F_MAXIMIZE);

  t->boundary_width = 0;
  t->boundary_height = (t->flags & BORDER) ? Scr.BoundaryWidth : 0;
  t->corner_width = 16 + t->boundary_height;

  if (t->flags & FRAME)
    t->bw = 0;
  else
    t->bw = 1;

  if (tflags & NOHANDLES_FLAG)
    t->bw = border_width;
  if (t->boundary_height == 0)
    t->flags &= ~BORDER;
  t->button_height = t->title_height - 7;
}

/****************************************************************************
 * 
 * Checks the function described in menuItem mi, and sees if it
 * is an allowed function for window Tmp_Win,
 * according to the motif way of life.
 * 
 * This routine is used to determine whether or not to grey out menu items.
 *
 ****************************************************************************/
int
check_allowed_function (MenuItem * mi)
{
  /* Complex functions are a little tricky... ignore them for now */

  if ((Tmp_win) &&
      (!(Tmp_win->flags & DoesWmDeleteWindow)) && (mi->func == F_DELETE))
    return 0;

  /* Move is a funny hint. Keeps it out of the menu, but you're still allowed
   * to move. */
  if ((mi->func == F_MOVE) && (Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_MOVE)))
    return 0;

  if ((mi->func == F_RESIZE) && (Tmp_win) &&
      (!(Tmp_win->functions & MWM_FUNC_RESIZE)))
    return 0;

  if ((mi->func == F_ICONIFY) && (Tmp_win) &&
      (!(Tmp_win->flags & ICONIFIED)) &&
      (!(Tmp_win->functions & MWM_FUNC_MINIMIZE)))
    return 0;

  if ((mi->func == F_MAXIMIZE) && (Tmp_win) &&
      (!(Tmp_win->functions & MWM_FUNC_MAXIMIZE)))
    return 0;

  if ((mi->func == F_SHADE) && (Tmp_win) &&
   (!(Tmp_win->functions & MWM_FUNC_MAXIMIZE) || !(Tmp_win->flags & TITLE)))
    return 0;

  if ((mi->func == F_DELETE) && (Tmp_win) &&
      (!(Tmp_win->functions & MWM_FUNC_CLOSE)))
    return 0;

  if ((mi->func == F_CLOSE) && (Tmp_win) &&
      (!(Tmp_win->functions & MWM_FUNC_CLOSE)))
    return 0;

  if ((mi->func == F_DESTROY) && (Tmp_win) &&
      (!(Tmp_win->functions & MWM_FUNC_CLOSE)))
    return 0;

  if (mi->func == F_NOP)
    return 0;

  if (mi->func == F_FUNCTION && mi->item != NULL)
    {
      /* Hard part! What to do now? */
      /* Hate to do it, but for lack of a better idea,
       * check based on the menu entry name */
      if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_MOVE)) &&
	  (mystrncasecmp (mi->item, MOVE_STRING, strlen (MOVE_STRING)) == 0))
	return 0;

      if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_RESIZE)) &&
	  (mystrncasecmp (mi->item, RESIZE_STRING1, strlen (RESIZE_STRING1)) == 0))
	return 0;

      if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_RESIZE)) &&
	  (mystrncasecmp (mi->item, RESIZE_STRING2, strlen (RESIZE_STRING2)) == 0))
	return 0;

      if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_MINIMIZE)) &&
	  (!(Tmp_win->flags & ICONIFIED)) &&
	  (mystrncasecmp (mi->item, MINIMIZE_STRING, strlen (MINIMIZE_STRING)) == 0))
	return 0;

      if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_MINIMIZE)) &&
	  (mystrncasecmp (mi->item, MINIMIZE_STRING2, strlen (MINIMIZE_STRING2)) == 0))
	return 0;

      if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_MAXIMIZE)) &&
	  (mystrncasecmp (mi->item, MAXIMIZE_STRING, strlen (MAXIMIZE_STRING)) == 0))
	return 0;

      if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_CLOSE)) &&
      (mystrncasecmp (mi->item, CLOSE_STRING1, strlen (CLOSE_STRING1)) == 0))
	return 0;

      if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_CLOSE)) &&
      (mystrncasecmp (mi->item, CLOSE_STRING2, strlen (CLOSE_STRING2)) == 0))
	return 0;

      if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_CLOSE)) &&
      (mystrncasecmp (mi->item, CLOSE_STRING3, strlen (CLOSE_STRING3)) == 0))
	return 0;

      if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_CLOSE)) &&
      (mystrncasecmp (mi->item, CLOSE_STRING4, strlen (CLOSE_STRING4)) == 0))
	return 0;

    }
  return 1;
}


/****************************************************************************
 * 
 * Checks the function "function", and sees if it
 * is an allowed function for window t,  according to the motif way of life.
 * This routine is used to decide if we should refuse to perform a function.
 *
 ****************************************************************************/
int
check_allowed_function2 (int function, ASWindow * t)
{

  if (Scr.flags & MWMHintOverride)
    return 1;

  if ((t) && (!(t->flags & DoesWmDeleteWindow)) && (function == F_DELETE))
    return 0;

  if ((function == F_RESIZE) && (t) &&
      (!(t->functions & MWM_FUNC_RESIZE)))
    return 0;

  if ((function == F_ICONIFY) && (t) &&
      (!(t->flags & ICONIFIED)) &&
      (!(t->functions & MWM_FUNC_MINIMIZE)))
    return 0;

  if ((function == F_MAXIMIZE) && (t) &&
      (!(t->functions & MWM_FUNC_MAXIMIZE)))
    return 0;

  if ((function == F_SHADE) && (t) &&
      (!(t->functions & MWM_FUNC_MAXIMIZE) || !(t->flags & TITLE)))
    return 0;

  if ((function == F_DELETE) && (t) &&
      (!(t->functions & MWM_FUNC_CLOSE)))
    return 0;

  if ((function == F_DESTROY) && (t) &&
      (!(t->functions & MWM_FUNC_CLOSE)))
    return 0;

  return 1;
}

#ifndef NO_TEXTURE

/***************************************************
 * 
 * Parse and fill color configuration
 * 
 * Returns 1 for success, 0 for syntax error
 ***************************************************/
int
ParseColor (const char *input, int from[3], int to[3])
{
  char *sfrom, *sto;
  XColor color1, color2;

  if (!input)
    return 0;

  sfrom = (char *) safemalloc (strlen (input));
  sto = (char *) safemalloc (strlen (input));
  sscanf (input, "%s %s", sfrom, sto);

  if ((!XParseColor (dpy, Scr.asv->colormap, sfrom, &color1)) ||
      (!XParseColor (dpy, Scr.asv->colormap, sto, &color2)))
    {
      free (sfrom);
      free (sto);
      return (0);
    }
  from[0] = color1.red;
  from[1] = color1.green;
  from[2] = color1.blue;
  to[0] = color2.red;
  to[1] = color2.green;
  to[2] = color2.blue;

  free (sfrom);
  free (sto);

  return (1);
}


/**********************************************************************
 * 
 * Initializes information for background texture rendering.
 * Set's colors for gradient if defined or default if not.
 * Set's the default number of colors to be used too.
 * 
 **********************************************************************/

#define XCOL(v)		((v)*(65535/256))
void
InitTextureData (TextureInfo * info, char *title, char *utitle,
	  char *mtitle, char *item, char *mhilite, char *sticky, char *text)
{
  if (!title)
    {
      info->Tfrom[0] = XCOL (0x10);
      info->Tfrom[1] = XCOL (0x10);
      info->Tfrom[2] = XCOL (0x30);
      info->Tto[0] = XCOL (0x30);
      info->Tto[1] = XCOL (0x30);
      info->Tto[2] = XCOL (0x80);
    }
  else if (!ParseColor (title, info->Tfrom, info->Tto))
    {
      afterstep_err ("Invalid TitleTextureColor %s\n", title, NULL, NULL);
    }
  if (!item)
    {
      info->Ifrom[0] = XCOL (0x86);
      info->Ifrom[1] = XCOL (0x86);
      info->Ifrom[2] = XCOL (0x8a);
      info->Ito[0] = XCOL (0xc0);
      info->Ito[1] = XCOL (0xb8);
      info->Ito[2] = XCOL (0xc3);
/*      
   info->Ifrom[0] = XCOL(0xd0);
   info->Ifrom[1] = XCOL(0xd0);
   info->Ifrom[2] = XCOL(0xd0);
   info->Ito[0] = XCOL(0x60);
   info->Ito[1] = XCOL(0x60);
   info->Ito[2] = XCOL(0x60);
 */
    }
  else if (!ParseColor (item, info->Ifrom, info->Ito))
    {
      afterstep_err ("Invalid MenuTextureColor %s\n", item, NULL, NULL);
    }
  if (!mhilite)
    {
      info->Hfrom[0] = XCOL (0xc0);
      info->Hfrom[1] = XCOL (0xb8);
      info->Hfrom[2] = XCOL (0xc3);
      info->Hto[0] = XCOL (0x86);
      info->Hto[1] = XCOL (0x86);
      info->Hto[2] = XCOL (0x8a);
    }
  else if (!ParseColor (mhilite, info->Hfrom, info->Hto))
    {
      afterstep_err ("Invalid MenuHiTextureColor %s\n", mhilite, NULL, NULL);
    }
  if (!mtitle)
    {
      info->Mfrom[0] = XCOL (0x10);
      info->Mfrom[1] = XCOL (0x10);
      info->Mfrom[2] = XCOL (0x30);
      info->Mto[0] = XCOL (0x30);
      info->Mto[1] = XCOL (0x30);
      info->Mto[2] = XCOL (0x80);
    }
  else if (!ParseColor (mtitle, info->Mfrom, info->Mto))
    {
      if (mtitle)
	{
	  afterstep_err ("Invalid MTitleTextureColor %s\n", mtitle, NULL, NULL);
	}
    }
  if (!utitle)
    {
      info->Ufrom[0] = XCOL (0x86);
      info->Ufrom[1] = XCOL (0x86);
      info->Ufrom[2] = XCOL (0x8a);
      info->Uto[0] = XCOL (0xc0);
      info->Uto[1] = XCOL (0xb8);
      info->Uto[2] = XCOL (0xc3);
    }
  else if (!ParseColor (utitle, info->Ufrom, info->Uto))
    {
      if (utitle)
	{
	  afterstep_err ("Invalid UTitleTextureColor %s\n", utitle, NULL, NULL);
	}
    }
  if (!sticky)
    {
      info->Sfrom[0] = XCOL (0x30);
      info->Sfrom[1] = XCOL (0x10);
      info->Sfrom[2] = XCOL (0x10);
      info->Sto[0] = XCOL (0x90);
      info->Sto[1] = XCOL (0x40);
      info->Sto[2] = XCOL (0x40);
    }
  else if (!ParseColor (sticky, info->Sfrom, info->Sto))
    {
      afterstep_err ("Invalid STitleTextureColor %s\n", sticky, NULL, NULL);
    }
  if (!text)
    {
      info->TGfrom[0] = XCOL (0xff);
      info->TGfrom[1] = XCOL (0xff);
      info->TGfrom[2] = XCOL (0xff);
      info->TGto[0] = XCOL (0x40);
      info->TGto[1] = XCOL (0x40);
      info->TGto[2] = XCOL (0x40);
    }
  else if (!ParseColor (text, info->TGfrom, info->TGto))
    {
      afterstep_err ("Invalid TextGradientColor %s\n", text, NULL, NULL);
    }
}
#undef XCOL

#endif /* ! N0_TEXTURE */
