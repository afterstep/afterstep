#ifndef BALLOON_H
#define BALLOON_H

#ifdef __cplusplus
extern "C" {
#endif


struct MyStyle;

struct MyStyle;
struct ASTBarData;
struct ASCanvas;

typedef struct ASBalloonLook
{
  Bool show;
  int border_hilite;
  int yoffset, xoffset;
  int delay;
  int close_delay;
  struct MyStyle *style;

}ASBalloonLook;

typedef struct ASBalloon
{
    char *text;                     /* text to display in balloon */
	unsigned long encoding ;
	enum
    {
        BALLOON_TIMER_OPEN,
        BALLOON_TIMER_CLOSE
    } timer_action;               /* what to do when the timer expires */
    struct ASTBarData *owner ;
}ASBalloon;

typedef struct ASBalloonState
{
    ASBalloonLook  look ;
    ASBalloon     *active ;
    struct ASCanvas      *active_canvas;
    struct ASTBarData    *active_bar ;
    Window         active_window ;
}ASBalloonState;

void balloon_init (int free_resources);
void withdraw_balloon( ASBalloon *balloon );
void withdraw_active_balloon();
void display_balloon( ASBalloon *balloon );
void set_balloon_look( ASBalloonLook *blook );
ASBalloon *create_asballoon (struct ASTBarData *owner);
ASBalloon *create_asballoon_with_text ( struct ASTBarData *owner, const char *text, unsigned long encoding);
void destroy_asballoon(ASBalloon **pballoon );
void balloon_set_text (ASBalloon * balloon, const char *text, unsigned long encoding);

#ifdef __cplusplus
}
#endif


#endif /* BALLOON_H */
