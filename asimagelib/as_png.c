/* This file contains code for unified image loading from JPEG file. */
/********************************************************************/
/* Copyright (c) 1998 Sasha Vasko   <sasha at aftercode.net>         */
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#define LIBASIMAGE

/*#define DEBUG_AS_PNG */
/*#define DO_CLOCKING     */
/*#define GETPIXEL_PUTPIXEL */

#include "../configure.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/xpm.h>
#include <X11/Xmd.h>

#ifdef PNG
/*
 * Include file for users of png library.
 */

#include <png.h>

/*
 * <setjmp.h> is used for the optional error recovery mechanism shown in
 * the second part of the example.
 */

#include <setjmp.h>


#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/style.h"
#include "../include/screen.h"
#include "../include/ascolor.h"

/* our input-output data structures definitions */
#include "../include/loadimg.h"
#include "../include/XImage_utils.h"

#ifndef min
#define min(x,y) ((x<y)?x:y)
#endif


#define PNG_ROWS_TO_READ_AT_A_TIME  1

#ifdef DEBUG_AS_PNG
#define LOG1(a)       fprintf( stderr, a );
#define LOG2(a,b)    fprintf( stderr, a, b );
#define LOG3(a,b,c)    fprintf( stderr, a, b, c );
#define LOG4(a,b,c,d)    fprintf( stderr, a, b, c, d );
#else
#define LOG1(a)
#define LOG2(a,b)
#define LOG3(a,b,c)
#define LOG4(a,b,c,d)
#endif



void put_png_scanline (int y, png_bytep row, LImageParams * pParams, int color_type, int row_bytes);
void LoadPNGFile (LImageParams * pParams);

void
put_png_scanline (int y,	/* y -coord   */
		  png_bytep row,	/* JSAMPLE's array */
		  LImageParams * pParams,	/* Load parameters from caller */
		  int color_type,
		  int row_bytes)
{
  ASCOLOR color;
  png_bytep src_row = row, end_row = row + row_bytes;
  register int x;
  Pixel alpha;
  int bytes_done = 0;
  DECLARE_XIMAGE_CURSOR (pParams->m_pImage, dst8, y);

  /* no checks for input parameters - all should be done before */
  for (x = 0; x < pParams->m_width && row < end_row; x++)
    {
      if (pParams->m_x_net)
	{
	  if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	    row = src_row + pParams->m_x_net[x];
	  else
	    row = src_row + pParams->m_x_net[x] * 3;
	  if (color_type & PNG_COLOR_MASK_ALPHA)
	    row += pParams->m_x_net[x];
	}

      /* handle color or grayscale jpegs */
      if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
	  color = MAKE_ASCOLOR_GRAY8 (*row);
	  row++;
	}
      else
	{
	  if (row + 3 > end_row)
	    break;
	  color = MAKE_ASCOLOR_RGB8 (row[0], row[1], row[2]);
	  row += 3;
	}

      PUT_ASCOLOR (pParams->m_pImage, color, dst8, x, y);

      if (row < end_row && (color_type & PNG_COLOR_MASK_ALPHA))
	{
	  alpha = (*row++) << 8;
	  bytes_done++;
	  if (pParams->m_pMaskImage)
	    XPutPixel (pParams->m_pMaskImage, x, y, (alpha > 0) ? 1 : 0);

	}
    }
}

