#ifndef AS_MOVERESIZE_H_HEADER_FILE_INCLUDED
#define AS_MOVERESIZE_H_HEADER_FILE_INCLUDED

/****h* libAfterWidget/moveresize.h
 * SYNOPSIS
 * Protocols for interactive widget movement/sizing.
 * DESCRIPTION
 * Initiating widget/app calls :
 * 		move_widget_interactively() or resize_widget_interactively()
 * Those functions initialize ASMoveResizeData, register event handler
 * with main widget's event handler and return pointer to the data.
 *
 * When interactive event loop neds to chage placement of widget -
 * it calls method stored in data.
 *
 * When interactive action is complete or cancelled - another method is
 * called pointer to which is stored in data, then data and loop
 * unregister self from the main even handler.
 *
 * AvoidCover algorithm:
 * When we are resizing - everything is nice and simple. At most 2 sides
 * are allowed to move at any time - for each we check if we reached any
 * AvoidCover gridline  - and if yes - we stop, and also we check if we
 * reached attracting distance of any other gridline, and if so  - we
 * snap to it.
 *
 * When we are moving - things get complicated, since we actually need to
 * move past AvoidCover windows if user insistently drags the mouse over
 * it. We are looking for point with x*x+y*y < dX*dX+dY*dY in one of the
 * 4 possible quadrants( depending on move direction ). Then we nee to
 * find such point that has lesser (x-Px)*(x-Px)+(y-Py)*(y-Py) where
 * Px,Py are current position of pointer. This point should also be placed
 * so that all the gridlines with negative gravity are avoided.
 *
 * SEE ALSO
 * Structures :
 * Functions :
 * Other libAfterWidget modules :
 * AUTHOR
 * Sasha Vasko <sasha at aftercode dot net>
 ******
 */

struct ASEvent;
struct ASOutlineSegment;
struct ASMoveResizeData;
struct ASGrid;
//struct ASWidget;

typedef	void (*as_interactive_pointer_handler)  (struct ASMoveResizeData *data, int x, int y);
typedef	void (*as_interactive_apply_handler)    (struct ASMoveResizeData *data);
typedef void (*as_interactive_subwindow_handler)(struct ASMoveResizeData *data, struct ASEvent *event);
typedef	void (*as_interactive_complete_handler) (struct ASMoveResizeData *data, Bool cancelled);
typedef int  (*as_outline_handler)              (struct ASOutlineSegment *s,
	                                             XRectangle *geom,
	                                             unsigned int scr_width,
									             unsigned int scr_height);

typedef struct ASOutlineSegment
{
	Window w ;
	int x, y;
	int size ;
	Bool vertical ;
}ASOutlineSegment;

typedef struct ASMoveResizeData
{
	/* mandatrory things : */
    struct ASWidget *parent;
    struct ASWidget *mr;
	struct ASFeel   *feel ;
	struct MyLook   *look ;
	as_interactive_apply_handler    apply_func;
	as_interactive_complete_handler complete_func;
	/* what leg ... err, side are we pulling on ??? ( see FR_* values) */
	int side ;

	/* Internally managed things : */
	as_interactive_pointer_handler  pointer_func;

	char 	   	    *geometry_string;
	Window           geometry_display;
	XRectangle 		 curr, last, start;

	int 			 origin_x, origin_y ;       /* parent's window root
												* coordinates */
	int 			 last_x, last_y ;
	int 			 lag_x, lag_y ;

	ASOutlineSegment *outline;
	unsigned int	 pointer_state ;
	Time			 pointer_grab_time;
	Window           curr_subwindow;

	/* Optional things : */
	as_interactive_subwindow_handler subwindow_func;
	Window           below_sibling ;          /* outline will be placed
											   * just below this sibling */
	struct ASGrid   *grid ;
	/* optional size constraints : */
	unsigned int min_width,  width_inc,  max_width;
	unsigned int min_height, height_inc, max_height;
}ASMoveResizeData;

ASOutlineSegment *make_outline_segments( struct ASWidget *parent, struct MyLook *look );
void move_outline( ASMoveResizeData * data );
void destroy_outline_segments( ASOutlineSegment **psegments );

ASMoveResizeData *
move_widget_interactively(struct ASWidget *parent,
                          struct ASWidget *mr,
						  struct ASEvent *trigger,
  						  as_interactive_apply_handler    apply_func,
						  as_interactive_complete_handler complete_func );
ASMoveResizeData*
resize_widget_interactively( struct ASWidget *parent,
							 struct ASWidget *mr,
							 struct ASEvent *trigger,
	  						 as_interactive_apply_handler    apply_func,
							 as_interactive_complete_handler complete_func,
							 int side );

#endif
