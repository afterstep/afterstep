#ifndef _MYSTYLE_
#define _MYSTYLE_

#include "../libAfterImage/afterimage.h"
#include "font.h"

#ifdef __cplusplus
extern "C" {
#endif

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

struct ASImage;
struct ASGradient;

#include "myicon.h"

/*
 * see comment to style_config in mystyle.c before changing this structure
 *
 * text_type 0: normal text
 * text_type 1: 3d effect #1
 * text_type 2: 3d effect #2
 */

typedef struct MyStyle
  {
    unsigned long magic ;

    ASHashTable *owner ;

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
    ASGradient gradient;	/* background gradient */
    ARGB32 tint;
#endif
  }
MyStyle;

/*
 * values < 16 are sequential control codes
 * values >= 16 are bit flags
 */
enum				/* MyStyle options */
  {
    F_ERROR         = 0,        /* error condition */
    F_DONE          = 1,         /* done parsing body */
    F_INHERIT       = 2,      /* inherit from another style */
    F_FONT          = (1 << 4),
    F_FORECOLOR     = (1 << 5),
    F_BACKCOLOR     = (1 << 6),
    F_MAXCOLORS     = (1 << 7),
    F_BACKGRADIENT  = (1 << 8),
    F_BACKPIXMAP    = (1 << 9),
    F_TEXTSTYLE     = (1 << 10),
    F_DRAWTEXTBACKGROUND = (1 << 11),
    F_BACKMULTIGRADIENT  = (1 << 12),
    F_BACKTRANSPIXMAP    = (1 << 13),   /* should never be set unless F_BACKPIXMAP is set!! */
    F_EXTERNAL_BACKPIX   = (1 << 14),   /* indicates that pixmap has been created by some other application and should not be freed */
    F_EXTERNAL_BACKMASK  = (1 << 15),
	F_TRANSPARENT		 = (1 << 16)           /* if set MyStyle is transparent, and everything that is drawn with it,
												* must be updated on background changes */
  };

#define TransparentMS(style)  (get_flags((style)->set_flags, F_TRANSPARENT))


  enum				/* texture types */
  {
    TEXTURE_SOLID = 0,

	TEXTURE_GRADIENT_START,
    TEXTURE_GRADIENT = TEXTURE_GRADIENT_START,
    TEXTURE_HGRADIENT,
    TEXTURE_HCGRADIENT,
    TEXTURE_VGRADIENT,
    TEXTURE_VCGRADIENT,
    TEXTURE_GRADIENT_TL2BR, /* 6 */
    TEXTURE_GRADIENT_BL2TR,
    TEXTURE_GRADIENT_T2B,
    TEXTURE_GRADIENT_L2R,  /* 9 */
    TEXTURE_GRADIENT_END = TEXTURE_GRADIENT_L2R,

	TEXTURE_TEXTURED_START = 125,
    TEXTURE_SHAPED_SCALED_PIXMAP = TEXTURE_TEXTURED_START,
    TEXTURE_SHAPED_PIXMAP,
    TEXTURE_SCALED_PIXMAP = 127,
    TEXTURE_PIXMAP,
    TEXTURE_TRANSPARENT = 129, /* tninted really */
    TEXTURE_TRANSPIXMAP = 130, /* 130-145 represent different
	                              blending methods from libAfterImage */
	TEXTURE_TRANSPIXMAP_ALLANON = TEXTURE_TRANSPIXMAP,
	TEXTURE_TRANSPIXMAP_ALPHA,
	TEXTURE_TRANSPIXMAP_TINT,
	TEXTURE_TRANSPIXMAP_ADD,
	TEXTURE_TRANSPIXMAP_SUB,
	TEXTURE_TRANSPIXMAP_DIFF,
	TEXTURE_TRANSPIXMAP_DARKEN,
	TEXTURE_TRANSPIXMAP_LIGHTEN,
	TEXTURE_TRANSPIXMAP_SCREEN,
	TEXTURE_TRANSPIXMAP_OVERLAY,
	TEXTURE_TRANSPIXMAP_HUE,
	TEXTURE_TRANSPIXMAP_SATURATE,
	TEXTURE_TRANSPIXMAP_VALUE,
	TEXTURE_TRANSPIXMAP_COLORIZE,
	TEXTURE_TRANSPIXMAP_DISSIPATE,

	TEXTURE_TRANSPIXMAP_END = 148,

    TEXTURE_TRANSPARENT_TWOWAY = 149, /* tinted both ways - lightened or darkened */

	TEXTURE_SCALED_TRANSPIXMAP = 150,
	TEXTURE_SCALED_TPM_ALLANON = TEXTURE_SCALED_TRANSPIXMAP,
	TEXTURE_SCALED_TPM_ALPHA,
	TEXTURE_SCALED_TPM_TINT,
	TEXTURE_SCALED_TPM_ADD,
	TEXTURE_SCALED_TPM_SUB,
	TEXTURE_SCALED_TPM_DIFF,
	TEXTURE_SCALED_TPM_DARKEN,
	TEXTURE_SCALED_TPM_LIGHTEN,
	TEXTURE_SCALED_TPM_SCREEN,
	TEXTURE_SCALED_TPM_OVERLAY,
	TEXTURE_SCALED_TPM_HUE,
	TEXTURE_SCALED_TPM_SATURATE,
	TEXTURE_SCALED_TPM_VALUE,
	TEXTURE_SCALED_TPM_COLORIZE,
	TEXTURE_SCALED_TPM_DISSIPATE,
	TEXTURE_SCALED_TRANSPIXMAP_END = 168,
	TEXTURE_TEXTURED_END = TEXTURE_SCALED_TRANSPIXMAP_END,

    TEXTURE_BUILTIN = 255
  };


ARGB32 GetShadow (ARGB32);
ARGB32 GetHilite (ARGB32);
ARGB32 GetAverage (ARGB32 foreground, ARGB32 background);

/* serice functions */
GC CreateTintGC (Drawable d, unsigned long tintColor, int function);

/* just a nice error printing function */
void mystyle_error (char *format, char *name, char *text2);

merge_scanlines_func mystyle_translate_texture_type( int texture_type );

int mystyle_translate_grad_type( int type );
ASImage *mystyle_draw_text_image( MyStyle *style, const char *text, unsigned long encoding );
unsigned int mystyle_get_font_height( MyStyle *style );

void mystyle_list_fix_styles (ASHashTable *list);
void mystyle_fix_styles (void);


ASImage *mystyle_make_image( MyStyle * style, int root_x, int root_y, int width, int height, int flip );
Pixmap mystyle_make_pixmap (MyStyle * style, int width, int height, Pixmap cache);
Pixmap mystyle_make_pixmap_overlay (MyStyle * style, int root_x, int root_y, int width, int height, Pixmap cache);
icon_t mystyle_make_icon (MyStyle * style, int width, int height, Pixmap cache);
icon_t mystyle_make_icon_overlay (MyStyle * style, int root_x, int root_y, int width, int height, Pixmap cache);
void mystyle_set_window_background (Window w, MyStyle * style);

MyStyle *mystyle_list_new (struct ASHashTable *list, char *name);
MyStyle *mystyle_new_with_name (char *name);

void mystyle_list_destroy_all( ASHashTable **plist );
void mystyle_destroy_all();

void mystyle_merge_font( MyStyle *style, MyFont *font, Bool override, Bool copy);
void mystyle_merge_styles (MyStyle * parent, MyStyle * child, Bool override, Bool copy);

MyStyle *mystyle_list_find (struct ASHashTable *list, const char *name);
MyStyle *mystyle_list_find_or_default (struct ASHashTable *list, const char *name);

MyStyle *mystyle_find (const char *name);
MyStyle *mystyle_find_or_default (const char *name);

void mystyle_parse (char *tline, FILE * fd, char **junk, int *junk2);
int mystyle_parse_member (MyStyle * style, char *str);
void mystyle_parse_set_style (char *text, FILE * fd, char **style, int *junk2);
int mystyle_parse_old_gradient (int type, ARGB32 c1, ARGB32 c2, ASGradient *gradient);
void mystyle_inherit_font (MyStyle * style, struct MyFont * font);
void mystyle_merge_colors (MyStyle * style, int type, char *fore, char *back, char *gradient, char *pixmap);

void set_func_arg (char *text, FILE * fd, char **value, int *junk);
ASImageBevel *mystyle_make_bevel (MyStyle *style, ASImageBevel *bevel, int hilite, Bool reverse);

extern MyStyle *mystyle_first;

#ifdef __cplusplus
}
#endif


#endif /* _MYSTYLE_ */
