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

#include "ASConfig.h"
#include "configfile.h"
#include "asproperty.h"

/*************************************************************************/


ASProperty *Root = NULL;

ASConfigTypeInfo ConfigTypeInfo[] = 
{
   	{ CONFIG_Base_ID, 		 &BaseSyntax,  0, 					1, {CONFIG_BaseFile_ID}, 			CONFIG_BaseOptions_ID},
	{ CONFIG_ColorScheme_ID, &ColorSyntax, 0, 					1, {CONFIG_ColorSchemeFile_ID}, 	CONFIG_ColorSchemeOptions_ID},	  
	{ CONFIG_Functions_ID, 	 &FunctionSyntax, CONFIG_FunctionsFiles_ID,  3, {CONFIG_FeelFile_ID,
																			 CONFIG_AfterStepFile_ID,
																			 CONFIG_AutoExecFile_ID}, 0},	 
	{ CONFIG_Popups_ID, 	 &PopupSyntax, CONFIG_PopupsFiles_ID,  3, {CONFIG_FeelFile_ID,
																	   CONFIG_AfterStepFile_ID,
																	   CONFIG_StartDir_ID}, 0},	 
	{ CONFIG_Database_ID, 	 &DatabaseSyntax, 0, 				   1, {CONFIG_DatabaseFile_ID}, 0},	  
  	{0}	 
};

int AfterStepFeelFiles[]	= {CONFIG_FeelFile_ID, CONFIG_AfterStepFile_ID, -1} ;
int AfterStepLookFiles[]	= {CONFIG_LookFile_ID, CONFIG_AfterStepFile_ID, -1} ;
int PagerFeelFiles[] 		= {CONFIG_FeelFile_ID, CONFIG_PagerFile_ID, -1} ;
int PagerLookFiles[] 		= {CONFIG_LookFile_ID, CONFIG_PagerFile_ID, -1} ;
int PagerPrivateFiles[] 	= {CONFIG_PagerFile_ID, -1} ;

ASModuleSyntax MyStylesSyntaxes[] ={{CONFIG_MyStyles_ID,  &ModuleMyStyleSyntax,  CONFIG_LookFiles_ID, PagerLookFiles, 0, NULL },
									{-1, NULL, 0, NULL, 0, NULL}};

ASModuleSyntax AfterStepLookSyntaxes[] ={{CONFIG_MyStyles_ID,  		&ModuleMyStyleSyntax,  CONFIG_LookFiles_ID, AfterStepLookFiles, 0, NULL },
										 {CONFIG_MyFrames_ID,  		&AfterStepMyFrameSyntax,  CONFIG_LookFiles_ID, AfterStepLookFiles, 0, NULL },
										 {CONFIG_MyBackgrounds_ID,	&AfterStepMyBackSyntax,  CONFIG_LookFiles_ID, AfterStepLookFiles, 0, NULL },
										 {CONFIG_TitleButtons_ID,	&AfterStepTitleButtonSyntax,  CONFIG_LookFiles_ID, AfterStepLookFiles, 0, NULL },
										 {-1, NULL, 0, NULL, 0, NULL}};

ASModuleSyntax AfterStepFeelSyntaxes[] ={{CONFIG_Cursors_ID,  		&AfterStepCursorSyntax,  CONFIG_FeelFiles_ID, AfterStepFeelFiles, 0, NULL },
										 {CONFIG_MouseBindings_ID,	&AfterStepMouseSyntax,  CONFIG_FeelFiles_ID, AfterStepFeelFiles, 0, NULL },
										 {CONFIG_KeyBindings_ID,	&AfterStepKeySyntax,  CONFIG_FeelFiles_ID, AfterStepFeelFiles, 0, NULL },
										 {CONFIG_WindowBoxes_ID,	&AfterStepWindowBoxSyntax,  CONFIG_FeelFiles_ID, AfterStepFeelFiles, 0, NULL },
										 {-1, NULL, 0, NULL, 0, NULL}};

ASModuleSyntax AfterStepSyntaxes[]={{CONFIG_AfterStepFeel_ID, &AfterStepFeelSyntax, CONFIG_FeelFiles_ID, AfterStepFeelFiles, CONFIG_FeelOptions_ID, &AfterStepFeelSyntaxes[0] },
									{CONFIG_AfterStepLook_ID, &AfterStepLookSyntax, CONFIG_LookFiles_ID, AfterStepLookFiles, CONFIG_LookOptions_ID, &AfterStepLookSyntaxes[0] },
									{-1, NULL, 0, NULL, 0, NULL}};


