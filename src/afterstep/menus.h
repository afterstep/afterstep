#ifndef _MENUS_
#define _MENUS_

#define MAX_FILE_SIZE 4096	/* max chars to read from file for cut */

#include "../../include/functions.h"

/* using long type here as we can get large numbers in
   commands from modules (SET_MASK in particular) */
typedef long val_type ;

typedef struct MenuItem
  {
    struct MenuItem *next;	/* next menu item */
    struct MenuItem *prev;	/* prev menu item */
    char *item;			/* the character string displayed on left */
    char *item2;		/* the character string displayed on right */
    char *action;		/* action to be performed */
    short item_num;		/* item number of this menu */
    short x;			/* x coordinate for text (item) */
    short x2;			/* x coordinate for text (item2) */
    short y_offset;		/* y coordinate for item */
    short y_height;		/* y height for item */
    FunctionCode func;		/* AfterStep built in function */
    val_type val1;		/* values needed for F_SCROLL */
    val_type val2;
    val_type val1_unit;		/* units for val1, val2 */
    val_type val2_unit;		/* pixels (unit=1) or percent of screen 
				 * (unit = Scr.MyDisplayWidth/Height */
    Bool is_hilited;		/* is the item selected? */
    short strlen;		/* strlen(item) */
    short strlen2;		/* strlen(item2) */
    char  hotkey;		/* Hot key */
    struct MenuRoot *menu;	/* sub-menu */
    MyIcon icon;		/* mini icon displayed on left */
  }
MenuItem;

typedef struct MenuRoot
  {
    struct MenuItem *first;	/* first item in menu */
    struct MenuItem *last;	/* last item in menu */
    struct MenuRoot *next;	/* next in list of root menus */
    char *name;			/* name of root */
    Window w;			/* the window of the menu */
    ASWindow *aw;		/* the AfterStep frame */
    short height;		/* height of the menu */
    short width;		/* width of the menu for 1st col */
    short width2;		/* width of the menu for 2nd col */
    int x, y;			/* location of menu when mapped */
    short items;		/* number of items in the menu */
    int context;		/* what context was this menu opened under? */
    Bool is_mapped;		/* is menu mapped? */
    Bool is_transient;		/* should menu be deleted when taken down? */
    Bool is_pinned;		/* is menu stuck to desktop? */
#ifndef NO_TEXTURE
    Pixmap titlebg, itembg;	/* background pixmap for title and others */
    Pixmap itemhibg;
#endif
  }
MenuRoot;

typedef struct MouseButton
  {
    FunctionCode func;		/* AfterStep built in function */
    MenuRoot *menu;		/* menu if func is F_POPUP */
    char* action;		/* action to perform if func != F_POPUP */
    int Button;
    int Context;
    int Modifier;
    val_type val1;
    val_type val2;
    val_type val1_unit;		/* units for val1, val2 */
    val_type val2_unit;		/* pixels (unit=1) or percent of screen 
				 * (unit = Scr.MyDisplayWidth/Height */
    struct MouseButton *NextButton;
  }
MouseButton;

typedef struct FuncKey
  {
    struct FuncKey *next;	/* next in the list of function keys */
    char *name;			/* key name */
    KeyCode keycode;		/* X keycode */
    int cont;			/* context */
    int mods;			/* modifiers */
    FunctionCode func;		/* AfterStep built in function */
    val_type val1;		/* values needed for F_SCROLL */
    val_type val2;
    val_type val1_unit;		/* units for val1, val2 */
    val_type val2_unit;		/* pixels (unit=1) or percent of screen  */
    MenuRoot *menu;		/* menu if func is F_POPUP */
    char *action;		/* action string (if any) */
  }
FuncKey;

#define MENU_ERROR -1
#define MENU_NOP 0
#define MENU_DONE 1
#define SUBMENU_DONE 2

/* Types of events for the FUNCTION builtin */
#define MOTION 'm'
#define IMMEDIATE 'i'
#define CLICK 'c'
#define DOUBLE_CLICK 'd'
#define TRIPLE_CLICK 't'
#define ONE_AND_A_HALF_CLICKS 'o'
#define TWO_AND_A_HALF_CLICKS 'h'

struct charstring
  {
    char key;
    int value;
  };

struct term_definition ;

char* parse_context (char *string, int *output, struct charstring *table);
char scan_for_hotkey (char* txt);

void init_func_data( FunctionData* data );
void parse_func_units( FunctionData* data );
void free_func_hash();

/* the following two returns pointer to static preinitialized memory!!! */
struct term_definition* txt2fterm( const char* txt, int quiet );
struct term_definition* func2fterm( FunctionCode func, int quiet );
int txt2func( const char* text, FunctionData* fdata, int quiet );

int parse_func( const char* text, FunctionData* data, int quiet );
int free_func_data( FunctionData* data );

void ExecuteFunction (FunctionCode, char *, Window, ASWindow *, XEvent *,
			     unsigned long, long, long, int, int,
			     struct MenuRoot *, int);
void FocusOn (ASWindow *, int, Bool);

MenuRoot* FindPopup( char* name, int quiet );

MenuItem* CreateMenuItem();
MenuItem* NewMenuItem(MenuRoot* menu);
void DeleteMenuItem( MenuItem* item );

MenuRoot *CreateMenuRoot();
MenuRoot *NewMenuRoot (char *name);
void DeleteMenuRoot (MenuRoot * menu);

void MenuItemFromFunc( MenuRoot* menu, FunctionData* fdata );
MenuItem* MenuItemParse(MenuRoot* menu, const char* buf);

int ParseMenuBody (MenuRoot *mr, FILE * fd);
void ParsePopupEntry (char *tline, FILE * fd, char **junk, int *junk2);
void ParseMouseEntry (char *tline, FILE * fd, char **junk, int *junk2);
void ParseKeyEntry (char *tline, FILE * fd, char **junk, int *junk2);

struct dirtree_t;
MenuRoot *dirtree_make_menu2 (struct dirtree_t* tree, char* buf);

void MakeMenu (MenuRoot * mr);

int RunCommand( char* cmd, int channel, Window w );

void do_menu(MenuRoot * menu, MenuRoot * parent);
Bool configure_menu(MenuRoot * menu, MenuRoot * parent);
void setup_menu_pixmaps(MenuRoot * menu);
Bool HandleMenuEvent (MenuRoot * menu, XEvent * event);
Bool map_menu(MenuRoot * menu, int context);
Bool unmap_menu (MenuRoot * menu);
Bool pin_menu(MenuRoot * menu);
void move_menu (MenuRoot * menu, int x, int y);

void PaintEntry (MenuRoot *, MenuItem *);
void PaintMenu (MenuRoot *, XEvent *);

void PopDownMenu (MenuRoot *);
Bool PopUpMenu (MenuRoot *, int, int);

MenuRoot *update_windowList (void);

#endif /* _MENUS_ */
