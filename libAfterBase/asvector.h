#ifndef ASVECTOR_HEADER_FILE_INCLUDED
#define ASVECTOR_HEADER_FILE_INCLUDED

typedef struct ASVector
{
    void *memory ;
    size_t allocated, used ;
    size_t unit ;
}ASVector;

#define DECL_VECTOR(t,v)    ASVector v = {NULL,0,0,sizeof(t)}
#define DECL_VECTOR_RAW(v)  ASVector v = {NULL,0,0,1}
#define VECTOR_HEAD(t,v)    ((t*)((v).memory))
#define VECTOR_HEAD_RAW(v)  ((v).memory)
#define VECTOR_TAIL(t,v)    ((t*)(((v).memory) + ((v).used)))
#define VECTOR_TAIL_RAW(v)  (((v).memory) + ((v).used))
#define VECTOR_USED(v)      ((v).used)

/* return memory starting address : */
void *alloc_vector( ASVector *v, size_t new_size );
void *realloc_vector( ASVector *v, size_t new_size );
/* returns self : */
ASVector *append_vector( ASVector *v, void * data, size_t size );

/* returns index on success, -1 on failure */
int vector_insert_elem( ASVector *v, void *data, size_t size, void *sibling, int before );
int vector_find_elem( ASVector *v, void *data );
/* returns 1 on success, 0 on failure */
int vector_remove_elem( ASVector *v, void *data );
int vector_remove_index( ASVector *v, size_t index );

void
print_vector( stream_func func, void* stream, ASVector *v, char *name, void (*print_func)( stream_func func, void* stream, void *data ) );


/* returns nothing : */
void flush_vector( ASVector *v ); /* reset used to 0 */
void free_vector( ASVector *v );  /* free all memory used, except for object itself */

#endif /* #ifndef ASVECTOR_HEADER_FILE_INCLUDED */
