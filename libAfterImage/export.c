/* This file contains code for unified image writing into many file formats */
/********************************************************************/
/* Copyright (c) 2001 Sasha Vasko <sashav@sprintmail.com>           */
/********************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
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

/*#define LOCAL_DEBUG */
#define DO_CLOCKING

#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
/* <setjmp.h> is used for the optional error recovery mechanism */

#ifdef HAVE_PNG
/* Include file for users of png library. */
#include <png.h>
#else
#include <setjmp.h>
#endif

#include "afterbase.h"
#ifdef HAVE_JPEG
/* Include file for users of png library. */
#include <jpeglib.h>
#endif
#ifdef HAVE_GIF
#include <gif_lib.h>
#endif
#ifdef HAVE_TIFF
#include <tiff.h>
#include <tiffio.h>
#endif
#ifdef HAVE_LIBXPM
#ifdef HAVE_LIBXPM_X11
#include <X11/xpm.h>
#else
#include <xpm.h>
#endif
#endif

#include "asimage.h"
#include "xcf.h"
#include "xpm.h"
#include "import.h"
#include "export.h"

inline int asimage_decode_line (ASImage * im, ColorPart color, CARD32 * to_buf, unsigned int y, unsigned int skip, unsigned int out_width);

/***********************************************************************************/
/* High level interface : 														   */
as_image_writer_func as_image_file_writers[ASIT_Unknown] =
{
	ASImage2xpm ,
	ASImage2xpm ,
	ASImage2xpm ,
	ASImage2png ,
	ASImage2jpeg,
	ASImage2xcf ,
	ASImage2ppm ,
	ASImage2ppm ,
	ASImage2bmp ,
	ASImage2ico ,
	ASImage2ico ,
	ASImage2gif ,
	ASImage2tiff,
	NULL,
	NULL,
	NULL
};

Bool
ASImage2file( ASImage *im, const char *dir, const char *file,
			  ASImageFileTypes type, int subimage,
			  unsigned int compression, unsigned int quality,
			  int max_colors, int depth )
{
	int   filename_len, dirname_len = 0 ;
	char *realfilename = NULL ;
	Bool  res = False ;

	if( im == NULL && file == NULL ) return False;

	filename_len = strlen(file);
	if( dir != NULL )
		dirname_len = strlen(dir)+1;
	realfilename = safemalloc( dirname_len+filename_len+1 );
	if( dir != NULL )
	{
		strcpy( realfilename, dir );
		realfilename[dirname_len-1] = '/' ;
	}
	strcpy( realfilename+dirname_len, file );

	if( type >= ASIT_Unknown || type < 0 )
		show_error( "Hmm, I don't seem to know anything about format you trying to write file \"%s\" in\n.\tPlease check the manual", realfilename );
   	else if( as_image_file_writers[type] )
   		res = as_image_file_writers[type](im, realfilename, type, subimage, compression, quality, max_colors, depth);
   	else
   		show_error( "Support for the format of image file \"%s\" has not been implemented yet.", realfilename );

	free( realfilename );
	return res;
}

/* hmm do we need pixmap2file ???? */

/***********************************************************************************/
/* Some helper functions :                                                         */

static FILE*
open_writeable_image_file( const char *path )
{
	FILE *fp = NULL;
	if ( path )
		if ((fp = fopen (path, "wb")) == NULL)
			show_error("cannot open image file \"%s\" for writing. Please check permissions.", path);
	return fp ;
}

void
scanline2raw( register CARD8 *row, ASScanline *buf, CARD8 *gamma_table, unsigned int width, Bool grayscale, Bool do_alpha )
{
	register int x = width;

	if( grayscale )
		row += do_alpha? width<<1 : width ;
	else
		row += width*(do_alpha?4:3) ;

	if( gamma_table )
	{
		if( !grayscale )
		{
			while ( --x >= 0 )
			{
				row -= 3 ;
				if( do_alpha )
				{
					--row;
					buf->alpha[x] = row[3];
				}
				buf->xc1[x]  = gamma_table[row[0]];
				buf->xc2[x]= gamma_table[row[1]];
				buf->xc3[x] = gamma_table[row[2]];
			}
		}else /* greyscale */
			while ( --x >= 0 )
			{
				if( do_alpha )
					buf->alpha[x] = *(--row);
				buf->xc1 [x] = buf->xc2[x] = buf->xc3[x]  = gamma_table[*(--row)];
			}
	}else
	{
		if( !grayscale )
		{
			while ( --x >= 0 )
			{
				row -= 3 ;
				if( do_alpha )
				{
					--row;
					buf->alpha[x] = row[3];
				}
				buf->xc1[x]  = row[0];
				buf->xc2[x]= row[1];
				buf->xc3[x] = row[2];
			}
		}else /* greyscale */
			while ( --x >= 0 )
			{
				if( do_alpha )
					buf->alpha[x] = *(--row);
				buf->xc1 [x] = buf->xc2[x] = buf->xc3[x]  = *(--row);
			}
	}
}

