#ifndef PARSER_HEADER_FILE_INCLUDED
#define PARSER_HEADER_FILE_INCLUDED

/*#define DEBUG_PARSER */
/* put it in here for now - later should probably be moved
   into configure stuff */
#define WITH_CONFIG_WRITER

struct syntax_definition;

typedef struct term_definition
  {
    /* the following needs to be defined by module */
#define TF_DONT_REMOVE_COMMENTS (1<<26)		/* indicates that comments sould not be */
    /* removed from the end of this term */
#define TF_SPECIAL_PROCESSING 	(1<<27)		/* supplied special processing procedure */
    /* needs to be called for this one */
#define TF_NO_MYNAME_PREPENDING	(1<<28)		/* has effect only when writing this back to */
    /* config file */
#define TF_DONT_SPLIT           (1<<29)		/* if it is desired to preserv data intact */
    /* vs. splitting it into space separated words */
#define TF_INDEXED              (1<<30)		/* if we have array of indexed items of the same */
    /* type and ID */
#define TF_SYNTAX_TERMINATOR    (1<<31)		/* will tell parser that reading of config */
    /* in context of current syntax completed */
    /* for example ~MyStyle ends MyStyle syntax */
    /* reading */

    unsigned long flags;	/* combination of any of above values */
    char *keyword;
    unsigned int keyword_len;
#define TT_ANY		0
#define TT_FLAG		1
#define TT_INTEGER  	2
#define TT_COLOR    	3
#define TT_FONT    	4
#define TT_FILENAME 	5
#define TT_PATHNAME 	6
#define TT_GEOMETRY 	7
#define TT_TEXT     	8
#define TT_SPECIAL     	9
#define TT_QUOTED_TEXT 10
#define TT_CUSTOM   	64	/* modules can define custom types starting */
    /* with this one */
    int type;			/* term's type */

/* that is done for some global config id's */
    /* all module's custom ID's should start from here */
#define ID_ANY			0
#define TT_CUSTOM_ID_START    	1024
    int id;			/* term's id */
    struct syntax_definition *sub_syntax;	/* points to the SyntaxDef structure 
						   of the syntax of complicated 
						   construct like PagerDecoration or 
						   MyStyle,
						   NULL if term has simple structure
						 */

    /* the following will be initialized automatically */
    struct term_definition *brother;	/* next term with the same hash key in hash table */
  }
TermDef;

typedef struct term_hash_table
  {
    int dummy;			/* remove this if/when this structure is ever used */
  }
TermHashTable;

#define TERM_HASH_SIZE 		61
typedef unsigned short int HashValue;


typedef struct syntax_definition
  {
    /* user initialized members */
    char terminator;		/* character, terminating single term */
    char file_terminator;	/* character, terminating configuration */
    TermDef *terms;		/* array of Term definitions */
    HashValue term_hash_size;	/* defaults to TERM_HASH_SIZE if set to 0 */
    /* generated members */
    TermDef **term_hash;	/* hash table for fast term search */
  }
SyntaxDef;

typedef struct syntax_stack
  {
    SyntaxDef *syntax;
    struct syntax_stack *next;
  }
SyntaxStack;

/* that is not supposed to be used outside of this code */
/* making it available just in case */
#define DISABLED_KEYWORD        "#~~DISABLED~~#"
#define DISABLED_KEYWORD_SIZE	14

typedef struct freestorage_elem
  {
    TermDef *term;
    unsigned long flags;	/* see current_flags for possible values */

    char **argv;		/* space separated words from the source data will
				   be placed here unless DONT_SPLIT_WORDS defined 
				   for the term */
    int argc;			/* number of words */
    struct freestorage_elem *next;
    struct freestorage_elem *sub;
    /* points to the chain of sub-elements, for representation of the complex
       config constructs.
       for example if following is encountered :
       MyStyle "some_style"
       BackPixmap "some_pixmap"
       ForeColor  blue
       ~MyStyle
       it will be read in to the following :
       ...->{MyStyle_term,"some_style", sub, next->}...      
       |
       ---------------------------------
       |
       V
       { BackPixmap_term,"some_pixmap", sub=NULL, next }
       |
       ---------------------------------------------
       |
       V                                        
       { ForeColor_term,"blue", sub=NULL, next=NULL }      
     */
  }
FreeStorageElem;

#define MAX_CONFIG_LINE 	MAXLINELENGTH
#define NORMAL_CONFIG_LINE 	128

struct config_definition;
typedef int (*SpecialFunc) (struct config_definition * conf_def, FreeStorageElem ** storage);

typedef struct storage_cursor
  {
    FreeStorageElem **tail;
    struct storage_cursor *next;	/* to enable stackable treatment in case of sub_syntax */
  }
StorageStack;

