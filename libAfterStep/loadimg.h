#ifndef AFTERSTEP_IMAGE_FORMAT_SUPPORT_HEADER
#define AFTERSTEP_IMAGE_FORMAT_SUPPORT_HEADER

#ifdef LIBASIMAGE

/* all posiible image file formats */
#define F_UNKNOWN      0
#define F_XPM          1
#define F_JPEG         2
#define F_GIF          3
#define F_PNG          4
#define F_TIFF         5
#define F_PM           6
#define F_PBMRAW       7
#define F_PBMASCII     8
#define F_XBM          9
#define F_SUNRAS       10
#define F_BMP          11
#define F_PS           12
#define F_IRIS         13
#define F_AUTO         14
#define F_PCX          15
#define F_TARGA        16
#define F_PCD          17


#define SCREEN_GAMMA 1.0	/* default gamma correction value - */
/* it can be adjusted via $SCREEN_GAMMA env. variable */


typedef struct load_image_params
  {
    Display *m_dpy;
    Window m_w;
    unsigned long m_max_colors;
    int m_max_x, m_max_y;
    Visual *m_visual;
    unsigned int m_bytes_per_pixel;
    Colormap m_colormap;
    int m_depth;
    const char *m_realfilename;

    /* internal use : that will be initialized by LoadImage */
    XImage *m_pImage;
    XImage *m_pMaskImage;
    int m_width, m_height;
    GC m_gc;
    int *m_x_net, *m_y_net;

    /* Output */
    Pixmap m_Target;
    Pixmap m_Mask;

    double m_gamma;
    ASCOLOR *m_img_colormap;
    CARD8 *m_gamma_table;

  }
LImageParams;

typedef struct pixmap_ref
  {
    struct pixmap_ref *next;
    int refcount;
    char *name;
    Pixmap pixmap;
    Pixmap mask;
  }
pixmap_ref_t;

#define MASK_DEPTH 1

extern int CreateTarget (LImageParams * pParams);
extern int CreateMask (LImageParams * pParams);
extern void XImageToPixmap (LImageParams * pParams, XImage * pImage, Pixmap * pTarget);
extern void CheckImageSize (LImageParams * pParams, unsigned int real_width, unsigned int real_height);
#endif

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
