/*
 * Copyright (c) 2005 Sasha Vasko <sasha at aftercode.net>
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

#include <unistd.h>

#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/session.h"
#include "../libAfterStep/desktop_category.h"

#include "afterconf.h"

const char *default_aliases[][2] = 
{
	{"ArcadeGame", "Arcade"},
	{"Application", "Applications"},
	{"Game", "Games"},
	{"Utility", "Utilities"},
	{NULL, NULL}	  

};	 
	

/*************************************************************************/
/* private stuff : 													 */
/*************************************************************************/

static char **
parse_category_list( char *list, int *pnum_return ) 
{
	char **shortcuts = NULL; 
	int num = 0;
	if( list ) 
	{
		int i ;
		for( i = 0 ; list[i] ; ++i ) 
			if( list[i] == ';' ) 
				++num ;
		if( i > 0 && list[i-1] != ';' ) 
			++num ;
		if( num > 0 ) 
		{
			int sc_i = 0 ;
			shortcuts = safecalloc( num, sizeof(char*));	

			shortcuts[sc_i++] = &list[0] ; 
			for( i = 0 ; list[i] ; ++i ) 
				if( list[i] == ';' ) 
				{
					list[i++] = '\0' ; 
					if( list[i] && sc_i < num ) 
						shortcuts[sc_i++] = &list[i] ;							
				}
		}	 
	}	 
	if( pnum_return ) 
		*pnum_return = num ;
	return shortcuts;			   
}	 

static char *
filter_desktop_entry_exec( const char *exec )
{
	char *clean_exec = mystrdup(exec); 	
	int start = 0, ts;

	while( clean_exec[start] != '\0' && !isspace(clean_exec[start]) ) ++start;
	while( clean_exec[start] != '\0')
	{
		ts = start ; 	  
		while( isspace(clean_exec[ts]) ) ++ts;
		if( clean_exec[ts] == '%' && isalpha(clean_exec[ts+1]) ) 
		{	
			while( !isspace(clean_exec[ts]) && clean_exec[ts] != '\0' ) 
			{
				clean_exec[ts] = ' ' ;
				++ts ;
			}	 
			start = ts ; 
		}else if( mystrncasecmp(&clean_exec[ts], "-caption ", 9 ) == 0 )
		{
			ts += 9 ;
			while( isspace(clean_exec[ts]) ) ++ts;
			if( mystrncasecmp(&clean_exec[ts], "\"%c\"", 4 ) == 0 )
			{
				ts += 3 ;	
				while( start < ts ) clean_exec[++start] = ' ';
			}else if( clean_exec[ts] == '%' && isalpha(clean_exec[ts+1]) )
			{
				ts += 1 ;	
				while( start < ts ) clean_exec[++start] = ' ';
			}
		}else if( clean_exec[ts] == '\"' )
		{
			char *end = find_doublequotes (&clean_exec[ts]);
			if( end != NULL ) 
				start = end - &clean_exec[0] ;
		}else
			for( start = ts ; !isspace(clean_exec[start]) && clean_exec[start] != '\0'	; ++start );
	}
	while( isspace(clean_exec[start-1]))
	{
		--start ;	
		clean_exec[start] = '\0' ;
	}	 
	return clean_exec;
}

