#ifndef SESSION_H_HEADER_INCLUDED
#define SESSION_H_HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

struct ASFeel ;
struct MyLook ;


typedef struct ASDeskSession
{
  int desk;
  char *background_file;
  char *feel_file;
  struct ASFeel *feel ;
  char *look_file;
  struct MyLook *look ;
  char *theme_file;
}ASDeskSession;

typedef struct ASSession
{
  struct ScreenInfo *scr ;                     /* back pointer to owning screen data */
  int   colordepth ;
  char *ashome, *asshare ; /* locations of the AfterStep config directories */
  char *overriding_file  ; /* if user specifies -f option to AS then we use this file for everything */
  char *overriding_look, *overriding_feel, *overriding_theme ;

  ASDeskSession *defaults;
  ASDeskSession **desks;
  unsigned int desks_used, desks_allocated ;

  void (*on_feel_changed_func)( struct ScreenInfo *scr, int desk, struct ASFeel *old_feel );
  void (*on_look_changed_func)( struct ScreenInfo *scr, int desk, struct MyLook *old_look );

  Bool changed;
}ASSession;


ASSession *create_assession ( struct ScreenInfo *scr, char *ashome, char *asshare);
void 	   destroy_assession (ASSession * session);

void       update_default_session ( ASSession *session, int func);

void 	   set_session_override(ASSession * session, const char *overriding_file, int function );
inline const char *get_session_override(ASSession * session, int function );

void 	   change_default_session (ASSession * session, const char *new_val, int function);
void 	   change_desk_session (ASSession * session, int desk, const char *new_val, int function);

void	   change_desk_session_feel (ASSession * session, int desk, struct ASFeel *feel);
void 	   change_desk_session_look (ASSession * session, int desk, struct MyLook *look);


const char   *get_session_file (ASSession * session, int desk, int function);
char **get_session_file_list (ASSession *session, int desk1, int desk2, int function);

char *make_session_file   (ASSession * session, const char *source, Bool use_depth );
char *make_session_dir    (ASSession * session, const char *source, Bool use_depth );
char *make_session_data_file  (ASSession * session, Bool shared, int if_mode_only, ... );

int CheckOrCreate (const char *what);
void check_AfterStep_dirtree ( char * ashome, Bool create_non_conf);

/* legacy session using non-configurable dir to store config files : */
ASSession *GetNCASSession (ScreenInfo *scr, const char *home, const char *share );

#ifdef __cplusplus
}
#endif


#endif

