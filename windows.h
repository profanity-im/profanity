#ifndef WINDOWS_H
#define WINDOWS_h

#include <ncurses.h>

struct prof_win {
    char from[100];
    WINDOW *win;
};

// create windows
void create_title_bar(void);
void create_input_bar(void);
void create_input_window(void);

// input bar actions
void inp_bar_print_message(char *msg);
void inp_bar_inactive(int win);
void inp_bar_active(int win);
void inp_bar_update_time(void);

// input window actions
void inp_get_command_str(char *cmd);
void inp_poll_char(int *ch, char command[], int *size);
void inp_clear(void);
void inp_put_back(void);
void inp_non_block(void);
void inp_get_password(char *passwd);

void gui_init(void);
void gui_close(void);
void title_bar_show(char *title);
void switch_to(int i);
void close_win(void);
int in_chat(void);
void get_recipient(char *recipient);
void show_incomming_msg(char *from, char *message);
void show_outgoing_msg(char *from, char *to, char *message);
void cons_help(void);
void cons_bad_command(char *cmd);
void cons_show(char *cmd);
#endif