ASModuleSyntax PagerSyntaxes[] = {	{CONFIG_PagerFeel_ID, &PagerFeelSyntax, CONFIG_FeelFiles_ID, PagerFeelFiles, CONFIG_FeelOptions_ID, NULL },
									{CONFIG_PagerLook_ID, &PagerLookSyntax, CONFIG_LookFiles_ID, PagerLookFiles, CONFIG_LookOptions_ID, &MyStylesSyntaxes[0] },
									{0, 	  			  &PagerPrivateSyntax,                0, PagerPrivateFiles, CONFIG_PagerOptions_ID, NULL },									
									{-1, NULL, 0, NULL, 0, NULL}};

ASModuleSpecs ModulesSpecs[] = 
	{{CLASS_PAGER, PagerSyntaxes},
	 {CLASS_AFTERSTEP, AfterStepSyntaxes},
	 {NULL, NULL}
	};

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
                     	
	REG_SPEC_KEYWORD(AfterStepLook);
	REG_SPEC_KEYWORD(AfterStepFeel);
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
	REG_SPEC_KEYWORD(AfterStepFile);
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
	REG_SPEC_KEYWORD(Cursors);
	REG_SPEC_KEYWORD(MouseBindings);
	REG_SPEC_KEYWORD(KeyBindings);
	REG_SPEC_KEYWORD(WindowBoxes);

	REG_SPEC_KEYWORD(LookOptions);
	REG_SPEC_KEYWORD(FeelOptions);
	REG_SPEC_KEYWORD(AfterStepOptions);
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
	REG_SPEC_KEYWORD(IncludeFile);

	REG_SPEC_KEYWORD(unpressed);
	REG_SPEC_KEYWORD(pressed);

	REG_SPEC_KEYWORD(source);
	REG_SPEC_KEYWORD(context);
	REG_SPEC_KEYWORD(mod);

	REG_SPEC_KEYWORD(left);
	REG_SPEC_KEYWORD(right);
	REG_SPEC_KEYWORD(top);
	REG_SPEC_KEYWORD(bottom);

	REG_SPEC_KEYWORD(image);
	REG_SPEC_KEYWORD(mask);
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

	if( curr->term->id == MYSTYLE_BACKGRADIENT_ID ) 
	{
				/* TODO */				   
	}else if( curr->term->id == MYSTYLE_BACKGRADIENT_ID ) 
	{
				/* TODO */				   
	}else if( curr->term->id == MYSTYLE_BACKPIXMAP_ID )
	{
		prop = create_property( curr->term->id, ASProp_Phony, NULL, True );	 				   
		add_integer_property( CONFIG_type_ID, item.data.integer, prop );

		if (curr->argc > 1)
		{	
			if( item.data.integer == TEXTURE_TRANSPARENT || item.data.integer == TEXTURE_TRANSPARENT_TWOWAY ) 
		  		add_string_property( CONFIG_tint_ID, curr->argv[1], prop );				   
			else
		  		add_string_property( CONFIG_pixmap_ID, curr->argv[1], prop );				   
		}
	}

	if( prop == NULL ) 
	{	
		switch( curr->term->type ) 
		{	
			case TT_GEOMETRY :
				prop = create_property( curr->term->id, ASProp_Phony, NULL, True );
				add_integer_property( CONFIG_flags_ID, item.data.geometry.flags, prop );
				if( get_flags(item.data.geometry.flags, XValue ) )
					add_integer_property( CONFIG_x_ID, item.data.geometry.x, prop );
				if( get_flags(item.data.geometry.flags, YValue ) )
					add_integer_property( CONFIG_y_ID, item.data.geometry.y, prop );
				if( get_flags(item.data.geometry.flags, WidthValue ) )
					add_integer_property( CONFIG_width_ID, item.data.geometry.width, prop );
				if( get_flags(item.data.geometry.flags, HeightValue ) )
					add_integer_property( CONFIG_height_ID, item.data.geometry.height, prop );
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
						add_char_property( CONFIG_hotkey_ID, pfunc->hotkey, prop );				
					if( pfunc->text )
					{	
						add_string_property( CONFIG_text_ID, pfunc->text, prop );				   
					
						if(
							(pfunc->func > F_REFRESH && pfunc->func <= F_MOVECURSOR ) ||
							(pfunc->func >= F_DESK && pfunc->func < F_POPUP ) ||
						  	pfunc->func == F_RESIZE || pfunc->func == F_MOVE ||
						  	pfunc->func == F_SETLAYER || pfunc->func == F_REFRESH ||
						  	pfunc->func == F_MAXIMIZE || pfunc->func == F_CHANGE_WINDOWS_DESK )
						{
							tmp = add_integer_property( CONFIG_value_ID, pfunc->func_val[0], prop );
							set_property_index( tmp, 0 );
							if( pfunc->unit[0] != '\0' )
							{	
								tmp = add_char_property( CONFIG_unit_ID, pfunc->unit[0], prop );				
								set_property_index( tmp, 0 );
							}
							tmp = add_integer_property( CONFIG_value_ID, pfunc->func_val[1], prop );
							set_property_index( tmp, 1 );
							if( pfunc->unit[1] != '\0' )
							{	
								tmp = add_char_property( CONFIG_unit_ID, pfunc->unit[1], prop );				
								set_property_index( tmp, 1 );
							}
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
								add_string_property( F_MINIPIXMAP, pfunc->name, prop );				   							   
							else
								add_string_property( F_MINIPIXMAP, pfunc->text, prop );				   							   
							*pcurr = curr ;
						}	 
					}	 
				}
				break ;
	 		case TT_BOX :
				prop = create_property( curr->term->id, ASProp_Phony, NULL, True );
				add_integer_property( CONFIG_flags_ID, item.data.box.flags, prop );
				if( get_flags(item.data.box.flags, LeftValue ) )
					add_integer_property( CONFIG_left_ID, item.data.box.left, prop );
				if( get_flags(item.data.box.flags, RightValue ) )
					add_integer_property( CONFIG_right_ID, item.data.box.right, prop );
				if( get_flags(item.data.box.flags, TopValue ) )
					add_integer_property( CONFIG_top_ID, item.data.box.top, prop );
				if( get_flags(item.data.box.flags, BottomValue ) )
					add_integer_property( CONFIG_bottom_ID, item.data.box.bottom, prop );
				break ;
	 		case TT_BUTTON :	
				prop = create_property( curr->term->id, ASProp_Phony, NULL, True );
				set_property_index( prop, item.index );
				if( item.data.button )
				{	
					if( item.data.button->shapes[ASB_State_Up] )
						add_string_property( CONFIG_unpressed_ID, item.data.button->shapes[ASB_State_Up], prop );				   
					if( item.data.button->shapes[ASB_State_Down] )
						add_string_property( CONFIG_pressed_ID, item.data.button->shapes[ASB_State_Down], prop );				   
				}
				break ;
	 		case TT_BINDING : 	
				prop = create_property( curr->term->id, ASProp_Phony, NULL, True );
				if( item.data.binding.sym )
					add_string_property( CONFIG_source_ID, item.data.binding.sym, prop );				   
				add_integer_property( CONFIG_context_ID, item.data.binding.context, prop );
				add_integer_property( CONFIG_mod_ID, item.data.binding.mods, prop );
				break ;
	 		case TT_INTARRAY : 	/* TODO */ break ;
	 		case TT_CURSOR :
				prop = create_property( curr->term->id, ASProp_Phony, NULL, True );
				if( item.data.cursor )
				{	
					if( item.data.cursor->image_file )
						add_string_property( CONFIG_image_ID, item.data.cursor->image_file, prop );				   
					if( item.data.cursor->mask_file )
						add_string_property( CONFIG_mask_ID, item.data.cursor->mask_file, prop );				   
				}
				break ;
				
		}
	}
	ReadConfigItem (&item, NULL);

	return prop;
}

