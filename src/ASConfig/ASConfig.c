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
#include "../../libAfterStep/mystyle.h"
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
	ASProp_Char,
	ASProp_ContentsTypes		   
}ASPropContentsType;

typedef struct ASProperty {

#define ASProp_Indexed				(0x01<<0)	
#define ASProp_Merged				(0x01<<1)	  
	ASFlagType flags ;
	
	ASStorageID id ;                 /* same a options IDs from autoconf.h */

	ASPropContentsType type ;
	char *name ;
	int index ;

	union {
		int 		 integer ;
		ASStorageID  data;			
		ASConfigFile *config_file ;
		char 		 c ;
	}contents;
	
	ASBiDirList *sub_props ;	   

	/* padding to 32 bytes */
	int reserved ;
}ASProperty;

/* hiererchy : 
 * root
 * 		Base
 * 			Options
 * 			BaseFile
 * 		ColorScheme
 * 			Options
 * 			ColorSchemeFile
 * 		Functions
 * 			complex_functions...
 * 			Files
 * 				feel
 * 				theme-override
 *  			autoexec
 * 		Popups
 * 			menus...
 * 			Files
 * 				feel
 * 				theme-override
 * 				start
 * 		Database
 *			styles...
 * 			DatabaseFile
 * 
 * Module Specific : 
 * 		Module "afterstep"
 * 			Look 
 * 				MyStyles
 * 					mystyles...
 * 				MyFrames
 * 					myframes...
 *	 			MyBackgrounds
 * 					mybackground...
 * 				TitleButtons
 * 					buttons...
 *	 			Options
 * 				Files
 * 					look
 * 					theme-override
 * 			Feel
 * 				Options
 * 				Files			
 * 					feel
 *	 				theme-override
 * 		Module "Pager"
 * 			Options
 * 			PagerLook
 * 				MyStyles
 * 					...
 * 				Options
 * 				Files
 * 					look
 * 					theme-override
 * 					pager
 * 
 * 			PagerFeel
 * 				Options
 * 				Files
 * 					feel
 * 					theme-override
 * 					pager
 * 			PagerFile
 * 		Module "Wharf"
 * 			RootFolder
 * 				entries...
 * 				[SubFolder]
 * 			Options
 * 			WharfLook
 * 				MyStyles
 * 					...
 * 				Options
 * 				Files
 * 					look
 * 					theme-override
 * 					wharf
 * 
 * 			WharfFeel
 * 				Options
 * 				Files
 * 					feel
 * 					theme-override
 * 					wharf
 * 			WharfFile
 * 		Module "WinList" 
 * 			Options
 * 			WinListLook
 * 				MyStyles
 * 					...
 * 				Options
 * 				Files
 * 					look
 * 					theme-override
 * 					winlist
 * 
 * 			WinListFeel
 * 				Options
 * 				Files
 * 					feel
 * 					theme-override
 * 					wharf
 * 			WinListFile
 **************************************************************************
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
typedef struct ASConfigFileInfo
{
	int config_file_id ; 
	char *session_file ; 
	Bool non_freeable ; 
	char *tmp_override_file ;
			  
}ASConfigFileInfo;
ASConfigFileInfo ConfigFilesInfo[] = 
{
{CONFIG_BaseFile_ID, 		NULL, 0, NULL },
{CONFIG_ColorSchemeFile_ID, NULL, 0, NULL },
{CONFIG_LookFile_ID, 		NULL, 0, NULL },
{CONFIG_FeelFile_ID, 		NULL, 0, NULL },
{CONFIG_AutoExecFile_ID,	NULL, 0, NULL },
{CONFIG_StartDir_ID,		NULL, 0, NULL },			
{CONFIG_LookFile_ID,		NULL, 0, NULL },				
{CONFIG_ThemeOverrideFile_ID,NULL, 0, NULL },		
{CONFIG_DatabaseFile_ID,	NULL, 0, NULL },			
{CONFIG_PagerFile_ID, 		NULL, 0, NULL },				
{CONFIG_WharfFile_ID, 		NULL, 0, NULL },				
{CONFIG_WinListFile_ID,		NULL, 0, NULL },
{0,NULL, 0, NULL}
};

typedef struct ASConfigTypeInfo
{
	int config_id ; 
	SyntaxDef *syntax ;
	int config_files_id ; 
	int config_file_ids_count ;
#define MAX_CONFIG_FILE_IDS		32
	int config_file_ids[MAX_CONFIG_FILE_IDS] ; 

	int config_options_id ;
			  
}ASConfigTypeInfo;
ASConfigTypeInfo ConfigTypeInfo[] = 
{
   	{ CONFIG_Base_ID, 		 &BaseSyntax,  0, 					1, {CONFIG_BaseFile_ID}, 			CONFIG_BaseOptions_ID},
	{ CONFIG_ColorScheme_ID, &ColorSyntax, 0, 					1, {CONFIG_ColorSchemeFile_ID}, 	CONFIG_ColorSchemeOptions_ID},	  
	{ CONFIG_Functions_ID, 	 &FunctionSyntax, CONFIG_FunctionsFiles_ID,  3, {CONFIG_FeelFile_ID,
																			 CONFIG_ThemeOverrideFile_ID,
																			 CONFIG_AutoExecFile_ID}, 0},	 
	{ CONFIG_Popups_ID, 	 &PopupSyntax, CONFIG_PopupsFiles_ID,  3, {CONFIG_FeelFile_ID,
																	   CONFIG_ThemeOverrideFile_ID,
																	   CONFIG_StartDir_ID}, 0},	 
	{ CONFIG_Database_ID, 	 &DatabaseSyntax, 0, 				   1, {CONFIG_DatabaseFile_ID}, 0},	  
  	{0}	 
};

/*************************************************************************/
void
init_ConfigFileInfo()
{
	int i = 0;
	while( ConfigFilesInfo[i].config_file_id != 0 ) 
	{
		switch( ConfigFilesInfo[i].config_file_id )
		{

			case CONFIG_BaseFile_ID :	
				ConfigFilesInfo[i].session_file	= make_session_file(Session, BASE_FILE, False ); 
				break ;	   
			case CONFIG_ColorSchemeFile_ID :	
				ConfigFilesInfo[i].session_file = (char*)get_session_file (Session, 0, F_CHANGE_COLORSCHEME, False); 
				ConfigFilesInfo[i].non_freeable = True ;
				break ;	  
			case CONFIG_LookFile_ID :	   
				ConfigFilesInfo[i].session_file = (char*)get_session_file (Session, 0, F_CHANGE_LOOK, False);
				ConfigFilesInfo[i].non_freeable = True ;
				break ;	  
			case CONFIG_FeelFile_ID :	   
				ConfigFilesInfo[i].session_file = (char*)get_session_file (Session, 0, F_CHANGE_FEEL, False);
				ConfigFilesInfo[i].non_freeable = True ;
				break ;	
			case CONFIG_AutoExecFile_ID :	   
				ConfigFilesInfo[i].session_file = (char*)get_session_file (Session, 0, F_CHANGE_FEEL, False);
				ConfigFilesInfo[i].non_freeable = True ;
				break ;	
			case CONFIG_StartDir_ID :	   
				ConfigFilesInfo[i].session_file = make_session_dir (Session, START_DIR, False); 
				break ;	
			case CONFIG_ThemeOverrideFile_ID :	   
				ConfigFilesInfo[i].session_file = make_session_data_file  (Session, False, R_OK, THEME_OVERRIDE_FILE, NULL );	
				break ;
			case CONFIG_DatabaseFile_ID :	   
				ConfigFilesInfo[i].session_file = make_session_file(Session, DATABASE_FILE, False ); 
				break ;	   
			case CONFIG_PagerFile_ID :	   
				ConfigFilesInfo[i].session_file = make_session_file(Session, "pager", False ); 
				break ;	 
			case CONFIG_WharfFile_ID :	   
				ConfigFilesInfo[i].session_file = make_session_file(Session, "wharf", False ); 
				break ;	 
			case CONFIG_WinListFile_ID :	   
				ConfigFilesInfo[i].session_file = make_session_file(Session, "winlist", False ); 
				break ;	 
		}	 
		++i ;	
	}	 
	
}	  

