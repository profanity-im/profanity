/*
 * capabilities.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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

#include "config.h"

#ifdef HAVE_GIT_VERSION
#include "gitversion.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#ifdef HAVE_LIBMESODE
#include <mesode.h>
#endif

#ifdef HAVE_LIBSTROPHE
#include <strophe.h>
#endif

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

static char *cache_loc;
static GKeyFile *cache;

static GHashTable *jid_to_ver;
static GHashTable *jid_to_caps;

static GHashTable *prof_features;
static char *my_sha1;

static void _save_cache(void);
static EntityCapabilities* _caps_by_ver(const char *const ver);
static EntityCapabilities* _caps_by_jid(const char *const jid);
static EntityCapabilities* _caps_copy(EntityCapabilities *caps);

void
caps_init(void)
{
    log_info("Loading capabilities cache");
    cache_loc = files_get_data_path(FILE_CAPSCACHE);

    if (g_file_test(cache_loc, G_FILE_TEST_EXISTS)) {
        g_chmod(cache_loc, S_IRUSR | S_IWUSR);
    }

    cache = g_key_file_new();
    g_key_file_load_from_file(cache, cache_loc, G_KEY_FILE_KEEP_COMMENTS, NULL);

    jid_to_ver = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
    jid_to_caps = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)caps_destroy);

    prof_features = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
    g_hash_table_add(prof_features, strdup(STANZA_NS_CAPS));
    g_hash_table_add(prof_features, strdup(XMPP_NS_DISCO_INFO));
    g_hash_table_add(prof_features, strdup(XMPP_NS_DISCO_ITEMS));
    g_hash_table_add(prof_features, strdup(STANZA_NS_MUC));
    g_hash_table_add(prof_features, strdup(STANZA_NS_CONFERENCE));
    g_hash_table_add(prof_features, strdup(STANZA_NS_VERSION));
    g_hash_table_add(prof_features, strdup(STANZA_NS_CHATSTATES));
    g_hash_table_add(prof_features, strdup(STANZA_NS_PING));
    if (prefs_get_boolean(PREF_RECEIPTS_SEND)) {
        g_hash_table_add(prof_features, strdup(STANZA_NS_RECEIPTS));
    }
    if (prefs_get_boolean(PREF_LASTACTIVITY)) {
        g_hash_table_add(prof_features, strdup(STANZA_NS_LASTACTIVITY));
    }
    my_sha1 = NULL;
}

void
caps_add_feature(char *feature)
{
    if (g_hash_table_contains(prof_features, feature)) {
        return;
    }

    g_hash_table_add(prof_features, strdup(feature));

    caps_reset_ver();

    // resend presence to update server's disco info data for this client
    if (connection_get_status() == JABBER_CONNECTED) {
        resource_presence_t last_presence = accounts_get_last_presence(session_get_account_name());
        cl_ev_presence_send(last_presence, 0);
    }
}

void
caps_remove_feature(char *feature)
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
    GList *result = NULL;

    GList *features_as_list = g_hash_table_get_keys(prof_features);
    GList *curr = features_as_list;
    while (curr) {
        result = g_list_append(result, strdup(curr->data));
        curr = g_list_next(curr);
    }
    g_list_free(features_as_list);

    GList *plugin_features = plugins_get_disco_features();
    curr = plugin_features;
    while (curr) {
        result = g_list_append(result, strdup(curr->data));
        curr = g_list_next(curr);
    }

    g_list_free(plugin_features);

    return result;
}

EntityCapabilities*
caps_create(const char *const category, const char *const type, const char *const name,
    const char *const software, const char *const software_version,
    const char *const os, const char *const os_version,
    GSList *features)
{
    EntityCapabilities *result = (EntityCapabilities *)malloc(sizeof(EntityCapabilities));

    if (category || type || name) {
        DiscoIdentity *identity = (DiscoIdentity*)malloc(sizeof(DiscoIdentity));
        identity->category = category ? strdup(category) : NULL;
        identity->type = type ? strdup(type) : NULL;
        identity->name = name ? strdup(name) : NULL;
        result->identity = identity;
    } else {
        result->identity = NULL;
    }

    if (software || software_version || os || os_version) {
        SoftwareVersion *software_versionp = (SoftwareVersion*)malloc(sizeof(SoftwareVersion));
        software_versionp->software = software ? strdup(software) : NULL;
        software_versionp->software_version = software_version ? strdup(software_version) : NULL;
        software_versionp->os = os ? strdup(os) : NULL;
        software_versionp->os_version = os_version ? strdup(os_version) : NULL;
        result->software_version = software_versionp;
    } else {
        result->software_version = NULL;
    }

    result->features = NULL;
    GSList *curr = features;
    while (curr) {
        result->features = g_slist_append(result->features, strdup(curr->data));
        curr = g_slist_next(curr);
    }

    return result;
}

void
caps_add_by_ver(const char *const ver, EntityCapabilities *caps)
{
    if (ver == NULL || caps == NULL) {
        return;
    }

    gboolean cached = g_key_file_has_group(cache, ver);
    if (cached) {
        return;
    }

    if (caps->identity) {
        DiscoIdentity *identity = caps->identity;
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
        SoftwareVersion *software_version = caps->software_version;
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
        GSList *curr_feature = caps->features;
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
caps_add_by_jid(const char *const jid, EntityCapabilities *caps)
{
    g_hash_table_insert(jid_to_caps, strdup(jid), caps);
}

void
caps_map_jid_to_ver(const char *const jid, const char *const ver)
{
    g_hash_table_insert(jid_to_ver, strdup(jid), strdup(ver));
}

gboolean
caps_cache_contains(const char *const ver)
{
    return (g_key_file_has_group(cache, ver));
}

EntityCapabilities*
caps_lookup(const char *const jid)
{
    char *ver = g_hash_table_lookup(jid_to_ver, jid);
    if (ver) {
        EntityCapabilities *caps = _caps_by_ver(ver);
        if (caps) {
            log_debug("Capabilities lookup %s, found by verification string %s.", jid, ver);
            return caps;
        }
    } else {
        EntityCapabilities *caps = _caps_by_jid(jid);
        if (caps) {
            log_debug("Capabilities lookup %s, found by JID.", jid);
            return _caps_copy(caps);
        }
    }

    log_debug("Capabilities lookup %s, none found.", jid);
    return NULL;
}

gboolean
caps_jid_has_feature(const char *const jid, const char *const feature)
{
    EntityCapabilities *caps = caps_lookup(jid);

    if (caps == NULL) {
        return FALSE;
    }

    GSList *found = g_slist_find_custom(caps->features, feature, (GCompareFunc)g_strcmp0);
    gboolean result = found != NULL;

    caps_destroy(caps);

    return result;
}

char*
caps_get_my_sha1(xmpp_ctx_t *const ctx)
{
    if (my_sha1 == NULL) {
        xmpp_stanza_t *query = stanza_create_caps_query_element(ctx);
        my_sha1 = stanza_create_caps_sha1_from_query(query);
        xmpp_stanza_release(query);
    }

    return my_sha1;
}

void
caps_reset_ver(void)
{
    if (my_sha1) {
        g_free(my_sha1);
        my_sha1 = NULL;
    }
}

void
caps_close(void)
{
    g_key_file_free(cache);
    cache = NULL;
    g_hash_table_destroy(jid_to_ver);
    g_hash_table_destroy(jid_to_caps);
    free(cache_loc);
    cache_loc = NULL;
    g_hash_table_destroy(prof_features);
    prof_features = NULL;
}

static EntityCapabilities*
_caps_by_ver(const char *const ver)
{
    if (!g_key_file_has_group(cache, ver)) {
        return NULL;
    }

    char *category = g_key_file_get_string(cache, ver, "category", NULL);
    char *type = g_key_file_get_string(cache, ver, "type", NULL);
    char *name = g_key_file_get_string(cache, ver, "name", NULL);

    char *software = g_key_file_get_string(cache, ver, "software", NULL);
    char *software_version = g_key_file_get_string(cache, ver, "software_version", NULL);
    char *os = g_key_file_get_string(cache, ver, "os", NULL);
    char *os_version = g_key_file_get_string(cache, ver, "os_version", NULL);

    gsize features_len = 0;
    gchar **features_list = g_key_file_get_string_list(cache, ver, "features", &features_len, NULL);
    GSList *features = NULL;
    if (features_list && features_len > 0) {
        int i;
        for (i = 0; i < features_len; i++) {
            features = g_slist_append(features, features_list[i]);
        }
    }

    EntityCapabilities *result = caps_create(
        category, type, name,
        software, software_version, os, os_version,
        features);

    g_free(category);
    g_free(type);
    g_free(name);
    g_free(software);
    g_free(software_version);
    g_free(os);
    g_free(os_version);
    if (features_list) {
        g_strfreev(features_list);
    }
    g_slist_free(features);

    return result;
}

static EntityCapabilities*
_caps_by_jid(const char *const jid)
{
    return g_hash_table_lookup(jid_to_caps, jid);
}

static EntityCapabilities*
_caps_copy(EntityCapabilities *caps)
{
    if (!caps) {
        return NULL;
    }

    const char *const categoty = caps->identity ? caps->identity->category : NULL;
    const char *const type = caps->identity ? caps->identity->type : NULL;
    const char *const name = caps->identity ? caps->identity->name : NULL;

    const char *const software = caps->software_version ? caps->software_version->software : NULL;
    const char *const software_version = caps->software_version ? caps->software_version->software_version : NULL;
    const char *const os = caps->software_version ? caps->software_version->os : NULL;
    const char *const os_version = caps->software_version ? caps->software_version->os_version : NULL;

    return caps_create(categoty, type, name, software, software_version, os, os_version, caps->features);
}

static void
_disco_identity_destroy(DiscoIdentity *disco_identity)
{
    if (disco_identity) {
        free(disco_identity->category);
        free(disco_identity->name);
        free(disco_identity->type);
        free(disco_identity);
    }
}

static void
_software_version_destroy(SoftwareVersion *software_version)
{
    if (software_version) {
        free(software_version->software);
        free(software_version->software_version);
        free(software_version->os);
        free(software_version->os_version);
        free(software_version);
    }
}

void
caps_destroy(EntityCapabilities *caps)
{
    if (caps) {
        _disco_identity_destroy(caps->identity);
        _software_version_destroy(caps->software_version);
        if (caps->features) {
            g_slist_free_full(caps->features, free);
        }
        free(caps);
    }
}

static void
_save_cache(void)
{
    gsize g_data_size;
    gchar *g_cache_data = g_key_file_to_data(cache, &g_data_size, NULL);
    g_file_set_contents(cache_loc, g_cache_data, g_data_size, NULL);
    g_chmod(cache_loc, S_IRUSR | S_IWUSR);
    g_free(g_cache_data);
}
