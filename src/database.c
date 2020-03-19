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

#include <sqlite3.h>
#include <stdio.h>
#include <glib.h>

#include "log.h"
#include "config/files.h"
#include "xmpp/xmpp.h"

static sqlite3 *g_chatlog_database;

bool
log_database_init(void)
{
    int ret = sqlite3_initialize();
    char *filename = files_get_chatlog_database_path();

	if (ret != SQLITE_OK) {
        free(filename);
        log_error("Error initializing SQLite database: %d", ret);
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
	char *query = "CREATE TABLE IF NOT EXISTS `ChatLogs` ( `id` INTEGER PRIMARY KEY, `jid` TEXT NOT NULL, `message` TEXT, `timestamp` TEXT)";
    if( SQLITE_OK != sqlite3_exec(g_chatlog_database, query, NULL, 0, &err_msg)) {
        if (err_msg) {
            log_error("SQLite error: %s", err_msg);
            sqlite3_free(err_msg);
        } else {
            log_error("Unknown SQLite error");
        }
        free(filename);
        return FALSE;
    }

    log_debug("Initialized SQLite database: %s", filename);
    free(filename);
    return TRUE;
}

void
log_database_close(void)
{
	sqlite3_close(g_chatlog_database);
	sqlite3_shutdown();
}

void
log_database_add(ProfMessage *message) {
    char *err_msg;
	char *query;

    //gchar *date_fmt = g_date_time_format_iso8601(message->timestamp);
    gchar *date_fmt = g_date_time_format(message->timestamp, "%Y/%m/%d %H:%M:%S");
    if (asprintf(&query, "INSERT INTO `ChatLogs` (`jid`, `message`, `timestamp`) VALUES ('%s', '%s', '%s')",
                message->jid->barejid, message->plain, date_fmt) == -1) {
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