static void 
parse_desktop_entry_line( ASDesktopEntry* de, char *ptr ) 
{
#define PARSE_ASDE_TYPE_VAL(val)	if(mystrncasecmp(ptr, #val, ASDE_KEYWORD_##val##_LEN) == 0){de->type = ASDE_Type##val; return;}					   			   				   
#define PARSE_ASDE_STRING_VAL(val)	if(mystrncasecmp(ptr, #val "=", ASDE_KEYWORD_##val##_LEN+1) == 0){if( de->val ) free( de->val ) ; de->val = stripcpy(ptr+ASDE_KEYWORD_##val##_LEN+1); return;}					   
#define PARSE_ASDE_FLAG_VAL(val)	if(mystrncasecmp(ptr, #val "=", ASDE_KEYWORD_##val##_LEN+1) == 0){set_flags(de->flags, ASDE_##val); return;}					   			   

/*	LOCAL_DEBUG_OUT( "PARSING \"%s\"", ptr );	*/

	if( ptr[0] == 'X' && ptr[1] == '-') 
	{
		ptr +=2 ; 
		if( mystrncasecmp( ptr, "AfterStep-", 10 ) == 0 ) 
		{
			ptr += 10 ;
			PARSE_ASDE_STRING_VAL(IndexName)
			PARSE_ASDE_STRING_VAL(Aliases)
		}else if( mystrncasecmp( ptr, "KDE-", 4 ) == 0 ) 
			set_flags( de->flags, ASDE_KDE );
		else if( mystrncasecmp( ptr, "GNOME-", 6 ) == 0 ) 
			set_flags( de->flags, ASDE_GNOME );
	}else if( mystrncasecmp( ptr, "Name[", 5 ) == 0 ) 
	{
		/* TODO */
	}else if( mystrncasecmp( ptr, "Type=", 5 ) == 0 ) 
	{
		ptr += 5 ;

		PARSE_ASDE_TYPE_VAL(Application)
		PARSE_ASDE_TYPE_VAL(Link)
		PARSE_ASDE_TYPE_VAL(FSDevice)
		PARSE_ASDE_TYPE_VAL(Directory)
	}else if( mystrncasecmp( ptr, "Comment[", 8 ) == 0 ) 
	{
		/* TODO */
	}else if( mystrncasecmp( ptr, "Encoding=UTF-8", 14 ) == 0 ) 
	{	
		set_flags( de->flags, ASDE_EncodingUTF8 );
	}else
	{
		PARSE_ASDE_STRING_VAL(Name)
		PARSE_ASDE_STRING_VAL(GenericName)
		PARSE_ASDE_STRING_VAL(Comment)
		PARSE_ASDE_STRING_VAL(Icon)
		PARSE_ASDE_STRING_VAL(TryExec)
		PARSE_ASDE_STRING_VAL(Exec)
		PARSE_ASDE_STRING_VAL(Path)
		PARSE_ASDE_STRING_VAL(SwallowTitle)
		PARSE_ASDE_STRING_VAL(SwallowExec)
		PARSE_ASDE_STRING_VAL(SortOrder)
		PARSE_ASDE_STRING_VAL(Categories)
		PARSE_ASDE_STRING_VAL(OnlyShowIn)
		PARSE_ASDE_STRING_VAL(NotShowIn)
		PARSE_ASDE_STRING_VAL(StartupWMClass)

		PARSE_ASDE_FLAG_VAL(NoDisplay)
		PARSE_ASDE_FLAG_VAL(Hidden)
		PARSE_ASDE_FLAG_VAL(Terminal)
		PARSE_ASDE_FLAG_VAL(StartupNotify)	  
	}	   
}

