#ifndef IMPORT_H_HEADER_INCLUDED
#define IMPORT_H_HEADER_INCLUDED

#include "xcf.h"
#include "asimage.h"

#define SCREEN_GAMMA 1.0	/* default gamma correction value - */

/* what file formats we support : */
typedef enum
{
	ASIT_Xpm = 0,
	ASIT_ZCompressedXpm,
	ASIT_GZCompressedXpm,
	ASIT_Png,
	ASIT_Jpeg,
	ASIT_Xcf,
	ASIT_Ppm,
	ASIT_Pnm,
	ASIT_Bmp,
	ASIT_Ico,
	ASIT_Cur,
	ASIT_Gif,
	ASIT_Tiff,
	ASIT_Xbm,
	ASIT_Targa,
	ASIT_Pcx,
	ASIT_Unknown
}ASImageFileTypes;

typedef ASImage* (*as_image_loader_func)( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
extern as_image_loader_func as_image_file_loaders[ASIT_Unknown];

ASImage *xpm2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *png2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *jpeg2ASImage( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *xcf2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *ppm2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *bmp2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *ico2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *gif2ASImage ( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );
ASImage *tiff2ASImage( const char * path, ASFlagType what, double gamma, CARD8 *gamma_table, int subimage, unsigned int compression );


/*
 * Main Interface to image loading :
 * file - filename, gamma - screen gamma to correct image,
 * compression 0 - 100 with 100 being max available compression,
 * ... - NULL terminated list of diferent PATHs to search for file in
 */
ASImage *file2ASImage( const char *file, ASFlagType what, double gamma, unsigned int compression, ... );

/* somewhat easier to call function : */
Pixmap file2pixmap(struct ASVisual *asv, Window root, const char *realfilename, Pixmap *mask_out);

#endif

