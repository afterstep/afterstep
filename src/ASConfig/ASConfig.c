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
	ASProp_String,
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
	REG_SPEC_KEYWORD(BaseOptions);

	REG_SPEC_KEYWORD(ColorScheme);
	REG_SPEC_KEYWORD(ColorSchemeOptions);

	REG_SPEC_KEYWORD(Look);
	REG_SPEC_KEYWORD(LookMyStyles);
	REG_SPEC_KEYWORD(LookMyFrames);
	REG_SPEC_KEYWORD(LookMyBackgrounds);
	REG_SPEC_KEYWORD(LookTitleButtons);
	REG_SPEC_KEYWORD(LookOptions);

	REG_SPEC_KEYWORD(Feel);
	REG_SPEC_KEYWORD(FeelOptions);
	REG_SPEC_KEYWORD(Pager);
	REG_SPEC_KEYWORD(PagerOptions);
	REG_SPEC_KEYWORD(Wharf);
	REG_SPEC_KEYWORD(WharfOptions);
	REG_SPEC_KEYWORD(WinList);
	REG_SPEC_KEYWORD(WinListOptions);

	REG_SPEC_KEYWORD(BaseFile);
	REG_SPEC_KEYWORD(ColorSchemeFile);
	REG_SPEC_KEYWORD(FeelFile);
	REG_SPEC_KEYWORD(AutoExecFile);
	REG_SPEC_KEYWORD(WorkspaceFile);
	REG_SPEC_KEYWORD(StartMenuDir);
	REG_SPEC_KEYWORD(LookFile);
	REG_SPEC_KEYWORD(ThemeOverrideFile);
	REG_SPEC_KEYWORD(DatabaseFile);
	REG_SPEC_KEYWORD(PagerFile);
	REG_SPEC_KEYWORD(WharfFile);
	REG_SPEC_KEYWORD(WinListFile);

	REG_SPEC_KEYWORD(LookFiles);
	REG_SPEC_KEYWORD(FeelFiles);
	REG_SPEC_KEYWORD(FunctionsFiles);
	REG_SPEC_KEYWORD(PopupsFiles);
	REG_SPEC_KEYWORD(PagerFiles);
	REG_SPEC_KEYWORD(WharfFiles);
	REG_SPEC_KEYWORD(WinListFiles);
	REG_SPEC_KEYWORD(BackgroundFiles);

	REG_SPEC_KEYWORD(PrivateFiles);
	REG_SPEC_KEYWORD(SharedFiles);

	REG_SPEC_KEYWORD(flags);
	REG_SPEC_KEYWORD(x);
	REG_SPEC_KEYWORD(y);
	REG_SPEC_KEYWORD(width);
	REG_SPEC_KEYWORD(height);

	REG_SPEC_KEYWORD(type);
	REG_SPEC_KEYWORD(tint);
	REG_SPEC_KEYWORD(pixmap);

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

