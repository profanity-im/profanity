/*
 * window.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include "config.h"

#include <wchar.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "ui/ui.h"
#include "ui/buffer.h"
#include "xmpp/xmpp.h"
#include "xmpp/chat_state.h"
#include "xmpp/contact.h"
#include "xmpp/muc.h"

#define PAD_SIZE 1000

void win_move_to_end(ProfWin *window);
void win_show_status_string(ProfWin *window, const char *const from,
    const char *const show, const char *const status,
    GDateTime *last_activity, const char *const pre,
    const char *const default_show);

void win_print_them(ProfWin *window, theme_item_t theme_item, char ch, int flags, const char *const them);
void win_println_them_message(ProfWin *window, char ch, int flags, const char *const them, const char *const id, const char *const replace_id, const char *const message, ...);
void win_print_outgoing_muc_msg(ProfWin *window, char ch, const char *const me, const char *const id, const char *const replace_id, const char *const message, ...);

void win_print_outgoing(ProfWin *window, const char ch, const char *const id, const char *const replace_id, const char *const message, ...);
void win_print_incoming(ProfWin *window, const char *const from, ProfMessage *message);
void win_print_history(ProfWin *window, GDateTime *timestamp, const char *const message, ...);

void win_print_http_upload(ProfWin *window, const char *const message, char *url);

void win_print_with_receipt(ProfWin *window, const char show_char, const char *const from, const char *const message,
    char *id);

void win_newline(ProfWin *window);
void win_redraw(ProfWin *window);
int win_roster_cols(void);
int win_occpuants_cols(void);
void win_sub_print(WINDOW *win, char *msg, gboolean newline, gboolean wrap, int indent);
void win_sub_newline_lazy(WINDOW *win);
void win_mark_received(ProfWin *window, const char *const id);
void win_update_entry_message(ProfWin *window, const char *const id, const char *const message);

gboolean win_has_active_subwin(ProfWin *window);

void win_page_up(ProfWin *window);
void win_page_down(ProfWin *window);
void win_sub_page_down(ProfWin *window);
void win_sub_page_up(ProfWin *window);

void win_insert_last_read_position_marker(ProfWin *window, char* id);
void win_remove_entry_message(ProfWin *window, const char *const id);

#endif
