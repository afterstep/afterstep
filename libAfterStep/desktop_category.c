/*
 * Copyright (C) 2005 Sasha Vasko <sasha at aftercode.net>
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

#include "../configure.h"
#include <unistd.h>

#include "asapp.h"
#include "afterstep.h"
#include "desktop_category.h"

/*************************************************************************/
/* Desktop Category functionality                                        */
/*************************************************************************/

ASDesktopCategory *
create_desktop_category( const char *name )
{
	ASDesktopCategory *dc = safecalloc( 1, sizeof(ASDesktopCategory) );
	dc->ref_count = 1 ;
	dc->index_name = mystrdup(name);
	dc->entries = create_asvector( sizeof(char*) );
	return dc;
}

static void 
destroy_desktop_category( ASDesktopCategory **pdc )
{
	if( pdc ) 
	{
		ASDesktopCategory *dc = *pdc ;
		if( dc ) 
		{
			if( dc->name ) 
				free( dc->name );
			if( dc->index_name ) 
				free( dc->index_name );
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

int ref_desktop_category( ASDesktopCategory *dc ) 
{
	if( dc ) 
		return ++dc->ref_count	;
	return 0;
}	 

int unref_desktop_category( ASDesktopCategory *dc ) 
{
	if( dc ) 
	{
		if( (--dc->ref_count) > 0 ) 
			return dc->ref_count;

		destroy_desktop_category( &dc );
	}
	return 0;
}	 

static void
desktop_category_destroy(ASHashableValue value, void *data)
{
	char *alias = (char*)value ; 
	ASDesktopCategory *dc = (ASDesktopCategory *)data ;

	if( alias != dc->name ) 
		free( alias );

	unref_desktop_category( dc ); 
}


void 
add_desktop_category_entry( ASDesktopCategory *dc, const char *entry_name )
{
	if( dc && entry_name ) 
	{
		LOCAL_DEBUG_OUT( "adding \"%s\" to category \"%s\"", entry_name, dc->index_name );
		if( mystrcasecmp(dc->name, entry_name) != 0 )
		{		               /* to prevent cyclic hierarchy !!! */
			char **existing = PVECTOR_HEAD(char*,dc->entries);
			int num = PVECTOR_USED(dc->entries);
			char *entry_name_copy;
			while ( --num >= 0 )
			{
				if( strcmp( existing[num], entry_name ) == 0 ) 
					return;
			}
			entry_name_copy = mystrdup(entry_name);
			append_vector( dc->entries, &entry_name_copy, 1 );
		}
	}
}

void 
remove_desktop_category_entry( ASDesktopCategory *dc, const char *entry_name )
{
	char **existing = PVECTOR_HEAD(char*,dc->entries);
	int num = PVECTOR_USED(dc->entries);
	LOCAL_DEBUG_OUT( "removing \"%s\" from category \"%s\"", entry_name, dc->name );
	while ( --num >= 0 )
	{
		if( strcmp( existing[num], entry_name ) == 0 ) 
		{
			free( existing[num] );
			vector_remove_index( dc->entries, num);
			return;
		}		  
	}
}

void 
print_desktop_category( ASDesktopCategory *dc )	
{
	if( dc ) 
	{
		char **entries = PVECTOR_HEAD(char*,dc->entries);
		int num = PVECTOR_USED(dc->entries);
		fprintf( stderr, "category(%p).index_name=\"%s\";\n", dc, dc->index_name );
		fprintf( stderr, "category(%p).name=\"%s\";\n", dc, dc->name?dc->name:"(null)" );
		fprintf( stderr, "category(%p).entries_num=%d;\n", dc, num );
		if( num > 0 ) 
		{	
			int i;
			fprintf( stderr, "category(%p).entries=", dc );
			for( i = 0 ; i < num ; ++i )
				fprintf( stderr, "%c\"%s\"", (i==0)?'{':';', entries[i] );
			fputs( "};\n", stderr );
		}	 
	}		  
}

void 
desktop_category_print( ASHashableValue value, void *data )
{
	print_desktop_category( (ASDesktopCategory*)data );	
}

/*************************************************************************/
/* Desktop Entry functionality                                           */
/*************************************************************************/

ASDesktopEntry *create_desktop_entry( ASDesktopEntryTypes default_type)
{	
 	ASDesktopEntry *de = safecalloc(1, sizeof(ASDesktopEntry));
	de->ref_count = 1 ; 
	de->type = default_type ;
	return de;
}

static void 
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

			FREE_ASDE_VAL(IndexName) ;
			FREE_ASDE_VAL(Alias) ;
			
			FREE_ASDE_VAL(categories_shortcuts) ; 
			FREE_ASDE_VAL(show_in_shortcuts) ; 
			FREE_ASDE_VAL(not_show_in_shortcuts);
			FREE_ASDE_VAL(fulliconname);
			FREE_ASDE_VAL(origin);
			free( de );
			*pde = NULL ;
		}
	}
}	 