void
LoadPNGFile (LImageParams * pParams)
{
  png_structp png_ptr;
  png_infop info_ptr;
  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  FILE *fp;
  int intent;
  int row_bytes = 0;
  int row, offset = 0;
  png_bytep row_data;
  double image_gamma = 0.0;

  LOG1 ("\nEntering LoadPNGFile...")
    if ((fp = fopen (pParams->m_realfilename, "rb")) == NULL)
    {
      fprintf (stderr, "can't open %s\n", pParams->m_realfilename);
      return;
    }

  /* Create and initialize the png_struct with the desired error handler
   * functions.  If you want to use the default stderr and longjump method,
   * you can supply NULL for the last three parameters.  We also supply the
   * the compiler header file version, so that we know if the application
   * was compiled with a compatible version of the library.  REQUIRED
   */
  png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (png_ptr == NULL)
    {
      fclose (fp);
      return;
    }

  /* Allocate/initialize the memory for image information.  REQUIRED. */
  info_ptr = png_create_info_struct (png_ptr);
  if (info_ptr == NULL)
    {
      fclose (fp);
      png_destroy_read_struct (&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
      return;
    }

  /* Set error handling if you are using the setjmp/longjmp method (this is
   * the normal method of doing things with libpng).  REQUIRED unless you
   * set up your own error handlers in the png_create_read_struct() earlier.
   */
  if (setjmp (png_ptr->jmpbuf))
    {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp) NULL);
      fclose (fp);
      /* If we get here, we had a problem reading the file */
      return;
    }

  /* Set up the input control if you are using standard C streams */
  png_init_io (png_ptr, fp);

  /* If we have already read some of the signature */
  /* png_set_sig_bytes(png_ptr, sig_read); */

  /* The call to png_read_info() gives us all of the information from the
   * PNG file before the first IDAT (image data chunk).  REQUIRED
   */
  png_read_info (png_ptr, info_ptr);

  png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		&interlace_type, NULL, NULL);

/**** Set up the data transformations you want.  Note that these are all
 **** optional.  Only call them if you want/need them.  Many of the
 **** transformations only work on specific types of images, and many
 **** are mutually exclusive.
 ****/
  LOG1 ("\n Checking image size")
    CheckImageSize (pParams, width, height);

  /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
   * byte into separate bytes (useful for paletted and grayscale images).
   */
  LOG1 ("\n Checking image depth")
    if (bit_depth < 8)
    {
      png_set_packing (png_ptr);
      bit_depth = 8;
      LOG1 ("\n  Depth is < 8  - png_set_packing")
    }

  /* tell libpng to strip 16 bit/color files down to 8 bits/color */
  if (bit_depth == 16)
    {
      png_set_strip_16 (png_ptr);
      bit_depth = 8;
      LOG1 ("\n  Depth is == 16  - png_set_strip_16")
    }

  /* Expand paletted colors into true RGB triplets */
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
      LOG1 ("\n   png_set_expand (image is PALETTED )")
	png_set_expand (png_ptr);
      color_type = PNG_COLOR_TYPE_RGB;
    }

  /* Expand paletted or RGB images with transparency to full alpha channels
   * so the data will be available as RGBA quartets.
   */
