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

typedef enum FunctionCode{
  F_NOP = 0,
  F_TITLE,
  F_BEEP,
  F_QUIT,
  F_RESTART,
	F_SYSTEM_SHUTDOWN,
	F_LOGOUT,
	F_QUIT_WM,
	F_SUSPEND,
	F_HIBERNATE,
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
  F_STOPMODULELIST,
  F_RESTARTMODULELIST,
  F_POPUP,
  F_FUNCTION,
  F_CATEGORY,
  F_CATEGORY_TREE,
  F_MINIPIXMAP,
  F_SMALL_MINIPIXMAP,
  F_LARGE_MINIPIXMAP,
	F_Preview,
  F_DesktopEntry,
  F_EXEC,
  F_ExecInDir,
  F_MODULE,
  F_ExecToolStart,
  F_ExecInTerm=F_ExecToolStart,
  F_ExecBrowser,
  F_ExecEditor,
  F_ExecToolEnd = F_ExecEditor,
  F_KILLMODULEBYNAME,
  F_RESTARTMODULEBYNAME,
  F_KILLALLMODULESBYNAME,
  F_QUICKRESTART,
  F_CHANGE_BACKGROUND,
  F_CHANGE_BACKGROUND_FOREIGN,
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
  F_SIGNAL_RELOAD_GTK_RCFILE,
  F_KIPC_SEND_MESSAGE_ALL,     /* sends KIPC message to all KDE windows */
  F_ENDFUNC,
  F_ENDPOPUP,
  F_TAKE_SCREENSHOT,
  F_SET,
  F_Test,    /* for debugging purposes to be able to test new features before actually
			  * enabling them for user */
	F_Remap,
			 
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
  F_FULLSCREEN,
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
  F_SWALLOW_WINDOW,
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

#define FUNC_ERR_START		-100
#define FUNC_ERR_NO_NAME	-100 
#define FUNC_ERR_NO_TEXT	-101

typedef enum {
	MINIPIXMAP_Icon = 0,
	MINIPIXMAP_Preview,
	MINIPIXMAP_TypesNum
} MinipixmapTypes;

#define GetMinipixmapType(f) (((f) >= F_MINIPIXMAP && (f) <= F_LARGE_MINIPIXMAP)? 0 : ((f)==F_Preview) ? 1 : 2)
#define IsMinipixmap(t)  ((t)< MINIPIXMAP_TypesNum)


#define IsWindowFunc(f)  ((f)>F_WINDOW_FUNC_START&&(f)<F_MODULE_FUNC_START)
#define IsModuleFunc(f)  ((f)>F_MODULE_FUNC_START&&(f)<F_INTERNAL_FUNC_START)
#define IsInternFunc(f)  ((f)>F_INTERNAL_FUNC_START&&(f)<F_FUNCTIONS_NUM)
#define IsValidFunc(f)   ((f)>=0&&(f)<F_FUNCTIONS_NUM)
#define IsSwallowFunc(f) ((f)>=F_SWALLOW_FUNC_START&&(f)<F_SWALLOW_FUNC_END)
#define IsExecFunc(f)    ((f)>= F_EXEC && (f)< F_KILLMODULEBYNAME)
#define IsMinipixmapFunc(f)  (IsMinipixmap(GetMinipixmapType(f)))

#ifndef NO_WINDOWLIST
#define IsDynamicPopup(f)	((f) >= F_WINDOWLIST     && (f) <= F_RESTARTMODULELIST)
#else
#define IsDynamicPopup(f)	((f) >= F_STOPMODULELIST && (f) <= F_RESTARTMODULELIST)
#endif

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
#define APPLY_VALUE_UNIT(size,value,unit) (((unit)>0)?(value)*(unit):((value)*(size))/100)

#define DEFAULT_MAXIMIZE    100

	char* name ;
	char* text ;
#define COMPLEX_FUNCTION_NAME(pd)  (((pd)->text)?(pd)->text:(pd)->name)
	char hotkey ;
	void* popup ; /* actually a MenuRoot pointer */
	int name_encoding ;
} FunctionData ;

typedef struct ComplexFunction
{
	unsigned long   magic;
	char           *name ;

	FunctionData   *items;
	unsigned int    items_num;
}ComplexFunction;

