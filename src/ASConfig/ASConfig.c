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
 *  Theme : 
 * 		Loaded file :  - as decided by Session
 * 		Available Files : 
 * 			User files : 
 * 				$(AFTER_DIR)/$(THEME_FILE_DIR)/theme.*
 * 			Shared Files : 
 * 				$(AFTER_SHAREDIR)/$(THEME_FILE_DIR)/theme.*	 ( editable by root only )
 * 			Selected default :   
 * 				$(AFTER_DIR)/$(AFTER_NONCF)/0_theme.#scr or :
 * 				$(AFTER_DIR)/$(AFTER_NONCF)/0_theme
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
 * Look == Colorscheme + Look + Theme + ThemeOverride + ModuleMyStyles
 * Feel == Feel + Theme + ThemeOverride
 * Functions == Feel + Theme + ThemeOverride + Autoexec + WorspaceState
 * Popups == Feel + Theme + ThemeOverride + startmenu
 * Database == Database
 * Pager == pager + Theme + ThemeOverride
 * Wharf == wharf + Theme + ThemeOverride
 * WinList == winlist + Theme + ThemeOverride
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
#include "../../libAfterConf/afterconf.h"


/*************************************************************************/
typedef struct ASProperty {
	int type ;
	int contents_type ;
	union {
		int 		 integer ;
		ASStorageID  data;			
		ASImage 	*im ;
		ARGB32		 argb ;
		ASGradient  *grad ;
	}contents;
	
	ASBiDirList *sub_props ;	   
}ASProperty;

typedef struct ASConfigFile {
	char *dirname ;
	char *filename ;
	char *fullname ;
	char *myname ;

	FreeStorageElem *free_storage ;

}ASConfigFile;

typedef struct ASPropertyRoot {
	ASConfigFile *source ;
	ASProperty   *prop ;

	void  (*on_property_changed)(ASProperty *prop);
}ASPropertyRoot;

/*************************************************************************/
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

int
main (int argc, char **argv)
{
	/* Save our program name - for error messages */
    InitMyApp (CLASS_ASCONFIG, argc, argv, NULL, NULL, 0 );
	InitSession();

	
	if( dpy )   
    	XCloseDisplay (dpy);
    return 0;
}
/**************************************************************************/
void
load_file()
{
	char *configfile = make_session_file(Session, BASE_FILE, False );
	if( configfile != NULL )
	{
		FreeStorageElem *storage = file2free_storage(configfile, "afterstep", &BaseSyntax, NULL );

		if( storage != NULL )
			show_progress("BASE configuration loaded from \"%s\" ...", configfile);
		else
			show_progress("BASE could not be loaded from \"%s\" ...", configfile);
		if( configfile != Session->overriding_file )
			free( configfile );
	}else
        show_warning("BASE configuration file cannot be found");

	
	
}	 

