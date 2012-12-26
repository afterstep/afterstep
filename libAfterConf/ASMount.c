/*
 * Copyright (c) 2012 Sasha Vasko <sasha at aftercode.net>
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


/*#define ASMOUNT_FEEL_TERMS \
    ASCF_DEFINE_KEYWORD(ASMOUNT, TF_DONT_SPLIT	, Action		, TT_TEXT, NULL), \
    ASCF_DEFINE_KEYWORD(ASMOUNT, 0				, FillRowsFirst	, TT_FLAG, NULL), \
    ASCF_DEFINE_KEYWORD(ASMOUNT, 0				, UseSkipList	, TT_FLAG, NULL)
*/
#define ASMOUNT_LOOK_TERMS \
    ASCF_DEFINE_KEYWORD_S(ASMOUNT, 0			    , Align				, TT_FLAG	, &AlignSyntax,ASMountConfig), \
    ASCF_DEFINE_KEYWORD  (ASMOUNT, 0			    , Bevel				, TT_FLAG	, &BevelSyntax), \
    ASCF_DEFINE_KEYWORD_S(ASMOUNT, 0			    , MountedBevel			, TT_FLAG	, &BevelSyntax,ASMountConfig), \
    ASCF_DEFINE_KEYWORD_S(ASMOUNT, 0			    , UnmountedBevel			, TT_FLAG	, &BevelSyntax,ASMountConfig), \
    ASCF_DEFINE_KEYWORD_S(ASMOUNT, TF_QUOTES_OPTIONAL, MountedStyle	, TT_QUOTED_TEXT	,  NULL,ASMountConfig), \
    ASCF_DEFINE_KEYWORD_S(ASMOUNT, TF_QUOTES_OPTIONAL, UnmountedStyle		, TT_QUOTED_TEXT	,  NULL,ASMountConfig), \
    ASCF_DEFINE_KEYWORD  (ASMOUNT, 0			    , ShapeToContents	, TT_FLAG	,  NULL), \
    ASCF_DEFINE_KEYWORD  (ASMOUNT, 0					, ShowHints  	, TT_FLAG		, NULL)
	
#define ASMOUNT_PRIVATE_TERMS \
    ASCF_DEFINE_KEYWORD_S(ASMOUNT, 0			    , TileSize			, TT_GEOMETRY	, NULL,ASMountConfig), \
    ASCF_DEFINE_KEYWORD  (ASMOUNT, 0			    , Vertical			, TT_FLAG	,  NULL)


TermDef       ASMountFeelTerms[] = {
	/* Feel */
//	ASMOUNT_FEEL_TERMS,
	BALLOON_FEEL_TERMS,
	{0, NULL, 0, 0, 0}
};

TermDef       ASMountLookTerms[] = {
    /* Look */
	ASMOUNT_LOOK_TERMS,
/* now special cases that should be processed by it's own handlers */
    BALLOON_LOOK_TERMS,
	{0, NULL, 0, 0, 0}
};

TermDef       ASMountPrivateTerms[] = {
	ASMOUNT_PRIVATE_TERMS,
	BALLOON_FLAG_TERM,
	{0, NULL, 0, 0, 0}
};

#define ASMOUNT_ALL_TERMS	ASMOUNT_PRIVATE_TERMS, ASMOUNT_LOOK_TERMS


TermDef       ASMountDefaultsTerms[] = {

	ASMOUNT_ALL_TERMS,
/* now special cases that should be processed by it's own handlers */
    BALLOON_TERMS,
	INCLUDE_MODULE_DEFAULTS_END,
    {0, NULL, 0, 0, 0}
};

SyntaxDef ASMountDefaultsSyntax	= {'\n', '\0', ASMountDefaultsTerms, 		0, ' ',  "", "\t", "Module:ASMount_defaults","ASMount",     "AfterStep module for mounting/unmounting volumes",NULL,0};


TermDef       ASMountTerms[] = {
	INCLUDE_MODULE_DEFAULTS(&ASMountDefaultsSyntax),

	ASMOUNT_ALL_TERMS,
	
/* including MyStyles definitions processing */
	INCLUDE_MYSTYLE,
/* now special cases that should be processed by it's own handlers */
    BALLOON_TERMS,
    {0, NULL, 0, 0, 0}
};


SyntaxDef ASMountFeelSyntax 	= {'\n', '\0', ASMountFeelTerms, 	0, '\t', "", "\t", "ASMountFeel", 	"ASMountFeel", "AfterStep Volumes manager module feel", NULL, 0};
SyntaxDef ASMountLookSyntax 	= {'\n', '\0', ASMountLookTerms, 	0, '\t', "", "\t", "ASMountLook", 	"ASMountLook", "AfterStep Volumes manager module look", NULL, 0};
SyntaxDef ASMountPrivateSyntax 	= {'\n', '\0', ASMountPrivateTerms,	0, '\t', "", "\t", "ASMount",     	"ASMount",     "AfterStep Volumes manager module", NULL,0};
SyntaxDef ASMountSyntax 		= {'\n', '\0', ASMountTerms, 		0, ' ',  "", "\t", "Module:ASMount","ASMount",     "AfterStep module for management of storage volumes",NULL,0};


