/*
 * Copyright (c) 2000,2003 Sasha Vasko <sasha at aftercode.net>
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

#include "../../configure.h"

#include "../../include/asapp.h"
#include "../../include/afterstep.h"
#include "../../include/mystyle.h"
#include "../../include/parser.h"
#include "../../include/parse.h"
#include "../../include/screen.h"
#include "../../include/mylook.h"
#include "../../include/confdefs.h"



TermDef       WindowBoxTerms[] = {
    {TF_NO_MYNAME_PREPENDING, "WindowBox", 9, TT_QUOTED_TEXT, WINDOWBOX_START_ID, NULL},
    {TF_NO_MYNAME_PREPENDING, "Area", 4,              TT_GEOMETRY,WINDOWBOX_Area_ID   	  	 , NULL},
    {TF_NO_MYNAME_PREPENDING, "Virtual", 7,           TT_FLAG,    WINDOWBOX_Virtual_ID       , NULL},
    {TF_NO_MYNAME_PREPENDING, "MinWidth", 8,          TT_INTEGER, WINDOWBOX_MinWidth_ID	  	 , NULL},
    {TF_NO_MYNAME_PREPENDING, "MinHeight", 9,         TT_INTEGER, WINDOWBOX_MinHeight_ID	 , NULL},
    {TF_NO_MYNAME_PREPENDING, "MaxWidth", 8,          TT_INTEGER, WINDOWBOX_MaxWidth_ID	  	 , NULL},
    {TF_NO_MYNAME_PREPENDING, "MaxHeight", 9,         TT_INTEGER, WINDOWBOX_MaxHeight_ID	 , NULL},
    {TF_NO_MYNAME_PREPENDING, "FirstTry", 8,          TT_INTEGER, WINDOWBOX_FirstTry_ID	  	 , NULL},
    {TF_NO_MYNAME_PREPENDING, "ThenTry", 7,           TT_INTEGER, WINDOWBOX_ThenTry_ID 	  	 , NULL},
    {TF_NO_MYNAME_PREPENDING, "VerticalPriority", 16, TT_FLAG,    WINDOWBOX_VerticalPriority_ID, NULL},
	{TF_NO_MYNAME_PREPENDING, "ReverseOrder", 12,     TT_FLAG,    WINDOWBOX_ReverseOrder_ID    , NULL},
	{TF_NO_MYNAME_PREPENDING |
	 TF_SYNTAX_TERMINATOR,    "~WindowBox", 10, TT_FLAG, WINDOWBOX_DONE_ID, NULL},
	{0, NULL, 0, 0, 0}
};

SyntaxDef     WindowBoxSyntax = {
	'\n',
	'\0',
	WindowBoxTerms,
    7,                                         /* hash size */
	'\t',
	"",
	"\t",
	"WindowBox definition",
	NULL,
	0
};


/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the base.<bpp> config
 * file
 *
 ****************************************************************************/
extern SyntaxDef     PopupFuncSyntax ;
void input_context_destroy(ASHashableValue value, void *data);

