#ifndef ASSHAPE_HEADER_FILE_INCLUDED
#define ASSHAPE_HEADER_FILE_INCLUDED

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */


#ifdef __cplusplus
extern "C" {
#endif

ASVector *create_shape();
void destroy_shape( ASVector **shape );

Bool add_shape_rectangles( ASVector *shape, XRectangle *rects, unsigned int count, int x_origin, int y_origin, unsigned int max_width, unsigned int max_height );
Bool print_shape( ASVector *shape );
Bool add_shape_mask( ASVector *shape, ASImage *mask_im );
Bool subtract_shape_rectangle( ASVector *shape, XRectangle *rect, unsigned int count, int x_origin, int y_origin, unsigned int clip_width, unsigned int clip_height );
Bool apply_shape_to_window( ASVector *shape, Window w );
Bool query_shape_from_window( ASVector *shape, Window w );

#ifdef __cplusplus
}
#endif


#endif /* #ifndef ASHAPE_HEADER_FILE_INCLUDED */
