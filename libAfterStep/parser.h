#ifndef PARSER_HEADER_FILE_INCLUDED
#define PARSER_HEADER_FILE_INCLUDED

/*#define DEBUG_PARSER
*/

/* put it in here for now - later should probably be moved
   into configure stuff */
#define WITH_CONFIG_WRITER

#include "afterstep.h"
#include "freestor.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SyntaxDef;
struct ASHashTable;

typedef struct TermDef
{
  /* the following needs to be defined by module */
#define TF_DONT_REMOVE_COMMENTS (1<<26)	/* indicates that comments sould not be */
  /* removed from the end of this term */
#define TF_SPECIAL_PROCESSING 	(1<<27)	/* supplied special processing procedure */
  /* needs to be called for this one */
#define TF_NO_MYNAME_PREPENDING	(1<<28)	/* has effect only when writing this back to */
  /* config file */
#define TF_DONT_SPLIT           (1<<29)	/* if it is desired to preserv data intact */
  /* vs. splitting it into space separated words */
#define TF_INDEXED              (1<<30)	/* if we have array of indexed items of the same */
  /* type and ID */
#define TF_SYNTAX_TERMINATOR    (1<<31)	/* will tell parser that reading of config */
  /* in context of current syntax completed */
  /* for example ~MyStyle ends MyStyle syntax */
  /* reading */
#define TF_NAMED_SUBCONFIG	(1<<25)	/* first token is used as the name */
  /* rest of the line gets parsed using subsyntax */
  /* usefull in things like database where you have: Style "name" <blah,blah,blah> */
  /* <blah,blah,blah> in that case in this case will be parsed using the subsyntax */
  /* and will be placed in the subtree of FreeStorageElems */
#define TF_OBSOLETE		(1<<24)	/* to enable error notification */
  /* for obsolete options */
#define TF_DEPRECIATED		(1<<23)	/* to enable filtering out and preprocessing */
  /* for depreciated options */
  /* this options should not be supported by ASCP */
  /* and must be filtered out and converted into */
  /* respective non-depreciated options by libASConfig */
#define TF_DIRECTION_INDEXED (1<<22) /* North,South, East,West, NorthWest, NorthEast, SouthWest, SouthEast as index */
#define TF_NONUNIQUE 		 (1<<21) /* Ther could be several options of this type in config */


  unsigned long flags;		/* combination of any of above values */
  char *keyword;
  unsigned int keyword_len;
#define TT_ANY		0
#define TT_FLAG		1
#define TT_INTEGER  	2
#define TT_UINTEGER  	3
#define TT_COLOR    	4
#define TT_FONT    	5
#define TT_FILENAME 	6
#define TT_PATHNAME 	7
#define TT_GEOMETRY 	8
#define TT_TEXT     	9
#define TT_SPECIAL     10
#define TT_QUOTED_TEXT 11
#define TT_FUNCTION    12
#define TT_BOX	       13	/* like for IconBox : left top right bottom */
#define TT_BUTTON      14	/* BUTTON DEFINITION */
#define TT_BINDING     15	/* key or mouse binding */
#define TT_BITLIST     16	/* comma or space separated list of bit numbers to set */
#define TT_INTARRAY    17	/* array of the integer numbers */
#define TT_CURSOR      18   /* ASCursor - pair of filenames */
#define TT_OPTIONAL_PATHNAME 19   /* optional quoted text - could be empty */
#define TT_DIRECTION   20   /* North,South, East,West, NorthWest, NorthEast, SouthWest, SouthEast */

#define TT_CUSTOM      64	/* modules can define custom types starting */
  /* with this one */
  int type;			/* term's type */

/* that is done for some global config id's */
  /* all module's custom ID's should start from here */
#define ID_ANY			0
#define TT_CUSTOM_ID_START    	1024
  int id;			/* term's id */
  struct SyntaxDef *sub_syntax;	/* points to the SyntaxDef structure
					   of the syntax of complicated
					   construct like PagerDecoration or
					   MyStyle,
					   NULL if term has simple structure
					 */
}
TermDef;

#define TERM_HASH_SIZE      61
typedef unsigned short int HashValue;


typedef struct SyntaxDef
{
  /* user initialized members */
  char terminator;		/* character, terminating single term */
  char file_terminator;		/* character, terminating configuration */
  TermDef *terms;		/* array of Term definitions */
  HashValue term_hash_size;	/* defaults to TERM_HASH_SIZE if set to 0 */

  /* writing beautification and error message members: */
  char token_separator;
  /* all of the following must not be NULL */
  char *prepend_one;		/* sequence of character to prepend single term with */
  char *prepend_sub;		/* sequence of character to prepend whole config with */
  char *display_name;		/* will be in error message */
  char *doc_path;
  char *display_purpose;    /* purpose of what is identifyed by display_name */
  	  
  /* generated members */
  struct ASHashTable *term_hash;	/* hash table for fast term search */
  int recursion;		/* prevents endless recursion in nested constructs */
}SyntaxDef;

typedef struct SyntaxStack
{
  SyntaxDef *syntax;
  struct SyntaxStack *next;
  TermDef *current_term;
  unsigned long current_flags;
}SyntaxStack;


