#ifndef SAFEMALLOC_H_HEADER_INCLUDED
#define SAFEMALLOC_H_HEADER_INCLUDED

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef safemalloc
void         *safemalloc (size_t length);
#endif
#ifndef safecalloc
void         *safecalloc (size_t num, size_t blength);
#endif
#ifndef safefree
void		  safefree (void *ptr);
#endif
#ifndef saferealloc
void         *saferealloc (void *orig_ptr, size_t length);
#endif

void        *guarded_malloc (size_t length);
void        *guarded_realloc (void *orig_ptr, size_t length);
void        *guarded_calloc (size_t num, size_t blength);
void		guarded_free (void *ptr);

void		  dump_memory();

#define	NEW(a)              	((a *)malloc(sizeof(a)))
#define	NEW_ARRAY_ZEROED(a, b)  ((a *)safecalloc(b, sizeof(a)))
#define	NEW_ARRAY(a, b)     	((a *)safemalloc(b*sizeof(a)))

#ifdef __cplusplus
}
#endif


#endif
