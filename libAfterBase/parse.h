#ifndef _PARSE_
#define _PARSE_

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

char *parse_argb_color( char *color, CARD32 *pargb );

char *find_doublequotes (char *ptr);
char *stripcpy (const char *source);
char *stripcpy2 (const char *source, int tab_sensitive);
char *stripcpy3 (const char *, Bool);
char *tokencpy (const char *source);
char *tokenskip( char *start, unsigned int n_tokens );
struct config *find_config (struct config *, const char *);
int quotestr (char *dest, const char *src, int maxlen);

/* here we'll strip comments and whitespaces */
char *stripcomments (char *source);
char *strip_whitespace (char *str);

/* will read space separated string and allocate space for it */
char *parse_token (const char *source, char **trg);
/* same for tab separated tokens */
char *parse_tab_token (const char *source, char **trg);
/* parses filename, optionally enclosed in doublequotes */
char *parse_filename (const char *source, char **trg);
/* will parse function values with unit - usefull in AS command parsing */
char *parse_func_args (char *tline, char *unit, int *func_val);

char *string_from_int (int param);
char *hex_to_buffer_reverse(void *data, size_t bytes, char* buffer);
char *hex_to_buffer(void *data, size_t bytes, char* buffer);

#ifdef WORDS_BIGENDIAN
#define NUMBER2HEX(n,b) hex_to_buffer(&n,sizeof(n),b)
#else
#define NUMBER2HEX(n,b) hex_to_buffer_reverse(&n,sizeof(n),b)
#endif

char scan_for_hotkey (char *txt);

/* this allows for parsing of the comma separated items from single string
   in to the string list */
/* string list is terminated with NULL element and does not constitute
   single block of memory - each element has to be allocated and deallocated
   individually */
char *get_comma_item (char *ptr, char **item_start, char **item_end);
char **comma_string2list (char *string);
char *list2comma_string (char **list);

char *make_tricky_text( char *src );


#endif /* _PARSE_ */
