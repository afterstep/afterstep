#ifndef ASINTERNALS_H_HEADER_INCLUDED
#define ASINTERNALS_H_HEADER_INCLUDED

#include "../../libAfterStep/asapp.h"

#include "../../libAfterImage/afterimage.h"

#include "../../libAfterStep/afterstep.h"
#include "../../libAfterStep/event.h"
#include "../../libAfterStep/decor.h"
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/mystyle.h"
#include "../../libAfterStep/mylook.h"
#include "../../libAfterStep/clientprops.h"
#include "../../libAfterStep/hints.h"
#include "../../libAfterStep/balloon.h"
#include "../../libAfterStep/parser.h"
#include "../../libAfterStep/shape.h"

#include "menus.h"

#ifdef SHAPE
#include <X11/Xresource.h>
#endif /* SHAPE */

struct FunctionData;
struct ASWindow;
struct ASRectangle;
struct ASDatabase;
struct ASWMProps;
struct MoveResizeData;
struct MenuItem;
struct ASEvent;
struct ASFeel;
struct MyLook;
struct ASHintWindow;

struct ASWindow;
typedef struct ASInternalWindow
{
    ASMagic  *data;                             /* internal data structure */
    struct ASWindow *owner;

    /* adds all the subwindows to window->ASWindow xref */
    void (*register_subwindows)( struct ASInternalWindow *asiw );

    void (*on_moveresize)( struct ASInternalWindow *asiw, Window w );
    /* fwindow looses/gains focus : */
    void (*on_hilite_changed)( struct ASInternalWindow *asiw, ASMagic *data, Bool focused );
    /* ButtonPress/Release event on one of the contexts : */
    void (*on_pressure_changed)( struct ASInternalWindow *asiw, int pressed_context );
    /* Mouse wheel scrolling event : */
    void (*on_scroll_event)( struct ASInternalWindow *asiw, struct ASEvent *event );
    /* Motion notify : */
    void (*on_pointer_event)( struct ASInternalWindow *asiw, struct ASEvent *event );
    /* KeyPress/Release : */
    void (*on_keyboard_event)( struct ASInternalWindow *asiw, struct ASEvent *event );

    /* reconfiguration : */
    void (*on_look_feel_changed)( struct ASInternalWindow *asiw, struct ASFeel *feel, struct MyLook *look, ASFlagType what );
    void (*on_root_background_changed)( struct ASInternalWindow *asiw );

    /* destruction */
    void (*destroy)( struct ASInternalWindow *asiw );

}ASInternalWindow;

/* for each window that is on the display, one of these structures
 * is allocated and linked into a list
 */
