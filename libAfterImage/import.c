/* This file contains code for unified image loading from XPM file  */
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "../configure.h"

/*#define LOCAL_DEBUG*/
#define DO_CLOCKING

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/screen.h"
#include "../include/asimage.h"
#include "../include/ashash.h"
#include "../include/asimage.h"
#include "../include/parse.h"

#ifdef XPM
#include <X11/xpm.h>

int
show_xpm_error (int Err)
{
	if (Err == XpmSuccess)
		return 0;
	fprintf (stderr, "\nlibXpm error: ");
	if (Err == XpmOpenFailed)
		fprintf (stderr, "error reading XPM file: %s\n", XpmGetErrorString (Err));
	else if (Err == XpmColorFailed)
		fprintf (stderr, "Couldn't allocate required colors\n");
	/* else if (Err == XpmFileInvalid)
	   fprintf (stderr, "Invalid Xpm File\n"); */
	else if (Err == XpmColorError)
		fprintf (stderr, "Invalid Color specified in Xpm file\n");
	else if (Err == XpmNoMemory)
		fprintf (stderr, "Insufficient Memory\n");
	return 1;
}

static struct {
	char 	*name ;
	ARGB32   argb ;
	} XpmRGB_Colors[] =
{/* this entire table is taken from libXpm 	       */
 /* Developed by HeDu 3/94 (hedu@cul-ipn.uni-kiel.de)  */
    {"AliceBlue", MAKE_ARGB32(255, 240, 248, 255)},
    {"AntiqueWhite", MAKE_ARGB32(255, 250, 235, 215)},
    {"Aquamarine", MAKE_ARGB32(255, 50, 191, 193)},
    {"Azure", MAKE_ARGB32(255, 240, 255, 255)},
    {"Beige", MAKE_ARGB32(255, 245, 245, 220)},
    {"Bisque", MAKE_ARGB32(255, 255, 228, 196)},
    {"Black", MAKE_ARGB32(255, 0, 0, 0)},
    {"BlanchedAlmond", MAKE_ARGB32(255, 255, 235, 205)},
    {"Blue", MAKE_ARGB32(255, 0, 0, 255)},
    {"BlueViolet", MAKE_ARGB32(255, 138, 43, 226)},
    {"Brown", MAKE_ARGB32(255, 165, 42, 42)},
    {"burlywood", MAKE_ARGB32(255, 222, 184, 135)},
    {"CadetBlue", MAKE_ARGB32(255, 95, 146, 158)},
    {"chartreuse", MAKE_ARGB32(255, 127, 255, 0)},
    {"chocolate", MAKE_ARGB32(255, 210, 105, 30)},
    {"Coral", MAKE_ARGB32(255, 255, 114, 86)},
    {"CornflowerBlue", MAKE_ARGB32(255, 34, 34, 152)},
    {"cornsilk", MAKE_ARGB32(255, 255, 248, 220)},
    {"Cyan", MAKE_ARGB32(255, 0, 255, 255)},
    {"DarkGoldenrod", MAKE_ARGB32(255, 184, 134, 11)},
    {"DarkGreen", MAKE_ARGB32(255, 0, 86, 45)},
    {"DarkKhaki", MAKE_ARGB32(255, 189, 183, 107)},
    {"DarkOliveGreen", MAKE_ARGB32(255, 85, 86, 47)},
    {"DarkOrange", MAKE_ARGB32(255, 255, 140, 0)},
    {"DarkOrchid", MAKE_ARGB32(255, 139, 32, 139)},
    {"DarkSalmon", MAKE_ARGB32(255, 233, 150, 122)},
    {"DarkSeaGreen", MAKE_ARGB32(255, 143, 188, 143)},
    {"DarkSlateBlue", MAKE_ARGB32(255, 56, 75, 102)},
    {"DarkSlateGray", MAKE_ARGB32(255, 47, 79, 79)},
    {"DarkTurquoise", MAKE_ARGB32(255, 0, 166, 166)},
    {"DarkViolet", MAKE_ARGB32(255, 148, 0, 211)},
    {"DeepPink", MAKE_ARGB32(255, 255, 20, 147)},
    {"DeepSkyBlue", MAKE_ARGB32(255, 0, 191, 255)},
    {"DimGray", MAKE_ARGB32(255, 84, 84, 84)},
    {"DodgerBlue", MAKE_ARGB32(255, 30, 144, 255)},
    {"Firebrick", MAKE_ARGB32(255, 142, 35, 35)},
    {"FloralWhite", MAKE_ARGB32(255, 255, 250, 240)},
    {"ForestGreen", MAKE_ARGB32(255, 80, 159, 105)},
    {"gainsboro", MAKE_ARGB32(255, 220, 220, 220)},
    {"GhostWhite", MAKE_ARGB32(255, 248, 248, 255)},
    {"Gold", MAKE_ARGB32(255, 218, 170, 0)},
    {"Goldenrod", MAKE_ARGB32(255, 239, 223, 132)},
    {"Gray", MAKE_ARGB32(255, 126, 126, 126)},
    {"Gray0", MAKE_ARGB32(255, 0, 0, 0)},
    {"Gray1", MAKE_ARGB32(255, 3, 3, 3)},
    {"Gray10", MAKE_ARGB32(255, 26, 26, 26)},
    {"Gray100", MAKE_ARGB32(255, 255, 255, 255)},
    {"Gray11", MAKE_ARGB32(255, 28, 28, 28)},
    {"Gray12", MAKE_ARGB32(255, 31, 31, 31)},
    {"Gray13", MAKE_ARGB32(255, 33, 33, 33)},
    {"Gray14", MAKE_ARGB32(255, 36, 36, 36)},
    {"Gray15", MAKE_ARGB32(255, 38, 38, 38)},
    {"Gray16", MAKE_ARGB32(255, 41, 41, 41)},
    {"Gray17", MAKE_ARGB32(255, 43, 43, 43)},
    {"Gray18", MAKE_ARGB32(255, 46, 46, 46)},
    {"Gray19", MAKE_ARGB32(255, 48, 48, 48)},
    {"Gray2", MAKE_ARGB32(255, 5, 5, 5)},
    {"Gray20", MAKE_ARGB32(255, 51, 51, 51)},
    {"Gray21", MAKE_ARGB32(255, 54, 54, 54)},
    {"Gray22", MAKE_ARGB32(255, 56, 56, 56)},
    {"Gray23", MAKE_ARGB32(255, 59, 59, 59)},
    {"Gray24", MAKE_ARGB32(255, 61, 61, 61)},
    {"Gray25", MAKE_ARGB32(255, 64, 64, 64)},
    {"Gray26", MAKE_ARGB32(255, 66, 66, 66)},
    {"Gray27", MAKE_ARGB32(255, 69, 69, 69)},
    {"Gray28", MAKE_ARGB32(255, 71, 71, 71)},
    {"Gray29", MAKE_ARGB32(255, 74, 74, 74)},
    {"Gray3", MAKE_ARGB32(255, 8, 8, 8)},
    {"Gray30", MAKE_ARGB32(255, 77, 77, 77)},
    {"Gray31", MAKE_ARGB32(255, 79, 79, 79)},
    {"Gray32", MAKE_ARGB32(255, 82, 82, 82)},
    {"Gray33", MAKE_ARGB32(255, 84, 84, 84)},
    {"Gray34", MAKE_ARGB32(255, 87, 87, 87)},
    {"Gray35", MAKE_ARGB32(255, 89, 89, 89)},
    {"Gray36", MAKE_ARGB32(255, 92, 92, 92)},
    {"Gray37", MAKE_ARGB32(255, 94, 94, 94)},
    {"Gray38", MAKE_ARGB32(255, 97, 97, 97)},
    {"Gray39", MAKE_ARGB32(255, 99, 99, 99)},
    {"Gray4", MAKE_ARGB32(255, 10, 10, 10)},
    {"Gray40", MAKE_ARGB32(255, 102, 102, 102)},
    {"Gray41", MAKE_ARGB32(255, 105, 105, 105)},
    {"Gray42", MAKE_ARGB32(255, 107, 107, 107)},
    {"Gray43", MAKE_ARGB32(255, 110, 110, 110)},
    {"Gray44", MAKE_ARGB32(255, 112, 112, 112)},
    {"Gray45", MAKE_ARGB32(255, 115, 115, 115)},
    {"Gray46", MAKE_ARGB32(255, 117, 117, 117)},
    {"Gray47", MAKE_ARGB32(255, 120, 120, 120)},
    {"Gray48", MAKE_ARGB32(255, 122, 122, 122)},
    {"Gray49", MAKE_ARGB32(255, 125, 125, 125)},
    {"Gray5", MAKE_ARGB32(255, 13, 13, 13)},
    {"Gray50", MAKE_ARGB32(255, 127, 127, 127)},
    {"Gray51", MAKE_ARGB32(255, 130, 130, 130)},
    {"Gray52", MAKE_ARGB32(255, 133, 133, 133)},
    {"Gray53", MAKE_ARGB32(255, 135, 135, 135)},
    {"Gray54", MAKE_ARGB32(255, 138, 138, 138)},
    {"Gray55", MAKE_ARGB32(255, 140, 140, 140)},
    {"Gray56", MAKE_ARGB32(255, 143, 143, 143)},
    {"Gray57", MAKE_ARGB32(255, 145, 145, 145)},
    {"Gray58", MAKE_ARGB32(255, 148, 148, 148)},
    {"Gray59", MAKE_ARGB32(255, 150, 150, 150)},
    {"Gray6", MAKE_ARGB32(255, 15, 15, 15)},
    {"Gray60", MAKE_ARGB32(255, 153, 153, 153)},
    {"Gray61", MAKE_ARGB32(255, 156, 156, 156)},
    {"Gray62", MAKE_ARGB32(255, 158, 158, 158)},
    {"Gray63", MAKE_ARGB32(255, 161, 161, 161)},
    {"Gray64", MAKE_ARGB32(255, 163, 163, 163)},
    {"Gray65", MAKE_ARGB32(255, 166, 166, 166)},
    {"Gray66", MAKE_ARGB32(255, 168, 168, 168)},
    {"Gray67", MAKE_ARGB32(255, 171, 171, 171)},
    {"Gray68", MAKE_ARGB32(255, 173, 173, 173)},
    {"Gray69", MAKE_ARGB32(255, 176, 176, 176)},
    {"Gray7", MAKE_ARGB32(255, 18, 18, 18)},
    {"Gray70", MAKE_ARGB32(255, 179, 179, 179)},
    {"Gray71", MAKE_ARGB32(255, 181, 181, 181)},
    {"Gray72", MAKE_ARGB32(255, 184, 184, 184)},
    {"Gray73", MAKE_ARGB32(255, 186, 186, 186)},
    {"Gray74", MAKE_ARGB32(255, 189, 189, 189)},
    {"Gray75", MAKE_ARGB32(255, 191, 191, 191)},
    {"Gray76", MAKE_ARGB32(255, 194, 194, 194)},
    {"Gray77", MAKE_ARGB32(255, 196, 196, 196)},
    {"Gray78", MAKE_ARGB32(255, 199, 199, 199)},
    {"Gray79", MAKE_ARGB32(255, 201, 201, 201)},
    {"Gray8", MAKE_ARGB32(255, 20, 20, 20)},
    {"Gray80", MAKE_ARGB32(255, 204, 204, 204)},
    {"Gray81", MAKE_ARGB32(255, 207, 207, 207)},
    {"Gray82", MAKE_ARGB32(255, 209, 209, 209)},
    {"Gray83", MAKE_ARGB32(255, 212, 212, 212)},
    {"Gray84", MAKE_ARGB32(255, 214, 214, 214)},
    {"Gray85", MAKE_ARGB32(255, 217, 217, 217)},
    {"Gray86", MAKE_ARGB32(255, 219, 219, 219)},
    {"Gray87", MAKE_ARGB32(255, 222, 222, 222)},
    {"Gray88", MAKE_ARGB32(255, 224, 224, 224)},
    {"Gray89", MAKE_ARGB32(255, 227, 227, 227)},
    {"Gray9", MAKE_ARGB32(255, 23, 23, 23)},
    {"Gray90", MAKE_ARGB32(255, 229, 229, 229)},
    {"Gray91", MAKE_ARGB32(255, 232, 232, 232)},
    {"Gray92", MAKE_ARGB32(255, 235, 235, 235)},
    {"Gray93", MAKE_ARGB32(255, 237, 237, 237)},
    {"Gray94", MAKE_ARGB32(255, 240, 240, 240)},
    {"Gray95", MAKE_ARGB32(255, 242, 242, 242)},
    {"Gray96", MAKE_ARGB32(255, 245, 245, 245)},
    {"Gray97", MAKE_ARGB32(255, 247, 247, 247)},
    {"Gray98", MAKE_ARGB32(255, 250, 250, 250)},
    {"Gray99", MAKE_ARGB32(255, 252, 252, 252)},
    {"Green", MAKE_ARGB32(255, 0, 255, 0)},
    {"GreenYellow", MAKE_ARGB32(255, 173, 255, 47)},
    {"honeydew", MAKE_ARGB32(255, 240, 255, 240)},
    {"HotPink", MAKE_ARGB32(255, 255, 105, 180)},
    {"IndianRed", MAKE_ARGB32(255, 107, 57, 57)},
    {"ivory", MAKE_ARGB32(255, 255, 255, 240)},
    {"Khaki", MAKE_ARGB32(255, 179, 179, 126)},
    {"lavender", MAKE_ARGB32(255, 230, 230, 250)},
    {"LavenderBlush", MAKE_ARGB32(255, 255, 240, 245)},
    {"LawnGreen", MAKE_ARGB32(255, 124, 252, 0)},
    {"LemonChiffon", MAKE_ARGB32(255, 255, 250, 205)},
    {"LightBlue", MAKE_ARGB32(255, 176, 226, 255)},
    {"LightCoral", MAKE_ARGB32(255, 240, 128, 128)},
    {"LightCyan", MAKE_ARGB32(255, 224, 255, 255)},
    {"LightGoldenrod", MAKE_ARGB32(255, 238, 221, 130)},
    {"LightGoldenrodYellow", MAKE_ARGB32(255, 250, 250, 210)},
    {"LightGray", MAKE_ARGB32(255, 168, 168, 168)},
    {"LightPink", MAKE_ARGB32(255, 255, 182, 193)},
    {"LightSalmon", MAKE_ARGB32(255, 255, 160, 122)},
    {"LightSeaGreen", MAKE_ARGB32(255, 32, 178, 170)},
    {"LightSkyBlue", MAKE_ARGB32(255, 135, 206, 250)},
    {"LightSlateBlue", MAKE_ARGB32(255, 132, 112, 255)},
    {"LightSlateGray", MAKE_ARGB32(255, 119, 136, 153)},
    {"LightSteelBlue", MAKE_ARGB32(255, 124, 152, 211)},
    {"LightYellow", MAKE_ARGB32(255, 255, 255, 224)},
    {"LimeGreen", MAKE_ARGB32(255, 0, 175, 20)},
    {"linen", MAKE_ARGB32(255, 250, 240, 230)},
    {"Magenta", MAKE_ARGB32(255, 255, 0, 255)},
    {"Maroon", MAKE_ARGB32(255, 143, 0, 82)},
    {"MediumAquamarine", MAKE_ARGB32(255, 0, 147, 143)},
    {"MediumBlue", MAKE_ARGB32(255, 50, 50, 204)},
    {"MediumForestGreen", MAKE_ARGB32(255, 50, 129, 75)},
    {"MediumGoldenrod", MAKE_ARGB32(255, 209, 193, 102)},
    {"MediumOrchid", MAKE_ARGB32(255, 189, 82, 189)},
    {"MediumPurple", MAKE_ARGB32(255, 147, 112, 219)},
    {"MediumSeaGreen", MAKE_ARGB32(255, 52, 119, 102)},
    {"MediumSlateBlue", MAKE_ARGB32(255, 106, 106, 141)},
    {"MediumSpringGreen", MAKE_ARGB32(255, 35, 142, 35)},
    {"MediumTurquoise", MAKE_ARGB32(255, 0, 210, 210)},
    {"MediumVioletRed", MAKE_ARGB32(255, 213, 32, 121)},
    {"MidnightBlue", MAKE_ARGB32(255, 47, 47, 100)},
    {"MintCream", MAKE_ARGB32(255, 245, 255, 250)},
    {"MistyRose", MAKE_ARGB32(255, 255, 228, 225)},
    {"moccasin", MAKE_ARGB32(255, 255, 228, 181)},
    {"NavajoWhite", MAKE_ARGB32(255, 255, 222, 173)},
    {"Navy", MAKE_ARGB32(255, 35, 35, 117)},
    {"NavyBlue", MAKE_ARGB32(255, 35, 35, 117)},
    {"None", MAKE_ARGB32(0, 0, 0, 1)},
    {"OldLace", MAKE_ARGB32(255, 253, 245, 230)},
    {"OliveDrab", MAKE_ARGB32(255, 107, 142, 35)},
    {"Orange", MAKE_ARGB32(255, 255, 135, 0)},
    {"OrangeRed", MAKE_ARGB32(255, 255, 69, 0)},
    {"Orchid", MAKE_ARGB32(255, 239, 132, 239)},
    {"PaleGoldenrod", MAKE_ARGB32(255, 238, 232, 170)},
    {"PaleGreen", MAKE_ARGB32(255, 115, 222, 120)},
    {"PaleTurquoise", MAKE_ARGB32(255, 175, 238, 238)},
    {"PaleVioletRed", MAKE_ARGB32(255, 219, 112, 147)},
    {"PapayaWhip", MAKE_ARGB32(255, 255, 239, 213)},
    {"PeachPuff", MAKE_ARGB32(255, 255, 218, 185)},
    {"peru", MAKE_ARGB32(255, 205, 133, 63)},
    {"Pink", MAKE_ARGB32(255, 255, 181, 197)},
    {"Plum", MAKE_ARGB32(255, 197, 72, 155)},
    {"PowderBlue", MAKE_ARGB32(255, 176, 224, 230)},
    {"purple", MAKE_ARGB32(255, 160, 32, 240)},
    {"Red", MAKE_ARGB32(255, 255, 0, 0)},
    {"RosyBrown", MAKE_ARGB32(255, 188, 143, 143)},
    {"RoyalBlue", MAKE_ARGB32(255, 65, 105, 225)},
    {"SaddleBrown", MAKE_ARGB32(255, 139, 69, 19)},
    {"Salmon", MAKE_ARGB32(255, 233, 150, 122)},
    {"SandyBrown", MAKE_ARGB32(255, 244, 164, 96)},
    {"SeaGreen", MAKE_ARGB32(255, 82, 149, 132)},
    {"seashell", MAKE_ARGB32(255, 255, 245, 238)},
    {"Sienna", MAKE_ARGB32(255, 150, 82, 45)},
    {"SkyBlue", MAKE_ARGB32(255, 114, 159, 255)},
    {"SlateBlue", MAKE_ARGB32(255, 126, 136, 171)},
    {"SlateGray", MAKE_ARGB32(255, 112, 128, 144)},
    {"snow", MAKE_ARGB32(255, 255, 250, 250)},
    {"SpringGreen", MAKE_ARGB32(255, 65, 172, 65)},
    {"SteelBlue", MAKE_ARGB32(255, 84, 112, 170)},
    {"Tan", MAKE_ARGB32(255, 222, 184, 135)},
    {"Thistle", MAKE_ARGB32(255, 216, 191, 216)},
    {"tomato", MAKE_ARGB32(255, 255, 99, 71)},
    {"Transparent", MAKE_ARGB32(0, 0, 0, 1)},
    {"Turquoise", MAKE_ARGB32(255, 25, 204, 223)},
    {"Violet", MAKE_ARGB32(255, 156, 62, 206)},
    {"VioletRed", MAKE_ARGB32(255, 243, 62, 150)},
    {"Wheat", MAKE_ARGB32(255, 245, 222, 179)},
    {"White", MAKE_ARGB32(255, 255, 255, 255)},
    {"WhiteSmoke", MAKE_ARGB32(255, 245, 245, 245)},
    {"Yellow", MAKE_ARGB32(255, 255, 255, 0)},
    {"YellowGreen", MAKE_ARGB32(255, 50, 216, 56)},
    {NULL,0}
};

