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

#define LOCAL_DEBUG

#include "../configure.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/hints.h"
#include "../libAfterStep/mystyle.h"
#include "../libAfterStep/screen.h"
#include "../libAfterStep/mylook.h"

#include "afterconf.h"


TermDef       AlignTerms[] = {
	ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,Left		,NULL,ALIGN_LEFT,0),
    ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,Top			,NULL,ALIGN_TOP,0),
    ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,Right		,NULL,ALIGN_RIGHT,0),
    ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,Bottom		,NULL,ALIGN_BOTTOM,0),
    ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,HTiled		,NULL,RESIZE_H,RESIZE_H_SCALE),
    ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,VTiled		,NULL,RESIZE_V,RESIZE_V_SCALE),
    ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,HScaled		,NULL,RESIZE_H|RESIZE_H_SCALE,0),
    ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,VScaled		,NULL,RESIZE_V|RESIZE_V_SCALE,0),
    ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,LabelSize	,NULL,FIT_LABEL_SIZE,0),
    ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,LabelWidth	,NULL,FIT_LABEL_WIDTH,0),
    ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,LabelHeight	,NULL,FIT_LABEL_HEIGHT,0),
    ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,Center		,NULL,ALIGN_CENTER,0),
    ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,HCenter		,NULL,ALIGN_HCENTER,0),
    ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,VCenter		,NULL,ALIGN_VCENTER,0),
    ASCF_DEFINE_KEYWORD_F(ALIGN,TF_NO_MYNAME_PREPENDING,None		,NULL,NO_ALIGN,ALIGN_MASK),
    {0, NULL, 0, 0, 0}
};

SyntaxDef     AlignSyntax = {
	',',
	'\n',
    AlignTerms,
	0,										   /* use default hash size */
    ' ',
	" ",
	"\t",
    "Look Alignment flags",
	"Align",
	"",
	NULL,
	0
};

flag_options_xref AlignFlagsXref[] = {
    {ALIGN_LEFT, ALIGN_Left_ID     , 0},
    {ALIGN_TOP, ALIGN_Top_ID      , 0},
    {ALIGN_RIGHT, ALIGN_Right_ID    , 0},
    {ALIGN_BOTTOM, ALIGN_Bottom_ID   , 0},
    {RESIZE_H, ALIGN_HTiled_ID   , 0},
    {RESIZE_V, ALIGN_VTiled_ID   , 0},
    {RESIZE_H, ALIGN_HScaled_ID  , 0},
    {RESIZE_V, ALIGN_VScaled_ID  , 0},
    {RESIZE_H_SCALE, ALIGN_HScaled_ID, ALIGN_HTiled_ID},
    {RESIZE_V_SCALE, ALIGN_VScaled_ID, ALIGN_VTiled_ID},
    {FIT_LABEL_SIZE, ALIGN_LabelSize_ID, 0},
    {FIT_LABEL_WIDTH, ALIGN_LabelWidth_ID, 0},
    {FIT_LABEL_HEIGHT, ALIGN_LabelHeight_ID, 0},
    {0, 0, 0}
};


TermDef       BevelTerms[] = {
    ASCF_DEFINE_KEYWORD_F(BEVEL,TF_NO_MYNAME_PREPENDING, None		,NULL,NO_HILITE,HILITE_MASK),
    ASCF_DEFINE_KEYWORD_F(BEVEL,TF_NO_MYNAME_PREPENDING, Left		,NULL,LEFT_HILITE,0),
    ASCF_DEFINE_KEYWORD_F(BEVEL,TF_NO_MYNAME_PREPENDING, Top		,NULL,TOP_HILITE,0),
    ASCF_DEFINE_KEYWORD_F(BEVEL,TF_NO_MYNAME_PREPENDING, Right		,NULL,RIGHT_HILITE,0),
    ASCF_DEFINE_KEYWORD_F(BEVEL,TF_NO_MYNAME_PREPENDING, Bottom		,NULL,BOTTOM_HILITE,0),
    ASCF_DEFINE_KEYWORD_F(BEVEL,TF_NO_MYNAME_PREPENDING, Extra		,NULL,EXTRA_HILITE,0),
    ASCF_DEFINE_KEYWORD_F(BEVEL,TF_NO_MYNAME_PREPENDING, NoOutline	,NULL,NO_HILITE_OUTLINE,0),
    ASCF_DEFINE_KEYWORD_F(BEVEL,TF_NO_MYNAME_PREPENDING, NoInline	,NULL,NO_HILITE_INLINE,0),
    {0, NULL, 0, 0, 0}
};

