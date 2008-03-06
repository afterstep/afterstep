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

TermDef       BalloonContentsTerms[] = {
    ASCF_DEFINE_KEYWORD_F(BALLOON, TF_NO_MYNAME_PREPENDING, Name		, NULL, BALLOON_SHOW_Name, ),
    ASCF_DEFINE_KEYWORD_F(BALLOON, TF_NO_MYNAME_PREPENDING, IconName	, NULL, BALLOON_SHOW_IconName, ),
    ASCF_DEFINE_KEYWORD_F(BALLOON, TF_NO_MYNAME_PREPENDING, ResClass	, NULL, BALLOON_SHOW_ResClass, ),
    ASCF_DEFINE_KEYWORD_F(BALLOON, TF_NO_MYNAME_PREPENDING, ResName		, NULL, BALLOON_SHOW_ResName, ),
    ASCF_DEFINE_KEYWORD_F(BALLOON, TF_NO_MYNAME_PREPENDING, Comment		, NULL, BALLOON_SHOW_Comment, ),
    ASCF_DEFINE_KEYWORD_F(BALLOON, TF_NO_MYNAME_PREPENDING, Exec		, NULL, BALLOON_SHOW_Exec, ),
    ASCF_DEFINE_KEYWORD_F(BALLOON, TF_NO_MYNAME_PREPENDING, GenericName	, NULL, BALLOON_SHOW_GenericName, ),
    {0, NULL, 0, 0, 0}
};

SyntaxDef     BalloonContentsSyntax = {
	',',
	'\n',
    BalloonContentsTerms,
	0,										   /* use default hash size */
    ' ',
	" ",
	"\t",
    "Balloon Contents flags",
	"BalloonContents",
	"",
	NULL,
	0
};
struct SyntaxDef     *BalloonContentsSyntaxPtr = &BalloonContentsSyntax;

flag_options_xref BalloonContentsFlagsXref[] = {
    {BALLOON_SHOW_Name, 		BALLOON_Name_ID			, 0},
    {BALLOON_SHOW_IconName, 	BALLOON_IconName_ID		, 0},
    {BALLOON_SHOW_ResClass, 	BALLOON_ResClass_ID		, 0},
    {BALLOON_SHOW_ResName, 		BALLOON_ResName_ID		, 0},
    {BALLOON_SHOW_Comment, 		BALLOON_Comment_ID		, 0},
    {BALLOON_SHOW_Exec, 		BALLOON_Exec_ID			, 0},
    {BALLOON_SHOW_GenericName, 	BALLOON_GenericName_ID	, 0},
    {0, 0, 0}
};

ASFlagType
ParseBalloonContentsOptions( FreeStorageElem * options )
{
    ASFlagType hints = 0 ;
    while( options )
	{
        LOCAL_DEBUG_OUT( "options(%p)->keyword(\"%s\")", options, options->term->keyword );
        if (options->term != NULL)
            ReadFlagItem (NULL, &hints, options, BalloonContentsFlagsXref);
        options = options->next;
    }
    return hints;
}


int ASDefaultBalloonTypes[]= { BALLOON_ID_START, 0 };

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
  config->type = CONFIG_Balloons_ID ; 
  
  return config;
}

