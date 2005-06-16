#ifndef AS_COMMAND_OPS_HEADER_INCLUDED
#define AS_COMMAND_OPS_HEADER_INCLUDED

#include "../configure.h"
#include "asapp.h"

#include "../libAfterImage/afterimage.h"

#include "afterstep.h"
#include "screen.h"
#include "module.h"
#include "mystyle.h"
#include "mystyle_property.h"
#include "parser.h"
#include "clientprops.h"
#include "wmprops.h"
#include "decor.h"
#include "aswindata.h"
#include "balloon.h"
#include "event.h"
#include "shape.h"

#include "../libAfterBase/aslist.h"
#include "../libAfterBase/ashash.h"


void move_handler(ASWindowData *wd, void *data);
void resize_handler(ASWindowData *wd, void *data);
void kill_handler(ASWindowData *wd, void *data);
void jump_handler(ASWindowData *wd, void *data);
void ls_handler(ASWindowData *wd, void *data);
void iconify_handler(ASWindowData *wd, void *data);
void send_to_desk_handler(ASWindowData *wd, void *data);
void center_handler(ASWindowData *wd, void *data);

typedef struct
{
	int x, y;

} move_params;

typedef struct
{
	int width, height;

} resize_params;

typedef struct
{
	int desk;
} send_to_desk_params;

#endif
