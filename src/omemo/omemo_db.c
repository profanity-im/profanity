/*
 * omemo_db.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sqlite3.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "common.h"
#include "log.h"
#include "ui/ui.h"
#include "config/files.h"
#include "database/db.h"
#include "omemo/omemo.h"
#include "omemo/store.h"
#include "omemo/omemo_db.h"

#define OMEMO_DB_VERSION 1

static sqlite3* omemo_db = NULL;

static sqlite3_stmt* stmt_save_identity;
static sqlite3_stmt* stmt_load_identity;
static sqlite3_stmt* stmt_has_identity;

static sqlite3_stmt* stmt_save_pre_key;
static sqlite3_stmt* stmt_remove_pre_key;
static sqlite3_stmt* stmt_load_all_pre_keys;

static sqlite3_stmt* stmt_save_signed_pre_key;
static sqlite3_stmt* stmt_remove_signed_pre_key;
static sqlite3_stmt* stmt_load_all_signed_pre_keys;

static sqlite3_stmt* stmt_save_session;
static sqlite3_stmt* stmt_delete_session;
static sqlite3_stmt* stmt_delete_all_sessions;
static sqlite3_stmt* stmt_load_all_sessions;

static sqlite3_stmt* stmt_save_trust;
static sqlite3_stmt* stmt_remove_trust;
static sqlite3_stmt* stmt_load_all_trust;
static sqlite3_stmt* stmt_is_verified;
static sqlite3_stmt* stmt_set_verified;
static sqlite3_stmt* stmt_remove_verified;

static sqlite3_stmt* stmt_save_device;
static sqlite3_stmt* stmt_load_all_devices;

static gboolean
_init_tables(void)
{
    const char* schema = "CREATE TABLE IF NOT EXISTS omemo_identity ("
                         "device_id INTEGER PRIMARY KEY, "
                         "registration_id INTEGER NOT NULL, "
                         "public_key BLOB NOT NULL, "
                         "private_key BLOB NOT NULL);"

                         "CREATE TABLE IF NOT EXISTS omemo_pre_keys ("
                         "pre_key_id INTEGER PRIMARY KEY, "
                         "record BLOB NOT NULL);"

                         "CREATE TABLE IF NOT EXISTS omemo_signed_pre_keys ("
                         "signed_pre_key_id INTEGER PRIMARY KEY, "
                         "record BLOB NOT NULL);"

                         "CREATE TABLE IF NOT EXISTS omemo_sessions ("
                         "barejid TEXT NOT NULL, "
                         "device_id INTEGER NOT NULL, "
                         "record BLOB NOT NULL, "
                         "PRIMARY KEY (barejid, device_id));"

                         "CREATE TABLE IF NOT EXISTS omemo_trust ("
                         "barejid TEXT NOT NULL, "
                         "device_id INTEGER NOT NULL, "
                         "identity_key BLOB NOT NULL, "
                         "PRIMARY KEY (barejid, device_id));"

                         "CREATE TABLE IF NOT EXISTS omemo_verified ("
                         "barejid TEXT PRIMARY KEY);"

                         "CREATE TABLE IF NOT EXISTS omemo_known_devices ("
                         "barejid TEXT NOT NULL, "
                         "device_id INTEGER NOT NULL, "
                         "fingerprint TEXT NOT NULL, "
                         "PRIMARY KEY (barejid, device_id));"

                         "CREATE TABLE IF NOT EXISTS DbVersion ("
                         "version INTEGER PRIMARY KEY);";

    if (!prof_db_exec(omemo_db, schema)) {
        return FALSE;
    }

    gint version = prof_db_get_version(omemo_db);
    if (version == -1) {
        if (!prof_db_set_version(omemo_db, OMEMO_DB_VERSION)) {
            return FALSE;
        }
    } else if (version > OMEMO_DB_VERSION) {
        log_error("[OMEMO][DB] Database version %d is newer than supported version %d", version, OMEMO_DB_VERSION);
        return FALSE;
    }

    return TRUE;
}

static gboolean
_prepare_all_stmts(void)
{
    gboolean ok = TRUE;

    ok &= prof_db_prepare(omemo_db, "INSERT OR REPLACE INTO omemo_identity (device_id, registration_id, public_key, private_key) VALUES (?, ?, ?, ?);", &stmt_save_identity);
    ok &= prof_db_prepare(omemo_db, "SELECT device_id, registration_id, public_key, private_key FROM omemo_identity LIMIT 1;", &stmt_load_identity);
    ok &= prof_db_prepare(omemo_db, "SELECT 1 FROM omemo_identity LIMIT 1;", &stmt_has_identity);

    ok &= prof_db_prepare(omemo_db, "INSERT OR REPLACE INTO omemo_pre_keys (pre_key_id, record) VALUES (?, ?);", &stmt_save_pre_key);
    ok &= prof_db_prepare(omemo_db, "DELETE FROM omemo_pre_keys WHERE pre_key_id = ?;", &stmt_remove_pre_key);
    ok &= prof_db_prepare(omemo_db, "SELECT pre_key_id, record FROM omemo_pre_keys;", &stmt_load_all_pre_keys);

    ok &= prof_db_prepare(omemo_db, "INSERT OR REPLACE INTO omemo_signed_pre_keys (signed_pre_key_id, record) VALUES (?, ?);", &stmt_save_signed_pre_key);
    ok &= prof_db_prepare(omemo_db, "DELETE FROM omemo_signed_pre_keys WHERE signed_pre_key_id = ?;", &stmt_remove_signed_pre_key);
    ok &= prof_db_prepare(omemo_db, "SELECT signed_pre_key_id, record FROM omemo_signed_pre_keys;", &stmt_load_all_signed_pre_keys);

    ok &= prof_db_prepare(omemo_db, "INSERT OR REPLACE INTO omemo_sessions (barejid, device_id, record) VALUES (?, ?, ?);", &stmt_save_session);
    ok &= prof_db_prepare(omemo_db, "DELETE FROM omemo_sessions WHERE barejid = ? AND device_id = ?;", &stmt_delete_session);
    ok &= prof_db_prepare(omemo_db, "DELETE FROM omemo_sessions WHERE barejid = ?;", &stmt_delete_all_sessions);
    ok &= prof_db_prepare(omemo_db, "SELECT barejid, device_id, record FROM omemo_sessions;", &stmt_load_all_sessions);

    ok &= prof_db_prepare(omemo_db, "INSERT OR REPLACE INTO omemo_trust (barejid, device_id, identity_key) VALUES (?, ?, ?);", &stmt_save_trust);
    ok &= prof_db_prepare(omemo_db, "DELETE FROM omemo_trust WHERE barejid = ? AND device_id = ?;", &stmt_remove_trust);
    ok &= prof_db_prepare(omemo_db, "SELECT barejid, device_id, identity_key FROM omemo_trust;", &stmt_load_all_trust);
    ok &= prof_db_prepare(omemo_db, "SELECT 1 FROM omemo_verified WHERE barejid = ? LIMIT 1;", &stmt_is_verified);
    ok &= prof_db_prepare(omemo_db, "INSERT OR REPLACE INTO omemo_verified (barejid) VALUES (?);", &stmt_set_verified);
    ok &= prof_db_prepare(omemo_db, "DELETE FROM omemo_verified WHERE barejid = ?;", &stmt_remove_verified);

    ok &= prof_db_prepare(omemo_db, "INSERT OR REPLACE INTO omemo_known_devices (barejid, device_id, fingerprint) VALUES (?, ?, ?);", &stmt_save_device);
    ok &= prof_db_prepare(omemo_db, "SELECT barejid, device_id, fingerprint FROM omemo_known_devices;", &stmt_load_all_devices);

    return ok;
}

gboolean
omemo_db_init(const ProfAccount* account)
{
    if (omemo_db) {
        return TRUE;
    }

    if (!account || !account->jid) {
        log_error("[OMEMO][DB] No account JID provided for database initialization");
        return FALSE;
    }

    auto_gchar gchar* db_path = files_file_in_account_data_path(DIR_DATABASE, account->jid, "omemo.db");
    if (!db_path) {
        log_error("[OMEMO][DB] Failed to get database path for account %s", account->jid);
        return FALSE;
    }

    if (!prof_db_open(db_path, &omemo_db)) {
        return FALSE;
    }

    if (!_init_tables()) {
        omemo_db_close();
        return FALSE;
    }

    if (!_prepare_all_stmts()) {
        omemo_db_close();
        return FALSE;
    }

    return TRUE;
}

void
omemo_db_close(void)
{
    prof_db_finalize(&stmt_save_identity);
    prof_db_finalize(&stmt_load_identity);
    prof_db_finalize(&stmt_has_identity);

    prof_db_finalize(&stmt_save_pre_key);
    prof_db_finalize(&stmt_remove_pre_key);
    prof_db_finalize(&stmt_load_all_pre_keys);

    prof_db_finalize(&stmt_save_signed_pre_key);
    prof_db_finalize(&stmt_remove_signed_pre_key);
    prof_db_finalize(&stmt_load_all_signed_pre_keys);

    prof_db_finalize(&stmt_save_session);
    prof_db_finalize(&stmt_delete_session);
    prof_db_finalize(&stmt_delete_all_sessions);
    prof_db_finalize(&stmt_load_all_sessions);

    prof_db_finalize(&stmt_save_trust);
    prof_db_finalize(&stmt_remove_trust);
    prof_db_finalize(&stmt_load_all_trust);
    prof_db_finalize(&stmt_is_verified);
    prof_db_finalize(&stmt_set_verified);
    prof_db_finalize(&stmt_remove_verified);

    prof_db_finalize(&stmt_save_device);
    prof_db_finalize(&stmt_load_all_devices);

    prof_db_close(&omemo_db);
}

void
omemo_db_migrate_legacy_files(const ProfAccount* account)
{
    if (!omemo_db || !account || !account->jid) {
        return;
    }

    auto_gchar gchar* omemo_dir = files_file_in_account_data_path(DIR_OMEMO, account->jid, NULL);
    if (!omemo_dir) {
        return;
    }

    auto_gchar gchar* identity_path = g_build_filename(omemo_dir, "identity.txt", NULL);
    if (!g_file_test(identity_path, G_FILE_TEST_IS_REGULAR)) {
        return;
    }

    prof_keyfile_t kf_identity;
    if (!load_custom_keyfile(&kf_identity, g_strdup(identity_path))) {
        return;
    }

    log_info("[OMEMO] Migrating legacy OMEMO files for account %s to omemo.db...", account->jid);

    if (!prof_db_begin(omemo_db)) {
        log_error("[OMEMO] Failed to begin transaction for OMEMO migration");
        free_keyfile(&kf_identity);
        return;
    }

    gboolean success = TRUE;

    // 1. Identity key & registration ID & device ID
    GError* gerr = NULL;
    uint32_t device_id = (uint32_t)g_key_file_get_uint64(kf_identity.keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_DEVICE_ID, &gerr);
    if (gerr) {
        log_error("[OMEMO] Migration error reading device_id: %s", gerr->message);
        g_clear_error(&gerr);
        success = FALSE;
    }

    uint32_t reg_id = (uint32_t)g_key_file_get_uint64(kf_identity.keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_REGISTRATION_ID, &gerr);
    if (gerr) {
        log_error("[OMEMO] Migration error reading registration_id: %s", gerr->message);
        g_clear_error(&gerr);
        success = FALSE;
    }

    auto_gchar gchar* pub_b64 = g_key_file_get_string(kf_identity.keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_IDENTITY_KEY_PUBLIC, &gerr);
    if (gerr) {
        log_error("[OMEMO] Migration error reading identity_key_public: %s", gerr->message);
        g_clear_error(&gerr);
        success = FALSE;
    }

    auto_gchar gchar* priv_b64 = g_key_file_get_string(kf_identity.keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_IDENTITY_KEY_PRIVATE, &gerr);
    if (gerr) {
        log_error("[OMEMO] Migration error reading identity_key_private: %s", gerr->message);
        g_clear_error(&gerr);
        success = FALSE;
    }

    if (success && pub_b64 && priv_b64) {
        size_t pub_len = 0, priv_len = 0;
        auto_guchar guchar* pub_raw = g_base64_decode(pub_b64, &pub_len);
        auto_guchar guchar* priv_raw = g_base64_decode(priv_b64, &priv_len);
        if (!omemo_db_save_identity(device_id, reg_id, pub_raw, pub_len, priv_raw, priv_len)) {
            success = FALSE;
        }
        if (priv_raw) {
            prof_wipe_memory(priv_raw, priv_len);
        }
        if (priv_b64) {
            prof_wipe_string(priv_b64);
        }
    } else {
        success = FALSE;
    }

    // 2. Pre-keys
    if (success) {
        gsize prekeys_len = 0;
        auto_gcharv gchar** prekey_keys = g_key_file_get_keys(kf_identity.keyfile, OMEMO_STORE_GROUP_PREKEYS, &prekeys_len, NULL);
        if (prekey_keys) {
            for (gsize i = 0; prekey_keys[i] != NULL; i++) {
                uint32_t pk_id = (uint32_t)strtoul(prekey_keys[i], NULL, 10);
                auto_gchar gchar* pk_b64 = g_key_file_get_string(kf_identity.keyfile, OMEMO_STORE_GROUP_PREKEYS, prekey_keys[i], NULL);
                if (pk_b64) {
                    size_t pk_len = 0;
                    auto_guchar guchar* pk_raw = g_base64_decode(pk_b64, &pk_len);
                    if (!omemo_db_save_pre_key(pk_id, pk_raw, pk_len)) {
                        success = FALSE;
                        break;
                    }
                }
            }
        }
    }

    // 3. Signed pre-keys
    if (success) {
        gsize signed_len = 0;
        auto_gcharv gchar** signed_keys = g_key_file_get_keys(kf_identity.keyfile, OMEMO_STORE_GROUP_SIGNED_PREKEYS, &signed_len, NULL);
        if (signed_keys) {
            for (gsize i = 0; signed_keys[i] != NULL; i++) {
                uint32_t spk_id = (uint32_t)strtoul(signed_keys[i], NULL, 10);
                auto_gchar gchar* spk_b64 = g_key_file_get_string(kf_identity.keyfile, OMEMO_STORE_GROUP_SIGNED_PREKEYS, signed_keys[i], NULL);
                if (spk_b64) {
                    size_t spk_len = 0;
                    auto_guchar guchar* spk_raw = g_base64_decode(spk_b64, &spk_len);
                    if (!omemo_db_save_signed_pre_key(spk_id, spk_raw, spk_len)) {
                        success = FALSE;
                        break;
                    }
                }
            }
        }
    }

    free_keyfile(&kf_identity);

    // 4. Sessions from sessions.txt
    auto_gchar gchar* sessions_path = g_build_filename(omemo_dir, "sessions.txt", NULL);
    prof_keyfile_t kf_sessions;
    if (success && g_file_test(sessions_path, G_FILE_TEST_IS_REGULAR) && load_custom_keyfile(&kf_sessions, g_strdup(sessions_path))) {
        gsize num_groups = 0;
        auto_gcharv gchar** groups = g_key_file_get_groups(kf_sessions.keyfile, &num_groups);
        if (groups) {
            for (gsize i = 0; groups[i] != NULL && success; i++) {
                const char* jid = groups[i];
                auto_gcharv gchar** dev_keys = g_key_file_get_keys(kf_sessions.keyfile, jid, NULL, NULL);
                if (dev_keys) {
                    for (gsize j = 0; dev_keys[j] != NULL; j++) {
                        uint32_t dev_id = (uint32_t)strtoul(dev_keys[j], NULL, 10);
                        auto_gchar gchar* s_b64 = g_key_file_get_string(kf_sessions.keyfile, jid, dev_keys[j], NULL);
                        if (s_b64) {
                            size_t s_len = 0;
                            auto_guchar guchar* s_raw = g_base64_decode(s_b64, &s_len);
                            if (!omemo_db_save_session(jid, dev_id, s_raw, s_len)) {
                                success = FALSE;
                                break;
                            }
                        }
                    }
                }
            }
        }
        free_keyfile(&kf_sessions);
    }

    // 5. Trust & verified contacts from trust.txt
    auto_gchar gchar* trust_path = g_build_filename(omemo_dir, "trust.txt", NULL);
    prof_keyfile_t kf_trust;
    if (success && g_file_test(trust_path, G_FILE_TEST_IS_REGULAR) && load_custom_keyfile(&kf_trust, g_strdup(trust_path))) {
        gsize num_groups = 0;
        auto_gcharv gchar** groups = g_key_file_get_groups(kf_trust.keyfile, &num_groups);
        if (groups) {
            for (gsize i = 0; groups[i] != NULL && success; i++) {
                const char* grp = groups[i];
                if (g_strcmp0(grp, OMEMO_TRUST_GROUP_VERIFIED) == 0) {
                    auto_gcharv gchar** ver_keys = g_key_file_get_keys(kf_trust.keyfile, grp, NULL, NULL);
                    if (ver_keys) {
                        for (gsize j = 0; ver_keys[j] != NULL; j++) {
                            gboolean ver_val = g_key_file_get_boolean(kf_trust.keyfile, grp, ver_keys[j], NULL);
                            if (ver_val) {
                                omemo_db_set_contact_verified(ver_keys[j], TRUE);
                            }
                        }
                    }
                } else {
                    auto_gcharv gchar** dev_keys = g_key_file_get_keys(kf_trust.keyfile, grp, NULL, NULL);
                    if (dev_keys) {
                        for (gsize j = 0; dev_keys[j] != NULL; j++) {
                            uint32_t dev_id = (uint32_t)strtoul(dev_keys[j], NULL, 10);
                            auto_gchar gchar* key_b64 = g_key_file_get_string(kf_trust.keyfile, grp, dev_keys[j], NULL);
                            if (key_b64) {
                                size_t key_len = 0;
                                auto_guchar guchar* key_raw = g_base64_decode(key_b64, &key_len);
                                if (!omemo_db_save_trust(grp, dev_id, key_raw, key_len)) {
                                    success = FALSE;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        free_keyfile(&kf_trust);
    }

    // 6. Known devices from known_devices.txt
    auto_gchar gchar* devices_path = g_build_filename(omemo_dir, "known_devices.txt", NULL);
    prof_keyfile_t kf_devices;
    if (success && g_file_test(devices_path, G_FILE_TEST_IS_REGULAR) && load_custom_keyfile(&kf_devices, g_strdup(devices_path))) {
        gsize num_groups = 0;
        auto_gcharv gchar** groups = g_key_file_get_groups(kf_devices.keyfile, &num_groups);
        if (groups) {
            for (gsize i = 0; groups[i] != NULL && success; i++) {
                const char* jid = groups[i];
                auto_gcharv gchar** dev_keys = g_key_file_get_keys(kf_devices.keyfile, jid, NULL, NULL);
                if (dev_keys) {
                    for (gsize j = 0; dev_keys[j] != NULL; j++) {
                        uint32_t dev_id = (uint32_t)strtoul(dev_keys[j], NULL, 10);
                        auto_gchar gchar* fp = g_key_file_get_string(kf_devices.keyfile, jid, dev_keys[j], NULL);
                        if (fp) {
                            if (!omemo_db_save_device(jid, dev_id, fp)) {
                                success = FALSE;
                                break;
                            }
                        }
                    }
                }
            }
        }
        free_keyfile(&kf_devices);
    }

    if (success) {
        if (prof_db_commit(omemo_db)) {
            const char* const files_to_rename[] = {
                "identity.txt", "sessions.txt", "trust.txt", "known_devices.txt", NULL
            };
            for (int k = 0; files_to_rename[k] != NULL; k++) {
                auto_gchar gchar* old_file = g_build_filename(omemo_dir, files_to_rename[k], NULL);
                if (g_file_test(old_file, G_FILE_TEST_IS_REGULAR)) {
                    auto_gchar gchar* new_file = g_strdup_printf("%s.migrated", old_file);
                    if (g_rename(old_file, new_file) != 0) {
                        log_warning("[OMEMO] Failed to rename %s to %s: %s", old_file, new_file, g_strerror(errno));
                    }
                }
            }
            log_info("[OMEMO] Successfully migrated OMEMO data to SQLite database (omemo.db)");
            cons_show("OMEMO storage migrated to SQLite database (omemo.db). Legacy files renamed to *.migrated.");
        } else {
            log_error("[OMEMO] Failed to commit transaction for OMEMO migration");
            prof_db_rollback(omemo_db);
        }
    } else {
        prof_db_rollback(omemo_db);
        log_error("[OMEMO] Migration of OMEMO data failed; transaction rolled back, legacy files untouched.");
    }
}

gboolean
omemo_db_has_identity(void)
{
    if (!omemo_db || !stmt_has_identity) {
        return FALSE;
    }

    int rc = sqlite3_step(stmt_has_identity);
    sqlite3_reset(stmt_has_identity);

    return (rc == SQLITE_ROW);
}

gboolean
omemo_db_save_identity(uint32_t device_id, uint32_t registration_id,
                       const uint8_t* pub_key, size_t pub_key_len,
                       const uint8_t* priv_key, size_t priv_key_len)
{
    if (!omemo_db || !stmt_save_identity || !pub_key || !priv_key) {
        return FALSE;
    }

    sqlite3_bind_int64(stmt_save_identity, 1, (sqlite3_int64)device_id);
    sqlite3_bind_int64(stmt_save_identity, 2, (sqlite3_int64)registration_id);
    sqlite3_bind_blob(stmt_save_identity, 3, pub_key, (int)pub_key_len, SQLITE_STATIC);
    sqlite3_bind_blob(stmt_save_identity, 4, priv_key, (int)priv_key_len, SQLITE_STATIC);

    return prof_db_step_done(omemo_db, stmt_save_identity);
}

gboolean
omemo_db_load_identity(uint32_t* device_id, uint32_t* registration_id,
                       uint8_t** pub_key, size_t* pub_key_len,
                       uint8_t** priv_key, size_t* priv_key_len)
{
    if (!omemo_db || !stmt_load_identity) {
        return FALSE;
    }

    int rc = sqlite3_step(stmt_load_identity);

    if (rc == SQLITE_ROW) {
        const void* pub_blob = sqlite3_column_blob(stmt_load_identity, 2);
        int pub_bytes = sqlite3_column_bytes(stmt_load_identity, 2);
        const void* priv_blob = sqlite3_column_blob(stmt_load_identity, 3);
        int priv_bytes = sqlite3_column_bytes(stmt_load_identity, 3);

        if (pub_key && (!pub_blob || pub_bytes <= 0)) {
            sqlite3_reset(stmt_load_identity);
            return FALSE;
        }

        if (priv_key && (!priv_blob || priv_bytes <= 0)) {
            sqlite3_reset(stmt_load_identity);
            return FALSE;
        }

        if (device_id) {
            *device_id = (uint32_t)sqlite3_column_int64(stmt_load_identity, 0);
        }
        if (registration_id) {
            *registration_id = (uint32_t)sqlite3_column_int64(stmt_load_identity, 1);
        }

        if (pub_key && pub_key_len) {
            *pub_key = g_memdup2(pub_blob, pub_bytes);
            *pub_key_len = (size_t)pub_bytes;
        }

        if (priv_key && priv_key_len) {
            *priv_key = g_memdup2(priv_blob, priv_bytes);
            *priv_key_len = (size_t)priv_bytes;
        }

        sqlite3_reset(stmt_load_identity);
        return TRUE;
    }

    sqlite3_reset(stmt_load_identity);
    return FALSE;
}

gboolean
omemo_db_save_pre_key(uint32_t pre_key_id, const uint8_t* record, size_t record_len)
{
    if (!omemo_db || !stmt_save_pre_key || !record) {
        return FALSE;
    }

    sqlite3_bind_int64(stmt_save_pre_key, 1, (sqlite3_int64)pre_key_id);
    sqlite3_bind_blob(stmt_save_pre_key, 2, record, (int)record_len, SQLITE_STATIC);

    return prof_db_step_done(omemo_db, stmt_save_pre_key);
}

gboolean
omemo_db_remove_pre_key(uint32_t pre_key_id)
{
    if (!omemo_db || !stmt_remove_pre_key) {
        return FALSE;
    }

    sqlite3_bind_int64(stmt_remove_pre_key, 1, (sqlite3_int64)pre_key_id);

    return prof_db_step_done(omemo_db, stmt_remove_pre_key);
}

gint
omemo_db_load_all_pre_keys(GHashTable* pre_key_store, uint32_t* max_pre_key_id)
{
    if (!omemo_db || !stmt_load_all_pre_keys || !pre_key_store) {
        return 0;
    }

    gint count = 0;
    uint32_t max_id = 0;

    while (sqlite3_step(stmt_load_all_pre_keys) == SQLITE_ROW) {
        uint32_t id = (uint32_t)sqlite3_column_int64(stmt_load_all_pre_keys, 0);
        const void* blob = sqlite3_column_blob(stmt_load_all_pre_keys, 1);
        int bytes = sqlite3_column_bytes(stmt_load_all_pre_keys, 1);

        if (blob && bytes > 0) {
            signal_buffer* buffer = signal_buffer_create((const uint8_t*)blob, (size_t)bytes);
            g_hash_table_insert(pre_key_store, GINT_TO_POINTER(id), buffer);
            if (id > max_id) {
                max_id = id;
            }
            count++;
        }
    }

    sqlite3_reset(stmt_load_all_pre_keys);

    if (max_pre_key_id) {
        *max_pre_key_id = max_id;
    }

    return count;
}

gboolean
omemo_db_save_signed_pre_key(uint32_t signed_pre_key_id, const uint8_t* record, size_t record_len)
{
    if (!omemo_db || !stmt_save_signed_pre_key || !record) {
        return FALSE;
    }

    sqlite3_bind_int64(stmt_save_signed_pre_key, 1, (sqlite3_int64)signed_pre_key_id);
    sqlite3_bind_blob(stmt_save_signed_pre_key, 2, record, (int)record_len, SQLITE_STATIC);

    return prof_db_step_done(omemo_db, stmt_save_signed_pre_key);
}

gboolean
omemo_db_remove_signed_pre_key(uint32_t signed_pre_key_id)
{
    if (!omemo_db || !stmt_remove_signed_pre_key) {
        return FALSE;
    }

    sqlite3_bind_int64(stmt_remove_signed_pre_key, 1, (sqlite3_int64)signed_pre_key_id);

    return prof_db_step_done(omemo_db, stmt_remove_signed_pre_key);
}

gint
omemo_db_load_all_signed_pre_keys(GHashTable* signed_pre_key_store, uint32_t* signed_pre_key_id)
{
    if (!omemo_db || !stmt_load_all_signed_pre_keys || !signed_pre_key_store) {
        return 0;
    }

    gint count = 0;
    uint32_t max_id = 0;

    while (sqlite3_step(stmt_load_all_signed_pre_keys) == SQLITE_ROW) {
        uint32_t id = (uint32_t)sqlite3_column_int64(stmt_load_all_signed_pre_keys, 0);
        const void* blob = sqlite3_column_blob(stmt_load_all_signed_pre_keys, 1);
        int bytes = sqlite3_column_bytes(stmt_load_all_signed_pre_keys, 1);

        if (blob && bytes > 0) {
            signal_buffer* buffer = signal_buffer_create((const uint8_t*)blob, (size_t)bytes);
            g_hash_table_insert(signed_pre_key_store, GINT_TO_POINTER(id), buffer);
            if (id > max_id) {
                max_id = id;
            }
            count++;
        }
    }

    sqlite3_reset(stmt_load_all_signed_pre_keys);

    if (signed_pre_key_id && count > 0) {
        *signed_pre_key_id = max_id;
    }

    return count;
}

gboolean
omemo_db_save_session(const char* name, uint32_t device_id, const uint8_t* record, size_t record_len)
{
    if (!omemo_db || !stmt_save_session || !name || !record) {
        return FALSE;
    }

    sqlite3_bind_text(stmt_save_session, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt_save_session, 2, (sqlite3_int64)device_id);
    sqlite3_bind_blob(stmt_save_session, 3, record, (int)record_len, SQLITE_STATIC);

    return prof_db_step_done(omemo_db, stmt_save_session);
}

gboolean
omemo_db_delete_session(const char* name, uint32_t device_id)
{
    if (!omemo_db || !stmt_delete_session || !name) {
        return FALSE;
    }

    sqlite3_bind_text(stmt_delete_session, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt_delete_session, 2, (sqlite3_int64)device_id);

    return prof_db_step_done(omemo_db, stmt_delete_session);
}

gint
omemo_db_delete_all_sessions(const char* name)
{
    if (!omemo_db || !stmt_delete_all_sessions || !name) {
        return -1;
    }

    sqlite3_bind_text(stmt_delete_all_sessions, 1, name, -1, SQLITE_STATIC);

    if (!prof_db_step_done(omemo_db, stmt_delete_all_sessions)) {
        return -1;
    }

    return (gint)sqlite3_changes(omemo_db);
}

void
omemo_db_load_all_sessions(GHashTable* session_store)
{
    if (!omemo_db || !stmt_load_all_sessions || !session_store) {
        return;
    }

    while (sqlite3_step(stmt_load_all_sessions) == SQLITE_ROW) {
        const char* name = (const char*)sqlite3_column_text(stmt_load_all_sessions, 0);
        uint32_t device_id = (uint32_t)sqlite3_column_int64(stmt_load_all_sessions, 1);
        const void* blob = sqlite3_column_blob(stmt_load_all_sessions, 2);
        int bytes = sqlite3_column_bytes(stmt_load_all_sessions, 2);

        if (name && blob && bytes > 0) {
            GHashTable* device_store = g_hash_table_lookup(session_store, name);
            if (!device_store) {
                device_store = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
                g_hash_table_insert(session_store, g_strdup(name), device_store);
            }
            signal_buffer* buffer = signal_buffer_create((const uint8_t*)blob, (size_t)bytes);
            g_hash_table_insert(device_store, GINT_TO_POINTER(device_id), buffer);
        }
    }

    sqlite3_reset(stmt_load_all_sessions);
}

gboolean
omemo_db_save_trust(const char* name, uint32_t device_id, const uint8_t* key, size_t key_len)
{
    if (!omemo_db || !stmt_save_trust || !name || !key) {
        return FALSE;
    }

    sqlite3_bind_text(stmt_save_trust, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt_save_trust, 2, (sqlite3_int64)device_id);
    sqlite3_bind_blob(stmt_save_trust, 3, key, (int)key_len, SQLITE_STATIC);

    return prof_db_step_done(omemo_db, stmt_save_trust);
}

gboolean
omemo_db_remove_trust(const char* name, uint32_t device_id)
{
    if (!omemo_db || !stmt_remove_trust || !name) {
        return FALSE;
    }

    sqlite3_bind_text(stmt_remove_trust, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt_remove_trust, 2, (sqlite3_int64)device_id);

    return prof_db_step_done(omemo_db, stmt_remove_trust);
}

void
omemo_db_load_all_trust(GHashTable* trusted_table)
{
    if (!omemo_db || !stmt_load_all_trust || !trusted_table) {
        return;
    }

    while (sqlite3_step(stmt_load_all_trust) == SQLITE_ROW) {
        const char* name = (const char*)sqlite3_column_text(stmt_load_all_trust, 0);
        uint32_t device_id = (uint32_t)sqlite3_column_int64(stmt_load_all_trust, 1);
        const void* blob = sqlite3_column_blob(stmt_load_all_trust, 2);
        int bytes = sqlite3_column_bytes(stmt_load_all_trust, 2);

        if (name && blob && bytes > 0) {
            GHashTable* trusted = g_hash_table_lookup(trusted_table, name);
            if (!trusted) {
                trusted = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
                g_hash_table_insert(trusted_table, g_strdup(name), trusted);
            }
            signal_buffer* buffer = signal_buffer_create((const uint8_t*)blob, (size_t)bytes);
            g_hash_table_insert(trusted, GINT_TO_POINTER(device_id), buffer);
        }
    }

    sqlite3_reset(stmt_load_all_trust);
}

gboolean
omemo_db_is_contact_verified(const char* barejid)
{
    if (!omemo_db || !stmt_is_verified || !barejid) {
        return FALSE;
    }

    sqlite3_bind_text(stmt_is_verified, 1, barejid, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt_is_verified);
    sqlite3_reset(stmt_is_verified);
    sqlite3_clear_bindings(stmt_is_verified);

    return (rc == SQLITE_ROW);
}

void
omemo_db_set_contact_verified(const char* barejid, gboolean verified)
{
    if (!omemo_db || !barejid) {
        return;
    }

    if (verified) {
        if (!stmt_set_verified) {
            return;
        }
        sqlite3_bind_text(stmt_set_verified, 1, barejid, -1, SQLITE_STATIC);
        prof_db_step_done(omemo_db, stmt_set_verified);
    } else {
        if (!stmt_remove_verified) {
            return;
        }
        sqlite3_bind_text(stmt_remove_verified, 1, barejid, -1, SQLITE_STATIC);
        prof_db_step_done(omemo_db, stmt_remove_verified);
    }
}

gboolean
omemo_db_save_device(const char* name, uint32_t device_id, const char* fingerprint)
{
    if (!omemo_db || !stmt_save_device || !name || !fingerprint) {
        return FALSE;
    }

    sqlite3_bind_text(stmt_save_device, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt_save_device, 2, (sqlite3_int64)device_id);
    sqlite3_bind_text(stmt_save_device, 3, fingerprint, -1, SQLITE_STATIC);

    return prof_db_step_done(omemo_db, stmt_save_device);
}

void
omemo_db_load_all_known_devices(GHashTable* known_devices, GHashTable* fingerprint_ac)
{
    if (!omemo_db || !stmt_load_all_devices || !known_devices) {
        return;
    }

    while (sqlite3_step(stmt_load_all_devices) == SQLITE_ROW) {
        const char* name = (const char*)sqlite3_column_text(stmt_load_all_devices, 0);
        uint32_t device_id = (uint32_t)sqlite3_column_int64(stmt_load_all_devices, 1);
        const char* fingerprint = (const char*)sqlite3_column_text(stmt_load_all_devices, 2);

        if (name && fingerprint) {
            GHashTable* known_identities = g_hash_table_lookup(known_devices, name);
            if (!known_identities) {
                known_identities = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
                g_hash_table_insert(known_devices, g_strdup(name), known_identities);
            }
            g_hash_table_insert(known_identities, g_strdup(fingerprint), GINT_TO_POINTER(device_id));

            if (fingerprint_ac) {
                Autocomplete ac = g_hash_table_lookup(fingerprint_ac, name);
                if (ac == NULL) {
                    ac = autocomplete_new();
                    g_hash_table_insert(fingerprint_ac, g_strdup(name), ac);
                }
                char* formatted_fingerprint = omemo_format_fingerprint(fingerprint);
                if (formatted_fingerprint) {
                    autocomplete_add(ac, formatted_fingerprint);
                    g_free(formatted_fingerprint);
                }
            }
        }
    }

    sqlite3_reset(stmt_load_all_devices);
}
