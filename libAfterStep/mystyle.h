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

struct ScreenInfo;
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
	unsigned long magic;

	ASHashTable *owner;

	ASFlagType set_flags;		/* == (user_flags | inherit_flags) */
	ASFlagType user_flags;		/* options the user set */
	ASFlagType inherit_flags;		/* inherited options */

	ASFlagType flags;			/* options that consist of flags only */
	char *name;
	int text_style;
	MyFont font;
	ColorPair colors;
	ColorPair relief;
	int texture_type;
	icon_t back_icon;		/* background pixmap */
	ASGradient gradient;	/* background gradient */
	ARGB32 tint;
	int slice_x_start, slice_x_end, slice_y_start, slice_y_end;
	int blur_x, blur_y;
		
	struct MyStyle *overlay;
	int overlay_type;

}MyStyle;

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
	F_SLICE	     	= (1 << 7),
	F_BACKGRADIENT  = (1 << 8),
	F_BACKPIXMAP    = (1 << 9),
	F_TEXTSTYLE     = (1 << 10),
	F_DRAWTEXTBACKGROUND = (1 << 11),
	F_BACKMULTIGRADIENT  = (1 << 12),
	F_BACKTRANSPIXMAP    = (1 << 13),   /* should never be set unless F_BACKPIXMAP is set!! */
	F_EXTERNAL_BACKPIX   = (1 << 14),   /* indicates that pixmap has been created by some other application and should not be freed */
	F_EXTERNAL_BACKMASK  = (1 << 15),
	F_TRANSPARENT		 = (1 << 16),   /* if set MyStyle is transparent, and everything that is drawn with it,
										 * must be updated on background changes */
  
	F_OVERLAY		     = (1 << 17),           
	F_BLUR			     = (1 << 18)
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
	TEXTURE_OLD_GRADIENT_END = TEXTURE_VCGRADIENT,
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
void mystyle_get_text_size (MyStyle * style, const char *text, unsigned int *width, unsigned int *height );

void mystyle_list_fix_styles (ASHashTable *list);
void mystyle_fix_styles (void);


ASImage *mystyle_make_image( MyStyle * style, int root_x, int root_y, int width, int height, int flip );
ASImage *mystyle_crop_image(MyStyle * style, int root_x, int root_y, int crop_x, int crop_y, int width, int height, int scale_width, int scale_height, int flip );


MyStyle *mystyle_list_new (struct ASHashTable *list, char *name);
MyStyle *mystyle_new_with_name (char *name);

void mystyle_list_destroy_all( ASHashTable **plist );
void mystyle_destroy_all();

void mystyle_merge_font( MyStyle *style, MyFont *font, Bool override);
void mystyle_merge_styles (MyStyle * parent, MyStyle * child, Bool override, Bool copy);

MyStyle *mystyle_list_find (struct ASHashTable *list, const char *name);
MyStyle *mystyle_list_find_or_default (struct ASHashTable *list, const char *name);

MyStyle *mystyle_find (const char *name);
MyStyle *mystyle_find_or_default (const char *name);

void mystyle_free_back_icon( MyStyle *style );


void mystyle_parse_set_style (char *text, FILE * fd, char **style, int *junk2);
int mystyle_parse_old_gradient (int type, ARGB32 c1, ARGB32 c2, ASGradient *gradient);
void mystyle_inherit_font (MyStyle * style, struct MyFont * font);
void mystyle_merge_colors (MyStyle * style, int type, char *fore, char *back, char *gradient, char *pixmap);

void set_func_arg (char *text, FILE * fd, char **value, int *junk);
ASImageBevel *mystyle_make_bevel (MyStyle *style, ASImageBevel *bevel, int hilite, Bool reverse);

struct ASImage *grab_root_asimage( struct ScreenInfo *scr, Window target, Bool screenshot );

extern MyStyle *mystyle_first;

#ifdef __cplusplus
}
#endif


#endif /* _MYSTYLE_ */
