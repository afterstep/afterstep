/* This file contains code for unified image writing into many file formats */
/********************************************************************/
/* Copyright (c) 2001 Sasha Vasko <sasha at aftercode.net>           */
/********************************************************************/
/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

/*#define LOCAL_DEBUG*/
/*#define DO_CLOCKING*/

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
#include "ungif.h"
#include "import.h"
#include "export.h"
#include "ascmap.h"

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
			  ASImageFileTypes type, ASImageExportParams *params )
{
	int   filename_len, dirname_len = 0 ;
	char *realfilename = NULL ;
	Bool  res = False ;

	if( im == NULL || file == NULL ) return False;

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
   		res = as_image_file_writers[type](im, realfilename, params);
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
ASImage2xpm ( ASImage *im, const char *path, ASImageExportParams *params )
{
	FILE *outfile;
	int y, x ;
	int *mapped_im, *row_pointer ;
	ASColormap         cmap;
	ASXpmCharmap       xpm_cmap ;
	int transp_idx = 0;
	START_TIME(started);
	ASXpmExportParams defaults = { ASIT_Xpm, EXPORT_ALPHA, 4, 127, 512 } ;
	register char *ptr ;

	LOCAL_DEBUG_CALLER_OUT ("(\"%s\")", path);

	if( params == NULL )
		params = (ASImageExportParams *)&defaults ;

	if ((outfile = open_writeable_image_file( path )) == NULL)
		return False;

	mapped_im = colormap_asimage( im, &cmap, params->xpm.max_colors, params->xpm.dither, params->xpm.opaque_threshold );
	if( !get_flags( params->xpm.flags, EXPORT_ALPHA) )
		cmap.has_opaque = False ;
	else
		transp_idx = cmap.count ;

LOCAL_DEBUG_OUT("building charmap%s","");
	build_xpm_charmap( &cmap, cmap.has_opaque, &xpm_cmap );
	SHOW_TIME("charmap calculation",started);

LOCAL_DEBUG_OUT("writing file%s","");
	fprintf( outfile, "/* XPM */\nstatic char *asxpm[] = {\n/* columns rows colors chars-per-pixel */\n"
					  "\"%d %d %d %d\",\n", im->width, im->height, xpm_cmap.count,  xpm_cmap.cpp );
    ptr = &(xpm_cmap.char_code[0]);
	for( y = 0 ; y < cmap.count ; y++ )
	{
		fprintf( outfile, "\"%s c #%2.2X%2.2X%2.2X\",\n", ptr, cmap.entries[y].red, cmap.entries[y].green, cmap.entries[y].blue );
		ptr += xpm_cmap.cpp+1 ;
	}
	if( cmap.has_opaque && y < xpm_cmap.count )
		fprintf( outfile, "\"%s c None\",\n", ptr );
	SHOW_TIME("image header writing",started);

	row_pointer = mapped_im ;
	for( y = 0 ; y < im->height ; y++ )
	{
		fputc( '"', outfile );
		for( x = 0; x < im->width ; x++ )
		{
			register int idx = (row_pointer[x] >= 0)? row_pointer[x] : transp_idx ;
			register char *ptr = &(xpm_cmap.char_code[idx*(xpm_cmap.cpp+1)]) ;
			if( idx >= cmap.count )
				fprintf( stderr, "(%d,%d) -> %d, %d: %s\n", x, y, idx, row_pointer[x], ptr );
			while( *ptr )
				fputc( *(ptr++), outfile );
		}
		row_pointer += im->width ;
		fputc( '"', outfile );
		if( y < im->height-1 )
			fputc( ',', outfile );
		fputc( '\n', outfile );
	}
	fprintf( outfile, "};\n" );
	fclose( outfile );

	SHOW_TIME("image writing",started);
	destroy_xpm_charmap( &xpm_cmap, True );
	free( mapped_im );
	destroy_colormap( &cmap, True );

	SHOW_TIME("total",started);
	return True;
}

#else  			/* XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM */

Bool
ASImage2xpm ( ASImage *im, const char *path,  ASImageExportParams *params )
{
	SHOW_UNSUPPORTED_NOTE("XPM",path);
	return False ;
}

#endif 			/* XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM */
/***********************************************************************************/

/***********************************************************************************/
#ifdef HAVE_PNG		/* PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG */
Bool
ASImage2png ( ASImage *im, const char *path, register ASImageExportParams *params )
{
	FILE *outfile;
	png_structp png_ptr  = NULL;
	png_infop   info_ptr = NULL;
	png_byte *row_pointer;
	ASScanline imbuf ;
	int y ;
	Bool has_alpha;
	Bool grayscale;
	int compression;
	START_TIME(started);
	static ASPngExportParams defaults = { ASIT_Png, EXPORT_ALPHA, -1 };

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

	if( params == NULL )
	{
		compression = defaults.compression ;
		grayscale = get_flags(defaults.flags, EXPORT_GRAYSCALE );
		has_alpha = get_flags(defaults.flags, EXPORT_ALPHA );
	}else
	{
		compression = params->png.compression ;
		grayscale = get_flags(params->png.flags, EXPORT_GRAYSCALE );
		has_alpha = get_flags(params->png.flags, EXPORT_ALPHA );
	}

	if( compression > 0 )
		png_set_compression_level(png_ptr,MIN(compression,99)/10);

	/* lets see if we have alpha channel indeed : */
	if( has_alpha )
	{
		if( !get_flags( get_asimage_chanmask(im), SCL_DO_ALPHA) )
			has_alpha = False ;
	}
	png_set_IHDR(png_ptr, info_ptr, im->width, im->height, 8,
		         grayscale ? (has_alpha?PNG_COLOR_TYPE_GRAY_ALPHA:PNG_COLOR_TYPE_GRAY):
		                     (has_alpha?PNG_COLOR_TYPE_RGB_ALPHA:PNG_COLOR_TYPE_RGB),
				 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				 PNG_FILTER_TYPE_DEFAULT );
	/* PNG treats alpha s alevel of opacity,
	 * and so do we - there is no need to reverse it : */
	/*	png_set_invert_alpha(png_ptr); */

	/* starting writing the file : writing info first */
	png_write_info(png_ptr, info_ptr);

	prepare_scanline( im->width, 0, &imbuf, False );
	if( grayscale )
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

				while( --i >= 0 ) /* normalized graylevel computing :  */
				{
					ptr[(i<<1)] = (54*imbuf.red[i]+183*imbuf.green[i]+19*imbuf.blue[i])/256 ;
					ptr[(i<<1)+1] = imbuf.alpha[i] ;
				}
			}else
				while( --i >= 0 ) /* normalized graylevel computing :  */
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
	return True ;
}
#else 			/* PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG */
Bool
ASImage2png ( ASImage *im, const char *path,  ASImageExportParams *params )
{
	SHOW_UNSUPPORTED_NOTE( "PNG", path );
	return False;
}

