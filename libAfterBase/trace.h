#ifndef AFTERSTEP_TRACE_H
#define AFTERSTEP_TRACE_H

/*
 * This code is a (hopefully) nearly transparent way to keep track of 
 * memory allocations and deallocations, to make finding memory leaks 
 * easier.  GCC is required (for the __FUNCTION__ preprocessor macro).
 *
 * To use it, define DEBUG_TRACE before including this header
 */

#if !defined(DEBUG_ALLOCS) && defined(DEBUG_TRACE)
/* Select section : select functions to trace here : */
/* Xlib calls */
#undef TRACE_XDestroyWindow
#undef TRACE_XGetGeometry
#define TRACE_XNextEvent  	/* all of the Event retreival functions */
/* the following allows filtering events using event masks 
   (It also applicable to the tracing of DispatchEvent) : */
/* couple additional masks to compensate for X defaults :*/
#define SelectionMask		(1L<<29) 
#define ClientMask		(1L<<30) 
#define MappingMask		(1L<<31) 
/* use standard X event masks here : */
#define EVENT_TRACE_MASK	(EnterWindowMask|LeaveWindowMask|FocusChangeMask)

/* libafterstep functions */
#undef TRACE_load_font 		/* both load and unload actually */
#undef TRACE_mystrdup          	/* both mystrdup and mystrndup */

/* asimagelib functions */
#undef TRACE_UnloadImage 	/* both UnloadMask and UnloadImage */

/* afterstep functions */
#undef TRACE_AddWindow
#undef TRACE_SetFocus
#undef TRACE_SetupFrame
#undef TRACE_ResizeFrame       /* both ResizeFrame and ResizeClent */ 
#define TRACE_DispatchEvent     /* see also EVENT_TRACE_MASK above */

#undef TRACE_ReparentIt

#undef TRACE_ExecuteFunction
/* this will allow for selection of the particular type of functions to trace: */
#define MiscFuncMask		(1L<<1)
#define StateFuncMask		(1L<<2)
#define ConfigFuncMask		(1L<<3)
#define VirtualFuncMask		(1L<<4)
#define WindowFuncMask		(1L<<5)
#define ModuleFuncMask		(1L<<6)
#define MenuFuncMask		(1L<<7)
/* use any combination of those in here : */
#define FUNC_TRACE_MASK		WindowFuncMask

/* End of the Select section */



struct ASWindow ;
struct MyFont ;
struct ASRectangle ;
struct MenuRoot ;

/* Xlib calls */
#ifdef TRACE_XDestroyWindow
#define XDestroyWindow(d,w) 	trace_XDestroyWindow(d,w,__FILE__,__LINE__)
extern Status trace_XDestroyWindow (Display * dpy, Window w, const char *filename, int line);
#endif

#ifdef TRACE_XGetGeometry
#define XGetGeometry(d,dr,r,x,y,w,h,b,de) 	trace_XGetGeometry(d,dr,r,x,y,w,h,b,de,__FILE__,__LINE__)
extern Status trace_XGetGeometry (Display * dpy,  Drawable d, 
                                  Window *root_return, 
				  int *x_return, int *y_return, 
				  unsigned int *width_return, unsigned int *height_return, 
				  unsigned int *border_width_return, 
				  unsigned int *depth_return, 
				  const char *filename, int line);
#endif

#ifdef TRACE_XNextEvent
#define XCheckMaskEvent(d,m,e) trace_XCheckMaskEvent(d,m,e,__FILE__,__LINE__)
extern Bool trace_XCheckMaskEvent( Display* dpy, long mask, XEvent* event, const char* file, int line );

#define XCheckTypedEvent(d,t,e) trace_XCheckTypedEvent(d,t,e,__FILE__,__LINE__)
extern Bool trace_XCheckTypedEvent( Display* dpy, int event_type , XEvent* event_return, const char* file, int line );

#define XCheckTypedWindowEvent(d,w,t,e) trace_XCheckTypedWindowEvent(d,w,t,e,__FILE__,__LINE__)
extern Bool trace_XCheckTypedWindowEvent( Display* dpy, Window w, int event_type, XEvent* event_return, const char* file, int line );

#define XCheckWindowEvent(d,w,m,e) trace_XCheckWindowEvent(d,w,m,e,__FILE__,__LINE__)
extern Bool trace_XCheckWindowEvent( Display* dpy, Window w, long event_mask, XEvent* event_return, const char* file, int line);

#define XNextEvent(d,e) trace_XNextEvent(d,e,__FILE__,__LINE__)
extern int trace_XNextEvent( Display* dpy, XEvent* event, const char* file, int line);

#define XPeekEvent(d,e) trace_XPeekEvent(d,e,__FILE__,__LINE__)
extern int trace_XPeekEvent(Display* dpy, XEvent* event_return, const char* file, int line );

#define XMaskEvent(d,m,e) trace_XMaskEvent(d,m,e,__FILE__,__LINE__)
extern int trace_XMaskEvent(Display* dpy, long event_mask, XEvent* event_return, const char* file, int line );

#define XPutBackEvent(d,e) trace_XPutBackEvent(d,e,__FILE__,__LINE__)
extern int trace_XPutBackEvent(Display* dpy, XEvent* event, const char* file, int line );

#define XWindowEvent(d,e) trace_XWindowEvent(d,e,__FILE__,__LINE__)
extern int trace_XWindowEvent(Display* dpy,  Window w, long event_mask, XEvent* event_return, const char* file, int line);

#endif


