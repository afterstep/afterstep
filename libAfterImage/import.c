/* This file contains code for unified image loading from many file formats */
/********************************************************************/
/* Copyright (c) 2001,2004 Sasha Vasko <sasha at aftercode.net>     */
/* Copyright (c) 2004 Maxim Nikulin <nikulin at gorodok.net>        */
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
/*#undef NO_DEBUG_OUTPUT*/
#undef LOCAL_DEBUG
#undef DO_CLOCKING
#undef DEBUG_TRANSP_GIF

#ifdef _WIN32
#include "win32/config.h"
#else
#include "config.h"
#endif

#ifdef HAVE_PNG
/* Include file for users of png library. */
# ifdef HAVE_BUILTIN_PNG
#  include "libpng/png.h"
# else
#  include <png.h>
# endif
#else
#include <setjmp.h>
#endif
#ifdef HAVE_JPEG
/* Include file for users of png library. */
# undef HAVE_STDLIB_H
# ifndef X_DISPLAY_MISSING
#  include <X11/Xmd.h>
# endif
# ifdef HAVE_BUILTIN_JPEG
#  include "libjpeg/jpeglib.h"
# else
#  include <jpeglib.h>
# endif
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#include <string.h>
#include <ctype.h>
/* <setjmp.h> is used for the optional error recovery mechanism */

#ifdef const
#undef const
#endif
#ifdef _WIN32
# include "win32/afterbase.h"
#else
# include "afterbase.h"
#endif
#ifdef HAVE_GIF
# ifdef HAVE_BUILTIN_UNGIF
#  include "libungif/gif_lib.h"
# else
#  include <gif_lib.h>
# endif
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
#include "imencdec.h"
#include "bmp.h"
#include "ximage.h"
#include "xcf.h"
#include "xpm.h"
#include "ungif.h"
#include "import.h"
#include "asimagexml.h"
#include "transform.h"

#ifdef jmpbuf
#undef jmpbuf
#endif


/***********************************************************************************/
/* High level interface : 														   */
static char *locate_image_file( const char *file, char **paths );
static ASImageFileTypes	check_image_type( const char *realfilename );

as_image_loader_func as_image_file_loaders[ASIT_Unknown] =
{
	xpm2ASImage ,
	xpm2ASImage ,
	xpm2ASImage ,
	png2ASImage ,
	jpeg2ASImage,
	xcf2ASImage ,
	ppm2ASImage ,
	ppm2ASImage ,
	bmp2ASImage ,
	ico2ASImage ,
	ico2ASImage ,
	gif2ASImage ,
	tiff2ASImage,
	xml2ASImage ,
	NULL,
	NULL,
	NULL
};
 
ASImage *
file2ASImage_extra( const char *file, ASImageImportParams *iparams )
{
	int 		  filename_len ;
	int 		  subimage = -1 ;
	char 		 *realfilename = NULL, *tmp = NULL ;
	register int i;

	ASImage *im = NULL;
	char *g_var ;
	ASImageImportParams dummy_iparams = {0};

	if( iparams == NULL )
		iparams = &dummy_iparams ;

	if( (g_var = getenv( "SCREEN_GAMMA" )) != NULL )
		iparams->gamma = atof(g_var);

	if( file )
	{
		filename_len = strlen(file);
#ifdef _WIN32
		{		
			int i = 0 ; 
			while( iparams->search_path[i] != NULL ) 
				unix_path2dos_path( iparams->search_path[i] );
		}
#endif

		/* first lets try to find file as it is */
		if( (realfilename = locate_image_file(file, iparams->search_path)) == NULL )
		{
			tmp = safemalloc( filename_len+3+1);
			strcpy(tmp, file);
		}
		if( realfilename == NULL )
		{ /* let's try and see if appending .gz will make any difference */
			strcpy(&(tmp[filename_len]), ".gz");
			realfilename = locate_image_file(tmp,iparams->search_path);
		}
		if( realfilename == NULL )
		{ /* let's try and see if appending .Z will make any difference */
			strcpy(&(tmp[filename_len]), ".Z");
			realfilename = locate_image_file(tmp,iparams->search_path);
		}
		if( realfilename == NULL )
		{ /* let's try and see if we have subimage number appended */
			for( i = filename_len-1 ; i > 0; i-- )
				if( !isdigit( (int)tmp[i] ) )
					break;
			if( i < filename_len-1 && i > 0 )
				if( tmp[i] == '.' )                 /* we have possible subimage number */
				{
					subimage = atoi( &tmp[i+1] );
					tmp[i] = '\0';
					filename_len = i ;
					realfilename = locate_image_file(tmp,iparams->search_path);
					if( realfilename == NULL )
					{ /* let's try and see if appending .gz will make any difference */
						strcpy(&(tmp[filename_len]), ".gz");
						realfilename = locate_image_file(tmp,iparams->search_path);
					}
					if( realfilename == NULL )
					{ /* let's try and see if appending .Z will make any difference */
						strcpy(&(tmp[filename_len]), ".Z");
						realfilename = locate_image_file(tmp,iparams->search_path);
					}
				}
		}
		if( tmp != realfilename && tmp != NULL )
			free( tmp );
	}
	if( realfilename )
	{
		ASImageFileTypes file_type = check_image_type( realfilename );
		if( file_type == ASIT_Unknown )
			show_error( "Hmm, I don't seem to know anything about format of the image file \"%s\"\n.\tPlease check the manual", realfilename );
		else if( as_image_file_loaders[file_type] )
			im = as_image_file_loaders[file_type](realfilename, iparams);
		else
			show_error( "Support for the format of image file \"%s\" has not been implemented yet.", realfilename );
#ifndef NO_DEBUG_OUTPUT
		show_progress( "image loaded from \"%s\"", realfilename );
#endif
		if( realfilename != file )
			free( realfilename );
	}else
		show_error( "I'm terribly sorry, but image file \"%s\" is nowhere to be found.", file );

	return im;
}

ASImage *
file2ASImage( const char *file, ASFlagType what, double gamma, unsigned int compression, ... )
{
	int i ;
	char 		 *paths[MAX_SEARCH_PATHS+1] ;
	ASImageImportParams iparams ;
	va_list       ap;

	iparams.flags = 0 ;
	iparams.width = 0 ;
	iparams.height = 0 ;
	iparams.filter = SCL_DO_ALL ;
	iparams.gamma = gamma ;
	iparams.gamma_table = NULL ;
	iparams.compression = compression ;
	iparams.format = ASA_ASImage ;
	iparams.search_path = &(paths[0]);
	iparams.subimage = 0 ;

	va_start (ap, compression);
	for( i = 0 ; i < MAX_SEARCH_PATHS ; i++ )
	{	
		if( (paths[i] = va_arg(ap,char*)) == NULL )
			break;
	}		   
	paths[MAX_SEARCH_PATHS] = NULL ;
	va_end (ap);

	return file2ASImage_extra( file, &iparams );

}

Pixmap
file2pixmap(ASVisual *asv, Window root, const char *realfilename, Pixmap *mask_out)
{
	Pixmap trg = None;
#ifndef X_DISPLAY_MISSING
	Pixmap mask = None ;
	if( asv && realfilename )
	{
		double gamma = SCREEN_GAMMA;
		char  *gamma_str;
		ASImage *im = NULL;

		if ((gamma_str = getenv ("SCREEN_GAMMA")) != NULL)
		{
			gamma = atof (gamma_str);
			if (gamma == 0.0)
				gamma = SCREEN_GAMMA;
		}

		im = file2ASImage( realfilename, 0xFFFFFFFF, gamma, 0, NULL );

		if( im != NULL )
		{
			trg = asimage2pixmap( asv, root, im, NULL, False );
			if( mask_out )
				if( get_flags( get_asimage_chanmask(im), SCL_DO_ALPHA ) )
					mask = asimage2mask( asv, root, im, NULL, False );
			destroy_asimage( &im );
		}
	}
	if( mask_out )
	{
		if( *mask_out )
			XFreePixmap( asv->dpy, *mask_out );
		*mask_out = mask ;
	}
#endif
	return trg ;
}

