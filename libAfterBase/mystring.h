#ifdef MYSTRING_H_HEADER_INCLUDED
#define MYSTRING_H_HEADER_INCLUDED

int mystrcasecmp (const char *s1, const char *s2);
int mystrncasecmp (const char *s1, const char *s2, size_t n);
char         *mystrdup (const char *str);
char         *mystrndup (const char *str, size_t n);


#endif /* MYSTRING_H_HEADER_INCLUDED */