#ifndef AFTERSTEP_LIB_HEADER_FILE_INCLUDED
#define AFTERSTEP_LIB_HEADER_FILE_INCLUDED

#include "afterbase.h"

#include <stdio.h>
#include <stdlib.h>
#include "../libAfterBase/safemalloc.h"
#include "../libAfterBase/mystring.h"
#include "../libAfterBase/os.h"
#include "../libAfterBase/sleep.h"
#include "../libAfterBase/fs.h"
#include "../libAfterBase/audit.h"


#define GetFdWidth			get_fd_width
#define PutHome(f)			put_file_home(f)
#define CopyFile(f1,f2)		copy_file((f1),(f2))

#define replaceEnvVar(p)	replace_envvar (p)
#define findIconFile(f,p,t)	find_file(f,p,t)



#define CLAMP(a,b,c)        ((a)<(b) ? (b) : ((a)>(c) ? (c) : (a)))
#define CLAMP_SIZE(a,b,c)   ((a)<(b) ? (b) : ((c)!=-1&&(a)>(c) ? (c) : (a)))


#include "general.h"		/* because misc.h is already taken */
#include "font.h"
#include "mystyle.h"
#include "mystyle_property.h"
#include "balloon.h"

typedef struct ASRectangle
{
  int x, y;
  unsigned int width, height;
}
ASRectangle;


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
