#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include <libAfterImage/config.h>
#include <libAfterImage/afterbase.h>
#include <libAfterImage/afterimage.h>

/* set_output_threshold(0xffff); */

MODULE = AfterImage	PACKAGE = AfterImage	PREFIX = image_

PROTOTYPES: DISABLE

ASImageManager*
image_c_manager_create(path)
		char* path
	CODE:
		{
			char* path2 = copy_replace_envvar(getenv(ASIMAGE_PATH_ENVVAR));
			if (path && path[0]) {
				RETVAL = create_image_manager(NULL, SCREEN_GAMMA, path, path2, NULL);
			} else {
				RETVAL = create_image_manager(NULL, SCREEN_GAMMA, path2, NULL);
			}
		}
	OUTPUT:
		RETVAL

ASVisual*
image_c_visual_create()
	CODE:
		RETVAL = create_asvisual(NULL, 0, 0, NULL);
	OUTPUT:
		RETVAL

ASImage*
image_c_load(manager, filename)
		char*	          filename
		ASImageManager* manager
	CODE:
		RETVAL = get_asimage(manager, filename, ASFLAGS_EVERYTHING, 100);
	OUTPUT:
		RETVAL

int
image_c_width(image)
		ASImage* image
	CODE:
		RETVAL = image->width;
	OUTPUT:
		RETVAL

int
image_c_height(image)
		ASImage* image
	CODE:
		RETVAL = image->height;
	OUTPUT:
		RETVAL

ASImage*
image_c_parse_xml(manager, visual, xml_str)
		ASImageManager* manager
		ASVisual*       visual
		char*	          xml_str
	INIT:
	CODE:
		{
			RETVAL = compose_asimage_xml(visual, manager, NULL, xml_str, ASFLAGS_EVERYTHING, 20, 0, NULL);
		}
	OUTPUT:
		RETVAL

ASImage*
image_c_create(width, height)
		int width
		int height
	CODE:
		RETVAL = create_asimage(width, height, ASIMAGE_QUALITY_TOP);
	OUTPUT:
		RETVAL

ASImage*
image_c_gradient(visual, width, height, colors, offsets, angle)
		ASVisual* visual
		int       width
		int       height
		SV*       colors
		SV*       offsets
		int       angle
	INIT:
		I32 numcolors = 0;
		I32 numoffsets = 0;
		if (!SvROK(colors) || SvTYPE(SvRV(colors)) != SVt_PVAV
		    || (numcolors = av_len((AV *)SvRV(colors))) < 1) {
			XSRETURN_UNDEF;
		}
		if (!SvROK(offsets) || SvTYPE(SvRV(offsets)) != SVt_PVAV
		    || (numoffsets = av_len((AV *)SvRV(offsets))) < 1) {
			XSRETURN_UNDEF;
		}
		numcolors++;
		numoffsets++;
		numoffsets = numcolors = numoffsets < numcolors ? numoffsets : numcolors;
	CODE:
		{
			int n, reverse = 0;
			ASGradient gradient;
			angle = (angle % 360 + 360) % 360;
			if (angle > 360 * 15 / 16 || angle < 360 * 1 / 16) {
				gradient.type = GRADIENT_Left2Right;
			} else if (angle < 360 * 3 / 16) {
				gradient.type = GRADIENT_TopLeft2BottomRight;
			} else if (angle < 360 * 5 / 16) {
				gradient.type = GRADIENT_Top2Bottom;
			} else if (angle < 360 * 7 / 16) {
				gradient.type = GRADIENT_BottomLeft2TopRight; reverse = 1;
			} else if (angle < 360 * 9 / 16) {
				gradient.type = GRADIENT_Left2Right; reverse = 1;
			} else if (angle < 360 * 11 / 16) {
				gradient.type = GRADIENT_TopLeft2BottomRight; reverse = 1;
			} else if (angle < 360 * 13 / 16) {
				gradient.type = GRADIENT_Top2Bottom; reverse = 1;
			} else {
				gradient.type = GRADIENT_BottomLeft2TopRight;
			}
			gradient.color = safecalloc(numoffsets, sizeof(ARGB32));
			gradient.offset = NEW_ARRAY(double, numoffsets);
			gradient.npoints = numoffsets;
			for (n = 0 ; n < numoffsets ; n++) {
				char* color = SvPV_nolen(*av_fetch((AV *)SvRV(colors), n, 0));
				parse_argb_color(color, gradient.color + n);
				gradient.offset[n] = SvNV(*av_fetch((AV *)SvRV(offsets), n, 0));
			}
			if (reverse) {
				for (n = 0 ; n < gradient.npoints / 2 ; n++) {
					int i2 = gradient.npoints - 1 - n;
					ARGB32 c = gradient.color[n];
					double o = gradient.offset[n];
					gradient.color[n] = gradient.color[i2];
					gradient.color[i2] = c;
					gradient.offset[n] = 1.0 - gradient.offset[i2];
					gradient.offset[i2] = 1.0 - o;
				}
			}
			RETVAL = make_gradient(visual, &gradient, width, height, SCL_DO_ALL, ASA_ASImage, 0, ASIMAGE_QUALITY_TOP);
			free(gradient.color);
			free(gradient.offset);
		}
	OUTPUT:
		RETVAL

