#ifndef ASHASH_HEADER_FILE_INCLUDED
#define ASHASH_HEADER_FILE_INCLUDED
/* REALY USEFULL and UNIVERSAL hash table implementation */

typedef union {
    unsigned long  long_val ;
    char 	  *string_val;
}ASHashableValue ; 

typedef struct ASHashItem
{
    struct ASHashItem 	*next ;
    ASHashableValue    	 value ;
    void 		*data ; /* optional data structure related to this 
				   hashable value */
}ASHashItem ;

typedef unsigned int ASHashKey ;
typedef ASHashItem* ASHashBucket ;

typedef struct ASHashTable
{
    ASHashKey 	size ;
    ASHashBucket *buckets ;
    ASHashKey	buckets_used ;
    unsigned long items_num ;
    
    ASHashKey 	(*hash_func)(ASHashableValue value, ASHashKey hash_size);
    long      	(*compare_func)(ASHashableValue value1, ASHashableValue value2);
    void 	(*item_destroy_func)(ASHashableValue value, void *data);
}ASHashTable;

typedef struct ASHashIterator
{
    ASHashKey  	 curr_bucket ;
    ASHashItem  *curr_item ;
    ASHashTable *hash ;
}ASHashIterator;

void init_ashash( ASHashTable *hash, Bool freeresources );
/* Note that all parameters are optional here. 
   If it is set to NULL - defaults will be used */

#define DEFAULT_HASH_SIZE 51 /* random value - not too big - not too small */
/* default hash_func is long_val%hash_size */
/* default compare_func is long_val1-long_val2 */

ASHashTable *create_ashash( ASHashKey size, 
                            ASHashKey (*hash_func)(ASHashableValue, ASHashKey), 
			    long      (*compare_func)(ASHashableValue, ASHashableValue),
			    void (*item_destroy_func)(ASHashableValue, void *));
void print_ashash( ASHashTable *hash, void (*item_print_func)(ASHashableValue value));
void destroy_ashash( ASHashTable **hash );

typedef enum {

    ASH_BadParameter 	= -3,
    ASH_ItemNotExists   = -2,
    ASH_ItemExistsDiffer= -1,
    ASH_ItemExistsSame 	=  0,
    ASH_Success		=  1
}ASHashResult ;

ASHashResult add_hash_item( ASHashTable *hash, ASHashableValue value, void *data );
/* Note that trg parameter is optional here */
ASHashResult get_hash_item( ASHashTable *hash, ASHashableValue value, void **trg );
/* here if trg is NULL then both data and value will be destroyed;
   otherwise only value will be destroyed.
   Note: if you never specifyed destroy_func  - nothing will be destroyed,
         except for HashItem itself.
 */
ASHashResult remove_hash_item( ASHashTable *hash, ASHashableValue value, void**trg, Bool destroy);

unsigned long sort_hash_items( ASHashTable *hash, ASHashableValue *values, void **data, unsigned long max_items );

Bool start_hash_iteration( ASHashTable *hash, ASHashIterator *iterator );
Bool next_hash_item( ASHashIterator *iterator );
ASHashableValue curr_hash_value( ASHashIterator *iterator );
void* curr_hash_data( ASHashIterator *iterator );


/**************************************************************************/
/**************************************************************************/
/* here is the set of implemented hash functions : */
/**************************************************************************/

/* configuration options - case unsensitive and spaces are not alowed */
ASHashKey option_hash_value (ASHashableValue value, ASHashKey hash_size);

/* case unsensitive strings  - spaces and control chars are alowed */
ASHashKey casestring_hash_value (ASHashableValue value, ASHashKey hash_size);

/* case sensitive strings  - spaces and control chars are alowed */
ASHashKey string_hash_value (ASHashableValue value, ASHashKey hash_size);
long string_compare(  ASHashableValue value1, ASHashableValue value2 );
void string_destroy(ASHashableValue value, void * data );
void string_print(ASHashableValue value);

/* basically any long value, but was written originally for colors hash */
ASHashKey color_hash_value(ASHashableValue value, ASHashKey hash_size);

#endif
