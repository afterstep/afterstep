#ifndef ASTYPES_H_HEADER_INCLUDED
#define ASTYPES_H_HEADER_INCLUDED

#ifndef XMD_H
#include <X11/Xmd.h>
#endif
#ifndef Bool
#define Bool int
#endif
#ifndef True
#define True 1
#endif
#ifndef False
#define False 0
#endif

typedef unsigned long ASFlagType ;
#define ASFLAGS_EVERYTHING  0xFFFFFFFF
typedef ASFlagType ASFlagsXref[5];

#define get_flags(var, val) 	((var) & (val))
#define set_flags(var, val) 	((var) |= (val))
#define clear_flags(var, val) 	((var) &= ~(val))
#define CheckSetFlag(b,f,v) 	{if((b)) (f) |= (v) ; else (f) &= ~(v);}


#endif

