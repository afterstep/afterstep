#ifndef ASIMAGE_HEADER_FILE_INCLUDED
#define ASIMAGE_HEADER_FILE_INCLUDED

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
ctrl_byte3    := 1NNNNNNN (first bit is 1, remaining are length)
*/

/* this is value */
#define RLE_EOL			0x00
/* this are masks */
#define RLE_DIRECT_B 		0x80
#define RLE_LONG_B 		0x40
/* this one is inverted mask */
#define RLE_SIMPLE_B_INV  	0xC0

/* this are masks to filter out control bits: */
#define RLE_DIRECT_D 		0x7F
#define RLE_LONG_D 		0x3F
#define RLE_SIMPLE_D  		0x3F

#define RLE_MAX_DIRECT_LEN      128
#define RLE_MAX_SIMPLE_LEN     	63
#define RLE_MAX_LONG_LEN     	(64*256)
#define RLE_THRESHOLD		3

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

void asimage_free_color (ASImage * im, CARD8 ** color);
void asimage_init (ASImage * im, Bool free_resources);
void asimage_start (ASImage * im, unsigned int width, unsigned int height);
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

unsigned int asimage_decode_line (ASImage * im, ColorPart color, CARD32 * to_buf, unsigned int y);

ASImage *asimage_from_ximage (XImage * xim);
ASImage *asimage_from_pixmap (Pixmap p, int x, int y,
			      unsigned int width,
			      unsigned int height, unsigned long plane_mask);

XImage* ximage_from_asimage (ASImage *im, int depth);
Pixmap pixmap_from_asimage(ASImage *im, Drawable d, GC gc);



#endif
