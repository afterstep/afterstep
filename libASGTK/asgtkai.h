#ifndef ASGTKAI_H_HEADER_INCLUDED
#define ASGTKAI_H_HEADER_INCLUDED

/* convinience functions to integrate libAfterImage with gtk apps : */

GdkPixbuf *ASImage2GdkPixbuf( struct ASImage *im ); 
GdkPixbuf *solid_color2GdkPixbuf( ARGB32 argb, int width, int height );
ASImage *GdkPixbuf2ASImage( GdkPixbuf *pixbuf);


#endif  /*  ASTHUMBBOX_H_HEADER_INCLUDED  */
