#ifndef BMP_H_HEADER_INCLUDED
#define BMP_H_HEADER_INCLUDED

#include "asimage.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagBITMAPFILEHEADER {
#define BMP_SIGNATURE		0x4D42             /* "BM" */
		CARD16  bfType;
        CARD32  bfSize;
        CARD32  bfReserved;
        CARD32  bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
	{
	   CARD32 biSize;
	   CARD32 biWidth,  biHeight;
	   CARD16 biPlanes, biBitCount;
	   CARD32 biCompression;
	   CARD32 biSizeImage;
	   CARD32 biXPelsPerMeter, biYPelsPerMeter;
	   CARD32 biClrUsed, biClrImportant;
}BITMAPINFOHEADER;

void 
dib_line_to_scanline( ASScanline *buf, 
                      BITMAPINFOHEADER *bmp_info, CARD8 *gamma_table, 
					  CARD8 *data, CARD8 *cmap, int cmap_entry_size); 


#ifdef __cplusplus
}
#endif

#endif

