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

	LOCAL_DEBUG_OUT( "theme file \"%s\"", filename );
	if( filename )
	{
#ifdef __CYGWIN__
		if( (f = fopen( filename, "rb" )) != NULL )
#else
		if( (f = fopen( filename, "r" )) != NULL )
#endif
		{
			char buf[17] = "";
			LOCAL_DEBUG_OUT( "theme file opened - reading 16 bytes ...%s","");
			if( fread( &(buf[0]), 1, 16, f ) == 16 )
			{
				buf[16] = '\0' ;
				LOCAL_DEBUG_OUT( "read data is \"%s\"", &(buf[0]));
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

Bool untar_file( const char *src, const char *dst, ASThemeFileType type )
{
	int i ;
	int src_len = strlen( (char*)src );
	int dst_len = strlen( (char*)dst );
	char * command_line = NULL ;
	char * tarball = (char*)src ;
	int tarball_len = src_len ;
	Bool success = False ;

	if( type != AST_ThemeTar )
	{
		tarball_len = dst_len + 4 ;
		tarball = safemalloc( tarball_len + 1 );
		sprintf( tarball, "%s.tar", dst );
	}

	if( type == AST_ThemeTarBz2 )
	{
		command_line = safemalloc( 11 + src_len + 15 + tarball_len + 1  + 1 );
		sprintf( command_line, "bzip2 -dc \"%s\" > \"%s\"", src, tarball );
	}else if( type == AST_ThemeTarGz )
	{
		command_line = safemalloc( 10 + src_len + 15 + tarball_len + 1 + 1 );
		sprintf( command_line, "gzip -dc \"%s\" > \"%s\"", src, tarball );
	}
	if( command_line )
	{
		LOCAL_DEBUG_OUT( "command line is \"%s\"", command_line );
		spawn_child( command_line, TAR_SINGLETON_ID, 0, 0, 0, True, False, NULL );
		free( command_line );
		command_line = NULL ;
		for( i = 3000 ; i >= 0 ; --i )
		{
			sleep_a_millisec(100);
			if( check_singleton_child (TAR_SINGLETON_ID, False) == 0 )
			{
				type = AST_ThemeTar ;
				break;
			}
		}
	}

	if( type == AST_ThemeTar )
	{
		command_line = safemalloc( 7 + dst_len + 9 + tarball_len + 1 + 1 );
		sprintf( command_line, "tar -C\"%s\" -x -f \"%s\"", dst, tarball );
		LOCAL_DEBUG_OUT( "command line is \"%s\"", command_line );
		spawn_child( command_line, TAR_SINGLETON_ID, 0, 0, 0, True, False, NULL );
		free( command_line );
		for( i = 3000 ; i >= 0 ; --i )
		{
			sleep_a_millisec(100);
			if( check_singleton_child (TAR_SINGLETON_ID, False) == 0 )
			{
				sleep_a_millisec(2000);        /* 2 sec delay for some reason */
				break;
			}
		}
		success = (i >= 0);
	}
	if( tarball != src )
		free( tarball );

	return success;
}

ThemeConfig *
build_theme_config( const char *theme_dir )
{
	ThemeConfig *config = NULL ;
	struct direntry **list;
	int           i, n;

	n = my_scandir ((char*)theme_dir, &list, ignore_dots, NULL);
	if( n > 0 )
	{
		FunctionData *install_func  ;
		config = safecalloc( 1, sizeof(ThemeConfig) );
		config->install = new_complex_func( NULL, THEME_INSTALL_FUNC_NAME );
		config->apply = new_complex_func( NULL, THEME_APPLY_FUNC_NAME );

		config->install->items_num = n ;
		config->install->items = install_func = safecalloc( n, sizeof(FunctionData)) ;

		config->apply->items_num = 3 ;
		config->apply->items = safecalloc( 3, sizeof(FunctionData)) ;

		for (i = 0; i < n; i++)
		{
			if (!S_ISDIR (list[i]->d_mode))
			{
				char *name = list[i]->d_name ;
				Bool added_new = True ;
				LOCAL_DEBUG_OUT( "checking file \"%s\"", name );
				if( mystrncasecmp( name, "look", 4 ) == 0 )
				{
					LOCAL_DEBUG_OUT( "adding Install/Change look command for \"%s\"", name );
					install_func->func = F_INSTALL_LOOK ;
					install_func->name = mystrdup( name );
					install_func->text = make_file_name( theme_dir, name );
					if( config->apply->items[0].func == F_NOP )
					{
						config->apply->items[0].func = F_CHANGE_LOOK ;
						config->apply->items[0].name = mystrdup( name );
						config->apply->items[0].text = mystrdup( name );
					}
				}else if( mystrncasecmp( name, "feel", 4 ) == 0 )
				{
					LOCAL_DEBUG_OUT( "adding Install/Change feel command for \"%s\"", name );
					install_func->func = F_INSTALL_FEEL ;
					install_func->name = mystrdup( name );
					install_func->text = make_file_name( theme_dir, name );
					if( config->apply->items[1].func == F_NOP )
					{
						config->apply->items[1].func = F_CHANGE_FEEL ;
						config->apply->items[1].name = mystrdup( name );
						config->apply->items[1].text = mystrdup( name );
					}
				}else if( mystrncasecmp( name, "background", 10 ) == 0 )
				{
					LOCAL_DEBUG_OUT( "adding Install/Change background command for \"%s\"", name );
					install_func->func = F_INSTALL_BACKGROUND ;
					install_func->name = mystrdup( name );
					install_func->text = make_file_name( theme_dir, name );
					if( config->apply->items[2].func == F_NOP )
					{
						config->apply->items[2].func = F_CHANGE_BACKGROUND ;
						config->apply->items[2].name = mystrdup( name );
						config->apply->items[2].text = mystrdup( name );
					}
				}else
				{
					int len = strlen( name );
					added_new = False ;
					if( len > 4 )
						if( mystrncasecmp( &(name[len-4]), ".xpm", 4 ) == 0 )
						{
							LOCAL_DEBUG_OUT( "adding Install icon command for \"%s\"", name );
							install_func->func = F_INSTALL_ICON ;
							install_func->name = mystrdup( name );
							install_func->text = make_file_name( theme_dir, name );
							added_new = True ;
						}
				}
				if( added_new )
					++install_func ;
			}
			free (list[i]);
		}
		if (n > 0)
			free (list);
	}
	return config ;
}

void
fix_install_theme_script( ComplexFunction *install_func, const char *theme_dir )
{
	int i ;
    for( i = 0 ; i < install_func->items_num ; i++ )
    {
		FunctionData *fdata = &(install_func->items[i]);
		int   func = fdata->func ;
		char *fixed_fname = NULL ;
		if( fdata->text != NULL )
		{
			switch( func )
			{
				case F_INSTALL_LOOK :
				case F_INSTALL_FEEL :
				case F_INSTALL_BACKGROUND :
				case F_INSTALL_FONT :
				case F_INSTALL_ICON :
				case F_INSTALL_TILE :
				case F_INSTALL_THEME_FILE :
					fixed_fname = make_file_name( theme_dir, fdata->text );
			    	break ;
			}
			if( fixed_fname != NULL )
			{
				free( fdata->text );
				fdata->text = fixed_fname ;
			}
		}
    }
}

void
fix_apply_theme_script( ComplexFunction *install_func )
{
	int i ;
    for( i = 0 ; i < install_func->items_num ; i++ )
    {
		FunctionData *fdata = &(install_func->items[i]);
		int   func = fdata->func ;
		char *fixed_fname = NULL ;
		if( fdata->text != NULL )
		{
			switch( func )
			{
				case F_CHANGE_LOOK :
					if( (fixed_fname = make_session_data_file(Session, False, S_IFREG, as_look_dir_name, fdata->text, NULL )) == NULL )
						fixed_fname = make_session_data_file(Session, True, S_IFREG, as_look_dir_name, fdata->text, NULL );
			    	break ;
				case F_CHANGE_FEEL :
					if( (fixed_fname = make_session_data_file(Session, False, S_IFREG, as_feel_dir_name, fdata->text, NULL )) == NULL )
						fixed_fname = make_session_data_file(Session, True, S_IFREG, as_feel_dir_name, fdata->text, NULL );
			    	break ;
				case F_CHANGE_BACKGROUND :
					if( (fixed_fname = make_session_data_file(Session, False, S_IFREG, as_background_dir_name, fdata->text, NULL )) == NULL )
						fixed_fname = make_session_data_file(Session, True, S_IFREG, as_background_dir_name, fdata->text, NULL );
			    	break ;
				case F_CHANGE_THEME_FILE :
					fixed_fname = make_session_data_file(Session, False, S_IFREG, as_theme_file_dir_name, fdata->text, NULL );
			    	break ;
			}
			if( fixed_fname != NULL )
			{
				free( fdata->text );
				fdata->text = fixed_fname ;
			}
		}
    }
}

void
cleanup_dir( const char *dirname )
{
	struct direntry **list;
	int           i, n;

	n = my_scandir ((char*)dirname, &list, ignore_dots, NULL);
	LOCAL_DEBUG_OUT( "found %d entries in \"%s\"", n, dirname );
	for (i = 0; i < n; i++)
	{
		char *tmp = make_file_name( dirname, list[i]->d_name );
		if (S_ISDIR (list[i]->d_mode))
		{
			cleanup_dir( tmp );
			rmdir( tmp );
		}else
			unlink( tmp );
		free( tmp );
		free (list[i]);
	}
	if (n > 0)
		free (list);
}

char *
get_theme_subdir( const char *dirname )
{
	char *subdir = NULL ;
	struct direntry **list;
	int           i, n;

	n = my_scandir ((char*)dirname, &list, ignore_dots, NULL);
	LOCAL_DEBUG_OUT( "found %d entries in \"%s\"", n, dirname );
	for (i = 0; i < n; i++)
	{
		if (S_ISDIR (list[i]->d_mode) && subdir == NULL )
		{
			LOCAL_DEBUG_OUT( "%d entry is a subdir \"%s\"", i, list[i]->d_name );
			subdir = make_file_name( dirname, list[i]->d_name );
		}
		free (list[i]);
	}
	if (n > 0)
		free (list);
	return subdir;
}



Bool
install_theme_file( const char *src )
{
	ASThemeFileType type = AST_ThemeBad ;
	char *theme_dir = NULL ;
	char *theme_script_fname = NULL ;
	Bool success = False ;

	type = detect_theme_file_type( src );
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
        sprintf (theme_dir, "%s/astheme.%ld", tmpdir, getuid() );
		if( mkdir( theme_dir, 0700 ) == -1 )
		{
			if( errno != EEXIST  )
			{
				show_error( "unable to create staging directory for theme files \"%s\"", theme_dir );
				free( theme_dir );
				return False;
			}else
			{
				/* need to cleanup the dir */
 				cleanup_dir( theme_dir );
			}
		}
		/* need to untar all the files into dir */
		if( untar_file( src, theme_dir, type ) )
		{
			char * theme_subdir ;
			type = AST_ThemeScript ;
			/* the actuall theme files will be in subdirectory named after the theme : */
			theme_subdir = get_theme_subdir( theme_dir );
			if( theme_subdir != NULL )
			{
				free( theme_dir );
				theme_dir = theme_subdir ;
				themedir_len = strlen( theme_dir );
				LOCAL_DEBUG_OUT( "using theme subdir \"%s\"", theme_dir );
			}
			/* lets hope that theme has its script file supplied : */
			theme_script_fname = safemalloc( themedir_len + 1 + 14 + 1 );
			sprintf( theme_script_fname, "%s/install_script", theme_dir );
		}else
			type = AST_ThemeBad ;
	}else
	{
		char *tmp = strrchr( src, '/' );
		if( tmp != NULL )
			theme_dir = mystrndup( src, tmp - src );
		else
			theme_dir = mystrdup( "" );
		theme_script_fname = mystrdup( src );
	}

	if( type == AST_ThemeScript )
	{
		/* step 1: parsing theme script file */
		ThemeConfig *theme = NULL ;

		if( theme_script_fname && CheckFile( theme_script_fname ) == 0 )
			theme = ParseThemeFile ( theme_script_fname, "afterstep");

		if( theme == NULL )
		{
			theme = build_theme_config( theme_dir );
		}else if( theme_dir[0] != '\0' && theme->install )
			fix_install_theme_script( theme->install, theme_dir );  /* appends theme_dir to filenames */

		if( theme == NULL )
			show_error( "Theme contains no recognizable files. Installation failed" );
		else
		{
			if( theme->install )
				ExecuteBatch( theme->install );

			if( theme->apply )
			{
				fix_apply_theme_script( theme->apply );  /* appends theme_dir to filenames */
				ExecuteBatch( theme->apply );
			}

			DestroyThemeConfig(theme);
			success = True ;
		}
	}
	if( theme_dir )
		free( theme_dir );
	if( theme_script_fname )
		free( theme_script_fname );

	return success;
}

