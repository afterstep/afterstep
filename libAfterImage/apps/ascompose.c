/*
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

#warning TODO: No-X support (#ifndef X_DISPLAY_MISSING).
#warning TODO: -onroot.
#warning TODO: Document undocumented options.

#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libAfterImage/afterbase.h>
#include <libAfterImage/afterimage.h>
#include "common.h"

#define xml_tagchar(a) (isalnum(a) || (a) == '-' || (a) == '_')

// We don't trust the math library to actually provide this number.
#undef PI
#define PI 180

typedef struct xml_elem_t {
	struct xml_elem_t* next;
	struct xml_elem_t* child;
	char* tag;
	char* parm;
} xml_elem_t;

char* load_file(const char* filename);
void showimage(ASImage* im);
ASImage* build_image_from_xml(xml_elem_t* doc, xml_elem_t** rparm);
void my_destroy_asimage(ASImage* image);
double parse_math(const char* str, char** endptr, double size);
xml_elem_t* xml_parse_parm(const char* parm);
void xml_print(xml_elem_t* root);
xml_elem_t* xml_elem_new(void);
xml_elem_t* xml_elem_remove(xml_elem_t** list, xml_elem_t* elem);
void xml_elem_delete(xml_elem_t** list, xml_elem_t* elem);
xml_elem_t* xml_parse_doc(const char* str);
int xml_parse(const char* str, xml_elem_t* current);
void xml_insert(xml_elem_t* parent, xml_elem_t* child);
char* lcstring(char* str);
Bool save_file(const char* file2bsaved, ASImage *im, const char* type);

Pixmap GetRootPixmap (Atom id);

Display* dpy = NULL;
int screen = 0, depth = 0;

ASVisual *asv;
int verbose = 0;
ASHashTable* image_hash = NULL;
struct ASFontManager *fontman = NULL;

char* default_doc_str = "
<composite op=hue>
  <composite op=add>
    <scale width=512 height=384><img src=rose512.jpg/></scale>
    <tile width=512 height=384><img src=back.xpm/></tile>
  </composite>
  <tile width=512 height=384><img src=fore.xpm/></tile>
</composite>
";

void version(void) {
	printf("ascompose version 1.2\n");
}

void usage(void) {
#ifndef X_DISPLAY_MISSING
	printf(
		"Usage:\n"
		"ascompose [-h] [-f file] [-n] [-o file] [-r] [-s string] [-t type] [-v] [-V]\n"
		"  -h --help          display this help and exit\n"
		"  -f --file file     an XML file to use as input\n"
		"  -n --no-display    don't display the final image\n"
		"  -o --output file   output to file\n"
		"  -r --root-window   draw result image on root window\n"
		"  -s --string string an XML string to use as input\n"
		"  -t --type type     type of file to output to\n"
		"  -v --version       display version and exit\n"
		"  -V --verbose       increase verbosity\n"
	);
#else /* X_DISPLAY_MISSING */
	printf(
		"Usage:\n"
		"ascompose [-h] [-f file] [-o file] [-s string] [-t type] [-v] [-V]\n"
		"  -h --help          display this help and exit\n"
		"  -f --file file     an XML file to use as input\n"
		"  -o --output file   output to file\n"
		"  -s --string string an XML string to use as input\n"
		"  -t --type type     type of file to output to\n"
		"  -v --version       display version and exit\n"
		"  -V --verbose       increase verbosity\n"
	);
#endif /* X_DISPLAY_MISSING */
}

int main(int argc, char** argv) {
	ASImage* im = NULL;
	xml_elem_t* doc;
	char* doc_str = default_doc_str;
	char* doc_file = NULL;
	char* doc_save = NULL;
	char* doc_save_type = NULL;
	int i;
	int display = 1;

	/* see ASView.1 : */
	set_application_name(argv[0]);

	// Parse command line.
	for (i = 1 ; i < argc ; i++) {
		if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			version();
			usage();
			exit(0);
		} else if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-v")) {
			version();
			exit(0);
		} else if (!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-V")) {
			verbose++;
		} else if (!strcmp(argv[i], "--no-display") || !strcmp(argv[i], "-n")) {
			display = 0;
		} else if ((!strcmp(argv[i], "--file") || !strcmp(argv[i], "-f")) && i < argc + 1) {
			doc_file = argv[++i];
		} else if ((!strcmp(argv[i], "--string") || !strcmp(argv[i], "-s")) && i < argc + 1) {
			doc_str = argv[++i];
		} else if ((!strcmp(argv[i], "--output") || !strcmp(argv[i], "-o")) && i < argc + 1) {
			doc_save = argv[++i];
		} else if ((!strcmp(argv[i], "--type") || !strcmp(argv[i], "-t")) && i < argc + 1) {
			doc_save_type = argv[++i];
		}
	}

	// Load the document from file, if one was given.
	if (doc_file) {
		doc_str = load_file(doc_file);
		if (!doc_str) {
			fprintf(stderr, "Unable to load file [%s]: %s.\n", doc_file, strerror(errno));
			exit(1);
		}
	}