#endif 			/* PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG */
/***********************************************************************************/


/***********************************************************************************/
#ifdef HAVE_JPEG     /* JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG */
Bool
ASImage2jpeg( ASImage *im, const char *path,  ASImageExportParams *params )
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
	ASJpegExportParams defaults = { ASIT_Jpeg, 0, -1 };
	Bool grayscale;
	START_TIME(started);

	if( params == NULL )
		params = (ASImageExportParams *)&defaults ;

	if ((outfile = open_writeable_image_file( path )) == NULL)
		return False;

	grayscale = get_flags(params->jpeg.flags, EXPORT_GRAYSCALE );

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
	cinfo.input_components = (grayscale)?1:3;		    /* # of color components per pixel */
	cinfo.in_color_space   = (grayscale)?JCS_GRAYSCALE:JCS_RGB; 	/* colorspace of input image */
	/* Now use the library's routine to set default compression parameters.
	* (You must set at least cinfo.in_color_space before calling this)*/
	jpeg_set_defaults(&cinfo);
	if( params->jpeg.quality > 0 )
		jpeg_set_quality(&cinfo, MIN(params->jpeg.quality,100), TRUE /* limit to baseline-JPEG values */);

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
	if( grayscale )
	{
		row_pointer[0] = safemalloc( im->width );
		for (y = 0; y < im->height; y++)
		{
			register int i = im->width;
			CARD8   *ptr = (CARD8*)row_pointer[0];
			asimage_decode_line (im, IC_RED,   imbuf.red, y, 0, imbuf.width);
			asimage_decode_line (im, IC_GREEN, imbuf.green, y, 0, imbuf.width);
			asimage_decode_line (im, IC_BLUE,  imbuf.blue, y, 0, imbuf.width);
			while( --i >= 0 ) /* normalized graylevel computing :  */
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
	return True ;
}
#else 			/* JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG */

Bool
ASImage2jpeg( ASImage *im, const char *path,  ASImageExportParams *params )
{
	SHOW_UNSUPPORTED_NOTE( "JPEG", path );
	return False;
}

#endif 			/* JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG */
/***********************************************************************************/

/***********************************************************************************/
/* XCF - GIMP's native file format : 											   */

Bool
ASImage2xcf ( ASImage *im, const char *path,  ASImageExportParams *params )
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
	return False ;
}

