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

char *stripcpy (const char *source);
char *stripcpy2 (const char *source, int tab_sensitive);
char *stripcpy3 (const char *, Bool);
char *tokencpy (const char *source);
struct config *find_config (struct config *, const char *);

/* here we'll strip comments and whitespaces */
char *stripcomments (char *source);
char *strip_whitespace (char *str);
/* some useful string parsing functions 
 * (returns next position in string after parsed value) */
char *ReadIntValue (char *restofline, int *value);
char *ReadColorValue (char *restofline, char **color, int *len);
char *ReadFileName (char *restofline, char **fname, int *len);

/* will read space separated string and allocate space for it */
char *parse_token (const char *source, char **trg);
/* same for tab separated tokens */
char *parse_tab_token (const char *source, char **trg);
/* will parse function values with unit - usefull in AS command parsing */
char *parse_func_args (char *tline, char *unit, int *func_val);
char *string_from_int (int param);

#endif /* _PARSE_ */
