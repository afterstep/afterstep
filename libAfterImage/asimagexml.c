/*
 * Copyright (c) 2001 Sasha Vasko <sasha@aftercode.net>
 * Copyright (c) 2001 Eric Kowalski <eric@beancrock.net>
 * Copyright (c) 2001 Ethan Fisher <allanon@crystaltokyo.com>
 *
 * This module is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/times.h>

#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

/* #define LOCAL_DEBUG */

#include "afterbase.h"
#include "afterimage.h"

/****h* libAfterImage/compose_asimage_xml
 * NAME
 * ascompose is a tool to compose image(s) and display/save it based on
 * supplied XML input file.
 *
 * SYNOPSIS
 *
 * DESCRIPTION
 * ascompose reads supplied XML data, and manipulates image accordingly.
 * It could transform images from files of any supported file format,
 * draw gradients, render antialiased texturized text, perform
 * superimposition of arbitrary number of images, and save images into
 * files of any of supported output file formats.
 *
 * At any point, the result of any operation could be assigned a name,
 * and later on referenced under this name.
 *
 * At any point during the script processing, result of any operation
 * could be saved into a file of any supported file types.
 *
 * Internal image format is 32bit ARGB with 8bit per channel.
 *
 * Last image referenced, will be displayed in X window, unless -n option
 * is specified. If -r option is specified, then this image will be
 * displayed in root window of X display, effectively setting a background
 * for a desktop. If -o option is specified, this image will also be
 * saved into the file or requested type.
 *
 *****/



/* Stolen from libAfterStep. */
static Pixmap __GetRootPixmap (ASVisual *asv, Atom id)
{
	Pixmap        currentRootPixmap = None;
#ifndef X_DISPLAY_MISSING

	if (id == None)
		id = XInternAtom (asv->dpy, "_XROOTPMAP_ID", True);

	if (id != None)
	{
		Atom          act_type;
		int           act_format;
		unsigned long nitems, bytes_after;
		unsigned char *prop = NULL;

		if (XGetWindowProperty (asv->dpy, RootWindow(asv->dpy, asv->visual_info.screen), id, 0, 1, False, XA_PIXMAP,
								&act_type, &act_format, &nitems, &bytes_after, &prop) == Success)
		{
			if (prop)
			{
				currentRootPixmap = *((Pixmap *) prop);
				XFree (prop);
			}
		}
	}
#endif /* X_DISPLAY_MISSING */
	return currentRootPixmap;
}

static char* cdata_str = "CDATA";
static char* container_str = "CONTAINER";

ASImage *
compose_asimage_xml(ASVisual *asv, ASImageManager *imman, ASFontManager *fontman, char *doc_str, ASFlagType flags, int verbose, Window display_win, const char *path) 
{
	ASImage* im = NULL;
	xml_elem_t* doc;
	ASImageManager *my_imman = imman ;
	ASFontManager  *my_fontman = fontman ;

	doc = xml_parse_doc(doc_str);
	if (verbose > 1) {
		xml_print(doc);
		fprintf(stderr, "\n");
	}

	/* Build the image(s) from the xml document structure. */
	
	if (doc) 
	{
		xml_elem_t* ptr;
		char *path2 ;
		if( my_imman == NULL ) 
		{
			path2 = copy_replace_envvar( getenv( ASIMAGE_PATH_ENVVAR ) );
			if( path == NULL ) 
				my_imman = create_image_manager( NULL, SCREEN_GAMMA, path, path2, NULL );
			else
				my_imman = create_image_manager( NULL, SCREEN_GAMMA, path2, NULL );
			if( path2 )
				free( path2 );
		}
		if( my_fontman == NULL ) 
		{
			path2 = copy_replace_envvar( getenv( ASFONT_PATH_ENVVAR ) );
			if( path != NULL )
			{
				if( path2 != NULL ) 
				{
					int path_len = strlen(path);
					char *full_path = safemalloc( path_len+1+strlen(path2)+1);
					strcpy( full_path, path );
					full_path[path_len] = ':';
					strcpy( &(full_path[path_len+1]), path2 );
					free( path2 );
					path2 = full_path ;
				}else
					path2 = (char*)path ;
			}
			my_fontman = create_font_manager( asv->dpy, path2, NULL );
			if( path2 && path2 != path )
				free( path2 );
		}
		for (ptr = doc->child ; ptr ; ptr = ptr->next) {
			ASImage* tmpim = build_image_from_xml(asv, my_imman, my_fontman, ptr, NULL, flags, verbose, display_win);
			if (tmpim && im) safe_asimage_destroy(im);
			if (tmpim) im = tmpim;
		}
LOCAL_DEBUG_OUT( "result im = %p, im->imman	= %p, my_imman = %p, im->magic = %8.8X", im, im?im->imageman:NULL, my_imman, im?im->magic:0 );
		
		if( my_imman != imman ) 
		{
			if( im->imageman == my_imman ) 
				forget_asimage( im );
			destroy_image_manager(my_imman, False);
		}
		if( my_fontman != fontman ) 
			destroy_font_manager(my_fontman, False);
	}

	/* Delete the xml. */
	if (doc) xml_elem_delete(NULL, doc);

LOCAL_DEBUG_OUT( "returning im = %p, im->imman	= %p, im->magic = %8.8X", im, im?im->imageman:NULL, im?im->magic:0 );
	return im;
}

Bool save_asimage_to_file(const char *file2bsaved, ASImage *im,
	           const char *strtype,
			   const char *compress,
			   const char *opacity,
			   int delay, int replace)
{
	ASImageExportParams params ;

	memset( &params, 0x00, sizeof(params) );
	params.gif.flags = EXPORT_ALPHA ;
	if (!mystrcasecmp(strtype, "jpeg") || !mystrcasecmp(strtype, "jpg"))  {
		params.type = ASIT_Jpeg;
		params.jpeg.quality = (compress==NULL)?-1:100-atoi(compress);
		if( params.jpeg.quality > 100 )
			params.jpeg.quality = 100;
	} else if (!mystrcasecmp(strtype, "bitmap") || !mystrcasecmp(strtype, "bmp")) {
		params.type = ASIT_Bmp;
	} else if (!mystrcasecmp(strtype, "png")) {
		params.type = ASIT_Png;
		params.png.compression = (compress==NULL)?-1:atoi(compress)/10;
		if( params.png.compression > 9 )
			params.png.compression = 9;
	} else if (!mystrcasecmp(strtype, "xcf")) {
		params.type = ASIT_Xcf;
	} else if (!mystrcasecmp(strtype, "ppm")) {
		params.type = ASIT_Ppm;
	} else if (!mystrcasecmp(strtype, "pnm")) {
		params.type = ASIT_Pnm;
	} else if (!mystrcasecmp(strtype, "ico")) {
		params.type = ASIT_Ico;
	} else if (!mystrcasecmp(strtype, "cur")) {
		params.type = ASIT_Cur;
	} else if (!mystrcasecmp(strtype, "gif")) {
		params.type = ASIT_Gif;
		params.gif.flags |= EXPORT_APPEND ;
		params.gif.opaque_threshold = (opacity==NULL)?127:atoi(opacity) ;
		params.gif.dither = (compress==NULL)?3:atoi(compress)/17;
		if( params.gif.dither > 6 )
			params.gif.dither = 6;
		params.gif.animate_delay = delay ;
	} else if (!mystrcasecmp(strtype, "xpm")) {
		params.type = ASIT_Xpm;
		params.xpm.opaque_threshold = (opacity==NULL)?127:atoi(opacity) ;
		params.xpm.dither = (compress==NULL)?3:atoi(compress)/17;
		if( params.xpm.dither > 6 )
			params.xpm.dither = 6;
	} else if (!mystrcasecmp(strtype, "xbm")) {
		params.type = ASIT_Xbm;
	} else if (!mystrcasecmp(strtype, "tiff")) {
		params.type = ASIT_Tiff;
		params.tiff.compression_type = TIFF_COMPRESSION_NONE ;
		if( compress )
		{
			if( strcasecmp( compress, "deflate" ) == 0 )
				params.tiff.compression_type = TIFF_COMPRESSION_DEFLATE ;
			else if( strcasecmp( compress, "jpeg" ) == 0 )
				params.tiff.compression_type = TIFF_COMPRESSION_JPEG ;
			else if( strcasecmp( compress, "ojpeg" ) == 0 )
				params.tiff.compression_type = TIFF_COMPRESSION_OJPEG ;
			else if( strcasecmp( compress, "packbits" ) == 0 )
				params.tiff.compression_type = TIFF_COMPRESSION_PACKBITS ;
		}
	} else {
		show_error("File type not found.");
		return(0);
	}

	if( replace  )
		unlink( file2bsaved );

	return ASImage2file(im, NULL, file2bsaved, params.type, &params);

}

void show_asimage(ASVisual *asv, ASImage* im, Window w, long delay) 
{
#ifndef X_DISPLAY_MISSING
	if ( im && w ) 
	{
		Pixmap p = asimage2pixmap(asv, w, im, NULL, False);
		struct timeval value;

		XSetWindowBackgroundPixmap( dpy, w, p );
		XClearWindow( dpy, w );
		XFlush( dpy );
		XFreePixmap( dpy, p );
		p = None ;
		value.tv_usec = delay % 10000;
		value.tv_sec = delay / 10000;
		PORTABLE_SELECT (1, 0, 0, 0, &value);
	}
#endif /* X_DISPLAY_MISSING */
}

