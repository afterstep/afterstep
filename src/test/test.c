#include "../../configure.h"

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>

#ifdef ISC			/* Saul */
#include <sys/bsdtypes.h>	/* Saul */
#endif /* Saul */

#include <stdlib.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#ifdef I18N
#include <X11/Xlocale.h>
#endif

#define IN_MODULE
#define MODULE_X_INTERFACE

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/ascolor.h"
#include "../../include/stepgfx.h"
#include "../../include/module.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"
#include "../../include/mystyle.h"
#include "../../include/background.h"

char *MyName;
ScreenInfo Scr;			/* AS compatible screen information structure */
Display *dpy;			/* which display are we talking to */
int screen;
static Atom wm_del_win;

void DeadPipe (int nonsense);

XColor col1[12] = {{0, 0xff00, 0xff00, 0xff00},
                 {0, 0x0000, 0x0000, 0x0000},
		 {0, 0xffff, 0xffff, 0x0000},
		 {0, 0x7fff, 0xffff, 0x0000},
		 {0, 0x0000, 0xffff, 0x0000},
		 {0, 0x0000, 0xffff, 0x7fff},
		 {0, 0x0000, 0xffff, 0xffff},
		 {0, 0x0000, 0x7fff, 0xffff},
		 {0, 0x0000, 0x0000, 0xffff},
		 {0, 0x7fff, 0x0000, 0xffff},
		 {0, 0xffff, 0x0000, 0xffff},
		 {0, 0xffff, 0x0000, 0x7fff}
} ;

double offset1[2] = { 0.0, 1.0 };
double offset2[3] = { 0.0, 0.5, 1.0 };
double offset3[5] = { 0.0, 0.125, 0.25, 0.5625, 1.0 };

ASCOLOR* ascol1 ;

typedef struct {
    int x1, y1, x2, y2, w, h ;
    int type1, type2 ;
    int finess ;
    int maxcol ;
    int col_offset, col_num ;
    double* offset ;
    int iterations ;

}astest_gradient ;

