/*
 * window_list.h
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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

#ifndef UI_WINDOW_LIST_H
#define UI_WINDOW_LIST_H

#include "ui/ui.h"

void wins_init(void);

ProfWin* wins_new_xmlconsole(void);
ProfWin* wins_new_chat(const char *const barejid);
ProfWin* wins_new_muc(const char *const roomjid);
ProfWin* wins_new_muc_config(const char *const roomjid, DataForm *form);
ProfWin* wins_new_private(const char *const fulljid);
ProfWin* wins_new_plugin(const char *const plugin_name, const char *const tag);

gboolean wins_chat_exists(const char *const barejid);
GList* wins_get_private_chats(const char *const roomjid);
void wins_private_nick_change(const char *const roomjid, const char *const oldnick, const char *const newnick);
void wins_change_nick(const char *const barejid, const char *const oldnick, const char *const newnick);
void wins_remove_nick(const char *const barejid, const char *const oldnick);

ProfWin* wins_get_console(void);
ProfChatWin* wins_get_chat(const char *const barejid);
GList* wins_get_chat_unsubscribed(void);
ProfMucWin* wins_get_muc(const char *const roomjid);
ProfMucConfWin* wins_get_muc_conf(const char *const roomjid);
ProfPrivateWin* wins_get_private(const char *const fulljid);
ProfPluginWin* wins_get_plugin(const char *const tag);
ProfXMLWin* wins_get_xmlconsole(void);

void wins_close_plugin(char *tag);

ProfWin* wins_get_current(void);

void wins_set_current_by_num(int i);

ProfWin* wins_get_by_num(int i);
ProfWin* wins_get_by_string(char *str);

ProfWin* wins_get_next(void);
ProfWin* wins_get_previous(void);
int wins_get_num(ProfWin *window);
int wins_get_current_num(void);
void wins_close_current(void);
void wins_close_by_num(int i);
gboolean wins_is_current(ProfWin *window);
gboolean wins_do_notify_remind(void);
int wins_get_total_unread(void);
void wins_resize_all(void);
GSList* wins_get_chat_recipients(void);
GSList* wins_get_prune_wins(void);
void wins_lost_connection(void);
gboolean wins_tidy(void);
GSList* wins_create_summary(gboolean unread);
void wins_destroy(void);
GList* wins_get_nums(void);
gboolean wins_swap(int source_win, int target_win);
void wins_hide_subwin(ProfWin *window);
void wins_show_subwin(ProfWin *window);

char* win_autocomplete(const char *const search_str, gboolean previous);
void win_reset_search_attempts(void);
char* win_close_autocomplete(const char *const search_str, gboolean previous);
void win_close_reset_search_attempts(void);

#endif
