#ifndef BLENDER_H_HEADER_INCLUDED
#define BLENDER_H_HEADER_INCLUDED

/****h* libAfterImage/blender.h
 * DESCRIPTION
 * Defines implemented methods for ASScanline combining, that could
 * be passed to merge_layers() via ASImageLayer structure.
 * Also includes functions for colorspace conversion RGB<->HSV and
 * RGB<->HLS.
 * SEE ALSO
 * Functions :
 *    Colorspace conversion :
 *          rgb2value(), rgb2saturation(), rgb2hue(), rgb2luminance(),
 *          rgb2hsv(), rgb2hls(), hsv2rgb(), hls2rgb().
 *
 *    merge_scanline methods :
 *          alphablend_scanlines(), allanon_scanlines(),
 *          tint_scanlines(), add_scanlines(), sub_scanlines(),
 *          diff_scanlines(), darken_scanlines(), lighten_scanlines(),
 *          screen_scanlines(), overlay_scanlines(), hue_scanlines(),
 *          saturate_scanlines(), value_scanlines(),
 *          colorize_scanlines(), dissipate_scanlines().
 *
 *    usefull merging function name to function translator :
 *          blend_scanlines_name2func()
 *
 * Other libAfterImage modules :
 *     asimage.h, asvisual.h, import.h, asfont.h
 * AUTHOR
 * Sasha Vasko <sashav at sprintmail dot com>
 ******************/


struct ASScanline;

/* it produces  bottom = bottom <merge> top */
typedef void (*merge_scanlines_func)( struct ASScanline *bottom, struct ASScanline *top, int mode);

/****d* libAfterImage/import/colorspace
 * DESCRIPTION
 * RGB colorspace: each color is represented as a combination of
 * red, green and blue values. Each value can be in 2 formats :
 * 8 bit and 24.8 bit. 24.8 bit makes for 32bit value with lower 8 bits
 * used as a fraction for better calculation precision.
 *
 * HSV colorspace: each color is represented as a combination of
 * hue, saturation and value. Hue is generally colorizing component where
 * value represents brightness.
 *
 * HLS colorspace: each color is represented as a combination of
 * hue, luminance and saturation. It is analogous to HSV with value
 * substituted by luminance, except that luminance could be both
 * negative and positive.
 *
 * alpha channel could be added to any of the above colorspaces. alpha
 * channel is generally used to define transparentness of the color.
 * libAfterImage is using ARGB colorspace as a base colorspace, and
 * represents most colors as ARGB32 values or ASScanline scanlines of
 * pixels.
 ****************/
/****f* libAfterImage/import/rgb2value()
 * SYNOPSIS
 * CARD32 rgb2value( CARD32 red, CARD32 green, CARD32 blue );
 * CARD32 rgb2saturation( CARD32 red, CARD32 green, CARD32 blue );
 * CARD32 rgb2hue( CARD32 red, CARD32 green, CARD32 blue );
 * CARD32 rgb2luminance (CARD32 red, CARD32 green, CARD32 blue );
 * INPUTS
 * red   - 32 bit value, 16 lower bits of which represent red channel
 * green - 32 bit value, 16 lower bits of which represent green channel
 * blue  - 32 bit value, 16 lower bits of which represent blue channel
 * RETURN VALUE
 * 32 bit value, 16 lower bits of which represent value, saturation, hue,
 * or luminance respectively.
 * DESCRIPTION
 * This functions translate RGB color into respective coordinates of
 * HSV and HLS colorspaces.
 ****************/
inline CARD32 rgb2value( CARD32 red, CARD32 green, CARD32 blue );
inline CARD32 rgb2saturation( CARD32 red, CARD32 green, CARD32 blue );
inline CARD32 rgb2hue( CARD32 red, CARD32 green, CARD32 blue );
inline CARD32 rgb2luminance (CARD32 red, CARD32 green, CARD32 blue );
/****f* libAfterImage/import/rgb2hsv()
 * SYNOPSIS
 * CARD32 rgb2hsv( CARD32 red, CARD32 green, CARD32 blue,
 *                 CARD32 *saturation, CARD32 *value );
 * CARD32 rgb2hls( CARD32 red, CARD32 green, CARD32 blue,
 *                 CARD32 *luminance, CARD32 *saturation );
 * INPUTS
 * red   - 32 bit value, 16 lower bits of which represent red channel
 * green - 32 bit value, 16 lower bits of which represent green channel
 * blue  - 32 bit value, 16 lower bits of which represent blue channel
 * RETURN VALUE
 * 32 bit value, 16 lower bits of which represent hue.
 * 32bit value pointed to by luminance, value and saturation will be set
 * respectively to color luminance, value and saturation.
 * DESCRIPTION
 * This functions translate RGB color into full set of HSV and HLS
 * coordinates at once. These functions work faster then separate
 * translation into each channel.
 ****************/
inline CARD32 rgb2hsv( CARD32 red, CARD32 green, CARD32 blue, CARD32 *saturation, CARD32 *value );
inline CARD32 rgb2hls (CARD32 red, CARD32 green, CARD32 blue, CARD32 *luminance, CARD32 *saturation );
/****f* libAfterImage/import/hsv2rgb()
 * SYNOPSIS
 * void hsv2rgb( CARD32 hue, CARD32 saturation, CARD32 value,
 *               CARD32 *red, CARD32 *green, CARD32 *blue);
 * void hls2rgb( CARD32 hue, CARD32 luminance, CARD32 saturation,
 *               CARD32 *red, CARD32 *green, CARD32 *blue);
 * INPUTS
 * hue        - 32 bit value, 16 lower bits of which represent hue.
 * saturation - 32 bit value, 16 lower bits of which represent saturation.
 * value      - 32 bit value, 16 lower bits of which represent value.
 * luminance  - 32 bit value, 16 lower bits of which represent luminance.
 * RETURN VALUE
 * 32bit value pointed to by red, green and blue will be set
 * respectively to RGB color channels.
 * DESCRIPTION
 * This functions performs reverse translation from HSV and HSL to
 * RGB color
 ****************/
