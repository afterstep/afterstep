/* from Scroll.c */
void DeadPipe(int nonsense);
void GetTargetWindow(Window *app_win);
void nocolor(char *a, char *b);
Window ClientWindow(Window input);

/* from GrabWindow.c */
void RelieveWindow(Window win,int x,int y,int w,int h, GC rgc,GC sgc);
void CreateWindow(int x, int y,int w, int h);
Pixel GetShadow(Pixel background);
Pixel GetHilite(Pixel background);
Pixel GetColor(char *name);
void Loop(Window target);
void RedrawWindow(Window target);
void change_window_name(char *str);
void send_clientmessage (Window w, Atom a, Time timestamp);
void GrabWindow(Window target);
void change_icon_name(char *str);
void RedrawLeftButton(GC rgc, GC sgc,int x1,int y1);
void RedrawRightButton(GC rgc, GC sgc,int x1,int y1);
void RedrawTopButton(GC rgc, GC sgc,int x1,int y1);
void RedrawBottomButton(GC rgc, GC sgc,int x1,int y1);
