#ifndef ASIM_AFTERBASE_H_HEADER_INCLUDED
#define ASIM_AFTERBASE_H_HEADER_INCLUDED

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#endif
   

/* our own version of X Wrapper : */
#include "xwrap.h"

/* the goal of this header is to provide sufficient code so that
   libAfterImage could live without libAfterBase at all.
   Basically with define macros and copy over few functions
   from libAfterBase
 */

/* from libAfterBase/astypes.h : */

#include <stdio.h>

#ifdef __GNUC__

#ifndef MIN
#define MIN(x,y)                                \
  ({ const typeof(x) _x = (x); const typeof(y) _y = (y); \
     (void) (&_x == &_y);                       \
     _x < _y ? _x : _y; })
#endif
#ifndef MAX
#define MAX(x,y)                                \
  ({ const typeof(x) _x = (x); const typeof(y) _y = (y); \
     (void) (&_x == &_y);                       \
     _x > _y ? _x : _y; })
#endif

#define AS_ASSERT(p)            ((p)==(typeof(p))0)
#define AS_ASSERT_NOTVAL(p,v)   ((p)!=(typeof(p))v)

#else

#define MIN(a,b)            ((a)<(b) ? (a) : (b))
#define MAX(a,b)            ((a)>(b) ? (a) : (b))
#define AS_ASSERT(p)            ((p)==0)
#define AS_ASSERT_NOTVAL(p,v)   ((p)!=(v))
#define inline

#endif

#ifdef __INTEL_COMPILER
#define inline
#endif

#ifndef max
#define max(x,y)            MAX(x,y)
#endif

#ifndef min
#define min(x,y)            MIN(x,y)
#endif

typedef unsigned long ASFlagType ;
#define ASFLAGS_EVERYTHING  0xFFFFFFFF
typedef ASFlagType ASFlagsXref[5];

#define get_flags(var, val) 	((var) & (val))  /* making it sign safe */
#define set_flags(var, val) 	((var) |= (val))
#define clear_flags(var, val) 	((var) &= ~(val))
#define CheckSetFlag(b,f,v) 	{if((b)) (f) |= (v) ; else (f) &= ~(v);}

#define PTR2CARD32(p)       ((CARD32)(p))
#define LONG2CARD32(l)      ((CARD32)(l))

typedef struct ASMagic
{ /* just so we can safely cast void* to query magic number :*/
    unsigned long magic ;
}ASMagic;

/* from libAfterBase/selfdiag.h : */
#define get_caller_func() "unknown"

/* from libAfterBase/output.h : */
/* user app must export these if libAfterBase is not used : */
void asim_set_application_name (char *argv0);
const char *asim_get_application_name();

#define set_application_name asim_set_application_name
#define get_application_name asim_get_application_name

/*
 * FEW PRESET LEVELS OF OUTPUT :
 */
#define OUTPUT_LEVEL_INVALID        0
#define OUTPUT_LEVEL_PARSE_ERR      1
#define OUTPUT_LEVEL_ERROR          1
#define OUTPUT_LEVEL_WARNING        4
#define OUTPUT_DEFAULT_THRESHOLD    5
#define OUTPUT_LEVEL_PROGRESS       OUTPUT_DEFAULT_THRESHOLD
#define OUTPUT_LEVEL_ACTIVITY       OUTPUT_DEFAULT_THRESHOLD
#define OUTPUT_VERBOSE_THRESHOLD    6
#define OUTPUT_LEVEL_DEBUG          10   /* anything above it is hardcore debugging */

/* AfterStep specific error and Warning handlers : */
/* Returns True if something was actually printed  */
unsigned int asim_get_output_threshold();
unsigned int asim_set_output_threshold( unsigned int threshold );
#define get_output_threshold asim_get_output_threshold
#define set_output_threshold asim_set_output_threshold

Bool asim_show_error( const char *error_format, ...);
Bool asim_show_warning( const char *warning_format, ...);
Bool asim_show_progress( const char *msg_format, ...);
Bool asim_show_debug( const char *file, const char *func, int line, const char *msg_format, ...);

