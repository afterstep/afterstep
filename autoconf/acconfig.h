/* self diagnostic SIGSEGV handling (as opposed to dumping core) */
#undef HAVE_SIGSEGV_HANDLING

#undef SHAPE
#undef XSHMIMAGE
#undef HAVE_XINERAMA

/* look and feel options : */
#undef DIFFERENTLOOKNFEELFOREACHDESKTOP
#undef PAGER_BACKGROUND
#undef I18N
#undef NO_WARPPOINTERTOMENU
#undef NO_SAVEWINDOWS
#undef NO_TEXTURE
#undef NO_SHADE
#undef NO_VIRTUAL
#undef NO_SAVEUNDERS
#undef NO_WINDOWLIST
#undef NO_AVAILABILITYCHECK

/* set this to conserve some memory if you don't need antialiased text : */
#undef MODULE_REUSE_LOADED_FONT

/* debugging options */
#undef  DEBUG_ALLOCS
#undef  DEBUG_TRACE
#undef  DEBUG_TRACE_X

/* menu options : */
#undef	FIXED_DIR

/* needed only by libafterconf and libafterstep : */
#undef  HAVE_AFTERBASE
#undef  HAVE_AFTERIMAGE
#undef  HAVE_AFTERSTEP

@BOTTOM@

/*#include "configure.h"*/