ASProperty* 
free_storage2property_list( FreeStorageElem *fs, ASProperty *pl )
{
	FreeStorageElem *curr ;
	ConfigItem    item;
	ASProperty *prop = NULL;
	
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
				if( curr->sub == NULL ) 
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
			set_property_index( prop, item.index );

		if( curr->sub != NULL )
			free_storage2property_list( curr->sub, prop ) ;

		append_property( pl, prop );			   
	}		
	return prop;    
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
	dup_property_contents( src, dst, True ); 
		   
	set_flags( src->flags, ASProp_Merged );		
	return True;	   
}

Bool
merge_prop_into_list(void *data, void *aux_data)
{
	ASProperty *prop = (ASProperty*)data;
	ASProperty *dst_prop = (ASProperty*)aux_data;

	clear_flags( prop->flags, ASProp_Merged );
	iterate_asbidirlist( dst_prop->sub_props, merge_prop_into_prop, prop, NULL, False );		  	 	
	if( !get_flags( prop->flags, ASProp_Merged )  )
	{
		prop = dup_property( prop, True );	
		append_property( dst_prop, prop );			   	
	}	 

	return True;	
}

void 
merge_property_list( ASProperty *src, ASProperty *dst )
{
	if( src->sub_props == NULL && dst->sub_props == NULL ) 
		return;
	LOCAL_DEBUG_CALLER_OUT("(%p,%p)", src, dst );	
	iterate_asbidirlist( src->sub_props, merge_prop_into_list, dst, NULL, False );		  	
}
/*************************************************************************/
ASProperty* asmenu_dir2property( const char *dirname, const char *menu_path, ASProperty *owner_prop, int func, const char *extension, const char *mini_ext );
void melt_menu_props( ASProperty *file, ASProperty *opts );
void melt_func_props( ASProperty *file, ASProperty *opts );
void melt_binding_props( ASProperty *src, ASProperty *dst );


