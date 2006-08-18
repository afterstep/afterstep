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
#define LOCAL_DEBUG
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
  balloonConfig *config = (balloonConfig *) safecalloc (1, sizeof (balloonConfig));
  return config;
}

void
Destroy_balloonConfig (balloonConfig * config)
{
  if (!config)
	return;
  if (config->Style)
    free (config->Style);
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
        fprintf( stderr,"flags = 0x%lX\n",config->flags);
		ASCF_PRINT_FLAGS_KEYWORD(stderr,BALLOON,config,BorderHilite);
		ASCF_PRINT_INT_KEYWORD(stderr,BALLOON,config,XOffset);
		ASCF_PRINT_INT_KEYWORD(stderr,BALLOON,config,YOffset);
		ASCF_PRINT_INT_KEYWORD(stderr,BALLOON,config,Delay);
		ASCF_PRINT_INT_KEYWORD(stderr,BALLOON,config,CloseDelay);
		ASCF_PRINT_STRING_KEYWORD(stderr,BALLOON,config,Style);
		ASCF_PRINT_INT_KEYWORD(stderr,BALLOON,config,TextPaddingX);
		ASCF_PRINT_INT_KEYWORD(stderr,BALLOON,config,TextPaddingY);
	}
}

void
balloon_config2look( MyLook *look, balloonConfig *config, const char *default_style )
{
    if( look )
    {
        if( look->balloon_look == NULL )
            look->balloon_look = safecalloc( 1, sizeof(ASBalloonLook) );

        if( config == NULL )
		{	
            memset( look->balloon_look, 0x00, sizeof(ASBalloonLook) );
			look->balloon_look->show = True ;
        	look->balloon_look->XOffset = 5 ;
        	look->balloon_look->YOffset = 5 ;
			look->balloon_look->Style = mystyle_list_find_or_default (look->styles_list, default_style);
			look->balloon_look->Delay = 200 ;
			look->balloon_look->CloseDelay = 2000 ;
		}else
        {
#define MERGE_BALLOON_SCALAR_VAL(val)  look->balloon_look->val = config->val 			

            look->balloon_look->show = get_flags( config->flags, BALLOON_USED );
			MERGE_BALLOON_SCALAR_VAL(BorderHilite);
			MERGE_BALLOON_SCALAR_VAL(XOffset);
			MERGE_BALLOON_SCALAR_VAL(YOffset);
			MERGE_BALLOON_SCALAR_VAL(Delay);
			MERGE_BALLOON_SCALAR_VAL(CloseDelay);
            look->balloon_look->Style = mystyle_list_find_or_default (look->styles_list, config->Style?config->Style:default_style);
			MERGE_BALLOON_SCALAR_VAL(TextPaddingX);
			MERGE_BALLOON_SCALAR_VAL(TextPaddingY);
		}
	    LOCAL_DEBUG_OUT( "balloon mystyle = %p (\"%s\")", look->balloon_look->Style,
                    	 look->balloon_look->Style?look->balloon_look->Style->name:"none" );

    }
}

void
set_default_balloon_style( balloonConfig *config, const char *style )
{	 
    if( !get_flags( config->set_flags, BALLOON_Style ) )
    {
        config->Style = mystrdup(style);
        set_flags( config->set_flags, BALLOON_Style );
    }
}

flag_options_xref BalloonsFlags[] = {
	ASCF_DEFINE_MODULE_ONOFF_FLAG_XREF(BALLOON,Balloons,balloonConfig),
    {0, 0, 0}
};


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
			if( !ReadFlagItemAuto (config, 0, options, &BalloonsFlags[0]) )
	            switch(options->term->id )
				{
		            ASCF_HANDLE_BEVEL_KEYWORD_CASE(BALLOON,config,options,BorderHilite); 
				}
            continue;
        }

        if (!ReadConfigItem (&item, options))
            continue;

        switch (options->term->id)
        {
			ASCF_HANDLE_INTEGER_KEYWORD_CASE(BALLOON,config,item,XOffset); 
			ASCF_HANDLE_INTEGER_KEYWORD_CASE(BALLOON,config,item,YOffset); 
			ASCF_HANDLE_INTEGER_KEYWORD_CASE(BALLOON,config,item,Delay); 
			ASCF_HANDLE_INTEGER_KEYWORD_CASE(BALLOON,config,item,CloseDelay); 
			ASCF_HANDLE_STRING_KEYWORD_CASE(BALLOON,config,item,Style); 
			ASCF_HANDLE_INTEGER_KEYWORD_CASE(BALLOON,config,item,TextPaddingX); 
			ASCF_HANDLE_INTEGER_KEYWORD_CASE(BALLOON,config,item,TextPaddingY); 
            default:
                item.ok_to_free = 1;
        }
    }
    ReadConfigItem (&item, NULL);
    return config;
}

void
MergeBalloonOptions ( ASModuleConfig *asm_to, ASModuleConfig *asm_from)
{
	if( asm_to && asm_from ) 
	{
		if( asm_from->balloon_conf != NULL ) 
		{

			if( asm_to->balloon_conf == NULL )
		   		asm_to->balloon_conf = Create_balloonConfig ();

		    ASCF_MERGE_FLAGS(asm_to->balloon_conf,asm_from->balloon_conf);
			ASCF_MERGE_SCALAR_KEYWORD(BALLOON, asm_to->balloon_conf, asm_from->balloon_conf, BorderHilite);
			ASCF_MERGE_SCALAR_KEYWORD(BALLOON, asm_to->balloon_conf, asm_from->balloon_conf, XOffset); 
			ASCF_MERGE_SCALAR_KEYWORD(BALLOON, asm_to->balloon_conf, asm_from->balloon_conf, YOffset); 
			ASCF_MERGE_SCALAR_KEYWORD(BALLOON, asm_to->balloon_conf, asm_from->balloon_conf, Delay); 
			ASCF_MERGE_SCALAR_KEYWORD(BALLOON, asm_to->balloon_conf, asm_from->balloon_conf, CloseDelay); 
			ASCF_MERGE_STRING_KEYWORD(BALLOON, asm_to->balloon_conf, asm_from->balloon_conf, Style);
			ASCF_MERGE_SCALAR_KEYWORD(BALLOON, asm_to->balloon_conf, asm_from->balloon_conf, TextPaddingX); 
			ASCF_MERGE_SCALAR_KEYWORD(BALLOON, asm_to->balloon_conf, asm_from->balloon_conf, TextPaddingY); 
		}
	}
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