typedef struct config_definition
  {
    char *myname;		/* prog name */

    SpecialFunc special;
    int fd;
    FILE *fp;			/* this one for compatibility with some old code - most
				 * notably balloons and MyStyle 
				 * when they'll be converted on new style - that should go away
				 */
    int bNeedToCloseFile;
    /* allocated to store lines read from the file */
    char *buffer;
    long buffer_size;
    /* this is the current parsing information */
    char *tline, *tline_start, *tdata;
    TermDef *current_term;
    SyntaxStack *current_syntax;
    SyntaxDef *syntax;		/* for easier handling only */
    StorageStack *current_tail;
    char *current_data;
    long current_data_size;
    int current_data_len;

#define CF_NONE			0
#define CF_DISABLED_OPTION	(1<<0)	/* option has #~~DISABLE~~# prepending it */
#define CF_PUBLIC_OPTION	(1<<1)	/* public options - with no * MyName prepending it */
#define CF_FOREIGN_OPTION	(1<<2)	/* option had * prepending it, but unknown MyName after that */

#define IsOptionEnabled(config)	(!(config->current_flags&CF_DISABLED_OPTION))
#define IsOptionDisabled(config)	(config->current_flags&CF_DISABLED_OPTION)
#define IsPublicOption(config)	(config->current_flags&CF_PUBLIC_OPTION)
#define IsPrivateOption(config)	(!(config->current_flags&CF_PUBLIC_OPTION))
#define IsMyOption(config)	(!(config->current_flags&CF_FOREIGN_OPTION))
#define IsForeignOption(config)	(config->current_flags&CF_FOREIGN_OPTION)
    unsigned long current_flags;

    char *cursor;

  }
ConfigDef;

typedef enum
  {
    CDT_Filename,
    CDT_FilePtr,
    CDT_FileDesc,
    CDT_Data
  }
ConfigDataType;

HashValue GetHashValue (char *text, int hash_size);
void InitHash (SyntaxDef * syntax);
void BuildHash (SyntaxDef * syntax);
TermDef *FindTerm (SyntaxDef * syntax, int type, int id);
TermDef *FindStatementTerm (char *tline, SyntaxDef * syntax);


ConfigDef *InitConfigReader (char *myname, SyntaxDef * syntax, ConfigDataType type, void *source, SpecialFunc special);
int ParseConfig (ConfigDef * config, FreeStorageElem ** tail);

ConfigDef *InitConfigWriter (char *myname, SyntaxDef * syntax, ConfigDataType type, void *source);

#define WF_DISCARD_NONE   	0
#define WF_DISCARD_PUBLIC	(1<<1)
#define WF_DISCARD_FOREIGN	(1<<2)
#define WF_DISCARD_COMMENTS 	(1<<3)
#define WF_DISCARD_UNKNOWN	(1<<4)
#define WF_DISCARD_EVERYTHING   0xFFFFFFFF
long WriteConfig (ConfigDef * config, FreeStorageElem ** storage, ConfigDataType target_type, void **target, unsigned long flags);
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

/* utility functions for writing config */
void ReverseFreeStorageOrder (FreeStorageElem ** storage);
FreeStorageElem *DupFreeStorageElem (FreeStorageElem * source);
FreeStorageElem *AddFreeStorageElem (SyntaxDef * syntax, FreeStorageElem ** tail, TermDef * pterm, int id);
void CopyFreeStorage (FreeStorageElem ** to, FreeStorageElem * from);
void DestroyFreeStorage (FreeStorageElem ** storage);
void StorageCleanUp (FreeStorageElem ** storage, FreeStorageElem ** garbadge_bin, unsigned long mask);

/* freestorage post processing stuff */
typedef struct
  {
    void *memory;		/* this one holds pointer to entire block of allocated memory */
    int ok_to_free;		/* must be set in order to free memory allocated before and 
				   stored in [memory] member */
    int type;
    int index;			/* valid only for those that has TF_INDEXED set */
    union
      {
	MyGeometry geometry;
	long integer;
	char *string;
      }
    data;
  }
ConfigItem;
int ReadConfigItem (ConfigItem * item, FreeStorageElem * stored);

/* utility functions */
void FlushConfigBuffer (ConfigDef * config);
void InitMyGeometry (MyGeometry * geometry);

/* string array manipulation functions */
/* StringArray is an array of pointers to continuous block of memory, 
 * holding several zero terminated strings.
 * When such array is to be deallocated - only the first pointer from it needs to be
 * deallocated - that will deallocate entire storage.
 * and when it needs to be created - first pointer should be allocated entire block of memory
 * to hold all strings and terminating zeros
 */
char **CreateStringArray (size_t elem_num);
size_t GetStringArraySize (int argc, char **argv);
char **DupStringArray (int argc, char **argv);
void AddStringToArray (int *argc, char ***argv, char *new_string);
#define REPLACE_STRING(str1,str2) {if(str1)free(str1);str1=str2;}


/* they all return pointer to the storage's tail */
#define Flag2FreeStorage(syntax,tail,id) (&(AddFreeStorageElem(syntax,tail,NULL,id)->next))
FreeStorageElem **Integer2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, int value, int id);
FreeStorageElem **String2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, char *string, int id);
FreeStorageElem **Geometry2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, MyGeometry * geometry, int id);
FreeStorageElem **StringArray2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, char **strings, int index1, int index2, int id);

#endif
