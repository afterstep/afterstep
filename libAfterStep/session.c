/*
 * Copyright (c) 2000 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

/*#define LOCAL_DEBUG */

#include "../configure.h"

#include <unistd.h>
#include <stdarg.h>

#include "asapp.h"
#include "afterstep.h"
#include "screen.h"
#include "functions.h"
#include "session.h"

static inline ASDeskSession *
create_desk_session ()
{
	ASDeskSession *session = (ASDeskSession *) safecalloc (1, sizeof (ASDeskSession));
	return session;
}

static void
destroy_desk_session (ASDeskSession * session)
{
	if (session)
	{
		if (session->look_file)
			free (session->look_file);
#ifdef MYLOOK_HEADER_FILE_INCLUDED
		if (session->look)
			mylook_destroy (&(session->look));
#endif
        if (session->feel_file)
			free (session->feel_file);
#ifdef ASFEEL_HEADER_FILE_INCLUDED
        if (session->feel)
			destroy_asfeel(session->feel, False);
#endif
        if (session->background_file)
			free (session->background_file);
        if (session->theme_file)
            free (session->theme_file);
        free (session);
	}
}

static char  *
find_default_file (ASSession *session, const char *filename, Bool check_shared)
{
    char         *fullfilename;

    fullfilename = make_file_name (session->ashome, filename);
	if (CheckFile (fullfilename) == 0)
		return fullfilename;
	free (fullfilename);

	if (check_shared)
	{
        fullfilename = make_file_name (session->asshare, filename);
		if (CheckFile (fullfilename) == 0)
			return fullfilename;
		free (fullfilename);
	}
	return NULL;
}

static char  *
find_default_background_file (ASSession *session)
{
    char         *legacy ;
	char         *file = NULL;

	/* it's more difficult with backgrounds since there could be different extensions */
	/* first checking only private dir : */
    legacy = safemalloc( strlen(BACK_FILE)+1+15 );
    if( session->scr->screen != 0 )
    {
        sprintf( legacy, BACK_FILE ".scr%ld", 0, session->scr->screen);
        file = find_default_file ( session, legacy, False);  /* legacy stuff */
    }
    if( file == NULL )
    {
        sprintf( legacy, BACK_FILE, 0 );
        file = find_default_file ( session, legacy, True);  /* legacy stuff */
    }
    free( legacy );
    if( file == NULL )
        file = find_default_file ( session, "backgrounds/DEFAULT", False);
	if (file == NULL)
        file = find_default_file ( session, "backgrounds/DEFAULT.xpm", False);
	if (file == NULL)
        file = find_default_file ( session, "backgrounds/DEFAULT.jpg", False);
	if (file == NULL)
        file = find_default_file ( session, "backgrounds/DEFAULT.png", False);

	if (file == NULL)
	{										   /* now checking shared dir as well : */
        file = find_default_file ( session, "backgrounds/DEFAULT", True);
		if (file == NULL)
            file = find_default_file ( session, "backgrounds/DEFAULT.xpm", True);
		if (file == NULL)
            file = find_default_file ( session, "backgrounds/DEFAULT.jpg", True);
		if (file == NULL)
            file = find_default_file ( session, "backgrounds/DEFAULT.png", True);
	}
	return file;
}

static char  *
find_default_look_file (ASSession *session)
{
    char         *legacy ;
	char         *file = NULL;

	/* it's more difficult with backgrounds since there could be different extensions */
	/* first checking only private dir : */
    legacy = safemalloc( strlen(LOOK_FILE)+11+1+15 );
    if( session->scr->screen != 0 )
    {
        sprintf( legacy, LOOK_FILE ".scr%ld", 0, session->colordepth, session->scr->screen);
        file = find_default_file ( session, legacy, False);  /* legacy stuff */
    }
    if( file == NULL )
    {
        sprintf( legacy, LOOK_FILE, 0, session->colordepth );
        file = find_default_file ( session, legacy, True);  /* legacy stuff */
    }
    free( legacy );
    if( file == NULL )
        file = find_default_file ( session, "looks/look.DEFAULT", True);
    return file;
}

