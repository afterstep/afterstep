#ifndef MODULE_H
#define MODULE_H

#include "screen.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ASTBarProps;
struct button_t;

/*************************************************************************
 * Standard Module types :
 *************************************************************************/
#define AS_MODULE_CLASS		"ASModule"         /* for WM_CLASS_HINT only */

#define CLASS_AFTERSTEP     "AfterStep"
#define CLASS_ANIMATE       "Animate"
#define CLASS_ARRANGE       "Arrange"
#define CLASS_ASRENDER      "ASRenderer"
#define CLASS_ASDOCGEN		"ASDocGen"
#define CLASS_ASCONFIG		"ASConfig"
#define CLASS_ASCP          "ASCP"
#define CLASS_ASCOLOR       "ASColor"
#define CLASS_ASIMBROWSER   "ASIMBrowser"
#define CLASS_ASWALLPAPER   "ASWallpaper"
#define CLASS_ASETROOT		"asetroot"
#define CLASS_SOUND			"Sound"
#define CLASS_BANNER		"Banner"
#define CLASS_CLEAN         "Clean"
#define CLASS_GNOME         "Gnome"
#define CLASS_IDENT			"Ident"
#define CLASS_PAGER         "Pager"
#define CLASS_SCROLL        "Scroll"
#define CLASS_WHARF         "Wharf"
#define CLASS_WHARF_WITHDRAWN         "WharfWithdrawn"
#define CLASS_WINCOMMAND    "WinCommand"
#define CLASS_WINLIST       "WinList"
#define CLASS_WINTABS       "WinTabs"
#define CLASS_TEST		    "Test"
#define CLASS_GADGET		"Gadget"

#define CLASS_CUSTOM		"ASCustom"
#define CLASS_ASRUN			"ASRun"

#define BASE_CONFIG     	(0x01<<0)
#define LOOK_CONFIG     	(0x01<<1)
#define FEEL_CONFIG     	(0x01<<2)
#define DATABASE_CONFIG 	(0x01<<3)
#define AUTOEXEC_CONFIG 	(0x01<<4)
#define COLORSCHEME_CONFIG 	(0x01<<5)
#define THEME_CONFIG 		(0x01<<6)

#define START_FLAG 0xffffffff

#define M_TOGGLE_PAGING      (1<<0)
#define M_NEW_DESKVIEWPORT   (1<<1)
#define M_ADD_WINDOW         (1<<2)
#define M_CONFIGURE_WINDOW   (1<<3)
#define M_STATUS_CHANGE		 (1<<4)
#define M_MAP                (1<<5)
#define M_FOCUS_CHANGE       (1<<6)
#define M_DESTROY_WINDOW     (1<<7)
#define M_WINDOW_NAME        (1<<8)
#define M_WINDOW_NAME_MATCHED (1<<9)
#define M_ICON_NAME          (1<<10)
#define M_RES_CLASS          (1<<11)
#define M_RES_NAME           (1<<12)
#define M_END_WINDOWLIST     (1<<13)
#define M_STACKING_ORDER     (1<<14)
#define M_NEW_BACKGROUND     (1<<15)
#define M_NEW_CONFIG         (1<<16)
#define M_NEW_MODULE_CONFIG	 (1<<17)
#define M_PLAY_SOUND		 (1<<18)
#define M_SWALLOW_WINDOW	 (1<<19)
#define M_SHUTDOWN			 (1<<20)

//#define M_LOCKONSEND         (1<<19)


#define MAX_MESSAGES          21
#define MAX_MASK             ((1<<MAX_MESSAGES)-1)

#define WAIT_AS_RESPONSE_TIMEOUT    20   /* seconds */

/* M_LOCKONSEND when set causes afterstep to wait for the module to send an
 * unlock message back, needless to say, we wouldn't want this on by default
 */

typedef CARD32 send_data_type;
typedef CARD32 send_ID_type;
typedef INT32 send_signed_data_type;

#define HEADER_SIZE         3
#define MSG_HEADER_SIZE     HEADER_SIZE
#define MAX_PACKET_SIZE    27
#define MAX_BODY_SIZE      (MAX_PACKET_SIZE - HEADER_SIZE)

/* from lib/module.c */
int module_connect (const char *socket_name);
char *module_get_socket_property (Window w);

/* from lib/readpacket.c */
int ReadASPacket (int, send_data_type *, send_data_type **);

typedef struct
  {
    send_data_type header[3];
    send_data_type *body;
  }
ASMessage;

/* from lib/module.c */
/* checks if there is message from Afterstep in incoming pipe,
 * and reads it if it is . Returns NULL if there is nothing available yet
 */
ASMessage *CheckASMessageFine ( int t_sec, int t_usec);
#define CheckASMessage(t_sec) CheckASMessageFine(t_sec,0)
void DestroyASMessage (ASMessage * msg);
void module_wait_pipes_input ( void (*as_msg_handler) (send_data_type type, send_data_type *body) );

/* returns fd of the AfterStep connection */
int ConnectAfterStep (send_data_type message_mask, send_data_type lock_on_send_mask);
void SetAfterStepDisconnected();

int get_module_out_fd();
int get_module_in_fd();

void SendInfo (char *message, send_ID_type window);
void SendNumCommand ( int func, const char *name, const send_signed_data_type *func_val, const send_signed_data_type *unit_val, send_ID_type window);
void SendTextCommand ( int func, const char *name, const char *text, send_ID_type window);
void SendCommand( FunctionData * pfunc, send_ID_type window);




/* constructs config filename and calls supplied user function */
void LoadBaseConfig (void (*read_base_options_func) (const char *));
void LoadConfig (char *config_file_name, void (*read_options_func) (const char *));

void 
button_from_astbar_props( struct ASTBarProps *tbar_props, struct button_t *button, 
						  int context, Atom kind, Atom kind_pressed );
void destroy_astbar_props( struct ASTBarProps **props );


#ifdef __cplusplus
}
#endif


#endif /* MODULE_H */
