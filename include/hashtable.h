#ifndef HASHTABLE_H_
#define HASHTABLE_H_


typedef struct HashEntry
  {
    struct HashEntry *nptr, *pptr;	/* linked list */
    unsigned long key;
    unsigned long data;
    struct HashEntry *next;	/* next on the linked-list for 
				 * collisions */
  }
HashEntry;


/*
 * A generic hash table structure
 */
typedef struct HashTable
  {
    int elements;		/* elements stored in the table */
    int size;			/* size of the table */
    HashEntry **table;
    HashEntry *last;		/* last on the linked list */
  }
HashTable;



HashTable *table_init (HashTable * table);
void table_idestroy (HashTable * table);
void table_iput (HashTable * table, unsigned long key, unsigned long data);
int table_iget (HashTable * table, unsigned long key, unsigned long *data);
void table_idelete (HashTable * table, unsigned long key);


#endif
