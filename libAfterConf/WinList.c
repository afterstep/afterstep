/*
 * Copyright (c) 2002 Sasha Vasko <sasha at aftercode.net>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include "../configure.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/mystyle.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/balloon.h"

#include "afterconf.h"

/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/

extern SyntaxDef AlignSyntax;


#define WINLIST_FEEL_TERMS \
    ASCF_DEFINE_KEYWORD(WINLIST, TF_DONT_SPLIT	, Action		, TT_TEXT, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0				, FillRowsFirst	, TT_FLAG, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0				, UseSkipList	, TT_FLAG, NULL)

#define WINLIST_LOOK_TERMS \
    ASCF_DEFINE_KEYWORD(WINLIST, 0				, UseName			, TT_INTEGER, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , Align				, TT_FLAG	, &AlignSyntax), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , Bevel				, TT_FLAG	, &BevelSyntax), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , FBevel			, TT_FLAG	, &BevelSyntax), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , UBevel			, TT_FLAG	, &BevelSyntax), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , SBevel			, TT_FLAG	, &BevelSyntax), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , FocusedBevel		, TT_FLAG	, &BevelSyntax), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , StickyBevel		, TT_FLAG	, &BevelSyntax), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , UnfocusedBevel	, TT_FLAG	, &BevelSyntax), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , CompositionMethod	, TT_UINTEGER, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , FCompositionMethod, TT_UINTEGER, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , UCompositionMethod, TT_UINTEGER, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , SCompositionMethod, TT_UINTEGER, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , Spacing			, TT_UINTEGER, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , HSpacing			, TT_UINTEGER, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , VSpacing			, TT_UINTEGER, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , UnfocusedStyle	, TT_TEXT	,  NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , FocusedStyle		, TT_TEXT	,  NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , StickyStyle		, TT_TEXT	,  NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , ShapeToContents	, TT_FLAG	,  NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , ShowIcon			, TT_FLAG	,  NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , IconLocation		, TT_UINTEGER, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , IconAlign			, TT_FLAG	, &AlignSyntax), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , IconSize			, TT_GEOMETRY, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , ScaleIconToTextHeight, TT_FLAG,  NULL)
	
#define WINLIST_PRIVATE_TERMS \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , Geometry			, TT_GEOMETRY	, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , MinSize			, TT_GEOMETRY	, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , MaxSize			, TT_GEOMETRY	, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , MaxRows			, TT_INTEGER	, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , MaxColumns		, TT_INTEGER	, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , MaxColWidth		, TT_INTEGER	, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , MinColWidth		, TT_INTEGER	, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, TF_OBSOLETE	, MaxWidth			, TT_UINTEGER	, NULL)


TermDef       WinListFeelTerms[] = {
	/* Feel */
	WINLIST_FEEL_TERMS,
	BALLOON_FEEL_TERMS,
	{0, NULL, 0, 0, 0}
};

TermDef       WinListLookTerms[] = {
    /* Look */
	WINLIST_LOOK_TERMS,
/* now special cases that should be processed by it's own handlers */
    BALLOON_LOOK_TERMS,
	{0, NULL, 0, 0, 0}
};

TermDef       WinListPrivateTerms[] = {
	WINLIST_PRIVATE_TERMS,
	BALLOON_FLAG_TERM,
	{0, NULL, 0, 0, 0}
};

TermDef       WinListTerms[] = {
	WINLIST_PRIVATE_TERMS,
	/* Feel */
	WINLIST_FEEL_TERMS,
	/* Look */
	WINLIST_LOOK_TERMS,
    {TF_OBSOLETE, "Orientation", 11, TT_TEXT, WINLIST_Orientation_ID, NULL},
/* including MyStyles definitions processing */
	INCLUDE_MYSTYLE,
/* now special cases that should be processed by it's own handlers */
    BALLOON_TERMS,
    {0, NULL, 0, 0, 0}
};


SyntaxDef WinListFeelSyntax 	= {'\n', '\0', WinListFeelTerms, 	0, '\t', "", "\t", "WinListFeel", 	"WinListFeel", "AfterStep window list module feel", NULL, 0};
SyntaxDef WinListLookSyntax 	= {'\n', '\0', WinListLookTerms, 	0, '\t', "", "\t", "WinListLook", 	"WinListLook", "AfterStep window list module look", NULL, 0};
SyntaxDef WinListPrivateSyntax 	= {'\n', '\0', WinListPrivateTerms,	0, '\t', "", "\t", "WinList",     	"WinList",     "AfterStep window list module", NULL,0};
SyntaxDef WinListSyntax 		= {'\n', '\0', WinListTerms, 		0, ' ',  "", "\t", "Module:WinList","WinList",     "AfterStep module displaying list of opened windows",NULL,0};

