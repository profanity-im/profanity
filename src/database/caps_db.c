/*
 * caps_db.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <errno.h>
#include <sqlite3.h>
#include <time.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "common.h"
#include "config/files.h"
#include "database/db.h"
#include "database/caps_db.h"
#include "log.h"
#include "ui/ui.h"

#define CAPS_DB_VERSION 1

static sqlite3* caps_db;
static sqlite3_stmt* stmt_contains;
static sqlite3_stmt* stmt_get;
static sqlite3_stmt* stmt_insert;
static sqlite3_stmt* stmt_update_last_seen;

static void
_caps_db_prune_stale(void)
{
    if (!caps_db) {
        return;
    }

    // Prune entries older than 90 days (90 * 86400 = 7776000 seconds)
    gint64 cutoff = (gint64)time(NULL) - 7776000;
    sqlite3_stmt* stmt_prune = NULL;
    const char* sql = "DELETE FROM caps_cache WHERE last_seen < ?;";
    if (prof_db_prepare(caps_db, sql, &stmt_prune)) {
        sqlite3_bind_int64(stmt_prune, 1, cutoff);
        if (prof_db_step_done(caps_db, stmt_prune)) {
            int changes = sqlite3_changes(caps_db);
            if (changes > 0) {
                log_info("Pruned %d stale capability cache entries older than 90 days", changes);
            }
        }
        prof_db_finalize(&stmt_prune);
    }
}

static void
_caps_migrate_legacy_keyfile(void)
{
    auto_gchar gchar* legacy_path = files_get_data_path(FILE_CAPSCACHE);
    if (!g_file_test(legacy_path, G_FILE_TEST_IS_REGULAR)) {
        return;
    }

    prof_keyfile_t legacy_kf;
    if (!load_data_keyfile(&legacy_kf, FILE_CAPSCACHE)) {
        free_keyfile(&legacy_kf);
        g_unlink(legacy_path);
        return;
    }

    GKeyFile* kf = legacy_kf.keyfile;
    if (!kf) {
        free_keyfile(&legacy_kf);
        g_unlink(legacy_path);
        return;
    }

    gsize num_groups = 0;
    auto_gcharv gchar** groups = g_key_file_get_groups(kf, &num_groups);
    if (!groups || num_groups == 0) {
        free_keyfile(&legacy_kf);
        g_unlink(legacy_path);
        return;
    }

    log_info("Migrating legacy capabilities cache (%u entries) to SQLite...", (guint)num_groups);

    if (!prof_db_begin(caps_db)) {
        log_error("Failed to begin transaction for caps cache migration");
        free_keyfile(&legacy_kf);
        return;
    }

    gint64 now = (gint64)time(NULL);
    gboolean success = TRUE;

    for (gsize i = 0; groups[i] != NULL; i++) {
        const gchar* ver = groups[i];

        auto_gchar gchar* category = g_key_file_get_string(kf, ver, "category", NULL);
        auto_gchar gchar* type = g_key_file_get_string(kf, ver, "type", NULL);
        auto_gchar gchar* name = g_key_file_get_string(kf, ver, "name", NULL);
        auto_gchar gchar* software = g_key_file_get_string(kf, ver, "software", NULL);
        auto_gchar gchar* software_version = g_key_file_get_string(kf, ver, "software_version", NULL);
        auto_gchar gchar* os = g_key_file_get_string(kf, ver, "os", NULL);
        auto_gchar gchar* os_version = g_key_file_get_string(kf, ver, "os_version", NULL);

        gsize features_len = 0;
        auto_gcharv gchar** features_list = g_key_file_get_string_list(kf, ver, "features", &features_len, NULL);
        auto_gchar gchar* features_str = NULL;
        if (features_list && features_len > 0) {
            features_str = g_strjoinv("\n", features_list);
        }

        sqlite3_bind_text(stmt_insert, 1, ver, -1, SQLITE_STATIC);
        if (category) {
            sqlite3_bind_text(stmt_insert, 2, category, -1, SQLITE_STATIC);
        } else {
            sqlite3_bind_null(stmt_insert, 2);
        }

        if (type) {
            sqlite3_bind_text(stmt_insert, 3, type, -1, SQLITE_STATIC);
        } else {
            sqlite3_bind_null(stmt_insert, 3);
        }

        if (name) {
            sqlite3_bind_text(stmt_insert, 4, name, -1, SQLITE_STATIC);
        } else {
            sqlite3_bind_null(stmt_insert, 4);
        }

        if (software) {
            sqlite3_bind_text(stmt_insert, 5, software, -1, SQLITE_STATIC);
        } else {
            sqlite3_bind_null(stmt_insert, 5);
        }

        if (software_version) {
            sqlite3_bind_text(stmt_insert, 6, software_version, -1, SQLITE_STATIC);
        } else {
            sqlite3_bind_null(stmt_insert, 6);
        }

        if (os) {
            sqlite3_bind_text(stmt_insert, 7, os, -1, SQLITE_STATIC);
        } else {
            sqlite3_bind_null(stmt_insert, 7);
        }

        if (os_version) {
            sqlite3_bind_text(stmt_insert, 8, os_version, -1, SQLITE_STATIC);
        } else {
            sqlite3_bind_null(stmt_insert, 8);
        }

        if (features_str) {
            sqlite3_bind_text(stmt_insert, 9, features_str, -1, SQLITE_STATIC);
        } else {
            sqlite3_bind_null(stmt_insert, 9);
        }

        sqlite3_bind_int64(stmt_insert, 10, now);

        if (!prof_db_step_done(caps_db, stmt_insert)) {
            success = FALSE;
            break;
        }
    }

    free_keyfile(&legacy_kf);

    if (success) {
        if (prof_db_commit(caps_db)) {
            auto_gchar gchar* migrated_path = g_strdup_printf("%s.migrated", legacy_path);
            if (g_rename(legacy_path, migrated_path) == 0) {
                log_info("Successfully migrated caps cache to SQLite. Legacy file renamed to: %s", migrated_path);
                cons_show("Capabilities cache migrated to SQLite database. Legacy file renamed to %s.", migrated_path);
            } else {
                log_warning("Failed to rename %s to %s: %s", legacy_path, migrated_path, g_strerror(errno));
            }
        } else {
            log_error("Failed to commit transaction for caps cache migration");
            prof_db_rollback(caps_db);
        }
    } else {
        prof_db_rollback(caps_db);
    }
}

gboolean
caps_db_init(void)
{
    auto_gchar gchar* db_dir = files_get_data_path(DIR_DATABASE);
    if (!create_dir(db_dir)) {
        log_error("Error creating database directory %s", db_dir);
        return FALSE;
    }

    auto_gchar gchar* db_path = g_build_filename(db_dir, "capscache.db", NULL);
    if (!prof_db_open(db_path, &caps_db)) {
        return FALSE;
    }

    const char* create_table_sql = "CREATE TABLE IF NOT EXISTS caps_cache ("
                                   "ver TEXT PRIMARY KEY, "
                                   "category TEXT, "
                                   "type TEXT, "
                                   "name TEXT, "
                                   "software TEXT, "
                                   "software_version TEXT, "
                                   "os TEXT, "
                                   "os_version TEXT, "
                                   "features TEXT, "
                                   "last_seen INTEGER NOT NULL);";

    if (!prof_db_exec(caps_db, create_table_sql)) {
        caps_db_close();
        return FALSE;
    }

    const char* create_index_sql = "CREATE INDEX IF NOT EXISTS idx_caps_cache_last_seen ON caps_cache(last_seen);";
    if (!prof_db_exec(caps_db, create_index_sql)) {
        caps_db_close();
        return FALSE;
    }

    const char* create_version_sql = "CREATE TABLE IF NOT EXISTS DbVersion (version INTEGER PRIMARY KEY);";
    if (!prof_db_exec(caps_db, create_version_sql)) {
        caps_db_close();
        return FALSE;
    }

    gint version = prof_db_get_version(caps_db);
    if (version == -1) {
        if (!prof_db_set_version(caps_db, CAPS_DB_VERSION)) {
            caps_db_close();
            return FALSE;
        }
    } else if (version > CAPS_DB_VERSION) {
        log_error("Capabilities cache database version %d is newer than supported version %d", version, CAPS_DB_VERSION);
        caps_db_close();
        return FALSE;
    }

    const char* contains_sql = "SELECT 1 FROM caps_cache WHERE ver = ? LIMIT 1;";
    if (!prof_db_prepare(caps_db, contains_sql, &stmt_contains)) {
        caps_db_close();
        return FALSE;
    }

    const char* get_sql = "SELECT category, type, name, software, software_version, os, os_version, features, last_seen FROM caps_cache WHERE ver = ?;";
    if (!prof_db_prepare(caps_db, get_sql, &stmt_get)) {
        caps_db_close();
        return FALSE;
    }

    const char* insert_sql = "INSERT OR REPLACE INTO caps_cache ("
                             "ver, category, type, name, software, software_version, os, os_version, features, last_seen"
                             ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    if (!prof_db_prepare(caps_db, insert_sql, &stmt_insert)) {
        caps_db_close();
        return FALSE;
    }

    const char* update_sql = "UPDATE caps_cache SET last_seen = ? WHERE ver = ?;";
    if (!prof_db_prepare(caps_db, update_sql, &stmt_update_last_seen)) {
        caps_db_close();
        return FALSE;
    }

    _caps_migrate_legacy_keyfile();
    _caps_db_prune_stale();

    return TRUE;
}

void
caps_db_close(void)
{
    prof_db_finalize(&stmt_contains);
    prof_db_finalize(&stmt_get);
    prof_db_finalize(&stmt_insert);
    prof_db_finalize(&stmt_update_last_seen);
    prof_db_close(&caps_db);
}

gboolean
caps_db_contains(const gchar* const ver)
{
    if (ver == NULL || !caps_db || !stmt_contains) {
        return FALSE;
    }

    sqlite3_bind_text(stmt_contains, 1, ver, -1, SQLITE_STATIC);

    gboolean found = FALSE;
    int rc = sqlite3_step(stmt_contains);
    if (rc == SQLITE_ROW) {
        found = TRUE;
    } else if (rc != SQLITE_DONE) {
        log_error("SQLite error checking caps cache for ver '%s': %s", ver, sqlite3_errmsg(caps_db));
    }

    sqlite3_reset(stmt_contains);
    sqlite3_clear_bindings(stmt_contains);
    return found;
}

EntityCapabilities*
caps_db_get(const gchar* const ver)
{
    if (ver == NULL || !caps_db || !stmt_get) {
        return NULL;
    }

    sqlite3_bind_text(stmt_get, 1, ver, -1, SQLITE_STATIC);

    EntityCapabilities* result = NULL;
    int rc = sqlite3_step(stmt_get);
    if (rc == SQLITE_ROW) {
        const gchar* category = (const gchar*)sqlite3_column_text(stmt_get, 0);
        const gchar* type = (const gchar*)sqlite3_column_text(stmt_get, 1);
        const gchar* name = (const gchar*)sqlite3_column_text(stmt_get, 2);
        const gchar* software = (const gchar*)sqlite3_column_text(stmt_get, 3);
        const gchar* software_version = (const gchar*)sqlite3_column_text(stmt_get, 4);
        const gchar* os = (const gchar*)sqlite3_column_text(stmt_get, 5);
        const gchar* os_version = (const gchar*)sqlite3_column_text(stmt_get, 6);
        const gchar* features_raw = (const gchar*)sqlite3_column_text(stmt_get, 7);
        gint64 last_seen = sqlite3_column_int64(stmt_get, 8);

        GSList* features = NULL;
        if (features_raw && *features_raw) {
            auto_gcharv gchar** feats = g_strsplit(features_raw, "\n", -1);
            for (gsize i = 0; feats[i] != NULL; i++) {
                if (*feats[i]) {
                    features = g_slist_prepend(features, feats[i]);
                }
            }
            features = g_slist_reverse(features);
            result = caps_create(category, type, name, software, software_version, os, os_version, features);
            g_slist_free(features);
        } else {
            result = caps_create(category, type, name, software, software_version, os, os_version, NULL);
        }

        sqlite3_reset(stmt_get);
        sqlite3_clear_bindings(stmt_get);

        // Throttle last_seen updates: only write to DB if entry was last seen > 1 day ago (86400s)
        // to avoid write-on-read lock contention while keeping 90-day pruning accurate.
        gint64 now = (gint64)time(NULL);
        if (stmt_update_last_seen && (now - last_seen > 86400)) {
            sqlite3_bind_int64(stmt_update_last_seen, 1, now);
            sqlite3_bind_text(stmt_update_last_seen, 2, ver, -1, SQLITE_STATIC);
            prof_db_step_done(caps_db, stmt_update_last_seen);
        }
    } else {
        if (rc != SQLITE_DONE) {
            log_error("SQLite error retrieving caps for ver '%s': %s", ver, sqlite3_errmsg(caps_db));
        }
        sqlite3_reset(stmt_get);
        sqlite3_clear_bindings(stmt_get);
    }
    return result;
}

void
caps_db_put(const gchar* const ver, EntityCapabilities* caps)
{
    if (ver == NULL || caps == NULL || !caps_db || !stmt_insert) {
        return;
    }

    auto_gchar gchar* features_str = NULL;
    if (caps->features) {
        GString* feat_buf = g_string_new(NULL);
        GSList* curr = caps->features;
        while (curr) {
            if (curr->data) {
                if (feat_buf->len > 0) {
                    g_string_append_c(feat_buf, '\n');
                }
                g_string_append(feat_buf, (const gchar*)curr->data);
            }
            curr = g_slist_next(curr);
        }
        features_str = g_string_free_and_steal(feat_buf);
    }

    sqlite3_bind_text(stmt_insert, 1, ver, -1, SQLITE_STATIC);

    if (caps->identity && caps->identity->category) {
        sqlite3_bind_text(stmt_insert, 2, caps->identity->category, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt_insert, 2);
    }

    if (caps->identity && caps->identity->type) {
        sqlite3_bind_text(stmt_insert, 3, caps->identity->type, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt_insert, 3);
    }

    if (caps->identity && caps->identity->name) {
        sqlite3_bind_text(stmt_insert, 4, caps->identity->name, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt_insert, 4);
    }

    if (caps->software_version && caps->software_version->software) {
        sqlite3_bind_text(stmt_insert, 5, caps->software_version->software, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt_insert, 5);
    }

    if (caps->software_version && caps->software_version->software_version) {
        sqlite3_bind_text(stmt_insert, 6, caps->software_version->software_version, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt_insert, 6);
    }

    if (caps->software_version && caps->software_version->os) {
        sqlite3_bind_text(stmt_insert, 7, caps->software_version->os, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt_insert, 7);
    }

    if (caps->software_version && caps->software_version->os_version) {
        sqlite3_bind_text(stmt_insert, 8, caps->software_version->os_version, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt_insert, 8);
    }

    if (features_str) {
        sqlite3_bind_text(stmt_insert, 9, features_str, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt_insert, 9);
    }

    sqlite3_bind_int64(stmt_insert, 10, (gint64)time(NULL));

    prof_db_step_done(caps_db, stmt_insert);
}
