/*
 * Copyright (c) 2004 Sasha Vasko <sasha@aftercode.net>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* ASConfig : 
 * 
 * these module manages configuration files : 
 * Files are loaded into memory and stored in FreeStorage format.
 * Modules can connect to our socket and read their config from us directly
 * using different commands.
 * There will be commands to alter config and save it to file - making it a 
 * back-end for future ascp.
 * 
 * Directories : 
 * User Dir : 	$(AFTER_DIR)/
 * 					$(AFTER_NONCF)/   	- non-configureable files - cache of selected files
 * 					$(START_DIR)/     	- user start menu
 * 					$(BACK_DIR)/      	- user backgrounds
 * 					$(LOOK_DIR)/      	- user looks
 * 					$(THEME_DIR)/     	- user themes
 * 					$(COLORSCHEME_DIR)/ - user colorschemes
 * 					$(THEME_FILE_DIR)/	- user themes
 * 					$(FEEL_DIR)/		- user feel
 * 					$(DESKTOP_DIR)/		- user clipart files :
 * 						$(FONT_DIR)/	- user TTF fonts
 * 						$(ICON_DIR)/	- user Icons
 * 						$(TILE_DIR)/	- user's tile images
 * 						$(BARS_DIR)/	- user's bar images ( used for frame sides )
 * 						$(BUTTONS_DIR)/	- user's buttons images ( used for titlebar buttons )
 *
 * Share dDir :  $(AFTER_SHAREDIR)/
 * 					$(AFTER_NONCF)/   	- non-configureable files - default configurations
 * 					$(START_DIR)/     	- shared start menu (used if user's one is not available)
 * 					$(BACK_DIR)/      	- shared backgrounds 
 * 					$(LOOK_DIR)/      	- shared looks
 * 					$(THEME_DIR)/     	- shared themes
 * 					$(COLORSCHEME_DIR)/ - shared colorschemes
 * 					$(THEME_FILE_DIR)/	- shared themes
 * 					$(FEEL_DIR)/		- shared feels
 * 					$(DESKTOP_DIR)/		- shared clipart files :
 * 						$(FONT_DIR)/	- shared TTF fonts
 * 						$(ICON_DIR)/	- shared Icons
 * 						$(TILE_DIR)/	- shared tile images
 * 						$(BARS_DIR)/	- shared bar images ( used for frame sides )
 * 						$(BUTTONS_DIR)/	- shared buttons images ( used for titlebar buttons )
 * 
 * Configuration Files : 
 *  Base : 
 * 		$(AFTER_DIR)/$(BASE_FILE)   	- user file
 * 		$(AFTER_SHAREDIR)/$(BASE_FILE) 	- global file  ( editable by root only )
 * 
 *  Colorscheme :
 * 	Look : 
 *  Feel : 
 * 		Loaded file :  - as decided by Session
 * 		Available Files : 
 * 			User files : 
 * 				$(AFTER_DIR)/$(_DIR)/<type>.*
 * 			Shared Files : 
 * 				$(AFTER_SHAREDIR)/$(_DIR)/<type>.*	 ( editable by root only )
 * 			Selected default :   
 * 				$(AFTER_DIR)/$(AFTER_NONCF)/0_<type>.#scr or :
 * 				$(AFTER_DIR)/$(AFTER_NONCF)/0_<type>
 * 			Default as installed :   
 * 				$(AFTER_SHAREDIR)/$(AFTER_NONCF)/0_<type>	 ( editable by root only )
 * 
 *  AutoExec : 
 * 		$(AFTER_DIR)/$(AUTOEXEC_FILE)      - user file	 
 * 		$(AFTER_SHAREDIR)/$(AUTOEXEC_FILE) - global file ( editable by root only )
 * 
 *  Startmenu :
 * 		$(AFTER_DIR)/$(START_DIR)      - user menu	 
 * 		$(AFTER_SHAREDIR)/$(START_DIR) - global menu ( editable by root only )
 * 
 * 	Theme override :
 * 		$(AFTER_DIR)/$(THEME_OVERRIDE_FILE)
 * 
 *  Database :
 * 		$(AFTER_DIR)/$(DATABASE_FILE)      	- user file	 
 * 		$(AFTER_SHAREDIR)/$(DATABASE_FILE) 	- global file ( editable by root only )
 * 
 * 	Pager :
 * 		$(AFTER_DIR)/pager      	- user file	 
 * 		$(AFTER_SHAREDIR)/pager 	- global file ( editable by root only )
 * 
 * 	Wharf :
 * 		$(AFTER_DIR)/wharf      	- user file	 
 * 		$(AFTER_SHAREDIR)/wharf 	- global file ( editable by root only )
 * 
 * 	WinList : 
 * 		$(AFTER_DIR)/winlist      	- user file	 
 * 		$(AFTER_SHAREDIR)/winlist 	- global file ( editable by root only )
 * 	
 * Possible outcomes : 
 * 
 * Base == Base
 * ColorScheme == Colorscheme
 * Look == Look + ThemeOverride + ModuleMyStyles
 * Feel == Feel + ThemeOverride
 * Functions == Feel + ThemeOverride + Autoexec + WorspaceState
 * Popups == Feel + ThemeOverride + startmenu
 * Database == Database
 * Pager == pager + Look.ModuleLook + ThemeOverride
 * Wharf == wharf + Look.ModuleLook + ThemeOverride
 * WinList == winlist + Look.ModuleLook + ThemeOverride
 * 
 * 
 */


