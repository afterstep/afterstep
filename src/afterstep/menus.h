#ifndef _MENUS_
#define _MENUS_

#define MAX_FILE_SIZE 4096	/* max chars to read from file for cut */

#include "../../libAfterStep/functions.h"

struct ASEvent;
struct TermDef;
struct dirtree_t;
struct ASCanvas;
struct ASTBarData;
struct ASWindow;

typedef struct ASMenuItem
{
#define AS_MenuItemDisabled     (0x01<<0)
#define AS_MenuItemFirst        (0x01<<1)
#define AS_MenuItemLast         (0x01<<2)
#define AS_MenuItemSubitem      (0x01<<3)
#define AS_MenuItemHasSubmenu   (0x01<<4)
    ASFlagType flags;
    struct ASTBarData *bar;
    struct ASImage    *icon;
    struct FunctionData fdata;
	char first_sym ;
    /* we don't really use this one except to set up last use time when item is selected :*/
    struct MenuDataItem *source ;
}ASMenuItem;

typedef struct ASMenu
{
    unsigned long magic ;

#define AS_MenuRendered		(0x01<<0)
#define AS_MenuFocused		(0x01<<1)
#define AS_MenuPinned		(0x01<<2)
#define AS_MenuBalloonShown	(0x01<<3)
#define AS_MenuNameIsUTF8	(0x01<<4)
#define AS_MenuTitleIsUTF8	(0x01<<5)
	ASFlagType state ; 
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
    unsigned int icon_space, arrow_space ;
	unsigned int scroll_bar_size ;

    struct ASWindow *owner;

	struct ASTBarData *scroll_up_bar, *scroll_down_bar;

    Window client_window;

	ASBalloon *comment_balloon ;
	ASBalloon *item_balloon ;
	
	MenuData  *volitile_menu_data ; /* will be destroyed upon menu being closed */
}ASMenu;


/*************************************************************************/
/* Function prototypes :                                                 */
/*************************************************************************/
char* parse_context (char *string, int *output, struct charstring *table);
char scan_for_hotkey (char* txt);

/* the following two returns pointer to static preinitialized memory!!! */
struct TermDef* txt2fterm( const char* txt, int quiet );
struct TermDef*  func2fterm( FunctionCode func, int quiet );
int txt2func( const char* text, FunctionData* fdata, int quiet );

int parse_func( const char* text, FunctionData* data, int quiet );
int free_func_data( FunctionData* data );

ComplexFunction *find_complex_func( struct ASHashTable *list, char *name );

void ExecuteFunction (struct FunctionData *, struct ASEvent *, int);
void FocusOn (struct ASWindow *, int, Bool);

MenuData* FindPopup( const char* name, int quiet );

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

MenuData *dirtree_make_menu2 (struct dirtree_t* tree, char* buf, Bool reload_submenus);
MenuData *update_windowList (void);


/*************************************************************************/
/* menu execution code :                                                 */
/*************************************************************************/
void run_menu( const char *name, Window client_window  );
ASMenu *run_menu_data( MenuData *md );
ASMenu *run_submenu( ASMenu *supermenu, MenuData *md, int x, int y );
ASMenu *find_asmenu( const char *name );
void pin_asmenu( ASMenu *menu );
Bool is_menu_pinnable( ASMenu *menu );


#endif /* _MENUS_ */