/****** libAfterImage/asimagexml/tags
 * TAGS
 * Here is the list and description of possible XML tags to use in the
 * script :
 * 	img       - load image from the file.
 * 	recall    - recall previously loaded/generated image by its name.
 * 	text      - render text string into new image.
 * 	save      - save an image into the file.
 * 	bevel     - draw solid bevel frame around the image.
 * 	gradient  - render multipoint gradient.
 * 	mirror    - create mirror copy of an image.
 * 	blur      - perform gaussian blur on an image.
 * 	rotate    - rotate/flip image in 90 degree increments.
 * 	scale     - scale an image to arbitrary size.
 * 	crop      - crop an image to arbitrary size.
 * 	tile      - tile an image to arbitrary size.
 * 	hsv       - adjust Hue, Saturation and Value of an image.
 * 	pad       - pad image with solid color from either or all sides.
 * 	solid     - generate new image of requested size, filled with solid
 *              color.
 * 	composite - superimpose arbitrary number of images using one of 15
 *              available methods.
 *
 * Each tag generates new image as the result of the transformation -
 * existing images are never modified and could be reused as many times
 * as needed. See below for description of each tag.
 *
 * Whenever numerical values are involved, the basic math ops (add,
 * subtract, multiply, divide), unary minus, and parentheses are
 * supported.
 * Operator precedence is NOT supported.  Percentages are allowed, and
 * apply to the "size" parameter of this function.
 *
 * Each tag is only allowed to return ONE image.
 *
 ******/
/* Math expression parsing algorithm. */
double parse_math(const char* str, char** endptr, double size) {
	double total = 0;
	char op = '+';
	char minus = 0;
	const char* startptr = str;
	while (*str) {
		while (isspace((int)*str)) str++;
		if (!op) {
			if (*str == '+' || *str == '-' || *str == '*' || *str == '/') op = *str++;
			else if (*str == '-') { minus = 1; str++; }
			else if (*str == ')') { str++; break; }
			else break;
		} else {
			char* ptr;
			double num;
			if (*str == '(') num = parse_math(str + 1, &ptr, size);
			else num = strtod(str, &ptr);
			if (str != ptr) {
				if (*ptr == '%') num *= size / 100.0, ptr++;
				if (minus) num = -num;
				if (op == '+') total += num;
				else if (op == '-') total -= num;
				else if (op == '*') total *= num;
				else if (op == '/' && num) total /= num;
			} else break;
			str = ptr;
			op = '\0';
			minus = 0;
		}
	}
	if (endptr) *endptr = (char*)str;
	show_debug(__FILE__,"parse_math",__LINE__,"Parsed math [%s] with reference [%.2f] into number [%.2f].", startptr, size, total);
	return total;
}


