#ifndef ASFONT_HEADER_ICLUDED
#define ASFONT_HEADER_ICLUDED

#define MAX_GLYPHS_PER_FONT  2048

typedef enum
{
	ASF_X11 = 0,
	ASF_Freetype,
	ASF_GuessWho
}ASFontType;

struct ASFontManager;
struct ASFont;

#ifdef I18N
/* size of the UTF-8 encoded character is based on value of the first byte : */
#define CHAR_SIZE(c) 	(((c)&0x80)?(((c)&0x40)?(((c)&0x20)?(((c)&0x10)?5:4):3):2):1)
#else
#define CHAR_SIZE(c) 	1
#endif

#ifdef INCLUDE_ASFONT_PRIVATE
typedef struct ASGlyph
{
	CARD8 		   *pixmap ;
	unsigned short 	width, height ;	  /* meaningfull width and height of the glyphs pixmap */
	short  lead ;			          /* distance from previous glyph */
	short  ascend, descend ;          /* distance of the top of the glyph from the baseline */
}ASGlyph;

typedef struct ASGlyphRange
{
	unsigned long   min_char, max_char;        /* for some locales that would be sufficient to
												* simply set range of characteres supported
												* by font */
	ASGlyph *glyphs;
	struct ASGlyphRange *below, *above;
}ASGlyphRange;


typedef struct ASFont
{
	unsigned long 	magic ;

	struct ASFontManager *fontman;              /* our owner */
	char 				 *name;

	ASFontType  	type ;
#ifdef HAVE_FREETYPE
	FT_Face  		ft_face;                    /* free type font handle */
#endif
	ASGlyphRange   *codemap;
	ASGlyph         default_glyph;

	unsigned int 	max_height, max_ascend, space_size;
#define LEFT_TO_RIGHT    1
#define RIGHT_TO_LEFT   -1
	int 			pen_move_dir ;
}ASFont;

typedef struct ASFontManager
{
	char 	   *font_path ;
#ifdef HAVE_FREETYPE
	Bool 		ft_ok ;
	FT_Library  ft_library;                    /* free type library handle */
#endif

	ASHashTable *fonts_hash ;

	size_t 		 unicode_used;
	CARD32		*local_unicode;                /* list of unicodes from current locale
												* - we use it to limit number of glyphs
												* we load */
}ASFontManager;

#endif

struct ASFontManager *create_font_manager( const char * font_path, struct ASFontManager *reusable_memory );
void    destroy_font_manager( struct ASFontManager *fontman, Bool reusable );
struct ASFont *open_freetype_font( struct ASFontManager *fontman, const char *font_string, int face_no, int size, Bool verbose);
struct ASFont *open_X11_font( struct ASFontManager *fontman, const char *font_string);
struct ASFont *get_asfont( struct ASFontManager *fontman, const char *font_string, int face_no, int size, ASFontType type );
void    print_asfont( FILE* stream, struct ASFont* font);
void 	print_asglyph( FILE* stream, struct ASFont* font, unsigned long c);
void    destroy_font( struct ASFont *font );

struct ASImage *draw_text( const char *text, struct ASFont *font, int compression );




#endif
