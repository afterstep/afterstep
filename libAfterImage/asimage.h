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

  CARD8 *buffer;
  unsigned int buf_used, buf_len;

  XImage *ximage ;
}
ASImage;

typedef enum
{
  IC_RED = 0,
  IC_GREEN,
  IC_BLUE,
  IC_ALPHA
}
ColorPart;

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
}ASImageOutput;


void asimage_free_color (ASImage * im, CARD8 ** color);
void asimage_init (ASImage * im, Bool free_resources);
void asimage_start (ASImage * im, unsigned int width, unsigned int height);

ASScanline*prepare_scanline( unsigned int width, unsigned int shift, ASScanline *reusable_memory, Bool BGR_mode);
void       free_scanline( ASScanline *sl, Bool reusable );

void asimage_apply_buffer (ASImage * im, ColorPart color, unsigned int y);
void asimage_add_line (ASImage * im, ColorPart color, CARD32 * data,
		       unsigned int y);

/* usefull for debugging : (returns memory usage)*/
unsigned int asimage_print_line (ASImage * im, ColorPart color,
				 unsigned int y, unsigned long verbosity);
/* this are verbosity flags : */
#define VRB_LINE_SUMMARY 	(0x01<<0)
#define VRB_LINE_CONTENT 	(0x01<<1)
#define VRB_CTRL_EXPLAIN 	(0x01<<2)
#define VRB_EVERYTHING		(VRB_LINE_SUMMARY|VRB_CTRL_EXPLAIN|VRB_LINE_CONTENT)

/* what file formats we support : */
ASImage *xpm2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage );
ASImage *png2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage );
ASImage *jpeg2ASImage( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage );
ASImage *xcf2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage );
ASImage *ppm2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage );
ASImage *bmp2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage );
ASImage *ico2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage );
ASImage *gif2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage );
ASImage *tiff2ASImage( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage );

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

typedef ASImage* (*as_image_loader_func)( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage );
extern as_image_loader_func as_image_file_loaders[ASIT_Unknown];

ASImage *file2ASImage( const char *file, ASFlagType what, double gamma, ... );

ASImage *ximage2asimage (struct ScreenInfo *scr, XImage * xim);
ASImage *pixmap2asimage (struct ScreenInfo *scr, Pixmap p, int x, int y,
	                     unsigned int width, unsigned int height,
		  				 unsigned long plane_mask, Bool keep_cache);

XImage* asimage2ximage  (struct ScreenInfo *scr, ASImage *im);
XImage* asimage2mask_ximage (struct ScreenInfo *scr, ASImage *im);
Pixmap  asimage2pixmap  (struct ScreenInfo *scr, ASImage *im, GC gc, Bool use_cached);
Pixmap  asimage2mask    (struct ScreenInfo *scr, ASImage *im, GC gc, Bool use_cached);

/* manipulations : */
ASImage *scale_asimage( struct ScreenInfo *scr, ASImage *src, int to_width, int to_height, Bool to_xim );
ASImage *tile_asimage ( struct ScreenInfo *scr, ASImage *src, int offset_x, int offset_y,  
															  unsigned int to_width, 
															  unsigned int to_height, Bool to_xim );



#endif