TermDef FeelTerms[] =
{
  /* focus : */
  {TF_NO_MYNAME_PREPENDING, "ClickToFocus",12           , TT_FLAG       , FEEL_ClickToFocus_ID          , NULL},
  {TF_NO_MYNAME_PREPENDING, "SloppyFocus",11            , TT_FLAG       , FEEL_SloppyFocus_ID           , NULL},
  {TF_NO_MYNAME_PREPENDING, "AutoFocus",9               , TT_FLAG       , FEEL_AutoFocus_ID             , NULL},
  /* decorations : */
  {TF_NO_MYNAME_PREPENDING, "DecorateTransients",18     , TT_FLAG       , FEEL_DecorateTransients_ID    , NULL},
  /* placement : */
  {TF_NO_MYNAME_PREPENDING, "DontMoveOff",11            , TT_FLAG       , FEEL_DontMoveOff_ID           , NULL},
  {TF_NO_MYNAME_PREPENDING|TF_DEPRECIATED, "NoPPosition",11            , TT_FLAG       , FEEL_NoPPosition_ID           , NULL},
  {TF_NO_MYNAME_PREPENDING, "SmartPlacement",14         , TT_FLAG       , FEEL_SmartPlacement_ID        , NULL},
  {TF_NO_MYNAME_PREPENDING, "RandomPlacement",15        , TT_FLAG       , FEEL_RandomPlacement_ID       , NULL},
  {TF_NO_MYNAME_PREPENDING, "StubbornPlacement",17      , TT_FLAG       , FEEL_StubbornPlacement_ID     , NULL},
  {TF_NO_MYNAME_PREPENDING, "MenusHigh",9               , TT_FLAG       , FEEL_MenusHigh_ID             , NULL},
  {TF_NO_MYNAME_PREPENDING, "CenterOnCirculate",17      , TT_FLAG       , FEEL_CenterOnCirculate_ID     , NULL},
  /* icons : */
  {TF_NO_MYNAME_PREPENDING, "SuppressIcons",13          , TT_FLAG       , FEEL_SuppressIcons_ID         , NULL},
  {TF_NO_MYNAME_PREPENDING, "IconTitle",9               , TT_FLAG       , FEEL_IconTitle_ID             , NULL},
  {TF_NO_MYNAME_PREPENDING, "KeepIconWindows",15        , TT_FLAG       , FEEL_KeepIconWindows_ID       , NULL},
  {TF_NO_MYNAME_PREPENDING, "StickyIcons",11            , TT_FLAG       , FEEL_StickyIcons_ID           , NULL},
  {TF_NO_MYNAME_PREPENDING, "StubbornIcons",13          , TT_FLAG       , FEEL_StubbornIcons_ID         , NULL},
  {TF_NO_MYNAME_PREPENDING, "StubbornIconPlacement",21  , TT_FLAG       , FEEL_StubbornIconPlacement_ID , NULL},
  {TF_NO_MYNAME_PREPENDING, "CirculateSkipIcons",18     , TT_FLAG       , FEEL_CirculateSkipIcons_ID    , NULL},
  /* backing store/save unders : */
  {TF_NO_MYNAME_PREPENDING, "BackingStore",12           , TT_FLAG       , FEEL_BackingStore_ID          , NULL},
  {TF_NO_MYNAME_PREPENDING, "AppsBackingStore",16       , TT_FLAG       , FEEL_AppsBackingStore_ID      , NULL},
  {TF_NO_MYNAME_PREPENDING, "SaveUnders",10             , TT_FLAG       , FEEL_SaveUnders_ID            , NULL},
  /* pageing */
  {TF_NO_MYNAME_PREPENDING, "PagingDefault",13          , TT_FLAG       , FEEL_PagingDefault_ID         , NULL},
  {TF_NO_MYNAME_PREPENDING, "AutoTabThroughDesks",19    , TT_FLAG       , FEEL_AutoTabThroughDesks_ID   , NULL},

  {TF_NO_MYNAME_PREPENDING, "ClickTime",9               , TT_UINTEGER   , FEEL_ClickTime_ID         , NULL},
  {TF_NO_MYNAME_PREPENDING, "OpaqueMove",10             , TT_UINTEGER   , FEEL_OpaqueMove_ID        , NULL},
  {TF_NO_MYNAME_PREPENDING, "OpaqueResize",12           , TT_UINTEGER   , FEEL_OpaqueResize_ID      , NULL},
  {TF_NO_MYNAME_PREPENDING, "AutoRaise",9               , TT_UINTEGER   , FEEL_AutoRaise_ID         , NULL},
  {TF_NO_MYNAME_PREPENDING, "AutoReverse",11            , TT_UINTEGER   , FEEL_AutoReverse_ID       , NULL},
  {TF_NO_MYNAME_PREPENDING, "DeskAnimationType",17      , TT_UINTEGER   , FEEL_DeskAnimationType_ID , NULL},
  {TF_NO_MYNAME_PREPENDING, "ShadeAnimationSteps",19    , TT_UINTEGER   , FEEL_ShadeAnimationSteps_ID,NULL},

  {TF_NO_MYNAME_PREPENDING, "DeskAnimationSteps",18     , TT_UINTEGER   , FEEL_DeskAnimationSteps_ID, NULL},

  {TF_NO_MYNAME_PREPENDING, "XorValue",8                , TT_INTEGER    , FEEL_XorValue_ID          , NULL},
  {TF_NO_MYNAME_PREPENDING, "Xzap",4                    , TT_INTEGER    , FEEL_Xzap_ID              , NULL},
  {TF_NO_MYNAME_PREPENDING, "Yzap",4                    , TT_INTEGER    , FEEL_Yzap_ID              , NULL},
  {TF_INDEXED|TF_NO_MYNAME_PREPENDING,
                            "Cursor",6                  , TT_INTEGER    , FEEL_Cursor_ID            , NULL},
  {TF_INDEXED|TF_NO_MYNAME_PREPENDING,
                            "CustomCursor",12           , TT_CURSOR     , FEEL_CustomCursor_ID      , NULL},

  {TF_NO_MYNAME_PREPENDING, "ClickToRaise",12           , TT_BITLIST    , FEEL_ClickToRaise_ID      , NULL},

  {TF_NO_MYNAME_PREPENDING, "EdgeScroll",10             , TT_INTARRAY   , FEEL_EdgeScroll_ID        , NULL},
  {TF_NO_MYNAME_PREPENDING, "EdgeResistance",14         , TT_INTARRAY   , FEEL_EdgeResistance_ID    , NULL},

  /* this stuff requires subsyntaxes : */
  {TF_NO_MYNAME_PREPENDING, "Popup",5                   , TT_QUOTED_TEXT, FEEL_Popup_ID             , &PopupFuncSyntax},
  {TF_NO_MYNAME_PREPENDING, "Function",8                , TT_QUOTED_TEXT, FEEL_Function_ID          , &PopupFuncSyntax},

  /* mouse and key bindings require special processing : */
  {TF_SPECIAL_PROCESSING|TF_NO_MYNAME_PREPENDING,
                            "Mouse",5                   , TT_BINDING    , FEEL_Mouse_ID             , &FuncSyntax},
  {TF_SPECIAL_PROCESSING|TF_NO_MYNAME_PREPENDING,
                            "Key",3                     , TT_BINDING    , FEEL_Key_ID               , &FuncSyntax},

    /* obsolete stuff : */
  {TF_OBSOLETE|TF_NO_MYNAME_PREPENDING, "MWMFunctionHints",16       , TT_FLAG       , FEEL_MWMFunctionHints_ID  , NULL},
  {TF_OBSOLETE|TF_NO_MYNAME_PREPENDING, "MWMDecorHints",13          , TT_FLAG       , FEEL_MWMDecorHints_ID     , NULL},
  {TF_OBSOLETE|TF_NO_MYNAME_PREPENDING, "MWMHintOverride",15        , TT_FLAG       , FEEL_MWMHintOverride_ID   , NULL},

  {0, NULL, 0, 0, 0}
};

SyntaxDef FeelSyntax =
{
  '\n',
  '\0',
  FeelTerms,
  0,				/* use default hash size */
  '\t',
  "",
  "\t",
  "Feel",
  NULL,
  0
};

flag_options_xref FeelFlagsXref[] = {
     /* focus : */
    {ClickToFocus          , FEEL_ClickToFocus_ID          , 0},
    {SloppyFocus           , FEEL_SloppyFocus_ID           , 0},
//    {AutoFocusNewWindows   , FEEL_AutoFocus_ID             , 0},
     /* decorations : */
    {DecorateTransients    , FEEL_DecorateTransients_ID    , 0},
     /* placement : */
    {DontMoveOff           , FEEL_DontMoveOff_ID           , 0},
    {NoPPosition           , FEEL_NoPPosition_ID           , 0},
    {SMART_PLACEMENT        , FEEL_SmartPlacement_ID        , 0},
    {RandomPlacement       , FEEL_RandomPlacement_ID       , 0},
    {StubbornPlacement     , FEEL_StubbornPlacement_ID     , 0},
    {MenusHigh             , FEEL_MenusHigh_ID             , 0},
    {CenterOnCirculate     , FEEL_CenterOnCirculate_ID     , 0},
     /* icons : */
    {SuppressIcons         , FEEL_SuppressIcons_ID         , 0},
    {IconTitle             , FEEL_IconTitle_ID             , 0},
    {KeepIconWindows       , FEEL_KeepIconWindows_ID       , 0},
    {StickyIcons           , FEEL_StickyIcons_ID           , 0},
    {StubbornIcons         , FEEL_StubbornIcons_ID         , 0},
    {StubbornIconPlacement , FEEL_StubbornIconPlacement_ID , 0},
    {CirculateSkipIcons    , FEEL_CirculateSkipIcons_ID    , 0},
     /* backing store/save unders : */
    {BackingStore          , FEEL_BackingStore_ID          , 0},
    {AppsBackingStore      , FEEL_AppsBackingStore_ID      , 0},
    {SaveUnders            , FEEL_SaveUnders_ID            , 0},
     /* pageing */
    {DoHandlePageing       , FEEL_PagingDefault_ID         , 0},
    {AutoTabThroughDesks   , FEEL_AutoTabThroughDesks_ID   , 0},

	{0, 0, 0}
};

