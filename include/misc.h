#ifndef _MISC_
#define _MISC_

#ifndef max
#define max(x,y)  ((x>=y)?x:y)
#endif

#ifndef min
#define min(x,y)  ((x<=y)?x:y)
#endif

#define SORTBYALPHA 1
#define SORTBYDATE  2

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
extern MyFont *IconFont;
extern Time lastTimestamp;
extern XEvent Event;
extern char NoName[];

void set_titlebar_geometry (ASWindow * t);
void get_client_geometry (ASWindow * t, int frame_x, int frame_y, int frame_width, int frame_height, int *client_x_out, int *client_y_out, int *client_width_out, int *client_height_out);
void get_parent_geometry (ASWindow * t, int frame_width, int frame_height, int *parent_x_out, int *parent_y_out, int *parent_width_out, int *parent_height_out);
void get_frame_geometry (ASWindow * t, int client_x, int client_y, int client_width, int client_height, int *frame_x_out, int *frame_y_out, int *frame_width_out, int *frame_height_out);
void get_resize_geometry (ASWindow * t, int client_x, int client_y, int client_width, int client_height, int *frame_x_out, int *frame_y_out, int *frame_width_out, int *frame_height_out);

extern void MoveOutline ( /* Window, */ ASWindow *, int, int, int, int);
extern void draw_fvwm_outline (int, int, int, int);
extern void draw_box_outline (int, int, int, int);
extern void draw_barndoor_outline (int, int, int, int);
extern void draw_wmaker_outline (int, int, int, int, ASWindow *);
extern void draw_tek_outline (int, int, int, int);
extern void draw_tek2_outline (int, int, int, int);
extern void draw_corners_outline (int, int, int, int);
extern void draw_hash_outline (int, int, int, int);
extern void draw_grid_outline (int, int, int, int);

extern void DoResize (int, int, ASWindow *, Bool);
extern void DisplaySize (ASWindow *, int, int, Bool);
extern void DisplayPosition (ASWindow *, int, int, Bool);
extern void SetupFrame (ASWindow *, int, int, int, int, Bool);
extern void CreateGCs (void);
extern void InstallWindowColormaps (ASWindow *);
extern void InstallRootColormap (void);
extern void UninstallRootColormap (void);
extern void FetchWmProtocols (ASWindow *);
extern void FetchWmColormapWindows (ASWindow *);
extern void InitBase (Bool);
extern void InitLook (Bool);
extern void InitFeel (Bool);
extern void InitDatabase (Bool);
extern void LoadASConfig (const char *, int, Bool, Bool, Bool);
extern void InitEvents (void);
extern void HandleEvents (void);
extern void HandleFocusIn (void);
extern void HandleFocusOut (void);
extern void HandleDestroyNotify (void);
extern void HandleMapRequest (void);
extern void HandleMapNotify (void);
extern void HandleUnmapNotify (void);
extern void HandleButtonPress (void);
extern void HandleEnterNotify (void);
extern void HandleLeaveNotify (void);
extern void HandleConfigureRequest (void);
extern void HandleClientMessage (void);
extern void HandlePropertyNotify (void);
extern void HandleKeyPress (void);
extern void HandleVisibilityNotify (void);
extern void HandleColormapNotify (void);
extern int SetTransparency (ASWindow *);
extern void SetTitleBar (ASWindow *, Bool, Bool);
extern void RestoreWithdrawnLocation (ASWindow *, Bool);
extern void Destroy (ASWindow *, Bool);
extern void GetGravityOffsets (ASWindow *, int *, int *);
extern void MoveViewport (int, int, Bool);
extern void init_titlebar_windows (ASWindow *, Bool);
extern Bool create_titlebar_windows (ASWindow *);
extern void init_titlebutton_windows (ASWindow *, Bool);
extern Bool create_titlebutton_windows (ASWindow *);
extern ASWindow *AddWindow (Window);
extern int MappedNotOverride (Window);
extern void GrabButtons (ASWindow *);
extern void GrabKeys (ASWindow *);
extern void GetWindowSizeHints (ASWindow *);
extern void ReallyRedrawPager (void);
extern void SwitchPages (Bool, Bool);
extern void NextPage (void);
extern void PrevPage (void);
extern void moveLoop (ASWindow *, int, int, int, int, int *, int *, Bool, Bool);

extern void Keyboard_shortcuts (XEvent *, int, int);

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

extern AFTER_INLINE void RelieveWindow (ASWindow *, Window, int, int, int, int,
					GC, GC, int);

void RelieveParts (ASWindow *, int, GC, GC);
#define NO_HILITE     0x0000
#define TOP_HILITE    0x0001
#define RIGHT_HILITE  0x0002
#define BOTTOM_HILITE 0x0004
#define LEFT_HILITE   0x0008
#define EXTRA_HILITE  0x0010
#define FULL_HILITE   0x001F

