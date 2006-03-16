#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include <libAfterImage/config.h>
#include <libAfterImage/afterbase.h>
#include <libAfterImage/afterimage.h>

/* set_output_threshold(0xffff); */

MODULE = AfterImage		PACKAGE = AfterImage		

PROTOTYPES: DISABLE

ASImageManager*
imagemanager_create(path)
		char* path
	CODE:
		{
			char* path2 = copy_replace_envvar(getenv(ASIMAGE_PATH_ENVVAR));
			if (path && path[0]) {
				RETVAL = create_image_manager(NULL, SCREEN_GAMMA, path, path2, NULL);
			} else {
				RETVAL = create_image_manager(NULL, SCREEN_GAMMA, path2, NULL);
			}
			RETVAL = create_image_manager(NULL, SCREEN_GAMMA, path, NULL);
		}
	OUTPUT:
		RETVAL

ASImage*
image_load(filename, manager)
		char*	          filename
		ASImageManager* manager
	CODE:
		RETVAL = get_asimage(manager, filename, ASFLAGS_EVERYTHING, 100);
	OUTPUT:
		RETVAL

ASImage*
image_parse_xml(xml_str, manager)
		char*	          xml_str
		ASImageManager* manager
	INIT:
	CODE:
		{
			ASVisual* asv = create_asvisual(NULL, 0, 0, NULL);
			RETVAL = compose_asimage_xml(asv, manager, NULL, xml_str, ASFLAGS_EVERYTHING, 20, 0, NULL);
		}
	OUTPUT:
		RETVAL

ASImage*
image_create(width, height)
		int width
		int height
	CODE:
		RETVAL = create_asimage(width, height, ASIMAGE_QUALITY_TOP);
	OUTPUT:
		RETVAL

ASImage*
image_gradient(width, height, colors, offsets, angle)
		int   width
		int   height
		SV*   colors
		SV*   offsets
		int   angle
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
			ASVisual* asv = create_asvisual(NULL, 0, 0, NULL);
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
			RETVAL = make_gradient(asv, &gradient, width, height, SCL_DO_ALL, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
			free(gradient.color);
			free(gradient.offset);
		}
	OUTPUT:
		RETVAL

ASImage*
image_solid(width, height, color)
		int   width
		int   height
		char* color
	CODE:
		{
			ASVisual* asv = create_asvisual(NULL, 0, 0, NULL);
			ASImage* result = create_asimage(width, height, ASIMAGE_QUALITY_TOP);
			ARGB32 argb;
			parse_argb_color(color, &argb);
			if (result) fill_asimage(asv, result, 0, 0, width, height, argb);
			RETVAL = result;
		}
	OUTPUT:
		RETVAL

ASImage*
image_composite(images, width, height, op, options)
		SV*   images
		int   width
		int   height
		char* op
		SV*   options
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
			ASVisual* asv = create_asvisual(NULL, 0, 0, NULL);
			ASImage* result;
			ASImageLayer *layers = create_image_layers(numimages);
			int i;
			merge_scanlines_func op_func = blend_scanlines_name2func(op);
			if (!asv || !result || !layers || !op_func) XSRETURN_UNDEF;

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
				asv, layers, numimages, width, height, ASA_ASImage, 0, 
				ASIMAGE_QUALITY_DEFAULT
			);
		}
	OUTPUT:
		RETVAL

int
image_destroy(im)
		ASImage* im
	CODE:
		RETVAL = release_asimage(im);
	OUTPUT:
		RETVAL

int
image_save(im, filename, type, compression, opacity)
		ASImage* im
		char*    filename
		char*    type
		char*    compression
		char*    opacity
	CODE:
		RETVAL = save_asimage_to_file(filename, im, type, compression, opacity, 0, 1);
	OUTPUT:
		RETVAL

