#ifndef AFTERSTEP_XPM_H_HEADER_INCLUDED
#define AFTERSTEP_XPM_H_HEADER_INCLUDED

/* our own Xpm handling code : */

typedef enum
{
	XPM_Outside = 0,
	XPM_InFile,
	XPM_InImage,
	XPM_InComments,
	XPM_InString
}ASXpmParseState;

#define MAX_XPM_BPP	16

typedef struct ASXpmFile
{
 	FILE 	*fp;
	char   **data;                             /* preparsed and preloaded data */

#define AS_XPM_BUFFER_UNDO		8
#define AS_XPM_BUFFER_SIZE		8192

#ifdef HAVE_LIBXPM
	XpmImage xpmImage;
#else
	char 	 buffer[AS_XPM_BUFFER_UNDO+AS_XPM_BUFFER_SIZE];
	size_t   bytes_in;
	size_t   curr_byte;
#endif

	int 	 curr_img;
	int 	 curr_img_line;

	ASXpmParseState parse_state;

	char 	*str_buf ;
	size_t 	 str_buf_size ;

	unsigned short width, height, bpp;
	size_t     cmap_size;
	ASScanline scl ;

	ARGB32		*cmap;
	ASHashTable *cmap_name_xref;

	Bool do_alpha ;
}ASXpmFile;

typedef enum {
	XPM_Error = -2,
	XPM_EndOfFile = -1,
	XPM_EndOfImage = 0,
	XPM_Success = 1
}ASXpmStatus;

/*************************************************************************
 * High level xpm reading interface ;
 *************************************************************************/
void 		close_xpm_file( ASXpmFile **xpm_file );
ASXpmFile  *open_xpm_file( const char *realfilename );
Bool		parse_xpm_header( ASXpmFile *xpm_file );
ASXpmStatus get_xpm_string( ASXpmFile *xpm_file );
ASImage    *create_xpm_image( ASXpmFile *xpm_file, int compression );
Bool 		build_xpm_colormap( ASXpmFile *xpm_file );
Bool 		convert_xpm_scanline( ASXpmFile *xpm_file, unsigned int line );


#endif /* AFTERSTEP_XPM_H_HEADER_INCLUDED */