extern void PagerMoveWindow (void);
extern void Stick (ASWindow *);
extern void Maximize (ASWindow *, int, int, int, int);
extern void Shade (ASWindow *);
extern void ResetShade (ASWindow *);
extern void RaiseWindow (ASWindow *);
extern void LowerWindow (ASWindow *);
extern Bool GrabEm (int);
extern void UngrabEm (void);
extern void CaptureAllWindows (void);
extern void SetTimer (int);
extern int flush_expose (Window);
extern void do_windowList (int, int);
extern void RaiseThisWindow (int);
extern int GetContext (ASWindow *, XEvent *, Window *);
extern void HandlePaging (ASWindow *, int, int, int *, int *, int *, int *, Bool);
extern void SetShape (ASWindow *, int);
extern void afterstep_err (const char *, const char *, const char *, const char *);
extern void executeModule (char *, FILE *, char **, int *);
extern Bool SetFocus (Window, ASWindow *, Bool);
extern void CheckAndSetFocus (void);
extern void nofont (char *name);
extern void match_string (struct config *, char *, char *, FILE *);
extern struct config *match_string2 (struct config *, char *);
extern void no_popup (char *);
extern void KillModule (int, int);
extern void KillModuleByName (char *);
extern void ClosePipes (void);

/* this is afterstep specific stuff - we don't want this in modules */
#ifndef IN_MODULE
extern char *fit_horizontal_text (MyFont, char *, int, int);
extern char *fit_vertical_text (MyFont, char *, int, int);
extern void ConstrainSize (ASWindow *, int *, int *);
extern void DispatchEvent (void);
extern void HandleExpose (void);
int AS_XNextEvent (Display *, XEvent *);
#endif /* IN_MODULE */

/*extern void GetXPMFile (ASWindow *); */
extern void init_old_look_variables (Bool);
extern int ParseColor (const char *, int[3], int[3]);
extern void merge_old_look_colors (MyStyle *, int, int, char *, char *, char *, char *);
extern void merge_old_look_variables (void);
extern void get_window_geometry (ASWindow * t, int flags, int *x, int *y, int *w, int *h);
extern int SmartPlacement (ASWindow *, int *, int *, int, int, int, int, int, int, int);
extern void usage (void);

void Broadcast (unsigned long, unsigned long,...);
void BroadcastConfig (unsigned long, ASWindow *);
void SendPacket (int, unsigned long, unsigned long,...);
void SendConfig (int, unsigned long, ASWindow *);
void BroadcastName (unsigned long, unsigned long, unsigned long, unsigned long,
		    char *);
void SendName (int, unsigned long, unsigned long, unsigned long, unsigned long,
	       char *);
void DeadPipe (int);
unsigned long GetGnomeState (ASWindow * t);
unsigned long SetGnomeState (ASWindow * t);
void GetMwmHints (ASWindow *);
void SelectDecor (ASWindow *, unsigned long, int, int);
void ComplexFunction (Window, ASWindow *, XEvent *, unsigned long, struct MenuRoot *);
extern int DeferExecution (XEvent *, Window *, ASWindow **, unsigned long *, int, int);
void send_clientmessage (Window, Atom, Time);
void SetBackgroundTexture (ASWindow * t, Window win, MyStyle * style, Pixmap cache);
int SetBackground (ASWindow * t, Window win);
void SetBorder (ASWindow *, Bool, Bool, Bool, Window);
void move_window (XEvent *, Window, ASWindow *, int, int, int, int, int);
void resize_window (Window, ASWindow *, int, int, int, int);
void SetMapStateProp (ASWindow *, int);
void SetStickyProp (ASWindow *, int, int, int);
void SetClientProp (ASWindow *);
void KeepOnTop (void);
void show_panner (void);
void WaitForButtonsUp (void);
Bool PlaceWindow (ASWindow *, unsigned long, int);
void free_window_names (ASWindow *, Bool, Bool);

int check_allowed_function (struct MenuItem *);
int check_allowed_function2 (int, ASWindow *);
void ReInstallActiveColormap (void);
void ParsePopupEntry (char *, FILE *, char **, int *);
void ParseMouseEntry (char *, FILE *, char **, int *);
void ParseKeyEntry (char *, FILE *, char **, int *);
void SetOneStyle (char *text, FILE *, char **, int *);

void ButtonStyle (void);
void IconStyle (void);
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
Bool GetIconXPM (char *, MyIcon *, int);
Bool GetIconJPG (char *, MyIcon *, int);
Bool GetIconFromFile (char *, MyIcon *, int);
Pixmap GetXPMTile (char *, int);

ASWindow *GetNextWindow (const ASWindow *, const int);
extern ASWindow *Circulate (ASWindow *, char *, Bool);
void PasteSelection (void);
void changeDesks (int, int);
void changeWindowsDesk (ASWindow *, int);
void aswindow_set_desk_property (ASWindow * t, int new_desk);
void MapIt (ASWindow *);
void UnmapIt (ASWindow *);
void do_save (void);
void checkPanFrames (void);
void raisePanFrames (void);
void initPanFrames (void);
Bool StashEventTime (XEvent *);
void SetCirculateSequence (ASWindow * tw, int dir);
void MyXGrabButton (Display *, unsigned, unsigned, Window, Bool, unsigned,
		    int, int, Window, Cursor);
void MyXUngrabButton (Display *, unsigned, unsigned, Window);
void MyXGrabKey (Display *, int, unsigned, Window, Bool, int, int);
void GrabRaiseClick (ASWindow *);
void UngrabRaiseClick (ASWindow *);
void UpdateVisibility (void);
void CorrectStackOrder (void);
void FlushQueue (int module);
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
