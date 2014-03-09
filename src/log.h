/*
 * log.h
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

#ifndef LOG_H
#define LOG_H

#include "glib.h"

// log levels
typedef enum {
    PROF_LEVEL_DEBUG,
    PROF_LEVEL_INFO,
    PROF_LEVEL_WARN,
    PROF_LEVEL_ERROR
} log_level_t;

typedef enum {
    PROF_IN_LOG,
    PROF_OUT_LOG
} chat_log_direction_t;

void log_init(log_level_t filter);
log_level_t log_get_filter(void);
void log_close(void);
void log_debug(const char * const msg, ...);
void log_info(const char * const msg, ...);
void log_warning(const char * const msg, ...);
void log_error(const char * const msg, ...);
void log_msg(log_level_t level, const char * const area,
    const char * const msg);
log_level_t log_level_from_string(char *log_level);

void chat_log_init(void);
void chat_log_chat(const gchar * const login, gchar *other,
    const gchar * const msg, chat_log_direction_t direction, GTimeVal *tv_stamp);
void chat_log_close(void);
GSList * chat_log_get_previous(const gchar * const login,
    const gchar * const recipient, GSList *history);

void groupchat_log_init(void);
void groupchat_log_chat(const gchar * const login, const gchar * const room,
    const gchar * const nick, const gchar * const msg);
#endif
