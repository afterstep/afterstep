#ifndef FONT_H
#define FONT_H

#ifdef I18N
#ifdef __STDC__
#define XTextWidth(x,y,z)      XmbTextEscapement(x ## set,y,z)
#else
#define XTextWidth(x,y,z)      XmbTextEscapement(x/**/set,y,z)
#endif
#define XDrawString(t,u,v,w,x,y,z) XmbDrawString(t,u,FONTSET,v,w,x,y,z)
#define XDrawImageString(t,u,v,w,x,y,z) XmbDrawImageString(t,u,FONTSET,v,w,x,y,z)
#endif

struct ASFont;
typedef struct MyFont
  {
    char *name;				/* name of the font */
	struct ASFont *as_font ;   	/* libAfterImage's font */
    XFontStruct *font;		/* font structure */
#ifdef I18N
    XFontSet fontset;
#endif
    int height;				/* height of the font */
    int width;				/* width of the font */
    int y;					/* Y coordinate to draw characters */
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


#endif /* FONT_H */
