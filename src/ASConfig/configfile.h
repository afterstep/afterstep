#ifndef CONFIGFILE_H_HEADER_INCLUDED
#define CONFIGFILE_H_HEADER_INCLUDED

struct SyntaxDef;
struct FreeStorageElem;


/*************************************************************************/

typedef struct ASConfigFile {
	char *dirname ;
	char *filename ;
	char *myname ;
	
	char *fullname ;
	struct SyntaxDef *syntax ;

	Bool writeable ;

	struct FreeStorageElem *free_storage ;

}ASConfigFile;




typedef struct ASConfigFileInfo
{
	int config_file_id ; 
	char *session_file ; 
	Bool non_freeable ; 
	char *tmp_override_file ;
			  
}ASConfigFileInfo;

extern ASConfigFileInfo ConfigFilesInfo[];

/*************************************************************************/
void init_ConfigFileInfo();
const char* get_config_file_name( int config_id );
/*************************************************************************/
ASConfigFile *load_config_file(const char *dirname, const char *filename, const char *myname, struct SyntaxDef *syntax );
void destroy_config_file( ASConfigFile *ascf );
ASConfigFile *dup_config_file( ASConfigFile *src );

/*************************************************************************/

#endif                         /* CONFIGFILE_H_HEADER_INCLUDED */
