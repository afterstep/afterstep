/*
 * Copyright (c) 2000,2001 Sasha Vasko <sashav@sprintmail.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "../configure.h"

/*#define LOCAL_DEBUG*/
#define HAVE_MMX

#include "../include/aftersteplib.h"
#include <X11/Intrinsic.h>

#include "../include/afterstep.h"
#include "../include/screen.h"
#include "../include/ascolor.h"
#include "../include/mytexture.h"
#include "../include/XImage_utils.h"
#include "../include/asimage.h"

/* Auxilary data structures : */
/********************************************************************/
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

void output_image_line_fine( ASImageOutput *, ASScanline *, int );
void output_image_line_fast( ASImageOutput *, ASScanline *, int );

typedef struct ASImageDecoder
{
	ScreenInfo 	   *scr;
	ASImage 	   *im ;
	ARGB32			tint;
	CARD8 		   *tint_red, *ting_green, *tint_blue, *tint_alpha ;
	int             origin,    /* x origin on source image before which we skip everything */
					out_len;   /* actuall length of the output scanline */
	ASScanline 		buffer;
	int 			next_line ;
}ASImageDecoder;


/**********************************************************************
 * quality control: we support several levels of quality to allow for
 * smooth work on older computers.
 **********************************************************************/
#define ASIMAGE_QUALITY_POOR	0
#define ASIMAGE_QUALITY_FAST	1
#define ASIMAGE_QUALITY_GOOD	2
#define ASIMAGE_QUALITY_TOP		3
static int asimage_quality_level = ASIMAGE_QUALITY_GOOD;
static void (*output_image_line)( ASImageOutput *, ASScanline *, int ) = output_image_line_fine;
#ifdef HAVE_MMX
static Bool asimage_use_mmx = True;
#else
static Bool asimage_use_mmx = False;
#endif
/**********************************************************************/
/* initialization routines 											  */
/**********************************************************************/
/*************************** MMX **************************************/
/*inline extern*/
int mmx_init(void)
{
int mmx_available;
#ifdef HAVE_MMX
	asm volatile (
                      /* Get CPU version information */
                      "pushl %%eax\n\t"
                      "pushl %%ebx\n\t"
                      "pushl %%ecx\n\t"
                      "pushl %%edx\n\t"
                      "movl $1, %%eax\n\t"
                      "cpuid\n\t"
                      "andl $0x800000, %%edx \n\t"
					  "movl %%edx, %0\n\t"
                      "popl %%edx\n\t"
                      "popl %%ecx\n\t"
                      "popl %%ebx\n\t"
                      "popl %%eax\n\t"
                      : "=m" (mmx_available)
                      : /* no input */
					  : "ebx", "ecx", "edx"
			  );
#endif
	return mmx_available;
}

int mmx_off(void)
{
#ifdef HAVE_MMX
	__asm__ __volatile__ (
                      /* exit mmx state : */
                      "emms \n\t"
                      : /* no output */
                      : /* no input  */
			  );
#endif
	return 0;
}

/**********************   ASImage  ************************************/
void
asimage_free_color (ASImage * im, CARD8 ** color)
{
	if (im != NULL && color != NULL)
	{
		register int  i;

		for (i = 0; i < im->height; i++)
			if (color[i])
				free (color[i]);
		free (color);
	}
}

void
asimage_init (ASImage * im, Bool free_resources)
{
	if (im != NULL)
	{
		if (free_resources)
		{
			asimage_free_color (im, im->red);
			asimage_free_color (im, im->green);
			asimage_free_color (im, im->blue);
			asimage_free_color (im, im->alpha);
			if (im->buffer)
				free (im->buffer);
			if( im->ximage )
				XDestroyImage( im->ximage );
		}
		memset (im, 0x00, sizeof (ASImage));
	}
}

void
asimage_start (ASImage * im, unsigned int width, unsigned int height)
{
	if (im)
	{
		asimage_init (im, True);
		im->width = width;
		im->buf_len = width + width;
		im->buffer = malloc (im->buf_len);

		im->height = height;

		im->red = safemalloc (sizeof (CARD8 *) * height);
		memset (im->red, 0x00, sizeof (CARD8 *) * height);
		im->green = safemalloc (sizeof (CARD8 *) * height);
		memset (im->green, 0x00, sizeof (CARD8 *) * height);
		im->blue = safemalloc (sizeof (CARD8 *) * height);
		memset (im->blue, 0x00, sizeof (CARD8 *) * height);
		im->alpha = safemalloc (sizeof (CARD8 *) * height);
		memset (im->alpha, 0x00, sizeof (CARD8 *) * height);
	}
}

static inline CARD8 **
asimage_get_color_ptr (ASImage * im, ColorPart color)
{
	switch (color)
	{
	 case IC_RED:
		 return im->red;
	 case IC_GREEN:
		 return im->green;
	 case IC_BLUE:
		 return im->blue;
	 case IC_ALPHA:
		 return im->alpha;
	}
	return NULL;
}

void
asimage_apply_buffer (ASImage * im, ColorPart color, unsigned int y)
{
	CARD8       **part;

	if (im == NULL)
		return;
	if (im->buffer == NULL || y >= im->height)
		return;

	if ((part = asimage_get_color_ptr (im, color)) != NULL)
	{
		if (part[y] != NULL)
			free (part[y]);
		part[y] = safemalloc (im->buf_used);
		memcpy (part[y], im->buffer, im->buf_used);
		im->buf_used = 0;
	}
}
/********************** ASScanline ************************************/
ASScanline*
prepare_scanline( unsigned int width, unsigned int shift, ASScanline *reusable_memory, Bool BGR_mode  )
{
	register ASScanline *sl = reusable_memory ;
	size_t aligned_width;
	void *ptr;

	if( sl == NULL )
		sl = safecalloc( 1, sizeof( ASScanline ) );

	sl->width 	= width ;
	sl->shift   = shift ;
	/* we want to align data by 8 byte boundary (double)
	 * to allow for code with less ifs and easier MMX/3Dnow utilization :*/
	aligned_width = width + (width&0x00000001);
	sl->buffer = ptr = safemalloc (((aligned_width*4)+4)*sizeof(CARD32));

	sl->xc1 = sl->red 	= (CARD32*)(((long)ptr>>3)*8);
	sl->xc2 = sl->green = sl->red   + aligned_width;
	sl->xc3 = sl->blue 	= sl->green + aligned_width;
	sl->alpha 	= sl->blue  + aligned_width;
	if( BGR_mode )
	{
		sl->xc1 = sl->blue ;
		sl->xc3 = sl->red ;
	}
	/* this way we can be sure that our buffers have size of multiplies of 8s
	 * and thus we can skip unneeded checks in code */
	/* initializing padding into 0 to avoid any garbadge carry-over
	 * bugs with diffusion: */
	sl->red[aligned_width-1]   = 0;
	sl->green[aligned_width-1] = 0;
	sl->blue[aligned_width-1]  = 0;
	sl->alpha[aligned_width-1] = 0;

	return sl;
}

void
free_scanline( ASScanline *sl, Bool reusable )
{
	if( sl )
	{
		if( sl->buffer )
			free( sl->buffer );
		if( !reusable )
			free( sl );
	}
}

/********************* ASImageOutput ****************************/

