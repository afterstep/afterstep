#undef StickyIcons

/*#define DEBUG_PAGER */
/*#define DEBUG_X_PAGER */
/*#define DEBUG_BACKGROUNDS*/


typedef struct pager_window 
{
  Window w;
  Window frame;
  int x;
  int y;
  int width;
  int height;
  int desk;
  int frame_x;
  int frame_y;
  int frame_width;
  int frame_height;
  int title_height;
  int border_width;
  int icon_x;
  int icon_y;
  int icon_width;
  int icon_height;
  unsigned long flags;
  Window icon_w;
  Window icon_pixmap_w;
  char *icon_name;
  Window PagerView;        /* window to be shown in Pager desk window */
  Window IconView;         /* window to be shown in iconized Pager */

  struct pager_window *next;
} PagerWindow;

typedef struct pager_view_position
{
  int normal_x, normal_y, normal_width, normal_height ;
  int icon_x, icon_y, icon_width, icon_height ;
}PagerViewPosition ;

typedef struct desk_info 
{
  Window w;
  Window title_w;
  
  Bool exposed ;
  
  char *label;
  char *StyleName ;

} DeskInfo;

typedef struct pager_info
{
  Window Pager_w;

  unsigned short Flags ;

  DeskInfo *Desks;
  Window SelectionWin[4];
  PagerWindow *Start;
  PagerWindow *FocusWin;

  int desk1, desk2, ndesks;
  int Rows, Columns;
  int PageRows, PageColumns ;
  int xSize, ySize ; /* x and y size of desktop */
  int AspectX, AspectY;
  
  int bStarted ; 
  
  Pixmap CurentRootBack ;
  ASDeskBackArray Backgrounds ;

} PagerInfo;


typedef enum {
    WIN_FOCUSED = 0,
    WIN_STICKY,
    WIN_UNFOCUSED,
    WIN_TYPES_NUM
}PagerWinType;

typedef struct pager_win_attr {
    unsigned long  back_pixel;
    Pixmap back_pixmap;
    GC     foreGC ;
}PagerWinAttr;    

typedef enum {
    STYLE_FWINDOW=WIN_FOCUSED,	
    STYLE_SWINDOW=WIN_STICKY,	
    STYLE_UWINDOW=WIN_UNFOCUSED, 
    STYLE_ADESK,	
    STYLE_INADESK,
    STYLE_MAX_STYLE
} PAGER_STYLES ;

typedef struct pager_look {

  MyStyle* Styles[STYLE_MAX_STYLE];
  GC WinForeGC[WIN_TYPES_NUM];
  MyStyle** DeskStyles ;
   
  int DeskBorderWidth ;
  int TitleAlign ;

  ARGB32 GridColor ;
  GC	GridGC ;

  ARGB32 BorderColor ;  
  ARGB32 SelectionColor ; 

  MyFont windowFont ; 
  int bNeedToFixStyles ;

}PagerLook ;

/* global constants definitions */

extern PagerInfo Pager ;
extern PagerLook Look;
extern ASPipes Pipes;

typedef enum {
    ATOM_WM_DEL_WIN=0,
    ATOM_ROOT_PIXMAP,
    ATOM_STYLES,
    ATOM_BACKGROUNDS,
    ATOMS_NUM
} PagerAtoms ;
extern ASAtom  Atoms[];

#define ON  1
#define OFF 0

#define TRUE  1
#define FALSE 0


extern int WaitASResponse;
#define PAGE_MOVE_THRESHOLD		15   /* precent */
#define WAIT_AS_RESPONSE_TIMEOUT 	20   /* seconds */

#define IS_STICKY(t)	(((t->flags&ICONIFIED)&&(Pager.Flags&STICKY_ICONS))|| \
	                  (t->flags&STICKY))

/*************************************************************************
 *
 * Subroutine Prototypes
 * 
 *************************************************************************/
void SendInfo(int *fd,char *message,unsigned long window);
void DeadPipe(int nonsense);
void process_message(unsigned long type,unsigned long *body);
void GetOptions(const char *filename);
void GetBaseOptions(const char *filename);

PagerWindow *FindWindow (Window w);
void DeleteWindows( PagerWindow*t );
void list_add(unsigned long *body);
void list_configure(unsigned long *body);
void list_destroy(unsigned long *body);
void list_focus(unsigned long *body);
void list_toggle(unsigned long *body);
void list_new_page(unsigned long *body);
void list_new_desk(unsigned long *body);
void list_new_background(unsigned long *body);
void list_raise(unsigned long *body);
void list_lower(unsigned long *body);
void list_unknown(unsigned long *body);
void list_iconify(unsigned long *body);
void list_deiconify(unsigned long *body);
void list_window_name(unsigned long *body);
void list_icon_name(unsigned long *body);
void list_class(unsigned long *body);
void list_res_name(unsigned long *body);
void list_end(void);

/* look management stuff */
void LookCleanUp();
void GetLook( FILE* fd );
void FixLook();
void ApplyLook();
void OnLookUpdated();

/* Stuff in x_pager.c */
void InitDesk( long Desk );
void RetrieveBackgrounds();
void initialize_pager(void);
unsigned long GetColor(char *name);
void nocolor(char *a, char *b);
void DispatchEvent(XEvent *Event);
void DecorateDesk (long Desk);
void UpdateTransparency();
void DecoratePager();
void DisplayFrame( long Desk );
void LowerFrame();
void HideFrame();
void DestroyView( PagerWindow* t );
void ReConfigure(void);
void ReConfigureEx( unsigned width, unsigned height );
void ReConfigureAll(void);
void MovePage(void);
void DrawGrid(int i);
void LabelDesk(int i  /*Desk */ );
void HilightDesk(int i, int if_texture);
void DrawIconGrid(int erase);
void SwitchToDesk(int Desk);
void SwitchToDeskAndPage(int Desk, XEvent *Event);
int  GetWinAttributes( PagerWindow* t, 
                       XSetWindowAttributes* attributes, 
		       unsigned long *valuemask, 
		       PagerViewPosition* ppos );
void AddNewWindow(PagerWindow *prev);
void MoveResizePagerView(PagerWindow *t);
void ChangeDeskForWindow(PagerWindow *t,long newdesk);
void MoveStickyWindow(void);
void MapBalloonWindow(XEvent *);
void UnmapBalloonWindow(void);
void DrawInBalloonWindow(void);
void Hilight(PagerWindow *t);
void Scroll(int Desk, int x, int y);
void MoveWindow(XEvent *Event);

PagerWinType GetWinType(PagerWindow * t);
void PutLabel(PagerWindow * t, Window w);
#define LabelWindow(t) 		PutLabel(t,t->PagerView )
#define LabelIconWindow(t) 	PutLabel(t,t->IconView )

void ReConfigureIcons(void);
void IconSwitchPage(XEvent *Event);
void IconScroll(int x, int y);
void IconMoveWindow(XEvent *Event,PagerWindow *t);
void HandleExpose(XEvent *Event);
void MoveStickyWindows(void);



