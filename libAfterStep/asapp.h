#ifndef ASAPP_HEADER_FILE_INCLUDED
#define ASAPP_HEADER_FILE_INCLUDED

#ifdef ISC
#include <sys/bsdtypes.h>
#endif

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef BROKEN_SUN_HEADERS
#include "sun_headers.h"
#endif

#if defined(__alpha)
#include "alpha_header.h"
#endif /* NEEDS_ALPHA_HEADER */

/* Some people say that AIX and AIXV3 need 3 preceding underscores, other say
 * no. I'll do both */
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#ifdef I18N
#include <X11/Xlocale.h>
#endif
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */
#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* HAVE_XINERAMA */


#include "afterbase.h"
#include "functions.h"

struct SyntaxDef;
struct TermDef;
struct ASSession;
//#include "trace.h"

/* compatibility macros : */
#define GetFdWidth          get_fd_width
#define PutHome(f)			put_file_home(f)
#define CopyFile(f1,f2)		copy_file((f1),(f2))

#define replaceEnvVar(p)	replace_envvar (p)
#define findIconFile(f,p,t)	find_file(f,p,t)



#ifdef SIGNALRETURNSINT
#define SIGNAL_T 		int
#define SIGNAL_RETURN 		return 0
#else
#define SIGNAL_T 		void
#define SIGNAL_RETURN 		return
#endif

/************************************************************************
 * ReapChildren - wait() for all dead child processes
 ************************************************************************/
#include <sys/wait.h>
#ifdef HAVE_SYS_WAIT_H
#define ReapChildren()          while ((waitpid(-1, NULL, WNOHANG)) > 0)
#define WAIT_CHILDREN(pstatus)  waitpid(-1, pstatus, WNOHANG)
#elif defined (HAVE_WAIT3)
#define ReapChildren()          while ((wait3(NULL, WNOHANG, NULL)) > 0)
#define WAIT_CHILDREN(pstatus)  wait3(pstatus, WNOHANG, NULL)
#else
#define ReapChildren()          while (1>0)
#define WAIT_CHILDREN(pstatus)  (*pstatus=-1)
#endif

struct charstring
{
    char key;
    int value;
};

/*********************************************************************************/
/* Some usefull data structures and operations on them :						 */
/*********************************************************************************/

typedef struct CommandLineOpts
{
    char *short_opt, *long_opt;
    char *descr1, *descr2 ;
    void (*handler)( char *argv, void *trg, long param );
    void *trg;
    long param;
#define CMO_HasArgs     (0x01<<0)
#define CMO_IsParam     (0x01<<1)
    ASFlagType flags;
}CommandLineOpts;

void  print_command_line_opt(const char *prompt, CommandLineOpts *options, ASFlagType mask);
int   match_command_line_opt( char *argvi, CommandLineOpts *options );

void  handler_show_info( char *argv, void *trg, long param );
void  handler_set_flag( char *argv, void *trg, long param );
void  handler_set_string( char *argv, void *trg, long param );
void  handler_set_int( char *argv, void *trg, long param );


typedef struct ASProgArgs
{
	/* backup of the cmdline : */
	int    saved_argc ;
	char **saved_argv ;

#define OPTION_SINGLE       (0x01<<4) /* see declaration of as_cmdl_options in globals.c*/
#define OPTION_RESTART      (0x01<<5) /* see declaration of as_cmdl_options in globals.c*/
#define OPTION_DISPLAY      (0x01<<6) /* see declaration of as_cmdl_options in globals.c*/

#define OPTION_USAGE_FORMAT				"\nusage: %s [standard_options]"
#define OPTION_SHORT_FORMAT				" -%s"
#define OPTION_NOSHORT_FORMAT			"   "
#define OPTION_DESCR1_FORMAT_VAL 		" --%-16.16s <val> - %s.\n"
#define OPTION_DESCR1_FORMAT_NOVAL 		" --%-16.16s       - %s.\n"
#define OPTION_DESCR2_FORMAT			OPTION_NOSHORT_FORMAT "                            %s\n"
#define OPTION_PARAM_FORMAT		         "  %-20.20s       - %s.\n"
    ASFlagType mask ;    /* mask, specifying what options are not supported */

    char      *override_config;
	char 	  *override_home, *override_share ;
	char 	  *override_look, *override_feel ;

	char 	  *display_name ; /* our display name */

	ASFlagType flags ;   /* debugging/restarting/single, etc - see afterstep.h */

    int        verbosity_level ;

	Window src_window;  /* window in which action accured that ended up launching us */
	int    src_context; /* context in window in which action accured that ended up launching us */
#ifdef DEBUG_TRACE_X
    char      *trace_calls ;
#endif
}ASProgArgs;


