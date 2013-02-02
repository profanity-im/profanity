/*
 * log.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "glib.h"

#include "log.h"

#include "common.h"
#include "config/preferences.h"
#include "files.h"

#define PROF "prof"

#ifdef _WIN32
// replace 'struct stat' and 'stat()' for windows
#define stat _stat
#endif

static FILE *logp;

static GTimeZone *tz;
static GDateTime *dt;
static log_level_t level_filter;

static GHashTable *logs;
static GDateTime *session_started;

struct dated_chat_log {
    gchar *filename;
    GDateTime *date;
};

static gboolean _log_roll_needed(struct dated_chat_log *dated_log);
static struct dated_chat_log *_create_log(char *other, const  char * const login);
static void _free_chat_log(struct dated_chat_log *dated_log);
static gboolean _key_equals(void *key1, void *key2);
static char * _get_log_filename(const char * const other, const char * const login,
    GDateTime *dt, gboolean create);
static gchar * _get_chatlog_dir(void);

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

void
chat_log_init(void)
{
    session_started = g_date_time_new_now_local();
    log_info("Initialising chat logs");
    logs = g_hash_table_new_full(g_str_hash, (GEqualFunc) _key_equals, g_free,
        (GDestroyNotify)_free_chat_log);
}

void
chat_log_chat(const gchar * const login, gchar *other,
    const gchar * const msg, chat_log_direction_t direction, GTimeVal *tv_stamp)
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

    gchar *date_fmt = NULL;
    GDateTime *dt = NULL;
    if (tv_stamp == NULL) {
        dt = g_date_time_new_now_local();
    } else {
        dt = g_date_time_new_from_timeval_utc(tv_stamp);
    }

    date_fmt = g_date_time_format(dt, "%H:%M:%S");

    FILE *logp = fopen(dated_log->filename, "a");

    if (direction == PROF_IN_LOG) {
        if (strncmp(msg, "/me ", 4) == 0) {
            fprintf(logp, "%s - *%s %s\n", date_fmt, other_copy, msg + 4);
        } else {
            fprintf(logp, "%s - %s: %s\n", date_fmt, other_copy, msg);
        }
    } else {
        if (strncmp(msg, "/me ", 4) == 0) {
            fprintf(logp, "%s - *me %s\n", date_fmt, msg + 4);
        } else {
            fprintf(logp, "%s - me: %s\n", date_fmt, msg);
        }
    }
    fflush(logp);
    int result = fclose(logp);
    if (result == EOF) {
        log_error("Error closing file %s, errno = %d", dated_log->filename, errno);
    }

    g_free(date_fmt);
    g_date_time_unref(dt);
}

GSList *
chat_log_get_previous(const gchar * const login, const gchar * const recipient,
    GSList *history)
{
    GTimeZone *tz = g_time_zone_new_local();

    GDateTime *now = g_date_time_new_now_local();
    GDateTime *log_date = g_date_time_new(tz,
        g_date_time_get_year(session_started),
        g_date_time_get_month(session_started),
        g_date_time_get_day_of_month(session_started),
        g_date_time_get_hour(session_started),
        g_date_time_get_minute(session_started),
        g_date_time_get_second(session_started));

    // get data from all logs from the day the session was started to today
    while (g_date_time_compare(log_date, now) != 1) {
        char *filename = _get_log_filename(recipient, login, log_date, FALSE);

        FILE *logp = fopen(filename, "r");
        char *line;
        if (logp != NULL) {
            GString *gs_header = g_string_new("");
            g_string_append_printf(gs_header, "%d/%d/%d:",
                g_date_time_get_day_of_month(log_date),
                g_date_time_get_month(log_date),
                g_date_time_get_year(log_date));
            char *header = strdup(gs_header->str);
            history = g_slist_append(history, header);
            g_string_free(gs_header, TRUE);

            while ((line = prof_getline(logp)) != NULL) {
                history = g_slist_append(history, line);
            }

            fclose(logp);
        }

        free(filename);
        GDateTime *next = g_date_time_add_days(log_date, 1);
        g_date_time_unref(log_date);
        log_date = g_date_time_ref(next);
    }

    g_time_zone_unref(tz);

    return history;
}

void
chat_log_close(void)
{
    g_hash_table_remove_all(logs);
    g_date_time_unref(session_started);
}

static struct dated_chat_log *
_create_log(char *other, const char * const login)
{
    GDateTime *now = g_date_time_new_now_local();
    char *filename = _get_log_filename(other, login, now, TRUE);

    struct dated_chat_log *new_log = malloc(sizeof(struct dated_chat_log));
    new_log->filename = strdup(filename);
    new_log->date = now;

    free(filename);

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
        if (dated_log->filename != NULL) {
            g_free(dated_log->filename);
            dated_log->filename = NULL;
        }
        if (dated_log->date != NULL) {
            g_date_time_unref(dated_log->date);
            dated_log->date = NULL;
        }
    }
    dated_log = NULL;
}

static
gboolean _key_equals(void *key1, void *key2)
{
    gchar *str1 = (gchar *) key1;
    gchar *str2 = (gchar *) key2;

    return (g_strcmp0(str1, str2) == 0);
}

static char *
_get_log_filename(const char * const other, const char * const login,
    GDateTime *dt, gboolean create)
{
    gchar *chatlogs_dir = _get_chatlog_dir();
    GString *log_file = g_string_new(chatlogs_dir);
    g_free(chatlogs_dir);

    gchar *login_dir = str_replace(login, "@", "_at_");
    g_string_append_printf(log_file, "/%s", login_dir);
    if (create) {
        create_dir(log_file->str);
    }
    free(login_dir);

    gchar *other_file = str_replace(other, "@", "_at_");
    g_string_append_printf(log_file, "/%s", other_file);
    if (create) {
        create_dir(log_file->str);
    }
    free(other_file);

    gchar *date = g_date_time_format(dt, "/%Y_%m_%d.log");
    g_string_append(log_file, date);

    char *result = strdup(log_file->str);
    g_string_free(log_file, TRUE);

    return result;
}

static gchar *
_get_chatlog_dir(void)
{
    gchar *xdg_data = xdg_get_data_home();
    GString *chatlogs_dir = g_string_new(xdg_data);
    g_string_append(chatlogs_dir, "/profanity/chatlogs");
    gchar *result = strdup(chatlogs_dir->str);
    g_free(xdg_data);
    g_string_free(chatlogs_dir, TRUE);

    return result;
}

