/*
 * mock_log.c
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
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <cmocka.h>
#include <glib.h>
#include <setjmp.h>

#include "log.h"

void
log_init(log_level_t filter, char* log_file)
{
}
log_level_t
log_get_filter(void)
{
    return mock_type(log_level_t);
}

void
log_reinit(void)
{
}
void
log_close(void)
{
}
void
log_debug(const char* const msg, ...)
{
}
void
log_info(const char* const msg, ...)
{
}
void
log_warning(const char* const msg, ...)
{
}
void
log_error(const char* const msg, ...)
{
}
void
log_msg(log_level_t level, const char* const area, const char* const msg)
{
}

const char*
get_log_file_location(void)
{
    return mock_ptr_type(char*);
}

log_level_t
log_level_from_string(char* log_level)
{
    return mock_type(log_level_t);
}

void
log_stderr_init(log_level_t level)
{
}
void
log_stderr_close(void)
{
}
void
log_stderr_handler(void)
{
}

void
chat_log_init(void)
{
}

void
chat_log_msg_out(const char* const barejid, const char* const msg, const char* const resource)
{
}
void
chat_log_otr_msg_out(const char* const barejid, const char* const msg, const char* const resource)
{
}
void
chat_log_pgp_msg_out(const char* const barejid, const char* const msg, const char* const resource)
{
}
void
chat_log_omemo_msg_out(const char* const barejid, const char* const msg, const char* const resource)
{
}

void
chat_log_msg_in(ProfMessage* message)
{
}
void
chat_log_otr_msg_in(ProfMessage* message)
{
}
void
chat_log_pgp_msg_in(ProfMessage* message)
{
}
void
chat_log_omemo_msg_in(ProfMessage* message)
{
}

void
chat_log_close(void)
{
}

void
groupchat_log_init(void)
{
}
void
groupchat_log_msg_in(const gchar* const room, const gchar* const nick, const gchar* const msg)
{
}
void
groupchat_log_msg_out(const gchar* const room, const gchar* const msg)
{
}
void
groupchat_log_omemo_msg_in(const gchar* const room, const gchar* const nick, const gchar* const msg)
{
}
void
groupchat_log_omemo_msg_out(const gchar* const room, const gchar* const msg)
{
}
