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

void KIPC_sendMessage(KIPC_Message msg, Window w, int data);


#ifdef __cplusplus
}
#endif


#endif /* #ifndef AFTERSTEP_KDE_H_HEADER_FILE_INCLUDED */