SyntaxDef     BevelSyntax = {
	',',
	'\n',
    BevelTerms,
	0,										   /* use default hash size */
    ' ',
	" ",
	"\t",
    "Look Bevel flags",
	"Bevel",
	"",
	NULL,
	0
};
struct SyntaxDef     *BevelSyntaxPtr = &BevelSyntax;

flag_options_xref BevelFlagsXref[] = {
    {LEFT_HILITE, BEVEL_Left_ID, BEVEL_None_ID},
    {TOP_HILITE, BEVEL_Top_ID, BEVEL_None_ID},
    {RIGHT_HILITE, BEVEL_Right_ID, BEVEL_None_ID},
    {BOTTOM_HILITE, BEVEL_Bottom_ID, BEVEL_None_ID},
    {EXTRA_HILITE, BEVEL_Extra_ID, BEVEL_None_ID},
    {NO_HILITE_OUTLINE, BEVEL_NoOutline_ID, BEVEL_None_ID},
    {NO_HILITE_INLINE, BEVEL_NoInline_ID, BEVEL_None_ID},
    {0, 0, 0}
};




ASFlagType
ParseBevelOptions( FreeStorageElem * options )
{
    ASFlagType bevel = 0 ;
    while( options )
	{
        LOCAL_DEBUG_OUT( "options(%p)->keyword(\"%s\")", options, options->term->keyword );
        if (options->term != NULL)
            ReadFlagItem (NULL, &bevel, options, BevelFlagsXref);
        options = options->next;
    }
    return bevel;
}

ASFlagType
ParseAlignOptions( FreeStorageElem * options )
{
    ASFlagType align = 0 ;
    while( options )
	{
        LOCAL_DEBUG_OUT( "options(%p)->keyword(\"%s\")", options, options->term->keyword );
        if (options->term != NULL)
		{
			switch( options->term->id )
			{
				case ALIGN_Center_ID :
					set_flags( align, ALIGN_CENTER );
				    break ;
				case ALIGN_HCenter_ID :
					set_flags( align, ALIGN_HCENTER );
				    break ;
				case ALIGN_VCenter_ID :
					set_flags( align, ALIGN_VCENTER );
				    break ;
				default:
            		ReadFlagItem (NULL, &align, options, AlignFlagsXref);
			}
		}
        options = options->next;
    }
    return align;
}



void
flags_parse(char *tline, FILE * fd, char *myname, int *pflags, SyntaxDef *syntax, ASFlagType (*parse_flags)( FreeStorageElem *) )
{
    FreeStorageElem *Storage = NULL;
    if( pflags == NULL )
        return;

    Storage = tline_subsyntax_parse(NULL, tline, fd, myname, syntax, NULL, NULL);
	if( Storage )
	{
    	*pflags = parse_flags(Storage);
		DestroyFreeStorage (&Storage);
	}
}

void
align_parse(char *tline, FILE * fd, char **myname, int *palign )
{
	flags_parse( tline, fd, (char*)myname, palign, &AlignSyntax, ParseAlignOptions );
}		 

void
bevel_parse(char *tline, FILE * fd, char **myname, int *pbevel )
{
	flags_parse( tline, fd, (char*)myname, pbevel, &BevelSyntax, ParseBevelOptions );
}		 