static char  *
find_default_feel_file (ASSession *session)
{
    char         *legacy ;
	char         *file = NULL;

	/* it's more difficult with backgrounds since there could be different extensions */
	/* first checking only private dir : */
    legacy = safemalloc( strlen(FEEL_FILE)+11+1+15 );
    if( session->scr->screen != 0 )
    {
        sprintf( legacy, FEEL_FILE ".scr%ld", 0, session->colordepth, session->scr->screen);
        file = find_default_file ( session, legacy, False);  /* legacy stuff */
    }
    if( file == NULL )
    {
        sprintf( legacy, FEEL_FILE, 0, session->colordepth );
        file = find_default_file ( session, legacy, True);  /* legacy stuff */
    }
    free( legacy );
    if( file == NULL )
        file = find_default_file ( session, "feels/feel.DEFAULT", True);
    return file;
}


static char  *
find_default_theme_file (ASSession *session)
{
    char         *legacy ;
	char         *file = NULL;

	/* it's more difficult with backgrounds since there could be different extensions */
	/* first checking only private dir : */
    legacy = safemalloc( strlen(THEME_FILE)+11+1+15 );
    if( session->scr->screen != 0 )
    {
        sprintf( legacy, THEME_FILE ".scr%ld", 0, session->colordepth, session->scr->screen);
        file = find_default_file ( session, legacy, False);  /* legacy stuff */
    }
    if( file == NULL )
    {
        sprintf( legacy, THEME_FILE, 0, session->colordepth );
        file = find_default_file ( session, legacy, True);  /* legacy stuff */
    }
    free( legacy );
    return file;
}

static char  *
check_file (const char *file)
{
	char         *full_file;

	full_file = mystrdup (file);

	if (CheckFile (full_file) == 0)
		return full_file;
	replace_envvar (&full_file);
    if (CheckFile (full_file) == 0)
		return full_file;
	free (full_file);
	return NULL;
}

static ASDeskSession *
get_desk_session (ASSession * session, int desk)
{
	register ASDeskSession *d = NULL;

	if (session)
	{
		register int i = session->desks_used, k ;

		if( !IsValidDesk(desk) )
			return session->defaults;

		while (--i >= 0 )
        {
			if (session->desks[i]->desk == desk)
                return session->desks[i];
			else if( session->desks[i]->desk < desk )
				break;
        }
		d = create_desk_session ();
		d->desk = desk;
		if( session->desks_used >= session->desks_allocated )
		{
			session->desks_allocated = (((session->desks_used*sizeof(ASDeskSession)+33)/32)*32)/sizeof(ASDeskSession);
			session->desks = realloc( session->desks, sizeof(ASDeskSession)*session->desks_allocated );
		}
		/* sorting them desks in ascending order : */
		++i ;
		++session->desks_used ;
		for( k = session->desks_used-1 ; k > i ; --k )
			session->desks[k] = session->desks[k-1] ;
		session->desks[i] = d ;
	}
	return d;
}


/*********************************************************************/
ASSession *
create_assession ( ScreenInfo *scr, char *ashome, char *asshare)
{
    ASSession *session = (ASSession *) safecalloc (1, sizeof (ASSession));


	session->scr = ( scr == NULL )?&Scr:scr ;     /* sensible default */

    session->colordepth = session->scr->d_depth ;
    session->ashome = ashome ;
    session->asshare = asshare ;

	session->defaults = create_desk_session ();
	session->defaults->desk = INVALID_DESK ;
    session->defaults->look_file = find_default_look_file (session);
    session->defaults->feel_file = find_default_feel_file (session);
    session->defaults->background_file = find_default_background_file (session);
    session->defaults->theme_file = find_default_theme_file (session);

	session->desks_used = 0 ;
	session->desks_allocated = 4 ;
	session->desks = safecalloc(session->desks_allocated, sizeof( ASDeskSession*));
	session->changed = False;
	return session;
}