typedef struct ASWindow
  {
    unsigned long magic ;
    Window           w;     /* the child window */
    Window           frame; /* the frame window */
#define get_window_frame(asw)   (asw->frame)

	struct ASHints       *hints;

    /* Window geometry:
     *  3 different settings: anchor, status and canvas
     * 1) anchor reflects anchor point of the client window according
     *    to its size and gravity and requested position. It is calculated in virtual coordinates
     *    for non-sticky windows. For example in case of SouthEastGravity anchor.x will be at the
     *    right edge of the frame, and anchor.y will be at the bottom edge of the frame.
     * 2) status reflects current size and position of the frame as calculated based upon anchor
     *    point and frame decorations size. It is always in real screen coordinates.
     *    status->viewport_x and status->viewport_y reflect viewport position at the time of such
     *    calculation. Whenever viewport changes - this two must be changed and status recalculated.
     * 3) canvases reflect window position as reported in last received ConfigureNotify event and
     *    reflect exact position of window on screen as viewed by user.
     *
     * Anchor is needed so we could handle changing size of the frame decorations
     * status represents desired geometry/state of the window
     * canvases represent factual geometry/state of the window
     */
	struct ASStatusHints *status;
    XRectangle            anchor ;
    XRectangle            saved_anchor; /* status prior to maximization */

    struct ASWindow      *transient_owner,  *group_lead ;
    struct ASVector      *transients,       *group_members ;


#define ASWIN_NAME(t)       ((t)->hints->names[0])
#define ASWIN_NAME_ENCODING(t)       ((t)->hints->names_encoding[0])
#define ASWIN_CLASS(t)      ((t)->hints->res_class)
#define ASWIN_RES_NAME(t)   ((t)->hints->res_name)
#define ASWIN_ICON_NAME(t)  ((t)->hints->icon_name)
#define ASWIN_ICON_NAME_ENC(t)  (get_hint_name_encoding((t)->hints, (t)->hints->icon_name_idx))
#define ASWIN_DESK(t)       ((t)->status->desktop)
#define ASWIN_LAYER(t)      ((t)->status->layer)
#define ASWIN_HFLAGS(t,f) 	get_flags((t)->hints->flags,(f))
#define ASWIN_PROTOS(t,f) 	get_flags((t)->hints->protocols,(f))
#define ASWIN_GET_FLAGS(t,f) 	get_flags((t)->status->flags,(f))
#define ASWIN_SET_FLAGS(t,f)    set_flags((t)->status->flags,(f))
#define ASWIN_CLEAR_FLAGS(t,f) 	clear_flags((t)->status->flags,(f))
#define ASWIN_FUNC_MASK(t)  ((t)->hints->function_mask)

#define ASWIN_FOCUSABLE(asw)  (ASWIN_HFLAGS(asw, AS_AcceptsFocus) || get_flags((asw)->hints->protocols, AS_DoesWmTakeFocus))

#define IsBtnDisabled(t,b)  get_flags((t)->hints->disabled_buttons,(0x01<<(b)))
#define IsBtnEnabled(t,b)   (!get_flags((t)->hints->disabled_buttons,(0x01<<(b))))
#define DisableBtn(t,b)  	clear_flags((t)->hints->disabled_buttons,(0x01<<(b)))

	/********************************************************************/
	/* ASWindow frame decorations :                                     */
	/********************************************************************/
	/* window frame decoration consists of :
	  Top level window
		  4 canvases - one for each side :
		  	  Top or left canvas contains titlebar+ adjusen frame side+corners if any
			  Bottom or right canvas contains sidebar which is the same as south frame side with corners
			  Remaining two canvasses contain east and west frame sides only ( if any );
		  Canvasses surround main window and its sizes are actually the frame size.
	 */

    struct ASCanvas   *frame_canvas ;
    struct ASCanvas   *client_canvas ;
    struct ASCanvas   *frame_sides[FRAME_SIDES] ;
    struct ASCanvas   *icon_canvas ;
	struct ASCanvas   *icon_title_canvas ; /* same as icon_canvas if !SeparateButtonTitle */

    struct MyFrame    *frame_data;  /* currently selected frame decorations for this window */

    struct ASTBarData *frame_bars[FRAME_PARTS] ; /* regular sidebar is the same as frame with S, SE and SW parts */
    struct ASTBarData *tbar ;                    /* same as frame_bars[FRAME_PARTS] for convinience */
    struct ASTBarData *icon_button ;
	struct ASTBarData *icon_title ;

    int      shading_steps;

    ASInternalWindow *internal ;               /* optional related data structure,
                                                * such as ASMenu or something else */

    enum {
#define ASWT_FROM_WITHDRAWN (0x01<<0)
#define ASWT_TO_ICONIC      (0x01<<1)
#define ASWT_FROM_ICONIC    (0x01<<2)
#define ASWT_TO_WITHDRAWN   (0x01<<3)

        ASWT_StableState = 0,
        ASWT_Withdrawn2Normal = ASWT_FROM_WITHDRAWN,
        ASWT_Withdrawn2Iconic = ASWT_FROM_WITHDRAWN|ASWT_TO_ICONIC,
        ASWT_Normal2Iconic    = ASWT_TO_ICONIC,
        ASWT_Iconic2Normal    = ASWT_FROM_ICONIC,
        ASWT_Normal2Withdrawn = ASWT_TO_WITHDRAWN,
		ASWT_Iconic2Withdrawn = ASWT_FROM_ICONIC|ASWT_TO_WITHDRAWN,
		/* window may be unmapped/destroyed even prior to being withdrawn */
        ASWT_Withdrawn2Withdrawn = ASWT_FROM_WITHDRAWN|ASWT_TO_WITHDRAWN
    }wm_state_transition ;

    Time    last_restack_time ;
    int DeIconifyDesk;  /* Desk to deiconify to, for StubbornIcons */

	int     maximize_ratio_x, maximize_ratio_y ;

#define ASWF_WindowComplete				(0x01<<0)  /* if set - then AddWindow has been completed */
#define ASWF_PendingShapeRemoval		(0x01<<1)
#define ASWF_NameChanged				(0x01<<2)
#define ASWF_FirstCornerFollowsTbarSize	(0x01<<3)
#define ASWF_LastCornerFollowsTbarSize	(0x01<<6)
	ASFlagType internal_flags ;
  
  /* we use that to avoid excessive refreshes when property is updated with exact same contents */
	XWMHints 	saved_wm_hints ; 
  	XSizeHints 	saved_wm_normal_hints;
}ASWindow;

