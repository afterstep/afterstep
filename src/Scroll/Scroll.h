/* include files shared by many AS modules */
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "../../include/module.h"

/* other include files */

/* global variables used in both Scoll.c & GrabWindow.c */
char *MyName;
int fd_width;
Display *dpy;                   /* which display are we talking to */
Window Root;
int screen;
int x_fd;
int d_depth;

/* from Scroll.c */
void DeadPipe(int nonsense);
void GetTargetWindow(Window *app_win);
Window ClientWindow(Window input);

/* from GrabWindow.c */
void RelieveWindow(Window win,int x,int y,int w,int h, GC rgc,GC sgc);
void CreateWindow(int x, int y,int w, int h);
Pixel GetShadow(Pixel background);
Pixel GetHilite(Pixel background);
Pixel GetColor(char *name);
void Loop(Window target);
void RedrawWindow(Window target);
void change_window_name(char *str);
void send_clientmessage (Window w, Atom a, Time timestamp);
void GrabWindow(Window target);
void change_icon_name(char *str);
void RedrawLeftButton(GC rgc, GC sgc,int x1,int y1);
void RedrawRightButton(GC rgc, GC sgc,int x1,int y1);
void RedrawTopButton(GC rgc, GC sgc,int x1,int y1);
void RedrawBottomButton(GC rgc, GC sgc,int x1,int y1);
