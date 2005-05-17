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

#include "afterconf.h"

typedef enum
{
	ASDE_TypeApplication = 0,	
	ASDE_TypeLink,	  
	ASDE_TypeFSDevice,	  
	ASDE_TypeDirectory,	  
	ASDE_Types	  
}ASDesktopEntryTypes ;

#define ASDE_KEYWORD_Application_LEN	11
#define ASDE_KEYWORD_Link_LEN			4	  
#define ASDE_KEYWORD_FSDevice_LEN		8	  
#define ASDE_KEYWORD_Directory_LEN		9	  



typedef struct ASDesktopEntry
{
#define ASDE_NoDisplay			(0x01<<0)	
#define ASDE_Hidden				(0x01<<1)	  
#define ASDE_Terminal			(0x01<<2)	  
#define ASDE_StartupNotify		(0x01<<3)	  
#define ASDE_EncodingUTF8		(0x01<<4)	  
	ASFlagType flags ; 

#define ASDE_KEYWORD_NoDisplay_LEN			9	
#define ASDE_KEYWORD_Hidden_LEN				6
#define ASDE_KEYWORD_Terminal_LEN			8
#define ASDE_KEYWORD_StartupNotify_LEN		13
	
	ASDesktopEntryTypes type ;	 
	
	char *Name_localized ;
	char *Comment_localized ;

	char *Name ; 
#define ASDE_KEYWORD_Name_LEN	4
	char *GenericName ;
#define ASDE_KEYWORD_GenericName_LEN	11
	char *Comment ; 
#define ASDE_KEYWORD_Comment_LEN	7

	char *Icon ;
#define ASDE_KEYWORD_Icon_LEN	4

	char *TryExec ;
#define ASDE_KEYWORD_TryExec_LEN	7
	char *Exec ;
#define ASDE_KEYWORD_Exec_LEN	4
	char *Path ;               /* work dir */
#define ASDE_KEYWORD_Path_LEN	4

	
	char *SwallowTitle ;
#define ASDE_KEYWORD_SwallowTitle_LEN	12
	char *SwallowExec ;
#define ASDE_KEYWORD_SwallowExec_LEN	11
	char *SortOrder ;
#define ASDE_KEYWORD_SortOrder_LEN	9
	
	char *Categories ;
#define ASDE_KEYWORD_Categories_LEN	10
	char *OnlyShowIn ;
#define ASDE_KEYWORD_OnlyShowIn_LEN	10
	char *NotShowIn ;
#define ASDE_KEYWORD_NotShowIn_LEN	9
	char *StartupWMClass ;
#define ASDE_KEYWORD_StartupWMClass_LEN	14

	/* calculated stuff : */
	int categories_len ; 
	char **categories_shortcuts ; 
	int categories_num ; 
	
	int show_in_len ; 
	char **show_in_shortcuts ; 
	int show_in_num ; 

	int not_show_in_len ; 
	char **not_show_in_shortcuts ; 
	int not_show_in_num ; 

	char *fulliconname ;
	char *clean_exec ; 

}ASDesktopEntry;

typedef struct ASDesktopCategory
{
	char *name ;
	ASVector *entries ;
}ASDesktopCategory;

ASHashTable *_as_application_registry = NULL ;
ASHashTable *_as_categories_registry = NULL ;

/*************************************************************************/
/* Desktop Category functionality                                        */
/*************************************************************************/

ASDesktopCategory *
create_desktop_category( const char *name )
{
	ASDesktopCategory *dc = safecalloc( 1, sizeof(ASDesktopCategory) );
	if( name ) 
		dc->name = mystrdup(name);
	dc->entries = create_asvector( sizeof(char*) );
	return dc;
}

void 
destroy_desktop_category( ASDesktopCategory **pdc )
{
	if( pdc ) 
	{
		ASDesktopCategory *dc = *pdc ;
		if( dc ) 
		{
			if( dc->name ) 
				free( dc->name );
			if( dc->entries )
			{	
				char **pe = PVECTOR_HEAD(char*,dc->entries);
				int i = PVECTOR_USED(dc->entries);
				while( --i >= 0 )
					if( pe[i] ) 
						free( pe[i] );
				destroy_asvector( &(dc->entries) );
			}
			free( dc );
			*pdc = NULL ;
		}	 
	}
}	 

