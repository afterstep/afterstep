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
 *     asimage.h, asvisual.h, blender.h, asfont.h, import.h
 * AUTHOR
 * Sasha Vasko <sashav at sprintmail dot com>
 ******************/

typedef Bool (*as_image_writer_func)( ASImage *im, const char *path,
			  ASImageFileTypes type, int subimage,
			  unsigned int compression, unsigned int quality,
			  int max_colors, int depth );
extern as_image_writer_func as_image_file_writers[ASIT_Unknown];


/****f* libAfterImage/export/ASImage2file()
 * SYNOPSIS
 * Bool ASImage2file( ASImage *im, const char *dir, const char *file,
 *                    ASImageFileTypes type, int subimage,
 *                    unsigned int compression, unsigned int quality,
 *                    int max_colors, int depth );
 * INPUTS
 * im			- Image to write out.
 * dir          - directory name to write file into (optional,
 *                could be NULL)
 * file         - file name with or without directory name.
 * type         - output file format. ( see ASImageFileTypes )
 * subimage     - (optional set to 0 if don't care )
 * compression  - compression level of the resulting file (if supported
 *                by file format). Should be in range 0-100. 0 will yeld
 *                default compression level.
 * quality      - optional quality (if supported by file format). Should
 *                be in range 0-100. 0 will yeld default quality level.
 * max_colors   - reserved for future use.
 * depth        - if 1 - will cause greyscale image to be written. Set to
 *                -1 or 0 if don't care.
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
			  ASImageFileTypes type, int subimage,
			  unsigned int compression, unsigned int quality,
			  int max_colors, int depth );

Bool ASImage2xpm ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth );
Bool ASImage2png ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth );
Bool ASImage2jpeg( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth );
Bool ASImage2xcf ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth );
Bool ASImage2ppm ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth );
Bool ASImage2bmp ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth );
Bool ASImage2ico ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth );
Bool ASImage2gif ( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth );
Bool ASImage2tiff( ASImage *im, const char *path, ASImageFileTypes type, int subimage, unsigned int compression, unsigned int quality, int max_colors, int depth );

#endif
