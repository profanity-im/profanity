/*
 * db.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef DATABASE_DB_H
#define DATABASE_DB_H

#include "config.h"

#include <glib.h>
#include <sqlite3.h>

/* Connection Lifecycle */
gboolean prof_db_open(const gchar* path, sqlite3** db);
void prof_db_close(sqlite3** db);

/* Schema & Commands */
gboolean prof_db_exec(sqlite3* db, const gchar* sql);

/* Prepared Statements */
gboolean prof_db_prepare(sqlite3* db, const gchar* sql, sqlite3_stmt** stmt);
void prof_db_finalize(sqlite3_stmt** stmt);
gboolean prof_db_step_done(sqlite3* db, sqlite3_stmt* stmt);

/* Transactions */
gboolean prof_db_begin(sqlite3* db);
gboolean prof_db_commit(sqlite3* db);
gboolean prof_db_rollback(sqlite3* db);

/* Schema Versioning */
gint prof_db_get_version(sqlite3* db);
gboolean prof_db_set_version(sqlite3* db, gint version);

#endif