ASImage*
image_c_solid(visual, width, height, color)
		ASVisual* visual
		int       width
		int       height
		char*     color
	CODE:
		{
			ASImage* result = create_asimage(width, height, ASIMAGE_QUALITY_TOP);
			ARGB32 argb;
			parse_argb_color(color, &argb);
			if (result) fill_asimage(visual, result, 0, 0, width, height, argb);
			RETVAL = result;
		}
	OUTPUT:
		RETVAL

=item image_composite(visual, images, width, height, op, options)
visual:  (ASVisualPtr) Visual used for compositing.
images:  (arrayref of ASImagePtr) List of images to composite. 
         First image will be on top.
width:   (int) Width of resulting image.
height:  (int) Height of resulting image.
op:      (string) Image compositing operation.
options: (arrayref of hashref). Image options:
  x:           (int) Sub-image X coordinate on result image.
  y:           (int) Sub-image Y coordinate on result image.
  clip_x:      (int) Sub-image clipping rectangle X.
  clip_y:      (int) Sub-image clipping rectangle Y.
  clip_width:  (int) Sub-image clipping rectangle width.
  clip_height: (int) Sub-image clipping rectangle height.
  tint:        (string) Sub-image tint.
  op:          (string) Sub-image compositing operation.
Please see the libAfterImage documentation for more details.
=cut

ASImage*
image_c_composite(visual, images, width, height, op, options)
		ASVisual* visual
		SV*       images
		int       width
		int       height
		char*     op
		SV*       options
	INIT:
		int numimages;
		int numoptions = 0;
		if (!SvROK(images) || SvTYPE(SvRV(images)) != SVt_PVAV
		    || (numimages = av_len((AV *)SvRV(images))) < 0) {
			XSRETURN_UNDEF;
		}
		if (SvROK(options) && SvTYPE(SvRV(options)) == SVt_PVAV) {
			numoptions = av_len((AV *)SvRV(options)) + 1;
		}
		numimages++;
	CODE:
		{
			ASImage* result;
			ASImageLayer *layers = create_image_layers(numimages);
			int i;
			merge_scanlines_func op_func = blend_scanlines_name2func(op);
			if (!visual || !result || !layers || !op_func) XSRETURN_UNDEF;

			for (i = 0 ; i < numimages ; i++) {
				SV* imref = *av_fetch((AV *)SvRV(images), numimages - 1 - i, 0);
				if (sv_derived_from(imref, "ASImagePtr")) {
					IV tmp = SvIV(SvRV(imref));
					layers[i].im = INT2PTR(ASImage *, tmp);
				} else {
					Perl_croak(aTHX_ "image layer is not of type ASImagePtr");
				}
				layers[i].clip_width = layers[i].im->width;
				layers[i].clip_height = layers[i].im->height;
				layers[i].merge_scanlines = op_func;
				if (numoptions > numimages - 1 - i) {
					SV* opt_hashref = *av_fetch((AV *)SvRV(options), numimages - 1 - i, 0);
					HV* opt_hash;
					SV** tmp;
					if (!SvROK(opt_hashref) || SvTYPE(SvRV(opt_hashref)) != SVt_PVHV)
						Perl_croak(aTHX_ "image option is not of type HashRef");
					opt_hash = (HV*)SvRV(opt_hashref);
					tmp = hv_fetch(opt_hash, "x", 1, 0);
					if (tmp) layers[i].dst_x = SvIV(*tmp);
					tmp = hv_fetch(opt_hash, "y", 1, 0);
					if (tmp) layers[i].dst_y = SvIV(*tmp);
					tmp = hv_fetch(opt_hash, "clip_x", 6, 0);
					if (tmp) layers[i].clip_x = SvIV(*tmp);
					tmp = hv_fetch(opt_hash, "clip_y", 6, 0);
					if (tmp) layers[i].clip_y = SvIV(*tmp);
					tmp = hv_fetch(opt_hash, "clip_width", 10, 0);
					if (tmp) layers[i].clip_width = SvIV(*tmp);
					tmp = hv_fetch(opt_hash, "clip_height", 11, 0);
					if (tmp) layers[i].clip_height = SvIV(*tmp);
					tmp = hv_fetch(opt_hash, "tint", 4, 0);
					if (tmp) {
						ARGB32 argb;
						if (parse_argb_color(SvPV_nolen(*tmp), &argb))
							layers[i].tint = argb;
					}
					tmp = hv_fetch(opt_hash, "op", 2, 0);
					if (tmp) {
						merge_scanlines_func layer_op_func = blend_scanlines_name2func(SvPV_nolen(*tmp));
						if (layer_op_func) layers[i].merge_scanlines = layer_op_func;
					}
				}
			}
			if (!width) width = layers[numimages-1].clip_width;
			if (!height) height = layers[numimages-1].clip_height;

			RETVAL = merge_layers(
				visual, layers, numimages, width, height, ASA_ASImage, 0, 
				ASIMAGE_QUALITY_TOP
			);
		}
	OUTPUT:
		RETVAL