typedef struct ASLayer
{
    int          layer ;
    Window       w ;
    struct ASVector    *members ;          /* vector of ASDesktopElems */
}ASLayer;

typedef struct ASWindowList
{
    struct ASBiDirList *clients ;
    struct ASHashTable *aswindow_xref;         /* xreference of window/resource IDs to ASWindow structures */
    struct ASHashTable *layers ;               /* list of ASLayer structures from above hashed by layer num */
    struct ASHashTable *bookmarks ;            /* list of windows with bookmark names assignet to them */

    /* lists of pointers to the ASWindow structures */
    struct ASVector    *circulate_list ;
    struct ASVector    *sticky_list ;

    ASWindow *root ;         /* root window - special purpose ASWindow struct to
                              * enable root window handling same way as any other window */

    /* warping/circulation pointers : */
    int       warp_curr_index;              /* last warped to window */
    int       warp_init_dir;                /* initial warping direction */
    int       warp_curr_dir, warp_user_dir; /* current warping direction
                                             * - internal direction maybe different from
                                             *  user direction in YOYO mode */

    /* focus pointers : */
    ASWindow *active;        /* currently active client
                              *     - may not have focus during housekeeping operations
                              *     - may be different from hilited/ungrabbed if we changed active
                              *       client during house keeping operation */
    ASWindow *focused;       /* currently focused client. Will be NULL during housekeeping */
    /* needed for proper AutoRaise implementation : */
    time_t last_focus_change_sec;
    time_t last_focus_change_usec;

    ASWindow *ungrabbed;     /* client that has no grab on mouse events */
    ASWindow *hilited;       /* client who's frame decorations has been hilited
                              * to show that it is focused. May not be same as focused/ungrabbed
                              * right after focus is handed over and before FocusIn event */
    ASWindow *previous_active;        /* last active client */
    ASWindow *pressed;       /* the client wich has one of its frame parts pressed at the moment */
    /* Focus management is somewhat tricky.
     * Firstly, we have to track pointer movements to see when mouse enters the window
     *          so we can switch focus to that window. ( or mouse clicked in the window
     *          where ClickToFocus is requested ). To Accomplish that we grab mouse
     *          events on all the windows but currently focused.
     * Secondly we need to hilite currently active window with frame decorations, when window
     *          gets focus. So right after focus is forced on window and before FocusIn event
     *          we'll have : (focused == ungrabbed) != hilited
     * Thirdly, we may need to steal focus for our own needs while we perform housekeeping
     *          like desk switching, etc. ( GrabEm/UngrabEm ) In such situation window
     *          remains hilited, but has no focus, and has no pointer event grabs.
     *          (focused == NULL) != (ungrabbed == hilited)
     * Fourthsly, When housekeeping is completed we want to give the focus back to it, but
     *          in some situations housekeeping operations will make us give the focus to
     *          some other window. To do that we substitute focused pointer to new window
     *          right before UngrabEm. In this case focused != ( ungrabbed == hilited )
     */

	ASVector	*stacking_order ; 		/* array of pointers to ASWindow structures */
	
}ASWindowList;

/* Mirror Note :
 *
 * For the purpose of sizing/placing left and right sides and corners - we employ somewhat
 * twisted logic - we mirror sides over lt2rb diagonal in case of
 * vertical title orientation. That allows us to apply simple x/y switching instead of complex
 * calculations. Note that we only do that for placement purposes. Contexts and images are
 * still taken from MyFrame parts as if it was rotated counterclockwise instead of mirrored.
 */