#define LOCAL_DEBUG
#define EVENT_TRACE

#include "../../configure.h"
#include "../../libAfterStep/asapp.h"
#include <unistd.h>
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/aswindata.h"
#include "../../libAfterStep/event.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterStep/parser.h"
#include "../../libAfterStep/session.h"
#include "../../libAfterConf/afterconf.h"


/*************************************************************************/
typedef struct ASConfigFile {
	char *dirname ;
	char *filename ;
	char *myname ;
	
	char *fullname ;
	SyntaxDef *syntax ;

	Bool writeable ;

	FreeStorageElem *free_storage ;

}ASConfigFile;



typedef enum 
{
	ASProp_Phony = 0,
	ASProp_Integer,
	ASProp_Data,
	ASProp_File,
	ASProp_String,
	ASProp_ContentsTypes		   
}ASPropContentsType;

typedef struct ASProperty {

#define ASProp_Indexed				(0x01<<0)	
	ASFlagType flags ;
	
	int id ;                 /* same a options IDs from autoconf.h */

	ASPropContentsType type ;
	char *name ;
	int index ;

	union {
		int 		 integer ;
		ASStorageID  data;			
		ASConfigFile *config_file ;
		char 		 *string ;
	}contents;
	
	ASBiDirList *sub_props ;	   

	/* padding to 32 bytes */
	int reserved ;
}ASProperty;

/* hiererchy : 
 * root
 * 		Base
 * 			Options
 * 			File
 * 
 * 		ColorScheme
 * 			Options
 * 			File
 * 
 * 		Look 
 * 			MyStyles
 * 				mystyles...
 * 			MyFrames
 * 				myframes...
 * 			MyBackgrounds
 * 				mybackground...
 * 			TitleButtons
 * 				buttons...
 * 			Options
 * 			Files
 * 				look
 * 				theme-override
 * 				pager
 * 				wharf
 * 				winlist
 * 		Feel
 * 			Options
 * 			Files			
 * 				feel
 * 				theme-override
 * 
 * 		Functions
 * 			complex_functions...
 * 			Files
 * 				feel
 * 				theme-override
 *  			autoexec
 * 				workspace_state
 * 
 * 		Popups
 * 			menus...
 * 			Files
 * 				feel
 * 				theme-override
 * 				start
 * 
 * 		Database
 * 			Styles
 * 				styles...
 * 			Files
 * 				database
 * 
 * 		Pager
 * 			Options
 * 			Files
 * 				Look
 * 				theme-override
 * 				pager
 * 		Wharf
 * 			RootFolder
 * 				entries...
 * 				[SubFolder]
 * 			Options
 * 			Files
 * 				Look
 * 				theme-override
 * 			 	wharf
 * 		WinList 
 * 			Options
 * 			Files
 * 				Look
 * 				theme-override
 * 				winlist
 * 		PrivateFiles
 * 			base
 * 			database
 * 			autoexec
 * 			theme_override
 * 			pager
 * 			wharf
 * 			winlist
 * 			Colorschemes
 * 				schemes...
 * 			Looks
 * 				looks...
 * 			Feels
 * 				feels...
 * 			Backgrounds
 * 				backgrounds...
 * 		SharedFiles
 * 			base
 * 			database
 * 			autoexec
 * 			theme_override
 * 			pager
 * 			wharf
 * 			winlist
 * 			Colorschemes
 * 				schemes...
 * 			Looks
 * 				looks...
 * 			Feels
 * 				feels...
 * 			Backgrounds
 * 				backgrounds...
 */
  		
