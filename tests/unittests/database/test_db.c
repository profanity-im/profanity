/*
 * test_db.c
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "prof_cmocka.h"
#include <sys/stat.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>

#include "common.h"
#include "helpers.h"
#include "config/files.h"
#include "database/db.h"
#include "database/test_db.h"

void create_data_dir(void** state);
void remove_data_dir(void** state);

static void
_clean_test_db(void)
{
    auto_gchar gchar* db_file = files_get_data_path(DIR_DATABASE "/test_common.db");
    auto_gchar gchar* db_wal = files_get_data_path(DIR_DATABASE "/test_common.db-wal");
    auto_gchar gchar* db_shm = files_get_data_path(DIR_DATABASE "/test_common.db-shm");
    auto_gchar gchar* db_dir = files_get_data_path(DIR_DATABASE);

    g_unlink(db_file);
    g_unlink(db_wal);
    g_unlink(db_shm);
    if (db_dir) {
        g_rmdir(db_dir);
    }
}

void
test_db__open_and_close(void** state)
{
    create_data_dir(state);
    _clean_test_db();

    auto_gchar gchar* db_dir = files_get_data_path(DIR_DATABASE);
    create_dir(db_dir);

    auto_gchar gchar* db_file = files_get_data_path(DIR_DATABASE "/test_common.db");

    sqlite3* db = NULL;
    assert_true(prof_db_open(db_file, &db));
    assert_non_null(db);

    // Verify file permissions are 0600 (S_IRUSR | S_IWUSR)
    GStatBuf st;
    assert_int_equal(g_stat(db_file, &st), 0);
    assert_int_equal(st.st_mode & 0777, S_IRUSR | S_IWUSR);

    // Verify WAL mode was enabled
    sqlite3_stmt* stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(db, "PRAGMA journal_mode;", -1, &stmt, NULL), SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    const char* journal_mode = (const char*)sqlite3_column_text(stmt, 0);
    assert_string_equal(journal_mode, "wal");
    sqlite3_finalize(stmt);

    // Verify busy timeout was set to 5000ms
    assert_int_equal(sqlite3_prepare_v2(db, "PRAGMA busy_timeout;", -1, &stmt, NULL), SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_int_equal(sqlite3_column_int(stmt, 0), 5000);
    sqlite3_finalize(stmt);

    prof_db_close(&db);
    assert_null(db);

    // Verify prof_db_close safely handles connections with unfinalized statements via sqlite3_close_v2
    assert_true(prof_db_open(db_file, &db));
    sqlite3_stmt* dangling_stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(db, "SELECT 1;", -1, &dangling_stmt, NULL), SQLITE_OK);
    prof_db_close(&db);
    assert_null(db);
    assert_int_equal(sqlite3_finalize(dangling_stmt), SQLITE_OK);

    // Verify opening invalid path fails cleanly and leaves db NULL
    sqlite3* fail_db = NULL;
    assert_false(prof_db_open("/nonexistent_directory_for_prof_db_test/fail.db", &fail_db));
    assert_null(fail_db);

    _clean_test_db();
    remove_data_dir(state);
    g_rmdir("./tests/files");
}

void
test_db__exec_and_prepare(void** state)
{
    create_data_dir(state);
    _clean_test_db();

    auto_gchar gchar* db_dir = files_get_data_path(DIR_DATABASE);
    create_dir(db_dir);

    auto_gchar gchar* db_file = files_get_data_path(DIR_DATABASE "/test_common.db");

    sqlite3* db = NULL;
    assert_true(prof_db_open(db_file, &db));

    assert_true(prof_db_exec(db, "CREATE TABLE items (id INTEGER PRIMARY KEY, name TEXT);"));

    sqlite3_stmt* stmt = NULL;
    assert_true(prof_db_prepare(db, "INSERT INTO items (id, name) VALUES (?, ?);", &stmt));
    assert_non_null(stmt);

    sqlite3_bind_int64(stmt, 1, 1);
    sqlite3_bind_text(stmt, 2, "apple", -1, SQLITE_STATIC);
    assert_true(prof_db_step_done(db, stmt));

    sqlite3_bind_int64(stmt, 1, 2);
    sqlite3_bind_text(stmt, 2, "banana", -1, SQLITE_STATIC);
    assert_true(prof_db_step_done(db, stmt));

    prof_db_finalize(&stmt);
    assert_null(stmt);

    // Query back
    sqlite3_stmt* qstmt = NULL;
    assert_true(prof_db_prepare(db, "SELECT name FROM items WHERE id = ?;", &qstmt));
    sqlite3_bind_int64(qstmt, 1, 1);
    assert_int_equal(sqlite3_step(qstmt), SQLITE_ROW);
    assert_string_equal((const char*)sqlite3_column_text(qstmt, 0), "apple");
    prof_db_finalize(&qstmt);

    prof_db_close(&db);

    _clean_test_db();
    remove_data_dir(state);
    g_rmdir("./tests/files");
}

void
test_db__transactions(void** state)
{
    create_data_dir(state);
    _clean_test_db();

    auto_gchar gchar* db_dir = files_get_data_path(DIR_DATABASE);
    create_dir(db_dir);

    auto_gchar gchar* db_file = files_get_data_path(DIR_DATABASE "/test_common.db");

    sqlite3* db = NULL;
    assert_true(prof_db_open(db_file, &db));
    assert_true(prof_db_exec(db, "CREATE TABLE tx_test (val INTEGER);"));

    // Test Commit
    assert_true(prof_db_begin(db));
    assert_true(prof_db_exec(db, "INSERT INTO tx_test VALUES (100);"));
    assert_true(prof_db_commit(db));

    // Test Rollback
    assert_true(prof_db_begin(db));
    assert_true(prof_db_exec(db, "INSERT INTO tx_test VALUES (200);"));
    assert_true(prof_db_rollback(db));

    sqlite3_stmt* stmt = NULL;
    assert_true(prof_db_prepare(db, "SELECT COUNT(*) FROM tx_test;", &stmt));
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_int_equal(sqlite3_column_int(stmt, 0), 1);
    prof_db_finalize(&stmt);

    prof_db_close(&db);

    _clean_test_db();
    remove_data_dir(state);
    g_rmdir("./tests/files");
}

void
test_db__version(void** state)
{
    create_data_dir(state);
    _clean_test_db();

    auto_gchar gchar* db_dir = files_get_data_path(DIR_DATABASE);
    create_dir(db_dir);

    auto_gchar gchar* db_file = files_get_data_path(DIR_DATABASE "/test_common.db");

    sqlite3* db = NULL;
    assert_true(prof_db_open(db_file, &db));

    // Initially, no DbVersion table exists
    assert_int_equal(prof_db_get_version(db), -1);

    // Set version to 1
    assert_true(prof_db_set_version(db, 1));
    assert_int_equal(prof_db_get_version(db), 1);

    // Update version to 2
    assert_true(prof_db_set_version(db, 2));
    assert_int_equal(prof_db_get_version(db), 2);

    prof_db_close(&db);

    _clean_test_db();
    remove_data_dir(state);
    g_rmdir("./tests/files");
}
