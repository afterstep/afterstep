/**********************************************************************
 *
 * Codes for afterstep builtins
 *
 **********************************************************************/

#ifndef _FUNCTIONS_
#define _FUNCTIONS_

#ifdef __cplusplus
extern "C" {
#endif


struct ASHashTable;
struct TermDef;
struct ASImage;

typedef enum {
  F_NOP = 0,
  F_TITLE,
  F_BEEP,
  F_QUIT,
  F_RESTART,
  F_REFRESH,
#ifndef NO_VIRTUAL
  F_SCROLL,
  F_GOTO_PAGE,
  F_TOGGLE_PAGE,
#endif
  F_MOVECURSOR,
  F_WARP_F,
  F_WARP_B,
  F_WAIT,
  F_DESK,
  F_GOTO_DESKVIEWPORT,
#ifndef NO_WINDOWLIST
  F_WINDOWLIST,
#endif
  F_POPUP,
  F_FUNCTION,
  F_MINIPIXMAP,
  F_EXEC,
  F_MODULE,
  F_KILLMODULEBYNAME,
  F_QUICKRESTART,
  F_CHANGE_BACKGROUND,
  F_CHANGE_LOOK,
  F_CHANGE_FEEL,
  F_CHANGE_THEME,
  F_CHANGE_THEME_FILE,
  F_CHANGE_COLORSCHEME,
  F_INSTALL_LOOK,
  F_INSTALL_FEEL,
  F_INSTALL_BACKGROUND,
  F_INSTALL_FONT,
  F_INSTALL_ICON,
  F_INSTALL_TILE,
  F_INSTALL_THEME_FILE,
  F_INSTALL_COLORSCHEME,
  F_SAVE_WORKSPACE,
  F_ENDFUNC,
  F_ENDPOPUP,
  F_TAKE_SCREENSHOT,
  F_Test,    /* for debugging purposes to be able to test new features before actually
              * enabling them for user */
  /* this functions require window as aparameter */
  F_WINDOW_FUNC_START,
  F_MOVE,
  F_RESIZE,
  F_RAISE,
  F_LOWER,
  F_RAISELOWER,
  F_PUTONTOP,
  F_PUTONBACK,
  F_SETLAYER,
  F_TOGGLELAYER,
  F_SHADE,
  F_DELETE,
  F_DESTROY,
  F_CLOSE,
  F_ICONIFY,
  F_MAXIMIZE,
  F_STICK,
  F_FOCUS,
  F_CHANGEWINDOW_UP,
  F_CHANGEWINDOW_DOWN,
  F_GOTO_BOOKMARK,
  F_GETHELP,
  F_PASTE_SELECTION,
  F_CHANGE_WINDOWS_DESK,
  F_BOOKMARK_WINDOW,
  F_PIN_MENU,
  F_TAKE_WINDOWSHOT,
  F_TAKE_FRAMESHOT,
  /* end of window functions */
  /* these are commands  to be used only by modules */
/* end of window functions */
  /* these are commands  to be used only by modules */
  F_MODULE_FUNC_START,
  F_SEND_WINDOW_LIST,
  F_SET_MASK,
  F_SET_NAME,
  F_UNLOCK,
  F_SET_FLAGS,
  /* these are internal commands */
  F_INTERNAL_FUNC_START,
  F_RAISE_IT,
  F_Folder,
  /* this one is treated by AS same as F_EXEC but it really
     should be used only in Wharf config : */
  F_SWALLOW_FUNC_START,
  F_Swallow = F_SWALLOW_FUNC_START,
  F_MaxSwallow,
  F_SwallowModule,
  F_MaxSwallowModule,
  F_DropExec,
  F_SWALLOW_FUNC_END,
  F_Size = F_SWALLOW_FUNC_END,
  F_Transient,
F_FUNCTIONS_NUM
} FunctionCode ;

#define NEED_NAME   (1<<0)
#define	NEED_PATH 	(1<<1)
#define	NEED_WINDOW 	(1<<2)
#define	NEED_WINIFNAME 	(1<<3)
#define	NEED_CMD 	(1<<4)
/* this one instructs us to append numeric arguments at the end of the
   function definition */
#define	USES_NUMVALS 	(1<<5)


#define IsWindowFunc(f)  ((f)>F_WINDOW_FUNC_START&&(f)<F_MODULE_FUNC_START)
#define IsModuleFunc(f)  ((f)>F_MODULE_FUNC_START&&(f)<F_INTERNAL_FUNC_START)
#define IsInternFunc(f)  ((f)>F_INTERNAL_FUNC_START&&(f)<F_FUNCTIONS_NUM)
#define IsValidFunc(f)   ((f)>=0&&(f)<F_FUNCTIONS_NUM)
#define IsSwallowFunc(f) ((f)>=F_SWALLOW_FUNC_START&&(f)<F_SWALLOW_FUNC_END)
#define IsExecFunc(f)    ((f)>= F_EXEC && (f)<=F_KILLMODULEBYNAME)


#define UP 1
#define DOWN 0
void ChangeWarpIndex(const long, FunctionCode);

#define MAX_FUNC_ARGS	2

typedef struct FunctionData
{
    CARD32 func;       /* AfterStep built in function */
    INT32 func_val[MAX_FUNC_ARGS];
#define DEFAULT_MAXIMIZE    100
#define DEFAULT_OTHERS      0
    INT32 unit_val[MAX_FUNC_ARGS];
    char unit[MAX_FUNC_ARGS] ;
#define APPLY_VALUE_UNIT(size,value,unit) ((((unit)>0)?(value)*(unit):(value)*(size))/100)

#define DEFAULT_MAXIMIZE    100

    char* name ;
    char* text ;
#define COMPLEX_FUNCTION_NAME(pd)  (((pd)->text)?(pd)->text:(pd)->name)
    char hotkey ;
    void* popup ; /* actually a MenuRoot pointer */
} FunctionData ;

typedef struct ComplexFunction
{
    unsigned long   magic;
    char           *name ;

    FunctionData   *items;
    unsigned int    items_num;
}ComplexFunction;

typedef struct MenuDataItem
  {

    unsigned long   magic;
    struct MenuDataItem  *next;  /* next menu item */
    struct MenuDataItem  *prev;  /* prev menu item */

#define MD_Disabled        (0x01<<0)
/* can't think of anything else atm - maybe add something later ? */
    ASFlagType            flags ;
    struct FunctionData  *fdata ;
    char                 *minipixmap ;         /* we always read filename from config !! */
    struct ASImage       *minipixmap_image ;
    char *item;         /* the character string displayed on left */
    char *item2;		/* the character string displayed on right */

    time_t                last_used_time ;
}MenuDataItem;

typedef struct MenuData
{
    unsigned long    magic;
    char *name;         /* name of root */

    struct MenuDataItem *first; /* first item in menu */
    struct MenuDataItem *last;  /* last item in menu */
    short items_num;        /* number of items in the menu */

	unsigned int    recent_items ;
}MenuData;

#define MAX_MENU_ITEM_HEIGHT    (Scr.MyDisplayHeight>>4)
#define MAX_MENU_WIDTH          (Scr.MyDisplayWidth>>1)
#define MAX_MENU_HEIGHT         ((Scr.MyDisplayHeight*9)/10)
#define MIN_MENU_X              5
#define MAX_MENU_X              (Scr.MyDisplayWidth-5)
#define MIN_MENU_Y              5
#define MAX_MENU_Y              (Scr.MyDisplayHeight-5)
#define DEFAULT_ARROW_SIZE      (Scr.MyDisplayWidth>>7)
#define DEFAULT_SCROLLBAR_SIZE      (Scr.MyDisplayWidth>>6)

/* Types of events for the FUNCTION builtin */
#define MOTION                       'm'
#define MOTION_UPPER                 'M'
#define IMMEDIATE                    'i'
#define IMMEDIATE_UPPER              'I'
#define CLICK                        'c'
#define CLICK_UPPER                  'C'
#define DOUBLE_CLICK                 'd'
#define DOUBLE_CLICK_UPPER           'D'
#define TRIPLE_CLICK                 't'
#define TRIPLE_CLICK_UPPER           'T'
#define ONE_AND_A_HALF_CLICKS        'o'
#define ONE_AND_A_HALF_CLICKS_UPPER  'O'
#define TWO_AND_A_HALF_CLICKS        'h'
#define TWO_AND_A_HALF_CLICKS_UPPER  'H'
#define CLICKS_TRIGGERS_LOWER        "mcodht"
#define CLICKS_TRIGGERS_UPPER        "MCODHT"
#define MAX_CLICKS_HANDLED           6

struct ASEvent;

typedef void (*as_function_handler)(struct FunctionData *data, struct ASEvent *event, int module);

struct TermDef *txt2fterm (const char *txt, int quiet);
struct TermDef *func2fterm (FunctionCode func, int quiet);
int txt2func (const char *text, FunctionData * fdata, int quiet);
int parse_func (const char *text, FunctionData * data, int quiet);
FunctionData *String2Func ( const char *string, FunctionData *p_fdata, Bool quiet );

void init_func_data (FunctionData * data);
void copy_func_data (FunctionData * dst, FunctionData * src);
void dup_func_data (FunctionData * dst, FunctionData * src);
inline FunctionData *create_named_function( int func, char *name);
void set_func_val (FunctionData * data, int arg, int value);
int free_func_data (FunctionData * data);
void destroy_func_data( FunctionData **pdata );
long default_func_val( FunctionCode func );
void decode_func_units (FunctionData * data);
void complex_function_destroy(ASHashableValue value, void *data);
void really_destroy_complex_func(ComplexFunction *cf);
void init_list_of_funcs(struct ASHashTable **list, Bool force);
ComplexFunction *new_complex_func( struct ASHashTable *list, char *name );
ComplexFunction *find_complex_func( struct ASHashTable *list, char *name );
void menu_data_item_destroy(MenuDataItem *mdi);
void purge_menu_data_items(MenuData *md);
void menu_data_destroy(ASHashableValue value, void *data);
void init_list_of_menus ( ASHashTable **list, Bool force);
MenuData *new_menu_data ( ASHashTable *list, char *name );
MenuData *find_menu_data( ASHashTable *list, char *name );
MenuDataItem *new_menu_data_item( MenuData *menu );
int parse_menu_item_name (MenuDataItem * item, char **name);

void add_menu_fdata_item( MenuData *menu, FunctionData *fdata, char *minipixmap, struct ASImage *img );
void menu_data_item_from_func (MenuData * menu, FunctionData * fdata);
struct ASImage *check_scale_menu_pmap( struct ASImage *im ); 
void reload_menu_pmaps( MenuData *menu );

void print_func_data(const char *file, const char *func, int line, FunctionData *data);

#if defined(LOCAL_DEBUG) || defined(DEBUG)
#define MenuDataItemFromFunc(m,f) \
do{fprintf(stderr,"MenuDataItemFromFunc called:");print_func_data(__FILE__, __FUNCTION__, __LINE__,f);menu_data_item_from_func((m),(f));}while(0)
#else
#define MenuDataItemFromFunc(m,f) menu_data_item_from_func((m),(f))
#endif



#ifdef __cplusplus
}
#endif



#endif /* _FUNCTIONS_ */