/***********************************************************************************/
/* PPM/PNM file format : 											   				   */
Bool
ASImage2ppm ( ASImage *im, const char *path,  ASImageExportParams *params )
{
	START_TIME(started);
	SHOW_PENDING_IMPLEMENTATION_NOTE("PPM");
	SHOW_TIME("image export",started);
	return False;
}

/***********************************************************************************/
/* Windows BMP file format :   									   				   */
Bool
ASImage2bmp ( ASImage *im, const char *path,  ASImageExportParams *params )
{
	START_TIME(started);
	SHOW_PENDING_IMPLEMENTATION_NOTE("BMP");
	SHOW_TIME("image export",started);
	return False;
}

/***********************************************************************************/
/* Windows ICO/CUR file format :   									   			   */
Bool
ASImage2ico ( ASImage *im, const char *path,  ASImageExportParams *params )
{
	START_TIME(started);
	SHOW_PENDING_IMPLEMENTATION_NOTE("ICO");
	SHOW_TIME("image export",started);
	return False;
}

/***********************************************************************************/
#ifdef HAVE_GIF		/* GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF */

Bool ASImage2gif( ASImage *im, const char *path,  ASImageExportParams *params )
{
	FILE *outfile = NULL, *infile = NULL;
	GifFileType *gif = NULL ;
	ColorMapObject *gif_cmap ;
	Bool dont_save_cmap = False ;
	ASGifExportParams defaults = { ASIT_Gif,EXPORT_ALPHA|EXPORT_APPEND, 3, 127, 10 };
	ASColormap         cmap;
	int *mapped_im ;
	int y ;
	GifPixelType *row_pointer ;
	Bool new_image = True ;
	START_TIME(started);
	int cmap_size = 1;
	unsigned char gce_bytes[5] = {0x01, 0x0, 0x0, 0x0, 0x0 }; /* Graphic Control Extension bytes : 
	                                                           * first byte - flags (0x01 for transparency )
															   * second and third bytes - animation delay
															   * forth byte - transoparent pixel value.
															   */
	LOCAL_DEBUG_CALLER_OUT ("(\"%s\")", path);

	if( params == NULL )
		params = (ASImageExportParams *)&defaults ;

	mapped_im = colormap_asimage( im, &cmap, 256, params->gif.dither, params->gif.opaque_threshold );

	if( get_flags( params->gif.flags, EXPORT_ALPHA) &&
		get_flags( get_asimage_chanmask(im), SCL_DO_ALPHA) )
		gce_bytes[GIF_GCE_TRANSPARENCY_BYTE] = cmap.count ;
	else
		gce_bytes[0] = 0 ;		
	gce_bytes[1] = (params->gif.animate_delay>>8)&0x00FF;
	gce_bytes[2] =  params->gif.animate_delay&0x00FF;

	while( cmap_size < 256 && cmap_size < cmap.count )
		cmap_size = cmap_size<<1 ;
	if( (gif_cmap = MakeMapObject(cmap_size, NULL )) == NULL )
	{
		ASIM_PrintGifError();
		return False;
	}
	memcpy( &(gif_cmap->Colors[0]), &(cmap.entries[0]), MIN(cmap.count,(unsigned int)cmap_size)*3 );

	if( get_flags(params->gif.flags, EXPORT_APPEND) )
		infile = fopen( path, "rb" );
	if( infile != NULL )
	{
		SavedImage *images = NULL ;
		int count = 0 ;
		/* TODO: do something about multiimage files !!! */
		gif = open_gif_read(infile);
		if( gif == NULL || get_gif_saved_images(gif, -1, &images, &count) == GIF_ERROR)
		{
			ASIM_PrintGifError();
			if( gif )
				DGifCloseFile(gif);
			fclose( infile );
			gif = NULL ;
		}else
		{
			GifFileType gif_src ;

			new_image = False ;
			gif_src = *gif ;
			gif->SColorMap = NULL ;
			gif->Image.ColorMap = NULL ;
			DGifCloseFile(gif);
			fclose( infile );
			outfile = open_writeable_image_file( path );
			gif = EGifOpenFileHandle(fileno(outfile));
			if( gif != NULL )
			{
				int status;
				if( ( status = EGifPutScreenDesc(gif, gif_src.SWidth, gif_src.SHeight,
				                       gif_src.SColorResolution,
									   gif_src.SBackGroundColor,
									   gif_src.SColorMap )) == GIF_OK )
					status = write_gif_saved_images( gif, images, count );
				if( status != GIF_OK )
					ASIM_PrintGifError();
			}
			if( gif_src.SColorMap )
			{  /* we only want to save private colormap if it is any different from
			    * screen colormap ( saves us  768 bytes per image ) */
				if( gif_cmap->ColorCount == gif_src.SColorMap->ColorCount )
					dont_save_cmap = ( memcmp( gif_cmap->Colors, gif_src.SColorMap->Colors, gif_cmap->ColorCount*sizeof(GifColorType)) == 0 );
				FreeMapObject(gif_src.SColorMap);
			}
			if( gif )
			{
				EGifPutExtension(gif, 0xf9, 5, &(gce_bytes[0]));
				if( EGifPutImageDesc(gif, 0, 0, im->width, im->height, FALSE, (dont_save_cmap)?NULL:gif_cmap ) == GIF_ERROR )
					ASIM_PrintGifError();
			}
		}
		free_gif_saved_images( images, count );
	}

	if( gif == NULL )
	{
		outfile = open_writeable_image_file( path );
		if( (gif = EGifOpenFileHandle(fileno(outfile))) == NULL )
			ASIM_PrintGifError();
	}

	if( new_image && gif )
	{
		if( EGifPutScreenDesc(gif, im->width, im->height, cmap_size, 0, gif_cmap ) == GIF_ERROR )
			ASIM_PrintGifError();
		EGifPutExtension(gif, 0xf9, 5, &(gce_bytes[0]));
		if( EGifPutImageDesc(gif, 0, 0, im->width, im->height, FALSE, NULL ) == GIF_ERROR )
			ASIM_PrintGifError();
	}

	if( gif_cmap )
	{
		FreeMapObject(gif_cmap);
		gif_cmap = NULL ;
	}
	if( gif )
	{
		row_pointer = safemalloc( im->width*sizeof(GifPixelType));

		/* it appears to be much faster to write image out in line by line fashion */
		for( y = 0 ; y < im->height ; y++ )
		{
			register int x = im->width ;
			register int *src = mapped_im + x*y;
			while( --x >= 0 )
				row_pointer[x] = src[x] ;
			if( EGifPutLine(gif, row_pointer, im->width)  == GIF_ERROR)
				ASIM_PrintGifError();
		}
		free( row_pointer );
		if (EGifCloseFile(gif) == GIF_ERROR)
			ASIM_PrintGifError();
	}
	free( mapped_im );
	destroy_colormap( &cmap, True );

	SHOW_TIME("image export",started);
	return True ;
}
#else 			/* GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF */
Bool
ASImage2gif( ASImage *im, const char *path, ASImageExportParams *params )
{
	SHOW_UNSUPPORTED_NOTE("GIF",path);
	return False ;
}
#endif			/* GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF */

#ifdef HAVE_TIFF/* TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF */
Bool
ASImage2tiff( ASImage *im, const char *path, ASImageExportParams *params)
{
	TIFF *out;
	ASTiffExportParams defaults = { ASIT_Tiff, 0, -1, TIFF_COMPRESSION_NONE, 100 };
	uint16 photometric = PHOTOMETRIC_RGB;
	tsize_t linebytes, scanline;
	ASScanline     imbuf;
	unsigned char* buf;
	CARD32  row ;
	Bool has_alpha ;
	int nsamples = 3 ;
	START_TIME(started);


	if( params == NULL )
		params = (ASImageExportParams *)&defaults ;

	out = TIFFOpen(path, "w");
	if (out == NULL)
		return False;
	/* I don't really know why by grayscale images in Tiff does not work :(
	 * still here is the code :*/
	if( get_flags( params->tiff.flags, EXPORT_GRAYSCALE ) )
		nsamples = 1 ;
	has_alpha = get_flags( params->tiff.flags, EXPORT_ALPHA );
	if( has_alpha )
	{
		if( !get_flags( get_asimage_chanmask(im), SCL_DO_ALPHA) )
			has_alpha = False ;
		else
			++nsamples ;
	}

	TIFFSetField(out, TIFFTAG_IMAGEWIDTH, (uint32) im->width);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, (uint32) im->height);
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, nsamples);
	if (has_alpha)
	{
	    uint16 v[1];
	    v[0] = EXTRASAMPLE_UNASSALPHA;
	    TIFFSetField(out, TIFFTAG_EXTRASAMPLES, 1, v);
	}

	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE,   8);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	if( params->tiff.compression_type == -1  )
		params->tiff.compression_type = defaults.compression_type ;
	TIFFSetField(out, TIFFTAG_COMPRESSION,  params->tiff.compression_type);
	switch (params->tiff.compression_type )
	{
		case COMPRESSION_JPEG:
			photometric = PHOTOMETRIC_YCBCR;
			if( params->tiff.jpeg_quality > 0 )
				TIFFSetField(out, TIFFTAG_JPEGQUALITY, params->tiff.jpeg_quality );
			TIFFSetField( out, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB );
			break;
	}
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);

	linebytes = im->width*nsamples;
	scanline = TIFFScanlineSize(out);
	if (scanline > linebytes)
	{
		buf = (unsigned char *)_TIFFmalloc(scanline);
		_TIFFmemset(buf+linebytes, 0, scanline-linebytes);
	} else
		buf = (unsigned char *)_TIFFmalloc(linebytes);
	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
				 TIFFDefaultStripSize(out, params->tiff.rows_per_strip));

	prepare_scanline( im->width, 0, &imbuf, False );

	for (row = 0; row < im->height; ++row)
	{
		register int i = im->width, k = (im->width-1)*nsamples ;
		asimage_decode_line (im, IC_RED,   imbuf.red, row, 0, imbuf.width);
		asimage_decode_line (im, IC_GREEN, imbuf.green, row, 0, imbuf.width);
		asimage_decode_line (im, IC_BLUE,  imbuf.blue, row, 0, imbuf.width);
		if( has_alpha )
		{
			asimage_decode_line (im, IC_ALPHA,  imbuf.alpha, row, 0, imbuf.width);
			if( nsamples == 2 )
				while ( --i >= 0 )
				{
					buf[k+1] = imbuf.alpha[i] ;
					buf[k] = (54*imbuf.red[i]+183*imbuf.green[i]+19*imbuf.blue[i])/256 ;
					k-= 2;
				}
			else
				while ( --i >= 0 )
				{
					buf[k+3] = imbuf.alpha[i] ;
					buf[k+2] = imbuf.blue[i] ;
					buf[k+1] = imbuf.green[i] ;
					buf[k] = imbuf.red[i] ;
					k-= 4;
				}
		}else if( nsamples == 1 )
			while ( --i >= 0 )
				buf[k--] = (54*imbuf.red[i]+183*imbuf.green[i]+19*imbuf.blue[i])/256 ;
		else
			while ( --i >= 0 )
			{
				buf[k+2] = imbuf.blue[i] ;
				buf[k+1] = imbuf.green[i] ;
				buf[k] = imbuf.red[i] ;
				k-= 3;
			}

		if (TIFFWriteScanline(out, buf, row, 0) < 0)
			break;
	}
	(void) TIFFClose(out);
	SHOW_TIME("image export",started);
	return True;
}
#else 			/* TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF */

Bool
ASImage2tiff( ASImage *im, const char *path, ASImageExportParams *params )
{
	SHOW_UNSUPPORTED_NOTE("TIFF",path);
	return False ;
}
#endif			/* TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF */
