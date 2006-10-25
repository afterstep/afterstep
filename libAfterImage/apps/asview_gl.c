#include "config.h"

#include <string.h>
#include <stdlib.h>

/****h* libAfterImage/tutorials/ASView_GL
 * NAME
 * ASView_GL
 * SYNOPSIS
 * Simple image viewer based on libAfterImage, using OpenGL API to transfer image to X.
 * DESCRIPTION
 * All we want to do here is to get image filename from the command line,
 * then load this image, and display it in simple window.
 * After that we would want to wait, until user closes our window.
 * SOURCE
 */
#define LOCAL_DEBUG
#define DO_CLOCKING

#include "../afterbase.h"
#include "../afterimage.h"
#include "common.h"

#ifdef DO_CLOCKING
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
#endif


#include <GL/gl.h>
#include <GL/glx.h>

void usage()
{
	printf( "Usage: asview [-h]|[image]\n");
	printf( "Where: image - filename of the image to display.\n");
}

void
glbuf2GLXPixmap(ASVisual *asv, Pixmap p, GLXContext glctx, ASImage *im )
{
//    GLboolean bparam;
	GLXPixmap glxp;
	int glbuf_size = 3 * im->width * im->height;
	CARD8 *glbuf = NULL;
	ASImageDecoder *imdec  = NULL ;
		
	if ((imdec = start_image_decoding(asv, im, SCL_DO_COLOR, 0, 0, im->width, im->height, NULL)) != NULL )
	{	 
		int i, l = glbuf_size;
		glbuf = safemalloc( glbuf_size );
		for (i = 0; i < (int)im->height; i++)
		{	
			int k = im->width;
			imdec->decode_image_scanline( imdec ); 
			while( --k >= 0 ) 
			{
				glbuf[--l] = imdec->buffer.blue[k] ;
				glbuf[--l] = imdec->buffer.green[k] ;
				glbuf[--l] = imdec->buffer.red[k] ;
			}	 
		}
		stop_image_decoding( &imdec );
	}else
		return;


//	show_warning( "asimage2pmap");
	//fprintf( stderr, "p = %lX, glxp = %lX, glctx = %p\n", p, glxp, glctx );
	glxp = glXCreateGLXPixmap( dpy, &(asv->visual_info), p);

	glXMakeCurrent (dpy, glxp, glctx);
	//fprintf( stderr, "line = %d, glerror = %d\n", __LINE__, glGetError() );
			
	glDisable(GL_BLEND);		/* optimize pixel transfer rates */
  	glDisable (GL_DEPTH_TEST);
  	glDisable (GL_DITHER);
  	glDisable (GL_FOG);
  	glDisable (GL_LIGHTING);
	
	glViewport(-(im->width/2), -(im->height/2), im->width, im->height);

	//glGetBooleanv (GL_DOUBLEBUFFER, &bparam);
	//fprintf( stderr, "doublebuffer = %d\n", bparam );
  	//if (bparam == GL_TRUE) 
   	glDrawBuffer (GL_FRONT);

	/* now put pixels on */
//	glRasterPos3i( 0, 0, 0 );
	//fprintf( stderr, "line = %d, glerror = %d\n", __LINE__, glGetError() );
	//fprintf( stderr, "i = %d\n", i );
//	show_warning( "glDrawPixels ...");
	glDrawPixels( im->width, im->height, GL_RGB, GL_UNSIGNED_BYTE, glbuf );
	free( glbuf );
//	show_warning( "glDrawPixels  done");
	glXMakeCurrent (dpy, None, NULL);	  
	glXDestroyGLXPixmap( dpy, glxp);
	glFinish(); 				   

}