ASProperty* 
load_current_config_fname( ASProperty* config, int id, const char *filename, const char *myname, 
					 SyntaxDef *syntax, int files_id, int file_id, int options_id )
{
	ASConfigFile *cf = NULL ;
	ASProperty *files = NULL ;
	ASProperty *file ;
	ASProperty *opts ;

	if( config == NULL ) 
		config = create_property( id, ASProp_Phony, NULL, True );

	if( filename )
	{
		if( CheckDir(filename) )
		{	
			LOCAL_DEBUG_OUT("loading file \"%s\"", filename );
			cf = load_config_file(NULL, filename, myname?myname:"afterstep", syntax );
		}
	}
	

	if( files_id != 0 ) 
	{
		files = find_property_by_id( config, files_id );
		if( files == NULL )
		{
			files = create_property( files_id, ASProp_Phony, NULL, True );
			append_property( config, files );			   
		}
	}	 
	
	file = find_property_by_id( files?files:config, file_id );	   
	if( file == NULL ) 
	{	
		file = create_property( file_id, ASProp_File, NULL, True );
		append_property( files?files:config, file );			   
	}
	file->contents.config_file = cf ;
	
	if( cf ) 
		free_storage2property_list( cf->free_storage, file );
	else if( file_id == CONFIG_StartDir_ID ) 
		asmenu_dir2property( filename, "", file, F_NOP, NULL, NULL );	

	if( options_id != 0 )
	{	
		opts = find_property_by_id( config, options_id );
		if( opts == NULL )		   
		{	
			opts = create_property( options_id, ASProp_Phony, NULL, True );
			append_property( config, opts );			   
		}
	}else
		opts = config ;

	if( file_id == CONFIG_StartDir_ID )
		melt_menu_props( file, opts );
	else if( syntax == &FunctionSyntax ) 
	{
		melt_func_props( file, opts );	  
	}else if( id == CONFIG_KeyBindings_ID || id == CONFIG_MouseBindings_ID ) 
		melt_binding_props( file, opts );	  
	else
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
ASProperty* 
asmenu_dir2property( const char *dirname, const char *menu_path, ASProperty *owner_prop, 
					 int func, const char *extension, const char *mini_ext )
{
	struct direntry  **list;
	int list_len, i ;
	ASProperty *popup, *include_file = NULL ;
	const char *ptr;
	ASConfigFile *include_cf = NULL ;
	char *new_path ;		
	ASProperty *item = NULL ;
	int mini_ext_len = mini_ext?strlen(mini_ext):0 ;
	int ext_len = extension?strlen(extension):0 ; 

	if( dirname == NULL ) 
		return NULL;
	
	ptr = strrchr(dirname,'/');

	list_len = my_scandir ((char*)dirname, &list, no_dots_except_include, NULL);
	LOCAL_DEBUG_OUT("dir \"%s\" has %d entries", dirname, list_len );
	if( list_len <= 0 )
		return NULL;
	   
	if( ptr == NULL ) 
		ptr = dirname ;
	else
		++ptr ;
	while( isdigit(*ptr) ) ++ptr;
	if( *ptr == '_' )
		++ptr ;
	
	new_path = make_file_name( menu_path, ptr );
	popup = create_property( F_POPUP, ASProp_Data, new_path, True );
	popup->contents.data = encode_string( dirname );
	append_property( owner_prop, popup );			   

	for (i = 0; i < list_len; i++)
		if( list[i]->d_name[0] == '.' ) 
		{
			ASProperty *cmd ;
			include_cf = load_config_file(dirname, list[i]->d_name, "afterstep", &includeSyntax );
			include_file = create_property( CONFIG_IncludeFile_ID, ASProp_File, NULL, True );
			append_property( popup, include_file );			   
			include_file->contents.config_file = include_cf ;
			free_storage2property_list( include_cf->free_storage, include_file );

			cmd = find_property_by_id( include_file, INCLUDE_command_ID );	 

			if( cmd ) 
			{
				ASProperty *cmd_func = find_property_by_id( cmd, -1 );	 ;
				if( cmd_func && cmd_func->id < F_FUNCTIONS_NUM )	
					func = cmd_func->id ;
			}	 
			break;	
		}
	for (i = 0; i < list_len; i++)
	{	
		char *sub_path = make_file_name( dirname, list[i]->d_name );
		if ( S_ISDIR (list[i]->d_mode) )
		{	
			ASProperty *sub_menu = asmenu_dir2property( sub_path, new_path, owner_prop, func, extension, mini_ext ) ;
			item = create_property( F_POPUP, ASProp_Data, sub_menu->name, True );
			item->contents.data = encode_string( sub_path );
			append_property( popup, item );			   
		}else if( list[i]->d_name[0] != '.' ) 
		{
			int len = strlen(list[i]->d_name);
			char *stripped_name = NULL;
			char *minipixmap = NULL ;
			char hotkey = '\0' ;
			int k ; 
			char *clean_name;
			int order = strtol (list[i]->d_name, &clean_name, 10);
			
			if( clean_name == list[i]->d_name )
			{	
				if( isalpha(clean_name[0]) && clean_name[1] == '_' )
				{	
					order = (int)(clean_name[0]) - (int)'0' ;
					clean_name += 2 ;
					len -= 2 ;
				}else
					order = -1 ;
			}else
				len -= (clean_name - list[i]->d_name );

			if( ext_len > 0 && len > ext_len )
			{	
				if( strcmp( clean_name + len - ext_len, extension) == 0 ) 
					stripped_name = mystrndup( clean_name, len - ext_len );
				else if( strncmp(clean_name, extension, ext_len) == 0 ) 
					stripped_name = mystrdup( clean_name + ext_len );
			}
			if( stripped_name == NULL )
			{	
				if( mini_ext_len > 0 && len > mini_ext_len )
				{ 
					if( strcmp( clean_name + len - mini_ext_len, mini_ext ) == 0 ) 
						continue;				
					if( strncmp( clean_name, mini_ext, mini_ext_len ) == 0 ) 
						continue;				
				}
				stripped_name = mystrdup( clean_name );
			}
			
			if( mini_ext_len > 0 )
			{
				for(k = 0; k < list_len; k++)
					if( k != i && !S_ISDIR (list[k]->d_mode)) 
					{
						int mlen = strlen(list[k]->d_name);	  
						if( mlen > mini_ext_len && len - ext_len == mlen - mini_ext_len)
						{	 
							if( strcmp( list[k]->d_name + mlen - mini_ext_len, mini_ext ) == 0) 
							{
								if( strncmp( list[k]->d_name, stripped_name, mlen - mini_ext_len ) == 0 )
								{  /* found suitable minipixmap */
									minipixmap = make_file_name( dirname, list[k]->d_name );
									break;	
								}
							}else if( strncmp( list[k]->d_name, mini_ext, mini_ext_len ) == 0) 
							{
								if( strcmp( list[k]->d_name + mini_ext_len, stripped_name ) == 0 )
								{  /* found suitable minipixmap */
									minipixmap = make_file_name( dirname, list[k]->d_name + mini_ext_len );
									break;	
								}
							}
						}	  	   
					}
			}	

			for( k = 0 ; stripped_name[k] != '\0' ; ++k ) 
			{	
				if( stripped_name[k] == '_' ) 
				{
					if( stripped_name[k+1] == '_' )	  
					{
						int l ;
						for( l = k+2 ; stripped_name[l] != '\0' ; ++l)
							stripped_name[l-2] = stripped_name[l] ;
						hotkey = stripped_name[k] ;	
					}else	 
						stripped_name[k] = ' ' ;
				}

			}						 

			if( func != F_NOP ) 
			{
				item = create_property( func, ASProp_Data, stripped_name, True );
				item->contents.data = encode_string( sub_path );
				append_property( popup, item );
				add_string_property( CONFIG_text_ID, sub_path, item );				   
				if( minipixmap )
					add_string_property( F_MINIPIXMAP, minipixmap, item );				   							   
				if( hotkey != '\0' )
					add_char_property( CONFIG_hotkey_ID, hotkey, item );				   
			}else
			{	
				ASConfigFile *cf = NULL ;
			
				cf = load_config_file(dirname, list[i]->d_name, "afterstep", &FuncSyntax );
				item = free_storage2property_list( cf->free_storage, popup );				
				if( item )
				{	
					item->type = ASProp_File ;
					item->contents.config_file = cf ;
					if( item->name == NULL )
					{	
						item->name = stripped_name ;
						stripped_name = NULL ;
					}else
						hotkey = scan_for_hotkey (stripped_name);
				
					if( minipixmap )
						if( find_property_by_id( item, F_MINIPIXMAP ) == NULL ) 	 
							add_string_property( F_MINIPIXMAP, minipixmap, item );				   							   
					if( hotkey != '\0' )
						if( find_property_by_id( item, CONFIG_hotkey_ID ) == NULL ) 	 
			   				add_char_property( CONFIG_hotkey_ID, hotkey, item );				   

				}else
					destroy_config_file( cf ); 
			}
			if( item ) 
				item->order = order ;
			if( stripped_name)
				free( stripped_name );
			if(minipixmap)
				free(minipixmap);
		}
		free( sub_path );
		free (list[i]);
	}
	free( list );
	free( new_path );
	return popup;
}

int compare_menuitems_handler(void *data1, void *data2)
{
	ASProperty *item1 = (ASProperty*)data1 ; 	
	ASProperty *item2 = (ASProperty*)data2 ; 
	static char buffer1[512], buffer2[512] ;
	int stored_len1 = 0, stored_len2 = 0 ;
	LOCAL_DEBUG_OUT( "comparing items with order %d and %d", item1->order, item2->order ); 
	if( item2->id == F_POPUP && item1->id != F_POPUP) 
		return 1 ;
	else if( item1->id == F_POPUP && item2->id != F_POPUP) 
		return 0;
	if( item2->order >= 0 )
	{
		if( item1->order  < 0 )
			return 1;
		return item1->order - item2->order;
	}
	if( item1->order >= 0 )
		return 0;

	if( item1->type == ASProp_File && item2->type == ASProp_File )
		return mystrcmp( item1->contents.config_file->filename, item2->contents.config_file->filename );
	else if( item1->type == ASProp_File && item2->type == ASProp_Data )
	{
	 	decode_string( item2->contents.data, &(buffer1[0]), 512, &stored_len1 );
		return mystrcmp( item1->contents.config_file->filename, &buffer1[0] );
	}else if( item2->type == ASProp_File && item1->type == ASProp_Data )
	{
	 	decode_string( item1->contents.data, &(buffer1[0]), 512, &stored_len1 );
		return mystrcmp( &buffer1[0], item2->contents.config_file->filename );
	}else if( item2->type == ASProp_Data && item1->type == ASProp_Data )
	{
		decode_string( item1->contents.data, &(buffer1[0]), 512, &stored_len1 );
	 	decode_string( item2->contents.data, &(buffer2[0]), 512, &stored_len2 );
		return mystrcmp( &buffer1[0], &buffer2[0] );
	}	 
	return 0; 
}	 

Bool
melt_menu_props_into_list(void *data, void *aux_data)
{
	ASProperty *popup = (ASProperty*)data;
	ASProperty *dst = (ASProperty*)aux_data ;
	ASProperty *incl ;
	ASProperty *dst_popup ;
	char *name = NULL ;

	incl = find_property_by_id( popup, CONFIG_IncludeFile_ID );	 
	dst_popup = find_property_by_id_name( dst, popup->id, popup->name );
	if( dst_popup == NULL ) 
	{	
		dst_popup = dup_property( popup, True ); 
		append_property( dst, dst_popup );
	}else
 		merge_property_list( popup, dst_popup );		
	
	if( incl )
	{	
		int func = F_NOP ;
#define MAX_EXTENTION 256
		char *extension = NULL ;
		char *mini_ext = NULL ;
		ASBiDirElem *curr ; 		   
		ASProperty *phony = NULL;

		remove_property_by_id( dst_popup, CONFIG_IncludeFile_ID );	 
		/* Pass 1 : collecting settings first : */
		for( curr = LIST_START(incl->sub_props); curr != NULL ; LIST_GOTO_NEXT(curr) )
		{
			ASProperty *prop = (ASProperty*)LISTELEM_DATA(curr) ;	  
			switch( prop->id )
			{
				case INCLUDE_keepname_ID           : /* OBSOLETE */   break ;
				case INCLUDE_extension_ID          : 
					if( extension )	free( extension ); 
					extension = decode_string_alloc( prop->contents.data );
					break ;
				case INCLUDE_miniextension_ID      :
					if( mini_ext )	free( mini_ext ); 
					mini_ext = decode_string_alloc( prop->contents.data );
					break ;
						
				case INCLUDE_minipixmap_ID         : /* OBSOLETE */   break ;
				case INCLUDE_command_ID        	   : /* TODO */   break ;
				case INCLUDE_order_ID       	   :    
					dst_popup->order = prop->contents.integer ; break ;
				case INCLUDE_RecentSubmenuItems_ID : /* TODO */   break ;
				case INCLUDE_name_ID          	   : 
					if( name )	free( name ); 
					name = decode_string_alloc( prop->contents.data );
					break ;
			}
		}
		/* Pass 2 : including submenus : */
		for( curr = LIST_START(incl->sub_props); curr != NULL ; LIST_GOTO_NEXT(curr) )
		{	
			ASProperty *prop = (ASProperty*)LISTELEM_DATA(curr) ;	  
			if( prop->id == INCLUDE_include_ID )
			{
				ASProperty *text_prop = find_property_by_id( prop, CONFIG_text_ID );	 
				char *fullfilename = PutHome( prop->name );
				int incl_func = func ;
				ASProperty *sub_popup ;

				LOCAL_DEBUG_OUT( "include \"%s\" with text_prop = %p", fullfilename, text_prop );
				
				if( text_prop != NULL ) 
				{	  
					char *txt = decode_string_alloc( text_prop->contents.data );
					if( txt )
					{	
						TermDef *fd  ;
						if( (fd = txt2fterm( txt, False)) != NULL )
							incl_func = fd->id ;
						free( txt );
					}
				}
				if( phony == NULL ) 		
					phony = create_property( CONFIG_StartDir_ID, ASProp_File, NULL, True );
				LOCAL_DEBUG_OUT( "func = (%d) phony = %p, popup = %p", incl_func, phony, popup );
				LOCAL_DEBUG_OUT( "popup->name = \"%s\"", popup->name );
				sub_popup = asmenu_dir2property( fullfilename, popup->name, phony, 
					 				 incl_func, extension, mini_ext );
				
				LOCAL_DEBUG_OUT( "sub_popup = %p", sub_popup );
				print_hierarchy( sub_popup, 0 );
				if( sub_popup != NULL )
				{	 
					merge_property_list( sub_popup, dst_popup );
					discard_bidirelem( phony->sub_props, sub_popup );
					if( phony->sub_props->count > 0 )
					{	
						melt_menu_props( phony, dst );
						purge_asbidirlist( phony->sub_props );
					}
				}
				free( fullfilename );
			}	 
		}
		if( phony != NULL ) 		   
			destroy_property( phony );
		if( extension )
			free( extension );
		if( mini_ext )
			free( mini_ext );
	}
	/* now we need to sort items according to order */
	bubblesort_asbidirlist( dst_popup->sub_props, compare_menuitems_handler );
	LOCAL_DEBUG_OUT( "sorted menu \"%s\"", dst_popup->name );
	print_hierarchy( dst_popup, 0 );
	/* and now we need to add F_TITLE item for menu name */
	if( name == NULL ) 
	{	
		int i = -1; 
		parse_file_name( dst_popup->name, NULL, &name );
		if( name )
			while( name[++i] ) if( name[i] == '_' ) name[i] = ' ' ;
	}
	if( name ) 
	{
		ASProperty *title ; 
		title = create_property( F_TITLE, ASProp_Phony, name, True );
		prepend_property( dst_popup, title  );
		free( name );
	}	 

	return True;
}

void 
melt_menu_props( ASProperty *src, ASProperty *dst )
{
	if( src->sub_props == NULL || dst == NULL ) 
		return;
	LOCAL_DEBUG_CALLER_OUT("(%p,%p)", src, dst );	
	iterate_asbidirlist( src->sub_props, melt_menu_props_into_list, dst, NULL, False );		  	
}

/*************************************************************************/
Bool
melt_func_props_into_list(void *data, void *aux_data)
{
	ASProperty *func = (ASProperty*)data;
	ASProperty *dst = (ASProperty*)aux_data ;
	ASProperty *dst_func ;
	int index = 0 ;
	ASBiDirElem *curr = LIST_START(func->sub_props); 		   


	dst_func = find_property_by_id_name( dst, func->id, func->name );

	if( dst_func == NULL ) 
	{	
		dst_func = dup_property( func, False ); 
		append_property( dst, dst_func );
	}

	while( curr ) 
	{
		ASProperty *prop = (ASProperty*)LISTELEM_DATA(curr) ;	  
		ASProperty *copy ;
		copy = dup_property( prop, True ); 
		set_property_index( copy, index );
		append_property( dst_func, copy );
		
		++index ;
		
		LIST_GOTO_NEXT(curr);
	}
	return True;
}

void 
melt_func_props( ASProperty *src, ASProperty *dst )
{
	if( src->sub_props == NULL || dst == NULL ) 
		return;
	LOCAL_DEBUG_CALLER_OUT("(%p,%p)", src, dst );	
	iterate_asbidirlist( src->sub_props, melt_func_props_into_list, dst, NULL, False );		  	
}
/*************************************************************************/
void 
melt_binding_props( ASProperty *src, ASProperty *dst )
{
	LOCAL_DEBUG_CALLER_OUT("(%p,%p)", src, dst );	  
	if( src->sub_props != NULL && dst != NULL ) 
	{	
		int index = 0 ;
		ASBiDirElem *curr = LIST_START(src->sub_props); 		   
		while( curr ) 
		{
			ASProperty *prop = (ASProperty*)LISTELEM_DATA(curr) ;	  
			ASProperty *copy ;
			copy = dup_property( prop, True ); 
			set_property_index( copy, index );
			append_property( dst, copy );
		
			++index ;
		
			LIST_GOTO_NEXT(curr);
		}
	}
}
/*************************************************************************/
void load_global_configs();
void add_module_config( const char *module_class, const char *module_name );


void 
load_hierarchy()
{
	Root = create_property( CONFIG_root_ID, ASProp_Phony, "", True );
		
	load_global_configs();
	add_module_config( CLASS_AFTERSTEP, "afterstep" );
	add_module_config( CLASS_PAGER, "Pager" );
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
			append_property( Root, tmp );
	}
}	 

