#ifndef OUTPUT_H_HEADER_INCLUDED
#define OUTPUT_H_HEADER_INCLUDED

#ifndef Bool
#define Bool int
#endif

/* This has to manually set in order for output to have meaningfull labels : */
extern char *ApplicationName ;
void set_application_name (char *argv0);
const char *get_application_name();


/***********************************************************************************/
/* The following can be used to enable/disable verbose output from AfterStep :     */
/***********************************************************************************/
typedef int (*stream_func)(void*, const char*,...);
unsigned int set_output_threshold( unsigned int threshold );
unsigned int get_output_threshold();

/*
 * FEW PRESET LEVELS OF OUTPUT :
 */
#define OUTPUT_LEVEL_INVALID        0
#define OUTPUT_LEVEL_PARSE_ERR      1
#define OUTPUT_LEVEL_ERROR          1
#define OUTPUT_LEVEL_WARNING        4
#define OUTPUT_DEFAULT_THRESHOLD    5
#define OUTPUT_LEVEL_PROGRESS       OUTPUT_DEFAULT_THRESHOLD+1
#define OUTPUT_LEVEL_ACTIVITY       OUTPUT_DEFAULT_THRESHOLD+2
#define OUTPUT_VERBOSE_THRESHOLD    OUTPUT_DEFAULT_THRESHOLD+3
#define OUTPUT_LEVEL_DEBUG          10   /* anything above it is hardcore debugging */

unsigned int set_output_level( unsigned int level );
void restore_output_level();
Bool is_output_level_under_threshold( unsigned int level );
/*
 * if pfunc is NULL then if threshold >= level - then we use fprintf
 * otherwise - return False
 */
Bool pre_print_check( stream_func *pfunc, void** pstream, void *data, const char *msg );

/* AfterStep specific error and Warning handlers : */
/* Returns True if something was actually printed  */
Bool show_error( const char *error_format, ...);
Bool show_system_error( const char *error_format, ...);  /* will also execute perror() */
Bool show_warning( const char *warning_format, ...);
Bool show_progress( const char *msg_format, ...);
Bool show_debug( const char *file, const char *func, int line, const char *msg_format, ...);
#define SHOW_CHECKPOINT show_debug(__FILE__,__FUNCTION__,__LINE__, "*checkpoint*")


void nonGNUC_debugout( const char *format, ...);
inline void nonGNUC_debugout_stub( const char *format, ...);
/* may be used below in case compilation problems occur.
 * Please submit a bug report if usage of any of the following generates errors on
 * your compiler . Thanks!!! */

/* Some usefull debugging macros : */
#ifdef __GNUC__

#if (!defined(NO_DEBUG_OUTPUT))&&(defined(LOCAL_DEBUG)||defined(DEBUG)||defined(DEBUG_ALL))
#define DEBUG_OUT(format,args...) \
    do{ fprintf( stderr, "%s:%s:%s:%d:>" format "\n", ApplicationName, __FILE__, __FUNCTION__, __LINE__, ## args );}while(0)
#else
#define DEBUG_OUT(format,args...)
#endif /* DEBUG */

#if (!defined(NO_DEBUG_OUTPUT))&&(defined(LOCAL_DEBUG)||defined(DEBUG_ALL))
#define LOCAL_DEBUG_OUT(format,args...) \
    do{ fprintf( stderr, "%s:%ld:%s:%s:%d:>" format "\n", ApplicationName, time(NULL),__FILE__, __FUNCTION__, __LINE__, ## args );}while(0)
#define LOCAL_DEBUG_CALLER_OUT(format,args...) \
    do{ fprintf( stderr, "%s:%ld:%s:%s:> called from [%s] with args(" format ")\n", ApplicationName, time(NULL),__FILE__, __FUNCTION__, get_caller_func(), ## args );}while(0)
#else
#define LOCAL_DEBUG_OUT(format,args...)
#define LOCAL_DEBUG_CALLER_OUT(format,args...)
#endif /* LOCAL_DEBUG */

#elif  __STDC_VERSION__ >= 199901              /* C99 standard provides support for this as well : */

#if (!defined(NO_DEBUG_OUTPUT))&&(defined(LOCAL_DEBUG)||defined(DEBUG)||defined(DEBUG_ALL))
#define DEBUG_OUT(...) \
    do{ fprintf( stderr, "%s:%s:%s:%d:>", ApplicationName, __FILE__, __FUNCTION__, __LINE__ ); \
        fprintf( stderr, __VA_ARGS__); \
        fprintf( stderr, "\n"); \
    }while(0)
#else
#define DEBUG_OUT(...)
#endif /* DEBUG */

#if (!defined(NO_DEBUG_OUTPUT))&&(defined(LOCAL_DEBUG)||defined(DEBUG_ALL))
#define LOCAL_DEBUG_OUT(...) \
    do{ fprintf( stderr, "%s:%s:%s:%d:>", ApplicationName, __FILE__, __FUNCTION__, __LINE__ ); \
        fprintf( stderr, __VA_ARGS__); \
        fprintf( stderr, "\n"); \
    }while(0)
#define LOCAL_DEBUG_CALLER_OUT(...) \
    do{ fprintf( stderr, "%s:%s:%s:> called from [%s] with args(", ApplicationName, __FILE__, get_caller_func() ); \
        fprintf( stderr, __VA_ARGS__); \
        fprintf( stderr, ")\n"); \
    }while(0)
#else
#define LOCAL_DEBUG_OUT(...)
#define LOCAL_DEBUG_CALLER_OUT(...)
#endif /* LOCAL_DEBUG */

#else  /* non __GNUC__ or C99 compliant compiler : */

#if (!defined(NO_DEBUG_OUTPUT))&&(defined(LOCAL_DEBUG)||defined(DEBUG)||defined(DEBUG_ALL))
#define DEBUG_OUT           nonGNUC_debugout
#else
#define DEBUG_OUT           nonGNUC_debugout_stub
#endif /* DEBUG */

#if (!defined(NO_DEBUG_OUTPUT))&&(defined(LOCAL_DEBUG)||defined(DEBUG_ALL))
#define LOCAL_DEBUG_OUT     nonGNUC_debugout
#define LOCAL_DEBUG_CALLER_OUT     nonGNUC_debugout_stub
#else
#define LOCAL_DEBUG_OUT            nonGNUC_debugout_stub
#define LOCAL_DEBUG_CALLER_OUT     nonGNUC_debugout_stub
#endif /* LOCAL_DEBUG */

#endif

#ifdef DO_CLOCKING
#define START_TIME(started)  time_t started = clock()
#define SHOW_TIME(s,started) fprintf (stderr, __FUNCTION__ " " s " time (clocks): %lu mlsec\n", ((clock () - (started))*100)/CLOCKS_PER_SEC)
#else
#define START_TIME(started)  unsigned long started = 0
#define SHOW_TIME(s,started) started = 0
#endif

#endif
