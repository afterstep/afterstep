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
	
	int ref_count ; 
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
	enum
	{
		ASBalloon_Text =0,
		ASBalloon_Image,
	} type;
	
	union
	{
		struct {
			char *text;                     /* text to display in balloon */
			unsigned long encoding ;
		}text;
		struct {
			char *filename;
			ASImage *image;
		}image;
	}data;
	
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
	ASBalloonLook *look ;
	ASBalloon     *active ;
	struct ASCanvas      *active_canvas;
	struct ASTBarData    *active_bar ;
	Window         active_window ;
	
	struct ASBalloonState *next ; 
}ASBalloonState;

ASBalloonState *is_balloon_click( XEvent *xe );
/* void balloon_init (int free_resources); */
ASBalloonState *create_balloon_state();
void destroy_balloon_state(ASBalloonState **pstate);
void cleanup_default_balloons();

void withdraw_balloon( ASBalloon *balloon );
void withdraw_active_balloon_from( ASBalloonState *state );
void withdraw_active_balloon();
void display_balloon( ASBalloon *balloon );
void display_balloon_nodelay( ASBalloon *balloon );

ASBalloonLook* create_balloon_look();
int destroy_balloon_look( ASBalloonLook *blook );
void set_balloon_look( ASBalloonLook *blook );
void set_balloon_state_look( ASBalloonState *state, ASBalloonLook *blook );

ASBalloon *create_asballoon_for_state (ASBalloonState *state, struct ASTBarData *owner);
ASBalloon *create_asballoon_with_text_for_state ( ASBalloonState *state, struct ASTBarData *owner, const char *text, unsigned long encoding);
ASBalloon *create_asballoon (struct ASTBarData *owner);
ASBalloon *create_asballoon_with_text ( struct ASTBarData *owner, const char *text, unsigned long encoding);
void destroy_asballoon(ASBalloon **pballoon );
void balloon_set_text (ASBalloon * balloon, const char *text, unsigned long encoding);
void balloon_set_image_from_file (ASBalloon * balloon, const char *filename);
void balloon_set_image (ASBalloon * balloon, ASImage *im);

#ifdef __cplusplus
}
#endif


#endif /* BALLOON_H */