ASImage *
get_asimage( ASImageManager* imageman, const char *file, ASFlagType what, unsigned int compression )
{
	ASImage *im = NULL ;
	if( imageman && file )
		if( (im = fetch_asimage(imageman, file )) == NULL )
		{
			char **path = &(imageman->search_path[0]);
			if( path[2] == NULL )
				im = file2ASImage( file, what, imageman->gamma, compression, path[0], path[1], NULL );
#if (MAX_SEARCH_PATHS!=8)
#error "Fixme: please update file2ASImage call to match max number of search paths in ImageManager"
#else
			else
				im = file2ASImage( file, what, imageman->gamma, compression, path[0], path[1], path[2],
			    	               path[3], path[4], path[5], path[6], path[7], NULL );
#endif
			if( im )
				store_asimage( imageman, im, file );
		}
	return im;
}

ASImageListEntry *
get_asimage_list( ASVisual *asv, const char *dir,
	              ASFlagType preview_type, double gamma,
				  unsigned int preview_width, unsigned int preview_height,
				  unsigned int preview_compression,
				  unsigned int *count_ret,
				  int (*select) (const char *) )
{
	ASImageListEntry *im_list = NULL ;
#ifndef _WIN32
	ASImageListEntry **curr = &im_list, *last = NULL ;
	struct direntry  **list;
	int n, i, count = 0;
	int dir_name_len ;


	if( asv == NULL || dir == NULL )
		return NULL ;

	dir_name_len = strlen(dir);
	n = my_scandir ((char*)dir, &list, select, NULL);
	if( n > 0 )
	{
		for (i = 0; i < n; i++)
		{
			if (!S_ISDIR (list[i]->d_mode))
			{
				ASImageFileTypes file_type ;
				char *realfilename ;

				realfilename = safemalloc( dir_name_len + 1 + strlen(list[i]->d_name)+1);
				sprintf( realfilename, "%s/%s", dir, list[i]->d_name );
				file_type = check_image_type( realfilename );
				if( file_type != ASIT_Unknown && as_image_file_loaders[file_type] == NULL )
					file_type = ASIT_Unknown ;

				++count ;
				*curr = safecalloc( 1, sizeof(ASImageListEntry) );
				if( last )
					last->next = *curr ;
				last = *curr ;
				curr = &(last->next);

				last->name = mystrdup( list[i]->d_name );
				last->fullfilename = realfilename ;
				last->type = file_type ;

				if( last->type != ASIT_Unknown && preview_type != 0 )
				{
					ASImageImportParams iparams = {0} ;
					ASImage *im = as_image_file_loaders[file_type](realfilename, &iparams);
					if( im )
					{
						int scale_width = im->width ;
						int scale_height = im->height ;
						int tile_width = im->width ;
						int tile_height = im->height ;

						if( preview_width > 0 )
						{
							if( get_flags( preview_type, SCALE_PREVIEW_H ) )
								scale_width = preview_width ;
							else
								tile_width = preview_width ;
						}
						if( preview_height > 0 )
						{
							if( get_flags( preview_type, SCALE_PREVIEW_V ) )
								scale_height = preview_height ;
							else
								tile_height = preview_height ;
						}
						if( scale_width != im->width || scale_height != im->height )
						{
							ASImage *tmp = scale_asimage( asv, im, scale_width, scale_height, ASA_ASImage, preview_compression, ASIMAGE_QUALITY_DEFAULT );
							if( tmp != NULL )
							{
								destroy_asimage( &im );
								im = tmp ;
							}
						}
						if( tile_width != im->width || tile_height != im->height )
						{
							ASImage *tmp = tile_asimage( asv, im, 0, 0, tile_width, tile_height, TINT_NONE, ASA_ASImage, preview_compression, ASIMAGE_QUALITY_DEFAULT );
							if( tmp != NULL )
							{
								destroy_asimage( &im );
								im = tmp ;
							}
						}
					}

			  		last->preview = im ;
				}
			}
			free( list[i] );
		}
		free (list);
	}

	if( count_ret )
		*count_ret = count ;
#endif
	return im_list;
}


/***********************************************************************************/
/* Some helper functions :                                                         */

static char *
locate_image_file( const char *file, char **paths )
{
	char *realfilename = NULL;
	if( file != NULL )
	{
		realfilename = mystrdup( file );
#ifdef _WIN32
		unix_path2dos_path( realfilename );
#endif
		
		if( CheckFile( realfilename ) != 0 )
		{
			free( realfilename ) ;
			realfilename = NULL ;
			if( paths != NULL )
			{	/* now lets try and find the file in any of the optional paths :*/
				register int i = 0;
				do
				{
					realfilename = find_file( file, paths[i], R_OK );
				}while( realfilename == NULL && paths[i++] != NULL );
			}
		}
	}
	return realfilename;
}

static FILE*
open_image_file( const char *path )
{
	FILE *fp = NULL;
	if ( path )
	{
		if ((fp = fopen (path, "rb")) == NULL)
			show_error("cannot open image file \"%s\" for reading. Please check permissions.", path);
	}else
		fp = stdin ;
	return fp ;
}

static ASImageFileTypes
check_image_type( const char *realfilename )
{
	int filename_len = strlen( realfilename );
#define FILE_HEADER_SIZE	512
	CARD8 head[FILE_HEADER_SIZE+1] ;
	int bytes_in = 0 ;
	FILE *fp ;
	ASImageFileTypes type = ASIT_Unknown ;
	/* lets check if we have compressed xpm file : */
	if( filename_len > 7 && mystrncasecmp( realfilename+filename_len-7, ".xpm.gz", 7 ) == 0 )
		type = ASIT_GZCompressedXpm;
	else if( filename_len > 6 && mystrncasecmp( realfilename+filename_len-6, ".xpm.Z", 6 ) == 0 )
		type = ASIT_ZCompressedXpm ;
	else if( (fp = open_image_file( realfilename )) != NULL )
	{
		bytes_in = fread( &(head[0]), sizeof(char), FILE_HEADER_SIZE, fp );
		head[FILE_HEADER_SIZE] = '\0' ;
		DEBUG_OUT("%s: head[0]=0x%2.2X(%d),head[2]=0x%2.2X(%d)\n", realfilename+filename_len-4, head[0], head[0], head[2], head[2] );
/*		fprintf( stderr, " IMAGE FILE HEADER READS : [%s][%c%c%c%c%c%c%c%c][%s], bytes_in = %d\n", (char*)&(head[0]),
						head[0], head[1], head[2], head[3], head[4], head[5], head[6], head[7], strstr ((char *)&(head[0]), "XPM"),bytes_in );
 */
		if( bytes_in > 3 )
		{
			if( head[0] == 0xff && head[1] == 0xd8 && head[2] == 0xff)
				type = ASIT_Jpeg;
			else if (strstr ((char *)&(head[0]), "XPM") != NULL)
				type =  ASIT_Xpm;
			else if (head[1] == 'P' && head[2] == 'N' && head[3] == 'G')
				type = ASIT_Png;
			else if (head[0] == 'G' && head[1] == 'I' && head[2] == 'F')
				type = ASIT_Gif;
			else if (head[0] == head[1] && (head[0] == 'I' || head[0] == 'M'))
				type = ASIT_Tiff;
			else if (head[0] == 'P' && isdigit(head[1]))
				type = (head[1]!='5' && head[1]!='6')?ASIT_Pnm:ASIT_Ppm;
			else if (head[0] == 0xa && head[1] <= 5 && head[2] == 1)
				type = ASIT_Pcx;
			else if (head[0] == 'B' && head[1] == 'M')
				type = ASIT_Bmp;
			else if (head[0] == 0 && head[2] == 1 && mystrncasecmp(realfilename+filename_len-4, ".ICO", 4)==0 )
				type = ASIT_Ico;
			else if (head[0] == 0 && head[2] == 2 &&
						(mystrncasecmp(realfilename+filename_len-4, ".CUR", 4)==0 ||
						 mystrncasecmp(realfilename+filename_len-4, ".ICO", 4)==0) )
				type = ASIT_Cur;
		}
		if( type == ASIT_Unknown && bytes_in  > 8 )
		{
			if( strncmp((char *)&(head[0]), XCF_SIGNATURE, (size_t) XCF_SIGNATURE_LEN) == 0)
				type = ASIT_Xcf;
	   		else if (head[0] == 0 && head[1] == 0 &&
			    	 head[2] == 2 && head[3] == 0 && head[4] == 0 && head[5] == 0 && head[6] == 0 && head[7] == 0)
				type = ASIT_Targa;
			else if (strncmp ((char *)&(head[0]), "#define", (size_t) 7) == 0)
				type = ASIT_Xbm;
			else
			{/* the nastiest check - for XML files : */
				register int i ;

				type = ASIT_XMLScript ;
				while( bytes_in > 0 && type == ASIT_XMLScript )
				{
					for( i = 0 ; i < bytes_in ; ++i ) if( !isspace(head[i]) ) break;
					if( i >= bytes_in )
						bytes_in = fread( &(head[0]), sizeof(CARD8), FILE_HEADER_SIZE, fp );
					else if( head[i] != '<' )
						type = ASIT_Unknown ;
					else
						break ;
				}
				while( bytes_in > 0 && type == ASIT_XMLScript )
				{
					for( i = 0 ; i < bytes_in ; ++i )
						if( !isspace(head[i]) )
						{
							if( !isprint(head[i]) )
							{
								type = ASIT_Unknown ;
								break ;
							}else if( head[i] == '>' )
								break ;
						}

					if( i >= bytes_in )
						bytes_in = fread( &(head[0]), sizeof(CARD8), FILE_HEADER_SIZE, fp );
					else
						break ;
				}
			}
		}
		fclose( fp );
	}
	return type;
}


/***********************************************************************************/
#ifdef HAVE_XPM      /* XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM */

#ifdef LOCAL_DEBUG
Bool print_component( CARD32*, int, unsigned int );
#endif

static ASImage *
xpm_file2ASImage( ASXpmFile *xpm_file, unsigned int compression )
{
	ASImage *im = NULL ;
	int line = 0;

	LOCAL_DEBUG_OUT( "do_alpha is %d. im->height = %d, im->width = %d", xpm_file->do_alpha, xpm_file->height, xpm_file->width );
	if( build_xpm_colormap( xpm_file ) )
		if( (im = create_xpm_image( xpm_file, compression )) != NULL )
		{
			int bytes_count = im->width*4 ;
			ASFlagType rgb_flags = ASStorage_RLEDiffCompress|ASStorage_32Bit ;
			ASFlagType alpha_flags = ASStorage_RLEDiffCompress|ASStorage_32Bit ;
			if( !xpm_file->full_alpha ) 
				alpha_flags |= ASStorage_Bitmap ;
			for( line = 0 ; line < xpm_file->height ; ++line )
			{
				if( !convert_xpm_scanline( xpm_file, line ) )
					break;
				im->channels[IC_RED][line]   = store_data( NULL, (CARD8*)xpm_file->scl.red, bytes_count, rgb_flags, 0);
				im->channels[IC_GREEN][line] = store_data( NULL, (CARD8*)xpm_file->scl.green, bytes_count, rgb_flags, 0);
				im->channels[IC_BLUE][line]  = store_data( NULL, (CARD8*)xpm_file->scl.blue, bytes_count, rgb_flags, 0);
				if( xpm_file->do_alpha )
					im->channels[IC_ALPHA][line]  = store_data( NULL, (CARD8*)xpm_file->scl.alpha, bytes_count, alpha_flags, 0);
#ifdef LOCAL_DEBUG
				printf( "%d: \"%s\"\n",  line, xpm_file->str_buf );
				print_component( xpm_file->scl.red, 0, xpm_file->width );
				print_component( xpm_file->scl.green, 0, xpm_file->width );
				print_component( xpm_file->scl.blue, 0, xpm_file->width );
#endif
			}
		}
	return im ;
}

ASImage *
xpm2ASImage( const char * path, ASImageImportParams *params )
{
	ASXpmFile *xpm_file = NULL;
	ASImage *im = NULL ;
	START_TIME(started);

	LOCAL_DEBUG_CALLER_OUT ("(\"%s\", 0x%lX)", path, params->flags);
	if( (xpm_file=open_xpm_file(path)) == NULL )
	{
		show_error("cannot open image file \"%s\" for reading. Please check permissions.", path);
		return NULL;
	}

	im = xpm_file2ASImage( xpm_file, params->compression );
	close_xpm_file( &xpm_file );

	SHOW_TIME("image loading",started);
	return im;
}

ASXpmFile *open_xpm_data(const char **data);

ASImage *
xpm_data2ASImage( const char **data, ASImageImportParams *params )
{
	ASXpmFile *xpm_file = NULL;
	ASImage *im = NULL ;
	START_TIME(started);

    LOCAL_DEBUG_CALLER_OUT ("(\"%s\", 0x%lX)", (char*)data, params->flags);
	if( (xpm_file=open_xpm_data(data)) == NULL )
	{
		show_error("cannot read XPM data.");
		return NULL;
	}

	im = xpm_file2ASImage( xpm_file, params->compression );
	close_xpm_file( &xpm_file );

	SHOW_TIME("image loading",started);
	return im;
}

#else  			/* XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM */

ASImage *
xpm2ASImage( const char * path, ASImageImportParams *params )
{
	show_error( "unable to load file \"%s\" - XPM image format is not supported.\n", path );
	return NULL ;
}

#endif 			/* XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM XPM */
/***********************************************************************************/

static inline void
apply_gamma( register CARD8* raw, register CARD8 *gamma_table, unsigned int width )
{
	if( gamma_table )
	{	
		register unsigned int i ;
		for( i = 0 ; i < width ; ++i )
			raw[i] = gamma_table[raw[i]] ;
	}
}

