/****************************************************************************
 * Copyright (c) 2003 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/

#define LOCAL_DEBUG

#include "../../configure.h"

#include "asinternals.h"

#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../libAfterStep/session.h"
#include "../../libAfterConf/afterconf.h"

/* Overview:
 * Theme could be represented by 3 different things :
 * 1) single installation script ( combines standard looks and feels and image files )
 * 2) theme tarball ( could be gzipped or bzipped ) that contains :
 *		look file, feel file, theme install script, theme file with config for modules
 *		It could also include TTF fonts and images.
 * 3) theme tarball ( old style ) without installation script :
 * 	    contains everything above but installation script.
 *
 * While installing themes we do several things :
 * 1) copy feel and look files into  afterstep/looks afterstep/feels directories
 * 2) copy all background files into afterstep/backgrounds
 * 3) copy remaining icon files into afterstep/desktop/icons
 * 4) copy remaining TTF  files into afterstep/desktop/fonts
 * 5) copy theme file into non-configurable/theme
 * 6) If ordered so in installation script we change look and feel to theme look and feel
 * 7) If ordered so in installation script we change backgrounds for different desktops
 * 8) finally we restart AS, but that is different story.
 *
 */

typedef enum {
	AST_ThemeScript = 0,
	AST_ThemeTarBz2,
	AST_ThemeTarGz,
	AST_ThemeTar,
	AST_ThemeBad
}ASThemeFileType;

ASThemeFileType
detect_theme_file_type( const char * filename )
{
	ASThemeFileType type = AST_ThemeBad ;
	FILE *f ;

	if( filename )
	{
#ifdef __CYGWIN__
		if( (f = fopen( filename, "rb" )) != NULL )
#else
		if( (f = fopen( filename, "r" )) != NULL )
#endif
		{
			char buf[17] = "";
			if( fread( &(buf[0]), 16, 1, f ) == 16 )
			{
				buf[16] = '\0' ;
				if( strncmp( &(buf[0]), "BZh", 3 ) == 0 )
					type = AST_ThemeTarBz2 ;
				else if( buf[0] == (char)0x1F && buf[1] == (char)0x8b )
					type = AST_ThemeTarGz ;
				else if( mystrcasecmp( &buf[0], "#AfterStep theme" ) == 0 )
					type = AST_ThemeScript ;
				else
				{
					int i;
					type = AST_ThemeTar ;
					for( i = 0 ; i < 17 && buf[i] != '\0' ; ++i )
						if( !isprint(buf[i]) )
						{
							type = AST_ThemeBad ;
							break;
						}
				}
			}
			fclose( f );
		}
	}
	return type;
}

Bool unbzip2_file( const char *src, const char *dst )
{

	return False;
}

Bool ungzip_file( const char *src, const char *dst )
{

	return False;
}

Bool untar_file( const char *src, const char *dst )
{

	return False;
}

ComplexFunction *
build_theme_script( const char *theme_dir )
{
	ComplexFunction *ts = NULL ;

	return ts ;
}

void
fix_theme_script( ComplexFunction *install_func, const char *theme_dir )
{


}

Bool
install_theme_file( const char *src, const char *dst )
{
	ASThemeFileType type = detect_theme_file_type( src );
	char *theme_tarball = NULL ;
	char *theme_dir = NULL ;
	char *theme_script_fname = NULL ;

	if( type == AST_ThemeBad )
	{
		show_error( "missing or incorrect format of the theme file \"%s\"", src );
		return False;
	}
	if( type != AST_ThemeScript )
	{
        char *tmpdir = getenv("TMPDIR");
        static char *default_tmp_dir = "/tmp" ;
		int themedir_len ;
        if( tmpdir != NULL )
            if( CheckDir( tmpdir ) < 0 )
                tmpdir = NULL ;

        if( tmpdir == NULL )
            tmpdir = default_tmp_dir ;
		themedir_len = strlen(tmpdir)+11+32;
        theme_dir = safemalloc (themedir_len+1);
        sprintf (theme_dir, "%s/astheme.%d", tmpdir, getpid() );
		if( mkdir( theme_dir, 700 ) == -1 )
		{
			if( errno != EEXIST  )
			{
				show_error( "unable to create staging directory for theme files \"%s\"", theme_dir );
				free( theme_dir );
				return False;
			}
		}
		if( type == AST_ThemeTarBz2 && type == AST_ThemeTarGz )
		{
			theme_tarball = safemalloc( themedir_len + 4 + 1 );
			sprintf (theme_dir, "%s.tar", theme_dir );
		}
	}else
	{
		char *tmp = strrchr( src, '/' );
		if( tmp != NULL )
			theme_dir = mystrndup( src, tmp - src );
		else
			theme_dir = mystrdup( "" );
	}

	if( theme_tarball == NULL  )
		theme_tarball = mystrdup( src );

	if( type == AST_ThemeTarBz2 )
	{	/* need to unbzip2 file */
		if( unbzip2_file( src, theme_tarball ) )
			type = AST_ThemeTar ;
		else
			type = AST_ThemeBad ;
	}else if( type == AST_ThemeTarGz )
	{	/* need to ungzip file */
		if( ungzip_file( src, theme_tarball ) )
			type = AST_ThemeTar ;
		else
			type = AST_ThemeBad ;
	}

	if( type == AST_ThemeTar )
	{	/* need to untar all the files into dir */
		if( untar_file( theme_tarball, theme_dir ) )
			type = AST_ThemeScript ;
		else
			type = AST_ThemeBad ;
	}

	if( type == AST_ThemeScript )
	{
		/* step 1: parsing theme script file */
		ComplexFunction *install_func = NULL ;
		if( theme_script_fname && CheckFile( theme_script_fname ) == 0 )
			install_func = ParseComplexFunctionFile ( theme_script_fname, "afterstep");

		if( install_func == NULL )
			install_func = build_theme_script( theme_dir );
		else if( theme_dir[0] != '\0' )
			fix_theme_script( install_func, theme_dir );  /* appends theme_dir to filenames */

		if( install_func == NULL )
			show_error( "Theme contains no recognizable files. Installation failed" );
		else
		{

		}
	}

	return True;
}