/***********************************************************************************/
#define SHOW_PENDING_IMPLEMENTATION_NOTE(f) \
	show_error( "I'm sorry, but " f " image writing is pending implementation. Appreciate your patience" )
#define SHOW_UNSUPPORTED_NOTE(f,path) \
	show_error( "unable to write file \"%s\" - " f " image format is not supported.\n", (path) )


#ifdef HAVE_XPM      /* XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM */

#ifdef LOCAL_DEBUG
Bool print_component( CARD32*, int, unsigned int );
#endif

Bool
ASImage2xpm ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	START_TIME(started);
	LOCAL_DEBUG_CALLER_OUT ("(\"%s\")", path);
	SHOW_PENDING_IMPLEMENTATION_NOTE("XPM");
	SHOW_TIME("image writing",started);
	return False;
}

#else  			/* XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM */

Bool
ASImage2xpm ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	SHOW_UNSUPPORTED_NOTE("XPM",path);
	return False ;
}

#endif 			/* XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM */
/***********************************************************************************/

/***********************************************************************************/
#ifdef HAVE_PNG		/* PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG */
Bool
ASImage2png ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	FILE *outfile;
	png_structp png_ptr  = NULL;
	png_infop   info_ptr = NULL;
	Bool has_alpha = False ;
	Bool greyscale = (depth == 1) ;
	png_byte *row_pointer;
	ASScanline imbuf ;
	int y ;
	START_TIME(started);


	if ((outfile = open_writeable_image_file( path )) == NULL)
		return False;

	png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
    if ( png_ptr != NULL )
    	if( (info_ptr = png_create_info_struct(png_ptr)) != NULL )
			if( setjmp(png_ptr->jmpbuf) )
			{
				png_destroy_info_struct(png_ptr, (png_infopp) &info_ptr);
				info_ptr = NULL ;
    		}

	if( !info_ptr)
	{
		if( png_ptr )
    		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		fclose( outfile );
    	return False;
    }
	png_init_io(png_ptr, outfile);

	if( compression > 0 )
		png_set_compression_level(png_ptr,MIN(compression,99)/10);

	/* lets see if we have alpha channel indeed : */
	for( y = 0 ; y < im->height ; y++ )
		if( im->alpha[y] != NULL )
		{
			has_alpha = True ;
			break;
		}

	png_set_IHDR(png_ptr, info_ptr, im->width, im->height, 8,
		         greyscale ? (has_alpha?PNG_COLOR_TYPE_GRAY_ALPHA:PNG_COLOR_TYPE_GRAY):
		                     (has_alpha?PNG_COLOR_TYPE_RGB_ALPHA:PNG_COLOR_TYPE_RGB),
				 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				 PNG_FILTER_TYPE_DEFAULT );
	/* PNG treats alpha s alevel of opacity,
	 * and so do we - there is no need to reverse it : */
	/*	png_set_invert_alpha(png_ptr); */

	/* starting writing the file : writing info first */
	png_write_info(png_ptr, info_ptr);

	prepare_scanline( im->width, 0, &imbuf, False );
	if( greyscale )
	{
		row_pointer = safemalloc( im->width*(has_alpha?2:1));
		for ( y = 0 ; y < im->height ; y++ )
		{
			register int i = im->width;
			CARD8   *ptr = (CARD8*)row_pointer;
			asimage_decode_line (im, IC_RED,   imbuf.red, y, 0, imbuf.width);
			asimage_decode_line (im, IC_GREEN, imbuf.green, y, 0, imbuf.width);
			asimage_decode_line (im, IC_BLUE,  imbuf.blue, y, 0, imbuf.width);
			if( has_alpha )
			{
				asimage_decode_line (im, IC_ALPHA,  imbuf.alpha, y, 0, imbuf.width);

				while( --i >= 0 ) /* normalized greylevel computing :  */
				{
					ptr[(i<<1)] = (54*imbuf.red[i]+183*imbuf.green[i]+19*imbuf.blue[i])/256 ;
					ptr[(i<<1)+1] = imbuf.alpha[i] ;
				}
			}else
				while( --i >= 0 ) /* normalized greylevel computing :  */
					ptr[i] = (54*imbuf.red[i]+183*imbuf.green[i]+19*imbuf.blue[i])/256 ;
			png_write_rows(png_ptr, &row_pointer, 1);
		}
	}else
	{
		row_pointer = safecalloc( im->width * (has_alpha?4:3), 1 );
		for (y = 0; y < im->height; y++)
		{
			register int i = im->width;
			CARD8   *ptr = (CARD8*)(row_pointer+(i-1)*(has_alpha?4:3)) ;
			asimage_decode_line (im, IC_RED,   imbuf.red, y, 0, imbuf.width);
			asimage_decode_line (im, IC_GREEN, imbuf.green, y, 0, imbuf.width);
			asimage_decode_line (im, IC_BLUE,  imbuf.blue, y, 0, imbuf.width);
			if( has_alpha )
			{
				asimage_decode_line (im, IC_ALPHA,  imbuf.alpha, y, 0, imbuf.width);
				while( --i >= 0 )
				{
					/* 0 is red, 1 is green, 2 is blue, 3 is alpha */
		            ptr[0] = imbuf.red[i] ;
					ptr[1] = imbuf.green[i] ;
					ptr[2] = imbuf.blue[i] ;
					ptr[3] = imbuf.alpha[i] ;
					ptr-=4;
				}
			}else
				while( --i >= 0 )
				{
					ptr[0] = imbuf.red[i] ;
					ptr[1] = imbuf.green[i] ;
					ptr[2] = imbuf.blue[i] ;
					ptr-=3;
				}
			png_write_rows(png_ptr, &row_pointer, 1);
		}
	}

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	free( row_pointer );
	free_scanline(&imbuf, True);
	fclose(outfile);

	SHOW_TIME("image writing", started);
	return False ;
}
#else 			/* PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG */
Bool
ASImage2png ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	SHOW_UNSUPPORTED_NOTE( "PNG", path );
	return False;
}