static ARGB32*
decode_xpm_colors( XpmImage *xpm_im )
{
	ARGB32* cmap = safemalloc( xpm_im->ncolors*sizeof(ARGB32) );
	XpmColor *xpm_cmap = xpm_im->colorTable ;
	int i ;
	static ASHashTable *xpm_color_names = NULL ;

	if( xpm_im == NULL )
	{/* why don't we do the cleanup here : */
		destroy_ashash(&xpm_color_names);
		return NULL;
	}
	if( xpm_color_names == NULL )
	{
		xpm_color_names = create_ashash( 0, casestring_hash_value, casestring_compare, NULL );
		for( i = 0 ; XpmRGB_Colors[i].name != NULL ; i++ )
			add_hash_item( xpm_color_names, (ASHashableValue)XpmRGB_Colors[i].name, (void*)XpmRGB_Colors[i].argb );
LOCAL_DEBUG_OUT( "done building XPM color names hash - %d entries.", i );
	}
	for( i = 0 ; i < xpm_im->ncolors ; i++ )
	{
		char **colornames = (char**)&(xpm_cmap[i].string);
		register int key = 5 ;
		do
		{
			if( colornames[key] )
			{
				if( *(colornames[key]) != '#' )
					if( get_hash_item( xpm_color_names, (ASHashableValue)colornames[key], (void**)&(cmap[i]) ) == ASH_Success )
					{
						LOCAL_DEBUG_OUT(" xpm color \"%s\" matched into 0x%lX", colornames[key], cmap[i] );
						break;
					}
				if( parse_argb_color( colornames[key], &(cmap[i]) ) != colornames[key] )
				{
					LOCAL_DEBUG_OUT(" xpm color \"%s\" parsed into 0x%lX", colornames[key], cmap[i] );
					break;
				}
				LOCAL_DEBUG_OUT(" xpm color \"%s\" is invalid :(", colornames[key] );
				/* unknown color - leaving it at 0 - that will make it transparent */
			}
		}while ( --key > 0);
	}
	return cmap;
}


