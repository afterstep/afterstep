#ifndef MYSTRING_H_HEADER_INCLUDED
#define MYSTRING_H_HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

int mystrcasecmp (const char *s1, const char *s2);
int mystrncasecmp (const char *s1, const char *s2, size_t n);
int mystrcmp (const char *s1, const char *s2);

#ifndef mystrdup
char *mystrdup (const char *str);
#endif
#ifndef mystrndup
char *mystrndup (const char *str, size_t n);
#endif

/* using macro causes excessive compiler warnings. Stupid GCC 4 !!!!! */
/* #define destroy_string(ps) do{ if(ps && *(ps)){free(*(ps)); *(ps)=NULL;}}while(0) */

void destroy_string(char **ps);

/* use it to set string as well as flags and deallocate memory
   referred to by the pointer */
void set_string (char **target, char *string);

#ifdef __cplusplus
}
#endif

#endif /* MYSTRING_H_HEADER_INCLUDED */
