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



static ASDesktopEntry*
parse_desktop_entry( const char *path, const char *fname, const char *default_category, ASDesktopEntryTypes default_type, const char *icon_path)
{
	char *fullfilename = make_file_name( path, fname );
	FILE *fp = NULL;
	ASDesktopEntry* de = NULL ;
	if( fullfilename  )
		fp = fopen( fullfilename, "r" );

#define PARSE_ASDE_TYPE_VAL(val)	if(mystrncasecmp(ptr, #val, ASDE_KEYWORD_##val##_LEN) == 0){de->type = ASDE_Type##val; continue;}					   			   				   
#define PARSE_ASDE_STRING_VAL(val)	if(mystrncasecmp(ptr, #val "=", ASDE_KEYWORD_##val##_LEN+1) == 0){de->val = stripcpy(ptr+ASDE_KEYWORD_##val##_LEN+1); continue;}					   
#define PARSE_ASDE_FLAG_VAL(val)	if(mystrncasecmp(ptr, #val "=", ASDE_KEYWORD_##val##_LEN+1) == 0){set_flags(de->flags, ASDE_##val); continue;}					   			   

	if( fp ) 
	{
		static char rb[MAXLINELENGTH+1] ; 
		de = create_desktop_entry(default_type);
		while( fgets (&(rb[0]), MAXLINELENGTH, fp) != NULL)
		{
			char *ptr = &(rb[0]);
			if( ptr[0] == 'X' && ptr[1] == '-') 
			{
				ptr +=2 ; 
				if( mystrncasecmp( ptr, "AfterStep-", 10 ) == 0 ) 
				{
					ptr += 10 ;
					PARSE_ASDE_STRING_VAL(IndexName)
					PARSE_ASDE_STRING_VAL(Alias)
				}	 
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
		if( default_category && de->Categories == NULL )
			de->Categories = mystrdup(default_category);

		if( de->Categories != NULL ) 

		if( de->Categories != NULL ) 
		{
			de->categories_len = strlen(de->Categories);
			de->categories_shortcuts = parse_category_list( de->Categories, &(de->categories_num) ); 
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

	 	fclose(fp);	
	}	 
	if( de ) 
		de->origin = fullfilename ;
	else
		free( fullfilename );
	return de;	
}

static void 
parse_desktop_entry_tree(const char *path, const char *dirname, ASBiDirList *entry_list, ASDesktopEntry *parent, const char *icon_path )
{
	char *fullpath = make_file_name( path, dirname );
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
				tmp = parse_desktop_entry( fullpath, list[i]->d_name, parent?parent->Name:NULL, ASDE_TypeDirectory, icon_path );		
				if( tmp ) 
				{
					if( tmp->Name == NULL ) 
						tmp->Name = mystrdup( list[i]->d_name );
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
		curr_dir = parse_desktop_entry( path, dirname, parent?parent->Name:NULL, ASDE_TypeDirectory, icon_path );		   
		if( curr_dir ) 
		{	
			curr_dir->Name = mystrdup(dirname);
			append_bidirelem( entry_list, curr_dir);
		}
	}	 
	for (i = 0; i < list_len; i++)
	{	
		if( list[i]->d_name[0] != '.' )
		{	
			if (S_ISDIR (list[i]->d_mode) )
			{
				parse_desktop_entry_tree( fullpath, list[i]->d_name, entry_list, curr_dir, icon_path );
			}else if( i != curr_dir_index ) 
			{	
				tmp = parse_desktop_entry( fullpath, list[i]->d_name, dir_category, ASDE_TypeApplication, icon_path );		   
				if( tmp ) 
					append_bidirelem( entry_list, tmp);
			}
		}
		free(list[i]);
	}
	
	if( list ) 
		free( list );   

	free( fullpath );
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
	if( ct && ct->origin ) 
	{	
		if ( CheckDir (ct->origin) == 0 )
		{
			ASBiDirList *entry_list = create_asbidirlist( desktop_entry_destroy_list_item );
		
			parse_desktop_entry_tree(ct->origin, NULL, entry_list, NULL, ct->icon_path );	
			
			iterate_asbidirlist( entry_list, register_desktop_entry_list_item, ct, NULL, False);
			destroy_asbidirlist( &entry_list );
			return True;
		}	  
	}
	return False ;
}


#ifdef TEST_AS_DESKTOP_ENTRY

#define REDHAT_APPLNK	"/etc/X11/applnk"
#define DEBIAN_APPLNK	"/usr/share/applications"

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
	
	const char * default_path_gnome = getenv("GNOMEDIR");
	const char * default_path_kde = getenv("KDEDIR");
	const char *gnome_path = default_path_gnome;
	const char *kde_path = default_path_kde;
	ASCategoryTree *standard_tree = NULL ; 
	ASCategoryTree *gnome_tree = NULL ; 
	ASCategoryTree *kde_tree = NULL ; 
	ASCategoryTree *combined_tree = NULL ; 


//	ASBiDirList *entry_list = create_asbidirlist( desktop_entry_destroy_list_item );

	InitMyApp ("TestASDesktopEntry", argc, argv, NULL, NULL, ASS_Restarting );
	
	standard_tree = create_category_tree( "", "./", "categories", NULL, 0, -1 );	 

	if( gnome_path != NULL ) 
	{	
		char *gnome_icon_path = make_file_name( gnome_path, "share/pixmaps" );
		show_progress( "reading GNOME entries from \"%s\"",  gnome_path );
		gnome_tree = create_category_tree( "GNOME", gnome_path, "share/applications", gnome_icon_path, 0, -1 );	
		//parse_desktop_entry_tree(gnome_path, "share/applications", entry_list, NULL, gnome_icon_path );	
		//show_progress( "entries loaded: %d",  entry_list->count ); 
		free( gnome_icon_path );
	}
	if( kde_path != NULL ) 
	{	   
		char *kde_icon_path = safemalloc(strlen(kde_path)+256 );
		show_progress( "reading KDE entries from \"%s\"",  kde_path );
		sprintf( kde_icon_path, "%s/share/icons/default.kde/48x48/apps:%s/share/icons/hicolor/48x48/apps:%s/share/icons/locolor/48x48/apps", kde_path, kde_path, kde_path );
		show_progress( "KDE icon_path is \"%s\"",  kde_icon_path );
		kde_tree = create_category_tree( "KDE", kde_path, "share/applnk", kde_icon_path, 0, -1 );	 
	 	//parse_desktop_entry_tree(kde_path, "share/applnk", entry_list, NULL, kde_icon_path );	 
		//show_progress( "entries loaded: %d",  entry_list->count ); 
		free( kde_icon_path );
	}
	//iterate_asbidirlist( entry_list, desktop_entry_print, NULL, NULL, False);

	combined_tree = create_category_tree( "", NULL, NULL, NULL, 0, -1 );	 
	
	load_category_tree( standard_tree );		   			   

	add_category_tree_subtree( gnome_tree, standard_tree );
	load_category_tree( gnome_tree );		   
	add_category_tree_subtree( kde_tree, standard_tree );
	load_category_tree( kde_tree );
	add_category_tree_subtree( combined_tree, standard_tree );
	add_category_tree_subtree( combined_tree, gnome_tree );
	add_category_tree_subtree( combined_tree, kde_tree );

	   
	print_category_tree( kde_tree );
	print_category_tree( gnome_tree );
	print_category_tree( combined_tree );

	fprintf( stderr, "#####################################################\n" );
	print_category_tree2( standard_tree );
	fprintf( stderr, "#####################################################\n" );
	print_category_tree2( combined_tree );
	fprintf( stderr, "#####################################################\n" );

	destroy_category_tree( &gnome_tree );
	destroy_category_tree( &kde_tree );
	FreeMyAppResources();
#ifdef DEBUG_ALLOCS
	print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */
	return 1;
}
#else
#ifdef MAKE_STANDARD_CATEGORIES
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
#endif
#endif


