#ifndef MYSTRING_H_HEADER_INCLUDED
#define MYSTRING_H_HEADER_INCLUDED

int mystrcasecmp (const char *s1, const char *s2);
int mystrncasecmp (const char *s1, const char *s2, size_t n);
#ifndef mystrdup
char *mystrdup (const char *str);
#endif
#ifndef mystrndup
char *mystrndup (const char *str, size_t n);
#endif

/* use it to set string as well as flags and deallocate memory
   referred to by the pointer */
void set_string_value (char **target, char *string,
		       unsigned long *set_flags, unsigned long flag);

#endif /* MYSTRING_H_HEADER_INCLUDED */
