/****************************************************************************
 *
 * Copyright (c) 1999 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/

#define LOCAL_DEBUG
#include "../configure.h"

#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#include "asapp.h"
#include "afterstep.h"
#include "parser.h"
#include "screen.h"
#include "functions.h"
#include "session.h"
#include "balloon.h"
#include "mystyle.h"
#include "wmprops.h"
#include "../libAfterImage/xpm.h"
#include "../libAfterImage/char2uni.h"


ASProgArgs    as_app_args = {0, NULL, 0, NULL, NULL, NULL, 0, 0};/* some typical progy cmd line options - set by SetMyArgs( argc, argv )*/
ASProgArgs	 *MyArgsPtr = &as_app_args ; 
char         *MyName = NULL;                    /* name are we known by */
char          MyClass[MAX_MY_CLASS+1]="unknown";/* application Class name ( Pager, Wharf, etc. ) - set by SetMyClass(char *) */
void        (*MyVersionFunc)   (void) = NULL;
void        (*MyUsageFunc)   (void) = NULL;

char     *as_afterstep_dir_name = AFTER_DIR;
char     *as_save_dir_name      = AFTER_DIR "/" AFTER_SAVE;
char     *as_start_dir_name     = AFTER_DIR "/" START_DIR;
char     *as_share_dir_name     = AFTER_SHAREDIR;

char     *as_background_dir_name = BACK_DIR ;
char     *as_look_dir_name = LOOK_DIR ;
char     *as_theme_dir_name = THEME_DIR ;
char     *as_theme_file_dir_name = THEME_FILE_DIR ;
char     *as_feel_dir_name = FEEL_DIR ;
char     *as_colorscheme_dir_name = COLORSCHEME_DIR;
char     *as_font_dir_name = FONT_DIR ;
char     *as_icon_dir_name = ICON_DIR ;
char     *as_tile_dir_name = TILE_DIR ;


int       fd_width;

unsigned int  nonlock_mods = 0;				   /* a mask for non-locking modifiers */
unsigned int  lock_mods[MAX_LOCK_MODS] = {0};  /* all combinations of lock modifier masks */
/* Now for each display we may have one or several screens ; */
ScreenInfo    *ASDefaultScr;					   /* ScreenInfo for the default screen */
//#define Scr	(*DefaultScr);
int x_fd = 0;                                  /* descriptor of the X Windows connection  */


int SingleScreen = -1 ;						   /* if >= 0 then [points to the only ScreenInfo structure available */
int PointerScreen = 0;						   /* screen that currently has pointer */
unsigned int  NumberOfScreens = 0; 			   /* number of screens on display */
/* unused - future development : */
ScreenInfo  **all_screens = NULL ;             /* all ScreenInfo structures for NumberOfScreens screens */
ASHashTable  *screens_window_hash =NULL	;	   /* so we can easily track what window is on what screen */
/* end of: unused - future development : */


struct ASFeel *DefaultFeel = NULL;/* unused - future development : */
struct MyLook *DefaultLook = NULL;/* unused - future development : */

void (*CloseOnExec)() = NULL ;

struct ASSession *Session = NULL;          /* filenames of look, feel and background */
struct ASEnvironment *Environment = NULL;

struct ASCategoryTree *StandardCategories = NULL ;
struct ASCategoryTree *KDECategories = NULL ;
struct ASCategoryTree *GNOMECategories = NULL ;
struct ASCategoryTree *SystemCategories = NULL ;
struct ASCategoryTree *CombinedCategories = NULL ;



/* names of AS functions - used all over the place  :*/

#define FUNC_TERM(txt,len,func)         {TF_NO_MYNAME_PREPENDING,txt,len,TT_FUNCTION,func,NULL}
#define FUNC_TERM2(flags,txt,len,func)  {TF_NO_MYNAME_PREPENDING|(flags),txt,len,TT_FUNCTION,func,NULL}

TermDef       FuncTerms[F_FUNCTIONS_NUM + 1] = {
	FUNC_TERM2 (NEED_NAME, "Nop", 3, F_NOP),   /* Nop      "name"|"" */
	FUNC_TERM2 (NEED_NAME, "Title", 5, F_TITLE),	/* Title    "name"    */
	FUNC_TERM ("Beep", 4, F_BEEP),			   /* Beep               */
	FUNC_TERM ("Quit", 4, F_QUIT),			   /* Quit     ["name"] */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "Restart", 7, F_RESTART),	/* Restart "name" WindowManagerName */
	FUNC_TERM ("Refresh", 7, F_REFRESH),	   /* Refresh  ["name"] */
#ifndef NO_VIRTUAL
	FUNC_TERM2 (USES_NUMVALS, "Scroll", 6, F_SCROLL),	/* Scroll     horiz vert */
	FUNC_TERM2 (USES_NUMVALS, "GotoPage", 8, F_GOTO_PAGE),	/* GotoPage   x     y    */
	FUNC_TERM ("TogglePage", 10, F_TOGGLE_PAGE),	/* TogglePage ["name"]   */
#endif
	FUNC_TERM2 (USES_NUMVALS, "CursorMove", 10, F_MOVECURSOR),	/* CursorMove horiz vert */
	FUNC_TERM2 (NEED_WINIFNAME, "WarpFore", 8, F_WARP_F),	/* WarpFore ["name" window_name] */
	FUNC_TERM2 (NEED_WINIFNAME, "WarpBack", 8, F_WARP_B),	/* WarpBack ["name" window_name] */
	FUNC_TERM2 (NEED_NAME | NEED_WINDOW, "Wait", 4, F_WAIT),	/* Wait      "name" window_name  */
	FUNC_TERM2 (USES_NUMVALS, "Desk", 4, F_DESK),	/* Desk arg1 [arg2] */
	FUNC_TERM2 (USES_NUMVALS, "GotoDeskViewport", 16, F_GOTO_DESKVIEWPORT),	/* GotoDeskViewport DESK+VX+VY */
#ifndef NO_WINDOWLIST
	FUNC_TERM2 (USES_NUMVALS, "WindowList", 10, F_WINDOWLIST),	/* WindowList [arg1 arg2] */
