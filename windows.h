#ifndef WINDOWS_H
#define WINDOWS_h

#include <strophe/strophe.h>
#include <ncurses.h>

void gui_init(void);
void gui_close(void);
void show_incomming_msg(char *from, char *message);
void show_outgoing_msg(char *from, char *message);
void cmd_get_command_str(char *cmd);
void cmd_poll_char(int *ch, char command[], int *size);
void cmd_clear(void);
void cmd_non_block(void);
void cmd_get_password(char *passwd);
void bar_print_message(char *msg);

#endif
