#ifndef _MISC_
#define _MISC_

/************************************************************************
 * ReapChildren - wait() for all dead child processes
 ************************************************************************/

#include <sys/wait.h>

#ifdef HAVE_SYS_WAIT_H
#define ReapChildren()  while ((waitpid(-1, NULL, WNOHANG)) > 0);
#elif defined (HAVE_WAIT3)
#define ReapChildren()  while ((wait3(NULL, WNOHANG, NULL)) > 0);
#endif

/* some fancy font handling stuff */
#define NewFontAndColor(newfont,color,backcolor) {\
   Globalgcv.font = newfont;\
   Globalgcv.foreground = color;\
   Globalgcv.background = backcolor;\
   Globalgcm = GCFont | GCForeground | GCBackground; \
   XChangeGC(dpy,Scr.FontGC,Globalgcm,&Globalgcv); \
}

#ifdef NO_ICONS
#define ICON_HEIGHT 1
#else
#define ICON_HEIGHT (IconFont->height+6)
#endif

struct MenuRoot;
struct MenuItem;

extern XGCValues Globalgcv;
extern unsigned long Globalgcm;

void set_titlebar_geometry (ASWindow * t);
void get_client_geometry (ASWindow * t, int frame_x, int frame_y, int frame_width, int frame_height, int *client_x_out, int *client_y_out, int *client_width_out, int *client_height_out);
void get_parent_geometry (ASWindow * t, int frame_width, int frame_height, int *parent_x_out, int *parent_y_out, int *parent_width_out, int *parent_height_out);
void get_frame_geometry (ASWindow * t, int client_x, int client_y, int client_width, int client_height, int *frame_x_out, int *frame_y_out, int *frame_width_out, int *frame_height_out);
void get_resize_geometry (ASWindow * t, int client_x, int client_y, int client_width, int client_height, int *frame_x_out, int *frame_y_out, int *frame_width_out, int *frame_height_out);

extern void DoResize (int, int, ASWindow *, Bool);
extern void DisplaySize (ASWindow *, int, int, Bool);
extern void DisplayPosition (ASWindow *, int, int, Bool);
extern void moveLoop (ASWindow *, int, int, int, int, int *, int *, Bool, Bool);
extern void Keyboard_shortcuts (XEvent *, int, int);

//extern void SetupFrame (ASWindow *, int, int, int, int, Bool);
void CreateGCs (void);

extern void init_old_look_variables (Bool);
extern void merge_old_look_colors (MyStyle *, int, int, char *, char *, char *, char *);
extern void merge_old_look_variables (void);
extern void InitBase (Bool);
extern void InitLook (Bool);
extern void InitFeel (Bool);
extern void InitDatabase (Bool);
extern void match_string (struct config *, char *, char *, FILE *);
extern struct config *match_string2 (struct config *, char *);
extern void LoadASConfig (const char *, int, Bool, Bool, Bool);

void quietly_unmap_window( Window w, long event_mask );
void quietly_reparent_window( Window w, Window new_parent, int x, int y, long event_mask );
inline void ungrab_window_buttons( Window w );
inline void ungrab_window_keys (Window w );
void MyXGrabButton ( unsigned button, unsigned modifiers,
                Window grab_window, Bool owner_events, unsigned event_mask,
                int pointer_mode, int keyboard_mode, Window confine_to, Cursor cursor);
void MyXUngrabButton ( unsigned button, unsigned modifiers, Window grab_window);
void grab_window_buttons (Window w, ASFlagType context_mask);
void grab_window_keys (Window w, ASFlagType context_mask);
void grab_focus_click( Window w );
void ungrab_focus_click( Window w );

/* from icons.c */
void AutoPlace (ASWindow * t);
void AutoPlaceStickyIcons (void);
void ChangeIcon (ASWindow * win);
void DeIconify (ASWindow * tmp_win);
void DrawIconWindow (ASWindow * Tmp_win);
void GetIcon (ASWindow * tmp_win);
void GrabIconButtons (ASWindow * tmp_win, Window w);
void GrabIconKeys (ASWindow * tmp_win, Window w);
void Iconify (ASWindow * tmp_win);
void RedoIconName (ASWindow * Tmp_win);
char *SearchIcon (ASWindow * tmp_win);
void UpdateIconShape (ASWindow * tmp_win);

