#ifndef EXPORT_H_HEADER_INCLUDED
#define EXPORT_H_HEADER_INCLUDED
/****h* libAfterImage/export.h
 * DESCRIPTION
 * Image output into different file formats.
 * SEE ALSO
 * Functions :
 *  		ASImage2file()
 *
 * Other libAfterImage modules :
 *          ascmap.h asfont.h asimage.h asvisual.h blender.h export.h
 *          import.h transform.h ximage.h
 * AUTHOR
 * Sasha Vasko <sasha at aftercode dot net>
 ******************/
#define EXPORT_GRAYSCALE			(0x01<<0)
#define EXPORT_ALPHA				(0x01<<1)
#define EXPORT_APPEND				(0x01<<3)  /* adds subimage  */

typedef struct
{
	ASImageFileTypes type;
	ASFlagType flags ;
	int dither ;
	int opaque_threshold ;
	int max_colors ;
}ASXpmExportParams ;

typedef struct
{
	ASImageFileTypes type;
	ASFlagType flags ;
	int compression ;
}ASPngExportParams ;

typedef struct
{
	ASImageFileTypes type;
	ASFlagType flags ;
	int quality ;
}ASJpegExportParams ;

typedef struct
{
	ASImageFileTypes type;
	ASFlagType flags ;
	int dither ;
	int opaque_threshold ;
}ASGifExportParams ;

typedef union ASImageExportParams
{
	ASImageFileTypes   type;
	ASXpmExportParams  xpm;
	ASPngExportParams  png;
	ASJpegExportParams jpeg;
	ASGifExportParams  gif;
}ASImageExportParams;

typedef Bool (*as_image_writer_func)( ASImage *im, const char *path,
									  ASImageExportParams *params );
extern as_image_writer_func as_image_file_writers[ASIT_Unknown];


/****f* libAfterImage/export/ASImage2file()
 * SYNOPSIS
 * Bool ASImage2file( ASImage *im, const char *dir, const char *file,
					  ASImageFileTypes type, ASImageExportParams *params );
 * INPUTS
 * im			- Image to write out.
 * dir          - directory name to write file into (optional,
 *                could be NULL)
 * file         - file name with or without directory name.
 * type         - output file format. ( see ASImageFileTypes )
 * params       - pointer to ASImageExportParams union's member for the
 *                above type, with additional export parameters, such as
 *                quality, compression, etc. If NULL then all defaults
 *                will be used.
 * RETURN VALUE
 * True on success. False - failure.
 * DESCRIPTION
 * ASImage2file will construct filename out of dir and file components
 * and then will call specific filter to write out file in requested
 * format.
 * NOTES
 * Some formats support compression, others support lossy compression,
 * yet others allows you to limit number of colors and colordepth.
 * Each specific filter will try to interpret those parameters in its
 * own way.
 * EXAMPLE
 * asmerge.c: ASMerge.3
 *********/

Bool
ASImage2file( ASImage *im, const char *dir, const char *file,
			  ASImageFileTypes type, ASImageExportParams *params );

Bool ASImage2xpm ( ASImage *im, const char *path, ASImageExportParams *params );
Bool ASImage2png ( ASImage *im, const char *path, ASImageExportParams *params );
Bool ASImage2jpeg( ASImage *im, const char *path, ASImageExportParams *params );
Bool ASImage2xcf ( ASImage *im, const char *path, ASImageExportParams *params );
Bool ASImage2ppm ( ASImage *im, const char *path, ASImageExportParams *params );
Bool ASImage2bmp ( ASImage *im, const char *path, ASImageExportParams *params );
Bool ASImage2ico ( ASImage *im, const char *path, ASImageExportParams *params );
Bool ASImage2gif ( ASImage *im, const char *path, ASImageExportParams *params );
Bool ASImage2tiff( ASImage *im, const char *path, ASImageExportParams *params );

#endif