#endif 			/* PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG */
/***********************************************************************************/


/***********************************************************************************/
#ifdef HAVE_JPEG     /* JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG */
Bool
ASImage2jpeg( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	ASScanline     imbuf;
	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	/* More stuff */
	FILE 		 *outfile;		/* target file */
    JSAMPROW      row_pointer[1];/* pointer to JSAMPLE row[s] */
	int 		  y;
	START_TIME(started);

	if ((outfile = open_writeable_image_file( path )) == NULL)
		return False;

	prepare_scanline( im->width, 0, &imbuf, False );
	/* Step 1: allocate and initialize JPEG compression object */
	/* We have to set up the error handler first, in case the initialization
	* step fails.  (Unlikely, but it could happen if you are out of memory.)
	* This routine fills in the contents of struct jerr, and returns jerr's
	* address which we place into the link field in cinfo.
	*/
	cinfo.err = jpeg_std_error(&jerr);
	/* Now we can initialize the JPEG compression object. */
	jpeg_create_compress(&cinfo);

	/* Step 2: specify data destination (eg, a file) */
	/* Note: steps 2 and 3 can be done in either order. */
	/* Here we use the library-supplied code to send compressed data to a
	* stdio stream.  You can also write your own code to do something else.
	* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	* requires it in order to write binary files.
	*/
	jpeg_stdio_dest(&cinfo, outfile);

	/* Step 3: set parameters for compression */
	cinfo.image_width  = im->width; 	/* image width and height, in pixels */
	cinfo.image_height = im->height;
	cinfo.input_components = (depth==1)?1:3;		    /* # of color components per pixel */
	cinfo.in_color_space   = (depth==1)?JCS_GRAYSCALE:JCS_RGB; 	/* colorspace of input image */
	/* Now use the library's routine to set default compression parameters.
	* (You must set at least cinfo.in_color_space before calling this)*/
	jpeg_set_defaults(&cinfo);
	if( quality > 0 )
		jpeg_set_quality(&cinfo, MIN(quality,100), TRUE /* limit to baseline-JPEG values */);

	/* Step 4: Start compressor */
	/* TRUE ensures that we will write a complete interchange-JPEG file.*/
	jpeg_start_compress(&cinfo, TRUE);

	/* Step 5: while (scan lines remain to be written) */
	/*           jpeg_write_scanlines(...); */

	/* Here we use the library's state variable cinfo.next_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	* To keep things simple, we pass one scanline per call; you can pass
	* more if you wish, though.
	*/
	if( depth == 1 )
	{
		row_pointer[0] = safemalloc( im->width );
		for (y = 0; y < im->height; y++)
		{
			register int i = im->width;
			CARD8   *ptr = (CARD8*)row_pointer[0];
			asimage_decode_line (im, IC_RED,   imbuf.red, y, 0, imbuf.width);
			asimage_decode_line (im, IC_GREEN, imbuf.green, y, 0, imbuf.width);
			asimage_decode_line (im, IC_BLUE,  imbuf.blue, y, 0, imbuf.width);
			while( --i >= 0 ) /* normalized greylevel computing :  */
				ptr[i] = (54*imbuf.red[i]+183*imbuf.green[i]+19*imbuf.blue[i])/256 ;
			(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}
	}else
	{
		row_pointer[0] = safemalloc( im->width * 3 );
		for (y = 0; y < im->height; y++)
		{
			register int i = im->width;
			CARD8   *ptr = (CARD8*)(row_pointer[0]+(i-1)*3) ;
LOCAL_DEBUG_OUT( "decoding  row %d", y );
			asimage_decode_line (im, IC_RED,   imbuf.red, y, 0, imbuf.width);
			asimage_decode_line (im, IC_GREEN, imbuf.green, y, 0, imbuf.width);
			asimage_decode_line (im, IC_BLUE,  imbuf.blue, y, 0, imbuf.width);
LOCAL_DEBUG_OUT( "building  row %d", y );
			while( --i >= 0 )
			{
				ptr[0] = imbuf.red[i] ;
				ptr[1] = imbuf.green[i] ;
				ptr[2] = imbuf.blue[i] ;
				ptr-=3;
			}
LOCAL_DEBUG_OUT( "writing  row %d", y );
			(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}
	}
LOCAL_DEBUG_OUT( "done writing image%s","" );
/*	free(buffer); */

	/* Step 6: Finish compression and release JPEG compression object*/
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	free_scanline(&imbuf, True);
	fclose(outfile);

	SHOW_TIME("image export",started);
	LOCAL_DEBUG_OUT("done writing JPEG image \"%s\"", path);
	return False ;
}
#else 			/* JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG */

Bool
ASImage2jpeg( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	SHOW_UNSUPPORTED_NOTE( "JPEG", path );
	return False;
}

#endif 			/* JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG */
/***********************************************************************************/

/***********************************************************************************/
/* XCF - GIMP's native file format : 											   */

Bool
ASImage2xcf ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	/* More stuff */
	XcfImage  *xcf_im = NULL;
	START_TIME(started);

	SHOW_PENDING_IMPLEMENTATION_NOTE("XCF");
	if( xcf_im == NULL )
		return False;

#ifdef LOCAL_DEBUG
	print_xcf_image( xcf_im );
#endif
	/* Make a one-row-high sample array that will go away when done with image */
	SHOW_TIME("write initialization",started);

	free_xcf_image(xcf_im);
	SHOW_TIME("image export",started);
	return True ;
}

/***********************************************************************************/
/* PPM/PNM file format : 											   				   */
Bool
ASImage2ppm ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	START_TIME(started);
	SHOW_PENDING_IMPLEMENTATION_NOTE("PPM");
	SHOW_TIME("image export",started);
	return False;
}