ASImage*
image_c_scale(visual, im, width, height)
		ASVisual* visual
		ASImage*  im
		int       width
		int       height
	CODE:
		RETVAL = scale_asimage(visual, im, width, height, ASA_ASImage, 0, ASIMAGE_QUALITY_TOP);
	OUTPUT:
		RETVAL

ASImage*
image_c_pad(visual, im, x, y, width, height)
		ASVisual* visual
		ASImage*  im
		int       x
		int       y
		int       width
		int       height
	CODE:
		RETVAL = pad_asimage(visual, im, x, y, width, height, 0, ASA_ASImage, 0, ASIMAGE_QUALITY_TOP);
	OUTPUT:
		RETVAL

ASImage*
image_c_tile(visual, im, x, y, width, height)
		ASVisual* visual
		ASImage*  im
		int       x
		int       y
		int       width
		int       height
	CODE:
		RETVAL = tile_asimage(visual, im, x, y, width, height, 0, ASA_ASImage, 0, ASIMAGE_QUALITY_TOP);
	OUTPUT:
		RETVAL

ASImage*
image_c_tint(visual, image, color)
		ASVisual* visual
		ASImage*  image
		char*     color
	CODE:
		{
			ARGB32 argb;
			parse_argb_color(color, &argb);
			RETVAL = tile_asimage(visual, image, 0, 0, image->width, image->height, argb, ASA_ASImage, 0, ASIMAGE_QUALITY_TOP);
		}
	OUTPUT:
		RETVAL

ASImage*
image_c_draw_text(visual, font, text, color, type_3d)
		ASVisual* visual
		ASFont*   font
		char*     text
		char*     color
		int       type_3d
	CODE:
		{
			ASTextAttributes attr = { ASTA_VERSION_INTERNAL, 0, 0, ASCT_Char, 8, 0, NULL, 0, ARGB32_White }; 
			ARGB32 argb;
			ASImage* text_color_image;
			ASImage* text_image;
			parse_argb_color(color, &argb);
			attr.char_type = ASCT_UTF8;
			attr.type = type_3d;
			text_image = draw_fancy_text(text, font, &attr, 0, 0);
			text_color_image = create_asimage(text_image->width, text_image->height, ASIMAGE_QUALITY_TOP);
			fill_asimage(visual, text_color_image, 0, 0, text_image->width, text_image->height, argb);
			move_asimage_channel(text_color_image, IC_ALPHA, text_image, IC_ALPHA);
			safe_asimage_destroy(text_image);
			RETVAL = text_color_image;
		}
	OUTPUT:
		RETVAL

