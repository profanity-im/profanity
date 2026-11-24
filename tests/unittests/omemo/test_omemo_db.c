/*
 * test_omemo_db.c
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "prof_cmocka.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "common.h"
#include "helpers.h"
#include "config/account.h"
#include "config/files.h"
#include "database/db.h"
#include "omemo/omemo.h"
#include "omemo/store.h"
#include "omemo/omemo_db.h"
#include "omemo/test_omemo_db.h"

void create_data_dir(void** state);
void remove_data_dir(void** state);

static ProfAccount test_account = {
    .name = "test_account",
    .jid = "testuser@example.org"
};

static void
_clean_omemo_files(void)
{
    auto_gchar gchar* db_file = files_file_in_account_data_path(DIR_DATABASE, test_account.jid, "omemo.db");
    auto_gchar gchar* db_wal = files_file_in_account_data_path(DIR_DATABASE, test_account.jid, "omemo.db-wal");
    auto_gchar gchar* db_shm = files_file_in_account_data_path(DIR_DATABASE, test_account.jid, "omemo.db-shm");
    auto_gchar gchar* db_dir = files_get_account_data_path(DIR_DATABASE, test_account.jid);
    auto_gchar gchar* db_root = files_get_data_path(DIR_DATABASE);

    auto_gchar gchar* omemo_dir = files_file_in_account_data_path(DIR_OMEMO, test_account.jid, NULL);

    if (omemo_dir) {
        const char* const files[] = {
            "identity.txt", "identity.txt.migrated",
            "sessions.txt", "sessions.txt.migrated",
            "trust.txt", "trust.txt.migrated",
            "known_devices.txt", "known_devices.txt.migrated",
            NULL
        };
        for (int i = 0; files[i] != NULL; i++) {
            auto_gchar gchar* f = g_build_filename(omemo_dir, files[i], NULL);
            g_unlink(f);
        }
        g_rmdir(omemo_dir);
    }
    auto_gchar gchar* omemo_root = files_get_data_path(DIR_OMEMO);
    if (omemo_root) {
        g_rmdir(omemo_root);
    }

    g_unlink(db_file);
    g_unlink(db_wal);
    g_unlink(db_shm);
    if (db_dir) {
        g_rmdir(db_dir);
    }
    if (db_root) {
        g_rmdir(db_root);
    }
}

void
omemo_db_test__identity(void** state)
{
    create_data_dir(state);
    _clean_omemo_files();

    assert_true(omemo_db_init(&test_account));
    assert_false(omemo_db_has_identity());

    const uint8_t pub_key[] = { 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
    const uint8_t priv_key[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80 };

    assert_true(omemo_db_save_identity(42, 12345, pub_key, sizeof(pub_key), priv_key, sizeof(priv_key)));
    assert_true(omemo_db_has_identity());

    uint32_t dev_id = 0;
    uint32_t reg_id = 0;
    uint8_t* loaded_pub = NULL;
    size_t loaded_pub_len = 0;
    uint8_t* loaded_priv = NULL;
    size_t loaded_priv_len = 0;

    assert_true(omemo_db_load_identity(&dev_id, &reg_id, &loaded_pub, &loaded_pub_len, &loaded_priv, &loaded_priv_len));
    assert_int_equal(dev_id, 42);
    assert_int_equal(reg_id, 12345);
    assert_int_equal(loaded_pub_len, sizeof(pub_key));
    assert_memory_equal(loaded_pub, pub_key, sizeof(pub_key));
    assert_int_equal(loaded_priv_len, sizeof(priv_key));
    assert_memory_equal(loaded_priv, priv_key, sizeof(priv_key));

    g_free(loaded_pub);
    g_free(loaded_priv);

    // Overwrite identity row with empty BLOB to test corruption handling
    sqlite3* raw_db = NULL;
    auto_gchar gchar* db_path = files_file_in_account_data_path(DIR_DATABASE, test_account.jid, "omemo.db");
    assert_true(prof_db_open(db_path, &raw_db));
    assert_true(prof_db_exec(raw_db, "UPDATE omemo_identity SET public_key = X'' WHERE device_id = 42;"));
    prof_db_close(&raw_db);

    omemo_db_close();
    assert_true(omemo_db_init(&test_account));
    loaded_pub = NULL;
    loaded_priv = NULL;
    assert_false(omemo_db_load_identity(&dev_id, &reg_id, &loaded_pub, &loaded_pub_len, &loaded_priv, &loaded_priv_len));

    omemo_db_close();
    _clean_omemo_files();
    remove_data_dir(state);
    g_rmdir("./tests/files");
}

void
omemo_db_test__pre_keys(void** state)
{
    create_data_dir(state);
    _clean_omemo_files();

    assert_true(omemo_db_init(&test_account));

    const uint8_t pk1[] = { 0xaa, 0xbb, 0xcc };
    const uint8_t pk2[] = { 0x11, 0x22, 0x33, 0x44 };

    assert_true(omemo_db_save_pre_key(1, pk1, sizeof(pk1)));
    assert_true(omemo_db_save_pre_key(2, pk2, sizeof(pk2)));

    GHashTable* pre_key_store = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
    uint32_t max_id = 0;
    gint count = omemo_db_load_all_pre_keys(pre_key_store, &max_id);
    assert_int_equal(count, 2);
    assert_int_equal(max_id, 2);

    signal_buffer* buf1 = g_hash_table_lookup(pre_key_store, GINT_TO_POINTER(1));
    assert_non_null(buf1);
    assert_int_equal(signal_buffer_len(buf1), sizeof(pk1));
    assert_memory_equal(signal_buffer_data(buf1), pk1, sizeof(pk1));

    signal_buffer* buf2 = g_hash_table_lookup(pre_key_store, GINT_TO_POINTER(2));
    assert_non_null(buf2);
    assert_int_equal(signal_buffer_len(buf2), sizeof(pk2));
    assert_memory_equal(signal_buffer_data(buf2), pk2, sizeof(pk2));
    g_hash_table_destroy(pre_key_store);

    assert_true(omemo_db_remove_pre_key(1));

    pre_key_store = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
    max_id = 0;
    count = omemo_db_load_all_pre_keys(pre_key_store, &max_id);
    assert_int_equal(count, 1);
    assert_int_equal(max_id, 2);
    assert_null(g_hash_table_lookup(pre_key_store, GINT_TO_POINTER(1)));
    assert_non_null(g_hash_table_lookup(pre_key_store, GINT_TO_POINTER(2)));
    g_hash_table_destroy(pre_key_store);

    omemo_db_close();
    _clean_omemo_files();
    remove_data_dir(state);
    g_rmdir("./tests/files");
}

void
omemo_db_test__signed_pre_keys(void** state)
{
    create_data_dir(state);
    _clean_omemo_files();

    assert_true(omemo_db_init(&test_account));

    const uint8_t spk[] = { 0xfe, 0xdc, 0xba, 0x98 };
    const uint8_t spk2[] = { 0x12, 0x34, 0x56, 0x78 };

    assert_true(omemo_db_save_signed_pre_key(1, spk, sizeof(spk)));
    assert_true(omemo_db_save_signed_pre_key(5, spk2, sizeof(spk2)));

    GHashTable* spk_store = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
    uint32_t spk_id = 0;
    gint count = omemo_db_load_all_signed_pre_keys(spk_store, &spk_id);
    assert_int_equal(count, 2);
    assert_int_equal(spk_id, 5);

    signal_buffer* sbuf1 = g_hash_table_lookup(spk_store, GINT_TO_POINTER(1));
    assert_non_null(sbuf1);
    assert_int_equal(signal_buffer_len(sbuf1), sizeof(spk));
    assert_memory_equal(signal_buffer_data(sbuf1), spk, sizeof(spk));

    signal_buffer* sbuf2 = g_hash_table_lookup(spk_store, GINT_TO_POINTER(5));
    assert_non_null(sbuf2);
    assert_int_equal(signal_buffer_len(sbuf2), sizeof(spk2));
    assert_memory_equal(signal_buffer_data(sbuf2), spk2, sizeof(spk2));
    g_hash_table_destroy(spk_store);

    assert_true(omemo_db_remove_signed_pre_key(1));

    spk_store = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
    spk_id = 0;
    count = omemo_db_load_all_signed_pre_keys(spk_store, &spk_id);
    assert_int_equal(count, 1);
    assert_int_equal(spk_id, 5);
    assert_null(g_hash_table_lookup(spk_store, GINT_TO_POINTER(1)));
    assert_non_null(g_hash_table_lookup(spk_store, GINT_TO_POINTER(5)));
    g_hash_table_destroy(spk_store);

    omemo_db_close();
    _clean_omemo_files();
    remove_data_dir(state);
    g_rmdir("./tests/files");
}

void
omemo_db_test__sessions(void** state)
{
    create_data_dir(state);
    _clean_omemo_files();

    assert_true(omemo_db_init(&test_account));

    const uint8_t s_alice1[] = { 0x01, 0x02, 0x03 };
    const uint8_t s_alice2[] = { 0x04, 0x05, 0x06 };
    const uint8_t s_bob[] = { 0x07, 0x08, 0x09 };

    assert_true(omemo_db_save_session("alice@example.org", 100, s_alice1, sizeof(s_alice1)));
    assert_true(omemo_db_save_session("alice@example.org", 200, s_alice2, sizeof(s_alice2)));
    assert_true(omemo_db_save_session("bob@example.org", 300, s_bob, sizeof(s_bob)));

    GHashTable* session_store = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_hash_table_destroy);
    omemo_db_load_all_sessions(session_store);
    GHashTable* alice_devs = g_hash_table_lookup(session_store, "alice@example.org");
    assert_non_null(alice_devs);
    assert_int_equal(g_hash_table_size(alice_devs), 2);
    signal_buffer* sbuf_a1 = g_hash_table_lookup(alice_devs, GINT_TO_POINTER(100));
    assert_non_null(sbuf_a1);
    assert_int_equal(signal_buffer_len(sbuf_a1), sizeof(s_alice1));
    assert_memory_equal(signal_buffer_data(sbuf_a1), s_alice1, sizeof(s_alice1));

    assert_non_null(g_hash_table_lookup(session_store, "bob@example.org"));
    g_hash_table_destroy(session_store);

    assert_true(omemo_db_delete_session("alice@example.org", 100));

    session_store = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_hash_table_destroy);
    omemo_db_load_all_sessions(session_store);
    alice_devs = g_hash_table_lookup(session_store, "alice@example.org");
    assert_non_null(alice_devs);
    assert_int_equal(g_hash_table_size(alice_devs), 1);
    assert_null(g_hash_table_lookup(alice_devs, GINT_TO_POINTER(100)));
    assert_non_null(g_hash_table_lookup(alice_devs, GINT_TO_POINTER(200)));
    g_hash_table_destroy(session_store);

    assert_int_equal(omemo_db_delete_all_sessions("alice@example.org"), 1);
    assert_int_equal(omemo_db_delete_all_sessions("alice@example.org"), 0);

    session_store = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_hash_table_destroy);
    omemo_db_load_all_sessions(session_store);
    assert_null(g_hash_table_lookup(session_store, "alice@example.org"));
    assert_non_null(g_hash_table_lookup(session_store, "bob@example.org"));
    g_hash_table_destroy(session_store);

    omemo_db_close();
    _clean_omemo_files();
    remove_data_dir(state);
    g_rmdir("./tests/files");
}

void
omemo_db_test__trust_and_verified(void** state)
{
    create_data_dir(state);
    _clean_omemo_files();

    assert_true(omemo_db_init(&test_account));

    const uint8_t key[] = { 0xde, 0xad, 0xbe, 0xef };
    assert_true(omemo_db_save_trust("contact@example.org", 55, key, sizeof(key)));

    GHashTable* trusted_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_hash_table_destroy);
    omemo_db_load_all_trust(trusted_table);
    GHashTable* dev_table = g_hash_table_lookup(trusted_table, "contact@example.org");
    assert_non_null(dev_table);
    signal_buffer* buf = g_hash_table_lookup(dev_table, GINT_TO_POINTER(55));
    assert_non_null(buf);
    assert_int_equal(signal_buffer_len(buf), sizeof(key));
    assert_memory_equal(signal_buffer_data(buf), key, sizeof(key));
    g_hash_table_destroy(trusted_table);

    assert_false(omemo_db_is_contact_verified("contact@example.org"));
    omemo_db_set_contact_verified("contact@example.org", TRUE);
    assert_true(omemo_db_is_contact_verified("contact@example.org"));
    omemo_db_set_contact_verified("contact@example.org", FALSE);
    assert_false(omemo_db_is_contact_verified("contact@example.org"));

    assert_true(omemo_db_remove_trust("contact@example.org", 55));

    omemo_db_close();
    _clean_omemo_files();
    remove_data_dir(state);
    g_rmdir("./tests/files");
}

void
omemo_db_test__devices(void** state)
{
    create_data_dir(state);
    _clean_omemo_files();

    assert_true(omemo_db_init(&test_account));

    assert_true(omemo_db_save_device("contact@example.org", 1234, "ABCD-1234-EFGH-5678"));

    GHashTable* known_devices = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_hash_table_destroy);
    omemo_db_load_all_known_devices(known_devices, NULL);

    GHashTable* identities = g_hash_table_lookup(known_devices, "contact@example.org");
    assert_non_null(identities);
    assert_int_equal(GPOINTER_TO_UINT(g_hash_table_lookup(identities, "ABCD-1234-EFGH-5678")), 1234);

    g_hash_table_destroy(known_devices);

    omemo_db_close();
    _clean_omemo_files();
    remove_data_dir(state);
    g_rmdir("./tests/files");
}

void
omemo_db_test__legacy_migration(void** state)
{
    create_data_dir(state);
    _clean_omemo_files();

    auto_gchar gchar* omemo_dir = files_file_in_account_data_path(DIR_OMEMO, test_account.jid, NULL);
    assert_non_null(omemo_dir);

    // 1. Create legacy identity.txt
    auto_gchar gchar* identity_path = g_build_filename(omemo_dir, "identity.txt", NULL);
    GKeyFile* kf_id = g_key_file_new();
    g_key_file_set_uint64(kf_id, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_DEVICE_ID, 999);
    g_key_file_set_uint64(kf_id, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_REGISTRATION_ID, 8888);

    const uint8_t test_pub[] = { 0x05, 0x11, 0x22, 0x33, 0x44 };
    const uint8_t test_priv[] = { 0xaa, 0xbb, 0xcc, 0xdd };
    auto_gchar gchar* pub_b64 = g_base64_encode(test_pub, sizeof(test_pub));
    auto_gchar gchar* priv_b64 = g_base64_encode(test_priv, sizeof(test_priv));
    g_key_file_set_string(kf_id, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_IDENTITY_KEY_PUBLIC, pub_b64);
    g_key_file_set_string(kf_id, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_IDENTITY_KEY_PRIVATE, priv_b64);

    const uint8_t test_pk[] = { 0x12, 0x34, 0x56 };
    auto_gchar gchar* pk_b64 = g_base64_encode(test_pk, sizeof(test_pk));
    g_key_file_set_string(kf_id, OMEMO_STORE_GROUP_PREKEYS, "10", pk_b64);

    const uint8_t test_spk[] = { 0x78, 0x9a, 0xbc };
    auto_gchar gchar* spk_b64 = g_base64_encode(test_spk, sizeof(test_spk));
    g_key_file_set_string(kf_id, OMEMO_STORE_GROUP_SIGNED_PREKEYS, "1", spk_b64);

    g_key_file_save_to_file(kf_id, identity_path, NULL);
    g_key_file_free(kf_id);

    // 2. Create legacy sessions.txt
    auto_gchar gchar* sessions_path = g_build_filename(omemo_dir, "sessions.txt", NULL);
    GKeyFile* kf_sess = g_key_file_new();
    const uint8_t test_sess[] = { 0xdd, 0xee, 0xff };
    auto_gchar gchar* sess_b64 = g_base64_encode(test_sess, sizeof(test_sess));
    g_key_file_set_string(kf_sess, "peer@example.org", "77", sess_b64);
    g_key_file_save_to_file(kf_sess, sessions_path, NULL);
    g_key_file_free(kf_sess);

    // 3. Create legacy trust.txt
    auto_gchar gchar* trust_path = g_build_filename(omemo_dir, "trust.txt", NULL);
    GKeyFile* kf_trust = g_key_file_new();
    const uint8_t test_trust[] = { 0x55, 0x66, 0x77 };
    auto_gchar gchar* trust_b64 = g_base64_encode(test_trust, sizeof(test_trust));
    g_key_file_set_string(kf_trust, "peer@example.org", "77", trust_b64);
    g_key_file_set_boolean(kf_trust, OMEMO_TRUST_GROUP_VERIFIED, "peer@example.org", TRUE);
    g_key_file_save_to_file(kf_trust, trust_path, NULL);
    g_key_file_free(kf_trust);

    // 4. Create legacy known_devices.txt
    auto_gchar gchar* devices_path = g_build_filename(omemo_dir, "known_devices.txt", NULL);
    GKeyFile* kf_dev = g_key_file_new();
    g_key_file_set_string(kf_dev, "peer@example.org", "77", "FINGERPRINT-SAMPLE-1234");
    g_key_file_save_to_file(kf_dev, devices_path, NULL);
    g_key_file_free(kf_dev);

    // Expect console notification for migration
    expect_string(cons_show, output,
                  "OMEMO storage migrated to SQLite database (omemo.db). Legacy files renamed to *.migrated.");

    // Initialize omemo_db and run migration
    assert_true(omemo_db_init(&test_account));
    omemo_db_migrate_legacy_files(&test_account);

    // Verify files renamed to *.migrated
    auto_gchar gchar* identity_migrated = g_strdup_printf("%s.migrated", identity_path);
    auto_gchar gchar* sessions_migrated = g_strdup_printf("%s.migrated", sessions_path);
    auto_gchar gchar* trust_migrated = g_strdup_printf("%s.migrated", trust_path);
    auto_gchar gchar* devices_migrated = g_strdup_printf("%s.migrated", devices_path);

    assert_false(g_file_test(identity_path, G_FILE_TEST_EXISTS));
    assert_true(g_file_test(identity_migrated, G_FILE_TEST_IS_REGULAR));
    assert_false(g_file_test(sessions_path, G_FILE_TEST_EXISTS));
    assert_true(g_file_test(sessions_migrated, G_FILE_TEST_IS_REGULAR));
    assert_false(g_file_test(trust_path, G_FILE_TEST_EXISTS));
    assert_true(g_file_test(trust_migrated, G_FILE_TEST_IS_REGULAR));
    assert_false(g_file_test(devices_path, G_FILE_TEST_EXISTS));
    assert_true(g_file_test(devices_migrated, G_FILE_TEST_IS_REGULAR));

    // Verify DB contents
    assert_true(omemo_db_has_identity());
    uint32_t dev_id = 0, reg_id = 0;
    uint8_t* pub_loaded = NULL;
    size_t pub_loaded_len = 0;
    assert_true(omemo_db_load_identity(&dev_id, &reg_id, &pub_loaded, &pub_loaded_len, NULL, NULL));
    assert_int_equal(dev_id, 999);
    assert_int_equal(reg_id, 8888);
    assert_int_equal(pub_loaded_len, sizeof(test_pub));
    assert_memory_equal(pub_loaded, test_pub, sizeof(test_pub));
    g_free(pub_loaded);

    GHashTable* pre_keys = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
    uint32_t max_pre_key = 0;
    assert_int_equal(omemo_db_load_all_pre_keys(pre_keys, &max_pre_key), 1);
    assert_non_null(g_hash_table_lookup(pre_keys, GINT_TO_POINTER(10)));
    g_hash_table_destroy(pre_keys);

    GHashTable* signed_pre_keys = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
    uint32_t signed_id = 0;
    assert_int_equal(omemo_db_load_all_signed_pre_keys(signed_pre_keys, &signed_id), 1);
    assert_non_null(g_hash_table_lookup(signed_pre_keys, GINT_TO_POINTER(1)));
    g_hash_table_destroy(signed_pre_keys);

    GHashTable* sessions = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_hash_table_destroy);
    omemo_db_load_all_sessions(sessions);
    GHashTable* peer_devs = g_hash_table_lookup(sessions, "peer@example.org");
    assert_non_null(peer_devs);
    assert_non_null(g_hash_table_lookup(peer_devs, GINT_TO_POINTER(77)));
    g_hash_table_destroy(sessions);

    assert_true(omemo_db_is_contact_verified("peer@example.org"));

    omemo_db_close();
    _clean_omemo_files();
    remove_data_dir(state);
    g_rmdir("./tests/files");
}