/***********************************************************************************/
/* Windows BMP file format :   									   				   */
Bool
ASImage2bmp ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	START_TIME(started);
	SHOW_PENDING_IMPLEMENTATION_NOTE("BMP");
	SHOW_TIME("image export",started);
	return False;
}

/***********************************************************************************/
/* Windows ICO/CUR file format :   									   			   */
Bool
ASImage2ico ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	START_TIME(started);
	SHOW_PENDING_IMPLEMENTATION_NOTE("ICO");
	SHOW_TIME("image export",started);
	return False;
}

/***********************************************************************************/
#ifdef HAVE_GIF		/* GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF */
Bool ASImage2gif( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	START_TIME(started);
	show_error( "I'm sorry but GIF image export is disabled due to stupid licensing issues. Blame UNISYS"):
 ÿÿÿSHOW_TIME("image export",started);
	return False ;
}
#else 			/* GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF */
Bool
ASImage2gif( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	SHOW_UNSUPPORTED_NOTE("GIF",path);
	return False ;
}
#endif			/* GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF */

#ifdef HAVE_TIFF/* TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF */
Bool
ASImage2tiff( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	START_TIME(started);
	SHOW_PENDING_IMPLEMENTATION_NOTE("ICO");
	SHOW_TIME("image export",started);
	return False;
}
#else 			/* TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF */

Bool
ASImage2tiff( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth )
{
	SHOW_UNSUPPORTED_NOTE("TIFF",path);
	return False ;
}
#endif			/* TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF */
