/*
 * log.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "glib.h"
#include "glib/gstdio.h"

#include "log.h"
#include "common.h"
#include "config/files.h"
#include "config/preferences.h"
#include "xmpp/xmpp.h"

#define PROF "prof"

static FILE *logp;
GString *mainlogfile;

static GTimeZone *tz;
static GDateTime *dt;
static log_level_t level_filter;

static GHashTable *logs;
static GHashTable *groupchat_logs;
static GDateTime *session_started;

enum {
    STDERR_BUFSIZE = 4000,
    STDERR_RETRY_NR = 5,
};
static int stderr_inited;
static log_level_t stderr_level;
static int stderr_pipe[2];
static char *stderr_buf;
static GString *stderr_msg;

struct dated_chat_log {
    gchar *filename;
    GDateTime *date;
};

static gboolean _log_roll_needed(struct dated_chat_log *dated_log);
static struct dated_chat_log* _create_log(const char *const other, const char *const login);
static struct dated_chat_log* _create_groupchat_log(const char *const room, const char *const login);
static void _free_chat_log(struct dated_chat_log *dated_log);
static gboolean _key_equals(void *key1, void *key2);
static char* _get_log_filename(const char *const other, const char *const login, GDateTime *dt, gboolean create);
static char* _get_groupchat_log_filename(const char *const room, const char *const login, GDateTime *dt,
    gboolean create);
static void _rotate_log_file(void);
static char* _log_string_from_level(log_level_t level);
static void _chat_log_chat(const char *const login, const char *const other, const gchar *const msg,
    chat_log_direction_t direction, GDateTime *timestamp);

void
log_debug(const char *const msg, ...)
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
log_info(const char *const msg, ...)
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
log_warning(const char *const msg, ...)
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
log_error(const char *const msg, ...)
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
    char *log_file = files_get_log_file();
    logp = fopen(log_file, "a");
    g_chmod(log_file, S_IRUSR | S_IWUSR);
    mainlogfile = g_string_new(log_file);
    free(log_file);
}

void
log_reinit(void)
{
    log_close();
    log_init(level_filter);
}

char*
get_log_file_location(void)
{
    return mainlogfile->str;
}

log_level_t
log_get_filter(void)
{
    return level_filter;
}

void
log_close(void)
{
    g_string_free(mainlogfile, TRUE);
    g_time_zone_unref(tz);
    if (logp) {
        fclose(logp);
    }
}

void
log_msg(log_level_t level, const char *const area, const char *const msg)
{
    if (level >= level_filter && logp) {
        dt = g_date_time_new_now(tz);

        char *level_str = _log_string_from_level(level);

        gchar *date_fmt = g_date_time_format(dt, "%d/%m/%Y %H:%M:%S");

        fprintf(logp, "%s: %s: %s: %s\n", date_fmt, area, level_str, msg);
        g_date_time_unref(dt);

        fflush(logp);
        g_free(date_fmt);

        if (prefs_get_boolean(PREF_LOG_ROTATE)) {
            long result = ftell(logp);
            if (result != -1 && result >= prefs_get_max_log_size()) {
                _rotate_log_file();
            }
        }
    }
}

log_level_t
log_level_from_string(char *log_level)
{
    assert(log_level != NULL);
    if (strcmp(log_level, "DEBUG") == 0) {
        return PROF_LEVEL_DEBUG;
    } else if (strcmp(log_level, "INFO") == 0) {
        return PROF_LEVEL_INFO;
    } else if (strcmp(log_level, "WARN") == 0) {
        return PROF_LEVEL_WARN;
    } else if (strcmp(log_level, "ERROR") == 0) {
        return PROF_LEVEL_ERROR;
    } else { // default to info
        return PROF_LEVEL_INFO;
    }
}

static void
_rotate_log_file(void)
{
    char *log_file = files_get_log_file();
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
    free(log_file);
    log_info("Log has been rotated");
}

void
chat_log_init(void)
{
    session_started = g_date_time_new_now_local();
    log_info("Initialising chat logs");
    logs = g_hash_table_new_full(g_str_hash, (GEqualFunc) _key_equals, free,
        (GDestroyNotify)_free_chat_log);
}

void
groupchat_log_init(void)
{
    log_info("Initialising groupchat logs");
    groupchat_logs = g_hash_table_new_full(g_str_hash, (GEqualFunc) _key_equals, free,
        (GDestroyNotify)_free_chat_log);
}

void
chat_log_msg_out(const char *const barejid, const char *const msg)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        const char *jid = connection_get_fulljid();
        Jid *jidp = jid_create(jid);
        _chat_log_chat(jidp->barejid, barejid, msg, PROF_OUT_LOG, NULL);
        jid_destroy(jidp);
    }
}

void
chat_log_otr_msg_out(const char *const barejid, const char *const msg)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        const char *jid = connection_get_fulljid();
        Jid *jidp = jid_create(jid);
        char *pref_otr_log = prefs_get_string(PREF_OTR_LOG);
        if (strcmp(pref_otr_log, "on") == 0) {
            _chat_log_chat(jidp->barejid, barejid, msg, PROF_OUT_LOG, NULL);
        } else if (strcmp(pref_otr_log, "redact") == 0) {
            _chat_log_chat(jidp->barejid, barejid, "[redacted]", PROF_OUT_LOG, NULL);
        }
        prefs_free_string(pref_otr_log);
        jid_destroy(jidp);
    }
}

void
chat_log_pgp_msg_out(const char *const barejid, const char *const msg)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        const char *jid = connection_get_fulljid();
        Jid *jidp = jid_create(jid);
        char *pref_pgp_log = prefs_get_string(PREF_PGP_LOG);
        if (strcmp(pref_pgp_log, "on") == 0) {
            _chat_log_chat(jidp->barejid, barejid, msg, PROF_OUT_LOG, NULL);
        } else if (strcmp(pref_pgp_log, "redact") == 0) {
            _chat_log_chat(jidp->barejid, barejid, "[redacted]", PROF_OUT_LOG, NULL);
        }
        prefs_free_string(pref_pgp_log);
        jid_destroy(jidp);
    }
}

void
chat_log_otr_msg_in(const char *const barejid, const char *const msg, gboolean was_decrypted, GDateTime *timestamp)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        const char *jid = connection_get_fulljid();
        Jid *jidp = jid_create(jid);
        char *pref_otr_log = prefs_get_string(PREF_OTR_LOG);
        if (!was_decrypted || (strcmp(pref_otr_log, "on") == 0)) {
            _chat_log_chat(jidp->barejid, barejid, msg, PROF_IN_LOG, timestamp);
        } else if (strcmp(pref_otr_log, "redact") == 0) {
            _chat_log_chat(jidp->barejid, barejid, "[redacted]", PROF_IN_LOG, timestamp);
        }
        prefs_free_string(pref_otr_log);
        jid_destroy(jidp);
    }
}

void
chat_log_pgp_msg_in(const char *const barejid, const char *const msg, GDateTime *timestamp)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        const char *jid = connection_get_fulljid();
        Jid *jidp = jid_create(jid);
        char *pref_pgp_log = prefs_get_string(PREF_PGP_LOG);
        if (strcmp(pref_pgp_log, "on") == 0) {
            _chat_log_chat(jidp->barejid, barejid, msg, PROF_IN_LOG, timestamp);
        } else if (strcmp(pref_pgp_log, "redact") == 0) {
            _chat_log_chat(jidp->barejid, barejid, "[redacted]", PROF_IN_LOG, timestamp);
        }
        prefs_free_string(pref_pgp_log);
        jid_destroy(jidp);
    }
}

void
chat_log_msg_in(const char *const barejid, const char *const msg, GDateTime *timestamp)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        const char *jid = connection_get_fulljid();
        Jid *jidp = jid_create(jid);
        _chat_log_chat(jidp->barejid, barejid, msg, PROF_IN_LOG, timestamp);
        jid_destroy(jidp);
    }
}

static void
_chat_log_chat(const char *const login, const char *const other, const char *const msg,
    chat_log_direction_t direction, GDateTime *timestamp)
{
    struct dated_chat_log *dated_log = g_hash_table_lookup(logs, other);

    // no log for user
    if (dated_log == NULL) {
        dated_log = _create_log(other, login);
        g_hash_table_insert(logs, strdup(other), dated_log);

    // log entry exists but file removed
    } else if (!g_file_test(dated_log->filename, G_FILE_TEST_EXISTS)) {
        dated_log = _create_log(other, login);
        g_hash_table_replace(logs, strdup(other), dated_log);

    // log file needs rolling
    } else if (_log_roll_needed(dated_log)) {
        dated_log = _create_log(other, login);
        g_hash_table_replace(logs, strdup(other), dated_log);
    }

    if (timestamp == NULL) {
        timestamp = g_date_time_new_now_local();
    } else {
        g_date_time_ref(timestamp);
    }

    gchar *date_fmt = g_date_time_format(timestamp, "%H:%M:%S");
    FILE *logp = fopen(dated_log->filename, "a");
    g_chmod(dated_log->filename, S_IRUSR | S_IWUSR);
    if (logp) {
        if (direction == PROF_IN_LOG) {
            if (strncmp(msg, "/me ", 4) == 0) {
                fprintf(logp, "%s - *%s %s\n", date_fmt, other, msg + 4);
            } else {
                fprintf(logp, "%s - %s: %s\n", date_fmt, other, msg);
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
    }

    g_free(date_fmt);
    g_date_time_unref(timestamp);
}

void
groupchat_log_chat(const gchar *const login, const gchar *const room, const gchar *const nick, const gchar *const msg)
{
    struct dated_chat_log *dated_log = g_hash_table_lookup(groupchat_logs, room);

    // no log for room
    if (dated_log == NULL) {
        dated_log = _create_groupchat_log(room, login);
        g_hash_table_insert(groupchat_logs, strdup(room), dated_log);

    // log exists but needs rolling
    } else if (_log_roll_needed(dated_log)) {
        dated_log = _create_groupchat_log(room, login);
        g_hash_table_replace(logs, strdup(room), dated_log);
    }

    GDateTime *dt = g_date_time_new_now_local();

    gchar *date_fmt = g_date_time_format(dt, "%H:%M:%S");

    FILE *logp = fopen(dated_log->filename, "a");
    g_chmod(dated_log->filename, S_IRUSR | S_IWUSR);
    if (logp) {
        if (strncmp(msg, "/me ", 4) == 0) {
            fprintf(logp, "%s - *%s %s\n", date_fmt, nick, msg + 4);
        } else {
            fprintf(logp, "%s - %s: %s\n", date_fmt, nick, msg);
        }

        fflush(logp);
        int result = fclose(logp);
        if (result == EOF) {
            log_error("Error closing file %s, errno = %d", dated_log->filename, errno);
        }
    }

    g_free(date_fmt);
    g_date_time_unref(dt);
}


GSList*
chat_log_get_previous(const gchar *const login, const gchar *const recipient)
{
    GSList *history = NULL;
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
        if (logp) {
            GString *header = g_string_new("");
            g_string_append_printf(header, "%d/%d/%d:",
                g_date_time_get_day_of_month(log_date),
                g_date_time_get_month(log_date),
                g_date_time_get_year(log_date));
            history = g_slist_append(history, header->str);
            g_string_free(header, FALSE);

            char *line;
            while ((line = file_getline(logp)) != NULL) {
                history = g_slist_append(history, line);
            }

            fclose(logp);
        }

        free(filename);

        GDateTime *next = g_date_time_add_days(log_date, 1);
        g_date_time_unref(log_date);
        log_date = next;
    }

    g_date_time_unref(log_date);
    g_date_time_unref(now);

    return history;
}

void
chat_log_close(void)
{
    g_hash_table_destroy(logs);
    g_hash_table_destroy(groupchat_logs);
    g_date_time_unref(session_started);
}

static struct dated_chat_log*
_create_log(const char *const other, const char *const login)
{
    GDateTime *now = g_date_time_new_now_local();
    char *filename = _get_log_filename(other, login, now, TRUE);

    struct dated_chat_log *new_log = malloc(sizeof(struct dated_chat_log));
    new_log->filename = strdup(filename);
    new_log->date = now;

    free(filename);

    return new_log;
}

static struct dated_chat_log*
_create_groupchat_log(const char * const room, const char *const login)
{
    GDateTime *now = g_date_time_new_now_local();
    char *filename = _get_groupchat_log_filename(room, login, now, TRUE);

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
    if (dated_log) {
        if (dated_log->filename) {
            g_free(dated_log->filename);
            dated_log->filename = NULL;
        }
        if (dated_log->date) {
            g_date_time_unref(dated_log->date);
            dated_log->date = NULL;
        }
        free(dated_log);
    }
}

static
gboolean _key_equals(void *key1, void *key2)
{
    gchar *str1 = (gchar *) key1;
    gchar *str2 = (gchar *) key2;

    return (g_strcmp0(str1, str2) == 0);
}

static char*
_get_log_filename(const char *const other, const char *const login, GDateTime *dt, gboolean create)
{
    char *chatlogs_dir = files_get_data_path(DIR_CHATLOGS);
    GString *log_file = g_string_new(chatlogs_dir);
    free(chatlogs_dir);

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
    g_free(date);

    char *result = strdup(log_file->str);
    g_string_free(log_file, TRUE);

    return result;
}

static char*
_get_groupchat_log_filename(const char *const room, const char *const login, GDateTime *dt, gboolean create)
{
    char *chatlogs_dir = files_get_data_path(DIR_CHATLOGS);
    GString *log_file = g_string_new(chatlogs_dir);
    free(chatlogs_dir);

    gchar *login_dir = str_replace(login, "@", "_at_");
    g_string_append_printf(log_file, "/%s", login_dir);
    if (create) {
        create_dir(log_file->str);
    }
    free(login_dir);

    g_string_append(log_file, "/rooms");
    if (create) {
        create_dir(log_file->str);
    }

    gchar *room_file = str_replace(room, "@", "_at_");
    g_string_append_printf(log_file, "/%s", room_file);
    if (create) {
        create_dir(log_file->str);
    }
    free(room_file);

    gchar *date = g_date_time_format(dt, "/%Y_%m_%d.log");
    g_string_append(log_file, date);
    g_free(date);

    char *result = strdup(log_file->str);
    g_string_free(log_file, TRUE);

    return result;
}

static char*
_log_string_from_level(log_level_t level)
{
    switch (level)
    {
        case PROF_LEVEL_ERROR:
            return "ERR";
        case PROF_LEVEL_WARN:
            return "WRN";
        case PROF_LEVEL_INFO:
            return "INF";
        case PROF_LEVEL_DEBUG:
            return "DBG";
        default:
            return "LOG";
    }
}

void
log_stderr_handler(void)
{
    GString * const s = stderr_msg;
    char * const buf = stderr_buf;
    ssize_t size;
    int retry = 0;
    int i;

    if (!stderr_inited)
        return;

    do {
        size = read(stderr_pipe[0], buf, STDERR_BUFSIZE);
        if (size == -1 && errno == EINTR && retry++ < STDERR_RETRY_NR)
            continue;
        if (size <= 0 || retry++ >= STDERR_RETRY_NR)
            break;

        for (i = 0; i < size; ++i) {
            if (buf[i] == '\n') {
                log_msg(stderr_level, "stderr", s->str);
                g_string_assign(s, "");
            } else
                g_string_append_c(s, buf[i]);
        }
    } while (1);

    if (s->len > 0 && s->str[0] != '\0') {
        log_msg(stderr_level, "stderr", s->str);
        g_string_assign(s, "");
    }
}

void
log_stderr_init(log_level_t level)
{
    int rc;
    int flags;

    rc = pipe(stderr_pipe);
    if (rc != 0)
        goto err;

    flags = fcntl(stderr_pipe[0], F_GETFL);
    rc = fcntl(stderr_pipe[0], F_SETFL, flags | O_NONBLOCK);
    if (rc != 0)
        goto err_close;

    close(STDERR_FILENO);
    rc = dup2(stderr_pipe[1], STDERR_FILENO);
    if (rc < 0)
        goto err_close;

    stderr_buf = malloc(STDERR_BUFSIZE);
    stderr_msg = g_string_sized_new(STDERR_BUFSIZE);
    stderr_level = level;
    stderr_inited = 1;

    if (stderr_buf == NULL || stderr_msg == NULL) {
        errno = ENOMEM;
        goto err_free;
    }
    return;

err_free:
    if (stderr_msg != NULL)
        g_string_free(stderr_msg, TRUE);
    free(stderr_buf);
err_close:
    close(stderr_pipe[0]);
    close(stderr_pipe[1]);
err:
    stderr_inited = 0;
    log_error("Unable to init stderr log handler: %s", strerror(errno));
}

void
log_stderr_close(void)
{
    if (!stderr_inited)
        return;

    /* handle remaining logs before close */
    log_stderr_handler();
    stderr_inited = 0;
    free(stderr_buf);
    g_string_free(stderr_msg, TRUE);
    close(stderr_pipe[0]);
    close(stderr_pipe[1]);
}
