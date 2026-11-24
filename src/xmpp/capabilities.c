/*
 * capabilities.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sqlite3.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <strophe.h>

#include "common.h"
#include "log.h"
#include "ui/ui.h"
#include "event/client_events.h"
#include "plugins/plugins.h"
#include "config/files.h"
#include "config/preferences.h"
#include "xmpp/xmpp.h"
#include "xmpp/stanza.h"
#include "xmpp/form.h"
#include "xmpp/capabilities.h"

static sqlite3* caps_db;
static sqlite3_stmt* stmt_contains;
static sqlite3_stmt* stmt_get;
static sqlite3_stmt* stmt_insert;
static sqlite3_stmt* stmt_update_last_seen;

static GHashTable* jid_to_ver;
static GHashTable* jid_to_caps;
static GHashTable* ver_to_caps;

static GHashTable* prof_features;
static gchar* my_sha1;

static EntityCapabilities* _caps_by_ver(const gchar* const ver);
static EntityCapabilities* _caps_by_jid(const gchar* const jid);
static EntityCapabilities* _caps_copy(EntityCapabilities* caps);

static gboolean
_caps_db_init(void)
{
    int ret = sqlite3_initialize();
    if (ret != SQLITE_OK) {
        log_error("Error initializing SQLite library: %d", ret);
        return FALSE;
    }

    auto_gchar gchar* db_dir = files_get_data_path(DIR_DATABASE);
    if (!create_dir(db_dir)) {
        log_error("Error creating database directory %s", db_dir);
        return FALSE;
    }

    auto_gchar gchar* db_path = g_build_filename(db_dir, "capscache.db", NULL);
    ret = sqlite3_open(db_path, &caps_db);
    if (ret != SQLITE_OK) {
        log_error("Error opening caps cache database %s: %s", db_path, sqlite3_errmsg(caps_db));
        return FALSE;
    }

    sqlite3_exec(caps_db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);

    char* err_msg = NULL;
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

    if (sqlite3_exec(caps_db, create_table_sql, NULL, NULL, &err_msg) != SQLITE_OK) {
        log_error("Error creating caps_cache table: %s", err_msg);
        sqlite3_free(err_msg);
        return FALSE;
    }

    const char* create_index_sql = "CREATE INDEX IF NOT EXISTS idx_caps_cache_last_seen ON caps_cache(last_seen);";
    if (sqlite3_exec(caps_db, create_index_sql, NULL, NULL, &err_msg) != SQLITE_OK) {
        log_error("Error creating caps_cache index: %s", err_msg);
        sqlite3_free(err_msg);
        return FALSE;
    }

    const char* contains_sql = "SELECT 1 FROM caps_cache WHERE ver = ? LIMIT 1;";
    if (sqlite3_prepare_v2(caps_db, contains_sql, -1, &stmt_contains, NULL) != SQLITE_OK) {
        log_error("Error preparing stmt_contains: %s", sqlite3_errmsg(caps_db));
        return FALSE;
    }

    const char* get_sql = "SELECT category, type, name, software, software_version, os, os_version, features FROM caps_cache WHERE ver = ?;";
    if (sqlite3_prepare_v2(caps_db, get_sql, -1, &stmt_get, NULL) != SQLITE_OK) {
        log_error("Error preparing stmt_get: %s", sqlite3_errmsg(caps_db));
        return FALSE;
    }

    const char* insert_sql = "INSERT OR REPLACE INTO caps_cache ("
                             "ver, category, type, name, software, software_version, os, os_version, features, last_seen"
                             ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(caps_db, insert_sql, -1, &stmt_insert, NULL) != SQLITE_OK) {
        log_error("Error preparing stmt_insert: %s", sqlite3_errmsg(caps_db));
        return FALSE;
    }

    const char* update_sql = "UPDATE caps_cache SET last_seen = ? WHERE ver = ?;";
    if (sqlite3_prepare_v2(caps_db, update_sql, -1, &stmt_update_last_seen, NULL) != SQLITE_OK) {
        log_error("Error preparing stmt_update_last_seen: %s", sqlite3_errmsg(caps_db));
        return FALSE;
    }

    return TRUE;
}

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
    if (sqlite3_prepare_v2(caps_db, sql, -1, &stmt_prune, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt_prune, 1, cutoff);
        if (sqlite3_step(stmt_prune) == SQLITE_DONE) {
            int changes = sqlite3_changes(caps_db);
            if (changes > 0) {
                log_info("Pruned %d stale capability cache entries older than 90 days", changes);
            }
        }
        sqlite3_finalize(stmt_prune);
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
        return;
    }

    GKeyFile* kf = legacy_kf.keyfile;
    if (!kf) {
        free_keyfile(&legacy_kf);
        return;
    }

    gsize num_groups = 0;
    auto_gcharv gchar** groups = g_key_file_get_groups(kf, &num_groups);
    if (!groups || num_groups == 0) {
        free_keyfile(&legacy_kf);
        return;
    }

    log_info("Migrating legacy capabilities cache (%u entries) to SQLite...", (guint)num_groups);

    char* err_msg = NULL;
    if (sqlite3_exec(caps_db, "BEGIN TRANSACTION;", NULL, NULL, &err_msg) != SQLITE_OK) {
        log_error("Failed to begin transaction for caps cache migration: %s", err_msg);
        sqlite3_free(err_msg);
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

        sqlite3_reset(stmt_insert);
        sqlite3_clear_bindings(stmt_insert);

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

        int rc = sqlite3_step(stmt_insert);
        if (rc != SQLITE_DONE) {
            log_error("Failed to insert migrated caps entry for '%s': %s", ver, sqlite3_errmsg(caps_db));
            success = FALSE;
            break;
        }
    }

    sqlite3_reset(stmt_insert);
    sqlite3_clear_bindings(stmt_insert);
    free_keyfile(&legacy_kf);

    if (success) {
        if (sqlite3_exec(caps_db, "COMMIT;", NULL, NULL, &err_msg) == SQLITE_OK) {
            auto_gchar gchar* migrated_path = g_strdup_printf("%s.migrated", legacy_path);
            if (g_rename(legacy_path, migrated_path) == 0) {
                log_info("Successfully migrated caps cache to SQLite. Legacy file renamed to: %s", migrated_path);
                cons_show("Capabilities cache migrated to SQLite database. Legacy file renamed to %s; you may delete it if you wish.", migrated_path);
            } else {
                log_warning("Failed to rename %s to %s: %s", legacy_path, migrated_path, g_strerror(errno));
            }
        } else {
            log_error("Failed to commit transaction for caps cache migration: %s", err_msg);
            sqlite3_free(err_msg);
            sqlite3_exec(caps_db, "ROLLBACK;", NULL, NULL, NULL);
        }
    } else {
        sqlite3_exec(caps_db, "ROLLBACK;", NULL, NULL, NULL);
    }
}

static void
_caps_close(void)
{
    caps_reset_ver();

    if (stmt_contains) {
        sqlite3_finalize(stmt_contains);
        stmt_contains = NULL;
    }
    if (stmt_get) {
        sqlite3_finalize(stmt_get);
        stmt_get = NULL;
    }
    if (stmt_insert) {
        sqlite3_finalize(stmt_insert);
        stmt_insert = NULL;
    }
    if (stmt_update_last_seen) {
        sqlite3_finalize(stmt_update_last_seen);
        stmt_update_last_seen = NULL;
    }
    if (caps_db) {
        sqlite3_close(caps_db);
        caps_db = NULL;
    }

    g_clear_pointer(&jid_to_ver, g_hash_table_destroy);
    g_clear_pointer(&jid_to_caps, g_hash_table_destroy);
    g_clear_pointer(&ver_to_caps, g_hash_table_destroy);
    g_clear_pointer(&prof_features, g_hash_table_destroy);
}

void
caps_init(void)
{
    log_info("Loading capabilities cache");

    prof_add_shutdown_routine(_caps_close);

    jid_to_ver = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    jid_to_caps = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)caps_destroy);
    ver_to_caps = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)caps_destroy);

    prof_features = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    g_hash_table_add(prof_features, g_strdup(STANZA_NS_CAPS));
    g_hash_table_add(prof_features, g_strdup(XMPP_NS_DISCO_INFO));
    g_hash_table_add(prof_features, g_strdup(XMPP_NS_DISCO_ITEMS));
    g_hash_table_add(prof_features, g_strdup(STANZA_NS_MUC));
    g_hash_table_add(prof_features, g_strdup(STANZA_NS_CONFERENCE));
    g_hash_table_add(prof_features, g_strdup(STANZA_NS_VERSION));
    g_hash_table_add(prof_features, g_strdup(STANZA_NS_CHATSTATES));
    g_hash_table_add(prof_features, g_strdup(STANZA_NS_PING));
    g_hash_table_add(prof_features, g_strdup(STANZA_NS_STABLE_ID));

    if (prefs_get_boolean(PREF_RECEIPTS_SEND)) {
        g_hash_table_add(prof_features, g_strdup(STANZA_NS_RECEIPTS));
    }

    if (prefs_get_boolean(PREF_LASTACTIVITY)) {
        g_hash_table_add(prof_features, g_strdup(STANZA_NS_LASTACTIVITY));
    }

    if (prefs_get_boolean(PREF_CORRECTION_ALLOW)) {
        g_hash_table_add(prof_features, g_strdup(STANZA_NS_LAST_MESSAGE_CORRECTION));
    }

    my_sha1 = NULL;

    if (_caps_db_init()) {
        _caps_migrate_legacy_keyfile();
        _caps_db_prune_stale();
    }
}

void
caps_add_feature(char* feature)
{
    if (g_hash_table_contains(prof_features, feature)) {
        return;
    }

    g_hash_table_add(prof_features, g_strdup(feature));

    caps_reset_ver();

    // resend presence to update server's disco info data for this client
    if (connection_get_status() == JABBER_CONNECTED) {
        resource_presence_t last_presence = accounts_get_last_presence(session_get_account_name());
        cl_ev_presence_send(last_presence, 0);
    }
}

void
caps_remove_feature(char* feature)
{
    if (!g_hash_table_contains(prof_features, feature)) {
        return;
    }

    g_hash_table_remove(prof_features, feature);

    caps_reset_ver();

    // resend presence to update server's disco info data for this client
    if (connection_get_status() == JABBER_CONNECTED) {
        resource_presence_t last_presence = accounts_get_last_presence(session_get_account_name());
        cl_ev_presence_send(last_presence, 0);
    }
}

GList*
caps_get_features(void)
{
    GList* result = NULL;

    GList* features_as_list = g_hash_table_get_keys(prof_features);
    GList* curr = features_as_list;
    while (curr) {
        result = g_list_append(result, g_strdup(curr->data));
        curr = g_list_next(curr);
    }
    g_list_free(features_as_list);

    GList* plugin_features = plugins_get_disco_features();
    curr = plugin_features;
    while (curr) {
        result = g_list_append(result, g_strdup(curr->data));
        curr = g_list_next(curr);
    }

    g_list_free(plugin_features);

    return result;
}

EntityCapabilities*
caps_create(const gchar* const category, const gchar* const type, const gchar* const name,
            const gchar* const software, const gchar* const software_version,
            const gchar* const os, const gchar* const os_version,
            GSList* features)
{
    EntityCapabilities* result = g_new0(EntityCapabilities, 1);

    if (category || type || name) {
        DiscoIdentity* identity = g_new0(DiscoIdentity, 1);
        identity->category = category ? g_strdup(category) : NULL;
        identity->type = type ? g_strdup(type) : NULL;
        identity->name = name ? g_strdup(name) : NULL;
        result->identity = identity;
    } else {
        result->identity = NULL;
    }

    if (software || software_version || os || os_version) {
        SoftwareVersion* software_versionp = g_new0(SoftwareVersion, 1);
        software_versionp->software = software ? g_strdup(software) : NULL;
        software_versionp->software_version = software_version ? g_strdup(software_version) : NULL;
        software_versionp->os = os ? g_strdup(os) : NULL;
        software_versionp->os_version = os_version ? g_strdup(os_version) : NULL;
        result->software_version = software_versionp;
    } else {
        result->software_version = NULL;
    }

    result->features = NULL;
    GSList* curr = features;
    while (curr) {
        result->features = g_slist_append(result->features, g_strdup(curr->data));
        curr = g_slist_next(curr);
    }

    return result;
}

void
caps_add_by_ver(const gchar* const ver, EntityCapabilities* caps)
{
    if (ver == NULL || caps == NULL) {
        return;
    }

    if (caps_cache_contains(ver)) {
        return;
    }

    if (ver_to_caps) {
        g_hash_table_insert(ver_to_caps, g_strdup(ver), _caps_copy(caps));
    }

    if (!caps_db || !stmt_insert) {
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

    sqlite3_reset(stmt_insert);
    sqlite3_clear_bindings(stmt_insert);

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

    int rc = sqlite3_step(stmt_insert);
    if (rc != SQLITE_DONE) {
        log_error("SQLite error inserting caps for ver '%s': %s", ver, sqlite3_errmsg(caps_db));
    }

    sqlite3_reset(stmt_insert);
    sqlite3_clear_bindings(stmt_insert);
}

void
caps_add_by_jid(const gchar* const jid, EntityCapabilities* caps)
{
    g_hash_table_insert(jid_to_caps, g_strdup(jid), caps);
}

void
caps_map_jid_to_ver(const gchar* const jid, const gchar* const ver)
{
    g_hash_table_insert(jid_to_ver, g_strdup(jid), g_strdup(ver));
}

gboolean
caps_cache_contains(const gchar* const ver)
{
    if (ver == NULL) {
        return FALSE;
    }

    if (ver_to_caps && g_hash_table_contains(ver_to_caps, ver)) {
        return TRUE;
    }

    if (!caps_db || !stmt_contains) {
        return FALSE;
    }

    sqlite3_reset(stmt_contains);
    sqlite3_clear_bindings(stmt_contains);
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
caps_lookup(const gchar* const jid)
{
    char* ver = g_hash_table_lookup(jid_to_ver, jid);
    if (ver) {
        EntityCapabilities* caps = _caps_by_ver(ver);
        if (caps) {
            log_debug("Capabilities lookup %s, found by verification string %s.", jid, ver);
            return caps;
        }
    } else {
        EntityCapabilities* caps = _caps_by_jid(jid);
        if (caps) {
            log_debug("Capabilities lookup %s, found by JID.", jid);
            return _caps_copy(caps);
        }
    }

    log_debug("Capabilities lookup %s, none found.", jid);
    return NULL;
}

gboolean
caps_jid_has_feature(const gchar* const jid, const gchar* const feature)
{
    EntityCapabilities* caps = caps_lookup(jid);

    if (caps == NULL) {
        return FALSE;
    }

    GSList* found = g_slist_find_custom(caps->features, feature, (GCompareFunc)g_strcmp0);
    gboolean result = found != NULL;

    caps_destroy(caps);

    return result;
}

gchar*
caps_get_my_sha1(xmpp_ctx_t* const ctx)
{
    if (my_sha1 == NULL) {
        xmpp_stanza_t* query = stanza_create_caps_query_element(ctx);
        my_sha1 = stanza_create_caps_sha1_from_query(query);
        xmpp_stanza_release(query);
    }

    return my_sha1;
}

void
caps_reset_ver(void)
{
    g_clear_pointer(&my_sha1, g_free);
}

static EntityCapabilities*
_caps_by_ver(const gchar* const ver)
{
    if (ver == NULL) {
        return NULL;
    }

    if (ver_to_caps) {
        EntityCapabilities* in_mem = g_hash_table_lookup(ver_to_caps, ver);
        if (in_mem) {
            return _caps_copy(in_mem);
        }
    }

    if (!caps_db || !stmt_get) {
        return NULL;
    }

    sqlite3_reset(stmt_get);
    sqlite3_clear_bindings(stmt_get);
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

        GSList* features = NULL;
        if (features_raw && *features_raw) {
            auto_gcharv gchar** feats = g_strsplit(features_raw, "\n", -1);
            for (gsize i = 0; feats[i] != NULL; i++) {
                if (*feats[i]) {
                    features = g_slist_append(features, feats[i]);
                }
            }
            result = caps_create(category, type, name, software, software_version, os, os_version, features);
            g_slist_free(features);
        } else {
            result = caps_create(category, type, name, software, software_version, os, os_version, NULL);
        }

        if (result && ver_to_caps) {
            g_hash_table_insert(ver_to_caps, g_strdup(ver), _caps_copy(result));
        }

        if (stmt_update_last_seen) {
            sqlite3_reset(stmt_update_last_seen);
            sqlite3_clear_bindings(stmt_update_last_seen);
            sqlite3_bind_int64(stmt_update_last_seen, 1, (gint64)time(NULL));
            sqlite3_bind_text(stmt_update_last_seen, 2, ver, -1, SQLITE_STATIC);
            sqlite3_step(stmt_update_last_seen);
            sqlite3_reset(stmt_update_last_seen);
            sqlite3_clear_bindings(stmt_update_last_seen);
        }
    } else if (rc != SQLITE_DONE) {
        log_error("SQLite error retrieving caps for ver '%s': %s", ver, sqlite3_errmsg(caps_db));
    }

    sqlite3_reset(stmt_get);
    sqlite3_clear_bindings(stmt_get);
    return result;
}

static EntityCapabilities*
_caps_by_jid(const gchar* const jid)
{
    return g_hash_table_lookup(jid_to_caps, jid);
}

static EntityCapabilities*
_caps_copy(EntityCapabilities* caps)
{
    if (!caps) {
        return NULL;
    }

    const gchar* const category = caps->identity ? caps->identity->category : NULL;
    const gchar* const type = caps->identity ? caps->identity->type : NULL;
    const gchar* const name = caps->identity ? caps->identity->name : NULL;

    const gchar* const software = caps->software_version ? caps->software_version->software : NULL;
    const gchar* const software_version = caps->software_version ? caps->software_version->software_version : NULL;
    const gchar* const os = caps->software_version ? caps->software_version->os : NULL;
    const gchar* const os_version = caps->software_version ? caps->software_version->os_version : NULL;

    return caps_create(category, type, name, software, software_version, os, os_version, caps->features);
}

static void
_disco_identity_destroy(DiscoIdentity* disco_identity)
{
    if (disco_identity) {
        g_free(disco_identity->category);
        g_free(disco_identity->name);
        g_free(disco_identity->type);
        g_free(disco_identity);
    }
}

static void
_software_version_destroy(SoftwareVersion* software_version)
{
    if (software_version) {
        g_free(software_version->software);
        g_free(software_version->software_version);
        g_free(software_version->os);
        g_free(software_version->os_version);
        g_free(software_version);
    }
}

void
caps_destroy(EntityCapabilities* caps)
{
    if (caps) {
        _disco_identity_destroy(caps->identity);
        _software_version_destroy(caps->software_version);
        if (caps->features) {
            g_slist_free_full(caps->features, g_free);
        }
        g_free(caps);
    }
}