/*************************************************************************/


ASProperty *Root = NULL;



/*************************************************************************/
void
DeadPipe (int foo)
{
	{
		static int already_dead = False ; 
		if( already_dead ) 	return;/* non-reentrant function ! */
		already_dead = True ;
	}
    FreeMyAppResources();
#ifdef DEBUG_ALLOCS
    print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

	if( dpy ) 
	{	
    	XFlush (dpy);
    	XCloseDisplay (dpy);
	}
    exit (0);
}


void load_hierarchy();


int
main (int argc, char **argv)
{
	/* Save our program name - for error messages */
    InitMyApp (CLASS_ASCONFIG, argc, argv, NULL, NULL, 0 );
	InitSession();

	load_hierarchy();
	
	if( dpy )   
    	XCloseDisplay (dpy);
    return 0;
}
/**************************************************************************/
ASConfigFile *
load_config_file(const char *dirname, const char *filename, const char *myname, SyntaxDef *syntax )
{
	ASConfigFile * ascf ;
		
	if( (dirname == NULL && filename == NULL) || syntax == NULL ) 
		return NULL;

	ascf = safecalloc( 1, sizeof(ASConfigFile));
	if( dirname == NULL ) 
		parse_file_name(filename, &(ascf->dirname), &(ascf->filename));
	else
	{
		ascf->dirname = mystrdup( dirname );
		ascf->filename = mystrdup( filename );
	}	 

	ascf->fullname = dirname?make_file_name( dirname, filename ): mystrdup(filename) ;
	ascf->writeable = (check_file_mode (ascf->fullname, W_OK) == 0);
	ascf->myname = mystrdup(myname);

	ascf->free_storage = file2free_storage(ascf->fullname, ascf->myname, syntax, NULL );
	ascf->syntax = syntax ;

	return ascf;
}	 

void destroy_config_file( ASConfigFile *ascf ) 
{
	if( ascf->dirname ) 
		free( ascf->dirname );
	if( ascf->filename ) 
		free( ascf->filename );
	if( ascf->fullname ) 
		free( ascf->fullname );
	if( ascf->myname ) 
		free( ascf->myname );
	if( ascf->free_storage ) 
		DestroyFreeStorage( &(ascf->free_storage) );
	
}	 

/*************************************************************************/
void destroy_property( void *data );

ASProperty *
create_property( int id, ASPropContentsType type, const char *name, Bool tree )
{
	ASProperty *prop = safecalloc( 1, sizeof(ASProperty));
	prop->id = id ;
	prop->type = type ;
	prop->name = mystrdup(name) ;
	if( tree ) 
		prop->sub_props = create_asbidirlist( destroy_property ) ;
	return prop;
}

void destroy_property( void *data )
{
	ASProperty *prop = (ASProperty*)data;

	if( prop )
	{	
		if( prop->name ) 
			free( prop->name );
		switch( prop->type ) 
		{
			case ASProp_Phony : break;
			case ASProp_Integer : break;
			case ASProp_Data : 
				forget_data(NULL, prop->contents.data); 
				break ;
			case ASProp_File : 
				destroy_config_file( prop->contents.config_file ); 
				break;
			case ASProp_String : 
				if( prop->contents.string ) 
					free(prop->contents.string);
				break;
			default: break;
		}	 
	
		if( prop->sub_props ) 
			destroy_asbidirlist( &(prop->sub_props) ); 
		free( prop );
	}
}	 

