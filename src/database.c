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
#include <sys/statvfs.h>
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
#include "ui/ui.h"
#include "xmpp/xmpp.h"
#include "xmpp/message.h"

static sqlite3* g_chatlog_database;

static void _add_to_db(ProfMessage* message, char* type, const Jid* const from_jid, const Jid* const to_jid);
static char* _get_db_filename(ProfAccount* account);
static prof_msg_type_t _get_message_type_type(const char* const type);
static prof_enc_t _get_message_enc_type(const char* const encstr);
static int _get_db_version(void);
static gboolean _migrate_to_v2(void);
static gboolean _check_available_space_for_db_migration(char* path_to_db);

#define auto_sqlite __attribute__((__cleanup__(auto_free_sqlite)))

static const int latest_version = 2;

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

    int db_version = _get_db_version();
    if (db_version == latest_version) {
        return TRUE;
    }

    // ChatLogs Table
    // Contains all chat messages
    //
    // id is primary key
    // from_jid is the sender's jid
    // to_jid is the receiver's jid
    // from_resource is the sender's resource
    // to_resource is the receiver's resource
    // message is the message's text
    // timestamp is the timestamp like "2020/03/24 11:12:14"
    // type is there to distinguish: message (chat), MUC message (muc), muc pm (mucpm)
    // stanza_id is the ID in <message>
    // archive_id is the stanza-id from from XEP-0359: Unique and Stable Stanza IDs used for XEP-0313: Message Archive Management
    // encryption is to distinguish: none, omemo, otr, pgp
    // marked_read is 0/1 whether a message has been marked as read via XEP-0333: Chat Markers
    // replace_id is the ID from XEP-0308: Last Message Correction
    // replaces_db_id is ID (primary key) of the original message that LMC message corrects/replaces
    // replaced_by_db_id is ID (primary key) of the last correcting (LMC) message for the original message
    char* query = "CREATE TABLE IF NOT EXISTS `ChatLogs` ("
                  "`id` INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "`from_jid` TEXT NOT NULL, "
                  "`to_jid` TEXT NOT NULL, "
                  "`from_resource` TEXT, "
                  "`to_resource` TEXT, "
                  "`message` TEXT, "
                  "`timestamp` TEXT, "
                  "`type` TEXT, "
                  "`stanza_id` TEXT, "
                  "`archive_id` TEXT, "
                  "`encryption` TEXT, "
                  "`marked_read` INTEGER, "
                  "`replace_id` TEXT, "
                  "`replaces_db_id` INTEGER, "
                  "`replaced_by_db_id` INTEGER)";
    if (SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        goto out;
    }

    query = "CREATE TRIGGER IF NOT EXISTS update_corrected_message "
            "AFTER INSERT ON ChatLogs "
            "FOR EACH ROW "
            "WHEN NEW.replaces_db_id IS NOT NULL "
            "BEGIN "
            "UPDATE ChatLogs "
            "SET replaced_by_db_id = NEW.id "
            "WHERE id = NEW.replaces_db_id; "
            "END;";
    if (SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        log_error("Unable to add `update_corrected_message` trigger.");
        goto out;
    }

    query = "CREATE INDEX IF NOT EXISTS ChatLogs_timestamp_IDX ON `ChatLogs` (`timestamp`)";
    if (SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        log_error("Unable to create index for timestamp.");
        goto out;
    }
    query = "CREATE INDEX IF NOT EXISTS ChatLogs_to_from_jid_IDX ON `ChatLogs` (`to_jid`, `from_jid`)";
    if (SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        log_error("Unable to create index for to_jid.");
        goto out;
    }

    query = "CREATE TABLE IF NOT EXISTS `DbVersion` (`dv_id` INTEGER PRIMARY KEY, `version` INTEGER UNIQUE)";
    if (SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        goto out;
    }

    if (db_version == -1) {
        query = "INSERT OR IGNORE INTO `DbVersion` (`version`) VALUES ('2')";
        if (SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
            goto out;
        }
        db_version = _get_db_version();
    }

    // Unlikely event, but we don't want to migrate if we are just unable to determine the DB version
    if (db_version == -1) {
        cons_show_error("DB Initialization Error: Unable to check DB version.");
        goto out;
    }

    if (db_version < latest_version) {
        cons_show("Migrating database schema. This operation may take a while...");
        if (db_version < 2 && (!_check_available_space_for_db_migration(filename) || !_migrate_to_v2())) {
            cons_show_error("Database Initialization Error: Unable to migrate database to version 2. Please, check error logs for details.");
            goto out;
        }
        cons_show("Database schema migration was successful.");
    }

    log_debug("Initialized SQLite database: %s", filename);
    return TRUE;

out:
    if (err_msg) {
        log_error("SQLite error in log_database_init(): %s", err_msg);
        sqlite3_free(err_msg);
    } else {
        log_error("Unknown SQLite error in log_database_init().");
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
    const char* jid = connection_get_fulljid();
    auto_jid Jid* myjid = jid_create(jid);
    if (!myjid)
        return NULL;

    const char* order = is_last ? "DESC" : "ASC";
    auto_sqlite char* query = sqlite3_mprintf("SELECT `archive_id`, `timestamp` FROM `ChatLogs` WHERE "
                                              "(`from_jid` = %Q AND `to_jid` = %Q) OR "
                                              "(`from_jid` = %Q AND `to_jid` = %Q) "
                                              "ORDER BY `timestamp` %s LIMIT 1;",
                                              contact_barejid, myjid->barejid, myjid->barejid, contact_barejid, order);

    if (!query) {
        log_error("Could not allocate memory for SQL query in log_database_get_limits_info()");
        return NULL;
    }

    int rc = sqlite3_prepare_v2(g_chatlog_database, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        log_error("Unknown SQLite error in log_database_get_last_info().");
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
    auto_sqlite gchar* query = sqlite3_mprintf("SELECT * FROM ("
                                               "SELECT COALESCE(B.`message`, A.`message`) AS message, "
                                               "A.`timestamp`, A.`from_jid`, A.`to_jid`, A.`type`, A.`encryption` FROM `ChatLogs` AS A "
                                               "LEFT JOIN `ChatLogs` AS B ON (A.`replaced_by_db_id` = B.`id` AND A.`from_jid` = B.`from_jid`) "
                                               "WHERE (A.`replaces_db_id` IS NULL) "
                                               "AND ((A.`from_jid` = %Q AND A.`to_jid` = %Q) OR (A.`from_jid` = %Q AND A.`to_jid` = %Q)) "
                                               "AND A.`timestamp` < %Q "
                                               "AND (%Q IS NULL OR A.`timestamp` > %Q) "
                                               "ORDER BY A.`timestamp` %s LIMIT %d) "
                                               "ORDER BY `timestamp` %s;",
                                               contact_barejid, myjid->barejid, myjid->barejid, contact_barejid, end_date_fmt, start_time, start_time, sort1, MESSAGES_TO_RETRIEVE, sort2);

    g_date_time_unref(now);

    if (!query) {
        log_error("Could not allocate memory");
        return NULL;
    }

    int rc = sqlite3_prepare_v2(g_chatlog_database, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        log_error("SQLite error in log_database_get_previous_chat(): %s", sqlite3_errmsg(g_chatlog_database));
        return NULL;
    }

    GSList* history = NULL;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        char* message = (char*)sqlite3_column_text(stmt, 0);
        char* date = (char*)sqlite3_column_text(stmt, 1);
        char* from = (char*)sqlite3_column_text(stmt, 2);
        char* to_jid = (char*)sqlite3_column_text(stmt, 3);
        char* type = (char*)sqlite3_column_text(stmt, 4);
        char* encryption = (char*)sqlite3_column_text(stmt, 5);

        ProfMessage* msg = message_init();
        msg->from_jid = jid_create(from);
        msg->to_jid = jid_create(to_jid);
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
    sqlite_int64 original_message_id = -1;

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
    auto_gchar gchar* date_fmt = NULL;

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

    // Apply LMC and check its validity (XEP-0308)
    if (message->replace_id) {
        auto_sqlite char* replace_check_query = sqlite3_mprintf("SELECT `id`, `from_jid`, `replaces_db_id` FROM `ChatLogs` WHERE `stanza_id` = %Q ORDER BY `timestamp` DESC LIMIT 1",
                                                                message->replace_id);

        if (!replace_check_query) {
            log_error("Could not allocate memory for SQL replace query in log_database_add()");
            return;
        }

        sqlite3_stmt* lmc_stmt = NULL;

        if (SQLITE_OK != sqlite3_prepare_v2(g_chatlog_database, replace_check_query, -1, &lmc_stmt, NULL)) {
            log_error("SQLite error in _add_to_db() on selecting original message: %s", sqlite3_errmsg(g_chatlog_database));
            return;
        }

        if (sqlite3_step(lmc_stmt) == SQLITE_ROW) {
            original_message_id = sqlite3_column_int64(lmc_stmt, 0);
            const char* from_jid_orig = (const char*)sqlite3_column_text(lmc_stmt, 1);

            // Handle non-XEP-compliant replacement messages (edit->edit->original)
            sqlite_int64 tmp = sqlite3_column_int64(lmc_stmt, 2);
            original_message_id = tmp ? tmp : original_message_id;

            if (g_strcmp0(from_jid_orig, from_jid->barejid) != 0) {
                log_error("Mismatch in sender JIDs when trying to do LMC. Corrected message sender: %s. Original message sender: %s. Replace-ID: %s. Message: %s", from_jid->barejid, from_jid_orig, message->replace_id, message->plain);
                cons_show_error("%s sent a message correction with mismatched sender. See log for details.", from_jid->barejid);
                sqlite3_finalize(lmc_stmt);
                return;
            }
        } else {
            log_warning("Got LMC message that does not have original message counterpart in the database from %s", message->from_jid->fulljid);
        }
        sqlite3_finalize(lmc_stmt);
    }

    // stanza-id (XEP-0359) doesn't have to be present in the message.
    // But if it's duplicated, it's a serious server-side problem, so we better track it.
    if (message->stanzaid) {
        auto_sqlite char* duplicate_check_query = sqlite3_mprintf("SELECT 1 FROM `ChatLogs` WHERE (`archive_id` = %Q)",
                                                                  message->stanzaid);

        if (!duplicate_check_query) {
            log_error("Could not allocate memory for SQL duplicate query in log_database_add()");
            return;
        }

        sqlite3_stmt* stmt;

        if (SQLITE_OK == sqlite3_prepare_v2(g_chatlog_database, duplicate_check_query, -1, &stmt, NULL)) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                log_error("Duplicate stanza-id found for the message. stanza_id: %s; archive_id: %s; sender: %s; content: %s", message->id, message->stanzaid, from_jid->barejid, message->plain);
                cons_show_error("Got a message with duplicate (server-generated) stanza-id from %s.", from_jid->fulljid);
            }
            sqlite3_finalize(stmt);
        }
    }

    auto_sqlite char* orig_message_id = original_message_id == -1 ? NULL : sqlite3_mprintf("%d", original_message_id);

    auto_sqlite char* query = sqlite3_mprintf("INSERT INTO `ChatLogs` "
                                              "(`from_jid`, `from_resource`, `to_jid`, `to_resource`, "
                                              "`message`, `timestamp`, `stanza_id`, `archive_id`, "
                                              "`replaces_db_id`, `replace_id`, `type`, `encryption`) "
                                              "VALUES (%Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q)",
                                              from_jid->barejid,
                                              from_jid->resourcepart,
                                              to_jid->barejid,
                                              to_jid->resourcepart,
                                              message->plain,
                                              date_fmt,
                                              message->id,
                                              message->stanzaid,
                                              orig_message_id,
                                              message->replace_id,
                                              type,
                                              enc);
    if (!query) {
        log_error("Could not allocate memory for SQL insert query in log_database_add()");
        return;
    }

    log_debug("Writing to DB. Query: %s", query);

    if (SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        if (err_msg) {
            log_error("SQLite error in _add_to_db(): %s", err_msg);
            sqlite3_free(err_msg);
        } else {
            log_error("Unknown SQLite error in _add_to_db().");
        }
    } else {
        int inserted_rows_count = sqlite3_changes(g_chatlog_database);
        if (inserted_rows_count < 1) {
            log_error("SQLite did not insert message (rows: %d, id: %s, content: %s)", inserted_rows_count, message->id, message->plain);
        }
    }
}