/* Each tag is only allowed to return ONE image. */
ASImage* 
build_image_from_xml( ASVisual *asv, ASImageManager *imman, ASFontManager *fontman, xml_elem_t* doc, xml_elem_t** rparm, ASFlagType flags, int verbose, Window display_win) 
{
	xml_elem_t* ptr;
	char* id = NULL;
	ASImage* result = NULL;

/****** libAfterImage/asimagexml/tags/img
 * NAME
 * img - load image from the file.
 * SYNOPSIS
 * <img id="new_img_id" src=filename/>
 * ATTRIBUTES
 * id     Optional.  Image will be given this name for future reference.
 * src    Required.  The filename (NOT URL) of the image file to load.
 * NOTES
 * The special image src "xroot:" will import the background image
 * of the root X window, if any.  No attempt will be made to offset this
 * image to fit the location of the resulting window, if one is displayed.
 ******/
	if (!strcmp(doc->tag, "img")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* src = NULL;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "src")) src = ptr->parm;
		}
		if (src && !strcmp(src, "xroot:")) {
			unsigned int width, height;
			Pixmap rp = __GetRootPixmap(asv, None);
			show_progress("Getting root pixmap.");
			if (rp) {
				get_drawable_size(rp, &width, &height);
				result = pixmap2asimage(asv, rp, 0, 0, width, height, 0xFFFFFFFF, False, 100);
			}
		} else if (src) {
			show_progress("Loading image [%s].", src);
			result = file2ASImage(src, 0xFFFFFFFF, SCREEN_GAMMA, 100, NULL);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

/****** libAfterImage/asimagexml/tags/recall
 * NAME
 * recall - recall previously generated and named image by its id.
 * SYNOPSIS
 * <recall id="new_id" srcid="image_id">
 * ATTRIBUTES
 * id       Optional.  Image will be given this name for future reference.
 * srcid    Required.  An image ID defined with the "id" parameter for
 *          any previously created image.
 ******/
	if (!strcmp(doc->tag, "recall")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* srcid = NULL;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "srcid")) srcid = ptr->parm;
		}
		if (srcid) {
			show_progress("Recalling image id [%s].", srcid);
			result = fetch_asimage(imman, srcid );
			if (!result)
				show_error("Image recall failed for id [%s].", srcid);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

/****** libAfterImage/asimagexml/tags/text
 * NAME
 * text - render text string into new image, using specific font, size
 *        and texture.
 * SYNOPSIS
 * <text id="new_id" font="font" point="size" fgcolor="color"
 *       bgcolor="color" fgimage="image_id" bgimage="image_id"
 *       spacing="points">My Text Here</text>
 * ATTRIBUTES
 * id       Optional.  Image will be given this name for future reference.
 * font     Optional.  Default is "fixed".  Font to use for text.
 * point    Optional.  Default is 12.  Size of text in points.
 * fgcolor  Optional.  No default.  The text will be drawn in this color.
 * bgcolor  Optional.  No default.  The area behind the text will be drawn
 *          in this color.
 * fgimage  Optional.  No default.  The text will be textured by this image.
 * bgimage  Optional.  No default.  The area behind the text will be filled
 *          with this image.
 * spacing  Optional.  Default 0.  Extra pixels to place between each glyph.
 * NOTES
 * <text> without bgcolor, fgcolor, fgimage, or bgimage will NOT
 * produce visible output by itself.  See EXAMPLES below.
 ******/
	if (!strcmp(doc->tag, "text")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* text = NULL;
		const char* font_name = "fixed";
		const char* fgimage_str = NULL;
		const char* bgimage_str = NULL;
		const char* fgcolor_str = NULL;
		const char* bgcolor_str = NULL;
		ARGB32 fgcolor = ARGB32_White, bgcolor = ARGB32_Black;
		int point = 12, spacing = 0;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "font")) font_name = ptr->parm;
			if (!strcmp(ptr->tag, "point")) point = strtol(ptr->parm, NULL, 0);
			if (!strcmp(ptr->tag, "spacing")) spacing = strtol(ptr->parm, NULL, 0);
			if (!strcmp(ptr->tag, "fgimage")) fgimage_str = ptr->parm;
			if (!strcmp(ptr->tag, "bgimage")) bgimage_str = ptr->parm;
			if (!strcmp(ptr->tag, "fgcolor")) fgcolor_str = ptr->parm;
			if (!strcmp(ptr->tag, "bgcolor")) bgcolor_str = ptr->parm;
		}
		for (ptr = doc->child ; ptr && !result ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, cdata_str)) text = ptr->parm;
		}
		if (text && point > 0) {
			struct ASFont *font = NULL;
			show_progress("Rendering text [%s] with font [%s] size [%d].", text, font_name, point);
			if (fontman) font = get_asfont(fontman, font_name, 0, point, ASF_GuessWho);
			if (font != NULL) {
				set_asfont_glyph_spacing(font, spacing, 0);
				result = draw_text(text, font, AST_Plain, 0);
				if (result && fgimage_str) {
					ASImage* fgimage = NULL;
					fgimage = fetch_asimage(imman, fgimage_str );
					show_progress("Using image [%s] as foreground.", fgimage_str);
					if (fgimage) {
						safe_asimage_destroy( fgimage );
						fgimage = tile_asimage(asv, fgimage, 0, 0, result->width, result->height, 0, ASA_ASImage, 100, ASIMAGE_QUALITY_TOP);
						move_asimage_channel(fgimage, IC_ALPHA, result, IC_ALPHA);
						safe_asimage_destroy(result);
						result = fgimage;
					}
				}
				if (result && fgcolor_str) {
					ASImage* fgimage = create_asimage(result->width, result->height, ASIMAGE_QUALITY_TOP);
					parse_argb_color(fgcolor_str, &fgcolor);
					fill_asimage(asv, fgimage, 0, 0, result->width, result->height, fgcolor);
					move_asimage_channel(fgimage, IC_ALPHA, result, IC_ALPHA);
					safe_asimage_destroy(result);
					result = fgimage;
				}
				if (result && (bgcolor_str || bgimage_str)) {
					ASImageLayer layers[2];
					init_image_layers(&(layers[0]), 2);
					if (bgimage_str) layers[0].im = fetch_asimage(imman, bgimage_str);
					if (bgcolor_str) 
						if( parse_argb_color(bgcolor_str, &bgcolor) != bgcolor_str )
						{
							if( layers[0].im != NULL )
								layers[0].im->back_color = bgcolor ;
							else 
								layers[0].solid_color = bgcolor ;
						}
					result->back_color = fgcolor ;
					layers[0].dst_x = 0;
					layers[0].dst_y = 0;
					layers[0].clip_width = result->width;
					layers[0].clip_height = result->height;
					layers[0].bevel = NULL;
					layers[1].im = result;
					layers[1].dst_x = 0;
					layers[1].dst_y = 0;
					layers[1].clip_width = result->width;
					layers[1].clip_height = result->height;
					result = merge_layers(asv, layers, 2, result->width, result->height, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
					safe_asimage_destroy( layers[0].im );
				}
			}
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

/****** libAfterImage/asimagexml/tags/save
 * NAME
 * save - write generated/loaded image into the file of one of the
 *        supported types
 * SYNOPSIS
 * <save id="new_id" dst="filename" format="format" compress="value"
 *       opacity="value" replace="0|1" delay="mlsecs">
 * ATTRIBUTES
 * id       Optional.  Image will be given this name for future reference.
 * dst      Required.  Name of file image will be saved to.
 * format   Optional.  Ouput format of saved image.  Defaults to the
 *          extension of the "dst" parameter.  Valid values are the
 *          standard AS image file formats: xpm, jpg, png, gif, tiff.
 * compress Optional.  Compression level if supported by output file
 *          format. Valid values are in range of 0 - 100 and any of
 *          "deflate", "jpeg", "ojpeg", "packbits" for TIFF files.
 *          Note that JPEG and GIF will produce images with deteriorated
 *          quality when compress is greater then 0. For JPEG default is
 *          25, for PNG default is 6 and for GIF it is 0.
 * opacity  Optional. Level below which pixel is considered to be
 *          transparent, while saving image as XPM or GIF. Valid values
 *          are in range 0-255. Default is 127.
 * replace  Optional. Causes ascompose to delete file if the file with the
 *          same name already exists. Valid values are 0 and 1. Default
 *          is 1 - files are deleted before being saved. Disable this to
 *          get multimage animated gifs.
 * delay    Optional. Delay to be stored in GIF image. This could be used
 *          to create animated gifs. Note that you have to set replace="0"
 *          and then write several images into the GIF file with the same
 *          name.
 * NOTES
 * This tag applies to the first image contained within the tag.  Any
 * further images will be discarded.
 *******/
	if (!strcmp(doc->tag, "save")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* dst = NULL;
		const char* ext = NULL;
		const char* compress = NULL ;
		const char* opacity = NULL ;
		int delay = 0 ;
		int replace = 1;
		/*<save id="" dst="" format="" compression="" delay="" replace="" opacity=""> */
		int autoext = 0;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			else if (!strcmp(ptr->tag, "dst")) dst = ptr->parm;
			else if (!strcmp(ptr->tag, "format")) ext = ptr->parm;
			else if (!strncmp(ptr->tag, "compress", 8)) compress = ptr->parm;
			else if (!strcmp(ptr->tag, "opacity")) opacity = ptr->parm;
			else if (!strcmp(ptr->tag, "delay"))   delay = atoi(ptr->parm);
			else if (!strcmp(ptr->tag, "replace")) replace = atoi(ptr->parm);
		}
		if (dst && !ext) {
			ext = strrchr(dst, '.');
			if (ext) ext++;
			autoext = 1;
		}
		if (dst && ext) {
			for (ptr = doc->child ; ptr && !result ; ptr = ptr->next) {
				result = build_image_from_xml(asv, imman, fontman, ptr, NULL, flags, verbose, display_win);
			}
			if (autoext)
				show_warning("No format given.  File extension [%s] used as format.", ext);
			show_progress("Saving image to file [%s].", dst);
			if (result && get_flags( flags, ASIM_XML_ENABLE_SAVE) )
			{
				if( !save_asimage_to_file(dst, result, ext, compress, opacity, delay, replace)) 
				show_error("Unable to save image into file [%s].", dst);
			}
		}
		if (rparm) *rparm = parm;
		else xml_elem_delete(NULL, parm);
	}

/****** libAfterImage/asimagexml/tags/bevel
 * NAME
 * bevel - draws solid bevel frame around the image.
 * SYNOPSIS
 * <bevel id="new_id" colors="color1 color2"
 *        border="left top right bottom">
 * ATTRIBUTES
 * id       Optional.  Image will be given this name for future reference.
 * colors   Optional.  Whitespace-separated list of colors.  Exactly two
 *          colors are required.  Default is "#ffdddddd #ff555555".  The
 *          first color is the color of the upper and left edges, and the
 *          second is the color of the lower and right edges.
 * borders  Optional.  Whitespace-separated list of integer values.
 *          Default is "10 10 10 10".  The values represent the offsets
 *          toward the center of the image of each border: left, top,
 *          right, bottom.
 * NOTES
 * This tag applies to the first image contained within the tag.  Any
 * further images will be discarded.
 ******/
	if (!strcmp(doc->tag, "bevel")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		ASImage* imtmp = NULL;
		char* color_str = NULL;
		char* border_str = NULL;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "colors")) color_str = ptr->parm;
			if (!strcmp(ptr->tag, "border")) border_str = ptr->parm;
		}
		for (ptr = doc->child ; ptr && !imtmp ; ptr = ptr->next) {
			imtmp = build_image_from_xml(asv, imman, fontman, ptr, NULL, flags, verbose, display_win);;
		}
		if (imtmp) {
			ASImageBevel bevel;
			ASImageLayer layer;
			bevel.type = BEVEL_SOLID_INLINE;
			bevel.hi_color = 0xffdddddd;
			bevel.lo_color = 0xff555555;
			bevel.top_outline = 0;
			bevel.left_outline = 0;
			bevel.right_outline = 0;
			bevel.bottom_outline = 0;
			bevel.top_inline = 10;
			bevel.left_inline = 10;
			bevel.right_inline = 10;
			bevel.bottom_inline = 10;
			if (color_str) {
				char* p = color_str;
				while (isspace((int)*p)) p++;
				parse_argb_color(p, &bevel.hi_color);
				while (*p && !isspace((int)*p)) p++;
				while (isspace((int)*p)) p++;
				parse_argb_color(p, &bevel.lo_color);
			}
			if (border_str) {
				char* p = (char*)border_str;
				bevel.left_inline = parse_math(p, &p, imtmp->width);
				bevel.top_inline = parse_math(p, &p, imtmp->height);
				bevel.right_inline = parse_math(p, &p, imtmp->width);
				bevel.bottom_inline = parse_math(p, &p, imtmp->height);
			}
			bevel.hihi_color = bevel.hi_color;
			bevel.hilo_color = bevel.hi_color;
			bevel.lolo_color = bevel.lo_color;
			show_progress("Generating bevel with offsets [%d %d %d %d] and colors [#%08x #%08x].", bevel.left_inline, bevel.top_inline, bevel.right_inline, bevel.bottom_inline, (unsigned int)bevel.hi_color, (unsigned int)bevel.lo_color);
			init_image_layers( &layer, 1 );
			layer.im = imtmp;
			layer.clip_width = imtmp->width;
			layer.clip_height = imtmp->height;
			layer.bevel = &bevel;
			result = merge_layers(asv, &layer, 1, imtmp->width, imtmp->height, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
			safe_asimage_destroy(imtmp);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

/****** libAfterImage/asimagexml/tags/gradient
 * NAME
 * gradient - render multipoint gradient.
 * SYNOPSIS
 * <gradient id="new_id" angle="degrees" width="pixels" height="pixels"
 *           colors ="color1 color2 color3 [...]"
 *           offsets="fraction1 fraction2 fraction3 [...]"/>
 * ATTRIBUTES
 * id       Optional.  Image will be given this name for future reference.
 * refid    Optional.  An image ID defined with the "id" parameter for
 *          any previously created image.  If set, percentages in "width"
 *          and "height" will be derived from the width and height of the
 *          refid image.
 * width    Required.  The gradient will have this width.
 * height   Required.  The gradient will have this height.
 * colors   Required.  Whitespace-separated list of colors.  At least two
 *          colors are required.  Each color in this list will be visited
 *          in turn, at the intervals given by the offsets attribute.
 * offsets  Optional.  Whitespace-separated list of floating point values
 *          ranging from 0.0 to 1.0.  The colors from the colors attribute
 *          are given these offsets, and the final gradient is rendered
 *          from the combination of the two.  If both colors and offsets
 *          are given but the number of colors and offsets do not match,
 *          the minimum of the two will be used, and the other will be
 *          truncated to match.  If offsets are not given, a smooth
 *          stepping from 0.0 to 1.0 will be used.
 * angle    Optional.  Given in degrees.  Default is 0.  This is the
 *          direction of the gradient.  Currently the only supported
 *          values are 0, 45, 90, 135, 180, 225, 270, 315.  0 means left
 *          to right, 90 means top to bottom, etc.
 *****/
	if (!strcmp(doc->tag, "gradient")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* refid = NULL;
		const char* width_str = NULL;
		const char* height_str = NULL;
		int width = 0, height = 0;
		double angle = 0;
		char* color_str = NULL;
		char* offset_str = NULL;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "width")) width_str = ptr->parm;
			if (!strcmp(ptr->tag, "height")) height_str = ptr->parm;
			if (!strcmp(ptr->tag, "angle")) angle = strtod(ptr->parm, NULL);
			if (!strcmp(ptr->tag, "colors")) color_str = ptr->parm;
			if (!strcmp(ptr->tag, "offsets")) offset_str = ptr->parm;
		}
		if (refid && width_str && height_str) {
			ASImage* refimg = fetch_asimage( imman, refid);
			if (refimg) {
				width = parse_math(width_str, NULL, refimg->width);
				height = parse_math(height_str, NULL, refimg->height);
			}
			safe_asimage_destroy(refimg);
		}
		if (!refid && width_str && height_str) {
			width = parse_math(width_str, NULL, width);
			height = parse_math(height_str, NULL, height);
		}
		if (width && height && color_str) {
			ASGradient gradient;
			int reverse = 0, npoints1 = 0, npoints2 = 0;
			char* p;
			angle = fmod(angle, 2 * PI);
			if (angle > 2 * PI * 15 / 16 || angle < 2 * PI * 1 / 16) {
				gradient.type = GRADIENT_Left2Right;
			} else if (angle < 2 * PI * 3 / 16) {
				gradient.type = GRADIENT_TopLeft2BottomRight;
			} else if (angle < 2 * PI * 5 / 16) {
				gradient.type = GRADIENT_Top2Bottom;
			} else if (angle < 2 * PI * 7 / 16) {
				gradient.type = GRADIENT_BottomLeft2TopRight; reverse = 1;
			} else if (angle < 2 * PI * 9 / 16) {
				gradient.type = GRADIENT_Left2Right; reverse = 1;
			} else if (angle < 2 * PI * 11 / 16) {
				gradient.type = GRADIENT_TopLeft2BottomRight; reverse = 1;
			} else if (angle < 2 * PI * 13 / 16) {
				gradient.type = GRADIENT_Top2Bottom; reverse = 1;
			} else {
				gradient.type = GRADIENT_BottomLeft2TopRight;
			}
			for (p = color_str ; isspace((int)*p) ; p++);
			for (npoints1 = 0 ; *p ; npoints1++) {
				if (*p) for ( ; *p && !isspace((int)*p) ; p++);
				for ( ; isspace((int)*p) ; p++);
			}
			if (offset_str) {
				for (p = offset_str ; isspace((int)*p) ; p++);
				for (npoints2 = 0 ; *p ; npoints2++) {
					if (*p) for ( ; *p && !isspace((int)*p) ; p++);
					for ( ; isspace((int)*p) ; p++);
				}
			}
			if (npoints1 > 1) {
				int i;
				if (offset_str && npoints1 > npoints2) npoints1 = npoints2;
				gradient.color = NEW_ARRAY(ARGB32, npoints1);
				gradient.offset = NEW_ARRAY(double, npoints1);
				for (p = color_str ; isspace((int)*p) ; p++);
				for (npoints1 = 0 ; *p ; ) {
					char* pb = p, ch;
					if (*p) for ( ; *p && !isspace((int)*p) ; p++);
					for ( ; isspace((int)*p) ; p++);
					ch = *p; *p = '\0';
					if (parse_argb_color(pb, gradient.color + npoints1)) npoints1++;
					*p = ch;
				}
				if (offset_str) {
					for (p = offset_str ; isspace((int)*p) ; p++);
					for (npoints2 = 0 ; *p ; ) {
						char* pb = p, ch;
						if (*p) for ( ; *p && !isspace((int)*p) ; p++);
						ch = *p; *p = '\0';
						gradient.offset[npoints2] = strtod(pb, &pb);
						if (pb == p) npoints2++;
						*p = ch;
						for ( ; isspace((int)*p) ; p++);
					}
				} else {
					for (npoints2 = 0 ; npoints2 < npoints1 ; npoints2++)
						gradient.offset[npoints2] = (double)npoints2 / (npoints1 - 1);
				}
				gradient.npoints = npoints1;
				if (npoints2 && gradient.npoints > npoints2)
					gradient.npoints = npoints2;
				if (reverse) {
					for (i = 0 ; i < gradient.npoints / 2 ; i++) {
						int i2 = gradient.npoints - 1 - i;
						ARGB32 c = gradient.color[i];
						double o = gradient.offset[i];
						gradient.color[i] = gradient.color[i2];
						gradient.color[i2] = c;
						gradient.offset[i] = gradient.offset[i2];
						gradient.offset[i2] = o;
					}
					for (i = 0 ; i < gradient.npoints ; i++) {
						gradient.offset[i] = 1.0 - gradient.offset[i];
					}
				}
				show_progress("Generating [%dx%d] gradient with angle [%f] and npoints [%d/%d].", width, height, angle, npoints1, npoints2);
				if (verbose > 1) {
					for (i = 0 ; i < gradient.npoints ; i++) {
						show_progress("  Point [%d] has color [#%08x] and offset [%f].", i, (unsigned int)gradient.color[i], gradient.offset[i]);
					}
				}
				result = make_gradient(asv, &gradient, width, height, SCL_DO_ALL, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
			}
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

/****** libAfterImage/asimagexml/tags/mirror
 * NAME
 * mirror - create new image as mirror copy of an old one.
 * SYNOPSIS
 *  <mirror id="new_id" dir="direction">
 * ATTRIBUTES
 * id       Optional. Image will be given this name for future reference.
 * dir      Required. Possible values are "vertical" and "horizontal".
 *          The image will be flipped over the x-axis if dir is vertical,
 *          and flipped over the y-axis if dir is horizontal.
 * NOTES
 * This tag applies to the first image contained within the tag.  Any
 * further images will be discarded.
 ******/
	if (!strcmp(doc->tag, "mirror")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		ASImage* imtmp = NULL;
		int dir = 0;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "dir")) dir = !mystrcasecmp(ptr->parm, "vertical");
		}
		for (ptr = doc->child ; ptr && !imtmp ; ptr = ptr->next) {
			imtmp = build_image_from_xml(asv, imman, fontman, ptr, NULL, flags, verbose, display_win);
		}
		if (imtmp) {
			result = mirror_asimage(asv, imtmp, 0, 0, imtmp->width, imtmp->height, dir, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
			safe_asimage_destroy(imtmp);
		}
		show_progress("Mirroring image [%sally].", dir ? "horizont" : "vertic");
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}
/****** libAfterImage/asimagexml/tags/blur
 * NAME
 * blur - perform a gaussian blurr on an image.
 * SYNOPSIS
 * <blur id="new_id" horz="radius" vert="radius">
 * ATTRIBUTES
 * id       Optional. Image will be given this name for future reference.
 * horz     Optional. Horizontal radius of the blur in pixels.
 * vert     Optional. Vertical radius of the blur in pixels.
 * NOTES
 * This tag applies to the first image contained within the tag.  Any
 * further images will be discarded.
 ******/
	if (!strcmp(doc->tag, "blur")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		ASImage* imtmp = NULL;
		int horz = 0, vert = 0;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "horz")) horz = strtod(ptr->parm, NULL);
			if (!strcmp(ptr->tag, "vert")) vert = strtod(ptr->parm, NULL);
		}
		for (ptr = doc->child ; ptr && !imtmp ; ptr = ptr->next) {
			imtmp = build_image_from_xml(asv, imman, fontman, ptr, NULL, flags, verbose, display_win);
		}
		if (imtmp) {
			result = blur_asimage_gauss(asv, imtmp, horz, vert, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
			safe_asimage_destroy(imtmp);
		}
		show_progress("Blurring image.");
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

/****** libAfterImage/asimagexml/tags/rotate
 * NAME
 * rotate - rotate an image in 90 degree increments (flip).
 * SYNOPSIS
 *  <rotate id="new_id" angle="degrees">
 * ATTRIBUTES
 * id       Optional. Image will be given this name for future reference.
 * angle    Required.  Given in degrees.  Possible values are currently
 *          "90", "180", and "270".  Rotates the image through the given
 *          angle.
 * NOTES
 * This tag applies to the first image contained within the tag.  Any
 * further images will be discarded.
 ******/
	if (!strcmp(doc->tag, "rotate")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		ASImage* imtmp = NULL;
		double angle = 0;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "angle")) angle = strtod(ptr->parm, NULL);
		}
		for (ptr = doc->child ; ptr && !imtmp ; ptr = ptr->next) {
			imtmp = build_image_from_xml(asv, imman, fontman, ptr, NULL, flags, verbose, display_win);;
		}
		if (imtmp) {
			int dir = 0;
			angle = fmod(angle, 2 * PI);
			if (angle > 2 * PI * 7 / 8 || angle < 2 * PI * 1 / 8) {
				dir = 0;
			} else if (angle < 2 * PI * 3 / 8) {
				dir = FLIP_VERTICAL;
			} else if (angle < 2 * PI * 5 / 8) {
				dir = FLIP_UPSIDEDOWN;
			} else {
				dir = FLIP_VERTICAL | FLIP_UPSIDEDOWN;
			}
			if (dir) {
				result = flip_asimage(asv, imtmp, 0, 0, imtmp->width, imtmp->height, dir, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
				safe_asimage_destroy(imtmp);
				show_progress("Rotating image [%f degrees].", angle);
			} else {
				result = imtmp;
			}
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

/****** libAfterImage/asimagexml/tags/scale
 * NAME
 * scale - scale image to arbitrary size
 * SYNOPSIS
 * <scale id="new_id" ref_id="other_imag" width="pixels" height="pixels">
 * ATTRIBUTES
 * id       Optional. Image will be given this name for future reference.
 * refid    Optional.  An image ID defined with the "id" parameter for
 *          any previously created image.  If set, percentages in "width"
 *          and "height" will be derived from the width and height of the
 *          refid image.
 * width    Required.  The image will be scaled to this width.
 * height   Required.  The image will be scaled to this height.
 * NOTES
 * This tag applies to the first image contained within the tag.  Any
 * further images will be discarded.
 ******/
	if (!strcmp(doc->tag, "scale")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* refid = NULL;
		const char* width_str = NULL;
		const char* height_str = NULL;
		int width = 0, height = 0;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "refid")) refid = ptr->parm;
			if (!strcmp(ptr->tag, "width")) width_str = ptr->parm;
			if (!strcmp(ptr->tag, "height")) height_str = ptr->parm;
		}
		if (refid && width_str && height_str) {
			ASImage* refimg = fetch_asimage(imman, refid );
			if (refimg) {
				width = parse_math(width_str, NULL, refimg->width);
				height = parse_math(height_str, NULL, refimg->height);
			}
			safe_asimage_destroy( refimg );
		}
		if (!refid && width_str && height_str) {
			width = parse_math(width_str, NULL, width);
			height = parse_math(height_str, NULL, height);
		}
		if (width && height) {
			ASImage* imtmp = NULL;
			for (ptr = doc->child ; ptr && !imtmp ; ptr = ptr->next) {
				imtmp = build_image_from_xml(asv, imman, fontman, ptr, NULL, flags, verbose, display_win);
			}
			if (imtmp) {
				result = scale_asimage(asv, imtmp, width, height, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT);
				safe_asimage_destroy(imtmp);
			}
			show_progress("Scaling image to [%dx%d].", width, height);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

/****** libAfterImage/asimagexml/tags/crop
 * NAME
 * crop - crop image to arbitrary area within it.
 * SYNOPSIS
 *  <crop id="new_id" refid="other_image" srcx="pixels" srcy="pixels"
 *        width="pixels" height="pixels" tint="color">
 * ATTRIBUTES
 * id       Optional. Image will be given this name for future reference.
 * refid    Optional. An image ID defined with the "id" parameter for
 *          any previously created image.  If set, percentages in "width"
 *          and "height" will be derived from the width and height of the
 *          refid image.
 * srcx     Optional. Default is "0". Skip this many pixels from the left.
 * srcy     Optional. Default is "0". Skip this many pixels from the top.
 * width    Optional. Default is "100%".  Keep this many pixels wide.
 * height   Optional. Default is "100%".  Keep this many pixels tall.
 * tint     Optional. Additionally tint an image to specified color.
 *          Tinting can both lighten and darken an image. Tinting color
 *          0 or #7f7f7f7f yeilds no tinting. Tinting can be performed on
 *          any channel, including alpha channel.
 * NOTES
 * This tag applies to the first image contained within the tag.  Any
 * further images will be discarded.
 ******/
	if (!strcmp(doc->tag, "crop")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* refid = NULL;
		const char* srcx_str = NULL;
		const char* srcy_str = NULL;
		const char* width_str = NULL;
		const char* height_str = NULL;
		ARGB32 tint = 0 ;
		int width = 0, height = 0, srcx = 0, srcy = 0;
		ASImage* imtmp = NULL;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "refid")) refid = ptr->parm;
			if (!strcmp(ptr->tag, "srcx")) srcx_str = ptr->parm;
			if (!strcmp(ptr->tag, "srcy")) srcy_str = ptr->parm;
			if (!strcmp(ptr->tag, "width")) width_str = ptr->parm;
			if (!strcmp(ptr->tag, "height")) height_str = ptr->parm;
			if (!strcmp(ptr->tag, "tint")) parse_argb_color(ptr->parm, &tint);
		}
		for (ptr = doc->child ; ptr && !imtmp ; ptr = ptr->next) {
			imtmp = build_image_from_xml(asv, imman, fontman, ptr, NULL, flags, verbose, display_win);
		}
		if (imtmp) {
			width = imtmp->width;
			height = imtmp->height;
			if (refid) {
				ASImage* refimg = fetch_asimage(imman, refid);
				if (refimg) {
					width = refimg->width;
					height = refimg->height;
				}
				safe_asimage_destroy( refimg );
			}
			if (srcx_str) srcx = parse_math(srcx_str, NULL, width);
			if (srcy_str) srcy = parse_math(srcy_str, NULL, height);
			if (width_str) width = parse_math(width_str, NULL, width);
			if (height_str) height = parse_math(height_str, NULL, height);
			if (width > imtmp->width) width = imtmp->width;
			if (height > imtmp->height) height = imtmp->height;
			if (width > 0 && height > 0) {
				result = tile_asimage(asv, imtmp, srcx, srcy, width, height, tint, ASA_ASImage, 100, ASIMAGE_QUALITY_TOP);
				safe_asimage_destroy(imtmp);
			}
			show_progress("Cropping image to [%dx%d].", width, height);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}