void
update_default_session ( ASSession *session, int func)
{
	switch( func )
	{
		case F_CHANGE_LOOK :
			if( session->defaults->look_file )
				free( session->defaults->look_file );
		    session->defaults->look_file = find_default_look_file (session);
			break;
		case F_CHANGE_FEEL :
			if( session->defaults->feel_file )
				free( session->defaults->feel_file );
			session->defaults->feel_file = find_default_feel_file (session);
			break;
		case F_CHANGE_BACKGROUND :
			if( session->defaults->background_file )
				free( session->defaults->background_file );
			session->defaults->background_file = find_default_background_file (session);
			break;
		case F_CHANGE_THEME :
			if( session->defaults->theme_file )
				free( session->defaults->theme_file );
			session->defaults->theme_file = find_default_theme_file (session);
			break;
	}
}

void
destroy_assession (ASSession * session)
{
	register int i;
	if( AS_ASSERT(session) )
		return;
	if (session->defaults)
		destroy_desk_session (session->defaults);

    if( session->ashome )
        free( session->ashome );
    if( session->asshare )
        free( session->asshare );
    if( session->overriding_file )
        free( session->overriding_file );
    if( session->overriding_look )
        free( session->overriding_look );
    if( session->overriding_feel )
        free( session->overriding_feel );
    if( session->overriding_theme )
        free( session->overriding_theme );

	i = session->desks_used ;
    while (--i >= 0)
		destroy_desk_session (session->desks[i]);
	free( session->desks );
	free( session );
}


void
change_default_session (ASSession * session, const char *new_val, int function)
{
	if (session)
	{
		register char **target = NULL;

		switch (function)
		{
         case F_CHANGE_BACKGROUND:
			 target = &(session->defaults->background_file);
			 break;
         case F_CHANGE_LOOK:
			 target = &(session->defaults->look_file);
			 break;
		 case F_CHANGE_FEEL:
			 target = &(session->defaults->feel_file);
			 break;
         case F_CHANGE_THEME:
             target = &(session->defaults->theme_file);
			 break;
        }
		if (target)
		{
			char         *good_file;

			if ((good_file = check_file (new_val)) == NULL)
			{
				show_error( "I cannot find configuration file \"%s\". I'll try to use default", new_val );
				return;
			}
			if (*target)
				free (*target);
			*target = good_file;
			session->changed = True;
		}
	}
}

void
change_desk_session (ASSession * session, int desk, const char *new_val, int function)
{
	if (session)
	{
		register ASDeskSession *d = get_desk_session (session, desk);
		register char **target = NULL;

		switch (function)
		{
         case F_CHANGE_BACKGROUND:
			 target = &(d->background_file);
			 break;
#ifdef DIFFERENTLOOKNFEELFOREACHDESKTOP
		 case F_CHANGE_LOOK:
			 target = &(d->look_file);
			 break;
		 case F_CHANGE_FEEL:
			 target = &(d->feel_file);
			 break;
         case F_CHANGE_THEME:
             target = &(d->theme_file);
			 break;
#endif
         default:
			 change_default_session (session, new_val, function);
		}
		if (target)
		{
			char         *good_file;

			if ((good_file = check_file (new_val)) == NULL)
				return;
			if (*target)
				free (*target);
			*target = good_file;
			session->changed = True;
		}
	}
}