WinListConfig *
CreateWinListConfig ()
{
	WinListConfig *config = (WinListConfig *) safecalloc (1, sizeof (WinListConfig));

	config->flags = WINLIST_ShowIcon|WINLIST_ScaleIconToTextHeight ;
    init_asgeometry (&(config->Geometry));
    config->gravity = NorthWestGravity;
	config->MaxRows = 1;
	config->UseName = ASN_Name;
    config->Align = ALIGN_CENTER;
    config->balloon_conf = NULL;
    config->HSpacing = DEFAULT_TBAR_HSPACING;
	config->VSpacing = DEFAULT_TBAR_VSPACING;
    config->FBevel = config->UBevel = config->SBevel = DEFAULT_TBAR_HILITE ;
	config->IconAlign = ALIGN_VCENTER ;
	config->IconLocation = 0 ;
	return config;
}

void
DestroyWinListConfig (WinListConfig * config)
{
	int           i;

	destroy_string(&(config->UnfocusedStyle));
	destroy_string(&(config->FocusedStyle));
	destroy_string(&(config->StickyStyle));

	for (i = 0; i < MAX_MOUSE_BUTTONS; ++i)
		if (config->Action[i])
            destroy_string_list(config->Action[i]);

    Destroy_balloonConfig (config->balloon_conf);
	DestroyFreeStorage (&(config->more_stuff));
	DestroyMyStyleDefinitions (&(config->style_defs));

	free (config);
}

void
PrintWinListConfig (WinListConfig * config)
{
	int           i;

	fprintf (stderr, "WinListConfig = %p;\n", config);
	if (config == NULL)
		return;

	ASCF_PRINT_FLAG_KEYWORD(stderr,WINLIST,config,FillRowsFirst);
	ASCF_PRINT_FLAG_KEYWORD(stderr,WINLIST,config,UseSkipList);
	ASCF_PRINT_GEOMETRY_KEYWORD(stderr,WINLIST,config,Geometry);
	ASCF_PRINT_SIZE_KEYWORD(stderr,WINLIST,config,MinSize);
	ASCF_PRINT_SIZE_KEYWORD(stderr,WINLIST,config,MaxSize);
	ASCF_PRINT_INT_KEYWORD(stderr,WINLIST,config,MaxRows);
	ASCF_PRINT_INT_KEYWORD(stderr,WINLIST,config,MaxColumns);
	ASCF_PRINT_INT_KEYWORD(stderr,WINLIST,config,MinColWidth);
	ASCF_PRINT_INT_KEYWORD(stderr,WINLIST,config,MaxColWidth);

	ASCF_PRINT_INT_KEYWORD(stderr,WINLIST,config,UseName);
	ASCF_PRINT_FLAGS_KEYWORD(stderr,WINLIST,config,Align );
	ASCF_PRINT_FLAGS_KEYWORD(stderr,WINLIST,config,FBevel);
	ASCF_PRINT_FLAGS_KEYWORD(stderr,WINLIST,config,UBevel);
	ASCF_PRINT_FLAGS_KEYWORD(stderr,WINLIST,config,SBevel);
	ASCF_PRINT_INT_KEYWORD(stderr,WINLIST,config,FCompositionMethod);
	ASCF_PRINT_INT_KEYWORD(stderr,WINLIST,config,UCompositionMethod);
	ASCF_PRINT_INT_KEYWORD(stderr,WINLIST,config,SCompositionMethod);
	ASCF_PRINT_INT_KEYWORD(stderr,WINLIST,config,HSpacing);
	ASCF_PRINT_INT_KEYWORD(stderr,WINLIST,config,VSpacing);

	ASCF_PRINT_STRING_KEYWORD(stderr,WINLIST,config,UnfocusedStyle);
	ASCF_PRINT_STRING_KEYWORD(stderr,WINLIST,config,FocusedStyle);
	ASCF_PRINT_STRING_KEYWORD(stderr,WINLIST,config,StickyStyle);
	
	ASCF_PRINT_INT_KEYWORD(stderr,WINLIST,config,IconLocation);	
	ASCF_PRINT_FLAGS_KEYWORD(stderr,WINLIST,config,IconAlign );
	ASCF_PRINT_SIZE_KEYWORD(stderr,WINLIST,config,IconSize);	
	
	fprintf (stderr, "WinListConfig.gravity = %d;\n", config->gravity);

	for (i = 0; i < MAX_MOUSE_BUTTONS; ++i)
        ASCF_PRINT_IDX_COMPOUND_STRING_KEYWORD(stderr,WINLIST,config,Action,',',i);
}

