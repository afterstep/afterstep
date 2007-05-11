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
#define LOCAL_DEBUG

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
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0				, UseName			, TT_INTEGER, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , Align				, TT_FLAG	, &AlignSyntax,WinListConfig), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , Bevel				, TT_FLAG	, &BevelSyntax), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , FBevel			, TT_FLAG	, &BevelSyntax,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , UBevel			, TT_FLAG	, &BevelSyntax,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , SBevel			, TT_FLAG	, &BevelSyntax,WinListConfig), \
    ASCF_DEFINE_KEYWORD_SA(WINLIST, 0			    , FocusedBevel		, TT_FLAG	, &BevelSyntax,WinListConfig,FBevel), \
    ASCF_DEFINE_KEYWORD_SA(WINLIST, 0			    , StickyBevel		, TT_FLAG	, &BevelSyntax,WinListConfig,SBevel), \
    ASCF_DEFINE_KEYWORD_SA(WINLIST, 0			    , UnfocusedBevel	, TT_FLAG	, &BevelSyntax,WinListConfig,UBevel), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , CompositionMethod	, TT_UINTEGER, NULL), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , FCompositionMethod, TT_UINTEGER, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , UCompositionMethod, TT_UINTEGER, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , SCompositionMethod, TT_UINTEGER, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , Spacing			, TT_UINTEGER, NULL), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , HSpacing			, TT_UINTEGER, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , VSpacing			, TT_UINTEGER, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, TF_QUOTES_OPTIONAL, UnfocusedStyle	, TT_QUOTED_TEXT	,  NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, TF_QUOTES_OPTIONAL, FocusedStyle		, TT_QUOTED_TEXT	,  NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, TF_QUOTES_OPTIONAL, StickyStyle		, TT_QUOTED_TEXT	,  NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, TF_QUOTES_OPTIONAL, UrgentStyle		, TT_QUOTED_TEXT	,  NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , ShapeToContents	, TT_FLAG	,  NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , ShowIcon			, TT_FLAG	,  NULL), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , IconLocation		, TT_UINTEGER, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , IconAlign			, TT_FLAG	, &AlignSyntax,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , IconSize			, TT_GEOMETRY, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD(WINLIST, 0			    , ScaleIconToTextHeight, TT_FLAG,  NULL), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0					,ShowHints  	, TT_FLAG		, &BalloonContentsSyntax,WinListConfig)
	
#define WINLIST_PRIVATE_TERMS \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , Geometry			, TT_GEOMETRY	, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , MinSize			, TT_GEOMETRY	, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , MaxSize			, TT_GEOMETRY	, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , MaxRows			, TT_INTEGER	, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , MaxColumns		, TT_INTEGER	, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , MaxColWidth		, TT_INTEGER	, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , MinColWidth		, TT_INTEGER	, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , MinColWidth		, TT_INTEGER	, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD_S(WINLIST, 0			    , NoCollidesSpacing	, TT_INTEGER	, NULL,WinListConfig), \
    ASCF_DEFINE_KEYWORD(WINLIST, TF_DONT_SPLIT		, NoCollides	    , TT_TEXT		, NULL), \
    ASCF_DEFINE_KEYWORD(WINLIST, TF_DONT_SPLIT		, AllowCollides	    , TT_TEXT		, NULL), \
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

#define WINLIST_ALL_TERMS	WINLIST_PRIVATE_TERMS,WINLIST_FEEL_TERMS, WINLIST_LOOK_TERMS


TermDef       WinListDefaultsTerms[] = {

	WINLIST_ALL_TERMS,
    {TF_OBSOLETE, "Orientation", 11, TT_TEXT, WINLIST_Orientation_ID, NULL},
/* now special cases that should be processed by it's own handlers */
    BALLOON_TERMS,
	INCLUDE_MODULE_DEFAULTS_END,
    {0, NULL, 0, 0, 0}
};

