#ifndef EVENT_H_HEADER_INCLUDED
#define EVENT_H_HEADER_INCLUDED

struct ScreenInfo;

/* couple additional event masks to compensate for X defaults :*/
#define SelectionMask	(1L<<28)
#define ClientMask		(1L<<29)
#define MappingMask		(1L<<30)

#define ButtonAnyMask	(Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask)

typedef enum
{
    ASE_NotSupported     = 0,
    ASE_MouseMotionEvent = (0x01<<0),
    ASE_MousePressEvent  = (0x01<<1),
    ASE_KeyboardEvent    = (0x01<<2),
    ASE_Expose           = (0x01<<3),
	ASE_Config           = (0x01<<4),
	ASE_Redirection      = (0x01<<5),
#define ASE_SUBWINDOW_EVENTS     (ASE_MouseMotionEvent|ASE_MousePressEvent|ASE_KeyboardEvent)
#define ASE_POINTER_EVENTS       (ASE_MouseMotionEvent|ASE_MousePressEvent|ASE_KeyboardEvent)
}ASEventClass;

typedef struct ASEventDescription
{
	char         *name;
	unsigned long mask;
	ASEventClass  event_class;
	/* private fields : */
	long		  time_offset;
	long		  window_offset;
	Time		  last_time;
}ASEventDescription;


/* Event Handling return codes: */
#define ASE_Usefull		(0x01<<0)      /* event was processed */
#define ASE_Consumed 	(0x01<<1)      /* event was consumed by
                                        * processing function, and
								   		* should not be handled by
								   		* anything else */

typedef struct ASEvent
{
	/* the following info is parsed using ASEventDescription */
	unsigned long 		mask ;
	ASEventClass		eclass;
	Time                last_time ;
	Time                typed_last_time ;
	Time                event_time ;

	Window 	 	 		w ;
	struct ScreenInfo  *scr;
    struct ASWindow    *client ;
    struct ASCanvas    *widget ;
//    ASContextValue      context ;
    int                 context ;
	XEvent 		 		x ;
}ASEvent;

/**********************************************************************/

/* This is used for error handling : */
extern int           last_event_type ;
extern Window        last_event_window;

void event_setup( Bool local );
const char *event_type2name( int type );

/* better use macros so we could override/trace calls later on */
#define ASCheckMaskEvent(m,e)       check_event_masked(m,e)
#define ASCheckTypedEvent(t,e)      check_event_typed(t,e)
#define ASCheckTypedWindowEvent(w,t,e) check_event_typed_windowed(w,t,e)
#define ASCheckWindowEvent(w,m,e)   check_event_windowed(w,m,e)

#define ASEventScreen(e)            (&Scr)
#define ASNextEvent(e)              next_event (e)
#define ASMaskEvent(m,e)            mask_event (m,e)
#define ASWindowEvent(w,m,e)        window_event(w,m,e)

#define ASPeekEvent(e)              peek_event (e)

#define ASFlushIfEmpty()            flush_event_queue(True)
#define ASFlush()                   flush_event_queue(False)
#define ASFlushAndSync()            sync_event_queue(False)
#define ASSync(f)                   sync_event_queue(f)

/* We should use this event handling procedures instead of directly calling Xlib : */
void flush_event_queue(Bool check_pending);
void sync_event_queue(Bool forget);

struct ScreenInfo *query_event_screen( register XEvent *event );

Window get_xevent_window( XEvent *xevt );

void setup_asevent_from_xevent( ASEvent *event );

Bool check_event_masked( register long mask, register XEvent * event_return);
Bool check_event_typed( register int event_type, register XEvent * event_return);
Bool check_event_typed_windowed( Window w, int event_type, register XEvent *event_return );
Bool check_event_windowed (Window w, long event_mask, register XEvent * event_return);

int  next_event (register XEvent * event_return);
int  mask_event (long event_mask, register XEvent * event_return);
int  window_event (Window w, long event_mask, register XEvent * event_return);

int  peek_event (register XEvent * event_return);

/* blocking UnmapNotify events since that may bring us into Withdrawn state */
void quietly_unmap_window( Window w, long event_mask );
void quietly_reparent_window( Window w, Window new_parent, int x, int y, long event_mask );
void safely_destroy_window (Window w);

void add_window_event_mask( Window w, long event_mask );

#endif
