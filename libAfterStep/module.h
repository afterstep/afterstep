#ifndef MODULE_H
#define MODULE_H

#include "screen.h"

struct queue_buff_struct
  {
    struct queue_buff_struct *next;
    unsigned long *data;
    int size;
    int done;
  };

typedef struct module_ibuf_t
  {
    int len;
    int done;
    int window;
    int size;
    char *text;
    int cont;
  }
module_ibuf_t;

typedef struct module_t
  {
    int fd;
    int active;
    char *name;
    long mask;
    struct queue_buff_struct *output_queue;
    module_ibuf_t ibuf;
  }
module_t;

extern int module_fd;
extern int npipes;
extern module_t *Module;
extern char *display_name ;

#define START_FLAG 0xffffffff

#define M_TOGGLE_PAGING      (1<<0)
#define M_NEW_PAGE           (1<<1)
#define M_NEW_DESK           (1<<2)
#define M_ADD_WINDOW         (1<<3)
#define M_RAISE_WINDOW       (1<<4)
#define M_LOWER_WINDOW       (1<<5)
#define M_CONFIGURE_WINDOW   (1<<6)
#define M_FOCUS_CHANGE       (1<<7)
#define M_DESTROY_WINDOW     (1<<8)
#define M_ICONIFY            (1<<9)
#define M_DEICONIFY          (1<<10)
#define M_WINDOW_NAME        (1<<11)
#define M_ICON_NAME          (1<<12)
#define M_RES_CLASS          (1<<13)
#define M_RES_NAME           (1<<14)
#define M_END_WINDOWLIST     (1<<15)
#define M_ICON_LOCATION      (1<<16)
#define M_MAP                (1<<17)
#define M_SHADE		     (1<<18)
#define M_UNSHADE	     (1<<19)
#define M_LOCKONSEND         (1<<20)
#define M_NEW_BACKGROUND     (1<<21)
#define M_NEW_THEME		     (1<<22)

#define MAX_MESSAGES          23
#define MAX_MASK             (((1<<MAX_MESSAGES)-1) & ~M_LOCKONSEND)

/* M_LOCKONSEND when set causes afterstep to wait for the module to send an
 * unlock message back, needless to say, we wouldn't want this on by default
 */

#define HEADER_SIZE         3
#define MAX_PACKET_SIZE    27
#define MAX_BODY_SIZE      (MAX_PACKET_SIZE - HEADER_SIZE)

/* from src/afterstep/module.c */
int module_setup_socket (Window w, const char *display_string);
int module_listen (const char *socket_name);
int module_accept (int socket_fd);
void module_init (int free_resources);
void module_setup (void);
int HandleModuleInput (int channel);

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
ASMessage *CheckASMessageFine (int fd, int t_sec, int t_usec);
#define CheckASMessage(fd,t_sec) CheckASMessageFine(fd,t_sec,0)
void DestroyASMessage (ASMessage * msg);

/* more usefull stuff removing duplicate code from modules */

typedef struct as_pipes
  {
    int fd[2];
    int fd_width;
    int x_fd;
  }
ASPipes;

typedef struct as_atom
  {
    Atom atom;
    char *name;
    Atom type;
  }
ASAtom;

/* sets MyName global variable from supplied argv[0] */
void SetMyName (char *argv0);

void default_version_func (void);
void (*custom_version_func) (void);
int ProcessModuleArgs (int argc, char **argv, char **global_config_file, unsigned long *app_window, unsigned long *app_context, void (*custom_usage_func) (void));

/* returns fd of the AfterStep connection */
int ConnectAfterStep (unsigned long message_mask);

/* constructs config filename and calls supplied user function */
void LoadBaseConfig (char *global_config_file, void (*read_base_options_func) (const char *));
void LoadConfig (char *global_config_file, char *config_file_name, void (*read_options_func) (const char *));

/* only aplicable to modules with X connection : lib/Xmodule.c */
#ifdef MODULE_X_INTERFACE
/* returns fd of the X server connection */
void get_Xinerama_rectangles(ScreenInfo *scr);
int ConnectX (ScreenInfo * scr, char *display_name, unsigned long message_mask);
/* get's Atom ID's of the atoms names listed in array */
void InitAtoms (Display * dpy, ASAtom * atoms);
/* X enabled version of RunModule */
int My_XNextEvent (Display * dpy, int x_fd, int as_fd, void (*as_msg_handler) (unsigned long type, unsigned long *body), XEvent * event);
#endif

#endif /* MODULE_H */