ASProperty *
special_free_storage2property( FreeStorageElem *curr )
{
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
			text_id = store_data( NULL, curr->argv[1], strlen(curr->argv[1])+1, ASStorage_RLEDiffCompress|ASStorage_NotTileable, 0);
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
	 		case TT_FUNCTION : 	/* TODO */ break ;
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
					default:
					/* handled by special_ as complex datatype */ break ;
				}	 
				prop = create_property( curr->term->id, type, name, (curr->sub != NULL) );	 
				if( type == ASProp_Data ) 
				{
					if( item.data.string == NULL ) 
						prop->contents.data = 0 ;
					else
						prop->contents.data = store_data( NULL, item.data.string, strlen(item.data.string)+1, ASStorage_RLEDiffCompress|ASStorage_NotTileable, 0);
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

		curr = curr->next ;
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
load_current_config( int id, const char *filename, const char *myname, 
					 SyntaxDef *syntax, int files_id, int file_id, int options_id )
{
	ASProperty *config = create_property( id, ASProp_Phony, NULL, True );
	ASConfigFile *cf = NULL ;

	if( filename )
	{
		LOCAL_DEBUG_OUT("loading file \"%s\"", filename );
	  
		cf = load_config_file(NULL, filename, myname?myname:"afterstep", syntax );
	}
	
	if( cf ) 
	{
		ASProperty *files = NULL ;
		ASProperty *file, *opts ;

		if( files_id != 0 ) 
		{
			files = create_property( files_id, ASProp_Phony, NULL, True );
			files->contents.config_file = cf ;
			append_bidirelem( config->sub_props, files );			   
		}	 
		
		file = create_property( file_id, ASProp_File, NULL, True );
		file->contents.config_file = cf ;
		
		free_storage2property_list( cf->free_storage, file );
 		append_bidirelem( files?files->sub_props:config->sub_props, file );			   
			
		opts = create_property( options_id, ASProp_Phony, NULL, True );
		merge_property_list( file, opts );
		append_bidirelem( config->sub_props, opts );			   
	}	 
	return config;
}	 

/*************************************************************************/
ASProperty* load_Base();
ASProperty* load_ColorScheme();
ASProperty* load_Look();


void 
load_hierarchy()
{
	ASProperty *tmp ;
	
	Root = create_property( CONFIG_root_ID, ASProp_Phony, "", True );
		
	if( (tmp = load_Base()) != NULL ) 
		append_bidirelem( Root->sub_props, tmp );
	
	if( (tmp = load_ColorScheme()) != NULL ) 
		append_bidirelem( Root->sub_props, tmp );
	
	if( (tmp = load_Look()) != NULL ) 
		append_bidirelem( Root->sub_props, tmp );
}

/*************************************************************************/
ASProperty* 
load_Base()
{
	ASProperty *base = NULL ;
	char *filename = make_session_file(Session, BASE_FILE, False );

	if( filename )
	{
		LOCAL_DEBUG_OUT("loading file \"%s\"", filename );
	   	base = load_current_config( CONFIG_Base_ID, filename, NULL, &BaseSyntax, 0, CONFIG_BaseFile_ID, CONFIG_BaseOptions_ID);
	  
		free( filename );
	}
	
	return base;
}	 

ASProperty* 
load_ColorScheme()
{
	ASProperty *cs = NULL;
	const char *filename = get_session_file (Session, 0, F_CHANGE_COLORSCHEME, False);

	if( filename )
	{
		LOCAL_DEBUG_OUT("loading file \"%s\"", filename );
		cs = load_current_config( CONFIG_ColorScheme_ID, filename, NULL, &ColorSyntax, 0, CONFIG_ColorSchemeFile_ID, CONFIG_ColorSchemeOptions_ID);	  
	}

	return cs;
}	 

ASProperty* 
load_Look()
{
	ASProperty *look = NULL;
	const char *filename = get_session_file (Session, 0, F_CHANGE_LOOK, False);

	if( filename )
	{
		LOCAL_DEBUG_OUT("loading file \"%s\"", filename );
		look = load_current_config( CONFIG_Look_ID, filename, NULL, &LookSyntax, CONFIG_LookFiles_ID, CONFIG_LookFile_ID, CONFIG_LookOptions_ID);	  
	}

	return look;
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
		bytes_out = fetch_data(NULL, root->contents.data, &string[0], 0, 127, 0, &orig_bytes);
		/* LOCAL_DEBUG_OUT( "fetched %d bytes", bytes_out ); */
		if( bytes_out < orig_bytes ) 
			fprintf( stderr, "= %d of %d chars:", bytes_out, orig_bytes );	 
		else
			fprintf( stderr, "= ");
		fprintf( stderr, "\"%s\"", string );	
		fputc( ';', stderr);
	}else if( root->type == ASProp_File )    
		fprintf( stderr, " loaded from [%s]", root->contents.config_file->fullname );

	fputc( '\n', stderr);
	if( root->sub_props ) 
	{
	 	iterate_asbidirlist( root->sub_props, print_hierarchy_iter_func, (void*)(level+1), NULL, False );		  
	}
	
}	 