static ASImageOutput *
start_image_output( ScreenInfo *scr, ASImage *im, XImage *xim, Bool to_xim, int shift )
{
	register ASImageOutput *imout= NULL;

	if( im == NULL )
		return imout;
	if( xim == NULL )
		xim = im->ximage ;
	if( to_xim && xim == NULL )
		return imout;
	imout = safecalloc( 1, sizeof(ASImageOutput));
	imout->scr = scr?scr:&Scr;
	imout->im = im ;
	imout->to_xim = to_xim ;
	imout->xim = xim ;

	imout->height = xim->height;
	imout->bpl 	  = xim->bytes_per_line;
	imout->xim_line = xim->data;

	prepare_scanline( im->width, 0, &(imout->buffer[0]), scr->BGR_mode);
	prepare_scanline( im->width, 0, &(imout->buffer[1]), scr->BGR_mode);
	imout->available = &(imout->buffer[0]);
	imout->used 	 = NULL;
	imout->buffer_shift = shift;
	imout->next_line = 0 ;
	if( output_image_line == NULL )
		output_image_line = (asimage_quality_level >= ASIMAGE_QUALITY_GOOD )?
							output_image_line_fine:output_image_line_fast;
	return imout;
}

static void
stop_image_output( ASImageOutput **pimout )
{
	if( pimout )
	{
		register ASImageOutput *imout = *pimout;
		if( imout )
		{
			if( imout->used )
				output_image_line( imout, NULL, 1);
			free_scanline(&(imout->buffer[0]), True);
			free_scanline(&(imout->buffer[1]), True);
			free( imout );
			*pimout = NULL;
		}
	}
}

/***********************************************************************/
/*  Compression/decompression 										   */
/***********************************************************************/
void
asimage_add_line (ASImage * im, ColorPart color, register CARD32 * data, unsigned int y)
{
	int             i = 1, bstart = 0, ccolor = 0;
	unsigned int    width;
	register CARD8 *dst;
	register int 	tail = 0;
	int best_size, best_bstart = 0, best_tail = 0;

	if (im == NULL || data == NULL)
		return;
	if (im->buffer == NULL || y >= im->height)
		return;

	best_size = 0 ;
	width = im->width;
	dst = im->buffer;
/*	fprintf( stderr, "%d:%d:%d<%2.2X ", y, color, 0, data[0] ); */

	if( width == 1 )
	{
		dst[0] = RLE_DIRECT_TAIL ;
		dst[1] = data[0] ;
		im->buf_used = 2;
	}else
	{
		do
		{
			while( i < width && data[i] == data[ccolor])
			{
/*				fprintf( stderr, "%d<%2.2X ", i, data[i] ); */
				++i ;
			}
			if( i > ccolor + RLE_THRESHOLD )
			{ /* we have to write repetition count and length */
				register unsigned int rep_count = i - ccolor - RLE_THRESHOLD;

				if (rep_count <= RLE_MAX_SIMPLE_LEN)
				{
					dst[tail] = (CARD8) rep_count;
					dst[++tail] = (CARD8) data[ccolor];
/*					fprintf( stderr, "\n%d:%d: >%d: %2.2X %d: %2.2X ", y, color, tail-1, dst[tail-1], tail, dst[tail] ); */
				} else
				{
					dst[tail] = (CARD8) ((rep_count >> 8) & RLE_LONG_D)|RLE_LONG_B;
					dst[++tail] = (CARD8) ((rep_count) & 0x00FF);
					dst[++tail] = (CARD8) data[ccolor];
/*					fprintf( stderr, "\n%d:%d: >%d: %2.2X %d: %2.2X %d: %2.2X ", y, color, tail-2, dst[tail-2], tail-1, dst[tail-1], tail, dst[tail] ); */
				}
				++tail ;
				bstart = ccolor = i;
			}
/*			fprintf( stderr, "\n"); */
			while( i < width )
			{
/*				fprintf( stderr, "%d<%2.2X ", i, data[i] ); */
				if( data[i] != data[ccolor] )
					ccolor = i ;
				else if( i-ccolor > RLE_THRESHOLD )
					break;
				++i ;
			}
			if( i == width )
				ccolor = i ;
			while( ccolor > bstart )
			{/* we have to write direct block */
				int dist = ccolor-bstart ;

				if( tail-bstart < best_size )
				{
					best_tail = tail ;
					best_bstart = bstart ;
					best_size = tail-bstart ;
				}
				if( dist > RLE_MAX_DIRECT_LEN )
					dist = RLE_MAX_DIRECT_LEN ;
				dst[tail] = RLE_DIRECT_B | ((CARD8)(dist-1));
/*				fprintf( stderr, "\n%d:%d: >%d: %2.2X ", y, color, tail, dst[tail] ); */
				dist += bstart ;
				++tail ;
				while ( bstart < dist )
				{
					dst[tail] = (CARD8) data[bstart];
/*					fprintf( stderr, "%d: %2.2X ", tail, dst[tail]); */
					++tail ;
					++bstart;
				}
			}
/*			fprintf( stderr, "\n"); */
		}while( i < width );
		if( best_size+width < tail )
		{
			LOCAL_DEBUG_OUT( " %d:%d >resetting bytes starting with offset %d(%d) (0x%2.2X) to DIRECT_TAIL( %d bytes total )", y, color, best_tail, best_bstart, dst[best_tail], width-best_bstart );
			dst[best_tail] = RLE_DIRECT_TAIL;
			dst += best_tail+1;
			data += best_bstart;
			for( i = width-best_bstart-1 ; i >=0 ; --i )
				dst[i] = data[i] ;
			im->buf_used = best_tail+1+width-best_bstart ;
		}else
		{
			dst[tail] = RLE_EOL;
			im->buf_used = tail+1;
		}
	}
	asimage_apply_buffer (im, color, y);
}

unsigned int
asimage_print_line (ASImage * im, ColorPart color, unsigned int y, unsigned long verbosity)
{
	CARD8       **color_ptr;
	register CARD8 *ptr;
	int           to_skip = 0;
	int 		  uncopressed_size = 0 ;

	if (im == NULL)
		return 0;
	if (y >= im->height)
		return 0;

	if ((color_ptr = asimage_get_color_ptr (im, color)) == NULL)
		return 0;
	ptr = color_ptr[y];
	if( ptr == NULL )
	{
		if(  verbosity != 0 )
			show_error( "no data available for line %d", y );
		return 0;
	}
	while (*ptr != RLE_EOL)
	{
		while (to_skip-- > 0)
		{
			if (get_flags (verbosity, VRB_LINE_CONTENT))
				fprintf (stderr, " %2.2X", *ptr);
			ptr++;
		}

		if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
			fprintf (stderr, "\nControl byte encountered : %2.2X", *ptr);

		if (((*ptr) & RLE_DIRECT_B) != 0)
		{
			if( *ptr == RLE_DIRECT_TAIL )
			{
				if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
					fprintf (stderr, " is RLE_DIRECT_TAIL ( %d bytes ) !", im->width-uncopressed_size);
				ptr += im->width-uncopressed_size;
				break;
			}
			to_skip = 1 + ((*ptr) & (RLE_DIRECT_D));
			uncopressed_size += to_skip;
			if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
				fprintf (stderr, " is RLE_DIRECT !");
		} else if (((*ptr) & RLE_SIMPLE_B_INV) == 0)
		{
			if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
				fprintf (stderr, " is RLE_SIMPLE !");
			uncopressed_size += ((int)ptr[0])+ RLE_THRESHOLD;
			to_skip = 1;
		} else if (((*ptr) & RLE_LONG_B) != 0)
		{
			if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
				fprintf (stderr, " is RLE_LONG !");
			uncopressed_size += ((int)ptr[1])+((((int)ptr[0])&RLE_LONG_D ) << 8)+RLE_THRESHOLD;
			to_skip = 1 + 1;
		}
		to_skip++;
		if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
			fprintf (stderr, " to_skip = %d, uncompressed size = %d\n", to_skip, uncopressed_size);
	}
	if (get_flags (verbosity, VRB_LINE_CONTENT))
		fprintf (stderr, " %2.2X\n", *ptr);

	ptr++;

	if (get_flags (verbosity, VRB_LINE_SUMMARY))
		fprintf (stderr, "Row %d, Component %d, Memory Used %d\n", y, color, ptr - color_ptr[y]);
	return ptr - color_ptr[y];
}

