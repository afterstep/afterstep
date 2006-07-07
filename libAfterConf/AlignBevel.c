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
    {TF_NO_MYNAME_PREPENDING, "Left", 4,        TT_FLAG, ALIGN_Left_ID      , NULL},
    {TF_NO_MYNAME_PREPENDING, "Top", 3,         TT_FLAG, ALIGN_Top_ID       , NULL},
    {TF_NO_MYNAME_PREPENDING, "Right", 5,       TT_FLAG, ALIGN_Right_ID     , NULL},
    {TF_NO_MYNAME_PREPENDING, "Bottom", 6,      TT_FLAG, ALIGN_Bottom_ID    , NULL},
    {TF_NO_MYNAME_PREPENDING, "HTiled", 6,      TT_FLAG, ALIGN_HTiled_ID    , NULL},
    {TF_NO_MYNAME_PREPENDING, "VTiled", 6,      TT_FLAG, ALIGN_VTiled_ID    , NULL},
    {TF_NO_MYNAME_PREPENDING, "HScaled", 7,     TT_FLAG, ALIGN_HScaled_ID   , NULL},
    {TF_NO_MYNAME_PREPENDING, "VScaled", 7,     TT_FLAG, ALIGN_VScaled_ID   , NULL},
    {TF_NO_MYNAME_PREPENDING, "LabelSize", 9,   TT_FLAG, ALIGN_LabelSize_ID , NULL},
    {TF_NO_MYNAME_PREPENDING, "LabelWidth", 10,    TT_FLAG, ALIGN_LabelWidth_ID,  NULL},
    {TF_NO_MYNAME_PREPENDING, "LabelHeight", 11,   TT_FLAG, ALIGN_LabelHeight_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "Center", 6,      TT_FLAG, ALIGN_Center_ID    , NULL},
    {TF_NO_MYNAME_PREPENDING, "HCenter", 7,     TT_FLAG, ALIGN_HCenter_ID   , NULL},
    {TF_NO_MYNAME_PREPENDING, "VCenter", 7,     TT_FLAG, ALIGN_VCenter_ID   , NULL},
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
    {TF_NO_MYNAME_PREPENDING, "None", 4,        TT_FLAG, BEVEL_None_ID     , NULL},
    {TF_NO_MYNAME_PREPENDING, "Left", 4,        TT_FLAG, BEVEL_Left_ID     , NULL},
    {TF_NO_MYNAME_PREPENDING, "Top", 3,         TT_FLAG, BEVEL_Top_ID      , NULL},
    {TF_NO_MYNAME_PREPENDING, "Right", 5,       TT_FLAG, BEVEL_Right_ID    , NULL},
    {TF_NO_MYNAME_PREPENDING, "Bottom", 6,      TT_FLAG, BEVEL_Bottom_ID   , NULL},
    {TF_NO_MYNAME_PREPENDING, "Extra", 5,       TT_FLAG, BEVEL_Extra_ID    , NULL},
    {TF_NO_MYNAME_PREPENDING, "NoOutline", 9,   TT_FLAG, BEVEL_NoOutline_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "NoInline", 8,    TT_FLAG, BEVEL_NoInline_ID, NULL},
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