int ref_desktop_entry( ASDesktopEntry *de ) 
{
	if( de ) 
		return ++de->ref_count	;
	return 0;
}	 

int unref_desktop_entry( ASDesktopEntry *de ) 
{
	if( de ) 
	{
		if( (--de->ref_count) > 0 ) 
			return de->ref_count;

		destroy_desktop_entry( &de );
	}
	return 0;
}	 



void 
print_desktop_entry( ASDesktopEntry* de )
{
	if( de != NULL ) 
	{
		int i ;

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

		fprintf(stderr, "de(%p).categories_num=%d;\n", de, de->categories_num );
		for( i = 0 ; i < de->categories_num ; ++i ) 
			fprintf(stderr, "de(%p).category[%d]=\"%s\";\n", de, i, de->categories_shortcuts[i] );
		PRINT_ASDE_VAL(Categories) ;
		PRINT_ASDE_VAL(OnlyShowIn) ;
		PRINT_ASDE_VAL(NotShowIn) ;
		PRINT_ASDE_VAL(StartupWMClass) ;

		PRINT_ASDE_VAL(IndexName) ;
		PRINT_ASDE_VAL(Alias) ;

//		PRINT_ASDE_VAL(categories_shortcuts) ; 
//		PRINT_ASDE_VAL(show_in_shortcuts) ; 
//		PRINT_ASDE_VAL(not_show_in_shortcuts);
		PRINT_ASDE_VAL(fulliconname);
		PRINT_ASDE_VAL(clean_exec) ;

		PRINT_ASDE_VAL(origin) ;
		fprintf( stderr, "\n" );
	}
}	 


void
desktop_entry_destroy(ASHashableValue value, void *data)
{
	unref_desktop_entry( (ASDesktopEntry*)data );	
}

void
desktop_entry_print(ASHashableValue value, void *data)
{
	print_desktop_entry( (ASDesktopEntry*)data );	
}


/*************************************************************************
 * Desktop Category Tree functionality : 
 *************************************************************************/

ASDesktopCategory *
fetch_desktop_category( ASCategoryTree *ct, const char *cname )
{
	void *tmp = NULL;
	int res ; 
/*	LOCAL_DEBUG_OUT( "fetching \"%s\"", cname?cname:"(null)" );	*/
	if( cname == NULL ) 
		return ct->default_category;
	
	res = get_hash_item( ct->categories, AS_HASHABLE(cname), &tmp );
	
	if( res != ASH_Success )
		return NULL;	   
	
/*	LOCAL_DEBUG_OUT( "found \"%s\" with ptr %p", cname, tmp );	  */
	return (ASDesktopCategory*)tmp ;
}

ASDesktopEntry *
fetch_desktop_entry( ASCategoryTree *ct, const char *name )
{
	void *tmp = NULL;
/*	LOCAL_DEBUG_OUT( "fetching \"%s\"", cname?cname:"(null)" );	*/
	if( name != NULL ) 
		if( get_hash_item( ct->entries, AS_HASHABLE(name), &tmp ) == ASH_Success )
			return (ASDesktopEntry*)tmp;
	return NULL;	   
}