flag_options_xref WinListFlags[] = {
	{WINLIST_FillRowsFirst, 		WINLIST_FillRowsFirst_ID, 0 },
	{WINLIST_UseSkipList, 			WINLIST_UseSkipList_ID, 0 },
    {WINLIST_ShapeToContents, 		WINLIST_ShapeToContents_ID, 0},
    {WINLIST_ShowIcon, 				WINLIST_ShowIcon_ID, 0},
    {WINLIST_ScaleIconToTextHeight, WINLIST_ScaleIconToTextHeight_ID, 0},
    {0, 0, 0}
};


WinListConfig *
ParseWinListOptions (const char *filename, char *myname)
{
	ConfigData    cd ;
	ConfigDef    *ConfigReader;
	WinListConfig *config = CreateWinListConfig ();

	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem    item;
	MyStyleDefinition **styles_tail = &(config->style_defs);

	cd.filename = filename ;
	ConfigReader = InitConfigReader (myname, &WinListSyntax, CDT_Filename, cd, NULL);
	if (!ConfigReader)
		return config;

	item.memory = NULL;
	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &Storage);

	/* getting rid of all the crap first */
	StorageCleanUp (&Storage, &(config->more_stuff), CF_DISABLED_OPTION);

    config->balloon_conf = Process_balloonOptions (Storage, NULL);

    for (pCurr = Storage; pCurr; pCurr = pCurr->next)
	{
		if (pCurr->term == NULL)
			continue;

		if (pCurr->term->type == TT_FLAG)
        {
	        if (ReadFlagItem (&(config->set_flags), &(config->flags), pCurr, WinListFlags))
        	    continue;

            switch (pCurr->term->id)
            {
				ASCF_HANDLE_ALIGN_KEYWORD_CASE(WINLIST,config,pCurr,Align ); 
                ASCF_HANDLE_BEVEL_KEYWORD_CASE(WINLIST,config,pCurr,FBevel); 
                ASCF_HANDLE_BEVEL_KEYWORD_CASE(WINLIST,config,pCurr,UBevel); 
                ASCF_HANDLE_BEVEL_KEYWORD_CASE(WINLIST,config,pCurr,SBevel); 
				ASCF_HANDLE_ALIGN_KEYWORD_CASE(WINLIST,config,pCurr,IconAlign);
				default:
					if( pCurr->term->id == WINLIST_UseIconNames_ID ) 
					{
	                    if (!get_flags (config->set_flags, WINLIST_UseName))
    	                    set_scalar_value(&(config->UseName),ASN_IconName, 
										 	 &(config->set_flags), WINLIST_UseName);
					}else if( pCurr->term->id ==  WINLIST_Bevel_ID )
					{	
						set_scalar_value(&(config->FBevel),ParseBevelOptions( pCurr->sub ), 
										 &(config->set_flags), WINLIST_Bevel);
                    	config->UBevel = config->SBevel = config->FBevel;
					}	
            }
        }else
		{
			if (!ReadConfigItem (&item, pCurr))
				continue;

			switch (pCurr->term->id)
			{
                ASCF_HANDLE_GEOMETRY_KEYWORD_CASE(WINLIST,config,item,Geometry); 
                ASCF_HANDLE_SIZE_KEYWORD_CASE(WINLIST,config,item,MinSize); 
                ASCF_HANDLE_SIZE_KEYWORD_CASE(WINLIST,config,item,MaxSize); 
                ASCF_HANDLE_INTEGER_KEYWORD_CASE(WINLIST,config,item,MaxRows); 
                ASCF_HANDLE_INTEGER_KEYWORD_CASE(WINLIST,config,item,MaxColumns); 
				ASCF_HANDLE_INTEGER_KEYWORD_CASE(WINLIST,config,item,MinColWidth); 
				ASCF_HANDLE_INTEGER_KEYWORD_CASE(WINLIST,config,item,MaxColWidth); 
				ASCF_HANDLE_INTEGER_KEYWORD_CASE(WINLIST,config,item,UseName); 
                
				ASCF_HANDLE_STRING_KEYWORD_CASE(WINLIST,config,item,UnfocusedStyle); 
                ASCF_HANDLE_STRING_KEYWORD_CASE(WINLIST,config,item,FocusedStyle); 
                ASCF_HANDLE_STRING_KEYWORD_CASE(WINLIST,config,item,StickyStyle); 
    			
				ASCF_HANDLE_INTEGER_KEYWORD_CASE(WINLIST,config,item,FCompositionMethod); 
				ASCF_HANDLE_INTEGER_KEYWORD_CASE(WINLIST,config,item,UCompositionMethod); 
				ASCF_HANDLE_INTEGER_KEYWORD_CASE(WINLIST,config,item,SCompositionMethod); 
                
				ASCF_HANDLE_INTEGER_KEYWORD_CASE(WINLIST,config,item,HSpacing); 
				ASCF_HANDLE_INTEGER_KEYWORD_CASE(WINLIST,config,item,VSpacing); 
				ASCF_HANDLE_INTEGER_KEYWORD_CASE(WINLIST,config,item,IconLocation); 
                ASCF_HANDLE_SIZE_KEYWORD_CASE(WINLIST,config,item,IconSize); 
				default:
					switch (pCurr->term->id)
					{						                
						case WINLIST_Spacing_ID :
                    		set_flags( config->set_flags, WINLIST_Spacing );
                    		config->HSpacing = config->VSpacing = item.data.integer;
                    		break ;
		                case WINLIST_CompositionMethod_ID :
        		            set_flags( config->set_flags, WINLIST_CompositionMethod );
                		    config->FCompositionMethod = 
								config->UCompositionMethod =
								config->SCompositionMethod = item.data.integer;
                    		break ;
						case MYSTYLE_START_ID:
                    		styles_tail = ProcessMyStyleOptions (pCurr->sub, styles_tail);
                    		item.ok_to_free = 1;
                    		break;
	    	            case WINLIST_MaxWidth_ID:
    	    	            set_flags (config->set_flags, WINLIST_MinSize|WINLIST_MaxSize);
                	    	config->MaxSize.width = config->MinSize.width = item.data.integer;
                    		break;

                		case WINLIST_Action_ID:
                    		{
                        		char         *ptr = item.data.string;
                        		int           action_no = 0, i;

                        		if (mystrncasecmp (ptr, "Click", 5) == 0)
                            		ptr += 5;
                        		if (isdigit (ptr[0]))
                        		{
                            		action_no = atoi (ptr);
                            		if (action_no <= 0)
                                		action_no = 1;
                            		--action_no;
                            		action_no %= MAX_MOUSE_BUTTONS;
                            		i = 0;
                            		while (!isspace (ptr[i]) && ptr[i])
                                		++i;
                            		while (isspace (ptr[i]) && ptr[i])
                                		++i;
                            		ptr += i;
                        		}
                        		if (*ptr)
                        		{
                            		destroy_string_list( config->Action[action_no] );
                            		config->Action[action_no] = comma_string2list( ptr );
                        		}
                        		item.ok_to_free = 1;
                    		}
                    		break;
		                default:
        		            item.ok_to_free = 1;
					}
			}

		}
	}
	
	ReadConfigItem (&item, NULL);
	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);