const char*
get_config_file_name( int config_id ) 
{
	int i = 0;
	while( ConfigFilesInfo[i].config_file_id != 0 ) 
	{
		if( ConfigFilesInfo[i].config_file_id == config_id )
		{	
			if( ConfigFilesInfo[i].tmp_override_file != NULL )
				return ConfigFilesInfo[i].tmp_override_file;
			else
				return ConfigFilesInfo[i].session_file;
		}
		++i ;	  
	}		
	return NULL;
}	 

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
/*************************************************************************/
/*************************************************************************/
void load_hierarchy();
void print_hierarchy( ASProperty *root, int level );
void register_special_keywords();
/*************************************************************************/
/*************************************************************************/
int
main (int argc, char **argv)
{
	/* Save our program name - for error messages */
    InitMyApp (CLASS_ASCONFIG, argc, argv, NULL, NULL, 0 );
	InitSession();

	register_special_keywords();
	init_ConfigFileInfo();

	LOCAL_DEBUG_OUT("loading hierarchy%s","");
	load_hierarchy();
	
	print_hierarchy(Root, 0);
	
	if( dpy )   
    	XCloseDisplay (dpy);
    return 0;
}
/**************************************************************************/
void register_special_keywords()
{
#define REG_SPEC_KEYWORD(k)		register_keyword_id( #k, CONFIG_##k##_ID )

	REG_SPEC_KEYWORD(root);
	REG_SPEC_KEYWORD(Base);
	REG_SPEC_KEYWORD(ColorScheme);
	REG_SPEC_KEYWORD(Functions);
	REG_SPEC_KEYWORD(Popups);
	REG_SPEC_KEYWORD(Database);
	REG_SPEC_KEYWORD(Module);
                     	
	REG_SPEC_KEYWORD(AfterStep);
	REG_SPEC_KEYWORD(Pager);
	REG_SPEC_KEYWORD(Wharf);
	REG_SPEC_KEYWORD(WinList);
                     	
	REG_SPEC_KEYWORD(Look);
	REG_SPEC_KEYWORD(Feel);
	REG_SPEC_KEYWORD(PagerLook);
	REG_SPEC_KEYWORD(PagerFeel);
	REG_SPEC_KEYWORD(WharfLook);
	REG_SPEC_KEYWORD(WharfFeel);
	REG_SPEC_KEYWORD(WinListLook);
	REG_SPEC_KEYWORD(WinListFeel);
                     	
	REG_SPEC_KEYWORD(LookFile);
	REG_SPEC_KEYWORD(FeelFile);
	REG_SPEC_KEYWORD(StartDir);
	REG_SPEC_KEYWORD(AutoExecFile);
	REG_SPEC_KEYWORD(PagerFile);
	REG_SPEC_KEYWORD(WharfFile);
	REG_SPEC_KEYWORD(WinListFile);
	REG_SPEC_KEYWORD(ThemeOverrideFile);
	REG_SPEC_KEYWORD(BaseFile);			
	REG_SPEC_KEYWORD(ColorSchemeFile);
	REG_SPEC_KEYWORD(DatabaseFile);		

                     	
	REG_SPEC_KEYWORD(FunctionsFiles);
	REG_SPEC_KEYWORD(PopupsFiles);
	REG_SPEC_KEYWORD(LookFiles);
	REG_SPEC_KEYWORD(FeelFiles);
	REG_SPEC_KEYWORD(BackgroundFiles);
	REG_SPEC_KEYWORD(PrivateFiles);
	REG_SPEC_KEYWORD(SharedFiles);
                     	
	REG_SPEC_KEYWORD(BaseOptions);
	REG_SPEC_KEYWORD(ColorSchemeOptions);
	REG_SPEC_KEYWORD(MyStyles);
	REG_SPEC_KEYWORD(MyFrames);
	REG_SPEC_KEYWORD(MyBackgrounds);
	REG_SPEC_KEYWORD(TitleButtons);
	REG_SPEC_KEYWORD(LookOptions);
	REG_SPEC_KEYWORD(FeelOptions);
	REG_SPEC_KEYWORD(PagerOptions);
	REG_SPEC_KEYWORD(WharfOptions);
	REG_SPEC_KEYWORD(WinListOptions);

	REG_SPEC_KEYWORD(flags);
	REG_SPEC_KEYWORD(x);
	REG_SPEC_KEYWORD(y);
	REG_SPEC_KEYWORD(width);
	REG_SPEC_KEYWORD(height); 			   	
                     	
	REG_SPEC_KEYWORD(type);	 			   	
	REG_SPEC_KEYWORD(tint);	 			   	   
	REG_SPEC_KEYWORD(pixmap);
	
	REG_SPEC_KEYWORD(hotkey);
	REG_SPEC_KEYWORD(text);
	REG_SPEC_KEYWORD(value);
	REG_SPEC_KEYWORD(unit);
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

ASConfigFile *
dup_config_file( ASConfigFile *src )
{
	ASConfigFile *dst = NULL ;
	if( src != NULL	)
	{
		dst = safecalloc( 1, sizeof(ASConfigFile));
		*dst = *src ;
		if( src->dirname ) 
			dst->dirname = mystrdup(src->dirname);
		if( src->filename ) 
			dst->filename = mystrdup( src->filename );
		if( src->fullname ) 
			dst->fullname = mystrdup( src->fullname );
		if( src->myname ) 
			dst->myname = mystrdup( src->myname );
		if( src->free_storage ) 
		{
			dst->free_storage = NULL ;
			CopyFreeStorage (&(dst->free_storage), src->free_storage);
		}
	}	 

	return dst;
	
}	   

/*************************************************************************/
ASStorageID 
encode_string( const char *str ) 
{
	if( str == NULL ) 
		return 0;
	return store_data( NULL, (CARD8*)str, strlen(str)+1, ASStorage_RLEDiffCompress|ASStorage_NotTileable, 0);
}

int 
decode_string( ASStorageID id, char *buffer, int buffer_length, int *stored_length )
{
	int bytes_out = 0 ;

	if( stored_length )
		*stored_length = 0 ;

	if( id != 0 && buffer && buffer_length > 0 )
	{
		bytes_out = fetch_data(NULL, id, &buffer[0], 0, buffer_length-1, 0, stored_length);	
		buffer[bytes_out] = '\0' ;
	}
	return bytes_out;		
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

ASProperty *
add_integer_property( int id, int val, ASBiDirList *owner_tree )
{
	ASProperty *prop = create_property( id, ASProp_Integer, NULL, False );
	prop->contents.integer = val ;
	
	if( owner_tree ) 
		append_bidirelem( owner_tree, prop );

	return prop;	 
}	 

ASProperty *
add_char_property( int id, char c, ASBiDirList *owner_tree )
{
	ASProperty *prop = create_property( id, ASProp_Char, NULL, False );
	prop->contents.c = c ;
	
	if( owner_tree ) 
		append_bidirelem( owner_tree, prop );

	return prop;	 
}	 

ASProperty *
add_string_property( int id, char *str, ASBiDirList *owner_tree )
{
	ASProperty *prop = create_property( id, ASProp_Data, NULL, False );
	prop->contents.data = encode_string( str );
	
	if( owner_tree ) 
		append_bidirelem( owner_tree, prop );

	return prop;	 
}	 


void merge_property_list( ASProperty *src, ASProperty *dst );

void
dup_property_contents( ASProperty *src, ASProperty *dst ) 
{
	if( src->type == ASProp_Integer ) 
		dst->contents.integer = src->contents.integer ;
	else if( src->type == ASProp_Data ) 
		dst->contents.data = dup_data( NULL, src->contents.data );
	else if( src->type == ASProp_File )
		dst->contents.config_file = dup_config_file( src->contents.config_file );	

	if( src->sub_props != NULL ) 
		merge_property_list( src, dst );
}

ASProperty *
dup_property( ASProperty *src ) 
{
	ASProperty *dst	= NULL ;

	if( src )
	{
		dst = create_property( src->id, src->type, src->name, (src->sub_props != NULL) );
		dst->flags = src->flags ;
		dst->index = src->index ;
		dup_property_contents( src, dst ); 
	}
	
	return dst;	 
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
			case ASProp_Char : 
				break;
			default: break;
		}	 
	
		if( prop->sub_props ) 
			destroy_asbidirlist( &(prop->sub_props) ); 
		free( prop );
	}
}

