#ifndef ASSHAPE_HEADER_FILE_INCLUDED
#define ASSHAPE_HEADER_FILE_INCLUDED

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */


#ifdef __cplusplus
extern "C" {
#endif

struct ASVector;
struct ASImage;

ASVector *create_shape();
void destroy_shape( struct ASVector **shape );

Bool add_shape_rectangles( struct ASVector *shape, XRectangle *rects, unsigned int count, int x_origin, int y_origin, unsigned int max_width, unsigned int max_height );
Bool print_shape( struct ASVector *shape );
Bool add_shape_mask( struct ASVector *shape, struct ASImage *mask_im );
Bool subtract_shape_rectangle( struct ASVector *shape, XRectangle *rect, unsigned int count, int x_origin, int y_origin, unsigned int clip_width, unsigned int clip_height );
Bool apply_shape_to_window( struct ASVector *shape, Window w );
Bool query_shape_from_window( struct ASVector *shape, Window w );

#ifdef __cplusplus
}
#endif


#endif /* #ifndef ASHAPE_HEADER_FILE_INCLUDED */