TermDef AutoExecTerms[] =
{
  {TF_NO_MYNAME_PREPENDING, "Function",8                , TT_QUOTED_TEXT, FEEL_Function_ID          , &PopupFuncSyntax},
  {0, NULL, 0, 0, 0}
};

SyntaxDef AutoExecSyntax =
{
  '\n',
  '\0',
  AutoExecTerms,
  0,				/* use default hash size */
  ' ',
  "",
  "\t",
  "autoexec",
  NULL,
  0
};


/**************************************************************************************
 * WindowBox code :
 **************************************************************************************/
#if 0
/********************************************************************/
/* First MyFrame processing code				    */
/********************************************************************/
MyFrameDefinition *
AddMyFrameDefinition (MyFrameDefinition ** tail)
{
    MyFrameDefinition *new_def = safecalloc (1, sizeof (MyFrameDefinition));
	*tail = new_def;
	return new_def;
}

void
DestroyMyFrameDefinitions (MyFrameDefinition ** list)
{
	if (*list)
	{
		register int  i;
        MyFrameDefinition *fd = *list;

        DestroyMyFrameDefinitions (&(fd->next));
        if (fd->name)
            free (fd->name);
		for (i = 0; i < FRAME_PARTS; i++)
            if (fd->parts[i])
                free (fd->parts[i]);

        for( i = 0 ; i < BACK_STYLES ; ++i )
        {
            if( fd->title_styles[i] )
                free(fd->title_styles[i]);
            if( fd->frame_styles[i] )
                free(fd->frame_styles[i]);
        }
        if( fd->title_back )
            free( fd->title_back );
        if( fd->inheritance_list )
        {
            i = fd->inheritance_num-1 ;
            while( --i >= 0 )
                if( fd->inheritance_list[i] )
                    free( fd->inheritance_list[i] );
            free( fd->inheritance_list );
        }
        free (fd);
        *list = NULL;
	}
}

void
PrintMyFrameDefinitions (MyFrameDefinition * list, int index)
{
	if (list)
	{
		register int  i;

        if (list->name)
            fprintf (stderr, "MyFrame[%d].name = \"%s\";\n", index, list->name);
        fprintf (stderr, "MyFrame[%d].set_parts = 0x%lX;\n", index, list->set_parts);
        fprintf (stderr, "MyFrame[%d].parts_mask = 0x%lX;\n", index, list->parts_mask);
        for (i = 0; i < FRAME_PARTS; i++)
			if (list->parts[i])
                fprintf (stderr, "MyFrame[%d].Side[%d] = \"%s\";\n", index, i, list->parts[i]);
        for( i = 0 ; i < BACK_STYLES ; ++i )
            if( list->title_styles[i])
                fprintf (stderr, "MyFrame[%d].TitleStyle[%d] = \"%s\";\n", index, i, list->title_styles[i]);
        for( i = 0 ; i < BACK_STYLES ; ++i )
            if( list->frame_styles[i])
                fprintf (stderr, "MyFrame[%d].FrameStyle[%d] = \"%s\";\n", index, i, list->frame_styles[i]);
        if( list->title_back )
            fprintf (stderr, "MyFrame[%d].TitleBack = \"%s\";\n", index, list->title_back);
        fprintf (stderr, "MyFrame[%d].set_part_size = 0x%lX;\n", index, list->set_part_size);
        fprintf (stderr, "MyFrame[%d].set_part_bevel = 0x%lX;\n", index, list->set_part_bevel);
        fprintf (stderr, "MyFrame[%d].set_part_align = 0x%lX;\n", index, list->set_part_align);
        for( i = 0 ; i < FRAME_PARTS ; ++i )
        {
            fprintf (stderr, "MyFrame[%d].Part[%d].width = %d;\n", index, i, list->part_width[i]);
            fprintf (stderr, "MyFrame[%d].Part[%d].length = %d;\n", index, i, list->part_length[i]);
            fprintf (stderr, "MyFrame[%d].Part[%d].bevel = 0x%lX;\n", index, i, list->part_bevel[i]);
            fprintf (stderr, "MyFrame[%d].Part[%d].align = 0x%lX;\n", index, i, list->part_align[i]);
        }
        fprintf (stderr, "MyFrame[%d].set_title_attr = 0x%lX;\n", index, list->set_title_attr);
        fprintf (stderr, "MyFrame[%d].title_bevel = 0x%lX;\n", index, list->title_bevel);
        fprintf (stderr, "MyFrame[%d].title_align = 0x%lX;\n", index, list->title_align);
        fprintf (stderr, "MyFrame[%d].title_back_align = 0x%lX;\n", index, list->title_back_align);
        if( list->inheritance_list )
            for( i = 0 ; i < list->inheritance_num ; ++i )
                fprintf (stderr, "MyFrame[%d].Inherit[%d] = \"%s\";\n", index, i, list->inheritance_list[i]);
        PrintMyFrameDefinitions (list->next, index+1);
    }
}