/**********************************************************************/
void
change_desk_session_feel (ASSession * session, int desk, struct ASFeel *feel)
{
#ifdef ASFEEL_HEADER_FILE_INCLUDED
    if (session && feel )
	{
		ASDeskSession *d = NULL;
        struct ASFeel *old_feel ;

#ifdef DIFFERENTLOOKNFEELFOREACHDESKTOP
		if( IsValidDesk(desk) && session->overriding_feel == NULL )
		{
			d = get_desk_session (session, desk);
			if( d->feel_file == NULL )
				d = session->defaults ;
		}
#endif
		if( d == NULL )
			d = session->defaults ;

		old_feel = d->feel ;
		d->feel = feel ;
		if( !AS_ASSERT(session->scr) )
		{
		}
		if( session->on_feel_changed_func )
		{
			session->on_feel_changed_func( session->scr, desk, old_feel );
			if( d->desk != desk )
				session->on_feel_changed_func( session->scr, d->desk, old_feel );
		}

		if ( old_feel )
			destroy_asfeel(old_feel, False);
	}
#endif
}
void
change_desk_session_look (ASSession * session, int desk, struct MyLook *look)
{
#ifdef MYLOOK_HEADER_FILE_INCLUDED
    if (session && look )
	{
		ASDeskSession *d = NULL;
		MyLook *old_look ;

#ifdef DIFFERENTLOOKNFEELFOREACHDESKTOP
		if( IsValidDesk(desk) && session->overriding_look == NULL )
		{
			d = get_desk_session (session, desk);
			if( d->feel_file == NULL )
				d = session->defaults ;
		}
#endif
		if( d == NULL )
			d = session->defaults ;

		old_look = d->look ;
		d->look = look ;
LOCAL_DEBUG_OUT( "new look = %p, desk_session = %p (default is %p ), scr is %p ",
				  look, d, session->defaults, session->scr );
		if( !AS_ASSERT(session->scr) )
		{
			ScreenInfo *scr = session->scr ;

			if( d == session->defaults ||
				scr->DefaultLook == old_look )
			{
				MyLook *old_default_look = scr->DefaultLook ;
				scr->DefaultLook = look ;

				if( scr->OnDefaultLookChanged_hook )
					scr->OnDefaultLookChanged_hook( scr, old_default_look );

				/* There is a possibility for contention here, if DefaultLook was
				 * taken from session other then default one */
				if( old_default_look && old_default_look != old_look )
					mylook_destroy(&(old_default_look));
			}
		}
		if( session->on_look_changed_func )
		{
			session->on_look_changed_func( session->scr, desk, old_look );
			if( d->desk != desk )
				session->on_look_changed_func( session->scr, d->desk, old_look );
		}

		if ( old_look )
			mylook_destroy(&(old_look));
	}
#endif
}

/************************************************************************/
static int
MakeASDir (const char *name, mode_t perms)
{
    show_progress ("Creating %s ... ", name);
	if (mkdir (name, perms))
	{
        show_error ("AfterStep depends on %s directory !\nPlease check permissions or contact your sysadmin !",
				 name);
		return (-1);
	}
    show_progress ("\t created.");
	return 0;
}

static int
MakeASFile (const char *name)
{
	FILE         *touch;

    show_progress ("Creating %s ... ", name);
	if ((touch = fopen (name, "w")) == NULL)
	{
        show_error ("Cannot open file %s for writing!\n"
                 " Please check permissions or contact your sysadmin !", name);
		return (-1);
	}
	fclose (touch);
    show_progress("\t created.");
	return 0;
}

static int
CheckOrCreate (const char *what)
{
	mode_t        perms = 0755;
	int           res = 0;

	if (*what == '~' && *(what + 1) == '/')
	{
		char         *checkdir = put_file_home (what);

		if (CheckDir (checkdir) != 0)
			res = MakeASDir (checkdir, perms);
		free (checkdir);
	} else if (CheckDir (what) != 0)
		res = MakeASDir (what, perms);

	return res;
}

static int
CheckOrCreateFile (const char *what)
{
	int           res = 0;

	if (*what == '~' && *(what + 1) == '/')
	{
		char         *checkfile = put_file_home (what);

		if (CheckFile (checkfile) != 0)
			res = MakeASFile (checkfile);
		free (checkfile);
	} else if (CheckFile (what) != 0)
		res = MakeASFile (what);

	return res;
}


void
check_AfterStep_dirtree ( char * ashome, Bool create_non_conf )
{
	/* Create missing directories & put there defaults */
	if (CheckDir (ashome) != 0)
	{
        char         *fullfilename;
        CheckOrCreate (as_gnustep_dir_name);
		CheckOrCreate (as_gnusteplib_dir_name);
		CheckOrCreate (ashome);

        fullfilename = make_file_name (ashome, AFTER_SAVE);
        CheckOrCreateFile (fullfilename);
        free( fullfilename );

        fullfilename = make_file_name (ashome, THEME_DATA_DIR);
        CheckOrCreate(fullfilename);
        free( fullfilename );

        if( create_non_conf )
        {
            fullfilename = make_file_name (ashome, AFTER_NONCF);
            /* legacy non-configurable dir: */
            CheckOrCreate(fullfilename);
            free( fullfilename );
        }
	}
}