void
Destroy_balloonConfig (balloonConfig * config)
{
  	if (!config)
		return;
  	if (config->Style)
    	free (config->Style);
	config->type = 0 ; 
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
        fprintf( stderr,"BALLOON.set_flags = 0x%lX\n",config->set_flags);
        fprintf( stderr,"BALLOON.flags = 0x%lX\n",config->flags);
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
balloon_config2look( MyLook *look, ASBalloonLook *balloon_look, balloonConfig *config, const char *default_style )
{
    if( look )
    {
		if( balloon_look == NULL ) 
		{
        	if( look->balloon_look == NULL )
            	look->balloon_look = create_balloon_look();
			balloon_look = look->balloon_look ;
		}
        if( config == NULL )
		{	
			balloon_look->show = True ;
        	balloon_look->XOffset = 5 ;
        	balloon_look->YOffset = 5 ;
			balloon_look->Style = mystyle_list_find_or_default (look->styles_list, default_style);
			balloon_look->Delay = 200 ;
			balloon_look->CloseDelay = 2000 ;
		}else
        {
#define MERGE_BALLOON_SCALAR_VAL(val)  balloon_look->val = config->val 			

            balloon_look->show = get_flags( config->flags, BALLOON_USED );
			MERGE_BALLOON_SCALAR_VAL(BorderHilite);
			MERGE_BALLOON_SCALAR_VAL(XOffset);
			MERGE_BALLOON_SCALAR_VAL(YOffset);
			MERGE_BALLOON_SCALAR_VAL(Delay);
			MERGE_BALLOON_SCALAR_VAL(CloseDelay);
            balloon_look->Style = mystyle_list_find_or_default (look->styles_list, config->Style?config->Style:default_style);
			MERGE_BALLOON_SCALAR_VAL(TextPaddingX);
			MERGE_BALLOON_SCALAR_VAL(TextPaddingY);
		}
	    LOCAL_DEBUG_OUT( "balloon mystyle = %p (\"%s\")", balloon_look->Style,
                    	 balloon_look->Style?balloon_look->Style->name:"none" );

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
Process_balloonOptions (FreeStorageElem * options, balloonConfig *config, int id_base )
{
    ConfigItem item;
    item.memory = NULL;

    if( config == NULL )
        config = Create_balloonConfig();

    for (; options; options = options->next)
    {
		int orig_id = options->term->id ; 
		int id = (int)options->term->id - id_base + BALLOON_ID_START ; 
        if (id < BALLOON_ID_START || id > BALLOON_ID_END )
            continue;

        if (options->term == NULL)
            continue;
        if (options->term->type == TT_FLAG)
        {
			options->term->id = id ; 
			if( !ReadFlagItemAuto (config, 0, options, &BalloonsFlags[0]) )
			{
	            switch(id )
				{
		            ASCF_HANDLE_BEVEL_KEYWORD_CASE(BALLOON,config,options,BorderHilite); 
				}
			}
			LOCAL_DEBUG_OUT( "%s - %lX:%lX", options->term->keyword, config->set_flags, config->flags );
			options->term->id = orig_id ;
            continue;
        }

        if (!ReadConfigItem (&item, options))
            continue;

        switch (id)
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
		int i ;
		for( i = 0 ; asm_to->class->balloon_types[i] > 0 && asm_from->class->balloon_types[i] > 0 ; ++i ) 
		{
			balloonConfig *from = asm_from->balloon_configs[i];
			balloonConfig *to = asm_to->balloon_configs[i];		
			if( to == NULL )
	   			to = Create_balloonConfig ();

			if( from != NULL ) 
			{
		    	ASCF_MERGE_FLAGS(to,from);
				ASCF_MERGE_SCALAR_KEYWORD(BALLOON, to, from, BorderHilite);
				ASCF_MERGE_SCALAR_KEYWORD(BALLOON, to, from, XOffset); 
				ASCF_MERGE_SCALAR_KEYWORD(BALLOON, to, from, YOffset); 
				ASCF_MERGE_SCALAR_KEYWORD(BALLOON, to, from, Delay); 
				ASCF_MERGE_SCALAR_KEYWORD(BALLOON, to, from, CloseDelay); 
				ASCF_MERGE_STRING_KEYWORD(BALLOON, to, from, Style);
				ASCF_MERGE_SCALAR_KEYWORD(BALLOON, to, from, TextPaddingX); 
				ASCF_MERGE_SCALAR_KEYWORD(BALLOON, to, from, TextPaddingY); 
			}
			asm_to->balloon_configs[i] = to ;
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