ASDesktopCategory *
obtain_category( ASCategoryTree *ct, const char *cname, Bool dont_add_to_default )
{
	ASDesktopCategory *dc = fetch_desktop_category( ct, cname );
	if( dc == NULL ) 		
	{
		dc = create_desktop_category( cname );
		LOCAL_DEBUG_OUT( "creating category using index_name \"%s\"", cname );
		if( add_hash_item( ct->categories, AS_HASHABLE(dc->index_name), dc) != ASH_Success ) 
		{	
			unref_desktop_category( dc );
			dc = NULL ;
		}else if( !dont_add_to_default && mystrcasecmp(cname, DEFAULT_DESKTOP_CATEGORY_NAME) != 0 ) 
			add_desktop_category_entry( ct->default_category, cname );
	}	 
	return dc;
}	 

Bool register_desktop_entry(ASCategoryTree *ct, ASDesktopEntry *de)
{
	int i ; 
	if( de && de->Name ) 
	{	
		Bool top_level = False ; 
		char *index_name = de->IndexName?de->IndexName:de->Name ;
		if( add_hash_item( ct->entries, AS_HASHABLE(index_name), de) == ASH_Success ) 
			ref_desktop_entry( de );  
		if( de->Name != index_name ) 
			if( add_hash_item( ct->entries, AS_HASHABLE(de->Name), de) == ASH_Success ) 
				ref_desktop_entry( de );  
		
		if( de->categories_num == 0 || 
			mystrcasecmp(de->categories_shortcuts[0], DEFAULT_DESKTOP_CATEGORY_NAME ) == 0 )
		{
			top_level = True ; 
		}
		
		if( de->type == ASDE_TypeDirectory ) 
		{
			ASDesktopCategory *dc = NULL ;
			void *tmp = NULL;
			LOCAL_DEBUG_OUT( "desktop entry is a directory \"%s\"", de->Name );
			print_desktop_entry( de );
			
			if( get_hash_item( ct->categories, AS_HASHABLE(de->Name), &tmp ) == ASH_Success )
				dc = (ASDesktopCategory *)tmp;
			
			if( dc == NULL && de->IndexName ) 
			{	
				if( get_hash_item( ct->categories, AS_HASHABLE(de->IndexName), &tmp ) == ASH_Success )
					dc = (ASDesktopCategory *)tmp;
			}
			
			if( dc == NULL && de->Alias ) 
			{	
				if( get_hash_item( ct->categories, AS_HASHABLE(de->Alias), &tmp ) == ASH_Success )
					dc = (ASDesktopCategory *)tmp;
			}
			
			if( dc != NULL ) 
			{
				LOCAL_DEBUG_OUT( "category already exists with index_name \"%s\" and name \"%s\"", dc->index_name, dc->name?dc->name:"(null)" );
				if( !top_level ) 
					remove_desktop_category_entry( ct->default_category, dc->index_name );
				index_name = dc->index_name ; 
			}else	 
			{	
		 		dc = create_desktop_category( index_name );
				LOCAL_DEBUG_OUT( "creating category using index_name \"%s\"", index_name );
				if( add_hash_item( ct->categories, AS_HASHABLE(dc->index_name), dc) != ASH_Success ) 
				{	
					unref_desktop_category( dc );
					dc = NULL ; 
				}
			}
			if( dc->name == NULL ) 
			{
				LOCAL_DEBUG_OUT( "setting category name to \"%s\"", de->Name );
	  			dc->name = mystrdup( de->Name );
			}

			if( de->Alias ) 	
			{
				char *tmp = mystrdup(de->Alias);	
				if( add_hash_item( ct->categories, AS_HASHABLE(tmp), dc) != ASH_Success ) 		
					free( tmp );
				else
				{
					LOCAL_DEBUG_OUT( "adding category alias to \"%s\"", de->Alias );	  
					ref_desktop_category( dc );  
				}
			}	 
			{
				char *tmp = mystrdup(de->Name);	
				if( add_hash_item( ct->categories, AS_HASHABLE(tmp), dc) != ASH_Success ) 		
					free( tmp );
				else
				{
					LOCAL_DEBUG_OUT( "adding category reference using common name \"%s\"", de->Name );	  	  
					ref_desktop_category( dc );  
				}
			}	 
		}	 

		if( top_level ) 
			add_desktop_category_entry( ct->default_category, index_name );

		for( i = top_level?1:0 ; i < de->categories_num ; ++i ) 
		{
			ASDesktopCategory *dc = NULL ; 
			if( de->categories_num == 1 || 
				mystrcasecmp( ct->name, de->categories_shortcuts[i] ) != 0 ) 
				dc = obtain_category( ct, de->categories_shortcuts[i], False ); 

			if( dc ) 
				add_desktop_category_entry( dc, index_name );
		}	
		LOCAL_DEBUG_OUT( "belongs to %d categories", i );
		return True;
	}
	return False;
}	 