#define show_error asim_show_error
#define show_warning asim_show_warning
#define show_progress asim_show_progress
#define show_debug asim_show_debug

void asim_nonGNUC_debugout( const char *format, ...);
void asim_nonGNUC_debugout_stub( const char *format, ...);
/* may be used below in case compilation problems occur.
 * Please submit a bug report if usage of any of the following generates errors on
 * your compiler . Thanks!!! */

/* Some usefull debugging macros : */
#ifdef __GNUC__

#if defined(LOCAL_DEBUG)||defined(DEBUG)||defined(DEBUG_ALL)
#define DEBUG_OUT(format,args...) \
    do{ fprintf( stderr, "%s:%s:%s:%d:>" format "\n", get_application_name(), __FILE__, __FUNCTION__, __LINE__, ## args );}while(0)
#else
#define DEBUG_OUT(format,args...)
#endif /* DEBUG */

#if defined(LOCAL_DEBUG)||defined(DEBUG_ALL)
#define LOCAL_DEBUG_OUT(format,args...) \
    do{ fprintf( stderr, "%s:%s:%s:%d:>" format "\n", get_application_name(), __FILE__, __FUNCTION__, __LINE__, ## args );}while(0)
#define LOCAL_DEBUG_CALLER_OUT(format,args...) \
    do{ fprintf( stderr, "%s:%s:%s:> called from [%s] with args(" format ")\n", get_application_name(), __FILE__, __FUNCTION__, get_caller_func(), ## args );}while(0)
#else
#define LOCAL_DEBUG_OUT(format,args...)
#define LOCAL_DEBUG_CALLER_OUT(format,args...)
#endif /* LOCAL_DEBUG */

#elif  __STDC_VERSION__ >= 199901              /* C99 standard provides support for this as well : */

#if defined(LOCAL_DEBUG)||defined(DEBUG)||defined(DEBUG_ALL)
#define DEBUG_OUT(...) \
    do{ fprintf( stderr, "%s:%s:%s:%d:>", get_application_name(), __FILE__, __FUNCTION__, __LINE__ ); \
        fprintf( stderr, __VA_ARGS__); \
        fprintf( stderr, "\n"); \
    }while(0)
#else
#define DEBUG_OUT(...)
#endif /* DEBUG */

#if defined(LOCAL_DEBUG)||defined(DEBUG_ALL)
#define LOCAL_DEBUG_OUT(...) \
    do{ fprintf( stderr, "%s:%s:%s:%d:>", get_application_name(), __FILE__, __FUNCTION__, __LINE__ ); \
        fprintf( stderr, __VA_ARGS__); \
        fprintf( stderr, "\n"); \
    }while(0)
#define LOCAL_DEBUG_CALLER_OUT(...) \
    do{ fprintf( stderr, "%s:%s:%s:> called from [%s] with args(", get_application_name(), __FILE__, get_caller_func() ); \
        fprintf( stderr, __VA_ARGS__); \
        fprintf( stderr, ")\n"); \
    }while(0)
#else
#define LOCAL_DEBUG_OUT(...)
#define LOCAL_DEBUG_CALLER_OUT(...)
#endif /* LOCAL_DEBUG */

#else  /* non __GNUC__ or C99 compliant compiler : */

#if defined(LOCAL_DEBUG)||defined(DEBUG)||defined(DEBUG_ALL)
#define DEBUG_OUT           asim_nonGNUC_debugout
#else
#define DEBUG_OUT           asim_nonGNUC_debugout_stub
#endif /* DEBUG */

#if defined(LOCAL_DEBUG)||defined(DEBUG_ALL)
#define LOCAL_DEBUG_OUT     asim_nonGNUC_debugout
#define LOCAL_DEBUG_CALLER_OUT     asim_nonGNUC_debugout_stub
#else
#define LOCAL_DEBUG_OUT            asim_nonGNUC_debugout_stub
#define LOCAL_DEBUG_CALLER_OUT     asim_nonGNUC_debugout_stub
#endif /* LOCAL_DEBUG */

#endif

#ifdef DO_CLOCKING
#define START_TIME(started)  time_t started = clock()
#define SHOW_TIME(s,started) fprintf (stderr, __FUNCTION__ " " s " time (clocks): %lu mlsec\n", ((clock () - (started))*100)/CLOCKS_PER_SEC)
#else
#define START_TIME(started)  unsigned long started = 0
#define SHOW_TIME(s,started) started = 0
#endif

/* from libAfterBase/safemalloc.h : */
#define safemalloc(s) 	malloc(s)
#define safecalloc(c,s) calloc(c,s)
#define safefree(m)   	free(m)
#define	NEW(a)              	((a *)malloc(sizeof(a)))
#define	NEW_ARRAY_ZEROED(a, b)  ((a *)calloc(b, sizeof(a)))
#define	NEW_ARRAY(a, b)     	((a *)malloc(b*sizeof(a)))

/* from libAfterBase/mystring.h : */
#include <string.h>
#define mystrdup(s)     strdup(s)

char   *asim_mystrndup(const char *str, size_t n);
#define mystrndup(s,n)    	 asim_mystrndup(s,n)
#ifdef _WIN32
#define mystrncasecmp(s,s2,n)    _strnicmp(s,s2,n)
#define mystrcasecmp(s,s2)       _stricmp(s,s2)
#else
#define mystrncasecmp(s,s2,n)    strncasecmp(s,s2,n)
#define mystrcasecmp(s,s2)       strcasecmp(s,s2)
#endif

/* from libAfterBase/fs.h : */
#ifndef _WIN32
struct direntry
  {
    mode_t d_mode;		/* S_IFDIR if a directory */
    time_t d_mtime;
    char d_name[1];
  };
#endif
int		asim_check_file_mode (const char *file, int mode);
#if !defined(S_IFREG) || !defined(S_IFDIR)
#include <sys/stat.h>
#endif
#define CheckFile(f) 	asim_check_file_mode(f,S_IFREG)
#define CheckDir(d) 	asim_check_file_mode(d,S_IFDIR)
char   *asim_put_file_home (const char *path_with_home);
#define put_file_home(p) asim_put_file_home(p)
char   *asim_load_file     (const char *realfilename);
#define load_file(r)     asim_load_file(r)
#ifndef _WIN32
int     asim_my_scandir (char *dirname, struct direntry *(*namelist[]),
			int (*select) (const char *), int (*dcomp) (struct direntry **, struct direntry **));
#define my_scandir(d,n,s,dc) asim_my_scandir((d),(n),(s),(dc))   
#endif

void unix_path2dos_path( char *path );
char   *asim_find_file (const char *file, const char *pathlist, int type);
#define find_file(f,p,t) asim_find_file(f,p,t)
char   *asim_copy_replace_envvar (char *path);
#define copy_replace_envvar(p) asim_copy_replace_envvar(p)

const char *asim_parse_argb_color( const char *color, CARD32 *pargb );
#define parse_argb_color(c,p) asim_parse_argb_color((c),(p))

#ifdef __hpux
#define PORTABLE_SELECT(w,i,o,e,t)	select((w),(int *)(i),(int *)(o),(e),(t))
#else
#define PORTABLE_SELECT(w,i,o,e,t)	select((w),(i),(o),(e),(t))
#endif

/* from libAfterBase/socket.h : */
#ifdef WORDS_BIGENDIAN
#define as_ntohl(ui32)		(ui32)
#define as_hlton(ui32)		(ui32)
#define as_ntohl16(ui16)		(ui16)
#define as_hlton16(ui16)		(ui16)
#else
#define as_ntohl(ui32)		((((ui32)&0x000000FF)<<24)|(((ui32)&0x0000FF00)<<8)|(((ui32)&0x00FF0000)>>8)|(((ui32)&0xFF000000)>>24))
#define as_hlton(ui32)		as_ntohl(ui32)     /* conversion is symmetrical */
#define as_ntohl16(ui16)		((((ui16)&0x00FF)<<8)|(((ui16)&0xFF00)>>8))
#define as_hlton16(ui16)		as_ntohl(ui16)     /* conversion is symmetrical */
#endif

#if 0
typedef union ASHashableValue
{
  unsigned long 	   long_val;
  char 				  *string_val;
  void 				  *ptr ;
}
ASHashableValue;
#else
typedef unsigned long ASHashableValue;
#endif

typedef union ASHashData
{
 	void  *vptr ;
 	int   *iptr ;
 	unsigned int   *uiptr ;
 	long  *lptr ;
 	unsigned long   *ulptr ;
	char  *cptr ;
	int    i ;
	unsigned int ui ;
	long   l ;
	unsigned long ul ;
	CARD32 c32 ;
	CARD16 c16 ;
	CARD8  c8 ;
}ASHashData;

#define AS_HASHABLE(v)  ((ASHashableValue)((unsigned long)(v)))

typedef struct ASHashItem
{
  struct ASHashItem *next;
  ASHashableValue value;
  void *data;			/* optional data structure related to this
				   hashable value */
}
ASHashItem;

typedef unsigned short ASHashKey;
typedef ASHashItem *ASHashBucket;

typedef struct ASHashTable
{
  ASHashKey size;
  ASHashBucket *buckets;
  ASHashKey buckets_used;
  unsigned long items_num;

  ASHashItem *most_recent ;

    ASHashKey (*hash_func) (ASHashableValue value, ASHashKey hash_size);
  long (*compare_func) (ASHashableValue value1, ASHashableValue value2);
  void (*item_destroy_func) (ASHashableValue value, void *data);
}
ASHashTable;

typedef enum
{

  ASH_BadParameter = -3,
  ASH_ItemNotExists = -2,
  ASH_ItemExistsDiffer = -1,
  ASH_ItemExistsSame = 0,
  ASH_Success = 1
}
ASHashResult;

void 		 asim_init_ashash (ASHashTable * hash, Bool freeresources);
ASHashTable *asim_create_ashash (ASHashKey size,
			  	 ASHashKey (*hash_func) (ASHashableValue, ASHashKey),
			   	long (*compare_func) (ASHashableValue, ASHashableValue),
			   	void (*item_destroy_func) (ASHashableValue, void *));
void         asim_destroy_ashash (ASHashTable ** hash);
ASHashResult asim_add_hash_item (ASHashTable * hash, ASHashableValue value, void *data);
ASHashResult asim_get_hash_item (ASHashTable * hash, ASHashableValue value, void **trg);
ASHashResult asim_remove_hash_item (ASHashTable * hash, ASHashableValue value, void **trg, Bool destroy);

void 		 asim_flush_ashash_memory_pool();
ASHashKey 	 asim_string_hash_value (ASHashableValue value, ASHashKey hash_size);
long 		 asim_string_compare (ASHashableValue value1, ASHashableValue value2);
void		 asim_string_destroy_without_data (ASHashableValue value, void *data);
/* variation for case-unsensitive strings */
ASHashKey 	 asim_casestring_hash_value (ASHashableValue value, ASHashKey hash_size);
long 		 asim_casestring_compare (ASHashableValue value1, ASHashableValue value2);

#define init_ashash(h,f) 			 asim_init_ashash(h,f)
#define create_ashash(s,h,c,d) 		 asim_create_ashash(s,h,c,d)
#define	destroy_ashash(h) 		 	 asim_destroy_ashash(h)
#define	add_hash_item(h,v,d) 		 asim_add_hash_item(h,v,d)
#define	get_hash_item(h,v,t) 		 asim_get_hash_item(h,v,t)
#define	remove_hash_item(h,v,t,d)	 asim_remove_hash_item(h,v,t,d)
#define	flush_ashash_memory_pool	 asim_flush_ashash_memory_pool

#define	string_hash_value 	 	 asim_string_hash_value
#define	string_compare 		 	 asim_string_compare
#define	string_destroy_without_data  asim_string_destroy_without_data
#define	casestring_hash_value		 asim_casestring_hash_value
#define	casestring_compare     		 asim_casestring_compare

/* from sleep.c */
void asim_start_ticker (unsigned int size);
void asim_wait_tick ();
#define start_ticker 	asim_start_ticker
#define wait_tick 		asim_wait_tick

#endif /* ASIM_AFTERBASE_H_HEADER_INCLUDED */

