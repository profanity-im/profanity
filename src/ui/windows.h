/*
 * windows.h
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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

#ifndef UI_WINDOWS_H
#define UI_WINDOWS_H

void wins_init(void);
ProfWin * wins_get_console(void);
ProfWin * wins_get_current(void);
void wins_set_current_by_num(int i);
ProfWin * wins_get_by_num(int i);
ProfWin * wins_get_next(void);
ProfWin * wins_get_previous(void);
ProfWin * wins_get_by_recipient(const char * const recipient);
int wins_get_num(ProfWin *window);
int wins_get_current_num(void);
void wins_close_current(void);
void wins_close_by_num(int i);
void wins_update_virtual_current(void);
void wins_update_virtual_console(void);
void wins_clear_current(void);
gboolean wins_is_current(ProfWin *window);
ProfWin * wins_new(const char * const from, win_type_t type);
int wins_get_total_unread(void);
void wins_resize_all(void);
gboolean wins_duck_exists(void);
GSList * wins_get_chat_recipients(void);
GSList * wins_get_prune_recipients(void);
void wins_lost_connection(void);
gboolean wins_tidy(void);
GSList * wins_create_summary(void);
void wins_destroy(void);
GList * wins_get_nums(void);

#endif
