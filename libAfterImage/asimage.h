#ifndef ASIMAGE_HEADER_FILE_INCLUDED
#define ASIMAGE_HEADER_FILE_INCLUDED

struct ScreenInfo;

#define MAX_IMPORT_IMAGE_SIZE 	4000

/* RLE format :
component := <line><line>...<line>
line      := <block><block>...<block><EOL>
block     := <EOL>|<simple_block>|<long_block>|<direct_block>

EOL       := 00000000 (all zero bits)

simple_block  := <ctrl_byte1><value_byte>
ctrl_byte1    := 00NNNNNN (first two bits are 0 remaining are length)

long_block    := <ctrl_byte2><more_length_byte><value_byte>
ctrl_byte2    := 01NNNNNN (NNNNNN are high 6 bits of length)
more_length_byte := low 8 bits of length

direct_block  := <ctrl_byte3><value_byte><value_byte>...<value_byte>
ctrl_byte3    := [1NNNNNNN|11111111] (first bit is 1, remaining are length.
 * 				If it is all 1's - then remaining part of the line up untill
 * 			    image width is monolitic uncompressed direct block)

*/

/* this is value */
#define RLE_EOL			0x00
/* this are masks */
#define RLE_DIRECT_B 		0x80
#define RLE_DIRECT_TAIL		0xFF
#define RLE_LONG_B 		0x40
/* this one is inverted mask */
#define RLE_SIMPLE_B_INV  	0xC0

/* this are masks to filter out control bits: */
#define RLE_DIRECT_D 		0x7F
#define RLE_LONG_D 		0x3F
#define RLE_SIMPLE_D  		0x3F

#define RLE_MAX_DIRECT_LEN      (RLE_DIRECT_D-1)
#define RLE_MAX_SIMPLE_LEN     	63
#define RLE_MAX_LONG_LEN     	(64*256)
#define RLE_THRESHOLD		1

typedef struct ASImage
{
  unsigned int width, height;
  CARD8 **red, **green, **blue;
  CARD8 **alpha;
  CARD8 **channels[IC_NUM_CHANNELS];/* merely a shortcut for faster translation to the
									 * above pointers*/

  CARD8 *buffer;
  unsigned int buf_used, buf_len;

  unsigned int max_compressed_width;           /* effectively limits compression to speed things up */

  XImage *ximage ;
}
ASImage;

/* Auxilary data structures : */
typedef struct ASImageDecoder
{
	ScreenInfo 	   *scr;
	ASImage 	   *im ;
	ASFlagType 		filter;
	unsigned int    offset_x,    /* left margin on source image before which we skip everything */
					out_width;   /* actuall length of the output scanline */
	unsigned int 	offset_y;	 /* top margin */
								 /* there is no need for out_height - if we go out of the
								  * image size - we simply reread lines from the beginning
                                  */
	ASScanline 		buffer;
	int 			next_line ;
}ASImageDecoder;

/* This is static piece of data that tell us what is the status of
 * the output stream, and allows us to easily put stuff out :       */
#define ASIMAGE_QUALITY_DEFAULT	-1
#define ASIMAGE_QUALITY_POOR	0
#define ASIMAGE_QUALITY_FAST	1
#define ASIMAGE_QUALITY_GOOD	2
#define ASIMAGE_QUALITY_TOP		3

struct ASImageOutput;
struct ASScanline;
typedef void (*encode_image_scanline_func)( struct ASImageOutput *imout, ASScanline *to_store );
typedef void (*output_image_scanline_func)( struct ASImageOutput *, ASScanline *, int );

typedef struct ASImageOutput
{
	ScreenInfo *scr;
	ASImage *im ;
	XImage *xim ;
	Bool to_xim ;
	unsigned char *xim_line;
	int            height, bpl;
	ASScanline buffer[2], *used, *available;
	int buffer_shift;   /* -1 means - buffer is empty */
	int next_line ;
	unsigned int tiling_step;       /* each line written will be repeated with this
									 * step untill we exceed image size */
	int quality ;/* see above */
	output_image_scanline_func output_image_scanline ;  /* high level interface -
														 * division/error diffusion
														 * as well as encoding */
	encode_image_scanline_func encode_image_scanline ;  /* low level interface -
														 * encoding only */
}ASImageOutput;

