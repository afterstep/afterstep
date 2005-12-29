/*
 * Copyright (c) 2000 Andrew Ferguson <andrew@owsla.cjb.net>
 * Copyright (c) 1998 Sasha Vasko <sasha at aftercode.net>
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


#define LOCAL_DEBUG
#include "../configure.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/screen.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/session.h"
#include "../libAfterImage/afterimage.h"

#include "afterconf.h"


/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the base.<bpp> config
 * file
 *
 ****************************************************************************/

TermDef       BaseTerms[] = {
    {TF_NO_MYNAME_PREPENDING, "ModulePath", 10,     TT_PATHNAME, BASE_MODULE_PATH_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "AudioPath", 9,       TT_PATHNAME, BASE_AUDIO_PATH_ID , NULL},
    {TF_NO_MYNAME_PREPENDING, "IconPath", 8,        TT_PATHNAME, BASE_ICON_PATH_ID  , NULL},
    {TF_NO_MYNAME_PREPENDING, "PixmapPath", 10,     TT_PATHNAME, BASE_PIXMAP_PATH_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "FontPath", 8,        TT_PATHNAME, BASE_FONT_PATH_ID,   NULL},
    {TF_NO_MYNAME_PREPENDING, "CursorPath", 10,     TT_PATHNAME, BASE_CURSOR_PATH_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "Path", 4,            TT_PATHNAME, BASE_MYNAME_PATH_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "gtkrcPath", 9,       TT_PATHNAME, BASE_GTKRC_PATH_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "gtkrc20Path", 11,    TT_PATHNAME, BASE_GTKRC20_PATH_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "DeskTopSize", 11,    TT_GEOMETRY, BASE_DESKTOP_SIZE_ID   , NULL},
    {TF_NO_MYNAME_PREPENDING, "DeskTopScale", 12,   TT_INTEGER,  BASE_DESKTOP_SCALE_ID  , NULL},
    {TF_NO_MYNAME_PREPENDING|TF_INDEXED, "TermCommand", 11,    TT_TEXT,     BASE_TermCommand_ID  , NULL},
    {TF_NO_MYNAME_PREPENDING|TF_INDEXED, "BrowserCommand", 14,    TT_TEXT,     BASE_BrowserCommand_ID  , NULL},
    {TF_NO_MYNAME_PREPENDING|TF_INDEXED, "EditorCommand", 13,    TT_TEXT,     BASE_EditorCommand_ID  , NULL},
    {TF_NO_MYNAME_PREPENDING, "DisableSharedMemory", 19,   TT_FLAG,  BASE_NoSharedMemory_ID  , NULL},
    {TF_NO_MYNAME_PREPENDING, "DisableKDEGlobalsTheming", 24,   TT_FLAG,  BASE_NoKDEGlobalsTheming_ID  , NULL},
	{0, NULL, 0, 0, 0}
};

SyntaxDef     BaseSyntax = {
	'\n',
	'\0',
	BaseTerms,
	0,										   /* use default hash size */
	' ',
	"",
	"\t",
	"AfterStep Base configuration",
	"Base",
	"essential AfterStep configuration options",
	NULL,
	0
};

flag_options_xref BaseFlags[] = {
	{BASE_NO_SHARED_MEMORY, BASE_NoSharedMemory_ID, 0},
	{BASE_NO_KDEGLOBALS_THEMING, BASE_NoKDEGlobalsTheming_ID, 0},
    {0, 0, 0}
};


BaseConfig   *
CreateBaseConfig ()
{
	BaseConfig   *config = safecalloc (1, sizeof (BaseConfig));

	/* let's initialize Base config with some nice values: */
	config->desktop_size.flags = WidthValue | HeightValue;
	config->desktop_size.width = config->desktop_size.height = 1;
	config->desktop_size.x = config->desktop_size.y = 0;
	config->desktop_scale = 32;

	return config;
}

void
DestroyBaseConfig (BaseConfig * config)
{
	int i = MAX_TOOL_COMMANDS;
	if (config->module_path)
		free (config->module_path);
	if (config->audio_path)
		free (config->audio_path);
	if (config->icon_path)
		free (config->icon_path);
	if (config->pixmap_path)
		free (config->pixmap_path);
	if (config->font_path)
		free (config->font_path);
	if (config->cursor_path)
		free (config->cursor_path);
	if (config->myname_path)
		free (config->myname_path);
	while( --i >= 0 ) 
	{	
		if( config->term_command[i] )
			free( config->term_command[i] );
		if( config->browser_command[i] )
			free( config->browser_command[i] );
		if( config->editor_command[i] )
			free( config->editor_command[i] );
	}
	
	DestroyFreeStorage (&(config->more_stuff));
	free (config);
}