typedef struct {
		char                 *filename;         /* we always read filename from config !! */
		struct ASImage       *image;
		int     							loadCount;
}MinipixmapData;

typedef struct MenuDataItem
  {

	unsigned long   magic;
	struct MenuDataItem  *next;  /* next menu item */
	struct MenuDataItem  *prev;  /* prev menu item */

/* same flags are used for both MenuDataItem and MenuData */

#define MD_Disabled        		(0x01<<0)
#define MD_ScaleMinipixmapDown 	(0x01<<1)
#define MD_ScaleMinipixmapUp  	(0x01<<2)
#define MD_NameIsUTF8		  	(0x01<<16)
#define MD_CommentIsUTF8	  	(0x01<<17)
/* can't think of anything else atm - maybe add something later ? */

	ASFlagType            flags ;
	struct FunctionData  *fdata ;
	MinipixmapData minipixmap[MINIPIXMAP_TypesNum];
	
	char                 *preview ;         /* we always read filename from config !! */
	struct ASImage       *preview_image ;
	char  *item;         /* the character string displayed on left */
	char  *item2;		 /* the character string displayed on right */
	char  *comment ;

	time_t                last_used_time ;
}MenuDataItem;

typedef struct MenuData
{
	unsigned long    magic;
	struct MenuDataItem *first; /* first item in menu */
	struct MenuDataItem *last;  /* last item in menu */
	short items_num;        /* number of items in the menu */

	ASFlagType            flags ; 
	char  *name;         /* name of root */
	char  *comment ;
	int    recent_items ;   /*  negative value indicates that default, 
								set in feel should be used */

}MenuData;

#define MAX_MENU_ITEM_HEIGHT    (ASDefaultScrHeight>>4)
#define MAX_MENU_WIDTH          (ASDefaultScrWidth>>1)
#define MAX_MENU_HEIGHT         ((ASDefaultScrHeight*9)/10)
#define MIN_MENU_X              5
#define MAX_MENU_X              (ASDefaultScrWidth-5)
#define MIN_MENU_Y              5
#define MAX_MENU_Y              (ASDefaultScrHeight-5)
#define DEFAULT_ARROW_SIZE      (ASDefaultScrWidth>>7)
#define DEFAULT_SCROLLBAR_SIZE  (ASDefaultScrWidth>>6)

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
int txt2func (const char *text, FunctionData * fdata, int quiet);
int parse_func (const char *text, FunctionData * data, int quiet);
FunctionData *String2Func ( const char *string, FunctionData *p_fdata, Bool quiet );

/* changes function meaning in the parsing table, has no effect on already parsed function data */
FunctionCode change_func_code (const char *func_name, FunctionCode new_code);

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
void destroy_menu_data( MenuData **pmd );
void init_list_of_menus ( ASHashTable **list, Bool force);
MenuData *create_menu_data( char *name );
MenuData *new_menu_data ( ASHashTable *list, char *name );
MenuData *find_menu_data( ASHashTable *list, char *name );
MenuDataItem *new_menu_data_item( MenuData *menu );
int parse_menu_item_name (MenuDataItem * item, char **name);

Bool check_fdata_availability( FunctionData *fdata );

MenuDataItem * add_menu_fdata_item( MenuData *menu, FunctionData *fdata, MinipixmapData *minipixmaps);
MenuDataItem * menu_data_item_from_func (MenuData * menu, FunctionData * fdata, Bool do_check_availability);
struct ASImage *check_scale_menu_pmap( struct ASImage *im, ASFlagType flags ); 
void free_menu_pmaps( MenuData *menu);
void load_menuitem_pmap (MenuDataItem *mdi, MinipixmapTypes type, Bool force);
void reload_menu_pmaps( MenuData *menu, Bool force  );

/* usefull to qsort array of FunctionData pointers : */
int compare_func_data_name(const void *a, const void *b);

Bool is_web_background (FunctionData *fdata);


void print_func_data(const char *file, const char *func, int line, FunctionData *data);
char *format_FunctionData (FunctionData *data);

#define MenuDataItemFromFunc(m,f) menu_data_item_from_func((m),(f), False)



#ifdef __cplusplus
}
#endif



#endif /* _FUNCTIONS_ */
