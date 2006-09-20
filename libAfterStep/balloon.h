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
  int BorderHilite;
  int YOffset, XOffset;
  int Delay;
  int CloseDelay;
  struct MyStyle *Style;
  int TextPaddingX, TextPaddingY;

}ASBalloonLook;

struct ASBalloonState;

typedef struct ASBalloon
{
    char *text;                     /* text to display in balloon */
	unsigned long encoding ;
	enum
    {
        BALLOON_TIMER_OPEN,
        BALLOON_TIMER_CLOSE
    } timer_action;               /* what to do when the timer expires */
    struct ASTBarData 		*owner ;
	struct ASBalloonState 	*state ;
}ASBalloon;

typedef struct ASBalloonState
{
    ASBalloonLook  look ;
    ASBalloon     *active ;
    struct ASCanvas      *active_canvas;
    struct ASTBarData    *active_bar ;
    Window         active_window ;
	
	struct ASBalloonState *next ; 
}ASBalloonState;

Bool is_balloon_click( XEvent *xe );
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