static void
fix_desktop_entry( ASDesktopEntry *de, const char *default_category, const char *icon_path, const char *origin, Bool applnk )
{			   
	if( de->Categories == NULL && default_category )
	{	
		de->Categories = mystrdup(default_category);
	}
	if( applnk ) 
		set_flags( de->flags, ASDE_KDE );

	if( de->Categories != NULL ) 
	{
		de->categories_len = strlen(de->Categories);
		de->categories_shortcuts = parse_category_list( de->Categories, &(de->categories_num) ); 
		if( get_flags( de->flags, ASDE_KDE|ASDE_GNOME ) == 0 )
		{
			int i = de->categories_num;
			ASFlagType kind = 0 ; 
			while ( --i >= 0 && kind == 0 )
			{
				char *ptr = de->categories_shortcuts[i] ;
				if( ptr[0] == 'X' ) 
				{
					if( ptr[1] != '-' ) 
						continue;
					ptr += 2 ; 
				}
				if( mystrncasecmp( ptr, "KDE", 3 ) == 0 ) 
					set_flags( kind, ASDE_KDE );
				else if( mystrncasecmp( ptr, "GNOME", 5 ) == 0 ) 
					set_flags( kind, ASDE_GNOME );
			}
			set_flags( de->flags, kind );
		}	 
	}
	if( get_flags( de->flags, ASDE_KDE|ASDE_GNOME ) == 0 && de->Name) 
	{	
		if( de->Name[0] == 'G' )
			set_flags( de->flags, ASDE_GNOME );
		else if( de->Name[0] == 'K' )
			set_flags( de->flags, ASDE_KDE );
	}
	if( de->Aliases != NULL ) 
	{
		de->aliases_len = strlen(de->Aliases);	  	  
		de->aliases_shortcuts = parse_category_list( de->Aliases, &(de->aliases_num) ); 
	}
	
	if( de->OnlyShowIn != NULL ) 
	{
		de->show_in_len = strlen(de->OnlyShowIn);	  
		de->show_in_shortcuts = parse_category_list( de->OnlyShowIn, &(de->show_in_num) ); 
	}
	if( de->NotShowIn != NULL ) 
	{
		de->not_show_in_len = strlen(de->NotShowIn);	  	  
		de->not_show_in_shortcuts = parse_category_list( de->NotShowIn, &(de->not_show_in_num) ); 
	}
	if( de->Icon && icon_path ) 
	{
		int l = strlen(de->Icon);
		if( l < 4 || mystrcasecmp( de->Icon+l-4, ".png") != 0 )
		{
			char *tmp = safemalloc( l+4+1 );
			strcpy(tmp, de->Icon);
			strcat(tmp, ".png"); 	   
			de->fulliconname = find_file (tmp, icon_path, R_OK);
			free( tmp );
		}	 
		if( de->fulliconname == NULL ) 
			de->fulliconname = find_file (de->Icon, icon_path, R_OK);
	}	 
	if( de->Exec ) 
		de->clean_exec = filter_desktop_entry_exec( de->Exec );
	
	de->origin = mystrdup( origin );
}

static ASDesktopEntry*
parse_desktop_entry( const char *fullfilename, const char *default_category, ASDesktopEntryTypes default_type, const char *icon_path, Bool applnk)
{
	ASDesktopEntry* de = NULL ;
	FILE *fp = NULL;
	
	if( fullfilename  )
		fp = fopen( fullfilename, "r" );

	if( fp ) 
	{
		static char rb[MAXLINELENGTH+1] ; 
		de = create_desktop_entry(default_type);
		while( fgets (&(rb[0]), MAXLINELENGTH, fp) != NULL)
			parse_desktop_entry_line( de, &(rb[0]) ); 
		fix_desktop_entry( de, default_category, icon_path, fullfilename, applnk );
	 	fclose(fp);	
	}	 
	return de;	
}

int
parse_desktop_entry_list( const char *fullfilename, ASBiDirList *entry_list, const char *default_category, ASDesktopEntryTypes default_type, const char *icon_path, Bool applnk)
{
	ASDesktopEntry* de = NULL ;
	int count = 0 ; 
	FILE *fp = NULL;

	LOCAL_DEBUG_OUT( "PARSING \"%s\"", fullfilename );	
	if( fullfilename  )
		fp = fopen( fullfilename, "r" );

	if( fp ) 
	{
		static char rb[MAXLINELENGTH+1] ; 
		while( fgets (&(rb[0]), MAXLINELENGTH, fp) != NULL	) 
		{
			LOCAL_DEBUG_OUT( "rb = \"%s\", de = %p", &(rb[0]), de ); 
			if( rb[0] == '[' )
			{
				if( ( rb[1] =='D' && (mystrncasecmp( &(rb[2]), "esktop Entry]", 13 ) == 0 ||
									  mystrncasecmp( &(rb[2]), "esktopEntry]", 12 ) == 0 )) ||
					( rb[1] == 'K' && mystrncasecmp( &(rb[2]), "DE Desktop Entry]", 17 ) == 0)	
				  ) 
				{
					if( de ) 
					{
						fix_desktop_entry( de, default_category, icon_path, fullfilename, applnk );
						append_bidirelem( entry_list, de);
					}	 
					de = create_desktop_entry(default_type);
					++count ;
				}else if( de ) 
				{	
					fix_desktop_entry( de, default_category, icon_path, fullfilename, applnk );
					append_bidirelem( entry_list, de);
					de = NULL ;
				}
			}else if( de )
				parse_desktop_entry_line( de, &(rb[0]) ); 
		}
		if( de ) 
		{
			fix_desktop_entry( de, default_category, icon_path, fullfilename, applnk );
			append_bidirelem( entry_list, de);
		}	 
	 	fclose(fp);	
	}	 
	return count;	
}