extern void Stick (ASWindow *);
extern void Maximize (ASWindow *, int, int, int, int);
extern void Shade (ASWindow *);
extern void RaiseWindow (ASWindow *);
extern void LowerWindow (ASWindow *);
extern void CaptureAllWindows (void);
extern void SetTimer (int);
extern void do_windowList (int, int);
extern void afterstep_err (const char *, const char *, const char *, const char *);

/* this is afterstep specific stuff - we don't want this in modules */
#ifndef IN_MODULE
extern char *fit_horizontal_text (MyFont, char *, int, int);
extern char *fit_vertical_text (MyFont, char *, int, int);
void DispatchEvent  ( ASEvent *event );
extern void HandleExpose (ASEvent *event);
int AS_XNextEvent (Display *, XEvent *);
#endif /* IN_MODULE */

extern void usage (void);

extern void executeModule (char *, FILE *, char **, int *);
extern void KillModule (int, int);
extern void KillModuleByName (char *);
extern void ClosePipes (void);
void FlushQueue (int module);

void DeadPipe (int);
unsigned long GetGnomeState (ASWindow * t);
unsigned long SetGnomeState (ASWindow * t);
/*void ComplexFunction (FunctionData *fdata, ASEvent *, unsigned long, struct MenuRoot *);*/
extern int DeferExecution (ASEvent *event, int cursor, int fin_event);
void move_window (XEvent *, Window, ASWindow *, int, int, int, int, int);
void resize_window (Window, ASWindow *, int, int, int, int);
void SetMapStateProp (ASWindow *, int);
void WaitForButtonsUp (void);

int check_allowed_function (struct MenuItem *);
int check_allowed_function2 (int, ASWindow *);
void ReInstallActiveColormap (void);
void ParsePopupEntry (char *, FILE *, char **, int *);
void ParseMouseEntry (char *, FILE *, char **, int *);
void ParseKeyEntry (char *, FILE *, char **, int *);

void SetTitleButton (char *, FILE *, char **, int *);
void SetTitleText (char *, FILE *, char **, int *);
void SetFlag (char *, FILE *, char **, int *);
void SetFlag2 (char *, FILE *, char **, int *);
void SetCursor (char *, FILE *, char **, int *);
void SetChangeCursor (char *, FILE *, char **, int *);
void SetInts (char *, FILE *, char **, int *);
void SetButtonList (char *, FILE *, char **, int *);
void SetBox (char *, FILE *, char **, int *);
void ReadPipeConfig (char *, FILE *, char **, int *);
void set_func (char *, FILE *, char **, int *);
void copy_config (FILE **);
Bool GetIconFromFile (char *, MyIcon *, int);

ASWindow *GetNextWindow (const ASWindow *, const int);
extern ASWindow *Circulate (ASWindow *, char *, Bool);
void aswindow_set_desk_property (ASWindow * t, int new_desk);
void MapIt (ASWindow *);
void UnmapIt (ASWindow *);
void do_save (void);
void SetCirculateSequence (ASWindow * tw, int dir);
void GrabRaiseClick (ASWindow *);
void UngrabRaiseClick (ASWindow *);
void UpdateVisibility (void);
void CorrectStackOrder (void);
void QuickRestart (char *);

/* function to set the gnome proxy click window */
void set_gnome_proxy ();

void InteractiveMove (Window *, ASWindow *, int *, int *, XEvent *);

#ifndef IN_MODULE
#include "frame.h"
#endif /* !IN_MODULE */

#ifdef BROKEN_SUN_HEADERS
#include "sun_headers.h"
#endif

#if defined(__alpha)
#include "alpha_header.h"
#endif /* NEEDS_ALPHA_HEADER */

#endif /* _MISC_ */