/*    PrintMyStyleDefinitions( config->style_defs ); */
	return config;

}

void
MergeWinListOptions ( WinListConfig *to, WinListConfig *from)
{
    int i ;
    START_TIME(option_time);

    /* Need to merge new config with what we have already :*/
    /* now lets check the config sanity : */
    /* mixing set and default flags : */
    ASCF_MERGE_FLAGS(to,from);
    
	ASCF_MERGE_GEOMETRY_KEYWORD(WINLIST, to, from, Geometry);
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, MinSize);
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, MaxSize);
    
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, MaxRows);
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, MaxColumns);
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, MaxColWidth);
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, MinColWidth);

	ASCF_MERGE_STRING_KEYWORD(WINLIST, to, from, FocusedStyle);
	ASCF_MERGE_STRING_KEYWORD(WINLIST, to, from, UnfocusedStyle);
	ASCF_MERGE_STRING_KEYWORD(WINLIST, to, from, StickyStyle);

	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, UseName);
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, Align);
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, FBevel);
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, UBevel);
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, SBevel);

	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, FCompositionMethod);
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, UCompositionMethod);
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, SCompositionMethod);

	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, HSpacing);
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, VSpacing);

	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, IconAlign);
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, IconLocation);
	ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, IconSize);

    for( i = 0 ; i < MAX_MOUSE_BUTTONS ; ++i )
        if( from->Action[i] )
        {
            destroy_string_list( to->Action[i] );
            to->Action[i] = from->Action[i];
        }

    if( to->balloon_conf )
        Destroy_balloonConfig( to->balloon_conf );
    to->balloon_conf = from->balloon_conf ;
    from->balloon_conf = NULL ;

    SHOW_TIME("to parsing",option_time);
}

