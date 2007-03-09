#ifndef AS_PARSE_H_HEADER_INCLUDED
#define AS_PARSE_H_HEADER_INCLUDED

#ifdef __STDC__
/* included for the declaration of config.action below */
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* used for parsing configuration */
struct config
{
  char *keyword;
#ifdef __STDC__
  void (*action) (char *, FILE *, char **, int *);
#else
  void (*action) ();
#endif
  char **arg;
  int *arg2;
};

void register_custom_color(const char* name, CARD32 value);
void unregister_custom_color(const char* name);
Bool get_custom_color(const char* name, CARD32 *color);
void custom_color_cleanup();

const char *parse_argb_color( const char *color, CARD32 *pargb );
const char *parse_hue( const char *color, int *hue );

char *find_doublequotes (const char *ptr);
char *stripcpy (const char *source);
char *stripcpy2 (const char *source, int tab_sensitive);
char *stripcpy3 (const char *, Bool);
char *tokencpy (const char *source);
char *tokenskip( const char *ptr, unsigned int n_tokens );
struct config *find_config (struct config *, const char *);
int quotestr (char *dest, const char *src, int maxlen);

/* here we'll strip comments and whitespaces */
char *stripcomments (char *source);
char *stripcomments2 (char *source, char **comments );
char *strip_whitespace (char *str);

/* will read space separated string and allocate space for it */
char *parse_token (const char *source, char **trg);
char *parse_token_strip_quotes (const char *source, char **trg);
/* parses filename, optionally enclosed in doublequotes -
 * same as parse_token_strip_quotes :*/
#define parse_filename(s,t)  parse_token_strip_quotes((s),(t))

/* same for tab separated tokens */
char *parse_tab_token (const char *source, char **trg);
/* will parse function values with unit - usefull in AS command parsing */
char *parse_func_args (char *tline, char *unit, int *func_val);
char *parse_signed_int (register char *tline, int *val_return, int *sign_return);

/* will parse geometry string in X format with AS extensions
( --10 as -0-10 for example )  */
char         *
parse_geometry (register char *tline,
                int *x_return, int *y_return,
                unsigned int *width_return,
  				unsigned int *height_return,
				int* flags_return );

double parse_math(const char* str, char** endptr, double size);

char *string_from_int (int param);
char *hex_to_buffer_reverse(void *data, size_t bytes, char* buffer);
char *hex_to_buffer(void *data, size_t bytes, char* buffer);

#ifdef WORDS_BIGENDIAN
#define NUMBER2HEX(n,b) hex_to_buffer(&n,sizeof(n),b)
#else
#define NUMBER2HEX(n,b) hex_to_buffer_reverse(&n,sizeof(n),b)
#endif

char scan_for_hotkey (char *txt);

/* generic functions for parsing a list of items separated with single character into a list of strings : */
char *get_string_list_item (char *ptr, char **item_start, char **item_end, char separator);
char **compound_string2string_list (char *string, char separator, Bool duplicate, int *nitems_return);
char *string_list2compound_string (char **list, char separator);
Bool match_compound_string (char *string, char separator, char *what);

/* below is the subcase of above where items are coma-separated
   (for compatibility with older versions) : */
/* this allows for parsing of the comma separated items from single string
   in to the string list */
/* string list is terminated with NULL element and does not constitute
   single block of memory - each element has to be allocated and deallocated
   individually */
char *get_comma_item (char *ptr, char **item_start, char **item_end);
char **comma_string2list (char *string);
char *list2comma_string (char **list);
void destroy_string_list( char **list );

char *interpret_ascii_string( const char *str );

char *make_tricky_text( char *src );

#ifdef __cplusplus
}
#endif

#endif /* AS_PARSE_H_HEADER_INCLUDED */
