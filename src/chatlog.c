/*
 * log.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2018 - 2023 Michael Vetter <jubalh@iodoru.org>
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

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "glib.h"
#include "glib/gstdio.h"

#include "log.h"
#include "chatlog.h"
#include "common.h"
#include "config/files.h"
#include "config/preferences.h"
#include "trace.h"
#include "xmpp/xmpp.h"
#include "xmpp/muc.h"

static GHashTable* logs;
static GHashTable* groupchat_logs;

struct dated_chat_log
{
    gchar* filename;
    GDateTime* date;
};

static gboolean _log_roll_needed(struct dated_chat_log* dated_log);
static struct dated_chat_log* _create_chatlog(const char* const other, const char* const login);
static struct dated_chat_log* _create_groupchat_log(const char* const room, const char* const login);
static void _free_chat_log(struct dated_chat_log* dated_log);
static gboolean _key_equals(void* key1, void* key2);
static void _chat_log_chat(const char* const login, const char* const other, const gchar* const msg,
                           chat_log_direction_t direction, GDateTime* timestamp, const char* const resourcepart);
static void _groupchat_log_chat(const gchar* const login, const gchar* const room, const gchar* const nick,
                                const gchar* const msg);
void
chat_log_init(void)
{
    log_info("Initialising chat logs");
    logs = g_hash_table_new_full(g_str_hash, (GEqualFunc)_key_equals, free,
                                 (GDestroyNotify)_free_chat_log);
}

void
chat_log_msg_out(const char* const barejid, const char* const msg, const char* const resource)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        auto_char char* mybarejid = connection_get_barejid();
        _chat_log_chat(mybarejid, barejid, msg, PROF_OUT_LOG, NULL, resource);
    }
}

void
chat_log_otr_msg_out(const char* const barejid, const char* const msg, const char* const resource)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        auto_char char* mybarejid = connection_get_barejid();
        auto_gchar gchar* pref_otr_log = prefs_get_string(PREF_OTR_LOG);
        if (strcmp(pref_otr_log, "on") == 0) {
            _chat_log_chat(mybarejid, barejid, msg, PROF_OUT_LOG, NULL, resource);
        } else if (strcmp(pref_otr_log, "redact") == 0) {
            _chat_log_chat(mybarejid, barejid, "[redacted]", PROF_OUT_LOG, NULL, resource);
        }
    }
}

void
chat_log_pgp_msg_out(const char* const barejid, const char* const msg, const char* const resource)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        auto_char char* mybarejid = connection_get_barejid();
        auto_gchar gchar* pref_pgp_log = prefs_get_string(PREF_PGP_LOG);
        if (strcmp(pref_pgp_log, "on") == 0) {
            _chat_log_chat(mybarejid, barejid, msg, PROF_OUT_LOG, NULL, resource);
        } else if (strcmp(pref_pgp_log, "redact") == 0) {
            _chat_log_chat(mybarejid, barejid, "[redacted]", PROF_OUT_LOG, NULL, resource);
        }
    }
}

void
chat_log_omemo_msg_out(const char* const barejid, const char* const msg, const char* const resource)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        auto_char char* mybarejid = connection_get_barejid();
        auto_gchar gchar* pref_omemo_log = prefs_get_string(PREF_OMEMO_LOG);
        if (strcmp(pref_omemo_log, "on") == 0) {
            _chat_log_chat(mybarejid, barejid, msg, PROF_OUT_LOG, NULL, resource);
        } else if (strcmp(pref_omemo_log, "redact") == 0) {
            _chat_log_chat(mybarejid, barejid, "[redacted]", PROF_OUT_LOG, NULL, resource);
        }
    }
}

void
chat_log_otr_msg_in(ProfMessage* message)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        auto_char char* mybarejid = connection_get_barejid();
        auto_gchar gchar* pref_otr_log = prefs_get_string(PREF_OTR_LOG);
        if (message->enc == PROF_MSG_ENC_NONE || (strcmp(pref_otr_log, "on") == 0)) {
            if (message->type == PROF_MSG_TYPE_MUCPM) {
                _chat_log_chat(mybarejid, message->from_jid->barejid, message->plain, PROF_IN_LOG, message->timestamp, message->from_jid->resourcepart);
            } else {
                _chat_log_chat(mybarejid, message->from_jid->barejid, message->plain, PROF_IN_LOG, message->timestamp, NULL);
            }
        } else if (strcmp(pref_otr_log, "redact") == 0) {
            if (message->type == PROF_MSG_TYPE_MUCPM) {
                _chat_log_chat(mybarejid, message->from_jid->barejid, "[redacted]", PROF_IN_LOG, message->timestamp, message->from_jid->resourcepart);
            } else {
                _chat_log_chat(mybarejid, message->from_jid->barejid, "[redacted]", PROF_IN_LOG, message->timestamp, NULL);
            }
        }
    }
}

void
chat_log_pgp_msg_in(ProfMessage* message)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        auto_char char* mybarejid = connection_get_barejid();
        auto_gchar gchar* pref_pgp_log = prefs_get_string(PREF_PGP_LOG);
        if (strcmp(pref_pgp_log, "on") == 0) {
            if (message->type == PROF_MSG_TYPE_MUCPM) {
                _chat_log_chat(mybarejid, message->from_jid->barejid, message->plain, PROF_IN_LOG, message->timestamp, message->from_jid->resourcepart);
            } else {
                _chat_log_chat(mybarejid, message->from_jid->barejid, message->plain, PROF_IN_LOG, message->timestamp, NULL);
            }
        } else if (strcmp(pref_pgp_log, "redact") == 0) {
            if (message->type == PROF_MSG_TYPE_MUCPM) {
                _chat_log_chat(mybarejid, message->from_jid->barejid, "[redacted]", PROF_IN_LOG, message->timestamp, message->from_jid->resourcepart);
            } else {
                _chat_log_chat(mybarejid, message->from_jid->barejid, "[redacted]", PROF_IN_LOG, message->timestamp, NULL);
            }
        }
    }
}

void
chat_log_omemo_msg_in(ProfMessage* message)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        auto_char char* mybarejid = connection_get_barejid();
        auto_gchar gchar* pref_omemo_log = prefs_get_string(PREF_OMEMO_LOG);
        if (strcmp(pref_omemo_log, "on") == 0) {
            if (message->type == PROF_MSG_TYPE_MUCPM) {
                _chat_log_chat(mybarejid, message->from_jid->barejid, message->plain, PROF_IN_LOG, message->timestamp, message->from_jid->resourcepart);
            } else {
                _chat_log_chat(mybarejid, message->from_jid->barejid, message->plain, PROF_IN_LOG, message->timestamp, NULL);
            }
        } else if (strcmp(pref_omemo_log, "redact") == 0) {
            if (message->type == PROF_MSG_TYPE_MUCPM) {
                _chat_log_chat(mybarejid, message->from_jid->barejid, "[redacted]", PROF_IN_LOG, message->timestamp, message->from_jid->resourcepart);
            } else {
                _chat_log_chat(mybarejid, message->from_jid->barejid, "[redacted]", PROF_IN_LOG, message->timestamp, message->from_jid->resourcepart);
            }
        }
    }
}

void
chat_log_msg_in(ProfMessage* message)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        auto_char char* mybarejid = connection_get_barejid();

        if (message->type == PROF_MSG_TYPE_MUCPM) {
            _chat_log_chat(mybarejid, message->from_jid->barejid, message->plain, PROF_IN_LOG, message->timestamp, message->from_jid->resourcepart);
        } else {
            _chat_log_chat(mybarejid, message->from_jid->barejid, message->plain, PROF_IN_LOG, message->timestamp, NULL);
        }
    }
}

static void
_chat_log_chat(const char* const login, const char* const other, const char* msg,
               chat_log_direction_t direction, GDateTime* timestamp, const char* const resourcepart)
{
    auto_gchar gchar* pref_dblog = prefs_get_string(PREF_DBLOG);
    if (g_strcmp0(pref_dblog, "redact") == 0) {
        msg = "[REDACTED]";
    }
    char* other_name;
    GString* other_str = NULL;

    if (resourcepart) {
        other_str = g_string_new(other);
        g_string_append(other_str, "_");
        g_string_append(other_str, resourcepart);

        other_name = other_str->str;
    } else {
        other_name = (char*)other;
    }

    struct dated_chat_log* dated_log = g_hash_table_lookup(logs, other_name);

    // no log for user
    if (dated_log == NULL) {
        dated_log = _create_chatlog(other_name, login);
        g_hash_table_insert(logs, strdup(other_name), dated_log);

        // log entry exists but file removed
    } else if (!g_file_test(dated_log->filename, G_FILE_TEST_EXISTS)) {
        dated_log = _create_chatlog(other_name, login);
        g_hash_table_replace(logs, strdup(other_name), dated_log);

        // log file needs rolling
    } else if (_log_roll_needed(dated_log)) {
        dated_log = _create_chatlog(other_name, login);
        g_hash_table_replace(logs, strdup(other_name), dated_log);
    }

    if (resourcepart) {
        g_string_free(other_str, TRUE);
    }

    if (timestamp == NULL) {
        timestamp = g_date_time_new_now_local();
    } else {
        g_date_time_ref(timestamp);
    }

    auto_gchar gchar* date_fmt = g_date_time_format_iso8601(timestamp);
    FILE* chatlogp = fopen(dated_log->filename, "a");
    g_chmod(dated_log->filename, S_IRUSR | S_IWUSR);
    if (chatlogp) {
        if (direction == PROF_IN_LOG) {
            if (strncmp(msg, "/me ", 4) == 0) {
                if (resourcepart) {
                    fprintf(chatlogp, "%s - *%s %s\n", date_fmt, resourcepart, msg + 4);
                } else {
                    fprintf(chatlogp, "%s - *%s %s\n", date_fmt, other, msg + 4);
                }
            } else {
                if (resourcepart) {
                    fprintf(chatlogp, "%s - %s: %s\n", date_fmt, resourcepart, msg);
                } else {
                    fprintf(chatlogp, "%s - %s: %s\n", date_fmt, other, msg);
                }
            }
        } else {
            if (strncmp(msg, "/me ", 4) == 0) {
                fprintf(chatlogp, "%s - *me %s\n", date_fmt, msg + 4);
            } else {
                fprintf(chatlogp, "%s - me: %s\n", date_fmt, msg);
            }
        }
        fflush(chatlogp);
        int result = fclose(chatlogp);
        if (result == EOF) {
            log_error("Error closing file %s, errno = %d", dated_log->filename, errno);
        }
    }

    g_date_time_unref(timestamp);
}

void
groupchat_log_init(void)
{
    log_info("Initialising groupchat logs");
    groupchat_logs = g_hash_table_new_full(g_str_hash, (GEqualFunc)_key_equals, free,
                                           (GDestroyNotify)_free_chat_log);
}

void
groupchat_log_msg_out(const gchar* const room, const gchar* const msg)
{
    if (prefs_get_boolean(PREF_GRLOG)) {
        auto_char char* mybarejid = connection_get_barejid();
        char* mynick = muc_nick(room);
        _groupchat_log_chat(mybarejid, room, mynick, msg);
    }
}

void
groupchat_log_msg_in(const gchar* const room, const gchar* const nick, const gchar* const msg)
{
    if (prefs_get_boolean(PREF_GRLOG)) {
        auto_char char* mybarejid = connection_get_barejid();
        _groupchat_log_chat(mybarejid, room, nick, msg);
    }
}

void
groupchat_log_omemo_msg_out(const gchar* const room, const gchar* const msg)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        auto_char char* mybarejid = connection_get_barejid();
        auto_gchar gchar* pref_omemo_log = prefs_get_string(PREF_OMEMO_LOG);
        char* mynick = muc_nick(room);

        if (strcmp(pref_omemo_log, "on") == 0) {
            _groupchat_log_chat(mybarejid, room, mynick, msg);
        } else if (strcmp(pref_omemo_log, "redact") == 0) {
            _groupchat_log_chat(mybarejid, room, mynick, "[redacted]");
        }
    }
}

void
groupchat_log_omemo_msg_in(const gchar* const room, const gchar* const nick, const gchar* const msg)
{
    if (prefs_get_boolean(PREF_CHLOG)) {
        auto_char char* mybarejid = connection_get_barejid();
        auto_gchar gchar* pref_omemo_log = prefs_get_string(PREF_OMEMO_LOG);

        if (strcmp(pref_omemo_log, "on") == 0) {
            _groupchat_log_chat(mybarejid, room, nick, msg);
        } else if (strcmp(pref_omemo_log, "redact") == 0) {
            _groupchat_log_chat(mybarejid, room, nick, "[redacted]");
        }
    }
}

void
_groupchat_log_chat(const gchar* const login, const gchar* const room, const gchar* const nick,
                    const gchar* const msg)
{
    struct dated_chat_log* dated_log = g_hash_table_lookup(groupchat_logs, room);

    // no log for room
    if (dated_log == NULL) {
        dated_log = _create_groupchat_log(room, login);
        g_hash_table_insert(groupchat_logs, strdup(room), dated_log);

        // log exists but needs rolling
    } else if (_log_roll_needed(dated_log)) {
        dated_log = _create_groupchat_log(room, login);
        g_hash_table_replace(logs, strdup(room), dated_log);
    }

    GDateTime* dt_tmp = g_date_time_new_now_local();

    auto_gchar gchar* date_fmt = g_date_time_format_iso8601(dt_tmp);

    FILE* grpchatlogp = fopen(dated_log->filename, "a");
    g_chmod(dated_log->filename, S_IRUSR | S_IWUSR);
    if (grpchatlogp) {
        if (strncmp(msg, "/me ", 4) == 0) {
            fprintf(grpchatlogp, "%s - *%s %s\n", date_fmt, nick, msg + 4);
        } else {
            fprintf(grpchatlogp, "%s - %s: %s\n", date_fmt, nick, msg);
        }

        fflush(grpchatlogp);
        int result = fclose(grpchatlogp);
        if (result == EOF) {
            log_error("Error closing file %s, errno = %d", dated_log->filename, errno);
        }
    }

    g_date_time_unref(dt_tmp);
}

void
chat_log_close(void)
{
    g_hash_table_destroy(logs);
    g_hash_table_destroy(groupchat_logs);
}

static char*
_get_log_filename(const char* const other, const char* const login, GDateTime* dt, gboolean is_room)
{
    auto_gchar gchar* chatlogs_dir = files_file_in_account_data_path(DIR_CHATLOGS, login, is_room ? "rooms" : NULL);
    auto_gchar gchar* logfile_name = g_date_time_format(dt, "%Y_%m_%d.log");
    auto_char char* other_ = str_replace(other, "@", "_at_");
    auto_gchar gchar* logs_path = g_strdup_printf("%s/%s", chatlogs_dir, other_);
    gchar* logfile_path = NULL;

    if (create_dir(logs_path)) {
        logfile_path = g_strdup_printf("%s/%s", logs_path, logfile_name);
    }

    return logfile_path;
}

static struct dated_chat_log*
_create_chatlog(const char* const other, const char* const login)
{
    GDateTime* now = g_date_time_new_now_local();
    auto_char char* filename = _get_log_filename(other, login, now, FALSE);

    struct dated_chat_log* new_log = malloc(sizeof(struct dated_chat_log));
    new_log->filename = strdup(filename);
    new_log->date = now;

    return new_log;
}

static struct dated_chat_log*
_create_groupchat_log(const char* const room, const char* const login)
{
    GDateTime* now = g_date_time_new_now_local();
    auto_char char* filename = _get_log_filename(room, login, now, TRUE);

    struct dated_chat_log* new_log = malloc(sizeof(struct dated_chat_log));
    new_log->filename = strdup(filename);
    new_log->date = now;

    return new_log;
}

static gboolean
_log_roll_needed(struct dated_chat_log* dated_log)
{
    gboolean result = FALSE;
    GDateTime* now = g_date_time_new_now_local();
    if (g_date_time_get_day_of_year(dated_log->date) != g_date_time_get_day_of_year(now)) {
        result = TRUE;
    }
    g_date_time_unref(now);

    return result;
}

static void
_free_chat_log(struct dated_chat_log* dated_log)
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

static gboolean
_key_equals(void* key1, void* key2)
{
    gchar* str1 = (gchar*)key1;
    gchar* str2 = (gchar*)key2;

    return (g_strcmp0(str1, str2) == 0);
}
