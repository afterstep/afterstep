#ifndef SAFEMALLOC_H_HEADER_INCLUDED
#define SAFEMALLOC_H_HEADER_INCLUDED

void         *safemalloc (size_t length);
void         *safecalloc (size_t num, size_t blength);
void		  safefree (void *ptr);
void		  dump_memory();

#endif