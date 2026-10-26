/*
 * capabilities.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <strophe.h>

#include "common.h"
#include "log.h"
#include "event/client_events.h"
#include "plugins/plugins.h"
#include "config/files.h"
#include "config/preferences.h"
#include "xmpp/xmpp.h"
#include "xmpp/stanza.h"
#include "xmpp/form.h"
#include "xmpp/capabilities.h"

static gchar* cache_loc;
static prof_keyfile_t caps_prof_keyfile;
static GKeyFile* cache;

static GHashTable* jid_to_ver;
static GHashTable* jid_to_caps;

static GHashTable* prof_features;
static gchar* my_sha1;

static void _save_cache(void);
static EntityCapabilities* _caps_by_ver(const gchar* const ver);
static EntityCapabilities* _caps_by_jid(const gchar* const jid);
static EntityCapabilities* _caps_copy(EntityCapabilities* caps);

static void
_caps_close(void)
{
    caps_reset_ver();
    free_keyfile(&caps_prof_keyfile);
    cache = NULL;
    g_clear_pointer(&jid_to_ver, g_hash_table_destroy);
    g_clear_pointer(&jid_to_caps, g_hash_table_destroy);
    g_clear_pointer(&cache_loc, g_free);
    g_clear_pointer(&prof_features, g_hash_table_destroy);
}

void
caps_init(void)
{
    log_info("Loading capabilities cache");

    prof_add_shutdown_routine(_caps_close);

    load_data_keyfile(&caps_prof_keyfile, FILE_CAPSCACHE);
    cache = caps_prof_keyfile.keyfile;

    jid_to_ver = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    jid_to_caps = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)caps_destroy);

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

    gboolean cached = g_key_file_has_group(cache, ver);
    if (cached) {
        return;
    }

    if (caps->identity) {
        DiscoIdentity* identity = caps->identity;
        if (identity->name) {
            g_key_file_set_string(cache, ver, "name", identity->name);
        }
        if (identity->category) {
            g_key_file_set_string(cache, ver, "category", identity->category);
        }
        if (identity->type) {
            g_key_file_set_string(cache, ver, "type", identity->type);
        }
    }

    if (caps->software_version) {
        SoftwareVersion* software_version = caps->software_version;
        if (software_version->software) {
            g_key_file_set_string(cache, ver, "software", software_version->software);
        }
        if (software_version->software_version) {
            g_key_file_set_string(cache, ver, "software_version", software_version->software_version);
        }
        if (software_version->os) {
            g_key_file_set_string(cache, ver, "os", software_version->os);
        }
        if (software_version->os_version) {
            g_key_file_set_string(cache, ver, "os_version", software_version->os_version);
        }
    }

    if (caps->features) {
        GSList* curr_feature = caps->features;
        int num = g_slist_length(caps->features);
        const gchar* features_list[num];
        int curr = 0;
        while (curr_feature) {
            features_list[curr++] = curr_feature->data;
            curr_feature = g_slist_next(curr_feature);
        }
        g_key_file_set_string_list(cache, ver, "features", features_list, num);
    }

    _save_cache();
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
    return (g_key_file_has_group(cache, ver));
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
    if (!g_key_file_has_group(cache, ver)) {
        return NULL;
    }

    auto_gchar gchar* category = g_key_file_get_string(cache, ver, "category", NULL);
    auto_gchar gchar* type = g_key_file_get_string(cache, ver, "type", NULL);
    auto_gchar gchar* name = g_key_file_get_string(cache, ver, "name", NULL);

    auto_gchar gchar* software = g_key_file_get_string(cache, ver, "software", NULL);
    auto_gchar gchar* software_version = g_key_file_get_string(cache, ver, "software_version", NULL);
    auto_gchar gchar* os = g_key_file_get_string(cache, ver, "os", NULL);
    auto_gchar gchar* os_version = g_key_file_get_string(cache, ver, "os_version", NULL);

    gsize features_len = 0;
    auto_gcharv gchar** features_list = g_key_file_get_string_list(cache, ver, "features", &features_len, NULL);
    GSList* features = NULL;
    if (features_list && features_len > 0) {
        for (gsize i = 0; i < features_len; i++) {
            features = g_slist_append(features, features_list[i]);
        }
    }

    EntityCapabilities* result = caps_create(
        category, type, name,
        software, software_version, os, os_version,
        features);

    g_slist_free(features);

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

    const gchar* const categoty = caps->identity ? caps->identity->category : NULL;
    const gchar* const type = caps->identity ? caps->identity->type : NULL;
    const gchar* const name = caps->identity ? caps->identity->name : NULL;

    const gchar* const software = caps->software_version ? caps->software_version->software : NULL;
    const gchar* const software_version = caps->software_version ? caps->software_version->software_version : NULL;
    const gchar* const os = caps->software_version ? caps->software_version->os : NULL;
    const gchar* const os_version = caps->software_version ? caps->software_version->os_version : NULL;

    return caps_create(categoty, type, name, software, software_version, os, os_version, caps->features);
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

static void
_save_cache(void)
{
    save_keyfile(&caps_prof_keyfile);
}