/* libafterstep functions */
#ifdef TRACE_load_font
#define load_font(a,b)	trace_load_font(a,b,__FUNCTION__,__LINE__)
#define unload_font(a)	trace_unload_font(a,__FUNCTION__,__LINE__)
Bool trace_load_font ( const char *font_name, MyFont *font, const char *file, int line);
void trace_unload_font (MyFont *font, const char *file, int line);
#endif

#ifdef TRACE_mystrdup
#define mystrdup(a)	trace_mystrdup(a,__FUNCTION__,__LINE__)
#define mystrndup(a,b)	trace_mystrndup(a,b,__FUNCTION__,__LINE__)
char *trace_mystrdup ( const char *str, const char *file, int line);
char *trace_mystrndup (const char *str, size_t n, const char *file, int line);
#endif

/* asimagelib functions */
#ifdef TRACE_UnloadImage
#define UnloadImage(a)	trace_UnloadImage(a,__FUNCTION__,__LINE__)
#define UnloadMask(a)	trace_UnloadMask(a,__FUNCTION__,__LINE__)
int trace_UnloadImage (Pixmap pixmap, const char *file, int line);
int trace_UnloadMask (Pixmap mask, const char *file, int line);
#endif 

/* afterstep functions */
#ifdef TRACE_AddWindow
#define AddWindow(w) 		trace_AddWindow(w,__FILE__,__LINE__)
extern struct ASWindow *trace_AddWindow (Window w, const char *filename, int line);
#endif

#ifdef TRACE_SetFocus
#define SetFocus(w,fw)	trace_SetFocus(w,fw,__FILE__,__LINE__)
extern Bool trace_SetFocus (Window w, struct ASWindow *Fw, const char *filename, int line);
#endif

#ifdef TRACE_SetupFrame
#define SetupFrame(t,s) 	trace_SetupFrame(t,s,__FILE__,__LINE__)
extern void trace_SetupFrame (struct ASWindow *tmp_win, Bool sendEvent, const char *filename, int line);
#endif

#ifdef TRACE_ResizeFrame
#define ResizeFrame(t,n,td) 	trace_ResizeFrame(t,n,td,__FILE__,__LINE__)
extern unsigned int trace_ResizeFrame (struct ASWindow * t, struct ASRectangle * new_frame, unsigned int todo, const char *filename, int line);
#define ResizeClient(t,n) 	trace_ResizeClient(t,n,__FILE__,__LINE__)
extern void trace_ResizeClient (struct ASWindow * t, struct ASRectangle * new_client, const char *filename, int line);
#endif

#ifdef TRACE_DispatchEvent
#define DispatchEvent(e) 	trace_DispatchEvent(e,__FILE__,__LINE__)
extern void trace_DispatchEvent (XEvent *event, const char *filename, int line);
#endif

#ifdef TRACE_ReparentIt
#define ReparentIt(t,w)	trace_ReparentIt (t,w,__FILE__,__LINE__)
extern void trace_ReparentIt (struct ASWindow * t, Window to_win, const char *filename, int line);
#endif

#ifdef TRACE_ExecuteFunction
#define ExecuteFunction(f,a,w,t,e,c,v1,v2,v1u,v2u,m,M) trace_ExecuteFunction(f,a,w,t,e,c,v1,v2,v1u,v2u,m,M,__FILE__,__LINE__)
extern void trace_ExecuteFunction (int func, char *action, Window in_w,
		 struct ASWindow * tmp_win, XEvent * eventp, unsigned long context,
		 long val1, long val2, int val1_unit, int val2_unit,
		 struct MenuRoot * menu, int Module,
		 const char *filename, int line);
#endif

#endif /* DEBUG_TRACE */

#ifdef DEBUG_TRACE_X
#define XRaiseWindow(a,b) trace_XRaiseWindow(__FILE__, __FUNCTION__, __LINE__, a, b)
#define XLowerWindow(a,b) trace_XLowerWindow(__FILE__, __FUNCTION__, __LINE__, a, b)
#define XRestackWindows(a,b,c) trace_XRestackWindows(__FILE__, __FUNCTION__, __LINE__, a, b, c)
#define XResizeWindow(a,b,c,d) trace_XResizeWindow(__FILE__, __FUNCTION__, __LINE__, a, b, c, d)
#define XMoveWindow(a,b,c,d) trace_XMoveWindow(__FILE__, __FUNCTION__, __LINE__, a, b, c, d)
#define XMoveResizeWindow(a,b,c,d,e,f) trace_XMoveResizeWindow(__FILE__, __FUNCTION__, __LINE__, a, b, c, d, e, f)
void trace_XRaiseWindow(const char* file, const char* func, int line, Display* dpy, Window w);
void trace_XLowerWindow(const char* file, const char* func, int line, Display* dpy, Window w);
void trace_XRestackWindows(const char* file, const char* func, int line, Display* dpy, Window windows[], int nwindows);
void trace_XResizeWindow(const char* file, const char* func, int line, Display* dpy, Window w, int width, int height);
void trace_XMoveWindow(const char* file, const char* func, int line, Display* dpy, Window w, int x, int y);
void trace_XMoveResizeWindow(const char* file, const char* func, int line, Display* dpy, Window w, int x, int y, int width, int height);
int trace_enable_function(const char* name);
int trace_disable_function(const char* name);
extern const char* (*trace_window_id2name_hook)(Display *dpy, Window w, int* how_to_free);
#endif /* DEBUG_TRACE_X */

#endif /* AFTERSTEP_TRACE_H */