/***********************************************************************************/
#ifdef HAVE_PNG		/* PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG */
ASImage *
png2ASImage( const char * path, ASImageImportParams *params )
{
	FILE 		 *fp ;
    double        image_gamma = DEFAULT_PNG_IMAGE_GAMMA;
	png_structp   png_ptr;
	png_infop     info_ptr;
	png_uint_32   width, height;
	int           bit_depth, color_type, interlace_type;
	int           intent;
	ASScanline    buf;
	Bool 		  do_alpha = False, grayscale = False ;
	png_bytep     *row_pointers, row;
	unsigned int  y;
	size_t		  row_bytes, offset ;
	static ASImage 	 *im = NULL ;
	START_TIME(started);

	im = NULL ;
	if ((fp = open_image_file(path)) == NULL)
		return NULL;
	/* Create and initialize the png_struct with the desired error handler
	 * functions.  If you want to use the default stderr and longjump method,
	 * you can supply NULL for the last three parameters.  We also supply the
	 * the compiler header file version, so that we know if the application
	 * was compiled with a compatible version of the library.  REQUIRED
	 */
	if((png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) != NULL )
	{
		/* Allocate/initialize the memory for image information.  REQUIRED. */
		if( (info_ptr = png_create_info_struct (png_ptr)) != NULL )
		{
		  	/* Set error handling if you are using the setjmp/longjmp method (this is
			 * the normal method of doing things with libpng).  REQUIRED unless you
			 * set up your own error handlers in the png_create_read_struct() earlier.
			 */
			if ( !setjmp (png_ptr->jmpbuf))
			{
				ASFlagType rgb_flags = ASStorage_RLEDiffCompress|ASStorage_32Bit ;
				
				png_init_io (png_ptr, fp);
		    	png_read_info (png_ptr, info_ptr);
				png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

				if (bit_depth < 8)
				{/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
				  * byte into separate bytes (useful for paletted and grayscale images).
				  */
					if( bit_depth == 1 ) 
						set_flags( rgb_flags, ASStorage_Bitmap );
					png_set_packing (png_ptr);
				}else if (bit_depth == 16)
				{/* tell libpng to strip 16 bit/color files down to 8 bits/color */
					png_set_strip_16 (png_ptr);
				}
				bit_depth = 8;

				/* Expand paletted colors into true RGB triplets */
				if (color_type == PNG_COLOR_TYPE_PALETTE)
				{
					png_set_expand (png_ptr);
					color_type = PNG_COLOR_TYPE_RGB;
				}

				/* Expand paletted or RGB images with transparency to full alpha channels
				 * so the data will be available as RGBA quartets.
		 		 */
   				if( color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY )
   				{
				   	if( png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
					{
						png_set_expand(png_ptr);
						color_type |= PNG_COLOR_MASK_ALPHA;
					}
   				}else
				{
					png_set_filler( png_ptr, 0xFF, PNG_FILLER_AFTER );
					color_type |= PNG_COLOR_MASK_ALPHA;
				}

/*				if( color_type == PNG_COLOR_TYPE_RGB )
					color_type = PNG_COLOR_TYPE_RGB_ALPHA ;
   				else
					color_type = PNG_COLOR_TYPE_GRAY_ALPHA ;
  */
				if (png_get_sRGB (png_ptr, info_ptr, &intent))
                    png_set_gamma (png_ptr, params->gamma, DEFAULT_PNG_IMAGE_GAMMA);
				else if (png_get_gAMA (png_ptr, info_ptr, &image_gamma))
					png_set_gamma (png_ptr, params->gamma, image_gamma);
				else
                    png_set_gamma (png_ptr, params->gamma, DEFAULT_PNG_IMAGE_GAMMA);

				/* Optional call to gamma correct and add the background to the palette
				 * and update info structure.  REQUIRED if you are expecting libpng to
				 * update the palette for you (ie you selected such a transform above).
				 */
				png_read_update_info (png_ptr, info_ptr);

				im = create_asimage( width, height, params->compression );
				do_alpha = ((color_type & PNG_COLOR_MASK_ALPHA) != 0 );
				grayscale = ( color_type == PNG_COLOR_TYPE_GRAY_ALPHA ||
				              color_type == PNG_COLOR_TYPE_GRAY) ;

				if( !do_alpha && grayscale ) 
					clear_flags( rgb_flags, ASStorage_32Bit );
				else
					prepare_scanline( im->width, 0, &buf, False );

				row_bytes = png_get_rowbytes (png_ptr, info_ptr);
				/* allocating big chunk of memory at once, to enable mmap
				 * that will release memory to system right after free() */
				row_pointers = safemalloc( height * sizeof( png_bytep ) + row_bytes * height );
				row = (png_bytep)(row_pointers + height) ;
				for (offset = 0, y = 0; y < height; y++, offset += row_bytes)
					row_pointers[y] = row + offset;

				/* The easiest way to read the image: */
				png_read_image (png_ptr, row_pointers);
				for (y = 0; y < height; y++)
				{
					if( do_alpha || !grayscale ) 
					{	
						raw2scanline( row_pointers[y], &buf, NULL, buf.width, grayscale, do_alpha );
						im->channels[IC_RED][y] = store_data( NULL, (CARD8*)buf.red, buf.width*4, rgb_flags, 0);
					}else
						im->channels[IC_RED][y] = store_data( NULL, row_pointers[y], buf.width, rgb_flags, 0);
					
					if( grayscale ) 
					{	
						im->channels[IC_GREEN][y] = dup_data( NULL, im->channels[IC_RED][y] );
						im->channels[IC_BLUE][y]  = dup_data( NULL, im->channels[IC_RED][y] );
					}else
					{
						im->channels[IC_GREEN][y] = store_data( NULL, (CARD8*)buf.green, buf.width*4, rgb_flags, 0);	
						im->channels[IC_BLUE][y] = store_data( NULL, (CARD8*)buf.blue, buf.width*4, rgb_flags, 0);
					}	 

					if( do_alpha )
					{
						int has_zero = False, has_nozero = False ;
						register unsigned int i;
						for ( i = 0 ; i < buf.width ; ++i)
						{
							if( buf.alpha[i] != 0x00FF )
							{	
								if( buf.alpha[i] == 0 )
									has_zero = True ;
								else
								{	
									has_nozero = True ;
									break;
								}
							}		
						}
						if( has_zero || has_nozero ) 
						{
							ASFlagType alpha_flags = ASStorage_32Bit|ASStorage_RLEDiffCompress ;
							if( !has_nozero ) 
								set_flags( alpha_flags, ASStorage_Bitmap );
							im->channels[IC_ALPHA][y] = store_data( NULL, (CARD8*)buf.alpha, buf.width*4, alpha_flags, 0);
						}
					}
				}
				free (row_pointers);
				if( do_alpha || !grayscale ) 
					free_scanline(&buf, True);
				/* read rest of file, and get additional chunks in info_ptr - REQUIRED */
				png_read_end (png_ptr, info_ptr);
		  	}
		}
		/* clean up after the read, and free any memory allocated - REQUIRED */
		png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp) NULL);
		if (info_ptr)
			free (info_ptr);
	}
	/* close the file */
	fclose (fp);
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
print_asimage( im, ASFLAGS_EVERYTHING, __FUNCTION__, __LINE__ );
#endif
	SHOW_TIME("image loading",started);
	return im ;
}
#else 			/* PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG */
ASImage *
png2ASImage( const char * path, ASImageImportParams *params )
{
	show_error( "unable to load file \"%s\" - PNG image format is not supported.\n", path );
	return NULL ;
}

#endif 			/* PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG PNG */
/***********************************************************************************/


/***********************************************************************************/
#ifdef HAVE_JPEG     /* JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG */
struct my_error_mgr
{
	struct jpeg_error_mgr pub;				   /* "public" fields */
	jmp_buf       setjmp_buffer;			   /* for return to caller */
};
typedef struct my_error_mgr *my_error_ptr;

METHODDEF (void)
my_error_exit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr  myerr = (my_error_ptr) cinfo->err;
	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);
	/* Return control to the setjmp point */
	longjmp (myerr->setjmp_buffer, 1);
}

