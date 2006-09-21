#ifndef AFTERCONF_H_FILE_INCLUDED
#define AFTERCONF_H_FILE_INCLUDED

#include "../libAfterImage/asvisual.h"
#include "../libAfterStep/clientprops.h"
#include "../libAfterStep/colorscheme.h"
#include "../libAfterStep/mylook.h"
#include "../libAfterStep/aswindata.h"

/***************************************************************************/
/*                        ASFunction parsing definitions                   */
/*                        this must go first as it is relying on IDs       */
/*                        defined in functions.h                           */
/****************************************************************************/

#ifndef NorthWestGravity
#include <X11/Xlib.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct ASImageManager;
struct ASFontManager;
struct SyntaxDef;
struct MyLook;
struct ASWindowBox ;
struct FreeStorageElem;
struct ConfigDef;
struct FunctionData;
struct ComplexFunction;
struct ConfigItem;
struct ASColorScheme;
struct ConfigDef;
struct FreeStorageElem;
struct ASCategoryTree;

extern struct SyntaxDef      BevelSyntax;
extern struct SyntaxDef      AlignSyntax;
extern struct SyntaxDef     *BevelSyntaxPtr;

/* All top level syntax definitions are listed below : */
extern struct SyntaxDef      ArrangeSyntax;  /* really is an empty syntax */  
extern struct SyntaxDef      AnimateSyntax;
extern struct SyntaxDef      AudioSyntax;
extern struct SyntaxDef      BaseSyntax;
extern struct SyntaxDef      CleanSyntax;
extern struct SyntaxDef      ColorSyntax;
extern struct SyntaxDef      DatabaseSyntax;
extern struct SyntaxDef      FeelSyntax;
extern struct SyntaxDef 	 FunctionSyntax;
extern struct SyntaxDef 	 PopupSyntax;
extern struct SyntaxDef 	 includeSyntax;

extern struct SyntaxDef      WindowBoxSyntax;
extern struct SyntaxDef		 ThemeSyntax;
extern struct SyntaxDef     SupportedHintsSyntax;

extern struct SyntaxDef      AutoExecSyntax;
extern struct SyntaxDef      LookSyntax;
extern struct SyntaxDef 	 AfterStepLookSyntax;
extern struct SyntaxDef 	 ModuleMyStyleSyntax;
extern struct SyntaxDef 	 AfterStepMyFrameSyntax;
extern struct SyntaxDef 	 AfterStepMyBackSyntax;
extern struct SyntaxDef 	 AfterStepTitleButtonSyntax;

extern struct SyntaxDef 	 AfterStepFeelSyntax;
extern struct SyntaxDef 	 AfterStepCursorSyntax;
extern struct SyntaxDef 	 AfterStepMouseSyntax;
extern struct SyntaxDef 	 AfterStepKeySyntax;
extern struct SyntaxDef 	 AfterStepWindowBoxSyntax;



extern struct SyntaxDef      PagerSyntax;
extern struct SyntaxDef      PagerPrivateSyntax;
extern struct SyntaxDef      PagerLookSyntax;
extern struct SyntaxDef      PagerFeelSyntax;
extern struct SyntaxDef		 WharfFeelSyntax; 
extern struct SyntaxDef		 WharfLookSyntax;
extern struct SyntaxDef		 WharfPrivateSyntax;
extern struct SyntaxDef 	 WharfFolderSyntax;
extern struct SyntaxDef 	 WharfSyntax;
extern struct SyntaxDef      WinCommandSyntax; 
extern struct SyntaxDef      WinListSyntax;
extern struct SyntaxDef      WinListPrivateSyntax;
extern struct SyntaxDef      WinListLookSyntax;
extern struct SyntaxDef      WinListFeelSyntax;
extern struct SyntaxDef      WinTabsSyntax;
extern struct SyntaxDef      PopupFuncSyntax;

