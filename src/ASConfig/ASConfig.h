#ifndef ASCONFIG_H_HEADER_INCLUDED
#define ASCONFIG_H_HEADER_INCLUDED

struct ASProperty;


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
 *  Database :
 * 		$(AFTER_DIR)/$(DATABASE_FILE)      	- user file	 
 * 		$(AFTER_SHAREDIR)/$(DATABASE_FILE) 	- global file ( editable by root only )
 * 
 * 	Afterstep :
 * 		$(AFTER_DIR)/afterstep
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
 * Functions == Feel + afterstep + Autoexec + WorspaceState
 * Popups == Feel + afterstep + startmenu
 * Database == Database
 * Module specific : 
 * 	Look == Look + module_config
 * 	Feel == Feel + module_config
 * 	afterstep == afterstep
 * 	Pager == pager
 * 	Wharf == wharf
 * 	WinList == winlist
 * 
 * 
 */


/*************************************************************************/

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
 * 				afterstep
 *  			autoexec
 * 		Popups
 * 			menus...
 * 			Files
 * 				feel
 * 				afterstep
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
 * 					afterstep
 * 			Feel
 * 				Options
 * 				Files			
 * 					feel
 *	 				afterstep
 * 		Module "Pager"
 * 			PagerLook
 * 				MyStyles
 * 					...
 * 				Options
 * 				Files
 * 					look
 * 					pager
 * 
 * 			PagerFeel
 * 				Options
 * 				Files
 * 					feel
 * 					pager
 * 		Module "Wharf"
 * 			Folders	
 *		 		RootFolder
 * 					entries...
 * 					[SubFolder]
 * 				WharfFile
 * 			Sounds
 * 				Sounds
 * 					entries...
 * 				WharfFile
 * 			WharfLook
 * 				MyStyles
 * 					...
 * 				Options
 * 				Files
 * 					look
 * 					wharf
 * 
 * 			WharfFeel
 * 				Options
 * 				Files
 * 					feel
 * 					wharf
 * 		Module "WinList" 
 * 			WinListLook
 * 				MyStyles
 * 					...
 * 				Options
 * 				Files
 * 					look
 * 					winlist
 * 
 * 			WinListFeel
 * 				Options
 * 				Files
 * 					feel
 * 					wharf
 **************************************************************************
 * 		PrivateFiles
 * 			base
 * 			database
 * 			autoexec
 * 			afterstep
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
 * 			afterstep
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

extern struct ASProperty *Root;

/*************************************************************************/
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
extern ASConfigTypeInfo ConfigTypeInfo[];

typedef struct ASModuleSyntax
{
	int config_id ;
	SyntaxDef *syntax ; 
	SpecialFunc special ;
	int config_files_id ;
	int *files ; 	   
	int config_options_id ;
	struct ASModuleSyntax *subsyntaxes ;
}ASModuleSyntax;

typedef struct ASModuleSpecs
{
	const char *module_class ;
	ASModuleSyntax *syntaxes ; 		
}ASModuleSpecs;

extern ASModuleSpecs ModulesSpecs[];

#endif /* ASCONFIG_H_HEADER_INCLUDED */