#endif
	FUNC_TERM ("StopModuleList", 14, F_STOPMODULELIST),	/* StopModuleList "name" */
	FUNC_TERM ("RestartModuleList", 17, F_RESTARTMODULELIST),	/* RestartModuleList "name" */
	FUNC_TERM2 (NEED_NAME, "PopUp", 5, F_POPUP),	/* PopUp    "popup_name" [popup_name] */
	FUNC_TERM2 (NEED_NAME, "Function", 8, F_FUNCTION),	/* Function "function_name" [function_name] */
	FUNC_TERM ("MiniPixmap", 10, F_MINIPIXMAP),	/* MiniPixmap "name" */
	FUNC_TERM ("SmallMiniPixmap", 15, F_SMALL_MINIPIXMAP),	/* SmallMiniPixmap "name" */
	FUNC_TERM ("LargeMiniPixmap", 15, F_SMALL_MINIPIXMAP),	/* LargeMiniPixmap "name" */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "Exec", 4, F_EXEC),	/* Exec   "name" command */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "Module", 6, F_MODULE),	/* Module "name" command */
    FUNC_TERM2 (NEED_NAME | NEED_CMD, "ExecInTerm", 10, F_ExecInTerm),   /* ExecInTerm   "name" command */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "KillModuleByName", 16, F_KILLMODULEBYNAME),	/* KillModuleByName "name" module */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "RestartModuleByName", 19, F_RESTARTMODULEBYNAME),	/* RestartModuleByName "name" module */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "QuickRestart", 12, F_QUICKRESTART),	/* QuickRestart "name" what */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "Background", 10, F_CHANGE_BACKGROUND),	/* Background "name" file_name */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "ChangeLook", 10, F_CHANGE_LOOK),	/* ChangeLook "name" file_name */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "ChangeFeel", 10, F_CHANGE_FEEL),	/* ChangeFeel "name" file_name */
    FUNC_TERM2 (NEED_NAME | NEED_CMD, "ChangeTheme", 11, F_CHANGE_THEME), /* ChangeTheme "name" file_name */
    FUNC_TERM2 (NEED_NAME | NEED_CMD, "ChangeThemeFile", 15, F_CHANGE_THEME_FILE), /* "name" file_name */
    FUNC_TERM2 (NEED_NAME | NEED_CMD, "ChangeColorscheme", 17, F_CHANGE_COLORSCHEME), /* ChangeColorscheme "name" file_name */
    FUNC_TERM2 (NEED_NAME | NEED_CMD, "InstallLook", 11, F_INSTALL_LOOK), /*  "name" file_name */
    FUNC_TERM2 (NEED_NAME | NEED_CMD, "InstallFeel", 11, F_INSTALL_FEEL), /*  "name" file_name */
    FUNC_TERM2 (NEED_NAME | NEED_CMD, "InstallBackground", 17, F_INSTALL_BACKGROUND), /* "name" file_name */
    FUNC_TERM2 (NEED_NAME | NEED_CMD, "InstallFont", 11, F_INSTALL_FONT), /* "name" file_name */
    FUNC_TERM2 (NEED_NAME | NEED_CMD, "InstallIcon", 11, F_INSTALL_ICON), /*  "name" file_name */
    FUNC_TERM2 (NEED_NAME | NEED_CMD, "InstallTile", 11, F_INSTALL_TILE), /*  "name" file_name */
    FUNC_TERM2 (NEED_NAME | NEED_CMD, "InstallThemeFile", 12, F_INSTALL_THEME_FILE), /*  "name" file_name */
    FUNC_TERM2 (NEED_NAME | NEED_CMD, "InstallColorscheme", 14, F_INSTALL_COLORSCHEME), /*  "name" file_name */
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "SaveWorkspace", 13, F_SAVE_WORKSPACE), /* SaveWorkspace "name" file_name */
	FUNC_TERM2 (TF_SYNTAX_TERMINATOR, "EndFunction", 11, F_ENDFUNC),
	FUNC_TERM2 (TF_SYNTAX_TERMINATOR, "EndPopup", 8, F_ENDPOPUP),
	FUNC_TERM ("TakeScreenShot", 14, F_TAKE_SCREENSHOT),
	FUNC_TERM2 (NEED_NAME | NEED_CMD, "Test", 4, F_Test),

	/* this functions require window as aparameter */
	FUNC_TERM ("&nonsense&", 10, F_WINDOW_FUNC_START),	/* not really a command */
	FUNC_TERM2 (USES_NUMVALS, "Move", 4, F_MOVE),	/* Move     ["name"] [whereX whereY] */
	FUNC_TERM2 (USES_NUMVALS, "Resize", 6, F_RESIZE),	/* Resize   ["name"] [toWidth toHeight] */
	FUNC_TERM ("Raise", 5, F_RAISE),		   /* Raise    ["name"] */
	FUNC_TERM ("Lower", 5, F_LOWER),		   /* Lower    ["name"] */
	FUNC_TERM ("RaiseLower", 10, F_RAISELOWER),	/* RaiseLower ["name"] */
	FUNC_TERM ("PutOnTop", 8, F_PUTONTOP),	   /* PutOnTop  */
	FUNC_TERM ("PutOnBack", 9, F_PUTONBACK),   /* PutOnBack */
	FUNC_TERM2 (USES_NUMVALS, "SetLayer", 8, F_SETLAYER),	/* SetLayer    layer */
	FUNC_TERM2 (USES_NUMVALS, "ToggleLayer", 11, F_TOGGLELAYER),	/* ToggleLayer layer1 layer2 */
	FUNC_TERM ("Shade", 5, F_SHADE),		   /* Shade    ["name"] */
	FUNC_TERM ("Delete", 6, F_DELETE),		   /* Delete   ["name"] */
	FUNC_TERM ("Destroy", 7, F_DESTROY),	   /* Destroy  ["name"] */
	FUNC_TERM ("Close", 5, F_CLOSE),		   /* Close    ["name"] */
	FUNC_TERM ("Iconify", 7, F_ICONIFY),	   /* Iconify  ["name"] value */
	FUNC_TERM2 (USES_NUMVALS, "Maximize", 8, F_MAXIMIZE),	/* Maximize ["name"] [hori vert] */
	FUNC_TERM ("Stick", 5, F_STICK),		   /* Stick    ["name"] */
	FUNC_TERM ("Focus", 5, F_FOCUS),		   /* Focus */
	FUNC_TERM2 (NEED_WINIFNAME, "ChangeWindowUp", 14, F_CHANGEWINDOW_UP),	/* ChangeWindowUp   ["name" window_name ] */
	FUNC_TERM2 (NEED_WINIFNAME, "ChangeWindowDown", 16, F_CHANGEWINDOW_DOWN),	/* ChangeWindowDown ["name" window_name ] */
    FUNC_TERM2 (NEED_WINIFNAME, "GoToBookmark", 12, F_GOTO_BOOKMARK),   /* GoToBookmark ["name" window_bookmark ] */
	FUNC_TERM ("GetHelp", 7, F_GETHELP),	   /* */
	FUNC_TERM ("PasteSelection", 14, F_PASTE_SELECTION),	/* */
	FUNC_TERM2 (USES_NUMVALS, "WindowsDesk", 11, F_CHANGE_WINDOWS_DESK),	/* WindowDesk "name" new_desk */
    FUNC_TERM ("BookmarkWindow", 14, F_BOOKMARK_WINDOW),    /* BookmarkWindow "name" new_bookmark */
    FUNC_TERM ("PinMenu", 7, F_PIN_MENU),    /* PinMenu ["name"] */
	FUNC_TERM ("TakeWindowShot", 14, F_TAKE_WINDOWSHOT),
	FUNC_TERM ("TakeFrameShot", 13, F_TAKE_FRAMESHOT),
	FUNC_TERM ("SwallowWindow", 13, F_SWALLOW_WINDOW),  /* SwallowWindow "name" module_name */ 
	/* end of window functions */
	/* these are commands  to be used only by modules */
	FUNC_TERM ("&nonsense&", 10, F_MODULE_FUNC_START),	/* not really a command */
	FUNC_TERM ("Send_WindowList", 15, F_SEND_WINDOW_LIST),	/* */
	FUNC_TERM ("SET_MASK", 8, F_SET_MASK),	   /* SET_MASK  mask lock_mask */
    FUNC_TERM2 (NEED_NAME, "SET_NAME", 8, F_SET_NAME),   /* SET_NAME  name */
	FUNC_TERM ("UNLOCK", 6, F_UNLOCK),		   /* UNLOCK    1  */
	FUNC_TERM ("SET_FLAGS", 9, F_SET_FLAGS),   /* SET_FLAGS flags */
	/* these are internal commands */
	FUNC_TERM ("&nonsense&", 10, F_INTERNAL_FUNC_START),	/* not really a command */
	FUNC_TERM ("&raise_it&", 10, F_RAISE_IT),  /* should not be used by user */
    /* wharf functions : */
    {TF_NO_MYNAME_PREPENDING, "Folder", 6, TT_FUNCTION, F_Folder, NULL},
    {TF_NO_MYNAME_PREPENDING | NEED_NAME | NEED_CMD, "Swallow", 7, TT_FUNCTION, F_Swallow, NULL},
    {TF_NO_MYNAME_PREPENDING | NEED_NAME | NEED_CMD, "MaxSwallow", 10, TT_FUNCTION, F_MaxSwallow, NULL},
    {TF_NO_MYNAME_PREPENDING | NEED_NAME | NEED_CMD, "SwallowModule", 13, TT_FUNCTION, F_SwallowModule, NULL},
    {TF_NO_MYNAME_PREPENDING | NEED_NAME | NEED_CMD, "MaxSwallowModule", 16, TT_FUNCTION, F_MaxSwallowModule, NULL},
    {TF_NO_MYNAME_PREPENDING, "Size", 4, TT_FUNCTION, F_Size, NULL},
    {TF_NO_MYNAME_PREPENDING, "Transient", 9, TT_FUNCTION, F_Transient, NULL},

	{0, NULL, 0, 0, 0}
};

