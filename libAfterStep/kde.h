#ifndef AFTERSTEP_KDE_H_HEADER_FILE_INCLUDED
#define AFTERSTEP_KDE_H_HEADER_FILE_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

typedef enum  	KIPC_Message {
  KIPC_PaletteChanged = 0, 
  KIPC_FontChanged, 
  KIPC_StyleChanged, 
  KIPC_BackgroundChanged,
  KIPC_SettingsChanged, 
  KIPC_IconChanged, 
  KIPC_ToolbarStyleChanged, 
  KIPC_ClipboardConfigChanged,
  KIPC_BlockShortcuts, 
  KIPC_UserMessage = 32
} KIPC_Message;

/*
 * we translate typical KDE config format : 
 * #line_comment
 * [group_name]
 * item_name=value
 * #line_comment2
 * ...
 * into XML like so : 
 * <CONTAINER>
 * <group>
 * 		<comment>line_comment</comment>
 * </group>
 * <group name="group_name">
 * 	  	<item name="item_name">value</item>
 * 		<comment>line_comment2</comment>
 * ...
 * </group>
 * </CONTAINER>
 */

struct xml_elem_t;

typedef enum KDEConfig_XMLTagIDs
{	
	KDEConfig_item = 0,		   
	KDEConfig_name,		   
	KDEConfig_group,
	KDEConfig_comment,		   

	KDEConfig_SUPPORTED_IDS
}KDEConfig_XMLTagIDs;

void KIPC_sendMessage(KIPC_Message msg, Window w, int data);

struct xml_elem_t* load_KDE_config(const char* realfilename); 
Bool save_KDE_config(const char* realfilename, struct xml_elem_t *elem );
void merge_KDE_config_groups( struct xml_elem_t *from, struct xml_elem_t *to );
struct xml_elem_t *get_KDE_config_group( struct xml_elem_t *config, const char *name, Bool create_if_missing );
void set_KDE_config_group_item( struct xml_elem_t *group, const char *item_name, const char *value );

Bool add_KDE_colorscheme( const char *new_cs_file );


#ifdef __cplusplus
}
#endif


#endif /* #ifndef AFTERSTEP_KDE_H_HEADER_FILE_INCLUDED */