astest_gradient grad_test[] = 
{
/* large linear gradients */
{10, 10, 10, 320, 300, 300, 0, 2, 40, 50, 0, 2, offset1, 1 },  
{10, 10, 320, 10, 300, 600, 1, 3, 40, 50, 0, 2, offset1, 1 },
{10, 10, 10, 320, 700, 300, 0, 2, 40, 56, 0, 2, offset1, 1 },  
{10, 10, 320, 10, 300, 700, 1, 3, 40, 56, 0, 2, offset1, 1 },
/* small linear gradients */
{10, 10, 70, 10, 50, 160, 0, 2, 40, 256, 0, 2, offset1, 10 },  
{10, 10, 70, 10, 50, 160, 1, 3, 40, 256, 0, 2, offset1, 10 },
/* large diagonal gradients */
{10, 10, 10, 320, 600, 300, 2, 0, 15, 56, 0, 2, offset1, 1 },
{10, 10, 10, 320, 600, 300, 3, 1, 15, 56, 0, 2, offset1, 1 },
{10, 10, 320, 10, 300, 600, 2, 0, 15, 56, 0, 2, offset1, 1 },
{10, 10, 320, 10, 300, 600, 3, 1, 15, 56, 0, 2, offset1, 1 },
/* small diagonal gradients */
{10, 10, 70, 10, 50, 160, 2, 0, 40, 256, 0, 2, offset1, 10 },
{10, 10, 70, 10, 50, 160, 3, 1, 40, 256, 0, 2, offset1, 10 },
{10, 10, 180, 10, 160, 50, 2, 0, 40, 256, 0, 2, offset1, 10 },
{10, 10, 180, 10, 160, 50, 3, 1, 40, 256, 0, 2, offset1, 10 },

/* now lets try the same with different colors */
/* large linear gradients */
{10, 10, 10, 320, 300, 300, 0, 2, 40, 56, 2, 2, offset1, 1 },  
{10, 10, 320, 10, 300, 600, 1, 3, 40, 56, 2, 2, offset1, 1 },
{10, 10, 10, 320, 700, 300, 0, 2, 40, 56, 2, 2, offset1, 1 },  
{10, 10, 320, 10, 300, 700, 1, 3, 40, 56, 2, 2, offset1, 1 },
/* small linear gradients */
{10, 10, 70, 10, 50, 160, 0, 2, 40, 256, 2, 2, offset1, 10 },  
{10, 10, 70, 10, 50, 160, 1, 3, 40, 256, 2, 2, offset1, 10 },
/* large diagonal gradients */
{10, 10, 10, 320, 600, 300, 2, 0, 15, 56, 2, 2, offset1, 1 },
{10, 10, 10, 320, 600, 300, 3, 1, 15, 56, 2, 2, offset1, 1 },
{10, 10, 320, 10, 300, 600, 2, 0, 15, 56, 2, 2, offset1, 1 },
{10, 10, 320, 10, 300, 600, 3, 1, 15, 56, 2, 2, offset1, 1 },
/* small diagonal gradients */
{10, 10, 70, 10, 50, 160, 2, 0, 40, 256, 2, 2, offset1, 10 },
{10, 10, 70, 10, 50, 160, 3, 1, 40, 256, 2, 2, offset1, 10 },
{10, 10, 180, 10, 160, 50, 2, 0, 40, 256, 2, 2, offset1, 10 },
{10, 10, 180, 10, 160, 50, 3, 1, 40, 256, 2, 2, offset1, 10 },
/* now lets try several points */
/* large linear gradients */
{10, 10, 10, 320, 300, 300, 0, 2, 40, 256, 5, 5, offset3, 1 },  
{10, 10, 320, 10, 300, 600, 1, 3, 40, 256, 5, 5, offset3, 1 },
{10, 10, 10, 320, 700, 300, 0, 2, 40, 256, 5, 5, offset3, 1 },  
{10, 10, 320, 10, 300, 700, 1, 3, 40, 256, 5, 5, offset3, 1 },
/* small linear gradients */
{10, 10, 70, 10, 50, 160, 0, 2, 40, 256, 5, 5, offset3, 10 },  
{10, 10, 70, 10, 50, 160, 1, 3, 40, 256, 5, 5, offset3, 10 },
/* large diagonal gradients */
{10, 10, 10, 320, 600, 300, 2, 0, 15, 256, 5, 5, offset3, 1 },
{10, 10, 10, 320, 600, 300, 3, 1, 15, 256, 5, 5, offset3, 1 },
{10, 10, 320, 10, 300, 600, 2, 0, 15, 256, 5, 5, offset3, 1 },
{10, 10, 320, 10, 300, 600, 3, 1, 15, 256, 5, 5, offset3, 1 },
/* small diagonal gradients */
{10, 10, 70, 10, 50, 160, 2, 0, 40, 256, 5, 5, offset3, 10 },
{10, 10, 70, 10, 50, 160, 3, 1, 40, 256, 5, 5, offset3, 10 },
{10, 10, 180, 10, 160, 50, 2, 0, 40, 256, 5, 5, offset3, 10 },
{10, 10, 180, 10, 160, 50, 3, 1, 40, 256, 5, 5, offset3, 10 },

{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0 }
};

void DoTest( Window w )
{
  int i;
  clock_t started ;
  Pixmap p ;
  static int curr_test = 0 ;
#define CTEST	grad_test[curr_test]    
    p = XCreatePixmap( dpy, w, 1000, 800, Scr.d_depth );

    if(grad_test[curr_test].iterations == 0 ) curr_test = 0 ;
    for( i = 0 ; i < 1; i++ )
    {
//        DrawASGradient( dpy, p, CTEST.x1, CTEST.y1, CTEST.w, CTEST.h, CTEST.col_num, ascol1+CTEST.col_offset, CTEST.offset, 0, CTEST.type1, CTEST.maxcol, CTEST.finess);
        draw_gradient( dpy, p, CTEST.x2, CTEST.y2, CTEST.w, CTEST.h, CTEST.col_num, col1+CTEST.col_offset, CTEST.offset, 0, CTEST.type2, CTEST.maxcol, CTEST.finess);
    }
    XFlush( dpy );
    XSetWindowBackgroundPixmap( dpy, w, p ); 
    XClearWindow(dpy, w );
    XFlush( dpy );
    for( i = 0 ; i < 10; i++ )
    {
//        DrawASGradient( dpy, p, CTEST.x1, CTEST.y1, CTEST.w, CTEST.h, CTEST.col_num, ascol1+CTEST.col_offset, CTEST.offset, 0, CTEST.type1, CTEST.maxcol, CTEST.finess);
        draw_gradient( dpy, p, CTEST.x2, CTEST.y2, CTEST.w, CTEST.h, CTEST.col_num, col1+CTEST.col_offset, CTEST.offset, 0, CTEST.type2, CTEST.maxcol, CTEST.finess);
    }
    XFlush( dpy );

    started = clock();
    for( i = 0 ; i < grad_test[curr_test].iterations; i++ )
    {
//        DrawASGradient( dpy, p, CTEST.x1, CTEST.y1, CTEST.w, CTEST.h, CTEST.col_num, ascol1+CTEST.col_offset, CTEST.offset, 0, CTEST.type1, CTEST.maxcol, CTEST.finess);
    }
    XFlush( dpy );
    fprintf( stderr, "Test#%3d: DrawASGradient in %5lu mlsec.\n", curr_test, ((clock()-started)*100)/CLOCKS_PER_SEC );         

    started = clock();
    for( i = 0 ; i < grad_test[curr_test].iterations; i++ )
    {
        draw_gradient( dpy, p, CTEST.x2, CTEST.y2, CTEST.w, CTEST.h, CTEST.col_num, col1+CTEST.col_offset, CTEST.offset, 0, CTEST.type2, CTEST.maxcol, CTEST.finess);
    }
    XFlush( dpy );
    fprintf( stderr, "Test#%3d: draw_gradient  in %5lu mlsec.\n", curr_test, ((clock()-started)*100)/CLOCKS_PER_SEC );         

    XFreePixmap( dpy, p );
    curr_test++ ;
}