MyFrameDefinition **
ProcessMyFrameOptions (FreeStorageElem * options, MyFrameDefinition ** tail)
{
	ConfigItem    item;
	int           rel_id;
    MyFrameDefinition *fd = NULL ;

	item.memory = NULL;

	for (; options; options = options->next)
	{
		if (options->term == NULL)
			continue;
        LOCAL_DEBUG_OUT( "MyFrame(%p)->options(%p)->keyword(\"%s\")", fd, options, options->term->keyword );
		if (options->term->id < MYFRAME_ID_START || options->term->id > MYFRAME_ID_END)
			continue;

		if (options->term->type == TT_FLAG)
		{
            if( fd != NULL  )
            {
                if( options->term->id == MYFRAME_DONE_ID )
                    break;                     /* for loop */
                switch( options->term->id )
                {
                    case MYFRAME_TitleBevel_ID :
                        fd->title_bevel = ParseBevelOptions( options->sub );
                        set_flags( fd->set_title_attr, MYFRAME_TitleBevelSet );
                        break;
                    case MYFRAME_TitleAlign_ID :
                        fd->title_align = ParseAlignOptions( options->sub );
                        set_flags( fd->set_title_attr, MYFRAME_TitleAlignSet );
                        break;
                    case MYFRAME_TitleBackgroundAlign_ID :
                        fd->title_back_align = ParseAlignOptions( options->sub );
                        set_flags( fd->set_title_attr, MYFRAME_TitleBackAlignSet );
                        break;
                    default:
                        if (!ReadConfigItem (&item, options))
                            continue;
                        LOCAL_DEBUG_OUT( "item.index = %d", item.index );
                        if( item.index < 0 || item.index >= FRAME_PARTS )
                        {
                            item.ok_to_free = 1;
                            continue;
                        }
                        switch( options->term->id )
                        {
                            case MYFRAME_NoSide_ID :
                            case MYFRAME_NoCorner_ID :
                                clear_flags( fd->parts_mask, 0x01<<item.index );
                                set_flags( fd->set_parts, 0x01<<item.index );
                                break;
                            case MYFRAME_SideAlign_ID :
                            case MYFRAME_CornerAlign_ID :
                                fd->part_align[item.index] = ParseAlignOptions( options->sub );
                                set_flags( fd->set_part_align, 0x01<<item.index );
                                break;
                            case MYFRAME_SideBevel_ID :
                            case MYFRAME_CornerBevel_ID :
                                fd->part_bevel[item.index] = ParseBevelOptions( options->sub );
                                set_flags( fd->set_part_bevel, 0x01<<item.index );
                                break;
                            default:
                                show_warning( "Unexpected MyFrame definition keyword \"%s\" . Ignoring.", options->term->keyword );
                                item.ok_to_free = 1;
                        }
                }
            }
            continue;
        }

		if (!ReadConfigItem (&item, options))
        {
            LOCAL_DEBUG_OUT( "ReadConfigItem has failed%s","");
			continue;
        }
		if (*tail == NULL)
            fd = AddMyFrameDefinition (tail);

        rel_id = options->term->id - MYFRAME_START_ID;
        if (rel_id == 0)
		{
            if (fd->name != NULL)
			{
				show_error("MyFrame \"%s\": Previous MyFrame definition [%s] was not terminated correctly, and will be ignored.",
                           item.data.string, fd->name);
				DestroyMyFrameDefinitions (tail);
                fd = AddMyFrameDefinition (tail);
			}
            set_string_value (&(fd->name), item.data.string, NULL, 0);
        } else if( rel_id <= FRAME_PARTS )
		{
            --rel_id ;
            set_string_value (&(fd->parts[rel_id]), item.data.string, &(fd->set_parts), (0x01<<rel_id));
            set_flags( fd->parts_mask, (0x01<<rel_id));
        }else
        {
            rel_id = item.index ;
            LOCAL_DEBUG_OUT( "item.index = %d", item.index );
            if( rel_id < 0 || rel_id >= FRAME_PARTS )
                rel_id = 0 ;
            switch( options->term->id )
            {
                case MYFRAME_Side_ID :
                case MYFRAME_Corner_ID :
                    set_string_value (&(fd->parts[rel_id]), item.data.string, &(fd->set_parts), (0x01<<rel_id));
                    set_flags( fd->parts_mask, (0x01<<rel_id));
                    break;
                case MYFRAME_TitleUnfocusedStyle_ID :
                    set_string_value (&(fd->title_styles[BACK_UNFOCUSED]), item.data.string, NULL, 0);
                    break;
                case MYFRAME_TitleFocusedStyle_ID :
                    set_string_value (&(fd->title_styles[BACK_FOCUSED]), item.data.string, NULL, 0);
                    break;
                case MYFRAME_TitleStickyStyle_ID :
                    set_string_value (&(fd->title_styles[BACK_STICKY]), item.data.string, NULL, 0);
                    break;
                case MYFRAME_FrameUnfocusedStyle_ID :
                    set_string_value (&(fd->frame_styles[BACK_UNFOCUSED]), item.data.string, NULL, 0);
                    break;
                case MYFRAME_FrameFocusedStyle_ID :
                    set_string_value (&(fd->frame_styles[BACK_FOCUSED]), item.data.string, NULL, 0);
                    break;
                case MYFRAME_FrameStickyStyle_ID :
                    set_string_value (&(fd->frame_styles[BACK_STICKY]), item.data.string, NULL, 0);
                    break;
                case MYFRAME_TitleBackground_ID :
                    set_string_value (&(fd->title_back), item.data.string, NULL, 0);
                    break;
                case MYFRAME_SideSize_ID :
                case MYFRAME_CornerSize_ID :
                    if( get_flags( item.data.geometry.flags, WidthValue) )
                        fd->part_width[rel_id] = item.data.geometry.width ;
                    else
                        fd->part_width[rel_id] = 0 ;
                    if( get_flags( item.data.geometry.flags, HeightValue) )
                        fd->part_length[rel_id] = item.data.geometry.height ;
                    else
                        fd->part_length[rel_id] = 0 ;
                    set_flags( fd->set_part_size, (0x01<<rel_id));
                    break;
                case MYFRAME_Inherit_ID :
                    {
                        fd->inheritance_list = realloc( fd->inheritance_list, (fd->inheritance_num+1)*sizeof(char*));
                        fd->inheritance_list[fd->inheritance_num] = item.data.string ;
                        ++(fd->inheritance_num);
                    }
                    break;
                default:
                    item.ok_to_free = 1;
                    show_warning( "Unexpected MyFrame definition keyword \"%s\" . Ignoring.", options->term->keyword );
            }
        }
    }
    if (fd != NULL )
    {
        if( fd->name == NULL)
        {
            show_warning( "MyFrame with no name encoutered. Discarding." );
            DestroyMyFrameDefinitions (tail);
        }else
        {
            LOCAL_DEBUG_OUT("done parsing myframe: [%s]", fd->name);
            while (*tail)
                tail = &((*tail)->next);    /* just in case */
        }
    }
    ReadConfigItem (&item, NULL);
	return tail;
}

static FreeStorageElem **
MyFrameDef2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, MyFrameDefinition * def)
{
	register int  i;
	FreeStorageElem **new_tail;

	if (def == NULL)
		return tail;

	new_tail = QuotedString2FreeStorage (syntax, tail, NULL, def->name, MYFRAME_START_ID);
	if (new_tail == tail)
		return tail;

    tail = &((*tail)->sub);

	for (i = 0; i < FRAME_PARTS; i++)
		tail = Path2FreeStorage (&MyFrameSyntax, tail, NULL, def->parts[i], MYFRAME_START_ID + i + 1);

	tail = Flag2FreeStorage (&MyFrameSyntax, tail, MYFRAME_DONE_ID);
    /* TODO: implement the rest of the MyFrame writing : */

	return new_tail;
}

FreeStorageElem **
MyFrameDefs2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, MyFrameDefinition * defs)
{
	while (defs)
	{
		tail = MyFrameDef2FreeStorage (syntax, tail, defs);
		defs = defs->next;
	}
	return tail;
}

void
myframe_parse (char *tline, FILE * fd, char **myname, int *myframe_list)
{
    FilePtrAndData fpd ;
    ConfigDef    *ConfigReader ;
    FreeStorageElem *Storage = NULL, *more_stuff = NULL;
    MyFrameDefinition **list = (MyFrameDefinition**)myframe_list, **tail ;

    if( list == NULL )
        return;
    for( tail = list ; *tail != NULL ; tail = &((*tail)->next) );

    fpd.fp = fd ;
    fpd.data = safemalloc( 12+1+strlen(tline)+1+1 ) ;
    sprintf( fpd.data, "MyFrame %s\n", tline );
LOCAL_DEBUG_OUT( "fd(%p)->tline(\"%s\")->fpd.data(\"%s\")", fd, tline, fpd.data );
    ConfigReader = InitConfigReader ((char*)myname, &MyFrameSyntax, CDT_FilePtrAndData, (void *)&fpd, NULL);
    free( fpd.data );

    if (!ConfigReader)
        return ;

	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &Storage);
	PrintFreeStorage (Storage);

	/* getting rid of all the crap first */
    StorageCleanUp (&Storage, &more_stuff, CF_DISABLED_OPTION);
    DestroyFreeStorage (&more_stuff);

    ProcessMyFrameOptions (Storage, tail);

	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);
}
#endif


/**************************************************************************************
 * FeelConfig management code.
 *************************************************************************************/
FeelConfig *
CreateFeelConfig ()
{
  FeelConfig *config = (FeelConfig *) safemalloc (sizeof (FeelConfig));
  memset( config, 0x00, sizeof( FeelConfig ) );
  config->more_stuff = NULL;

  config->feel = create_asfeel();

  return config;
}

void
DestroyFeelConfig (FeelConfig * config)
{
    register int i ;

    if( config->feel )
        destroy_asfeel( config->feel, False );
    if( config->menu_locations )
    {
        for( i = 0 ; i < config->menu_locs_num ; i++ )
            if( config->menu_locations[i] )
                free( config->menu_locations[i] );
        free( config->menu_locations );
    }

    DestroyFreeStorage (&(config->more_stuff));
    free (config);
}

/**********************************************************************
 *  Feel Parsing code :
 **********************************************************************/
unsigned long
BindingSpecialFunc (ConfigDef * config, FreeStorageElem ** storage)
{
    /* since we have have subconfig of Functions that has \n as line terminator
     * we are going to get entire line again at config->cursor
     * so lets skip 3 tokens of <key/mouse button> <context> <modifyers> ,
     * since those are not parts of following function */
	return TrailingFuncSpecial( config, storage, 3 );
}

void
ParseKeyBinding( ConfigItem *item, FreeStorageElem *func_elem, struct FuncKey **keyboard )
{
    KeySym        keysym;
	KeyCode       keycode;
    int           context, mods ;
	int 		  min = 0, max = 0 ;

    if( item == NULL )
		return ;
    item->ok_to_free = 1;

    if( item == NULL || func_elem == NULL || keyboard == NULL )
        return ;
    if( func_elem->term == NULL || func_elem->term->type != TT_FUNCTION )
        return ;

	/*
	 * Don't let a 0 keycode go through, since that means AnyKey to the
	 * XGrabKey call in GrabKeys().
	 */
	if( (keysym = XStringToKeysym (item->data.binding.sym)) == NoSymbol ||
		(keycode = XKeysymToKeycode (dpy, keysym)) == 0)
		return ;

	XDisplayKeycodes (dpy, &min, &max);
	for (keycode = min; keycode <= max; keycode++)
		if (XKeycodeToKeysym (dpy, keycode, 0) == keysym)
			break;

	if (keycode > max)
		return ;

    context = item->data.binding.context ;
    mods = item->data.binding.mods ;

    if( !ReadConfigItem( item, func_elem ) )
        return ;

    if( item->data.function )
    { /* gotta add it to the keyboard hash */
		FuncKey *tmp ;

		tmp = (FuncKey *) safemalloc (sizeof (FuncKey));

        tmp->next = *keyboard;
		*keyboard = tmp ;

		tmp->name = mystrdup(item->data.binding.sym);
		tmp->keycode = keycode;
		tmp->cont = context;
		tmp->mods = mods;
		tmp->fdata = item->data.function;
		item->data.function = NULL ;
    }
    item->ok_to_free = (item->data.function != NULL);
}


FreeStorageElem **
Keyboard2FreeStorage( SyntaxDef *syntax, FreeStorageElem **tail, struct FuncKey *ic, int id )
{
    FreeStorageElem **d_tail;

    if ( syntax == NULL || tail == NULL )
        return tail ;

    while( ic != NULL )
    {
        d_tail = tail ;
        tail = Binding2FreeStorage (syntax, tail, ic->name, ic->cont, ic->mods, id);
        if( tail != d_tail && *d_tail )   /* saving our function as a sub-elem */
            Func2FreeStorage( &FuncSyntax, &((*d_tail)->sub), ic->fdata );
		ic = ic->next ;
    }
    return tail;
}

FreeStorageElem **
Mouse2FreeStorage( SyntaxDef *syntax, FreeStorageElem **tail, struct MouseButton *ic, int id )
{
    FreeStorageElem **d_tail;

    if ( syntax == NULL || tail == NULL )
        return tail ;

    while( ic != NULL )
    {
		static char buffer[32] ;
        d_tail = tail ;
		sprintf( &(buffer[0]), "%d", ic->Button );
        tail = Binding2FreeStorage (syntax, tail, buffer, ic->Context, ic->Modifier, id);
        if( tail != d_tail && *d_tail )   /* saving our function as a sub-elem */
            Func2FreeStorage( &FuncSyntax, &((*d_tail)->sub), ic->fdata );
		ic = ic->NextButton ;
    }
    return tail;
}

