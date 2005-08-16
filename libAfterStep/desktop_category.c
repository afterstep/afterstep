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

void
desktop_category_destroy(ASHashableValue value, void *data)
{
	ASDesktopCategory *dc = (ASDesktopCategory*)data;
	destroy_desktop_category( &dc );	
}

void 
add_desktop_category_entry( ASDesktopCategory *dc, const char *entry_name )
{
	if( dc && entry_name ) 
	{
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
		fprintf( stderr, "category(%p).name=\"%s\";\n", dc, dc->name );
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
fetch_category( ASCategoryTree *ct, const char *cname )
{
	void *tmp = NULL;
	
	if( cname == NULL ) 
		return ct->default_category;
	
	if( get_hash_item( ct->categories, AS_HASHABLE(cname), &tmp ) == ASH_Success )
		return (ASDesktopCategory*)tmp ;
	
	return NULL;	   
}


ASDesktopCategory *
obtain_category( ASCategoryTree *ct, const char *cname )
{
	ASDesktopCategory *dc = fetch_category( ct, cname );
	if( dc == NULL ) 		
	{
		dc = create_desktop_category( cname );
		if( add_hash_item( ct->categories, AS_HASHABLE(cname), dc) != ASH_Success ) 
			destroy_desktop_category( &dc );
		else if( mystrcasecmp(cname, DEFAULT_DESKTOP_CATEGORY_NAME) != 0 ) 
			add_desktop_category_entry( ct->default_category, cname );
	}	 
	return dc;
}	 

Bool register_desktop_entry(ASCategoryTree *ct, ASDesktopEntry *de)
{
	int i ; 
	if( de && de->Name ) 
	{	
		if( add_hash_item( ct->entries, AS_HASHABLE(de->Name), de) == ASH_Success ) 
			ref_desktop_entry( de );  
		if( de->type == ASDE_TypeDirectory ) 
		{
			void *tmp = NULL;
			if( de->categories_num == 0 || 
				mystrcasecmp(de->categories_shortcuts[0], DEFAULT_DESKTOP_CATEGORY_NAME ) == 0 )
			{
				add_desktop_category_entry( ct->default_category, de->Name );
			}else if( get_hash_item( ct->categories, AS_HASHABLE(de->Name), &tmp ) == ASH_Success )
			{
				remove_desktop_category_entry( ct->default_category, de->Name );
			}	 
		}	 

		for( i = 0 ; i < de->categories_num ; ++i ) 
		{
			ASDesktopCategory *dc = obtain_category( ct, de->categories_shortcuts[i] ); 

			if( dc ) 
				add_desktop_category_entry( dc, de->Name );
		}	
		return True;
	}
	return False;
}	 

ASCategoryTree*
create_category_tree( const char *name, const char *path, const char *dirname, const char *icon_path, ASFlagType flags, int max_depth )	
{
	ASCategoryTree *ct = safecalloc( 1, sizeof(ASCategoryTree));
	ct->max_depth = max_depth ; 
	ct->flags = flags ; 
	if( path || dirname )
		ct->origin = make_file_name( path, dirname );
	ct->name = mystrdup(name);
	ct->icon_path = mystrdup(icon_path);
	ct->entries = create_ashash( 0, string_hash_value, string_compare, desktop_entry_destroy );
	ct->categories = create_ashash( 0, casestring_hash_value, casestring_compare, desktop_category_destroy );
	ct->default_category = obtain_category( ct, DEFAULT_DESKTOP_CATEGORY_NAME );	
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
			destroy_string( &(ct->origin) );
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
		 	ASDesktopCategory *subtree_dc = curr_hash_data( &i );
			ASDesktopCategory *dc = obtain_category( ct, subtree_dc->name );
			if( dc ) 
			{	
				char **subtree_entries = PVECTOR_HEAD(char*,subtree_dc->entries);
				int num = PVECTOR_USED(subtree_dc->entries);
				while ( --num >= 0 )
				{
					add_desktop_category_entry( dc, subtree_entries[num] );
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
		fprintf(stderr, "category_tree.origin=\"%s\";\n", ct->origin?ct->origin:"(null)" );
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
	ASDesktopCategory *dc = fetch_category( ct, entry_name );
	if( dc && level < 40 ) 
	{	
		char **entries = PVECTOR_HEAD(char*,dc->entries);
		int num = PVECTOR_USED(dc->entries);
		int i, k ; 
		++level ; 
		for( i = 0 ; i < num ; ++i ) 
		{
			fprintf( stderr, "%5.5d:", level );
			for( k = 0 ; k < level ; ++k ) 
				fputc('\t', stderr );
			fprintf( stderr, "%s\n", entries[i] );
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
		fprintf(stderr, "category_tree.origin=\"%s\";\n", ct->origin?ct->origin:"(null)" );
		fprintf(stderr, "category_tree.name=\"%s\";\n", ct->name?ct->name:"(null)" );
		fprintf(stderr, "category_tree.icon_path=\"%s\";\n", ct->icon_path?ct->icon_path:"(null)" );
		fprintf(stderr, "category_tree.entries_num=%ld;\n", ct->entries->items_num );
		fprintf(stderr, "category_tree.categories_num=%ld;\n", ct->categories->items_num );
		print_category_subtree( ct, DEFAULT_DESKTOP_CATEGORY_NAME, 0 );
	}		  
	
}	 

/****************** /public **********************/