void
CheckWinListConfigSanity(WinListConfig *Config, ASGeometry *default_geometry, int default_gravity)
{
    if( Config == NULL )
        Config = CreateWinListConfig ();

    if( Config->MaxRows > MAX_WINLIST_WINDOW_COUNT || Config->MaxRows == 0  )
        Config->MaxRows = MAX_WINLIST_WINDOW_COUNT;

    if( Config->MaxColumns > MAX_WINLIST_WINDOW_COUNT || Config->MaxColumns == 0  )
        Config->MaxColumns = MAX_WINLIST_WINDOW_COUNT;

	if( default_geometry && default_geometry->flags != 0 ) 
		Config->Geometry = *default_geometry ;
	
    Config->gravity = NorthWestGravity ;
    if( get_flags(Config->Geometry.flags, XNegative) )
        Config->gravity = get_flags(Config->Geometry.flags, YNegative)? SouthEastGravity:NorthEastGravity;
    else if( get_flags(Config->Geometry.flags, YNegative) )
        Config->gravity = SouthWestGravity;

	if(default_gravity != ForgetGravity)
    	Config->gravity = default_gravity ;

    Config->anchor_x = get_flags( Config->Geometry.flags, XValue )?Config->Geometry.x:0;
    if( get_flags(Config->Geometry.flags, XNegative) )
        Config->anchor_x += Scr.MyDisplayWidth ;

    Config->anchor_y = get_flags( Config->Geometry.flags, YValue )?Config->Geometry.y:0;
    if( get_flags(Config->Geometry.flags, YNegative) )
        Config->anchor_y += Scr.MyDisplayHeight ;

	Config->UseName %= ASN_NameTypes;

	if( !get_flags( Config->set_flags, WINLIST_IconLocation ) )
	{
		if( get_flags(Config->Align, PAD_H_MASK) == ALIGN_RIGHT ) 
			Config->IconLocation = 2 ; 
	}		
	if( !get_flags( Config->set_flags, WINLIST_IconAlign ) )
	{
	   	if( get_flags(Config->Align, PAD_H_MASK) == PAD_H_MASK ) 
			if( Config->IconLocation == 0 || Config->IconLocation == 4 || Config->IconLocation == 7) 	  
				Config->IconAlign = ALIGN_RIGHT ; 
	}	 
}


#if 0
/* returns:
 *            0 on success
 *              1 if data is empty
 *              2 if toWriter cannot be initialized
 *
 */