void handle_module_syntaxes( ASProperty *owner, const char *module_name, ASModuleSyntax *syntaxes )
{	
	int k ;
	ASProperty *tmp ;
	for( k = 0 ; syntaxes[k].config_id >= 0 ; ++k )
	{	
		ASModuleSyntax *ms = &(syntaxes[k]);
		int l ;

		if( ms->config_id == 0 ) 
		{
			load_current_config( 	owner,
									ms->config_id, module_name, 
									ms->syntax, 
									0, 
									ms->files[0], 
									ms->config_options_id);				 		
		}else
		{		 
			tmp = NULL ;
			for( l = 0 ; ms->files[l] >= 0 ; ++l )
			{
				tmp = load_current_config( 	tmp,
											ms->config_id, module_name, 
											ms->syntax, 
											ms->config_files_id, 
											ms->files[l], 
											ms->config_options_id);
			}
			if( tmp ) 
				append_property( owner, tmp );
					
			if( ms->subsyntaxes ) 
				handle_module_syntaxes( tmp, module_name, ms->subsyntaxes );
		}
	}
}

void
add_module_config( const char *module_class, const char *module_name )
{
	int i ;
	ASProperty *config;

	for( i = 0 ; ModulesSpecs[i].module_class != NULL ; ++i )
		if( mystrcmp( ModulesSpecs[i].module_class, module_class ) == 0 )
		{	
			config = create_property( CONFIG_Module_ID, ASProp_Phony, module_name, True );
			append_property( Root, config );
			
			handle_module_syntaxes( config, module_name, ModulesSpecs[i].syntaxes );
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
	if( root->order >= 0 ) 
		fprintf( stderr, "order=%d ", root->order );
	if( get_flags( root->flags, ASProp_Indexed ) ) 
	   	fprintf( stderr, "[%d] ", root->index );
	else if( root->name )
		fprintf( stderr, "\"%s\" ", root->name );
	
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
			fprintf( stderr, "loaded from [%s]", root->contents.config_file->fullname );
		else
			fprintf( stderr, "not loaded" );
	}

	fputc( '\n', stderr);
	if( root->sub_props ) 
	{
	 	iterate_asbidirlist( root->sub_props, print_hierarchy_iter_func, (void*)(level+1), NULL, False );		  
	}
	
}	 
