#include "config.h"

#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>

#include "afterimage.h"

#ifndef HAVE_AFTERBASE
char *ApplicationName ;
Display *dpy ;

void
set_application_name (char *argv0)
{
	char         *temp = strrchr (argv0, '/');

	/* Save our program name - for error messages */
	ApplicationName = temp ? temp + 1 : argv0;
}

const char *
get_application_name()
{
	return ApplicationName;
}

unsigned int
get_output_threshold()
{
  return 5 ;
}
#endif

Atom _XA_WM_DELETE_WINDOW = None;


Window 
create_top_level_window( ASVisual *asv, Window root, int x, int y, unsigned int width, unsigned int height, unsigned int border_width, unsigned long attr_mask, XSetWindowAttributes *attr, char *app_class )
{
	Window w ;
	char *tmp ;
	XTextProperty name;
	XClassHint class1;

	w = create_visual_window(asv, root, x, y, width, height, border_width, InputOutput, attr_mask, attr );

	tmp = (char*)get_application_name();
    XStringListToTextProperty (&tmp, 1, &name);

    class1.res_name = tmp;	/* for future use */
    class1.res_class = app_class;
    XSetWMProtocols (dpy, w, &_XA_WM_DELETE_WINDOW, 1);
    XSetWMProperties (dpy, w, &name, &name, NULL, 0, NULL, NULL, &class1);
    /* final cleanup */
    XFree ((char *) name.value);
	
	return w;
}


int main(int argc, char* argv[])
{
	Window w ;
	ASVisual *asv ;
	int screen, depth ;
	char *image_file = "test.xpm" ;
	ASImage *im ;

	set_application_name( argv[0] );
	
	if( argc > 1 ) 
		image_file = argv[1] ;
	
    dpy = XOpenDisplay(NULL);
	_XA_WM_DELETE_WINDOW = XInternAtom( dpy, "WM_DELETE_WINDOW", False);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( dpy, screen );
	im = file2ASImage( image_file, 0xFFFFFFFF, SCREEN_GAMMA, 0, NULL ); 
	
	if( im != NULL ) 
	{	
		asv = create_asvisual( dpy, screen, depth, NULL );
		
		w = create_top_level_window( asv, DefaultRootWindow(dpy), 32, 32, im->width, im->height, 1, 0, NULL, "ASView" );
		if( w != None )
		{
			Pixmap p ;
			
			XSelectInput (dpy, w, (StructureNotifyMask | ButtonPress));
	  		XMapRaised   (dpy, w);
			p = asimage2pixmap( asv, DefaultRootWindow(dpy), im, NULL, False );
			if( p != None ) 
			{
				XSetWindowBackgroundPixmap( dpy, w, p );
				XClearWindow( dpy, w );
				XFlush( dpy );
				XFreePixmap( dpy, p );
				p = None ;
			}
		}	

	    while(w != None)
  		{
    		XEvent event ;
	        XNextEvent (dpy, &event);
  		    switch(event.type)
			{
		  		case ButtonPress:
					break ;
	  		    case ClientMessage:
			        if ((event.xclient.format == 32) &&
	  			        (event.xclient.data.l[0] == _XA_WM_DELETE_WINDOW))
		  			{
						XDestroyWindow( dpy, w );
						XFlush( dpy );
						w = None ;
				    }
					break;
			}
  		}
	}		
    if( dpy )
        XCloseDisplay (dpy);
    return 0 ;
}