ASProperty *
find_property_by_id( ASBiDirList *list, int id )	 
{
	ASBiDirElem *curr = LIST_START(list); 		
	while( curr ) 
	{
		ASProperty *prop = (ASProperty*)LISTELEM_DATA(curr) ;	  
		if( prop->id == id ) 
			return prop;
		LIST_GOTO_NEXT(curr);
	}
	return NULL;
}


/*************************************************************************/

ASProperty *
special_free_storage2property( FreeStorageElem **pcurr )
{
	FreeStorageElem *curr = *pcurr ;
	ASProperty *prop = NULL ;
	int type = curr->term->type ;
	ConfigItem    item;
	item.memory = NULL;
	
	ReadConfigItem (&item, curr);

	if( type == MYSTYLE_BACKGRADIENT_ID ) 
	{
				/* TODO */				   
	}else if( type == MYSTYLE_BACKGRADIENT_ID ) 
	{
				/* TODO */				   
	}else if( type == MYSTYLE_BACKPIXMAP_ID )
	{
		ASStorageID text_id = 0 ;
		ASProperty *type = NULL ;
		ASProperty *val = NULL ;

		prop = create_property( curr->term->id, ASProp_Phony, NULL, True );	 				   
		type = create_property( CONFIG_type_ID, ASProp_Integer, NULL, False );	 				   
		type->contents.integer = item.data.integer;
		append_bidirelem( prop->sub_props, type );			   

		if (curr->argc > 1)
			text_id = encode_string( curr->argv[1] ); 
		if( text_id != 0 ) 
		{	
			if( item.data.integer == TEXTURE_TRANSPARENT || 
				item.data.integer == TEXTURE_TRANSPARENT_TWOWAY ) 
			{
				val = create_property( CONFIG_type_ID, ASProp_Data, NULL, False );	 				   							
			}else
				val = create_property( CONFIG_pixmap_ID, ASProp_Data, NULL, False );	 				   							   
			val->contents.data = text_id;
			append_bidirelem( prop->sub_props, val );			   
		}
	}

	if( prop == NULL ) 
	{	
		switch( curr->term->type ) 
		{	
			case TT_GEOMETRY :
				prop = create_property( curr->term->id, ASProp_Phony, NULL, True );
				add_integer_property( CONFIG_flags_ID, item.data.geometry.flags, prop->sub_props );
				if( get_flags(item.data.geometry.flags, XValue ) )
					add_integer_property( CONFIG_x_ID, item.data.geometry.x, prop->sub_props );
				if( get_flags(item.data.geometry.flags, YValue ) )
					add_integer_property( CONFIG_y_ID, item.data.geometry.y, prop->sub_props );
				if( get_flags(item.data.geometry.flags, WidthValue ) )
					add_integer_property( CONFIG_width_ID, item.data.geometry.width, prop->sub_props );
				if( get_flags(item.data.geometry.flags, HeightValue ) )
					add_integer_property( CONFIG_height_ID, item.data.geometry.height, prop->sub_props );
		    	break ;
	 		case TT_SPECIAL : 	/* should be handled based on its type : */ 
				if( type == WHARF_Wharf_ID )			
				{
					/* TODO */				
				
				}	 
				break ;
	 		case TT_FUNCTION :
				{	
					FunctionData *pfunc = (item.data.function);
					ASProperty *tmp ;
					prop = create_property( curr->term->id, ASProp_Phony, pfunc->name, True );
					if( pfunc->hotkey != '\0' )
						add_char_property( CONFIG_hotkey_ID, pfunc->hotkey, prop->sub_props );				
					if( pfunc->text ) 
					{	
						add_string_property( CONFIG_text_ID, pfunc->text, prop->sub_props );				   
					
						tmp = add_integer_property( CONFIG_value_ID, pfunc->func_val[0], prop->sub_props );
						set_flags( tmp->flags, ASProp_Indexed );
						if( pfunc->unit[0] != '\0' )
						{	
							tmp = add_char_property( CONFIG_unit_ID, pfunc->unit[0], prop->sub_props );				
							set_flags( tmp->flags, ASProp_Indexed );
						}
						tmp = add_integer_property( CONFIG_value_ID, pfunc->func_val[1], prop->sub_props );
						set_flags( tmp->flags, ASProp_Indexed );
						tmp->index = 1 ;
						if( pfunc->unit[1] != '\0' )
						{	
							tmp = add_char_property( CONFIG_unit_ID, pfunc->unit[1], prop->sub_props );				
							set_flags( tmp->flags, ASProp_Indexed );
							tmp->index = 1 ;
						}
					}
					if( curr->next != NULL ) 
					{
						curr = curr->next ;
						if( curr->term->id == F_MINIPIXMAP )
						{
							ReadConfigItem (&item, curr);	
							pfunc = (item.data.function);
							if( pfunc->name != NULL ) 
								add_string_property( F_MINIPIXMAP, pfunc->name, prop->sub_props );				   							   
							else
								add_string_property( F_MINIPIXMAP, pfunc->text, prop->sub_props );				   							   
							*pcurr = curr ;
						}	 
					}	 
				}
				break ;
	 		case TT_BOX :		/* TODO */ break ;
	 		case TT_BUTTON :	/* TODO */ break ;
	 		case TT_BINDING : 	/* TODO */ break ;
	 		case TT_INTARRAY : 	/* TODO */ break ;
	 		case TT_CURSOR : 	/* TODO */ break ;
		}
	}
	ReadConfigItem (&item, NULL);

	return prop;
}

