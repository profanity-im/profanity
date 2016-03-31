/*
 * window.h
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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

#include "contact.h"
#include "muc.h"
#include "ui/ui.h"
#include "ui/buffer.h"
#include "xmpp/xmpp.h"
#include "chat_state.h"

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#define PAD_SIZE 1000

void win_move_to_end(ProfWin *window);
void win_show_status_string(ProfWin *window, const char *const from,
    const char *const show, const char *const status,
    GDateTime *last_activity, const char *const pre,
    const char *const default_show);
void win_print_incoming_message(ProfWin *window, GDateTime *timestamp,
    const char *const from, const char *const message, prof_enc_t enc_mode);
void win_print_with_receipt(ProfWin *window, const char show_char, int pad_indent, GTimeVal *tstamp, int flags,
    theme_item_t theme_item, const char *const from, const char *const message, char *id);
void win_newline(ProfWin *window);
void win_redraw(ProfWin *window);
int win_roster_cols(void);
int win_occpuants_cols(void);
void win_sub_print(WINDOW *win, char *msg, gboolean newline, gboolean wrap, int indent);
void win_sub_newline_lazy(WINDOW *win);
void win_mark_received(ProfWin *window, const char *const id);

gboolean win_has_active_subwin(ProfWin *window);

void win_page_up(ProfWin *window);
void win_page_down(ProfWin *window);
void win_sub_page_down(ProfWin *window);
void win_sub_page_up(ProfWin *window);

#endif
