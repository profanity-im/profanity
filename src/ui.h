/*
 * ui.h
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

#ifndef UI_H
#define UI_H

#include "config.h"

#include <glib.h>

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#endif
#ifdef HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#endif

#include "jabber.h"

#define INP_WIN_MAX 1000

typedef enum {
    WIN_UNUSED,
    WIN_CONSOLE,
    WIN_CHAT,
    WIN_MUC,
    WIN_PRIVATE
} win_type_t;

struct prof_win {
    char from[100];
    WINDOW *win;
    win_type_t type;
    int y_pos;
    int paged;
    int unread;
    int history_shown;
};

// gui startup and shutdown, resize
void gui_init(void);
void gui_refresh(void);
void gui_close(void);
void gui_resize(const int ch, const char * const input,
    const int size);

// create windows
void create_title_bar(void);
void create_status_bar(void);
void create_input_window(void);

// title bar actions
void title_bar_refresh(void);
void title_bar_resize(void);
void title_bar_show(const char * const title);
void title_bar_title(void);
void title_bar_set_status(jabber_presence_t status);
void title_bar_set_recipient(char *from);
void title_bar_set_typing(gboolean is_typing);
void title_bar_draw(void);

// main window actions
void win_close_win(void);
int win_in_chat(void);
char *win_get_recipient(void);
void win_show_typing(const char * const from);
void win_show_gone(const char * const from);
void win_show_incomming_msg(const char * const from, const char * const message,
    GTimeVal *tv_stamp, gboolean priv);
void win_show_error_msg(const char * const from, const char *err_msg);
void win_show_system_msg(const char * const from, const char *message);
void win_show_outgoing_msg(const char * const from, const char * const to,
    const char * const message);
void win_handle_special_keys(const int * const ch);
void win_page_off(void);
void win_contact_online(const char * const from, const char * const show,
    const char * const status);
void win_contact_offline(const char * const from, const char * const show,
    const char * const status);
void win_disconnected(void);
void win_show(const char * const msg);
void win_bad_show(const char * const msg);
void win_remind(void);
void win_activity(void);
void win_no_activity(void);

void win_join_chat(const char * const room, const char * const nick);
void win_show_room_roster(const char * const room);
int win_in_groupchat(void);
void win_show_room_history(const char * const room_jid, const char * const nick,
    GTimeVal tv_stamp, const char * const message);
void win_show_room_message(const char * const room_jid, const char * const nick,
    const char * const message);
void win_show_room_subject(const char * const room_jid,
    const char * const subject);
void win_show_room_broadcast(const char * const room_jid,
    const char * const message);
void win_show_room_member_offline(const char * const room, const char * const nick);
void win_show_room_member_online(const char * const room,
    const char * const nick, const char * const show, const char * const status);
void win_show_status(const char * const contact);
void win_show_wins(void);
int win_in_private_chat(void);
void win_show_room_member_nick_change(const char * const room,
    const char * const old_nick, const char * const nick);
void win_show_room_nick_change(const char * const room, const char * const nick);
void win_show_room_member_presence(const char * const room,
    const char * const nick, const char * const show, const char * const status);
void win_load_colours(void);

// console window actions
void cons_about(void);
void cons_help(void);
void cons_basic_help(void);
void cons_settings_help(void);
void cons_presence_help(void);
void cons_navigation_help(void);
void cons_prefs(void);
void cons_bad_command(const char * const cmd);
void cons_show(const char * const cmd, ...);
void cons_show_time(void);
void cons_show_word(const char * const word);
void cons_bad_show(const char * const cmd, ...);
void cons_highlight_show(const char * const cmd);
void cons_show_contacts(GSList * list);
void cons_check_version(gboolean not_available_msg);

// status bar actions
void status_bar_refresh(void);
void status_bar_resize(void);
void status_bar_clear(void);
void status_bar_clear_message(void);
void status_bar_get_password(void);
void status_bar_print_message(const char * const msg);
void status_bar_inactive(const int win);
void status_bar_active(const int win);
void status_bar_new(const int win);
void status_bar_update_time(void);

// input window actions
void inp_get_char(int *ch, char *input, int *size);
void inp_clear(void);
void inp_win_resize(const char * input, const int size);
void inp_put_back(void);
void inp_non_block(void);
void inp_block(void);
void inp_get_password(char *passwd);
void inp_replace_input(char *input, const char * const new_input, int *size);

#endif