/* it produces  bottom = bottom <merge> top */
typedef void (*merge_scanlines_func)( ASScanline *bottom, ASScanline *top, int mode);
void alphablend_scanlines( ASScanline *bottom, ASScanline *top, int unused );
void allanon_scanlines( ASScanline *bottom, ASScanline *top, int unused );
void tint_scanlines( ASScanline *bottom, ASScanline *top, int unused );
void add_scanlines( ASScanline *bottom, ASScanline *top, int unused );
void sub_scanlines( ASScanline *bottom, ASScanline *top, int unused );
void diff_scanlines( ASScanline *bottom, ASScanline *top, int unused );
void darken_scanlines( ASScanline *bottom, ASScanline *top, int unused );
void lighten_scanlines( ASScanline *bottom, ASScanline *top, int unused );
void screen_scanlines( ASScanline *bottom, ASScanline *top, int unused );
void overlay_scanlines( ASScanline *bottom, ASScanline *top, int unused );
void hue_scanlines( ASScanline *bottom, ASScanline *top, int mode );
void saturate_scanlines( ASScanline *bottom, ASScanline *top, int mode );
void value_scanlines( ASScanline *bottom, ASScanline *top, int mode );
void colorize_scanlines( ASScanline *bottom, ASScanline *top, int mode );
void dissipate_scanlines( ASScanline *bottom, ASScanline *top, int unused );

typedef struct ASImageLayer
{
	ASImage *im;
	int dst_x, dst_y;
	/* clip area could be partially outside of the image - iumage gets tiled in it */
	int clip_x, clip_y;
	unsigned int clip_width, clip_height;
	ARGB32 tint ;                              /* if 0 - no tint */
	int merge_mode ;
	merge_scanlines_func merge_scanlines ;
}ASImageLayer;


void asimage_init (ASImage * im, Bool free_resources);
void asimage_start (ASImage * im, unsigned int width, unsigned int height, unsigned int compression);

ASScanline*prepare_scanline( unsigned int width, unsigned int shift, ASScanline *reusable_memory, Bool BGR_mode);
void       free_scanline( ASScanline *sl, Bool reusable );

size_t asimage_add_line (ASImage * im, ColorPart color, CARD32 * data, unsigned int y);

/* usefull for debugging : (returns memory usage)*/
unsigned int asimage_print_line (ASImage * im, ColorPart color,
				 unsigned int y, unsigned long verbosity);
/* this are verbosity flags : */
#define VRB_LINE_SUMMARY 	(0x01<<0)
#define VRB_LINE_CONTENT 	(0x01<<1)
#define VRB_CTRL_EXPLAIN 	(0x01<<2)
#define VRB_EVERYTHING		(VRB_LINE_SUMMARY|VRB_CTRL_EXPLAIN|VRB_LINE_CONTENT)

/* what file formats we support : */
ASImage *xpm2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *png2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *jpeg2ASImage( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *xcf2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *ppm2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *bmp2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *ico2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *gif2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *tiff2ASImage( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );

typedef enum
{
	ASIT_Xpm = 0,
	ASIT_ZCompressedXpm,
	ASIT_GZCompressedXpm,
	ASIT_Png,
	ASIT_Jpeg,
	ASIT_Xcf,
	ASIT_Ppm,
	ASIT_Pnm,
	ASIT_Bmp,
	ASIT_Ico,
	ASIT_Cur,
	ASIT_Gif,
	ASIT_Tiff,
	ASIT_Xbm,
	ASIT_Targa,
	ASIT_Pcx,
	ASIT_Unknown
}ASImageFileTypes;

typedef ASImage* (*as_image_loader_func)( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
extern as_image_loader_func as_image_file_loaders[ASIT_Unknown];

ASImage *file2ASImage( const char *file, ASFlagType what, double gamma, unsigned int compression, ... );

ASImage *ximage2asimage (struct ScreenInfo *scr, XImage * xim, unsigned int compression);
ASImage *pixmap2asimage (struct ScreenInfo *scr, Pixmap p, int x, int y,
	                     unsigned int width, unsigned int height,
		  				 unsigned long plane_mask, Bool keep_cache, unsigned int compression);

XImage* asimage2ximage  (struct ScreenInfo *scr, ASImage *im);
XImage* asimage2mask_ximage (struct ScreenInfo *scr, ASImage *im);
Pixmap  asimage2pixmap  (struct ScreenInfo *scr, ASImage *im, GC gc, Bool use_cached);
Pixmap  asimage2mask    (struct ScreenInfo *scr, ASImage *im, GC gc, Bool use_cached);

/* manipulations : */
ASImage *scale_asimage( struct ScreenInfo *scr, ASImage *src, unsigned int to_width, unsigned int to_height,
						Bool to_xim, unsigned int compression_out, int quality );
ASImage *tile_asimage ( struct ScreenInfo *scr, ASImage *src, int offset_x, int offset_y,
  					    unsigned int to_width,  unsigned int to_height, ARGB32 tint,
						Bool to_xim, unsigned int compression_out, int quality );
ASImage *merge_layers ( struct ScreenInfo *scr, ASImageLayer *layers, int count,
			  		    unsigned int dst_width, unsigned int dst_height,
			  		    Bool to_xim, unsigned int compression_out, int quality );



#endif
