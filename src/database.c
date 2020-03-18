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

#include <sqlite3.h>

#include "log.h"

static sqlite3 *g_log_database;

bool
log_database_init(void)
{
    int ret = sqlite3_initialize();
    char *filename = "test";

	if (ret != SQLITE_OK) {
        log_error("Error initializing SQLite database: %d", ret);
        return FALSE;
	}

    ret = sqlite3_open(filename, &g_log_database);
    if (ret != SQLITE_OK) {
        const char *err_msg = sqlite3_errmsg(g_log_database);
        log_error("Error opening SQLite database: %s", err_msg);
        return FALSE;
    }

    char *err_msg;
	char *query = "CREATE TABLE IF NOT EXISTS `ChatLogs` ( `id` INTEGER PRIMARY KEY, `jid` TEXT NOT NULL, `message` TEXT, `timestamp` TEXT)";
    if( SQLITE_OK != sqlite3_exec(g_db, query, NULL, 0, &err_msg)) {
        if (err_msg) {
            log_error("SQLite error: %s", err_msg);
            sqlite3_free(err_msg);
        } else {
            log_error("Unknown SQLite error");
        }
        return FALSE;
    }

    log_debug("Initialized SQLite database: %s", filename);
    return TRUE;
}

void
log_database_close(void)
{
	sqlite3_close(g_log_database);
	sqlite3_shutdown();
}