ASImage *
jpeg2ASImage( const char * path, ASImageImportParams *params )
{
	ASImage *im ;
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
	FILE         *infile;					   /* source file */
	JSAMPARRAY    buffer;					   /* Output row buffer */
	ASScanline    buf;
	int y;
	START_TIME(started);
 /*	register int i ;*/

	/* we want to open the input file before doing anything else,
	 * so that the setjmp() error recovery below can assume the file is open.
	 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	 * requires it in order to read binary files.
	 */

	if ((infile = open_image_file(path)) == NULL)
		return NULL;

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
		return NULL;
	}
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress (&cinfo);
	/* Step 2: specify data source (eg, a file) */
	jpeg_stdio_src (&cinfo, infile);
	/* Step 3: read file parameters with jpeg_read_header() */
	(void)jpeg_read_header (&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */

	/* Step 4: set parameters for decompression */
	/* Adjust default decompression parameters */
	cinfo.quantize_colors = FALSE;		       /* we don't want no stinking colormaps ! */
	cinfo.output_gamma = params->gamma;
	/* Step 5: Start decompressor */
	(void)jpeg_start_decompress (&cinfo);
	LOCAL_DEBUG_OUT("stored image size %dx%d", cinfo.output_width,  cinfo.output_height);

	im = create_asimage( cinfo.output_width,  cinfo.output_height, params->compression );
	
	if( cinfo.output_components != 1 ) 
		prepare_scanline( im->width, 0, &buf, False );

	/* Make a one-row-high sample array that will go away when done with image */
	buffer =(*cinfo.mem->alloc_sarray)((j_common_ptr) & cinfo, JPOOL_IMAGE,
										cinfo.output_width * cinfo.output_components, 1);

	/* Step 6: while (scan lines remain to be read) */
	SHOW_TIME("loading initialization",started);
	y = -1 ;
	/*cinfo.output_scanline*/
/*	for( i = 0 ; i < im->width ; i++ )	fprintf( stderr, "%3.3d    ", i );
	fprintf( stderr, "\n");
 */
 	while ( ++y < (int)cinfo.output_height )
	{
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		(void)jpeg_read_scanlines (&cinfo, buffer, 1);
		if( cinfo.output_components==1 ) 
		{	
			apply_gamma( (CARD8*)buffer[0], params->gamma_table, im->width );
			im->channels[IC_RED][y] = store_data( NULL, (CARD8*)buffer[0], im->width, ASStorage_RLEDiffCompress, 0);
			im->channels[IC_GREEN][y] = dup_data( NULL, im->channels[IC_RED][y] );
			im->channels[IC_BLUE][y]  = dup_data( NULL, im->channels[IC_RED][y] );
		}else
		{		   
			raw2scanline( (CARD8*)buffer[0], &buf, params->gamma_table, im->width, (cinfo.output_components==1), False);
			im->channels[IC_RED][y] = store_data( NULL, (CARD8*)buf.red, buf.width*4, ASStorage_32BitRLE, 0);
			im->channels[IC_GREEN][y] = store_data( NULL, (CARD8*)buf.green, buf.width*4, ASStorage_32BitRLE, 0);
			im->channels[IC_BLUE][y] = store_data( NULL, (CARD8*)buf.blue, buf.width*4, ASStorage_32BitRLE, 0);
		}
/*		fprintf( stderr, "src:");
		for( i = 0 ; i < im->width ; i++ )
			fprintf( stderr, "%2.2X%2.2X%2.2X ", buffer[0][i*3], buffer[0][i*3+1], buffer[0][i*3+2] );
		fprintf( stderr, "\ndst:");
		for( i = 0 ; i < im->width ; i++ )
			fprintf( stderr, "%2.2X%2.2X%2.2X ", buf.red[i], buf.green[i], buf.blue[i] );
		fprintf( stderr, "\n");
 */
	}
	if( cinfo.output_components != 1 ) 
		free_scanline(&buf, True);
	SHOW_TIME("read",started);

	/* Step 7: Finish decompression */
	/* we must abort the decompress if not all lines were read */
	if (cinfo.output_scanline < cinfo.output_height)
		jpeg_abort_decompress (&cinfo);
	else
		(void)jpeg_finish_decompress (&cinfo);
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
	SHOW_TIME("image loading",started);
	LOCAL_DEBUG_OUT("done loading JPEG image \"%s\"", path);
	return im ;
}
#else 			/* JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG */
ASImage *
jpeg2ASImage( const char * path, ASImageImportParams *params )
{
	show_error( "unable to load file \"%s\" - JPEG image format is not supported.\n", path );
	return NULL ;
}

#endif 			/* JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG JPEG */
/***********************************************************************************/

/***********************************************************************************/
/* XCF - GIMP's native file format : 											   */

ASImage *
xcf2ASImage( const char * path, ASImageImportParams *params )
{
	ASImage *im = NULL ;
	/* More stuff */
	FILE         *infile;					   /* source file */
	XcfImage  *xcf_im;
	START_TIME(started);

	/* we want to open the input file before doing anything else,
	 * so that the setjmp() error recovery below can assume the file is open.
	 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	 * requires it in order to read binary files.
	 */
	if ((infile = open_image_file(path)) == NULL)
		return NULL;

	xcf_im = read_xcf_image( infile );
	fclose( infile );

	if( xcf_im == NULL )
		return NULL;

	LOCAL_DEBUG_OUT("stored image size %ldx%ld", xcf_im->width,  xcf_im->height);
#ifdef LOCAL_DEBUG
	print_xcf_image( xcf_im );
#endif
	{/* TODO : temporary workaround untill we implement layers merging */
		XcfLayer *layer = xcf_im->layers ;
		while ( layer )
		{
			if( layer->hierarchy )
				if( layer->hierarchy->image )
					if( layer->hierarchy->width == xcf_im->width &&
						layer->hierarchy->height == xcf_im->height )
					{
						im = layer->hierarchy->image ;
						layer->hierarchy->image = NULL ;
					}
			layer = layer->next ;
		}
	}
 	free_xcf_image(xcf_im);

	SHOW_TIME("image loading",started);
	return im ;
}

/***********************************************************************************/
/* PPM/PNM file format : 											   				   */
ASImage *
ppm2ASImage( const char * path, ASImageImportParams *params )
{
	ASImage *im = NULL ;
	/* More stuff */
	FILE         *infile;					   /* source file */
	ASScanline    buf;
	int y;
	unsigned int type = 0, width = 0, height = 0, colors = 0;
#define PPM_BUFFER_SIZE 71                     /* Sun says that no line should be longer then this */
	char buffer[PPM_BUFFER_SIZE];
	START_TIME(started);

	if ((infile = open_image_file(path)) == NULL)
		return NULL;

	if( fgets( &(buffer[0]), PPM_BUFFER_SIZE, infile ) )
	{
		if( buffer[0] == 'P' )
			switch( buffer[1] )
			{    /* we only support RAWBITS formats : */
					case '5' : 	type= 5 ; break ;
					case '6' : 	type= 6 ; break ;
					case '8' : 	type= 8 ; break ;
				default:
					show_error( "invalid or unsupported PPM/PNM file format in image file \"%s\"", path );
			}
		if( type > 0 )
		{
			while ( fgets( &(buffer[0]), PPM_BUFFER_SIZE, infile ) )
			{
				if( buffer[0] != '#' )
				{
					register int i = 0;
					if( width > 0 )
					{
						colors = atoi(&(buffer[i]));
						break;
					}
					width = atoi( &(buffer[i]) );
					while ( buffer[i] != '\0' && !isspace((int)buffer[i]) ) ++i;
					while ( isspace((int)buffer[i]) ) ++i;
					if( buffer[i] != '\0')
						height = atoi(&(buffer[i]));
				}
			}
		}
	}

	if( type > 0 && colors <= 255 &&
		width > 0 && width < MAX_IMPORT_IMAGE_SIZE &&
		height > 0 && height < MAX_IMPORT_IMAGE_SIZE )
	{
		CARD8 *data ;
		size_t row_size = width * ((type==6)?3:((type==8)?4:1));

		data = safemalloc( row_size );

		LOCAL_DEBUG_OUT("stored image size %dx%d", width,  height);
		im = create_asimage( width,  height, params->compression );
		prepare_scanline( im->width, 0, &buf, False );
		y = -1 ;
		/*cinfo.output_scanline*/
		while ( ++y < (int)height )
		{
			if( fread( data, sizeof (char), row_size, infile ) < row_size )
				break;

			raw2scanline( data, &buf, params->gamma_table, im->width, (type==5), (type==8));

			asimage_add_line (im, IC_RED,   buf.red  , y);
			asimage_add_line (im, IC_GREEN, buf.green, y);
			asimage_add_line (im, IC_BLUE,  buf.blue , y);
			if( type == 8 )
				asimage_add_line (im, IC_ALPHA,   buf.alpha  , y);
		}
		free_scanline(&buf, True);
		free( data );
	}
	fclose( infile );
	SHOW_TIME("image loading",started);
	return im ;
}