inline void hsv2rgb (CARD32 hue, CARD32 saturation, CARD32 value, CARD32 *red, CARD32 *green, CARD32 *blue);
inline void hls2rgb (CARD32 hue, CARD32 luminance, CARD32 saturation, CARD32 *red, CARD32 *green, CARD32 *blue);

/* scanline blending 													 */
/****f* libAfterImage/import/merge_scanline
 * SYNOPSIS
 * void alphablend_scanlines( ASScanline *bottom, ASScanline *top, int );
 * void allanon_scanlines   ( ASScanline *bottom, ASScanline *top, int );
 * void tint_scanlines      ( ASScanline *bottom, ASScanline *top, int );
 * void add_scanlines       ( ASScanline *bottom, ASScanline *top, int );
 * void sub_scanlines       ( ASScanline *bottom, ASScanline *top, int );
 * void diff_scanlines      ( ASScanline *bottom, ASScanline *top, int );
 * void darken_scanlines    ( ASScanline *bottom, ASScanline *top, int );
 * void lighten_scanlines   ( ASScanline *bottom, ASScanline *top, int );
 * void screen_scanlines    ( ASScanline *bottom, ASScanline *top, int );
 * void overlay_scanlines   ( ASScanline *bottom, ASScanline *top, int );
 * void hue_scanlines       ( ASScanline *bottom, ASScanline *top, int );
 * void saturate_scanlines  ( ASScanline *bottom, ASScanline *top, int );
 * void value_scanlines     ( ASScanline *bottom, ASScanline *top, int );
 * void colorize_scanlines  ( ASScanline *bottom, ASScanline *top, int );
 * void dissipate_scanlines ( ASScanline *bottom, ASScanline *top, int );
 * INPUTS
 * bottom   - pointer to the ASScanline that will be overalayed
 * top      - pointer to ASScanline that will overlay bottom.
 * DESCRIPTION
 * This functions accept 2 scanlines as an arguments stored in
 * ASScanline structures with data in 24.8 format. Merging operation is
 * performed on these scanlines and result is stored in bottom
 * ASScanline.
 * The following are merging methods used in each function :
 *
 * alphablend_scanlines - combines top and bottom RGB components based
 *                        on alpha channel value:
 *                        bottom = bottom*(255-top_alpha)+top*top_alpha;
 * allanon_scanlines    - averages each pixel between two scanlines.
 *                        This method has been first implemented by
 *                        Ethan Fisher aka allanon as mode 130:
 *                        bottom = (bottom+top)/2;
 * tint_scanlines       - tints bottom scanline with top scanline( with
 *                        saturation to prevent overflow) :
 *                        bottom = (bottom*(top/2))/32768;
 * add_scanlines        - adds top scanline to bottom scanline with
 *                        saturation to prevent overflow:
 *                        bottom = bottom+top;
 * sub_scanlines        - substrates top scanline from bottom scanline
 *                        with saturation to prevent overflow:
 *                        bottom = bottom-top;
 * diff_scanlines       - for each pixel calculates absolute difference
 *                        between bottom and top color value :
 *                        bottom = (bottom>top)?bottom-top:top-bottom;
 * darken_scanlines     - substitutes each pixel with minimum color
 *                        value of top and bottom :
 *                        bottom = (bottom>top)?top:bottom;
 * lighten_scanlines    - substitutes each pixel with maximum color
 *                        value of top and bottom :
 *                        bottom = (bottom>top)?bottom:top;
 * screen_scanlines     - some wierd merging algorithm taken from GIMP;
 * overlay_scanlines    - some wierd merging algorithm taken from GIMP;
 * hue_scanlines        - substitute hue of bottom scanline with hue of
 *                        top scanline;
 * saturate_scanlines   - substitute saturation of bottom scanline with
 *                        the saturation of top scanline;
 * value_scanlines      - substitute value of bottom scanline with
 *                        the value of top scanline;
 * colorize_scanlines   - combine luminance of bottom scanline with hue
 *                        and saturation of top scanline;
 * dissipate_scanlines  - randomly alpha-blend bottom and top scanlines,
 *                        using alpha value of top scanline as a
 *                        threshold for random values.
 ****************/
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

/****f* libAfterImage/import/blend_scanlines_name2func()
 * SYNOPSIS
 * merge_scanlines_func blend_scanlines_name2func( const char *name );
 * INPUTS
 * name - string, identifying scanline merging function.
 * RETURN VALUE
 * returns pointer to the scanline merging function on succes.
 * NULL on failure.
 * DESCRIPTION
 * blend_scanlines_name2func() will strip leading whitespaces off of
 * the supplied name, and then will attempt to match it against the list
 * of names of merging functions. It will then return pointer to the 
 * function with matching name.
 ****************/
merge_scanlines_func blend_scanlines_name2func( const char *name );

#endif             /* BLENDER_H_HEADER_INCLUDED */