BaseConfig   *
ParseBaseOptions (const char *filename, char *myname)
{
	BaseConfig   *config = CreateBaseConfig ();
	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem    item;

	Storage = file2free_storage(filename, myname, &BaseSyntax, NULL, &(config->more_stuff) );
	if (Storage == NULL)
		return config;

	item.memory = NULL;

	for (pCurr = Storage; pCurr; pCurr = pCurr->next)
	{
		if (pCurr->term == NULL)
			continue;
		if (ReadFlagItem (&(config->set_flags), &(config->flags), pCurr, BaseFlags))
            continue;
		if (!ReadConfigItem (&item, pCurr))
			continue;
		switch (pCurr->term->id)
		{
		 case BASE_MODULE_PATH_ID:
			 set_string( &(config->module_path), item.data.string );
			 break;
		 case BASE_AUDIO_PATH_ID:
			 set_string( &(config->audio_path), item.data.string );
			 break;
		 case BASE_ICON_PATH_ID:
			 set_string( &(config->icon_path), item.data.string );
			 break;
		 case BASE_PIXMAP_PATH_ID:
			 set_string( &(config->pixmap_path), item.data.string );
			 break;
		 case BASE_FONT_PATH_ID:
			 set_string( &(config->font_path), item.data.string );
			 break;
		 case BASE_CURSOR_PATH_ID:
			 set_string( &(config->cursor_path), item.data.string );
			 break;
		 case BASE_MYNAME_PATH_ID:
			 set_string( &(config->myname_path), item.data.string );
			 break;
		 case BASE_DESKTOP_SIZE_ID:
		 	 set_flags( config->set_flags, BASE_DESKTOP_SIZE_SET );
			 config->desktop_size = item.data.geometry;
			 /* errorneous value check */
			 if (!(config->desktop_size.flags & WidthValue))
				 config->desktop_size.width = 1;
			 if (!(config->desktop_size.flags & HeightValue))
				 config->desktop_size.height = 1;
			 config->desktop_size.flags = WidthValue | HeightValue;
			 break;
		 case BASE_DESKTOP_SCALE_ID:
		 	 set_flags( config->set_flags, BASE_DESKTOP_SCALE_SET );
			 config->desktop_scale = item.data.integer;
			 /* errorneous value check */
			 if (config->desktop_scale < 1)
				 config->desktop_scale = 1;
			 break;
		 case BASE_TermCommand_ID :
		 	 if( item.index  < MAX_TOOL_COMMANDS || item.index >= 0 ) 
			 {	
			 	set_string(&(config->term_command[item.index]), item.data.string );		 	
			 	break;
			 }
    	 case BASE_BrowserCommand_ID :
		 	 if( item.index  < MAX_TOOL_COMMANDS || item.index >= 0 ) 
			 {	
			 	set_string(&(config->browser_command[item.index]), item.data.string );		 	
			 	break;
			 }
	   	 case BASE_EditorCommand_ID :	
		 	 if( item.index  < MAX_TOOL_COMMANDS || item.index >= 0 ) 
			 {	
			 	set_string(&(config->editor_command[item.index]), item.data.string );		 	
			 	break;
			 }
		 default:
			 item.ok_to_free = 1;
		}
	}
	ReadConfigItem (&item, NULL);

	DestroyFreeStorage (&Storage);
	return config;
}

/* returns:
 *		0 on success
 *		1 if data is empty
 *		2 if ConfigWriter cannot be initialized
 *
 */