SyntaxDef WinListDefaultsSyntax	= {'\n', '\0', WinListDefaultsTerms, 		0, ' ',  "", "\t", "Module:WinList_defaults","WinList",     "AfterStep module displaying list of opened windows",NULL,0};


TermDef       WinListTerms[] = {
	INCLUDE_MODULE_DEFAULTS(&WinListDefaultsSyntax),

	WINLIST_ALL_TERMS,
	
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

/*
	int        type ; 
	ASFlagType flags ;
	
	char *private_config_file ;
	
	ASModuleConfig* (*create_config_func)();
	void  (*free_storage2config_func)(ASModuleConfig *config, struct FreeStorageElem *storage);
	void  (*merge_config_func)( ASModuleConfig *to, ASModuleConfig *from);
	void  (*destroy_config_func)( ASModuleConfig *config);
	
	struct SyntaxDef *module_syntax ; 
	struct SyntaxDef *look_syntax ; 
	struct SyntaxDef *feel_syntax ; 

	struct flag_options_xref *flags_xref ;
	ptrdiff_t 		   set_flags_field_offset ;
*/

flag_options_xref WinListFlags[] = {
	ASCF_DEFINE_MODULE_FLAG_XREF(WINLIST,FillRowsFirst,WinListConfig),
	ASCF_DEFINE_MODULE_FLAG_XREF(WINLIST,UseSkipList,WinListConfig),
	ASCF_DEFINE_MODULE_FLAG_XREF(WINLIST,ShapeToContents,WinListConfig),
	ASCF_DEFINE_MODULE_FLAG_XREF(WINLIST,ShowIcon,WinListConfig),
	ASCF_DEFINE_MODULE_FLAG_XREF(WINLIST,ScaleIconToTextHeight,WinListConfig),
    {0, 0, 0}
};

static void InitWinListConfig (ASModuleConfig *asm_config, Bool free_resources);
void WinList_fs2config( ASModuleConfig *asmodule_config, FreeStorageElem *Storage );
void MergeWinListOptions ( ASModuleConfig *to, ASModuleConfig *from);


static ASModuleConfigClass _winlist_config_class = 
{	CONFIG_WinList_ID,
 	0,
	sizeof(WinListConfig),
 	"winlist",
 	InitWinListConfig,
	WinList_fs2config,
	MergeWinListOptions,
	&WinListSyntax,
	&WinListLookSyntax,
	&WinListFeelSyntax,
	WinListFlags,
	offsetof(WinListConfig,set_flags),
	
	ASDefaultBalloonTypes
 };
 
ASModuleConfigClass *WinListConfigClass = &_winlist_config_class;


static void
InitWinListConfig (ASModuleConfig *asm_config, Bool free_resources)
{
	WinListConfig *config = AS_WINLIST_CONFIG(asm_config);
	if( config ) 
	{
		if( free_resources ) 
		{
			int i ;
			destroy_string(&(config->UnfocusedStyle));
			destroy_string(&(config->FocusedStyle));
			destroy_string(&(config->StickyStyle));
			destroy_string(&(config->UrgentStyle));

			for (i = 0; i < MAX_MOUSE_BUTTONS; ++i)
				if (config->Action[i])
            		destroy_string_list(config->Action[i],0);
			
#define FREE_COLLIDES(type) do { \
			destroy_string_list(&(config->type##Collides[0]),config->type##Collides_nitems); \
			if( config->type##Collides_wrexp ) { \
				for( i = 0 ; i < config->type##Collides_nitems ; ++i ) destroy_wild_reg_exp(config->type##Collides_wrexp[i]); \
				free( config->type##Collides_wrexp ); \
			}}while(0)
			
			if( config->NoCollides_nitems > 0 ) FREE_COLLIDES(No);
			if( config->AllowCollides_nitems > 0 ) FREE_COLLIDES(Allow);			
		}
//		memset( config, 0x00, sizeof(WinListConfig));
		config->flags = WINLIST_ShowIcon|WINLIST_ScaleIconToTextHeight ;
    	init_asgeometry (&(config->Geometry));
	    config->gravity = StaticGravity;
		config->MaxRows = 1;
		config->UseName = ASN_Name;
	    config->Align = ALIGN_CENTER;
	    config->HSpacing = DEFAULT_TBAR_HSPACING;
		config->VSpacing = DEFAULT_TBAR_VSPACING;
	    config->FBevel = config->UBevel = config->SBevel = DEFAULT_TBAR_HILITE ;
		config->IconAlign = NO_ALIGN ;
		config->IconLocation = 0 ;
		config->ShowHints = WINLIST_DEFAULT_ShowHints ;
		if( !free_resources ) 
		{
 	    	config->NoCollides = safecalloc(2, sizeof(char*)); ;
			config->NoCollides[0] = mystrdup(CLASS_PAGER);
			config->NoCollides[1] = mystrdup(CLASS_WHARF);		
			config->NoCollides_nitems = 2 ; 
		}
		config->NoCollidesSpacing = 1 ; 
	}
}

void
PrintWinListConfig (WinListConfig * config)
{
	int           i;

	fprintf (stderr, "WinListConfig = %p;\n", config);
	if (config == NULL)
		return;

	fprintf (stderr, "WinListConfig.flags = 0x%lX;\n", config->flags);
	fprintf (stderr, "WinListConfig.set_flags = 0x%lX;\n", config->set_flags);
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
	ASCF_PRINT_STRING_KEYWORD(stderr,WINLIST,config,UrgentStyle);
	
	ASCF_PRINT_INT_KEYWORD(stderr,WINLIST,config,IconLocation);	
	ASCF_PRINT_FLAGS_KEYWORD(stderr,WINLIST,config,IconAlign );
	ASCF_PRINT_SIZE_KEYWORD(stderr,WINLIST,config,IconSize);	
	ASCF_PRINT_FLAGS_KEYWORD(stderr,WINLIST,config,ShowHints );
	
	fprintf (stderr, "WinListConfig.gravity = %d;\n", config->gravity);

	for (i = 0; i < MAX_MOUSE_BUTTONS; ++i)
        ASCF_PRINT_IDX_COMPOUND_STRING_KEYWORD(stderr,WINLIST,config,Action,',',i);

	ASCF_PRINT_INT_KEYWORD(stderr,WINLIST,config,NoCollidesSpacing);	

}

void
WinList_fs2config( ASModuleConfig *asmodule_config, FreeStorageElem *Storage )
{
	FreeStorageElem *pCurr;
	ConfigItem    item;
	WinListConfig *config = AS_WINLIST_CONFIG(asmodule_config) ;
	
	if( config == NULL ) 
		return ; 
	
	item.memory = NULL;
    for (pCurr = Storage; pCurr; pCurr = pCurr->next)
	{
		if (pCurr->term == NULL)
			continue;

		if (pCurr->term->type == TT_FLAG)
        {
			if( pCurr->term->id ==  WINLIST_Bevel_ID )
			{	
				set_scalar_value(&(config->FBevel),ParseBevelOptions( pCurr->sub ), 
								 &(config->set_flags), WINLIST_Bevel);
                config->UBevel = config->SBevel = config->FBevel;
			}	
        }else
		{
			if (!ReadConfigItem (&item, pCurr))
				continue;

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
                            destroy_string_list( config->Action[action_no], 0 );
                            config->Action[action_no] = comma_string2list( ptr );
                        }
                        item.ok_to_free = 1;
                    }
                    break;

#define HANDLE_COLLIDES_ITEM(type) \
	if( item.data.string != NULL ) { \
		config->type##Collides = saferealloc( config->type##Collides, (config->type##Collides_nitems+1)*sizeof(char*)); \
		config->type##Collides[config->type##Collides_nitems] = item.data.string ; \
		++config->type##Collides_nitems ; }	break

				case WINLIST_NoCollides_ID: HANDLE_COLLIDES_ITEM(No);
				case WINLIST_AllowCollides_ID: HANDLE_COLLIDES_ITEM(Allow);

		        default:
        		    item.ok_to_free = 1;
			}
		}
	}
	
	ReadConfigItem (&item, NULL);
}

void
MergeWinListOptions ( ASModuleConfig *asm_to, ASModuleConfig *asm_from)
{
    int i ;
    START_TIME(option_time);

	WinListConfig *to = AS_WINLIST_CONFIG(asm_to);
	WinListConfig *from = AS_WINLIST_CONFIG(asm_from);
	if( to && from )
	{

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
		ASCF_MERGE_STRING_KEYWORD(WINLIST, to, from, UrgentStyle);

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
		ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, NoCollidesSpacing);
		ASCF_MERGE_SCALAR_KEYWORD(WINLIST, to, from, ShowHints);

    	for( i = 0 ; i < MAX_MOUSE_BUTTONS ; ++i )
        	if( from->Action[i] )
        	{
            	destroy_string_list( to->Action[i], 0 );
            	to->Action[i] = from->Action[i];
				from->Action[i] = NULL ;
        	}

#define MERGE_COLLIDES_ITEM(type) \
	 	do{ to->type##Collides = saferealloc( to->type##Collides, (from->type##Collides_nitems+to->type##Collides_nitems)*sizeof(char*)); \
		memcpy(&(to->type##Collides[to->type##Collides_nitems]), from->type##Collides, from->type##Collides_nitems*sizeof(char*) ) ; \
		to->type##Collides_nitems += from->type##Collides_nitems; \
		from->type##Collides_nitems = 0 ; free( from->type##Collides ); from->type##Collides = NULL ; }while(0)

		if( from->NoCollides_nitems > 0 )		MERGE_COLLIDES_ITEM(No);
		if( from->AllowCollides_nitems > 0 )		MERGE_COLLIDES_ITEM(Allow);
	}
    SHOW_TIME("to parsing",option_time);
}

ASFlagType DigestWinListAlign( WinListConfig *Config, ASFlagType align )
{
	LOCAL_DEBUG_OUT( "Align = 0x%8.8lx, IconAlign = 0x%8.8lx, Location = %d", align, Config->IconAlign, Config->IconLocation );

	if( !get_flags( Config->set_flags, WINLIST_IconLocation ) )
	{
		if( get_flags(align, PAD_H_MASK) == ALIGN_RIGHT ) 
			Config->IconLocation = 6 ; 
		else if( get_flags(align, PAD_H_MASK) == ALIGN_LEFT ) 
			Config->IconLocation = 4 ; 
	}		
	if( !get_flags( Config->set_flags, WINLIST_IconAlign ) )
	{
		if( get_flags(align, PAD_H_MASK) == PAD_H_MASK )
		{
			if( Config->IconLocation == 0 || Config->IconLocation == 4  )
			{
				Config->IconAlign = ALIGN_RIGHT ; 
				align = (align&(~PAD_H_MASK))|ALIGN_LEFT ; 
			}else if( Config->IconLocation == 6  )
			{
				Config->IconAlign = ALIGN_LEFT ; 
				align = (align&(~PAD_H_MASK))|ALIGN_RIGHT ; 
			}
		}else if( get_flags(align, PAD_H_MASK) == ALIGN_LEFT && (Config->IconLocation == 0 || Config->IconLocation == 4) )
			Config->IconAlign = ALIGN_VCENTER ;
		else if( get_flags(align, PAD_H_MASK) == ALIGN_RIGHT && Config->IconLocation == 6 )
			Config->IconAlign = ALIGN_VCENTER ;
	}
	LOCAL_DEBUG_OUT( "Align = 0x%8.8lx, IconAlign = 0x%8.8lx, Location = %d", align, Config->IconAlign, Config->IconLocation );
	return align ;

}

void
CheckWinListConfigSanity( WinListConfig *Config, ASGeometry *default_geometry, int default_gravity, 
						  int max_columns_override, int max_rows_override )
{
	int i ; 

    if( Config == NULL )
        Config = AS_WINLIST_CONFIG(create_ASModule_config(WinListConfigClass));

	if( max_columns_override > 0 ) 
		Config->MaxColumns = max_columns_override ;
		
	if( max_rows_override > 0 ) 
		Config->MaxRows = max_rows_override ;

    if( Config->MaxRows > MAX_WINLIST_WINDOW_COUNT || Config->MaxRows == 0  )
        Config->MaxRows = MAX_WINLIST_WINDOW_COUNT;

    if( Config->MaxColumns > MAX_WINLIST_WINDOW_COUNT || Config->MaxColumns == 0  )
        Config->MaxColumns = MAX_WINLIST_WINDOW_COUNT;

	if( max_columns_override > 0 && max_rows_override > 0 ) 
	{
		if( Config->MaxColumns > Config->MaxRows )
			set_flags( Config->flags, ASWL_RowsFirst );
		else
			clear_flags( Config->flags, ASWL_RowsFirst );
	}		

	if( default_geometry && default_geometry->flags != 0 ) 
		Config->Geometry = *default_geometry ;

    if( get_flags(Config->Geometry.flags, WidthValue) && Config->Geometry.width > 0 )
        Config->MinSize.width = Config->MaxSize.width = Config->Geometry.width ;
	
    Config->gravity = StaticGravity ;
    if( get_flags(Config->Geometry.flags, XNegative) )
        Config->gravity = get_flags(Config->Geometry.flags, YNegative)? SouthEastGravity:NorthEastGravity;
    else if( get_flags(Config->Geometry.flags, YNegative) )
        Config->gravity = SouthWestGravity;

LOCAL_DEBUG_OUT( "gravity = %d, default_gravity = %d", Config->gravity, default_gravity );
	if(default_gravity != ForgetGravity)
    	Config->gravity = default_gravity ;

    Config->anchor_x = get_flags( Config->Geometry.flags, XValue )?Config->Geometry.x:0;
    if( get_flags(Config->Geometry.flags, XNegative) )
        Config->anchor_x += Scr.MyDisplayWidth ;

    Config->anchor_y = get_flags( Config->Geometry.flags, YValue )?Config->Geometry.y:0;
    if( get_flags(Config->Geometry.flags, YNegative) )
        Config->anchor_y += Scr.MyDisplayHeight ;

	/* ASN_NameTypes - is a valid value now - meaning that there won't be any name - only an icon */
	Config->UseName %= ASN_NameTypes+1; 
	
	if( Config->UseName == ASN_NameTypes ) 
	{
		if ( get_flags(Config->flags, WINLIST_ShowIcon) )
		{
			/* clear_flags(Config->flags, WINLIST_ScaleIconToTextHeight);  
			   - can use xml vars to determine text instead or icons turn out too big */
		}else
			Config->UseName = 0; /* default - ASN_Name */
	}

	if( get_flags( Config->flags, WINLIST_ShowIcon ) )
		Config->Align = DigestWinListAlign( Config, Config->Align );

#define MAKE_COLLIDES_WREXP(type)	do{ \
		Config->type##Collides_wrexp = safecalloc( Config->type##Collides_nitems, sizeof(wild_reg_exp)); \
		for( i = 0 ; i < Config->type##Collides_nitems ; ++i ) \
			Config->type##Collides_wrexp[i] = compile_wild_reg_exp( Config->type##Collides[i] ); \
	}while(0)	

	if( Config->NoCollides_nitems > 0 ) MAKE_COLLIDES_WREXP(No);
	if( Config->AllowCollides_nitems > 0 ) MAKE_COLLIDES_WREXP(Allow);
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
