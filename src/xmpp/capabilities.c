/*
 * capabilities.c
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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
#include "plugins/plugins.h"
#include "config/files.h"
#include "xmpp/xmpp.h"
#include "xmpp/stanza.h"
#include "xmpp/form.h"
#include "xmpp/capabilities.h"

static char *cache_loc;
static GKeyFile *cache;

static GHashTable *jid_to_ver;
static GHashTable *jid_to_caps;

static GList *prof_features;
static char *my_sha1;

static void _save_cache(void);
static EntityCapabilities* _caps_by_ver(const char *const ver);
static EntityCapabilities* _caps_by_jid(const char *const jid);
EntityCapabilities* _caps_copy(EntityCapabilities *caps);

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

    prof_features = NULL;
    prof_features = g_list_append(prof_features, strdup(STANZA_NS_CAPS));
    prof_features = g_list_append(prof_features, strdup(XMPP_NS_DISCO_INFO));
    prof_features = g_list_append(prof_features, strdup(XMPP_NS_DISCO_ITEMS));
    prof_features = g_list_append(prof_features, strdup(STANZA_NS_MUC));
    prof_features = g_list_append(prof_features, strdup(STANZA_NS_CONFERENCE));
    prof_features = g_list_append(prof_features, strdup(STANZA_NS_VERSION));
    prof_features = g_list_append(prof_features, strdup(STANZA_NS_CHATSTATES));
    prof_features = g_list_append(prof_features, strdup(STANZA_NS_PING));
    prof_features = g_list_append(prof_features, strdup(STANZA_NS_RECEIPTS));
    prof_features = g_list_append(prof_features, strdup(STANZA_NS_LASTACTIVITY));

    my_sha1 = NULL;
}

GList*
caps_get_features(void)
{
    GList *result = NULL;

    GList *curr = prof_features;
    while (curr) {
        result = g_list_append(result, curr->data);
        curr = g_list_next(curr);
    }

    GList *plugin_features = plugins_get_disco_features();
    curr = plugin_features;
    while (curr) {
        result = g_list_append(result, curr->data);
        curr = g_list_next(curr);
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

EntityCapabilities*
_caps_copy(EntityCapabilities *caps)
{
    if (!caps) {
        return NULL;
    }

    EntityCapabilities *result = (EntityCapabilities *)malloc(sizeof(EntityCapabilities));

    if (caps->identity) {
        DiscoIdentity *identity = (DiscoIdentity*)malloc(sizeof(DiscoIdentity));
        identity->category = caps->identity->category ? strdup(caps->identity->category) : NULL;
        identity->type = caps->identity->type ? strdup(caps->identity->type) : NULL;
        identity->name = caps->identity->name ? strdup(caps->identity->name) : NULL;
        result->identity = identity;
    } else {
        result->identity = NULL;
    }

    if (caps->software_version) {
        SoftwareVersion *software_version = (SoftwareVersion*)malloc(sizeof(SoftwareVersion));
        software_version->software = caps->software_version->software ? strdup(caps->software_version->software) : NULL;
        software_version->software_version = caps->software_version->software_version ? strdup(caps->software_version->software_version) : NULL;
        software_version->os = caps->software_version->os ? strdup(caps->software_version->os) : NULL;
        software_version->os_version = caps->software_version->os_version ? strdup(caps->software_version->os_version) : NULL;
    } else {
        result->software_version = NULL;
    }

    result->features = NULL;
    GSList *curr = caps->features;
    while (curr) {
        result->features = g_slist_append(result->features, strdup(curr->data));
        curr = g_slist_next(curr);
    }

    return result;
}

EntityCapabilities*
caps_create(xmpp_stanza_t *query)
{
    char *software = NULL;
    char *software_version = NULL;
    char *os = NULL;
    char *os_version = NULL;

    xmpp_stanza_t *softwareinfo = xmpp_stanza_get_child_by_ns(query, STANZA_NS_DATA);
    if (softwareinfo) {
        DataForm *form = form_create(softwareinfo);
        FormField *formField = NULL;

        char *form_type = form_get_form_type_field(form);
        if (g_strcmp0(form_type, STANZA_DATAFORM_SOFTWARE) == 0) {
            GSList *field = form->fields;
            while (field) {
                formField = field->data;
                if (formField->values) {
                    if (strcmp(formField->var, "software") == 0) {
                        software = strdup(formField->values->data);
                    } else if (strcmp(formField->var, "software_version") == 0) {
                        software_version = strdup(formField->values->data);
                    } else if (strcmp(formField->var, "os") == 0) {
                        os = strdup(formField->values->data);
                    } else if (strcmp(formField->var, "os_version") == 0) {
                        os_version = strdup(formField->values->data);
                    }
                }
                field = g_slist_next(field);
            }
        }

        form_destroy(form);
    }

    xmpp_stanza_t *child = xmpp_stanza_get_children(query);
    GSList *identity_stanzas = NULL;
    GSList *features = NULL;
    while (child) {
        if (g_strcmp0(xmpp_stanza_get_name(child), "feature") == 0) {
            features = g_slist_append(features, strdup(xmpp_stanza_get_attribute(child, "var")));
        }
        if (g_strcmp0(xmpp_stanza_get_name(child), "identity") == 0) {
            identity_stanzas = g_slist_append(identity_stanzas, child);
        }

        child = xmpp_stanza_get_next(child);
    }

    // find identity by locale
    const gchar* const *langs = g_get_language_names();
    int num_langs = g_strv_length((gchar**)langs);
    xmpp_stanza_t *found = NULL;
    GSList *curr_identity = identity_stanzas;
    while (curr_identity) {
        xmpp_stanza_t *id_stanza = curr_identity->data;
        const char *stanza_lang = xmpp_stanza_get_attribute(id_stanza, "xml:lang");
        if (stanza_lang) {
            int i = 0;
            for (i = 0; i < num_langs; i++) {
                if (g_strcmp0(langs[i], stanza_lang) == 0) {
                    found = id_stanza;
                    break;
                }
            }
        }
        if (found) {
            break;
        }
        curr_identity = g_slist_next(curr_identity);
    }

    // not lang match, use default with no lang
    if (!found) {
        curr_identity = identity_stanzas;
        while (curr_identity) {
            xmpp_stanza_t *id_stanza = curr_identity->data;
            const char *stanza_lang = xmpp_stanza_get_attribute(id_stanza, "xml:lang");
            if (!stanza_lang) {
                found = id_stanza;
                break;
            }

            curr_identity = g_slist_next(curr_identity);
        }
    }

    // no matching lang, no identity without lang, use first
    if (!found) {
        if (identity_stanzas) {
            found = identity_stanzas->data;
        }
    }

    g_slist_free(identity_stanzas);

    const char *category = NULL;
    const char *type = NULL;
    const char *name = NULL;
    if (found) {
        category = xmpp_stanza_get_attribute(found, "category");
        type = xmpp_stanza_get_attribute(found, "type");
        name = xmpp_stanza_get_attribute(found, "name");
    }

    EntityCapabilities *new_caps = malloc(sizeof(struct entity_capabilities_t));

    if (category || type || name) {
        DiscoIdentity *identity = malloc(sizeof(struct disco_identity_t));
        identity->category = category ? strdup(category) : NULL;
        identity->type = type ? strdup(type) : NULL;
        identity->name = name ? strdup(name) : NULL;
        new_caps->identity = identity;
    } else {
        new_caps->identity = NULL;
    }

    if (software || software_version || os || os_version) {
        SoftwareVersion *software_versionp = malloc(sizeof(struct software_version_t));
        software_versionp->software = software;
        software_versionp->software_version = software_version;
        software_versionp->os = os;
        software_versionp->os_version = os_version;
    }

    if (features) {
        new_caps->features = features;
    } else {
        new_caps->features = NULL;
    }

    return new_caps;
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
    g_list_free_full(prof_features, free);
    prof_features = NULL;
}

static EntityCapabilities*
_caps_by_ver(const char *const ver)
{
    if (!g_key_file_has_group(cache, ver)) {
        return NULL;
    }

    EntityCapabilities *new_caps = malloc(sizeof(struct entity_capabilities_t));

    char *category = g_key_file_get_string(cache, ver, "category", NULL);
    char *type = g_key_file_get_string(cache, ver, "type", NULL);
    char *name = g_key_file_get_string(cache, ver, "name", NULL);
    if (category || type || name) {
        DiscoIdentity *identity = malloc(sizeof(struct disco_identity_t));
        identity->category = category;
        identity->type = type;
        identity->name = name;
        new_caps->identity = identity;
    } else {
        new_caps->identity = NULL;
    }

    char *software = g_key_file_get_string(cache, ver, "software", NULL);
    char *software_version = g_key_file_get_string(cache, ver, "software_version", NULL);
    char *os = g_key_file_get_string(cache, ver, "os", NULL);
    char *os_version = g_key_file_get_string(cache, ver, "os_version", NULL);
    if (software || software_version || os || os_version) {
        SoftwareVersion *software_versionp = malloc(sizeof(struct software_version_t));
        software_versionp->software = software;
        software_versionp->software_version = software_version;
        software_versionp->os = os;
        software_versionp->os_version = os_version;
        new_caps->software_version = software_versionp;
    } else {
        new_caps->software_version = NULL;
    }

    gsize features_len = 0;
    gchar **features = g_key_file_get_string_list(cache, ver, "features", &features_len, NULL);
    if (features && features_len > 0) {
        GSList *features_list = NULL;
        int i;
        for (i = 0; i < features_len; i++) {
            features_list = g_slist_append(features_list, strdup(features[i]));
        }
        new_caps->features = features_list;
        g_strfreev(features);
    } else {
        new_caps->features = NULL;
    }
    return new_caps;
}

static EntityCapabilities*
_caps_by_jid(const char *const jid)
{
    return g_hash_table_lookup(jid_to_caps, jid);
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
