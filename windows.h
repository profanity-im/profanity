/* 
 * windows.h
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
 * 
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef WINDOWS_H
#define WINDOWS_h

#include <ncurses.h>
#include "contact_list.h"

struct prof_win {
    char from[100];
    WINDOW *win;
    int y_pos;
    int paged;
};

// gui startup and shutdown
void gui_init(void);
void gui_refresh(void);
void gui_close(void);

// create windows
void create_title_bar(void);
void create_status_bar(void);
void create_input_window(void);

// title bar actions
void title_bar_refresh(void);
void title_bar_show(const char * const title);
void title_bar_title(void);
void title_bar_connected(void);
void title_bar_disconnected(void);

// main window actions
int win_close_win(void);
int win_in_chat(void);
char *win_get_recipient(void);
void win_show_incomming_msg(const char * const from, const char * const message);
void win_show_outgoing_msg(const char * const from, const char * const to, 
    const char * const message);
void win_handle_special_keys(const int * const ch);
void win_page_off(void);
void win_contact_online(const char * const from, const char * const show, 
    const char * const status);
void win_contact_offline(const char * const from, const char * const show, 
    const char * const status);

// console window actions
void cons_help(void);
void cons_bad_command(const char * const cmd);
void cons_bad_connect(void);
void cons_not_disconnected(void);
void cons_not_connected(void);
void cons_bad_message(void);
void cons_show(const char * const cmd);
void cons_bad_show(const char * const cmd);
void cons_highlight_show(const char * const cmd);
void cons_show_online_contacts(struct contact_node_t * list);

// status bar actions
void status_bar_refresh(void);
void status_bar_clear(void);
void status_bar_get_password(void);
void status_bar_print_message(const char * const msg);
void status_bar_inactive(const int win);
void status_bar_active(const int win);
void status_bar_update_time(void);

// input window actions
void inp_get_char(int *ch, char *input, int *size);
void inp_clear(void);
void inp_put_back(void);
void inp_non_block(void);
void inp_block(void);
void inp_get_password(char *passwd);

#endif
