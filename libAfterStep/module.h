#ifndef MODULE_H
#define MODULE_H

#include "screen.h"

#ifdef __cplusplus
extern "C" {
#endif


/*************************************************************************
 * Standard Module types :
 *************************************************************************/
#define AS_MODULE_CLASS		"ASModule"         /* for WM_CLASS_HINT only */

#define CLASS_AFTERSTEP     "AfterStep"
#define CLASS_ANIMATE       "Animate"
#define CLASS_ARRANGE       "Arrange"
#define CLASS_ASRENDER      "ASRenderer"
#define CLASS_ASCP          "ASCP"
#define CLASS_ASCOLOR       "ASColor"
#define CLASS_ASIMBROWSER   "ASIMBrowser"
#define CLASS_ASETROOT		"asetroot"
#define CLASS_AUDIO			"Audio"
#define CLASS_BANNER		"Banner"
#define CLASS_CLEAN         "Clean"
#define CLASS_GNOME         "Gnome"
#define CLASS_IDENT			"Ident"
#define CLASS_PAGER         "Pager"
#define CLASS_SCROLL        "Scroll"
#define CLASS_WHARF         "Wharf"
#define CLASS_WINLIST       "WinList"

#define CLASS_CUSTOM		"ASCustom"


#define START_FLAG 0xffffffff

#define M_TOGGLE_PAGING      (1<<0)
#define M_NEW_DESKVIEWPORT   (1<<1)
#define M_ADD_WINDOW         (1<<2)
#define M_CONFIGURE_WINDOW   (1<<3)
#define M_MAP                (1<<4)
#define M_FOCUS_CHANGE       (1<<5)
#define M_DESTROY_WINDOW     (1<<6)
#define M_WINDOW_NAME        (1<<7)
#define M_ICON_NAME          (1<<8)
#define M_RES_CLASS          (1<<9)
#define M_RES_NAME           (1<<10)
#define M_END_WINDOWLIST     (1<<11)
#define M_STACKING_ORDER     (1<<12)
#define M_LOCKONSEND         (1<<13)
#define M_NEW_BACKGROUND     (1<<14)
#define M_NEW_THEME          (1<<15)

#define MAX_MESSAGES          15
#define MAX_MASK             (((1<<MAX_MESSAGES)-1) & ~M_LOCKONSEND)

#define WAIT_AS_RESPONSE_TIMEOUT    20   /* seconds */

/* M_LOCKONSEND when set causes afterstep to wait for the module to send an
 * unlock message back, needless to say, we wouldn't want this on by default
 */

#define HEADER_SIZE         3
#define MSG_HEADER_SIZE     HEADER_SIZE
#define MAX_PACKET_SIZE    27
#define MAX_BODY_SIZE      (MAX_PACKET_SIZE - HEADER_SIZE)

/* from lib/module.c */
int module_connect (const char *socket_name);
char *module_get_socket_property (Window w);

/* from lib/readpacket.c */
int ReadASPacket (int, unsigned long *, unsigned long **);

typedef struct
  {
    unsigned long header[3];
    unsigned long *body;
  }
ASMessage;

/* from lib/module.c */
/* checks if there is message from Afterstep in incoming pipe,
 * and reads it if it is . Returns NULL if there is nothing available yet
 */
ASMessage *CheckASMessageFine ( int t_sec, int t_usec);
#define CheckASMessage(t_sec) CheckASMessageFine(t_sec,0)
void DestroyASMessage (ASMessage * msg);
void module_wait_pipes_input ( void (*as_msg_handler) (unsigned long type, unsigned long *body) );

/* returns fd of the AfterStep connection */
int ConnectAfterStep (unsigned long message_mask);
void SendInfo (char *message, unsigned long window);
void SendNumCommand ( int func, const char *name, const long *func_val, const long *unit_val, unsigned long window);
void SendTextCommand ( int func, const char *name, const char *text, unsigned long window);
void SendCommand( FunctionData * pfunc, unsigned long window);




/* constructs config filename and calls supplied user function */
void LoadBaseConfig (void (*read_base_options_func) (const char *));
void LoadConfig (char *config_file_name, void (*read_options_func) (const char *));
#ifdef __cplusplus
}
#endif


#endif /* MODULE_H */