#ifndef X_DISPLAY_MISSING
	dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth(dpy, screen);
#endif

	asv = create_asvisual(dpy, screen, depth, NULL);

	doc = xml_parse_doc(doc_str);
	if (verbose > 1) {
		xml_print(doc);
		printf("\n");
	}

	// Initialize the image hash.
	image_hash = create_ashash(53, &string_hash_value, &string_compare, &string_destroy);

	// Build the image(s) from the xml document structure.
	if (doc) {
		xml_elem_t* ptr;
		for (ptr = doc->child ; ptr ; ptr = ptr->next) {
			ASImage* tmpim = build_image_from_xml(ptr, NULL);
			if (tmpim && im) my_destroy_asimage(im);
			if (tmpim) im = tmpim;
		}
	}

	// Destroy the font manager, if we created one.
	if (fontman) destroy_font_manager(fontman, False);

	// Automagically determine the output type, if none was given.
	if (doc_save && !doc_save_type) {
		doc_save_type = strrchr(doc_save, '.');
		if (doc_save_type) doc_save_type++;
	}

	// Save the result image if desired.
	if (doc_save && doc_save_type) {
		if(!save_file(doc_save, im, doc_save_type)) {
			printf("Save failed.\n");
		} else {
			printf("Save successful.\n");
		}
	}

	// Display the image if desired.
	if (display == 1) {
		showimage(im);
	}

	destroy_asimage(&im);
#ifdef DEBUG_ALLOCS
	print_unfreed_mem();
#endif

	return 0;
}

Bool save_file(const char* file2bsaved, ASImage *im, const char* strtype) {

	int type;

	if (!mystrcasecmp(strtype, "jpeg") || !mystrcasecmp(strtype, "jpg"))  {
		type = ASIT_Jpeg;
	} else if (!mystrcasecmp(strtype, "bitmap") || !mystrcasecmp(strtype, "bmp")) {
		type = ASIT_Bmp;
	} else if (!mystrcasecmp(strtype, "png")) {
		type = ASIT_Png;
	} else if (!mystrcasecmp(strtype, "xcf")) {
		type = ASIT_Xcf;
	} else if (!mystrcasecmp(strtype, "ppm")) {
		type = ASIT_Ppm;
	} else if (!mystrcasecmp(strtype, "pnm")) {
		type = ASIT_Pnm;
	} else if (!mystrcasecmp(strtype, "ico")) {
		type = ASIT_Ico;
	} else if (!mystrcasecmp(strtype, "cur")) {
		type = ASIT_Cur;
	} else if (!mystrcasecmp(strtype, "gif")) {
		type = ASIT_Gif;
	} else if (!mystrcasecmp(strtype, "xbm")) {
		type = ASIT_Xbm;
	} else if (!mystrcasecmp(strtype, "tiff")) {
		type = ASIT_Tiff;
	} else {
		printf("File type not found.\n");
		return(0);
	}

	return ASImage2file(im, NULL, file2bsaved, type, NULL);

}

char* load_file(const char* filename) {
	struct stat st;
	FILE* fp;
	char* str;
	int len;

	// Get the file size.
	if (stat(filename, &st)) return NULL;

	// Open the file.
	fp = fopen(filename, "rb");
	if (!(fp = fopen(filename, "rb"))) return NULL;

	// Read in the file.
	str = NEW_ARRAY(char, st.st_size + 1);
	len = fread(str, 1, st.st_size, fp);
	if (len >= 0) str[len] = '\0';

	fclose(fp);

	return str;
}

void showimage(ASImage* im) {
#ifndef X_DISPLAY_MISSING
	if( im != NULL )
	{
		/* see ASView.4 : */
		Window w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
			                         im->width, im->height, 1, 0, NULL,
									 "ASView" );
		if( w != None )
		{
			Pixmap p ;

			XSelectInput (dpy, w, (StructureNotifyMask | ButtonPress));
	  		XMapRaised   (dpy, w);
			/* see ASView.5 : */
			p = asimage2pixmap( asv, DefaultRootWindow(dpy), im, NULL,
				                False );
			/* see common.c:set_window_background_and_free(): */
			p = set_window_background_and_free( w, p );
		}
		/* see ASView.6 : */
	    while(w != None)
  		{
    		XEvent event ;
	        XNextEvent (dpy, &event);
  		    switch(event.type)
			{
		  		case ButtonPress:
					break ;
	  		    case ClientMessage:
			        if ((event.xclient.format == 32) &&
	  			        (event.xclient.data.l[0] == _XA_WM_DELETE_WINDOW))
		  			{
						XDestroyWindow( dpy, w );
						XFlush( dpy );
						w = None ;
				    }
					break;
			}
  		}
	}

    if( dpy )
        XCloseDisplay (dpy);