typedef struct ASOrientation
{
    unsigned int frame_contexts[FRAME_PARTS];
    unsigned int frame_rotation[FRAME_PARTS];
    unsigned int tbar2canvas_xref[FRAME_PARTS+1];
    unsigned int tbar_side;
    unsigned int tbar_corners[2];
    unsigned int tbar_mirror_corners[2];               /* see note below */
    unsigned int sbar_side;
    unsigned int sbar_corners[2];
    unsigned int sbar_mirror_corners[2];               /* see note below */
    unsigned int left_side, right_side;
    unsigned int left_mirror_side, right_mirror_side;  /* see note below */
    unsigned long horz_side_mask;
    int left_btn_order, right_btn_order;
    int *in_x, *in_y;
    unsigned int *in_width, *in_height;
    int *out_x, *out_y;
    unsigned int *out_width, *out_height;
    int flip;
#define ASO_TBAR_ELEM_LBTN      	   	MYFRAME_TITLE_BACK_LBTN
#define ASO_TBAR_ELEM_LSPACER		   	MYFRAME_TITLE_BACK_LSPACER
#define ASO_TBAR_ELEM_LTITLE_SPACER    	MYFRAME_TITLE_BACK_LTITLE_SPACER
#define ASO_TBAR_ELEM_LBL       	   	MYFRAME_TITLE_BACK_LBL
#define ASO_TBAR_ELEM_RTITLE_SPACER	 	MYFRAME_TITLE_BACK_RTITLE_SPACER
#define ASO_TBAR_ELEM_RSPACER			MYFRAME_TITLE_BACK_RSPACER
#define ASO_TBAR_ELEM_RBTN      		MYFRAME_TITLE_BACK_RBTN
#define ASO_TBAR_ELEM_NUM       		MYFRAME_TITLE_BACKS
    unsigned int default_tbar_elem_col[ASO_TBAR_ELEM_NUM];
    unsigned int default_tbar_elem_row[ASO_TBAR_ELEM_NUM];
    ASFlagType left_spacer_needed_align ;
    ASFlagType right_spacer_needed_align ;
}ASOrientation;


typedef struct queue_buff_struct
{
  struct queue_buff_struct *next;
  unsigned char            *data;
  short                     size;
  short                     done;
  short 					prealloced_idx ; 
  short 					unused;
  /* 16 bytes total */
}queue_buff_struct;

typedef struct module_ibuf_t
{
  /* we always use 32 bit values for communications */
  CARD32                len;
  CARD32                done;
  CARD32                window;
  CARD32                size;
  char                 *text;
  CARD32                name_size, text_size;
  struct FunctionData  *func;
  CARD32                cont;
}module_ibuf_t;

typedef struct module_t
{
  int                   fd;
  int                   active;
  char                 *name;
  char                 *cmd_line;
  CARD32                mask;
  CARD32                lock_on_send_mask;
  queue_buff_struct    *output_queue;
  module_ibuf_t         ibuf;
}module_t;


/******************************************************************/
/* these are global functions and variables private for afterstep */

/* from configure.c */
void error_point();
void tline_error(const char* err_text);
void str_error(const char* err_format, const char* string);

int is_executable_in_path (const char *name);

/* from dirtree.c */
char * strip_whitespace (char *str);

/* from configure.c */
extern struct ASDatabase    *Database;

/**************************************************************************/
/* Global variables :                                                     */
/**************************************************************************/

extern ASFlagType    AfterStepState;              /* see ASS_ flags above */
/* this are linked lists of structures : */
extern struct ASDatabase *Database;
extern ASHashTable       *ComplexFunctions;

extern ASBalloonState *MenuBalloons ; 
extern ASBalloonState *TitlebarBalloons ; 


/* global variables for Look values : */
extern unsigned long XORvalue;
extern int           RubberBand;
extern char         *RMGeom;
extern int           Xzap, Yzap;
extern int           DrawMenuBorders;
extern int           TextureMenuItemsIndividually;
extern int           StartMenuSortMode;
extern int           ShadeAnimationSteps;

extern int           fd_width, x_fd;

extern struct ASWindow *ColormapWin;

extern struct ASVector *Modules;               /* dynamically resizable array of module_t data structures */
#define MODULES_LIST    VECTOR_HEAD(module_t,*Modules)
#define MODULES_NUM     VECTOR_USED(*Modules)

extern int           Module_fd;
extern int           Module_npipes;

extern Bool   		 menu_event_mask[LASTEvent];  /* menu event filter */

extern int    menuFromFrameOrWindowOrTitlebar;

