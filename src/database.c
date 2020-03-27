/*
 * database.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 Michael Vetter <jubalh@idoru.org>
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

#define _GNU_SOURCE 1

#include <sys/stat.h>
#include <sqlite3.h>
#include <stdio.h>
#include <errno.h>

#include "log.h"
#include "config/files.h"

static sqlite3 *g_chatlog_database;

static char*
_get_db_filename(ProfAccount *account)
{
    char *databasedir = files_get_data_path(DIR_DATABASE);
    GString *basedir = g_string_new(databasedir);
    free(databasedir);

    g_string_append(basedir, "/");

    gchar *account_dir = str_replace(account->jid, "@", "_at_");
    g_string_append(basedir, account_dir);
    free(account_dir);

    int res = g_mkdir_with_parents(basedir->str, S_IRWXU);
    if (res == -1) {
        char *errmsg = strerror(errno);
        if (errmsg) {
            log_error("DATABASE: error creating directory: %s, %s", basedir->str, errmsg);
        } else {
            log_error("DATABASE: creating directory: %s", basedir->str);
        }
        g_string_free(basedir, TRUE);
        return NULL;
    }

    g_string_append(basedir, "/chatlog.db");
    char *result = strdup(basedir->str);
    g_string_free(basedir, TRUE);

    return result;
}

gboolean
log_database_init(ProfAccount *account)
{
    int ret = sqlite3_initialize();
    if (ret != SQLITE_OK) {
        log_error("Error initializing SQLite database: %d", ret);
        return FALSE;
    }

    char *filename = _get_db_filename(account);
    if (!filename) {
        return FALSE;
    }

    ret = sqlite3_open(filename, &g_chatlog_database);
    if (ret != SQLITE_OK) {
        const char *err_msg = sqlite3_errmsg(g_chatlog_database);
        log_error("Error opening SQLite database: %s", err_msg);
        free(filename);
        return FALSE;
    }

    char *err_msg;
    // id is the ID of DB the entry
    // from_jid is the senders jid
    // to_jid is the receivers jid
    // from_resource is the senders resource
    // to_jid is the receivers resource
    // message is the message text
    // timestamp the timestamp like "2020/03/24 11:12:14"
    // type is there to distinguish: message (chat), MUC message (muc), muc pm (mucpm)
    // stanza_id is the ID from XEP-0359: Unique and Stable Stanza IDs
    // archive_id is the ID from XEP-0313: Message Archive Management
    // replace_id is the ID from XEP-0308: Last Message Correction
    // encryption is to distinguish: none, omemo, otr, pgp
    // marked_read is 0/1 whether a message has been marked as read via XEP-0333: Chat Markers
    char *query = "CREATE TABLE IF NOT EXISTS `ChatLogs` ( `id` INTEGER PRIMARY KEY AUTOINCREMENT, `from_jid` TEXT NOT NULL, `to_jid` TEXT NOT NULL, `from_resource` TEXT, `to_resource` TEXT, `message` TEXT, `timestamp` TEXT, `type` TEXT, `stanza_id` TEXT, `archive_id` TEXT, `replace_id` TEXT, `encryption` TEXT, `marked_read` INTEGER)";
    if( SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        goto out;
    }

    query = "CREATE TABLE IF NOT EXISTS `DbVersion` ( `dv_id` INTEGER PRIMARY KEY, `version` INTEGER UNIQUE)";
    if( SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        goto out;
    }

    query = "INSERT OR IGNORE INTO `DbVersion` (`version`) VALUES('1')";
    if( SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        goto out;
    }

    log_debug("Initialized SQLite database: %s", filename);
    free(filename);
    return TRUE;

out:
    if (err_msg) {
        log_error("SQLite error: %s", err_msg);
        sqlite3_free(err_msg);
    } else {
        log_error("Unknown SQLite error");
    }
    free(filename);
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
log_database_add(ProfMessage *message, const char *const type) {
    if (!g_chatlog_database) {
        log_debug("log_database_add() called but db is not initialized");
        return;
    }

    char *err_msg;
    char *query;

    const char *jid = connection_get_fulljid();
    Jid *myjid = jid_create(jid);

    //gchar *date_fmt = g_date_time_format_iso8601(message->timestamp);
    gchar *date_fmt = g_date_time_format(message->timestamp, "%Y/%m/%d %H:%M:%S");
    if (asprintf(&query, "INSERT INTO `ChatLogs` (`from_jid`, `from_resource`, `to_jid`, `message`, `timestamp`, `stanza_id`, `replace_id`, `type`) VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')",
                message->jid->barejid,
                message->jid->resourcepart,
                myjid->barejid,
                message->plain,
                date_fmt,
                message->id ? message->id : "",
                message->replace_id ? message->replace_id : "",
                type) == -1) {
        log_error("log_database_add(): could not allocate memory");
        return;
    }
    g_free(date_fmt);

    if( SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        if (err_msg) {
            log_error("SQLite error: %s", err_msg);
            sqlite3_free(err_msg);
        } else {
            log_error("Unknown SQLite error");
        }
    }
    free(query);
}
