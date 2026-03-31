/*
 * database.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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
static gboolean _migrate_to_v3(void);
static gboolean _check_available_space_for_db_migration(char* path_to_db);

static const int latest_version = 3;

static char*
_db_strdup(const char* str)
{
    return str ? strdup(str) : NULL;
}

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
                  "`archive_id` TEXT UNIQUE, "
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
        query = "INSERT INTO `DbVersion` (`version`) VALUES ('2') ON CONFLICT(`version`) DO NOTHING";
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
        if (db_version < 3 && (!_check_available_space_for_db_migration(filename) || !_migrate_to_v3())) {
            cons_show_error("Database Initialization Error: Unable to migrate database to version 3. Please, check error logs for details.");
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
        _add_to_db(message, NULL, message->from_jid, connection_get_jid());
    }
}

static void
_log_database_add_outgoing(char* type, const char* const id, const char* const barejid, const char* const message, const char* const replace_id, prof_enc_t enc)
{
    ProfMessage* msg = message_init();

    msg->id = _db_strdup(id);
    msg->from_jid = jid_create(barejid);
    msg->plain = _db_strdup(message);
    msg->replace_id = _db_strdup(replace_id);
    msg->timestamp = g_date_time_new_now_local(); // TODO: get from outside. best to have whole ProfMessage from outside
    msg->enc = enc;

    _add_to_db(msg, type, connection_get_jid(), msg->from_jid); // TODO: myjid now in profmessage

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
    const Jid* myjid = connection_get_jid();
    if (!myjid->str)
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

        msg->stanzaid = _db_strdup(archive_id);
        msg->timestamp = g_date_time_new_from_iso8601(date, NULL);
    } else {
        message_free(msg);
        msg = NULL;
    }
    sqlite3_finalize(stmt);

    return msg;
}

// Query previous chats, constraints start_time and end_time. If end_time is
// null the current time is used. from_start gets first few messages if true
// otherwise the last ones. Flip flips the order of the results
GSList*
log_database_get_previous_chat(const gchar* const contact_barejid, const char* start_time, const char* end_time, gboolean from_start, gboolean flip)
{
    sqlite3_stmt* stmt = NULL;
    const Jid* myjid = connection_get_jid();
    if (!myjid->str)
        return NULL;

    // Flip order when querying older pages
    gchar* sort1 = from_start ? "ASC" : "DESC";
    gchar* sort2 = !flip ? "ASC" : "DESC";
    auto_gchar gchar* end_date_fmt = end_time ? g_strdup(end_time) : prof_date_time_format_iso8601(NULL);
    auto_sqlite gchar* query = sqlite3_mprintf("SELECT * FROM ("
                                               "SELECT COALESCE(B.`message`, A.`message`) AS message, "
                                               "A.`timestamp`, A.`from_jid`, A.`from_resource`, A.`to_jid`, A.`to_resource`, A.`type`, A.`encryption`, A.`stanza_id` FROM `ChatLogs` AS A "
                                               "LEFT JOIN `ChatLogs` AS B ON (A.`replaced_by_db_id` = B.`id` AND A.`from_jid` = B.`from_jid`) "
                                               "WHERE (A.`replaces_db_id` IS NULL) "
                                               "AND ((A.`from_jid` = %Q AND A.`to_jid` = %Q) OR (A.`from_jid` = %Q AND A.`to_jid` = %Q)) "
                                               "AND A.`timestamp` < %Q "
                                               "AND (%Q IS NULL OR A.`timestamp` > %Q) "
                                               "ORDER BY A.`timestamp` %s LIMIT %d) "
                                               "ORDER BY `timestamp` %s;",
                                               contact_barejid, myjid->barejid, myjid->barejid, contact_barejid, end_date_fmt, start_time, start_time, sort1, MESSAGES_TO_RETRIEVE, sort2);

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
        char* from_jid = (char*)sqlite3_column_text(stmt, 2);
        char* from_resource = (char*)sqlite3_column_text(stmt, 3);
        char* to_jid = (char*)sqlite3_column_text(stmt, 4);
        char* to_resource = (char*)sqlite3_column_text(stmt, 5);
        char* type = (char*)sqlite3_column_text(stmt, 6);
        char* encryption = (char*)sqlite3_column_text(stmt, 7);
        char* id = (char*)sqlite3_column_text(stmt, 8);

        ProfMessage* msg = message_init();
        msg->id = id ? strdup(id) : NULL;
        msg->from_jid = jid_create_from_bare_and_resource(from_jid, from_resource);
        msg->to_jid = jid_create_from_bare_and_resource(to_jid, to_resource);
        msg->plain = strdup(message ?: "");
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
    auto_gchar gchar* date_fmt = prof_date_time_format_iso8601(message->timestamp);
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
    // We use archive_id UNIQUE constraint and ON CONFLICT DO NOTHING for deduplication.

    auto_sqlite char* orig_message_id = original_message_id == -1 ? NULL : sqlite3_mprintf("%d", original_message_id);

    auto_sqlite char* query = sqlite3_mprintf("INSERT INTO `ChatLogs` "
                                              "(`from_jid`, `from_resource`, `to_jid`, `to_resource`, "
                                              "`message`, `timestamp`, `stanza_id`, `archive_id`, "
                                              "`replaces_db_id`, `replace_id`, `type`, `encryption`) "
                                              "VALUES (%Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q) "
                                              "ON CONFLICT(`archive_id`) DO NOTHING "
                                              "RETURNING id",
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

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(g_chatlog_database, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        log_error("SQLite error in _add_to_db() (prepare): %s", sqlite3_errmsg(g_chatlog_database));
        return;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        log_debug("Successfully inserted message into database.");
    } else if (rc == SQLITE_DONE) {
        log_debug("Message already exists in database (archive_id: %s), skipping.", message->stanzaid);
    } else {
        log_error("SQLite error in _add_to_db() (step): %s", sqlite3_errmsg(g_chatlog_database));
    }

    sqlite3_finalize(stmt);
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

static gboolean
_migrate_to_v3(void)
{
    char* err_msg = NULL;

    const char* sql_statements[] = {
        "BEGIN TRANSACTION",
        "CREATE TABLE `ChatLogs_v3_migration` ("
        "`id` INTEGER PRIMARY KEY AUTOINCREMENT, "
        "`from_jid` TEXT NOT NULL, "
        "`to_jid` TEXT NOT NULL, "
        "`from_resource` TEXT, "
        "`to_resource` TEXT, "
        "`message` TEXT, "
        "`timestamp` TEXT, "
        "`type` TEXT, "
        "`stanza_id` TEXT, "
        "`archive_id` TEXT UNIQUE, "
        "`encryption` TEXT, "
        "`marked_read` INTEGER, "
        "`replace_id` TEXT, "
        "`replaces_db_id` INTEGER, "
        "`replaced_by_db_id` INTEGER)",

        "INSERT INTO `ChatLogs_v3_migration` "
        "SELECT * FROM `ChatLogs` "
        "WHERE `archive_id` IS NULL "
        "UNION ALL "
        "SELECT * FROM (SELECT * FROM `ChatLogs` WHERE `archive_id` IS NOT NULL GROUP BY `archive_id`)",

        "DROP TABLE `ChatLogs` ",
        "ALTER TABLE `ChatLogs_v3_migration` RENAME TO `ChatLogs` ",

        "CREATE INDEX ChatLogs_timestamp_IDX ON `ChatLogs` (`timestamp`)",
        "CREATE INDEX ChatLogs_to_from_jid_IDX ON `ChatLogs` (`to_jid`, `from_jid`)",
        "CREATE TRIGGER update_corrected_message "
        "AFTER INSERT ON ChatLogs "
        "FOR EACH ROW "
        "WHEN NEW.replaces_db_id IS NOT NULL "
        "BEGIN "
        "UPDATE ChatLogs "
        "SET replaced_by_db_id = NEW.id "
        "WHERE id = NEW.replaces_db_id; "
        "END;",

        "UPDATE `DbVersion` SET `version` = 3;",
        "END TRANSACTION"
    };

    int statements_count = sizeof(sql_statements) / sizeof(sql_statements[0]);

    for (int i = 0; i < statements_count; i++) {
        if (SQLITE_OK != sqlite3_exec(g_chatlog_database, sql_statements[i], NULL, 0, &err_msg)) {
            log_error("SQLite error in _migrate_to_v3() on statement %d: %s", i, err_msg);
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
        log_error("DB Migration] Unable to ROLLBACK: %s", err_msg);
        if (err_msg) {
            sqlite3_free(err_msg);
        }
    }

    return FALSE;
}
