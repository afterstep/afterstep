#include "../../configure.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"

extern Display *dpy;
extern ScreenInfo Scr;

Window GnomeProxyWin = None;

void
set_gnome_proxy ()
{
  Atom atom_set;
  CARD32 val;

  atom_set = XInternAtom (dpy, "_WIN_DESKTOP_BUTTON_PROXY", False);
  GnomeProxyWin = XCreateSimpleWindow (dpy, Scr.Root, 0, 0, 5, 5, 0, 0, 0);
  if( GnomeProxyWin )
  {
	val = GnomeProxyWin;
    XChangeProperty (dpy, Scr.Root, atom_set, XA_CARDINAL, 32,
  		   PropModeReplace, (unsigned char *) &val, 1);
	XChangeProperty (dpy, GnomeProxyWin, atom_set, XA_CARDINAL, 32,
		   PropModeReplace, (unsigned char *) &val, 1);
  }
}