ASCategoryTree*
create_category_tree( const char *name, const char *path, const char *icon_path, ASFlagType flags, int max_depth )	
{
	ASCategoryTree *ct = safecalloc( 1, sizeof(ASCategoryTree));
	ct->max_depth = max_depth ; 
	ct->flags = flags ; 
	if( path )
	{
		int i = 0 ;
		char *clean_path = copy_replace_envvar( path );
		while( clean_path[i] ) 
		{
			if( clean_path[i] == ':' && i > 0 && clean_path[i-1] != ':' )
				++(ct->dir_num);
			++i ;
		}	 
		if( i > 1 && clean_path[i] != ':' )
			++(ct->dir_num);
		if( ct->dir_num > 0 ) 
		{	
			int dir = 0 ; 
			ct->dir_list = safecalloc( ct->dir_num, sizeof( char*) );
			i = 0 ; 
			do 
			{	
				while( clean_path[i] == ':' ) ++i ; 
				if( clean_path[i] ) 
				{
					int start = i; 
					while( clean_path[i] != ':' && clean_path[i] ) ++i ; 
					if( i - start > 0 ) 
						ct->dir_list[dir++] = mystrndup( &clean_path[start], i-start);
				}	 
			}while( clean_path[i] );
		}
		free( clean_path ) ;
	}
	ct->name = mystrdup(name);
	ct->icon_path = copy_replace_envvar(icon_path);
	ct->entries = create_ashash( 0, string_hash_value, string_compare, desktop_entry_destroy );
	ct->categories = create_ashash( 0, casestring_hash_value, casestring_compare, desktop_category_destroy );
	ct->default_category = obtain_category( ct, DEFAULT_DESKTOP_CATEGORY_NAME, True );	
	return ct;
}

void
destroy_category_tree( ASCategoryTree **pct )
{
	if( pct ) 
	{
		ASCategoryTree *ct = *pct ;
		if( ct ) 
		{
			int i ; 
			if( ct->dir_list ) 
			{	
				for( i = 0 ; i < ct->dir_num ; ++i ) 
					destroy_string( &(ct->dir_list[i]) );
				free( ct->dir_list );
			}					 
			destroy_string( &(ct->name));
			destroy_string( &(ct->icon_path)); 
			
			destroy_ashash( &(ct->entries));
			destroy_ashash( &(ct->categories));
	
			memset( ct, 0x00, sizeof(ASCategoryTree));
			free(ct);
			*pct = NULL ; 			   
		}	 
		
	}			   
}	  

void
add_category_tree_subtree( ASCategoryTree* ct, ASCategoryTree* subtree )
{
	ASHashIterator i;

	if( ct == NULL || subtree == NULL ) 
		return ;

	if( start_hash_iteration ( subtree->entries, &i) )
	{
		do
		{
		 	ASDesktopEntry *de = curr_hash_data( &i );
			if( add_hash_item( ct->entries, AS_HASHABLE(de->Name), de) == ASH_Success ) 
				ref_desktop_entry( de );  
				 
		}while( next_hash_item( &i ) );
	}	 
	if( start_hash_iteration ( subtree->categories, &i) )
	{
		do
		{
			char *alias = (char*)curr_hash_value( &i );
		 	ASDesktopCategory *subtree_dc = curr_hash_data( &i );
			ASDesktopCategory *dc = fetch_desktop_category( ct, subtree_dc->name );
			if( dc == NULL ) 
			{	
		 		dc = create_desktop_category( subtree_dc->index_name );
				if( subtree_dc->name != NULL ) 
					dc->name = mystrdup(subtree_dc->name);
				LOCAL_DEBUG_OUT( "creating category using index_name \"%s\" and name \"%s\"", dc->index_name, dc->name?dc->name:"(null)" );
				if( add_hash_item( ct->categories, AS_HASHABLE(dc->index_name), dc) != ASH_Success ) 
				{	
					unref_desktop_category( dc );
					dc = NULL ; 
				}else if( dc->name )
					add_hash_item( ct->categories, AS_HASHABLE(dc->name), dc);
			}
			if( dc ) 
			{
				if( mystrcasecmp( alias, dc->index_name ) != 0 ) 
				{
					if( dc->name && mystrcasecmp( alias, dc->name ) != 0 ) 
					{
						char *tmp = mystrdup(alias);	  
						if( add_hash_item( ct->categories, AS_HASHABLE(tmp), dc) != ASH_Success ) 
							free( tmp );
					}
				}	 					   
			}	 
			if( dc ) 
			{	
				char **subtree_entries = PVECTOR_HEAD(char*,subtree_dc->entries);
				int num = PVECTOR_USED(subtree_dc->entries);
				int i ; 
				for( i = 0 ; i < num ; ++i )
				{
					ASDesktopCategory *sub_dc = fetch_desktop_category( ct, subtree_entries[i] );
					if( sub_dc ) 
						add_desktop_category_entry( dc, sub_dc->index_name );
					else
						add_desktop_category_entry( dc, subtree_entries[i] );
				}
			}				 
		}while( next_hash_item( &i ) );
	}	 
}	 