#define BACKGROUND_DRAW_CHILD   (MAX_SINGLETONS_NUM-1)



extern ASOrientation HorzOrientation ;
extern ASOrientation VertOrientation ;


/**************************************************************************/
/**************************************************************************/
/* Function prototypes :                                                  */
/**************************************************************************/

/*************************** afterstep.c : ********************************/
void Done (Bool restart, char *command);

/*************************** from aswindow.c : ****************************/
ASWindowList *init_aswindow_list();
void destroy_aswindow_list( ASWindowList **list, Bool restore_root );
void publish_aswindow_list( ASWindowList *list, Bool stacking_only );

ASWindow *window2ASWindow( Window w );
Bool register_aswindow( Window w, ASWindow *asw );
Bool unregister_aswindow( Window w );
Bool destroy_registered_window( Window w );
Bool bookmark_aswindow( ASWindow *asw, char *bookmark );
ASWindow *bookmark2ASWindow( const char *bookmark );
ASWindow *pattern2ASWindow( const char *pattern );
ASWindow *complex_pattern2ASWindow( char *pattern );
ASLayer *get_aslayer( int layer, ASWindowList *list );
ASWindow *find_group_lead_aswindow( Window id );
void tie_aswindow( ASWindow *t );
void untie_aswindow( ASWindow *t );
void add_aswindow_to_layer( ASWindow *asw, int layer );
void remove_aswindow_from_layer( ASWindow *asw, int layer );
Bool enlist_aswindow( ASWindow *t );
void delist_aswindow( ASWindow *t );
void save_aswindow_list( ASWindowList *list, char *file );
ASWindow* find_topmost_client( int desk, int root_x, int root_y );
void free_scratch_layers_vector();
void free_scratch_ids_vector();
void update_stacking_order();
void restack_window_list( int desk);
void send_stacking_order( int desk );
void apply_stacking_order( int desk );


Bool is_window_obscured (ASWindow * above, ASWindow * below);
void restack_window( ASWindow *t, Window sibling_window, int stack_mode );

#ifndef NO_DEBUG_OUTPUT
#define RaiseWindow(asw)    do{show_progress(__FILE__ " %s:%d R",__FUNCTION__ ,__LINE__);restack_window((asw),None,Above);}while(0)
#define LowerWindow(asw)    do{show_progress(__FILE__ " %s:%d L",__FUNCTION__ ,__LINE__);restack_window((asw),None,Below);}while(0)
#define RaiseObscuredWindow(asw)  do{show_progress(__FILE__ " %s:%d RO",__FUNCTION__,__LINE__);restack_window((asw),None,TopIf);}while(0)
#define RaiseLowerWindow(asw)     do{show_progress(__FILE__ " %s:%d RL",__FUNCTION__,__LINE__);restack_window((asw),None,Opposite);}while(0)
#else
#define RaiseWindow(asw)    restack_window((asw),None,Above)
#define LowerWindow(asw)    restack_window((asw),None,Below)
#define RaiseObscuredWindow(asw)  restack_window((asw),None,TopIf)
#define RaiseLowerWindow(asw)     restack_window((asw),None,Opposite)
#endif

ASWindow     *get_next_window (ASWindow * curr_win, char *action, int dir);
ASWindow     *warp_aswindow_list ( ASWindowList *list, Bool backwards );

MenuData *make_desk_winlist_menu(  ASWindowList *list, int desk, int sort_order, Bool icon_name );

void hide_focus();
Bool focus_window( ASWindow *asw, Window w );
void autoraise_window( ASWindow *asw );
#define FOCUS_ASW_CAN_AUTORAISE		False
#define FOCUS_ASW_DONT_AUTORAISE	True
Bool focus_aswindow( ASWindow *asw, Bool suppress_autoraise );
Bool focus_active_window();
void focus_next_aswindow( ASWindow *asw );     /* should be called when window is unmapped or destroyed */
void focus_prev_aswindow( ASWindow *asw );     /* should be called when window is unmapped or destroyed */
void commit_circulation();


/*************************** from add_window.c : *************************/
void destroy_icon_windows( ASWindow *asw );
Bool get_icon_root_geometry( ASWindow *asw, ASRectangle *geom );

/* swiss army knife - does everything about grabbing : */
void grab_window_input( ASWindow *asw, Bool release_grab );
ASImage* get_window_icon_image( ASWindow *asw );