/*************************************************************************/
/* Desktop Entry functionality                                           */
/*************************************************************************/
void 
destroy_desktop_entry( ASDesktopEntry** pde )
{
	if( pde ) 
	{
		ASDesktopEntry* de ;
		if( (de = *pde) != NULL ) 
		{
#define FREE_ASDE_VAL(val)	do{if(de->val) free( de->val );}while(0)			
			FREE_ASDE_VAL(Name_localized) ;
			FREE_ASDE_VAL(Comment_localized) ;

			FREE_ASDE_VAL(Name) ; 
			FREE_ASDE_VAL(GenericName) ;
			FREE_ASDE_VAL(Comment) ; 

			FREE_ASDE_VAL(Icon) ;

			FREE_ASDE_VAL(TryExec) ;
			FREE_ASDE_VAL(Exec) ;
			FREE_ASDE_VAL(Path) ;               /* work dir */


			FREE_ASDE_VAL(SwallowTitle) ;
			FREE_ASDE_VAL(SwallowExec) ;
			FREE_ASDE_VAL(SortOrder) ;

			FREE_ASDE_VAL(Categories) ;
			FREE_ASDE_VAL(OnlyShowIn) ;
			FREE_ASDE_VAL(NotShowIn) ;
			FREE_ASDE_VAL(StartupWMClass) ;

			FREE_ASDE_VAL(categories_shortcuts) ; 
			FREE_ASDE_VAL(show_in_shortcuts) ; 
			FREE_ASDE_VAL(not_show_in_shortcuts);
			FREE_ASDE_VAL(fulliconname);
			free( de );
			*pde = NULL ;
		}
	}
}	 

void 
print_desktop_entry( ASDesktopEntry* de )
{
	if( de != NULL ) 
	{
		switch( de->type ) 
		{
			case ASDE_TypeApplication : fprintf(stderr, "de(%p).type=Application;\n", de );break;
			case ASDE_TypeLink :  fprintf(stderr, "de(%p).type=Link;\n", de );break;
			case ASDE_TypeFSDevice : fprintf(stderr, "de(%p).type=FSDevice;\n", de );break;	  
			case ASDE_TypeDirectory : fprintf(stderr, "de(%p).type=Directory;\n", de );break;	  
			default: break;
		}

#define PRINT_ASDE_VAL(val)	do{if(de->val) fprintf(stderr, "de(%p)." #val "=\"%s\";\n", de, de->val );}while(0)			
		PRINT_ASDE_VAL(Name_localized) ;
		PRINT_ASDE_VAL(Comment_localized) ;

		PRINT_ASDE_VAL(Name) ; 
		PRINT_ASDE_VAL(GenericName) ;
		PRINT_ASDE_VAL(Comment) ; 

		PRINT_ASDE_VAL(Icon) ;

		PRINT_ASDE_VAL(TryExec) ;
		PRINT_ASDE_VAL(Exec) ;
		PRINT_ASDE_VAL(Path) ;               /* work dir */


		PRINT_ASDE_VAL(SwallowTitle) ;
		PRINT_ASDE_VAL(SwallowExec) ;
		PRINT_ASDE_VAL(SortOrder) ;

		PRINT_ASDE_VAL(Categories) ;
		PRINT_ASDE_VAL(OnlyShowIn) ;
		PRINT_ASDE_VAL(NotShowIn) ;
		PRINT_ASDE_VAL(StartupWMClass) ;

//		PRINT_ASDE_VAL(categories_shortcuts) ; 
//		PRINT_ASDE_VAL(show_in_shortcuts) ; 
//		PRINT_ASDE_VAL(not_show_in_shortcuts);
		PRINT_ASDE_VAL(fulliconname);
		PRINT_ASDE_VAL(clean_exec) ;
		fprintf( stderr, "\n" );
	}
}	 


void
desktop_entry_destroy(void *data)
{
	ASDesktopEntry *de = (ASDesktopEntry*)data;
	destroy_desktop_entry( &de );	
}