static void 
parse_desktop_entry_tree( char *fullpath, const char *dirname, ASBiDirList *entry_list, ASDesktopEntry *parent, const char *icon_path, const char *default_app_category, Bool applnk )
{
	struct direntry  **list = NULL;
	int list_len, i, curr_dir_index = -1 ;
	char *dir_category = NULL ; 
	ASDesktopEntry * tmp, *curr_dir = NULL;
	 
	
	list_len = my_scandir (fullpath, &list, no_dots_except_directory, NULL);
	for (i = 0; i < list_len; i++)
	{	
		if (!S_ISDIR (list[i]->d_mode) && list[i]->d_name[0] == '.' )
		{	
		 	if( strcasecmp(list[i]->d_name, ".directory") == 0 )
			{
				curr_dir_index = i ;
				tmp = parse_desktop_entry( fullpath, parent?parent->Name:NULL, ASDE_TypeDirectory, icon_path, applnk );		
				if( tmp ) 
				{
					if( tmp->Name == NULL ) 
						tmp->Name = mystrdup( dirname );
					if( mystrcasecmp(tmp->Name, DEFAULT_DESKTOP_CATEGORY_NAME) != 0 ) 
						dir_category = mystrdup( tmp->Name );	
					curr_dir = tmp ;
					append_bidirelem( entry_list, tmp);
				}	 
				break;
			}
		}
	}
	if( curr_dir == NULL ) 
	{
		curr_dir = parse_desktop_entry( fullpath, parent?parent->Name:NULL, ASDE_TypeDirectory, icon_path, applnk );		   
		if( curr_dir ) 
		{	
			if( curr_dir->Name == NULL ) 
			{	
				curr_dir->Name = mystrdup(dirname);
				append_bidirelem( entry_list, curr_dir);
			}
		}
	}	 
	for (i = 0; i < list_len; i++)
	{	
		if( list[i]->d_name[0] != '.' )
		{	
			char *entry_fullpath = make_file_name( fullpath, list[i]->d_name );

			if (S_ISDIR (list[i]->d_mode) )
			{
				parse_desktop_entry_tree( entry_fullpath, list[i]->d_name, entry_list, curr_dir, icon_path, default_app_category, applnk );
			}else if( i != curr_dir_index ) 
			{	
				parse_desktop_entry_list( entry_fullpath, entry_list, dir_category?dir_category:default_app_category, ASDE_TypeApplication, icon_path, applnk);
			}
			free( entry_fullpath );
		}
		free(list[i]);
	}
	
	if( list ) 
		free( list );   

	if( dir_category ) 
		free( dir_category );
}

static void
desktop_entry_destroy_list_item(void *data)
{
	unref_desktop_entry( (ASDesktopEntry*)data );	  
}

static Bool register_desktop_entry_list_item(void *data, void *aux_data)
{
	register_desktop_entry( (ASCategoryTree*)aux_data, (ASDesktopEntry*)data );
	return True;
}

/*************************************************************************/
/* public methods : 													 */
/*************************************************************************/

Bool
load_category_tree( ASCategoryTree*	ct )
{
	if( ct && ct->dir_list ) 
	{	
		int i ; 
		ASBiDirList *entry_list = create_asbidirlist( desktop_entry_destroy_list_item );
		for( i = 0 ; i < ct->dir_num ; ++i ) 
		{	
			Bool applnk = (strstr(ct->dir_list[i], "/applnk")!= NULL) ; 
			if ( CheckDir (ct->dir_list[i]) == 0 )
			{
				/*fprintf( stderr, "location : \"%s\", applnk == %d", ct->dir_list[i], applnk ); */
	  			parse_desktop_entry_tree(ct->dir_list[i], NULL, entry_list, NULL, ct->icon_path, ct->name, applnk );	
			}else if ( CheckFile (ct->dir_list[i]) == 0 )
				parse_desktop_entry_list( ct->dir_list[i], entry_list, ct->name, ASDE_TypeDirectory, ct->icon_path, applnk);
		}
		iterate_asbidirlist( entry_list, register_desktop_entry_list_item, ct, NULL, False);
		destroy_asbidirlist( &entry_list );
/*		flush_asbidirlist_memory_pool(); */
/*		print_unfreed_mem (); */
		return True;
	}
	return False ;
}

