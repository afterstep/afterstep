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
        if (session->colorscheme_file)
            free (session->colorscheme_file);
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
find_desk_background_file (ASSession *session, int desk)
{
    char         *legacy ;
	char         *file = NULL;
	/* it's more difficult with backgrounds since there could be different extensions */
	/* first checking only private dir : */
    legacy = safemalloc( strlen(BACK_FILE)+15+1+15 );
    if( session->scr->screen != 0 )
    {
        sprintf( legacy, BACK_FILE ".scr%ld", desk, session->scr->screen);
        file = find_default_file ( session, legacy, False);  /* legacy stuff */
    }
    if( file == NULL )
    {
        sprintf( legacy, BACK_FILE, desk );
        file = find_default_file ( session, legacy, True);  /* legacy stuff */
    }
    free( legacy );
	return file;
}


static char  *
find_default_special_file (ASSession *session, const char* file_fmt, const char* file_scr_fmt, const char *default_fname)
{
    char         *legacy ;
	char         *file = NULL;
	int len1 = 0;
	int len2 = 0;

	if( file_scr_fmt )
		len1 = strlen((char*)file_scr_fmt);
	len2 = strlen((char*)file_fmt);
	/* it's more difficult with backgrounds since there could be different extensions */
	/* first checking only private dir : */
    legacy = safemalloc( max(len1,len2)+11+1+15 );
    if( session->scr->screen != 0 && file_scr_fmt != NULL )
    {
        sprintf( legacy, file_scr_fmt, 0, session->scr->screen);
        file = find_default_file ( session, legacy, False);  /* legacy stuff */
    }
    if( file == NULL )
    {
        sprintf( legacy, file_fmt, 0 );
        file = find_default_file ( session, legacy, True);  /* legacy stuff */
    }
    free( legacy );
    if( file == NULL && default_fname != NULL )
        file = find_default_file ( session, default_fname, True);
    return file;
}

static char  *
find_desk_special_file (ASSession *session, const char* file_fmt, const char* file_scr_fmt, int desk)
{
    char         *legacy ;
	char         *file = NULL;
	int len1 = 0;
	int len2 = 0;

	if( file_scr_fmt )
		len1 = strlen((char*)file_scr_fmt);
	len2 = strlen((char*)file_fmt);
	/* it's more difficult with backgrounds since there could be different extensions */
	/* first checking only private dir : */
    legacy = safemalloc( max(len1,len2)+15+11+1+15 );
    if( session->scr->screen != 0 && file_scr_fmt != NULL )
    {
        sprintf( legacy, file_scr_fmt, desk, session->scr->screen);
        file = find_default_file ( session, legacy, False);  /* legacy stuff */
    }
    if( file == NULL )
    {
        sprintf( legacy, file_fmt, desk );
        file = find_default_file ( session, legacy, False);  /* legacy stuff */
    }
    free( legacy );
    return file;
}


#define find_default_look_file(s)  find_default_special_file (s,LOOK_FILE,LOOK_FILE ".scr%ld", LOOK_DIR "/look.DEFAULT")
#define find_default_feel_file(s)  find_default_special_file (s,FEEL_FILE,FEEL_FILE ".scr%ld", FEEL_DIR "/feel.DEFAULT")
#define find_default_theme_file(s) find_default_special_file (s,THEME_FILE,THEME_FILE ".scr%ld",NULL)
#define find_default_colorscheme_file(s) find_default_special_file (s,COLORSCHEME_FILE,COLORSCHEME_FILE ".scr%ld", COLORSCHEME_DIR "/colorscheme.DEFAULT")
#define find_desk_look_file(s,desk)  find_desk_special_file (s,LOOK_FILE,LOOK_FILE ".scr%ld", desk)
#define find_desk_feel_file(s,desk)  find_desk_special_file (s,FEEL_FILE,FEEL_FILE ".scr%ld", desk)
#define find_desk_theme_file(s,desk) find_desk_special_file (s,THEME_FILE,THEME_FILE ".scr%ld",desk)
#define find_desk_colorscheme_file(s,desk) find_desk_special_file (s,COLORSCHEME_FILE,COLORSCHEME_FILE ".scr%ld",desk)

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

		d->look_file = find_desk_look_file (session, desk);
    	d->feel_file = find_desk_feel_file (session, desk);
    	d->background_file = find_desk_background_file (session, desk);
    	d->theme_file = find_desk_theme_file (session, desk);
    	d->colorscheme_file = find_desk_colorscheme_file (session, desk);

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

