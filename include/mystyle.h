#ifndef _MYSTYLE_
#define _MYSTYLE_

/***********************************************************************
 *
 * afterstep style include file
 *
 ***********************************************************************/

/*
 * To use:
 * 1. set extern globals (MyName, dpy, screen)
 * 2. parse config file, calling mystyle_parse() when a "MyStyle" 
 *    keyword is seen
 * 3. call mystyle_fix_styles(), which will create the default style (if 
 *    necessary) and fill in unset style members
 * 4. call mystyle_find() to get style(s) by name; this pointer can be 
 *    safely stored for later use
 *
 * Styles are now ready for use.
 *
 * 5. when drawing using styles, normally call mystyle_get_global_gcs() 
 *    to get drawing GCs; for more control, call mystyle_set_gcs(), 
 *    which sets any arbitrary set of GCs (see Notes for caveat, however)
 *
 * Backward compatibility (replaces step 2, above):
 *
 * 2a. find (or create, if necessary) the appropriate style
 * 2b. convert the config line into MyStyle command(s)
 * 2c. call mystyle_parse_member() to parse the modified config line 
 *     into the MyStyle
 *
 * Notes:
 *  o it is always safe to create a style, even if that style will be 
 *    later used by the parsing code
 *  o mystyle_set_gcs() will set the GC foreground, background, font, 
 *    and fill style, and may set the tile, stipple, tile/stipple origin, 
 *    function, clip mask, and clip origin
 *  o if the GCs returned by mystyle_get_global_gcs() are modified, 
 *    change them back! otherwise, strange things could happen
 *  o mystyle_parse() and mystyle_parse_member() both need a pixmap path, 
 *    which should be a colon (:) delimited list of paths to search for 
 *    pixmaps (eg, "~/GNUstep/Library/Afterstep/backgrounds:.")
 *  o always call mystyle_fix_styles() after creating a style and before 
 *    using it! the MyStyle code assumes some style members are defined 
 *    (and it's mystyle_fix_styles()'s job to make sure they are)
 */

#if 0				/* Example */
FILE *fp;
char *buf;
   /* set externs */
fp = fopen (...);
buf = malloc (...);

while (fgets (buf, /* buf size */ , fp) != NULL)
  {
    if (!mystrncasecmp (buf, "MyStyle", 7))
      mystyle_parse (buf + 7, fp, pixmap_path, NULL);
    ...
  }

...

my_global_style = mystyle_find ("*MyName");
if (my_global_style == NULL)
  my_global_style = mystyle_new_with_name ("*MyName");
mystyle_fix_styles ();
#endif /* Example */

typedef struct icon_t
  {
    XImage *image;		/* XImage of pix, to reduce XGetImage() calls */
    Pixmap pix;			/* icon pixmap */
    Pixmap mask;		/* icon mask */
    int width;			/* icon width */
    int height;			/* icon height */
  }
icon_t;

/*
 * see comment to style_config in mystyle.c before changing this structure
 *
 * text_type 0: normal text
 * text_type 1: 3d effect #1
 * text_type 2: 3d effect #2
 */

typedef struct MyStyle
  {
    struct MyStyle *next;
    int set_flags;		/* == (user_flags | inherit_flags) */
    int user_flags;		/* options the user set */
    int inherit_flags;		/* inherited options */

    int flags;			/* options that consist of flags only */
    char *name;
    int text_style;
    MyFont font;
    ColorPair colors;
    ColorPair relief;
    int texture_type;
#ifndef NO_TEXTURE
    int max_colors;
    icon_t back_icon;		/* background pixmap */
    gradient_t gradient;	/* background gradient */
    XColor tint;
#endif
  }
MyStyle;

/* 
 * values < 16 are sequential control codes
 * values >= 16 are bit flags
 */
enum				/* MyStyle options */
  {
    F_ERROR = 0,		/* error condition */
    F_DONE = 1,			/* done parsing body */
    F_INHERIT = 2,		/* inherit from another style */
    F_FONT = (1 << 4),
    F_FORECOLOR = (1 << 5),
    F_BACKCOLOR = (1 << 6),
    F_MAXCOLORS = (1 << 7),
    F_BACKGRADIENT = (1 << 8),
    F_BACKPIXMAP = (1 << 9),
    F_TEXTSTYLE = (1 << 10),
    F_DRAWTEXTBACKGROUND = (1 << 11),
    F_BACKMULTIGRADIENT = (1 << 12),
    F_BACKTRANSPIXMAP = (1 << 13)	/* should never be set unless F_BACKPIXMAP is set!! */
  };

/* serice functions */
GC CreateTintGC (Drawable d, unsigned long tintColor, int function);

/* just a nice error printing function */
void mystyle_error (char *format, char *name, char *text2);

/* these functions change the global style */
void mystyle_set_global_gcs (MyStyle * style);
void mystyle_set_gcs (MyStyle * style, GC foreGC, GC backGC, GC reliefGC, GC shadowGC);
void mystyle_get_global_gcs (MyStyle * style, GC * foreGC, GC * backGC, GC * reliefGC, GC * shadowGC);
void mystyle_get_text_geometry (MyStyle * style, const char *str, int len, int *width, int *height);
void mystyle_draw_text (Window w, MyStyle * style, const char *text, int x, int y);
void mystyle_draw_vertical_text (Window w, MyStyle * style, const char *text, int ix, int y);

void mystyle_fix_styles (void);
Pixmap mystyle_make_pixmap (MyStyle * style, int width, int height, Pixmap cache);
Pixmap mystyle_make_pixmap_overlay (MyStyle * style, int root_x, int root_y, int width, int height, Pixmap cache);
icon_t mystyle_make_icon (MyStyle * style, int width, int height, Pixmap cache);
icon_t mystyle_make_icon_overlay (MyStyle * style, int root_x, int root_y, int width, int height, Pixmap cache);
void mystyle_set_window_background (Window w, MyStyle * style);
MyStyle *mystyle_new (void);
MyStyle *mystyle_new_with_name (char *name);
void mystyle_delete (MyStyle * style);
void mystyle_merge_styles (MyStyle * parent, MyStyle * child, Bool override, Bool copy);
MyStyle *mystyle_find (const char *name);
MyStyle *mystyle_find_or_default (const char *name);

void mystyle_parse (char *tline, FILE * fd, char **junk, int *junk2);
int mystyle_parse_member (MyStyle * style, char *str, const char *PixmapPath);
void mystyle_parse_set_style (char *text, FILE * fd, char **style, int *junk2);
int mystyle_parse_old_gradient (int type, const char *color1, const char *color2, gradient_t * gradient);

void set_func_arg (char *text, FILE * fd, char **value, int *junk);

extern MyStyle *mystyle_first;

#endif /* _MYSTYLE_ */