void 
free_storage2property_list( FreeStorageElem *fs, ASProperty *pl )
{
	FreeStorageElem *curr = fs ;
	ConfigItem    item;
	ASProperty *prop ;
	
	LOCAL_DEBUG_CALLER_OUT("(%p,%p)", fs, pl );	  
	item.memory = NULL;
	
	for( curr = fs ; curr ; curr = curr->next ) 
	{
		if (curr->term == NULL)
			continue;
		if ( get_flags( curr->term->flags, TF_SYNTAX_TERMINATOR ) )
			continue;
		if (!ReadConfigItem (&item, curr))
			continue;
		if( curr == fs && pl->id == curr->term->id )
			continue;

		if( (curr->term->type == TT_QUOTED_TEXT || curr->term->type == TT_TEXT) &&
			 curr->term->sub_syntax != NULL && 
			 !get_flags(curr->term->flags, TF_INDEXED|TF_DIRECTION_INDEXED) )
		{	
			prop = create_property( curr->term->id, ASProp_Phony, item.data.string, (curr->sub != NULL) );	 
		}else                  /* we actually have some data */
		{
			prop = special_free_storage2property( &curr );
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
					default:
					/* handled by special_ as complex datatype */ break ;
				}	 
				prop = create_property( curr->term->id, type, name, (curr->sub != NULL) );	 
				if( type == ASProp_Data ) 
				{
					if( item.data.string == NULL ) 
						prop->contents.data = 0 ;
					else
						prop->contents.data = encode_string( item.data.string );
					/* LOCAL_DEBUG_OUT( "stored with id = %d, string = \"%s\"", prop->contents.data, item.data.string ); */
				}else
					prop->contents.integer = item.data.integer ;
			}
		}	 

		if( get_flags(curr->term->flags, TF_INDEXED|TF_DIRECTION_INDEXED))
		{	
			prop->index = item.index ;
			set_flags( prop->flags, ASProp_Indexed );
		}

		if( curr->sub != NULL )
			free_storage2property_list( curr->sub, prop ) ;

		if( prop ) 
			append_bidirelem( pl->sub_props, prop );			   
	}		   
}