#endif /* X_DISPLAY_MISSING */
}

// Each tag is only allowed to return ONE image.
ASImage* build_image_from_xml(xml_elem_t* doc, xml_elem_t** rparm) {
	xml_elem_t* ptr;
	char* id = NULL;
	ASImage* result = NULL;

	if (!strcmp(doc->tag, "img")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* src = NULL;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "src")) src = ptr->parm;
		}
		if (src && !strcmp(src, "xroot:")) {
			int width, height;
			Pixmap rp = GetRootPixmap(None);
			if (verbose) printf("Getting root pixmap.\n");
			if (rp) {
				get_drawable_size(rp, &width, &height);
				result = pixmap2asimage(asv, rp, 0, 0, width, height, 0xFFFFFFFF, False, 100);
			}
		} else if (src) {
			if (verbose) printf("Loading image [%s].\n", src);
			result = file2ASImage(src, 0xFFFFFFFF, SCREEN_GAMMA, 100, NULL);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

	if (!strcmp(doc->tag, "recall")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* srcid = NULL;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "srcid")) srcid = ptr->parm;
		}
		if (srcid) {
			if (verbose) printf("Recalling image id [%s].\n", srcid);
			get_hash_item(image_hash, (ASHashableValue)(char*)srcid, (void**)&result);
			if (result) result->ref_count++;
			if (verbose > 1 && !result) printf("Image recall failed.\n");
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

	if (!strcmp(doc->tag, "text")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* text = NULL;
		const char* font_name = "fixed";
		const char* fgimage_str = NULL;
		const char* bgimage_str = NULL;
		const char* fgcolor_str = NULL;
		const char* bgcolor_str = NULL;
		ARGB32 fgcolor = ARGB32_White, bgcolor = ARGB32_Black;
		int point = 12;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "font")) font_name = ptr->parm;
			if (!strcmp(ptr->tag, "point")) point = strtol(ptr->parm, NULL, 0);
			if (!strcmp(ptr->tag, "fgimage")) fgimage_str = ptr->parm;
			if (!strcmp(ptr->tag, "bgimage")) bgimage_str = ptr->parm;
			if (!strcmp(ptr->tag, "fgcolor")) fgcolor_str = ptr->parm;
			if (!strcmp(ptr->tag, "bgcolor")) bgcolor_str = ptr->parm;
		}
		for (ptr = doc->child ; ptr && !result ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "CDATA")) text = ptr->parm;
		}
		if (text && point > 0) {
			struct ASFont *font = NULL;
			if (verbose) printf("Rendering text [%s] with font [%s] size [%d].\n", text, font_name, point);
			if (!fontman) fontman = create_font_manager(dpy, NULL, NULL);
			if (fontman) font = get_asfont(fontman, font_name, 0, point, ASF_GuessWho);
			if (font != NULL) {
				set_asfont_glyph_spacing(font, 3, 0);
				result = draw_text(text, font, AST_Plain, 0);
				if (result && fgimage_str) {
					ASImage* fgimage = NULL;
					get_hash_item(image_hash, (ASHashableValue)(char*)fgimage_str, (void**)&fgimage);
					if (verbose > 1) printf("Using image [%s] as foreground.\n", fgimage_str);
					if (fgimage) {
						fgimage = tile_asimage(asv, fgimage, 0, 0, result->width, result->height, 0, ASA_ASImage, 100, ASIMAGE_QUALITY_TOP);
						move_asimage_channel(fgimage, IC_ALPHA, result, IC_ALPHA);
						my_destroy_asimage(result);
						result = fgimage;
					}
				}
				if (result && fgcolor_str) {
					ASImage* fgimage = create_asimage(result->width, result->height, ASIMAGE_QUALITY_TOP);
					parse_argb_color(fgcolor_str, &fgcolor);
					fill_asimage(asv, fgimage, 0, 0, result->width, result->height, fgcolor);
					move_asimage_channel(fgimage, IC_ALPHA, result, IC_ALPHA);
					my_destroy_asimage(result);
					result = fgimage;
				}
				if (result && (bgcolor_str || bgimage_str)) {
					ASImageLayer layers[2];
					ASImage* bgimage = NULL;
					if (bgimage_str) get_hash_item(image_hash, (ASHashableValue)(char*)bgimage_str, (void**)&bgimage);
					if (bgcolor_str) parse_argb_color(bgcolor_str, &bgcolor);
					init_image_layers(&(layers[0]), 2);
					bgimage->back_color = bgcolor ;
					result->back_color = fgcolor ;
					layers[0].im = bgimage;
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
				}
			}
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

	if (!strcmp(doc->tag, "save")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* dst = NULL;
		const char* ext = NULL;
		int autoext = 0;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "dst")) dst = ptr->parm;
			if (!strcmp(ptr->tag, "format")) ext = ptr->parm;
		}
		if (dst && !ext) {
			ext = strrchr(dst, '.');
			if (ext) ext++;
			autoext = 1;
		}
		if (dst && ext) {
			for (ptr = doc->child ; ptr && !result ; ptr = ptr->next) {
				result = build_image_from_xml(ptr, NULL);
			}
			if (verbose > 1 && autoext)
				printf("No format given.  File extension [%s] used as format.\n", ext);
			if (verbose) printf("Saving image to file [%s].\n", dst);
			if (result && !save_file(dst, result, ext)) {
				fprintf(stderr, "Unable to save image.\n");
			}
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

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
			imtmp = build_image_from_xml(ptr, NULL);
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
				while (isspace(*p)) p++;
				parse_argb_color(p, &bevel.hi_color);
				while (*p && !isspace(*p)) p++;
				while (isspace(*p)) p++;
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
			if (verbose) printf("Generating bevel with offsets [%d %d %d %d] and colors [#%08x #%08x].\n", bevel.left_inline, bevel.top_inline, bevel.right_inline, bevel.bottom_inline, (unsigned int)bevel.hi_color, (unsigned int)bevel.lo_color);
			init_image_layers( &layer, 1 );
			layer.im = imtmp;
			layer.clip_width = imtmp->width;
			layer.clip_height = imtmp->height;
			layer.bevel = &bevel;
			result = merge_layers(asv, &layer, 1, imtmp->width, imtmp->height, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
			my_destroy_asimage(imtmp);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

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
			ASImage* refimg = NULL;
			get_hash_item(image_hash, (ASHashableValue)(char*)refid, (void**)&refimg);
			if (refimg) {
				width = parse_math(width_str, NULL, refimg->width);
				height = parse_math(height_str, NULL, refimg->height);
			}
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
			for (p = color_str ; isspace(*p) ; p++);
			for (npoints1 = 0 ; *p ; npoints1++) {
				if (*p) for ( ; *p && !isspace(*p) ; p++);
				for ( ; isspace(*p) ; p++);
			}
			if (offset_str) {
				for (p = offset_str ; isspace(*p) ; p++);
				for (npoints2 = 0 ; *p ; npoints2++) {
					if (*p) for ( ; *p && !isspace(*p) ; p++);
					for ( ; isspace(*p) ; p++);
				}
			}
			if (npoints1 > 1) {
				int i;
				if (offset_str && npoints1 > npoints2) npoints1 = npoints2;
				gradient.color = NEW_ARRAY(ARGB32, npoints1);
				gradient.offset = NEW_ARRAY(double, npoints1);
				for (p = color_str ; isspace(*p) ; p++);
				for (npoints1 = 0 ; *p ; ) {
					char* pb = p, ch;
					if (*p) for ( ; *p && !isspace(*p) ; p++);
					for ( ; isspace(*p) ; p++);
					ch = *p; *p = '\0';
					if (parse_argb_color(pb, gradient.color + npoints1)) npoints1++;
					*p = ch;
				}
				if (offset_str) {
					for (p = offset_str ; isspace(*p) ; p++);
					for (npoints2 = 0 ; *p ; ) {
						char* pb = p, ch;
						if (*p) for ( ; *p && !isspace(*p) ; p++);
						ch = *p; *p = '\0';
						gradient.offset[npoints2] = strtod(pb, &pb);
						if (pb == p) npoints2++;
						*p = ch;
						for ( ; isspace(*p) ; p++);
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
				if (verbose) printf("Generating [%dx%d] gradient with angle [%f] and npoints [%d/%d].\n", width, height, angle, npoints1, npoints2);
				if (verbose > 1) {
					for (i = 0 ; i < gradient.npoints ; i++) {
						printf("  Point [%d] has color [#%08x] and offset [%f].\n", i, (unsigned int)gradient.color[i], gradient.offset[i]);
					}
				}
				result = make_gradient(asv, &gradient, width, height, SCL_DO_ALL, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
			}
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

	if (!strcmp(doc->tag, "mirror")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		ASImage* imtmp = NULL;
		int dir = 0;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "dir")) dir = !mystrcasecmp(ptr->parm, "vertical");
		}
		for (ptr = doc->child ; ptr && !imtmp ; ptr = ptr->next) {
			imtmp = build_image_from_xml(ptr, NULL);
		}
		if (imtmp) {
			result = mirror_asimage(asv, imtmp, 0, 0, imtmp->width, imtmp->height, dir, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
			my_destroy_asimage(imtmp);
		}
		if (verbose) printf("Mirroring image [%sally].\n", dir ? "horizont" : "vertic");
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

	if (!strcmp(doc->tag, "rotate")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		ASImage* imtmp = NULL;
		double angle = 0;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "angle")) angle = strtod(ptr->parm, NULL);
		}
		for (ptr = doc->child ; ptr && !imtmp ; ptr = ptr->next) {
			imtmp = build_image_from_xml(ptr, NULL);
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
				my_destroy_asimage(imtmp);
				if (verbose) printf("Rotating image [%f degrees].\n", angle);
			} else {
				result = imtmp;
			}
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

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
			ASImage* refimg = NULL;
			get_hash_item(image_hash, (ASHashableValue)(char*)refid, (void**)&refimg);
			if (refimg) {
				width = parse_math(width_str, NULL, refimg->width);
				height = parse_math(height_str, NULL, refimg->height);
			}
		}
		if (!refid && width_str && height_str) {
			width = parse_math(width_str, NULL, width);
			height = parse_math(height_str, NULL, height);
		}
		if (width && height) {
			ASImage* imtmp = NULL;
			for (ptr = doc->child ; ptr && !imtmp ; ptr = ptr->next) {
				imtmp = build_image_from_xml(ptr, NULL);
			}
			if (imtmp) {
				result = scale_asimage(asv, imtmp, width, height, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT);
				my_destroy_asimage(imtmp);
			}
			if (verbose) printf("Scaling image to [%dx%d].\n", width, height);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

	if (!strcmp(doc->tag, "crop")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* refid = NULL;
		const char* srcx_str = NULL;
		const char* srcy_str = NULL;
		const char* width_str = NULL;
		const char* height_str = NULL;
		int width = 0, height = 0, srcx = 0, srcy = 0;
		ASImage* imtmp = NULL;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "refid")) refid = ptr->parm;
			if (!strcmp(ptr->tag, "srcx")) srcx_str = ptr->parm;
			if (!strcmp(ptr->tag, "srcy")) srcy_str = ptr->parm;
			if (!strcmp(ptr->tag, "width")) width_str = ptr->parm;
			if (!strcmp(ptr->tag, "height")) height_str = ptr->parm;
		}
		for (ptr = doc->child ; ptr && !imtmp ; ptr = ptr->next) {
			imtmp = build_image_from_xml(ptr, NULL);
		}
		if (imtmp) {
			width = imtmp->width;
			height = imtmp->height;
			if (refid) {
				ASImage* refimg = NULL;
				get_hash_item(image_hash, (ASHashableValue)(char*)refid, (void**)&refimg);
				if (refimg) {
					width = refimg->width;
					height = refimg->height;
				}
			}
			if (srcx_str) srcx = parse_math(srcx_str, NULL, width);
			if (srcy_str) srcy = parse_math(srcy_str, NULL, height);
			if (width_str) width = parse_math(width_str, NULL, width);
			if (height_str) height = parse_math(height_str, NULL, height);
			if (width > imtmp->width) width = imtmp->width;
			if (height > imtmp->height) height = imtmp->height;
			if (width > 0 && height > 0) {
				result = tile_asimage(asv, imtmp, srcx, srcy, width, height, 0, ASA_ASImage, 100, ASIMAGE_QUALITY_TOP);
				my_destroy_asimage(imtmp);
			}
			if (verbose) printf("Cropping image to [%dx%d].\n", width, height);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

	if (!strcmp(doc->tag, "tile")) {
		xml_elem_t* parm = xml_parse_parm(doc->parm);
		const char* refid = NULL;
		const char* width_str = "100%";
		const char* height_str = "100%";
		int width = 0, height = 0;
		ASImage* imtmp = NULL;
		for (ptr = parm ; ptr ; ptr = ptr->next) {
			if (!strcmp(ptr->tag, "id")) id = strdup(ptr->parm);
			if (!strcmp(ptr->tag, "refid")) refid = ptr->parm;
			if (!strcmp(ptr->tag, "width")) width_str = ptr->parm;
			if (!strcmp(ptr->tag, "height")) height_str = ptr->parm;
		}
		for (ptr = doc->child ; ptr && !imtmp ; ptr = ptr->next) {
			imtmp = build_image_from_xml(ptr, NULL);
		}
		if (imtmp) {
			width = imtmp->width;
			height = imtmp->height;
			if (refid) {
				ASImage* refimg = NULL;
				get_hash_item(image_hash, (ASHashableValue)(char*)refid, (void**)&refimg);
				if (refimg) {
					width = refimg->width;
					height = refimg->height;
				}
			}
			if (width_str) width = parse_math(width_str, NULL, width);
			if (height_str) height = parse_math(height_str, NULL, height);
			if (width > 0 && height > 0) {
				result = tile_asimage(asv, imtmp, 0, 0, width, height, 0, ASA_ASImage, 100, ASIMAGE_QUALITY_TOP);
				my_destroy_asimage(imtmp);
			}
			if (verbose) printf("Tiling image to [%dx%d].\n", width, height);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

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
			ASImage* refimg = NULL;
			get_hash_item(image_hash, (ASHashableValue)(char*)refid, (void**)&refimg);
			if (refimg) {
				width = parse_math(width_str, NULL, refimg->width);
				height = parse_math(height_str, NULL, refimg->height);
			}
		}
		if (!refid && width_str && height_str) {
			width = parse_math(width_str, NULL, 0);
			height = parse_math(height_str, NULL, 0);
		}
		if (width && height) {
			result = create_asimage(width, height, ASIMAGE_QUALITY_TOP);
			if (result) fill_asimage(asv, result, 0, 0, width, height, color);
			if (verbose) printf("Creating solid color [#%08x] image [%dx%d].\n", (unsigned int)color, width, height);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

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
		// Find out how many subimages we have.
		num = 0;
		for (ptr = doc->child ; ptr ; ptr = ptr->next) {
			if (strcmp(ptr->tag, "CDATA")) num++;
		}
		if (num) {
			int width = 0, height = 0;
			ASImageLayer *layers;

			// Build the layers first.
			layers = create_image_layers( num );
			for (num = 0, ptr = doc->child ; ptr ; ptr = ptr->next) {
				int x = 0, y = 0;
				ARGB32 tint = 0;
				xml_elem_t* sparm = NULL;
				if (!strcmp(ptr->tag, "CDATA")) continue;
				layers[num].im = build_image_from_xml(ptr, &sparm);
				if (sparm) {
					xml_elem_t* tmp;
					const char* x_str = NULL;
					const char* y_str = NULL;
					const char* refid = NULL;
					for (tmp = sparm ; tmp ; tmp = tmp->next) {
						if (!strcmp(tmp->tag, "refid")) refid = tmp->parm;
						if (!strcmp(tmp->tag, "x")) x_str = tmp->parm;
						if (!strcmp(tmp->tag, "y")) y_str = tmp->parm;
						if (!strcmp(tmp->tag, "tint")) parse_argb_color(tmp->parm, &tint);
					}
					if (refid) {
						ASImage* refimg = NULL;
						get_hash_item(image_hash, (ASHashableValue)(char*)refid, (void**)&refimg);
						if (refimg) {
							x = refimg->width;
							y = refimg->height;
						}
					}
					x = x_str ? parse_math(x_str, NULL, x) : 0;
					y = y_str ? parse_math(y_str, NULL, y) : 0;
				}
				if (layers[num].im) {
					layers[num].dst_x = x;
					layers[num].dst_y = y;
					layers[num].clip_x = 0;
					layers[num].clip_y = 0;
					layers[num].clip_width = layers[num].im->width;
					layers[num].clip_height = layers[num].im->height;
					layers[num].tint = tint;
					layers[num].bevel = 0;
					layers[num].merge_scanlines = blend_scanlines_name2func(pop);
					if (width < layers[num].im->width) width = layers[num].im->width;
					if (height < layers[num].im->height) height = layers[num].im->height;
					num++;
				}
				if (sparm) xml_elem_delete(NULL, sparm);
			}

			if (merge) {
				width = layers[0].im->width;
				height = layers[0].im->height;
			}

			if (verbose) {
				printf("Compositing [%d] image(s) with op [%s].  Final geometry [%dx%d].", num, pop, width, height);
				if (keep_trans) printf("  Keeping transparency.");
				printf("\n");
			}
			if (verbose > 1) {
				int i;
				for (i = 0 ; i < num ; i++) {
					printf("  Image [%d] geometry [%dx%d+%d+%d]", i, layers[i].clip_width, layers[i].clip_height, layers[i].dst_x, layers[i].dst_y);
					if (layers[i].tint) printf(" tint (#%08x)", (unsigned int)layers[i].tint);
					printf(".\n");
				}
			}

			if (num) {
				result = merge_layers(asv, layers, num, width, height, ASA_ASImage, 0, ASIMAGE_QUALITY_DEFAULT);
				if (keep_trans && result && layers[0].im) {
					// FIXME: This should be copy_asimage_channel(), or random crashes may occur.
					move_asimage_channel(result, IC_ALPHA, layers[0].im, IC_ALPHA);
				}
				while (--num >= 0) my_destroy_asimage(layers[num].im);
			}
			free(layers);
		}
		if (rparm) *rparm = parm; else xml_elem_delete(NULL, parm);
	}

	// No match so far... see if one of our children can do any better.
	if (!result) {
		xml_elem_t* tparm = NULL;
		for (ptr = doc->child ; ptr && !result ; ptr = ptr->next) {
			xml_elem_t* sparm = NULL;
			ASImage* imtmp = build_image_from_xml(ptr, &sparm);
			if (imtmp) {
				if (tparm) xml_elem_delete(NULL, tparm);
				tparm = NULL;
				if (sparm) tparm = sparm; else xml_elem_delete(NULL, sparm);
			}
		}
	}

	if (id && result) {
		ASImage* imtmp = NULL;
		if (verbose > 1) printf("Storing image id [%s].\n", id);
		remove_hash_item(image_hash, (ASHashableValue)id, (void**)&imtmp, 1);
		if (imtmp) my_destroy_asimage(imtmp);
		add_hash_item(image_hash, (ASHashableValue)id, result);
		result->ref_count++;
	}

	return result;
}

void my_destroy_asimage(ASImage* image) {
	image->ref_count--;
	if (verbose > 1 && image->ref_count < 0) printf("Destroying image [%08x] with refcount [%d].\n", (unsigned int)image, image->ref_count);
	if (image->ref_count < 0) destroy_asimage(&image);
}

// Math expression parsing algorithm.  The basic math ops (add, subtract, 
// multiply, divide), unary minus, and parentheses are supported.  
// Operator precedence is NOT supported.  Percentages are allowed, and 
// apply to the "size" parameter of this function.
double parse_math(const char* str, char** endptr, double size) {
	double total = 0;
	char op = '+';
	char minus = 0;
	const char* startptr = str;
	while (*str) {
		while (isspace(*str)) str++;
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
	if (verbose > 2) {
		printf("Parsed math [%s] with reference [%.2f] into number [%.2f].\n", startptr, size, total);
	}
	return total;
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

		// Spin past any leading whitespace.
		for (bname = eparm ; isspace(*bname) ; bname++);

		// Check for a parm.  First is the parm name.
		for (ename = bname ; xml_tagchar(*ename) ; ename++);

		// No name equals no parm equals broken tag.
		if (!*ename) { eparm = NULL; break; }

		// No "=" equals broken tag.  We do not support HTML-style parms
		// with no value.
		for (bval = ename ; isspace(*bval) ; bval++);
		if (*bval != '=') { eparm = NULL; break; }

		while (isspace(*++bval));

		// If the next character is a quote, spin until we see another one.
		if (*bval == '"' || *bval == '\'') {
			char quote = *bval;
			bval++;
			for (eval = bval ; *eval && *eval != quote ; eval++);
		} else {
			for (eval = bval ; *eval && !isspace(*eval) ; eval++);
		}

		for (eparm = eval ; *eparm && !isspace(*eparm) ; eparm++);

		// Add the parm to our list.
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
	if (!strcmp(root->tag, "CDATA")) {
		printf("%s", root->parm);
	} else {
		printf("<%s", root->tag);
		if (root->parm) {
			xml_elem_t* parm = xml_parse_parm(root->parm);
			while (parm) {
				xml_elem_t* p = parm->next;
				printf(" %s=\"%s\"", parm->tag, parm->parm);
				free(parm->tag);
				free(parm->parm);
				free(parm);
				parm = p;
			}
		}
		printf(">");
		for (child = root->child ; child ; child = child->next) xml_print(child);
		printf("</%s>", root->tag);
	}
}

xml_elem_t* xml_elem_new(void) {
	xml_elem_t* elem = NEW(xml_elem_t);
	elem->next = elem->child = NULL;
	elem->parm = elem->tag = NULL;
	return elem;
}

xml_elem_t* xml_elem_remove(xml_elem_t** list, xml_elem_t* elem) {
	// Splice the element out of the list, if it's in one.
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
		free(ptr->tag);
		free(ptr->parm);
		free(ptr);
	}
}

xml_elem_t* xml_parse_doc(const char* str) {
	xml_elem_t* elem = xml_elem_new();
	elem->tag = "CONTAINER";
	xml_parse(str, elem);
	return elem;
}

int xml_parse(const char* str, xml_elem_t* current) {
	const char* ptr = str;

	// Find a tag of the form <tag opts>, </tag>, or <tag opts/>.
	while (*ptr) {
		const char* oab = ptr;

		// Look for an open oab bracket.
		for (oab = ptr ; *oab && *oab != '<' ; oab++);

		// If there are no oab brackets left, we're done.
		if (*oab != '<') return oab - str;

		// Does this look like a close tag?
		if (oab[1] == '/') {
			const char* etag;
			// Find the end of the tag.
			for (etag = oab + 2 ; xml_tagchar(*etag) ; etag++);

			// If this is an end tag, and the tag matches the tag we're parsing,
			// we're done.  If not, continue on blindly.
			if (*etag == '>') {
				if (!strncasecmp(oab + 2, current->tag, etag - (oab + 2))) {
					if (oab - ptr) {
						xml_elem_t* child = xml_elem_new();
						child->tag = "CDATA";
						child->parm = mystrndup(ptr, oab - ptr);
						xml_insert(current, child);
					}
					return (etag + 1) - str;
				}
			}

			// This tag isn't interesting after all.
			ptr = oab + 1;
		}

		// Does this look like a start tag?
		if (oab[1] != '/') {
			int empty = 0;
			const char* btag = oab + 1;
			const char* etag;
			const char* bparm;
			const char* eparm;

			// Find the end of the tag.
			for (etag = btag ; xml_tagchar(*etag) ; etag++);

			// If we reached the end of the document, continue on.
			if (!*etag) { ptr = oab + 1; continue; }

			// Find the beginning of the parameters, if they exist.
			for (bparm = etag ; isspace(*bparm) ; bparm++);

			// From here on, we're looking for a sequence of parms, which have
			// the form [a-z0-9-]+=("[^"]"|'[^']'|[^ \t\n]), followed by either
			// a ">" or a "/>".
			for (eparm = bparm ; *eparm ; ) {
				const char* tmp;

				// Spin past any leading whitespace.
				for ( ; isspace(*eparm) ; eparm++);

				// Are we at the end of the tag?
				if (*eparm == '>' || (*eparm == '/' && eparm[1] == '>')) break;

				// Check for a parm.  First is the parm name.
				for (tmp = eparm ; xml_tagchar(*tmp) ; tmp++);

				// No name equals no parm equals broken tag.
				if (!*tmp) { eparm = NULL; break; }

				// No "=" equals broken tag.  We do not support HTML-style parms
				// with no value.
				for ( ; isspace(*tmp) ; tmp++);
				if (*tmp != '=') { eparm = NULL; break; }

				while (isspace(*++tmp));

				// If the next character is a quote, spin until we see another one.
				if (*tmp == '"' || *tmp == '\'') {
					char quote = *tmp;
					for (tmp++ ; *tmp && *tmp != quote ; tmp++);
				}

				// Now look for a space or the end of the tag.
				for ( ; *tmp && !isspace(*tmp) && *tmp != '>' && !(*tmp == '/' && tmp[1] == '>') ; tmp++);

				// If we reach the end of the string, there cannot be a '>'.
				if (!*tmp) { eparm = NULL; break; }

				// End of the parm.
				if (!isspace(*tmp)) { eparm = tmp; break; }

				eparm = tmp;
			}

			// If eparm is NULL, the parm string is invalid, and we should
			// abort processing.
			if (!eparm) { ptr = oab + 1; continue; }

			// Save CDATA, if there is any.
			if (oab - ptr) {
				xml_elem_t* child = xml_elem_new();
				child->tag = "CDATA";
				child->parm = mystrndup(ptr, oab - ptr);
				xml_insert(current, child);
			}

			// We found a tag!  Advance the pointer.
			for (ptr = eparm ; isspace(*ptr) ; ptr++);
			empty = (*ptr == '/');
			ptr += empty + 1;

			// Add the tag to our children and parse it.
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
	for ( ; *ptr ; ptr++) if (isupper(*ptr)) *ptr = tolower(*ptr);
	return str;
}

/* Stolen from libAfterStep. */
Pixmap GetRootPixmap (Atom id)
{
	Pixmap        currentRootPixmap = None;
#ifndef X_DISPLAY_MISSING

	if (id == None)
		id = XInternAtom (dpy, "_XROOTPMAP_ID", True);

	if (id != None)
	{
		Atom          act_type;
		int           act_format;
		unsigned long nitems, bytes_after;
		unsigned char *prop = NULL;

/*fprintf(stderr, "\n aterm GetRootPixmap(): root pixmap is set");                  */
		if (XGetWindowProperty (dpy, RootWindow(dpy, screen), id, 0, 1, False, XA_PIXMAP,
								&act_type, &act_format, &nitems, &bytes_after, &prop) == Success)
		{
			if (prop)
			{
				currentRootPixmap = *((Pixmap *) prop);
				XFree (prop);
/*fprintf(stderr, "\n aterm GetRootPixmap(): root pixmap is [%lu]", currentRootPixmap); */
			}
		}
	}
#endif /* X_DISPLAY_MISSING */
	return currentRootPixmap;
}
