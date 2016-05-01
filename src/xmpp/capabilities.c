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
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
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
#include "xmpp/xmpp.h"
#include "xmpp/stanza.h"
#include "xmpp/form.h"
#include "xmpp/capabilities.h"
#include "plugins/plugins.h"

static gchar *cache_loc;
static GKeyFile *cache;

static GHashTable *jid_to_ver;
static GHashTable *jid_to_caps;

static char *my_sha1;

static gchar* _get_cache_file(void);
static void _save_cache(void);
static Capabilities* _caps_by_ver(const char *const ver);
static Capabilities* _caps_by_jid(const char *const jid);
Capabilities* _caps_copy(Capabilities *caps);

void
caps_init(void)
{
    log_info("Loading capabilities cache");
    cache_loc = _get_cache_file();

    if (g_file_test(cache_loc, G_FILE_TEST_EXISTS)) {
        g_chmod(cache_loc, S_IRUSR | S_IWUSR);
    }

    cache = g_key_file_new();
    g_key_file_load_from_file(cache, cache_loc, G_KEY_FILE_KEEP_COMMENTS,
        NULL);

    jid_to_ver = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    jid_to_caps = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)caps_destroy);

    my_sha1 = NULL;
}

void
caps_add_by_ver(const char *const ver, Capabilities *caps)
{
    gboolean cached = g_key_file_has_group(cache, ver);
    if (!cached) {
        if (caps->name) {
            g_key_file_set_string(cache, ver, "name", caps->name);
        }
        if (caps->category) {
            g_key_file_set_string(cache, ver, "category", caps->category);
        }
        if (caps->type) {
            g_key_file_set_string(cache, ver, "type", caps->type);
        }
        if (caps->software) {
            g_key_file_set_string(cache, ver, "software", caps->software);
        }
        if (caps->software_version) {
            g_key_file_set_string(cache, ver, "software_version", caps->software_version);
        }
        if (caps->os) {
            g_key_file_set_string(cache, ver, "os", caps->os);
        }
        if (caps->os_version) {
            g_key_file_set_string(cache, ver, "os_version", caps->os_version);
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
}

void
caps_add_by_jid(const char *const jid, Capabilities *caps)
{
    g_hash_table_insert(jid_to_caps, strdup(jid), caps);
}

void
caps_map_jid_to_ver(const char *const jid, const char *const ver)
{
    g_hash_table_insert(jid_to_ver, strdup(jid), strdup(ver));
}

gboolean
caps_contains(const char *const ver)
{
    return (g_key_file_has_group(cache, ver));
}

static Capabilities*
_caps_by_ver(const char *const ver)
{
    if (g_key_file_has_group(cache, ver)) {
        Capabilities *new_caps = malloc(sizeof(struct capabilities_t));

        char *category = g_key_file_get_string(cache, ver, "category", NULL);
        if (category) {
            new_caps->category = category;
        } else {
            new_caps->category = NULL;
        }

        char *type = g_key_file_get_string(cache, ver, "type", NULL);
        if (type) {
            new_caps->type = type;
        } else {
            new_caps->type = NULL;
        }

        char *name = g_key_file_get_string(cache, ver, "name", NULL);
        if (name) {
            new_caps->name = name;
        } else {
            new_caps->name = NULL;
        }

        char *software = g_key_file_get_string(cache, ver, "software", NULL);
        if (software) {
            new_caps->software = software;
        } else {
            new_caps->software = NULL;
        }

        char *software_version = g_key_file_get_string(cache, ver, "software_version", NULL);
        if (software_version) {
            new_caps->software_version = software_version;
        } else {
            new_caps->software_version = NULL;
        }

        char *os = g_key_file_get_string(cache, ver, "os", NULL);
        if (os) {
            new_caps->os = os;
        } else {
            new_caps->os = NULL;
        }

        char *os_version = g_key_file_get_string(cache, ver, "os_version", NULL);
        if (os_version) {
            new_caps->os_version = os_version;
        } else {
            new_caps->os_version = NULL;
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
    } else {
        return NULL;
    }
}

static Capabilities*
_caps_by_jid(const char *const jid)
{
    return g_hash_table_lookup(jid_to_caps, jid);
}

Capabilities*
caps_lookup(const char *const jid)
{
    char *ver = g_hash_table_lookup(jid_to_ver, jid);
    if (ver) {
        Capabilities *caps = _caps_by_ver(ver);
        if (caps) {
            log_debug("Capabilities lookup %s, found by verification string %s.", jid, ver);
            return caps;
        }
    } else {
        Capabilities *caps = _caps_by_jid(jid);
        if (caps) {
            log_debug("Capabilities lookup %s, found by JID.", jid);
            return _caps_copy(caps);
        }
    }

    log_debug("Capabilities lookup %s, none found.", jid);
    return NULL;
}

Capabilities*
_caps_copy(Capabilities *caps)
{
    if (!caps) {
        return NULL;
    } else {
        Capabilities *result = (Capabilities *)malloc(sizeof(Capabilities));
        result->category = caps->category ? strdup(caps->category) : NULL;
        result->type = caps->type ? strdup(caps->type) : NULL;
        result->name = caps->name ? strdup(caps->name) : NULL;
        result->software = caps->software ? strdup(caps->software) : NULL;
        result->software_version = caps->software_version ? strdup(caps->software_version) : NULL;
        result->os = caps->os ? strdup(caps->os) : NULL;
        result->os_version = caps->os_version ? strdup(caps->os_version) : NULL;

        result->features = NULL;
        GSList *curr = caps->features;
        while (curr) {
            result->features = g_slist_append(result->features, strdup(curr->data));
            curr = g_slist_next(curr);
        }

        return result;
    }
}

char*
caps_create_sha1_str(xmpp_stanza_t *const query)
{
    const char *category = NULL;
    const char *type = NULL;
    const char *lang = NULL;
    const char *name = NULL;
    const char *feature_str = NULL;
    GSList *identities = NULL;
    GSList *features = NULL;
    GSList *form_names = NULL;
    DataForm *form = NULL;
    FormField *field = NULL;
    GHashTable *forms = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)form_destroy);

    xmpp_stanza_t *child = xmpp_stanza_get_children(query);
    while (child) {
        if (g_strcmp0(xmpp_stanza_get_name(child), STANZA_NAME_IDENTITY) == 0) {
            category = xmpp_stanza_get_attribute(child, "category");
            type = xmpp_stanza_get_attribute(child, "type");
            lang = xmpp_stanza_get_attribute(child, "xml:lang");
            name = xmpp_stanza_get_attribute(child, "name");

            GString *identity_str = g_string_new(category);
            g_string_append(identity_str, "/");
            if (type) {
                g_string_append(identity_str, type);
            }
            g_string_append(identity_str, "/");
            if (lang) {
                g_string_append(identity_str, lang);
            }
            g_string_append(identity_str, "/");
            if (name) {
                g_string_append(identity_str, name);
            }
            g_string_append(identity_str, "<");
            identities = g_slist_insert_sorted(identities, g_strdup(identity_str->str), (GCompareFunc)strcmp);
            g_string_free(identity_str, TRUE);
        } else if (g_strcmp0(xmpp_stanza_get_name(child), STANZA_NAME_FEATURE) == 0) {
            feature_str = xmpp_stanza_get_attribute(child, "var");
            features = g_slist_insert_sorted(features, g_strdup(feature_str), (GCompareFunc)strcmp);
        } else if (g_strcmp0(xmpp_stanza_get_name(child), STANZA_NAME_X) == 0) {
            if (g_strcmp0(xmpp_stanza_get_ns(child), STANZA_NS_DATA) == 0) {
                form = form_create(child);
                char *form_type = form_get_form_type_field(form);
                form_names = g_slist_insert_sorted(form_names, g_strdup(form_type), (GCompareFunc)strcmp);
                g_hash_table_insert(forms, g_strdup(form_type), form);
            }
        }
        child = xmpp_stanza_get_next(child);
    }

    GString *s = g_string_new("");

    GSList *curr = identities;
    while (curr) {
        g_string_append(s, curr->data);
        curr = g_slist_next(curr);
    }

    curr = features;
    while (curr) {
        g_string_append(s, curr->data);
        g_string_append(s, "<");
        curr = g_slist_next(curr);
    }

    curr = form_names;
    while (curr) {
        form = g_hash_table_lookup(forms, curr->data);
        char *form_type = form_get_form_type_field(form);
        g_string_append(s, form_type);
        g_string_append(s, "<");

        GSList *sorted_fields = form_get_non_form_type_fields_sorted(form);
        GSList *curr_field = sorted_fields;
        while (curr_field) {
            field = curr_field->data;
            g_string_append(s, field->var);
            g_string_append(s, "<");

            GSList *sorted_values = form_get_field_values_sorted(field);
            GSList *curr_value = sorted_values;
            while (curr_value) {
                g_string_append(s, curr_value->data);
                g_string_append(s, "<");
                curr_value = g_slist_next(curr_value);
            }
            g_slist_free(sorted_values);
            curr_field = g_slist_next(curr_field);
        }
        g_slist_free(sorted_fields);

        curr = g_slist_next(curr);
    }

    char *result = p_sha1_hash(s->str);

    g_string_free(s, TRUE);
    g_slist_free_full(identities, g_free);
    g_slist_free_full(features, g_free);
    g_slist_free_full(form_names, g_free);
    g_hash_table_destroy(forms);

    return result;
}

Capabilities*
caps_create(xmpp_stanza_t *query)
{
    const char *category = NULL;
    const char *type = NULL;
    const char *name = NULL;
    char *software = NULL;
    char *software_version = NULL;
    char *os = NULL;
    char *os_version = NULL;
    GSList *features = NULL;

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

    if (found) {
        category = xmpp_stanza_get_attribute(found, "category");
        type = xmpp_stanza_get_attribute(found, "type");
        name = xmpp_stanza_get_attribute(found, "name");
    }

    Capabilities *new_caps = malloc(sizeof(struct capabilities_t));

    if (category) {
        new_caps->category = strdup(category);
    } else {
        new_caps->category = NULL;
    }
    if (type) {
        new_caps->type = strdup(type);
    } else {
        new_caps->type = NULL;
    }
    if (name) {
        new_caps->name = strdup(name);
    } else {
        new_caps->name = NULL;
    }
    if (software) {
        new_caps->software = software;
    } else {
        new_caps->software = NULL;
    }
    if (software_version) {
        new_caps->software_version = software_version;
    } else {
        new_caps->software_version = NULL;
    }
    if (os) {
        new_caps->os = os;
    } else {
        new_caps->os = NULL;
    }
    if (os_version) {
        new_caps->os_version = os_version;
    } else {
        new_caps->os_version = NULL;
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
        xmpp_stanza_t *query = caps_create_query_response_stanza(ctx);
        my_sha1 = caps_create_sha1_str(query);
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

xmpp_stanza_t*
caps_create_query_response_stanza(xmpp_ctx_t *const ctx)
{
    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, XMPP_NS_DISCO_INFO);

    xmpp_stanza_t *identity = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(identity, "identity");
    xmpp_stanza_set_attribute(identity, "category", "client");
    xmpp_stanza_set_attribute(identity, "type", "console");

    GString *name_str = g_string_new("Profanity ");
    g_string_append(name_str, PACKAGE_VERSION);
    if (strcmp(PACKAGE_STATUS, "development") == 0) {
#ifdef HAVE_GIT_VERSION
        g_string_append(name_str, "dev.");
        g_string_append(name_str, PROF_GIT_BRANCH);
        g_string_append(name_str, ".");
        g_string_append(name_str, PROF_GIT_REVISION);
#else
        g_string_append(name_str, "dev");
#endif
    }
    xmpp_stanza_set_attribute(identity, "name", name_str->str);
    g_string_free(name_str, TRUE);

    xmpp_stanza_t *feature_caps = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(feature_caps, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_caps, STANZA_ATTR_VAR, STANZA_NS_CAPS);

    xmpp_stanza_t *feature_discoinfo = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(feature_discoinfo, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_discoinfo, STANZA_ATTR_VAR, XMPP_NS_DISCO_INFO);

    xmpp_stanza_t *feature_discoitems = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(feature_discoitems, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_discoitems, STANZA_ATTR_VAR, XMPP_NS_DISCO_ITEMS);

    xmpp_stanza_t *feature_muc = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(feature_muc, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_muc, STANZA_ATTR_VAR, STANZA_NS_MUC);

    xmpp_stanza_t *feature_conference = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(feature_conference, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_conference, STANZA_ATTR_VAR, STANZA_NS_CONFERENCE);

    xmpp_stanza_t *feature_version = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(feature_version, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_version, STANZA_ATTR_VAR, STANZA_NS_VERSION);

    xmpp_stanza_t *feature_chatstates = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(feature_chatstates, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_chatstates, STANZA_ATTR_VAR, STANZA_NS_CHATSTATES);

    xmpp_stanza_t *feature_ping = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(feature_ping, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_ping, STANZA_ATTR_VAR, STANZA_NS_PING);

    xmpp_stanza_t *feature_receipts = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(feature_receipts, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_receipts, STANZA_ATTR_VAR, STANZA_NS_RECEIPTS);

    xmpp_stanza_t *feature_last = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(feature_last, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_last, STANZA_ATTR_VAR, STANZA_NS_LASTACTIVITY);

    xmpp_stanza_add_child(query, identity);

    xmpp_stanza_add_child(query, feature_caps);
    xmpp_stanza_add_child(query, feature_chatstates);
    xmpp_stanza_add_child(query, feature_discoinfo);
    xmpp_stanza_add_child(query, feature_discoitems);
    xmpp_stanza_add_child(query, feature_muc);
    xmpp_stanza_add_child(query, feature_version);
    xmpp_stanza_add_child(query, feature_last);
    xmpp_stanza_add_child(query, feature_conference);
    xmpp_stanza_add_child(query, feature_ping);
    xmpp_stanza_add_child(query, feature_receipts);

    GList *plugin_features = plugins_get_disco_features();
    GList *curr = plugin_features;
    while (curr) {
        xmpp_stanza_t *feature = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(feature, STANZA_NAME_FEATURE);
        xmpp_stanza_set_attribute(feature, STANZA_ATTR_VAR, curr->data);
        xmpp_stanza_add_child(query, feature);
        xmpp_stanza_release(feature);

        curr = g_list_next(curr);
    }

    xmpp_stanza_release(feature_receipts);
    xmpp_stanza_release(feature_ping);
    xmpp_stanza_release(feature_conference);
    xmpp_stanza_release(feature_last);
    xmpp_stanza_release(feature_version);
    xmpp_stanza_release(feature_muc);
    xmpp_stanza_release(feature_discoitems);
    xmpp_stanza_release(feature_discoinfo);
    xmpp_stanza_release(feature_chatstates);
    xmpp_stanza_release(feature_caps);
    xmpp_stanza_release(identity);

    return query;
}

void
caps_close(void)
{
    g_key_file_free(cache);
    cache = NULL;
    g_hash_table_destroy(jid_to_ver);
    g_hash_table_destroy(jid_to_caps);
}

void
caps_destroy(Capabilities *caps)
{
    if (caps) {
        free(caps->category);
        free(caps->type);
        free(caps->name);
        free(caps->software);
        free(caps->software_version);
        free(caps->os);
        free(caps->os_version);
        if (caps->features) {
            g_slist_free_full(caps->features, free);
        }
        free(caps);
    }
}

static gchar*
_get_cache_file(void)
{
    gchar *xdg_data = xdg_get_data_home();
    GString *cache_file = g_string_new(xdg_data);
    g_string_append(cache_file, "/profanity/capscache");
    gchar *result = strdup(cache_file->str);
    g_free(xdg_data);
    g_string_free(cache_file, TRUE);

    return result;
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