/***********************************************************************************/
/* Windows BMP file format :   									   				   */
static size_t
bmp_read32 (FILE *fp, CARD32 *data, int count)
{
  	size_t total = count;
	if( count > 0 )
	{
#ifdef WORDS_BIGENDIAN                         /* BMPs are encoded as Little Endian */
		CARD8 *raw = (CARD8*)data ;
#endif
		total = fread((char*) data, sizeof (CARD8), count<<2, fp)>>2;
		count = 0 ;
#ifdef WORDS_BIGENDIAN                         /* BMPs are encoded as Little Endian */
		while( count < total )
		{
			data[count] = (raw[0]<<24)|(raw[1]<<16)|(raw[2]<<8)|raw[3];
			++count ;
			raw += 4 ;
		}
#endif
	}
	return total;
}

static size_t
bmp_read16 (FILE *fp, CARD16 *data, int count)
{
  	size_t total = count;
	if( count > 0 )
	{
#ifdef WORDS_BIGENDIAN                         /* BMPs are encoded as Little Endian */
		CARD8 *raw = (CARD8*)data ;
#endif
		total = fread((char*) data, sizeof (CARD8), count<<1, fp)>>1;
		count = 0 ;
#ifdef WORDS_BIGENDIAN                         /* BMPs are encoded as Little Endian */
		while( count < total )
		{
			data[count] = (raw[0]<<16)|raw[1];
			++count ;
			raw += 2 ;
		}
#endif
	}
	return total;
}


ASImage *
read_bmp_image( FILE *infile, size_t data_offset, BITMAPINFOHEADER *bmp_info,
				ASScanline *buf, CARD8 *gamma_table,
				unsigned int width, unsigned int height,
				Bool add_colormap, unsigned int compression )
{
	Bool success = False ;
	CARD8 *cmap = NULL ;
	int cmap_entries = 0, cmap_entry_size = 4, row_size ;
	int y;
	ASImage *im = NULL ;
	CARD8 *data ;
	int direction = -1 ;

	if( bmp_read32( infile, &bmp_info->biSize, 1 ) )
	{
		if( bmp_info->biSize == 40 )
		{/* long header */
			bmp_read32( infile, (CARD32*)&bmp_info->biWidth, 2 );
			bmp_read16( infile, &bmp_info->biPlanes, 2 );
			bmp_info->biCompression = 1 ;
			success = (bmp_read32( infile, &bmp_info->biCompression, 6 )==6);
		}else
		{
			CARD16 dumm[2] ;
			bmp_read16( infile, &dumm[0], 2 );
			bmp_info->biWidth = dumm[0] ;
			bmp_info->biHeight = dumm[1] ;
			success = ( bmp_read16( infile, &bmp_info->biPlanes, 2 ) == 2 );
			bmp_info->biCompression = 0 ;
		}
	}
#ifdef LOCAL_DEBUG
	fprintf( stderr, "bmp.info.biSize = %ld(0x%lX)\n", bmp_info->biSize, bmp_info->biSize );
	fprintf( stderr, "bmp.info.biWidth = %ld\nbmp.info.biHeight = %ld\n",  bmp_info->biWidth,  bmp_info->biHeight );
	fprintf( stderr, "bmp.info.biPlanes = %d\nbmp.info.biBitCount = %d\n", bmp_info->biPlanes, bmp_info->biBitCount );
	fprintf( stderr, "bmp.info.biCompression = %ld\n", bmp_info->biCompression );
	fprintf( stderr, "bmp.info.biSizeImage = %ld\n", bmp_info->biSizeImage );
#endif
	if( ((int)(bmp_info->biHeight)) < 0 )
		direction = 1 ;
	if( height == 0 )
		height  = direction == 1 ? -((long)(bmp_info->biHeight)):bmp_info->biHeight ;
	if( width == 0 )
		width = bmp_info->biWidth ;

	if( !success || bmp_info->biCompression != 0 ||
		width > MAX_IMPORT_IMAGE_SIZE ||
		height > MAX_IMPORT_IMAGE_SIZE )
	{
		return NULL;
	}
	if( bmp_info->biBitCount < 16 )
		cmap_entries = 0x01<<bmp_info->biBitCount ;

	if( bmp_info->biSize != 40 )
		cmap_entry_size = 3;
	if( cmap_entries )
	{
		cmap = safemalloc( cmap_entries * cmap_entry_size );
		fread(cmap, sizeof (CARD8), cmap_entries * cmap_entry_size, infile);
	}

	if( add_colormap )
		data_offset += cmap_entries*cmap_entry_size ;

	fseek( infile, data_offset, SEEK_SET );
	row_size = (width*bmp_info->biBitCount)>>3 ;
	if( row_size == 0 )
		row_size = 1 ;
	else
		row_size = (row_size+3)/4 ;            /* everything is aligned by 32 bits */
	row_size *= 4 ;                            /* in bytes  */
	data = safemalloc( row_size );

	im = create_asimage( width,  height, compression );
	/* Window BMP files are little endian  - we need to swap Red and Blue */
	prepare_scanline( im->width, 0, buf, True );

	y =( direction == 1 )?0:height-1 ;
	while( y >= 0 && y < (int)height)
	{
		if( fread( data, sizeof (char), row_size, infile ) < (unsigned int)row_size )
			break;
 		dib_data_to_scanline(buf, bmp_info, gamma_table, data, cmap, cmap_entry_size); 
		asimage_add_line (im, IC_RED,   buf->red  , y);
		asimage_add_line (im, IC_GREEN, buf->green, y);
		asimage_add_line (im, IC_BLUE,  buf->blue , y);
		y += direction ;
	}
	free( data );
	if( cmap )
		free( cmap );
	return im ;
}

ASImage *
bmp2ASImage( const char * path, ASImageImportParams *params )
{
	ASImage *im = NULL ;
	/* More stuff */
	FILE         *infile;					   /* source file */
	ASScanline    buf;
	BITMAPFILEHEADER  bmp_header ;
	BITMAPINFOHEADER  bmp_info;
	START_TIME(started);


	if ((infile = open_image_file(path)) == NULL)
		return NULL;

	bmp_header.bfType = 0 ;
	if( bmp_read16( infile, &bmp_header.bfType, 1 ) )
		if( bmp_header.bfType == BMP_SIGNATURE )
			if( bmp_read32( infile, &bmp_header.bfSize, 3 ) == 3 )
				im = read_bmp_image( infile, bmp_header.bfOffBits, &bmp_info, &buf, params->gamma_table, 0, 0, False, params->compression );
#ifdef LOCAL_DEBUG
	fprintf( stderr, "bmp.header.bfType = 0x%X\nbmp.header.bfSize = %ld\nbmp.header.bfOffBits = %ld(0x%lX)\n",
					  bmp_header.bfType, bmp_header.bfSize, bmp_header.bfOffBits, bmp_header.bfOffBits );
#endif
	if( im != NULL )
		free_scanline( &buf, True );
	else
		show_error( "invalid or unsupported BMP format in image file \"%s\"", path );

	fclose( infile );
	SHOW_TIME("image loading",started);
	return im ;
}