ASImage*
image_c_gaussian_blur(visual, image, x_radius, y_radius)
		ASVisual* visual
		ASImage*  image
		double    x_radius
		double    y_radius
	CODE:
		RETVAL = blur_asimage_gauss(visual, image, x_radius, y_radius, SCL_DO_ALL, ASA_ASImage, 0, ASIMAGE_QUALITY_TOP);
	OUTPUT:
		RETVAL

=item image_c_rotate(visual, image, angle)
visual:  (ASVisualPtr) Visual used for compositing.
image:   (ASImagePtr) Image to rotate.
angle:   (double, degrees) Angle to rotate image by. Rotates clockwise.
         Currently supports angles of 90, 180, 270 only.
If angle is -45 <= angle < 45, then the returned image will be the same 
as the input image with an incremented refcount. In this case, the 
original image MUST be released and not re-used.
=cut
ASImage*
image_c_rotate(visual, image, angle)
		ASVisual* visual
		ASImage*  image
		double    angle
	CODE:
		{
			int int_angle = ((int)angle % 360 + 360 + 45) % 360 / 90 * 90;
			int width = image->width;
			int height = image->height;
			int flip_flags = 0;
			if (int_angle == 90 || int_angle == 270) {
				width = image->height;
				height = image->width;
			}
			switch (int_angle) {
				case  90: flip_flags = FLIP_VERTICAL; break;
				case 180: flip_flags = FLIP_UPSIDEDOWN; break;
				case 270: flip_flags = FLIP_VERTICAL | FLIP_UPSIDEDOWN; break;
				default:  flip_flags = 0;
			}
			if (flip_flags) {
				RETVAL = flip_asimage(visual, image, 0, 0, width, height, flip_flags, ASA_ASImage, 0, ASIMAGE_QUALITY_TOP);
			} else {
				image->ref_count++;
				RETVAL = image;
			}
		}
	OUTPUT:
		RETVAL

=item image_c_mirror(visual, image, vertical)
visual:   (ASVisualPtr) Visual used for compositing.
image:    (ASImagePtr) Image to rotate.
vertical: (int, bool) If true, mirror vertically. Else horizontally.
=cut
ASImage*
image_c_mirror(visual, image, vertical)
		ASVisual* visual
		ASImage*  image
		int       vertical
	CODE:
		RETVAL = mirror_asimage(visual, image, 0, 0, image->width, image->height, vertical, ASA_ASImage, 0, ASIMAGE_QUALITY_TOP);
	OUTPUT:
		RETVAL

int
image_c_draw_filled_rectangle(visual, im, x, y, width, height, color)
		ASVisual* visual
		ASImage*  im
		int       x
		int       y
		int       width
		int       height
		char*     color
	CODE:
		{
			ARGB32 argb;
			parse_argb_color(color, &argb);
			RETVAL = fill_asimage(visual, im, x, y, width, height, argb);
		}
	OUTPUT:
		RETVAL

int
image_c_clone(im)
		ASImage* im
	CODE:
		RETVAL = asimage_clone(im, 0x7fffffff);
	OUTPUT:
		RETVAL

int
image_c_release(im)
		ASImage* im
	CODE:
		RETVAL = release_asimage(im);
	OUTPUT:
		RETVAL

int
image_c_save(im, filename, type, compression, opacity)
		ASImage* im
		char*    filename
		char*    type
		char*    compression
		char*    opacity
	CODE:
		RETVAL = save_asimage_to_file(filename, im, type, compression, opacity, 0, 1);
	OUTPUT:
		RETVAL

MODULE = AfterImage	PACKAGE = AfterImage::Font	PREFIX = font_

PROTOTYPES: DISABLE

ASFontManager*
font_c_manager_create(path)
		char* path
	CODE:
		RETVAL = create_font_manager(NULL, path, NULL);
	OUTPUT:
		RETVAL

ASFont*
font_c_open(manager, font_name, points)
		ASFontManager* manager
		char*          font_name
		int            points
	CODE:
		RETVAL = get_asfont(manager, font_name, 0, points, ASF_GuessWho);
	OUTPUT:
		RETVAL