ASImage *
xpm2ASImage( const char * path, ASFlagType what )
{
	XpmImage      xpmImage;
	ASImage 	 *im = NULL ;
#ifdef DO_CLOCKING
	clock_t       started = clock ();
#endif
	ARGB32       *cmap = NULL ;
	unsigned int *data ;
	ASScanline    xpm_buf;
	Bool 		  do_alpha = False ;
	int 		  i ;
#ifdef LOCAL_DEBUG
	CARD32       *tmp ;
#endif

	LOCAL_DEBUG_CALLER_OUT ("(\"%s\", 0x%lX)", path, what);
	if( XpmReadFileToXpmImage ((char *)path, &xpmImage, NULL) != XpmSuccess)
		return NULL;

	im = safecalloc( 1, sizeof( ASImage ) );
	asimage_start( im, xpmImage.width, xpmImage.height );
	prepare_scanline( im->width, 0, &xpm_buf, False );

	cmap = decode_xpm_colors( &xpmImage );
LOCAL_DEBUG_OUT( "done building colormap - %d entries.", xpmImage.ncolors );
	data = xpmImage.data ;
	/* now we can proceed to actually encoding out ASImage : */
#ifdef LOCAL_DEBUG
	tmp = safemalloc( im->width * sizeof(CARD32));
#endif

	for (i = 0; i < xpmImage.ncolors; i++)
		if( ARGB32_ALPHA8(cmap[i]) != 0x00FF )
		{
			do_alpha = True ;
			break;
		}
LOCAL_DEBUG_OUT( "do_alpha is %d. im->height = %d, im->width = %d", do_alpha, im->height, im->width );

	for (i = 0; i < im->height; i++)
	{
		register int k = im->width ;
		while( --k >= 0 )
		{
			register CARD32 c = cmap[data[k]] ;
			xpm_buf.red[k]   = ARGB32_RED8(c);
			xpm_buf.green[k] = ARGB32_GREEN8(c);
			xpm_buf.blue[k]  = ARGB32_BLUE8(c);
			if( do_alpha )
				xpm_buf.alpha[k]  = ARGB32_ALPHA8(c);
		}
		asimage_add_line (im, IC_RED,   xpm_buf.red, i);
		asimage_add_line (im, IC_GREEN, xpm_buf.green, i);
		asimage_add_line (im, IC_BLUE,  xpm_buf.blue, i);
		if( do_alpha )
			asimage_add_line (im, IC_ALPHA,  xpm_buf.alpha, i);
#ifdef LOCAL_DEBUG
		if( !asimage_compare_line( im, IC_RED,  xpm_buf.red, tmp, i, True ) )
			exit(0);
		if( !asimage_compare_line( im, IC_GREEN,  xpm_buf.green, tmp, i, True ) )
			exit(0);
		if( !asimage_compare_line( im, IC_BLUE,  xpm_buf.blue, tmp, i, True ) )
			exit(0);
		if( do_alpha )
			if( !asimage_compare_line( im, IC_ALPHA,  xpm_buf.alpha, tmp, i, True ) )
				exit(0);
#endif
		data += im->width ;
	}
	XpmFreeXpmImage (&xpmImage);
	free_scanline(&xpm_buf, True);
	free( cmap );
#ifdef DO_CLOCKING
	printf ("\n image loading time (clocks): %lu\n", clock () - started);
#endif
	return im;
}

#else  /* XPM */

ASImage *
xpm2ASImage( const char * path, ASFlagType *what )
{
	return NULL ;
}

#endif /* XPM */


