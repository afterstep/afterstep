#include "../../include/config.h"

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>

#include "afterimage.h"

#ifndef HAVE_LIBAFTERBASE
char *ApplicationName ;

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

int main(int argc, char* argv[])
{
    int x_fd ;
	XTextProperty name;
	Window w ;
	XClassHint class1;
    int i ;
	ASVisual *asv ;
	int screen, depth ;
	Display *dpy ;
	char *tmp ;

	set_application_name( argv[0] );
	
    dpy = XOpenDisplay(NULL);
	screen = DefaultScreen(dpy);
	depth = DefaultDepth( screen, screen );
	asv = create_asvisual( dpy, screen, depth, NULL );
	w = create_visual_window(asv, DefaultRootWindow(dpy), 32, 32, 640,512, 1, InputOutput, 0, NULL );

	tmp = get_application_name();
    XStringListToTextProperty (&tmp, 1, &name);

    class1.res_name = tmp;	/* for future use */
    class1.res_class = "ASView";

    XSetWMProtocols (dpy, w, &_XA_WM_DELETE_WINDOW, 1);
    XSetWMProperties (dpy, w, &name, &name, NULL, 0, NULL, NULL, &class1);
    /* showing window to let user see that we are doing something */
    XMapRaised (dpy, w);
    /* final cleanup */
    XFree ((char *) name.value);
    XSelectInput (dpy, w, (StructureNotifyMask | ButtonPress));

    while(1)
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
			return 0 ;
		    }
	}
    }
    if( dpy )
        XCloseDisplay (dpy);
    return 0 ;
}