inline int
asimage_decode_line (ASImage * im, ColorPart color, CARD32 * to_buf, unsigned int y)
{
	CARD8       **color_ptr = asimage_get_color_ptr (im, color);
	register CARD8  *src = color_ptr[y];
	register CARD32 *dst = to_buf;
	/* that thing below is supposedly highly optimized : */
	if( src == NULL )
	{                                          /* zero filling output : */
		register int i;
		for( i = 0 ; i < im->width ; i++ )
			to_buf[i] = 0 ;
		return im->width;
	}
	while ( *src != RLE_EOL)
	{
		if( src[0] == RLE_DIRECT_TAIL )
		{
			register int i = im->width - (dst-to_buf) ;
			dst += i ;
			src += i+1 ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[i] ;
				++i ;
			}
			break;
		}else if( ((*src)&RLE_DIRECT_B) != 0 )
		{
			register int i = (((int)src[0])&RLE_DIRECT_D) + 1;
			dst += i ;
			src += i+1 ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[i] ;
				++i ;
			}
		}else if( ((*src)&RLE_LONG_B) != 0 )
		{
			register int i = ((((int)src[0])&RLE_LONG_D)<<8|src[1]) + RLE_THRESHOLD;
			dst += i ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[2] ;
				++i ;
			}
			src += 3;
		}else
		{
			register int i = (int)src[0] + RLE_THRESHOLD ;
			dst += i ;
			i = -i;
			while( i <= 0 )
			{
				dst[i] = src[1] ;
				++i ;
			}
			src += 2;
		}
	}
	return (dst - to_buf);
}

unsigned int
asimage_copy_line (CARD8 * from, CARD8 * to, int width)
{
	register CARD8 *src = from, *dst = to;
	int uncompressed_size = 0;

	/* merely copying the data */
	if (src == NULL || dst == NULL)
		return 0;
	while (*src != RLE_EOL)
	{
		if ( *src == RLE_DIRECT_TAIL)
		{
			register int  to_write = width - uncompressed_size;
			while (to_write-- >= 0)			   /* we start counting from 0 - 0 is actually count of 1 */
				dst[to_write] = src[to_write];
			dst += to_write ;
			src += to_write ;
			break;
		} else if (((*src) & RLE_DIRECT_B) != 0)
		{
			register int  to_write = 1+ ((*src) & (RLE_DIRECT_D)) + 1;
			uncompressed_size += to_write-1 ;
			while (to_write-- >= 0)			   /* we start counting from 0 - 0 is actually count of 1 */
				dst[to_write] = src[to_write];
			dst += to_write ;
			src += to_write ;
		} else if (((*src) & RLE_SIMPLE_B_INV) == 0)
		{
			uncompressed_size += src[0]+RLE_THRESHOLD ;
			dst[0] = src[0];
			dst[1] = src[1];
			dst += 2 ;
			src += 2 ;
		} else if (((*src) & RLE_LONG_B) != 0)
		{
			uncompressed_size += src[1] + ((src[0]&RLE_LONG_D)<<8) + RLE_THRESHOLD ;
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst += 3 ;
			src += 3 ;
		}
	}
	return (dst - to);
}

Bool
asimage_compare_line (ASImage *im, ColorPart color, CARD32 *to_buf, CARD32 *tmp, unsigned int y, Bool verbose)
{
	register int i;
	asimage_decode_line( im, color, tmp, y );
	for( i = 0 ; i < im->width ; i++ )
		if( tmp[i] != to_buf[i] )
		{
			if( verbose )
				show_error( "line %d, component %d differ at offset %d ( 0x%lX(compresed) != 0x%lX(orig) )\n", y, color, i, tmp[i], to_buf[i] );
			return False ;
		}
	return True;
}

/*******************************************************************************/
/* below goes all kinds of funky stuff we can do with scanlines : 			   */
/*******************************************************************************/
/* this will enlarge array based on count of items in dst per PAIR of src item with smoothing/scatter/dither */
/* the following formulas use linear approximation to calculate   */
/* color values for new pixels : 				  				  */
/* note that we shift values by 8 to keep quanitzation error in   */
/* lower 1 byte for subsequent dithering 	:					  */
#define QUANT_ERR_BITS  	8
#define QUANT_ERR_MASK  	0x000000FF
/* for scale factor of 2 we use this formula :    */
/* C = (-C1+3*C2+3*C3-C4)/4 					  */
/* or better :				 					  */
/* C = (-C1+5*C2+5*C3-C4)/8 					  */
#define INTERPOLATE_COLOR1(c) 			   	((c)<<QUANT_ERR_BITS)  /* nothing really to interpolate here */
#define INTERPOLATE_COLOR2(c1,c2,c3,c4)    	((((c2)<<2)+(c2)+((c3)<<2)+(c3)-(c1)-(c4))<<(QUANT_ERR_BITS-3))
#define INTERPOLATE_COLOR2_V(c1,c2,c3,c4)    	((((c2)<<2)+(c2)+((c3)<<2)+(c3)-(c1)-(c4))>>3)
/* for scale factor of 3 we use these formulas :  */
/* Ca = (-2C1+8*C2+5*C3-2C4)/9 		  			  */
/* Cb = (-2C1+5*C2+8*C3-2C4)/9 		  			  */
/* or better : 									  */
/* Ca = (-C1+5*C2+3*C3-C4)/6 		  			  */
/* Cb = (-C1+3*C2+5*C3-C4)/6 		  			  */
#define INTERPOLATE_A_COLOR3(c1,c2,c3,c4)  	(((((c2)<<2)+(c2)+((c3)<<1)+(c3)-(c1)-(c4))<<QUANT_ERR_BITS)/6)
#define INTERPOLATE_B_COLOR3(c1,c2,c3,c4)  	(((((c2)<<1)+(c2)+((c3)<<2)+(c3)-(c1)-(c4))<<QUANT_ERR_BITS)/6)
#define INTERPOLATE_A_COLOR3_V(c1,c2,c3,c4)  	((((c2)<<2)+(c2)+((c3)<<1)+(c3)-(c1)-(c4))/6)
#define INTERPOLATE_B_COLOR3_V(c1,c2,c3,c4)  	((((c2)<<1)+(c2)+((c3)<<2)+(c3)-(c1)-(c4))/6)
/* just a hypotesus, but it looks good for scale factors S > 3: */
/* Cn = (-C1+(2*(S-n)+1)*C2+(2*n+1)*C3-C4)/2S  	  			   */
/* or :
 * Cn = (-C1+(2*S+1)*C2+C3-C4+n*(2*C3-2*C2)/2S  			   */