char *
find_workspace_file( ASSession *session )
{
	char *fname, *full_fname ;

	if( session->scr == NULL || session->scr->screen == 0 )
		return make_file_name( session->ashome, AFTER_SAVE );

	fname = safemalloc( strlen( AFTER_SAVE ) + 4 + 15 + 1 );
	sprintf( fname, AFTER_SAVE ".scr%ld", session->scr->screen );
	full_fname = make_file_name (session->ashome, fname);
	free( fname );
	return full_fname;
}

/*********************************************************************/
ASSession *
create_assession ( ScreenInfo *scr, char *ashome, char *asshare)
{
    ASSession *session = (ASSession *) safecalloc (1, sizeof (ASSession));



	session->scr = ( scr == NULL )?ASDefaultScr:scr ;     /* sensible default */

    session->colordepth = session->scr->d_depth ;
    session->ashome = ashome ;
    session->asshare = asshare ;

	session->defaults = create_desk_session ();
	session->defaults->desk = INVALID_DESK ;
    session->defaults->look_file = find_default_look_file (session);
    session->defaults->feel_file = find_default_feel_file (session);
    session->defaults->background_file = find_default_background_file (session);
    session->defaults->theme_file = find_default_theme_file (session);
    session->defaults->colorscheme_file = find_default_colorscheme_file (session);

	session->workspace_state = find_workspace_file(session);

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
		case F_CHANGE_THEME_FILE :
			if( session->defaults->theme_file )
				free( session->defaults->theme_file );
			session->defaults->theme_file = find_default_theme_file (session);
			break;
		case F_CHANGE_COLORSCHEME :
			if( session->defaults->colorscheme_file )
				free( session->defaults->colorscheme_file );
			session->defaults->colorscheme_file = find_default_colorscheme_file (session);
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
    if( session->overriding_colorscheme )
        free( session->overriding_colorscheme );
	if( session->workspace_state )
		free( session->workspace_state );

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
  		 case F_CHANGE_THEME_FILE :
             target = &(session->defaults->theme_file);
			 break;
         case F_CHANGE_COLORSCHEME:
             target = &(session->defaults->colorscheme_file);
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
		 case F_CHANGE_THEME_FILE :

             target = &(d->theme_file);
			 break;
         case F_CHANGE_COLORSCHEME:
             target = &(d->colorscheme_file);
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

int
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
    char         *fullfilename;
	/* Create missing directories & put there defaults */
	if (CheckDir (ashome) != 0)
	{
		CheckOrCreate (ashome);

#if defined(DO_SEND_POSTCARD) /*&& defined(HAVE_POPEN) */
		/* send some info to sasha @ aftercode.net */
		{
			FILE *p;
			char *filename = make_file_name(ashome, ".postcard");
		    /*p = popen ("mail -s \"AfterStep installation info\" sasha@aftercode.net", "w");*/
			p = fopen( filename, "wt" );
			free(filename);
		    if (p)
			{
				fprintf( p, "AfterStep_Version=\"%s\";\n", VERSION );
				fprintf( p, "CanonicalBuild=\"%s\";\n", CANONICAL_BUILD );
				fprintf( p, "CanonicalOS=\"%s\";\n", CANONICAL_BUILD_OS );
				fprintf( p, "CanonicalCPU=\"%s\";\n", CANONICAL_BUILD_CPU );
				fprintf( p, "CanonicalVendor=\"%s\";\n", CANONICAL_BUILD_VENDOR );
				if( dpy )
				{
					fprintf (p, "X_DefaultScreenNumber=%d;\n", DefaultScreen (dpy));
				    fprintf (p, "X_NumberOfScreens=%d;\n", ScreenCount (dpy));
				    fprintf (p, "X_Display=\"%s\";\n", DisplayString (dpy));
				    fprintf (p, "X_ProtocolVersion=%d.%d;\n", ProtocolVersion (dpy), ProtocolRevision (dpy));
				    fprintf (p, "X_Vendor=\"%s\";\n", ServerVendor (dpy));
				    fprintf (p, "X_VendorRelease=%d;\n", VendorRelease (dpy));
				    if (strstr(ServerVendor (dpy), "XFree86"))
					{
						int vendrel = VendorRelease(dpy);
						fprintf(p, "X_XFree86Version=");
						if (vendrel < 336)
						{
							fprintf(p, "%d.%d.%d", vendrel / 100, (vendrel / 10) % 10, vendrel       % 10);
						} else if (vendrel < 3900)
						{
							fprintf(p, "%d.%d", vendrel / 1000,  (vendrel /  100) % 10);
							if (((vendrel / 10) % 10) || (vendrel % 10))
							{
						  		fprintf(p, ".%d", (vendrel / 10) % 10);
								if (vendrel % 10)
								    fprintf(p, ".%d", vendrel % 10);
							}
						} else if (vendrel < 40000000)
						{
						    fprintf(p, "%d.%d", vendrel/1000,  (vendrel/10) % 10);
						    if (vendrel % 10)
						  		fprintf(p, ".%d", vendrel % 10);
						} else
						{
						    fprintf(p, "%d.%d.%d", vendrel/10000000,(vendrel/100000)%100, (vendrel/1000)%100);
						    if (vendrel % 1000)
								fprintf(p, ".%d", vendrel % 1000);
	  					}
						fprintf(p, ";\n");
					}
					if( ASDefaultScrWidth > 0 )
					{
						fprintf( p, "AS_Screen=%ld;\n", ASDefaultScr->screen );
						fprintf( p, "AS_RootGeometry=%dx%d;\n", ASDefaultScrWidth, ASDefaultScrHeight );
					}
					if( ASDefaultVisual )
					{
						fprintf( p, "AS_Visual=0x%lx;\n", ASDefaultVisual->visual_info.visualid );
						fprintf( p, "AS_Colordepth=%d;\n", ASDefaultVisual->visual_info.depth );
						fprintf( p, "AS_RedMask=0x%lX;\n", ASDefaultVisual->visual_info.red_mask );
						fprintf( p, "AS_GreenMask=0x%lX;\n", ASDefaultVisual->visual_info.green_mask );
						fprintf( p, "AS_BlueMask=0x%lX;\n", ASDefaultVisual->visual_info.blue_mask );
						fprintf( p, "AS_ByteOrdering=%s;\n", (ImageByteOrder(ASDefaultVisual->dpy)==MSBFirst)?"MSBFirst":"LSBFirst" );
					}
			    }
				fclose(p);
				/*pclose (p);*/
		    /*p = popen ("mail -s \"AfterStep installation info\" sasha@aftercode.net", "w");*/
		    }
		}
#endif
	}
    fullfilename = make_file_name (ashome, AFTER_SAVE);
    CheckOrCreateFile (fullfilename);
    free( fullfilename );

#if 0
    fullfilename = make_file_name (ashome, THEME_FILE_DIR);
    CheckOrCreate(fullfilename);
    free( fullfilename );

	fullfilename = make_file_name (ashome, LOOK_DIR);
    CheckOrCreate(fullfilename);
    free( fullfilename );

	fullfilename = make_file_name (ashome, FEEL_DIR);
    CheckOrCreate(fullfilename);
    free( fullfilename );

	fullfilename = make_file_name (ashome, THEME_DIR);
    CheckOrCreate(fullfilename);
    free( fullfilename );

	fullfilename = make_file_name (ashome, COLORSCHEME_DIR);
    CheckOrCreate(fullfilename);
    free( fullfilename );

	fullfilename = make_file_name (ashome, BACK_DIR);
    CheckOrCreate(fullfilename);
    free( fullfilename );
#endif
	fullfilename = make_file_name (ashome, DESKTOP_DIR);
    CheckOrCreate(fullfilename);
    free( fullfilename );

	fullfilename = make_file_name (ashome, ICON_DIR);
    CheckOrCreate(fullfilename);
    free( fullfilename );

	fullfilename = make_file_name (ashome, FONT_DIR);
    CheckOrCreate(fullfilename);
    free( fullfilename );

	fullfilename = make_file_name (ashome, TILE_DIR);
    CheckOrCreate(fullfilename);
    free( fullfilename );
	
	if( create_non_conf )
    {
		char *postcard_fname ;
		FILE *f ;
        fullfilename = make_file_name (ashome, AFTER_NONCF);
        /* legacy non-configurable dir: */
        CheckOrCreate(fullfilename);
		postcard_fname = make_file_name( fullfilename, "send_postcard.sh" );
        free( fullfilename );
		
		f = fopen( postcard_fname, "wt" );
		if( f ) 
		{
			fprintf( f, "#!/bin/sh\n\n" );
			fprintf( f, "xterm -e \"%s/postcard.sh\"\n", AFTER_BIN_DIR );
			fprintf( f, "if [ -r %s/.postcard ] \nthen echo -n \nelse rm %s \nfi\n", ashome, postcard_fname );
			fclose( f );
		}
		chmod (postcard_fname, 0700);
		free(postcard_fname);
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
		 case F_CHANGE_THEME_FILE :

             file = d->theme_file;
			 break;
		 case F_CHANGE_COLORSCHEME:
			 file = d->colorscheme_file;
			 break;
        }
	return file;
}