void 
print_category_tree( ASCategoryTree* ct )
{
	fprintf(stderr, "Printing category_tree %p :\n", ct );
	if( ct ) 
	{
	 	fprintf(stderr, "category_tree.flags=0x%lX;\n", ct->flags );		
		fprintf(stderr, "category_tree.name=\"%s\";\n", ct->name?ct->name:"(null)" );
		fprintf(stderr, "category_tree.icon_path=\"%s\";\n", ct->icon_path?ct->icon_path:"(null)" );
		fprintf(stderr, "category_tree.entries_num=%ld;\n", ct->entries->items_num );
		print_ashash2(ct->entries, desktop_entry_print );
		fprintf(stderr, "category_tree.categories_num=%ld;\n", ct->categories->items_num );
		print_ashash2(ct->categories, desktop_category_print );
	}		  
	
}	 

void 
print_category_subtree( ASCategoryTree* ct, const char *entry_name, int level )
{
	ASDesktopCategory *dc = fetch_desktop_category( ct, entry_name );
	if( dc && level < 40 ) 
	{	
		char **entries = PVECTOR_HEAD(char*,dc->entries);
		int num = PVECTOR_USED(dc->entries);
		int i, k ; 
		++level ; 
		for( i = 0 ; i < num ; ++i ) 
		{
			ASDesktopCategory *sub_dc = fetch_desktop_category( ct, entries[i] );
			if( sub_dc == dc ) 
			{	
				fprintf( stderr, "%5.5d:reccurent refference to self \"%s\"!!! ", level, entries[i] );
				continue;
			}
			fprintf( stderr, "%5.5d:", level );
			for( k = 0 ; k < level ; ++k ) 
				fputc('\t', stderr );
			if( sub_dc ) 
			{
				if( sub_dc->name == NULL ) 
					fprintf( stderr, "C: %s\n", sub_dc->index_name );
				else 
					fprintf( stderr, "C: %s(%s)\n", sub_dc->name, sub_dc->index_name );
			}else
				fprintf( stderr, "E: %s\n", entries[i] );
			if( sub_dc ) 
				print_category_subtree( ct, entries[i], level );
		}	 
	}
}

void 
print_category_tree2( ASCategoryTree* ct )
{
	fprintf(stderr, "Printing category_tree %p :\n", ct );
	if( ct ) 
	{
	 	fprintf(stderr, "category_tree.flags=0x%lX;\n", ct->flags );		
		fprintf(stderr, "category_tree.name=\"%s\";\n", ct->name?ct->name:"(null)" );
		fprintf(stderr, "category_tree.icon_path=\"%s\";\n", ct->icon_path?ct->icon_path:"(null)" );
		fprintf(stderr, "category_tree.entries_num=%ld;\n", ct->entries->items_num );
		fprintf(stderr, "category_tree.categories_num=%ld;\n", ct->categories->items_num );
		print_category_subtree( ct, DEFAULT_DESKTOP_CATEGORY_NAME, 0 );
	}		  
	
}	 

/****************** /public **********************/


