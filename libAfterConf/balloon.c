/*
 * Copyright (c) 2000 Andrew Ferguson <andrew@owsla.cjb.net>
 * Copyright (c) 1998 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#include "../configure.h"
#define NEED_BALLOON
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/mystyle.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/balloon.h"
#include "../libAfterStep/mylook.h"

#include "afterconf.h"


/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the base.<bpp> config
 * file
 *
 ****************************************************************************/

balloonConfig *
Create_balloonConfig ()
{
  balloonConfig *config = (balloonConfig *) safemalloc (sizeof (balloonConfig));
  /* let's initialize Base config with some nice values: */
  memset( config, 0x00, sizeof (balloonConfig));

  return config;
}

void
Destroy_balloonConfig (balloonConfig * config)
{
  if (!config)
	return;
  if (config->style)
    free (config->style);
  free (config);
  config = NULL;
}

void
Print_balloonConfig (balloonConfig *config )
{
    if( config == NULL )
        fprintf( stderr, "No balloon configuration available \n");
    else
    {
        fprintf( stderr,"set_flags = 0x%lX\n",config->set_flags);
        fprintf( stderr,"BalloonBorderHilite 0x%lX\n",config->border_hilite);
        fprintf( stderr,"BalloonXOffset %d\n",config->x_offset);
        fprintf( stderr,"BalloonYOffset %d\n",config->y_offset);
        fprintf( stderr,"BalloonDelay %d\n",config->delay);
        fprintf( stderr,"BalloonCloseDelay %d\n",config->close_delay);
        fprintf( stderr,"BalloonStyle \"%s\"\n",config->style);
    }
}

void
balloon_config2look( MyLook *look, balloonConfig *config )
{
    if( look )
    {
        if( look->balloon_look == NULL )
            look->balloon_look = safecalloc( 1, sizeof(ASBalloonLook) );

        if( config == NULL )
            memset( look->balloon_look, 0x00, sizeof(ASBalloonLook) );
        else
        {
            look->balloon_look->show = get_flags( config->set_flags, BALLOON_USED );
            look->balloon_look->border_hilite = config->border_hilite ;
            look->balloon_look->xoffset = config->x_offset ;
            look->balloon_look->yoffset = config->y_offset ;
            look->balloon_look->delay = config->delay ;
            look->balloon_look->close_delay = config->close_delay ;
            look->balloon_look->style = mystyle_list_find_or_default (look->styles_list, config->style);
        }
    }
}

balloonConfig*
Process_balloonOptions (FreeStorageElem * options, balloonConfig *config)
{
    ConfigItem item;
    item.memory = NULL;

    if( config == NULL )
        config = Create_balloonConfig();

    for (; options; options = options->next)
    {
        if (options->term->id < BALLOON_ID_START || options->term->id > BALLOON_ID_END )
            continue;

        if (options->term == NULL)
            continue;
        if (options->term->type == TT_FLAG)
        {
            if(options->term->id == BALLOON_USED_ID )
                config->set_flags |= BALLOON_USED ;
            else if( options->term->id ==  BALLOON_BorderHilite_ID )
            {
                set_flags( config->set_flags, BALLOON_HILITE );
                config->border_hilite = ParseBevelOptions( options->sub );
            }
            continue;
        }

        if (!ReadConfigItem (&item, options))
            continue;

        switch (options->term->id)
        {
            case BALLOON_XOffset_ID :
                set_flags(config->set_flags, BALLOON_XOFFSET );
                config->x_offset = item.data.integer;
                break;
            case BALLOON_YOffset_ID :
                set_flags(config->set_flags, BALLOON_YOFFSET );
                config->y_offset = item.data.integer;
                break;
            case BALLOON_Delay_ID :
                set_flags(config->set_flags, BALLOON_DELAY );
                config->delay = item.data.integer;
                break;
            case BALLOON_CloseDelay_ID :
                set_flags(config->set_flags, BALLOON_CLOSE_DELAY );
                config->close_delay = item.data.integer;
                break;
            case BALLOON_Style_ID :
                set_string_value( &(config->style), item.data.string, &(config->set_flags), BALLOON_STYLE );
                break;
            default:
                item.ok_to_free = 1;
        }
    }
    ReadConfigItem (&item, NULL);
    return config;
}

#if 0

/* returns:
 * 	tail
 *
 */
FreeStorageElem **
balloon2FreeStorage( SyntaxDef *syntax, FreeStorageElem ** tail, balloonConfig *config)
{
    if( config == NULL || tail == NULL ) return tail;

    /* adding balloon free storage here */
    if( config->set_flags == 0 ) return tail ; /* nothing to write */

    tail = ADD_SET_FLAG( syntax, tail, config->set_flags, BALLOON_USED, BALLOON_USED_ID );
    /* border color */
    if( config->set_flags & BALLOON_COLOR )
	tail = String2FreeStorage( syntax, tail, config->border_color, BALLOON_BorderColor_ID ) ;
     /* border width */
    if( config->set_flags & BALLOON_WIDTH )
        tail = Integer2FreeStorage( syntax, tail, NULL, config->border_width, BALLOON_BorderWidth_ID ) ;
     /*  */
    if( config->set_flags & BALLOON_OFFSET )
        tail = Integer2FreeStorage( syntax, tail, NULL, config->y_offset, BALLOON_YOffset_ID ) ;
     /*  */
    if( config->set_flags & BALLOON_DELAY )
        tail = Integer2FreeStorage( syntax, tail, NULL, config->delay, BALLOON_Delay_ID ) ;
     /*  */
    if( config->set_flags & BALLOON_CLOSE_DELAY )
        tail = Integer2FreeStorage( syntax, tail, NULL, config->close_delay, BALLOON_CloseDelay_ID ) ;

    return tail ;
}

void
BalloonConfig2BalloonLook(BalloonLook *blook, struct balloonConfig *config)
{
	if( config && blook )
	{
		blook->show = (config->set_flags & BALLOON_USED);
		/* border color */
		if (config->set_flags & BALLOON_COLOR)
		{
			blook->border_color = 0 ;
			parse_argb_color( config->border_color, &(blook->border_color));
		}
		/* border width */
		if (config->set_flags & BALLOON_WIDTH)
			blook->border_width = config->border_width;
		/*  */
		if (config->set_flags & BALLOON_OFFSET)
			blook->yoffset = config->y_offset;
		/*  */
		if (config->set_flags & BALLOON_DELAY)
			blook->delay = config->delay;
		/*  */
		if (config->set_flags & BALLOON_CLOSE_DELAY)
			blook->close_delay = config->close_delay;
	}
}

#endif