flag_options_xref ASMountFlags[] = {
	ASCF_DEFINE_MODULE_FLAG_XREF(ASMOUNT,Vertical,ASMountConfig),
	ASCF_DEFINE_MODULE_FLAG_XREF(ASMOUNT,ShapeToContents,ASMountConfig),
	ASCF_DEFINE_MODULE_FLAG_XREF(ASMOUNT,ShowHints,ASMountConfig),
    {0, 0, 0}
};

static void InitASMountConfig (ASModuleConfig *asm_config, Bool free_resources);
void ASMount_fs2config( ASModuleConfig *asmodule_config, FreeStorageElem *Storage );
void MergeASMountOptions ( ASModuleConfig *to, ASModuleConfig *from);


static ASModuleConfigClass _asmount_config_class = 
{	CONFIG_ASMount_ID,
 	0,
	sizeof(ASMountConfig),
 	"asmount",
 	InitASMountConfig,
	ASMount_fs2config,
	MergeASMountOptions,
	&ASMountSyntax,
	&ASMountLookSyntax,
	&ASMountFeelSyntax,
	ASMountFlags,
	offsetof(ASMountConfig,set_flags),
	
	ASDefaultBalloonTypes
 };
 
ASModuleConfigClass *getASMountConfigClass() { return &_asmount_config_class; }


static void
InitASMountConfig (ASModuleConfig *asm_config, Bool free_resources)
{
	ASMountConfig *config = AS_ASMOUNT_CONFIG(asm_config);
	if( config ) 
	{
		if( free_resources ) 
		{
			destroy_string(&(config->MountedStyle));
			destroy_string(&(config->UnmountedStyle));
		}
		config->flags = 0;
   	init_asgeometry (&(config->TileSize));
    config->MountedBevel = config->UnmountedBevel = DEFAULT_TBAR_HILITE ;
	}
}

void
PrintASMountConfig (ASMountConfig * config)
{
	fprintf (stderr, "ASMountConfig = %p;\n", config);
	if (config == NULL)
		return;

	fprintf (stderr, "ASMountConfig.flags = 0x%lX;\n", config->flags);
	fprintf (stderr, "ASMountConfig.set_flags = 0x%lX;\n", config->set_flags);
	ASCF_PRINT_FLAG_KEYWORD(stderr,ASMOUNT,config,ShapeToContents);
	ASCF_PRINT_FLAG_KEYWORD(stderr,ASMOUNT,config,Vertical);
	ASCF_PRINT_GEOMETRY_KEYWORD(stderr,ASMOUNT,config,TileSize);

	ASCF_PRINT_FLAGS_KEYWORD(stderr,ASMOUNT,config,Align );
	ASCF_PRINT_FLAGS_KEYWORD(stderr,ASMOUNT,config,MountedBevel);
	ASCF_PRINT_FLAGS_KEYWORD(stderr,ASMOUNT,config,UnmountedBevel);

	ASCF_PRINT_STRING_KEYWORD(stderr,ASMOUNT,config,UnmountedStyle);
	ASCF_PRINT_STRING_KEYWORD(stderr,ASMOUNT,config,MountedStyle);
}

void
ASMount_fs2config( ASModuleConfig *asmodule_config, FreeStorageElem *Storage )
{
	FreeStorageElem *pCurr;
	ConfigItem    item;
	ASMountConfig *config = AS_ASMOUNT_CONFIG(asmodule_config) ;
	
	if( config == NULL ) 
		return ; 
	
	item.memory = NULL;
  for (pCurr = Storage; pCurr; pCurr = pCurr->next)	{
		if (pCurr->term == NULL)
			continue;

		if (pCurr->term->type == TT_FLAG) {
			if( pCurr->term->id ==  ASMOUNT_Bevel_ID ) {	
				set_scalar_value(&(config->MountedBevel),ParseBevelOptions( pCurr->sub ), 
								 &(config->set_flags), ASMOUNT_Bevel);
                config->UnmountedBevel = config->MountedBevel;
			}	
    }else	{
			if (!ReadConfigItem (&item, pCurr))
				continue;

	    item.ok_to_free = 1;
		}
	}
	
	ReadConfigItem (&item, NULL);
}

void
MergeASMountOptions ( ASModuleConfig *asm_to, ASModuleConfig *asm_from)
{
  START_TIME(option_time);

	ASMountConfig *to = AS_ASMOUNT_CONFIG(asm_to);
	ASMountConfig *from = AS_ASMOUNT_CONFIG(asm_from);
	if( to && from )	{
    ASCF_MERGE_FLAGS(to,from);
		ASCF_MERGE_GEOMETRY_KEYWORD(ASMOUNT, to, from, TileSize);
		ASCF_MERGE_STRING_KEYWORD(ASMOUNT, to, from, MountedStyle);
		ASCF_MERGE_STRING_KEYWORD(ASMOUNT, to, from, UnmountedStyle);
	}
  SHOW_TIME("to parsing",option_time);
}

ASFlagType DigestASMountAlign( ASMountConfig *Config, ASFlagType align )
{
	return align ;
}

void
CheckASMountConfigSanity( ASMountConfig *Config, ASGeometry *default_tile_size)
{
  if( Config == NULL )
	  Config = AS_ASMOUNT_CONFIG(create_ASModule_config(getASMountConfigClass()));

	if( default_tile_size && default_tile_size->flags != 0 ) 
		Config->TileSize = *default_tile_size ;
}


