#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "../afterbase.h"
#include "../afterimage.h"
#include "common.h"

/* Usage:  ascheckttf [-f font] [-s size] [-t text]|[--glyphs <listof unicodes>]
 */

#define BEVEL_HI_WIDTH 20
#define BEVEL_LO_WIDTH 30
#define BEVEL_ADDON    (BEVEL_HI_WIDTH+BEVEL_LO_WIDTH)

void usage()
{
	printf( "  Usage:   ascheckttf [-h] [-f font] [-s size] [-t text]|[--unicode <listof unicodes>]"
			"[-S 3D_style] \n");
	printf( "  Where: font - TrueType font's filename\n");
	printf( "         size - size in points for TrueType fonts;\n");
	printf( "         text - text;\n");
	printf( "         unicode - comma separated list of unicode codes;\n");
	printf( "         3D_style - 3D style of text. "
			"One of the following:\n");
	printf( "             0 - plain 2D tetx, 1 - embossed, 2 - sunken, "
			"3 - shade above,\n");
	printf( "             4 - shade below, 5 - embossed thick, "
			"6 - sunken thick.\n");
	printf( "             7 - ouline above, 8 - ouline below, "
			"9 - full ouline.\n");


}

int main(int argc, char* argv[])
{
	char *font_name = NULL;
	int size = 32 ;
	char *unicode = NULL;
	UNICODE_CHAR* uc_ptr = NULL;
 	char *text = NULL;
	
	struct ASFontManager *fontman = NULL;
	struct ASFont  *font = NULL;
	int i ;
	int text_margin = size/2 ;
	char * font_path = NULL;
	ASGlyph** glyphs = NULL;
	
	/* see ASView.1 : */
	set_application_name( argv[0] );
#if (HAVE_AFTERBASE_FLAG==1)
	set_output_threshold(OUTPUT_LEVEL_DEBUG);
#endif

	if( argc == 1 )
		usage();
	else 
		for (i = 1 ; i < argc ; i++)
		{
			if (strncmp( argv[i], "-h", 2 ) == 0)
			{
				usage();
				return 0;
			}

			if (i+1 < argc)
			{
				if( strncmp( argv[i], "-f", 2 ) == 0 )
					font_name = argv[i+1] ;
				else if( strncmp( argv[i], "-s", 2 ) == 0 )
				{
					size = atoi(argv[i+1]);
					text_margin = size/2 ;
				}else if (strncmp( argv[i], "-t", 2 ) == 0)
					text = argv[i+1] ;
				else if (strncmp( argv[i], "--unicode", 9 ) == 0)
					unicode = argv[i+1] ;
			}
		}

	if (font_name == NULL)
	{
		show_error( "Font must be specified." );
		usage();
		return 1;
	}

	if (text == NULL && unicode == NULL)
	{
		show_error( "Either text or list of unicode must be specified." );
		usage();
		return 1;
	}
	
	if( getenv("FONT_PATH") != NULL ) 
	{
		font_path = safemalloc( strlen(getenv("FONT_PATH"))+1+2+1);
		sprintf( font_path, "%s:./", getenv("FONT_PATH") );
		
	}	 
	
	if( (fontman = create_font_manager( NULL, font_path, NULL )) != NULL )
		font = get_asfont( fontman, font_name, 0, size, ASF_Freetype );

	if( font == NULL )
	{
		show_error( "unable to load requested font \"%s\". ", font_name );
		return 1;
	}

	if (text)
	{
		glyphs = get_text_glyph_list (text, font, ASCT_Char, 0);
	}else /* unicode */
	{
		char *endp = unicode;

		i = 0;
		uc_ptr = safecalloc (strlen (unicode)+1, sizeof (UNICODE_CHAR));
		while ((uc_ptr[i++] = strtol(endp, &endp, 0)) > 0)
			if (*endp == ',' || *endp == ';') ++endp;

		glyphs = get_text_glyph_list ((char*)uc_ptr, font, ASCT_Unicode, 0);
		
	}
	for( i = 0 ; glyphs[i] ; ++i ) 
	{
		if (text)
			printf ("0x%8.8X ", (unsigned int)text[i]);
		else
			printf ("0x%8.8X ", (unsigned int)uc_ptr[i]);
			
		if (glyphs[i] == &(font->default_glyph))
			printf ("N 0 0\n");
		else
			printf ("Y %d %d\n", glyphs[i]->width, glyphs[i]->height);
	}
	
	release_font( font );
	destroy_font_manager( fontman, False );
    return 0 ;
}