/****** libAfterImage/asimagexml/tags/tile
 * NAME
 * tile - tile an image to specified area.
 * SYNOPSIS
 *  <tile id="new_id" refid="other_image" width="pixels" height="pixels"
 *        x_origin="pixels" y_origin="pixels" tint="color">
 * ATTRIBUTES
 * id       Optional. Image will be given this name for future reference.
 * refid    Optional. An image ID defined with the "id" parameter for
 *          any previously created image.  If set, percentages in "width"
 *          and "height" will be derived from the width and height of the
 *          refid image.
 * width    Optional. Default is "100%". The image will be tiled to this
 *          width.
 * height   Optional. Default is "100%". The image will be tiled to this
 *          height.
 * x_origin Optional. Horizontal position on infinite surface, covered
 *          with tiles of the image, from which to cut out resulting
 *          image.
 * y_origin Optional. Vertical position on infinite surface, covered
 *          with tiles of the image, from which to cut out resulting
 *          image.
 * tint     Optional. Additionally tint an image to specified color.
 *          Tinting can both lighten and darken an image. Tinting color
 *          0 or #7f7f7f7f yields no tinting. Tinting can be performed
 *          on any channel, including alpha channel.
 * NOTES
 * This tag applies to the first image contained within the tag.  Any
 * further images will be discarded.
 ******/
	if (!strcmp(doc->tag, "tile")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* refid = NULL;
		const char* xorig_str = NULL;
		const char* yorig_str = NULL;
		const char* width_str = "100%";
		const char* height_str = "100%";
		int width = 0, height = 0, xorig = 0, yorig = 0;
		ARGB32 tint = 0 ;
		ASImage* imtmp = NULL;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			else if (!strcmp(ptr->tag, "refid")) refid = ptr->parm;
			else if (!strcmp(ptr->tag, "x_origin")) xorig_str = ptr->parm;
			else if (!strcmp(ptr->tag, "y_origin")) yorig_str = ptr->parm;
			else if (!strcmp(ptr->tag, "width")) width_str = ptr->parm;
			else if (!strcmp(ptr->tag, "height")) height_str = ptr->parm;
			else if (!strcmp(ptr->tag, "tint")) parse_argb_color(ptr->parm, &tint);
		}
		for (ptr = doc->child ; ptr && !imtmp ; ptr = ptr->next) {
			imtmp = build_image_from_xml(asv, imman, fontman, ptr, NULL, flags, verbose, display_win);
		}
		if (imtmp) {
			width = imtmp->width;
			height = imtmp->height;
			if (refid) {
				ASImage* refimg = fetch_asimage(imman, refid);;
				if (refimg) {
					width = refimg->width;
					height = refimg->height;
				}
				safe_asimage_destroy( refimg );
			}
			if (width_str) width = parse_math(width_str, NULL, width);
			if (height_str) height = parse_math(height_str, NULL, height);
			if (xorig_str) xorig = parse_math(xorig_str, NULL, width);
			if (yorig_str) yorig = parse_math(yorig_str, NULL, height);
			if (width > 0 && height > 0) {
				result = tile_asimage(asv, imtmp, xorig, yorig, width, height, tint, ASA_ASImage, 100, ASIMAGE_QUALITY_TOP);
				safe_asimage_destroy(imtmp);
			}
			show_progress("Tiling image to [%dx%d].", width, height);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}