Bool
merge_prop_into_prop(void *data, void *aux_data)	
{
	ASProperty *dst = (ASProperty*)data;
	ASProperty *src = (ASProperty*)aux_data;
	
	if( dst->id != src->id || dst == src ) 
		return True;

	if( get_flags( src->flags, ASProp_Indexed ) && get_flags( dst->flags, ASProp_Indexed ) )
	{	
		if( src->index != dst->index )
			return True;
	}else if( src->name != NULL && dst->name != NULL )
		if( strcmp( src->name, dst->name ) )
			return True;

	/* otherwise we have to merge it  : */
	dup_property_contents( src, dst ); 
		   
	set_flags( src->flags, ASProp_Merged );		
	return True;	   
}

Bool
merge_prop_into_list(void *data, void *aux_data)
{
	ASProperty *prop = (ASProperty*)data;
	ASBiDirList *list = (ASBiDirList*)aux_data;

	clear_flags( prop->flags, ASProp_Merged );
	iterate_asbidirlist( list, merge_prop_into_prop, prop, NULL, False );		  	 	
	if( !get_flags( prop->flags, ASProp_Merged )  )
	{
		prop = dup_property( prop );	
		append_bidirelem( list, prop );			   	
	}	 

	return True;	
}

void 
merge_property_list( ASProperty *src, ASProperty *dst )
{
	if( src->sub_props == NULL && dst->sub_props == NULL ) 
		return;
	LOCAL_DEBUG_CALLER_OUT("(%p,%p)", src, dst );	
	iterate_asbidirlist( src->sub_props, merge_prop_into_list, dst->sub_props, NULL, False );		  	
}
/*************************************************************************/
ASProperty* 
load_current_config_fname( ASProperty* config, int id, const char *filename, const char *myname, 
					 SyntaxDef *syntax, int files_id, int file_id, int options_id )
{
	const char *filename = get_config_file_name( file_id ); 
	ASConfigFile *cf = NULL ;
	ASProperty *files = NULL ;
	ASProperty *file ;
	ASProperty *opts ;

	if( config == NULL ) 
		config = create_property( id, ASProp_Phony, NULL, True );

	if( filename )
	{
		LOCAL_DEBUG_OUT("loading file \"%s\"", filename );
		cf = load_config_file(NULL, filename, myname?myname:"afterstep", syntax );
	}
	

	if( files_id != 0 ) 
	{
		files = find_property_by_id( config->sub_props, files_id );
		if( files == NULL )
		{
			files = create_property( files_id, ASProp_Phony, NULL, True );
			append_bidirelem( config->sub_props, files );			   
		}
	}	 
	
	file = find_property_by_id( files?files->sub_props:config->sub_props, file_id );	   
	if( file == NULL ) 
	{	
		file = create_property( file_id, ASProp_File, NULL, True );
		append_bidirelem( files?files->sub_props:config->sub_props, file );			   
	}
	file->contents.config_file = cf ;
	
	if( cf ) 
		free_storage2property_list( cf->free_storage, file );
	else if( file_id == CONFIG_StartDir_ID ) 
	{
		
		
	}	 

	if( options_id != 0 )
	{	
		opts = find_property_by_id( config->sub_props, options_id );
		if( opts == NULL )		   
		{	
			opts = create_property( options_id, ASProp_Phony, NULL, True );
			append_bidirelem( config->sub_props, opts );			   
		}
	}else
		opts = config ;

	merge_property_list( file, opts );
	
	return config;
}	 