int
WriteBaseOptions (const char *filename, char *myname, BaseConfig * config, unsigned long flags)
{
	ConfigDef    *BaseConfigWriter = NULL;
	FreeStorageElem *Storage = NULL, **tail = &Storage;
	ConfigData cd ;

	if (config == NULL)
		return 1;
	cd.filename = filename ;
	if ((BaseConfigWriter = InitConfigWriter (myname, &BaseSyntax, CDT_Filename, cd)) == NULL)
		return 2;

	CopyFreeStorage (&Storage, config->more_stuff);

	/* building free storage here */

	/* module_path */
	tail = String2FreeStorage (&BaseSyntax, tail, config->module_path, BASE_MODULE_PATH_ID);

	/* icon_path */
	tail = String2FreeStorage (&BaseSyntax, tail, config->icon_path, BASE_ICON_PATH_ID);

	/* pixmap_path */
	tail = String2FreeStorage (&BaseSyntax, tail, config->pixmap_path, BASE_PIXMAP_PATH_ID);

	/* cursor_path */
	tail = String2FreeStorage (&BaseSyntax, tail, config->cursor_path, BASE_CURSOR_PATH_ID);

	/* audio_path */
	tail = String2FreeStorage (&BaseSyntax, tail, config->audio_path, BASE_AUDIO_PATH_ID);

	/* myname_path */
	tail = String2FreeStorage (&BaseSyntax, tail, config->myname_path, BASE_MYNAME_PATH_ID);

	/* desktop_size */
	tail = Geometry2FreeStorage (&BaseSyntax, tail, &(config->desktop_size), BASE_DESKTOP_SIZE_ID);

	/* desktop_scale */
    tail = Integer2FreeStorage (&BaseSyntax, tail, NULL, config->desktop_scale, BASE_DESKTOP_SCALE_ID);

	cd.filename = filename ;
	/* writing config into the file */
	WriteConfig (BaseConfigWriter, &Storage, CDT_Filename, &cd, flags);
	DestroyConfig (BaseConfigWriter);

	if (Storage)
	{
		fprintf (stderr, "\n%s:Config Writing warning: Not all Free Storage discarded! Trying again...", myname);
		DestroyFreeStorage (&Storage);
		fprintf (stderr, (Storage != NULL) ? " failed." : " success.");
	}
	return 0;
}

/*************************************************************************
 * Supplementary functionality to handle Base configuration changes.
 *************************************************************************/
void
ExtractPath (BaseConfig * config,
			 char **module_path,
			 char **audio_path,
			 char **icon_path,
			 char **pixmap_path,
			 char **font_path,
			 char **cursor_path,
			 char **myname_path,
			 char **gtkrc_path,
			 char **gtkrc20_path)
{
	register char *tmp ;
	if (config)
	{
		if (module_path)
		{
			tmp = copy_replace_envvar (config->module_path);
			set_string(module_path, tmp );
		}
		if (audio_path)
		{
			tmp = copy_replace_envvar (config->audio_path);
			set_string(audio_path, tmp );
		}
		if (icon_path)
		{
			tmp = copy_replace_envvar (config->icon_path);
			set_string(icon_path, tmp );
		}
		if (pixmap_path)
		{
			tmp = copy_replace_envvar (config->pixmap_path);
			set_string(pixmap_path, tmp );
		}
		if (font_path)
		{
			tmp = copy_replace_envvar (config->font_path);
			set_string(font_path, tmp );
		}
		if (cursor_path)
		{
			tmp = copy_replace_envvar (config->cursor_path);
			set_string(cursor_path, tmp );
		}
		if (myname_path)
		{
			tmp = copy_replace_envvar (config->myname_path);
			set_string(myname_path, tmp );
		}
		if (gtkrc_path)
		{
			tmp = make_session_rc_file (Session, config->gtkrc_path);
			set_string(gtkrc_path, tmp );
		}
		if (gtkrc20_path)
		{
			tmp = make_session_rc_file (Session, config->gtkrc20_path);
			set_string(gtkrc20_path, tmp );
		}
	}
}

void
BaseConfig2ASEnvironment( register BaseConfig *config, ASEnvironment **penv )
{
	register ASEnvironment *env = *penv;
	if( env == NULL )
		env = safecalloc( 1, sizeof( ASEnvironment ) );
	ExtractPath (config, &(env->module_path),
		            	&(env->audio_path),
						&(env->icon_path),
						&(env->pixmap_path),
						&(env->font_path),
						&(env->cursor_path),
						NULL,
						&(env->gtkrc_path),
						&(env->gtkrc20_path));
	if (config->desktop_size.flags & WidthValue)
		env->desk_pages_h = config->desktop_size.width ;
	else
		env->desk_pages_h = 0 ;

	if (config->desktop_size.flags & HeightValue)
		env->desk_pages_v = config->desktop_size.height ;
	else
		env->desk_pages_v = 0 ;
	env->desk_scale = config->desktop_scale ;
	
	set_environment_tool_from_list( env, ASTool_Term, config->term_command, MAX_TOOL_COMMANDS );
	set_environment_tool_from_list( env, ASTool_Browser, config->browser_command, MAX_TOOL_COMMANDS );
	set_environment_tool_from_list( env, ASTool_Editor, config->editor_command, MAX_TOOL_COMMANDS );

	if( get_flags(config->set_flags, BASE_NO_SHARED_MEMORY ) )
	{	
		if( get_flags(config->flags, BASE_NO_SHARED_MEMORY ) )
			set_flags( env->flags, ASE_NoSharedMemory );
		else
			clear_flags( env->flags, ASE_NoSharedMemory );
	}
	if( get_flags(config->set_flags, BASE_NO_KDEGLOBALS_THEMING ) )
	{	
		if( get_flags(config->flags, BASE_NO_KDEGLOBALS_THEMING ) )
			set_flags( env->flags, ASE_NoKDEGlobalsTheming );
		else
			clear_flags( env->flags, ASE_NoKDEGlobalsTheming );
	}
	*penv = env ;
}

