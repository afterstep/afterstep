struct target_struct
{
  char res[256];
  char class[256];
  char name[256];
  char icon_name[256];
  unsigned long id;
  unsigned long frame;
  long frame_x;
  long frame_y;
  long frame_w;
  long frame_h;
  long base_w;
  long base_h;
  long width_inc;
  long height_inc;
  long desktop;
  unsigned long gravity;
  unsigned long flags;
  long title_h;
  long border_w;
  Bool gnome_enabled;
};

struct Item
{
  char* col1;
  char* col2;
  struct Item* next;
};

/*************************************************************************
 *
 * Subroutine Prototypes
 * 
 *************************************************************************/
void ParseOptions(const char *);
void Loop(int *fd);
void SendInfo(char *message,unsigned long window);
void DeadPipe(int nonsense);
void process_message(unsigned long type,unsigned long *body);
void GetTargetWindow(Window *app_win);
void sleep_a_little(int n);
void RedrawWindow(void);
void change_window_name(char *str);
Pixel GetColor(char *name);
void nocolor(char *a, char *b);
void CopyString(char **dest, char *source);
char *CatString2(char *a, char *b);
void AddToList(char *, char *);
void MakeList(void);
void freelist(void);

void list_configure(unsigned long *body);
void list_window_name(unsigned long *body);
void list_icon_name(unsigned long *body);
void list_class(unsigned long *body);
void list_res_name(unsigned long *body);
void list_end(void);
