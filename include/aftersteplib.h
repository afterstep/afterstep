#ifndef AFTERSTEP_LIB_HEADER_FILE_INCLUDED
#define AFTERSTEP_LIB_HEADER_FILE_INCLUDED

#include "../libAfterBase/astypes.h"

/***********************************************************************/
/* This stuff should be coming from libAfterBase now :                 */
#if 0
#ifndef ABS
#define ABS(a)              ((a)>0   ? (a) : -(a))
#endif
#ifndef SGN
#define SGN(a)              ((a)>0   ?  1  : ((a)<0 ? -1 : 0))
#endif
#ifndef MIN
#define MIN(a,b)            ((a)<(b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b)            ((a)>(b) ? (a) : (b))
#endif

#define SWAP(a, b, type)    { type SWAP_NaMe = a; a = b; b = SWAP_NaMe; }

#ifndef get_flags
#define set_flags(v,f)  	((v)|=(f))
#define clear_flags(v,f)	((v)&=~(f))
#define get_flags(v,f)  	((v)&(f))
#endif

#include <X11/Xlib.h>
#ifdef I18N
#include <X11/Xlocale.h>
#endif

/* from safemalloc.c */
void *safemalloc (size_t);
void safefree (void *);
/* returns old value */
int set_use_tmp_heap (int on);

int mystrcasecmp (const char *, const char *);
int mystrncasecmp (const char *, const char *, size_t);

/* from mystrdup.c */
#if defined(LOG_STRDUP_CALLS) && defined(DEBUG_ALLOCS)
char *l_mystrdup (const char *, int, const char *);
char *l_mystrndup (const char *, int, const char *, size_t);
#define mystrdup(a)	l_mystrdup(__FUNCTION__,__LINE__,a)
#define mystrndup(a,b)	l_mystrndup(__FUNCTION__,__LINE__,a,b)
#else
char *mystrdup (const char *str);
char *mystrndup (const char *str, size_t n);
#endif

#endif /* #if 0 */
/*  End of libAfterBase stuff                                              */
/***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "../libAfterBase/audit.h"
#include "../libAfterBase/safemalloc.h"
#include "../libAfterBase/mystring.h"
#include "../libAfterBase/os.h"
#include "../libAfterBase/sleep.h"
#include "../libAfterBase/fs.h"


#define GetFdWidth			get_fd_width
#define PutHome(f)			put_file_home(f)
#define CopyFile(f1,f2)		copy_file((f1),(f2))

#define replaceEnvVar(p)	replace_envvar (p)
#define findIconFile(f,p,t)	find_file(f,p,t)



#define CLAMP(a,b,c)        ((a)<(b) ? (b) : ((a)>(c) ? (c) : (a)))
#define CLAMP_SIZE(a,b,c)   ((a)<(b) ? (b) : ((c)!=-1&&(a)>(c) ? (c) : (a)))


#include "general.h"		/* because misc.h is already taken */
#include "font.h"
#include "timer.h"
#include "mystyle.h"
#include "mystyle_property.h"
#include "balloon.h"

/* from getcolor.c */
unsigned long GetColor (char *);
unsigned long GetShadow (unsigned long);
unsigned long GetHilite (unsigned long);


char *CatString3 (const char *, const char *, const char *);

/* from sendinfo.c */
void SendInfo (int *, char *, unsigned long);

/* from wild.c */
int matchWildcards (const char *, const char *);

void CopyString (char **, char *);

#ifndef DEBUG_ALLOCS
/*#define free(a)       safefree(a) */
#endif

void QuietlyDestroyWindow( Window w );
int ASErrorHandler (Display * dpy, XErrorEvent * event);



#endif /* #ifndef AFTERSTEP_LIB_HEADER_FILE_INCLUDED */
