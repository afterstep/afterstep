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


#include "../../configure.h"
#define NEED_BALLOON

#include "../../include/asapp.h"
#include "../../include/afterstep.h"
#include "../../include/mystyle.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"
#include "../../include/balloon.h"

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
  if (config->border_color)
    free (config->border_color);
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
	fprintf( stderr,"BalloonBorderColor [%s]\n",config->border_color);
	fprintf( stderr,"BalloonBorderWidth %u\n",config->border_width);
	fprintf( stderr,"BalloonYOffset %d\n",config->y_offset);
	fprintf( stderr,"BalloonDelay %d\n",config->delay);
	fprintf( stderr,"BalloonCloseDelay %d\n",config->close_delay);
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
      if (options->term->id < BALLOON_ID_START
	  || options->term->id > BALLOON_ID_END)
	continue;

      if (options->term == NULL)
	continue;
      if (options->term->type == TT_FLAG)
	{
	  switch (options->term->id)
	    {
	    case BALLOON_USED_ID:
	      config->set_flags |= BALLOON_USED ;
	      break;
	    }
	  continue;
	}

      if (!ReadConfigItem (&item, options))
	continue;

      item.ok_to_free = 1;
      switch (options->term->id)
	{
	case BALLOON_BorderColor_ID:
	  set_string_value( &(config->border_color), item.data.string,
			    &(config->set_flags),BALLOON_COLOR);

	  item.ok_to_free = 0;
	  break;
	case BALLOON_BorderWidth_ID :
	  config->set_flags |= BALLOON_WIDTH ;
	  config->border_width = item.data.integer;
	  break;
	case BALLOON_YOffset_ID :
	  config->set_flags |= BALLOON_OFFSET ;
	  config->y_offset = item.data.integer;
	  break;
	case BALLOON_Delay_ID :
	  config->set_flags |= BALLOON_DELAY ;
	  config->delay = item.data.integer;
	  break;
	case BALLOON_CloseDelay_ID :
	  config->set_flags |= BALLOON_CLOSE_DELAY ;
	  config->close_delay = item.data.integer;
	  break;
	default:
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