/* that is not supposed to be used outside of this code */
/* making it available just in case */
#define DISABLED_KEYWORD        "#~~DISABLED~~#"
#define DISABLED_KEYWORD_SIZE	14

#define MAX_CONFIG_LINE 	MAXLINELENGTH
#define NORMAL_CONFIG_LINE 	128

struct StorageStack;

typedef struct FilePtrAndData
{
    FILE *fp ;
    char *data ;                               /* prefetched line from the above file ! */
}FilePtrAndData;

typedef struct ConfigDef
{
  char *myname;			/* prog name */

  SpecialFunc special;
  int fd;
  FILE *fp;			/* this one for compatibility with some old code - most
				 * notably balloons and MyStyle
				 * when they'll be converted on new style - that should go away
				 */

#define CP_NeedToCloseFile   (0x01<<0)
#define CP_ReadLines         (0x01<<1)
  ASFlagType flags ;
  /* allocated to store lines read from the file */
  char *buffer;
  long buffer_size;
  long bytes_in;
  /* this is the current parsing information */
  char *tline, *tline_start, *tdata;
  TermDef *current_term;
  SyntaxStack *current_syntax;
  SyntaxDef *syntax;		/* for easier handling only */
  struct StorageStack *current_tail;
  char *current_prepend;
  int current_prepend_size, current_prepend_allocated;
  char *current_data;
  long current_data_size;
  int current_data_len;

#define CF_NONE			0
#define CF_DISABLED_OPTION	(1<<0)	/* option has #~~DISABLE~~# prepending it */
#define CF_PUBLIC_OPTION	(1<<1)	/* public options - with no * MyName prepending it */
#define CF_FOREIGN_OPTION	(1<<2)	/* option had * prepending it, but unknown MyName after that */
#define CF_LAST_OPTION	(1<<3)	/* option is last in the config file */
#define CF_COMMENTED_OPTION	(1<<4)	/* option is last in the config file */

#define IsOptionEnabled(config)	(!(config->current_flags&CF_DISABLED_OPTION))
#define IsOptionDisabled(config)	(config->current_flags&CF_DISABLED_OPTION)
#define IsPublicOption(config)	(config->current_flags&CF_PUBLIC_OPTION)
#define IsPrivateOption(config)	(!(config->current_flags&CF_PUBLIC_OPTION))
#define IsMyOption(config)	(!(config->current_flags&CF_FOREIGN_OPTION))
#define IsForeignOption(config)	(config->current_flags&CF_FOREIGN_OPTION)
#define IsLastOption(config)	(config->current_flags&CF_LAST_OPTION)
  unsigned long current_flags;

  char *cursor;
  int line_count;

}
ConfigDef;

typedef enum
{
  CDT_Filename,
  CDT_FilePtr,
  CDT_FileDesc,
  CDT_Data,
  CDT_FilePtrAndData
}
ConfigDataType;

typedef union
{
	void *vptr ;
	const char *filename;
	FILE *fileptr ;
	int *filedesc ;
	char *data ;
	FilePtrAndData *fileptranddata ;
}ConfigData ;

void register_keyword_id( const char *keyword, int id );
const char* keyword_id2keyword( int id );	

void BuildHash (SyntaxDef * syntax);
void PrepareSyntax (SyntaxDef * syntax);
void FreeSyntaxHash (SyntaxDef * syntax);

TermDef *FindTerm (SyntaxDef * syntax, int type, int id);
TermDef *FindStatementTerm (char *tline, SyntaxDef * syntax);

int PopSyntax (ConfigDef * config);
int PopStorage (ConfigDef * config);
char *GetNextStatement (ConfigDef * config, int my_only);

ConfigDef *InitConfigReader (char *myname, SyntaxDef * syntax,
			     ConfigDataType type, ConfigData source,
			     SpecialFunc special);
int ParseConfig (ConfigDef * config, FreeStorageElem ** tail);
FreeStorageElem *file2free_storage(const char *filename, char *myname, SyntaxDef *syntax, FreeStorageElem **foreign_options );


ConfigDef *InitConfigWriter (char *myname, SyntaxDef * syntax,
			     ConfigDataType type, ConfigData  source);

#define WF_DISCARD_NONE   	0
#define WF_DISCARD_PUBLIC	(1<<1)
#define WF_DISCARD_FOREIGN	(1<<2)
#define WF_DISCARD_COMMENTS 	(1<<3)
#define WF_DISCARD_UNKNOWN	(1<<4)
#define WF_DISCARD_EVERYTHING   0xFFFFFFFF
long WriteConfig (ConfigDef * config, FreeStorageElem ** storage,
		  ConfigDataType target_type, ConfigData *target,
		  unsigned long flags);
/* Note: WriteConfig discards FreeStorage if everything is fine,
 *       in which case *storage will be set to NULL
 */

void DestroyConfig (ConfigDef * config);

/* debugging stuff */
#ifdef DEBUG_PARSER
void PrintConfigReader (ConfigDef * config);
void PrintFreeStorage (FreeStorageElem * storage);
#else
#define PrintConfigReader(a)
#define PrintFreeStorage(b)
#endif

#define COMMENTS_CHAR '#'
#define MYNAME_CHAR   '*'

#ifdef __cplusplus
}
#endif


#endif
