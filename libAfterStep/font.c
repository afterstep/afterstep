#include "../configure.h"
#include "asapp.h"
#include "screen.h"
#include "../libAfterImage/afterimage.h"

const char   *default_font = "fixed";

/***************************************************************
 *
 * load a font into a MyFont structure
 * if name == NULL, loads default font
 *
 **************************************************************/
#if defined(LOG_FONT_CALLS) && defined(DEBUG_ALLOCS)
Bool
l_load_font (const char *file, int line, const char *name, MyFont * font)
#else
Bool
load_font (const char *name, MyFont * font)
#endif
{
	int           font_size = 14;
	char 		 *clean_name = (char*)name ;

#if defined(LOG_FONT_CALLS) && defined(DEBUG_ALLOCS)
	log_call (file, line, "load_font", name);
#endif
	if (Scr.font_manager == NULL)
	{
		char         *path = getenv ("FONT_PATH");

		if (path == NULL)
			path = getenv ("PATH");
		Scr.font_manager = create_font_manager (dpy, path, NULL);
	}

	if( clean_name != NULL )
	{
		int i = 0 ;
		register char *ptr = clean_name ;
		while( ptr[i] ) ++i ;
		while( --i >= 0 )
			if( !isdigit(ptr[i]) )
				break;
		if( (isspace( ptr[i] ) || ptr[i] == '-') && ptr[i+1] )
		{
			font_size = atoi( &(ptr[i+1]) );
			while( i > 0 && isspace(ptr[i-1]) )	--i ;
			clean_name = mystrndup( name, i );
		}
	}
	if( clean_name != NULL )
	{
		if( (font->as_font = get_asfont (Scr.font_manager, clean_name, 0, font_size, ASF_Freetype)) != NULL )
			show_progress( "Successfully loaded freetype font \"%s\"", clean_name );
	}
	if( font->as_font == NULL && name != NULL )
	{
		if( (font->as_font = get_asfont (Scr.font_manager, name, 0, font_size, ASF_GuessWho)) != NULL )
			show_progress( "Successfully loaded font \"%s\"", name );
	}
	if( font->as_font == NULL )
	{
		font->as_font = get_asfont (Scr.font_manager, default_font, 0, font_size, ASF_GuessWho);
		show_warning( "failed to load font \"%s\" - using default instead", name );
	}
	if( clean_name && clean_name != name )
		free( clean_name );
    if ( font->as_font != NULL )
		font->name = mystrdup (name);
	return (font->as_font != NULL);
}

/***************************************************************
 *
 * unload a font from a MyFont structure
 *
 **************************************************************/
#if defined(LOG_FONT_CALLS) && defined(DEBUG_ALLOCS)
void
l_unload_font (const char *file, int line, MyFont * font)
#else
void
unload_font (MyFont * font)
#endif
{
#if defined(LOG_FONT_CALLS) && defined(DEBUG_ALLOCS)
	log_call (file, line, "unload_font", font->name);
#endif
	if (font->as_font)
	{
		release_font (font->as_font);
		font->as_font = NULL;
	}

	if (font->name != NULL)
		free (font->name);
	font->name = NULL;
}
