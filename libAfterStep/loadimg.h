#ifndef AFTERSTEP_IMAGE_FORMAT_SUPPORT_HEADER
#define AFTERSTEP_IMAGE_FORMAT_SUPPORT_HEADER

/* this can be used by application safely */
extern int bReportErrorIfTypeUnknown;

extern Pixmap LoadImageWithMask (Display * dpy, Window w, unsigned long max_colors, const char *realfilename, Pixmap * pMask);
extern Pixmap LoadImageWithMaskAndScale (Display * dpy, Window w, unsigned long max_colors, const char *realfilename, unsigned int to_width, unsigned int to_height, Pixmap * pMask);

#if defined(LOG_LOADIMG_CALLS) && defined(DEBUG_ALLOCS)
int l_UnloadImage (const char *, int, Pixmap);
int l_UnloadMask (const char *, int, Pixmap);
#define UnloadImage(a)	l_UnloadImage(__FUNCTION__,__LINE__,a)
#define UnloadMask(a)	l_UnloadMask(__FUNCTION__,__LINE__,a)
#else
int UnloadImage (Pixmap pixmap);
int UnloadMask (Pixmap mask);
#endif

int set_use_pixmap_ref (int on);
int pixmap_ref_purge (void);

#define LoadImage(dpy,w,max_colors,realfilename) LoadImageWithMask(dpy,w,max_colors,realfilename,NULL)
#define LoadImageAndScale(dpy,w,max_colors,realfilename,to_width,to_height) LoadImageWithMaskAndScale(dpy,w,max_colors,realfilename,to_width,to_height,NULL)


#endif
