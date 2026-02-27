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

#include <glib.h>
#include "prof_cmocka.h"

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

const gchar*
get_log_file_location(void)
{
    return mock_ptr_type(gchar*);
}

int
log_level_from_string(char* log_level, log_level_t* level)
{
    return mock_type(int);
}

void
log_stderr_init(log_level_t level)
{
}
void
log_stderr_handler(void)
{
}