/*
   LOG1( "\n converting to ALPHA" )
   if( color_type& == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY )
   {

   if( png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
   {
   LOG1( "\n  png_set_expand  - to get ALPHA channel")
   png_set_expand(png_ptr);
   }
   else  png_set_filler( png_ptr, 0xFF, PNG_FILLER_AFTER );
   if( color_type == PNG_COLOR_TYPE_RGB ) color_type = PNG_COLOR_TYPE_RGB_ALPHA ;
   else color_type = PNG_COLOR_TYPE_GRAY_ALPHA ;
   }
 */
  /* Change the order of packed pixels to least significant bit first
   * (not useful if you are using png_set_packing). */
  /*  png_set_packswap(png_ptr); */
  /* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
  /* if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) */
  /*   png_set_expand(png_ptr); */
  /* Set the background color to draw transparent and alpha images over.
   * It is possible to set the red, green, and blue components directly
   * for paletted images instead of supplying a palette index.  Note that
   * even if the PNG file supplies a background, you are not required to
   * use it - you should use the (solid) application background if it has one.
   * if (png_get_bKGD(png_ptr, info_ptr, &image_background))
   * png_set_background(png_ptr, image_background,
   *                     PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
   *  else
   *     png_set_background(png_ptr, &my_background,
   *                     PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
   *
   * Some suggestions as to how to get a screen gamma value */

  /* Note that screen gamma is (display_gamma/viewing_gamma) */
  /*This is one way that applications share the same screen gamma value
     else */
  /* Tell libpng to handle the gamma conversion for you.  The second call
   * is a good guess for PC generated images, but it should be configurable
   * by the user at run time by the user.  It is strongly suggested that
   * your application support gamma correction.
   */
  LOG1 ("\n Setting Up Gamma")
    if (png_get_sRGB (png_ptr, info_ptr, &intent))
    png_set_sRGB (png_ptr, info_ptr, image_gamma);
  else if (png_get_gAMA (png_ptr, info_ptr, &image_gamma))
    png_set_gamma (png_ptr, pParams->m_gamma, image_gamma);
  else
    png_set_gamma (png_ptr, pParams->m_gamma, 0.50);

  /* Dither RGB files down to 8 bit palette or reduce palettes
   * to the number of colors available on your screen.
   *
   * Sasha: need some consideration
   *  if (color_type & PNG_COLOR_MASK_COLOR)
   * {
   *    png_uint_32 num_palette;
   *    png_colorp palette;
   *
   * This reduces the image to the application supplied palette
   * An array of colors to which the image should be dithered
   *
   *      png_color std_color_cube[MAX_SCREEN_COLORS];
   *
   *      png_set_dither(png_ptr, std_color_cube, MAX_SCREEN_COLORS,
   *      MAX_SCREEN_COLORS, NULL, 0);
   * }
   */
  /* Turn on interlace handling.  REQUIRED if you are not using
   * png_read_image().  To see how to handle interlacing passes,
   * see the png_read_row() method below:
   * number_passes = png_set_interlace_handling(png_ptr);
   */

  /* Optional call to gamma correct and add the background to the palette
   * and update info structure.  REQUIRED if you are expecting libpng to
   * update the palette for you (ie you selected such a transform above).
   */
  LOG1 ("\n png_read_update_info")
    png_read_update_info (png_ptr, info_ptr);

  if (CreateTarget (pParams))
    {
      png_bytep row_pointers[height];
      unsigned int y = 0;

      set_ascolor_depth (pParams->m_w, pParams->m_pImage->bits_per_pixel);

      if (color_type & PNG_COLOR_MASK_ALPHA)
	CreateMask (pParams);

      row_bytes = png_get_rowbytes (png_ptr, info_ptr);

      /* allocating big chunk of memory at once, to enable mmap
       * that will release memory to system right after free() */
      row_data = (png_bytep) safemalloc (row_bytes * height);
      for (offset = 0, row = 0; row < height; row++, offset += row_bytes)
	row_pointers[row] = row_data + offset;

      /* The easiest way to read the image: */
      LOG1 ("\n Reading Image UP ")
	png_read_image (png_ptr, row_pointers);

      LOG1 ("\n Storing Image")
	if (height < pParams->m_height || pParams->m_y_net == NULL)
	for (row = 0; y < pParams->m_height; y++)
	  {
	    if (pParams->m_y_net && y)
	      if (pParams->m_y_net[y - 1] == pParams->m_y_net[y])
		{
		  CopyXImageLine (pParams->m_pImage, pParams->m_pImage, y - 1, y);
		  if (pParams->m_pMaskImage && (color_type & PNG_COLOR_MASK_ALPHA))
		    CopyXImageLine (pParams->m_pMaskImage, pParams->m_pMaskImage, y - 1, y);
		  continue;
		}

	    put_png_scanline (y, row_pointers[row], pParams, color_type, row_bytes);
	    if (row < height - 1)
	      row++;
	  }
      else
	for (y = 0; y < pParams->m_height; y++)
	  put_png_scanline (y, row_pointers[pParams->m_y_net[y]], pParams, color_type, row_bytes);

      free (row_data);

      XImageToPixmap (pParams, pParams->m_pImage, &(pParams->m_Target));
      LOG1 ("\n Image stored")
	if (pParams->m_Target && (color_type & PNG_COLOR_MASK_ALPHA))
	{
	  XImageToPixmap (pParams, pParams->m_pMaskImage, &(pParams->m_Mask));
	  LOG1 ("\n Mask stored")
	}

    }				/*CreateXImage( pParams ) */

  /* read rest of file, and get additional chunks in info_ptr - REQUIRED */
  png_read_end (png_ptr, info_ptr);

  /* clean up after the read, and free any memory allocated - REQUIRED */
  png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp) NULL);
  if (info_ptr)
    free (info_ptr);

  /* close the file */
  fclose (fp);
}

#endif /*PNG */