static int
_get_db_version(void)
{
    int current_version = -1;
    const char* query = "SELECT `version` FROM `DbVersion` LIMIT 1";
    sqlite3_stmt* statement;

    if (sqlite3_prepare_v2(g_chatlog_database, query, -1, &statement, NULL) == SQLITE_OK) {
        if (sqlite3_step(statement) == SQLITE_ROW) {
            current_version = sqlite3_column_int(statement, 0);
        }
        sqlite3_finalize(statement);
    }
    return current_version;
}

/**
 * Migration to version 2 introduces new columns. Returns TRUE on success.
 *
 * New columns:
 * `replaces_db_id` database ID for correcting message of the original message
 * `replaced_by_db_id` database ID for original message of the last correcting message
 */
static gboolean
_migrate_to_v2(void)
{
    char* err_msg = NULL;

    // from_resource, to_resource, message, timestamp, stanza_id, archive_id, replace_id, type, encryption
    const char* sql_statements[] = {
        "BEGIN TRANSACTION",
        "ALTER TABLE `ChatLogs` ADD COLUMN `replaces_db_id` INTEGER;",
        "ALTER TABLE `ChatLogs` ADD COLUMN `replaced_by_db_id` INTEGER;",
        "UPDATE `ChatLogs` AS A "
        "SET `replaces_db_id` = B.`id` "
        "FROM `ChatLogs` AS B "
        "WHERE A.`replace_id` IS NOT NULL AND A.`replace_id` != '' "
        "AND A.`replace_id` = B.`stanza_id` "
        "AND A.`from_jid` = B.`from_jid` AND A.`to_jid` = B.`to_jid`;",
        "UPDATE `ChatLogs` AS A "
        "SET `replaced_by_db_id` = B.`id` "
        "FROM `ChatLogs` AS B "
        "WHERE (A.`replace_id` IS NULL OR A.`replace_id` = '') "
        "AND A.`id` = B.`replaces_db_id` "
        "AND A.`from_jid` = B.`from_jid`;",
        "UPDATE ChatLogs SET "
        "from_resource = COALESCE(NULLIF(from_resource, ''), NULL), "
        "to_resource = COALESCE(NULLIF(to_resource, ''), NULL), "
        "message = COALESCE(NULLIF(message, ''), NULL), "
        "timestamp = COALESCE(NULLIF(timestamp, ''), NULL), "
        "stanza_id = COALESCE(NULLIF(stanza_id, ''), NULL), "
        "archive_id = COALESCE(NULLIF(archive_id, ''), NULL), "
        "replace_id = COALESCE(NULLIF(replace_id, ''), NULL), "
        "type = COALESCE(NULLIF(type, ''), NULL), "
        "encryption = COALESCE(NULLIF(encryption, ''), NULL);",
        "UPDATE `DbVersion` SET `version` = 2;",
        "END TRANSACTION"
    };

    int statements_count = sizeof(sql_statements) / sizeof(sql_statements[0]);

    for (int i = 0; i < statements_count; i++) {
        if (SQLITE_OK != sqlite3_exec(g_chatlog_database, sql_statements[i], NULL, 0, &err_msg)) {
            log_error("SQLite error in _migrate_to_v2() on statement %d: %s", i, err_msg);
            if (err_msg) {
                sqlite3_free(err_msg);
                err_msg = NULL;
            }
            goto cleanup;
        }
    }

    return TRUE;

cleanup:
    if (SQLITE_OK != sqlite3_exec(g_chatlog_database, "ROLLBACK;", NULL, 0, &err_msg)) {
        log_error("[DB Migration] Unable to ROLLBACK: %s", err_msg);
        if (err_msg) {
            sqlite3_free(err_msg);
        }
    }

    return FALSE;
}

// Checks if there is more system storage space available than current database takes + 40% (for indexing and other potential size increases)
static gboolean
_check_available_space_for_db_migration(char* path_to_db)
{
    struct stat file_stat;
    struct statvfs fs_stat;

    if (statvfs(path_to_db, &fs_stat) == 0 && stat(path_to_db, &file_stat) == 0) {
        unsigned long long file_size = file_stat.st_size / 1024;
        unsigned long long available_space_kb = fs_stat.f_frsize * fs_stat.f_bavail / 1024;
        log_debug("_check_available_space_for_db_migration(): Available space on disk: %llu KB; DB size: %llu KB", available_space_kb, file_size);

        return (available_space_kb >= (file_size + (file_size * 10 / 4)));
    } else {
        log_error("Error checking available space.");
        return FALSE;
    }
}