void redecorate_window( ASWindow *asw, Bool free_resources );
void update_window_transparency( ASWindow *asw, Bool force  );
void on_window_moveresize( ASWindow *asw, Window w );
void on_icon_changed( ASWindow *asw );
void on_window_title_changed( ASWindow *asw, Bool update_display );
void on_window_hints_changed( ASWindow *asw );
void on_window_opacity_changed( ASWindow *asw );
void on_window_status_changed( ASWindow *asw, Bool reconfigured );
void on_window_hilite_changed( ASWindow *asw, Bool focused );
void on_window_pressure_changed( ASWindow *asw, int pressed_context );

void SelectDecor (ASWindow *);
void check_aswindow_shaped( ASWindow *asw );
ASWindow *AddWindow (Window w, Bool from_map_request);
ASWindow *AddInternalWindow (Window w,
                             ASInternalWindow **pinternal,
                             struct ASHints **phints, struct ASStatusHints *status);
void Destroy (ASWindow *, Bool);
void RestoreWithdrawnLocation (ASWindow *, Bool);
void SetShape (ASWindow *, int);
void ClearShape (ASWindow *asw);
void SendConfigureNotify(ASWindow *asw);



/*************************** colormaps.c : ********************************/
void SetupColormaps();
void CleanupColormaps();
void InstallWindowColormaps (ASWindow *asw);
void UninstallWindowColormaps (ASWindow *asw);
void InstallRootColormap (void);
void UninstallRootColormap (void);
void InstallAfterStepColormap (void);
void UninstallAfterStepColormap (void);


/*************************** configure.c **********************************/
Bool GetIconFromFile (char *file, MyIcon * icon, int max_colors);
struct ASImage *GetASImageFromFile (char *file);

void InitBase (Bool free_resources);
void InitDatabase (Bool free_resources);

void InitLook (struct MyLook *look, Bool free_resources);
void InitFeel (struct ASFeel *feel, Bool free_resources);

#define PARSE_BASE_CONFIG       BASE_CONFIG
#define PARSE_LOOK_CONFIG       LOOK_CONFIG
#define PARSE_FEEL_CONFIG       FEEL_CONFIG
#define PARSE_DATABASE_CONFIG   DATABASE_CONFIG

#define PARSE_EVERYTHING        (0xFFFFFFFF)

void LoadASConfig (int thisdesktop, ASFlagType what);

/*************************** cover.c **************************************/

void remove_desktop_cover();
Window get_desktop_cover_window();
void restack_desktop_cover();
void cover_desktop();
void desktop_cover_cleanup();
void display_progress( Bool new_line, const char *msg_format, ... );

/*************************** dbus.c ***************************************/
int asdbus_init();
void asdbus_shutdown();

/*************************** decorations.c ********************************/
ASOrientation* get_orientation_data( ASWindow *asw );
void grab_aswindow_buttons( ASWindow *asw, Bool focused );

int check_allowed_function2 (int func, ASHints *hints);
int check_allowed_function (FunctionData *fdata, ASHints *hints);
ASFlagType compile_titlebuttons_mask (ASHints *hints);
void estimate_titlebar_size( ASHints *hints, unsigned int *width_ret, unsigned int *height_ret );
void disable_titlebuttons_with_function (ASWindow * t, int function);
Bool hints2decorations( ASWindow *asw, ASHints *old_hints );
void invalidate_window_mystyles( ASWindow *asw );

/*************************** events.c ********************************/
const char *context2text(int ctx);
void DigestEvent    ( struct ASEvent *event );
void DispatchEvent  ( struct ASEvent *event, Bool deferred );
void HandleEvents   ();
void InteractiveMoveLoop ();
void WaitForButtonsUpLoop ();
Bool WaitEventLoop( struct ASEvent *event, int finish_event_type, long timeout, struct ASHintWindow *hint );
Bool IsClickLoop( struct ASEvent *event, unsigned int end_mask, unsigned int click_time );
ASWindow *WaitWindowLoop( char *pattern, long timeout );
void ConfigureNotifyLoop();
void MapConfigureNotifyLoop();


void AlarmHandler (int nonsense);

Bool KeyboardShortcuts (XEvent * xevent, int return_event, int move_size);