/***********************************************************************************/
/* Windows ICO/CUR file format :   									   			   */

ASImage *
ico2ASImage( const char * path, ASImageImportParams *params )
{
	ASImage *im = NULL ;
	/* More stuff */
	FILE         *infile;					   /* source file */
	ASScanline    buf;
	int y, mask_bytes;
    CARD8  *and_mask;
	START_TIME(started);
	struct IconDirectoryEntry {
    	CARD8  bWidth;
    	CARD8  bHeight;
    	CARD8  bColorCount;
    	CARD8  bReserved;
    	CARD16  wPlanes;
    	CARD16  wBitCount;
    	CARD32 dwBytesInRes;
    	CARD32 dwImageOffset;
	};
	struct ICONDIR {
    	CARD16          idReserved;
    	CARD16          idType;
    	CARD16          idCount;
	} icon_dir;
   	struct IconDirectoryEntry  icon;
	BITMAPINFOHEADER bmp_info;

	if ((infile = open_image_file(path)) == NULL)
		return NULL;

	icon_dir.idType = 0 ;
	if( bmp_read16( infile, &icon_dir.idReserved, 3 ) == 3)
		if( icon_dir.idType == 1 || icon_dir.idType == 2)
		{
			fread( &(icon.bWidth), sizeof(CARD8),4,infile );
			bmp_read16( infile, &(icon.wPlanes), 2 );
			if( bmp_read32( infile, &(icon.dwBytesInRes), 2 ) == 2 )
			{
				fseek( infile, icon.dwImageOffset, SEEK_SET );
				im = read_bmp_image( infile, icon.dwImageOffset+40+(icon.bColorCount*4), &bmp_info, &buf, params->gamma_table,
					                 icon.bWidth, icon.bHeight, (icon.bColorCount==0), params->compression );
			}
		}
#ifdef LOCAL_DEBUG
	fprintf( stderr, "icon.dir.idType = 0x%X\nicon.dir.idCount = %d\n",  icon_dir.idType, icon_dir.idCount );
	fprintf( stderr, "icon[1].bWidth = %d(0x%X)\n",  icon.bWidth,  icon.bWidth );
	fprintf( stderr, "icon[1].bHeight = %d(0x%X)\n",  icon.bHeight,  icon.bHeight );
	fprintf( stderr, "icon[1].bColorCount = %d\n",  icon.bColorCount );
	fprintf( stderr, "icon[1].dwImageOffset = %ld(0x%lX)\n",  icon.dwImageOffset,  icon.dwImageOffset );
    fprintf( stderr, "icon[1].bmp_size = %ld\n",  icon.dwBytesInRes );
    fprintf( stderr, "icon[1].dwBytesInRes = %ld\n",  icon.dwBytesInRes );
#endif
	if( im != NULL )
	{
        mask_bytes = ((icon.bWidth>>3)+3)/4 ;    /* everything is aligned by 32 bits */
        mask_bytes *= 4 ;                      /* in bytes  */
        and_mask = safemalloc( mask_bytes );
        for( y = icon.bHeight-1 ; y >= 0 ; y-- )
		{
			int x ;
            if( fread( and_mask, sizeof (CARD8), mask_bytes, infile ) < (unsigned int)mask_bytes )
				break;
			for( x = 0 ; x < icon.bWidth ; ++x )
            {
				buf.alpha[x] = (and_mask[x>>3]&(0x80>>(x&0x7)))? 0x0000 : 0x00FF ;
            }
			im->channels[IC_ALPHA][y]  = store_data( NULL, (CARD8*)buf.alpha, im->width*4, 
													 ASStorage_32BitRLE|ASStorage_Bitmap, 0);
		}
        free( and_mask );
		free_scanline( &buf, True );
	}else
		show_error( "invalid or unsupported ICO format in image file \"%s\"", path );

	fclose( infile );
	SHOW_TIME("image loading",started);
	return im ;
}

/***********************************************************************************/
#ifdef HAVE_GIF		/* GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF */

int
gif_interlaced2y(int line /* 0 -- (height - 1) */, int height)
{
   	int passed_lines = 0;
   	int lines_in_current_pass;
   	/* pass 1 */
   	lines_in_current_pass = height / 8 + (height%8?1:0);
   	if (line < lines_in_current_pass) 
    	return line * 8;
   
   	passed_lines = lines_in_current_pass;
   	/* pass 2 */
   	if (height > 4) 
   	{
      	lines_in_current_pass = (height - 4) / 8 + ((height - 4)%8 ? 1 : 0);
      	if (line < lines_in_current_pass + passed_lines) 
         	return 4 + 8*(line - passed_lines);
      	passed_lines += lines_in_current_pass;
   	}
   	/* pass 3 */
   	if (height > 2) 
   	{
      	lines_in_current_pass = (height - 2) / 4 + ((height - 2)%4 ? 1 : 0);
      	if (line < lines_in_current_pass + passed_lines) 
        	return 2 + 4*(line - passed_lines);
    	passed_lines += lines_in_current_pass;
   	}
	return 1 + 2*(line - passed_lines);
}


ASImage *
gif2ASImage( const char * path, ASImageImportParams *params )
{
	FILE			   *fp ;
	int					status = GIF_ERROR;
	GifFileType        *gif;
	ASImage 	 	   *im = NULL ;
	int  		transparent = -1 ;
	unsigned int  		y;
	unsigned int		width = 0, height = 0;
	ColorMapObject     *cmap = NULL ;

	START_TIME(started);

	if ((fp = open_image_file(path)) == NULL)
		return NULL;
	if( (gif = open_gif_read(fp)) != NULL )
	{
		SavedImage	*sp = NULL ;
		int count = 0 ;
		if( (status = get_gif_saved_images(gif, params->subimage, &sp, &count )) == GIF_OK )
		{
			GifPixelType *row_pointer ;
#ifdef DEBUG_TRANSP_GIF
			fprintf( stderr, "Ext block = %p, count = %d\n", sp->ExtensionBlocks, sp->ExtensionBlockCount );
#endif
			if( sp->ExtensionBlocks )
				for ( y = 0; y < (unsigned int)sp->ExtensionBlockCount; y++)
				{
#ifdef DEBUG_TRANSP_GIF
					fprintf( stderr, "%d: func = %X, bytes[0] = 0x%X\n", y, sp->ExtensionBlocks[y].Function, sp->ExtensionBlocks[y].Bytes[0]);
#endif
					if( sp->ExtensionBlocks[y].Function == 0xf9 &&
			 			(sp->ExtensionBlocks[y].Bytes[0]&0x01))
					{
			   		 	transparent = ((unsigned int) sp->ExtensionBlocks[y].Bytes[GIF_GCE_TRANSPARENCY_BYTE])&0x00FF;
#ifdef DEBUG_TRANSP_GIF
						fprintf( stderr, "transp = %u\n", transparent );
#endif
					}
				}
			cmap = gif->SColorMap ;

			cmap = (sp->ImageDesc.ColorMap == NULL)?gif->SColorMap:sp->ImageDesc.ColorMap;
		    width = sp->ImageDesc.Width;
		    height = sp->ImageDesc.Height;

			if( cmap != NULL && (row_pointer = (unsigned char*)sp->RasterBits) != NULL &&
			    width < MAX_IMPORT_IMAGE_SIZE && height < MAX_IMPORT_IMAGE_SIZE )
			{
				int bg_color =   gif->SBackGroundColor ;
                int interlaced = sp->ImageDesc.Interlace;
                int image_y;
				CARD8 		 *r = NULL, *g = NULL, *b = NULL, *a = NULL ;
				r = safemalloc( width );	   
				g = safemalloc( width );	   
				b = safemalloc( width );	   
				a = safemalloc( width );

				im = create_asimage( width, height, params->compression );
				for (y = 0; y < height; ++y)
				{
					unsigned int x ;
					Bool do_alpha = False ;
                    image_y = interlaced ? gif_interlaced2y(y, height):y;
					for (x = 0; x < width; ++x)
					{
						int c = row_pointer[x];
      					if ( c == transparent)
						{
							c = bg_color ;
							do_alpha = True ;
							a[x] = 0 ;
						}else
							a[x] = 0x00FF ;
						
						r[x] = cmap->Colors[c].Red;
		        		g[x] = cmap->Colors[c].Green;
						b[x] = cmap->Colors[c].Blue;
	        		}
					row_pointer += x ;
					im->channels[IC_RED][image_y]  = store_data( NULL, r, width, ASStorage_RLEDiffCompress, 0);
				 	im->channels[IC_GREEN][image_y] = store_data( NULL, g, width, ASStorage_RLEDiffCompress, 0);	
					im->channels[IC_BLUE][image_y]  = store_data( NULL, b, width, ASStorage_RLEDiffCompress, 0);
					if( do_alpha )
						im->channels[IC_ALPHA][image_y]  = store_data( NULL, a, im->width, ASStorage_RLEDiffCompress|ASStorage_Bitmap, 0);
				}
				free(a);
				free(b);
				free(g);
				free(r);
			}
			free_gif_saved_images( sp, count );
		}else
			ASIM_PrintGifError();
		DGifCloseFile(gif);
		fclose( fp );
	}
	SHOW_TIME("image loading",started);
	return im ;
}
#else 			/* GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF */
ASImage *
gif2ASImage( const char * path, ASImageImportParams *params )
{
	show_error( "unable to load file \"%s\" - missing GIF image format libraries.\n", path );
	return NULL ;
}
#endif			/* GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF GIF */

