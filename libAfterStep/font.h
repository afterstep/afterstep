#ifndef FONT_H
#define FONT_H

#ifdef __cplusplus
extern "C" {
#endif

struct ASFont;
typedef struct MyFont
  {
    char *name;				/* name of the font */
	struct ASFont *as_font ;   	/* libAfterImage's font */
  }
MyFont;

#if defined(LOG_FONT_CALLS) && defined(DEBUG_ALLOCS)
Bool l_load_font (const char *, int, const char *, MyFont *);
void l_unload_font (const char *, int, MyFont *);
#define load_font(a,b)	l_load_font(__FUNCTION__,__LINE__,a,b)
#define unload_font(a)	l_unload_font(__FUNCTION__,__LINE__,a)
#else
Bool load_font (const char *, MyFont *);
void unload_font (MyFont *);
#endif

#ifdef __cplusplus
}
#endif


#endif /* FONT_H */