static const char *
get_desk_file (ASDeskSession * d, int function)
{
	char         *file = NULL;

	if (d)
		switch (function)
		{
         case F_CHANGE_BACKGROUND:
			 file = d->background_file;
			 break;
         case F_CHANGE_LOOK:
			 file = d->look_file;
			 break;
		 case F_CHANGE_FEEL:
			 file = d->feel_file;
			 break;
         case F_CHANGE_THEME:
             file = d->theme_file;
			 break;
        }
	return file;
}

const char   *
get_session_file (ASSession * session, int desk, int function)
{
	ASDeskSession *d = NULL;
	char         *file = NULL;

	if (session)
	{
        /* backgrounds are not config files, and therefor cannot be overriden :*/
        if( session->overriding_look && function == F_CHANGE_LOOK )
            return session->overriding_look ;
        if( session->overriding_feel && function == F_CHANGE_FEEL )
            return session->overriding_feel ;
        if( session->overriding_theme && function == F_CHANGE_THEME )
            return session->overriding_theme ;
        if( session->overriding_file && function != F_CHANGE_BACKGROUND )
            return session->overriding_file ;

		switch (function)
		{
         case F_CHANGE_BACKGROUND:
			 d = get_desk_session (session, desk);
			 break;
#ifdef DIFFERENTLOOKNFEELFOREACHDESKTOP
		 case F_CHANGE_LOOK:
		 case F_CHANGE_FEEL:
         case F_CHANGE_THEME:
             d = get_desk_session (session, desk);
			 break;
#else
		 case F_CHANGE_LOOK:
		 case F_CHANGE_FEEL:
         case F_CHANGE_THEME:
			 d = session->defaults;
			 break;
#endif
        }
		if (d)
		{
			file = (char *)get_desk_file (d, function);
            LOCAL_DEBUG_OUT( "file for desk %d is \"%s\"", desk, file?"":file );
            if (file != NULL)
				if (CheckFile (file) != 0)
					file = NULL;
            /* fallback to defaults */
            if (file == NULL && d != session->defaults)
            {
                file = (char *)get_desk_file (session->defaults, function);
                LOCAL_DEBUG_OUT( "default file is \"%s\"", file );
                if (file != NULL)
                    if (CheckFile (file) != 0)
                        file = NULL;
            }
        }
    }
	return file;
}

static inline char *
make_session_filedir   (ASSession * session, const char *source, Bool use_depth, int mode )
{
    char *realfilename = NULL;

    if( session->overriding_file )
        return mystrdup(session->overriding_file) ;

    if( source )
    {
        char *filename = (char*)source ;
        if( session->scr->screen != 0 )
        {
            filename = safemalloc( strlen(source) + 1 + 32 + 32 );
            if( use_depth )
                sprintf( filename, "%s.scr%ld.%dbpp", source, session->scr->screen, session->colordepth );
            else
                sprintf( filename, "%s.scr%ld", source, session->scr->screen );

            realfilename = (char *)make_file_name (session->ashome, filename);
            free( filename );
            filename = (char*)source ;
        }
        if( realfilename == NULL || check_file_mode(realfilename, mode) != 0 )
        {
            if( use_depth )
            {
                filename = safemalloc( strlen(source) + 1 + 32 );
                sprintf( filename, "%s.%dbpp", source, session->colordepth );
            }
            if( realfilename )
                free( realfilename );
            realfilename = (char *)make_file_name (session->ashome, filename);
            if (check_file_mode(realfilename, mode) != 0)
            {
                free (realfilename);
                realfilename = make_file_name (session->asshare, filename);
                if (check_file_mode(realfilename, mode) != 0)
                {
                    free (realfilename);
                    realfilename = NULL;
                }
            }
        }
        if( filename != source )
            free( filename );
    }

    return realfilename ;
}