const char   *
get_session_file (ASSession * session, int desk, int function, Bool no_default)
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
        if( session->overriding_theme && ( function == F_CHANGE_THEME || function == F_CHANGE_THEME_FILE ))
            return session->overriding_theme ;
        if( session->overriding_colorscheme && function == F_CHANGE_COLORSCHEME )
            return session->overriding_colorscheme ;
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
		 case F_CHANGE_THEME_FILE :
		 case F_CHANGE_COLORSCHEME :
             d = get_desk_session (session, desk);
			 break;
#else
		 case F_CHANGE_LOOK:
		 case F_CHANGE_FEEL:
         case F_CHANGE_THEME:
		 case F_CHANGE_THEME_FILE :
		 case F_CHANGE_COLORSCHEME :
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
            if (file == NULL && d != session->defaults && !no_default )
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

const char   *
get_session_ws_file ( ASSession * session, Bool only_if_available )/* workspace_state filename */
{
	if( session == NULL )
		return NULL;
	if( session  && only_if_available )
	 	if (CheckFile(session->workspace_state) != 0)
			return NULL;
	return session->workspace_state ;
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
            filename = safemalloc( strlen((char*)source) + 1 + 32 + 32 );
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
                filename = safemalloc( strlen((char*)source) + 1 + 32 );
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
        else if( function == F_CHANGE_THEME || function == F_CHANGE_THEME_FILE )
            target = &(session->overriding_theme);
        else if( function == F_CHANGE_COLORSCHEME )
            target = &(session->overriding_colorscheme);

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
        else if( function == F_CHANGE_THEME || function == F_CHANGE_THEME_FILE )
            return session->overriding_theme;
        else if( function == F_CHANGE_COLORSCHEME )
            return session->overriding_colorscheme;
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
			list[i-desk1] = (char*)get_session_file( session, i, function, False );
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
		scr = ASDefaultScr ;

    check_AfterStep_dirtree (ashome, True);

    session = create_assession (scr, ashome, asshare);

#ifdef DIFFERENTLOOKNFEELFOREACHDESKTOP
    /* TODO : add check of non-cf dir for desktop specific configs : */
#endif
    session->scr = scr ;

    return session;
}