#ifdef HAVE_TIFF/* TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF */
ASImage *
tiff2ASImage( const char * path, ASImageImportParams *params )
{
	TIFF 		 *tif ;

	static ASImage 	 *im = NULL ;
	CARD32 *data;
	CARD32 width = 1, height = 1;
	CARD16 depth = 4 ;
	CARD16 bits = 0 ;
	CARD32 rows_per_strip =0 ;
	CARD32 tile_width = 0, tile_length = 0 ;
	CARD16 photo = 0;
	START_TIME(started);

	if ((tif = TIFFOpen(path,"r")) == NULL)
	{
		show_error("cannot open image file \"%s\" for reading. Please check permissions.", path);
		return NULL;
	}

	if( params->subimage > 0 )
		if( !TIFFSetDirectory(tif, params->subimage))
			show_warning("failed to read subimage %d from image file \"%s\". Reading first available instead.", params->subimage, path);

	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
	if( !TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &depth) )
		depth = 3 ;
	if( !TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bits) )
		bits = 8 ;
	if( !TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rows_per_strip ) )
		rows_per_strip = height ;	
	if( !TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photo) )
		photo = 0 ;
	
	if( TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_width) ||
		TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_length) )
	{
		show_error( "Tiled TIFF image format is not supported yet." );
		return NULL;   
	}		

	if( rows_per_strip == 0 || rows_per_strip > height ) 
		rows_per_strip = height ;
	if( depth <= 0 ) 
		depth = 4 ;
	if( depth <= 2 && get_flags( photo, PHOTOMETRIC_RGB) )
		depth += 2 ;
	LOCAL_DEBUG_OUT( "size = %ldx%ld, depth = %d, bits = %d, rps = %ld, photo = 0x%X", 
					 width, height, depth, bits, rows_per_strip, photo );
	if( width < MAX_IMPORT_IMAGE_SIZE && height < MAX_IMPORT_IMAGE_SIZE )
	{
		if ((data = (CARD32*) _TIFFmalloc(width*rows_per_strip*sizeof(CARD32))) != NULL)
		{
			CARD8 		 *r = NULL, *g = NULL, *b = NULL, *a = NULL ;
			ASFlagType store_flags = ASStorage_RLEDiffCompress	;
			int first_row = 0 ;
			if( bits == 1 ) 
				set_flags( store_flags, ASStorage_Bitmap );
			
			im = create_asimage( width, height, params->compression );
			if( depth == 2 || depth == 4 ) 
				a = safemalloc( width );
			r = safemalloc( width );	   
			if( depth > 2 ) 
			{
				g = safemalloc( width );	   
				b = safemalloc( width );	   
			}	 
			
			while( first_row < height ) 
			{	
				if (TIFFReadRGBAStrip(tif, first_row, (void*)data))
				{
					register CARD32 *row = data ;
					int y = first_row + rows_per_strip ;
					if( y > height ) 
						y = height ;
					while( --y >= first_row )
					{
						int x ;
						for( x = 0 ; x < width ; ++x )
						{
							CARD32 c = row[x] ;
							if( depth == 4 || depth == 2 ) 
								a[x] = TIFFGetA(c);
							r[x]   = TIFFGetR(c);
							if( depth > 2 ) 
							{
								g[x] = TIFFGetG(c);
								b[x]  = TIFFGetB(c);
							}
						}
						im->channels[IC_RED][y]  = store_data( NULL, r, width, store_flags, 0);
						if( depth > 2 ) 
						{
					 		im->channels[IC_GREEN][y] = store_data( NULL, g, width, store_flags, 0);	
							im->channels[IC_BLUE][y]  = store_data( NULL, b, width, store_flags, 0);
						}else
						{
					 		im->channels[IC_GREEN][y] = dup_data( NULL, im->channels[IC_RED][y]);	  
							im->channels[IC_BLUE][y]  = dup_data( NULL, im->channels[IC_RED][y]);
						}		 
					
						if( depth == 4 || depth == 2 ) 
							im->channels[IC_ALPHA][y]  = store_data( NULL, a, width, store_flags, 0);
						row += width ;
					}
				}
				first_row += rows_per_strip ;
		    }
			if( b ) free( b );
			if( g ) free( g );
			if( r ) free( r );
			if( a ) free( a );
			_TIFFfree(data);
		}
	}
	/* close the file */
	TIFFClose(tif);
	SHOW_TIME("image loading",started);

	return im ;
}
#else 			/* TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF */

ASImage *
tiff2ASImage( const char * path, ASImageImportParams *params )
{
	show_error( "unable to load file \"%s\" - missing TIFF image format libraries.\n", path );
	return NULL ;
}
#endif			/* TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF TIFF */


ASImage *
load_xml2ASImage( ASImageManager *imman, const char *path, unsigned int compression )
{
	ASVisual fake_asv ;
	char *slash, *curr_path = NULL ;
	char *doc_str = NULL ;
	ASImage *im = NULL ;

	memset( &fake_asv, 0x00, sizeof(ASVisual) );
	if( (slash = strrchr( path, '/' )) != NULL )
		curr_path = mystrndup( path, slash-path );

	if((doc_str = load_file(path)) == NULL )
		show_error( "unable to load file \"%s\" file is either too big or is not readable.\n", path );
	else
	{
		im = compose_asimage_xml(&fake_asv, imman, NULL, doc_str, 0, 0, None, curr_path);
		free( doc_str );
	}

	if( curr_path )
		free( curr_path );
	return im ;
}


ASImage *
xml2ASImage( const char *path, ASImageImportParams *params )
{
	static ASImage 	 *im = NULL ;
	START_TIME(started);

	im = load_xml2ASImage( NULL, path, params->compression );

	SHOW_TIME("image loading",started);
	return im ;
}
