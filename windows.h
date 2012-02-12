#ifndef WINDOWS_H
#define WINDOWS_h

#include <ncurses.h>

struct prof_win {
    char from[100];
    WINDOW *win;
};

// gui startup and shutdown
void gui_init(void);
void gui_close(void);

// create windows
void create_title_bar(void);
void create_status_bar(void);
void create_input_window(void);

// status bar actions
void status_bar_clear(void);
void status_bar_get_password(void);
void status_bar_print_message(char *msg);
void status_bar_inactive(int win);
void status_bar_active(int win);
void status_bar_update_time(void);

// input window actions
void inp_get_command_str(char *cmd);
void inp_poll_char(int *ch, char command[], int *size);
void inp_clear(void);
void inp_put_back(void);
void inp_non_block(void);
void inp_get_password(char *passwd);

// main window actions
void win_title_bar_show(char *title);
int win_is_active(int i);
void win_switch_to(int i);
void win_close_win(void);
int win_in_chat(void);
void win_get_recipient(char *recipient);
void win_show_incomming_msg(char *from, char *message);
void win_show_outgoing_msg(char *from, char *to, char *message);
void win_cons_help(void);
void win_cons_bad_command(char *cmd);
void win_cons_show(char *cmd);

#endif
