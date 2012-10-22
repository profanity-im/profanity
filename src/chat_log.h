/*
 * chat_log.h
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

#ifndef CHAT_LOG_H
#define CHAT_LOG_H

#include <glib.h>

typedef enum {
    IN,
    OUT
} chat_log_direction_t;

void chat_log_init(void);
void chat_log_chat(const gchar * const login, gchar *other,
    const gchar * const msg, chat_log_direction_t direction);
void chat_log_close(void);
GSList * chat_log_get_previous(const gchar * const login,
    const gchar * const recipient, GSList *history);

#endif