int main(int argc, char* argv[])
{
	char *image_file = "rose512.jpg" ;
	ASImage *im ;
	/* see ASView.1 : */
	set_application_name( argv[0] );
#if (HAVE_AFTERBASE_FLAG==1)
	set_output_threshold(OUTPUT_LEVEL_DEBUG);
#endif

	if( argc > 1 )
	{
		if( strcmp( argv[1], "-h" ) == 0 )
		{
			usage();
			return 0;
		}
		image_file = argv[1] ;
	}else
	{
		show_warning( 	"Image filename was not specified. "
						"Using default: \"%s\"", image_file );
		usage();
	}
	/* see ASView.2 : */
	im = file2ASImage( image_file, 0xFFFFFFFF, SCREEN_GAMMA, 0, getenv("IMAGE_PATH"), NULL );

	/* The following could be used to dump JPEG version of the image into
	 * stdout : */
	/* ASImage2file( im, NULL, NULL, ASIT_Jpeg, NULL ); */


	if( im != NULL )
	{
#ifndef X_DISPLAY_MISSING
		Window w = None;
		ASVisual *asv ;
		int screen, depth ;

#if 0
		XRectangle *rects ;	unsigned int rects_count =0; int i ;
		rects = get_asimage_channel_rects( im, IC_ALPHA, 10, 
											&rects_count );
		fprintf( stderr, " %d rectangles generated : \n", rects_count );
		for( i = 0 ; i < rects_count ; ++i )
			fprintf( stderr, "\trect[%d]=%dx%d%+d%+d;\n", 
					 i, rects[i].width, rects[i].height, 
					 rects[i].x, rects[i].y );
#endif

	    dpy = XOpenDisplay(NULL);
		XSynchronize (dpy, True);
		if (! glXQueryExtension (dpy, NULL, NULL))
		{	
    		show_error("X server does not support OpenGL GLX extension");
			return 0 ;
		}
		_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", 
											False);
		screen = DefaultScreen(dpy);
		depth = DefaultDepth( dpy, screen );
		/* see ASView.3 : */
		asv = create_asvisual( dpy, screen, depth, NULL );

		/* test example for fill_asimage : */
#if 0		 
		fill_asimage(asv, im, 0, 0, 50, 50, 0xFFFF0000);
		fill_asimage(asv, im, 50, 50, 100, 50, 0xFFFF0000);
		fill_asimage(asv, im, 0, 100, 200, 50, 0xFFFF0000);
		fill_asimage(asv, im, 150, 0, 50, 50, 0xFFFF0000);
#endif
		/* test example for conversion to argb32 :*/
#if 0
		{
			ASImage *tmp = tile_asimage( asv, im, 0, 0, im->width, im->height, TINT_NONE, ASA_ARGB32, 
										  0, ASIMAGE_QUALITY_DEFAULT );	 
			destroy_asimage( &im );
			set_flags( tmp->flags, ASIM_DATA_NOT_USEFUL|ASIM_XIMAGE_NOT_USEFUL );
			im = tmp ;
		}		   
#endif		   
		


		/* see ASView.4 : */
		w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32,
			                         im->width, im->height, 1, 0, NULL,
									 "ASView - using OpenGL", image_file );
		if( w != None )
		{
			Pixmap    p;
			int i ;
//			GLXContext glctx;

	  		XMapRaised   (dpy, w);
			XSync(dpy,False);
			//sleep_a_millisec(1000);
			//XSync(dpy,False);
			/* see ASView.5 : */
//			show_warning( "asimage2pmap Done");
			p = create_visual_pixmap( asv, DefaultRootWindow(dpy), im->width, im->height, 0 );
//			glctx = glXCreateContext (dpy, &(asv->visual_info), NULL, True);
			{
				START_TIME(started);
				time_t t = time(NULL);
				for( i = 0 ; i < 100 ; ++i ) 
				{
#if 1
# if 1
					asimage2drawable_gl( asv, p, im, 0, 0, 0, 0, im->width, im->height,  
										 im->width, im->height,  False );
# else										
					asimage2drawable_gl( asv, w, im, 0, 0, 0, 0, im->width, im->height,  
										 im->width, im->height,  True );
# endif
#else
					asimage2drawable( asv, p, im, NULL, 0, 0, 0, 0, im->width, im->height, False);
#endif 						  
//					glbuf2GLXPixmap(asv, p, glctx, im );	
					LOCAL_DEBUG_OUT ("timestamp%s","");
				}
				SHOW_TIME("", started);
				fprintf( stderr, "runtime = %ld sec\n", time(NULL)-t );
			}
			/* print_storage(NULL); */
			destroy_asimage( &im );
			/* see common.c:set_window_background_and_free(): */
			p = set_window_background_and_free( w, p );
		}
		/* see common.c: wait_closedown() : */
		wait_closedown(w);

#else
		/* writing result into the file */
		ASImage2file( im, NULL, "asview.jpg", ASIT_Jpeg, NULL );
#endif
	}

    return 0 ;
}
/**************/