/*********************************************************************************/
/**************************** Actual parsing code ********************************/
/*********************************************************************************/
FeelConfig *
ParseFeelOptions (const char *filename, char *myname)
{
    ConfigDef *ConfigReader =
        InitConfigReader (myname, &FeelSyntax, CDT_Filename, (void *) filename, BindingSpecialFunc);
    FeelConfig *config = CreateFeelConfig ();
	FreeStorageElem *Storage = NULL, *pCurr;
    ConfigItem item;

	if (!ConfigReader)
  		return config;

    item.memory = NULL;
	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &Storage);
    PrintFreeStorage (Storage);

	/* getting rid of all the crap first */
    StorageCleanUp (&Storage, &(config->more_stuff), CF_DISABLED_OPTION);

	for (pCurr = Storage; pCurr; pCurr = pCurr->next)
  	{
  	    if (pCurr->term == NULL)
			continue;
        if (ReadFlagItem (NULL, &(config->feel->flags), pCurr, FeelFlagsXref))
            continue;
        if (!ReadConfigItem (&item, pCurr))
			continue;
        switch (pCurr->term->id)
		{
            case FEEL_ClickTime_ID          :
                config->feel->ClickTime = item.data.integer;
                set_flags (config->feel->set_val_flags, FEEL_ClickTime);
                break ;
            case FEEL_OpaqueMove_ID         :
                config->feel->OpaqueMove = item.data.integer;
                set_flags (config->feel->set_val_flags, FEEL_OpaqueMove);
                break ;
            case FEEL_OpaqueResize_ID       :
                config->feel->OpaqueResize = item.data.integer;
                set_flags (config->feel->set_val_flags, FEEL_OpaqueResize);
                break ;
            case FEEL_AutoRaise_ID          :
                config->feel->AutoRaiseDelay = item.data.integer;
                set_flags (config->feel->set_val_flags, FEEL_AutoRaise);
                break ;
            case FEEL_AutoReverse_ID        :
                config->feel->AutoReverse = item.data.integer;
                set_flags (config->feel->set_val_flags, FEEL_AutoReverse);
                break ;
            case FEEL_ShadeAnimationSteps_ID :
                config->feel->ShadeAnimationSteps = item.data.integer;
                set_flags (config->feel->set_val_flags, FEEL_ShadeAnimationSteps);
                break ;

            case FEEL_XorValue_ID           :
                config->feel->XorValue = item.data.integer;
                set_flags (config->feel->set_val_flags, FEEL_XorValue);
                break ;
            case FEEL_Xzap_ID               :
                config->feel->Xzap = item.data.integer;
                set_flags (config->feel->set_val_flags, FEEL_Xzap);
                break ;
            case FEEL_Yzap_ID               :
                config->feel->Yzap = item.data.integer;
                set_flags (config->feel->set_val_flags, FEEL_Yzap);
                break ;

            case FEEL_Cursor_ID             :                   /* TT_INTEGER */
				/* TODO: backport from as-devel : */
                /*if ( item.index  > 0 && item.index < MAX_CURSORS)
                    config->feel->standard_cursors[item.index] = item.data.integer ;
				 */
                break ;
            case FEEL_CustomCursor_ID       :                   /* TT_BUTTON  */
				/* TODO: backport from as-devel : */
				/*
					if ( item.index  > 0 && item.index < MAX_CURSORS)
                	{
                    	if( config->feel->custom_cursors[item.index] )
                        	destroy_ascursor( &(config->feel->custom_cursors[item.index]));
                    	config->feel->custom_cursors[item.index] = item.data.cursor ;
                	}
				 */
                break ;

            case FEEL_ClickToRaise_ID       :                   /* TT_BITLIST */
                config->feel->RaiseButtons = item.data.integer ;
                set_flags (config->feel->set_val_flags, FEEL_ClickToRaise);
                set_flags (config->feel->set_flags, ClickToRaise);
				set_flags (config->feel->flags, ClickToRaise);
                break ;

            case FEEL_EdgeScroll_ID         :                   /* TT_INTARRAY*/
				item.ok_to_free = 1;
                if( item.data.int_array.size > 0 )
                {
                    config->feel->EdgeScrollX = item.data.int_array.array[0];
                    if( item.data.int_array.size > 1 )
                        config->feel->EdgeScrollY = item.data.int_array.array[1];
                    set_flags (config->feel->set_val_flags, FEEL_EdgeScroll );
                }
                break ;
            case FEEL_EdgeResistance_ID     :                   /* TT_INTARRAY*/
				item.ok_to_free = 1;
                if( item.data.int_array.size > 0 )
                {
                    config->feel->EdgeResistanceScroll = item.data.int_array.array[0];
                    if( item.data.int_array.size > 1 )
                        config->feel->EdgeResistanceMove = item.data.int_array.array[1];
                    set_flags (config->feel->set_val_flags, FEEL_EdgeResistance );
                }
                break ;

            case FEEL_Popup_ID              :
				/* TODO: backport from as-devel : */
                /* FreeStorage2MenuData( pCurr, &item, config->feel->Popups ); */
                break ;
            case FEEL_Function_ID           :
                FreeStorage2ComplexFunction( pCurr, &item, config->feel->ComplexFunctions );
                break ;
            case FEEL_Mouse_ID              :
                if( item.data.binding.sym )
                    if( isdigit( (int)item.data.binding.sym[0] ) && pCurr->sub )
                    {
                        int button_num = item.data.binding.sym[0] - '0' ;
                        if( button_num >= 0 && button_num <= MAX_MOUSE_BUTTONS &&
                            pCurr->sub->term->type == TT_FUNCTION )
                        {
                            ConfigItem func_item ;
                            func_item.memory = NULL ;
                            if( ReadConfigItem( &func_item, pCurr->sub ) )
							{
								MouseButton *tmp = safecalloc( 1, sizeof(MouseButton) );
								tmp->Button = button_num ;
								tmp->Modifier = item.data.binding.mods ;
								tmp->Context = item.data.binding.context ;
								tmp->fdata = func_item.data.function ;
								func_item.data.function = NULL ;
								tmp->NextButton = config->feel->MouseButtonRoot ;
								config->feel->MouseButtonRoot = tmp ;
							}
                            if( func_item.data.function )
                            {
                                func_item.ok_to_free = 1;
                                ReadConfigItem( &func_item, NULL );
                            }
                        }
                    }
                item.ok_to_free = 1;
                break ;
            case FEEL_Key_ID                :
                ParseKeyBinding( &item, pCurr->sub, &(config->feel->FuncKeyRoot) );
                break ;
          default:
				item.ok_to_free = 1;
		}
    }
	ReadConfigItem (&item, NULL);

	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);
	return config;
}