/*
 * FEW PRESET LEVELS OF OUTPUT :
 */
#define OUTPUT_LEVEL_PARSE_ERR      1
#define OUTPUT_LEVEL_HINTS          OUTPUT_VERBOSE_THRESHOLD
#define OUTPUT_LEVEL_DATABASE       OUTPUT_VERBOSE_THRESHOLD
#define OUTPUT_LEVEL_VROOT          7
#define OUTPUT_LEVEL_WINDOW_LIST   (OUTPUT_LEVEL_DEBUG+9) /* too much output - too slow */


/***********************************************************************************/
/* general purpose application launcher :                                          */
/***********************************************************************************/

#define MAX_SINGLETONS_NUM 				32
#define BACKGROUND_SINGLETON_ID 		(MAX_SINGLETONS_NUM-1)
#define AUDIO_SINGLETON_ID 				(MAX_SINGLETONS_NUM-2)
#define MAX_USER_SINGLETONS_NUM 		(MAX_SINGLETONS_NUM-2)
/* Singleton is the child process of which we should not launch more then one
 * instance. singleton_id should be in range of 0...MAX_SINGLETONS_NUM. Some
 * IDs are reserved for special use - background drawing, sound playing, etc.
 */
/* use this function to see if previously launched instance of the singleton
 * is still alive. Optionally you can request to kill it.
 */

int check_singleton_child (int singleton_id, Bool kill_it_to_death);
int spawn_child( const char *cmd, int singleton_id, int screen, Window w, int context, Bool do_fork, Bool pass_args, ... );

/***********************************************************************************/
/* GLOBALS :                                                                       */
/***********************************************************************************/
/* this call will set most of them up : */
void InitMyApp (  const char *app_class, int agrc, char **argv, void (*version_func) (void), void (*custom_usage_func) (void), ASFlagType opt_mask );
void InitSession();


/* these are global public variables from libafterstep */
void DeadPipe (int);
/* Command Line stuff : */
extern ASProgArgs  MyArgs;	/* some typical progy cmd line options - set by SetMyArgs( argc, argv )*/
extern char 	  *MyName;	/* name are we known by - set by SetMyName(argv[0]) */
#define MAX_MY_CLASS    64
extern char        MyClass[MAX_MY_CLASS+1]; /* application Class name ( Pager, Wharf, etc. ) - set by SetMyClass(char *) */
extern void      (*MyVersionFunc) (void);
extern void      (*MyUsageFunc)   (void);

extern char 	 *as_gnustep_dir_name;
extern char 	 *as_gnusteplib_dir_name;
extern char 	 *as_afterstep_dir_name;
extern char 	 *as_save_dir_name;
extern char 	 *as_start_dir_name;
extern char 	 *as_share_dir_name;

#define START_DIR       "start"
#define BACK_DIR        "backgrounds"
#define LOOK_DIR        "looks"
#define FEEL_DIR        "feels"
#define BASE_FILE       "base"                           /* scrdepth */

extern int           fd_width;
/* set by screen.c:setup_modifiers() in screen.c:ConnectX() :*/
extern unsigned int  nonlock_mods;	/* a mask for non-locking modifiers */
extern unsigned int *lock_mods;  	/* all combinations of lock modifier masks */
/* Now for each display we may have one or several screens ; */
extern struct ScreenInfo   Scr;						   /* ScreenInfo for the default screen */

/* this two are unused in as-stable yet : */
extern struct ScreenInfo **all_screens ;               /* all ScreenInfo structures for NumberOfScreens screens */
extern struct ASHashTable  *screens_window_hash;      /* so we can easily track what window is on what screen */

extern int SingleScreen ;                              /* if >= 0 then [points to the only ScreenInfo structure available */
extern unsigned int  NumberOfScreens;   			   /* number of screens on display */
extern int PointerScreen ;							   /* screen that currently has pointer */

extern struct TermDef       FuncTerms[F_FUNCTIONS_NUM + 1];
extern struct SyntaxDef FuncSyntax, *pFuncSyntax ;

/* Base config : */
extern char *PixmapPath;
extern char *CursorPath;
extern char *IconPath;
extern char *ModulePath;
extern char *FontPath;

struct ASSession *Session;          /* filenames of look, feel and background */

/* this two are unused in as-stable yet : */
struct ASFeel;
struct MyLook;
extern struct ASFeel *DefaultFeel;
extern struct MyLook *DefaultLook;

extern struct ASEnvironment *DefaultEnv;


#endif /* #ifndef AFTERSTEP_LIB_HEADER_FILE_INCLUDED */
