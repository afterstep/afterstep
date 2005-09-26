#ifndef FREESTOR_H_HEADER_INCLUDED
#define FREESTOR_H_HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

struct SyntaxDef;
struct TermDef;
struct ASHashTable;
struct ConfigDef;
struct ASCursor ;

typedef struct FreeStorageElem
{
  struct TermDef *term;
  unsigned long flags;		/* see current_flags for possible values */

  char **argv;			/* space separated words from the source data will
				   be placed here unless DONT_SPLIT_WORDS defined
				   for the term */
  int argc;			/* number of words */
  struct FreeStorageElem *next;
  struct FreeStorageElem *sub;
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

typedef unsigned long (*SpecialFunc) (struct ConfigDef * conf_def,
				      FreeStorageElem ** storage);

#define SPECIAL_BREAK			(0x01<<0)
#define SPECIAL_SKIP			(0x01<<1)
#define SPECIAL_STORAGE_ADDED		(0x01<<2)

typedef struct StorageStack
{
  FreeStorageElem **tail;
  struct StorageStack *next;	/* to enable stackable treatment in case of sub_syntax */
}
StorageStack;


void init_asgeometry (ASGeometry * geometry);

int parse_modifier( char *tline );
int parse_win_context (char *tline);
char *parse_modifier_str( char *tline, int *mods_ret );
char *parse_win_context_str (char *tline, int *ctxs_ret);

/* pelem must be preinitialized with pointer to particular term */
void args2FreeStorage (FreeStorageElem * pelem, char *data, int data_len);

/* utility functions for writing config */
void ReverseFreeStorageOrder (FreeStorageElem ** storage);
FreeStorageElem *DupFreeStorageElem (FreeStorageElem * source);
FreeStorageElem *AddFreeStorageElem (struct SyntaxDef * syntax,
				     FreeStorageElem ** tail, struct TermDef * pterm,
				     int id);
void CopyFreeStorage (FreeStorageElem ** to, FreeStorageElem * from);
void DestroyFreeStorage (FreeStorageElem ** storage);
void StorageCleanUp (FreeStorageElem ** storage,
		     FreeStorageElem ** garbadge_bin, unsigned long mask);

struct FunctionData;

/* freestorage post processing stuff */
struct FunctionData *free_storage2func (FreeStorageElem * stored, int *ppos);
char *free_storage2quoted_text (FreeStorageElem * stored, int *ppos);
int *free_storage2int_array (FreeStorageElem * stored, int *ppos);

#define SET_CONFIG_FLAG(flags,mask,val) do{set_flags(flags,val);set_flags(mask,val);}while(0)

typedef struct ConfigItem
{
  void *memory;			/* this one holds pointer to entire block of allocated memory */
  int ok_to_free;		/* must be set in order to free memory allocated before and
				   stored in [memory] member */
  int type;
  int index;			/* valid only for those that has TF_INDEXED set */
  union
  {
    ASGeometry geometry;
    long integer;
    Bool flag;
    struct
    {
      int size;
      int *array;
    }
    int_array;
    char *string;
    struct FunctionData *function;
    struct ASButton *button;
    ASBox box;
    struct
    {
        char *sym ;
        int context ;
        int mods ;
    }binding;
    struct ASCursor *cursor ;
  }
  data;
}ConfigItem;

typedef struct flag_options_xref
{
  unsigned long flag;
  int id_on, id_off;
}
flag_options_xref;

/* this functions return 1 on success - 0 otherwise */
int ReadConfigItem (ConfigItem * item, FreeStorageElem * stored);
/* indexed flags cannot be handled by ReadFlagItem - it will return 0
   for those - handle them manually using ReadConfigItem */
int ReadFlagItem (unsigned long *set_flags, unsigned long *flags,
		  FreeStorageElem * stored, flag_options_xref * xref);

/* really defined in libAfterBase/mystring.h (unsigned long*)*/
void set_string (char **target, char *string);
#define set_string_value(ptarget,string,pset_flags,flag) \
	do {set_string(ptarget,string); if(pset_flags)set_flags(*(pset_flags),(flag));}while(0)
#define set_scalar_value(ptarget,val,pset_flags,flag) \
	do {*(ptarget)=(val); if(pset_flags)set_flags(*(pset_flags),(flag));}while(0)
#define set_size_geometry(pw,ph,pitem,pset_flags,flag) \
	do { *(pw) = get_flags ((pitem)->data.geometry.flags, WidthValue)?(pitem)->data.geometry.width:0; \
		 *(ph) = get_flags ((pitem)->data.geometry.flags, HeightValue)?(pitem)->data.geometry.height:0; \
		 if(pset_flags)set_flags(*(pset_flags),(flag));}while(0)


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
char *CompressStringArray (int argc, char **argv);
char **DupStringArray (int argc, char **argv);
void AddStringToArray (int *argc, char ***argv, char *new_string);
#define REPLACE_STRING(str1,str2) {if(str1)free(str1);str1=str2;}

/* they all return pointer to the storage's tail */
FreeStorageElem **Flag2FreeStorage (struct SyntaxDef * syntax,
				    FreeStorageElem ** tail, int id);

/* you can add all your flags at once : */
FreeStorageElem **Flags2FreeStorage (struct SyntaxDef * syntax,
				     FreeStorageElem ** tail,
				     flag_options_xref * xref,
				     unsigned long set_flags,
				     unsigned long flags);

#define ADD_SET_FLAG(syntax,tail,flags,flag,id)	\
    ((get_flags((flags),(flag)))?Flag2FreeStorage((syntax),(tail),(id)):(tail))

FreeStorageElem **Integer2FreeStorage (struct SyntaxDef * syntax,
                       FreeStorageElem ** tail,
                       int *index, int value, int id);
FreeStorageElem **Strings2FreeStorage (struct SyntaxDef * syntax,
				       FreeStorageElem ** tail,
				       char **strings, unsigned int num,
				       int id);
#define String2FreeStorage(syntax,t,s,id) Strings2FreeStorage(syntax,t,&(s), 1, id)

FreeStorageElem **QuotedString2FreeStorage (struct SyntaxDef * syntax,
					    FreeStorageElem ** tail,
					    int *index, char *string, int id);
FreeStorageElem **StringArray2FreeStorage (struct SyntaxDef * syntax,
					   FreeStorageElem ** tail,
					   char **strings, int index1,
					   int index2, int id, char *iformat);
FreeStorageElem **Path2FreeStorage (struct SyntaxDef * syntax,
				    FreeStorageElem ** tail,
				    int *index, char *string, int id);
FreeStorageElem **Geometry2FreeStorage (struct SyntaxDef * syntax,
					FreeStorageElem ** tail,
					ASGeometry * geometry, int id);
FreeStorageElem **ASButton2FreeStorage (struct SyntaxDef * syntax,
					FreeStorageElem ** tail,
					int index, ASButton * b, int id);
FreeStorageElem **Box2FreeStorage (struct SyntaxDef * syntax,
				   FreeStorageElem ** tail,
				   ASBox * box, int id);
FreeStorageElem **Binding2FreeStorage (struct SyntaxDef * syntax,
                    FreeStorageElem ** tail,
                    char *sym, int context, int mods, int id);
FreeStorageElem **ASCursor2FreeStorage (struct SyntaxDef * syntax,
                    FreeStorageElem ** tail,
                    int index, struct ASCursor *c, int id);
FreeStorageElem **Bitlist2FreeStorage (struct SyntaxDef * syntax,
                    FreeStorageElem ** tail,
                    long bits, int id);
#ifdef __cplusplus
}
#endif


#endif
