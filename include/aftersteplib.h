#ifndef AFTERSTEP_LIB_HEADER_FILE_INCLUDED
#define AFTERSTEP_LIB_HEADER_FILE_INCLUDED

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

#define CLAMP(a,b,c)        ((a)<(b) ? (b) : ((a)>(c) ? (c) : (a)))
#define CLAMP_SIZE(a,b,c)   ((a)<(b) ? (b) : ((c)!=-1&&(a)>(c) ? (c) : (a)))
#define	NEW(a)              ((a *)malloc(sizeof(a)))
#define	NEW_ARRAY(a, b)     ((a *)malloc(sizeof(a) * (b)))
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
#include <stdio.h>

typedef struct gradient_t
  {
    int npoints;
    XColor *color;
    double *offset;
  }
gradient_t;

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

char *CatString3 (const char *, const char *, const char *);

/* from gethostname.c */
int mygethostname (char *, size_t);

/* from sendinfo.c */
void SendInfo (int *, char *, unsigned long);

/* from safemalloc.c */
void *safemalloc (size_t);
void safefree (void *);
/* returns old value */
int set_use_tmp_heap (int on);

/* from findiconfile.c */
char *findIconFile (const char *, const char *, int);

/* from wild.c */
int matchWildcards (const char *, const char *);

void replaceEnvVar (char **);
void CopyString (char **, char *);

/* sleeping functions */
void sleep_a_little (int);
void start_ticker( unsigned int size /* in ms */ );
void wait_tick();
Bool is_tick();

int GetFdWidth (void);

/* constructs filename from two parts */
char *make_file_name (const char *path, const char *file);
int CopyFile (const char *realfilename1, const char *realfilename2);

int CheckMode (const char *file, int mode);
#if !defined(S_IFREG) || !defined(S_IFDIR)
#include <sys/stat.h>
#endif
#define CheckFile(f) 	CheckMode(f,S_IFREG)
#define CheckDir(d) 	CheckMode(d,S_IFDIR)

char *PutHome (const char *);

#include "audit.h"
#ifndef DEBUG_ALLOCS
/*#define free(a)       safefree(a) */
#endif


#endif /* #ifndef AFTERSTEP_LIB_HEADER_FILE_INCLUDED */
