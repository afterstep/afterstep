#ifndef ASTYPES_H_HEADER_INCLUDED
#define ASTYPES_H_HEADER_INCLUDED

#include "xwrap.h"

#ifndef ABS
#define ABS(a)              ((a)>0   ? (a) : -(a))
#endif
#ifndef SGN
#define SGN(a)              ((a)>0   ?  1  : ((a)<0 ? -1 : 0))
#endif
#ifndef MIN
#define MIN(a,b)            ((a)<(b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b)            ((a)>(b) ? (a) : (b))
#endif

#ifndef max
#define max(x,y)            (((x)>=(y))?(x):(y))
#endif

#ifndef min
#define min(x,y)            (((x)<=(y))?(x):(y))
#endif

#define FIT_IN_RANGE(from,val,to)  (((val)<(from))?(from):(((val) >(to))?(to):(val)))
#define FIT_SIZE_IN_RANGE(from,val,size,to)  (((val)<(from))?(from):(((val)+(size) >(to))?(to)-(size):(val)))

/* same as above actually */
#define AS_CLAMP(a,b,c)        ((a)<(b) ? (b) : ((a)>(c) ? (c) : (a)))
#define AS_CLAMP_SIZE(a,b,c)   ((a)<(b) ? (b) : ((c)!=-1&&(a)>(c) ? (c) : (a)))
#define SWAP(a, b, type)    { type SWAP_NaMe = a; a = b; b = SWAP_NaMe; }

typedef unsigned long ASFlagType ;
#define ASFLAGS_EVERYTHING  0xFFFFFFFF
typedef ASFlagType ASFlagsXref[5];

#define get_flags(var, val) 	((var) & (val))
#define set_flags(var, val) 	((var) |= (val))
#define clear_flags(var, val) 	((var) &= ~(val))
#define CheckSetFlag(b,f,v) 	{if((b)) (f) |= (v) ; else (f) &= ~(v);}

typedef struct ASMagic
{ /* just so we can safely cast void* to query magic number :*/
    unsigned long magic ;
}ASMagic;

#endif

