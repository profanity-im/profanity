#ifndef WINDOWS_H
#define WINDOWS_h

#include <strophe/strophe.h>
#include <ncurses.h>

// windows
WINDOW *title_bar;
WINDOW *cmd_bar;
WINDOW *cmd_win;
WINDOW *main_win;

void gui_init(void);
void gui_close(void);
void show_incomming_msg(char *from, char *message);
void cmd_get_command_str(char *cmd);
void cmd_get_password(char *passwd);
void bar_print_message(char *msg);

#endif
