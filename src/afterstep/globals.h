#ifndef AS_GLOBALS_HEADER_FILE
#define AS_GLOBALS_HEADER_FILE

/* these are global functions and variables private for afterstep */

/* from configure.c */
void error_point();
void tline_error(const char* err_text);
void str_error(const char* err_format, const char* string);

int is_executable_in_path (const char *name);

typedef struct _as_dirs
{
    char* after_dir ;
    char* after_sharedir;
    char* afters_noncfdir;
} ASDirs;

/* from afterstep.c */
extern ASDirs as_dirs;

/* from dirtree.c */
char * strip_whitespace (char *str);

/* from configure.c */
extern XContext MenuContext;	/* context for afterstep menus */
extern int MenuMiniPixmaps;

extern struct ASDatabase    *Database;


#endif
