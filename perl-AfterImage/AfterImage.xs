#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include <libAfterImage/config.h>
#include <libAfterImage/afterbase.h>
#include <libAfterImage/afterimage.h>


MODULE = AfterImage		PACKAGE = AfterImage		

ASImage*
afterimage_load(filename)
		char*	   filename
	CODE:
		char* doc_str = load_file(filename);
		ASVisual* asv = create_asvisual(NULL, 0, 0, NULL);
		ASImage* im = NULL;
		/* set_output_threshold(0xffff); */
		im = compose_asimage_xml(asv, NULL, NULL, doc_str, ASFLAGS_EVERYTHING, 20, 0, NULL);
		RETVAL = im;
	OUTPUT:
		RETVAL

int
afterimage_save(im, filename, type, compression, opacity)
		ASImage* im
		char*    filename
		char*    type
		char*    compression
		char*    opacity
	CODE:
		RETVAL = save_asimage_to_file(filename, im, type, compression, opacity, 0, 1);
	OUTPUT:
		RETVAL

