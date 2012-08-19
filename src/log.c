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

#include "glib.h"

#include "log.h"
#include "common.h"

static FILE *logp;

static GTimeZone *tz;
static GDateTime *dt;
static log_level_t prof_log_level;

void
log_msg(log_level_t level, const char * const area, const char * const msg)
{
    if (level >= prof_log_level) {
        dt = g_date_time_new_now(tz);                                      
        gchar *date_fmt = g_date_time_format(dt, "%d/%m/%Y %H:%M:%S");               
        fprintf(logp, "%s: %s: %s\n", date_fmt, area, msg);
        g_date_time_unref(dt);
    }
}

void
log_init(log_level_t log_level)
{
    prof_log_level = log_level;
    tz = g_time_zone_new_local();
    GString *log_file = g_string_new(getenv("HOME"));
    g_string_append(log_file, "/.profanity/log");
    create_dir(log_file->str);
    g_string_append(log_file, "/profanity.log");
    logp = fopen(log_file->str, "a");
    g_string_free(log_file, TRUE);
    log_msg(PROF_LEVEL_INFO, PROF, "Starting Profanity...");
}

log_level_t
log_get_level(void)
{
    return prof_log_level;
}

void
log_close(void)
{
    g_time_zone_unref(tz);
    fclose(logp);
}

