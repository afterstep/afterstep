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

#ifdef INCLUDE_ASFONT_PRIVATE
typedef struct ASGlyph
{
	CARD8 		   *pixmap ;
	unsigned short 	width, height ;	  /* meaningfull width and height of the glyphs pixmap */
	unsigned short  lead ;			  /* distance from previous glyph */
	short  descend ;                  /* distance of the bottom of the g;yph from the baseline */
}ASGlyph;

typedef struct ASFont
{
	unsigned long 	magic ;

	struct ASFontManager *fontman;              /* our owner */

	ASFontType  	type ;
#ifdef HAVE_FREETYPE
	FT_Face  		ft_face;                    /* free type font handle */
#endif
	ASGlyph		   *glyphs;
	size_t          glyphs_num;
	unsigned int 	max_height;
}ASFont;

typedef struct ASFontManager
{
	char 	   *font_path ;
#ifdef HAVE_FREETYPE
	Bool 		ft_ok ;
	FT_Library  ft_library;                    /* free type library handle */
#endif

	ASHashTable *fonts_hash ;
}ASFontManager;

#endif

struct ASFontManager *create_font_manager( const char * font_path, struct ASFontManager *reusable_memory );
void    destroy_font_manager( struct ASFontManager *fontman, Bool reusable );
struct ASFont *open_freetype_font( struct ASFontManager *fontman, const char *font_string, int face_no, int size, Bool verbose);
struct ASFont *open_X11_font( struct ASFontManager *fontman, const char *font_string);
struct ASFont *get_asfont( struct ASFontManager *fontman, const char *font_string, int face_no, int size, ASFontType type );
void    print_asfont( FILE* stream, struct ASFont* font);
void 	print_asglyph( FILE* stream, struct ASFont* font, unsigned int glyph_index);
void    destroy_font( struct ASFont *font );


#endif