/**********************************************************************
 *  Menu merging/parsing code :
 **********************************************************************/

void
load_feel_menu( ASFeel *feel, char *location )
{
    char *fullfilename ;
    if( location && feel )
    {
        fullfilename = put_file_home( location );
#if 0
        if ( CheckDir (fullfilename) == 0 )
            dir2menu_data( fullfilename, &(feel->menus_list));
        else if ( CheckFile (fullfilename) == 0 )
            file2menu_data( fullfilename, &(feel->menus_list));
        else
            show_error("unable to locate menu at location [%s].", fullfilename);
#endif
        free( fullfilename );
    }
}

void
LoadFeelMenus (FeelConfig *config)
{
    if( config )
    {
        if( config->feel )
        {
/* TODO: add config options to be able to specify menu locations : */
            if( config->menu_locations )
            {
                register int i ;

                for( i = 0 ; i < config->menu_locs_num ; i++ )
                    if( config->menu_locations[i] )
                        load_feel_menu( config->feel, config->menu_locations[i] );
            }
/*		    if( config->feel->menus_list )
				print_ashash( config->feel->menus_list, string_print );
*/
			set_flags( config->feel->set_flags, FEEL_ExternalMenus );
  		}
    }
}

/**********************************************************************
 *  Feel Writing code :
 **********************************************************************/
#if 0
/* returns:
 *            0 on success
 *              1 if data is empty
 *              2 if ConfigWriter cannot be initialized
 *
 */
int
WriteFeelOptions (const char *filename, char *myname,
		  FeelConfig * config, unsigned long flags)
{
	ConfigDef *ConfigWriter = NULL;
    FreeStorageElem *Storage = NULL, **tail = &Storage;
    int i ;

	if (config == NULL)
  		return 1;
	if ((ConfigWriter = InitConfigWriter (myname, &FeelSyntax, CDT_Filename,
			  (void *) filename)) == NULL)
	    return 2;

    CopyFreeStorage (&Storage, config->more_stuff);

	/* building free storage here */
    /* flags : */
    tail = Flags2FreeStorage (&FeelSyntax, tail, FeelFlagsXref, config->feel->flags, config->feel->flags);

    /* integer parameters : */
    if (get_flags (config->feel->set_flags, FEEL_ClickTime))
        tail = Integer2FreeStorage (&FeelSyntax, tail, NULL, config->feel->ClickTime, FEEL_ClickTime_ID);
    if (get_flags (config->feel->set_flags, FEEL_OpaqueMove))
        tail = Integer2FreeStorage (&FeelSyntax, tail, NULL, config->feel->OpaqueMove, FEEL_OpaqueMove_ID);
    if (get_flags (config->feel->set_flags, FEEL_OpaqueResize))
        tail = Integer2FreeStorage (&FeelSyntax, tail, NULL, config->feel->OpaqueResize, FEEL_OpaqueResize_ID);
    if (get_flags (config->feel->set_flags, FEEL_AutoRaise))
        tail = Integer2FreeStorage (&FeelSyntax, tail, NULL, config->feel->AutoRaiseDelay, FEEL_AutoRaise_ID);
    if (get_flags (config->feel->set_flags, FEEL_AutoReverse))
        tail = Integer2FreeStorage (&FeelSyntax, tail, NULL, config->feel->AutoReverse, FEEL_AutoReverse_ID);
    if (get_flags (config->feel->set_flags, FEEL_DeskAnimationType))
        tail = Integer2FreeStorage (&FeelSyntax, tail, NULL, config->feel->DeskAnimationType, FEEL_DeskAnimationType_ID);
    if (get_flags (config->feel->set_flags, FEEL_DeskAnimationSteps))
        tail = Integer2FreeStorage (&FeelSyntax, tail, NULL, config->feel->DeskAnimationSteps, FEEL_DeskAnimationSteps_ID);
    if (get_flags (config->feel->set_flags, FEEL_ShadeAnimationSteps))
        tail = Integer2FreeStorage (&FeelSyntax, tail, NULL, config->feel->ShadeAnimationSteps, FEEL_ShadeAnimationSteps_ID);

    if (get_flags (config->feel->set_flags, FEEL_XorValue))
        tail = Integer2FreeStorage (&FeelSyntax, tail, NULL, config->feel->XorValue, FEEL_XorValue_ID);
    if (get_flags (config->feel->set_flags, FEEL_Xzap))
        tail = Integer2FreeStorage (&FeelSyntax, tail, NULL, config->feel->Xzap, FEEL_Xzap_ID);
    if (get_flags (config->feel->set_flags, FEEL_Yzap))
        tail = Integer2FreeStorage (&FeelSyntax, tail, NULL, config->feel->Yzap, FEEL_Yzap_ID);

    for( i = 0 ; i < MAX_CURSORS ; i++ )
    {
        if (config->feel->standard_cursors[i] != 0)
            tail = Integer2FreeStorage (&FeelSyntax, tail, &i, config->feel->standard_cursors[i], FEEL_Cursor_ID);
        if (config->feel->custom_cursors[i] != NULL)
            tail = ASCursor2FreeStorage (&FeelSyntax, tail, i, config->feel->custom_cursors[i], FEEL_CustomCursor_ID);
    }

    if (get_flags (config->feel->set_flags, FEEL_ClickToRaise))
        Bitlist2FreeStorage( &FeelSyntax, tail, config->feel->RaiseButtons, FEEL_ClickToRaise_ID);

    if (get_flags (config->feel->set_flags, FEEL_EdgeScroll))
        Integer2FreeStorage( &FeelSyntax, tail, &(config->feel->EdgeScrollX), config->feel->EdgeScrollY, FEEL_EdgeScroll_ID);
    if (get_flags (config->feel->set_flags, FEEL_EdgeResistance))
        Integer2FreeStorage( &FeelSyntax, tail, &(config->feel->EdgeResistanceScroll), config->feel->EdgeResistanceMove, FEEL_EdgeResistance_ID);

    /* complex functions : */
    if( config->feel->funcs_list )
        if( config->feel->funcs_list->items_num )
        {
            ComplexFunction **list ;

            list = safecalloc( config->feel->funcs_list->items_num, sizeof(ComplexFunction*));
            if( (i = sort_hash_items( config->feel->funcs_list, NULL, (void**)list, 0 )) > 0 )
                while ( --i >= 0 )
                    tail = ComplexFunction2FreeStorage( &FeelSyntax, tail, list[i]);
            free( list );
        }
    /* menus */
    /* menus writing require additional work due to the nature of the problem -
     * menus can come from file or directory or from feel file itself.
     * plus there has to be some mechanismus in place in order to preserve
     * .include configuration when menu is saved as directory.
     *          Sasha.
     */
    /* this is preliminary code that only saves menus into feel file itself  - make sure
     * you remove all the other menus from the feel->menus_list prior to calling this */
    if( config->feel->menus_list )
        if( config->feel->menus_list->items_num )
        {
            MenuData **list ;
            register int i ;
            list = safecalloc( config->feel->menus_list->items_num, sizeof(MenuData*));
            if( (i = sort_hash_items( config->feel->menus_list, NULL, (void**)list, 0 )) > 0 )
                while ( --i >= 0 )
                    tail = MenuData2FreeStorage( &FeelSyntax, tail, list[i]);
            free( list );
        }
    /* mouse bindings : */
    for( i = 0 ; i < MAX_MOUSE_BUTTONS+1 ; i++ )
    {
        char btn_id[2] ;
        btn_id[0] = '0'+i ;
        btn_id[1] = '\0' ;
        tail = Contexts2FreeStorage( &FeelSyntax, tail, btn_id, &(config->feel->mouse[i]), FEEL_Mouse_ID );
    }

    /* keyboard bindings : */
    if( config->feel->keyboard )
        if( config->feel->keyboard->items_num > 0 )
        {
            ASHashableValue  *keys;
            ASInputContexts **contexts;
            unsigned int items_num = config->feel->keyboard->items_num ;

            keys = safecalloc( items_num, sizeof(ASHashableValue) );
            contexts = safecalloc( items_num, sizeof(ASInputContexts*) );
            items_num = sort_hash_items( config->feel->keyboard, keys, (void**)contexts, 0 );
            for( i = 0 ; i < items_num ; i++ )
            {
                KeySym keysym = (KeySym)keys[i];
                tail = Contexts2FreeStorage( &FeelSyntax, tail, XKeysymToString(keysym), contexts[i], FEEL_Key_ID );
            }
        }

    /* writing config into the file */
	WriteConfig (ConfigWriter, &Storage, CDT_Filename, (void **) &filename, flags);
    DestroyConfig (ConfigWriter);

	if (Storage)
  	{
    	fprintf (stderr,
	  		     "\n%s:Config Writing warning: Not all Free Storage discarded! Trying again...",
	      		 myname);
    	DestroyFreeStorage (&Storage);
  		fprintf (stderr, (Storage != NULL) ? " failed." : " success.");
    }
	return 0;
}