void HandleExpose (struct ASEvent*);
void HandleFocusIn (struct ASEvent *event);
void HandleDestroyNotify (struct ASEvent *event);
void HandleMapRequest (struct ASEvent *event);
void HandleMapNotify (struct ASEvent *event);
void HandleUnmapNotify (struct ASEvent *event);
void HandleButtonRelease(struct ASEvent *event, Bool deffered);
void HandleButtonPress (struct ASEvent *event, Bool deffered);
void HandleEnterNotify (struct ASEvent *event);
void HandleLeaveNotify (struct ASEvent *event);
void HandleConfigureRequest (struct ASEvent *event);
void HandleClientMessage (struct ASEvent *event);
void HandlePropertyNotify (struct ASEvent *event);
void HandleKeyPress (struct ASEvent *event);
void HandleVisibilityNotify (struct ASEvent *event);
void HandleColormapNotify (struct ASEvent *event);
void HandleSelectionClear( struct ASEvent *event );

void HandleShapeNotify (struct ASEvent *event);
void HandleShmCompletion(struct ASEvent *event);
/*************************** functions.c **********************************/
void SetupFunctionHandlers();
ComplexFunction *get_complex_function( char *name );

/* schedule function for execution( add to queue ) */
void ExecuteFunction (struct FunctionData *data, struct ASEvent *event, int Module);
void ExecuteFunctionForClient(struct FunctionData *data, Window client);
void ExecuteFunctionExt (struct FunctionData *data, struct ASEvent *event, int module, Bool defered);
/* execute all the scheduled functions from the queue */
void ExecutePendingFunctions();
void DestroyPendingFunctionsQueue();
/* non-window specific, non-defferrable functions are run : */
void ExecuteBatch ( ComplexFunction *batch );

int  DeferExecution (struct ASEvent *event, int cursor, int FinishEvent);
void QuickRestart (char *what);

/************************* housekeeping.c ********************************/
Bool GrabEm   ( struct ScreenInfo *scr, Cursor cursor );
void UngrabEm ();
void CheckGrabbedFocusDestroyed(ASWindow *destroyed);


Bool StartWarping(struct ScreenInfo *scr);
void ChangeWarpingFocus(ASWindow *new_focus);
void CheckWarpingFocusDestroyed(ASWindow *destroyed);
void EndWarping();

void PasteSelection (struct ScreenInfo *scr );

/*************************** icons.c *********************************/
void destroy_asiconbox( ASIconBox **pib );
ASIconBox *get_iconbox( int desktop );
Bool add_iconbox_icon( ASWindow *asw );
Bool remove_iconbox_icon( ASWindow *asw );
Bool change_iconbox_icon_desk( ASWindow *asw, int from_desk, int to_desk );
void rearrange_iconbox_icons( int desktop );


/*************************** menus.c *********************************/

/*************************** menuitem.c *********************************/
int parse_modifier( char *tline );
FunctionData *String2Func ( const char *string, FunctionData *p_fdata, Bool quiet );
void ParsePopupEntry (char *tline, FILE * fd, char **junk, int *junk2);


/*************************** misc.c *********************************/
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
void SetTimer (int delay);


/***************************** module.c ***********************************/
void SetupModules(void);

void ExecModule (char *action, Window win, int context);
int  AcceptModuleConnection (int socket_fd);

void SendVector (int channel, send_data_type msg_type, ASVector *vector);
void SendPacket ( int channel, send_data_type msg_type, send_data_type num_datum, ...);
void SendConfig (int module, send_data_type event_type, ASWindow * t);
void SendString ( int channel, send_data_type msg_type,
             Window w, Window frame, ASWindow *asw_ptr,
			 char *string, send_data_type encoding );
void SendStackingOrder (int channel, send_data_type msg_type, send_data_type desk, ASVector *ids);
/* simplified specialized interface to above functions : */
void broadcast_focus_change( ASWindow *asw, Bool focused );
void broadcast_window_name( ASWindow *asw );
void broadcast_icon_name( ASWindow *asw );
void broadcast_res_names( ASWindow *asw );
void broadcast_status_change( int message, ASWindow *asw );
void broadcast_config (send_data_type event_type, ASWindow * t);


void HandleModuleInOut(unsigned int channel, Bool has_input, Bool has_output);

void KillModuleByName (char *name);
void KillAllModulesByName (char *name);

int FindModuleByName (char *name);
char *GetModuleCmdLineByName(char *name);
void DeadPipe (int nonsense);
void ShutdownModules(Bool dont_free_memory);

