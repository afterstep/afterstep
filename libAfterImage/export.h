#ifndef EXPORT_H_HEADER_INCLUDED
#define EXPORT_H_HEADER_INCLUDED

typedef Bool (*as_image_writer_func)( ASImage *im, const char *path,
			  ASImageFileTypes type, int subimage,
			  unsigned int compression, unsigned int quality,
			  int max_colors, int depth );
extern as_image_writer_func as_image_file_writers[ASIT_Unknown];


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