char *path = "/aaa/bbb/ccc:~/:$PATH";
char *file1 = "~/GNUstep/Library/AfterStep/animate";

int main(int argc, char* argv[])
{
  int x_fd ;
  XTextProperty name;
  Window w ;
  XClassHint class1;
  char* display_name = NULL ;
  char* test, *test2 ;
  
    test = mystrdup(path);
    fprintf(stderr, "\nsource:[%s]",test );
    replaceEnvVar(&test);
    fprintf(stderr, "\nreplaceEndVar:[%s]",test );

    fprintf(stderr, "\nsource:[%s]",file1 );
    test2 = PutHome(file1);
    fprintf(stderr, "\nPutHome:[%s]",test2 );
    free( test2 );
    test2 = findIconFile(file1, test, X_OK);
    fprintf(stderr, "\nfindIconFile:[%s]\n",test2 );
    if(test2) free( test2 );
    test2 = findIconFile("animate", test, X_OK);
    fprintf(stderr, "\nfindIconFile:[%s]\n",test2 );
    free( test );

    exit(0);
    
#if 0
  
 /* Save our program name - for error messages */    
    SetMyName(argv[0]);

  signal (SIGHUP, DeadPipe);
  signal (SIGINT, DeadPipe);
  signal (SIGPIPE, DeadPipe);
  signal (SIGQUIT, DeadPipe);
  signal (SIGTERM, DeadPipe);

    x_fd = ConnectX(&Scr, display_name, PropertyChangeMask );
    
    wm_del_win = XInternAtom (dpy, "WM_DELETE_WINDOW", False);

    w = XCreateSimpleWindow (dpy, Scr.Root, 1, 1, 800,750 , 1, 0, 0 );

    XStringListToTextProperty (&MyName, 1, &name);

    class1.res_name = MyName;	/* for future use */
    class1.res_class = "ASTest";

    XSetWMProtocols (dpy, w, &wm_del_win, 1);
    XSetWMProperties (dpy, w, &name, &name, NULL, 0, NULL, NULL, &class1);
    /* showing window to let user see that we are doing something */
    XMapRaised (dpy, w);
    /* final cleanup */
    XFree ((char *) name.value);
    XSelectInput (dpy, w, (StructureNotifyMask | ButtonPress));  
    
    ascol1 = XColors2ASCOLORS(col1,12);

    while(1)
    {
      XEvent event ;
        XNextEvent (dpy, &event);
        switch(event.type)
	{
	    case ButtonPress:
		DoTest(w);
		break ;
	    case ClientMessage:
	        if ((event.xclient.format == 32) &&
	            (event.xclient.data.l[0] == wm_del_win))
		    {
			XDestroyWindow( dpy, w );
			XFlush( dpy );
			return 0 ;
		    }
	}
    }
    return 0 ;
#endif    
}

void 
DeadPipe (int nonsense)
{
    
#ifdef DEBUG_ALLOCS
/* normally, we let the system clean up, but when auditing time comes 
 * around, it's best to have the books in order... */
    {
      print_unfreed_mem();
    }
    
#endif /* DEBUG_ALLOCS */

  XFlush (dpy);			
  XCloseDisplay (dpy);		
  exit (0);
}
