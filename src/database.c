/*
 * database.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 - 2023 Michael Vetter <jubalh@iodoru.org>
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

#include <sys/stat.h>
#include <sqlite3.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "log.h"
#include "common.h"
#include "config/files.h"
#include "database.h"
#include "config/preferences.h"
#include "xmpp/xmpp.h"
#include "xmpp/message.h"

static sqlite3* g_chatlog_database;

static void _add_to_db(ProfMessage* message, char* type, const Jid* const from_jid, const Jid* const to_jid);
static char* _get_db_filename(ProfAccount* account);
static prof_msg_type_t _get_message_type_type(const char* const type);
static prof_enc_t _get_message_enc_type(const char* const encstr);

#define auto_sqlite __attribute__((__cleanup__(auto_free_sqlite)))

static void
auto_free_sqlite(gchar** str)
{
    if (str == NULL)
        return;
    sqlite3_free(*str);
}

static char*
_get_db_filename(ProfAccount* account)
{
    return files_file_in_account_data_path(DIR_DATABASE, account->jid, "chatlog.db");
}

gboolean
log_database_init(ProfAccount* account)
{
    int ret = sqlite3_initialize();
    if (ret != SQLITE_OK) {
        log_error("Error initializing SQLite database: %d", ret);
        return FALSE;
    }

    auto_char char* filename = _get_db_filename(account);
    if (!filename) {
        return FALSE;
    }

    ret = sqlite3_open(filename, &g_chatlog_database);
    if (ret != SQLITE_OK) {
        const char* err_msg = sqlite3_errmsg(g_chatlog_database);
        log_error("Error opening SQLite database: %s", err_msg);
        return FALSE;
    }

    char* err_msg;
    // id is the ID of DB the entry
    // from_jid is the senders jid
    // to_jid is the receivers jid
    // from_resource is the senders resource
    // to_jid is the receivers resource
    // message is the message text
    // timestamp the timestamp like "2020/03/24 11:12:14"
    // type is there to distinguish: message (chat), MUC message (muc), muc pm (mucpm)
    // stanza_id is the ID in <message>
    // archive_id is the stanza-id from from XEP-0359: Unique and Stable Stanza IDs used for XEP-0313: Message Archive Management
    // replace_id is the ID from XEP-0308: Last Message Correction
    // encryption is to distinguish: none, omemo, otr, pgp
    // marked_read is 0/1 whether a message has been marked as read via XEP-0333: Chat Markers
    char* query = "CREATE TABLE IF NOT EXISTS `ChatLogs` ( `id` INTEGER PRIMARY KEY AUTOINCREMENT, `from_jid` TEXT NOT NULL, `to_jid` TEXT NOT NULL, `from_resource` TEXT, `to_resource` TEXT, `message` TEXT, `timestamp` TEXT, `type` TEXT, `stanza_id` TEXT, `archive_id` TEXT, `replace_id` TEXT, `encryption` TEXT, `marked_read` INTEGER)";
    if (SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        goto out;
    }

    query = "CREATE TABLE IF NOT EXISTS `DbVersion` ( `dv_id` INTEGER PRIMARY KEY, `version` INTEGER UNIQUE)";
    if (SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        goto out;
    }

    query = "INSERT OR IGNORE INTO `DbVersion` (`version`) VALUES('1')";
    if (SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        goto out;
    }

    log_debug("Initialized SQLite database: %s", filename);
    return TRUE;

out:
    if (err_msg) {
        log_error("SQLite error: %s", err_msg);
        sqlite3_free(err_msg);
    } else {
        log_error("Unknown SQLite error");
    }
    return FALSE;
}

void
log_database_close(void)
{
    if (g_chatlog_database) {
        sqlite3_close(g_chatlog_database);
        sqlite3_shutdown();
        g_chatlog_database = NULL;
    }
}

void
log_database_add_incoming(ProfMessage* message)
{
    if (message->to_jid) {
        _add_to_db(message, NULL, message->from_jid, message->to_jid);
    } else {
        auto_jid Jid* myjid = jid_create(connection_get_fulljid());

        _add_to_db(message, NULL, message->from_jid, myjid);
    }
}

static void
_log_database_add_outgoing(char* type, const char* const id, const char* const barejid, const char* const message, const char* const replace_id, prof_enc_t enc)
{
    ProfMessage* msg = message_init();

    msg->id = id ? strdup(id) : NULL;
    msg->from_jid = jid_create(barejid);
    msg->plain = message ? strdup(message) : NULL;
    msg->replace_id = replace_id ? strdup(replace_id) : NULL;
    msg->timestamp = g_date_time_new_now_local(); // TODO: get from outside. best to have whole ProfMessage from outside
    msg->enc = enc;

    auto_jid Jid* myjid = jid_create(connection_get_fulljid());

    _add_to_db(msg, type, myjid, msg->from_jid); // TODO: myjid now in profmessage

    message_free(msg);
}

void
log_database_add_outgoing_chat(const char* const id, const char* const barejid, const char* const message, const char* const replace_id, prof_enc_t enc)
{
    _log_database_add_outgoing("chat", id, barejid, message, replace_id, enc);
}

void
log_database_add_outgoing_muc(const char* const id, const char* const barejid, const char* const message, const char* const replace_id, prof_enc_t enc)
{
    _log_database_add_outgoing("muc", id, barejid, message, replace_id, enc);
}

void
log_database_add_outgoing_muc_pm(const char* const id, const char* const barejid, const char* const message, const char* const replace_id, prof_enc_t enc)
{
    _log_database_add_outgoing("mucpm", id, barejid, message, replace_id, enc);
}

// Get info (timestamp and stanza_id) of the first or last message in db
ProfMessage*
log_database_get_limits_info(const gchar* const contact_barejid, gboolean is_last)
{
    sqlite3_stmt* stmt = NULL;
    gchar* query;
    const char* jid = connection_get_fulljid();
    auto_jid Jid* myjid = jid_create(jid);
    if (!myjid)
        return NULL;

    if (is_last) {
        query = sqlite3_mprintf("SELECT * FROM (SELECT `archive_id`, `timestamp` from `ChatLogs` WHERE (`from_jid` = '%q' AND `to_jid` = '%q') OR (`from_jid` = '%q' AND `to_jid` = '%q') ORDER BY `timestamp` DESC LIMIT 1) ORDER BY `timestamp` ASC;", contact_barejid, myjid->barejid, myjid->barejid, contact_barejid);
    } else {
        query = sqlite3_mprintf("SELECT * FROM (SELECT `archive_id`, `timestamp` from `ChatLogs` WHERE (`from_jid` = '%q' AND `to_jid` = '%q') OR (`from_jid` = '%q' AND `to_jid` = '%q') ORDER BY `timestamp` ASC LIMIT 1) ORDER BY `timestamp` ASC;", contact_barejid, myjid->barejid, myjid->barejid, contact_barejid);
    }

    if (!query) {
        log_error("log_database_get_last_info(): SQL query. could not allocate memory");
        return NULL;
    }

    int rc = sqlite3_prepare_v2(g_chatlog_database, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        log_error("log_database_get_last_info(): unknown SQLite error");
        return NULL;
    }

    ProfMessage* msg = message_init();

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        char* archive_id = (char*)sqlite3_column_text(stmt, 0);
        char* date = (char*)sqlite3_column_text(stmt, 1);

        msg->stanzaid = strdup(archive_id);
        msg->timestamp = g_date_time_new_from_iso8601(date, NULL);
    }
    sqlite3_finalize(stmt);
    sqlite3_free(query);

    return msg;
}

// Query previous chats, constraints start_time and end_time. If end_time is
// null the current time is used. from_start gets first few messages if true
// otherwise the last ones. Flip flips the order of the results
GSList*
log_database_get_previous_chat(const gchar* const contact_barejid, const char* start_time, char* end_time, gboolean from_start, gboolean flip)
{
    sqlite3_stmt* stmt = NULL;
    const char* jid = connection_get_fulljid();
    auto_jid Jid* myjid = jid_create(jid);
    if (!myjid)
        return NULL;

    // Flip order when querying older pages
    gchar* sort1 = from_start ? "ASC" : "DESC";
    gchar* sort2 = !flip ? "ASC" : "DESC";
    GDateTime* now = g_date_time_new_now_local();
    auto_gchar gchar* end_date_fmt = end_time ? end_time : g_date_time_format_iso8601(now);
    auto_sqlite gchar* query = sqlite3_mprintf("SELECT * FROM (SELECT COALESCE(B.`message`, A.`message`) AS message, A.`timestamp`, A.`from_jid`, A.`type`, A.`encryption` from `ChatLogs` AS A LEFT JOIN `ChatLogs` AS B ON A.`stanza_id` = B.`replace_id` WHERE A.`replace_id` = '' AND ((A.`from_jid` = '%q' AND A.`to_jid` = '%q') OR (A.`from_jid` = '%q' AND A.`to_jid` = '%q')) AND A.`timestamp` < '%q' AND (%Q IS NULL OR A.`timestamp` > %Q) ORDER BY A.`timestamp` %s LIMIT %d) ORDER BY `timestamp` %s;", contact_barejid, myjid->barejid, myjid->barejid, contact_barejid, end_date_fmt, start_time, start_time, sort1, MESSAGES_TO_RETRIEVE, sort2);

    g_date_time_unref(now);

    if (!query) {
        log_error("log_database_get_previous_chat(): SQL query. could not allocate memory");
        return NULL;
    }

    int rc = sqlite3_prepare_v2(g_chatlog_database, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        log_error("log_database_get_previous_chat(): unknown SQLite error");
        return NULL;
    }

    GSList* history = NULL;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        // TODO: also save to jid. since now part of profmessage
        char* message = (char*)sqlite3_column_text(stmt, 0);
        char* date = (char*)sqlite3_column_text(stmt, 1);
        char* from = (char*)sqlite3_column_text(stmt, 2);
        char* type = (char*)sqlite3_column_text(stmt, 3);
        char* encryption = (char*)sqlite3_column_text(stmt, 3);

        ProfMessage* msg = message_init();
        msg->from_jid = jid_create(from);
        msg->plain = strdup(message);
        msg->timestamp = g_date_time_new_from_iso8601(date, NULL);
        msg->type = _get_message_type_type(type);
        msg->enc = _get_message_enc_type(encryption);

        history = g_slist_append(history, msg);
    }
    sqlite3_finalize(stmt);

    return history;
}

static const char*
_get_message_type_str(prof_msg_type_t type)
{
    switch (type) {
    case PROF_MSG_TYPE_CHAT:
        return "chat";
    case PROF_MSG_TYPE_MUC:
        return "muc";
    case PROF_MSG_TYPE_MUCPM:
        return "mucpm";
    case PROF_MSG_TYPE_UNINITIALIZED:
        return NULL;
    }
    return NULL;
}

static prof_msg_type_t
_get_message_type_type(const char* const type)
{
    if (g_strcmp0(type, "chat") == 0) {
        return PROF_MSG_TYPE_CHAT;
    } else if (g_strcmp0(type, "muc") == 0) {
        return PROF_MSG_TYPE_MUC;
    } else if (g_strcmp0(type, "mucpm") == 0) {
        return PROF_MSG_TYPE_MUCPM;
    } else {
        return PROF_MSG_TYPE_UNINITIALIZED;
    }
}

static const char*
_get_message_enc_str(prof_enc_t enc)
{
    switch (enc) {
    case PROF_MSG_ENC_OX:
        return "ox";
    case PROF_MSG_ENC_PGP:
        return "pgp";
    case PROF_MSG_ENC_OTR:
        return "otr";
    case PROF_MSG_ENC_OMEMO:
        return "omemo";
    case PROF_MSG_ENC_NONE:
        return "none";
    }

    return "none";
}

static prof_enc_t
_get_message_enc_type(const char* const encstr)
{
    if (g_strcmp0(encstr, "ox") == 0) {
        return PROF_MSG_ENC_OX;
    } else if (g_strcmp0(encstr, "pgp") == 0) {
        return PROF_MSG_ENC_PGP;
    } else if (g_strcmp0(encstr, "otr") == 0) {
        return PROF_MSG_ENC_OTR;
    } else if (g_strcmp0(encstr, "omemo") == 0) {
        return PROF_MSG_ENC_OMEMO;
    }

    return PROF_MSG_ENC_NONE;
}

static void
_add_to_db(ProfMessage* message, char* type, const Jid* const from_jid, const Jid* const to_jid)
{
    auto_gchar gchar* pref_dblog = prefs_get_string(PREF_DBLOG);

    if (g_strcmp0(pref_dblog, "off") == 0) {
        return;
    } else if (g_strcmp0(pref_dblog, "redact") == 0) {
        if (message->plain) {
            free(message->plain);
        }
        message->plain = strdup("[REDACTED]");
    }

    if (!g_chatlog_database) {
        log_debug("log_database_add() called but db is not initialized");
        return;
    }

    char* err_msg;
    auto_gchar gchar* date_fmt;

    if (message->timestamp) {
        date_fmt = g_date_time_format_iso8601(message->timestamp);
    } else {
        GDateTime* dt = g_date_time_new_now_local();
        date_fmt = g_date_time_format_iso8601(dt);
        g_date_time_unref(dt);
    }

    const char* enc = _get_message_enc_str(message->enc);

    if (!type) {
        type = (char*)_get_message_type_str(message->type);
    }

    auto_sqlite char* duplicate_check_query = sqlite3_mprintf("SELECT 1 FROM `ChatLogs` WHERE (`archive_id` = '%q' AND `archive_id` != '') OR (`stanza_id` = '%q' AND `stanza_id` != '')",
                                                              message->stanzaid ? message->stanzaid : "",
                                                              message->id ? message->id : "");

    if (!duplicate_check_query) {
        log_error("log_database_add(): SQL query for duplicate check. could not allocate memory");
        return;
    }

    int duplicate_exists = 0;
    sqlite3_stmt* stmt;

    if (SQLITE_OK == sqlite3_prepare_v2(g_chatlog_database, duplicate_check_query, -1, &stmt, NULL)) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            duplicate_exists = 1;
        }
        sqlite3_finalize(stmt);
    }

    if (duplicate_exists) {
        log_warning("Duplicate stanza-id found for the message. stanza_id: %s; archive_id: %s; sender: %s; content: %s", message->id, message->stanzaid, from_jid->barejid, message->plain);
        return;
    }

    auto_sqlite char* query = sqlite3_mprintf("INSERT INTO `ChatLogs` (`from_jid`, `from_resource`, `to_jid`, `to_resource`, `message`, `timestamp`, `stanza_id`, `archive_id`, `replace_id`, `type`, `encryption`) VALUES ('%q', '%q', '%q', '%q', '%q', '%q', '%q', '%q', '%q', '%q', '%q')",
                                              from_jid->barejid,
                                              from_jid->resourcepart ? from_jid->resourcepart : "",
                                              to_jid->barejid,
                                              to_jid->resourcepart ? to_jid->resourcepart : "",
                                              message->plain ? message->plain : "",
                                              date_fmt ? date_fmt : "",
                                              message->id ? message->id : "",
                                              message->stanzaid ? message->stanzaid : "",
                                              message->replace_id ? message->replace_id : "",
                                              type ? type : "",
                                              enc ? enc : "");
    if (!query) {
        log_error("log_database_add(): SQL query. could not allocate memory");
        return;
    }

    log_debug("Writing to DB. Query: %s", query);

    if (SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        if (err_msg) {
            log_error("SQLite error: %s", err_msg);
            sqlite3_free(err_msg);
        } else {
            log_error("Unknown SQLite error");
        }
    } else {
        int inserted_rows_count = sqlite3_changes(g_chatlog_database);
        if (inserted_rows_count < 1) {
            log_error("SQLite did not insert message (rows: %d, id: %s, content: %s)", inserted_rows_count, message->id, message->plain);
        }
    }
}
