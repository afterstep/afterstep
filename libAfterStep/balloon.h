#ifndef BALLOON_H
#define BALLOON_H

/*
 * To use:
 * 1. set extern globals (MyName, dpy, screen)
 * 2. parse config file, calling balloon_parse() at each line
 * 3. call balloon_setup()
 * 4. call balloon_set_style()
 * 5. call balloon_handle_event() inside event loop
 *
 * Notes:
 *  o EnterNotify and LeaveNotify must be selected for parent windows
 *  o if active rectangles are used, MotionNotify should be selected for 
 *    parent windows
 *  o balloons may be created (with balloon_new()) at any point
 *  o unless precise control of balloon position is required, 
 *    set_position() should not be used
 */

typedef struct Balloon
  {
    struct Balloon *next;	/* next balloon in the list */
    Display *dpy;		/* associated display */
    Window parent;		/* window balloon should pop up over */
    char *text;			/* text to display in balloon */
    int px, py, pwidth, pheight;	/* rectangle in parent window that triggers this balloon */
    int x, y;			/* position of this rectangle */
    int timer_action;		/* what to do when the timer expires */
  }
Balloon;

enum
  {
    BALLOON_TIMER_OPEN,
    BALLOON_TIMER_CLOSE
  };

extern Bool balloon_show;

Bool balloon_parse (char *tline, FILE * fd);
void balloon_setup (Display * dpy);
void balloon_init (int free_resources);
Balloon *balloon_new (Display * dpy, Window parent);
Balloon *balloon_new_with_text (Display * dpy, Window parent, char *text);
Balloon *balloon_find (Window parent);
void balloon_delete (Balloon * balloon);
void balloon_set_style (Display * dpy, MyStyle * style);
void balloon_set_text (Balloon * balloon, const char *text);
void balloon_set_active_rectangle (Balloon * balloon, int x, int y, int width, int height);
void balloon_set_position (Balloon * balloon, int x, int y);
Bool balloon_handle_event (XEvent * event);

#endif /* BALLOON_H */
