#ifndef DESKTOP_CATEGORY_HEADER_INCLUDED
#define DESKTOP_CATEGORY_HEADER_INCLUDED

struct ASVector;
struct ASHashTable;

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
	int ref_count ;

#define ASDE_NoDisplay			(0x01<<0)	  
#define ASDE_Hidden				(0x01<<1)	  
#define ASDE_Terminal			(0x01<<2)	  
#define ASDE_StartupNotify		(0x01<<3)	  
#define ASDE_EncodingUTF8		(0x01<<4)	  
#define ASDE_KDE				(0x01<<5)	  
#define ASDE_GNOME				(0x01<<6)	  
#define ASDE_ASModule			(0x01<<7)	  
#define ASDE_CheckAvailability	(0x01<<8)	  
#define ASDE_Unavailable		(0x01<<9)  /* calculated if CheckAvailability is set */ 	  
	ASFlagType flags ; 

#define ASDE_KEYWORD_NoDisplay_LEN			9	
#define ASDE_KEYWORD_Hidden_LEN				6
#define ASDE_KEYWORD_Terminal_LEN			8
#define ASDE_KEYWORD_StartupNotify_LEN		13
#define ASDE_KEYWORD_ASModule_LEN			8
#define ASDE_KEYWORD_CheckAvailability_LEN	17
	
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

	/* AfterStep extentions : */
	char *IndexName ;
#define ASDE_KEYWORD_IndexName_LEN	    9
	char *Aliases ;
#define ASDE_KEYWORD_Aliases_LEN		7

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

	char *origin ;

	int aliases_len ; 
	char **aliases_shortcuts ; 
	int aliases_num ; 

}ASDesktopEntry;

typedef struct ASDesktopCategory
{
	int ref_count ;

#define DEFAULT_DESKTOP_CATEGORY_NAME	"Default"

	char *index_name ;
	char *name ;
	struct ASVector *entries ;
}ASDesktopCategory;

typedef struct ASCategoryTree
{
	ASFlagType flags ;
/* all entries must belong to category bearing the name of this tree :*/
#define ASCT_ConstrainCategory			(0x01<<0)	  
/* self-explanatory : */
#define ASCT_ExcludeGNOME				(0x01<<1)	  
#define ASCT_ExcludeKDE					(0x01<<2)	  
#define ASCT_OnlyGNOME					(0x01<<3)	  
#define ASCT_OnlyKDE					(0x01<<4)	  

	char *name ;
	char *icon_path;
	int max_depth ; 

	char  **dir_list ; 
	int 	dir_num ; 

	/*ASBiDirList *entry_list ;*/
	ASDesktopCategory  *default_category ;
	struct ASHashTable *categories ;		  
	struct ASHashTable *entries ;		  
}ASCategoryTree;

ASDesktopCategory *create_desktop_category( const char *name );
int ref_desktop_category( ASDesktopCategory *dc ); 
int unref_desktop_category( ASDesktopCategory *dc ); 

void add_desktop_category_entry( ASDesktopCategory *dc, const char *entry_name );
void print_desktop_category( ASDesktopCategory *dc );
char *make_desktop_entry_categories( ASDesktopEntry *de );
void save_desktop_entry( ASDesktopEntry* de, FILE *fp );


ASDesktopEntry *fetch_desktop_entry( ASCategoryTree *ct, const char *name );
ASDesktopCategory *fetch_desktop_category( ASCategoryTree *ct, const char *cname );
Bool desktop_entry_belongs_to( ASCategoryTree *ct, ASDesktopEntry *de, ASDesktopEntry *category_de );

Bool dup_desktop_entry_Name( ASDesktopEntry* de, char **trg );
Bool dup_desktop_entry_Comment( ASDesktopEntry* de, char **trg );

/*************************************************************************/
/* Desktop Entry functionality                                           */
/*************************************************************************/

ASDesktopEntry *create_desktop_entry( ASDesktopEntryTypes default_type);
int ref_desktop_entry( ASDesktopEntry *de );
int unref_desktop_entry( ASDesktopEntry *de ); 
void print_desktop_entry( ASDesktopEntry* de );

/*************************************************************************
 * Desktop Category Tree functionality : 
 *************************************************************************/

ASCategoryTree*create_category_tree( const char *name, const char *path, const char *icon_path, ASFlagType flags, int max_depth );	
void destroy_category_tree( ASCategoryTree **pct );
Bool register_desktop_entry(ASCategoryTree *ct, ASDesktopEntry *de);
void add_category_tree_subtree( ASCategoryTree* ct, ASCategoryTree* subtree );
void print_category_tree( ASCategoryTree* ct );
void print_category_tree2( ASCategoryTree* ct, ASDesktopCategory *dc );
void save_category_tree( ASCategoryTree* ct, FILE *fp );

#endif
