/*************************************************************************
 *
 * Subroutine Prototypes
 * 
 *************************************************************************/

extern void   CreateWindow(void);
extern Pixel  GetColor(char *name);
extern Pixel  GetHilite(Pixel background);
extern Pixel  GetShadow(Pixel background);
extern void   nocolor(char *a, char *b);
extern void   RedrawWindow(int);
extern void   match_string(char *tline);
extern void   DispatchEvent(XEvent*);
extern void   ParseOptions(const char *);
extern void   ParseBaseOptions(const char *);
extern void   change_window_name(char *str);
extern AFTER_INLINE void RelieveWindow(Window win,int x,int y,int w,int h,GC rGC,GC sGC);
extern void   DeadPipe(int nonsense);
extern void   LoadIconFile(int button);
extern void   CreateIconWindow(int button);
extern void   ConfigureIconWindow(int button,int row, int column);
extern void   DrawIconWindow(int button);
extern void   GetBitmapFile(int button);
extern void   GetImageFile(int button);
void process_message(unsigned long type,unsigned long *body);
void send_clientmessage (Window w, Atom a, Time timestamp);
void swallow(unsigned long *body);
void ConstrainSize (XSizeHints *hints, int *widthp, int *height);

extern Display *dpy;			/* which display are we talking to */
extern Window main_win;
extern int screen;
extern Pixel back_pix, fore_pix;
extern GC  NormalGC;
extern GC  ReliefGC;
extern int ButtonWidth,ButtonHeight;
extern MyFont font;

#undef  MAX_BUTTONS
#define MAX_BUTTONS 100

struct button_info
{
  char *action;
  char *title;
  char *icon_file;
  int BWidth;                   /* Width of this button in "buttons" */
  int BHeight;
  int icon_w;
  int icon_h;
  Pixmap iconPixmap;		/* pixmap for the icon */
  Pixmap icon_maskPixmap;	/* pixmap for the icon mask */
  Window IconWin;
  XSizeHints hints;
  int icon_depth;
  char *hangon;
  char up;
  char swallow;
  char module;
};

extern struct button_info Buttons[MAX_BUTTONS];

extern char *iconPath;
extern char *pixmapPath;