/****** libAfterImage/asimagexml/tags/hsv
 * NAME
 * hsv - adjust Hue, Saturation and/or Value of an image and optionally
 * tile an image to arbitrary area.
 * SYNOPSIS
 * <hsv id="new_id" refid="other_image"
 *      x_origin="pixels" y_origin="pixels" width="pixels" height="pixels"
 *      affected_hue="degrees|color" affected_radius="degrees"
 *      hue_offset="degrees" saturation_offset="value"
 *      value_offset="value">
 * ATTRIBUTES
 * id       Optional. Image will be given this name for future reference.
 * refid    Optional. An image ID defined with the "id" parameter for
 *          any previously created image.  If set, percentages in "width"
 *          and "height" will be derived from the width and height of the
 *          refid image.
 * width    Optional. Default is "100%". The image will be tiled to this
 *          width.
 * height   Optional. Default is "100%". The image will be tiled to this
 *          height.
 * x_origin Optional. Horizontal position on infinite surface, covered
 *          with tiles of the image, from which to cut out resulting
 *          image.
 * y_origin Optional. Vertical position on infinite surface, covered
 *          with tiles of the image, from which to cut out resulting
 *          image.
 * affected_hue    Optional. Limits effects to the renage of hues around
 *          this hue. If numeric value is specified - it is treated as
 *          degrees on 360 degree circle, with :
 *              red = 0,
 *              yellow = 60,
 *              green = 120,
 *              cyan = 180,
 *              blue = 240,
 *              magenta = 300.
 *          If colorname or value preceded with # is specified here - it
 *          will be treated as RGB color and converted into hue
 *          automagically.
 * affected_radius
 *          Optional. Value in degrees to be used in order to
 *          calculate the range of affected hues. Range is determined by
 *          substracting and adding this value from/to affected_hue.
 * hue_offset
 *          Optional. Value by which to adjust the hue.
 * saturation_offset
 *          Optional. Value by which to adjust the saturation.
 * value_offset
 *          Optional. Value by which to adjust the value.
 * NOTES
 * One of the Offsets must be not 0, in order for operation to be
 * performed.
 *
 * This tag applies to the first image contained within the tag.  Any
 * further images will be discarded.
 ******/
	if (!strcmp(doc->tag, "hsv")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* refid = NULL;
		const char* xorig_str = NULL;
		const char* yorig_str = NULL;
		const char* width_str = "100%";
		const char* height_str = "100%";
		int affected_hue = 0, affected_radius = 360 ;
		int hue_offset = 0, saturation_offset = 0, value_offset = 0 ;
		int width = 0, height = 0, xorig = 0, yorig = 0;
		ASImage* imtmp = NULL;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			else if (!strcmp(ptr->tag, "refid")) refid = ptr->parm;
			else if (!strcmp(ptr->tag, "x_origin")) xorig_str = ptr->parm;
			else if (!strcmp(ptr->tag, "y_origin")) yorig_str = ptr->parm;
			else if (!strcmp(ptr->tag, "width")) width_str = ptr->parm;
			else if (!strcmp(ptr->tag, "height")) height_str = ptr->parm;
			else if (!strcmp(ptr->tag, "affected_hue"))
			{
				if( isdigit( ptr->parm[0] ) )
					affected_hue = atoi(ptr->parm);
				else
				{
					ARGB32 color = 0;
					if( parse_argb_color( ptr->parm, &color ) != ptr->parm )
						affected_hue = hue162degrees(rgb2hue( ARGB32_RED16(color),
												ARGB32_GREEN16(color),
												ARGB32_BLUE16(color)));
				}
			}
			else if (!strcmp(ptr->tag, "affected_radius")) 	affected_radius = atoi(ptr->parm);
			else if (!strcmp(ptr->tag, "hue_offset")) 		hue_offset = atoi(ptr->parm);
			else if (!strcmp(ptr->tag, "saturation_offset"))saturation_offset = atoi(ptr->parm);
			else if (!strcmp(ptr->tag, "value_offset")) 	value_offset = atoi(ptr->parm);
		}
		for (ptr = doc->child ; ptr && !imtmp ; ptr = ptr->next) {
			imtmp = build_image_from_xml(asv, imman, fontman, ptr, NULL, flags, verbose, display_win);
		}
		if (imtmp) {
			width = imtmp->width;
			height = imtmp->height;
			if (refid) {
				ASImage* refimg = fetch_asimage(imman, refid);;
				if (refimg) {
					width = refimg->width;
					height = refimg->height;
				}
				safe_asimage_destroy( refimg );
			}
			if (width_str) width = parse_math(width_str, NULL, width);
			if (height_str) height = parse_math(height_str, NULL, height);
			if (xorig_str) xorig = parse_math(xorig_str, NULL, width);
			if (yorig_str) yorig = parse_math(yorig_str, NULL, height);
			if (width > 0 && height > 0 &&
				(hue_offset!=0 || saturation_offset != 0 || value_offset != 0 )) {
				result = adjust_asimage_hsv(asv, imtmp, xorig, yorig, width, height,
				                            affected_hue, affected_radius,
											hue_offset, saturation_offset, value_offset,
				                            ASA_ASImage, 100, ASIMAGE_QUALITY_TOP);
				safe_asimage_destroy(imtmp);
			}
			show_progress("adjusting HSV of the image by [%d,%d,%d] affected hues are %d-%d.", hue_offset, saturation_offset, value_offset, affected_hue-affected_radius, affected_hue+affected_radius);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

/****** libAfterImage/asimagexml/tags/pad
 * NAME
 * pad - pad an image with solid color rectangles.
 * SYNOPSIS
 * <pad id="new_id" refid="other_image" left="pixels" top="pixels"
 *      right="pixels" bottom="pixels" color="color">
 * ATTRIBUTES
 * id       Optional. Image will be given this name for future reference.
 * refid    Optional. An image ID defined with the "id" parameter for
 *          any previously created image.  If set, percentages in "pixel"
 *          pad values will be derived from the width and height of the
 *          refid image.
 * left     Optional. Size to add to the left of the image.
 * top      Optional. Size to add to the top of the image.
 * right    Optional. Size to add to the right of the image.
 * bottom   Optional. Size to add to the bottom of the image.
 * color    Optional. Color value to fill added areas with. It could be
 *          transparent of course. Default is #FF000000 - totally black.
 * NOTES
 * This tag applies to the first image contained within the tag.  Any
 * further images will be discarded.
 ******/
	if (!strcmp(doc->tag, "pad")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* refid = NULL;
		const char* left_str = "0";
		const char* top_str  = "0";
		const char* right_str  = "0";
		const char* bottom_str  = "0";
		ARGB32 color  = ARGB32_Black;
		int left = 0, top = 0, right = 0, bottom = 0;
		ASImage* imtmp = NULL;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			else if (!strcmp(ptr->tag, "refid"))  refid = ptr->parm;
			else if (!strcmp(ptr->tag, "left"))   left_str = ptr->parm;
			else if (!strcmp(ptr->tag, "top"))    top_str = ptr->parm;
			else if (!strcmp(ptr->tag, "right"))  right_str = ptr->parm;
			else if (!strcmp(ptr->tag, "bottom")) bottom_str = ptr->parm;
			else if (!strcmp(ptr->tag, "color"))  parse_argb_color(ptr->parm, &color);
		}
		for (ptr = doc->child ; ptr && !imtmp ; ptr = ptr->next) {
			imtmp = build_image_from_xml(asv, imman, fontman, ptr, NULL, flags, verbose, display_win);
		}
		if (imtmp) {
			int width = imtmp->width;
			int height = imtmp->height;
			if (refid) {
				ASImage* refimg = fetch_asimage(imman, refid);
				if (refimg) {
					width = refimg->width;
					height = refimg->height;
				}
				safe_asimage_destroy( refimg );
			}
			if (left_str) left = parse_math(left_str, NULL, width);
			if (top_str)  top = parse_math(top_str, NULL, height);
			if (right_str) right = parse_math(right_str, NULL, width);
			if (bottom_str)  bottom = parse_math(bottom_str, NULL, height);
			if (left > 0 || top > 0 || right > 0 || bottom > 0 )
			{
				result = pad_asimage(asv, imtmp, left, top, width+left+right, height+top+bottom,
					                 color, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT);
				safe_asimage_destroy(imtmp);
			}
			show_progress("Padding image to [%dx%d%+d%+d].", width+left+right, height+top+bottom, left, top);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

/****** libAfterImage/asimagexml/tags/solid
 * NAME
 * solid - generate image of specified size and fill it with solid color.
 * SYNOPSIS
 * <solid id="new_id" color="color" width="pixels" height="pixels"/>
 * ATTRIBUTES
 * id       Optional. Image will be given this name for future reference.
 * color    Optional.  Default is "#ffffffff".  An image will be created
 *          and filled with this color.
 * width    Required.  The image will have this width.
 * height   Required.  The image will have this height.
 ******/
	if (!strcmp(doc->tag, "solid")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* refid = NULL;
		const char* width_str = NULL;
		const char* height_str = NULL;
		int width = 0, height = 0;
		ARGB32 color = ARGB32_White;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "refid")) refid = ptr->parm;
			if (!strcmp(ptr->tag, "color")) parse_argb_color(ptr->parm, &color);
			if (!strcmp(ptr->tag, "width")) width_str = ptr->parm;
			if (!strcmp(ptr->tag, "height")) height_str = ptr->parm;
		}
		if (refid && width_str && height_str) {
			ASImage* refimg = fetch_asimage(imman, refid);
			if (refimg) {
				width = parse_math(width_str, NULL, refimg->width);
				height = parse_math(height_str, NULL, refimg->height);
			}
			safe_asimage_destroy( refimg );
		}
		if (!refid && width_str && height_str) {
			width = parse_math(width_str, NULL, 0);
			height = parse_math(height_str, NULL, 0);
		}
		if (width && height) {
			result = create_asimage(width, height, ASIMAGE_QUALITY_TOP);
			if (result) fill_asimage(asv, result, 0, 0, width, height, color);
			show_progress("Creating solid color [#%08x] image [%dx%d].", (unsigned int)color, width, height);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

/****** libAfterImage/asimagexml/tags/composite
 * NAME
 * composite - superimpose arbitrary number of images on top of each
 * other.
 * SYNOPSIS
 * <composite id="new_id" op="op_desc"
 *            keep-transparency="0|1" merge="0|1">
 * ATTRIBUTES
 * id       Optional. Image will be given this name for future reference.
 * op       Optional. Default is "alphablend". The compositing operation.
 *          Valid values are the standard AS blending ops: add, alphablend,
 *          allanon, colorize, darken, diff, dissipate, hue, lighten,
 *          overlay, saturate, screen, sub, tint, value.
 * merge    Optional. Default is "expand". Valid values are "clip" and
 *          "expand". Determines whether final image will be expanded to
 *          the maximum size of the layers, or clipped to the bottom
 *          layer.
 * keep-transparency
 *          Optional. Default is "0". Valid values are "0" and "1". If
 *          set to "1", the transparency of the bottom layer will be
 *          kept for the final image.
 * NOTES
 * All images surrounded by this tag will be composited with the given op.
 *
 * All tags surrounded by this tag will be given the following
 * additional attributes in addition to their normal ones.  Under no
 * circumstances is there a conflict with the normal child attributes.
 * crefid   Optional. An image ID defined with the "id" parameter for
 *          any previously created image. If set, percentages in "x"
 *          and "y" will be derived from the width and height of the
 *          crefid image.
 * x        Optional. Default is 0. Pixel coordinate of left edge.
 * y        Optional. Default is 0. Pixel coordinate of top edge.
 * clip_x   Optional. Default is 0. X Offset on infinite surface tiled
 *          with this image, from which to cut portion of an image to be
 *          used in composition.
 * clip_y   Optional. Default is 0. Y Offset on infinite surface tiled
 *          with this image, from which to cut portion of an image to be
 *          used in composition.
 * clip_width
 *          Optional. Default is image width. Tile image to this width
 *          prior to superimposition.
 * clip_height
 *          Optional. Default is image height. Tile image to this height
 *          prior to superimposition.
 * tile     Optional. Default is 0. If set will cause image to be tiled
 *          across entire composition, unless overridden by clip_width or
 *          clip_height.
 * tint     Optional. Additionally tint an image to specified color.
 *          Tinting can both lighten and darken an image. Tinting color
 *          0 or #7f7f7f7f yields no tinting. Tinting can be performed
 *          on any channel, including alpha channel.
 * SEE ALSO
 * libAfterImage
 ******/
	if (!strcmp(doc->tag, "composite")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* pop = "alphablend";
		int keep_trans = 0;
		int merge = 0;
		int num;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "op")) pop = ptr->parm;
			if (!strcmp(ptr->tag, "keep-transparency")) keep_trans = strtol(ptr->parm, NULL, 0);
			if (!strcmp(ptr->tag, "merge") && !mystrcasecmp(ptr->parm, "clip")) merge = 1;
		}
		/* Find out how many subimages we have. */
		num = 0;
		for (ptr = doc->child ; ptr ; ptr = ptr->next) {
			if (strcmp(ptr->tag, cdata_str)) num++;
		}
		if (num) {
			int width = 0, height = 0;
			ASImageLayer *layers;
			int i ;

			/* Build the layers first. */
			layers = create_image_layers( num );
			for (num = 0, ptr = doc->child ; ptr ; ptr = ptr->next) {
				int x = 0, y = 0;
				int clip_x = 0, clip_y = 0;
				int clip_width = 0, clip_height = 0;
				ARGB32 tint = 0;
				Bool tile = False ;
				xml_elem_t* sparm = NULL;
				if (!strcmp(ptr->tag, cdata_str)) continue;
				if( (layers[num].im = build_image_from_xml(asv, imman, fontman, ptr, &sparm, flags, verbose, display_win)) != NULL )
				{
					clip_width = layers[num].im->width;
					clip_height = layers[num].im->height;
				}
				if (sparm) {
					xml_elem_t* tmp;
					const char* x_str = NULL;
					const char* y_str = NULL;
					const char* clip_x_str = NULL;
					const char* clip_y_str = NULL;
					const char* clip_width_str = NULL;
					const char* clip_height_str = NULL;
					const char* refid = NULL;
					for (tmp = sparm ; tmp ; tmp = tmp->next) {
						if (!strcmp(tmp->tag, "crefid")) refid = tmp->parm;
						else if (!strcmp(tmp->tag, "x")) x_str = tmp->parm;
						else if (!strcmp(tmp->tag, "y")) y_str = tmp->parm;
						else if (!strcmp(tmp->tag, "clip_x")) clip_x_str = tmp->parm;
						else if (!strcmp(tmp->tag, "clip_y")) clip_y_str = tmp->parm;
						else if (!strcmp(tmp->tag, "clip_width")) clip_width_str = tmp->parm;
						else if (!strcmp(tmp->tag, "clip_height")) clip_height_str = tmp->parm;
						else if (!strcmp(tmp->tag, "tint")) parse_argb_color(tmp->parm, &tint);
						else if (!strcmp(tmp->tag, "tile")) tile = True;
					}
					if (refid) {
						ASImage* refimg = fetch_asimage(imman, refid);
						if (refimg) {
							x = refimg->width;
							y = refimg->height;
						}
						safe_asimage_destroy(refimg );
					}
					x = x_str ? parse_math(x_str, NULL, x) : 0;
					y = y_str ? parse_math(y_str, NULL, y) : 0;
					clip_x = clip_x_str ? parse_math(clip_x_str, NULL, x) : 0;
					clip_y = clip_y_str ? parse_math(clip_y_str, NULL, y) : 0;
					if( clip_width_str )
						clip_width = parse_math(clip_width_str, NULL, clip_width);
					else if( tile )
						clip_width = 0 ;
					if( clip_height_str )
						clip_height = parse_math(clip_height_str, NULL, clip_height);
					else if( tile )
						clip_height = 0 ;
				}
				if (layers[num].im) {
					layers[num].dst_x = x;
					layers[num].dst_y = y;
					layers[num].clip_x = clip_x;
					layers[num].clip_y = clip_y;
					layers[num].clip_width = clip_width ;
					layers[num].clip_height = clip_height ;
					layers[num].tint = tint;
					layers[num].bevel = 0;
					layers[num].merge_scanlines = blend_scanlines_name2func(pop);
					if( clip_width + x > 0 )
					{
						if( width < clip_width + x )
							width = clip_width + x;
					}else
					 	if (width < layers[num].im->width) width = layers[num].im->width;
					if( clip_height + y > 0 )
					{
						if( height < clip_height + y )
							height = clip_height + y ;
					}else
						if (height < layers[num].im->height) height = layers[num].im->height;
					num++;
				}
				if (sparm) xml_elem_delete(NULL, sparm);
			}

			if (merge) {
				width = layers[0].im->width;
				height = layers[0].im->height;
			}
	   		for (i = 0 ; i < num ; i++) {
				if( layers[i].clip_width == 0 )
					layers[i].clip_width = width - layers[i].dst_x;
				if( layers[i].clip_height == 0 )
					layers[i].clip_height = height - layers[i].dst_y;
			}

			show_progress("Compositing [%d] image(s) with op [%s].  Final geometry [%dx%d].", num, pop, width, height);
			if (keep_trans) show_progress("  Keeping transparency.");
			if (verbose > 1) {
				for (i = 0 ; i < num ; i++) {
					show_progress("  Image [%d] geometry [%dx%d+%d+%d]", i, layers[i].clip_width, layers[i].clip_height, layers[i].dst_x, layers[i].dst_y);
					if (layers[i].tint) show_progress(" tint (#%08x)", (unsigned int)layers[i].tint);
				}
			}

			if (num) {
				result = merge_layers(asv, layers, num, width, height, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
				if (keep_trans && result && layers[0].im) {
					copy_asimage_channel(result, IC_ALPHA, layers[0].im, IC_ALPHA);
				}
				while (--num >= 0) safe_asimage_destroy(layers[num].im);
			}
			free(layers);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

	/* No match so far... see if one of our children can do any better.*/
	if (!result) {
		xml_elem_t* tparm = NULL;
		for (ptr = doc->child ; ptr && !result ; ptr = ptr->next) {
			xml_elem_t* sparm = NULL;
			ASImage* imtmp = build_image_from_xml(asv, imman, fontman, ptr, &sparm, flags, verbose, display_win);
			if (imtmp) {
				if (tparm) xml_elem_delete(NULL, tparm);
				tparm = NULL;
				if (sparm) tparm = sparm; else xml_elem_delete(NULL, sparm);
			}
		}
	}

	if (id && result) {
		show_progress("Storing image id [%s].", id);
		if( !store_asimage( imman, result, id ) ) 
		{
			safe_asimage_destroy(result );
			result = fetch_asimage( imman, id );	 
		}
	}

	return result;
}



xml_elem_t* xml_parse_parm(const char* parm) {
	xml_elem_t* list = NULL;
	const char* eparm;

	if (!parm) return NULL;

	for (eparm = parm ; *eparm ; ) {
		xml_elem_t* p;
		const char* bname;
		const char* ename;
		const char* bval;
		const char* eval;

		/* Spin past any leading whitespace. */
		for (bname = eparm ; isspace((int)*bname) ; bname++);

		/* Check for a parm.  First is the parm name. */
		for (ename = bname ; xml_tagchar((int)*ename) ; ename++);

		/* No name equals no parm equals broken tag. */
		if (!*ename) { eparm = NULL; break; }

		/* No "=" equals broken tag.  We do not support HTML-style parms */
		/* with no value.                                                */
		for (bval = ename ; isspace((int)*bval) ; bval++);
		if (*bval != '=') { eparm = NULL; break; }

		while (isspace((int)*++bval));

		/* If the next character is a quote, spin until we see another one. */
		if (*bval == '"' || *bval == '\'') {
			char quote = *bval;
			bval++;
			for (eval = bval ; *eval && *eval != quote ; eval++);
		} else {
			for (eval = bval ; *eval && !isspace((int)*eval) ; eval++);
		}

		for (eparm = eval ; *eparm && !isspace((int)*eparm) ; eparm++);

		/* Add the parm to our list. */
		p = xml_elem_new();
		if (!list) list = p;
		else { p->next = list; list = p; }
		p->tag = lcstring(mystrndup(bname, ename - bname));
		p->parm = mystrndup(bval, eval - bval);
	}

	if (!eparm) {
		while (list) {
			xml_elem_t* p = list->next;
			free(list->tag);
			free(list->parm);
			free(list);
			list = p;
		}
	}

	return list;
}

void xml_print(xml_elem_t* root) {
	xml_elem_t* child;
	if (!strcmp(root->tag, cdata_str)) {
		fprintf(stderr, "%s", root->parm);
	} else {
		fprintf(stderr, "<%s", root->tag);
		if (root->parm) {
			xml_elem_t* parm = xml_parse_parm(root->parm);
			while (parm) {
				xml_elem_t* p = parm->next;
				fprintf(stderr, " %s=\"%s\"", parm->tag, parm->parm);
				free(parm->tag);
				free(parm->parm);
				free(parm);
				parm = p;
			}
		}
		fprintf(stderr, ">");
		for (child = root->child ; child ; child = child->next) xml_print(child);
		fprintf(stderr, "</%s>", root->tag);
	}
}

xml_elem_t* xml_elem_new(void) {
	xml_elem_t* elem = NEW(xml_elem_t);
	elem->next = elem->child = NULL;
	elem->parm = elem->tag = NULL;
	return elem;
}

xml_elem_t* xml_elem_remove(xml_elem_t** list, xml_elem_t* elem) {
	/* Splice the element out of the list, if it's in one. */
	if (list) {
		if (*list == elem) {
			*list = elem->next;
		} else {
			xml_elem_t* ptr;
			for (ptr = *list ; ptr->next ; ptr = ptr->next) {
				if (ptr->next == elem) {
					ptr->next = elem->next;
					break;
				}
			}
		}
	}
	elem->next = NULL;
	return elem;
}

void xml_elem_delete(xml_elem_t** list, xml_elem_t* elem) {
	if (list) xml_elem_remove(list, elem);
	while (elem) {
		xml_elem_t* ptr = elem;
		elem = elem->next;
		if (ptr->child) xml_elem_delete(NULL, ptr->child);
		if (ptr->tag && ptr->tag != cdata_str && ptr->tag != container_str) free(ptr->tag);
		if (ptr->parm) free(ptr->parm);
		free(ptr);
	}
}

xml_elem_t* xml_parse_doc(const char* str) {
	xml_elem_t* elem = xml_elem_new();
	elem->tag = container_str;
	xml_parse(str, elem);
	return elem;
}

int xml_parse(const char* str, xml_elem_t* current) {
	const char* ptr = str;

	/* Find a tag of the form <tag opts>, </tag>, or <tag opts/>. */
	while (*ptr) {
		const char* oab = ptr;

		/* Look for an open oab bracket. */
		for (oab = ptr ; *oab && *oab != '<' ; oab++);

		/* If there are no oab brackets left, we're done. */
		if (*oab != '<') return oab - str;

		/* Does this look like a close tag? */
		if (oab[1] == '/') {
			const char* etag;
			/* Find the end of the tag. */
			for (etag = oab + 2 ; xml_tagchar((int)*etag) ; etag++);

			/* If this is an end tag, and the tag matches the tag we're parsing, */
			/* we're done.  If not, continue on blindly. */
			if (*etag == '>') {
				if (!strncasecmp(oab + 2, current->tag, etag - (oab + 2))) {
					if (oab - ptr) {
						xml_elem_t* child = xml_elem_new();
						child->tag = cdata_str;
						child->parm = mystrndup(ptr, oab - ptr);
						xml_insert(current, child);
					}
					return (etag + 1) - str;
				}
			}

			/* This tag isn't interesting after all. */
			ptr = oab + 1;
		}

		/* Does this look like a start tag? */
		if (oab[1] != '/') {
			int empty = 0;
			const char* btag = oab + 1;
			const char* etag;
			const char* bparm;
			const char* eparm;

			/* Find the end of the tag. */
			for (etag = btag ; xml_tagchar((int)*etag) ; etag++);

			/* If we reached the end of the document, continue on. */
			if (!*etag) { ptr = oab + 1; continue; }

			/* Find the beginning of the parameters, if they exist. */
			for (bparm = etag ; isspace((int)*bparm) ; bparm++);

			/* From here on, we're looking for a sequence of parms, which have
			 * the form [a-z0-9-]+=("[^"]"|'[^']'|[^ \t\n]), followed by either
			 * a ">" or a "/>". */
			for (eparm = bparm ; *eparm ; ) {
				const char* tmp;

				/* Spin past any leading whitespace. */
				for ( ; isspace((int)*eparm) ; eparm++);

				/* Are we at the end of the tag? */
				if (*eparm == '>' || (*eparm == '/' && eparm[1] == '>')) break;

				/* Check for a parm.  First is the parm name. */
				for (tmp = eparm ; xml_tagchar((int)*tmp) ; tmp++);

				/* No name equals no parm equals broken tag. */
				if (!*tmp) { eparm = NULL; break; }

				/* No "=" equals broken tag.  We do not support HTML-style parms
				   with no value. */
				for ( ; isspace((int)*tmp) ; tmp++);
				if (*tmp != '=') { eparm = NULL; break; }

				while (isspace((int)*++tmp));

				/* If the next character is a quote, spin until we see another one. */
				if (*tmp == '"' || *tmp == '\'') {
					char quote = *tmp;
					for (tmp++ ; *tmp && *tmp != quote ; tmp++);
				}

				/* Now look for a space or the end of the tag. */
				for ( ; *tmp && !isspace((int)*tmp) && *tmp != '>' && !(*tmp == '/' && tmp[1] == '>') ; tmp++);

				/* If we reach the end of the string, there cannot be a '>'. */
				if (!*tmp) { eparm = NULL; break; }

				/* End of the parm.  */
				if (!isspace((int)*tmp)) { eparm = tmp; break; }

				eparm = tmp;
			}

			/* If eparm is NULL, the parm string is invalid, and we should
			 * abort processing. */
			if (!eparm) { ptr = oab + 1; continue; }

			/* Save CDATA, if there is any. */
			if (oab - ptr) {
				xml_elem_t* child = xml_elem_new();
				child->tag = cdata_str;
				child->parm = mystrndup(ptr, oab - ptr);
				xml_insert(current, child);
			}

			/* We found a tag!  Advance the pointer. */
			for (ptr = eparm ; isspace((int)*ptr) ; ptr++);
			empty = (*ptr == '/');
			ptr += empty + 1;

			/* Add the tag to our children and parse it. */
			{
				xml_elem_t* child = xml_elem_new();
				child->tag = lcstring(mystrndup(btag, etag - btag));
				if (eparm - bparm) child->parm = mystrndup(bparm, eparm - bparm);
				xml_insert(current, child);
				if (!empty) ptr += xml_parse(ptr, child);
			}
		}
	}
	return ptr - str;
}

void xml_insert(xml_elem_t* parent, xml_elem_t* child) {
	child->next = NULL;
	if (!parent->child) {
		parent->child = child;
		return;
	}
	for (parent = parent->child ; parent->next ; parent = parent->next);
	parent->next = child;
}

char* lcstring(char* str) {
	char* ptr = str;
	for ( ; *ptr ; ptr++) if (isupper((int)*ptr)) *ptr = tolower((int)*ptr);
	return str;
}