struct SyntaxDef FuncSyntax = {
    '\0',   '\n',	FuncTerms,
	0, 		' ',	"",			"\t",
	"AfterStep Function",
	"Functions",
	"built in AfterStep functions",
	NULL,	0
};
SyntaxDef     PopupFuncSyntax = {
	'\n',	'\0',	FuncTerms,
	0, 		' ',	"\t",		"\t",
    "Popup/Complex function definition",
	"Popup",
	"",	
	NULL,	0
};



struct SyntaxDef     *pFuncSyntax = &FuncSyntax;
struct SyntaxDef     *pPopupFuncSyntax = &PopupFuncSyntax;

TermDef      *
func2fterm (FunctionCode func, int quiet)
{
	register int  i;

	/* in most cases that should work : */
	if( func < F_FUNCTIONS_NUM )
		if (FuncTerms[func].id == func)
			return &(FuncTerms[func]);

	/* trying fallback if it did not : */
	for (i = 0; i < F_FUNCTIONS_NUM; i++)
		if (FuncTerms[i].id == func)
			return &(FuncTerms[i]);

	/* something terribly wrong has happened : */
	return NULL;
}

/************************************************************************************/
/* Command Line Processing/ App initialization here :                               */
/************************************************************************************/
void
SetMyClass (const char *app_class)
{
    if( app_class != NULL )
    {
        strncpy( MyClass, (char*)app_class, MAX_MY_CLASS );
        MyClass[MAX_MY_CLASS] = '\0' ;
    }
}

void
SetMyName (char *argv0)
{
	char         *temp = strrchr (argv0, '/');

	/* Save our program name - for error messages */
	MyName = temp ? temp + 1 : argv0;
	set_application_name(argv0);
}

/* If you change/add options please change InitMyApp below and option flags in aftersteplib.h */

