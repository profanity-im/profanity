#ifndef WINDOWS_H
#define WINDOWS_h

#include <strophe/strophe.h>
#include <ncurses.h>

void gui_init(void);
void gui_close(void);
void show_incomming_msg(char *from, char *message);
void show_outgoing_msg(char *from, char *message);
void inp_get_command_str(char *cmd);
void inp_poll_char(int *ch, char command[], int *size);
void inp_clear(void);
void inp_non_block(void);
void inp_get_password(char *passwd);
void bar_print_message(char *msg);
void cons_help(void);
void cons_show(void);
void chat_show(void);
void cons_bad_command(char *cmd);
#endif
