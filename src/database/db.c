/*
 * db.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <sqlite3.h>
#include <glib.h>

#include "log.h"
#include "database/db.h"

#define PROF_DB_BUSY_TIMEOUT_MS 5000

gboolean
prof_db_open(const gchar* path, sqlite3** db)
{
    if (!path || !db) {
        return FALSE;
    }

    int ret = sqlite3_initialize();
    if (ret != SQLITE_OK) {
        log_error("Error initializing SQLite library: %d", ret);
        return FALSE;
    }

    ret = sqlite3_open(path, db);
    if (ret != SQLITE_OK) {
        log_error("Error opening database %s: %s", path, sqlite3_errmsg(*db));
        if (*db) {
            sqlite3_close(*db);
            *db = NULL;
        }
        return FALSE;
    }

    sqlite3_busy_timeout(*db, PROF_DB_BUSY_TIMEOUT_MS);
    sqlite3_exec(*db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(*db, "PRAGMA foreign_keys=ON;", NULL, NULL, NULL);

    return TRUE;
}

void
prof_db_close(sqlite3** db)
{
    if (!db || !*db) {
        return;
    }

    sqlite3_close(*db);
    *db = NULL;
}

gboolean
prof_db_exec(sqlite3* db, const gchar* sql)
{
    if (!db || !sql) {
        return FALSE;
    }

    char* err_msg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        log_error("SQL execution error: %s (SQL: %s)", err_msg ? err_msg : "unknown", sql);
        sqlite3_free(err_msg);
        return FALSE;
    }

    return TRUE;
}

gboolean
prof_db_prepare(sqlite3* db, const gchar* sql, sqlite3_stmt** stmt)
{
    if (!db || !sql || !stmt) {
        return FALSE;
    }

    int rc = sqlite3_prepare_v2(db, sql, -1, stmt, NULL);
    if (rc != SQLITE_OK) {
        log_error("Error preparing statement: %s (SQL: %s)", sqlite3_errmsg(db), sql);
        return FALSE;
    }

    return TRUE;
}

void
prof_db_finalize(sqlite3_stmt** stmt)
{
    if (!stmt || !*stmt) {
        return;
    }

    sqlite3_finalize(*stmt);
    *stmt = NULL;
}

gboolean
prof_db_step_done(sqlite3* db, sqlite3_stmt* stmt)
{
    if (!db || !stmt) {
        return FALSE;
    }

    int rc = sqlite3_step(stmt);
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    if (rc != SQLITE_DONE) {
        log_error("SQLite step error: %s", sqlite3_errmsg(db));
        return FALSE;
    }

    return TRUE;
}

gboolean
prof_db_begin(sqlite3* db)
{
    return prof_db_exec(db, "BEGIN TRANSACTION;");
}

gboolean
prof_db_commit(sqlite3* db)
{
    return prof_db_exec(db, "COMMIT;");
}

gboolean
prof_db_rollback(sqlite3* db)
{
    return prof_db_exec(db, "ROLLBACK;");
}