CommandLineOpts as_standard_cmdl_options[STANDARD_CMDL_OPTS_NUM] =
{
#define  SHOW_VERSION   0
#define  SHOW_CONFIG    1
#define  SHOW_USAGE     2
/* 0*/{"v", "version", "Display version information and stop", NULL, handler_show_info, NULL, SHOW_VERSION  },
/* 1*/{"c", "config",  "Display Config information and stop",  NULL, handler_show_info, NULL, SHOW_CONFIG},
/* 2*/{"h", "help",    "Display uasge information and stop",   NULL, handler_show_info, NULL, SHOW_USAGE},
/* 3*/{NULL,"debug",   "Debugging: Run in Synchronous mode",  NULL, handler_set_flag, &(as_app_args.flags), ASS_Debugging},
/* 4*/{"s", "single",  "Run on single screen only", NULL, handler_set_flag, &(as_app_args.flags), ASS_SingleScreen},
/* 5*/{"r", "restart", "Run as if it was restarted","same as regular startup, only \nruns RestartFunctioninstead of InitFunction",
                                                           handler_set_flag, &(as_app_args.flags), ASS_Restarting},
#define OPTION_HAS_ARGS     6
/* 6*/{"d", "display", "Specify what X display we should connect to","Overrides $DISPLAY environment variable",
                                                           handler_set_string, &(as_app_args.display_name), 0, CMO_HasArgs },
/* 7*/{"f", "config-file", "Read all config from requested file","Use it if you want to use .steprc\ninstead of standard config files",
                                                           handler_set_string, &(as_app_args.override_config), 0, CMO_HasArgs },
/* 8*/{"p", "user-dir","Read all the config from requested dir","Use it to override config location\nrequested in compile time" ,
                                                           handler_set_string, &(as_app_args.override_home), 0, CMO_HasArgs },
/* 9*/{"g", "global-dir","Use requested dir as a shared config dir","Use it to override shared config location\nrequested in compile time" ,
                                                           handler_set_string, &(as_app_args.override_share), 0, CMO_HasArgs },
/*10*/{"V", "verbosity-level","Change verbosity of the AfterStep output","0 - will disable any output;\n1 - will allow only error messages;\n5 - both errors and warnings(default)",
                                                           handler_set_int, &(as_app_args.verbosity_level), 0, CMO_HasArgs },
/*11*/{NULL, "window", "Internal Use: Window in which action occured", "interface part which has triggered our startup",
                                                           handler_set_int, &(as_app_args.src_window), 0, CMO_HasArgs },
/*12*/{NULL, "context","Internal Use: Context in which action occured", "interface part which has triggered our startup",
                                                           handler_set_int, &(as_app_args.src_context), 0, CMO_HasArgs },
/*13*/{NULL, "look","Read look config from requested file","Use it if you want to use different look\ninstead of what was selected from the menu",
                                                           handler_set_string, &(as_app_args.override_look), 0, CMO_HasArgs },
/*14*/{NULL, "feel","Read feel config from requested file","Use it if you want to use different feel\ninstead of what was selected from the menu",
                                                           handler_set_string, &(as_app_args.override_feel), 0, CMO_HasArgs },
/*15*/{NULL, "theme","Read theme config from requested file","Use it if you want to use different theme\ninstead of what was selected from the menu",
                                                           handler_set_string, &(as_app_args.override_feel), 0, CMO_HasArgs },
#ifdef DEBUG_TRACE_X
/*16*/{NULL, "trace-func","Debugging: Trace calls to a function with requested name", NULL,
                                                           handler_set_string, &(as_app_args.trace_calls), 0, CMO_HasArgs },
#endif
/*17*/{"l", "log","Save all output into the file", "(instead of printing it to console)",
                                                           handler_set_string, &(as_app_args.log_file), 0, CMO_HasArgs },
/*18*/{"L", "locale","Set language locale", "to be used while displaying text",
                                                           handler_set_string, &(as_app_args.locale), 0, CMO_HasArgs },
/*19*/{NULL, "myname","Overrides module name", "will be used while parsing config files\nand reporting to AfterStep",
                                                           handler_set_string, &(MyName), 0, CMO_HasArgs },
/*20*/{NULL, "geometry","Overrides module's geometry", NULL,
                                                           handler_set_geometry, &(as_app_args.geometry), 0, CMO_HasArgs },
/*21*/{NULL, "gravity","Overrides module's gravity", NULL,
                                                           handler_set_gravity, &(as_app_args.gravity), 0, CMO_HasArgs },
      {NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

void
standard_version (void)
{
    if( MyVersionFunc )
        MyVersionFunc();
    else
	    printf ("%s version %s\n", MyClass, VERSION);
}

void
print_command_line_opt(const char *prompt, CommandLineOpts *options, ASFlagType mask)
{
    register int i ;
    ASFlagType bit = 0x01;

	if( options == NULL ) 
		options = as_standard_cmdl_options ;
    printf ("%s:\n", prompt);

    for( i = 0 ; options[i].handler != NULL ; i++ )
    {
        if( !get_flags( bit, mask ) )
        {
            if( options[i].short_opt )
                printf( OPTION_SHORT_FORMAT, options[i].short_opt );
            else
                printf( OPTION_NOSHORT_FORMAT);

            if( !get_flags( options[i].flags, CMO_HasArgs ) )
                printf( OPTION_DESCR1_FORMAT_NOVAL, options[i].long_opt, options[i].descr1 );
            else
                printf( OPTION_DESCR1_FORMAT_VAL, options[i].long_opt, options[i].descr1 );

            if( options[i].descr2 )
			{
                register char *start = options[i].descr2 ;
				register char *end ;
				do
				{
					end = strchr( start, '\n' );
					if( end == NULL )
						printf(OPTION_DESCR2_FORMAT, start );
					else
					{
						static char buffer[81];
						register int len = (end > start+80 )? 80 : end-start ;
						strncpy( buffer, start, len );
						buffer[len] = '\0' ;
						printf(OPTION_DESCR2_FORMAT, buffer );
		  				start = end+1 ;
					}
				}while( end != NULL );
			}
        }
        bit = bit << 1 ;
    }
}

void
standard_usage()
{
    standard_version();
    if( MyUsageFunc )
        MyUsageFunc();
    else
        printf (OPTION_USAGE_FORMAT "\n", MyName );
    print_command_line_opt("standard_options are :", as_standard_cmdl_options, as_app_args.mask);
}

void  handler_show_info( char *argv, void *trg, long param )
{
    switch( param )
    {
        case SHOW_VERSION :
            standard_version();
            break;
        case SHOW_CONFIG  :
            standard_version();
            printf ("BinDir            %s\n", AFTER_BIN_DIR);
            printf ("ManDir            %s\n", AFTER_MAN_DIR);
            printf ("DocDir            %s\n", AFTER_DOC_DIR);
            printf ("ShareDir          %s\n", AFTER_SHAREDIR);
            printf ("AfterDir          %s\n", AFTER_DIR);
            break;
        case SHOW_USAGE   :
            standard_usage();
            break;
    }
    exit (0);
}
void  handler_set_flag( char *argv, void *trg, long param )
{
    register ASFlagType *f = trg ;
    set_flags( *f, param );
}
void  handler_set_string( char *argv, void *trg, long param )
{
    register char **s = trg;
    if( argv )
        *s = argv;
}
void  handler_set_int( char *argv, void *trg, long param )
{
    register int *i = trg ;
    if( argv )
        *i = atoi(argv);
}

void  handler_set_geometry( char *argv, void *trg, long param )
{
    register ASGeometry *geom = trg ;
    if( argv )
	{
		memset( geom, 0x00, sizeof(ASGeometry));
		parse_geometry (argv, &(geom->x), &(geom->y), &(geom->width), &(geom->height), &(geom->flags) );
	}
}

void  handler_set_gravity( char *argv, void *trg, long param )
{
    register int *i = trg ;
    *i = ForgetGravity ;
	if( argv )
	{

		if( mystrncasecmp( argv, "NorthWest", 9 )== 0 )	  *i = NorthWestGravity ; 
		else if( mystrncasecmp( argv, "SouthWest", 9 )== 0 )	  *i = SouthWestGravity ; 
		else if( mystrncasecmp( argv, "NorthEasth", 10 )== 0 )	*i = NorthEastGravity ; 
		else if( mystrncasecmp( argv, "SouthEast", 9 )== 0 )	  *i = SouthEastGravity ; 
		else if( mystrncasecmp( argv, "Center", 6 )== 0 )	  *i = CenterGravity ; 
		else if( mystrncasecmp( argv, "Static", 6 )== 0 )	  *i = StaticGravity ; 
		else show_warning( "unknown gravity type \"%s\". Use one of: NorthWest, SouthWest, NorthEast, SouthEast, Center, Static", argv );
	}
}


int
match_command_line_opt( char *argvi, CommandLineOpts *options )
{
    register char *ptr = argvi;
    register int opt ;

    if( ptr == NULL )
        return -1;
    if( *ptr == '-' )
    {
        ++ptr;
        if( *ptr == '-' )
        {
            ++ptr;
            for( opt = 0 ; options[opt].handler ; ++opt )
                if( strcmp(options[opt].long_opt, ptr ) == 0 )
                    break;
        }else
        {
            for( opt = 0 ; options[opt].handler ; ++opt )
                if( options[opt].short_opt )
                    if( strcmp(options[opt].short_opt, ptr ) == 0 )
                        break;
        }
    }else
        opt = -1;
    if( options[opt].handler == NULL )
        opt = -1;
    return opt;
}

static DeadPipe_handler _as_dead_pipe_handler = NULL ; 

DeadPipe_handler set_DeadPipe_handler( DeadPipe_handler new_handler ) 
{
	DeadPipe_handler old = _as_dead_pipe_handler ; 
	_as_dead_pipe_handler = new_handler ; 
	return old;
}	 

void ASDeadPipe( int nonsense ) 
{
	if( _as_dead_pipe_handler )
		_as_dead_pipe_handler(nonsense);
	else
		exit (0);
}	 

static char **AS_environ = NULL ; 

void override_environ( char **envp )
{
	AS_environ = envp ; 	
}	 

void
InitMyApp (  const char *app_class, int argc, char **argv, void (*version_func) (void), void (*custom_usage_func) (void), ASFlagType opt_mask )
{
    /* first of all let's set us some nice signal handlers */
#ifdef HAVE_SIGSEGV_HANDLING
    set_signal_handler (SIGSEGV);
#endif

	set_signal_handler (SIGUSR2);
    signal (SIGPIPE, ASDeadPipe);        /* don't forget DeadPipe should be provided by the app */

    SetMyClass( app_class );
    MyVersionFunc = version_func ;
    MyUsageFunc = custom_usage_func ;

    memset( &as_app_args, 0x00, sizeof(ASProgArgs) );
	as_app_args.locale = mystrdup(AFTER_LOCALE);
	if (as_app_args.locale[0] == '\0')
	{
		free( as_app_args.locale );
		as_app_args.locale = mystrdup(getenv("LANG"));
	}

    as_app_args.mask = opt_mask ;
#ifndef NO_DEBUG_OUTPUT
    as_app_args.verbosity_level = OUTPUT_VERBOSE_THRESHOLD ;
#else
    as_app_args.verbosity_level = OUTPUT_DEFAULT_THRESHOLD ;
#endif


    ASDefaultScr = safecalloc(1, sizeof(ScreenInfo));

    if( argc > 0 && argv )
    {
        int i ;

		as_app_args.saved_argc = argc ;
		as_app_args.saved_argv = safecalloc( argc, sizeof(char*));
		for( i = 0 ; i < argc ; ++i )
			as_app_args.saved_argv[i] = argv[i] ;

        SetMyName( argv[0] );

        for( i = 1 ; i < argc ; i++ )
        {
            register int opt ;
            if( (opt = match_command_line_opt( &(argv[i][0]), as_standard_cmdl_options )) < 0 )
                continue;
            if( get_flags( (0x01<<opt), as_app_args.mask) )
                continue;
            if( get_flags( as_standard_cmdl_options[opt].flags, CMO_HasArgs ) )
            {
				argv[i] = NULL ;
                if( ++i >= argc )
                    continue;
                else
                    as_standard_cmdl_options[opt].handler( argv[i], as_standard_cmdl_options[opt].trg, as_standard_cmdl_options[opt].param );
            }else
                as_standard_cmdl_options[opt].handler( NULL, as_standard_cmdl_options[opt].trg, as_standard_cmdl_options[opt].param );
            argv[i] = NULL ;
        }
    }

    fd_width = get_fd_width ();

    if (FuncSyntax.term_hash == NULL)
		PrepareSyntax (&FuncSyntax);
    set_output_threshold( as_app_args.verbosity_level );
    if(as_app_args.log_file)
		if( freopen( as_app_args.log_file, get_flags( as_app_args.flags, ASS_Restarting)?"a":"w", stderr ) == NULL )
	    	show_system_error( "failed to redirect output into file \"%s\"", as_app_args.log_file );

	if( as_app_args.locale && strlen(as_app_args.locale) > 0)
	{
		as_set_charset( parse_charset_name( as_app_args.locale ));
#ifdef I18N
		if (strlen(as_app_args.locale) > 0)
			if (setlocale (LC_CTYPE, as_app_args.locale) == NULL)
			{
  			    show_error ("unable to set locale");
			}
#endif
	}else
	{
#ifdef I18N
	    show_warning ("LANG environment variable is not set - use -L \"locale\" command line option to define locale");
#endif
	}	

#ifdef DEBUG_TRACE_X
    trace_enable_function(as_app_args.trace_calls);
#endif

	asxml_var_insert("icon.button.width", 64);
	asxml_var_insert("icon.button.height", 64);
	asxml_var_insert("icon.width", 48);
	asxml_var_insert("icon.height", 48);
	asxml_var_insert("minipixmap.width", 24);
	asxml_var_insert("minipixmap.height", 24);
	asxml_var_insert("title.font.size", 14);
	asxml_var_insert("menu.font.size", 16);

}

void
free_func_hash ()
{
	if (FuncSyntax.term_hash)
	{
        FreeSyntaxHash (&FuncSyntax);
	}
}

/*********** end command line parsing **************************/

ASEnvironment *
make_default_environment()
{
	ASEnvironment *e = safecalloc( 1, sizeof(ASEnvironment) );
	static const char *default_pixmap_path_format =
		"%s/desktop/icons/:"
		"%s/desktop/icons/:"
		"%s/desktop/:"
		"%s/desktop/:"
		"%s/desktop/buttons/:"
		"%s/desktop/buttons/:"
		"%s/backgrounds/:"
		"%s/backgrounds/:"
		"%s" ;

	static const char *default_font_path_format =
		"%s/desktop/fonts/:"
		"%s/desktop/fonts/:"
		"/usr/share/fonts/default/TrueType/:"
		"%s" ;

	static const char *default_cursor_path_format =
		"%s/desktop/cursors:"
		"%s/desktop/cursors";

	e->desk_scale = 24 ;
	e->desk_pages_h = 2 ;
	e->desk_pages_v = 2 ;
	e->module_path = mystrdup( AFTER_BIN_DIR );
	e->icon_path = mystrdup( DEFAULT_ICON_DIR );
	e->pixmap_path = safemalloc( strlen((char*)default_pixmap_path_format)+
		                         strlen(AFTER_DIR)*4+
								 strlen(AFTER_SHAREDIR)*4 +
								 strlen(DEFAULT_PIXMAP_DIR) + 1);
	sprintf( e->pixmap_path, default_pixmap_path_format,
	                         AFTER_DIR,
							 AFTER_SHAREDIR,
							 AFTER_DIR,
							 AFTER_SHAREDIR,
							 AFTER_DIR,
							 AFTER_SHAREDIR,
							 AFTER_DIR,
							 AFTER_SHAREDIR,
							 DEFAULT_PIXMAP_DIR );

	e->font_path = safemalloc(   strlen((char*)default_font_path_format)+
								 strlen(AFTER_DIR)+
								 strlen(AFTER_SHAREDIR) +
								 strlen(DEFAULT_TTF_DIR) + 1);
	sprintf( e->font_path, default_font_path_format,
	                         AFTER_DIR,
							 AFTER_SHAREDIR,
							 DEFAULT_TTF_DIR );

	e->cursor_path = safemalloc( strlen((char*)default_cursor_path_format)+
								 strlen(AFTER_DIR)+
								 strlen(AFTER_SHAREDIR) + 1);
	sprintf( e->cursor_path, default_cursor_path_format,
	                         AFTER_DIR,
							 AFTER_SHAREDIR );
	return e;
}

void
destroy_asenvironment( ASEnvironment **penv )
{
	if( penv )
	{
		ASEnvironment *e = *penv ;
		if( e )
		{
			if( e->module_path )
				free( e->module_path );
			if( e->audio_path )
				free( e->audio_path );
			if( e->icon_path )
				free( e->icon_path );
			if( e->pixmap_path )
				free( e->pixmap_path );
			if( e->font_path )
				free( e->font_path );
			if( e->cursor_path )
				free( e->cursor_path );

			free( e ) ;
			*penv = NULL ;
		}
	}
}

void
InitSession()
{
    /* initializing our dirs names */
	if( Session == NULL ) 
	{	
    	Session = GetNCASSession(ASDefaultScr, as_app_args.override_home, as_app_args.override_share);
    	if( as_app_args.override_config )
        	set_session_override( Session, as_app_args.override_config, 0 );
    	if( as_app_args.override_look )
        	set_session_override( Session, as_app_args.override_look, F_CHANGE_LOOK );
    	if( as_app_args.override_feel )
        	set_session_override( Session, as_app_args.override_feel, F_CHANGE_FEEL );
	}
}


void
FreeMyAppResources()
{
	
    balloon_init (True);
    mystyle_destroy_all();
    destroy_image_manager( ASDefaultScr->image_manager, False );
    destroy_font_manager( ASDefaultScr->font_manager, False );
    clientprops_cleanup ();
    destroy_wmprops (ASDefaultScr->wmprops, False);
    wmprops_cleanup ();
    free_func_hash();
    purge_asimage_registry();
	asxml_var_cleanup();
	custom_color_cleanup();
    build_xpm_colormap( NULL );
    flush_ashash_memory_pool();
    destroy_screen_gcs(ASDefaultScr);
	if( ASDefaultScr->RootImage ) 
	{	
		safe_asimage_destroy( ASDefaultScr->RootImage );
		ASDefaultScr->RootImage = NULL ;
	}
	destroy_asvisual( ASDefaultScr->asv, False );
    flush_asbidirlist_memory_pool();
    free( as_app_args.saved_argv );
    destroy_assession( Session );
	Session = NULL ;
	destroy_asenvironment( &Environment );
	is_executable_in_path ( NULL );
#ifdef XSHMIMAGE
	flush_shm_cache();
#endif
	free( ASDefaultScr );
	flush_default_asstorage();

}


/************ child process spawning ***************************/
/***********************************************************************
 *  Procedure:
 *  general purpose child launcher - spawn child.
 *  Pass along all the cmd line args if needed.
 *  returns PID of the spawned process
 ************************************************************************/
static int  	as_singletons[MAX_SINGLETONS_NUM];
static Bool 	as_init_singletons = True;

void
as_sigchild_handler (int signum)
{
	int           pid;
	int status ;

	signal (SIGCHLD, as_sigchild_handler);
	DEBUG_OUT("Entering SigChild_handler(%lu)", time (NULL));
	if( as_init_singletons )
		return;
	while ((pid = WAIT_CHILDREN (&status)) > 0 )
	{
		register int i ;
		LOCAL_DEBUG_OUT( "pid = %d", pid );
		for( i = 0; i < MAX_SINGLETONS_NUM ; i++ )
			if( pid == as_singletons[i] )
			{
				as_singletons[i] = 0 ;
				break;
			}
	}
	DEBUG_OUT("Exiting SigChild_handler(%lu)", time (NULL));
}

/*
 * This should return 0 if process of running external app to draw background completed or killed.
 * otherwise it returns > 0
 */
int
check_singleton_child (int singleton_id, Bool kill_it_to_death)
{
	int           i;
	int 		  pid, status;

	if( as_init_singletons || singleton_id < 0 )
		return -1 ;

	if( singleton_id >= MAX_SINGLETONS_NUM )
		singleton_id = MAX_SINGLETONS_NUM-1;

	DEBUG_OUT("CheckingForDrawChild(%lu)....",time (NULL));
	if ( (pid = as_singletons[singleton_id]) > 0)
	{
		DEBUG_OUT("checking on singleton child #%d started with PID (%d)", singleton_id, pid);
		if (kill_it_to_death)
		{
			kill (pid, SIGTERM);
			for (i = 0; i < 100; i++)		   /* give it 10 sec to terminate */
			{
				sleep_a_millisec (100);
				if ( WAIT_CHILDREN (&status) == pid || as_singletons[singleton_id] <= 0 )
					break;
			}
			if (i >= 100)
				kill (pid, SIGKILL);  /* no more mercy */
			as_singletons[singleton_id] = 0 ;
		}
	} else if (as_singletons[singleton_id] < 0)
		as_singletons[singleton_id] = 0;

	DEBUG_OUT ("Done(%lu). Child PID on exit = %d.", time (NULL), as_singletons[singleton_id]);
	return as_singletons[singleton_id];
}

int
spawn_child( const char *cmd, int singleton_id, int screen, Window w, int context, Bool do_fork, Bool pass_args, ... )
{
    int pid = 0;

    if( cmd == NULL )
        return 0;
	if( as_init_singletons )
	{
		register int i ;
		for( i = 0; i < MAX_SINGLETONS_NUM ; i++ )
			as_singletons[i] = 0 ;
		signal (SIGCHLD, as_sigchild_handler);
		as_init_singletons = False ;
	}

	if( singleton_id >= 0 )
	{
		if( singleton_id >= MAX_SINGLETONS_NUM )
			singleton_id = MAX_SINGLETONS_NUM-1;
		if( as_singletons[singleton_id] > 0 )
			check_singleton_child( singleton_id, True );
	}

    if( do_fork )
        pid = fork();

    if( pid != 0 )
	{
		/* there is a possibility of the race condition here
		 * but it really is not worse the trouble to try and avoid it.
		 */
    	if( singleton_id >= 0 )
			as_singletons[singleton_id] = pid ;
  	    return pid;
	}else
    {/* we get here only in child process. We now need to spawn new proggy here: */
        int len;
        char *display = mystrdup(XDisplayString (dpy));
		char **envp ; 
        register char *ptr ;

        char *cmdl;
        char *arg, *screen_str = NULL, *w_str = NULL, *context_str = NULL ;
		int env_s = 0;
		char **envvars = AS_environ ; 
		int font_path_slot = -1, image_path_slot = -1 ;

        va_list ap;
		LOCAL_DEBUG_OUT( "dpy = %p, DisplayString = \"%s\"", dpy, display );
		LOCAL_DEBUG_OUT( "pid(%d), entered child process to spawn ...", pid );

#if HAVE_DECL_ENVIRON
		if( envvars == NULL ) 
		{
			envvars = environ ;
		}	 
#else
/* how the hell could we get environment otherwise ? */
#endif
		if( envvars ) 
		{
			int font_path_len = strlen(ASFONT_PATH_ENVVAR);
			int image_path_len = strlen(ASIMAGE_PATH_ENVVAR);
			for( env_s = 0  ; envvars[env_s] != NULL ; ++env_s )
			{
				if( font_path_slot < 0 && strlen(envvars[env_s]) > font_path_len ) 
					if(strncmp(envvars[env_s], ASFONT_PATH_ENVVAR, font_path_len)==0)
						font_path_slot = env_s ; 
				if( image_path_slot < 0 && strlen(envvars[env_s]) > image_path_len) 
					if(strncmp(envvars[env_s], ASIMAGE_PATH_ENVVAR, image_path_len)==0)
						image_path_slot = env_s ;
			}	
		}
		if( font_path_slot < 0 ) 
			++env_s ;
		if( image_path_slot  < 0 ) 
			++env_s ;
		envp = safecalloc( env_s+2, sizeof(char*));

		/* environment variabless to pass to child process */
		if( envvars ) 
			for( env_s = 0  ; envvars[env_s] != NULL ; ++env_s )
				envp[env_s] = envvars[env_s] ;	
        
		envp[env_s] = safemalloc(8+strlen(display)+1);
		sprintf( envp[env_s], "DISPLAY=%s", display );
		++env_s ;
		if( Environment ) 
		{
			if( Environment->pixmap_path != NULL ) 
			{	
				int slot_no = image_path_slot ; 
				if( slot_no < 0 ) 
					slot_no = env_s++ ; 
					
				envp[slot_no] = safemalloc(strlen( ASIMAGE_PATH_ENVVAR ) + 1 + strlen(Environment->pixmap_path)+1) ;
				sprintf( envp[slot_no], "%s=%s", ASIMAGE_PATH_ENVVAR, Environment->pixmap_path );
			}
			if( Environment->font_path ) 
			{	
				int slot_no = font_path_slot ; 
				if( slot_no < 0 ) 
					slot_no = env_s++ ; 
				envp[slot_no] = safemalloc(strlen( ASFONT_PATH_ENVVAR ) + 1 + strlen(Environment->font_path)+1) ;
				sprintf( envp[slot_no], "%s=%s", ASFONT_PATH_ENVVAR, Environment->font_path );
			}
		}	 
			
        len = strlen((char*)cmd);
        if( pass_args )
        {
            register int i = 0 ;

            while( display[i] ) ++i;

            while( i > 0 && isdigit(display[--i]) );
            if( display[i] == '.' )
                display[i+1] = '\0' ;

            if( screen >= 0 )
                screen_str = string_from_int( screen );
            if( w != None )
                w_str = string_from_int( w );
            if( context != C_NO_CONTEXT )
                context_str = string_from_int( context );

            len += 1+2+1+strlen( display );
            if( screen_str )
                len += strlen(screen_str);
            len += 3 ;                         /* for "-s " */
            if ( get_flags( as_app_args.flags, ASS_Debugging) )
                len += 8 ;
            if ( get_flags( as_app_args.flags, ASS_Restarting) )
                len += 3 ;
            if ( as_app_args.override_config )
                len += 4+strlen(as_app_args.override_config);
            if ( as_app_args.override_home )
                len += 4+strlen(as_app_args.override_home);
            if ( as_app_args.override_share )
                len += 4+strlen(as_app_args.override_share);

			if ( as_app_args.locale )
                len += 4+strlen(as_app_args.locale);

            if( as_app_args.verbosity_level != OUTPUT_DEFAULT_THRESHOLD )
                len += 4+32 ;
#ifdef DEBUG_TRACE_X
            if( as_app_args.trace_calls )
                len += 13+strlen( as_app_args.trace_calls );
#endif
            if( w_str )
                len += 1+8+1+strlen(w_str);
            if( context_str )
                len += 1+9+1+strlen(context_str);
        }
        /* now we want to append arbitrary number of arguments to the end of command line : */
        va_start( ap, pass_args );
        while( (arg = va_arg(ap,char*)) != NULL )
            len += 1+strlen(arg);
        va_end(ap);

        len+=4;

        ptr = cmdl = safemalloc( len );
        strcpy( cmdl, (char*)cmd );
        while(*ptr) ptr++;
        if( pass_args )
        {
            ptr += sprintf( ptr, " -d %s%s -s", display, screen_str?screen_str:"" );
            if ( get_flags( as_app_args.flags, ASS_Debugging) )
            {
                strcpy( ptr, " --debug");
                ptr+=8 ;
            }
            if ( get_flags( as_app_args.flags, ASS_Restarting) )
            {
                strcpy( ptr, " -r");
                ptr += 3 ;
            }
            if ( as_app_args.override_config )
                ptr += sprintf( ptr, " -f %s", as_app_args.override_config );
            if ( as_app_args.override_home )
                ptr += sprintf( ptr, " -p %s", as_app_args.override_home );
            if ( as_app_args.override_share )
                ptr += sprintf( ptr, " -g %s", as_app_args.override_share );
            if( as_app_args.verbosity_level != OUTPUT_DEFAULT_THRESHOLD )
                ptr += sprintf( ptr, " -V %d", as_app_args.verbosity_level );
LOCAL_DEBUG_OUT( "len = %d, cmdl = \"%s\" strlen = %d, locale = \"%s\", ptr-cmdl = %d", len, cmdl, strlen(cmdl), as_app_args.locale, ptr-cmdl );
			if( as_app_args.locale && as_app_args.locale[0] && !isspace(as_app_args.locale[0]))
                ptr += sprintf( ptr, " -L %s", as_app_args.locale );
			
#ifdef DEBUG_TRACE_X
            if( as_app_args.trace_calls )
                ptr += sprintf( ptr, " --trace-func %s", as_app_args.trace_calls );
#endif
            if( w_str )
                ptr += sprintf( ptr, " --window %s", w_str );
            if( context_str )
                ptr += sprintf( ptr, " --context %s", context_str );
        }
        
		va_start( ap, pass_args );
        while( (arg = va_arg(ap,char*)) != NULL )
        {
            *(ptr++) = ' ';
            strcpy( ptr, arg );
            while(*ptr) ptr++;
LOCAL_DEBUG_OUT( "len = %d, cmdl = \"%s\" strlen = %d", len, cmdl, strlen(cmdl) );
        
		}
        va_end(ap);
        if( do_fork )
        {
            int i = ptr-cmdl;
            while( --i >= 0 ) if( !isspace(cmdl[i]) ) break;
            do_fork = ( i < 0 || cmdl[i] != '&' );
        }
		strcpy (ptr, do_fork?" &\n":"\n");

LOCAL_DEBUG_OUT( "len = %d, cmdl = \"%s\" strlen = %d", len, cmdl, strlen(cmdl) );
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
		{
			FILE *fff = fopen ("/tmp/afterstep.exec.log", "a");
			if( fff ) 
			{
				fprintf( fff, "%s:%ld: [%s]", MyName, time(NULL), cmdl );
				fclose(fff);
			}
		}	
#endif			   
     
        LOCAL_DEBUG_OUT("execle(\"%s\")", cmdl );
        /* fprintf( stderr, "len=%d: execl(\"%s\")", len, cmdl ); */

        /* CYGWIN does not handle close-on-exec gracefully - whave to do it ourselves : */
        if( CloseOnExec )
            CloseOnExec();

		{
			const char     *shell;
			char *argv0 ;

	    	if ((shell = getenv("SHELL")) == NULL || *shell == '\0')
				shell = mystrdup("/bin/sh");

			parse_file_name(shell, NULL, &argv0);
	    	/* argv0 = basename(shell); */

        	execle (shell, argv0 , "-c", cmdl, (char *)0, envp);
		}

        if( screen >= 0 )
            show_error( "failed to start %s on the screen %d", cmd, screen );
        else
            show_error( "failed to start %s", cmd );
        show_system_error( " complete command line: \"%s\"\n", cmdl );
        exit(128);
    }
}
/************ end child process spawning ***************************/

