/*
 * mock_log.c
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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