void 
free_storage2property_list( FreeStorageElem *fs, ASProperty *pl )
{
	FreeStorageElem *curr = fs ;
	ConfigItem    item;
	ASProperty *prop ;
	
	item.memory = NULL;
	
	while( curr ) 
	{
		if (curr->term == NULL)
			continue;
		if (!ReadConfigItem (&item, curr))
			continue;

		if( (curr->term->type == TT_QUOTED_TEXT || curr->term->type == TT_TEXT) &&
			 curr->term->sub_syntax != NULL && 
			 !get_flags(curr->term->flags, TF_INDEXED|TF_DIRECTION_INDEXED) )
		{	
			prop = create_property( curr->term->id, ASProp_Phony, item.data.string, (curr->sub != NULL) );	 
		}else                  /* we actually have some data */
		{
			prop = special_free_storage2property( curr );
			if( prop == NULL ) 
			{	
				int type = ASProp_Phony;
				char *name = NULL ;

				switch( curr->term->type )
				{
					case TT_FLAG : 
					case TT_INTEGER : 
					case TT_UINTEGER :
					case TT_BITLIST : 	type = ASProp_Integer ; break ;

					case TT_COLOR : 	
					case TT_FONT : 		
					case TT_FILENAME : 	
					case TT_PATHNAME : 	
					case TT_TEXT : 
					case TT_QUOTED_TEXT :
					case TT_OPTIONAL_PATHNAME : type = ASProp_Data ; break ;
					case TT_GEOMETRY :	 /* handled by special_ as complex datatype */ break ;
					case TT_SPECIAL : 	/* handled by special_ as complex datatype */ break ;
					case TT_FUNCTION : 	/* handled by special_ as complex datatype */ break ;
					case TT_BOX :		/* handled by special_ as complex datatype */ break ;
					case TT_BUTTON :	/* handled by special_ as complex datatype */ break ;
					case TT_BINDING : 	/* handled by special_ as complex datatype */ break ;
					case TT_INTARRAY : 	/* handled by special_ as complex datatype */ break ;
					case TT_CURSOR : 	/* handled by special_ as complex datatype */ break ;
				}	 
				prop = create_property( curr->term->id, type, name, (curr->sub != NULL) );	 
			}
		}	 

		if( get_flags(curr->term->flags, TF_INDEXED|TF_DIRECTION_INDEXED))
		{	
			prop->index = item.index ;
			set_flags( prop->flags, ASProp_Indexed );
		}
	}		   
}


void 
merge_property_list( ASProperty *src, ASProperty *dst )
{
		
	
}

/*************************************************************************/
ASProperty* load_Base();


void 
load_hierarchy()
{
	ASProperty *tmp ;
	
	Root = create_property( CONFIG_root_ID, ASProp_Phony, "", True );
		
	if( (tmp = load_Base()) != NULL ) 
		append_bidirelem( Root->sub_props, tmp );
		
	
}
/*************************************************************************/
ASProperty* 
load_Base()
{
	ASProperty *base = create_property( CONFIG_Base_ID, ASProp_Phony, "Base", True );
	char *filename = make_session_file(Session, BASE_FILE, False );
	ASConfigFile *cf = NULL ;

	if( filename )
	{	
		cf = load_config_file(NULL, filename, "afterstep", &BaseSyntax );
		free( filename );
	}
	
	if( cf ) 
	{
		ASProperty *file, *opts ;

		file = create_property( CONFIG_BaseFile_ID, ASProp_File, "File", True );
		file->contents.config_file = cf ;
		
		free_storage2property_list( cf->free_storage, file );
 		append_bidirelem( base->sub_props, file );			   
			
		opts = create_property( CONFIG_BaseOptions_ID, ASProp_Phony, "Options", True );
		merge_property_list( file, opts );
		append_bidirelem( base->sub_props, opts );			   
	}	 
	return base;
}	 

