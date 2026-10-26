/*
 * window.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include "config.h"

#include <wchar.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#elif HAVE_CURSES_H
#include <curses.h>
#endif

#include "ui/ui.h"
#include "ui/buffer.h"
#include "xmpp/xmpp.h"
#include "xmpp/chat_state.h"
#include "xmpp/contact.h"
#include "xmpp/muc.h"

void win_move_to_end(ProfWin* window);
void win_show_status_string(ProfWin* window, const gchar* const from,
                            const gchar* const show, const gchar* const status,
                            GDateTime* last_activity, const gchar* const pre,
                            const gchar* const default_show);

void win_print_them(ProfWin* window, theme_item_t theme_item, GDateTime* timestamp, const gchar* const show_char, int flags, const gchar* const them);
void win_print_incoming(ProfWin* window, const gchar* const from, ProfMessage* message);
void win_print_outgoing(ProfWin* window, const gchar* show_char, const gchar* const id, const gchar* const replace_id, const gchar* const message);
void win_print_outgoing_with_receipt(ProfWin* window, const gchar* show_char, const gchar* const from, const gchar* const message, const gchar* const id, const gchar* const replace_id);
void win_println_incoming_muc_msg(ProfWin* window, const gchar* show_char, int flags, const ProfMessage* const message);
void win_print_outgoing_muc_msg(ProfWin* window, const gchar* show_char, const gchar* const me, const gchar* const id, const gchar* const replace_id, const gchar* const message);
void win_print_history(ProfWin* window, const ProfMessage* const message);
void win_print_old_history(ProfWin* window, const ProfMessage* const message);

void win_newline(ProfWin* window);
void win_redraw(ProfWin* window);
void win_print_loading_history(ProfWin* window);
void win_remove_loading_history(ProfWin* window);
void win_print_end_of_archive(ProfWin* window);
int win_roster_cols(void);
int win_occpuants_cols(void);
void win_sub_print(WINDOW* win, char* msg, gboolean newline, gboolean wrap, int indent);
void win_sub_newline_lazy(WINDOW* win);
void win_mark_received(ProfWin* window, const gchar* const id);
void win_update_entry_message(ProfWin* window, const gchar* const id, const gchar* const message);
void win_update_entry(ProfWin* window, const gchar* const id, const gchar* const message, theme_item_t theme_item, int flags);
void win_print_status_with_id(ProfWin* window, const gchar* const message, const gchar* const id, theme_item_t theme_item, int flags);

gboolean win_has_active_subwin(ProfWin* window);

gboolean win_warn_needed(ProfWin* window, const gchar* const type, const gchar* const jid);
void win_warn_sent(ProfWin* window, const gchar* const type, const gchar* const jid);
void win_clear_warned_jids(ProfWin* window);

void win_page_up(ProfWin* window, int scroll_size);
void win_page_down(ProfWin* window, int scroll_size);
void win_sub_page_down(ProfWin* window);
void win_sub_page_up(ProfWin* window);

void win_insert_last_read_position_marker(ProfWin* window, const gchar* const id);
void win_remove_entry_message(ProfWin* window, const gchar* const id);

gchar* win_quote_autocomplete(ProfWin* window, const gchar* const input, gboolean previous);

const gchar* get_show_char(prof_enc_t encryption_mode);
const gchar* get_enc_char(prof_enc_t enc_mode, const gchar* alt);

#endif