ASProperty* 
load_current_config( ASProperty* config, int id, const char *myname, 
					 SyntaxDef *syntax, int files_id, int file_id, int options_id )
{
	const char *filename = get_config_file_name( file_id ); 
	return load_current_config_fname( config, id, filename, myname, syntax, files_id, file_id, options_id );

}


/*************************************************************************/
void load_global_configs();

void 
load_hierarchy()
{
	Root = create_property( CONFIG_root_ID, ASProp_Phony, "", True );
		
	load_global_configs();
}

/*************************************************************************/
void
load_global_configs()
{
	int i ;
	ASProperty *tmp ;

	for( i = 0 ; ConfigTypeInfo[i].config_id != 0 ; ++i )
	{	
		int k ;
		tmp = NULL ;
		for( k = 0 ; k < ConfigTypeInfo[i].config_file_ids_count ; ++k )
		{	
			tmp = load_current_config( 	tmp,
										ConfigTypeInfo[i].config_id, NULL, 
										ConfigTypeInfo[i].syntax, 
										ConfigTypeInfo[i].config_files_id, 
										ConfigTypeInfo[i].config_file_ids[k], 
										ConfigTypeInfo[i].config_options_id);
		}
		if( tmp ) 
			append_bidirelem( Root->sub_props, tmp );
	}
}	 
/*************************************************************************/
void print_hierarchy( ASProperty *root, int level );

