/* 
 * chat_log.c
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

#include "glib.h"

#include "chat_log.h"
#include "common.h"
#include "log.h"

static GHashTable *logs;

struct dated_chat_log {
    FILE *logp;
    GDateTime *date;
};

static gboolean _log_roll_needed(struct dated_chat_log *dated_log);
static struct dated_chat_log *_create_log(char *other, const  char * const login);
static void _free_chat_log(struct dated_chat_log *dated_log);

void
chat_log_init(void)
{
    log_info("Initialising chat logs");
    logs = g_hash_table_new_full(NULL, (GEqualFunc) g_strcmp0, g_free, 
        (GDestroyNotify)_free_chat_log);
}

void
chat_log_chat(const gchar * const login, gchar *other, 
    const gchar * const msg, chat_log_direction_t direction)
{
    gchar *other_copy = strdup(other);
    struct dated_chat_log *dated_log = g_hash_table_lookup(logs, other_copy);
    
    // no log for user
    if (dated_log == NULL) {
        dated_log = _create_log(other_copy, login);
        g_hash_table_insert(logs, other_copy, dated_log);

    // log exists but needs rolling
    } else if (_log_roll_needed(dated_log)) {
        dated_log = _create_log(other_copy, login);
        g_hash_table_replace(logs, other_copy, dated_log);
    }

    GDateTime *dt = g_date_time_new_now_local();
    gchar *date_fmt = g_date_time_format(dt, "%H:%M:%S");

    if (direction == IN) {
        fprintf(dated_log->logp, "%s - %s: %s\n", date_fmt, other_copy, msg);
    } else {
        fprintf(dated_log->logp, "%s - me: %s\n", date_fmt, msg);
    }
    fflush(dated_log->logp);
    
    g_free(date_fmt);
    g_date_time_unref(dt);
}

void
chat_log_close(void)
{
    g_hash_table_remove_all(logs);
}

static struct dated_chat_log *
_create_log(char *other, const char * const login)
{
    GString *log_file = g_string_new(getenv("HOME"));
    g_string_append(log_file, "/.profanity/log");
    create_dir(log_file->str);

    gchar *login_dir = str_replace(login, "@", "_at_");
    g_string_append_printf(log_file, "/%s", login_dir);
    create_dir(log_file->str);
    free(login_dir);

    gchar *other_file = str_replace(other, "@", "_at_");
    g_string_append_printf(log_file, "/%s", other_file);
    create_dir(log_file->str);
    free(other_file);

    GDateTime *dt = g_date_time_new_now_local();
    gchar *date = g_date_time_format(dt, "/%Y_%m_%d.log");
    g_string_append_printf(log_file, date);

    struct dated_chat_log *new_log = malloc(sizeof(struct dated_chat_log));
    new_log->logp = fopen(log_file->str, "a");
    new_log->date = dt;

    g_string_free(log_file, TRUE);

    return new_log;
}


static gboolean
_log_roll_needed(struct dated_chat_log *dated_log)
{
    gboolean result = FALSE;
    GDateTime *now = g_date_time_new_now_local();
    if (g_date_time_get_day_of_year(dated_log->date) !=
            g_date_time_get_day_of_year(now)) {
        result = TRUE;
    }
    g_date_time_unref(now);

    return result;
}

static void
_free_chat_log(struct dated_chat_log *dated_log)
{
    if (dated_log != NULL) {
        if (dated_log->logp != NULL) {
            fclose(dated_log->logp);
            dated_log->logp = NULL;
        }
        if (dated_log->date != NULL) {
            g_date_time_unref(dated_log->date);
            dated_log->date = NULL;
        }
    }
    dated_log = NULL;
}
