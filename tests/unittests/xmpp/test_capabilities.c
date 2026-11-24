/*
 * test_capabilities.c
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "prof_cmocka.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>

#include "common.h"
#include "helpers.h"
#include "config/files.h"
#include "xmpp/xmpp.h"
#include "xmpp/capabilities.h"
#include "xmpp/test_capabilities.h"

void prof_shutdown(void);
void create_data_dir(void** state);
void remove_data_dir(void** state);

static void
_clean_caps_files(void)
{
    auto_gchar gchar* db_file = files_get_data_path(DIR_DATABASE "/capscache.db");
    auto_gchar gchar* db_wal = files_get_data_path(DIR_DATABASE "/capscache.db-wal");
    auto_gchar gchar* db_shm = files_get_data_path(DIR_DATABASE "/capscache.db-shm");
    auto_gchar gchar* db_dir = files_get_data_path(DIR_DATABASE);
    auto_gchar gchar* legacy_file = files_get_data_path(FILE_CAPSCACHE);
    auto_gchar gchar* migrated_file = g_strdup_printf("%s.migrated", legacy_file);

    g_unlink(db_file);
    g_unlink(db_wal);
    g_unlink(db_shm);
    g_rmdir(db_dir);
    g_unlink(legacy_file);
    g_unlink(migrated_file);
}

void
caps_test__add_and_lookup_by_ver(void** state)
{
    load_preferences(state);
    create_data_dir(state);
    _clean_caps_files();

    caps_init();

    GSList* features = NULL;
    features = g_slist_append(features, "http://jabber.org/protocol/disco#info");
    features = g_slist_append(features, "urn:xmpp:ping");

    EntityCapabilities* caps = caps_create("client", "pc", "ProfTest", "Profanity", "0.20.0", "Linux", "6.6", features);
    g_slist_free(features);

    caps_add_by_ver("test_ver_hash_1", caps);

    assert_true(caps_cache_contains("test_ver_hash_1"));
    assert_false(caps_cache_contains("unknown_ver"));

    caps_map_jid_to_ver("buddy@example.org/desktop", "test_ver_hash_1");

    EntityCapabilities* looked_up = caps_lookup("buddy@example.org/desktop");
    assert_non_null(looked_up);
    assert_non_null(looked_up->identity);
    assert_string_equal(looked_up->identity->category, "client");
    assert_string_equal(looked_up->identity->type, "pc");
    assert_string_equal(looked_up->identity->name, "ProfTest");

    assert_non_null(looked_up->software_version);
    assert_string_equal(looked_up->software_version->software, "Profanity");
    assert_string_equal(looked_up->software_version->software_version, "0.20.0");
    assert_string_equal(looked_up->software_version->os, "Linux");
    assert_string_equal(looked_up->software_version->os_version, "6.6");

    assert_non_null(looked_up->features);
    assert_int_equal(g_slist_length(looked_up->features), 2);

    caps_destroy(caps);
    caps_destroy(looked_up);

    prof_shutdown();
    _clean_caps_files();
    remove_data_dir(state);
    close_preferences(state);
    g_rmdir("./tests/files");
}

void
caps_test__jid_has_feature(void** state)
{
    load_preferences(state);
    create_data_dir(state);
    _clean_caps_files();

    caps_init();

    GSList* features = NULL;
    features = g_slist_append(features, "urn:xmpp:ping");

    EntityCapabilities* caps = caps_create(NULL, NULL, NULL, NULL, NULL, NULL, NULL, features);
    g_slist_free(features);

    caps_add_by_ver("ver_feature_test", caps);
    caps_map_jid_to_ver("contact@server.com/res", "ver_feature_test");

    assert_true(caps_jid_has_feature("contact@server.com/res", "urn:xmpp:ping"));
    assert_false(caps_jid_has_feature("contact@server.com/res", "urn:xmpp:receipts"));
    assert_false(caps_jid_has_feature("nonexistent@server.com/res", "urn:xmpp:ping"));

    caps_destroy(caps);

    prof_shutdown();
    _clean_caps_files();
    remove_data_dir(state);
    close_preferences(state);
    g_rmdir("./tests/files");
}

void
caps_test__legacy_migration(void** state)
{
    load_preferences(state);
    create_data_dir(state);
    _clean_caps_files();

    // Create legacy capscache INI file
    auto_gchar gchar* legacy_path = files_get_data_path(FILE_CAPSCACHE);
    auto_gchar gchar* migrated_path = g_strdup_printf("%s.migrated", legacy_path);

    GKeyFile* kf = g_key_file_new();
    const gchar* const ver = "migrated_hash_test_42";
    g_key_file_set_string(kf, ver, "category", "client");
    g_key_file_set_string(kf, ver, "type", "bot");
    g_key_file_set_string(kf, ver, "name", "TestBot");
    g_key_file_set_string(kf, ver, "software", "BotSoft");
    g_key_file_set_string(kf, ver, "software_version", "2.1");
    g_key_file_set_string(kf, ver, "os", "FreeBSD");
    g_key_file_set_string(kf, ver, "os_version", "14.0");

    const gchar* feats[] = { "urn:xmpp:ping", "http://jabber.org/protocol/muc", NULL };
    g_key_file_set_string_list(kf, ver, "features", feats, 2);

    g_key_file_save_to_file(kf, legacy_path, NULL);
    g_key_file_free(kf);

    assert_true(g_file_test(legacy_path, G_FILE_TEST_IS_REGULAR));

    // Expect the notification displayed in console when migration completes
    auto_gchar gchar* expected_output = g_strdup_printf(
        "Capabilities cache migrated to SQLite database. Legacy file renamed to %s.",
        migrated_path);
    expect_string(cons_show, output, expected_output);

    // Initialize caps: triggers migration
    caps_init();

    // Legacy file must be renamed to .migrated
    assert_false(g_file_test(legacy_path, G_FILE_TEST_EXISTS));
    assert_true(g_file_test(migrated_path, G_FILE_TEST_IS_REGULAR));

    // Data must now be accessible in SQLite
    assert_true(caps_cache_contains(ver));

    caps_map_jid_to_ver("bot@service.org/main", ver);
    EntityCapabilities* looked_up = caps_lookup("bot@service.org/main");
    assert_non_null(looked_up);
    assert_non_null(looked_up->identity);
    assert_string_equal(looked_up->identity->name, "TestBot");
    assert_string_equal(looked_up->identity->category, "client");
    assert_string_equal(looked_up->identity->type, "bot");

    assert_non_null(looked_up->software_version);
    assert_string_equal(looked_up->software_version->software, "BotSoft");
    assert_string_equal(looked_up->software_version->software_version, "2.1");

    assert_true(caps_jid_has_feature("bot@service.org/main", "urn:xmpp:ping"));
    assert_true(caps_jid_has_feature("bot@service.org/main", "http://jabber.org/protocol/muc"));
    assert_false(caps_jid_has_feature("bot@service.org/main", "urn:xmpp:receipts"));

    caps_destroy(looked_up);

    prof_shutdown();
    _clean_caps_files();
    remove_data_dir(state);
    close_preferences(state);
    g_rmdir("./tests/files");
}
