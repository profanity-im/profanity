#ifndef WINDOWS_H
#define WINDOWS_h

// windows
WINDOW *title_bar;
WINDOW *cmd_bar;
WINDOW *cmd_win;
WINDOW *main_win;

// window creation
void initgui(void);
void show_incomming_msg(char *from, char *message);

#endif
