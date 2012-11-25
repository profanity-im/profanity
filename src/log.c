/*
 * log.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "glib.h"

#include "common.h"
#include "files.h"
#include "log.h"
#include "preferences.h"

#define PROF "prof"

#ifdef _WIN32
// replace 'struct stat' and 'stat()' for windows
#define stat _stat
#endif

static FILE *logp;

static GTimeZone *tz;
static GDateTime *dt;
static log_level_t level_filter;

static void _rotate_log_file(void);

void
log_debug(const char * const msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    log_msg(PROF_LEVEL_DEBUG, PROF, fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
log_info(const char * const msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    log_msg(PROF_LEVEL_INFO, PROF, fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
log_warning(const char * const msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    log_msg(PROF_LEVEL_WARN, PROF, fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
log_error(const char * const msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    log_msg(PROF_LEVEL_ERROR, PROF, fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
log_init(log_level_t filter)
{
    level_filter = filter;
    tz = g_time_zone_new_local();
    gchar *log_file = files_get_log_file();
    logp = fopen(log_file, "a");
    g_free(log_file);
}

log_level_t
log_get_filter(void)
{
    return level_filter;
}

void
log_close(void)
{
    g_time_zone_unref(tz);
    fclose(logp);
}

void
log_msg(log_level_t level, const char * const area, const char * const msg)
{
    if (level >= level_filter) {
        struct stat st;
        int result;
        gchar *log_file = files_get_log_file();
        dt = g_date_time_new_now(tz);

        gchar *date_fmt = g_date_time_format(dt, "%d/%m/%Y %H:%M:%S");
        fprintf(logp, "%s: %s: %s\n", date_fmt, area, msg);
        g_date_time_unref(dt);

        fflush(logp);
        g_free(date_fmt);

        result = stat(log_file, &st);
        if (result == 0 && st.st_size >= prefs_get_max_log_size()) {
            _rotate_log_file();
        }

        g_free(log_file);
    }
}

static void
_rotate_log_file(void)
{
    gchar *log_file = files_get_log_file();
    size_t len = strlen(log_file);
    char *log_file_new = malloc(len + 3);

    strncpy(log_file_new, log_file, len);
    log_file_new[len] = '.';
    log_file_new[len+1] = '1';
    log_file_new[len+2] = 0;

    log_close();
    rename(log_file, log_file_new);
    log_init(log_get_filter());

    free(log_file_new);
    g_free(log_file);
    log_info("Log has been rotated");
}