int
WriteWinListOptions (const char *filename, char *myname, WinListto * config, unsigned long flags)
{
	toDef    *PagertoWriter = NULL;
	FreeStorageElem *Storage = NULL, **tail = &Storage;
	char         *Decorations = NULL;

	if (config == NULL)
		return 1;
	if ((PagertoWriter = InittoWriter (myname, &PagerSyntax, CDT_Filename, (void *)filename)) == NULL)
		return 2;

	CopyFreeStorage (&Storage, config->more_stuff);

    if (config->balloon_conf)
		tail = balloon2FreeStorage (&WinListSyntax, tail, config->balloon_conf);

	/* building free storage here */
	/* geometry */
	tail = Geometry2FreeStorage (&PagerSyntax, tail, &(config->geometry), PAGER_GEOMETRY_ID);
	/* icon_geometry */
	tail = Geometry2FreeStorage (&PagerSyntax, tail, &(config->icon_geometry), PAGER_ICON_GEOMETRY_ID);
	/* labels */
	tail = StringArray2FreeStorage (&PagerSyntax, tail, config->labels, desk1, desk2, PAGER_LABEL_ID);
	/* styles */
	tail = StringArray2FreeStorage (&PagerSyntax, tail, config->styles, desk1, desk2, PAGER_STYLE_ID);
	/* align */
	tail = Integer2FreeStorage (&PagerSyntax, tail, config->align, PAGER_ALIGN_ID);
	/* flags */
	if (!(config->flags & REDRAW_BG))
		tail = Flag2FreeStorage (&PagerSyntax, tail, PAGER_DRAW_BG_ID);
	if (config->flags & START_ICONIC)
		tail = Flag2FreeStorage (&PagerSyntax, tail, PAGER_START_ICONIC_ID);
	if (config->flags & FAST_STARTUP)
		tail = Flag2FreeStorage (&PagerSyntax, tail, PAGER_FAST_STARTUP_ID);
	if (config->flags & SET_ROOT_ON_STARTUP)
		tail = Flag2FreeStorage (&PagerSyntax, tail, PAGER_SET_ROOT_ID);
	if (config->flags & STICKY_ICONS)
		tail = Flag2FreeStorage (&PagerSyntax, tail, PAGER_STICKY_ICONS_ID);

	/* small_font_name */
	tail = String2FreeStorage (&PagerSyntax, tail, config->small_font_name, PAGER_SMALL_FONT_ID);
	/* rows */
	tail = Integer2FreeStorage (&PagerSyntax, tail, config->rows, PAGER_ROWS_ID);
	/* columns */
	tail = Integer2FreeStorage (&PagerSyntax, tail, config->columns, PAGER_COLUMNS_ID);

	/* now storing PagerDecorations */
	{
		toDef    *Decorto = InittoWriter (myname, &PagerDecorationSyntax, CDT_Data, NULL);
		FreeStorageElem *DecorStorage = NULL, **d_tail = &DecorStorage;

		/* flags */
		if (!(config->flags & ~USE_LABEL))
			d_tail = Flag2FreeStorage (&PagerDecorationSyntax, d_tail, PAGER_DECOR_NOLABEL_ID);
		if (!(config->flags & PAGE_SEPARATOR))
			d_tail = Flag2FreeStorage (&PagerDecorationSyntax, d_tail, PAGER_DECOR_NOSEPARATOR_ID);
		if (!(config->flags & SHOW_SELECTION))
			d_tail = Flag2FreeStorage (&PagerDecorationSyntax, d_tail, PAGER_DECOR_NOSELECTION_ID);
		if (config->flags & LABEL_BELOW_DESK)
			d_tail = Flag2FreeStorage (&PagerDecorationSyntax, d_tail, PAGER_DECOR_LABEL_BELOW_ID);
		if (config->flags & HIDE_INACTIVE_LABEL)
			d_tail = Flag2FreeStorage (&PagerDecorationSyntax, d_tail, PAGER_DECOR_HIDE_INACTIVE_ID);

		/* selection_color */
		d_tail = String2FreeStorage (&PagerDecorationSyntax, d_tail, config->selection_color, PAGER_DECOR_SEL_COLOR_ID);
		/* border_width */
		d_tail =
			Integer2FreeStorage (&PagerDecorationSyntax, d_tail, config->border_width, PAGER_DECOR_BORDER_WIDTH_ID);
		/* grid_color */
		d_tail = String2FreeStorage (&PagerDecorationSyntax, d_tail, config->grid_color, PAGER_DECOR_GRID_COLOR_ID);
		/* border_color */
		d_tail = String2FreeStorage (&PagerDecorationSyntax, d_tail, config->border_color, PAGER_DECOR_BORDER_COLOR_ID);

		Writeto (Decorto, &DecorStorage, CDT_Data, (void **)&Decorations, 0);
		Destroyto (Decorto);
		if (DecorStorage)
			DestroyFreeStorage (&DecorStorage);

		tail = String2FreeStorage (&PagerSyntax, tail, Decorations, PAGER_DECORATION_ID);
		if (Decorations)
		{
			free (Decorations);
			Decorations = NULL;
		}
	}

	/* writing config into the file */
	Writeto (PagertoWriter, &Storage, CDT_Filename, (void **)&filename, flags);
	Destroyto (PagertoWriter);

	if (Storage)
	{
		fprintf (stderr, "\n%s:to Writing warning: Not all Free Storage discarded! Trying again...", myname);
		DestroyFreeStorage (&Storage);
		fprintf (stderr, (Storage != NULL) ? " failed." : " success.");
	}
	return 0;
}

#endif
