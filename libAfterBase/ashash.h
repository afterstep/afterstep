#ifndef ASHASH_HEADER_FILE_INCLUDED
#define ASHASH_HEADER_FILE_INCLUDED
/* REALY USEFULL and UNIVERSAL hash table implementation */

#include "astypes.h"

#ifdef __cplusplus
extern "C" {
#endif


struct wild_reg_exp;

#if 0
typedef union ASHashableValue
{
  unsigned long 	   long_val;
  char 				  *string_val;
  struct wild_reg_exp *wrexp_val;	/* regular expression */
  void 				  *ptr ;
}ASHashableValue;
#else
typedef unsigned long ASHashableValue;
#endif

typedef union ASHashData
{
 	void  *vptr ;
 	int   *iptr ;
 	unsigned int   *uiptr ;
 	long  *lptr ;
 	unsigned long   *ulptr ;
	char  *cptr ;
	int    i ;
	unsigned int ui ;
	long   l ;
	unsigned long ul ;
	CARD32 c32 ;
	CARD16 c16 ;
	CARD8  c8 ;
}ASHashData;


#define AS_HASHABLE(v)  ((ASHashableValue)((unsigned long)(v)))

typedef struct ASHashItem
{
  struct ASHashItem *next;
  ASHashableValue value;
  void *data;			/* optional data structure related to this
				   hashable value */
}
ASHashItem;

typedef unsigned short ASHashKey;
typedef ASHashItem *ASHashBucket;

typedef struct ASHashTable
{
  ASHashKey size;
  ASHashBucket *buckets;
  ASHashKey buckets_used;
  unsigned long items_num;

  ASHashItem *most_recent ;

    ASHashKey (*hash_func) (ASHashableValue value, ASHashKey hash_size);
  long (*compare_func) (ASHashableValue value1, ASHashableValue value2);
  void (*item_destroy_func) (ASHashableValue value, void *data);
}
ASHashTable;

typedef struct ASHashIterator
{
  ASHashKey curr_bucket;
  ASHashItem **curr_item;
  ASHashTable *hash;
}
ASHashIterator;

void init_ashash (ASHashTable * hash, Bool freeresources);
/* Note that all parameters are optional here.
   If it is set to NULL - defaults will be used */

#define DEFAULT_HASH_SIZE 51	/* random value - not too big - not too small */
/* default hash_func is long_val%hash_size */
/* default compare_func is long_val1-long_val2 */

ASHashTable *create_ashash (ASHashKey size,
                ASHashKey (*hash_func) (ASHashableValue,ASHashKey),
                long (*compare_func) (ASHashableValue,  ASHashableValue),
                void (*item_destroy_func) (ASHashableValue,void *));
void print_ashash (ASHashTable * hash,
		   void (*item_print_func) (ASHashableValue value));
void destroy_ashash (ASHashTable ** hash);

typedef enum
{

  ASH_BadParameter = -3,
  ASH_ItemNotExists = -2,
  ASH_ItemExistsDiffer = -1,
  ASH_ItemExistsSame = 0,
  ASH_Success = 1
}
ASHashResult;

ASHashResult add_hash_item (ASHashTable * hash, ASHashableValue value,
			    void *data);
/* Note that trg parameter is optional here */
ASHashResult get_hash_item (ASHashTable * hash, ASHashableValue value,
			    void **trg);
/* here if trg is NULL then both data and value will be destroyed;
   otherwise only value will be destroyed.
   Note: if you never specifyed destroy_func  - nothing will be destroyed,
         except for HashItem itself.
 */
ASHashResult remove_hash_item (ASHashTable * hash, ASHashableValue value,
			       void **trg, Bool destroy);

/* removes all the items from hash table */
void flush_ashash (ASHashTable * hash);

/* need to run this in order to free up cached memory */
void flush_ashash_memory_pool();

/* if max_items == 0 then all hash items will be returned */
unsigned long sort_hash_items (ASHashTable * hash, ASHashableValue * values,
			       void **data, unsigned long max_items);
unsigned long list_hash_items (ASHashTable * hash, ASHashableValue * values,
			       void **data, unsigned long max_items);


Bool start_hash_iteration (ASHashTable * hash, ASHashIterator * iterator);
Bool next_hash_item (ASHashIterator * iterator);
inline ASHashableValue curr_hash_value (ASHashIterator * iterator);
inline void *curr_hash_data (ASHashIterator * iterator);
void remove_curr_hash_item (ASHashIterator * iterator, Bool destroy);


/**************************************************************************/
/**************************************************************************/
/* here is the set of implemented hash functions : */
/**************************************************************************/

ASHashKey pointer_hash_value (ASHashableValue value, ASHashKey hash_size);


long
desc_long_compare_func (ASHashableValue value1, ASHashableValue value2);

/* configuration options - case unsensitive and spaces are not alowed */
ASHashKey option_hash_value (ASHashableValue value, ASHashKey hash_size);
long option_compare (ASHashableValue value1, ASHashableValue value2);

/* case unsensitive strings  - spaces and control chars are alowed */
ASHashKey casestring_hash_value (ASHashableValue value, ASHashKey hash_size);
long casestring_compare (ASHashableValue value1, ASHashableValue value2);

/* case sensitive strings  - spaces and control chars are alowed */
ASHashKey string_hash_value (ASHashableValue value, ASHashKey hash_size);
long string_compare (ASHashableValue value1, ASHashableValue value2);
void string_destroy (ASHashableValue value, void *data);
void string_destroy_without_data (ASHashableValue value, void *data);
void string_print (ASHashableValue value);

/* basically any long value, but was written originally for colors hash */
ASHashKey color_hash_value (ASHashableValue value, ASHashKey hash_size);

#ifdef __cplusplus
}
#endif

#endif
