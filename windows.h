#ifndef WINDOWS_H
#define WINDOWS_h

#include <strophe/strophe.h>
#include <ncurses.h>

struct prof_win {
    char from[100];
    WINDOW *win;
};

void gui_init(void);
void gui_close(void);
void switch_to(int i);
void close_win(void);
int in_chat(void);
void get_recipient(char *recipient);
void show_incomming_msg(char *from, char *message);
void show_outgoing_msg(char *from, char *message);
void inp_get_command_str(char *cmd);
void inp_poll_char(int *ch, char command[], int *size);
void inp_clear(void);
void inp_non_block(void);
void inp_get_password(char *passwd);
void bar_print_message(char *msg);
void cons_help(void);
void cons_bad_command(char *cmd);
#endif
