/**********************************************************************
 *
 * Codes for afterstep builtins
 *
 **********************************************************************/

#ifndef _FUNCTIONS_
#define _FUNCTIONS_

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
#ifndef NO_WINDOWLIST
  F_WINDOWLIST,
#endif
  F_POPUP,
  F_FUNCTION,
#ifndef NO_TEXTURE
  F_MINIPIXMAP,
#endif
  F_EXEC,
  F_MODULE,
  F_KILLMODULEBYNAME,
  F_QUICKRESTART,
  F_CHANGE_BACKGROUND,
  F_CHANGE_LOOK,
  F_CHANGE_FEEL,
  F_ENDFUNC,
  F_ENDPOPUP,

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
  F_GETHELP,
  F_PASTE_SELECTION,
  F_CHANGE_WINDOWS_DESK,
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
  F_FUNCTIONS_NUM
} FunctionCode ;

#define IsWindowFunc(f)  ((f)>F_WINDOW_FUNC_START&&(f)<F_MODULE_FUNC_START)
#define IsModuleFunc(f)  ((f)>F_MODULE_FUNC_START&&(f)<F_INTERNAL_FUNC_START)
#define IsInternFunc(f)  ((f)>F_INTERNAL_FUNC_START&&(f)<F_FUNCTIONS_NUM)
#define IsValidFunc(f)   ((f)>=0&&(f)<F_FUNCTIONS_NUM)

#define UP 1
#define DOWN 0
void ChangeWarpIndex(const long, FunctionCode);

#define MAX_FUNC_ARGS	2

typedef struct FunctionData
{
    int func;		/* AfterStep built in function */
    long func_val[MAX_FUNC_ARGS];
    long unit_val[MAX_FUNC_ARGS];
    char unit[MAX_FUNC_ARGS] ;
#define APPLY_VALUE_UNIT(size,value,unit) ((((unit)>0)?(value)*(unit):(value)*(size))/100)
    char* name ;
    char hotkey ;
    char* text ;
    void* popup ; /* actually a MenuRoot pointer */
} FunctionData ;

#endif /* _FUNCTIONS_ */
