#ifndef BLENDER_H_HEADER_INCLUDED
#define BLENDER_H_HEADER_INCLUDED

struct ASScanline;

/* it produces  bottom = bottom <merge> top */
typedef void (*merge_scanlines_func)( struct ASScanline *bottom, struct ASScanline *top, int mode);

/* colorspace conversion functions : 											 */

inline CARD32 rgb2value( CARD32 red, CARD32 green, CARD32 blue );
inline CARD32 rgb2saturation( CARD32 red, CARD32 green, CARD32 blue );
inline CARD32 rgb2hue( CARD32 red, CARD32 green, CARD32 blue );
inline CARD32 rgb2hsv( CARD32 red, CARD32 green, CARD32 blue, CARD32 *saturation, CARD32 *value );
inline void hsv2rgb (CARD32 hue, CARD32 saturation, CARD32 value, CARD32 *red, CARD32 *green, CARD32 *blue);
inline CARD32 rgb2luminance (CARD32 red, CARD32 green, CARD32 blue );
inline CARD32 rgb2hls (CARD32 red, CARD32 green, CARD32 blue, CARD32 *luminance, CARD32 *saturation );
inline void hls2rgb (CARD32 hue, CARD32 luminance, CARD32 saturation, CARD32 *red, CARD32 *green, CARD32 *blue);

/* scanline blending 													 */

void alphablend_scanlines( struct ASScanline *bottom, struct ASScanline *top, int unused );
void allanon_scanlines( struct ASScanline *bottom, struct ASScanline *top, int unused );
void tint_scanlines( struct ASScanline *bottom, struct ASScanline *top, int unused );
void add_scanlines( struct ASScanline *bottom, struct ASScanline *top, int unused );
/* substruction with saturation : */
void sub_scanlines( struct ASScanline *bottom, struct ASScanline *top, int unused );
/* absolute pixel value difference : */
void diff_scanlines( struct ASScanline *bottom, struct ASScanline *top, int unused );
/* darkest of the two makes it in : */
void darken_scanlines( struct ASScanline *bottom, struct ASScanline *top, int unused );
/* lightest of the two makes it in : */
void lighten_scanlines( struct ASScanline *bottom, struct ASScanline *top, int unused );
/* guess what this one does - I could not :) */
void screen_scanlines( struct ASScanline *bottom, struct ASScanline *top, int unused );
/* somehow overlays bottom with top : */
void overlay_scanlines( struct ASScanline *bottom, struct ASScanline *top, int unused );
void hue_scanlines( struct ASScanline *bottom, struct ASScanline *top, int mode );
void saturate_scanlines( struct ASScanline *bottom, struct ASScanline *top, int mode );
void value_scanlines( struct ASScanline *bottom, struct ASScanline *top, int mode );
void colorize_scanlines( struct ASScanline *bottom, struct ASScanline *top, int mode );
void dissipate_scanlines( struct ASScanline *bottom, struct ASScanline *top, int unused );

#endif             /* BLENDER_H_HEADER_INCLUDED */