#endif
/********************************************************************************/
/*   AutoExec config :                                                          */
/********************************************************************************/

AutoExecConfig *
CreateAutoExecConfig ()
{
  AutoExecConfig *config = (AutoExecConfig *) safecalloc (1, sizeof (AutoExecConfig));
  return config;
}

void
DestroyAutoExecConfig (AutoExecConfig * config)
{
    if( config->init )
        really_destroy_complex_func( config->init );
    if( config->restart )
        really_destroy_complex_func( config->restart );

    DestroyFreeStorage (&(config->more_stuff));
    free (config);
}

AutoExecConfig *
ParseAutoExecOptions (const char *filename, char *myname)
{
    ConfigDef *ConfigReader =
        InitConfigReader (myname, &AutoExecSyntax, CDT_Filename, (void *) filename, NULL);
    AutoExecConfig *config = CreateAutoExecConfig ();
	FreeStorageElem *Storage = NULL, *pCurr;
    ConfigItem item;

	if (!ConfigReader)
  		return config;

    item.memory = NULL;
	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &Storage);
    PrintFreeStorage (Storage);

	/* getting rid of all the crap first */
    StorageCleanUp (&Storage, &(config->more_stuff), CF_DISABLED_OPTION);

    for (pCurr = Storage; pCurr; pCurr = pCurr->next)
  	{
  	    if (pCurr->term == NULL)
			continue;
        if (!ReadConfigItem (&item, pCurr))
			continue;

        switch (pCurr->term->id)
		{
            case FEEL_Function_ID           :
                if( mystrncasecmp( item.data.string, "InitFunction", 12 ) == 0 )
                {
                    if( config->init )
                        really_destroy_complex_func( config->init );
                     config->init = FreeStorage2ComplexFunction( pCurr, &item, NULL );
                }else if( mystrncasecmp( item.data.string, "RestartFunction", 15 ) == 0 )
                {
                    if( config->restart )
                        really_destroy_complex_func( config->restart );
                    config->restart = FreeStorage2ComplexFunction( pCurr, &item, NULL );
                }
                break ;
          default:
				item.ok_to_free = 1;
		}
    }
	ReadConfigItem (&item, NULL);

	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);
	return config;

}

int
WriteAutoExecOptions (const char *filename, char *myname,  AutoExecConfig * config, unsigned long flags)
{
    ConfigDef *ConfigWriter = NULL;
    FreeStorageElem *Storage = NULL, **tail = &Storage;

	if (config == NULL)
  		return 1;
    if ((ConfigWriter = InitConfigWriter (myname, &AutoExecSyntax, CDT_Filename,
			  (void *) filename)) == NULL)
	    return 2;

    CopyFreeStorage (&Storage, config->more_stuff);

    if( config->init )
        tail = ComplexFunction2FreeStorage( &AutoExecSyntax, tail, config->init );
    if(config->restart)
        tail = ComplexFunction2FreeStorage( &AutoExecSyntax, tail, config->restart );

    /* writing config into the file */
	WriteConfig (ConfigWriter, &Storage, CDT_Filename, (void **) &filename, flags);
    DestroyConfig (ConfigWriter);

	if (Storage)
  	{
    	fprintf (stderr,
	  		     "\n%s:Config Writing warning: Not all Free Storage discarded! Trying again...",
	      		 myname);
    	DestroyFreeStorage (&Storage);
  		fprintf (stderr, (Storage != NULL) ? " failed." : " success.");
    }
	return 0;
}

