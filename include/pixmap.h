#ifndef PIXMAP_H_INCLUDED
#define PIXMAP_H_INCLUDED

#ifndef NO_PIXMAP_PROTOS
void CopyAndShadeArea (Drawable src, Pixmap trg,
		       int x, int y, int w, int h,
		       int trg_x, int trg_y,
		       GC gc, ShadingInfo * shading);

void ShadeTiledPixmap (Pixmap src, Pixmap trg, int src_w, int src_h, int x, int y, int w, int h, GC gc, ShadingInfo * shading);
int FillPixmapWithTile (Pixmap pixmap, Pixmap tile, int x, int y, int width, int height, int tile_x, int tile_y);
Pixmap ShadePixmap (Pixmap src, int x, int y, int width, int height, GC gc, ShadingInfo * shading);
Pixmap ScalePixmap (Pixmap src, int src_w, int src_h, int width, int height, GC gc, ShadingInfo * shading);
Pixmap CenterPixmap (Pixmap src, int src_w, int src_h, int width, int height, GC gc, ShadingInfo * shading);
Pixmap GrowPixmap (Pixmap src, int src_w, int src_h, int width, int height, GC gc, ShadingInfo * shading);

int GetRootDimensions (int *width, int *height);
int GetWinPosition (Window w, int *x, int *y);

Pixmap CutWinPixmap (Window win, Drawable src, int src_w, int src_h, int width, int height, GC gc, ShadingInfo * shading);

Pixmap GetRootPixmap (Atom id);

int pixmap_error_handler (Display * dpy, XErrorEvent * error);
Pixmap ValidatePixmap (Pixmap p, int bSetHandler, int bTransparent, unsigned int *pWidth, unsigned int *pHeight);

int fill_with_darkened_background (Pixmap * pixmap, XColor color, int x, int y, int width, int height, int root_x, int root_y, int bDiscardOriginal);
int fill_with_pixmapped_background (Pixmap * pixmap, XImage * image, int x, int y, int width, int height, int root_x, int root_y, int bDiscardOriginal);
#endif

#endif