#define ASCF_DEFINE_MODULE_FLAG_XREF(module,keyword,struct_type) \
	{module##_##keyword,module##_##keyword##_ID,0,offsetof(struct_type,flags),offsetof(struct_type,set_flags)}	
#define ASCF_DEFINE_MODULE_ONOFF_FLAG_XREF(module,keyword,struct_type) \
	{module##_##keyword,module##_##keyword##_ID,module##_No##keyword##_ID,offsetof(struct_type,flags),offsetof(struct_type,set_flags)}	


#define ASCF_DEFINE_KEYWORD(module,flags,keyword,type,subsyntax) \
	{(flags),#keyword,sizeof(#keyword)-1,type,module##_##keyword##_ID,subsyntax,0,0,0}	

#define ASCF_DEFINE_KEYWORD_S(module,flags,keyword,type,subsyntax,struct_type) \
	{(flags),#keyword,sizeof(#keyword)-1,type,module##_##keyword##_ID,subsyntax,offsetof(struct_type,keyword),module##_##keyword,0}	

#define ASCF_DEFINE_KEYWORD_SA(module,flags,keyword,type,subsyntax,struct_type,alias_for) \
	{(flags),#keyword,sizeof(#keyword)-1,type,module##_##keyword##_ID,subsyntax,offsetof(struct_type,alias_for),module##_##alias_for,0}	

#define ASCF_DEFINE_KEYWORD_SF(module,flags,keyword,type,subsyntax,struct_type) \
	{(flags),#keyword,sizeof(#keyword)-1,type,module##_##keyword##_ID,subsyntax,offsetof(struct_type,keyword),flags_on,flags_off}	

#define ASCF_PRINT_FLAG_KEYWORD(stream,module,__config,keyword) \
	do{ if(get_flags((__config)->set_flags,module##_##keyword) ) \
			fprintf (stream, #module "." #keyword " = %s;\n",get_flags((__config)->flags,module##_##keyword)? "On" : "Off");}while(0)

#define ASCF_PRINT_FLAGS_KEYWORD(stream,module,__config,keyword) \
	do{ if(get_flags((__config)->set_flags,module##_##keyword) ) fprintf (stream, #module "." #keyword " = 0x%lX;\n",(__config)->keyword); }while(0)

#define ASCF_PRINT_GEOMETRY_KEYWORD(stream,module,__config,keyword) \
	do{ if(get_flags((__config)->set_flags,module##_##keyword) )	\
		{ 	fprintf (stream, #module "." #keyword " = "); \
			if(get_flags((__config)->keyword.flags,WidthValue))	fprintf (stream, "%u",(__config)->keyword.width); \
			fputc( 'x', stream ); \
			if(get_flags((__config)->keyword.flags,HeightValue))	fprintf (stream, "%u",(__config)->keyword.height); \
			if(get_flags((__config)->keyword.flags,XValue))	fprintf (stream, "%+d",(__config)->keyword.x); \
			else if(get_flags((__config)->keyword.flags,YValue)) fputc ('+',stream);	\
			if(get_flags((__config)->keyword.flags,XValue))	fprintf (stream, "%+d",(__config)->keyword.y); \
			fputc( ';', stream ); fputc( '\n', stream ); \
		}}while(0)

#define ASCF_PRINT_SIZE_KEYWORD(stream,module,__config,keyword) \
	do{ if(get_flags((__config)->set_flags,module##_##keyword) )	\
			fprintf (stream, #module "." #keyword " = %ux%u;\n",(__config)->keyword.width,(__config)->keyword.height); }while(0)

#define ASCF_PRINT_INT_KEYWORD(stream,module,__config,keyword) \
	do{ if(get_flags((__config)->set_flags,module##_##keyword) ) fprintf (stream, #module "." #keyword " = %d;\n",(__config)->keyword); }while(0)

#define ASCF_PRINT_STRING_KEYWORD(stream,module,__config,keyword) \
	do{ if( (__config)->keyword != NULL  ) fprintf (stream, #module "." #keyword " = \"%s\";\n",(__config)->keyword); }while(0)

#define ASCF_PRINT_IDX_STRING_KEYWORD(stream,module,__config,keyword,idx) \
	do{ if( (__config)->keyword[idx] != NULL  ) fprintf (stream, #module "." #keyword "[%d] = \"%s\";\n",idx,(__config)->keyword[idx]); }while(0)

#define ASCF_PRINT_COMPOUND_STRING_KEYWORD(stream,module,__config,keyword,separator) \
	do{ if( (__config)->keyword != NULL  ) \
		{int __i__; \
			fprintf (stream, #module "." #keyword " = \""); \
			for( __i__=0; (__config)->keyword[__i__]!=NULL ; ++__i__ ) \
				fprintf (stream, "%s%c",(__config)->keyword[__i__],separator); \
			fputc( ';', stream ); fputc( '\n', stream ); \
		}}while(0)

#define ASCF_PRINT_IDX_COMPOUND_STRING_KEYWORD(stream,module,__config,keyword,separator,idx) \
	do{ if( (__config)->keyword[idx] != NULL  ) \
		{int __i__; \
			fprintf (stream, #module "." #keyword "[%d] = \"",idx); \
			for( __i__=0; (__config)->keyword[idx][__i__]!=NULL ; ++__i__ ) \
				fprintf (stream, "%s%c",(__config)->keyword[idx][__i__],separator); \
			fputc( '\"', stream ); fputc( ';', stream ); fputc( '\n', stream ); \
		}}while(0)

#define ASCF_HANDLE_SUBSYNTAX_KEYWORD_CASE(module,__config,__curr,keyword,type) \
		case module##_##keyword##_##ID : \
			if( __curr->sub ){ \
				set_flags(__config->set_flags,module##_##keyword); \
		   		__config->keyword = Parse##type##Options( __curr->sub ); }break

#define ASCF_HANDLE_ALIGN_KEYWORD_CASE(module,__config,__curr,keyword) \
		ASCF_HANDLE_SUBSYNTAX_KEYWORD_CASE(module,__config,__curr,keyword,Align)

#define ASCF_HANDLE_BEVEL_KEYWORD_CASE(module,__config,__curr,keyword) \
		ASCF_HANDLE_SUBSYNTAX_KEYWORD_CASE(module,__config,__curr,keyword,Bevel)
			

#define ASCF_HANDLE_GEOMETRY_KEYWORD_CASE(module,__config,__item,keyword) \
		case module##_##keyword##_##ID : set_flags(__config->set_flags,module##_##keyword); \
			__config->keyword = __item.data.geometry; break

#define ASCF_HANDLE_SIZE_KEYWORD_CASE(module,__config,__item,keyword) \
		case module##_##keyword##_##ID : set_flags(__config->set_flags,module##_##keyword); \
			__config->keyword.width = get_flags( __item.data.geometry.flags, WidthValue )?__item.data.geometry.width:0; \
			__config->keyword.height = get_flags( __item.data.geometry.flags, HeightValue )?__item.data.geometry.height:0; \
			break
			
#define ASCF_HANDLE_INTEGER_KEYWORD_CASE(module,__config,__item,keyword) \
		case module##_##keyword##_##ID : set_flags(__config->set_flags,module##_##keyword); \
			__config->keyword = __item.data.integer; break

#define ASCF_HANDLE_STRING_KEYWORD_CASE(module,__config,__item,keyword) \
		case module##_##keyword##_##ID : \
			if( __config->keyword ) free( __config->keyword ); \
			__config->keyword = __item.data.string; break


#define ASCF_MERGE_FLAGS(__to,__from) \
    do{	(__to)->flags = ((__from)->flags&(__from)->set_flags)|((__to)->flags & (~(__from)->set_flags)); \
    	(__to)->set_flags |= (__from)->set_flags;}while(0)

#define ASCF_MERGE_GEOMETRY_KEYWORD(module,__to,__from,keyword) \
	do{ if( get_flags((__from)->set_flags, module##_##keyword) )	\
        	merge_geometry(&((__from)->keyword), &((__to)->keyword) ); }while(0)

#define ASCF_MERGE_SCALAR_KEYWORD(module,__to,__from,keyword) \
	do{ if( get_flags((__from)->set_flags, module##_##keyword) )	\
        	(__to)->keyword = (__from)->keyword ; }while(0)

#define ASCF_MERGE_STRING_KEYWORD(module,__to,__from,keyword) \
	do{ if( (__from)->keyword )	\
        {	set_string(	&((__to)->keyword), stripcpy2((__from)->keyword, False));}}while(0)

/* misc function stuff : */
#define FUNC_ID_START           F_NOP   /* 0 */
#define FUNC_ID_END           	F_FUNCTIONS_NUM

extern struct SyntaxDef *pFuncSyntax;
extern struct SyntaxDef DummyFuncSyntax;

/* used in some "special" function to correctly process trailing function definition */
unsigned long TrailingFuncSpecial (struct ConfigDef * config, int skip_tokens);

struct FreeStorageElem **Func2FreeStorage (struct SyntaxDef * syntax,
				    struct FreeStorageElem ** tail,
				    struct FunctionData * func);

struct TermDef          *txt2fterm (const char *txt, int quiet);
struct TermDef          *func2fterm (FunctionCode func, int quiet);
struct FunctionData     *String2Func ( const char *string, struct FunctionData *p_fdata, Bool quiet );


#define CONFIG_ID_START					(FUNC_ID_END+1)
#define CONFIG_root_ID					CONFIG_ID_START
#define CONFIG_Base_ID					(CONFIG_ID_START+1)
#define CONFIG_ColorScheme_ID			(CONFIG_ID_START+2)
#define CONFIG_Functions_ID				(CONFIG_ID_START+3)
#define CONFIG_Popups_ID				(CONFIG_ID_START+4)
#define CONFIG_Database_ID				(CONFIG_ID_START+5)
#define CONFIG_Module_ID				(CONFIG_ID_START+6)

#define CONFIG_MODULE_TYPES				(CONFIG_ID_START+7)
#define CONFIG_AfterStep_ID				(CONFIG_MODULE_TYPES)
#define CONFIG_Pager_ID					(CONFIG_MODULE_TYPES+1)
#define CONFIG_Wharf_ID					(CONFIG_MODULE_TYPES+2)
#define CONFIG_WinList_ID				(CONFIG_MODULE_TYPES+3)


#define CONFIG_MODULE_SPECIFIC			(CONFIG_MODULE_TYPES+4)
#define CONFIG_AfterStepLook_ID			(CONFIG_MODULE_SPECIFIC)
#define CONFIG_AfterStepFeel_ID			(CONFIG_MODULE_SPECIFIC+1)
#define CONFIG_PagerLook_ID 			(CONFIG_MODULE_SPECIFIC+2)
#define CONFIG_PagerFeel_ID				(CONFIG_MODULE_SPECIFIC+3)
#define CONFIG_WharfLook_ID 			(CONFIG_MODULE_SPECIFIC+4)
#define CONFIG_WharfFeel_ID				(CONFIG_MODULE_SPECIFIC+5)
#define CONFIG_WharfFolders_ID 			(CONFIG_MODULE_SPECIFIC+6)
#define CONFIG_WinListLook_ID 			(CONFIG_MODULE_SPECIFIC+7)
#define CONFIG_WinListFeel_ID 			(CONFIG_MODULE_SPECIFIC+8)


#define CONFIG_FILE_IDS			   		(CONFIG_MODULE_SPECIFIC+9)
#define CONFIG_LookFile_ID				(CONFIG_FILE_IDS)
#define CONFIG_FeelFile_ID 				(CONFIG_FILE_IDS+1)
#define CONFIG_StartDir_ID 				(CONFIG_FILE_IDS+2)
#define CONFIG_AutoExecFile_ID 			(CONFIG_FILE_IDS+3)
#define CONFIG_PagerFile_ID 			(CONFIG_FILE_IDS+4)
#define CONFIG_WharfFile_ID 			(CONFIG_FILE_IDS+5)
#define CONFIG_WinListFile_ID 			(CONFIG_FILE_IDS+6)
#define CONFIG_AfterStepFile_ID	 		(CONFIG_FILE_IDS+7)
#define CONFIG_BaseFile_ID				(CONFIG_FILE_IDS+8)
#define CONFIG_ColorSchemeFile_ID		(CONFIG_FILE_IDS+9)
#define CONFIG_DatabaseFile_ID			(CONFIG_FILE_IDS+10)
#define CONFIG_IncludeFile_ID			(CONFIG_FILE_IDS+11)


#define CONFIG_FILES_IDS			    (CONFIG_FILE_IDS+12)
#define CONFIG_FunctionsFiles_ID		(CONFIG_FILES_IDS)
#define CONFIG_PopupsFiles_ID			(CONFIG_FILES_IDS+1)
#define CONFIG_LookFiles_ID				(CONFIG_FILES_IDS+2)
#define CONFIG_FeelFiles_ID				(CONFIG_FILES_IDS+3)
#define CONFIG_BackgroundFiles_ID		(CONFIG_FILES_IDS+4)
#define CONFIG_PrivateFiles_ID			(CONFIG_FILES_IDS+5)
#define CONFIG_SharedFiles_ID			(CONFIG_FILES_IDS+6)

#define CONFIG_OPTIONS_IDS			    (CONFIG_FILES_IDS+7)
#define CONFIG_BaseOptions_ID			(CONFIG_OPTIONS_IDS)
#define CONFIG_ColorSchemeOptions_ID 	(CONFIG_OPTIONS_IDS+1)
#define CONFIG_MyStyles_ID				(CONFIG_OPTIONS_IDS+2)
#define CONFIG_MyFrames_ID				(CONFIG_OPTIONS_IDS+3)
#define CONFIG_MyBackgrounds_ID		 	(CONFIG_OPTIONS_IDS+4)
#define CONFIG_TitleButtons_ID		 	(CONFIG_OPTIONS_IDS+5)
#define CONFIG_Cursors_ID	  			(CONFIG_OPTIONS_IDS+6)
#define CONFIG_MouseBindings_ID			(CONFIG_OPTIONS_IDS+7)
#define CONFIG_KeyBindings_ID			(CONFIG_OPTIONS_IDS+8)
#define CONFIG_WindowBoxes_ID			(CONFIG_OPTIONS_IDS+9)
#define CONFIG_LookOptions_ID		 	(CONFIG_OPTIONS_IDS+10)
#define CONFIG_FeelOptions_ID			(CONFIG_OPTIONS_IDS+11)
#define CONFIG_AfterStepOptions_ID		(CONFIG_OPTIONS_IDS+12)
#define CONFIG_PagerOptions_ID			(CONFIG_OPTIONS_IDS+13)
#define CONFIG_WharfOptions_ID			(CONFIG_OPTIONS_IDS+14)
#define CONFIG_WinListOptions_ID		(CONFIG_OPTIONS_IDS+15)

#define CONFIG_SUBOPTIONS_IDS	   	    (CONFIG_OPTIONS_IDS+16)
#define CONFIG_flags_ID					(CONFIG_SUBOPTIONS_IDS)
#define CONFIG_x_ID						(CONFIG_SUBOPTIONS_IDS+1)
#define CONFIG_y_ID						(CONFIG_SUBOPTIONS_IDS+2)
#define CONFIG_width_ID					(CONFIG_SUBOPTIONS_IDS+3)
#define CONFIG_height_ID 				(CONFIG_SUBOPTIONS_IDS+4)

#define CONFIG_type_ID	 				(CONFIG_SUBOPTIONS_IDS+5)
#define CONFIG_tint_ID	 				(CONFIG_SUBOPTIONS_IDS+6)
#define CONFIG_pixmap_ID 				(CONFIG_SUBOPTIONS_IDS+7)

#define CONFIG_hotkey_ID				(CONFIG_SUBOPTIONS_IDS+8)
#define CONFIG_text_ID					(CONFIG_SUBOPTIONS_IDS+9)
#define CONFIG_value_ID					(CONFIG_SUBOPTIONS_IDS+10)
#define CONFIG_unit_ID					(CONFIG_SUBOPTIONS_IDS+11)

#define CONFIG_unpressed_ID				(CONFIG_SUBOPTIONS_IDS+12)
#define CONFIG_pressed_ID 				(CONFIG_SUBOPTIONS_IDS+13)

#define CONFIG_source_ID 				(CONFIG_SUBOPTIONS_IDS+14)
#define CONFIG_context_ID 				(CONFIG_SUBOPTIONS_IDS+15)
#define CONFIG_mod_ID		 			(CONFIG_SUBOPTIONS_IDS+16)

#define CONFIG_left_ID	  				(CONFIG_SUBOPTIONS_IDS+17)
#define CONFIG_right_ID	  				(CONFIG_SUBOPTIONS_IDS+18)
#define CONFIG_top_ID	  				(CONFIG_SUBOPTIONS_IDS+19)
#define CONFIG_bottom_ID  				(CONFIG_SUBOPTIONS_IDS+20)

#define CONFIG_image_ID  				(CONFIG_SUBOPTIONS_IDS+21)
#define CONFIG_mask_ID  				(CONFIG_SUBOPTIONS_IDS+22)

#define CONFIG_ID_END					(CONFIG_SUBOPTIONS_IDS+23)

#define MODULE_Defaults_ID				(CONFIG_ID_END+1)
#define MODULE_DefaultsEnd_ID				(CONFIG_ID_END+2)

#define INCLUDE_MODULE_DEFAULTS(syntax)	{0, "Defaults", 8, TT_FLAG, MODULE_Defaults_ID, syntax}
#define INCLUDE_MODULE_DEFAULTS_END	   	{TF_SYNTAX_TERMINATOR, "~Defaults", 9, TT_FLAG, MODULE_DefaultsEnd_ID, NULL}



/* must call this one to fix all the pointers referencing libAfterStep */
void LinkAfterStepConfig();
struct FreeStorageElem;
struct MyStyleDefinition;
struct balloonConfig;

struct ASModuleConfigClass;

typedef struct ASModuleConfig
{
	int type ; /* any of the CONFIG_ values above */
	struct ASModuleConfigClass *class ;
	
	struct MyStyleDefinition *style_defs ;
    struct balloonConfig	 *balloon_conf;

    struct FreeStorageElem   *more_stuff;
	
}ASModuleConfig;

#define AS_MODULE_CONFIG(p) ((ASModuleConfig*)(p))

#define AS_MODULE_CONFIG_TYPED(__p,__type_id,__type) \
	({const ASModuleConfig *__a = (const ASModuleConfig*)(__p); __a ? ( __a->type == __type_id ? (__type*)__a:NULL):NULL; })



void init_asmodule_config( ASModuleConfig *config, Bool free_resources ); 
struct flag_options_xref;

typedef struct ASModuleConfigClass
{
	
	int        type ; /* any of the CONFIG_ values */
	ASFlagType flags ;
	int 	   config_struct_size ; /* sizeof(ConfigType) */
	
	char *private_config_file ;
	
	void  (*init_config_func)( ASModuleConfig *config, Bool free_resources);
	void  (*free_storage2config_func)(ASModuleConfig *config, struct FreeStorageElem *storage);
	void  (*merge_config_func)( ASModuleConfig *to, ASModuleConfig *from);
	
	struct SyntaxDef *module_syntax ; 
	struct SyntaxDef *look_syntax ; 
	struct SyntaxDef *feel_syntax ; 

	struct flag_options_xref *flags_xref ;
	ptrdiff_t 		   set_flags_field_offset ;

}ASModuleConfigClass;


void destroy_ASModule_config( ASModuleConfig *config );
ASModuleConfig* create_ASModule_config( ASModuleConfigClass *class );
ASModuleConfig *parse_asmodule_config_all(ASModuleConfigClass *class );



/***************************************************************************/
/*                        Base file pasring definitions                    */
/***************************************************************************/
#define BASE_ID_START        	(MODULE_DefaultsEnd_ID+1)
#define BASE_MODULE_PATH_ID     BASE_ID_START
#define BASE_AUDIO_PATH_ID      BASE_ID_START+1
#define BASE_ICON_PATH_ID     	BASE_ID_START+2
#define BASE_PIXMAP_PATH_ID     BASE_ID_START+3
#define BASE_FONT_PATH_ID       BASE_ID_START+4
#define BASE_CURSOR_PATH_ID     BASE_ID_START+5
#define BASE_MYNAME_PATH_ID     BASE_ID_START+6
#define BASE_GTKRC_PATH_ID      BASE_ID_START+7
#define BASE_GTKRC20_PATH_ID    BASE_ID_START+8
#define BASE_DESKTOP_SIZE_ID	BASE_ID_START+9
#define BASE_DESKTOP_SCALE_ID	BASE_ID_START+10
#define BASE_TermCommand_ID		BASE_ID_START+11
#define BASE_BrowserCommand_ID	BASE_ID_START+12
#define BASE_EditorCommand_ID	BASE_ID_START+13
#define BASE_NoSharedMemory_ID	BASE_ID_START+14
#define BASE_NoKDEGlobalsTheming_ID	BASE_ID_START+15
#define BASE_ID_END             BASE_ID_START+16

typedef struct
{
#define BASE_NO_SHARED_MEMORY	(0x01<<0)	
#define BASE_NO_KDEGLOBALS_THEMING	(0x01<<1)	  
#define BASE_DESKTOP_SIZE_SET	(0x01<<16)	  
#define BASE_DESKTOP_SCALE_SET	(0x01<<17)	  
	ASFlagType flags, set_flags ; 
    char *module_path;
    char *audio_path;
    char *icon_path;
    char *pixmap_path;
    char *font_path;
    char *cursor_path;
    char *myname_path;
    char *gtkrc_path;
    char *gtkrc20_path;
    ASGeometry desktop_size;
    int desktop_scale;

#define MAX_TOOL_COMMANDS	8
	char *term_command[MAX_TOOL_COMMANDS] ; 
	char *browser_command[MAX_TOOL_COMMANDS] ; 
	char *editor_command[MAX_TOOL_COMMANDS] ; 

    struct FreeStorageElem *more_stuff;
}BaseConfig;

BaseConfig *ParseBaseOptions (const char *filename, char *myname);
void ExtractPath (BaseConfig * config,
			 char **module_path,
			 char **audio_path,
			 char **icon_path,
			 char **pixmap_path,
			 char **font_path,
			 char **cursor_path,
			 char **myname_path,
			 char **gtkrc_path,
			 char **gtkrc20_path);

void BaseConfig2ASEnvironment( register BaseConfig *config, ASEnvironment **penv );
void ReloadASImageManager( ASImageManager **old_imageman );
Bool ReloadASEnvironment( struct ASImageManager **old_imageman, struct ASFontManager **old_fontman, BaseConfig **config_return, 
						  Bool flush_images, Bool support_shared_images );

/*
 * all data members that has been used from BaseConfig structure, returned
 * by this call must be set to NULL, or memory allocated for them will be
 * deallocated by the following DestroyBaseConfig function !
 */

void DestroyBaseConfig (BaseConfig * config);

/***************************************************************************/
/*                           MyStyles                                      */
/***************************************************************************/
#define MYSTYLE_ID_START				BASE_ID_END+1
#define MYSTYLE_START_ID				MYSTYLE_ID_START
#define MYSTYLE_INHERIT_ID 				MYSTYLE_ID_START+1
#define MYSTYLE_FONT_ID					MYSTYLE_ID_START+2
#define MYSTYLE_FORECOLOR_ID			MYSTYLE_ID_START+3
#define MYSTYLE_BACKCOLOR_ID			MYSTYLE_ID_START+4
#define MYSTYLE_TEXTSTYLE_ID			MYSTYLE_ID_START+5
#define MYSTYLE_BACKGRADIENT_ID			MYSTYLE_ID_START+6
#define MYSTYLE_BACKMULTIGRADIENT_ID	MYSTYLE_ID_START+7
#define MYSTYLE_BACKPIXMAP_ID  			MYSTYLE_ID_START+8
#define MYSTYLE_DRAWTEXTBACKGROUND_ID 	MYSTYLE_ID_START+9
#define MYSTYLE_SliceXStart_ID			MYSTYLE_ID_START+10
#define MYSTYLE_SliceXEnd_ID			MYSTYLE_ID_START+11
#define MYSTYLE_SliceYStart_ID			MYSTYLE_ID_START+12
#define MYSTYLE_SliceYEnd_ID			MYSTYLE_ID_START+13
#define MYSTYLE_Overlay_ID				MYSTYLE_ID_START+14

#define MYSTYLE_DONE_ID					MYSTYLE_ID_START+15

#define MYSTYLE_ID_END					MYSTYLE_ID_START+20

#define MYSTYLE_TERMS \
{TF_NO_MYNAME_PREPENDING,"MyStyle", 	7, TT_QUOTED_TEXT, MYSTYLE_START_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING|TF_NONUNIQUE|TF_QUOTES_OPTIONAL,"Inherit", 	7, TT_QUOTED_TEXT, MYSTYLE_INHERIT_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"Font",    	4, TT_FONT, MYSTYLE_FONT_ID		, NULL},\
{TF_NO_MYNAME_PREPENDING,"ForeColor",	9, TT_COLOR, MYSTYLE_FORECOLOR_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"BackColor",	9, TT_COLOR, MYSTYLE_BACKCOLOR_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"TextStyle",	9, TT_INTEGER, MYSTYLE_TEXTSTYLE_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"BackGradient",12, TT_INTEGER, MYSTYLE_BACKGRADIENT_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"BackMultiGradient", 17, TT_INTEGER, MYSTYLE_BACKMULTIGRADIENT_ID},\
{TF_NO_MYNAME_PREPENDING,"BackPixmap",	10,TT_INTEGER, MYSTYLE_BACKPIXMAP_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"DrawTextBackground",18,TT_FLAG, MYSTYLE_DRAWTEXTBACKGROUND_ID, NULL},\
{TF_NO_MYNAME_PREPENDING,"SliceXStart",	11, TT_INTEGER, MYSTYLE_SliceXStart_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"SliceXEnd",	9, TT_INTEGER, MYSTYLE_SliceXEnd_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"SliceYStart",	11, TT_INTEGER, MYSTYLE_SliceYStart_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"SliceYEnd",	9, TT_INTEGER, MYSTYLE_SliceYEnd_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING,"Overlay",	    7,TT_INTEGER, MYSTYLE_Overlay_ID	, NULL},\
{TF_NO_MYNAME_PREPENDING|TF_SYNTAX_TERMINATOR,"~MyStyle", 	8, TT_FLAG, MYSTYLE_DONE_ID		, NULL}

extern struct SyntaxDef MyStyleSyntax;
/* use this in module term definition to add MyStyle parsing functionality */
#define INCLUDE_MYSTYLE {TF_NO_MYNAME_PREPENDING,"MyStyle", 7, TT_QUOTED_TEXT, MYSTYLE_START_ID, &MyStyleSyntax}

typedef struct MyStyleDefinition
{
#define MYSTYLE_DRAW_TEXT_BACKGROUND	(0x01<<0)
#define MYSTYLE_FINISHED				(0x01<<1)
#define MYSTYLE_TEXT_STYLE_SET			(0x01<<2)
#define MYSTYLE_SLICE_SET				(0x01<<3)
   	ASFlagType flags;

	char   *name;
    int     inherit_num;
    char  **inherit;

	char   *font;
    char   *fore_color, *back_color;
    int 	text_style;

	int 	texture_type ;

	int     back_grad_type;
    int     back_grad_npoints;
    char  **back_grad_colors;
    double *back_grad_offsets;

    char   *back_pixmap;

	int slice_x_start, slice_x_end, slice_y_start, slice_y_end ;

	int 	overlay_type ;
    char   *overlay;

    struct  FreeStorageElem *more_stuff;
    struct  MyStyleDefinition *next;	/* as long as there could be several MyStyle definitions
					 * per config file, we arrange them all into the linked list
					 */
}MyStyleDefinition;
/* this function will process consequent MyStyle options from FreeStorage,
 * create (if needed ) and initialize MyStyleDefinition structure
 * new structures will be added at the tail of linked list.
 * it will return the new tail.
 * [options] will be changed to point to the next non-MyStyle FreeStorageElem
 */
void DestroyMyStyleDefinitions (MyStyleDefinition ** list);
MyStyleDefinition **ProcessMyStyleOptions (struct FreeStorageElem * options, MyStyleDefinition ** tail);
void mystyle_parse (char *tline, FILE * fd, char **myname, int *mystyle_list);
void          mystyle_create_from_definition (MyStyleDefinition * def);

void PrintMyStyleDefinitions (MyStyleDefinition * list);
/*
 * this function process a linked list of MyStyle definitions
 * and create MyStyle for each valid definition
 * this operation is destructive in the way that all
 * data members of definitions that are used in MyStyle will be
 * set to NULL, so to prevent them being deallocated by destroy function,
 * and/or being used in other places
 * ATTENTION !!!!!
 * MyStyleDefinitions become unusable as the result, and get's destroyed
 * pointer to a list becomes NULL !
 */
void ProcessMyStyleDefinitions (MyStyleDefinition ** list);

void MergeMyStyleText (MyStyleDefinition ** list, const char *name,
                  const char *new_font, const char *new_fcolor, const char *new_bcolor, int new_style);
void MergeMyStyleTextureOld (MyStyleDefinition ** list, const char *name,
                        int type, char *color_from, char *color_to, char *pixmap);
struct FreeStorageElem **MyStyleDefs2FreeStorage (struct SyntaxDef * syntax, struct FreeStorageElem ** tail, MyStyleDefinition * defs);


/**************************************************************************/
/*                        MyFrame parsing definitions                     */
/**************************************************************************/
#define MYFRAME_ID_START	(MYSTYLE_ID_END+1)

#define MYFRAME_START_ID 	(MYFRAME_ID_START)
#define MYFRAME_North_ID        (MYFRAME_ID_START+1)
#define MYFRAME_East_ID         (MYFRAME_ID_START+2)
#define MYFRAME_South_ID        (MYFRAME_ID_START+3)
#define MYFRAME_West_ID         (MYFRAME_ID_START+4)
#define MYFRAME_NorthWest_ID    (MYFRAME_ID_START+5)
#define MYFRAME_NorthEast_ID    (MYFRAME_ID_START+6)
#define MYFRAME_SouthWest_ID    (MYFRAME_ID_START+7)
#define MYFRAME_SouthEast_ID    (MYFRAME_ID_START+8)
#define MYFRAME_Side_ID                 (MYFRAME_ID_START+9)
#define MYFRAME_NoSide_ID               (MYFRAME_ID_START+10)
#define MYFRAME_Corner_ID               (MYFRAME_ID_START+11)
#define MYFRAME_NoCorner_ID             (MYFRAME_ID_START+12)
#define MYFRAME_TitleUnfocusedStyle_ID  (MYFRAME_ID_START+13)
#define MYFRAME_TitleFocusedStyle_ID    (MYFRAME_ID_START+14)
#define MYFRAME_TitleStickyStyle_ID     (MYFRAME_ID_START+15)
#define MYFRAME_FrameUnfocusedStyle_ID  (MYFRAME_ID_START+16)
#define MYFRAME_FrameFocusedStyle_ID    (MYFRAME_ID_START+17)
#define MYFRAME_FrameStickyStyle_ID     (MYFRAME_ID_START+18)

#define MYFRAME_SideSize_ID             (MYFRAME_ID_START+19)
#define MYFRAME_CornerSize_ID           (MYFRAME_ID_START+20)
#define MYFRAME_SideAlign_ID            (MYFRAME_ID_START+21)
#define MYFRAME_CornerAlign_ID          (MYFRAME_ID_START+22)
#define MYFRAME_SideBevel_ID            (MYFRAME_ID_START+23)
#define MYFRAME_CornerBevel_ID          (MYFRAME_ID_START+24)
#define MYFRAME_SideFBevel_ID           (MYFRAME_ID_START+25)
#define MYFRAME_CornerFBevel_ID         (MYFRAME_ID_START+26)
#define MYFRAME_SideUBevel_ID           (MYFRAME_ID_START+27)
#define MYFRAME_CornerUBevel_ID         (MYFRAME_ID_START+28)
#define MYFRAME_SideSBevel_ID           (MYFRAME_ID_START+29)
#define MYFRAME_CornerSBevel_ID         (MYFRAME_ID_START+30)
#define MYFRAME_TitleBevel_ID           (MYFRAME_ID_START+31)
#define MYFRAME_TitleFBevel_ID          (MYFRAME_ID_START+32)
#define MYFRAME_TitleUBevel_ID          (MYFRAME_ID_START+33)
#define MYFRAME_TitleSBevel_ID          (MYFRAME_ID_START+34)
#define MYFRAME_TitleAlign_ID           (MYFRAME_ID_START+35)
#define MYFRAME_TitleCM_ID              (MYFRAME_ID_START+36)
#define MYFRAME_TitleFCM_ID             (MYFRAME_ID_START+37)
#define MYFRAME_TitleUCM_ID             (MYFRAME_ID_START+38)
#define MYFRAME_TitleSCM_ID             (MYFRAME_ID_START+39)
#define MYFRAME_TitleFHue_ID			(MYFRAME_ID_START+40)
#define MYFRAME_TitleUHue_ID			(MYFRAME_ID_START+41)
#define MYFRAME_TitleSHue_ID			(MYFRAME_ID_START+42)
#define MYFRAME_TitleFSat_ID			(MYFRAME_ID_START+43)
#define MYFRAME_TitleUSat_ID			(MYFRAME_ID_START+44)
#define MYFRAME_TitleSSat_ID			(MYFRAME_ID_START+45)
#define MYFRAME_Inherit_ID              (MYFRAME_ID_START+46)
#define MYFRAME_InheritDefaults_ID      (MYFRAME_ID_START+47)
#define MYFRAME_TitleHSpacing_ID 		(MYFRAME_ID_START+48)
#define MYFRAME_TitleVSpacing_ID 		(MYFRAME_ID_START+49)
#define MYFRAME_DONE_ID                 (MYFRAME_ID_START+50)

#define MYFRAME_TitleBackground_ID_START	(MYFRAME_DONE_ID+1)
#define MYFRAME_LeftBtnBackground_ID		(MYFRAME_TitleBackground_ID_START+MYFRAME_TITLE_BACK_LBTN	)
#define MYFRAME_LeftSpacerBackground_ID 	(MYFRAME_TitleBackground_ID_START+MYFRAME_TITLE_BACK_LSPACER)
#define MYFRAME_LTitleSpacerBackground_ID 	(MYFRAME_TitleBackground_ID_START+MYFRAME_TITLE_BACK_LTITLE_SPACER)
#define MYFRAME_TitleBackground_ID      	(MYFRAME_TitleBackground_ID_START+MYFRAME_TITLE_BACK_LBL	)
#define MYFRAME_RTitleSpacerBackground_ID 	(MYFRAME_TitleBackground_ID_START+MYFRAME_TITLE_BACK_RTITLE_SPACER)
#define MYFRAME_RightSpacerBackground_ID 	(MYFRAME_TitleBackground_ID_START+MYFRAME_TITLE_BACK_RSPACER)
#define MYFRAME_RightBtnBackground_ID   	(MYFRAME_TitleBackground_ID_START+MYFRAME_TITLE_BACK_RBTN	)
#define MYFRAME_TitleBackground_ID_END      (MYFRAME_TitleBackground_ID_START+MYFRAME_TITLE_BACKS)

#define MYFRAME_TitleBackgroundAlign_ID_START	(MYFRAME_TitleBackground_ID_END)
#define MYFRAME_LeftBtnBackAlign_ID				(MYFRAME_TitleBackgroundAlign_ID_START+MYFRAME_TITLE_BACK_LBTN	)
#define MYFRAME_LeftSpacerBackAlign_ID  		(MYFRAME_TitleBackgroundAlign_ID_START+MYFRAME_TITLE_BACK_LSPACER)
#define MYFRAME_LTitleSpacerBackAlign_ID  		(MYFRAME_TitleBackgroundAlign_ID_START+MYFRAME_TITLE_BACK_LTITLE_SPACER)
#define MYFRAME_TitleBackgroundAlign_ID 		(MYFRAME_TitleBackgroundAlign_ID_START+MYFRAME_TITLE_BACK_LBL	 )
#define MYFRAME_RTitleSpacerBackAlign_ID 		(MYFRAME_TitleBackgroundAlign_ID_START+MYFRAME_TITLE_BACK_RTITLE_SPACER)
#define MYFRAME_RightSpacerBackAlign_ID 		(MYFRAME_TitleBackgroundAlign_ID_START+MYFRAME_TITLE_BACK_RSPACER)
#define MYFRAME_RightBtnBackAlign_ID			(MYFRAME_TitleBackgroundAlign_ID_START+MYFRAME_TITLE_BACK_RBTN	 )
#define MYFRAME_TitleBackgroundAlign_ID_END     (MYFRAME_TitleBackgroundAlign_ID_START+MYFRAME_TITLE_BACKS)

#define MYFRAME_CondenseTitlebar_ID             (MYFRAME_TitleBackgroundAlign_ID_END+1) 
#define MYFRAME_LeftTitlebarLayout_ID			(MYFRAME_TitleBackgroundAlign_ID_END+2)
#define MYFRAME_RightTitlebarLayout_ID			(MYFRAME_TitleBackgroundAlign_ID_END+3)

#define MYFRAME_LeftBtnAlign_ID					(MYFRAME_TitleBackgroundAlign_ID_END+4)
#define MYFRAME_RightBtnAlign_ID				(MYFRAME_TitleBackgroundAlign_ID_END+5)

#define MYFRAME_NoBorder_ID             		(MYFRAME_TitleBackgroundAlign_ID_END+6) 
#define MYFRAME_AllowBorder_ID             		(MYFRAME_NoBorder_ID+1) 

#define MYFRAME_ID_END      (MYFRAME_ID_START+128)

#define ALIGN_ID_START      (MYFRAME_ID_END+1)
#define ALIGN_Left_ID       (ALIGN_ID_START+1)
#define ALIGN_Top_ID        (ALIGN_ID_START+2)
#define ALIGN_Right_ID      (ALIGN_ID_START+3)
#define ALIGN_Bottom_ID     (ALIGN_ID_START+4)
#define ALIGN_HTiled_ID     (ALIGN_ID_START+5)
#define ALIGN_VTiled_ID     (ALIGN_ID_START+6)
#define ALIGN_HScaled_ID    (ALIGN_ID_START+7)
#define ALIGN_VScaled_ID    (ALIGN_ID_START+8)
#define ALIGN_LabelWidth_ID  (ALIGN_ID_START+9)
#define ALIGN_LabelHeight_ID (ALIGN_ID_START+10)
#define ALIGN_LabelSize_ID  (ALIGN_ID_START+11)
#define ALIGN_Center_ID     (ALIGN_ID_START+12)
#define ALIGN_HCenter_ID    (ALIGN_ID_START+13)
#define ALIGN_VCenter_ID    (ALIGN_ID_START+14)
#define ALIGN_ID_END        (ALIGN_ID_START+16)

#define BEVEL_ID_START      (ALIGN_ID_END+1)
#define BEVEL_None_ID       (BEVEL_ID_START+1)
#define BEVEL_Left_ID       (BEVEL_ID_START+2)
#define BEVEL_Top_ID        (BEVEL_ID_START+3)
#define BEVEL_Right_ID      (BEVEL_ID_START+4)
#define BEVEL_Bottom_ID     (BEVEL_ID_START+5)
#define BEVEL_Extra_ID      (BEVEL_ID_START+6)
#define BEVEL_NoOutline_ID  (BEVEL_ID_START+7)
#define BEVEL_NoInline_ID   (BEVEL_ID_START+8)
#define BEVEL_ID_END        (BEVEL_ID_START+10)

#define TBAR_LAYOUT_ID_START  		(BEVEL_ID_END+1)
#define TBAR_LAYOUT_Buttons_ID     	(TBAR_LAYOUT_ID_START+MYFRAME_TITLE_BACK_BTN)
#define TBAR_LAYOUT_Spacer_ID      	(TBAR_LAYOUT_ID_START+MYFRAME_TITLE_BACK_SPACER)
#define TBAR_LAYOUT_TitleSpacer_ID 	(TBAR_LAYOUT_ID_START+MYFRAME_TITLE_BACK_TITLE_SPACER)

#define TBAR_LAYOUT_ID_END        	(TBAR_LAYOUT_ID_START+10)


/*********************************************************************
 * Window decorations Frame can be defined as such :
 *
 * MyFrame "name"
 *     [Inherit     "name"]
 * #traditional form :
 *     [North       <pixmap>]
 *     [East        <pixmap>]
 *     [South       <pixmap>]
 *     [West        <pixmap>]
 *     [NorthEast   <pixmap>]
 *     [NorthWest   <pixmap>]
 *     [SouthEast   <pixmap>]
 *     [SouthWest   <pixmap>]
 * #alternative form :
 *     [Side        North|South|East|West|Any [<pixmap>]] - if pixmap is ommited -
 *                                                          empty bevel will be drawn
 *     [NoSide      North|South|East|West|Any]
 *     [Corner      NorthEast|SouthEast|NorthWest|SouthWest|Any <pixmap>] - if pixmap is ommited -
 *                                                                          empty bevel will be drawn
 *     [NoCorner    NorthEast|SouthEast|NorthWest|SouthWest|Any]
 * #new settings :
 *     [TitleUnfocusedStyle   <style>
 *     [TitleFocusedStyle     <style>
 *     [TitleStickyStyle      <style>
 *     [FrameUnfocusedStyle   <style>
 *     [FrameFocusedStyle     <style>
 *     [FrameStickyStyle      <style>
 *     [TitleBackground       <pixmap>] - gets overlayed over background and under the text
 * #additional attributes :
 *     [SideSize        North|South|East|West|Any <WIDTHxLENGTH>] - pixmap will be scaled to this size
 *     [SideAlign       North|South|East|West|Any Left,Top,Right,Bottom,HTiled,VTiled,HScaled,VScaled]
 *     [SideBevel       North|South|East|West|Any None|[Left,Top,Right,Bottom,Extra,NoOutline]]
 *     [SideFocusedBevel      North|South|East|West|Any None|[Left,Top,Right,Bottom,Extra,NoOutline]]
 *     [SideUnfocusedBevel    North|South|East|West|Any None|[Left,Top,Right,Bottom,Extra,NoOutline]]
 *     [SideStickyBevel       North|South|East|West|Any None|[Left,Top,Right,Bottom,Extra,NoOutline]]
 *     [CornerSize      NorthEast|SouthEast|NorthWest|SouthWest|Any <WIDTHxHEIGHT>]
 *     [CornerAlign     NorthEast|SouthEast|NorthWest|SouthWest|Any Left,Top,Right,Bottom,HTiled,VTiled,HScaled,VScaled]
 *     [CornerBevel     NorthEast|SouthEast|NorthWest|SouthWest|Any None|[Left,Top,Right,Bottom,Extra,NoOutline]]
 *     [CornerFocusedBevel    NorthEast|SouthEast|NorthWest|SouthWest|Any None|[Left,Top,Right,Bottom,Extra,NoOutline]]
 *     [CornerUnfocusedBevel  NorthEast|SouthEast|NorthWest|SouthWest|Any None|[Left,Top,Right,Bottom,Extra,NoOutline]]
 *     [CornerStickyBevel     NorthEast|SouthEast|NorthWest|SouthWest|Any None|[Left,Top,Right,Bottom,Extra,NoOutline]]
 *     [TitleBevel      None|[Left,Top,Right,Bottom,Extra,NoOutline]
 *     [TitleFocusedBevel     None|[Left,Top,Right,Bottom,Extra,NoOutline]
 *     [TitleUnfocusedBevel   None|[Left,Top,Right,Bottom,Extra,NoOutline]
 *     [TitleStickyBevel      None|[Left,Top,Right,Bottom,Extra,NoOutline]
 *     [TitleAlign      None|[Left,Top,Right,Bottom]
 *     [TitleBackgroundAlign  None|[Left,Top,Right,Bottom,HTiled,VTiled,HScaled,VScaled,LabelSize]
 *     [TitleCompositionMethod   testure_type]
 *     [TitleFocusedCompositionMethod     testure_type]
 *     [TitleUnfocusedCompositionMethod   testure_type]
 *     [TitleStickyCompositionMethod      testure_type]
 * 	   [CondenseTitlebar]
 * 	   [LeftTitlebarLayout		Buttons,Spacer,TitleSpacer]			  
 * 	   [RightTitlebarLayout		Buttons,Spacer,TitleSpacer]			  
 * ~MyFrame
 */

extern struct SyntaxDef MyFrameSyntax;
/* use this in module term definition to add MyStyle parsing functionality */
#define INCLUDE_MYFRAME	{TF_NO_MYNAME_PREPENDING,"MyFrame", 7, TT_QUOTED_TEXT, MYFRAME_START_ID, &MyFrameSyntax}

typedef struct MyFrameDefinition
{
    struct MyFrameDefinition *next;

    char        *name;
    ASFlagType   set_parts;
    ASFlagType   parts_mask; /* first 8 bits represent one enabled side/corner each */
    char        *parts[FRAME_PARTS];
    char        *title_styles[BACK_STYLES];
    char        *frame_styles[BACK_STYLES];
	char        *title_backs[MYFRAME_TITLE_BACKS];
    ASFlagType   set_part_size ;
    unsigned int part_width[FRAME_PARTS];
    unsigned int part_length[FRAME_PARTS];
    ASFlagType   set_part_bevel ;
    ASFlagType   part_fbevel[FRAME_PARTS];
    ASFlagType   part_ubevel[FRAME_PARTS];
    ASFlagType   part_sbevel[FRAME_PARTS];
    ASFlagType   set_part_align ;
    ASFlagType   part_align[FRAME_PARTS];
    ASFlagType   set_title_attr ;
    ASFlagType   title_fbevel, title_ubevel, title_sbevel;
    ASFlagType   title_align ;
	ASFlagType   title_backs_align[MYFRAME_TITLE_BACKS];
    int          title_fcm, title_ucm, title_scm;
	char 		*title_fhue, *title_uhue, *title_shue; 
	int          title_fsat, title_usat, title_ssat;
	int          title_h_spacing, title_v_spacing;
    ASFlagType   condense_titlebar ;
	unsigned long left_layout, right_layout ;
	ASFlagType   left_btn_align, right_btn_align ;

	ASFlagType   flags ;
	ASFlagType   set_flags ;

    char       **inheritance_list ;
    int          inheritance_num ;

}MyFrameDefinition;

/* this functions work exactly like MyStyle stuff ( see above ) */
void PrintMyFrameDefinitions (MyFrameDefinition * list, int index);
MyFrameDefinition *AddMyFrameDefinition (MyFrameDefinition ** tail);
void DestroyMyFrameDefinitions (MyFrameDefinition ** list);
MyFrameDefinition **ProcessMyFrameOptions (struct FreeStorageElem * options,
					   MyFrameDefinition ** tail);

/* converts MYFRAME defs back into FreeStorage */
struct FreeStorageElem **MyFrameDefs2FreeStorage (struct SyntaxDef * syntax,
					   struct FreeStorageElem ** tail,
					   MyFrameDefinition * defs);

void myframe_parse (char *tline, FILE * fd, char **myname, int *myframe_list);
/**************************************************************************/
/**************************************************************************/
/*                        balloon pasring definitions                       */
/**************************************************************************/

#define BALLOON_ID_START            (TBAR_LAYOUT_ID_END+1)

#define BALLOON_Balloons_ID			 BALLOON_ID_START
#define BALLOON_USED_ID	 			 BALLOON_Balloons_ID
#define BALLOON_BorderHilite_ID     (BALLOON_ID_START+1)
#define BALLOON_XOffset_ID          (BALLOON_ID_START+2)
#define BALLOON_YOffset_ID          (BALLOON_ID_START+3)
#define BALLOON_Delay_ID            (BALLOON_ID_START+4)
#define BALLOON_CloseDelay_ID       (BALLOON_ID_START+5)
#define BALLOON_Style_ID            (BALLOON_ID_START+6)
#define BALLOON_TextPaddingX_ID     (BALLOON_ID_START+7)
#define BALLOON_TextPaddingY_ID     (BALLOON_ID_START+8)
#define BALLOON_NoBalloons_ID	 	(BALLOON_ID_START+9)

#define	BALLOON_ID_END				(BALLOON_ID_START+10)

#define BALLOON_FLAG_TERM \
 {0, "Balloons",    8, TT_FLAG , BALLOON_Balloons_ID   , NULL}, \
 {0, "NoBalloons", 10, TT_FLAG , BALLOON_NoBalloons_ID   , NULL}

#define BALLOON_FEEL_TERMS \
 {0, "BalloonXOffset", 14, TT_INTEGER, BALLOON_XOffset_ID        , NULL}, \
 {0, "BalloonYOffset", 14, TT_INTEGER, BALLOON_YOffset_ID        , NULL}, \
 {0, "BalloonDelay", 12, TT_UINTEGER, BALLOON_Delay_ID      , NULL}, \
 {0, "BalloonCloseDelay", 17, TT_UINTEGER, BALLOON_CloseDelay_ID , NULL}

#define BALLOON_LOOK_TERMS \
 {0, "BalloonBorderHilite",19, TT_FLAG, BALLOON_BorderHilite_ID, &BevelSyntax}, \
 {0, "BalloonStyle", 12, TT_QUOTED_TEXT, BALLOON_Style_ID , NULL}, \
 {0, "BalloonTextPaddingX",19, TT_INTEGER, BALLOON_TextPaddingX_ID , NULL}, \
 {0, "BalloonTextPaddingY",19, TT_INTEGER, BALLOON_TextPaddingY_ID , NULL}

#define BALLOON_TERMS BALLOON_FLAG_TERM,BALLOON_FEEL_TERMS,BALLOON_LOOK_TERMS

typedef struct balloonConfig
{
  unsigned long set_flags;	/* identifyes what option is set */
  unsigned long flags;	
#define BALLOON_Balloons			(0x01<<0)
#define BALLOON_USED 				BALLOON_Balloons
#define BALLOON_BorderHilite        (0x01<<1)
#define BALLOON_XOffset             (0x01<<2)
#define BALLOON_YOffset             (0x01<<3)
#define BALLOON_Delay               (0x01<<4)
#define BALLOON_CloseDelay			(0x01<<5)
#define BALLOON_Style               (0x01<<6)
#define BALLOON_TextPaddingX		(0x01<<7)
#define BALLOON_TextPaddingY		(0x01<<8)
  ASFlagType BorderHilite;
  int XOffset, YOffset;
  unsigned int Delay, CloseDelay;
  char *Style ;
  int TextPaddingX, TextPaddingY;
}balloonConfig;

balloonConfig *Create_balloonConfig ();
void Destroy_balloonConfig (balloonConfig * config);
balloonConfig *Process_balloonOptions (struct FreeStorageElem * options,
				       balloonConfig * config);
void Print_balloonConfig (balloonConfig * config);
struct FreeStorageElem **balloon2FreeStorage (struct SyntaxDef * syntax,
				       struct FreeStorageElem ** tail,
				       balloonConfig * config);

void MergeBalloonOptions ( ASModuleConfig *asm_to, ASModuleConfig *asm_from);
struct ASBalloonLook;
void balloon_config2look( struct MyLook *look, struct ASBalloonLook *balloon_look, balloonConfig *config, const char *default_style );


/***************************************************************************/

/***************************************************************************/
extern char *pixmapPath;
/****************************************************************************/
/*                             Pager                                        */
/****************************************************************************/
/* flags used in configuration */
#define USE_LABEL   			(1<<0)
#define START_ICONIC			(1<<1)
#define REDRAW_BG				(1<<2)
#define STICKY_ICONS			(1<<3)
#define LABEL_BELOW_DESK 		(1<<4)
#define HIDE_INACTIVE_LABEL		(1<<5)
#define PAGE_SEPARATOR			(1<<6)
#define DIFFERENT_GRID_COLOR	(1<<7)
#define DIFFERENT_BORDER_COLOR	(1<<8)
#define SHOW_SELECTION          (1<<9)
#define FAST_STARTUP			(1<<10)
#define SET_ROOT_ON_STARTUP		(1<<11)
#define VERTICAL_LABEL          (1<<12)
#define PAGER_FLAGS_MAX_SHIFT   12
#define PAGER_FLAGS_DEFAULT	(USE_LABEL|REDRAW_BG|PAGE_SEPARATOR|SHOW_SELECTION)
/* set/unset flags : */
#define PAGER_SET_GEOMETRY  		(1<<16)
#define PAGER_SET_ICON_GEOMETRY  	(1<<17)
#define PAGER_SET_ALIGN 			(1<<18)
#define PAGER_SET_SMALL_FONT 		(1<<19)
#define PAGER_SET_ROWS 				(1<<20)
#define PAGER_SET_COLUMNS 			(1<<21)
#define PAGER_SET_SELECTION_COLOR	(1<<22)
#define PAGER_SET_GRID_COLOR		(1<<23)
#define PAGER_SET_BORDER_COLOR		(1<<24)
#define PAGER_SET_BORDER_WIDTH		(1<<25)
#define PAGER_SET_ACTIVE_BEVEL      (1<<26)
#define PAGER_SET_INACTIVE_BEVEL    (1<<27)


/* ID's used in our config */
#define PAGER_ID_START      (BALLOON_ID_END+1)
#define PAGER_GEOMETRY_ID 	(PAGER_ID_START)
#define PAGER_ICON_GEOMETRY_ID  (PAGER_ID_START+1)
#define PAGER_ALIGN_ID		(PAGER_ID_START+2)
#define PAGER_DRAW_BG_ID	(PAGER_ID_START+3)
#define PAGER_FAST_STARTUP_ID	(PAGER_ID_START+4)
#define PAGER_SET_ROOT_ID	(PAGER_ID_START+5)
#define PAGER_SMALL_FONT_ID	(PAGER_ID_START+6)
#define PAGER_START_ICONIC_ID	(PAGER_ID_START+7)
#define PAGER_ROWS_ID		(PAGER_ID_START+8)
#define PAGER_COLUMNS_ID	(PAGER_ID_START+9)
#define PAGER_STICKY_ICONS_ID	(PAGER_ID_START+10)
#define PAGER_LABEL_ID 		(PAGER_ID_START+11)
#define PAGER_STYLE_ID      (PAGER_ID_START+12)
#define PAGER_SHADE_BUTTON_ID   (PAGER_ID_START+13)

#define PAGER_DECORATION_ID	(PAGER_ID_START+20)
#define PAGER_MYSTYLE_ID	(PAGER_ID_START+21)
#define PAGER_BALLOONS_ID	(PAGER_ID_START+22)

#define PAGER_DECOR_NOLABEL_ID 		(PAGER_ID_START+30)
#define PAGER_DECOR_NOSEPARATOR_ID 	(PAGER_ID_START+31)
#define PAGER_DECOR_NOSELECTION_ID 	(PAGER_ID_START+32)
#define PAGER_DECOR_SEL_COLOR_ID 	(PAGER_ID_START+33)
#define PAGER_DECOR_GRID_COLOR_ID 	(PAGER_ID_START+34)
#define PAGER_DECOR_BORDER_WIDTH_ID (PAGER_ID_START+35)
#define PAGER_DECOR_BORDER_COLOR_ID	(PAGER_ID_START+36)
#define PAGER_DECOR_LABEL_BELOW_ID	(PAGER_ID_START+37)
#define PAGER_DECOR_HIDE_INACTIVE_ID    (PAGER_ID_START+38)
#define PAGER_DECOR_VERTICAL_LABEL_ID   (PAGER_ID_START+39)
#define PAGER_ActiveBevel_ID        (PAGER_ID_START+40)
#define PAGER_InActiveBevel_ID      (PAGER_ID_START+41)

#define PAGER_ID_END                (PAGER_ID_START+50)
/* config data structure */

typedef struct
  {
    int    rows, columns;
    ASGeometry geometry, icon_geometry;
	int ndesks ;
    char **labels;
    char **styles;
    int    align;
    unsigned long flags, set_flags;
    char  *small_font_name;
    int    border_width;
    char  *shade_button[2];

    char *selection_color;
    char *grid_color;
    char *border_color;

	int h_spacing, v_spacing ;

    balloonConfig *balloon_conf;
    MyStyleDefinition *style_defs;
    struct FreeStorageElem *more_stuff;

    /* these are generated after reading the config : */
    int gravity ;
    ARGB32  selection_color_argb;
    ARGB32  grid_color_argb;
    ARGB32  border_color_argb;
#define DESK_ACTIVE     0
#define DESK_INACTIVE   1
#define DESK_STYLES     2
    struct MyStyle *MSDeskTitle[DESK_STYLES];
    struct MyStyle **MSDeskBack;

    ASFlagType  active_desk_bevel ;
    ASFlagType  inactive_desk_bevel ;

}PagerConfig;

PagerConfig *CreatePagerConfig (int ndesks);
PagerConfig *ParsePagerOptions (const char *filename, char *myname, int desk1, int desk2);
int WritePagerOptions (const char *filename, char *myname, int desk1, int desk2, PagerConfig * config, unsigned long flags);
void DestroyPagerConfig (PagerConfig * config);

/**************************************************************************/
/***************************************************************************/
/*                        MyBackground pasring definitions                 */
/***************************************************************************/
#define BGR_ID_START        	PAGER_ID_END
#define BGR_MYBACKGROUND        BGR_ID_START
#define BGR_USE		        BGR_ID_START+1
#define BGR_CUT       		BGR_ID_START+2
#define BGR_TINT	        BGR_ID_START+3
#define BGR_SCALE	        BGR_ID_START+4
#define BGR_ALIGN		BGR_ID_START+5
#define BGR_PAD		        BGR_ID_START+6
#define BGR_MYBACKGROUND_END    BGR_ID_START+7

#define BGR_DESK_BACK     	BGR_ID_START+8
#define BGR_ID_END        	BGR_ID_START+20

typedef struct my_background_config
  {

    char *name;
    unsigned long flags;
    char *data;
    ASGeometry cut;
    char *tint;
    ASGeometry scale;
    char *pad;
    struct my_background_config *next;
  }
MyBackgroundConfig;

typedef struct desk_back_config
  {
    int desk;
    char *back_name;
    MyBackgroundConfig *back;

    struct desk_back_config *next;
  }
DeskBackConfig;

typedef struct
  {
    MyBackgroundConfig *my_backs;
    DeskBackConfig *my_desks;

    MyStyleDefinition *style_defs;

    struct FreeStorageElem *more_stuff;
  }
ASetRootConfig;

MyBackgroundConfig *ParseMyBackgroundOptions (struct FreeStorageElem * Storage, char *myname);
ASetRootConfig *ParseASetRootOptions (const char *filename, char *myname);
/*
 * all data members that has been used from ASetRootConfig structure, returned
 * by this call must be set to NULL, or memory allocated for them will be
 * deallocated by the following DestroyBaseConfig function !
 */

void DestroyASetRootConfig (ASetRootConfig * config);
void myback_parse (char *tline, FILE * fd, char **myname, int *mylook);
void DestroyMyBackgroundConfig (MyBackgroundConfig ** head);


/***************************************************************************/
/***************************************************************************/
/*                        WinList config parsing definitions               */
/***************************************************************************/
/* New winlist config :
 *
 *	*WinListGeometry		+x+y
 *  *WinListMinSize			WxH
 *  *WinListMaxSize			WxH
 *  *WinListMaxRows			count
 *  *WinListMaxColumns		count
 *  *WinListMinColWidth		width
 *  *WinListMaxColWidth		width
 *  *WinListFillRowsFirst
 *  *WinListUseSkipList
 *  *WinListUnfocusedStyle 	"style"
 *  *WinListFocusedStyle 	"style"
 *  *WinListStickyStyle 	"style"
 *	*WinListUseName			0|1|2|3   # 0 - Name, 1 - icon, 2 - res_name, 3 - res_class
 *  *WinListAlign           Left,Right,Top,Bottom
 *  *WinListBevel           None,Left,Right,Top, Bottom, NoOutline
 *  *WinListFBevel           None,Left,Right,Top, Bottom, NoOutline
 *  *WinListUBevel           None,Left,Right,Top, Bottom, NoOutline
 *  *WinListSBevel           None,Left,Right,Top, Bottom, NoOutline
 *  *WinListAction          [Click]1|2|3|4|5
 *  *WinListShapeToContents
 *
 * Depreciated functions :
 *
 *  *WinListMaxWidth        width
 *  *WinListOrientation		across|vertical
 *
 * Obsolete functions :
 *
 *  *WinListHideGeometry	WxH+x+y
 *  *WinListNoAnchor
 *  *WinListUseIconNames
 *  *WinListAutoHide
 */
#define WINLIST_ID_START        		(BGR_ID_END+1)
#define WINLIST_FillRowsFirst_ID		(WINLIST_ID_START)
#define WINLIST_UseSkipList_ID			(WINLIST_ID_START+1)
#define WINLIST_Geometry_ID				(WINLIST_ID_START+2)
#define WINLIST_MinSize_ID				(WINLIST_ID_START+3)
#define WINLIST_MaxSize_ID				(WINLIST_ID_START+4)
#define WINLIST_MaxRows_ID				(WINLIST_ID_START+5)
#define WINLIST_MaxColumns_ID			(WINLIST_ID_START+6)
#define WINLIST_MaxColWidth_ID			(WINLIST_ID_START+7)
#define WINLIST_MinColWidth_ID			(WINLIST_ID_START+8)
#define WINLIST_UseName_ID				(WINLIST_ID_START+9)
#define WINLIST_Align_ID                (WINLIST_ID_START+10)
#define WINLIST_Bevel_ID                (WINLIST_ID_START+11)
#define WINLIST_FBevel_ID               (WINLIST_ID_START+12)
#define WINLIST_FocusedBevel_ID         WINLIST_FBevel_ID
#define WINLIST_UBevel_ID               (WINLIST_ID_START+13)
#define WINLIST_UnfocusedBevel_ID       WINLIST_UBevel_ID
#define WINLIST_SBevel_ID               (WINLIST_ID_START+14)
#define WINLIST_StickyBevel_ID          WINLIST_SBevel_ID
#define WINLIST_Action_ID               (WINLIST_ID_START+15)
#define WINLIST_UnfocusedStyle_ID       (WINLIST_ID_START+16)
#define WINLIST_FocusedStyle_ID         (WINLIST_ID_START+17)
#define WINLIST_StickyStyle_ID          (WINLIST_ID_START+18)
#define WINLIST_UrgentStyle_ID          (WINLIST_ID_START+19)
#define WINLIST_ShapeToContents_ID      (WINLIST_ID_START+20)
#define WINLIST_CompositionMethod_ID    (WINLIST_ID_START+21)
#define WINLIST_FCompositionMethod_ID   (WINLIST_ID_START+22)
#define WINLIST_UCompositionMethod_ID   (WINLIST_ID_START+23)
#define WINLIST_SCompositionMethod_ID   (WINLIST_ID_START+24)
#define WINLIST_Spacing_ID              (WINLIST_ID_START+25)
#define WINLIST_HSpacing_ID             (WINLIST_ID_START+26)
#define WINLIST_VSpacing_ID             (WINLIST_ID_START+27)


#define WINLIST_BALLOONS_ID             (WINLIST_ID_START+28)

#define WINLIST_HideGeometry_ID         (WINLIST_ID_START+29)
#define WINLIST_MaxWidth_ID             (WINLIST_ID_START+30)
#define WINLIST_Orientation_ID          (WINLIST_ID_START+31)
#define WINLIST_NoAnchor_ID             (WINLIST_ID_START+32)
#define WINLIST_UseIconNames_ID         (WINLIST_ID_START+33)
#define WINLIST_AutoHide_ID             (WINLIST_ID_START+34)

#define WINLIST_ShowIcon_ID		  		(WINLIST_ID_START+35)
#define WINLIST_IconLocation_ID         (WINLIST_ID_START+36)
#define WINLIST_IconAlign_ID	        (WINLIST_ID_START+37)
#define WINLIST_IconSize_ID				(WINLIST_ID_START+38)
#define WINLIST_ScaleIconToTextHeight_ID (WINLIST_ID_START+39)

#define WINLIST_ID_END                  (WINLIST_ID_START+40)

/* config data structure */


typedef struct WinListConfig
{
#define WINLIST_FillRowsFirst	(0x01<<0)
#define WINLIST_UseSkipList		(0x01<<1)
#define WINLIST_Geometry		(0x01<<2)
#define WINLIST_MinSize			(0x01<<3)
#define WINLIST_MaxSize			(0x01<<4)
#define WINLIST_MaxRows			(0x01<<5)
#define WINLIST_MaxColumns		(0x01<<6)
#define WINLIST_MaxColWidth		(0x01<<7)
#define WINLIST_MinColWidth		(0x01<<8)
#define WINLIST_UseName			(0x01<<9)
#define WINLIST_Align           (0x01<<10)
#define WINLIST_FBevel          (0x01<<11)
#define WINLIST_UBevel          (0x01<<12)
#define WINLIST_SBevel          (0x01<<13)
#define WINLIST_Bevel           (WINLIST_FBevel|WINLIST_UBevel|WINLIST_SBevel)
#define WINLIST_ShapeToContents (0x01<<14)
#define WINLIST_FCompositionMethod             (0x01<<15)
#define WINLIST_UCompositionMethod             (0x01<<16)
#define WINLIST_SCompositionMethod             (0x01<<17)
#define WINLIST_CompositionMethod              (WINLIST_FCompositionMethod|WINLIST_UCompositionMethod|WINLIST_SCompositionMethod)

#define WINLIST_HSpacing       	(0x01<<18)
#define WINLIST_VSpacing       	(0x01<<19)
#define WINLIST_Spacing       	(WINLIST_HSpacing|WINLIST_VSpacing)

#define WINLIST_ShowIcon        (0x01<<20)
#define WINLIST_IconLocation    (0x01<<21)
#define WINLIST_IconAlign	    (0x01<<22)
#define WINLIST_IconSize		(0x01<<23)
#define WINLIST_ScaleIconToTextHeight (0x01<<24)
	   

#define 	ASWL_RowsFirst 				WINLIST_FillRowsFirst
#define 	ASWL_UseSkipList			WINLIST_UseSkipList
#define 	ASWL_ShowIcon				WINLIST_ShowIcon
#define 	ASWL_ScaleIconToTextHeight	WINLIST_ScaleIconToTextHeight

	ASModuleConfig asmodule_config;

	ASFlagType	flags ;
	ASFlagType	set_flags ;
	
    ASGeometry 	Geometry ;
    ASGeometry	   	MinSize ;
	ASGeometry		MaxSize ;
#define MAX_WINLIST_WINDOW_COUNT    512        /* 512 x 4 == 2048 == 1 page in memory */
	unsigned int MaxRows, MaxColumns ;
	unsigned int MaxColWidth, MinColWidth ;

/* phony flags */
#define WINLIST_UnfocusedStyle	0
#define WINLIST_FocusedStyle 	0
#define WINLIST_StickyStyle 	0
#define WINLIST_UrgentStyle 	0
	char *UnfocusedStyle ;
	char *FocusedStyle ;
	char *StickyStyle ;
	char *UrgentStyle ;

	ASNameTypes     UseName ; /* 0, 1, 2, 3 */
    ASFlagType      Align ;
    ASFlagType      FBevel, UBevel, SBevel ;
    int             UCompositionMethod, FCompositionMethod, SCompositionMethod;
    unsigned int    HSpacing, VSpacing;
    ASFlagType      IconAlign ;
	int 			IconLocation ;
	ASGeometry	 	IconSize ;

    char **Action[MAX_MOUSE_BUTTONS];

    /* calculated based on geometry : */
    int anchor_x, anchor_y ;
	int gravity ;

}WinListConfig;

ASModuleConfigClass *WinListConfigClass ;

#define AS_WINLIST_CONFIG(p) AS_MODULE_CONFIG_TYPED(p,CONFIG_WinList_ID,WinListConfig)

ASModuleConfig *CreateWinListConfig ();
void DestroyWinListConfig (ASModuleConfig *config);
void PrintWinListConfig (WinListConfig *config);
int WriteWinListOptions (const char *filename, char *myname, WinListConfig * config, unsigned long flags);
WinListConfig *ParseWinListOptions (const char *filename, char *myname);
void MergeWinListOptions ( ASModuleConfig *to, ASModuleConfig *from);
void CheckWinListConfigSanity(WinListConfig *Config, ASGeometry *default_geometry, int default_gravity);



/**************************************************************************/
/*                        database pasring definitions                    */
/**************************************************************************/


#define GRAVITY_ID_START            (WINLIST_ID_END+1)

#define GRAVITY_NorthWest_ID        (GRAVITY_ID_START+NorthWestGravity)
#define GRAVITY_North_ID            (GRAVITY_ID_START+NorthGravity)
#define GRAVITY_NorthEast_ID        (GRAVITY_ID_START+NorthEastGravity)
#define GRAVITY_West_ID             (GRAVITY_ID_START+WestGravity)
#define GRAVITY_Center_ID           (GRAVITY_ID_START+CenterGravity)
#define GRAVITY_East_ID             (GRAVITY_ID_START+EastGravity)
#define GRAVITY_SouthWest_ID        (GRAVITY_ID_START+SouthWestGravity)
#define GRAVITY_South_ID            (GRAVITY_ID_START+SouthGravity)
#define GRAVITY_SouthEast_ID        (GRAVITY_ID_START+SouthEastGravity)
#define GRAVITY_Static_ID           (GRAVITY_ID_START+StaticGravity)
#define GRAVITY_ID_END              (GRAVITY_ID_START+11)

#define DATABASE_ID_START           (GRAVITY_ID_END+1)
#define DATABASE_STYLE_ID            DATABASE_ID_START
#define DATABASE_Icon_ID            (DATABASE_ID_START+1)
#define DATABASE_NoIcon_ID          (DATABASE_ID_START+2)
#define DATABASE_FocusStyle_ID		(DATABASE_ID_START+3)
#define DATABASE_UnfocusStyle_ID	(DATABASE_ID_START+4)
#define DATABASE_StickyStyle_ID		(DATABASE_ID_START+5)
#define DATABASE_NoIconTitle_ID		(DATABASE_ID_START+6)
#define DATABASE_IconTitle_ID		(DATABASE_ID_START+7)
#define DATABASE_Focus_ID           (DATABASE_ID_START+8)
#define DATABASE_NoFocus_ID         (DATABASE_ID_START+9)
#define DATABASE_NoTitle_ID         (DATABASE_ID_START+10)
#define DATABASE_Title_ID           (DATABASE_ID_START+11)
#define DATABASE_NoHandles_ID		(DATABASE_ID_START+12)
#define DATABASE_Handles_ID         (DATABASE_ID_START+13)
#define DATABASE_NoButton_ID		(DATABASE_ID_START+14)
#define DATABASE_Button_ID          (DATABASE_ID_START+15)
#define DATABASE_WindowListSkip_ID	(DATABASE_ID_START+16)
#define DATABASE_WindowListHit_ID	(DATABASE_ID_START+17)
#define DATABASE_CirculateSkip_ID	(DATABASE_ID_START+18)
#define DATABASE_CirculateHit_ID	(DATABASE_ID_START+19)
#define DATABASE_StartIconic_ID		(DATABASE_ID_START+20)
#define DATABASE_StartNormal_ID		(DATABASE_ID_START+21)
#define DATABASE_Layer_ID           (DATABASE_ID_START+22)
#define DATABASE_StaysOnTop_ID		(DATABASE_ID_START+23)
#define DATABASE_StaysPut_ID		(DATABASE_ID_START+24)
#define DATABASE_StaysOnBack_ID		(DATABASE_ID_START+25)
#define DATABASE_AvoidCover_ID		(DATABASE_ID_START+26)
#define DATABASE_AllowCover_ID		(DATABASE_ID_START+27)
#define DATABASE_VerticalTitle_ID	(DATABASE_ID_START+28)
#define DATABASE_HorizontalTitle_ID	(DATABASE_ID_START+29)
#define DATABASE_Sticky_ID          (DATABASE_ID_START+30)
#define DATABASE_Slippery_ID		(DATABASE_ID_START+31)
#define DATABASE_BorderWidth_ID	 	(DATABASE_ID_START+32)
#define DATABASE_HandleWidth_ID		(DATABASE_ID_START+33)
#define DATABASE_StartsOnDesk_ID	(DATABASE_ID_START+34)
#define DATABASE_ViewportX_ID		(DATABASE_ID_START+35)
#define DATABASE_ViewportY_ID		(DATABASE_ID_START+36)
#define DATABASE_StartsAnywhere_ID	(DATABASE_ID_START+37)
#define DATABASE_NoFrame_ID         (DATABASE_ID_START+38)
#define DATABASE_Frame_ID           (DATABASE_ID_START+39)
#define DATABASE_Windowbox_ID       (DATABASE_ID_START+40)
#define DATABASE_DefaultGeometry_ID        (DATABASE_ID_START+41)
#define DATABASE_OverrideGravity_ID        (DATABASE_ID_START+42)
#define DATABASE_HonorPPosition_ID         (DATABASE_ID_START+43)
#define DATABASE_NoPPosition_ID            (DATABASE_ID_START+44)
#define DATABASE_HonorGroupHints_ID        (DATABASE_ID_START+45)
#define DATABASE_NoGroupHints_ID           (DATABASE_ID_START+46)
#define DATABASE_HonorTransientHints_ID    (DATABASE_ID_START+47)
#define DATABASE_NoTransientHints_ID       (DATABASE_ID_START+48)
#define DATABASE_HonorMotifHints_ID        (DATABASE_ID_START+49)
#define DATABASE_NoMotifHints_ID           (DATABASE_ID_START+50)
#define DATABASE_HonorGnomeHints_ID        (DATABASE_ID_START+51)
#define DATABASE_NoGnomeHints_ID           (DATABASE_ID_START+52)
#define DATABASE_HonorKDEHints_ID          (DATABASE_ID_START+53)
#define DATABASE_NoKDEHints_ID             (DATABASE_ID_START+54)
#define DATABASE_HonorExtWMHints_ID        (DATABASE_ID_START+55)
#define DATABASE_NoExtWMHints_ID           (DATABASE_ID_START+56)
#define DATABASE_HonorXResources_ID        (DATABASE_ID_START+57)
#define DATABASE_NoXResources_ID           (DATABASE_ID_START+58)
#define DATABASE_FocusOnMap_ID             (DATABASE_ID_START+59)
#define DATABASE_NoFocusOnMap_ID           (DATABASE_ID_START+60)
#define DATABASE_LongLiving_ID             (DATABASE_ID_START+61)
#define DATABASE_ShortLiving_ID            (DATABASE_ID_START+62)
#define DATABASE_IgnoreConfig_ID           (DATABASE_ID_START+63)
#define DATABASE_HonorConfig_ID            (DATABASE_ID_START+64)
#define DATABASE_UseCurrentViewport_ID     (DATABASE_ID_START+65)
#define DATABASE_UseAnyViewport_ID		   (DATABASE_ID_START+66)
#define DATABASE_Fullscreen_ID             (DATABASE_ID_START+67)
#define DATABASE_NoFullscreen_ID		   (DATABASE_ID_START+68)
#define DATABASE_WindowOpacity_ID		   (DATABASE_ID_START+69)


#define DATABASE_ID_END             (DATABASE_ID_START+70)

/* we use name_list structure 1 to 1 in here, as it does not requre any
   preprocessing from us
 */
struct name_list;

unsigned int translate_title_button (unsigned int user_button);
unsigned int translate_title_button_back (unsigned int title_button);
struct name_list *ParseDatabaseOptions (const char *filename, char *myname);
struct name_list *string2DatabaseStyle (char *style_txt);

int WriteDatabaseOptions (const char *filename, char *myname,
			  struct name_list *config, unsigned long flags);
Bool ReloadASDatabase();

/**************************************************************************/
/*                        Wharf pasring definitions                       */
/**************************************************************************/

#define WHARF_ID_START          (DATABASE_ID_END+1)
#define WHARF_Wharf_ID			(WHARF_ID_START)
#define WHARF_FolderEnd_ID		(WHARF_ID_START+1)
#define WHARF_Geometry_ID		(WHARF_ID_START+2)
#define WHARF_Rows_ID			(WHARF_ID_START+3)
#define WHARF_Columns_ID		(WHARF_ID_START+4)
#define WHARF_NoPush_ID			(WHARF_ID_START+5)
#define WHARF_FullPush_ID		(WHARF_ID_START+6)
#define WHARF_NoBorder_ID		(WHARF_ID_START+7)
#define WHARF_WithdrawStyle_ID		(WHARF_ID_START+8)
/* the NoWithdraw option is undocumented, deprecated, and
 ** may be removed at Wharf's maintainer's discretion */
#define WHARF_NoWithdraw_ID  		(WHARF_ID_START+9)
#define WHARF_ForceSize_ID 		(WHARF_ID_START+10)
/* TextureType, MaxColors, BgColor, TextureColor, and Pixmap are obsolete */
#define WHARF_TextureType_ID		(WHARF_ID_START+11)
#define WHARF_MaxColors_ID 		(WHARF_ID_START+12)
#define WHARF_BgColor_ID 		(WHARF_ID_START+13)
#define WHARF_TextureColor_ID		(WHARF_ID_START+14)
#define WHARF_Pixmap_ID 		(WHARF_ID_START+15)
#define WHARF_AnimateStepsMain_ID	(WHARF_ID_START+16)
#define WHARF_AnimateSteps_ID		(WHARF_ID_START+17)
#define WHARF_AnimateDelay_ID		(WHARF_ID_START+18)
#define WHARF_AnimateMain_ID 		(WHARF_ID_START+19)
#define WHARF_Animate_ID		(WHARF_ID_START+20)

#define WHARF_Player_ID 		(WHARF_ID_START+21)
#define WHARF_Sound_ID 			(WHARF_ID_START+22)
#define WHARF_ShowLabel_ID      (WHARF_ID_START+23)
#define WHARF_LabelLocation_ID  (WHARF_ID_START+24)
#define WHARF_FlipLabel_ID      (WHARF_ID_START+25)
#define WHARF_FitContents_ID    (WHARF_ID_START+26)
#define WHARF_ShapeToContents_ID    (WHARF_ID_START+27)
#define WHARF_AlignContents_ID  (WHARF_ID_START+28)
#define WHARF_Bevel_ID  (WHARF_ID_START+29)
#define WHARF_CompositionMethod_ID  (WHARF_ID_START+30)
#define WHARF_FolderOffset_ID   (WHARF_ID_START+31)
#define WHARF_OrthogonalFolderOffset_ID   (WHARF_ID_START+32)
#define WHARF_StretchBackground_ID   (WHARF_ID_START+33)

#define WHARF_ID_END            (WHARF_ID_START+34)
#define WFUNC_START_ID			(WHARF_ID_END)

#define WFUNC_Folders_ID		(WFUNC_START_ID)
#define WFUNC_Swallow_ID		(WFUNC_START_ID+1)

#define	WFUNC_ID_END			(WFUNC_START_ID+10)

#define WHEV_PUSH		0
#define WHEV_CLOSE_FOLDER	1
#define WHEV_OPEN_FOLDER	2
#define WHEV_CLOSE_MAIN		3
#define WHEV_OPEN_MAIN		4
#define WHEV_DROP		5
#define WHEV_MAX_EVENTS		6

#define WHEV_START_ID			(WFUNC_ID_END)

#define WHEV_Id2Code(id)		((id)-WHEV_START_ID)
#define WHEV_Code2Id(code)		((code)+WHEV_START_ID)

#define WHEV_PUSH_ID			WHEV_Code2Id(WHEV_PUSH)
#define WHEV_CLOSE_FOLDER_ID		WHEV_Code2Id(WHEV_CLOSE_FOLDER)
#define WHEV_OPEN_FOLDER_ID		WHEV_Code2Id(WHEV_OPEN_FOLDER)
#define WHEV_CLOSE_MAIN_ID		WHEV_Code2Id(WHEV_CLOSE_MAIN)
#define WHEV_OPEN_MAIN_ID		WHEV_Code2Id(WHEV_OPEN_MAIN)
#define WHEV_DROP_ID			WHEV_Code2Id(WHEV_DROP)

#define WHEV_END_ID			WHEV_Code2Id(WHEV_MAX_EVENTS)


struct WharfFolder;
struct FunctionData;

typedef struct WharfButtonContent
{
	char **icon;
  	struct FunctionData *function;
	Bool unavailable ;
}WharfButtonContent ;

typedef struct WharfButton
{
#define WHARF_BUTTON_WIDTH			(0x01<<0)
#define WHARF_BUTTON_HEIGHT	  		(0x01<<1)
#define WHARF_BUTTON_SIZE	  		(WHARF_BUTTON_HEIGHT|WHARF_BUTTON_WIDTH)
#define WHARF_BUTTON_TRANSIENT		(0x01<<2)
#define WHARF_BUTTON_DISABLED		(0x01<<3)

  unsigned long set_flags;
  char *title;
  unsigned int width, height;

  /* there could be several functions/icons assigned to the wharf button,
   * we will use the first available one (non-exec function or exec with available application ) */
  WharfButtonContent *contents ;
  int contents_num, selected_content ;

  struct WharfButton  *next;
  struct WharfButton  *folder;
}
WharfButton;

#define  WHARF_GEOMETRY         (0x01<<0)
#define  WHARF_ROWS             (0x01<<1)
#define  WHARF_COLUMNS			(0x01<<2)
#define  WHARF_NO_PUSH			(0x01<<3)
#define  WHARF_FULL_PUSH		(0x01<<4)
#define  WHARF_NO_BORDER		(0x01<<5)
#define  WHARF_WITHDRAW_STYLE		(0x01<<6)
#define  WHARF_NO_WITHDRAW		(0x01<<7)
#define  WHARF_FORCE_SIZE		(0x01<<8)
#define  WHARF_TEXTURE_TYPE		(0x01<<9)
#define  WHARF_MAX_COLORS		(0x01<<10)
#define  WHARF_BG_COLOR			(0x01<<11)
#define  WHARF_TEXTURE_COLOR		(0x01<<12)
#define  WHARF_PIXMAP			(0x01<<13)
#define  WHARF_ANIMATE_STEPS		(0x01<<14)
#define  WHARF_ANIMATE_STEPS_MAIN	(0x01<<15)
#define  WHARF_ANIMATE_DELAY		(0x01<<16)
#define  WHARF_ANIMATE_MAIN		(0x01<<17)
#define  WHARF_ANIMATE			(0x01<<18)
#define  WHARF_SOUND			(0x01<<19)
#define  WHARF_SHOW_LABEL       (0x01<<20)
#define  WHARF_LabelLocation 	 (0x01<<21)
#define  WHARF_FlipLabel         (0x01<<22)
#define  WHARF_FitContents  	 (0x01<<23)
#define  WHARF_ShapeToContents   (0x01<<24)
#define  WHARF_AlignContents	 (0x01<<25)
#define  WHARF_Bevel             (0x01<<26)
#define  WHARF_CompositionMethod (0x01<<27)
#define  WHARF_FolderOffset	 	 (0x01<<28)
#define  WHARF_OrthogonalFolderOffset  (0x01<<29)
#define  WHARF_StretchBackground   (0x01<<30)


typedef struct
{
    unsigned long set_flags;
    unsigned long flags;

    ASGeometry geometry;
    unsigned int rows, columns;
#define NO_WITHDRAW                         0
#define WITHDRAW_ON_ANY_BUTTON              1
#define WITHDRAW_ON_EDGE_BUTTON             2
#define WITHDRAW_ON_ANY_BUTTON_AND_SHOW     3
#define WITHDRAW_ON_EDGE_BUTTON_AND_SHOW    4
#define WITHDRAW_ON_ANY(c)   ((c)->withdraw_style == WITHDRAW_ON_ANY_BUTTON  || (c)->withdraw_style == WITHDRAW_ON_ANY_BUTTON_AND_SHOW )
#define WITHDRAW_ON_EDGE(c)  ((c)->withdraw_style == WITHDRAW_ON_EDGE_BUTTON || (c)->withdraw_style == WITHDRAW_ON_EDGE_BUTTON_AND_SHOW )
    unsigned int withdraw_style;
    ASGeometry force_size;
    unsigned int texture_type, max_colors;
    char *bg_color, *texture_color;
    char *pixmap;
    unsigned int animate_steps, animate_steps_main, animate_delay;
    char *sounds[WHEV_MAX_EVENTS];
    WharfButton *root_folder;

    unsigned int LabelLocation;

#define WHARF_DEFAULT_AlignContents 	ALIGN_CENTER
    ASFlagType   AlignContents;
	ASFlagType   Bevel;

    balloonConfig *balloon_conf;
    MyStyleDefinition *style_defs;

    struct FreeStorageElem *more_stuff;

#define WHARF_DEFAULT_CompositionMethod 	TEXTURE_TRANSPIXMAP_ALPHA
    int CompositionMethod ;

#define WHARF_DEFAULT_FolderOffset			-5
#define WHARF_DEFAULT_OrthogonalFolderOffset		0
	int FolderOffset, OrthogonalFolderOffset ;
}
WharfConfig;


unsigned long WharfSpecialFunc (struct ConfigDef * config);
WharfButton *CreateWharfButton ();
WharfConfig *CreateWharfConfig ();

void DestroyWharfButton (WharfButton **pbtn);
void DestroyWharfConfig (WharfConfig * config);

WharfConfig *ParseWharfOptions (const char *filename, char *myname);
void PrintWharfConfig(WharfConfig *config );

int WriteWharfOptions (const char *filename, char *myname,
		       WharfConfig * config, unsigned long flags);

/***************************************************************************/
/***************************************************************************/
/*                 Supported HINTS parsing definitions                     */
/***************************************************************************/

#define HINTS_ID_START              (WHEV_END_ID+1)
#define HINTS_ICCCM_ID              (HINTS_ID_START+HINTS_ICCCM)
#define HINTS_GroupLead_ID          (HINTS_ID_START+HINTS_GroupLead)
#define HINTS_Transient_ID          (HINTS_ID_START+HINTS_Transient)
#define HINTS_Motif_ID              (HINTS_ID_START+HINTS_Motif)
#define HINTS_Gnome_ID              (HINTS_ID_START+HINTS_Gnome)
#define HINTS_KDE_ID                (HINTS_ID_START+HINTS_KDE)
#define HINTS_ExtendedWM_ID         (HINTS_ID_START+HINTS_ExtendedWM)
#define HINTS_XResources_ID         (HINTS_ID_START+HINTS_XResources)
#define HINTS_ASDatabase_ID         (HINTS_ID_START+HINTS_ASDatabase)
#define HINTS_ID_END                (HINTS_ID_START+HINTS_Supported)

/****************************************************************************/
/*                                LOOK OPTIONS 	   		   	    */
/****************************************************************************/
#define DESK_CONFIG_BACK	0
#define DESK_CONFIG_LAYOUT	1

#define LOOK_ID_START               (HINTS_ID_END+1)

#define LOOK_DEPRECIATED_ID_START	(LOOK_ID_START)
#define LOOK_Font_ID			(LOOK_DEPRECIATED_ID_START)
#define LOOK_WindowFont_ID		(LOOK_DEPRECIATED_ID_START+1)
#define LOOK_MTitleForeColor_ID		(LOOK_DEPRECIATED_ID_START+2)
#define LOOK_MTitleBackColor_ID		(LOOK_DEPRECIATED_ID_START+3)
#define LOOK_MenuForeColor_ID		(LOOK_DEPRECIATED_ID_START+4)
#define LOOK_MenuBackColor_ID		(LOOK_DEPRECIATED_ID_START+5)
#define LOOK_MenuHiForeColor_ID		(LOOK_DEPRECIATED_ID_START+6)
#define LOOK_MenuHiBackColor_ID		(LOOK_DEPRECIATED_ID_START+7)
#define LOOK_MenuStippleColor_ID	(LOOK_DEPRECIATED_ID_START+8)
#define LOOK_StdForeColor_ID		(LOOK_DEPRECIATED_ID_START+9)
#define LOOK_StdBackColor_ID		(LOOK_DEPRECIATED_ID_START+10)
#define LOOK_StickyForeColor_ID 	(LOOK_DEPRECIATED_ID_START+11)
#define LOOK_StickyBackColor_ID 	(LOOK_DEPRECIATED_ID_START+12)
#define LOOK_HiForeColor_ID		(LOOK_DEPRECIATED_ID_START+13)
#define LOOK_HiBackColor_ID		(LOOK_DEPRECIATED_ID_START+14)
#define LOOK_IconFont_ID		(LOOK_DEPRECIATED_ID_START+15)
#define LOOK_TitleTextMode_ID		(LOOK_DEPRECIATED_ID_START+16)

#define LOOK_TextureTypes_ID		(LOOK_DEPRECIATED_ID_START+17)
#define LOOK_TextureMaxColors_ID	(LOOK_DEPRECIATED_ID_START+18)
#define LOOK_TitleTextureColor_ID	(LOOK_DEPRECIATED_ID_START+19)	/* title */
#define LOOK_UTitleTextureColor_ID	(LOOK_DEPRECIATED_ID_START+20)	/* unfoc tit */
#define LOOK_STitleTextureColor_ID	(LOOK_DEPRECIATED_ID_START+21)	/* stic tit */
#define LOOK_MTitleTextureColor_ID	(LOOK_DEPRECIATED_ID_START+22)	/* menu title */
#define LOOK_MenuTextureColor_ID	(LOOK_DEPRECIATED_ID_START+23)	/* menu items */
#define LOOK_MenuHiTextureColor_ID	(LOOK_DEPRECIATED_ID_START+24)	/* sel items */
#define LOOK_MenuPixmap_ID		(LOOK_DEPRECIATED_ID_START+25)	/* menu entry */
#define LOOK_MenuHiPixmap_ID		(LOOK_DEPRECIATED_ID_START+26)	/* hil m entr */
#define LOOK_MTitlePixmap_ID		(LOOK_DEPRECIATED_ID_START+27)	/* menu title */
#define LOOK_TitlePixmap_ID		(LOOK_DEPRECIATED_ID_START+28)	/* foc tit */
#define LOOK_UTitlePixmap_ID		(LOOK_DEPRECIATED_ID_START+29)	/* unfoc tit */
#define LOOK_STitlePixmap_ID		(LOOK_DEPRECIATED_ID_START+30)	/* stick tit */

#define LOOK_ButtonTextureType_ID	(LOOK_DEPRECIATED_ID_START+31)
#define LOOK_ButtonBgColor_ID		(LOOK_DEPRECIATED_ID_START+32)
#define LOOK_ButtonTextureColor_ID	(LOOK_DEPRECIATED_ID_START+33)
#define LOOK_ButtonMaxColors_ID		(LOOK_DEPRECIATED_ID_START+34)
#define LOOK_ButtonPixmap_ID		(LOOK_DEPRECIATED_ID_START+35)

#define LOOK_DEPRECIATED_ID_END		(LOOK_DEPRECIATED_ID_START+48)

/* non depreciated options : */
#define LOOK_SUPPORTED_ID_START		(LOOK_DEPRECIATED_ID_END)

#define LOOK_IconBox_ID							(LOOK_SUPPORTED_ID_START+1)							  
#define LOOK_MyStyle_ID							(LOOK_SUPPORTED_ID_START+3)						  
#define LOOK_MyBackground_ID					(LOOK_SUPPORTED_ID_START+4)
#define LOOK_DeskBack_ID						(LOOK_SUPPORTED_ID_START+5)				  
#define LOOK_asetrootDeskBack_ID				(LOOK_SUPPORTED_ID_START+6)
#define LOOK_MyFrame_ID							(LOOK_SUPPORTED_ID_START+7)						  
#define LOOK_DefaultFrame_ID					(LOOK_SUPPORTED_ID_START+8)				  
#define LOOK_DontDrawBackground_ID				(LOOK_SUPPORTED_ID_START+9)
#define LOOK_CustomCursor_ID					(LOOK_SUPPORTED_ID_START+10)
#define LOOK_CursorFore_ID						(LOOK_SUPPORTED_ID_START+11)						  
#define LOOK_CursorBack_ID						(LOOK_SUPPORTED_ID_START+12)
#define LOOK_Cursor_ID							(LOOK_SUPPORTED_ID_START+13)					  
#define LOOK_MenuPinOn_ID						(LOOK_SUPPORTED_ID_START+14)					  
#define LOOK_MArrowPixmap_ID					(LOOK_SUPPORTED_ID_START+15)					  
#define LOOK_TitlebarNoPush_ID					(LOOK_SUPPORTED_ID_START+16)	  
#define LOOK_TextureMenuItemsIndividually_ID	(LOOK_SUPPORTED_ID_START+17)
#define LOOK_MenuMiniPixmaps_ID					(LOOK_SUPPORTED_ID_START+18)
#define LOOK_TitleTextAlign_ID					(LOOK_SUPPORTED_ID_START+19)
#define LOOK_TitleButtonSpacingLeft_ID			(LOOK_SUPPORTED_ID_START+20)
#define LOOK_TitleButtonSpacingRight_ID			(LOOK_SUPPORTED_ID_START+21)
#define LOOK_TitleButtonSpacing_ID				(LOOK_SUPPORTED_ID_START+22)		  
#define LOOK_TitleButtonXOffsetLeft_ID			(LOOK_SUPPORTED_ID_START+23)
#define LOOK_TitleButtonXOffsetRight_ID			(LOOK_SUPPORTED_ID_START+24)
#define LOOK_TitleButtonXOffset_ID				(LOOK_SUPPORTED_ID_START+25)
#define LOOK_TitleButtonYOffsetLeft_ID			(LOOK_SUPPORTED_ID_START+26)
#define LOOK_TitleButtonYOffsetRight_ID			(LOOK_SUPPORTED_ID_START+27)
#define LOOK_TitleButtonYOffset_ID				(LOOK_SUPPORTED_ID_START+28)
#define LOOK_TitleButtonStyle_ID				(LOOK_SUPPORTED_ID_START+29)
#define LOOK_TitleButtonOrder_ID				(LOOK_SUPPORTED_ID_START+30)
#define LOOK_ResizeMoveGeometry_ID				(LOOK_SUPPORTED_ID_START+31)
#define LOOK_StartMenuSortMode_ID				(LOOK_SUPPORTED_ID_START+32)
#define LOOK_DrawMenuBorders_ID					(LOOK_SUPPORTED_ID_START+33)
#define LOOK_ButtonSize_ID						(LOOK_SUPPORTED_ID_START+34)			  
#define LOOK_ButtonIconSpacing_ID			 	(LOOK_SUPPORTED_ID_START+35)			  
#define LOOK_ButtonBevel_ID					 	(LOOK_SUPPORTED_ID_START+36)			  
#define LOOK_ButtonAlign_ID					 	(LOOK_SUPPORTED_ID_START+37)			  
#define LOOK_SeparateButtonTitle_ID				(LOOK_SUPPORTED_ID_START+38)
#define LOOK_RubberBand_ID						(LOOK_SUPPORTED_ID_START+39)					  
#define LOOK_WindowStyle_ID_START				(LOOK_SUPPORTED_ID_START+40)

#define LOOK_IconsGrowVertically_ID                             (LOOK_SUPPORTED_ID_START+41)

#define LOOK_DefaultStyle_ID					LOOK_WindowStyle_ID_START						  
#define LOOK_FWindowStyle_ID					(LOOK_WindowStyle_ID_START+1)						  
#define LOOK_UWindowStyle_ID					(LOOK_WindowStyle_ID_START+2)						  
#define LOOK_SWindowStyle_ID					(LOOK_WindowStyle_ID_START+3)						  
#define LOOK_WindowStyle_ID_END					(LOOK_WindowStyle_ID_START+3)
#define LOOK_MenuStyle_ID_START					(LOOK_WindowStyle_ID_END+1)
#define LOOK_MenuItemStyle_ID					(LOOK_MenuStyle_ID_START)					  
#define LOOK_MenuTitleStyle_ID					(LOOK_MenuStyle_ID_START+1)					  
#define LOOK_MenuHiliteStyle_ID					(LOOK_MenuStyle_ID_START+2)					  
#define LOOK_MenuStippleStyle_ID				(LOOK_MenuStyle_ID_START+3)					  
#define LOOK_MenuSubItemStyle_ID				(LOOK_MenuStyle_ID_START+4)					  
#define LOOK_MenuHiTitleStyle_ID				(LOOK_MenuStyle_ID_START+6)
#define LOOK_MenuStyle_ID_END					(LOOK_MenuStyle_ID_START+6)		  
#define LOOK_MenuItemCompositionMethod_ID		(LOOK_MenuStyle_ID_END+1)	  
#define LOOK_MenuHiliteCompositionMethod_ID		(LOOK_MenuStyle_ID_END+2)
#define LOOK_MenuStippleCompositionMethod_ID	(LOOK_MenuStyle_ID_END+3)
#define LOOK_ShadeAnimationSteps_ID				(LOOK_MenuStyle_ID_END+4)	  
#define LOOK_TitleButtonBalloonBorderHilite_ID	(LOOK_MenuStyle_ID_END+5)
#define LOOK_TitleButtonBalloonXOffset_ID		(LOOK_MenuStyle_ID_END+6)
#define LOOK_TitleButtonBalloonYOffset_ID		(LOOK_MenuStyle_ID_END+7)
#define LOOK_TitleButtonBalloonDelay_ID			(LOOK_MenuStyle_ID_END+8)		  
#define LOOK_TitleButtonBalloonCloseDelay_ID	(LOOK_MenuStyle_ID_END+9)
#define LOOK_TitleButtonBalloonStyle_ID			(LOOK_MenuStyle_ID_END+10)
#define LOOK_TitleButtonBalloons_ID				(LOOK_MenuStyle_ID_END+11)
#define LOOK_TitleButton_ID						(LOOK_MenuStyle_ID_END+12)
#define LOOK_KillBackgroundThreshold_ID			(LOOK_MenuStyle_ID_END+13)
#define LOOK_DontAnimateBackground_ID			(LOOK_MenuStyle_ID_END+14)
#define LOOK_CoverAnimationSteps_ID				(LOOK_MenuStyle_ID_END+15)				  
#define LOOK_CoverAnimationType_ID				(LOOK_MenuStyle_ID_END+16)
#define LOOK_SupportedHints_ID  				(LOOK_MenuStyle_ID_END+17)							  
#define LOOK_MinipixmapSize_ID					(LOOK_MenuStyle_ID_END+18)

#define LOOK_SUPPORTED_ID_END		(LOOK_MenuStyle_ID_END+32)
#define LOOK_ID_END			(LOOK_SUPPORTED_ID_END)

typedef struct DesktopConfig
{
  int desk;
  char *back_name;
  char *layout_name;
  MyBackgroundConfig *back;

  struct DesktopConfig *next;
}DesktopConfig;

void DestroyDesktopConfig (DesktopConfig ** head);
DesktopConfig *ParseDesktopOptions (DesktopConfig **plist, struct ConfigItem * item, int id);

typedef struct LookConfig
{

/* this are values for different LOOK flags */
  unsigned long flags;
  unsigned long set_flags;

  ASBox *icon_boxes;
  short unsigned int icon_boxes_num;

  char *menu_arrow;		/* menu arrow */
  char *menu_pin_on;		/* menu pin */
  char *menu_pin_off;

  char *text_gradient[2];	/* title text */
  unsigned short int title_text_align;
  short int title_button_spacing;
  unsigned short int title_button_style;
  short int title_button_x_offset;
  short int title_button_y_offset;
  ASButton *normal_buttons[MAX_BUTTONS];       /* titlebar buttons for windows */
  ASButton *icon_buttons[MAX_BUTTONS];         /* titlebar buttons for iconifyed windows */
  ASGeometry resize_move_geometry;
  unsigned short int start_menu_sort_mode;
  unsigned short int draw_menu_borders;
  ASBox button_size;

  unsigned int rubber_band;
  unsigned int shade_animation_steps;

  char *window_styles[BACK_STYLES];
  char *menu_styles[MENU_BACK_STYLES];

  balloonConfig *balloon_conf;
  MyStyleDefinition *style_defs;
  MyFrameDefinition *frame_defs;
  MyBackgroundConfig *back_defs;
  DesktopConfig      *desk_configs;

  char *menu_frame ;

  struct ASSupportedHints *supported_hints ;

  struct FreeStorageElem *more_stuff;

}
LookConfig;

LookConfig *CreateLookConfig ();
void DestroyLookConfig (LookConfig * config);

ASFlagType ParseBevelOptions( struct FreeStorageElem * options );
ASFlagType ParseAlignOptions( struct FreeStorageElem * options );
void bevel_parse(char *text, FILE * fd, char **myname, int *pbevel);
void align_parse(char *text, FILE * fd, char **myname, int *palign);

LookConfig *ParseLookOptions (const char *filename, char *myname);
struct MyLook *LookConfig2MyLook ( struct LookConfig *config, struct MyLook *look,
			  			   	       struct ASImageManager *imman,
								   struct ASFontManager *fontman,
                                   Bool free_resources, Bool do_init,
								   unsigned long what_flags	);
/***************************************************************************/
/***************************************************************************/
/*                        Directory Tree definitions                       */
/***************************************************************************/
#if 0
#include "dirtree.h"

/***************************************************************************/
/***************************************************************************/
/*                        Menu data definitions                            */
/***************************************************************************/
MenuData *dirtree2menu_data (struct ASHashTable **list, dirtree_t * tree, char *buf);
MenuData *FreeStorage2MenuData( struct FreeStorageElem *storage, ConfigItem *item, struct ASHashTable *list );
struct FreeStorageElem **MenuData2FreeStorage( struct SyntaxDef *syntax, struct FreeStorageElem **tail, MenuData *md );
void      dir2menu_data (char *name, struct ASHashTable** list);
void      file2menu_data (char *name, struct ASHashTable** list);

void      load_standard_menu( struct ASHashTable** list );
void      load_fixed_menu( struct ASHashTable** list  );
#endif
/***************************************************************************/
/***************************************************************************/
/*                  Complex Function data definitions                      */
/***************************************************************************/
ComplexFunction *FreeStorage2ComplexFunction( struct FreeStorageElem *storage, struct ConfigItem *item, struct ASHashTable *list );
struct FreeStorageElem **ComplexFunction2FreeStorage( struct SyntaxDef *syntax, struct FreeStorageElem **tail, ComplexFunction *cf );
/***************************************************************************/
/***************************************************************************/
/*                        FEEL parsing definitions                         */
/***************************************************************************/

#define INCLUDE_ID_START              (LOOK_ID_END+1)
#define INCLUDE_include_ID            (INCLUDE_ID_START)	
#define INCLUDE_keepname_ID           (INCLUDE_ID_START+1)	
#define INCLUDE_extension_ID          (INCLUDE_ID_START+2) 
#define INCLUDE_miniextension_ID      (INCLUDE_ID_START+3)
#define INCLUDE_minipixmap_ID         (INCLUDE_ID_START+4)
#define INCLUDE_command_ID            (INCLUDE_ID_START+5)	
#define INCLUDE_order_ID       		  (INCLUDE_ID_START+6)
#define INCLUDE_RecentSubmenuItems_ID (INCLUDE_ID_START+7)
#define INCLUDE_name_ID          	  (INCLUDE_ID_START+8)	
#define INCLUDE_ID_END                (INCLUDE_ID_START+9)     

#define FEEL_ID_START                 (INCLUDE_ID_END+1)

#define FEEL_ClickToFocus_ID          (FEEL_ID_START)
#define FEEL_SloppyFocus_ID           (FEEL_ID_START+1)
#define FEEL_AutoFocus_ID             (FEEL_ID_START+2)

#define FEEL_DecorateTransients_ID    (FEEL_ID_START+3)

#define FEEL_DontMoveOff_ID           (FEEL_ID_START+4)
#define FEEL_NoPPosition_ID           (FEEL_ID_START+5)
#define FEEL_StubbornPlacement_ID     (FEEL_ID_START+6)
    
#define FEEL_MenusHigh_ID             (FEEL_ID_START+9)
#define FEEL_CenterOnCirculate_ID     (FEEL_ID_START+10)

#define FEEL_SuppressIcons_ID         (FEEL_ID_START+11)
#define FEEL_IconTitle_ID             (FEEL_ID_START+12)
#define FEEL_KeepIconWindows_ID       (FEEL_ID_START+13)
#define FEEL_StickyIcons_ID           (FEEL_ID_START+14)
#define FEEL_StubbornIcons_ID         (FEEL_ID_START+15)
#define FEEL_StubbornIconPlacement_ID (FEEL_ID_START+16)
#define FEEL_CirculateSkipIcons_ID    (FEEL_ID_START+17)

#define FEEL_BackingStore_ID          (FEEL_ID_START+18)
#define FEEL_AppsBackingStore_ID      (FEEL_ID_START+19)
#define FEEL_SaveUnders_ID            (FEEL_ID_START+20)

#define FEEL_PagingDefault_ID         (FEEL_ID_START+21)
#define FEEL_AutoTabThroughDesks_ID   (FEEL_ID_START+22)

#define FEEL_ClickTime_ID             (FEEL_ID_START+23)
#define FEEL_OpaqueMove_ID            (FEEL_ID_START+24)
#define FEEL_OpaqueResize_ID          (FEEL_ID_START+25)
#define FEEL_AutoRaise_ID             (FEEL_ID_START+26)
#define FEEL_AutoReverse_ID           (FEEL_ID_START+27)
#define FEEL_DeskAnimationType_ID     (FEEL_ID_START+28)
#define FEEL_DeskAnimationSteps_ID    (FEEL_ID_START+29)
#define FEEL_XorValue_ID              (FEEL_ID_START+30)
#define FEEL_Xzap_ID                  (FEEL_ID_START+31)
#define FEEL_Yzap_ID                  (FEEL_ID_START+32)
#define FEEL_Cursor_ID                (FEEL_ID_START+33)
#define FEEL_CustomCursor_ID          (FEEL_ID_START+34)
#define FEEL_ClickToRaise_ID          (FEEL_ID_START+35)
#define FEEL_EdgeScroll_ID            (FEEL_ID_START+36)
#define FEEL_EdgeResistance_ID        (FEEL_ID_START+37)
#define FEEL_Popup_ID                 F_POPUP/*(FEEL_ID_START+38)*/
#define FEEL_Function_ID              F_FUNCTION/*(FEEL_ID_START+39)*/
#define FEEL_Mouse_ID                 (FEEL_ID_START+40)
#define FEEL_Key_ID                   (FEEL_ID_START+41)
#define FEEL_ShadeAnimationSteps_ID   (FEEL_ID_START+42)
#define FEEL_CoverAnimationSteps_ID   (FEEL_ID_START+43)
#define FEEL_CoverAnimationType_ID	  (FEEL_ID_START+44)

#define FEEL_FollowTitleChanges_ID	 	(FEEL_ID_START+45)
#define FEEL_PersistentMenus_ID		   	(FEEL_ID_START+46)
#define FEEL_NoSnapKey_ID			   	(FEEL_ID_START+47)
#define FEEL_EdgeAttractionScreen_ID	(FEEL_ID_START+48)
#define FEEL_EdgeAttractionWindow_ID	(FEEL_ID_START+49)
#define FEEL_DontRestoreFocus_ID	   	(FEEL_ID_START+50)
#define FEEL_WindowBox_ID			   	(FEEL_ID_START+51)
#define FEEL_DefaultWindowBox_ID	   	(FEEL_ID_START+52)
#define FEEL_RecentSubmenuItems_ID	 	(FEEL_ID_START+53)
#define FEEL_WinListSortOrder_ID	   	(FEEL_ID_START+54)
#define FEEL_WinListHideIcons_ID	   	(FEEL_ID_START+55)
#define FEEL_AnimateDeskChange_ID		(FEEL_ID_START+56)	   

/* obsolete stuff : */
#define FEEL_MWMFunctionHints_ID      	(FEEL_ID_START+45)
#define FEEL_MWMDecorHints_ID         	(FEEL_ID_START+46)
#define FEEL_MWMHintOverride_ID       	(FEEL_ID_START+47)

#define FEEL_PLACEMENT_START_ID        	(FEEL_ID_START+48)
#define FEEL_SmartPlacement_ID        	(FEEL_PLACEMENT_START_ID+0)
#define FEEL_RandomPlacement_ID       	(FEEL_PLACEMENT_START_ID+1)
#define FEEL_Tile_ID				  	(FEEL_PLACEMENT_START_ID+2)	
#define FEEL_Cascade_ID				  	(FEEL_PLACEMENT_START_ID+3)	
#define FEEL_Manual_ID				  	(FEEL_PLACEMENT_START_ID+4)

#define FEEL_ID_END                   	(FEEL_PLACEMENT_START_ID+10)

/************************************
 * WindowBox configuration may look something like this :
 * WindowBox   "some_name"
 * 		Area   WxH+X+Y
 * 		Virtual
 * 		MinWidth	width
 * 		MinHeight	height
 * 		MaxWidth	width
 * 		MaxHeight	height
 * 		FirstTry	SmartPlacement|RandomPlacement|Tile
 * 		ThenTry 	RandomPlacement|Cascade|Manual
 * 		VerticalPriority
 * 		ReverseOrder
 *      Desk        desk
 *      MinLayer    min_layer
 *      MaxLayer    max_layer
 * ~WindowBox
 */

#define WINDOWBOX_ID_START	 			(FEEL_ID_END+1)
#define WINDOWBOX_START_ID				(WINDOWBOX_ID_START)
#define WINDOWBOX_Area_ID   	  		(WINDOWBOX_ID_START+1)
#define WINDOWBOX_Virtual_ID      		(WINDOWBOX_ID_START+2)
#define WINDOWBOX_MinWidth_ID	  		(WINDOWBOX_ID_START+3)
#define WINDOWBOX_MinHeight_ID	  		(WINDOWBOX_ID_START+4)
#define WINDOWBOX_MaxWidth_ID	  		(WINDOWBOX_ID_START+5)
#define WINDOWBOX_MaxHeight_ID	  		(WINDOWBOX_ID_START+6)
#define WINDOWBOX_FirstTry_ID	  		(WINDOWBOX_ID_START+7)
#define WINDOWBOX_ThenTry_ID 	  		(WINDOWBOX_ID_START+8)
#define WINDOWBOX_Desk_ID               (WINDOWBOX_ID_START+9)
#define WINDOWBOX_MinLayer_ID           (WINDOWBOX_ID_START+10)
#define WINDOWBOX_MaxLayer_ID           (WINDOWBOX_ID_START+11)
#define WINDOWBOX_VerticalPriority_ID   (WINDOWBOX_ID_START+12)
#define WINDOWBOX_ReverseOrder_ID       (WINDOWBOX_ID_START+13)
#define WINDOWBOX_ReverseOrderHorizontal_ID	(WINDOWBOX_ID_START+14)
#define WINDOWBOX_ReverseOrderVertical_ID  	(WINDOWBOX_ID_START+15)
#define WINDOWBOX_XSpacing_ID			(WINDOWBOX_ID_START+16)
#define WINDOWBOX_YSpacing_ID			(WINDOWBOX_ID_START+17)
#define WINDOWBOX_DONE_ID               (WINDOWBOX_ID_START+18)

#define WINDOWBOX_ID_END            	(WINDOWBOX_ID_START+19)

/* we don't really need FeelConfig since feel does not cary
   any information that requires char2bin conversion and consecutive
   memory allocation/deallocation. Hence we use ASFeel as FeelConfig.
 */
struct ASFeel;

typedef struct FeelConfig
{
	struct ASFeel *feel ;
    char         **menu_locations ;
    int            menu_locs_num ;

	struct FreeStorageElem *more_stuff ;

}FeelConfig;

void complex_function_parse (char *tline, FILE * fd, char *list, int *count);

FeelConfig *CreateFeelConfig ();
void DestroyFeelConfig (FeelConfig * config);
FeelConfig *ParseFeelOptions (const char *filename, char *myname);
void        LoadFeelMenus (FeelConfig *config);
struct ASWindowBox *ProcessWindowBoxOptions (struct FreeStorageElem * options);
void windowbox_parse (char *tline, FILE * fd, char **list, int *count);

int WriteFeelOptions (const char *filename, char *myname,  FeelConfig * config, unsigned long flags);

/* AutoExec config is basically a very limited subset of the feel config
 * therefore it does not deserve its own section : */

typedef struct AutoExecConfig
{
    ComplexFunction *init ;
    ComplexFunction *restart ;

    struct FreeStorageElem *more_stuff ;

}AutoExecConfig;

typedef struct ThemeConfig
{
    ComplexFunction *install ;
    ComplexFunction *apply ;

    struct FreeStorageElem *more_stuff ;

}ThemeConfig;

#define THEME_INSTALL_FUNC_NAME "InstallTheme"
#define THEME_APPLY_FUNC_NAME 	"ApplyTheme"

AutoExecConfig *CreateAutoExecConfig ();
void    DestroyAutoExecConfig (AutoExecConfig * config);
AutoExecConfig *ParseAutoExecOptions (const char *filename, char *myname);
int WriteAutoExecOptions (const char *filename, char *myname,  AutoExecConfig * config, unsigned long flags);

void DestroyThemeConfig (ThemeConfig * config);
ThemeConfig *ParseThemeFile (const char *filename, char *myname);
/***************************************************************************/
#define COLOR_ID_START	 				(WINDOWBOX_ID_END+1)
#define COLOR_Base_ID					(COLOR_ID_START+ASMC_Base 					 )
#define COLOR_Inactive1_ID				(COLOR_ID_START+ASMC_Inactive1 				 )
#define COLOR_Inactive2_ID				(COLOR_ID_START+ASMC_Inactive2 				 )
#define COLOR_Active_ID  				(COLOR_ID_START+ASMC_Active 				 )
#define COLOR_InactiveText1_ID			(COLOR_ID_START+ASMC_InactiveText1 		 	 )
#define COLOR_InactiveText2_ID			(COLOR_ID_START+ASMC_InactiveText2 			 )

#define COLOR_ActiveText_ID				(COLOR_ID_START+ASMC_ActiveText 			 )
#define COLOR_HighInactive_ID			(COLOR_ID_START+ASMC_HighInactive 			 )
#define COLOR_HighActive_ID				(COLOR_ID_START+ASMC_HighActive 			 )
#define COLOR_HighInactiveBack_ID		(COLOR_ID_START+ASMC_HighInactiveBack 		 )
#define COLOR_HighActiveBack_ID			(COLOR_ID_START+ASMC_HighActiveBack 	  )
#define COLOR_HighInactiveText_ID		(COLOR_ID_START+ASMC_HighInactiveText 		 )
#define COLOR_HighActiveText_ID			(COLOR_ID_START+ASMC_HighActiveText			 )
#define COLOR_DisabledText_ID			(COLOR_ID_START+ASMC_DisabledText			 )

#define COLOR_BaseDark_ID				(COLOR_ID_START+ASMC_BaseDark				 )
#define COLOR_BaseLight_ID				(COLOR_ID_START+ASMC_BaseLight				 )
#define COLOR_Inactive1Dark_ID			(COLOR_ID_START+ASMC_Inactive1Dark			 )
#define COLOR_Inactive1Light_ID			(COLOR_ID_START+ASMC_Inactive1Light			 )
#define COLOR_Inactive2Dark_ID			(COLOR_ID_START+ASMC_Inactive2Dark			 )
#define COLOR_Inactive2Light_ID			(COLOR_ID_START+ASMC_Inactive2Light			 )
#define COLOR_ActiveDark_ID				(COLOR_ID_START+ASMC_ActiveDark				 )
#define COLOR_ActiveLight_ID			(COLOR_ID_START+ASMC_ActiveLight			 )
#define COLOR_HighInactiveDark_ID		(COLOR_ID_START+ASMC_HighInactiveDark		 )
#define COLOR_HighInactiveLight_ID		(COLOR_ID_START+ASMC_HighInactiveLight		 )
#define COLOR_HighActiveDark_ID			(COLOR_ID_START+ASMC_HighActiveDark			 )
#define COLOR_HighActiveLight_ID		(COLOR_ID_START+ASMC_HighActiveLight		 )
#define COLOR_HighInactiveBackDark_ID	(COLOR_ID_START+ASMC_HighInactiveBackDark	)
#define COLOR_HighInactiveBackLight_ID 	(COLOR_ID_START+ASMC_HighInactiveBackLight)
#define COLOR_HighActiveBackDark_ID		(COLOR_ID_START+ASMC_HighActiveBackDark		 )
#define COLOR_HighActiveBackLight_ID	(COLOR_ID_START+ASMC_HighActiveBackLight  )
#define COLOR_Cursor_ID					(COLOR_ID_START+ASMC_Cursor  )
#define COLOR_Angle_ID					(COLOR_ID_START+ASMC_MainColors)
#define COLOR_ID_END            		(COLOR_ID_START+ASMC_MainColors+1)

typedef enum
{
#define COLOR_SET_FLAG(name)  COLOR_##name = (0x01<<(ASMC_##name))

	COLOR_SET_FLAG(Base),
	COLOR_SET_FLAG(Inactive1),
	COLOR_SET_FLAG(Inactive2),
	COLOR_SET_FLAG(Active),
	COLOR_SET_FLAG(InactiveText1),
	COLOR_SET_FLAG(InactiveText2),
	COLOR_SET_FLAG(ActiveText),
	COLOR_SET_FLAG(HighInactive),
	COLOR_SET_FLAG(HighActive),
	COLOR_SET_FLAG(HighInactiveBack),
	COLOR_SET_FLAG(HighActiveBack),
	COLOR_SET_FLAG(HighInactiveText),
	COLOR_SET_FLAG(HighActiveText),
	COLOR_SET_FLAG(DisabledText),
	COLOR_SET_FLAG(BaseDark),
	COLOR_SET_FLAG(BaseLight),
	COLOR_SET_FLAG(Inactive1Dark),
	COLOR_SET_FLAG(Inactive1Light),
	COLOR_SET_FLAG(Inactive2Dark),
	COLOR_SET_FLAG(Inactive2Light),
	COLOR_SET_FLAG(ActiveDark),
	COLOR_SET_FLAG(ActiveLight),
	COLOR_SET_FLAG(HighInactiveDark),
	COLOR_SET_FLAG(HighInactiveLight),
	COLOR_SET_FLAG(HighActiveDark),
	COLOR_SET_FLAG(HighActiveLight),
	COLOR_SET_FLAG(HighInactiveBackDark),
	COLOR_SET_FLAG(HighInactiveBackLight),
	COLOR_SET_FLAG(HighActiveBackDark),
	COLOR_SET_FLAG(HighActiveBackLight),
	COLOR_SET_FLAG(Cursor),
	COLOR_Angle = (0x01<<ASMC_MainColors)

}ColorConfigSetFlags;


typedef struct ColorConfig
{
	ASFlagType set_main_colors ;
	ARGB32 main_colors[ASMC_MainColors] ;
	int angle ;

    struct FreeStorageElem *more_stuff ;

}ColorConfig;

void DestroyColorConfig (ColorConfig * config);

ColorConfig *ParseColorOptions (const char *filename, char *myname);
int WriteColorOptions (const char *filename, char *myname, ColorConfig * config, unsigned long flags);

ColorConfig *ASColorScheme2ColorConfig( ASColorScheme *cs );
ASColorScheme *ColorConfig2ASColorScheme( ColorConfig *config );

void LoadColorScheme();                        /* high level easy to use function */
Bool translate_gtkrc_template_file( const char *template_fname, const char *output_fname );
Bool UpdateGtkRC(ASEnvironment *e);
Bool UpdateKCSRC();



/***************************************************************************/
/*                        WinTabs config parsing definitions               */
/***************************************************************************/
/* New winlist config :
 *
 *  *WinTabsGeometry         WxH+X+Y
 *  *WinTabsMinTabMaxRows    rows
 *  *WinTabsMinTabMaxColumns cols
 *  *WinTabsMinTabWidth      width
 *  *WinTabsMaxTabWidth      width
 *  *WinTabsUnfocusedStyle   "style"
 *  *WinTabsFocusedStyle     "style"
 *  *WinTabsStickyStyle      "style"
 *  *WinTabsUseSkipList      width
 *  *WinTabsPattern          0|1|2|3 <pattern>  # 0 - Name, 1 - icon, 2 - res_name, 3 - res_class
 *  *WinTabsExcludePattern   0|1|2|3 <pattern>  # 0 - Name, 1 - icon, 2 - res_name, 3 - res_class
 *  *WinTabsAlign            Left,Right,Top,Bottom
 *  *WinTabsBevel            None,Left,Right,Top, Bottom, NoOutline
 *  *WinTabsFBevel           None,Left,Right,Top, Bottom, NoOutline
 *  *WinTabsUBevel           None,Left,Right,Top, Bottom, NoOutline
 *  *WinTabsSBevel           None,Left,Right,Top, Bottom, NoOutline
*/
#define WINTABS_ID_START        		(COLOR_ID_END+1)
#define WINTABS_Geometry_ID				(WINTABS_ID_START+1)
#define WINTABS_MaxRows_ID				(WINTABS_ID_START+2)
#define WINTABS_MaxColumns_ID			(WINTABS_ID_START+3)
#define WINTABS_MaxTabWidth_ID          (WINTABS_ID_START+4)
#define WINTABS_MinTabWidth_ID          (WINTABS_ID_START+5)
#define WINTABS_Pattern_ID				(WINTABS_ID_START+6)
#define WINTABS_UseSkipList_ID			(WINTABS_ID_START+7)
#define WINTABS_Align_ID                (WINTABS_ID_START+8)
#define WINTABS_Bevel_ID                (WINTABS_ID_START+9)
#define WINTABS_FBevel_ID               (WINTABS_ID_START+10)
#define WINTABS_UBevel_ID               (WINTABS_ID_START+11)
#define WINTABS_SBevel_ID               (WINTABS_ID_START+12)
#define WINTABS_UnfocusedStyle_ID       (WINTABS_ID_START+13)
#define WINTABS_FocusedStyle_ID         (WINTABS_ID_START+14)
#define WINTABS_StickyStyle_ID          (WINTABS_ID_START+15)
#define WINTABS_CM_ID                   (WINTABS_ID_START+16)
#define WINTABS_FCM_ID                  (WINTABS_ID_START+17)
#define WINTABS_UCM_ID                  (WINTABS_ID_START+18)
#define WINTABS_SCM_ID                  (WINTABS_ID_START+19)
#define WINTABS_Spacing_ID              (WINTABS_ID_START+20)
#define WINTABS_HSpacing_ID             (WINTABS_ID_START+21)
#define WINTABS_VSpacing_ID             (WINTABS_ID_START+22)
#define WINTABS_ExcludePattern_ID		(WINTABS_ID_START+23)
#define WINTABS_AllDesks_ID				(WINTABS_ID_START+24)
#define WINTABS_Title_ID				(WINTABS_ID_START+25)
#define WINTABS_IconTitle_ID	 		(WINTABS_ID_START+26)
#define WINTABS_SkipTransients_ID		(WINTABS_ID_START+27)


#define WINTABS_BALLOONS_ID             (WINTABS_ID_START+31)
#define WINTABS_ID_END                  (WINTABS_ID_START+32)

/* config data structure */

typedef struct WinTabsConfig
{
#define WINTABS_UseSkipList		(0x01<<0)
#define WINTABS_HideWhenEmpty  	(0x01<<1)
#define WINTABS_AllDesks	  	(0x01<<2)
#define WINTABS_Geometry		(0x01<<3)
#define WINTABS_MaxRows			(0x01<<5)
#define WINTABS_MaxColumns		(0x01<<6)
#define WINTABS_MaxTabWidth     (0x01<<7)
#define WINTABS_MinTabWidth     (0x01<<8)
#define WINTABS_Align           (0x01<<10)
#define WINTABS_FBevel          (0x01<<11)
#define WINTABS_UBevel          (0x01<<12)
#define WINTABS_SBevel          (0x01<<13)
#define WINTABS_Bevel           (WINTABS_FBevel|WINTABS_UBevel|WINTABS_SBevel)
#define WINTABS_FCM             (0x01<<15)
#define WINTABS_UCM             (0x01<<16)
#define WINTABS_SCM             (0x01<<17)
#define WINTABS_CM              (WINTABS_FCM|WINTABS_UCM|WINTABS_SCM)

#define WINTABS_H_SPACING       (0x01<<18)
#define WINTABS_V_SPACING       (0x01<<19)
#define WINTABS_PatternType	    (0x01<<20)
#define WINTABS_ExcludePatternType	    (0x01<<21)
#define WINTABS_SkipTransients	(0x01<<22)


#define ASWT_UseSkipList		WINTABS_UseSkipList
#define ASWT_HideWhenEmpty		WINTABS_HideWhenEmpty

	ASFlagType	flags ;
	ASFlagType	set_flags ;
    ASGeometry  geometry ;
#define MAX_WINTABS_WINDOW_COUNT    64
	unsigned int max_rows, max_columns ;
    unsigned int min_tab_width, max_tab_width ;

	char *unfocused_style ;
	char *focused_style ;
	char *sticky_style ;

	ASNameTypes     pattern_type ; /* 0, 1, 2, 3 */
	char 		   *pattern ;
	ASNameTypes     exclude_pattern_type ; /* 0, 1, 2, 3 */
	char 		   *exclude_pattern ;
    ASFlagType      name_aligment ;
    ASFlagType      fbevel, ubevel, sbevel ;
    int             ucm, fcm, scm;             /* composition methods */
    unsigned int    h_spacing, v_spacing ;
	char 		   *title, *icon_title ;

    balloonConfig *balloon_conf;
    MyStyleDefinition *style_defs;

    struct FreeStorageElem *more_stuff;

    /* calculated based on geometry : */
    int anchor_x, anchor_y ;
	int gravity ;


}WinTabsConfig;

WinTabsConfig *CreateWinTabsConfig ();
void DestroyWinTabsConfig (WinTabsConfig * config);
void PrintWinTabsConfig (WinTabsConfig * config);
int WriteWinTabsOptions (const char *filename, char *myname, WinTabsConfig * config, unsigned long flags);
WinTabsConfig *ParseWinTabsOptions (const char *filename, char *myname);
/***************************************************************************/

/***************************************************************************/
/*                        Animate pasring definitions                 */
/***************************************************************************/


#define ANIMATE_ID_START       		(WINTABS_ID_END+1)
#define ANIMATE_COLOR_ID			ANIMATE_ID_START
#define ANIMATE_DELAY_ID       		(ANIMATE_ID_START+1)
#define ANIMATE_ITERATIONS_ID		(ANIMATE_ID_START+2)
#define ANIMATE_TWIST_ID    		(ANIMATE_ID_START+3)
#define ANIMATE_WIDTH_ID	        (ANIMATE_ID_START+4)
#define ANIMATE_RESIZE_ID	        (ANIMATE_ID_START+5)

typedef enum {
	ART_Twist = 0,
	ART_Flip,
	ART_Turn,
	ART_Zoom,
	ART_Zoom3D,
	ART_Random,
	ART_None
}AnimateResizeType;

#define ANIMATE_RESIZE_ID_START		(ANIMATE_RESIZE_ID+1)
#define ANIMATE_ResizeTwist_ID		(ANIMATE_RESIZE_ID_START+ART_Twist)
#define ANIMATE_ResizeFlip_ID		(ANIMATE_RESIZE_ID_START+ART_Flip)
#define ANIMATE_ResizeTurn_ID		(ANIMATE_RESIZE_ID_START+ART_Turn)
#define ANIMATE_ResizeZoom_ID		(ANIMATE_RESIZE_ID_START+ART_Zoom)
#define ANIMATE_ResizeZoom3D_ID		(ANIMATE_RESIZE_ID_START+ART_Zoom3D)
#define ANIMATE_ResizeRandom_ID		(ANIMATE_RESIZE_ID_START+ART_Random)
#define ANIMATE_RESIZE_ID_END		(ANIMATE_RESIZE_ID_START+ART_None)

#define ANIMATE_ID_END        	ANIMATE_ID_START+20

/* config data structure */

typedef struct
{
#define ANIMATE_SET_DELAY		(0x01<<0)	
#define ANIMATE_SET_ITERATIONS	(0x01<<1)	  
#define ANIMATE_SET_TWIST		(0x01<<2)	  
#define ANIMATE_SET_WIDTH		(0x01<<3)	  
#define ANIMATE_SET_RESIZE		(0x01<<4)	  
	ASFlagType set_flags ;
	char *color;

#define ANIMATE_DEFAULT_DELAY		10         /* in milliseconds */
#define ANIMATE_DEFAULT_ITERATIONS 	12

	int delay;
	int iterations;
	int twist;
	int width;
	AnimateResizeType resize;

	struct FreeStorageElem *more_stuff;

}AnimateConfig;

AnimateConfig *CreateAnimateConfig ();
AnimateConfig *ParseAnimateOptions (const char *filename, char *myname);
int WriteAnimateOptions (const char *filename, char *myname,
			 AnimateConfig * config, unsigned long flags);
void DestroyAnimateConfig (AnimateConfig * config);

/**************************************************************************/
/***************************************************************************/
/*                        Clean pasring definitions                 */
/***************************************************************************/


#define CLEAN_ID_START       		(ANIMATE_ID_END+1)
#define CLEAN_Clean_ID				CLEAN_ID_START
#define CLEAN_ID_END        		CLEAN_ID_START+20

/* config data structure */
typedef struct CleanLineConfig
{
  int seconds;
  FunctionData *action;
  struct CleanLineConfig *next ;
}CleanLineConfig;


typedef struct
{
	struct CleanLineConfig *lines ;
	struct FreeStorageElem *more_stuff;

}CleanConfig;

CleanConfig *CreateCleanConfig ();
CleanConfig *ParseCleanOptions (const char *filename, char *myname);
int WriteCleanOptions (const char *filename, char *myname,
			 CleanConfig * config, unsigned long flags);
void DestroyCleanConfig (CleanConfig * config);

/**************************************************************************/
/***************************************************************************/
/*                        Audio pasring definitions                 */
/***************************************************************************/
/***************************************************************************/
/*                        Possible AfterStep communication events :        */
/***************************************************************************/
#define	EVENT_WindowAdded			0
#define	EVENT_WindowNames    		1
#define	EVENT_WindowDestroyed		2
#define	EVENT_WindowActivated		3
#define	EVENT_WindowRaised			4
#define	EVENT_WindowIconified		5
#define	EVENT_WindowDeiconified		6
#define	EVENT_WindowShaded			7
#define	EVENT_WindowUnshaded		8
#define	EVENT_WindowStuck			9 
#define	EVENT_WindowUnstuck			10
#define	EVENT_WindowMaximized		11
#define	EVENT_WindowRestored		12
#define	EVENT_BackgroundChanged		13
#define	EVENT_DeskViewportChanged	14
#define	EVENT_Startup				15
#define	EVENT_Shutdown				16
#define	EVENT_Config				17
#define	EVENT_ModuleConfig			18
#define	EVENT_PlaySound				19
#define	AFTERSTEP_EVENTS_NUM 		20


#define EVENT_ID_START          		(CLEAN_ID_END+1)

#define EVENT_WindowAdded_ID            (EVENT_ID_START+EVENT_WindowAdded)
#define EVENT_WindowNames_ID            (EVENT_ID_START+EVENT_WindowNames)    		
#define EVENT_WindowDestroyed_ID        (EVENT_ID_START+EVENT_WindowDestroyed)
#define EVENT_WindowActivated_ID        (EVENT_ID_START+EVENT_WindowActivated)		
#define EVENT_WindowRaised_ID           (EVENT_ID_START+EVENT_WindowRaised)
#define EVENT_WindowIconified_ID        (EVENT_ID_START+EVENT_WindowIconified)
#define EVENT_WindowDeiconified_ID      (EVENT_ID_START+EVENT_WindowDeiconified)
#define EVENT_WindowShaded_ID           (EVENT_ID_START+EVENT_WindowShaded)
#define EVENT_WindowUnshaded_ID         (EVENT_ID_START+EVENT_WindowUnshaded)
#define EVENT_WindowStuck_ID            (EVENT_ID_START+EVENT_WindowStuck)
#define EVENT_WindowUnstuck_ID          (EVENT_ID_START+EVENT_WindowUnstuck)
#define EVENT_WindowMaximized_ID        (EVENT_ID_START+EVENT_WindowMaximized)
#define EVENT_WindowRestored_ID         (EVENT_ID_START+EVENT_WindowRestored)
#define EVENT_BackgroundChanged_ID      (EVENT_ID_START+EVENT_BackgroundChanged)
#define EVENT_DeskViewportChanged_ID    (EVENT_ID_START+EVENT_DeskViewportChanged)
#define EVENT_Startup_ID                (EVENT_ID_START+EVENT_Startup)
#define EVENT_Shutdown_ID               (EVENT_ID_START+EVENT_Shutdown)				
#define EVENT_Config_ID               	(EVENT_ID_START+EVENT_Config)				   
#define EVENT_ModuleConfig_ID          	(EVENT_ID_START+EVENT_ModuleConfig)				   

#define EVENT_ID_END          			(EVENT_ID_START+AFTERSTEP_EVENTS_NUM)

#define AUDIO_ID_START        	(EVENT_ID_END+1)	
#define AUDIO_PLAYCMD_ID        (AUDIO_ID_START+1)
#define AUDIO_DELAY_ID          (AUDIO_ID_START+2)
#define AUDIO_RPLAY_HOST_ID     (AUDIO_ID_START+3)
#define AUDIO_RPLAY_PRI_ID      (AUDIO_ID_START+4)
#define AUDIO_RPLAY_VOL_ID      (AUDIO_ID_START+5)
#define AUDIO_AUDIO_ID          (AUDIO_ID_START+6)

#define AUDIO_ID_END            (AUDIO_ID_START+10)

/* config data structure */

typedef struct
{
	char *playcmd;
  	char *sounds[AFTERSTEP_EVENTS_NUM] ;

#define AUDIO_SET_DELAY  			(0x01<<0)
#define AUDIO_SET_RPLAY_HOST		(0x01<<1)
#define AUDIO_SET_RPLAY_PRIORITY  	(0x01<<2)
#define AUDIO_SET_RPLAY_VOLUME  	(0x01<<3)
  
	ASFlagType set_flags ;

	int delay;
	char *rplay_host;
	int rplay_priority;
	int rplay_volume;

	struct FreeStorageElem *more_stuff;
}AudioConfig;

AudioConfig *CreateAudioConfig ();
AudioConfig *ParseAudioOptions (const char *filename, char *myname);
int WriteAudioOptions (const char *filename, char *myname,
		       AudioConfig * config, unsigned long flags);
void DestroyAudioConfig (AudioConfig * config);

/**************************************************************************/
/***************************************************************************/
/*                        Ident pasring definitions                 */
/***************************************************************************/


#define IDENT_ID_START        	AUDIO_ID_END

#define IDENT_Geometry_ID 		(IDENT_ID_START)

#define IDENT_ID_END        	IDENT_ID_START+10

/* config data structure */

typedef struct
{
#define IDENT_SET_GEOMETRY  (0x01<<0)
	ASFlagType set_flags ;
		   
	ASGeometry geometry;
	
	MyStyleDefinition *style_defs;
	struct FreeStorageElem *more_stuff;


	int gravity ;

}
IdentConfig;

IdentConfig *CreateIdentConfig ();
IdentConfig *ParseIdentOptions (const char *filename, char *myname);
int WriteIdentOptions (const char *filename, char *myname,
		       IdentConfig * config, unsigned long flags);
void DestroyIdentConfig (IdentConfig * config);

/**************************************************************************/

/***************************************************************************/
/* ALL POSSIBLE aFTERsTEP CONFIGS : */
/***************************************************************************/

#define ASCONFIG_ID_START        		(100000)
#define ASCONFIG_Base_ID				(ASCONFIG_ID_START+1)
#define ASCONFIG_Colorscheme_ID			(ASCONFIG_ID_START+2)
#define ASCONFIG_Database_ID			(ASCONFIG_ID_START+3)
#define ASCONFIG_AutoExec_ID			(ASCONFIG_ID_START+4)
#define ASCONFIG_Command_ID				(ASCONFIG_ID_START+5)
#define ASCONFIG_Look_ID				(ASCONFIG_ID_START+6)
#define ASCONFIG_Feel_ID				(ASCONFIG_ID_START+7)
#define ASCONFIG_Theme_ID				(ASCONFIG_ID_START+8)
#define ASCONFIG_Pager_ID				(ASCONFIG_ID_START+9)
#define ASCONFIG_Wharf_ID				(ASCONFIG_ID_START+10)
#define ASCONFIG_WinList_ID				(ASCONFIG_ID_START+11)
#define ASCONFIG_WinTabs_ID				(ASCONFIG_ID_START+12)
#define ASCONFIG_ID_END                 (ASCONFIG_ID_START+16)

extern struct SyntaxDef ASConfigSyntax;   /* THIS SYNTAX INCLUDES ALL OTHER SYNTAXES */

/***************************************************************************/
/***************************************************************************/
/* Desktop Category loading from file or directory tree : 				   */
/***************************************************************************/

Bool load_category_tree( struct ASCategoryTree*	ct );

void DestroyCategories();
void ReloadCategories(Bool cached);
void UpdateCategoriesCache();
struct ASDesktopCategory;
struct ASDesktopCategory *name2desktop_category( const char *name, struct ASCategoryTree **tree_return );


/***************************************************************************/


#ifdef __cplusplus
}
#endif


#endif /* AFTERCONF_H_FILE_INCLUDED */
