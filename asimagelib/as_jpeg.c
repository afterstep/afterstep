/* This file contains code for unified image loading from JPEG file. */
/********************************************************************/
/* Copyright (c) 1998 Sasha Vasko   <sashav@sprintmail.com>         */
/* Copyright (c) 1998 Ethan Fischer <allanon@u.washington.edu>      */
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
/*#define DO_CLOCKING  */
/*#define DEBUG_AS_JPG */
/*#define GETPIXEL_PUTPIXEL */

#define LIBASIMAGE

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

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/style.h"
#include "../include/screen.h"
#include "../include/ascolor.h"

/* our input-output data structures definitions */
#include "../include/loadimg.h"
#include "../include/XImage_utils.h"

#ifdef JPEG

/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */

#include <jpeglib.h>

/*
 * <setjmp.h> is used for the optional error recovery mechanism shown in
 * the second part of the example.
 */

#include <setjmp.h>




#ifndef min
#define min(x,y) ((x<y)?x:y)
#endif

#define RETURN_GAMMA 2.2	/* Request JPEG load with gamma of 2.2 */


#ifdef DEBUG_AS_JPG
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

void
put_scanline (int y,		/* y -coord   */
	      JSAMPROW row,	/* JSAMPLE's array */
	      LImageParams * pParams)	/* Load parameters from caller */
{
  ASCOLOR color;
  JSAMPROW src_row = row;
  register int x;
  DECLARE_XIMAGE_CURSOR (pParams->m_pImage, dst8, y);

  /* no checks for input parameters - all should be done before */
  for (x = 0; x < pParams->m_width; x++)
    {
      if (pParams->m_x_net)
	{
	  if (pParams->m_img_colormap != NULL)
	    row = src_row + pParams->m_x_net[x] * 3;
	  else
	    row = src_row + pParams->m_x_net[x];
	}
      /* handle color or grayscale jpegs */
      if (pParams->m_img_colormap)
	color = pParams->m_img_colormap[*row++];
      else
	{
	  if (pParams->m_gamma_table)
	    color = MAKE_ASCOLOR_RGB8 (pParams->m_gamma_table[row[0]],
				       pParams->m_gamma_table[row[1]],
				       pParams->m_gamma_table[row[2]]);
	  else
	    color = MAKE_ASCOLOR_RGB8 (row[0], row[1], row[2]);
	  row += 3;
	}
      PUT_ASCOLOR (pParams->m_pImage, color, dst8, x, y);
    }
}

/* following is the JPEG library stuff */

/* we'll
 * assume that we do not intend to put the whole image into an in-memory
 * buffer, but to send it line-by-line someplace else.  We need a one-
 * scanline-high JSAMPLE array as a work buffer, and we will let the JPEG
 * memory manager allocate it for us.  This approach is actually quite useful
 * because we don't need to remember to deallocate the buffer separately: it
 * will go away automatically when the JPEG object is cleaned up.
 */

/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * Our example here shows how to override the "error_exit" method so that
 * control is returned to the library's caller when a fatal error occurs,
 * rather than calling exit() as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */

struct my_error_mgr
{
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr *my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF (void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp (myerr->setjmp_buffer, 1);
}

void
build_jpeg_cmap (LImageParams * pParams,
		 struct jpeg_decompress_struct *p_cinfo)
{
  if (p_cinfo->output_components != 3)
    {
      register int i;
      pParams->m_img_colormap = safemalloc (256 * sizeof (ASCOLOR));
      if (p_cinfo->out_color_space == JCS_RGB)
	{
	  JSAMPARRAY colormap = p_cinfo->colormap;

	  if (pParams->m_gamma == 1.0)
	    for (i = 0; i < 256; i++)
	      pParams->m_img_colormap[i] =
		MAKE_ASCOLOR_RGB8 (GETJSAMPLE (colormap[0][i]),
				   GETJSAMPLE (colormap[1][i]),
				   GETJSAMPLE (colormap[2][i]));
	  else
	    for (i = 0; i < 256; i++)
	      pParams->m_img_colormap[i] =
		MAKE_ASCOLOR_RGB8 (pParams->m_gamma_table[GETJSAMPLE (colormap[0][i])],
			pParams->m_gamma_table[GETJSAMPLE (colormap[1][i])],
		       pParams->m_gamma_table[GETJSAMPLE (colormap[2][i])]);
	}
      else
	{
	  if (pParams->m_gamma == 1.0)
	    for (i = 0; i < 256; i++)
	      pParams->m_img_colormap[i] = MAKE_ASCOLOR_GRAY8 (i);
	  else
	    for (i = 0; i < 256; i++)
	      pParams->m_img_colormap[i] =
		MAKE_ASCOLOR_GRAY8 (pParams->m_gamma_table[i]);
	}
    }
}

/*
 * routine for JPEG decompression.  We assume that the source file name
 * is passed in.  We want to return 1 on success, 0 on error.
 */

#endif

int
LoadJPEGFile (LImageParams * pParams)
{

#ifdef JPEG
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct my_error_mgr jerr;
  /* More stuff */
  FILE *infile;			/* source file */
  JSAMPARRAY buffer;		/* Output row buffer */
#ifdef DO_CLOCKING
  clock_t started = clock ();
#endif

  /* we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  if (pParams == NULL)
    return 0;

  if ((infile = fopen (pParams->m_realfilename, "rb")) == NULL)
    {
      fprintf (stderr, "can't open %s\n", pParams->m_realfilename);
      return 0;
    }

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
    {
      /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object, close the input file, and return.
       */
      jpeg_destroy_decompress (&cinfo);
      fclose (infile);
      return 0;
    }
  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress (&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src (&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header (&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* Adjust default decompression parameters */
  cinfo.quantize_colors = FALSE;
  cinfo.output_gamma = RETURN_GAMMA;
  if (pParams->m_max_colors > 1 && pParams->m_max_colors < 256)
    {
      /* libjpeg-6a cannot handle less than 8 colors */
      if (pParams->m_max_colors < 8)
	pParams->m_max_colors = 8;
      cinfo.quantize_colors = TRUE;
      cinfo.desired_number_of_colors = pParams->m_max_colors;
    }
  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress (&cinfo);

  LOG3 ("\nJPEG LoadJPEGFile: stored image size %dx%d", cinfo.output_width, cinfo.output_height)

    CheckImageSize (pParams, cinfo.output_width, cinfo.output_height);

  LOG3 ("\nJPEG LoadJPEGFile: x_net =%x; y_net =%x. ", pParams->m_x_net, pParams->m_y_net);
  LOG3 ("\nJPEG LoadJPEGFile: Creating XImage %dx%d", pParams->m_width, pParams->m_height)
    if (CreateTarget (pParams))
    {
      int y = 0;

      set_ascolor_depth (pParams->m_w, pParams->m_pImage->bits_per_pixel);

      /* Make a one-row-high sample array that will go away when done with image */
      LOG2 ("\nJPEG LoadJPEGFile: Creating buffer of %lu bytes... ", cinfo.output_width * cinfo.output_components)
	buffer = (*cinfo.mem->alloc_sarray)
	((j_common_ptr) & cinfo, JPOOL_IMAGE, cinfo.output_width * cinfo.output_components, 1);

      /* Step 6: while (scan lines remain to be read) */
      /*            jpeg_read_scanlines(...); */
      LOG1 ("Done.")
#ifdef DO_CLOCKING
	printf ("\n loading initialization time (clocks): %lu\n", clock () - started);
#endif
      build_jpeg_cmap (pParams, &cinfo);
      while (cinfo.output_scanline < cinfo.output_height &&
	     y < pParams->m_height)
	{
	  /* jpeg_read_scanlines expects an array of pointers to scanlines.
	   * Here the array is only one element long, but you could ask for
	   * more than one scanline at a time if that's more convenient.
	   */
	  if (pParams->m_y_net && y)
	    if (pParams->m_y_net[y - 1] == pParams->m_y_net[y])
	      {
		CopyXImageLine (pParams->m_pImage, pParams->m_pImage, y - 1, y);
		y++;
		continue;
	      }

	  (void) jpeg_read_scanlines (&cinfo, buffer, 1);
	  /* Assume put_scanline_someplace wants a pointer and sample count. */
	  if (pParams->m_y_net)
	    if (pParams->m_y_net[y] != cinfo.output_scanline - 1)
	      continue;

	  put_scanline (y, buffer[0], pParams);
	  y++;
	}
#ifdef DO_CLOCKING
      printf ("\n read time (clocks): %lu\n", clock () - started);
#endif

      LOG3 ("\nJPEG LoadJPEGFile: Creating pixmap %dx%d", pParams->m_width, pParams->m_height)
	XImageToPixmap (pParams, pParams->m_pImage, &(pParams->m_Target));
    }				/* if( CreateTarget()) */

  /* Step 7: Finish decompression */

  /* we must abort the decompress if not all lines were read */
  if (cinfo.output_scanline < cinfo.output_height)
    jpeg_abort_decompress (&cinfo);
  else
    (void) jpeg_finish_decompress (&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress (&cinfo);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  fclose (infile);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

#ifdef DO_CLOCKING
  printf ("\n image loading time (clocks): %lu\n", clock () - started);
#endif

#endif
  /* And we're done! */
  return 1;
}

/*
 * SOME FINE POINTS:
 *
 * In the above code, we ignored the return value of jpeg_read_scanlines,
 * which is the number of scanlines actually read.  We could get away with
 * this because we asked for only one line at a time and we weren't using
 * a suspending data source.  See libjpeg.doc for more info.
 *
 * We cheated a bit by calling alloc_sarray() after jpeg_start_decompress();
 * we should have done it beforehand to ensure that the space would be
 * counted against the JPEG max_memory setting.  In some systems the above
 * code would risk an out-of-memory error.  However, in general we don't
 * know the output image dimensions before jpeg_start_decompress(), unless we
 * call jpeg_calc_output_dimensions().  See libjpeg.doc for more about this.
 *
 * Scanlines are returned in the same order as they appear in the JPEG file,
 * which is standardly top-to-bottom.  If you must emit data bottom-to-top,
 * you can use one of the virtual arrays provided by the JPEG memory manager
 * to invert the data.  See wrbmp.c for an example.
 *
 * As with compression, some operating modes may require temporary files.
 * On some systems you may need to set up a signal handler to ensure that
 * temporary files are deleted if the program is interrupted.  See libjpeg.doc.
 */
