/*
 * Copyright (c) 1999 Rafal Wierzbicki <rafal@mcss.mcmaster.ca>
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (c) 1994 Mike Finger <mfinger@mermaid.micro.umn.edu>
 *                    or <Mike_Finger@atk.com>
 * Copyright (c) 1994 Robert Nation
 * Copyright (c) 1994 Nobutaka Suzuki
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
 * $Id: WinList.h,v 1.3 2002/05/01 06:15:00 sashav Exp $
 */

#include "../../configure.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/time.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif
#ifdef ISC			/* Saul */
#include <sys/bsdtypes.h>	/* Saul */
#endif /* Saul */
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#ifdef I18N
#include <X11/Xlocale.h>
#endif

#define IN_MODULE
#define MODULE_X_INTERFACE

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/parse.h"
#include "../../include/loadimg.h"
#include "ButtonArray.h"
#include "List.h"

/* Motif  window hints */
typedef struct
{
    CARD32      flags;
    CARD32      functions;
    CARD32      decorations;
    INT32       inputMode;
} PropMotifWmHints;

typedef PropMotifWmHints        PropMwmHints;

/* Motif window hints */
#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)       

/* values for MwmHints.input_mode */
#define MWM_INPUT_MODELESS                      0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL     1
#define MWM_INPUT_SYSTEM_MODAL                  2
#define MWM_INPUT_FULL_APPLICATION_MODAL        3         

/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL                 (1L << 0)
#define MWM_DECOR_BORDER              (1L << 1)
#define MWM_DECOR_RESIZEH             (1L << 2)
#define MWM_DECOR_TITLE               (1L << 3)
#define MWM_DECOR_MENU                (1L << 4)
#define MWM_DECOR_MINIMIZE            (1L << 5)
#define MWM_DECOR_MAXIMIZE            (1L << 6)

#define PROP_MOTIF_WM_HINTS_ELEMENTS  4
#define PROP_MWM_HINTS_ELEMENTS       PROP_MOTIF_WM_HINTS_ELEMENTS

/* masks for AS pipe */
#define  mask_reg (M_ADD_WINDOW | M_CONFIGURE_WINDOW | M_DESTROY_WINDOW | \
		M_ICONIFY | M_DEICONIFY | M_WINDOW_NAME | M_ICON_NAME | \
		M_END_WINDOWLIST)
#define mask_hide (M_ADD_WINDOW | M_DESTROY_WINDOW | M_ICONIFY | M_DEICONIFY | \
                               M_WINDOW_NAME | M_ICON_NAME)
#define mask_off  0

/* global variables */
extern Window window; /* app window */
extern int w_height;              /* window height */
extern int w_width;               /* window width  */
extern int Balloons;               /* flag for running with balloons enabled */
extern Display *dpy;                   /* display, needed by AS libs */
extern int screen;	         /* for balloons and mystyles*/
extern ScreenInfo Scr;                   /* root window */
extern char *MyName;     /* module name, needed by AS libs */
extern ButtonArray buttons;        /* array of buttons */
extern List windows;                    /* list of same */

/* macros */
#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#define max(a,b) (((a)>(b)) ? (a) : (b))
#endif
#define LOG(str) fprintf (stderr, "LOG: %s\n", (str));

/* other useful stuff */
#define GRAB_EVENTS (ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|EnterWindowMask|LeaveWindowMask)
#define SomeButtonDown(a) ((a)&Button1Mask||(a)&Button2Mask||(a)&Button3Mask)

/*************************************************************************
  Subroutine Prototypes
**************************************************************************/
void DeadPipe(int nonsense);
void MakeMeWindow(void);
void WaitForExpose(void);
void RedrawWindow(int force);
void StartMeUp(void);
void ShutMeDown(int exitstat);
void ConsoleMessage(char *fmt,...);
int OpenConsole(void);
void ParseOptions(const char *file);
void ParseBaseOptions(const char *file);
void DispatchEvent (XEvent *event);
int AdjustWindow(void);
char *makename(char *string,long flags);
void ChangeWindowName(char *str);
void LinkAction(char *string);


void SetMwmHints(unsigned int value,unsigned int funcs,unsigned int input);

void unhide_winlist ();
int error_handler (Display *disp, XErrorEvent *event);
void hide_winlist  (Boolean force);
void update_winlist_background  ();
void update_hidden_winlist ();
void update_look ();
void send_as_mesg (const char *message, const unsigned long id);
void set_as_mask (long unsigned mask);
void process_message (unsigned long type, unsigned long *body);
void get_geometry (int *gravity, int *posx, int *posy);
void wm_size_hints  (int gravity, int x, int y);
int list_configure (unsigned long *body);
int list_add_window (unsigned long *body);
int list_window_name  (unsigned long *body);
int list_icon_name (unsigned long *body);
int list_destroy_window (unsigned long *body);
int list_end ();
int list_iconify (unsigned long *body);
int list_deiconify (unsigned long *body);
void hide_window ();