void RunCommand (FunctionData * fdata, unsigned int channel, Window w);

void FlushAllQueues();
MenuData *make_module_menu( FunctionCode func, const char *title, int sort_order );
#define make_stop_module_menu(sort_order)  make_module_menu(  F_KILLMODULEBYNAME, "Stop Running Module", sort_order )
#define make_restart_module_menu(sort_order)  make_module_menu(  F_RESTARTMODULEBYNAME, "Restart Running Module", sort_order )


/******************************* outline.c ********************************/
void MoveOutline( struct MoveResizeData * pdata );

/******************************* pager.c ***********************************/
/* we use 4 windows that are InputOnly and therefore are invisible on the
 * sides of the screen to steal mouse events and allow for virtual viewport
 * move when cursor reaches edge of the screen. :*/
void MoveViewport (int newx, int newy, Bool grab);
void HandlePaging (int HorWarpSize, int VertWarpSize, int *xl,
                   int *yt, int *delta_x, int *delta_y, Bool Grab, struct ASEvent *event);
void ChangeDesks (int new_desk);
void ChangeDeskAndViewport ( int new_desk, int new_vx, int new_vy, Bool force_grab);
MyBackground *get_desk_back_or_default( int desk, Bool old_desk );
void change_desktop_background( int desk );
void HandleBackgroundRequest( struct ASEvent *event );
Bool is_background_xfer_ximage( unsigned long id );
void stop_all_background_xfer();
void release_all_old_background( Bool forget );




/******************************* placement.c *******************************/
ASGrid* make_desktop_grid (int desk, int min_layer, Bool frame_only, int vx, int vy, ASWindow *target);
void setup_aswindow_moveresize (ASWindow *asw,  struct ASMoveResizeData *mvrdata);
Bool place_aswindow (ASWindow *asw);
void apply_aswindow_move (struct ASMoveResizeData *data);
void apply_aswindow_moveresize (struct ASMoveResizeData *data);
void complete_aswindow_move (struct ASMoveResizeData *data, Bool cancelled);
void complete_aswindow_moveresize (struct ASMoveResizeData *data, Bool cancelled);
void enforce_avoid_cover (ASWindow *asw);
void obey_avoid_cover (ASWindow *asw, ASStatusHints *tmp_status, XRectangle *tmp_anchor, int max_layer);


/******************************* theme.c ***********************************/
Bool install_theme_file( const char *src );

/******************************* winstatus.c *******************************/
void complete_wm_state_transition( ASWindow *asw, int state );
Bool apply_window_status_size(register ASWindow *asw, ASOrientation *od);
Bool set_window_wm_state( ASWindow *asw, Bool iconify, Bool force_unmapped );
Bool make_aswindow_visible( ASWindow *asw, Bool deiconify );
void change_aswindow_layer( ASWindow *asw, int layer );
void quietly_reparent_aswindow( ASWindow *asw, Window dst, Bool user_root_pos );
void change_aswindow_desktop( ASWindow *asw, int new_desk, Bool force );
void toggle_aswindow_status( ASWindow *asw, ASFlagType flags );
Bool check_canvas_offscreen (ASCanvas *pc);
Bool check_window_offscreen (ASWindow *asw);
Bool check_frame_offscreen (ASWindow *asw);
Bool check_frame_side_offscreen (ASWindow *asw, int i);



void hide_hilite();                            /* unhilites currently highlited window */
void hilite_aswindow( ASWindow *asw );         /* actually hilites focused window on reception of event */
void warp_to_aswindow( ASWindow *asw, Bool deiconify );
Bool activate_aswindow( ASWindow *asw, Bool force, Bool deiconify );
void press_aswindow( ASWindow *asw, int context );
void release_pressure();

void save_aswindow_anchor( ASWindow *asw, Bool hor, Bool vert );
void moveresize_aswindow_wm( ASWindow *asw, int x, int y, unsigned int width, unsigned int height, Bool save_anchor );

void on_window_anchor_changed( ASWindow *asw );
void validate_window_anchor (ASWindow *asw, XRectangle *new_anchor, Bool initial_placement);


Bool init_aswindow_status( ASWindow *t, ASStatusHints *status );


#endif /* ASINTERNALS_H_HEADER_INCLUDED */