Bool
desktop_entry_print(void *data, void *aux_data)
{
	print_desktop_entry( (ASDesktopEntry*)data );	
	return True;
}


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
		if( i > 0 ) 
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

char *
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


ASDesktopEntry*
parse_desktop_entry( const char *path, const char *fname, const char *default_category, ASDesktopEntryTypes default_type, const char *icon_path)
{
	char *fullfilename = make_file_name( path, fname );
	FILE *fp = NULL;
	ASDesktopEntry* de = NULL ;
	if( fullfilename  )
		fp = fopen( fullfilename, "r" );

	if( fp ) 
	{
		static char rb[MAXLINELENGTH+1] ; 
		de = safecalloc(1, sizeof(ASDesktopEntry));
		de->type = default_type ;
		while( fgets (&(rb[0]), MAXLINELENGTH, fp) != NULL)
		{
			char *ptr = &(rb[0]);
			if( mystrncasecmp( ptr, "Name[", 5 ) == 0 ) 
			{
				/* TODO */
			}else if( mystrncasecmp( ptr, "Type=", 5 ) == 0 ) 
			{
				ptr += 5 ;

#define PARSE_ASDE_TYPE_VAL(val)	if(mystrncasecmp(ptr, #val, ASDE_KEYWORD_##val##_LEN) == 0){de->type = ASDE_Type##val; continue;}					   			   				   
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
#define PARSE_ASDE_STRING_VAL(val)	if(mystrncasecmp(ptr, #val "=", ASDE_KEYWORD_##val##_LEN+1) == 0){de->val = stripcpy(ptr+ASDE_KEYWORD_##val##_LEN+1); continue;}					
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

#define PARSE_ASDE_FLAG_VAL(val)	if(mystrncasecmp(ptr, #val "=", ASDE_KEYWORD_##val##_LEN+1) == 0){set_flags(de->flags, ASDE_##val); continue;}					   			   
				PARSE_ASDE_FLAG_VAL(NoDisplay)
				PARSE_ASDE_FLAG_VAL(Hidden)
				PARSE_ASDE_FLAG_VAL(Terminal)
				PARSE_ASDE_FLAG_VAL(StartupNotify)	  
			}	   

		}	 
		if( default_category && de->Categories == NULL )
			de->Categories = mystrdup(default_category);
		
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
	free( fullfilename );
	return de;	
}

void 
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

#ifdef TEST_AS_DESKTOP_ENTRY

int 
main( int argc, char ** argv ) 
{
	const char * default_path_gnome = getenv("GNOMEDIR");
	const char * default_path_kde = getenv("KDEDIR");
	const char *gnome_path = default_path_gnome;
	const char *kde_path = default_path_kde;
	ASBiDirList *entry_list = create_asbidirlist( desktop_entry_destroy );
	

	InitMyApp ("TestASDesktopEntry", argc, argv, NULL, NULL, ASS_Restarting );
	
	if( gnome_path != NULL ) 
	{	
		char *gnome_icon_path = make_file_name( gnome_path, "share/pixmaps" );
		show_progress( "reading GNOME entries from \"%s\"",  gnome_path );
		parse_desktop_entry_tree(gnome_path, "share/applications", entry_list, NULL, gnome_icon_path );	
		show_progress( "entries loaded: %d",  entry_list->count );
		free( gnome_icon_path );
	}
	if( kde_path != NULL ) 
	{	   
		char *kde_icon_path = safemalloc(strlen(kde_path)+256 );
		show_progress( "reading KDE entries from \"%s\"",  kde_path );
		sprintf( kde_icon_path, "%s/share/icons/default.kde/48x48/apps:%s/share/icons/hicolor/48x48/apps:%s/share/icons/locolor/48x48/apps", kde_path, kde_path, kde_path );
		show_progress( "KDE icon_path is \"%s\"",  kde_icon_path );
		parse_desktop_entry_tree(kde_path, "share/applnk", entry_list, NULL, kde_icon_path );	 
		show_progress( "entries loaded: %d",  entry_list->count );
		free( kde_icon_path );
	}
	iterate_asbidirlist( entry_list, desktop_entry_print, NULL, NULL, False);

	FreeMyAppResources();
#ifdef DEBUG_ALLOCS
	print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */
	return 1;
}

#endif
