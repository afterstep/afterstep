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

typedef struct ASScanline
{
#define SCL_DO_RED          (0x01<<0)
#define SCL_DO_GREEN        (0x01<<1)
#define SCL_DO_BLUE         (0x01<<2)
#define SCL_DO_ALPHA		(0x01<<3)
#define SCL_DO_ALL			(SCL_DO_RED|SCL_DO_GREEN|SCL_DO_BLUE|SCL_DO_ALPHA)
	ASFlagType 	   flags;
	CARD32        *buffer ;
	CARD32        *red, *green, *blue, *alpha;
	unsigned int   width, shift;
/*    CARD32 r_mask, g_mask, b_mask ; */
}ASScanline;

/********************************************************************/
/* This is static piece of data that tell us what is the status of
 * the output stream, and allows us to easily put stuff out :       */
typedef struct ASImageOutput
{
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
static ASScanline*
prepare_scanline( unsigned int width, unsigned int shift, ASScanline *reusable_memory  )
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

	sl->red 	= (CARD32*)(((long)ptr>>3)*8);
	sl->green 	= sl->red   + aligned_width;
	sl->blue 	= sl->green + aligned_width;
	sl->alpha 	= sl->blue  + aligned_width;
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

ASImageOutput *
start_image_output( ASImage *im, XImage *xim, Bool to_xim, int shift )
{
	register ASImageOutput *imout= NULL;

	if( im == NULL )
		return imout;
	if( xim == NULL )
		xim = im->ximage ;
	if( to_xim && xim == NULL )
		return imout;
	imout = safecalloc( 1, sizeof(ASImageOutput));
	imout->im = im ;
	imout->to_xim = to_xim ;
	imout->xim = xim ;

	imout->height = xim->height;
	imout->bpl 	  = xim->bytes_per_line;
	imout->xim_line = xim->data;

	prepare_scanline( im->width, 0, &(imout->buffer[0]));
	prepare_scanline( im->width, 0, &(imout->buffer[1]));
	imout->available = &(imout->buffer[0]);
	imout->used 	 = NULL;
	imout->buffer_shift = shift;
	imout->next_line = 0 ;
	if( output_image_line == NULL )
		output_image_line = (asimage_quality_level >= ASIMAGE_QUALITY_GOOD )?
							output_image_line_fine:output_image_line_fast;
	return imout;
}

void
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
		return 0;
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
/* for scale factor of 3 we use these formulas :  */
/* Ca = (-2C1+8*C2+5*C3-2C4)/9 		  			  */
/* Cb = (-2C1+5*C2+8*C3-2C4)/9 		  			  */
/* or better : 									  */
/* Ca = (-C1+5*C2+3*C3-C4)/6 		  			  */
/* Cb = (-C1+3*C2+5*C3-C4)/6 		  			  */
#define INTERPOLATE_A_COLOR3(c1,c2,c3,c4)  	(((((c2)<<2)+(c2)+((c3)<<1)+(c3)-(c1)-(c4))<<QUANT_ERR_BITS)/6)
#define INTERPOLATE_B_COLOR3(c1,c2,c3,c4)  	(((((c2)<<1)+(c2)+((c3)<<2)+(c3)-(c1)-(c4))<<QUANT_ERR_BITS)/6)
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
	register int c1 = src[0], c4 = src[1];
LOCAL_DEBUG_OUT( "scaling from %d", len );
	--len; --len ;
	while( i < len )
	{
		c4 = src[i+2];
		if( scales[i] == 1 )
		{
			c1 = src[i];     /* that's right we can do that PRIOR as we calculate nothing */
			dst[k] = INTERPOLATE_COLOR1(c1) ;
			++k;
		}else
		{
			register int c2 = src[i], c3 = src[i+1] ;
			c3 = INTERPOLATE_COLOR2(c1,c2,c3,c3);
			dst[k] = (c3&0xFF000000 )?0:c3;
			++k;
			c1 = c2;
		}
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
				register short S = scales[i];
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
}

static inline void
add_component( CARD32 *src, CARD32 *incr, int *scales, int len )
{
	int i = 0;

	len += len&0x01;
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
				dst[i] = src[i]>>1;
				dst[i+1] = src[i+1]>>1;
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
			if( c&0xFFFF0000 ) 
				c = ( c&0x7F000000 )?0:0x0000FFFF;
			dst[i] = c>>(QUANT_ERR_BITS) ;
			if( ++i >= len ) 
				break;
			c = ((c&QUANT_ERR_MASK)>>(QUANT_ERR_BITS+1))+src[i];
		}while(1);
	}else if( ratio == 2 )
	{
		register CARD32 c = src[0];
  	    do
		{
			if( c&0xFFFF0000 ) 
				c = ( c&0x7F000000 )?0:0x00007FFF;
			else
				c = c>>1 ;
			dst[i] = c>>(QUANT_ERR_BITS) ;
			if( ++i >= len ) 
				break;
			c = ((c&QUANT_ERR_MASK)>>(QUANT_ERR_BITS+1))+src[i];
		}while( 1 );
	}else
	{
		register CARD32 c = src[0];
  	    do
		{
			if( c&0xFFFF0000 ) 
				c = ( c&0x7F000000 )?0:0x0000FFFF;
			
			c = c/ratio ;
			dst[i] = c>>(QUANT_ERR_BITS) ;
			if( ++i >= len ) 
				break;
			c = ((c&QUANT_ERR_MASK)>>(QUANT_ERR_BITS+1))+src[i];
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
component_interpolation2( CARD32 *c1, CARD32 *c2, CARD32 *c3, CARD32 *c4, register CARD32 *T, CARD32 *unused, CARD16 unused2, int len)
{
	register int i;
	for( i = 0 ; i < len ; i++ )
	{
		register int rc2 = c2[i], rc3 = c3[i] ;
		T[i] = INTERPOLATE_COLOR2(c1[i],rc2,rc3,c4[i]);
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

/**********************************************************************/
/* ASImage->XImage low level conversion routines : 					  */
/**********************************************************************/
static inline void
fill_ximage_buffer_pseudo (XImage *xim, ASScanline * xim_buf, unsigned int line)
{
 	XColor        color;
	register int i ;
	for( i = 0 ; i < xim_buf->width ; i++ )
	{
		color.pixel = XGetPixel( xim, i, line );
		XQueryColor (dpy, ascolor_colormap, &color);
		xim_buf->red[i] = color.red ;
		xim_buf->green[i] = color.green ;
		xim_buf->blue[i] = color.blue ;
	}
}

static inline void
put_ximage_buffer_pseudo (XImage *xim, ASScanline * xim_buf, unsigned int line)
{
 	XColor        color;
	register int i ;
	for( i = 0 ; i < xim_buf->width ; i++ )
	{
		color.red   = xim_buf->red[i] ;
		color.green = xim_buf->green[i] ;
		color.blue  = xim_buf->blue[i] ;
		XAllocColor (dpy, ascolor_colormap, &color);
		XPutPixel( xim, i, line, color.pixel );
	}
}


static inline void
fill_ximage_buffer (unsigned char *xim_line, ASScanline * xim_buf, int BGR_mode, int byte_order, int bpp)
{
	int           i, width = xim_buf->width;
	register CARD32 *r = xim_buf->red, *g = xim_buf->green, *b = xim_buf->blue;
	register CARD8  *src = (CARD8 *) xim_line;
#ifdef LOCAL_DEBUG
    static int debug_count = 10 ;
#endif
	if (BGR_mode != byte_order)
	{
		r = xim_buf->blue;
		b = xim_buf->red ;
	}
	if ( ascolor_true_depth == 24)
	{
		for (i = 0 ; i < width; i++)
		{   						   			/* must add LSB/MSB checking */
			r[i] = src[0];
			g[i] = src[1];
			b[i] = src[2];
			src += 3;
		}
	} else if (ascolor_true_depth == 32)
	{
		if (byte_order == MSBFirst)
			for (i = 0 ; i < width; i++)
			{
				r[i] = src[1];
				g[i] = src[2];
				b[i] = src[3];
				src+= 4;
			}
		else
			for (i = 0 ; i < width; i++)
			{
				r[i] = src[0];
				g[i] = src[1];
				b[i] = src[2];
				src+=4;
			}
	} else if (ascolor_true_depth == 16)
	{										   /* must add LSB/MSB checking */
LOCAL_DEBUG_OUT( "reading row in 16bpp with %s: ", (byte_order == MSBFirst)?"MSBFirst":"no MSBFirst" );
		if (byte_order == MSBFirst)
			for (i = 0 ; i < width; i++)
			{
				r[i] =  (src[0]&0xF8);
				g[i] = ((src[0]&0x07)<<5)|((src[1]&0xE0)>>3);
				b[i] =  (src[1]&0x1F)<<3;
				src += 2;
			}
		else
			for (i = 0 ; i < width; i++)
			{
				r[i] =  (src[1]&0xF8);
				g[i] = ((src[1]&0x07)<<5)|((src[0]&0xE0)>>3);;
				b[i] =  (src[0]&0x1F)<<3;
#ifdef LOCAL_DEBUG
				if( debug_count > 0 )
				{
fprintf( stderr, "rgb #%2.2lX%2.2lX%2.2lX in: 0x%4.4X( %2.2X %2.2X )\n", r[i], g[i], b[i], *((unsigned short*)src), src[0], src[1] );
					debug_count-- ;
				}
#endif
				src += 2;
			}
	} else if (ascolor_true_depth == 15)
	{										   /* must add LSB/MSB checking */
		if (byte_order == MSBFirst)
			for (i = 0 ; i < width; i++)
			{
				r[i] =  (src[0]&0x7C)<<1;
				g[i] = ((src[0]&0x03)<<6)|((src[1]&0xE0)>>2);
				b[i] =  (src[1]&0x1F)<<3;
				src += 2;
			}
		else
			for (i = 0 ; i < width; i++)
			{
				r[i] =  (src[1]&0x7C)<<1;
				g[i] = ((src[1]&0x03)<<6)|((src[0]&0xE0)>>2);
				b[i] =  (src[0]&0x1F)<<3;
				src += 2;
			}
	} else
	{										   /* below 8 bpp handling */
		for (i = 0 ; i < width; i++)
			pixel_to_color_low( src[i], r+i, g+i, b+i );
	}
}

void
put_ximage_buffer (unsigned char *xim_line, ASScanline * xim_buf, int BGR_mode, int byte_order, int bpp)
{
	int           i, width = xim_buf->width, step = width+(width&0x01);
	register CARD32 *r = xim_buf->red, *g = xim_buf->green, *b = xim_buf->blue;
	register CARD8  *src = (CARD8 *) xim_line;
#ifdef LOCAL_DEBUG
    static int debug_count = 10 ;
#endif

/*fprintf( stderr, "bpp = %d, MSBFirst = %d \n", bpp, byte_order==MSBFirst ) ;*/
	if (BGR_mode != byte_order)
	{
		r = xim_buf->blue;
		b = xim_buf->red ;
	}
	if ( ascolor_true_depth == 24)
	{
		for (i = 0 ; i < width; i++)
		{   						   			/* must add LSB/MSB checking */
			src[0] = ((r[i]&0x00FFFF00)!=0)?0xFF:r[i];
			src[1] = ((g[i]&0x00FFFF00)!=0)?0xFF:g[i];
			src[2] = ((b[i]&0x00FFFF00)!=0)?0xFF:b[i];
			src += 3;
		}
	} else if (ascolor_true_depth == 32)
	{
		if (byte_order == MSBFirst)
			for (i = 0 ; i < width; i++)
			{
				src[1] = ((r[i]&0x00FFFF00)!=0)?0xFF:r[i];
				src[2] = ((g[i]&0x00FFFF00)!=0)?0xFF:g[i];
				src[3] = ((b[i]&0x00FFFF00)!=0)?0xFF:b[i];
				src+= 4;
			}
		else
			for (i = 0 ; i < width; i++)
			{
				src[0] = ((r[i]&0x00FFFF00)!=0)?0xFF:r[i];
				src[1] = ((g[i]&0x00FFFF00)!=0)?0xFF:g[i];
				src[2] = ((b[i]&0x00FFFF00)!=0)?0xFF:b[i];
				src+=4;
			}
	} else if (ascolor_true_depth == 16)
	{										   /* must add LSB/MSB checking */
#if 1
			register CARD32 c = (g[-step]<<20) | (g[0]<<10) | (g[step]);
			register CARD16 *src = (CARD16*)xim_line;
LOCAL_DEBUG_OUT( "writing row in 16bpp with %s: ", (byte_order == MSBFirst)?"MSBFirst":"no MSBFirst" );
			i = 0 ;
			do
			{
				if (byte_order == MSBFirst)
					src[i] = ((c>>17)&0x0007)|((c>>1)&0xE000)|((c>>20)&0x00F8)|((c<<5)&0x1F00);
				else
					src[i] = ((c>>7)&0x07E0)|((c>>12)&0xF800)|((c>>3)&0x001F);
				if( ++i >= width )
					break;
				/* carry over quantization error allow for error diffusion:*/
				c = ((c>>1)&0x00300403)+((g[i-step]<<20) | (g[i]<<10) | (g[i+step]));
				{
					register CARD32 d = c&0x300C0300 ;
					if( d ) 
					{
						if( c&0x30000000 )
							d |= 0x0FF00000;
						if( c&0x000C0000 )
							d |= 0x0003FC00 ;
						if( c&0x00000300 )
							d |= 0x000000FF ;
						c ^= d;
					}
/*fprintf( stderr, "c = 0x%X, d = 0x%X, c^d = 0x%X\n", c, d, c^d );*/
				}
			}while(1);
#else
			register CARD32 red=0, green=0, blue=0;
			for (i = 0 ; i < width; i++)
			{/* diffusion to compensate for quantization error :*/
				red   =  (r[i]+red > 0x00FF)?0x00FF:r[i]+red ;
				blue  =  (b[i]+blue > 0x00FF)?0x00FF:b[i]+blue ;
/*
				green  =  (g[i]+green > 0x00FF)?0x00FF:g[i]+green ;
				src[1] = (green>>5) ;
				src[0] = (CARD8)(0xE0&(green<<3));
*/
				if( g[i] > (0x00FF^green) )
				{
					if (byte_order == MSBFirst)
					{
						src[0] = red|0x07;
						src[1] = (CARD8)(0xE0|(blue>>3));
					}else
					{
						src[1] = red|0x07;
						src[0] = (CARD8)(0xE0|(blue>>3));
					}
  					green = 0x01;
				}else
				{
					green+=g[i];
					if (byte_order == MSBFirst)
					{
						src[0] = (red&0xF8)|(green>>5);
						src[1] = (CARD8)(((green<<3)&0xE0)|(blue>>3));
					}else
					{
						src[1] = (red&0xF8)|(green>>5);
						src[0] = (CARD8)(((green<<3)&0xE0)|(blue>>3));
					}
					green = (green&0x03)>>1;
				}
/*				green = (green&0x03)>>1; */
				red =   (red&0x07)>>1;
				blue =  (blue&0x07)>>1;
				src += 2;
			}
#endif
	} else if (ascolor_true_depth == 15)
	{										   /* must add LSB/MSB checking */
			register CARD32 c = (g[-step]<<20) | (g[0]<<10) | (g[step]);
			register CARD16 *src = (CARD16*)xim_line;
LOCAL_DEBUG_OUT( "writing row in 15bpp with %s: ", (byte_order == MSBFirst)?"MSBFirst":"no MSBFirst" );
			i = 0;
			do
			{
/*				c += (g[i-step]<<20) | (g[i]<<10) | (g[i+step]); */

				if (byte_order == MSBFirst)
					src[i] = ((c>>18)&0x0003)|((c>>2)&0xE000)|((c>>21)&0x007C)|((c<<5)&0x1F00);
				else
					src[i] = ((c>>8)&0x03E0)|((c>>13)&0x7C00)|((c>>3)&0x001F);
				if( ++i >= width )
					break;
			 	/* carry over quantization error allow for error diffusion:*/
				c = ((c>>1)&0x00300C03)+((g[i-step]<<20) | (g[i]<<10) | (g[i+step]));
				{
					register CARD32 d = c&0x300C0300 ;
					if( d )
					{
						if( c&0x30000000 )
							d |= 0x0FF00000;
						if( c&0x000C0000 )
							d |= 0x0003FC00 ;
						if( c&0x00000300 )
							d |= 0x000000FF ;
/*fprintf( stderr, "c = 0x%X, d = 0x%X, c^d = 0x%X\n", c, d, c^d );*/
						c ^= d;
					}
				}
			}while(1);
	} else
	{										   /* below 8 bpp handling */
		for (i = 0 ; i < width; i++)
			src[i] = color_to_pixel_low( r[i], g[i], b[i] );
	}
}

void
output_image_line_fine( ASImageOutput *imout, ASScanline *new_line, int ratio )
{
	/* caching and preprocessing line into our buffer : */
	if( new_line )
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

		if( imout->to_xim )
		{
			if( imout->next_line < imout->xim->height )
			{
				if( ascolor_true_depth == 0 )
					put_ximage_buffer_pseudo( imout->xim, imout->used, imout->next_line );
				else
				{
					put_ximage_buffer (imout->xim_line, imout->used, 0, imout->xim->byte_order, imout->xim->bits_per_pixel );
					imout->xim_line += imout->bpl;
				}
			}
		}else if( imout->next_line < imout->im->height )
			ENCODE_SCANLINE(imout->im,*(imout->used),imout->next_line);
		++(imout->next_line);
	}
	/* rotating the buffers : */
	if( new_line == NULL )
		imout->used = NULL ;
	else
		imout->used = imout->available ;
	imout->available = &(imout->buffer[0]);
	if( imout->available == imout->used )
		imout->available = &(imout->buffer[1]);
}

void
output_image_line_fast( ASImageOutput *imout, ASScanline *new_line, int ratio )
{
	/* caching and preprocessing line into our buffer : */
	if( new_line )
	{
		imout->used = &(imout->buffer[0]);
		SCANLINE_FUNC(fast_output_filter,*(new_line),*(imout->used),ratio,new_line->width);
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
				if( ascolor_true_depth == 0 )
					put_ximage_buffer_pseudo( imout->xim, imout->used, imout->next_line );
				else
				{
					put_ximage_buffer (imout->xim_line, imout->used, 0, imout->xim->byte_order, imout->xim->bits_per_pixel );
					imout->xim_line += imout->bpl;
				}
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
scale_image_down( ASImage *src, ASImage *dst, int h_ratio, int *scales_h, int* scales_v, Bool to_xim)
{
	ASScanline src_line, dst_line, total ;
	int i = 0, k = 0, line_len = MIN(dst->width,src->width);
	ASImageOutput *imout ;

	if((imout = start_image_output( dst, dst->ximage, to_xim, QUANT_ERR_BITS )) == NULL )
		return;

	prepare_scanline( src->width, 0, &src_line );
	prepare_scanline( dst->width, QUANT_ERR_BITS, &dst_line );
	prepare_scanline( dst->width, QUANT_ERR_BITS, &total );
	while( k < dst->height )
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
	stop_image_output( &imout );
}

void
scale_image_up( ASImage *src, ASImage *dst, int h_ratio, int *scales_h, int* scales_v, Bool to_xim)
{
	ASScanline step, src_lines[4], *c1, *c2, *c3, *c4 = NULL, tmp;
	int i = 0, max_i, line_len = MIN(dst->width,src->width);
	ASImageOutput *imout ;

	if((imout = start_image_output( dst, dst->ximage, to_xim, QUANT_ERR_BITS )) == NULL )
		return;

	for( i = 0 ; i < 4 ; i++ )
		prepare_scanline( dst->width, 0, &(src_lines[i]));
	prepare_scanline( src->width, QUANT_ERR_BITS, &tmp );
	prepare_scanline( dst->width, QUANT_ERR_BITS, &step );

	set_component(src_lines[0].red,0x00000F00,0,line_len*3);

	DECODE_SCANLINE(src,tmp,0);
	step.flags = src_lines[0].flags = tmp.flags ;
	CHOOSE_SCANLINE_FUNC(h_ratio,tmp,src_lines[1],scales_h,line_len);

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
		if( S == 2 )
		{
			SCANLINE_COMBINE(component_interpolation2,*c1,*c2,*c3,*c4,*c1,*c1,2,dst->width);
			output_image_line( imout, c1, 1);
		}else
		{
			SCANLINE_COMBINE(start_component_interpolation,*c1,*c2,*c3,*c4,*c1,step,S,dst->width);
			do
			{
				output_image_line( imout, c1, 1);
				if( --S <= 0 )
					break;
				SCANLINE_FUNC(add_component,*c1,step,NULL,dst->width );
 			}while (1);
		}
	}while( ++i < max_i );
	output_image_line( imout, c4, 1);

	for( i = 0 ; i < 4 ; i++ )
		free_scanline(&(src_lines[i]), True);
	free_scanline(&tmp, True);
	free_scanline(&step, True);
	stop_image_output( &imout );
}

ASImage *
scale_asimage( ASImage *src, int to_width, int to_height, Bool to_xim )
{
	ASImage *dst = NULL ;
	int h_ratio ;
	int *scales_h = NULL, *scales_v = NULL;

	if( !check_scale_parameters(src,&to_width,&to_height) )
		return NULL;

	dst = safecalloc(1, sizeof(ASImage));
	asimage_start (dst, to_width, to_height);
	if( to_xim )
		dst->ximage = CreateXImageAndData( dpy, ascolor_visual, ascolor_true_depth, ZPixmap, 0, to_width, to_height );

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
	if( to_height < src->height ) 					   /* scaling down */
		scale_image_down( src, dst, h_ratio, scales_h, scales_v, to_xim );
	else
		scale_image_up( src, dst, h_ratio, scales_h, scales_v, to_xim );
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
asimage_from_ximage (XImage * xim)
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
	prepare_scanline( xim->width, 0, &xim_buf );

	if( ascolor_true_depth == 0 )
		for (i = 0; i < height; i++)
		{
			fill_ximage_buffer_pseudo( xim, &xim_buf, i );
			asimage_add_line (im, IC_RED,   xim_buf.red, i);
			asimage_add_line (im, IC_GREEN, xim_buf.green, i);
			asimage_add_line (im, IC_BLUE,  xim_buf.blue, i);
		}
	else                                       /* TRue color visual */
		for (i = 0; i < height; i++)
		{
			fill_ximage_buffer (xim_line, &xim_buf, 0, xim->byte_order, xim->bits_per_pixel );
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
ximage_from_asimage (ASImage *im)
{
	XImage        *xim = NULL;
	int           i;
	ASScanline    xim_buf;
	ASImageOutput *imout ;

	if (im == NULL)
		return xim;

	xim = CreateXImageAndData( dpy, ascolor_visual, ascolor_true_depth, ZPixmap, 0, im->width, im->height );
	if( (imout = start_image_output( im, xim, True, 0 )) == NULL )
		return xim;

	prepare_scanline( xim->width, 0, &xim_buf );
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
asimage_from_pixmap (Pixmap p, int x, int y, unsigned int width, unsigned int height, unsigned long plane_mask, Bool keep_cache)
{
	XImage       *xim = XGetImage (dpy, p, x, y, width, height, plane_mask, ZPixmap);
	ASImage      *im = NULL;

	if (xim)
	{
		im = asimage_from_ximage (xim);
		if( keep_cache )
			im->ximage = xim ;
		else
			XDestroyImage (xim);
	}
	return im;
}

Pixmap
pixmap_from_asimage(ASImage *im, Window w, GC gc, Bool use_cached)
{
	XImage       *xim ;
	Pixmap        p = None;
	XWindowAttributes attr;

	if( XGetWindowAttributes (dpy, w, &attr) )
	{
		set_ascolor_depth( w, attr.depth );
		if ( !use_cached || im->ximage == NULL )
			xim = ximage_from_asimage( im );
		else
			xim = im->ximage ;
		if (xim != NULL )
		{
			p = XCreatePixmap( dpy, w, xim->width, xim->height, attr.depth );
			XPutImage( dpy, p, gc, xim, 0, 0, 0, 0, xim->width, xim->height );
			if( xim != im->ximage )
				XDestroyImage (xim);
		}
	}
	return p;
}