/*       [ T                   [C2s]  [C3s]]   			       */
#define INTERPOLATION_Cs(c)	 		 	    ((c)<<1)
/*#define INTERPOLATION_TOTAL_START(c1,c2,c3,c4,S) 	(((S)<<1)*(c2)+((c3)<<1)+(c3)-c2-c1-c4)*/
#define INTERPOLATION_TOTAL_START(c1,c2,c3,c4,S) 	((((S)<<1)+1)*(c2)+(c3)-(c1)-(c4))
#define INTERPOLATION_TOTAL_STEP(c2,c3)  	((c3<<1)-(c2<<1))
#define INTERPOLATE_N_COLOR(T,S)		  	(((T)<<(QUANT_ERR_BITS-1))/(S))

#define AVERAGE_COLOR1(c) 					((c)<<QUANT_ERR_BITS)
#define AVERAGE_COLOR2(c1,c2)				(((c1)+(c2))<<(QUANT_ERR_BITS-1))
#define AVERAGE_COLORN(T,N)					(((T)<<QUANT_ERR_BITS)/N)

static inline void
enlarge_component12( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* expected len >= 2  */
	register int i = 0, k = 0;
	register int c1 = src[0], c4;
LOCAL_DEBUG_OUT( "scaling from %d", len );
	--len; --len ;
	while( i < len )
	{
		c4 = src[i+2];
		/* that's right we can do that PRIOR as we calculate nothing */
		dst[k] = INTERPOLATE_COLOR1(src[i]) ;
		if( scales[i] == 2 )
		{
			register int c2 = src[i], c3 = src[i+1] ;
			c3 = INTERPOLATE_COLOR2(c1,c2,c3,c4);
			dst[++k] = (c3&0xFF000000 )?0:c3;
		}
		c1 = src[i];
		++k;
		++i;
	}

	/* to avoid one more if() in loop we moved tail part out of the loop : */
	if( scales[i] == 1 )
		dst[k] = INTERPOLATE_COLOR1(src[i]);
	else
	{
		register int c2 = src[i], c3 = src[i+1] ;
		c2 = INTERPOLATE_COLOR2(c1,c2,c3,c3);
		dst[k] = (c2&0xFF000000 )?0:c2;
	}
	dst[k+1] = INTERPOLATE_COLOR1(src[i+1]);
}

static inline void
enlarge_component23( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* expected len >= 2  */
	register int i = 0, k = 0;
	register int c1 = src[0], c4 = src[1];
LOCAL_DEBUG_OUT( "scaling from %d", len );
	if( scales[0] == 1 )
	{/* special processing for first element - it can be 1 - others can only be 2 or 3 */
		dst[k] = INTERPOLATE_COLOR1(src[0]) ;
		++k;
		++i;
	}
	--len; --len;
	while( i < len )
	{
		register int c2 = src[i], c3 = src[i+1] ;
		c4 = src[i+2];
		dst[k] = INTERPOLATE_COLOR1(c2) ;
		if( scales[i] == 2 )
		{
			c3 = INTERPOLATE_COLOR2(c1,c2,c3,c3);
			dst[++k] = (c3&0xFF000000 )?0:c3;
		}else
		{
			dst[++k] = INTERPOLATE_A_COLOR3(c1,c2,c3,c4);
			if( dst[k]&0xFF000000 )
				dst[k] = 0 ;
			c3 = INTERPOLATE_B_COLOR3(c1,c2,c3,c3);
			dst[++k] = (c3&0xFF000000 )?0:c3;
		}
		c1 = c2 ;
		++k;
		++i;
	}
	/* to avoid one more if() in loop we moved tail part out of the loop : */
	{
		register int c2 = src[i], c3 = src[i+1] ;
		dst[k] = INTERPOLATE_COLOR1(c2) ;
		if( scales[i] == 2 )
		{
			c2 = INTERPOLATE_COLOR2(c1,c2,c3,c3);
			dst[k+1] = (c2&0xFF000000 )?0:c2;
		}else
		{
			if( scales[i] == 1 )
				--k;
			else
			{
				dst[++k] = INTERPOLATE_A_COLOR3(c1,c2,c3,c4);
				if( dst[k]&0xFF000000 )
					dst[k] = 0 ;
				c2 = INTERPOLATE_B_COLOR3(c1,c2,c3,c3);
				dst[k+1] = (c2&0xFF000000 )?0:c2;
			}
		}
	}
	dst[k+2] = INTERPOLATE_COLOR1(c4) ;
}

/* this case is more complex since we cannot really hardcode coefficients
 * visible artifacts on smooth gradient-like images
 */
static inline void
enlarge_component( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* we skip all checks as it is static function and we want to optimize it
  * as much as possible */
	int i = 0;
	int c1 = src[0];
LOCAL_DEBUG_OUT( "len = %d", len );
	--len ;
	do
	{
		register int step = INTERPOLATION_TOTAL_STEP(src[i],src[i+1]);
		register int T ;
		register short S = scales[i];

/*		LOCAL_DEBUG_OUT( "pixel %d, S = %d, step = %d", i, S, step );*/
		T = INTERPOLATION_TOTAL_START(c1,src[i],src[i+1],src[i+2],S);
		if( step )
		{
			register int n = 0 ;
			do
			{
				dst[n] = (T&0x7F000000)?0:INTERPOLATE_N_COLOR(T,S);
				if( ++n >= S ) break;
				(int)T += (int)step;
			}while(1);
			dst += n ;
		}else
		{
			register CARD32 c = (T&0x7F000000)?0:INTERPOLATE_N_COLOR(T,S);
			while(--S >= 0){	dst[S] = c;	}
			dst += scales[i] ;
		}
		c1 = src[i];
		if( ++i >= len )
			break;
	}while(1);
	*dst = INTERPOLATE_COLOR1(src[i]) ;
/*LOCAL_DEBUG_OUT( "%d pixels written", k );*/
}