void 
DestroyCategories()
{
	if( StandardCategories ) destroy_category_tree( &StandardCategories ); 
	if( KDECategories      ) destroy_category_tree( &KDECategories      ); 	
	if( GNOMECategories    ) destroy_category_tree( &GNOMECategories    ); 	
	if( SystemCategories   ) destroy_category_tree( &SystemCategories   ); 	
	if( CombinedCategories ) destroy_category_tree( &CombinedCategories ); 	
#ifdef DEBUG_ALLOCS
/*	print_unfreed_mem (); */
#endif /* DEBUG_ALLOCS */
}

void 
ReloadCategories()
{
	char *configfile ;
	DestroyCategories();
	
    if( (configfile = make_session_file(Session, STANDARD_CATEGORIES_FILE, False )) != NULL )
	{
		StandardCategories = create_category_tree( "Default", configfile , NULL, 0, -1 );	 
		free( configfile );
	}

	KDECategories = create_category_tree( "KDE", KDE_APPS_PATH, KDE_ICONS_PATH, ASCT_OnlyKDE, -1 );	   
 	GNOMECategories = create_category_tree( "GNOME", GNOME_APPS_PATH, GNOME_ICONS_PATH, ASCT_OnlyGNOME, -1 );	
 	SystemCategories = create_category_tree( "SYSTEM", SYSTEM_APPS_PATH, SYSTEM_ICONS_PATH, ASCT_ExcludeGNOME|ASCT_ExcludeKDE, -1 );	

	CombinedCategories = create_category_tree( "", NULL, NULL, 0, -1 );	 
	
 	load_category_tree( StandardCategories );		   			   

 	add_category_tree_subtree( KDECategories   , StandardCategories );
 	add_category_tree_subtree( GNOMECategories , StandardCategories );
 	add_category_tree_subtree( SystemCategories, StandardCategories );
	
 	load_category_tree( KDECategories    );
	load_category_tree( GNOMECategories  );
 	load_category_tree( SystemCategories );

	fprintf( stderr, "@ Building up Combined: @@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	
 	add_category_tree_subtree( CombinedCategories, StandardCategories );
	add_category_tree_subtree( CombinedCategories, KDECategories      );
 	add_category_tree_subtree( CombinedCategories, GNOMECategories    );
 	add_category_tree_subtree( CombinedCategories, SystemCategories   );
	   
}	 

#ifdef PRINT_DESKTOP_ENTRIES
int 
main( int argc, char ** argv ) 
{
	InitMyApp ("PrintDesktopEntries", argc, argv, NULL, NULL, 0 );
	InitSession();
	ReloadCategories();

	fprintf( stderr, "#Standard: ####################################################\n" );
	print_category_tree2( StandardCategories );
	fprintf( stderr, "#KDE:      ####################################################\n" );
	print_category_tree2( KDECategories );
	fprintf( stderr, "#GNOME:    ####################################################\n" );
	print_category_tree2( GNOMECategories );
	fprintf( stderr, "#SYSTEM:   ####################################################\n" );
	print_category_tree2( SystemCategories );
	fprintf( stderr, "#Combined: ####################################################\n" );
	print_category_tree2( CombinedCategories );
	fprintf( stderr, "#####################################################\n" );

	DestroyCategories();
	FreeMyAppResources();
	return 1;
}

#else
# ifdef TEST_AS_DESKTOP_ENTRY

# define REDHAT_APPLNK	"/etc/X11/applnk"
# define DEBIAN_APPLNK	"/usr/share/applications"

/* 
 * From e-mail : 
 * The paths to the directories should be given by the DESKTOP_FILE_PATH
 * enviromental variable if other directories then /usr/share/applications/ are
 * needed.  This environment variable has the same format as the PATH
 * evironment variable, ':'separating entries.  If DESKTOP_FILE_PATH is present,
 * /usr/share/applications is not checked by default, and thus shoul dbe included
 * in the path.
 * 
 * see: https://listman.redhat.com/archives/xdg-list/2002-July/msg00049.html
 * 		http://standards.freedesktop.org/menu-spec/menu-spec-0.9.html#paths
 * 		http://www.freedesktop.org/Standards/desktop-entry-spec
*/

int 
main( int argc, char ** argv ) 
{
	

//	ASBiDirList *entry_list = create_asbidirlist( desktop_entry_destroy_list_item );

	InitMyApp ("TestASDesktopEntry", argc, argv, NULL, NULL, 0 );
	InitSession();
	ReloadCategories();
//	ReloadCategories();
//	ReloadCategories();

	fprintf( stderr, "#Standard: ####################################################\n" );
	print_category_tree2( StandardCategories );
	fprintf( stderr, "#KDE: ####################################################\n" );
	print_category_tree2( KDECategories );
	fprintf( stderr, "#GNOME: ####################################################\n" );
	print_category_tree2( GNOMECategories );
	fprintf( stderr, "#SYSTEM: ####################################################\n" );
	print_category_tree2( SystemCategories );
	fprintf( stderr, "#Combined: ####################################################\n" );
	print_category_tree2( CombinedCategories );
	fprintf( stderr, "#####################################################\n" );

	DestroyCategories();
	FreeMyAppResources();
#   ifdef DEBUG_ALLOCS
	print_unfreed_mem ();
#   endif /* DEBUG_ALLOCS */
	return 1;
}
# else
#  ifdef MAKE_STANDARD_CATEGORIES
/* helper app to generate set of .desktop files for the list */
int 
main( int argc, char ** argv ) 
{
 	char *doc_str;
	xml_elem_t* doc;
	xml_elem_t *tr, *td;


	
	if( argc < 2 ) 
	{
		show_error("Usage: make_standard_categories <source_file_name>\n");
		exit(1);
	}
		
	doc_str = load_file(argv[1]);   
	if (!doc_str) 
	{
		show_error("Unable to load file [%s]: %s.\n", argv[1], strerror(errno));
		exit(1);
	}
	
	doc = xml_parse_doc(doc_str, NULL);
	if (!doc) 
	{
		show_error("Unable to parse data: %s.\n", strerror(errno));
		exit(1);
	}

	for (tr = doc->child ; tr ; tr = tr->next) 
	{
		if( strcmp( tr->tag, "tr" ) == 0 ) 
		{
	 		char *name = NULL , *parent = NULL, *comment = NULL ;
			td = tr->child ;
			if( td && td->child && strcmp(td->child->tag, XML_CDATA_STR ) == 0 ) 
			{
				name = td->child->parm ; 
				td = td->next ; 	
				if( td ) 
				{
					if( td->child && strcmp(td->child->tag, XML_CDATA_STR ) == 0 ) 
						comment = td->child->parm ; 
					td = td->next ; 	
					if( td && td->child && strcmp(td->child->tag, XML_CDATA_STR ) == 0 ) 
						parent = td->child->parm ; 
				}	 
			}	 
			if( name && strlen(name) < 200) 
			{
				char filename[1024] ;
				FILE *fp ;
				sprintf( filename, "categories/%s.desktop", name ) ;
				fp = fopen( filename, "wt" );
				if( fp ) 
				{
					fprintf( fp, "[DesktopEntry]\nName=%s\n", name );
					fprintf( fp, "X-AfterStep-IndexName=%s\n", name );
					fprintf( fp, "Categories=%s\n", parent?parent:"" );
					fprintf( fp, "Comment=%s\n", comment?comment:"" );
					fclose( fp );	
				}	 
			}	 
			
		}	  
	}
	
	return 1;
}
#  endif
# endif
#endif