Bool
print_hierarchy_iter_func(void *data, void *aux_data)
{
    ASProperty *prop = (ASProperty*)data;
	int level = (int)aux_data ;
	print_hierarchy( prop, level );		
	return True;
}

void 
print_hierarchy( ASProperty *root, int level )
{
	int i ;

	//LOCAL_DEBUG_CALLER_OUT("(%p,%d)",root, level );

	if( root == NULL ) 
		return ;

	for( i = 0 ; i < level ; ++i ) 
		fputc( '\t', stderr);
	fprintf( stderr, "%s(%ld) ", keyword_id2keyword(root->id), root->id );
	if( get_flags( root->flags, ASProp_Indexed ) ) 
	   	fprintf( stderr, "[%d]", root->index );
	else if( root->name )
		fprintf( stderr, "\"%s\"", root->name );
	
	if( root->type == ASProp_Integer ) 
		fprintf( stderr, "= %d;", root->contents.integer );	
	else if( root->type == ASProp_Data ) 
	{
		static char string[128] ;	  
		int bytes_out, orig_bytes ;
		/*LOCAL_DEBUG_OUT( "fetching data with id = %d", root->contents.data );  */
		bytes_out = decode_string( root->contents.data, &string[0], 128, &orig_bytes );
		/* LOCAL_DEBUG_OUT( "fetched %d bytes", bytes_out ); */
		if( bytes_out < orig_bytes ) 
			fprintf( stderr, "= %d of %d chars:", bytes_out, orig_bytes );	 
		else
			fprintf( stderr, "= ");
		fprintf( stderr, "\"%s\"", string );	
		fputc( ';', stderr);
	}else if( root->type == ASProp_File )    
	{
		if( root->contents.config_file )
			fprintf( stderr, " loaded from [%s]", root->contents.config_file->fullname );
		else
			fprintf( stderr, " not loaded" );
	}

	fputc( '\n', stderr);
	if( root->sub_props ) 
	{
	 	iterate_asbidirlist( root->sub_props, print_hierarchy_iter_func, (void*)(level+1), NULL, False );		  
	}
	
}	 