/* this will shrink array based on count of items in src per one dst item with averaging */
static inline void
shrink_component( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{/* we skip all checks as it is static function and we want to optimize it
  * as much as possible */
	register int i = -1, k = -1;
LOCAL_DEBUG_OUT( "len = %d", len );
	while( ++k < len )
	{
		register int reps = scales[k] ;
		register int c1 = src[++i];
/*LOCAL_DEBUG_OUT( "pixel = %d, scale[k] = %d", k, reps );*/
		if( reps == 1 )
			dst[k] = AVERAGE_COLOR1(c1);
		else if( reps == 2 )
		{
			++i;
			dst[k] = AVERAGE_COLOR2(c1,src[i]);
		}else
		{
			reps += i-1;
			while( reps > i )
			{
				++i ;
				c1 += src[i];
			}
			{
				register short S = scales[k];
				dst[k] = AVERAGE_COLORN(c1,S);
			}
		}
	}
}
static inline void
shrink_component11( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		dst[i] = AVERAGE_COLOR1(src[i]);
}


/* for consistency sake : */
static inline void
copy_component( register CARD32 *src, register CARD32 *dst, int *scales, int len )
{
#if 1
#ifdef CARD64
	CARD64 *dsrc = (CARD64*)src;
	CARD64 *ddst = (CARD64*)dst;
#else
	double *dsrc = (double*)src;
	double *ddst = (double*)dst;
#endif
	register int i = 0;

	len += len&0x01;
	len = len>>1 ;
	do
	{
		ddst[i] = dsrc[i];
	}while(++i < len );
#else
	register int i = 0;

	len += len&0x01;
	do
	{
		dst[i] = src[i];
	}while(++i < len );

#endif

}

static inline void
add_component( CARD32 *src, CARD32 *incr, int *scales, int len )
{
	int i = 0;

	len += len&0x01;
#if 1
#ifdef HAVE_MMX
	if( asimage_use_mmx )
	{
		double *ddst = (double*)&(src[0]);
		double *dinc = (double*)&(incr[0]);
		len = len>>1;
		do{
			asm volatile
       		(
            	"movq %0, %%mm0  \n\t" /* load 8 bytes from src[i] into MM0 */
            	"paddd %1, %%mm0 \n\t" /* MM0=src[i]>>1              */
            	"movq %%mm0, %0  \n\t" /* store the result in dest */
				: "=m" (ddst[i])       /* %0 */
				:  "m"  (dinc[i])       /* %2 */
	        );
		}while( ++i < len );
	}else
#endif
#endif
	{
		register int c1, c2;
		do{
			c1 = (int)src[i] + (int)incr[i] ;
			c2 = (int)src[i+1] + (int)incr[i+1] ;
			src[i] = c1;
			src[i+1] = c2;
			i += 2 ;
		}while( i < len );
	}
}

static inline int
set_component( register CARD32 *src, register CARD32 value, int offset, int len )
{
	register int i ;
	for( i = offset ; i < len ; ++i )
		src[i] = value;
	return len-offset;
}

static inline void
divide_component( register CARD32 *src, register CARD32 *dst, CARD16 ratio, int len )
{
	register int i = 0;
	len += len&0x00000001;                     /* we are 8byte aligned/padded anyways */
	if( ratio == 2 )
	{
#ifdef HAVE_MMX
		if( asimage_use_mmx )
		{
			double *ddst = (double*)&(dst[0]);
			double *dsrc = (double*)&(src[0]);
			len = len>>1;
			do{
				asm volatile
       		    (
            		"movq %1, %%mm0  \n\t" // load 8 bytes from src[i] into MM0
            		"psrld $1, %%mm0 \n\t" // MM0=src[i]>>1
            		"movq %%mm0, %0  \n\t" // store the result in dest
					: "=m" (ddst[i]) // %0
					: "m"  (dsrc[i]) // %1
	            );
			}while( ++i < len );
		}else
#endif
			do{
				dst[i] = src[i] >> 1;
				dst[i+1] = src[i+1]>> 1;
				i += 2 ;
			}while( i < len );
	}else
	{
		do{
			register int c1 = src[i];
			register int c2 = src[i+1];
			dst[i] = c1/ratio;
			dst[i+1] = c2/ratio;
			i+=2;
		}while( i < len );
	}
}

static inline void
rbitshift_component( register CARD32 *src, register CARD32 *dst, int shift, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		dst[i] = src[i]>>shift;
}

/* diffusingly combine src onto self and dst, and rightbitshift src by quantization shift */
static inline void
fine_output_filter( register CARD32 *line1, register CARD32 *line2, int unused, int len )
{/* we carry half of the quantization error onto the surrounding pixels : */
 /*        X    7/16 */
 /* 3/16  5/16  1/16 */
	register int i ;
	register CARD32 errp = 0, err = 0, c;
	c = ((line1[0]&0xFF000000)!=0)?0:line1[0] ;
	errp = c&QUANT_ERR_MASK;
	line1[0] = c>>QUANT_ERR_BITS ;
	line2[0] += (errp*5)>>4 ;

	for( i = 1 ; i < len ; ++i )
	{
		c = (((line1[i]&0xFF000000)!=0)?0:line1[i])+((errp*7)>>4) ;
		err = c&QUANT_ERR_MASK;
		line1[i] = c>>QUANT_ERR_BITS ;
		line2[i-1] += (err*3)>>4 ;
		line2[i] += ((err*5)>>4)+(errp>>4);
		errp = err ;
	}
}

static inline void
fast_output_filter( register CARD32 *src, register CARD32 *dst, short ratio, int len )
{/* we carry half of the quantization error onto the following pixel and store it in dst: */
	register int i = 0;
	if( ratio <= 1 )
	{
		register int c = src[0];
  	    do
		{
			if( (c&0xFFFF0000)!= 0 )
				c = ( c&0x7F000000 )?0:0x0000FFFF;
			dst[i] = c>>(QUANT_ERR_BITS) ;
			if( ++i >= len )
				break;
			c = ((c&QUANT_ERR_MASK)>>1)+src[i];
		}while(1);
	}else if( ratio == 2 )
	{
		register CARD32 c = src[0];
  	    do
		{
			c = c>>1 ;
			if( (c&0xFFFF0000) != 0 )
				c = ( c&0x7F000000 )?0:0x0000FFFF;
			dst[i] = c>>(QUANT_ERR_BITS) ;
			if( ++i >= len )
				break;
			c = ((c&QUANT_ERR_MASK)>>1)+src[i];
		}while( 1 );
	}else
	{
		register CARD32 c = src[0];
  	    do
		{
			c = c/ratio ;
			if( c&0xFFFF0000 )
				c = ( c&0x7F000000 )?0:0x0000FFFF;
			dst[i] = c>>(QUANT_ERR_BITS) ;
			if( ++i >= len )
				break;
			c = ((c&QUANT_ERR_MASK)>>1)+src[i];
		}while(1);
	}
}

static inline void
start_component_interpolation( CARD32 *c1, CARD32 *c2, CARD32 *c3, CARD32 *c4, register CARD32 *T, register CARD32 *step, CARD16 S, int len)
{
	register int i;
	for( i = 0 ; i < len ; i++ )
	{
		register int rc2 = c2[i], rc3 = c3[i] ;
		T[i] = INTERPOLATION_TOTAL_START(c1[i],rc2,rc3,c4[i],S)/(S<<1);
		step[i] = INTERPOLATION_TOTAL_STEP(rc2,rc3)/(S<<1);
	}
}

static inline void
component_interpolation_hardcoded( CARD32 *c1, CARD32 *c2, CARD32 *c3, CARD32 *c4, register CARD32 *T, CARD32 *unused, CARD16 kind, int len)
{
	register int i;
	if( kind == 1 )
	{
		for( i = 0 ; i < len ; i++ )
		{
#if 1
			/* its seems that this simple formula is completely sufficient
			   and even better then more complicated one : */
			T[i] = (c2[i]+c3[i])>>1 ;
#else
    		register int minus = c1[i]+c4[i] ;
			register int plus  = (c2[i]<<1)+c2[i]+(c3[i]<<1)+c3[i];

			T[i] = ( (plus>>1) < minus )?(c2[i]+c3[i])>>1 :
								   		 (plus-minus)>>2;
#endif
		}
	}else if( kind == 2 )
	{
		for( i = 0 ; i < len ; i++ )
		{
    		register int rc1 = c1[i], rc2 = c2[i], rc3 = c3[i] ;
			T[i] = INTERPOLATE_A_COLOR3_V(rc1,rc2,rc3,c4[i]);
		}
	}else
		for( i = 0 ; i < len ; i++ )
		{
    		register int rc1 = c1[i], rc2 = c2[i], rc3 = c3[i] ;
			T[i] = INTERPOLATE_B_COLOR3_V(rc1,rc2,rc3,c4[i]);
		}
}

static inline void
divide_component_mod( register CARD32 *data, CARD16 ratio, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		data[i] /= ratio;
}

static inline void
rbitshift_component_mod( register CARD32 *data, int bits, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		data[i] = data[i]>>bits;
}

static inline void
fast_output_filter_mod( register CARD32 *data, int unused, int len )
{/* we carry half of the quantization error onto the following pixel : */
	register int i ;
	register CARD32 err = 0, c;
	for( i = 0 ; i < len ; ++i )
	{
		c = (((data[i]&0xFF000000)!=0)?0:data[i])+err;
		err = (c&QUANT_ERR_MASK)>>1 ;
		data[i] = c>>QUANT_ERR_BITS ;
	}
}

static void
print_component( register CARD32 *data, int nonsense, int len )
{
	register int i ;
	for( i = 0 ; i < len ; ++i )
		fprintf( stderr, " %8.8lX", data[i] );
	fprintf( stderr, "\n");
}

/* the following 5 macros will in fact unfold into huge but fast piece of code : */
/* we make poor compiler work overtime unfolding all this macroses but I bet it  */
/* is still better then C++ templates :)									     */

#define DECODE_SCANLINE(im,dst,y) \
do{												\
	clear_flags( (dst).flags,SCL_DO_ALL);			\
	if( set_component((dst).red  ,0,asimage_decode_line((im),IC_RED  ,(dst).red,  (y)),(dst).width)<(dst).width) \
		set_flags( (dst).flags,SCL_DO_RED);												 \
	if( set_component((dst).green,0,asimage_decode_line((im),IC_GREEN,(dst).green,(y)),(dst).width)<(dst).width) \
		set_flags( (dst).flags,SCL_DO_GREEN);												 \
	if( set_component((dst).blue ,0,asimage_decode_line((im),IC_BLUE ,(dst).blue, (y)),(dst).width)<(dst).width) \
		set_flags( (dst).flags,SCL_DO_BLUE);												 \
	if( set_component((dst).alpha,0,asimage_decode_line((im),IC_ALPHA,(dst).alpha,(y)),(dst).width)<(dst).width) \
		set_flags( (dst).flags,SCL_DO_ALPHA);												 \
  }while(0)

#define ENCODE_SCANLINE(im,src,y) \
do{	asimage_add_line((im), IC_RED,   (src).red,   (y)); \
   	asimage_add_line((im), IC_GREEN, (src).green, (y)); \
   	asimage_add_line((im), IC_BLUE,  (src).blue,  (y)); \
	if( get_flags((src).flags,SCL_DO_ALPHA))asimage_add_line((im), IC_ALPHA, (src).alpha, (y)); \
  }while(0)

#define SCANLINE_FUNC_slow(f,src,dst,scales,len) \
do{	f((src).red,  (dst).red,  (scales),(len));		\
	f((src).green,(dst).green,(scales),(len)); 		\
	f((src).blue, (dst).blue, (scales),(len));   	\
	if(get_flags((src).flags,SCL_DO_ALPHA)) f((src).alpha,(dst).alpha,(scales),(len)); \
  }while(0)

#define SCANLINE_FUNC(f,src,dst,scales,len) \
do{	f((src).red,  (dst).red,  (scales),(len+(len&0x01))*3);		\
	if(get_flags((src).flags,SCL_DO_ALPHA)) f((src).alpha,(dst).alpha,(scales),(len)); \
  }while(0)

#define CHOOSE_SCANLINE_FUNC(r,src,dst,scales,len) \
 switch(r)                                              							\
 {  case 0:	SCANLINE_FUNC_slow(shrink_component11,(src),(dst),(scales),(len));break;   	\
	case 1: SCANLINE_FUNC_slow(shrink_component, (src),(dst),(scales),(len));	break;  \
	case 2:	SCANLINE_FUNC_slow(enlarge_component12,(src),(dst),(scales),(len));break ; 	\
	case 3:	SCANLINE_FUNC_slow(enlarge_component23,(src),(dst),(scales),(len));break;  	\
	default:SCANLINE_FUNC_slow(enlarge_component,  (src),(dst),(scales),(len));        	\
 }

#define SCANLINE_MOD(f,src,p,len) \
do{	f((src).red,(p),(len));		\
	f((src).green,(p),(len));		\
	f((src).blue,(p),(len));		\
	if(get_flags((src).flags,SCL_DO_ALPHA)) f((src).alpha,(p),(len));\
  }while(0)

#define SCANLINE_COMBINE_slow(f,c1,c2,c3,c4,o1,o2,p,len)						   \
do{	f((c1).red,(c2).red,(c3).red,(c4).red,(o1).red,(o2).red,(p),(len));		\
	f((c1).green,(c2).green,(c3).green,(c4).green,(o1).green,(o2).green,(p),(len));	\
	f((c1).blue,(c2).blue,(c3).blue,(c4).blue,(o1).blue,(o2).blue,(p),(len));		\
	if(get_flags((c1).flags,SCL_DO_ALPHA)) f((c1).alpha,(c2).alpha,(c3).alpha,(c4).alpha,(o1).alpha,(o2).alpha,(p),(len));	\
  }while(0)

#define SCANLINE_COMBINE(f,c1,c2,c3,c4,o1,o2,p,len)						   \
do{	f((c1).red,(c2).red,(c3).red,(c4).red,(o1).red,(o2).red,(p),(len+(len&0x01))*3);		\
	if(get_flags((c1).flags,SCL_DO_ALPHA)) f((c1).alpha,(c2).alpha,(c3).alpha,(c4).alpha,(o1).alpha,(o2).alpha,(p),(len));	\
  }while(0)

void
output_image_line_fine( ASImageOutput *imout, ASScanline *new_line, int ratio )
{
	ASScanline *to_store = NULL ;
	/* caching and preprocessing line into our buffer : */
	if( imout->buffer_shift == 0 )
		to_store = new_line ;
	else if( new_line )
	{
		if( asimage_quality_level == ASIMAGE_QUALITY_TOP )
		{
			if( ratio > 1 )
				SCANLINE_FUNC(divide_component,*(new_line),*(imout->available),ratio,new_line->width);
			else
				SCANLINE_FUNC(copy_component,*(new_line),*(imout->available),NULL,new_line->width);

		}else
			SCANLINE_FUNC(fast_output_filter, *(new_line),*(imout->available),ratio,new_line->width);
	}
	/* copying/encoding previously cahced line into destination image : */
	if( imout->used != NULL )
	{
		if( asimage_quality_level == ASIMAGE_QUALITY_TOP )
		{
			if( new_line != NULL )
				SCANLINE_FUNC(fine_output_filter,*(imout->used),*(imout->available),0,new_line->width);
			else
				SCANLINE_MOD(fast_output_filter_mod,*(imout->used),0,imout->used->width);
		}
#if 0
		LOCAL_DEBUG_OUT( "output line %d :", imout->next_line );
		SCANLINE_MOD(print_component,*(imout->used),1,imout->used->width);
#endif
		to_store = imout->used ;
	}
	if( to_store )
	{
		if( imout->to_xim )
		{
			if( imout->next_line < imout->xim->height )
			{
				PUT_SCANLINE(imout->scr,imout->xim,to_store,imout->next_line,imout->xim_line);
				imout->xim_line += imout->bpl;
			}
		}else if( imout->next_line < imout->im->height )
			ENCODE_SCANLINE(imout->im,*to_store,imout->next_line);
		++(imout->next_line);
	}
	/* rotating the buffers : */
	if( imout->buffer_shift > 0 )
	{
		if( new_line == NULL )
			imout->used = NULL ;
		else
			imout->used = imout->available ;
		imout->available = &(imout->buffer[0]);
		if( imout->available == imout->used )
			imout->available = &(imout->buffer[1]);
	}
}


void
output_image_line_fast( ASImageOutput *imout, ASScanline *new_line, int ratio )
{
	/* caching and preprocessing line into our buffer : */
	if( new_line )
	{
		if( imout->buffer_shift > 0 )
		{
			imout->used = &(imout->buffer[0]);
			SCANLINE_FUNC(fast_output_filter,*(new_line),*(imout->used),ratio,new_line->width);
		}else
			imout->used = new_line ;
	}
	/* copying/encoding previously cahced line into destination image : */
	if( imout->used != NULL )
	{
#ifdef LOCAL_DEBUG
		LOCAL_DEBUG_OUT( "output line %d :", imout->next_line );
		SCANLINE_MOD(print_component,*(imout->used),1,imout->used->width);
#endif
		if( imout->to_xim )
		{
			if( imout->next_line < imout->xim->height )
			{
				PUT_SCANLINE(imout->scr,imout->xim,imout->used,imout->next_line,imout->xim_line);
				imout->xim_line += imout->bpl;
			}
		}else if( imout->next_line < imout->im->height )
			ENCODE_SCANLINE(imout->im,*(imout->used),imout->next_line);
		++(imout->next_line);
	}
	/* rotating the buffers : */
	imout->used = NULL ;
}

Bool
check_scale_parameters( ASImage *src, int *to_width, int *to_height )
{
	if( src == NULL )
		return False;

	if( *to_width < 0 )
		*to_width = src->width ;
	else if( *to_width < 2 )
		*to_width = 2 ;
	if( *to_height < 0 )
		*to_height = src->height ;
	else if( *to_height < 2 )
		*to_height = 2 ;
	return True;
}

int *
make_scales( unsigned short from_size, unsigned short to_size )
{
	int *scales ;
	unsigned short smaller = MIN(from_size,to_size);
	unsigned short bigger  = MAX(from_size,to_size);
	register int i = 0, k = 0;
	int eps;

	if( from_size < to_size )
	{	smaller--; bigger-- ; }
	if( smaller == 0 )
		smaller = 1;
	if( bigger == 0 )
		bigger = 1;
	scales = safecalloc( smaller, sizeof(int));
	eps = -(bigger>>1);
	/* now using Bresengham algoritm to fiill the scales :
	 * since scaling is merely transformation
	 * from 0:bigger space (x) to 0:smaller space(y)*/
	for ( i = 0 ; i < bigger ; i++ )
	{
		++scales[k];
		eps += smaller;
		if( (eps << 1) >= bigger )
		{
			++k ;
			eps -= bigger ;
		}
	}
	return scales;
}

/********************************************************************/
void
scale_image_down( ASImage *src, ASImageOutput *imout, int h_ratio, int *scales_h, int* scales_v)
{
	ASScanline src_line, dst_line, total ;
	int i = 0, k = 0, max_k = imout->im->height, line_len = MIN(imout->im->width,src->width);

	prepare_scanline( src->width, 0, &src_line, imout->scr->BGR_mode );
	prepare_scanline( imout->im->width, QUANT_ERR_BITS, &dst_line, imout->scr->BGR_mode );
	prepare_scanline( imout->im->width, QUANT_ERR_BITS, &total, imout->scr->BGR_mode );
	while( k < max_k )
	{
		int reps = scales_v[k] ;
		DECODE_SCANLINE(src,src_line,i);
		total.flags = src_line.flags ;
		CHOOSE_SCANLINE_FUNC(h_ratio,src_line,total,scales_h,line_len);
		reps += i;

#ifdef LOCAL_DEBUG
		LOCAL_DEBUG_OUT( "source line %d max line in this reps %d", i, reps );
		SCANLINE_MOD(print_component,total,1,total.width);
#endif
		++i ;
		while ( reps > i )
		{
			DECODE_SCANLINE(src,src_line,i);
			total.flags |= src_line.flags ;
			CHOOSE_SCANLINE_FUNC(h_ratio,src_line,dst_line,scales_h,line_len);
#ifdef LOCAL_DEBUG
			LOCAL_DEBUG_OUT( "source line %d max line in this reps %d", i, reps );
			SCANLINE_MOD(print_component,dst_line,1,total.width);
#endif
			SCANLINE_FUNC(add_component,total,dst_line,NULL,total.width);
			++i ;
		}
		output_image_line( imout, &total, scales_v[k] );
		++k;
	}
	free_scanline(&src_line, True);
	free_scanline(&dst_line, True);
	free_scanline(&total, True);
}

void
scale_image_up( ASImage *src, ASImageOutput *imout, int h_ratio, int *scales_h, int* scales_v)
{
	ASScanline step, src_lines[4], *c1, *c2, *c3, *c4 = NULL, tmp;
	int i = 0, max_i, line_len = MIN(imout->im->width,src->width), out_width = imout->im->width;

	for( i = 0 ; i < 4 ; i++ )
		prepare_scanline( out_width, 0, &(src_lines[i]), imout->scr->BGR_mode);
	prepare_scanline( src->width, QUANT_ERR_BITS, &tmp, imout->scr->BGR_mode );
	prepare_scanline( out_width, QUANT_ERR_BITS, &step, imout->scr->BGR_mode );


/*	set_component(src_lines[0].red,0x00000000,0,out_width*3); */

	DECODE_SCANLINE(src,tmp,0);
	step.flags = src_lines[0].flags = tmp.flags ;
	CHOOSE_SCANLINE_FUNC(h_ratio,tmp,src_lines[1],scales_h,line_len);

	SCANLINE_FUNC(copy_component,src_lines[1],src_lines[0],0,out_width);

	DECODE_SCANLINE(src,tmp,1);
	src_lines[1].flags = tmp.flags ;
	CHOOSE_SCANLINE_FUNC(h_ratio,tmp,src_lines[2],scales_h,line_len);

	i = 0 ;
	max_i = src->height-1 ;
	do
	{
		int S = scales_v[i] ;

		c1 = &(src_lines[i&0x03]);
		c2 = &(src_lines[(i+1)&0x03]);
		c3 = &(src_lines[(i+2)&0x03]);
		c4 = &(src_lines[(i+3)&0x03]);

		if( i+2 < src->height )
		{
			DECODE_SCANLINE(src,tmp,i+2);
			c4->flags = tmp.flags ;
			CHOOSE_SCANLINE_FUNC(h_ratio,tmp,*c4,scales_h,line_len);
		}
		/* now we'll prepare total and step : */
		output_image_line( imout, c2, 1);
		if( S > 1 )
		{
			if( S == 2 )
			{
				SCANLINE_COMBINE(component_interpolation_hardcoded,*c1,*c2,*c3,*c4,*c1,*c1,1,out_width);
				output_image_line( imout, c1, 1);
			}else if( S == 3 )
			{
				SCANLINE_COMBINE(component_interpolation_hardcoded,*c1,*c2,*c3,*c4,*c1,*c1,2,out_width);
				output_image_line( imout, c1, 1);
				SCANLINE_COMBINE(component_interpolation_hardcoded,*c1,*c2,*c3,*c4,*c1,*c1,3,out_width);
				output_image_line( imout, c1, 1);
			}else
			{
				SCANLINE_COMBINE(start_component_interpolation,*c1,*c2,*c3,*c4,*c1,step,S,out_width);
				do
				{
					output_image_line( imout, c1, 1);
					if(!(--S))
						break;
					SCANLINE_FUNC(add_component,*c1,step,NULL,out_width );
 				}while(1);
			}
		}
	}while( ++i < max_i );
	output_image_line( imout, c4, 1);

	for( i = 0 ; i < 4 ; i++ )
		free_scanline(&(src_lines[i]), True);
	free_scanline(&tmp, True);
	free_scanline(&step, True);
}

ASImage *
scale_asimage( ScreenInfo *scr, ASImage *src, int to_width, int to_height, Bool to_xim )
{
	ASImage *dst = NULL ;
	ASImageOutput *imout ;
	int h_ratio ;
	int *scales_h = NULL, *scales_v = NULL;

	if( !check_scale_parameters(src,&to_width,&to_height) )
		return NULL;

	dst = safecalloc(1, sizeof(ASImage));
	asimage_start (dst, to_width, to_height);
	if( to_xim )
		if( (dst->ximage = create_screen_ximage( scr, to_width, to_height, 0 )) == NULL )
		{
			show_error( "Unable to create XImage for the screen %d", scr->screen );
			asimage_init(dst, True);
			free( dst );
			return NULL ;
		}
	if( to_width == src->width )
		h_ratio = 0;
	else
	{
	    h_ratio = to_width/src->width;
		if( to_width%src->width > 0 )
			h_ratio++ ;
	}

	scales_h = make_scales( src->width, to_width );
	scales_v = make_scales( src->height, to_height );

#ifdef LOCAL_DEBUG
	{
	  register int i ;
	  for( i = 0 ; i < MIN(src->width, to_width) ; i++ )
		fprintf( stderr, " %d", scales_h[i] );
	  fprintf( stderr, "\n" );
	  for( i = 0 ; i < MIN(src->height, to_height) ; i++ )
		fprintf( stderr, " %d", scales_v[i] );
	  fprintf( stderr, "\n" );
	}
#endif
#ifdef HAVE_MMX
	mmx_init();
#endif
	if((imout = start_image_output( scr, dst, dst->ximage, to_xim, QUANT_ERR_BITS )) == NULL )
	{
		asimage_init(dst, True);
		free( dst );
		dst = NULL ;
	}else
	{
		if( to_height <= src->height ) 					   /* scaling down */
			scale_image_down( src, imout, h_ratio, scales_h, scales_v );
		else
			scale_image_up( src, imout, h_ratio, scales_h, scales_v );
		stop_image_output( &imout );
	}
#ifdef HAVE_MMX
	mmx_off();
#endif
	free( scales_h );
	free( scales_v );
	return dst;
}

/****************************************************************************/
/* ASImage->XImage->pixmap->XImage->ASImage conversion						*/
/****************************************************************************/

ASImage      *
asimage_from_ximage (ScreenInfo *scr, XImage * xim)
{
	ASImage      *im = NULL;
	unsigned char *xim_line;
	int           i, height, bpl;
	ASScanline    xim_buf;
#ifdef LOCAL_DEBUG
	CARD32       *tmp ;
#endif

	if( xim == NULL)
		return im;

	height = xim->height;
	bpl 	  = xim->bytes_per_line;
	xim_line = xim->data;

	im = (ASImage *) safecalloc (1, sizeof (ASImage));
	asimage_start (im, xim->width, xim->height);
#ifdef LOCAL_DEBUG
	tmp = safemalloc( xim->width * sizeof(CARD32));
#endif
	prepare_scanline( xim->width, 0, &xim_buf, scr->BGR_mode );

	for (i = 0; i < height; i++)
	{
		GET_SCANLINE(scr,xim,&xim_buf,i,xim_line);
		asimage_add_line (im, IC_RED,   xim_buf.red, i);
		asimage_add_line (im, IC_GREEN, xim_buf.green, i);
		asimage_add_line (im, IC_BLUE,  xim_buf.blue, i);
#ifdef LOCAL_DEBUG
		if( !asimage_compare_line( im, IC_RED,  xim_buf.red, tmp, i, True ) )
			exit(0);
		if( !asimage_compare_line( im, IC_GREEN,  xim_buf.green, tmp, i, True ) )
			exit(0);
		if( !asimage_compare_line( im, IC_BLUE,  xim_buf.blue, tmp, i, True ) )
			exit(0);
#endif
		xim_line += bpl;
	}
	free_scanline(&xim_buf, True);

	return im;
}

XImage*
ximage_from_asimage (ScreenInfo *scr, ASImage *im)
{
	XImage        *xim = NULL;
	int           i;
	ASScanline    xim_buf;
	ASImageOutput *imout ;

	if (im == NULL)
		return xim;

	xim = create_screen_ximage( scr, im->width, im->height, 0 );
	if( (imout = start_image_output( scr, im, xim, True, 0 )) == NULL )
		return xim;

	prepare_scanline( xim->width, 0, &xim_buf, scr->BGR_mode );
	for (i = 0; i < im->height; i++)
	{
		asimage_decode_line (im, IC_RED,   xim_buf.red, i);
		asimage_decode_line (im, IC_GREEN, xim_buf.green, i);
		asimage_decode_line (im, IC_BLUE,  xim_buf.blue, i);
		output_image_line( imout, &xim_buf, 1 );
	}
	free_scanline(&xim_buf, True);

	return xim;
}

ASImage      *
asimage_from_pixmap (ScreenInfo *scr, Pixmap p, int x, int y, unsigned int width, unsigned int height, unsigned long plane_mask, Bool keep_cache)
{
	XImage       *xim = XGetImage (dpy, p, x, y, width, height, plane_mask, ZPixmap);
	ASImage      *im = NULL;

	if (xim)
	{
		im = asimage_from_ximage (scr, xim);
		if( keep_cache )
			im->ximage = xim ;
		else
			XDestroyImage (xim);
	}
	return im;
}

Pixmap
pixmap_from_asimage(ScreenInfo *scr, ASImage *im, Window w, GC gc, Bool use_cached)
{
	XImage       *xim ;
	Pixmap        p = None;
	XWindowAttributes attr;

	if( XGetWindowAttributes (dpy, w, &attr) )
	{
		if ( !use_cached || im->ximage == NULL )
		{
			if( (xim = ximage_from_asimage( scr, im )) == NULL )
			{
				show_error("cannot export image into XImage.");
				return None ;
			}
		}else
			xim = im->ximage ;
		if (xim != NULL )
		{
			p = create_screen_pixmap( scr, xim->width, xim->height, 0 );
			XPutImage( dpy, p, gc, xim, 0, 0, 0, 0, xim->width, xim->height );
			if( xim != im->ximage )
				XDestroyImage (xim);
		}
	}else
		show_error("cannot create pixmap for drawable 0x%X - drawable is invalid.", w);

	return p;
}




