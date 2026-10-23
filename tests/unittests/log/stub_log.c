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
log_init(log_level_t filter, const gchar* const log_file)
{
}
log_level_t
log_get_filter(void)
{
    return mock_type(log_level_t);
}

void
log_set_filter(log_level_t filter)
{
}

void
log_close(void)
{
}
void
log_debug(const gchar* const msg, ...)
{
}
void
log_info(const gchar* const msg, ...)
{
}
void
log_warning(const gchar* const msg, ...)
{
}
void
log_error(const gchar* const msg, ...)
{
}
void
log_msg(log_level_t level, const gchar* const area, const gchar* const msg)
{
}

const gchar*
get_log_file_location(void)
{
    return mock_ptr_type(gchar*);
}

int
log_level_from_string(const gchar* const log_level, log_level_t* level)
{
    return mock_type(int);
}

const gchar*
log_string_from_level(log_level_t level)
{
    return "INFO";
}

void
log_stderr_init(log_level_t level)
{
}
void
log_stderr_handler(void)
{
}