char *
make_session_file   (ASSession * session, const char *source, Bool use_depth )
{
	return make_session_filedir(session,source,use_depth, S_IFREG );
}

char *
make_session_dir   (ASSession * session, const char *source, Bool use_depth )
{
	return make_session_filedir(session,source,use_depth, S_IFDIR );
}

char *
make_session_data_file  (ASSession * session, Bool shared, int if_mode_only, ... )
{
    char         *realfilename = NULL;
    va_list       ap;
    int           len = 0 ;
    register int  i ;
    register char *ptr ;

    if( session == NULL ) return NULL ;

    va_start (ap, if_mode_only);
    while( (ptr = va_arg(ap,char*)) != NULL )
    {
        for( i = 0 ; ptr[i] != '\0' ; i++ ) ;
        len += i+1 ;
    }
    va_end(ap);

    ptr = shared?session->asshare:session->ashome;

    realfilename = safemalloc( strlen(ptr)+1+len );
    for( i = 0 ; ptr[i] != '\0' ; i++ )
        realfilename[i] = ptr[i];

    if( len == 0 )
    {
        realfilename[i] = '\0' ;
        return realfilename;
    }

    if( i > 0 )
        realfilename[i++] = '/' ;

    va_start (ap, if_mode_only);
    while( (ptr = va_arg(ap,char*)) != NULL )
    {
        register int k = 0;
        while( ptr[k] )
            realfilename[i++] = ptr[k++] ;
        realfilename[i++] = '/' ;
    }
    va_end(ap);

    realfilename[--i] = '\0' ;

    if( if_mode_only != 0 )
        if (check_file_mode (realfilename, if_mode_only) != 0)
        {
            free (realfilename);
            realfilename = NULL ;
        }
    return realfilename ;
}

void
set_session_override(ASSession * session, const char *overriding_file, int function )
{
    if( session )
    {
		char **target = &(session->overriding_file);

		if( function == F_CHANGE_LOOK )
			target = &(session->overriding_look);
		else if( function == F_CHANGE_FEEL )
			target = &(session->overriding_feel);
        else if( function == F_CHANGE_THEME )
            target = &(session->overriding_theme);

        if( *target )
        {
            free( *target );
            *target = NULL ;
        }
        if( overriding_file )
            *target = check_file( overriding_file );
    }
}

inline const char *
get_session_override(ASSession * session, int function )
{
	if( session )
	{
		if( session->overriding_file )
			return session->overriding_file;
		else if( function == F_CHANGE_LOOK  )
			return session->overriding_look;
		else if( function == F_CHANGE_FEEL )
			return session->overriding_feel;
        else if( function == F_CHANGE_THEME )
            return session->overriding_theme;
    }
    return NULL;
}

char **
get_session_file_list (ASSession *session, int desk1, int desk2, int function)
{
	char **list = NULL ;
	if( session )
	{
		register int i ;
		if( desk1 > desk2 )
		{
			i = desk2 ;
	  		desk2 = desk1 ;
			desk1 = i ;
		}
		list = safecalloc( (desk2 - desk1)+1, sizeof(char*));
		for( i = desk1 ; i <= desk2 ; i++ )
			list[i-desk1] = (char*)get_session_file( session, i, function );
	}
	return list ;
}

/*************************************************************************************/
ASSession *
GetNCASSession (ScreenInfo *scr, const char *home, const char *share )
{
    ASSession    *session = NULL;
    char         *ashome  = put_file_home (home?home:as_afterstep_dir_name);
    char         *asshare = put_file_home (share?share:as_share_dir_name);

    if( scr == NULL )
		scr = &Scr ;

    check_AfterStep_dirtree (ashome, True);

    session = create_assession (scr, ashome, asshare);

#ifdef DIFFERENTLOOKNFEELFOREACHDESKTOP
    /* TODO : add check of non-cf dir for desktop specific configs : */
#endif
    session->scr = scr ;

    return session;
}