Bool
ReloadASEnvironment( ASImageManager **old_imageman, 
					 ASFontManager **old_fontman, 
					 BaseConfig **config_return, 
					 Bool flush_images, Bool support_shared_images )
{
	char *old_pixmap_path = NULL ;
	char *old_font_path = NULL ;
    char *configfile = NULL ;
	BaseConfig *config = NULL ;
	ASEnvironment *e = NULL ;
	ScreenInfo *scr = get_current_screen();

	if( Environment != NULL )
	{
		old_pixmap_path = Environment->pixmap_path ;
		Environment->pixmap_path = NULL ;
		old_font_path   = Environment->font_path ;
		Environment->font_path   = NULL ;
	}

	configfile = Session->overriding_file ;
	if( configfile == NULL )
		configfile = make_session_file(Session, BASE_FILE, False/* no longer use #bpp in filenames */ );
	if( configfile != NULL )
	{
		config = ParseBaseOptions (configfile, MyName);
		if( config != NULL )
			show_progress("BASE configuration loaded from \"%s\" ...", configfile);
		else
			show_progress("BASE could not be loaded from \"%s\" ...", configfile);
		if( configfile != Session->overriding_file )
			free( configfile );
	}else
        show_warning("BASE configuration file cannot be found");

	if( config == NULL )
	{
		if( Environment != NULL )
		{
			Environment->pixmap_path = old_pixmap_path ;
			Environment->font_path 	 = old_font_path ;
			return  False;
		}
		/* otherwise we should use default values  - Environment should never be NULL */
		Environment = make_default_environment();
	}else
	{
		BaseConfig2ASEnvironment( config, &Environment );
		if( config_return )
			*config_return = config ;
		else
			DestroyBaseConfig (config);
	}

	e = Environment ;
	/* Save base filename to pass to modules */
    if( mystrcmp(old_pixmap_path, e->pixmap_path) == 0 ||
		(e->pixmap_path != NULL && scr->image_manager == NULL) ||
		flush_images )
    {
		reload_screen_image_manager( scr, old_imageman );
	}
	if( old_pixmap_path && old_pixmap_path != e->pixmap_path )
    	free( old_pixmap_path );

	if( mystrcmp(old_font_path, e->font_path) == 0 ||
		(e->font_path != NULL && scr->font_manager == NULL) )
    {
		if( old_fontman )
		{
			*old_fontman = scr->font_manager ;
		}else if( scr->font_manager )
			destroy_font_manager( scr->font_manager, False );

        scr->font_manager = create_font_manager( dpy, e->font_path, NULL );
		set_xml_font_manager( scr->font_manager );
	    show_progress("Font Path changed to \"%s\" ...", e->font_path?e->font_path:"");
    }
    if( old_font_path && old_font_path != e->font_path )
        free( old_font_path );

	if( e->desk_pages_h > 0 )
		scr->VxMax = (e->desk_pages_h-1)*scr->MyDisplayWidth ;
	else
		scr->VxMax = 0 ;
	if( e->desk_pages_v > 0 )
		scr->VyMax = (e->desk_pages_v-1)*scr->MyDisplayHeight ;
	else
		scr->VyMax = 0 ;
	scr->VScale = e->desk_scale;
	if( scr->VScale <= 1 ) 
		scr->VScale = 2 ;
	else if( scr->VScale >= scr->MyDisplayHeight/2 ) 
		scr->VScale = scr->MyDisplayHeight/2 ;

#ifdef XSHMIMAGE
	if( support_shared_images ) 
	{
		if(get_flags( e->flags, ASE_NoSharedMemory ) )
			disable_shmem_images ();
		else
			enable_shmem_images ();
	}
SHOW_CHECKPOINT;
#endif

	
	return (config!=NULL);
}

