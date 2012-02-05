#ifndef WINDOWS_H
#define WINDOWS_h

// windows
WINDOW *title_bar;
WINDOW *cmd_bar;
WINDOW *cmd_win;
WINDOW *main_win;

// window creation
void create_title_bar(void);
void create_command_bar(void);
void create_command_window(void);
void create_main_window(void);

#endif
