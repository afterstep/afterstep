#ifndef XIMAGE_UTILS_HEADER_FILE_INCLUDED
#define XIMAGE_UTILS_HEADER_FILE_INCLUDED

#ifndef NO_XIMAGE_PROTO
unsigned long GetXImageDataSize (XImage * ximage);
XImage *CreateXImageBySample (XImage * ximage, unsigned int width, unsigned int height);
XImage *CreateXImageAndData (register Display * dpy,
			     register Visual * visual,
			     unsigned int depth,
			     int format,
			     int offset,	/*How many pixels from the start of the data does the
						   picture to be transmitted start? */
			     unsigned int width,
			     unsigned int height);

void CopyXImageLine (XImage * src, XImage * trg, int src_y, int trg_y);

void Scale (int *target, int target_size,
	    int src_start, int src_end);
XImage *ScaleXImageToSize (XImage * src, int width, int height);
XImage *ScaleXImage (XImage * src, double scale_x, double scale_y);
#endif

typedef struct _ShadingInfo
  {
    XColor tintColor;
    unsigned int shading;
  }
ShadingInfo;

#define NO_NEED_TO_SHADE(o) (o.shading==100 && o.tintColor.red==0xFFFF && o.tintColor.green==0xFFFF && o.tintColor.blue == 0xFFFF)
#define INIT_SHADING(o)     {o.shading=100; o.tintColor.red=0xFFFF; o.tintColor.green=0xFFFF; o.tintColor.blue = 0xFFFF;}
#define INIT_SHADING2(o,c)     {o.shading=100; o.tintColor.red=c.red; o.tintColor.green=c.green; o.tintColor.blue = c.blue;}
#define SET_SHADING_COLOR(o,c)   {o.tintColor.red=c.red; o.tintColor.green=c.green; o.tintColor.blue = c.blue;}

void ShadeXImage (XImage * srcImage,	/* at the end returns shaded XImage */
		  ShadingInfo * shading, GC gc);
int CombinePixmapWithXImage (Pixmap dest, XImage * src, int x, int y, int width, int height);
void CombineXImageWithXImage (XImage * dest, XImage * src);

#endif
