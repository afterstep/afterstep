#ifndef _MENUS_
#define _MENUS_

#define MAX_FILE_SIZE 4096	/* max chars to read from file for cut */

#include "../../include/functions.h"

struct ASEvent;
struct TermDef;
struct dirtree_t;
struct ASCanvas;
struct ASTBarData;
struct ASWindow;

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
    char *item;         /* the character string displayed on left */
    char *item2;		/* the character string displayed on right */
}MenuDataItem;

typedef struct MenuData
{
    unsigned long    magic;
    char *name;         /* name of root */

    struct MenuDataItem *first; /* first item in menu */
    struct MenuDataItem *last;  /* last item in menu */
    short items_num;        /* number of items in the menu */
}MenuData;

typedef struct ASMenuItem
{
#define AS_MenuItemDisabled     (0x01<<0)
    ASFlagType flags;
    struct ASTBarData *bar;
    struct MenuData   *submenu;
}ASMenuItem;

typedef struct ASMenu
{
    unsigned long magic ;
    char  *name ;                              /* the name of the popup */
    char  *title ;                             /* text of the first F_TITLE item */
    struct ASCanvas *main_canvas;

    unsigned int items_num ;
    ASMenuItem   *items ;

    unsigned int item_width, item_height ;
    unsigned int top_item;
    int selected_item ;                        /* if -1 - then none is selected */
    int pressed_item ;                         /* if -1 - then none is pressed */
    struct ASMenu *supermenu;                  /* upper level menu */
    struct ASMenu *submenu;                    /* lower level menu */

    unsigned int visible_items_num ;

    unsigned int optimal_width, optimal_height;

    struct ASWindow *owner;
}ASMenu;

#define MAX_MENU_ITEM_HEIGHT    (Scr.MyDisplayHeight>>4)
#define MAX_MENU_WIDTH          (Scr.MyDisplayWidth>>1)
#define MAX_MENU_HEIGHT         ((Scr.MyDisplayHeight*3)/4)
#define MIN_MENU_X              5
#define MAX_MENU_X              (Scr.MyDisplayWidth-5)
#define MIN_MENU_Y              5
#define MAX_MENU_Y              (Scr.MyDisplayHeight-5)



/*************************************************************************/
/* Function prototypes :                                                 */
/*************************************************************************/
char* parse_context (char *string, int *output, struct charstring *table);
char scan_for_hotkey (char* txt);

void init_func_data( FunctionData* data );
void parse_func_units( FunctionData* data );
void free_func_hash();

/* the following two returns pointer to static preinitialized memory!!! */
struct TermDef* txt2fterm( const char* txt, int quiet );
struct TermDef*  func2fterm( FunctionCode func, int quiet );
int txt2func( const char* text, FunctionData* fdata, int quiet );

int parse_func( const char* text, FunctionData* data, int quiet );
int free_func_data( FunctionData* data );

ComplexFunction *find_complex_func( struct ASHashTable *list, char *name );

void ExecuteFunction (struct FunctionData *, struct ASEvent *, int);
void FocusOn (ASWindow *, int, Bool);

MenuData* FindPopup( const char* name, int quiet );

MenuDataItem* CreateMenuItem();
MenuDataItem* NewMenuItem(MenuData* menu);
void DeleteMenuItem( MenuDataItem* item );

MenuData *CreateMenuData();
MenuData *NewMenuData (char *name);
void DeleteMenuData (MenuData * menu);

void MenuItemFromFunc( MenuData* menu, FunctionData* fdata );

int ParseMenuBody (MenuData *mr, FILE * fd);
void ParseMenuEntry (char *tline, FILE * fd, char **junk, int *junk2);
void ParseFunctionEntry (char *tline, FILE * fd, char **junk, int *junk2);

void ParseMouseEntry (char *tline, FILE * fd, char **junk, int *junk2);
void ParseKeyEntry (char *tline, FILE * fd, char **junk, int *junk2);

MenuData *dirtree_make_menu2 (struct dirtree_t* tree, char* buf);
MenuData *update_windowList (void);


/*************************************************************************/
/* menu execution code :                                                 */
/*************************************************************************/
void run_menu( const char *name );
ASMenu *run_submenu( ASMenu *supermenu, MenuData *md, int x, int y );


#endif /* _MENUS_ */
